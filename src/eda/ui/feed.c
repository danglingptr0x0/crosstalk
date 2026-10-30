#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "eda/ui/feed.h"
#include "ui/cell.h"
#include "ui/theme.h"

#define FEED_STEP 1
#define FEED_ORIG_COL 0
#define FEED_HINT_TEXT "feed <sigs>; cycle-stamped change events; feed off releases"
#define FEED_ROW_FMT "%12" PRIu64 " %-12.12s 0x%" PRIX64 "%.24s"
#define FEED_LINE_CAP 128
#define FEED_DRAIN_CAP 128
#define FEED_EXC_MARK_A "exc"
#define FEED_EXC_MARK_B "cause"
#define FEED_EXC_INVALID " invalid op"
#define FEED_EXC_UNALIGNED " unaligned"
#define FEED_EXC_STACK_OVER " stack over"
#define FEED_EXC_STACK_UNDER " stack under"
#define FEED_EXC_NONE_TEXT ""
#define FEED_EXC_DIV_ZERO " divide by zero"
#define FEED_EXC_DR14 " dr14 write"
#define FEED_EXC_VAL_INVALID 1
#define FEED_EXC_VAL_UNALIGNED 2
#define FEED_EXC_VAL_STACK_OVER 3
#define FEED_EXC_VAL_STACK_UNDER 4
#define FEED_EXC_VAL_DIV_ZERO 5
#define FEED_EXC_VAL_DR14 6
#define FEED_FAULT_HDR_FMT "fault halt; cause:%.16s; loc: "
#define FEED_FAULT_INSTR_TEXT "; instr: "
#define FEED_FAULT_ADDR_TEXT "addr: "
#define FEED_FAULT_MID_FMT "; size: %" PRIu64 "; sp: "
#define FEED_FAULT_SP_TEXT "sp: "
#define FEED_FAULT_STEP_FMT "; step: %" PRIu64
#define FEED_HEX_FMT "0x%" PRIX64
#define FEED_WORD_FMT "0x%08" PRIX64
#define FEED_FAULT_UNKNOWN_TEXT " unknown"

static const char* feed_cause_name_get(uint64_t val)
{
    if (val == (uint64_t)FEED_EXC_VAL_INVALID) { return FEED_EXC_INVALID; }

    if (val == (uint64_t)FEED_EXC_VAL_UNALIGNED) { return FEED_EXC_UNALIGNED; }

    if (val == (uint64_t)FEED_EXC_VAL_STACK_OVER) { return FEED_EXC_STACK_OVER; }

    if (val == (uint64_t)FEED_EXC_VAL_STACK_UNDER) { return FEED_EXC_STACK_UNDER; }

    if (val == (uint64_t)FEED_EXC_VAL_DIV_ZERO) { return FEED_EXC_DIV_ZERO; }

    if (val == (uint64_t)FEED_EXC_VAL_DR14) { return FEED_EXC_DR14; }

    return FEED_EXC_NONE_TEXT;
}

static const char* feed_exc_note_get(const crosstalk_eda_ui_feed_lane_t *lane, uint64_t val)
{
    if (strstr(lane->name, FEED_EXC_MARK_A) == 0x0 && strstr(lane->name, FEED_EXC_MARK_B) == 0x0) { return FEED_EXC_NONE_TEXT; }

    return feed_cause_name_get(val);
}

static uint32_t feed_seg_draw(ldg_term_render_t *render, const char *text, uint32_t *pos_col, uint32_t row, uint32_t rgba)
{
    uint32_t text_w = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || text == 0x0 || pos_col == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_ui_cell_rgba_put(render, *pos_col, row, text, (uint64_t)FEED_LINE_CAP, rgba, &text_w);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *pos_col = *pos_col + text_w;

    return LDG_ERR_AOK;
}

static uint32_t feed_pump(crosstalk_eda_ui_feed_ctx_t *ctx)
{
    crosstalk_eda_logic_probe_sample_t drain[FEED_DRAIN_CAP];
    uint32_t got = 0;
    uint32_t lane_idx = 0;
    uint32_t idx = 0;
    uint32_t at = 0;

    if (ctx->probes == 0x0) { return LDG_ERR_NOT_INIT; }

    for (lane_idx = 0; lane_idx < ctx->lane_cunt; lane_idx = lane_idx + 0x1)
    {
        if (ctx->lane[lane_idx].live != LDG_TRUTH_TRUE) { continue; }

        if (crosstalk_eda_logic_probe_live_is(ctx->probes, ctx->lane[lane_idx].probe_handle) != LDG_TRUTH_TRUE) { continue; }

        if (crosstalk_eda_logic_probe_drain(ctx->probes, ctx->lane[lane_idx].probe_handle, drain, (uint32_t)FEED_DRAIN_CAP, &got) != LDG_ERR_AOK) { continue; }

        for (idx = 0; idx < got; idx = idx + 0x1)
        {
            if (ctx->event_cunt < (uint32_t)CROSSTALK_EDA_UI_FEED_EVENT_CAP)
            {
                at = (ctx->event_hd + ctx->event_cunt) % (uint32_t)CROSSTALK_EDA_UI_FEED_EVENT_CAP;
                ctx->event_cunt = ctx->event_cunt + 0x1;
            }
            else
            {
                at = ctx->event_hd;
                ctx->event_hd = (ctx->event_hd + 0x1) % (uint32_t)CROSSTALK_EDA_UI_FEED_EVENT_CAP;
            }

            ctx->event[at].cycle = drain[idx].cycle;
            ctx->event[at].val = drain[idx].val;
            ctx->event[at].lane_idx = lane_idx;
        }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_feed_init(crosstalk_eda_ui_feed_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->win.scroll_on = LDG_TRUTH_TRUE;
    ctx->win.open = LDG_TRUTH_FALSE;
    ctx->open = LDG_TRUTH_FALSE;
    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_feed_bind(crosstalk_eda_ui_feed_ctx_t *ctx, crosstalk_eda_logic_probe_ctx_t *probes)
{
    if (LDG_UNLIKELY(ctx == 0x0 || probes == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->probes = probes;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_feed_lane_add(crosstalk_eda_ui_feed_ctx_t *ctx, const char *name, uint32_t probe_handle, uint32_t width)
{
    crosstalk_eda_ui_feed_lane_t *lane = 0x0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (at = 0; at < ctx->lane_cunt; at = at + 0x1) { if (strcmp(ctx->lane[at].name, name) == 0) { return LDG_ERR_EXISTS; } }

    if (ctx->lane_cunt >= (uint32_t)CROSSTALK_EDA_UI_FEED_LANE_CAP) { return LDG_ERR_FULL; }

    lane = &ctx->lane[ctx->lane_cunt];
    memset(lane, 0x0, sizeof(*lane));

    if (ldg_str_rbrcpy(lane->name, name, sizeof(lane->name)) != LDG_ERR_AOK) { return LDG_ERR_STR_TRUNC; }

    lane->probe_handle = probe_handle;
    lane->width = width;
    lane->live = LDG_TRUTH_TRUE;
    ctx->lane_cunt = ctx->lane_cunt + 0x1;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_feed_lane_handle_get(const crosstalk_eda_ui_feed_ctx_t *ctx, uint32_t lane_idx, uint32_t *out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;

    if (lane_idx >= ctx->lane_cunt) { return LDG_ERR_BOUNDS; }

    *out = ctx->lane[lane_idx].probe_handle;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_feed_lane_cunt_get(const crosstalk_eda_ui_feed_ctx_t *ctx, uint32_t *out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = ctx->lane_cunt;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_feed_fault_set(crosstalk_eda_ui_feed_ctx_t *ctx, const uint64_t *rec, uint32_t cunt)
{
    if (LDG_UNLIKELY(ctx == 0x0 || rec == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (cunt < (uint32_t)CROSSTALK_EDA_UI_FEED_FAULT_CUNT) { return LDG_ERR_BOUNDS; }

    if (rec[CROSSTALK_HDL_ABI_FAULT_VALID_AT] == 0) { ctx->fault_has = LDG_TRUTH_FALSE; return LDG_ERR_AOK; }

    memcpy(ctx->fault, rec, sizeof(ctx->fault));
    ctx->fault_has = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_feed_rst(crosstalk_eda_ui_feed_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx->lane, 0x0, sizeof(ctx->lane));
    memset(ctx->event, 0x0, sizeof(ctx->event));
    memset(ctx->fault, 0x0, sizeof(ctx->fault));
    ctx->lane_cunt = 0;
    ctx->event_hd = 0;
    ctx->event_cunt = 0;
    ctx->fault_has = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_feed_open_set(crosstalk_eda_ui_feed_ctx_t *ctx, uint8_t open)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->open = open;
    ctx->win.open = open;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_ui_feed_open_is(const crosstalk_eda_ui_feed_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->win.open != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t feed_fault_draw(crosstalk_eda_ui_feed_ctx_t *ctx, ldg_term_render_t *render, uint32_t *row)
{
    char line[FEED_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t cause = 0;
    const char *name = 0x0;
    uint32_t pos_col = FEED_ORIG_COL;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0 || row == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    cause = ctx->fault[CROSSTALK_HDL_ABI_FAULT_CAUSE_AT];
    name = feed_cause_name_get(cause);

    if (name[0] == LDG_STR_TERM) { name = FEED_FAULT_UNKNOWN_TEXT; }

    if (snprintf(line, sizeof(line), FEED_FAULT_HDR_FMT, name) < 0) { return LDG_ERR_STR_FMT; }

    ret = feed_seg_draw(render, line, &pos_col, *row, (uint32_t)CROSSTALK_UI_THEME_HOT_RGBA);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (snprintf(line, sizeof(line), FEED_HEX_FMT, ctx->fault[CROSSTALK_HDL_ABI_FAULT_LOC_AT]) < 0) { return LDG_ERR_STR_FMT; }

    ret = feed_seg_draw(render, line, &pos_col, *row, (uint32_t)CROSSTALK_UI_THEME_HOT_RGBA);
    if (ret == LDG_ERR_AOK) { ret = feed_seg_draw(render, FEED_FAULT_INSTR_TEXT, &pos_col, *row, (uint32_t)CROSSTALK_UI_THEME_HOT_RGBA); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (snprintf(line, sizeof(line), FEED_WORD_FMT, ctx->fault[CROSSTALK_HDL_ABI_FAULT_INSTR_AT]) < 0) { return LDG_ERR_STR_FMT; }

    ret = feed_seg_draw(render, line, &pos_col, *row, (uint32_t)CROSSTALK_UI_THEME_HOT_RGBA);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *row = *row + (uint32_t)FEED_STEP;
    pos_col = FEED_ORIG_COL;

    if (cause == (uint64_t)FEED_EXC_VAL_UNALIGNED || cause == (uint64_t)FEED_EXC_VAL_STACK_OVER)
    {
        ret = feed_seg_draw(render, FEED_FAULT_ADDR_TEXT, &pos_col, *row, (uint32_t)CROSSTALK_UI_THEME_DATA_RGBA);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (snprintf(line, sizeof(line), FEED_HEX_FMT, ctx->fault[CROSSTALK_HDL_ABI_FAULT_ADDR_AT]) < 0) { return LDG_ERR_STR_FMT; }

        ret = feed_seg_draw(render, line, &pos_col, *row, (uint32_t)CROSSTALK_UI_THEME_DATA_RGBA);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (snprintf(line, sizeof(line), FEED_FAULT_MID_FMT, ctx->fault[CROSSTALK_HDL_ABI_FAULT_SIZE_AT]) < 0) { return LDG_ERR_STR_FMT; }

        ret = feed_seg_draw(render, line, &pos_col, *row, (uint32_t)CROSSTALK_UI_THEME_DATA_RGBA);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    else
    {
        ret = feed_seg_draw(render, FEED_FAULT_SP_TEXT, &pos_col, *row, (uint32_t)CROSSTALK_UI_THEME_DATA_RGBA);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (snprintf(line, sizeof(line), FEED_HEX_FMT, ctx->fault[CROSSTALK_HDL_ABI_FAULT_SP_AT]) < 0) { return LDG_ERR_STR_FMT; }

    ret = feed_seg_draw(render, line, &pos_col, *row, (uint32_t)CROSSTALK_UI_THEME_DATA_RGBA);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (snprintf(line, sizeof(line), FEED_FAULT_STEP_FMT, ctx->fault[CROSSTALK_HDL_ABI_FAULT_STEP_AT]) < 0) { return LDG_ERR_STR_FMT; }

    ret = feed_seg_draw(render, line, &pos_col, *row, (uint32_t)CROSSTALK_UI_THEME_DATA_RGBA);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *row = *row + (uint32_t)FEED_STEP;

    return LDG_ERR_AOK;
}

static uint32_t feed_draw_cb(uint64_t win_id, void *user, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    crosstalk_eda_ui_feed_ctx_t *ctx = (crosstalk_eda_ui_feed_ctx_t *)user;
    char line[FEED_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t row = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (win_id != (uint64_t)CROSSTALK_EDA_UI_FEED_WINDOW_ID || col_cunt == 0 || row_cunt == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->fault_has == LDG_TRUTH_TRUE)
    {
        ret = feed_fault_draw(ctx, render, &row);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (ctx->lane_cunt == 0)
    {
        ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)FEED_ORIG_COL, row, FEED_HINT_TEXT, (uint64_t)sizeof(FEED_HINT_TEXT), (uint32_t)CROSSTALK_UI_THEME_DEAD_RGBA, 0x0);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ctx->win.content_row_cunt = row + (uint32_t)FEED_STEP;

        return LDG_ERR_AOK;
    }

    if (feed_pump(ctx) != LDG_ERR_AOK) { return LDG_ERR_NOT_INIT; }

    for (idx = ctx->event_cunt; idx != 0; idx = idx - (uint32_t)FEED_STEP)
    {
        uint32_t at = (ctx->event_hd + idx - (uint32_t)FEED_STEP) % (uint32_t)CROSSTALK_EDA_UI_FEED_EVENT_CAP;
        const crosstalk_eda_ui_feed_lane_t *lane = &ctx->lane[ctx->event[at].lane_idx];
        const char *note = feed_exc_note_get(lane, ctx->event[at].val);

        if (snprintf(line, sizeof(line), FEED_ROW_FMT, ctx->event[at].cycle, lane->name, ctx->event[at].val, note) < 0) { return LDG_ERR_STR_FMT; }

        ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)FEED_ORIG_COL, row, line, (uint64_t)sizeof(line), note[0] != LDG_STR_TERM ? (uint32_t)CROSSTALK_UI_THEME_HOT_RGBA : (uint32_t)CROSSTALK_UI_THEME_DATA_RGBA, 0x0);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        row = row + (uint32_t)FEED_STEP;
    }

    ctx->win.content_row_cunt = row;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_feed_win_reg(crosstalk_eda_ui_feed_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot)
{
    if (LDG_UNLIKELY(ctx == 0x0 || win_nav == 0x0 || out_slot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_ui_window_reg(win_nav, (uint64_t)CROSSTALK_EDA_UI_FEED_WINDOW_ID, CROSSTALK_EDA_UI_FEED_WINDOW_TITLE, &ctx->win, feed_draw_cb, 0x0, ctx, out_slot);
}
