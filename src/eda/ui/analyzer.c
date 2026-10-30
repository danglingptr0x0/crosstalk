#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "eda/ui/analyzer.h"
#include "ui/cell.h"
#include "ui/theme.h"

#define ANALYZER_STEP 1
#define ANALYZER_ORIG_COL 0
#define ANALYZER_HDR_ROW 0
#define ANALYZER_LANE_ROW_FIRST 1
#define ANALYZER_LANE_ROW_CUNT 3
#define ANALYZER_LANE_MID_ROW 1
#define ANALYZER_LANE_LO_ROW 2
#define ANALYZER_LABEL_W 18
#define ANALYZER_BUS_LABEL_MIN_CELLS 6
#define ANALYZER_CELL_MID_DIV 2
#define ANALYZER_HINT_TEXT "analyzer <sigs>; up to 4 lanes; analyzer span <cycles>; analyzer off"
#define ANALYZER_HDR_FMT "span %" PRIu64 " cycles; t %" PRIu64
#define ANALYZER_LABEL_FMT "%.12s 0x%" PRIX64
#define ANALYZER_BUS_LABEL_FMT "%" PRIX64
#define ANALYZER_LINE_CAP 96
#define ANALYZER_DRAIN_CAP 256
#define ANALYZER_BIT_WIDTH 1
#define ANALYZER_CYCLE_ORIG 0

static uint32_t analyzer_lane_pump(crosstalk_eda_ui_analyzer_ctx_t *ctx, crosstalk_eda_ui_analyzer_lane_t *lane)
{
    crosstalk_eda_logic_probe_sample_t drain[ANALYZER_DRAIN_CAP];
    uint32_t got = 0;
    uint32_t idx = 0;
    uint32_t at = 0;

    if (ctx->probes == 0x0 || lane->live != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (crosstalk_eda_logic_probe_live_is(ctx->probes, lane->probe_handle) != LDG_TRUTH_TRUE) { return LDG_ERR_CLOSED; }

    if (crosstalk_eda_logic_probe_drain(ctx->probes, lane->probe_handle, drain, (uint32_t)ANALYZER_DRAIN_CAP, &got) != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    for (idx = 0; idx < got; idx = idx + 0x1)
    {
        if (lane->cunt < (uint32_t)CROSSTALK_EDA_UI_ANALYZER_EDGE_CAP)
        {
            at = (lane->hd + lane->cunt) % (uint32_t)CROSSTALK_EDA_UI_ANALYZER_EDGE_CAP;
            lane->cunt = lane->cunt + 0x1;
        }
        else
        {
            at = lane->hd;
            lane->hd = (lane->hd + 0x1) % (uint32_t)CROSSTALK_EDA_UI_ANALYZER_EDGE_CAP;
        }

        lane->edge_cycle[at] = drain[idx].cycle;
        lane->edge_val[at] = drain[idx].val;
        lane->last_val = drain[idx].val;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_analyzer_init(crosstalk_eda_ui_analyzer_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->win.scroll_on = LDG_TRUTH_TRUE;
    ctx->win.open = LDG_TRUTH_FALSE;
    ctx->span = (uint64_t)CROSSTALK_EDA_UI_ANALYZER_SPAN_DEFAULT;
    ctx->open = LDG_TRUTH_FALSE;
    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_analyzer_bind(crosstalk_eda_ui_analyzer_ctx_t *ctx, crosstalk_eda_logic_probe_ctx_t *probes)
{
    if (LDG_UNLIKELY(ctx == 0x0 || probes == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->probes = probes;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_analyzer_lane_add(crosstalk_eda_ui_analyzer_ctx_t *ctx, const char *name, uint32_t probe_handle, uint32_t width)
{
    crosstalk_eda_ui_analyzer_lane_t *lane = 0x0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (at = 0; at < ctx->lane_cunt; at = at + 0x1) { if (strcmp(ctx->lane[at].name, name) == 0) { return LDG_ERR_EXISTS; } }

    if (ctx->lane_cunt >= (uint32_t)CROSSTALK_EDA_UI_ANALYZER_LANE_CAP) { return LDG_ERR_FULL; }

    lane = &ctx->lane[ctx->lane_cunt];
    memset(lane, 0x0, sizeof(*lane));

    if (ldg_str_rbrcpy(lane->name, name, sizeof(lane->name)) != LDG_ERR_AOK) { return LDG_ERR_STR_TRUNC; }

    lane->probe_handle = probe_handle;
    lane->width = width;
    lane->live = LDG_TRUTH_TRUE;
    ctx->lane_cunt = ctx->lane_cunt + 0x1;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_analyzer_lane_handle_get(const crosstalk_eda_ui_analyzer_ctx_t *ctx, uint32_t lane_idx, uint32_t *out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;

    if (lane_idx >= ctx->lane_cunt) { return LDG_ERR_BOUNDS; }

    *out = ctx->lane[lane_idx].probe_handle;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_analyzer_lane_cunt_get(const crosstalk_eda_ui_analyzer_ctx_t *ctx, uint32_t *out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = ctx->lane_cunt;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_analyzer_rst(crosstalk_eda_ui_analyzer_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx->lane, 0x0, sizeof(ctx->lane));
    ctx->lane_cunt = 0;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_analyzer_span_set(crosstalk_eda_ui_analyzer_ctx_t *ctx, uint64_t span)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (span < (uint64_t)CROSSTALK_EDA_UI_ANALYZER_SPAN_MIN || span > (uint64_t)CROSSTALK_EDA_UI_ANALYZER_SPAN_MAX) { return LDG_ERR_RANGE; }

    ctx->span = span;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_analyzer_open_set(crosstalk_eda_ui_analyzer_ctx_t *ctx, uint8_t open)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->open = open;
    ctx->win.open = open;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_ui_analyzer_open_is(const crosstalk_eda_ui_analyzer_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->win.open != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

typedef struct analyzer_lane_geom
{
    uint32_t x_s;
    uint32_t w_s;
    uint32_t y_hi;
    uint32_t y_mid;
    uint32_t y_lo;
    uint32_t row;
    uint32_t sample_col;
    uint8_t pudding[4];
} analyzer_lane_geom_t;

static uint32_t analyzer_lane_draw(ldg_term_render_t *render, const crosstalk_eda_ui_analyzer_lane_t *lane, uint32_t rgba, const analyzer_lane_geom_t *geom, uint64_t win_lo, uint64_t win_hi)
{
    char text[ANALYZER_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t seg_x = 0;
    uint32_t seg_val_y = 0;
    uint64_t seg_val = 0;
    uint64_t span = win_hi - win_lo;
    uint8_t seg_have = LDG_TRUTH_FALSE;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || lane == 0x0 || geom == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    seg_x = geom->x_s;

    if (snprintf(text, sizeof(text), ANALYZER_LABEL_FMT, lane->name, lane->last_val) < 0) { return LDG_ERR_STR_FMT; }

    ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)ANALYZER_ORIG_COL, geom->row + (uint32_t)ANALYZER_LANE_MID_ROW, text, (uint64_t)sizeof(text), (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, 0x0);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (span == 0 || geom->sample_col == 0) { return LDG_ERR_AOK; }

    for (idx = 0; idx <= lane->cunt; idx = idx + (uint32_t)ANALYZER_STEP)
    {
        uint64_t at_cycle = 0;
        uint64_t at_val = 0;
        uint32_t at_x = 0;
        uint8_t last_pass = idx == lane->cunt ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

        if (last_pass == LDG_TRUTH_TRUE)
        {
            at_cycle = win_hi;
            at_val = seg_have == LDG_TRUTH_TRUE ? seg_val : lane->last_val;
        }
        else
        {
            uint32_t slot = (lane->hd + idx) % (uint32_t)CROSSTALK_EDA_UI_ANALYZER_EDGE_CAP;

            at_cycle = lane->edge_cycle[slot];
            at_val = lane->edge_val[slot];
        }

        if (at_cycle < win_lo)
        {
            seg_val = at_val;
            seg_have = LDG_TRUTH_TRUE;
            continue;
        }

        if (at_cycle > win_hi) { at_cycle = win_hi; }

        at_x = geom->x_s + (uint32_t)((at_cycle - win_lo) * (uint64_t)geom->w_s / span);

        if (seg_have == LDG_TRUTH_TRUE && at_x > seg_x)
        {
            if (lane->width == (uint32_t)ANALYZER_BIT_WIDTH) { seg_val_y = seg_val != 0 ? geom->y_hi : geom->y_lo; }
            else { seg_val_y = geom->y_mid; }

            ret = crosstalk_ui_cell_line_draw(render, seg_x, seg_val_y, at_x, seg_val_y, rgba);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (lane->width != (uint32_t)ANALYZER_BIT_WIDTH && at_x - seg_x >= (uint32_t)ANALYZER_BUS_LABEL_MIN_CELLS * geom->sample_col)
            {
                if (snprintf(text, sizeof(text), ANALYZER_BUS_LABEL_FMT, seg_val) < 0) { return LDG_ERR_STR_FMT; }

                ret = crosstalk_ui_cell_rgba_put(render, seg_x / geom->sample_col + (uint32_t)ANALYZER_STEP, geom->row, text, (uint64_t)sizeof(text), (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, 0x0);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
        }

        if (last_pass != LDG_TRUTH_TRUE && seg_have == LDG_TRUTH_TRUE && at_val != seg_val)
        {
            ret = crosstalk_ui_cell_line_draw(render, at_x, geom->y_hi, at_x, geom->y_lo, rgba);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        if (last_pass != LDG_TRUTH_TRUE)
        {
            seg_x = at_x;
            seg_val = at_val;
            seg_have = LDG_TRUTH_TRUE;
        }
    }

    return LDG_ERR_AOK;
}

static uint32_t analyzer_draw_cb(uint64_t win_id, void *user, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    crosstalk_eda_ui_analyzer_ctx_t *ctx = (crosstalk_eda_ui_analyzer_ctx_t *)user;
    char text[ANALYZER_LINE_CAP] = LDG_ARR_ZERO_INIT;
    const uint32_t lane_rgba[CROSSTALK_EDA_UI_ANALYZER_LANE_CAP] = { CROSSTALK_UI_THEME_LIVE_RGBA, CROSSTALK_UI_THEME_MARK_RGBA, CROSSTALK_UI_THEME_HOT_RGBA, CROSSTALK_UI_THEME_TEXT_RGBA };
    analyzer_lane_geom_t geom = LDG_STRUCT_ZERO_INIT;
    uint32_t sample_row = 0;
    uint64_t t_hi = 0;
    uint64_t win_lo = 0;
    uint32_t idx = 0;
    uint32_t edge_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (win_id != (uint64_t)CROSSTALK_EDA_UI_ANALYZER_WINDOW_ID || col_cunt == 0 || row_cunt == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->lane_cunt == 0) { return crosstalk_ui_cell_rgba_put(render, (uint32_t)ANALYZER_ORIG_COL, (uint32_t)ANALYZER_HDR_ROW, ANALYZER_HINT_TEXT, (uint64_t)sizeof(ANALYZER_HINT_TEXT), (uint32_t)CROSSTALK_UI_THEME_DEAD_RGBA, 0x0); }

    for (idx = 0; idx < ctx->lane_cunt; idx = idx + (uint32_t)ANALYZER_STEP)
    {
        if (analyzer_lane_pump(ctx, &ctx->lane[idx]) != LDG_ERR_AOK) { continue; }

        for (edge_idx = 0; edge_idx < ctx->lane[idx].cunt; edge_idx = edge_idx + (uint32_t)ANALYZER_STEP)
        {
            uint32_t slot = (ctx->lane[idx].hd + edge_idx) % (uint32_t)CROSSTALK_EDA_UI_ANALYZER_EDGE_CAP;

            if (ctx->lane[idx].edge_cycle[slot] > t_hi) { t_hi = ctx->lane[idx].edge_cycle[slot]; }
        }
    }

    win_lo = t_hi > ctx->span ? t_hi - ctx->span : (uint64_t)ANALYZER_CYCLE_ORIG;

    if (snprintf(text, sizeof(text), ANALYZER_HDR_FMT, ctx->span, t_hi) < 0) { return LDG_ERR_STR_FMT; }

    ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)ANALYZER_ORIG_COL, (uint32_t)ANALYZER_HDR_ROW, text, (uint64_t)sizeof(text), (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, 0x0);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->win.content_row_cunt = (uint32_t)ANALYZER_LANE_ROW_FIRST + ctx->lane_cunt * (uint32_t)ANALYZER_LANE_ROW_CUNT;

    if (col_cunt <= (uint32_t)ANALYZER_LABEL_W) { return LDG_ERR_AOK; }

    sample_row = render->sample_row_cunt;
    geom.sample_col = render->sample_col_cunt;
    geom.x_s = (uint32_t)ANALYZER_LABEL_W * geom.sample_col;
    geom.w_s = (col_cunt - (uint32_t)ANALYZER_LABEL_W) * geom.sample_col;

    for (idx = 0; idx < ctx->lane_cunt; idx = idx + (uint32_t)ANALYZER_STEP)
    {
        geom.row = (uint32_t)ANALYZER_LANE_ROW_FIRST + idx * (uint32_t)ANALYZER_LANE_ROW_CUNT;
        geom.y_hi = geom.row * sample_row + sample_row / (uint32_t)ANALYZER_CELL_MID_DIV;
        geom.y_mid = geom.y_hi + sample_row * (uint32_t)ANALYZER_LANE_MID_ROW;
        geom.y_lo = geom.y_hi + sample_row * (uint32_t)ANALYZER_LANE_LO_ROW;

        ret = crosstalk_ui_cell_line_draw(render, geom.x_s, geom.row * sample_row, geom.x_s + geom.w_s, geom.row * sample_row, (uint32_t)CROSSTALK_UI_THEME_GRID_RGBA);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = analyzer_lane_draw(render, &ctx->lane[idx], lane_rgba[idx], &geom, win_lo, t_hi);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_analyzer_win_reg(crosstalk_eda_ui_analyzer_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot)
{
    if (LDG_UNLIKELY(ctx == 0x0 || win_nav == 0x0 || out_slot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_ui_window_reg(win_nav, (uint64_t)CROSSTALK_EDA_UI_ANALYZER_WINDOW_ID, CROSSTALK_EDA_UI_ANALYZER_WINDOW_TITLE, &ctx->win, analyzer_draw_cb, 0x0, ctx, out_slot);
}
