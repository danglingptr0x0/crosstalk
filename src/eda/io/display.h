#ifndef CROSSTALK_EDA_IO_DISPLAY_H
#define CROSSTALK_EDA_IO_DISPLAY_H

#include <stdint.h>

#include "hdl/abi.h"
#include "hdl/exec.h"

#define CROSSTALK_EDA_IO_DISPLAY_ERR_PORT 943
#define CROSSTALK_EDA_IO_DISPLAY_ERR_LIVE 944
#define CROSSTALK_EDA_IO_DISPLAY_ERR_MODE 945

#define CROSSTALK_EDA_IO_DISPLAY_W_MAX 640
#define CROSSTALK_EDA_IO_DISPLAY_H_MAX 480
#define CROSSTALK_EDA_IO_DISPLAY_PX_MAX (CROSSTALK_EDA_IO_DISPLAY_W_MAX * CROSSTALK_EDA_IO_DISPLAY_H_MAX)
#define CROSSTALK_EDA_IO_DISPLAY_BUFF_CUNT 2

#define CROSSTALK_EDA_IO_DISPLAY_PIN_HSYNC 0
#define CROSSTALK_EDA_IO_DISPLAY_PIN_VSYNC 1
#define CROSSTALK_EDA_IO_DISPLAY_PIN_VIDEO 2
#define CROSSTALK_EDA_IO_DISPLAY_PIN_CUNT 3

#define CROSSTALK_EDA_IO_DISPLAY_MODE_MONO 0
#define CROSSTALK_EDA_IO_DISPLAY_MODE_CGA 1
#define CROSSTALK_EDA_IO_DISPLAY_MODE_VGA 2

#define CROSSTALK_EDA_IO_DISPLAY_KIND_CRT 0
#define CROSSTALK_EDA_IO_DISPLAY_KIND_PLASMA 1
#define CROSSTALK_EDA_IO_DISPLAY_KIND_NAME_CRT "CRT"
#define CROSSTALK_EDA_IO_DISPLAY_KIND_NAME_PLASMA "gas-plasma"

#define CROSSTALK_EDA_IO_DISPLAY_PORT_CAP 64
#define CROSSTALK_EDA_IO_DISPLAY_SYNC_LOW 0
#define CROSSTALK_EDA_IO_DISPLAY_LIT 255

typedef struct crosstalk_eda_io_display_pin
{
    uint64_t *abi_vals;
    char port[CROSSTALK_EDA_IO_DISPLAY_PORT_CAP];
    uint32_t sig_idx;
    uint32_t abi_sig;
    uint8_t abi_ck;
    uint8_t bound;
    uint8_t pudding[6];
} crosstalk_eda_io_display_pin_t;

typedef struct crosstalk_eda_io_display_ctx
{
    uint8_t px[CROSSTALK_EDA_IO_DISPLAY_BUFF_CUNT][CROSSTALK_EDA_IO_DISPLAY_PX_MAX];
    crosstalk_eda_io_display_pin_t pin[CROSSTALK_EDA_IO_DISPLAY_PIN_CUNT];
    uint64_t frames;
    uint32_t front;
    uint32_t slot_idx;
    uint32_t mode;
    uint32_t h_pos;
    uint32_t v_pos;
    uint32_t w_seen;
    uint32_t h_seen;
    uint32_t sync_active;
    uint8_t prev_hs;
    uint8_t prev_vs;
    uint8_t live;
    uint8_t kind;
    uint8_t pudding[4];
} crosstalk_eda_io_display_ctx_t;

uint32_t crosstalk_eda_io_display_init(crosstalk_eda_io_display_ctx_t *ctx);
uint32_t crosstalk_eda_io_display_attach(crosstalk_eda_io_display_ctx_t *ctx, uint32_t slot_idx, const crosstalk_hdl_exec_ctx_t *exec, uint32_t mode, const char *hsync, const char *vsync, const char *video);
uint32_t crosstalk_eda_io_display_detach(crosstalk_eda_io_display_ctx_t *ctx);
uint32_t crosstalk_eda_io_display_abi_rst(crosstalk_eda_io_display_ctx_t *ctx, uint32_t slot_idx);
uint32_t crosstalk_eda_io_display_cycle_sample(crosstalk_eda_io_display_ctx_t *ctx, uint32_t slot_idx, const crosstalk_hdl_exec_ctx_t *exec, const crosstalk_hdl_abi_t *abi);
uint32_t crosstalk_eda_io_display_front_get(const crosstalk_eda_io_display_ctx_t *ctx, const uint8_t **out_px, uint32_t *out_w, uint32_t *out_h);
uint64_t crosstalk_eda_io_display_frames_get(const crosstalk_eda_io_display_ctx_t *ctx);
uint8_t crosstalk_eda_io_display_live_is(const crosstalk_eda_io_display_ctx_t *ctx);
const char* crosstalk_eda_io_display_kind_name_get(const crosstalk_eda_io_display_ctx_t *ctx);

#endif
