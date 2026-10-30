#ifndef CROSSTALK_HDL_LUTMAP_H
#define CROSSTALK_HDL_LUTMAP_H

#include <stdint.h>

#include "hdl/aig.h"
#include "hdl/netarr.h"

#define CROSSTALK_HDL_ERR_LUTMAP_CAP 739
#define CROSSTALK_HDL_ERR_LUTMAP_STARVE 740
#define CROSSTALK_HDL_ERR_LUTMAP_CONE 741
#define CROSSTALK_HDL_ERR_LUTMAP_FILL 742

#define CROSSTALK_HDL_LUTMAP_K 4
#define CROSSTALK_HDL_LUTMAP_K_MAX 6
#define CROSSTALK_HDL_LUTMAP_CUT_MAX 8
#define CROSSTALK_HDL_LUTMAP_K4_LEVEL_PS 350
#define CROSSTALK_HDL_LUTMAP_K4_FIXED_PS 1000
#define CROSSTALK_HDL_LUTMAP_K6_LEVEL_PS 400
#define CROSSTALK_HDL_LUTMAP_K6_FIXED_PS 800
#define CROSSTALK_HDL_LUTMAP_PS_PER_KHZ 1000000000

typedef struct crosstalk_hdl_lutmap_res
{
    uint32_t lut_cunt;
    uint32_t depth;
    uint32_t ff_bits;
    uint32_t le_cunt;
    uint32_t node_cunt;
    uint32_t pi_cunt;
    uint32_t period_ps;
    uint32_t fmax_khz;
} crosstalk_hdl_lutmap_res_t;

uint32_t crosstalk_hdl_lutmap_run(const crosstalk_hdl_aig_ctx_t *aig_ctx, void *pool, uint32_t lut_k, crosstalk_hdl_lutmap_res_t *out_res);
uint32_t crosstalk_hdl_lutmap_netarr_run(const crosstalk_hdl_aig_ctx_t *aig_ctx, void *pool, uint32_t lut_k, crosstalk_hdl_netarr_t *out_netarr, crosstalk_hdl_lutmap_res_t *out_res);

#endif
