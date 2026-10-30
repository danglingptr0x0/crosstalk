#include <inttypes.h>
#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "net/wshop/wshop.h"

uint32_t crosstalk_net_wshop_init(crosstalk_net_wshop_ctx_t *ctx)
{
    const ldg_net_steamworks_wshop_type_desc_t type_desc[CROSSTALK_NET_WSHOP_TYPE_CUNT] = {
        { CROSSTALK_NET_WSHOP_TAG_BOARD, CROSSTALK_NET_WSHOP_MARKER_BOARD },
        { CROSSTALK_NET_WSHOP_TAG_CORE, CROSSTALK_NET_WSHOP_MARKER_CORE },
        { CROSSTALK_NET_WSHOP_TAG_SOFTWARE, CROSSTALK_NET_WSHOP_MARKER_SOFTWARE }
    };
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->init_ok = LDG_TRUTH_FALSE;

    ret = ldg_net_steamworks_wshop_init(&ctx->steam, type_desc, CROSSTALK_NET_WSHOP_TYPE_CUNT);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_DEBUG : LDG_LOGGING_LVL_WARN, "wshop: init; types: %u32; err: %u32", (uint32_t)CROSSTALK_NET_WSHOP_TYPE_CUNT, ret);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->init_ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

void crosstalk_net_wshop_shutdown(crosstalk_net_wshop_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (ctx->init_ok == LDG_TRUTH_FALSE) { return; }

    ldg_net_steamworks_wshop_shutdown(&ctx->steam);
    ctx->init_ok = LDG_TRUTH_FALSE;
}

uint32_t crosstalk_net_wshop_refresh(crosstalk_net_wshop_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->init_ok == LDG_TRUTH_FALSE) { return LDG_ERR_NOT_INIT; }

    ret = ldg_net_steamworks_wshop_refresh(&ctx->steam);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "wshop: refresh; err: %u32; raw: %u32", ret, ctx->steam.item_cunt);

    return ret;
}

void crosstalk_net_wshop_poll(crosstalk_net_wshop_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (ctx->init_ok == LDG_TRUTH_FALSE) { return; }

    ldg_net_steamworks_wshop_poll(&ctx->steam);
}

uint32_t crosstalk_net_wshop_item_cunt_get(const crosstalk_net_wshop_ctx_t *ctx, uint32_t type)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return CROSSTALK_NET_WSHOP_CUNT_NONE; }

    if (ctx->init_ok == LDG_TRUTH_FALSE) { return CROSSTALK_NET_WSHOP_CUNT_NONE; }

    if (type >= CROSSTALK_NET_WSHOP_TYPE_CUNT) { return CROSSTALK_NET_WSHOP_CUNT_NONE; }

    return ldg_net_steamworks_wshop_item_cunt_get(&ctx->steam, type);
}

const ldg_net_steamworks_wshop_item_t* crosstalk_net_wshop_item_get(const crosstalk_net_wshop_ctx_t *ctx, uint32_t type, uint32_t idx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return 0x0; }

    if (ctx->init_ok == LDG_TRUTH_FALSE) { return 0x0; }

    if (type >= CROSSTALK_NET_WSHOP_TYPE_CUNT) { return 0x0; }

    return ldg_net_steamworks_wshop_item_get(&ctx->steam, type, idx);
}

uint8_t crosstalk_net_wshop_item_rdy_is(const ldg_net_steamworks_wshop_item_t *item)
{
    if (LDG_UNLIKELY(item == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (item->installed != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    if (item->path[0] == LDG_STR_TERM) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

uint32_t crosstalk_net_wshop_upload_start(crosstalk_net_wshop_ctx_t *ctx, ldg_net_steamworks_wshop_upload_ctx_t *up, uint32_t type, const char *title, const char *desc, const char *content_path, const char *preview_path)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || up == 0x0 || title == 0x0 || content_path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->init_ok == LDG_TRUTH_FALSE) { return LDG_ERR_NOT_INIT; }

    if (type >= CROSSTALK_NET_WSHOP_TYPE_CUNT) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ldg_net_steamworks_wshop_upload_state_get(up) == LDG_NET_STEAMWORKS_WSHOP_UPLOAD_CREATING) { return LDG_ERR_BUSY; }

    if (ldg_net_steamworks_wshop_upload_state_get(up) == LDG_NET_STEAMWORKS_WSHOP_UPLOAD_UPDATING) { return LDG_ERR_BUSY; }

    ret = ldg_net_steamworks_wshop_upload_init(up);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_net_steamworks_wshop_upload_start(&ctx->steam, up, type, title, desc, content_path, preview_path);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "wshop: upload start; type: %u32; title: %.63s; content: %.191s; err: %u32", type, title, content_path, ret);

    return ret;
}

uint32_t crosstalk_net_wshop_upload_state_get(const ldg_net_steamworks_wshop_upload_ctx_t *up, uint32_t *out_state)
{
    if (LDG_UNLIKELY(up == 0x0 || out_state == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_state = (uint32_t)ldg_net_steamworks_wshop_upload_state_get(up);

    if (*out_state == LDG_NET_STEAMWORKS_WSHOP_UPLOAD_ERR) { return LDG_ERR_IO_WR; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_wshop_item_pull(const crosstalk_net_wshop_ctx_t *ctx, const ldg_net_steamworks_wshop_item_t *item)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || item == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->init_ok == LDG_TRUTH_FALSE) { return LDG_ERR_NOT_INIT; }

    if (crosstalk_net_wshop_item_rdy_is(item) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = ldg_net_steamworks_wshop_item_download(item->file_id);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "wshop: pull; id: %u64; state: %u32; err: %u32", item->file_id, item->state, ret);

    if (ret == LDG_ERR_AOK) { return LDG_ERR_AGAIN; }

    return ret;
}
