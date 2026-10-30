#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/time/time.h>

#include "game/totals.h"
#include "ui/cell.h"
#include "ui/theme.h"

#define TOTALS_STEP 1
#define TOTALS_ORIG_COL 0
#define TOTALS_SECTION_GAP 1
#define TOTALS_LINE_CAP 320
#define TOTALS_PROG_ROW_MAX 8
#define TOTALS_HOURS_PER_DAY 24
#define TOTALS_TITLE "City totals"

static void totals_span_fmt(uint64_t span_ms, char *out, uint64_t out_size)
{
    uint64_t total_secs = span_ms / LDG_MS_PER_SEC;
    uint64_t hours = total_secs / LDG_SECS_PER_HOUR;
    uint64_t mins = (total_secs % LDG_SECS_PER_HOUR) / LDG_SECS_PER_MIN;

    snprintf(out, out_size, "%" PRIu64 "h %" PRIu64 "m", hours, mins);
}

static uint32_t totals_row_draw(ldg_term_render_t *render, uint32_t *row, const char *text)
{
    uint32_t ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)TOTALS_ORIG_COL, *row, text, (uint64_t)TOTALS_LINE_CAP, (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, 0x0);

    *row = *row + (uint32_t)TOTALS_STEP;

    return ret;
}

uint32_t crosstalk_game_totals_init(crosstalk_game_totals_ctx_t *ctx)
{
    crosstalk_game_totals_ctx_t empty = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *ctx = empty;
    ctx->win.scroll_on = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_totals_open_set(crosstalk_game_totals_ctx_t *ctx, uint8_t open)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->open = open;
    ctx->win.open = open;

    return LDG_ERR_AOK;
}

static LDG_NOINLINE uint32_t totals_emu_draw(const crosstalk_eda_ic_fpga_emu_stat_t *stat, ldg_term_render_t *render, uint32_t *row)
{
    char line[TOTALS_LINE_CAP] = LDG_ARR_ZERO_INIT;
    char span[TOTALS_LINE_CAP] = LDG_ARR_ZERO_INIT;
    char hlt_span[TOTALS_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t rows = 0;
    uint32_t idx = 0;
    uint32_t ret = LDG_ERR_AOK;

    if (stat->run_ms > 0 || stat->hlt_ms > 0)
    {
        totals_span_fmt(stat->run_ms, span, sizeof(span));
        totals_span_fmt(stat->hlt_ms, hlt_span, sizeof(hlt_span));

        snprintf(line, sizeof(line), "time emulating: %.31s; halted: %.31s", span, hlt_span);
        ret = totals_row_draw(render, row, line);

        snprintf(line, sizeof(line), "emulator runs: %" PRIu64 "; typed: %" PRIu64 " characters", stat->sesh_cunt, stat->chars);
        if (ret == LDG_ERR_AOK) { ret = totals_row_draw(render, row, line); }
    }

    if (ret != LDG_ERR_AOK || stat->prog_cunt == 0) { return ret; }

    *row = *row + (uint32_t)TOTALS_SECTION_GAP;

    ret = totals_row_draw(render, row, "programs:");

    rows = stat->prog_cunt < (uint32_t)TOTALS_PROG_ROW_MAX ? stat->prog_cunt : (uint32_t)TOTALS_PROG_ROW_MAX;

    for (idx = 0; idx < rows && ret == LDG_ERR_AOK; idx = idx + (uint32_t)TOTALS_STEP)
    {
        totals_span_fmt(stat->prog[idx].run_ms, span, sizeof(span));
        totals_span_fmt(stat->prog[idx].hlt_ms, hlt_span, sizeof(hlt_span));

        snprintf(line, sizeof(line), "%.23s; %.31s; halted: %.31s; runs: %" PRIu64, stat->prog[idx].name, span, hlt_span, stat->prog[idx].launches);
        ret = totals_row_draw(render, row, line);
    }

    return ret;
}

static uint32_t totals_draw_cb(uint64_t win_id, void *user, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    crosstalk_game_totals_ctx_t *ctx = (crosstalk_game_totals_ctx_t *)user;
    const crosstalk_game_totals_info_t *info = 0x0;
    char line[TOTALS_LINE_CAP] = LDG_ARR_ZERO_INIT;
    char span[TOTALS_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t now_ms = 0;
    uint32_t row = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (win_id != (uint64_t)CROSSTALK_GAME_TOTALS_WINDOW_ID || col_cunt == 0 || row_cunt == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    info = &ctx->info;
    now_ms = ldg_time_epoch_ms_get();

    snprintf(line, sizeof(line), "board: %.47s; founded by: %.63s", info->board_name != 0x0 ? info->board_name : "-", info->author_name != 0x0 && info->author_name[0] != LDG_STR_TERM ? info->author_name : "-");
    ret = totals_row_draw(render, &row, line);

    if (ret == LDG_ERR_AOK && info->created_epoch_ms > 0 && now_ms > info->created_epoch_ms)
    {
        uint64_t age_days = (now_ms - info->created_epoch_ms) / LDG_MS_PER_SEC / LDG_SECS_PER_HOUR / TOTALS_HOURS_PER_DAY;

        totals_span_fmt(info->bench_ms_total, span, sizeof(span));
        snprintf(line, sizeof(line), "age: %" PRIu64 " days; time on the bench: %.31s", age_days, span);
        ret = totals_row_draw(render, &row, line);

        totals_span_fmt(info->sim_ms_total, span, sizeof(span));
        snprintf(line, sizeof(line), "time simulating: %.31s", span);
        if (ret == LDG_ERR_AOK) { ret = totals_row_draw(render, &row, line); }
    }

    if (ret == LDG_ERR_AOK && info->emu_stat != 0x0) { ret = totals_emu_draw(info->emu_stat, render, &row); }

    ctx->win.content_row_cunt = row;

    return ret;
}

static uint32_t totals_key_isr(uint64_t win_id, void *user, const ldg_term_key_event_t *key, uint8_t *consumed)
{
    crosstalk_game_totals_ctx_t *ctx = (crosstalk_game_totals_ctx_t *)user;

    if (LDG_UNLIKELY(ctx == 0x0 || key == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    if (win_id != (uint64_t)CROSSTALK_GAME_TOTALS_WINDOW_ID) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (key->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE || key->key != (uint8_t)LDG_TERM_KEY_ESCAPE) { return LDG_ERR_AOK; }

    *consumed = LDG_TRUTH_TRUE;

    return crosstalk_game_totals_open_set(ctx, LDG_TRUTH_FALSE);
}

uint32_t crosstalk_game_totals_win_reg(crosstalk_game_totals_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot)
{
    if (LDG_UNLIKELY(ctx == 0x0 || win_nav == 0x0 || out_slot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_ui_window_reg(win_nav, (uint64_t)CROSSTALK_GAME_TOTALS_WINDOW_ID, TOTALS_TITLE, &ctx->win, totals_draw_cb, totals_key_isr, ctx, out_slot);
}

uint32_t crosstalk_game_totals_frame_set(crosstalk_game_totals_ctx_t *ctx, const crosstalk_game_totals_info_t *info)
{
    if (LDG_UNLIKELY(ctx == 0x0 || info == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->info = *info;

    return LDG_ERR_AOK;
}
