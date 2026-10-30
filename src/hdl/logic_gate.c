#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>

#include "hdl/logic_gate.h"

static uint32_t crosstalk_hdl_logic_gate_arr_alloc(void *pool, uint64_t cunt, uint64_t item_size, void **out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(pool == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;
    if (cunt == 0) { cunt = 0x1; }

    ret = ldg_mem_pool_alloc((ldg_mem_pool_t *)pool, cunt * item_size, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    memset(*out, 0x0, cunt * item_size);
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_logic_gate_lit_bit_get(const crosstalk_hdl_logic_gate_ctx_t *ctx, uint32_t lit)
{
    return (lit & 0x1) ^ (uint32_t)ctx->node_val[lit >> 0x1];
}

static uint64_t crosstalk_hdl_logic_gate_roots_word_get(const crosstalk_hdl_logic_gate_ctx_t *ctx, uint32_t off, uint32_t width)
{
    uint64_t word = 0;
    uint32_t bit_idx = 0;

    for (bit_idx = 0; bit_idx < width; bit_idx = bit_idx + 0x1) { word = word | ((uint64_t)crosstalk_hdl_logic_gate_lit_bit_get(ctx, ctx->aig_ctx->roots[off + bit_idx]) << bit_idx); }

    return word;
}

uint32_t crosstalk_hdl_logic_gate_init(crosstalk_hdl_logic_gate_ctx_t *ctx, const crosstalk_hdl_aig_ctx_t *aig_ctx, const crosstalk_hdl_netarr_t *netarr, void *pool)
{
    const crosstalk_hdl_ir_t *ir_ctx = 0x0;
    uint64_t mem_op_total = 0;
    uint32_t sig_idx = 0;
    uint32_t bit_idx = 0;
    uint32_t root_at = 0;
    uint32_t fill_at = 0;
    uint32_t op_idx = 0;
    uint32_t pi_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || aig_ctx == 0x0 || netarr == 0x0 || pool == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));
    ctx->aig_ctx = aig_ctx;
    ctx->netarr = netarr;
    ir_ctx = aig_ctx->ir_ctx;

    for (sig_idx = 0; sig_idx < ir_ctx->sig_cunt; sig_idx = sig_idx + 0x1)
    {
        if (aig_ctx->lit_off[sig_idx] == UINT32_MAX) { continue; }

        if (ir_ctx->sigs[sig_idx].width > (uint32_t)CROSSTALK_HDL_AIG_WIDTH_MAX) { return CROSSTALK_HDL_ERR_LOGIC_GATE_WIDTH; }
    }

    ret = crosstalk_hdl_logic_gate_arr_alloc(pool, aig_ctx->node_cunt, sizeof(uint8_t), (void **)&ctx->node_val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (sig_idx = 0; sig_idx < ir_ctx->sig_cunt; sig_idx = sig_idx + 0x1)
    {
        if (aig_ctx->latch_is[sig_idx] == LDG_TRUTH_FALSE || aig_ctx->last_wr[sig_idx] == UINT32_MAX || aig_ctx->op_placed[aig_ctx->last_wr[sig_idx]] == LDG_TRUTH_FALSE) { continue; }

        ctx->latch_cunt = ctx->latch_cunt + ir_ctx->sigs[sig_idx].width;
    }

    ret = crosstalk_hdl_logic_gate_arr_alloc(pool, ctx->latch_cunt, sizeof(uint32_t), (void **)&ctx->latch_pi_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_logic_gate_arr_alloc(pool, ctx->latch_cunt, sizeof(uint32_t), (void **)&ctx->latch_root_lit);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    root_at = aig_ctx->latch_root_base;
    fill_at = 0;
    for (sig_idx = 0; sig_idx < ir_ctx->sig_cunt; sig_idx = sig_idx + 0x1)
    {
        if (aig_ctx->latch_is[sig_idx] == LDG_TRUTH_FALSE || aig_ctx->last_wr[sig_idx] == UINT32_MAX || aig_ctx->op_placed[aig_ctx->last_wr[sig_idx]] == LDG_TRUTH_FALSE) { continue; }

        for (bit_idx = 0; bit_idx < ir_ctx->sigs[sig_idx].width; bit_idx = bit_idx + 0x1)
        {
            uint32_t found = (uint32_t)UINT32_MAX;

            ctx->latch_root_lit[fill_at] = aig_ctx->roots[root_at];
            root_at = root_at + 0x1;
            for (pi_idx = 0; pi_idx < aig_ctx->pi_cunt; pi_idx = pi_idx + 0x1)
            {
                if (aig_ctx->pi_kind[pi_idx] != (uint8_t)CROSSTALK_HDL_AIG_PI_KIND_LATCH) { continue; }

                if (aig_ctx->pi_sig[pi_idx] != sig_idx || aig_ctx->pi_bit[pi_idx] != bit_idx) { continue; }

                found = aig_ctx->pi_node[pi_idx] >> 0x1;
                break;
            }

            if (found == (uint32_t)UINT32_MAX) { return CROSSTALK_HDL_ERR_LOGIC_GATE_BIND; }

            ctx->latch_pi_node[fill_at] = found;
            fill_at = fill_at + 0x1;
        }
    }

    for (sig_idx = 0; sig_idx < ir_ctx->sig_cunt; sig_idx = sig_idx + 0x1) { if (aig_ctx->ff_off[sig_idx] != UINT32_MAX) { ctx->ff_cunt = ctx->ff_cunt + 0x1; } }

    ret = crosstalk_hdl_logic_gate_arr_alloc(pool, ctx->ff_cunt, sizeof(uint32_t), (void **)&ctx->ff_sig);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_logic_gate_arr_alloc(pool, ctx->ff_cunt, sizeof(uint32_t), (void **)&ctx->ff_root_off);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    root_at = aig_ctx->ff_root_base;
    fill_at = 0;
    for (sig_idx = 0; sig_idx < ir_ctx->sig_cunt; sig_idx = sig_idx + 0x1)
    {
        if (aig_ctx->ff_off[sig_idx] != UINT32_MAX)
        {
            ctx->ff_sig[fill_at] = sig_idx;
            ctx->ff_root_off[fill_at] = root_at;
            root_at = root_at + ir_ctx->sigs[sig_idx].width;
            fill_at = fill_at + 0x1;
        }

        if (aig_ctx->lit_off[sig_idx] != UINT32_MAX && (ir_ctx->sigs[sig_idx].port == CROSSTALK_HDL_IR_PORT_OUT || ir_ctx->sigs[sig_idx].port == CROSSTALK_HDL_IR_PORT_INOUT)) { root_at = root_at + ir_ctx->sigs[sig_idx].width; }
    }

    mem_op_total = (uint64_t)aig_ctx->mem_rd_cunt + (uint64_t)aig_ctx->mem_wr_cunt;
    ret = crosstalk_hdl_logic_gate_arr_alloc(pool, mem_op_total, sizeof(crosstalk_hdl_logic_gate_mem_op_t), (void **)&ctx->mem_ops);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    fill_at = 0;
    for (op_idx = 0; op_idx < ir_ctx->comb_cunt + ir_ctx->seq_op_cunt; op_idx = op_idx + 0x1)
    {
        const crosstalk_hdl_ir_op_t *op = op_idx < ir_ctx->comb_cunt ? &ir_ctx->comb_ops[op_idx] : &ir_ctx->seq_ops[op_idx - ir_ctx->comb_cunt];
        crosstalk_hdl_logic_gate_mem_op_t *ent = 0x0;

        if (aig_ctx->op_root_off[op_idx] == UINT32_MAX) { continue; }

        if (op->kind != CROSSTALK_HDL_IR_OP_MEM_RD && op->kind != CROSSTALK_HDL_IR_OP_MEM_WR) { continue; }

        if (LDG_UNLIKELY(fill_at >= mem_op_total)) { return CROSSTALK_HDL_ERR_LOGIC_GATE_BIND; }

        ent = &ctx->mem_ops[fill_at];
        ent->mem_sig = op->src0;
        ent->addr_w = op->src1 != UINT32_MAX ? ir_ctx->sigs[op->src1].width : 0x0;
        if (op->kind == CROSSTALK_HDL_IR_OP_MEM_WR)
        {
            uint32_t mem_w = ir_ctx->sigs[op->src0].width;

            ent->wr_is = LDG_TRUTH_TRUE;
            ent->en_off = aig_ctx->op_root_off[op_idx];
            ent->addr_off = ent->en_off + 0x1;
            ent->val_off = ent->addr_off + ent->addr_w;
            ent->val_w = op->src2 != UINT32_MAX ? ir_ctx->sigs[op->src2].width : 0x0;
            ent->val_mask = mem_w >= (uint32_t)CROSSTALK_HDL_AIG_WIDTH_MAX ? UINT64_MAX : (((uint64_t)0x1 << mem_w) - 0x1);
        }
        else
        {
            ent->wr_is = LDG_TRUTH_FALSE;
            ent->addr_off = aig_ctx->op_root_off[op_idx];
            ent->pi_base = aig_ctx->op_pi_base[op_idx];
            ent->dst_w = ir_ctx->sigs[op->dst].width;
        }

        fill_at = fill_at + 0x1;
    }

    ctx->mem_op_cunt = fill_at;
    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_logic_gate_cycle_run(crosstalk_hdl_logic_gate_ctx_t *ctx, crosstalk_hdl_exec_ctx_t *exec, uint32_t clk_sig)
{
    const crosstalk_hdl_aig_ctx_t *aig_ctx = 0x0;
    const crosstalk_hdl_ir_t *ir_ctx = 0x0;
    uint32_t pass = 0;
    uint32_t lut_idx = 0;
    uint32_t pi_idx = 0;
    uint32_t mem_idx = 0;
    uint32_t sig_idx = 0;
    uint32_t bit_idx = 0;
    uint32_t ret = 0;
    uint8_t changed = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0 || exec == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    aig_ctx = ctx->aig_ctx;
    ir_ctx = aig_ctx->ir_ctx;
    if (LDG_UNLIKELY(clk_sig >= ir_ctx->sig_cunt)) { return LDG_ERR_BOUNDS; }

    for (pi_idx = 0; pi_idx < aig_ctx->pi_cunt; pi_idx = pi_idx + 0x1)
    {
        if (aig_ctx->pi_kind[pi_idx] == (uint8_t)CROSSTALK_HDL_AIG_PI_KIND_MEM_RD) { continue; }

        ctx->node_val[aig_ctx->pi_node[pi_idx] >> 0x1] = (uint8_t)((exec->vals[aig_ctx->pi_sig[pi_idx]] >> aig_ctx->pi_bit[pi_idx]) & 0x1);
    }

    for (pass = 0; pass < (uint32_t)CROSSTALK_HDL_LOGIC_GATE_SETTLE_MAX; pass = pass + 0x1)
    {
        for (lut_idx = 0; lut_idx < ctx->netarr->lut_cunt; lut_idx = lut_idx + 0x1)
        {
            const crosstalk_hdl_netarr_lut_t *lut = &ctx->netarr->luts[lut_idx];
            uint32_t pick = 0;
            uint32_t in_idx = 0;

            for (in_idx = 0; in_idx < lut->in_cunt; in_idx = in_idx + 0x1) { pick = pick | ((uint32_t)ctx->node_val[lut->in_node[in_idx]] << in_idx); }

            ctx->node_val[lut->out_node] = (uint8_t)((lut->mask >> pick) & 0x1);
        }

        changed = LDG_TRUTH_FALSE;
        for (mem_idx = 0; mem_idx < ctx->mem_op_cunt; mem_idx = mem_idx + 0x1)
        {
            const crosstalk_hdl_logic_gate_mem_op_t *ent = &ctx->mem_ops[mem_idx];
            uint64_t addr = 0;
            uint64_t val = 0;

            if (ent->wr_is == LDG_TRUTH_TRUE) { continue; }

            addr = crosstalk_hdl_logic_gate_roots_word_get(ctx, ent->addr_off, ent->addr_w);
            ret = crosstalk_hdl_exec_mem_ld(exec, ent->mem_sig, addr, &val);
            if (ret != LDG_ERR_AOK) { val = 0; }

            for (bit_idx = 0; bit_idx < ent->dst_w; bit_idx = bit_idx + 0x1)
            {
                uint32_t node = aig_ctx->pi_node[ent->pi_base + bit_idx] >> 0x1;
                uint8_t nxt = (uint8_t)((val >> bit_idx) & 0x1);

                if (ctx->node_val[node] != nxt)
                {
                    ctx->node_val[node] = nxt;
                    changed = LDG_TRUTH_TRUE;
                }
            }
        }

        for (bit_idx = 0; bit_idx < ctx->latch_cunt; bit_idx = bit_idx + 0x1)
        {
            uint8_t nxt = (uint8_t)crosstalk_hdl_logic_gate_lit_bit_get(ctx, ctx->latch_root_lit[bit_idx]);

            if (ctx->node_val[ctx->latch_pi_node[bit_idx]] != nxt)
            {
                ctx->node_val[ctx->latch_pi_node[bit_idx]] = nxt;
                changed = LDG_TRUTH_TRUE;
            }
        }

        ctx->pass_cunt = ctx->pass_cunt + 0x1;
        if (changed == LDG_TRUTH_FALSE) { break; }
    }

    if (LDG_UNLIKELY(changed == LDG_TRUTH_TRUE)) { return CROSSTALK_HDL_ERR_LOGIC_GATE_SETTLE; }

    for (sig_idx = 0; sig_idx < ir_ctx->sig_cunt; sig_idx = sig_idx + 0x1)
    {
        uint64_t word = 0;
        uint32_t off = aig_ctx->lit_off[sig_idx];

        if (off == UINT32_MAX) { continue; }

        for (bit_idx = 0; bit_idx < ir_ctx->sigs[sig_idx].width; bit_idx = bit_idx + 0x1) { word = word | ((uint64_t)crosstalk_hdl_logic_gate_lit_bit_get(ctx, aig_ctx->lit_arena[off + bit_idx]) << bit_idx); }

        exec->vals[sig_idx] = word;
    }

    for (mem_idx = 0; mem_idx < ctx->mem_op_cunt; mem_idx = mem_idx + 0x1)
    {
        const crosstalk_hdl_logic_gate_mem_op_t *ent = &ctx->mem_ops[mem_idx];
        uint64_t addr = 0;
        uint64_t val = 0;

        if (ent->wr_is != LDG_TRUTH_TRUE) { continue; }

        if (crosstalk_hdl_logic_gate_lit_bit_get(ctx, aig_ctx->roots[ent->en_off]) == 0) { continue; }

        addr = crosstalk_hdl_logic_gate_roots_word_get(ctx, ent->addr_off, ent->addr_w);
        val = crosstalk_hdl_logic_gate_roots_word_get(ctx, ent->val_off, ent->val_w) & ent->val_mask;
        ret = crosstalk_hdl_exec_mem_st(exec, ent->mem_sig, addr, val);
        if (ret != LDG_ERR_AOK) { continue; }
    }

    for (sig_idx = 0; sig_idx < ctx->ff_cunt; sig_idx = sig_idx + 0x1) { exec->vals[ctx->ff_sig[sig_idx]] = crosstalk_hdl_logic_gate_roots_word_get(ctx, ctx->ff_root_off[sig_idx], ir_ctx->sigs[ctx->ff_sig[sig_idx]].width); }

    exec->vals[clk_sig] = 0x0;
    exec->tick_cunt = exec->tick_cunt + 0x1;
    ctx->cycle_cunt = ctx->cycle_cunt + 0x1;

    return LDG_ERR_AOK;
}
