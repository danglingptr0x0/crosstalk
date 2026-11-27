#ifndef CROSSTALK_GAME_MUSIC_H
#define CROSSTALK_GAME_MUSIC_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/sys/info.h>
#include <dangling/term/render.h>

#include <libresteam/backend/backend.h>

#include "game/settings.h"

#define CROSSTALK_GAME_MUSIC_TITLE_CAP 96
#define CROSSTALK_GAME_MUSIC_ACTION_PREV 0
#define CROSSTALK_GAME_MUSIC_ACTION_TOGGLE 1
#define CROSSTALK_GAME_MUSIC_ACTION_NEXT 2
#define CROSSTALK_GAME_MUSIC_POLL_INTERVAL ((uint64_t)500)

#define CROSSTALK_GAME_MUSIC_SRC_STEAM 0
#define CROSSTALK_GAME_MUSIC_SRC_NATIVE 1
#define CROSSTALK_GAME_MUSIC_SRC_SYS 2
#define CROSSTALK_GAME_MUSIC_SRC_CUNT 3

#define CROSSTALK_GAME_MUSIC_DONE_CLEAR 0
#define CROSSTALK_GAME_MUSIC_DONE_SET 1

#define CROSSTALK_GAME_MUSIC_STATE_IDLE 0
#define CROSSTALK_GAME_MUSIC_STATE_PLAYING 1
#define CROSSTALK_GAME_MUSIC_STATE_PAUSED 2

#define CROSSTALK_GAME_MUSIC_KIND_NONE 0
#define CROSSTALK_GAME_MUSIC_KIND_TRACKS 1
#define CROSSTALK_GAME_MUSIC_KIND_COMMENTARY 2
#define CROSSTALK_GAME_MUSIC_KIND_THEME 3

#define CROSSTALK_GAME_MUSIC_VOL_FULL 1.0
#define CROSSTALK_GAME_MUSIC_THEME_BITS 8
#define CROSSTALK_GAME_MUSIC_PCM_BITS 16
#define CROSSTALK_GAME_MUSIC_TRACK_FIRST 0
#define CROSSTALK_GAME_MUSIC_SESH_CAP 8
#define CROSSTALK_GAME_MUSIC_STAT_CAP 64
#define CROSSTALK_GAME_MUSIC_STAT_PLAY_MIN (LDG_SECS_PER_MIN * LDG_MS_PER_SEC)
#define CROSSTALK_GAME_MUSIC_STAT_HALF_DIV 2
#define CROSSTALK_GAME_MUSIC_STAT_REC_LEN 208
#define CROSSTALK_GAME_MUSIC_STAT_HDR_LEN 8
#define CROSSTALK_GAME_MUSIC_STAT_BLOB_CAP (CROSSTALK_GAME_MUSIC_STAT_HDR_LEN + CROSSTALK_GAME_MUSIC_STAT_CAP * CROSSTALK_GAME_MUSIC_STAT_REC_LEN)
#define CROSSTALK_GAME_MUSIC_STAT_PUDDING_SIZE 4
#define CROSSTALK_GAME_MUSIC_SNAP_PUDDING_SIZE 6
#define CROSSTALK_GAME_MUSIC_PUDDING_SIZE 2

typedef struct crosstalk_game_music_snap
{
    uint64_t len;
    uint64_t pos;
    char title[CROSSTALK_GAME_MUSIC_TITLE_CAP];
    char artist[CROSSTALK_GAME_MUSIC_TITLE_CAP];
    char player[CROSSTALK_GAME_SETTINGS_WHITELIST_NAME_CAP];
    uint32_t track_idx;
    uint32_t track_cunt;
    uint8_t state;
    uint8_t avail;
    uint8_t pudding[CROSSTALK_GAME_MUSIC_SNAP_PUDDING_SIZE];
} crosstalk_game_music_snap_t;

typedef struct crosstalk_game_music_stat
{
    uint64_t play_time;
    uint32_t play_cunt;
    char title[CROSSTALK_GAME_MUSIC_TITLE_CAP];
    char artist[CROSSTALK_GAME_MUSIC_TITLE_CAP];
    uint8_t pudding[CROSSTALK_GAME_MUSIC_STAT_PUDDING_SIZE];
} crosstalk_game_music_stat_t;

typedef struct crosstalk_game_music_ctx
{
    char title[CROSSTALK_GAME_MUSIC_TITLE_CAP];
    char artist[CROSSTALK_GAME_MUSIC_TITLE_CAP];
    char player[CROSSTALK_GAME_SETTINGS_WHITELIST_NAME_CAP];
    char whitelist[CROSSTALK_GAME_SETTINGS_WHITELIST_CAP][CROSSTALK_GAME_SETTINGS_WHITELIST_NAME_CAP];
    char listen_title[CROSSTALK_GAME_MUSIC_TITLE_CAP];
    char listen_artist[CROSSTALK_GAME_MUSIC_TITLE_CAP];
    uint8_t stat_blob[CROSSTALK_GAME_MUSIC_STAT_BLOB_CAP];
    crosstalk_game_music_snap_t back;
    ldg_sys_media_info_t sesh[CROSSTALK_GAME_MUSIC_SESH_CAP];
    crosstalk_game_music_stat_t stat[CROSSTALK_GAME_MUSIC_STAT_CAP];
    const libresteam_backend_set_t *set;
    uint64_t poll_next;
    uint64_t len;
    uint64_t pos;
    uint64_t listen;
    uint32_t track_idx;
    uint32_t track_cunt;
    uint32_t done;
    uint32_t whitelist_cunt;
    uint32_t stat_cunt;
    uint8_t src;
    uint8_t state;
    uint8_t avail;
    uint8_t inflight;
    uint8_t kind;
    uint8_t listen_tallied;
    uint16_t theme_channel_cunt;
    uint16_t theme_bit_depth;
    uint8_t pudding[CROSSTALK_GAME_MUSIC_PUDDING_SIZE];
} crosstalk_game_music_ctx_t;

uint32_t crosstalk_game_music_init(crosstalk_game_music_ctx_t *ctx, const libresteam_backend_set_t *set);
uint32_t crosstalk_game_music_src_set(crosstalk_game_music_ctx_t *ctx, uint8_t src);
uint32_t crosstalk_game_music_tick(crosstalk_game_music_ctx_t *ctx, uint64_t dt);
uint8_t crosstalk_game_music_poll_due_is(const crosstalk_game_music_ctx_t *ctx);
uint32_t crosstalk_game_music_poll_run(crosstalk_game_music_ctx_t *ctx);
uint32_t crosstalk_game_music_width_get(const crosstalk_game_music_ctx_t *ctx, uint32_t *out);
uint32_t crosstalk_game_music_draw(const crosstalk_game_music_ctx_t *ctx, ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t col_cap, uint32_t rest_rgba);
uint32_t crosstalk_game_music_transport(crosstalk_game_music_ctx_t *ctx, uint32_t action, uint8_t *out_ld_req);
uint32_t crosstalk_game_music_now_playing_get(const crosstalk_game_music_ctx_t *ctx, char *out, uint32_t out_cap);
uint8_t crosstalk_game_music_native_is(const crosstalk_game_music_ctx_t *ctx);
uint8_t crosstalk_game_music_kind_get(const crosstalk_game_music_ctx_t *ctx);
uint32_t crosstalk_game_music_file_play(crosstalk_game_music_ctx_t *ctx, const char *path, uint8_t kind);
uint32_t crosstalk_game_music_theme_play(crosstalk_game_music_ctx_t *ctx, const char *path, double vol);
uint32_t crosstalk_game_music_theme_stop(crosstalk_game_music_ctx_t *ctx);
uint32_t crosstalk_game_music_list_play(crosstalk_game_music_ctx_t *ctx, const char *const *path, uint32_t cunt, uint8_t kind);
uint32_t crosstalk_game_music_stop(crosstalk_game_music_ctx_t *ctx);
uint32_t crosstalk_game_music_whitelist_set(crosstalk_game_music_ctx_t *ctx, const char (*list)[CROSSTALK_GAME_SETTINGS_WHITELIST_NAME_CAP], uint32_t cap);
uint8_t crosstalk_game_music_whitelist_has(const char (*list)[CROSSTALK_GAME_SETTINGS_WHITELIST_NAME_CAP], uint32_t cunt, const char *player);
uint32_t crosstalk_game_music_stat_tick(crosstalk_game_music_ctx_t *ctx, uint64_t dt, uint8_t in_proj);
uint32_t crosstalk_game_music_stat_serialize(crosstalk_game_music_ctx_t *ctx, const uint8_t **out, uint32_t *out_len);
uint32_t crosstalk_game_music_stat_deserialize(crosstalk_game_music_ctx_t *ctx, const uint8_t *buff, uint32_t len);

#endif
