#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <dangling/logging/logging.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/core/bits.h>
#include <dangling/io/path.h>
#include <dangling/str/str.h>

#include <libresteam/cb/cb.h>
#include <libresteam/friends/friends.h>
#include <libresteam/lobby/lobby.h>
#include <libresteam/net_socks/net_socks.h>
#include <libresteam/user/user.h>
#include <libresteam/utils/utils.h>

#include "core/log.h"
#include "net/libresteam.h"
#include "net/presence.h"

#define NET_LIBRESTEAM_NOTICE_TTL 4.0
#define NET_LIBRESTEAM_P2P_VIRTUAL_PORT 0
#define NET_LIBRESTEAM_LOBBY_MAP "crosstalk"
#define NET_LIBRESTEAM_LOBBY_REV "1"
#define NET_LIBRESTEAM_CONNECT_LOBBY_ARG "+connect_lobby "
#define NET_LIBRESTEAM_CONNECT_SCAN_CUNT 1
#define NET_LIBRESTEAM_STEP 1U
#define NET_LIBRESTEAM_WIN_SEP '\\'
#define NET_LIBRESTEAM_PLAYER_NAME_DEFAULT "player"
#define NET_LIBRESTEAM_ID_STR_CAP 32
#define NET_LIBRESTEAM_CB_RES_CAP 64U
#define NET_LIBRESTEAM_ERES_OK 1U
#define NET_LIBRESTEAM_LOBBY_ENTER_OK 1U
#define NET_LIBRESTEAM_CB_JOIN_LOBBY_REQ 333U
#define NET_LIBRESTEAM_CB_JOIN_CONNECT_REQ 337U
#define NET_LIBRESTEAM_CB_FRIEND_CHAT_MSG 343U
#define NET_LIBRESTEAM_CB_LOBBY_ENTER 504U
#define NET_LIBRESTEAM_CB_LOBBY_MATCH_LIST 510U
#define NET_LIBRESTEAM_CB_LOBBY_CREATED 513U
#define NET_LIBRESTEAM_CB_CONN_STATUS 1221U
#define NET_LIBRESTEAM_OFF_FIRST_FIELD 0U
#define NET_LIBRESTEAM_OFF_SECOND_ID 8U
#define NET_LIBRESTEAM_OFF_LOBBY_ENTER_RESP 16U
#define NET_LIBRESTEAM_OFF_CREATED_LOBBY_PACK4 4U
#define NET_LIBRESTEAM_OFF_CREATED_LOBBY_PACK8 8U
#define NET_LIBRESTEAM_CONN_STATE_CONNECTING 1U
#define NET_LIBRESTEAM_CONN_STATE_ROUTE 2U
#define NET_LIBRESTEAM_CONN_STATE_CONNECTED 3U
#define NET_LIBRESTEAM_CONN_STATE_CLOSED_BY_PEER 4U
#define NET_LIBRESTEAM_CONN_STATE_PROBLEM 5U
#define NET_LIBRESTEAM_CONN_CLOSE_REASON_APP 1000U
#define NET_LIBRESTEAM_LINGER_OFF 0U
#define NET_LIBRESTEAM_ID_NONE 0U
#define NET_LIBRESTEAM_CONF_VAL_NONE 0U
#define NET_LIBRESTEAM_FLAG_OFF_CH '0'
#define NET_LIBRESTEAM_FLAG_ON "1"
#define NET_LIBRESTEAM_FLAG_OFF "0"
#define NET_LIBRESTEAM_LAN_ADDR_V4_PREFIX_OFF 10U
#define NET_LIBRESTEAM_LAN_ADDR_V4_OFF 12U
#define NET_LIBRESTEAM_LAN_ADDR_V4_MARK 0xFFU
#define NET_LIBRESTEAM_HOST_CONN_ID 0U

void crosstalk_net_libresteam_notice_push(crosstalk_net_libresteam_ctx_t *ctx, const char *text)
{
    crosstalk_net_libresteam_notice_t *slot = 0x0;
    uint32_t idx = 0;
    uint32_t rc = 0;

    if (LDG_UNLIKELY(!ctx || !text)) { return; }

    if (ctx->notice_cunt < CROSSTALK_NET_NOTICE_RING)
    {
        idx = (ctx->notice_hd + ctx->notice_cunt) % CROSSTALK_NET_NOTICE_RING;
        ctx->notice_cunt = ctx->notice_cunt + 1;
    }
    else
    {
        idx = ctx->notice_hd;
        ctx->notice_hd = (ctx->notice_hd + 1) % CROSSTALK_NET_NOTICE_RING;
    }

    slot = &ctx->notice[idx];
    rc = ldg_str_rbrcpy(slot->text, text, sizeof(slot->text));
    if (rc != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "libresteam: notice truncated; rc: %u32", rc); }

    slot->ttl = NET_LIBRESTEAM_NOTICE_TTL;
}

uint8_t crosstalk_net_libresteam_notice_active_is(const crosstalk_net_libresteam_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->notice_cunt == 0) { return LDG_TRUTH_FALSE; }

    return ctx->notice[ctx->notice_hd].ttl > 0.0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

const char* crosstalk_net_libresteam_notice_text_get(const crosstalk_net_libresteam_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx) || ctx->notice_cunt == 0) { return 0x0; }

    return ctx->notice[ctx->notice_hd].text;
}

static void net_libresteam_notice_tick(crosstalk_net_libresteam_ctx_t *ctx, double dt)
{
    crosstalk_net_libresteam_notice_t *slot = 0x0;

    if (LDG_UNLIKELY(!ctx) || ctx->notice_cunt == 0) { return; }

    slot = &ctx->notice[ctx->notice_hd];
    if (dt > 0.0) { slot->ttl = slot->ttl - dt; }

    if (slot->ttl <= 0.0)
    {
        slot->ttl = 0.0;
        slot->text[0] = 0;
        ctx->notice_hd = (ctx->notice_hd + 1) % CROSSTALK_NET_NOTICE_RING;
        ctx->notice_cunt = ctx->notice_cunt - 1;
    }
}

static uint32_t net_libresteam_u32_rd(const uint8_t *at)
{
    return (uint32_t)at[0] | ((uint32_t)at[1] << (uint32_t)LDG_BYTE_BITS) | ((uint32_t)at[2] << (uint32_t)LDG_BYTE_SHIFT_2) | ((uint32_t)at[3] << (uint32_t)LDG_BYTE_SHIFT_3);
}

static uint64_t net_libresteam_u64_rd(const uint8_t *at)
{
    return (uint64_t)net_libresteam_u32_rd(at) | ((uint64_t)net_libresteam_u32_rd(at + (uint64_t)LDG_WORD_BYTES) << (uint64_t)(LDG_BYTE_BITS * LDG_WORD_BYTES));
}

static libresteam_steam_id_t net_libresteam_id_make(uint64_t raw)
{
    libresteam_steam_id_t id = LDG_STRUCT_ZERO_INIT;

    id.all = raw;

    return id;
}

static void net_libresteam_ident_steam_set(libresteam_net_ident_t *ident, uint64_t steam_id)
{
    memset(ident, 0x0, sizeof(*ident));
    ident->type = LIBRESTEAM_NET_IDENT_TYPE_STEAM_ID;
    ident->size = (uint32_t)sizeof(steam_id);
    memcpy(ident->data, &steam_id, sizeof(steam_id));
}

static uint64_t net_libresteam_ident_steam_get(const libresteam_net_ident_t *ident)
{
    uint64_t steam_id = NET_LIBRESTEAM_ID_NONE;

    if (ident->type != LIBRESTEAM_NET_IDENT_TYPE_STEAM_ID || ident->size != (uint32_t)sizeof(steam_id)) { return NET_LIBRESTEAM_ID_NONE; }

    memcpy(&steam_id, ident->data, sizeof(steam_id));

    return steam_id;
}

static void net_libresteam_persona_copy(const crosstalk_net_libresteam_ctx_t *ctx, uint64_t steam_id, char *out, uint64_t cap)
{
    const char *persona = 0x0;

    out[0] = LDG_STR_TERM;

    if (libresteam_friends_friend_persona_name_get(ctx->set, net_libresteam_id_make(steam_id), &persona) != LDG_ERR_AOK || persona == 0x0 || persona[0] == LDG_STR_TERM) { persona = NET_LIBRESTEAM_PLAYER_NAME_DEFAULT; }

    if (ldg_str_rbrcpy(out, persona, cap) != LDG_ERR_AOK) { out[0] = LDG_STR_TERM; }
}

static uint32_t net_libresteam_client_find(const crosstalk_net_libresteam_ctx_t *ctx, uint32_t conn)
{
    uint32_t idx = 0;

    for (idx = 0; idx < CROSSTALK_NET_CLIENT_MAX; idx = idx + NET_LIBRESTEAM_STEP) { if (ctx->client[idx].live == LDG_TRUTH_TRUE && ctx->client[idx].conn == conn) { return idx; } }

    return CROSSTALK_NET_CLIENT_ID_NONE;
}

static void net_libresteam_conn_drop(crosstalk_net_libresteam_ctx_t *ctx, uint32_t conn)
{
    if (libresteam_net_socks_conn_close(ctx->set, conn, NET_LIBRESTEAM_CONN_CLOSE_REASON_APP, 0x0, NET_LIBRESTEAM_LINGER_OFF) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "libresteam: conn close failed; conn: %u32", conn); }
}

static void net_libresteam_srv_conn_status(crosstalk_net_libresteam_ctx_t *ctx, uint32_t conn, const libresteam_net_conn_info_t *info)
{
    uint32_t eres = 0;
    uint32_t slot = net_libresteam_client_find(ctx, conn);
    uint32_t idx = 0;

    if (info->state == NET_LIBRESTEAM_CONN_STATE_CONNECTING && slot == CROSSTALK_NET_CLIENT_ID_NONE)
    {
        for (idx = 0; idx < CROSSTALK_NET_CLIENT_MAX && ctx->client[idx].live == LDG_TRUTH_TRUE; idx = idx + NET_LIBRESTEAM_STEP) {}

        if (idx >= CROSSTALK_NET_CLIENT_MAX || libresteam_net_socks_conn_accept(ctx->set, conn, &eres) != LDG_ERR_AOK || eres != NET_LIBRESTEAM_ERES_OK)
        {
            net_libresteam_conn_drop(ctx, conn);
            return;
        }

        if (libresteam_net_socks_conn_poll_group_set(ctx->set, conn, ctx->poll_group) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "libresteam: poll group set failed; conn: %u32", conn); }

        ctx->client[idx].conn = conn;
        ctx->client[idx].steam_id = net_libresteam_ident_steam_get(&info->ident_remote);
        ctx->client[idx].live = LDG_TRUTH_TRUE;
        net_libresteam_persona_copy(ctx, ctx->client[idx].steam_id, ctx->client[idx].name, (uint64_t)sizeof(ctx->client[idx].name));
        ctx->client_cunt = ctx->client_cunt + NET_LIBRESTEAM_STEP;

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "libresteam: player joined; name: %.63s; conn: %u32", ctx->client[idx].name, idx);

        if (ctx->cb != 0x0 && ctx->cb->on_conn != 0x0) { ctx->cb->on_conn(ctx, idx); }

        return;
    }

    if ((info->state != NET_LIBRESTEAM_CONN_STATE_CLOSED_BY_PEER && info->state != NET_LIBRESTEAM_CONN_STATE_PROBLEM) || slot == CROSSTALK_NET_CLIENT_ID_NONE) { return; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "libresteam: player left; name: %.63s; conn: %u32", ctx->client[slot].name, slot);

    if (ctx->cb != 0x0 && ctx->cb->on_dc != 0x0) { ctx->cb->on_dc(ctx, slot); }

    net_libresteam_conn_drop(ctx, conn);
    memset(&ctx->client[slot], 0x0, sizeof(ctx->client[slot]));
    ctx->client_cunt = ctx->client_cunt - NET_LIBRESTEAM_STEP;
}

static void net_libresteam_client_conn_status(crosstalk_net_libresteam_ctx_t *ctx, uint32_t conn, const libresteam_net_conn_info_t *info)
{
    if (conn != ctx->srv_conn) { return; }

    if (info->state == NET_LIBRESTEAM_CONN_STATE_CONNECTED)
    {
        ctx->status = (uint32_t)CROSSTALK_NET_LIBRESTEAM_CONN_STATUS_CONN;

        if (ctx->cb != 0x0 && ctx->cb->on_conn != 0x0) { ctx->cb->on_conn(ctx, NET_LIBRESTEAM_HOST_CONN_ID); }

        return;
    }

    if (info->state != NET_LIBRESTEAM_CONN_STATE_CLOSED_BY_PEER && info->state != NET_LIBRESTEAM_CONN_STATE_PROBLEM) { return; }

    if (ctx->cb != 0x0 && ctx->cb->on_dc != 0x0) { ctx->cb->on_dc(ctx, NET_LIBRESTEAM_HOST_CONN_ID); }

    net_libresteam_conn_drop(ctx, conn);
    ctx->srv_conn = LIBRESTEAM_NET_HANDLE_INVALID;
    ctx->status = (uint32_t)CROSSTALK_NET_LIBRESTEAM_CONN_STATUS_DCD;
    ctx->mode = (uint32_t)CROSSTALK_NET_LIBRESTEAM_MODE_NONE;
    ctx->sesh_live = LDG_TRUTH_FALSE;
}

static void net_libresteam_join_connect_parse(crosstalk_net_libresteam_ctx_t *ctx, uint64_t friend_id, const char *connect)
{
    uint64_t prefix_len = (uint64_t)strlen(NET_LIBRESTEAM_CONNECT_LOBBY_ARG);
    uint64_t lobby_id = NET_LIBRESTEAM_ID_NONE;

    if (strncmp(connect, NET_LIBRESTEAM_CONNECT_LOBBY_ARG, prefix_len) != 0) { return; }

    if (sscanf(&connect[prefix_len], "%" SCNu64, &lobby_id) != NET_LIBRESTEAM_CONNECT_SCAN_CUNT || lobby_id == NET_LIBRESTEAM_ID_NONE) { return; }

    ctx->join_lobby_id = lobby_id;
    ctx->join_friend_id = friend_id;
    ctx->join_pending = LDG_TRUTH_TRUE;
}

static void net_libresteam_cb_msg(void *content, uint32_t user, uint32_t cb_id, const uint8_t *param, uint32_t param_size)
{
    crosstalk_net_libresteam_ctx_t *ctx = (crosstalk_net_libresteam_ctx_t *)content;
    libresteam_net_conn_info_t info = LDG_STRUCT_ZERO_INIT;
    char connect[CROSSTALK_NET_JOIN_CONNECT_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t copy_len = 0;
    uint32_t conn = 0;
    uint32_t slot = 0;

    if (ctx == 0x0 || param == 0x0 || user == UINT32_MAX) { return; }

    if (cb_id == NET_LIBRESTEAM_CB_JOIN_LOBBY_REQ && param_size >= NET_LIBRESTEAM_OFF_SECOND_ID + (uint32_t)sizeof(uint64_t))
    {
        ctx->join_lobby_id = net_libresteam_u64_rd(param + NET_LIBRESTEAM_OFF_FIRST_FIELD);
        ctx->join_friend_id = net_libresteam_u64_rd(param + NET_LIBRESTEAM_OFF_SECOND_ID);
        ctx->join_pending = LDG_TRUTH_TRUE;
    }
    else if (cb_id == NET_LIBRESTEAM_CB_JOIN_CONNECT_REQ && param_size > NET_LIBRESTEAM_OFF_SECOND_ID)
    {
        copy_len = param_size - NET_LIBRESTEAM_OFF_SECOND_ID;
        if (copy_len > (uint32_t)(sizeof(connect) - LDG_STR_TERM_SIZE)) { copy_len = (uint32_t)(sizeof(connect) - LDG_STR_TERM_SIZE); }

        memcpy(connect, param + NET_LIBRESTEAM_OFF_SECOND_ID, copy_len);
        net_libresteam_join_connect_parse(ctx, net_libresteam_u64_rd(param + NET_LIBRESTEAM_OFF_FIRST_FIELD), connect);
    }
    else if (cb_id == NET_LIBRESTEAM_CB_FRIEND_CHAT_MSG && param_size >= NET_LIBRESTEAM_OFF_SECOND_ID + (uint32_t)sizeof(uint32_t))
    {
        if (ctx->friend_msg_cunt >= CROSSTALK_NET_FRIEND_MSG_RING)
        {
            ctx->friend_msg_drop = ctx->friend_msg_drop + NET_LIBRESTEAM_STEP;
            return;
        }

        slot = (ctx->friend_msg_hd + ctx->friend_msg_cunt) % CROSSTALK_NET_FRIEND_MSG_RING;
        ctx->friend_msg_ring[slot].steam_id = net_libresteam_u64_rd(param + NET_LIBRESTEAM_OFF_FIRST_FIELD);
        ctx->friend_msg_ring[slot].msg_id = net_libresteam_u32_rd(param + NET_LIBRESTEAM_OFF_SECOND_ID);
        ctx->friend_msg_cunt = ctx->friend_msg_cunt + NET_LIBRESTEAM_STEP;
    }
    else if (cb_id == NET_LIBRESTEAM_CB_CONN_STATUS && param_size >= (uint32_t)sizeof(uint32_t))
    {
        conn = net_libresteam_u32_rd(param + NET_LIBRESTEAM_OFF_FIRST_FIELD);

        if (libresteam_net_socks_conn_info_get(ctx->set, conn, &info) != LDG_ERR_AOK) { return; }

        if (ctx->mode == (uint32_t)CROSSTALK_NET_LIBRESTEAM_MODE_SRV) { net_libresteam_srv_conn_status(ctx, conn, &info); }
        else if (ctx->mode == (uint32_t)CROSSTALK_NET_LIBRESTEAM_MODE_CLIENT) { net_libresteam_client_conn_status(ctx, conn, &info); }
    }
}

uint32_t crosstalk_net_libresteam_init(crosstalk_net_libresteam_ctx_t *ctx, const libresteam_backend_set_t *set, crosstalk_net_libresteam_cb_t *cb)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || set == 0x0 || cb == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));
    ctx->set = set;
    ctx->cb = cb;
    ctx->sink_ops.msg_cb = net_libresteam_cb_msg;
    ctx->sink.ops = &ctx->sink_ops;
    ctx->sink.content = ctx;
    ctx->listen_sock = LIBRESTEAM_NET_HANDLE_INVALID;
    ctx->poll_group = LIBRESTEAM_NET_HANDLE_INVALID;
    ctx->srv_conn = LIBRESTEAM_NET_HANDLE_INVALID;
    ctx->status = (uint32_t)CROSSTALK_NET_LIBRESTEAM_CONN_STATUS_DCD;
    ctx->mode = (uint32_t)CROSSTALK_NET_LIBRESTEAM_MODE_NONE;
    ctx->lobby_state = (uint32_t)CROSSTALK_NET_LIBRESTEAM_LOBBY_STATE_NONE;

    ret = ldg_str_rbrcpy(ctx->lobby_map, NET_LIBRESTEAM_LOBBY_MAP, (uint64_t)sizeof(ctx->lobby_map));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_mem_alloc((uint64_t)CROSSTALK_NET_SNAP_CAP, (void **)&ctx->snap_buff);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = libresteam_friends_msg_listen_set(set, LDG_TRUTH_TRUE);
    if (ret == LDG_ERR_AOK) { ctx->friend_msg_live = LDG_TRUTH_TRUE; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "libresteam: friend msg listen; err: %u32", ret);

    ctx->snap_cap = CROSSTALK_NET_SNAP_CAP;
    ctx->lobby_live = LDG_TRUTH_TRUE;
    ctx->lan_port = CROSSTALK_NET_LAN_PORT_DEFAULT;
    ctx->lan_sesh = (uint8_t)CROSSTALK_NET_LAN_SESH_NONE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_libresteam_friend_msg_get(crosstalk_net_libresteam_ctx_t *ctx, uint64_t *out_steam_id, const char **out_text, uint32_t *out_len)
{
    crosstalk_net_libresteam_friend_msg_t entry = LDG_STRUCT_ZERO_INIT;
    uint32_t type = 0;
    uint32_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !out_steam_id || !out_text || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_steam_id = 0;
    *out_text = 0x0;
    *out_len = 0;

    if (ctx->friend_msg_live != LDG_TRUTH_TRUE || ctx->friend_msg_cunt == 0) { return LDG_ERR_EMPTY; }

    entry = ctx->friend_msg_ring[ctx->friend_msg_hd];
    ctx->friend_msg_hd = (ctx->friend_msg_hd + NET_LIBRESTEAM_STEP) % CROSSTALK_NET_FRIEND_MSG_RING;
    ctx->friend_msg_cunt = ctx->friend_msg_cunt - NET_LIBRESTEAM_STEP;

    ret = libresteam_friends_msg_get(ctx->set, net_libresteam_id_make(entry.steam_id), entry.msg_id, ctx->friend_msg_text, (uint32_t)(sizeof(ctx->friend_msg_text) - LDG_STR_TERM_SIZE), &type, &len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (len > (uint32_t)(sizeof(ctx->friend_msg_text) - LDG_STR_TERM_SIZE)) { len = (uint32_t)(sizeof(ctx->friend_msg_text) - LDG_STR_TERM_SIZE); }

    ctx->friend_msg_text[len] = LDG_STR_TERM;

    *out_steam_id = entry.steam_id;
    *out_text = ctx->friend_msg_text;
    *out_len = len;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_libresteam_friend_msg_reply(const crosstalk_net_libresteam_ctx_t *ctx, uint64_t steam_id_friend, const char *text)
{
    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return libresteam_friends_msg_reply(ctx->set, net_libresteam_id_make(steam_id_friend), text);
}

void crosstalk_net_libresteam_shutdown(crosstalk_net_libresteam_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return; }

    crosstalk_net_libresteam_sesh_stop(ctx);

    if (ctx->friend_msg_live != LDG_TRUTH_FALSE)
    {
        ret = libresteam_friends_msg_listen_set(ctx->set, LDG_TRUTH_FALSE);

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "libresteam: friend msg listen off; err: %u32; drops: %u32", ret, ctx->friend_msg_drop);
        ctx->friend_msg_live = LDG_TRUTH_FALSE;
    }

    ctx->friend_cunt = CROSSTALK_NET_CUNT_NONE;

    if (ctx->lobby_live != LDG_TRUTH_FALSE)
    {
        crosstalk_net_libresteam_lobby_leave(ctx);
        ctx->lobby_live = LDG_TRUTH_FALSE;
    }

    if (ctx->snap_buff != 0x0)
    {
        ldg_mem_dealloc(ctx->snap_buff);
        ctx->snap_buff = 0x0;
        ctx->snap_cap = 0;
        ctx->snap_len = 0;
        ctx->snap_total = 0;
    }

    ctx->cb = 0x0;
    ctx->sesh_live = LDG_TRUTH_FALSE;
}

static uint32_t net_libresteam_friend_presence_get(const crosstalk_net_libresteam_ctx_t *ctx, uint64_t steam_id, const char *key, char *out, uint64_t cap)
{
    const char *val = 0x0;
    uint32_t ret = 0;

    out[0] = LDG_STR_TERM;

    ret = libresteam_friends_friend_rich_presence_get(ctx->set, net_libresteam_id_make(steam_id), key, &val);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (val == 0x0 || val[0] == LDG_STR_TERM) { return LDG_ERR_EMPTY; }

    return ldg_str_rbrcpy(out, val, cap);
}

static void net_libresteam_friend_detail_fill(const crosstalk_net_libresteam_ctx_t *ctx, uint64_t steam_id, char *out, uint32_t cap)
{
    char display[CROSSTALK_NET_FRIEND_DETAIL_CAP] = LDG_ARR_ZERO_INIT;
    char players[CROSSTALK_NET_FRIEND_DETAIL_CAP] = LDG_ARR_ZERO_INIT;
    char players_max[CROSSTALK_NET_FRIEND_DETAIL_CAP] = LDG_ARR_ZERO_INIT;

    out[0] = LDG_STR_TERM;

    if (net_libresteam_friend_presence_get(ctx, steam_id, CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_DISPLAY, display, (uint64_t)sizeof(display)) != LDG_ERR_AOK)
    {
        snprintf(out, cap, "%.47s", CROSSTALK_NET_FRIEND_DETAIL_OURS);
        return;
    }

    if (strcmp(display, CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_MENU) == 0)
    {
        snprintf(out, cap, "%.47s", CROSSTALK_NET_FRIEND_DETAIL_MENU);
        return;
    }

    if (strcmp(display, CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_KB) == 0 || strcmp(display, CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_KB_SOLO) == 0)
    {
        snprintf(out, cap, "%.47s", CROSSTALK_NET_FRIEND_DETAIL_SANDBOX);
        return;
    }

    if (strcmp(display, CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_LOBBY) != 0)
    {
        snprintf(out, cap, "%.47s", CROSSTALK_NET_FRIEND_DETAIL_OURS);
        return;
    }

    if (net_libresteam_friend_presence_get(ctx, steam_id, CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_PLAYERS, players, (uint64_t)sizeof(players)) != LDG_ERR_AOK || net_libresteam_friend_presence_get(ctx, steam_id, CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_PLAYERS_MAX, players_max, (uint64_t)sizeof(players_max)) != LDG_ERR_AOK)
    {
        snprintf(out, cap, "%.47s", CROSSTALK_NET_FRIEND_DETAIL_LOBBY);
        return;
    }

    snprintf(out, cap, "%.15s (%.7s/%.7s)", CROSSTALK_NET_FRIEND_DETAIL_LOBBY, players, players_max);
}

static uint8_t net_libresteam_friend_ours_prev_is(const crosstalk_net_libresteam_ctx_t *ctx, uint64_t steam_id)
{
    for (uint32_t idx = 0; idx < ctx->friend_ours_prev_cunt; idx++) { if (ctx->friend_ours_prev[idx] == steam_id) { return LDG_TRUTH_TRUE; } }

    return LDG_TRUTH_FALSE;
}

static void net_libresteam_friends_refresh(crosstalk_net_libresteam_ctx_t *ctx)
{
    libresteam_steam_id_t friend_id = LDG_STRUCT_ZERO_INIT;
    uint32_t ours_cunt = 0;
    uint32_t got = 0;
    uint32_t cunt = 0;
    uint32_t ret = 0;

    ret = libresteam_friends_friend_cunt_get(ctx->set, LIBRESTEAM_FRIENDS_FLAG_IMM, &got);
    if (ret != LDG_ERR_AOK)
    {
        ctx->friend_cunt = CROSSTALK_NET_CUNT_NONE;
        return;
    }

    for (uint32_t idx = 0; idx < got && cunt < (uint32_t)CROSSTALK_NET_FRIEND_CAP; idx++)
    {
        libresteam_friends_game_info_t game = LDG_STRUCT_ZERO_INIT;
        libresteam_friends_persona_state_t state = LIBRESTEAM_FRIENDS_PERSONA_STATE_OFFLINE;
        const char *persona = 0x0;
        uint8_t playing = LDG_TRUTH_FALSE;

        if (libresteam_friends_friend_get(ctx->set, idx, LIBRESTEAM_FRIENDS_FLAG_IMM, &friend_id) != LDG_ERR_AOK) { continue; }

        if (libresteam_friends_friend_persona_state_get(ctx->set, friend_id, &state) != LDG_ERR_AOK) { state = LIBRESTEAM_FRIENDS_PERSONA_STATE_OFFLINE; }

        ctx->friend_list[cunt].steam_id = friend_id.all;
        ctx->friend_list[cunt].lobby_id = CROSSTALK_NET_LOBBY_ID_NONE;
        ctx->friend_list[cunt].name[0] = LDG_STR_TERM;
        ctx->friend_list[cunt].state = (uint32_t)state;
        ctx->friend_list[cunt].in_game = LDG_TRUTH_FALSE;
        ctx->friend_list[cunt].in_ours = LDG_TRUTH_FALSE;
        ctx->friend_list[cunt].started = LDG_TRUTH_FALSE;
        ctx->friend_list[cunt].detail[0] = LDG_STR_TERM;

        if (libresteam_friends_friend_persona_name_get(ctx->set, friend_id, &persona) == LDG_ERR_AOK && persona != 0x0 && persona[0] != LDG_STR_TERM)
        {
            ret = ldg_str_rbrcpy(ctx->friend_list[cunt].name, persona, (uint64_t)sizeof(ctx->friend_list[cunt].name));
            if (ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC) { ctx->friend_list[cunt].name[0] = LDG_STR_TERM; }
        }

        if (libresteam_friends_friend_game_played_get(ctx->set, friend_id, &game, &playing) == LDG_ERR_AOK && playing != LDG_TRUTH_FALSE)
        {
            ctx->friend_list[cunt].in_game = LDG_TRUTH_TRUE;
            ctx->friend_list[cunt].lobby_id = game.lobby.all;

            if ((uint32_t)(game.game_id & LIBRESTEAM_FRIENDS_GAME_ID_APP_MASK) == CROSSTALK_NET_LIBRESTEAM_APP_ID)
            {
                ctx->friend_list[cunt].in_ours = LDG_TRUTH_TRUE;
                ctx->friend_list[cunt].started = (uint8_t)((net_libresteam_friend_ours_prev_is(ctx, friend_id.all) == LDG_TRUTH_TRUE) ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE);
                net_libresteam_friend_detail_fill(ctx, friend_id.all, ctx->friend_list[cunt].detail, (uint32_t)sizeof(ctx->friend_list[cunt].detail));
            }
            else { snprintf(ctx->friend_list[cunt].detail, sizeof(ctx->friend_list[cunt].detail), "%.47s", CROSSTALK_NET_FRIEND_DETAIL_OTHER); }
        }

        cunt++;
    }

    if (cunt != ctx->friend_cunt) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "libresteam: friends refreshed; cunt: %u32", cunt); }

    ctx->friend_cunt = cunt;

    for (uint32_t idx = 0; idx < cunt && ours_cunt < (uint32_t)CROSSTALK_NET_FRIEND_CAP; idx++)
    {
        if (ctx->friend_list[idx].in_ours != LDG_TRUTH_TRUE) { continue; }

        ctx->friend_ours_prev[ours_cunt] = ctx->friend_list[idx].steam_id;
        ours_cunt++;
    }

    ctx->friend_ours_prev_cunt = ours_cunt;
}

uint32_t crosstalk_net_libresteam_friend_cunt_get(const crosstalk_net_libresteam_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return CROSSTALK_NET_CUNT_NONE; }

    return ctx->friend_cunt;
}

const crosstalk_net_libresteam_friend_t* crosstalk_net_libresteam_friend_get(const crosstalk_net_libresteam_ctx_t *ctx, uint32_t idx)
{
    if (LDG_UNLIKELY(!ctx)) { return 0x0; }

    if (LDG_UNLIKELY(idx >= ctx->friend_cunt)) { return 0x0; }

    return &ctx->friend_list[idx];
}

uint8_t crosstalk_net_libresteam_friend_started_take(crosstalk_net_libresteam_ctx_t *ctx, uint32_t idx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_TRUTH_FALSE; }

    if (LDG_UNLIKELY(idx >= ctx->friend_cunt)) { return LDG_TRUTH_FALSE; }

    if (ctx->friend_list[idx].started != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    ctx->friend_list[idx].started = LDG_TRUTH_FALSE;

    return LDG_TRUTH_TRUE;
}

const char* crosstalk_net_libresteam_persona_state_name_get(uint32_t state)
{
    if (state == (uint32_t)LIBRESTEAM_FRIENDS_PERSONA_STATE_ONLINE) { return "online"; }

    if (state == (uint32_t)LIBRESTEAM_FRIENDS_PERSONA_STATE_BUSY) { return "busy"; }

    if (state == (uint32_t)LIBRESTEAM_FRIENDS_PERSONA_STATE_AWAY) { return "away"; }

    if (state == (uint32_t)LIBRESTEAM_FRIENDS_PERSONA_STATE_SNOOZE) { return "snooze"; }

    if (state == (uint32_t)LIBRESTEAM_FRIENDS_PERSONA_STATE_LOOKING_TO_TRADE) { return "looking to trade"; }

    if (state == (uint32_t)LIBRESTEAM_FRIENDS_PERSONA_STATE_LOOKING_TO_PLAY) { return "looking to play"; }

    if (state == (uint32_t)LIBRESTEAM_FRIENDS_PERSONA_STATE_INVISIBLE) { return "invisible"; }

    return "offline";
}

static uint32_t net_libresteam_call_res_get(const crosstalk_net_libresteam_ctx_t *ctx, uint32_t cb_id, uint8_t *res, uint8_t *out_done)
{
    uint8_t done = LDG_TRUTH_FALSE;
    uint8_t failed = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    *out_done = LDG_TRUTH_FALSE;

    ret = libresteam_utils_call_completed_get(ctx->set, ctx->lobby_call, &done, &failed);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (done != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    *out_done = LDG_TRUTH_TRUE;

    if (failed == LDG_TRUTH_TRUE) { return LDG_ERR_IO_RD; }

    ret = libresteam_cb_call_res_get(ctx->set, ctx->lobby_call, res, NET_LIBRESTEAM_CB_RES_CAP, cb_id, &failed);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (failed == LDG_TRUTH_TRUE) { return LDG_ERR_IO_RD; }

    return LDG_ERR_AOK;
}

static void net_libresteam_lobby_data_copy(const crosstalk_net_libresteam_ctx_t *ctx, uint64_t lobby_id, const char *key, char *out, uint64_t cap)
{
    const char *val = 0x0;

    out[0] = LDG_STR_TERM;

    if (libresteam_lobby_data_get(ctx->set, net_libresteam_id_make(lobby_id), key, &val) != LDG_ERR_AOK || val == 0x0) { return; }

    if (ldg_str_rbrcpy(out, val, cap) != LDG_ERR_AOK) { out[0] = LDG_STR_TERM; }
}

static void net_libresteam_lobby_list_fill(crosstalk_net_libresteam_ctx_t *ctx, uint32_t cunt)
{
    crosstalk_net_libresteam_lobby_info_t *info = 0x0;
    libresteam_steam_id_t lobby = LDG_STRUCT_ZERO_INIT;
    libresteam_steam_id_t owner = LDG_STRUCT_ZERO_INIT;
    uint32_t members = 0;
    uint32_t limit = 0;
    uint32_t idx = 0;

    ctx->lobby_list_cunt = 0;

    for (idx = 0; idx < cunt && ctx->lobby_list_cunt < CROSSTALK_NET_LOBBY_LIST_MAX; idx = idx + NET_LIBRESTEAM_STEP)
    {
        if (libresteam_lobby_list_lobby_get(ctx->set, idx, &lobby) != LDG_ERR_AOK) { continue; }

        info = &ctx->lobby_list[ctx->lobby_list_cunt];
        memset(info, 0x0, sizeof(*info));
        info->lobby_id = lobby.all;

        net_libresteam_lobby_data_copy(ctx, lobby.all, CROSSTALK_NET_LOBBY_KEY_MAP, info->map_name, (uint64_t)sizeof(info->map_name));

        if (libresteam_lobby_owner_get(ctx->set, lobby, &owner) == LDG_ERR_AOK) { net_libresteam_persona_copy(ctx, owner.all, info->host_name, (uint64_t)sizeof(info->host_name)); }

        if (libresteam_lobby_member_cunt_get(ctx->set, lobby, &members) == LDG_ERR_AOK) { info->player_cunt = (uint8_t)members; }

        if (libresteam_lobby_member_limit_get(ctx->set, lobby, &limit) == LDG_ERR_AOK) { info->player_max = (uint8_t)limit; }

        ctx->lobby_list_cunt = ctx->lobby_list_cunt + NET_LIBRESTEAM_STEP;
    }
}

static void net_libresteam_lobby_entered(crosstalk_net_libresteam_ctx_t *ctx, uint64_t lobby_id, uint8_t host)
{
    char host_str[NET_LIBRESTEAM_ID_STR_CAP] = LDG_ARR_ZERO_INIT;
    libresteam_steam_id_t lobby = net_libresteam_id_make(lobby_id);

    ctx->lobby_id = lobby_id;
    ctx->lobby_is_host = host;
    ctx->lobby_host_id = NET_LIBRESTEAM_ID_NONE;
    ctx->lobby_state = host == LDG_TRUTH_TRUE ? (uint32_t)CROSSTALK_NET_LIBRESTEAM_LOBBY_STATE_HOSTING : (uint32_t)CROSSTALK_NET_LIBRESTEAM_LOBBY_STATE_JOINED;

    if (host != LDG_TRUTH_TRUE) { return; }

    if (libresteam_lobby_data_set(ctx->set, lobby, CROSSTALK_NET_LOBBY_KEY_MAP, ctx->lobby_map) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "libresteam: lobby map set failed"); }

    if (libresteam_lobby_data_set(ctx->set, lobby, CROSSTALK_NET_LOBBY_KEY_REV, NET_LIBRESTEAM_LOBBY_REV) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "libresteam: lobby rev set failed"); }

    if (snprintf(host_str, sizeof(host_str), "%" PRIu64, crosstalk_net_libresteam_local_steam_id_get(ctx)) < 0) { return; }

    if (libresteam_lobby_data_set(ctx->set, lobby, CROSSTALK_NET_LOBBY_KEY_HOST, host_str) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "libresteam: lobby host set failed"); }
}

static void net_libresteam_lobby_pending_poll(crosstalk_net_libresteam_ctx_t *ctx)
{
    uint8_t res[NET_LIBRESTEAM_CB_RES_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t lobby_off = 0;
    uint8_t done = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (ctx->lobby_pending == CROSSTALK_NET_LOBBY_PENDING_NONE) { return; }

    if (ctx->lobby_pending == CROSSTALK_NET_LOBBY_PENDING_CREATE)
    {
        ret = net_libresteam_call_res_get(ctx, NET_LIBRESTEAM_CB_LOBBY_CREATED, res, &done);
        if (ret == LDG_ERR_AOK && done != LDG_TRUTH_TRUE) { return; }

        lobby_off = ldg_io_path_sep_get() == NET_LIBRESTEAM_WIN_SEP ? NET_LIBRESTEAM_OFF_CREATED_LOBBY_PACK8 : NET_LIBRESTEAM_OFF_CREATED_LOBBY_PACK4;

        if (ret == LDG_ERR_AOK && net_libresteam_u32_rd(res + NET_LIBRESTEAM_OFF_FIRST_FIELD) == NET_LIBRESTEAM_ERES_OK) { net_libresteam_lobby_entered(ctx, net_libresteam_u64_rd(res + lobby_off), LDG_TRUTH_TRUE); }
        else { ctx->lobby_state = (uint32_t)CROSSTALK_NET_LIBRESTEAM_LOBBY_STATE_NONE; }
    }
    else if (ctx->lobby_pending == CROSSTALK_NET_LOBBY_PENDING_JOIN)
    {
        ret = net_libresteam_call_res_get(ctx, NET_LIBRESTEAM_CB_LOBBY_ENTER, res, &done);
        if (ret == LDG_ERR_AOK && done != LDG_TRUTH_TRUE) { return; }

        if (ret == LDG_ERR_AOK && net_libresteam_u32_rd(res + NET_LIBRESTEAM_OFF_LOBBY_ENTER_RESP) == NET_LIBRESTEAM_LOBBY_ENTER_OK) { net_libresteam_lobby_entered(ctx, net_libresteam_u64_rd(res + NET_LIBRESTEAM_OFF_FIRST_FIELD), LDG_TRUTH_FALSE); }
        else { ctx->lobby_state = (uint32_t)CROSSTALK_NET_LIBRESTEAM_LOBBY_STATE_NONE; }
    }
    else if (ctx->lobby_pending == CROSSTALK_NET_LOBBY_PENDING_LIST)
    {
        ret = net_libresteam_call_res_get(ctx, NET_LIBRESTEAM_CB_LOBBY_MATCH_LIST, res, &done);
        if (ret == LDG_ERR_AOK && done != LDG_TRUTH_TRUE) { return; }

        if (ret == LDG_ERR_AOK) { net_libresteam_lobby_list_fill(ctx, net_libresteam_u32_rd(res + NET_LIBRESTEAM_OFF_FIRST_FIELD)); }
        else { ctx->lobby_list_cunt = 0; }
    }

    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "libresteam: lobby call failed; pending: %u32; err: %u32", ctx->lobby_pending, ret); }

    ctx->lobby_pending = CROSSTALK_NET_LOBBY_PENDING_NONE;
}

static uint8_t net_libresteam_member_flag_get(const crosstalk_net_libresteam_ctx_t *ctx, libresteam_steam_id_t lobby, libresteam_steam_id_t user, const char *key)
{
    const char *val = 0x0;

    if (libresteam_lobby_member_data_get(ctx->set, lobby, user, key, &val) != LDG_ERR_AOK || val == 0x0 || val[0] == LDG_STR_TERM) { return LDG_TRUTH_FALSE; }

    return val[0] == NET_LIBRESTEAM_FLAG_OFF_CH ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE;
}

static void net_libresteam_lobby_members_refresh(crosstalk_net_libresteam_ctx_t *ctx)
{
    char host_str[NET_LIBRESTEAM_ID_STR_CAP] = LDG_ARR_ZERO_INIT;
    crosstalk_net_libresteam_lobby_member_t *member = 0x0;
    libresteam_steam_id_t lobby = net_libresteam_id_make(ctx->lobby_id);
    libresteam_steam_id_t user = LDG_STRUCT_ZERO_INIT;
    uint64_t host_id = NET_LIBRESTEAM_ID_NONE;
    uint32_t cunt = 0;
    uint32_t idx = 0;

    if (ctx->lobby_id == NET_LIBRESTEAM_ID_NONE) { return; }

    if (libresteam_lobby_member_cunt_get(ctx->set, lobby, &cunt) != LDG_ERR_AOK) { return; }

    ctx->lobby_member_cunt = 0;

    for (idx = 0; idx < cunt && ctx->lobby_member_cunt < CROSSTALK_NET_LOBBY_MEMBER_MAX; idx = idx + NET_LIBRESTEAM_STEP)
    {
        if (libresteam_lobby_member_get(ctx->set, lobby, idx, &user) != LDG_ERR_AOK) { continue; }

        member = &ctx->lobby_member[ctx->lobby_member_cunt];
        memset(member, 0x0, sizeof(*member));
        member->user_id = user.all;
        net_libresteam_persona_copy(ctx, user.all, member->name, (uint64_t)sizeof(member->name));
        member->rdy = net_libresteam_member_flag_get(ctx, lobby, user, CROSSTALK_NET_LOBBY_MEMBER_KEY_RDY);
        member->team = net_libresteam_member_flag_get(ctx, lobby, user, CROSSTALK_NET_LOBBY_MEMBER_KEY_TEAM);
        ctx->lobby_member_cunt = ctx->lobby_member_cunt + NET_LIBRESTEAM_STEP;
    }

    if (ctx->lobby_state != (uint32_t)CROSSTALK_NET_LIBRESTEAM_LOBBY_STATE_JOINED) { return; }

    net_libresteam_lobby_data_copy(ctx, ctx->lobby_id, CROSSTALK_NET_LOBBY_KEY_START, host_str, (uint64_t)sizeof(host_str));

    if (host_str[0] == LDG_STR_TERM || sscanf(host_str, "%" SCNu64, &host_id) != NET_LIBRESTEAM_CONNECT_SCAN_CUNT || host_id == NET_LIBRESTEAM_ID_NONE) { return; }

    ctx->lobby_host_id = host_id;
    ctx->lobby_state = (uint32_t)CROSSTALK_NET_LIBRESTEAM_LOBBY_STATE_STARTING;
}

static void net_libresteam_msgs_drain(crosstalk_net_libresteam_ctx_t *ctx)
{
    libresteam_net_msg_t *msg[CROSSTALK_NET_MSG_BATCH] = LDG_ARR_ZERO_INIT;
    uint32_t cunt = 0;
    uint32_t slot = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (ctx->mode == (uint32_t)CROSSTALK_NET_LIBRESTEAM_MODE_SRV && ctx->poll_group != LIBRESTEAM_NET_HANDLE_INVALID) { ret = libresteam_net_socks_poll_group_msgs_recv(ctx->set, ctx->poll_group, msg, CROSSTALK_NET_MSG_BATCH, &cunt); }
    else if (ctx->mode == (uint32_t)CROSSTALK_NET_LIBRESTEAM_MODE_CLIENT && ctx->srv_conn != LIBRESTEAM_NET_HANDLE_INVALID) { ret = libresteam_net_socks_conn_msgs_recv(ctx->set, ctx->srv_conn, msg, CROSSTALK_NET_MSG_BATCH, &cunt); }
    else { return; }

    if (ret != LDG_ERR_AOK) { return; }

    if (cunt > CROSSTALK_NET_MSG_BATCH) { cunt = CROSSTALK_NET_MSG_BATCH; }

    for (idx = 0; idx < cunt; idx = idx + NET_LIBRESTEAM_STEP)
    {
        if (msg[idx] == 0x0) { continue; }

        slot = ctx->mode == (uint32_t)CROSSTALK_NET_LIBRESTEAM_MODE_SRV ? net_libresteam_client_find(ctx, msg[idx]->conn) : NET_LIBRESTEAM_HOST_CONN_ID;

        if (slot != CROSSTALK_NET_CLIENT_ID_NONE && ctx->cb != 0x0 && ctx->cb->on_data != 0x0) { ctx->cb->on_data(ctx, slot, msg[idx]->data, msg[idx]->size); }

        if (msg[idx]->release_cb != 0x0) { msg[idx]->release_cb(msg[idx]); }
    }
}

void crosstalk_net_libresteam_poll(crosstalk_net_libresteam_ctx_t *ctx, double dt)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    ret = libresteam_cb_pump(ctx->set, &ctx->sink);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "libresteam: callback pump failed; err: %u32", ret); }

    net_libresteam_lobby_pending_poll(ctx);

    if (ctx->lobby_live != LDG_TRUTH_FALSE) { net_libresteam_lobby_members_refresh(ctx); }

    if (ctx->friend_next > 0.0) { ctx->friend_next -= dt; }
    else
    {
        net_libresteam_friends_refresh(ctx);
        ctx->friend_next = CROSSTALK_NET_FRIEND_REFRESH_INTERVAL;
    }

    net_libresteam_msgs_drain(ctx);

    net_libresteam_notice_tick(ctx, dt);
}

uint32_t crosstalk_net_libresteam_join_req_get(crosstalk_net_libresteam_ctx_t *ctx, uint64_t *out_lobby_id)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_lobby_id == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_lobby_id = NET_LIBRESTEAM_ID_NONE;

    if (ctx->join_pending != LDG_TRUTH_TRUE) { return LDG_ERR_EMPTY; }

    ctx->join_pending = LDG_TRUTH_FALSE;

    if (ctx->join_lobby_id == NET_LIBRESTEAM_ID_NONE) { return LDG_ERR_INVALID; }

    *out_lobby_id = ctx->join_lobby_id;

    return LDG_ERR_AOK;
}

uint64_t crosstalk_net_libresteam_join_friend_steam_id_get(const crosstalk_net_libresteam_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return NET_LIBRESTEAM_ID_NONE; }

    return ctx->join_friend_id;
}

uint64_t crosstalk_net_libresteam_local_steam_id_get(const crosstalk_net_libresteam_ctx_t *ctx)
{
    libresteam_steam_id_t id = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(ctx == 0x0)) { return NET_LIBRESTEAM_ID_NONE; }

    if (libresteam_user_steam_id_get(ctx->set, &id) != LDG_ERR_AOK) { return NET_LIBRESTEAM_ID_NONE; }

    return id.all;
}

static uint32_t net_libresteam_srv_up(crosstalk_net_libresteam_ctx_t *ctx)
{
    uint32_t ret = 0;

    ret = libresteam_net_socks_poll_group_create(ctx->set, &ctx->poll_group);
    if (ret != LDG_ERR_AOK)
    {
        if (libresteam_net_socks_listen_sock_close(ctx->set, ctx->listen_sock) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "libresteam: listen sock close failed"); }

        ctx->listen_sock = LIBRESTEAM_NET_HANDLE_INVALID;
        ctx->poll_group = LIBRESTEAM_NET_HANDLE_INVALID;
        return ret;
    }

    ctx->mode = (uint32_t)CROSSTALK_NET_LIBRESTEAM_MODE_SRV;
    ctx->status = (uint32_t)CROSSTALK_NET_LIBRESTEAM_CONN_STATUS_CONN;
    ctx->client_cunt = 0;
    ctx->sesh_live = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static void net_libresteam_client_up(crosstalk_net_libresteam_ctx_t *ctx)
{
    ctx->mode = (uint32_t)CROSSTALK_NET_LIBRESTEAM_MODE_CLIENT;
    ctx->status = (uint32_t)CROSSTALK_NET_LIBRESTEAM_CONN_STATUS_CONNECTING;
    ctx->sesh_live = LDG_TRUTH_TRUE;
}

static void net_libresteam_ip_v4_set(libresteam_net_ip_addr_t *addr, uint32_t ip, uint16_t port)
{
    memset(addr, 0x0, sizeof(*addr));
    addr->ipv6[NET_LIBRESTEAM_LAN_ADDR_V4_PREFIX_OFF] = NET_LIBRESTEAM_LAN_ADDR_V4_MARK;
    addr->ipv6[NET_LIBRESTEAM_LAN_ADDR_V4_PREFIX_OFF + NET_LIBRESTEAM_STEP] = NET_LIBRESTEAM_LAN_ADDR_V4_MARK;
    addr->ipv6[NET_LIBRESTEAM_LAN_ADDR_V4_OFF] = (uint8_t)((ip >> (uint32_t)LDG_BYTE_SHIFT_3) & (uint32_t)LDG_BYTE_MASK);
    addr->ipv6[NET_LIBRESTEAM_LAN_ADDR_V4_OFF + NET_LIBRESTEAM_STEP] = (uint8_t)((ip >> (uint32_t)LDG_BYTE_SHIFT_2) & (uint32_t)LDG_BYTE_MASK);
    addr->ipv6[NET_LIBRESTEAM_LAN_ADDR_V4_OFF + NET_LIBRESTEAM_STEP + NET_LIBRESTEAM_STEP] = (uint8_t)((ip >> (uint32_t)LDG_BYTE_BITS) & (uint32_t)LDG_BYTE_MASK);
    addr->ipv6[NET_LIBRESTEAM_LAN_ADDR_V4_OFF + NET_LIBRESTEAM_STEP + NET_LIBRESTEAM_STEP + NET_LIBRESTEAM_STEP] = (uint8_t)(ip & (uint32_t)LDG_BYTE_MASK);
    addr->port = port;
}

uint32_t crosstalk_net_libresteam_host_start(crosstalk_net_libresteam_ctx_t *ctx, int32_t virtual_port)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (virtual_port < 0) { virtual_port = NET_LIBRESTEAM_P2P_VIRTUAL_PORT; }

    crosstalk_net_libresteam_sesh_stop(ctx);

    ret = libresteam_net_socks_listen_sock_p2p_create(ctx->set, (uint32_t)virtual_port, NET_LIBRESTEAM_CONF_VAL_NONE, 0x0, &ctx->listen_sock);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = net_libresteam_srv_up(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->lan_sesh = (uint8_t)CROSSTALK_NET_LAN_SESH_NONE;
    crosstalk_net_libresteam_notice_push(ctx, "hosting session");
    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_libresteam_client_join(crosstalk_net_libresteam_ctx_t *ctx, uint64_t host_steam_id, int32_t virtual_port)
{
    libresteam_net_ident_t remote = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(host_steam_id == NET_LIBRESTEAM_ID_NONE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (virtual_port < 0) { virtual_port = NET_LIBRESTEAM_P2P_VIRTUAL_PORT; }

    crosstalk_net_libresteam_sesh_stop(ctx);

    net_libresteam_ident_steam_set(&remote, host_steam_id);

    ret = libresteam_net_socks_p2p_connect(ctx->set, &remote, (uint32_t)virtual_port, NET_LIBRESTEAM_CONF_VAL_NONE, 0x0, &ctx->srv_conn);
    if (ret != LDG_ERR_AOK) { return ret; }

    net_libresteam_client_up(ctx);
    ctx->lan_sesh = (uint8_t)CROSSTALK_NET_LAN_SESH_NONE;
    crosstalk_net_libresteam_notice_push(ctx, "joining session");
    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_libresteam_lan_host_start(crosstalk_net_libresteam_ctx_t *ctx, uint16_t port, uint32_t player_max, const char *map, const char *name)
{
    libresteam_net_ip_addr_t local = LDG_STRUCT_ZERO_INIT;
    const char *srv_map = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (port == 0) { port = CROSSTALK_NET_LAN_PORT_DEFAULT; }

    if (player_max == 0) { player_max = CROSSTALK_NET_LOBBY_MAX_MEMBERS; }

    srv_map = (map != 0x0 && map[0] != LDG_STR_TERM) ? map : NET_LIBRESTEAM_LOBBY_MAP;

    if (name != 0x0 && name[0] != LDG_STR_TERM) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "libresteam: lan host name; name: %.63s", name); }

    crosstalk_net_libresteam_sesh_stop(ctx);

    memset(&local, 0x0, sizeof(local));
    local.port = port;

    ret = libresteam_net_socks_listen_sock_ip_create(ctx->set, &local, NET_LIBRESTEAM_CONF_VAL_NONE, 0x0, &ctx->listen_sock);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "libresteam: lan listen failed; err: %u32; port: %u32", ret, (uint32_t)port);
        return ret;
    }

    ret = net_libresteam_srv_up(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->lan_port = port;
    ctx->lan_sesh = (uint8_t)CROSSTALK_NET_LAN_SESH_HOSTING;
    crosstalk_net_libresteam_notice_push(ctx, "hosting LAN session");
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "libresteam: lan host; port: %u32; max: %u32; map: %.31s", (uint32_t)port, player_max, srv_map);
    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_libresteam_lan_client_join(crosstalk_net_libresteam_ctx_t *ctx, uint32_t ip, uint16_t port)
{
    libresteam_net_ip_addr_t addr = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ip == 0 || port == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    crosstalk_net_libresteam_sesh_stop(ctx);

    net_libresteam_ip_v4_set(&addr, ip, port);

    ret = libresteam_net_socks_ip_addr_connect(ctx->set, &addr, NET_LIBRESTEAM_CONF_VAL_NONE, 0x0, &ctx->srv_conn);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "libresteam: lan join failed; err: %u32; ip: %u32; port: %u32", ret, ip, (uint32_t)port);
        return ret;
    }

    net_libresteam_client_up(ctx);
    ctx->lan_sesh = (uint8_t)CROSSTALK_NET_LAN_SESH_JOINED;
    crosstalk_net_libresteam_notice_push(ctx, "joining LAN session");
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "libresteam: lan join; ip: %u32; port: %u32", ip, (uint32_t)port);
    return LDG_ERR_AOK;
}

uint8_t crosstalk_net_libresteam_lan_sesh_get(const crosstalk_net_libresteam_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return (uint8_t)CROSSTALK_NET_LAN_SESH_NONE; }

    return ctx->lan_sesh;
}

void crosstalk_net_libresteam_sesh_stop(crosstalk_net_libresteam_ctx_t *ctx)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    ctx->lan_sesh = (uint8_t)CROSSTALK_NET_LAN_SESH_NONE;

    if (ctx->sesh_live == LDG_TRUTH_FALSE && ctx->mode == (uint32_t)CROSSTALK_NET_LIBRESTEAM_MODE_NONE) { return; }

    for (idx = 0; idx < CROSSTALK_NET_CLIENT_MAX; idx = idx + NET_LIBRESTEAM_STEP)
    {
        if (ctx->client[idx].live != LDG_TRUTH_TRUE) { continue; }

        net_libresteam_conn_drop(ctx, ctx->client[idx].conn);
        memset(&ctx->client[idx], 0x0, sizeof(ctx->client[idx]));
    }

    if (ctx->srv_conn != LIBRESTEAM_NET_HANDLE_INVALID) { net_libresteam_conn_drop(ctx, ctx->srv_conn); }

    if (ctx->poll_group != LIBRESTEAM_NET_HANDLE_INVALID && libresteam_net_socks_poll_group_destroy(ctx->set, ctx->poll_group) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "libresteam: poll group destroy failed"); }

    if (ctx->listen_sock != LIBRESTEAM_NET_HANDLE_INVALID && libresteam_net_socks_listen_sock_close(ctx->set, ctx->listen_sock) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "libresteam: listen sock close failed"); }

    ctx->srv_conn = LIBRESTEAM_NET_HANDLE_INVALID;
    ctx->poll_group = LIBRESTEAM_NET_HANDLE_INVALID;
    ctx->listen_sock = LIBRESTEAM_NET_HANDLE_INVALID;
    ctx->client_cunt = 0;
    ctx->mode = (uint32_t)CROSSTALK_NET_LIBRESTEAM_MODE_NONE;
    ctx->status = (uint32_t)CROSSTALK_NET_LIBRESTEAM_CONN_STATUS_DCD;
    ctx->sesh_live = LDG_TRUTH_FALSE;
}

crosstalk_net_libresteam_conn_status_t crosstalk_net_libresteam_status_get(const crosstalk_net_libresteam_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return CROSSTALK_NET_LIBRESTEAM_CONN_STATUS_DCD; }

    return (crosstalk_net_libresteam_conn_status_t)ctx->status;
}

crosstalk_net_libresteam_mode_t crosstalk_net_libresteam_mode_get(const crosstalk_net_libresteam_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return CROSSTALK_NET_LIBRESTEAM_MODE_NONE; }

    return (crosstalk_net_libresteam_mode_t)ctx->mode;
}

uint32_t crosstalk_net_libresteam_client_cunt_get(const crosstalk_net_libresteam_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return CROSSTALK_NET_CUNT_NONE; }

    return ctx->client_cunt;
}

const char* crosstalk_net_libresteam_client_name_get(const crosstalk_net_libresteam_ctx_t *ctx, uint32_t client_id)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return 0x0; }

    if (client_id >= CROSSTALK_NET_CLIENT_MAX || ctx->client[client_id].live != LDG_TRUTH_TRUE) { return 0x0; }

    return ctx->client[client_id].name;
}

uint64_t crosstalk_net_libresteam_client_steam_id_get(const crosstalk_net_libresteam_ctx_t *ctx, uint32_t client_id)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return NET_LIBRESTEAM_ID_NONE; }

    if (client_id >= CROSSTALK_NET_CLIENT_MAX || ctx->client[client_id].live != LDG_TRUTH_TRUE) { return NET_LIBRESTEAM_ID_NONE; }

    return ctx->client[client_id].steam_id;
}

static uint32_t net_libresteam_conn_send(crosstalk_net_libresteam_ctx_t *ctx, uint32_t conn, const void *data, uint32_t len, uint8_t reliable)
{
    uint64_t msg_num = 0;
    uint32_t eres = 0;
    uint32_t flags = reliable != LDG_TRUTH_FALSE ? LIBRESTEAM_NET_SEND_RELIABLE : LIBRESTEAM_NET_SEND_UNRELIABLE;
    uint32_t ret = 0;

    ret = libresteam_net_socks_conn_msg_send(ctx->set, conn, data, len, flags, &msg_num, &eres);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (eres != NET_LIBRESTEAM_ERES_OK) { return LDG_ERR_NET_SEND; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_libresteam_send(crosstalk_net_libresteam_ctx_t *ctx, uint32_t client_id, const void *data, uint32_t len, uint8_t reliable)
{
    if (LDG_UNLIKELY(ctx == 0x0 || data == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->mode == (uint32_t)CROSSTALK_NET_LIBRESTEAM_MODE_CLIENT)
    {
        if (ctx->srv_conn == LIBRESTEAM_NET_HANDLE_INVALID) { return LDG_ERR_NET_DISCONNECTED; }

        return net_libresteam_conn_send(ctx, ctx->srv_conn, data, len, reliable);
    }

    if (ctx->mode != (uint32_t)CROSSTALK_NET_LIBRESTEAM_MODE_SRV) { return LDG_ERR_NET_DISCONNECTED; }

    if (client_id >= CROSSTALK_NET_CLIENT_MAX || ctx->client[client_id].live != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_FOUND; }

    return net_libresteam_conn_send(ctx, ctx->client[client_id].conn, data, len, reliable);
}

uint32_t crosstalk_net_libresteam_bcast(crosstalk_net_libresteam_ctx_t *ctx, const void *data, uint32_t len, uint8_t reliable)
{
    uint32_t idx = 0;
    uint32_t ret = 0;
    uint32_t first = LDG_ERR_AOK;

    if (LDG_UNLIKELY(ctx == 0x0 || data == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->mode != (uint32_t)CROSSTALK_NET_LIBRESTEAM_MODE_SRV) { return crosstalk_net_libresteam_send(ctx, NET_LIBRESTEAM_HOST_CONN_ID, data, len, reliable); }

    for (idx = 0; idx < CROSSTALK_NET_CLIENT_MAX; idx = idx + NET_LIBRESTEAM_STEP)
    {
        if (ctx->client[idx].live != LDG_TRUTH_TRUE) { continue; }

        ret = net_libresteam_conn_send(ctx, ctx->client[idx].conn, data, len, reliable);
        if (ret != LDG_ERR_AOK && first == LDG_ERR_AOK) { first = ret; }
    }

    return first;
}

uint32_t crosstalk_net_libresteam_lobby_mode_set(crosstalk_net_libresteam_ctx_t *ctx, const char *mode)
{
    if (LDG_UNLIKELY(ctx == 0x0 || mode == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ldg_str_rbrcpy(ctx->lobby_map, mode, (uint64_t)sizeof(ctx->lobby_map));
}

uint32_t crosstalk_net_libresteam_lobby_create(crosstalk_net_libresteam_ctx_t *ctx, int32_t max_members)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->lobby_pending != CROSSTALK_NET_LOBBY_PENDING_NONE) { return LDG_ERR_BUSY; }

    if (max_members <= 0) { max_members = CROSSTALK_NET_LOBBY_MAX_MEMBERS; }

    ret = libresteam_lobby_create(ctx->set, (libresteam_lobby_type_t)CROSSTALK_NET_LOBBY_TYPE_PUBLIC, (uint32_t)max_members, &ctx->lobby_call);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->lobby_pending = CROSSTALK_NET_LOBBY_PENDING_CREATE;
    ctx->lobby_state = (uint32_t)CROSSTALK_NET_LIBRESTEAM_LOBBY_STATE_CREATING;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_libresteam_lobby_list_req(crosstalk_net_libresteam_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->lobby_pending != CROSSTALK_NET_LOBBY_PENDING_NONE) { return LDG_ERR_BUSY; }

    ret = libresteam_lobby_list_str_filter_add(ctx->set, CROSSTALK_NET_LOBBY_KEY_MAP, ctx->lobby_map, CROSSTALK_NET_LOBBY_CMP_EQUAL);
    if (ret == LDG_ERR_AOK) { ret = libresteam_lobby_list_req(ctx->set, &ctx->lobby_call); }

    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->lobby_pending = CROSSTALK_NET_LOBBY_PENDING_LIST;

    if (ctx->lobby_id == NET_LIBRESTEAM_ID_NONE) { ctx->lobby_state = (uint32_t)CROSSTALK_NET_LIBRESTEAM_LOBBY_STATE_BROWSING; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_libresteam_lobby_join(crosstalk_net_libresteam_ctx_t *ctx, uint64_t lobby_id)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (lobby_id == NET_LIBRESTEAM_ID_NONE) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->lobby_pending != CROSSTALK_NET_LOBBY_PENDING_NONE) { return LDG_ERR_BUSY; }

    ret = libresteam_lobby_join(ctx->set, net_libresteam_id_make(lobby_id), &ctx->lobby_call);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->lobby_pending = CROSSTALK_NET_LOBBY_PENDING_JOIN;
    ctx->lobby_state = (uint32_t)CROSSTALK_NET_LIBRESTEAM_LOBBY_STATE_JOINING;

    return LDG_ERR_AOK;
}

void crosstalk_net_libresteam_lobby_leave(crosstalk_net_libresteam_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (ctx->lobby_id != NET_LIBRESTEAM_ID_NONE && libresteam_lobby_leave(ctx->set, net_libresteam_id_make(ctx->lobby_id)) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "libresteam: lobby leave failed; lobby: %u64", ctx->lobby_id); }

    ctx->lobby_id = NET_LIBRESTEAM_ID_NONE;
    ctx->lobby_host_id = NET_LIBRESTEAM_ID_NONE;
    ctx->lobby_member_cunt = 0;
    ctx->lobby_is_host = LDG_TRUTH_FALSE;
    ctx->lobby_pending = CROSSTALK_NET_LOBBY_PENDING_NONE;
    ctx->lobby_state = (uint32_t)CROSSTALK_NET_LIBRESTEAM_LOBBY_STATE_NONE;
}

static void net_libresteam_member_flag_set(crosstalk_net_libresteam_ctx_t *ctx, const char *key, uint8_t on)
{
    if (ctx->lobby_id == NET_LIBRESTEAM_ID_NONE) { return; }

    if (libresteam_lobby_member_data_set(ctx->set, net_libresteam_id_make(ctx->lobby_id), key, on != LDG_TRUTH_FALSE ? NET_LIBRESTEAM_FLAG_ON : NET_LIBRESTEAM_FLAG_OFF) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "libresteam: member data set failed; key: %.15s", key); }
}

void crosstalk_net_libresteam_lobby_rdy_set(crosstalk_net_libresteam_ctx_t *ctx, uint8_t rdy)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    net_libresteam_member_flag_set(ctx, CROSSTALK_NET_LOBBY_MEMBER_KEY_RDY, rdy);
}

void crosstalk_net_libresteam_lobby_busy_set(crosstalk_net_libresteam_ctx_t *ctx, uint8_t busy)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    net_libresteam_member_flag_set(ctx, CROSSTALK_NET_LOBBY_MEMBER_KEY_TEAM, busy);
}

uint32_t crosstalk_net_libresteam_lobby_game_start(crosstalk_net_libresteam_ctx_t *ctx)
{
    char host_str[NET_LIBRESTEAM_ID_STR_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t host_id = NET_LIBRESTEAM_ID_NONE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->lobby_id == NET_LIBRESTEAM_ID_NONE || ctx->lobby_is_host != LDG_TRUTH_TRUE) { return LDG_ERR_INVALID; }

    host_id = crosstalk_net_libresteam_local_steam_id_get(ctx);
    if (host_id == NET_LIBRESTEAM_ID_NONE) { return LDG_ERR_NOT_INIT; }

    if (snprintf(host_str, sizeof(host_str), "%" PRIu64, host_id) < 0) { return LDG_ERR_STR_FMT; }

    ret = libresteam_lobby_data_set(ctx->set, net_libresteam_id_make(ctx->lobby_id), CROSSTALK_NET_LOBBY_KEY_START, host_str);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->lobby_host_id = host_id;
    ctx->lobby_state = (uint32_t)CROSSTALK_NET_LIBRESTEAM_LOBBY_STATE_STARTING;

    return LDG_ERR_AOK;
}

crosstalk_net_libresteam_lobby_state_t crosstalk_net_libresteam_lobby_state_get(const crosstalk_net_libresteam_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return CROSSTALK_NET_LIBRESTEAM_LOBBY_STATE_NONE; }

    return (crosstalk_net_libresteam_lobby_state_t)ctx->lobby_state;
}

uint32_t crosstalk_net_libresteam_lobby_member_cunt_get(const crosstalk_net_libresteam_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return CROSSTALK_NET_CUNT_NONE; }

    return ctx->lobby_member_cunt;
}

const crosstalk_net_libresteam_lobby_member_t* crosstalk_net_libresteam_lobby_member_get(const crosstalk_net_libresteam_ctx_t *ctx, uint32_t idx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return 0x0; }

    if (idx >= ctx->lobby_member_cunt) { return 0x0; }

    return &ctx->lobby_member[idx];
}

uint32_t crosstalk_net_libresteam_lobby_list_cunt_get(const crosstalk_net_libresteam_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return CROSSTALK_NET_CUNT_NONE; }

    return ctx->lobby_list_cunt;
}

const crosstalk_net_libresteam_lobby_info_t* crosstalk_net_libresteam_lobby_list_get(const crosstalk_net_libresteam_ctx_t *ctx, uint32_t idx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return 0x0; }

    if (idx >= ctx->lobby_list_cunt) { return 0x0; }

    return &ctx->lobby_list[idx];
}

uint64_t crosstalk_net_libresteam_lobby_host_steam_id_get(crosstalk_net_libresteam_ctx_t *ctx)
{
    libresteam_steam_id_t owner = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(ctx == 0x0)) { return NET_LIBRESTEAM_ID_NONE; }

    if (ctx->lobby_host_id != NET_LIBRESTEAM_ID_NONE) { return ctx->lobby_host_id; }

    if (ctx->lobby_id == NET_LIBRESTEAM_ID_NONE) { return NET_LIBRESTEAM_ID_NONE; }

    if (libresteam_lobby_owner_get(ctx->set, net_libresteam_id_make(ctx->lobby_id), &owner) != LDG_ERR_AOK) { return NET_LIBRESTEAM_ID_NONE; }

    return owner.all;
}

uint64_t crosstalk_net_libresteam_lobby_id_get(const crosstalk_net_libresteam_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return NET_LIBRESTEAM_ID_NONE; }

    return ctx->lobby_id;
}

static void net_libresteam_presence_kv_set(const libresteam_backend_set_t *set, const char *key, const char *val)
{
    uint32_t ret = 0;

    ret = libresteam_friends_rich_presence_set(set, key, val);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "presence: libresteam set failed; key: %.63s; err: %u32", key, ret); }
}

static void net_libresteam_presence_lines_set(const libresteam_backend_set_t *set, const char *op, const char *att, uint8_t in_lobby)
{
    const char *display = 0x0;
    uint8_t op_on = (op != 0x0 && op[0] != LDG_STR_TERM) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    uint8_t att_on = (att != 0x0 && att[0] != LDG_STR_TERM) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    net_libresteam_presence_kv_set(set, CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_OP, op_on == LDG_TRUTH_TRUE ? op : CROSSTALK_NET_LIBRESTEAM_PRESENCE_VAL_CLEAR);
    net_libresteam_presence_kv_set(set, CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_ACTIVITY, att_on == LDG_TRUTH_TRUE ? att : CROSSTALK_NET_LIBRESTEAM_PRESENCE_VAL_CLEAR);

    if (op_on == LDG_TRUTH_TRUE && att_on == LDG_TRUTH_TRUE) { display = in_lobby != LDG_TRUTH_FALSE ? CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_LOBBY_OP_ACT : CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_KB_OP_ACT; }
    else if (op_on == LDG_TRUTH_TRUE) { display = in_lobby != LDG_TRUTH_FALSE ? CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_LOBBY_OP : CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_KB_OP; }
    else { display = in_lobby != LDG_TRUTH_FALSE ? CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_LOBBY_ACT : CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_KB_ACT; }

    net_libresteam_presence_kv_set(set, CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_DISPLAY, display);
}

static void net_libresteam_presence_players_set(const libresteam_backend_set_t *set, uint32_t players, uint32_t players_max, uint8_t in_lobby)
{
    char players_str[CROSSTALK_NET_LIBRESTEAM_PRESENCE_NUM_CAP] = LDG_ARR_ZERO_INIT;
    char max_str[CROSSTALK_NET_LIBRESTEAM_PRESENCE_NUM_CAP] = LDG_ARR_ZERO_INIT;

    if (in_lobby == LDG_TRUTH_FALSE && players <= CROSSTALK_NET_LIBRESTEAM_PRESENCE_PLAYERS_SOLO)
    {
        net_libresteam_presence_kv_set(set, CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_DISPLAY, CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_KB_SOLO);
        return;
    }

    if (snprintf(players_str, sizeof(players_str), "%" PRIu32, players) < 0) { return; }

    net_libresteam_presence_kv_set(set, CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_PLAYERS, players_str);

    if (in_lobby == LDG_TRUTH_FALSE)
    {
        net_libresteam_presence_kv_set(set, CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_DISPLAY, CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_KB);
        return;
    }

    if (snprintf(max_str, sizeof(max_str), "%" PRIu32, players_max) < 0) { return; }

    net_libresteam_presence_kv_set(set, CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_PLAYERS_MAX, max_str);
    net_libresteam_presence_kv_set(set, CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_DISPLAY, CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_LOBBY);
}

static void net_libresteam_presence_menu_set(const libresteam_backend_set_t *set, const char *menu, const char *display)
{
    if (menu != 0x0) { net_libresteam_presence_kv_set(set, CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_MENU, menu); }

    net_libresteam_presence_kv_set(set, CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_DISPLAY, display);
}

static void net_libresteam_presence_group_set(const libresteam_backend_set_t *set, uint64_t group_id, uint32_t players)
{
    char group_str[CROSSTALK_NET_LIBRESTEAM_PRESENCE_ID_CAP] = LDG_ARR_ZERO_INIT;
    char size_str[CROSSTALK_NET_LIBRESTEAM_PRESENCE_NUM_CAP] = LDG_ARR_ZERO_INIT;

    if (group_id == CROSSTALK_NET_LIBRESTEAM_PRESENCE_GROUP_CLEAR || players == 0)
    {
        net_libresteam_presence_kv_set(set, CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_GROUP, CROSSTALK_NET_LIBRESTEAM_PRESENCE_VAL_CLEAR);
        net_libresteam_presence_kv_set(set, CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_GROUP_SIZE, CROSSTALK_NET_LIBRESTEAM_PRESENCE_VAL_CLEAR);
        return;
    }

    if (snprintf(group_str, sizeof(group_str), "%" PRIu64, group_id) < 0) { return; }

    if (snprintf(size_str, sizeof(size_str), "%" PRIu32, players) < 0) { return; }

    net_libresteam_presence_kv_set(set, CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_GROUP, group_str);
    net_libresteam_presence_kv_set(set, CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_GROUP_SIZE, size_str);
}

static void net_libresteam_presence_connect_set(const libresteam_backend_set_t *set, uint64_t lobby_id)
{
    char connect[CROSSTALK_NET_LIBRESTEAM_PRESENCE_CONNECT_CAP] = LDG_ARR_ZERO_INIT;

    if (lobby_id == 0)
    {
        net_libresteam_presence_kv_set(set, CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_CONNECT, CROSSTALK_NET_LIBRESTEAM_PRESENCE_VAL_CLEAR);
        return;
    }

    if (snprintf(connect, sizeof(connect), CROSSTALK_NET_LIBRESTEAM_PRESENCE_CONNECT_FMT, lobby_id) < 0) { return; }

    net_libresteam_presence_kv_set(set, CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_CONNECT, connect);
}

uint32_t crosstalk_net_libresteam_presence_push(void *backend_ctx, const crosstalk_net_presence_desc_t *desc)
{
    const libresteam_backend_set_t *set = (const libresteam_backend_set_t *)backend_ctx;
    uint8_t in_lobby = LDG_TRUTH_FALSE;
    uint8_t lines_on = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(set == 0x0 || desc == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    in_lobby = desc->screen == (uint32_t)CROSSTALK_NET_PRESENCE_SCREEN_LOBBY ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    lines_on = ((desc->op != 0x0 && desc->op[0] != LDG_STR_TERM) || (desc->att != 0x0 && desc->att[0] != LDG_STR_TERM)) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    net_libresteam_presence_connect_set(set, desc->connect_lobby_id);
    net_libresteam_presence_group_set(set, desc->group_id, desc->members);

    switch (desc->screen)
    {
        case CROSSTALK_NET_PRESENCE_SCREEN_SANDBOX:
        case CROSSTALK_NET_PRESENCE_SCREEN_LOBBY:
            if (lines_on == LDG_TRUTH_TRUE) { net_libresteam_presence_lines_set(set, desc->op, desc->att, in_lobby); }
            else if (desc->paused == LDG_TRUTH_TRUE) { net_libresteam_presence_kv_set(set, CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_DISPLAY, in_lobby == LDG_TRUTH_TRUE ? CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_LOBBY_PAUSED : CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_KB_PAUSED); }
            else { net_libresteam_presence_players_set(set, desc->members, desc->members_max, in_lobby); }

            break;
        case CROSSTALK_NET_PRESENCE_SCREEN_LD:
            net_libresteam_presence_menu_set(set, 0x0, CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_LD);
            break;
        case CROSSTALK_NET_PRESENCE_SCREEN_BROWSE:
            net_libresteam_presence_menu_set(set, 0x0, CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_BROWSE);
            break;
        case CROSSTALK_NET_PRESENCE_SCREEN_NEWDESIGN:
            net_libresteam_presence_menu_set(set, CROSSTALK_NET_LIBRESTEAM_PRESENCE_MENU_NEW, CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_MENU);
            break;
        case CROSSTALK_NET_PRESENCE_SCREEN_NEWPROJ:
            net_libresteam_presence_kv_set(set, CROSSTALK_NET_LIBRESTEAM_PRESENCE_KEY_ACTIVITY, desc->proj_reverse == LDG_TRUTH_TRUE ? CROSSTALK_NET_LIBRESTEAM_PRESENCE_PROJ_REVERSE : CROSSTALK_NET_LIBRESTEAM_PRESENCE_PROJ_RTL);
            net_libresteam_presence_menu_set(set, 0x0, CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_NEWPROJ);
            break;
        case CROSSTALK_NET_PRESENCE_SCREEN_EXTRAS:
            net_libresteam_presence_menu_set(set, 0x0, CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_EXTRAS);
            break;
        case CROSSTALK_NET_PRESENCE_SCREEN_CREDITS:
            net_libresteam_presence_menu_set(set, 0x0, CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_CREDITS);
            break;
        case CROSSTALK_NET_PRESENCE_SCREEN_SETTINGS:
            net_libresteam_presence_menu_set(set, CROSSTALK_NET_LIBRESTEAM_PRESENCE_MENU_SETTINGS, CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_MENU);
            break;
        default:
            net_libresteam_presence_menu_set(set, CROSSTALK_NET_LIBRESTEAM_PRESENCE_MENU_MAIN, CROSSTALK_NET_LIBRESTEAM_PRESENCE_DISPLAY_MENU);
            break;
    }

    return LDG_ERR_AOK;
}
