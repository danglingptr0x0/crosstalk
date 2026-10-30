#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <dangling/logging/logging.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/serial.h>

#include "core/log.h"
#include "emu/net.h"

#define NET_SPAN_MAX 65536

static uint32_t net_to_host_mov(crosstalk_emu_net_ctx_t *ctx, const crosstalk_hdl_abi_t *abi)
{
    uint64_t total = 0;
    uint64_t got = 0;
    uint64_t sent = 0;
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

        ret = ldg_io_serial_wr(&ctx->port, &ctx->to_host[ctx->to_host_off], ctx->to_host_len - ctx->to_host_off, &sent);
        if (ret == LDG_ERR_WOULD_BLOCK || ret == LDG_ERR_AGAIN) { return LDG_ERR_AOK; }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ctx->to_host_off = ctx->to_host_off + sent;
        ctx->sent_total = ctx->sent_total + sent;
        total = total + sent;

        if (ctx->to_host_off != ctx->to_host_len) { return LDG_ERR_AOK; }
    }

    return LDG_ERR_AOK;
}

static uint32_t net_to_guest_mov(crosstalk_emu_net_ctx_t *ctx, const crosstalk_hdl_abi_t *abi)
{
    uint64_t total = 0;
    uint64_t got = 0;
    uint64_t taken = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || abi == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (total < (uint64_t)NET_SPAN_MAX)
    {
        if (ctx->to_guest_off == ctx->to_guest_len)
        {
            got = 0;

            ret = ldg_io_serial_rd(&ctx->port, ctx->to_guest, (uint64_t)sizeof(ctx->to_guest), &got);
            if (ret == LDG_ERR_WOULD_BLOCK || ret == LDG_ERR_AGAIN) { return LDG_ERR_AOK; }

            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (LDG_UNLIKELY(got > (uint64_t)sizeof(ctx->to_guest))) { return LDG_ERR_BOUNDS; }

            if (got == 0) { return LDG_ERR_AOK; }

            ctx->to_guest_len = got;
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
    ldg_io_serial_conf_t conf = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->port.handle = (uint64_t)LDG_IO_SERIAL_HANDLE_NONE;

    if (path == 0x0) { return LDG_ERR_AOK; }

    ret = ldg_io_serial_conf_default_get(&conf);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_io_serial_open(path, &conf, &ctx->port);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu net: endpoint open failed; path: %.127s; err: %u32", path, ret);

        return ret;
    }

    ctx->open = LDG_TRUTH_TRUE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu net: endpoint open; path: %.127s", path);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_net_shutdown(crosstalk_emu_net_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = ldg_io_serial_close(&ctx->port);

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
