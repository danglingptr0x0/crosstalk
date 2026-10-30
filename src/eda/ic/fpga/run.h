#ifndef CROSSTALK_EDA_IC_FPGA_RUN_H
#define CROSSTALK_EDA_IC_FPGA_RUN_H

#include <stdint.h>

#include <dangling/thread/pool.h>

#include "eda/clk/clk.h"
#include "eda/ic/fpga/fpga.h"
#include "eda/logic/stream.h"

#define CROSSTALK_EDA_IC_FPGA_RUN_ERR_LIVE 933
#define CROSSTALK_EDA_IC_FPGA_RUN_ERR_IDLE 934

#define CROSSTALK_EDA_IC_FPGA_RUN_WORKERS 1
#define CROSSTALK_EDA_IC_FPGA_RUN_STEP 1
#define CROSSTALK_EDA_IC_FPGA_RUN_CHUNK_CYCLES 65536
#define CROSSTALK_EDA_IC_FPGA_RUN_HLT_WAIT_NS 100000
#define CROSSTALK_EDA_IC_FPGA_RUN_IDLE_YIELD_NS 1000
#define CROSSTALK_EDA_IC_FPGA_RUN_CPUSET_CAP 8
#define CROSSTALK_EDA_IC_FPGA_RUN_TUNE_NS 250000000
#define CROSSTALK_EDA_IC_FPGA_RUN_TUNE_HZ_MIN 1000
#define CROSSTALK_EDA_IC_FPGA_RUN_TUNE_CLIMB_DIV 8
#define CROSSTALK_EDA_IC_FPGA_RUN_TUNE_DEADBAND_DIV 16
#define CROSSTALK_EDA_IC_FPGA_RUN_LOG_SECS 10
#define CROSSTALK_EDA_IC_FPGA_RUN_LOG_NS ((uint64_t)CROSSTALK_EDA_IC_FPGA_RUN_LOG_SECS * (uint64_t)LDG_NS_PER_SEC)

typedef struct crosstalk_eda_ic_fpga_run_ctx
{
    ldg_thread_pool_t pool;
    crosstalk_eda_ic_fpga_ctx_t *fpga;
    crosstalk_eda_clk_ctx_t *clk;
    crosstalk_eda_logic_stream_ctx_t *stream;
    crosstalk_eda_logic_probe_ctx_t *probes;
    uint64_t cycles_total;
    uint64_t steps_total;
    uint64_t tune_mark;
    uint64_t tune_cycles;
    uint64_t tune_hz;
    uint64_t tune_hz_cap;
    uint64_t pin_hz;
    uint64_t log_mark;
    uint64_t log_cycles;
    uint64_t rate_hz;
    uint64_t iters_total;
    uint64_t log_iters;
    uint64_t busy_ns;
    uint64_t log_busy_ns;
    uint32_t stop;
    uint32_t worker_id;
    uint32_t alive;
    uint32_t cpuset_id[CROSSTALK_EDA_IC_FPGA_RUN_CPUSET_CAP];
    uint32_t cpuset_cunt;
    uint8_t live;
    uint8_t ok;
    uint8_t pudding[2];
} crosstalk_eda_ic_fpga_run_ctx_t;

uint32_t crosstalk_eda_ic_fpga_run_init(crosstalk_eda_ic_fpga_run_ctx_t *ctx, crosstalk_eda_ic_fpga_ctx_t *fpga, crosstalk_eda_clk_ctx_t *clk);
uint32_t crosstalk_eda_ic_fpga_run_shutdown(crosstalk_eda_ic_fpga_run_ctx_t *ctx);
uint32_t crosstalk_eda_ic_fpga_run_stream_bind(crosstalk_eda_ic_fpga_run_ctx_t *ctx, crosstalk_eda_logic_stream_ctx_t *stream, crosstalk_eda_logic_probe_ctx_t *probes);
uint32_t crosstalk_eda_ic_fpga_run_start(crosstalk_eda_ic_fpga_run_ctx_t *ctx);
uint32_t crosstalk_eda_ic_fpga_run_hlt(crosstalk_eda_ic_fpga_run_ctx_t *ctx);
uint32_t crosstalk_eda_ic_fpga_run_cycles_get(const crosstalk_eda_ic_fpga_run_ctx_t *ctx, uint64_t *out_cycles);
uint32_t crosstalk_eda_ic_fpga_run_rate_get(const crosstalk_eda_ic_fpga_run_ctx_t *ctx, uint64_t *out_hz);
uint32_t crosstalk_eda_ic_fpga_run_pin_set(crosstalk_eda_ic_fpga_run_ctx_t *ctx, uint64_t hz);
uint32_t crosstalk_eda_ic_fpga_run_cpuset_set(crosstalk_eda_ic_fpga_run_ctx_t *ctx, const uint32_t *id, uint32_t cunt);
uint32_t crosstalk_eda_ic_fpga_run_pin_get(const crosstalk_eda_ic_fpga_run_ctx_t *ctx, uint64_t *out);
uint8_t crosstalk_eda_ic_fpga_run_live_is(const crosstalk_eda_ic_fpga_run_ctx_t *ctx);

#endif
