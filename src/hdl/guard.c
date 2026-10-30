#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <ucontext.h>
#endif

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "hdl/guard.h"

static crosstalk_hdl_guard_ctx_t *guard_live = 0x0;

static uint8_t guard_range_hit_is(const crosstalk_hdl_guard_ctx_t *ctx, uint64_t at, uint32_t *out)
{
    uint32_t idx = 0;

    if (ctx == 0x0 || out == 0x0) { return LDG_TRUTH_FALSE; }

    *out = (uint32_t)UINT32_MAX;

    for (idx = 0; idx < CROSSTALK_HDL_GUARD_RANGE_MAX; idx = idx + (uint32_t)CROSSTALK_HDL_GUARD_STEP)
    {
        if (ctx->range[idx].live != LDG_TRUTH_TRUE) { continue; }

        if (at < ctx->range[idx].base) { continue; }

        if (at >= ctx->range[idx].base + ctx->range[idx].span) { continue; }

        *out = ctx->range[idx].slot;

        return LDG_TRUTH_TRUE;
    }

    return LDG_TRUTH_FALSE;
}

#ifdef _WIN32
static void guard_land(void)
{
    longjmp(guard_live->bail, (int32_t)CROSSTALK_HDL_GUARD_BAIL);
}

static LONG CALLBACK guard_veh(PEXCEPTION_POINTERS info)
{
    uint64_t at = 0;
    uint32_t slot = 0;

    if (guard_live == 0x0 || guard_live->armed != LDG_TRUTH_TRUE) { return EXCEPTION_CONTINUE_SEARCH; }

    if (info == 0x0 || info->ContextRecord == 0x0 || info->ExceptionRecord == 0x0) { return EXCEPTION_CONTINUE_SEARCH; }

    at = (uint64_t)info->ContextRecord->Rip;

    if (guard_range_hit_is(guard_live, at, &slot) != LDG_TRUTH_TRUE) { return EXCEPTION_CONTINUE_SEARCH; }

    guard_live->fault_addr = at;
    guard_live->fault_slot = slot;
    guard_live->fault_cunt = guard_live->fault_cunt + (uint32_t)CROSSTALK_HDL_GUARD_STEP;
    guard_live->armed = LDG_TRUTH_FALSE;

    info->ContextRecord->Rip = (DWORD64)(uintptr_t)guard_land;

    return EXCEPTION_CONTINUE_EXECUTION;
}

#else
static void guard_sig(int32_t sig, siginfo_t *info, void *uc)
{
    ucontext_t *state = (ucontext_t *)uc;
    uint64_t at = 0;
    uint32_t slot = 0;

    if (guard_live == 0x0 || guard_live->armed != LDG_TRUTH_TRUE || state == 0x0 || info == 0x0 || sig == 0) { return; }

    at = (uint64_t)state->uc_mcontext.gregs[CROSSTALK_HDL_GUARD_REG_RIP];

    if (guard_range_hit_is(guard_live, at, &slot) != LDG_TRUTH_TRUE) { return; }

    guard_live->fault_addr = at;
    guard_live->fault_slot = slot;
    guard_live->fault_cunt = guard_live->fault_cunt + (uint32_t)CROSSTALK_HDL_GUARD_STEP;
    guard_live->armed = LDG_TRUTH_FALSE;

    siglongjmp(guard_live->bail, (int32_t)CROSSTALK_HDL_GUARD_BAIL);
}

#endif

uint32_t crosstalk_hdl_guard_init(crosstalk_hdl_guard_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (guard_live != 0x0) { return CROSSTALK_HDL_ERR_GUARD_LIVE; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->fault_slot = (uint32_t)UINT32_MAX;

#ifdef _WIN32
    ctx->handle = AddVectoredExceptionHandler((ULONG)CROSSTALK_HDL_GUARD_VEH_FIRST, guard_veh);
    if (ctx->handle == 0x0) { return CROSSTALK_HDL_ERR_GUARD_INSTALL; }

#else
    {
        struct sigaction act = LDG_STRUCT_ZERO_INIT;

        act.sa_sigaction = guard_sig;
        act.sa_flags = SA_SIGINFO | SA_NODEFER;
        sigemptyset(&act.sa_mask);

        if (sigaction(SIGSEGV, &act, &ctx->prev_segv) != 0) { return CROSSTALK_HDL_ERR_GUARD_INSTALL; }

        if (sigaction(SIGILL, &act, &ctx->prev_ill) != 0) { return CROSSTALK_HDL_ERR_GUARD_INSTALL; }

        if (sigaction(SIGFPE, &act, &ctx->prev_fpe) != 0) { return CROSSTALK_HDL_ERR_GUARD_INSTALL; }

        if (sigaction(SIGBUS, &act, &ctx->prev_bus) != 0) { return CROSSTALK_HDL_ERR_GUARD_INSTALL; }
    }
#endif

    ctx->ok = LDG_TRUTH_TRUE;
    guard_live = ctx;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "guard: installed");

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_guard_shutdown(crosstalk_hdl_guard_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

#ifdef _WIN32
    if (ctx->handle != 0x0 && RemoveVectoredExceptionHandler(ctx->handle) == 0) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "guard remove failed"); }

#else
    if (sigaction(SIGSEGV, &ctx->prev_segv, 0x0) != 0) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "guard segv restore failed"); }

    if (sigaction(SIGILL, &ctx->prev_ill, 0x0) != 0) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "guard ill restore failed"); }

    if (sigaction(SIGFPE, &ctx->prev_fpe, 0x0) != 0) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "guard fpe restore failed"); }

    if (sigaction(SIGBUS, &ctx->prev_bus, 0x0) != 0) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "guard bus restore failed"); }

#endif

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "guard: removed; faults: %u32", ctx->fault_cunt);

    memset(ctx, 0x0, sizeof(*ctx));
    guard_live = 0x0;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_guard_range_add(crosstalk_hdl_guard_ctx_t *ctx, uint64_t base, uint64_t span, uint32_t slot)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (base == 0 || span == 0) { return CROSSTALK_HDL_ERR_GUARD_RANGE; }

    for (idx = 0; idx < CROSSTALK_HDL_GUARD_RANGE_MAX; idx = idx + (uint32_t)CROSSTALK_HDL_GUARD_STEP)
    {
        if (ctx->range[idx].live == LDG_TRUTH_TRUE) { continue; }

        ctx->range[idx].base = base;
        ctx->range[idx].span = span;
        ctx->range[idx].slot = slot;
        ctx->range[idx].live = LDG_TRUTH_TRUE;

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "guard: range added; slot: %u32; base: 0x%X64; span: %u64", slot, base, span);

        return LDG_ERR_AOK;
    }

    return CROSSTALK_HDL_ERR_GUARD_FULL;
}

uint32_t crosstalk_hdl_guard_range_drop(crosstalk_hdl_guard_ctx_t *ctx, uint32_t slot)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < CROSSTALK_HDL_GUARD_RANGE_MAX; idx = idx + (uint32_t)CROSSTALK_HDL_GUARD_STEP)
    {
        if (ctx->range[idx].live != LDG_TRUTH_TRUE || ctx->range[idx].slot != slot) { continue; }

        ctx->range[idx] = (crosstalk_hdl_guard_range_t)LDG_STRUCT_ZERO_INIT;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_guard_fault_get(const crosstalk_hdl_guard_ctx_t *ctx, uint32_t *out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = ctx->fault_slot;

    return LDG_ERR_AOK;
}
