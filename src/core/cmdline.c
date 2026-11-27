#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>
#include <dangling/logging/logging.h>

#include <dangling/term/ln.h>
#include <dangling/term/render.h>
#include <dangling/term/vt.h>

#include "core/log.h"
#include "core/cmdline.h"
#include "ui/cell.h"
#include "ui/theme.h"

#define CROSSTALK_CORE_CMDLINE_TEXT_NONE ""
#define CROSSTALK_CORE_CMDLINE_LINE_ROW_CUNT 1
#define CROSSTALK_CORE_CMDLINE_HIST_STEP 1
#define CROSSTALK_CORE_CMDLINE_WORD_SEP ' '
#define CROSSTALK_CORE_CMDLINE_LEAD_COLON ':'
#define CROSSTALK_CORE_CMDLINE_LEAD_BANG '!'

static uint32_t cmdline_cmd_cb(void *user, const char *const *arg, uint32_t arg_cunt, uint8_t *out_truth)
{
    crosstalk_core_cmdline_ctx_t *ctx = (crosstalk_core_cmdline_ctx_t *)user;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0 || out_truth == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_truth = LDG_TRUTH_INDETERMINATE;

    if (arg_cunt > (uint32_t)LDG_SHELL_ARG_MAX) { return LDG_ERR_FUNC_ARG_INVALID; }

    ctx->verb_seen = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t cmdline_hist_cb(void *user, uint32_t idx, const char **out)
{
    crosstalk_core_cmdline_ctx_t *ctx = (crosstalk_core_cmdline_ctx_t *)user;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (ctx->shell == 0x0) { return LDG_ERR_NOT_INIT; }

    return ldg_shell_hist_get(ctx->shell, idx, out);
}

static uint32_t cmdline_candidate_cb(void *user, const char *text, uint32_t len, char (*out)[LDG_TERM_LN_CANDIDATE_CAP], uint32_t cap, uint32_t *out_cunt, uint32_t *out_at, uint8_t *out_more)
{
    crosstalk_core_cmdline_ctx_t *ctx = (crosstalk_core_cmdline_ctx_t *)user;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0 || out == 0x0 || out_cunt == 0x0 || out_at == 0x0 || out_more == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;
    *out_at = 0;
    *out_more = LDG_TRUTH_FALSE;

    if (ctx->shell == 0x0) { return LDG_ERR_NOT_INIT; }

    while (at < len && text[at] != CROSSTALK_CORE_CMDLINE_WORD_SEP) { at++; }

    if (at == len || ctx->candidate_cb == 0x0) { return ldg_shell_candidate_get(ctx->shell, text, len, out, cap, out_cunt, out_at, out_more); }

    return ctx->candidate_cb(ctx->candidate_user, text, len, out, cap, out_cunt, out_at, out_more);
}

static uint32_t cmdline_hist_ld_cb(void *user, char *dst, uint64_t cap, uint64_t *out_len)
{
    crosstalk_core_cmdline_ctx_t *ctx = (crosstalk_core_cmdline_ctx_t *)user;
    ldg_io_file_t *fh = 0x0;
    uint64_t got = 0;
    uint64_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || dst == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (ctx->hist_path[0] == LDG_STR_TERM || ctx->io_buff == 0x0) { return LDG_ERR_NOT_INIT; }

    if (ldg_io_path_exists_is(ctx->hist_path) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = ldg_io_file_open(ctx->hist_path, LDG_IO_RD_ONLY | LDG_IO_DIRECT, 0x0, &fh);
    if (ret != LDG_ERR_AOK) { return ret; }

    while (len < ctx->io_cap)
    {
        ret = ldg_io_file_rd(fh, &ctx->io_buff[len], ctx->io_cap - len, &got);
        if (ret != LDG_ERR_AOK || got == 0) { break; }

        len = len + got;

        if ((got & ((uint64_t)CROSSTALK_CORE_CMDLINE_IO_BLK - (uint64_t)CROSSTALK_CORE_CMDLINE_HIST_STEP)) != 0) { break; }
    }

    if (ldg_io_file_close(fh) != LDG_ERR_AOK) { ldg_logging_ln_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cmdline hist close failed"); }

    if (ret != LDG_ERR_AOK) { return ret; }

    if (len > cap) { len = cap; }

    if (len != 0) { memcpy(dst, ctx->io_buff, len); }

    *out_len = len;

    return LDG_ERR_AOK;
}

static uint32_t cmdline_hist_st_cb(void *user, const char *src, uint64_t len)
{
    crosstalk_core_cmdline_ctx_t *ctx = (crosstalk_core_cmdline_ctx_t *)user;
    ldg_io_file_t *fh = 0x0;
    uint64_t padded = 0;
    uint64_t wrote = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || src == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->hist_path[0] == LDG_STR_TERM || ctx->io_buff == 0x0) { return LDG_ERR_NOT_INIT; }

    if (len > ctx->io_cap) { return LDG_ERR_OVERFLOW; }

    padded = (len + (uint64_t)CROSSTALK_CORE_CMDLINE_IO_BLK - (uint64_t)CROSSTALK_CORE_CMDLINE_HIST_STEP) & ~((uint64_t)CROSSTALK_CORE_CMDLINE_IO_BLK - (uint64_t)CROSSTALK_CORE_CMDLINE_HIST_STEP);

    memset(ctx->io_buff, 0x0, padded);

    if (len != 0) { memcpy(ctx->io_buff, src, len); }

    ret = ldg_io_file_open(ctx->hist_path, LDG_IO_WR_ONLY | LDG_IO_CREATE | LDG_IO_TRUNCATE | LDG_IO_DIRECT, (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_MODE, &fh);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (padded != 0)
    {
        ret = ldg_io_file_wr(fh, ctx->io_buff, padded, &wrote);
        if (ret == LDG_ERR_AOK && wrote != padded) { ret = LDG_ERR_IO_WR; }
    }

    if (ret == LDG_ERR_AOK) { ret = ldg_io_file_truncate(fh, len); }

    if (ret == LDG_ERR_AOK) { ret = ldg_io_file_commit(fh); }

    if (ldg_io_file_close(fh) != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    return ret;
}

uint32_t crosstalk_core_cmdline_verb_reg(crosstalk_core_cmdline_ctx_t *ctx, const char *name, const char *help, uint32_t arg_max)
{
    ldg_shell_cmd_t cmd = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0 || help == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->shell == 0x0) { return LDG_ERR_NOT_INIT; }

    cmd.name = name;
    cmd.help = help;
    cmd.cb = cmdline_cmd_cb;
    cmd.user = ctx;
    cmd.arg_max = arg_max;
    cmd.kind = (uint8_t)LDG_SHELL_CMD_PLAIN;

    return ldg_shell_cmd_reg(ctx->shell, &cmd);
}

uint32_t crosstalk_core_cmdline_candidate_cb_set(crosstalk_core_cmdline_ctx_t *ctx, ldg_term_ln_candidate_cb_t cb, void *user)
{
    if (LDG_UNLIKELY(ctx == 0x0 || cb == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->candidate_cb = cb;
    ctx->candidate_user = user;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_cmdline_prompt_set(crosstalk_core_cmdline_ctx_t *ctx, const char *prompt)
{
    if (LDG_UNLIKELY(ctx == 0x0 || prompt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->edit == 0x0) { return LDG_ERR_NOT_INIT; }

    return ldg_term_ln_prompt_set(ctx->edit, prompt);
}

static void cmdline_parts_drop(crosstalk_core_cmdline_ctx_t *ctx)
{
    if (ctx->edit != 0x0 && ldg_term_ln_uncreate(&ctx->edit) != LDG_ERR_AOK) { ldg_logging_ln_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cmdline line uncreate failed"); }

    if (ctx->shell != 0x0 && ldg_shell_uncreate(&ctx->shell) != LDG_ERR_AOK) { ldg_logging_ln_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cmdline shell uncreate failed"); }

    if (ctx->edit_pool != 0x0 && ldg_mem_pool_destroy(&ctx->edit_pool) != LDG_ERR_AOK) { ldg_logging_ln_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cmdline line pool destroy failed"); }

    if (ctx->pool != 0x0 && ldg_mem_pool_destroy(&ctx->pool) != LDG_ERR_AOK) { ldg_logging_ln_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cmdline shell pool destroy failed"); }

    if (ctx->io_raw != 0x0 && ldg_mem_dealloc(ctx->io_raw) != LDG_ERR_AOK) { ldg_logging_ln_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cmdline io buffer dealloc failed"); }

    ctx->io_raw = 0x0;
    ctx->io_buff = 0x0;
    ctx->io_cap = 0;
    ctx->edit = 0x0;
    ctx->shell = 0x0;
    ctx->edit_pool = 0x0;
    ctx->pool = 0x0;
}

static uint32_t cmdline_parts_make(crosstalk_core_cmdline_ctx_t *ctx)
{
    ldg_shell_conf_t shell_conf = LDG_STRUCT_ZERO_INIT;
    ldg_term_ln_conf_t edit_conf = LDG_STRUCT_ZERO_INIT;
    uint64_t pool_size = 0;
    uint32_t ret = 0;

    ret = ldg_shell_conf_default_get(&shell_conf);
    if (ret == LDG_ERR_AOK) { ret = ldg_shell_pool_size_get(&shell_conf, &pool_size); }

    if (ret == LDG_ERR_AOK) { ret = ldg_mem_pool_create(0x0, pool_size, &ctx->pool); }

    if (ret == LDG_ERR_AOK) { ret = ldg_shell_create(ctx->pool, &shell_conf, &ctx->shell); }

    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_term_ln_conf_default_get(&edit_conf);
    if (ret == LDG_ERR_AOK) { ret = ldg_term_ln_pool_size_get(&edit_conf, &pool_size); }

    if (ret == LDG_ERR_AOK) { ret = ldg_mem_pool_create(0x0, pool_size, &ctx->edit_pool); }

    if (ret == LDG_ERR_AOK) { ret = ldg_term_ln_create(ctx->edit_pool, &edit_conf, &ctx->edit); }

    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_mem_alloc((uint64_t)CROSSTALK_CORE_CMDLINE_IO_CAP + (uint64_t)CROSSTALK_CORE_CMDLINE_IO_BLK, &ctx->io_raw);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->io_buff = (uint8_t *)(((uintptr_t)ctx->io_raw + (uintptr_t)CROSSTALK_CORE_CMDLINE_IO_BLK - (uintptr_t)CROSSTALK_CORE_CMDLINE_HIST_STEP) & ~((uintptr_t)CROSSTALK_CORE_CMDLINE_IO_BLK - (uintptr_t)CROSSTALK_CORE_CMDLINE_HIST_STEP));
    ctx->io_cap = (uint64_t)CROSSTALK_CORE_CMDLINE_IO_CAP;

    ret = ldg_term_ln_hist_cb_set(ctx->edit, cmdline_hist_cb, ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    return ldg_term_ln_candidate_cb_set(ctx->edit, cmdline_candidate_cb, ctx);
}

uint32_t crosstalk_core_cmdline_drawer_init(crosstalk_core_cmdline_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->drawer_ok == LDG_TRUTH_TRUE) { return LDG_ERR_EXISTS; }

    ret = cmdline_parts_make(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        cmdline_parts_drop(ctx);

        return ret;
    }

    ctx->drawer_ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_cmdline_drawer_shutdown(crosstalk_core_cmdline_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->drawer_ok != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ctx->drawer_ok = LDG_TRUTH_FALSE;
    ctx->open = LDG_TRUTH_FALSE;

    cmdline_parts_drop(ctx);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_cmdline_hist_ld(crosstalk_core_cmdline_ctx_t *ctx, const char *path)
{
    ldg_shell_hist_io_t io = LDG_STRUCT_ZERO_INIT;
    uint32_t cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->shell == 0x0) { return LDG_ERR_NOT_INIT; }

    ret = ldg_str_rbrcpy(ctx->hist_path, path, (uint64_t)sizeof(ctx->hist_path));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    io.ld_cb = cmdline_hist_ld_cb;
    io.st_cb = cmdline_hist_st_cb;
    io.user = ctx;

    ret = ldg_shell_hist_io_set(ctx->shell, &io);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_shell_hist_ld(ctx->shell);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (ldg_shell_hist_cunt_get(ctx->shell, &cunt) != LDG_ERR_AOK) { cunt = 0; }

    ldg_logging_ln_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "cmdline: hist loaded; entries: %u32; path: %.255s", cunt, path);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_cmdline_out_push(crosstalk_core_cmdline_ctx_t *ctx, const char *text)
{
    char ln[CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->drawer_ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (text[0] == LDG_STR_TERM) { return LDG_ERR_AOK; }

    ret = ldg_str_rbrcpy(ln, text, (uint64_t)sizeof(ln) - (uint64_t)CROSSTALK_CORE_CMDLINE_LF_ROOM);
    if (ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC) { return ret; }

    ret = ldg_str_len_get(ln, (uint64_t)sizeof(ln), &len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (ln[len - (uint64_t)CROSSTALK_CORE_CMDLINE_LAST_CH_BACK] != LDG_ASCII_LF)
    {
        ln[len] = LDG_ASCII_LF;
        ln[len + (uint64_t)CROSSTALK_CORE_CMDLINE_LF_ROOM] = LDG_STR_TERM;
    }

    return ldg_shell_out_push(ctx->shell, ln);
}

uint32_t crosstalk_core_cmdline_wipe_ck(crosstalk_core_cmdline_ctx_t *ctx)
{
    uint64_t cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->shell == 0x0) { return LDG_ERR_NOT_INIT; }

    ret = ldg_shell_wipe_cunt_get(ctx->shell, &cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (cunt == ctx->wipe_seen) { return LDG_ERR_AOK; }

    ctx->wipe_seen = cunt;

    return ldg_shell_out_seq_get(ctx->shell, &ctx->out_floor);
}

uint32_t crosstalk_core_cmdline_ln_trim(char *text, uint64_t cap, uint64_t *out_len)
{
    uint64_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(text == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    ret = ldg_str_len_get(text, cap, &len);
    if (ret != LDG_ERR_AOK) { return ret; }

    while (len != 0 && (text[len - (uint64_t)CROSSTALK_CORE_CMDLINE_LAST_CH_BACK] == LDG_ASCII_LF || text[len - (uint64_t)CROSSTALK_CORE_CMDLINE_LAST_CH_BACK] == LDG_ASCII_CR))
    {
        len--;
        text[len] = LDG_STR_TERM;
    }

    *out_len = len;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_cmdline_out_seq_get(crosstalk_core_cmdline_ctx_t *ctx, uint64_t *out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (ctx->drawer_ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    return ldg_shell_out_seq_get(ctx->shell, out);
}

uint32_t crosstalk_core_cmdline_out_get(crosstalk_core_cmdline_ctx_t *ctx, uint64_t seq, char *out, uint32_t out_cap, uint64_t *out_seq)
{
    uint64_t got = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0 || out_seq == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    out[0] = LDG_STR_TERM;
    *out_seq = 0;

    if (ctx->drawer_ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ret = ldg_shell_out_get(ctx->shell, seq, out, (uint64_t)out_cap, &got);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (got == seq || got == seq + (uint64_t)CROSSTALK_CORE_CMDLINE_HIST_STEP)
    {
        *out_seq = seq + (uint64_t)CROSSTALK_CORE_CMDLINE_HIST_STEP;
        return LDG_ERR_AOK;
    }

    out[0] = LDG_STR_TERM;

    return LDG_ERR_NOT_FOUND;
}

uint32_t crosstalk_core_cmdline_echo_set(crosstalk_core_cmdline_ctx_t *ctx, uint8_t on)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->echo_off = on == LDG_TRUTH_TRUE ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

void crosstalk_core_cmdline_log_cb(void *cls, const char *app_name, const time_t date, const CROSSTALK_CORE_CMDLINE_LOG_LVL_T level, const char *msg)
{
    crosstalk_core_cmdline_ctx_t *ctx = (crosstalk_core_cmdline_ctx_t *)cls;

    if (ctx == 0x0 || msg == 0x0) { return; }

    if (app_name == 0x0 && date == (time_t)0x0) { return; }

    if (level == LDG_LOGGING_LVL_DEBUG) { return; }

    if (crosstalk_core_cmdline_out_push(ctx, msg) != LDG_ERR_AOK) { return; }
}

uint32_t crosstalk_core_cmdline_open(crosstalk_core_cmdline_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->edit == 0x0) { return LDG_ERR_NOT_INIT; }

    ret = ldg_term_ln_rst(ctx->edit);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->open = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_cmdline_close(crosstalk_core_cmdline_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->open = LDG_TRUTH_FALSE;

    if (ctx->edit == 0x0) { return LDG_ERR_AOK; }

    return ldg_term_ln_rst(ctx->edit);
}

uint8_t crosstalk_core_cmdline_open_is(const crosstalk_core_cmdline_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->open;
}

static uint32_t cmdline_color_get(ldg_term_render_color_t *out_fg, ldg_term_render_color_t *out_ghost_fg, ldg_term_render_color_t *out_bg)
{
    uint32_t ret = 0;

    ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, out_fg);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, out_ghost_fg);
    if (ret != LDG_ERR_AOK) { return ret; }

    return crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_CELL_PANE_RGBA, out_bg);
}

static uint32_t cmdline_drawer_draw(crosstalk_core_cmdline_ctx_t *ctx, ldg_term_render_t *render, const ldg_term_render_color_t *fg, const ldg_term_render_color_t *bg)
{
    char text[CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t seq = 0;
    uint64_t at = 0;
    uint64_t got = 0;
    uint64_t len = 0;
    uint32_t shown = 0;
    uint32_t row = 0;
    uint32_t ret = 0;

    ret = ldg_shell_out_seq_get(ctx->shell, &seq);
    if (ret != LDG_ERR_AOK) { return ret; }

    shown = (uint32_t)CROSSTALK_CORE_CMDLINE_DRAWER_SHOW_CUNT;
    if (shown > ctx->drawer_row) { shown = ctx->drawer_row; }

    if ((uint64_t)shown > seq) { shown = (uint32_t)seq; }

    at = seq - (uint64_t)shown;
    row = ctx->drawer_row - shown;

    if (at < ctx->out_floor && ctx->out_floor <= seq)
    {
        row = row + (uint32_t)(ctx->out_floor - at);
        at = ctx->out_floor;
    }

    while (at < seq && row < ctx->drawer_row)
    {
        ret = crosstalk_core_cmdline_out_get(ctx, at, text, (uint32_t)sizeof(text), &got);
        if (ret != LDG_ERR_AOK)
        {
            at = at + (uint64_t)CROSSTALK_CORE_CMDLINE_HIST_STEP;
            row = row + (uint32_t)CROSSTALK_CORE_CMDLINE_LINE_ROW_CUNT;
            continue;
        }

        ret = crosstalk_ui_cell_span_fill(render, ctx->drawer_col, row, ctx->drawer_col_cunt, fg, bg);
        if (ret != LDG_ERR_AOK) { return ret; }

        ret = crosstalk_core_cmdline_ln_trim(text, (uint64_t)sizeof(text), &len);
        if (ret != LDG_ERR_AOK) { return ret; }

        if (len > (uint64_t)ctx->drawer_col_cunt) { len = (uint64_t)ctx->drawer_col_cunt; }

        if (len != 0)
        {
            ret = ldg_term_render_text_draw(render, ctx->drawer_col, row, text, len, fg, bg);
            if (ret != LDG_ERR_AOK) { return ret; }
        }

        at = at + (uint64_t)CROSSTALK_CORE_CMDLINE_HIST_STEP;
        row = row + (uint32_t)CROSSTALK_CORE_CMDLINE_LINE_ROW_CUNT;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_cmdline_drawer_overlay_draw(crosstalk_core_cmdline_ctx_t *ctx, ldg_term_render_t *render)
{
    ldg_term_ln_view_t view = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t ghost_fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t bg = LDG_STRUCT_ZERO_INIT;
    uint32_t above = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->open != LDG_TRUTH_TRUE || ctx->drawer_ok != LDG_TRUTH_TRUE || ctx->drawer_col_cunt == 0) { return LDG_ERR_AOK; }

    ret = cmdline_color_get(&fg, &ghost_fg, &bg);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_term_ln_view_get(ctx->edit, &view);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (view.candidate_on != LDG_TRUTH_TRUE) { return cmdline_drawer_draw(ctx, render, &fg, &bg); }

    above = (uint32_t)CROSSTALK_CORE_CMDLINE_DRAWER_SHOW_CUNT;
    if (above > ctx->drawer_row) { above = ctx->drawer_row; }

    return ldg_term_ln_draw(ctx->edit, render, ctx->drawer_col, ctx->drawer_row - above, ctx->drawer_col_cunt, above + (uint32_t)CROSSTALK_CORE_CMDLINE_LINE_ROW_CUNT, &fg, &ghost_fg, &bg);
}

static uint32_t cmdline_lead_strip(crosstalk_core_cmdline_ctx_t *ctx)
{
    char text[CROSSTALK_CORE_CMDLINE_CAP] = LDG_ARR_ZERO_INIT;
    const char *typed = 0x0;
    uint32_t typed_len = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    ret = ldg_term_ln_text_get(ctx->edit, &typed, &typed_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (typed == 0x0 || typed_len == 0) { return LDG_ERR_AOK; }

    while (at < typed_len && (typed[at] == CROSSTALK_CORE_CMDLINE_LEAD_COLON || typed[at] == CROSSTALK_CORE_CMDLINE_LEAD_BANG)) { at++; }

    if (at == 0) { return LDG_ERR_AOK; }

    ret = ldg_str_rbrcpy(text, &typed[at], (uint64_t)sizeof(text));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    return ldg_term_ln_text_set(ctx->edit, text);
}

static uint32_t cmdline_line_collect(crosstalk_core_cmdline_ctx_t *ctx)
{
    const char *last = 0x0;
    uint32_t cunt = 0;
    uint8_t kept = LDG_TRUTH_FALSE;

    if (ldg_term_ln_take(ctx->edit, ctx->line, (uint64_t)sizeof(ctx->line)) != LDG_ERR_AOK) { ctx->line[0] = LDG_STR_TERM; }

    if (ctx->line[0] == LDG_STR_TERM) { return LDG_ERR_AOK; }

    if (ctx->echo_off != LDG_TRUTH_TRUE && crosstalk_core_cmdline_out_push(ctx, ctx->line) != LDG_ERR_AOK) { ldg_logging_ln_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cmdline echo push failed"); }

    if (ldg_shell_hist_cunt_get(ctx->shell, &cunt) == LDG_ERR_AOK && cunt != 0 && ldg_shell_hist_get(ctx->shell, cunt - (uint32_t)CROSSTALK_CORE_CMDLINE_HIST_STEP, &last) == LDG_ERR_AOK && last != 0x0 && strncmp(last, ctx->line, (uint64_t)sizeof(ctx->line)) == 0) { kept = LDG_TRUTH_TRUE; }

    if (kept != LDG_TRUTH_TRUE && ldg_shell_hist_push(ctx->shell, ctx->line) != LDG_ERR_AOK) { ldg_logging_ln_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cmdline history push failed"); }

    if (kept != LDG_TRUTH_TRUE && ctx->hist_path[0] != LDG_STR_TERM && ldg_shell_hist_st(ctx->shell) != LDG_ERR_AOK) { ldg_logging_ln_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cmdline: hist store failed"); }

    ctx->verb_seen = LDG_TRUTH_FALSE;

    if (ldg_shell_ln_exec(ctx->shell, ctx->line) == LDG_ERR_AOK && ctx->verb_seen != LDG_TRUTH_TRUE)
    {
        ldg_logging_ln_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "cmdline: run by the shell; line: %.128s", ctx->line);

        if (crosstalk_core_cmdline_wipe_ck(ctx) != LDG_ERR_AOK) { ldg_logging_ln_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cmdline: wipe check failed"); }

        ctx->line[0] = LDG_STR_TERM;

        return crosstalk_core_cmdline_close(ctx);
    }

    ctx->line_rdy = LDG_TRUTH_TRUE;

    ldg_logging_ln_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "cmdline: handed to the script; line: %.128s", ctx->line);

    return crosstalk_core_cmdline_close(ctx);
}

uint32_t crosstalk_core_cmdline_key_isr(crosstalk_core_cmdline_ctx_t *ctx, const ldg_term_key_event_t *irq, uint8_t *consumed)
{
    ldg_term_ln_view_t view = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;
    uint8_t plain = LDG_TRUTH_FALSE;
    uint8_t taken = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0 || irq == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    if (ctx->open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (ctx->edit == 0x0) { return LDG_ERR_NOT_INIT; }

    *consumed = LDG_TRUTH_TRUE;

    if (irq->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE) { return LDG_ERR_AOK; }

    ret = ldg_term_ln_view_get(ctx->edit, &view);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    plain = (view.candidate_on != LDG_TRUTH_TRUE && view.search_on != LDG_TRUTH_TRUE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (plain == LDG_TRUTH_TRUE && irq->key == (uint8_t)LDG_TERM_KEY_ESCAPE) { return crosstalk_core_cmdline_close(ctx); }

    if (plain == LDG_TRUTH_TRUE && irq->key == (uint8_t)LDG_TERM_KEY_ENTER && view.text_len == 0) { return crosstalk_core_cmdline_close(ctx); }

    ret = ldg_term_ln_key_put(ctx->edit, irq, &taken);
    if (ret != LDG_ERR_AOK && ret != LDG_ERR_FULL) { return ret; }

    if (ctx->edit->rdy == LDG_TRUTH_TRUE) { return cmdline_line_collect(ctx); }

    return cmdline_lead_strip(ctx);
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
    ldg_term_render_color_t ghost_fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t bg = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->open != LDG_TRUTH_TRUE || ctx->edit == 0x0 || col_cunt == 0) { return LDG_ERR_AOK; }

    ret = cmdline_color_get(&fg, &ghost_fg, &bg);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_term_ln_draw(ctx->edit, render, col, row, col_cunt, (uint32_t)CROSSTALK_CORE_CMDLINE_LINE_ROW_CUNT, &fg, &ghost_fg, &bg);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->drawer_col = col;
    ctx->drawer_row = row;
    ctx->drawer_col_cunt = col_cunt;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_cmdline_encode(crosstalk_core_cmdline_ctx_t *ctx, uint32_t col_cunt, ldg_byte_t *out, uint64_t cap, uint64_t *out_len)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (ctx->edit == 0x0) { return LDG_ERR_NOT_INIT; }

    return ldg_term_ln_encode(ctx->edit, col_cunt, out, cap, out_len);
}
