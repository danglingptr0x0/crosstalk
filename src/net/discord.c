#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <dangling/logging/logging.h>

#include <dangling/core/macros.h>
#include <dangling/core/err.h>
#include <dangling/str/str.h>
#include <dangling/mem/secure.h>
#include <dangling/sys/info.h>
#include <dangling/time/time.h>

#include <cdiscord.h>

#include "core/log.h"
#include "net/discord.h"

#define CROSSTALK_NET_DISCORD_ASSET_LARGE "crosstalk_icon_alpha"
#define CROSSTALK_NET_DISCORD_ASSET_SMALL ""
#define CROSSTALK_NET_DISCORD_ASSET_LARGE_TEXT "Crosstalk on Steam"
#define CROSSTALK_NET_DISCORD_WSHOP_URL "https://steamcommunity.com/app/4898790/workshop/"
#define CROSSTALK_NET_DISCORD_BADGE_SOLO "badge_solo"
#define CROSSTALK_NET_DISCORD_BADGE_COOP "badge_coop"
#define CROSSTALK_NET_DISCORD_BADGE_LOBBY "badge_lobby"
#define CROSSTALK_NET_DISCORD_BADGE_ATTACK "badge_attack"
#define CROSSTALK_NET_DISCORD_BADGE_DEFEND "badge_defend"
#define CROSSTALK_NET_DISCORD_BADGE_BOARD "badge_board"
#define CROSSTALK_NET_DISCORD_BADGE_PAUSED "badge_paused"
#define CROSSTALK_NET_DISCORD_BADGE_TEXT_SOLO "Building alone"
#define CROSSTALK_NET_DISCORD_BADGE_TEXT_COOP "Building together"
#define CROSSTALK_NET_DISCORD_BADGE_TEXT_LOBBY "In a lobby"
#define CROSSTALK_NET_DISCORD_BADGE_TEXT_ATTACK "Attacking"
#define CROSSTALK_NET_DISCORD_BADGE_TEXT_DEFEND "Defending"
#define CROSSTALK_NET_DISCORD_BADGE_TEXT_BOARD "Choosing a board"
#define CROSSTALK_NET_DISCORD_BADGE_TEXT_PAUSED "Paused"
#define CROSSTALK_NET_DISCORD_PARTY_PREFIX "ct-"
#define CROSSTALK_NET_DISCORD_SECRET_PREFIX "ctlobby-"
#define CROSSTALK_NET_DISCORD_VOICE_SECRET_PREFIX "ctvoice-"
#define CROSSTALK_NET_DISCORD_NO_INPUT_DBFS -60.0
#define CROSSTALK_NET_DISCORD_VOL_DEFAULT 100.0
#define CROSSTALK_NET_DISCORD_CK_OK 1
#define CROSSTALK_NET_DISCORD_CK_BAD -1
#define CROSSTALK_NET_DISCORD_NUL 0x0
#define CROSSTALK_NET_DISCORD_FILL 0
#define CROSSTALK_NET_DISCORD_CUNT_NONE 0
#define CROSSTALK_NET_DISCORD_TIMER_DUE 0.0
#define CROSSTALK_NET_DISCORD_VOL_MUTE 0.0
#define CROSSTALK_NET_DISCORD_MATCH 0x0
#define CROSSTALK_NET_DISCORD_SCAN_CUNT 1
#define CROSSTALK_NET_DISCORD_RING_STEP 1
#define CROSSTALK_NET_DISCORD_PCT_LEN 3
#define CROSSTALK_NET_DISCORD_HEX_OFF 1
#define CROSSTALK_NET_DISCORD_ASCII_DIGIT_MIN 0x30
#define CROSSTALK_NET_DISCORD_ASCII_DIGIT_MAX 0x39
#define CROSSTALK_NET_DISCORD_TEXT_MENU "In the menus"
#define CROSSTALK_NET_DISCORD_TEXT_SANDBOX_SOLO "At the keyboard"
#define CROSSTALK_NET_DISCORD_TEXT_SANDBOX_PAUSED "Paused at the keyboard"
#define CROSSTALK_NET_DISCORD_TEXT_LOBBY_PAUSED "Paused in a lobby"
#define CROSSTALK_NET_DISCORD_TEXT_SANDBOX "At the keyboard with %" PRIu32
#define CROSSTALK_NET_DISCORD_TEXT_LOBBY "Busylooping in a lobby (%" PRIu32 "/%" PRIu32 ")"
#define CROSSTALK_NET_DISCORD_PARTY_NONE 0
#define CROSSTALK_NET_DISCORD_STAMP_NONE 0
#define CROSSTALK_NET_DISCORD_STAMP_DRIFT_MAX 2
#define CROSSTALK_NET_DISCORD_TEXT_LD "Choosing a project"
#define CROSSTALK_NET_DISCORD_TEXT_BROWSE "Seeking players"
#define CROSSTALK_NET_DISCORD_TEXT_NEWPROJ_FMT "Starting %.63s"
#define CROSSTALK_NET_DISCORD_TEXT_EXTRAS "In the extras"
#define CROSSTALK_NET_DISCORD_TEXT_CREDITS "Reading the credits"
#define CROSSTALK_NET_DISCORD_STORE_URL "https://store.steampowered.com/app/4898790/Crosstalk/"
#define CROSSTALK_NET_DISCORD_BTN_LABEL "View on Steam"
#define CROSSTALK_NET_DISCORD_SDK_BOOL(v) ((uint8_t)((v) != LDG_TRUTH_FALSE))
#define CROSSTALK_NET_DISCORD_SDK_OK(res) (CROSSTALK_NET_DISCORD_SDK_BOOL(Discord_ClientResult_Successful(res)) == LDG_TRUTH_TRUE)

typedef char crosstalk_net_discord_client_size_ck[(sizeof(Discord_Client) == sizeof(void *)) ? CROSSTALK_NET_DISCORD_CK_OK : CROSSTALK_NET_DISCORD_CK_BAD];
typedef char crosstalk_net_discord_call_size_ck[(sizeof(Discord_Call) == (sizeof(void *) * CROSSTALK_NET_DISCORD_CALL_OPAQUE_CUNT)) ? CROSSTALK_NET_DISCORD_CK_OK : CROSSTALK_NET_DISCORD_CK_BAD];
typedef char crosstalk_net_discord_verifier_size_ck[(sizeof(Discord_AuthorizationCodeVerifier) == sizeof(void *)) ? CROSSTALK_NET_DISCORD_CK_OK : CROSSTALK_NET_DISCORD_CK_BAD];

static Discord_Client* crosstalk_net_discord_client_get(crosstalk_net_discord_ctx_t *ctx)
{
    return (Discord_Client *)((void *)&ctx->client);
}

static Discord_Call* crosstalk_net_discord_call_get(crosstalk_net_discord_ctx_t *ctx)
{
    return (Discord_Call *)((void *)&ctx->call[0]);
}

static Discord_ActivityInvite* crosstalk_net_discord_invite_get(crosstalk_net_discord_ctx_t *ctx)
{
    return (Discord_ActivityInvite *)((void *)&ctx->invite);
}

static Discord_AuthorizationCodeVerifier* crosstalk_net_discord_verifier_get(crosstalk_net_discord_ctx_t *ctx)
{
    return (Discord_AuthorizationCodeVerifier *)((void *)&ctx->verifier);
}

static Discord_AuthorizationArgs* crosstalk_net_discord_auth_args_get(crosstalk_net_discord_ctx_t *ctx)
{
    return (Discord_AuthorizationArgs *)((void *)&ctx->auth_args);
}

static Discord_AuthorizationCodeChallenge* crosstalk_net_discord_auth_challenge_get(crosstalk_net_discord_ctx_t *ctx)
{
    return (Discord_AuthorizationCodeChallenge *)((void *)&ctx->auth_challenge);
}

static void crosstalk_net_discord_auth_args_drop(crosstalk_net_discord_ctx_t *ctx)
{
    if (ctx->auth_args_live == LDG_TRUTH_FALSE) { return; }

    Discord_AuthorizationArgs_Drop(crosstalk_net_discord_auth_args_get(ctx));
    Discord_AuthorizationCodeChallenge_Drop(crosstalk_net_discord_auth_challenge_get(ctx));

    ctx->auth_challenge = CROSSTALK_NET_DISCORD_NUL;
    ctx->auth_args = CROSSTALK_NET_DISCORD_NUL;
    ctx->auth_args_live = LDG_TRUTH_FALSE;
}

static void crosstalk_net_discord_str_make(const char *text, Discord_String *out)
{
    if (LDG_UNLIKELY(!out)) { return; }

    out->ptr = CROSSTALK_NET_DISCORD_NUL;
    out->size = 0;

    if (LDG_UNLIKELY(!text)) { return; }

    out->ptr = (uint8_t *)((void *)text);
    out->size = (uint64_t)strlen(text);
}

static void crosstalk_net_discord_str_copy(const Discord_String *src, char *dst, uint32_t cap)
{
    uint64_t len = 0;

    if (LDG_UNLIKELY(!dst || cap == 0)) { return; }

    dst[0] = '\0';

    if (LDG_UNLIKELY(!src || !src->ptr || (uint64_t)src->size == 0)) { return; }

    len = (uint64_t)src->size;
    if (len >= (uint64_t)cap) { len = (uint64_t)cap - CROSSTALK_NET_DISCORD_CK_OK; }

    memcpy(dst, src->ptr, len);
    dst[len] = '\0';
}

static uint8_t crosstalk_net_discord_url_safe_is(char ch)
{
    if (ch >= 'a' && ch <= 'z') { return LDG_TRUTH_TRUE; }

    if (ch >= 'A' && ch <= 'Z') { return LDG_TRUTH_TRUE; }

    if (ch >= CROSSTALK_NET_DISCORD_ASCII_DIGIT_MIN && ch <= CROSSTALK_NET_DISCORD_ASCII_DIGIT_MAX) { return LDG_TRUTH_TRUE; }

    if (ch == '-' || ch == '_' || ch == '.' || ch == '~') { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static void crosstalk_net_discord_url_encode(const char *src, char *dst, uint32_t cap)
{
    uint32_t out_idx = 0;

    dst[0] = LDG_STR_TERM;

    for (uint32_t idx = 0; src[idx] != LDG_STR_TERM; idx++)
    {
        if (crosstalk_net_discord_url_safe_is(src[idx]) == LDG_TRUTH_TRUE)
        {
            if (out_idx + LDG_STR_TERM_SIZE >= cap) { break; }

            dst[out_idx] = src[idx];
            out_idx++;
            continue;
        }

        if (out_idx + CROSSTALK_NET_DISCORD_PCT_LEN + LDG_STR_TERM_SIZE > cap) { break; }

        dst[out_idx] = '%';
        ldg_byte_to_hex((ldg_byte_t)src[idx], &dst[out_idx + CROSSTALK_NET_DISCORD_HEX_OFF]);
        out_idx = out_idx + CROSSTALK_NET_DISCORD_PCT_LEN;
    }

    dst[out_idx] = LDG_STR_TERM;
}

static void crosstalk_net_discord_scopes_ld(crosstalk_net_discord_ctx_t *ctx, Discord_String *out)
{
    Discord_String sdk_scopes = LDG_STRUCT_ZERO_INIT;
    char merged[CROSSTALK_NET_DISCORD_SCOPE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    Discord_Client_GetDefaultCommunicationScopes(&sdk_scopes);
    crosstalk_net_discord_str_copy(&sdk_scopes, ctx->scopes, (uint32_t)sizeof(ctx->scopes));
    Discord_Free(sdk_scopes.ptr);

    snprintf(merged, sizeof(merged), "%.247s%.7s", ctx->scopes, CROSSTALK_NET_DISCORD_GUILD_SCOPE);

    ret = ldg_str_rbrcpy(ctx->scopes, merged, (uint64_t)sizeof(ctx->scopes));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "discord: scope merge failed; err: %u32", ret); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: authorize scopes: %.255s", ctx->scopes);
    crosstalk_net_discord_str_make(ctx->scopes, out);
}

static void crosstalk_net_discord_user_data_free_cb(void *ptr)
{
    if (!ptr) { return; }
}

static void crosstalk_net_discord_err_capture(crosstalk_net_discord_ctx_t *ctx, Discord_ClientResult *res, const char *what)
{
    Discord_String text = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!ctx || !what)) { return; }

    if (!res)
    {
        snprintf(ctx->err_text, sizeof(ctx->err_text), "%.80s", what);
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "discord: %.159s", ctx->err_text);
        return;
    }

    Discord_ClientResult_ToString(res, &text);
    snprintf(ctx->err_text, sizeof(ctx->err_text), "%.60s: %.*s", what, (int32_t)text.size, (const char *)text.ptr);
    Discord_Free(text.ptr);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "discord: %.159s; http: %i32; code: %i32", ctx->err_text, (int32_t)Discord_ClientResult_Status(res), (int32_t)Discord_ClientResult_ErrorCode(res));
}

static void crosstalk_net_discord_user_refresh(crosstalk_net_discord_ctx_t *ctx)
{
    Discord_UserHandle user = LDG_STRUCT_ZERO_INIT;
    Discord_String name = LDG_STRUCT_ZERO_INIT;

    if (CROSSTALK_NET_DISCORD_SDK_BOOL(Discord_Client_GetCurrentUserV2(crosstalk_net_discord_client_get(ctx), &user)) == LDG_TRUTH_FALSE) { return; }

    ctx->user_id = (uint64_t)Discord_UserHandle_Id(&user);
    Discord_UserHandle_DisplayName(&user, &name);

    if ((uint64_t)name.size == 0)
    {
        Discord_Free(name.ptr);
        Discord_UserHandle_Username(&user, &name);
    }

    crosstalk_net_discord_str_copy(&name, ctx->user_name, (uint32_t)sizeof(ctx->user_name));
    Discord_Free(name.ptr);
    Discord_UserHandle_Drop(&user);
}

static void crosstalk_net_discord_log_cb(Discord_String msg, int32_t severity, void *user_data)
{
    if (!user_data) { return; }

    if (severity >= (int32_t)Discord_LoggingSeverity_Error) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "discord: %.*s", (uint32_t)msg.size, (const char *)msg.ptr); return; }

    if (severity == (int32_t)Discord_LoggingSeverity_Warning) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "discord: %.*s", (uint32_t)msg.size, (const char *)msg.ptr); return; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "discord: %.*s", (uint32_t)msg.size, (const char *)msg.ptr);
}

static void crosstalk_net_discord_status_cb(int32_t status, int32_t err, int32_t err_detail, void *user_data)
{
    crosstalk_net_discord_ctx_t *ctx = (crosstalk_net_discord_ctx_t *)user_data;
    Discord_String text = LDG_STRUCT_ZERO_INIT;

    if (!ctx) { return; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: client status; status: %i32; err: %i32; detail: %i32", status, err, err_detail);

    if (status == (int32_t)Discord_Client_Status_Ready)
    {
        ctx->state = CROSSTALK_NET_DISCORD_STATE_RDY;
        ctx->err_text[0] = LDG_STR_TERM;
        memset(&ctx->presence_sent, CROSSTALK_NET_DISCORD_FILL, sizeof(ctx->presence_sent));
        ctx->presence_next = CROSSTALK_NET_DISCORD_TIMER_DUE;
        crosstalk_net_discord_user_refresh(ctx);
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: ready; user: %.63s; id: %u64", ctx->user_name, ctx->user_id);
        return;
    }

    if (status == (int32_t)Discord_Client_Status_Connecting || status == (int32_t)Discord_Client_Status_Connected || status == (int32_t)Discord_Client_Status_Reconnecting || status == (int32_t)Discord_Client_Status_HttpWait)
    {
        ctx->state = CROSSTALK_NET_DISCORD_STATE_CONNECTING;
        return;
    }

    ctx->state = CROSSTALK_NET_DISCORD_STATE_DISCONNECTED;

    if (ctx->call_live == LDG_TRUTH_TRUE)
    {
        Discord_Call_Drop(crosstalk_net_discord_call_get(ctx));
        memset(&ctx->call[0], CROSSTALK_NET_DISCORD_FILL, sizeof(ctx->call));
    }

    ctx->call_live = LDG_TRUTH_FALSE;
    ctx->call_state = CROSSTALK_NET_DISCORD_CALL_OFF;
    ctx->lobby_live = LDG_TRUTH_FALSE;
    ctx->call_lobby_id = 0;
    ctx->call_req = LDG_TRUTH_FALSE;
    ctx->call_next = CROSSTALK_NET_DISCORD_TIMER_DUE;
    ctx->call_fail_cunt = 0;
    ctx->contact_cunt = 0;
    ctx->dm_cunt = 0;

    if (err != (int32_t)Discord_Client_Error_None)
    {
        ctx->state = CROSSTALK_NET_DISCORD_STATE_FAIL;
        Discord_Client_ErrorToString((Discord_Client_Error)err, &text);
        snprintf(ctx->err_text, sizeof(ctx->err_text), "client failed; err: %.*s; detail: %" PRId32, (int32_t)text.size, (const char *)text.ptr, err_detail);
        Discord_Free(text.ptr);
    }
}

static void crosstalk_net_discord_activity_join_cb(Discord_String join_secret, void *user_data)
{
    crosstalk_net_discord_ctx_t *ctx = (crosstalk_net_discord_ctx_t *)user_data;

    if (!ctx) { return; }

    crosstalk_net_discord_str_copy(&join_secret, ctx->join_secret_in, (uint32_t)sizeof(ctx->join_secret_in));
    ctx->join_pending = LDG_TRUTH_TRUE;
}

static void crosstalk_net_discord_tok_update_cb(Discord_ClientResult *res, void *user_data)
{
    crosstalk_net_discord_ctx_t *ctx = (crosstalk_net_discord_ctx_t *)user_data;

    if (!ctx) { return; }

    if (!CROSSTALK_NET_DISCORD_SDK_OK(res))
    {
        ctx->auth = CROSSTALK_NET_DISCORD_AUTH_NONE;
        ctx->state = CROSSTALK_NET_DISCORD_STATE_FAIL;
        crosstalk_net_discord_err_capture(ctx, res, "token update failed");
        return;
    }

    Discord_Client_Connect(crosstalk_net_discord_client_get(ctx));
}

static void crosstalk_net_discord_tok_cb(Discord_ClientResult *res, Discord_String access_tok, Discord_String refresh_tok, int32_t tok_type, int32_t expires_in, Discord_String scopes, void *user_data)
{
    crosstalk_net_discord_ctx_t *ctx = (crosstalk_net_discord_ctx_t *)user_data;
    uint32_t ret = 0;

    if (!ctx) { return; }

    if (!CROSSTALK_NET_DISCORD_SDK_OK(res))
    {
        ctx->auth = CROSSTALK_NET_DISCORD_AUTH_NONE;
        ctx->state = CROSSTALK_NET_DISCORD_STATE_FAIL;

        if (Discord_ClientResult_Status(res) == Discord_HttpStatusCode_BadRequest)
        {
            ret = ldg_sys_secret_set(CROSSTALK_NET_DISCORD_SECRET_APP, CROSSTALK_NET_DISCORD_SECRET_KEY, "");

            ldg_mem_secure_zero(ctx->refresh_tok, sizeof(ctx->refresh_tok));
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "discord: refresh token rejected; stored token dropped; err: %u32", ret);
        }

        crosstalk_net_discord_err_capture(ctx, res, "token exchange failed");
        return;
    }

    if (expires_in <= 0 || (uint64_t)scopes.size == 0) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "discord: token carries no scopes or lifetime; expires_in: %i32", expires_in); }

    crosstalk_net_discord_str_copy(&refresh_tok, ctx->refresh_tok, (uint32_t)sizeof(ctx->refresh_tok));

    if (ctx->refresh_tok[0] != LDG_STR_TERM)
    {
        ret = ldg_sys_secret_set(CROSSTALK_NET_DISCORD_SECRET_APP, CROSSTALK_NET_DISCORD_SECRET_KEY, ctx->refresh_tok);

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "discord: refresh token stored; err: %u32", ret);
    }

    if (ctx->refresh_tok[0] != LDG_STR_TERM) { ctx->auth = CROSSTALK_NET_DISCORD_AUTH_LINKED; }

    Discord_Client_UpdateToken(crosstalk_net_discord_client_get(ctx), (Discord_AuthorizationTokenType)tok_type, access_tok, crosstalk_net_discord_tok_update_cb, crosstalk_net_discord_user_data_free_cb, ctx);
}

static void crosstalk_net_discord_tok_expired_cb(void *user_data)
{
    crosstalk_net_discord_ctx_t *ctx = (crosstalk_net_discord_ctx_t *)user_data;
    Discord_String refresh = LDG_STRUCT_ZERO_INIT;

    if (!ctx) { return; }

    if (ctx->refresh_tok[0] == '\0')
    {
        ctx->auth = CROSSTALK_NET_DISCORD_AUTH_NONE;
        snprintf(ctx->err_text, sizeof(ctx->err_text), "token expired; no refresh token held");
        return;
    }

    crosstalk_net_discord_str_make(ctx->refresh_tok, &refresh);
    Discord_Client_RefreshToken(crosstalk_net_discord_client_get(ctx), ctx->app_id, refresh, (Discord_Client_TokenExchangeCallback)crosstalk_net_discord_tok_cb, crosstalk_net_discord_user_data_free_cb, ctx);
}

static void crosstalk_net_discord_authorize_cb(Discord_ClientResult *res, Discord_String code, Discord_String redirect_uri, void *user_data)
{
    crosstalk_net_discord_ctx_t *ctx = (crosstalk_net_discord_ctx_t *)user_data;
    Discord_String verifier = LDG_STRUCT_ZERO_INIT;

    if (!ctx) { return; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: authorize callback; redirect: %.*s", (uint32_t)redirect_uri.size, (const char *)redirect_uri.ptr);
    crosstalk_net_discord_auth_args_drop(ctx);

    if (!CROSSTALK_NET_DISCORD_SDK_OK(res))
    {
        if (ctx->auth_local_req == LDG_TRUTH_TRUE) { ctx->auth_fail_req = LDG_TRUTH_TRUE; }

        ctx->auth_local_req = LDG_TRUTH_FALSE;
        ctx->auth = CROSSTALK_NET_DISCORD_AUTH_NONE;
        crosstalk_net_discord_err_capture(ctx, res, "authorize failed");
        return;
    }

    ctx->auth_local_req = LDG_TRUTH_FALSE;

    if (ctx->verifier_live == LDG_TRUTH_FALSE)
    {
        snprintf(ctx->err_text, sizeof(ctx->err_text), "authorize returned without a live code verifier");
        return;
    }

    {
        Discord_String sdk_verifier = LDG_STRUCT_ZERO_INIT;

        Discord_AuthorizationCodeVerifier_Verifier(crosstalk_net_discord_verifier_get(ctx), &sdk_verifier);
        crosstalk_net_discord_str_copy(&sdk_verifier, ctx->verifier_str, (uint32_t)sizeof(ctx->verifier_str));
        Discord_Free(sdk_verifier.ptr);
    }

    crosstalk_net_discord_str_make(ctx->verifier_str, &verifier);

    Discord_Client_GetToken(crosstalk_net_discord_client_get(ctx), ctx->app_id, code, verifier, redirect_uri, (Discord_Client_TokenExchangeCallback)crosstalk_net_discord_tok_cb, crosstalk_net_discord_user_data_free_cb, ctx);
}

static void crosstalk_net_discord_guilds_cb(Discord_ClientResult *res, Discord_GuildMinimalSpan guilds, void *user_data)
{
    crosstalk_net_discord_ctx_t *ctx = (crosstalk_net_discord_ctx_t *)user_data;
    uint64_t span_cunt = 0;
    uint64_t idx = 0;

    if (!ctx) { return; }

    ctx->guild_req = LDG_TRUTH_FALSE;

    if (!CROSSTALK_NET_DISCORD_SDK_OK(res))
    {
        ctx->guild_next = CROSSTALK_NET_DISCORD_GUILD_RETRY;
        crosstalk_net_discord_err_capture(ctx, res, "guild list failed");
        return;
    }

    span_cunt = (uint64_t)guilds.size;
    ctx->guild_member = LDG_TRUTH_FALSE;

    for (idx = 0; idx < span_cunt; idx = idx + 1)
    {
        if (Discord_GuildMinimal_Id(&guilds.ptr[idx]) != CROSSTALK_NET_DISCORD_GUILD_ID) { continue; }

        ctx->guild_member = LDG_TRUTH_TRUE;
        break;
    }

    if (ctx->guild_member == LDG_TRUTH_TRUE) { ctx->guild_next = CROSSTALK_NET_DISCORD_GUILD_INTERVAL; }
    else { ctx->guild_next = CROSSTALK_NET_DISCORD_GUILD_RETRY; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: guild check; guilds: %u64; member: %u32", span_cunt, (uint32_t)ctx->guild_member);
}

static void crosstalk_net_discord_app_installed_cb(uint8_t installed, void *user_data)
{
    if (!user_data) { return; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: app installed: %u32", (uint32_t)installed);
}

static void crosstalk_net_discord_invite_name_resolve(crosstalk_net_discord_ctx_t *ctx, uint64_t user_id)
{
    Discord_UserHandle user = LDG_STRUCT_ZERO_INIT;
    Discord_String name = LDG_STRUCT_ZERO_INIT;

    ctx->invite_name[0] = LDG_STR_TERM;

    if (CROSSTALK_NET_DISCORD_SDK_BOOL(Discord_Client_GetUser(crosstalk_net_discord_client_get(ctx), user_id, &user)) == LDG_TRUTH_FALSE) { return; }

    Discord_UserHandle_DisplayName(&user, &name);
    if ((uint64_t)name.size == 0) { Discord_Free(name.ptr); Discord_UserHandle_Username(&user, &name); }

    crosstalk_net_discord_str_copy(&name, ctx->invite_name, (uint32_t)sizeof(ctx->invite_name));
    Discord_Free(name.ptr);
    Discord_UserHandle_Drop(&user);
}

static void crosstalk_net_discord_invite_created_cb(Discord_ActivityInvite *invite, void *user_data)
{
    crosstalk_net_discord_ctx_t *ctx = (crosstalk_net_discord_ctx_t *)user_data;

    if (!ctx || !invite) { return; }

    if (ctx->invite_live == LDG_TRUTH_TRUE)
    {
        Discord_ActivityInvite_Drop(crosstalk_net_discord_invite_get(ctx));
        ctx->invite_live = LDG_TRUTH_FALSE;
    }

    Discord_ActivityInvite_Clone(crosstalk_net_discord_invite_get(ctx), invite);
    ctx->invite_live = LDG_TRUTH_TRUE;
    ctx->invite_sender_id = Discord_ActivityInvite_SenderId(invite);
    crosstalk_net_discord_invite_name_resolve(ctx, ctx->invite_sender_id);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: invite recv; from: %.63s; id: %u64", ctx->invite_name, ctx->invite_sender_id);
}

static void crosstalk_net_discord_invite_accept_cb(Discord_ClientResult *res, Discord_String join_secret, void *user_data)
{
    crosstalk_net_discord_ctx_t *ctx = (crosstalk_net_discord_ctx_t *)user_data;

    if (!ctx) { return; }

    if (!CROSSTALK_NET_DISCORD_SDK_OK(res))
    {
        crosstalk_net_discord_err_capture(ctx, res, "invite accept failed");
        return;
    }

    crosstalk_net_discord_str_copy(&join_secret, ctx->join_secret_in, (uint32_t)sizeof(ctx->join_secret_in));
    ctx->join_pending = LDG_TRUTH_TRUE;
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: invite accepted; secret: %.63s", ctx->join_secret_in);
}

static void crosstalk_net_discord_invite_sent_cb(Discord_ClientResult *res, void *user_data)
{
    crosstalk_net_discord_ctx_t *ctx = (crosstalk_net_discord_ctx_t *)user_data;

    if (!ctx) { return; }

    if (!CROSSTALK_NET_DISCORD_SDK_OK(res))
    {
        crosstalk_net_discord_err_capture(ctx, res, "invite send failed");
        return;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: invite sent");
}

static void crosstalk_net_discord_msg_handle_ingest(crosstalk_net_discord_ctx_t *ctx, Discord_MessageHandle *handle, uint8_t hist)
{
    Discord_UserHandle author = LDG_STRUCT_ZERO_INIT;
    Discord_UserHandle recipient = LDG_STRUCT_ZERO_INIT;
    Discord_LobbyHandle lobby = LDG_STRUCT_ZERO_INIT;
    Discord_String text = LDG_STRUCT_ZERO_INIT;
    uint64_t author_id = 0;
    uint32_t slot = 0;
    uint32_t ret = 0;
    uint8_t dm = 0;

    if (CROSSTALK_NET_DISCORD_SDK_BOOL(Discord_MessageHandle_Lobby(handle, &lobby)) == LDG_TRUTH_TRUE) { Discord_LobbyHandle_Drop(&lobby); }
    else { dm = LDG_TRUTH_TRUE; }

    author_id = Discord_MessageHandle_AuthorId(handle);
    if (author_id == ctx->user_id && hist == LDG_TRUTH_FALSE) { return; }

    slot = (ctx->msg_hd + ctx->msg_cunt) % CROSSTALK_NET_DISCORD_MSG_RING;

    if (ctx->msg_cunt == CROSSTALK_NET_DISCORD_MSG_RING) { ctx->msg_hd = (ctx->msg_hd + CROSSTALK_NET_DISCORD_RING_STEP) % CROSSTALK_NET_DISCORD_MSG_RING; }
    else { ctx->msg_cunt = ctx->msg_cunt + CROSSTALK_NET_DISCORD_RING_STEP; }

    Discord_MessageHandle_Content(handle, &text);
    crosstalk_net_discord_str_copy(&text, ctx->msg[slot].text, (uint32_t)sizeof(ctx->msg[slot].text));
    Discord_Free(text.ptr);

    ctx->msg[slot].name[0] = LDG_STR_TERM;
    ctx->msg[slot].peer_name[0] = LDG_STR_TERM;
    ctx->msg[slot].author_id = author_id;
    ctx->msg[slot].peer_id = CROSSTALK_NET_DISCORD_USER_ID_NONE;
    ctx->msg[slot].dm = dm;
    ctx->msg[slot].hist = hist;

    if (CROSSTALK_NET_DISCORD_SDK_BOOL(Discord_MessageHandle_Author(handle, &author)) == LDG_TRUTH_TRUE)
    {
        Discord_UserHandle_DisplayName(&author, &text);
        if ((uint64_t)text.size == 0) { Discord_Free(text.ptr); Discord_UserHandle_Username(&author, &text); }

        crosstalk_net_discord_str_copy(&text, ctx->msg[slot].name, (uint32_t)sizeof(ctx->msg[slot].name));
        Discord_Free(text.ptr);
        Discord_UserHandle_Drop(&author);
    }

    if (dm == LDG_TRUTH_FALSE) { return; }

    if (author_id != ctx->user_id)
    {
        ctx->msg[slot].peer_id = author_id;
        ret = ldg_str_rbrcpy(ctx->msg[slot].peer_name, ctx->msg[slot].name, (uint64_t)sizeof(ctx->msg[slot].peer_name));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "discord peer name copy failed"); }

        return;
    }

    ctx->msg[slot].peer_id = Discord_MessageHandle_RecipientId(handle);

    if (CROSSTALK_NET_DISCORD_SDK_BOOL(Discord_MessageHandle_Recipient(handle, &recipient)) == LDG_TRUTH_TRUE)
    {
        Discord_UserHandle_DisplayName(&recipient, &text);
        if ((uint64_t)text.size == 0) { Discord_Free(text.ptr); Discord_UserHandle_Username(&recipient, &text); }

        crosstalk_net_discord_str_copy(&text, ctx->msg[slot].peer_name, (uint32_t)sizeof(ctx->msg[slot].peer_name));
        Discord_Free(text.ptr);
        Discord_UserHandle_Drop(&recipient);
    }
}

static void crosstalk_net_discord_history_cb(Discord_ClientResult *res, Discord_MessageHandleSpan msgs, void *user_data)
{
    crosstalk_net_discord_ctx_t *ctx = (crosstalk_net_discord_ctx_t *)user_data;

    if (!ctx) { return; }

    if (!CROSSTALK_NET_DISCORD_SDK_OK(res))
    {
        crosstalk_net_discord_err_capture(ctx, res, "message history failed");
        return;
    }

    {
        uint64_t span_cunt = (uint64_t)msgs.size;
        uint8_t newest_first = LDG_TRUTH_FALSE;

        if (span_cunt > CROSSTALK_NET_DISCORD_RING_STEP)
        {
            uint64_t first_ms = Discord_MessageHandle_SentTimestamp(&msgs.ptr[0]);
            uint64_t last_ms = Discord_MessageHandle_SentTimestamp(&msgs.ptr[span_cunt - CROSSTALK_NET_DISCORD_RING_STEP]);

            if (first_ms > last_ms) { newest_first = LDG_TRUTH_TRUE; }
        }

        for (uint64_t idx = 0; idx < span_cunt; idx++)
        {
            uint64_t pick = (newest_first == LDG_TRUTH_TRUE) ? (span_cunt - CROSSTALK_NET_DISCORD_RING_STEP - idx) : idx;

            crosstalk_net_discord_msg_handle_ingest(ctx, &msgs.ptr[pick], LDG_TRUTH_TRUE);
        }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: history filled; cunt: %u64; newest_first: %u32; ring: %u32", span_cunt, (uint32_t)newest_first, ctx->msg_cunt);
    }
}

static void crosstalk_net_discord_msg_created_cb(uint64_t msg_id, void *user_data)
{
    crosstalk_net_discord_ctx_t *ctx = (crosstalk_net_discord_ctx_t *)user_data;
    Discord_MessageHandle handle = LDG_STRUCT_ZERO_INIT;
    Discord_LobbyHandle lobby = LDG_STRUCT_ZERO_INIT;
    uint64_t author_id = 0;
    uint64_t msg_lobby_id = 0;
    uint32_t slot = 0;
    uint8_t dm = 0;

    if (!ctx) { return; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "discord: msg cb fired; msg: %u64; self: %u64; lobby_live: %u32; ring: %u32", msg_id, ctx->user_id, (uint32_t)ctx->lobby_live, ctx->msg_cunt);

    if (CROSSTALK_NET_DISCORD_SDK_BOOL(Discord_Client_GetMessageHandle(crosstalk_net_discord_client_get(ctx), msg_id, &handle)) == LDG_TRUTH_FALSE)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "discord: msg cb dropped; reason: no handle; msg: %u64", msg_id);
        return;
    }

    if (CROSSTALK_NET_DISCORD_SDK_BOOL(Discord_MessageHandle_Lobby(&handle, &lobby)) == LDG_TRUTH_TRUE)
    {
        msg_lobby_id = Discord_LobbyHandle_Id(&lobby);
        Discord_LobbyHandle_Drop(&lobby);
    }
    else { dm = LDG_TRUTH_TRUE; }

    author_id = Discord_MessageHandle_AuthorId(&handle);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "discord: msg cb classified; msg: %u64; author: %u64; dm: %u32; lobby: %u64", msg_id, author_id, (uint32_t)dm, msg_lobby_id);

    if (author_id == ctx->user_id)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "discord: msg cb dropped; reason: self echo; msg: %u64", msg_id);
        Discord_MessageHandle_Drop(&handle);
        return;
    }

    if (ctx->msg_cunt == CROSSTALK_NET_DISCORD_MSG_RING) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "discord: msg ring full; overwriting oldest; cap: %u32", (uint32_t)CROSSTALK_NET_DISCORD_MSG_RING); }

    crosstalk_net_discord_msg_handle_ingest(ctx, &handle, LDG_TRUTH_FALSE);
    slot = (ctx->msg_hd + ctx->msg_cunt + CROSSTALK_NET_DISCORD_MSG_RING - CROSSTALK_NET_DISCORD_RING_STEP) % CROSSTALK_NET_DISCORD_MSG_RING;

    Discord_MessageHandle_Drop(&handle);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: chat recv; from: %.63s; author: %u64; dm: %u32; slot: %u32; ring: %u32", ctx->msg[slot].name, author_id, (uint32_t)dm, slot, ctx->msg_cunt);
}

static void crosstalk_net_discord_msg_sent_cb(Discord_ClientResult *res, uint64_t msg_id, void *user_data)
{
    crosstalk_net_discord_ctx_t *ctx = (crosstalk_net_discord_ctx_t *)user_data;

    if (!ctx) { return; }

    if (!CROSSTALK_NET_DISCORD_SDK_OK(res))
    {
        crosstalk_net_discord_err_capture(ctx, res, "chat send failed");
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "discord: chat send failed; msg: %u64; detail: %.159s", msg_id, ctx->err_text);
        return;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: chat sent; msg: %u64", msg_id);
}

static void crosstalk_net_discord_presence_cb(Discord_ClientResult *res, void *user_data)
{
    crosstalk_net_discord_ctx_t *ctx = (crosstalk_net_discord_ctx_t *)user_data;

    if (!ctx) { return; }

    if (!CROSSTALK_NET_DISCORD_SDK_OK(res))
    {
        crosstalk_net_discord_err_capture(ctx, res, "presence update failed");
        return;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "discord: presence accepted; kind: %u32", ctx->presence_sent.kind);
}

static void crosstalk_net_discord_dev_span_copy(Discord_AudioDeviceSpan dev_span, crosstalk_net_discord_dev_t *out, uint32_t cap, uint32_t *out_cunt)
{
    Discord_String text = LDG_STRUCT_ZERO_INIT;
    uint64_t total = 0;

    if (LDG_UNLIKELY(!out || !out_cunt)) { return; }

    *out_cunt = 0;
    total = (uint64_t)dev_span.size;
    if (total > (uint64_t)cap) { total = (uint64_t)cap; }

    for (uint32_t idx = 0; idx < (uint32_t)total; idx++)
    {
        Discord_AudioDevice_Id(&dev_span.ptr[idx], &text);
        crosstalk_net_discord_str_copy(&text, out[idx].id, (uint32_t)CROSSTALK_NET_DISCORD_DEV_ID_CAP);
        Discord_Free(text.ptr);

        Discord_AudioDevice_Name(&dev_span.ptr[idx], &text);
        crosstalk_net_discord_str_copy(&text, out[idx].name, (uint32_t)CROSSTALK_NET_DISCORD_DEV_NAME_CAP);
        Discord_Free(text.ptr);
    }

    *out_cunt = (uint32_t)total;
}

static void crosstalk_net_discord_dev_in_cb(Discord_AudioDeviceSpan dev_span, void *user_data)
{
    crosstalk_net_discord_ctx_t *ctx = (crosstalk_net_discord_ctx_t *)user_data;

    if (!ctx) { return; }

    crosstalk_net_discord_dev_span_copy(dev_span, ctx->dev_in, (uint32_t)CROSSTALK_NET_DISCORD_DEV_CAP, &ctx->dev_in_cunt);
}

static void crosstalk_net_discord_dev_out_cb(Discord_AudioDeviceSpan dev_span, void *user_data)
{
    crosstalk_net_discord_ctx_t *ctx = (crosstalk_net_discord_ctx_t *)user_data;

    if (!ctx) { return; }

    crosstalk_net_discord_dev_span_copy(dev_span, ctx->dev_out, (uint32_t)CROSSTALK_NET_DISCORD_DEV_CAP, &ctx->dev_out_cunt);
}

static void crosstalk_net_discord_dev_change_cb(Discord_AudioDeviceSpan in_span, Discord_AudioDeviceSpan out_span, void *user_data)
{
    crosstalk_net_discord_ctx_t *ctx = (crosstalk_net_discord_ctx_t *)user_data;

    if (!ctx) { return; }

    crosstalk_net_discord_dev_span_copy(in_span, ctx->dev_in, (uint32_t)CROSSTALK_NET_DISCORD_DEV_CAP, &ctx->dev_in_cunt);
    crosstalk_net_discord_dev_span_copy(out_span, ctx->dev_out, (uint32_t)CROSSTALK_NET_DISCORD_DEV_CAP, &ctx->dev_out_cunt);
}

static void crosstalk_net_discord_dev_set_cb(Discord_ClientResult *res, void *user_data)
{
    crosstalk_net_discord_ctx_t *ctx = (crosstalk_net_discord_ctx_t *)user_data;

    if (!ctx) { return; }

    if (!CROSSTALK_NET_DISCORD_SDK_OK(res)) { crosstalk_net_discord_err_capture(ctx, res, "audio dev select failed"); }
}

static void crosstalk_net_discord_no_input_cb(uint8_t in_detected, void *user_data)
{
    crosstalk_net_discord_ctx_t *ctx = (crosstalk_net_discord_ctx_t *)user_data;

    if (!ctx) { return; }

    if (in_detected != LDG_TRUTH_FALSE)
    {
        ctx->err_text[0] = '\0';
        return;
    }

    snprintf(ctx->err_text, sizeof(ctx->err_text), "no microphone input detected");
}

static void crosstalk_net_discord_call_status_cb(int32_t status, int32_t err, int32_t err_detail, void *user_data)
{
    crosstalk_net_discord_ctx_t *ctx = (crosstalk_net_discord_ctx_t *)user_data;
    Discord_String text = LDG_STRUCT_ZERO_INIT;

    if (!ctx) { return; }

    if (status == (int32_t)Discord_Call_Status_Connected)
    {
        ctx->call_state = CROSSTALK_NET_DISCORD_CALL_LIVE;
        return;
    }

    if (status == (int32_t)Discord_Call_Status_Joining || status == (int32_t)Discord_Call_Status_Connecting || status == (int32_t)Discord_Call_Status_SignalingConnected || status == (int32_t)Discord_Call_Status_Reconnecting)
    {
        ctx->call_state = CROSSTALK_NET_DISCORD_CALL_CONNECTING;
        return;
    }

    ctx->call_state = CROSSTALK_NET_DISCORD_CALL_OFF;

    if (err != (int32_t)Discord_Call_Error_None)
    {
        ctx->call_state = CROSSTALK_NET_DISCORD_CALL_FAIL;
        Discord_Call_ErrorToString((Discord_Call_Error)err, &text);
        snprintf(ctx->err_text, sizeof(ctx->err_text), "voice failed; err: %.*s; detail: %" PRId32, (int32_t)text.size, (const char *)text.ptr, err_detail);
        Discord_Free(text.ptr);
    }
}

static double crosstalk_net_discord_call_retry_delay_get(uint32_t fail_cunt)
{
    double delay = CROSSTALK_NET_DISCORD_CALL_RETRY_MIN;

    for (uint32_t idx = 1; idx < fail_cunt; idx++)
    {
        delay *= CROSSTALK_NET_DISCORD_CALL_RETRY_GROWTH;
        if (delay >= CROSSTALK_NET_DISCORD_CALL_RETRY_MAX) { return CROSSTALK_NET_DISCORD_CALL_RETRY_MAX; }
    }

    return delay;
}

static void crosstalk_net_discord_call_backoff_arm(crosstalk_net_discord_ctx_t *ctx)
{
    ctx->call_req = LDG_TRUTH_FALSE;
    ctx->call_state = CROSSTALK_NET_DISCORD_CALL_FAIL;
    ctx->call_fail_cunt++;
    ctx->call_next = crosstalk_net_discord_call_retry_delay_get(ctx->call_fail_cunt);
}

static void crosstalk_net_discord_call_start(crosstalk_net_discord_ctx_t *ctx)
{
    if (ctx->lobby_live == LDG_TRUTH_FALSE) { return; }

    if (ctx->call_live == LDG_TRUTH_TRUE) { return; }

    if (CROSSTALK_NET_DISCORD_SDK_BOOL(Discord_Client_StartCall(crosstalk_net_discord_client_get(ctx), ctx->call_lobby_id, crosstalk_net_discord_call_get(ctx))) == LDG_TRUTH_FALSE)
    {
        ctx->call_state = CROSSTALK_NET_DISCORD_CALL_FAIL;
        snprintf(ctx->err_text, sizeof(ctx->err_text), "voice call start rejected; lobby: %" PRIu64, ctx->call_lobby_id);
        return;
    }

    ctx->call_live = LDG_TRUTH_TRUE;
    ctx->call_state = CROSSTALK_NET_DISCORD_CALL_CONNECTING;
    Discord_Call_SetStatusChangedCallback(crosstalk_net_discord_call_get(ctx), (Discord_Call_OnStatusChanged)crosstalk_net_discord_call_status_cb, crosstalk_net_discord_user_data_free_cb, ctx);
    Discord_Client_SetSelfMuteAll(crosstalk_net_discord_client_get(ctx), (int32_t)ctx->self_mute);
    Discord_Client_SetSelfDeafAll(crosstalk_net_discord_client_get(ctx), (int32_t)ctx->self_deaf);
}

static void crosstalk_net_discord_lobby_leave_cb(Discord_ClientResult *res, void *user_data)
{
    crosstalk_net_discord_ctx_t *ctx = (crosstalk_net_discord_ctx_t *)user_data;

    if (!ctx) { return; }

    if (!CROSSTALK_NET_DISCORD_SDK_OK(res)) { crosstalk_net_discord_err_capture(ctx, res, "discord lobby leave failed"); }
}

static void crosstalk_net_discord_voice_lobby_cb(Discord_ClientResult *res, uint64_t lobby_id, void *user_data)
{
    crosstalk_net_discord_ctx_t *ctx = (crosstalk_net_discord_ctx_t *)user_data;

    if (!ctx) { return; }

    if (ctx->call_req == LDG_TRUTH_FALSE)
    {
        if (CROSSTALK_NET_DISCORD_SDK_OK(res)) { Discord_Client_LeaveLobby(crosstalk_net_discord_client_get(ctx), lobby_id, crosstalk_net_discord_lobby_leave_cb, crosstalk_net_discord_user_data_free_cb, ctx); }

        return;
    }

    if (!CROSSTALK_NET_DISCORD_SDK_OK(res))
    {
        crosstalk_net_discord_call_backoff_arm(ctx);
        crosstalk_net_discord_err_capture(ctx, res, "discord lobby join failed");
        return;
    }

    ctx->call_req = LDG_TRUTH_FALSE;
    ctx->call_fail_cunt = 0;
    ctx->call_next = CROSSTALK_NET_DISCORD_TIMER_DUE;
    ctx->lobby_live = LDG_TRUTH_TRUE;
    ctx->call_lobby_id = lobby_id;
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: lobby joined; lobby: %u64; voice: %u32", lobby_id, (uint32_t)ctx->voice_on);
    Discord_Client_GetLobbyMessagesWithLimit(crosstalk_net_discord_client_get(ctx), lobby_id, (int32_t)CROSSTALK_NET_DISCORD_HISTORY_LIMIT, (Discord_Client_GetLobbyMessagesCallback)crosstalk_net_discord_history_cb, crosstalk_net_discord_user_data_free_cb, ctx);

    if (ctx->voice_on == LDG_TRUTH_FALSE)
    {
        ctx->call_state = CROSSTALK_NET_DISCORD_CALL_OFF;
        return;
    }

    crosstalk_net_discord_call_start(ctx);
}

static void crosstalk_net_discord_end_call_cb(void *user_data)
{
    crosstalk_net_discord_ctx_t *ctx = (crosstalk_net_discord_ctx_t *)user_data;

    if (!ctx) { return; }

    ctx->call_state = CROSSTALK_NET_DISCORD_CALL_OFF;
}

static void crosstalk_net_discord_call_end(crosstalk_net_discord_ctx_t *ctx)
{
    if (ctx->call_live == LDG_TRUTH_FALSE) { return; }

    Discord_Client_EndCall(crosstalk_net_discord_client_get(ctx), ctx->call_lobby_id, crosstalk_net_discord_end_call_cb, crosstalk_net_discord_user_data_free_cb, ctx);
    Discord_Call_Drop(crosstalk_net_discord_call_get(ctx));

    memset(&ctx->call[0], CROSSTALK_NET_DISCORD_FILL, sizeof(ctx->call));
    ctx->call_live = LDG_TRUTH_FALSE;
    ctx->call_state = CROSSTALK_NET_DISCORD_CALL_OFF;
}

static void crosstalk_net_discord_dm_summary_cb(Discord_ClientResult *res, Discord_UserMessageSummarySpan summaries, void *user_data)
{
    crosstalk_net_discord_ctx_t *ctx = (crosstalk_net_discord_ctx_t *)user_data;
    uint32_t cunt = 0;

    if (!ctx) { return; }

    if (!CROSSTALK_NET_DISCORD_SDK_OK(res))
    {
        crosstalk_net_discord_err_capture(ctx, res, "dm summaries failed");
        return;
    }

    for (uint64_t idx = 0; idx < (uint64_t)summaries.size && cunt < (uint32_t)CROSSTALK_NET_DISCORD_DM_CAP; idx++)
    {
        uint64_t dm_id = Discord_UserMessageSummary_UserId(&summaries.ptr[idx]);

        if (dm_id == 0 || dm_id == ctx->user_id) { continue; }

        ctx->dm_user[cunt] = dm_id;
        cunt++;
    }

    if (cunt != ctx->dm_cunt) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: dm conversations; cunt: %u32", cunt); }

    ctx->dm_cunt = cunt;
}

static uint32_t crosstalk_net_discord_contact_slot_get(crosstalk_net_discord_ctx_t *ctx, uint64_t user_id)
{
    for (uint32_t idx = 0; idx < ctx->contact_cunt; idx++) { if (ctx->contact[idx].user_id == user_id) { return idx; } }

    if (ctx->contact_cunt >= (uint32_t)CROSSTALK_NET_DISCORD_CONTACT_CAP) { return UINT32_MAX; }

    ctx->contact[ctx->contact_cunt].user_id = user_id;
    ctx->contact[ctx->contact_cunt].name[0] = LDG_STR_TERM;
    ctx->contact[ctx->contact_cunt].status = (uint32_t)CROSSTALK_NET_DISCORD_STATUS_UNKNOWN;
    ctx->contact[ctx->contact_cunt].kind = (uint8_t)CROSSTALK_NET_DISCORD_CONTACT_KIND_OTHER;
    ctx->contact[ctx->contact_cunt].conn = LDG_TRUTH_FALSE;
    ctx->contact[ctx->contact_cunt].provisional = LDG_TRUTH_FALSE;
    ctx->contact_cunt++;

    return ctx->contact_cunt - CROSSTALK_NET_DISCORD_RING_STEP;
}

static uint8_t crosstalk_net_discord_activity_kind_get(int32_t type)
{
    if (type == (int32_t)Discord_ActivityTypes_Playing) { return (uint8_t)CROSSTALK_NET_DISCORD_ACTIVITY_PLAYING; }

    if (type == (int32_t)Discord_ActivityTypes_Streaming) { return (uint8_t)CROSSTALK_NET_DISCORD_ACTIVITY_STREAMING; }

    if (type == (int32_t)Discord_ActivityTypes_Listening) { return (uint8_t)CROSSTALK_NET_DISCORD_ACTIVITY_LISTENING; }

    if (type == (int32_t)Discord_ActivityTypes_Watching) { return (uint8_t)CROSSTALK_NET_DISCORD_ACTIVITY_WATCHING; }

    if (type == (int32_t)Discord_ActivityTypes_CustomStatus) { return (uint8_t)CROSSTALK_NET_DISCORD_ACTIVITY_CUSTOM; }

    if (type == (int32_t)Discord_ActivityTypes_Competing) { return (uint8_t)CROSSTALK_NET_DISCORD_ACTIVITY_COMPETING; }

    if (type == (int32_t)Discord_ActivityTypes_HangStatus) { return (uint8_t)CROSSTALK_NET_DISCORD_ACTIVITY_HANG; }

    return (uint8_t)CROSSTALK_NET_DISCORD_ACTIVITY_NONE;
}

static void crosstalk_net_discord_contact_activity_fill(crosstalk_net_discord_ctx_t *ctx, uint32_t slot, Discord_UserHandle *user)
{
    Discord_Activity activity = LDG_STRUCT_ZERO_INIT;
    Discord_String text = LDG_STRUCT_ZERO_INIT;
    uint64_t app_id = 0;

    ctx->contact[slot].activity = (uint8_t)CROSSTALK_NET_DISCORD_ACTIVITY_NONE;
    ctx->contact[slot].activity_name[0] = LDG_STR_TERM;
    ctx->contact[slot].ours = LDG_TRUTH_FALSE;

    if (CROSSTALK_NET_DISCORD_SDK_BOOL(Discord_UserHandle_GameActivity(user, &activity)) == LDG_TRUTH_FALSE) { return; }

    ctx->contact[slot].activity = crosstalk_net_discord_activity_kind_get((int32_t)Discord_Activity_Type(&activity));

    if (CROSSTALK_NET_DISCORD_SDK_BOOL(Discord_Activity_ApplicationId(&activity, &app_id)) == LDG_TRUTH_TRUE && app_id == CROSSTALK_NET_DISCORD_APP_ID) { ctx->contact[slot].ours = LDG_TRUTH_TRUE; }

    if (ctx->contact[slot].activity == (uint8_t)CROSSTALK_NET_DISCORD_ACTIVITY_CUSTOM) { if (CROSSTALK_NET_DISCORD_SDK_BOOL(Discord_Activity_State(&activity, &text)) == LDG_TRUTH_FALSE) { text.ptr = 0x0; text.size = 0; } }
    else { Discord_Activity_Name(&activity, &text); }

    if ((uint64_t)text.size == 0)
    {
        Discord_Free(text.ptr);
        text.ptr = 0x0;
        text.size = 0;

        if (ctx->contact[slot].activity == (uint8_t)CROSSTALK_NET_DISCORD_ACTIVITY_CUSTOM) { Discord_Activity_Name(&activity, &text); }
        else if (CROSSTALK_NET_DISCORD_SDK_BOOL(Discord_Activity_State(&activity, &text)) == LDG_TRUTH_FALSE) { text.ptr = 0x0; text.size = 0; }
    }

    if ((uint64_t)text.size != 0) { crosstalk_net_discord_str_copy(&text, ctx->contact[slot].activity_name, (uint32_t)sizeof(ctx->contact[slot].activity_name)); }

    Discord_Free(text.ptr);
    Discord_Activity_Drop(&activity);
}

static void crosstalk_net_discord_contact_user_fill(crosstalk_net_discord_ctx_t *ctx, uint32_t slot, Discord_UserHandle *user)
{
    Discord_String text = LDG_STRUCT_ZERO_INIT;

    Discord_UserHandle_DisplayName(user, &text);
    if ((uint64_t)text.size == 0) { Discord_Free(text.ptr); Discord_UserHandle_Username(user, &text); }

    if ((uint64_t)text.size != 0) { crosstalk_net_discord_str_copy(&text, ctx->contact[slot].name, (uint32_t)sizeof(ctx->contact[slot].name)); }

    Discord_Free(text.ptr);

    ctx->contact[slot].status = (uint32_t)Discord_UserHandle_Status(user);
    ctx->contact[slot].provisional = CROSSTALK_NET_DISCORD_SDK_BOOL(Discord_UserHandle_IsProvisional(user));

    crosstalk_net_discord_contact_activity_fill(ctx, slot, user);
}

static uint8_t crosstalk_net_discord_rel_kind_get(int32_t rel)
{
    if (rel == (int32_t)Discord_RelationshipType_Friend) { return (uint8_t)CROSSTALK_NET_DISCORD_CONTACT_KIND_FRIEND; }

    if (rel == (int32_t)Discord_RelationshipType_PendingIncoming) { return (uint8_t)CROSSTALK_NET_DISCORD_CONTACT_KIND_PENDING_IN; }

    if (rel == (int32_t)Discord_RelationshipType_PendingOutgoing) { return (uint8_t)CROSSTALK_NET_DISCORD_CONTACT_KIND_PENDING_OUT; }

    return (uint8_t)CROSSTALK_NET_DISCORD_CONTACT_KIND_OTHER;
}

static void crosstalk_net_discord_rel_collect(crosstalk_net_discord_ctx_t *ctx)
{
    Discord_RelationshipHandleSpan rels = LDG_STRUCT_ZERO_INIT;

    Discord_Client_GetRelationships(crosstalk_net_discord_client_get(ctx), &rels);

    for (uint64_t idx = 0; idx < (uint64_t)rels.size; idx++)
    {
        Discord_UserHandle user = LDG_STRUCT_ZERO_INIT;
        uint64_t rel_id = Discord_RelationshipHandle_Id(&rels.ptr[idx]);
        int32_t rel_type = (int32_t)Discord_RelationshipHandle_DiscordRelationshipType(&rels.ptr[idx]);
        int32_t game_type = (int32_t)Discord_RelationshipHandle_GameRelationshipType(&rels.ptr[idx]);
        uint8_t kind = 0;
        uint32_t slot = 0;

        if (rel_id == ctx->user_id) { continue; }

        if (rel_type == (int32_t)Discord_RelationshipType_Blocked) { continue; }

        kind = crosstalk_net_discord_rel_kind_get(rel_type);
        if (kind == (uint8_t)CROSSTALK_NET_DISCORD_CONTACT_KIND_OTHER) { kind = crosstalk_net_discord_rel_kind_get(game_type); }

        if (kind == (uint8_t)CROSSTALK_NET_DISCORD_CONTACT_KIND_OTHER && rel_type != (int32_t)Discord_RelationshipType_Implicit) { continue; }

        slot = crosstalk_net_discord_contact_slot_get(ctx, rel_id);
        if (slot == UINT32_MAX) { break; }

        ctx->contact[slot].kind = kind;

        if (CROSSTALK_NET_DISCORD_SDK_BOOL(Discord_RelationshipHandle_User(&rels.ptr[idx], &user)) == LDG_TRUTH_TRUE)
        {
            crosstalk_net_discord_contact_user_fill(ctx, slot, &user);
            Discord_UserHandle_Drop(&user);
        }
    }

    Discord_Free(rels.ptr);
}

static void crosstalk_net_discord_dm_collect(crosstalk_net_discord_ctx_t *ctx)
{
    for (uint32_t idx = 0; idx < ctx->dm_cunt; idx++)
    {
        Discord_UserHandle user = LDG_STRUCT_ZERO_INIT;
        uint32_t slot = crosstalk_net_discord_contact_slot_get(ctx, ctx->dm_user[idx]);

        if (slot == UINT32_MAX) { break; }

        if (ctx->contact[slot].kind == (uint8_t)CROSSTALK_NET_DISCORD_CONTACT_KIND_OTHER) { ctx->contact[slot].kind = (uint8_t)CROSSTALK_NET_DISCORD_CONTACT_KIND_DM; }

        if (ctx->contact[slot].name[0] != LDG_STR_TERM) { continue; }

        if (CROSSTALK_NET_DISCORD_SDK_BOOL(Discord_Client_GetUser(crosstalk_net_discord_client_get(ctx), ctx->dm_user[idx], &user)) == LDG_TRUTH_TRUE)
        {
            crosstalk_net_discord_contact_user_fill(ctx, slot, &user);
            Discord_UserHandle_Drop(&user);
        }
    }
}

static void crosstalk_net_discord_lobby_collect(crosstalk_net_discord_ctx_t *ctx)
{
    Discord_LobbyHandle lobby = LDG_STRUCT_ZERO_INIT;
    Discord_UInt64Span ids = LDG_STRUCT_ZERO_INIT;

    if (ctx->lobby_live == LDG_TRUTH_FALSE) { return; }

    if (CROSSTALK_NET_DISCORD_SDK_BOOL(Discord_Client_GetLobbyHandle(crosstalk_net_discord_client_get(ctx), ctx->call_lobby_id, &lobby)) == LDG_TRUTH_FALSE) { return; }

    Discord_LobbyHandle_LobbyMemberIds(&lobby, &ids);

    for (uint64_t idx = 0; idx < (uint64_t)ids.size; idx++)
    {
        Discord_LobbyMemberHandle member = LDG_STRUCT_ZERO_INIT;
        Discord_UserHandle user = LDG_STRUCT_ZERO_INIT;
        uint64_t member_id = ids.ptr[idx];
        uint32_t slot = 0;

        if (member_id == ctx->user_id) { continue; }

        slot = crosstalk_net_discord_contact_slot_get(ctx, member_id);
        if (slot == UINT32_MAX) { break; }

        if (CROSSTALK_NET_DISCORD_SDK_BOOL(Discord_LobbyHandle_GetLobbyMemberHandle(&lobby, member_id, &member)) == LDG_TRUTH_FALSE) { continue; }

        ctx->contact[slot].conn = CROSSTALK_NET_DISCORD_SDK_BOOL(Discord_LobbyMemberHandle_Connected(&member));
        if (ctx->contact[slot].kind == (uint8_t)CROSSTALK_NET_DISCORD_CONTACT_KIND_OTHER) { ctx->contact[slot].kind = (uint8_t)CROSSTALK_NET_DISCORD_CONTACT_KIND_LOBBY; }

        if (ctx->contact[slot].name[0] == LDG_STR_TERM && CROSSTALK_NET_DISCORD_SDK_BOOL(Discord_LobbyMemberHandle_User(&member, &user)) == LDG_TRUTH_TRUE)
        {
            crosstalk_net_discord_contact_user_fill(ctx, slot, &user);
            Discord_UserHandle_Drop(&user);
        }

        Discord_LobbyMemberHandle_Drop(&member);
    }

    Discord_Free(ids.ptr);
    Discord_LobbyHandle_Drop(&lobby);
}

static void crosstalk_net_discord_contacts_refresh(crosstalk_net_discord_ctx_t *ctx)
{
    uint32_t prev = ctx->contact_cunt;

    ctx->contact_cunt = 0;

    if (ctx->state != CROSSTALK_NET_DISCORD_STATE_RDY)
    {
        ctx->dm_cunt = 0;
        return;
    }

    crosstalk_net_discord_rel_collect(ctx);
    crosstalk_net_discord_dm_collect(ctx);
    crosstalk_net_discord_lobby_collect(ctx);

    Discord_Client_GetUserMessageSummaries(crosstalk_net_discord_client_get(ctx), crosstalk_net_discord_dm_summary_cb, crosstalk_net_discord_user_data_free_cb, ctx);

    if (prev != ctx->contact_cunt) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: contacts refreshed; cunt: %u32; dm: %u32", ctx->contact_cunt, ctx->dm_cunt); }
}

static void crosstalk_net_discord_presence_flush(crosstalk_net_discord_ctx_t *ctx)
{
    Discord_Activity activity = LDG_STRUCT_ZERO_INIT;
    Discord_ActivityAssets assets = LDG_STRUCT_ZERO_INIT;
    Discord_ActivityTimestamps stamps = LDG_STRUCT_ZERO_INIT;
    Discord_ActivityParty party = LDG_STRUCT_ZERO_INIT;
    Discord_ActivitySecrets secrets = LDG_STRUCT_ZERO_INIT;
    Discord_ActivityButton button = LDG_STRUCT_ZERO_INIT;
    Discord_String text = LDG_STRUCT_ZERO_INIT;
    uint64_t start = 0;
    uint8_t party_on = 0;
    uint8_t secret_on = 0;
    uint8_t asset_on = 0;

    if (ctx->presence.kind == CROSSTALK_NET_DISCORD_KIND_NONE)
    {
        Discord_Client_ClearRichPresence(crosstalk_net_discord_client_get(ctx));
        memcpy(&ctx->presence_sent, &ctx->presence, sizeof(ctx->presence_sent));
        return;
    }

    party_on = (ctx->presence.party_cunt_max > 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    secret_on = (ctx->presence.joinable == LDG_TRUTH_TRUE && ctx->presence.join_secret[0] != '\0') ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    Discord_Activity_Init(&activity);
    Discord_Activity_SetType(&activity, Discord_ActivityTypes_Playing);

    if (ctx->presence.details[0] != LDG_STR_TERM)
    {
        const char *details = ctx->presence.details;

        if (ctx->presence.paused == LDG_TRUTH_TRUE && ctx->presence.kind == CROSSTALK_NET_DISCORD_KIND_SANDBOX) { details = CROSSTALK_NET_DISCORD_TEXT_SANDBOX_PAUSED; }
        else if (ctx->presence.paused == LDG_TRUTH_TRUE && ctx->presence.kind == CROSSTALK_NET_DISCORD_KIND_LOBBY) { details = CROSSTALK_NET_DISCORD_TEXT_LOBBY_PAUSED; }

        crosstalk_net_discord_str_make(details, &text);
        Discord_Activity_SetDetails(&activity, &text);
    }

    if (ctx->presence.state[0] != LDG_STR_TERM || ctx->presence.music[0] != LDG_STR_TERM || ctx->presence.activity[0] != LDG_STR_TERM)
    {
        char state_out[CROSSTALK_NET_DISCORD_STATE_CAP] = LDG_ARR_ZERO_INIT;
        const char *base = (ctx->presence.activity[0] != LDG_STR_TERM) ? ctx->presence.activity : ctx->presence.state;

        if (ctx->presence.music[0] == LDG_STR_TERM) { snprintf(state_out, sizeof(state_out), "%.127s", base); }
        else if (base[0] == LDG_STR_TERM) { snprintf(state_out, sizeof(state_out), "%.127s", ctx->presence.music); }
        else { snprintf(state_out, sizeof(state_out), "%.29s" CROSSTALK_NET_DISCORD_MUSIC_SEP "%.94s", base, ctx->presence.music); }

        crosstalk_net_discord_str_make(state_out, &text);
        Discord_Activity_SetState(&activity, &text);
    }

    if (ctx->presence.asset_large[0] != LDG_STR_TERM)
    {
        asset_on = LDG_TRUTH_TRUE;
        Discord_ActivityAssets_Init(&assets);
        crosstalk_net_discord_str_make(ctx->presence.asset_large, &text);
        Discord_ActivityAssets_SetLargeImage(&assets, &text);

        if (ctx->presence.asset_large_text[0] != LDG_STR_TERM)
        {
            crosstalk_net_discord_str_make(ctx->presence.asset_large_text, &text);
            Discord_ActivityAssets_SetLargeText(&assets, &text);
        }

        if (ctx->presence.asset_large_url[0] != LDG_STR_TERM)
        {
            crosstalk_net_discord_str_make(ctx->presence.asset_large_url, &text);
            Discord_ActivityAssets_SetLargeUrl(&assets, &text);
        }

        if (ctx->presence.paused != LDG_TRUTH_FALSE)
        {
            crosstalk_net_discord_str_make(CROSSTALK_NET_DISCORD_BADGE_PAUSED, &text);
            Discord_ActivityAssets_SetSmallImage(&assets, &text);
            crosstalk_net_discord_str_make(CROSSTALK_NET_DISCORD_BADGE_TEXT_PAUSED, &text);
            Discord_ActivityAssets_SetSmallText(&assets, &text);
        }
        else if (ctx->presence.asset_small[0] != LDG_STR_TERM)
        {
            crosstalk_net_discord_str_make(ctx->presence.asset_small, &text);
            Discord_ActivityAssets_SetSmallImage(&assets, &text);

            if (ctx->presence.asset_small_text[0] != LDG_STR_TERM)
            {
                crosstalk_net_discord_str_make(ctx->presence.asset_small_text, &text);
                Discord_ActivityAssets_SetSmallText(&assets, &text);
            }
        }

        if (ctx->presence.asset_small_url[0] != LDG_STR_TERM)
        {
            crosstalk_net_discord_str_make(ctx->presence.asset_small_url, &text);
            Discord_ActivityAssets_SetSmallUrl(&assets, &text);
        }

        Discord_Activity_SetAssets(&activity, &assets);
    }

    start = ctx->presence.start_epoch_s;
    Discord_ActivityTimestamps_Init(&stamps);
    Discord_ActivityTimestamps_SetStart(&stamps, start);
    if (ctx->presence.end_epoch_s > CROSSTALK_NET_DISCORD_STAMP_NONE) { Discord_ActivityTimestamps_SetEnd(&stamps, (uint64_t)ctx->presence.end_epoch_s); }

    Discord_Activity_SetTimestamps(&activity, &stamps);

    if (party_on == LDG_TRUTH_TRUE)
    {
        Discord_ActivityParty_Init(&party);
        crosstalk_net_discord_str_make(ctx->presence.party_id, &text);
        Discord_ActivityParty_SetId(&party, text);
        Discord_ActivityParty_SetCurrentSize(&party, (int32_t)ctx->presence.party_cunt);
        Discord_ActivityParty_SetMaxSize(&party, (int32_t)ctx->presence.party_cunt_max);
        Discord_ActivityParty_SetPrivacy(&party, (ctx->presence.joinable == LDG_TRUTH_TRUE) ? Discord_ActivityPartyPrivacy_Public : Discord_ActivityPartyPrivacy_Private);
        Discord_Activity_SetParty(&activity, &party);
    }

    if (secret_on == LDG_TRUTH_TRUE)
    {
        Discord_ActivitySecrets_Init(&secrets);
        crosstalk_net_discord_str_make(ctx->presence.join_secret, &text);
        Discord_ActivitySecrets_SetJoin(&secrets, text);
        Discord_Activity_SetSecrets(&activity, &secrets);
    }

    if (secret_on != LDG_TRUTH_TRUE)
    {
        Discord_ActivityButton_Init(&button);
        crosstalk_net_discord_str_make(CROSSTALK_NET_DISCORD_BTN_LABEL, &text);
        Discord_ActivityButton_SetLabel(&button, text);
        crosstalk_net_discord_str_make(CROSSTALK_NET_DISCORD_STORE_URL, &text);
        Discord_ActivityButton_SetUrl(&button, text);
        Discord_Activity_AddButton(&activity, &button);
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: presence push; kind: %u32; details: %.63s; state: %.63s; activity: %.95s; party: %u32/%u32", ctx->presence.kind, ctx->presence.details, ctx->presence.state, ctx->presence.activity, ctx->presence.party_cunt, ctx->presence.party_cunt_max);

    Discord_Client_UpdateRichPresence(crosstalk_net_discord_client_get(ctx), &activity, crosstalk_net_discord_presence_cb, crosstalk_net_discord_user_data_free_cb, ctx);

    if (secret_on != LDG_TRUTH_TRUE) { Discord_ActivityButton_Drop(&button); }

    if (secret_on == LDG_TRUTH_TRUE) { Discord_ActivitySecrets_Drop(&secrets); }

    if (party_on == LDG_TRUTH_TRUE) { Discord_ActivityParty_Drop(&party); }

    Discord_ActivityTimestamps_Drop(&stamps);
    if (asset_on == LDG_TRUTH_TRUE) { Discord_ActivityAssets_Drop(&assets); }

    Discord_Activity_Drop(&activity);

    memcpy(&ctx->presence_sent, &ctx->presence, sizeof(ctx->presence_sent));
}

static void crosstalk_net_discord_badge_set(crosstalk_net_discord_ctx_t *ctx, const char *key, const char *text)
{
    snprintf(ctx->presence.asset_small, sizeof(ctx->presence.asset_small), "%.63s", key);
    snprintf(ctx->presence.asset_small_text, sizeof(ctx->presence.asset_small_text), "%.63s", text);
}

static void crosstalk_net_discord_presence_stage(crosstalk_net_discord_ctx_t *ctx, uint32_t kind)
{
    if (ctx->presence.kind != kind || ctx->presence.start_epoch_s == 0) { ctx->presence.start_epoch_s = ldg_time_epoch_ms_get() / LDG_MS_PER_SEC; }

    ctx->presence.kind = kind;
    snprintf(ctx->presence.asset_large, sizeof(ctx->presence.asset_large), "%.63s", CROSSTALK_NET_DISCORD_ASSET_LARGE);
    crosstalk_net_discord_badge_set(ctx, CROSSTALK_NET_DISCORD_ASSET_SMALL, CROSSTALK_NET_DISCORD_ASSET_SMALL);
    snprintf(ctx->presence.asset_large_text, sizeof(ctx->presence.asset_large_text), "%.63s", CROSSTALK_NET_DISCORD_ASSET_LARGE_TEXT);
    snprintf(ctx->presence.asset_large_url, sizeof(ctx->presence.asset_large_url), "%.127s", CROSSTALK_NET_DISCORD_STORE_URL);
    snprintf(ctx->presence.asset_small_url, sizeof(ctx->presence.asset_small_url), "%.127s", CROSSTALK_NET_DISCORD_WSHOP_URL);
}

uint32_t crosstalk_net_discord_init(crosstalk_net_discord_ctx_t *ctx, uint64_t app_id, uint32_t steam_app_id)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(app_id == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    memset(ctx, CROSSTALK_NET_DISCORD_FILL, sizeof(*ctx));

    ctx->app_id = app_id;
    ctx->state = CROSSTALK_NET_DISCORD_STATE_OFF;
    ctx->auth = CROSSTALK_NET_DISCORD_AUTH_NONE;
    ctx->call_state = CROSSTALK_NET_DISCORD_CALL_OFF;
    ctx->presence_on = LDG_TRUTH_TRUE;
    ctx->voice_on = LDG_TRUTH_TRUE;
    ctx->guild_member = LDG_TRUTH_INDETERMINATE;
    ctx->guild_req = LDG_TRUTH_FALSE;
    ctx->vol_in = CROSSTALK_NET_DISCORD_VOL_DEFAULT;
    ctx->vol_out = CROSSTALK_NET_DISCORD_VOL_DEFAULT;

    Discord_Client_Init(crosstalk_net_discord_client_get(ctx));

    if (LDG_UNLIKELY(!ctx->client))
    {
        ctx->state = CROSSTALK_NET_DISCORD_STATE_FAIL;
        snprintf(ctx->err_text, sizeof(ctx->err_text), "sdk client init failed; rev: %.32s", CROSSTALK_NET_DISCORD_SDK_REV);
        return LDG_ERR_NOT_INIT;
    }

    ctx->client_live = LDG_TRUTH_TRUE;

    Discord_Client_SetApplicationId(crosstalk_net_discord_client_get(ctx), app_id);

    if (steam_app_id != 0)
    {
        uint8_t launch_ok = CROSSTALK_NET_DISCORD_SDK_BOOL(Discord_Client_RegisterLaunchSteamApplication(crosstalk_net_discord_client_get(ctx), app_id, steam_app_id));
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, launch_ok == LDG_TRUTH_TRUE ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "discord: steam launch registered; steam_app: %u32; ok: %u32", steam_app_id, (uint32_t)launch_ok);
    }

    Discord_Client_AddLogCallback(crosstalk_net_discord_client_get(ctx), (Discord_Client_LogCallback)crosstalk_net_discord_log_cb, crosstalk_net_discord_user_data_free_cb, ctx, Discord_LoggingSeverity_Verbose);
    Discord_Client_SetStatusChangedCallback(crosstalk_net_discord_client_get(ctx), (Discord_Client_OnStatusChanged)crosstalk_net_discord_status_cb, crosstalk_net_discord_user_data_free_cb, ctx);
    {
        uint64_t pid = 0;

        if (ldg_sys_pid_get(&pid) == LDG_ERR_AOK) { Discord_Client_SetGameWindowPid(crosstalk_net_discord_client_get(ctx), (int32_t)pid); }
    }

    Discord_Client_SetTokenExpirationCallback(crosstalk_net_discord_client_get(ctx), crosstalk_net_discord_tok_expired_cb, crosstalk_net_discord_user_data_free_cb, ctx);
    Discord_Client_SetActivityJoinCallback(crosstalk_net_discord_client_get(ctx), crosstalk_net_discord_activity_join_cb, crosstalk_net_discord_user_data_free_cb, ctx);
    Discord_Client_SetMessageCreatedCallback(crosstalk_net_discord_client_get(ctx), crosstalk_net_discord_msg_created_cb, crosstalk_net_discord_user_data_free_cb, ctx);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: msg callback registered");
    Discord_Client_SetActivityInviteCreatedCallback(crosstalk_net_discord_client_get(ctx), crosstalk_net_discord_invite_created_cb, crosstalk_net_discord_user_data_free_cb, ctx);
    Discord_Client_SetDeviceChangeCallback(crosstalk_net_discord_client_get(ctx), crosstalk_net_discord_dev_change_cb, crosstalk_net_discord_user_data_free_cb, ctx);
    Discord_Client_SetNoAudioInputCallback(crosstalk_net_discord_client_get(ctx), (Discord_Client_NoAudioInputCallback)crosstalk_net_discord_no_input_cb, crosstalk_net_discord_user_data_free_cb, ctx);
    Discord_Client_SetNoAudioInputThreshold(crosstalk_net_discord_client_get(ctx), (float)CROSSTALK_NET_DISCORD_NO_INPUT_DBFS);
    Discord_Client_SetEchoCancellation(crosstalk_net_discord_client_get(ctx), (int32_t)LDG_TRUTH_TRUE);
    Discord_Client_SetAutomaticGainControl(crosstalk_net_discord_client_get(ctx), (int32_t)LDG_TRUTH_TRUE);
    Discord_Client_SetNoiseSuppression(crosstalk_net_discord_client_get(ctx), (int32_t)LDG_TRUTH_TRUE);

    ctx->en = LDG_TRUTH_TRUE;
    ctx->state = CROSSTALK_NET_DISCORD_STATE_DISCONNECTED;

    return LDG_ERR_AOK;
}

void crosstalk_net_discord_shutdown(crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return; }

    if (ctx->client_live == LDG_TRUTH_FALSE) { return; }

    if (ctx->call_live == LDG_TRUTH_TRUE)
    {
        Discord_Client_EndCalls(crosstalk_net_discord_client_get(ctx), crosstalk_net_discord_end_call_cb, crosstalk_net_discord_user_data_free_cb, ctx);
        Discord_Call_Drop(crosstalk_net_discord_call_get(ctx));
        ctx->call_live = LDG_TRUTH_FALSE;
    }

    if (ctx->verifier_live == LDG_TRUTH_TRUE)
    {
        Discord_AuthorizationCodeVerifier_Drop(crosstalk_net_discord_verifier_get(ctx));
        ctx->verifier_live = LDG_TRUTH_FALSE;
    }

    crosstalk_net_discord_auth_args_drop(ctx);

    if (ctx->invite_live == LDG_TRUTH_TRUE)
    {
        Discord_ActivityInvite_Drop(crosstalk_net_discord_invite_get(ctx));
        ctx->invite = CROSSTALK_NET_DISCORD_NUL;
        ctx->invite_live = LDG_TRUTH_FALSE;
    }

    Discord_Client_ClearRichPresence(crosstalk_net_discord_client_get(ctx));
    Discord_Client_Disconnect(crosstalk_net_discord_client_get(ctx));
    Discord_RunCallbacks();
    Discord_Client_Drop(crosstalk_net_discord_client_get(ctx));

    ctx->client_live = LDG_TRUTH_FALSE;
    ctx->client = CROSSTALK_NET_DISCORD_NUL;
    ctx->state = CROSSTALK_NET_DISCORD_STATE_OFF;
    ctx->auth = CROSSTALK_NET_DISCORD_AUTH_NONE;
    ctx->call_state = CROSSTALK_NET_DISCORD_CALL_OFF;
}

void crosstalk_net_discord_poll(crosstalk_net_discord_ctx_t *ctx, double dt)
{
    if (LDG_UNLIKELY(!ctx)) { return; }

    if (ctx->client_live == LDG_TRUTH_FALSE || ctx->en == LDG_TRUTH_FALSE) { return; }

    Discord_RunCallbacks();

    if (ctx->presence_next > CROSSTALK_NET_DISCORD_TIMER_DUE) { ctx->presence_next -= dt; }

    if (ctx->call_next > CROSSTALK_NET_DISCORD_TIMER_DUE) { ctx->call_next -= dt; }

    if (ctx->auth_local_req == LDG_TRUTH_TRUE) { if ((double)ldg_time_epoch_ms_get() >= ctx->auth_deadline)
        {
            ctx->auth_local_req = LDG_TRUTH_FALSE;
            ctx->auth_fail_req = LDG_TRUTH_TRUE;
            Discord_Client_AbortAuthorize(crosstalk_net_discord_client_get(ctx));
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "discord: local authorize timed out");
        }
    }

    if (ctx->state != CROSSTALK_NET_DISCORD_STATE_RDY || ctx->user_id == CROSSTALK_NET_DISCORD_USER_ID_NONE) { ctx->contact_primed = LDG_TRUTH_FALSE; }
    else if (ctx->contact_primed == LDG_TRUTH_FALSE)
    {
        ctx->contact_primed = LDG_TRUTH_TRUE;
        ctx->contact_next = CROSSTALK_NET_DISCORD_TIMER_DUE;
    }

    if (ctx->contact_next > CROSSTALK_NET_DISCORD_TIMER_DUE) { ctx->contact_next -= dt; }
    else
    {
        crosstalk_net_discord_contacts_refresh(ctx);
        ctx->contact_next = CROSSTALK_NET_DISCORD_CONTACT_REFRESH_INTERVAL;
    }

    if (ctx->auth != CROSSTALK_NET_DISCORD_AUTH_LINKED || ctx->state != CROSSTALK_NET_DISCORD_STATE_RDY)
    {
        ctx->guild_member = LDG_TRUTH_INDETERMINATE;
        ctx->guild_next = CROSSTALK_NET_DISCORD_TIMER_DUE;
    }
    else if (ctx->guild_next > CROSSTALK_NET_DISCORD_TIMER_DUE) { ctx->guild_next -= dt; }
    else if (ctx->guild_req == LDG_TRUTH_TRUE)
    {
        ctx->guild_req = LDG_TRUTH_FALSE;
        ctx->guild_next = CROSSTALK_NET_DISCORD_GUILD_RETRY;
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "discord: guild check timed out");
    }
    else
    {
        ctx->guild_req = LDG_TRUTH_TRUE;
        ctx->guild_next = CROSSTALK_NET_DISCORD_GUILD_TIMEOUT;
        Discord_Client_GetUserGuilds(crosstalk_net_discord_client_get(ctx), crosstalk_net_discord_guilds_cb, crosstalk_net_discord_user_data_free_cb, ctx);
    }

    if (ctx->presence_on == LDG_TRUTH_FALSE) { return; }

    if (ctx->state != CROSSTALK_NET_DISCORD_STATE_RDY) { return; }

    if (ctx->presence_beat > CROSSTALK_NET_DISCORD_TIMER_DUE) { ctx->presence_beat -= dt; }
    else
    {
        ctx->presence_beat = CROSSTALK_NET_DISCORD_PRESENCE_BEAT;
        memset(&ctx->presence_sent, CROSSTALK_NET_DISCORD_FILL, sizeof(ctx->presence_sent));
    }

    if (ctx->presence_next > CROSSTALK_NET_DISCORD_TIMER_DUE) { return; }

    if (memcmp(&ctx->presence, &ctx->presence_sent, sizeof(ctx->presence)) == CROSSTALK_NET_DISCORD_MATCH) { return; }

    crosstalk_net_discord_presence_flush(ctx);
    ctx->presence_next = CROSSTALK_NET_DISCORD_PRESENCE_MIN_INTERVAL;
}

uint32_t crosstalk_net_discord_en_set(crosstalk_net_discord_ctx_t *ctx, uint8_t on)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    if (ctx->en == on) { return LDG_ERR_AOK; }

    ctx->en = on;

    if (on == LDG_TRUTH_FALSE)
    {
        crosstalk_net_discord_call_end(ctx);
        Discord_Client_ClearRichPresence(crosstalk_net_discord_client_get(ctx));
        Discord_Client_Disconnect(crosstalk_net_discord_client_get(ctx));
        ctx->state = CROSSTALK_NET_DISCORD_STATE_DISCONNECTED;
        ctx->lobby_live = LDG_TRUTH_FALSE;
        ctx->call_lobby_id = 0;
        ctx->call_req = LDG_TRUTH_FALSE;
        ctx->call_next = CROSSTALK_NET_DISCORD_TIMER_DUE;
        ctx->call_fail_cunt = 0;
        return LDG_ERR_AOK;
    }

    if (ctx->auth != CROSSTALK_NET_DISCORD_AUTH_NONE) { Discord_Client_Connect(crosstalk_net_discord_client_get(ctx)); }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_auth_link(crosstalk_net_discord_ctx_t *ctx)
{
    Discord_String scopes = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->auth == CROSSTALK_NET_DISCORD_AUTH_LINKED)) { return LDG_ERR_EXISTS; }

    if (ctx->verifier_live == LDG_TRUTH_TRUE)
    {
        Discord_AuthorizationCodeVerifier_Drop(crosstalk_net_discord_verifier_get(ctx));
        ctx->verifier_live = LDG_TRUTH_FALSE;
    }

    Discord_Client_CreateAuthorizationCodeVerifier(crosstalk_net_discord_client_get(ctx), crosstalk_net_discord_verifier_get(ctx));

    if (LDG_UNLIKELY(!ctx->verifier))
    {
        snprintf(ctx->err_text, sizeof(ctx->err_text), "code verifier create failed");
        return LDG_ERR_NOT_INIT;
    }

    ctx->verifier_live = LDG_TRUTH_TRUE;

    crosstalk_net_discord_auth_args_drop(ctx);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: pre-authorize; sdk_status: %i32; authenticated: %u32; app_id: %u64", (int32_t)Discord_Client_GetStatus(crosstalk_net_discord_client_get(ctx)), (uint32_t)CROSSTALK_NET_DISCORD_SDK_BOOL(Discord_Client_IsAuthenticated(crosstalk_net_discord_client_get(ctx))), Discord_Client_GetApplicationId(crosstalk_net_discord_client_get(ctx)));
    Discord_Client_IsDiscordAppInstalled(crosstalk_net_discord_client_get(ctx), (Discord_Client_IsDiscordAppInstalledCallback)crosstalk_net_discord_app_installed_cb, crosstalk_net_discord_user_data_free_cb, ctx);

    Discord_AuthorizationCodeVerifier_Challenge(crosstalk_net_discord_verifier_get(ctx), crosstalk_net_discord_auth_challenge_get(ctx));

    {
        Discord_String challenge = LDG_STRUCT_ZERO_INIT;
        Discord_String verifier = LDG_STRUCT_ZERO_INIT;
        char challenge_str[CROSSTALK_NET_DISCORD_TOK_CAP] = LDG_ARR_ZERO_INIT;
        char redirect_enc[CROSSTALK_NET_DISCORD_REDIRECT_CAP] = LDG_ARR_ZERO_INIT;
        char scopes_enc[CROSSTALK_NET_DISCORD_SCOPE_CAP] = LDG_ARR_ZERO_INIT;

        Discord_AuthorizationCodeVerifier_Verifier(crosstalk_net_discord_verifier_get(ctx), &verifier);
        crosstalk_net_discord_str_copy(&verifier, ctx->verifier_str, (uint32_t)sizeof(ctx->verifier_str));
        Discord_Free(verifier.ptr);

        Discord_AuthorizationCodeChallenge_Challenge(crosstalk_net_discord_auth_challenge_get(ctx), &challenge);
        crosstalk_net_discord_str_copy(&challenge, challenge_str, (uint32_t)sizeof(challenge_str));
        Discord_Free(challenge.ptr);

        crosstalk_net_discord_scopes_ld(ctx, &scopes);

        crosstalk_net_discord_url_encode(CROSSTALK_NET_DISCORD_REDIRECT_URI, redirect_enc, (uint32_t)sizeof(redirect_enc));
        crosstalk_net_discord_url_encode(ctx->scopes, scopes_enc, (uint32_t)sizeof(scopes_enc));

        snprintf(ctx->auth_url, sizeof(ctx->auth_url), "%.63s?client_id=%" PRIu64 "&response_type=code&redirect_uri=%.255s&scope=%.255s&code_challenge=%.255s&code_challenge_method=S256", CROSSTALK_NET_DISCORD_AUTH_ENDPOINT, ctx->app_id, redirect_enc, scopes_enc, challenge_str);

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: auth url built; len: %u64", (uint64_t)strlen(ctx->auth_url));
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_auth_restore(crosstalk_net_discord_ctx_t *ctx)
{
    Discord_String refresh = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->auth != CROSSTALK_NET_DISCORD_AUTH_NONE)) { return LDG_ERR_EXISTS; }

    ret = ldg_sys_secret_get(CROSSTALK_NET_DISCORD_SECRET_APP, CROSSTALK_NET_DISCORD_SECRET_KEY, ctx->refresh_tok, (uint64_t)sizeof(ctx->refresh_tok));
    if (ret != LDG_ERR_AOK || ctx->refresh_tok[0] == LDG_STR_TERM)
    {
        ctx->refresh_tok[0] = LDG_STR_TERM;
        return LDG_ERR_EMPTY;
    }

    ctx->auth = CROSSTALK_NET_DISCORD_AUTH_PENDING;
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: restoring session from stored refresh token");

    crosstalk_net_discord_str_make(ctx->refresh_tok, &refresh);
    Discord_Client_RefreshToken(crosstalk_net_discord_client_get(ctx), ctx->app_id, refresh, (Discord_Client_TokenExchangeCallback)crosstalk_net_discord_tok_cb, crosstalk_net_discord_user_data_free_cb, ctx);

    return LDG_ERR_AOK;
}

uint8_t crosstalk_net_discord_auth_fail_take(crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_TRUTH_FALSE; }

    if (ctx->auth_fail_req == LDG_TRUTH_FALSE) { return LDG_TRUTH_FALSE; }

    ctx->auth_fail_req = LDG_TRUTH_FALSE;

    return LDG_TRUTH_TRUE;
}

uint32_t crosstalk_net_discord_auth_link_local(crosstalk_net_discord_ctx_t *ctx)
{
    Discord_String scopes = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    ret = crosstalk_net_discord_auth_link(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    crosstalk_net_discord_scopes_ld(ctx, &scopes);

    Discord_AuthorizationArgs_Init(crosstalk_net_discord_auth_args_get(ctx));
    Discord_AuthorizationArgs_SetClientId(crosstalk_net_discord_auth_args_get(ctx), ctx->app_id);
    Discord_AuthorizationArgs_SetScopes(crosstalk_net_discord_auth_args_get(ctx), scopes);
    Discord_AuthorizationArgs_SetCodeChallenge(crosstalk_net_discord_auth_args_get(ctx), crosstalk_net_discord_auth_challenge_get(ctx));
    ctx->auth_args_live = LDG_TRUTH_TRUE;

    ctx->auth_local_req = LDG_TRUTH_TRUE;
    ctx->auth_deadline = (double)ldg_time_epoch_ms_get() + (CROSSTALK_NET_DISCORD_AUTH_LOCAL_TIMEOUT * (double)LDG_MS_PER_SEC);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: local authorize try; timeout: %.1f", (double)CROSSTALK_NET_DISCORD_AUTH_LOCAL_TIMEOUT);
    Discord_Client_Authorize(crosstalk_net_discord_client_get(ctx), crosstalk_net_discord_auth_args_get(ctx), crosstalk_net_discord_authorize_cb, crosstalk_net_discord_user_data_free_cb, ctx);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_auth_unlink(crosstalk_net_discord_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->auth != CROSSTALK_NET_DISCORD_AUTH_LINKED)) { return LDG_ERR_INVALID; }

    ret = ldg_sys_secret_set(CROSSTALK_NET_DISCORD_SECRET_APP, CROSSTALK_NET_DISCORD_SECRET_KEY, "");
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "discord: stored token cleared; err: %u32", ret);

    ldg_mem_secure_zero(ctx->refresh_tok, sizeof(ctx->refresh_tok));

    ctx->auth = CROSSTALK_NET_DISCORD_AUTH_NONE;
    ctx->state = CROSSTALK_NET_DISCORD_STATE_OFF;
    ctx->user_name[0] = LDG_STR_TERM;
    ctx->user_id = CROSSTALK_NET_DISCORD_USER_ID_NONE;
    ctx->contact_cunt = 0;
    ctx->guild_member = LDG_TRUTH_INDETERMINATE;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_net_discord_live_is(const crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->client_live == LDG_TRUTH_FALSE || ctx->en == LDG_TRUTH_FALSE) { return LDG_TRUTH_FALSE; }

    if (ctx->state == CROSSTALK_NET_DISCORD_STATE_RDY) { return LDG_TRUTH_TRUE; }

    if (ctx->state == CROSSTALK_NET_DISCORD_STATE_CONNECTING) { return LDG_TRUTH_MAYBE; }

    return LDG_TRUTH_FALSE;
}

uint8_t crosstalk_net_discord_linked_is(const crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->client_live == LDG_TRUTH_FALSE) { return LDG_TRUTH_FALSE; }

    if (ctx->auth == CROSSTALK_NET_DISCORD_AUTH_LINKED) { return LDG_TRUTH_TRUE; }

    if (ctx->auth == CROSSTALK_NET_DISCORD_AUTH_PENDING) { return LDG_TRUTH_MAYBE; }

    return LDG_TRUTH_FALSE;
}

uint8_t crosstalk_net_discord_guild_member_is(const crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->client_live == LDG_TRUTH_FALSE) { return LDG_TRUTH_INDETERMINATE; }

    return ctx->guild_member;
}

uint32_t crosstalk_net_discord_guild_refresh(crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    ctx->guild_next = CROSSTALK_NET_DISCORD_TIMER_DUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_state_get(const crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return CROSSTALK_NET_DISCORD_STATE_OFF; }

    return ctx->state;
}

uint32_t crosstalk_net_discord_auth_get(const crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return CROSSTALK_NET_DISCORD_AUTH_NONE; }

    return ctx->auth;
}

const char* crosstalk_net_discord_user_name_get(const crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return ""; }

    return ctx->user_name;
}

const char* crosstalk_net_discord_err_text_get(const crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return ""; }

    return ctx->err_text;
}

uint32_t crosstalk_net_discord_presence_on_set(crosstalk_net_discord_ctx_t *ctx, uint8_t on)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    if (ctx->presence_on == on) { return LDG_ERR_AOK; }

    ctx->presence_on = on;

    if (on == LDG_TRUTH_FALSE)
    {
        Discord_Client_ClearRichPresence(crosstalk_net_discord_client_get(ctx));
        memset(&ctx->presence_sent, CROSSTALK_NET_DISCORD_FILL, sizeof(ctx->presence_sent));
        return LDG_ERR_AOK;
    }

    ctx->presence_next = CROSSTALK_NET_DISCORD_TIMER_DUE;

    return LDG_ERR_AOK;
}

static void crosstalk_net_discord_presence_solo_stage(crosstalk_net_discord_ctx_t *ctx, const char *details)
{
    crosstalk_net_discord_presence_stage(ctx, CROSSTALK_NET_DISCORD_KIND_MENU);
    snprintf(ctx->presence.details, sizeof(ctx->presence.details), "%.100s", details);
    ctx->presence.state[0] = LDG_STR_TERM;
    ctx->presence.party_id[0] = LDG_STR_TERM;
    ctx->presence.join_secret[0] = LDG_STR_TERM;
    ctx->presence.party_cunt = CROSSTALK_NET_DISCORD_PARTY_NONE;
    ctx->presence.party_cunt_max = CROSSTALK_NET_DISCORD_PARTY_NONE;
    ctx->presence.joinable = LDG_TRUTH_FALSE;
    ctx->presence.end_epoch_s = CROSSTALK_NET_DISCORD_STAMP_NONE;
}

uint32_t crosstalk_net_discord_presence_menu_set(crosstalk_net_discord_ctx_t *ctx, const char *menu)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    crosstalk_net_discord_presence_solo_stage(ctx, CROSSTALK_NET_DISCORD_TEXT_MENU);
    snprintf(ctx->presence.state, sizeof(ctx->presence.state), "%.63s", menu != 0x0 ? menu : CROSSTALK_NET_DISCORD_TEXT_MENU_MAIN);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_presence_ld_set(crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    crosstalk_net_discord_presence_solo_stage(ctx, CROSSTALK_NET_DISCORD_TEXT_LD);
    crosstalk_net_discord_badge_set(ctx, CROSSTALK_NET_DISCORD_BADGE_BOARD, CROSSTALK_NET_DISCORD_BADGE_TEXT_BOARD);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_presence_browse_set(crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    crosstalk_net_discord_presence_solo_stage(ctx, CROSSTALK_NET_DISCORD_TEXT_BROWSE);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_presence_newproj_set(crosstalk_net_discord_ctx_t *ctx, const char *kind)
{
    char line[CROSSTALK_NET_DISCORD_DETAILS_CAP] = LDG_ARR_ZERO_INIT;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    snprintf(line, sizeof(line), CROSSTALK_NET_DISCORD_TEXT_NEWPROJ_FMT, kind != 0x0 ? kind : CROSSTALK_NET_DISCORD_TEXT_PROJ_RTL);
    crosstalk_net_discord_presence_solo_stage(ctx, line);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_presence_extras_set(crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    crosstalk_net_discord_presence_solo_stage(ctx, CROSSTALK_NET_DISCORD_TEXT_EXTRAS);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_presence_credits_set(crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    crosstalk_net_discord_presence_solo_stage(ctx, CROSSTALK_NET_DISCORD_TEXT_CREDITS);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_presence_sandbox_set(crosstalk_net_discord_ctx_t *ctx, uint32_t players, uint32_t comp_cunt, uint32_t trace_cunt)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    crosstalk_net_discord_presence_stage(ctx, CROSSTALK_NET_DISCORD_KIND_SANDBOX);

    if (players <= CROSSTALK_NET_DISCORD_SCAN_CUNT)
    {
        snprintf(ctx->presence.details, sizeof(ctx->presence.details), CROSSTALK_NET_DISCORD_TEXT_SANDBOX_SOLO);
        crosstalk_net_discord_badge_set(ctx, CROSSTALK_NET_DISCORD_BADGE_SOLO, CROSSTALK_NET_DISCORD_BADGE_TEXT_SOLO);
    }
    else
    {
        snprintf(ctx->presence.details, sizeof(ctx->presence.details), CROSSTALK_NET_DISCORD_TEXT_SANDBOX, players);
        crosstalk_net_discord_badge_set(ctx, CROSSTALK_NET_DISCORD_BADGE_COOP, CROSSTALK_NET_DISCORD_BADGE_TEXT_COOP);
    }

    snprintf(ctx->presence.state, sizeof(ctx->presence.state), "%" PRIu32 " components/%" PRIu32 " traces", comp_cunt, trace_cunt);
    ctx->presence.party_id[0] = LDG_STR_TERM;
    ctx->presence.join_secret[0] = LDG_STR_TERM;
    ctx->presence.party_cunt = 0;
    ctx->presence.party_cunt_max = 0;
    ctx->presence.joinable = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_presence_paused_set(crosstalk_net_discord_ctx_t *ctx, uint8_t paused)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->presence.paused = (paused != LDG_TRUTH_FALSE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_presence_music_set(crosstalk_net_discord_ctx_t *ctx, const char *text)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (text == 0x0 || text[0] == LDG_STR_TERM)
    {
        ctx->presence.music[0] = LDG_STR_TERM;
        return LDG_ERR_AOK;
    }

    snprintf(ctx->presence.music, sizeof(ctx->presence.music), "%.95s", text);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_presence_activity_set(crosstalk_net_discord_ctx_t *ctx, const char *text)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (text == 0x0 || text[0] == LDG_STR_TERM)
    {
        ctx->presence.activity[0] = LDG_STR_TERM;
        return LDG_ERR_AOK;
    }

    snprintf(ctx->presence.activity, sizeof(ctx->presence.activity), "%.95s", text);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_presence_lobby_set(crosstalk_net_discord_ctx_t *ctx, uint32_t players, uint32_t players_max, uint64_t lobby_id)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(players_max == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    crosstalk_net_discord_presence_stage(ctx, CROSSTALK_NET_DISCORD_KIND_LOBBY);
    crosstalk_net_discord_badge_set(ctx, CROSSTALK_NET_DISCORD_BADGE_LOBBY, CROSSTALK_NET_DISCORD_BADGE_TEXT_LOBBY);
    snprintf(ctx->presence.details, sizeof(ctx->presence.details), CROSSTALK_NET_DISCORD_TEXT_LOBBY, players, players_max);
    snprintf(ctx->presence.party_id, sizeof(ctx->presence.party_id), "%.16s%" PRIu64, CROSSTALK_NET_DISCORD_PARTY_PREFIX, lobby_id);
    snprintf(ctx->presence.join_secret, sizeof(ctx->presence.join_secret), "%.32s%" PRIu64, CROSSTALK_NET_DISCORD_SECRET_PREFIX, lobby_id);
    ctx->presence.party_cunt = players;
    ctx->presence.party_cunt_max = players_max;
    ctx->presence.joinable = (players < players_max) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_presence_clear(crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    memset(&ctx->presence, CROSSTALK_NET_DISCORD_FILL, sizeof(ctx->presence));
    ctx->presence.kind = CROSSTALK_NET_DISCORD_KIND_NONE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_join_secret_get(crosstalk_net_discord_ctx_t *ctx, uint64_t *out_lobby_id)
{
    uint64_t lobby_id = 0;
    uint32_t prefix_len = 0;

    if (LDG_UNLIKELY(!ctx || !out_lobby_id)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_lobby_id = 0;

    if (ctx->join_pending == LDG_TRUTH_FALSE) { return LDG_ERR_EMPTY; }

    ctx->join_pending = LDG_TRUTH_FALSE;
    prefix_len = (uint32_t)strlen(CROSSTALK_NET_DISCORD_SECRET_PREFIX);

    if (LDG_UNLIKELY(strncmp(ctx->join_secret_in, CROSSTALK_NET_DISCORD_SECRET_PREFIX, prefix_len) != CROSSTALK_NET_DISCORD_MATCH)) { return LDG_ERR_INVALID; }

    if (LDG_UNLIKELY(sscanf(&ctx->join_secret_in[prefix_len], "%" SCNu64, &lobby_id) != CROSSTALK_NET_DISCORD_SCAN_CUNT)) { return LDG_ERR_INVALID; }

    if (LDG_UNLIKELY(lobby_id == 0)) { return LDG_ERR_INVALID; }

    *out_lobby_id = lobby_id;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_chat_send(crosstalk_net_discord_ctx_t *ctx, const char *text)
{
    Discord_String content = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!ctx || !text)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(text[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ctx->state != CROSSTALK_NET_DISCORD_STATE_RDY)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->call_lobby_id == 0)) { return LDG_ERR_EMPTY; }

    crosstalk_net_discord_str_make(text, &content);
    Discord_Client_SendLobbyMessage(crosstalk_net_discord_client_get(ctx), ctx->call_lobby_id, content, crosstalk_net_discord_msg_sent_cb, crosstalk_net_discord_user_data_free_cb, ctx);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_chat_get(crosstalk_net_discord_ctx_t *ctx, const char **out_name, const char **out_text, const char **out_peer_name, uint64_t *out_author_id, uint64_t *out_peer_id, uint8_t *out_dm, uint8_t *out_hist)
{
    uint32_t slot = 0;

    if (LDG_UNLIKELY(!ctx || !out_name || !out_text || !out_peer_name || !out_author_id || !out_peer_id || !out_dm || !out_hist)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_name = "";
    *out_text = "";
    *out_peer_name = "";
    *out_author_id = 0;
    *out_peer_id = 0;
    *out_dm = LDG_TRUTH_FALSE;
    *out_hist = LDG_TRUTH_FALSE;

    if (ctx->msg_cunt == 0) { return LDG_ERR_EMPTY; }

    slot = ctx->msg_hd;
    ctx->msg_hd = (ctx->msg_hd + CROSSTALK_NET_DISCORD_RING_STEP) % CROSSTALK_NET_DISCORD_MSG_RING;
    ctx->msg_cunt = ctx->msg_cunt - CROSSTALK_NET_DISCORD_RING_STEP;

    *out_name = ctx->msg[slot].name;
    *out_text = ctx->msg[slot].text;
    *out_peer_name = ctx->msg[slot].peer_name;
    *out_author_id = ctx->msg[slot].author_id;
    *out_peer_id = ctx->msg[slot].peer_id;
    *out_dm = ctx->msg[slot].dm;
    *out_hist = ctx->msg[slot].hist;

    return LDG_ERR_AOK;
}

uint64_t crosstalk_net_discord_user_id_get(const crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return CROSSTALK_NET_DISCORD_USER_ID_NONE; }

    if (ctx->state != CROSSTALK_NET_DISCORD_STATE_RDY) { return CROSSTALK_NET_DISCORD_USER_ID_NONE; }

    return ctx->user_id;
}

uint32_t crosstalk_net_discord_dm_history_req(crosstalk_net_discord_ctx_t *ctx, uint64_t user_id)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->state != CROSSTALK_NET_DISCORD_STATE_RDY)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(user_id == 0)) { return LDG_ERR_EMPTY; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: dm history req; user: %u64; limit: %u32", user_id, (uint32_t)CROSSTALK_NET_DISCORD_HISTORY_LIMIT);
    Discord_Client_GetUserMessagesWithLimit(crosstalk_net_discord_client_get(ctx), user_id, (int32_t)CROSSTALK_NET_DISCORD_HISTORY_LIMIT, (Discord_Client_UserMessagesWithLimitCallback)crosstalk_net_discord_history_cb, crosstalk_net_discord_user_data_free_cb, ctx);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_lobby_history_req(crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->state != CROSSTALK_NET_DISCORD_STATE_RDY)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->lobby_live == LDG_TRUTH_FALSE)) { return LDG_ERR_EMPTY; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: lobby history req; lobby: %u64; limit: %u32", ctx->call_lobby_id, (uint32_t)CROSSTALK_NET_DISCORD_HISTORY_LIMIT);
    Discord_Client_GetLobbyMessagesWithLimit(crosstalk_net_discord_client_get(ctx), ctx->call_lobby_id, (int32_t)CROSSTALK_NET_DISCORD_HISTORY_LIMIT, (Discord_Client_GetLobbyMessagesCallback)crosstalk_net_discord_history_cb, crosstalk_net_discord_user_data_free_cb, ctx);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_contact_cunt_get(const crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return CROSSTALK_NET_DISCORD_CUNT_NONE; }

    return ctx->contact_cunt;
}

const crosstalk_net_discord_contact_t* crosstalk_net_discord_contact_get(const crosstalk_net_discord_ctx_t *ctx, uint32_t idx)
{
    if (LDG_UNLIKELY(!ctx)) { return 0x0; }

    if (LDG_UNLIKELY(idx >= ctx->contact_cunt)) { return 0x0; }

    return &ctx->contact[idx];
}

const char* crosstalk_net_discord_status_name_get(uint32_t status)
{
    if (status == (uint32_t)CROSSTALK_NET_DISCORD_STATUS_ONLINE) { return "online"; }

    if (status == (uint32_t)CROSSTALK_NET_DISCORD_STATUS_OFFLINE) { return "offline"; }

    if (status == (uint32_t)CROSSTALK_NET_DISCORD_STATUS_BLK) { return "blocked"; }

    if (status == (uint32_t)CROSSTALK_NET_DISCORD_STATUS_IDLE) { return "idle"; }

    if (status == (uint32_t)CROSSTALK_NET_DISCORD_STATUS_DND) { return "do not disturb"; }

    if (status == (uint32_t)CROSSTALK_NET_DISCORD_STATUS_INVISIBLE) { return "invisible"; }

    if (status == (uint32_t)CROSSTALK_NET_DISCORD_STATUS_STREAMING) { return "streaming"; }

    return "unknown";
}

const char* crosstalk_net_discord_activity_verb_get(uint8_t activity)
{
    if (activity == (uint8_t)CROSSTALK_NET_DISCORD_ACTIVITY_PLAYING) { return "Playing"; }

    if (activity == (uint8_t)CROSSTALK_NET_DISCORD_ACTIVITY_STREAMING) { return "Streaming"; }

    if (activity == (uint8_t)CROSSTALK_NET_DISCORD_ACTIVITY_LISTENING) { return "Listening to"; }

    if (activity == (uint8_t)CROSSTALK_NET_DISCORD_ACTIVITY_WATCHING) { return "Watching"; }

    if (activity == (uint8_t)CROSSTALK_NET_DISCORD_ACTIVITY_CUSTOM) { return ""; }

    if (activity == (uint8_t)CROSSTALK_NET_DISCORD_ACTIVITY_COMPETING) { return "Competing in"; }

    if (activity == (uint8_t)CROSSTALK_NET_DISCORD_ACTIVITY_HANG) { return "Hanging out"; }

    return "";
}

const char* crosstalk_net_discord_kind_name_get(uint8_t kind)
{
    if (kind == (uint8_t)CROSSTALK_NET_DISCORD_CONTACT_KIND_FRIEND) { return "friend"; }

    if (kind == (uint8_t)CROSSTALK_NET_DISCORD_CONTACT_KIND_PENDING_IN) { return "request received"; }

    if (kind == (uint8_t)CROSSTALK_NET_DISCORD_CONTACT_KIND_PENDING_OUT) { return "request sent"; }

    if (kind == (uint8_t)CROSSTALK_NET_DISCORD_CONTACT_KIND_DM) { return "conversation"; }

    if (kind == (uint8_t)CROSSTALK_NET_DISCORD_CONTACT_KIND_LOBBY) { return "in your session"; }

    return "known";
}

uint32_t crosstalk_net_discord_dm_send(crosstalk_net_discord_ctx_t *ctx, uint64_t user_id, const char *text)
{
    Discord_String content = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!ctx || !text)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(text[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ctx->state != CROSSTALK_NET_DISCORD_STATE_RDY)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(user_id == 0)) { return LDG_ERR_EMPTY; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: dm send req; to: %u64; len: %u64", user_id, (uint64_t)strlen(text));

    crosstalk_net_discord_str_make(text, &content);
    Discord_Client_SendUserMessage(crosstalk_net_discord_client_get(ctx), user_id, content, crosstalk_net_discord_msg_sent_cb, crosstalk_net_discord_user_data_free_cb, ctx);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_invite_send(crosstalk_net_discord_ctx_t *ctx, uint64_t user_id, const char *text)
{
    Discord_String content = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!ctx || !text)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->state != CROSSTALK_NET_DISCORD_STATE_RDY)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(user_id == 0)) { return LDG_ERR_EMPTY; }

    if (LDG_UNLIKELY(ctx->presence.joinable != LDG_TRUTH_TRUE)) { return LDG_ERR_INVALID; }

    crosstalk_net_discord_str_make(text, &content);
    Discord_Client_SendActivityInvite(crosstalk_net_discord_client_get(ctx), user_id, content, crosstalk_net_discord_invite_sent_cb, crosstalk_net_discord_user_data_free_cb, ctx);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_invite_pending_get(const crosstalk_net_discord_ctx_t *ctx, const char **out_name, uint64_t *out_sender_id)
{
    if (LDG_UNLIKELY(!ctx || !out_name || !out_sender_id)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_name = "";
    *out_sender_id = 0;

    if (ctx->invite_live != LDG_TRUTH_TRUE) { return LDG_ERR_EMPTY; }

    *out_name = ctx->invite_name;
    *out_sender_id = ctx->invite_sender_id;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_invite_accept(crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    if (ctx->invite_live != LDG_TRUTH_TRUE) { return LDG_ERR_EMPTY; }

    Discord_Client_AcceptActivityInvite(crosstalk_net_discord_client_get(ctx), crosstalk_net_discord_invite_get(ctx), crosstalk_net_discord_invite_accept_cb, crosstalk_net_discord_user_data_free_cb, ctx);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_invite_drop(crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->invite_live != LDG_TRUTH_TRUE) { return LDG_ERR_EMPTY; }

    Discord_ActivityInvite_Drop(crosstalk_net_discord_invite_get(ctx));
    ctx->invite = CROSSTALK_NET_DISCORD_NUL;
    ctx->invite_live = LDG_TRUTH_FALSE;
    ctx->invite_sender_id = 0;
    ctx->invite_name[0] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_lobby_join(crosstalk_net_discord_ctx_t *ctx, uint64_t lobby_id)
{
    char secret[CROSSTALK_NET_DISCORD_SECRET_CAP] = LDG_ARR_ZERO_INIT;
    Discord_String text = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(lobby_id == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->en == LDG_TRUTH_FALSE) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(ctx->state != CROSSTALK_NET_DISCORD_STATE_RDY)) { return LDG_ERR_NOT_INIT; }

    if (ctx->lobby_live == LDG_TRUTH_TRUE && ctx->call_lobby_id == lobby_id) { return LDG_ERR_AOK; }

    if (ctx->lobby_live == LDG_TRUTH_TRUE) { return LDG_ERR_BUSY; }

    if (ctx->call_req == LDG_TRUTH_TRUE) { return LDG_ERR_BUSY; }

    if (ctx->call_next > CROSSTALK_NET_DISCORD_TIMER_DUE) { return LDG_ERR_AGAIN; }

    ctx->call_req = LDG_TRUTH_TRUE;
    ctx->call_state = ctx->voice_on == LDG_TRUTH_TRUE ? CROSSTALK_NET_DISCORD_CALL_CONNECTING : CROSSTALK_NET_DISCORD_CALL_OFF;
    snprintf(secret, sizeof(secret), "%.32s%" PRIu64, CROSSTALK_NET_DISCORD_VOICE_SECRET_PREFIX, lobby_id);

    crosstalk_net_discord_str_make(secret, &text);
    Discord_Client_CreateOrJoinLobby(crosstalk_net_discord_client_get(ctx), text, crosstalk_net_discord_voice_lobby_cb, crosstalk_net_discord_user_data_free_cb, ctx);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_voice_on_set(crosstalk_net_discord_ctx_t *ctx, uint8_t on)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    if (ctx->voice_on == on) { return LDG_ERR_AOK; }

    ctx->voice_on = on;

    if (on == LDG_TRUTH_FALSE) { crosstalk_net_discord_call_end(ctx); }
    else { crosstalk_net_discord_call_start(ctx); }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_lobby_leave(crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    ctx->call_req = LDG_TRUTH_FALSE;
    ctx->call_next = CROSSTALK_NET_DISCORD_TIMER_DUE;
    ctx->call_fail_cunt = 0;

    if (ctx->lobby_live == LDG_TRUTH_FALSE) { return LDG_ERR_AOK; }

    crosstalk_net_discord_call_end(ctx);
    Discord_Client_LeaveLobby(crosstalk_net_discord_client_get(ctx), ctx->call_lobby_id, crosstalk_net_discord_lobby_leave_cb, crosstalk_net_discord_user_data_free_cb, ctx);

    ctx->lobby_live = LDG_TRUTH_FALSE;
    ctx->call_lobby_id = 0;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_voice_state_get(const crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return CROSSTALK_NET_DISCORD_CALL_OFF; }

    return ctx->call_state;
}

uint32_t crosstalk_net_discord_voice_self_mute_set(crosstalk_net_discord_ctx_t *ctx, uint8_t mute)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    ctx->self_mute = (mute != LDG_TRUTH_FALSE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    Discord_Client_SetSelfMuteAll(crosstalk_net_discord_client_get(ctx), (int32_t)ctx->self_mute);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_voice_self_deaf_set(crosstalk_net_discord_ctx_t *ctx, uint8_t deaf)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    ctx->self_deaf = (deaf != LDG_TRUTH_FALSE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    Discord_Client_SetSelfDeafAll(crosstalk_net_discord_client_get(ctx), (int32_t)ctx->self_deaf);

    return LDG_ERR_AOK;
}

uint8_t crosstalk_net_discord_voice_self_mute_is(const crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    return (ctx->self_mute == LDG_TRUTH_TRUE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint8_t crosstalk_net_discord_voice_self_deaf_is(const crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    return (ctx->self_deaf == LDG_TRUTH_TRUE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_net_discord_voice_dev_refresh(crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    Discord_Client_GetInputDevices(crosstalk_net_discord_client_get(ctx), crosstalk_net_discord_dev_in_cb, crosstalk_net_discord_user_data_free_cb, ctx);
    Discord_Client_GetOutputDevices(crosstalk_net_discord_client_get(ctx), crosstalk_net_discord_dev_out_cb, crosstalk_net_discord_user_data_free_cb, ctx);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_voice_dev_in_cunt_get(const crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return CROSSTALK_NET_DISCORD_CUNT_NONE; }

    return ctx->dev_in_cunt;
}

uint32_t crosstalk_net_discord_voice_dev_out_cunt_get(const crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return CROSSTALK_NET_DISCORD_CUNT_NONE; }

    return ctx->dev_out_cunt;
}

const crosstalk_net_discord_dev_t* crosstalk_net_discord_voice_dev_in_get(const crosstalk_net_discord_ctx_t *ctx, uint32_t idx)
{
    if (LDG_UNLIKELY(!ctx)) { return CROSSTALK_NET_DISCORD_NUL; }

    if (LDG_UNLIKELY(idx >= ctx->dev_in_cunt)) { return CROSSTALK_NET_DISCORD_NUL; }

    return &ctx->dev_in[idx];
}

const crosstalk_net_discord_dev_t* crosstalk_net_discord_voice_dev_out_get(const crosstalk_net_discord_ctx_t *ctx, uint32_t idx)
{
    if (LDG_UNLIKELY(!ctx)) { return CROSSTALK_NET_DISCORD_NUL; }

    if (LDG_UNLIKELY(idx >= ctx->dev_out_cunt)) { return CROSSTALK_NET_DISCORD_NUL; }

    return &ctx->dev_out[idx];
}

uint32_t crosstalk_net_discord_voice_dev_in_set(crosstalk_net_discord_ctx_t *ctx, const char *dev_id)
{
    Discord_String text = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!ctx || !dev_id)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY((uint64_t)strlen(dev_id) >= (uint64_t)sizeof(ctx->dev_in_sel))) { return LDG_ERR_OVERFLOW; }

    snprintf(ctx->dev_in_sel, sizeof(ctx->dev_in_sel), "%.159s", dev_id);

    crosstalk_net_discord_str_make(ctx->dev_in_sel, &text);
    Discord_Client_SetInputDevice(crosstalk_net_discord_client_get(ctx), text, crosstalk_net_discord_dev_set_cb, crosstalk_net_discord_user_data_free_cb, ctx);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_voice_dev_out_set(crosstalk_net_discord_ctx_t *ctx, const char *dev_id)
{
    Discord_String text = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!ctx || !dev_id)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY((uint64_t)strlen(dev_id) >= (uint64_t)sizeof(ctx->dev_out_sel))) { return LDG_ERR_OVERFLOW; }

    snprintf(ctx->dev_out_sel, sizeof(ctx->dev_out_sel), "%.159s", dev_id);

    crosstalk_net_discord_str_make(ctx->dev_out_sel, &text);
    Discord_Client_SetOutputDevice(crosstalk_net_discord_client_get(ctx), text, crosstalk_net_discord_dev_set_cb, crosstalk_net_discord_user_data_free_cb, ctx);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_voice_vol_in_set(crosstalk_net_discord_ctx_t *ctx, double vol)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(vol < CROSSTALK_NET_DISCORD_VOL_MIN || vol > CROSSTALK_NET_DISCORD_VOL_MAX)) { return LDG_ERR_RANGE; }

    ctx->vol_in = vol;
    Discord_Client_SetInputVolume(crosstalk_net_discord_client_get(ctx), (float)vol);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_discord_voice_vol_out_set(crosstalk_net_discord_ctx_t *ctx, double vol)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->client_live == LDG_TRUTH_FALSE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(vol < CROSSTALK_NET_DISCORD_VOL_MIN || vol > CROSSTALK_NET_DISCORD_VOL_MAX)) { return LDG_ERR_RANGE; }

    ctx->vol_out = vol;
    Discord_Client_SetOutputVolume(crosstalk_net_discord_client_get(ctx), (float)vol);

    return LDG_ERR_AOK;
}

double crosstalk_net_discord_voice_vol_in_get(const crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return CROSSTALK_NET_DISCORD_VOL_MUTE; }

    return ctx->vol_in;
}

double crosstalk_net_discord_voice_vol_out_get(const crosstalk_net_discord_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return CROSSTALK_NET_DISCORD_VOL_MUTE; }

    return ctx->vol_out;
}
