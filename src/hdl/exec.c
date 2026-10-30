#include <stdint.h>
#include <string.h>

#include <dangling/core/arith.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/mem.h>

#include "hdl/exec.h"
#include "hdl/logic_gate.h"
#include "hdl/systask.h"

static uint64_t crosstalk_hdl_exec_mask_get(uint32_t width)
{
    if (width >= 64) { return ~(uint64_t)0x0; }

    if (width == 0) { return 0x0; }

    return ((uint64_t)0x1 << width) - 0x1;
}

static uint64_t crosstalk_hdl_exec_mask_hi_get(uint32_t width)
{
    if (width <= 64) { return 0x0; }

    return crosstalk_hdl_exec_mask_get(width - 64);
}

static int64_t crosstalk_hdl_exec_sext_get(uint64_t val, uint32_t width)
{
    uint64_t sign_bit = 0;

    if (width == 0 || width >= 64) { return (int64_t)val; }

    sign_bit = (uint64_t)0x1 << (width - 0x1);

    if ((val & sign_bit) != 0) { return (int64_t)(val | ~crosstalk_hdl_exec_mask_get(width)); }

    return (int64_t)val;
}

static uint32_t crosstalk_hdl_exec_arr_alloc(void *pool, uint64_t cunt, uint64_t item_size, void **out)
{
    uint64_t bytes = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(pool == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (cunt == 0) { cunt = 0x1; }

    ret = ldg_arith_uint64_mul(cunt, item_size, &bytes);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_OVERFLOW; }

    return ldg_mem_pool_alloc((ldg_mem_pool_t *)pool, bytes, out);
}

static uint32_t crosstalk_hdl_exec_nrw_build(crosstalk_hdl_exec_ctx_t *ctx, void *pool)
{
    const crosstalk_hdl_ir_t *ir_ctx = 0x0;
    const crosstalk_hdl_ir_op_t *op = 0x0;
    crosstalk_hdl_exec_nrw_t *nop = 0x0;
    void *raw = 0x0;
    uint64_t total = 0;
    uint32_t zero_slot = 0;
    uint32_t one_slot = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || pool == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ir_ctx = ctx->ir_ctx;
    zero_slot = ir_ctx->sig_cunt;
    one_slot = ir_ctx->sig_cunt + 0x1;
    total = (uint64_t)ir_ctx->comb_cunt + (uint64_t)ir_ctx->seq_op_cunt;
    ret = crosstalk_hdl_exec_arr_alloc(pool, total, (uint64_t)sizeof(crosstalk_hdl_exec_nrw_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->nrw = (crosstalk_hdl_exec_nrw_t *)raw;

    for (idx = 0; idx < (uint32_t)total; idx++)
    {
        op = idx < ir_ctx->comb_cunt ? &ir_ctx->comb_ops[idx] : &ir_ctx->seq_ops[idx - ir_ctx->comb_cunt];
        nop = &ctx->nrw[idx];
        nop->op_mask = crosstalk_hdl_exec_mask_get(op->width);
        nop->dst_mask = 0;
        nop->imm = op->imm;
        nop->kind = op->kind;
        nop->dst = op->dst == UINT32_MAX ? zero_slot : op->dst;
        nop->src0 = op->src0 == UINT32_MAX ? zero_slot : op->src0;
        nop->src1 = op->src1 == UINT32_MAX ? zero_slot : op->src1;
        nop->src2 = op->src2 == UINT32_MAX ? zero_slot : op->src2;
        nop->depth = 0;
        nop->moff = 0;
        nop->width = op->width;
        if (op->kind == CROSSTALK_HDL_IR_OP_MEM_RD || op->kind == CROSSTALK_HDL_IR_OP_MEM_WR)
        {
            if (LDG_UNLIKELY(op->src0 == UINT32_MAX)) { return LDG_ERR_INVALID; }

            nop->depth = ir_ctx->sigs[op->src0].depth;
            nop->moff = ctx->mem_off[op->src0];
        }

        if (op->kind == CROSSTALK_HDL_IR_OP_MEM_WR)
        {
            nop->dst = op->dst == UINT32_MAX ? one_slot : op->dst;
            nop->op_mask = crosstalk_hdl_exec_mask_get(ir_ctx->sigs[op->src0].width);
            continue;
        }

        if (LDG_UNLIKELY(op->dst == UINT32_MAX)) { return LDG_ERR_INVALID; }

        nop->dst_mask = crosstalk_hdl_exec_mask_get(ir_ctx->sigs[op->dst].width);
        if (op->kind == CROSSTALK_HDL_IR_OP_CONST) { nop->imm = op->imm & nop->dst_mask; }

        if (op->kind == CROSSTALK_HDL_IR_OP_RED_AND) { nop->imm = crosstalk_hdl_exec_mask_get((uint32_t)op->imm); }

        if (op->kind == CROSSTALK_HDL_IR_OP_CONCAT) { nop->op_mask = crosstalk_hdl_exec_mask_get((uint32_t)op->imm); }

        if (op->kind == CROSSTALK_HDL_IR_OP_LT_S || op->kind == CROSSTALK_HDL_IR_OP_LE_S || op->kind == CROSSTALK_HDL_IR_OP_GT_S || op->kind == CROSSTALK_HDL_IR_OP_GE_S)
        {
            if (LDG_UNLIKELY(op->width == 0 || op->width > 64)) { return LDG_ERR_INVALID; }

            nop->op_mask = (uint64_t)0x1 << (op->width - 0x1);
        }
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_exec_nrw_apply(crosstalk_hdl_exec_ctx_t *ctx, const crosstalk_hdl_exec_nrw_t *nop, uint8_t seq, uint8_t *out_changed)
{
    uint64_t s0 = 0;
    uint64_t s1 = 0;
    uint64_t res = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || nop == 0x0 || out_changed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    s0 = ctx->vals[nop->src0];
    s1 = ctx->vals[nop->src1];
    switch (nop->kind)
    {
        case CROSSTALK_HDL_IR_OP_CONST:
            res = nop->imm;
            break;
        case CROSSTALK_HDL_IR_OP_COPY:
        case CROSSTALK_HDL_IR_OP_ZEXT:
            res = s0;
            break;
        case CROSSTALK_HDL_IR_OP_SEXT:
            res = (uint64_t)crosstalk_hdl_exec_sext_get(s0, (uint32_t)nop->imm);
            break;
        case CROSSTALK_HDL_IR_OP_AND:
            res = s0 & s1;
            break;
        case CROSSTALK_HDL_IR_OP_OR:
            res = s0 | s1;
            break;
        case CROSSTALK_HDL_IR_OP_XOR:
            res = s0 ^ s1;
            break;
        case CROSSTALK_HDL_IR_OP_XNOR:
            res = ~(s0 ^ s1);
            break;
        case CROSSTALK_HDL_IR_OP_NOT:
            res = ~s0;
            break;
        case CROSSTALK_HDL_IR_OP_ADD:
            res = s0 + s1;
            break;
        case CROSSTALK_HDL_IR_OP_SUB:
            res = s0 - s1;
            break;
        case CROSSTALK_HDL_IR_OP_MUL:
            res = s0 * s1;
            break;
        case CROSSTALK_HDL_IR_OP_DIV:
            res = s1 != 0 ? s0 / s1 : 0x0;
            break;
        case CROSSTALK_HDL_IR_OP_MOD:
            res = s1 != 0 ? s0 % s1 : 0x0;
            break;
        case CROSSTALK_HDL_IR_OP_NEG:
            res = ~s0 + 0x1;
            break;
        case CROSSTALK_HDL_IR_OP_SHL:
            res = s1 >= 64 ? 0x0 : s0 << s1;
            break;
        case CROSSTALK_HDL_IR_OP_SHR:
            res = s1 >= 64 ? 0x0 : s0 >> s1;
            break;
        case CROSSTALK_HDL_IR_OP_ASHR:
        {
            int64_t sval = crosstalk_hdl_exec_sext_get(s0, nop->width);

            res = s1 >= 64 ? (sval < 0 ? ~(uint64_t)0x0 : 0x0) : (uint64_t)(sval >> s1);
            break;
        }
        case CROSSTALK_HDL_IR_OP_EQ:
            res = s0 == s1 ? 0x1 : 0x0;
            break;
        case CROSSTALK_HDL_IR_OP_NEQ:
            res = s0 != s1 ? 0x1 : 0x0;
            break;
        case CROSSTALK_HDL_IR_OP_LT:
            res = s0 < s1 ? 0x1 : 0x0;
            break;
        case CROSSTALK_HDL_IR_OP_LE:
            res = s0 <= s1 ? 0x1 : 0x0;
            break;
        case CROSSTALK_HDL_IR_OP_GT:
            res = s0 > s1 ? 0x1 : 0x0;
            break;
        case CROSSTALK_HDL_IR_OP_GE:
            res = s0 >= s1 ? 0x1 : 0x0;
            break;
        case CROSSTALK_HDL_IR_OP_LT_S:
            res = (s0 ^ nop->op_mask) < (s1 ^ nop->op_mask) ? 0x1 : 0x0;
            break;
        case CROSSTALK_HDL_IR_OP_LE_S:
            res = (s0 ^ nop->op_mask) <= (s1 ^ nop->op_mask) ? 0x1 : 0x0;
            break;
        case CROSSTALK_HDL_IR_OP_GT_S:
            res = (s0 ^ nop->op_mask) > (s1 ^ nop->op_mask) ? 0x1 : 0x0;
            break;
        case CROSSTALK_HDL_IR_OP_GE_S:
            res = (s0 ^ nop->op_mask) >= (s1 ^ nop->op_mask) ? 0x1 : 0x0;
            break;
        case CROSSTALK_HDL_IR_OP_LAND:
            res = s0 != 0 && s1 != 0 ? 0x1 : 0x0;
            break;
        case CROSSTALK_HDL_IR_OP_LOR:
            res = s0 != 0 || s1 != 0 ? 0x1 : 0x0;
            break;
        case CROSSTALK_HDL_IR_OP_LNOT:
            res = s0 == 0 ? 0x1 : 0x0;
            break;
        case CROSSTALK_HDL_IR_OP_RED_AND:
            res = s0 == nop->imm ? 0x1 : 0x0;
            break;
        case CROSSTALK_HDL_IR_OP_RED_OR:
            res = s0 != 0 ? 0x1 : 0x0;
            break;
        case CROSSTALK_HDL_IR_OP_RED_XOR:
        {
            uint64_t fold = s0;

            fold = fold ^ (fold >> 32);
            fold = fold ^ (fold >> 16);
            fold = fold ^ (fold >> 8);
            fold = fold ^ (fold >> 4);
            fold = fold ^ (fold >> 2);
            fold = fold ^ (fold >> 0x1);
            res = fold & 0x1;
            break;
        }
        case CROSSTALK_HDL_IR_OP_CONCAT:
            res = (nop->imm >= 64 ? 0x0 : s0 << nop->imm) | (s1 & nop->op_mask);
            break;
        case CROSSTALK_HDL_IR_OP_SLICE:
            res = (nop->imm >= 64 ? 0x0 : s0 >> nop->imm) & nop->op_mask;
            break;
        case CROSSTALK_HDL_IR_OP_SLICE_DYN:
            res = (s1 >= 64 ? 0x0 : s0 >> s1) & nop->op_mask;
            break;
        case CROSSTALK_HDL_IR_OP_INSERT:
        case CROSSTALK_HDL_IR_OP_INSERT_DYN:
        {
            uint64_t amt = nop->kind == CROSSTALK_HDL_IR_OP_INSERT ? nop->imm : ctx->vals[nop->src2];
            uint64_t hole = 0;

            if (amt >= 64)
            {
                res = s0;
                break;
            }

            hole = nop->op_mask << amt;
            res = (s0 & ~hole) | ((s1 & nop->op_mask) << amt);
            break;
        }
        case CROSSTALK_HDL_IR_OP_MUX:
            res = s0 != 0 ? s1 : ctx->vals[nop->src2];
            break;
        case CROSSTALK_HDL_IR_OP_MEM_RD:
            res = s1 < (uint64_t)nop->depth ? ctx->mem_arena[nop->moff + s1] : 0x0;
            break;
        case CROSSTALK_HDL_IR_OP_MEM_WR:
        {
            if (ctx->vals[nop->dst] == 0) { return LDG_ERR_AOK; }

            if (seq == LDG_TRUTH_TRUE)
            {
                if (LDG_UNLIKELY(ctx->stage_cunt >= ctx->stage_cap)) { return LDG_ERR_FULL; }

                ctx->stage[ctx->stage_cunt].mem_is = LDG_TRUTH_TRUE;
                ctx->stage[ctx->stage_cunt].sig = nop->src0;
                ctx->stage[ctx->stage_cunt].addr = s1;
                ctx->stage[ctx->stage_cunt].val = ctx->vals[nop->src2] & nop->op_mask;
                ctx->stage_cunt++;
            }
            else if (s1 < (uint64_t)nop->depth) { ctx->mem_arena[nop->moff + s1] = ctx->vals[nop->src2] & nop->op_mask; }

            return LDG_ERR_AOK;
        }
        case CROSSTALK_HDL_IR_OP_REG_NEXT:
        {
            if (LDG_UNLIKELY(ctx->stage_cunt >= ctx->stage_cap)) { return LDG_ERR_FULL; }

            ctx->stage[ctx->stage_cunt].mem_is = LDG_TRUTH_FALSE;
            ctx->stage[ctx->stage_cunt].sig = nop->dst;
            ctx->stage[ctx->stage_cunt].val = s0 & nop->dst_mask;
            ctx->stage_cunt++;

            return LDG_ERR_AOK;
        }
        default:
            return LDG_ERR_UNSUPPORTED;
    }

    res = res & nop->dst_mask;
    if (ctx->vals[nop->dst] != res)
    {
        ctx->vals[nop->dst] = res;
        *out_changed = LDG_TRUTH_TRUE;
    }

    return LDG_ERR_AOK;
}

static void crosstalk_hdl_exec_stage_commit(crosstalk_hdl_exec_ctx_t *ctx, uint8_t *out_changed)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_changed == 0x0)) { return; }

    *out_changed = LDG_TRUTH_FALSE;

    if (ctx->nrw != 0x0)
    {
        for (idx = 0; idx < ctx->stage_cunt; idx++)
        {
            const crosstalk_hdl_exec_stage_t *st = &ctx->stage[idx];

            if (st->mem_is == LDG_TRUTH_TRUE) { if (st->addr < (uint64_t)ctx->ir_ctx->sigs[st->sig].depth && ctx->mem_arena[ctx->mem_off[st->sig] + st->addr] != st->val)
                {
                    ctx->mem_arena[ctx->mem_off[st->sig] + st->addr] = st->val;
                    *out_changed = LDG_TRUTH_TRUE;
                }
            }
            else if (ctx->vals[st->sig] != st->val)
            {
                ctx->vals[st->sig] = st->val;
                *out_changed = LDG_TRUTH_TRUE;
            }
        }

        ctx->stage_cunt = 0;

        return;
    }

    for (idx = 0; idx < ctx->stage_cunt; idx++)
    {
        uint32_t st_sig = ctx->stage[idx].sig;
        uint32_t st_w = ctx->ir_ctx->sigs[st_sig].width;
        uint64_t st_val = ctx->stage[idx].val & crosstalk_hdl_exec_mask_get(st_w);
        uint64_t st_val_hi = ctx->stage[idx].val_hi & crosstalk_hdl_exec_mask_hi_get(st_w);

        if (ctx->stage[idx].mem_is == LDG_TRUTH_TRUE) { if (ctx->stage[idx].addr < (uint64_t)ctx->ir_ctx->sigs[st_sig].depth) { if (ctx->mem_arena[ctx->mem_off[st_sig] + ctx->stage[idx].addr] != st_val || ctx->mem_arena_hi[ctx->mem_off[st_sig] + ctx->stage[idx].addr] != st_val_hi)
                {
                    ctx->mem_arena[ctx->mem_off[st_sig] + ctx->stage[idx].addr] = st_val;
                    ctx->mem_arena_hi[ctx->mem_off[st_sig] + ctx->stage[idx].addr] = st_val_hi;
                    *out_changed = LDG_TRUTH_TRUE;
                }
            } }
        else if (ctx->vals[st_sig] != st_val || ctx->vals_hi[st_sig] != st_val_hi)
        {
            ctx->vals[st_sig] = st_val;
            ctx->vals_hi[st_sig] = st_val_hi;
            *out_changed = LDG_TRUTH_TRUE;
        }
    }

    ctx->stage_cunt = 0;
}

uint32_t crosstalk_hdl_exec_init(crosstalk_hdl_exec_ctx_t *ctx, const crosstalk_hdl_ir_t *ir_ctx, const char *src, void *pool)
{
    void *raw = 0x0;
    uint64_t words = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || ir_ctx == 0x0 || src == 0x0 || pool == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->ir_ctx = ir_ctx;
    ctx->src = src;
    ctx->level_res = 0x0;
    ctx->systask = 0x0;
    ctx->tick_cunt = 0;
    ctx->cycle_cunt = 0;
    ctx->act_op_cunt = 0;
    ctx->eval_cunt = 0;
    ctx->fast_clk = UINT32_MAX;
    ctx->fast_alias_cunt = 0;
    ctx->fast_ok = LDG_TRUTH_FALSE;
    ctx->poke_dirty = LDG_TRUTH_TRUE;
    ctx->stage_cunt = 0;
    ctx->stage_cap = ir_ctx->seq_op_cunt == 0 ? 0x1 : ir_ctx->seq_op_cunt;
    ctx->mem_words = 0;
    ctx->busy_sig = UINT32_MAX;
    ctx->busy_op = UINT32_MAX;
    memset(ctx->pudding, 0x0, sizeof(ctx->pudding));

    ret = crosstalk_hdl_exec_arr_alloc(pool, (uint64_t)ir_ctx->sig_cunt + 0x2, (uint64_t)sizeof(uint64_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->vals = (uint64_t *)raw;
    ctx->vals[ir_ctx->sig_cunt] = 0;
    ctx->vals[ir_ctx->sig_cunt + 0x1] = 0x1;

    ret = crosstalk_hdl_exec_arr_alloc(pool, (uint64_t)ir_ctx->sig_cunt + 0x2, (uint64_t)sizeof(uint64_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->vals_hi = (uint64_t *)raw;
    ctx->vals_hi[ir_ctx->sig_cunt] = 0;
    ctx->vals_hi[ir_ctx->sig_cunt + 0x1] = 0;

    ret = crosstalk_hdl_exec_arr_alloc(pool, ir_ctx->sig_cunt, (uint64_t)sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->mem_off = (uint32_t *)raw;

    for (idx = 0; idx < ir_ctx->sig_cunt; idx++)
    {
        ctx->vals[idx] = ir_ctx->sigs[idx].init_val & crosstalk_hdl_exec_mask_get(ir_ctx->sigs[idx].width);
        ctx->vals_hi[idx] = 0;
        ctx->mem_off[idx] = UINT32_MAX;

        if (ir_ctx->sigs[idx].depth > 0)
        {
            ctx->mem_off[idx] = (uint32_t)words;
            words = words + (uint64_t)ir_ctx->sigs[idx].depth;
        }
    }

    ctx->mem_words = (uint32_t)words;

    ret = crosstalk_hdl_exec_arr_alloc(pool, words, (uint64_t)sizeof(uint64_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->mem_arena = (uint64_t *)raw;

    ret = crosstalk_hdl_exec_arr_alloc(pool, words, (uint64_t)sizeof(uint64_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->mem_arena_hi = (uint64_t *)raw;

    for (idx = 0; idx < (uint32_t)words; idx++)
    {
        ctx->mem_arena[idx] = 0;
        ctx->mem_arena_hi[idx] = 0;
    }

    for (idx = 0; idx < ir_ctx->mem_init_cunt; idx++)
    {
        const crosstalk_hdl_ir_mem_init_t *init = &ir_ctx->mem_inits[idx];

        if (init->sig >= ir_ctx->sig_cunt || ctx->mem_off[init->sig] == UINT32_MAX || init->addr >= (uint64_t)ir_ctx->sigs[init->sig].depth) { return LDG_ERR_BOUNDS; }

        ctx->mem_arena[ctx->mem_off[init->sig] + init->addr] = init->val & crosstalk_hdl_exec_mask_get(ir_ctx->sigs[init->sig].width);
    }

    ret = crosstalk_hdl_exec_arr_alloc(pool, ir_ctx->seq_blk_cunt, (uint64_t)sizeof(uint64_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->prev_clk = (uint64_t *)raw;

    ret = crosstalk_hdl_exec_arr_alloc(pool, ir_ctx->seq_blk_cunt, (uint64_t)sizeof(uint64_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->prev_rst = (uint64_t *)raw;

    for (idx = 0; idx < ir_ctx->seq_blk_cunt; idx++)
    {
        ctx->prev_clk[idx] = 0;
        ctx->prev_rst[idx] = 0;
    }

    ret = crosstalk_hdl_exec_arr_alloc(pool, ctx->stage_cap, (uint64_t)sizeof(crosstalk_hdl_exec_stage_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->stage = (crosstalk_hdl_exec_stage_t *)raw;
    ctx->nrw = 0x0;

    for (idx = 0; idx < ir_ctx->sig_cunt; idx++) { if (ir_ctx->sigs[idx].width > 64) { return LDG_ERR_AOK; } }

    return crosstalk_hdl_exec_nrw_build(ctx, pool);
}

uint32_t crosstalk_hdl_exec_sig_name_get(const crosstalk_hdl_exec_ctx_t *ctx, uint32_t sig_idx, char *out, uint32_t cap)
{
    uint32_t len = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_BOUNDS; }

    out[0] = LDG_STR_TERM;

    if (sig_idx >= ctx->ir_ctx->sig_cunt) { return LDG_ERR_BOUNDS; }

    len = ctx->ir_ctx->sigs[sig_idx].name.len;
    if (len == 0) { return LDG_ERR_EMPTY; }

    if (len > cap - LDG_STR_TERM_SIZE) { len = cap - LDG_STR_TERM_SIZE; }

    memcpy(out, ctx->src + ctx->ir_ctx->sigs[sig_idx].name.off, len);
    out[len] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_exec_sig_sub_find(const crosstalk_hdl_exec_ctx_t *ctx, const char *needle, uint32_t from_idx, uint32_t *out_idx)
{
    uint64_t needle_len = 0;
    uint32_t idx = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || needle == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;
    needle_len = strlen(needle);
    if (needle_len == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = from_idx; idx < ctx->ir_ctx->sig_cunt; idx++)
    {
        const char *name = ctx->src + ctx->ir_ctx->sigs[idx].name.off;
        uint32_t name_len = ctx->ir_ctx->sigs[idx].name.len;

        if ((uint64_t)name_len < needle_len) { continue; }

        for (at = 0; (uint64_t)at + needle_len <= (uint64_t)name_len; at++)
        {
            if (memcmp(name + at, needle, needle_len) != 0) { continue; }

            *out_idx = idx;

            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t crosstalk_hdl_exec_sig_find(const crosstalk_hdl_exec_ctx_t *ctx, const char *name, uint32_t *out_idx)
{
    uint64_t name_len = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    if (ctx->ir_ctx == 0x0 || ctx->src == 0x0) { return LDG_ERR_NOT_FOUND; }

    name_len = strlen(name);

    for (idx = 0; idx < ctx->ir_ctx->sig_cunt; idx++)
    {
        if ((uint64_t)ctx->ir_ctx->sigs[idx].name.len == name_len && ctx->ir_ctx->sigs[idx].name.len != 0 && memcmp(ctx->src + ctx->ir_ctx->sigs[idx].name.off, name, name_len) == 0)
        {
            *out_idx = idx;

            return LDG_ERR_AOK;
        }

        if (ctx->ir_ctx->qual_text == 0x0) { continue; }

        if ((uint64_t)ctx->ir_ctx->sigs[idx].qual_len != name_len || ctx->ir_ctx->sigs[idx].qual_len == 0) { continue; }

        if (ctx->ir_ctx->sigs[idx].qual_off == UINT32_MAX) { continue; }

        if (memcmp(ctx->ir_ctx->qual_text + ctx->ir_ctx->sigs[idx].qual_off, name, name_len) != 0) { continue; }

        *out_idx = idx;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t crosstalk_hdl_exec_poke(crosstalk_hdl_exec_ctx_t *ctx, uint32_t sig_idx, uint64_t val)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(sig_idx >= ctx->ir_ctx->sig_cunt)) { return LDG_ERR_BOUNDS; }

    ctx->vals[sig_idx] = val & crosstalk_hdl_exec_mask_get(ctx->ir_ctx->sigs[sig_idx].width);
    ctx->vals_hi[sig_idx] = 0;
    ctx->poke_dirty = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_exec_peek(const crosstalk_hdl_exec_ctx_t *ctx, uint32_t sig_idx, uint64_t *out_val)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_val == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_val = 0;

    if (LDG_UNLIKELY(sig_idx >= ctx->ir_ctx->sig_cunt)) { return LDG_ERR_BOUNDS; }

    *out_val = ctx->vals[sig_idx];

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_exec_mem_st(crosstalk_hdl_exec_ctx_t *ctx, uint32_t sig_idx, uint64_t addr, uint64_t val)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(sig_idx >= ctx->ir_ctx->sig_cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(ctx->mem_off[sig_idx] == UINT32_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(addr >= (uint64_t)ctx->ir_ctx->sigs[sig_idx].depth)) { return LDG_ERR_BOUNDS; }

    ctx->mem_arena[ctx->mem_off[sig_idx] + addr] = val & crosstalk_hdl_exec_mask_get(ctx->ir_ctx->sigs[sig_idx].width);
    ctx->mem_arena_hi[ctx->mem_off[sig_idx] + addr] = 0;
    ctx->poke_dirty = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_exec_mem_ld(const crosstalk_hdl_exec_ctx_t *ctx, uint32_t sig_idx, uint64_t addr, uint64_t *out_val)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_val == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_val = 0;

    if (LDG_UNLIKELY(sig_idx >= ctx->ir_ctx->sig_cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(ctx->mem_off[sig_idx] == UINT32_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (addr >= (uint64_t)ctx->ir_ctx->sigs[sig_idx].depth) { return LDG_ERR_AOK; }

    *out_val = ctx->mem_arena[ctx->mem_off[sig_idx] + addr];

    return LDG_ERR_AOK;
}

static void crosstalk_hdl_exec_shl_wide(uint64_t lo, uint64_t hi, uint64_t amt, uint64_t amt_hi, uint64_t *out_lo, uint64_t *out_hi)
{
    ldg_uint128_t op_val = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t res_val = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    *out_lo = 0;
    *out_hi = 0;

    if (amt_hi != 0 || amt >= 128) { return; }

    op_val.limb[0] = lo;
    op_val.limb[1] = hi;

    ret = ldg_arith_uint128_shl(&op_val, (uint32_t)amt, &res_val);
    if (ret != LDG_ERR_AOK) { return; }

    *out_lo = res_val.limb[0];
    *out_hi = res_val.limb[1];
}

static void crosstalk_hdl_exec_shr_wide(uint64_t lo, uint64_t hi, uint64_t amt, uint64_t amt_hi, uint64_t *out_lo, uint64_t *out_hi)
{
    ldg_uint128_t op_val = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t res_val = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    *out_lo = 0;
    *out_hi = 0;

    if (amt_hi != 0 || amt >= 128) { return; }

    op_val.limb[0] = lo;
    op_val.limb[1] = hi;

    ret = ldg_arith_uint128_shr(&op_val, (uint32_t)amt, &res_val);
    if (ret != LDG_ERR_AOK) { return; }

    *out_lo = res_val.limb[0];
    *out_hi = res_val.limb[1];
}

static uint32_t crosstalk_hdl_exec_op_apply(crosstalk_hdl_exec_ctx_t *ctx, const crosstalk_hdl_ir_op_t *op, uint8_t seq, uint8_t *out_changed)
{
    uint64_t src0_val = 0;
    uint64_t src1_val = 0;
    uint64_t src2_val = 0;
    uint64_t src0_hi = 0;
    uint64_t src1_hi = 0;
    uint64_t src2_hi = 0;
    uint64_t res = 0;
    uint64_t res_hi = 0;
    uint64_t dst_mask = 0;
    uint64_t dst_mask_hi = 0;
    uint64_t op_mask = 0;
    uint64_t op_mask_hi = 0;
    uint32_t dst_w = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || op == 0x0 || out_changed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (op->src0 != UINT32_MAX) { src0_val = ctx->vals[op->src0]; src0_hi = ctx->vals_hi[op->src0]; }

    if (op->src1 != UINT32_MAX) { src1_val = ctx->vals[op->src1]; src1_hi = ctx->vals_hi[op->src1]; }

    if (op->src2 != UINT32_MAX) { src2_val = ctx->vals[op->src2]; src2_hi = ctx->vals_hi[op->src2]; }

    op_mask = crosstalk_hdl_exec_mask_get(op->width);
    op_mask_hi = crosstalk_hdl_exec_mask_hi_get(op->width);

    if (op->kind == CROSSTALK_HDL_IR_OP_MEM_WR)
    {
        uint64_t en = 0x1;

        if (op->dst != UINT32_MAX) { en = ctx->vals[op->dst]; }

        if (en != 0)
        {
            if (seq == LDG_TRUTH_TRUE)
            {
                if (LDG_UNLIKELY(ctx->stage_cunt >= ctx->stage_cap)) { return LDG_ERR_FULL; }

                ctx->stage[ctx->stage_cunt].mem_is = LDG_TRUTH_TRUE;
                ctx->stage[ctx->stage_cunt].sig = op->src0;
                ctx->stage[ctx->stage_cunt].addr = src1_val;
                ctx->stage[ctx->stage_cunt].val = src2_val;
                ctx->stage[ctx->stage_cunt].val_hi = src2_hi;
                ctx->stage_cunt++;
            }
            else if (src1_val < (uint64_t)ctx->ir_ctx->sigs[op->src0].depth)
            {
                ctx->mem_arena[ctx->mem_off[op->src0] + src1_val] = src2_val & crosstalk_hdl_exec_mask_get(ctx->ir_ctx->sigs[op->src0].width);
                ctx->mem_arena_hi[ctx->mem_off[op->src0] + src1_val] = src2_hi & crosstalk_hdl_exec_mask_hi_get(ctx->ir_ctx->sigs[op->src0].width);
            }
        }

        return LDG_ERR_AOK;
    }

    if (op->kind == CROSSTALK_HDL_IR_OP_REG_NEXT)
    {
        if (LDG_UNLIKELY(ctx->stage_cunt >= ctx->stage_cap)) { return LDG_ERR_FULL; }

        ctx->stage[ctx->stage_cunt].mem_is = LDG_TRUTH_FALSE;
        ctx->stage[ctx->stage_cunt].sig = op->dst;
        ctx->stage[ctx->stage_cunt].addr = 0;
        ctx->stage[ctx->stage_cunt].val = src0_val;
        ctx->stage[ctx->stage_cunt].val_hi = src0_hi;
        ctx->stage_cunt++;

        return LDG_ERR_AOK;
    }

    switch (op->kind)
    {
        case CROSSTALK_HDL_IR_OP_CONST:
            res = op->imm;
            break;
        case CROSSTALK_HDL_IR_OP_COPY:
            res = src0_val;
            res_hi = src0_hi;
            break;
        case CROSSTALK_HDL_IR_OP_ZEXT:
            res = src0_val;
            res_hi = src0_hi;
            break;
        case CROSSTALK_HDL_IR_OP_SEXT:
        {
            if (op->imm <= 64)
            {
                int64_t sval = crosstalk_hdl_exec_sext_get(src0_val, (uint32_t)op->imm);

                res = (uint64_t)sval;
                res_hi = sval < 0 ? ~(uint64_t)0x0 : 0x0;
            }
            else
            {
                uint64_t sign = (src0_hi >> ((uint32_t)op->imm - 65)) & 0x1;

                res = src0_val;
                res_hi = sign != 0 ? src0_hi | ~crosstalk_hdl_exec_mask_hi_get((uint32_t)op->imm) : src0_hi;
            }

            break;
        }
        case CROSSTALK_HDL_IR_OP_AND:
            res = src0_val & src1_val;
            res_hi = src0_hi & src1_hi;
            break;
        case CROSSTALK_HDL_IR_OP_OR:
            res = src0_val | src1_val;
            res_hi = src0_hi | src1_hi;
            break;
        case CROSSTALK_HDL_IR_OP_XOR:
            res = src0_val ^ src1_val;
            res_hi = src0_hi ^ src1_hi;
            break;
        case CROSSTALK_HDL_IR_OP_XNOR:
            res = ~(src0_val ^ src1_val);
            res_hi = ~(src0_hi ^ src1_hi);
            break;
        case CROSSTALK_HDL_IR_OP_NOT:
            res = ~src0_val;
            res_hi = ~src0_hi;
            break;
        case CROSSTALK_HDL_IR_OP_ADD:
        {
            res = src0_val + src1_val;
            res_hi = src0_hi + src1_hi + (res < src0_val ? 0x1 : 0x0);
            break;
        }
        case CROSSTALK_HDL_IR_OP_SUB:
        {
            res = src0_val - src1_val;
            res_hi = src0_hi - src1_hi - (src0_val < src1_val ? 0x1 : 0x0);
            break;
        }
        case CROSSTALK_HDL_IR_OP_MUL:
        {
            ldg_uint128_t mul_lhs = LDG_STRUCT_ZERO_INIT;
            ldg_uint128_t mul_rhs = LDG_STRUCT_ZERO_INIT;
            ldg_uint256_t mul_res = LDG_STRUCT_ZERO_INIT;

            mul_lhs.limb[0] = src0_val;
            mul_lhs.limb[1] = src0_hi;
            mul_rhs.limb[0] = src1_val;
            mul_rhs.limb[1] = src1_hi;

            if (ldg_arith_uint128_to_uint256_mul(&mul_lhs, &mul_rhs, &mul_res) == LDG_ERR_AOK)
            {
                res = mul_res.limb[0];
                res_hi = mul_res.limb[1];
            }

            break;
        }
        case CROSSTALK_HDL_IR_OP_DIV:
        case CROSSTALK_HDL_IR_OP_MOD:
        {
            ldg_uint128_t div_lhs = LDG_STRUCT_ZERO_INIT;
            ldg_uint128_t div_rhs = LDG_STRUCT_ZERO_INIT;
            ldg_uint128_t div_quot = LDG_STRUCT_ZERO_INIT;
            ldg_uint128_t div_rem = LDG_STRUCT_ZERO_INIT;

            if ((src1_val | src1_hi) == 0) { break; }

            div_lhs.limb[0] = src0_val;
            div_lhs.limb[1] = src0_hi;
            div_rhs.limb[0] = src1_val;
            div_rhs.limb[1] = src1_hi;

            if (ldg_arith_uint128_div_compute(&div_lhs, &div_rhs, &div_quot, &div_rem) == LDG_ERR_AOK)
            {
                if (op->kind == CROSSTALK_HDL_IR_OP_DIV)
                {
                    res = div_quot.limb[0];
                    res_hi = div_quot.limb[1];
                }
                else
                {
                    res = div_rem.limb[0];
                    res_hi = div_rem.limb[1];
                }
            }

            break;
        }
        case CROSSTALK_HDL_IR_OP_NEG:
        {
            res = ~src0_val + 0x1;
            res_hi = ~src0_hi + (src0_val == 0 ? 0x1 : 0x0);
            break;
        }
        case CROSSTALK_HDL_IR_OP_SHL:
            crosstalk_hdl_exec_shl_wide(src0_val, src0_hi, src1_val, src1_hi, &res, &res_hi);
            break;
        case CROSSTALK_HDL_IR_OP_SHR:
            crosstalk_hdl_exec_shr_wide(src0_val, src0_hi, src1_val, src1_hi, &res, &res_hi);
            break;
        case CROSSTALK_HDL_IR_OP_ASHR:
        {
            if (op->width <= 64)
            {
                int64_t sval = crosstalk_hdl_exec_sext_get(src0_val, op->width);

                if (src1_hi != 0 || src1_val >= 64) { res = sval < 0 ? ~(uint64_t)0x0 : 0x0; }
                else { res = (uint64_t)(sval >> src1_val); }

                res_hi = sval < 0 ? ~(uint64_t)0x0 : 0x0;
            }
            else
            {
                uint64_t sign = (src0_hi >> (op->width - 65)) & 0x1;

                if (src1_hi != 0 || src1_val >= (uint64_t)op->width)
                {
                    res = sign != 0 ? ~(uint64_t)0x0 : 0x0;
                    res_hi = res;
                }
                else
                {
                    crosstalk_hdl_exec_shr_wide(src0_val, src0_hi, src1_val, 0x0, &res, &res_hi);

                    if (sign != 0)
                    {
                        uint64_t fill_lo = 0;
                        uint64_t fill_hi = 0;

                        crosstalk_hdl_exec_shl_wide(crosstalk_hdl_exec_mask_get((uint32_t)src1_val), src1_val > 64 ? crosstalk_hdl_exec_mask_get((uint32_t)src1_val - 64) : 0x0, (uint64_t)op->width - src1_val, 0x0, &fill_lo, &fill_hi);

                        res = res | fill_lo;
                        res_hi = res_hi | fill_hi;
                    }
                }
            }

            break;
        }
        case CROSSTALK_HDL_IR_OP_EQ:
            res = src0_val == src1_val && src0_hi == src1_hi ? 0x1 : 0x0;
            break;
        case CROSSTALK_HDL_IR_OP_NEQ:
            res = src0_val != src1_val || src0_hi != src1_hi ? 0x1 : 0x0;
            break;
        case CROSSTALK_HDL_IR_OP_LT:
            res = src0_hi != src1_hi ? (src0_hi < src1_hi ? 0x1 : 0x0) : (src0_val < src1_val ? 0x1 : 0x0);
            break;
        case CROSSTALK_HDL_IR_OP_LE:
            res = src0_hi != src1_hi ? (src0_hi < src1_hi ? 0x1 : 0x0) : (src0_val <= src1_val ? 0x1 : 0x0);
            break;
        case CROSSTALK_HDL_IR_OP_GT:
            res = src0_hi != src1_hi ? (src0_hi > src1_hi ? 0x1 : 0x0) : (src0_val > src1_val ? 0x1 : 0x0);
            break;
        case CROSSTALK_HDL_IR_OP_GE:
            res = src0_hi != src1_hi ? (src0_hi > src1_hi ? 0x1 : 0x0) : (src0_val >= src1_val ? 0x1 : 0x0);
            break;
        case CROSSTALK_HDL_IR_OP_LT_S:
        case CROSSTALK_HDL_IR_OP_LE_S:
        case CROSSTALK_HDL_IR_OP_GT_S:
        case CROSSTALK_HDL_IR_OP_GE_S:
        {
            uint64_t flip_a_lo = src0_val;
            uint64_t flip_a_hi = src0_hi;
            uint64_t flip_b_lo = src1_val;
            uint64_t flip_b_hi = src1_hi;
            uint8_t lt_is = 0;
            uint8_t eq_is = 0;

            if (op->width <= 64)
            {
                flip_a_lo = src0_val ^ ((uint64_t)0x1 << (op->width - 0x1));
                flip_b_lo = src1_val ^ ((uint64_t)0x1 << (op->width - 0x1));
                flip_a_hi = 0;
                flip_b_hi = 0;
            }
            else
            {
                flip_a_hi = src0_hi ^ ((uint64_t)0x1 << (op->width - 65));
                flip_b_hi = src1_hi ^ ((uint64_t)0x1 << (op->width - 65));
            }

            lt_is = flip_a_hi != flip_b_hi ? (flip_a_hi < flip_b_hi ? 0x1 : 0x0) : (flip_a_lo < flip_b_lo ? 0x1 : 0x0);
            eq_is = flip_a_hi == flip_b_hi && flip_a_lo == flip_b_lo ? 0x1 : 0x0;

            if (op->kind == CROSSTALK_HDL_IR_OP_LT_S) { res = lt_is; }
            else if (op->kind == CROSSTALK_HDL_IR_OP_LE_S) { res = lt_is != 0 || eq_is != 0 ? 0x1 : 0x0; }
            else if (op->kind == CROSSTALK_HDL_IR_OP_GT_S) { res = lt_is == 0 && eq_is == 0 ? 0x1 : 0x0; }
            else { res = lt_is == 0 ? 0x1 : 0x0; }

            break;
        }
        case CROSSTALK_HDL_IR_OP_LAND:
            res = (src0_val | src0_hi) != 0 && (src1_val | src1_hi) != 0 ? 0x1 : 0x0;
            break;
        case CROSSTALK_HDL_IR_OP_LOR:
            res = (src0_val | src0_hi) != 0 || (src1_val | src1_hi) != 0 ? 0x1 : 0x0;
            break;
        case CROSSTALK_HDL_IR_OP_LNOT:
            res = (src0_val | src0_hi) == 0 ? 0x1 : 0x0;
            break;
        case CROSSTALK_HDL_IR_OP_RED_AND:
            res = src0_val == crosstalk_hdl_exec_mask_get((uint32_t)op->imm) && src0_hi == crosstalk_hdl_exec_mask_hi_get((uint32_t)op->imm) ? 0x1 : 0x0;
            break;
        case CROSSTALK_HDL_IR_OP_RED_OR:
            res = (src0_val | src0_hi) != 0 ? 0x1 : 0x0;
            break;
        case CROSSTALK_HDL_IR_OP_RED_XOR:
        {
            uint64_t fold = src0_val ^ src0_hi;

            fold = fold ^ (fold >> 32);
            fold = fold ^ (fold >> 16);
            fold = fold ^ (fold >> 8);
            fold = fold ^ (fold >> 4);
            fold = fold ^ (fold >> 2);
            fold = fold ^ (fold >> 0x1);
            res = fold & 0x1;
            break;
        }
        case CROSSTALK_HDL_IR_OP_CONCAT:
        {
            uint64_t sh_lo = 0;
            uint64_t sh_hi = 0;

            crosstalk_hdl_exec_shl_wide(src0_val, src0_hi, op->imm, 0x0, &sh_lo, &sh_hi);
            res = sh_lo | (src1_val & crosstalk_hdl_exec_mask_get((uint32_t)op->imm));
            res_hi = sh_hi | (src1_hi & crosstalk_hdl_exec_mask_hi_get((uint32_t)op->imm));
            break;
        }
        case CROSSTALK_HDL_IR_OP_SLICE:
        {
            crosstalk_hdl_exec_shr_wide(src0_val, src0_hi, op->imm, 0x0, &res, &res_hi);
            res = res & op_mask;
            res_hi = res_hi & op_mask_hi;
            break;
        }
        case CROSSTALK_HDL_IR_OP_SLICE_DYN:
        {
            crosstalk_hdl_exec_shr_wide(src0_val, src0_hi, src1_val, src1_hi, &res, &res_hi);
            res = res & op_mask;
            res_hi = res_hi & op_mask_hi;
            break;
        }
        case CROSSTALK_HDL_IR_OP_INSERT:
        case CROSSTALK_HDL_IR_OP_INSERT_DYN:
        {
            uint64_t ins_amt = op->kind == CROSSTALK_HDL_IR_OP_INSERT ? op->imm : src2_val;
            uint64_t ins_amt_hi = op->kind == CROSSTALK_HDL_IR_OP_INSERT ? 0x0 : src2_hi;
            uint64_t hole_lo = 0;
            uint64_t hole_hi = 0;
            uint64_t fld_lo = 0;
            uint64_t fld_hi = 0;

            if (ins_amt_hi != 0 || ins_amt >= 128)
            {
                res = src0_val;
                res_hi = src0_hi;
                break;
            }

            crosstalk_hdl_exec_shl_wide(op_mask, op_mask_hi, ins_amt, 0x0, &hole_lo, &hole_hi);
            crosstalk_hdl_exec_shl_wide(src1_val & op_mask, src1_hi & op_mask_hi, ins_amt, 0x0, &fld_lo, &fld_hi);
            res = (src0_val & ~hole_lo) | fld_lo;
            res_hi = (src0_hi & ~hole_hi) | fld_hi;
            break;
        }
        case CROSSTALK_HDL_IR_OP_MUX:
        {
            res = (src0_val | src0_hi) != 0 ? src1_val : src2_val;
            res_hi = (src0_val | src0_hi) != 0 ? src1_hi : src2_hi;
            break;
        }
        case CROSSTALK_HDL_IR_OP_MEM_RD:
        {
            res = 0;
            res_hi = 0;
            if (src1_val < (uint64_t)ctx->ir_ctx->sigs[op->src0].depth)
            {
                res = ctx->mem_arena[ctx->mem_off[op->src0] + src1_val];
                res_hi = ctx->mem_arena_hi[ctx->mem_off[op->src0] + src1_val];
            }

            break;
        }
        default:
            return LDG_ERR_UNSUPPORTED;
    }

    dst_w = ctx->ir_ctx->sigs[op->dst].width;
    dst_mask = crosstalk_hdl_exec_mask_get(dst_w);
    dst_mask_hi = crosstalk_hdl_exec_mask_hi_get(dst_w);
    res = res & dst_mask;
    res_hi = res_hi & dst_mask_hi;

    if (ctx->vals[op->dst] != res || ctx->vals_hi[op->dst] != res_hi)
    {
        ctx->vals[op->dst] = res;
        ctx->vals_hi[op->dst] = res_hi;
        *out_changed = LDG_TRUTH_TRUE;
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_exec_eval_level(crosstalk_hdl_exec_ctx_t *ctx)
{
    const crosstalk_hdl_level_res_t *lvl = 0x0;
    const crosstalk_hdl_level_unit_t *unit = 0x0;
    uint32_t unit_idx = 0;
    uint32_t pos = 0;
    uint32_t pass = 0;
    uint32_t ret = 0;
    uint8_t changed = 0;
    uint8_t op_changed = 0;

    lvl = ctx->level_res;
    ctx->busy_sig = UINT32_MAX;
    ctx->busy_op = UINT32_MAX;
    ctx->eval_cunt = ctx->eval_cunt + 0x1;

    for (unit_idx = 0; unit_idx < lvl->unit_cunt; unit_idx++)
    {
        unit = &lvl->units[unit_idx];

        if (unit->iter_is == 0)
        {
            if (ctx->nrw != 0x0)
            {
                for (pos = 0; pos < unit->cunt; pos++)
                {
                    op_changed = LDG_TRUTH_FALSE;

                    ret = crosstalk_hdl_exec_nrw_apply(ctx, &ctx->nrw[lvl->order[unit->start + pos]], LDG_TRUTH_FALSE, &op_changed);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    if (op_changed == LDG_TRUTH_TRUE) { ctx->act_op_cunt = ctx->act_op_cunt + 0x1; }
                }

                continue;
            }

            for (pos = 0; pos < unit->cunt; pos++)
            {
                op_changed = LDG_TRUTH_FALSE;

                ret = crosstalk_hdl_exec_op_apply(ctx, &ctx->ir_ctx->comb_ops[lvl->order[unit->start + pos]], LDG_TRUTH_FALSE, &op_changed);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                if (op_changed == LDG_TRUTH_TRUE) { ctx->act_op_cunt = ctx->act_op_cunt + 0x1; }
            }

            continue;
        }

        for (pass = 0; pass < CROSSTALK_HDL_EXEC_SETTLE_MAX; pass++)
        {
            changed = LDG_TRUTH_FALSE;
            ctx->busy_sig = UINT32_MAX;
            ctx->busy_op = UINT32_MAX;

            for (pos = 0; pos < unit->cunt; pos++)
            {
                op_changed = LDG_TRUTH_FALSE;

                ret = ctx->nrw != 0x0 ? crosstalk_hdl_exec_nrw_apply(ctx, &ctx->nrw[lvl->order[unit->start + pos]], LDG_TRUTH_FALSE, &op_changed) : crosstalk_hdl_exec_op_apply(ctx, &ctx->ir_ctx->comb_ops[lvl->order[unit->start + pos]], LDG_TRUTH_FALSE, &op_changed);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                if (op_changed == LDG_TRUTH_TRUE)
                {
                    changed = LDG_TRUTH_TRUE;
                    ctx->act_op_cunt = ctx->act_op_cunt + 0x1;

                    if (ctx->busy_op == UINT32_MAX)
                    {
                        ctx->busy_sig = ctx->ir_ctx->comb_ops[lvl->order[unit->start + pos]].dst;
                        ctx->busy_op = lvl->order[unit->start + pos];
                    }
                }
            }

            if (changed != LDG_TRUTH_TRUE) { break; }
        }

        if (LDG_UNLIKELY(pass >= CROSSTALK_HDL_EXEC_SETTLE_MAX)) { return LDG_ERR_BUSY; }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_exec_level_set(crosstalk_hdl_exec_ctx_t *ctx, const crosstalk_hdl_level_res_t *level_res)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->level_res = level_res;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_exec_fast_set(crosstalk_hdl_exec_ctx_t *ctx, uint32_t clk_sig, const uint32_t *alias_sigs, uint32_t alias_cunt)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(clk_sig >= ctx->ir_ctx->sig_cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(alias_cunt > CROSSTALK_HDL_EXEC_FAST_ALIAS_CAP || (alias_cunt > 0 && alias_sigs == 0x0))) { return LDG_ERR_FUNC_ARG_INVALID; }

    ctx->fast_clk = clk_sig;
    ctx->fast_alias_cunt = alias_cunt;

    for (idx = 0; idx < alias_cunt; idx++) { ctx->fast_alias[idx] = alias_sigs[idx]; }

    ctx->fast_ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_exec_eval(crosstalk_hdl_exec_ctx_t *ctx)
{
    uint32_t pass = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;
    uint8_t changed = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->level_res != 0x0) { return crosstalk_hdl_exec_eval_level(ctx); }

    ctx->eval_cunt = ctx->eval_cunt + 0x1;

    for (pass = 0; pass < CROSSTALK_HDL_EXEC_SETTLE_MAX; pass++)
    {
        changed = LDG_TRUTH_FALSE;
        ctx->busy_sig = UINT32_MAX;
        ctx->busy_op = UINT32_MAX;

        for (idx = 0; idx < ctx->ir_ctx->comb_cunt; idx++)
        {
            uint8_t op_changed = LDG_TRUTH_FALSE;

            ret = ctx->nrw != 0x0 ? crosstalk_hdl_exec_nrw_apply(ctx, &ctx->nrw[idx], LDG_TRUTH_FALSE, &op_changed) : crosstalk_hdl_exec_op_apply(ctx, &ctx->ir_ctx->comb_ops[idx], LDG_TRUTH_FALSE, &op_changed);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (op_changed == LDG_TRUTH_TRUE)
            {
                changed = LDG_TRUTH_TRUE;
                ctx->act_op_cunt = ctx->act_op_cunt + 0x1;

                if (ctx->busy_op == UINT32_MAX)
                {
                    ctx->busy_sig = ctx->ir_ctx->comb_ops[idx].dst;
                    ctx->busy_op = idx;
                }
            }
        }

        if (changed != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }
    }

    return LDG_ERR_BUSY;
}

static uint32_t crosstalk_hdl_exec_settled(crosstalk_hdl_exec_ctx_t *ctx, uint32_t ret)
{
    if (ret != LDG_ERR_AOK || ctx->systask == 0x0) { return ret; }

    return crosstalk_hdl_systask_settle_run((crosstalk_hdl_systask_ctx_t *)ctx->systask, ctx);
}

uint32_t crosstalk_hdl_exec_step(crosstalk_hdl_exec_ctx_t *ctx)
{
    const crosstalk_hdl_ir_seq_blk_t *blk = 0x0;
    uint64_t clk_val = 0;
    uint64_t rst_val = 0;
    uint32_t blk_idx = 0;
    uint32_t op_idx = 0;
    uint32_t ret = 0;
    uint8_t fire = 0;
    uint8_t changed = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_exec_eval(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->stage_cunt = 0;

    for (blk_idx = 0; blk_idx < ctx->ir_ctx->seq_blk_cunt; blk_idx++)
    {
        blk = &ctx->ir_ctx->seq_blks[blk_idx];
        fire = LDG_TRUTH_FALSE;

        clk_val = blk->clk_sig == UINT32_MAX ? 0x0 : ctx->vals[blk->clk_sig];

        if (blk->clk_sig != UINT32_MAX)
        {
            if (blk->clk_edge == CROSSTALK_HDL_IR_EDGE_POS && ctx->prev_clk[blk_idx] == 0 && clk_val != 0) { fire = LDG_TRUTH_TRUE; }

            if (blk->clk_edge == CROSSTALK_HDL_IR_EDGE_NEG && ctx->prev_clk[blk_idx] != 0 && clk_val == 0) { fire = LDG_TRUTH_TRUE; }
        }

        rst_val = 0;
        if (blk->rst_sig != UINT32_MAX)
        {
            rst_val = ctx->vals[blk->rst_sig];

            if (blk->rst_edge == CROSSTALK_HDL_IR_EDGE_POS && ctx->prev_rst[blk_idx] == 0 && rst_val != 0) { fire = LDG_TRUTH_TRUE; }

            if (blk->rst_edge == CROSSTALK_HDL_IR_EDGE_NEG && ctx->prev_rst[blk_idx] != 0 && rst_val == 0) { fire = LDG_TRUTH_TRUE; }
        }

        if (fire == LDG_TRUTH_TRUE)
        {
            for (op_idx = 0; op_idx < blk->op_cunt; op_idx++)
            {
                ret = ctx->nrw != 0x0 ? crosstalk_hdl_exec_nrw_apply(ctx, &ctx->nrw[ctx->ir_ctx->comb_cunt + blk->op_start + op_idx], LDG_TRUTH_TRUE, &changed) : crosstalk_hdl_exec_op_apply(ctx, &ctx->ir_ctx->seq_ops[blk->op_start + op_idx], LDG_TRUTH_TRUE, &changed);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            if (ctx->systask != 0x0)
            {
                ret = crosstalk_hdl_systask_blk_run((crosstalk_hdl_systask_ctx_t *)ctx->systask, ctx, blk_idx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
        }

        ctx->prev_clk[blk_idx] = clk_val;
        ctx->prev_rst[blk_idx] = rst_val;
    }

    crosstalk_hdl_exec_stage_commit(ctx, &changed);
    ctx->tick_cunt++;
    ctx->poke_dirty = LDG_TRUTH_FALSE;

    if (changed != LDG_TRUTH_TRUE) { return crosstalk_hdl_exec_settled(ctx, LDG_ERR_AOK); }

    return crosstalk_hdl_exec_settled(ctx, crosstalk_hdl_exec_eval(ctx));
}

static uint32_t crosstalk_hdl_exec_cycle_fast(crosstalk_hdl_exec_ctx_t *ctx, uint32_t clk_sig)
{
    const crosstalk_hdl_ir_seq_blk_t *blk = 0x0;
    uint64_t rst_val = 0;
    uint32_t blk_idx = 0;
    uint32_t op_idx = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;
    uint8_t changed = 0;

    ctx->vals[clk_sig] = 0x1;

    for (idx = 0; idx < ctx->fast_alias_cunt; idx++) { ctx->vals[ctx->fast_alias[idx]] = 0x1; }

    ctx->stage_cunt = 0;

    for (blk_idx = 0; blk_idx < ctx->ir_ctx->seq_blk_cunt; blk_idx++)
    {
        blk = &ctx->ir_ctx->seq_blks[blk_idx];

        rst_val = 0;
        if (blk->rst_sig != UINT32_MAX) { rst_val = ctx->vals[blk->rst_sig]; }

        for (op_idx = 0; op_idx < blk->op_cunt; op_idx++)
        {
            ret = ctx->nrw != 0x0 ? crosstalk_hdl_exec_nrw_apply(ctx, &ctx->nrw[ctx->ir_ctx->comb_cunt + blk->op_start + op_idx], LDG_TRUTH_TRUE, &changed) : crosstalk_hdl_exec_op_apply(ctx, &ctx->ir_ctx->seq_ops[blk->op_start + op_idx], LDG_TRUTH_TRUE, &changed);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        if (ctx->systask != 0x0)
        {
            ret = crosstalk_hdl_systask_blk_run((crosstalk_hdl_systask_ctx_t *)ctx->systask, ctx, blk_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ctx->prev_clk[blk_idx] = 0x0;
        ctx->prev_rst[blk_idx] = rst_val;
    }

    crosstalk_hdl_exec_stage_commit(ctx, &changed);
    ctx->vals[clk_sig] = 0x0;

    for (idx = 0; idx < ctx->fast_alias_cunt; idx++) { ctx->vals[ctx->fast_alias[idx]] = 0x0; }

    if (changed != LDG_TRUTH_TRUE) { return crosstalk_hdl_exec_settled(ctx, LDG_ERR_AOK); }

    return crosstalk_hdl_exec_settled(ctx, crosstalk_hdl_exec_eval(ctx));
}

uint32_t crosstalk_hdl_exec_cycle(crosstalk_hdl_exec_ctx_t *ctx, uint32_t clk_sig)
{
    const crosstalk_hdl_ir_seq_blk_t *blk = 0x0;
    uint64_t clk_val = 0;
    uint64_t rst_val = 0;
    uint32_t blk_idx = 0;
    uint32_t op_idx = 0;
    uint32_t ret = 0;
    uint8_t fire = 0;
    uint8_t changed = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(clk_sig >= ctx->ir_ctx->sig_cunt)) { return LDG_ERR_BOUNDS; }

    ctx->cycle_cunt = ctx->cycle_cunt + 0x1;

    if (ctx->gate_on == LDG_TRUTH_TRUE && ctx->gate != 0x0) { return crosstalk_hdl_logic_gate_cycle_run((crosstalk_hdl_logic_gate_ctx_t *)ctx->gate, ctx, clk_sig); }

    if (ctx->fast_ok == LDG_TRUTH_TRUE && ctx->poke_dirty != LDG_TRUTH_TRUE && clk_sig == ctx->fast_clk) { return crosstalk_hdl_exec_cycle_fast(ctx, clk_sig); }

    ctx->poke_dirty = LDG_TRUTH_FALSE;
    ctx->vals[clk_sig] = 0x1;

    ret = crosstalk_hdl_exec_eval(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->stage_cunt = 0;

    for (blk_idx = 0; blk_idx < ctx->ir_ctx->seq_blk_cunt; blk_idx++)
    {
        blk = &ctx->ir_ctx->seq_blks[blk_idx];
        fire = LDG_TRUTH_FALSE;

        clk_val = blk->clk_sig == UINT32_MAX ? 0x0 : ctx->vals[blk->clk_sig];

        if (blk->clk_sig != UINT32_MAX)
        {
            if (blk->clk_edge == CROSSTALK_HDL_IR_EDGE_POS && ctx->prev_clk[blk_idx] == 0 && clk_val != 0) { fire = LDG_TRUTH_TRUE; }

            if (blk->clk_edge == CROSSTALK_HDL_IR_EDGE_NEG && ctx->prev_clk[blk_idx] != 0 && clk_val == 0) { fire = LDG_TRUTH_TRUE; }
        }

        rst_val = 0;
        if (blk->rst_sig != UINT32_MAX)
        {
            rst_val = ctx->vals[blk->rst_sig];

            if (blk->rst_edge == CROSSTALK_HDL_IR_EDGE_POS && ctx->prev_rst[blk_idx] == 0 && rst_val != 0) { fire = LDG_TRUTH_TRUE; }

            if (blk->rst_edge == CROSSTALK_HDL_IR_EDGE_NEG && ctx->prev_rst[blk_idx] != 0 && rst_val == 0) { fire = LDG_TRUTH_TRUE; }
        }

        if (fire == LDG_TRUTH_TRUE)
        {
            for (op_idx = 0; op_idx < blk->op_cunt; op_idx++)
            {
                ret = ctx->nrw != 0x0 ? crosstalk_hdl_exec_nrw_apply(ctx, &ctx->nrw[ctx->ir_ctx->comb_cunt + blk->op_start + op_idx], LDG_TRUTH_TRUE, &changed) : crosstalk_hdl_exec_op_apply(ctx, &ctx->ir_ctx->seq_ops[blk->op_start + op_idx], LDG_TRUTH_TRUE, &changed);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            if (ctx->systask != 0x0)
            {
                ret = crosstalk_hdl_systask_blk_run((crosstalk_hdl_systask_ctx_t *)ctx->systask, ctx, blk_idx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
        }

        ctx->prev_clk[blk_idx] = clk_val;
        ctx->prev_rst[blk_idx] = rst_val;
    }

    crosstalk_hdl_exec_stage_commit(ctx, &changed);
    ctx->vals[clk_sig] = 0x0;

    ret = crosstalk_hdl_exec_eval(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (blk_idx = 0; blk_idx < ctx->ir_ctx->seq_blk_cunt; blk_idx++)
    {
        blk = &ctx->ir_ctx->seq_blks[blk_idx];
        fire = LDG_TRUTH_FALSE;

        clk_val = blk->clk_sig == UINT32_MAX ? 0x0 : ctx->vals[blk->clk_sig];

        if (blk->clk_sig != UINT32_MAX && blk->clk_edge == CROSSTALK_HDL_IR_EDGE_NEG && ctx->prev_clk[blk_idx] != 0 && clk_val == 0) { fire = LDG_TRUTH_TRUE; }

        rst_val = 0;
        if (blk->rst_sig != UINT32_MAX)
        {
            rst_val = ctx->vals[blk->rst_sig];

            if (blk->rst_edge == CROSSTALK_HDL_IR_EDGE_POS && ctx->prev_rst[blk_idx] == 0 && rst_val != 0) { fire = LDG_TRUTH_TRUE; }

            if (blk->rst_edge == CROSSTALK_HDL_IR_EDGE_NEG && ctx->prev_rst[blk_idx] != 0 && rst_val == 0) { fire = LDG_TRUTH_TRUE; }
        }

        if (fire == LDG_TRUTH_TRUE)
        {
            for (op_idx = 0; op_idx < blk->op_cunt; op_idx++)
            {
                ret = ctx->nrw != 0x0 ? crosstalk_hdl_exec_nrw_apply(ctx, &ctx->nrw[ctx->ir_ctx->comb_cunt + blk->op_start + op_idx], LDG_TRUTH_TRUE, &changed) : crosstalk_hdl_exec_op_apply(ctx, &ctx->ir_ctx->seq_ops[blk->op_start + op_idx], LDG_TRUTH_TRUE, &changed);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            if (ctx->systask != 0x0)
            {
                ret = crosstalk_hdl_systask_blk_run((crosstalk_hdl_systask_ctx_t *)ctx->systask, ctx, blk_idx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
        }

        ctx->prev_clk[blk_idx] = clk_val;
        ctx->prev_rst[blk_idx] = rst_val;
    }

    if (ctx->stage_cunt > 0)
    {
        crosstalk_hdl_exec_stage_commit(ctx, &changed);

        if (changed == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_exec_eval(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }

    ctx->tick_cunt++;

    return crosstalk_hdl_exec_settled(ctx, LDG_ERR_AOK);
}
