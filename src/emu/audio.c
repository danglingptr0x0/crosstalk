#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <dangling/logging/logging.h>

#include <dangling/audio/audio.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#include "core/log.h"
#include "emu/audio.h"

#define AUDIO_STEP 1

static uint32_t audio_depth_get(const crosstalk_emu_audio_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return UINT32_MAX; }

    return (ctx->tl - ctx->hd) & (uint32_t)CROSSTALK_EMU_AUDIO_RING_MASK;
}

static uint32_t audio_sample_push(crosstalk_emu_audio_ctx_t *ctx, ldg_byte_t raw)
{
    uint32_t next = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    next = (ctx->tl + (uint32_t)AUDIO_STEP) & (uint32_t)CROSSTALK_EMU_AUDIO_RING_MASK;

    if (next == ctx->hd)
    {
        ctx->dropped = ctx->dropped + (uint64_t)AUDIO_STEP;

        return LDG_ERR_FULL;
    }

    ctx->ring[ctx->tl] = (uint16_t)((uint32_t)(raw ^ (ldg_byte_t)CROSSTALK_EMU_AUDIO_SIGN_FLIP) << (uint32_t)CROSSTALK_EMU_AUDIO_SAMPLE_SHIFT);
    ctx->tl = next;
    ctx->produced = ctx->produced + (uint64_t)AUDIO_STEP;

    return LDG_ERR_AOK;
}

static uint32_t audio_span_push(crosstalk_emu_audio_ctx_t *ctx, const ldg_byte_t *ram, uint64_t ram_bytes, uint64_t from, uint64_t to)
{
    uint64_t at = 0;
    uint64_t span = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || ram == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (to <= from) { return LDG_ERR_AOK; }

    if (from >= ram_bytes) { return LDG_ERR_AOK; }

    if (to > ram_bytes) { to = ram_bytes; }

    span = to - from;

    if (span > (uint64_t)CROSSTALK_EMU_AUDIO_SPAN_MAX) { from = to - (uint64_t)CROSSTALK_EMU_AUDIO_SPAN_MAX; }

    for (at = from; at < to; at = at + (uint64_t)AUDIO_STEP) { if (audio_sample_push(ctx, ram[at]) != LDG_ERR_AOK) { break; } }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_audio_frames_pull(void *user, void *buff, uint32_t frame_cunt, uint32_t *out)
{
    crosstalk_emu_audio_ctx_t *ctx = (crosstalk_emu_audio_ctx_t *)user;
    uint16_t *dst = (uint16_t *)buff;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || buff == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    for (at = 0; at < frame_cunt; at = at + (uint32_t)AUDIO_STEP)
    {
        if (ctx->hd == ctx->tl)
        {
            ctx->underrun = ctx->underrun + (uint64_t)(frame_cunt - at);

            break;
        }

        dst[at] = ctx->ring[ctx->hd];
        ctx->hd = (ctx->hd + (uint32_t)AUDIO_STEP) & (uint32_t)CROSSTALK_EMU_AUDIO_RING_MASK;
    }

    ctx->pulled = ctx->pulled + (uint64_t)at;
    *out = at;

    while (at < frame_cunt)
    {
        dst[at] = 0;
        at = at + (uint32_t)AUDIO_STEP;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_audio_init(crosstalk_emu_audio_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->curr = (uint64_t)UINT64_MAX;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_audio_stop(crosstalk_emu_audio_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->streaming != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (ldg_audio_sink_pcm_close() != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu pcm sink close failed"); }

    ctx->streaming = LDG_TRUTH_FALSE;
    ctx->live = LDG_TRUTH_FALSE;
    ctx->curr = (uint64_t)UINT64_MAX;
    ctx->hd = 0;
    ctx->tl = 0;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu audio: sound card released");

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_audio_shutdown(crosstalk_emu_audio_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_emu_audio_stop(ctx);
}

static uint32_t audio_start(crosstalk_emu_audio_ctx_t *ctx, uint32_t rate)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_emu_audio_stop(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_audio_sink_pcm_open(rate, (uint16_t)CROSSTALK_EMU_AUDIO_CHANNEL_CUNT, (uint16_t)CROSSTALK_EMU_AUDIO_BIT_DEPTH);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu audio: pcm sink open failed; rate: %u32; err: %u32", rate, ret);

        return CROSSTALK_EMU_AUDIO_ERR_OPEN;
    }

    ret = ldg_audio_sink_pcm_src_set(crosstalk_emu_audio_frames_pull, ctx);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu audio: pcm src bind failed; err: %u32", ret);

        if (ldg_audio_sink_pcm_close() != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu pcm sink close failed"); }

        return CROSSTALK_EMU_AUDIO_ERR_OPEN;
    }

    ctx->rate = rate;
    ctx->streaming = LDG_TRUTH_TRUE;
    ctx->live = LDG_TRUTH_TRUE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu audio: sound card routed; rate: %u32 Hz; channels: %u32; depth: %u32", rate, (uint32_t)CROSSTALK_EMU_AUDIO_CHANNEL_CUNT, (uint32_t)CROSSTALK_EMU_AUDIO_BIT_DEPTH);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_audio_tick(crosstalk_emu_audio_ctx_t *ctx, const crosstalk_hdl_abi_t *abi)
{
    uint8_t pcm[CROSSTALK_EMU_AUDIO_DRAIN_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t got = 0x0;
    uint64_t drained = 0x0;
    uint32_t ret = 0x0;
    uint64_t *dsu = 0x0;
    uint64_t *mem = 0x0;
    const ldg_byte_t *ram = 0x0;
    uint64_t ram_bytes = 0;
    uint64_t base = 0;
    uint64_t tail = 0;
    uint64_t curr = 0;
    uint64_t rate = 0;
    uint64_t run = 0;
    uint32_t dsu_cunt = 0;
    uint32_t mem_cunt = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || abi == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (abi->state_get == 0x0) { return LDG_ERR_UNSUPPORTED; }

    if (abi->state_get((uint32_t)CROSSTALK_HDL_ABI_STATE_EMU_DSU, &dsu, &dsu_cunt) != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    if (dsu == 0x0 || dsu_cunt < (uint32_t)CROSSTALK_HDL_ABI_DSU_CUNT) { return LDG_ERR_AOK; }

    base = dsu[CROSSTALK_HDL_ABI_DSU_BASE_AT];
    tail = dsu[CROSSTALK_HDL_ABI_DSU_TAIL_AT];
    curr = dsu[CROSSTALK_HDL_ABI_DSU_CURR_AT];
    rate = dsu[CROSSTALK_HDL_ABI_DSU_RATE_AT];
    run = dsu[CROSSTALK_HDL_ABI_DSU_RUN_AT];

    if (abi->audio_drain != 0x0)
    {
        if (LDG_UNLIKELY(rate < CROSSTALK_EMU_AUDIO_RATE_MIN || rate > CROSSTALK_EMU_AUDIO_RATE_MAX)) { return CROSSTALK_EMU_AUDIO_ERR_RATE; }

        while (drained < CROSSTALK_EMU_AUDIO_SPAN_MAX)
        {
            ret = abi->audio_drain(pcm, sizeof(pcm), &got);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (LDG_UNLIKELY(got > sizeof(pcm))) { return LDG_ERR_BOUNDS; }

            if (got == 0x0) { return LDG_ERR_AOK; }

            if (ctx->streaming != LDG_TRUTH_TRUE || ctx->rate != (uint32_t)rate)
            {
                ret = audio_start(ctx, (uint32_t)rate);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            if (audio_depth_get(ctx) < CROSSTALK_EMU_AUDIO_DEPTH_MAX)
            {
                ret = audio_span_push(ctx, pcm, got, 0x0, got);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            drained += got;
            if (got < sizeof(pcm)) { return LDG_ERR_AOK; }
        }
        return LDG_ERR_AOK;
    }

    if (run == 0 || base == 0 || tail <= base || rate < (uint64_t)CROSSTALK_EMU_AUDIO_RATE_MIN || rate > (uint64_t)CROSSTALK_EMU_AUDIO_RATE_MAX)
    {
        if (ctx->streaming == LDG_TRUTH_TRUE) { return crosstalk_emu_audio_stop(ctx); }

        return LDG_ERR_AOK;
    }

    if (ctx->streaming != LDG_TRUTH_TRUE || ctx->rate != (uint32_t)rate) { if (audio_start(ctx, (uint32_t)rate) != LDG_ERR_AOK) { return LDG_ERR_AOK; } }

    if (abi->state_get((uint32_t)CROSSTALK_HDL_ABI_STATE_MEM, &mem, &mem_cunt) != LDG_ERR_AOK || mem == 0x0) { return LDG_ERR_AOK; }

    ram = (const ldg_byte_t *)(const void *)mem;
    ram_bytes = (uint64_t)mem_cunt * (uint64_t)CROSSTALK_EMU_AUDIO_WORD_BYTES;

    if (ctx->curr == (uint64_t)UINT64_MAX) { ctx->curr = curr; }

    if (audio_depth_get(ctx) >= (uint32_t)CROSSTALK_EMU_AUDIO_DEPTH_MAX)
    {
        ctx->curr = curr;

        return LDG_ERR_AOK;
    }

    if (curr >= ctx->curr) { if (audio_span_push(ctx, ram, ram_bytes, ctx->curr, curr) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu audio span push failed"); } }
    else
    {
        if (audio_span_push(ctx, ram, ram_bytes, ctx->curr, tail) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu audio span push failed"); }

        if (audio_span_push(ctx, ram, ram_bytes, base, curr) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu audio span push failed"); }
    }

    ctx->curr = curr;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_emu_audio_live_is(const crosstalk_emu_audio_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->live == LDG_TRUTH_TRUE && ctx->streaming == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}
