#ifndef CROSSTALK_EDA_UI_REGVIEW_H
#define CROSSTALK_EDA_UI_REGVIEW_H

#include <stdint.h>

#include "eda/ic/fpga/fpga.h"
#include "ui/window.h"

#define CROSSTALK_EDA_UI_REGVIEW_ROW_CAP 20
#define CROSSTALK_EDA_UI_REGVIEW_REG_CUNT 16

#define CROSSTALK_EDA_UI_REGVIEW_WINDOW_ID 0x52454753ULL
#define CROSSTALK_EDA_UI_REGVIEW_WINDOW_TITLE "Registers"

typedef struct crosstalk_eda_ui_regview_ctx
{
    crosstalk_ui_window_state_t win;
    const crosstalk_eda_ic_fpga_ctx_t *fpga;
    uint64_t prev[CROSSTALK_EDA_UI_REGVIEW_ROW_CAP];
    uint32_t flash[CROSSTALK_EDA_UI_REGVIEW_ROW_CAP];
    uint32_t slot;
    uint8_t open;
    uint8_t ok;
    uint8_t miss_logged;
    uint8_t pudding[1];
} crosstalk_eda_ui_regview_ctx_t;

uint32_t crosstalk_eda_ui_regview_init(crosstalk_eda_ui_regview_ctx_t *ctx);
uint32_t crosstalk_eda_ui_regview_bind(crosstalk_eda_ui_regview_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot);
uint32_t crosstalk_eda_ui_regview_open_set(crosstalk_eda_ui_regview_ctx_t *ctx, uint8_t open);
uint8_t crosstalk_eda_ui_regview_open_is(const crosstalk_eda_ui_regview_ctx_t *ctx);
uint32_t crosstalk_eda_ui_regview_win_reg(crosstalk_eda_ui_regview_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot);

#endif
