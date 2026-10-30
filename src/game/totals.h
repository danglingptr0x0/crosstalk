#ifndef CROSSTALK_GAME_TOTALS_H
#define CROSSTALK_GAME_TOTALS_H

#include <stdint.h>

#include "eda/ic/fpga/emu.h"
#include "ui/window.h"

#define CROSSTALK_GAME_TOTALS_WINDOW_ID 10

typedef struct crosstalk_game_totals_info
{
    const char *board_name;
    const char *author_name;
    const crosstalk_eda_ic_fpga_emu_stat_t *emu_stat;
    uint64_t created_epoch_ms;
    uint64_t bench_ms_total;
    uint64_t sim_ms_total;
} crosstalk_game_totals_info_t;

typedef struct crosstalk_game_totals_ctx
{
    crosstalk_ui_window_state_t win;
    crosstalk_game_totals_info_t info;
    uint8_t open;
    uint8_t pudding[7];
} crosstalk_game_totals_ctx_t;

uint32_t crosstalk_game_totals_init(crosstalk_game_totals_ctx_t *ctx);
uint32_t crosstalk_game_totals_open_set(crosstalk_game_totals_ctx_t *ctx, uint8_t open);
uint32_t crosstalk_game_totals_win_reg(crosstalk_game_totals_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot);
uint32_t crosstalk_game_totals_frame_set(crosstalk_game_totals_ctx_t *ctx, const crosstalk_game_totals_info_t *info);

#endif
