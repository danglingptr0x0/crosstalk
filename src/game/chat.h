#ifndef CROSSTALK_GAME_CHAT_H
#define CROSSTALK_GAME_CHAT_H

#include <stdint.h>

#include "ui/window.h"

#define CROSSTALK_GAME_CHAT_WINDOW_ID 7
#define CROSSTALK_GAME_CHAT_TEXT_CAP 240
#define CROSSTALK_GAME_CHAT_NAME_CAP 64
#define CROSSTALK_GAME_CHAT_CONV_CAP 12
#define CROSSTALK_GAME_CHAT_HISTORY_CAP 48
#define CROSSTALK_GAME_CHAT_PEER_NONE 0

typedef enum crosstalk_game_chat_group
{
    CROSSTALK_GAME_CHAT_GROUP_SESH = 0,
    CROSSTALK_GAME_CHAT_GROUP_STEAM,
    CROSSTALK_GAME_CHAT_GROUP_DISCORD,
    CROSSTALK_GAME_CHAT_GROUP_CUNT
} crosstalk_game_chat_group_t;

typedef struct crosstalk_game_chat_msg
{
    char name[CROSSTALK_GAME_CHAT_NAME_CAP];
    char text[CROSSTALK_GAME_CHAT_TEXT_CAP];
} crosstalk_game_chat_msg_t;

typedef struct crosstalk_game_chat_conv
{
    crosstalk_game_chat_msg_t msg[CROSSTALK_GAME_CHAT_HISTORY_CAP];
    char name[CROSSTALK_GAME_CHAT_NAME_CAP];
    uint64_t peer_id;
    uint32_t hd;
    uint32_t cunt;
    uint8_t group;
    uint8_t unseen;
    uint8_t live;
    uint8_t reach;
    uint8_t pudding[4];
} crosstalk_game_chat_conv_t;

typedef struct crosstalk_game_chat_ctx
{
    crosstalk_ui_window_state_t win;
    crosstalk_ui_window_ctx_t *win_nav;
    char out_text[CROSSTALK_GAME_CHAT_TEXT_CAP];
    crosstalk_game_chat_conv_t conv[CROSSTALK_GAME_CHAT_CONV_CAP];
    char draft[CROSSTALK_GAME_CHAT_TEXT_CAP];
    uint32_t conv_sel[CROSSTALK_GAME_CHAT_GROUP_CUNT];
    uint32_t conv_cunt;
    uint32_t line_seen;
    uint8_t group;
    uint8_t group_seen;
    uint8_t out_send;
    uint8_t sesh_live;
    uint8_t discord_live;
    uint8_t unseen;
    uint8_t pudding[2];
} crosstalk_game_chat_ctx_t;

uint32_t crosstalk_game_chat_init(crosstalk_game_chat_ctx_t *ctx);
uint32_t crosstalk_game_chat_open_set(crosstalk_game_chat_ctx_t *ctx, uint8_t open);
uint32_t crosstalk_game_chat_rst(crosstalk_game_chat_ctx_t *ctx);
uint32_t crosstalk_game_chat_conv_get(crosstalk_game_chat_ctx_t *ctx, uint8_t group, uint64_t peer_id, const char *name, uint32_t *out_idx);
uint32_t crosstalk_game_chat_conv_sel(crosstalk_game_chat_ctx_t *ctx, uint8_t group, uint64_t peer_id, const char *name);
uint32_t crosstalk_game_chat_conv_reach_set(crosstalk_game_chat_ctx_t *ctx, uint8_t group, uint64_t peer_id, uint8_t reach);
uint32_t crosstalk_game_chat_push(crosstalk_game_chat_ctx_t *ctx, uint8_t group, uint64_t peer_id, const char *conv_name, const char *name, const char *text);
uint64_t crosstalk_game_chat_peer_get(const crosstalk_game_chat_ctx_t *ctx, uint8_t group);
uint32_t crosstalk_game_chat_win_reg(crosstalk_game_chat_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot);
uint32_t crosstalk_game_chat_frame_set(crosstalk_game_chat_ctx_t *ctx, uint8_t sesh_live, uint8_t discord_live);
uint32_t crosstalk_game_chat_out_get(crosstalk_game_chat_ctx_t *ctx, char *out_text, uint32_t out_cap, uint8_t *out_send);

#endif
