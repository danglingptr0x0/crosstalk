#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/logging/logging.h>
#include <dangling/time/time.h>

#include "core/log.h"
#include "net/presence.h"

#define NET_PRESENCE_STEP 1U

uint32_t crosstalk_net_presence_init(crosstalk_net_presence_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (uint32_t idx = 0; idx < CROSSTALK_NET_PRESENCE_BACKEND_CAP; idx++)
    {
        ctx->backend[idx].backend_ctx = 0x0;
        ctx->backend[idx].push = 0x0;
    }

    ctx->next_ms = 0;
    ctx->backend_cunt = 0;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_presence_reg(crosstalk_net_presence_ctx_t *ctx, void *backend_ctx, crosstalk_net_presence_push_cb_t push)
{
    if (LDG_UNLIKELY(ctx == 0x0 || backend_ctx == 0x0 || push == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (uint32_t idx = 0; idx < ctx->backend_cunt; idx++) { if (ctx->backend[idx].backend_ctx == backend_ctx) { return LDG_ERR_EXISTS; } }

    if (ctx->backend_cunt >= CROSSTALK_NET_PRESENCE_BACKEND_CAP) { return LDG_ERR_FULL; }

    ctx->backend[ctx->backend_cunt].backend_ctx = backend_ctx;
    ctx->backend[ctx->backend_cunt].push = push;
    ctx->backend_cunt = ctx->backend_cunt + NET_PRESENCE_STEP;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_presence_unreg(crosstalk_net_presence_ctx_t *ctx, void *backend_ctx)
{
    uint32_t last = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || backend_ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (uint32_t idx = 0; idx < ctx->backend_cunt; idx++)
    {
        if (ctx->backend[idx].backend_ctx != backend_ctx) { continue; }

        last = ctx->backend_cunt - NET_PRESENCE_STEP;
        ctx->backend[idx] = ctx->backend[last];
        ctx->backend[last].backend_ctx = 0x0;
        ctx->backend[last].push = 0x0;
        ctx->backend_cunt = last;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t crosstalk_net_presence_push(crosstalk_net_presence_ctx_t *ctx, const crosstalk_net_presence_desc_t *desc)
{
    uint64_t now_ms = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || desc == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (desc->proj_open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    now_ms = ldg_time_epoch_ms_get();

    if (now_ms < ctx->next_ms) { return LDG_ERR_AOK; }

    ctx->next_ms = now_ms + (uint64_t)CROSSTALK_NET_PRESENCE_PERIOD_MS;

    for (uint32_t idx = 0; idx < ctx->backend_cunt; idx++)
    {
        ret = ctx->backend[idx].push(ctx->backend[idx].backend_ctx, desc);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "presence: push failed; backend: %u32; err: %u32", idx, ret); }
    }

    return LDG_ERR_AOK;
}
