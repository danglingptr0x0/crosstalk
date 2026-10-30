#ifndef CROSSTALK_HDL_AIG_H
#define CROSSTALK_HDL_AIG_H

#include <stdint.h>

#include "hdl/ir.h"

#define CROSSTALK_HDL_ERR_AIG_CAP 737
#define CROSSTALK_HDL_ERR_AIG_CYCLE 738

#define CROSSTALK_HDL_AIG_LIT_FALSE 0x0
#define CROSSTALK_HDL_AIG_LIT_TRUE 0x1
#define CROSSTALK_HDL_AIG_WIDTH_MAX 64
#define CROSSTALK_HDL_AIG_NODE_CAP_DEFAULT 0x400000
#define CROSSTALK_HDL_AIG_ROOT_CAP_DEFAULT 0x40000

#define CROSSTALK_HDL_AIG_PI_KIND_IN 0
#define CROSSTALK_HDL_AIG_PI_KIND_FF 1
#define CROSSTALK_HDL_AIG_PI_KIND_MEM_RD 2
#define CROSSTALK_HDL_AIG_PI_KIND_LATCH 3

typedef struct crosstalk_hdl_aig_node
{
    uint32_t fanin0;
    uint32_t fanin1;
} crosstalk_hdl_aig_node_t;

typedef struct crosstalk_hdl_aig_ctx
{
    const crosstalk_hdl_ir_t *ir_ctx;
    crosstalk_hdl_aig_node_t *nodes;
    uint32_t *hash_tbl;
    uint32_t *lit_arena;
    uint32_t *lit_off;
    uint32_t *ff_arena;
    uint32_t *ff_off;
    uint32_t *roots;
    uint8_t *op_placed;
    uint32_t *op_order;
    uint32_t *last_wr;
    uint8_t *latch_is;
    uint32_t *rd_dep_sig;
    uint32_t *rd_dep_op;
    uint32_t *op_out;
    uint32_t *pi_node;
    uint32_t *pi_sig;
    uint32_t *pi_bit;
    uint32_t *pi_op;
    uint8_t *pi_kind;
    uint32_t *op_root_off;
    uint32_t *op_pi_base;
    uint32_t pi_cap;
    uint32_t latch_root_base;
    uint32_t ff_root_base;
    uint32_t node_cunt;
    uint32_t node_cap;
    uint32_t hash_cap;
    uint32_t root_cunt;
    uint32_t root_cap;
    uint32_t pi_cunt;
    uint32_t ff_bits;
    uint32_t residue_cunt;
    uint32_t io_bits;
    uint64_t mem_bits;
    uint32_t mem_rd_cunt;
    uint32_t mem_wr_cunt;
    uint32_t latch_bits;
    uint32_t cur_op;
    uint8_t comb_mode;
    uint8_t pudding[3];
} crosstalk_hdl_aig_ctx_t;

#define CROSSTALK_HDL_AIG_RD_SLOT_CUNT 3

uint32_t crosstalk_hdl_aig_build(crosstalk_hdl_aig_ctx_t *ctx, const crosstalk_hdl_ir_t *ir_ctx, void *pool, uint32_t node_cap);

#endif
