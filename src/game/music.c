#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/arch/amd64/atomic.h>
#include <dangling/audio/audio.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/path.h>
#include <dangling/net/steamworks.h>
#include <dangling/sys/info.h>
#include <dangling/str/str.h>

#include <dangling/logging/logging.h>

#include "core/log.h"
#include "game/hub.h"
#include "game/music.h"
#include "ui/cell.h"
#include "ui/theme.h"

#define CROSSTALK_GAME_MUSIC_STEP 1
#define CROSSTALK_GAME_MUSIC_BG_OURS 0x5000FFFFU
#define CROSSTALK_GAME_MUSIC_BG_ELSE 0x6B54C4FFU
#define CROSSTALK_GAME_MUSIC_GAP 1
#define CROSSTALK_GAME_MUSIC_ELLIPSIS_LEN 3
#define CROSSTALK_GAME_MUSIC_TRACK_BASE 1
#define CROSSTALK_GAME_MUSIC_PREV_TEXT "|<"
#define CROSSTALK_GAME_MUSIC_NEXT_TEXT ">|"
#define CROSSTALK_GAME_MUSIC_PLAY_TEXT ">"
#define CROSSTALK_GAME_MUSIC_PAUSE_TEXT "||"
#define CROSSTALK_GAME_MUSIC_NONE_TEXT "---"
#define CROSSTALK_GAME_MUSIC_LINE_CAP 128
#define CROSSTALK_GAME_MUSIC_ELLIPSIS_TEXT "..."
#define CROSSTALK_GAME_MUSIC_FIT_FMT "%.*s%.3s"

uint32_t crosstalk_game_music_init(crosstalk_game_music_ctx_t *ctx)
{
    crosstalk_game_music_ctx_t empty = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *ctx = empty;
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
    ctx->poll_next = 0.0;

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
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "music: theme loop set failed; err: %u32", ret); }

    ret = ldg_audio_play_vol_set(vol);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "music: theme vol set before start failed; vol: %.3f; err: %u32", vol, ret); }

    ctx->kind = (uint8_t)CROSSTALK_GAME_MUSIC_KIND_THEME;
    ctx->poll_next = 0.0;

    ret = ldg_audio_play_start();
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_audio_play_vol_set(vol);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "music: theme vol set after start failed; vol: %.3f; err: %u32", vol, ret); }

    ret = ldg_audio_play_vol_get(&vol_now);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "music: theme vol; want: %.3f; got: %.3f; err: %u32", vol, vol_now, ret);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_music_theme_stop(crosstalk_game_music_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->kind != (uint8_t)CROSSTALK_GAME_MUSIC_KIND_THEME) { return LDG_ERR_AOK; }

    if (ldg_audio_play_loop_set(LDG_TRUTH_FALSE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "theme loop clear failed"); }

    ret = crosstalk_game_music_stop(ctx);

    if (ldg_audio_play_vol_set(CROSSTALK_GAME_MUSIC_VOL_FULL) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "theme volume restore failed"); }

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
    ctx->poll_next = 0.0;

    return ldg_audio_play_start();
}

uint32_t crosstalk_game_music_src_set(crosstalk_game_music_ctx_t *ctx, uint8_t src)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(src >= (uint8_t)CROSSTALK_GAME_MUSIC_SRC_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (src == ctx->src) { return LDG_ERR_AOK; }

    if (ctx->src == (uint8_t)CROSSTALK_GAME_MUSIC_SRC_NATIVE) { if (LDG_UNLIKELY(crosstalk_game_music_stop(ctx) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "music native stop on src change failed"); } }

    ctx->src = src;
    ctx->title[0] = LDG_STR_TERM;
    ctx->artist[0] = LDG_STR_TERM;
    ctx->state = (uint8_t)CROSSTALK_GAME_MUSIC_STATE_IDLE;
    ctx->track_idx = 0;
    ctx->track_cunt = 0;
    ctx->poll_next = 0.0;

    return LDG_ERR_AOK;
}

static void crosstalk_game_music_steam_poll(crosstalk_game_music_ctx_t *ctx)
{
    ldg_net_steamworks_music_status_t status = LDG_NET_STEAMWORKS_MUSIC_STATUS_UNDEFINED;

    ctx->back.avail = ldg_net_steamworks_music_en_is();

    if (ctx->back.avail != LDG_TRUTH_TRUE)
    {
        ctx->back.state = (uint8_t)CROSSTALK_GAME_MUSIC_STATE_IDLE;
        return;
    }

    status = ldg_net_steamworks_music_status_get();

    if (status == LDG_NET_STEAMWORKS_MUSIC_STATUS_PLAYING) { ctx->back.state = (uint8_t)CROSSTALK_GAME_MUSIC_STATE_PLAYING; }
    else if (status == LDG_NET_STEAMWORKS_MUSIC_STATUS_PAUSED) { ctx->back.state = (uint8_t)CROSSTALK_GAME_MUSIC_STATE_PAUSED; }
    else { ctx->back.state = (uint8_t)CROSSTALK_GAME_MUSIC_STATE_IDLE; }
}

static void crosstalk_game_music_sys_poll(crosstalk_game_music_ctx_t *ctx)
{
    ldg_sys_media_info_t info = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    ret = ldg_sys_media_info_get(&info);

    ctx->back.avail = (ret == LDG_ERR_UNSUPPORTED) ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE;
    if (ret != LDG_ERR_AOK) { return; }

    if (info.status == LDG_SYS_MEDIA_STATUS_PLAYING) { ctx->back.state = (uint8_t)CROSSTALK_GAME_MUSIC_STATE_PLAYING; }
    else if (info.status == LDG_SYS_MEDIA_STATUS_PAUSED) { ctx->back.state = (uint8_t)CROSSTALK_GAME_MUSIC_STATE_PAUSED; }

    if (info.title[0] != LDG_STR_TERM)
    {
        ret = ldg_str_rbrcpy(ctx->back.title, info.title, (uint64_t)sizeof(ctx->back.title));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { ctx->back.title[0] = LDG_STR_TERM; }
    }

    if (info.artist[0] != LDG_STR_TERM)
    {
        ret = ldg_str_rbrcpy(ctx->back.artist, info.artist, (uint64_t)sizeof(ctx->back.artist));
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

    return (ctx->poll_next > 0.0) ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE;
}

uint32_t crosstalk_game_music_tick(crosstalk_game_music_ctx_t *ctx, double dt)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->poll_next > 0.0) { ctx->poll_next = ctx->poll_next - dt; }

    if (ctx->inflight == LDG_TRUTH_FALSE) { return LDG_ERR_AOK; }

    if (LDG_LD_ACQUIRE(ctx->done) == 0) { return LDG_ERR_AOK; }

    ctx->state = ctx->back.state;
    ctx->avail = ctx->back.avail;
    ctx->track_idx = ctx->back.track_idx;
    ctx->track_cunt = ctx->back.track_cunt;
    memcpy(ctx->title, ctx->back.title, sizeof(ctx->title));
    memcpy(ctx->artist, ctx->back.artist, sizeof(ctx->artist));

    ctx->inflight = LDG_TRUTH_FALSE;
    ctx->poll_next = CROSSTALK_GAME_MUSIC_POLL_INTERVAL;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_music_now_playing_get(const crosstalk_game_music_ctx_t *ctx, char *out, uint32_t out_cap)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(out_cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;

    if (ctx->state != (uint8_t)CROSSTALK_GAME_MUSIC_STATE_PLAYING) { return LDG_ERR_NOT_FOUND; }

    if (ctx->title[0] == LDG_STR_TERM) { return LDG_ERR_NOT_FOUND; }

    if (ctx->artist[0] != LDG_STR_TERM) { snprintf(out, (uint64_t)out_cap, "%.47s: %.47s", ctx->artist, ctx->title); }
    else { snprintf(out, (uint64_t)out_cap, "%.95s", ctx->title); }

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
    else if (ctx->src == (uint8_t)CROSSTALK_GAME_MUSIC_SRC_SYS) { if (prev_pressed != 0) { ret = ldg_sys_media_prev_skip(); }
        else if (next_pressed != 0) { ret = ldg_sys_media_next_skip(); }
        else if (ctx->state == (uint8_t)CROSSTALK_GAME_MUSIC_STATE_PLAYING) { ret = ldg_sys_media_pause(); }
        else { ret = ldg_sys_media_play(); } }
    else { if (prev_pressed != 0) { ret = ldg_net_steamworks_music_prev_play(); }
        else if (next_pressed != 0) { ret = ldg_net_steamworks_music_next_play(); }
        else if (ctx->state == (uint8_t)CROSSTALK_GAME_MUSIC_STATE_PLAYING) { ret = ldg_net_steamworks_music_pause(); }
        else { ret = ldg_net_steamworks_music_play(); } }

    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "music transport failed"); }

    ctx->poll_next = 0.0;

    return ret;
}

static void music_line_fit(char *line, uint64_t cap, uint32_t max_w)
{
    char full[CROSSTALK_GAME_MUSIC_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t len = (uint32_t)strnlen(line, cap);

    if (len <= max_w) { return; }

    if (max_w <= (uint32_t)CROSSTALK_GAME_MUSIC_ELLIPSIS_LEN || ldg_str_rbrcpy(full, line, (uint64_t)sizeof(full)) != LDG_ERR_AOK)
    {
        line[max_w < (uint32_t)cap ? max_w : (uint32_t)cap - (uint32_t)LDG_STR_TERM_SIZE] = LDG_STR_TERM;
        return;
    }

    snprintf(line, cap, CROSSTALK_GAME_MUSIC_FIT_FMT, (int32_t)(max_w - (uint32_t)CROSSTALK_GAME_MUSIC_ELLIPSIS_LEN), full, CROSSTALK_GAME_MUSIC_ELLIPSIS_TEXT);
}

uint32_t crosstalk_game_music_draw(const crosstalk_game_music_ctx_t *ctx, ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t col_cap)
{
    char line[CROSSTALK_GAME_MUSIC_LINE_CAP] = LDG_ARR_ZERO_INIT;
    const char *ctrl[] = { CROSSTALK_GAME_MUSIC_PREV_TEXT, CROSSTALK_GAME_MUSIC_PLAY_TEXT, CROSSTALK_GAME_MUSIC_NEXT_TEXT };
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t bg = LDG_STRUCT_ZERO_INIT;
    uint32_t pos = col;
    uint32_t seg_w = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(col_cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->title[0] == LDG_STR_TERM) { snprintf(line, sizeof(line), "%.15s", CROSSTALK_GAME_MUSIC_NONE_TEXT); }
    else if (ctx->artist[0] != LDG_STR_TERM) { snprintf(line, sizeof(line), "%.47s: %.47s", ctx->artist, ctx->title); }
    else if (ctx->track_cunt != 0) { snprintf(line, sizeof(line), "%.95s (%u/%u)", ctx->title, ctx->track_idx + CROSSTALK_GAME_MUSIC_TRACK_BASE, ctx->track_cunt); }
    else { snprintf(line, sizeof(line), "%.95s", ctx->title); }

    if (ctx->state == (uint8_t)CROSSTALK_GAME_MUSIC_STATE_PLAYING) { ctrl[CROSSTALK_GAME_MUSIC_ACTION_TOGGLE] = CROSSTALK_GAME_MUSIC_PAUSE_TEXT; }

    for (at = 0; at < (uint32_t)(sizeof(ctrl) / sizeof(ctrl[0])); at = at + (uint32_t)CROSSTALK_GAME_MUSIC_STEP)
    {
        ret = crosstalk_ui_cell_rgba_put(render, pos, row, ctrl[at], (uint64_t)CROSSTALK_GAME_MUSIC_LINE_CAP, (uint32_t)CROSSTALK_UI_THEME_STATUS_RGBA, &seg_w);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        pos = pos + seg_w + (uint32_t)CROSSTALK_GAME_MUSIC_GAP;
    }

    if (pos >= col + col_cap) { return LDG_ERR_AOK; }

    music_line_fit(line, (uint64_t)sizeof(line), col + col_cap - pos);

    ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_GAME_HUB_COL_ONLINE, &fg);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get(ctx->src == (uint8_t)CROSSTALK_GAME_MUSIC_SRC_NATIVE ? (uint32_t)CROSSTALK_GAME_MUSIC_BG_OURS : (uint32_t)CROSSTALK_GAME_MUSIC_BG_ELSE, &bg); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_ui_cell_text_put(render, pos, row, line, (uint64_t)sizeof(line), &fg, &bg);
}
