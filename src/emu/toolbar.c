#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "emu/toolbar.h"

#define TOOLBAR_STEP 1
#define TOOLBAR_MHZ_MIN 1.0
#define TOOLBAR_MHZ_MAX 2000.0
#define TOOLBAR_MHZ_STEP_FINE 1.0
#define TOOLBAR_MHZ_STEP_COARSE 100.0
#define TOOLBAR_HZ_PER_MHZ 1000000.0
#define TOOLBAR_HZ_FMT "%.0f MHz"
#define TOOLBAR_CONF_TEXT "F2 Config"
#define TOOLBAR_CLOSE_TEXT "F10 Close"
#define TOOLBAR_CLOCK_TEXT "Clock "
#define TOOLBAR_BLANK_CAP 32
#define TOOLBAR_BLANK_CH ' '
#define TOOLBAR_ORIG_COL 0
#define TOOLBAR_BAR_FG_LEVEL 0xFF
#define TOOLBAR_BAR_BG_LEVEL 0x40
#define TOOLBAR_PANEL_BG_LEVEL 0x20

static uint32_t toolbar_color_set(ldg_term_render_color_t *out, uint8_t level)
{
    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->red = level;
    out->green = level;
    out->blue = level;
    out->pudding[0] = 0;

    return LDG_ERR_AOK;
}

static uint32_t toolbar_row_fill(ldg_term_render_t *render, uint32_t row, uint32_t col_cunt, const ldg_term_render_color_t *fg, const ldg_term_render_color_t *bg)
{
    char blank[TOOLBAR_BLANK_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t col = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || fg == 0x0 || bg == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(&blank[0], TOOLBAR_BLANK_CH, sizeof(blank));

    for (col = 0; col < col_cunt; col = col + (uint32_t)TOOLBAR_BLANK_CAP)
    {
        uint32_t len = col_cunt - col < (uint32_t)TOOLBAR_BLANK_CAP ? col_cunt - col : (uint32_t)TOOLBAR_BLANK_CAP;

        ret = ldg_term_render_text_draw(render, col, row, &blank[0], (uint64_t)len, fg, bg);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t toolbar_hz_text_set(crosstalk_emu_toolbar_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (snprintf(ctx->hz_text, sizeof(ctx->hz_text), TOOLBAR_HZ_FMT, ctx->hz_mhz) < 0) { return LDG_ERR_STR_FMT; }

    return LDG_ERR_AOK;
}

static uint32_t toolbar_hz_step(crosstalk_emu_toolbar_ctx_t *ctx, crosstalk_emu_ctx_t *emu, double delta)
{
    double want = 0.0;
    uint64_t now_hz = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || emu == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    want = ctx->hz_mhz + delta;

    if (want < TOOLBAR_MHZ_MIN) { want = TOOLBAR_MHZ_MIN; }

    if (want > TOOLBAR_MHZ_MAX) { want = TOOLBAR_MHZ_MAX; }

    now_hz = crosstalk_emu_hz_get(emu);
    if (now_hz == UINT64_MAX) { return LDG_ERR_NOT_INIT; }

    if (crosstalk_emu_hz_set(emu, (uint64_t)(want * TOOLBAR_HZ_PER_MHZ)) == LDG_ERR_AOK) { ctx->hz_mhz = want; }
    else { ctx->hz_mhz = (double)now_hz / TOOLBAR_HZ_PER_MHZ; }

    return toolbar_hz_text_set(ctx);
}

uint32_t crosstalk_emu_toolbar_init(crosstalk_emu_toolbar_ctx_t *ctx, const crosstalk_emu_ctx_t *emu)
{
    uint64_t hz = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || emu == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->open = LDG_TRUTH_FALSE;
    ctx->close_req = LDG_TRUTH_FALSE;

    hz = crosstalk_emu_hz_get(emu);
    if (hz == UINT64_MAX) { return LDG_ERR_NOT_INIT; }

    ctx->hz_mhz = (double)hz / TOOLBAR_HZ_PER_MHZ;

    return toolbar_hz_text_set(ctx);
}

uint32_t crosstalk_emu_toolbar_key_isr(crosstalk_emu_toolbar_ctx_t *ctx, crosstalk_emu_ctx_t *emu, const ldg_term_key_event_t *key, uint8_t *out_consumed)
{
    if (LDG_UNLIKELY(ctx == 0x0 || emu == 0x0 || key == 0x0 || out_consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_consumed = LDG_TRUTH_FALSE;

    if (key->key == (uint8_t)LDG_TERM_KEY_F10)
    {
        *out_consumed = LDG_TRUTH_TRUE;

        if (key->kind == (uint8_t)LDG_TERM_KEY_KIND_PRESS) { ctx->close_req = LDG_TRUTH_TRUE; }

        return LDG_ERR_AOK;
    }

    if (key->key == (uint8_t)LDG_TERM_KEY_F2)
    {
        *out_consumed = LDG_TRUTH_TRUE;

        if (key->kind == (uint8_t)LDG_TERM_KEY_KIND_PRESS) { ctx->open = ctx->open == LDG_TRUTH_TRUE ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE; }

        return LDG_ERR_AOK;
    }

    if (ctx->open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    *out_consumed = LDG_TRUTH_TRUE;

    if (key->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE) { return LDG_ERR_AOK; }

    if (key->key == (uint8_t)LDG_TERM_KEY_ESCAPE) { ctx->open = LDG_TRUTH_FALSE; return LDG_ERR_AOK; }

    if (key->key == (uint8_t)LDG_TERM_KEY_UP || key->key == (uint8_t)LDG_TERM_KEY_RHS) { return toolbar_hz_step(ctx, emu, TOOLBAR_MHZ_STEP_FINE); }

    if (key->key == (uint8_t)LDG_TERM_KEY_DOWN || key->key == (uint8_t)LDG_TERM_KEY_LHS) { return toolbar_hz_step(ctx, emu, -TOOLBAR_MHZ_STEP_FINE); }

    if (key->key == (uint8_t)LDG_TERM_KEY_PAGE_UP) { return toolbar_hz_step(ctx, emu, TOOLBAR_MHZ_STEP_COARSE); }

    if (key->key == (uint8_t)LDG_TERM_KEY_PAGE_DOWN) { return toolbar_hz_step(ctx, emu, -TOOLBAR_MHZ_STEP_COARSE); }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_toolbar_draw(const crosstalk_emu_toolbar_ctx_t *ctx, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t bar_bg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t panel_bg = LDG_STRUCT_ZERO_INIT;
    uint64_t hz_len = 0;
    uint32_t close_len = (uint32_t)(sizeof(TOOLBAR_CLOSE_TEXT) - LDG_STR_TERM_SIZE);
    uint32_t clock_len = (uint32_t)(sizeof(TOOLBAR_CLOCK_TEXT) - LDG_STR_TERM_SIZE);
    uint32_t bar_row = 0;
    uint32_t panel_row = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (col_cunt == 0 || row_cunt < (uint32_t)CROSSTALK_EMU_TOOLBAR_ROW_CUNT) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = toolbar_color_set(&fg, (uint8_t)TOOLBAR_BAR_FG_LEVEL);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = toolbar_color_set(&bar_bg, (uint8_t)TOOLBAR_BAR_BG_LEVEL);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = toolbar_color_set(&panel_bg, (uint8_t)TOOLBAR_PANEL_BG_LEVEL);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    bar_row = row_cunt - (uint32_t)CROSSTALK_EMU_TOOLBAR_ROW_CUNT;

    ret = toolbar_row_fill(render, bar_row, col_cunt, &fg, &bar_bg);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_term_render_text_draw(render, (uint32_t)TOOLBAR_ORIG_COL, bar_row, TOOLBAR_CONF_TEXT, (uint64_t)(sizeof(TOOLBAR_CONF_TEXT) - LDG_STR_TERM_SIZE), &fg, &bar_bg);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (col_cunt > close_len)
    {
        ret = ldg_term_render_text_draw(render, col_cunt - close_len, bar_row, TOOLBAR_CLOSE_TEXT, (uint64_t)close_len, &fg, &bar_bg);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (ctx->open != LDG_TRUTH_TRUE || bar_row < (uint32_t)TOOLBAR_STEP) { return LDG_ERR_AOK; }

    panel_row = bar_row - (uint32_t)TOOLBAR_STEP;

    ret = toolbar_row_fill(render, panel_row, col_cunt, &fg, &panel_bg);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_term_render_text_draw(render, (uint32_t)TOOLBAR_ORIG_COL, panel_row, TOOLBAR_CLOCK_TEXT, (uint64_t)clock_len, &fg, &panel_bg);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_str_len_get(ctx->hz_text, (uint64_t)sizeof(ctx->hz_text), &hz_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_term_render_text_draw(render, clock_len, panel_row, ctx->hz_text, hz_len, &fg, &panel_bg);
}

uint8_t crosstalk_emu_toolbar_close_req_is(const crosstalk_emu_toolbar_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->close_req;
}
