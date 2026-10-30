#ifndef CROSSTALK_HDL_NETARR_H
#define CROSSTALK_HDL_NETARR_H

#include <stdint.h>

#include "hdl/aig.h"

#define CROSSTALK_HDL_NETARR_IN_MAX 6
#define CROSSTALK_HDL_NETARR_MASK_BITS 64

typedef struct crosstalk_hdl_netarr_lut
{
    uint64_t mask;
    uint32_t in_node[CROSSTALK_HDL_NETARR_IN_MAX];
    uint32_t out_node;
    uint32_t in_cunt;
} crosstalk_hdl_netarr_lut_t;

typedef struct crosstalk_hdl_netarr
{
    const crosstalk_hdl_aig_ctx_t *aig_ctx;
    crosstalk_hdl_netarr_lut_t *luts;
    uint32_t lut_cunt;
    uint32_t lut_k;
    uint32_t node_cunt;
    uint8_t pudding[4];
} crosstalk_hdl_netarr_t;

uint32_t crosstalk_hdl_netarr_v_wr(const crosstalk_hdl_netarr_t *netarr, const char *src, const char *top, const char *clk_name, const char *path);

#endif
