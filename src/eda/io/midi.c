#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/core/bits.h>
#include <dangling/io/file.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "eda/io/midi.h"

#define MIDI_STEP 1
#define MIDI_BIT_MASK 0x1
#define MIDI_DATA_CUNT_VOICE 2
#define MIDI_DATA_CUNT_PROG 1
#define MIDI_REALTIME_LEN 1
#define MIDI_DATA_NONE 0
#define MIDI_PORT_FMT "%.63s"
#define MIDI_NAME_FMT "%.63s"
#define MIDI_PATH_FMT "%.511s"
#define MIDI_TAG_LEN 4
#define MIDI_LEN_BYTES 4
#define MIDI_WORD_BYTES 2
#define MIDI_FMT_OFF 8
#define MIDI_DIV_OFF 12
#define MIDI_TAG_HDR "MThd"
#define MIDI_TAG_TRACK "MTrk"

static uint32_t midi_msg_wire_get(const ldg_io_midi_msg_t *msg, uint8_t *out_bytes, uint32_t *out_len)
{
    uint32_t data_cunt = 0;

    if (LDG_UNLIKELY(msg == 0x0 || out_bytes == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (msg->kind < (uint8_t)LDG_IO_MIDI_STATUS_MIN) { return LDG_ERR_AOK; }

    if (msg->kind >= (uint8_t)LDG_IO_MIDI_REALTIME_MIN)
    {
        out_bytes[0] = msg->kind;
        *out_len = (uint32_t)MIDI_REALTIME_LEN;

        return LDG_ERR_AOK;
    }

    if (msg->kind >= (uint8_t)LDG_IO_MIDI_SYS_MIN) { return LDG_ERR_AOK; }

    if (msg->kind == (uint8_t)LDG_IO_MIDI_MSG_KIND_PROG_CHANGE || msg->kind == (uint8_t)LDG_IO_MIDI_MSG_KIND_AFTERTOUCH_CHAN) { data_cunt = (uint32_t)MIDI_DATA_CUNT_PROG; }
    else { data_cunt = (uint32_t)MIDI_DATA_CUNT_VOICE; }

    out_bytes[0] = (uint8_t)((msg->kind & (uint8_t)LDG_IO_MIDI_STATUS_MASK) | (msg->channel & (uint8_t)LDG_IO_MIDI_CHANNEL_MASK));
    out_bytes[1] = (uint8_t)(msg->data[0] & (uint8_t)LDG_IO_MIDI_DATA_MASK);
    out_bytes[2] = (uint8_t)(msg->data[1] & (uint8_t)LDG_IO_MIDI_DATA_MASK);

    *out_len = data_cunt + (uint32_t)MIDI_STEP;

    return LDG_ERR_AOK;
}

static uint32_t midi_byte_push(crosstalk_eda_io_midi_ctx_t *ctx, uint8_t val)
{
    uint32_t next = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    next = (ctx->wr_at + (uint32_t)MIDI_STEP) & (uint32_t)CROSSTALK_EDA_IO_MIDI_QUEUE_MASK;

    if (next == ctx->rd_at) { ctx->dropped = ctx->dropped + (uint64_t)MIDI_STEP; return CROSSTALK_EDA_IO_MIDI_ERR_FULL; }

    ctx->queue[ctx->wr_at] = val;
    ctx->wr_at = next;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_midi_init(crosstalk_eda_io_midi_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->dev.handle = (uint64_t)LDG_IO_MIDI_HANDLE_NONE;
    ctx->dev_idx = (uint64_t)UINT64_MAX;
    ctx->pin.sig_idx = (uint32_t)UINT32_MAX;
    ctx->pin.abi_sig = (uint32_t)UINT32_MAX;
    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_midi_shutdown(crosstalk_eda_io_midi_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->dev_open == LDG_TRUTH_TRUE)
    {
        ret = ldg_io_midi_in_close(&ctx->dev);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "midi: close failed; err: %u32", ret); }
    }

    ctx->dev_open = LDG_TRUTH_FALSE;
    ctx->live = LDG_TRUTH_FALSE;
    ctx->ok = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_midi_ports_refresh(crosstalk_eda_io_midi_ctx_t *ctx)
{
    uint64_t got = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ret = ldg_io_midi_in_port_list(ctx->port, (uint64_t)LDG_IO_MIDI_PORT_MAX, &got);
    if (ret != LDG_ERR_AOK) { ctx->port_cunt = 0; return CROSSTALK_EDA_IO_MIDI_ERR_DEV; }

    ctx->port_cunt = (uint32_t)got;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_midi_port_cunt_get(const crosstalk_eda_io_midi_ctx_t *ctx, uint32_t *out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = ctx->port_cunt;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_midi_port_name_get(const crosstalk_eda_io_midi_ctx_t *ctx, uint32_t idx, const char **out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (idx >= ctx->port_cunt) { return LDG_ERR_BOUNDS; }

    *out = ctx->port[idx].name;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_midi_dev_name_get(const crosstalk_eda_io_midi_ctx_t *ctx, const char **out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = ctx->dev_name;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_midi_dev_close(crosstalk_eda_io_midi_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->dev_open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = ldg_io_midi_in_close(&ctx->dev);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "midi: close failed; err: %u32", ret); }

    ctx->dev_open = LDG_TRUTH_FALSE;
    ctx->dev_idx = (uint64_t)UINT64_MAX;
    ctx->dev_name[0] = LDG_STR_TERM;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "midi: closed");

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_midi_dev_open(crosstalk_eda_io_midi_ctx_t *ctx, uint32_t idx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (idx >= ctx->port_cunt) { return LDG_ERR_BOUNDS; }

    ret = crosstalk_eda_io_midi_dev_close(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_midi_in_open((uint64_t)idx, &ctx->dev);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "midi: open failed; idx: %u32; name: %.63s; err: %u32", idx, ctx->port[idx].name, ret);
        return CROSSTALK_EDA_IO_MIDI_ERR_DEV;
    }

    if (snprintf(ctx->dev_name, sizeof(ctx->dev_name), MIDI_NAME_FMT, ctx->port[idx].name) < 0) { return LDG_ERR_IO_FMT; }

    ctx->dev_idx = (uint64_t)idx;
    ctx->dev_open = LDG_TRUTH_TRUE;
    ctx->busted = LDG_TRUTH_FALSE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "midi: opened; idx: %u32; name: %.63s", idx, ctx->dev_name);

    return LDG_ERR_AOK;
}

static uint32_t midi_dev_resolve(crosstalk_eda_io_midi_ctx_t *ctx)
{
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->dev_open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (ctx->dev_idx < (uint64_t)ctx->port_cunt && strncmp(ctx->port[ctx->dev_idx].name, ctx->dev_name, sizeof(ctx->dev_name)) == 0) { return LDG_ERR_AOK; }

    for (idx = 0; idx < ctx->port_cunt; idx = idx + (uint32_t)MIDI_STEP)
    {
        if (strncmp(ctx->port[idx].name, ctx->dev_name, sizeof(ctx->dev_name)) != 0) { continue; }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "midi: device moved; name: %.63s; was: %u64; now: %u32", ctx->dev_name, ctx->dev_idx, idx);

        ret = ldg_io_midi_in_close(&ctx->dev);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "midi: close failed; err: %u32", ret); }

        ret = ldg_io_midi_in_open((uint64_t)idx, &ctx->dev);
        if (ret != LDG_ERR_AOK)
        {
            ctx->dev_open = LDG_TRUTH_FALSE;
            ctx->busted = LDG_TRUTH_TRUE;
            return CROSSTALK_EDA_IO_MIDI_ERR_DEV;
        }

        ctx->dev_idx = (uint64_t)idx;

        return LDG_ERR_AOK;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "midi: device gone; name: %.63s", ctx->dev_name);

    ret = ldg_io_midi_in_close(&ctx->dev);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "midi: close failed; err: %u32", ret); }

    ctx->dev_open = LDG_TRUTH_FALSE;
    ctx->dev_idx = (uint64_t)UINT64_MAX;
    ctx->busted = LDG_TRUTH_TRUE;

    return CROSSTALK_EDA_IO_MIDI_ERR_BUSTED;
}

uint32_t crosstalk_eda_io_midi_attach(crosstalk_eda_io_midi_ctx_t *ctx, uint32_t slot_idx, const crosstalk_hdl_exec_ctx_t *exec, const char *pin, uint64_t freq)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || pin == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(freq == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ctx->live = LDG_TRUTH_FALSE;
    ctx->pin.port[0] = LDG_STR_TERM;
    ctx->pin.sig_idx = (uint32_t)UINT32_MAX;
    ctx->pin.abi_sig = (uint32_t)UINT32_MAX;
    ctx->pin.bound = LDG_TRUTH_FALSE;

    if (snprintf(ctx->pin.port, sizeof(ctx->pin.port), MIDI_PORT_FMT, pin) < 0) { return LDG_ERR_IO_FMT; }

    if (exec != 0x0)
    {
        ret = crosstalk_hdl_exec_sig_find(exec, pin, &ctx->pin.sig_idx);
        if (ret != LDG_ERR_AOK) { return CROSSTALK_EDA_IO_MIDI_ERR_PORT; }

        ctx->pin.bound = LDG_TRUTH_TRUE;
    }

    ctx->slot_idx = slot_idx;
    ctx->freq = freq;
    ctx->phase = 0;
    ctx->frame = 0;
    ctx->bit_at = 0;
    ctx->sending = LDG_TRUTH_FALSE;
    ctx->line = (uint8_t)CROSSTALK_EDA_IO_MIDI_PIN_LOW;
    ctx->wr_at = 0;
    ctx->rd_at = 0;
    ctx->live = LDG_TRUTH_TRUE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "midi: attached; slot: %u32; pin: %.63s; clk: %u64; baud: %u32", slot_idx, ctx->pin.port, freq, (uint32_t)CROSSTALK_EDA_IO_MIDI_BAUD);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_midi_detach(crosstalk_eda_io_midi_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->live = LDG_TRUTH_FALSE;
    ctx->sending = LDG_TRUTH_FALSE;
    ctx->wr_at = 0;
    ctx->rd_at = 0;
    ctx->pin.abi_sig = (uint32_t)UINT32_MAX;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_midi_slot_detach(crosstalk_eda_io_midi_ctx_t *ctx, uint32_t slot_idx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (ctx->slot_idx != slot_idx) { return LDG_ERR_AOK; }

    return crosstalk_eda_io_midi_detach(ctx);
}

uint32_t crosstalk_eda_io_midi_bytes_put(crosstalk_eda_io_midi_ctx_t *ctx, const uint8_t *bytes, uint32_t len)
{
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || bytes == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_IO_MIDI_ERR_LIVE; }

    for (at = 0; at < len; at = at + (uint32_t)MIDI_STEP)
    {
        ret = midi_byte_push(ctx, bytes[at]);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_midi_msg_put(crosstalk_eda_io_midi_ctx_t *ctx, const ldg_io_midi_msg_t *msg)
{
    uint8_t wire[CROSSTALK_EDA_IO_MIDI_WIRE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || msg == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = midi_msg_wire_get(msg, wire, &len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (len == 0) { return LDG_ERR_AOK; }

    ret = crosstalk_eda_io_midi_bytes_put(ctx, wire, len);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->msgs_total = ctx->msgs_total + (uint64_t)MIDI_STEP;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_midi_note_put(crosstalk_eda_io_midi_ctx_t *ctx, uint8_t note, uint8_t vel)
{
    uint8_t wire[CROSSTALK_EDA_IO_MIDI_WIRE_CAP] = LDG_ARR_ZERO_INIT;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    wire[0] = (uint8_t)CROSSTALK_EDA_IO_MIDI_NOTE_ON;
    wire[1] = (uint8_t)(note & (uint8_t)LDG_IO_MIDI_DATA_MASK);
    wire[2] = (uint8_t)(vel & (uint8_t)LDG_IO_MIDI_DATA_MASK);

    return crosstalk_eda_io_midi_bytes_put(ctx, wire, (uint32_t)CROSSTALK_EDA_IO_MIDI_WIRE_CAP);
}

uint32_t crosstalk_eda_io_midi_quiet_put(crosstalk_eda_io_midi_ctx_t *ctx)
{
    uint8_t wire[CROSSTALK_EDA_IO_MIDI_WIRE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t chan = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    for (chan = 0; chan < (uint32_t)LDG_IO_MIDI_CHANNEL_CUNT; chan = chan + (uint32_t)MIDI_STEP)
    {
        wire[0] = (uint8_t)((uint32_t)CROSSTALK_EDA_IO_MIDI_CC_STATUS | chan);
        wire[1] = (uint8_t)CROSSTALK_EDA_IO_MIDI_CC_ALL_OFF;
        wire[2] = 0x0;

        ret = crosstalk_eda_io_midi_bytes_put(ctx, wire, (uint32_t)CROSSTALK_EDA_IO_MIDI_WIRE_CAP);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t midi_field_get(const uint8_t *at, uint32_t cunt)
{
    uint32_t out = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(at == 0x0)) { return 0x0; }

    for (idx = 0; idx < cunt; idx = idx + (uint32_t)MIDI_STEP) { out = (out << (uint32_t)LDG_BYTE_BITS) | (uint32_t)at[idx]; }

    return out;
}

static uint32_t midi_vlq_get(const crosstalk_eda_io_midi_ctx_t *ctx, uint32_t *at, uint32_t end, uint64_t *out)
{
    uint8_t byte = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || at == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    while (*at < end)
    {
        byte = ctx->file[*at];
        *at = *at + (uint32_t)MIDI_STEP;
        *out = (*out << (uint64_t)CROSSTALK_EDA_IO_MIDI_VLQ_SHIFT) | (uint64_t)(byte & (uint8_t)CROSSTALK_EDA_IO_MIDI_VLQ_MASK);

        if ((byte & (uint8_t)CROSSTALK_EDA_IO_MIDI_VLQ_CONT) == 0) { return LDG_ERR_AOK; }
    }

    return LDG_ERR_EOF;
}

static uint32_t midi_track_delta_rd(crosstalk_eda_io_midi_ctx_t *ctx, uint32_t idx)
{
    crosstalk_eda_io_midi_track_t *track = 0x0;
    uint64_t delta = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    track = &ctx->track[idx];

    if (track->at >= track->end) { track->done = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

    if (midi_vlq_get(ctx, &track->at, track->end, &delta) != LDG_ERR_AOK) { track->done = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

    track->tick = track->tick + delta;

    return LDG_ERR_AOK;
}

static uint32_t midi_track_fire(crosstalk_eda_io_midi_ctx_t *ctx, uint32_t idx)
{
    uint8_t wire[CROSSTALK_EDA_IO_MIDI_WIRE_CAP] = LDG_ARR_ZERO_INIT;
    crosstalk_eda_io_midi_track_t *track = 0x0;
    uint64_t len = 0;
    uint32_t data_cunt = 0;
    uint8_t status = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    track = &ctx->track[idx];

    if (track->at >= track->end) { track->done = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

    status = ctx->file[track->at];

    if (status >= (uint8_t)LDG_IO_MIDI_STATUS_MIN) { track->at = track->at + (uint32_t)MIDI_STEP; track->status = status; }
    else { status = track->status; }

    if (status == (uint8_t)CROSSTALK_EDA_IO_MIDI_META)
    {
        uint8_t kind = 0;

        if (track->at >= track->end) { track->done = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

        kind = ctx->file[track->at];
        track->at = track->at + (uint32_t)MIDI_STEP;
        track->status = 0x0;

        if (midi_vlq_get(ctx, &track->at, track->end, &len) != LDG_ERR_AOK) { track->done = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

        if (kind == (uint8_t)CROSSTALK_EDA_IO_MIDI_META_TEMPO && len == (uint64_t)CROSSTALK_EDA_IO_MIDI_TEMPO_BYTES && track->at + (uint32_t)CROSSTALK_EDA_IO_MIDI_TEMPO_BYTES <= track->end)
        {
            ctx->tempo = (uint64_t)midi_field_get(&ctx->file[track->at], (uint32_t)CROSSTALK_EDA_IO_MIDI_TEMPO_BYTES);
            ctx->tick_span = ctx->freq * ctx->tempo;
        }

        if (kind == (uint8_t)CROSSTALK_EDA_IO_MIDI_META_END) { track->done = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

        track->at = track->at + (uint32_t)len;

        return midi_track_delta_rd(ctx, idx);
    }

    if (status == (uint8_t)CROSSTALK_EDA_IO_MIDI_SYSEX || status == (uint8_t)CROSSTALK_EDA_IO_MIDI_SYSEX_ESC)
    {
        track->status = 0x0;

        if (midi_vlq_get(ctx, &track->at, track->end, &len) != LDG_ERR_AOK) { track->done = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

        track->at = track->at + (uint32_t)len;

        return midi_track_delta_rd(ctx, idx);
    }

    if (status < (uint8_t)LDG_IO_MIDI_STATUS_MIN) { track->done = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

    if ((status & (uint8_t)LDG_IO_MIDI_STATUS_MASK) == (uint8_t)LDG_IO_MIDI_MSG_KIND_PROG_CHANGE || (status & (uint8_t)LDG_IO_MIDI_STATUS_MASK) == (uint8_t)LDG_IO_MIDI_MSG_KIND_AFTERTOUCH_CHAN) { data_cunt = (uint32_t)MIDI_DATA_CUNT_PROG; }
    else { data_cunt = (uint32_t)MIDI_DATA_CUNT_VOICE; }

    if (track->at + data_cunt > track->end) { track->done = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

    wire[0] = status;
    wire[1] = ctx->file[track->at];
    wire[2] = data_cunt == (uint32_t)MIDI_DATA_CUNT_VOICE ? ctx->file[track->at + (uint32_t)MIDI_STEP] : (uint8_t)0x0;

    track->at = track->at + data_cunt;

    if (crosstalk_eda_io_midi_bytes_put(ctx, wire, data_cunt + (uint32_t)MIDI_STEP) != LDG_ERR_AOK) { ctx->dropped = ctx->dropped + (uint64_t)MIDI_STEP; }

    return midi_track_delta_rd(ctx, idx);
}

static uint32_t midi_tick_fire(crosstalk_eda_io_midi_ctx_t *ctx)
{
    uint32_t idx = 0;
    uint8_t more = LDG_TRUTH_TRUE;
    uint8_t alive = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (more == LDG_TRUTH_TRUE)
    {
        more = LDG_TRUTH_FALSE;

        for (idx = 0; idx < ctx->track_cunt; idx = idx + (uint32_t)MIDI_STEP)
        {
            if (ctx->track[idx].done == LDG_TRUTH_TRUE) { continue; }

            if (ctx->track[idx].tick > ctx->play_tick) { continue; }

            if (midi_track_fire(ctx, idx) != LDG_ERR_AOK) { ctx->track[idx].done = LDG_TRUTH_TRUE; }

            more = LDG_TRUTH_TRUE;
        }
    }

    for (idx = 0; idx < ctx->track_cunt; idx = idx + (uint32_t)MIDI_STEP) { if (ctx->track[idx].done != LDG_TRUTH_TRUE) { alive = LDG_TRUTH_TRUE; } }

    if (alive != LDG_TRUTH_TRUE)
    {
        ctx->playing = LDG_TRUTH_FALSE;

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "midi: file done; tick: %u64; path: %.255s", ctx->play_tick, ctx->file_path);

        return crosstalk_eda_io_midi_quiet_put(ctx);
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_midi_file_stop(crosstalk_eda_io_midi_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->playing != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ctx->playing = LDG_TRUTH_FALSE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "midi: file stopped; tick: %u64", ctx->play_tick);

    return crosstalk_eda_io_midi_quiet_put(ctx);
}

uint8_t crosstalk_eda_io_midi_playing_is(const crosstalk_eda_io_midi_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->playing == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_eda_io_midi_file_play(crosstalk_eda_io_midi_ctx_t *ctx, const char *path)
{
    ldg_io_file_t *fh = 0x0;
    uint64_t got = 0;
    uint32_t at = 0;
    uint32_t fmt = 0;
    uint32_t want = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ret = crosstalk_eda_io_midi_file_stop(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, 0x0, &fh);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_rd(fh, ctx->file, (uint64_t)CROSSTALK_EDA_IO_MIDI_FILE_CAP, &got);

    if (ldg_io_file_close(fh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "midi file close failed"); }

    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->file_len = (uint32_t)got;

    if (ctx->file_len < (uint32_t)CROSSTALK_EDA_IO_MIDI_HDR_LEN) { return LDG_ERR_INVALID; }

    if (memcmp(ctx->file, MIDI_TAG_HDR, (uint64_t)MIDI_TAG_LEN) != 0) { return LDG_ERR_INVALID; }

    fmt = midi_field_get(&ctx->file[MIDI_FMT_OFF], (uint32_t)MIDI_WORD_BYTES);
    ctx->tick_div = midi_field_get(&ctx->file[MIDI_DIV_OFF], (uint32_t)MIDI_WORD_BYTES);

    if (fmt > (uint32_t)CROSSTALK_EDA_IO_MIDI_FMT_MAX) { return LDG_ERR_UNSUPPORTED; }

    if ((ctx->tick_div & (uint32_t)CROSSTALK_EDA_IO_MIDI_SMPTE_BIT) != 0 || ctx->tick_div == 0) { return LDG_ERR_UNSUPPORTED; }

    ctx->track_cunt = 0;
    at = (uint32_t)CROSSTALK_EDA_IO_MIDI_HDR_LEN;

    while (at + (uint32_t)CROSSTALK_EDA_IO_MIDI_CHUNK_HDR <= ctx->file_len && ctx->track_cunt < (uint32_t)CROSSTALK_EDA_IO_MIDI_TRACK_MAX)
    {
        want = midi_field_get(&ctx->file[at + (uint32_t)MIDI_TAG_LEN], (uint32_t)MIDI_LEN_BYTES);

        if (memcmp(&ctx->file[at], MIDI_TAG_TRACK, (uint64_t)MIDI_TAG_LEN) != 0)
        {
            at = at + (uint32_t)CROSSTALK_EDA_IO_MIDI_CHUNK_HDR + want;
            continue;
        }

        at = at + (uint32_t)CROSSTALK_EDA_IO_MIDI_CHUNK_HDR;

        ctx->track[ctx->track_cunt].at = at;
        ctx->track[ctx->track_cunt].end = at + want > ctx->file_len ? ctx->file_len : at + want;
        ctx->track[ctx->track_cunt].tick = 0;
        ctx->track[ctx->track_cunt].status = 0x0;
        ctx->track[ctx->track_cunt].done = LDG_TRUTH_FALSE;

        ret = midi_track_delta_rd(ctx, ctx->track_cunt);
        if (ret != LDG_ERR_AOK) { return ret; }

        ctx->track_cunt = ctx->track_cunt + (uint32_t)MIDI_STEP;
        at = at + want;
    }

    if (ctx->track_cunt == 0) { return LDG_ERR_EMPTY; }

    if (snprintf(ctx->file_path, sizeof(ctx->file_path), MIDI_PATH_FMT, path) < 0) { return LDG_ERR_IO_FMT; }

    ctx->tempo = (uint64_t)CROSSTALK_EDA_IO_MIDI_TEMPO_DEFAULT;
    ctx->tick_rate = (uint64_t)ctx->tick_div * (uint64_t)CROSSTALK_EDA_IO_MIDI_USEC_PER_SEC;
    ctx->tick_span = ctx->freq * ctx->tempo;
    ctx->play_tick = 0;
    ctx->play_phase = 0;
    ctx->playing = LDG_TRUTH_TRUE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "midi: file; bytes: %u32; fmt: %u32; tracks: %u32; per quarter: %u32; path: %.255s", ctx->file_len, fmt, ctx->track_cunt, ctx->tick_div, path);

    return midi_tick_fire(ctx);
}

uint32_t crosstalk_eda_io_midi_pump(crosstalk_eda_io_midi_ctx_t *ctx)
{
    uint64_t got = 0;
    uint64_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (ctx->dev_open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = ldg_io_midi_in_poll(&ctx->dev, ctx->msg, (uint64_t)CROSSTALK_EDA_IO_MIDI_MSG_CAP, &got);

    if (ret == LDG_ERR_WOULD_BLOCK || ret == LDG_ERR_AGAIN) { return LDG_ERR_AOK; }

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "midi: poll failed; err: %u32", ret);
        return CROSSTALK_EDA_IO_MIDI_ERR_DEV;
    }

    if (ctx->live != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    for (at = 0; at < got; at = at + (uint64_t)MIDI_STEP)
    {
        ret = crosstalk_eda_io_midi_msg_put(ctx, &ctx->msg[at]);
        if (ret != LDG_ERR_AOK) { break; }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_midi_tick(crosstalk_eda_io_midi_ctx_t *ctx, double dt)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ctx->refresh_acc = ctx->refresh_acc + dt;

    if (ctx->refresh_acc < (double)CROSSTALK_EDA_IO_MIDI_REFRESH_SECS) { return LDG_ERR_AOK; }

    ctx->refresh_acc = 0.0;

    ret = crosstalk_eda_io_midi_ports_refresh(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    return midi_dev_resolve(ctx);
}

static uint32_t midi_line_set(crosstalk_eda_io_midi_ctx_t *ctx, uint8_t bit, crosstalk_hdl_exec_ctx_t *exec, const crosstalk_hdl_abi_t *abi, uint8_t packed)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->line == bit) { return LDG_ERR_AOK; }

    if (packed == LDG_TRUTH_TRUE) { abi->poke(ctx->pin.abi_sig, (uint64_t)bit); }
    else if (crosstalk_hdl_exec_poke(exec, ctx->pin.sig_idx, (uint64_t)bit) != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    ctx->line = bit;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_midi_cycle_drive(crosstalk_eda_io_midi_ctx_t *ctx, uint32_t slot_idx, crosstalk_hdl_exec_ctx_t *exec, const crosstalk_hdl_abi_t *abi)
{
    uint8_t packed = LDG_TRUTH_FALSE;
    uint8_t bit = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (ctx->slot_idx != slot_idx) { return LDG_ERR_AOK; }

    if (ctx->freq == 0) { return LDG_ERR_AOK; }

    if (abi != 0x0 && abi->ok == LDG_TRUTH_TRUE) { packed = LDG_TRUTH_TRUE; }

    if (packed != LDG_TRUTH_TRUE && exec == 0x0) { return LDG_ERR_FUNC_ARG_NULL; }

    if (packed == LDG_TRUTH_TRUE)
    {
        if (ctx->pin.abi_sig == (uint32_t)UINT32_MAX) { ctx->pin.abi_sig = abi->sig_find(ctx->pin.port); }

        if (ctx->pin.abi_sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { return LDG_ERR_AOK; }
    }

    if (ctx->playing == LDG_TRUTH_TRUE && ctx->tick_span != 0)
    {
        ctx->play_phase = ctx->play_phase + ctx->tick_rate;

        while (ctx->play_phase >= ctx->tick_span && ctx->playing == LDG_TRUTH_TRUE)
        {
            ctx->play_phase = ctx->play_phase - ctx->tick_span;
            ctx->play_tick = ctx->play_tick + (uint64_t)MIDI_STEP;

            if (midi_tick_fire(ctx) != LDG_ERR_AOK) { break; }
        }
    }

    ctx->phase = ctx->phase + (uint64_t)CROSSTALK_EDA_IO_MIDI_BAUD;

    if (ctx->phase < ctx->freq) { return LDG_ERR_AOK; }

    ctx->phase = ctx->phase - ctx->freq;

    if (ctx->sending != LDG_TRUTH_TRUE)
    {
        if (ctx->rd_at == ctx->wr_at) { return midi_line_set(ctx, (uint8_t)CROSSTALK_EDA_IO_MIDI_PIN_HIGH, exec, abi, packed); }

        ctx->frame = ((uint32_t)ctx->queue[ctx->rd_at] << (uint32_t)CROSSTALK_EDA_IO_MIDI_DATA_SHIFT) | (uint32_t)CROSSTALK_EDA_IO_MIDI_STOP_BIT;
        ctx->bit_at = 0;
        ctx->sending = LDG_TRUTH_TRUE;
    }

    bit = (uint8_t)((ctx->frame >> ctx->bit_at) & (uint32_t)MIDI_BIT_MASK);

    ret = midi_line_set(ctx, bit, exec, abi, packed);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->bit_at = ctx->bit_at + (uint32_t)MIDI_STEP;

    if (ctx->bit_at < (uint32_t)CROSSTALK_EDA_IO_MIDI_FRAME_BITS) { return LDG_ERR_AOK; }

    ctx->sending = LDG_TRUTH_FALSE;
    ctx->rd_at = (ctx->rd_at + (uint32_t)MIDI_STEP) & (uint32_t)CROSSTALK_EDA_IO_MIDI_QUEUE_MASK;
    ctx->bytes_total = ctx->bytes_total + (uint64_t)MIDI_STEP;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_midi_pending_get(const crosstalk_eda_io_midi_ctx_t *ctx, uint32_t *out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (ctx->wr_at - ctx->rd_at) & (uint32_t)CROSSTALK_EDA_IO_MIDI_QUEUE_MASK;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_io_midi_live_is(const crosstalk_eda_io_midi_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->live == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint8_t crosstalk_eda_io_midi_dev_open_is(const crosstalk_eda_io_midi_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->dev_open == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}
