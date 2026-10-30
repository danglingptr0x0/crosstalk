#include <stdint.h>
#include <string.h>

#include <dangling/core/arith.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#include "eda/clk/clk.h"

static uint32_t wide_scale_div(uint64_t val, uint64_t mul, uint64_t div, uint64_t *out)
{
    ldg_uint128_t lhs = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t rhs = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t prod = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t quot = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t rem = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (div == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    lhs.limb[0] = val;
    rhs.limb[0] = mul;

    ret = ldg_arith_uint128_mul(&lhs, &rhs, &prod);
    if (ret != LDG_ERR_AOK) { return ret; }

    rhs.limb[0] = div;

    ret = ldg_arith_uint128_div_compute(&prod, &rhs, &quot, &rem);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (quot.limb[1] != 0) { return LDG_ERR_OVERFLOW; }

    *out = quot.limb[0];
    return LDG_ERR_AOK;
}

static void domain_step(crosstalk_eda_clk_domain_t *domain, uint64_t step_hz, uint64_t steps)
{
    uint64_t idx = 0;
    uint64_t run = 0;

    domain->cycles_step = 0x0;

    if (domain->en != LDG_TRUTH_TRUE || domain->f_hz == 0) { return; }

    for (idx = 0; idx < steps; idx = idx + 0x1)
    {
        domain->phase = domain->phase + domain->f_hz;
        run = domain->phase / step_hz;
        domain->phase = domain->phase % step_hz;
        domain->cycles_step = domain->cycles_step + run;
    }

    domain->cycles = domain->cycles + domain->cycles_step;
}

uint32_t crosstalk_eda_clk_init(crosstalk_eda_clk_ctx_t *ctx, uint64_t step_hz)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (step_hz == 0) { return CROSSTALK_EDA_CLK_ERR_STEP; }

    *ctx = (crosstalk_eda_clk_ctx_t)LDG_STRUCT_ZERO_INIT;
    ctx->step_hz = step_hz;
    ctx->ok = LDG_TRUTH_TRUE;
    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_clk_shutdown(crosstalk_eda_clk_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *ctx = (crosstalk_eda_clk_ctx_t)LDG_STRUCT_ZERO_INIT;
    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_clk_domain_add(crosstalk_eda_clk_ctx_t *ctx, uint32_t net, uint64_t f_hz, uint32_t *out)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (f_hz == 0) { return CROSSTALK_EDA_CLK_ERR_FREQ; }

    for (idx = 0; idx < CROSSTALK_EDA_CLK_DOMAIN_MAX; idx = idx + 0x1)
    {
        if (ctx->domain[idx].live == LDG_TRUTH_TRUE) { continue; }

        ctx->domain[idx] = (crosstalk_eda_clk_domain_t)LDG_STRUCT_ZERO_INIT;
        ctx->domain[idx].f_hz = f_hz;
        ctx->domain[idx].net = net;
        ctx->domain[idx].en = LDG_TRUTH_TRUE;
        ctx->domain[idx].live = LDG_TRUTH_TRUE;
        ctx->domain_cunt = ctx->domain_cunt + 0x1;
        *out = idx;
        return LDG_ERR_AOK;
    }

    return CROSSTALK_EDA_CLK_ERR_FULL;
}

uint32_t crosstalk_eda_clk_domain_drop(crosstalk_eda_clk_ctx_t *ctx, uint32_t idx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (idx >= CROSSTALK_EDA_CLK_DOMAIN_MAX) { return CROSSTALK_EDA_CLK_ERR_DOMAIN; }

    if (ctx->domain[idx].live != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_CLK_ERR_DOMAIN; }

    ctx->domain[idx] = (crosstalk_eda_clk_domain_t)LDG_STRUCT_ZERO_INIT;
    ctx->domain[idx].net = UINT32_MAX;
    ctx->domain_cunt = ctx->domain_cunt - 0x1;
    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_clk_domain_find(const crosstalk_eda_clk_ctx_t *ctx, uint32_t net, uint32_t *out)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;

    for (idx = 0; idx < CROSSTALK_EDA_CLK_DOMAIN_MAX; idx = idx + 0x1)
    {
        if (ctx->domain[idx].live != LDG_TRUTH_TRUE) { continue; }

        if (ctx->domain[idx].net == net)
        {
            *out = idx;
            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t crosstalk_eda_clk_domain_freq_set(crosstalk_eda_clk_ctx_t *ctx, uint32_t idx, uint64_t f_hz)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (idx >= CROSSTALK_EDA_CLK_DOMAIN_MAX || ctx->domain[idx].live != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_CLK_ERR_DOMAIN; }

    if (f_hz == 0) { return CROSSTALK_EDA_CLK_ERR_FREQ; }

    ctx->domain[idx].f_hz = f_hz;
    ctx->domain[idx].phase = 0x0;
    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_clk_domain_freq_get(const crosstalk_eda_clk_ctx_t *ctx, uint32_t idx, uint64_t *out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (idx >= CROSSTALK_EDA_CLK_DOMAIN_MAX || ctx->domain[idx].live != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_CLK_ERR_DOMAIN; }

    *out = ctx->domain[idx].f_hz;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_clk_domain_en_set(crosstalk_eda_clk_ctx_t *ctx, uint32_t idx, uint8_t en)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (idx >= CROSSTALK_EDA_CLK_DOMAIN_MAX || ctx->domain[idx].live != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_CLK_ERR_DOMAIN; }

    ctx->domain[idx].en = (en != LDG_TRUTH_FALSE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_clk_advance(crosstalk_eda_clk_ctx_t *ctx, double dt_secs, uint64_t *out)
{
    uint64_t dt_ns = 0;
    uint64_t steps = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (dt_secs <= 0.0) { return LDG_ERR_AOK; }

    dt_ns = (uint64_t)(dt_secs * (double)LDG_NS_PER_SEC);

    ctx->acc = ctx->acc + dt_ns * ctx->step_hz;
    steps = ctx->acc / (uint64_t)LDG_NS_PER_SEC;
    ctx->acc = ctx->acc % (uint64_t)LDG_NS_PER_SEC;

    if (steps > CROSSTALK_EDA_CLK_STEP_MAX)
    {
        ctx->steps_dropped = ctx->steps_dropped + (steps - CROSSTALK_EDA_CLK_STEP_MAX);
        steps = CROSSTALK_EDA_CLK_STEP_MAX;
    }

    for (idx = 0; idx < CROSSTALK_EDA_CLK_DOMAIN_MAX; idx = idx + 0x1)
    {
        if (ctx->domain[idx].live != LDG_TRUTH_TRUE) { continue; }

        domain_step(&ctx->domain[idx], ctx->step_hz, steps);
    }

    ctx->steps_total = ctx->steps_total + steps;
    *out = steps;
    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_clk_cycles_get(const crosstalk_eda_clk_ctx_t *ctx, uint32_t idx, uint64_t *out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (idx >= CROSSTALK_EDA_CLK_DOMAIN_MAX || ctx->domain[idx].live != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_CLK_ERR_DOMAIN; }

    *out = ctx->domain[idx].cycles;
    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_clk_step_cycles_get(const crosstalk_eda_clk_ctx_t *ctx, uint32_t idx, uint64_t *out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (idx >= CROSSTALK_EDA_CLK_DOMAIN_MAX || ctx->domain[idx].live != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_CLK_ERR_DOMAIN; }

    *out = ctx->domain[idx].cycles_step;
    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_clk_edges_get(const crosstalk_eda_clk_ctx_t *ctx, uint32_t idx, uint64_t *out_rise, uint64_t *out_fall)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_rise == 0x0 || out_fall == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_rise = 0x0;
    *out_fall = 0x0;

    if (idx >= CROSSTALK_EDA_CLK_DOMAIN_MAX || ctx->domain[idx].live != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_CLK_ERR_DOMAIN; }

    *out_rise = ctx->domain[idx].cycles;
    *out_fall = ctx->domain[idx].cycles;
    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_clk_sim_ns_get(const crosstalk_eda_clk_ctx_t *ctx, uint32_t idx, uint64_t *out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (idx >= CROSSTALK_EDA_CLK_DOMAIN_MAX || ctx->domain[idx].live != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_CLK_ERR_DOMAIN; }

    return wide_scale_div(ctx->domain[idx].cycles, (uint64_t)LDG_NS_PER_SEC, ctx->domain[idx].f_hz, out);
}

uint32_t crosstalk_eda_clk_period_ps_get(const crosstalk_eda_clk_ctx_t *ctx, uint32_t idx, uint64_t *out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (idx >= CROSSTALK_EDA_CLK_DOMAIN_MAX || ctx->domain[idx].live != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_CLK_ERR_DOMAIN; }

    return wide_scale_div((uint64_t)LDG_NS_PER_SEC, (uint64_t)LDG_PS_PER_NS, ctx->domain[idx].f_hz, out);
}
