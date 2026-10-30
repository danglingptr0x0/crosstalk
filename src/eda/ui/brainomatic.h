#ifndef CROSSTALK_EDA_UI_BRAINOMATIC_H
#define CROSSTALK_EDA_UI_BRAINOMATIC_H

#include <stdint.h>

#include "ui/window.h"

#define CROSSTALK_EDA_UI_BRAINOMATIC_WINDOW_ID 0x42524E4FULL
#define CROSSTALK_EDA_UI_BRAINOMATIC_WINDOW_TITLE "Brain'o'matic 3000"

typedef enum crosstalk_eda_ui_brainomatic_arch
{
    CROSSTALK_EDA_UI_BRAINOMATIC_ARCH_DANGCSA = 0x0
} crosstalk_eda_ui_brainomatic_arch_t;

typedef struct crosstalk_eda_ui_brainomatic_ctx
{
    crosstalk_ui_window_state_t win;
    uint32_t arch;
    uint8_t open;
    uint8_t ok;
    uint8_t pudding[2];
} crosstalk_eda_ui_brainomatic_ctx_t;

uint32_t crosstalk_eda_ui_brainomatic_init(crosstalk_eda_ui_brainomatic_ctx_t *ctx);
uint32_t crosstalk_eda_ui_brainomatic_arch_set(crosstalk_eda_ui_brainomatic_ctx_t *ctx, uint32_t arch);
uint32_t crosstalk_eda_ui_brainomatic_open_set(crosstalk_eda_ui_brainomatic_ctx_t *ctx, uint8_t open);
uint8_t crosstalk_eda_ui_brainomatic_open_is(const crosstalk_eda_ui_brainomatic_ctx_t *ctx);
uint32_t crosstalk_eda_ui_brainomatic_win_reg(crosstalk_eda_ui_brainomatic_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot);

#endif
