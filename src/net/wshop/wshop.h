#ifndef CROSSTALK_NET_WSHOP_H
#define CROSSTALK_NET_WSHOP_H

#include <stdint.h>

#include <dangling/net/steamworks_wshop.h>

#define CROSSTALK_NET_WSHOP_TYPE_BOARD 0U
#define CROSSTALK_NET_WSHOP_TYPE_CORE 1U
#define CROSSTALK_NET_WSHOP_TYPE_SOFTWARE 2U
#define CROSSTALK_NET_WSHOP_TYPE_CUNT 3U
#define CROSSTALK_NET_WSHOP_CUNT_NONE 0U

#define CROSSTALK_NET_WSHOP_TAG_BOARD "Board"
#define CROSSTALK_NET_WSHOP_MARKER_BOARD ".xtlk"
#define CROSSTALK_NET_WSHOP_TAG_CORE "Core"
#define CROSSTALK_NET_WSHOP_MARKER_CORE ".xtcore"
#define CROSSTALK_NET_WSHOP_TAG_SOFTWARE "Software"
#define CROSSTALK_NET_WSHOP_MARKER_SOFTWARE ".xtimg"

typedef struct crosstalk_net_wshop_ctx
{
    ldg_net_steamworks_wshop_ctx_t steam;
    uint8_t init_ok;
    uint8_t pudding[63];
} crosstalk_net_wshop_ctx_t;

uint32_t crosstalk_net_wshop_init(crosstalk_net_wshop_ctx_t *ctx);
void crosstalk_net_wshop_shutdown(crosstalk_net_wshop_ctx_t *ctx);
uint32_t crosstalk_net_wshop_refresh(crosstalk_net_wshop_ctx_t *ctx);
void crosstalk_net_wshop_poll(crosstalk_net_wshop_ctx_t *ctx);
uint32_t crosstalk_net_wshop_item_cunt_get(const crosstalk_net_wshop_ctx_t *ctx, uint32_t type);
const ldg_net_steamworks_wshop_item_t* crosstalk_net_wshop_item_get(const crosstalk_net_wshop_ctx_t *ctx, uint32_t type, uint32_t idx);
uint32_t crosstalk_net_wshop_item_pull(const crosstalk_net_wshop_ctx_t *ctx, const ldg_net_steamworks_wshop_item_t *item);
uint8_t crosstalk_net_wshop_item_rdy_is(const ldg_net_steamworks_wshop_item_t *item);
uint32_t crosstalk_net_wshop_upload_start(crosstalk_net_wshop_ctx_t *ctx, ldg_net_steamworks_wshop_upload_ctx_t *up, uint32_t type, const char *title, const char *desc, const char *content_path, const char *preview_path);
uint32_t crosstalk_net_wshop_upload_state_get(const ldg_net_steamworks_wshop_upload_ctx_t *up, uint32_t *out_state);

#endif
