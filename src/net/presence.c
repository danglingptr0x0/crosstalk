#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>

#include <dangling/core/macros.h>

#include <dangling/net/steamworks.h>

#include "net/presence.h"

void crosstalk_net_presence_set(const char *key, const char *val)
{
    if (LDG_UNLIKELY(!key || !val)) { return; }

    ldg_net_steamworks_presence_set(key, val);
}

void crosstalk_net_presence_paused_set(uint8_t in_lobby)
{
    ldg_net_steamworks_presence_set(CROSSTALK_NET_PRESENCE_KEY_DISPLAY, in_lobby != LDG_TRUTH_FALSE ? CROSSTALK_NET_PRESENCE_DISPLAY_LOBBY_PAUSED : CROSSTALK_NET_PRESENCE_DISPLAY_KB_PAUSED);
}

void crosstalk_net_presence_lines_set(const char *op, const char *att, uint8_t in_lobby)
{
    const char *display = 0x0;
    uint8_t op_on = (op != 0x0 && op[0] != LDG_STR_TERM) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    uint8_t att_on = (att != 0x0 && att[0] != LDG_STR_TERM) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (op_on != LDG_TRUTH_TRUE && att_on != LDG_TRUTH_TRUE) { return; }

    ldg_net_steamworks_presence_set(CROSSTALK_NET_PRESENCE_KEY_OP, op_on == LDG_TRUTH_TRUE ? op : "");
    ldg_net_steamworks_presence_set(CROSSTALK_NET_PRESENCE_KEY_ACTIVITY, att_on == LDG_TRUTH_TRUE ? att : "");

    if (op_on == LDG_TRUTH_TRUE && att_on == LDG_TRUTH_TRUE) { display = in_lobby != LDG_TRUTH_FALSE ? CROSSTALK_NET_PRESENCE_DISPLAY_LOBBY_OP_ACT : CROSSTALK_NET_PRESENCE_DISPLAY_KB_OP_ACT; }
    else if (op_on == LDG_TRUTH_TRUE) { display = in_lobby != LDG_TRUTH_FALSE ? CROSSTALK_NET_PRESENCE_DISPLAY_LOBBY_OP : CROSSTALK_NET_PRESENCE_DISPLAY_KB_OP; }
    else { display = in_lobby != LDG_TRUTH_FALSE ? CROSSTALK_NET_PRESENCE_DISPLAY_LOBBY_ACT : CROSSTALK_NET_PRESENCE_DISPLAY_KB_ACT; }

    ldg_net_steamworks_presence_set(CROSSTALK_NET_PRESENCE_KEY_DISPLAY, display);
}

void crosstalk_net_presence_sandbox_solo_set(void)
{
    ldg_net_steamworks_presence_set(CROSSTALK_NET_PRESENCE_KEY_DISPLAY, CROSSTALK_NET_PRESENCE_DISPLAY_KB_SOLO);
}

void crosstalk_net_presence_sandbox_set(uint32_t players)
{
    char players_str[16] = LDG_ARR_ZERO_INIT;

    if (players <= 1)
    {
        crosstalk_net_presence_sandbox_solo_set();
        return;
    }

    snprintf(players_str, sizeof(players_str), "%" PRIu32, players);
    ldg_net_steamworks_presence_set(CROSSTALK_NET_PRESENCE_KEY_PLAYERS, players_str);
    ldg_net_steamworks_presence_set(CROSSTALK_NET_PRESENCE_KEY_DISPLAY, CROSSTALK_NET_PRESENCE_DISPLAY_KB);
}

void crosstalk_net_presence_lobby_set(uint32_t players, uint32_t players_max)
{
    char players_str[16] = LDG_ARR_ZERO_INIT;
    char max_str[16] = LDG_ARR_ZERO_INIT;

    snprintf(players_str, sizeof(players_str), "%" PRIu32, players);
    snprintf(max_str, sizeof(max_str), "%" PRIu32, players_max);
    ldg_net_steamworks_presence_set(CROSSTALK_NET_PRESENCE_KEY_PLAYERS, players_str);
    ldg_net_steamworks_presence_set(CROSSTALK_NET_PRESENCE_KEY_PLAYERS_MAX, max_str);
    ldg_net_steamworks_presence_set(CROSSTALK_NET_PRESENCE_KEY_DISPLAY, CROSSTALK_NET_PRESENCE_DISPLAY_LOBBY);
}

void crosstalk_net_presence_menu_set(const char *menu)
{
    ldg_net_steamworks_presence_set(CROSSTALK_NET_PRESENCE_KEY_MENU, menu != 0x0 ? menu : CROSSTALK_NET_PRESENCE_MENU_MAIN);
    ldg_net_steamworks_presence_set(CROSSTALK_NET_PRESENCE_KEY_DISPLAY, CROSSTALK_NET_PRESENCE_DISPLAY_MENU);
}

void crosstalk_net_presence_ld_set(void)
{
    ldg_net_steamworks_presence_set(CROSSTALK_NET_PRESENCE_KEY_DISPLAY, CROSSTALK_NET_PRESENCE_DISPLAY_LD);
}

void crosstalk_net_presence_browse_set(void)
{
    ldg_net_steamworks_presence_set(CROSSTALK_NET_PRESENCE_KEY_DISPLAY, CROSSTALK_NET_PRESENCE_DISPLAY_BROWSE);
}

void crosstalk_net_presence_newproj_set(const char *kind)
{
    ldg_net_steamworks_presence_set(CROSSTALK_NET_PRESENCE_KEY_ACTIVITY, kind != 0x0 ? kind : CROSSTALK_NET_PRESENCE_PROJ_RTL);
    ldg_net_steamworks_presence_set(CROSSTALK_NET_PRESENCE_KEY_DISPLAY, CROSSTALK_NET_PRESENCE_DISPLAY_NEWPROJ);
}

void crosstalk_net_presence_extras_set(void)
{
    ldg_net_steamworks_presence_set(CROSSTALK_NET_PRESENCE_KEY_DISPLAY, CROSSTALK_NET_PRESENCE_DISPLAY_EXTRAS);
}

void crosstalk_net_presence_credits_set(void)
{
    ldg_net_steamworks_presence_set(CROSSTALK_NET_PRESENCE_KEY_DISPLAY, CROSSTALK_NET_PRESENCE_DISPLAY_CREDITS);
}

void crosstalk_net_presence_group_set(uint64_t group_id, uint32_t players)
{
    char group_str[32] = LDG_ARR_ZERO_INIT;
    char size_str[16] = LDG_ARR_ZERO_INIT;

    if (group_id == 0 || players == 0)
    {
        ldg_net_steamworks_presence_set(CROSSTALK_NET_PRESENCE_KEY_GROUP, "");
        ldg_net_steamworks_presence_set(CROSSTALK_NET_PRESENCE_KEY_GROUP_SIZE, "");
        return;
    }

    snprintf(group_str, sizeof(group_str), "%" PRIu64, group_id);
    snprintf(size_str, sizeof(size_str), "%" PRIu32, players);
    ldg_net_steamworks_presence_set(CROSSTALK_NET_PRESENCE_KEY_GROUP, group_str);
    ldg_net_steamworks_presence_set(CROSSTALK_NET_PRESENCE_KEY_GROUP_SIZE, size_str);
}

void crosstalk_net_presence_joinable_set(uint64_t host_steam_id, int32_t players)
{
    ldg_net_steamworks_joinable_set(host_steam_id, (uint32_t)players);
}

void crosstalk_net_presence_connect_set(uint64_t lobby_id)
{
    char connect[CROSSTALK_NET_PRESENCE_CONNECT_CAP] = LDG_ARR_ZERO_INIT;

    if (lobby_id == 0)
    {
        ldg_net_steamworks_presence_set(CROSSTALK_NET_PRESENCE_KEY_CONNECT, "");
        return;
    }

    snprintf(connect, sizeof(connect), CROSSTALK_NET_PRESENCE_CONNECT_FMT, lobby_id);
    ldg_net_steamworks_presence_set(CROSSTALK_NET_PRESENCE_KEY_CONNECT, connect);
}
