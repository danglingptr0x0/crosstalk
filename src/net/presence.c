#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/logging/logging.h>

#include <libresteam/friends/friends.h>

#include "core/log.h"
#include "net/presence.h"

static void presence_kv_set(const libresteam_backend_set_t *set, const char *key, const char *val)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(set == 0x0 || key == 0x0 || val == 0x0)) { return; }

    ret = libresteam_friends_rich_presence_set(set, key, val);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "presence: set failed; key: %.63s; err: %u32", key, ret); }
}

void crosstalk_net_presence_set(const libresteam_backend_set_t *set, const char *key, const char *val)
{
    presence_kv_set(set, key, val);
}

void crosstalk_net_presence_paused_set(const libresteam_backend_set_t *set, uint8_t in_lobby)
{
    presence_kv_set(set, CROSSTALK_NET_PRESENCE_KEY_DISPLAY, in_lobby != LDG_TRUTH_FALSE ? CROSSTALK_NET_PRESENCE_DISPLAY_LOBBY_PAUSED : CROSSTALK_NET_PRESENCE_DISPLAY_KB_PAUSED);
}

void crosstalk_net_presence_lines_set(const libresteam_backend_set_t *set, const char *op, const char *att, uint8_t in_lobby)
{
    const char *display = 0x0;
    uint8_t op_on = (op != 0x0 && op[0] != LDG_STR_TERM) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    uint8_t att_on = (att != 0x0 && att[0] != LDG_STR_TERM) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (op_on != LDG_TRUTH_TRUE && att_on != LDG_TRUTH_TRUE) { return; }

    presence_kv_set(set, CROSSTALK_NET_PRESENCE_KEY_OP, op_on == LDG_TRUTH_TRUE ? op : CROSSTALK_NET_PRESENCE_VAL_CLEAR);
    presence_kv_set(set, CROSSTALK_NET_PRESENCE_KEY_ACTIVITY, att_on == LDG_TRUTH_TRUE ? att : CROSSTALK_NET_PRESENCE_VAL_CLEAR);

    if (op_on == LDG_TRUTH_TRUE && att_on == LDG_TRUTH_TRUE) { display = in_lobby != LDG_TRUTH_FALSE ? CROSSTALK_NET_PRESENCE_DISPLAY_LOBBY_OP_ACT : CROSSTALK_NET_PRESENCE_DISPLAY_KB_OP_ACT; }
    else if (op_on == LDG_TRUTH_TRUE) { display = in_lobby != LDG_TRUTH_FALSE ? CROSSTALK_NET_PRESENCE_DISPLAY_LOBBY_OP : CROSSTALK_NET_PRESENCE_DISPLAY_KB_OP; }
    else { display = in_lobby != LDG_TRUTH_FALSE ? CROSSTALK_NET_PRESENCE_DISPLAY_LOBBY_ACT : CROSSTALK_NET_PRESENCE_DISPLAY_KB_ACT; }

    presence_kv_set(set, CROSSTALK_NET_PRESENCE_KEY_DISPLAY, display);
}

void crosstalk_net_presence_sandbox_solo_set(const libresteam_backend_set_t *set)
{
    presence_kv_set(set, CROSSTALK_NET_PRESENCE_KEY_DISPLAY, CROSSTALK_NET_PRESENCE_DISPLAY_KB_SOLO);
}

void crosstalk_net_presence_sandbox_set(const libresteam_backend_set_t *set, uint32_t players)
{
    char players_str[CROSSTALK_NET_PRESENCE_NUM_CAP] = LDG_ARR_ZERO_INIT;

    if (players <= CROSSTALK_NET_PRESENCE_PLAYERS_SOLO)
    {
        crosstalk_net_presence_sandbox_solo_set(set);
        return;
    }

    if (snprintf(players_str, sizeof(players_str), "%" PRIu32, players) < 0) { return; }

    presence_kv_set(set, CROSSTALK_NET_PRESENCE_KEY_PLAYERS, players_str);
    presence_kv_set(set, CROSSTALK_NET_PRESENCE_KEY_DISPLAY, CROSSTALK_NET_PRESENCE_DISPLAY_KB);
}

void crosstalk_net_presence_lobby_set(const libresteam_backend_set_t *set, uint32_t players, uint32_t players_max)
{
    char players_str[CROSSTALK_NET_PRESENCE_NUM_CAP] = LDG_ARR_ZERO_INIT;
    char max_str[CROSSTALK_NET_PRESENCE_NUM_CAP] = LDG_ARR_ZERO_INIT;

    if (snprintf(players_str, sizeof(players_str), "%" PRIu32, players) < 0) { return; }

    if (snprintf(max_str, sizeof(max_str), "%" PRIu32, players_max) < 0) { return; }

    presence_kv_set(set, CROSSTALK_NET_PRESENCE_KEY_PLAYERS, players_str);
    presence_kv_set(set, CROSSTALK_NET_PRESENCE_KEY_PLAYERS_MAX, max_str);
    presence_kv_set(set, CROSSTALK_NET_PRESENCE_KEY_DISPLAY, CROSSTALK_NET_PRESENCE_DISPLAY_LOBBY);
}

void crosstalk_net_presence_menu_set(const libresteam_backend_set_t *set, const char *menu)
{
    presence_kv_set(set, CROSSTALK_NET_PRESENCE_KEY_MENU, menu != 0x0 ? menu : CROSSTALK_NET_PRESENCE_MENU_MAIN);
    presence_kv_set(set, CROSSTALK_NET_PRESENCE_KEY_DISPLAY, CROSSTALK_NET_PRESENCE_DISPLAY_MENU);
}

void crosstalk_net_presence_ld_set(const libresteam_backend_set_t *set)
{
    presence_kv_set(set, CROSSTALK_NET_PRESENCE_KEY_DISPLAY, CROSSTALK_NET_PRESENCE_DISPLAY_LD);
}

void crosstalk_net_presence_browse_set(const libresteam_backend_set_t *set)
{
    presence_kv_set(set, CROSSTALK_NET_PRESENCE_KEY_DISPLAY, CROSSTALK_NET_PRESENCE_DISPLAY_BROWSE);
}

void crosstalk_net_presence_newproj_set(const libresteam_backend_set_t *set, const char *kind)
{
    presence_kv_set(set, CROSSTALK_NET_PRESENCE_KEY_ACTIVITY, kind != 0x0 ? kind : CROSSTALK_NET_PRESENCE_PROJ_RTL);
    presence_kv_set(set, CROSSTALK_NET_PRESENCE_KEY_DISPLAY, CROSSTALK_NET_PRESENCE_DISPLAY_NEWPROJ);
}

void crosstalk_net_presence_extras_set(const libresteam_backend_set_t *set)
{
    presence_kv_set(set, CROSSTALK_NET_PRESENCE_KEY_DISPLAY, CROSSTALK_NET_PRESENCE_DISPLAY_EXTRAS);
}

void crosstalk_net_presence_credits_set(const libresteam_backend_set_t *set)
{
    presence_kv_set(set, CROSSTALK_NET_PRESENCE_KEY_DISPLAY, CROSSTALK_NET_PRESENCE_DISPLAY_CREDITS);
}

void crosstalk_net_presence_group_set(const libresteam_backend_set_t *set, uint64_t group_id, uint32_t players)
{
    char group_str[CROSSTALK_NET_PRESENCE_ID_CAP] = LDG_ARR_ZERO_INIT;
    char size_str[CROSSTALK_NET_PRESENCE_NUM_CAP] = LDG_ARR_ZERO_INIT;

    if (group_id == CROSSTALK_NET_PRESENCE_GROUP_CLEAR || players == 0)
    {
        presence_kv_set(set, CROSSTALK_NET_PRESENCE_KEY_GROUP, CROSSTALK_NET_PRESENCE_VAL_CLEAR);
        presence_kv_set(set, CROSSTALK_NET_PRESENCE_KEY_GROUP_SIZE, CROSSTALK_NET_PRESENCE_VAL_CLEAR);
        return;
    }

    if (snprintf(group_str, sizeof(group_str), "%" PRIu64, group_id) < 0) { return; }

    if (snprintf(size_str, sizeof(size_str), "%" PRIu32, players) < 0) { return; }

    presence_kv_set(set, CROSSTALK_NET_PRESENCE_KEY_GROUP, group_str);
    presence_kv_set(set, CROSSTALK_NET_PRESENCE_KEY_GROUP_SIZE, size_str);
}

void crosstalk_net_presence_joinable_set(const libresteam_backend_set_t *set, uint64_t host_steam_id, uint32_t players)
{
    crosstalk_net_presence_group_set(set, host_steam_id, players);
}

void crosstalk_net_presence_connect_set(const libresteam_backend_set_t *set, uint64_t lobby_id)
{
    char connect[CROSSTALK_NET_PRESENCE_CONNECT_CAP] = LDG_ARR_ZERO_INIT;

    if (lobby_id == 0)
    {
        presence_kv_set(set, CROSSTALK_NET_PRESENCE_KEY_CONNECT, CROSSTALK_NET_PRESENCE_VAL_CLEAR);
        return;
    }

    if (snprintf(connect, sizeof(connect), CROSSTALK_NET_PRESENCE_CONNECT_FMT, lobby_id) < 0) { return; }

    presence_kv_set(set, CROSSTALK_NET_PRESENCE_KEY_CONNECT, connect);
}
