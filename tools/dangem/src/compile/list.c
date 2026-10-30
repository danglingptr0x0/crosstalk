#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/file.h>
#include <dangling/io/path.h>
#include <dangling/str/str.h>

#include "compile/list.h"
#include "run/run.h"

static uint32_t list_key_find(const char *key, uint64_t len, uint32_t *out_key, uint32_t *out_kind)
{
    const char *const key_name[DANGEM_COMPILE_LIST_KEY_CUNT] = { "root", "bin", "tgt", "steam", "ldginc", "ldglib", "out", "std", "cflagset", "ldflagset", "libset" };
    const char *const kind_name[DANGEM_COMPILE_LIST_KIND_CUNT] = { "src", "def", "inc", "cflag", "lib", "ldflag" };
    uint32_t idx = 0;

    *out_key = UINT32_MAX;
    *out_kind = UINT32_MAX;

    for (idx = 0; idx < (uint32_t)DANGEM_COMPILE_LIST_KEY_CUNT; idx++) { if (strlen(key_name[idx]) == len && memcmp(key_name[idx], key, len) == 0)
        {
            *out_key = idx;
            return LDG_ERR_AOK;
        }
    }

    for (idx = 0; idx < (uint32_t)DANGEM_COMPILE_LIST_KIND_CUNT; idx++) { if (strlen(kind_name[idx]) == len && memcmp(kind_name[idx], key, len) == 0)
        {
            *out_kind = idx;
            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t list_line_take(dangem_compile_list_t *list, uint64_t hd, uint64_t tail)
{
    uint64_t sp = hd;
    uint32_t key = UINT32_MAX;
    uint32_t kind = UINT32_MAX;

    while (sp < tail && list->text[sp] != ' ') { sp = sp + DANGEM_COMPILE_LIST_STEP; }

    if (sp >= tail) { return LDG_ERR_AOK; }

    if (list_key_find(&list->text[hd], sp - hd, &key, &kind) != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    sp = sp + DANGEM_COMPILE_LIST_STEP;

    if (sp >= tail) { return LDG_ERR_AOK; }

    if (key != UINT32_MAX)
    {
        list->key_off[key] = (uint32_t)sp;
        return LDG_ERR_AOK;
    }

    if (list->item_cunt >= (uint32_t)DANGEM_COMPILE_LIST_ITEM_MAX) { return LDG_ERR_FULL; }

    list->item_off[list->item_cunt] = (uint32_t)sp;
    list->item_kind[list->item_cunt] = (uint8_t)kind;
    list->item_cunt = list->item_cunt + DANGEM_COMPILE_LIST_STEP;

    if (kind == (uint32_t)DANGEM_COMPILE_LIST_KIND_SRC) { list->src_cunt = list->src_cunt + DANGEM_COMPILE_LIST_STEP; }

    return LDG_ERR_AOK;
}

uint32_t dangem_compile_list_ld(dangem_compile_list_t *list, const char *path)
{
    ldg_io_file_t *file = 0x0;
    uint64_t got = 0;
    uint64_t pos = 0;
    uint64_t hd = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(list == 0x0 || path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    list->text_len = 0;
    list->item_cunt = 0;
    list->src_cunt = 0;

    for (idx = 0; idx < (uint32_t)DANGEM_COMPILE_LIST_KEY_CUNT; idx++) { list->key_off[idx] = UINT32_MAX; }

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, DANGEM_COMPILE_LIST_OPEN_MODE, &file);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_rd(file, list->text, (uint64_t)(DANGEM_COMPILE_LIST_TEXT_CAP - LDG_STR_TERM_SIZE), &got);

    if (ldg_io_file_close(file) != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    if (ret != LDG_ERR_AOK) { return ret; }

    list->text_len = got;
    list->text[got] = LDG_STR_TERM;

    for (pos = 0; pos <= got; pos++)
    {
        if (pos != got && list->text[pos] != '\n' && list->text[pos] != '\r') { continue; }

        list->text[pos] = LDG_STR_TERM;

        if (pos > hd)
        {
            ret = list_line_take(list, hd, pos);
            if (ret != LDG_ERR_AOK) { return ret; }
        }

        hd = pos + DANGEM_COMPILE_LIST_STEP;
    }

    return LDG_ERR_AOK;
}

uint32_t dangem_compile_list_key_get(const dangem_compile_list_t *list, uint32_t key, const char **out)
{
    if (LDG_UNLIKELY(list == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (key >= (uint32_t)DANGEM_COMPILE_LIST_KEY_CUNT) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (list->key_off[key] == UINT32_MAX || list->text[list->key_off[key]] == LDG_STR_TERM) { return LDG_ERR_NOT_FOUND; }

    *out = &list->text[list->key_off[key]];

    return LDG_ERR_AOK;
}

static uint32_t list_splice(const char *src, uint64_t at, uint64_t cut, const char *repl, char *dst, uint64_t cap)
{
    uint64_t repl_len = (uint64_t)strlen(repl);
    uint64_t tail_len = (uint64_t)strlen(src + at + cut);

    if (at + repl_len + tail_len + LDG_STR_TERM_SIZE > cap) { return LDG_ERR_STR_TRUNC; }

    memcpy(dst, src, at);
    memcpy(dst + at, repl, repl_len);
    memcpy(dst + at + repl_len, src + at + cut, tail_len);
    dst[at + repl_len + tail_len] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint32_t dangem_compile_list_map(const dangem_compile_list_t *list, const char *root, const char *src, char *dst, uint64_t cap)
{
    const uint32_t prefix_key[] = { DANGEM_COMPILE_LIST_KEY_BIN, DANGEM_COMPILE_LIST_KEY_LDGINC, DANGEM_COMPILE_LIST_KEY_STEAM, DANGEM_COMPILE_LIST_KEY_ROOT };
    const char *prefix_dir[] = { 0x0, DANGEM_COMPILE_LIST_DIR_LDG_INC, DANGEM_COMPILE_LIST_DIR_STEAM, 0x0 };
    char repl[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char leaf[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    const char *val = 0x0;
    uint64_t hit = 0;
    uint64_t src_len = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(list == 0x0 || root == 0x0 || src == 0x0 || dst == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = dangem_run_build_dir_get(&prefix_dir[DANGEM_COMPILE_LIST_BUILD_IDX]);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (dangem_run_ldg_bundled_is() == LDG_TRUTH_TRUE && dangem_compile_list_key_get(list, DANGEM_COMPILE_LIST_KEY_LDGLIB, &val) == LDG_ERR_AOK && strcmp(val, src) == 0)
    {
        ret = ldg_io_path_leaf_get(src, leaf, (uint64_t)sizeof(leaf));
        if (ret == LDG_ERR_AOK) { ret = ldg_io_path_join(root, DANGEM_COMPILE_LIST_DIR_LDG_LIB, repl, (uint64_t)sizeof(repl)); }

        if (ret == LDG_ERR_AOK) { ret = ldg_io_path_join(repl, leaf, dst, cap); }

        return ret;
    }

    for (idx = 0; idx < (uint32_t)(sizeof(prefix_key) / sizeof(prefix_key[0])); idx++)
    {
        if (prefix_key[idx] == (uint32_t)DANGEM_COMPILE_LIST_KEY_LDGINC && dangem_run_ldg_bundled_is() != LDG_TRUTH_TRUE) { continue; }

        if (dangem_compile_list_key_get(list, prefix_key[idx], &val) != LDG_ERR_AOK || val[0] == LDG_STR_TERM) { continue; }

        if (ldg_str_substring_find(src, val, (uint64_t)LDG_IO_PATH_MAX, &hit) != LDG_ERR_AOK) { continue; }

        if (prefix_dir[idx] != 0x0) { ret = ldg_io_path_join(root, prefix_dir[idx], repl, (uint64_t)sizeof(repl)); }
        else { ret = ldg_io_path_normalize(root, repl, (uint64_t)sizeof(repl)); }

        if (ret != LDG_ERR_AOK) { return ret; }

        return list_splice(src, hit, (uint64_t)strlen(val), repl, dst, cap);
    }

    src_len = (uint64_t)strlen(src);
    if (src_len + LDG_STR_TERM_SIZE > cap) { return LDG_ERR_STR_TRUNC; }

    memcpy(dst, src, src_len + LDG_STR_TERM_SIZE);

    return LDG_ERR_AOK;
}
