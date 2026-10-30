#include <stdint.h>
#include <math.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/math.h>
#include <dangling/mem/alloc.h>
#include <dangling/time/time.h>

#include "core/log.h"
#include "game/fx/sfx.h"

#define CROSSTALK_GAME_FX_SFX_RATE 44100
#define CROSSTALK_GAME_FX_SFX_CHANNELS 1
#define CROSSTALK_GAME_FX_SFX_MIX_RATE 48000
#define CROSSTALK_GAME_FX_SFX_MIX_CHANNELS 2
#define CROSSTALK_GAME_FX_SFX_ROTARY_FRAMES 560
#define CROSSTALK_GAME_FX_SFX_ROTARY_CW_BODY_HZ 1750.0
#define CROSSTALK_GAME_FX_SFX_ROTARY_CCW_BODY_HZ 1320.0
#define CROSSTALK_GAME_FX_SFX_ROTARY_BODY_DECAY 0.0038
#define CROSSTALK_GAME_FX_SFX_ROTARY_TICK_DECAY 0.0011
#define CROSSTALK_GAME_FX_SFX_ROTARY_TICK_MIX 0.55
#define CROSSTALK_GAME_FX_SFX_ROTARY_BODY_MIX 0.45
#define CROSSTALK_GAME_FX_SFX_ROTARY_GAIN 0.62
#define CROSSTALK_GAME_FX_SFX_ROTARY_VOL 0.45
#define CROSSTALK_GAME_FX_SFX_LEVER_VOL 0.5
#define CROSSTALK_GAME_FX_SFX_LEVER_PITCH 0.78
#define CROSSTALK_GAME_FX_SFX_LEVER_BODY_HZ 980.0
#define CROSSTALK_GAME_FX_SFX_PITCH_NATIVE 1.0
#define CROSSTALK_GAME_FX_SFX_PITCH_MIN 0.93
#define CROSSTALK_GAME_FX_SFX_PITCH_SPAN 0.14
#define CROSSTALK_GAME_FX_SFX_MIN_GAP_MS 24
#define CROSSTALK_GAME_FX_SFX_LEVER_GAP_MS 40
#define CROSSTALK_GAME_FX_SFX_STEP_LAST_OFF 1
#define CROSSTALK_GAME_FX_SFX_STEP_JITTER 0.05
#define CROSSTALK_GAME_FX_SFX_STEP_RISE 0.22
#define CROSSTALK_GAME_FX_SFX_PEAK 32767.0
#define CROSSTALK_GAME_FX_SFX_RNG_SEED 0x9E3779B9U
#define CROSSTALK_GAME_FX_SFX_RNG_SHL 13
#define CROSSTALK_GAME_FX_SFX_RNG_SHR 17
#define CROSSTALK_GAME_FX_SFX_RNG_SHL2 5
#define CROSSTALK_GAME_FX_SFX_RNG_NORM 4294967295.0
#define CROSSTALK_GAME_FX_SFX_CHIRP_FRAMES 3072
#define CROSSTALK_GAME_FX_SFX_CHIRP_HZ_LO 420.0
#define CROSSTALK_GAME_FX_SFX_CHIRP_HZ_HI 1260.0
#define CROSSTALK_GAME_FX_SFX_CHIRP_ATTACK_S 0.004
#define CROSSTALK_GAME_FX_SFX_CHIRP_DECAY 0.042
#define CROSSTALK_GAME_FX_SFX_CHIRP_GAIN 0.5
#define CROSSTALK_GAME_FX_SFX_CHIRP_VOL 0.4
#define CROSSTALK_GAME_FX_SFX_CHIRP_GAP_MS 120
#define CROSSTALK_GAME_FX_SFX_PING_FRAMES 22050
#define CROSSTALK_GAME_FX_SFX_PING_HZ 1175.0
#define CROSSTALK_GAME_FX_SFX_PING_GLIDE_HZ 90.0
#define CROSSTALK_GAME_FX_SFX_PING_ATTACK_S 0.003
#define CROSSTALK_GAME_FX_SFX_PING_DECAY 0.14
#define CROSSTALK_GAME_FX_SFX_PING_SHIMMER_RATIO 2.01
#define CROSSTALK_GAME_FX_SFX_PING_SHIMMER_MIX 0.22
#define CROSSTALK_GAME_FX_SFX_PING_SHIMMER_DECAY 0.05
#define CROSSTALK_GAME_FX_SFX_PING_GAIN 0.55
#define CROSSTALK_GAME_FX_SFX_PING_VOL 0.5
#define CROSSTALK_GAME_FX_SFX_PING_GAP_MS 150
#define CROSSTALK_GAME_FX_SFX_TONE_HZ 1000.0
#define CROSSTALK_GAME_FX_SFX_TONE_DBFS -18.0
#define CROSSTALK_GAME_FX_SFX_TONE_DB_PER_DECADE 20.0
#define CROSSTALK_GAME_FX_SFX_CW_HZ 700.0
#define CROSSTALK_GAME_FX_SFX_CW_WPM 30
#define CROSSTALK_GAME_FX_SFX_CW_RISE_S 0.005
#define CROSSTALK_GAME_FX_SFX_CW_STEP 0x1
#define CROSSTALK_GAME_FX_SFX_CW_PATTERN_CAP 1024

static uint8_t crosstalk_game_fx_sfx_cw_key_on_is(const crosstalk_game_fx_sfx_ctx_t *ctx)
{
    if (ctx->cw_key_idx >= ctx->cw_key_cunt) { return LDG_TRUTH_FALSE; }

    return ctx->cw_keys[ctx->cw_key_idx].on != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint64_t crosstalk_game_fx_sfx_cw_key_frames_get(const ldg_proto_cw_key_t *key)
{
    return key->dur * (uint64_t)CROSSTALK_GAME_FX_SFX_RATE / (uint64_t)LDG_NS_PER_SEC;
}

static void crosstalk_game_fx_sfx_cw_advance(crosstalk_game_fx_sfx_ctx_t *ctx)
{
    while (ctx->cw_run_frames == 0)
    {
        ctx->cw_key_idx = ctx->cw_key_idx + (uint64_t)CROSSTALK_GAME_FX_SFX_CW_STEP;

        if (ctx->cw_key_idx >= ctx->cw_key_cunt)
        {
            ctx->cw_done = LDG_TRUTH_TRUE;
            return;
        }

        ctx->cw_run_frames = crosstalk_game_fx_sfx_cw_key_frames_get(&ctx->cw_keys[ctx->cw_key_idx]);
    }
}

static void crosstalk_game_fx_sfx_stream_render(crosstalk_game_fx_sfx_ctx_t *ctx, int16_t *out, uint32_t frame_cunt)
{
    double amp = pow(LDG_BASE_DEC, CROSSTALK_GAME_FX_SFX_TONE_DBFS / CROSSTALK_GAME_FX_SFX_TONE_DB_PER_DECADE);
    double hz = ctx->tone_cw == LDG_TRUTH_TRUE ? CROSSTALK_GAME_FX_SFX_CW_HZ : CROSSTALK_GAME_FX_SFX_TONE_HZ;
    double step = LDG_MATH_TAU * hz / (double)CROSSTALK_GAME_FX_SFX_RATE;
    double rise = 1.0 / (CROSSTALK_GAME_FX_SFX_CW_RISE_S * (double)CROSSTALK_GAME_FX_SFX_RATE);
    double gate = 1.0;
    uint32_t idx = 0;

    for (idx = 0; idx < frame_cunt; idx = idx + 1)
    {
        if (ctx->tone_cw == LDG_TRUTH_TRUE)
        {
            if (ctx->cw_run_frames == 0 && ctx->cw_done != LDG_TRUTH_TRUE) { crosstalk_game_fx_sfx_cw_advance(ctx); }

            if (ctx->cw_run_frames > 0) { ctx->cw_run_frames = ctx->cw_run_frames - (uint64_t)CROSSTALK_GAME_FX_SFX_CW_STEP; }

            ctx->cw_gate = ctx->cw_gate + (crosstalk_game_fx_sfx_cw_key_on_is(ctx) == LDG_TRUTH_TRUE ? rise : -rise);

            if (ctx->cw_gate > 1.0) { ctx->cw_gate = 1.0; }

            if (ctx->cw_gate < 0.0) { ctx->cw_gate = 0.0; }

            gate = ctx->cw_gate;
        }

        out[idx] = (int16_t)(sin(ctx->osc_phase) * gate * amp * CROSSTALK_GAME_FX_SFX_PEAK);
        ctx->osc_phase = ctx->osc_phase + step;

        if (ctx->osc_phase >= LDG_MATH_TAU) { ctx->osc_phase = ctx->osc_phase - LDG_MATH_TAU; }
    }
}

static uint8_t crosstalk_game_fx_sfx_cw_finished_is(const crosstalk_game_fx_sfx_ctx_t *ctx)
{
    if (ctx->tone_cw != LDG_TRUTH_TRUE || ctx->cw_done != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    return ctx->cw_run_frames == 0 && ctx->cw_gate <= 0.0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t crosstalk_game_fx_sfx_stream_cb(void *user, int16_t *dst, uint32_t frame_cunt, uint32_t *out_cunt)
{
    crosstalk_game_fx_sfx_ctx_t *ctx = (crosstalk_game_fx_sfx_ctx_t *)user;

    if (LDG_UNLIKELY(ctx == 0x0 || dst == 0x0 || out_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;
    crosstalk_game_fx_sfx_stream_render(ctx, dst, frame_cunt);
    *out_cunt = frame_cunt;
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_game_fx_sfx_vol_get(double vol)
{
    return (uint32_t)(vol * (double)LDG_AUDIO_MIX_VOL_UNITY);
}

static uint32_t crosstalk_game_fx_sfx_pitch_get(double pitch)
{
    return (uint32_t)(pitch * (double)LDG_AUDIO_MIX_PITCH_UNITY);
}

static uint32_t crosstalk_game_fx_sfx_clip_play(crosstalk_game_fx_sfx_ctx_t *ctx, uint32_t clip, double pitch, double vol)
{
    uint64_t voice = 0;

    if (clip == UINT32_MAX) { return LDG_ERR_NOT_INIT; }

    return ldg_audio_mix_clip_play(ctx->mix, clip, crosstalk_game_fx_sfx_pitch_get(pitch), crosstalk_game_fx_sfx_vol_get(vol), &voice);
}

static uint32_t crosstalk_game_fx_sfx_rng_next(uint32_t *state)
{
    uint32_t val = *state;

    val = val ^ (val << CROSSTALK_GAME_FX_SFX_RNG_SHL);
    val = val ^ (val >> CROSSTALK_GAME_FX_SFX_RNG_SHR);
    val = val ^ (val << CROSSTALK_GAME_FX_SFX_RNG_SHL2);
    *state = val;
    return val;
}

static void crosstalk_game_fx_sfx_rotary_render(int16_t *out, uint32_t frame_cunt, uint32_t *rng, double body_hz)
{
    double time_s = 0.0;
    double tick_env = 0.0;
    double body_env = 0.0;
    double noise = 0.0;
    double sample = 0.0;
    uint32_t idx = 0;

    for (idx = 0; idx < frame_cunt; idx = idx + 1)
    {
        time_s = (double)idx / (double)CROSSTALK_GAME_FX_SFX_RATE;
        tick_env = exp(-time_s / CROSSTALK_GAME_FX_SFX_ROTARY_TICK_DECAY);
        body_env = exp(-time_s / CROSSTALK_GAME_FX_SFX_ROTARY_BODY_DECAY);
        noise = (double)crosstalk_game_fx_sfx_rng_next(rng) / CROSSTALK_GAME_FX_SFX_RNG_NORM * 2.0 - 1.0;
        sample = noise * tick_env * CROSSTALK_GAME_FX_SFX_ROTARY_TICK_MIX;
        sample = sample + sin(LDG_MATH_TAU * body_hz * time_s) * body_env * CROSSTALK_GAME_FX_SFX_ROTARY_BODY_MIX;
        sample = sample * CROSSTALK_GAME_FX_SFX_ROTARY_GAIN;
        if (sample > 1.0) { sample = 1.0; }

        if (sample < -1.0) { sample = -1.0; }

        out[idx] = (int16_t)(sample * CROSSTALK_GAME_FX_SFX_PEAK);
    }
}

static uint32_t crosstalk_game_fx_sfx_rotary_ld(crosstalk_game_fx_sfx_ctx_t *ctx, double body_hz, uint32_t *out_clip)
{
    int16_t samples[CROSSTALK_GAME_FX_SFX_ROTARY_FRAMES] = LDG_ARR_ZERO_INIT;

    if (LDG_UNLIKELY(ctx == 0x0 || out_clip == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_clip = UINT32_MAX;
    crosstalk_game_fx_sfx_rotary_render(samples, CROSSTALK_GAME_FX_SFX_ROTARY_FRAMES, &ctx->rng, body_hz);
    return ldg_audio_mix_clip_ld(ctx->mix, samples, (uint64_t)CROSSTALK_GAME_FX_SFX_ROTARY_FRAMES, (uint32_t)CROSSTALK_GAME_FX_SFX_RATE, out_clip);
}

static void crosstalk_game_fx_sfx_chirp_render(int16_t *out, uint32_t frame_cunt)
{
    double time_s = 0.0;
    double dur_s = 0.0;
    double sweep = 0.0;
    double phase = 0.0;
    double env = 0.0;
    double saw = 0.0;
    double sample = 0.0;
    uint32_t idx = 0;

    dur_s = (double)frame_cunt / (double)CROSSTALK_GAME_FX_SFX_RATE;
    sweep = (CROSSTALK_GAME_FX_SFX_CHIRP_HZ_HI - CROSSTALK_GAME_FX_SFX_CHIRP_HZ_LO) / dur_s;

    for (idx = 0; idx < frame_cunt; idx = idx + 1)
    {
        time_s = (double)idx / (double)CROSSTALK_GAME_FX_SFX_RATE;
        phase = CROSSTALK_GAME_FX_SFX_CHIRP_HZ_LO * time_s + 0.5 * sweep * time_s * time_s;
        saw = 2.0 * (phase - floor(phase + 0.5));
        env = exp(-time_s / CROSSTALK_GAME_FX_SFX_CHIRP_DECAY);
        if (time_s < CROSSTALK_GAME_FX_SFX_CHIRP_ATTACK_S) { env = env * (time_s / CROSSTALK_GAME_FX_SFX_CHIRP_ATTACK_S); }

        sample = saw * env * CROSSTALK_GAME_FX_SFX_CHIRP_GAIN;
        if (sample > 1.0) { sample = 1.0; }

        if (sample < -1.0) { sample = -1.0; }

        out[idx] = (int16_t)(sample * CROSSTALK_GAME_FX_SFX_PEAK);
    }
}

static void crosstalk_game_fx_sfx_ping_render(int16_t *out, uint32_t frame_cunt)
{
    double time_s = 0.0;
    double dur_s = 0.0;
    double phase = 0.0;
    double env = 0.0;
    double shimmer_env = 0.0;
    double sample = 0.0;
    uint32_t idx = 0;

    dur_s = (double)frame_cunt / (double)CROSSTALK_GAME_FX_SFX_RATE;

    for (idx = 0; idx < frame_cunt; idx = idx + 1)
    {
        time_s = (double)idx / (double)CROSSTALK_GAME_FX_SFX_RATE;
        phase = CROSSTALK_GAME_FX_SFX_PING_HZ * time_s - 0.5 * CROSSTALK_GAME_FX_SFX_PING_GLIDE_HZ * time_s * time_s / dur_s;
        env = exp(-time_s / CROSSTALK_GAME_FX_SFX_PING_DECAY);
        shimmer_env = exp(-time_s / CROSSTALK_GAME_FX_SFX_PING_SHIMMER_DECAY);
        if (time_s < CROSSTALK_GAME_FX_SFX_PING_ATTACK_S) { env = env * (time_s / CROSSTALK_GAME_FX_SFX_PING_ATTACK_S); }

        sample = sin(LDG_MATH_TAU * phase) * env;
        sample = sample + sin(LDG_MATH_TAU * phase * CROSSTALK_GAME_FX_SFX_PING_SHIMMER_RATIO) * shimmer_env * CROSSTALK_GAME_FX_SFX_PING_SHIMMER_MIX;
        sample = sample * CROSSTALK_GAME_FX_SFX_PING_GAIN;
        if (sample > 1.0) { sample = 1.0; }

        if (sample < -1.0) { sample = -1.0; }

        out[idx] = (int16_t)(sample * CROSSTALK_GAME_FX_SFX_PEAK);
    }
}

static uint32_t crosstalk_game_fx_sfx_ping_ld(crosstalk_game_fx_sfx_ctx_t *ctx)
{
    void *buff = 0x0;
    uint32_t ret = 0;

    ret = ldg_mem_alloc(sizeof(int16_t) * CROSSTALK_GAME_FX_SFX_PING_FRAMES, &buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK || buff == 0x0)) { return LDG_ERR_ALLOC_NULL; }

    crosstalk_game_fx_sfx_ping_render((int16_t *)buff, CROSSTALK_GAME_FX_SFX_PING_FRAMES);
    ret = ldg_audio_mix_clip_ld(ctx->mix, (const int16_t *)buff, (uint64_t)CROSSTALK_GAME_FX_SFX_PING_FRAMES, (uint32_t)CROSSTALK_GAME_FX_SFX_RATE, &ctx->clip_ping);
    if (ldg_mem_dealloc(buff) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sfx ping buff dealloc failed"); }

    return ret;
}

static uint32_t crosstalk_game_fx_sfx_chirp_ld(crosstalk_game_fx_sfx_ctx_t *ctx)
{
    void *buff = 0x0;
    uint32_t ret = 0;

    ret = ldg_mem_alloc(sizeof(int16_t) * CROSSTALK_GAME_FX_SFX_CHIRP_FRAMES, &buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK || buff == 0x0)) { return LDG_ERR_ALLOC_NULL; }

    crosstalk_game_fx_sfx_chirp_render((int16_t *)buff, CROSSTALK_GAME_FX_SFX_CHIRP_FRAMES);
    ret = ldg_audio_mix_clip_ld(ctx->mix, (const int16_t *)buff, (uint64_t)CROSSTALK_GAME_FX_SFX_CHIRP_FRAMES, (uint32_t)CROSSTALK_GAME_FX_SFX_RATE, &ctx->clip_chirp);
    if (ldg_mem_dealloc(buff) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sfx chirp buff dealloc failed"); }

    return ret;
}

uint32_t crosstalk_game_fx_sfx_init(crosstalk_game_fx_sfx_ctx_t *ctx)
{
    ldg_audio_mix_conf_t conf = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->mix = 0x0;
    ctx->clip_rotary_cw = UINT32_MAX;
    ctx->clip_rotary_ccw = UINT32_MAX;
    ctx->clip_lever = UINT32_MAX;
    ctx->clip_chirp = UINT32_MAX;
    ctx->clip_ping = UINT32_MAX;
    ctx->tone_on = LDG_TRUTH_FALSE;
    ctx->tone_cw = LDG_TRUTH_FALSE;
    ctx->cw_done = LDG_TRUTH_FALSE;
    ctx->cw_key_cunt = 0;
    ctx->cw_key_idx = 0;
    ctx->cw_run_frames = 0;
    ctx->osc_phase = 0.0;
    ctx->cw_gate = 0.0;
    ctx->play_cb = 0x0;
    ctx->play_cb_user = 0x0;
    ctx->ready = LDG_TRUTH_FALSE;
    ctx->last_play_ms = 0;
    ctx->last_lever_ms = 0;
    ctx->last_chirp_ms = 0;
    ctx->last_ping_ms = 0;
    ctx->rng = CROSSTALK_GAME_FX_SFX_RNG_SEED;

    ret = ldg_audio_mix_conf_default_get(&conf);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    conf.rate = (uint32_t)CROSSTALK_GAME_FX_SFX_MIX_RATE;
    conf.channel_cunt = (uint16_t)CROSSTALK_GAME_FX_SFX_MIX_CHANNELS;
    conf.player_en = LDG_TRUTH_TRUE;

    ret = ldg_audio_mix_create(&conf, &ctx->mix);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_audio_mix_attach(ctx->mix);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (ldg_audio_mix_destroy(ctx->mix) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sfx mix destroy failed"); }

        ctx->mix = 0x0;
        return ret;
    }

    ret = crosstalk_game_fx_sfx_rotary_ld(ctx, CROSSTALK_GAME_FX_SFX_ROTARY_CW_BODY_HZ, &ctx->clip_rotary_cw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sfx rotary cw ld failed; err: %u32", ret); }

    ret = crosstalk_game_fx_sfx_rotary_ld(ctx, CROSSTALK_GAME_FX_SFX_ROTARY_CCW_BODY_HZ, &ctx->clip_rotary_ccw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sfx rotary ccw ld failed; err: %u32", ret); }

    ret = crosstalk_game_fx_sfx_rotary_ld(ctx, CROSSTALK_GAME_FX_SFX_LEVER_BODY_HZ, &ctx->clip_lever);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sfx lever ld failed; err: %u32", ret); }

    ret = crosstalk_game_fx_sfx_chirp_ld(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sfx chirp ld failed; err: %u32", ret); }

    ret = crosstalk_game_fx_sfx_ping_ld(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sfx ping ld failed; err: %u32", ret); }

    ctx->ready = LDG_TRUTH_TRUE;
    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_fx_sfx_shutdown(crosstalk_game_fx_sfx_ctx_t *ctx)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ready != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (ctx->tone_on == LDG_TRUTH_TRUE && ldg_audio_mix_stream_clear(ctx->mix) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sfx stream clear failed"); }

    if (ldg_audio_mix_detach(ctx->mix) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sfx mix detach failed"); }

    ret = ldg_audio_mix_destroy(ctx->mix);
    ctx->mix = 0x0;
    ctx->clip_rotary_cw = UINT32_MAX;
    ctx->clip_rotary_ccw = UINT32_MAX;
    ctx->clip_lever = UINT32_MAX;
    ctx->clip_chirp = UINT32_MAX;
    ctx->clip_ping = UINT32_MAX;
    ctx->tone_on = LDG_TRUTH_FALSE;
    ctx->tone_cw = LDG_TRUTH_FALSE;
    ctx->ready = LDG_TRUTH_FALSE;
    return ret;
}

uint32_t crosstalk_game_fx_sfx_tone_start(crosstalk_game_fx_sfx_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ready != LDG_TRUTH_TRUE || ctx->tone_on == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ctx->tone_cw = LDG_TRUTH_FALSE;
    ctx->osc_phase = 0.0;
    ret = ldg_audio_mix_stream_set(ctx->mix, crosstalk_game_fx_sfx_stream_cb, ctx, (uint32_t)CROSSTALK_GAME_FX_SFX_RATE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->tone_on = LDG_TRUTH_TRUE;
    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_fx_sfx_cw_start(crosstalk_game_fx_sfx_ctx_t *ctx, const char *text, double probability, uint8_t *out_sent)
{
    char pattern[CROSSTALK_GAME_FX_SFX_CW_PATTERN_CAP] = LDG_ARR_ZERO_INIT;
    ldg_proto_cw_timing_t timing = LDG_STRUCT_ZERO_INIT;
    uint64_t pattern_len = 0;
    double roll = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0 || out_sent == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_sent = LDG_TRUTH_FALSE;

    if (ctx->ready != LDG_TRUTH_TRUE || ctx->tone_on == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (text[0] == LDG_STR_TERM) { return LDG_ERR_AOK; }

    roll = (double)crosstalk_game_fx_sfx_rng_next(&ctx->rng) / CROSSTALK_GAME_FX_SFX_RNG_NORM;
    if (roll >= probability) { return LDG_ERR_AOK; }

    ret = ldg_proto_cw_text_to_pattern_convert(text, pattern, sizeof(pattern), &pattern_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    timing.dit = ldg_proto_cw_dit_get(CROSSTALK_GAME_FX_SFX_CW_WPM);
    timing.gap_dit = timing.dit;
    ret = ldg_proto_cw_pattern_to_key_convert(pattern, &timing, ctx->cw_keys, CROSSTALK_GAME_FX_SFX_CW_KEY_CAP, &ctx->cw_key_cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (ctx->cw_key_cunt == 0) { return LDG_ERR_EMPTY; }

    ctx->cw_key_idx = 0;
    ctx->cw_run_frames = crosstalk_game_fx_sfx_cw_key_frames_get(&ctx->cw_keys[0]);
    ctx->cw_gate = 0.0;
    ctx->cw_done = LDG_TRUTH_FALSE;
    ctx->tone_cw = LDG_TRUTH_TRUE;
    ctx->osc_phase = 0.0;
    ret = ldg_audio_mix_stream_set(ctx->mix, crosstalk_game_fx_sfx_stream_cb, ctx, (uint32_t)CROSSTALK_GAME_FX_SFX_RATE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->tone_on = LDG_TRUTH_TRUE;
    *out_sent = LDG_TRUTH_TRUE;
    return LDG_ERR_AOK;
}

uint8_t crosstalk_game_fx_sfx_cw_on_is(const crosstalk_game_fx_sfx_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->tone_on == LDG_TRUTH_TRUE && ctx->tone_cw == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_game_fx_sfx_tone_stop(crosstalk_game_fx_sfx_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->tone_on != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = ldg_audio_mix_stream_clear(ctx->mix);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->tone_on = LDG_TRUTH_FALSE;
    ctx->tone_cw = LDG_TRUTH_FALSE;
    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_fx_sfx_tone_tick(crosstalk_game_fx_sfx_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->tone_on != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (crosstalk_game_fx_sfx_cw_finished_is(ctx) == LDG_TRUTH_TRUE) { return crosstalk_game_fx_sfx_tone_stop(ctx); }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_game_fx_sfx_play_cb_invoke(crosstalk_game_fx_sfx_ctx_t *ctx, crosstalk_game_fx_sfx_kind_t kind)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->play_cb == 0x0) { return LDG_ERR_AOK; }

    return ctx->play_cb(ctx->play_cb_user, kind);
}

uint32_t crosstalk_game_fx_sfx_play_cb_set(crosstalk_game_fx_sfx_ctx_t *ctx, crosstalk_game_fx_sfx_play_cb_t cb, void *user)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->play_cb = cb;
    ctx->play_cb_user = user;
    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_fx_sfx_rotary_play(crosstalk_game_fx_sfx_ctx_t *ctx, crosstalk_game_fx_sfx_rotary_dir_t dir)
{
    uint32_t clip = UINT32_MAX;
    uint64_t now_ms = 0;
    double pitch = 0.0;
    uint32_t ret = 0;
    crosstalk_game_fx_sfx_kind_t kind = CROSSTALK_GAME_FX_SFX_KIND_ROTARY_CW;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ready != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    now_ms = ldg_time_epoch_ms_get();
    if (now_ms - ctx->last_play_ms < CROSSTALK_GAME_FX_SFX_MIN_GAP_MS) { return LDG_ERR_AOK; }

    if (dir == CROSSTALK_GAME_FX_SFX_ROTARY_CCW)
    {
        clip = ctx->clip_rotary_ccw;
        kind = CROSSTALK_GAME_FX_SFX_KIND_ROTARY_CCW;
    }
    else
    {
        clip = ctx->clip_rotary_cw;
        kind = CROSSTALK_GAME_FX_SFX_KIND_ROTARY_CW;
    }

    pitch = CROSSTALK_GAME_FX_SFX_PITCH_MIN + (double)crosstalk_game_fx_sfx_rng_next(&ctx->rng) / CROSSTALK_GAME_FX_SFX_RNG_NORM * CROSSTALK_GAME_FX_SFX_PITCH_SPAN;
    ret = crosstalk_game_fx_sfx_clip_play(ctx, clip, pitch, CROSSTALK_GAME_FX_SFX_ROTARY_VOL);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->last_play_ms = now_ms;
    ret = crosstalk_game_fx_sfx_play_cb_invoke(ctx, kind);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_fx_sfx_rotary_step_play(crosstalk_game_fx_sfx_ctx_t *ctx, uint32_t step, uint32_t step_cunt, crosstalk_game_fx_sfx_rotary_dir_t dir)
{
    uint32_t clip = UINT32_MAX;
    uint64_t now_ms = 0;
    double pitch = 0.0;
    double rise = 0.0;
    uint32_t ret = 0;
    crosstalk_game_fx_sfx_kind_t kind = CROSSTALK_GAME_FX_SFX_KIND_ROTARY_CW;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ready != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    now_ms = ldg_time_epoch_ms_get();
    if (now_ms - ctx->last_play_ms < CROSSTALK_GAME_FX_SFX_MIN_GAP_MS) { return LDG_ERR_AOK; }

    if (dir == CROSSTALK_GAME_FX_SFX_ROTARY_CCW)
    {
        clip = ctx->clip_rotary_ccw;
        kind = CROSSTALK_GAME_FX_SFX_KIND_ROTARY_CCW;
    }
    else
    {
        clip = ctx->clip_rotary_cw;
        kind = CROSSTALK_GAME_FX_SFX_KIND_ROTARY_CW;
    }

    if (step_cunt > CROSSTALK_GAME_FX_SFX_STEP_LAST_OFF) { rise = (double)step / (double)(step_cunt - CROSSTALK_GAME_FX_SFX_STEP_LAST_OFF); }

    if (rise > 1.0) { rise = 1.0; }

    pitch = CROSSTALK_GAME_FX_SFX_PITCH_MIN + (double)crosstalk_game_fx_sfx_rng_next(&ctx->rng) / CROSSTALK_GAME_FX_SFX_RNG_NORM * CROSSTALK_GAME_FX_SFX_STEP_JITTER + rise * CROSSTALK_GAME_FX_SFX_STEP_RISE;
    ret = crosstalk_game_fx_sfx_clip_play(ctx, clip, pitch, CROSSTALK_GAME_FX_SFX_ROTARY_VOL);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->last_play_ms = now_ms;
    ret = crosstalk_game_fx_sfx_play_cb_invoke(ctx, kind);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_fx_sfx_lever_play(crosstalk_game_fx_sfx_ctx_t *ctx)
{
    uint64_t now_ms = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ready != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    now_ms = ldg_time_epoch_ms_get();
    if (now_ms - ctx->last_lever_ms < CROSSTALK_GAME_FX_SFX_LEVER_GAP_MS) { return LDG_ERR_AOK; }

    ret = crosstalk_game_fx_sfx_clip_play(ctx, ctx->clip_lever, CROSSTALK_GAME_FX_SFX_LEVER_PITCH, CROSSTALK_GAME_FX_SFX_LEVER_VOL);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->last_lever_ms = now_ms;
    ret = crosstalk_game_fx_sfx_play_cb_invoke(ctx, CROSSTALK_GAME_FX_SFX_KIND_LEVER);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_fx_sfx_chirp_play(crosstalk_game_fx_sfx_ctx_t *ctx)
{
    uint64_t now_ms = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ready != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    now_ms = ldg_time_epoch_ms_get();
    if (now_ms - ctx->last_chirp_ms < CROSSTALK_GAME_FX_SFX_CHIRP_GAP_MS) { return LDG_ERR_AOK; }

    ret = crosstalk_game_fx_sfx_clip_play(ctx, ctx->clip_chirp, CROSSTALK_GAME_FX_SFX_PITCH_NATIVE, CROSSTALK_GAME_FX_SFX_CHIRP_VOL);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->last_chirp_ms = now_ms;
    ret = crosstalk_game_fx_sfx_play_cb_invoke(ctx, CROSSTALK_GAME_FX_SFX_KIND_CHIRP);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_fx_sfx_music_vol_set(crosstalk_game_fx_sfx_ctx_t *ctx, double vol)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->mix == 0x0) { return LDG_ERR_NOT_INIT; }

    return ldg_audio_mix_bus_vol_set(ctx->mix, (uint32_t)LDG_AUDIO_MIX_BUS_MUSIC, crosstalk_game_fx_sfx_vol_get(vol));
}

uint32_t crosstalk_game_fx_sfx_ping_play(crosstalk_game_fx_sfx_ctx_t *ctx)
{
    uint64_t now_ms = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ready != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    now_ms = ldg_time_epoch_ms_get();
    if (now_ms - ctx->last_ping_ms < CROSSTALK_GAME_FX_SFX_PING_GAP_MS) { return LDG_ERR_AOK; }

    ret = crosstalk_game_fx_sfx_clip_play(ctx, ctx->clip_ping, CROSSTALK_GAME_FX_SFX_PITCH_NATIVE, CROSSTALK_GAME_FX_SFX_PING_VOL);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->last_ping_ms = now_ms;
    ret = crosstalk_game_fx_sfx_play_cb_invoke(ctx, CROSSTALK_GAME_FX_SFX_KIND_PING);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return LDG_ERR_AOK;
}
