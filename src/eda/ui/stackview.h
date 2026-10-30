#ifndef CROSSTALK_EDA_UI_STACKVIEW_H
#define CROSSTALK_EDA_UI_STACKVIEW_H

#include <stdint.h>

#include "eda/ic/fpga/fpga.h"
#include "eda/logic/probe.h"
#include "ui/window.h"

#define CROSSTALK_EDA_UI_STACKVIEW_WINDOW_ID 0x53544B56ULL
#define CROSSTALK_EDA_UI_STACKVIEW_WINDOW_TITLE "Stack"
#define CROSSTALK_EDA_UI_STACKVIEW_ROW_CAP 64
#define CROSSTALK_EDA_UI_STACKVIEW_LANE_CAP 4
#define CROSSTALK_EDA_UI_STACKVIEW_LANE_HIST_CAP 256
#define CROSSTALK_EDA_UI_STACKVIEW_LANE_SHOW_CAP 32
#define CROSSTALK_EDA_UI_STACKVIEW_SIG_CAP 64
#define CROSSTALK_EDA_UI_STACKVIEW_QUERY_CAP 64
#define CROSSTALK_EDA_UI_STACKVIEW_FAULT_CUNT 8
#define CROSSTALK_EDA_UI_STACKVIEW_RING_CAP (CROSSTALK_HDL_ABI_RING_ENTRY_MAX * CROSSTALK_HDL_ABI_RING_STRIDE)
#define CROSSTALK_EDA_UI_STACKVIEW_MEM_SIG "cells"
#define CROSSTALK_EDA_UI_STACKVIEW_SP_SIG "dr15"

typedef enum crosstalk_eda_ui_stackview_req
{
    CROSSTALK_EDA_UI_STACKVIEW_REQ_NONE = 0,
    CROSSTALK_EDA_UI_STACKVIEW_REQ_ADD,
    CROSSTALK_EDA_UI_STACKVIEW_REQ_REMOVE
} crosstalk_eda_ui_stackview_req_t;

typedef struct crosstalk_eda_ui_stackview_run
{
    uint64_t from;
    uint64_t to;
    uint64_t instr;
    uint64_t sp;
    uint32_t cunt;
    uint8_t pudding[4];
} crosstalk_eda_ui_stackview_run_t;

typedef struct crosstalk_eda_ui_stackview_lane
{
    char name[CROSSTALK_EDA_UI_STACKVIEW_SIG_CAP];
    uint64_t cycle[CROSSTALK_EDA_UI_STACKVIEW_LANE_HIST_CAP];
    uint64_t val[CROSSTALK_EDA_UI_STACKVIEW_LANE_HIST_CAP];
    uint32_t probe_handle;
    uint32_t hd;
    uint32_t cunt;
    uint8_t live;
    uint8_t pudding[3];
} crosstalk_eda_ui_stackview_lane_t;

typedef struct crosstalk_eda_ui_stackview_ctx
{
    crosstalk_ui_window_state_t win;
    const crosstalk_eda_ic_fpga_ctx_t *fpga;
    crosstalk_eda_logic_probe_ctx_t *probes;
    crosstalk_eda_ui_stackview_lane_t lane[CROSSTALK_EDA_UI_STACKVIEW_LANE_CAP];
    uint64_t word[CROSSTALK_EDA_UI_STACKVIEW_ROW_CAP];
    uint8_t call_mark[CROSSTALK_EDA_UI_STACKVIEW_ROW_CAP];
    uint64_t fault[CROSSTALK_EDA_UI_STACKVIEW_FAULT_CUNT];
    uint64_t ring[CROSSTALK_EDA_UI_STACKVIEW_RING_CAP];
    crosstalk_eda_ui_stackview_run_t run[CROSSTALK_HDL_ABI_RING_ENTRY_MAX];
    uint32_t ring_cunt;
    uint32_t run_cunt;
    uint64_t sp;
    uint64_t stack_base;
    char query[CROSSTALK_EDA_UI_STACKVIEW_QUERY_CAP];
    char req_sig[CROSSTALK_EDA_UI_STACKVIEW_SIG_CAP];
    char mem_sig[CROSSTALK_EDA_UI_STACKVIEW_SIG_CAP];
    uint32_t query_len;
    uint32_t req;
    uint32_t word_cunt;
    uint32_t lane_cunt;
    uint32_t slot;
    uint8_t frozen;
    uint8_t capture_want;
    uint8_t fault_has;
    uint8_t open;
    uint8_t ok;
    uint8_t pudding[7];
} crosstalk_eda_ui_stackview_ctx_t;

uint32_t crosstalk_eda_ui_stackview_init(crosstalk_eda_ui_stackview_ctx_t *ctx);
uint32_t crosstalk_eda_ui_stackview_bind(crosstalk_eda_ui_stackview_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, crosstalk_eda_logic_probe_ctx_t *probes, uint32_t slot);
uint32_t crosstalk_eda_ui_stackview_open_set(crosstalk_eda_ui_stackview_ctx_t *ctx, uint8_t open);
uint32_t crosstalk_eda_ui_stackview_mem_set(crosstalk_eda_ui_stackview_ctx_t *ctx, const char *sig);
uint8_t crosstalk_eda_ui_stackview_open_is(const crosstalk_eda_ui_stackview_ctx_t *ctx);
uint32_t crosstalk_eda_ui_stackview_fault_set(crosstalk_eda_ui_stackview_ctx_t *ctx, const uint64_t *rec, uint32_t cunt);
uint32_t crosstalk_eda_ui_stackview_rst(crosstalk_eda_ui_stackview_ctx_t *ctx);
uint32_t crosstalk_eda_ui_stackview_req_get(const crosstalk_eda_ui_stackview_ctx_t *ctx, char *out_sig, uint64_t out_size, uint32_t *out_kind);
uint32_t crosstalk_eda_ui_stackview_req_clear(crosstalk_eda_ui_stackview_ctx_t *ctx);
uint32_t crosstalk_eda_ui_stackview_lane_add(crosstalk_eda_ui_stackview_ctx_t *ctx, const char *name, uint32_t probe_handle);
uint32_t crosstalk_eda_ui_stackview_lane_handle_get(const crosstalk_eda_ui_stackview_ctx_t *ctx, const char *name, uint32_t *out);
uint32_t crosstalk_eda_ui_stackview_lane_remove(crosstalk_eda_ui_stackview_ctx_t *ctx, const char *name);
uint32_t crosstalk_eda_ui_stackview_win_reg(crosstalk_eda_ui_stackview_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot);

#endif
