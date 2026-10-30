#ifndef CROSSTALK_EDA_UI_PIPEVIEW_H
#define CROSSTALK_EDA_UI_PIPEVIEW_H

#include <stdint.h>

#include "eda/ic/fpga/fpga.h"
#include "ui/window.h"

#define CROSSTALK_EDA_UI_PIPEVIEW_WINDOW_ID 0x50495045ULL
#define CROSSTALK_EDA_UI_PIPEVIEW_WINDOW_TITLE "Pipeline"

#define CROSSTALK_EDA_UI_PIPEVIEW_STAGE_CAP 8
#define CROSSTALK_EDA_UI_PIPEVIEW_SIG_CAP 64

typedef struct crosstalk_eda_ui_pipeview_ctx
{
    crosstalk_ui_window_state_t win;
    const crosstalk_eda_ic_fpga_ctx_t *fpga;
    char stage_sig[CROSSTALK_EDA_UI_PIPEVIEW_STAGE_CAP][CROSSTALK_EDA_UI_PIPEVIEW_SIG_CAP];
    uint64_t prev[CROSSTALK_EDA_UI_PIPEVIEW_STAGE_CAP];
    uint32_t flash[CROSSTALK_EDA_UI_PIPEVIEW_STAGE_CAP];
    uint32_t stage_cunt;
    uint32_t slot;
    uint8_t open;
    uint8_t ok;
    uint8_t pudding[6];
} crosstalk_eda_ui_pipeview_ctx_t;

uint32_t crosstalk_eda_ui_pipeview_init(crosstalk_eda_ui_pipeview_ctx_t *ctx);
uint32_t crosstalk_eda_ui_pipeview_bind(crosstalk_eda_ui_pipeview_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot);
uint32_t crosstalk_eda_ui_pipeview_stages_set(crosstalk_eda_ui_pipeview_ctx_t *ctx, const char *args);
uint32_t crosstalk_eda_ui_pipeview_open_set(crosstalk_eda_ui_pipeview_ctx_t *ctx, uint8_t open);
uint8_t crosstalk_eda_ui_pipeview_open_is(const crosstalk_eda_ui_pipeview_ctx_t *ctx);
uint32_t crosstalk_eda_ui_pipeview_win_reg(crosstalk_eda_ui_pipeview_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot);

#endif
