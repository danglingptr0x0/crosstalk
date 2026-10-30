#ifndef CROSSTALK_NET_PRESENCE_H
#define CROSSTALK_NET_PRESENCE_H

#include <stdint.h>
#include <inttypes.h>

#define CROSSTALK_NET_PRESENCE_KIND_NONE 0
#define CROSSTALK_NET_PRESENCE_KIND_MENU 1
#define CROSSTALK_NET_PRESENCE_KIND_SANDBOX 2
#define CROSSTALK_NET_PRESENCE_KIND_LOBBY 3
#define CROSSTALK_NET_PRESENCE_KEY_DISPLAY "steam_display"
#define CROSSTALK_NET_PRESENCE_KEY_PLAYERS "players"
#define CROSSTALK_NET_PRESENCE_KEY_PLAYERS_MAX "maxplayers"
#define CROSSTALK_NET_PRESENCE_KEY_GROUP "steam_player_group"
#define CROSSTALK_NET_PRESENCE_KEY_GROUP_SIZE "steam_player_group_size"
#define CROSSTALK_NET_PRESENCE_KEY_CONNECT "connect"
#define CROSSTALK_NET_PRESENCE_CONNECT_FMT "+connect_lobby %" PRIu64
#define CROSSTALK_NET_PRESENCE_CONNECT_CAP 48
#define CROSSTALK_NET_PRESENCE_GROUP_CLEAR 0
#define CROSSTALK_NET_PRESENCE_KEY_MENU "menu"
#define CROSSTALK_NET_PRESENCE_DISPLAY_LOBBY "#Lobby"
#define CROSSTALK_NET_PRESENCE_DISPLAY_MENU "#InMenu"
#define CROSSTALK_NET_PRESENCE_DISPLAY_KB_SOLO "#KeyboardSolo"
#define CROSSTALK_NET_PRESENCE_DISPLAY_KB_OP "#KeyboardOp"
#define CROSSTALK_NET_PRESENCE_DISPLAY_KB_ACT "#KeyboardAct"
#define CROSSTALK_NET_PRESENCE_DISPLAY_KB_OP_ACT "#KeyboardOpAct"
#define CROSSTALK_NET_PRESENCE_DISPLAY_LOBBY_OP "#LobbyOp"
#define CROSSTALK_NET_PRESENCE_DISPLAY_LOBBY_ACT "#LobbyAct"
#define CROSSTALK_NET_PRESENCE_DISPLAY_LOBBY_OP_ACT "#LobbyOpAct"
#define CROSSTALK_NET_PRESENCE_KEY_ACTIVITY "activity"
#define CROSSTALK_NET_PRESENCE_KEY_OP "op"
#define CROSSTALK_NET_PRESENCE_DISPLAY_KB "#Keyboard"
#define CROSSTALK_NET_PRESENCE_DISPLAY_KB_PAUSED "#KeyboardPaused"
#define CROSSTALK_NET_PRESENCE_DISPLAY_LOBBY_PAUSED "#LobbyPaused"
#define CROSSTALK_NET_PRESENCE_DISPLAY_SNIFFERS "#Sniffers"
#define CROSSTALK_NET_PRESENCE_DISPLAY_STORY "#Story"
#define CROSSTALK_NET_PRESENCE_DISPLAY_DANGCSA_GAME "#DangcsaGame"
#define CROSSTALK_NET_PRESENCE_DISPLAY_LD "#LoadMenu"
#define CROSSTALK_NET_PRESENCE_DISPLAY_BROWSE "#Browsing"
#define CROSSTALK_NET_PRESENCE_DISPLAY_NEWPROJ "#NewProject"
#define CROSSTALK_NET_PRESENCE_DISPLAY_EXTRAS "#Extras"
#define CROSSTALK_NET_PRESENCE_DISPLAY_CREDITS "#Credits"
#define CROSSTALK_NET_PRESENCE_MENU_MAIN "Main menu"
#define CROSSTALK_NET_PRESENCE_MENU_NEW "Starting a new project"
#define CROSSTALK_NET_PRESENCE_MENU_SETTINGS "Fiddling with the rotaries"
#define CROSSTALK_NET_PRESENCE_PROJ_RTL "an RTL project"
#define CROSSTALK_NET_PRESENCE_PROJ_ZONE "a Zone"
#define CROSSTALK_NET_PRESENCE_PROJ_REVERSE "a Reverse project"

void crosstalk_net_presence_set(const char *key, const char *val);
void crosstalk_net_presence_paused_set(uint8_t in_lobby);
void crosstalk_net_presence_lines_set(const char *op, const char *att, uint8_t in_lobby);
void crosstalk_net_presence_sandbox_solo_set(void);
void crosstalk_net_presence_sandbox_set(uint32_t players);
void crosstalk_net_presence_lobby_set(uint32_t players, uint32_t players_max);
void crosstalk_net_presence_menu_set(const char *menu);
void crosstalk_net_presence_ld_set(void);
void crosstalk_net_presence_browse_set(void);
void crosstalk_net_presence_newproj_set(const char *kind);
void crosstalk_net_presence_extras_set(void);
void crosstalk_net_presence_credits_set(void);
void crosstalk_net_presence_group_set(uint64_t group_id, uint32_t players);
void crosstalk_net_presence_joinable_set(uint64_t host_steam_id, int32_t players);
void crosstalk_net_presence_connect_set(uint64_t lobby_id);

#endif
