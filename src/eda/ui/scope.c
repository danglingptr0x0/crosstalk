#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "eda/clk/clk.h"
#include "eda/ui/scope.h"
#include "ui/cell.h"

#define SCOPE_ORIG_COL 0
#define SCOPE_IDLE_ROW 0
#define SCOPE_PLOT_ORIG 0
#define SCOPE_FADE_FULL 255
#define SCOPE_RGBA_ALPHA_FULL 0xFFU
#define SCOPE_RGBA_CHAN_BITS 8
#define SCOPE_RGBA_RED_SHIFT 24
#define SCOPE_RGBA_GREEN_SHIFT 16
#define SCOPE_RGBA_BLUE_SHIFT 8
#define SCOPE_STATUS_FMT "%.15s.%.7s %s %.2f Vpp modeled; %" PRIu64 " Hz / %" PRIu64 " Hz comp; %" PRIu64 " ns/div; %.2f V/div"
#define SCOPE_STATUS_SIG_FMT "%.63s measured; %" PRIu64 " Hz; %" PRIu64 " ns/sample; %" PRIu64 " ns/div; %.2f V/div"
#define SCOPE_STATUS_MEM_FMT "%.31s: %.3f; "
#define SCOPE_WAVE_SINE_TEXT "sine"
#define SCOPE_WAVE_SQUARE_TEXT "square"

uint32_t crosstalk_eda_ui_scope_init(crosstalk_eda_ui_scope_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->win.open = LDG_TRUTH_FALSE;
    ctx->open = LDG_TRUTH_FALSE;
    ctx->ns_per_div = (uint64_t)CROSSTALK_EDA_UI_SCOPE_NS_PER_DIV_DEFAULT;
    ctx->volt_per_div = CROSSTALK_EDA_UI_SCOPE_VOLT_PER_DIV_DEFAULT;
    ctx->amplitude = CROSSTALK_EDA_UI_SCOPE_LOGIC_VOLT;
    ctx->mem_mode = (uint32_t)CROSSTALK_EDA_UI_SCOPE_MODE_YT;
    ctx->xy_x = (uint32_t)UINT32_MAX;
    ctx->xy_y = (uint32_t)UINT32_MAX;
    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_scope_shutdown(crosstalk_eda_ui_scope_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_scope_open_set(crosstalk_eda_ui_scope_ctx_t *ctx, uint8_t open)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->open = open;
    ctx->win.open = open;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_ui_scope_open_is(const crosstalk_eda_ui_scope_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->win.open != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_eda_ui_scope_detach(crosstalk_eda_ui_scope_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->live = LDG_TRUTH_FALSE;
    ctx->hd = 0;
    ctx->cunt = 0;
    ctx->src_f_hz = CROSSTALK_EDA_UI_SCOPE_HZ_NONE;
    ctx->src = (uint32_t)CROSSTALK_EDA_UI_SCOPE_SRC_PIN;
    ctx->sig[0] = LDG_STR_TERM;
    ctx->sig_step_ns = 0;
    ctx->sig_ns = 0;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_scope_sig_attach(crosstalk_eda_ui_scope_ctx_t *ctx, const char *sig, uint64_t step_ns)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || sig == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (step_ns == 0) { return CROSSTALK_EDA_UI_SCOPE_ERR_TIMEBASE; }

    ret = ldg_str_rbrcpy(ctx->sig, sig, (uint64_t)sizeof(ctx->sig));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    ctx->ref[0] = LDG_STR_TERM;
    ctx->pin[0] = LDG_STR_TERM;
    ctx->src = (uint32_t)CROSSTALK_EDA_UI_SCOPE_SRC_SIG;
    ctx->sig_step_ns = step_ns;
    ctx->sig_ns = 0;
    ctx->src_f_hz = (uint64_t)LDG_NS_PER_SEC / step_ns;
    ctx->hd = 0;
    ctx->cunt = 0;
    ctx->last_ns = 0;
    ctx->ns_per_div = step_ns * (uint64_t)CROSSTALK_EDA_UI_SCOPE_PERIOD_SAMPLES;
    ctx->live = LDG_TRUTH_TRUE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "scope: signal attached; sig: %.63s; step: %u64 ns; ns/div: %u64", sig, step_ns, ctx->ns_per_div);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_scope_sink_cb(void *user, const uint8_t *bytes, uint32_t len)
{
    crosstalk_eda_ui_scope_ctx_t *ctx = (crosstalk_eda_ui_scope_ctx_t *)user;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || bytes == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live != LDG_TRUTH_TRUE || ctx->src != (uint32_t)CROSSTALK_EDA_UI_SCOPE_SRC_SIG) { return LDG_ERR_AOK; }

    while (at + (uint32_t)CROSSTALK_EDA_UI_SCOPE_HALF <= len)
    {
        uint32_t slot = (ctx->hd + ctx->cunt) % (uint32_t)CROSSTALK_EDA_UI_SCOPE_SAMPLE_CAP;
        uint16_t raw = (uint16_t)((uint32_t)bytes[at] | ((uint32_t)bytes[at + CROSSTALK_EDA_UI_SCOPE_STEP] << (uint32_t)LDG_BYTE_BITS));
        double turn = (double)(int16_t)raw / CROSSTALK_EDA_UI_SCOPE_SIG_FULL;

        ctx->sig_ns = ctx->sig_ns + ctx->sig_step_ns;

        ctx->sample[slot].sim_ns = ctx->sig_ns;
        ctx->sample[slot].volt = (turn + CROSSTALK_EDA_UI_SCOPE_MID) * CROSSTALK_EDA_UI_SCOPE_LOGIC_VOLT;

        if (ctx->cunt < (uint32_t)CROSSTALK_EDA_UI_SCOPE_SAMPLE_CAP) { ctx->cunt = ctx->cunt + (uint32_t)CROSSTALK_EDA_UI_SCOPE_STEP; }
        else { ctx->hd = (ctx->hd + (uint32_t)CROSSTALK_EDA_UI_SCOPE_STEP) % (uint32_t)CROSSTALK_EDA_UI_SCOPE_SAMPLE_CAP; }

        ctx->last_ns = ctx->sig_ns;
        at = at + (uint32_t)CROSSTALK_EDA_UI_SCOPE_HALF;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_scope_timebase_step(crosstalk_eda_ui_scope_ctx_t *ctx, int32_t dir)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (dir > 0 && ctx->ns_per_div < (uint64_t)CROSSTALK_EDA_UI_SCOPE_NS_PER_DIV_MAX) { ctx->ns_per_div = ctx->ns_per_div * (uint64_t)CROSSTALK_EDA_UI_SCOPE_TIMEBASE_STEP; }

    if (dir < 0 && ctx->ns_per_div > (uint64_t)CROSSTALK_EDA_UI_SCOPE_NS_PER_DIV_MIN) { ctx->ns_per_div = ctx->ns_per_div / (uint64_t)CROSSTALK_EDA_UI_SCOPE_TIMEBASE_STEP; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_scope_capture(crosstalk_eda_ui_scope_ctx_t *ctx, uint64_t sim_ns)
{
    uint64_t period_ns = 0;
    uint64_t step_ns = 0;
    uint64_t at_ns = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (ctx->src == (uint32_t)CROSSTALK_EDA_UI_SCOPE_SRC_SIG) { return LDG_ERR_AOK; }

    if (ctx->src_f_hz == 0) { return CROSSTALK_EDA_UI_SCOPE_ERR_NO_SIG; }

    if (sim_ns <= ctx->last_ns) { return LDG_ERR_AOK; }

    period_ns = (uint64_t)LDG_NS_PER_SEC / ctx->src_f_hz;
    if (period_ns == 0) { period_ns = (uint64_t)CROSSTALK_EDA_UI_SCOPE_EDGE_MIN; }

    step_ns = period_ns / (uint64_t)CROSSTALK_EDA_UI_SCOPE_PERIOD_SAMPLES;
    if (step_ns == 0) { step_ns = (uint64_t)CROSSTALK_EDA_UI_SCOPE_STEP; }

    if (ctx->last_ns == 0)
    {
        uint64_t back_ns = (uint64_t)CROSSTALK_EDA_UI_SCOPE_SAMPLE_CAP * step_ns;

        ctx->last_ns = sim_ns > back_ns ? sim_ns - back_ns : (uint64_t)CROSSTALK_EDA_UI_SCOPE_TIME_ORIG;
    }

    for (at_ns = ctx->last_ns + step_ns; at_ns <= sim_ns; at_ns = at_ns + step_ns)
    {
        uint32_t slot = (ctx->hd + ctx->cunt) % (uint32_t)CROSSTALK_EDA_UI_SCOPE_SAMPLE_CAP;
        uint64_t in_period = at_ns % period_ns;
        double turn = (double)in_period / (double)period_ns;

        ctx->sample[slot].sim_ns = at_ns;

        if (ctx->wave == (uint32_t)CROSSTALK_EDA_UI_SCOPE_WAVE_SINE) { ctx->sample[slot].volt = ctx->amplitude * CROSSTALK_EDA_UI_SCOPE_MID * (CROSSTALK_EDA_UI_SCOPE_FRAC_MAX + sin(CROSSTALK_EDA_UI_SCOPE_TAU * turn)); }
        else { ctx->sample[slot].volt = turn < CROSSTALK_EDA_UI_SCOPE_MID ? ctx->amplitude : 0.0; }

        if (ctx->cunt < (uint32_t)CROSSTALK_EDA_UI_SCOPE_SAMPLE_CAP) { ctx->cunt = ctx->cunt + (uint32_t)CROSSTALK_EDA_UI_SCOPE_STEP; }
        else { ctx->hd = (ctx->hd + (uint32_t)CROSSTALK_EDA_UI_SCOPE_STEP) % (uint32_t)CROSSTALK_EDA_UI_SCOPE_SAMPLE_CAP; }

        ctx->last_ns = at_ns;
    }

    ctx->last_ns = sim_ns;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_scope_freq_measure(const crosstalk_eda_ui_scope_ctx_t *ctx, uint64_t *out)
{
    uint64_t first_ns = 0;
    uint64_t last_ns = 0;
    double mid = 0.0;
    uint32_t edges = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = CROSSTALK_EDA_UI_SCOPE_HZ_NONE;

    mid = ctx->amplitude * CROSSTALK_EDA_UI_SCOPE_MID;

    if (ctx->cunt < (uint32_t)CROSSTALK_EDA_UI_SCOPE_EDGE_MIN) { return LDG_ERR_EMPTY; }

    for (idx = CROSSTALK_EDA_UI_SCOPE_STEP; idx < ctx->cunt; idx++)
    {
        uint32_t prev = (ctx->hd + idx - (uint32_t)CROSSTALK_EDA_UI_SCOPE_STEP) % (uint32_t)CROSSTALK_EDA_UI_SCOPE_SAMPLE_CAP;
        uint32_t cur = (ctx->hd + idx) % (uint32_t)CROSSTALK_EDA_UI_SCOPE_SAMPLE_CAP;

        if (ctx->sample[prev].volt >= mid) { continue; }

        if (ctx->sample[cur].volt < mid) { continue; }

        if (edges == 0) { first_ns = ctx->sample[cur].sim_ns; }

        last_ns = ctx->sample[cur].sim_ns;
        edges = edges + (uint32_t)CROSSTALK_EDA_UI_SCOPE_STEP;
    }

    if (edges < (uint32_t)CROSSTALK_EDA_UI_SCOPE_EDGE_MIN || last_ns <= first_ns) { return LDG_ERR_EMPTY; }

    *out = (uint64_t)(edges - (uint32_t)CROSSTALK_EDA_UI_SCOPE_STEP) * (uint64_t)LDG_NS_PER_SEC / (last_ns - first_ns);

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_ui_scope_mem_live_is(const crosstalk_eda_ui_scope_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->mem_cunt != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_eda_ui_scope_mem_clear(crosstalk_eda_ui_scope_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx->mem_trace, 0x0, sizeof(ctx->mem_trace));

    ctx->mem_cunt = 0;
    ctx->mem_mode = (uint32_t)CROSSTALK_EDA_UI_SCOPE_MODE_YT;
    ctx->xy_x = (uint32_t)UINT32_MAX;
    ctx->xy_y = (uint32_t)UINT32_MAX;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_scope_mem_attach(crosstalk_eda_ui_scope_ctx_t *ctx, uint64_t addr, uint32_t width, uint8_t fixed, const char *label, uint32_t *out_idx)
{
    crosstalk_eda_ui_scope_mem_trace_t *trace = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0 || label == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = (uint32_t)UINT32_MAX;

    if (width > (uint32_t)CROSSTALK_EDA_UI_SCOPE_WIDTH_INT32) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->mem_cunt >= (uint32_t)CROSSTALK_EDA_UI_SCOPE_MEM_TRACE_MAX) { return LDG_ERR_FULL; }

    trace = &ctx->mem_trace[ctx->mem_cunt];

    memset(trace, 0x0, sizeof(*trace));

    trace->addr = addr;
    trace->width = width;
    trace->fixed = fixed != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    trace->live = LDG_TRUTH_TRUE;

    if (ldg_str_rbrcpy(trace->label, label, (uint64_t)sizeof(trace->label)) == LDG_ERR_STR_TRUNC) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scope trace label truncated"); }

    *out_idx = ctx->mem_cunt;
    ctx->mem_cunt = ctx->mem_cunt + (uint32_t)CROSSTALK_EDA_UI_SCOPE_STEP;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_scope_xy_set(crosstalk_eda_ui_scope_ctx_t *ctx, uint32_t trace_x, uint32_t trace_y)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (trace_x == (uint32_t)UINT32_MAX || trace_y == (uint32_t)UINT32_MAX)
    {
        ctx->mem_mode = (uint32_t)CROSSTALK_EDA_UI_SCOPE_MODE_YT;

        return LDG_ERR_AOK;
    }

    if (trace_x >= ctx->mem_cunt || trace_y >= ctx->mem_cunt) { return CROSSTALK_EDA_UI_SCOPE_ERR_TRACE; }

    ctx->xy_x = trace_x;
    ctx->xy_y = trace_y;
    ctx->mem_mode = (uint32_t)CROSSTALK_EDA_UI_SCOPE_MODE_XY;

    return LDG_ERR_AOK;
}

static double scope_mem_val_get(const uint64_t *byte, uint32_t width, uint8_t fixed)
{
    uint32_t raw = 0;
    double val = 0.0;

    raw = (uint32_t)((byte[0] & (uint64_t)LDG_BYTE_MASK) | ((byte[1] & (uint64_t)LDG_BYTE_MASK) << (uint64_t)LDG_BYTE_BITS) | ((byte[2] & (uint64_t)LDG_BYTE_MASK) << ((uint64_t)LDG_BYTE_BITS * 0x2)) | ((byte[3] & (uint64_t)LDG_BYTE_MASK) << ((uint64_t)LDG_BYTE_BITS * 0x3)));

    if (width == (uint32_t)CROSSTALK_EDA_UI_SCOPE_WIDTH_UINT8) { val = (double)(raw & (uint32_t)LDG_BYTE_MASK); }
    else if (width == (uint32_t)CROSSTALK_EDA_UI_SCOPE_WIDTH_UINT16) { val = (double)(raw & (uint32_t)UINT16_MAX); }
    else if (width == (uint32_t)CROSSTALK_EDA_UI_SCOPE_WIDTH_INT8) { val = (double)(int8_t)(raw & (uint32_t)LDG_BYTE_MASK); }
    else if (width == (uint32_t)CROSSTALK_EDA_UI_SCOPE_WIDTH_INT16) { val = (double)(int16_t)(raw & (uint32_t)UINT16_MAX); }
    else if (width == (uint32_t)CROSSTALK_EDA_UI_SCOPE_WIDTH_INT32) { val = (double)(int32_t)raw; }
    else { val = (double)raw; }

    if (fixed == LDG_TRUTH_TRUE) { return val / CROSSTALK_EDA_UI_SCOPE_FIXED_SCALE; }

    return val;
}

uint32_t crosstalk_eda_ui_scope_mem_capture(crosstalk_eda_ui_scope_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, const char *sig)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || fpga == 0x0 || sig == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (at = 0; at < ctx->mem_cunt; at = at + (uint32_t)CROSSTALK_EDA_UI_SCOPE_STEP)
    {
        crosstalk_eda_ui_scope_mem_trace_t *trace = &ctx->mem_trace[at];
        uint64_t byte[CROSSTALK_EDA_UI_SCOPE_MEM_WORD_BYTES] = LDG_ARR_ZERO_INIT;
        uint32_t got = 0;
        uint32_t slot = 0;

        if (trace->live != LDG_TRUTH_TRUE) { continue; }

        if (crosstalk_eda_ic_fpga_mem_byte_rd(fpga, slot_idx, sig, trace->addr, byte, (uint32_t)CROSSTALK_EDA_UI_SCOPE_MEM_WORD_BYTES, &got) != LDG_ERR_AOK) { continue; }

        if (got < (uint32_t)CROSSTALK_EDA_UI_SCOPE_MEM_WORD_BYTES) { continue; }

        trace->last = scope_mem_val_get(byte, trace->width, trace->fixed);

        slot = (trace->hd + trace->cunt) % (uint32_t)CROSSTALK_EDA_UI_SCOPE_MEM_SAMPLE_CAP;
        trace->val[slot] = trace->last;

        if (trace->cunt < (uint32_t)CROSSTALK_EDA_UI_SCOPE_MEM_SAMPLE_CAP) { trace->cunt = trace->cunt + (uint32_t)CROSSTALK_EDA_UI_SCOPE_STEP; }
        else { trace->hd = (trace->hd + (uint32_t)CROSSTALK_EDA_UI_SCOPE_STEP) % (uint32_t)CROSSTALK_EDA_UI_SCOPE_MEM_SAMPLE_CAP; }
    }

    return LDG_ERR_AOK;
}

static uint32_t scope_mem_range_get(crosstalk_eda_ui_scope_mem_trace_t *trace)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(trace == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (trace->cunt == 0) { return LDG_ERR_EMPTY; }

    trace->lo = trace->val[trace->hd];
    trace->hi = trace->lo;

    for (at = 0; at < trace->cunt; at = at + (uint32_t)CROSSTALK_EDA_UI_SCOPE_STEP)
    {
        double val = trace->val[(trace->hd + at) % (uint32_t)CROSSTALK_EDA_UI_SCOPE_MEM_SAMPLE_CAP];

        if (val < trace->lo) { trace->lo = val; }

        if (val > trace->hi) { trace->hi = val; }
    }

    if (trace->hi - trace->lo < CROSSTALK_EDA_UI_SCOPE_RANGE_FLAT)
    {
        trace->lo = trace->lo - CROSSTALK_EDA_UI_SCOPE_RANGE_FLAT;
        trace->hi = trace->hi + CROSSTALK_EDA_UI_SCOPE_RANGE_FLAT;
    }

    return LDG_ERR_AOK;
}

static uint32_t scope_trace_col_get(uint32_t idx)
{
    if (idx == 0) { return (uint32_t)CROSSTALK_UI_THEME_LIVE_RGBA; }

    if (idx == 0x1) { return (uint32_t)CROSSTALK_EDA_UI_SCOPE_TRACE_B_COL; }

    if (idx == 0x2) { return (uint32_t)CROSSTALK_UI_THEME_FOCUS_RGBA; }

    return (uint32_t)CROSSTALK_UI_THEME_ACCENT_RGBA;
}

static uint32_t scope_step_draw(ldg_term_render_t *render, double prev_x, double prev_y, double at_x, double at_y, uint32_t rgba)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_ui_cell_line_draw(render, (uint32_t)prev_x, (uint32_t)prev_y, (uint32_t)at_x, (uint32_t)prev_y, rgba);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if ((uint32_t)at_y == (uint32_t)prev_y) { return LDG_ERR_AOK; }

    return crosstalk_ui_cell_line_draw(render, (uint32_t)at_x, (uint32_t)prev_y, (uint32_t)at_x, (uint32_t)at_y, rgba);
}

static uint32_t scope_fade_rgba_get(uint32_t rgba, double fade)
{
    uint32_t level = (uint32_t)fade;
    uint32_t red = (rgba >> (uint32_t)SCOPE_RGBA_RED_SHIFT) & (uint32_t)LDG_BYTE_MASK;
    uint32_t green = (rgba >> (uint32_t)SCOPE_RGBA_GREEN_SHIFT) & (uint32_t)LDG_BYTE_MASK;
    uint32_t blue = (rgba >> (uint32_t)SCOPE_RGBA_BLUE_SHIFT) & (uint32_t)LDG_BYTE_MASK;

    if (level > (uint32_t)SCOPE_FADE_FULL) { level = (uint32_t)SCOPE_FADE_FULL; }

    red = red * level / (uint32_t)SCOPE_FADE_FULL;
    green = green * level / (uint32_t)SCOPE_FADE_FULL;
    blue = blue * level / (uint32_t)SCOPE_FADE_FULL;

    return (red << (uint32_t)SCOPE_RGBA_RED_SHIFT) | (green << (uint32_t)SCOPE_RGBA_GREEN_SHIFT) | (blue << (uint32_t)SCOPE_RGBA_BLUE_SHIFT) | (uint32_t)SCOPE_RGBA_ALPHA_FULL;
}

static uint32_t scope_mem_yt_draw(crosstalk_eda_ui_scope_ctx_t *ctx, ldg_term_render_t *render, double grid_w, double grid_h)
{
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (at = 0; at < ctx->mem_cunt; at = at + (uint32_t)CROSSTALK_EDA_UI_SCOPE_STEP)
    {
        crosstalk_eda_ui_scope_mem_trace_t *trace = &ctx->mem_trace[at];
        uint32_t rgba = scope_trace_col_get(at);
        double prev_x = 0.0;
        double prev_y = 0.0;
        uint8_t have_prev = LDG_TRUTH_FALSE;
        uint32_t idx = 0;

        if (trace->live != LDG_TRUTH_TRUE || scope_mem_range_get(trace) != LDG_ERR_AOK) { continue; }

        for (idx = 0; idx < trace->cunt; idx = idx + (uint32_t)CROSSTALK_EDA_UI_SCOPE_STEP)
        {
            double val = trace->val[(trace->hd + idx) % (uint32_t)CROSSTALK_EDA_UI_SCOPE_MEM_SAMPLE_CAP];
            double at_x = grid_w * (double)idx / (double)CROSSTALK_EDA_UI_SCOPE_MEM_SAMPLE_CAP;
            double at_y = grid_h - grid_h * (val - trace->lo) / (trace->hi - trace->lo);

            if (have_prev == LDG_TRUTH_TRUE)
            {
                ret = scope_step_draw(render, prev_x, prev_y, at_x, at_y, rgba);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            prev_x = at_x;
            prev_y = at_y;
            have_prev = LDG_TRUTH_TRUE;
        }
    }

    return LDG_ERR_AOK;
}

static uint32_t scope_mem_xy_draw(crosstalk_eda_ui_scope_ctx_t *ctx, ldg_term_render_t *render, double grid_w, double grid_h)
{
    crosstalk_eda_ui_scope_mem_trace_t *trace_x = 0x0;
    crosstalk_eda_ui_scope_mem_trace_t *trace_y = 0x0;
    uint32_t span = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->xy_x >= ctx->mem_cunt || ctx->xy_y >= ctx->mem_cunt) { return CROSSTALK_EDA_UI_SCOPE_ERR_TRACE; }

    trace_x = &ctx->mem_trace[ctx->xy_x];
    trace_y = &ctx->mem_trace[ctx->xy_y];

    if (scope_mem_range_get(trace_x) != LDG_ERR_AOK || scope_mem_range_get(trace_y) != LDG_ERR_AOK) { return LDG_ERR_EMPTY; }

    span = trace_x->cunt < trace_y->cunt ? trace_x->cunt : trace_y->cunt;

    for (idx = 0; idx < span; idx = idx + (uint32_t)CROSSTALK_EDA_UI_SCOPE_STEP)
    {
        double val_x = trace_x->val[(trace_x->hd + idx) % (uint32_t)CROSSTALK_EDA_UI_SCOPE_MEM_SAMPLE_CAP];
        double val_y = trace_y->val[(trace_y->hd + idx) % (uint32_t)CROSSTALK_EDA_UI_SCOPE_MEM_SAMPLE_CAP];
        double at_x = grid_w * (val_x - trace_x->lo) / (trace_x->hi - trace_x->lo);
        double at_y = grid_h - grid_h * (val_y - trace_y->lo) / (trace_y->hi - trace_y->lo);
        double fade = CROSSTALK_EDA_UI_SCOPE_XY_FADE_MIN + (CROSSTALK_EDA_UI_SCOPE_XY_FADE_MAX - CROSSTALK_EDA_UI_SCOPE_XY_FADE_MIN) * (double)idx / (double)span;

        ret = crosstalk_ui_cell_dot_draw(render, (uint32_t)at_x, (uint32_t)at_y, scope_fade_rgba_get(scope_trace_col_get(ctx->xy_y), fade));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t scope_grid_draw(ldg_term_render_t *render, uint32_t divs_x, uint32_t divs_y)
{
    uint32_t grid_w = divs_x * (uint32_t)CROSSTALK_EDA_UI_SCOPE_DIV_SAMPLES;
    uint32_t grid_h = divs_y * (uint32_t)CROSSTALK_EDA_UI_SCOPE_DIV_SAMPLES;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx <= divs_x; idx = idx + (uint32_t)CROSSTALK_EDA_UI_SCOPE_STEP)
    {
        uint32_t grid_x = idx * (uint32_t)CROSSTALK_EDA_UI_SCOPE_DIV_SAMPLES;

        ret = crosstalk_ui_cell_line_draw(render, grid_x, (uint32_t)SCOPE_PLOT_ORIG, grid_x, grid_h, (uint32_t)CROSSTALK_UI_THEME_GRID_RGBA);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (idx = 0; idx <= divs_y; idx = idx + (uint32_t)CROSSTALK_EDA_UI_SCOPE_STEP)
    {
        uint32_t grid_y = idx * (uint32_t)CROSSTALK_EDA_UI_SCOPE_DIV_SAMPLES;

        ret = crosstalk_ui_cell_line_draw(render, (uint32_t)SCOPE_PLOT_ORIG, grid_y, grid_w, grid_y, (uint32_t)CROSSTALK_UI_THEME_GRID_RGBA);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t scope_draw_cb(uint64_t win_id, void *user, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    crosstalk_eda_ui_scope_ctx_t *ctx = (crosstalk_eda_ui_scope_ctx_t *)user;
    char status[CROSSTALK_EDA_UI_SCOPE_STATUS_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t meas_hz = 0;
    uint64_t win_lo_ns = 0;
    uint64_t win_hi_ns = 0;
    double grid_w = 0.0;
    double grid_h = 0.0;
    double mid_y = 0.0;
    double prev_x = 0.0;
    double prev_y = 0.0;
    uint64_t span_ns = 0;
    uint32_t plot_row_cunt = 0;
    uint32_t divs_x = 0;
    uint32_t divs_y = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;
    uint8_t have_prev = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (win_id != (uint64_t)CROSSTALK_EDA_UI_SCOPE_WINDOW_ID || col_cunt == 0 || row_cunt <= (uint32_t)CROSSTALK_EDA_UI_SCOPE_STATUS_ROW_CUNT) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->live != LDG_TRUTH_TRUE && crosstalk_eda_ui_scope_mem_live_is(ctx) != LDG_TRUTH_TRUE) { return crosstalk_ui_cell_rgba_put(render, (uint32_t)SCOPE_ORIG_COL, (uint32_t)SCOPE_IDLE_ROW, CROSSTALK_EDA_UI_SCOPE_IDLE_TEXT, (uint64_t)sizeof(CROSSTALK_EDA_UI_SCOPE_IDLE_TEXT), (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, 0x0); }

    plot_row_cunt = row_cunt - (uint32_t)CROSSTALK_EDA_UI_SCOPE_STATUS_ROW_CUNT;
    divs_x = col_cunt * render->sample_col_cunt / (uint32_t)CROSSTALK_EDA_UI_SCOPE_DIV_SAMPLES;
    divs_y = plot_row_cunt * render->sample_row_cunt / (uint32_t)CROSSTALK_EDA_UI_SCOPE_DIV_SAMPLES;

    if (divs_x < (uint32_t)CROSSTALK_EDA_UI_SCOPE_DIV_MIN || divs_y < (uint32_t)CROSSTALK_EDA_UI_SCOPE_DIV_MIN) { return LDG_ERR_AOK; }

    grid_w = (double)(divs_x * (uint32_t)CROSSTALK_EDA_UI_SCOPE_DIV_SAMPLES);
    grid_h = (double)(divs_y * (uint32_t)CROSSTALK_EDA_UI_SCOPE_DIV_SAMPLES);
    mid_y = grid_h * CROSSTALK_EDA_UI_SCOPE_MID;

    ret = scope_grid_draw(render, divs_x, divs_y);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (crosstalk_eda_ui_scope_mem_live_is(ctx) == LDG_TRUTH_TRUE)
    {
        uint32_t trace_at = 0;
        uint32_t pos = 0;

        if (ctx->mem_mode == (uint32_t)CROSSTALK_EDA_UI_SCOPE_MODE_XY) { ret = scope_mem_xy_draw(ctx, render, grid_w, grid_h); }
        else { ret = scope_mem_yt_draw(ctx, render, grid_w, grid_h); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_EMPTY && ret != (uint32_t)CROSSTALK_EDA_UI_SCOPE_ERR_TRACE)) { return ret; }

        for (trace_at = 0; trace_at < ctx->mem_cunt && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(status); trace_at = trace_at + (uint32_t)CROSSTALK_EDA_UI_SCOPE_STEP)
        {
            int32_t put = snprintf(&status[pos], sizeof(status) - pos, SCOPE_STATUS_MEM_FMT, ctx->mem_trace[trace_at].label, ctx->mem_trace[trace_at].last);

            if (put < 0) { break; }

            pos = pos + (uint32_t)put;
        }

        return crosstalk_ui_cell_rgba_put(render, (uint32_t)SCOPE_ORIG_COL, plot_row_cunt, status, (uint64_t)sizeof(status), (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, 0x0);
    }

    if (ctx->cunt == 0) { return crosstalk_ui_cell_rgba_put(render, (uint32_t)SCOPE_ORIG_COL, (uint32_t)SCOPE_IDLE_ROW, CROSSTALK_EDA_UI_SCOPE_DEAD_TEXT, (uint64_t)sizeof(CROSSTALK_EDA_UI_SCOPE_DEAD_TEXT), (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, 0x0); }

    span_ns = (uint64_t)divs_x * ctx->ns_per_div;

    win_hi_ns = ctx->sample[(ctx->hd + ctx->cunt - (uint32_t)CROSSTALK_EDA_UI_SCOPE_STEP) % (uint32_t)CROSSTALK_EDA_UI_SCOPE_SAMPLE_CAP].sim_ns;
    win_lo_ns = win_hi_ns > span_ns ? win_hi_ns - span_ns : (uint64_t)CROSSTALK_EDA_UI_SCOPE_TIME_ORIG;

    for (idx = 0; idx < ctx->cunt; idx = idx + (uint32_t)CROSSTALK_EDA_UI_SCOPE_STEP)
    {
        uint32_t slot = (ctx->hd + idx) % (uint32_t)CROSSTALK_EDA_UI_SCOPE_SAMPLE_CAP;
        uint64_t at_ns = ctx->sample[slot].sim_ns;
        double at_x = 0.0;
        double at_y = 0.0;

        if (at_ns < win_lo_ns || win_hi_ns <= win_lo_ns) { continue; }

        at_x = (double)(at_ns - win_lo_ns) / (double)ctx->ns_per_div * (double)CROSSTALK_EDA_UI_SCOPE_DIV_SAMPLES;
        if (at_x > grid_w) { continue; }

        at_y = mid_y - (ctx->sample[slot].volt - ctx->amplitude * CROSSTALK_EDA_UI_SCOPE_MID) / ctx->volt_per_div * (double)CROSSTALK_EDA_UI_SCOPE_DIV_SAMPLES;

        if (at_y < (double)SCOPE_PLOT_ORIG) { at_y = (double)SCOPE_PLOT_ORIG; }

        if (at_y > grid_h) { at_y = grid_h; }

        if (have_prev == LDG_TRUTH_TRUE)
        {
            ret = scope_step_draw(render, prev_x, prev_y, at_x, at_y, (uint32_t)CROSSTALK_UI_THEME_LIVE_RGBA);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        prev_x = at_x;
        prev_y = at_y;
        have_prev = LDG_TRUTH_TRUE;
    }

    if (crosstalk_eda_ui_scope_freq_measure(ctx, &meas_hz) != LDG_ERR_AOK) { meas_hz = CROSSTALK_EDA_UI_SCOPE_HZ_NONE; }

    if (ctx->src == (uint32_t)CROSSTALK_EDA_UI_SCOPE_SRC_SIG) { if (snprintf(status, sizeof(status), SCOPE_STATUS_SIG_FMT, ctx->sig, meas_hz, ctx->sig_step_ns, ctx->ns_per_div, ctx->volt_per_div) < 0) { return LDG_ERR_STR_FMT; } }
    else if (snprintf(status, sizeof(status), SCOPE_STATUS_FMT, ctx->ref, ctx->pin, ctx->wave == (uint32_t)CROSSTALK_EDA_UI_SCOPE_WAVE_SINE ? SCOPE_WAVE_SINE_TEXT : SCOPE_WAVE_SQUARE_TEXT, ctx->amplitude, meas_hz, ctx->src_f_hz, ctx->ns_per_div, ctx->volt_per_div) < 0) { return LDG_ERR_STR_FMT; }

    return crosstalk_ui_cell_rgba_put(render, (uint32_t)SCOPE_ORIG_COL, plot_row_cunt, status, (uint64_t)sizeof(status), (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, 0x0);
}

uint32_t crosstalk_eda_ui_scope_win_reg(crosstalk_eda_ui_scope_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot)
{
    if (LDG_UNLIKELY(ctx == 0x0 || win_nav == 0x0 || out_slot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_ui_window_reg(win_nav, (uint64_t)CROSSTALK_EDA_UI_SCOPE_WINDOW_ID, CROSSTALK_EDA_UI_SCOPE_WINDOW_TITLE, &ctx->win, scope_draw_cb, 0x0, ctx, out_slot);
}
