#ifndef CROSSTALK_EDA_UI_WSHOP_PUBLISH_H
#define CROSSTALK_EDA_UI_WSHOP_PUBLISH_H

#include <stdint.h>

#include "ui/window.h"

#define CROSSTALK_EDA_UI_WSHOP_PUBLISH_WINDOW_ID 0x57505542ULL

#define CROSSTALK_EDA_UI_WSHOP_PUBLISH_TITLE_CAP 128
#define CROSSTALK_EDA_UI_WSHOP_PUBLISH_LICENSE_CAP 64
#define CROSSTALK_EDA_UI_WSHOP_PUBLISH_ISA_CUNT 9

typedef struct crosstalk_eda_ui_wshop_publish_ctx
{
    crosstalk_ui_window_state_t win;
    const char *status_text;
    char title[CROSSTALK_EDA_UI_WSHOP_PUBLISH_TITLE_CAP];
    char license[CROSSTALK_EDA_UI_WSHOP_PUBLISH_LICENSE_CAP];
    uint32_t isa_idx;
    uint32_t field;
    uint8_t open;
    uint8_t out_publish;
    uint8_t pudding[6];
} crosstalk_eda_ui_wshop_publish_ctx_t;

uint32_t crosstalk_eda_ui_wshop_publish_init(crosstalk_eda_ui_wshop_publish_ctx_t *ctx);
uint32_t crosstalk_eda_ui_wshop_publish_open_set(crosstalk_eda_ui_wshop_publish_ctx_t *ctx, uint8_t open, const char *seed_title);
const char* crosstalk_eda_ui_wshop_publish_isa_get(const crosstalk_eda_ui_wshop_publish_ctx_t *ctx);
uint32_t crosstalk_eda_ui_wshop_publish_win_reg(crosstalk_eda_ui_wshop_publish_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot);
uint32_t crosstalk_eda_ui_wshop_publish_frame_set(crosstalk_eda_ui_wshop_publish_ctx_t *ctx, const char *status_text);
uint32_t crosstalk_eda_ui_wshop_publish_out_get(crosstalk_eda_ui_wshop_publish_ctx_t *ctx, uint8_t *out_publish);

#endif
