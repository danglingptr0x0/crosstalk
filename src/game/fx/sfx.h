#ifndef CROSSTALK_GAME_FX_SFX_H
#define CROSSTALK_GAME_FX_SFX_H

#include <stdint.h>

#include <dangling/audio/mix.h>
#include <dangling/proto/cw.h>

#define CROSSTALK_GAME_FX_SFX_CW_KEY_CAP 512

typedef enum crosstalk_game_fx_sfx_kind
{
    CROSSTALK_GAME_FX_SFX_KIND_ROTARY_CW = 0,
    CROSSTALK_GAME_FX_SFX_KIND_ROTARY_CCW,
    CROSSTALK_GAME_FX_SFX_KIND_LEVER,
    CROSSTALK_GAME_FX_SFX_KIND_CHIRP,
    CROSSTALK_GAME_FX_SFX_KIND_PING
} crosstalk_game_fx_sfx_kind_t;

typedef enum crosstalk_game_fx_sfx_rotary_dir
{
    CROSSTALK_GAME_FX_SFX_ROTARY_CW = 0,
    CROSSTALK_GAME_FX_SFX_ROTARY_CCW
} crosstalk_game_fx_sfx_rotary_dir_t;

typedef uint32_t (*crosstalk_game_fx_sfx_play_cb_t)(void *user, crosstalk_game_fx_sfx_kind_t kind);

typedef struct crosstalk_game_fx_sfx_ctx
{
    ldg_audio_mix_t *mix;
    crosstalk_game_fx_sfx_play_cb_t play_cb;
    void *play_cb_user;
    ldg_proto_cw_key_t cw_keys[CROSSTALK_GAME_FX_SFX_CW_KEY_CAP];
    uint64_t cw_key_cunt;
    uint64_t cw_key_idx;
    uint64_t cw_run_frames;
    uint64_t last_play_ms;
    uint64_t last_lever_ms;
    uint64_t last_chirp_ms;
    uint64_t last_ping_ms;
    double osc_phase;
    double cw_gate;
    uint32_t clip_rotary_cw;
    uint32_t clip_rotary_ccw;
    uint32_t clip_lever;
    uint32_t clip_chirp;
    uint32_t clip_ping;
    uint32_t rng;
    uint8_t ready;
    uint8_t tone_on;
    uint8_t tone_cw;
    uint8_t cw_done;
    uint8_t pudding[4];
} crosstalk_game_fx_sfx_ctx_t;

uint32_t crosstalk_game_fx_sfx_init(crosstalk_game_fx_sfx_ctx_t *ctx);
uint32_t crosstalk_game_fx_sfx_shutdown(crosstalk_game_fx_sfx_ctx_t *ctx);
uint32_t crosstalk_game_fx_sfx_play_cb_set(crosstalk_game_fx_sfx_ctx_t *ctx, crosstalk_game_fx_sfx_play_cb_t cb, void *user);
uint32_t crosstalk_game_fx_sfx_rotary_play(crosstalk_game_fx_sfx_ctx_t *ctx, crosstalk_game_fx_sfx_rotary_dir_t dir);
uint32_t crosstalk_game_fx_sfx_rotary_step_play(crosstalk_game_fx_sfx_ctx_t *ctx, uint32_t step, uint32_t step_cunt, crosstalk_game_fx_sfx_rotary_dir_t dir);
uint32_t crosstalk_game_fx_sfx_lever_play(crosstalk_game_fx_sfx_ctx_t *ctx);
uint32_t crosstalk_game_fx_sfx_chirp_play(crosstalk_game_fx_sfx_ctx_t *ctx);
uint32_t crosstalk_game_fx_sfx_ping_play(crosstalk_game_fx_sfx_ctx_t *ctx);
uint32_t crosstalk_game_fx_sfx_music_vol_set(crosstalk_game_fx_sfx_ctx_t *ctx, double vol);
uint32_t crosstalk_game_fx_sfx_tone_start(crosstalk_game_fx_sfx_ctx_t *ctx);
uint32_t crosstalk_game_fx_sfx_cw_start(crosstalk_game_fx_sfx_ctx_t *ctx, const char *text, double probability, uint8_t *out_sent);
uint8_t crosstalk_game_fx_sfx_cw_on_is(const crosstalk_game_fx_sfx_ctx_t *ctx);
uint32_t crosstalk_game_fx_sfx_tone_stop(crosstalk_game_fx_sfx_ctx_t *ctx);
uint32_t crosstalk_game_fx_sfx_tone_tick(crosstalk_game_fx_sfx_ctx_t *ctx);

#endif
