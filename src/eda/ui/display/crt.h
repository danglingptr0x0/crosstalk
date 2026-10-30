#ifndef CROSSTALK_EDA_UI_DISPLAY_CRT_H
#define CROSSTALK_EDA_UI_DISPLAY_CRT_H

#include <stdint.h>

#include <dangling/io/path.h>

#include "eda/io/display.h"
#include "eda/ic/fpga/fpga.h"
#include "ui/window.h"

#define CROSSTALK_EDA_UI_DISPLAY_CRT_WINDOW_ID 0x43525400ULL

#define CROSSTALK_EDA_UI_DISPLAY_CRT_ERR_BIND 988

#define CROSSTALK_EDA_UI_DISPLAY_CRT_SIG_CAP 64
#define CROSSTALK_EDA_UI_DISPLAY_CRT_TEXT_COL_CUNT 80
#define CROSSTALK_EDA_UI_DISPLAY_CRT_TEXT_ROW_CUNT 25
#define CROSSTALK_EDA_UI_DISPLAY_CRT_TEXT_CELL_CUNT (CROSSTALK_EDA_UI_DISPLAY_CRT_TEXT_COL_CUNT * CROSSTALK_EDA_UI_DISPLAY_CRT_TEXT_ROW_CUNT)
#define CROSSTALK_EDA_UI_DISPLAY_CRT_TEXT_BYTE_CUNT (CROSSTALK_EDA_UI_DISPLAY_CRT_TEXT_CELL_CUNT * 2)
#define CROSSTALK_EDA_UI_DISPLAY_CRT_TEXT_BASE_DEFAULT 0xB8000
#define CROSSTALK_EDA_UI_DISPLAY_CRT_TEXT_REFRESH_SECS 0.05
#define CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_W_MAX 640
#define CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_H_MAX 480
#define CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_PX_CUNT (CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_W_MAX * CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_H_MAX)
#define CROSSTALK_EDA_UI_DISPLAY_CRT_GAMMA_LUT_CUNT 256
#define CROSSTALK_EDA_UI_DISPLAY_CRT_GAMMA_DEFAULT 1.0
#define CROSSTALK_EDA_UI_DISPLAY_CRT_GAMMA_MIN 0.5
#define CROSSTALK_EDA_UI_DISPLAY_CRT_GAMMA_MAX 4.0
#define CROSSTALK_EDA_UI_DISPLAY_CRT_GAMMA_UNIT 1.0
#define CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_BYTES_PER_PX 4
#define CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_CHUNK 256
#define CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_REFRESH_SECS 0.0
#define CROSSTALK_EDA_UI_DISPLAY_CRT_DDU_CTRL_SIG "ddu.ctrl"
#define CROSSTALK_EDA_UI_DISPLAY_CRT_DDU_BASE_SIG "ddu.fb_base"
#define CROSSTALK_EDA_UI_DISPLAY_CRT_DDU_MODE_SIG "ddu.fb_mode"
#define CROSSTALK_EDA_UI_DISPLAY_CRT_DDU_FMT_SIG "ddu.fb_fmt"
#define CROSSTALK_EDA_UI_DISPLAY_CRT_DDU_FRAME_SIG "ddu.fb_frame"
#define CROSSTALK_EDA_UI_DISPLAY_CRT_CTRL_GFX_BIT 0x2

typedef enum crosstalk_eda_ui_display_crt_src
{
    CROSSTALK_EDA_UI_DISPLAY_CRT_SRC_RASTER = 0,
    CROSSTALK_EDA_UI_DISPLAY_CRT_SRC_TEXT,
    CROSSTALK_EDA_UI_DISPLAY_CRT_SRC_GFX
} crosstalk_eda_ui_display_crt_src_t;

typedef enum crosstalk_eda_ui_display_crt_kind
{
    CROSSTALK_EDA_UI_DISPLAY_CRT_KIND_RAW = 0,
    CROSSTALK_EDA_UI_DISPLAY_CRT_KIND_CRT,
    CROSSTALK_EDA_UI_DISPLAY_CRT_KIND_VGA,
    CROSSTALK_EDA_UI_DISPLAY_CRT_KIND_CGA,
    CROSSTALK_EDA_UI_DISPLAY_CRT_KIND_SCOPE
} crosstalk_eda_ui_display_crt_kind_t;

typedef struct crosstalk_eda_ui_display_crt_ctx
{
    crosstalk_ui_window_state_t win;
    const crosstalk_eda_io_display_ctx_t *src_disp;
    uint64_t cell[CROSSTALK_EDA_UI_DISPLAY_CRT_TEXT_BYTE_CUNT];
    uint32_t gfx_px[CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_PX_CUNT];
    uint8_t gamma_lut[CROSSTALK_EDA_UI_DISPLAY_CRT_GAMMA_LUT_CUNT];
    double gamma;
    uint8_t gfx_crop;
    uint8_t gfx_text;
    uint8_t text_native;
    uint8_t crop_pudding[5];
    char text_sig[CROSSTALK_EDA_UI_DISPLAY_CRT_SIG_CAP];
    uint64_t text_base;
    double text_next;
    uint64_t gfx_base;
    uint64_t gfx_frame;
    double gfx_next;
    uint64_t seen_frames;
    uint32_t cell_cunt;
    uint32_t gfx_w;
    uint32_t gfx_h;
    uint32_t gfx_ok;
    uint32_t src;
    uint32_t kind;
    uint8_t open;
    uint8_t gfx_geom_ok;
    uint8_t title_hlt;
    uint8_t title_pudding[5];
} crosstalk_eda_ui_display_crt_ctx_t;

uint32_t crosstalk_eda_ui_display_crt_init(crosstalk_eda_ui_display_crt_ctx_t *ctx);
uint32_t crosstalk_eda_ui_display_crt_gamma_set(crosstalk_eda_ui_display_crt_ctx_t *ctx, double gamma);
uint32_t crosstalk_eda_ui_display_crt_open_set(crosstalk_eda_ui_display_crt_ctx_t *ctx, uint8_t open);
uint32_t crosstalk_eda_ui_display_crt_kind_set(crosstalk_eda_ui_display_crt_ctx_t *ctx, uint32_t kind);
uint32_t crosstalk_eda_ui_display_crt_text_bind(crosstalk_eda_ui_display_crt_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, const char *sig_name, uint64_t base);
uint32_t crosstalk_eda_ui_display_crt_text_tick(crosstalk_eda_ui_display_crt_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, double now);
uint32_t crosstalk_eda_ui_display_crt_gfx_bind(crosstalk_eda_ui_display_crt_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, const char *sig_name, uint64_t base, uint32_t gfx_w, uint32_t gfx_h);
uint32_t crosstalk_eda_ui_display_crt_gfx_dev_bind(crosstalk_eda_ui_display_crt_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, const char *sig_name);
uint32_t crosstalk_eda_ui_display_crt_gfx_tick(crosstalk_eda_ui_display_crt_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, double now);
uint32_t crosstalk_eda_ui_display_crt_state_serialize(const crosstalk_eda_ui_display_crt_ctx_t *ctx, uint8_t *buff, uint32_t cap, uint32_t *out_need);
uint32_t crosstalk_eda_ui_display_crt_state_deserialize(crosstalk_eda_ui_display_crt_ctx_t *ctx, const uint8_t *buff, uint32_t len);
uint32_t crosstalk_eda_ui_display_crt_win_reg(crosstalk_eda_ui_display_crt_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, const crosstalk_eda_io_display_ctx_t *src, uint32_t *out_slot);

#endif
