#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <dangling/logging/logging.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/str/str.h>
#include <dangling/net/steamworks.h>
#include <dangling/net/steamworks_lobby.h>
#include <dangling/net/steamworks_srv.h>
#include <dangling/net/steamworks_srv_browser.h>

#include "core/log.h"
#include "net/steam.h"
#include "net/presence.h"

#define STEAM_NET_NOTICE_TTL 4.0
#define STEAM_NET_P2P_VIRTUAL_PORT 0
#define STEAM_NET_LOBBY_MAP "crosstalk"
#define STEAM_NET_LOBBY_REV "1"
#define STEAM_NET_CONNECT_LOBBY_ARG "+connect_lobby "
#define STEAM_NET_CONNECT_SCAN_CUNT 1

void crosstalk_net_steam_notice_push(steam_net_ctx_t *ctx, const char *text)
{
    steam_net_notice_t *slot = 0x0;
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
    if (rc != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "steam_net: notice truncated; rc: %u32", rc); }

    slot->ttl = STEAM_NET_NOTICE_TTL;
}

uint8_t crosstalk_net_steam_notice_active_is(const steam_net_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->notice_cunt == 0) { return LDG_TRUTH_FALSE; }

    return ctx->notice[ctx->notice_hd].ttl > 0.0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

const char* crosstalk_net_steam_notice_text_get(const steam_net_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx) || ctx->notice_cunt == 0) { return 0x0; }

    return ctx->notice[ctx->notice_hd].text;
}

static void steam_net_notice_tick(steam_net_ctx_t *ctx, double dt)
{
    steam_net_notice_t *slot = 0x0;

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

static void steam_net_on_conn(ldg_net_ctx_t *net, uint32_t conn_id)
{
    const char *name = 0x0;

    if (LDG_UNLIKELY(!net)) { return; }

    name = ldg_net_steamworks_player_name_get(net, conn_id);
    if (name == 0x0 || name[0] == 0) { name = "player"; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "steam_net: player joined; name: %s; conn: %u32", name, conn_id);
}

static void steam_net_on_dc(ldg_net_ctx_t *net, uint32_t conn_id)
{
    const char *name = 0x0;

    if (LDG_UNLIKELY(!net)) { return; }

    name = ldg_net_steamworks_player_name_get(net, conn_id);
    if (name == 0x0 || name[0] == 0) { name = "player"; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "steam_net: player left; name: %s; conn: %u32", name, conn_id);
}

uint32_t crosstalk_net_steam_init(steam_net_ctx_t *ctx, ldg_net_steamworks_ctx_t *sesh, ldg_net_cb_t *cb)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !sesh || !cb)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0, sizeof(*ctx));
    ctx->sesh = sesh;
    ctx->cb = cb;
    cb->on_conn = steam_net_on_conn;
    cb->on_dc = steam_net_on_dc;
    cb->on_data = 0x0;
    cb->on_chat = 0x0;
    cb->on_voice = 0x0;

    ret = ldg_net_steamworks_lobby_init(&ctx->lobby, STEAM_NET_LOBBY_MAP, STEAM_NET_LOBBY_REV);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_net_steamworks_hub_init(&ctx->hub);
    if (ret != LDG_ERR_AOK)
    {
        ldg_net_steamworks_lobby_shutdown(&ctx->lobby);
        return ret;
    }

    ctx->hub.lobby = &ctx->lobby;

    ret = ldg_net_steamworks_join_init(&ctx->join);
    if (ret != LDG_ERR_AOK)
    {
        ldg_net_steamworks_lobby_shutdown(&ctx->lobby);
        return ret;
    }

    ctx->hub.join = &ctx->join;

    ret = ldg_net_steamworks_friend_call_init(&ctx->friend_call);
    if (ret == LDG_ERR_AOK) { ctx->hub.friend_call = &ctx->friend_call; }
    else { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "steam_net: friend call init failed; err: %u32", ret); }

    ret = ldg_mem_alloc((uint64_t)CROSSTALK_NET_SNAP_CAP, (void **)&ctx->snap_buff);
    if (ret != LDG_ERR_AOK)
    {
        ldg_net_steamworks_lobby_shutdown(&ctx->lobby);
        return ret;
    }

    ret = ldg_mem_alloc((uint64_t)sizeof(ldg_net_steamworks_friend_msg_ctx_t), (void **)&ctx->friend_msg);
    if (ret == LDG_ERR_AOK) { ret = ldg_net_steamworks_friend_msg_init(ctx->friend_msg); }

    if (ret == LDG_ERR_AOK)
    {
        ctx->hub.friend_msg = ctx->friend_msg;
        ret = ldg_net_steamworks_friend_msg_listen_set(LDG_TRUTH_TRUE);
        if (ret == LDG_ERR_AOK) { ctx->friend_msg_live = LDG_TRUTH_TRUE; }
        else { ctx->hub.friend_msg = 0x0; }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "steam_net: friend msg listen; err: %u32", ret);
    }
    else { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "steam_net: friend msg ring alloc failed; err: %u32", ret); }

    ctx->snap_cap = CROSSTALK_NET_SNAP_CAP;
    ctx->lobby_live = LDG_TRUTH_TRUE;
    ctx->lan_port = LDG_NET_PORT_DEFAULT;
    ctx->lan_sesh = (uint8_t)CROSSTALK_NET_LAN_SESH_NONE;

    ret = ldg_net_steamworks_srv_browser_init(&ctx->lan_browser);
    if (ret == LDG_ERR_AOK) { ctx->lan_browser_ok = LDG_TRUTH_TRUE; }
    else
    {
        ctx->lan_browser_ok = LDG_TRUTH_FALSE;
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "steam_net: lan browser init failed; err: %u32", ret);
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_steam_friend_msg_get(steam_net_ctx_t *ctx, uint64_t *out_steam_id, const char **out_text, uint32_t *out_len)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !out_steam_id || !out_text || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_steam_id = 0;
    *out_text = 0x0;
    *out_len = 0;

    if (ctx->friend_msg_live != LDG_TRUTH_TRUE || ctx->friend_msg == 0x0) { return LDG_ERR_EMPTY; }

    ret = ldg_net_steamworks_friend_msg_get(ctx->friend_msg, &ctx->friend_msg_buff);
    if (ret != LDG_ERR_AOK) { return ret; }

    *out_steam_id = ctx->friend_msg_buff.steam_id;
    *out_text = ctx->friend_msg_buff.text;
    *out_len = ctx->friend_msg_buff.len;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_steam_friend_msg_reply(uint64_t steam_id_friend, const char *text)
{
    return ldg_net_steamworks_friend_msg_reply(steam_id_friend, text);
}

void crosstalk_net_steam_shutdown(steam_net_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return; }

    crosstalk_net_steam_sesh_stop(ctx);
    if (ctx->lan_browser_ok == LDG_TRUTH_TRUE)
    {
        ldg_net_steamworks_srv_browser_cancel(&ctx->lan_browser);
        ldg_net_steamworks_srv_browser_shutdown(&ctx->lan_browser);
        ctx->lan_browser_ok = LDG_TRUTH_FALSE;
    }

    if (ctx->friend_msg_live != 0)
    {
        ret = ldg_net_steamworks_friend_msg_listen_set(LDG_TRUTH_FALSE);

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "steam_net: friend msg listen off; err: %u32; drops: %u32", ret, ldg_net_steamworks_friend_msg_drop_cunt_get(ctx->friend_msg));
        ctx->friend_msg_live = 0;
    }

    ctx->hub.friend_call = 0x0;
    ctx->friend_cunt = CROSSTALK_NET_CUNT_NONE;

    ctx->hub.friend_msg = 0x0;
    if (ctx->friend_msg != 0x0)
    {
        ldg_mem_dealloc(ctx->friend_msg);
        ctx->friend_msg = 0x0;
    }

    if (ctx->lobby_live != 0)
    {
        ldg_net_steamworks_lobby_leave(&ctx->lobby);
        ldg_net_steamworks_lobby_shutdown(&ctx->lobby);
        ctx->lobby_live = 0;
    }

    if (ctx->snap_buff != 0x0)
    {
        ldg_mem_dealloc(ctx->snap_buff);
        ctx->snap_buff = 0x0;
        ctx->snap_cap = 0;
        ctx->snap_len = 0;
        ctx->snap_total = 0;
    }

    ctx->sesh = 0x0;
    ctx->cb = 0x0;
    ctx->sesh_live = 0;
}

static void steam_net_friend_detail_fill(uint64_t steam_id, char *out, uint32_t cap)
{
    char display[CROSSTALK_NET_FRIEND_DETAIL_CAP] = LDG_ARR_ZERO_INIT;
    char players[CROSSTALK_NET_FRIEND_DETAIL_CAP] = LDG_ARR_ZERO_INIT;
    char players_max[CROSSTALK_NET_FRIEND_DETAIL_CAP] = LDG_ARR_ZERO_INIT;

    out[0] = LDG_STR_TERM;

    if (ldg_net_steamworks_friend_presence_get(steam_id, CROSSTALK_NET_PRESENCE_KEY_DISPLAY, display, (uint64_t)sizeof(display)) != LDG_ERR_AOK)
    {
        snprintf(out, cap, "%.47s", CROSSTALK_NET_FRIEND_DETAIL_OURS);
        return;
    }

    if (strcmp(display, CROSSTALK_NET_PRESENCE_DISPLAY_MENU) == 0)
    {
        snprintf(out, cap, "%.47s", CROSSTALK_NET_FRIEND_DETAIL_MENU);
        return;
    }

    if (strcmp(display, CROSSTALK_NET_PRESENCE_DISPLAY_KB) == 0 || strcmp(display, CROSSTALK_NET_PRESENCE_DISPLAY_KB_SOLO) == 0)
    {
        snprintf(out, cap, "%.47s", CROSSTALK_NET_FRIEND_DETAIL_SANDBOX);
        return;
    }

    if (strcmp(display, CROSSTALK_NET_PRESENCE_DISPLAY_LOBBY) != 0)
    {
        snprintf(out, cap, "%.47s", CROSSTALK_NET_FRIEND_DETAIL_OURS);
        return;
    }

    if (ldg_net_steamworks_friend_presence_get(steam_id, CROSSTALK_NET_PRESENCE_KEY_PLAYERS, players, (uint64_t)sizeof(players)) != LDG_ERR_AOK || ldg_net_steamworks_friend_presence_get(steam_id, CROSSTALK_NET_PRESENCE_KEY_PLAYERS_MAX, players_max, (uint64_t)sizeof(players_max)) != LDG_ERR_AOK)
    {
        snprintf(out, cap, "%.47s", CROSSTALK_NET_FRIEND_DETAIL_LOBBY);
        return;
    }

    snprintf(out, cap, "%.15s (%.7s/%.7s)", CROSSTALK_NET_FRIEND_DETAIL_LOBBY, players, players_max);
}

static uint8_t steam_net_friend_ours_prev_is(const steam_net_ctx_t *ctx, uint64_t steam_id)
{
    for (uint32_t idx = 0; idx < ctx->friend_ours_prev_cunt; idx++) { if (ctx->friend_ours_prev[idx] == steam_id) { return LDG_TRUTH_TRUE; } }

    return LDG_TRUTH_FALSE;
}

static void steam_net_friends_refresh(steam_net_ctx_t *ctx)
{
    uint64_t steam_id[CROSSTALK_NET_FRIEND_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t ours_cunt = 0;
    uint32_t got = 0;
    uint32_t cunt = 0;
    uint32_t ret = 0;

    ret = ldg_net_steamworks_friend_list_get(LDG_NET_STEAMWORKS_FRIEND_FLAG_IMM, steam_id, (uint32_t)CROSSTALK_NET_FRIEND_CAP, &got);
    if (ret != LDG_ERR_AOK && ret != LDG_ERR_FULL)
    {
        ctx->friend_cunt = CROSSTALK_NET_CUNT_NONE;
        return;
    }

    for (uint32_t idx = 0; idx < got && cunt < (uint32_t)CROSSTALK_NET_FRIEND_CAP; idx++)
    {
        ldg_net_steamworks_friend_game_t game = LDG_STRUCT_ZERO_INIT;
        const char *persona = 0x0;

        ctx->friend_list[cunt].steam_id = steam_id[idx];
        ctx->friend_list[cunt].lobby_id = CROSSTALK_NET_LOBBY_ID_NONE;
        ctx->friend_list[cunt].name[0] = LDG_STR_TERM;
        ctx->friend_list[cunt].state = (uint32_t)ldg_net_steamworks_friend_persona_state_get(steam_id[idx]);
        ctx->friend_list[cunt].in_game = LDG_TRUTH_FALSE;
        ctx->friend_list[cunt].in_ours = LDG_TRUTH_FALSE;
        ctx->friend_list[cunt].started = LDG_TRUTH_FALSE;
        ctx->friend_list[cunt].detail[0] = LDG_STR_TERM;

        persona = ldg_net_steamworks_friend_name_get(steam_id[idx]);
        if (persona != 0x0 && persona[0] != LDG_STR_TERM)
        {
            ret = ldg_str_rbrcpy(ctx->friend_list[cunt].name, persona, (uint64_t)sizeof(ctx->friend_list[cunt].name));
            if (ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC) { ctx->friend_list[cunt].name[0] = LDG_STR_TERM; }
        }

        if (ldg_net_steamworks_friend_game_get(steam_id[idx], &game) == LDG_ERR_AOK)
        {
            ctx->friend_list[cunt].in_game = LDG_TRUTH_TRUE;
            ctx->friend_list[cunt].lobby_id = game.lobby_id;

            if ((uint32_t)(game.game_id & CROSSTALK_NET_STEAM_APPID_MASK) == CROSSTALK_NET_STEAM_APP_ID)
            {
                ctx->friend_list[cunt].in_ours = LDG_TRUTH_TRUE;
                ctx->friend_list[cunt].started = (uint8_t)((steam_net_friend_ours_prev_is(ctx, steam_id[idx]) == LDG_TRUTH_TRUE) ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE);
                steam_net_friend_detail_fill(steam_id[idx], ctx->friend_list[cunt].detail, (uint32_t)sizeof(ctx->friend_list[cunt].detail));
            }
            else { snprintf(ctx->friend_list[cunt].detail, sizeof(ctx->friend_list[cunt].detail), "%.47s", CROSSTALK_NET_FRIEND_DETAIL_OTHER); }
        }

        cunt++;
    }

    if (cunt != ctx->friend_cunt) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "steam_net: friends refreshed; cunt: %u32", cunt); }

    ctx->friend_cunt = cunt;

    for (uint32_t idx = 0; idx < cunt && ours_cunt < (uint32_t)CROSSTALK_NET_FRIEND_CAP; idx++)
    {
        if (ctx->friend_list[idx].in_ours != LDG_TRUTH_TRUE) { continue; }

        ctx->friend_ours_prev[ours_cunt] = ctx->friend_list[idx].steam_id;
        ours_cunt++;
    }

    ctx->friend_ours_prev_cunt = ours_cunt;
}

uint32_t crosstalk_net_steam_friend_cunt_get(const steam_net_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return CROSSTALK_NET_CUNT_NONE; }

    return ctx->friend_cunt;
}

const crosstalk_net_steam_friend_t* crosstalk_net_steam_friend_get(const steam_net_ctx_t *ctx, uint32_t idx)
{
    if (LDG_UNLIKELY(!ctx)) { return 0x0; }

    if (LDG_UNLIKELY(idx >= ctx->friend_cunt)) { return 0x0; }

    return &ctx->friend_list[idx];
}

uint8_t crosstalk_net_steam_friend_started_take(steam_net_ctx_t *ctx, uint32_t idx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_TRUTH_FALSE; }

    if (LDG_UNLIKELY(idx >= ctx->friend_cunt)) { return LDG_TRUTH_FALSE; }

    if (ctx->friend_list[idx].started != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    ctx->friend_list[idx].started = LDG_TRUTH_FALSE;

    return LDG_TRUTH_TRUE;
}

const char* crosstalk_net_steam_persona_state_name_get(uint32_t state)
{
    if (state == (uint32_t)LDG_NET_STEAMWORKS_PERSONA_STATE_ONLINE) { return "online"; }

    if (state == (uint32_t)LDG_NET_STEAMWORKS_PERSONA_STATE_BUSY) { return "busy"; }

    if (state == (uint32_t)LDG_NET_STEAMWORKS_PERSONA_STATE_AWAY) { return "away"; }

    if (state == (uint32_t)LDG_NET_STEAMWORKS_PERSONA_STATE_SNOOZE) { return "snooze"; }

    if (state == (uint32_t)LDG_NET_STEAMWORKS_PERSONA_STATE_TRADE) { return "looking to trade"; }

    if (state == (uint32_t)LDG_NET_STEAMWORKS_PERSONA_STATE_PLAY) { return "looking to play"; }

    if (state == (uint32_t)LDG_NET_STEAMWORKS_PERSONA_STATE_INVISIBLE) { return "invisible"; }

    return "offline";
}

void crosstalk_net_steam_poll(steam_net_ctx_t *ctx, double dt)
{
    if (LDG_UNLIKELY(!ctx)) { return; }

    ldg_net_steamworks_tick(&ctx->hub);

    if (ctx->lobby_live != 0) { ldg_net_steamworks_lobby_member_refresh(&ctx->lobby); }

    if (ctx->lan_browser_ok == LDG_TRUTH_TRUE) { ldg_net_steamworks_srv_browser_poll(&ctx->lan_browser); }

    if (ctx->friend_next > 0.0) { ctx->friend_next -= dt; }
    else
    {
        steam_net_friends_refresh(ctx);
        ctx->friend_next = CROSSTALK_NET_FRIEND_REFRESH_INTERVAL;
    }

    if (ctx->sesh != 0x0 && ctx->cb != 0x0 && ldg_net_steamworks_status_get(ctx->sesh) != LDG_NET_CONN_STATUS_DCD) { ldg_net_steamworks_poll(ctx->sesh, ctx->cb, 0); }

    steam_net_notice_tick(ctx, dt);
}

uint32_t crosstalk_net_steam_join_req_get(steam_net_ctx_t *ctx, uint64_t *out_lobby_id)
{
    char connect[LDG_NET_STEAMWORKS_JOIN_CONNECT_MAX + LDG_STR_TERM_SIZE] = LDG_ARR_ZERO_INIT;
    ldg_net_steamworks_join_state_t state = LDG_NET_STEAMWORKS_JOIN_STATE_NONE;
    uint64_t prefix_len = 0;

    if (LDG_UNLIKELY(!ctx || !out_lobby_id)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_lobby_id = 0;
    state = ldg_net_steamworks_join_state_get(&ctx->join);

    if (state == LDG_NET_STEAMWORKS_JOIN_STATE_LOBBY) { return ldg_net_steamworks_join_lobby_get(&ctx->join, out_lobby_id); }

    if (state != LDG_NET_STEAMWORKS_JOIN_STATE_CONNECT) { return LDG_ERR_EMPTY; }

    if (ldg_net_steamworks_join_connect_get(&ctx->join, connect, (uint64_t)sizeof(connect)) != LDG_ERR_AOK) { return LDG_ERR_EMPTY; }

    prefix_len = (uint64_t)strlen(STEAM_NET_CONNECT_LOBBY_ARG);

    if (strncmp(connect, STEAM_NET_CONNECT_LOBBY_ARG, prefix_len) != 0) { return LDG_ERR_INVALID; }

    if (sscanf(&connect[prefix_len], "%" SCNu64, out_lobby_id) != STEAM_NET_CONNECT_SCAN_CUNT) { return LDG_ERR_INVALID; }

    if (*out_lobby_id == 0) { return LDG_ERR_INVALID; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_steam_host_start(steam_net_ctx_t *ctx, int32_t virtual_port)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !ctx->sesh)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (virtual_port < 0) { virtual_port = STEAM_NET_P2P_VIRTUAL_PORT; }

    crosstalk_net_steam_sesh_stop(ctx);
    ret = ldg_net_steamworks_srv_p2p_start(ctx->sesh, (uint32_t)virtual_port);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->sesh_live = 1;
    ctx->lan_sesh = (uint8_t)CROSSTALK_NET_LAN_SESH_NONE;
    crosstalk_net_steam_notice_push(ctx, "hosting session");
    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_steam_client_join(steam_net_ctx_t *ctx, uint64_t host_steam_id, int32_t virtual_port)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !ctx->sesh)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(host_steam_id == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (virtual_port < 0) { virtual_port = STEAM_NET_P2P_VIRTUAL_PORT; }

    crosstalk_net_steam_sesh_stop(ctx);
    ret = ldg_net_steamworks_client_p2p_conn(ctx->sesh, host_steam_id, (uint32_t)virtual_port);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->sesh_live = 1;
    ctx->lan_sesh = (uint8_t)CROSSTALK_NET_LAN_SESH_NONE;
    crosstalk_net_steam_notice_push(ctx, "joining session");
    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_steam_lan_host_start(steam_net_ctx_t *ctx, uint16_t port, uint32_t player_max, const char *map, const char *name)
{
    ldg_net_steamworks_srv_conf_t conf = LDG_STRUCT_ZERO_INIT;
    const char *srv_name = 0x0;
    const char *srv_map = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || ctx->sesh == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (port == 0) { port = LDG_NET_PORT_DEFAULT; }

    if (player_max == 0) { player_max = CROSSTALK_NET_LOBBY_MAX_MEMBERS; }

    srv_name = (name != 0x0 && name[0] != LDG_STR_TERM) ? name : CROSSTALK_NET_LAN_PRODUCT;
    srv_map = (map != 0x0 && map[0] != LDG_STR_TERM) ? map : STEAM_NET_LOBBY_MAP;

    crosstalk_net_steam_sesh_stop(ctx);

    conf.product = CROSSTALK_NET_LAN_PRODUCT;
    conf.mod_dir = CROSSTALK_NET_LAN_MOD_DIR;
    conf.name = srv_name;
    conf.desc = CROSSTALK_NET_LAN_DESC;
    conf.map = srv_map;
    conf.tag = srv_map;
    conf.rev = CROSSTALK_NET_LAN_REV;
    conf.player_max = player_max;
    conf.bot_cunt = 0;
    conf.password = 0;
    conf.advertise = 1;

    ret = ldg_net_steamworks_srv_init(port, &conf);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "steam_net: lan game server init failed; err: %u32; port: %u32", ret, (uint32_t)port);
        return ret;
    }

    ctx->lan_srv_ok = LDG_TRUTH_TRUE;
    ctx->lan_port = port;

    ret = ldg_net_steamworks_srv_start(ctx->sesh, port);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "steam_net: lan listen failed; err: %u32; port: %u32", ret, (uint32_t)port);
        ctx->lan_srv_ok = LDG_TRUTH_FALSE;
        return ret;
    }

    ctx->sesh_live = 1;
    ctx->lan_sesh = (uint8_t)CROSSTALK_NET_LAN_SESH_HOSTING;
    crosstalk_net_steam_notice_push(ctx, "hosting LAN session");
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "steam_net: lan host; port: %u32; max: %u32; map: %.31s", (uint32_t)port, player_max, srv_map);
    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_steam_lan_client_join(steam_net_ctx_t *ctx, uint32_t ip, uint16_t port)
{
    char host[CROSSTALK_NET_LAN_HOST_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || ctx->sesh == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ip == 0 || port == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    snprintf(host, sizeof(host), "%u.%u.%u.%u", (ip >> 24) & 0xFFU, (ip >> 16) & 0xFFU, (ip >> 8) & 0xFFU, ip & 0xFFU);

    crosstalk_net_steam_sesh_stop(ctx);
    ret = ldg_net_steamworks_client_conn(ctx->sesh, host, port);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "steam_net: lan join failed; err: %u32; host: %s; port: %u32", ret, host, (uint32_t)port);
        return ret;
    }

    ctx->sesh_live = 1;
    ctx->lan_sesh = (uint8_t)CROSSTALK_NET_LAN_SESH_JOINED;
    crosstalk_net_steam_notice_push(ctx, "joining LAN session");
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "steam_net: lan join; host: %s; port: %u32", host, (uint32_t)port);
    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_steam_lan_list_req(steam_net_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->lan_browser_ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ret = ldg_net_steamworks_srv_browser_lan_refresh(&ctx->lan_browser);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "steam_net: lan list req; err: %u32", ret);
    return ret;
}

uint32_t crosstalk_net_steam_lan_list_cunt_get(const steam_net_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0 || ctx->lan_browser_ok != LDG_TRUTH_TRUE)) { return 0; }

    return ldg_net_steamworks_srv_browser_cunt_get((ldg_net_steamworks_srv_browser_ctx_t *)&ctx->lan_browser);
}

const ldg_net_srv_browser_entry_t* crosstalk_net_steam_lan_list_get(const steam_net_ctx_t *ctx, uint32_t idx)
{
    if (LDG_UNLIKELY(ctx == 0x0 || ctx->lan_browser_ok != LDG_TRUTH_TRUE)) { return 0x0; }

    return ldg_net_steamworks_srv_browser_entry_get((ldg_net_steamworks_srv_browser_ctx_t *)&ctx->lan_browser, idx);
}

uint8_t crosstalk_net_steam_lan_sesh_get(const steam_net_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return (uint8_t)CROSSTALK_NET_LAN_SESH_NONE; }

    return ctx->lan_sesh;
}

void crosstalk_net_steam_sesh_stop(steam_net_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx || !ctx->sesh)) { return; }

    if (ctx->sesh_live == 0 && ldg_net_steamworks_status_get(ctx->sesh) == LDG_NET_CONN_STATUS_DCD)
    {
        ctx->lan_sesh = (uint8_t)CROSSTALK_NET_LAN_SESH_NONE;
        return;
    }

    if (ctx->sesh->net.is_srv != 0) { ldg_net_steamworks_srv_stop(ctx->sesh); }
    else { ldg_net_steamworks_client_dc(ctx->sesh); }

    ctx->sesh_live = 0;
    ctx->lan_sesh = (uint8_t)CROSSTALK_NET_LAN_SESH_NONE;
}

ldg_net_conn_status_t crosstalk_net_steam_status_get(const steam_net_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx || !ctx->sesh)) { return LDG_NET_CONN_STATUS_DCD; }

    return ldg_net_steamworks_status_get(ctx->sesh);
}

uint32_t crosstalk_net_steam_client_cunt_get(const steam_net_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx || !ctx->sesh)) { return 0; }

    return ldg_net_steamworks_client_cunt_get(ctx->sesh);
}

const char* crosstalk_net_steam_client_name_get(const steam_net_ctx_t *ctx, uint32_t client_id)
{
    if (LDG_UNLIKELY(!ctx || !ctx->sesh)) { return 0x0; }

    return ldg_net_steamworks_player_name_get(&ctx->sesh->net, client_id);
}

uint32_t crosstalk_net_steam_lobby_mode_set(steam_net_ctx_t *ctx, const char *mode)
{
    if (LDG_UNLIKELY(!ctx || !mode)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ldg_str_rbrcpy(ctx->lobby.map, mode, (uint64_t)sizeof(ctx->lobby.map));
}

uint32_t crosstalk_net_steam_lobby_create(steam_net_ctx_t *ctx, int32_t max_members)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (max_members <= 0) { max_members = CROSSTALK_NET_LOBBY_MAX_MEMBERS; }

    return ldg_net_steamworks_lobby_create(&ctx->lobby, (uint32_t)max_members);
}

uint32_t crosstalk_net_steam_lobby_list_req(steam_net_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ldg_net_steamworks_lobby_list_req(&ctx->lobby);
}

uint32_t crosstalk_net_steam_lobby_join(steam_net_ctx_t *ctx, uint64_t lobby_id)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ldg_net_steamworks_lobby_join(&ctx->lobby, lobby_id);
}

void crosstalk_net_steam_lobby_leave(steam_net_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return; }

    ldg_net_steamworks_lobby_leave(&ctx->lobby);
}

void crosstalk_net_steam_lobby_rdy_set(steam_net_ctx_t *ctx, uint8_t rdy)
{
    if (LDG_UNLIKELY(!ctx)) { return; }

    ldg_net_steamworks_lobby_rdy_set(&ctx->lobby, rdy);
}

void crosstalk_net_steam_lobby_busy_set(steam_net_ctx_t *ctx, uint8_t busy)
{
    if (LDG_UNLIKELY(!ctx)) { return; }

    ldg_net_steamworks_lobby_team_set(&ctx->lobby, busy);
}

uint32_t crosstalk_net_steam_lobby_game_start(steam_net_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ldg_net_steamworks_lobby_game_start(&ctx->lobby);
}

ldg_net_lobby_state_t crosstalk_net_steam_lobby_state_get(const steam_net_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_NET_LOBBY_STATE_NONE; }

    return ctx->lobby.lobby.state;
}

uint32_t crosstalk_net_steam_lobby_member_cunt_get(const steam_net_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return 0; }

    return ctx->lobby.lobby.member_cunt;
}

const ldg_net_lobby_member_t* crosstalk_net_steam_lobby_member_get(const steam_net_ctx_t *ctx, uint32_t idx)
{
    if (LDG_UNLIKELY(!ctx)) { return 0x0; }

    if (idx >= ctx->lobby.lobby.member_cunt) { return 0x0; }

    return &ctx->lobby.lobby.member[idx];
}

uint32_t crosstalk_net_steam_lobby_list_cunt_get(const steam_net_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return 0; }

    return ctx->lobby.lobby.list_cunt;
}

const ldg_net_lobby_info_t* crosstalk_net_steam_lobby_list_get(const steam_net_ctx_t *ctx, uint32_t idx)
{
    if (LDG_UNLIKELY(!ctx)) { return 0x0; }

    if (idx >= ctx->lobby.lobby.list_cunt) { return 0x0; }

    return &ctx->lobby.lobby.list[idx];
}

uint64_t crosstalk_net_steam_lobby_host_steam_id_get(steam_net_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return 0; }

    return ldg_net_steamworks_lobby_host_steam_id_get(&ctx->lobby);
}

uint64_t crosstalk_net_steam_lobby_id_get(const steam_net_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return 0; }

    return ctx->lobby.lobby.curr_lobby;
}
