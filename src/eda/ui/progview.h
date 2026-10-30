#ifndef CROSSTALK_EDA_UI_PROGVIEW_H
#define CROSSTALK_EDA_UI_PROGVIEW_H

#include <stdint.h>

#include "eda/ic/fpga/fpga.h"
#include "ui/window.h"

#define CROSSTALK_EDA_UI_PROGVIEW_WINDOW_ID 0x50524F47ULL
#define CROSSTALK_EDA_UI_PROGVIEW_WINDOW_TITLE "Program"

#define CROSSTALK_EDA_UI_PROGVIEW_ROW_CUNT 24
#define CROSSTALK_EDA_UI_PROGVIEW_SIG_CAP 64
#define CROSSTALK_EDA_UI_PROGVIEW_ASM_CAP 48

typedef struct crosstalk_eda_ui_progview_ctx
{
    crosstalk_ui_window_state_t win;
    const crosstalk_eda_ic_fpga_ctx_t *fpga;
    char sig[CROSSTALK_EDA_UI_PROGVIEW_SIG_CAP];
    char loc_sig[CROSSTALK_EDA_UI_PROGVIEW_SIG_CAP];
    uint32_t slot;
    uint8_t open;
    uint8_t ok;
    uint8_t pudding[2];
} crosstalk_eda_ui_progview_ctx_t;

uint32_t crosstalk_eda_ui_progview_init(crosstalk_eda_ui_progview_ctx_t *ctx);
uint32_t crosstalk_eda_ui_progview_shutdown(crosstalk_eda_ui_progview_ctx_t *ctx);
uint32_t crosstalk_eda_ui_progview_bind(crosstalk_eda_ui_progview_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot);
uint32_t crosstalk_eda_ui_progview_sig_set(crosstalk_eda_ui_progview_ctx_t *ctx, const char *sig_name);
uint32_t crosstalk_eda_ui_progview_open_set(crosstalk_eda_ui_progview_ctx_t *ctx, uint8_t open);
uint8_t crosstalk_eda_ui_progview_open_is(const crosstalk_eda_ui_progview_ctx_t *ctx);
uint32_t crosstalk_eda_ui_progview_win_reg(crosstalk_eda_ui_progview_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot);

#endif
