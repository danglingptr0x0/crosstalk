#include <stdint.h>

#include <dangling/core/arith.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/mem.h>

#include "hdl/level.h"

static uint32_t crosstalk_hdl_level_arr_alloc(void *pool, uint64_t cunt, uint64_t item_size, void **out)
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

static uint32_t crosstalk_hdl_level_wsig_get(const crosstalk_hdl_ir_op_t *op)
{
    if (op->kind == CROSSTALK_HDL_IR_OP_MEM_WR) { return op->src0; }

    return op->dst;
}

static uint32_t crosstalk_hdl_level_rds_get(const crosstalk_hdl_ir_op_t *op, uint32_t *out_sigs)
{
    uint32_t cunt = 0;

    switch (op->kind)
    {
        case CROSSTALK_HDL_IR_OP_CONST:
            break;
        case CROSSTALK_HDL_IR_OP_COPY:
        case CROSSTALK_HDL_IR_OP_ZEXT:
        case CROSSTALK_HDL_IR_OP_SEXT:
        case CROSSTALK_HDL_IR_OP_NOT:
        case CROSSTALK_HDL_IR_OP_NEG:
        case CROSSTALK_HDL_IR_OP_LNOT:
        case CROSSTALK_HDL_IR_OP_RED_AND:
        case CROSSTALK_HDL_IR_OP_RED_OR:
        case CROSSTALK_HDL_IR_OP_RED_XOR:
        case CROSSTALK_HDL_IR_OP_SLICE:
        case CROSSTALK_HDL_IR_OP_REG_NEXT:
            out_sigs[cunt] = op->src0;
            cunt = cunt + 0x1;
            break;
        case CROSSTALK_HDL_IR_OP_SLICE_DYN:
        case CROSSTALK_HDL_IR_OP_INSERT:
            out_sigs[cunt] = op->src0;
            cunt = cunt + 0x1;
            out_sigs[cunt] = op->src1;
            cunt = cunt + 0x1;
            break;
        case CROSSTALK_HDL_IR_OP_INSERT_DYN:
        case CROSSTALK_HDL_IR_OP_MUX:
            out_sigs[cunt] = op->src0;
            cunt = cunt + 0x1;
            out_sigs[cunt] = op->src1;
            cunt = cunt + 0x1;
            out_sigs[cunt] = op->src2;
            cunt = cunt + 0x1;
            break;
        case CROSSTALK_HDL_IR_OP_MEM_RD:
            out_sigs[cunt] = op->src1;
            cunt = cunt + 0x1;
            out_sigs[cunt] = op->src0;
            cunt = cunt + 0x1;
            break;
        case CROSSTALK_HDL_IR_OP_MEM_WR:
            out_sigs[cunt] = op->dst;
            cunt = cunt + 0x1;
            out_sigs[cunt] = op->src1;
            cunt = cunt + 0x1;
            out_sigs[cunt] = op->src2;
            cunt = cunt + 0x1;
            break;
        default:
            out_sigs[cunt] = op->src0;
            cunt = cunt + 0x1;
            out_sigs[cunt] = op->src1;
            cunt = cunt + 0x1;
            break;
    }

    return cunt;
}

static void crosstalk_hdl_level_heap_push(uint32_t *heap, uint32_t *heap_cunt, const uint32_t *key, uint32_t item)
{
    uint32_t slot = 0;
    uint32_t parent = 0;
    uint32_t tmp = 0;

    heap[*heap_cunt] = item;
    slot = *heap_cunt;
    *heap_cunt = *heap_cunt + 0x1;

    while (slot > 0)
    {
        parent = (slot - 0x1) / 0x2;

        if (key[heap[parent]] <= key[heap[slot]]) { break; }

        tmp = heap[parent];
        heap[parent] = heap[slot];
        heap[slot] = tmp;
        slot = parent;
    }
}

static uint32_t crosstalk_hdl_level_heap_pop(uint32_t *heap, uint32_t *heap_cunt, const uint32_t *key)
{
    uint32_t top = 0;
    uint32_t slot = 0;
    uint32_t child = 0;
    uint32_t tmp = 0;

    top = heap[0];
    *heap_cunt = *heap_cunt - 0x1;
    heap[0] = heap[*heap_cunt];

    while (0x1)
    {
        child = slot * 0x2 + 0x1;

        if (child >= *heap_cunt) { break; }

        if (child + 0x1 < *heap_cunt && key[heap[child + 0x1]] < key[heap[child]]) { child = child + 0x1; }

        if (key[heap[slot]] <= key[heap[child]]) { break; }

        tmp = heap[slot];
        heap[slot] = heap[child];
        heap[child] = tmp;
        slot = child;
    }

    return top;
}

uint32_t crosstalk_hdl_level_run(const crosstalk_hdl_ir_t *ir_ctx, void *pool, crosstalk_hdl_level_res_t *res)
{
    void *raw = 0x0;
    uint32_t *wr_base = 0x0;
    uint32_t *wr_ops = 0x0;
    uint32_t *succ_base = 0x0;
    uint32_t *succ_ops = 0x0;
    uint32_t *indeg = 0x0;
    uint32_t *t_idx = 0x0;
    uint32_t *t_low = 0x0;
    uint8_t *t_onstk = 0x0;
    uint32_t *t_stk = 0x0;
    uint32_t *frame_op = 0x0;
    uint32_t *frame_edge = 0x0;
    uint32_t *scc_id = 0x0;
    uint32_t *scc_base = 0x0;
    uint32_t *scc_ops = 0x0;
    uint8_t *scc_iter = 0x0;
    uint32_t *scc_key = 0x0;
    uint32_t *scc_level = 0x0;
    uint32_t *cond_base = 0x0;
    uint32_t *cond_succ = 0x0;
    uint32_t *cond_indeg = 0x0;
    uint32_t *heap = 0x0;
    uint32_t rd_sigs[4] = LDG_ARR_ZERO_INIT;
    uint32_t op_cunt = 0;
    uint32_t sig_cunt = 0;
    uint32_t edge_cunt = 0;
    uint32_t rd_cunt = 0;
    uint32_t scc_cunt = 0;
    uint32_t heap_cunt = 0;
    uint32_t visit_cunt = 0;
    uint32_t t_stk_cunt = 0;
    uint32_t frame_cunt = 0;
    uint32_t emit_pos = 0;
    uint32_t emit_scc_cunt = 0;
    uint32_t once_start = 0;
    uint32_t once_cunt = 0;
    uint32_t idx = 0;
    uint32_t sub = 0;
    uint32_t wsig = 0;
    uint32_t sig = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ir_ctx == 0x0 || pool == 0x0 || res == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    res->order = 0x0;
    res->depth = 0x0;
    res->units = 0x0;
    res->unit_cunt = 0;
    res->iter_op_cunt = 0;
    res->level_max = 0;

    op_cunt = ir_ctx->comb_cunt;
    sig_cunt = ir_ctx->sig_cunt;

    ret = crosstalk_hdl_level_arr_alloc(pool, (uint64_t)op_cunt, (uint64_t)sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    res->order = (uint32_t *)raw;

    ret = crosstalk_hdl_level_arr_alloc(pool, (uint64_t)op_cunt, (uint64_t)sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    res->depth = (uint32_t *)raw;

    ret = crosstalk_hdl_level_arr_alloc(pool, (uint64_t)op_cunt + 0x1, (uint64_t)sizeof(crosstalk_hdl_level_unit_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    res->units = (crosstalk_hdl_level_unit_t *)raw;

    if (op_cunt == 0) { return LDG_ERR_AOK; }

    ret = crosstalk_hdl_level_arr_alloc(pool, (uint64_t)sig_cunt + 0x1, (uint64_t)sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    wr_base = (uint32_t *)raw;

    for (idx = 0; idx <= sig_cunt; idx++) { wr_base[idx] = 0; }

    for (idx = 0; idx < op_cunt; idx++)
    {
        wsig = crosstalk_hdl_level_wsig_get(&ir_ctx->comb_ops[idx]);

        if (wsig != UINT32_MAX && wsig < sig_cunt) { wr_base[wsig + 0x1] = wr_base[wsig + 0x1] + 0x1; }
    }

    for (idx = 0; idx < sig_cunt; idx++) { wr_base[idx + 0x1] = wr_base[idx + 0x1] + wr_base[idx]; }

    ret = crosstalk_hdl_level_arr_alloc(pool, (uint64_t)wr_base[sig_cunt], (uint64_t)sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    wr_ops = (uint32_t *)raw;

    {
        uint32_t *wr_fill = 0x0;

        ret = crosstalk_hdl_level_arr_alloc(pool, (uint64_t)sig_cunt, (uint64_t)sizeof(uint32_t), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        wr_fill = (uint32_t *)raw;

        for (idx = 0; idx < sig_cunt; idx++) { wr_fill[idx] = wr_base[idx]; }

        for (idx = 0; idx < op_cunt; idx++)
        {
            wsig = crosstalk_hdl_level_wsig_get(&ir_ctx->comb_ops[idx]);

            if (wsig != UINT32_MAX && wsig < sig_cunt)
            {
                wr_ops[wr_fill[wsig]] = idx;
                wr_fill[wsig] = wr_fill[wsig] + 0x1;
            }
        }
    }

    ret = crosstalk_hdl_level_arr_alloc(pool, (uint64_t)op_cunt + 0x1, (uint64_t)sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    succ_base = (uint32_t *)raw;

    edge_cunt = 0;

    for (idx = 0; idx < op_cunt; idx++)
    {
        rd_cunt = crosstalk_hdl_level_rds_get(&ir_ctx->comb_ops[idx], rd_sigs);

        for (sub = 0; sub < rd_cunt; sub++)
        {
            sig = rd_sigs[sub];

            if (sig != UINT32_MAX && sig < sig_cunt) { edge_cunt = edge_cunt + (wr_base[sig + 0x1] - wr_base[sig]); }
        }
    }

    for (idx = 0; idx < sig_cunt; idx++)
    {
        uint32_t wr_cunt = wr_base[idx + 0x1] - wr_base[idx];

        if (wr_cunt > 0x1) { edge_cunt = edge_cunt + wr_cunt - 0x1; }
    }

    for (idx = 0; idx <= op_cunt; idx++) { succ_base[idx] = 0; }

    for (idx = 0; idx < op_cunt; idx++)
    {
        rd_cunt = crosstalk_hdl_level_rds_get(&ir_ctx->comb_ops[idx], rd_sigs);

        for (sub = 0; sub < rd_cunt; sub++)
        {
            uint32_t wpos = 0;

            sig = rd_sigs[sub];

            if (sig == UINT32_MAX || sig >= sig_cunt) { continue; }

            for (wpos = wr_base[sig]; wpos < wr_base[sig + 0x1]; wpos++) { succ_base[wr_ops[wpos] + 0x1] = succ_base[wr_ops[wpos] + 0x1] + 0x1; }
        }
    }

    for (idx = 0; idx < sig_cunt; idx++)
    {
        uint32_t wpos = 0;

        for (wpos = wr_base[idx]; wpos + 0x1 < wr_base[idx + 0x1]; wpos++) { succ_base[wr_ops[wpos] + 0x1] = succ_base[wr_ops[wpos] + 0x1] + 0x1; }
    }

    for (idx = 0; idx < op_cunt; idx++) { succ_base[idx + 0x1] = succ_base[idx + 0x1] + succ_base[idx]; }

    ret = crosstalk_hdl_level_arr_alloc(pool, (uint64_t)edge_cunt, (uint64_t)sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    succ_ops = (uint32_t *)raw;

    ret = crosstalk_hdl_level_arr_alloc(pool, (uint64_t)op_cunt, (uint64_t)sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    indeg = (uint32_t *)raw;

    for (idx = 0; idx < op_cunt; idx++) { indeg[idx] = succ_base[idx]; }

    for (idx = 0; idx < op_cunt; idx++)
    {
        rd_cunt = crosstalk_hdl_level_rds_get(&ir_ctx->comb_ops[idx], rd_sigs);

        for (sub = 0; sub < rd_cunt; sub++)
        {
            uint32_t wpos = 0;

            sig = rd_sigs[sub];

            if (sig == UINT32_MAX || sig >= sig_cunt) { continue; }

            for (wpos = wr_base[sig]; wpos < wr_base[sig + 0x1]; wpos++)
            {
                succ_ops[indeg[wr_ops[wpos]]] = idx;
                indeg[wr_ops[wpos]] = indeg[wr_ops[wpos]] + 0x1;
            }
        }
    }

    for (idx = 0; idx < sig_cunt; idx++)
    {
        uint32_t wpos = 0;

        for (wpos = wr_base[idx]; wpos + 0x1 < wr_base[idx + 0x1]; wpos++)
        {
            succ_ops[indeg[wr_ops[wpos]]] = wr_ops[wpos + 0x1];
            indeg[wr_ops[wpos]] = indeg[wr_ops[wpos]] + 0x1;
        }
    }

    ret = crosstalk_hdl_level_arr_alloc(pool, (uint64_t)op_cunt, (uint64_t)sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    t_idx = (uint32_t *)raw;

    ret = crosstalk_hdl_level_arr_alloc(pool, (uint64_t)op_cunt, (uint64_t)sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    t_low = (uint32_t *)raw;

    ret = crosstalk_hdl_level_arr_alloc(pool, (uint64_t)op_cunt, (uint64_t)sizeof(uint8_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    t_onstk = (uint8_t *)raw;

    ret = crosstalk_hdl_level_arr_alloc(pool, (uint64_t)op_cunt, (uint64_t)sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    t_stk = (uint32_t *)raw;

    ret = crosstalk_hdl_level_arr_alloc(pool, (uint64_t)op_cunt, (uint64_t)sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    frame_op = (uint32_t *)raw;

    ret = crosstalk_hdl_level_arr_alloc(pool, (uint64_t)op_cunt, (uint64_t)sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    frame_edge = (uint32_t *)raw;

    ret = crosstalk_hdl_level_arr_alloc(pool, (uint64_t)op_cunt, (uint64_t)sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    scc_id = (uint32_t *)raw;

    for (idx = 0; idx < op_cunt; idx++)
    {
        t_idx[idx] = UINT32_MAX;
        t_low[idx] = 0;
        t_onstk[idx] = LDG_TRUTH_FALSE;
        scc_id[idx] = UINT32_MAX;
    }

    for (idx = 0; idx < op_cunt; idx++)
    {
        if (t_idx[idx] != UINT32_MAX) { continue; }

        frame_op[0] = idx;
        frame_edge[0] = 0;
        frame_cunt = 0x1;

        while (frame_cunt > 0)
        {
            uint32_t now = frame_op[frame_cunt - 0x1];
            uint32_t epos = frame_edge[frame_cunt - 0x1];

            if (epos == 0 && t_idx[now] == UINT32_MAX)
            {
                t_idx[now] = visit_cunt;
                t_low[now] = visit_cunt;
                visit_cunt = visit_cunt + 0x1;
                t_stk[t_stk_cunt] = now;
                t_stk_cunt = t_stk_cunt + 0x1;
                t_onstk[now] = LDG_TRUTH_TRUE;
            }

            if (epos < succ_base[now + 0x1] - succ_base[now])
            {
                uint32_t succ_op = succ_ops[succ_base[now] + epos];

                frame_edge[frame_cunt - 0x1] = epos + 0x1;

                if (t_idx[succ_op] == UINT32_MAX)
                {
                    frame_op[frame_cunt] = succ_op;
                    frame_edge[frame_cunt] = 0;
                    frame_cunt = frame_cunt + 0x1;
                }
                else if (t_onstk[succ_op] == LDG_TRUTH_TRUE && t_idx[succ_op] < t_low[now]) { t_low[now] = t_idx[succ_op]; }

                continue;
            }

            frame_cunt = frame_cunt - 0x1;

            if (frame_cunt > 0)
            {
                uint32_t up = frame_op[frame_cunt - 0x1];

                if (t_low[now] < t_low[up]) { t_low[up] = t_low[now]; }
            }

            if (t_low[now] == t_idx[now])
            {
                while (0x1)
                {
                    uint32_t member = t_stk[t_stk_cunt - 0x1];

                    t_stk_cunt = t_stk_cunt - 0x1;
                    t_onstk[member] = LDG_TRUTH_FALSE;
                    scc_id[member] = scc_cunt;

                    if (member == now) { break; }
                }

                scc_cunt = scc_cunt + 0x1;
            }
        }
    }

    ret = crosstalk_hdl_level_arr_alloc(pool, (uint64_t)scc_cunt + 0x1, (uint64_t)sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    scc_base = (uint32_t *)raw;

    ret = crosstalk_hdl_level_arr_alloc(pool, (uint64_t)op_cunt, (uint64_t)sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    scc_ops = (uint32_t *)raw;

    ret = crosstalk_hdl_level_arr_alloc(pool, (uint64_t)scc_cunt, (uint64_t)sizeof(uint8_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    scc_iter = (uint8_t *)raw;

    ret = crosstalk_hdl_level_arr_alloc(pool, (uint64_t)scc_cunt, (uint64_t)sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    scc_key = (uint32_t *)raw;

    ret = crosstalk_hdl_level_arr_alloc(pool, (uint64_t)scc_cunt, (uint64_t)sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    scc_level = (uint32_t *)raw;

    for (idx = 0; idx <= scc_cunt; idx++) { scc_base[idx] = 0; }

    for (idx = 0; idx < op_cunt; idx++) { scc_base[scc_id[idx] + 0x1] = scc_base[scc_id[idx] + 0x1] + 0x1; }

    for (idx = 0; idx < scc_cunt; idx++) { scc_base[idx + 0x1] = scc_base[idx + 0x1] + scc_base[idx]; }

    {
        uint32_t *scc_fill = 0x0;

        ret = crosstalk_hdl_level_arr_alloc(pool, (uint64_t)scc_cunt, (uint64_t)sizeof(uint32_t), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        scc_fill = (uint32_t *)raw;

        for (idx = 0; idx < scc_cunt; idx++)
        {
            scc_fill[idx] = scc_base[idx];
            scc_iter[idx] = LDG_TRUTH_FALSE;
            scc_key[idx] = UINT32_MAX;
            scc_level[idx] = 0;
        }

        for (idx = 0; idx < op_cunt; idx++)
        {
            scc_ops[scc_fill[scc_id[idx]]] = idx;
            scc_fill[scc_id[idx]] = scc_fill[scc_id[idx]] + 0x1;

            if (idx < scc_key[scc_id[idx]]) { scc_key[scc_id[idx]] = idx; }
        }
    }

    for (idx = 0; idx < scc_cunt; idx++) { if (scc_base[idx + 0x1] - scc_base[idx] > 0x1) { scc_iter[idx] = LDG_TRUTH_TRUE; } }

    for (idx = 0; idx < op_cunt; idx++)
    {
        uint32_t epos = 0;

        for (epos = succ_base[idx]; epos < succ_base[idx + 0x1]; epos++) { if (succ_ops[epos] == idx) { scc_iter[scc_id[idx]] = LDG_TRUTH_TRUE; } }
    }

    ret = crosstalk_hdl_level_arr_alloc(pool, (uint64_t)scc_cunt + 0x1, (uint64_t)sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    cond_base = (uint32_t *)raw;

    ret = crosstalk_hdl_level_arr_alloc(pool, (uint64_t)edge_cunt, (uint64_t)sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    cond_succ = (uint32_t *)raw;

    ret = crosstalk_hdl_level_arr_alloc(pool, (uint64_t)scc_cunt, (uint64_t)sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    cond_indeg = (uint32_t *)raw;

    for (idx = 0; idx <= scc_cunt; idx++) { cond_base[idx] = 0; }

    for (idx = 0; idx < scc_cunt; idx++) { cond_indeg[idx] = 0; }

    for (idx = 0; idx < op_cunt; idx++)
    {
        uint32_t epos = 0;

        for (epos = succ_base[idx]; epos < succ_base[idx + 0x1]; epos++) { if (scc_id[succ_ops[epos]] != scc_id[idx]) { cond_base[scc_id[idx] + 0x1] = cond_base[scc_id[idx] + 0x1] + 0x1; } }
    }

    for (idx = 0; idx < scc_cunt; idx++) { cond_base[idx + 0x1] = cond_base[idx + 0x1] + cond_base[idx]; }

    {
        uint32_t *cond_fill = 0x0;

        ret = crosstalk_hdl_level_arr_alloc(pool, (uint64_t)scc_cunt, (uint64_t)sizeof(uint32_t), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        cond_fill = (uint32_t *)raw;

        for (idx = 0; idx < scc_cunt; idx++) { cond_fill[idx] = cond_base[idx]; }

        for (idx = 0; idx < op_cunt; idx++)
        {
            uint32_t epos = 0;

            for (epos = succ_base[idx]; epos < succ_base[idx + 0x1]; epos++) { if (scc_id[succ_ops[epos]] != scc_id[idx])
                {
                    cond_succ[cond_fill[scc_id[idx]]] = scc_id[succ_ops[epos]];
                    cond_fill[scc_id[idx]] = cond_fill[scc_id[idx]] + 0x1;
                    cond_indeg[scc_id[succ_ops[epos]]] = cond_indeg[scc_id[succ_ops[epos]]] + 0x1;
                }
            }
        }
    }

    ret = crosstalk_hdl_level_arr_alloc(pool, (uint64_t)scc_cunt, (uint64_t)sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    heap = (uint32_t *)raw;

    for (idx = 0; idx < scc_cunt; idx++) { if (cond_indeg[idx] == 0) { crosstalk_hdl_level_heap_push(heap, &heap_cunt, scc_key, idx); } }

    once_start = 0;
    once_cunt = 0;

    while (heap_cunt > 0)
    {
        uint32_t now_scc = crosstalk_hdl_level_heap_pop(heap, &heap_cunt, scc_key);
        uint32_t member_cunt = scc_base[now_scc + 0x1] - scc_base[now_scc];
        uint32_t epos = 0;

        for (idx = scc_base[now_scc]; idx < scc_base[now_scc + 0x1]; idx++)
        {
            res->order[emit_pos] = scc_ops[idx];
            res->depth[scc_ops[idx]] = scc_level[now_scc];
            emit_pos = emit_pos + 0x1;
        }

        if (scc_level[now_scc] > res->level_max) { res->level_max = scc_level[now_scc]; }

        if (scc_iter[now_scc] == LDG_TRUTH_TRUE)
        {
            if (once_cunt > 0)
            {
                res->units[res->unit_cunt].start = once_start;
                res->units[res->unit_cunt].cunt = once_cunt;
                res->units[res->unit_cunt].iter_is = 0;
                res->unit_cunt = res->unit_cunt + 0x1;
            }

            res->units[res->unit_cunt].start = emit_pos - member_cunt;
            res->units[res->unit_cunt].cunt = member_cunt;
            res->units[res->unit_cunt].iter_is = 0x1;
            res->unit_cunt = res->unit_cunt + 0x1;
            res->iter_op_cunt = res->iter_op_cunt + member_cunt;
            once_start = emit_pos;
            once_cunt = 0;
        }
        else { once_cunt = once_cunt + member_cunt; }

        emit_scc_cunt = emit_scc_cunt + 0x1;

        for (epos = cond_base[now_scc]; epos < cond_base[now_scc + 0x1]; epos++)
        {
            uint32_t succ_scc = cond_succ[epos];

            if (scc_level[now_scc] + 0x1 > scc_level[succ_scc]) { scc_level[succ_scc] = scc_level[now_scc] + 0x1; }

            cond_indeg[succ_scc] = cond_indeg[succ_scc] - 0x1;

            if (cond_indeg[succ_scc] == 0) { crosstalk_hdl_level_heap_push(heap, &heap_cunt, scc_key, succ_scc); }
        }
    }

    if (once_cunt > 0)
    {
        res->units[res->unit_cunt].start = once_start;
        res->units[res->unit_cunt].cunt = once_cunt;
        res->units[res->unit_cunt].iter_is = 0;
        res->unit_cunt = res->unit_cunt + 0x1;
    }

    if (LDG_UNLIKELY(emit_scc_cunt != scc_cunt || emit_pos != op_cunt)) { return CROSSTALK_HDL_ERR_LEVEL; }

    return LDG_ERR_AOK;
}

static uint8_t crosstalk_hdl_level_set_has(const uint32_t *set_arr, uint32_t set_cunt, uint32_t sig)
{
    uint32_t idx = 0;

    for (idx = 0; idx < set_cunt; idx++) { if (set_arr[idx] == sig) { return LDG_TRUTH_TRUE; } }

    return LDG_TRUTH_FALSE;
}

uint8_t crosstalk_hdl_level_fast_is(const crosstalk_hdl_ir_t *ir_ctx, uint32_t clk_sig, uint32_t *alias_sigs, uint32_t alias_cap, uint32_t *out_alias_cunt)
{
    uint32_t set_arr[CROSSTALK_HDL_LEVEL_FAST_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t rd_sigs[4] = LDG_ARR_ZERO_INIT;
    uint32_t set_cunt = 0;
    uint32_t rd_cunt = 0;
    uint32_t idx = 0;
    uint32_t sub = 0;
    uint32_t wsig = 0;
    uint8_t grew = 0;

    if (LDG_UNLIKELY(ir_ctx == 0x0 || alias_sigs == 0x0 || out_alias_cunt == 0x0)) { return LDG_TRUTH_FALSE; }

    *out_alias_cunt = 0;

    if (clk_sig >= ir_ctx->sig_cunt) { return LDG_TRUTH_FALSE; }

    set_arr[0] = clk_sig;
    set_cunt = 0x1;
    grew = LDG_TRUTH_TRUE;

    while (grew == LDG_TRUTH_TRUE)
    {
        grew = LDG_TRUTH_FALSE;

        for (idx = 0; idx < ir_ctx->comb_cunt; idx++)
        {
            const crosstalk_hdl_ir_op_t *op = &ir_ctx->comb_ops[idx];
            uint32_t wr_cunt = 0;
            uint32_t sweep = 0;

            if (op->kind != CROSSTALK_HDL_IR_OP_COPY && op->kind != CROSSTALK_HDL_IR_OP_ZEXT && (op->kind != CROSSTALK_HDL_IR_OP_SLICE || op->imm != 0)) { continue; }

            if (op->dst == UINT32_MAX || op->src0 == UINT32_MAX) { continue; }

            if (crosstalk_hdl_level_set_has(set_arr, set_cunt, op->src0) != LDG_TRUTH_TRUE) { continue; }

            if (crosstalk_hdl_level_set_has(set_arr, set_cunt, op->dst) == LDG_TRUTH_TRUE) { continue; }

            for (sweep = 0; sweep < ir_ctx->comb_cunt; sweep++) { if (crosstalk_hdl_level_wsig_get(&ir_ctx->comb_ops[sweep]) == op->dst) { wr_cunt = wr_cunt + 0x1; } }

            for (sweep = 0; sweep < ir_ctx->seq_op_cunt; sweep++) { if (crosstalk_hdl_level_wsig_get(&ir_ctx->seq_ops[sweep]) == op->dst) { wr_cunt = wr_cunt + 0x2; } }

            if (wr_cunt != 0x1) { continue; }

            if (set_cunt >= CROSSTALK_HDL_LEVEL_FAST_CAP) { return LDG_TRUTH_FALSE; }

            set_arr[set_cunt] = op->dst;
            set_cunt = set_cunt + 0x1;
            grew = LDG_TRUTH_TRUE;
        }
    }

    for (idx = 0; idx < ir_ctx->seq_blk_cunt; idx++)
    {
        const crosstalk_hdl_ir_seq_blk_t *blk = &ir_ctx->seq_blks[idx];

        if (blk->clk_sig == UINT32_MAX || blk->clk_edge != CROSSTALK_HDL_IR_EDGE_POS) { return LDG_TRUTH_FALSE; }

        if (crosstalk_hdl_level_set_has(set_arr, set_cunt, blk->clk_sig) != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

        if (blk->rst_sig != UINT32_MAX && crosstalk_hdl_level_set_has(set_arr, set_cunt, blk->rst_sig) == LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }
    }

    for (idx = 0; idx < ir_ctx->seq_op_cunt; idx++) { if (crosstalk_hdl_level_set_has(set_arr, set_cunt, crosstalk_hdl_level_wsig_get(&ir_ctx->seq_ops[idx])) == LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; } }

    for (idx = 0; idx < ir_ctx->comb_cunt; idx++)
    {
        const crosstalk_hdl_ir_op_t *op = &ir_ctx->comb_ops[idx];

        wsig = crosstalk_hdl_level_wsig_get(op);

        if (wsig != UINT32_MAX && crosstalk_hdl_level_set_has(set_arr, set_cunt, wsig) == LDG_TRUTH_TRUE)
        {
            if (wsig == clk_sig) { return LDG_TRUTH_FALSE; }

            continue;
        }

        rd_cunt = crosstalk_hdl_level_rds_get(op, rd_sigs);

        for (sub = 0; sub < rd_cunt; sub++)
        {
            if (rd_sigs[sub] == UINT32_MAX) { continue; }

            if (crosstalk_hdl_level_set_has(set_arr, set_cunt, rd_sigs[sub]) == LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }
        }
    }

    if (set_cunt - 0x1 > alias_cap) { return LDG_TRUTH_FALSE; }

    for (idx = 0x1; idx < set_cunt; idx++) { alias_sigs[idx - 0x1] = set_arr[idx]; }

    *out_alias_cunt = set_cunt - 0x1;

    return LDG_TRUTH_TRUE;
}
