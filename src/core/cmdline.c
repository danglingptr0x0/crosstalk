#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>
#include <dangling/logging/logging.h>

#include <dangling/term/render.h>
#include <dangling/term/vt.h>

#include "core/log.h"
#include "core/cmdline.h"
#include "ui/cell.h"
#include "ui/theme.h"

#define CROSSTALK_CORE_CMDLINE_HINT_GAP 1
#define CROSSTALK_CORE_CMDLINE_MATCH_UNIQUE 1
#define CROSSTALK_CORE_CMDLINE_ISEARCH_HINT_FMT "bck-i-search: %.170s"
#define CROSSTALK_CORE_CMDLINE_ISEARCH_NONE_TEXT "bck-i-search: no match"
#define CROSSTALK_CORE_CMDLINE_CURSOR_W 1
#define CROSSTALK_CORE_CMDLINE_TEXT_ORIG 0
#define CROSSTALK_CORE_CMDLINE_DRAWER_SCROLL_NONE 0
#define CROSSTALK_CORE_CMDLINE_ISEARCH_CODEPOINT 'r'
#define CROSSTALK_CORE_CMDLINE_ISEARCH_CODEPOINT_UPPER 'R'
#define CROSSTALK_CORE_CMDLINE_ISEARCH_CTRL_CODE 0x12
#define CROSSTALK_CORE_CMDLINE_WORD_SEP ' '
#define CROSSTALK_CORE_CMDLINE_LIST_SEP ','
#define CROSSTALK_CORE_CMDLINE_PATH_SEP '/'
#define CROSSTALK_CORE_CMDLINE_STMT_END ';'
#define CROSSTALK_CORE_CMDLINE_VERB_ARG_IDX 0
#define CROSSTALK_CORE_CMDLINE_CANDIDATE_GAP 2
#define CROSSTALK_CORE_CMDLINE_CANDIDATE_FIRST 0
#define CROSSTALK_CORE_CMDLINE_CANDIDATE_SEL_FMT "%u of %u; tab cycles, shift+tab back"
#define CROSSTALK_CORE_CMDLINE_CANDIDATE_LIST_FMT "%u candidates; tab cycles"
#define CROSSTALK_CORE_CMDLINE_WORD_FWD_CODEPOINT 'l'
#define CROSSTALK_CORE_CMDLINE_WORD_FWD_CODEPOINT_UPPER 'L'
#define CROSSTALK_CORE_CMDLINE_WORD_FWD_CTRL_CODE 0x0C
#define CROSSTALK_CORE_CMDLINE_WORD_DROP_CODEPOINT 'w'
#define CROSSTALK_CORE_CMDLINE_WORD_DROP_CODEPOINT_UPPER 'W'
#define CROSSTALK_CORE_CMDLINE_WORD_DROP_CTRL_CODE 0x17
#define CROSSTALK_CORE_CMDLINE_CLEAR_CODEPOINT 'c'
#define CROSSTALK_CORE_CMDLINE_CLEAR_CODEPOINT_UPPER 'C'
#define CROSSTALK_CORE_CMDLINE_CLEAR_CTRL_CODE 0x03
#define CROSSTALK_CORE_CMDLINE_CURSOR_END UINT32_MAX
#define CROSSTALK_CORE_CMDLINE_CURSOR_STEP 1

static uint32_t crosstalk_core_cmdline_len_get(const char *text)
{
    uint32_t len = 0;

    if (LDG_UNLIKELY(text == 0x0)) { return len; }

    while (len < CROSSTALK_CORE_CMDLINE_CAP && text[len] != LDG_STR_TERM) { len++; }

    return len;
}

static void crosstalk_core_cmdline_trim(char *text)
{
    uint32_t len = 0;

    if (LDG_UNLIKELY(text == 0x0)) { return; }

    len = crosstalk_core_cmdline_len_get(text);
    while (len > 0)
    {
        len--;
        if (text[len] != '\n' && text[len] != '\r' && text[len] != ' ') { break; }

        text[len] = LDG_STR_TERM;
    }
}

static uint32_t crosstalk_core_cmdline_verb_len_get(const char *text)
{
    uint32_t len = 0;

    if (LDG_UNLIKELY(text == 0x0)) { return len; }

    while (len < CROSSTALK_CORE_CMDLINE_CAP && text[len] != LDG_STR_TERM && text[len] != ' ' && text[len] != ';') { len++; }

    return len;
}

uint32_t crosstalk_core_cmdline_esc(crosstalk_core_cmdline_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (ctx->candidate_on == LDG_TRUTH_TRUE)
    {
        ctx->candidate_on = LDG_TRUTH_FALSE;
        ctx->candidate_sel = (uint32_t)UINT32_MAX;

        return LDG_ERR_AOK;
    }

    if (ctx->isearch_on == LDG_TRUTH_TRUE)
    {
        ctx->isearch_on = LDG_TRUTH_FALSE;

        return LDG_ERR_AOK;
    }

    return crosstalk_core_cmdline_close(ctx);
}

uint32_t crosstalk_core_cmdline_verb_reg(crosstalk_core_cmdline_ctx_t *ctx, const char *name, const char *help, uint32_t arg_max)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0 || help == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->verbs_cunt >= (uint32_t)CROSSTALK_CORE_CMDLINE_VERB_CUNT) { return LDG_ERR_FULL; }

    ret = ldg_str_rbrcpy(ctx->verb_name[ctx->verbs_cunt], name, (uint64_t)CROSSTALK_CORE_CMDLINE_VERB_NAME_CAP);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    ret = ldg_str_rbrcpy(ctx->verb_help[ctx->verbs_cunt], help, (uint64_t)CROSSTALK_CORE_CMDLINE_VERB_HELP_CAP);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    ctx->verb_arg_max[ctx->verbs_cunt] = arg_max;
    ctx->verbs_cunt++;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_cmdline_verb_cunt_get(const crosstalk_core_cmdline_ctx_t *ctx, uint32_t *out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = ctx->verbs_cunt;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_cmdline_verb_get(const crosstalk_core_cmdline_ctx_t *ctx, uint32_t idx, const char **out_name, const char **out_help)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_name == 0x0 || out_help == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_name = 0x0;
    *out_help = 0x0;

    if (idx >= ctx->verbs_cunt) { return LDG_ERR_NOT_FOUND; }

    *out_name = ctx->verb_name[idx];
    *out_help = ctx->verb_help[idx];

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_cmdline_candidate_cb_set(crosstalk_core_cmdline_ctx_t *ctx, crosstalk_core_cmdline_candidate_cb_t cb, void *user)
{
    if (LDG_UNLIKELY(ctx == 0x0 || cb == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->candidate_cb = cb;
    ctx->candidate_user = user;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_cmdline_hist_get(const crosstalk_core_cmdline_ctx_t *ctx, uint32_t idx, const char **out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (idx >= ctx->hist_cunt) { return LDG_ERR_NOT_FOUND; }

    *out = ctx->hist[(ctx->hist_hd + idx) % (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_MAX];

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_cmdline_drawer_init(crosstalk_core_cmdline_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mut_init(&ctx->drawer_mut, LDG_TRUTH_FALSE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->drawer_hd = 0;
    ctx->drawer_cunt = 0;
    ctx->drawer_ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_cmdline_drawer_shutdown(crosstalk_core_cmdline_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->drawer_ok != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ctx->drawer_ok = LDG_TRUTH_FALSE;

    return ldg_mut_destroy(&ctx->drawer_mut);
}

uint32_t crosstalk_core_cmdline_out_push(crosstalk_core_cmdline_ctx_t *ctx, const char *text)
{
    uint32_t slot = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->drawer_ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (text[0] == LDG_STR_TERM) { return LDG_ERR_AOK; }

    ret = ldg_mut_lock(&ctx->drawer_mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    slot = (ctx->drawer_hd + ctx->drawer_cunt) % (uint32_t)CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CUNT;

    ret = ldg_str_rbrcpy(ctx->drawer[slot], text, (uint64_t)CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CAP);
    if (ret == LDG_ERR_AOK || ret == LDG_ERR_STR_TRUNC)
    {
        if (ctx->drawer_cunt < (uint32_t)CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CUNT) { ctx->drawer_cunt = ctx->drawer_cunt + (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP; }
        else { ctx->drawer_hd = (ctx->drawer_hd + (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP) % (uint32_t)CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CUNT; }

        ret = LDG_ERR_AOK;
    }

    if (ldg_mut_unlock(&ctx->drawer_mut) != LDG_ERR_AOK) { return LDG_ERR_INVALID; }

    return ret;
}

void crosstalk_core_cmdline_log_cb(void *cls, const char *app_name, const time_t date, const CROSSTALK_CORE_CMDLINE_LOG_LVL_T level, const char *msg)
{
    crosstalk_core_cmdline_ctx_t *ctx = (crosstalk_core_cmdline_ctx_t *)cls;

    if (ctx == 0x0 || msg == 0x0) { return; }

    if (app_name == 0x0 && date == (time_t)0x0) { return; }

    if (level == LDG_LOGGING_LVL_DEBUG) { return; }

    if (crosstalk_core_cmdline_out_push(ctx, msg) != LDG_ERR_AOK) { return; }
}

static uint8_t isearch_match_is(const char *entry, const char *needle)
{
    if (LDG_UNLIKELY(entry == 0x0 || needle == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return strstr(entry, needle) != 0x0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t isearch_seek(crosstalk_core_cmdline_ctx_t *ctx, uint32_t from)
{
    const char *entry = 0x0;
    uint32_t walked = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->isearch_sel = (uint32_t)UINT32_MAX;

    if (ctx->hist_cunt == 0) { return LDG_ERR_AOK; }

    if (from >= ctx->hist_cunt) { from = ctx->hist_cunt - (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP; }

    for (walked = 0; walked < ctx->hist_cunt; walked = walked + (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP)
    {
        idx = (from + ctx->hist_cunt - walked) % ctx->hist_cunt;

        if (crosstalk_core_cmdline_hist_get(ctx, idx, &entry) != LDG_ERR_AOK) { continue; }

        if (isearch_match_is(entry, ctx->isearch) == LDG_TRUTH_TRUE)
        {
            ctx->isearch_sel = idx;

            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_AOK;
}

static uint32_t isearch_seek_fwd(crosstalk_core_cmdline_ctx_t *ctx, uint32_t from)
{
    const char *entry = 0x0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = from; idx < ctx->hist_cunt; idx = idx + (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP)
    {
        if (crosstalk_core_cmdline_hist_get(ctx, idx, &entry) != LDG_ERR_AOK) { continue; }

        if (isearch_match_is(entry, ctx->isearch) == LDG_TRUTH_TRUE)
        {
            ctx->isearch_sel = idx;

            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_AOK;
}

static uint32_t hist_slot_put(crosstalk_core_cmdline_ctx_t *ctx, const char *text)
{
    const char *last = 0x0;
    uint32_t slot = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (text[0] == LDG_STR_TERM) { return LDG_ERR_AOK; }

    if (ctx->hist_cunt != 0 && crosstalk_core_cmdline_hist_get(ctx, ctx->hist_cunt - (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP, &last) == LDG_ERR_AOK) { if (strncmp(last, text, (uint64_t)CROSSTALK_CORE_CMDLINE_CAP) == 0) { return LDG_ERR_AOK; } }

    slot = (ctx->hist_hd + ctx->hist_cunt) % (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_MAX;

    ret = ldg_str_rbrcpy(ctx->hist[slot], text, (uint64_t)CROSSTALK_CORE_CMDLINE_CAP);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    if (ctx->hist_cunt < (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_MAX) { ctx->hist_cunt = ctx->hist_cunt + (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP; }
    else { ctx->hist_hd = (ctx->hist_hd + (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP) % (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_MAX; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_cmdline_hist_st(crosstalk_core_cmdline_ctx_t *ctx)
{
    ldg_io_file_t *fh = 0x0;
    uint64_t wrote = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->hist_path[0] == LDG_STR_TERM) { return LDG_ERR_NOT_INIT; }

    ret = ldg_io_file_open(ctx->hist_path, LDG_IO_WR_ONLY | LDG_IO_CREATE | LDG_IO_TRUNCATE, (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_MODE, &fh);
    if (ret != LDG_ERR_AOK) { return ret; }

    for (idx = 0; idx < ctx->hist_cunt; idx = idx + (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP)
    {
        const char *entry = 0x0;

        if (crosstalk_core_cmdline_hist_get(ctx, idx, &entry) != LDG_ERR_AOK) { continue; }

        ret = ldg_io_file_wr(fh, entry, (uint64_t)crosstalk_core_cmdline_len_get(entry), &wrote);
        if (ret != LDG_ERR_AOK) { break; }

        ret = ldg_io_file_wr(fh, CROSSTALK_CORE_CMDLINE_HIST_EOL, (uint64_t)LDG_STR_TERM_SIZE, &wrote);
        if (ret != LDG_ERR_AOK) { break; }
    }

    if (ldg_io_file_close(fh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cmdline hist close failed"); }

    return ret;
}

uint32_t crosstalk_core_cmdline_hist_ld(crosstalk_core_cmdline_ctx_t *ctx, const char *path)
{
    ldg_byte_t chunk[CROSSTALK_CORE_CMDLINE_HIST_CHUNK];
    char line[CROSSTALK_CORE_CMDLINE_CAP] = LDG_ARR_ZERO_INIT;
    ldg_io_file_t *fh = 0x0;
    uint64_t got = 0;
    uint32_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_str_rbrcpy(ctx->hist_path, path, (uint64_t)sizeof(ctx->hist_path));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    if (ldg_io_path_exists_is(path) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, 0x0, &fh);
    if (ret != LDG_ERR_AOK) { return ret; }

    while (ldg_io_file_rd(fh, chunk, (uint64_t)CROSSTALK_CORE_CMDLINE_HIST_CHUNK, &got) == LDG_ERR_AOK && got != 0)
    {
        uint64_t at = 0;

        for (at = 0; at < got; at++)
        {
            if (chunk[at] == (ldg_byte_t)'\n' || chunk[at] == (ldg_byte_t)'\r')
            {
                line[len] = LDG_STR_TERM;

                if (len != 0 && hist_slot_put(ctx, line) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cmdline hist entry rejected"); }

                len = 0;
                continue;
            }

            if (len < (uint32_t)CROSSTALK_CORE_CMDLINE_CAP - LDG_STR_TERM_SIZE)
            {
                line[len] = (char)chunk[at];
                len = len + (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP;
            }
        }
    }

    if (len != 0)
    {
        line[len] = LDG_STR_TERM;
        if (hist_slot_put(ctx, line) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cmdline hist entry rejected"); }
    }

    if (ldg_io_file_close(fh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cmdline hist close failed"); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "cmdline: hist loaded; entries: %u32; path: %.255s", ctx->hist_cunt, path);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_cmdline_hist_push(crosstalk_core_cmdline_ctx_t *ctx, const char *text)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = hist_slot_put(ctx, text);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (ctx->hist_path[0] == LDG_STR_TERM) { return LDG_ERR_AOK; }

    ret = crosstalk_core_cmdline_hist_st(ctx);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cmdline: hist store failed; err: %u32", ret); }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_cmdline_open(crosstalk_core_cmdline_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->buff[0] = LDG_STR_TERM;
    ctx->ghost[0] = LDG_STR_TERM;
    ctx->hint[0] = LDG_STR_TERM;
    ctx->draft[0] = LDG_STR_TERM;
    ctx->isearch[0] = LDG_STR_TERM;
    ctx->hist_sel = (uint32_t)UINT32_MAX;
    ctx->isearch_sel = (uint32_t)UINT32_MAX;
    ctx->candidate_sel = (uint32_t)UINT32_MAX;
    ctx->candidate_cunt = 0;
    ctx->candidate_on = LDG_TRUTH_FALSE;
    ctx->isearch_on = LDG_TRUTH_FALSE;
    ctx->drawer_scroll = (uint32_t)CROSSTALK_CORE_CMDLINE_DRAWER_SCROLL_NONE;
    ctx->cursor = (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_END;
    ctx->open = LDG_TRUTH_TRUE;
    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_cmdline_close(crosstalk_core_cmdline_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->buff[0] = LDG_STR_TERM;
    ctx->ghost[0] = LDG_STR_TERM;
    ctx->hint[0] = LDG_STR_TERM;
    ctx->draft[0] = LDG_STR_TERM;
    ctx->isearch[0] = LDG_STR_TERM;
    ctx->hist_sel = (uint32_t)UINT32_MAX;
    ctx->isearch_sel = (uint32_t)UINT32_MAX;
    ctx->candidate_sel = (uint32_t)UINT32_MAX;
    ctx->candidate_cunt = 0;
    ctx->candidate_on = LDG_TRUTH_FALSE;
    ctx->isearch_on = LDG_TRUTH_FALSE;
    ctx->drawer_scroll = (uint32_t)CROSSTALK_CORE_CMDLINE_DRAWER_SCROLL_NONE;
    ctx->cursor = (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_END;
    ctx->open = LDG_TRUTH_FALSE;
    return LDG_ERR_AOK;
}

uint8_t crosstalk_core_cmdline_open_is(const crosstalk_core_cmdline_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->open;
}

static uint32_t cmdline_text_put(ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t width, const char *text, uint64_t cap, uint32_t fg_rgba)
{
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t bg = LDG_STRUCT_ZERO_INIT;
    uint64_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_str_len_get(text, cap, &len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (len > (uint64_t)width) { len = (uint64_t)width; }

    if (len == 0) { return LDG_ERR_AOK; }

    ret = crosstalk_ui_cell_color_get(fg_rgba, &fg);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_CELL_PANE_RGBA, &bg); }

    if (ret != LDG_ERR_AOK) { return ret; }

    return ldg_term_render_text_draw(render, col, row, text, len, &fg, &bg);
}

static uint32_t cmdline_row_clear(ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t width)
{
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t bg = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, &fg);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_CELL_PANE_RGBA, &bg); }

    if (ret != LDG_ERR_AOK) { return ret; }

    return crosstalk_ui_cell_span_fill(render, col, row, width, &fg, &bg);
}

static LDG_NOINLINE uint32_t drawer_draw(crosstalk_core_cmdline_ctx_t *ctx, ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t width)
{
    char lines[CROSSTALK_CORE_CMDLINE_DRAWER_SHOW_CUNT][CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CAP];
    uint32_t top_row = 0;
    uint32_t max_scroll = 0;
    uint32_t total = 0;
    uint32_t start = 0;
    uint32_t shown = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->drawer_ok != LDG_TRUTH_TRUE || ctx->drawer_cunt == 0) { return LDG_ERR_AOK; }

    ret = ldg_mut_lock(&ctx->drawer_mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    total = ctx->drawer_cunt;
    max_scroll = total > (uint32_t)CROSSTALK_CORE_CMDLINE_DRAWER_SHOW_CUNT ? total - (uint32_t)CROSSTALK_CORE_CMDLINE_DRAWER_SHOW_CUNT : (uint32_t)CROSSTALK_CORE_CMDLINE_DRAWER_SCROLL_NONE;

    if (ctx->drawer_scroll > max_scroll) { ctx->drawer_scroll = max_scroll; }

    start = total > (uint32_t)CROSSTALK_CORE_CMDLINE_DRAWER_SHOW_CUNT ? total - (uint32_t)CROSSTALK_CORE_CMDLINE_DRAWER_SHOW_CUNT - ctx->drawer_scroll : (uint32_t)CROSSTALK_CORE_CMDLINE_DRAWER_SCROLL_NONE;
    shown = total - start > (uint32_t)CROSSTALK_CORE_CMDLINE_DRAWER_SHOW_CUNT ? (uint32_t)CROSSTALK_CORE_CMDLINE_DRAWER_SHOW_CUNT : total - start;

    for (idx = 0; idx < shown; idx = idx + (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP)
    {
        uint32_t slot = (ctx->drawer_hd + start + idx) % (uint32_t)CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CUNT;

        ret = ldg_str_rbrcpy(lines[idx], ctx->drawer[slot], (uint64_t)CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CAP);
        if (ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC) { lines[idx][0] = LDG_STR_TERM; }
    }

    if (ldg_mut_unlock(&ctx->drawer_mut) != LDG_ERR_AOK) { return LDG_ERR_INVALID; }

    if (shown > row) { shown = row; }

    top_row = row - shown;

    for (idx = 0; idx < shown; idx = idx + (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP)
    {
        ret = cmdline_row_clear(render, col, top_row + idx, width);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = cmdline_text_put(render, col, top_row + idx, width, lines[idx], (uint64_t)CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CAP, (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t candidate_grid_draw(crosstalk_core_cmdline_ctx_t *ctx, ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t width)
{
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t bg = LDG_STRUCT_ZERO_INIT;
    uint32_t cell_w = 0;
    uint32_t col_cunt = 0;
    uint32_t row_cunt = 0;
    uint32_t shown = 0;
    uint32_t first_row = 0;
    uint32_t top_row = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->candidate_cunt == 0 || row == 0) { return LDG_ERR_AOK; }

    for (idx = 0; idx < ctx->candidate_cunt; idx++) { if (crosstalk_core_cmdline_len_get(ctx->candidate[idx]) > cell_w) { cell_w = crosstalk_core_cmdline_len_get(ctx->candidate[idx]); } }

    cell_w = cell_w + (uint32_t)CROSSTALK_CORE_CMDLINE_CANDIDATE_GAP;
    col_cunt = width / cell_w;
    if (col_cunt == 0) { col_cunt = (uint32_t)CROSSTALK_CORE_CMDLINE_MATCH_UNIQUE; }

    row_cunt = (ctx->candidate_cunt + col_cunt - (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP) / col_cunt;
    shown = row_cunt > (uint32_t)CROSSTALK_CORE_CMDLINE_DRAWER_SHOW_CUNT ? (uint32_t)CROSSTALK_CORE_CMDLINE_DRAWER_SHOW_CUNT : row_cunt;
    if (shown > row) { shown = row; }

    if (ctx->candidate_sel != (uint32_t)UINT32_MAX && ctx->candidate_sel / col_cunt >= shown) { first_row = ctx->candidate_sel / col_cunt - shown + (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP; }

    top_row = row - shown;

    for (idx = 0; idx < shown; idx++)
    {
        ret = cmdline_row_clear(render, col, top_row + idx, width);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (idx = first_row * col_cunt; idx < ctx->candidate_cunt && idx < (first_row + shown) * col_cunt; idx++)
    {
        uint32_t at_col = col + (idx % col_cunt) * cell_w;
        uint32_t at_row = top_row + idx / col_cunt - first_row;

        if (idx != ctx->candidate_sel)
        {
            ret = cmdline_text_put(render, at_col, at_row, width - (idx % col_cunt) * cell_w, ctx->candidate[idx], (uint64_t)CROSSTALK_CORE_CMDLINE_CANDIDATE_CAP, (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            continue;
        }

        ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, &bg);
        if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_CELL_PANE_RGBA, &fg); }

        if (ret == LDG_ERR_AOK) { ret = ldg_term_render_text_draw(render, at_col, at_row, ctx->candidate[idx], (uint64_t)crosstalk_core_cmdline_len_get(ctx->candidate[idx]), &fg, &bg); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_cmdline_drawer_overlay_draw(crosstalk_core_cmdline_ctx_t *ctx, ldg_term_render_t *render)
{
    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (ctx->drawer_col_cunt == 0) { return LDG_ERR_AOK; }

    if (ctx->candidate_on == LDG_TRUTH_TRUE) { return candidate_grid_draw(ctx, render, ctx->drawer_col, ctx->drawer_row, ctx->drawer_col_cunt); }

    return drawer_draw(ctx, render, ctx->drawer_col, ctx->drawer_row, ctx->drawer_col_cunt);
}

static uint32_t verb_match_get(const crosstalk_core_cmdline_ctx_t *ctx, uint32_t *out_verb_len, uint32_t *out_cunt, const char **out_name, const char **out_help)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_verb_len == 0x0 || out_cunt == 0x0 || out_name == 0x0 || out_help == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;
    *out_name = 0x0;
    *out_help = 0x0;
    *out_verb_len = crosstalk_core_cmdline_verb_len_get(ctx->buff);

    for (idx = 0; idx < ctx->verbs_cunt; idx++)
    {
        if (*out_verb_len != 0 && strncmp(ctx->verb_name[idx], ctx->buff, (uint64_t)*out_verb_len) != 0) { continue; }

        *out_cunt = *out_cunt + (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP;

        if (*out_name == 0x0)
        {
            *out_name = ctx->verb_name[idx];
            *out_help = ctx->verb_help[idx];
        }
    }

    return LDG_ERR_AOK;
}

static uint32_t isearch_resync(crosstalk_core_cmdline_ctx_t *ctx)
{
    const char *held = 0x0;
    uint32_t from = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->isearch_on != LDG_TRUTH_TRUE || ctx->hist_cunt == 0) { return LDG_ERR_AOK; }

    if (ctx->isearch_sel != (uint32_t)UINT32_MAX && crosstalk_core_cmdline_hist_get(ctx, ctx->isearch_sel, &held) == LDG_ERR_AOK && isearch_match_is(held, ctx->isearch) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    from = ctx->isearch_sel == (uint32_t)UINT32_MAX ? ctx->hist_cunt - (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP : ctx->isearch_sel;

    return isearch_seek(ctx, from);
}

static uint32_t isearch_step(crosstalk_core_cmdline_ctx_t *ctx, uint8_t fwd)
{
    uint32_t from = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->isearch_on != LDG_TRUTH_TRUE)
    {
        ctx->isearch[0] = LDG_STR_TERM;
        ctx->isearch_sel = (uint32_t)UINT32_MAX;
        ctx->isearch_on = LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    if (ctx->hist_cunt == 0) { return LDG_ERR_AOK; }

    if (fwd == LDG_TRUTH_TRUE)
    {
        if (ctx->isearch_sel == (uint32_t)UINT32_MAX) { return LDG_ERR_AOK; }

        return isearch_seek_fwd(ctx, ctx->isearch_sel + (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP);
    }

    from = ctx->isearch_sel == (uint32_t)UINT32_MAX ? ctx->hist_cunt - (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP : (ctx->isearch_sel + ctx->hist_cunt - (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP) % ctx->hist_cunt;

    return isearch_seek(ctx, from);
}

static uint32_t text_ch_append(char *dst, uint32_t codepoint)
{
    uint32_t len = 0;

    if (LDG_UNLIKELY(dst == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (codepoint < (uint32_t)LDG_ASCII_PRINTABLE_MIN || codepoint > (uint32_t)LDG_ASCII_PRINTABLE_MAX) { return LDG_ERR_AOK; }

    len = crosstalk_core_cmdline_len_get(dst);
    if (len + (uint32_t)LDG_STR_TERM_SIZE >= (uint32_t)CROSSTALK_CORE_CMDLINE_CAP) { return LDG_ERR_FULL; }

    dst[len] = (char)codepoint;
    dst[len + (uint32_t)LDG_STR_TERM_SIZE] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

static uint32_t text_ch_drop(char *dst)
{
    uint32_t len = 0;

    if (LDG_UNLIKELY(dst == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    len = crosstalk_core_cmdline_len_get(dst);
    if (len == 0) { return LDG_ERR_AOK; }

    dst[len - (uint32_t)LDG_STR_TERM_SIZE] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

static uint32_t cursor_pos_get(const crosstalk_core_cmdline_ctx_t *ctx)
{
    uint32_t len = crosstalk_core_cmdline_len_get(ctx->buff);

    if (ctx->cursor == (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_END || ctx->cursor > len) { return len; }

    return ctx->cursor;
}

static uint32_t cursor_mov(crosstalk_core_cmdline_ctx_t *ctx, uint8_t back)
{
    uint32_t len = 0;
    uint32_t pos = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    len = crosstalk_core_cmdline_len_get(ctx->buff);
    pos = cursor_pos_get(ctx);

    if (back == LDG_TRUTH_TRUE)
    {
        if (pos == 0) { return LDG_ERR_AOK; }

        ctx->cursor = pos - (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_STEP;
        ctx->ghost[0] = LDG_STR_TERM;

        return LDG_ERR_AOK;
    }

    if (pos >= len) { return LDG_ERR_AOK; }

    pos = pos + (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_STEP;
    ctx->cursor = pos >= len ? (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_END : pos;

    return LDG_ERR_AOK;
}

static uint32_t cursor_word_mov(crosstalk_core_cmdline_ctx_t *ctx, uint8_t back)
{
    uint32_t len = 0;
    uint32_t pos = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    len = crosstalk_core_cmdline_len_get(ctx->buff);
    pos = cursor_pos_get(ctx);

    if (back == LDG_TRUTH_TRUE)
    {
        while (pos > 0 && ctx->buff[pos - (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_STEP] == CROSSTALK_CORE_CMDLINE_WORD_SEP) { pos--; }

        while (pos > 0 && ctx->buff[pos - (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_STEP] != CROSSTALK_CORE_CMDLINE_WORD_SEP) { pos--; }

        if (pos >= len) { return LDG_ERR_AOK; }

        ctx->cursor = pos;
        ctx->ghost[0] = LDG_STR_TERM;

        return LDG_ERR_AOK;
    }

    while (pos < len && ctx->buff[pos] == CROSSTALK_CORE_CMDLINE_WORD_SEP) { pos++; }

    while (pos < len && ctx->buff[pos] != CROSSTALK_CORE_CMDLINE_WORD_SEP) { pos++; }

    ctx->cursor = pos >= len ? (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_END : pos;

    return LDG_ERR_AOK;
}

static uint32_t text_ch_insert(crosstalk_core_cmdline_ctx_t *ctx, uint32_t codepoint)
{
    uint32_t len = 0;
    uint32_t pos = 0;

    if (codepoint < (uint32_t)LDG_ASCII_PRINTABLE_MIN || codepoint > (uint32_t)LDG_ASCII_PRINTABLE_MAX) { return LDG_ERR_AOK; }

    len = crosstalk_core_cmdline_len_get(ctx->buff);
    if (len + (uint32_t)LDG_STR_TERM_SIZE >= (uint32_t)CROSSTALK_CORE_CMDLINE_CAP) { return LDG_ERR_FULL; }

    pos = cursor_pos_get(ctx);

    memmove(&ctx->buff[pos + (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_STEP], &ctx->buff[pos], (uint64_t)(len - pos + (uint32_t)LDG_STR_TERM_SIZE));
    ctx->buff[pos] = (char)codepoint;

    if (ctx->cursor != (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_END) { ctx->cursor = pos + (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_STEP; }

    return LDG_ERR_AOK;
}

static uint32_t text_irq_insert(crosstalk_core_cmdline_ctx_t *ctx, const ldg_term_key_event_t *irq)
{
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || irq == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if ((irq->mods & ((uint32_t)LDG_TERM_MOD_CTRL | (uint32_t)LDG_TERM_MOD_ALT)) != 0) { return LDG_ERR_AOK; }

    if (irq->text_cunt == 0)
    {
        if (irq->key != (uint8_t)LDG_TERM_KEY_NONE) { return LDG_ERR_AOK; }

        return text_ch_insert(ctx, irq->codepoint);
    }

    for (at = 0; at < irq->text_cunt && at < (uint32_t)LDG_TERM_KEY_TEXT_MAX; at = at + (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_STEP)
    {
        ret = text_ch_insert(ctx, irq->text[at]);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t text_span_drop(crosstalk_core_cmdline_ctx_t *ctx, uint32_t start, uint32_t pos)
{
    uint32_t len = crosstalk_core_cmdline_len_get(ctx->buff);

    if (start >= pos) { return LDG_ERR_AOK; }

    memmove(&ctx->buff[start], &ctx->buff[pos], (uint64_t)(len - pos + (uint32_t)LDG_STR_TERM_SIZE));

    if (ctx->cursor != (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_END) { ctx->cursor = start; }

    return LDG_ERR_AOK;
}

static uint32_t text_ch_back_drop(crosstalk_core_cmdline_ctx_t *ctx)
{
    uint32_t pos = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    pos = cursor_pos_get(ctx);
    if (pos == 0) { return LDG_ERR_AOK; }

    return text_span_drop(ctx, pos - (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_STEP, pos);
}

static uint32_t text_word_back_drop(crosstalk_core_cmdline_ctx_t *ctx)
{
    uint32_t pos = 0;
    uint32_t start = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    pos = cursor_pos_get(ctx);
    start = pos;

    while (start > 0 && ctx->buff[start - (uint32_t)LDG_STR_TERM_SIZE] == CROSSTALK_CORE_CMDLINE_WORD_SEP) { start--; }

    while (start > 0 && ctx->buff[start - (uint32_t)LDG_STR_TERM_SIZE] != CROSSTALK_CORE_CMDLINE_WORD_SEP) { start--; }

    return text_span_drop(ctx, start, pos);
}

static uint32_t text_irq_append(char *dst, const ldg_term_key_event_t *irq)
{
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(dst == 0x0 || irq == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if ((irq->mods & ((uint32_t)LDG_TERM_MOD_CTRL | (uint32_t)LDG_TERM_MOD_ALT)) != 0) { return LDG_ERR_AOK; }

    if (irq->text_cunt == 0)
    {
        if (irq->key != (uint8_t)LDG_TERM_KEY_NONE) { return LDG_ERR_AOK; }

        return text_ch_append(dst, irq->codepoint);
    }

    for (at = 0; at < irq->text_cunt && at < (uint32_t)LDG_TERM_KEY_TEXT_MAX; at = at + (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP)
    {
        ret = text_ch_append(dst, irq->text[at]);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t hist_walk(crosstalk_core_cmdline_ctx_t *ctx, uint8_t older)
{
    const char *entry = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->cursor = (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_END;

    if (older == LDG_TRUTH_TRUE)
    {
        if (ctx->hist_cunt == 0) { return LDG_ERR_AOK; }

        if (ctx->hist_sel == (uint32_t)UINT32_MAX)
        {
            ret = ldg_str_rbrcpy(ctx->draft, ctx->buff, (uint64_t)sizeof(ctx->draft));
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

            ctx->hist_sel = ctx->hist_cunt - (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP;
        }
        else if (ctx->hist_sel != 0) { ctx->hist_sel = ctx->hist_sel - (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP; }
    }
    else
    {
        if (ctx->hist_sel == (uint32_t)UINT32_MAX) { return LDG_ERR_AOK; }

        ctx->hist_sel = ctx->hist_sel + (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP;

        if (ctx->hist_sel >= ctx->hist_cunt)
        {
            ctx->hist_sel = (uint32_t)UINT32_MAX;

            ret = ldg_str_rbrcpy(ctx->buff, ctx->draft, (uint64_t)sizeof(ctx->buff));
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

            return LDG_ERR_AOK;
        }
    }

    if (crosstalk_core_cmdline_hist_get(ctx, ctx->hist_sel, &entry) != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    ret = ldg_str_rbrcpy(ctx->buff, entry, (uint64_t)sizeof(ctx->buff));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    return LDG_ERR_AOK;
}

static uint32_t line_submit(crosstalk_core_cmdline_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    crosstalk_core_cmdline_trim(ctx->buff);
    if (crosstalk_core_cmdline_len_get(ctx->buff) == 0) { return crosstalk_core_cmdline_close(ctx); }

    if (crosstalk_core_cmdline_out_push(ctx, ctx->buff) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cmdline echo push failed"); }

    if (crosstalk_core_cmdline_hist_push(ctx, ctx->buff) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cmdline history push failed"); }

    ret = ldg_str_rbrcpy(ctx->line, ctx->buff, (uint64_t)sizeof(ctx->line));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    ctx->line_rdy = LDG_TRUTH_TRUE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "cmdline: handed to the script; line: %.128s", ctx->line);

    return crosstalk_core_cmdline_close(ctx);
}

static uint32_t isearch_key_isr(crosstalk_core_cmdline_ctx_t *ctx, const ldg_term_key_event_t *irq)
{
    const char *match = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || irq == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (irq->key == (uint8_t)LDG_TERM_KEY_ENTER)
    {
        ctx->isearch_on = LDG_TRUTH_FALSE;

        if (ctx->isearch_sel == (uint32_t)UINT32_MAX || crosstalk_core_cmdline_hist_get(ctx, ctx->isearch_sel, &match) != LDG_ERR_AOK) { return LDG_ERR_AOK; }

        ret = ldg_str_rbrcpy(ctx->buff, match, (uint64_t)sizeof(ctx->buff));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

        return line_submit(ctx);
    }

    if (irq->key == (uint8_t)LDG_TERM_KEY_BACKSPACE) { ret = text_ch_drop(ctx->isearch); }
    else { ret = text_irq_append(ctx->isearch, irq); }

    if (ret != LDG_ERR_AOK && ret != LDG_ERR_FULL) { return ret; }

    return isearch_resync(ctx);
}

static uint32_t word_at_get(const char *text)
{
    uint32_t len = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(text == 0x0)) { return at; }

    len = crosstalk_core_cmdline_len_get(text);

    for (at = len; at > 0; at--) { if (text[at - (uint32_t)LDG_STR_TERM_SIZE] == CROSSTALK_CORE_CMDLINE_WORD_SEP || text[at - (uint32_t)LDG_STR_TERM_SIZE] == CROSSTALK_CORE_CMDLINE_LIST_SEP) { break; } }

    return at;
}

static uint32_t arg_idx_get(const char *text, uint32_t at)
{
    uint32_t tok_cunt = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(text == 0x0)) { return tok_cunt; }

    for (idx = (uint32_t)LDG_STR_TERM_SIZE; idx < at; idx++) { if (text[idx] == CROSSTALK_CORE_CMDLINE_WORD_SEP && text[idx - (uint32_t)LDG_STR_TERM_SIZE] != CROSSTALK_CORE_CMDLINE_WORD_SEP) { tok_cunt++; } }

    return tok_cunt > 0 ? tok_cunt - (uint32_t)LDG_STR_TERM_SIZE : (uint32_t)CROSSTALK_CORE_CMDLINE_VERB_ARG_IDX;
}

static uint8_t candidate_prefix_is(const char *text, const char *word, uint64_t word_len)
{
    if (LDG_UNLIKELY(text == 0x0 || word == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return strncmp(text, word, word_len) == 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t candidate_sift(crosstalk_core_cmdline_ctx_t *ctx, const char *word)
{
    char held[CROSSTALK_CORE_CMDLINE_CANDIDATE_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t word_len = 0;
    uint32_t kept = 0;
    uint32_t idx = 0;
    uint32_t prior = 0;
    uint8_t dup = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0 || word == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    word_len = (uint64_t)strlen(word);

    for (idx = 0; idx < ctx->candidate_cunt; idx++)
    {
        if (candidate_prefix_is(ctx->candidate[idx], word, word_len) != LDG_TRUTH_TRUE) { continue; }

        dup = LDG_TRUTH_FALSE;

        for (prior = 0; prior < kept; prior++) { if (strcmp(ctx->candidate[prior], ctx->candidate[idx]) == 0) { dup = LDG_TRUTH_TRUE; break; } }

        if (dup == LDG_TRUTH_TRUE) { continue; }

        if (kept != idx) { memcpy(ctx->candidate[kept], ctx->candidate[idx], (uint64_t)CROSSTALK_CORE_CMDLINE_CANDIDATE_CAP); }

        kept++;
    }

    ctx->candidate_cunt = kept;

    for (idx = (uint32_t)LDG_STR_TERM_SIZE; idx < kept; idx++)
    {
        memcpy(held, ctx->candidate[idx], (uint64_t)CROSSTALK_CORE_CMDLINE_CANDIDATE_CAP);

        for (prior = idx; prior > 0 && strcmp(ctx->candidate[prior - (uint32_t)LDG_STR_TERM_SIZE], held) > 0; prior--) { memcpy(ctx->candidate[prior], ctx->candidate[prior - (uint32_t)LDG_STR_TERM_SIZE], (uint64_t)CROSSTALK_CORE_CMDLINE_CANDIDATE_CAP); }

        memcpy(ctx->candidate[prior], held, (uint64_t)CROSSTALK_CORE_CMDLINE_CANDIDATE_CAP);
    }

    return LDG_ERR_AOK;
}

static uint32_t candidate_gather(crosstalk_core_cmdline_ctx_t *ctx)
{
    char line[CROSSTALK_CORE_CMDLINE_CAP] = LDG_ARR_ZERO_INIT;
    const char *word = 0x0;
    uint64_t word_len = 0;
    uint32_t name_len = 0;
    uint32_t got = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->candidate_cunt = 0;
    ctx->candidate_at = word_at_get(ctx->buff);
    word = &ctx->buff[ctx->candidate_at];
    word_len = (uint64_t)strlen(word);

    if (ctx->candidate_at == 0)
    {
        for (idx = 0; idx < ctx->verbs_cunt && ctx->candidate_cunt < (uint32_t)CROSSTALK_CORE_CMDLINE_CANDIDATE_CUNT; idx++)
        {
            if (candidate_prefix_is(ctx->verb_name[idx], word, word_len) != LDG_TRUTH_TRUE) { continue; }

            ret = ldg_str_rbrcpy(ctx->candidate[ctx->candidate_cunt], ctx->verb_name[idx], (uint64_t)CROSSTALK_CORE_CMDLINE_CANDIDATE_CAP);
            if (ret != LDG_ERR_AOK) { continue; }

            name_len = crosstalk_core_cmdline_len_get(ctx->candidate[ctx->candidate_cunt]);

            if (ctx->verb_arg_max[idx] == 0 && name_len + (uint32_t)LDG_STR_TERM_SIZE < (uint32_t)CROSSTALK_CORE_CMDLINE_CANDIDATE_CAP)
            {
                ctx->candidate[ctx->candidate_cunt][name_len] = CROSSTALK_CORE_CMDLINE_STMT_END;
                ctx->candidate[ctx->candidate_cunt][name_len + (uint32_t)LDG_STR_TERM_SIZE] = LDG_STR_TERM;
            }

            ctx->candidate_cunt++;
        }

        return candidate_sift(ctx, word);
    }

    if (ctx->candidate_cb == 0x0) { return LDG_ERR_AOK; }

    memcpy(line, ctx->buff, (uint64_t)ctx->candidate_at);
    line[ctx->candidate_at] = LDG_STR_TERM;

    ret = ctx->candidate_cb(ctx->candidate_user, line, arg_idx_get(ctx->buff, ctx->candidate_at), word, ctx->candidate, (uint32_t)CROSSTALK_CORE_CMDLINE_CANDIDATE_CUNT, &got);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->candidate_cunt = got > (uint32_t)CROSSTALK_CORE_CMDLINE_CANDIDATE_CUNT ? (uint32_t)CROSSTALK_CORE_CMDLINE_CANDIDATE_CUNT : got;

    return candidate_sift(ctx, word);
}

static uint32_t word_put(crosstalk_core_cmdline_ctx_t *ctx, const char *text, uint32_t len)
{
    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->candidate_at + len + (uint32_t)LDG_STR_TERM_SIZE > (uint32_t)CROSSTALK_CORE_CMDLINE_CAP) { return LDG_ERR_FULL; }

    memcpy(&ctx->buff[ctx->candidate_at], text, (uint64_t)len);
    ctx->buff[ctx->candidate_at + len] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

static uint32_t ghost_refresh(crosstalk_core_cmdline_ctx_t *ctx)
{
    const char *entry = 0x0;
    const char *word = 0x0;
    uint32_t len = 0;
    uint32_t idx = 0;
    uint32_t pick_len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->ghost[0] = LDG_STR_TERM;

    if (ctx->isearch_on == LDG_TRUTH_TRUE || ctx->cursor != (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_END) { return LDG_ERR_AOK; }

    len = crosstalk_core_cmdline_len_get(ctx->buff);
    if (len == 0) { return LDG_ERR_AOK; }

    for (idx = ctx->hist_cunt; idx > 0; idx--)
    {
        if (crosstalk_core_cmdline_hist_get(ctx, idx - (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP, &entry) != LDG_ERR_AOK) { continue; }

        if (strncmp(entry, ctx->buff, (uint64_t)len) != 0 || entry[len] == LDG_STR_TERM) { continue; }

        return ldg_str_rbrcpy(ctx->ghost, entry, (uint64_t)sizeof(ctx->ghost));
    }

    ret = candidate_gather(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    word = &ctx->buff[ctx->candidate_at];

    if (word[0] == LDG_STR_TERM || ctx->candidate_cunt == 0) { return LDG_ERR_AOK; }

    pick_len = crosstalk_core_cmdline_len_get(ctx->candidate[CROSSTALK_CORE_CMDLINE_CANDIDATE_FIRST]);

    if (pick_len <= len - ctx->candidate_at || ctx->candidate_at + pick_len + (uint32_t)LDG_STR_TERM_SIZE > (uint32_t)CROSSTALK_CORE_CMDLINE_CAP) { return LDG_ERR_AOK; }

    memcpy(ctx->ghost, ctx->buff, (uint64_t)ctx->candidate_at);
    memcpy(&ctx->ghost[ctx->candidate_at], ctx->candidate[CROSSTALK_CORE_CMDLINE_CANDIDATE_FIRST], (uint64_t)pick_len);
    ctx->ghost[ctx->candidate_at + pick_len] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

static uint32_t ghost_accept(crosstalk_core_cmdline_ctx_t *ctx, uint8_t word_only)
{
    uint32_t len = 0;
    uint32_t ghost_len = 0;
    uint32_t end = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    len = crosstalk_core_cmdline_len_get(ctx->buff);
    ghost_len = crosstalk_core_cmdline_len_get(ctx->ghost);

    if (ghost_len <= len || strncmp(ctx->ghost, ctx->buff, (uint64_t)len) != 0) { return LDG_ERR_AOK; }

    end = ghost_len;

    if (word_only == LDG_TRUTH_TRUE)
    {
        end = len;

        while (end < ghost_len && ctx->ghost[end] == CROSSTALK_CORE_CMDLINE_WORD_SEP) { end++; }

        while (end < ghost_len && ctx->ghost[end] != CROSSTALK_CORE_CMDLINE_WORD_SEP && ctx->ghost[end] != CROSSTALK_CORE_CMDLINE_PATH_SEP) { end++; }

        if (end < ghost_len && ctx->ghost[end] == CROSSTALK_CORE_CMDLINE_PATH_SEP) { end++; }
    }

    memcpy(ctx->buff, ctx->ghost, (uint64_t)end);
    ctx->buff[end] = LDG_STR_TERM;
    ctx->cursor = (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_END;

    return ghost_refresh(ctx);
}

static uint32_t candidate_tab(crosstalk_core_cmdline_ctx_t *ctx, uint8_t back)
{
    const char *word = 0x0;
    const char *first = 0x0;
    uint32_t word_len = 0;
    uint32_t common = 0;
    uint32_t pick_len = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->cursor = (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_END;

    if (ctx->candidate_on == LDG_TRUTH_TRUE && ctx->candidate_cunt != 0)
    {
        if (ctx->candidate_sel == (uint32_t)UINT32_MAX) { ctx->candidate_sel = back == LDG_TRUTH_TRUE ? ctx->candidate_cunt - (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP : (uint32_t)CROSSTALK_CORE_CMDLINE_CANDIDATE_FIRST; }
        else if (back == LDG_TRUTH_TRUE) { ctx->candidate_sel = (ctx->candidate_sel + ctx->candidate_cunt - (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP) % ctx->candidate_cunt; }
        else { ctx->candidate_sel = (ctx->candidate_sel + (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP) % ctx->candidate_cunt; }

        return word_put(ctx, ctx->candidate[ctx->candidate_sel], crosstalk_core_cmdline_len_get(ctx->candidate[ctx->candidate_sel]));
    }

    ret = candidate_gather(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (ctx->candidate_cunt == 0) { return LDG_ERR_AOK; }

    first = ctx->candidate[CROSSTALK_CORE_CMDLINE_CANDIDATE_FIRST];

    if (ctx->candidate_cunt == (uint32_t)CROSSTALK_CORE_CMDLINE_MATCH_UNIQUE)
    {
        pick_len = crosstalk_core_cmdline_len_get(first);

        ret = word_put(ctx, first, pick_len);
        if (ret != LDG_ERR_AOK) { return ret; }

        if (pick_len != 0 && first[pick_len - (uint32_t)LDG_STR_TERM_SIZE] != CROSSTALK_CORE_CMDLINE_PATH_SEP && first[pick_len - (uint32_t)LDG_STR_TERM_SIZE] != CROSSTALK_CORE_CMDLINE_STMT_END)
        {
            ret = text_ch_append(ctx->buff, (uint32_t)CROSSTALK_CORE_CMDLINE_WORD_SEP);
            if (ret != LDG_ERR_AOK && ret != LDG_ERR_FULL) { return ret; }
        }

        return ghost_refresh(ctx);
    }

    word = &ctx->buff[ctx->candidate_at];
    word_len = crosstalk_core_cmdline_len_get(word);
    common = crosstalk_core_cmdline_len_get(first);

    for (idx = (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP; idx < ctx->candidate_cunt; idx++)
    {
        uint32_t at = 0;

        while (at < common && ctx->candidate[idx][at] == first[at]) { at++; }

        common = at;
    }

    if (common > word_len)
    {
        ret = word_put(ctx, first, common);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    ctx->ghost[0] = LDG_STR_TERM;
    ctx->candidate_sel = (uint32_t)UINT32_MAX;
    ctx->candidate_on = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t candidate_close(crosstalk_core_cmdline_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->candidate_on = LDG_TRUTH_FALSE;
    ctx->candidate_sel = (uint32_t)UINT32_MAX;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_cmdline_key_isr(crosstalk_core_cmdline_ctx_t *ctx, const ldg_term_key_event_t *irq, uint8_t *consumed)
{
    uint32_t held_sel = (uint32_t)UINT32_MAX;
    uint32_t ret = 0;
    uint8_t ctrl = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0 || irq == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    if (ctx->open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    *consumed = LDG_TRUTH_TRUE;

    if (irq->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE) { return LDG_ERR_AOK; }

    ctrl = (irq->mods & (uint32_t)LDG_TERM_MOD_CTRL) != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (irq->key == (uint8_t)LDG_TERM_KEY_ESCAPE) { return crosstalk_core_cmdline_esc(ctx); }

    if (irq->codepoint == (uint32_t)CROSSTALK_CORE_CMDLINE_CLEAR_CTRL_CODE || (ctrl == LDG_TRUTH_TRUE && (irq->codepoint == (uint32_t)CROSSTALK_CORE_CMDLINE_CLEAR_CODEPOINT || irq->codepoint == (uint32_t)CROSSTALK_CORE_CMDLINE_CLEAR_CODEPOINT_UPPER)))
    {
        ctx->buff[0] = LDG_STR_TERM;
        ctx->ghost[0] = LDG_STR_TERM;
        ctx->isearch[0] = LDG_STR_TERM;
        ctx->hist_sel = (uint32_t)UINT32_MAX;
        ctx->isearch_sel = (uint32_t)UINT32_MAX;
        ctx->isearch_on = LDG_TRUTH_FALSE;
        ctx->cursor = (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_END;

        return candidate_close(ctx);
    }

    if (irq->codepoint == (uint32_t)CROSSTALK_CORE_CMDLINE_ISEARCH_CTRL_CODE || (ctrl == LDG_TRUTH_TRUE && (irq->codepoint == (uint32_t)CROSSTALK_CORE_CMDLINE_ISEARCH_CODEPOINT || irq->codepoint == (uint32_t)CROSSTALK_CORE_CMDLINE_ISEARCH_CODEPOINT_UPPER))) { return isearch_step(ctx, (irq->mods & (uint32_t)LDG_TERM_MOD_SHIFT) != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    if (irq->key == (uint8_t)LDG_TERM_KEY_HOME)
    {
        ctx->drawer_scroll = ctx->drawer_scroll + (uint32_t)CROSSTALK_CORE_CMDLINE_DRAWER_SHOW_CUNT;

        return LDG_ERR_AOK;
    }

    if (irq->key == (uint8_t)LDG_TERM_KEY_END && ctx->isearch_on != LDG_TRUTH_TRUE && ctx->ghost[0] != LDG_STR_TERM) { return ghost_accept(ctx, LDG_TRUTH_FALSE); }

    if (irq->key == (uint8_t)LDG_TERM_KEY_END)
    {
        ctx->drawer_scroll = ctx->drawer_scroll > (uint32_t)CROSSTALK_CORE_CMDLINE_DRAWER_SHOW_CUNT ? ctx->drawer_scroll - (uint32_t)CROSSTALK_CORE_CMDLINE_DRAWER_SHOW_CUNT : (uint32_t)CROSSTALK_CORE_CMDLINE_DRAWER_SCROLL_NONE;

        return LDG_ERR_AOK;
    }

    if (ctx->isearch_on == LDG_TRUTH_TRUE) { return isearch_key_isr(ctx, irq); }

    if (irq->key == (uint8_t)LDG_TERM_KEY_TAB) { return candidate_tab(ctx, (irq->mods & (uint32_t)LDG_TERM_MOD_SHIFT) != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    if (ctx->candidate_on == LDG_TRUTH_TRUE)
    {
        held_sel = ctx->candidate_sel;

        ret = candidate_close(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if ((irq->key == (uint8_t)LDG_TERM_KEY_ENTER && held_sel != (uint32_t)UINT32_MAX) || (irq->key == (uint8_t)LDG_TERM_KEY_RHS && irq->mods == 0)) { return ghost_refresh(ctx); }
    }

    if (ctrl == LDG_TRUTH_TRUE && irq->key == (uint8_t)LDG_TERM_KEY_LHS) { return cursor_word_mov(ctx, LDG_TRUTH_TRUE); }

    if (ctrl == LDG_TRUTH_TRUE && irq->key == (uint8_t)LDG_TERM_KEY_RHS && ctx->cursor != (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_END)
    {
        ret = cursor_word_mov(ctx, LDG_TRUTH_FALSE);
        if (ret != LDG_ERR_AOK) { return ret; }

        return ghost_refresh(ctx);
    }

    if (irq->codepoint == (uint32_t)CROSSTALK_CORE_CMDLINE_WORD_FWD_CTRL_CODE || (ctrl == LDG_TRUTH_TRUE && (irq->key == (uint8_t)LDG_TERM_KEY_RHS || irq->codepoint == (uint32_t)CROSSTALK_CORE_CMDLINE_WORD_FWD_CODEPOINT || irq->codepoint == (uint32_t)CROSSTALK_CORE_CMDLINE_WORD_FWD_CODEPOINT_UPPER))) { return ghost_accept(ctx, LDG_TRUTH_TRUE); }

    if (irq->mods == 0 && irq->key == (uint8_t)LDG_TERM_KEY_LHS) { return cursor_mov(ctx, LDG_TRUTH_TRUE); }

    if (irq->mods == 0 && irq->key == (uint8_t)LDG_TERM_KEY_RHS && ctx->cursor != (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_END)
    {
        ret = cursor_mov(ctx, LDG_TRUTH_FALSE);
        if (ret != LDG_ERR_AOK) { return ret; }

        return ghost_refresh(ctx);
    }

    if (irq->mods == 0 && irq->key == (uint8_t)LDG_TERM_KEY_RHS) { return ghost_accept(ctx, LDG_TRUTH_FALSE); }

    if (irq->key == (uint8_t)LDG_TERM_KEY_ENTER) { return line_submit(ctx); }

    if (irq->key == (uint8_t)LDG_TERM_KEY_UP || irq->key == (uint8_t)LDG_TERM_KEY_PAGE_UP) { ret = hist_walk(ctx, LDG_TRUTH_TRUE); }
    else if (irq->key == (uint8_t)LDG_TERM_KEY_DOWN || irq->key == (uint8_t)LDG_TERM_KEY_PAGE_DOWN) { ret = hist_walk(ctx, LDG_TRUTH_FALSE); }
    else if (irq->key == (uint8_t)LDG_TERM_KEY_BACKSPACE) { ret = text_ch_back_drop(ctx); }
    else if (irq->codepoint == (uint32_t)CROSSTALK_CORE_CMDLINE_WORD_DROP_CTRL_CODE || (ctrl == LDG_TRUTH_TRUE && (irq->codepoint == (uint32_t)CROSSTALK_CORE_CMDLINE_WORD_DROP_CODEPOINT || irq->codepoint == (uint32_t)CROSSTALK_CORE_CMDLINE_WORD_DROP_CODEPOINT_UPPER))) { ret = text_word_back_drop(ctx); }
    else { ret = text_irq_insert(ctx, irq); }

    if (ret != LDG_ERR_AOK && ret != LDG_ERR_FULL) { return ret; }

    while (ctx->buff[0] == ':' || ctx->buff[0] == '!')
    {
        uint32_t shift_len = crosstalk_core_cmdline_len_get(ctx->buff);

        memmove(ctx->buff, &ctx->buff[LDG_STR_TERM_SIZE], (uint64_t)shift_len);

        if (ctx->cursor != (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_END && ctx->cursor != 0) { ctx->cursor = ctx->cursor - (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_STEP; }
    }

    return ghost_refresh(ctx);
}

uint32_t crosstalk_core_cmdline_line_take(crosstalk_core_cmdline_ctx_t *ctx, char *out, uint32_t out_cap)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    out[0] = LDG_STR_TERM;

    if (ctx->line_rdy != LDG_TRUTH_TRUE) { return LDG_ERR_EMPTY; }

    ret = ldg_str_rbrcpy(out, ctx->line, (uint64_t)out_cap);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    ctx->line[0] = LDG_STR_TERM;
    ctx->line_rdy = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_cmdline_draw(crosstalk_core_cmdline_ctx_t *ctx, ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t col_cunt)
{
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t bg = LDG_STRUCT_ZERO_INIT;
    const char *match_name = 0x0;
    const char *match_help = 0x0;
    const char *isearch_match = 0x0;
    const char *text = 0x0;
    const char *ghost_rest = 0x0;
    uint64_t text_len = 0;
    uint32_t text_start = 0;
    uint32_t cursor_at = 0;
    uint32_t cursor_col = 0;
    uint32_t prompt_w = 0;
    uint32_t hint_col = 0;
    uint32_t verb_len = 0;
    uint32_t match_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = verb_match_get(ctx, &verb_len, &match_cunt, &match_name, &match_help);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->isearch_on == LDG_TRUTH_TRUE && ctx->isearch_sel != (uint32_t)UINT32_MAX && crosstalk_core_cmdline_hist_get(ctx, ctx->isearch_sel, &isearch_match) != LDG_ERR_AOK) { isearch_match = 0x0; }

    if (ctx->isearch_on == LDG_TRUTH_TRUE) { if (isearch_match != 0x0) { ret = (uint32_t)snprintf(ctx->hint, sizeof(ctx->hint), CROSSTALK_CORE_CMDLINE_ISEARCH_HINT_FMT, isearch_match); }
        else { ret = ldg_str_rbrcpy(ctx->hint, CROSSTALK_CORE_CMDLINE_ISEARCH_NONE_TEXT, (uint64_t)sizeof(ctx->hint)); } }
    else if (ctx->candidate_on == LDG_TRUTH_TRUE && ctx->candidate_sel != (uint32_t)UINT32_MAX) { ret = (uint32_t)snprintf(ctx->hint, sizeof(ctx->hint), CROSSTALK_CORE_CMDLINE_CANDIDATE_SEL_FMT, ctx->candidate_sel + (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP, ctx->candidate_cunt); }
    else if (ctx->candidate_on == LDG_TRUTH_TRUE) { ret = (uint32_t)snprintf(ctx->hint, sizeof(ctx->hint), CROSSTALK_CORE_CMDLINE_CANDIDATE_LIST_FMT, ctx->candidate_cunt); }
    else if (match_help != 0x0 && match_cunt == CROSSTALK_CORE_CMDLINE_MATCH_UNIQUE) { ret = ldg_str_rbrcpy(ctx->hint, match_help, (uint64_t)sizeof(ctx->hint)); }
    else if (match_cunt > CROSSTALK_CORE_CMDLINE_MATCH_UNIQUE) { ret = (uint32_t)snprintf(ctx->hint, sizeof(ctx->hint), "%u matches; tab to complete", match_cunt); }
    else { ctx->hint[0] = LDG_STR_TERM; }

    prompt_w = (uint32_t)((double)col_cunt * CROSSTALK_CORE_CMDLINE_PROMPT_FRAC);
    if (prompt_w <= (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_W) { prompt_w = col_cunt; }

    ret = cmdline_row_clear(render, col, row, col_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    text = ctx->isearch_on == LDG_TRUTH_TRUE ? ctx->isearch : ctx->buff;

    ret = ldg_str_len_get(text, (uint64_t)CROSSTALK_CORE_CMDLINE_CAP, &text_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    cursor_at = ctx->isearch_on == LDG_TRUTH_TRUE ? (uint32_t)text_len : cursor_pos_get(ctx);

    text_start = (uint32_t)CROSSTALK_CORE_CMDLINE_TEXT_ORIG;
    if (cursor_at + (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_W > prompt_w) { text_start = cursor_at + (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_W - prompt_w; }

    ret = cmdline_text_put(render, col, row, prompt_w, text + text_start, (uint64_t)CROSSTALK_CORE_CMDLINE_CAP - (uint64_t)text_start, (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    cursor_col = cursor_at - text_start;

    if (ctx->isearch_on != LDG_TRUTH_TRUE && (uint64_t)crosstalk_core_cmdline_len_get(ctx->ghost) > text_len && strncmp(ctx->ghost, ctx->buff, (uint64_t)text_len) == 0) { ghost_rest = &ctx->ghost[text_len]; }

    if (ghost_rest != 0x0 && cursor_col < prompt_w)
    {
        ret = cmdline_text_put(render, col + cursor_col, row, prompt_w - cursor_col, ghost_rest, (uint64_t)CROSSTALK_CORE_CMDLINE_CAP - text_len, (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, &bg);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_CELL_PANE_RGBA, &fg); }

    if (ret == LDG_ERR_AOK && ghost_rest != 0x0) { ret = ldg_term_render_text_draw(render, col + cursor_col, row, ghost_rest, (uint64_t)CROSSTALK_CORE_CMDLINE_CURSOR_W, &fg, &bg); }
    else if (ret == LDG_ERR_AOK && (uint64_t)cursor_at < text_len) { ret = ldg_term_render_text_draw(render, col + cursor_col, row, &text[cursor_at], (uint64_t)CROSSTALK_CORE_CMDLINE_CURSOR_W, &fg, &bg); }
    else if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_span_fill(render, col + cursor_col, row, (uint32_t)CROSSTALK_CORE_CMDLINE_CURSOR_W, &fg, &bg); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    hint_col = prompt_w + (uint32_t)CROSSTALK_CORE_CMDLINE_HINT_GAP;

    if (hint_col < col_cunt)
    {
        ret = cmdline_text_put(render, col + hint_col, row, col_cunt - hint_col, ctx->hint, (uint64_t)sizeof(ctx->hint), (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ctx->drawer_col = col;
    ctx->drawer_row = row;
    ctx->drawer_col_cunt = prompt_w;

    return LDG_ERR_AOK;
}
