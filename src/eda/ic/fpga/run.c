#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <dangling/arch/amd64/atomic.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/sys/cpuset.h>
#include <dangling/thread/yield.h>
#include <dangling/time/time.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "eda/ic/fpga/run.h"

static uint32_t run_worker(uint32_t worker_id, void *arg)
{
    crosstalk_eda_ic_fpga_run_ctx_t *ctx = (crosstalk_eda_ic_fpga_run_ctx_t *)arg;
    uint64_t mark = 0;
    uint32_t alive_now = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    alive_now = LDG_ADD_FETCH(ctx->alive, 0x1);

    if (ctx->cpuset_cunt != 0)
    {
        ret = ldg_sys_cpuset_thread_self_set(ctx->cpuset_id, ctx->cpuset_cunt);

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "fpga run: worker cpuset; first: %u32; cunt: %u32; err: %u32", ctx->cpuset_id[0], ctx->cpuset_cunt, ret);
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga run: worker enter; id: %u32; alive: %u32", worker_id, alive_now);

    if (LDG_LD_ACQUIRE(ctx->stop) == LDG_TRUTH_TRUE)
    {
        alive_now = LDG_SUB_FETCH(ctx->alive, 0x1);
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga run: worker exit; id: %u32; alive: %u32", worker_id, alive_now);
        return LDG_ERR_AOK;
    }

    mark = ldg_time_epoch_ns_get();
    ctx->tune_mark = mark;
    ctx->tune_cycles = ctx->cycles_total;
    ctx->log_mark = mark;
    ctx->log_cycles = ctx->cycles_total;

    while (LDG_LD_ACQUIRE(ctx->stop) != LDG_TRUTH_TRUE)
    {
        uint64_t now = ldg_time_epoch_ns_get();
        uint64_t steps = 0;
        uint32_t slot_idx = 0;

        if (crosstalk_eda_clk_advance(ctx->clk, (double)(now - mark) / (double)LDG_NS_PER_SEC, &steps) != LDG_ERR_AOK) { steps = 0; }

        mark = now;
        ctx->iters_total = ctx->iters_total + 0x1;

        if (steps == 0)
        {
            if (ldg_thread_yield((uint64_t)CROSSTALK_EDA_IC_FPGA_RUN_IDLE_YIELD_NS) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga run idle yield failed"); }

            continue;
        }

        ctx->steps_total = ctx->steps_total + steps;

        for (slot_idx = 0; slot_idx < CROSSTALK_EDA_IC_FPGA_SLOT_MAX; slot_idx = slot_idx + (uint32_t)CROSSTALK_EDA_IC_FPGA_RUN_STEP)
        {
            uint64_t want = 0;
            uint64_t done = 0;

            if (LDG_LD_ACQUIRE(ctx->stop) == LDG_TRUTH_TRUE) { break; }

            if (crosstalk_eda_ic_fpga_slot_live_is(ctx->fpga, slot_idx) != LDG_TRUTH_TRUE) { continue; }

            if (crosstalk_eda_clk_step_cycles_get(ctx->clk, ctx->fpga->slot[slot_idx].clk_domain, &want) != LDG_ERR_AOK) { continue; }

            {
                uint64_t busy_mark = ldg_time_epoch_ns_get();
                uint64_t remaining = want;

                ret = LDG_ERR_AOK;

                while (remaining != 0)
                {
                    uint64_t ask = remaining > (uint64_t)CROSSTALK_EDA_IC_FPGA_RUN_CHUNK_CYCLES ? (uint64_t)CROSSTALK_EDA_IC_FPGA_RUN_CHUNK_CYCLES : remaining;
                    uint64_t chunk_done = 0;

                    ret = crosstalk_eda_ic_fpga_cycles_run(ctx->fpga, slot_idx, ask, &chunk_done);
                    if (ret != LDG_ERR_AOK) { break; }

                    done = done + chunk_done;
                    remaining = remaining - ask;

                    if (LDG_LD_ACQUIRE(ctx->stop) == LDG_TRUTH_TRUE) { break; }
                }

                ctx->busy_ns = ctx->busy_ns + (ldg_time_epoch_ns_get() - busy_mark);

                if (ret != LDG_ERR_AOK) { continue; }
            }

            ctx->cycles_total = ctx->cycles_total + done;

            if (ctx->fpga->slot[slot_idx].comp.f_op_hz > ctx->tune_hz_cap) { ctx->tune_hz_cap = ctx->fpga->slot[slot_idx].comp.f_op_hz; }
        }


        if (now - ctx->tune_mark >= (uint64_t)CROSSTALK_EDA_IC_FPGA_RUN_TUNE_NS)
        {
            uint64_t span = now - ctx->tune_mark;
            uint64_t made = ctx->cycles_total - ctx->tune_cycles;
            uint64_t hz = (made * (uint64_t)LDG_NS_PER_SEC) / span;

            ctx->tune_mark = now;
            ctx->tune_cycles = ctx->cycles_total;
            ctx->rate_hz = hz;

            if (ctx->pin_hz != 0) { hz = ctx->pin_hz; }
            else
            {
                hz = hz + hz / (uint64_t)CROSSTALK_EDA_IC_FPGA_RUN_TUNE_CLIMB_DIV;

                if (hz < (uint64_t)CROSSTALK_EDA_IC_FPGA_RUN_TUNE_HZ_MIN) { hz = (uint64_t)CROSSTALK_EDA_IC_FPGA_RUN_TUNE_HZ_MIN; }

                if (ctx->tune_hz_cap != 0 && hz > ctx->tune_hz_cap) { hz = ctx->tune_hz_cap; }
            }

            if ((hz > ctx->tune_hz ? hz - ctx->tune_hz : ctx->tune_hz - hz) > ctx->tune_hz / (uint64_t)CROSSTALK_EDA_IC_FPGA_RUN_TUNE_DEADBAND_DIV)
            {
                for (slot_idx = 0; slot_idx < CROSSTALK_EDA_IC_FPGA_SLOT_MAX; slot_idx = slot_idx + (uint32_t)CROSSTALK_EDA_IC_FPGA_RUN_STEP)
                {
                    if (crosstalk_eda_ic_fpga_slot_live_is(ctx->fpga, slot_idx) != LDG_TRUTH_TRUE) { continue; }

                    if (crosstalk_eda_clk_domain_freq_set(ctx->clk, ctx->fpga->slot[slot_idx].clk_domain, hz) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga run clk tune failed"); }
                }

                ctx->tune_hz = hz;
            }

            if (now - ctx->log_mark >= (uint64_t)CROSSTALK_EDA_IC_FPGA_RUN_LOG_NS)
            {
                uint64_t log_span = now - ctx->log_mark;
                uint64_t mean = ((ctx->cycles_total - ctx->log_cycles) * (uint64_t)LDG_NS_PER_SEC) / log_span;

                uint64_t iters = ((ctx->iters_total - ctx->log_iters) * (uint64_t)LDG_NS_PER_SEC) / log_span;
                uint64_t busy_pct = ((ctx->busy_ns - ctx->log_busy_ns) * 100) / log_span;

                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga run: clk; hz: %u64; mean: %u64; pin: %u64; cap: %u64; cycles: %u64; iters/s: %u64; busy: %u64%%", ctx->tune_hz, mean, ctx->pin_hz, ctx->tune_hz_cap, ctx->cycles_total, iters, busy_pct);

                ctx->log_mark = now;
                ctx->log_cycles = ctx->cycles_total;
                ctx->log_iters = ctx->iters_total;
                ctx->log_busy_ns = ctx->busy_ns;
            }
        }
    }

    ctx->worker_id = worker_id;

    alive_now = LDG_SUB_FETCH(ctx->alive, 0x1);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga run: worker exit; id: %u32; alive: %u32", worker_id, alive_now);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_run_pin_set(crosstalk_eda_ic_fpga_run_ctx_t *ctx, uint64_t hz)
{
    uint32_t slot_idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->pin_hz = hz;

    if (hz == 0)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga run: clk unpinned; tuning resumes");
        return LDG_ERR_AOK;
    }

    for (slot_idx = 0; slot_idx < CROSSTALK_EDA_IC_FPGA_SLOT_MAX; slot_idx = slot_idx + (uint32_t)CROSSTALK_EDA_IC_FPGA_RUN_STEP)
    {
        if (crosstalk_eda_ic_fpga_slot_live_is(ctx->fpga, slot_idx) != LDG_TRUTH_TRUE) { continue; }

        if (crosstalk_eda_clk_domain_freq_set(ctx->clk, ctx->fpga->slot[slot_idx].clk_domain, hz) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga run clk pin failed"); }
    }

    ctx->tune_hz = hz;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga run: clk pinned; hz: %u64", hz);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_run_cpuset_set(crosstalk_eda_ic_fpga_run_ctx_t *ctx, const uint32_t *id, uint32_t cunt)
{
    if (LDG_UNLIKELY(ctx == 0x0 || id == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (cunt > (uint32_t)CROSSTALK_EDA_IC_FPGA_RUN_CPUSET_CAP) { return LDG_ERR_BOUNDS; }

    for (uint32_t at = 0; at < cunt; at = at + 0x1) { ctx->cpuset_id[at] = id[at]; }

    ctx->cpuset_cunt = cunt;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_run_pin_get(const crosstalk_eda_ic_fpga_run_ctx_t *ctx, uint64_t *out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = ctx->pin_hz;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_run_init(crosstalk_eda_ic_fpga_run_ctx_t *ctx, crosstalk_eda_ic_fpga_ctx_t *fpga, crosstalk_eda_clk_ctx_t *clk)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || fpga == 0x0 || clk == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->fpga = fpga;
    ctx->clk = clk;
    ctx->cpuset_cunt = 0;

    ret = ldg_thread_pool_init(&ctx->pool, (uint32_t)CROSSTALK_EDA_IC_FPGA_RUN_WORKERS);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_thread_pool_priority_set(&ctx->pool, LDG_THREAD_POOL_PRIORITY_NORMAL);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "fpga run: worker priority; lvl: normal; err: %u32", ret);

    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_run_shutdown(crosstalk_eda_ic_fpga_run_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (crosstalk_eda_ic_fpga_run_hlt(ctx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga run halt failed"); }

    if (ldg_thread_pool_shutdown(&ctx->pool) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga run pool shutdown failed"); }

    memset(ctx, 0x0, sizeof(*ctx));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_run_stream_bind(crosstalk_eda_ic_fpga_run_ctx_t *ctx, crosstalk_eda_logic_stream_ctx_t *stream, crosstalk_eda_logic_probe_ctx_t *probes)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->stream = stream;
    ctx->probes = probes;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_run_start(crosstalk_eda_ic_fpga_run_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (ctx->live == LDG_TRUTH_TRUE) { return CROSSTALK_EDA_IC_FPGA_RUN_ERR_LIVE; }

    while (LDG_LD_ACQUIRE(ctx->alive) != 0) { if (ldg_thread_yield((uint64_t)CROSSTALK_EDA_IC_FPGA_RUN_HLT_WAIT_NS) != LDG_ERR_AOK) { break; } }

    LDG_ST_RELEASE(ctx->stop, LDG_TRUTH_FALSE);

    ret = ldg_thread_pool_start(&ctx->pool, run_worker, ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->live = LDG_TRUTH_TRUE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga run: started");

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_run_hlt(crosstalk_eda_ic_fpga_run_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    LDG_ST_RELEASE(ctx->stop, LDG_TRUTH_TRUE);

    if (ldg_thread_pool_stop(&ctx->pool) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga run pool stop failed"); }

    while (LDG_LD_ACQUIRE(ctx->alive) != 0) { if (ldg_thread_yield((uint64_t)CROSSTALK_EDA_IC_FPGA_RUN_HLT_WAIT_NS) != LDG_ERR_AOK) { break; } }

    ctx->live = LDG_TRUTH_FALSE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga run: halted; worker: %u32; cycles: %u64; steps: %u64", ctx->worker_id, ctx->cycles_total, ctx->steps_total);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_run_cycles_get(const crosstalk_eda_ic_fpga_run_ctx_t *ctx, uint64_t *out_cycles)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_cycles == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cycles = ctx->cycles_total;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_run_rate_get(const crosstalk_eda_ic_fpga_run_ctx_t *ctx, uint64_t *out_hz)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_hz == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_hz = ctx->rate_hz;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_ic_fpga_run_live_is(const crosstalk_eda_ic_fpga_run_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->live == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}
