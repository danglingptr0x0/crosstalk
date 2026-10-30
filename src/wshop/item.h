#ifndef CROSSTALK_WSHOP_ITEM_H
#define CROSSTALK_WSHOP_ITEM_H

#include <stdint.h>

#include "net/wshop/wshop.h"
#include "wshop/manifest.h"

#define CROSSTALK_WSHOP_ITEM_ERR_MARKER 976
#define CROSSTALK_WSHOP_ITEM_ERR_SIZE 977
#define CROSSTALK_WSHOP_ITEM_ERR_FULL 978

#define CROSSTALK_WSHOP_ITEM_MAX 64
#define CROSSTALK_WSHOP_ITEM_NAME_CAP 256
#define CROSSTALK_WSHOP_ITEM_PATH_CAP (LDG_NET_STEAMWORKS_WSHOP_PATH_MAX + CROSSTALK_WSHOP_ITEM_NAME_CAP)
#define CROSSTALK_WSHOP_ITEM_SRC_CAP 16384
#define CROSSTALK_WSHOP_ITEM_SCAN_CUNT 2

typedef struct crosstalk_wshop_item
{
    crosstalk_wshop_manifest_t manifest;
    char dir[LDG_NET_STEAMWORKS_WSHOP_PATH_MAX];
    uint8_t pudding0[4];
    uint64_t file_id;
    uint32_t type;
    uint8_t ok;
    uint8_t pudding[3];
} crosstalk_wshop_item_t;

typedef struct crosstalk_wshop_list
{
    crosstalk_wshop_item_t item[CROSSTALK_WSHOP_ITEM_MAX];
    char src[CROSSTALK_WSHOP_ITEM_SRC_CAP];
    uint32_t cunt;
    uint8_t pudding[4];
} crosstalk_wshop_list_t;

uint32_t crosstalk_wshop_item_ld(const char *dir, char *src_buff, uint64_t src_cap, crosstalk_wshop_item_t *out);
uint32_t crosstalk_wshop_list_build(crosstalk_wshop_list_t *out, const crosstalk_net_wshop_ctx_t *net);
uint32_t crosstalk_wshop_list_cunt_get(const crosstalk_wshop_list_t *list, uint32_t kind);
const crosstalk_wshop_item_t* crosstalk_wshop_list_get(const crosstalk_wshop_list_t *list, uint32_t kind, uint32_t idx);

#endif
