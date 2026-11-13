#ifndef CROSSTALK_NET_PRESENCE_H
#define CROSSTALK_NET_PRESENCE_H

#include <stdint.h>

#define CROSSTALK_NET_PRESENCE_BACKEND_CAP 4
#define CROSSTALK_NET_PRESENCE_PERIOD_MS 30000
#define CROSSTALK_NET_PRESENCE_PUDDING_SIZE 7
#define CROSSTALK_NET_PRESENCE_DESC_PUDDING_SIZE 4

typedef enum crosstalk_net_presence_screen
{
    CROSSTALK_NET_PRESENCE_SCREEN_MENU = 0,
    CROSSTALK_NET_PRESENCE_SCREEN_SETTINGS,
    CROSSTALK_NET_PRESENCE_SCREEN_NEWDESIGN,
    CROSSTALK_NET_PRESENCE_SCREEN_NEWPROJ,
    CROSSTALK_NET_PRESENCE_SCREEN_LD,
    CROSSTALK_NET_PRESENCE_SCREEN_BROWSE,
    CROSSTALK_NET_PRESENCE_SCREEN_EXTRAS,
    CROSSTALK_NET_PRESENCE_SCREEN_CREDITS,
    CROSSTALK_NET_PRESENCE_SCREEN_SANDBOX,
    CROSSTALK_NET_PRESENCE_SCREEN_LOBBY
} crosstalk_net_presence_screen_t;

typedef struct crosstalk_net_presence_desc
{
    const char *op;
    const char *att;
    const char *state;
    const char *music;
    uint64_t lobby_id;
    uint64_t group_id;
    uint64_t connect_lobby_id;
    uint32_t screen;
    uint32_t members;
    uint32_t members_max;
    uint8_t proj_open;
    uint8_t paused;
    uint8_t proj_reverse;
    uint8_t transponder_off;
    uint8_t pudding[CROSSTALK_NET_PRESENCE_DESC_PUDDING_SIZE];
} crosstalk_net_presence_desc_t;

typedef uint32_t (*crosstalk_net_presence_push_cb_t)(void *backend_ctx, const crosstalk_net_presence_desc_t *desc);

typedef struct crosstalk_net_presence_backend
{
    void *backend_ctx;
    crosstalk_net_presence_push_cb_t push;
} crosstalk_net_presence_backend_t;

typedef struct crosstalk_net_presence_ctx
{
    crosstalk_net_presence_backend_t backend[CROSSTALK_NET_PRESENCE_BACKEND_CAP];
    uint64_t next_ms;
    uint64_t lobby_id_prev;
    uint64_t connect_lobby_id_prev;
    uint32_t backend_cunt;
    uint32_t screen_prev;
    uint32_t members_prev;
    uint32_t members_max_prev;
    uint8_t transponder_off_prev;
    uint8_t pudding[CROSSTALK_NET_PRESENCE_PUDDING_SIZE];
} crosstalk_net_presence_ctx_t;

uint32_t crosstalk_net_presence_init(crosstalk_net_presence_ctx_t *ctx);
uint32_t crosstalk_net_presence_reg(crosstalk_net_presence_ctx_t *ctx, void *backend_ctx, crosstalk_net_presence_push_cb_t push);
uint32_t crosstalk_net_presence_unreg(crosstalk_net_presence_ctx_t *ctx, void *backend_ctx);
uint32_t crosstalk_net_presence_push(crosstalk_net_presence_ctx_t *ctx, const crosstalk_net_presence_desc_t *desc);

#endif
