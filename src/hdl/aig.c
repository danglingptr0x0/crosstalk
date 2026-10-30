#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>

#include "hdl/ir.h"
#include "hdl/aig.h"

#define CROSSTALK_HDL_AIG_SHIFT_LAYER_CUNT 6
#define CROSSTALK_HDL_AIG_HASH_BYTE_FILL 255

static uint32_t crosstalk_hdl_aig_arr_alloc(void *pool, uint64_t cunt, uint64_t item_size, void **out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;
    if (cunt == 0) { cunt = 0x1; }

    ret = ldg_mem_pool_alloc((ldg_mem_pool_t *)pool, cunt * item_size, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    memset(*out, 0x0, cunt * item_size);
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_aig_lit_not_get(uint32_t lit)
{
    return lit ^ 0x1;
}

static uint32_t crosstalk_hdl_aig_pi_get(crosstalk_hdl_aig_ctx_t *ctx, uint32_t *out_lit)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_lit == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_lit = CROSSTALK_HDL_AIG_LIT_FALSE;
    if (LDG_UNLIKELY(ctx->node_cunt >= ctx->node_cap)) { return CROSSTALK_HDL_ERR_AIG_CAP; }

    ctx->nodes[ctx->node_cunt].fanin0 = UINT32_MAX;
    ctx->nodes[ctx->node_cunt].fanin1 = UINT32_MAX;
    *out_lit = ctx->node_cunt << 0x1;
    ctx->node_cunt = ctx->node_cunt + 0x1;
    ctx->pi_cunt = ctx->pi_cunt + 0x1;
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_aig_pi_note(crosstalk_hdl_aig_ctx_t *ctx, uint32_t kind, uint32_t sig, uint32_t bit, uint32_t op_idx, uint32_t lit)
{
    uint32_t pi_idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->pi_cunt == 0 || ctx->pi_cunt > ctx->pi_cap)) { return CROSSTALK_HDL_ERR_AIG_CAP; }

    pi_idx = ctx->pi_cunt - 0x1;
    ctx->pi_node[pi_idx] = lit >> 0x1;
    ctx->pi_sig[pi_idx] = sig;
    ctx->pi_bit[pi_idx] = bit;
    ctx->pi_op[pi_idx] = op_idx;
    ctx->pi_kind[pi_idx] = (uint8_t)kind;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_aig_and_get(crosstalk_hdl_aig_ctx_t *ctx, uint32_t lhs_lit, uint32_t rhs_lit, uint32_t *out_lit)
{
    uint32_t swap_lit = 0;
    uint32_t hash_idx = 0;
    uint32_t node_idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_lit == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_lit = CROSSTALK_HDL_AIG_LIT_FALSE;
    if (lhs_lit == CROSSTALK_HDL_AIG_LIT_FALSE || rhs_lit == CROSSTALK_HDL_AIG_LIT_FALSE) { return LDG_ERR_AOK; }

    if (lhs_lit == CROSSTALK_HDL_AIG_LIT_TRUE) { *out_lit = rhs_lit; return LDG_ERR_AOK; }

    if (rhs_lit == CROSSTALK_HDL_AIG_LIT_TRUE) { *out_lit = lhs_lit; return LDG_ERR_AOK; }

    if (lhs_lit == rhs_lit) { *out_lit = lhs_lit; return LDG_ERR_AOK; }

    if (lhs_lit == crosstalk_hdl_aig_lit_not_get(rhs_lit)) { return LDG_ERR_AOK; }

    if (lhs_lit > rhs_lit)
    {
        swap_lit = lhs_lit;
        lhs_lit = rhs_lit;
        rhs_lit = swap_lit;
    }

    hash_idx = (lhs_lit * 0x9E3779B1 + rhs_lit * 0x85EBCA77) & (ctx->hash_cap - 0x1);
    while (ctx->hash_tbl[hash_idx] != UINT32_MAX)
    {
        node_idx = ctx->hash_tbl[hash_idx];
        if (ctx->nodes[node_idx].fanin0 == lhs_lit && ctx->nodes[node_idx].fanin1 == rhs_lit)
        {
            *out_lit = node_idx << 0x1;
            return LDG_ERR_AOK;
        }

        hash_idx = (hash_idx + 0x1) & (ctx->hash_cap - 0x1);
    }

    if (LDG_UNLIKELY(ctx->node_cunt >= ctx->node_cap)) { return CROSSTALK_HDL_ERR_AIG_CAP; }

    node_idx = ctx->node_cunt;
    ctx->nodes[node_idx].fanin0 = lhs_lit;
    ctx->nodes[node_idx].fanin1 = rhs_lit;
    ctx->node_cunt = ctx->node_cunt + 0x1;
    ctx->hash_tbl[hash_idx] = node_idx;
    *out_lit = node_idx << 0x1;
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_aig_or_get(crosstalk_hdl_aig_ctx_t *ctx, uint32_t lhs_lit, uint32_t rhs_lit, uint32_t *out_lit)
{
    uint32_t and_lit = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(out_lit == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_aig_and_get(ctx, crosstalk_hdl_aig_lit_not_get(lhs_lit), crosstalk_hdl_aig_lit_not_get(rhs_lit), &and_lit);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_lit = crosstalk_hdl_aig_lit_not_get(and_lit);
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_aig_xor_get(crosstalk_hdl_aig_ctx_t *ctx, uint32_t lhs_lit, uint32_t rhs_lit, uint32_t *out_lit)
{
    uint32_t hi_lit = 0;
    uint32_t lo_lit = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(out_lit == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_aig_and_get(ctx, lhs_lit, crosstalk_hdl_aig_lit_not_get(rhs_lit), &hi_lit);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_aig_and_get(ctx, crosstalk_hdl_aig_lit_not_get(lhs_lit), rhs_lit, &lo_lit);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_aig_or_get(ctx, hi_lit, lo_lit, out_lit);
}

static uint32_t crosstalk_hdl_aig_mux_get(crosstalk_hdl_aig_ctx_t *ctx, uint32_t sel_lit, uint32_t true_lit, uint32_t false_lit, uint32_t *out_lit)
{
    uint32_t hi_lit = 0;
    uint32_t lo_lit = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(out_lit == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_aig_and_get(ctx, sel_lit, true_lit, &hi_lit);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_aig_and_get(ctx, crosstalk_hdl_aig_lit_not_get(sel_lit), false_lit, &lo_lit);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_aig_or_get(ctx, hi_lit, lo_lit, out_lit);
}

static uint32_t crosstalk_hdl_aig_red_or_get(crosstalk_hdl_aig_ctx_t *ctx, const uint32_t *lits, uint32_t lit_cunt, uint32_t *out_lit)
{
    uint32_t acc_lit = CROSSTALK_HDL_AIG_LIT_FALSE;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(lits == 0x0 || out_lit == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < lit_cunt; idx = idx + 1)
    {
        ret = crosstalk_hdl_aig_or_get(ctx, acc_lit, lits[idx], &acc_lit);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    *out_lit = acc_lit;
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_aig_red_and_get(crosstalk_hdl_aig_ctx_t *ctx, const uint32_t *lits, uint32_t lit_cunt, uint32_t *out_lit)
{
    uint32_t acc_lit = CROSSTALK_HDL_AIG_LIT_TRUE;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(lits == 0x0 || out_lit == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < lit_cunt; idx = idx + 1)
    {
        ret = crosstalk_hdl_aig_and_get(ctx, acc_lit, lits[idx], &acc_lit);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    *out_lit = acc_lit;
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_aig_red_xor_get(crosstalk_hdl_aig_ctx_t *ctx, const uint32_t *lits, uint32_t lit_cunt, uint32_t *out_lit)
{
    uint32_t acc_lit = CROSSTALK_HDL_AIG_LIT_FALSE;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(lits == 0x0 || out_lit == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < lit_cunt; idx = idx + 1)
    {
        ret = crosstalk_hdl_aig_xor_get(ctx, acc_lit, lits[idx], &acc_lit);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    *out_lit = acc_lit;
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_aig_vec_rd(const crosstalk_hdl_aig_ctx_t *ctx, uint32_t sig_idx, uint32_t width, uint32_t *out_lits)
{
    const uint32_t *src_lits = 0x0;
    uint32_t dep_op = UINT32_MAX;
    uint32_t slot_idx = 0;
    uint32_t sig_w = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_lits == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < width; idx = idx + 1) { out_lits[idx] = CROSSTALK_HDL_AIG_LIT_FALSE; }

    if (sig_idx == UINT32_MAX) { return LDG_ERR_AOK; }

    if (ctx->lit_off[sig_idx] == UINT32_MAX) { return LDG_ERR_AOK; }

    sig_w = ctx->ir_ctx->sigs[sig_idx].width;
    src_lits = ctx->lit_arena + ctx->lit_off[sig_idx];
    if (ctx->comb_mode != LDG_TRUTH_FALSE) { for (slot_idx = 0; slot_idx < CROSSTALK_HDL_AIG_RD_SLOT_CUNT; slot_idx = slot_idx + 1)
        {
            if (ctx->rd_dep_sig[ctx->cur_op * CROSSTALK_HDL_AIG_RD_SLOT_CUNT + slot_idx] != sig_idx) { continue; }

            dep_op = ctx->rd_dep_op[ctx->cur_op * CROSSTALK_HDL_AIG_RD_SLOT_CUNT + slot_idx];
            if (dep_op != UINT32_MAX) { src_lits = ctx->op_out + (uint64_t)dep_op * CROSSTALK_HDL_AIG_WIDTH_MAX; }

            break;
        }
    }

    for (idx = 0; idx < width && idx < sig_w; idx = idx + 1) { out_lits[idx] = src_lits[idx]; }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_aig_vec_wr(crosstalk_hdl_aig_ctx_t *ctx, uint32_t sig_idx, const uint32_t *lits, uint32_t width)
{
    uint32_t *dst_lits = 0x0;
    uint32_t sig_w = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || lits == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (sig_idx == UINT32_MAX || ctx->lit_off[sig_idx] == UINT32_MAX) { return LDG_ERR_AOK; }

    sig_w = ctx->ir_ctx->sigs[sig_idx].width;
    dst_lits = ctx->comb_mode != LDG_TRUTH_FALSE ? ctx->op_out + (uint64_t)ctx->cur_op * CROSSTALK_HDL_AIG_WIDTH_MAX : ctx->lit_arena + ctx->lit_off[sig_idx];
    for (idx = 0; idx < sig_w; idx = idx + 1) { dst_lits[idx] = idx < width ? lits[idx] : CROSSTALK_HDL_AIG_LIT_FALSE; }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_aig_add_vec(crosstalk_hdl_aig_ctx_t *ctx, const uint32_t *lhs, const uint32_t *rhs, uint32_t width, uint32_t carry_lit, uint32_t *out_lits)
{
    uint32_t axb_lit = 0;
    uint32_t gen_lit = 0;
    uint32_t prop_lit = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(lhs == 0x0 || rhs == 0x0 || out_lits == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < width; idx = idx + 1)
    {
        ret = crosstalk_hdl_aig_xor_get(ctx, lhs[idx], rhs[idx], &axb_lit);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_aig_and_get(ctx, lhs[idx], rhs[idx], &gen_lit);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_aig_xor_get(ctx, axb_lit, carry_lit, &out_lits[idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_aig_and_get(ctx, axb_lit, carry_lit, &prop_lit);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_aig_or_get(ctx, gen_lit, prop_lit, &carry_lit);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_aig_sub_vec(crosstalk_hdl_aig_ctx_t *ctx, const uint32_t *lhs, const uint32_t *rhs, uint32_t width, uint32_t *out_lits)
{
    uint32_t rhs_not[CROSSTALK_HDL_AIG_WIDTH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(lhs == 0x0 || rhs == 0x0 || out_lits == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < width; idx = idx + 1) { rhs_not[idx] = crosstalk_hdl_aig_lit_not_get(rhs[idx]); }

    return crosstalk_hdl_aig_add_vec(ctx, lhs, rhs_not, width, CROSSTALK_HDL_AIG_LIT_TRUE, out_lits);
}

static uint32_t crosstalk_hdl_aig_lt_get(crosstalk_hdl_aig_ctx_t *ctx, const uint32_t *lhs, const uint32_t *rhs, uint32_t width, uint32_t *out_lit)
{
    uint32_t borrow_lit = CROSSTALK_HDL_AIG_LIT_FALSE;
    uint32_t diff_lit = 0;
    uint32_t low_lit = 0;
    uint32_t keep_lit = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(lhs == 0x0 || rhs == 0x0 || out_lit == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < width; idx = idx + 1)
    {
        ret = crosstalk_hdl_aig_and_get(ctx, crosstalk_hdl_aig_lit_not_get(lhs[idx]), rhs[idx], &low_lit);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_aig_xor_get(ctx, lhs[idx], rhs[idx], &diff_lit);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_aig_and_get(ctx, crosstalk_hdl_aig_lit_not_get(diff_lit), borrow_lit, &keep_lit);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_aig_or_get(ctx, low_lit, keep_lit, &borrow_lit);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    *out_lit = borrow_lit;
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_aig_eq_get(crosstalk_hdl_aig_ctx_t *ctx, const uint32_t *lhs, const uint32_t *rhs, uint32_t width, uint32_t *out_lit)
{
    uint32_t diff_lits[CROSSTALK_HDL_AIG_WIDTH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t any_lit = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(lhs == 0x0 || rhs == 0x0 || out_lit == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < width; idx = idx + 1)
    {
        ret = crosstalk_hdl_aig_xor_get(ctx, lhs[idx], rhs[idx], &diff_lits[idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_aig_red_or_get(ctx, diff_lits, width, &any_lit);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_lit = crosstalk_hdl_aig_lit_not_get(any_lit);
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_aig_mux_vec(crosstalk_hdl_aig_ctx_t *ctx, uint32_t sel_lit, const uint32_t *true_lits, const uint32_t *false_lits, uint32_t width, uint32_t *out_lits)
{
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(true_lits == 0x0 || false_lits == 0x0 || out_lits == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < width; idx = idx + 1)
    {
        ret = crosstalk_hdl_aig_mux_get(ctx, sel_lit, true_lits[idx], false_lits[idx], &out_lits[idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_aig_shift_hi_get(crosstalk_hdl_aig_ctx_t *ctx, const uint32_t *sh_lits, uint32_t sh_w, uint32_t *out_lit)
{
    if (LDG_UNLIKELY(sh_lits == 0x0 || out_lit == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_lit = CROSSTALK_HDL_AIG_LIT_FALSE;
    if (sh_w <= CROSSTALK_HDL_AIG_SHIFT_LAYER_CUNT) { return LDG_ERR_AOK; }

    return crosstalk_hdl_aig_red_or_get(ctx, sh_lits + CROSSTALK_HDL_AIG_SHIFT_LAYER_CUNT, sh_w - CROSSTALK_HDL_AIG_SHIFT_LAYER_CUNT, out_lit);
}

static uint32_t crosstalk_hdl_aig_shr_vec(crosstalk_hdl_aig_ctx_t *ctx, const uint32_t *in_lits, uint32_t width, const uint32_t *sh_lits, uint32_t sh_w, uint32_t fill_lit, uint32_t *out_lits)
{
    uint32_t current[CROSSTALK_HDL_AIG_WIDTH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t next[CROSSTALK_HDL_AIG_WIDTH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t fill_vec[CROSSTALK_HDL_AIG_WIDTH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t hi_lit = 0;
    uint32_t stride = 0;
    uint32_t layer_idx = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(in_lits == 0x0 || sh_lits == 0x0 || out_lits == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < width; idx = idx + 1) { current[idx] = in_lits[idx]; }

    for (layer_idx = 0; layer_idx < CROSSTALK_HDL_AIG_SHIFT_LAYER_CUNT && layer_idx < sh_w; layer_idx = layer_idx + 1)
    {
        stride = (uint32_t)0x1 << layer_idx;
        for (idx = 0; idx < width; idx = idx + 1)
        {
            ret = crosstalk_hdl_aig_mux_get(ctx, sh_lits[layer_idx], idx + stride < width ? current[idx + stride] : fill_lit, current[idx], &next[idx]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        for (idx = 0; idx < width; idx = idx + 1) { current[idx] = next[idx]; }
    }

    ret = crosstalk_hdl_aig_shift_hi_get(ctx, sh_lits, sh_w, &hi_lit);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < width; idx = idx + 1) { fill_vec[idx] = fill_lit; }

    return crosstalk_hdl_aig_mux_vec(ctx, hi_lit, fill_vec, current, width, out_lits);
}

static uint32_t crosstalk_hdl_aig_shl_vec(crosstalk_hdl_aig_ctx_t *ctx, const uint32_t *in_lits, uint32_t width, const uint32_t *sh_lits, uint32_t sh_w, uint32_t *out_lits)
{
    uint32_t current[CROSSTALK_HDL_AIG_WIDTH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t next[CROSSTALK_HDL_AIG_WIDTH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t zero_vec[CROSSTALK_HDL_AIG_WIDTH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t hi_lit = 0;
    uint32_t stride = 0;
    uint32_t layer_idx = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(in_lits == 0x0 || sh_lits == 0x0 || out_lits == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < width; idx = idx + 1) { current[idx] = in_lits[idx]; }

    for (layer_idx = 0; layer_idx < CROSSTALK_HDL_AIG_SHIFT_LAYER_CUNT && layer_idx < sh_w; layer_idx = layer_idx + 1)
    {
        stride = (uint32_t)0x1 << layer_idx;
        for (idx = 0; idx < width; idx = idx + 1)
        {
            ret = crosstalk_hdl_aig_mux_get(ctx, sh_lits[layer_idx], idx >= stride ? current[idx - stride] : CROSSTALK_HDL_AIG_LIT_FALSE, current[idx], &next[idx]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        for (idx = 0; idx < width; idx = idx + 1) { current[idx] = next[idx]; }
    }

    ret = crosstalk_hdl_aig_shift_hi_get(ctx, sh_lits, sh_w, &hi_lit);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_aig_mux_vec(ctx, hi_lit, zero_vec, current, width, out_lits);
}

static uint32_t crosstalk_hdl_aig_mul_vec(crosstalk_hdl_aig_ctx_t *ctx, const uint32_t *lhs, const uint32_t *rhs, uint32_t width, uint32_t *out_lits)
{
    uint32_t acc[CROSSTALK_HDL_AIG_WIDTH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t addend[CROSSTALK_HDL_AIG_WIDTH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t bit_idx = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(lhs == 0x0 || rhs == 0x0 || out_lits == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (bit_idx = 0; bit_idx < width; bit_idx = bit_idx + 1)
    {
        for (idx = 0; idx < width; idx = idx + 1)
        {
            addend[idx] = CROSSTALK_HDL_AIG_LIT_FALSE;
            if (idx >= bit_idx)
            {
                ret = crosstalk_hdl_aig_and_get(ctx, lhs[idx - bit_idx], rhs[bit_idx], &addend[idx]);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
        }

        ret = crosstalk_hdl_aig_add_vec(ctx, acc, addend, width, CROSSTALK_HDL_AIG_LIT_FALSE, acc);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (idx = 0; idx < width; idx = idx + 1) { out_lits[idx] = acc[idx]; }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_aig_divmod_vec(crosstalk_hdl_aig_ctx_t *ctx, const uint32_t *lhs, const uint32_t *rhs, uint32_t width, uint32_t *out_quo, uint32_t *out_rem)
{
    uint32_t rem[CROSSTALK_HDL_AIG_WIDTH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t diff[CROSSTALK_HDL_AIG_WIDTH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t zero_vec[CROSSTALK_HDL_AIG_WIDTH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t lt_lit = 0;
    uint32_t ge_lit = 0;
    uint32_t any_lit = 0;
    uint32_t zero_lit = 0;
    uint32_t quo_idx = 0;
    uint32_t bit_idx = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(lhs == 0x0 || rhs == 0x0 || out_quo == 0x0 || out_rem == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (bit_idx = width; bit_idx > 0; bit_idx = bit_idx - 1)
    {
        quo_idx = bit_idx - 0x1;
        for (idx = width - 0x1; idx > 0; idx = idx - 1) { rem[idx] = rem[idx - 0x1]; }

        rem[0] = lhs[quo_idx];
        ret = crosstalk_hdl_aig_lt_get(ctx, rem, rhs, width, &lt_lit);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ge_lit = crosstalk_hdl_aig_lit_not_get(lt_lit);
        ret = crosstalk_hdl_aig_sub_vec(ctx, rem, rhs, width, diff);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_aig_mux_vec(ctx, ge_lit, diff, rem, width, rem);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        out_quo[quo_idx] = ge_lit;
    }

    ret = crosstalk_hdl_aig_red_or_get(ctx, rhs, width, &any_lit);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    zero_lit = crosstalk_hdl_aig_lit_not_get(any_lit);
    ret = crosstalk_hdl_aig_mux_vec(ctx, zero_lit, zero_vec, out_quo, width, out_quo);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_aig_mux_vec(ctx, zero_lit, zero_vec, rem, width, out_rem);
}

static uint32_t crosstalk_hdl_aig_root_add(crosstalk_hdl_aig_ctx_t *ctx, const uint32_t *lits, uint32_t lit_cunt)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || lits == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->root_cunt + lit_cunt > ctx->root_cap)) { return CROSSTALK_HDL_ERR_AIG_CAP; }

    for (idx = 0; idx < lit_cunt; idx = idx + 1)
    {
        ctx->roots[ctx->root_cunt] = lits[idx];
        ctx->root_cunt = ctx->root_cunt + 0x1;
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_aig_op_proc(crosstalk_hdl_aig_ctx_t *ctx, const crosstalk_hdl_ir_op_t *op)
{
    uint32_t src0_lits[CROSSTALK_HDL_AIG_WIDTH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t src1_lits[CROSSTALK_HDL_AIG_WIDTH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t src2_lits[CROSSTALK_HDL_AIG_WIDTH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t res_lits[CROSSTALK_HDL_AIG_WIDTH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t aux_lits[CROSSTALK_HDL_AIG_WIDTH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t dst_w = 0;
    uint32_t src0_w = 0;
    uint32_t src1_w = 0;
    uint32_t src2_w = 0;
    uint32_t work_w = 0;
    uint32_t msb_idx = 0;
    uint32_t bit_lit = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || op == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (op->src0 != UINT32_MAX) { src0_w = ctx->ir_ctx->sigs[op->src0].width; }

    if (op->src1 != UINT32_MAX) { src1_w = ctx->ir_ctx->sigs[op->src1].width; }

    if (op->src2 != UINT32_MAX) { src2_w = ctx->ir_ctx->sigs[op->src2].width; }

    if (op->kind == CROSSTALK_HDL_IR_OP_MEM_WR)
    {
        uint32_t en_lit = CROSSTALK_HDL_AIG_LIT_TRUE;

        if (op->dst != UINT32_MAX)
        {
            ret = crosstalk_hdl_aig_vec_rd(ctx, op->dst, ctx->ir_ctx->sigs[op->dst].width, src0_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_red_or_get(ctx, src0_lits, ctx->ir_ctx->sigs[op->dst].width, &en_lit);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ctx->op_root_off[ctx->cur_op] = ctx->root_cunt;
        ret = crosstalk_hdl_aig_root_add(ctx, &en_lit, 0x1);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_aig_vec_rd(ctx, op->src1, src1_w, src1_lits);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_aig_root_add(ctx, src1_lits, src1_w);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_aig_vec_rd(ctx, op->src2, src2_w, src2_lits);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_aig_root_add(ctx, src2_lits, src2_w);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ctx->mem_wr_cunt = ctx->mem_wr_cunt + 0x1;
        return LDG_ERR_AOK;
    }

    if (op->kind == CROSSTALK_HDL_IR_OP_REG_NEXT)
    {
        if (LDG_UNLIKELY(op->dst == UINT32_MAX || ctx->ff_off[op->dst] == UINT32_MAX)) { return LDG_ERR_INVALID; }

        dst_w = ctx->ir_ctx->sigs[op->dst].width;
        ret = crosstalk_hdl_aig_vec_rd(ctx, op->src0, dst_w, src0_lits);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        for (idx = 0; idx < dst_w; idx = idx + 1) { ctx->ff_arena[ctx->ff_off[op->dst] + idx] = src0_lits[idx]; }

        return LDG_ERR_AOK;
    }

    if (op->kind == CROSSTALK_HDL_IR_OP_MEM_RD)
    {
        dst_w = ctx->ir_ctx->sigs[op->dst].width;
        ret = crosstalk_hdl_aig_vec_rd(ctx, op->src1, src1_w, src1_lits);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ctx->op_root_off[ctx->cur_op] = ctx->root_cunt;
        ret = crosstalk_hdl_aig_root_add(ctx, src1_lits, src1_w);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ctx->op_pi_base[ctx->cur_op] = ctx->pi_cunt;
        for (idx = 0; idx < dst_w; idx = idx + 1)
        {
            ret = crosstalk_hdl_aig_pi_get(ctx, &res_lits[idx]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_pi_note(ctx, CROSSTALK_HDL_AIG_PI_KIND_MEM_RD, op->dst, idx, ctx->cur_op, res_lits[idx]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ctx->mem_rd_cunt = ctx->mem_rd_cunt + 0x1;
        return crosstalk_hdl_aig_vec_wr(ctx, op->dst, res_lits, dst_w);
    }

    dst_w = ctx->ir_ctx->sigs[op->dst].width;
    switch (op->kind)
    {
        case CROSSTALK_HDL_IR_OP_CONST:
        {
            for (idx = 0; idx < dst_w; idx = idx + 1) { res_lits[idx] = ((op->imm >> idx) & 0x1) != 0 ? CROSSTALK_HDL_AIG_LIT_TRUE : CROSSTALK_HDL_AIG_LIT_FALSE; }

            break;
        }
        case CROSSTALK_HDL_IR_OP_COPY:
        case CROSSTALK_HDL_IR_OP_ZEXT:
        {
            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src0, dst_w, res_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            break;
        }
        case CROSSTALK_HDL_IR_OP_SEXT:
        {
            uint32_t src_w = (uint32_t)op->imm;

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src0, dst_w, res_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (src_w > 0)
            {
                msb_idx = src_w - 0x1;
                for (idx = src_w; idx < dst_w; idx = idx + 1) { res_lits[idx] = res_lits[msb_idx]; }
            }

            break;
        }
        case CROSSTALK_HDL_IR_OP_AND:
        case CROSSTALK_HDL_IR_OP_OR:
        case CROSSTALK_HDL_IR_OP_XOR:
        case CROSSTALK_HDL_IR_OP_XNOR:
        {
            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src0, dst_w, src0_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src1, dst_w, src1_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            for (idx = 0; idx < dst_w; idx = idx + 1)
            {
                if (op->kind == CROSSTALK_HDL_IR_OP_AND) { ret = crosstalk_hdl_aig_and_get(ctx, src0_lits[idx], src1_lits[idx], &res_lits[idx]); }
                else if (op->kind == CROSSTALK_HDL_IR_OP_OR) { ret = crosstalk_hdl_aig_or_get(ctx, src0_lits[idx], src1_lits[idx], &res_lits[idx]); }
                else { ret = crosstalk_hdl_aig_xor_get(ctx, src0_lits[idx], src1_lits[idx], &res_lits[idx]); }

                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                if (op->kind == CROSSTALK_HDL_IR_OP_XNOR) { res_lits[idx] = crosstalk_hdl_aig_lit_not_get(res_lits[idx]); }
            }

            break;
        }
        case CROSSTALK_HDL_IR_OP_NOT:
        {
            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src0, dst_w, res_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            for (idx = 0; idx < dst_w; idx = idx + 1) { res_lits[idx] = crosstalk_hdl_aig_lit_not_get(res_lits[idx]); }

            break;
        }
        case CROSSTALK_HDL_IR_OP_ADD:
        case CROSSTALK_HDL_IR_OP_SUB:
        {
            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src0, dst_w, src0_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src1, dst_w, src1_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (op->kind == CROSSTALK_HDL_IR_OP_ADD) { ret = crosstalk_hdl_aig_add_vec(ctx, src0_lits, src1_lits, dst_w, CROSSTALK_HDL_AIG_LIT_FALSE, res_lits); }
            else { ret = crosstalk_hdl_aig_sub_vec(ctx, src0_lits, src1_lits, dst_w, res_lits); }

            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            break;
        }
        case CROSSTALK_HDL_IR_OP_NEG:
        {
            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src0, dst_w, src0_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_sub_vec(ctx, res_lits, src0_lits, dst_w, res_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            break;
        }
        case CROSSTALK_HDL_IR_OP_MUL:
        {
            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src0, dst_w, src0_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src1, dst_w, src1_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_mul_vec(ctx, src0_lits, src1_lits, dst_w, res_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            break;
        }
        case CROSSTALK_HDL_IR_OP_DIV:
        case CROSSTALK_HDL_IR_OP_MOD:
        {
            work_w = src0_w > src1_w ? src0_w : src1_w;
            if (work_w == 0) { work_w = dst_w; }

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src0, work_w, src0_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src1, work_w, src1_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_divmod_vec(ctx, src0_lits, src1_lits, work_w, src2_lits, aux_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            for (idx = 0; idx < dst_w; idx = idx + 1) { if (op->kind == CROSSTALK_HDL_IR_OP_DIV) { res_lits[idx] = idx < work_w ? src2_lits[idx] : CROSSTALK_HDL_AIG_LIT_FALSE; }
                else { res_lits[idx] = idx < work_w ? aux_lits[idx] : CROSSTALK_HDL_AIG_LIT_FALSE; } }

            break;
        }
        case CROSSTALK_HDL_IR_OP_SHL:
        {
            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src0, dst_w, src0_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src1, src1_w, src1_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_shl_vec(ctx, src0_lits, dst_w, src1_lits, src1_w, res_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            break;
        }
        case CROSSTALK_HDL_IR_OP_SHR:
        {
            work_w = src0_w > dst_w ? src0_w : dst_w;
            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src0, work_w, src0_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src1, src1_w, src1_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_shr_vec(ctx, src0_lits, work_w, src1_lits, src1_w, CROSSTALK_HDL_AIG_LIT_FALSE, aux_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            for (idx = 0; idx < dst_w; idx = idx + 1) { res_lits[idx] = idx < work_w ? aux_lits[idx] : CROSSTALK_HDL_AIG_LIT_FALSE; }

            break;
        }
        case CROSSTALK_HDL_IR_OP_ASHR:
        {
            uint32_t sign_lit = 0;

            work_w = op->width > dst_w ? op->width : dst_w;
            if (work_w == 0 || work_w > CROSSTALK_HDL_AIG_WIDTH_MAX) { work_w = dst_w; }

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src0, work_w, src0_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            sign_lit = CROSSTALK_HDL_AIG_LIT_FALSE;
            if (op->width > 0 && op->width <= work_w)
            {
                msb_idx = op->width - 0x1;
                sign_lit = src0_lits[msb_idx];
            }

            for (idx = op->width; idx < work_w; idx = idx + 1) { src0_lits[idx] = sign_lit; }

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src1, src1_w, src1_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_shr_vec(ctx, src0_lits, work_w, src1_lits, src1_w, sign_lit, aux_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            for (idx = 0; idx < dst_w; idx = idx + 1) { res_lits[idx] = idx < work_w ? aux_lits[idx] : sign_lit; }

            break;
        }
        case CROSSTALK_HDL_IR_OP_EQ:
        case CROSSTALK_HDL_IR_OP_NEQ:
        case CROSSTALK_HDL_IR_OP_LT:
        case CROSSTALK_HDL_IR_OP_LE:
        case CROSSTALK_HDL_IR_OP_GT:
        case CROSSTALK_HDL_IR_OP_GE:
        {
            work_w = src0_w > src1_w ? src0_w : src1_w;
            if (work_w == 0) { work_w = 0x1; }

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src0, work_w, src0_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src1, work_w, src1_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (op->kind == CROSSTALK_HDL_IR_OP_EQ || op->kind == CROSSTALK_HDL_IR_OP_NEQ)
            {
                ret = crosstalk_hdl_aig_eq_get(ctx, src0_lits, src1_lits, work_w, &bit_lit);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                if (op->kind == CROSSTALK_HDL_IR_OP_NEQ) { bit_lit = crosstalk_hdl_aig_lit_not_get(bit_lit); }
            }
            else
            {
                if (op->kind == CROSSTALK_HDL_IR_OP_LT || op->kind == CROSSTALK_HDL_IR_OP_GE) { ret = crosstalk_hdl_aig_lt_get(ctx, src0_lits, src1_lits, work_w, &bit_lit); }
                else { ret = crosstalk_hdl_aig_lt_get(ctx, src1_lits, src0_lits, work_w, &bit_lit); }

                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                if (op->kind == CROSSTALK_HDL_IR_OP_GE || op->kind == CROSSTALK_HDL_IR_OP_LE) { bit_lit = crosstalk_hdl_aig_lit_not_get(bit_lit); }
            }

            res_lits[0] = bit_lit;
            break;
        }
        case CROSSTALK_HDL_IR_OP_LT_S:
        case CROSSTALK_HDL_IR_OP_LE_S:
        case CROSSTALK_HDL_IR_OP_GT_S:
        case CROSSTALK_HDL_IR_OP_GE_S:
        {
            work_w = op->width;
            if (work_w == 0 || work_w > CROSSTALK_HDL_AIG_WIDTH_MAX) { work_w = 0x1; }

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src0, work_w, src0_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src1, work_w, src1_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            msb_idx = work_w - 0x1;
            src0_lits[msb_idx] = crosstalk_hdl_aig_lit_not_get(src0_lits[msb_idx]);
            src1_lits[msb_idx] = crosstalk_hdl_aig_lit_not_get(src1_lits[msb_idx]);
            if (op->kind == CROSSTALK_HDL_IR_OP_LT_S || op->kind == CROSSTALK_HDL_IR_OP_GE_S) { ret = crosstalk_hdl_aig_lt_get(ctx, src0_lits, src1_lits, work_w, &bit_lit); }
            else { ret = crosstalk_hdl_aig_lt_get(ctx, src1_lits, src0_lits, work_w, &bit_lit); }

            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (op->kind == CROSSTALK_HDL_IR_OP_GE_S || op->kind == CROSSTALK_HDL_IR_OP_LE_S) { bit_lit = crosstalk_hdl_aig_lit_not_get(bit_lit); }

            res_lits[0] = bit_lit;
            break;
        }
        case CROSSTALK_HDL_IR_OP_LAND:
        case CROSSTALK_HDL_IR_OP_LOR:
        {
            uint32_t lhs_any = 0;
            uint32_t rhs_any = 0;

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src0, src0_w, src0_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_red_or_get(ctx, src0_lits, src0_w, &lhs_any);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src1, src1_w, src1_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_red_or_get(ctx, src1_lits, src1_w, &rhs_any);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (op->kind == CROSSTALK_HDL_IR_OP_LAND) { ret = crosstalk_hdl_aig_and_get(ctx, lhs_any, rhs_any, &res_lits[0]); }
            else { ret = crosstalk_hdl_aig_or_get(ctx, lhs_any, rhs_any, &res_lits[0]); }

            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            break;
        }
        case CROSSTALK_HDL_IR_OP_LNOT:
        {
            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src0, src0_w, src0_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_red_or_get(ctx, src0_lits, src0_w, &bit_lit);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            res_lits[0] = crosstalk_hdl_aig_lit_not_get(bit_lit);
            break;
        }
        case CROSSTALK_HDL_IR_OP_RED_AND:
        {
            uint32_t red_w = (uint32_t)op->imm;

            if (red_w == 0 || red_w > CROSSTALK_HDL_AIG_WIDTH_MAX) { red_w = src0_w; }

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src0, red_w, src0_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_red_and_get(ctx, src0_lits, red_w, &res_lits[0]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            break;
        }
        case CROSSTALK_HDL_IR_OP_RED_OR:
        {
            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src0, src0_w, src0_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_red_or_get(ctx, src0_lits, src0_w, &res_lits[0]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            break;
        }
        case CROSSTALK_HDL_IR_OP_RED_XOR:
        {
            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src0, src0_w, src0_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_red_xor_get(ctx, src0_lits, src0_w, &res_lits[0]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            break;
        }
        case CROSSTALK_HDL_IR_OP_CONCAT:
        {
            uint32_t low_w = (uint32_t)op->imm;
            uint32_t hi_w = dst_w > low_w ? dst_w - low_w : 0x0;

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src1, low_w > dst_w ? dst_w : low_w, src1_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src0, hi_w, src0_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            for (idx = 0; idx < dst_w; idx = idx + 1) { res_lits[idx] = idx < low_w ? src1_lits[idx] : src0_lits[idx - low_w]; }

            break;
        }
        case CROSSTALK_HDL_IR_OP_SLICE:
        {
            uint32_t lo_bit = (uint32_t)op->imm;

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src0, src0_w, src0_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            for (idx = 0; idx < dst_w; idx = idx + 1)
            {
                res_lits[idx] = CROSSTALK_HDL_AIG_LIT_FALSE;
                if (idx < op->width && lo_bit + idx < src0_w) { res_lits[idx] = src0_lits[lo_bit + idx]; }
            }

            break;
        }
        case CROSSTALK_HDL_IR_OP_SLICE_DYN:
        {
            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src0, src0_w, src0_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src1, src1_w, src1_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_shr_vec(ctx, src0_lits, src0_w, src1_lits, src1_w, CROSSTALK_HDL_AIG_LIT_FALSE, aux_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            for (idx = 0; idx < dst_w; idx = idx + 1)
            {
                res_lits[idx] = CROSSTALK_HDL_AIG_LIT_FALSE;
                if (idx < op->width && idx < src0_w) { res_lits[idx] = aux_lits[idx]; }
            }

            break;
        }
        case CROSSTALK_HDL_IR_OP_INSERT:
        {
            uint32_t lo_bit = (uint32_t)op->imm;

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src0, dst_w, src0_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src1, op->width > CROSSTALK_HDL_AIG_WIDTH_MAX ? CROSSTALK_HDL_AIG_WIDTH_MAX : op->width, src1_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            for (idx = 0; idx < dst_w; idx = idx + 1)
            {
                res_lits[idx] = src0_lits[idx];
                if (idx >= lo_bit && idx < lo_bit + op->width) { res_lits[idx] = src1_lits[idx - lo_bit]; }
            }

            break;
        }
        case CROSSTALK_HDL_IR_OP_INSERT_DYN:
        {
            uint32_t mask_lits[CROSSTALK_HDL_AIG_WIDTH_MAX] = LDG_ARR_ZERO_INIT;
            uint32_t field_lits[CROSSTALK_HDL_AIG_WIDTH_MAX] = LDG_ARR_ZERO_INIT;
            uint32_t mix_lit = 0;

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src0, dst_w, src0_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src1, op->width > dst_w ? dst_w : op->width, src1_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src2, src2_w, src2_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            for (idx = 0; idx < dst_w; idx = idx + 1) { aux_lits[idx] = idx < op->width ? CROSSTALK_HDL_AIG_LIT_TRUE : CROSSTALK_HDL_AIG_LIT_FALSE; }

            ret = crosstalk_hdl_aig_shl_vec(ctx, aux_lits, dst_w, src2_lits, src2_w, mask_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_shl_vec(ctx, src1_lits, dst_w, src2_lits, src2_w, field_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            for (idx = 0; idx < dst_w; idx = idx + 1)
            {
                ret = crosstalk_hdl_aig_mux_get(ctx, mask_lits[idx], field_lits[idx], src0_lits[idx], &mix_lit);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                res_lits[idx] = mix_lit;
            }

            break;
        }
        case CROSSTALK_HDL_IR_OP_MUX:
        {
            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src0, src0_w, src0_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_red_or_get(ctx, src0_lits, src0_w, &bit_lit);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src1, dst_w, src1_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_vec_rd(ctx, op->src2, dst_w, src2_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_aig_mux_vec(ctx, bit_lit, src1_lits, src2_lits, dst_w, res_lits);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            break;
        }
        default:
            return LDG_ERR_UNSUPPORTED;
    }

    return crosstalk_hdl_aig_vec_wr(ctx, op->dst, res_lits, dst_w);
}

static uint32_t crosstalk_hdl_aig_comb_place(crosstalk_hdl_aig_ctx_t *ctx)
{
    const crosstalk_hdl_ir_t *ir_ctx = 0x0;
    const crosstalk_hdl_ir_op_t *op = 0x0;
    uint32_t *seen_wr = 0x0;
    uint32_t rd_sigs[3] = LDG_ARR_ZERO_INIT;
    uint32_t rd_cunt = 0;
    uint32_t placed_cunt = 0;
    uint32_t progress = 0;
    uint32_t ready = 0;
    uint32_t op_idx = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ir_ctx = ctx->ir_ctx;
    seen_wr = ctx->op_order;
    for (idx = 0; idx < ir_ctx->sig_cunt; idx = idx + 1)
    {
        ctx->last_wr[idx] = UINT32_MAX;
        seen_wr[idx] = UINT32_MAX;
    }

    for (op_idx = 0; op_idx < ir_ctx->comb_cunt; op_idx = op_idx + 1)
    {
        op = &ir_ctx->comb_ops[op_idx];
        if (op->kind == CROSSTALK_HDL_IR_OP_MEM_WR || op->dst == UINT32_MAX) { continue; }

        ctx->last_wr[op->dst] = op_idx;
    }

    for (op_idx = 0; op_idx < ir_ctx->comb_cunt; op_idx = op_idx + 1)
    {
        op = &ir_ctx->comb_ops[op_idx];
        rd_cunt = 0;
        if (op->kind == CROSSTALK_HDL_IR_OP_MEM_WR)
        {
            if (op->dst != UINT32_MAX) { rd_sigs[rd_cunt] = op->dst; rd_cunt = rd_cunt + 0x1; }

            if (op->src1 != UINT32_MAX) { rd_sigs[rd_cunt] = op->src1; rd_cunt = rd_cunt + 0x1; }

            if (op->src2 != UINT32_MAX) { rd_sigs[rd_cunt] = op->src2; rd_cunt = rd_cunt + 0x1; }
        }
        else if (op->kind == CROSSTALK_HDL_IR_OP_MEM_RD) { if (op->src1 != UINT32_MAX) { rd_sigs[rd_cunt] = op->src1; rd_cunt = rd_cunt + 0x1; } }
        else
        {
            if (op->src0 != UINT32_MAX) { rd_sigs[rd_cunt] = op->src0; rd_cunt = rd_cunt + 0x1; }

            if (op->src1 != UINT32_MAX) { rd_sigs[rd_cunt] = op->src1; rd_cunt = rd_cunt + 0x1; }

            if (op->src2 != UINT32_MAX) { rd_sigs[rd_cunt] = op->src2; rd_cunt = rd_cunt + 0x1; }
        }

        for (idx = 0; idx < CROSSTALK_HDL_AIG_RD_SLOT_CUNT; idx = idx + 1)
        {
            ctx->rd_dep_sig[op_idx * CROSSTALK_HDL_AIG_RD_SLOT_CUNT + idx] = UINT32_MAX;
            ctx->rd_dep_op[op_idx * CROSSTALK_HDL_AIG_RD_SLOT_CUNT + idx] = UINT32_MAX;
        }

        for (idx = 0; idx < rd_cunt; idx = idx + 1)
        {
            ctx->rd_dep_sig[op_idx * CROSSTALK_HDL_AIG_RD_SLOT_CUNT + idx] = rd_sigs[idx];
            ctx->rd_dep_op[op_idx * CROSSTALK_HDL_AIG_RD_SLOT_CUNT + idx] = seen_wr[rd_sigs[idx]] != UINT32_MAX ? seen_wr[rd_sigs[idx]] : ctx->last_wr[rd_sigs[idx]];
        }

        if (op->kind != CROSSTALK_HDL_IR_OP_MEM_WR && op->dst != UINT32_MAX) { seen_wr[op->dst] = op_idx; }
    }

    progress = 0x1;
    while (progress != 0)
    {
        progress = 0;
        for (op_idx = 0; op_idx < ir_ctx->comb_cunt; op_idx = op_idx + 1)
        {
            if (ctx->op_placed[op_idx] != LDG_TRUTH_FALSE) { continue; }

            op = &ir_ctx->comb_ops[op_idx];
            ready = 0x1;
            for (idx = 0; idx < CROSSTALK_HDL_AIG_RD_SLOT_CUNT && ready != 0; idx = idx + 1)
            {
                uint32_t dep_op = ctx->rd_dep_op[op_idx * CROSSTALK_HDL_AIG_RD_SLOT_CUNT + idx];

                if (dep_op != UINT32_MAX && ctx->op_placed[dep_op] == LDG_TRUTH_FALSE) { ready = 0; }
            }

            if (ready == 0) { continue; }

            ctx->cur_op = op_idx;
            ret = crosstalk_hdl_aig_op_proc(ctx, op);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ctx->op_placed[op_idx] = LDG_TRUTH_TRUE;
            placed_cunt = placed_cunt + 0x1;
            progress = 0x1;
        }

        if (progress == 0 && placed_cunt < ir_ctx->comb_cunt)
        {
            uint32_t latch_sig = UINT32_MAX;
            uint32_t slot_sig = 0;
            uint32_t slot_dep = 0;
            uint32_t bit_idx = 0;
            uint32_t slot_idx = 0;

            for (op_idx = 0; op_idx < ir_ctx->comb_cunt && latch_sig == UINT32_MAX; op_idx = op_idx + 1)
            {
                if (ctx->op_placed[op_idx] != LDG_TRUTH_FALSE) { continue; }

                for (slot_idx = 0; slot_idx < CROSSTALK_HDL_AIG_RD_SLOT_CUNT && latch_sig == UINT32_MAX; slot_idx = slot_idx + 1)
                {
                    slot_sig = ctx->rd_dep_sig[op_idx * CROSSTALK_HDL_AIG_RD_SLOT_CUNT + slot_idx];
                    slot_dep = ctx->rd_dep_op[op_idx * CROSSTALK_HDL_AIG_RD_SLOT_CUNT + slot_idx];
                    if (slot_sig == UINT32_MAX || slot_dep == UINT32_MAX) { continue; }

                    if (ctx->op_placed[slot_dep] != LDG_TRUTH_FALSE) { continue; }

                    if (op_idx <= slot_dep && slot_dep == ctx->last_wr[slot_sig]) { latch_sig = slot_sig; }
                }
            }

            if (latch_sig == UINT32_MAX) { break; }

            if (ctx->lit_off[latch_sig] != UINT32_MAX) { for (bit_idx = 0; bit_idx < ir_ctx->sigs[latch_sig].width; bit_idx = bit_idx + 0x1)
                {
                    ret = crosstalk_hdl_aig_pi_get(ctx, &ctx->lit_arena[ctx->lit_off[latch_sig] + bit_idx]);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    ret = crosstalk_hdl_aig_pi_note(ctx, CROSSTALK_HDL_AIG_PI_KIND_LATCH, latch_sig, bit_idx, UINT32_MAX, ctx->lit_arena[ctx->lit_off[latch_sig] + bit_idx]);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
                }
            }

            for (op_idx = 0; op_idx < ir_ctx->comb_cunt; op_idx = op_idx + 1) { for (slot_idx = 0; slot_idx < CROSSTALK_HDL_AIG_RD_SLOT_CUNT; slot_idx = slot_idx + 1)
                {
                    slot_sig = ctx->rd_dep_sig[op_idx * CROSSTALK_HDL_AIG_RD_SLOT_CUNT + slot_idx];
                    slot_dep = ctx->rd_dep_op[op_idx * CROSSTALK_HDL_AIG_RD_SLOT_CUNT + slot_idx];
                    if (slot_sig != latch_sig || slot_dep != ctx->last_wr[latch_sig]) { continue; }

                    if (op_idx <= slot_dep) { ctx->rd_dep_op[op_idx * CROSSTALK_HDL_AIG_RD_SLOT_CUNT + slot_idx] = UINT32_MAX; }
                }
            }

            ctx->latch_is[latch_sig] = LDG_TRUTH_TRUE;
            ctx->latch_bits = ctx->latch_bits + ir_ctx->sigs[latch_sig].width;
            progress = 0x1;
        }
    }

    ctx->latch_root_base = ctx->root_cunt;
    for (idx = 0; idx < ir_ctx->sig_cunt; idx = idx + 1)
    {
        if (ctx->latch_is[idx] == LDG_TRUTH_FALSE || ctx->last_wr[idx] == UINT32_MAX || ctx->op_placed[ctx->last_wr[idx]] == LDG_TRUTH_FALSE) { continue; }

        ret = crosstalk_hdl_aig_root_add(ctx, ctx->op_out + (uint64_t)ctx->last_wr[idx] * CROSSTALK_HDL_AIG_WIDTH_MAX, ir_ctx->sigs[idx].width);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (idx = 0; idx < ir_ctx->sig_cunt; idx = idx + 1)
    {
        uint32_t fin_op = ctx->last_wr[idx];
        uint32_t sig_w = ir_ctx->sigs[idx].width;
        uint32_t bit_idx = 0;

        if (fin_op == UINT32_MAX || ctx->op_placed[fin_op] == LDG_TRUTH_FALSE || ctx->lit_off[idx] == UINT32_MAX) { continue; }

        for (bit_idx = 0; bit_idx < sig_w; bit_idx = bit_idx + 1) { ctx->lit_arena[ctx->lit_off[idx] + bit_idx] = ctx->op_out[(uint64_t)fin_op * CROSSTALK_HDL_AIG_WIDTH_MAX + bit_idx]; }
    }

    ctx->residue_cunt = ir_ctx->comb_cunt - placed_cunt;
    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_aig_build(crosstalk_hdl_aig_ctx_t *ctx, const crosstalk_hdl_ir_t *ir_ctx, void *pool, uint32_t node_cap)
{
    const crosstalk_hdl_ir_sig_t *sig = 0x0;
    const crosstalk_hdl_ir_op_t *op = 0x0;
    uint64_t lit_total = 0;
    uint64_t ff_total = 0;
    uint64_t root_total = 0;
    uint32_t mem_op_total = 0;
    uint32_t blk_idx = 0;
    uint32_t op_idx = 0;
    uint32_t bit_idx = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || ir_ctx == 0x0 || pool == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));
    ctx->ir_ctx = ir_ctx;
    ctx->node_cap = node_cap != 0 ? node_cap : CROSSTALK_HDL_AIG_NODE_CAP_DEFAULT;
    ctx->hash_cap = 0x1;
    while (ctx->hash_cap < ctx->node_cap * 0x2) { ctx->hash_cap = ctx->hash_cap << 0x1; }

    ret = crosstalk_hdl_aig_arr_alloc(pool, ctx->node_cap, sizeof(crosstalk_hdl_aig_node_t), (void **)&ctx->nodes);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_aig_arr_alloc(pool, ctx->hash_cap, sizeof(uint32_t), (void **)&ctx->hash_tbl);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    memset(ctx->hash_tbl, CROSSTALK_HDL_AIG_HASH_BYTE_FILL, (uint64_t)ctx->hash_cap * sizeof(uint32_t));

    ret = crosstalk_hdl_aig_arr_alloc(pool, ir_ctx->sig_cunt, sizeof(uint32_t), (void **)&ctx->lit_off);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_aig_arr_alloc(pool, ir_ctx->sig_cunt, sizeof(uint32_t), (void **)&ctx->ff_off);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_aig_arr_alloc(pool, ir_ctx->sig_cunt, sizeof(uint32_t), (void **)&ctx->last_wr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_aig_arr_alloc(pool, ir_ctx->sig_cunt, sizeof(uint8_t), (void **)&ctx->latch_is);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_aig_arr_alloc(pool, ir_ctx->sig_cunt > ir_ctx->comb_cunt ? ir_ctx->sig_cunt : ir_ctx->comb_cunt, sizeof(uint32_t), (void **)&ctx->op_order);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_aig_arr_alloc(pool, ir_ctx->comb_cunt, sizeof(uint8_t), (void **)&ctx->op_placed);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_aig_arr_alloc(pool, (uint64_t)ir_ctx->comb_cunt * CROSSTALK_HDL_AIG_RD_SLOT_CUNT, sizeof(uint32_t), (void **)&ctx->rd_dep_sig);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_aig_arr_alloc(pool, (uint64_t)ir_ctx->comb_cunt * CROSSTALK_HDL_AIG_RD_SLOT_CUNT, sizeof(uint32_t), (void **)&ctx->rd_dep_op);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_aig_arr_alloc(pool, (uint64_t)ir_ctx->comb_cunt * CROSSTALK_HDL_AIG_WIDTH_MAX, sizeof(uint32_t), (void **)&ctx->op_out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < ir_ctx->sig_cunt; idx = idx + 1)
    {
        sig = &ir_ctx->sigs[idx];
        ctx->lit_off[idx] = UINT32_MAX;
        ctx->ff_off[idx] = UINT32_MAX;
        if (sig->kind == CROSSTALK_HDL_IR_SIG_MEM)
        {
            ctx->mem_bits = ctx->mem_bits + (uint64_t)sig->width * (uint64_t)sig->depth;
            continue;
        }

        ctx->lit_off[idx] = (uint32_t)lit_total;
        lit_total = lit_total + sig->width;
        if (sig->port != CROSSTALK_HDL_IR_PORT_NONE) { ctx->io_bits = ctx->io_bits + sig->width; }
    }

    ret = crosstalk_hdl_aig_arr_alloc(pool, lit_total, sizeof(uint32_t), (void **)&ctx->lit_arena);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (op_idx = 0; op_idx < ir_ctx->seq_op_cunt; op_idx = op_idx + 1)
    {
        op = &ir_ctx->seq_ops[op_idx];
        if (op->kind != CROSSTALK_HDL_IR_OP_REG_NEXT || op->dst == UINT32_MAX) { continue; }

        if (ctx->ff_off[op->dst] != UINT32_MAX) { continue; }

        ctx->ff_off[op->dst] = (uint32_t)ff_total;
        ff_total = ff_total + ir_ctx->sigs[op->dst].width;
        ctx->ff_bits = ctx->ff_bits + ir_ctx->sigs[op->dst].width;
    }

    ret = crosstalk_hdl_aig_arr_alloc(pool, ff_total, sizeof(uint32_t), (void **)&ctx->ff_arena);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    root_total = ff_total + (uint64_t)ctx->io_bits;
    for (op_idx = 0; op_idx < ir_ctx->comb_cunt; op_idx = op_idx + 1)
    {
        op = &ir_ctx->comb_ops[op_idx];
        if (op->kind == CROSSTALK_HDL_IR_OP_MEM_RD || op->kind == CROSSTALK_HDL_IR_OP_MEM_WR) { root_total = root_total + 0x1 + (uint64_t)CROSSTALK_HDL_AIG_WIDTH_MAX * 0x2; mem_op_total = mem_op_total + 0x1; }
    }

    for (op_idx = 0; op_idx < ir_ctx->seq_op_cunt; op_idx = op_idx + 1)
    {
        op = &ir_ctx->seq_ops[op_idx];
        if (op->kind == CROSSTALK_HDL_IR_OP_MEM_RD || op->kind == CROSSTALK_HDL_IR_OP_MEM_WR) { root_total = root_total + 0x1 + (uint64_t)CROSSTALK_HDL_AIG_WIDTH_MAX * 0x2; mem_op_total = mem_op_total + 0x1; }
    }

    ctx->root_cap = (uint32_t)root_total + CROSSTALK_HDL_AIG_ROOT_CAP_DEFAULT;
    ret = crosstalk_hdl_aig_arr_alloc(pool, ctx->root_cap, sizeof(uint32_t), (void **)&ctx->roots);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->pi_cap = (uint32_t)(ff_total + (uint64_t)ctx->io_bits + (uint64_t)mem_op_total * CROSSTALK_HDL_AIG_WIDTH_MAX) + CROSSTALK_HDL_AIG_ROOT_CAP_DEFAULT;
    ret = crosstalk_hdl_aig_arr_alloc(pool, ctx->pi_cap, sizeof(uint32_t), (void **)&ctx->pi_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_aig_arr_alloc(pool, ctx->pi_cap, sizeof(uint32_t), (void **)&ctx->pi_sig);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_aig_arr_alloc(pool, ctx->pi_cap, sizeof(uint32_t), (void **)&ctx->pi_bit);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_aig_arr_alloc(pool, ctx->pi_cap, sizeof(uint32_t), (void **)&ctx->pi_op);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_aig_arr_alloc(pool, ctx->pi_cap, sizeof(uint8_t), (void **)&ctx->pi_kind);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_aig_arr_alloc(pool, (uint64_t)ir_ctx->comb_cunt + (uint64_t)ir_ctx->seq_op_cunt, sizeof(uint32_t), (void **)&ctx->op_root_off);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_aig_arr_alloc(pool, (uint64_t)ir_ctx->comb_cunt + (uint64_t)ir_ctx->seq_op_cunt, sizeof(uint32_t), (void **)&ctx->op_pi_base);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (op_idx = 0; op_idx < ir_ctx->comb_cunt + ir_ctx->seq_op_cunt; op_idx = op_idx + 1)
    {
        ctx->op_root_off[op_idx] = UINT32_MAX;
        ctx->op_pi_base[op_idx] = UINT32_MAX;
    }

    ctx->nodes[0].fanin0 = UINT32_MAX;
    ctx->nodes[0].fanin1 = UINT32_MAX;
    ctx->node_cunt = 0x1;

    for (idx = 0; idx < ir_ctx->sig_cunt; idx = idx + 1)
    {
        sig = &ir_ctx->sigs[idx];
        if (ctx->lit_off[idx] == UINT32_MAX) { continue; }

        if (sig->port == CROSSTALK_HDL_IR_PORT_IN || sig->port == CROSSTALK_HDL_IR_PORT_INOUT || ctx->ff_off[idx] != UINT32_MAX)
        {
            for (bit_idx = 0; bit_idx < sig->width; bit_idx = bit_idx + 1)
            {
                ret = crosstalk_hdl_aig_pi_get(ctx, &ctx->lit_arena[ctx->lit_off[idx] + bit_idx]);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                ret = crosstalk_hdl_aig_pi_note(ctx, ctx->ff_off[idx] != UINT32_MAX ? CROSSTALK_HDL_AIG_PI_KIND_FF : CROSSTALK_HDL_AIG_PI_KIND_IN, idx, bit_idx, UINT32_MAX, ctx->lit_arena[ctx->lit_off[idx] + bit_idx]);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            continue;
        }

        for (bit_idx = 0; bit_idx < sig->width; bit_idx = bit_idx + 1) { ctx->lit_arena[ctx->lit_off[idx] + bit_idx] = ((sig->init_val >> bit_idx) & 0x1) != 0 ? CROSSTALK_HDL_AIG_LIT_TRUE : CROSSTALK_HDL_AIG_LIT_FALSE; }
    }

    ctx->comb_mode = LDG_TRUTH_TRUE;
    ret = crosstalk_hdl_aig_comb_place(ctx);
    ctx->comb_mode = LDG_TRUTH_FALSE;
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (blk_idx = 0; blk_idx < ir_ctx->seq_blk_cunt; blk_idx = blk_idx + 1) { for (op_idx = 0; op_idx < ir_ctx->seq_blks[blk_idx].op_cunt; op_idx = op_idx + 1)
        {
            ctx->cur_op = ir_ctx->comb_cunt + ir_ctx->seq_blks[blk_idx].op_start + op_idx;
            ret = crosstalk_hdl_aig_op_proc(ctx, &ir_ctx->seq_ops[ir_ctx->seq_blks[blk_idx].op_start + op_idx]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }

    ctx->ff_root_base = ctx->root_cunt;
    for (idx = 0; idx < ir_ctx->sig_cunt; idx = idx + 1)
    {
        if (ctx->ff_off[idx] != UINT32_MAX)
        {
            ret = crosstalk_hdl_aig_root_add(ctx, ctx->ff_arena + ctx->ff_off[idx], ir_ctx->sigs[idx].width);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        sig = &ir_ctx->sigs[idx];
        if (ctx->lit_off[idx] != UINT32_MAX && (sig->port == CROSSTALK_HDL_IR_PORT_OUT || sig->port == CROSSTALK_HDL_IR_PORT_INOUT))
        {
            ret = crosstalk_hdl_aig_root_add(ctx, ctx->lit_arena + ctx->lit_off[idx], sig->width);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }

    return LDG_ERR_AOK;
}
