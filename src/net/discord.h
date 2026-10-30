#ifndef CROSSTALK_NET_DISCORD_H
#define CROSSTALK_NET_DISCORD_H

#include <stdint.h>

#define CROSSTALK_NET_DISCORD_SDK_REV "1.10.18247"
#define CROSSTALK_NET_DISCORD_APP_ID 1530141048104288377ULL

#define CROSSTALK_NET_DISCORD_STATE_OFF 0
#define CROSSTALK_NET_DISCORD_STATE_DISCONNECTED 1
#define CROSSTALK_NET_DISCORD_STATE_CONNECTING 2
#define CROSSTALK_NET_DISCORD_STATE_RDY 3
#define CROSSTALK_NET_DISCORD_STATE_FAIL 4

#define CROSSTALK_NET_DISCORD_AUTH_NONE 0
#define CROSSTALK_NET_DISCORD_AUTH_PENDING 1
#define CROSSTALK_NET_DISCORD_AUTH_PROVISIONAL 2
#define CROSSTALK_NET_DISCORD_AUTH_LINKED 3

#define CROSSTALK_NET_DISCORD_KIND_NONE 0
#define CROSSTALK_NET_DISCORD_KIND_MENU 1
#define CROSSTALK_NET_DISCORD_KIND_SANDBOX 2
#define CROSSTALK_NET_DISCORD_KIND_LOBBY 3
#define CROSSTALK_NET_DISCORD_KIND_MATCH 4

#define CROSSTALK_NET_DISCORD_CALL_OFF 0
#define CROSSTALK_NET_DISCORD_CALL_CONNECTING 1
#define CROSSTALK_NET_DISCORD_CALL_LIVE 2
#define CROSSTALK_NET_DISCORD_CALL_FAIL 3

#define CROSSTALK_NET_DISCORD_DETAILS_CAP 128
#define CROSSTALK_NET_DISCORD_STATE_CAP 128
#define CROSSTALK_NET_DISCORD_PARTY_ID_CAP 64
#define CROSSTALK_NET_DISCORD_SECRET_CAP 128
#define CROSSTALK_NET_DISCORD_ASSET_CAP 64
#define CROSSTALK_NET_DISCORD_ASSET_URL_CAP 128
#define CROSSTALK_NET_DISCORD_NAME_CAP 64
#define CROSSTALK_NET_DISCORD_ERR_CAP 160
#define CROSSTALK_NET_DISCORD_DEV_CAP 16
#define CROSSTALK_NET_DISCORD_DEV_ID_CAP 160
#define CROSSTALK_NET_DISCORD_DEV_NAME_CAP 160
#define CROSSTALK_NET_DISCORD_CALL_OPAQUE_CUNT 3
#define CROSSTALK_NET_DISCORD_TOK_CAP 256
#define CROSSTALK_NET_DISCORD_TKT_CAP 2048
#define CROSSTALK_NET_DISCORD_SCOPE_CAP 256
#define CROSSTALK_NET_DISCORD_REDIRECT_CAP 256
#define CROSSTALK_NET_DISCORD_PRESENCE_MIN_INTERVAL 2.0
#define CROSSTALK_NET_DISCORD_PRESENCE_BEAT 120.0
#define CROSSTALK_NET_DISCORD_CALL_RETRY_MIN 5.0
#define CROSSTALK_NET_DISCORD_CALL_RETRY_MAX 60.0
#define CROSSTALK_NET_DISCORD_CALL_RETRY_GROWTH 2.0
#define CROSSTALK_NET_DISCORD_VOL_MIN 0.0
#define CROSSTALK_NET_DISCORD_VOL_MAX 200.0
#define CROSSTALK_NET_DISCORD_VOL_DEFAULT_PCT 100.0
#define CROSSTALK_NET_DISCORD_MSG_RING 128
#define CROSSTALK_NET_DISCORD_HISTORY_LIMIT 32
#define CROSSTALK_NET_DISCORD_MSG_TEXT_CAP 240
#define CROSSTALK_NET_DISCORD_SECRET_APP "crosstalk"
#define CROSSTALK_NET_DISCORD_SECRET_KEY "discord_refresh_tok"
#define CROSSTALK_NET_DISCORD_AUTH_LOCAL_TIMEOUT 90.0
#define CROSSTALK_NET_DISCORD_AUTH_URL_CAP 1024
#define CROSSTALK_NET_DISCORD_CODE_CAP 256
#define CROSSTALK_NET_DISCORD_SCHEME_SEP "//"
#define CROSSTALK_NET_DISCORD_SCHEME "crosstalk:"
#define CROSSTALK_NET_DISCORD_SCHEME_NAME "crosstalk"
#define CROSSTALK_NET_DISCORD_REDIRECT_URI "file:" CROSSTALK_NET_DISCORD_SCHEME_SEP "/C:/Program%20Files%20(x86)/Steam/steamapps/common/crosstalk/cb.html"
#define CROSSTALK_NET_DISCORD_AUTH_ENDPOINT "https:" CROSSTALK_NET_DISCORD_SCHEME_SEP "discord.com/oauth2/authorize"
#define CROSSTALK_NET_DISCORD_USER_ID_NONE 0
#define CROSSTALK_NET_DISCORD_GUILD_ID 1526629724825063566ULL
#define CROSSTALK_NET_DISCORD_GUILD_SCOPE " guilds"
#define CROSSTALK_NET_DISCORD_GUILD_INTERVAL 30.0
#define CROSSTALK_NET_DISCORD_GUILD_RETRY 5.0
#define CROSSTALK_NET_DISCORD_GUILD_TIMEOUT 10.0
#define CROSSTALK_NET_DISCORD_MUSIC_SEP "; "
#define CROSSTALK_NET_DISCORD_MUSIC_PREFIX "Music: "
#define CROSSTALK_NET_DISCORD_TEXT_MENU_MAIN "Main menu"
#define CROSSTALK_NET_DISCORD_TEXT_MENU_NEW "Starting a new project"
#define CROSSTALK_NET_DISCORD_TEXT_MENU_SETTINGS "Fiddling with the rotaries"
#define CROSSTALK_NET_DISCORD_TEXT_PROJ_RTL "an RTL project"
#define CROSSTALK_NET_DISCORD_TEXT_PROJ_ZONE "a Zone"
#define CROSSTALK_NET_DISCORD_TEXT_PROJ_REVERSE "a Reverse project"
#define CROSSTALK_NET_DISCORD_CONTACT_CAP 128
#define CROSSTALK_NET_DISCORD_DM_CAP 64
#define CROSSTALK_NET_DISCORD_CONTACT_REFRESH_INTERVAL 4.0

#define CROSSTALK_NET_DISCORD_CONTACT_KIND_FRIEND 0
#define CROSSTALK_NET_DISCORD_CONTACT_KIND_PENDING_IN 1
#define CROSSTALK_NET_DISCORD_CONTACT_KIND_PENDING_OUT 2
#define CROSSTALK_NET_DISCORD_CONTACT_KIND_DM 3
#define CROSSTALK_NET_DISCORD_CONTACT_KIND_LOBBY 4
#define CROSSTALK_NET_DISCORD_CONTACT_KIND_OTHER 5

#define CROSSTALK_NET_DISCORD_ACTIVITY_NONE 0
#define CROSSTALK_NET_DISCORD_ACTIVITY_PLAYING 1
#define CROSSTALK_NET_DISCORD_ACTIVITY_STREAMING 2
#define CROSSTALK_NET_DISCORD_ACTIVITY_LISTENING 3
#define CROSSTALK_NET_DISCORD_ACTIVITY_WATCHING 4
#define CROSSTALK_NET_DISCORD_ACTIVITY_CUSTOM 5
#define CROSSTALK_NET_DISCORD_ACTIVITY_COMPETING 6
#define CROSSTALK_NET_DISCORD_ACTIVITY_HANG 7
#define CROSSTALK_NET_DISCORD_ACTIVITY_CAP 48

#define CROSSTALK_NET_DISCORD_STATUS_ONLINE 0
#define CROSSTALK_NET_DISCORD_STATUS_OFFLINE 1
#define CROSSTALK_NET_DISCORD_STATUS_BLK 2
#define CROSSTALK_NET_DISCORD_STATUS_IDLE 3
#define CROSSTALK_NET_DISCORD_STATUS_DND 4
#define CROSSTALK_NET_DISCORD_STATUS_INVISIBLE 5
#define CROSSTALK_NET_DISCORD_STATUS_STREAMING 6
#define CROSSTALK_NET_DISCORD_STATUS_UNKNOWN 7

typedef struct crosstalk_net_discord_msg
{
    uint64_t author_id;
    uint64_t peer_id;
    char name[CROSSTALK_NET_DISCORD_NAME_CAP];
    char peer_name[CROSSTALK_NET_DISCORD_NAME_CAP];
    char text[CROSSTALK_NET_DISCORD_MSG_TEXT_CAP];
    uint8_t dm;
    uint8_t hist;
    uint8_t pudding[6];
} crosstalk_net_discord_msg_t;

typedef struct crosstalk_net_discord_contact
{
    uint64_t user_id;
    char name[CROSSTALK_NET_DISCORD_NAME_CAP];
    char activity_name[CROSSTALK_NET_DISCORD_ACTIVITY_CAP];
    uint32_t status;
    uint8_t kind;
    uint8_t conn;
    uint8_t provisional;
    uint8_t activity;
    uint8_t ours;
    uint8_t pudding[3];
} crosstalk_net_discord_contact_t;

typedef struct crosstalk_net_discord_dev
{
    char id[CROSSTALK_NET_DISCORD_DEV_ID_CAP];
    char name[CROSSTALK_NET_DISCORD_DEV_NAME_CAP];
} crosstalk_net_discord_dev_t;

typedef struct crosstalk_net_discord_presence
{
    uint64_t start_epoch_s;
    int64_t end_epoch_s;
    char details[CROSSTALK_NET_DISCORD_DETAILS_CAP];
    char state[CROSSTALK_NET_DISCORD_STATE_CAP];
    char party_id[CROSSTALK_NET_DISCORD_PARTY_ID_CAP];
    char join_secret[CROSSTALK_NET_DISCORD_SECRET_CAP];
    char asset_large[CROSSTALK_NET_DISCORD_ASSET_CAP];
    char asset_large_text[CROSSTALK_NET_DISCORD_ASSET_CAP];
    char asset_large_url[CROSSTALK_NET_DISCORD_ASSET_URL_CAP];
    char asset_small[CROSSTALK_NET_DISCORD_ASSET_CAP];
    char asset_small_text[CROSSTALK_NET_DISCORD_ASSET_CAP];
    char asset_small_url[CROSSTALK_NET_DISCORD_ASSET_URL_CAP];
    char music[CROSSTALK_NET_DISCORD_STATE_CAP];
    char activity[CROSSTALK_NET_DISCORD_STATE_CAP];
    uint32_t party_cunt;
    uint32_t party_cunt_max;
    uint32_t kind;
    uint8_t joinable;
    uint8_t paused;
    uint8_t pudding[2];
} crosstalk_net_discord_presence_t;

typedef struct crosstalk_net_discord_ctx
{
    void *client;
    void *invite;
    void *verifier;
    void *auth_args;
    void *auth_challenge;
    void *call[CROSSTALK_NET_DISCORD_CALL_OPAQUE_CUNT];
    crosstalk_net_discord_presence_t presence;
    crosstalk_net_discord_presence_t presence_sent;
    crosstalk_net_discord_msg_t msg[CROSSTALK_NET_DISCORD_MSG_RING];
    uint32_t msg_hd;
    uint32_t msg_cunt;
    crosstalk_net_discord_contact_t contact[CROSSTALK_NET_DISCORD_CONTACT_CAP];
    uint64_t dm_user[CROSSTALK_NET_DISCORD_DM_CAP];
    crosstalk_net_discord_dev_t dev_in[CROSSTALK_NET_DISCORD_DEV_CAP];
    crosstalk_net_discord_dev_t dev_out[CROSSTALK_NET_DISCORD_DEV_CAP];
    uint64_t app_id;
    uint64_t user_id;
    uint64_t call_lobby_id;
    double presence_next;
    double call_next;
    double contact_next;
    double guild_next;
    double presence_beat;
    double auth_deadline;
    double vol_in;
    double vol_out;
    char user_name[CROSSTALK_NET_DISCORD_NAME_CAP];
    char err_text[CROSSTALK_NET_DISCORD_ERR_CAP];
    char join_secret_in[CROSSTALK_NET_DISCORD_SECRET_CAP];
    char invite_name[CROSSTALK_NET_DISCORD_NAME_CAP];
    uint64_t invite_sender_id;
    char refresh_tok[CROSSTALK_NET_DISCORD_TOK_CAP];
    char scopes[CROSSTALK_NET_DISCORD_SCOPE_CAP];
    char verifier_str[CROSSTALK_NET_DISCORD_TOK_CAP];
    char auth_url[CROSSTALK_NET_DISCORD_AUTH_URL_CAP];
    char dev_in_sel[CROSSTALK_NET_DISCORD_DEV_ID_CAP];
    char dev_out_sel[CROSSTALK_NET_DISCORD_DEV_ID_CAP];
    uint32_t contact_cunt;
    uint32_t dm_cunt;
    uint32_t dev_in_cunt;
    uint32_t dev_out_cunt;
    uint32_t state;
    uint32_t auth;
    uint32_t call_state;
    uint32_t call_fail_cunt;
    uint32_t last_err;
    uint8_t client_live;
    uint8_t en;
    uint8_t presence_on;
    uint8_t voice_on;
    uint8_t self_mute;
    uint8_t self_deaf;
    uint8_t join_pending;
    uint8_t verifier_live;
    uint8_t invite_live;
    uint8_t auth_args_live;
    uint8_t call_live;
    uint8_t call_req;
    uint8_t lobby_live;
    uint8_t auth_local_req;
    uint8_t auth_fail_req;
    uint8_t contact_primed;
    uint8_t guild_member;
    uint8_t guild_req;
} crosstalk_net_discord_ctx_t;

uint32_t crosstalk_net_discord_init(crosstalk_net_discord_ctx_t *ctx, uint64_t app_id, uint32_t steam_app_id);
void crosstalk_net_discord_shutdown(crosstalk_net_discord_ctx_t *ctx);
void crosstalk_net_discord_poll(crosstalk_net_discord_ctx_t *ctx, double dt);

uint32_t crosstalk_net_discord_en_set(crosstalk_net_discord_ctx_t *ctx, uint8_t on);
uint32_t crosstalk_net_discord_auth_link(crosstalk_net_discord_ctx_t *ctx);
uint32_t crosstalk_net_discord_auth_unlink(crosstalk_net_discord_ctx_t *ctx);

uint8_t crosstalk_net_discord_live_is(const crosstalk_net_discord_ctx_t *ctx);
uint8_t crosstalk_net_discord_linked_is(const crosstalk_net_discord_ctx_t *ctx);
uint8_t crosstalk_net_discord_guild_member_is(const crosstalk_net_discord_ctx_t *ctx);
uint32_t crosstalk_net_discord_guild_refresh(crosstalk_net_discord_ctx_t *ctx);
uint32_t crosstalk_net_discord_state_get(const crosstalk_net_discord_ctx_t *ctx);
uint32_t crosstalk_net_discord_auth_get(const crosstalk_net_discord_ctx_t *ctx);
const char* crosstalk_net_discord_user_name_get(const crosstalk_net_discord_ctx_t *ctx);
const char* crosstalk_net_discord_err_text_get(const crosstalk_net_discord_ctx_t *ctx);

uint32_t crosstalk_net_discord_presence_on_set(crosstalk_net_discord_ctx_t *ctx, uint8_t on);
uint32_t crosstalk_net_discord_presence_menu_set(crosstalk_net_discord_ctx_t *ctx, const char *menu);
uint32_t crosstalk_net_discord_presence_ld_set(crosstalk_net_discord_ctx_t *ctx);
uint32_t crosstalk_net_discord_presence_browse_set(crosstalk_net_discord_ctx_t *ctx);
uint32_t crosstalk_net_discord_presence_sandbox_set(crosstalk_net_discord_ctx_t *ctx, uint32_t players, uint32_t comp_cunt, uint32_t trace_cunt);
uint32_t crosstalk_net_discord_presence_music_set(crosstalk_net_discord_ctx_t *ctx, const char *text);
uint32_t crosstalk_net_discord_presence_activity_set(crosstalk_net_discord_ctx_t *ctx, const char *text);
uint32_t crosstalk_net_discord_presence_paused_set(crosstalk_net_discord_ctx_t *ctx, uint8_t paused);
uint32_t crosstalk_net_discord_presence_lobby_set(crosstalk_net_discord_ctx_t *ctx, uint32_t players, uint32_t players_max, uint64_t lobby_id);
uint32_t crosstalk_net_discord_presence_newproj_set(crosstalk_net_discord_ctx_t *ctx, const char *kind);
uint32_t crosstalk_net_discord_presence_extras_set(crosstalk_net_discord_ctx_t *ctx);
uint32_t crosstalk_net_discord_presence_credits_set(crosstalk_net_discord_ctx_t *ctx);
uint32_t crosstalk_net_discord_presence_clear(crosstalk_net_discord_ctx_t *ctx);

uint32_t crosstalk_net_discord_join_secret_get(crosstalk_net_discord_ctx_t *ctx, uint64_t *out_lobby_id);

uint32_t crosstalk_net_discord_chat_send(crosstalk_net_discord_ctx_t *ctx, const char *text);
uint32_t crosstalk_net_discord_chat_get(crosstalk_net_discord_ctx_t *ctx, const char **out_name, const char **out_text, const char **out_peer_name, uint64_t *out_author_id, uint64_t *out_peer_id, uint8_t *out_dm, uint8_t *out_hist);
uint32_t crosstalk_net_discord_dm_send(crosstalk_net_discord_ctx_t *ctx, uint64_t user_id, const char *text);
uint32_t crosstalk_net_discord_dm_history_req(crosstalk_net_discord_ctx_t *ctx, uint64_t user_id);
uint32_t crosstalk_net_discord_lobby_history_req(crosstalk_net_discord_ctx_t *ctx);

uint64_t crosstalk_net_discord_user_id_get(const crosstalk_net_discord_ctx_t *ctx);
uint32_t crosstalk_net_discord_contact_cunt_get(const crosstalk_net_discord_ctx_t *ctx);
const crosstalk_net_discord_contact_t* crosstalk_net_discord_contact_get(const crosstalk_net_discord_ctx_t *ctx, uint32_t idx);
const char* crosstalk_net_discord_status_name_get(uint32_t status);
const char* crosstalk_net_discord_activity_verb_get(uint8_t activity);
const char* crosstalk_net_discord_kind_name_get(uint8_t kind);
uint32_t crosstalk_net_discord_auth_restore(crosstalk_net_discord_ctx_t *ctx);
uint8_t crosstalk_net_discord_auth_fail_take(crosstalk_net_discord_ctx_t *ctx);
uint32_t crosstalk_net_discord_auth_link_local(crosstalk_net_discord_ctx_t *ctx);

uint32_t crosstalk_net_discord_invite_send(crosstalk_net_discord_ctx_t *ctx, uint64_t user_id, const char *text);
uint32_t crosstalk_net_discord_invite_pending_get(const crosstalk_net_discord_ctx_t *ctx, const char **out_name, uint64_t *out_sender_id);
uint32_t crosstalk_net_discord_invite_accept(crosstalk_net_discord_ctx_t *ctx);
uint32_t crosstalk_net_discord_invite_drop(crosstalk_net_discord_ctx_t *ctx);

uint32_t crosstalk_net_discord_lobby_join(crosstalk_net_discord_ctx_t *ctx, uint64_t lobby_id);
uint32_t crosstalk_net_discord_lobby_leave(crosstalk_net_discord_ctx_t *ctx);
uint32_t crosstalk_net_discord_voice_on_set(crosstalk_net_discord_ctx_t *ctx, uint8_t on);
uint32_t crosstalk_net_discord_voice_state_get(const crosstalk_net_discord_ctx_t *ctx);
uint32_t crosstalk_net_discord_voice_self_mute_set(crosstalk_net_discord_ctx_t *ctx, uint8_t mute);
uint32_t crosstalk_net_discord_voice_self_deaf_set(crosstalk_net_discord_ctx_t *ctx, uint8_t deaf);
uint8_t crosstalk_net_discord_voice_self_mute_is(const crosstalk_net_discord_ctx_t *ctx);
uint8_t crosstalk_net_discord_voice_self_deaf_is(const crosstalk_net_discord_ctx_t *ctx);
uint32_t crosstalk_net_discord_voice_dev_refresh(crosstalk_net_discord_ctx_t *ctx);
uint32_t crosstalk_net_discord_voice_dev_in_cunt_get(const crosstalk_net_discord_ctx_t *ctx);
uint32_t crosstalk_net_discord_voice_dev_out_cunt_get(const crosstalk_net_discord_ctx_t *ctx);
const crosstalk_net_discord_dev_t* crosstalk_net_discord_voice_dev_in_get(const crosstalk_net_discord_ctx_t *ctx, uint32_t idx);
const crosstalk_net_discord_dev_t* crosstalk_net_discord_voice_dev_out_get(const crosstalk_net_discord_ctx_t *ctx, uint32_t idx);
uint32_t crosstalk_net_discord_voice_dev_in_set(crosstalk_net_discord_ctx_t *ctx, const char *dev_id);
uint32_t crosstalk_net_discord_voice_dev_out_set(crosstalk_net_discord_ctx_t *ctx, const char *dev_id);
uint32_t crosstalk_net_discord_voice_vol_in_set(crosstalk_net_discord_ctx_t *ctx, double vol);
uint32_t crosstalk_net_discord_voice_vol_out_set(crosstalk_net_discord_ctx_t *ctx, double vol);
double crosstalk_net_discord_voice_vol_in_get(const crosstalk_net_discord_ctx_t *ctx);
double crosstalk_net_discord_voice_vol_out_get(const crosstalk_net_discord_ctx_t *ctx);

#endif
