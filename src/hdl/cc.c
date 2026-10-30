#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <stdio.h>
#include <windows.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/file.h>
#include <dangling/io/path.h>
#include <dangling/str/str.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "hdl/cc.h"

static uint32_t cc_argv_build(const crosstalk_hdl_cc_ctx_t *ctx, const char *src_path, const char *out_path, const char **argv, uint32_t cap)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || src_path == 0x0 || out_path == 0x0 || argv == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (cap < (uint32_t)CROSSTALK_HDL_CC_ARG_MAX) { return CROSSTALK_HDL_ERR_CC_ARG; }

    argv[at] = ctx->exe;
    at = at + (uint32_t)CROSSTALK_HDL_CC_STEP;

    argv[at] = CROSSTALK_HDL_CC_TGT_WIN;
    at = at + (uint32_t)CROSSTALK_HDL_CC_STEP;

    argv[at] = CROSSTALK_HDL_CC_OPT;
    at = at + (uint32_t)CROSSTALK_HDL_CC_STEP;

    argv[at] = CROSSTALK_HDL_CC_ARCH;
    at = at + (uint32_t)CROSSTALK_HDL_CC_STEP;

    argv[at] = CROSSTALK_HDL_CC_FREE;
    at = at + (uint32_t)CROSSTALK_HDL_CC_STEP;

    argv[at] = CROSSTALK_HDL_CC_NOBUILTIN;
    at = at + (uint32_t)CROSSTALK_HDL_CC_STEP;

    argv[at] = CROSSTALK_HDL_CC_NOLIB;
    at = at + (uint32_t)CROSSTALK_HDL_CC_STEP;

    argv[at] = CROSSTALK_HDL_CC_SHARED;
    at = at + (uint32_t)CROSSTALK_HDL_CC_STEP;

    argv[at] = CROSSTALK_HDL_CC_LD;
    at = at + (uint32_t)CROSSTALK_HDL_CC_STEP;

    argv[at] = CROSSTALK_HDL_CC_NOENTRY;
    at = at + (uint32_t)CROSSTALK_HDL_CC_STEP;

    argv[at] = CROSSTALK_HDL_CC_NOPROBE;
    at = at + (uint32_t)CROSSTALK_HDL_CC_STEP;

    argv[at] = src_path;
    at = at + (uint32_t)CROSSTALK_HDL_CC_STEP;

    argv[at] = CROSSTALK_HDL_CC_OUT_FLAG;
    at = at + (uint32_t)CROSSTALK_HDL_CC_STEP;

    argv[at] = out_path;
    at = at + (uint32_t)CROSSTALK_HDL_CC_STEP;

    argv[at] = 0x0;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_cc_init(crosstalk_hdl_cc_ctx_t *ctx, const char *dir)
{
    char probe[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    if (dir != 0x0 && dir[0] != LDG_STR_TERM)
    {
        ret = ldg_io_path_join(dir, CROSSTALK_HDL_CC_EXE_WIN, probe, (uint64_t)sizeof(probe));
        if (ret != LDG_ERR_AOK) { return ret; }

        if (ldg_io_path_exists_is(probe) == LDG_TRUTH_TRUE)
        {
            ret = ldg_str_rbrcpy(ctx->exe, probe, (uint64_t)sizeof(ctx->exe));
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

            ctx->ok = LDG_TRUTH_TRUE;

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "cc: shipped compiler; path: %.255s", ctx->exe);

            return LDG_ERR_AOK;
        }
    }

    ret = ldg_str_rbrcpy(ctx->exe, CROSSTALK_HDL_CC_EXE_WIN, (uint64_t)sizeof(ctx->exe));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    ctx->ok = LDG_TRUTH_TRUE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "cc: no shipped compiler; falling back to PATH; exe: %.63s", ctx->exe);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_cc_shutdown(crosstalk_hdl_cc_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_cc_run(crosstalk_hdl_cc_ctx_t *ctx, const char *src_path, const char *out_path)
{
    const char *argv[CROSSTALK_HDL_CC_ARG_MAX];
    uint32_t code = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || src_path == 0x0 || out_path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (ldg_io_path_exists_is(src_path) != LDG_TRUTH_TRUE) { return CROSSTALK_HDL_ERR_CC_MISSING; }

    ret = cc_argv_build(ctx, src_path, out_path, argv, (uint32_t)CROSSTALK_HDL_CC_ARG_MAX);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->runs = ctx->runs + (uint64_t)CROSSTALK_HDL_CC_STEP;

    {
        char line[LDG_IO_PATH_MAX * CROSSTALK_HDL_CC_LINE_MUL] = LDG_ARR_ZERO_INIT;
        STARTUPINFOA si = LDG_STRUCT_ZERO_INIT;
        PROCESS_INFORMATION pi = LDG_STRUCT_ZERO_INIT;
        DWORD status = 0;
        uint32_t at = 0;
        uint64_t off = 0;

        si.cb = (DWORD)sizeof(si);

        while (argv[at] != 0x0)
        {
            int32_t wrote = snprintf(line + off, sizeof(line) - off, at == 0 ? "\"%.1023s\"" : " \"%.1023s\"", argv[at]);

            if (wrote < 0 || (uint64_t)wrote >= sizeof(line) - off) { return CROSSTALK_HDL_ERR_CC_ARG; }

            off = off + (uint64_t)wrote;
            at = at + (uint32_t)CROSSTALK_HDL_CC_STEP;
        }

        if (CreateProcessA(0x0, line, 0x0, 0x0, FALSE, CREATE_NO_WINDOW, 0x0, 0x0, &si, &pi) == 0)
        {
            ctx->fails = ctx->fails + (uint64_t)CROSSTALK_HDL_CC_STEP;

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cc: spawn failed; exe: %.255s; last: %u32", ctx->exe, (uint32_t)GetLastError());

            return CROSSTALK_HDL_ERR_CC_SPAWN;
        }

        WaitForSingleObject(pi.hProcess, INFINITE);

        if (GetExitCodeProcess(pi.hProcess, &status) == 0) { status = (DWORD)CROSSTALK_HDL_ERR_CC_EXIT; }

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        code = (uint32_t)status;
    }

    ctx->last_exit = code;

    if (code != (uint32_t)CROSSTALK_HDL_CC_EXIT_OK)
    {
        ctx->fails = ctx->fails + (uint64_t)CROSSTALK_HDL_CC_STEP;

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cc: compile failed; src: %.255s; exit: %u32", src_path, code);

        return CROSSTALK_HDL_ERR_CC_EXIT;
    }

    if (ldg_io_path_exists_is(out_path) != LDG_TRUTH_TRUE)
    {
        ctx->fails = ctx->fails + (uint64_t)CROSSTALK_HDL_CC_STEP;

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cc: compiler exited clean but wrote nothing; out: %.255s", out_path);

        return CROSSTALK_HDL_ERR_CC_MISSING;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "cc: compiled; src: %.255s; out: %.255s; runs: %u64", src_path, out_path, ctx->runs);

    return LDG_ERR_AOK;
}

uint8_t crosstalk_hdl_cc_ready_is(const crosstalk_hdl_cc_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->ok == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}
