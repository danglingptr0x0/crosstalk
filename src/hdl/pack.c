#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/str/str.h>

#include "hdl/aig.h"
#include "hdl/ir.h"
#include "hdl/pack.h"

#define CROSSTALK_HDL_PACK_LANE_CUNT 64
#define CROSSTALK_HDL_PACK_LANE_MASK 63
#define CROSSTALK_HDL_PACK_WORD_STATE 0
#define CROSSTALK_HDL_PACK_WORD_AND 1
#define CROSSTALK_HDL_PACK_WORD_MEM_RD 2
#define CROSSTALK_HDL_PACK_SETTLE_MAX 128
#define CROSSTALK_HDL_PACK_BODY_BASE_CAP 0x2000000
#define CROSSTALK_HDL_PACK_OUT_BASE_CAP 0x1000000
#define CROSSTALK_HDL_PACK_BYTE_FILL 255

static uint64_t crosstalk_hdl_pack_mask_get(uint32_t width)
{
    if (width >= CROSSTALK_HDL_PACK_LANE_CUNT) { return ~(uint64_t)0x0; }

    if (width == 0) { return 0x0; }

    return ((uint64_t)0x1 << width) - 0x1;
}

static uint32_t crosstalk_hdl_pack_arr_alloc(void *pool, uint64_t cunt, uint64_t item_size, void **out)
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

static uint32_t crosstalk_hdl_pack_bytes_emit(crosstalk_hdl_pack_ctx_t *ctx, const char *ptr, uint32_t wr_len)
{
    if (LDG_UNLIKELY(ctx == 0x0 || ptr == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (wr_len == 0) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(ctx->out_len + wr_len > ctx->out_cap)) { return LDG_ERR_FULL; }

    memcpy(ctx->out + ctx->out_len, ptr, wr_len);
    ctx->out_len = ctx->out_len + wr_len;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_pack_str_emit(crosstalk_hdl_pack_ctx_t *ctx, const char *text)
{
    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_hdl_pack_bytes_emit(ctx, text, (uint32_t)strlen(text));
}

static uint32_t crosstalk_hdl_pack_dec_emit(crosstalk_hdl_pack_ctx_t *ctx, uint64_t val)
{
    char digits[24];
    uint32_t cunt = 0;
    uint32_t idx = 0;
    char swap_ch = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (val == 0)
    {
        digits[0] = 0x30;

        return crosstalk_hdl_pack_bytes_emit(ctx, digits, 0x1);
    }

    while (val != 0)
    {
        digits[cunt] = (char)(0x30 + (val % LDG_BASE_DEC));
        cunt = cunt + 0x1;
        val = val / LDG_BASE_DEC;
    }

    for (idx = 0; idx < cunt / 0x2; idx = idx + 0x1)
    {
        swap_ch = digits[idx];
        digits[idx] = digits[cunt - 0x1 - idx];
        digits[cunt - 0x1 - idx] = swap_ch;
    }

    return crosstalk_hdl_pack_bytes_emit(ctx, digits, cunt);
}

static uint32_t crosstalk_hdl_pack_u64_emit(crosstalk_hdl_pack_ctx_t *ctx, uint64_t val)
{
    uint32_t ret = 0;

    ret = crosstalk_hdl_pack_dec_emit(ctx, val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_pack_str_emit(ctx, "UL");
}

static uint32_t crosstalk_hdl_pack_name_emit(crosstalk_hdl_pack_ctx_t *ctx, uint32_t sig_idx)
{
    const crosstalk_hdl_tok_t *tok = 0x0;
    uint32_t idx = 0;
    uint32_t ok = 0x1;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    tok = &ctx->ir_ctx->sigs[sig_idx].name;
    if (tok->len == 0 || ctx->src == 0x0) { ok = 0; }

    for (idx = 0; ok != 0 && idx < tok->len; idx = idx + 0x1)
    {
        char name_ch = ctx->src[tok->off + idx];

        if (ldg_char_alnum_is(name_ch) != LDG_TRUTH_TRUE && name_ch != '_') { ok = 0; }
    }

    if (ok == 0) { return LDG_ERR_AOK; }

    ret = crosstalk_hdl_pack_bytes_emit(ctx, ctx->src + tok->off, tok->len);

    return ret;
}

static uint32_t crosstalk_hdl_pack_pl_get(const crosstalk_hdl_pack_ctx_t *ctx, uint32_t lit)
{
    return (ctx->pos[lit >> 0x1] << 0x1) | (lit & 0x1);
}

static uint32_t crosstalk_hdl_pack_blob_add(crosstalk_hdl_pack_ctx_t *ctx, const uint32_t *entries, uint32_t cunt, uint32_t *out_off)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || entries == 0x0 || out_off == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_off = ctx->gt_cunt;
    if (LDG_UNLIKELY(ctx->gt_cunt + cunt > ctx->gt_cap)) { return CROSSTALK_HDL_ERR_PACK; }

    for (idx = 0; idx < cunt; idx = idx + 0x1)
    {
        ctx->gt_blob[ctx->gt_cunt] = entries[idx];
        ctx->gt_cunt = ctx->gt_cunt + 0x1;
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_pack_side_emit(crosstalk_hdl_pack_ctx_t *ctx, const uint32_t *lane_ent)
{
    uint64_t neg_mask = 0;
    uint32_t used_cunt = 0;
    uint32_t first_lane = UINT32_MAX;
    uint32_t bcast_is = 0x1;
    uint32_t affine_is = 0x1;
    uint32_t affine_base = 0;
    uint32_t bcast_pos = 0;
    uint32_t blob_off = 0;
    uint32_t lane = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || lane_ent == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (lane = 0; lane < CROSSTALK_HDL_PACK_LANE_CUNT; lane = lane + 0x1)
    {
        uint32_t ent = lane_ent[lane];
        uint32_t src_pos = 0;

        if (ent == UINT32_MAX) { continue; }

        src_pos = ent >> 0x1;
        if ((ent & 0x1) != 0) { neg_mask = neg_mask | ((uint64_t)0x1 << lane); }

        if (first_lane == UINT32_MAX)
        {
            first_lane = lane;
            bcast_pos = src_pos;
            if (src_pos + first_lane >= src_pos + lane && src_pos >= lane) { affine_base = src_pos - lane; }
            else { affine_is = 0; }
        }
        else
        {
            if (src_pos != bcast_pos) { bcast_is = 0; }

            if (src_pos < lane || src_pos - lane != affine_base) { affine_is = 0; }
        }

        used_cunt = used_cunt + 0x1;
    }

    if (used_cunt == 0) { return crosstalk_hdl_pack_str_emit(ctx, "0x0"); }

    if (bcast_is != 0 && used_cunt > 0x1)
    {
        ctx->bcast_cunt = ctx->bcast_cunt + 0x1;
        ret = crosstalk_hdl_pack_str_emit(ctx, "(((hdl_u64)0x0 - (nw[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_dec_emit(ctx, bcast_pos >> 0x6);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_str_emit(ctx, "] >> ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_dec_emit(ctx, bcast_pos & CROSSTALK_HDL_PACK_LANE_MASK);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_str_emit(ctx, " & 0x1))");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (neg_mask != 0)
        {
            ret = crosstalk_hdl_pack_str_emit(ctx, " ^ ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_pack_u64_emit(ctx, neg_mask);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        return crosstalk_hdl_pack_str_emit(ctx, ")");
    }

    if (affine_is != 0)
    {
        uint32_t base_word = affine_base >> 0x6;
        uint32_t base_sh = affine_base & CROSSTALK_HDL_PACK_LANE_MASK;

        ctx->affine_cunt = ctx->affine_cunt + 0x1;
        ret = crosstalk_hdl_pack_str_emit(ctx, "(");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (base_sh == 0)
        {
            ret = crosstalk_hdl_pack_str_emit(ctx, "nw[");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_pack_dec_emit(ctx, base_word);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_pack_str_emit(ctx, "]");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
        else
        {
            ret = crosstalk_hdl_pack_str_emit(ctx, "(nw[");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_pack_dec_emit(ctx, base_word);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_pack_str_emit(ctx, "] >> ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_pack_dec_emit(ctx, base_sh);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_pack_str_emit(ctx, " | nw[");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_pack_dec_emit(ctx, base_word + 0x1);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_pack_str_emit(ctx, "] << ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_pack_dec_emit(ctx, CROSSTALK_HDL_PACK_LANE_CUNT - base_sh);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_pack_str_emit(ctx, ")");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        if (neg_mask != 0)
        {
            ret = crosstalk_hdl_pack_str_emit(ctx, " ^ ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_pack_u64_emit(ctx, neg_mask);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        return crosstalk_hdl_pack_str_emit(ctx, ")");
    }

    ctx->table_cunt = ctx->table_cunt + 0x1;
    ret = crosstalk_hdl_pack_blob_add(ctx, lane_ent, CROSSTALK_HDL_PACK_LANE_CUNT, &blob_off);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "hdl_pack_gt(&gsl[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_dec_emit(ctx, blob_off);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_pack_str_emit(ctx, "])");
}

static uint32_t crosstalk_hdl_pack_ext_emit(crosstalk_hdl_pack_ctx_t *ctx, const uint32_t *lits, uint32_t cunt)
{
    uint32_t entries[CROSSTALK_HDL_PACK_LANE_CUNT];
    uint32_t blob_off = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || lits == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt > CROSSTALK_HDL_PACK_LANE_CUNT)) { return CROSSTALK_HDL_ERR_PACK; }

    for (idx = 0; idx < cunt; idx = idx + 0x1) { entries[idx] = crosstalk_hdl_pack_pl_get(ctx, lits[idx]); }

    ret = crosstalk_hdl_pack_blob_add(ctx, entries, cunt, &blob_off);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "hdl_pack_ext(&gsl[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_dec_emit(ctx, blob_off);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "], ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_dec_emit(ctx, cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_pack_str_emit(ctx, ")");
}

static uint32_t crosstalk_hdl_pack_sort_le_is(const uint32_t *kneg, const uint32_t *kp0, const uint32_t *kp1, uint32_t lhs, uint32_t rhs)
{
    if (kneg[lhs] != kneg[rhs]) { return kneg[lhs] < kneg[rhs] ? 0x1 : 0x0; }

    if (kp0[lhs] != kp0[rhs]) { return kp0[lhs] < kp0[rhs] ? 0x1 : 0x0; }

    if (kp1[lhs] != kp1[rhs]) { return kp1[lhs] < kp1[rhs] ? 0x1 : 0x0; }

    return 0x1;
}

static void crosstalk_hdl_pack_sort_run(uint32_t *arr, uint32_t *buff, uint32_t cunt, const uint32_t *kneg, const uint32_t *kp0, const uint32_t *kp1)
{
    uint32_t *src = arr;
    uint32_t *dst = buff;
    uint32_t *swap_ptr = 0x0;
    uint32_t run_w = 0x1;
    uint32_t lo = 0;

    if (cunt < 0x2) { return; }

    while (run_w < cunt)
    {
        for (lo = 0; lo < cunt; lo = lo + run_w * 0x2)
        {
            uint32_t mid = lo + run_w < cunt ? lo + run_w : cunt;
            uint32_t hi = lo + run_w * 0x2 < cunt ? lo + run_w * 0x2 : cunt;
            uint32_t li = lo;
            uint32_t ri = mid;
            uint32_t oi = lo;

            while (li < mid && ri < hi)
            {
                if (crosstalk_hdl_pack_sort_le_is(kneg, kp0, kp1, src[li], src[ri]) != 0) { dst[oi] = src[li]; li = li + 0x1; }
                else { dst[oi] = src[ri]; ri = ri + 0x1; }

                oi = oi + 0x1;
            }

            while (li < mid) { dst[oi] = src[li]; li = li + 0x1; oi = oi + 0x1; }

            while (ri < hi) { dst[oi] = src[ri]; ri = ri + 0x1; oi = oi + 0x1; }
        }

        swap_ptr = src;
        src = dst;
        dst = swap_ptr;
        run_w = run_w * 0x2;
    }

    if (src != arr) { memcpy(arr, src, (uint64_t)cunt * sizeof(uint32_t)); }
}

static uint32_t crosstalk_hdl_pack_clk_root_get(const crosstalk_hdl_ir_t *ir_ctx, uint32_t sig)
{
    uint32_t depth = 0;

    if (LDG_UNLIKELY(ir_ctx == 0x0)) { return UINT32_MAX; }

    for (depth = 0; depth < CROSSTALK_HDL_PACK_LANE_CUNT && sig != UINT32_MAX; depth = depth + 0x1)
    {
        const crosstalk_hdl_ir_op_t *wr_op = 0x0;
        uint32_t wr_cunt = 0;
        uint32_t op_idx = 0;

        for (op_idx = 0; op_idx < ir_ctx->comb_cunt; op_idx = op_idx + 0x1)
        {
            const crosstalk_hdl_ir_op_t *op = &ir_ctx->comb_ops[op_idx];

            if (op->kind == CROSSTALK_HDL_IR_OP_MEM_WR || op->dst != sig) { continue; }

            wr_op = op;
            wr_cunt = wr_cunt + 0x1;
        }

        if (wr_cunt != 0x1) { break; }

        if (wr_op->kind != CROSSTALK_HDL_IR_OP_COPY && wr_op->kind != CROSSTALK_HDL_IR_OP_ZEXT && (wr_op->kind != CROSSTALK_HDL_IR_OP_SLICE || wr_op->imm != 0)) { break; }

        if (wr_op->src0 == UINT32_MAX) { break; }

        sig = wr_op->src0;
    }

    return sig;
}

static uint32_t crosstalk_hdl_pack_gop_get(const crosstalk_hdl_pack_ctx_t *ctx, uint32_t gop, const crosstalk_hdl_ir_op_t **out_op)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_op == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (gop < ctx->ir_ctx->comb_cunt) { *out_op = &ctx->ir_ctx->comb_ops[gop]; }
    else { *out_op = &ctx->ir_ctx->seq_ops[gop - ctx->ir_ctx->comb_cunt]; }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_pack_lvl_build(crosstalk_hdl_pack_ctx_t *ctx, const uint32_t *node_pi)
{
    const crosstalk_hdl_aig_ctx_t *aig_ctx = 0x0;
    const crosstalk_hdl_ir_op_t *op = 0x0;
    uint32_t node = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || node_pi == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    aig_ctx = ctx->aig_ctx;
    ctx->lvl[0] = 0;
    ctx->level_max = 0;
    for (node = 0x1; node < aig_ctx->node_cunt; node = node + 0x1)
    {
        uint32_t node_lvl = 0;

        if (aig_ctx->nodes[node].fanin0 == UINT32_MAX)
        {
            uint32_t pi_idx = node_pi[node];

            if (LDG_UNLIKELY(pi_idx == UINT32_MAX)) { return CROSSTALK_HDL_ERR_PACK; }

            if (aig_ctx->pi_kind[pi_idx] == CROSSTALK_HDL_AIG_PI_KIND_MEM_RD)
            {
                uint32_t gop = aig_ctx->pi_op[pi_idx];
                uint32_t root_off = aig_ctx->op_root_off[gop];
                uint32_t addr_w = 0;
                uint32_t idx = 0;

                ret = crosstalk_hdl_pack_gop_get(ctx, gop, &op);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                if (op->src1 != UINT32_MAX) { addr_w = ctx->ir_ctx->sigs[op->src1].width; }

                for (idx = 0; idx < addr_w; idx = idx + 0x1)
                {
                    uint32_t addr_node = aig_ctx->roots[root_off + idx] >> 0x1;

                    if (ctx->lvl[addr_node] >= node_lvl) { node_lvl = ctx->lvl[addr_node] + 0x1; }
                }

                if (node_lvl == 0) { node_lvl = 0x1; }
            }
        }
        else
        {
            uint32_t f0_lvl = ctx->lvl[aig_ctx->nodes[node].fanin0 >> 0x1];
            uint32_t f1_lvl = ctx->lvl[aig_ctx->nodes[node].fanin1 >> 0x1];

            node_lvl = (f0_lvl > f1_lvl ? f0_lvl : f1_lvl) + 0x1;
        }

        ctx->lvl[node] = node_lvl;
        if (node_lvl > ctx->level_max) { ctx->level_max = node_lvl; }
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_pack_word_open(crosstalk_hdl_pack_ctx_t *ctx, uint32_t *bit_pos, uint32_t kind, uint32_t gop, uint32_t word_cap)
{
    uint32_t word_idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || bit_pos == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if ((*bit_pos & CROSSTALK_HDL_PACK_LANE_MASK) != 0) { *bit_pos = (*bit_pos | CROSSTALK_HDL_PACK_LANE_MASK) + 0x1; }

    word_idx = *bit_pos >> 0x6;
    if (LDG_UNLIKELY(word_idx >= word_cap)) { return CROSSTALK_HDL_ERR_PACK; }

    ctx->word_kind[word_idx] = kind;
    ctx->word_op[word_idx] = gop;
    if (word_idx + 0x1 > ctx->word_cunt) { ctx->word_cunt = word_idx + 0x1; }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_pack_place_run(crosstalk_hdl_pack_ctx_t *ctx, const uint32_t *node_pi, uint32_t *bit_node, uint32_t word_cap, uint32_t *kneg, uint32_t *kp0, uint32_t *kp1)
{
    const crosstalk_hdl_aig_ctx_t *aig_ctx = 0x0;
    uint32_t bit_pos = 0;
    uint32_t lvl_idx = 0;
    uint32_t node = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || node_pi == 0x0 || bit_node == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    aig_ctx = ctx->aig_ctx;
    for (idx = 0; idx <= ctx->level_max; idx = idx + 0x1) { ctx->lvl_cunt[idx] = 0; }

    for (node = 0; node < aig_ctx->node_cunt; node = node + 0x1) { ctx->lvl_cunt[ctx->lvl[node]] = ctx->lvl_cunt[ctx->lvl[node]] + 0x1; }

    ctx->lvl_off[0] = 0;
    for (idx = 0x1; idx <= ctx->level_max; idx = idx + 0x1) { ctx->lvl_off[idx] = ctx->lvl_off[idx - 0x1] + ctx->lvl_cunt[idx - 0x1]; }

    for (idx = 0; idx <= ctx->level_max; idx = idx + 0x1) { ctx->lvl_cunt[idx] = 0; }

    for (node = 0; node < aig_ctx->node_cunt; node = node + 0x1)
    {
        uint32_t node_lvl = ctx->lvl[node];

        ctx->ord[ctx->lvl_off[node_lvl] + ctx->lvl_cunt[node_lvl]] = node;
        ctx->lvl_cunt[node_lvl] = ctx->lvl_cunt[node_lvl] + 0x1;
    }

    for (idx = 0; idx < (uint64_t)word_cap * CROSSTALK_HDL_PACK_LANE_CUNT / CROSSTALK_HDL_PACK_LANE_CUNT; idx = idx + 0x1) { ctx->word_kind[idx] = CROSSTALK_HDL_PACK_WORD_STATE; }

    memset(bit_node, CROSSTALK_HDL_PACK_BYTE_FILL, (uint64_t)word_cap * CROSSTALK_HDL_PACK_LANE_CUNT * sizeof(uint32_t));
    ret = crosstalk_hdl_pack_word_open(ctx, &bit_pos, CROSSTALK_HDL_PACK_WORD_STATE, UINT32_MAX, word_cap);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->pos[0] = 0;
    bit_node[0] = 0;
    bit_pos = 0x1;
    for (idx = 0; idx < ctx->lvl_cunt[0]; idx = idx + 0x1)
    {
        node = ctx->ord[ctx->lvl_off[0] + idx];
        if (node == 0) { continue; }

        if ((bit_pos & CROSSTALK_HDL_PACK_LANE_MASK) == 0)
        {
            ret = crosstalk_hdl_pack_word_open(ctx, &bit_pos, CROSSTALK_HDL_PACK_WORD_STATE, UINT32_MAX, word_cap);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ctx->pos[node] = bit_pos;
        bit_node[bit_pos] = node;
        bit_pos = bit_pos + 0x1;
    }

    for (lvl_idx = 0x1; lvl_idx <= ctx->level_max; lvl_idx = lvl_idx + 0x1)
    {
        uint32_t lvl_base = ctx->lvl_off[lvl_idx];
        uint32_t lvl_total = ctx->lvl_cunt[lvl_idx];
        uint32_t and_cunt = 0;
        uint32_t open_is = 0;

        for (idx = 0; idx < lvl_total; idx = idx + 0x1)
        {
            node = ctx->ord[lvl_base + idx];
            if (aig_ctx->nodes[node].fanin0 != UINT32_MAX)
            {
                uint32_t f0 = aig_ctx->nodes[node].fanin0;
                uint32_t f1 = aig_ctx->nodes[node].fanin1;

                kneg[node] = ((f0 & 0x1) << 0x1) | (f1 & 0x1);
                kp0[node] = ctx->pos[f0 >> 0x1];
                kp1[node] = ctx->pos[f1 >> 0x1];
                ctx->sort_buff[and_cunt] = node;
                and_cunt = and_cunt + 0x1;
                continue;
            }

            {
                uint32_t pi_idx = node_pi[node];
                uint32_t gop = aig_ctx->pi_op[pi_idx];
                uint32_t bit_idx = aig_ctx->pi_bit[pi_idx];

                if (bit_idx == 0)
                {
                    ret = crosstalk_hdl_pack_word_open(ctx, &bit_pos, CROSSTALK_HDL_PACK_WORD_MEM_RD, gop, word_cap);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
                }

                ctx->pos[node] = (bit_pos & ~(uint32_t)CROSSTALK_HDL_PACK_LANE_MASK) + bit_idx;
                if (LDG_UNLIKELY((ctx->pos[node] >> 0x6) >= word_cap)) { return CROSSTALK_HDL_ERR_PACK; }

                bit_node[ctx->pos[node]] = node;
                bit_pos = ctx->pos[node] + 0x1;
            }
        }

        if (and_cunt > 0)
        {
            crosstalk_hdl_pack_sort_run(ctx->sort_buff, ctx->sort_buff + and_cunt, and_cunt, kneg, kp0, kp1);
            ret = crosstalk_hdl_pack_word_open(ctx, &bit_pos, CROSSTALK_HDL_PACK_WORD_AND, UINT32_MAX, word_cap);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            open_is = 0x1;
            for (idx = 0; idx < and_cunt; idx = idx + 0x1)
            {
                uint32_t cur_word = bit_pos >> 0x6;
                uint32_t f0_word = 0;
                uint32_t f1_word = 0;

                node = ctx->sort_buff[idx];
                f0_word = ctx->pos[aig_ctx->nodes[node].fanin0 >> 0x1] >> 0x6;
                f1_word = ctx->pos[aig_ctx->nodes[node].fanin1 >> 0x1] >> 0x6;
                if (open_is == 0 || (bit_pos & CROSSTALK_HDL_PACK_LANE_MASK) == 0 || f0_word == cur_word || f1_word == cur_word)
                {
                    if ((bit_pos & CROSSTALK_HDL_PACK_LANE_MASK) != 0 || open_is == 0 || ctx->word_kind[bit_pos >> 0x6] != CROSSTALK_HDL_PACK_WORD_AND)
                    {
                        ret = crosstalk_hdl_pack_word_open(ctx, &bit_pos, CROSSTALK_HDL_PACK_WORD_AND, UINT32_MAX, word_cap);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
                    }

                    open_is = 0x1;
                }

                ctx->pos[node] = bit_pos;
                bit_node[bit_pos] = node;
                bit_pos = bit_pos + 0x1;
            }
        }
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_pack_mem_layout_get(const crosstalk_hdl_pack_ctx_t *ctx, uint32_t *mem_off, uint64_t *out_words)
{
    uint64_t total = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || mem_off == 0x0 || out_words == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < ctx->ir_ctx->sig_cunt; idx = idx + 0x1)
    {
        mem_off[idx] = 0;
        if (ctx->ir_ctx->sigs[idx].kind != CROSSTALK_HDL_IR_SIG_MEM) { continue; }

        mem_off[idx] = (uint32_t)total;
        total = total + ctx->ir_ctx->sigs[idx].depth;
    }

    *out_words = total != 0 ? total : 0x1;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_pack_mem_rd_emit(crosstalk_hdl_pack_ctx_t *ctx, uint32_t word_idx, const uint32_t *mem_off, uint8_t chg_is)
{
    const crosstalk_hdl_aig_ctx_t *aig_ctx = 0x0;
    const crosstalk_hdl_ir_op_t *op = 0x0;
    uint32_t gop = 0;
    uint32_t addr_w = 0;
    uint32_t mem_sig = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || mem_off == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    aig_ctx = ctx->aig_ctx;
    gop = ctx->word_op[word_idx];
    ret = crosstalk_hdl_pack_gop_get(ctx, gop, &op);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    mem_sig = op->src0;
    if (LDG_UNLIKELY(mem_sig == UINT32_MAX || aig_ctx->op_root_off[gop] == UINT32_MAX)) { return CROSSTALK_HDL_ERR_PACK; }

    if (op->src1 != UINT32_MAX) { addr_w = ctx->ir_ctx->sigs[op->src1].width; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "{ hdl_u64 ad = ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (addr_w > 0) { ret = crosstalk_hdl_pack_ext_emit(ctx, aig_ctx->roots + aig_ctx->op_root_off[gop], addr_w); }
    else { ret = crosstalk_hdl_pack_str_emit(ctx, "0x0"); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (chg_is == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_pack_str_emit(ctx, "; hdl_u64 nv2 = ad < "); }
    else
    {
        ret = crosstalk_hdl_pack_str_emit(ctx, "; nw[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_dec_emit(ctx, word_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_str_emit(ctx, "] = ad < ");
    }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_u64_emit(ctx, ctx->ir_ctx->sigs[mem_sig].depth);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, " ? m[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_dec_emit(ctx, mem_off[mem_sig]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, " + ad] & ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_u64_emit(ctx, crosstalk_hdl_pack_mask_get(ctx->ir_ctx->sigs[mem_sig].width));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (chg_is != LDG_TRUTH_TRUE) { return crosstalk_hdl_pack_str_emit(ctx, " : 0x0; }\n"); }

    ret = crosstalk_hdl_pack_str_emit(ctx, " : 0x0; if (nw[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_dec_emit(ctx, word_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "] != nv2) { nw[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_dec_emit(ctx, word_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_pack_str_emit(ctx, "] = nv2; chg = 0x1; } }\n");
}

static uint32_t crosstalk_hdl_pack_and_word_emit(crosstalk_hdl_pack_ctx_t *ctx, uint32_t word_idx, const uint32_t *bit_node)
{
    uint32_t ent0[CROSSTALK_HDL_PACK_LANE_CUNT];
    uint32_t ent1[CROSSTALK_HDL_PACK_LANE_CUNT];
    uint32_t lane = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || bit_node == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (lane = 0; lane < CROSSTALK_HDL_PACK_LANE_CUNT; lane = lane + 0x1)
    {
        uint32_t node = bit_node[((uint64_t)word_idx << 0x6) + lane];

        ent0[lane] = UINT32_MAX;
        ent1[lane] = UINT32_MAX;
        if (node == UINT32_MAX) { continue; }

        ent0[lane] = crosstalk_hdl_pack_pl_get(ctx, ctx->aig_ctx->nodes[node].fanin0);
        ent1[lane] = crosstalk_hdl_pack_pl_get(ctx, ctx->aig_ctx->nodes[node].fanin1);
    }

    ret = crosstalk_hdl_pack_str_emit(ctx, "nw[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_dec_emit(ctx, word_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "] = ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_side_emit(ctx, ent0);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, " & ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_side_emit(ctx, ent1);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_pack_str_emit(ctx, ";\n");
}

static uint32_t crosstalk_hdl_pack_mem_wr_emit(crosstalk_hdl_pack_ctx_t *ctx, uint32_t gop, const uint32_t *mem_off, uint8_t comb_is)
{
    const crosstalk_hdl_aig_ctx_t *aig_ctx = 0x0;
    const crosstalk_hdl_ir_op_t *op = 0x0;
    uint32_t root_off = 0;
    uint32_t addr_w = 0;
    uint32_t data_w = 0;
    uint32_t mem_sig = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || mem_off == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    aig_ctx = ctx->aig_ctx;
    root_off = aig_ctx->op_root_off[gop];
    ret = crosstalk_hdl_pack_gop_get(ctx, gop, &op);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    mem_sig = op->src0;
    if (LDG_UNLIKELY(mem_sig == UINT32_MAX || root_off == UINT32_MAX)) { return CROSSTALK_HDL_ERR_PACK; }

    if (op->src1 != UINT32_MAX) { addr_w = ctx->ir_ctx->sigs[op->src1].width; }

    if (op->src2 != UINT32_MAX) { data_w = ctx->ir_ctx->sigs[op->src2].width; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "if (hdl_pack_bit(");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_dec_emit(ctx, crosstalk_hdl_pack_pl_get(ctx, aig_ctx->roots[root_off]));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, ") != 0)\n{\nhdl_u64 ad = ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (addr_w > 0) { ret = crosstalk_hdl_pack_ext_emit(ctx, aig_ctx->roots + root_off + 0x1, addr_w); }
    else { ret = crosstalk_hdl_pack_str_emit(ctx, "0x0"); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, ";\nhdl_u64 dv = ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (data_w > 0) { ret = crosstalk_hdl_pack_ext_emit(ctx, aig_ctx->roots + root_off + 0x1 + addr_w, data_w); }
    else { ret = crosstalk_hdl_pack_str_emit(ctx, "0x0"); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, ";\nif (ad < ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_u64_emit(ctx, ctx->ir_ctx->sigs[mem_sig].depth);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (comb_is == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_pack_str_emit(ctx, ")\n{\nhdl_u64 nv2 = dv & ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_u64_emit(ctx, crosstalk_hdl_pack_mask_get(ctx->ir_ctx->sigs[mem_sig].width));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_str_emit(ctx, ";\nif (m[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_dec_emit(ctx, mem_off[mem_sig]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_str_emit(ctx, " + ad] != nv2) { m[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_dec_emit(ctx, mem_off[mem_sig]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_str_emit(ctx, " + ad] = nv2; chg = 0x1; }\n}\n}\n");

        return ret;
    }

    ret = crosstalk_hdl_pack_str_emit(ctx, ") { m[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_dec_emit(ctx, mem_off[mem_sig]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, " + ad] = dv & ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_u64_emit(ctx, crosstalk_hdl_pack_mask_get(ctx->ir_ctx->sigs[mem_sig].width));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_pack_str_emit(ctx, "; }\n}\n");
}

static uint32_t crosstalk_hdl_pack_comb_emit(crosstalk_hdl_pack_ctx_t *ctx, const uint32_t *bit_node, const uint32_t *mem_off, uint32_t latch_cunt)
{
    const crosstalk_hdl_ir_op_t *op = 0x0;
    uint8_t *wr_mark = 0x0;
    uint32_t comb_wr_cunt = 0;
    uint32_t word_idx = 0;
    uint32_t gop = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || bit_node == 0x0 || mem_off == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_pack_arr_alloc(ctx->pool, ctx->ir_ctx->sig_cunt, sizeof(uint8_t), (void **)&wr_mark);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (gop = 0; gop < ctx->ir_ctx->comb_cunt; gop = gop + 0x1)
    {
        op = &ctx->ir_ctx->comb_ops[gop];
        if (op->kind != CROSSTALK_HDL_IR_OP_MEM_WR || op->src0 == UINT32_MAX || ctx->aig_ctx->op_root_off[gop] == UINT32_MAX) { continue; }

        wr_mark[op->src0] = LDG_TRUTH_TRUE;
        comb_wr_cunt = comb_wr_cunt + 0x1;
    }

    ret = crosstalk_hdl_pack_str_emit(ctx, "static hdl_u32 hdl_pack_comb(void)\n{\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (latch_cunt > 0 || comb_wr_cunt > 0)
    {
        ret = crosstalk_hdl_pack_str_emit(ctx, "hdl_u32 it2;\nhdl_u32 chg;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (latch_cunt > 0)
        {
            ret = crosstalk_hdl_pack_str_emit(ctx, "hdl_u32 li;\n");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_pack_str_emit(ctx, "for (it2 = 0; it2 < ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_dec_emit(ctx, CROSSTALK_HDL_PACK_SETTLE_MAX);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_str_emit(ctx, "; it2++)\n{\nchg = 0x0;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (word_idx = 0; word_idx < ctx->word_cunt; word_idx = word_idx + 0x1)
    {
        if (ctx->word_kind[word_idx] == CROSSTALK_HDL_PACK_WORD_STATE) { continue; }

        if (ctx->word_kind[word_idx] == CROSSTALK_HDL_PACK_WORD_MEM_RD)
        {
            uint32_t rd_gop = ctx->word_op[word_idx];
            uint8_t chg_is = LDG_TRUTH_FALSE;

            ret = crosstalk_hdl_pack_gop_get(ctx, rd_gop, &op);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (op->src0 != UINT32_MAX && wr_mark[op->src0] == LDG_TRUTH_TRUE) { chg_is = LDG_TRUTH_TRUE; }

            ret = crosstalk_hdl_pack_mem_rd_emit(ctx, word_idx, mem_off, chg_is);
        }
        else { ret = crosstalk_hdl_pack_and_word_emit(ctx, word_idx, bit_node); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (gop = 0; gop < ctx->ir_ctx->comb_cunt; gop = gop + 0x1)
    {
        op = &ctx->ir_ctx->comb_ops[gop];
        if (op->kind != CROSSTALK_HDL_IR_OP_MEM_WR || op->src0 == UINT32_MAX || ctx->aig_ctx->op_root_off[gop] == UINT32_MAX) { continue; }

        ret = crosstalk_hdl_pack_mem_wr_emit(ctx, gop, mem_off, LDG_TRUTH_TRUE);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (latch_cunt > 0)
    {
        ret = crosstalk_hdl_pack_str_emit(ctx, "for (li = 0; li < ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_dec_emit(ctx, latch_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_str_emit(ctx, "; li++)\n{\nhdl_u64 nv = hdl_pack_bit(lt[li * 2 + 0x1]);\nhdl_u32 p2 = lt[li * 2];\nhdl_u64 ov = (nw[p2 >> 6] >> (p2 & 63)) & 0x1;\nif (nv != ov) { nw[p2 >> 6] ^= (hdl_u64)0x1 << (p2 & 63); chg = 0x1; }\n}\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (latch_cunt > 0 || comb_wr_cunt > 0) { return crosstalk_hdl_pack_str_emit(ctx, "if (chg == 0x0) { return 0x0; }\n}\nreturn 0x9;\n}\n\n"); }

    return crosstalk_hdl_pack_str_emit(ctx, "return 0x0;\n}\n\n");
}

static uint32_t crosstalk_hdl_pack_commit_emit(crosstalk_hdl_pack_ctx_t *ctx, const uint32_t *mem_off, const uint32_t *ff_word_list, uint32_t ff_word_cunt, const uint32_t *ff_ent, const uint64_t *ff_mask)
{
    const crosstalk_hdl_aig_ctx_t *aig_ctx = 0x0;
    uint32_t gop = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || mem_off == 0x0 || ff_word_list == 0x0 || ff_ent == 0x0 || ff_mask == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    aig_ctx = ctx->aig_ctx;
    ret = crosstalk_hdl_pack_str_emit(ctx, "static void hdl_pack_commit(void)\n{\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ff_word_cunt > 0)
    {
        ret = crosstalk_hdl_pack_str_emit(ctx, "hdl_u64 tf[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_dec_emit(ctx, ff_word_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_str_emit(ctx, "];\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (gop = ctx->ir_ctx->comb_cunt; gop < ctx->ir_ctx->comb_cunt + ctx->ir_ctx->seq_op_cunt; gop = gop + 0x1)
    {
        const crosstalk_hdl_ir_op_t *op = 0x0;

        if (aig_ctx->op_root_off[gop] == UINT32_MAX) { continue; }

        ret = crosstalk_hdl_pack_gop_get(ctx, gop, &op);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (op->kind != CROSSTALK_HDL_IR_OP_MEM_WR) { continue; }

        ret = crosstalk_hdl_pack_mem_wr_emit(ctx, gop, mem_off, LDG_TRUTH_FALSE);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (idx = 0; idx < ff_word_cunt; idx = idx + 0x1)
    {
        ret = crosstalk_hdl_pack_str_emit(ctx, "tf[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_dec_emit(ctx, idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_str_emit(ctx, "] = ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_side_emit(ctx, ff_ent + (uint64_t)idx * CROSSTALK_HDL_PACK_LANE_CUNT);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_str_emit(ctx, ";\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (idx = 0; idx < ff_word_cunt; idx = idx + 0x1)
    {
        ret = crosstalk_hdl_pack_str_emit(ctx, "nw[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_dec_emit(ctx, ff_word_list[idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_str_emit(ctx, "] = (nw[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_dec_emit(ctx, ff_word_list[idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_str_emit(ctx, "] & ~");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_u64_emit(ctx, ff_mask[idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_str_emit(ctx, ") | (tf[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_dec_emit(ctx, idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_str_emit(ctx, "] & ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_u64_emit(ctx, ff_mask[idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_str_emit(ctx, ");\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return crosstalk_hdl_pack_str_emit(ctx, "}\n\n");
}

static uint32_t crosstalk_hdl_pack_helpers_emit(crosstalk_hdl_pack_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "static hdl_u64 hdl_pack_bit(hdl_u32 e)\n{\nreturn ((nw[e >> 7] >> ((e >> 0x1) & 63)) ^ (hdl_u64)e) & 0x1;\n}\n\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "static hdl_u64 hdl_pack_gt(const hdl_u32 *g)\n{\nhdl_u64 r2 = 0x0;\nhdl_u32 l2;\nfor (l2 = 0x0; l2 < 64; l2++)\n{\nhdl_u32 e2 = g[l2];\nif (e2 != 4294967295) { r2 |= hdl_pack_bit(e2) << l2; }\n}\nreturn r2;\n}\n\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "static hdl_u64 hdl_pack_ext(const hdl_u32 *g, hdl_u32 n2)\n{\nhdl_u64 r2 = 0x0;\nhdl_u32 l2;\nfor (l2 = 0x0; l2 < n2; l2++) { r2 |= hdl_pack_bit(g[l2]) << l2; }\nreturn r2;\n}\n\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_pack_str_emit(ctx, "#ifdef HDL_GEN_MAIN\n#include <stdio.h>\nstatic hdl_u32 hdl_pack_ck(void)\n{\nhdl_u32 i;\nhdl_u32 bad = 0x0;\nfor (i = 0x0; i < HDL_PACK_CK_CUNT; i++)\n{\nhdl_u64 want = hdl_pack_bit(ckt[i * 3]) & hdl_pack_bit(ckt[i * 3 + 0x1]);\nhdl_u32 p2 = ckt[i * 3 + 0x2];\nhdl_u64 got = (nw[p2 >> 6] >> (p2 & 63)) & 0x1;\nif (want != got)\n{\nprintf(\"ck fail; node slot: %u; pos: %u; word: %u; lane: %u; f0: %u; f1: %u\\n\", i, p2, p2 >> 6, p2 & 63, ckt[i * 3], ckt[i * 3 + 0x1]);\nbad = 0x1;\n}\n}\nreturn bad;\n}\n#endif\n\n");
}

static uint32_t crosstalk_hdl_pack_iface_emit(crosstalk_hdl_pack_ctx_t *ctx, uint32_t clk_pl, const uint64_t *init_word, uint32_t init_word_cunt)
{
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || init_word == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "static hdl_u32 hdl_gen_step(void)\n{\nhdl_u32 rr;\nhdl_u64 cv;\nrr = hdl_pack_comb();\nif (rr != 0x0) { return rr; }\ncv = ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (clk_pl != UINT32_MAX)
    {
        ret = crosstalk_hdl_pack_str_emit(ctx, "hdl_pack_bit(");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_dec_emit(ctx, clk_pl);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_str_emit(ctx, ")");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    else { ret = crosstalk_hdl_pack_str_emit(ctx, "0x0"); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, ";\nif (pc2 == 0x0 && cv != 0x0)\n{\nhdl_pack_commit();\nrr = hdl_pack_comb();\nif (rr != 0x0) { return rr; }\n}\npc2 = cv;\npkd = 0x0;\nreturn 0x0;\n}\n\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "static hdl_u32 hdl_gen_cycle(hdl_u32 ck)\n{\nhdl_u32 rr;\nif (ck >= HDL_GEN_SIG_CUNT) { return 0x5; }\nif (pkd != 0x0)\n{\nrr = hdl_pack_comb();\nif (rr != 0x0) { return rr; }\npkd = 0x0;\n}\nhdl_pack_commit();\npc2 = 0x0;\nreturn hdl_pack_comb();\n}\n\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "static void hdl_gen_init(void)\n{\npkd = 0x1;\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < init_word_cunt; idx = idx + 0x1)
    {
        if (init_word[idx] == 0) { continue; }

        ret = crosstalk_hdl_pack_str_emit(ctx, "nw[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_dec_emit(ctx, idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_str_emit(ctx, "] |= ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_u64_emit(ctx, init_word[idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_str_emit(ctx, ";\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (idx = 0; idx < ctx->ir_ctx->mem_init_cunt; idx = idx + 0x1)
    {
        const crosstalk_hdl_ir_mem_init_t *init = &ctx->ir_ctx->mem_inits[idx];

        if (init->sig >= ctx->ir_ctx->sig_cunt || init->addr >= (uint64_t)ctx->ir_ctx->sigs[init->sig].depth) { return LDG_ERR_BOUNDS; }

        ret = crosstalk_hdl_pack_str_emit(ctx, "m[mo[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_dec_emit(ctx, init->sig);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_str_emit(ctx, "] + ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_u64_emit(ctx, init->addr);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_str_emit(ctx, "] = ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_u64_emit(ctx, init->val);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_str_emit(ctx, " & mk[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_dec_emit(ctx, init->sig);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_str_emit(ctx, "];\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_pack_str_emit(ctx, "}\n\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "static hdl_u32 hdl_gen_sig_find(const char *snm)\n{\nhdl_u32 i;\nfor (i = 0x0; i < HDL_GEN_SIG_CUNT; i++) { if (nmt[i][0x0] != 0x0 && strcmp(nmt[i], snm) == 0x0) { return i; } }\nreturn 4294967295;\n}\n\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "static void hdl_gen_poke(hdl_u32 i, hdl_u64 pv)\n{\nhdl_u32 b2;\nhdl_u32 e2;\nif (i >= HDL_GEN_SIG_CUNT || slo[i] == 4294967295) { return; }\nfor (b2 = 0x0; b2 < wd2[i]; b2++)\n{\ne2 = sl[slo[i] + b2];\nif (e2 == 4294967295 || (e2 >> 0x1) == 0x0) { continue; }\nnw[e2 >> 7] = (nw[e2 >> 7] & ~((hdl_u64)0x1 << ((e2 >> 0x1) & 63))) | ((((pv >> b2) ^ (hdl_u64)e2) & 0x1) << ((e2 >> 0x1) & 63));\n}\npkd = 0x1;\n}\n\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "static hdl_u64 hdl_gen_peek(hdl_u32 i)\n{\nhdl_u64 r2 = 0x0;\nhdl_u32 b2;\nhdl_u32 e2;\nif (i >= HDL_GEN_SIG_CUNT || slo[i] == 4294967295) { return 0x0; }\nfor (b2 = 0x0; b2 < wd2[i]; b2++)\n{\ne2 = sl[slo[i] + b2];\nif (e2 == 4294967295) { continue; }\nr2 |= hdl_pack_bit(e2) << b2;\n}\nreturn r2;\n}\n\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "static void hdl_gen_mem_st(hdl_u32 i, hdl_u64 ad, hdl_u64 sv)\n{\nif (i < HDL_GEN_SIG_CUNT && dp[i] > 0x0 && ad < (hdl_u64)dp[i]) { m[mo[i] + ad] = sv & mk[i]; pkd = 0x1; }\n}\n\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_pack_str_emit(ctx, "static hdl_u64 hdl_gen_mem_ld(hdl_u32 i, hdl_u64 ad)\n{\nif (i < HDL_GEN_SIG_CUNT && dp[i] > 0x0 && ad < (hdl_u64)dp[i]) { return m[mo[i] + ad]; }\nreturn 0x0;\n}\n\n");
}

static uint32_t crosstalk_hdl_pack_export_emit(crosstalk_hdl_pack_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "#ifndef HDL_GEN_MAIN\n#if defined(_WIN32)\n#define HDL_ABI_EXPORT __declspec(dllexport)\n#else\n#define HDL_ABI_EXPORT __attribute__((visibility(\"default\")))\n#endif\n\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "void *memcpy(void *dd, const void *ss, " "siz" "e_t nn)\n{\n" "unsig" "ned char *d2 = (" "unsig" "ned char *)dd;\nconst " "unsig" "ned char *s2 = (const " "unsig" "ned char *)ss;\n" "siz" "e_t i2 = 0;\n" "siz" "e_t nw = nn & ~(" "siz" "e_t)7;\nwhile (i2 < nw) { *(hdl_u64 *)(d2 + i2) = *(const hdl_u64 *)(s2 + i2); i2 += 8; }\nwhile (i2 < nn) { d2[i2] = s2[i2]; i2++; }\nreturn dd;\n}\n\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "void *memset(void *dd, " "in" "t cc, " "siz" "e_t nn)\n{\n" "unsig" "ned char *d2 = (" "unsig" "ned char *)dd;\nhdl_u64 wv = (hdl_u64)(" "unsig" "ned char)cc * 0x0101010101010101;\n" "siz" "e_t i2 = 0;\n" "siz" "e_t nw = nn & ~(" "siz" "e_t)7;\nwhile (i2 < nw) { *(hdl_u64 *)(d2 + i2) = wv; i2 += 8; }\nwhile (i2 < nn) { d2[i2] = (" "unsig" "ned char)cc; i2++; }\nreturn dd;\n}\n\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "in" "t strcmp(const char *aa, const char *bb)\n{\n" "siz" "e_t i2 = 0;\nwhile (aa[i2] != 0 && aa[i2] == bb[i2]) { i2++; }\nreturn (" "in" "t)((" "unsig" "ned char)aa[i2] - (" "unsig" "ned char)bb[i2]);\n}\n\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "HDL_ABI_EXPORT hdl_u32 hdl_abi_rev(void) { return 0x1; }\n\nHDL_ABI_EXPORT void hdl_abi_init(void) { hdl_gen_init(); }\n\nHDL_ABI_EXPORT hdl_u32 hdl_abi_cycle(hdl_u32 ck) { return hdl_gen_cycle(ck); }\n\nHDL_ABI_EXPORT hdl_u32 hdl_abi_sig_find(const char *snm) { return hdl_gen_sig_find(snm); }\n\nHDL_ABI_EXPORT void hdl_abi_poke(hdl_u32 i2, hdl_u64 pv) { hdl_gen_poke(i2, pv); }\n\nHDL_ABI_EXPORT hdl_u64 hdl_abi_peek(hdl_u32 i2) { return hdl_gen_peek(i2); }\n\nHDL_ABI_EXPORT void hdl_abi_mem_st(hdl_u32 i2, hdl_u64 ad, hdl_u64 sv) { hdl_gen_mem_st(i2, ad, sv); }\n\nHDL_ABI_EXPORT hdl_u64 hdl_abi_mem_ld(hdl_u32 i2, hdl_u64 ad) { return hdl_gen_mem_ld(i2, ad); }\n\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_pack_str_emit(ctx, "HDL_ABI_EXPORT hdl_u32 hdl_abi_state_get(hdl_u32 kd, hdl_u64 **ob, hdl_u32 *oc)\n{\nif (ob == 0x0 || oc == 0x0) { return 0x4; }\n*ob = 0x0;\n*oc = 0x0;\nif (kd == 0x0) { *ob = nw; *oc = HDL_GEN_NW_CUNT; return 0x0; }\nif (kd == 0x1) { *ob = m; *oc = HDL_GEN_MEM_WORDS; return 0x0; }\nif (kd == 0x2) { *ob = &pc2; *oc = 0x1; return 0x0; }\nreturn 0x4;\n}\n#endif\n\n");
}

static uint32_t crosstalk_hdl_pack_harness_emit(crosstalk_hdl_pack_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "#ifdef HDL_GEN_MAIN\n#include <stdio.h>\n#include <stdlib.h>\n#include <inttypes.h>\n#include <termios.h>\n#include <unistd.h>\n#include <fcntl.h>\n\n#ifndef HDL_GEN_CYCLES\n#define HDL_GEN_CYCLES 8000000\n#endif\n\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "static const hdl_u32 hdl_scan_tbl[128] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 14, 15, 28, 0x0, 0x0, 28, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 57, 0x2, 40, 0x4, 0x5, 0x6, 0x8, 40, 10, 11, 0x9, 13, 51, 12, 52, 53, 11, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 10, 39, 39, 51, 13, 52, 53, 0x3, 30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38, 50, 49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44, 26, 43, 27, 0x7, 12, 41, 30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38, 50, 49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44, 26, 43, 27, 41, 14};\n\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "static void hdl_gen_fb_dump(hdl_u32 fmi, hdl_u32 fpi, hdl_u64 fba, hdl_u32 fbn)\n{\nFILE *ff2;\nchar fnb[64];\nchar pxb[3];\nhdl_u32 px2;\nhdl_u64 cvl;\nsnprintf(fnb, sizeof(fnb), \"fb_%06\" \"u.ppm\", fbn);\nff2 = fopen(fnb, \"wb\");\nif (ff2 == 0x0) { return; }\nfprintf(ff2, \"P6\\n320 200\\n255\\n\");\nfor (px2 = 0x0; px2 < 64000; px2++)\n{\ncvl = hdl_gen_mem_ld(fmi, fba + px2) & 255;\npxb[0x0] = (char)((hdl_gen_mem_ld(fpi, cvl * 0x3) & 63) << 0x2);\npxb[0x1] = (char)((hdl_gen_mem_ld(fpi, cvl * 0x3 + 0x1) & 63) << 0x2);\npxb[0x2] = (char)((hdl_gen_mem_ld(fpi, cvl * 0x3 + 0x2) & 63) << 0x2);\nfwrite(pxb, 0x1, 0x3, ff2);\n}\nfclose(ff2);\n}\n\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "int main(int argc, char **argv)\n{\nFILE *hf;\nFILE *kfp = 0x0;\nhdl_i64 gc;\nhdl_i64 knb = 0x0;\nhdl_u64 kpv = 0x0;\nhdl_u64 kav = 0x0;\nhdl_u64 kgp = 0x0;\nhdl_u64 sca = 0x0;\nhdl_u64 chv = 0x0;\nhdl_u32 scn = 0x0;\nhdl_u32 kcs = 4294967295;\nhdl_u32 krs = 4294967295;\nhdl_u32 kas = 4294967295;\nhdl_u32 ai2;\nhdl_u32 mi2;\nhdl_u32 rw2;\nhdl_u32 co2;\nchar pch;\nFILE *stf = 0x0;\nconst char *svp = 0x0;\nconst char *ldp = 0x0;\nhdl_u32 sdn = 0x0;\nhdl_u32 rawmode = 0x0;\nhdl_u32 kq2[64];\nhdl_u32 kqh = 0x0;\nhdl_u32 kqt = 0x0;\nhdl_u64 srn = 0x0;\nhdl_u32 cyc_lim = HDL_GEN_CYCLES;\nhdl_u64 fba = 655360;\nhdl_u32 fbe = 0x0;\nhdl_u32 fbf = 0x0;\nhdl_u32 fbn = 0x0;\nhdl_u32 fmi = 4294967295;\nhdl_u32 fpi = 4294967295;\nstruct termios tio_o;\nstruct termios tio_r;\nhdl_u64 wv = 0x0;\nhdl_u64 aw = 0x0;\nhdl_u64 sp = 0x0;\nhdl_u64 sv = 0x0;\nhdl_u64 lp = 0x0;\nhdl_u64 lv = 0x0;\nhdl_u64 lo = 0x0;\nhdl_u32 dc = 0x0;\nhdl_u32 hj;\nhdl_u32 am;\nhdl_u64 sw;\nhdl_u32 ck;\nhdl_u32 rs;\nhdl_u32 se;\nhdl_u32 le;\nhdl_u32 lc;\nhdl_u32 hb;\nhdl_u32 cy;\nhdl_u32 rr;\nconst char *wnm[8];\nhdl_u32 wsg[8];\nhdl_u64 wpv[8];\nhdl_u64 wcv;\nhdl_u32 wct = 0x0;\nhdl_u32 wi2;\nhdl_u32 dmp = 0x0;\nhdl_u32 ckf = 0x0;\nhdl_gen_init();\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "for (ai2 = 0x1; (hdl_u32)argc > ai2; ai2++)\n{\nif (strcmp(argv[ai2], \"-stdin\") == 0x0) { sdn = 0x1; }\nelse if ((hdl_u32)argc > ai2 + 0x1 && strcmp(argv[ai2], \"-keys\") == 0x0) { kfp = fopen(argv[ai2 + 0x1], \"rb\"); if (kfp == 0x0) { printf(\"keys file missing: %s\\n\", argv[ai2 + 0x1]); } ai2++; }\nelse if ((hdl_u32)argc > ai2 + 0x1 && strcmp(argv[ai2], \"-screen\") == 0x0) { sca = strtoull(argv[ai2 + 0x1], 0x0, 0x0); scn = 0x1; ai2++; }\nelse if ((hdl_u32)argc > ai2 + 0x1 && strcmp(argv[ai2], \"-save\") == 0x0) { svp = argv[ai2 + 0x1]; ai2++; }\nelse if ((hdl_u32)argc > ai2 + 0x1 && strcmp(argv[ai2], \"-load\") == 0x0) { ldp = argv[ai2 + 0x1]; ai2++; }\nelse if ((hdl_u32)argc > ai2 + 0x1 && strcmp(argv[ai2], \"-cycles\") == 0x0) { cyc_lim = (hdl_u32)strtoull(argv[ai2 + 0x1], 0x0, 0x0); ai2++; }\nelse if ((hdl_u32)argc > ai2 + 0x1 && strcmp(argv[ai2], \"-fb\") == 0x0) { fba = strtoull(argv[ai2 + 0x1], 0x0, 16); fbf = 0x1; ai2++; }\nelse if ((hdl_u32)argc > ai2 + 0x1 && strcmp(argv[ai2], \"-fbevery\") == 0x0) { fbe = (hdl_u32)strtoull(argv[ai2 + 0x1], 0x0, 0x0); fbf = 0x1; ai2++; }\nelse if ((hdl_u32)argc > ai2 + 0x1 && strcmp(argv[ai2], \"-watch\") == 0x0) { if (wct < 8) { wnm[wct] = argv[ai2 + 0x1]; wct++; } ai2++; }\nelse if (strcmp(argv[ai2], \"-dump\") == 0x0) { dmp = 0x1; }\nelse if (strcmp(argv[ai2], \"-ck\") == 0x0) { ckf = 0x1; }\n}\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "for (hj = 0x0; hj < HDL_GEN_HEX_CUNT && ldp == 0x0; hj++)\n{\nhf = fopen(hx_path[hj], \"rb\");\nif (hf == 0x0) { printf(\"hex img missing: %s\\n\", hx_path[hj]); continue; }\naw = 0x0;\nwv = 0x0;\ndc = 0x0;\nam = 0x0;\nsw = 0x0;\ngc = (hdl_i64)fgetc(hf);\nwhile (gc != (hdl_i64)EOF)\n{\nchar hc = (char)gc;\nhdl_u32 hx_is = 0x0;\nhdl_u64 nb = 0x0;\nif (hc >= 0x30 && hc <= 0x39) { nb = (hdl_u64)(hdl_u32)(hc - 0x30); hx_is = 0x1; }\nelse if (hc >= 0x61 && hc <= 0x66) { nb = (hdl_u64)(hdl_u32)(hc - 0x61) + 10; hx_is = 0x1; }\nelse if (hc >= 0x41 && hc <= 0x46) { nb = (hdl_u64)(hdl_u32)(hc - 0x41) + 10; hx_is = 0x1; }\nif (hx_is != 0x0) { wv = wv * 16 + nb; dc++; }\nelse if (dc > 0x0)\n{\nif (am != 0x0) { aw = wv; am = 0x0; }\nelse { hdl_gen_mem_st(hx_sig[hj], aw, wv); aw++; sw++; }\nwv = 0x0;\ndc = 0x0;\n}\nif (hc == 0x40) { am = 0x1; }\ngc = (hdl_i64)fgetc(hf);\n}\nif (dc > 0x0 && am == 0x0) { hdl_gen_mem_st(hx_sig[hj], aw, wv); sw++; }\nfclose(hf);\nprintf(\"hex img loaded; words: %\" PRIu64 \"\\n\", sw);\n}\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "ck = hdl_gen_sig_find(\"clk\");\nif (ck == 4294967295) { printf(\"clk sig missing\\n\"); return 0x1; }\nrs = hdl_gen_sig_find(\"btnrst\");\nif (rs == 4294967295) { printf(\"btnrst sig missing\\n\"); return 0x1; }\nse = hdl_gen_sig_find(\"serial_out\");\nif (se == 4294967295) { printf(\"serial_out sig missing\\n\"); }\nle = hdl_gen_sig_find(\"led_out\");\nif (le == 4294967295) { printf(\"led_out sig missing\\n\"); }\nlc = hdl_gen_sig_find(\"loc\");\nif (lc == 4294967295) { printf(\"loc sig missing\\n\"); }\nhb = hdl_gen_sig_find(\"hlt_out\");\nif (hb == 4294967295) { printf(\"hlt_out sig missing\\n\"); }\nfor (wi2 = 0x0; wi2 < wct; wi2++)\n{\nwsg[wi2] = hdl_gen_sig_find(wnm[wi2]);\nwpv[wi2] = 0x0;\nif (wsg[wi2] == 4294967295) { printf(\"watch sig missing: %s\\n\", wnm[wi2]); }\n}\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "if (ldp != 0x0)\n{\nstf = fopen(ldp, \"rb\");\nif (stf == 0x0) { printf(\"state file missing: %s\\n\", ldp); return 0x1; }\nsrn = srn + (hdl_u64)fread(nw, sizeof(hdl_u64), HDL_GEN_NW_CUNT, stf);\nsrn = srn + (hdl_u64)fread(m, sizeof(hdl_u64), HDL_GEN_MEM_WORDS, stf);\nsrn = srn + (hdl_u64)fread(&pc2, sizeof(hdl_u64), 0x1, stf);\nfclose(stf);\npkd = 0x0;\nprintf(\"state ld; items: %\" PRIu64 \"\\n\", srn);\n}\nelse\n{\nhdl_gen_poke(rs, 0x0);\nfor (cy = 0x0; cy < 8; cy++)\n{\nhdl_gen_poke(ck, 0x1);\nrr = hdl_gen_step();\nif (rr != 0x0) { printf(\"rst cycle failed; err: %u\\n\", rr); return 0x1; }\nhdl_gen_poke(ck, 0x0);\nrr = hdl_gen_step();\nif (rr != 0x0) { printf(\"rst cycle failed; err: %u\\n\", rr); return 0x1; }\n}\nhdl_gen_poke(rs, 0x1);\n}\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "if (fbf != 0x0)\n{\nfor (mi2 = 0x0; mi2 < HDL_GEN_SIG_CUNT; mi2++) { if (dp[mi2] > 0x0 && strcmp(nmt[mi2], \"mem\") == 0x0) { fmi = mi2; break; } }\nfor (mi2 = 0x0; mi2 < HDL_GEN_SIG_CUNT; mi2++) { if (dp[mi2] > 0x0 && strcmp(nmt[mi2], \"vga_pal\") == 0x0) { fpi = mi2; break; } }\nif (fmi == 4294967295 || fpi == 4294967295) { fbf = 0x0; fbe = 0x0; printf(\"fb sigs missing\\n\"); }\n}\nif (kfp != 0x0 || sdn != 0x0)\n{\nkcs = hdl_gen_sig_find(\"key_code\");\nkrs = hdl_gen_sig_find(\"key_rdy\");\nkas = hdl_gen_sig_find(\"key_ack\");\nif (kcs == 4294967295 || krs == 4294967295 || kas == 4294967295) { if (kfp != 0x0) { fclose(kfp); } kfp = 0x0; sdn = 0x0; printf(\"key sigs missing\\n\"); }\n}\nif (kfp != 0x0)\n{\nknb = (hdl_i64)fgetc(kfp);\nif (knb != (hdl_i64)EOF) { hdl_gen_poke(kcs, (hdl_u64)knb); hdl_gen_poke(krs, 0x1); }\n}\nif (sdn != 0x0)\n{\ntcgetattr(0x0, &tio_o);\ntio_r = tio_o;\ntio_r.c_lflag = tio_r.c_lflag & (hdl_u32)~(ICANON | ECHO);\ntio_r.c_cc[VMIN] = 0x0;\ntio_r.c_cc[VTIME] = 0x0;\ntcsetattr(0x0, TCSANOW, &tio_r);\nfcntl(0x0, F_SETFL, fcntl(0x0, F_GETFL) | O_NONBLOCK);\nrawmode = 0x1;\n}\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "for (cy = 0x0; cy < cyc_lim; cy++)\n{\nrr = hdl_gen_cycle(ck);\nif (rr != 0x0) { printf(\"cycle %u failed; err: %u\\n\", cy, rr); return 0x1; }\nif (ckf != 0x0 && hdl_pack_ck() != 0x0) { printf(\"ck fail; cycle: %u\\n\", cy); return 0x1; }\nif (sdn != 0x0 && (cy & 4095) == 0x0)\n{\nchar rb2;\nwhile (read(0x0, &rb2, 0x1) == 0x1)\n{\nhdl_u32 uc2 = ((hdl_u32)rb2) & 255;\nhdl_u32 sft = 0x0;\nif (uc2 == 29) { sdn = 0x0; cy = cyc_lim - 0x1; break; }\nif (uc2 == 127) { uc2 = 0x8; }\nif (uc2 >= 128 || hdl_scan_tbl[uc2] == 0x0) { continue; }\nif (uc2 >= 65 && uc2 <= 90) { sft = 0x1; }\nelse if (strchr(\"!@#$%^&*()_+{}:\\\"~|<>?\", (char)uc2) != 0x0) { sft = 0x1; }\nif (((kqt + 0x4) & 63) == kqh || ((kqt + 0x5) & 63) == kqh) { break; }\nif (sft != 0x0) { kq2[kqt] = 42; kqt = (kqt + 0x1) & 63; }\nkq2[kqt] = hdl_scan_tbl[uc2]; kqt = (kqt + 0x1) & 63;\nkq2[kqt] = hdl_scan_tbl[uc2] | 128; kqt = (kqt + 0x1) & 63;\nif (sft != 0x0) { kq2[kqt] = 170; kqt = (kqt + 0x1) & 63; }\n}\n}\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "if (kfp != 0x0 || sdn != 0x0)\n{\nkav = hdl_gen_peek(kas);\nif (kav != 0x0 && kpv == 0x0)\n{\nhdl_gen_poke(krs, 0x0);\nif (kfp != 0x0) { knb = (hdl_i64)fgetc(kfp); }\nelse if (kqh != kqt) { kqh = (kqh + 0x1) & 63; }\nkgp = 64;\n}\nkpv = kav;\nif (kgp > 0x0) { kgp--; }\nif (kgp == 0x0 && hdl_gen_peek(krs) == 0x0)\n{\nif (kfp != 0x0 && knb != (hdl_i64)EOF) { hdl_gen_poke(kcs, (hdl_u64)knb); hdl_gen_poke(krs, 0x1); }\nelse if (sdn != 0x0 && kqh != kqt) { hdl_gen_poke(kcs, (hdl_u64)kq2[kqh]); hdl_gen_poke(krs, 0x1); }\n}\n}\nif (se != 4294967295)\n{\nsv = hdl_gen_peek(se);\nif (sv != sp)\n{\nif (sv != 0x0) { printf(\"%c\", (char)sv); }\nsp = sv;\n}\n}\nif (le != 4294967295)\n{\nlv = hdl_gen_peek(le);\nif (lv != lp)\n{\nprintf(\"led: %\" PRIX64 \"; cycle: %u\\n\", lv, cy);\nlp = lv;\n}\n}\nfor (wi2 = 0x0; wi2 < wct; wi2++)\n{\nif (wsg[wi2] == 4294967295) { continue; }\nwcv = hdl_gen_peek(wsg[wi2]);\nif (wcv != wpv[wi2])\n{\nprintf(\"w %s: %\" PRIX64 \"; cycle: %u\\n\", wnm[wi2], wcv, cy);\nwpv[wi2] = wcv;\n}\n}\nif (fbe != 0x0 && (cy % fbe) == 0x0) { hdl_gen_fb_dump(fmi, fpi, fba, fbn); fbn++; }\nif (hb != 4294967295 && hdl_gen_peek(hb) != 0x0) { printf(\"\\nhlt; cycle: %u\", cy); break; }\n}\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "if (rawmode != 0x0) { tcsetattr(0x0, TCSANOW, &tio_o); }\nif (svp != 0x0)\n{\nstf = fopen(svp, \"wb\");\nif (stf == 0x0) { printf(\"state save failed: %s\\n\", svp); }\nelse\n{\nsrn = 0x0;\nsrn = srn + (hdl_u64)fwrite(nw, sizeof(hdl_u64), HDL_GEN_NW_CUNT, stf);\nsrn = srn + (hdl_u64)fwrite(m, sizeof(hdl_u64), HDL_GEN_MEM_WORDS, stf);\nsrn = srn + (hdl_u64)fwrite(&pc2, sizeof(hdl_u64), 0x1, stf);\nfclose(stf);\nprintf(\"\\nstate saved; items: %\" PRIu64 \"\\n\", srn);\n}\n}\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_pack_str_emit(ctx, "if (scn != 0x0)\n{\nfor (mi2 = 0x0; mi2 < HDL_GEN_SIG_CUNT; mi2++) { if (dp[mi2] > 0x0 && strcmp(nmt[mi2], \"mem\") == 0x0) { break; } }\nif (mi2 < HDL_GEN_SIG_CUNT)\n{\nprintf(\"\\nscreen; cycle: %u\\n\", cy);\nfor (rw2 = 0x0; rw2 < 25; rw2++)\n{\nprintf(\"|\");\nfor (co2 = 0x0; co2 < 80; co2++)\n{\nchv = hdl_gen_mem_ld(mi2, sca + ((hdl_u64)rw2 * 80 + co2) * 0x2);\npch = (char)(chv & 255);\nif (pch < 32 || pch > 126) { pch = 46; }\nprintf(\"%c\", pch);\n}\nprintf(\"\\n\");\n}\n}\n}\nif (fbf != 0x0) { hdl_gen_fb_dump(fmi, fpi, fba, fbn); fbn++; }\nif (dmp != 0x0)\n{\nprintf(\"\\n\");\nfor (mi2 = 0x0; mi2 < HDL_GEN_SIG_CUNT; mi2++)\n{\nif (nmt[mi2][0x0] == 0x0 || dp[mi2] != 0x0) { continue; }\nprintf(\"d %u %s: %\" PRIX64 \"\\n\", mi2, nmt[mi2], hdl_gen_peek(mi2));\n}\nfor (mi2 = 0x0; mi2 < HDL_GEN_SIG_CUNT; mi2++)\n{\nif (nmt[mi2][0x0] == 0x0 || dp[mi2] == 0x0 || dp[mi2] > 256) { continue; }\nfor (aw = 0x0; aw < (hdl_u64)dp[mi2]; aw++) { printf(\"d %u %s %\" PRIu64 \": %\" PRIX64 \"\\n\", mi2, nmt[mi2], aw, hdl_gen_mem_ld(mi2, aw)); }\n}\n}\nprintf(\"\\nrun done; cycles: %u\", cy);\nif (lc != 4294967295)\n{\nlo = hdl_gen_peek(lc);\nprintf(\"; loc: %\" PRIX64, lo);\n}\nprintf(\"\\n\");\nreturn 0x0;\n}\n#endif\n");
}

static uint32_t crosstalk_hdl_pack_u32_arr_emit(crosstalk_hdl_pack_ctx_t *ctx, const char *decl, const uint32_t *vals, uint32_t cunt)
{
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || decl == 0x0 || vals == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_pack_str_emit(ctx, decl);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < cunt; idx = idx + 0x1)
    {
        if (idx > 0)
        {
            ret = crosstalk_hdl_pack_str_emit(ctx, ", ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_pack_dec_emit(ctx, vals[idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return crosstalk_hdl_pack_str_emit(ctx, "};\n");
}

static uint32_t crosstalk_hdl_pack_head_emit(crosstalk_hdl_pack_ctx_t *ctx, const uint32_t *mem_off, uint64_t mem_words, const uint32_t *lt_pairs, uint32_t latch_cunt)
{
    const crosstalk_hdl_ir_t *ir_ctx = 0x0;
    const crosstalk_hdl_aig_ctx_t *aig_ctx = 0x0;
    uint32_t hex_cunt = 0;
    uint32_t emitted = 0;
    uint32_t bit_idx = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || mem_off == 0x0 || lt_pairs == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ir_ctx = ctx->ir_ctx;
    aig_ctx = ctx->aig_ctx;
    for (idx = 0; idx < ir_ctx->sig_cunt; idx = idx + 0x1) { if (ir_ctx->sigs[idx].depth > 0 && ir_ctx->sigs[idx].mem_file.len > 0x2) { hex_cunt = hex_cunt + 0x1; } }

    ret = crosstalk_hdl_pack_str_emit(ctx, "#include <stdint.h>\n#ifdef HDL_GEN_MAIN\n#include <string.h>\n#else\n#include <stddef.h>\n" "in" "t strcmp(const char *aa, const char *bb);\nvoid *memcpy(void *dd, const void *ss, " "siz" "e_t nn);\nvoid *memset(void *dd, " "in" "t cc, " "siz" "e_t nn);\n#endif\n\ntypedef uint64_t hdl_u64;\ntypedef uint32_t hdl_u32;\ntypedef int64_t hdl_i64;\n\n#define HDL_GEN_SIG_CUNT ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_dec_emit(ctx, ir_ctx->sig_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "\n#define HDL_GEN_HEX_CUNT ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_dec_emit(ctx, hex_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "\n#define HDL_GEN_MEM_WORDS ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_dec_emit(ctx, mem_words);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "\n#define HDL_GEN_NW_CUNT ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_dec_emit(ctx, ctx->word_cunt + 0x1);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "\n\nstatic hdl_u64 nw[HDL_GEN_NW_CUNT];\nstatic hdl_u64 m[HDL_GEN_MEM_WORDS];\nstatic hdl_u64 pc2;\nstatic hdl_u32 pkd;\n\nstatic const hdl_u64 mk[HDL_GEN_SIG_CUNT] = {");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < ir_ctx->sig_cunt; idx = idx + 0x1)
    {
        if (idx > 0)
        {
            ret = crosstalk_hdl_pack_str_emit(ctx, ", ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_pack_u64_emit(ctx, crosstalk_hdl_pack_mask_get(ir_ctx->sigs[idx].width));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_pack_str_emit(ctx, "};\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "static const hdl_u32 dp[HDL_GEN_SIG_CUNT] = {");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < ir_ctx->sig_cunt; idx = idx + 0x1)
    {
        if (idx > 0)
        {
            ret = crosstalk_hdl_pack_str_emit(ctx, ", ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_pack_dec_emit(ctx, ir_ctx->sigs[idx].depth);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_pack_str_emit(ctx, "};\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_u32_arr_emit(ctx, "static const hdl_u32 mo[HDL_GEN_SIG_CUNT] = {", mem_off, ir_ctx->sig_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "static const hdl_u32 wd2[HDL_GEN_SIG_CUNT] = {");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < ir_ctx->sig_cunt; idx = idx + 0x1)
    {
        if (idx > 0)
        {
            ret = crosstalk_hdl_pack_str_emit(ctx, ", ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_pack_dec_emit(ctx, ir_ctx->sigs[idx].width);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_pack_str_emit(ctx, "};\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "static const hdl_u32 slo[HDL_GEN_SIG_CUNT] = {");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < ir_ctx->sig_cunt; idx = idx + 0x1)
    {
        if (idx > 0)
        {
            ret = crosstalk_hdl_pack_str_emit(ctx, ", ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_pack_dec_emit(ctx, aig_ctx->lit_off[idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_pack_str_emit(ctx, "};\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "static const hdl_u32 sl[] = {");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    emitted = 0;
    for (idx = 0; idx < ir_ctx->sig_cunt; idx = idx + 0x1)
    {
        if (aig_ctx->lit_off[idx] == UINT32_MAX) { continue; }

        for (bit_idx = 0; bit_idx < ir_ctx->sigs[idx].width; bit_idx = bit_idx + 0x1)
        {
            if (emitted > 0)
            {
                ret = crosstalk_hdl_pack_str_emit(ctx, ", ");
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            ret = crosstalk_hdl_pack_dec_emit(ctx, crosstalk_hdl_pack_pl_get(ctx, aig_ctx->lit_arena[aig_ctx->lit_off[idx] + bit_idx]));
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            emitted = emitted + 0x1;
        }
    }

    if (emitted == 0)
    {
        ret = crosstalk_hdl_pack_str_emit(ctx, "0x0");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_pack_str_emit(ctx, "};\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "static const char *const nmt[HDL_GEN_SIG_CUNT] = {");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < ir_ctx->sig_cunt; idx = idx + 0x1)
    {
        if (idx > 0)
        {
            ret = crosstalk_hdl_pack_str_emit(ctx, ", ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_pack_str_emit(ctx, "\"");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_name_emit(ctx, idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_str_emit(ctx, "\"");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_pack_str_emit(ctx, "};\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "static const hdl_u32 hx_sig[] = {");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    emitted = 0;
    for (idx = 0; idx < ir_ctx->sig_cunt; idx = idx + 0x1)
    {
        if (ir_ctx->sigs[idx].depth == 0 || ir_ctx->sigs[idx].mem_file.len <= 0x2) { continue; }

        if (emitted > 0)
        {
            ret = crosstalk_hdl_pack_str_emit(ctx, ", ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_pack_dec_emit(ctx, idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        emitted = emitted + 0x1;
    }

    if (emitted == 0)
    {
        ret = crosstalk_hdl_pack_dec_emit(ctx, UINT32_MAX);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_pack_str_emit(ctx, "};\nstatic const char *const hx_path[] = {");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    emitted = 0;
    for (idx = 0; idx < ir_ctx->sig_cunt; idx = idx + 0x1)
    {
        if (ir_ctx->sigs[idx].depth == 0 || ir_ctx->sigs[idx].mem_file.len <= 0x2) { continue; }

        if (emitted > 0)
        {
            ret = crosstalk_hdl_pack_str_emit(ctx, ", ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_pack_str_emit(ctx, "\"");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_bytes_emit(ctx, ctx->src + ir_ctx->sigs[idx].mem_file.off + 0x1, ir_ctx->sigs[idx].mem_file.len - 0x2);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_str_emit(ctx, "\"");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        emitted = emitted + 0x1;
    }

    if (emitted == 0)
    {
        ret = crosstalk_hdl_pack_str_emit(ctx, "\"\"");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_pack_str_emit(ctx, "};\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (latch_cunt > 0)
    {
        ret = crosstalk_hdl_pack_u32_arr_emit(ctx, "static const hdl_u32 lt[] = {", lt_pairs, latch_cunt * 0x2);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (ctx->gt_cunt > 0)
    {
        ret = crosstalk_hdl_pack_u32_arr_emit(ctx, "static const hdl_u32 gsl[] = {", ctx->gt_blob, ctx->gt_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    else
    {
        ret = crosstalk_hdl_pack_str_emit(ctx, "static const hdl_u32 gsl[] = {4294967295};\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    emitted = 0;
    for (idx = 0x1; idx < aig_ctx->node_cunt; idx = idx + 0x1) { if (aig_ctx->nodes[idx].fanin0 != UINT32_MAX) { emitted = emitted + 0x1; } }

    ret = crosstalk_hdl_pack_str_emit(ctx, "#define HDL_PACK_CK_CUNT ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_dec_emit(ctx, emitted);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_str_emit(ctx, "\nstatic const hdl_u32 ckt[] = {");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    emitted = 0;
    for (idx = 0x1; idx < aig_ctx->node_cunt; idx = idx + 0x1)
    {
        if (aig_ctx->nodes[idx].fanin0 == UINT32_MAX) { continue; }

        if (emitted > 0)
        {
            ret = crosstalk_hdl_pack_str_emit(ctx, ", ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_pack_dec_emit(ctx, crosstalk_hdl_pack_pl_get(ctx, aig_ctx->nodes[idx].fanin0));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_str_emit(ctx, ", ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_dec_emit(ctx, crosstalk_hdl_pack_pl_get(ctx, aig_ctx->nodes[idx].fanin1));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_str_emit(ctx, ", ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_pack_dec_emit(ctx, ctx->pos[idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        emitted = emitted + 0x1;
    }

    ret = crosstalk_hdl_pack_str_emit(ctx, "};\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_pack_str_emit(ctx, "\n");
}

uint32_t crosstalk_hdl_pack_run(crosstalk_hdl_pack_ctx_t *ctx, const crosstalk_hdl_aig_ctx_t *aig_ctx, const char *src, void *pool, char **out_src, uint32_t *out_len)
{
    const crosstalk_hdl_ir_t *ir_ctx = 0x0;
    char *body = 0x0;
    char *head = 0x0;
    uint64_t body_cap = 0;
    uint64_t head_cap = 0;
    uint64_t mem_words = 0;
    uint64_t lit_total = 0;
    uint32_t *node_pi = 0x0;
    uint32_t *bit_node = 0x0;
    uint32_t *kneg = 0x0;
    uint32_t *kp0 = 0x0;
    uint32_t *kp1 = 0x0;
    uint32_t *mem_off = 0x0;
    uint32_t *ff_slot_map = 0x0;
    uint32_t *ff_word_list = 0x0;
    uint32_t *ff_ent = 0x0;
    uint64_t *ff_mask = 0x0;
    uint32_t *lt_pairs = 0x0;
    uint32_t *latch_map = 0x0;
    uint64_t *init_word = 0x0;
    uint32_t body_len = 0;
    uint32_t word_cap = 0;
    uint32_t ff_slot_cap = 0;
    uint32_t clk_pl = UINT32_MAX;
    uint32_t root_cursor = 0;
    uint32_t pi_idx = 0;
    uint32_t bit_idx = 0;
    uint32_t node = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || aig_ctx == 0x0 || pool == 0x0 || out_src == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_src = 0x0;
    *out_len = 0;
    memset(ctx, 0x0, sizeof(*ctx));
    ctx->aig_ctx = aig_ctx;
    ctx->ir_ctx = aig_ctx->ir_ctx;
    ctx->src = src;
    ctx->pool = pool;
    ctx->wide_sig = UINT32_MAX;
    ir_ctx = ctx->ir_ctx;
    for (idx = 0; idx < ir_ctx->sig_cunt; idx = idx + 0x1)
    {
        if (ir_ctx->sigs[idx].width > CROSSTALK_HDL_PACK_LANE_CUNT)
        {
            ctx->wide_sig = idx;

            return CROSSTALK_HDL_ERR_PACK_WIDE;
        }

        if (aig_ctx->lit_off[idx] != UINT32_MAX)
        {
            uint64_t lit_end = (uint64_t)aig_ctx->lit_off[idx] + ir_ctx->sigs[idx].width;

            if (lit_end > lit_total) { lit_total = lit_end; }
        }
    }

    if (LDG_UNLIKELY(aig_ctx->residue_cunt > 0)) { return CROSSTALK_HDL_ERR_PACK; }

    for (idx = 0; idx < ir_ctx->seq_blk_cunt; idx = idx + 0x1)
    {
        if (LDG_UNLIKELY(ir_ctx->seq_blks[idx].clk_edge != CROSSTALK_HDL_IR_EDGE_POS)) { return CROSSTALK_HDL_ERR_PACK; }

        if (LDG_UNLIKELY(crosstalk_hdl_pack_clk_root_get(ir_ctx, ir_ctx->seq_blks[idx].clk_sig) != crosstalk_hdl_pack_clk_root_get(ir_ctx, ir_ctx->seq_blks[0].clk_sig))) { return CROSSTALK_HDL_ERR_PACK; }
    }

    ret = crosstalk_hdl_pack_arr_alloc(pool, aig_ctx->node_cunt, sizeof(uint32_t), (void **)&node_pi);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    memset(node_pi, CROSSTALK_HDL_PACK_BYTE_FILL, (uint64_t)aig_ctx->node_cunt * sizeof(uint32_t));
    for (pi_idx = 0; pi_idx < aig_ctx->pi_cunt; pi_idx = pi_idx + 0x1) { node_pi[aig_ctx->pi_node[pi_idx]] = pi_idx; }

    ret = crosstalk_hdl_pack_arr_alloc(pool, aig_ctx->node_cunt, sizeof(uint32_t), (void **)&ctx->lvl);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_arr_alloc(pool, aig_ctx->node_cunt, sizeof(uint32_t), (void **)&ctx->pos);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_lvl_build(ctx, node_pi);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_arr_alloc(pool, (uint64_t)ctx->level_max + 0x2, sizeof(uint32_t), (void **)&ctx->lvl_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_arr_alloc(pool, (uint64_t)ctx->level_max + 0x2, sizeof(uint32_t), (void **)&ctx->lvl_off);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_arr_alloc(pool, aig_ctx->node_cunt, sizeof(uint32_t), (void **)&ctx->ord);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_arr_alloc(pool, aig_ctx->node_cunt, sizeof(uint32_t), (void **)&kneg);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_arr_alloc(pool, aig_ctx->node_cunt, sizeof(uint32_t), (void **)&kp0);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_arr_alloc(pool, aig_ctx->node_cunt, sizeof(uint32_t), (void **)&kp1);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_arr_alloc(pool, (uint64_t)aig_ctx->node_cunt * 0x2, sizeof(uint32_t), (void **)&ctx->sort_buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < ir_ctx->comb_cunt + ir_ctx->seq_op_cunt; idx = idx + 0x1) { if (aig_ctx->op_root_off[idx] != UINT32_MAX) { ctx->mem_op_cunt = ctx->mem_op_cunt + 0x1; } }

    word_cap = aig_ctx->node_cunt / CROSSTALK_HDL_PACK_LANE_CUNT + 0x2 * (ctx->level_max + 0x2) + ctx->mem_op_cunt * 0x2 + aig_ctx->pi_cunt / CROSSTALK_HDL_PACK_LANE_CUNT + 0x10;
    ret = crosstalk_hdl_pack_arr_alloc(pool, word_cap, sizeof(uint32_t), (void **)&ctx->word_kind);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_arr_alloc(pool, word_cap, sizeof(uint32_t), (void **)&ctx->word_op);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_arr_alloc(pool, (uint64_t)word_cap * CROSSTALK_HDL_PACK_LANE_CUNT, sizeof(uint32_t), (void **)&bit_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_place_run(ctx, node_pi, bit_node, word_cap, kneg, kp0, kp1);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_arr_alloc(pool, ir_ctx->sig_cunt, sizeof(uint32_t), (void **)&mem_off);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_mem_layout_get(ctx, mem_off, &mem_words);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->gt_cap = (uint32_t)((uint64_t)ctx->word_cunt * (CROSSTALK_HDL_PACK_LANE_CUNT * 0x2) + (uint64_t)ctx->mem_op_cunt * 0xC0 + ((uint64_t)aig_ctx->ff_bits + 0x1) * CROSSTALK_HDL_PACK_LANE_CUNT + 0x400);
    ret = crosstalk_hdl_pack_arr_alloc(pool, ctx->gt_cap, sizeof(uint32_t), (void **)&ctx->gt_blob);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ff_slot_cap = aig_ctx->ff_bits + 0x1;
    ret = crosstalk_hdl_pack_arr_alloc(pool, (uint64_t)ctx->word_cunt + 0x1, sizeof(uint32_t), (void **)&ff_slot_map);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    memset(ff_slot_map, CROSSTALK_HDL_PACK_BYTE_FILL, ((uint64_t)ctx->word_cunt + 0x1) * sizeof(uint32_t));
    ret = crosstalk_hdl_pack_arr_alloc(pool, ff_slot_cap, sizeof(uint32_t), (void **)&ff_word_list);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_arr_alloc(pool, (uint64_t)ff_slot_cap * CROSSTALK_HDL_PACK_LANE_CUNT, sizeof(uint32_t), (void **)&ff_ent);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    memset(ff_ent, CROSSTALK_HDL_PACK_BYTE_FILL, (uint64_t)ff_slot_cap * CROSSTALK_HDL_PACK_LANE_CUNT * sizeof(uint32_t));
    ret = crosstalk_hdl_pack_arr_alloc(pool, ff_slot_cap, sizeof(uint64_t), (void **)&ff_mask);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_arr_alloc(pool, (uint64_t)ctx->word_cunt + 0x1, sizeof(uint64_t), (void **)&init_word);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (pi_idx = 0; pi_idx < aig_ctx->pi_cunt; pi_idx = pi_idx + 0x1)
    {
        uint32_t pi_pos = 0;
        uint32_t pi_word = 0;
        uint32_t pi_lane = 0;
        uint32_t pi_sig = aig_ctx->pi_sig[pi_idx];

        node = aig_ctx->pi_node[pi_idx];
        pi_pos = ctx->pos[node];
        pi_word = pi_pos >> 0x6;
        pi_lane = pi_pos & CROSSTALK_HDL_PACK_LANE_MASK;
        if (aig_ctx->pi_kind[pi_idx] == CROSSTALK_HDL_AIG_PI_KIND_FF)
        {
            uint32_t slot = ff_slot_map[pi_word];

            if (slot == UINT32_MAX)
            {
                if (LDG_UNLIKELY(ctx->ff_word_cunt >= ff_slot_cap)) { return CROSSTALK_HDL_ERR_PACK; }

                slot = ctx->ff_word_cunt;
                ff_slot_map[pi_word] = slot;
                ff_word_list[slot] = pi_word;
                ctx->ff_word_cunt = ctx->ff_word_cunt + 0x1;
            }

            ff_ent[(uint64_t)slot * CROSSTALK_HDL_PACK_LANE_CUNT + pi_lane] = crosstalk_hdl_pack_pl_get(ctx, aig_ctx->ff_arena[aig_ctx->ff_off[pi_sig] + aig_ctx->pi_bit[pi_idx]]);
            ff_mask[slot] = ff_mask[slot] | ((uint64_t)0x1 << pi_lane);
        }

        if (aig_ctx->pi_kind[pi_idx] == CROSSTALK_HDL_AIG_PI_KIND_FF || aig_ctx->pi_kind[pi_idx] == CROSSTALK_HDL_AIG_PI_KIND_LATCH) { if (((ir_ctx->sigs[pi_sig].init_val >> aig_ctx->pi_bit[pi_idx]) & 0x1) != 0) { init_word[pi_word] = init_word[pi_word] | ((uint64_t)0x1 << pi_lane); } }
    }

    ret = crosstalk_hdl_pack_arr_alloc(pool, lit_total + 0x1, sizeof(uint32_t), (void **)&latch_map);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    memset(latch_map, CROSSTALK_HDL_PACK_BYTE_FILL, (lit_total + 0x1) * sizeof(uint32_t));
    for (pi_idx = 0; pi_idx < aig_ctx->pi_cunt; pi_idx = pi_idx + 0x1)
    {
        if (aig_ctx->pi_kind[pi_idx] != CROSSTALK_HDL_AIG_PI_KIND_LATCH) { continue; }

        latch_map[aig_ctx->lit_off[aig_ctx->pi_sig[pi_idx]] + aig_ctx->pi_bit[pi_idx]] = ctx->pos[aig_ctx->pi_node[pi_idx]];
    }

    ret = crosstalk_hdl_pack_arr_alloc(pool, ((uint64_t)aig_ctx->latch_bits + 0x1) * 0x2, sizeof(uint32_t), (void **)&lt_pairs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    root_cursor = aig_ctx->latch_root_base;
    for (idx = 0; idx < ir_ctx->sig_cunt; idx = idx + 0x1)
    {
        if (aig_ctx->latch_is[idx] == LDG_TRUTH_FALSE || aig_ctx->last_wr[idx] == UINT32_MAX || aig_ctx->op_placed[aig_ctx->last_wr[idx]] == LDG_TRUTH_FALSE) { continue; }

        for (bit_idx = 0; bit_idx < ir_ctx->sigs[idx].width; bit_idx = bit_idx + 0x1)
        {
            uint32_t pi_pos = latch_map[aig_ctx->lit_off[idx] + bit_idx];

            if (LDG_UNLIKELY(pi_pos == UINT32_MAX || ctx->latch_cunt >= aig_ctx->latch_bits)) { return CROSSTALK_HDL_ERR_PACK; }

            lt_pairs[ctx->latch_cunt * 0x2] = pi_pos;
            lt_pairs[ctx->latch_cunt * 0x2 + 0x1] = crosstalk_hdl_pack_pl_get(ctx, aig_ctx->roots[root_cursor]);
            root_cursor = root_cursor + 0x1;
            ctx->latch_cunt = ctx->latch_cunt + 0x1;
        }
    }

    if (ir_ctx->seq_blk_cunt > 0)
    {
        uint32_t clk_sig = crosstalk_hdl_pack_clk_root_get(ir_ctx, ir_ctx->seq_blks[0].clk_sig);

        if (clk_sig != UINT32_MAX && aig_ctx->lit_off[clk_sig] != UINT32_MAX) { clk_pl = crosstalk_hdl_pack_pl_get(ctx, aig_ctx->lit_arena[aig_ctx->lit_off[clk_sig]]); }
    }

    body_cap = CROSSTALK_HDL_PACK_BODY_BASE_CAP + (uint64_t)ctx->word_cunt * 0x200;
    ret = ldg_mem_pool_alloc((ldg_mem_pool_t *)pool, body_cap, (void **)&body);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->out = body;
    ctx->out_len = 0;
    ctx->out_cap = (uint32_t)body_cap;
    ret = crosstalk_hdl_pack_helpers_emit(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_comb_emit(ctx, bit_node, mem_off, ctx->latch_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_commit_emit(ctx, mem_off, ff_word_list, ctx->ff_word_cunt, ff_ent, ff_mask);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_iface_emit(ctx, clk_pl, init_word, ctx->word_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_export_emit(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_harness_emit(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    body_len = ctx->out_len;
    head_cap = body_cap + CROSSTALK_HDL_PACK_OUT_BASE_CAP + lit_total * 0xC + (uint64_t)ctx->gt_cunt * 0xC + (uint64_t)ir_ctx->sig_cunt * 0x60;
    ret = ldg_mem_pool_alloc((ldg_mem_pool_t *)pool, head_cap, (void **)&head);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->out = head;
    ctx->out_len = 0;
    ctx->out_cap = (uint32_t)head_cap;
    ret = crosstalk_hdl_pack_head_emit(ctx, mem_off, mem_words, lt_pairs, ctx->latch_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_pack_bytes_emit(ctx, body, body_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_src = ctx->out;
    *out_len = ctx->out_len;

    return LDG_ERR_AOK;
}
