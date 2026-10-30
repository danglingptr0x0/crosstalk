#ifndef CROSSTALK_EDA_UI_ANALYZER_H
#define CROSSTALK_EDA_UI_ANALYZER_H

#include <stdint.h>

#include "eda/logic/probe.h"
#include "ui/window.h"

#define CROSSTALK_EDA_UI_ANALYZER_WINDOW_ID 0x414E4C5AULL
#define CROSSTALK_EDA_UI_ANALYZER_WINDOW_TITLE "Logic analyzer"

#define CROSSTALK_EDA_UI_ANALYZER_LANE_CAP 4
#define CROSSTALK_EDA_UI_ANALYZER_EDGE_CAP 512
#define CROSSTALK_EDA_UI_ANALYZER_NAME_CAP 64
#define CROSSTALK_EDA_UI_ANALYZER_SPAN_DEFAULT 2048
#define CROSSTALK_EDA_UI_ANALYZER_SPAN_MIN 16
#define CROSSTALK_EDA_UI_ANALYZER_SPAN_MAX 1048576

typedef struct crosstalk_eda_ui_analyzer_lane
{
    uint64_t edge_cycle[CROSSTALK_EDA_UI_ANALYZER_EDGE_CAP];
    uint64_t edge_val[CROSSTALK_EDA_UI_ANALYZER_EDGE_CAP];
    char name[CROSSTALK_EDA_UI_ANALYZER_NAME_CAP];
    uint64_t last_val;
    uint32_t hd;
    uint32_t cunt;
    uint32_t probe_handle;
    uint32_t width;
    uint8_t live;
    uint8_t pudding[7];
} crosstalk_eda_ui_analyzer_lane_t;

typedef struct crosstalk_eda_ui_analyzer_ctx
{
    crosstalk_eda_ui_analyzer_lane_t lane[CROSSTALK_EDA_UI_ANALYZER_LANE_CAP];
    crosstalk_ui_window_state_t win;
    crosstalk_eda_logic_probe_ctx_t *probes;
    uint64_t span;
    uint32_t lane_cunt;
    uint8_t open;
    uint8_t ok;
    uint8_t pudding[2];
} crosstalk_eda_ui_analyzer_ctx_t;

uint32_t crosstalk_eda_ui_analyzer_init(crosstalk_eda_ui_analyzer_ctx_t *ctx);
uint32_t crosstalk_eda_ui_analyzer_bind(crosstalk_eda_ui_analyzer_ctx_t *ctx, crosstalk_eda_logic_probe_ctx_t *probes);
uint32_t crosstalk_eda_ui_analyzer_lane_add(crosstalk_eda_ui_analyzer_ctx_t *ctx, const char *name, uint32_t probe_handle, uint32_t width);
uint32_t crosstalk_eda_ui_analyzer_lane_handle_get(const crosstalk_eda_ui_analyzer_ctx_t *ctx, uint32_t lane_idx, uint32_t *out);
uint32_t crosstalk_eda_ui_analyzer_lane_cunt_get(const crosstalk_eda_ui_analyzer_ctx_t *ctx, uint32_t *out);
uint32_t crosstalk_eda_ui_analyzer_rst(crosstalk_eda_ui_analyzer_ctx_t *ctx);
uint32_t crosstalk_eda_ui_analyzer_span_set(crosstalk_eda_ui_analyzer_ctx_t *ctx, uint64_t span);
uint32_t crosstalk_eda_ui_analyzer_open_set(crosstalk_eda_ui_analyzer_ctx_t *ctx, uint8_t open);
uint8_t crosstalk_eda_ui_analyzer_open_is(const crosstalk_eda_ui_analyzer_ctx_t *ctx);
uint32_t crosstalk_eda_ui_analyzer_win_reg(crosstalk_eda_ui_analyzer_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot);

#endif
