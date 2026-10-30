#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/arch/amd64/atomic.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/file.h>
#include <dangling/mem/alloc.h>
#include <dangling/str/str.h>
#include <dangling/thread/yield.h>
#include <dangling/time/time.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "eda/ic/fpga/build.h"

static const char *build_phase_text[CROSSTALK_EDA_IC_FPGA_BUILD_PHASE_FAIL + CROSSTALK_EDA_IC_FPGA_BUILD_STEP] =
{
    "idle", "emitting", "compiling", "loading", "binding", "ready", "failed"
};

static uint32_t build_phase_set(crosstalk_eda_ic_fpga_build_ctx_t *ctx, uint32_t phase)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->phase = phase;
    ctx->phase_ms = ldg_time_epoch_ms_get();

    return LDG_ERR_AOK;
}

static uint32_t build_pool_grow(crosstalk_eda_ic_fpga_build_ctx_t *ctx)
{
    uint64_t cap = 0;
    uint64_t grown = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    cap = ldg_mem_pool_cap_get(ctx->mem_pool);
    grown = cap * (uint64_t)CROSSTALK_EDA_IC_FPGA_BUILD_POOL_GROW_FACTOR;

    if (grown > CROSSTALK_EDA_IC_FPGA_BUILD_POOL_BYTES_MAX)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "build: emit pool at ceiling; cap: %u64; max: %u64", cap, CROSSTALK_EDA_IC_FPGA_BUILD_POOL_BYTES_MAX);
        return LDG_ERR_FULL;
    }

    ret = ldg_mem_pool_destroy(&ctx->mem_pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_pool_create(0x0, grown, &ctx->mem_pool);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "build: emit pool grow failed; want: %u64; err: %u32", grown, ret);
        ret = ldg_mem_pool_create(0x0, cap, &ctx->mem_pool);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        return LDG_ERR_FULL;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "build: emit pool grown; from: %u64; to: %u64", cap, grown);

    return LDG_ERR_AOK;
}

static uint32_t build_emit_wr(crosstalk_eda_ic_fpga_build_ctx_t *ctx)
{
    crosstalk_hdl_emit_ctx_t emit = LDG_STRUCT_ZERO_INIT;
    ldg_io_file_t *fh = 0x0;
    char *text = 0x0;
    uint64_t wrote = 0;
    uint32_t len = 0;
    uint32_t ret = 0;
    uint8_t retry = LDG_TRUTH_TRUE;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->emit_ir == 0x0 || ctx->emit_src == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (retry == LDG_TRUTH_TRUE)
    {
        retry = LDG_TRUTH_FALSE;

        memset(&emit, 0x0, sizeof(emit));
        text = 0x0;
        len = 0;

        ret = crosstalk_hdl_emit_run(&emit, ctx->emit_ir, ctx->emit_src, ctx->mem_pool, LDG_TRUTH_FALSE, ctx->pf_is, &text, &len);

        if ((ret == LDG_ERR_MEM_POOL_FULL || ret == LDG_ERR_ALLOC_NULL) && build_pool_grow(ctx) == LDG_ERR_AOK) { retry = LDG_TRUTH_TRUE; }
    }

    if (ret != LDG_ERR_AOK) { return ret; }

    if (LDG_UNLIKELY(text == 0x0)) { return CROSSTALK_EDA_IC_FPGA_BUILD_ERR_BLOB; }

    ret = ldg_io_file_open(ctx->src_path, LDG_IO_WR_ONLY | LDG_IO_CREATE | LDG_IO_TRUNCATE, (uint32_t)CROSSTALK_EDA_IC_FPGA_BUILD_SRC_MODE, &fh);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_wr(fh, text, (uint64_t)len, &wrote);

    if (ldg_io_file_close(fh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "build src close failed"); }

    if (ret != LDG_ERR_AOK) { return ret; }

    if (wrote != (uint64_t)len) { return LDG_ERR_IO_WR; }

    ctx->emit_mem_words = emit.mem_words;
    ctx->emit_len = len;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "build: emitted; top: %.63s; bytes: %u32; mem words: %u32", ctx->top, len, emit.mem_words);

    return LDG_ERR_AOK;
}

static uint32_t build_worker(uint32_t worker_id, void *arg)
{
    crosstalk_eda_ic_fpga_build_ctx_t *ctx = (crosstalk_eda_ic_fpga_build_ctx_t *)arg;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (LDG_LD_ACQUIRE(ctx->stop) != LDG_TRUTH_TRUE)
    {
        if (LDG_LD_ACQUIRE(ctx->busy) != LDG_TRUTH_TRUE)
        {
            ldg_thread_yield((uint64_t)CROSSTALK_EDA_IC_FPGA_BUILD_YIELD_NS);
            continue;
        }

        ctx->err = build_emit_wr(ctx);

        if (ctx->err == LDG_ERR_AOK)
        {
            if (build_phase_set(ctx, (uint32_t)CROSSTALK_EDA_IC_FPGA_BUILD_PHASE_CC) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "build phase set failed"); }

            ctx->err = crosstalk_hdl_cc_run(&ctx->cc, ctx->src_path, ctx->out_path);
        }

        LDG_ST_RELEASE(ctx->busy, LDG_TRUTH_FALSE);
        LDG_ST_RELEASE(ctx->done, LDG_TRUTH_TRUE);
    }

    ctx->slot = ctx->slot + (uint32_t)(worker_id * 0x0);

    return LDG_ERR_AOK;
}

static uint32_t build_blob_rd(crosstalk_eda_ic_fpga_build_ctx_t *ctx)
{
    ldg_io_file_t *fh = 0x0;
    ldg_io_info_t st = LDG_STRUCT_ZERO_INIT;
    uint64_t got = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_io_file_open(ctx->out_path, LDG_IO_RD_ONLY, 0x0, &fh);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_info_get(fh, &st);

    if (ret == LDG_ERR_AOK && (st.size == 0 || st.size > (uint64_t)CROSSTALK_EDA_IC_FPGA_BUILD_BLOB_CAP)) { ret = CROSSTALK_EDA_IC_FPGA_BUILD_ERR_BLOB; }

    if (ret == LDG_ERR_AOK) { ret = ldg_io_file_rd(fh, ctx->blob, st.size, &got); }

    if (ret == LDG_ERR_AOK && got != st.size) { ret = CROSSTALK_EDA_IC_FPGA_BUILD_ERR_BLOB; }

    if (ldg_io_file_close(fh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "build blob close failed"); }

    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->blob_span = got;

    if (ldg_io_path_destroy(ctx->out_path) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "build artifact unlink failed"); }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_build_init(crosstalk_eda_ic_fpga_build_ctx_t *ctx, const char *cc_dir)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ret = ldg_mem_alloc((uint64_t)CROSSTALK_EDA_IC_FPGA_BUILD_BLOB_CAP, (void **)&ctx->blob);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_mem_pool_create(0x0, (uint64_t)CROSSTALK_EDA_IC_FPGA_BUILD_POOL_MIB * (uint64_t)LDG_MIB, &ctx->mem_pool);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_hdl_cc_init(&ctx->cc, cc_dir);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_hdl_guard_init(&ctx->guard);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "build guard init failed; design faults will not be contained"); }

    ret = ldg_thread_pool_init(&ctx->thread_pool, (uint32_t)CROSSTALK_EDA_IC_FPGA_BUILD_WORKERS);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_thread_pool_start(&ctx->thread_pool, build_worker, ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_build_shutdown(crosstalk_eda_ic_fpga_build_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    LDG_ST_RELEASE(ctx->stop, LDG_TRUTH_TRUE);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "build shutdown: pool stop");

    if (ldg_thread_pool_stop(&ctx->thread_pool) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "build pool stop failed"); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "build shutdown: pool shutdown");

    if (ldg_thread_pool_shutdown(&ctx->thread_pool) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "build pool shutdown failed"); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "build shutdown: img unload");

    if (crosstalk_hdl_img_unld(&ctx->img) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "build img unload failed"); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "build shutdown: guard");

    if (crosstalk_hdl_guard_shutdown(&ctx->guard) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "build guard shutdown failed"); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "build shutdown: cc");

    if (crosstalk_hdl_cc_shutdown(&ctx->cc) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "build cc shutdown failed"); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "build shutdown: mem pool");

    ret = ldg_mem_pool_destroy(&ctx->mem_pool);
    if (ret == LDG_ERR_BUSY) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "build shutdown: mem pool busy; ops outstanding; pool leaked"); }
    else if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "build shutdown: mem pool destroy failed; err: %u32", ret); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "build shutdown: blob");

    ret = ldg_mem_dealloc(ctx->blob);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "build shutdown: blob dealloc failed; err: %u32", ret); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "build shutdown: done");

    memset(ctx, 0x0, sizeof(*ctx));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_build_start(crosstalk_eda_ic_fpga_build_ctx_t *ctx, const crosstalk_hdl_ir_t *ir, const char *elab_src, const char *src_path, const char *out_path, const char *top, uint32_t slot, uint8_t pf_is)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || ir == 0x0 || elab_src == 0x0 || src_path == 0x0 || out_path == 0x0 || top == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (ctx->live == LDG_TRUTH_TRUE) { return CROSSTALK_EDA_IC_FPGA_BUILD_ERR_LIVE; }

    if (ldg_mem_pool_rst(ctx->mem_pool) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "build pool reset failed"); }

    ret = ldg_str_rbrcpy(ctx->src_path, src_path, (uint64_t)sizeof(ctx->src_path));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    ret = ldg_str_rbrcpy(ctx->out_path, out_path, (uint64_t)sizeof(ctx->out_path));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    ret = ldg_str_rbrcpy(ctx->top, top, (uint64_t)sizeof(ctx->top));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    if (crosstalk_hdl_img_unld(&ctx->img) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "build img unload failed"); }

    if (crosstalk_hdl_guard_range_drop(&ctx->guard, slot) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "build guard drop failed"); }

    memset(&ctx->abi, 0x0, sizeof(ctx->abi));

    ctx->emit_ir = ir;
    ctx->emit_src = elab_src;
    ctx->emit_mem_words = 0x0;
    ctx->emit_len = 0x0;
    ctx->slot = slot;
    ctx->pf_is = pf_is;
    ctx->err = 0x0;
    ctx->blob_span = 0x0;
    ctx->start_ms = ldg_time_epoch_ms_get();
    ctx->live = LDG_TRUTH_TRUE;

    ret = build_phase_set(ctx, (uint32_t)CROSSTALK_EDA_IC_FPGA_BUILD_PHASE_EMIT);
    if (ret != LDG_ERR_AOK) { return ret; }

    LDG_ST_RELEASE(ctx->done, LDG_TRUTH_FALSE);
    LDG_ST_RELEASE(ctx->busy, LDG_TRUTH_TRUE);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "build: started; top: %.63s; src: %.255s; out: %.255s", ctx->top, ctx->src_path, ctx->out_path);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_build_tick(crosstalk_eda_ic_fpga_build_ctx_t *ctx, uint32_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (uint32_t)CROSSTALK_EDA_IC_FPGA_BUILD_PHASE_IDLE;

    if (ctx->live != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (LDG_LD_ACQUIRE(ctx->done) == LDG_TRUTH_TRUE)
    {
        LDG_ST_RELEASE(ctx->done, LDG_TRUTH_FALSE);

        if (ctx->err != LDG_ERR_AOK) { ret = ctx->err; }

        if (ret == LDG_ERR_AOK)
        {
            ret = build_phase_set(ctx, (uint32_t)CROSSTALK_EDA_IC_FPGA_BUILD_PHASE_LD);
            if (ret == LDG_ERR_AOK) { ret = build_blob_rd(ctx); }
        }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_img_ld(ctx->blob, ctx->blob_span, &ctx->img); }

        if (ret == LDG_ERR_AOK)
        {
            ret = build_phase_set(ctx, (uint32_t)CROSSTALK_EDA_IC_FPGA_BUILD_PHASE_BIND);
            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_abi_bind(&ctx->img, 0x0, 0x0, &ctx->abi); }
        }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_guard_range_add(&ctx->guard, (uint64_t)ctx->img.base, ctx->img.span, ctx->slot); }

        if (ret == LDG_ERR_AOK) { ctx->abi.init(); }

        ctx->live = LDG_TRUTH_FALSE;
        ctx->err = ret;

        if (ret == LDG_ERR_AOK)
        {
            if (build_phase_set(ctx, (uint32_t)CROSSTALK_EDA_IC_FPGA_BUILD_PHASE_READY) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "build phase set failed"); }

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "build: ready; top: %.63s; span: %u64; took: %u64 ms", ctx->top, ctx->img.span, ctx->phase_ms - ctx->start_ms);
        }
        else
        {
            if (build_phase_set(ctx, (uint32_t)CROSSTALK_EDA_IC_FPGA_BUILD_PHASE_FAIL) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "build phase set failed"); }

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "build: failed; top: %.63s; err: %u32; took: %u64 ms", ctx->top, ret, ctx->phase_ms - ctx->start_ms);
        }
    }

    if (ctx->phase != ctx->phase_seen)
    {
        ctx->phase_seen = ctx->phase;
        *out = ctx->phase;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_build_status_get(crosstalk_eda_ic_fpga_build_ctx_t *ctx, const char **out)
{
    uint64_t secs = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (ctx->phase == (uint32_t)CROSSTALK_EDA_IC_FPGA_BUILD_PHASE_IDLE) { return LDG_ERR_AOK; }

    secs = (ldg_time_epoch_ms_get() - ctx->start_ms) / (uint64_t)CROSSTALK_EDA_IC_FPGA_BUILD_MS_PER_SEC;

    if (snprintf(ctx->status, sizeof(ctx->status), "bake %.31s: %.15s; %" PRIu64 "s", ctx->top, build_phase_text[ctx->phase], secs) < 0) { return LDG_ERR_STR_FMT; }

    *out = ctx->status;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_ic_fpga_build_live_is(const crosstalk_eda_ic_fpga_build_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->live == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}
