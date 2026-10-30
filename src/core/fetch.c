#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/dir.h>
#include <dangling/io/file.h>
#include <dangling/io/path.h>
#include <dangling/logging/logging.h>
#include <dangling/str/str.h>
#include <dangling/sys/info.h>

#include "core/fetch.h"
#include "core/log.h"
#include "hdl/proc.h"

#define FETCH_STEP 1
#define FETCH_PAIR 2
#define FETCH_HASH_MUL 33U
#define FETCH_SLASH '/'
#define FETCH_STAR '*'
#define FETCH_TICK '\''
#define FETCH_SLOT_EMPTY 0U
#define FETCH_SLOT_BIAS 1U
#define FETCH_FILE_ACCESS_NONE 0U
#define FETCH_KW_MODULE "module"
#define FETCH_KW_MACROMODULE "macromodule"
#define FETCH_KW_INTERFACE "interface"
#define FETCH_KW_PROGRAM "program"
#define FETCH_KW_PACKAGE "package"
#define FETCH_KW_AUTOMATIC "automatic"
#define FETCH_KW_STATIC "static"
#define FETCH_GIT_SUFFIX ".git"
#define FETCH_GIT_DIR ".git"
#define FETCH_GIT_EXE_WIN "git.exe"
#define FETCH_GIT_EXE_UNIX "git"
#define FETCH_PATH_ENV "PATH"
#define FETCH_LIST_SEP_WIN ';'
#define FETCH_LIST_SEP_UNIX ':'
#define FETCH_SEP_WIN '\\'
#define FETCH_FILE_SEP ","
#define FETCH_SEP_LEN_NONE 0U
#define FETCH_EXT_SV ".sv"
#define FETCH_EXT_V ".v"
#define FETCH_HIDDEN_LEAD '.'
#define FETCH_NAME_HERE "."
#define FETCH_NAME_UP ".."
#define FETCH_EXIT_OK 0U
#define FETCH_ARG_CAP 10
#define FETCH_DEPTH_TEXT "1"
#define FETCH_REASON_NOT_CLONE_TEXT "the directory exists and is not a git clone; it needs removing or another name"
#define FETCH_REASON_PUMP_TEXT "lost contact with git"

static uint64_t fetch_len_get(const char *text)
{
    return (uint64_t)strlen(text);
}

static uint8_t fetch_url_char_ok_is(char ch)
{
    if ((uint8_t)ch <= (uint8_t)LDG_ASCII_SPACE || (uint8_t)ch > (uint8_t)LDG_ASCII_PRINTABLE_MAX) { return LDG_TRUTH_FALSE; }

    if (ch == '"' || ch == '\'' || ch == '\\' || ch == '`') { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

uint8_t crosstalk_core_fetch_url_is(const char *text)
{
    uint64_t lead_len = fetch_len_get(CROSSTALK_CORE_FETCH_URL_LEAD);

    if (LDG_UNLIKELY(text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return strncmp(text, CROSSTALK_CORE_FETCH_URL_LEAD, lead_len) == 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint8_t crosstalk_core_fetch_name_valid_is(const char *name)
{
    uint64_t at = 0;

    if (LDG_UNLIKELY(name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (name[0] == LDG_STR_TERM) { return LDG_TRUTH_FALSE; }

    if (strcmp(name, FETCH_NAME_HERE) == 0 || strcmp(name, FETCH_NAME_UP) == 0) { return LDG_TRUTH_FALSE; }

    for (at = 0; name[at] != LDG_STR_TERM; at = at + FETCH_STEP)
    {
        if (at + LDG_STR_TERM_SIZE >= (uint64_t)CROSSTALK_CORE_FETCH_NAME_CAP) { return LDG_TRUTH_FALSE; }

        if (ldg_char_alnum_is(name[at]) == LDG_TRUTH_TRUE || name[at] == '.' || name[at] == '-' || name[at] == '_') { continue; }

        return LDG_TRUTH_FALSE;
    }

    return LDG_TRUTH_TRUE;
}

static uint32_t fetch_repo_name_get(const char *url, char *out, uint64_t cap)
{
    uint64_t lead_len = fetch_len_get(CROSSTALK_CORE_FETCH_URL_LEAD);
    uint64_t suffix_len = fetch_len_get(FETCH_GIT_SUFFIX);
    uint64_t len = fetch_len_get(url);
    uint64_t start = 0;
    uint64_t name_len = 0;
    uint64_t at = 0;
    uint8_t host_end = LDG_TRUTH_FALSE;

    out[0] = LDG_STR_TERM;

    if (len <= lead_len || len >= (uint64_t)CROSSTALK_CORE_FETCH_URL_CAP) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (at = 0; at < len; at = at + FETCH_STEP) { if (fetch_url_char_ok_is(url[at]) != LDG_TRUTH_TRUE) { return LDG_ERR_FUNC_ARG_INVALID; } }

    while (len > lead_len && url[len - FETCH_STEP] == '/') { len = len - FETCH_STEP; }

    start = len;

    while (start > lead_len && url[start - FETCH_STEP] != '/') { start = start - FETCH_STEP; }

    for (at = lead_len; at + FETCH_STEP < start; at = at + FETCH_STEP) { if (url[at] == '/') { host_end = LDG_TRUTH_TRUE; } }

    if (start <= lead_len || host_end != LDG_TRUTH_TRUE) { return LDG_ERR_FUNC_ARG_INVALID; }

    name_len = len - start;

    if (name_len > suffix_len && memcmp(&url[len - suffix_len], FETCH_GIT_SUFFIX, suffix_len) == 0) { name_len = name_len - suffix_len; }

    if (name_len == 0 || name_len + LDG_STR_TERM_SIZE > cap) { return LDG_ERR_FUNC_ARG_INVALID; }

    memcpy(out, &url[start], name_len);
    out[name_len] = LDG_STR_TERM;

    return crosstalk_core_fetch_name_valid_is(out) == LDG_TRUTH_TRUE ? LDG_ERR_AOK : LDG_ERR_FUNC_ARG_INVALID;
}

static uint32_t fetch_git_find(crosstalk_core_fetch_ctx_t *ctx, char *out, uint64_t cap)
{
    char dir[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    const char *exe = ldg_io_path_sep_get() == FETCH_SEP_WIN ? FETCH_GIT_EXE_WIN : FETCH_GIT_EXE_UNIX;
    char sep = ldg_io_path_sep_get() == FETCH_SEP_WIN ? FETCH_LIST_SEP_WIN : FETCH_LIST_SEP_UNIX;
    uint64_t at = 0;
    uint64_t used = 0;
    uint32_t ret = 0;

    out[0] = LDG_STR_TERM;

    ret = ldg_sys_env_get(FETCH_PATH_ENV, ctx->env, (uint64_t)sizeof(ctx->env));
    if (ret != LDG_ERR_AOK) { return LDG_ERR_NOT_FOUND; }

    for (at = 0; ; at = at + FETCH_STEP)
    {
        char ch = ctx->env[at];

        if (ch != sep && ch != LDG_STR_TERM)
        {
            if (ch != '"' && used + LDG_STR_TERM_SIZE < (uint64_t)sizeof(dir))
            {
                dir[used] = ch;
                used = used + FETCH_STEP;
            }

            continue;
        }

        dir[used] = LDG_STR_TERM;

        if (used != 0 && ldg_io_path_join(dir, exe, out, cap) == LDG_ERR_AOK && ldg_io_path_exists_is(out) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

        used = 0;

        if (ch == LDG_STR_TERM) { break; }
    }

    out[0] = LDG_STR_TERM;

    return LDG_ERR_NOT_FOUND;
}

static void fetch_reason_set(crosstalk_core_fetch_ctx_t *ctx, const char *text, uint64_t len)
{
    uint64_t at = 0;
    uint64_t used = 0;

    for (at = 0; at < len && used + LDG_STR_TERM_SIZE < (uint64_t)sizeof(ctx->reason); at = at + FETCH_STEP)
    {
        if (text[at] == (char)LDG_ASCII_LF || text[at] == (char)LDG_ASCII_CR)
        {
            if (used != 0) { break; }

            continue;
        }

        ctx->reason[used] = ((uint8_t)text[at] < (uint8_t)LDG_ASCII_PRINTABLE_MIN || (uint8_t)text[at] > (uint8_t)LDG_ASCII_PRINTABLE_MAX) ? (char)LDG_ASCII_NONPRINTABLE_CHAR : text[at];
        used = used + FETCH_STEP;
    }

    ctx->reason[used] = LDG_STR_TERM;
}

uint32_t crosstalk_core_fetch_start(crosstalk_core_fetch_ctx_t *ctx, const char *parent, const char *url, const char *top)
{
    char name[CROSSTALK_CORE_FETCH_NAME_CAP] = LDG_ARR_ZERO_INIT;
    char git[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char git_dir[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    const char *argv[FETCH_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || parent == 0x0 || url == 0x0 || top == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->phase == CROSSTALK_CORE_FETCH_PHASE_RUN) { return LDG_ERR_BUSY; }

    ctx->reason[0] = LDG_STR_TERM;
    ctx->exit_code = UINT32_MAX;

    if (fetch_len_get(top) + LDG_STR_TERM_SIZE > (uint64_t)sizeof(ctx->top)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = fetch_repo_name_get(url, name, (uint64_t)sizeof(name));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_path_join(parent, name, ctx->repo_dir, (uint64_t)sizeof(ctx->repo_dir));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_path_join(ctx->repo_dir, FETCH_GIT_DIR, git_dir, (uint64_t)sizeof(git_dir));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = fetch_git_find(ctx, git, (uint64_t)sizeof(git));
    if (ret != LDG_ERR_AOK) { return ret; }

    memcpy(ctx->top, top, fetch_len_get(top) + LDG_STR_TERM_SIZE);

    argv[0] = git;

    if (ldg_io_path_exists_is(git_dir) == LDG_TRUTH_TRUE)
    {
        argv[1] = "-C";
        argv[2] = ctx->repo_dir;
        argv[3] = "pull";
        argv[4] = "--ff-only";
        argv[5] = "--quiet";
        argv[6] = 0x0;
    }
    else if (ldg_io_path_exists_is(ctx->repo_dir) == LDG_TRUTH_TRUE)
    {
        fetch_reason_set(ctx, FETCH_REASON_NOT_CLONE_TEXT, fetch_len_get(FETCH_REASON_NOT_CLONE_TEXT));
        return LDG_ERR_EXISTS;
    }
    else
    {
        argv[1] = "clone";
        argv[2] = "--depth";
        argv[3] = FETCH_DEPTH_TEXT;
        argv[4] = "--quiet";
        argv[5] = "--";
        argv[6] = url;
        argv[7] = ctx->repo_dir;
        argv[8] = 0x0;
    }

    ret = crosstalk_hdl_proc_start(&ctx->proc, git, argv, parent);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->phase = CROSSTALK_CORE_FETCH_PHASE_RUN;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fetch: git started; op: %.15s; dir: %.191s; url: %.191s", argv[1][0] == '-' ? "pull" : "clone", ctx->repo_dir, url);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_fetch_tick(crosstalk_core_fetch_ctx_t *ctx, uint32_t *out_phase)
{
    const ldg_byte_t *text = 0x0;
    uint32_t text_len = 0;
    uint32_t code = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_phase == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_phase = ctx->phase;

    if (ctx->phase != CROSSTALK_CORE_FETCH_PHASE_RUN) { return LDG_ERR_AOK; }

    ret = crosstalk_hdl_proc_pump(&ctx->proc);
    if (ret != LDG_ERR_AOK)
    {
        fetch_reason_set(ctx, FETCH_REASON_PUMP_TEXT, fetch_len_get(FETCH_REASON_PUMP_TEXT));
        ctx->phase = CROSSTALK_CORE_FETCH_PHASE_FAIL;
        *out_phase = ctx->phase;

        if (crosstalk_hdl_proc_stop(&ctx->proc) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fetch: git stop failed"); }

        return ret;
    }

    if (crosstalk_hdl_proc_alive_is(&ctx->proc) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = crosstalk_hdl_proc_exit_code_get(&ctx->proc, &code);
    if (ret == LDG_ERR_AGAIN) { return LDG_ERR_AOK; }

    ctx->exit_code = code;
    ctx->phase = (ret == LDG_ERR_AOK && code == FETCH_EXIT_OK) ? CROSSTALK_CORE_FETCH_PHASE_DONE : CROSSTALK_CORE_FETCH_PHASE_FAIL;

    if (ctx->phase == CROSSTALK_CORE_FETCH_PHASE_FAIL && crosstalk_hdl_proc_err_get(&ctx->proc, &text, &text_len) == LDG_ERR_AOK) { fetch_reason_set(ctx, (const char *)text, (uint64_t)text_len); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fetch: git exited; code: %u32; dir: %.191s", code, ctx->repo_dir);

    if (crosstalk_hdl_proc_stop(&ctx->proc) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fetch: git stop failed"); }

    *out_phase = ctx->phase;

    return LDG_ERR_AOK;
}

static uint8_t fetch_ext_hdl_is(const char *name)
{
    uint64_t len = fetch_len_get(name);
    uint64_t sv_len = fetch_len_get(FETCH_EXT_SV);
    uint64_t v_len = fetch_len_get(FETCH_EXT_V);

    if (len > sv_len && strcmp(&name[len - sv_len], FETCH_EXT_SV) == 0) { return LDG_TRUTH_TRUE; }

    if (len > v_len && strcmp(&name[len - v_len], FETCH_EXT_V) == 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t fetch_rel_join(const char *rel, const char *name, char *out, uint64_t cap)
{
    if (rel[0] == LDG_STR_TERM) { return ldg_str_rbrcpy(out, name, cap); }

    return ldg_io_path_join(rel, name, out, cap);
}

static uint32_t fetch_list_append(char *out, uint64_t cap, uint64_t *io_used, const char *rel)
{
    uint64_t sep_len = *io_used == 0 ? FETCH_SEP_LEN_NONE : fetch_len_get(FETCH_FILE_SEP);
    uint64_t rel_len = fetch_len_get(rel);

    if (*io_used + sep_len + rel_len + LDG_STR_TERM_SIZE > cap) { return LDG_ERR_FULL; }

    memcpy(&out[*io_used], FETCH_FILE_SEP, sep_len);
    memcpy(&out[*io_used + sep_len], rel, rel_len + LDG_STR_TERM_SIZE);

    *io_used = *io_used + sep_len + rel_len;

    return LDG_ERR_AOK;
}

static uint32_t fetch_tree_walk(crosstalk_core_fetch_ctx_t *ctx)
{
    char abs[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char child[CROSSTALK_CORE_FETCH_REL_CAP] = LDG_ARR_ZERO_INIT;
    ldg_io_dir_t *dir = 0x0;
    ldg_io_entry_t ent = LDG_STRUCT_ZERO_INIT;
    uint32_t hd = 0;
    uint32_t tail = 0;
    uint32_t ret = 0;

    ctx->file_cunt = 0;
    ctx->queue[0][0] = LDG_STR_TERM;
    tail = FETCH_STEP;

    while (hd < tail && ret == LDG_ERR_AOK)
    {
        const char *rel = ctx->queue[hd];

        hd = hd + FETCH_STEP;

        if (rel[0] == LDG_STR_TERM) { ret = ldg_str_rbrcpy(abs, ctx->repo_dir, (uint64_t)sizeof(abs)); }
        else { ret = ldg_io_path_join(ctx->repo_dir, rel, abs, (uint64_t)sizeof(abs)); }

        if (ret != LDG_ERR_AOK) { break; }

        ret = ldg_io_dir_open(abs, &dir);
        if (ret != LDG_ERR_AOK) { break; }

        while (ret == LDG_ERR_AOK && ldg_io_dir_rd(dir, &ent) == LDG_ERR_AOK)
        {
            const char *name = 0x0;

            if (ldg_io_entry_name_get(&ent, &name) != LDG_ERR_AOK || name == 0x0 || name[0] == FETCH_HIDDEN_LEAD) { continue; }

            ret = fetch_rel_join(rel, name, child, (uint64_t)sizeof(child));
            if (ret != LDG_ERR_AOK) { break; }

            if (ldg_io_entry_dir_is(&ent) == LDG_TRUTH_TRUE)
            {
                if (tail >= (uint32_t)CROSSTALK_CORE_FETCH_DIR_MAX)
                {
                    ret = LDG_ERR_FULL;
                    break;
                }

                memcpy(ctx->queue[tail], child, fetch_len_get(child) + LDG_STR_TERM_SIZE);
                tail = tail + FETCH_STEP;
                continue;
            }

            if (fetch_ext_hdl_is(name) != LDG_TRUTH_TRUE) { continue; }

            if (ctx->file_cunt >= (uint32_t)CROSSTALK_CORE_FETCH_FILE_MAX)
            {
                ret = LDG_ERR_FULL;
                break;
            }

            memcpy(ctx->file[ctx->file_cunt], child, fetch_len_get(child) + LDG_STR_TERM_SIZE);
            ctx->file_cunt = ctx->file_cunt + FETCH_STEP;
        }

        if (ldg_io_dir_close(dir) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fetch: dir close failed; dir: %.191s", abs); }

        dir = 0x0;
    }

    return ret;
}

static uint32_t fetch_hash_get(const char *text, uint64_t len)
{
    uint32_t hash = (uint32_t)LDG_DJB2_HASH_INIT;
    uint64_t at = 0;

    for (at = 0; at < len; at = at + FETCH_STEP) { hash = hash * FETCH_HASH_MUL + (uint32_t)(uint8_t)text[at]; }

    return hash;
}

static uint8_t fetch_ident_start_is(char ch)
{
    if (ldg_char_alpha_is(ch) == LDG_TRUTH_TRUE || ch == '_') { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t fetch_ident_cont_is(char ch)
{
    if (ldg_char_alnum_is(ch) == LDG_TRUTH_TRUE || ch == '_' || ch == '$') { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t fetch_tok_next(const char *src, uint64_t len, uint64_t *io_pos, uint64_t *out_start, uint64_t *out_len)
{
    uint64_t pos = *io_pos;

    *out_start = 0;
    *out_len = 0;

    while (pos < len)
    {
        char ch = src[pos];
        char nx = pos + FETCH_STEP < len ? src[pos + FETCH_STEP] : (char)LDG_STR_TERM;

        if (ch == FETCH_SLASH && nx == FETCH_SLASH)
        {
            while (pos < len && src[pos] != (char)LDG_ASCII_LF) { pos = pos + FETCH_STEP; }

            continue;
        }

        if (ch == FETCH_SLASH && nx == FETCH_STAR)
        {
            pos = pos + FETCH_PAIR;

            while (pos + FETCH_STEP < len && (src[pos] != FETCH_STAR || src[pos + FETCH_STEP] != FETCH_SLASH)) { pos = pos + FETCH_STEP; }

            pos = pos + FETCH_PAIR;
            continue;
        }

        if (ch == (char)LDG_ASCII_QUOTE)
        {
            pos = pos + FETCH_STEP;

            while (pos < len && src[pos] != (char)LDG_ASCII_QUOTE && src[pos] != (char)LDG_ASCII_LF) { pos = pos + (src[pos] == (char)LDG_ASCII_BSLASH ? FETCH_PAIR : FETCH_STEP); }

            pos = pos + FETCH_STEP;
            continue;
        }

        if (ch == (char)LDG_ASCII_BSLASH)
        {
            while (pos < len && (uint8_t)src[pos] > (uint8_t)LDG_ASCII_SPACE) { pos = pos + FETCH_STEP; }

            continue;
        }

        if (fetch_ident_start_is(ch) == LDG_TRUTH_TRUE)
        {
            *out_start = pos;

            while (pos < len && fetch_ident_cont_is(src[pos]) == LDG_TRUTH_TRUE) { pos = pos + FETCH_STEP; }

            *out_len = pos - *out_start;
            *io_pos = pos;

            return LDG_ERR_AOK;
        }

        if (ldg_char_digit_is(ch) == LDG_TRUTH_TRUE)
        {
            while (pos < len && (fetch_ident_cont_is(src[pos]) == LDG_TRUTH_TRUE || src[pos] == FETCH_TICK)) { pos = pos + FETCH_STEP; }

            continue;
        }

        pos = pos + FETCH_STEP;
    }

    *io_pos = len;

    return LDG_ERR_EOF;
}

static uint8_t fetch_tok_eq_is(const char *src, uint64_t start, uint64_t len, const char *word)
{
    if (fetch_len_get(word) != len) { return LDG_TRUTH_FALSE; }

    return memcmp(&src[start], word, len) == 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t fetch_def_slot_find(const crosstalk_core_fetch_ctx_t *ctx, const char *text, uint64_t len, uint32_t hash, uint32_t *out_slot)
{
    uint32_t slot = hash & (uint32_t)CROSSTALK_CORE_FETCH_DEF_SLOT_MASK;
    uint32_t probe = 0;

    for (probe = 0; probe < (uint32_t)CROSSTALK_CORE_FETCH_DEF_SLOT_CAP; probe = probe + FETCH_STEP)
    {
        uint32_t held = ctx->def_slot[slot];
        const crosstalk_core_fetch_def_t *def = 0x0;

        if (held == FETCH_SLOT_EMPTY)
        {
            *out_slot = slot;
            return LDG_ERR_NOT_FOUND;
        }

        def = &ctx->def[held - FETCH_SLOT_BIAS];

        if (def->hash == hash && fetch_len_get(def->name) == len && memcmp(def->name, text, len) == 0)
        {
            *out_slot = slot;
            return LDG_ERR_AOK;
        }

        slot = (slot + FETCH_STEP) & (uint32_t)CROSSTALK_CORE_FETCH_DEF_SLOT_MASK;
    }

    return LDG_ERR_FULL;
}

static uint32_t fetch_depth_get(const char *rel)
{
    uint32_t depth = 0;
    uint64_t at = 0;

    for (at = 0; rel[at] != LDG_STR_TERM; at = at + FETCH_STEP) { if (rel[at] == FETCH_SLASH || rel[at] == FETCH_SEP_WIN) { depth = depth + FETCH_STEP; } }

    return depth;
}

static uint8_t fetch_file_better_is(const crosstalk_core_fetch_ctx_t *ctx, uint32_t candidate_idx, uint32_t held_idx)
{
    uint32_t candidate_depth = fetch_depth_get(ctx->file[candidate_idx]);
    uint32_t held_depth = fetch_depth_get(ctx->file[held_idx]);

    if (candidate_depth != held_depth) { return candidate_depth < held_depth ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    return strcmp(ctx->file[candidate_idx], ctx->file[held_idx]) < 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t fetch_def_add(crosstalk_core_fetch_ctx_t *ctx, const char *text, uint64_t len, uint32_t file_idx)
{
    crosstalk_core_fetch_def_t *def = 0x0;
    uint32_t hash = 0;
    uint32_t slot = 0;
    uint32_t ret = 0;

    if (len + LDG_STR_TERM_SIZE > (uint64_t)CROSSTALK_CORE_FETCH_IDENT_CAP) { return LDG_ERR_AOK; }

    hash = fetch_hash_get(text, len);

    ret = fetch_def_slot_find(ctx, text, len, hash, &slot);
    if (ret == LDG_ERR_AOK)
    {
        def = &ctx->def[ctx->def_slot[slot] - FETCH_SLOT_BIAS];

        if (def->file_idx == file_idx) { return LDG_ERR_AOK; }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fetch: defined twice; name: %.63s; one: %.127s; other: %.127s", def->name, ctx->file[def->file_idx], ctx->file[file_idx]);

        if (fetch_file_better_is(ctx, file_idx, def->file_idx) == LDG_TRUTH_TRUE) { def->file_idx = file_idx; }

        return LDG_ERR_AOK;
    }

    if (ret != LDG_ERR_NOT_FOUND) { return ret; }

    if (ctx->def_cunt >= (uint32_t)CROSSTALK_CORE_FETCH_DEF_MAX) { return LDG_ERR_FULL; }

    def = &ctx->def[ctx->def_cunt];
    memcpy(def->name, text, len);
    def->name[len] = LDG_STR_TERM;
    def->hash = hash;
    def->file_idx = file_idx;

    ctx->def_cunt = ctx->def_cunt + FETCH_STEP;
    ctx->def_slot[slot] = ctx->def_cunt;

    return LDG_ERR_AOK;
}

static uint32_t fetch_file_ld(crosstalk_core_fetch_ctx_t *ctx, uint32_t idx, uint64_t *out_len)
{
    char path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    ldg_io_info_t st = LDG_STRUCT_ZERO_INIT;
    ldg_io_file_t *fh = 0x0;
    uint64_t got = 0;
    uint32_t ret = 0;

    *out_len = 0;

    ret = ldg_io_path_join(ctx->repo_dir, ctx->file[idx], path, (uint64_t)sizeof(path));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_path_info_get(path, &st);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (st.is_file != LDG_TRUTH_TRUE) { return LDG_ERR_IO_IS_DIR; }

    if (st.size >= (uint64_t)CROSSTALK_CORE_FETCH_SCAN_CAP) { return LDG_ERR_FULL; }

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, FETCH_FILE_ACCESS_NONE, &fh);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_rd(fh, ctx->scan, st.size, &got);

    if (ldg_io_file_close(fh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fetch: file close failed; file: %.127s", ctx->file[idx]); }

    if (ret != LDG_ERR_AOK) { return ret; }

    if (got != st.size) { return LDG_ERR_IO_RD; }

    *out_len = got;

    return LDG_ERR_AOK;
}

static uint32_t fetch_defs_scan(crosstalk_core_fetch_ctx_t *ctx, uint32_t idx, uint64_t len)
{
    uint64_t pos = 0;
    uint64_t start = 0;
    uint64_t tok_len = 0;
    uint8_t want = LDG_TRUTH_FALSE;
    uint8_t pkg = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    while (fetch_tok_next(ctx->scan, len, &pos, &start, &tok_len) == LDG_ERR_AOK)
    {
        if (want == LDG_TRUTH_TRUE)
        {
            if (fetch_tok_eq_is(ctx->scan, start, tok_len, FETCH_KW_AUTOMATIC) == LDG_TRUTH_TRUE || fetch_tok_eq_is(ctx->scan, start, tok_len, FETCH_KW_STATIC) == LDG_TRUTH_TRUE) { continue; }

            want = LDG_TRUTH_FALSE;

            ret = fetch_def_add(ctx, &ctx->scan[start], tok_len, idx);
            if (ret != LDG_ERR_AOK) { return ret; }

            if (pkg == LDG_TRUTH_TRUE) { ctx->file_pkg[idx] = LDG_TRUTH_TRUE; }

            continue;
        }

        if (fetch_tok_eq_is(ctx->scan, start, tok_len, FETCH_KW_PACKAGE) == LDG_TRUTH_TRUE)
        {
            want = LDG_TRUTH_TRUE;
            pkg = LDG_TRUTH_TRUE;
            continue;
        }

        if (fetch_tok_eq_is(ctx->scan, start, tok_len, FETCH_KW_MODULE) == LDG_TRUTH_TRUE || fetch_tok_eq_is(ctx->scan, start, tok_len, FETCH_KW_MACROMODULE) == LDG_TRUTH_TRUE || fetch_tok_eq_is(ctx->scan, start, tok_len, FETCH_KW_INTERFACE) == LDG_TRUTH_TRUE || fetch_tok_eq_is(ctx->scan, start, tok_len, FETCH_KW_PROGRAM) == LDG_TRUTH_TRUE)
        {
            want = LDG_TRUTH_TRUE;
            pkg = LDG_TRUTH_FALSE;
        }
    }

    return LDG_ERR_AOK;
}

static uint32_t fetch_refs_scan(crosstalk_core_fetch_ctx_t *ctx, uint64_t len, uint32_t *io_tail)
{
    uint64_t pos = 0;
    uint64_t start = 0;
    uint64_t tok_len = 0;
    uint32_t slot = 0;
    uint32_t file_idx = 0;

    while (fetch_tok_next(ctx->scan, len, &pos, &start, &tok_len) == LDG_ERR_AOK)
    {
        if (tok_len + LDG_STR_TERM_SIZE > (uint64_t)CROSSTALK_CORE_FETCH_IDENT_CAP) { continue; }

        if (fetch_def_slot_find(ctx, &ctx->scan[start], tok_len, fetch_hash_get(&ctx->scan[start], tok_len), &slot) != LDG_ERR_AOK) { continue; }

        file_idx = ctx->def[ctx->def_slot[slot] - FETCH_SLOT_BIAS].file_idx;

        if (ctx->file_kept[file_idx] == LDG_TRUTH_TRUE) { continue; }

        if (*io_tail >= (uint32_t)CROSSTALK_CORE_FETCH_FILE_MAX) { return LDG_ERR_FULL; }

        ctx->file_kept[file_idx] = LDG_TRUTH_TRUE;
        ctx->file_order[*io_tail] = file_idx;
        *io_tail = *io_tail + FETCH_STEP;
    }

    return LDG_ERR_AOK;
}

static uint32_t fetch_list_emit(crosstalk_core_fetch_ctx_t *ctx, char *out, uint64_t cap, uint64_t *io_used, uint32_t kept_cunt, uint8_t pkg, uint32_t *io_cunt)
{
    uint32_t at = 0;
    uint32_t ret = 0;

    for (at = 0; at < kept_cunt; at = at + FETCH_STEP)
    {
        uint32_t file_idx = ctx->file_order[at];

        if ((ctx->file_pkg[file_idx] == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE) != pkg) { continue; }

        ret = fetch_list_append(out, cap, io_used, ctx->file[file_idx]);
        if (ret != LDG_ERR_AOK) { return ret; }

        *io_cunt = *io_cunt + FETCH_STEP;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_fetch_files_get(crosstalk_core_fetch_ctx_t *ctx, char *out, uint64_t cap, uint32_t *out_cunt)
{
    uint64_t len = 0;
    uint64_t used = 0;
    uint32_t idx = 0;
    uint32_t hd = 0;
    uint32_t tail = 0;
    uint32_t slot = 0;
    uint32_t cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0 || out_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (cap == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;

    ret = fetch_tree_walk(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (ctx->file_cunt == 0) { return LDG_ERR_EMPTY; }

    ctx->def_cunt = 0;
    memset(ctx->def_slot, 0x0, sizeof(ctx->def_slot));
    memset(ctx->file_kept, 0x0, sizeof(ctx->file_kept));
    memset(ctx->file_pkg, 0x0, sizeof(ctx->file_pkg));

    for (idx = 0; idx < ctx->file_cunt; idx = idx + FETCH_STEP)
    {
        ret = fetch_file_ld(ctx, idx, &len);
        if (ret != LDG_ERR_AOK)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fetch: file unreadable; file: %.127s; err: %u32", ctx->file[idx], ret);
            continue;
        }

        ret = fetch_defs_scan(ctx, idx, len);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    ret = fetch_def_slot_find(ctx, ctx->top, fetch_len_get(ctx->top), fetch_hash_get(ctx->top, fetch_len_get(ctx->top)), &slot);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fetch: top not defined; top: %.63s; files: %u32; defs: %u32", ctx->top, ctx->file_cunt, ctx->def_cunt);
        return LDG_ERR_NOT_FOUND;
    }

    idx = ctx->def[ctx->def_slot[slot] - FETCH_SLOT_BIAS].file_idx;
    ctx->file_kept[idx] = LDG_TRUTH_TRUE;
    ctx->file_order[0] = idx;
    tail = FETCH_STEP;

    while (hd < tail)
    {
        idx = ctx->file_order[hd];
        hd = hd + FETCH_STEP;

        if (fetch_file_ld(ctx, idx, &len) != LDG_ERR_AOK) { continue; }

        ret = fetch_refs_scan(ctx, len, &tail);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    ret = fetch_list_emit(ctx, out, cap, &used, tail, LDG_TRUTH_TRUE, &cunt);
    if (ret == LDG_ERR_AOK) { ret = fetch_list_emit(ctx, out, cap, &used, tail, LDG_TRUTH_FALSE, &cunt); }

    *out_cunt = cunt;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fetch: closure; top: %.63s; files: %u32; kept: %u32; defs: %u32; err: %u32", ctx->top, ctx->file_cunt, cunt, ctx->def_cunt, ret);

    return ret;
}

uint32_t crosstalk_core_fetch_stop(crosstalk_core_fetch_ctx_t *ctx)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_proc_stop(&ctx->proc);

    ctx->phase = CROSSTALK_CORE_FETCH_PHASE_NONE;

    return ret;
}
