#ifndef CROSSTALK_EDA_UI_WSHOP_BROWSER_H
#define CROSSTALK_EDA_UI_WSHOP_BROWSER_H

#include <stdint.h>

#include "ui/window.h"
#include "wshop/item.h"

#define CROSSTALK_EDA_UI_WSHOP_BROWSER_WINDOW_ID 0x57534850ULL

#define CROSSTALK_EDA_UI_WSHOP_BROWSER_LABEL_CAP 96

typedef struct crosstalk_eda_ui_wshop_browser_ctx
{
    crosstalk_ui_window_state_t win;
    const crosstalk_wshop_list_t *list;
    const crosstalk_wshop_item_t *mounted;
    const crosstalk_wshop_item_t *out_mount;
    uint32_t row_item[CROSSTALK_WSHOP_ITEM_MAX];
    char row_label[CROSSTALK_WSHOP_ITEM_MAX][CROSSTALK_EDA_UI_WSHOP_BROWSER_LABEL_CAP];
    uint32_t rows_cunt;
    uint32_t active;
    uint32_t kind;
    uint32_t seen_cunt;
    uint64_t seen_mount_id;
    uint8_t open;
    uint8_t dirty;
    uint8_t out_share_req;
    uint8_t pudding[5];
} crosstalk_eda_ui_wshop_browser_ctx_t;

uint32_t crosstalk_eda_ui_wshop_browser_init(crosstalk_eda_ui_wshop_browser_ctx_t *ctx);
uint32_t crosstalk_eda_ui_wshop_browser_open_set(crosstalk_eda_ui_wshop_browser_ctx_t *ctx, uint8_t open);
uint32_t crosstalk_eda_ui_wshop_browser_win_reg(crosstalk_eda_ui_wshop_browser_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot);
uint32_t crosstalk_eda_ui_wshop_browser_frame_set(crosstalk_eda_ui_wshop_browser_ctx_t *ctx, const crosstalk_wshop_list_t *list, const crosstalk_wshop_item_t *mounted);
uint32_t crosstalk_eda_ui_wshop_browser_out_get(crosstalk_eda_ui_wshop_browser_ctx_t *ctx, const crosstalk_wshop_item_t **out_mount, uint8_t *out_share_req);

#endif
