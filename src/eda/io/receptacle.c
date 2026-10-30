#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/core/bits.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "eda/io/receptacle.h"
#include "eda/io/scancode.h"

#define RECEPTACLE_STEP 1
#define RECEPTACLE_PIN_LOW 0
#define RECEPTACLE_PIN_HIGH 1
#define RECEPTACLE_HELD_NONE 0
#define RECEPTACLE_PORT_FMT "%.63s"

static uint32_t receptacle_pin_bind(crosstalk_eda_io_receptacle_ctx_t *ctx, uint32_t pin_idx, const crosstalk_hdl_exec_ctx_t *exec, const char *port)
{
    crosstalk_eda_io_receptacle_pin_t *pin = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    pin = &ctx->pin[pin_idx];

    pin->port[0] = LDG_STR_TERM;
    pin->sig_idx = UINT32_MAX;
    pin->abi_sig = UINT32_MAX;
    pin->abi_ck = LDG_TRUTH_FALSE;
    pin->bound = LDG_TRUTH_FALSE;

    if (port == 0x0 || port[0] == LDG_STR_TERM) { return LDG_ERR_AOK; }

    if (snprintf(pin->port, sizeof(pin->port), RECEPTACLE_PORT_FMT, port) < 0) { return LDG_ERR_IO_FMT; }

    if (exec == 0x0) { return LDG_ERR_AOK; }

    ret = crosstalk_hdl_exec_sig_find(exec, port, &pin->sig_idx);
    if (ret != LDG_ERR_AOK) { return CROSSTALK_EDA_IO_RECEPTACLE_ERR_PORT; }

    pin->bound = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_receptacle_init(crosstalk_eda_io_receptacle_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_receptacle_attach(crosstalk_eda_io_receptacle_ctx_t *ctx, uint32_t slot_idx, const crosstalk_hdl_exec_ctx_t *exec, const char *data, const char *rdy, const char *ack, uint32_t mode)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || data == 0x0 || rdy == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(mode > (uint32_t)CROSSTALK_EDA_IO_RECEPTACLE_MODE_SCAN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ctx->live = LDG_TRUTH_FALSE;

    ret = receptacle_pin_bind(ctx, (uint32_t)CROSSTALK_EDA_IO_RECEPTACLE_PIN_DATA, exec, data);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = receptacle_pin_bind(ctx, (uint32_t)CROSSTALK_EDA_IO_RECEPTACLE_PIN_RDY, exec, rdy);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = receptacle_pin_bind(ctx, (uint32_t)CROSSTALK_EDA_IO_RECEPTACLE_PIN_ACK, exec, ack);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->ack_has = ctx->pin[CROSSTALK_EDA_IO_RECEPTACLE_PIN_ACK].port[0] != LDG_STR_TERM ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    ctx->slot_idx = slot_idx;
    ctx->state = (uint32_t)CROSSTALK_EDA_IO_RECEPTACLE_STATE_IDLE;
    ctx->held = RECEPTACLE_HELD_NONE;
    ctx->mode = mode;
    ctx->wr_at = 0;
    ctx->rd_at = 0;
    ctx->rdy_warned = LDG_TRUTH_FALSE;
    ctx->live = LDG_TRUTH_TRUE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "receptacle: attached; slot: %u32; data: %.63s; rdy: %.63s; ack: %.63s; mode: %s", slot_idx, data, rdy, ctx->ack_has == LDG_TRUTH_TRUE ? ack : "none", mode == (uint32_t)CROSSTALK_EDA_IO_RECEPTACLE_MODE_SCAN ? "scan" : "raw");

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_receptacle_detach(crosstalk_eda_io_receptacle_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->live = LDG_TRUTH_FALSE;
    ctx->state = (uint32_t)CROSSTALK_EDA_IO_RECEPTACLE_STATE_IDLE;
    ctx->wr_at = 0;
    ctx->rd_at = 0;

    return LDG_ERR_AOK;
}

static uint32_t receptacle_byte_push(crosstalk_eda_io_receptacle_ctx_t *ctx, uint8_t val)
{
    uint32_t next = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    next = (ctx->wr_at + (uint32_t)RECEPTACLE_STEP) & (uint32_t)CROSSTALK_EDA_IO_RECEPTACLE_QUEUE_MASK;

    if (next == ctx->rd_at) { ctx->dropped = ctx->dropped + (uint64_t)RECEPTACLE_STEP; return CROSSTALK_EDA_IO_RECEPTACLE_ERR_FULL; }

    ctx->queue[ctx->wr_at] = val;
    ctx->wr_at = next;

    return LDG_ERR_AOK;
}

static uint32_t receptacle_scan_put(crosstalk_eda_io_receptacle_ctx_t *ctx, uint8_t ch)
{
    uint8_t code = CROSSTALK_EDA_IO_SCANCODE_NONE;
    uint8_t shift = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_eda_io_scancode_map(ch, &code, &shift);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (code == (uint8_t)CROSSTALK_EDA_IO_SCANCODE_NONE) { return LDG_ERR_AOK; }

    if (shift == LDG_TRUTH_TRUE)
    {
        ret = receptacle_byte_push(ctx, (uint8_t)CROSSTALK_EDA_IO_SCANCODE_LSHIFT);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    ret = receptacle_byte_push(ctx, code);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = receptacle_byte_push(ctx, (uint8_t)(code | (uint8_t)CROSSTALK_EDA_IO_SCANCODE_BREAK));
    if (ret != LDG_ERR_AOK) { return ret; }

    if (shift == LDG_TRUTH_TRUE)
    {
        ret = receptacle_byte_push(ctx, (uint8_t)(CROSSTALK_EDA_IO_SCANCODE_LSHIFT | CROSSTALK_EDA_IO_SCANCODE_BREAK));
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_receptacle_scan_edge_put(crosstalk_eda_io_receptacle_ctx_t *ctx, uint8_t code, uint8_t down)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_IO_RECEPTACLE_ERR_LIVE; }

    if (code == (uint8_t)CROSSTALK_EDA_IO_SCANCODE_NONE) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (down == LDG_TRUTH_TRUE) { return receptacle_byte_push(ctx, code); }

    return receptacle_byte_push(ctx, (uint8_t)(code | (uint8_t)CROSSTALK_EDA_IO_SCANCODE_BREAK));
}

uint32_t crosstalk_eda_io_receptacle_put(crosstalk_eda_io_receptacle_ctx_t *ctx, const uint8_t *bytes, uint32_t len)
{
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || bytes == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_IO_RECEPTACLE_ERR_LIVE; }

    for (at = 0; at < len; at = at + (uint32_t)RECEPTACLE_STEP)
    {
        if (ctx->mode == (uint32_t)CROSSTALK_EDA_IO_RECEPTACLE_MODE_SCAN) { ret = receptacle_scan_put(ctx, bytes[at]); }
        else { ret = receptacle_byte_push(ctx, bytes[at]); }

        if (ret != LDG_ERR_AOK) { ctx->dropped = ctx->dropped + (uint64_t)(len - at); return ret; }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_receptacle_cycle_drive(crosstalk_eda_io_receptacle_ctx_t *ctx, uint32_t slot_idx, crosstalk_hdl_exec_ctx_t *exec, const crosstalk_hdl_abi_t *abi)
{
    crosstalk_eda_io_receptacle_pin_t *data_pin = 0x0;
    crosstalk_eda_io_receptacle_pin_t *rdy_pin = 0x0;
    crosstalk_eda_io_receptacle_pin_t *ack_pin = 0x0;
    uint64_t ack_val = 0;
    uint8_t packed = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (ctx->slot_idx != slot_idx) { return LDG_ERR_AOK; }

    if (abi != 0x0 && abi->ok == LDG_TRUTH_TRUE) { packed = LDG_TRUTH_TRUE; }

    if (packed != LDG_TRUTH_TRUE && exec == 0x0) { return LDG_ERR_FUNC_ARG_NULL; }

    data_pin = &ctx->pin[CROSSTALK_EDA_IO_RECEPTACLE_PIN_DATA];
    rdy_pin = &ctx->pin[CROSSTALK_EDA_IO_RECEPTACLE_PIN_RDY];
    ack_pin = &ctx->pin[CROSSTALK_EDA_IO_RECEPTACLE_PIN_ACK];

    if (packed == LDG_TRUTH_TRUE)
    {
        if (data_pin->abi_ck != LDG_TRUTH_TRUE) { data_pin->abi_sig = abi->sig_find(data_pin->port); data_pin->abi_ck = LDG_TRUTH_TRUE; }

        if (rdy_pin->abi_ck != LDG_TRUTH_TRUE) { rdy_pin->abi_sig = abi->sig_find(rdy_pin->port); rdy_pin->abi_ck = LDG_TRUTH_TRUE; }

        if (ctx->ack_has == LDG_TRUTH_TRUE && ack_pin->abi_ck != LDG_TRUTH_TRUE) { ack_pin->abi_sig = abi->sig_find(ack_pin->port); ack_pin->abi_ck = LDG_TRUTH_TRUE; }

        if (data_pin->abi_sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { return LDG_ERR_AOK; }

        if (rdy_pin->abi_sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE)
        {
            if (ctx->rd_at == ctx->wr_at) { return LDG_ERR_AOK; }

            if (ctx->rdy_warned != LDG_TRUTH_TRUE)
            {
                ctx->rdy_warned = LDG_TRUTH_TRUE;
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "receptacle: the design has no ready line; writing straight to the data port; data: %.63s; rdy: %.63s", data_pin->port, rdy_pin->port);
            }

            abi->poke(data_pin->abi_sig, (uint64_t)ctx->queue[ctx->rd_at]);

            ctx->rd_at = (ctx->rd_at + (uint32_t)RECEPTACLE_STEP) & (uint32_t)CROSSTALK_EDA_IO_RECEPTACLE_QUEUE_MASK;
            ctx->bytes_total = ctx->bytes_total + (uint64_t)RECEPTACLE_STEP;
            ctx->state = (uint32_t)CROSSTALK_EDA_IO_RECEPTACLE_STATE_IDLE;

            return LDG_ERR_AOK;
        }
    }

    if (ctx->state == (uint32_t)CROSSTALK_EDA_IO_RECEPTACLE_STATE_IDLE)
    {
        if (ctx->rd_at == ctx->wr_at) { return LDG_ERR_AOK; }

        if (packed == LDG_TRUTH_TRUE)
        {
            abi->poke(data_pin->abi_sig, (uint64_t)ctx->queue[ctx->rd_at]);
            abi->poke(rdy_pin->abi_sig, (uint64_t)RECEPTACLE_PIN_HIGH);
        }
        else
        {
            if (crosstalk_hdl_exec_poke(exec, data_pin->sig_idx, (uint64_t)ctx->queue[ctx->rd_at]) != LDG_ERR_AOK) { return LDG_ERR_AOK; }

            if (crosstalk_hdl_exec_poke(exec, rdy_pin->sig_idx, (uint64_t)RECEPTACLE_PIN_HIGH) != LDG_ERR_AOK) { return LDG_ERR_AOK; }
        }

        ctx->state = (uint32_t)CROSSTALK_EDA_IO_RECEPTACLE_STATE_HELD;
        ctx->held = RECEPTACLE_HELD_NONE;

        return LDG_ERR_AOK;
    }

    ctx->held = ctx->held + (uint32_t)RECEPTACLE_STEP;

    if (ctx->ack_has == LDG_TRUTH_TRUE) { if (packed == LDG_TRUTH_TRUE && ack_pin->abi_sig != (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { ack_val = abi->peek(ack_pin->abi_sig); }
        else if (packed != LDG_TRUTH_TRUE && ack_pin->bound == LDG_TRUTH_TRUE) { crosstalk_hdl_exec_peek(exec, ack_pin->sig_idx, &ack_val); } }

    if (ack_val == 0 && ctx->held < (uint32_t)CROSSTALK_EDA_IO_RECEPTACLE_HOLD_MAX) { return LDG_ERR_AOK; }

    if (packed == LDG_TRUTH_TRUE) { abi->poke(rdy_pin->abi_sig, (uint64_t)RECEPTACLE_PIN_LOW); }
    else if (crosstalk_hdl_exec_poke(exec, rdy_pin->sig_idx, (uint64_t)RECEPTACLE_PIN_LOW) != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    if (ack_val == 0) { ctx->dropped = ctx->dropped + (uint64_t)RECEPTACLE_STEP; }

    ctx->rd_at = (ctx->rd_at + (uint32_t)RECEPTACLE_STEP) & (uint32_t)CROSSTALK_EDA_IO_RECEPTACLE_QUEUE_MASK;
    ctx->bytes_total = ctx->bytes_total + (uint64_t)RECEPTACLE_STEP;
    ctx->state = (uint32_t)CROSSTALK_EDA_IO_RECEPTACLE_STATE_IDLE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_receptacle_pending_get(const crosstalk_eda_io_receptacle_ctx_t *ctx, uint32_t *out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (ctx->wr_at - ctx->rd_at) & (uint32_t)CROSSTALK_EDA_IO_RECEPTACLE_QUEUE_MASK;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_io_receptacle_live_is(const crosstalk_eda_io_receptacle_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->live == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint8_t crosstalk_eda_io_receptacle_idle_is(const crosstalk_eda_io_receptacle_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_TRUTH_TRUE; }

    if (ctx->state != (uint32_t)CROSSTALK_EDA_IO_RECEPTACLE_STATE_IDLE) { return LDG_TRUTH_FALSE; }

    return ctx->rd_at == ctx->wr_at ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}
