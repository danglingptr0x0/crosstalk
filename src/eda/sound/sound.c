#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <dangling/arch/amd64/atomic.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/audio/audio.h>
#include <dangling/str/str.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "eda/sound/sound.h"

static void sample_put(crosstalk_eda_sound_ctx_t *ctx, uint16_t val)
{
    uint32_t tl = ctx->tl;
    uint32_t hd = LDG_LD_ACQUIRE(ctx->hd);

    if (tl - hd >= (uint32_t)CROSSTALK_EDA_SOUND_RING_CAP) { ctx->dropped = ctx->dropped + (uint64_t)CROSSTALK_EDA_SOUND_STEP; }

    ctx->ring[tl & (uint32_t)CROSSTALK_EDA_SOUND_RING_MASK] = val;

    LDG_ST_RELEASE(ctx->tl, tl + (uint32_t)CROSSTALK_EDA_SOUND_STEP);

    ctx->produced = ctx->produced + (uint64_t)CROSSTALK_EDA_SOUND_STEP;
}

static uint32_t avail_claim(crosstalk_eda_sound_ctx_t *ctx)
{
    uint32_t tl = LDG_LD_ACQUIRE(ctx->tl);
    uint32_t avail = tl - ctx->hd;

    if (avail > (uint32_t)CROSSTALK_EDA_SOUND_RING_CAP)
    {
        LDG_ST_RELEASE(ctx->hd, tl - (uint32_t)CROSSTALK_EDA_SOUND_RING_CAP);
        avail = (uint32_t)CROSSTALK_EDA_SOUND_RING_CAP;
    }

    return avail;
}

uint32_t crosstalk_eda_sound_init(crosstalk_eda_sound_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->rate = (uint32_t)CROSSTALK_EDA_SOUND_RATE_DEFAULT;
    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_sound_shutdown(crosstalk_eda_sound_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (crosstalk_eda_sound_stream_stop(ctx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sound stream stop failed"); }

    memset(ctx, 0x0, sizeof(*ctx));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_sound_rate_set(crosstalk_eda_sound_ctx_t *ctx, uint32_t rate)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (rate < (uint32_t)CROSSTALK_EDA_SOUND_RATE_MIN || rate > (uint32_t)CROSSTALK_EDA_SOUND_RATE_MAX) { return CROSSTALK_EDA_SOUND_ERR_RATE; }

    ctx->rate = rate;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_sound_bind(crosstalk_eda_sound_ctx_t *ctx, const char *port)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || port == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ret = ldg_str_rbrcpy(ctx->port, port, (uint64_t)sizeof(ctx->port));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    ctx->pending = 0x0;
    ctx->pending_len = 0x0;
    ctx->produced = 0x0;
    ctx->pulled = 0x0;
    ctx->dropped = 0x0;
    ctx->underrun = 0x0;
    ctx->report_mark = 0x0;
    ctx->report_acc = 0.0;

    LDG_ST_RELEASE(ctx->hd, 0x0);
    LDG_ST_RELEASE(ctx->tl, 0x0);

    ctx->live = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_sound_unbind(crosstalk_eda_sound_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (crosstalk_eda_sound_stream_stop(ctx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sound stream stop failed"); }

    ctx->live = LDG_TRUTH_FALSE;
    ctx->port[0] = LDG_STR_TERM;

    LDG_ST_RELEASE(ctx->hd, 0x0);
    LDG_ST_RELEASE(ctx->tl, 0x0);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_sound_sink_cb(void *user, const uint8_t *bytes, uint32_t len)
{
    crosstalk_eda_sound_ctx_t *ctx = (crosstalk_eda_sound_ctx_t *)user;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || bytes == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    for (idx = 0; idx < len; idx++)
    {
        if (ctx->pending_len == 0)
        {
            ctx->pending = (uint32_t)bytes[idx];
            ctx->pending_len = (uint8_t)CROSSTALK_EDA_SOUND_STEP;
            continue;
        }

        sample_put(ctx, (uint16_t)(ctx->pending | ((uint32_t)bytes[idx] << (uint32_t)LDG_BYTE_BITS)));
        ctx->pending = 0x0;
        ctx->pending_len = 0x0;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_sound_frames_pull(void *user, void *buff, uint32_t frame_cunt, uint32_t *out)
{
    crosstalk_eda_sound_ctx_t *ctx = (crosstalk_eda_sound_ctx_t *)user;
    uint16_t *frame = (uint16_t *)buff;
    uint32_t avail = 0;
    uint32_t cunt = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || buff == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    avail = avail_claim(ctx);

    while (cunt < frame_cunt && cunt < avail)
    {
        frame[cunt] = ctx->ring[(ctx->hd + cunt) & (uint32_t)CROSSTALK_EDA_SOUND_RING_MASK];
        cunt = cunt + (uint32_t)CROSSTALK_EDA_SOUND_STEP;
    }

    if (cunt != 0) { LDG_ST_RELEASE(ctx->hd, ctx->hd + cunt); }

    ctx->pulled = ctx->pulled + (uint64_t)cunt;

    if (cunt < frame_cunt)
    {
        uint32_t at = cunt;

        while (at < frame_cunt)
        {
            frame[at] = (uint16_t)CROSSTALK_EDA_SOUND_SILENCE;
            at = at + (uint32_t)CROSSTALK_EDA_SOUND_STEP;
        }

        ctx->underrun = ctx->underrun + (uint64_t)(frame_cunt - cunt);
    }

    *out = frame_cunt;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_sound_stream_start(crosstalk_eda_sound_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (ctx->streaming == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_eda_sound_stream_stop(ctx);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    ret = ldg_audio_sink_pcm_open(ctx->rate, (uint16_t)CROSSTALK_EDA_SOUND_CHANNEL_CUNT, (uint16_t)CROSSTALK_EDA_SOUND_BIT_DEPTH);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sound: pcm sink open failed; rate: %u32; err: %u32", ctx->rate, ret);
        return ret;
    }

    ret = ldg_audio_sink_pcm_src_set(crosstalk_eda_sound_frames_pull, ctx);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sound: pcm src bind failed; err: %u32", ret);

        if (ldg_audio_sink_pcm_close() != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pcm sink close failed"); }

        return ret;
    }

    ctx->streaming = LDG_TRUTH_TRUE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "sound: pass-through open; port: %.63s; rate: %u32; channels: %u32; depth: %u32", ctx->port, ctx->rate, (uint32_t)CROSSTALK_EDA_SOUND_CHANNEL_CUNT, (uint32_t)CROSSTALK_EDA_SOUND_BIT_DEPTH);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_sound_depth_get(const crosstalk_eda_sound_ctx_t *ctx, uint32_t *out)
{
    uint32_t held = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    held = LDG_LD_ACQUIRE(ctx->tl) - ctx->hd;
    if (held > (uint32_t)CROSSTALK_EDA_SOUND_RING_CAP) { held = (uint32_t)CROSSTALK_EDA_SOUND_RING_CAP; }

    *out = held;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_sound_flush(crosstalk_eda_sound_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->streaming == LDG_TRUTH_TRUE) { return CROSSTALK_EDA_SOUND_ERR_STREAMING; }

    ctx->hd = LDG_LD_ACQUIRE(ctx->tl);
    ctx->pending = 0;
    ctx->pending_len = 0;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_sound_stream_stop(crosstalk_eda_sound_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->streaming != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ctx->streaming = LDG_TRUTH_FALSE;

    ret = ldg_audio_sink_pcm_close();
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sound: pcm sink close failed; err: %u32", ret); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "sound: pass-through closed; pulled: %u64; underrun: %u64; dropped: %u64", ctx->pulled, ctx->underrun, ctx->dropped);

    return ret;
}

uint8_t crosstalk_eda_sound_streaming_is(const crosstalk_eda_sound_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->streaming == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_eda_sound_fmt_get(const crosstalk_eda_sound_ctx_t *ctx, uint32_t *out_rate, uint16_t *out_channel_cunt, uint16_t *out_bit_depth)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_rate == 0x0 || out_channel_cunt == 0x0 || out_bit_depth == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_rate = ctx->rate;
    *out_channel_cunt = (uint16_t)CROSSTALK_EDA_SOUND_CHANNEL_CUNT;
    *out_bit_depth = (uint16_t)CROSSTALK_EDA_SOUND_BIT_DEPTH;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_sound_bake(crosstalk_eda_sound_ctx_t *ctx, const ldg_byte_t **out_blob, uint64_t *out_size)
{
    uint64_t pcm_size = 0;
    uint32_t frames = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_blob == 0x0 || out_size == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_blob = 0x0;
    *out_size = 0x0;

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (ctx->streaming == LDG_TRUTH_TRUE) { return CROSSTALK_EDA_SOUND_ERR_STREAMING; }

    frames = avail_claim(ctx);
    if (frames == 0) { return LDG_ERR_EMPTY; }

    for (idx = 0; idx < frames; idx++)
    {
        uint16_t val = ctx->ring[(ctx->hd + idx) & (uint32_t)CROSSTALK_EDA_SOUND_RING_MASK];
        uint32_t at = idx * (uint32_t)CROSSTALK_EDA_SOUND_SAMPLE_BYTES;

        ctx->pcm[at] = (ldg_byte_t)(val & (uint16_t)LDG_BYTE_MASK);
        ctx->pcm[at + (uint32_t)CROSSTALK_EDA_SOUND_STEP] = (ldg_byte_t)((val >> (uint16_t)LDG_BYTE_BITS) & (uint16_t)LDG_BYTE_MASK);
    }

    pcm_size = (uint64_t)frames * (uint64_t)CROSSTALK_EDA_SOUND_SAMPLE_BYTES;

    ret = ldg_audio_wav_wrap(ctx->pcm, pcm_size, ctx->rate, (uint16_t)CROSSTALK_EDA_SOUND_CHANNEL_CUNT, (uint16_t)CROSSTALK_EDA_SOUND_BIT_DEPTH, ctx->blob, (uint64_t)CROSSTALK_EDA_SOUND_BLOB_CAP, out_size);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sound: wav wrap failed; pcm: %u64; err: %u32", pcm_size, ret);
        return ret;
    }

    *out_blob = ctx->blob;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "sound: baked; port: %.63s; frames: %u32; rate: %u32; blob: %u64 bytes", ctx->port, frames, ctx->rate, *out_size);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_sound_tick(crosstalk_eda_sound_ctx_t *ctx, double dt_secs)
{
    uint64_t made = 0;
    uint32_t held = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ctx->report_acc = ctx->report_acc + dt_secs;
    if (ctx->report_acc < CROSSTALK_EDA_SOUND_REPORT_SECS) { return LDG_ERR_AOK; }

    made = ctx->produced - ctx->report_mark;
    ctx->report_mark = ctx->produced;
    ctx->report_acc = 0.0;

    if (made == ctx->made_prev && ctx->dropped == ctx->dropped_prev && ctx->underrun == ctx->underrun_prev) { return LDG_ERR_AOK; }

    ctx->made_prev = made;
    ctx->dropped_prev = ctx->dropped;
    ctx->underrun_prev = ctx->underrun;

    held = LDG_LD_ACQUIRE(ctx->tl) - ctx->hd;
    if (held > (uint32_t)CROSSTALK_EDA_SOUND_RING_CAP) { held = (uint32_t)CROSSTALK_EDA_SOUND_RING_CAP; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "sound: rate; port: %.63s; made: %u64/s; want: %u32/s; ring: %u32; dropped: %u64; underrun: %u64", ctx->port, made, ctx->rate, held, ctx->dropped, ctx->underrun);

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_sound_live_is(const crosstalk_eda_sound_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->live == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}
