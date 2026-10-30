#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include <dangling/logging/logging.h>

#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/str/str.h>

#include "core/log.h"
#include "emu/screen.h"
#include "ui/cga.h"
#include "ui/theme.h"

#define SCREEN_STEP 1
#define SCREEN_CELL_BYTES 2
#define SCREEN_WORD_BYTES 4
#define SCREEN_CENTER_DIV 2
#define SCREEN_ATTR_BG_SHIFT 4
#define SCREEN_PAL_CUNT 16
#define SCREEN_PAL_MASK 0xF
#define SCREEN_CH_MASK 0xFFU
#define SCREEN_CH_BITS 8
#define SCREEN_R_SHIFT 16
#define SCREEN_COL_SHIFT_R 24
#define SCREEN_COL_SHIFT_G 16
#define SCREEN_COL_SHIFT_B 8
#define SCREEN_DDU_CTRL_SIG "ddu.ctrl"
#define SCREEN_DDU_CTRL_GFX_BIT 0x2
#define SCREEN_DDU_GFX_ACTIVE_H 400
#define SCREEN_DDU_BORDER_CUNT 2
#define SCREEN_DDU_GREEN_OFF 1
#define SCREEN_DDU_BLUE_OFF 2
#define SCREEN_IDLE_TEXT "no signal"
#define SCREEN_IDLE_ORIG_COL 0
#define SCREEN_IDLE_ORIG_ROW 0


static uint32_t screen_pal_get(uint8_t attr)
{
    const uint32_t pal[SCREEN_PAL_CUNT] = {
        CROSSTALK_UI_CGA_PAL_0, CROSSTALK_UI_CGA_PAL_1, CROSSTALK_UI_CGA_PAL_2, CROSSTALK_UI_CGA_PAL_3, CROSSTALK_UI_CGA_PAL_4, CROSSTALK_UI_CGA_PAL_5, CROSSTALK_UI_CGA_PAL_6, CROSSTALK_UI_CGA_PAL_7, CROSSTALK_UI_CGA_PAL_8, CROSSTALK_UI_CGA_PAL_9, CROSSTALK_UI_CGA_PAL_A, CROSSTALK_UI_CGA_PAL_B, CROSSTALK_UI_CGA_PAL_C, CROSSTALK_UI_CGA_PAL_D, CROSSTALK_UI_CGA_PAL_E, CROSSTALK_UI_CGA_PAL_F
    };

    return pal[attr & (uint8_t)SCREEN_PAL_MASK];
}

static uint32_t screen_color_get(uint32_t packed, ldg_term_render_color_t *out)
{
    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->red = (uint8_t)((packed >> SCREEN_COL_SHIFT_R) & (uint32_t)LDG_BYTE_MASK);
    out->green = (uint8_t)((packed >> SCREEN_COL_SHIFT_G) & (uint32_t)LDG_BYTE_MASK);
    out->blue = (uint8_t)((packed >> SCREEN_COL_SHIFT_B) & (uint32_t)LDG_BYTE_MASK);
    out->pudding[0] = 0;

    return LDG_ERR_AOK;
}

static uint32_t screen_sample_fill(ldg_term_render_t *render, uint32_t pos_x, uint32_t pos_y, uint32_t extent_w, uint32_t extent_h, const ldg_term_render_color_t *color)
{
    ldg_term_render_fixed_t pos[LDG_TERM_RENDER_PLANE_CUNT] = LDG_ARR_ZERO_INIT;
    ldg_term_render_fixed_t extent[LDG_TERM_RENDER_PLANE_CUNT] = LDG_ARR_ZERO_INIT;

    if (LDG_UNLIKELY(render == 0x0 || color == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    pos[0] = (ldg_term_render_fixed_t)(pos_x << LDG_TERM_RENDER_FIXED_SHIFT);
    pos[1] = (ldg_term_render_fixed_t)(pos_y << LDG_TERM_RENDER_FIXED_SHIFT);
    extent[0] = (ldg_term_render_fixed_t)(extent_w << LDG_TERM_RENDER_FIXED_SHIFT);
    extent[1] = (ldg_term_render_fixed_t)(extent_h << LDG_TERM_RENDER_FIXED_SHIFT);

    return ldg_term_render_rect_fill(render, &pos[0], &extent[0], color);
}

static uint32_t screen_fb_get(const crosstalk_hdl_abi_t *abi, uint64_t *out_addr, uint64_t *out_fmt, uint64_t *out_w, uint64_t *out_h, uint64_t *out_frame)
{
    uint64_t *fb = 0x0;
    uint32_t fb_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(abi == 0x0 || out_addr == 0x0 || out_fmt == 0x0 || out_w == 0x0 || out_h == 0x0 || out_frame == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_addr = 0;
    *out_fmt = 0;
    *out_w = 0;
    *out_h = 0;
    *out_frame = 0;

    if (abi->state_get == 0x0) { return LDG_ERR_UNSUPPORTED; }

    ret = abi->state_get((uint32_t)CROSSTALK_HDL_ABI_STATE_EMU_FB, &fb, &fb_cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (fb == 0x0 || fb_cunt < (uint32_t)CROSSTALK_HDL_ABI_FB_CUNT) { return LDG_ERR_UNSUPPORTED; }

    *out_addr = fb[CROSSTALK_HDL_ABI_FB_ADDR_AT];
    *out_fmt = fb[CROSSTALK_HDL_ABI_FB_FMT_AT];
    *out_w = fb[CROSSTALK_HDL_ABI_FB_MODE_AT] & (uint64_t)CROSSTALK_HDL_ABI_FB_MODE_W_MASK;
    *out_h = fb[CROSSTALK_HDL_ABI_FB_MODE_AT] >> (uint64_t)CROSSTALK_HDL_ABI_FB_MODE_H_SHIFT;
    *out_frame = fb[CROSSTALK_HDL_ABI_FB_FRAME_AT];

    return LDG_ERR_AOK;
}

static uint8_t screen_fb_drawable_is(uint64_t fmt, uint64_t width, uint64_t height)
{
    if (fmt != (uint64_t)CROSSTALK_HDL_ABI_FB_FMT_XRGB8888 && fmt != (uint64_t)CROSSTALK_HDL_ABI_FB_FMT_TEXT_RASTER) { return LDG_TRUTH_FALSE; }

    if (width == 0 || width > (uint64_t)CROSSTALK_EMU_SCREEN_W_MAX) { return LDG_TRUTH_FALSE; }

    if (height == 0 || height > (uint64_t)CROSSTALK_EMU_SCREEN_H_MAX) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static uint32_t screen_px_pack(const crosstalk_emu_screen_ctx_t *ctx, uint32_t word)
{
    uint32_t col_b = ctx->gamma_lut[word & (uint32_t)SCREEN_CH_MASK];
    uint32_t col_g = ctx->gamma_lut[(word >> (uint32_t)SCREEN_CH_BITS) & (uint32_t)SCREEN_CH_MASK];
    uint32_t col_r = ctx->gamma_lut[(word >> (uint32_t)SCREEN_R_SHIFT) & (uint32_t)SCREEN_CH_MASK];

    return (uint32_t)LDG_BYTE_MASK | (col_b << SCREEN_COL_SHIFT_B) | (col_g << SCREEN_COL_SHIFT_G) | (col_r << SCREEN_COL_SHIFT_R);
}

static uint32_t screen_gfx_scan(crosstalk_emu_screen_ctx_t *ctx, const crosstalk_hdl_abi_t *abi, uint32_t mem_sig)
{
    uint32_t px_cunt = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || abi == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    px_cunt = ctx->gfx_w * ctx->gfx_h;

    if (px_cunt > (uint32_t)CROSSTALK_EMU_SCREEN_PX_MAX) { return CROSSTALK_EMU_SCREEN_ERR_GEOM; }

    if (abi->mem_ld == 0x0) { return LDG_ERR_UNSUPPORTED; }

    if (mem_sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { return LDG_ERR_NOT_FOUND; }

    for (at = 0; at < px_cunt; at = at + (uint32_t)SCREEN_STEP) { ctx->gfx_px[at] = screen_px_pack(ctx, (uint32_t)(abi->mem_ld(mem_sig, ctx->gfx_base + (uint64_t)at) & (uint64_t)UINT32_MAX)); }

    return LDG_ERR_AOK;
}

static uint32_t screen_ddu_scan(crosstalk_emu_screen_ctx_t *ctx, const crosstalk_hdl_abi_t *abi)
{
    uint8_t *rgba = 0x0;
    uint64_t ctrl = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t top = 0;
    uint32_t sig = (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE;
    uint32_t at = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || abi == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (abi->ddu_frame_get == 0x0) { return LDG_ERR_UNSUPPORTED; }

    rgba = (uint8_t *)(void *)ctx->gfx_px;

    ret = abi->ddu_frame_get(rgba, (uint64_t)CROSSTALK_EMU_SCREEN_PX_MAX * (uint64_t)CROSSTALK_EMU_SCREEN_BYTES_PER_PX, &width, &height);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (width == 0 || height == 0 || width > (uint32_t)CROSSTALK_EMU_SCREEN_W_MAX || height > (uint32_t)CROSSTALK_EMU_SCREEN_H_MAX) { return CROSSTALK_EMU_SCREEN_ERR_GEOM; }

    if (abi->sig_find == 0x0 || abi->peek == 0x0) { return LDG_ERR_UNSUPPORTED; }

    sig = abi->sig_find(SCREEN_DDU_CTRL_SIG);
    if (sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { return LDG_ERR_UNSUPPORTED; }

    ctrl = abi->peek(sig);

    if ((ctrl & (uint64_t)SCREEN_DDU_CTRL_GFX_BIT) != 0 && height >= (uint32_t)SCREEN_DDU_GFX_ACTIVE_H)
    {
        top = (height - (uint32_t)SCREEN_DDU_GFX_ACTIVE_H) / (uint32_t)SCREEN_DDU_BORDER_CUNT;
        height = (uint32_t)SCREEN_DDU_GFX_ACTIVE_H;
    }

    for (idx = 0; idx < width * height; idx = idx + (uint32_t)SCREEN_STEP)
    {
        at = (top * width + idx) * (uint32_t)CROSSTALK_EMU_SCREEN_BYTES_PER_PX;

        ctx->gfx_px[idx] = (uint32_t)LDG_BYTE_MASK | ((uint32_t)ctx->gamma_lut[rgba[at]] << SCREEN_COL_SHIFT_R) | ((uint32_t)ctx->gamma_lut[rgba[at + (uint32_t)SCREEN_DDU_GREEN_OFF]] << SCREEN_COL_SHIFT_G) | ((uint32_t)ctx->gamma_lut[rgba[at + (uint32_t)SCREEN_DDU_BLUE_OFF]] << SCREEN_COL_SHIFT_B);
    }

    ctx->gfx_w = width;
    ctx->gfx_h = height;
    ctx->gfx_ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t screen_gfx_draw(const crosstalk_emu_screen_ctx_t *ctx, ldg_term_render_t *render, uint32_t fit_x, uint32_t fit_y, uint32_t fit_w, uint32_t fit_h)
{
    ldg_term_render_color_t color = LDG_STRUCT_ZERO_INIT;
    uint32_t row = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->gfx_ok != LDG_TRUTH_TRUE || ctx->gfx_w == 0 || ctx->gfx_h == 0) { return LDG_ERR_AOK; }

    for (row = 0; row < ctx->gfx_h; row = row + (uint32_t)SCREEN_STEP)
    {
        uint32_t top = (uint32_t)((uint64_t)row * (uint64_t)fit_h / (uint64_t)ctx->gfx_h);
        uint32_t bottom = (uint32_t)((uint64_t)(row + (uint32_t)SCREEN_STEP) * (uint64_t)fit_h / (uint64_t)ctx->gfx_h);
        uint32_t run_beg = 0;
        uint32_t run_px = ctx->gfx_px[row * ctx->gfx_w];
        uint32_t col = 0;

        if (bottom == top) { continue; }

        for (col = SCREEN_STEP; col <= ctx->gfx_w; col = col + (uint32_t)SCREEN_STEP)
        {
            uint32_t px = col < ctx->gfx_w ? ctx->gfx_px[row * ctx->gfx_w + col] : ~run_px;
            uint32_t lhs = 0;
            uint32_t rhs = 0;

            if (px == run_px) { continue; }

            lhs = (uint32_t)((uint64_t)run_beg * (uint64_t)fit_w / (uint64_t)ctx->gfx_w);
            rhs = (uint32_t)((uint64_t)col * (uint64_t)fit_w / (uint64_t)ctx->gfx_w);

            if ((run_px & (uint32_t)CROSSTALK_UI_CGA_GFX_RGB_MASK) != 0 && rhs != lhs)
            {
                ret = screen_color_get(run_px, &color);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                ret = screen_sample_fill(render, fit_x + lhs, fit_y + top, rhs - lhs, bottom - top, &color);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            run_beg = col;
            run_px = px;
        }
    }

    return LDG_ERR_AOK;
}

static uint32_t screen_text_scan(crosstalk_emu_screen_ctx_t *ctx, const crosstalk_hdl_abi_t *abi, uint32_t mem_sig)
{
    uint64_t word_at = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || abi == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (abi->mem_ld == 0x0) { return LDG_ERR_UNSUPPORTED; }

    if (mem_sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { return LDG_ERR_NOT_FOUND; }

    word_at = ctx->text_base / (uint64_t)SCREEN_WORD_BYTES;

    for (at = 0; at < (uint32_t)CROSSTALK_EMU_SCREEN_TEXT_BYTE_CUNT; at = at + (uint32_t)SCREEN_WORD_BYTES)
    {
        uint64_t word = abi->mem_ld(mem_sig, word_at + (uint64_t)(at / (uint32_t)SCREEN_WORD_BYTES));
        uint32_t lane = 0;

        for (lane = 0; lane < (uint32_t)SCREEN_WORD_BYTES; lane = lane + (uint32_t)SCREEN_STEP) { ctx->cell[at + lane] = (word >> (uint64_t)((uint32_t)LDG_BYTE_BITS * lane)) & (uint64_t)LDG_BYTE_MASK; }
    }

    ctx->cell_cunt = (uint32_t)CROSSTALK_EMU_SCREEN_TEXT_BYTE_CUNT;

    return LDG_ERR_AOK;
}

static uint8_t screen_ddu_text_mode_is(const crosstalk_hdl_abi_t *abi)
{
    uint32_t sig = (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE;

    if (LDG_UNLIKELY(abi == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (abi->sig_find == 0x0 || abi->peek == 0x0) { return LDG_TRUTH_FALSE; }

    sig = abi->sig_find(SCREEN_DDU_CTRL_SIG);
    if (sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { return LDG_TRUTH_FALSE; }

    return (abi->peek(sig) & (uint64_t)SCREEN_DDU_CTRL_GFX_BIT) == 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t screen_ddu_text_get(crosstalk_emu_screen_ctx_t *ctx, const crosstalk_hdl_abi_t *abi)
{
    uint8_t text[CROSSTALK_EMU_SCREEN_TEXT_BYTE_CUNT] = LDG_ARR_ZERO_INIT;
    uint32_t cols = 0;
    uint32_t rows = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || abi == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (abi->ddu_text_get == 0x0) { return LDG_ERR_UNSUPPORTED; }

    ret = abi->ddu_text_get(text, (uint64_t)sizeof(text), &cols, &rows);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (cols != (uint32_t)CROSSTALK_EMU_SCREEN_TEXT_COL_CUNT || rows != (uint32_t)CROSSTALK_EMU_SCREEN_TEXT_ROW_CUNT) { return CROSSTALK_EMU_SCREEN_ERR_GEOM; }

    for (at = 0; at < (uint32_t)CROSSTALK_EMU_SCREEN_TEXT_BYTE_CUNT; at = at + (uint32_t)SCREEN_STEP) { ctx->cell[at] = (uint64_t)text[at]; }

    ctx->cell_cunt = (uint32_t)CROSSTALK_EMU_SCREEN_TEXT_BYTE_CUNT;

    if (ctx->src != (uint32_t)CROSSTALK_EMU_SCREEN_SRC_TEXT)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "screen: the console is live; source: core text");

        ctx->src = (uint32_t)CROSSTALK_EMU_SCREEN_SRC_TEXT;
        ctx->gfx_ok = LDG_TRUTH_FALSE;
    }

    return LDG_ERR_AOK;
}

static uint32_t screen_text_run_draw(ldg_term_render_t *render, uint32_t col, uint32_t row, const char *run, uint32_t run_len, uint8_t attr)
{
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t bg = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || run == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = screen_color_get(screen_pal_get(attr), &fg);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = screen_color_get(screen_pal_get((uint8_t)(attr >> (uint8_t)SCREEN_ATTR_BG_SHIFT)), &bg);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_term_render_text_draw(render, col, row, run, (uint64_t)run_len, &fg, &bg);
}

static uint32_t screen_text_draw(const crosstalk_emu_screen_ctx_t *ctx, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    char run[CROSSTALK_EMU_SCREEN_TEXT_COL_CUNT] = LDG_ARR_ZERO_INIT;
    uint32_t orig_col = 0;
    uint32_t orig_row = 0;
    uint32_t shown_col = (uint32_t)CROSSTALK_EMU_SCREEN_TEXT_COL_CUNT;
    uint32_t shown_row = (uint32_t)CROSSTALK_EMU_SCREEN_TEXT_ROW_CUNT;
    uint32_t row = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (col_cunt > shown_col) { orig_col = (col_cunt - shown_col) / (uint32_t)SCREEN_CENTER_DIV; }
    else { shown_col = col_cunt; }

    if (row_cunt > shown_row) { orig_row = (row_cunt - shown_row) / (uint32_t)SCREEN_CENTER_DIV; }
    else { shown_row = row_cunt; }

    for (row = 0; row < shown_row; row = row + (uint32_t)SCREEN_STEP)
    {
        uint32_t col = 0;
        uint32_t run_beg = 0;
        uint32_t run_len = 0;
        uint8_t run_attr = 0;

        for (col = 0; col < shown_col; col = col + (uint32_t)SCREEN_STEP)
        {
            uint32_t at = (row * (uint32_t)CROSSTALK_EMU_SCREEN_TEXT_COL_CUNT + col) * (uint32_t)SCREEN_CELL_BYTES;
            uint8_t ch = 0;
            uint8_t attr = 0;

            if (at + (uint32_t)SCREEN_STEP >= ctx->cell_cunt) { break; }

            ch = (uint8_t)(ctx->cell[at] & (uint64_t)LDG_BYTE_MASK);
            attr = (uint8_t)(ctx->cell[at + (uint32_t)SCREEN_STEP] & (uint64_t)LDG_BYTE_MASK);

            if (ch < (uint8_t)LDG_ASCII_PRINTABLE_MIN || ch > (uint8_t)LDG_ASCII_PRINTABLE_MAX) { ch = (uint8_t)' '; }

            if (run_len != 0 && attr != run_attr)
            {
                ret = screen_text_run_draw(render, orig_col + run_beg, orig_row + row, &run[0], run_len, run_attr);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                run_len = 0;
            }

            if (run_len == 0) { run_beg = col; run_attr = attr; }

            run[run_len] = (char)ch;
            run_len = run_len + (uint32_t)SCREEN_STEP;
        }

        if (run_len == 0) { continue; }

        ret = screen_text_run_draw(render, orig_col + run_beg, orig_row + row, &run[0], run_len, run_attr);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_screen_fit_get(uint32_t area_w, uint32_t area_h, uint32_t *out_x, uint32_t *out_y, uint32_t *out_w, uint32_t *out_h)
{
    uint32_t fit_w = 0;
    uint32_t fit_h = 0;

    if (LDG_UNLIKELY(out_x == 0x0 || out_y == 0x0 || out_w == 0x0 || out_h == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_x = 0;
    *out_y = 0;
    *out_w = 0;
    *out_h = 0;

    if (area_w == 0 || area_h == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    fit_w = area_w;
    fit_h = (uint32_t)((uint64_t)fit_w * (uint64_t)CROSSTALK_EMU_SCREEN_ASPECT_DEN / (uint64_t)CROSSTALK_EMU_SCREEN_ASPECT_NUM);

    if (fit_h > area_h)
    {
        fit_h = area_h;
        fit_w = (uint32_t)((uint64_t)fit_h * (uint64_t)CROSSTALK_EMU_SCREEN_ASPECT_NUM / (uint64_t)CROSSTALK_EMU_SCREEN_ASPECT_DEN);
    }

    *out_x = (area_w - fit_w) / (uint32_t)SCREEN_CENTER_DIV;
    *out_y = (area_h - fit_h) / (uint32_t)SCREEN_CENTER_DIV;
    *out_w = fit_w;
    *out_h = fit_h;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_screen_gamma_set(crosstalk_emu_screen_ctx_t *ctx, double gamma)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (gamma < CROSSTALK_EMU_SCREEN_GAMMA_MIN || gamma > CROSSTALK_EMU_SCREEN_GAMMA_MAX) { return LDG_ERR_RANGE; }

    ctx->gamma = gamma;

    for (at = 0; at <= (uint32_t)LDG_BYTE_MASK; at = at + (uint32_t)SCREEN_STEP)
    {
        double lit = pow((double)at / (double)LDG_BYTE_MASK, CROSSTALK_EMU_SCREEN_GAMMA_UNIT / gamma) * (double)LDG_BYTE_MASK;

        ctx->gamma_lut[at] = (uint8_t)(lit > (double)LDG_BYTE_MASK ? (double)LDG_BYTE_MASK : lit);
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_screen_init(crosstalk_emu_screen_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->text_base = (uint64_t)CROSSTALK_EMU_SCREEN_TEXT_BASE_DEFAULT;
    ctx->src = (uint32_t)CROSSTALK_EMU_SCREEN_SRC_NONE;

    ret = crosstalk_emu_screen_gamma_set(ctx, CROSSTALK_EMU_SCREEN_GAMMA_DEFAULT);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_alloc((uint64_t)CROSSTALK_EMU_SCREEN_PX_MAX * (uint64_t)CROSSTALK_EMU_SCREEN_BYTES_PER_PX, (void **)&ctx->gfx_px);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "screen: pixel buffer alloc refused; bytes: %u64; err: %u32", (uint64_t)CROSSTALK_EMU_SCREEN_PX_MAX * (uint64_t)CROSSTALK_EMU_SCREEN_BYTES_PER_PX, ret);

        return ret;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_screen_shutdown(crosstalk_emu_screen_ctx_t *ctx)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->gfx_px != 0x0) { ret = ldg_mem_dealloc(ctx->gfx_px); }

    ctx->gfx_px = 0x0;

    return ret;
}

uint32_t crosstalk_emu_screen_tick(crosstalk_emu_screen_ctx_t *ctx, const crosstalk_hdl_abi_t *abi, uint32_t mem_sig, double now)
{
    uint64_t addr = 0;
    uint64_t fmt = 0;
    uint64_t width = 0;
    uint64_t height = 0;
    uint64_t frame = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || abi == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (now < ctx->gfx_next && ctx->src == (uint32_t)CROSSTALK_EMU_SCREEN_SRC_GFX && ctx->gfx_ok == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (screen_ddu_text_mode_is(abi) == LDG_TRUTH_TRUE)
    {
        ret = screen_ddu_text_get(ctx, abi);
        if (ret != LDG_ERR_UNSUPPORTED) { return ret; }
    }

    ret = screen_ddu_scan(ctx, abi);

    if (ret == LDG_ERR_UNSUPPORTED && screen_ddu_text_get(ctx, abi) == LDG_ERR_AOK) { return LDG_ERR_AOK; }

    if (ret == LDG_ERR_AOK)
    {
        ctx->gfx_next = now + CROSSTALK_EMU_SCREEN_GFX_REFRESH_SECS;

        if (ctx->src != (uint32_t)CROSSTALK_EMU_SCREEN_SRC_GFX)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "screen: card-resident framebuffer live; scanout leaves guest ram");

            ctx->src = (uint32_t)CROSSTALK_EMU_SCREEN_SRC_GFX;
        }

        return LDG_ERR_AOK;
    }

    if (screen_fb_get(abi, &addr, &fmt, &width, &height, &frame) == LDG_ERR_AOK && fmt == (uint64_t)CROSSTALK_HDL_ABI_FB_FMT_CARD)
    {
        if (ctx->src != (uint32_t)CROSSTALK_EMU_SCREEN_SRC_GFX)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "screen: card-resident framebuffer live; scanout leaves guest ram");

            ctx->src = (uint32_t)CROSSTALK_EMU_SCREEN_SRC_GFX;
        }

        return LDG_ERR_AOK;
    }

    if (screen_fb_get(abi, &addr, &fmt, &width, &height, &frame) == LDG_ERR_AOK && screen_fb_drawable_is(fmt, width, height) == LDG_TRUTH_TRUE)
    {
        uint64_t base = addr / (uint64_t)SCREEN_WORD_BYTES;

        if (base != ctx->gfx_base || (uint32_t)width != ctx->gfx_w || (uint32_t)height != ctx->gfx_h)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "screen: framebuffer changed; addr: 0x%X64; width: %u64; height: %u64; fmt: %u64", addr, width, height, fmt);

            ctx->gfx_base = base;
            ctx->gfx_w = (uint32_t)width;
            ctx->gfx_h = (uint32_t)height;
            ctx->gfx_ok = LDG_TRUTH_FALSE;
        }

        if (ctx->src != (uint32_t)CROSSTALK_EMU_SCREEN_SRC_GFX)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "screen: graphics live; width: %u64; height: %u64", width, height);

            ctx->src = (uint32_t)CROSSTALK_EMU_SCREEN_SRC_GFX;
            ctx->gfx_ok = LDG_TRUTH_FALSE;
        }

        if (now < ctx->gfx_next && frame == ctx->gfx_frame) { return LDG_ERR_AOK; }

        ctx->gfx_next = now + CROSSTALK_EMU_SCREEN_GFX_REFRESH_SECS;
        ctx->gfx_frame = frame;

        ret = screen_gfx_scan(ctx, abi, mem_sig);
        if (ret != LDG_ERR_AOK) { return ret; }

        ctx->gfx_ok = LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    if (ctx->src != (uint32_t)CROSSTALK_EMU_SCREEN_SRC_TEXT)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "screen: the console is live; fmt: %u64", fmt);

        ctx->src = (uint32_t)CROSSTALK_EMU_SCREEN_SRC_TEXT;
        ctx->gfx_ok = LDG_TRUTH_FALSE;
    }

    if (now < ctx->text_next) { return LDG_ERR_AOK; }

    ctx->text_next = now + CROSSTALK_EMU_SCREEN_TEXT_REFRESH_SECS;

    return screen_text_scan(ctx, abi, mem_sig);
}

static uint32_t screen_gfx_fit_draw(const crosstalk_emu_screen_ctx_t *ctx, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    uint32_t unit_w = 0;
    uint32_t unit_h = 0;
    uint32_t fit_x = 0;
    uint32_t fit_y = 0;
    uint32_t fit_w = 0;
    uint32_t fit_h = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    unit_w = render->cell_aspect_num * render->sample_row_cunt;
    unit_h = render->cell_aspect_den * render->sample_col_cunt;

    if (unit_w == 0 || unit_h == 0) { return LDG_ERR_NOT_INIT; }

    ret = crosstalk_emu_screen_fit_get(col_cunt * render->sample_col_cunt * unit_w, row_cunt * render->sample_row_cunt * unit_h, &fit_x, &fit_y, &fit_w, &fit_h);
    if (ret != LDG_ERR_AOK) { return ret; }

    return screen_gfx_draw(ctx, render, fit_x / unit_w, fit_y / unit_h, fit_w / unit_w, fit_h / unit_h);
}

static uint32_t screen_idle_draw(ldg_term_render_t *render)
{
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t bg = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = screen_color_get((uint32_t)CROSSTALK_UI_THEME_WHITE_RGBA, &fg);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_term_render_text_draw(render, (uint32_t)SCREEN_IDLE_ORIG_COL, (uint32_t)SCREEN_IDLE_ORIG_ROW, SCREEN_IDLE_TEXT, (uint64_t)(sizeof(SCREEN_IDLE_TEXT) - LDG_STR_TERM_SIZE), &fg, &bg);
}

uint32_t crosstalk_emu_screen_draw(const crosstalk_emu_screen_ctx_t *ctx, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (col_cunt == 0 || row_cunt == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->src == (uint32_t)CROSSTALK_EMU_SCREEN_SRC_GFX && ctx->gfx_ok == LDG_TRUTH_TRUE) { return screen_gfx_fit_draw(ctx, render, col_cunt, row_cunt); }

    if (ctx->src == (uint32_t)CROSSTALK_EMU_SCREEN_SRC_TEXT && ctx->cell_cunt != 0) { return screen_text_draw(ctx, render, col_cunt, row_cunt); }

    return screen_idle_draw(render);
}
