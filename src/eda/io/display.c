#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/arch/amd64/atomic.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "eda/io/display.h"

#define DISPLAY_STEP 1
#define DISPLAY_DARK 0
#define DISPLAY_FRAMES_NONE 0
#define DISPLAY_PX_MASK 255
#define DISPLAY_SYNC_MASK 1
#define DISPLAY_LOG_MASK 255

static uint32_t display_pin_bind(crosstalk_eda_io_display_ctx_t *ctx, uint32_t pin_idx, const crosstalk_hdl_exec_ctx_t *exec, const char *port)
{
    crosstalk_eda_io_display_pin_t *pin = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || port == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    pin = &ctx->pin[pin_idx];

    if (snprintf(pin->port, sizeof(pin->port), "%.63s", port) < 0) { return LDG_ERR_IO_FMT; }

    pin->sig_idx = UINT32_MAX;
    pin->abi_sig = UINT32_MAX;
    pin->abi_vals = 0x0;
    pin->abi_ck = LDG_TRUTH_FALSE;
    pin->bound = LDG_TRUTH_FALSE;

    if (exec == 0x0) { return LDG_ERR_AOK; }

    ret = crosstalk_hdl_exec_sig_find(exec, port, &pin->sig_idx);
    if (ret != LDG_ERR_AOK) { return CROSSTALK_EDA_IO_DISPLAY_ERR_PORT; }

    pin->bound = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_display_abi_rst(crosstalk_eda_io_display_ctx_t *ctx, uint32_t slot_idx)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->slot_idx != slot_idx) { return LDG_ERR_AOK; }

    for (idx = 0; idx < (uint32_t)CROSSTALK_EDA_IO_DISPLAY_PIN_CUNT; idx = idx + 0x1)
    {
        ctx->pin[idx].abi_vals = 0x0;
        ctx->pin[idx].abi_sig = UINT32_MAX;
        ctx->pin[idx].abi_ck = LDG_TRUTH_FALSE;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_display_init(crosstalk_eda_io_display_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->sync_active = CROSSTALK_EDA_IO_DISPLAY_SYNC_LOW;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_display_attach(crosstalk_eda_io_display_ctx_t *ctx, uint32_t slot_idx, const crosstalk_hdl_exec_ctx_t *exec, uint32_t mode, const char *hsync, const char *vsync, const char *video)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || hsync == 0x0 || vsync == 0x0 || video == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (mode > CROSSTALK_EDA_IO_DISPLAY_MODE_VGA) { return CROSSTALK_EDA_IO_DISPLAY_ERR_MODE; }

    ctx->live = LDG_TRUTH_FALSE;

    ret = display_pin_bind(ctx, CROSSTALK_EDA_IO_DISPLAY_PIN_HSYNC, exec, hsync);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = display_pin_bind(ctx, CROSSTALK_EDA_IO_DISPLAY_PIN_VSYNC, exec, vsync);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = display_pin_bind(ctx, CROSSTALK_EDA_IO_DISPLAY_PIN_VIDEO, exec, video);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->slot_idx = slot_idx;
    ctx->mode = mode;
    ctx->h_pos = 0;
    ctx->v_pos = 0;
    ctx->w_seen = 0;
    ctx->h_seen = 0;
    ctx->frames = DISPLAY_FRAMES_NONE;
    ctx->prev_hs = LDG_TRUTH_FALSE;
    ctx->prev_vs = LDG_TRUTH_FALSE;
    ctx->live = LDG_TRUTH_TRUE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "display: attached; slot: %u32; mode: %u32; hsync: %.31s; vsync: %.31s; video: %.31s", slot_idx, mode, hsync, vsync, video);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_display_detach(crosstalk_eda_io_display_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->live = LDG_TRUTH_FALSE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "display: detached; frames: %u64", ctx->frames);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_display_cycle_sample(crosstalk_eda_io_display_ctx_t *ctx, uint32_t slot_idx, const crosstalk_hdl_exec_ctx_t *exec, const crosstalk_hdl_abi_t *abi)
{
    uint64_t val[CROSSTALK_EDA_IO_DISPLAY_PIN_CUNT] = LDG_ARR_ZERO_INIT;
    crosstalk_eda_io_display_pin_t *pin = 0x0;
    uint32_t idx = 0;
    uint8_t packed = LDG_TRUTH_FALSE;
    uint8_t hs_act = LDG_TRUTH_FALSE;
    uint8_t vs_act = LDG_TRUTH_FALSE;
    uint8_t px_val = DISPLAY_DARK;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (ctx->slot_idx != slot_idx) { return LDG_ERR_AOK; }

    if (abi != 0x0 && abi->ok == LDG_TRUTH_TRUE) { packed = LDG_TRUTH_TRUE; }

    if (packed != LDG_TRUTH_TRUE && exec == 0x0) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < CROSSTALK_EDA_IO_DISPLAY_PIN_CUNT; idx = idx + DISPLAY_STEP)
    {
        pin = &ctx->pin[idx];

        if (packed == LDG_TRUTH_TRUE)
        {
            if (pin->abi_ck != LDG_TRUTH_TRUE)
            {
                uint64_t *base = 0x0;
                uint32_t cunt = 0;

                pin->abi_sig = abi->sig_find(pin->port);
                pin->abi_ck = LDG_TRUTH_TRUE;

                if (pin->abi_sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "display: port absent from the compiled component; port: %.63s", pin->port); }
                else if (abi->state_get((uint32_t)CROSSTALK_HDL_ABI_STATE_SIG_VALS, &base, &cunt) == LDG_ERR_AOK && base != 0x0 && pin->abi_sig < cunt) { pin->abi_vals = base; }
            }

            if (pin->abi_sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { return LDG_ERR_AOK; }

            if (pin->abi_vals != 0x0) { val[idx] = pin->abi_vals[pin->abi_sig]; }
            else { val[idx] = abi->peek(pin->abi_sig); }

            continue;
        }

        if (pin->bound != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

        val[idx] = exec->vals[pin->sig_idx];
    }

    if ((uint32_t)(val[CROSSTALK_EDA_IO_DISPLAY_PIN_HSYNC] & DISPLAY_SYNC_MASK) == ctx->sync_active) { hs_act = LDG_TRUTH_TRUE; }

    if ((uint32_t)(val[CROSSTALK_EDA_IO_DISPLAY_PIN_VSYNC] & DISPLAY_SYNC_MASK) == ctx->sync_active) { vs_act = LDG_TRUTH_TRUE; }

    if (ctx->mode == CROSSTALK_EDA_IO_DISPLAY_MODE_MONO) { if (val[CROSSTALK_EDA_IO_DISPLAY_PIN_VIDEO] != 0) { px_val = CROSSTALK_EDA_IO_DISPLAY_LIT; } }
    else { px_val = (uint8_t)(val[CROSSTALK_EDA_IO_DISPLAY_PIN_VIDEO] & DISPLAY_PX_MASK); }

    if (hs_act == LDG_TRUTH_TRUE && ctx->prev_hs != LDG_TRUTH_TRUE)
    {
        if (ctx->h_pos > ctx->w_seen) { ctx->w_seen = ctx->h_pos; }

        ctx->h_pos = 0;
        ctx->v_pos = ctx->v_pos + DISPLAY_STEP;
    }
    else if (hs_act != LDG_TRUTH_TRUE && vs_act != LDG_TRUTH_TRUE)
    {
        if (ctx->h_pos < CROSSTALK_EDA_IO_DISPLAY_W_MAX && ctx->v_pos < CROSSTALK_EDA_IO_DISPLAY_H_MAX) { ctx->px[ctx->front ^ DISPLAY_STEP][ctx->v_pos * CROSSTALK_EDA_IO_DISPLAY_W_MAX + ctx->h_pos] = px_val; }

        ctx->h_pos = ctx->h_pos + DISPLAY_STEP;
    }

    if (vs_act == LDG_TRUTH_TRUE && ctx->prev_vs != LDG_TRUTH_TRUE)
    {
        if (ctx->v_pos > ctx->h_seen) { ctx->h_seen = ctx->v_pos; }

        ctx->v_pos = 0;
        ctx->frames = ctx->frames + DISPLAY_STEP;
        LDG_ST_RELEASE(ctx->front, ctx->front ^ DISPLAY_STEP);

        if ((ctx->frames & DISPLAY_LOG_MASK) == 0) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "display: frame; cunt: %u64; w: %u32; h: %u32", ctx->frames, ctx->w_seen, ctx->h_seen); }
    }

    ctx->prev_hs = hs_act;
    ctx->prev_vs = vs_act;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_display_front_get(const crosstalk_eda_io_display_ctx_t *ctx, const uint8_t **out_px, uint32_t *out_w, uint32_t *out_h)
{
    uint32_t front = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_px == 0x0 || out_w == 0x0 || out_h == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_px = 0x0;
    *out_w = 0;
    *out_h = 0;

    if (ctx->live != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_IO_DISPLAY_ERR_LIVE; }

    front = LDG_LD_ACQUIRE(ctx->front);

    *out_px = &ctx->px[front][0];
    *out_w = ctx->w_seen;
    *out_h = ctx->h_seen;

    if (*out_w > CROSSTALK_EDA_IO_DISPLAY_W_MAX) { *out_w = CROSSTALK_EDA_IO_DISPLAY_W_MAX; }

    if (*out_h > CROSSTALK_EDA_IO_DISPLAY_H_MAX) { *out_h = CROSSTALK_EDA_IO_DISPLAY_H_MAX; }

    return LDG_ERR_AOK;
}

uint64_t crosstalk_eda_io_display_frames_get(const crosstalk_eda_io_display_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return DISPLAY_FRAMES_NONE; }

    return ctx->frames;
}

uint8_t crosstalk_eda_io_display_live_is(const crosstalk_eda_io_display_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

const char* crosstalk_eda_io_display_kind_name_get(const crosstalk_eda_io_display_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return CROSSTALK_EDA_IO_DISPLAY_KIND_NAME_CRT; }

    if (ctx->kind == CROSSTALK_EDA_IO_DISPLAY_KIND_PLASMA) { return CROSSTALK_EDA_IO_DISPLAY_KIND_NAME_PLASMA; }

    return CROSSTALK_EDA_IO_DISPLAY_KIND_NAME_CRT;
}
