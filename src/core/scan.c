#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <windows.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/file.h>
#include <dangling/io/path.h>
#include <dangling/mem/alloc.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "core/scan.h"

#define SCAN_STEP 1

typedef struct scan_amsi_ctx_opaque *scan_amsi_ctx_t;
typedef struct scan_amsi_sesh_opaque *scan_amsi_sesh_t;

typedef HRESULT (WINAPI *scan_amsi_init_cb)(LPCWSTR app_name, scan_amsi_ctx_t *out);
typedef void (WINAPI *scan_amsi_uninit_cb)(scan_amsi_ctx_t amsi);
typedef HRESULT (WINAPI *scan_amsi_open_cb)(scan_amsi_ctx_t amsi, scan_amsi_sesh_t *out);
typedef void (WINAPI *scan_amsi_close_cb)(scan_amsi_ctx_t amsi, scan_amsi_sesh_t sesh);
typedef HRESULT (WINAPI *scan_amsi_scan_cb)(scan_amsi_ctx_t amsi, void *buff, ULONG len, LPCWSTR name, scan_amsi_sesh_t sesh, uint32_t *out);

uint32_t crosstalk_core_scan_init(crosstalk_core_scan_ctx_t *ctx)
{
    scan_amsi_init_cb init_cb = 0x0;
    scan_amsi_ctx_t amsi = 0x0;
    HMODULE lib = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    lib = LoadLibraryA(CROSSTALK_CORE_SCAN_AMSI_DLL);
    if (lib == 0x0)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scan: no amsi provider; dll: %.31s", CROSSTALK_CORE_SCAN_AMSI_DLL);

        return CROSSTALK_CORE_SCAN_ERR_PROVIDER;
    }

    ctx->provider = (void *)lib;
    ctx->init_cb = (void *)GetProcAddress(lib, CROSSTALK_CORE_SCAN_AMSI_INIT);
    ctx->uninit_cb = (void *)GetProcAddress(lib, CROSSTALK_CORE_SCAN_AMSI_UNINIT);
    ctx->open_cb = (void *)GetProcAddress(lib, CROSSTALK_CORE_SCAN_AMSI_OPEN);
    ctx->close_cb = (void *)GetProcAddress(lib, CROSSTALK_CORE_SCAN_AMSI_CLOSE);
    ctx->scan_cb = (void *)GetProcAddress(lib, CROSSTALK_CORE_SCAN_AMSI_SCAN);

    if (ctx->init_cb == 0x0 || ctx->uninit_cb == 0x0 || ctx->scan_cb == 0x0)
    {
        if (FreeLibrary(lib) == 0) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scan amsi free failed"); }

        memset(ctx, 0x0, sizeof(*ctx));

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scan: amsi exports missing");

        return CROSSTALK_CORE_SCAN_ERR_PROVIDER;
    }

    init_cb = (scan_amsi_init_cb)ctx->init_cb;

    if (init_cb(L"" CROSSTALK_CORE_SCAN_APP_NAME, &amsi) != S_OK || amsi == 0x0)
    {
        if (FreeLibrary(lib) == 0) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scan amsi free failed"); }

        memset(ctx, 0x0, sizeof(*ctx));

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scan: amsi init refused");

        return CROSSTALK_CORE_SCAN_ERR_PROVIDER;
    }

    ctx->ctx_handle = (void *)amsi;
    ctx->ok = LDG_TRUTH_TRUE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "scan: amsi provider bound");

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_scan_shutdown(crosstalk_core_scan_ctx_t *ctx)
{
    scan_amsi_uninit_cb uninit_cb = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ctx_handle != 0x0 && ctx->uninit_cb != 0x0)
    {
        uninit_cb = (scan_amsi_uninit_cb)ctx->uninit_cb;

        uninit_cb((scan_amsi_ctx_t)ctx->ctx_handle);
    }

    if (ctx->provider != 0x0) { if (FreeLibrary((HMODULE)ctx->provider) == 0) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scan amsi free failed"); } }

    memset(ctx, 0x0, sizeof(*ctx));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_scan_file(crosstalk_core_scan_ctx_t *ctx, const char *path, uint32_t *out_verdict)
{
    scan_amsi_scan_cb scan_cb = 0x0;
    scan_amsi_open_cb open_cb = 0x0;
    scan_amsi_close_cb close_cb = 0x0;
    scan_amsi_sesh_t sesh = 0x0;
    ldg_io_info_t st = LDG_STRUCT_ZERO_INIT;
    ldg_io_file_t *file = 0x0;
    ldg_byte_t *buff = 0x0;
    uint64_t bytes_rd = 0;
    uint32_t verdict = 0;
    uint32_t ret = 0;

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
        ldg_mem_dealloc(buff);

        return ret;
    }

    ctx->runs = ctx->runs + (uint64_t)SCAN_STEP;

    if (ctx->open_cb != 0x0)
    {
        open_cb = (scan_amsi_open_cb)ctx->open_cb;

        if (open_cb((scan_amsi_ctx_t)ctx->ctx_handle, &sesh) != S_OK) { sesh = 0x0; }
    }

    scan_cb = (scan_amsi_scan_cb)ctx->scan_cb;

    if (scan_cb((scan_amsi_ctx_t)ctx->ctx_handle, buff, (ULONG)st.size, L"" CROSSTALK_CORE_SCAN_APP_NAME, sesh, &verdict) != S_OK) { ret = CROSSTALK_CORE_SCAN_ERR_EXIT; }

    if (sesh != 0x0 && ctx->close_cb != 0x0)
    {
        close_cb = (scan_amsi_close_cb)ctx->close_cb;

        close_cb((scan_amsi_ctx_t)ctx->ctx_handle, sesh);
    }

    ldg_mem_dealloc(buff);

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scan: amsi call failed; path: %.191s", path);

        return ret;
    }

    if (verdict >= (uint32_t)CROSSTALK_CORE_SCAN_AMSI_DETECTED)
    {
        *out_verdict = (uint32_t)CROSSTALK_CORE_SCAN_VERDICT_DETECTED;
        ctx->hits = ctx->hits + (uint64_t)SCAN_STEP;

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scan: detected; path: %.191s; amsi: %u32", path, verdict);

        return LDG_ERR_AOK;
    }

    return LDG_ERR_AOK;
}

uint8_t crosstalk_core_scan_ready_is(const crosstalk_core_scan_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->ok == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}
