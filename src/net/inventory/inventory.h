#ifndef CROSSTALK_NET_INVENTORY_H
#define CROSSTALK_NET_INVENTORY_H

#include <stdint.h>

#include <dangling/net/steamworks_inventory.h>

#define CROSSTALK_NET_INVENTORY_DEF_COMP_LIB 100

typedef struct crosstalk_net_inventory_ctx
{
    ldg_net_steamworks_inventory_ctx_t steam;
    uint8_t init_ok;
    uint8_t granted;
    uint8_t fetched;
    uint8_t pudding[5];
} crosstalk_net_inventory_ctx_t;

uint32_t crosstalk_net_inventory_init(crosstalk_net_inventory_ctx_t *ctx);
void crosstalk_net_inventory_shutdown(crosstalk_net_inventory_ctx_t *ctx);
uint32_t crosstalk_net_inventory_refresh(crosstalk_net_inventory_ctx_t *ctx);
void crosstalk_net_inventory_poll(crosstalk_net_inventory_ctx_t *ctx);
uint8_t crosstalk_net_inventory_owned_is(const crosstalk_net_inventory_ctx_t *ctx, int32_t def);

#endif
