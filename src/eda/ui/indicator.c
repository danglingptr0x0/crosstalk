#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "eda/ui/indicator.h"
#include "ui/cell.h"

#define INDICATOR_STATUS_FMT "%.31s 0x%" PRIX64 " cycle %" PRIu64
#define INDICATOR_BIT_FMT "%u"
#define INDICATOR_STEP 1
#define INDICATOR_ORIG_COL 0
#define INDICATOR_LAMP_ROW 0
#define INDICATOR_BIT_ROW 1
#define INDICATOR_STATUS_ROW 2
#define INDICATOR_ACC_ROW 4
#define INDICATOR_ACC_LABEL_ROW 5
#define INDICATOR_LIVE_ROW_CUNT 3
#define INDICATOR_ACC_ROW_CUNT 6
#define INDICATOR_LABEL_CAP 16

uint32_t crosstalk_eda_ui_indicator_init(crosstalk_eda_ui_indicator_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->win.scroll_on = LDG_TRUTH_TRUE;
    ctx->win.open = LDG_TRUTH_FALSE;
    ctx->open = LDG_TRUTH_FALSE;
    ctx->stream_handle = UINT32_MAX;
    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_indicator_shutdown(crosstalk_eda_ui_indicator_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_indicator_open_set(crosstalk_eda_ui_indicator_ctx_t *ctx, uint8_t open)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->open = open;
    ctx->win.open = open;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_ui_indicator_open_is(const crosstalk_eda_ui_indicator_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->win.open != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_eda_ui_indicator_bind(crosstalk_eda_ui_indicator_ctx_t *ctx, uint32_t stream_handle, uint32_t width, const char *port)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || port == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (width == 0 || width > (uint32_t)CROSSTALK_EDA_UI_INDICATOR_BITS_MAX) { return CROSSTALK_EDA_UI_INDICATOR_ERR_WIDTH; }

    ctx->stream_handle = stream_handle;
    ctx->width = width;
    ctx->live = LDG_TRUTH_TRUE;

    ret = ldg_str_rbrcpy(ctx->port, port, (uint64_t)sizeof(ctx->port));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_indicator_acc_set(crosstalk_eda_ui_indicator_ctx_t *ctx, const uint64_t *acc, uint32_t cunt)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || acc == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (cunt < (uint32_t)CROSSTALK_EDA_UI_INDICATOR_ACC_CUNT) { return LDG_ERR_BOUNDS; }

    for (idx = 0; idx < (uint32_t)CROSSTALK_EDA_UI_INDICATOR_ACC_CUNT; idx = idx + 0x1)
    {
        ctx->acc_lit[idx] = acc[idx] != ctx->acc_prev[idx] ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
        ctx->acc_prev[idx] = acc[idx];
    }

    ctx->acc_on = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_indicator_acc_clear(crosstalk_eda_ui_indicator_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx->acc_prev, 0x0, sizeof(ctx->acc_prev));
    memset(ctx->acc_lit, 0x0, sizeof(ctx->acc_lit));
    ctx->acc_on = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

static uint32_t indicator_lamp_draw(ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t width, uint32_t rgba)
{
    ldg_term_render_color_t lamp = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_ui_cell_color_get(rgba, &lamp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_ui_cell_span_fill(render, col, row, width, &lamp, &lamp);
}

static LDG_NOINLINE uint32_t indicator_acc_draw(crosstalk_eda_ui_indicator_ctx_t *ctx, ldg_term_render_t *render)
{
    const char *acc_text[CROSSTALK_EDA_UI_INDICATOR_ACC_CUNT] = { CROSSTALK_EDA_UI_INDICATOR_ACC_RAM_RD_TEXT, CROSSTALK_EDA_UI_INDICATOR_ACC_RAM_WR_TEXT, CROSSTALK_EDA_UI_INDICATOR_ACC_MMIO_RD_TEXT, CROSSTALK_EDA_UI_INDICATOR_ACC_MMIO_WR_TEXT };
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)INDICATOR_ORIG_COL, (uint32_t)INDICATOR_ACC_ROW, CROSSTALK_EDA_UI_INDICATOR_ACC_LABEL, (uint64_t)INDICATOR_LABEL_CAP, (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, 0x0);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < (uint32_t)CROSSTALK_EDA_UI_INDICATOR_ACC_CUNT; idx = idx + (uint32_t)INDICATOR_STEP)
    {
        uint32_t lamp_col = (idx + (uint32_t)INDICATOR_STEP) * ((uint32_t)CROSSTALK_EDA_UI_INDICATOR_ACC_LAMP_W + (uint32_t)CROSSTALK_EDA_UI_INDICATOR_LAMP_GAP);
        uint32_t lamp_rgba = ctx->acc_lit[idx] == LDG_TRUTH_TRUE ? (uint32_t)CROSSTALK_EDA_UI_INDICATOR_ACC_ON_COL : (uint32_t)CROSSTALK_EDA_UI_INDICATOR_ACC_OFF_COL;

        ret = indicator_lamp_draw(render, lamp_col, (uint32_t)INDICATOR_ACC_ROW, (uint32_t)CROSSTALK_EDA_UI_INDICATOR_ACC_LAMP_W, lamp_rgba);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_ui_cell_rgba_put(render, lamp_col, (uint32_t)INDICATOR_ACC_LABEL_ROW, acc_text[idx], (uint64_t)INDICATOR_LABEL_CAP, (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, 0x0);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t indicator_idle_draw(ldg_term_render_t *render)
{
    if (LDG_UNLIKELY(render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_ui_cell_rgba_put(render, (uint32_t)INDICATOR_ORIG_COL, (uint32_t)INDICATOR_LAMP_ROW, CROSSTALK_EDA_UI_INDICATOR_IDLE_TEXT, (uint64_t)sizeof(CROSSTALK_EDA_UI_INDICATOR_IDLE_TEXT), (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, 0x0);
}

static uint32_t indicator_draw_cb(uint64_t win_id, void *user, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    crosstalk_eda_ui_indicator_ctx_t *ctx = (crosstalk_eda_ui_indicator_ctx_t *)user;
    char status[CROSSTALK_EDA_UI_INDICATOR_STATUS_CAP] = LDG_ARR_ZERO_INIT;
    char bit_text[CROSSTALK_EDA_UI_INDICATOR_BIT_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t val = 0;
    uint64_t cycle = 0;
    uint32_t gap_cells = 0;
    uint32_t lamp_w = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE || ctx->streams == 0x0) { return LDG_ERR_NOT_INIT; }

    if (win_id != (uint64_t)CROSSTALK_EDA_UI_INDICATOR_WINDOW_ID || col_cunt == 0 || row_cunt == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->live != LDG_TRUTH_TRUE) { return indicator_idle_draw(render); }

    ret = crosstalk_eda_logic_stream_level_get(ctx->streams, ctx->stream_handle, &val, &cycle);
    if (ret != LDG_ERR_AOK)
    {
        ctx->live = LDG_TRUTH_FALSE;

        return indicator_idle_draw(render);
    }

    gap_cells = (uint32_t)CROSSTALK_EDA_UI_INDICATOR_LAMP_GAP * (ctx->width - (uint32_t)CROSSTALK_EDA_UI_INDICATOR_BIT_STEP);
    if (col_cunt <= gap_cells) { return LDG_ERR_AOK; }

    lamp_w = (col_cunt - gap_cells) / ctx->width;
    if (lamp_w == 0) { return LDG_ERR_AOK; }

    for (idx = 0; idx < ctx->width; idx = idx + (uint32_t)INDICATOR_STEP)
    {
        uint32_t bit = ctx->width - (uint32_t)CROSSTALK_EDA_UI_INDICATOR_BIT_STEP - idx;
        uint64_t on = (val >> bit) & (uint64_t)CROSSTALK_EDA_UI_INDICATOR_BIT_STEP;
        uint32_t lamp_col = idx * (lamp_w + (uint32_t)CROSSTALK_EDA_UI_INDICATOR_LAMP_GAP);

        ret = indicator_lamp_draw(render, lamp_col, (uint32_t)INDICATOR_LAMP_ROW, lamp_w, on != 0 ? (uint32_t)CROSSTALK_UI_THEME_LIVE_RGBA : (uint32_t)CROSSTALK_EDA_UI_INDICATOR_LAMP_OFF_COL);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (snprintf(bit_text, sizeof(bit_text), INDICATOR_BIT_FMT, bit) < 0) { return LDG_ERR_STR_FMT; }

        ret = crosstalk_ui_cell_rgba_put(render, lamp_col, (uint32_t)INDICATOR_BIT_ROW, bit_text, (uint64_t)sizeof(bit_text), (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, 0x0);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (snprintf(status, sizeof(status), INDICATOR_STATUS_FMT, ctx->port, val, cycle) < 0) { return LDG_ERR_STR_FMT; }

    ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)INDICATOR_ORIG_COL, (uint32_t)INDICATOR_STATUS_ROW, status, (uint64_t)sizeof(status), (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, 0x0);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->win.content_row_cunt = (uint32_t)INDICATOR_LIVE_ROW_CUNT;

    if (ctx->acc_on != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = indicator_acc_draw(ctx, render);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->win.content_row_cunt = (uint32_t)INDICATOR_ACC_ROW_CUNT;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_indicator_win_reg(crosstalk_eda_ui_indicator_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, const crosstalk_eda_logic_stream_ctx_t *streams, uint32_t *out_slot)
{
    if (LDG_UNLIKELY(ctx == 0x0 || win_nav == 0x0 || streams == 0x0 || out_slot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->streams = streams;

    return crosstalk_ui_window_reg(win_nav, (uint64_t)CROSSTALK_EDA_UI_INDICATOR_WINDOW_ID, CROSSTALK_EDA_UI_INDICATOR_WINDOW_TITLE, &ctx->win, indicator_draw_cb, 0x0, ctx, out_slot);
}
