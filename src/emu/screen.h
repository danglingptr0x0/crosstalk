#ifndef CROSSTALK_EMU_SCREEN_H
#define CROSSTALK_EMU_SCREEN_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/core/types.h>
#include <dangling/term/render.h>

#include "hdl/abi.h"

#define CROSSTALK_EMU_SCREEN_ERR_GEOM 1024

#define CROSSTALK_EMU_SCREEN_W_MAX 1600
#define CROSSTALK_EMU_SCREEN_H_MAX 1200
#define CROSSTALK_EMU_SCREEN_PX_MAX (CROSSTALK_EMU_SCREEN_W_MAX * CROSSTALK_EMU_SCREEN_H_MAX)
#define CROSSTALK_EMU_SCREEN_BYTES_PER_PX 4

#define CROSSTALK_EMU_SCREEN_TEXT_COL_CUNT 80
#define CROSSTALK_EMU_SCREEN_TEXT_ROW_CUNT 25
#define CROSSTALK_EMU_SCREEN_TEXT_CELL_CUNT (CROSSTALK_EMU_SCREEN_TEXT_COL_CUNT * CROSSTALK_EMU_SCREEN_TEXT_ROW_CUNT)
#define CROSSTALK_EMU_SCREEN_TEXT_BYTE_CUNT (CROSSTALK_EMU_SCREEN_TEXT_CELL_CUNT * 2)
#define CROSSTALK_EMU_SCREEN_TEXT_BASE_DEFAULT 0xB8000
#define CROSSTALK_EMU_SCREEN_TEXT_REFRESH_SECS 0.05
#define CROSSTALK_EMU_SCREEN_GFX_REFRESH_SECS 0.0166

#define CROSSTALK_EMU_SCREEN_GAMMA_LUT_CUNT 256
#define CROSSTALK_EMU_SCREEN_GAMMA_DEFAULT 1.0
#define CROSSTALK_EMU_SCREEN_GAMMA_MIN 0.5
#define CROSSTALK_EMU_SCREEN_GAMMA_MAX 4.0
#define CROSSTALK_EMU_SCREEN_GAMMA_UNIT 1.0

#define CROSSTALK_EMU_SCREEN_ASPECT_NUM 4
#define CROSSTALK_EMU_SCREEN_ASPECT_DEN 3

typedef enum crosstalk_emu_screen_src
{
    CROSSTALK_EMU_SCREEN_SRC_NONE = 0,
    CROSSTALK_EMU_SCREEN_SRC_TEXT,
    CROSSTALK_EMU_SCREEN_SRC_GFX
} crosstalk_emu_screen_src_t;

typedef struct crosstalk_emu_screen_ctx
{
    uint32_t *gfx_px;
    uint64_t cell[CROSSTALK_EMU_SCREEN_TEXT_BYTE_CUNT];
    uint8_t gamma_lut[CROSSTALK_EMU_SCREEN_GAMMA_LUT_CUNT];
    double gamma;
    double text_next;
    double gfx_next;
    uint64_t gfx_base;
    uint64_t gfx_frame;
    uint64_t text_base;
    uint32_t cell_cunt;
    uint32_t gfx_w;
    uint32_t gfx_h;
    uint32_t src;
    uint8_t gfx_ok;
    uint8_t pudding[7];
} crosstalk_emu_screen_ctx_t;

uint32_t crosstalk_emu_screen_init(crosstalk_emu_screen_ctx_t *ctx);
uint32_t crosstalk_emu_screen_shutdown(crosstalk_emu_screen_ctx_t *ctx);
uint32_t crosstalk_emu_screen_gamma_set(crosstalk_emu_screen_ctx_t *ctx, double gamma);
uint32_t crosstalk_emu_screen_tick(crosstalk_emu_screen_ctx_t *ctx, const crosstalk_hdl_abi_t *abi, uint32_t mem_sig, double now);
uint32_t crosstalk_emu_screen_draw(const crosstalk_emu_screen_ctx_t *ctx, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt);
uint32_t crosstalk_emu_screen_fit_get(uint32_t area_w, uint32_t area_h, uint32_t *out_x, uint32_t *out_y, uint32_t *out_w, uint32_t *out_h);

#endif
