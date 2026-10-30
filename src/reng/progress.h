#ifndef CROSSTALK_RENG_PROGRESS_H
#define CROSSTALK_RENG_PROGRESS_H

#include <stdint.h>

#include "reng/reng.h"
#include "ui/window.h"

#define CROSSTALK_RENG_PROGRESS_ID 0x50524753ULL
#define CROSSTALK_RENG_PROGRESS_TITLE "Progress"

typedef struct crosstalk_reng_progress_ctx
{
    crosstalk_ui_window_state_t win;
    crosstalk_reng_ctx_t *reng;
    uint8_t ok;
    uint8_t pudding[7];
} crosstalk_reng_progress_ctx_t;

uint32_t crosstalk_reng_progress_init(crosstalk_reng_progress_ctx_t *ctx, crosstalk_reng_ctx_t *reng);
uint32_t crosstalk_reng_progress_reg(crosstalk_reng_progress_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot);
uint32_t crosstalk_reng_progress_open_set(crosstalk_reng_progress_ctx_t *ctx, uint8_t open);
uint8_t crosstalk_reng_progress_open_is(const crosstalk_reng_progress_ctx_t *ctx);

#endif
