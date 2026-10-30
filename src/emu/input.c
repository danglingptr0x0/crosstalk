#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <dangling/logging/logging.h>

#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "core/log.h"
#include "eda/io/scancode.h"
#include "emu/input.h"

#define INPUT_STEP 1
#define INPUT_PIN_LOW 0
#define INPUT_PIN_HIGH 1
#define INPUT_HELD_NONE 0
#define INPUT_ASCII_BS 0x08
#define INPUT_SCAN_LCTRL 0x1D
#define INPUT_SCAN_LSHIFT 0x2A
#define INPUT_SCAN_RSHIFT 0x36
#define INPUT_SCAN_LALT 0x38
#define INPUT_SCAN_UP 0x48
#define INPUT_SCAN_LHS 0x4B
#define INPUT_SCAN_RHS 0x4D
#define INPUT_SCAN_DOWN 0x50
#define INPUT_KITTY_LSHIFT 57441
#define INPUT_KITTY_LCTRL 57442
#define INPUT_KITTY_LALT 57443
#define INPUT_KITTY_RSHIFT 57447

static uint32_t input_byte_push(crosstalk_emu_input_ctx_t *ctx, uint8_t val)
{
    uint32_t next = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    next = (ctx->wr_at + (uint32_t)INPUT_STEP) & (uint32_t)CROSSTALK_EMU_INPUT_QUEUE_MASK;

    if (next == ctx->rd_at)
    {
        ctx->dropped = ctx->dropped + (uint64_t)INPUT_STEP;

        return CROSSTALK_EMU_INPUT_ERR_FULL;
    }

    ctx->queue[ctx->wr_at] = val;
    ctx->wr_at = next;

    return LDG_ERR_AOK;
}

static uint32_t input_scan_put(crosstalk_emu_input_ctx_t *ctx, uint8_t ch)
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
        ret = input_byte_push(ctx, (uint8_t)CROSSTALK_EDA_IO_SCANCODE_LSHIFT);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    ret = input_byte_push(ctx, code);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = input_byte_push(ctx, (uint8_t)(code | (uint8_t)CROSSTALK_EDA_IO_SCANCODE_BREAK));
    if (ret != LDG_ERR_AOK) { return ret; }

    if (shift == LDG_TRUTH_TRUE)
    {
        ret = input_byte_push(ctx, (uint8_t)(CROSSTALK_EDA_IO_SCANCODE_LSHIFT | CROSSTALK_EDA_IO_SCANCODE_BREAK));
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_input_init(crosstalk_emu_input_ctx_t *ctx, uint32_t mode)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->mode = mode;
    ctx->data_sig = (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE;
    ctx->rdy_sig = (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE;
    ctx->ack_sig = (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE;
    ctx->state = (uint32_t)CROSSTALK_EMU_INPUT_STATE_IDLE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_input_bind(crosstalk_emu_input_ctx_t *ctx, const crosstalk_hdl_abi_t *abi, const char *data, const char *rdy, const char *ack)
{
    if (LDG_UNLIKELY(ctx == 0x0 || abi == 0x0 || data == 0x0 || rdy == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (abi->ok != LDG_TRUTH_TRUE || abi->sig_find == 0x0) { return LDG_ERR_NOT_INIT; }

    ctx->data_sig = abi->sig_find(data);
    ctx->rdy_sig = abi->sig_find(rdy);
    ctx->ack_sig = ack != 0x0 ? abi->sig_find(ack) : (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE;
    ctx->ack_has = ctx->ack_sig != (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (ctx->data_sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "input: the core has no data port; keystrokes are discarded; data: %.63s", data);

        ctx->live = LDG_TRUTH_FALSE;

        return CROSSTALK_EMU_INPUT_ERR_PORT;
    }

    ctx->live = LDG_TRUTH_TRUE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "input: bound; data: %.63s; rdy: %.63s; ack: %.63s; mode: %u32", data, rdy, ack != 0x0 ? ack : "", ctx->mode);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_input_put(crosstalk_emu_input_ctx_t *ctx, const uint8_t *bytes, uint32_t len)
{
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || bytes == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    for (at = 0; at < len; at = at + (uint32_t)INPUT_STEP)
    {
        if (ctx->mode == (uint32_t)CROSSTALK_EMU_INPUT_MODE_SCAN) { ret = input_scan_put(ctx, bytes[at]); }
        else { ret = input_byte_push(ctx, bytes[at]); }

        if (ret != LDG_ERR_AOK)
        {
            ctx->dropped = ctx->dropped + (uint64_t)(len - at);

            return ret;
        }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_input_scan_edge_put(crosstalk_emu_input_ctx_t *ctx, uint8_t code, uint8_t down)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (code == (uint8_t)CROSSTALK_EDA_IO_SCANCODE_NONE) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (down == LDG_TRUTH_TRUE) { return input_byte_push(ctx, code); }

    return input_byte_push(ctx, (uint8_t)(code | (uint8_t)CROSSTALK_EDA_IO_SCANCODE_BREAK));
}

static uint32_t input_scan_of_key(const ldg_term_key_event_t *key, uint8_t *out)
{
    if (LDG_UNLIKELY(key == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = CROSSTALK_EDA_IO_SCANCODE_NONE;

    if (key->key == (uint8_t)LDG_TERM_KEY_ENTER) { return crosstalk_eda_io_scancode_get((uint8_t)LDG_ASCII_CR, out); }

    if (key->key == (uint8_t)LDG_TERM_KEY_BACKSPACE) { return crosstalk_eda_io_scancode_get((uint8_t)INPUT_ASCII_BS, out); }

    if (key->key == (uint8_t)LDG_TERM_KEY_TAB) { return crosstalk_eda_io_scancode_get((uint8_t)LDG_ASCII_TAB, out); }

    if (key->key == (uint8_t)LDG_TERM_KEY_ESCAPE) { return crosstalk_eda_io_scancode_get((uint8_t)LDG_ASCII_ESC, out); }

    if (key->key == (uint8_t)LDG_TERM_KEY_UP) { *out = (uint8_t)INPUT_SCAN_UP; return LDG_ERR_AOK; }

    if (key->key == (uint8_t)LDG_TERM_KEY_DOWN) { *out = (uint8_t)INPUT_SCAN_DOWN; return LDG_ERR_AOK; }

    if (key->key == (uint8_t)LDG_TERM_KEY_LHS) { *out = (uint8_t)INPUT_SCAN_LHS; return LDG_ERR_AOK; }

    if (key->key == (uint8_t)LDG_TERM_KEY_RHS) { *out = (uint8_t)INPUT_SCAN_RHS; return LDG_ERR_AOK; }

    if (key->key != (uint8_t)LDG_TERM_KEY_NONE) { return LDG_ERR_NOT_FOUND; }

    if (key->codepoint == (uint32_t)INPUT_KITTY_LCTRL) { *out = (uint8_t)INPUT_SCAN_LCTRL; return LDG_ERR_AOK; }

    if (key->codepoint == (uint32_t)INPUT_KITTY_LALT) { *out = (uint8_t)INPUT_SCAN_LALT; return LDG_ERR_AOK; }

    if (key->codepoint == (uint32_t)INPUT_KITTY_LSHIFT) { *out = (uint8_t)INPUT_SCAN_LSHIFT; return LDG_ERR_AOK; }

    if (key->codepoint == (uint32_t)INPUT_KITTY_RSHIFT) { *out = (uint8_t)INPUT_SCAN_RSHIFT; return LDG_ERR_AOK; }

    if (key->codepoint < (uint32_t)LDG_ASCII_PRINTABLE_MIN || key->codepoint > (uint32_t)LDG_ASCII_PRINTABLE_MAX) { return LDG_ERR_NOT_FOUND; }

    return crosstalk_eda_io_scancode_get((uint8_t)key->codepoint, out);
}

static uint32_t input_text_put(crosstalk_emu_input_ctx_t *ctx, const ldg_term_key_event_t *key)
{
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || key == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (at = 0; at < key->text_cunt && at < (uint32_t)LDG_TERM_KEY_TEXT_MAX; at = at + (uint32_t)INPUT_STEP)
    {
        uint8_t byte = 0;

        if (key->text[at] < (uint32_t)LDG_ASCII_PRINTABLE_MIN || key->text[at] > (uint32_t)LDG_ASCII_PRINTABLE_MAX) { continue; }

        byte = (uint8_t)key->text[at];

        ret = crosstalk_emu_input_put(ctx, &byte, (uint32_t)INPUT_STEP);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_input_key_isr(crosstalk_emu_input_ctx_t *ctx, const ldg_term_key_event_t *key, uint8_t release_report)
{
    uint8_t code = CROSSTALK_EDA_IO_SCANCODE_NONE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || key == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (ctx->mode != (uint32_t)CROSSTALK_EMU_INPUT_MODE_SCAN)
    {
        if (key->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE) { return LDG_ERR_AOK; }

        return input_text_put(ctx, key);
    }

    if (release_report != LDG_TRUTH_TRUE && key->key == (uint8_t)LDG_TERM_KEY_NONE) { return input_text_put(ctx, key); }

    ret = input_scan_of_key(key, &code);
    if (ret == LDG_ERR_NOT_FOUND) { return LDG_ERR_AOK; }

    if (ret != LDG_ERR_AOK) { return ret; }

    if (key->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE) { return crosstalk_emu_input_scan_edge_put(ctx, code, LDG_TRUTH_FALSE); }

    ret = crosstalk_emu_input_scan_edge_put(ctx, code, LDG_TRUTH_TRUE);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (release_report == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    return crosstalk_emu_input_scan_edge_put(ctx, code, LDG_TRUTH_FALSE);
}

uint32_t crosstalk_emu_input_cycle_drive(crosstalk_emu_input_ctx_t *ctx, const crosstalk_hdl_abi_t *abi)
{
    uint64_t ack_val = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || abi == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (abi->ok != LDG_TRUTH_TRUE || abi->poke == 0x0 || abi->peek == 0x0) { return LDG_ERR_NOT_INIT; }

    if (ctx->data_sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { return LDG_ERR_AOK; }

    if (ctx->rdy_sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE)
    {
        if (ctx->rd_at == ctx->wr_at) { return LDG_ERR_AOK; }

        abi->poke(ctx->data_sig, (uint64_t)ctx->queue[ctx->rd_at]);

        ctx->rd_at = (ctx->rd_at + (uint32_t)INPUT_STEP) & (uint32_t)CROSSTALK_EMU_INPUT_QUEUE_MASK;
        ctx->bytes_total = ctx->bytes_total + (uint64_t)INPUT_STEP;
        ctx->state = (uint32_t)CROSSTALK_EMU_INPUT_STATE_IDLE;

        return LDG_ERR_AOK;
    }

    if (ctx->state == (uint32_t)CROSSTALK_EMU_INPUT_STATE_IDLE)
    {
        if (ctx->rd_at == ctx->wr_at) { return LDG_ERR_AOK; }

        abi->poke(ctx->data_sig, (uint64_t)ctx->queue[ctx->rd_at]);
        abi->poke(ctx->rdy_sig, (uint64_t)INPUT_PIN_HIGH);

        ctx->state = (uint32_t)CROSSTALK_EMU_INPUT_STATE_HELD;
        ctx->held = INPUT_HELD_NONE;

        return LDG_ERR_AOK;
    }

    ctx->held = ctx->held + (uint32_t)INPUT_STEP;

    if (ctx->ack_has == LDG_TRUTH_TRUE) { ack_val = abi->peek(ctx->ack_sig); }
    else if (abi->peek(ctx->rdy_sig) == (uint64_t)INPUT_PIN_LOW) { ack_val = (uint64_t)INPUT_PIN_HIGH; }

    if (ack_val == 0 && ctx->held < (uint32_t)CROSSTALK_EMU_INPUT_HOLD_MAX) { return LDG_ERR_AOK; }

    abi->poke(ctx->rdy_sig, (uint64_t)INPUT_PIN_LOW);

    if (ack_val == 0) { ctx->dropped = ctx->dropped + (uint64_t)INPUT_STEP; }

    ctx->rd_at = (ctx->rd_at + (uint32_t)INPUT_STEP) & (uint32_t)CROSSTALK_EMU_INPUT_QUEUE_MASK;
    ctx->bytes_total = ctx->bytes_total + (uint64_t)INPUT_STEP;
    ctx->state = (uint32_t)CROSSTALK_EMU_INPUT_STATE_IDLE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_input_pending_get(const crosstalk_emu_input_ctx_t *ctx, uint32_t *out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (ctx->wr_at - ctx->rd_at) & (uint32_t)CROSSTALK_EMU_INPUT_QUEUE_MASK;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_emu_input_live_is(const crosstalk_emu_input_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->live == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint8_t crosstalk_emu_input_idle_is(const crosstalk_emu_input_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_TRUTH_TRUE; }

    if (ctx->state != (uint32_t)CROSSTALK_EMU_INPUT_STATE_IDLE) { return LDG_TRUTH_FALSE; }

    return ctx->rd_at == ctx->wr_at ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}
