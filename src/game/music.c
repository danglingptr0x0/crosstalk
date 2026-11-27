#include <stdint.h>
#include <string.h>

#include <dangling/arch/amd64/atomic.h>
#include <dangling/audio/audio.h>
#include <dangling/core/arith.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/path.h>
#include <dangling/mem/secure.h>
#include <dangling/sys/info.h>
#include <dangling/str/fmt.h>
#include <dangling/str/str.h>

#include <dangling/logging/logging.h>

#include <libresteam/music/music.h>

#include "core/log.h"
#include "game/hub.h"
#include "game/music.h"
#include "ui/cell.h"
#include "ui/theme.h"

#define CROSSTALK_GAME_MUSIC_ELLIPSIS_LEN 3
#define CROSSTALK_GAME_MUSIC_TRACK_BASE 1
#define CROSSTALK_GAME_MUSIC_NONE_TEXT "---"
#define CROSSTALK_GAME_MUSIC_LINE_CAP 128
#define CROSSTALK_GAME_MUSIC_ELLIPSIS_TEXT "..."
#define CROSSTALK_GAME_MUSIC_FIT_FMT "%.124s%.3s"
#define CROSSTALK_GAME_MUSIC_FILL_TEXT_RGBA 0x000000FFU
#define CROSSTALK_GAME_MUSIC_TRACK_FMT "%.47s (%.47s)"

uint32_t crosstalk_game_music_init(crosstalk_game_music_ctx_t *ctx, const libresteam_backend_set_t *set)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_secure_zero(ctx, (uint64_t)sizeof(*ctx));
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->set = set;
    ctx->src = (uint8_t)CROSSTALK_GAME_MUSIC_SRC_NATIVE;
    ctx->state = (uint8_t)CROSSTALK_GAME_MUSIC_STATE_IDLE;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_game_music_native_is(const crosstalk_game_music_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return (uint8_t)(ctx->src == (uint8_t)CROSSTALK_GAME_MUSIC_SRC_NATIVE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE);
}

uint8_t crosstalk_game_music_kind_get(const crosstalk_game_music_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return (uint8_t)CROSSTALK_GAME_MUSIC_KIND_NONE; }

    return ctx->kind;
}

uint32_t crosstalk_game_music_stop(crosstalk_game_music_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->kind = (uint8_t)CROSSTALK_GAME_MUSIC_KIND_NONE;
    ctx->title[0] = LDG_STR_TERM;
    ctx->artist[0] = LDG_STR_TERM;
    ctx->state = (uint8_t)CROSSTALK_GAME_MUSIC_STATE_IDLE;
    ctx->track_idx = 0;
    ctx->track_cunt = 0;

    ret = ldg_audio_play_stop();
    if (ret != LDG_ERR_AOK) { return ret; }

    return ldg_audio_play_close();
}

uint32_t crosstalk_game_music_file_play(crosstalk_game_music_ctx_t *ctx, const char *path, uint8_t kind)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(path[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->src != (uint8_t)CROSSTALK_GAME_MUSIC_SRC_NATIVE) { return LDG_ERR_UNSUPPORTED; }

    ret = ldg_audio_play_close();
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_NOT_INIT)) { return ret; }

    ret = ldg_audio_play_open(path);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->kind = kind;
    ctx->poll_next = 0;

    return ldg_audio_play_start();
}

uint32_t crosstalk_game_music_theme_play(crosstalk_game_music_ctx_t *ctx, const char *path, double vol)
{
    double vol_now = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(path[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->src != (uint8_t)CROSSTALK_GAME_MUSIC_SRC_NATIVE && ctx->state == (uint8_t)CROSSTALK_GAME_MUSIC_STATE_PLAYING) { return LDG_ERR_BUSY; }

    ret = ldg_audio_play_close();
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_NOT_INIT)) { return ret; }

    ret = ldg_audio_play_open(path);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_audio_play_loop_set(LDG_TRUTH_TRUE);
    if (ret != LDG_ERR_AOK) { ldg_logging_ln_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "music: theme loop set failed; err: %u32", ret); }

    ret = ldg_audio_play_vol_set(vol);
    if (ret != LDG_ERR_AOK) { ldg_logging_ln_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "music: theme vol set before start failed; vol: %.3f; err: %u32", vol, ret); }

    ctx->kind = (uint8_t)CROSSTALK_GAME_MUSIC_KIND_THEME;
    ctx->poll_next = 0;

    ret = ldg_audio_play_start();
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_audio_play_vol_set(vol);
    if (ret != LDG_ERR_AOK) { ldg_logging_ln_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "music: theme vol set after start failed; vol: %.3f; err: %u32", vol, ret); }

    ret = ldg_audio_play_vol_get(&vol_now);

    ldg_logging_ln_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "music: theme vol; want: %.3f; got: %.3f; err: %u32", vol, vol_now, ret);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_music_theme_stop(crosstalk_game_music_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->kind != (uint8_t)CROSSTALK_GAME_MUSIC_KIND_THEME) { return LDG_ERR_AOK; }

    if (ldg_audio_play_loop_set(LDG_TRUTH_FALSE) != LDG_ERR_AOK) { ldg_logging_ln_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "theme loop clear failed"); }

    ret = crosstalk_game_music_stop(ctx);

    if (ldg_audio_play_vol_set(CROSSTALK_GAME_MUSIC_VOL_FULL) != LDG_ERR_AOK) { ldg_logging_ln_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "theme volume restore failed"); }

    return ret;
}

uint32_t crosstalk_game_music_list_play(crosstalk_game_music_ctx_t *ctx, const char *const *path, uint32_t cunt, uint8_t kind)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->src != (uint8_t)CROSSTALK_GAME_MUSIC_SRC_NATIVE) { return LDG_ERR_UNSUPPORTED; }

    ret = ldg_audio_play_close();
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_NOT_INIT)) { return ret; }

    ret = ldg_audio_play_list_open(path, cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->kind = kind;
    ctx->poll_next = 0;

    return ldg_audio_play_start();
}

uint32_t crosstalk_game_music_src_set(crosstalk_game_music_ctx_t *ctx, uint8_t src)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(src >= (uint8_t)CROSSTALK_GAME_MUSIC_SRC_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (src == ctx->src) { return LDG_ERR_AOK; }

    if (ctx->src == (uint8_t)CROSSTALK_GAME_MUSIC_SRC_NATIVE) { if (LDG_UNLIKELY(crosstalk_game_music_stop(ctx) != LDG_ERR_AOK)) { ldg_logging_ln_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "music native stop on src change failed"); } }

    ctx->src = src;
    ctx->title[0] = LDG_STR_TERM;
    ctx->artist[0] = LDG_STR_TERM;
    ctx->player[0] = LDG_STR_TERM;
    ctx->state = (uint8_t)CROSSTALK_GAME_MUSIC_STATE_IDLE;
    ctx->track_idx = 0;
    ctx->track_cunt = 0;
    ctx->poll_next = 0;

    return LDG_ERR_AOK;
}

static void crosstalk_game_music_steam_poll(crosstalk_game_music_ctx_t *ctx)
{
    libresteam_music_status_t status = LIBRESTEAM_MUSIC_STATUS_UNDEFINED;
    uint8_t avail = LDG_TRUTH_FALSE;

    ctx->back.avail = LDG_TRUTH_FALSE;

    if (ctx->set != 0x0 && libresteam_music_en_get(ctx->set, &avail) == LDG_ERR_AOK && avail != LDG_TRUTH_FALSE) { ctx->back.avail = LDG_TRUTH_TRUE; }

    if (ctx->back.avail != LDG_TRUTH_TRUE)
    {
        ctx->back.state = (uint8_t)CROSSTALK_GAME_MUSIC_STATE_IDLE;
        return;
    }

    if (libresteam_music_status_get(ctx->set, &status) != LDG_ERR_AOK) { status = LIBRESTEAM_MUSIC_STATUS_UNDEFINED; }

    if (status == LIBRESTEAM_MUSIC_STATUS_PLAYING) { ctx->back.state = (uint8_t)CROSSTALK_GAME_MUSIC_STATE_PLAYING; }
    else if (status == LIBRESTEAM_MUSIC_STATUS_PAUSED) { ctx->back.state = (uint8_t)CROSSTALK_GAME_MUSIC_STATE_PAUSED; }
    else { ctx->back.state = (uint8_t)CROSSTALK_GAME_MUSIC_STATE_IDLE; }
}

uint8_t crosstalk_game_music_whitelist_has(const char (*list)[CROSSTALK_GAME_SETTINGS_WHITELIST_NAME_CAP], uint32_t cunt, const char *player)
{
    uint32_t cmp = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(list == 0x0 || player == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (player[0] == LDG_STR_TERM) { return LDG_TRUTH_FALSE; }

    for (at = 0; at < cunt && at < (uint32_t)CROSSTALK_GAME_SETTINGS_WHITELIST_CAP; at++)
    {
        if (list[at][0] == LDG_STR_TERM) { continue; }

        if (ldg_str_case_cmp(list[at], player, (uint64_t)CROSSTALK_GAME_SETTINGS_WHITELIST_NAME_CAP, &cmp) == LDG_ERR_AOK && cmp == LDG_ARITH_ORD_EQ) { return LDG_TRUTH_TRUE; }
    }

    return LDG_TRUTH_FALSE;
}

uint32_t crosstalk_game_music_whitelist_set(crosstalk_game_music_ctx_t *ctx, const char (*list)[CROSSTALK_GAME_SETTINGS_WHITELIST_NAME_CAP], uint32_t cap)
{
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || list == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->whitelist_cunt = 0;

    for (at = 0; at < cap && at < (uint32_t)CROSSTALK_GAME_SETTINGS_WHITELIST_CAP; at++)
    {
        if (list[at][0] == LDG_STR_TERM) { continue; }

        ret = ldg_str_rbrcpy(ctx->whitelist[ctx->whitelist_cunt], list[at], (uint64_t)sizeof(ctx->whitelist[ctx->whitelist_cunt]));
        if (ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC) { return ret; }

        ctx->whitelist_cunt++;
    }

    ctx->poll_next = 0;

    return LDG_ERR_AOK;
}

static void crosstalk_game_music_sys_poll(crosstalk_game_music_ctx_t *ctx)
{
    const ldg_sys_media_info_t *info = 0x0;
    uint32_t cunt = 0;
    uint32_t at = 0;
    uint32_t pick = UINT32_MAX;
    uint32_t ret = 0;

    ret = ldg_sys_media_list(ctx->sesh, (uint32_t)CROSSTALK_GAME_MUSIC_SESH_CAP, &cunt);

    ctx->back.avail = (ret == LDG_ERR_UNSUPPORTED) ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE;
    if (ret != LDG_ERR_AOK && ret != LDG_ERR_FULL) { return; }

    if (cunt > (uint32_t)CROSSTALK_GAME_MUSIC_SESH_CAP) { cunt = (uint32_t)CROSSTALK_GAME_MUSIC_SESH_CAP; }

    for (at = 0; at < cunt; at++)
    {
        if (crosstalk_game_music_whitelist_has(ctx->whitelist, ctx->whitelist_cunt, ctx->sesh[at].player) != LDG_TRUTH_TRUE) { continue; }

        if (pick == UINT32_MAX) { pick = at; }

        if (ctx->sesh[at].status == LDG_SYS_MEDIA_STATUS_PLAYING)
        {
            pick = at;
            break;
        }
    }

    if (pick == UINT32_MAX) { return; }

    info = &ctx->sesh[pick];

    if (info->status == LDG_SYS_MEDIA_STATUS_PLAYING) { ctx->back.state = (uint8_t)CROSSTALK_GAME_MUSIC_STATE_PLAYING; }
    else if (info->status == LDG_SYS_MEDIA_STATUS_PAUSED) { ctx->back.state = (uint8_t)CROSSTALK_GAME_MUSIC_STATE_PAUSED; }

    ctx->back.len = info->len;
    ctx->back.pos = info->pos;

    ret = ldg_str_rbrcpy(ctx->back.player, info->player, (uint64_t)sizeof(ctx->back.player));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { ctx->back.player[0] = LDG_STR_TERM; }

    if (info->title[0] != LDG_STR_TERM)
    {
        ret = ldg_str_rbrcpy(ctx->back.title, info->title, (uint64_t)sizeof(ctx->back.title));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { ctx->back.title[0] = LDG_STR_TERM; }
    }

    if (info->artist[0] != LDG_STR_TERM)
    {
        ret = ldg_str_rbrcpy(ctx->back.artist, info->artist, (uint64_t)sizeof(ctx->back.artist));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { ctx->back.artist[0] = LDG_STR_TERM; }
    }
}

static void crosstalk_game_music_native_poll(crosstalk_game_music_ctx_t *ctx)
{
    ldg_audio_play_status_t status = LDG_STRUCT_ZERO_INIT;
    char base[CROSSTALK_GAME_MUSIC_TITLE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    ctx->back.avail = LDG_TRUTH_TRUE;

    ret = ldg_audio_play_status_get(&status);
    if (ret != LDG_ERR_AOK) { return; }

    ctx->back.track_idx = status.track_idx;
    ctx->back.track_cunt = status.track_cunt;

    if (status.rate != 0)
    {
        ctx->back.len = status.frame_cunt * (uint64_t)LDG_MS_PER_SEC / (uint64_t)status.rate;
        ctx->back.pos = status.frame_pos * (uint64_t)LDG_MS_PER_SEC / (uint64_t)status.rate;
    }

    if (status.state == LDG_AUDIO_PLAY_STATE_PLAYING) { ctx->back.state = (uint8_t)CROSSTALK_GAME_MUSIC_STATE_PLAYING; }
    else if (status.state == LDG_AUDIO_PLAY_STATE_PAUSED) { ctx->back.state = (uint8_t)CROSSTALK_GAME_MUSIC_STATE_PAUSED; }

    if (status.path[0] == LDG_STR_TERM) { return; }

    ret = ldg_io_path_leaf_get(status.path, base, (uint64_t)sizeof(base));
    if (ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC) { return; }

    {
        char ext[CROSSTALK_GAME_MUSIC_TITLE_CAP] = LDG_ARR_ZERO_INIT;
        uint64_t base_len = 0;
        uint64_t ext_len = 0;

        if (ldg_io_path_ext_get(base, ext, (uint64_t)sizeof(ext)) == LDG_ERR_AOK)
        {
            base_len = (uint64_t)strlen(base);
            ext_len = (uint64_t)strlen(ext);

            if (ext_len > 0 && base_len > ext_len) { base[base_len - ext_len] = LDG_STR_TERM; }

            base_len = (uint64_t)strlen(base);

            if (base_len > 0 && base[base_len - LDG_STR_TERM_SIZE] == '.') { base[base_len - LDG_STR_TERM_SIZE] = LDG_STR_TERM; }
        }
    }

    ret = ldg_str_rbrcpy(ctx->back.title, base, (uint64_t)sizeof(ctx->back.title));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { ctx->back.title[0] = LDG_STR_TERM; }
}

uint32_t crosstalk_game_music_poll_run(crosstalk_game_music_ctx_t *ctx)
{
    crosstalk_game_music_snap_t empty = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->back = empty;
    ctx->back.state = (uint8_t)CROSSTALK_GAME_MUSIC_STATE_IDLE;

    if (ctx->src == (uint8_t)CROSSTALK_GAME_MUSIC_SRC_NATIVE) { crosstalk_game_music_native_poll(ctx); }
    else if (ctx->src == (uint8_t)CROSSTALK_GAME_MUSIC_SRC_SYS) { crosstalk_game_music_sys_poll(ctx); }
    else { crosstalk_game_music_steam_poll(ctx); }

    return LDG_ERR_AOK;
}

uint8_t crosstalk_game_music_poll_due_is(const crosstalk_game_music_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->inflight != LDG_TRUTH_FALSE) { return LDG_TRUTH_FALSE; }

    return (ctx->poll_next > 0) ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE;
}

uint32_t crosstalk_game_music_tick(crosstalk_game_music_ctx_t *ctx, uint64_t dt)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->poll_next = ctx->poll_next - (ctx->poll_next < dt ? ctx->poll_next : dt);

    if (ctx->inflight == LDG_TRUTH_FALSE) { return LDG_ERR_AOK; }

    if (LDG_LD_ACQUIRE(ctx->done) == 0) { return LDG_ERR_AOK; }

    ctx->state = ctx->back.state;
    ctx->avail = ctx->back.avail;
    ctx->track_idx = ctx->back.track_idx;
    ctx->track_cunt = ctx->back.track_cunt;
    ctx->len = ctx->back.len;
    ctx->pos = ctx->back.pos;
    memcpy(&ctx->title[0], ctx->back.title, sizeof(ctx->title));
    memcpy(&ctx->artist[0], ctx->back.artist, sizeof(ctx->artist));
    memcpy(&ctx->player[0], ctx->back.player, sizeof(ctx->player));

    ctx->inflight = LDG_TRUTH_FALSE;
    ctx->poll_next = CROSSTALK_GAME_MUSIC_POLL_INTERVAL;

    return LDG_ERR_AOK;
}

static uint32_t music_stat_slot_get(crosstalk_game_music_ctx_t *ctx, uint32_t *out)
{
    uint32_t at = 0;
    uint32_t victim = 0;
    uint32_t ret = 0;

    for (at = 0; at < ctx->stat_cunt; at++) { if (strncmp(ctx->stat[at].title, ctx->listen_title, sizeof(ctx->stat[at].title)) == 0 && strncmp(ctx->stat[at].artist, ctx->listen_artist, sizeof(ctx->stat[at].artist)) == 0)
        {
            *out = at;
            return LDG_ERR_AOK;
        }
    }

    if (ctx->stat_cunt < (uint32_t)CROSSTALK_GAME_MUSIC_STAT_CAP)
    {
        victim = ctx->stat_cunt;
        ctx->stat_cunt++;
    }
    else { for (at = 0; at < ctx->stat_cunt; at++) { if (ctx->stat[at].play_cunt < ctx->stat[victim].play_cunt || (ctx->stat[at].play_cunt == ctx->stat[victim].play_cunt && ctx->stat[at].play_time < ctx->stat[victim].play_time)) { victim = at; } } }

    ret = ldg_mem_secure_zero(&ctx->stat[victim], (uint64_t)sizeof(ctx->stat[victim]));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_str_rbrcpy(ctx->stat[victim].title, ctx->listen_title, (uint64_t)sizeof(ctx->stat[victim].title));
    if (ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC) { return ret; }

    ret = ldg_str_rbrcpy(ctx->stat[victim].artist, ctx->listen_artist, (uint64_t)sizeof(ctx->stat[victim].artist));
    if (ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC) { return ret; }

    *out = victim;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_music_stat_tick(crosstalk_game_music_ctx_t *ctx, uint64_t dt, uint8_t in_proj)
{
    uint64_t need = (uint64_t)CROSSTALK_GAME_MUSIC_STAT_PLAY_MIN;
    uint32_t slot = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->src != (uint8_t)CROSSTALK_GAME_MUSIC_SRC_SYS || ctx->state != (uint8_t)CROSSTALK_GAME_MUSIC_STATE_PLAYING || in_proj != LDG_TRUTH_TRUE || ctx->title[0] == LDG_STR_TERM) { return LDG_ERR_AOK; }

    if (strncmp(ctx->title, ctx->listen_title, sizeof(ctx->title)) != 0 || strncmp(ctx->artist, ctx->listen_artist, sizeof(ctx->artist)) != 0)
    {
        memcpy(&ctx->listen_title[0], ctx->title, sizeof(ctx->listen_title));
        memcpy(&ctx->listen_artist[0], ctx->artist, sizeof(ctx->listen_artist));
        ctx->listen = 0;
        ctx->listen_tallied = LDG_TRUTH_FALSE;
    }

    ctx->listen = ctx->listen + dt;

    if (ctx->listen_tallied == LDG_TRUTH_TRUE)
    {
        ret = music_stat_slot_get(ctx, &slot);
        if (ret != LDG_ERR_AOK) { return ret; }

        ctx->stat[slot].play_time = ctx->stat[slot].play_time + dt;

        return LDG_ERR_AOK;
    }

    if (ctx->len != 0 && ctx->len / (uint64_t)CROSSTALK_GAME_MUSIC_STAT_HALF_DIV < need) { need = ctx->len / (uint64_t)CROSSTALK_GAME_MUSIC_STAT_HALF_DIV; }

    if (ctx->listen < need) { return LDG_ERR_AOK; }

    ret = music_stat_slot_get(ctx, &slot);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->listen_tallied = LDG_TRUTH_TRUE;
    ctx->stat[slot].play_cunt++;
    ctx->stat[slot].play_time = ctx->stat[slot].play_time + ctx->listen;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_music_stat_serialize(crosstalk_game_music_ctx_t *ctx, const uint8_t **out, uint32_t *out_len)
{
    uint32_t rec_len = (uint32_t)CROSSTALK_GAME_MUSIC_STAT_REC_LEN;
    uint32_t off = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;
    *out_len = 0;

    ret = ldg_mem_secure_zero(ctx->stat_blob, (uint64_t)sizeof(ctx->stat_blob));
    if (ret != LDG_ERR_AOK) { return ret; }

    memcpy(ctx->stat_blob + off, &ctx->stat_cunt, sizeof(ctx->stat_cunt));
    off = off + (uint32_t)sizeof(ctx->stat_cunt);
    memcpy(ctx->stat_blob + off, &rec_len, sizeof(rec_len));
    off = off + (uint32_t)sizeof(rec_len);

    for (at = 0; at < ctx->stat_cunt; at++)
    {
        uint32_t rec = (uint32_t)CROSSTALK_GAME_MUSIC_STAT_HDR_LEN + at * rec_len;

        memcpy(ctx->stat_blob + rec, ctx->stat[at].title, sizeof(ctx->stat[at].title));
        rec = rec + (uint32_t)sizeof(ctx->stat[at].title);
        memcpy(ctx->stat_blob + rec, ctx->stat[at].artist, sizeof(ctx->stat[at].artist));
        rec = rec + (uint32_t)sizeof(ctx->stat[at].artist);
        memcpy(ctx->stat_blob + rec, &ctx->stat[at].play_time, sizeof(ctx->stat[at].play_time));
        rec = rec + (uint32_t)sizeof(ctx->stat[at].play_time);
        memcpy(ctx->stat_blob + rec, &ctx->stat[at].play_cunt, sizeof(ctx->stat[at].play_cunt));
    }

    *out = ctx->stat_blob;
    *out_len = (uint32_t)CROSSTALK_GAME_MUSIC_STAT_HDR_LEN + ctx->stat_cunt * rec_len;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_music_stat_deserialize(crosstalk_game_music_ctx_t *ctx, const uint8_t *buff, uint32_t len)
{
    uint32_t cunt = 0;
    uint32_t rec_len = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || buff == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->stat_cunt = 0;

    if (len < (uint32_t)CROSSTALK_GAME_MUSIC_STAT_HDR_LEN) { return LDG_ERR_IO_FMT; }

    memcpy(&cunt, buff, sizeof(cunt));
    memcpy(&rec_len, buff + sizeof(cunt), sizeof(rec_len));

    if (rec_len != (uint32_t)CROSSTALK_GAME_MUSIC_STAT_REC_LEN || cunt > (uint32_t)CROSSTALK_GAME_MUSIC_STAT_CAP || len != (uint32_t)CROSSTALK_GAME_MUSIC_STAT_HDR_LEN + cunt * rec_len) { return LDG_ERR_IO_FMT; }

    for (at = 0; at < cunt; at++)
    {
        uint32_t rec = (uint32_t)CROSSTALK_GAME_MUSIC_STAT_HDR_LEN + at * rec_len;

        ret = ldg_mem_secure_zero(&ctx->stat[at], (uint64_t)sizeof(ctx->stat[at]));
        if (ret != LDG_ERR_AOK) { return ret; }

        memcpy(ctx->stat[at].title, buff + rec, sizeof(ctx->stat[at].title));
        rec = rec + (uint32_t)sizeof(ctx->stat[at].title);
        memcpy(ctx->stat[at].artist, buff + rec, sizeof(ctx->stat[at].artist));
        rec = rec + (uint32_t)sizeof(ctx->stat[at].artist);
        memcpy(&ctx->stat[at].play_time, buff + rec, sizeof(ctx->stat[at].play_time));
        rec = rec + (uint32_t)sizeof(ctx->stat[at].play_time);
        memcpy(&ctx->stat[at].play_cunt, buff + rec, sizeof(ctx->stat[at].play_cunt));

        ctx->stat[at].title[CROSSTALK_GAME_MUSIC_TITLE_CAP - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
        ctx->stat[at].artist[CROSSTALK_GAME_MUSIC_TITLE_CAP - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
    }

    ctx->stat_cunt = cunt;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_music_now_playing_get(const crosstalk_game_music_ctx_t *ctx, char *out, uint32_t out_cap)
{
    uint64_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(out_cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;

    if (ctx->state != (uint8_t)CROSSTALK_GAME_MUSIC_STATE_PLAYING) { return LDG_ERR_NOT_FOUND; }

    if (ctx->title[0] == LDG_STR_TERM) { return LDG_ERR_NOT_FOUND; }

    if (ctx->artist[0] != LDG_STR_TERM) { ret = ldg_str_fmt(out, (uint64_t)out_cap, &len, CROSSTALK_GAME_MUSIC_TRACK_FMT, ctx->title, ctx->artist); }
    else { ret = ldg_str_fmt(out, (uint64_t)out_cap, &len, "%.95s", ctx->title); }

    if (ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)
    {
        out[0] = LDG_STR_TERM;
        return ret;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_music_transport(crosstalk_game_music_ctx_t *ctx, uint32_t action, uint8_t *out_ld_req)
{
    uint8_t prev_pressed = action == (uint32_t)CROSSTALK_GAME_MUSIC_ACTION_PREV ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    uint8_t next_pressed = action == (uint32_t)CROSSTALK_GAME_MUSIC_ACTION_NEXT ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_ld_req == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_ld_req = LDG_TRUTH_FALSE;

    if (action > (uint32_t)CROSSTALK_GAME_MUSIC_ACTION_NEXT) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->src == (uint8_t)CROSSTALK_GAME_MUSIC_SRC_NATIVE && ctx->kind == (uint8_t)CROSSTALK_GAME_MUSIC_KIND_NONE && prev_pressed == 0 && next_pressed == 0)
    {
        *out_ld_req = LDG_TRUTH_TRUE;
        return LDG_ERR_AOK;
    }

    if (ctx->src == (uint8_t)CROSSTALK_GAME_MUSIC_SRC_NATIVE) { if (prev_pressed != 0) { ret = ldg_audio_play_prev(); }
        else if (next_pressed != 0) { ret = ldg_audio_play_next(); }
        else if (ctx->state == (uint8_t)CROSSTALK_GAME_MUSIC_STATE_PLAYING) { ret = ldg_audio_play_pause(); }
        else { ret = ldg_audio_play_start(); } }
    else if (ctx->src == (uint8_t)CROSSTALK_GAME_MUSIC_SRC_SYS) { if (ctx->player[0] == LDG_STR_TERM) { ret = LDG_ERR_NOT_FOUND; }
        else if (prev_pressed != 0) { ret = ldg_sys_media_player_prev_skip(ctx->player); }
        else if (next_pressed != 0) { ret = ldg_sys_media_player_next_skip(ctx->player); }
        else if (ctx->state == (uint8_t)CROSSTALK_GAME_MUSIC_STATE_PLAYING) { ret = ldg_sys_media_player_pause(ctx->player); }
        else { ret = ldg_sys_media_player_play(ctx->player); } }
    else if (ctx->set == 0x0) { ret = LDG_ERR_NOT_INIT; }
    else { if (prev_pressed != 0) { ret = libresteam_music_prev_play(ctx->set); }
        else if (next_pressed != 0) { ret = libresteam_music_next_play(ctx->set); }
        else if (ctx->state == (uint8_t)CROSSTALK_GAME_MUSIC_STATE_PLAYING) { ret = libresteam_music_pause(ctx->set); }
        else { ret = libresteam_music_play(ctx->set); } }

    if (ret != LDG_ERR_AOK) { ldg_logging_ln_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "music transport failed"); }

    ctx->poll_next = 0;

    return ret;
}

static void music_line_fit(char *line, uint64_t cap, uint32_t max_w)
{
    char full[CROSSTALK_GAME_MUSIC_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t fit_len = 0;
    uint32_t len = (uint32_t)strnlen(line, cap);

    if (len <= max_w) { return; }

    if (max_w <= (uint32_t)CROSSTALK_GAME_MUSIC_ELLIPSIS_LEN || ldg_str_rbrcpy(full, line, (uint64_t)sizeof(full)) != LDG_ERR_AOK)
    {
        line[max_w < (uint32_t)cap ? max_w : (uint32_t)cap - (uint32_t)LDG_STR_TERM_SIZE] = LDG_STR_TERM;
        return;
    }

    full[max_w - (uint32_t)CROSSTALK_GAME_MUSIC_ELLIPSIS_LEN] = LDG_STR_TERM;

    if (ldg_str_fmt(line, cap, &fit_len, CROSSTALK_GAME_MUSIC_FIT_FMT, full, CROSSTALK_GAME_MUSIC_ELLIPSIS_TEXT) != LDG_ERR_AOK) { line[0] = LDG_STR_TERM; }
}

static void music_line_compose(const crosstalk_game_music_ctx_t *ctx, char *line, uint64_t cap)
{
    uint64_t len = 0;
    uint32_t ret = 0;

    if (ctx->title[0] == LDG_STR_TERM) { ret = ldg_str_fmt(line, cap, &len, "%.15s", CROSSTALK_GAME_MUSIC_NONE_TEXT); }
    else if (ctx->artist[0] != LDG_STR_TERM) { ret = ldg_str_fmt(line, cap, &len, CROSSTALK_GAME_MUSIC_TRACK_FMT, ctx->title, ctx->artist); }
    else if (ctx->track_cunt != 0) { ret = ldg_str_fmt(line, cap, &len, "%.95s (%u32/%u32)", ctx->title, ctx->track_idx + CROSSTALK_GAME_MUSIC_TRACK_BASE, ctx->track_cunt); }
    else { ret = ldg_str_fmt(line, cap, &len, "%.95s", ctx->title); }

    if (ret != LDG_ERR_AOK) { line[0] = LDG_STR_TERM; }
}

uint32_t crosstalk_game_music_width_get(const crosstalk_game_music_ctx_t *ctx, uint32_t *out)
{
    char line[CROSSTALK_GAME_MUSIC_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    music_line_compose(ctx, line, (uint64_t)sizeof(line));

    ret = ldg_str_len_get(line, (uint64_t)sizeof(line), &len);
    if (ret != LDG_ERR_AOK) { return ret; }

    *out = (uint32_t)len;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_music_draw(const crosstalk_game_music_ctx_t *ctx, ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t col_cap, uint32_t rest_rgba)
{
    char line[CROSSTALK_GAME_MUSIC_LINE_CAP] = LDG_ARR_ZERO_INIT;
    char filled[CROSSTALK_GAME_MUSIC_LINE_CAP] = LDG_ARR_ZERO_INIT;
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t bg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t rest = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t fill_fg = LDG_STRUCT_ZERO_INIT;
    uint64_t text_len = 0;
    uint64_t fill = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(col_cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    music_line_compose(ctx, line, (uint64_t)sizeof(line));
    music_line_fit(line, (uint64_t)sizeof(line), col_cap);

    ret = ldg_str_len_get(line, (uint64_t)sizeof(line), &text_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    fill = text_len;

    if (ctx->len != 0 && ctx->pos < ctx->len) { fill = text_len * ctx->pos / ctx->len; }

    ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_GAME_HUB_COL_ONLINE, &fg);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_ACCENT_RGBA, &bg); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get(rest_rgba, &rest); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_GAME_MUSIC_FILL_TEXT_RGBA, &fill_fg); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (fill != 0)
    {
        memcpy(filled, line, (uint64_t)fill);
        filled[fill] = LDG_STR_TERM;

        ret = crosstalk_ui_cell_text_put(render, col, row, filled, (uint64_t)sizeof(filled), &fill_fg, &bg);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (fill >= text_len) { return LDG_ERR_AOK; }

    return crosstalk_ui_cell_text_put(render, col + (uint32_t)fill, row, line + fill, (uint64_t)sizeof(line) - fill, &fg, &rest);
}
