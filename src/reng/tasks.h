#ifndef CROSSTALK_RENG_TASKS_H
#define CROSSTALK_RENG_TASKS_H

#include <stdint.h>

#include "reng/reng.h"
#include "ui/window.h"

#define CROSSTALK_RENG_TASKS_ID 0x5441534BULL
#define CROSSTALK_RENG_TASKS_TITLE "Tasks"
#define CROSSTALK_RENG_TASKS_LABEL_CAP 176
#define CROSSTALK_RENG_TASKS_ROWS_MAX 256

typedef struct crosstalk_reng_tasks_ctx
{
    crosstalk_ui_window_state_t win;
    crosstalk_reng_ctx_t *reng;
    char row_label[CROSSTALK_RENG_TASKS_ROWS_MAX][CROSSTALK_RENG_TASKS_LABEL_CAP];
    char entry[CROSSTALK_RENG_TEXT_CAP];
    uint32_t row_idx[CROSSTALK_RENG_TASKS_ROWS_MAX];
    uint32_t rows_cunt;
    uint32_t active;
    uint32_t gen_seen;
    uint8_t ok;
    uint8_t pudding[3];
} crosstalk_reng_tasks_ctx_t;

uint32_t crosstalk_reng_tasks_init(crosstalk_reng_tasks_ctx_t *ctx, crosstalk_reng_ctx_t *reng);
uint32_t crosstalk_reng_tasks_reg(crosstalk_reng_tasks_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot);
uint32_t crosstalk_reng_tasks_open_set(crosstalk_reng_tasks_ctx_t *ctx, uint8_t open);
uint8_t crosstalk_reng_tasks_open_is(const crosstalk_reng_tasks_ctx_t *ctx);

#endif
