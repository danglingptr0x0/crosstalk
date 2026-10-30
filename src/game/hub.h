#ifndef CROSSTALK_GAME_HUB_H
#define CROSSTALK_GAME_HUB_H

#include <stdint.h>

#include "ui/window.h"
#include "ui/theme.h"

#define CROSSTALK_GAME_HUB_WINDOW_ID 8
#define CROSSTALK_GAME_HUB_COL_ONLINE 0xE8E8F0FFU
#define CROSSTALK_GAME_HUB_COL_OFFLINE 0x6E6E78FFU
#define CROSSTALK_GAME_HUB_ROW_CAP 64
#define CROSSTALK_GAME_HUB_COL_CAP 10
#define CROSSTALK_GAME_HUB_COL_CUNT 64
#define CROSSTALK_GAME_HUB_NAME_CAP 64
#define CROSSTALK_GAME_HUB_STATUS_CAP 32

typedef enum crosstalk_game_hub_src
{
    CROSSTALK_GAME_HUB_SRC_DISCORD = 0,
    CROSSTALK_GAME_HUB_SRC_STEAM,
    CROSSTALK_GAME_HUB_SRC_CUNT
} crosstalk_game_hub_src_t;

typedef enum crosstalk_game_hub_reach
{
    CROSSTALK_GAME_HUB_REACH_JOINABLE = 0,
    CROSSTALK_GAME_HUB_REACH_IN_GAME,
    CROSSTALK_GAME_HUB_REACH_OTHER_GAME,
    CROSSTALK_GAME_HUB_REACH_ONLINE,
    CROSSTALK_GAME_HUB_REACH_AWAY,
    CROSSTALK_GAME_HUB_REACH_OFFLINE,
    CROSSTALK_GAME_HUB_REACH_CUNT
} crosstalk_game_hub_reach_t;

typedef enum crosstalk_game_hub_action
{
    CROSSTALK_GAME_HUB_ACTION_NONE = 0,
    CROSSTALK_GAME_HUB_ACTION_DM,
    CROSSTALK_GAME_HUB_ACTION_INVITE,
    CROSSTALK_GAME_HUB_ACTION_JOIN
} crosstalk_game_hub_action_t;

typedef struct crosstalk_game_hub_row
{
    uint64_t user_id;
    uint64_t lobby_id;
    char name[CROSSTALK_GAME_HUB_NAME_CAP];
    char status[CROSSTALK_GAME_HUB_STATUS_CAP];
    uint8_t src;
    uint8_t reach;
    uint8_t pudding[6];
} crosstalk_game_hub_row_t;

typedef struct crosstalk_game_hub_ctx
{
    crosstalk_ui_window_state_t win;
    char out_name[CROSSTALK_GAME_HUB_NAME_CAP];
    uint64_t out_user_id;
    uint64_t out_lobby_id;
    crosstalk_game_hub_action_t out_action;
    uint8_t out_src;
    uint8_t pudding_out[3];
    crosstalk_game_hub_row_t row[CROSSTALK_GAME_HUB_ROW_CAP];
    uint32_t row_cunt;
    uint8_t sel_src;
    uint8_t sel_on;
    uint8_t pick_src;
    uint8_t pick_on;
    uint64_t sel_user_id;
} crosstalk_game_hub_ctx_t;

uint32_t crosstalk_game_hub_init(crosstalk_game_hub_ctx_t *ctx);
uint32_t crosstalk_game_hub_open_set(crosstalk_game_hub_ctx_t *ctx, uint8_t open);
uint8_t crosstalk_game_hub_open_is(const crosstalk_game_hub_ctx_t *ctx);
uint32_t crosstalk_game_hub_reach_col_get(uint8_t reach);
uint32_t crosstalk_game_hub_rows_rst(crosstalk_game_hub_ctx_t *ctx);
uint32_t crosstalk_game_hub_row_add(crosstalk_game_hub_ctx_t *ctx, uint64_t user_id, const char *name, const char *status, uint8_t src, uint8_t reach, uint64_t lobby_id);
uint32_t crosstalk_game_hub_rows_sort(crosstalk_game_hub_ctx_t *ctx);
uint32_t crosstalk_game_hub_extent_get(const crosstalk_game_hub_ctx_t *ctx, uint32_t *out_row_cunt);
uint32_t crosstalk_game_hub_win_reg(crosstalk_game_hub_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot);
uint32_t crosstalk_game_hub_out_get(crosstalk_game_hub_ctx_t *ctx, crosstalk_game_hub_action_t *out_action, uint64_t *out_user_id, uint64_t *out_lobby_id, uint8_t *out_src, char *out_name, uint32_t out_name_cap);

#endif
