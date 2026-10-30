#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>

#include "hdl/aig.h"
#include "hdl/lutmap.h"

#define CROSSTALK_HDL_LUTMAP_AF_LUT_COST 256

typedef struct crosstalk_hdl_lutmap_cut
{
    uint32_t leaf[CROSSTALK_HDL_LUTMAP_K_MAX];
    uint32_t depth;
    uint32_t leaf_cunt;
    uint32_t area_flow;
} crosstalk_hdl_lutmap_cut_t;

typedef struct crosstalk_hdl_lutmap_ctx
{
    const crosstalk_hdl_aig_ctx_t *aig_ctx;
    crosstalk_hdl_lutmap_cut_t *cuts;
    uint32_t *arrival;
    uint32_t *fanout;
    uint32_t *af_node;
    uint32_t *req;
    uint8_t *need;
    uint8_t *cut_cunt;
    uint8_t *lut_is;
    uint8_t *pi_is;
    uint8_t *chosen;
    uint32_t lut_k;
    uint8_t pudding[4];
} crosstalk_hdl_lutmap_ctx_t;

static uint32_t crosstalk_hdl_lutmap_arr_alloc(void *pool, uint64_t cunt, uint64_t item_size, void **out)
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

static uint32_t crosstalk_hdl_lutmap_cut_merge(const crosstalk_hdl_lutmap_cut_t *lhs, const crosstalk_hdl_lutmap_cut_t *rhs, const uint32_t *arrival, uint32_t lut_k, crosstalk_hdl_lutmap_cut_t *out_cut)
{
    uint32_t lhs_idx = 0;
    uint32_t rhs_idx = 0;
    uint32_t leaf_val = 0;
    uint32_t deep = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(lhs == 0x0 || rhs == 0x0 || arrival == 0x0 || out_cut == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    out_cut->leaf_cunt = 0;
    while (lhs_idx < lhs->leaf_cunt || rhs_idx < rhs->leaf_cunt)
    {
        if (lhs_idx < lhs->leaf_cunt && rhs_idx < rhs->leaf_cunt && lhs->leaf[lhs_idx] == rhs->leaf[rhs_idx])
        {
            leaf_val = lhs->leaf[lhs_idx];
            lhs_idx = lhs_idx + 0x1;
            rhs_idx = rhs_idx + 0x1;
        }
        else if (rhs_idx >= rhs->leaf_cunt || (lhs_idx < lhs->leaf_cunt && lhs->leaf[lhs_idx] < rhs->leaf[rhs_idx]))
        {
            leaf_val = lhs->leaf[lhs_idx];
            lhs_idx = lhs_idx + 0x1;
        }
        else
        {
            leaf_val = rhs->leaf[rhs_idx];
            rhs_idx = rhs_idx + 0x1;
        }

        if (out_cut->leaf_cunt >= lut_k) { return LDG_ERR_FULL; }

        out_cut->leaf[out_cut->leaf_cunt] = leaf_val;
        out_cut->leaf_cunt = out_cut->leaf_cunt + 0x1;
    }

    deep = 0;
    for (idx = 0; idx < out_cut->leaf_cunt; idx = idx + 1) { if (arrival[out_cut->leaf[idx]] > deep) { deep = arrival[out_cut->leaf[idx]]; } }

    out_cut->depth = deep + 0x1;
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_lutmap_cut_af_get(const crosstalk_hdl_lutmap_ctx_t *ctx, const crosstalk_hdl_lutmap_cut_t *cut)
{
    uint64_t flow = CROSSTALK_HDL_LUTMAP_AF_LUT_COST;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || cut == 0x0)) { return UINT32_MAX; }

    for (idx = 0; idx < cut->leaf_cunt; idx = idx + 1) { flow = flow + ctx->af_node[cut->leaf[idx]]; }

    if (flow > UINT32_MAX) { flow = UINT32_MAX; }

    return (uint32_t)flow;
}

static uint32_t crosstalk_hdl_lutmap_cut_keep(crosstalk_hdl_lutmap_cut_t *slot_arr, uint8_t *slot_cunt, const crosstalk_hdl_lutmap_cut_t *cand)
{
    uint32_t idx = 0;
    uint32_t put_idx = 0;
    uint32_t cunt = 0;

    if (LDG_UNLIKELY(slot_arr == 0x0 || slot_cunt == 0x0 || cand == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    cunt = *slot_cunt;
    put_idx = cunt;
    for (idx = 0; idx < cunt; idx = idx + 1) { if (cand->depth < slot_arr[idx].depth || (cand->depth == slot_arr[idx].depth && cand->leaf_cunt < slot_arr[idx].leaf_cunt))
        {
            put_idx = idx;
            break;
        }
    }

    if (put_idx >= CROSSTALK_HDL_LUTMAP_CUT_MAX) { return LDG_ERR_AOK; }

    if (cunt < CROSSTALK_HDL_LUTMAP_CUT_MAX) { cunt = cunt + 0x1; }

    for (idx = cunt - 0x1; idx > put_idx; idx = idx - 1) { slot_arr[idx] = slot_arr[idx - 0x1]; }

    slot_arr[put_idx] = *cand;
    *slot_cunt = (uint8_t)cunt;
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_lutmap_node_cuts_build(crosstalk_hdl_lutmap_ctx_t *ctx, uint32_t node_idx)
{
    const crosstalk_hdl_aig_node_t *node = 0x0;
    crosstalk_hdl_lutmap_cut_t lhs_triv = LDG_STRUCT_ZERO_INIT;
    crosstalk_hdl_lutmap_cut_t rhs_triv = LDG_STRUCT_ZERO_INIT;
    crosstalk_hdl_lutmap_cut_t cand = LDG_STRUCT_ZERO_INIT;
    const crosstalk_hdl_lutmap_cut_t *lhs_cut = 0x0;
    const crosstalk_hdl_lutmap_cut_t *rhs_cut = 0x0;
    uint32_t lhs_node = 0;
    uint32_t rhs_node = 0;
    uint32_t lhs_cunt = 0;
    uint32_t rhs_cunt = 0;
    uint32_t lhs_idx = 0;
    uint32_t rhs_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    node = &ctx->aig_ctx->nodes[node_idx];
    lhs_node = node->fanin0 >> 0x1;
    rhs_node = node->fanin1 >> 0x1;
    lhs_triv.leaf[0] = lhs_node;
    lhs_triv.leaf_cunt = 0x1;
    lhs_triv.depth = ctx->arrival[lhs_node];
    rhs_triv.leaf[0] = rhs_node;
    rhs_triv.leaf_cunt = 0x1;
    rhs_triv.depth = ctx->arrival[rhs_node];

    lhs_cunt = ctx->cut_cunt[lhs_node];
    rhs_cunt = ctx->cut_cunt[rhs_node];
    for (lhs_idx = 0; lhs_idx <= lhs_cunt; lhs_idx = lhs_idx + 1)
    {
        lhs_cut = lhs_idx < lhs_cunt ? &ctx->cuts[(uint64_t)lhs_node * CROSSTALK_HDL_LUTMAP_CUT_MAX + lhs_idx] : &lhs_triv;
        for (rhs_idx = 0; rhs_idx <= rhs_cunt; rhs_idx = rhs_idx + 1)
        {
            rhs_cut = rhs_idx < rhs_cunt ? &ctx->cuts[(uint64_t)rhs_node * CROSSTALK_HDL_LUTMAP_CUT_MAX + rhs_idx] : &rhs_triv;
            ret = crosstalk_hdl_lutmap_cut_merge(lhs_cut, rhs_cut, ctx->arrival, ctx->lut_k, &cand);
            if (ret == LDG_ERR_FULL) { continue; }

            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            cand.area_flow = crosstalk_hdl_lutmap_cut_af_get(ctx, &cand);
            ret = crosstalk_hdl_lutmap_cut_keep(&ctx->cuts[(uint64_t)node_idx * CROSSTALK_HDL_LUTMAP_CUT_MAX], &ctx->cut_cunt[node_idx], &cand);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }

    if (LDG_UNLIKELY(ctx->cut_cunt[node_idx] == 0)) { return CROSSTALK_HDL_ERR_LUTMAP_CAP; }

    ctx->arrival[node_idx] = ctx->cuts[(uint64_t)node_idx * CROSSTALK_HDL_LUTMAP_CUT_MAX].depth;
    {
        uint32_t best_af = UINT32_MAX;
        uint32_t cut_idx = 0;

        for (cut_idx = 0; cut_idx < ctx->cut_cunt[node_idx]; cut_idx = cut_idx + 1) { if (ctx->cuts[(uint64_t)node_idx * CROSSTALK_HDL_LUTMAP_CUT_MAX + cut_idx].area_flow < best_af) { best_af = ctx->cuts[(uint64_t)node_idx * CROSSTALK_HDL_LUTMAP_CUT_MAX + cut_idx].area_flow; } }

        ctx->af_node[node_idx] = best_af / (ctx->fanout[node_idx] != 0 ? ctx->fanout[node_idx] : 0x1);
    }

    return LDG_ERR_AOK;
}

static uint64_t crosstalk_hdl_lutmap_var_mask_get(uint32_t var_idx)
{
    uint64_t mask = 0;
    uint32_t bit_at = 0;

    for (bit_at = 0; bit_at < (uint32_t)CROSSTALK_HDL_NETARR_MASK_BITS; bit_at = bit_at + 0x1) { if (((bit_at >> var_idx) & 0x1) != 0x0) { mask = mask | ((uint64_t)0x1 << bit_at); } }

    return mask;
}

static uint32_t crosstalk_hdl_lutmap_lut_mask_get(const crosstalk_hdl_aig_ctx_t *aig_ctx, const crosstalk_hdl_lutmap_cut_t *cut, uint32_t node_idx, uint64_t *vals, uint32_t *gen_arr, uint32_t gen, uint32_t *stack, uint64_t stack_cap, uint64_t *out_mask)
{
    uint64_t sp = 0;
    uint32_t leaf_idx = 0;

    if (LDG_UNLIKELY(aig_ctx == 0x0 || cut == 0x0 || vals == 0x0 || gen_arr == 0x0 || stack == 0x0 || out_mask == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_mask = 0x0;
    vals[0] = 0x0;
    gen_arr[0] = gen;
    for (leaf_idx = 0; leaf_idx < cut->leaf_cunt; leaf_idx = leaf_idx + 0x1)
    {
        if (cut->leaf[leaf_idx] == 0x0) { continue; }

        vals[cut->leaf[leaf_idx]] = crosstalk_hdl_lutmap_var_mask_get(leaf_idx);
        gen_arr[cut->leaf[leaf_idx]] = gen;
    }

    if (gen_arr[node_idx] == gen)
    {
        *out_mask = vals[node_idx];
        return LDG_ERR_AOK;
    }

    stack[sp] = node_idx;
    sp = sp + 0x1;
    while (sp != 0)
    {
        const crosstalk_hdl_aig_node_t *node = 0x0;
        uint64_t lhs_val = 0;
        uint64_t rhs_val = 0;
        uint32_t at = stack[sp - 0x1];
        uint32_t lhs_node = 0;
        uint32_t rhs_node = 0;

        if (gen_arr[at] == gen)
        {
            sp = sp - 0x1;
            continue;
        }

        node = &aig_ctx->nodes[at];
        if (node->fanin0 == UINT32_MAX) { return CROSSTALK_HDL_ERR_LUTMAP_CONE; }

        lhs_node = node->fanin0 >> 0x1;
        rhs_node = node->fanin1 >> 0x1;
        if (gen_arr[lhs_node] != gen || gen_arr[rhs_node] != gen)
        {
            if (sp + 0x2 > stack_cap) { return CROSSTALK_HDL_ERR_LUTMAP_CONE; }

            if (gen_arr[lhs_node] != gen)
            {
                stack[sp] = lhs_node;
                sp = sp + 0x1;
            }

            if (gen_arr[rhs_node] != gen)
            {
                stack[sp] = rhs_node;
                sp = sp + 0x1;
            }

            continue;
        }

        lhs_val = (node->fanin0 & 0x1) != 0x0 ? ~vals[lhs_node] : vals[lhs_node];
        rhs_val = (node->fanin1 & 0x1) != 0x0 ? ~vals[rhs_node] : vals[rhs_node];
        vals[at] = lhs_val & rhs_val;
        gen_arr[at] = gen;
        sp = sp - 0x1;
    }

    *out_mask = vals[node_idx];
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_lutmap_flow_run(const crosstalk_hdl_aig_ctx_t *aig_ctx, void *pool, uint32_t lut_k, crosstalk_hdl_netarr_t *out_netarr, crosstalk_hdl_lutmap_res_t *out_res)
{
    crosstalk_hdl_lutmap_ctx_t ctx = LDG_STRUCT_ZERO_INIT;
    const crosstalk_hdl_lutmap_cut_t *best_cut = 0x0;
    uint64_t node_total = 0;
    uint32_t node_idx = 0;
    uint32_t root_node = 0;
    uint32_t leaf_idx = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(aig_ctx == 0x0 || pool == 0x0 || out_res == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(lut_k < 0x2 || lut_k > CROSSTALK_HDL_LUTMAP_K_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    memset(out_res, 0x0, sizeof(*out_res));
    ctx.aig_ctx = aig_ctx;
    ctx.lut_k = lut_k;
    node_total = aig_ctx->node_cunt;
    ret = crosstalk_hdl_lutmap_arr_alloc(pool, node_total * CROSSTALK_HDL_LUTMAP_CUT_MAX, sizeof(crosstalk_hdl_lutmap_cut_t), (void **)&ctx.cuts);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_lutmap_arr_alloc(pool, node_total, sizeof(uint32_t), (void **)&ctx.arrival);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_lutmap_arr_alloc(pool, node_total, sizeof(uint32_t), (void **)&ctx.fanout);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_lutmap_arr_alloc(pool, node_total, sizeof(uint32_t), (void **)&ctx.af_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_lutmap_arr_alloc(pool, node_total, sizeof(uint32_t), (void **)&ctx.req);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_lutmap_arr_alloc(pool, node_total, sizeof(uint8_t), (void **)&ctx.need);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_lutmap_arr_alloc(pool, node_total, sizeof(uint8_t), (void **)&ctx.cut_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_lutmap_arr_alloc(pool, node_total, sizeof(uint8_t), (void **)&ctx.lut_is);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_lutmap_arr_alloc(pool, node_total, sizeof(uint8_t), (void **)&ctx.pi_is);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_lutmap_arr_alloc(pool, node_total, sizeof(uint8_t), (void **)&ctx.chosen);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (node_idx = 0x1; node_idx < aig_ctx->node_cunt; node_idx = node_idx + 1)
    {
        if (aig_ctx->nodes[node_idx].fanin0 == UINT32_MAX)
        {
            ctx.pi_is[node_idx] = LDG_TRUTH_TRUE;
            continue;
        }

        ctx.fanout[aig_ctx->nodes[node_idx].fanin0 >> 0x1] = ctx.fanout[aig_ctx->nodes[node_idx].fanin0 >> 0x1] + 0x1;
        ctx.fanout[aig_ctx->nodes[node_idx].fanin1 >> 0x1] = ctx.fanout[aig_ctx->nodes[node_idx].fanin1 >> 0x1] + 0x1;
    }

    for (idx = 0; idx < aig_ctx->root_cunt; idx = idx + 1) { ctx.fanout[aig_ctx->roots[idx] >> 0x1] = ctx.fanout[aig_ctx->roots[idx] >> 0x1] + 0x1; }

    for (node_idx = 0x1; node_idx < aig_ctx->node_cunt; node_idx = node_idx + 1)
    {
        ctx.req[node_idx] = UINT32_MAX;
        if (ctx.pi_is[node_idx] != LDG_TRUTH_FALSE) { continue; }

        ret = crosstalk_hdl_lutmap_node_cuts_build(&ctx, node_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (idx = 0; idx < aig_ctx->root_cunt; idx = idx + 1)
    {
        root_node = aig_ctx->roots[idx] >> 0x1;
        if (root_node == 0 || ctx.pi_is[root_node] == LDG_TRUTH_TRUE) { continue; }

        if (ctx.arrival[root_node] > out_res->depth) { out_res->depth = ctx.arrival[root_node]; }

        ctx.need[root_node] = LDG_TRUTH_TRUE;
        if (ctx.req[root_node] > ctx.arrival[root_node]) { ctx.req[root_node] = ctx.arrival[root_node]; }
    }

    if (out_netarr != 0x0)
    {
        uint32_t sig_idx = 0;
        uint32_t bit_idx = 0;

        for (sig_idx = 0; sig_idx < aig_ctx->ir_ctx->sig_cunt; sig_idx = sig_idx + 1)
        {
            if (aig_ctx->lit_off[sig_idx] == UINT32_MAX) { continue; }

            for (bit_idx = 0; bit_idx < aig_ctx->ir_ctx->sigs[sig_idx].width; bit_idx = bit_idx + 1)
            {
                root_node = aig_ctx->lit_arena[aig_ctx->lit_off[sig_idx] + bit_idx] >> 0x1;
                if (root_node == 0 || ctx.pi_is[root_node] == LDG_TRUTH_TRUE) { continue; }

                ctx.need[root_node] = LDG_TRUTH_TRUE;
                if (ctx.req[root_node] > ctx.arrival[root_node]) { ctx.req[root_node] = ctx.arrival[root_node]; }
            }
        }
    }

    for (node_idx = aig_ctx->node_cunt; node_idx > 0x1; node_idx = node_idx - 1)
    {
        uint32_t sweep_idx = node_idx - 0x1;
        uint32_t best_af = UINT32_MAX;
        uint32_t cut_idx = 0;

        if (ctx.need[sweep_idx] == LDG_TRUTH_FALSE || ctx.pi_is[sweep_idx] != LDG_TRUTH_FALSE) { continue; }

        best_cut = 0x0;
        for (cut_idx = 0; cut_idx < ctx.cut_cunt[sweep_idx]; cut_idx = cut_idx + 1)
        {
            const crosstalk_hdl_lutmap_cut_t *cand_cut = &ctx.cuts[(uint64_t)sweep_idx * CROSSTALK_HDL_LUTMAP_CUT_MAX + cut_idx];

            if (cand_cut->depth > ctx.req[sweep_idx]) { continue; }

            if (cand_cut->area_flow < best_af || best_cut == 0x0)
            {
                best_af = cand_cut->area_flow;
                best_cut = cand_cut;
                ctx.chosen[sweep_idx] = (uint8_t)cut_idx;
            }
        }

        if (LDG_UNLIKELY(best_cut == 0x0)) { return CROSSTALK_HDL_ERR_LUTMAP_STARVE; }

        ctx.lut_is[sweep_idx] = LDG_TRUTH_TRUE;
        out_res->lut_cunt = out_res->lut_cunt + 0x1;
        for (leaf_idx = 0; leaf_idx < best_cut->leaf_cunt; leaf_idx = leaf_idx + 1)
        {
            uint32_t leaf_node = best_cut->leaf[leaf_idx];

            if (leaf_node == 0 || ctx.pi_is[leaf_node] != LDG_TRUTH_FALSE) { continue; }

            ctx.need[leaf_node] = LDG_TRUTH_TRUE;
            if (ctx.req[leaf_node] > ctx.req[sweep_idx] - 0x1) { ctx.req[leaf_node] = ctx.req[sweep_idx] - 0x1; }
        }
    }

    out_res->ff_bits = aig_ctx->ff_bits;
    out_res->le_cunt = out_res->lut_cunt > aig_ctx->ff_bits ? out_res->lut_cunt : aig_ctx->ff_bits;
    out_res->node_cunt = aig_ctx->node_cunt;
    out_res->pi_cunt = aig_ctx->pi_cunt;
    if (lut_k >= 6) { out_res->period_ps = CROSSTALK_HDL_LUTMAP_K6_FIXED_PS + out_res->depth * CROSSTALK_HDL_LUTMAP_K6_LEVEL_PS; }
    else { out_res->period_ps = CROSSTALK_HDL_LUTMAP_K4_FIXED_PS + out_res->depth * CROSSTALK_HDL_LUTMAP_K4_LEVEL_PS; }

    out_res->fmax_khz = (uint32_t)((uint64_t)CROSSTALK_HDL_LUTMAP_PS_PER_KHZ / out_res->period_ps);
    if (out_netarr != 0x0)
    {
        uint64_t *vals = 0x0;
        uint32_t *gen_arr = 0x0;
        uint32_t *stack = 0x0;
        uint64_t stack_cap = node_total * 0x2;
        uint32_t at = 0;
        uint32_t gen = 0;

        memset(out_netarr, 0x0, sizeof(*out_netarr));
        ret = crosstalk_hdl_lutmap_arr_alloc(pool, out_res->lut_cunt, sizeof(crosstalk_hdl_netarr_lut_t), (void **)&out_netarr->luts);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_lutmap_arr_alloc(pool, node_total, sizeof(uint64_t), (void **)&vals);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_lutmap_arr_alloc(pool, node_total, sizeof(uint32_t), (void **)&gen_arr);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_lutmap_arr_alloc(pool, stack_cap, sizeof(uint32_t), (void **)&stack);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        for (node_idx = 0x1; node_idx < aig_ctx->node_cunt; node_idx = node_idx + 1)
        {
            const crosstalk_hdl_lutmap_cut_t *pick = 0x0;
            crosstalk_hdl_netarr_lut_t *lut = 0x0;

            if (ctx.lut_is[node_idx] != LDG_TRUTH_TRUE) { continue; }

            if (LDG_UNLIKELY(at >= out_res->lut_cunt)) { return CROSSTALK_HDL_ERR_LUTMAP_FILL; }

            pick = &ctx.cuts[(uint64_t)node_idx * CROSSTALK_HDL_LUTMAP_CUT_MAX + ctx.chosen[node_idx]];
            lut = &out_netarr->luts[at];
            lut->in_cunt = pick->leaf_cunt;
            lut->out_node = node_idx;
            for (leaf_idx = 0; leaf_idx < (uint32_t)CROSSTALK_HDL_NETARR_IN_MAX; leaf_idx = leaf_idx + 1) { lut->in_node[leaf_idx] = leaf_idx < pick->leaf_cunt ? pick->leaf[leaf_idx] : (uint32_t)UINT32_MAX; }

            gen = gen + 0x1;
            ret = crosstalk_hdl_lutmap_lut_mask_get(aig_ctx, pick, node_idx, vals, gen_arr, gen, stack, stack_cap, &lut->mask);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            at = at + 0x1;
        }

        if (LDG_UNLIKELY(at != out_res->lut_cunt)) { return CROSSTALK_HDL_ERR_LUTMAP_FILL; }

        out_netarr->aig_ctx = aig_ctx;
        out_netarr->lut_cunt = out_res->lut_cunt;
        out_netarr->lut_k = lut_k;
        out_netarr->node_cunt = aig_ctx->node_cunt;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_lutmap_run(const crosstalk_hdl_aig_ctx_t *aig_ctx, void *pool, uint32_t lut_k, crosstalk_hdl_lutmap_res_t *out_res)
{
    return crosstalk_hdl_lutmap_flow_run(aig_ctx, pool, lut_k, 0x0, out_res);
}

uint32_t crosstalk_hdl_lutmap_netarr_run(const crosstalk_hdl_aig_ctx_t *aig_ctx, void *pool, uint32_t lut_k, crosstalk_hdl_netarr_t *out_netarr, crosstalk_hdl_lutmap_res_t *out_res)
{
    if (LDG_UNLIKELY(out_netarr == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_hdl_lutmap_flow_run(aig_ctx, pool, lut_k, out_netarr, out_res);
}
