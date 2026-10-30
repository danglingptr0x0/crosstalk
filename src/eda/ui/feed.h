#ifndef CROSSTALK_EDA_UI_FEED_H
#define CROSSTALK_EDA_UI_FEED_H

#include <stdint.h>

#include "eda/logic/probe.h"
#include "ui/window.h"

#define CROSSTALK_EDA_UI_FEED_WINDOW_ID 0x46454544ULL
#define CROSSTALK_EDA_UI_FEED_WINDOW_TITLE "Fault feed"

#define CROSSTALK_EDA_UI_FEED_LANE_CAP 4
#define CROSSTALK_EDA_UI_FEED_EVENT_CAP 128
#define CROSSTALK_EDA_UI_FEED_NAME_CAP 64
#define CROSSTALK_EDA_UI_FEED_FAULT_CUNT 8

typedef struct crosstalk_eda_ui_feed_lane
{
    char name[CROSSTALK_EDA_UI_FEED_NAME_CAP];
    uint32_t probe_handle;
    uint32_t width;
    uint8_t live;
    uint8_t pudding[7];
} crosstalk_eda_ui_feed_lane_t;

typedef struct crosstalk_eda_ui_feed_event
{
    uint64_t cycle;
    uint64_t val;
    uint32_t lane_idx;
    uint8_t pudding[4];
} crosstalk_eda_ui_feed_event_t;

typedef struct crosstalk_eda_ui_feed_ctx
{
    crosstalk_eda_ui_feed_event_t event[CROSSTALK_EDA_UI_FEED_EVENT_CAP];
    crosstalk_eda_ui_feed_lane_t lane[CROSSTALK_EDA_UI_FEED_LANE_CAP];
    crosstalk_ui_window_state_t win;
    crosstalk_eda_logic_probe_ctx_t *probes;
    uint64_t fault[CROSSTALK_EDA_UI_FEED_FAULT_CUNT];
    uint32_t event_hd;
    uint32_t event_cunt;
    uint32_t lane_cunt;
    uint8_t open;
    uint8_t ok;
    uint8_t fault_has;
    uint8_t pudding[1];
} crosstalk_eda_ui_feed_ctx_t;

uint32_t crosstalk_eda_ui_feed_init(crosstalk_eda_ui_feed_ctx_t *ctx);
uint32_t crosstalk_eda_ui_feed_bind(crosstalk_eda_ui_feed_ctx_t *ctx, crosstalk_eda_logic_probe_ctx_t *probes);
uint32_t crosstalk_eda_ui_feed_lane_add(crosstalk_eda_ui_feed_ctx_t *ctx, const char *name, uint32_t probe_handle, uint32_t width);
uint32_t crosstalk_eda_ui_feed_lane_handle_get(const crosstalk_eda_ui_feed_ctx_t *ctx, uint32_t lane_idx, uint32_t *out);
uint32_t crosstalk_eda_ui_feed_lane_cunt_get(const crosstalk_eda_ui_feed_ctx_t *ctx, uint32_t *out);
uint32_t crosstalk_eda_ui_feed_fault_set(crosstalk_eda_ui_feed_ctx_t *ctx, const uint64_t *rec, uint32_t cunt);
uint32_t crosstalk_eda_ui_feed_rst(crosstalk_eda_ui_feed_ctx_t *ctx);
uint32_t crosstalk_eda_ui_feed_open_set(crosstalk_eda_ui_feed_ctx_t *ctx, uint8_t open);
uint8_t crosstalk_eda_ui_feed_open_is(const crosstalk_eda_ui_feed_ctx_t *ctx);
uint32_t crosstalk_eda_ui_feed_win_reg(crosstalk_eda_ui_feed_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot);

#endif
