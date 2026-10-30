#ifndef CROSSTALK_GAME_MUSIC_H
#define CROSSTALK_GAME_MUSIC_H

#include <stdint.h>

#include <dangling/term/render.h>

#define CROSSTALK_GAME_MUSIC_TITLE_CAP 96
#define CROSSTALK_GAME_MUSIC_ACTION_PREV 0
#define CROSSTALK_GAME_MUSIC_ACTION_TOGGLE 1
#define CROSSTALK_GAME_MUSIC_ACTION_NEXT 2
#define CROSSTALK_GAME_MUSIC_POLL_INTERVAL 0.5

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

typedef struct crosstalk_game_music_snap
{
    char title[CROSSTALK_GAME_MUSIC_TITLE_CAP];
    char artist[CROSSTALK_GAME_MUSIC_TITLE_CAP];
    uint32_t track_idx;
    uint32_t track_cunt;
    uint8_t state;
    uint8_t avail;
    uint8_t pudding[6];
} crosstalk_game_music_snap_t;

typedef struct crosstalk_game_music_ctx
{
    char title[CROSSTALK_GAME_MUSIC_TITLE_CAP];
    char artist[CROSSTALK_GAME_MUSIC_TITLE_CAP];
    crosstalk_game_music_snap_t back;
    double poll_next;
    uint32_t track_idx;
    uint32_t track_cunt;
    uint32_t done;
    uint8_t src;
    uint8_t state;
    uint8_t avail;
    uint8_t inflight;
    uint8_t kind;
    uint8_t kind_pudding[1];
    uint16_t theme_channel_cunt;
    uint16_t theme_bit_depth;
    uint8_t pudding[2];
} crosstalk_game_music_ctx_t;

uint32_t crosstalk_game_music_init(crosstalk_game_music_ctx_t *ctx);
uint32_t crosstalk_game_music_src_set(crosstalk_game_music_ctx_t *ctx, uint8_t src);
uint32_t crosstalk_game_music_tick(crosstalk_game_music_ctx_t *ctx, double dt);
uint8_t crosstalk_game_music_poll_due_is(const crosstalk_game_music_ctx_t *ctx);
uint32_t crosstalk_game_music_poll_run(crosstalk_game_music_ctx_t *ctx);
uint32_t crosstalk_game_music_draw(const crosstalk_game_music_ctx_t *ctx, ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t col_cap);
uint32_t crosstalk_game_music_transport(crosstalk_game_music_ctx_t *ctx, uint32_t action, uint8_t *out_ld_req);
uint32_t crosstalk_game_music_now_playing_get(const crosstalk_game_music_ctx_t *ctx, char *out, uint32_t out_cap);
uint8_t crosstalk_game_music_native_is(const crosstalk_game_music_ctx_t *ctx);
uint8_t crosstalk_game_music_kind_get(const crosstalk_game_music_ctx_t *ctx);
uint32_t crosstalk_game_music_file_play(crosstalk_game_music_ctx_t *ctx, const char *path, uint8_t kind);
uint32_t crosstalk_game_music_theme_play(crosstalk_game_music_ctx_t *ctx, const char *path, double vol);
uint32_t crosstalk_game_music_theme_stop(crosstalk_game_music_ctx_t *ctx);
uint32_t crosstalk_game_music_list_play(crosstalk_game_music_ctx_t *ctx, const char *const *path, uint32_t cunt, uint8_t kind);
uint32_t crosstalk_game_music_stop(crosstalk_game_music_ctx_t *ctx);

#endif
