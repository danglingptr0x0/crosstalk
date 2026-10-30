#include <stdint.h>

#include <dangling/core/arith.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/mem.h>

#include "hdl/ir.h"

static uint32_t crosstalk_hdl_ir_arr_alloc(void *pool, uint32_t cap, uint64_t item_size, void **out)
{
    uint64_t bytes = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(pool == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_arith_uint64_mul((uint64_t)cap, item_size, &bytes);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_OVERFLOW; }

    return ldg_mem_pool_alloc((ldg_mem_pool_t *)pool, bytes, out);
}

uint32_t crosstalk_hdl_ir_init(crosstalk_hdl_ir_t *ir_ctx, void *pool, uint32_t sig_cap, uint32_t comb_cap, uint32_t seq_op_cap, uint32_t seq_blk_cap)
{
    void *raw = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ir_ctx == 0x0 || pool == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ir_ctx->sigs = 0x0;
    ir_ctx->comb_ops = 0x0;
    ir_ctx->seq_ops = 0x0;
    ir_ctx->seq_blks = 0x0;
    ir_ctx->insts = 0x0;
    ir_ctx->jacks = 0x0;
    ir_ctx->mem_inits = 0x0;
    ir_ctx->mem_init_cunt = 0;
    ir_ctx->mem_init_cap = 0;
    ir_ctx->systasks = 0x0;
    ir_ctx->systask_args = 0x0;
    ir_ctx->systask_cunt = 0;
    ir_ctx->systask_arg_cunt = 0;
    ir_ctx->final_cunt = 0;
    ir_ctx->qual_text = 0x0;
    ir_ctx->qual_text_len = 0;
    ir_ctx->qual_text_cap = 0;
    ir_ctx->inst_cunt = 0;
    ir_ctx->jack_cunt = 0;
    ir_ctx->sig_cunt = 0;
    ir_ctx->sig_cap = 0;
    ir_ctx->comb_cunt = 0;
    ir_ctx->comb_cap = 0;
    ir_ctx->seq_op_cunt = 0;
    ir_ctx->seq_op_cap = 0;
    ir_ctx->seq_blk_cunt = 0;
    ir_ctx->seq_blk_cap = 0;
    ir_ctx->tmp_base = 0;

    ret = crosstalk_hdl_ir_arr_alloc(pool, sig_cap, (uint64_t)sizeof(crosstalk_hdl_ir_sig_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ir_ctx->sigs = (crosstalk_hdl_ir_sig_t *)raw;
    ir_ctx->sig_cap = sig_cap;

    ret = crosstalk_hdl_ir_arr_alloc(pool, comb_cap, (uint64_t)sizeof(crosstalk_hdl_ir_op_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ir_ctx->comb_ops = (crosstalk_hdl_ir_op_t *)raw;
    ir_ctx->comb_cap = comb_cap;

    ret = crosstalk_hdl_ir_arr_alloc(pool, seq_op_cap, (uint64_t)sizeof(crosstalk_hdl_ir_op_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ir_ctx->seq_ops = (crosstalk_hdl_ir_op_t *)raw;
    ir_ctx->seq_op_cap = seq_op_cap;

    ret = crosstalk_hdl_ir_arr_alloc(pool, seq_blk_cap, (uint64_t)sizeof(crosstalk_hdl_ir_seq_blk_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ir_ctx->seq_blks = (crosstalk_hdl_ir_seq_blk_t *)raw;
    ir_ctx->seq_blk_cap = seq_blk_cap;

    ret = crosstalk_hdl_ir_arr_alloc(pool, CROSSTALK_HDL_IR_INST_MAX, (uint64_t)sizeof(crosstalk_hdl_ir_inst_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ir_ctx->insts = (crosstalk_hdl_ir_inst_t *)raw;

    ret = crosstalk_hdl_ir_arr_alloc(pool, CROSSTALK_HDL_IR_JACK_MAX, (uint64_t)sizeof(crosstalk_hdl_ir_jack_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ir_ctx->jacks = (crosstalk_hdl_ir_jack_t *)raw;

    ret = crosstalk_hdl_ir_arr_alloc(pool, CROSSTALK_HDL_IR_MEM_INIT_MAX, (uint64_t)sizeof(crosstalk_hdl_ir_mem_init_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ir_ctx->mem_inits = (crosstalk_hdl_ir_mem_init_t *)raw;
    ir_ctx->mem_init_cap = CROSSTALK_HDL_IR_MEM_INIT_MAX;

    ret = crosstalk_hdl_ir_arr_alloc(pool, CROSSTALK_HDL_IR_SYSTASK_MAX, (uint64_t)sizeof(crosstalk_hdl_ir_systask_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ir_ctx->systasks = (crosstalk_hdl_ir_systask_t *)raw;

    ret = crosstalk_hdl_ir_arr_alloc(pool, CROSSTALK_HDL_IR_SYSTASK_ARG_MAX, (uint64_t)sizeof(crosstalk_hdl_ir_systask_arg_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ir_ctx->systask_args = (crosstalk_hdl_ir_systask_arg_t *)raw;

    ret = crosstalk_hdl_ir_arr_alloc(pool, sig_cap, (uint64_t)CROSSTALK_HDL_IR_QUAL_AVG, &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ir_ctx->qual_text = (char *)raw;
    ir_ctx->qual_text_cap = sig_cap * (uint32_t)CROSSTALK_HDL_IR_QUAL_AVG;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_ir_inst_add(crosstalk_hdl_ir_t *ir_ctx, const crosstalk_hdl_tok_t *name, const crosstalk_hdl_tok_t *module_name, uint32_t *out_idx)
{
    crosstalk_hdl_ir_inst_t *inst = 0x0;

    if (LDG_UNLIKELY(ir_ctx == 0x0 || name == 0x0 || module_name == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    if (LDG_UNLIKELY(ir_ctx->insts == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ir_ctx->inst_cunt >= CROSSTALK_HDL_IR_INST_MAX)) { return LDG_ERR_FULL; }

    inst = &ir_ctx->insts[ir_ctx->inst_cunt];
    inst->name = *name;
    inst->module_name = *module_name;

    *out_idx = ir_ctx->inst_cunt;
    ir_ctx->inst_cunt++;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_ir_jack_add(crosstalk_hdl_ir_t *ir_ctx, const crosstalk_hdl_tok_t *name, uint32_t inst, uint32_t sig, uint32_t width, uint32_t dir)
{
    crosstalk_hdl_ir_jack_t *jack = 0x0;

    if (LDG_UNLIKELY(ir_ctx == 0x0 || name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ir_ctx->jacks == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(inst >= ir_ctx->inst_cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(ir_ctx->jack_cunt >= CROSSTALK_HDL_IR_JACK_MAX)) { return LDG_ERR_FULL; }

    jack = &ir_ctx->jacks[ir_ctx->jack_cunt];
    jack->name = *name;
    jack->inst = inst;
    jack->sig = sig;
    jack->width = width;
    jack->dir = dir;
    ir_ctx->jack_cunt++;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_ir_sig_add(crosstalk_hdl_ir_t *ir_ctx, const crosstalk_hdl_tok_t *name, uint32_t width, uint32_t depth, uint32_t kind, uint32_t port, uint64_t init_val, uint32_t *out_idx)
{
    crosstalk_hdl_tok_t blank = LDG_STRUCT_ZERO_INIT;
    crosstalk_hdl_ir_sig_t *sig = 0x0;

    if (LDG_UNLIKELY(ir_ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    if (LDG_UNLIKELY(ir_ctx->sigs == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(width == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ir_ctx->sig_cunt >= ir_ctx->sig_cap)) { return LDG_ERR_FULL; }

    sig = &ir_ctx->sigs[ir_ctx->sig_cunt];
    sig->name = name != 0x0 ? *name : blank;
    sig->mem_file = blank;
    sig->width = width;
    sig->depth = depth;
    sig->kind = kind;
    sig->port = port;
    sig->init_val = init_val;
    sig->qual_off = UINT32_MAX;
    sig->qual_len = 0;

    *out_idx = ir_ctx->sig_cunt;
    ir_ctx->sig_cunt++;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_ir_comb_op_add(crosstalk_hdl_ir_t *ir_ctx, const crosstalk_hdl_ir_op_t *op)
{
    if (LDG_UNLIKELY(ir_ctx == 0x0 || op == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ir_ctx->comb_ops == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ir_ctx->comb_cunt >= ir_ctx->comb_cap)) { return LDG_ERR_FULL; }

    ir_ctx->comb_ops[ir_ctx->comb_cunt] = *op;
    ir_ctx->comb_cunt++;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_ir_seq_blk_open(crosstalk_hdl_ir_t *ir_ctx, uint32_t clk_sig, uint32_t clk_edge, uint32_t rst_sig, uint32_t rst_edge, uint32_t *out_idx)
{
    crosstalk_hdl_ir_seq_blk_t *blk = 0x0;

    if (LDG_UNLIKELY(ir_ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    if (LDG_UNLIKELY(ir_ctx->seq_blks == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ir_ctx->seq_blk_cunt >= ir_ctx->seq_blk_cap)) { return LDG_ERR_FULL; }

    blk = &ir_ctx->seq_blks[ir_ctx->seq_blk_cunt];
    blk->clk_sig = clk_sig;
    blk->clk_edge = clk_edge;
    blk->rst_sig = rst_sig;
    blk->rst_edge = rst_edge;
    blk->op_start = ir_ctx->seq_op_cunt;
    blk->op_cunt = 0;

    *out_idx = ir_ctx->seq_blk_cunt;
    ir_ctx->seq_blk_cunt++;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_ir_seq_op_add(crosstalk_hdl_ir_t *ir_ctx, uint32_t blk_idx, const crosstalk_hdl_ir_op_t *op)
{
    if (LDG_UNLIKELY(ir_ctx == 0x0 || op == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ir_ctx->seq_ops == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(blk_idx >= ir_ctx->seq_blk_cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(blk_idx + 0x1 != ir_ctx->seq_blk_cunt)) { return LDG_ERR_INVALID; }

    if (LDG_UNLIKELY(ir_ctx->seq_op_cunt >= ir_ctx->seq_op_cap)) { return LDG_ERR_FULL; }

    ir_ctx->seq_ops[ir_ctx->seq_op_cunt] = *op;
    ir_ctx->seq_op_cunt++;
    ir_ctx->seq_blks[blk_idx].op_cunt++;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_ir_systask_arg_add(crosstalk_hdl_ir_t *ir_ctx, const crosstalk_hdl_ir_systask_arg_t *arg)
{
    if (LDG_UNLIKELY(ir_ctx == 0x0 || arg == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ir_ctx->systask_args == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ir_ctx->systask_arg_cunt >= (uint32_t)CROSSTALK_HDL_IR_SYSTASK_ARG_MAX)) { return LDG_ERR_FULL; }

    ir_ctx->systask_args[ir_ctx->systask_arg_cunt] = *arg;
    ir_ctx->systask_arg_cunt++;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_ir_systask_add(crosstalk_hdl_ir_t *ir_ctx, const crosstalk_hdl_ir_systask_t *task)
{
    if (LDG_UNLIKELY(ir_ctx == 0x0 || task == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ir_ctx->systasks == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ir_ctx->systask_cunt >= (uint32_t)CROSSTALK_HDL_IR_SYSTASK_MAX)) { return LDG_ERR_FULL; }

    if (LDG_UNLIKELY(task->arg_cunt > ir_ctx->systask_arg_cunt || task->arg_start > ir_ctx->systask_arg_cunt - task->arg_cunt)) { return LDG_ERR_BOUNDS; }

    ir_ctx->systasks[ir_ctx->systask_cunt] = *task;
    ir_ctx->systask_cunt++;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_ir_sig_mem_file_set(crosstalk_hdl_ir_t *ir_ctx, uint32_t sig_idx, const crosstalk_hdl_tok_t *file_tok)
{
    if (LDG_UNLIKELY(ir_ctx == 0x0 || file_tok == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ir_ctx->sigs == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(sig_idx >= ir_ctx->sig_cunt)) { return LDG_ERR_BOUNDS; }

    ir_ctx->sigs[sig_idx].mem_file = *file_tok;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_ir_mem_init_add(crosstalk_hdl_ir_t *ir_ctx, uint32_t sig_idx, uint64_t addr, uint64_t val)
{
    crosstalk_hdl_ir_mem_init_t *init = 0x0;

    if (LDG_UNLIKELY(ir_ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ir_ctx->sigs == 0x0 || ir_ctx->mem_inits == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(sig_idx >= ir_ctx->sig_cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(addr >= (uint64_t)ir_ctx->sigs[sig_idx].depth)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(ir_ctx->mem_init_cunt >= ir_ctx->mem_init_cap)) { return LDG_ERR_FULL; }

    init = &ir_ctx->mem_inits[ir_ctx->mem_init_cunt];
    init->addr = addr;
    init->val = val;
    init->sig = sig_idx;
    init->pudding[0] = 0;
    init->pudding[1] = 0;
    init->pudding[2] = 0;
    init->pudding[3] = 0;
    ir_ctx->mem_init_cunt++;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_ir_sig_qual_set(crosstalk_hdl_ir_t *ir_ctx, uint32_t sig_idx, const char *text, uint32_t len)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ir_ctx == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ir_ctx->sigs == 0x0 || ir_ctx->qual_text == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(sig_idx >= ir_ctx->sig_cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ir_ctx->qual_text_len + len > ir_ctx->qual_text_cap) { return LDG_ERR_FULL; }

    for (idx = 0; idx < len; idx = idx + 0x1) { ir_ctx->qual_text[ir_ctx->qual_text_len + idx] = text[idx]; }

    ir_ctx->sigs[sig_idx].qual_off = ir_ctx->qual_text_len;
    ir_ctx->sigs[sig_idx].qual_len = len;
    ir_ctx->qual_text_len = ir_ctx->qual_text_len + len;

    return LDG_ERR_AOK;
}
