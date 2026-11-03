#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/logging/logging.h>

#include <libresteam/inventory/inventory.h>

#include "core/log.h"
#include "net/inventory/inventory.h"

#define INVENTORY_STEP 1U

static void inventory_res_drop(crosstalk_net_inventory_ctx_t *ctx, uint32_t *res)
{
    if (LDG_UNLIKELY(ctx == 0x0 || res == 0x0)) { return; }

    if (*res == LIBRESTEAM_INVENTORY_RES_INVALID) { return; }

    if (libresteam_inventory_res_destroy(ctx->set, *res) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "inventory: result destroy failed; res: %u32", *res); }

    *res = LIBRESTEAM_INVENTORY_RES_INVALID;
}

static uint32_t inventory_res_done_get(const crosstalk_net_inventory_ctx_t *ctx, uint32_t res, uint8_t *out_done, uint8_t *out_ok)
{
    uint32_t eres = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_done == 0x0 || out_ok == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_done = LDG_TRUTH_FALSE;
    *out_ok = LDG_TRUTH_FALSE;

    if (res == LIBRESTEAM_INVENTORY_RES_INVALID) { return LDG_ERR_AOK; }

    ret = libresteam_inventory_res_status_get(ctx->set, res, &eres);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (eres == CROSSTALK_NET_INVENTORY_ERES_PENDING) { return LDG_ERR_AOK; }

    *out_done = LDG_TRUTH_TRUE;
    *out_ok = eres == CROSSTALK_NET_INVENTORY_ERES_OK ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (*out_ok != LDG_TRUTH_TRUE) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "inventory: result failed; res: %u32; eres: %u32", res, eres); }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_inventory_init(crosstalk_net_inventory_ctx_t *ctx, const libresteam_backend_set_t *set)
{
    if (LDG_UNLIKELY(ctx == 0x0 || set == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->set = set;
    ctx->item_cunt = 0;
    ctx->res_grant = LIBRESTEAM_INVENTORY_RES_INVALID;
    ctx->res_all = LIBRESTEAM_INVENTORY_RES_INVALID;
    ctx->init_ok = LDG_TRUTH_TRUE;
    ctx->granted = LDG_TRUTH_FALSE;
    ctx->fetched = LDG_TRUTH_FALSE;

    return libresteam_inventory_item_defs_ld(set);
}

void crosstalk_net_inventory_shutdown(crosstalk_net_inventory_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (ctx->init_ok == LDG_TRUTH_FALSE) { return; }

    inventory_res_drop(ctx, &ctx->res_grant);
    inventory_res_drop(ctx, &ctx->res_all);

    ctx->init_ok = LDG_TRUTH_FALSE;
}

uint32_t crosstalk_net_inventory_refresh(crosstalk_net_inventory_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->init_ok == LDG_TRUTH_FALSE) { return LDG_ERR_NOT_INIT; }

    if (ctx->granted == LDG_TRUTH_FALSE && ctx->res_grant == LIBRESTEAM_INVENTORY_RES_INVALID)
    {
        ret = libresteam_inventory_promo_items_grant(ctx->set, &ctx->res_grant);
        if (ret != LDG_ERR_AOK)
        {
            ctx->res_grant = LIBRESTEAM_INVENTORY_RES_INVALID;
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "inventory: promo grant failed; err: %u32", ret);
        }
    }

    if (ctx->res_all != LIBRESTEAM_INVENTORY_RES_INVALID) { return LDG_ERR_BUSY; }

    ret = libresteam_inventory_all_items_get(ctx->set, &ctx->res_all);
    if (ret != LDG_ERR_AOK) { ctx->res_all = LIBRESTEAM_INVENTORY_RES_INVALID; }

    return ret;
}

void crosstalk_net_inventory_poll(crosstalk_net_inventory_ctx_t *ctx)
{
    uint32_t cunt = CROSSTALK_NET_INVENTORY_ITEM_CAP;
    uint8_t done = LDG_TRUTH_FALSE;
    uint8_t ok = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (ctx->init_ok == LDG_TRUTH_FALSE) { return; }

    if (inventory_res_done_get(ctx, ctx->res_grant, &done, &ok) == LDG_ERR_AOK && done == LDG_TRUTH_TRUE)
    {
        if (ok == LDG_TRUTH_TRUE) { ctx->granted = LDG_TRUTH_TRUE; }

        inventory_res_drop(ctx, &ctx->res_grant);
    }

    if (inventory_res_done_get(ctx, ctx->res_all, &done, &ok) != LDG_ERR_AOK || done != LDG_TRUTH_TRUE) { return; }

    if (ok == LDG_TRUTH_TRUE)
    {
        ret = libresteam_inventory_res_items_get(ctx->set, ctx->res_all, ctx->item, &cunt);
        if (ret == LDG_ERR_AOK)
        {
            ctx->item_cunt = cunt > CROSSTALK_NET_INVENTORY_ITEM_CAP ? CROSSTALK_NET_INVENTORY_ITEM_CAP : cunt;
            ctx->fetched = LDG_TRUTH_TRUE;
        }
        else { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "inventory: items read failed; err: %u32", ret); }
    }

    inventory_res_drop(ctx, &ctx->res_all);
}

uint8_t crosstalk_net_inventory_owned_is(const crosstalk_net_inventory_ctx_t *ctx, int32_t def)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->init_ok == LDG_TRUTH_FALSE) { return LDG_ERR_NOT_INIT; }

    if (ctx->fetched != LDG_TRUTH_TRUE) { return LDG_ERR_AGAIN; }

    for (idx = 0; idx < ctx->item_cunt; idx = idx + INVENTORY_STEP)
    {
        if (ctx->item[idx].def != (uint32_t)def) { continue; }

        if ((ctx->item[idx].flags & LIBRESTEAM_INVENTORY_ITEM_FLAG_REMOVED) != 0) { continue; }

        if ((ctx->item[idx].flags & LIBRESTEAM_INVENTORY_ITEM_FLAG_CONSUMED) != 0) { continue; }

        return LDG_TRUTH_TRUE;
    }

    return LDG_TRUTH_FALSE;
}
