#ifndef CROSSTALK_HDL_LOGIC_GATE_H
#define CROSSTALK_HDL_LOGIC_GATE_H

#include <stdint.h>

#include "hdl/aig.h"
#include "hdl/exec.h"
#include "hdl/netarr.h"

#define CROSSTALK_HDL_ERR_LOGIC_GATE_WIDTH 745
#define CROSSTALK_HDL_ERR_LOGIC_GATE_SETTLE 746
#define CROSSTALK_HDL_ERR_LOGIC_GATE_BIND 747

#define CROSSTALK_HDL_LOGIC_GATE_SETTLE_MAX 128

typedef struct crosstalk_hdl_logic_gate_mem_op
{
    uint64_t val_mask;
    uint32_t mem_sig;
    uint32_t en_off;
    uint32_t addr_off;
    uint32_t addr_w;
    uint32_t val_off;
    uint32_t val_w;
    uint32_t pi_base;
    uint32_t dst_w;
    uint32_t wr_is;
    uint8_t pudding[4];
} crosstalk_hdl_logic_gate_mem_op_t;

typedef struct crosstalk_hdl_logic_gate_ctx
{
    const crosstalk_hdl_aig_ctx_t *aig_ctx;
    const crosstalk_hdl_netarr_t *netarr;
    crosstalk_hdl_logic_gate_mem_op_t *mem_ops;
    uint8_t *node_val;
    uint32_t *latch_pi_node;
    uint32_t *latch_root_lit;
    uint32_t *ff_sig;
    uint32_t *ff_root_off;
    uint64_t cycle_cunt;
    uint64_t pass_cunt;
    uint32_t latch_cunt;
    uint32_t ff_cunt;
    uint32_t mem_op_cunt;
    uint8_t ok;
    uint8_t pudding[3];
} crosstalk_hdl_logic_gate_ctx_t;

uint32_t crosstalk_hdl_logic_gate_init(crosstalk_hdl_logic_gate_ctx_t *ctx, const crosstalk_hdl_aig_ctx_t *aig_ctx, const crosstalk_hdl_netarr_t *netarr, void *pool);
uint32_t crosstalk_hdl_logic_gate_cycle_run(crosstalk_hdl_logic_gate_ctx_t *ctx, crosstalk_hdl_exec_ctx_t *exec, uint32_t clk_sig);

#endif
