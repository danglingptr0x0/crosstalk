#ifndef CROSSTALK_HDL_EXEC_H
#define CROSSTALK_HDL_EXEC_H

#include <stdint.h>

#include "hdl/ir.h"
#include "hdl/level.h"
#include "hdl/lex.h"

#define CROSSTALK_HDL_EXEC_SETTLE_MAX 128
#define CROSSTALK_HDL_EXEC_FAST_ALIAS_CAP 32

typedef struct crosstalk_hdl_exec_stage
{
    uint64_t val;
    uint64_t val_hi;
    uint64_t addr;
    uint32_t sig;
    uint32_t mem_is;
} crosstalk_hdl_exec_stage_t;

typedef struct crosstalk_hdl_exec_nrw
{
    uint64_t op_mask;
    uint64_t dst_mask;
    uint64_t imm;
    uint32_t kind;
    uint32_t dst;
    uint32_t src0;
    uint32_t src1;
    uint32_t src2;
    uint32_t depth;
    uint32_t moff;
    uint32_t width;
} crosstalk_hdl_exec_nrw_t;

typedef struct crosstalk_hdl_exec_ctx
{
    const crosstalk_hdl_ir_t *ir_ctx;
    const char *src;
    uint64_t *vals;
    uint64_t *vals_hi;
    uint64_t *mem_arena;
    uint64_t *mem_arena_hi;
    uint32_t *mem_off;
    uint64_t *prev_clk;
    uint64_t *prev_rst;
    crosstalk_hdl_exec_stage_t *stage;
    crosstalk_hdl_exec_nrw_t *nrw;
    const crosstalk_hdl_level_res_t *level_res;
    void *gate;
    void *systask;
    uint64_t tick_cunt;
    uint64_t cycle_cunt;
    uint64_t act_op_cunt;
    uint64_t eval_cunt;
    uint32_t stage_cunt;
    uint32_t stage_cap;
    uint32_t mem_words;
    uint32_t busy_sig;
    uint32_t busy_op;
    uint32_t fast_clk;
    uint32_t fast_alias[CROSSTALK_HDL_EXEC_FAST_ALIAS_CAP];
    uint32_t fast_alias_cunt;
    uint8_t fast_ok;
    uint8_t poke_dirty;
    uint8_t gate_on;
    uint8_t pudding[5];
} crosstalk_hdl_exec_ctx_t;

uint32_t crosstalk_hdl_exec_init(crosstalk_hdl_exec_ctx_t *ctx, const crosstalk_hdl_ir_t *ir_ctx, const char *src, void *pool);
uint32_t crosstalk_hdl_exec_level_set(crosstalk_hdl_exec_ctx_t *ctx, const crosstalk_hdl_level_res_t *level_res);
uint32_t crosstalk_hdl_exec_fast_set(crosstalk_hdl_exec_ctx_t *ctx, uint32_t clk_sig, const uint32_t *alias_sigs, uint32_t alias_cunt);
uint32_t crosstalk_hdl_exec_sig_find(const crosstalk_hdl_exec_ctx_t *ctx, const char *name, uint32_t *out_idx);
uint32_t crosstalk_hdl_exec_sig_sub_find(const crosstalk_hdl_exec_ctx_t *ctx, const char *needle, uint32_t from_idx, uint32_t *out_idx);
uint32_t crosstalk_hdl_exec_sig_name_get(const crosstalk_hdl_exec_ctx_t *ctx, uint32_t sig_idx, char *out, uint32_t cap);
uint32_t crosstalk_hdl_exec_poke(crosstalk_hdl_exec_ctx_t *ctx, uint32_t sig_idx, uint64_t val);
uint32_t crosstalk_hdl_exec_peek(const crosstalk_hdl_exec_ctx_t *ctx, uint32_t sig_idx, uint64_t *out_val);
uint32_t crosstalk_hdl_exec_mem_st(crosstalk_hdl_exec_ctx_t *ctx, uint32_t sig_idx, uint64_t addr, uint64_t val);
uint32_t crosstalk_hdl_exec_mem_ld(const crosstalk_hdl_exec_ctx_t *ctx, uint32_t sig_idx, uint64_t addr, uint64_t *out_val);
uint32_t crosstalk_hdl_exec_eval(crosstalk_hdl_exec_ctx_t *ctx);
uint32_t crosstalk_hdl_exec_step(crosstalk_hdl_exec_ctx_t *ctx);
uint32_t crosstalk_hdl_exec_cycle(crosstalk_hdl_exec_ctx_t *ctx, uint32_t clk_sig);

#endif
