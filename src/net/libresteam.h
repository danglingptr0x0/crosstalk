#ifndef CROSSTALK_NET_LIBRESTEAM_H
#define CROSSTALK_NET_LIBRESTEAM_H

#include <stdint.h>
#include <inttypes.h>

#include <libresteam/backend/backend.h>
#include <libresteam/cb/ops.h>
#include <libresteam/net/types.h>

#include "net/presence.h"

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
#define CROSSTALK_NET_LIBRESTEAM_APP_ID 4898790U
#define CROSSTALK_NET_LIBRESTEAM_APPID_MASK 0xFFFFFFFFULL
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
#define CROSSTALK_NET_LAN_PORT_DEFAULT 27015
#define CROSSTALK_NET_CLIENT_MAX 10
#define CROSSTALK_NET_CLIENT_NAME_MAX 64
#define CROSSTALK_NET_CLIENT_ID_NONE UINT32_MAX
#define CROSSTALK_NET_LOBBY_MEMBER_MAX 10
#define CROSSTALK_NET_LOBBY_LIST_MAX 16
#define CROSSTALK_NET_LOBBY_NAME_MAX 64
#define CROSSTALK_NET_LOBBY_MAP_MAX 64
#define CROSSTALK_NET_LOBBY_KEY_MAP "map"
#define CROSSTALK_NET_LOBBY_KEY_REV "ver"
#define CROSSTALK_NET_LOBBY_KEY_HOST "host"
#define CROSSTALK_NET_LOBBY_KEY_START "start"
#define CROSSTALK_NET_LOBBY_MEMBER_KEY_RDY "rdy"
#define CROSSTALK_NET_LOBBY_MEMBER_KEY_TEAM "team"
#define CROSSTALK_NET_LOBBY_TYPE_PUBLIC 2U
#define CROSSTALK_NET_LOBBY_CMP_EQUAL 0U
#define CROSSTALK_NET_LOBBY_PENDING_NONE 0U
#define CROSSTALK_NET_LOBBY_PENDING_CREATE 1U
#define CROSSTALK_NET_LOBBY_PENDING_LIST 2U
#define CROSSTALK_NET_LOBBY_PENDING_JOIN 3U
#define CROSSTALK_NET_FRIEND_MSG_RING 16
#define CROSSTALK_NET_FRIEND_MSG_TEXT_CAP 2048
#define CROSSTALK_NET_JOIN_CONNECT_MAX 256
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_DISPLAY "steam_display"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_PLAYERS "players"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_PLAYERS_MAX "maxplayers"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_GROUP "steam_player_group"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_GROUP_SIZE "steam_player_group_size"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_CONNECT "connect"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_MENU "menu"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_ACTIVITY "activity"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_OP "op"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_CONNECT_FMT "+connect_lobby %" PRIu64
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_CONNECT_CAP 48
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_GROUP_CLEAR 0
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_VAL_CLEAR ""
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_NUM_CAP 16
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_ID_CAP 32
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_PLAYERS_SOLO 1U
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_LOBBY "#Lobby"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_MENU "#InMenu"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_KB "#Keyboard"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_KB_SOLO "#KeyboardSolo"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_KB_OP "#KeyboardOp"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_KB_ACT "#KeyboardAct"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_KB_OP_ACT "#KeyboardOpAct"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_KB_PAUSED "#KeyboardPaused"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_LOBBY_OP "#LobbyOp"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_LOBBY_ACT "#LobbyAct"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_LOBBY_OP_ACT "#LobbyOpAct"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_LOBBY_PAUSED "#LobbyPaused"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_SNIFFERS "#Sniffers"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_STORY "#Story"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_DANGCSA_GAME "#DangcsaGame"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_LD "#LoadMenu"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_BROWSE "#Browsing"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_NEWPROJ "#NewProject"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_EXTRAS "#Extras"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_CREDITS "#Credits"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_MENU_MAIN "Main menu"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_MENU_NEW "Starting a new project"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_MENU_SETTINGS "Fiddling with the rotaries"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_PROJ_RTL "an RTL project"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_PROJ_ZONE "a Zone"
#define CROSSTALK_NET_LIBRESTEAM_PRESENCE_PROJ_REVERSE "a Reverse project"
#define CROSSTALK_NET_MSG_BATCH 32

typedef enum crosstalk_net_libresteam_lobby_state
{
    CROSSTALK_NET_LIBRESTEAM_LOBBY_STATE_NONE = 0,
    CROSSTALK_NET_LIBRESTEAM_LOBBY_STATE_BROWSING,
    CROSSTALK_NET_LIBRESTEAM_LOBBY_STATE_CREATING,
    CROSSTALK_NET_LIBRESTEAM_LOBBY_STATE_HOSTING,
    CROSSTALK_NET_LIBRESTEAM_LOBBY_STATE_JOINING,
    CROSSTALK_NET_LIBRESTEAM_LOBBY_STATE_JOINED,
    CROSSTALK_NET_LIBRESTEAM_LOBBY_STATE_STARTING
} crosstalk_net_libresteam_lobby_state_t;

typedef enum crosstalk_net_libresteam_conn_status
{
    CROSSTALK_NET_LIBRESTEAM_CONN_STATUS_DCD = 0,
    CROSSTALK_NET_LIBRESTEAM_CONN_STATUS_CONNECTING,
    CROSSTALK_NET_LIBRESTEAM_CONN_STATUS_CONN
} crosstalk_net_libresteam_conn_status_t;

typedef enum crosstalk_net_libresteam_mode
{
    CROSSTALK_NET_LIBRESTEAM_MODE_NONE = 0,
    CROSSTALK_NET_LIBRESTEAM_MODE_SRV,
    CROSSTALK_NET_LIBRESTEAM_MODE_CLIENT
} crosstalk_net_libresteam_mode_t;

typedef struct crosstalk_net_libresteam_lobby_member
{
    uint64_t user_id;
    char name[CROSSTALK_NET_LOBBY_NAME_MAX];
    uint8_t rdy;
    uint8_t team;
    uint8_t pudding[6];
} crosstalk_net_libresteam_lobby_member_t;

typedef struct crosstalk_net_libresteam_lobby_info
{
    uint64_t lobby_id;
    char host_name[CROSSTALK_NET_LOBBY_NAME_MAX];
    char map_name[CROSSTALK_NET_LOBBY_MAP_MAX];
    uint8_t player_cunt;
    uint8_t player_max;
    uint8_t pudding[6];
} crosstalk_net_libresteam_lobby_info_t;

typedef struct crosstalk_net_libresteam_client
{
    char name[CROSSTALK_NET_CLIENT_NAME_MAX];
    uint64_t steam_id;
    uint32_t conn;
    uint8_t live;
    uint8_t pudding[3];
} crosstalk_net_libresteam_client_t;

typedef struct crosstalk_net_libresteam_friend_msg
{
    uint64_t steam_id;
    uint32_t msg_id;
    uint8_t pudding[4];
} crosstalk_net_libresteam_friend_msg_t;

struct crosstalk_net_libresteam_ctx;

typedef void (*crosstalk_net_libresteam_conn_cb_t)(struct crosstalk_net_libresteam_ctx *ctx, uint32_t conn_id);
typedef void (*crosstalk_net_libresteam_dc_cb_t)(struct crosstalk_net_libresteam_ctx *ctx, uint32_t conn_id);
typedef void (*crosstalk_net_libresteam_data_cb_t)(struct crosstalk_net_libresteam_ctx *ctx, uint32_t conn_id, const void *data, uint32_t len);

typedef struct crosstalk_net_libresteam_cb
{
    crosstalk_net_libresteam_conn_cb_t on_conn;
    crosstalk_net_libresteam_dc_cb_t on_dc;
    crosstalk_net_libresteam_data_cb_t on_data;
    void *user;
} crosstalk_net_libresteam_cb_t;

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

typedef struct crosstalk_net_libresteam_friend
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
} crosstalk_net_libresteam_friend_t;

typedef struct crosstalk_net_libresteam_notice
{
    char text[CROSSTALK_NET_NOTICE_CAP];
    double ttl;
} crosstalk_net_libresteam_notice_t;

typedef struct crosstalk_net_libresteam_ctx
{
    const libresteam_backend_set_t *set;
    crosstalk_net_libresteam_cb_t *cb;
    libresteam_cb_sink_ops_t sink_ops;
    libresteam_cb_sink_t sink;
    crosstalk_net_libresteam_client_t client[CROSSTALK_NET_CLIENT_MAX];
    crosstalk_net_libresteam_lobby_info_t lobby_list[CROSSTALK_NET_LOBBY_LIST_MAX];
    crosstalk_net_libresteam_lobby_member_t lobby_member[CROSSTALK_NET_LOBBY_MEMBER_MAX];
    crosstalk_net_libresteam_friend_msg_t friend_msg_ring[CROSSTALK_NET_FRIEND_MSG_RING];
    char friend_msg_text[CROSSTALK_NET_FRIEND_MSG_TEXT_CAP];
    char lobby_map[CROSSTALK_NET_LOBBY_MAP_MAX];
    char join_ask_name[CROSSTALK_NET_CLIENT_NAME_MAX];
    uint64_t lobby_id;
    uint64_t lobby_host_id;
    uint64_t lobby_call;
    uint64_t join_lobby_id;
    uint64_t join_friend_id;
    uint64_t join_ask_steam_id;
    uint32_t lobby_pending;
    uint32_t lobby_state;
    uint32_t lobby_list_cunt;
    uint32_t lobby_member_cunt;
    uint32_t friend_msg_hd;
    uint32_t friend_msg_cunt;
    uint32_t friend_msg_drop;
    uint32_t listen_sock;
    uint32_t poll_group;
    uint32_t srv_conn;
    uint32_t mode;
    uint32_t status;
    uint32_t client_cunt;
    uint32_t join_ask_conn;
    uint8_t lobby_is_host;
    uint8_t join_pending;
    uint8_t join_ask_live;
    uint8_t pudding0[5];
    crosstalk_net_libresteam_friend_t friend_list[CROSSTALK_NET_FRIEND_CAP];
    uint64_t friend_ours_prev[CROSSTALK_NET_FRIEND_CAP];
    uint32_t friend_ours_prev_cunt;
    uint32_t friend_cunt;
    double friend_next;
    crosstalk_net_libresteam_notice_t notice[CROSSTALK_NET_NOTICE_RING];
    uint32_t notice_hd;
    uint32_t notice_cunt;
    uint32_t wire_seq;
    uint8_t *snap_buff;
    uint32_t snap_cap;
    uint32_t snap_len;
    uint32_t snap_total;
    uint16_t lan_port;
    uint8_t lobby_live;
    uint8_t sesh_live;
    uint8_t friend_msg_live;
    uint8_t lan_sesh;
    uint8_t pudding[6];
    uint8_t wire_buff[CROSSTALK_NET_WIRE_BUFF_CAP];
} crosstalk_net_libresteam_ctx_t;

uint32_t crosstalk_net_libresteam_init(crosstalk_net_libresteam_ctx_t *ctx, const libresteam_backend_set_t *set, crosstalk_net_libresteam_cb_t *cb);
void crosstalk_net_libresteam_shutdown(crosstalk_net_libresteam_ctx_t *ctx);
void crosstalk_net_libresteam_poll(crosstalk_net_libresteam_ctx_t *ctx, double dt);
uint32_t crosstalk_net_libresteam_join_req_get(crosstalk_net_libresteam_ctx_t *ctx, uint64_t *out_lobby_id);
uint32_t crosstalk_net_libresteam_friend_cunt_get(const crosstalk_net_libresteam_ctx_t *ctx);
const crosstalk_net_libresteam_friend_t* crosstalk_net_libresteam_friend_get(const crosstalk_net_libresteam_ctx_t *ctx, uint32_t idx);
uint8_t crosstalk_net_libresteam_friend_started_take(crosstalk_net_libresteam_ctx_t *ctx, uint32_t idx);
const char* crosstalk_net_libresteam_persona_state_name_get(uint32_t state);

uint32_t crosstalk_net_libresteam_host_start(crosstalk_net_libresteam_ctx_t *ctx, int32_t virtual_port);
uint32_t crosstalk_net_libresteam_client_join(crosstalk_net_libresteam_ctx_t *ctx, uint64_t host_steam_id, int32_t virtual_port);
uint32_t crosstalk_net_libresteam_lan_host_start(crosstalk_net_libresteam_ctx_t *ctx, uint16_t port, uint32_t player_max, const char *map, const char *name);
uint32_t crosstalk_net_libresteam_lan_client_join(crosstalk_net_libresteam_ctx_t *ctx, uint32_t ip, uint16_t port);
uint8_t crosstalk_net_libresteam_lan_sesh_get(const crosstalk_net_libresteam_ctx_t *ctx);
void crosstalk_net_libresteam_sesh_stop(crosstalk_net_libresteam_ctx_t *ctx);
uint32_t crosstalk_net_libresteam_join_ask_get(const crosstalk_net_libresteam_ctx_t *ctx, uint64_t *out_steam_id, const char **out_name);
uint32_t crosstalk_net_libresteam_join_ask_answer(crosstalk_net_libresteam_ctx_t *ctx, uint8_t allow);
crosstalk_net_libresteam_conn_status_t crosstalk_net_libresteam_status_get(const crosstalk_net_libresteam_ctx_t *ctx);
crosstalk_net_libresteam_mode_t crosstalk_net_libresteam_mode_get(const crosstalk_net_libresteam_ctx_t *ctx);
uint32_t crosstalk_net_libresteam_client_cunt_get(const crosstalk_net_libresteam_ctx_t *ctx);
const char* crosstalk_net_libresteam_client_name_get(const crosstalk_net_libresteam_ctx_t *ctx, uint32_t client_id);
uint64_t crosstalk_net_libresteam_client_steam_id_get(const crosstalk_net_libresteam_ctx_t *ctx, uint32_t client_id);
uint32_t crosstalk_net_libresteam_send(crosstalk_net_libresteam_ctx_t *ctx, uint32_t client_id, const void *data, uint32_t len, uint8_t reliable);
uint32_t crosstalk_net_libresteam_bcast(crosstalk_net_libresteam_ctx_t *ctx, const void *data, uint32_t len, uint8_t reliable);
uint64_t crosstalk_net_libresteam_local_steam_id_get(const crosstalk_net_libresteam_ctx_t *ctx);
uint64_t crosstalk_net_libresteam_join_friend_steam_id_get(const crosstalk_net_libresteam_ctx_t *ctx);

uint32_t crosstalk_net_libresteam_lobby_mode_set(crosstalk_net_libresteam_ctx_t *ctx, const char *mode);
uint32_t crosstalk_net_libresteam_lobby_create(crosstalk_net_libresteam_ctx_t *ctx, int32_t max_members);
uint32_t crosstalk_net_libresteam_lobby_list_req(crosstalk_net_libresteam_ctx_t *ctx);
uint32_t crosstalk_net_libresteam_lobby_join(crosstalk_net_libresteam_ctx_t *ctx, uint64_t lobby_id);
void crosstalk_net_libresteam_lobby_leave(crosstalk_net_libresteam_ctx_t *ctx);
void crosstalk_net_libresteam_lobby_rdy_set(crosstalk_net_libresteam_ctx_t *ctx, uint8_t rdy);
void crosstalk_net_libresteam_lobby_busy_set(crosstalk_net_libresteam_ctx_t *ctx, uint8_t busy);
uint32_t crosstalk_net_libresteam_lobby_game_start(crosstalk_net_libresteam_ctx_t *ctx);
crosstalk_net_libresteam_lobby_state_t crosstalk_net_libresteam_lobby_state_get(const crosstalk_net_libresteam_ctx_t *ctx);
uint32_t crosstalk_net_libresteam_lobby_member_cunt_get(const crosstalk_net_libresteam_ctx_t *ctx);
const crosstalk_net_libresteam_lobby_member_t* crosstalk_net_libresteam_lobby_member_get(const crosstalk_net_libresteam_ctx_t *ctx, uint32_t idx);
uint32_t crosstalk_net_libresteam_lobby_list_cunt_get(const crosstalk_net_libresteam_ctx_t *ctx);
const crosstalk_net_libresteam_lobby_info_t* crosstalk_net_libresteam_lobby_list_get(const crosstalk_net_libresteam_ctx_t *ctx, uint32_t idx);
uint64_t crosstalk_net_libresteam_lobby_host_steam_id_get(crosstalk_net_libresteam_ctx_t *ctx);
uint64_t crosstalk_net_libresteam_lobby_id_get(const crosstalk_net_libresteam_ctx_t *ctx);

uint32_t crosstalk_net_libresteam_friend_msg_get(crosstalk_net_libresteam_ctx_t *ctx, uint64_t *out_steam_id, const char **out_text, uint32_t *out_len);
uint32_t crosstalk_net_libresteam_friend_msg_reply(const crosstalk_net_libresteam_ctx_t *ctx, uint64_t steam_id_friend, const char *text);

void crosstalk_net_libresteam_notice_push(crosstalk_net_libresteam_ctx_t *ctx, const char *text);
uint8_t crosstalk_net_libresteam_notice_active_is(const crosstalk_net_libresteam_ctx_t *ctx);
const char* crosstalk_net_libresteam_notice_text_get(const crosstalk_net_libresteam_ctx_t *ctx);

uint32_t crosstalk_net_libresteam_presence_push(void *backend_ctx, const crosstalk_net_presence_desc_t *desc);

#endif
