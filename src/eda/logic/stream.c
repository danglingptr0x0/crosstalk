#include <stdint.h>
#include <string.h>

#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#include "eda/logic/stream.h"

static uint32_t stream_sinks_feed(crosstalk_eda_logic_stream_t *stream, const uint8_t *bytes, uint32_t len)
{
    uint32_t idx = 0;
    uint32_t ret = LDG_ERR_AOK;

    if (len == 0x0) { return LDG_ERR_AOK; }

    for (idx = 0; idx < stream->sink_cunt; idx = idx + 0x1)
    {
        if (stream->sink[idx].cb == 0x0) { continue; }

        ret = stream->sink[idx].cb(stream->sink[idx].user, bytes, len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    stream->bytes_total = stream->bytes_total + (uint64_t)len;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_logic_stream_init(crosstalk_eda_logic_stream_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_logic_stream_shutdown(crosstalk_eda_logic_stream_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_logic_stream_open(crosstalk_eda_logic_stream_ctx_t *ctx, uint32_t probe_handle, crosstalk_eda_logic_stream_decode_t decode, uint32_t *out)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (probe_handle >= CROSSTALK_EDA_LOGIC_PROBE_MAX) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < CROSSTALK_EDA_LOGIC_STREAM_MAX; idx = idx + 0x1) { if (ctx->stream[idx].live != LDG_TRUTH_TRUE) { break; } }

    if (idx >= CROSSTALK_EDA_LOGIC_STREAM_MAX) { return CROSSTALK_EDA_LOGIC_STREAM_ERR_FULL; }

    ctx->stream[idx].probe_handle = probe_handle;
    ctx->stream[idx].decode = (uint32_t)decode;
    ctx->stream[idx].sink_cunt = 0x0;
    ctx->stream[idx].bytes_total = 0x0;
    ctx->stream[idx].level_val = 0x0;
    ctx->stream[idx].level_cycle = 0x0;
    ctx->stream[idx].live = LDG_TRUTH_TRUE;

    *out = idx;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_logic_stream_close(crosstalk_eda_logic_stream_ctx_t *ctx, uint32_t handle)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (handle >= CROSSTALK_EDA_LOGIC_STREAM_MAX) { return CROSSTALK_EDA_LOGIC_STREAM_ERR_HANDLE; }

    ctx->stream[handle] = (crosstalk_eda_logic_stream_t)LDG_STRUCT_ZERO_INIT;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_logic_stream_sink_add(crosstalk_eda_logic_stream_ctx_t *ctx, uint32_t handle, crosstalk_eda_logic_stream_sink_cb_t cb, void *user)
{
    crosstalk_eda_logic_stream_t *stream = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0 || cb == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (handle >= CROSSTALK_EDA_LOGIC_STREAM_MAX) { return CROSSTALK_EDA_LOGIC_STREAM_ERR_HANDLE; }

    stream = &ctx->stream[handle];
    if (stream->live != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (stream->sink_cunt >= CROSSTALK_EDA_LOGIC_STREAM_SINK_MAX) { return CROSSTALK_EDA_LOGIC_STREAM_ERR_SINK_FULL; }

    stream->sink[stream->sink_cunt].cb = cb;
    stream->sink[stream->sink_cunt].user = user;
    stream->sink_cunt = stream->sink_cunt + 0x1;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_logic_stream_width_set(crosstalk_eda_logic_stream_ctx_t *ctx, uint32_t handle, uint32_t width)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (handle >= CROSSTALK_EDA_LOGIC_STREAM_MAX) { return CROSSTALK_EDA_LOGIC_STREAM_ERR_HANDLE; }

    if (width == 0 || width > (uint32_t)LDG_BYTE_BITS * (uint32_t)CROSSTALK_EDA_LOGIC_STREAM_SAMPLE_BYTES) { return LDG_ERR_FUNC_ARG_INVALID; }

    ctx->stream[handle].width = width;

    return LDG_ERR_AOK;
}

static uint16_t stream_sample_norm_get(uint64_t val, uint32_t width)
{
    uint64_t mid = (uint64_t)CROSSTALK_EDA_LOGIC_STREAM_LSB << (width - (uint32_t)CROSSTALK_EDA_LOGIC_STREAM_LSB);
    uint64_t mask = ((uint64_t)CROSSTALK_EDA_LOGIC_STREAM_LSB << width) - (uint64_t)CROSSTALK_EDA_LOGIC_STREAM_LSB;
    uint64_t centered = (val ^ mid) & mask;

    return (uint16_t)(centered << ((uint32_t)CROSSTALK_EDA_LOGIC_STREAM_SAMPLE_BITS - width));
}

uint32_t crosstalk_eda_logic_stream_pump(crosstalk_eda_logic_stream_ctx_t *ctx, crosstalk_eda_logic_probe_ctx_t *probes)
{
    uint32_t idx = 0;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(ctx == 0x0 || probes == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    for (idx = 0; idx < CROSSTALK_EDA_LOGIC_STREAM_MAX; idx = idx + 0x1)
    {
        crosstalk_eda_logic_stream_t *stream = &ctx->stream[idx];
        uint32_t cunt = 0;

        if (stream->live != LDG_TRUTH_TRUE) { continue; }

        if (crosstalk_eda_logic_probe_live_is(probes, stream->probe_handle) != LDG_TRUTH_TRUE) { continue; }

        while (LDG_TRUTH_TRUE)
        {
            uint32_t pos = 0;

            ret = crosstalk_eda_logic_probe_drain(probes, stream->probe_handle, ctx->drain, (uint32_t)CROSSTALK_EDA_LOGIC_STREAM_DRAIN_CAP, &cunt);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (cunt == 0x0) { break; }

            if (stream->decode == (uint32_t)CROSSTALK_EDA_LOGIC_STREAM_DECODE_LEVEL)
            {
                stream->level_val = ctx->drain[cunt - 0x1].val;
                stream->level_cycle = ctx->drain[cunt - 0x1].cycle;
                continue;
            }

            if (stream->decode == (uint32_t)CROSSTALK_EDA_LOGIC_STREAM_DECODE_SAMPLE)
            {
                uint32_t width = stream->width != 0 ? stream->width : (uint32_t)CROSSTALK_EDA_LOGIC_STREAM_SAMPLE_BITS;

                for (pos = 0; pos < cunt; pos = pos + 0x1)
                {
                    uint32_t at = pos * (uint32_t)CROSSTALK_EDA_LOGIC_STREAM_SAMPLE_BYTES;
                    uint16_t norm = stream_sample_norm_get(ctx->drain[pos].val, width);

                    ctx->byte[at] = (uint8_t)(norm & (uint16_t)CROSSTALK_EDA_LOGIC_STREAM_BYTE_MASK);
                    ctx->byte[at + 0x1] = (uint8_t)((norm >> (uint16_t)LDG_BYTE_BITS) & (uint16_t)CROSSTALK_EDA_LOGIC_STREAM_BYTE_MASK);
                }
            }
            else { for (pos = 0; pos < cunt; pos = pos + 0x1) { ctx->byte[pos] = (uint8_t)(ctx->drain[pos].val & (uint64_t)CROSSTALK_EDA_LOGIC_STREAM_BYTE_MASK); } }

            stream->level_val = ctx->drain[cunt - 0x1].val;
            stream->level_cycle = ctx->drain[cunt - 0x1].cycle;

            ret = stream_sinks_feed(stream, ctx->byte, stream->decode == (uint32_t)CROSSTALK_EDA_LOGIC_STREAM_DECODE_SAMPLE ? cunt * (uint32_t)CROSSTALK_EDA_LOGIC_STREAM_SAMPLE_BYTES : cunt);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (cunt < (uint32_t)CROSSTALK_EDA_LOGIC_STREAM_DRAIN_CAP) { break; }
        }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_logic_stream_level_get(const crosstalk_eda_logic_stream_ctx_t *ctx, uint32_t handle, uint64_t *out_val, uint64_t *out_cycle)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_val == 0x0 || out_cycle == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_val = 0x0;
    *out_cycle = 0x0;

    if (handle >= CROSSTALK_EDA_LOGIC_STREAM_MAX) { return CROSSTALK_EDA_LOGIC_STREAM_ERR_HANDLE; }

    if (ctx->stream[handle].live != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    *out_val = ctx->stream[handle].level_val;
    *out_cycle = ctx->stream[handle].level_cycle;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_logic_stream_bytes_total_get(const crosstalk_eda_logic_stream_ctx_t *ctx, uint32_t handle, uint64_t *out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (handle >= CROSSTALK_EDA_LOGIC_STREAM_MAX) { return CROSSTALK_EDA_LOGIC_STREAM_ERR_HANDLE; }

    *out = ctx->stream[handle].bytes_total;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_logic_stream_live_is(const crosstalk_eda_logic_stream_ctx_t *ctx, uint32_t handle)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (handle >= CROSSTALK_EDA_LOGIC_STREAM_MAX) { return LDG_ERR_FUNC_ARG_INVALID; }

    return ctx->stream[handle].live == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}
