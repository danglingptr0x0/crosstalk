#ifndef CROSSTALK_NET_INVENTORY_H
#define CROSSTALK_NET_INVENTORY_H

#include <stdint.h>

#include <libresteam/backend/backend.h>
#include <libresteam/inventory/types.h>

#define CROSSTALK_NET_INVENTORY_DEF_COMP_LIB 100
#define CROSSTALK_NET_INVENTORY_ITEM_CAP 256U
#define CROSSTALK_NET_INVENTORY_ERES_OK 1U
#define CROSSTALK_NET_INVENTORY_ERES_PENDING 22U

typedef struct crosstalk_net_inventory_ctx
{
    libresteam_inventory_item_t item[CROSSTALK_NET_INVENTORY_ITEM_CAP];
    const libresteam_backend_set_t *set;
    uint32_t item_cunt;
    uint32_t res_grant;
    uint32_t res_all;
    uint8_t init_ok;
    uint8_t granted;
    uint8_t fetched;
    uint8_t pudding[1];
} crosstalk_net_inventory_ctx_t;

uint32_t crosstalk_net_inventory_init(crosstalk_net_inventory_ctx_t *ctx, const libresteam_backend_set_t *set);
void crosstalk_net_inventory_shutdown(crosstalk_net_inventory_ctx_t *ctx);
uint32_t crosstalk_net_inventory_refresh(crosstalk_net_inventory_ctx_t *ctx);
void crosstalk_net_inventory_poll(crosstalk_net_inventory_ctx_t *ctx);
uint8_t crosstalk_net_inventory_owned_is(const crosstalk_net_inventory_ctx_t *ctx, int32_t def);

#endif
