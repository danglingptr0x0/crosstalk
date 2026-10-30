#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include <dangling/logging/logging.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/core/bits.h>
#include <dangling/str/str.h>
#include <dangling/term/fx.h>
#include <dangling/time/time.h>

#include "core/log.h"
#include "eda/ui/display/crt.h"
#include "ui/cell.h"
#include "ui/cga.h"
#include "ui/theme.h"

#define CRT_STEP 1
#define CRT_ORIG 0
#define CRT_CENTER_DIV 2
#define CRT_TITLE "Monitor"
#define CRT_IDLE_TEXT "no signal"
#define CRT_TEXT_CELL_BYTES 2
#define CRT_TEXT_RUN_CAP 80
#define CRT_STATE_BYTES (sizeof(uint32_t) * 4 + sizeof(uint64_t) * 2 + CROSSTALK_EDA_UI_DISPLAY_CRT_SIG_CAP)
#define CRT_TEXT_ATTR_BG_SHIFT 4
#define CRT_TEXT_COL_SHIFT_R 24
#define CRT_TEXT_COL_SHIFT_G 16
#define CRT_TEXT_COL_SHIFT_B 8
#define CRT_TEXT_PAL_CUNT 16
#define CRT_TEXT_PAL_MASK 0xF
#define CRT_GFX_LANE_G 1
#define CRT_GFX_LANE_R 2
#define CRT_GFX_WORD_WIDTH 32
#define CRT_GFX_WORD_MASK 0xFFFFFFFFU
#define CRT_GFX_CH_MASK 0xFFU
#define CRT_GFX_CH_BITS 8
#define CRT_GFX_R_SHIFT 16
#define CRT_CHAR_COL_CUNT 8
#define CRT_CHAR_ROW_CUNT 16

static uint32_t crt_text_col_get(uint8_t attr)
{
    const uint32_t pal[CRT_TEXT_PAL_CUNT] = {
        CROSSTALK_UI_CGA_PAL_0, CROSSTALK_UI_CGA_PAL_1, CROSSTALK_UI_CGA_PAL_2, CROSSTALK_UI_CGA_PAL_3, CROSSTALK_UI_CGA_PAL_4, CROSSTALK_UI_CGA_PAL_5, CROSSTALK_UI_CGA_PAL_6, CROSSTALK_UI_CGA_PAL_7, CROSSTALK_UI_CGA_PAL_8, CROSSTALK_UI_CGA_PAL_9, CROSSTALK_UI_CGA_PAL_A, CROSSTALK_UI_CGA_PAL_B, CROSSTALK_UI_CGA_PAL_C, CROSSTALK_UI_CGA_PAL_D, CROSSTALK_UI_CGA_PAL_E, CROSSTALK_UI_CGA_PAL_F
    };

    return pal[attr & (uint8_t)CRT_TEXT_PAL_MASK];
}

typedef struct crt_fit
{
    uint32_t pos_x;
    uint32_t pos_y;
    uint32_t extent_w;
    uint32_t extent_h;
} crt_fit_t;

static uint32_t crt_fit_get(const ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt, uint32_t src_w, uint32_t src_h, crt_fit_t *out)
{
    uint64_t unit_w = 0;
    uint64_t unit_h = 0;
    uint64_t area_w = 0;
    uint64_t area_h = 0;
    uint64_t fit_w = 0;
    uint64_t fit_h = 0;

    if (LDG_UNLIKELY(render == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(out, 0x0, sizeof(*out));

    unit_w = (uint64_t)render->cell_aspect_num * (uint64_t)render->sample_row_cunt;
    unit_h = (uint64_t)render->cell_aspect_den * (uint64_t)render->sample_col_cunt;

    if (unit_w == 0 || unit_h == 0 || src_w == 0 || src_h == 0) { return LDG_ERR_NOT_INIT; }

    area_w = (uint64_t)col_cunt * (uint64_t)render->sample_col_cunt * unit_w;
    area_h = (uint64_t)row_cunt * (uint64_t)render->sample_row_cunt * unit_h;
    fit_w = area_w;
    fit_h = fit_w * (uint64_t)src_h / (uint64_t)src_w;

    if (fit_h > area_h)
    {
        fit_h = area_h;
        fit_w = fit_h * (uint64_t)src_w / (uint64_t)src_h;
    }

    out->pos_x = (uint32_t)((area_w - fit_w) / (uint64_t)CRT_CENTER_DIV / unit_w);
    out->pos_y = (uint32_t)((area_h - fit_h) / (uint64_t)CRT_CENTER_DIV / unit_h);
    out->extent_w = (uint32_t)(fit_w / unit_w);
    out->extent_h = (uint32_t)(fit_h / unit_h);

    return LDG_ERR_AOK;
}

static uint32_t crt_run_fill(ldg_term_render_t *render, const crt_fit_t *fit, uint32_t src_w, uint32_t src_h, uint32_t row, uint32_t run_beg, uint32_t run_end, uint32_t rgba)
{
    uint32_t lhs = 0;
    uint32_t rhs = 0;
    uint32_t top = 0;
    uint32_t bottom = 0;

    if (LDG_UNLIKELY(render == 0x0 || fit == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    lhs = (uint32_t)((uint64_t)run_beg * (uint64_t)fit->extent_w / (uint64_t)src_w);
    rhs = (uint32_t)((uint64_t)run_end * (uint64_t)fit->extent_w / (uint64_t)src_w);
    top = (uint32_t)((uint64_t)row * (uint64_t)fit->extent_h / (uint64_t)src_h);
    bottom = (uint32_t)((uint64_t)(row + (uint32_t)CRT_STEP) * (uint64_t)fit->extent_h / (uint64_t)src_h);

    if (rhs == lhs || bottom == top) { return LDG_ERR_AOK; }

    return crosstalk_ui_cell_sample_fill(render, fit->pos_x + lhs, fit->pos_y + top, rhs - lhs, bottom - top, rgba);
}

static uint32_t crt_raster_draw(ldg_term_render_t *render, const uint8_t *px, uint32_t src_w, uint32_t src_h, uint32_t col_cunt, uint32_t row_cunt)
{
    crt_fit_t fit = LDG_STRUCT_ZERO_INIT;
    uint32_t row = 0;
    uint32_t col = 0;
    uint32_t run_beg = 0;
    uint32_t ret = 0;
    uint8_t lit = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(render == 0x0 || px == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crt_fit_get(render, col_cunt, row_cunt, src_w, src_h, &fit);
    if (ret != LDG_ERR_AOK) { return ret; }

    for (row = 0; row < src_h; row = row + (uint32_t)CRT_STEP)
    {
        lit = LDG_TRUTH_FALSE;

        for (col = 0; col <= src_w; col = col + (uint32_t)CRT_STEP)
        {
            uint8_t on = col < src_w && px[row * CROSSTALK_EDA_IO_DISPLAY_W_MAX + col] != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

            if (on == LDG_TRUTH_TRUE)
            {
                if (lit != LDG_TRUTH_TRUE) { run_beg = col; lit = LDG_TRUTH_TRUE; }

                continue;
            }

            if (lit != LDG_TRUTH_TRUE) { continue; }

            ret = crt_run_fill(render, &fit, src_w, src_h, row, run_beg, col, (uint32_t)CROSSTALK_UI_THEME_WHITE_RGBA);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            lit = LDG_TRUTH_FALSE;
        }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_display_crt_gamma_set(crosstalk_eda_ui_display_crt_ctx_t *ctx, double gamma)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (gamma < CROSSTALK_EDA_UI_DISPLAY_CRT_GAMMA_MIN || gamma > CROSSTALK_EDA_UI_DISPLAY_CRT_GAMMA_MAX) { return LDG_ERR_RANGE; }

    ctx->gamma = gamma;

    for (at = 0; at <= (uint32_t)LDG_BYTE_MASK; at = at + (uint32_t)CRT_STEP)
    {
        double lit = pow((double)at / (double)LDG_BYTE_MASK, CROSSTALK_EDA_UI_DISPLAY_CRT_GAMMA_UNIT / gamma) * (double)LDG_BYTE_MASK;

        ctx->gamma_lut[at] = (uint8_t)(lit > (double)LDG_BYTE_MASK ? (double)LDG_BYTE_MASK : lit);
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crt: gamma set; gamma: %.2f", gamma);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_display_crt_init(crosstalk_eda_ui_display_crt_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->kind = (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_KIND_CRT;

    if (crosstalk_eda_ui_display_crt_gamma_set(ctx, CROSSTALK_EDA_UI_DISPLAY_CRT_GAMMA_DEFAULT) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crt gamma seed failed"); }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_display_crt_open_set(crosstalk_eda_ui_display_crt_ctx_t *ctx, uint8_t open)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->open = open != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    ctx->win.open = ctx->open;

    return LDG_ERR_AOK;
}

static uint64_t crt_mem_byte_width_get(const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, const char *sig_name)
{
    uint64_t width = 0;
    uint32_t elem_width = 0;
    uint32_t elem_bytes = 0;

    if (crosstalk_eda_ic_fpga_mem_width_get(fpga, slot_idx, sig_name, &width, &elem_width) != LDG_ERR_AOK) { return UINT64_MAX; }

    elem_bytes = elem_width / (uint32_t)LDG_BYTE_BITS;
    if (elem_bytes == 0) { elem_bytes = (uint32_t)CRT_STEP; }

    return width * (uint64_t)elem_bytes;
}

uint32_t crosstalk_eda_ui_display_crt_text_bind(crosstalk_eda_ui_display_crt_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, const char *sig_name, uint64_t base)
{
    uint64_t width = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || fpga == 0x0 || sig_name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    width = crt_mem_byte_width_get(fpga, slot_idx, sig_name);
    if (width == UINT64_MAX) { return CROSSTALK_EDA_UI_DISPLAY_CRT_ERR_BIND; }

    if (base + (uint64_t)CROSSTALK_EDA_UI_DISPLAY_CRT_TEXT_BYTE_CUNT > width) { return CROSSTALK_EDA_UI_DISPLAY_CRT_ERR_BIND; }

    ret = ldg_str_rbrcpy(ctx->text_sig, sig_name, (uint64_t)sizeof(ctx->text_sig));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    ctx->text_base = base;
    ctx->text_next = 0.0;
    ctx->text_native = LDG_TRUTH_FALSE;
    ctx->cell_cunt = 0;
    ctx->src = (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_SRC_TEXT;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_display_crt_text_tick(crosstalk_eda_ui_display_crt_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, double now)
{
    if (LDG_UNLIKELY(ctx == 0x0 || fpga == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (ctx->src != (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_SRC_TEXT || ctx->text_sig[0] == LDG_STR_TERM || ctx->text_native == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (now < ctx->text_next) { return LDG_ERR_AOK; }

    ctx->text_next = now + CROSSTALK_EDA_UI_DISPLAY_CRT_TEXT_REFRESH_SECS;

    return crosstalk_eda_ic_fpga_mem_rd(fpga, slot_idx, ctx->text_sig, ctx->text_base, ctx->cell, (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_TEXT_BYTE_CUNT, &ctx->cell_cunt);
}

static uint32_t crt_text_run_draw(ldg_term_render_t *render, uint32_t col, uint32_t row, const char *run, uint32_t run_len, uint8_t attr)
{
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t bg = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || run == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_ui_cell_color_get(crt_text_col_get(attr), &fg);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get(crt_text_col_get((uint8_t)(attr >> (uint8_t)CRT_TEXT_ATTR_BG_SHIFT)), &bg); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_term_render_text_draw(render, col, row, run, (uint64_t)run_len, &fg, &bg);
}

static uint32_t crt_text_draw(crosstalk_eda_ui_display_crt_ctx_t *ctx, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    char run[CRT_TEXT_RUN_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t orig_col = CRT_ORIG;
    uint32_t orig_row = CRT_ORIG;
    uint32_t shown_col = (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_TEXT_COL_CUNT;
    uint32_t shown_row = (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_TEXT_ROW_CUNT;
    uint32_t row = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (col_cunt > shown_col) { orig_col = (col_cunt - shown_col) / (uint32_t)CRT_CENTER_DIV; }
    else { shown_col = col_cunt; }

    if (row_cunt > shown_row) { orig_row = (row_cunt - shown_row) / (uint32_t)CRT_CENTER_DIV; }
    else { shown_row = row_cunt; }

    for (row = 0; row < shown_row; row = row + (uint32_t)CRT_STEP)
    {
        uint32_t col = 0;
        uint32_t run_beg = 0;
        uint32_t run_len = 0;
        uint8_t run_attr = 0;

        for (col = 0; col < shown_col; col = col + (uint32_t)CRT_STEP)
        {
            uint32_t at = (row * (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_TEXT_COL_CUNT + col) * (uint32_t)CRT_TEXT_CELL_BYTES;
            uint8_t ch = 0;
            uint8_t attr = 0;

            if (at + (uint32_t)CRT_STEP >= ctx->cell_cunt) { break; }

            ch = (uint8_t)(ctx->cell[at] & (uint64_t)LDG_BYTE_MASK);
            attr = (uint8_t)(ctx->cell[at + (uint32_t)CRT_STEP] & (uint64_t)LDG_BYTE_MASK);

            if (ch < (uint8_t)LDG_ASCII_PRINTABLE_MIN || ch > (uint8_t)LDG_ASCII_PRINTABLE_MAX) { ch = (uint8_t)' '; }

            if (run_len != 0 && attr != run_attr)
            {
                ret = crt_text_run_draw(render, orig_col + run_beg, orig_row + row, &run[0], run_len, run_attr);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                run_len = 0;
            }

            if (run_len == 0) { run_beg = col; run_attr = attr; }

            run[run_len] = (char)ch;
            run_len = run_len + (uint32_t)CRT_STEP;
        }

        if (run_len == 0) { continue; }

        ret = crt_text_run_draw(render, orig_col + run_beg, orig_row + row, &run[0], run_len, run_attr);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_display_crt_gfx_bind(crosstalk_eda_ui_display_crt_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, const char *sig_name, uint64_t base, uint32_t gfx_w, uint32_t gfx_h)
{
    uint64_t width = 0;
    uint64_t span = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || fpga == 0x0 || sig_name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (gfx_w == 0 || gfx_w > (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_W_MAX || gfx_h == 0 || gfx_h > (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_H_MAX) { return LDG_ERR_FUNC_ARG_INVALID; }

    width = crt_mem_byte_width_get(fpga, slot_idx, sig_name);
    if (width == UINT64_MAX) { return CROSSTALK_EDA_UI_DISPLAY_CRT_ERR_BIND; }

    span = (uint64_t)gfx_w * (uint64_t)gfx_h * (uint64_t)CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_BYTES_PER_PX;

    if (base + span > width) { return CROSSTALK_EDA_UI_DISPLAY_CRT_ERR_BIND; }

    ret = ldg_str_rbrcpy(ctx->text_sig, sig_name, (uint64_t)sizeof(ctx->text_sig));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    ctx->gfx_base = base;
    ctx->gfx_w = gfx_w;
    ctx->gfx_h = gfx_h;
    ctx->gfx_next = 0.0;
    ctx->gfx_ok = LDG_TRUTH_FALSE;
    ctx->gfx_geom_ok = LDG_TRUTH_TRUE;
    ctx->src = (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_SRC_GFX;

    return LDG_ERR_AOK;
}

typedef struct crt_fb
{
    uint64_t addr;
    uint64_t fmt;
    uint64_t width;
    uint64_t height;
    uint64_t frame;
} crt_fb_t;

static uint32_t crt_fb_active_get(const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, crt_fb_t *out)
{
    uint64_t *fb = 0x0;
    uint32_t fb_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(fpga == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->addr = 0;
    out->fmt = 0;
    out->width = 0;
    out->height = 0;
    out->frame = 0;

    ret = crosstalk_eda_ic_fpga_prof_get(fpga, slot_idx, (uint32_t)CROSSTALK_HDL_ABI_STATE_EMU_FB, &fb, &fb_cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (fb == 0x0 || fb_cunt < (uint32_t)CROSSTALK_HDL_ABI_FB_CUNT) { return LDG_ERR_UNSUPPORTED; }

    out->addr = fb[CROSSTALK_HDL_ABI_FB_ADDR_AT];
    out->fmt = fb[CROSSTALK_HDL_ABI_FB_FMT_AT];
    out->width = fb[CROSSTALK_HDL_ABI_FB_MODE_AT] & (uint64_t)CROSSTALK_HDL_ABI_FB_MODE_W_MASK;
    out->height = fb[CROSSTALK_HDL_ABI_FB_MODE_AT] >> (uint64_t)CROSSTALK_HDL_ABI_FB_MODE_H_SHIFT;
    out->frame = fb[CROSSTALK_HDL_ABI_FB_FRAME_AT];

    return LDG_ERR_AOK;
}

static uint8_t crt_fb_drawable_is(const crt_fb_t *fb)
{
    if (LDG_UNLIKELY(fb == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (fb->fmt != (uint64_t)CROSSTALK_HDL_ABI_FB_FMT_XRGB8888 && fb->fmt != (uint64_t)CROSSTALK_HDL_ABI_FB_FMT_TEXT_RASTER) { return LDG_TRUTH_FALSE; }

    if (fb->width == 0 || fb->width > (uint64_t)CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_W_MAX) { return LDG_TRUTH_FALSE; }

    if (fb->height == 0 || fb->height > (uint64_t)CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_H_MAX) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

#define CRT_NATIVE_GREEN_OFF 1
#define CRT_NATIVE_BLUE_OFF 2
#define CRT_NATIVE_GFX_ACTIVE_H 400
#define CRT_NATIVE_BORDER_CUNT 2

static uint32_t crt_native_text_get(crosstalk_eda_ui_display_crt_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx)
{
    uint8_t text[CROSSTALK_EDA_UI_DISPLAY_CRT_TEXT_BYTE_CUNT] = LDG_ARR_ZERO_INIT;
    uint32_t cols = 0x0;
    uint32_t rows = 0x0;
    uint32_t at = 0x0;
    uint32_t ret = 0x0;

    ret = crosstalk_eda_ic_fpga_ddu_text_get(fpga, slot_idx, text, (uint64_t)sizeof(text), &cols, &rows);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (cols != (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_TEXT_COL_CUNT || rows != (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_TEXT_ROW_CUNT) { return LDG_ERR_BOUNDS; }

    for (at = 0x0; at < (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_TEXT_BYTE_CUNT; at++) { ctx->cell[at] = (uint64_t)text[at]; }

    ctx->cell_cunt = (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_TEXT_BYTE_CUNT;
    ctx->text_native = LDG_TRUTH_TRUE;
    ctx->gfx_text = LDG_TRUTH_FALSE;
    ctx->gfx_ok = LDG_TRUTH_FALSE;
    ctx->src = CROSSTALK_EDA_UI_DISPLAY_CRT_SRC_TEXT;

    return LDG_ERR_AOK;
}

static uint32_t crt_native_frame_get(crosstalk_eda_ui_display_crt_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx)
{
    uint8_t *rgba = (uint8_t *)(void *)ctx->gfx_px;
    uint32_t width = 0x0;
    uint32_t height = 0x0;
    uint64_t ctrl = 0x0;
    uint32_t ctrl_width = 0x0;
    uint32_t top = 0x0;
    uint32_t idx = 0x0;
    uint32_t at = 0x0;
    uint32_t ret = 0x0;

    if (crosstalk_eda_ic_fpga_sig_peek(fpga, slot_idx, CROSSTALK_EDA_UI_DISPLAY_CRT_DDU_CTRL_SIG, &ctrl, &ctrl_width) == LDG_ERR_AOK && (ctrl & CROSSTALK_EDA_UI_DISPLAY_CRT_CTRL_GFX_BIT) == 0x0)
    {
        ret = crt_native_text_get(ctx, fpga, slot_idx);
        if (ret != LDG_ERR_UNSUPPORTED) { return ret; }
    }

    ret = crosstalk_eda_ic_fpga_ddu_frame_get(fpga, slot_idx, rgba, sizeof(ctx->gfx_px), &width, &height);
    if (ret == LDG_ERR_UNSUPPORTED)
    {
        ret = crt_native_text_get(ctx, fpga, slot_idx);
        if (ret == LDG_ERR_AOK) { return ret; }

        return LDG_ERR_UNSUPPORTED;
    }

    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->text_native = LDG_TRUTH_FALSE;
    if (LDG_UNLIKELY(width == 0x0 || height == 0x0 || width > CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_W_MAX || height > CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_H_MAX)) { ctx->gfx_ok = LDG_TRUTH_FALSE; return LDG_ERR_BOUNDS; }

    ret = crosstalk_eda_ic_fpga_sig_peek(fpga, slot_idx, CROSSTALK_EDA_UI_DISPLAY_CRT_DDU_CTRL_SIG, &ctrl, &ctrl_width);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->gfx_ok = LDG_TRUTH_FALSE; return ret; }

    ctx->gfx_crop = (ctrl & CROSSTALK_EDA_UI_DISPLAY_CRT_CTRL_GFX_BIT) != 0x0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    ctx->gfx_text = ctx->gfx_crop == LDG_TRUTH_TRUE ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE;
    if (ctx->gfx_crop == LDG_TRUTH_TRUE)
    {
        if (LDG_UNLIKELY(height < CRT_NATIVE_GFX_ACTIVE_H)) { ctx->gfx_ok = LDG_TRUTH_FALSE; return LDG_ERR_BOUNDS; }

        top = (height - CRT_NATIVE_GFX_ACTIVE_H) / CRT_NATIVE_BORDER_CUNT;
        height = CRT_NATIVE_GFX_ACTIVE_H;
    }

    for (idx = 0x0; idx < width * height; idx++)
    {
        at = (top * width + idx) * CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_BYTES_PER_PX;
        ctx->gfx_px[idx] = ((uint32_t)ctx->gamma_lut[rgba[at]] << CRT_TEXT_COL_SHIFT_R) | ((uint32_t)ctx->gamma_lut[rgba[at + CRT_NATIVE_GREEN_OFF]] << CRT_TEXT_COL_SHIFT_G) | ((uint32_t)ctx->gamma_lut[rgba[at + CRT_NATIVE_BLUE_OFF]] << CRT_TEXT_COL_SHIFT_B) | (uint32_t)LDG_BYTE_MASK;
    }

    ctx->gfx_w = width;
    ctx->gfx_h = height;
    ctx->gfx_geom_ok = LDG_TRUTH_TRUE;
    ctx->gfx_ok = LDG_TRUTH_TRUE;
    ctx->src = CROSSTALK_EDA_UI_DISPLAY_CRT_SRC_GFX;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_display_crt_gfx_dev_bind(crosstalk_eda_ui_display_crt_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, const char *sig_name)
{
    uint64_t fmt = 0;
    uint64_t mode = 0;
    uint64_t base = 0;
    uint32_t elem_width = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || fpga == 0x0 || sig_name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crt_native_frame_get(ctx, fpga, slot_idx);
    if (ret == LDG_ERR_AOK)
    {
        ret = ldg_str_rbrcpy(ctx->text_sig, sig_name, sizeof(ctx->text_sig));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

        ctx->gfx_next = 0.0;
        return LDG_ERR_AOK;
    }

    if (ret != LDG_ERR_UNSUPPORTED && ret != LDG_ERR_NOT_INIT) { return ret; }

    {
        crt_fb_t active = LDG_STRUCT_ZERO_INIT;

        if (crt_fb_active_get(fpga, slot_idx, &active) == LDG_ERR_AOK && crt_fb_drawable_is(&active) == LDG_TRUTH_TRUE)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crt: the display describes itself; addr: 0x%X64; width: %u64; height: %u64; frame: %u64", active.addr, active.width, active.height, active.frame);

            return crosstalk_eda_ui_display_crt_gfx_bind(ctx, fpga, slot_idx, sig_name, active.addr / (uint64_t)CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_BYTES_PER_PX, (uint32_t)active.width, (uint32_t)active.height);
        }
    }

    ret = crosstalk_eda_ic_fpga_sig_peek(fpga, slot_idx, CROSSTALK_EDA_UI_DISPLAY_CRT_DDU_FMT_SIG, &fmt, &elem_width);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (fmt == (uint64_t)CROSSTALK_HDL_ABI_FB_FMT_NONE)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crt: the display has no framebuffer configured; fmt: %u64", fmt);

        return CROSSTALK_EDA_UI_DISPLAY_CRT_ERR_BIND;
    }

    if (fmt != (uint64_t)CROSSTALK_HDL_ABI_FB_FMT_XRGB8888)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crt: unknown framebuffer format; fmt: %u64", fmt);

        return CROSSTALK_EDA_UI_DISPLAY_CRT_ERR_BIND;
    }

    ret = crosstalk_eda_ic_fpga_sig_peek(fpga, slot_idx, CROSSTALK_EDA_UI_DISPLAY_CRT_DDU_MODE_SIG, &mode, &elem_width);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_eda_ic_fpga_sig_peek(fpga, slot_idx, CROSSTALK_EDA_UI_DISPLAY_CRT_DDU_BASE_SIG, &base, &elem_width);
    if (ret != LDG_ERR_AOK) { return ret; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crt: the display describes itself; fmt: %u64; w: %u64; h: %u64; base: 0x%X64", fmt, mode & (uint64_t)CROSSTALK_HDL_ABI_FB_MODE_W_MASK, mode >> (uint64_t)CROSSTALK_HDL_ABI_FB_MODE_H_SHIFT, base);

    return crosstalk_eda_ui_display_crt_gfx_bind(ctx, fpga, slot_idx, sig_name, base / (uint64_t)CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_BYTES_PER_PX, (uint32_t)(mode & (uint64_t)CROSSTALK_HDL_ABI_FB_MODE_W_MASK), (uint32_t)(mode >> (uint64_t)CROSSTALK_HDL_ABI_FB_MODE_H_SHIFT));
}

uint32_t crosstalk_eda_ui_display_crt_gfx_tick(crosstalk_eda_ui_display_crt_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, double now)
{
    uint64_t buff[CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_CHUNK];
    uint64_t total = 0;
    uint64_t got_bytes = 0;
    uint32_t elems_per_px = (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_BYTES_PER_PX;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || fpga == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (now < ctx->gfx_next) { return LDG_ERR_AOK; }

    ctx->gfx_next = now + CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_REFRESH_SECS;

    ret = crt_native_frame_get(ctx, fpga, slot_idx);
    if (ret == LDG_ERR_AOK) { return LDG_ERR_AOK; }

    if (ret != LDG_ERR_UNSUPPORTED && ret != LDG_ERR_NOT_INIT) { ctx->gfx_ok = LDG_TRUTH_FALSE; return ret; }

    ctx->gfx_crop = LDG_TRUTH_FALSE;
    ctx->gfx_text = LDG_TRUTH_FALSE;

    if (ctx->text_sig[0] == LDG_STR_TERM) { return LDG_ERR_AOK; }

    {
        crt_fb_t active = LDG_STRUCT_ZERO_INIT;
        uint64_t frame = 0;
        uint32_t frame_width = 0;
        uint8_t frame_has = LDG_TRUTH_FALSE;

        if (crt_fb_active_get(fpga, slot_idx, &active) == LDG_ERR_AOK)
        {
            uint64_t fb_base = active.addr / (uint64_t)CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_BYTES_PER_PX;

            if (crt_fb_drawable_is(&active) != LDG_TRUTH_TRUE)
            {
                if (ctx->src == (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_SRC_GFX && ctx->text_base != 0)
                {
                    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crt: video mode left graphics; the console is live; fmt: %u64", active.fmt);

                    ctx->src = (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_SRC_TEXT;
                    ctx->text_next = 0.0;
                    ctx->gfx_ok = LDG_TRUTH_FALSE;
                    ctx->gfx_geom_ok = LDG_TRUTH_FALSE;
                }

                return LDG_ERR_AOK;
            }

            if (fb_base != ctx->gfx_base || (uint32_t)active.width != ctx->gfx_w || (uint32_t)active.height != ctx->gfx_h)
            {
                uint64_t width = crt_mem_byte_width_get(fpga, slot_idx, ctx->text_sig);
                uint64_t span = active.width * active.height * (uint64_t)CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_BYTES_PER_PX;

                ctx->gfx_base = fb_base;
                ctx->gfx_w = (uint32_t)active.width;
                ctx->gfx_h = (uint32_t)active.height;
                ctx->gfx_ok = LDG_TRUTH_FALSE;
                ctx->gfx_geom_ok = LDG_TRUTH_FALSE;

                if (width == UINT64_MAX || fb_base + span > width)
                {
                    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crt: active framebuffer out of range; addr: 0x%X64; width: %u64; height: %u64; mem width: %u64", active.addr, active.width, active.height, width);

                    return LDG_ERR_AOK;
                }

                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crt: active framebuffer changed; addr: 0x%X64; width: %u64; height: %u64", active.addr, active.width, active.height);

                ctx->gfx_geom_ok = LDG_TRUTH_TRUE;
            }

            if (ctx->gfx_geom_ok != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

            if (ctx->src != (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_SRC_GFX)
            {
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crt: video mode entered graphics; width: %u64; height: %u64", active.width, active.height);

                ctx->src = (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_SRC_GFX;
                ctx->gfx_ok = LDG_TRUTH_FALSE;
            }

            frame = active.frame;
            frame_has = LDG_TRUTH_TRUE;
        }
        else if (ctx->src != (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_SRC_GFX) { return LDG_ERR_AOK; }
        else if (crosstalk_eda_ic_fpga_sig_peek(fpga, slot_idx, CROSSTALK_EDA_UI_DISPLAY_CRT_DDU_FRAME_SIG, &frame, &frame_width) == LDG_ERR_AOK) { frame_has = LDG_TRUTH_TRUE; }

        if (frame_has == LDG_TRUTH_TRUE)
        {
            if (ctx->gfx_ok == LDG_TRUTH_TRUE && frame == ctx->gfx_frame) { return LDG_ERR_AOK; }

            ctx->gfx_frame = frame;
        }
    }

    {
        uint64_t width = 0;
        uint32_t elem_width = 0;

        ret = crosstalk_eda_ic_fpga_mem_width_get(fpga, slot_idx, ctx->text_sig, &width, &elem_width);
        if (ret != LDG_ERR_AOK) { return ret; }

        elems_per_px = elem_width >= (uint32_t)CRT_GFX_WORD_WIDTH ? (uint32_t)CRT_STEP : (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_BYTES_PER_PX;
    }

    if (elems_per_px == (uint32_t)CRT_STEP)
    {
        uint64_t *ram = 0x0;
        uint32_t ram_cunt = 0;

        if (crosstalk_eda_ic_fpga_prof_get(fpga, slot_idx, (uint32_t)CROSSTALK_HDL_ABI_STATE_MEM, &ram, &ram_cunt) == LDG_ERR_AOK && ram != 0x0)
        {
            const uint32_t *word = (const uint32_t *)(const void *)ram;
            uint32_t px_cunt = ctx->gfx_w * ctx->gfx_h;
            uint32_t at = 0;

            if (px_cunt > (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_PX_CUNT) { px_cunt = (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_PX_CUNT; }

            for (at = 0; at < px_cunt; at = at + (uint32_t)CRT_STEP)
            {
                uint32_t val = word[ctx->gfx_base + (uint64_t)at];

                ctx->gfx_px[at] = (uint32_t)LDG_BYTE_MASK | ((uint32_t)ctx->gamma_lut[val & (uint32_t)CRT_GFX_CH_MASK] << CRT_TEXT_COL_SHIFT_B) | ((uint32_t)ctx->gamma_lut[(val >> (uint32_t)CRT_GFX_CH_BITS) & (uint32_t)CRT_GFX_CH_MASK] << CRT_TEXT_COL_SHIFT_G) | ((uint32_t)ctx->gamma_lut[(val >> (uint32_t)CRT_GFX_R_SHIFT) & (uint32_t)CRT_GFX_CH_MASK] << CRT_TEXT_COL_SHIFT_R);
            }

            ctx->gfx_ok = LDG_TRUTH_TRUE;

            return LDG_ERR_AOK;
        }
    }

    total = (uint64_t)ctx->gfx_w * (uint64_t)ctx->gfx_h * (uint64_t)elems_per_px;

    while (got_bytes < total)
    {
        uint64_t want = total - got_bytes;
        uint32_t got = 0;
        uint32_t at = 0;

        if (want > (uint64_t)CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_CHUNK) { want = (uint64_t)CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_CHUNK; }

        ret = crosstalk_eda_ic_fpga_mem_rd(fpga, slot_idx, ctx->text_sig, ctx->gfx_base + got_bytes, buff, (uint32_t)want, &got);
        if (ret != LDG_ERR_AOK) { return ret; }

        if (got != (uint32_t)want) { return LDG_ERR_BOUNDS; }

        for (at = 0; at < got; at = at + (uint32_t)CRT_STEP)
        {
            uint64_t elem_at = got_bytes + (uint64_t)at;
            uint32_t px_at = (uint32_t)(elem_at / (uint64_t)elems_per_px);

            if (px_at >= (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_PX_CUNT) { break; }

            if (elems_per_px == (uint32_t)CRT_STEP)
            {
                uint32_t word = (uint32_t)(buff[at] & (uint64_t)CRT_GFX_WORD_MASK);

                ctx->gfx_px[px_at] = (uint32_t)LDG_BYTE_MASK | ((word & (uint32_t)CRT_GFX_CH_MASK) << CRT_TEXT_COL_SHIFT_B) | (((word >> (uint32_t)CRT_GFX_CH_BITS) & (uint32_t)CRT_GFX_CH_MASK) << CRT_TEXT_COL_SHIFT_G) | (((word >> (uint32_t)CRT_GFX_R_SHIFT) & (uint32_t)CRT_GFX_CH_MASK) << CRT_TEXT_COL_SHIFT_R);
            }
            else
            {
                uint32_t lane = (uint32_t)(elem_at % (uint64_t)elems_per_px);
                uint32_t val = (uint32_t)(buff[at] & (uint64_t)LDG_BYTE_MASK);

                if (lane == 0) { ctx->gfx_px[px_at] = (uint32_t)LDG_BYTE_MASK | (val << CRT_TEXT_COL_SHIFT_B); }
                else if (lane == (uint32_t)CRT_GFX_LANE_G) { ctx->gfx_px[px_at] = ctx->gfx_px[px_at] | (val << CRT_TEXT_COL_SHIFT_G); }
                else if (lane == (uint32_t)CRT_GFX_LANE_R) { ctx->gfx_px[px_at] = ctx->gfx_px[px_at] | (val << CRT_TEXT_COL_SHIFT_R); }
            }
        }

        got_bytes = got_bytes + (uint64_t)got;
    }

    ctx->gfx_ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t crt_text_phase_get(const crosstalk_eda_ui_display_crt_ctx_t *ctx, uint32_t pitch, uint8_t vertical)
{
    uint32_t lit[CRT_CHAR_ROW_CUNT] = LDG_ARR_ZERO_INIT;
    uint32_t best = 0;
    uint32_t best_score = UINT32_MAX;
    uint32_t phase = 0;
    uint32_t row = 0;
    uint32_t col = 0;

    for (row = 0; row < ctx->gfx_h; row = row + (uint32_t)CRT_STEP) { for (col = 0; col < ctx->gfx_w; col = col + (uint32_t)CRT_STEP)
        {
            uint32_t at = (vertical == LDG_TRUTH_TRUE ? row : col) % pitch;
            if ((ctx->gfx_px[row * ctx->gfx_w + col] & (uint32_t)CROSSTALK_UI_CGA_GFX_RGB_MASK) == 0) { continue; }

            lit[at] = lit[at] + (uint32_t)CRT_STEP;
        }
    }

    for (phase = 0; phase < pitch; phase = phase + (uint32_t)CRT_STEP)
    {
        uint32_t score = lit[phase] + lit[(phase + pitch - (uint32_t)CRT_STEP) % pitch];

        if (score < best_score)
        {
            best_score = score;
            best = phase;
        }
    }

    return best;
}

static uint32_t crt_text_scan(const crosstalk_eda_ui_display_crt_ctx_t *ctx, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    uint32_t col_phase = crt_text_phase_get(ctx, (uint32_t)CRT_CHAR_COL_CUNT, LDG_TRUTH_FALSE);
    uint32_t row_phase = crt_text_phase_get(ctx, (uint32_t)CRT_CHAR_ROW_CUNT, LDG_TRUTH_TRUE);
    uint32_t grid_w = (ctx->gfx_w - col_phase) / (uint32_t)CRT_CHAR_COL_CUNT * (uint32_t)CRT_CHAR_COL_CUNT;
    uint32_t grid_h = (ctx->gfx_h - row_phase) / (uint32_t)CRT_CHAR_ROW_CUNT * (uint32_t)CRT_CHAR_ROW_CUNT;
    uint32_t avail_w = col_cunt * render->sample_col_cunt;
    uint32_t avail_h = row_cunt * render->sample_row_cunt;
    uint32_t scale = (uint32_t)CRT_STEP;
    uint32_t pos_x = 0;
    uint32_t pos_y = 0;
    uint32_t row = 0;
    uint32_t ret = 0;

    if (grid_w == 0 || grid_h == 0) { return LDG_ERR_AOK; }

    if (avail_w > grid_w * scale) { pos_x = (avail_w - grid_w * scale) / (uint32_t)CRT_CENTER_DIV / render->sample_col_cunt * render->sample_col_cunt; }

    if (avail_h > grid_h * scale) { pos_y = (avail_h - grid_h * scale) / (uint32_t)CRT_CENTER_DIV / render->sample_row_cunt * render->sample_row_cunt; }

    for (row = 0; row < grid_h; row = row + (uint32_t)CRT_STEP)
    {
        const uint32_t *line = &ctx->gfx_px[(row_phase + row) * ctx->gfx_w + col_phase];
        uint32_t run_beg = 0;
        uint32_t run_px = line[0];
        uint32_t col = 0;

        for (col = CRT_STEP; col <= grid_w; col = col + (uint32_t)CRT_STEP)
        {
            uint32_t px = col < grid_w ? line[col] : ~run_px;

            if (px == run_px) { continue; }

            if ((run_px & (uint32_t)CROSSTALK_UI_CGA_GFX_RGB_MASK) != 0)
            {
                ret = crosstalk_ui_cell_sample_fill(render, pos_x + run_beg * scale, pos_y + row * scale, (col - run_beg) * scale, scale, run_px);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            run_beg = col;
            run_px = px;
        }
    }

    return LDG_ERR_AOK;
}

static uint32_t crt_gfx_draw(const crosstalk_eda_ui_display_crt_ctx_t *ctx, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    crt_fit_t fit = LDG_STRUCT_ZERO_INIT;
    uint32_t row = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->gfx_ok != LDG_TRUTH_TRUE || ctx->gfx_w == 0 || ctx->gfx_h == 0) { return LDG_ERR_AOK; }

    if (ctx->gfx_text == LDG_TRUTH_TRUE && render->sample_col_cunt == (uint32_t)CRT_CHAR_COL_CUNT && render->sample_row_cunt == (uint32_t)CRT_CHAR_ROW_CUNT) { return crt_text_scan(ctx, render, col_cunt, row_cunt); }

    ret = crt_fit_get(render, col_cunt, row_cunt, ctx->gfx_w, ctx->gfx_h, &fit);
    if (ret != LDG_ERR_AOK) { return ret; }

    for (row = 0; row < ctx->gfx_h; row = row + (uint32_t)CRT_STEP)
    {
        uint32_t run_beg = 0;
        uint32_t run_px = ctx->gfx_px[row * ctx->gfx_w];
        uint32_t col = 0;

        for (col = CRT_STEP; col <= ctx->gfx_w; col = col + (uint32_t)CRT_STEP)
        {
            uint32_t px = col < ctx->gfx_w ? ctx->gfx_px[row * ctx->gfx_w + col] : ~run_px;

            if (px == run_px) { continue; }

            if ((run_px & (uint32_t)CROSSTALK_UI_CGA_GFX_RGB_MASK) != 0)
            {
                ret = crt_run_fill(render, &fit, ctx->gfx_w, ctx->gfx_h, row, run_beg, col, run_px);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            run_beg = col;
            run_px = px;
        }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_display_crt_kind_set(crosstalk_eda_ui_display_crt_ctx_t *ctx, uint32_t kind)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->kind = kind;

    return LDG_ERR_AOK;
}

static uint32_t crt_draw_cb(uint64_t win_id, void *user, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    crosstalk_eda_ui_display_crt_ctx_t *ctx = (crosstalk_eda_ui_display_crt_ctx_t *)user;
    const uint8_t *px = 0x0;
    uint32_t src_w = 0;
    uint32_t src_h = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->src_disp == 0x0) { return LDG_ERR_NOT_INIT; }

    if (win_id != (uint64_t)CROSSTALK_EDA_UI_DISPLAY_CRT_WINDOW_ID || col_cunt == 0 || row_cunt == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->src == (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_SRC_TEXT) { ret = crt_text_draw(ctx, render, col_cunt, row_cunt); }
    else if (ctx->src == (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_SRC_GFX) { ret = crt_gfx_draw(ctx, render, col_cunt, row_cunt); }
    else if (crosstalk_eda_io_display_front_get(ctx->src_disp, &px, &src_w, &src_h) == LDG_ERR_AOK && px != 0x0 && src_w != 0 && src_h != 0)
    {
        ctx->seen_frames = crosstalk_eda_io_display_frames_get(ctx->src_disp);

        ret = crt_raster_draw(render, px, src_w, src_h, col_cunt, row_cunt);
    }
    else { ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)CRT_ORIG, (uint32_t)CRT_ORIG, CRT_IDLE_TEXT, (uint64_t)sizeof(CRT_IDLE_TEXT), (uint32_t)CROSSTALK_UI_THEME_WHITE_RGBA, 0x0); }

    return ret;
}

uint32_t crosstalk_eda_ui_display_crt_win_reg(crosstalk_eda_ui_display_crt_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, const crosstalk_eda_io_display_ctx_t *src, uint32_t *out_slot)
{
    if (LDG_UNLIKELY(ctx == 0x0 || win_nav == 0x0 || src == 0x0 || out_slot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->src_disp = src;

    return crosstalk_ui_window_reg(win_nav, (uint64_t)CROSSTALK_EDA_UI_DISPLAY_CRT_WINDOW_ID, CRT_TITLE, &ctx->win, crt_draw_cb, 0x0, ctx, out_slot);
}

uint32_t crosstalk_eda_ui_display_crt_state_serialize(const crosstalk_eda_ui_display_crt_ctx_t *ctx, uint8_t *buff, uint32_t cap, uint32_t *out_need)
{
    uint32_t off = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_need == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_need = (uint32_t)CRT_STATE_BYTES;

    if (buff == 0x0) { return LDG_ERR_AOK; }

    if (cap < (uint32_t)CRT_STATE_BYTES) { return LDG_ERR_BOUNDS; }

    memcpy(&buff[off], &ctx->src, sizeof(ctx->src));
    off = off + (uint32_t)sizeof(ctx->src);
    memcpy(&buff[off], &ctx->kind, sizeof(ctx->kind));
    off = off + (uint32_t)sizeof(ctx->kind);
    memcpy(&buff[off], &ctx->text_base, sizeof(ctx->text_base));
    off = off + (uint32_t)sizeof(ctx->text_base);
    memcpy(&buff[off], &ctx->gfx_base, sizeof(ctx->gfx_base));
    off = off + (uint32_t)sizeof(ctx->gfx_base);
    memcpy(&buff[off], &ctx->gfx_w, sizeof(ctx->gfx_w));
    off = off + (uint32_t)sizeof(ctx->gfx_w);
    memcpy(&buff[off], &ctx->gfx_h, sizeof(ctx->gfx_h));
    off = off + (uint32_t)sizeof(ctx->gfx_h);
    memcpy(&buff[off], ctx->text_sig, sizeof(ctx->text_sig));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_display_crt_state_deserialize(crosstalk_eda_ui_display_crt_ctx_t *ctx, const uint8_t *buff, uint32_t len)
{
    uint32_t off = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || buff == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len != (uint32_t)CRT_STATE_BYTES) { return LDG_ERR_FUNC_ARG_INVALID; }

    memcpy(&ctx->src, &buff[off], sizeof(ctx->src));
    off = off + (uint32_t)sizeof(ctx->src);
    memcpy(&ctx->kind, &buff[off], sizeof(ctx->kind));
    off = off + (uint32_t)sizeof(ctx->kind);
    memcpy(&ctx->text_base, &buff[off], sizeof(ctx->text_base));
    off = off + (uint32_t)sizeof(ctx->text_base);
    memcpy(&ctx->gfx_base, &buff[off], sizeof(ctx->gfx_base));
    off = off + (uint32_t)sizeof(ctx->gfx_base);
    memcpy(&ctx->gfx_w, &buff[off], sizeof(ctx->gfx_w));
    off = off + (uint32_t)sizeof(ctx->gfx_w);
    memcpy(&ctx->gfx_h, &buff[off], sizeof(ctx->gfx_h));
    off = off + (uint32_t)sizeof(ctx->gfx_h);
    memcpy(ctx->text_sig, &buff[off], sizeof(ctx->text_sig));
    ctx->text_sig[sizeof(ctx->text_sig) - LDG_STR_TERM_SIZE] = LDG_STR_TERM;

    ctx->text_next = 0.0;
    ctx->gfx_next = 0.0;
    ctx->gfx_ok = LDG_TRUTH_FALSE;
    ctx->cell_cunt = 0;

    return LDG_ERR_AOK;
}
