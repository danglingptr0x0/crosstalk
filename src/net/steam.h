#ifndef CROSSTALK_NET_STEAM_H
#define CROSSTALK_NET_STEAM_H

#include <stdint.h>

#include <dangling/net/steamworks.h>
#include <dangling/net/steamworks_lobby.h>
#include <dangling/net/steamworks_srv.h>
#include <dangling/net/steamworks_srv_browser.h>

#define CROSSTALK_NET_NOTICE_CAP 128
#define CROSSTALK_NET_NOTICE_RING 8
#define CROSSTALK_NET_LOBBY_MAX_MEMBERS 8
#define CROSSTALK_NET_WIRE_HDR_CAP 24
#define CROSSTALK_NET_WIRE_PAYLOAD_MAX 262144
#define CROSSTALK_NET_WIRE_BUFF_CAP (CROSSTALK_NET_WIRE_HDR_CAP + CROSSTALK_NET_WIRE_PAYLOAD_MAX)
#define CROSSTALK_NET_WIRE_CHUNK_HDR_CAP 16
#define CROSSTALK_NET_WIRE_CHUNK_FRAME 32768
#define CROSSTALK_NET_WIRE_CHUNK_MAX (CROSSTALK_NET_WIRE_CHUNK_FRAME - CROSSTALK_NET_WIRE_CHUNK_HDR_CAP)
#define CROSSTALK_NET_SNAP_CAP 16777216
#define CROSSTALK_NET_FRIEND_CAP 128
#define CROSSTALK_NET_FRIEND_NAME_CAP 64
#define CROSSTALK_NET_FRIEND_DETAIL_CAP 48
#define CROSSTALK_NET_STEAM_APP_ID 4898790U
#define CROSSTALK_NET_STEAM_APPID_MASK 0xFFFFFFFFULL
#define CROSSTALK_NET_FRIEND_DETAIL_OURS "in Crosstalk"
#define CROSSTALK_NET_FRIEND_DETAIL_MENU "in the menus"
#define CROSSTALK_NET_FRIEND_DETAIL_SANDBOX "in the sandbox"
#define CROSSTALK_NET_FRIEND_DETAIL_LOBBY "in a lobby"
#define CROSSTALK_NET_FRIEND_DETAIL_OTHER "in another game"
#define CROSSTALK_NET_FRIEND_REFRESH_INTERVAL 5.0
#define CROSSTALK_NET_CUNT_NONE 0
#define CROSSTALK_NET_LOBBY_ID_NONE 0
#define CROSSTALK_NET_LAN_PRODUCT "Crosstalk"
#define CROSSTALK_NET_LAN_MOD_DIR "crosstalk"
#define CROSSTALK_NET_LAN_DESC "Crosstalk LAN session"
#define CROSSTALK_NET_LAN_REV "1"
#define CROSSTALK_NET_LAN_HOST_MAX 64

typedef enum crosstalk_net_transport
{
    CROSSTALK_NET_TRANSPORT_ONLINE = 0,
    CROSSTALK_NET_TRANSPORT_LAN = 1
} crosstalk_net_transport_t;

typedef enum crosstalk_net_lan_sesh
{
    CROSSTALK_NET_LAN_SESH_NONE = 0,
    CROSSTALK_NET_LAN_SESH_HOSTING = 1,
    CROSSTALK_NET_LAN_SESH_JOINED = 2
} crosstalk_net_lan_sesh_t;

typedef struct crosstalk_net_steam_friend
{
    uint64_t steam_id;
    uint64_t lobby_id;
    char name[CROSSTALK_NET_FRIEND_NAME_CAP];
    char detail[CROSSTALK_NET_FRIEND_DETAIL_CAP];
    uint32_t state;
    uint8_t in_game;
    uint8_t in_ours;
    uint8_t started;
    uint8_t pudding[1];
} crosstalk_net_steam_friend_t;

typedef struct steam_net_notice
{
    char text[CROSSTALK_NET_NOTICE_CAP];
    double ttl;
} steam_net_notice_t;

typedef struct steam_net_ctx
{
    ldg_net_steamworks_ctx_t *sesh;
    ldg_net_cb_t *cb;
    ldg_net_steamworks_lobby_ctx_t lobby;
    ldg_net_steamworks_hub_t hub;
    ldg_net_steamworks_join_ctx_t join;
    ldg_net_steamworks_friend_call_ctx_t friend_call;
    ldg_net_steamworks_srv_browser_ctx_t lan_browser;
    crosstalk_net_steam_friend_t friend_list[CROSSTALK_NET_FRIEND_CAP];
    uint64_t friend_ours_prev[CROSSTALK_NET_FRIEND_CAP];
    uint32_t friend_ours_prev_cunt;
    uint32_t friend_cunt;
    double friend_next;
    steam_net_notice_t notice[CROSSTALK_NET_NOTICE_RING];
    uint32_t notice_hd;
    uint32_t notice_cunt;
    uint32_t wire_seq;
    uint8_t *snap_buff;
    ldg_net_steamworks_friend_msg_ctx_t *friend_msg;
    ldg_net_steamworks_friend_msg_t friend_msg_buff;
    uint32_t friend_msg_drop_prev;
    uint32_t snap_cap;
    uint32_t snap_len;
    uint32_t snap_total;
    uint16_t lan_port;
    uint8_t lobby_live;
    uint8_t sesh_live;
    uint8_t friend_msg_live;
    uint8_t lan_browser_ok;
    uint8_t lan_srv_ok;
    uint8_t lan_sesh;
    uint8_t pudding[1];
    uint8_t wire_buff[CROSSTALK_NET_WIRE_BUFF_CAP];
} steam_net_ctx_t;

uint32_t crosstalk_net_steam_init(steam_net_ctx_t *ctx, ldg_net_steamworks_ctx_t *sesh, ldg_net_cb_t *cb);
void crosstalk_net_steam_shutdown(steam_net_ctx_t *ctx);
void crosstalk_net_steam_poll(steam_net_ctx_t *ctx, double dt);
uint32_t crosstalk_net_steam_join_req_get(steam_net_ctx_t *ctx, uint64_t *out_lobby_id);
uint32_t crosstalk_net_steam_friend_cunt_get(const steam_net_ctx_t *ctx);
const crosstalk_net_steam_friend_t* crosstalk_net_steam_friend_get(const steam_net_ctx_t *ctx, uint32_t idx);
uint8_t crosstalk_net_steam_friend_started_take(steam_net_ctx_t *ctx, uint32_t idx);
const char* crosstalk_net_steam_persona_state_name_get(uint32_t state);

uint32_t crosstalk_net_steam_host_start(steam_net_ctx_t *ctx, int32_t virtual_port);
uint32_t crosstalk_net_steam_client_join(steam_net_ctx_t *ctx, uint64_t host_steam_id, int32_t virtual_port);
uint32_t crosstalk_net_steam_lan_host_start(steam_net_ctx_t *ctx, uint16_t port, uint32_t player_max, const char *map, const char *name);
uint32_t crosstalk_net_steam_lan_client_join(steam_net_ctx_t *ctx, uint32_t ip, uint16_t port);
uint32_t crosstalk_net_steam_lan_list_req(steam_net_ctx_t *ctx);
uint32_t crosstalk_net_steam_lan_list_cunt_get(const steam_net_ctx_t *ctx);
const ldg_net_srv_browser_entry_t* crosstalk_net_steam_lan_list_get(const steam_net_ctx_t *ctx, uint32_t idx);
uint8_t crosstalk_net_steam_lan_sesh_get(const steam_net_ctx_t *ctx);
void crosstalk_net_steam_sesh_stop(steam_net_ctx_t *ctx);
ldg_net_conn_status_t crosstalk_net_steam_status_get(const steam_net_ctx_t *ctx);
uint32_t crosstalk_net_steam_client_cunt_get(const steam_net_ctx_t *ctx);
const char* crosstalk_net_steam_client_name_get(const steam_net_ctx_t *ctx, uint32_t client_id);

uint32_t crosstalk_net_steam_lobby_mode_set(steam_net_ctx_t *ctx, const char *mode);
uint32_t crosstalk_net_steam_lobby_create(steam_net_ctx_t *ctx, int32_t max_members);
uint32_t crosstalk_net_steam_lobby_list_req(steam_net_ctx_t *ctx);
uint32_t crosstalk_net_steam_lobby_join(steam_net_ctx_t *ctx, uint64_t lobby_id);
void crosstalk_net_steam_lobby_leave(steam_net_ctx_t *ctx);
void crosstalk_net_steam_lobby_rdy_set(steam_net_ctx_t *ctx, uint8_t rdy);
void crosstalk_net_steam_lobby_busy_set(steam_net_ctx_t *ctx, uint8_t busy);
uint32_t crosstalk_net_steam_lobby_game_start(steam_net_ctx_t *ctx);
ldg_net_lobby_state_t crosstalk_net_steam_lobby_state_get(const steam_net_ctx_t *ctx);
uint32_t crosstalk_net_steam_lobby_member_cunt_get(const steam_net_ctx_t *ctx);
const ldg_net_lobby_member_t* crosstalk_net_steam_lobby_member_get(const steam_net_ctx_t *ctx, uint32_t idx);
uint32_t crosstalk_net_steam_lobby_list_cunt_get(const steam_net_ctx_t *ctx);
const ldg_net_lobby_info_t* crosstalk_net_steam_lobby_list_get(const steam_net_ctx_t *ctx, uint32_t idx);
uint64_t crosstalk_net_steam_lobby_host_steam_id_get(steam_net_ctx_t *ctx);
uint64_t crosstalk_net_steam_lobby_id_get(const steam_net_ctx_t *ctx);

uint32_t crosstalk_net_steam_friend_msg_get(steam_net_ctx_t *ctx, uint64_t *out_steam_id, const char **out_text, uint32_t *out_len);
uint32_t crosstalk_net_steam_friend_msg_reply(uint64_t steam_id_friend, const char *text);

void crosstalk_net_steam_notice_push(steam_net_ctx_t *ctx, const char *text);
uint8_t crosstalk_net_steam_notice_active_is(const steam_net_ctx_t *ctx);
const char* crosstalk_net_steam_notice_text_get(const steam_net_ctx_t *ctx);

#endif
