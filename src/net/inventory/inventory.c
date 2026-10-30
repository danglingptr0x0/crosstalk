#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#include "net/inventory/inventory.h"

uint32_t crosstalk_net_inventory_init(crosstalk_net_inventory_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->init_ok = LDG_TRUTH_FALSE;
    ctx->granted = LDG_TRUTH_FALSE;
    ctx->fetched = LDG_TRUTH_FALSE;

    ret = ldg_net_steamworks_inventory_init(&ctx->steam);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->init_ok = LDG_TRUTH_TRUE;

    return ldg_net_steamworks_inventory_def_ld();
}

void crosstalk_net_inventory_shutdown(crosstalk_net_inventory_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (ctx->init_ok == LDG_TRUTH_FALSE) { return; }

    ldg_net_steamworks_inventory_shutdown(&ctx->steam);
    ctx->init_ok = LDG_TRUTH_FALSE;
}

uint32_t crosstalk_net_inventory_refresh(crosstalk_net_inventory_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->init_ok == LDG_TRUTH_FALSE) { return LDG_ERR_NOT_INIT; }

    if (ctx->granted == LDG_TRUTH_FALSE)
    {
        ret = ldg_net_steamworks_inventory_promo_grant(&ctx->steam);
        if (ret == LDG_ERR_AOK) { ctx->granted = LDG_TRUTH_TRUE; }
    }

    ret = ldg_net_steamworks_inventory_all_get(&ctx->steam);
    if (ret == LDG_ERR_AOK) { ctx->fetched = LDG_TRUTH_TRUE; }

    return ret;
}

void crosstalk_net_inventory_poll(crosstalk_net_inventory_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (ctx->init_ok == LDG_TRUTH_FALSE) { return; }

    ldg_net_steamworks_inventory_poll(&ctx->steam);
}

uint8_t crosstalk_net_inventory_owned_is(const crosstalk_net_inventory_ctx_t *ctx, int32_t def)
{
    const ldg_net_steamworks_inventory_item_t *item = 0x0;
    uint32_t cunt = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->init_ok == LDG_TRUTH_FALSE) { return LDG_ERR_NOT_INIT; }

    if (ctx->steam.status != LDG_NET_STEAMWORKS_INVENTORY_STATUS_OK) { return LDG_ERR_AGAIN; }

    cunt = ldg_net_steamworks_inventory_item_cunt_get(&ctx->steam);

    for (idx = 0; idx < cunt; idx = idx + 1)
    {
        item = ldg_net_steamworks_inventory_item_get(&ctx->steam, idx);
        if (item == 0x0) { continue; }

        if (item->def != def) { continue; }

        if ((item->flags & LDG_NET_STEAMWORKS_INVENTORY_FLAG_REMOVED) != 0) { continue; }

        if ((item->flags & LDG_NET_STEAMWORKS_INVENTORY_FLAG_CONSUMED) != 0) { continue; }

        return LDG_TRUTH_TRUE;
    }

    return LDG_TRUTH_FALSE;
}
