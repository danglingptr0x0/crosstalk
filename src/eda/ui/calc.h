#ifndef CROSSTALK_EDA_UI_CALC_H
#define CROSSTALK_EDA_UI_CALC_H

#include <stdint.h>

#include "eda/ic/fpga/fpga.h"
#include "eda/ic/fpga/run.h"
#include "ui/window.h"

#define CROSSTALK_EDA_UI_CALC_ERR_PARSE 950
#define CROSSTALK_EDA_UI_CALC_ERR_DIV_ZERO 951
#define CROSSTALK_EDA_UI_CALC_ERR_NO_CLK 952
#define CROSSTALK_EDA_UI_CALC_ERR_IDENT 953

#define CROSSTALK_EDA_UI_CALC_EXPR_CAP 128
#define CROSSTALK_EDA_UI_CALC_LINE_CAP 128
#define CROSSTALK_EDA_UI_CALC_VAR_CAP 32
#define CROSSTALK_EDA_UI_CALC_VAR_NAME_CAP 24
#define CROSSTALK_EDA_UI_CALC_HIST_CAP 24
#define CROSSTALK_EDA_UI_CALC_HIST_EXPR_CAP 64
#define CROSSTALK_EDA_UI_CALC_HIST_TEXT_CAP 96

#define CROSSTALK_EDA_UI_CALC_WINDOW_ID 0x43414C43ULL
#define CROSSTALK_EDA_UI_CALC_WINDOW_TITLE "Calculator"

typedef struct crosstalk_eda_ui_calc_var
{
    char name[CROSSTALK_EDA_UI_CALC_VAR_NAME_CAP];
    uint64_t val;
} crosstalk_eda_ui_calc_var_t;

typedef struct crosstalk_eda_ui_calc_hist
{
    char expr[CROSSTALK_EDA_UI_CALC_HIST_EXPR_CAP];
    char text[CROSSTALK_EDA_UI_CALC_HIST_TEXT_CAP];
    uint64_t val;
    uint8_t text_is;
    uint8_t pudding[7];
} crosstalk_eda_ui_calc_hist_t;

typedef struct crosstalk_eda_ui_calc_ctx
{
    crosstalk_ui_window_state_t win;
    const crosstalk_eda_ic_fpga_ctx_t *fpga;
    const crosstalk_eda_ic_fpga_run_ctx_t *run;
    crosstalk_eda_ui_calc_var_t var[CROSSTALK_EDA_UI_CALC_VAR_CAP];
    crosstalk_eda_ui_calc_hist_t hist[CROSSTALK_EDA_UI_CALC_HIST_CAP];
    char expr[CROSSTALK_EDA_UI_CALC_EXPR_CAP];
    uint64_t res;
    uint64_t res_last;
    uint32_t var_cunt;
    uint32_t hist_cunt;
    uint32_t slot;
    uint32_t ret;
    uint8_t res_last_ok;
    uint8_t edit;
    uint8_t focused;
    uint8_t res_ok;
    uint8_t open;
    uint8_t ok;
    uint8_t pudding[2];
} crosstalk_eda_ui_calc_ctx_t;

uint32_t crosstalk_eda_ui_calc_init(crosstalk_eda_ui_calc_ctx_t *ctx);
uint32_t crosstalk_eda_ui_calc_bind(crosstalk_eda_ui_calc_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, const crosstalk_eda_ic_fpga_run_ctx_t *run, uint32_t slot);
uint32_t crosstalk_eda_ui_calc_open_set(crosstalk_eda_ui_calc_ctx_t *ctx, uint8_t open);
uint8_t crosstalk_eda_ui_calc_open_is(const crosstalk_eda_ui_calc_ctx_t *ctx);
uint32_t crosstalk_eda_ui_calc_var_get(const crosstalk_eda_ui_calc_ctx_t *ctx, const char *name, uint64_t *out);
uint32_t crosstalk_eda_ui_calc_var_at_get(const crosstalk_eda_ui_calc_ctx_t *ctx, uint64_t addr, char *out, uint32_t cap);
uint32_t crosstalk_eda_ui_calc_focus_set(crosstalk_eda_ui_calc_ctx_t *ctx, uint8_t focused);
uint32_t crosstalk_eda_ui_calc_route_set(crosstalk_eda_ui_calc_ctx_t *ctx, uint8_t focused);
uint32_t crosstalk_eda_ui_calc_state_serialize(const crosstalk_eda_ui_calc_ctx_t *ctx, uint8_t *out, uint32_t cap, uint32_t *out_len);
uint32_t crosstalk_eda_ui_calc_state_deserialize(crosstalk_eda_ui_calc_ctx_t *ctx, const uint8_t *buff, uint32_t len);
uint32_t crosstalk_eda_ui_calc_win_reg(crosstalk_eda_ui_calc_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot);

#endif
