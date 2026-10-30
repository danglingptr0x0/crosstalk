#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/dir.h>
#include <dangling/io/file.h>
#include <dangling/io/path.h>
#include <dangling/str/fmt.h>
#include <dangling/str/str.h>
#include <dangling/sys/info.h>
#include <dangling/sys/proc.h>
#include <dangling/time/time.h>

#include "compile/compile.h"
#include "compile/list.h"
#include "run/run.h"

static uint32_t build_file_wr(const char *path, const char *data, uint64_t len)
{
    ldg_io_file_t *file = 0x0;
    uint64_t wrote = 0;
    uint32_t ret = 0;

    ret = ldg_io_file_open(path, LDG_IO_WR_ONLY | LDG_IO_CREATE | LDG_IO_TRUNCATE, DANGEM_COMPILE_FILE_MODE, &file);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_wr(file, data, len, &wrote);
    if (ret == LDG_ERR_AOK && wrote != len) { ret = LDG_ERR_IO_WR; }

    if (ldg_io_file_close(file) != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    return ret;
}

static uint32_t build_file_rd(const char *path, char *dst, uint64_t cap, uint64_t *out_len)
{
    ldg_io_file_t *file = 0x0;
    uint64_t got = 0;
    uint32_t ret = 0;

    *out_len = 0;
    dst[0] = LDG_STR_TERM;

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, DANGEM_COMPILE_FILE_MODE, &file);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_rd(file, dst, cap - LDG_STR_TERM_SIZE, &got);

    if (ldg_io_file_close(file) != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    if (ret != LDG_ERR_AOK) { return ret; }

    dst[got] = LDG_STR_TERM;
    *out_len = got;

    return LDG_ERR_AOK;
}

static void build_line_cut(char *text)
{
    uint64_t pos = 0;

    while (text[pos] != LDG_STR_TERM && text[pos] != '\n' && text[pos] != '\r') { pos = pos + DANGEM_COMPILE_STEP; }

    text[pos] = LDG_STR_TERM;
}

static uint32_t build_rsp_put(dangem_compile_t *build, uint64_t *io_len, char ch)
{
    if (*io_len + DANGEM_COMPILE_STEP >= (uint64_t)DANGEM_COMPILE_RSP_CAP) { return LDG_ERR_FULL; }

    build->rsp[*io_len] = ch;
    *io_len = *io_len + DANGEM_COMPILE_STEP;

    return LDG_ERR_AOK;
}

static uint32_t build_rsp_arg(dangem_compile_t *build, uint64_t *io_len, const char *prefix, const char *arg)
{
    const char *part[] = { prefix, arg };
    uint32_t idx = 0;
    uint64_t pos = 0;
    uint32_t ret = 0;

    if (arg == 0x0 || arg[0] == LDG_STR_TERM) { return LDG_ERR_AOK; }

    ret = build_rsp_put(build, io_len, '"');

    for (idx = 0; ret == LDG_ERR_AOK && idx < (uint32_t)(sizeof(part) / sizeof(part[0])); idx++)
    {
        if (part[idx] == 0x0) { continue; }

        for (pos = 0; ret == LDG_ERR_AOK && part[idx][pos] != LDG_STR_TERM; pos++)
        {
            if (part[idx][pos] == '"' || part[idx][pos] == '\\') { ret = build_rsp_put(build, io_len, '\\'); }

            if (ret == LDG_ERR_AOK) { ret = build_rsp_put(build, io_len, part[idx][pos]); }
        }
    }

    if (ret == LDG_ERR_AOK) { ret = build_rsp_put(build, io_len, '"'); }

    if (ret == LDG_ERR_AOK) { ret = build_rsp_put(build, io_len, '\n'); }

    return ret;
}

static uint32_t build_rsp_set(dangem_compile_t *build, uint64_t *io_len, uint32_t key)
{
    char tok[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char mapped[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    const char *set = 0x0;
    uint64_t pos = 0;
    uint64_t tok_len = 0;
    uint32_t ret = 0;

    if (dangem_compile_list_key_get(&build->list, key, &set) != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    for (pos = 0; ; pos++)
    {
        if (set[pos] != ' ' && set[pos] != LDG_STR_TERM)
        {
            if (tok_len + DANGEM_COMPILE_STEP >= (uint64_t)sizeof(tok)) { return LDG_ERR_STR_TRUNC; }

            tok[tok_len] = set[pos];
            tok_len = tok_len + DANGEM_COMPILE_STEP;
            continue;
        }

        if (tok_len != 0)
        {
            tok[tok_len] = LDG_STR_TERM;
            ret = dangem_compile_list_map(&build->list, build->root, tok, mapped, (uint64_t)sizeof(mapped));
            if (ret == LDG_ERR_AOK) { ret = build_rsp_arg(build, io_len, 0x0, mapped); }

            if (ret != LDG_ERR_AOK) { return ret; }

            tok_len = 0;
        }

        if (set[pos] == LDG_STR_TERM) { return LDG_ERR_AOK; }
    }
}

static const char* build_item_get(const dangem_compile_t *build, uint32_t item)
{
    return &build->list.text[build->list.item_off[item]];
}

static uint8_t build_suffix_is(const char *text, const char *suffix)
{
    uint64_t text_len = (uint64_t)strlen(text);
    uint64_t suffix_len = (uint64_t)strlen(suffix);

    if (suffix_len > text_len) { return LDG_TRUTH_FALSE; }

    return memcmp(text + text_len - suffix_len, suffix, suffix_len) == 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint8_t build_item_compiled_is(const dangem_compile_t *build, uint32_t item)
{
    if (build->list.item_kind[item] != (uint8_t)DANGEM_COMPILE_LIST_KIND_SRC) { return LDG_TRUTH_FALSE; }

    return build_suffix_is(build_item_get(build, item), DANGEM_COMPILE_RC_EXT) == LDG_TRUTH_TRUE ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE;
}

static uint32_t build_obj_path(const dangem_compile_t *build, const char *src, char *dst, uint64_t cap)
{
    char flat[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t root_len = (uint64_t)strlen(build->root);
    uint64_t pos = 0;
    uint64_t at = 0;

    if (strncmp(src, build->root, root_len) == 0) { pos = root_len; }

    while (src[pos] == '/' || src[pos] == '\\') { pos = pos + DANGEM_COMPILE_STEP; }

    for (; src[pos] != LDG_STR_TERM; pos++)
    {
        if (at + DANGEM_COMPILE_STEP + sizeof(DANGEM_COMPILE_OBJ_EXT) >= (uint64_t)sizeof(flat)) { return LDG_ERR_STR_TRUNC; }

        flat[at] = (src[pos] == '/' || src[pos] == '\\' || src[pos] == ':') ? '_' : src[pos];
        at = at + DANGEM_COMPILE_STEP;
    }

    memcpy(flat + at, DANGEM_COMPILE_OBJ_EXT, sizeof(DANGEM_COMPILE_OBJ_EXT));

    return ldg_io_path_join(build->obj_dir, flat, dst, cap);
}

static uint32_t build_cflags_wr(dangem_compile_t *build)
{
    char mapped[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char local[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    const char *val = 0x0;
    uint64_t len = 0;
    uint64_t local_len = 0;
    uint32_t item = 0;
    uint32_t kind = 0;
    uint32_t ret = 0;

    if (dangem_compile_list_key_get(&build->list, DANGEM_COMPILE_LIST_KEY_STD, &val) == LDG_ERR_AOK) { ret = build_rsp_arg(build, &len, "-std=c", val); }

    if (ret == LDG_ERR_AOK && dangem_compile_list_key_get(&build->list, DANGEM_COMPILE_LIST_KEY_TGT, &val) == LDG_ERR_AOK) { ret = build_rsp_arg(build, &len, "--target=", val); }

    if (ret == LDG_ERR_AOK) { ret = build_rsp_set(build, &len, DANGEM_COMPILE_LIST_KEY_CFLAGSET); }

    if (ret == LDG_ERR_AOK) { ret = build_rsp_arg(build, &len, 0x0, DANGEM_COMPILE_MARCH_FLAG); }

    if (ret == LDG_ERR_AOK) { ret = ldg_io_path_join(build->root, "src", local, (uint64_t)sizeof(local)); }

    local_len = (uint64_t)strlen(local);

    for (item = 0; ret == LDG_ERR_AOK && item < build->list.item_cunt; item++)
    {
        kind = (uint32_t)build->list.item_kind[item];
        if (kind != (uint32_t)DANGEM_COMPILE_LIST_KIND_CFLAG && kind != (uint32_t)DANGEM_COMPILE_LIST_KIND_DEF && kind != (uint32_t)DANGEM_COMPILE_LIST_KIND_INC) { continue; }

        ret = dangem_compile_list_map(&build->list, build->root, build_item_get(build, item), mapped, (uint64_t)sizeof(mapped));
        if (ret != LDG_ERR_AOK) { break; }

        if (kind == (uint32_t)DANGEM_COMPILE_LIST_KIND_CFLAG) { ret = build_rsp_arg(build, &len, 0x0, mapped); }
        else if (kind == (uint32_t)DANGEM_COMPILE_LIST_KIND_DEF) { ret = build_rsp_arg(build, &len, "-D", mapped); }
        else if (strncmp(mapped, local, local_len) == 0) { ret = build_rsp_arg(build, &len, "-I", mapped); }
        else
        {
            ret = build_rsp_arg(build, &len, 0x0, "-isystem");
            if (ret == LDG_ERR_AOK) { ret = build_rsp_arg(build, &len, 0x0, mapped); }
        }
    }

    if (ret == LDG_ERR_AOK && dangem_run_ldg_bundled_is() == LDG_TRUTH_TRUE)
    {
        ret = ldg_io_path_join(build->root, DANGEM_COMPILE_LIST_DIR_LDG_INC, mapped, (uint64_t)sizeof(mapped));
        if (ret == LDG_ERR_AOK) { ret = build_rsp_arg(build, &len, 0x0, "-isystem"); }

        if (ret == LDG_ERR_AOK) { ret = build_rsp_arg(build, &len, 0x0, mapped); }
    }

    if (ret != LDG_ERR_AOK) { return ret; }

    return build_file_wr(build->cflags_rsp, build->rsp, len);
}

static uint32_t build_lib_map(dangem_compile_t *build, const char *val, char *dst, uint64_t cap, uint8_t *out_leaf_is)
{
    const char *ldg = 0x0;

    *out_leaf_is = LDG_TRUTH_FALSE;

    if (dangem_run_shlib_local_is() != LDG_TRUTH_TRUE || ldg_io_path_absolute_is(val) != LDG_TRUTH_TRUE) { return dangem_compile_list_map(&build->list, build->root, val, dst, cap); }

    if (dangem_compile_list_key_get(&build->list, DANGEM_COMPILE_LIST_KEY_LDGLIB, &ldg) == LDG_ERR_AOK && strcmp(ldg, val) == 0) { return dangem_compile_list_map(&build->list, build->root, val, dst, cap); }

    *out_leaf_is = LDG_TRUTH_TRUE;

    return ldg_io_path_leaf_get(val, dst, cap);
}

static uint32_t build_link_wr(dangem_compile_t *build)
{
    char mapped[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    const char *val = 0x0;
    uint64_t len = 0;
    uint64_t at = 0;
    uint32_t item = 0;
    uint32_t ret = 0;

    if (dangem_compile_list_key_get(&build->list, DANGEM_COMPILE_LIST_KEY_TGT, &val) == LDG_ERR_AOK) { ret = build_rsp_arg(build, &len, "--target=", val); }

    if (ret == LDG_ERR_AOK) { ret = build_rsp_arg(build, &len, 0x0, DANGEM_COMPILE_LINKER_FLAG); }

    if (ret == LDG_ERR_AOK && dangem_run_shlib_local_is() == LDG_TRUTH_TRUE) { ret = build_rsp_arg(build, &len, 0x0, DANGEM_COMPILE_RPATH_FLAG); }

    if (ret == LDG_ERR_AOK) { ret = build_rsp_set(build, &len, DANGEM_COMPILE_LIST_KEY_LDFLAGSET); }

    if (ret == LDG_ERR_AOK) { ret = build_rsp_arg(build, &len, 0x0, "-o"); }

    if (ret == LDG_ERR_AOK) { ret = build_rsp_arg(build, &len, 0x0, build->out_path); }

    for (item = 0; ret == LDG_ERR_AOK && item < build->list.item_cunt; item++)
    {
        if (build_item_compiled_is(build, item) != LDG_TRUTH_TRUE) { continue; }

        ret = dangem_compile_list_map(&build->list, build->root, build_item_get(build, item), mapped, (uint64_t)sizeof(mapped));
        if (ret == LDG_ERR_AOK) { ret = build_obj_path(build, mapped, mapped, (uint64_t)sizeof(mapped)); }

        if (ret == LDG_ERR_AOK) { ret = build_rsp_arg(build, &len, 0x0, mapped); }
    }

    for (item = 0; ret == LDG_ERR_AOK && item < build->list.item_cunt; item++)
    {
        if (build->list.item_kind[item] != (uint8_t)DANGEM_COMPILE_LIST_KIND_LDFLAG) { continue; }

        ret = dangem_compile_list_map(&build->list, build->root, build_item_get(build, item), mapped, (uint64_t)sizeof(mapped));
        if (ret == LDG_ERR_AOK) { ret = build_rsp_arg(build, &len, 0x0, mapped); }
    }

    if (ret == LDG_ERR_AOK && dangem_run_ldg_bundled_is() == LDG_TRUTH_TRUE)
    {
        ret = ldg_io_path_join(build->root, DANGEM_COMPILE_LIST_DIR_LDG_LIB, mapped, (uint64_t)sizeof(mapped));
        if (ret == LDG_ERR_AOK) { ret = build_rsp_arg(build, &len, "-L", mapped); }
    }

    if (ret == LDG_ERR_AOK && dangem_run_shlib_local_is() == LDG_TRUTH_TRUE) { ret = build_rsp_arg(build, &len, "-L", build->root); }

    for (item = 0; ret == LDG_ERR_AOK && item < build->list.item_cunt; item++)
    {
        uint8_t leaf_is = LDG_TRUTH_FALSE;

        if (build->list.item_kind[item] != (uint8_t)DANGEM_COMPILE_LIST_KIND_LIB) { continue; }

        val = build_item_get(build, item);

        ret = build_lib_map(build, val, mapped, (uint64_t)sizeof(mapped), &leaf_is);
        if (ret != LDG_ERR_AOK) { break; }

        if (leaf_is == LDG_TRUTH_TRUE) { ret = build_rsp_arg(build, &len, "-l:", mapped); }
        else if (val[0] == '-' || ldg_str_char_find(val, '/', (uint64_t)LDG_IO_PATH_MAX, &at) == LDG_ERR_AOK || ldg_str_char_find(val, '\\', (uint64_t)LDG_IO_PATH_MAX, &at) == LDG_ERR_AOK) { ret = build_rsp_arg(build, &len, 0x0, mapped); }
        else { ret = build_rsp_arg(build, &len, "-l", mapped); }
    }

    if (ret == LDG_ERR_AOK) { ret = build_rsp_set(build, &len, DANGEM_COMPILE_LIST_KEY_LIBSET); }

    if (ret != LDG_ERR_AOK) { return ret; }

    return build_file_wr(build->link_rsp, build->rsp, len);
}

static void build_fail_set(dangem_compile_t *build, const char *src, const char *text, uint32_t len)
{
    uint64_t src_len = 0;
    uint32_t take = len;

    if (build->fail_len != 0) { return; }

    if (take >= (uint32_t)DANGEM_COMPILE_ERR_CAP) { take = (uint32_t)DANGEM_COMPILE_ERR_CAP - LDG_STR_TERM_SIZE; }

    memcpy(build->fail_text, text, take);
    build->fail_text[take] = LDG_STR_TERM;
    build->fail_len = take;

    if (src != 0x0 && ldg_str_fmt(build->fail_src, (uint64_t)sizeof(build->fail_src), &src_len, "%.4095s", src) != LDG_ERR_AOK) { build->fail_src[0] = LDG_STR_TERM; }
}

static void build_job_drain(dangem_compile_job_t *job)
{
    char sink[DANGEM_COMPILE_DRAIN_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t got = 0;
    uint64_t room = 0;
    uint32_t ret = 0;

    do
    {
        got = 0;
        room = (uint64_t)(DANGEM_COMPILE_ERR_CAP - LDG_STR_TERM_SIZE - job->err_len);

        if (room != 0) { ret = ldg_sys_proc_stderr_rd(&job->proc, (ldg_byte_t *)job->err + job->err_len, room, &got); }
        else { ret = ldg_sys_proc_stderr_rd(&job->proc, (ldg_byte_t *)sink, (uint64_t)sizeof(sink), &got); }

        if (ret == LDG_ERR_AOK && room != 0) { job->err_len = job->err_len + (uint32_t)got; }
    } while (ret == LDG_ERR_AOK && got != 0);

    do
    {
        got = 0;
        ret = ldg_sys_proc_stdout_rd(&job->proc, (ldg_byte_t *)sink, (uint64_t)sizeof(sink), &got);
    } while (ret == LDG_ERR_AOK && got != 0);

    job->err[job->err_len] = LDG_STR_TERM;
}

static uint32_t build_job_spawn(dangem_compile_t *build, dangem_compile_job_t *job, const char *const *argv, uint32_t item)
{
    uint32_t ret = 0;

    job->proc = (ldg_sys_proc_t)LDG_STRUCT_ZERO_INIT;
    job->err_len = 0;
    job->err[0] = LDG_STR_TERM;
    job->item = item;

    ret = ldg_sys_proc_spawn(build->cc, argv, build->root, &job->proc);
    if (ret != LDG_ERR_AOK) { return ret; }

    job->live = LDG_TRUTH_TRUE;
    build->live_cunt = build->live_cunt + DANGEM_COMPILE_STEP;

    return LDG_ERR_AOK;
}

static uint32_t build_compile_spawn(dangem_compile_t *build, dangem_compile_job_t *job, uint32_t item)
{
    char src[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char obj[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char rsp[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    const char *argv[DANGEM_COMPILE_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t len = 0;
    uint32_t ret = 0;

    ret = dangem_compile_list_map(&build->list, build->root, build_item_get(build, item), src, (uint64_t)sizeof(src));
    if (ret == LDG_ERR_AOK) { ret = build_obj_path(build, src, obj, (uint64_t)sizeof(obj)); }

    if (ret == LDG_ERR_AOK) { ret = ldg_str_fmt(rsp, (uint64_t)sizeof(rsp), &len, "@%.4094s", build->cflags_rsp); }

    if (ret != LDG_ERR_AOK) { return ret; }

    argv[0] = build->cc;
    argv[1] = rsp;
    argv[2] = "-c";
    argv[3] = src;
    argv[4] = "-o";
    argv[5] = obj;
    argv[6] = 0x0;

    return build_job_spawn(build, job, argv, item);
}

static uint32_t build_link_spawn(dangem_compile_t *build)
{
    char rsp[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    const char *argv[DANGEM_COMPILE_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t len = 0;
    uint32_t ret = 0;

    ret = build_link_wr(build);
    if (ret == LDG_ERR_AOK) { ret = ldg_str_fmt(rsp, (uint64_t)sizeof(rsp), &len, "@%.4094s", build->link_rsp); }

    if (ret != LDG_ERR_AOK) { return ret; }

    argv[0] = build->cc;
    argv[1] = rsp;
    argv[2] = 0x0;

    return build_job_spawn(build, &build->job[0], argv, UINT32_MAX);
}

static uint32_t build_stamp_wr(dangem_compile_t *build)
{
    char stamp[DANGEM_COMPILE_STAMP_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t len = 0;
    uint32_t ret = 0;

    ret = ldg_str_fmt(stamp, (uint64_t)sizeof(stamp), &len, "%.63s\n%u64\n", build->release, build->end_ms - build->start_ms);
    if (ret != LDG_ERR_AOK) { return ret; }

    return build_file_wr(build->stamp_path, stamp, len);
}

static void build_job_reap(dangem_compile_t *build, dangem_compile_job_t *job)
{
    char src[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t code = 0;
    uint32_t ret = 0;

    build_job_drain(job);

    if (ldg_sys_proc_alive_is(&job->proc) == LDG_TRUTH_TRUE) { return; }

    ret = ldg_sys_proc_exit_code_get(&job->proc, &code);
    if (ret == LDG_ERR_AGAIN) { return; }

    build_job_drain(job);

    if (ldg_sys_proc_close(&job->proc) != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    job->live = LDG_TRUTH_FALSE;
    build->live_cunt = build->live_cunt - DANGEM_COMPILE_STEP;

    if (job->item != UINT32_MAX && dangem_compile_list_map(&build->list, build->root, build_item_get(build, job->item), src, (uint64_t)sizeof(src)) != LDG_ERR_AOK) { src[0] = LDG_STR_TERM; }

    if (ret != LDG_ERR_AOK || code != 0)
    {
        build_fail_set(build, job->item != UINT32_MAX ? src : "link", job->err, job->err_len);
        return;
    }

    if (job->item != UINT32_MAX) { build->src_done = build->src_done + DANGEM_COMPILE_STEP; }
}

static uint32_t build_cc_find(dangem_compile_t *build, const char *pform, const char *ext)
{
    char dir[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char sub[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char name[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t len = 0;
    uint32_t ret = 0;

    ret = ldg_str_fmt(name, (uint64_t)sizeof(name), &len, "%.63s%.15s", DANGEM_COMPILE_CC_NAME, ext);
    if (ret == LDG_ERR_AOK) { ret = ldg_io_path_join(build->root, DANGEM_COMPILE_CC_DIR, dir, (uint64_t)sizeof(dir)); }

    if (ret == LDG_ERR_AOK) { ret = ldg_io_path_join(dir, DANGEM_COMPILE_CC_BIN, sub, (uint64_t)sizeof(sub)); }

    if (ret == LDG_ERR_AOK) { ret = ldg_io_path_join(sub, name, build->cc, (uint64_t)sizeof(build->cc)); }

    if (ret == LDG_ERR_AOK && ldg_io_path_exists_is(build->cc) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = ldg_io_path_join(dir, pform, sub, (uint64_t)sizeof(sub));
    if (ret == LDG_ERR_AOK) { ret = ldg_io_path_join(sub, DANGEM_COMPILE_CC_BIN, dir, (uint64_t)sizeof(dir)); }

    if (ret == LDG_ERR_AOK) { ret = ldg_io_path_join(dir, name, build->cc, (uint64_t)sizeof(build->cc)); }

    if (ret == LDG_ERR_AOK && ldg_io_path_exists_is(build->cc) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    return LDG_ERR_NOT_FOUND;
}

static void build_release_rd(dangem_compile_t *build)
{
    char dir[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char text[DANGEM_COMPILE_STAMP_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t len = 0;

    build->release[0] = LDG_STR_TERM;

    if (ldg_io_path_join(build->build_dir, DANGEM_COMPILE_LIST_DIR, dir, (uint64_t)sizeof(dir)) != LDG_ERR_AOK) { return; }

    if (ldg_io_path_join(dir, DANGEM_COMPILE_RELEASE_FILE, path, (uint64_t)sizeof(path)) != LDG_ERR_AOK) { return; }

    if (build_file_rd(path, text, (uint64_t)sizeof(text), &len) != LDG_ERR_AOK) { return; }

    build_line_cut(text);
    if (ldg_str_fmt(build->release, (uint64_t)sizeof(build->release), &len, "%.63s", text) != LDG_ERR_AOK) { build->release[0] = LDG_STR_TERM; }
}

static void build_prev_rd(dangem_compile_t *build)
{
    char text[DANGEM_COMPILE_STAMP_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t ms = 0;
    uint64_t len = 0;
    uint64_t val = 0;

    build->prev_ms = 0;

    if (build_file_rd(build->stamp_path, text, (uint64_t)sizeof(text), &len) != LDG_ERR_AOK) { return; }

    if (ldg_str_char_find(text, '\n', (uint64_t)sizeof(text), &ms) != LDG_ERR_AOK) { return; }

    ms = ms + DANGEM_COMPILE_STEP;
    build_line_cut(text + ms);

    if (ldg_str_uint_parse(text + ms, LDG_BASE_DEC, (uint64_t)sizeof(text) - ms, &val) == LDG_ERR_AOK) { build->prev_ms = val; }
}

static uint32_t build_compiled_cunt_get(const dangem_compile_t *build)
{
    uint32_t item = 0;
    uint32_t cunt = 0;

    for (item = 0; item < build->list.item_cunt; item++) { if (build_item_compiled_is(build, item) == LDG_TRUTH_TRUE) { cunt = cunt + DANGEM_COMPILE_STEP; } }

    return cunt;
}

static uint8_t build_tree_newer_is(const char *dir, uint64_t ref_ns, uint32_t depth)
{
    ldg_io_dir_t *handle = 0x0;
    ldg_io_entry_t entry = LDG_STRUCT_ZERO_INIT;
    ldg_io_info_t info = LDG_STRUCT_ZERO_INIT;
    char path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    const char *name = 0x0;
    uint8_t newer = LDG_TRUTH_FALSE;

    if (depth >= (uint32_t)DANGEM_COMPILE_WALK_DEPTH_MAX) { return LDG_TRUTH_FALSE; }

    if (ldg_io_dir_open(dir, &handle) != LDG_ERR_AOK) { return LDG_TRUTH_FALSE; }

    while (newer != LDG_TRUTH_TRUE && ldg_io_dir_rd(handle, &entry) == LDG_ERR_AOK)
    {
        if (ldg_io_entry_name_get(&entry, &name) != LDG_ERR_AOK || name[0] == DANGEM_COMPILE_HIDDEN_MARK) { continue; }

        if (ldg_io_path_join(dir, name, path, (uint64_t)sizeof(path)) != LDG_ERR_AOK) { continue; }

        if (ldg_io_entry_dir_is(&entry) == LDG_TRUTH_TRUE)
        {
            newer = build_tree_newer_is(path, ref_ns, depth + DANGEM_COMPILE_STEP);
            continue;
        }

        if (ldg_io_path_info_get(path, &info) == LDG_ERR_AOK && info.modified_ns > ref_ns) { newer = LDG_TRUTH_TRUE; }
    }

    if (ldg_io_dir_close(handle) != LDG_ERR_AOK) { return newer; }

    return newer;
}

static void build_src_newer_ck(dangem_compile_t *build)
{
    const char *const root_sub[] = { DANGEM_COMPILE_SRC_DIR, DANGEM_COMPILE_LDG_DIR };
    const char *const build_sub[] = { DANGEM_COMPILE_LIST_DIR, DANGEM_COMPILE_GEN_DIR };
    ldg_io_info_t info = LDG_STRUCT_ZERO_INIT;
    char dir[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t idx = 0;

    build->src_newer = LDG_TRUTH_FALSE;

    if (build->out_path[0] == LDG_STR_TERM || ldg_io_path_info_get(build->out_path, &info) != LDG_ERR_AOK) { return; }

    for (idx = 0; idx < (uint32_t)(sizeof(root_sub) / sizeof(root_sub[0])); idx++) { if (ldg_io_path_join(build->root, root_sub[idx], dir, (uint64_t)sizeof(dir)) == LDG_ERR_AOK && build_tree_newer_is(dir, info.modified_ns, DANGEM_COMPILE_WALK_DEPTH_FIRST) == LDG_TRUTH_TRUE)
        {
            build->src_newer = LDG_TRUTH_TRUE;
            return;
        }
    }

    for (idx = 0; idx < (uint32_t)(sizeof(build_sub) / sizeof(build_sub[0])); idx++) { if (ldg_io_path_join(build->build_dir, build_sub[idx], dir, (uint64_t)sizeof(dir)) == LDG_ERR_AOK && build_tree_newer_is(dir, info.modified_ns, DANGEM_COMPILE_WALK_DEPTH_FIRST) == LDG_TRUTH_TRUE)
        {
            build->src_newer = LDG_TRUTH_TRUE;
            return;
        }
    }
}

uint32_t dangem_compile_init(dangem_compile_t *build, const char *root)
{
    char list_dir[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char list_path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char list_name[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    const char *pform = 0x0;
    const char *ext = 0x0;
    const char *out = 0x0;
    const char *build_dir = 0x0;
    uint64_t len = 0;
    uint32_t cpu = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(build == 0x0 || root == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(build, 0x0, sizeof(*build));
    build->phase = DANGEM_COMPILE_PHASE_IDLE;

    ret = dangem_run_pform_get(&pform, &ext);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_str_fmt(build->root, (uint64_t)sizeof(build->root), &len, "%.4095s", root);
    if (ret == LDG_ERR_AOK) { ret = ldg_str_fmt(list_name, (uint64_t)sizeof(list_name), &len, "%.15s%.15s%.15s", DANGEM_COMPILE_LIST_PREFIX, pform, DANGEM_COMPILE_LIST_EXT); }

    if (ret == LDG_ERR_AOK) { ret = dangem_run_build_dir_get(&build_dir); }

    if (ret == LDG_ERR_AOK) { ret = ldg_io_path_join(root, build_dir, build->build_dir, (uint64_t)sizeof(build->build_dir)); }

    if (ret == LDG_ERR_AOK) { ret = ldg_io_path_join(build->build_dir, DANGEM_COMPILE_OBJ_DIR, build->obj_dir, (uint64_t)sizeof(build->obj_dir)); }

    if (ret == LDG_ERR_AOK) { ret = ldg_io_path_join(build->build_dir, DANGEM_COMPILE_CFLAGS_RSP, build->cflags_rsp, (uint64_t)sizeof(build->cflags_rsp)); }

    if (ret == LDG_ERR_AOK) { ret = ldg_io_path_join(build->build_dir, DANGEM_COMPILE_LINK_RSP, build->link_rsp, (uint64_t)sizeof(build->link_rsp)); }

    if (ret == LDG_ERR_AOK) { ret = ldg_io_path_join(build->build_dir, DANGEM_COMPILE_STAMP_FILE, build->stamp_path, (uint64_t)sizeof(build->stamp_path)); }

    if (ret == LDG_ERR_AOK) { ret = ldg_io_path_join(build->build_dir, DANGEM_COMPILE_LIST_DIR, list_dir, (uint64_t)sizeof(list_dir)); }

    if (ret == LDG_ERR_AOK) { ret = ldg_io_path_join(list_dir, list_name, list_path, (uint64_t)sizeof(list_path)); }

    if (ret != LDG_ERR_AOK) { return ret; }

    build->list_ok = dangem_compile_list_ld(&build->list, list_path) == LDG_ERR_AOK ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    build->cc_ok = build_cc_find(build, pform, ext) == LDG_ERR_AOK ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    build->src_total = build_compiled_cunt_get(build);

    if (build->list_ok == LDG_TRUTH_TRUE && dangem_compile_list_key_get(&build->list, DANGEM_COMPILE_LIST_KEY_OUT, &out) == LDG_ERR_AOK) { ret = ldg_io_path_join(root, out, build->out_path, (uint64_t)sizeof(build->out_path)); }

    if (ret != LDG_ERR_AOK) { return ret; }

    if (ldg_sys_cpu_cunt_get(&cpu) != LDG_ERR_AOK || cpu == 0) { cpu = DANGEM_COMPILE_STEP; }

    build->job_cunt = cpu < (uint32_t)DANGEM_COMPILE_JOB_MAX ? cpu : (uint32_t)DANGEM_COMPILE_JOB_MAX;

    build_release_rd(build);
    build_prev_rd(build);
    build_src_newer_ck(build);

    return LDG_ERR_AOK;
}

uint32_t dangem_compile_start(dangem_compile_t *build)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(build == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (build->list_ok != LDG_TRUTH_TRUE || build->cc_ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (dangem_compile_busy_is(build) == LDG_TRUTH_TRUE) { return LDG_ERR_BUSY; }

    ret = ldg_io_dir_create(build->build_dir, DANGEM_COMPILE_DIR_MODE);
    if (ret != LDG_ERR_AOK && ret != LDG_ERR_EXISTS) { return ret; }

    ret = ldg_io_dir_create(build->obj_dir, DANGEM_COMPILE_DIR_MODE);
    if (ret != LDG_ERR_AOK && ret != LDG_ERR_EXISTS) { return ret; }

    ret = build_cflags_wr(build);
    if (ret != LDG_ERR_AOK) { return ret; }

    build->fail_len = 0;
    build->fail_text[0] = LDG_STR_TERM;
    build->fail_src[0] = LDG_STR_TERM;
    build->item_next = 0;
    build->src_done = 0;
    build->start_ms = ldg_time_epoch_ms_get();
    build->end_ms = build->start_ms;
    build->phase = DANGEM_COMPILE_PHASE_COMPILE;

    return LDG_ERR_AOK;
}

uint32_t dangem_compile_pump(dangem_compile_t *build)
{
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(build == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (dangem_compile_busy_is(build) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    for (idx = 0; idx < build->job_cunt; idx++) { if (build->job[idx].live == LDG_TRUTH_TRUE) { build_job_reap(build, &build->job[idx]); } }

    if (build->phase == DANGEM_COMPILE_PHASE_COMPILE)
    {
        for (idx = 0; build->fail_len == 0 && idx < build->job_cunt; idx++)
        {
            if (build->job[idx].live == LDG_TRUTH_TRUE) { continue; }

            while (build->item_next < build->list.item_cunt && build_item_compiled_is(build, build->item_next) != LDG_TRUTH_TRUE) { build->item_next = build->item_next + DANGEM_COMPILE_STEP; }

            if (build->item_next >= build->list.item_cunt) { break; }

            ret = build_compile_spawn(build, &build->job[idx], build->item_next);
            if (ret != LDG_ERR_AOK) { build_fail_set(build, build_item_get(build, build->item_next), "compiler spawn failed", (uint32_t)(sizeof("compiler spawn failed") - LDG_STR_TERM_SIZE)); }

            build->item_next = build->item_next + DANGEM_COMPILE_STEP;
        }

        if (build->live_cunt != 0) { return LDG_ERR_AOK; }

        if (build->fail_len == 0 && build->item_next < build->list.item_cunt) { return LDG_ERR_AOK; }

        if (build->fail_len != 0)
        {
            build->end_ms = ldg_time_epoch_ms_get();
            build->phase = DANGEM_COMPILE_PHASE_FAIL;
            return LDG_ERR_AOK;
        }

        ret = build_link_spawn(build);
        if (ret != LDG_ERR_AOK)
        {
            build_fail_set(build, "link", "linker spawn failed", (uint32_t)(sizeof("linker spawn failed") - LDG_STR_TERM_SIZE));
            build->end_ms = ldg_time_epoch_ms_get();
            build->phase = DANGEM_COMPILE_PHASE_FAIL;
            return LDG_ERR_AOK;
        }

        build->phase = DANGEM_COMPILE_PHASE_LINK;
        return LDG_ERR_AOK;
    }

    if (build->live_cunt != 0) { return LDG_ERR_AOK; }

    build->end_ms = ldg_time_epoch_ms_get();

    if (build->fail_len != 0)
    {
        build->phase = DANGEM_COMPILE_PHASE_FAIL;
        return LDG_ERR_AOK;
    }

    build->phase = DANGEM_COMPILE_PHASE_DONE;
    build->src_newer = LDG_TRUTH_FALSE;

    return build_stamp_wr(build);
}

uint8_t dangem_compile_stale_is(const dangem_compile_t *build)
{
    char text[DANGEM_COMPILE_STAMP_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t len = 0;

    if (LDG_UNLIKELY(build == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (build->out_path[0] == LDG_STR_TERM || ldg_io_path_exists_is(build->out_path) != LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    if (build->src_newer == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    if (build_file_rd(build->stamp_path, text, (uint64_t)sizeof(text), &len) != LDG_ERR_AOK) { return LDG_TRUTH_TRUE; }

    build_line_cut(text);

    return strcmp(text, build->release) == 0 ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE;
}

uint8_t dangem_compile_busy_is(const dangem_compile_t *build)
{
    if (LDG_UNLIKELY(build == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return (build->phase == DANGEM_COMPILE_PHASE_COMPILE || build->phase == DANGEM_COMPILE_PHASE_LINK) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}
