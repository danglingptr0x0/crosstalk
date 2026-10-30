#ifndef CROSSTALK_HDL_LEVEL_H
#define CROSSTALK_HDL_LEVEL_H

#include <stdint.h>

#include "hdl/ir.h"

#define CROSSTALK_HDL_ERR_LEVEL 739
#define CROSSTALK_HDL_LEVEL_FAST_CAP 33

typedef struct crosstalk_hdl_level_unit
{
    uint32_t start;
    uint32_t cunt;
    uint32_t iter_is;
    uint8_t pudding[4];
} crosstalk_hdl_level_unit_t;

typedef struct crosstalk_hdl_level_res
{
    uint32_t *order;
    uint32_t *depth;
    crosstalk_hdl_level_unit_t *units;
    uint32_t unit_cunt;
    uint32_t iter_op_cunt;
    uint32_t level_max;
    uint8_t pudding[4];
} crosstalk_hdl_level_res_t;

uint32_t crosstalk_hdl_level_run(const crosstalk_hdl_ir_t *ir_ctx, void *pool, crosstalk_hdl_level_res_t *res);
uint8_t crosstalk_hdl_level_fast_is(const crosstalk_hdl_ir_t *ir_ctx, uint32_t clk_sig, uint32_t *alias_sigs, uint32_t alias_cap, uint32_t *out_alias_cunt);

#endif
