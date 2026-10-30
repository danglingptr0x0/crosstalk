#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <windows.h>

#include <dangling/logging/logging.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#include "core/log.h"
#include "emu/net.h"

#define NET_SPAN_MAX 65536
#define NET_PIPE_NONE ((uint64_t)(uintptr_t)INVALID_HANDLE_VALUE)
#define NET_SHARE_NONE 0
#define NET_ATTR_NONE 0
#define NET_PEEK_NO_COPY 0

static HANDLE net_pipe_get(const crosstalk_emu_net_ctx_t *ctx)
{
    return (HANDLE)(uintptr_t)ctx->pipe;
}

static uint32_t net_to_host_mov(crosstalk_emu_net_ctx_t *ctx, const crosstalk_hdl_abi_t *abi)
{
    uint64_t total = 0;
    uint64_t got = 0;
    DWORD sent = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || abi == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (total < (uint64_t)NET_SPAN_MAX)
    {
        if (ctx->to_host_off == ctx->to_host_len)
        {
            ret = abi->net_drain(ctx->to_host, (uint64_t)sizeof(ctx->to_host), &got);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (LDG_UNLIKELY(got > (uint64_t)sizeof(ctx->to_host))) { return LDG_ERR_BOUNDS; }

            if (got == 0) { return LDG_ERR_AOK; }

            ctx->to_host_len = got;
            ctx->to_host_off = 0;
        }

        sent = 0;

        if (WriteFile(net_pipe_get(ctx), &ctx->to_host[ctx->to_host_off], (DWORD)(ctx->to_host_len - ctx->to_host_off), &sent, 0x0) == 0) { return LDG_ERR_CLOSED; }

        ctx->to_host_off = ctx->to_host_off + (uint64_t)sent;
        ctx->sent_total = ctx->sent_total + (uint64_t)sent;
        total = total + (uint64_t)sent;

        if (ctx->to_host_off != ctx->to_host_len) { return LDG_ERR_AOK; }
    }

    return LDG_ERR_AOK;
}

static uint32_t net_to_guest_mov(crosstalk_emu_net_ctx_t *ctx, const crosstalk_hdl_abi_t *abi)
{
    uint64_t total = 0;
    uint64_t taken = 0;
    DWORD avail = 0;
    DWORD got = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || abi == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (total < (uint64_t)NET_SPAN_MAX)
    {
        if (ctx->to_guest_off == ctx->to_guest_len)
        {
            avail = 0;

            if (PeekNamedPipe(net_pipe_get(ctx), 0x0, (DWORD)NET_PEEK_NO_COPY, 0x0, &avail, 0x0) == 0) { return LDG_ERR_CLOSED; }

            if (avail == 0) { return LDG_ERR_AOK; }

            if (avail > (DWORD)sizeof(ctx->to_guest)) { avail = (DWORD)sizeof(ctx->to_guest); }

            got = 0;

            if (ReadFile(net_pipe_get(ctx), ctx->to_guest, avail, &got, 0x0) == 0) { return LDG_ERR_CLOSED; }

            if (got == 0) { return LDG_ERR_AOK; }

            ctx->to_guest_len = (uint64_t)got;
            ctx->to_guest_off = 0;
        }

        taken = 0;

        ret = abi->net_feed(&ctx->to_guest[ctx->to_guest_off], ctx->to_guest_len - ctx->to_guest_off, &taken);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(taken > ctx->to_guest_len - ctx->to_guest_off)) { return LDG_ERR_BOUNDS; }

        ctx->to_guest_off = ctx->to_guest_off + taken;
        ctx->recv_total = ctx->recv_total + taken;
        total = total + taken;

        if (ctx->to_guest_off != ctx->to_guest_len) { return LDG_ERR_AOK; }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_net_init(crosstalk_emu_net_ctx_t *ctx, const char *path)
{
    HANDLE pipe = INVALID_HANDLE_VALUE;
    DWORD mode = PIPE_READMODE_BYTE;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->pipe = NET_PIPE_NONE;

    if (path == 0x0) { return LDG_ERR_AOK; }

    pipe = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, (DWORD)NET_SHARE_NONE, 0x0, OPEN_EXISTING, (DWORD)NET_ATTR_NONE, 0x0);
    if (pipe == INVALID_HANDLE_VALUE)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu net: endpoint open failed; path: %.127s; last: %u32", path, (uint32_t)GetLastError());

        return LDG_ERR_IO_OPEN;
    }

    if (SetNamedPipeHandleState(pipe, &mode, 0x0, 0x0) == 0)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu net: endpoint not a pipe; path: %.127s; last: %u32", path, (uint32_t)GetLastError());

        if (CloseHandle(pipe) == 0) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu net: endpoint close failed"); }

        return LDG_ERR_IO_OPEN;
    }

    ctx->pipe = (uint64_t)(uintptr_t)pipe;
    ctx->open = LDG_TRUTH_TRUE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu net: endpoint open; path: %.127s", path);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_net_shutdown(crosstalk_emu_net_ctx_t *ctx)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (CloseHandle(net_pipe_get(ctx)) == 0) { ret = LDG_ERR_IO_CLOSE; }

    ctx->pipe = NET_PIPE_NONE;
    ctx->open = LDG_TRUTH_FALSE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu net: endpoint closed; sent: %u64; recv: %u64; err: %u32", ctx->sent_total, ctx->recv_total, ret);

    return ret;
}

uint32_t crosstalk_emu_net_tick(crosstalk_emu_net_ctx_t *ctx, const crosstalk_hdl_abi_t *abi)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || abi == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (abi->net_drain != 0x0)
    {
        ret = net_to_host_mov(ctx, abi);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (abi->net_feed != 0x0)
    {
        ret = net_to_guest_mov(ctx, abi);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}
