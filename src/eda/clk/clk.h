#ifndef CROSSTALK_EDA_CLK_H
#define CROSSTALK_EDA_CLK_H

#include <stdint.h>

#include <dangling/core/macros.h>

#define CROSSTALK_EDA_CLK_ERR_DOMAIN 960
#define CROSSTALK_EDA_CLK_ERR_FULL 961
#define CROSSTALK_EDA_CLK_ERR_FREQ 962
#define CROSSTALK_EDA_CLK_ERR_STEP 963

#define CROSSTALK_EDA_CLK_DOMAIN_MAX 16
#define CROSSTALK_EDA_CLK_STEP_HZ_DEFAULT 120
#define CROSSTALK_EDA_CLK_STEP_MAX 32

typedef struct crosstalk_eda_clk_domain
{
    uint64_t f_hz;
    uint64_t cycles;
    uint64_t cycles_step;
    uint64_t phase;
    uint32_t net;
    uint8_t en;
    uint8_t live;
    uint8_t pudding[2];
} crosstalk_eda_clk_domain_t;

typedef struct crosstalk_eda_clk_ctx
{
    crosstalk_eda_clk_domain_t domain[CROSSTALK_EDA_CLK_DOMAIN_MAX];
    uint64_t acc;
    uint64_t steps_total;
    uint64_t steps_dropped;
    uint64_t step_hz;
    uint32_t domain_cunt;
    uint8_t ok;
    uint8_t pudding[3];
} crosstalk_eda_clk_ctx_t;

uint32_t crosstalk_eda_clk_init(crosstalk_eda_clk_ctx_t *ctx, uint64_t step_hz);
uint32_t crosstalk_eda_clk_shutdown(crosstalk_eda_clk_ctx_t *ctx);
uint32_t crosstalk_eda_clk_domain_add(crosstalk_eda_clk_ctx_t *ctx, uint32_t net, uint64_t f_hz, uint32_t *out);
uint32_t crosstalk_eda_clk_domain_drop(crosstalk_eda_clk_ctx_t *ctx, uint32_t idx);
uint32_t crosstalk_eda_clk_domain_find(const crosstalk_eda_clk_ctx_t *ctx, uint32_t net, uint32_t *out);
uint32_t crosstalk_eda_clk_domain_freq_set(crosstalk_eda_clk_ctx_t *ctx, uint32_t idx, uint64_t f_hz);
uint32_t crosstalk_eda_clk_domain_freq_get(const crosstalk_eda_clk_ctx_t *ctx, uint32_t idx, uint64_t *out);
uint32_t crosstalk_eda_clk_domain_en_set(crosstalk_eda_clk_ctx_t *ctx, uint32_t idx, uint8_t en);
uint32_t crosstalk_eda_clk_advance(crosstalk_eda_clk_ctx_t *ctx, double dt_secs, uint64_t *out);
uint32_t crosstalk_eda_clk_cycles_get(const crosstalk_eda_clk_ctx_t *ctx, uint32_t idx, uint64_t *out);
uint32_t crosstalk_eda_clk_step_cycles_get(const crosstalk_eda_clk_ctx_t *ctx, uint32_t idx, uint64_t *out);
uint32_t crosstalk_eda_clk_edges_get(const crosstalk_eda_clk_ctx_t *ctx, uint32_t idx, uint64_t *out_rise, uint64_t *out_fall);
uint32_t crosstalk_eda_clk_sim_ns_get(const crosstalk_eda_clk_ctx_t *ctx, uint32_t idx, uint64_t *out);
uint32_t crosstalk_eda_clk_period_ps_get(const crosstalk_eda_clk_ctx_t *ctx, uint32_t idx, uint64_t *out);

#endif
