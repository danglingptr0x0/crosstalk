#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/file.h>
#include <dangling/io/path.h>
#include <dangling/mem/alloc.h>
#include <dangling/sys/scan.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "core/scan.h"

#define SCAN_STEP 1

uint32_t crosstalk_core_scan_init(crosstalk_core_scan_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ret = ldg_sys_scan_ctx_create(CROSSTALK_CORE_SCAN_APP_NAME, &ctx->provider);
    if (ret != LDG_ERR_AOK)
    {
        ctx->provider = 0x0;

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scan: no antimalware provider; err: %u32", ret);

        return CROSSTALK_CORE_SCAN_ERR_PROVIDER;
    }

    ctx->ok = LDG_TRUTH_TRUE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "scan: antimalware provider bound");

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_scan_shutdown(crosstalk_core_scan_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->provider != 0x0 && ldg_sys_scan_ctx_destroy(&ctx->provider) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scan provider release failed"); }

    memset(ctx, 0x0, sizeof(*ctx));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_scan_file(crosstalk_core_scan_ctx_t *ctx, const char *path, uint32_t *out_verdict)
{
    ldg_io_info_t st = LDG_STRUCT_ZERO_INIT;
    ldg_io_file_t *file = 0x0;
    ldg_byte_t *buff = 0x0;
    uint64_t bytes_rd = 0;
    uint32_t ret = 0;
    uint8_t clean = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0 || path == 0x0 || out_verdict == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_verdict = (uint32_t)CROSSTALK_CORE_SCAN_VERDICT_CLEAN;

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ret = ldg_io_path_info_get(path, &st);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (st.size == 0 || st.size > (uint64_t)CROSSTALK_CORE_SCAN_SPAN_MAX)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scan: span refused; path: %.191s; bytes: %u64", path, st.size);

        return CROSSTALK_CORE_SCAN_ERR_SPAN;
    }

    ret = ldg_mem_alloc(st.size, (void **)&buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, 0x0, &file);
    if (ret == LDG_ERR_AOK)
    {
        ret = ldg_io_file_rd(file, buff, st.size, &bytes_rd);

        if (ldg_io_file_close(file) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scan file close failed"); }
    }

    if (ret == LDG_ERR_AOK && bytes_rd != st.size) { ret = LDG_ERR_IO_RD; }

    if (ret != LDG_ERR_AOK)
    {
        if (ldg_mem_dealloc(buff) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scan buffer release failed"); }

        return ret;
    }

    ctx->runs = ctx->runs + (uint64_t)SCAN_STEP;

    clean = ldg_sys_scan_clean_is(ctx->provider, buff, st.size, CROSSTALK_CORE_SCAN_APP_NAME);

    if (ldg_mem_dealloc(buff) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scan buffer release failed"); }

    if (clean == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (clean != LDG_TRUTH_FALSE)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scan: provider call failed; path: %.191s; err: %u32", path, (uint32_t)clean);

        return CROSSTALK_CORE_SCAN_ERR_EXIT;
    }

    *out_verdict = (uint32_t)CROSSTALK_CORE_SCAN_VERDICT_DETECTED;
    ctx->hits = ctx->hits + (uint64_t)SCAN_STEP;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scan: detected; path: %.191s", path);

    return LDG_ERR_AOK;
}

uint8_t crosstalk_core_scan_ready_is(const crosstalk_core_scan_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->ok == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}
