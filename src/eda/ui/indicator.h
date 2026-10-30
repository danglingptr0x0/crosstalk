#ifndef CROSSTALK_EDA_UI_INDICATOR_H
#define CROSSTALK_EDA_UI_INDICATOR_H

#include <stdint.h>

#include "eda/logic/stream.h"
#include "ui/window.h"
#include "ui/theme.h"

#define CROSSTALK_EDA_UI_INDICATOR_ERR_WIDTH 948
#define CROSSTALK_EDA_UI_INDICATOR_ERR_SRC 949

#define CROSSTALK_EDA_UI_INDICATOR_BITS_MAX 32
#define CROSSTALK_EDA_UI_INDICATOR_PORT_CAP 64
#define CROSSTALK_EDA_UI_INDICATOR_STATUS_CAP 96
#define CROSSTALK_EDA_UI_INDICATOR_BIT_CAP 8

#define CROSSTALK_EDA_UI_INDICATOR_WINDOW_ID 0x494E4443ULL
#define CROSSTALK_EDA_UI_INDICATOR_WINDOW_TITLE "Indicators"
#define CROSSTALK_EDA_UI_INDICATOR_LAMP_GAP 1
#define CROSSTALK_EDA_UI_INDICATOR_LAMP_OFF_COL 0x1E2A22FFU
#define CROSSTALK_EDA_UI_INDICATOR_IDLE_TEXT "no signal attached"
#define CROSSTALK_EDA_UI_INDICATOR_BIT_STEP 1

#define CROSSTALK_EDA_UI_INDICATOR_ACC_CUNT 4
#define CROSSTALK_EDA_UI_INDICATOR_ACC_LABEL "data"
#define CROSSTALK_EDA_UI_INDICATOR_ACC_RAM_RD_TEXT "rd"
#define CROSSTALK_EDA_UI_INDICATOR_ACC_RAM_WR_TEXT "wr"
#define CROSSTALK_EDA_UI_INDICATOR_ACC_MMIO_RD_TEXT "io rd"
#define CROSSTALK_EDA_UI_INDICATOR_ACC_MMIO_WR_TEXT "io wr"
#define CROSSTALK_EDA_UI_INDICATOR_ACC_LAMP_W 6
#define CROSSTALK_EDA_UI_INDICATOR_ACC_ON_COL 0xE8A93BFFU
#define CROSSTALK_EDA_UI_INDICATOR_ACC_OFF_COL 0x2A2418FFU

typedef struct crosstalk_eda_ui_indicator_ctx
{
    crosstalk_ui_window_state_t win;
    const crosstalk_eda_logic_stream_ctx_t *streams;
    char port[CROSSTALK_EDA_UI_INDICATOR_PORT_CAP];
    uint64_t acc_prev[CROSSTALK_EDA_UI_INDICATOR_ACC_CUNT];
    uint32_t stream_handle;
    uint32_t width;
    uint8_t acc_lit[CROSSTALK_EDA_UI_INDICATOR_ACC_CUNT];
    uint8_t open;
    uint8_t live;
    uint8_t ok;
    uint8_t acc_on;
} crosstalk_eda_ui_indicator_ctx_t;

uint32_t crosstalk_eda_ui_indicator_init(crosstalk_eda_ui_indicator_ctx_t *ctx);
uint32_t crosstalk_eda_ui_indicator_shutdown(crosstalk_eda_ui_indicator_ctx_t *ctx);
uint32_t crosstalk_eda_ui_indicator_open_set(crosstalk_eda_ui_indicator_ctx_t *ctx, uint8_t open);
uint8_t crosstalk_eda_ui_indicator_open_is(const crosstalk_eda_ui_indicator_ctx_t *ctx);
uint32_t crosstalk_eda_ui_indicator_bind(crosstalk_eda_ui_indicator_ctx_t *ctx, uint32_t stream_handle, uint32_t width, const char *port);
uint32_t crosstalk_eda_ui_indicator_acc_set(crosstalk_eda_ui_indicator_ctx_t *ctx, const uint64_t *acc, uint32_t cunt);
uint32_t crosstalk_eda_ui_indicator_acc_clear(crosstalk_eda_ui_indicator_ctx_t *ctx);
uint32_t crosstalk_eda_ui_indicator_win_reg(crosstalk_eda_ui_indicator_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, const crosstalk_eda_logic_stream_ctx_t *streams, uint32_t *out_slot);

#endif
