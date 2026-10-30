#include <stdint.h>
#include <string.h>

#include <dangling/core/arith.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/mem.h>

#include "hdl/elab.h"

#define CROSSTALK_HDL_ELAB_CONN_CAP 64
#define CROSSTALK_HDL_ELAB_PARAM_CAP 32
#define CROSSTALK_HDL_ELAB_QUAL_PATH_CAP 192
#define CROSSTALK_HDL_ELAB_QUAL_SEP '.'
#define CROSSTALK_HDL_ELAB_QUAL_SEP_SIZE 1
#define CROSSTALK_HDL_ELAB_GEN_OPEN '['
#define CROSSTALK_HDL_ELAB_GEN_CLOSE ']'
#define CROSSTALK_HDL_ELAB_GEN_BRACKET_SIZE 2
#define CROSSTALK_HDL_ELAB_GEN_SUFFIX_CAP (CROSSTALK_HDL_ELAB_GEN_DEPTH_MAX * CROSSTALK_HDL_ELAB_GEN_IDX_CAP)
#define CROSSTALK_HDL_ELAB_LV_NONE 0
#define CROSSTALK_HDL_ELAB_LV_SIG 1
#define CROSSTALK_HDL_ELAB_LV_PART 2
#define CROSSTALK_HDL_ELAB_LV_PART_DYN 3
#define CROSSTALK_HDL_ELAB_LV_MEM 4
#define CROSSTALK_HDL_ELAB_LV_MEM_PART 5
#define CROSSTALK_HDL_ELAB_SYSTASK_DISPLAY_NAME "$display"
#define CROSSTALK_HDL_ELAB_SYSTASK_WR_NAME "$write"
#define CROSSTALK_HDL_ELAB_SYSTASK_FINISH_NAME "$finish"

typedef struct crosstalk_hdl_elab_lv
{
    uint64_t off;
    uint32_t kind;
    uint32_t sig;
    uint32_t off_sig;
    uint32_t addr_sig;
    uint32_t width;
    uint8_t pudding[4];
} crosstalk_hdl_elab_lv_t;

typedef struct crosstalk_hdl_elab_conn
{
    crosstalk_hdl_tok_t name;
    crosstalk_hdl_elab_lv_t lv;
    uint32_t pattern_sig[CROSSTALK_HDL_ELAB_PATTERN_CAP];
    uint32_t pattern_w[CROSSTALK_HDL_ELAB_PATTERN_CAP];
    uint32_t pattern_cunt;
    uint32_t val_sig;
    uint32_t val_w;
    uint32_t styp;
    uint32_t base;
    uint32_t mem_sig;
    uint32_t mem_inner;
} crosstalk_hdl_elab_conn_t;

typedef struct crosstalk_hdl_elab_pbind
{
    crosstalk_hdl_tok_t name;
    uint64_t val;
    uint8_t str_is;
    uint8_t pudding[7];
} crosstalk_hdl_elab_pbind_t;

static uint32_t crosstalk_hdl_elab_stmt(crosstalk_hdl_elab_ctx_t *ctx, uint32_t node_idx);
static uint32_t crosstalk_hdl_elab_module(crosstalk_hdl_elab_ctx_t *ctx, uint32_t mod_node, const crosstalk_hdl_elab_pbind_t *pbinds, uint32_t pbind_cunt, crosstalk_hdl_elab_conn_t *conns, uint32_t conn_cunt);
static uint32_t crosstalk_hdl_elab_item(crosstalk_hdl_elab_ctx_t *ctx, uint32_t node_idx);

static uint32_t crosstalk_hdl_elab_child_get(const crosstalk_hdl_elab_ctx_t *ctx, uint32_t node_idx, uint32_t *out_idx, const crosstalk_hdl_ast_node_t **out_node)
{
    const crosstalk_hdl_ast_node_t *node = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    ret = crosstalk_hdl_ast_node_get(ctx->ast, node_idx, &node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_idx = node->first_child;

    if (out_node != 0x0 && node->first_child != UINT32_MAX) { return crosstalk_hdl_ast_node_get(ctx->ast, node->first_child, out_node); }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_elab_type_width_get(crosstalk_hdl_elab_ctx_t *ctx, uint32_t type_idx, uint32_t *out_width)
{
    const crosstalk_hdl_ast_node_t *type_node = 0x0;
    const crosstalk_hdl_ast_node_t *range_node = 0x0;
    const crosstalk_hdl_ast_node_t *lsb_node = 0x0;
    uint64_t msb_val = 0;
    uint64_t lsb_val = 0;
    uint64_t width = 0x1;
    uint32_t child_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_width == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_width = 0x1;

    if (type_idx == UINT32_MAX) { return LDG_ERR_AOK; }

    ret = crosstalk_hdl_ast_node_get(ctx->ast, type_idx, &type_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (crosstalk_hdl_elab_tok_lit_eq_is(ctx->src, &type_node->tok, "in" "t") == LDG_TRUTH_TRUE || crosstalk_hdl_elab_tok_lit_eq_is(ctx->src, &type_node->tok, "in" "teger") == LDG_TRUTH_TRUE) { width = 32; }

    {
        uint8_t range_has = LDG_TRUTH_FALSE;

        child_idx = type_node->first_child;
        while (child_idx != UINT32_MAX)
        {
            ret = crosstalk_hdl_ast_node_get(ctx->ast, child_idx, &range_node);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (range_node->kind == CROSSTALK_HDL_NODE_RANGE)
            {
                ret = crosstalk_hdl_elab_const_eval(ctx, range_node->first_child, &msb_val);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                ret = crosstalk_hdl_ast_node_get(ctx->ast, range_node->first_child, &lsb_node);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                ret = crosstalk_hdl_elab_const_eval(ctx, lsb_node->next_sibling, &lsb_val);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                if (msb_val >= lsb_val) { width = msb_val - lsb_val + 0x1; }
                else { width = lsb_val - msb_val + 0x1; }

                range_has = LDG_TRUTH_TRUE;
            }

            child_idx = range_node->next_sibling;
        }

        if (range_has == LDG_TRUTH_FALSE && type_node->tok.kind == CROSSTALK_HDL_TOK_IDENT)
        {
            uint32_t type_sym = 0;

            ret = crosstalk_hdl_elab_sym_find(ctx, &type_node->tok, &type_sym);
            if (ret == LDG_ERR_AOK && ctx->syms[type_sym].kind == CROSSTALK_HDL_ELAB_SYM_TYPE) { width = ctx->syms[type_sym].width; }
        }
    }

    if (LDG_UNLIKELY(width == 0 || width > CROSSTALK_HDL_ELAB_WIDTH_MAX)) { return CROSSTALK_HDL_ERR_ELAB_WIDTH; }

    *out_width = (uint32_t)width;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_elab_guard_mix(crosstalk_hdl_elab_ctx_t *ctx, uint32_t cond_sig, uint8_t invert, uint32_t *out_guard)
{
    crosstalk_hdl_ir_op_t op = LDG_STRUCT_ZERO_INIT;
    uint32_t cond_use = cond_sig;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_guard == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_guard = UINT32_MAX;

    if (invert == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_elab_tmp_make(ctx, 0x1, &cond_use);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        op.kind = CROSSTALK_HDL_IR_OP_LNOT;
        op.dst = cond_use;
        op.src0 = cond_sig;
        op.src1 = UINT32_MAX;
        op.src2 = UINT32_MAX;
        op.width = 0x1;

        ret = crosstalk_hdl_elab_op_emit(ctx, &op);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (ctx->guard == UINT32_MAX)
    {
        *out_guard = cond_use;

        return LDG_ERR_AOK;
    }

    ret = crosstalk_hdl_elab_tmp_make(ctx, 0x1, out_guard);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    op.kind = CROSSTALK_HDL_IR_OP_LAND;
    op.dst = *out_guard;
    op.src0 = ctx->guard;
    op.src1 = cond_use;
    op.src2 = UINT32_MAX;
    op.width = 0x1;

    return crosstalk_hdl_elab_op_emit(ctx, &op);
}

uint32_t crosstalk_hdl_elab_member_resolve(crosstalk_hdl_elab_ctx_t *ctx, uint32_t dot_idx, uint32_t *out_sig, uint32_t *out_width, uint32_t *out_addr_sig)
{
    const crosstalk_hdl_ast_node_t *dot_node = 0x0;
    const crosstalk_hdl_ast_node_t *base_node = 0x0;
    const crosstalk_hdl_ast_node_t *ident_node = 0x0;
    const crosstalk_hdl_elab_styp_t *styp = 0x0;
    uint64_t depth = 0;
    uint32_t sym_idx = 0;
    uint32_t styp_idx = 0;
    uint32_t fld_idx = 0;
    uint32_t addr_sig = UINT32_MAX;
    uint32_t addr_w = 0;
    uint32_t ret = 0;
    uint8_t addr_sgn = 0;
    uint8_t two_d = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0 || out_sig == 0x0 || out_width == 0x0 || out_addr_sig == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_sig = UINT32_MAX;
    *out_width = 0;
    *out_addr_sig = UINT32_MAX;

    ret = crosstalk_hdl_ast_node_get(ctx->ast, dot_idx, &dot_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_ast_node_get(ctx->ast, dot_node->first_child, &base_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (base_node->kind == CROSSTALK_HDL_NODE_EXPR_DOT)
    {
        const crosstalk_hdl_ast_node_t *inner_base = 0x0;
        const crosstalk_hdl_elab_styp_t *par_styp = 0x0;
        uint32_t grp_idx = 0;

        ret = crosstalk_hdl_ast_node_get(ctx->ast, base_node->first_child, &inner_base);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (inner_base->kind != CROSSTALK_HDL_NODE_EXPR_IDENT) { return LDG_ERR_NOT_FOUND; }

        ret = crosstalk_hdl_elab_sym_find(ctx, &inner_base->tok, &sym_idx);
        if (ret != LDG_ERR_AOK) { return LDG_ERR_NOT_FOUND; }

        if (ctx->syms[sym_idx].kind != CROSSTALK_HDL_ELAB_SYM_SVAR) { return LDG_ERR_NOT_FOUND; }

        if (LDG_UNLIKELY(ctx->syms[sym_idx].val >> 32 != 0))
        {
            if (dot_node->tok.line != 0) { ctx->err_tok = dot_node->tok; }

            return CROSSTALK_HDL_ERR_ELAB;
        }

        par_styp = &ctx->styps[(uint32_t)ctx->syms[sym_idx].val];

        for (grp_idx = 0; grp_idx < par_styp->group_cunt; grp_idx++)
        {
            const crosstalk_hdl_elab_sfield_t *grp = &ctx->sfields[par_styp->group_base + grp_idx];
            const crosstalk_hdl_elab_styp_t *sub = 0x0;

            if (crosstalk_hdl_elab_tok_eq_is(ctx->src, &grp->name, &base_node->tok) != LDG_TRUTH_TRUE) { continue; }

            sub = &ctx->styps[grp->sub_styp];

            for (fld_idx = 0; fld_idx < sub->field_cunt; fld_idx++)
            {
                const crosstalk_hdl_elab_sfield_t *leaf = &ctx->sfields[sub->field_base + fld_idx];

                if (crosstalk_hdl_elab_tok_eq_is(ctx->src, &leaf->name, &dot_node->tok) == LDG_TRUTH_TRUE)
                {
                    *out_sig = ctx->syms[sym_idx].sig + grp->sub_leaf_base + fld_idx;
                    *out_width = leaf->width;

                    return LDG_ERR_AOK;
                }
            }

            break;
        }

        if (dot_node->tok.line != 0) { ctx->err_tok = dot_node->tok; }

        return CROSSTALK_HDL_ERR_ELAB;
    }

    if (base_node->kind == CROSSTALK_HDL_NODE_EXPR_IDX)
    {
        ret = crosstalk_hdl_ast_node_get(ctx->ast, base_node->first_child, &ident_node);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (ident_node->kind == CROSSTALK_HDL_NODE_EXPR_IDX)
        {
            two_d = LDG_TRUTH_TRUE;

            ret = crosstalk_hdl_ast_node_get(ctx->ast, ident_node->first_child, &ident_node);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }
    else { ident_node = base_node; }

    if (ident_node->kind != CROSSTALK_HDL_NODE_EXPR_IDENT) { return LDG_ERR_NOT_FOUND; }

    ret = crosstalk_hdl_elab_sym_find(ctx, &ident_node->tok, &sym_idx);
    if (ret != LDG_ERR_AOK) { return LDG_ERR_NOT_FOUND; }

    if (ctx->syms[sym_idx].kind != CROSSTALK_HDL_ELAB_SYM_SVAR) { return LDG_ERR_NOT_FOUND; }

    styp_idx = (uint32_t)ctx->syms[sym_idx].val;
    depth = ctx->syms[sym_idx].val >> 32;
    styp = &ctx->styps[styp_idx];

    if (base_node->kind == CROSSTALK_HDL_NODE_EXPR_IDX)
    {
        if (LDG_UNLIKELY(depth == 0)) { ctx->err_tok = dot_node->tok; return CROSSTALK_HDL_ERR_ELAB; }

        if (two_d == LDG_TRUTH_TRUE)
        {
            const crosstalk_hdl_ast_node_t *inner_node = 0x0;
            crosstalk_hdl_ir_op_t addr_op = LDG_STRUCT_ZERO_INIT;
            uint64_t dim_two = (uint64_t)ctx->syms[sym_idx].width;
            uint32_t row_sig = 0;
            uint32_t col_sig = 0;
            uint32_t dim_sig = 0;
            uint32_t mul_sig = 0;

            if (LDG_UNLIKELY(dim_two == 0)) { ctx->err_tok = dot_node->tok; return CROSSTALK_HDL_ERR_ELAB; }

            ret = crosstalk_hdl_ast_node_get(ctx->ast, base_node->first_child, &inner_node);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_elab_expr(ctx, ident_node->next_sibling, &row_sig, &addr_w, &addr_sgn);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_elab_expr(ctx, inner_node->next_sibling, &col_sig, &addr_w, &addr_sgn);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_elab_const_sig(ctx, dim_two, 32, &dim_sig);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_elab_tmp_make(ctx, 32, &mul_sig);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            addr_op.kind = CROSSTALK_HDL_IR_OP_MUL;
            addr_op.dst = mul_sig;
            addr_op.src0 = row_sig;
            addr_op.src1 = dim_sig;
            addr_op.src2 = UINT32_MAX;
            addr_op.imm = 0;
            addr_op.width = 32;

            ret = crosstalk_hdl_elab_op_emit(ctx, &addr_op);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_elab_tmp_make(ctx, 32, &addr_sig);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            addr_op.kind = CROSSTALK_HDL_IR_OP_ADD;
            addr_op.dst = addr_sig;
            addr_op.src0 = mul_sig;
            addr_op.src1 = col_sig;
            addr_op.src2 = UINT32_MAX;
            addr_op.imm = 0;
            addr_op.width = 32;

            ret = crosstalk_hdl_elab_op_emit(ctx, &addr_op);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
        else
        {
            ret = crosstalk_hdl_ast_node_get(ctx->ast, base_node->first_child, &ident_node);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_elab_expr(ctx, ident_node->next_sibling, &addr_sig, &addr_w, &addr_sgn);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        *out_addr_sig = addr_sig;
    }
    else if (LDG_UNLIKELY(depth > 0)) { ctx->err_tok = dot_node->tok; return CROSSTALK_HDL_ERR_ELAB; }

    for (fld_idx = 0; fld_idx < styp->field_cunt; fld_idx++)
    {
        const crosstalk_hdl_elab_sfield_t *fld = &ctx->sfields[styp->field_base + fld_idx];

        if (crosstalk_hdl_elab_tok_eq_is(ctx->src, &fld->name, &dot_node->tok) == LDG_TRUTH_TRUE)
        {
            *out_sig = ctx->syms[sym_idx].sig + fld_idx;
            *out_width = fld->width;

            return LDG_ERR_AOK;
        }
    }

    ctx->err_tok = dot_node->tok;

    return CROSSTALK_HDL_ERR_ELAB;
}

static uint32_t crosstalk_hdl_elab_lv_resolve(crosstalk_hdl_elab_ctx_t *ctx, uint32_t node_idx, crosstalk_hdl_elab_lv_t *out_lv)
{
    const crosstalk_hdl_ast_node_t *node = 0x0;
    const crosstalk_hdl_ast_node_t *base_node = 0x0;
    const crosstalk_hdl_ast_node_t *msb_node = 0x0;
    uint64_t msb_val = 0;
    uint64_t lsb_val = 0;
    uint32_t sym_idx = 0;
    uint32_t addr_sig = 0;
    uint32_t addr_w = 0;
    uint32_t ret = 0;
    uint8_t addr_sgn = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_lv == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    out_lv->kind = CROSSTALK_HDL_ELAB_LV_NONE;
    out_lv->off = 0;
    out_lv->sig = UINT32_MAX;
    out_lv->off_sig = UINT32_MAX;
    out_lv->addr_sig = UINT32_MAX;
    out_lv->width = 0;

    ret = crosstalk_hdl_ast_node_get(ctx->ast, node_idx, &node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_DOT)
    {
        uint32_t member_sig = 0;
        uint32_t member_w = 0;
        uint32_t member_addr = UINT32_MAX;

        ret = crosstalk_hdl_elab_member_resolve(ctx, node_idx, &member_sig, &member_w, &member_addr);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            if (node->tok.line != 0) { ctx->err_tok = node->tok; }

            return CROSSTALK_HDL_ERR_ELAB;
        }

        if (member_addr != UINT32_MAX)
        {
            out_lv->kind = CROSSTALK_HDL_ELAB_LV_MEM;
            out_lv->addr_sig = member_addr;
        }
        else { out_lv->kind = CROSSTALK_HDL_ELAB_LV_SIG; }

        out_lv->sig = member_sig;
        out_lv->width = member_w;

        return LDG_ERR_AOK;
    }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_IDENT)
    {
        ret = crosstalk_hdl_elab_sym_find(ctx, &node->tok, &sym_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            if (node->tok.line != 0) { ctx->err_tok = node->tok; }

            return CROSSTALK_HDL_ERR_ELAB;
        }

        if (LDG_UNLIKELY(ctx->syms[sym_idx].kind == CROSSTALK_HDL_ELAB_SYM_PARAM))
        {
            if (node->tok.line != 0) { ctx->err_tok = node->tok; }

            return CROSSTALK_HDL_ERR_ELAB;
        }

        if (LDG_UNLIKELY(ctx->syms[sym_idx].kind == CROSSTALK_HDL_ELAB_SYM_MEM))
        {
            if (node->tok.line != 0) { ctx->err_tok = node->tok; }

            return CROSSTALK_HDL_ERR_ELAB;
        }

        out_lv->kind = CROSSTALK_HDL_ELAB_LV_SIG;
        out_lv->sig = ctx->syms[sym_idx].sig;
        out_lv->width = ctx->syms[sym_idx].width;

        return LDG_ERR_AOK;
    }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_IDX)
    {
        ret = crosstalk_hdl_ast_node_get(ctx->ast, node->first_child, &base_node);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (base_node->kind == CROSSTALK_HDL_NODE_EXPR_DOT)
        {
            uint32_t member_sig = 0;
            uint32_t member_w = 0;
            uint32_t member_addr = UINT32_MAX;
            uint64_t bit_val = 0;

            ret = crosstalk_hdl_elab_member_resolve(ctx, node->first_child, &member_sig, &member_w, &member_addr);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
            {
                if (node->tok.line != 0) { ctx->err_tok = node->tok; }

                return CROSSTALK_HDL_ERR_ELAB;
            }

            if (crosstalk_hdl_elab_const_eval(ctx, base_node->next_sibling, &bit_val) == LDG_ERR_AOK)
            {
                if (member_addr != UINT32_MAX)
                {
                    out_lv->kind = CROSSTALK_HDL_ELAB_LV_MEM_PART;
                    out_lv->addr_sig = member_addr;
                }
                else { out_lv->kind = CROSSTALK_HDL_ELAB_LV_PART; }

                out_lv->sig = member_sig;
                out_lv->off = bit_val;
                out_lv->width = 0x1;

                return LDG_ERR_AOK;
            }

            if (LDG_UNLIKELY(member_addr != UINT32_MAX))
            {
                if (node->tok.line != 0) { ctx->err_tok = node->tok; }

                return CROSSTALK_HDL_ERR_ELAB;
            }

            ret = crosstalk_hdl_elab_expr(ctx, base_node->next_sibling, &addr_sig, &addr_w, &addr_sgn);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            out_lv->kind = CROSSTALK_HDL_ELAB_LV_PART_DYN;
            out_lv->sig = member_sig;
            out_lv->off_sig = addr_sig;
            out_lv->width = 0x1;

            return LDG_ERR_AOK;
        }

        if (base_node->kind == CROSSTALK_HDL_NODE_EXPR_IDX)
        {
            const crosstalk_hdl_ast_node_t *inner_node = 0x0;
            uint64_t bit_val = 0;

            ret = crosstalk_hdl_ast_node_get(ctx->ast, base_node->first_child, &inner_node);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (LDG_UNLIKELY(inner_node->kind != CROSSTALK_HDL_NODE_EXPR_IDENT))
            {
                if (node->tok.line != 0) { ctx->err_tok = node->tok; }

                return CROSSTALK_HDL_ERR_ELAB;
            }

            ret = crosstalk_hdl_elab_sym_find(ctx, &inner_node->tok, &sym_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->err_tok = inner_node->tok; return CROSSTALK_HDL_ERR_ELAB; }

            if (LDG_UNLIKELY(ctx->syms[sym_idx].kind != CROSSTALK_HDL_ELAB_SYM_MEM || ctx->syms[sym_idx].val != 0)) { ctx->err_tok = inner_node->tok; return CROSSTALK_HDL_ERR_ELAB; }

            ret = crosstalk_hdl_elab_expr(ctx, inner_node->next_sibling, &addr_sig, &addr_w, &addr_sgn);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_elab_const_eval(ctx, base_node->next_sibling, &bit_val);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
            {
                if (node->tok.line != 0) { ctx->err_tok = node->tok; }

                return CROSSTALK_HDL_ERR_ELAB;
            }

            out_lv->kind = CROSSTALK_HDL_ELAB_LV_MEM_PART;
            out_lv->sig = ctx->syms[sym_idx].sig;
            out_lv->addr_sig = addr_sig;
            out_lv->off = bit_val;
            out_lv->width = 0x1;

            return LDG_ERR_AOK;
        }

        if (LDG_UNLIKELY(base_node->kind != CROSSTALK_HDL_NODE_EXPR_IDENT))
        {
            if (node->tok.line != 0) { if (node->tok.line != 0) { ctx->err_tok = node->tok; } }

            return CROSSTALK_HDL_ERR_ELAB;
        }

        ret = crosstalk_hdl_elab_sym_find(ctx, &base_node->tok, &sym_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            if (base_node->tok.line != 0) { ctx->err_tok = base_node->tok; }

            return CROSSTALK_HDL_ERR_ELAB;
        }

        if (ctx->syms[sym_idx].kind == CROSSTALK_HDL_ELAB_SYM_MEM)
        {
            ret = crosstalk_hdl_elab_expr(ctx, base_node->next_sibling, &addr_sig, &addr_w, &addr_sgn);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            out_lv->kind = CROSSTALK_HDL_ELAB_LV_MEM;
            out_lv->sig = ctx->syms[sym_idx].sig;
            out_lv->addr_sig = addr_sig;
            out_lv->width = ctx->syms[sym_idx].width;

            return LDG_ERR_AOK;
        }

        ret = crosstalk_hdl_elab_const_eval(ctx, base_node->next_sibling, &msb_val);
        if (ret == LDG_ERR_AOK)
        {
            out_lv->kind = CROSSTALK_HDL_ELAB_LV_PART;
            out_lv->sig = ctx->syms[sym_idx].sig;
            out_lv->off = msb_val;
            out_lv->width = 0x1;

            return LDG_ERR_AOK;
        }

        ret = crosstalk_hdl_elab_expr(ctx, base_node->next_sibling, &addr_sig, &addr_w, &addr_sgn);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        out_lv->kind = CROSSTALK_HDL_ELAB_LV_PART_DYN;
        out_lv->sig = ctx->syms[sym_idx].sig;
        out_lv->off_sig = addr_sig;
        out_lv->width = 0x1;

        return LDG_ERR_AOK;
    }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_PART)
    {
        ret = crosstalk_hdl_ast_node_get(ctx->ast, node->first_child, &base_node);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (base_node->kind == CROSSTALK_HDL_NODE_EXPR_DOT)
        {
            uint32_t member_sig = 0;
            uint32_t member_w = 0;
            uint32_t member_addr = UINT32_MAX;

            ret = crosstalk_hdl_elab_member_resolve(ctx, node->first_child, &member_sig, &member_w, &member_addr);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
            {
                if (node->tok.line != 0) { ctx->err_tok = node->tok; }

                return CROSSTALK_HDL_ERR_ELAB;
            }

            ret = crosstalk_hdl_elab_const_eval(ctx, base_node->next_sibling, &msb_val);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_ast_node_get(ctx->ast, base_node->next_sibling, &msb_node);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_elab_const_eval(ctx, msb_node->next_sibling, &lsb_val);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (LDG_UNLIKELY(msb_val < lsb_val))
            {
                if (node->tok.line != 0) { ctx->err_tok = node->tok; }

                return CROSSTALK_HDL_ERR_ELAB;
            }

            if (member_addr != UINT32_MAX)
            {
                out_lv->kind = CROSSTALK_HDL_ELAB_LV_MEM_PART;
                out_lv->addr_sig = member_addr;
            }
            else { out_lv->kind = CROSSTALK_HDL_ELAB_LV_PART; }

            out_lv->sig = member_sig;
            out_lv->off = lsb_val;
            out_lv->width = (uint32_t)(msb_val - lsb_val) + 0x1;

            return LDG_ERR_AOK;
        }

        if (base_node->kind == CROSSTALK_HDL_NODE_EXPR_IDX)
        {
            const crosstalk_hdl_ast_node_t *inner_node = 0x0;

            ret = crosstalk_hdl_ast_node_get(ctx->ast, base_node->first_child, &inner_node);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (LDG_UNLIKELY(inner_node->kind != CROSSTALK_HDL_NODE_EXPR_IDENT))
            {
                if (base_node->tok.line != 0) { ctx->err_tok = base_node->tok; }

                return CROSSTALK_HDL_ERR_ELAB;
            }

            ret = crosstalk_hdl_elab_sym_find(ctx, &inner_node->tok, &sym_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->err_tok = inner_node->tok; return CROSSTALK_HDL_ERR_ELAB; }

            if (LDG_UNLIKELY(ctx->syms[sym_idx].kind != CROSSTALK_HDL_ELAB_SYM_MEM)) { ctx->err_tok = inner_node->tok; return CROSSTALK_HDL_ERR_ELAB; }

            ret = crosstalk_hdl_elab_expr(ctx, inner_node->next_sibling, &addr_sig, &addr_w, &addr_sgn);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_ast_node_get(ctx->ast, node->first_child, &base_node);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_elab_const_eval(ctx, base_node->next_sibling, &msb_val);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_ast_node_get(ctx->ast, base_node->next_sibling, &msb_node);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_elab_const_eval(ctx, msb_node->next_sibling, &lsb_val);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (LDG_UNLIKELY(msb_val < lsb_val)) { ctx->err_tok = inner_node->tok; return CROSSTALK_HDL_ERR_ELAB; }

            out_lv->kind = CROSSTALK_HDL_ELAB_LV_MEM_PART;
            out_lv->sig = ctx->syms[sym_idx].sig;
            out_lv->addr_sig = addr_sig;
            out_lv->off = lsb_val;
            out_lv->width = (uint32_t)(msb_val - lsb_val) + 0x1;

            return LDG_ERR_AOK;
        }

        if (LDG_UNLIKELY(base_node->kind != CROSSTALK_HDL_NODE_EXPR_IDENT))
        {
            if (node->tok.line != 0) { ctx->err_tok = node->tok; }

            return CROSSTALK_HDL_ERR_ELAB;
        }

        ret = crosstalk_hdl_elab_sym_find(ctx, &base_node->tok, &sym_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            if (base_node->tok.line != 0) { ctx->err_tok = base_node->tok; }

            return CROSSTALK_HDL_ERR_ELAB;
        }

        ret = crosstalk_hdl_elab_const_eval(ctx, base_node->next_sibling, &msb_val);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_node_get(ctx->ast, base_node->next_sibling, &msb_node);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_elab_const_eval(ctx, msb_node->next_sibling, &lsb_val);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(msb_val < lsb_val))
        {
            if (node->tok.line != 0) { ctx->err_tok = node->tok; }

            return CROSSTALK_HDL_ERR_ELAB;
        }

        out_lv->kind = CROSSTALK_HDL_ELAB_LV_PART;
        out_lv->sig = ctx->syms[sym_idx].sig;
        out_lv->off = lsb_val;
        out_lv->width = (uint32_t)(msb_val - lsb_val) + 0x1;

        return LDG_ERR_AOK;
    }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_PART_PLUS || node->kind == CROSSTALK_HDL_NODE_EXPR_PART_MINUS)
    {
        uint64_t off_val = 0;
        uint64_t width_val = 0;

        ret = crosstalk_hdl_ast_node_get(ctx->ast, node->first_child, &base_node);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(base_node->kind != CROSSTALK_HDL_NODE_EXPR_IDENT))
        {
            if (node->tok.line != 0) { ctx->err_tok = node->tok; }

            return CROSSTALK_HDL_ERR_ELAB;
        }

        ret = crosstalk_hdl_elab_sym_find(ctx, &base_node->tok, &sym_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            if (base_node->tok.line != 0) { ctx->err_tok = base_node->tok; }

            return CROSSTALK_HDL_ERR_ELAB;
        }

        ret = crosstalk_hdl_ast_node_get(ctx->ast, base_node->next_sibling, &msb_node);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_elab_const_eval(ctx, msb_node->next_sibling, &width_val);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        out_lv->sig = ctx->syms[sym_idx].sig;
        out_lv->width = (uint32_t)width_val;

        if (crosstalk_hdl_elab_const_eval(ctx, base_node->next_sibling, &off_val) == LDG_ERR_AOK)
        {
            out_lv->kind = CROSSTALK_HDL_ELAB_LV_PART;
            out_lv->off = node->kind == CROSSTALK_HDL_NODE_EXPR_PART_MINUS ? off_val - width_val + 0x1 : off_val;

            return LDG_ERR_AOK;
        }

        ret = crosstalk_hdl_elab_expr(ctx, base_node->next_sibling, &addr_sig, &addr_w, &addr_sgn);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (node->kind == CROSSTALK_HDL_NODE_EXPR_PART_MINUS && width_val > 0x1)
        {
            crosstalk_hdl_ir_op_t adj_op = LDG_STRUCT_ZERO_INIT;
            uint32_t adj_const = 0;
            uint32_t adj_sig = 0;

            ret = crosstalk_hdl_elab_const_sig(ctx, width_val - 0x1, addr_w, &adj_const);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_elab_tmp_make(ctx, addr_w, &adj_sig);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            adj_op.kind = CROSSTALK_HDL_IR_OP_SUB;
            adj_op.dst = adj_sig;
            adj_op.src0 = addr_sig;
            adj_op.src1 = adj_const;
            adj_op.src2 = UINT32_MAX;
            adj_op.imm = 0;
            adj_op.width = addr_w;

            ret = crosstalk_hdl_elab_op_emit(ctx, &adj_op);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            addr_sig = adj_sig;
        }

        out_lv->kind = CROSSTALK_HDL_ELAB_LV_PART_DYN;
        out_lv->off_sig = addr_sig;

        return LDG_ERR_AOK;
    }

    if (node->tok.line != 0) { if (node->tok.line != 0) { ctx->err_tok = node->tok; } }

    return CROSSTALK_HDL_ERR_ELAB;
}

static uint32_t crosstalk_hdl_elab_sig_current_get(crosstalk_hdl_elab_ctx_t *ctx, uint32_t sig, uint32_t *out_sig)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_sig == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_sig = sig;

    if (sig < ctx->current_cap && ctx->current_map[sig] != UINT32_MAX) { *out_sig = ctx->current_map[sig]; }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_elab_sig_next_get(crosstalk_hdl_elab_ctx_t *ctx, uint32_t sig, uint32_t *out_sig)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_sig == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (sig < ctx->current_cap && ctx->next_map[sig] != UINT32_MAX)
    {
        *out_sig = ctx->next_map[sig];
        return LDG_ERR_AOK;
    }

    return crosstalk_hdl_elab_sig_current_get(ctx, sig, out_sig);
}

static uint32_t crosstalk_hdl_elab_assign_apply(crosstalk_hdl_elab_ctx_t *ctx, const crosstalk_hdl_elab_lv_t *lv, uint32_t val_sig, uint32_t val_w)
{
    crosstalk_hdl_ir_op_t op = LDG_STRUCT_ZERO_INIT;
    uint32_t fit_sig = 0;
    uint32_t old_sig = 0;
    uint32_t full_sig = 0;
    uint32_t new_sig = 0;
    uint32_t full_w = 0;
    uint32_t ret = 0;
    uint8_t nb_is = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0 || lv == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->nb_is == LDG_TRUTH_TRUE && ctx->mode == CROSSTALK_HDL_ELAB_MODE_SEQ) { nb_is = LDG_TRUTH_TRUE; }

    ret = crosstalk_hdl_elab_width_fit(ctx, val_sig, val_w, LDG_TRUTH_FALSE, lv->width, &fit_sig);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (lv->kind == CROSSTALK_HDL_ELAB_LV_MEM)
    {
        op.kind = CROSSTALK_HDL_IR_OP_MEM_WR;
        op.dst = ctx->guard;
        op.src0 = lv->sig;
        op.src1 = lv->addr_sig;
        op.src2 = fit_sig;
        op.width = lv->width;

        return crosstalk_hdl_elab_op_emit(ctx, &op);
    }

    if (lv->kind == CROSSTALK_HDL_ELAB_LV_MEM_PART)
    {
        uint32_t elem_w = ctx->ir_ctx->sigs[lv->sig].width;

        ret = crosstalk_hdl_elab_tmp_make(ctx, elem_w, &old_sig);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        op.kind = CROSSTALK_HDL_IR_OP_MEM_RD;
        op.dst = old_sig;
        op.src0 = lv->sig;
        op.src1 = lv->addr_sig;
        op.src2 = UINT32_MAX;
        op.imm = 0;
        op.width = elem_w;

        ret = crosstalk_hdl_elab_op_emit(ctx, &op);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_elab_tmp_make(ctx, elem_w, &full_sig);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        op.kind = CROSSTALK_HDL_IR_OP_INSERT;
        op.dst = full_sig;
        op.src0 = old_sig;
        op.src1 = fit_sig;
        op.src2 = UINT32_MAX;
        op.imm = lv->off;
        op.width = lv->width;

        ret = crosstalk_hdl_elab_op_emit(ctx, &op);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        op.kind = CROSSTALK_HDL_IR_OP_MEM_WR;
        op.dst = ctx->guard;
        op.src0 = lv->sig;
        op.src1 = lv->addr_sig;
        op.src2 = full_sig;
        op.imm = 0;
        op.width = elem_w;

        return crosstalk_hdl_elab_op_emit(ctx, &op);
    }

    if (LDG_UNLIKELY(lv->kind == CROSSTALK_HDL_ELAB_LV_NONE)) { return CROSSTALK_HDL_ERR_ELAB; }

    ret = nb_is == LDG_TRUTH_TRUE ? crosstalk_hdl_elab_sig_next_get(ctx, lv->sig, &old_sig) : crosstalk_hdl_elab_sig_current_get(ctx, lv->sig, &old_sig);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    full_w = ctx->ir_ctx->sigs[lv->sig].width;
    full_sig = fit_sig;

    if (lv->kind == CROSSTALK_HDL_ELAB_LV_PART || lv->kind == CROSSTALK_HDL_ELAB_LV_PART_DYN)
    {
        ret = crosstalk_hdl_elab_tmp_make(ctx, full_w, &full_sig);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        op.kind = lv->kind == CROSSTALK_HDL_ELAB_LV_PART ? CROSSTALK_HDL_IR_OP_INSERT : CROSSTALK_HDL_IR_OP_INSERT_DYN;
        op.dst = full_sig;
        op.src0 = old_sig;
        op.src1 = fit_sig;
        op.src2 = lv->off_sig;
        op.imm = lv->off;
        op.width = lv->width;

        ret = crosstalk_hdl_elab_op_emit(ctx, &op);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    new_sig = full_sig;

    if (ctx->guard != UINT32_MAX)
    {
        ret = crosstalk_hdl_elab_tmp_make(ctx, full_w, &new_sig);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        op.kind = CROSSTALK_HDL_IR_OP_MUX;
        op.dst = new_sig;
        op.src0 = ctx->guard;
        op.src1 = full_sig;
        op.src2 = old_sig;
        op.imm = 0;
        op.width = full_w;

        ret = crosstalk_hdl_elab_op_emit(ctx, &op);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (LDG_UNLIKELY(lv->sig >= ctx->current_cap)) { return LDG_ERR_BOUNDS; }

    if (nb_is == LDG_TRUTH_TRUE) { ctx->next_map[lv->sig] = new_sig; }
    else { ctx->current_map[lv->sig] = new_sig; }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_elab_concat_assign(crosstalk_hdl_elab_ctx_t *ctx, uint32_t concat_idx, uint32_t val_sig)
{
    const crosstalk_hdl_ast_node_t *concat_node = 0x0;
    const crosstalk_hdl_ast_node_t *member_node = 0x0;
    crosstalk_hdl_elab_lv_t lv = LDG_STRUCT_ZERO_INIT;
    crosstalk_hdl_ir_op_t op = LDG_STRUCT_ZERO_INIT;
    uint32_t member_arr[32] = LDG_ARR_ZERO_INIT;
    uint32_t member_cunt = 0;
    uint32_t member_idx = 0;
    uint32_t bit_off = 0;
    uint32_t part_sig = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_ast_node_get(ctx->ast, concat_idx, &concat_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    member_idx = concat_node->first_child;
    while (member_idx != UINT32_MAX)
    {
        if (LDG_UNLIKELY(member_cunt >= (uint32_t)(sizeof(member_arr) / sizeof(member_arr[0])))) { return LDG_ERR_FULL; }

        member_arr[member_cunt] = member_idx;
        member_cunt = member_cunt + 0x1;

        ret = crosstalk_hdl_ast_node_get(ctx->ast, member_idx, &member_node);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        member_idx = member_node->next_sibling;
    }

    for (idx = member_cunt; idx > 0; idx = idx - 1)
    {
        ret = crosstalk_hdl_elab_lv_resolve(ctx, member_arr[idx - 0x1], &lv);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_elab_tmp_make(ctx, lv.width, &part_sig);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        op.kind = CROSSTALK_HDL_IR_OP_SLICE;
        op.dst = part_sig;
        op.src0 = val_sig;
        op.src1 = UINT32_MAX;
        op.src2 = UINT32_MAX;
        op.imm = bit_off;
        op.width = lv.width;

        ret = crosstalk_hdl_elab_op_emit(ctx, &op);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_elab_assign_apply(ctx, &lv, part_sig, lv.width);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        bit_off = bit_off + lv.width;
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_elab_svar_probe(crosstalk_hdl_elab_ctx_t *ctx, uint32_t node_idx, uint32_t *out_sym, uint32_t *out_addr_expr)
{
    const crosstalk_hdl_ast_node_t *node = 0x0;
    const crosstalk_hdl_ast_node_t *ident_node = 0x0;
    uint32_t sym_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_sym == 0x0 || out_addr_expr == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_sym = UINT32_MAX;
    *out_addr_expr = UINT32_MAX;

    ret = crosstalk_hdl_ast_node_get(ctx->ast, node_idx, &node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_IDX)
    {
        ret = crosstalk_hdl_ast_node_get(ctx->ast, node->first_child, &ident_node);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (ident_node->kind != CROSSTALK_HDL_NODE_EXPR_IDENT) { return LDG_ERR_NOT_FOUND; }

        *out_addr_expr = ident_node->next_sibling;
    }
    else if (node->kind == CROSSTALK_HDL_NODE_EXPR_IDENT) { ident_node = node; }
    else { return LDG_ERR_NOT_FOUND; }

    ret = crosstalk_hdl_elab_sym_find(ctx, &ident_node->tok, &sym_idx);
    if (ret != LDG_ERR_AOK) { return LDG_ERR_NOT_FOUND; }

    if (ctx->syms[sym_idx].kind != CROSSTALK_HDL_ELAB_SYM_SVAR) { return LDG_ERR_NOT_FOUND; }

    *out_sym = sym_idx;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_elab_struct_vals_get(crosstalk_hdl_elab_ctx_t *ctx, uint32_t node_idx, uint32_t styp_idx, uint32_t *out_vals)
{
    const crosstalk_hdl_ast_node_t *node = 0x0;
    const crosstalk_hdl_elab_styp_t *styp = 0x0;
    crosstalk_hdl_ir_op_t op = LDG_STRUCT_ZERO_INIT;
    uint32_t svar_sym = 0;
    uint32_t addr_expr = 0;
    uint32_t fld_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_vals == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->depth >= CROSSTALK_HDL_ELAB_DEPTH_MAX)) { return CROSSTALK_HDL_ERR_ELAB; }

    styp = &ctx->styps[styp_idx];

    if (LDG_UNLIKELY(styp->field_cunt > CROSSTALK_HDL_ELAB_SFLD_MAX)) { return LDG_ERR_FULL; }

    if (crosstalk_hdl_elab_svar_probe(ctx, node_idx, &svar_sym, &addr_expr) == LDG_ERR_AOK)
    {
        uint32_t addr_sig = UINT32_MAX;
        uint32_t addr_w = 0;
        uint8_t addr_sgn = 0;

        if (LDG_UNLIKELY((uint32_t)ctx->syms[svar_sym].val != styp_idx)) { return CROSSTALK_HDL_ERR_ELAB; }

        if (addr_expr != UINT32_MAX)
        {
            ret = crosstalk_hdl_elab_expr(ctx, addr_expr, &addr_sig, &addr_w, &addr_sgn);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        for (fld_idx = 0; fld_idx < styp->field_cunt; fld_idx++)
        {
            const crosstalk_hdl_elab_sfield_t *fld = &ctx->sfields[styp->field_base + fld_idx];

            if (addr_sig != UINT32_MAX)
            {
                ret = crosstalk_hdl_elab_tmp_make(ctx, fld->width, &out_vals[fld_idx]);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                op.kind = CROSSTALK_HDL_IR_OP_MEM_RD;
                op.dst = out_vals[fld_idx];
                op.src0 = ctx->syms[svar_sym].sig + fld_idx;
                op.src1 = addr_sig;
                op.src2 = UINT32_MAX;
                op.imm = 0;
                op.width = fld->width;

                ret = crosstalk_hdl_elab_op_emit(ctx, &op);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
            else
            {
                ret = crosstalk_hdl_elab_sig_current_get(ctx, ctx->syms[svar_sym].sig + fld_idx, &out_vals[fld_idx]);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
        }

        return LDG_ERR_AOK;
    }

    ret = crosstalk_hdl_ast_node_get(ctx->ast, node_idx, &node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_DOT)
    {
        const crosstalk_hdl_ast_node_t *grp_base = 0x0;
        const crosstalk_hdl_elab_styp_t *par_styp = 0x0;
        uint32_t grp_sym = 0;
        uint32_t grp_idx = 0;

        ret = crosstalk_hdl_ast_node_get(ctx->ast, node->first_child, &grp_base);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(grp_base->kind != CROSSTALK_HDL_NODE_EXPR_IDENT))
        {
            if (node->tok.line != 0) { ctx->err_tok = node->tok; }

            return CROSSTALK_HDL_ERR_ELAB;
        }

        ret = crosstalk_hdl_elab_sym_find(ctx, &grp_base->tok, &grp_sym);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK || ctx->syms[grp_sym].kind != CROSSTALK_HDL_ELAB_SYM_SVAR || ctx->syms[grp_sym].val >> 32 != 0))
        {
            if (node->tok.line != 0) { ctx->err_tok = node->tok; }

            return CROSSTALK_HDL_ERR_ELAB;
        }

        par_styp = &ctx->styps[(uint32_t)ctx->syms[grp_sym].val];

        for (grp_idx = 0; grp_idx < par_styp->group_cunt; grp_idx++)
        {
            const crosstalk_hdl_elab_sfield_t *grp = &ctx->sfields[par_styp->group_base + grp_idx];

            if (crosstalk_hdl_elab_tok_eq_is(ctx->src, &grp->name, &node->tok) != LDG_TRUTH_TRUE) { continue; }

            if (LDG_UNLIKELY(grp->sub_styp != styp_idx))
            {
                if (node->tok.line != 0) { ctx->err_tok = node->tok; }

                return CROSSTALK_HDL_ERR_ELAB;
            }

            for (fld_idx = 0; fld_idx < styp->field_cunt; fld_idx++)
            {
                ret = crosstalk_hdl_elab_sig_current_get(ctx, ctx->syms[grp_sym].sig + grp->sub_leaf_base + fld_idx, &out_vals[fld_idx]);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            return LDG_ERR_AOK;
        }

        if (node->tok.line != 0) { ctx->err_tok = node->tok; }

        return CROSSTALK_HDL_ERR_ELAB;
    }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_LIT)
    {
        uint64_t lit_val = 0;
        uint32_t lit_w = 0;
        uint8_t ones_is = LDG_TRUTH_FALSE;

        if (node->tok.kind == CROSSTALK_HDL_TOK_LIT_UNBASED) { ones_is = ctx->src[node->tok.off + 0x1] == 0x31 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }
        else
        {
            ret = crosstalk_hdl_elab_lit_decode(ctx->src, &node->tok, &lit_val, &lit_w);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK || lit_val != 0))
            {
                if (node->tok.line != 0) { ctx->err_tok = node->tok; }

                return CROSSTALK_HDL_ERR_ELAB;
            }
        }

        for (fld_idx = 0; fld_idx < styp->field_cunt; fld_idx++)
        {
            const crosstalk_hdl_elab_sfield_t *fld = &ctx->sfields[styp->field_base + fld_idx];
            uint64_t fill = 0;

            if (ones_is == LDG_TRUTH_TRUE) { fill = fld->width >= 64 ? ~(uint64_t)0x0 : ((uint64_t)0x1 << fld->width) - 0x1; }

            ret = crosstalk_hdl_elab_const_sig(ctx, fill, fld->width, &out_vals[fld_idx]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        return LDG_ERR_AOK;
    }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_COND)
    {
        const crosstalk_hdl_ast_node_t *then_node = 0x0;
        uint32_t then_vals[CROSSTALK_HDL_ELAB_SFLD_MAX];
        uint32_t else_vals[CROSSTALK_HDL_ELAB_SFLD_MAX];
        uint32_t cond_sig = 0;
        uint32_t cond_w = 0;
        uint32_t then_idx = 0;
        uint32_t else_idx = 0;
        uint8_t cond_sgn = 0;

        ret = crosstalk_hdl_elab_expr(ctx, node->first_child, &cond_sig, &cond_w, &cond_sgn);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_node_get(ctx->ast, node->first_child, &then_node);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        then_idx = then_node->next_sibling;

        ret = crosstalk_hdl_ast_node_get(ctx->ast, then_idx, &then_node);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        else_idx = then_node->next_sibling;

        ctx->depth++;
        ret = crosstalk_hdl_elab_struct_vals_get(ctx, then_idx, styp_idx, then_vals);
        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_elab_struct_vals_get(ctx, else_idx, styp_idx, else_vals); }

        ctx->depth--;

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        for (fld_idx = 0; fld_idx < styp->field_cunt; fld_idx++)
        {
            const crosstalk_hdl_elab_sfield_t *fld = &ctx->sfields[styp->field_base + fld_idx];

            ret = crosstalk_hdl_elab_tmp_make(ctx, fld->width, &out_vals[fld_idx]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            op.kind = CROSSTALK_HDL_IR_OP_MUX;
            op.dst = out_vals[fld_idx];
            op.src0 = cond_sig;
            op.src1 = then_vals[fld_idx];
            op.src2 = else_vals[fld_idx];
            op.imm = 0;
            op.width = fld->width;

            ret = crosstalk_hdl_elab_op_emit(ctx, &op);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        return LDG_ERR_AOK;
    }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_CALL)
    {
        uint32_t call_sig = 0;
        uint32_t call_w = 0;
        uint8_t call_sgn = 0;

        ret = crosstalk_hdl_elab_expr(ctx, node_idx, &call_sig, &call_w, &call_sgn);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(ctx->call_styp != styp_idx))
        {
            if (node->tok.line != 0) { ctx->err_tok = node->tok; }

            return CROSSTALK_HDL_ERR_ELAB;
        }

        for (fld_idx = 0; fld_idx < styp->field_cunt; fld_idx++)
        {
            ret = crosstalk_hdl_elab_sig_current_get(ctx, ctx->call_base + fld_idx, &out_vals[fld_idx]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        return LDG_ERR_AOK;
    }

    if (node->tok.line != 0) { ctx->err_tok = node->tok; }

    return CROSSTALK_HDL_ERR_ELAB;
}

static uint32_t crosstalk_hdl_elab_struct_assign(crosstalk_hdl_elab_ctx_t *ctx, uint32_t styp_idx, uint32_t lhs_base, uint32_t lhs_addr_expr, const uint32_t *rhs_vals)
{
    crosstalk_hdl_elab_lv_t lv = LDG_STRUCT_ZERO_INIT;
    const crosstalk_hdl_elab_styp_t *styp = 0x0;
    uint32_t lhs_addr_sig = UINT32_MAX;
    uint32_t addr_w = 0;
    uint32_t fld_idx = 0;
    uint32_t ret = 0;
    uint8_t addr_sgn = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || rhs_vals == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    styp = &ctx->styps[styp_idx];

    if (lhs_addr_expr != UINT32_MAX)
    {
        ret = crosstalk_hdl_elab_expr(ctx, lhs_addr_expr, &lhs_addr_sig, &addr_w, &addr_sgn);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (fld_idx = 0; fld_idx < styp->field_cunt; fld_idx++)
    {
        const crosstalk_hdl_elab_sfield_t *fld = &ctx->sfields[styp->field_base + fld_idx];

        if (lhs_addr_sig != UINT32_MAX)
        {
            lv.kind = CROSSTALK_HDL_ELAB_LV_MEM;
            lv.addr_sig = lhs_addr_sig;
        }
        else
        {
            lv.kind = CROSSTALK_HDL_ELAB_LV_SIG;
            lv.addr_sig = UINT32_MAX;
        }

        lv.sig = lhs_base + fld_idx;
        lv.width = fld->width;
        lv.off = 0;
        lv.off_sig = UINT32_MAX;

        ret = crosstalk_hdl_elab_assign_apply(ctx, &lv, rhs_vals[fld_idx], fld->width);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_elab_commit(crosstalk_hdl_elab_ctx_t *ctx)
{
    crosstalk_hdl_ir_op_t op = LDG_STRUCT_ZERO_INIT;
    uint32_t sig = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->mode == CROSSTALK_HDL_ELAB_MODE_FINAL)
    {
        for (sig = 0; sig < ctx->current_cap; sig++)
        {
            ctx->current_map[sig] = UINT32_MAX;
            ctx->next_map[sig] = UINT32_MAX;
        }

        return LDG_ERR_AOK;
    }

    for (sig = 0; sig < ctx->current_cap; sig++)
    {
        if (ctx->current_map[sig] == UINT32_MAX) { continue; }

        op.kind = ctx->mode == CROSSTALK_HDL_ELAB_MODE_SEQ ? CROSSTALK_HDL_IR_OP_REG_NEXT : CROSSTALK_HDL_IR_OP_COPY;
        op.dst = sig;
        op.src0 = ctx->current_map[sig];
        op.src1 = UINT32_MAX;
        op.src2 = UINT32_MAX;
        op.imm = 0;
        op.width = ctx->ir_ctx->sigs[sig].width;

        ret = crosstalk_hdl_elab_op_emit(ctx, &op);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ctx->current_map[sig] = UINT32_MAX;
    }

    for (sig = 0; sig < ctx->current_cap; sig++)
    {
        if (ctx->next_map[sig] == UINT32_MAX) { continue; }

        op.kind = ctx->mode == CROSSTALK_HDL_ELAB_MODE_SEQ ? CROSSTALK_HDL_IR_OP_REG_NEXT : CROSSTALK_HDL_IR_OP_COPY;
        op.dst = sig;
        op.src0 = ctx->next_map[sig];
        op.src1 = UINT32_MAX;
        op.src2 = UINT32_MAX;
        op.imm = 0;
        op.width = ctx->ir_ctx->sigs[sig].width;

        ret = crosstalk_hdl_elab_op_emit(ctx, &op);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ctx->next_map[sig] = UINT32_MAX;
    }

    return LDG_ERR_AOK;
}

static uint8_t crosstalk_hdl_elab_tok_same_is(const char *src, const crosstalk_hdl_tok_t *lhs, const crosstalk_hdl_tok_t *rhs)
{
    if (LDG_UNLIKELY(src == 0x0 || lhs == 0x0 || rhs == 0x0)) { return LDG_TRUTH_FALSE; }

    if (lhs->len != rhs->len) { return LDG_TRUTH_FALSE; }

    return memcmp(src + lhs->off, src + rhs->off, lhs->len) == 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint8_t crosstalk_hdl_elab_tree_rd_tok_is(const crosstalk_hdl_elab_ctx_t *ctx, uint32_t node_idx, const crosstalk_hdl_tok_t *name_tok)
{
    const crosstalk_hdl_ast_node_t *node = 0x0;
    uint32_t child_idx = 0;

    if (node_idx == UINT32_MAX) { return LDG_TRUTH_FALSE; }

    if (crosstalk_hdl_ast_node_get(ctx->ast, node_idx, &node) != LDG_ERR_AOK) { return LDG_TRUTH_TRUE; }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_IDENT && crosstalk_hdl_elab_tok_same_is(ctx->src, &node->tok, name_tok) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    child_idx = node->first_child;
    while (child_idx != UINT32_MAX)
    {
        const crosstalk_hdl_ast_node_t *child = 0x0;

        if (crosstalk_hdl_elab_tree_rd_tok_is(ctx, child_idx, name_tok) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

        if (crosstalk_hdl_ast_node_get(ctx->ast, child_idx, &child) != LDG_ERR_AOK) { return LDG_TRUTH_TRUE; }

        child_idx = child->next_sibling;
    }

    return LDG_TRUTH_FALSE;
}

static uint32_t crosstalk_hdl_elab_case_flat_scan(crosstalk_hdl_elab_ctx_t *ctx, const crosstalk_hdl_ast_node_t *case_node, uint32_t *out_base_stmt)
{
    const crosstalk_hdl_ast_node_t *item_node = 0x0;
    const crosstalk_hdl_ast_node_t *body_node = 0x0;
    const crosstalk_hdl_ast_node_t *lv_node = 0x0;
    const crosstalk_hdl_ast_node_t *walk_node = 0x0;
    crosstalk_hdl_tok_t dst_tok = LDG_STRUCT_ZERO_INIT;
    uint32_t item_idx = 0;
    uint32_t body_idx = 0;
    uint32_t walk_idx = 0;
    uint8_t dst_has = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || case_node == 0x0 || out_base_stmt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_base_stmt = UINT32_MAX;

    ret = crosstalk_hdl_ast_node_get(ctx->ast, case_node->first_child, &item_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    item_idx = item_node->next_sibling;
    while (item_idx != UINT32_MAX)
    {
        ret = crosstalk_hdl_ast_node_get(ctx->ast, item_idx, &item_node);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (item_node->kind == CROSSTALK_HDL_NODE_CASE_ITEM || item_node->kind == CROSSTALK_HDL_NODE_CASE_DEFAULT)
        {
            body_idx = item_node->first_child;
            walk_idx = body_idx;
            while (walk_idx != UINT32_MAX)
            {
                ret = crosstalk_hdl_ast_node_get(ctx->ast, walk_idx, &walk_node);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                body_idx = walk_idx;
                walk_idx = walk_node->next_sibling;
            }

            if (body_idx == UINT32_MAX) { return LDG_ERR_NOT_FOUND; }

            ret = crosstalk_hdl_ast_node_get(ctx->ast, body_idx, &body_node);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (body_node->kind != CROSSTALK_HDL_NODE_B_ASSIGN) { return LDG_ERR_NOT_FOUND; }

            ret = crosstalk_hdl_ast_node_get(ctx->ast, body_node->first_child, &lv_node);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (lv_node->kind != CROSSTALK_HDL_NODE_EXPR_IDENT) { return LDG_ERR_NOT_FOUND; }

            if (dst_has == LDG_TRUTH_FALSE)
            {
                dst_tok = lv_node->tok;
                dst_has = LDG_TRUTH_TRUE;
            }
            else if (crosstalk_hdl_elab_tok_same_is(ctx->src, &lv_node->tok, &dst_tok) != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_FOUND; }

            if (item_node->kind == CROSSTALK_HDL_NODE_CASE_ITEM && crosstalk_hdl_elab_tree_rd_tok_is(ctx, lv_node->next_sibling, &dst_tok) == LDG_TRUTH_TRUE) { return LDG_ERR_NOT_FOUND; }

            if (item_node->kind == CROSSTALK_HDL_NODE_CASE_DEFAULT) { *out_base_stmt = body_idx; }
        }

        item_idx = item_node->next_sibling;
    }

    if (*out_base_stmt == UINT32_MAX) { return LDG_ERR_NOT_FOUND; }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_elab_case_stmt(crosstalk_hdl_elab_ctx_t *ctx, uint32_t node_idx)
{
    const crosstalk_hdl_ast_node_t *node = 0x0;
    const crosstalk_hdl_ast_node_t *item_node = 0x0;
    const crosstalk_hdl_ast_node_t *label_node = 0x0;
    crosstalk_hdl_ir_op_t op = LDG_STRUCT_ZERO_INIT;
    uint32_t subj_sig = 0;
    uint32_t subj_w = 0;
    uint32_t matched_sig = UINT32_MAX;
    uint32_t item_idx = 0;
    uint32_t guard_save = 0;
    uint32_t item_guard = 0;
    uint32_t eq_sig = 0;
    uint32_t tmp_sig = 0;
    uint32_t stmt_idx = 0;
    uint32_t ret = 0;
    uint8_t subj_sgn = 0;
    uint8_t flat_is = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_ast_node_get(ctx->ast, node_idx, &node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_elab_expr(ctx, node->first_child, &subj_sig, &subj_w, &subj_sgn);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_elab_const_sig(ctx, 0x0, 0x1, &matched_sig);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_ast_node_get(ctx->ast, node->first_child, &item_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    item_idx = item_node->next_sibling;
    guard_save = ctx->guard;

    {
        uint32_t base_stmt = UINT32_MAX;

        if (crosstalk_hdl_elab_case_flat_scan(ctx, node, &base_stmt) == LDG_ERR_AOK)
        {
            ctx->guard = guard_save;

            ret = crosstalk_hdl_elab_stmt(ctx, base_stmt);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ctx->guard = guard_save;
            flat_is = LDG_TRUTH_TRUE;
        }
    }

    while (item_idx != UINT32_MAX)
    {
        ret = crosstalk_hdl_ast_node_get(ctx->ast, item_idx, &item_node);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (item_node->kind == CROSSTALK_HDL_NODE_CASE_DEFAULT && flat_is == LDG_TRUTH_TRUE)
        {
            item_idx = item_node->next_sibling;
            continue;
        }

        if (item_node->kind == CROSSTALK_HDL_NODE_CASE_ITEM || item_node->kind == CROSSTALK_HDL_NODE_CASE_DEFAULT)
        {
            uint32_t or_sig = UINT32_MAX;
            uint32_t label_idx = item_node->first_child;

            stmt_idx = UINT32_MAX;

            if (item_node->kind == CROSSTALK_HDL_NODE_CASE_ITEM)
            {
                while (label_idx != UINT32_MAX)
                {
                    ret = crosstalk_hdl_ast_node_get(ctx->ast, label_idx, &label_node);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    if (label_node->next_sibling == UINT32_MAX) { stmt_idx = label_idx; break; }

                    {
                        uint32_t lab_sig = 0;
                        uint32_t lab_w = 0;
                        uint32_t cmp_sig = subj_sig;
                        uint8_t lab_sgn = 0;
                        uint8_t wild_is = LDG_TRUTH_FALSE;

                        if ((node->tok.val == CROSSTALK_HDL_KW_CASEZ || node->tok.val == CROSSTALK_HDL_KW_CASEX) && label_node->kind == CROSSTALK_HDL_NODE_EXPR_LIT)
                        {
                            uint64_t lab_val = 0;
                            uint64_t lab_mask = 0;

                            ret = crosstalk_hdl_elab_lit_mask_decode(ctx->src, &label_node->tok, &lab_val, &lab_mask, &lab_w);
                            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                            if (lab_mask != ~(uint64_t)0x0)
                            {
                                uint32_t mask_sig = 0;

                                wild_is = LDG_TRUTH_TRUE;

                                ret = crosstalk_hdl_elab_const_sig(ctx, lab_mask, subj_w, &mask_sig);
                                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                                ret = crosstalk_hdl_elab_tmp_make(ctx, subj_w, &cmp_sig);
                                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                                op.kind = CROSSTALK_HDL_IR_OP_AND;
                                op.dst = cmp_sig;
                                op.src0 = subj_sig;
                                op.src1 = mask_sig;
                                op.src2 = UINT32_MAX;
                                op.imm = 0;
                                op.width = subj_w;

                                ret = crosstalk_hdl_elab_op_emit(ctx, &op);
                                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                                ret = crosstalk_hdl_elab_const_sig(ctx, lab_val & lab_mask, subj_w, &lab_sig);
                                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
                            }
                        }

                        if (wild_is == LDG_TRUTH_FALSE)
                        {
                            ret = crosstalk_hdl_elab_expr(ctx, label_idx, &lab_sig, &lab_w, &lab_sgn);
                            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                            ret = crosstalk_hdl_elab_width_fit(ctx, lab_sig, lab_w, LDG_TRUTH_FALSE, subj_w, &lab_sig);
                            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
                        }

                        ret = crosstalk_hdl_elab_tmp_make(ctx, 0x1, &eq_sig);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                        op.kind = CROSSTALK_HDL_IR_OP_EQ;
                        op.dst = eq_sig;
                        op.src0 = cmp_sig;
                        op.src1 = lab_sig;
                        op.src2 = UINT32_MAX;
                        op.imm = 0;
                        op.width = subj_w;

                        ret = crosstalk_hdl_elab_op_emit(ctx, &op);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                        if (or_sig == UINT32_MAX) { or_sig = eq_sig; }
                        else
                        {
                            ret = crosstalk_hdl_elab_tmp_make(ctx, 0x1, &tmp_sig);
                            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                            op.kind = CROSSTALK_HDL_IR_OP_LOR;
                            op.dst = tmp_sig;
                            op.src0 = or_sig;
                            op.src1 = eq_sig;
                            op.width = 0x1;

                            ret = crosstalk_hdl_elab_op_emit(ctx, &op);
                            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                            or_sig = tmp_sig;
                        }
                    }

                    label_idx = label_node->next_sibling;
                }
            }
            else { stmt_idx = item_node->first_child; }

            if (LDG_UNLIKELY(stmt_idx == UINT32_MAX)) { return CROSSTALK_HDL_ERR_ELAB; }

            {
                uint32_t not_matched = 0;
                uint32_t sel_sig = 0;

                ret = crosstalk_hdl_elab_tmp_make(ctx, 0x1, &not_matched);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                op.kind = CROSSTALK_HDL_IR_OP_LNOT;
                op.dst = not_matched;
                op.src0 = matched_sig;
                op.src1 = UINT32_MAX;
                op.src2 = UINT32_MAX;
                op.width = 0x1;

                ret = crosstalk_hdl_elab_op_emit(ctx, &op);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                if (item_node->kind == CROSSTALK_HDL_NODE_CASE_ITEM)
                {
                    ret = crosstalk_hdl_elab_tmp_make(ctx, 0x1, &sel_sig);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    op.kind = CROSSTALK_HDL_IR_OP_LAND;
                    op.dst = sel_sig;
                    op.src0 = or_sig;
                    op.src1 = not_matched;
                    op.width = 0x1;

                    ret = crosstalk_hdl_elab_op_emit(ctx, &op);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
                }
                else { sel_sig = not_matched; }

                ctx->guard = guard_save;

                ret = crosstalk_hdl_elab_guard_mix(ctx, sel_sig, LDG_TRUTH_FALSE, &item_guard);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                ctx->guard = item_guard;

                ret = crosstalk_hdl_elab_stmt(ctx, stmt_idx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                ctx->guard = guard_save;

                if (item_node->kind == CROSSTALK_HDL_NODE_CASE_ITEM)
                {
                    ret = crosstalk_hdl_elab_tmp_make(ctx, 0x1, &tmp_sig);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    op.kind = CROSSTALK_HDL_IR_OP_LOR;
                    op.dst = tmp_sig;
                    op.src0 = matched_sig;
                    op.src1 = or_sig;
                    op.src2 = UINT32_MAX;
                    op.width = 0x1;

                    ret = crosstalk_hdl_elab_op_emit(ctx, &op);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    matched_sig = tmp_sig;
                }
            }
        }

        item_idx = item_node->next_sibling;
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_elab_for_stmt_unroll(crosstalk_hdl_elab_ctx_t *ctx, uint32_t node_idx, uint8_t item_is)
{
    const crosstalk_hdl_ast_node_t *node = 0x0;
    const crosstalk_hdl_ast_node_t *init_node = 0x0;
    const crosstalk_hdl_ast_node_t *cond_node = 0x0;
    const crosstalk_hdl_ast_node_t *step_node = 0x0;
    const crosstalk_hdl_ast_node_t *name_node = 0x0;
    const crosstalk_hdl_ast_node_t *step_lhs = 0x0;
    crosstalk_hdl_tok_t var_tok = LDG_STRUCT_ZERO_INIT;
    uint64_t var_val = 0;
    uint64_t cond_val = 0;
    uint64_t step_val = 0;
    uint64_t step_prev = 0;
    uint8_t step_dec_is = LDG_TRUTH_FALSE;
    uint8_t dec_seen = LDG_TRUTH_FALSE;
    uint32_t init_idx = 0;
    uint32_t cond_idx = 0;
    uint32_t step_idx = 0;
    uint32_t body_idx = 0;
    uint32_t sym_idx = 0;
    uint32_t iter = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_ast_node_get(ctx->ast, node_idx, &node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    init_idx = node->first_child;
    ret = crosstalk_hdl_ast_node_get(ctx->ast, init_idx, &init_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    cond_idx = init_node->next_sibling;
    ret = crosstalk_hdl_ast_node_get(ctx->ast, cond_idx, &cond_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    step_idx = cond_node->next_sibling;
    ret = crosstalk_hdl_ast_node_get(ctx->ast, step_idx, &step_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    body_idx = step_node->next_sibling;

    if (init_node->kind == CROSSTALK_HDL_NODE_VAR_DECL)
    {
        uint32_t child_idx = init_node->first_child;

        while (child_idx != UINT32_MAX)
        {
            ret = crosstalk_hdl_ast_node_get(ctx->ast, child_idx, &name_node);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (name_node->kind == CROSSTALK_HDL_NODE_DECL_NAME)
            {
                var_tok = name_node->tok;
                if (name_node->first_child != UINT32_MAX)
                {
                    ret = crosstalk_hdl_elab_const_eval(ctx, name_node->first_child, &var_val);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
                }
            }

            child_idx = name_node->next_sibling;
        }
    }
    else if (init_node->kind == CROSSTALK_HDL_NODE_B_ASSIGN)
    {
        ret = crosstalk_hdl_ast_node_get(ctx->ast, init_node->first_child, &name_node);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        var_tok = name_node->tok;

        ret = crosstalk_hdl_elab_const_eval(ctx, name_node->next_sibling, &var_val);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    else { ctx->err_tok = init_node->tok; return CROSSTALK_HDL_ERR_ELAB; }

    ret = crosstalk_hdl_elab_sym_add(ctx, &var_tok, CROSSTALK_HDL_ELAB_SYM_PARAM, UINT32_MAX, var_val, 32);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_elab_sym_find(ctx, &var_tok, &sym_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (iter = 0; iter < CROSSTALK_HDL_ELAB_UNROLL_MAX; iter++)
    {
        ret = crosstalk_hdl_elab_const_eval(ctx, cond_idx, &cond_val);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (cond_val == 0) { break; }

        if (item_is == LDG_TRUTH_TRUE)
        {
            uint8_t pushed = LDG_TRUTH_FALSE;

            if (ctx->gen_depth < (uint32_t)CROSSTALK_HDL_ELAB_GEN_DEPTH_MAX)
            {
                ctx->gen_at[ctx->gen_depth] = (uint32_t)ctx->syms[sym_idx].val;
                ctx->gen_depth = ctx->gen_depth + 0x1;
                pushed = LDG_TRUTH_TRUE;
            }
            else { ctx->warn_cunt++; }

            ret = crosstalk_hdl_elab_item(ctx, body_idx);

            if (pushed == LDG_TRUTH_TRUE) { ctx->gen_depth = ctx->gen_depth - 0x1; }
        }
        else { ret = crosstalk_hdl_elab_stmt(ctx, body_idx); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_node_get(ctx->ast, step_node->first_child, &step_lhs);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        step_prev = ctx->syms[sym_idx].val;
        step_dec_is = LDG_TRUTH_FALSE;

        if (step_node->tok.kind == CROSSTALK_HDL_TOK_OP && step_node->tok.val == CROSSTALK_HDL_OP_INC) { ctx->syms[sym_idx].val = ctx->syms[sym_idx].val + 0x1; }
        else if (step_node->tok.kind == CROSSTALK_HDL_TOK_OP && step_node->tok.val == CROSSTALK_HDL_OP_DEC)
        {
            ctx->syms[sym_idx].val = ctx->syms[sym_idx].val - 0x1;
            step_dec_is = LDG_TRUTH_TRUE;
        }
        else if (step_lhs->next_sibling != UINT32_MAX)
        {
            ret = crosstalk_hdl_elab_const_eval(ctx, step_lhs->next_sibling, &step_val);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (step_node->tok.val == CROSSTALK_HDL_OP_PLUS_ASSIGN) { ctx->syms[sym_idx].val = ctx->syms[sym_idx].val + step_val; }
            else if (step_node->tok.val == CROSSTALK_HDL_OP_MINUS_ASSIGN)
            {
                ctx->syms[sym_idx].val = ctx->syms[sym_idx].val - step_val;
                step_dec_is = step_val != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
            }
            else { ctx->syms[sym_idx].val = step_val; }
        }
        else { ctx->err_tok = step_node->tok; return CROSSTALK_HDL_ERR_ELAB; }

        if (ctx->syms[sym_idx].val < step_prev) { dec_seen = LDG_TRUTH_TRUE; }

        if ((step_dec_is == LDG_TRUTH_TRUE || dec_seen == LDG_TRUTH_TRUE) && ctx->syms[sym_idx].val > step_prev) { break; }
    }

    if (LDG_UNLIKELY(iter >= CROSSTALK_HDL_ELAB_UNROLL_MAX))
    {
        if (node->tok.line != 0) { ctx->err_tok = node->tok; }

        return CROSSTALK_HDL_ERR_ELAB;
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_elab_systask(crosstalk_hdl_elab_ctx_t *ctx, uint32_t call_idx, uint32_t where)
{
    const crosstalk_hdl_ast_node_t *call_node = 0x0;
    const crosstalk_hdl_ast_node_t *fn_node = 0x0;
    const crosstalk_hdl_ast_node_t *arg_node = 0x0;
    crosstalk_hdl_ir_systask_t task = LDG_STRUCT_ZERO_INIT;
    crosstalk_hdl_ir_systask_arg_t arg = LDG_STRUCT_ZERO_INIT;
    uint32_t arg_idx = 0;
    uint32_t sym_idx = 0;
    uint32_t sig = 0;
    uint32_t width = 0;
    uint32_t ret = 0;
    uint8_t sgn = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_ast_node_get(ctx->ast, call_idx, &call_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_ast_node_get(ctx->ast, call_node->first_child, &fn_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (crosstalk_hdl_elab_tok_lit_eq_is(ctx->src, &fn_node->tok, CROSSTALK_HDL_ELAB_SYSTASK_DISPLAY_NAME) == LDG_TRUTH_TRUE) { task.kind = CROSSTALK_HDL_IR_SYSTASK_DISPLAY; }
    else if (crosstalk_hdl_elab_tok_lit_eq_is(ctx->src, &fn_node->tok, CROSSTALK_HDL_ELAB_SYSTASK_WR_NAME) == LDG_TRUTH_TRUE) { task.kind = CROSSTALK_HDL_IR_SYSTASK_WR; }
    else if (crosstalk_hdl_elab_tok_lit_eq_is(ctx->src, &fn_node->tok, CROSSTALK_HDL_ELAB_SYSTASK_FINISH_NAME) == LDG_TRUTH_TRUE) { task.kind = CROSSTALK_HDL_IR_SYSTASK_FINISH; }
    else { return LDG_ERR_AOK; }

    task.call = fn_node->tok;
    task.where = where;
    task.guard = where == CROSSTALK_HDL_IR_SYSTASK_WHERE_INITIAL ? UINT32_MAX : ctx->guard;
    task.arg_start = ctx->ir_ctx->systask_arg_cunt;

    if (where == CROSSTALK_HDL_IR_SYSTASK_WHERE_SEQ) { task.blk = ctx->seq_blk; }
    else if (where == CROSSTALK_HDL_IR_SYSTASK_WHERE_FINAL) { task.blk = ctx->ir_ctx->final_cunt; }
    else { task.blk = UINT32_MAX; }

    for (arg_idx = task.kind == CROSSTALK_HDL_IR_SYSTASK_FINISH ? UINT32_MAX : fn_node->next_sibling; arg_idx != UINT32_MAX; arg_idx = arg_node->next_sibling)
    {
        ret = crosstalk_hdl_ast_node_get(ctx->ast, arg_idx, &arg_node);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        arg = (crosstalk_hdl_ir_systask_arg_t)LDG_STRUCT_ZERO_INIT;
        arg.sig = UINT32_MAX;

        if (arg_node->tok.kind == CROSSTALK_HDL_TOK_LIT_STR)
        {
            arg.str = arg_node->tok;
            arg.str_is = LDG_TRUTH_TRUE;
        }
        else if (arg_node->kind == CROSSTALK_HDL_NODE_EXPR_IDENT && crosstalk_hdl_elab_sym_find(ctx, &arg_node->tok, &sym_idx) == LDG_ERR_AOK && ctx->syms[sym_idx].kind == CROSSTALK_HDL_ELAB_SYM_STR)
        {
            arg.str = arg_node->tok;
            arg.str.off = (uint32_t)(ctx->syms[sym_idx].val >> 32);
            arg.str.len = (uint32_t)ctx->syms[sym_idx].val;
            arg.str_is = LDG_TRUTH_TRUE;
        }
        else
        {
            ret = crosstalk_hdl_elab_expr(ctx, arg_idx, &sig, &width, &sgn);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
            {
                if (arg_node->tok.line != 0) { ctx->err_tok = arg_node->tok; }

                return ret;
            }

            arg.sig = sig;
            arg.width = width;
            arg.sgn = sgn;
        }

        ret = crosstalk_hdl_ir_systask_arg_add(ctx->ir_ctx, &arg);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->err_tok = fn_node->tok; return ret; }

        task.arg_cunt = task.arg_cunt + 0x1;
    }

    ret = crosstalk_hdl_ir_systask_add(ctx->ir_ctx, &task);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->err_tok = fn_node->tok; }

    return ret;
}

static uint32_t crosstalk_hdl_elab_stmt(crosstalk_hdl_elab_ctx_t *ctx, uint32_t node_idx)
{
    const crosstalk_hdl_ast_node_t *node = 0x0;
    const crosstalk_hdl_ast_node_t *child = 0x0;
    crosstalk_hdl_elab_lv_t lv = LDG_STRUCT_ZERO_INIT;
    uint32_t cond_sig = 0;
    uint32_t cond_w = 0;
    uint32_t val_sig = 0;
    uint32_t val_w = 0;
    uint32_t guard_save = 0;
    uint32_t new_guard = 0;
    uint32_t sub_idx = 0;
    uint32_t ret = 0;
    uint8_t cond_sgn = 0;
    uint8_t val_sgn = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_ast_node_get(ctx->ast, node_idx, &node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (node->tok.line != 0) { if (node->tok.line != 0) { ctx->err_tok = node->tok; } }

    if (node->kind == CROSSTALK_HDL_NODE_RET_STMT)
    {
        crosstalk_hdl_elab_lv_t ret_lv = LDG_STRUCT_ZERO_INIT;

        if (LDG_UNLIKELY(ctx->ret_sig == UINT32_MAX))
        {
            if (node->tok.line != 0) { ctx->err_tok = node->tok; }

            return CROSSTALK_HDL_ERR_ELAB;
        }

        if (node->first_child == UINT32_MAX) { return LDG_ERR_AOK; }

        if (ctx->ret_svar_sym != UINT32_MAX)
        {
            uint32_t ret_vals[CROSSTALK_HDL_ELAB_SFLD_MAX];
            uint32_t ret_styp = (uint32_t)ctx->syms[ctx->ret_svar_sym].val;

            ret = crosstalk_hdl_elab_struct_vals_get(ctx, node->first_child, ret_styp, ret_vals);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
            {
                if (ctx->err_tok.line == 0) { if (node->tok.line != 0) { ctx->err_tok = node->tok; } }

                return ret;
            }

            return crosstalk_hdl_elab_struct_assign(ctx, ret_styp, ctx->syms[ctx->ret_svar_sym].sig, UINT32_MAX, ret_vals);
        }

        ctx->cw = ctx->ret_width;
        ret = crosstalk_hdl_elab_expr(ctx, node->first_child, &val_sig, &val_w, &val_sgn);
        ctx->cw = 0x0;
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret_lv.kind = CROSSTALK_HDL_ELAB_LV_SIG;
        ret_lv.sig = ctx->ret_sig;
        ret_lv.width = ctx->ret_width;

        return crosstalk_hdl_elab_assign_apply(ctx, &ret_lv, val_sig, val_w);
    }

    if (node->kind == CROSSTALK_HDL_NODE_STMT_EXPR || node->kind == CROSSTALK_HDL_NODE_EXPR_CALL)
    {
        const crosstalk_hdl_ast_node_t *callee_node = 0x0;
        uint32_t call_idx = node_idx;
        uint32_t call_sig = 0;
        uint32_t call_w = 0;
        uint8_t call_sgn = 0;

        if (node->kind == CROSSTALK_HDL_NODE_STMT_EXPR)
        {
            call_idx = node->first_child;
            ret = crosstalk_hdl_ast_node_get(ctx->ast, call_idx, &child);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (child->kind != CROSSTALK_HDL_NODE_EXPR_CALL) { return CROSSTALK_HDL_ERR_ELAB; }
        }
        else { child = node; }

        ret = crosstalk_hdl_ast_node_get(ctx->ast, child->first_child, &callee_node);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (ctx->src[callee_node->tok.off] == '$')
        {
            uint32_t where = CROSSTALK_HDL_IR_SYSTASK_WHERE_COMB;

            if (ctx->mode == CROSSTALK_HDL_ELAB_MODE_SEQ) { where = CROSSTALK_HDL_IR_SYSTASK_WHERE_SEQ; }
            else if (ctx->mode == CROSSTALK_HDL_ELAB_MODE_FINAL) { where = CROSSTALK_HDL_IR_SYSTASK_WHERE_FINAL; }

            return crosstalk_hdl_elab_systask(ctx, call_idx, where);
        }

        return crosstalk_hdl_elab_func_call(ctx, call_idx, &call_sig, &call_w, &call_sgn);
    }

    if (node->kind == CROSSTALK_HDL_NODE_VAR_DECL || node->kind == CROSSTALK_HDL_NODE_NET_DECL)
    {
        uint8_t stmt_decl_save = ctx->stmt_decl;

        ctx->stmt_decl = LDG_TRUTH_TRUE;
        ret = crosstalk_hdl_elab_item(ctx, node_idx);
        ctx->stmt_decl = stmt_decl_save;

        return ret;
    }

    if (node->kind == CROSSTALK_HDL_NODE_BLK)
    {
        sub_idx = node->first_child;
        while (sub_idx != UINT32_MAX)
        {
            ret = crosstalk_hdl_elab_stmt(ctx, sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_ast_node_get(ctx->ast, sub_idx, &child);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            sub_idx = child->next_sibling;
        }

        return LDG_ERR_AOK;
    }

    if (node->kind == CROSSTALK_HDL_NODE_NB_ASSIGN || node->kind == CROSSTALK_HDL_NODE_B_ASSIGN)
    {
        uint8_t nb_save = ctx->nb_is;

        if (LDG_UNLIKELY(node->first_child == UINT32_MAX)) { return CROSSTALK_HDL_ERR_ELAB; }

        ret = crosstalk_hdl_ast_node_get(ctx->ast, node->first_child, &child);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(child->next_sibling == UINT32_MAX))
        {
            if (node->tok.line != 0) { ctx->err_tok = node->tok; }

            return CROSSTALK_HDL_ERR_ELAB;
        }

        if (child->kind == CROSSTALK_HDL_NODE_EXPR_CONCAT)
        {
            ret = crosstalk_hdl_elab_expr(ctx, child->next_sibling, &val_sig, &val_w, &val_sgn);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ctx->nb_is = node->kind == CROSSTALK_HDL_NODE_NB_ASSIGN ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
            ret = crosstalk_hdl_elab_concat_assign(ctx, node->first_child, val_sig);
            ctx->nb_is = nb_save;

            return ret;
        }

        {
            uint32_t lhs_svar = 0;
            uint32_t lhs_addr = 0;

            if (crosstalk_hdl_elab_svar_probe(ctx, node->first_child, &lhs_svar, &lhs_addr) == LDG_ERR_AOK)
            {
                uint32_t lhs_styp = (uint32_t)ctx->syms[lhs_svar].val;
                uint32_t lhs_base = ctx->syms[lhs_svar].sig;
                uint32_t rhs_vals[CROSSTALK_HDL_ELAB_SFLD_MAX];

                ret = crosstalk_hdl_elab_struct_vals_get(ctx, child->next_sibling, lhs_styp, rhs_vals);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
                {
                    if (ctx->err_tok.line == 0) { if (node->tok.line != 0) { ctx->err_tok = node->tok; } }

                    return ret;
                }

                ctx->nb_is = node->kind == CROSSTALK_HDL_NODE_NB_ASSIGN ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
                ret = crosstalk_hdl_elab_struct_assign(ctx, lhs_styp, lhs_base, lhs_addr, rhs_vals);
                ctx->nb_is = nb_save;

                if (LDG_UNLIKELY(ret != LDG_ERR_AOK) && ctx->err_tok.line == 0) { if (node->tok.line != 0) { ctx->err_tok = node->tok; } }

                return ret;
            }
        }

        ret = crosstalk_hdl_elab_lv_resolve(ctx, node->first_child, &lv);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ctx->cw = lv.width;
        ret = crosstalk_hdl_elab_expr(ctx, child->next_sibling, &val_sig, &val_w, &val_sgn);
        ctx->cw = 0x0;
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ctx->nb_is = node->kind == CROSSTALK_HDL_NODE_NB_ASSIGN ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
        ret = crosstalk_hdl_elab_assign_apply(ctx, &lv, val_sig, val_w);
        ctx->nb_is = nb_save;

        return ret;
    }

    if (node->kind == CROSSTALK_HDL_NODE_IF)
    {
        uint32_t then_idx = 0;
        uint32_t else_idx = UINT32_MAX;

        ret = crosstalk_hdl_elab_expr(ctx, node->first_child, &cond_sig, &cond_w, &cond_sgn);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_node_get(ctx->ast, node->first_child, &child);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        then_idx = child->next_sibling;
        ret = crosstalk_hdl_ast_node_get(ctx->ast, then_idx, &child);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        else_idx = child->next_sibling;
        guard_save = ctx->guard;

        ret = crosstalk_hdl_elab_guard_mix(ctx, cond_sig, LDG_TRUTH_FALSE, &new_guard);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ctx->guard = new_guard;

        ret = crosstalk_hdl_elab_stmt(ctx, then_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->guard = guard_save; return ret; }

        ctx->guard = guard_save;

        if (else_idx != UINT32_MAX)
        {
            ret = crosstalk_hdl_elab_guard_mix(ctx, cond_sig, LDG_TRUTH_TRUE, &new_guard);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ctx->guard = new_guard;

            ret = crosstalk_hdl_elab_stmt(ctx, else_idx);
            ctx->guard = guard_save;

            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        return LDG_ERR_AOK;
    }

    if (node->kind == CROSSTALK_HDL_NODE_CASE || node->kind == CROSSTALK_HDL_NODE_CASEZ || node->kind == CROSSTALK_HDL_NODE_CASEX) { return crosstalk_hdl_elab_case_stmt(ctx, node_idx); }

    if (node->kind == CROSSTALK_HDL_NODE_FOR) { return crosstalk_hdl_elab_for_stmt_unroll(ctx, node_idx, LDG_TRUTH_FALSE); }

    if (node->kind == CROSSTALK_HDL_NODE_VAR_DECL || node->kind == CROSSTALK_HDL_NODE_NET_DECL || node->kind == CROSSTALK_HDL_NODE_GENVAR_DECL)
    {
        uint8_t stmt_decl_save = ctx->stmt_decl;

        ctx->stmt_decl = LDG_TRUTH_TRUE;
        ret = crosstalk_hdl_elab_item(ctx, node_idx);
        ctx->stmt_decl = stmt_decl_save;

        return ret;
    }

    if (node->tok.line != 0) { if (node->tok.line != 0) { ctx->err_tok = node->tok; } }

    return CROSSTALK_HDL_ERR_ELAB;
}

#define CROSSTALK_HDL_ELAB_OARG_CAP 16

uint32_t crosstalk_hdl_elab_func_call(crosstalk_hdl_elab_ctx_t *ctx, uint32_t call_idx, uint32_t *out_sig, uint32_t *out_width, uint8_t *out_sgn)
{
    const crosstalk_hdl_ast_node_t *call_node = 0x0;
    const crosstalk_hdl_ast_node_t *fn_node = 0x0;
    const crosstalk_hdl_ast_node_t *def_node = 0x0;
    const crosstalk_hdl_ast_node_t *child = 0x0;
    const crosstalk_hdl_ast_node_t *port_sub = 0x0;
    const crosstalk_hdl_ast_node_t *arg_node = 0x0;
    crosstalk_hdl_ir_op_t op = LDG_STRUCT_ZERO_INIT;
    uint32_t func_idx = UINT32_MAX;
    uint32_t child_idx = 0;
    uint32_t arg_idx = 0;
    uint32_t ret_sig = 0;
    uint32_t ret_w = 0x1;
    uint32_t ret_sig_save = 0;
    uint32_t ret_w_save = 0;
    uint32_t ret_styp = UINT32_MAX;
    uint32_t ret_svar_save = 0;
    uint32_t ret_svar_new = UINT32_MAX;
    uint32_t sym_mark = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;
    uint32_t oarg_node[CROSSTALK_HDL_ELAB_OARG_CAP];
    uint32_t oarg_base[CROSSTALK_HDL_ELAB_OARG_CAP];
    uint32_t oarg_styp[CROSSTALK_HDL_ELAB_OARG_CAP];
    uint32_t oarg_w[CROSSTALK_HDL_ELAB_OARG_CAP];
    uint32_t oarg_cunt = 0;
    uint32_t arg_dir = CROSSTALK_HDL_IR_PORT_IN;

    if (LDG_UNLIKELY(ctx == 0x0 || out_sig == 0x0 || out_width == 0x0 || out_sgn == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_sig = UINT32_MAX;
    *out_width = 0;
    *out_sgn = 0;

    ret = crosstalk_hdl_ast_node_get(ctx->ast, call_idx, &call_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_ast_node_get(ctx->ast, call_node->first_child, &fn_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < ctx->func_cunt; idx++) { if (crosstalk_hdl_elab_tok_eq_is(ctx->src, &ctx->funcs[idx].name, &fn_node->tok) == LDG_TRUTH_TRUE) { func_idx = idx; break; } }

    if (func_idx == UINT32_MAX) { ctx->err_tok = fn_node->tok; return CROSSTALK_HDL_ERR_ELAB; }

    if (LDG_UNLIKELY(ctx->depth >= CROSSTALK_HDL_ELAB_DEPTH_MAX)) { ctx->err_tok = fn_node->tok; return CROSSTALK_HDL_ERR_ELAB; }

    ctx->depth++;
    sym_mark = ctx->sym_cunt;

    ret = crosstalk_hdl_ast_node_get(ctx->ast, ctx->funcs[func_idx].node, &def_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

    child_idx = def_node->first_child;
    if (child_idx != UINT32_MAX)
    {
        ret = crosstalk_hdl_ast_node_get(ctx->ast, child_idx, &child);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

        if (child->kind == CROSSTALK_HDL_NODE_TYPE)
        {
            if (child->tok.kind == CROSSTALK_HDL_TOK_IDENT)
            {
                uint32_t type_sym = 0;

                ret = crosstalk_hdl_elab_sym_find(ctx, &child->tok, &type_sym);
                if (ret == LDG_ERR_AOK && ctx->syms[type_sym].kind == CROSSTALK_HDL_ELAB_SYM_STYP) { ret_styp = (uint32_t)ctx->syms[type_sym].val; }
            }

            if (ret_styp == UINT32_MAX)
            {
                ret = crosstalk_hdl_elab_type_width_get(ctx, child_idx, &ret_w);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }
            }
            else { ret_w = ctx->styps[ret_styp].total_width; }
        }
    }

    if (ret_styp != UINT32_MAX)
    {
        const crosstalk_hdl_elab_styp_t *styp = &ctx->styps[ret_styp];
        uint32_t fld_idx = 0;
        uint32_t fsig = 0;

        for (fld_idx = 0; fld_idx < styp->field_cunt; fld_idx++)
        {
            const crosstalk_hdl_elab_sfield_t *fld = &ctx->sfields[styp->field_base + fld_idx];

            ret = crosstalk_hdl_ir_sig_add(ctx->ir_ctx, &fld->name, fld->width, 0x0, CROSSTALK_HDL_IR_SIG_VAR, CROSSTALK_HDL_IR_PORT_NONE, 0x0, &fsig);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

            if (fld_idx == 0) { ret_sig = fsig; }
        }

        ret = crosstalk_hdl_elab_sym_add(ctx, &def_node->tok, CROSSTALK_HDL_ELAB_SYM_SVAR, ret_sig, (uint64_t)ret_styp, 0x0);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

        ret_svar_new = ctx->sym_cunt - 0x1;
    }
    else
    {
        ret = crosstalk_hdl_elab_tmp_make(ctx, ret_w, &ret_sig);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

        ret = crosstalk_hdl_elab_sym_add(ctx, &def_node->tok, CROSSTALK_HDL_ELAB_SYM_SIG, ret_sig, 0x0, ret_w);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }
    }

    ret_sig_save = ctx->ret_sig;
    ret_w_save = ctx->ret_width;
    ret_svar_save = ctx->ret_svar_sym;
    ctx->ret_sig = ret_sig;
    ctx->ret_width = ret_w;
    ctx->ret_svar_sym = ret_svar_new;

    arg_idx = fn_node->next_sibling;
    child_idx = def_node->first_child;
    while (child_idx != UINT32_MAX)
    {
        ret = crosstalk_hdl_ast_node_get(ctx->ast, child_idx, &child);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

        if (child->kind == CROSSTALK_HDL_NODE_PORT)
        {
            uint32_t formal_w = 0x1;
            uint32_t formal_styp = UINT32_MAX;
            uint32_t sub_idx = child->first_child;

            while (sub_idx != UINT32_MAX)
            {
                ret = crosstalk_hdl_ast_node_get(ctx->ast, sub_idx, &port_sub);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

                if (port_sub->kind == CROSSTALK_HDL_NODE_DIR) { if (port_sub->tok.val == CROSSTALK_HDL_KW_INPUT) { arg_dir = CROSSTALK_HDL_IR_PORT_IN; }
                    else if (port_sub->tok.val == CROSSTALK_HDL_KW_OUTPUT) { arg_dir = CROSSTALK_HDL_IR_PORT_OUT; }
                    else { arg_dir = CROSSTALK_HDL_IR_PORT_INOUT; } }
                else if (port_sub->kind == CROSSTALK_HDL_NODE_TYPE)
                {
                    formal_styp = UINT32_MAX;

                    if (port_sub->tok.kind == CROSSTALK_HDL_TOK_IDENT)
                    {
                        uint32_t type_sym = 0;

                        ret = crosstalk_hdl_elab_sym_find(ctx, &port_sub->tok, &type_sym);
                        if (ret == LDG_ERR_AOK && ctx->syms[type_sym].kind == CROSSTALK_HDL_ELAB_SYM_STYP) { formal_styp = (uint32_t)ctx->syms[type_sym].val; }
                    }

                    if (formal_styp == UINT32_MAX)
                    {
                        ret = crosstalk_hdl_elab_type_width_get(ctx, sub_idx, &formal_w);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }
                    }
                }
                else if (port_sub->kind == CROSSTALK_HDL_NODE_DECL_NAME && formal_styp != UINT32_MAX)
                {
                    const crosstalk_hdl_elab_styp_t *arg_styp = &ctx->styps[formal_styp];
                    uint32_t arg_vals[CROSSTALK_HDL_ELAB_SFLD_MAX];
                    uint32_t arg_base = UINT32_MAX;
                    uint32_t fld_k = 0;

                    if (arg_idx == UINT32_MAX) { ctx->err_tok = fn_node->tok; ctx->depth--; return CROSSTALK_HDL_ERR_ELAB; }

                    if (arg_dir != CROSSTALK_HDL_IR_PORT_OUT)
                    {
                        ret = crosstalk_hdl_elab_struct_vals_get(ctx, arg_idx, formal_styp, arg_vals);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }
                    }

                    for (fld_k = 0; fld_k < arg_styp->field_cunt; fld_k++)
                    {
                        const crosstalk_hdl_elab_sfield_t *fld = &ctx->sfields[arg_styp->field_base + fld_k];
                        uint32_t fsig = 0;

                        ret = crosstalk_hdl_ir_sig_add(ctx->ir_ctx, &fld->name, fld->width, 0x0, CROSSTALK_HDL_IR_SIG_VAR, CROSSTALK_HDL_IR_PORT_NONE, 0x0, &fsig);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

                        if (arg_dir != CROSSTALK_HDL_IR_PORT_OUT)
                        {
                            op.kind = CROSSTALK_HDL_IR_OP_COPY;
                            op.dst = fsig;
                            op.src0 = arg_vals[fld_k];
                            op.src1 = UINT32_MAX;
                            op.src2 = UINT32_MAX;
                            op.imm = 0;
                            op.width = fld->width;

                            ret = crosstalk_hdl_elab_op_emit(ctx, &op);
                            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }
                        }

                        if (fld_k == 0) { arg_base = fsig; }
                    }

                    ret = crosstalk_hdl_elab_sym_add(ctx, &port_sub->tok, CROSSTALK_HDL_ELAB_SYM_SVAR, arg_base, (uint64_t)formal_styp, 0x0);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

                    if (arg_dir != CROSSTALK_HDL_IR_PORT_IN)
                    {
                        if (LDG_UNLIKELY(oarg_cunt >= CROSSTALK_HDL_ELAB_OARG_CAP)) { ctx->depth--; return LDG_ERR_FULL; }

                        oarg_node[oarg_cunt] = arg_idx;
                        oarg_base[oarg_cunt] = arg_base;
                        oarg_styp[oarg_cunt] = formal_styp;
                        oarg_w[oarg_cunt] = 0;
                        oarg_cunt++;
                    }

                    ret = crosstalk_hdl_ast_node_get(ctx->ast, arg_idx, &arg_node);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

                    arg_idx = arg_node->next_sibling;
                }
                else if (port_sub->kind == CROSSTALK_HDL_NODE_DECL_NAME)
                {
                    uint32_t val_sig = 0;
                    uint32_t val_w = 0;
                    uint32_t formal_sig = 0;
                    uint8_t val_sgn = 0;

                    if (arg_idx == UINT32_MAX) { ctx->err_tok = fn_node->tok; ctx->depth--; return CROSSTALK_HDL_ERR_ELAB; }

                    ret = crosstalk_hdl_elab_tmp_make(ctx, formal_w, &formal_sig);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

                    if (arg_dir != CROSSTALK_HDL_IR_PORT_OUT)
                    {
                        ret = crosstalk_hdl_elab_expr(ctx, arg_idx, &val_sig, &val_w, &val_sgn);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

                        ret = crosstalk_hdl_elab_width_fit(ctx, val_sig, val_w, LDG_TRUTH_FALSE, formal_w, &val_sig);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

                        op.kind = CROSSTALK_HDL_IR_OP_COPY;
                        op.dst = formal_sig;
                        op.src0 = val_sig;
                        op.src1 = UINT32_MAX;
                        op.src2 = UINT32_MAX;
                        op.imm = 0;
                        op.width = formal_w;

                        ret = crosstalk_hdl_elab_op_emit(ctx, &op);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }
                    }

                    ret = crosstalk_hdl_elab_sym_add(ctx, &port_sub->tok, CROSSTALK_HDL_ELAB_SYM_SIG, formal_sig, 0x0, formal_w);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

                    if (arg_dir != CROSSTALK_HDL_IR_PORT_IN)
                    {
                        if (LDG_UNLIKELY(oarg_cunt >= CROSSTALK_HDL_ELAB_OARG_CAP)) { ctx->depth--; return LDG_ERR_FULL; }

                        oarg_node[oarg_cunt] = arg_idx;
                        oarg_base[oarg_cunt] = formal_sig;
                        oarg_styp[oarg_cunt] = UINT32_MAX;
                        oarg_w[oarg_cunt] = formal_w;
                        oarg_cunt++;
                    }

                    ret = crosstalk_hdl_ast_node_get(ctx->ast, arg_idx, &arg_node);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

                    arg_idx = arg_node->next_sibling;
                }

                sub_idx = port_sub->next_sibling;
            }
        }
        else if (child->kind != CROSSTALK_HDL_NODE_TYPE)
        {
            ret = crosstalk_hdl_elab_stmt(ctx, child_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }
        }

        child_idx = child->next_sibling;
    }

    ctx->sym_cunt = sym_mark;

    for (idx = 0; idx < oarg_cunt; idx++)
    {
        if (oarg_styp[idx] != UINT32_MAX)
        {
            const crosstalk_hdl_elab_styp_t *ostyp = &ctx->styps[oarg_styp[idx]];
            uint32_t rhs_vals[CROSSTALK_HDL_ELAB_SFLD_MAX];
            uint32_t lhs_svar = 0;
            uint32_t lhs_addr = 0;
            uint32_t fld_k = 0;

            ret = crosstalk_hdl_elab_svar_probe(ctx, oarg_node[idx], &lhs_svar, &lhs_addr);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->err_tok = fn_node->tok; ctx->depth--; return CROSSTALK_HDL_ERR_ELAB; }

            for (fld_k = 0; fld_k < ostyp->field_cunt; fld_k++)
            {
                ret = crosstalk_hdl_elab_sig_current_get(ctx, oarg_base[idx] + fld_k, &rhs_vals[fld_k]);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }
            }

            ret = crosstalk_hdl_elab_struct_assign(ctx, oarg_styp[idx], ctx->syms[lhs_svar].sig, lhs_addr, rhs_vals);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }
        }
        else
        {
            crosstalk_hdl_elab_lv_t oarg_lv = LDG_STRUCT_ZERO_INIT;
            uint32_t oarg_val = 0;

            ret = crosstalk_hdl_elab_sig_current_get(ctx, oarg_base[idx], &oarg_val);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

            ret = crosstalk_hdl_elab_lv_resolve(ctx, oarg_node[idx], &oarg_lv);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

            ret = crosstalk_hdl_elab_assign_apply(ctx, &oarg_lv, oarg_val, oarg_w[idx]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }
        }
    }

    if (ret_styp != UINT32_MAX)
    {
        *out_sig = ret_sig;
        ctx->call_styp = ret_styp;
        ctx->call_base = ret_sig;
    }
    else
    {
        ret = crosstalk_hdl_elab_sig_current_get(ctx, ret_sig, out_sig);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

        ctx->call_styp = UINT32_MAX;
        ctx->call_base = UINT32_MAX;
    }

    *out_width = ret_w;
    ctx->ret_sig = ret_sig_save;
    ctx->ret_width = ret_w_save;
    ctx->ret_svar_sym = ret_svar_save;
    ctx->sym_cunt = sym_mark;
    ctx->depth--;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_elab_proc(crosstalk_hdl_elab_ctx_t *ctx, uint32_t node_idx)
{
    const crosstalk_hdl_ast_node_t *node = 0x0;
    const crosstalk_hdl_ast_node_t *sens_node = 0x0;
    const crosstalk_hdl_ast_node_t *edge_node = 0x0;
    const crosstalk_hdl_ast_node_t *edge_expr = 0x0;
    uint32_t sens_idx = 0;
    uint32_t stmt_idx = 0;
    uint32_t clk_sig = UINT32_MAX;
    uint32_t clk_edge = CROSSTALK_HDL_IR_EDGE_POS;
    uint32_t rst_sig = UINT32_MAX;
    uint32_t rst_edge = CROSSTALK_HDL_IR_EDGE_POS;
    uint32_t edge_idx = 0;
    uint32_t sig_w = 0;
    uint32_t ret = 0;
    uint8_t seq = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_ast_node_get(ctx->ast, node_idx, &node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    sens_idx = node->first_child;
    ret = crosstalk_hdl_ast_node_get(ctx->ast, sens_idx, &sens_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    stmt_idx = sens_node->next_sibling;

    edge_idx = sens_node->first_child;
    while (edge_idx != UINT32_MAX)
    {
        ret = crosstalk_hdl_ast_node_get(ctx->ast, edge_idx, &edge_node);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (edge_node->tok.kind == CROSSTALK_HDL_TOK_KEYWORD)
        {
            uint32_t edge_dir = edge_node->tok.val == CROSSTALK_HDL_KW_NEGEDGE ? CROSSTALK_HDL_IR_EDGE_NEG : CROSSTALK_HDL_IR_EDGE_POS;
            uint32_t edge_sig = 0;
            uint8_t edge_sgn = 0;

            seq = LDG_TRUTH_TRUE;

            ret = crosstalk_hdl_ast_node_get(ctx->ast, edge_node->first_child, &edge_expr);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_elab_expr(ctx, edge_node->first_child, &edge_sig, &sig_w, &edge_sgn);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (clk_sig == UINT32_MAX)
            {
                clk_sig = edge_sig;
                clk_edge = edge_dir;
            }
            else if (rst_sig == UINT32_MAX)
            {
                rst_sig = edge_sig;
                rst_edge = edge_dir;
            }
        }

        edge_idx = edge_node->next_sibling;
    }

    if (node->kind == CROSSTALK_HDL_NODE_ALWAYS_FF && LDG_UNLIKELY(clk_sig == UINT32_MAX))
    {
        if (node->tok.line != 0) { ctx->err_tok = node->tok; }

        return CROSSTALK_HDL_ERR_ELAB;
    }

    if (seq == LDG_TRUTH_TRUE)
    {
        ctx->mode = CROSSTALK_HDL_ELAB_MODE_SEQ;

        ret = crosstalk_hdl_ir_seq_blk_open(ctx->ir_ctx, clk_sig, clk_edge, rst_sig, rst_edge, &ctx->seq_blk);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    else { ctx->mode = CROSSTALK_HDL_ELAB_MODE_COMB; }

    ctx->guard = UINT32_MAX;

    {
        uint32_t proc_sym_mark = ctx->sym_cunt;

        ret = crosstalk_hdl_elab_stmt(ctx, stmt_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->mode = CROSSTALK_HDL_ELAB_MODE_COMB; ctx->sym_cunt = proc_sym_mark; return ret; }

        ret = crosstalk_hdl_elab_commit(ctx);
        ctx->sym_cunt = proc_sym_mark;
    }

    ctx->mode = CROSSTALK_HDL_ELAB_MODE_COMB;
    ctx->guard = UINT32_MAX;

    return ret;
}

static uint32_t crosstalk_hdl_elab_final(crosstalk_hdl_elab_ctx_t *ctx, uint32_t node_idx)
{
    const crosstalk_hdl_ast_node_t *node = 0x0;
    uint32_t sym_mark = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_ast_node_get(ctx->ast, node_idx, &node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    sym_mark = ctx->sym_cunt;
    ctx->mode = CROSSTALK_HDL_ELAB_MODE_FINAL;
    ctx->guard = UINT32_MAX;

    ret = crosstalk_hdl_elab_stmt(ctx, node->first_child);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_elab_commit(ctx); }

    ctx->sym_cunt = sym_mark;
    ctx->mode = CROSSTALK_HDL_ELAB_MODE_COMB;
    ctx->guard = UINT32_MAX;

    if (ret == LDG_ERR_AOK) { ctx->ir_ctx->final_cunt = ctx->ir_ctx->final_cunt + 0x1; }

    return ret;
}

static uint32_t crosstalk_hdl_elab_initial_mem(crosstalk_hdl_elab_ctx_t *ctx, const crosstalk_hdl_ast_node_t *node, const crosstalk_hdl_ast_node_t *lhs)
{
    const crosstalk_hdl_ast_node_t *base_node = 0x0;
    uint64_t addr = 0;
    uint64_t val = 0;
    uint32_t sym_idx = 0;
    uint32_t sig_idx = 0;
    uint32_t ret = 0;

    ret = crosstalk_hdl_ast_node_get(ctx->ast, lhs->first_child, &base_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(base_node->kind != CROSSTALK_HDL_NODE_EXPR_IDENT)) { ctx->err_tok = lhs->tok; return CROSSTALK_HDL_ERR_ELAB; }

    ret = crosstalk_hdl_elab_sym_find(ctx, &base_node->tok, &sym_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->err_tok = base_node->tok; return CROSSTALK_HDL_ERR_ELAB; }

    if (LDG_UNLIKELY(ctx->syms[sym_idx].kind != CROSSTALK_HDL_ELAB_SYM_MEM)) { ctx->err_tok = base_node->tok; return CROSSTALK_HDL_ERR_ELAB; }

    ret = crosstalk_hdl_elab_const_eval(ctx, base_node->next_sibling, &addr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->err_tok = lhs->tok; return CROSSTALK_HDL_ERR_ELAB; }

    ret = crosstalk_hdl_elab_const_eval(ctx, lhs->next_sibling, &val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->err_tok = node->tok; return CROSSTALK_HDL_ERR_ELAB; }

    sig_idx = ctx->syms[sym_idx].sig;

    ret = crosstalk_hdl_ir_mem_init_add(ctx->ir_ctx, sig_idx, addr, val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->err_tok = lhs->tok; return ret == LDG_ERR_BOUNDS ? CROSSTALK_HDL_ERR_ELAB : ret; }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_elab_initial(crosstalk_hdl_elab_ctx_t *ctx, uint32_t node_idx)
{
    const crosstalk_hdl_ast_node_t *node = 0x0;
    const crosstalk_hdl_ast_node_t *child = 0x0;
    const crosstalk_hdl_ast_node_t *call_node = 0x0;
    const crosstalk_hdl_ast_node_t *fn_node = 0x0;
    const crosstalk_hdl_ast_node_t *arg_node = 0x0;
    uint64_t init_val = 0;
    uint32_t sym_idx = 0;
    uint32_t sub_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_ast_node_get(ctx->ast, node_idx, &node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (node->kind == CROSSTALK_HDL_NODE_INITIAL || node->kind == CROSSTALK_HDL_NODE_BLK)
    {
        sub_idx = node->first_child;
        while (sub_idx != UINT32_MAX)
        {
            ret = crosstalk_hdl_elab_initial(ctx, sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_ast_node_get(ctx->ast, sub_idx, &child);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            sub_idx = child->next_sibling;
        }

        return LDG_ERR_AOK;
    }

    if (node->kind == CROSSTALK_HDL_NODE_FOR)
    {
        ctx->warn_cunt = ctx->warn_cunt + 0x1;

        return LDG_ERR_AOK;
    }

    if (node->kind == CROSSTALK_HDL_NODE_IF)
    {
        uint64_t cond_val = 0;

        ret = crosstalk_hdl_elab_const_eval(ctx, node->first_child, &cond_val);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return CROSSTALK_HDL_ERR_ELAB; }

        ret = crosstalk_hdl_ast_node_get(ctx->ast, node->first_child, &child);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        sub_idx = child->next_sibling;
        if (sub_idx == UINT32_MAX) { return LDG_ERR_AOK; }

        if (cond_val != 0) { return crosstalk_hdl_elab_initial(ctx, sub_idx); }

        ret = crosstalk_hdl_ast_node_get(ctx->ast, sub_idx, &child);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (child->next_sibling == UINT32_MAX) { return LDG_ERR_AOK; }

        return crosstalk_hdl_elab_initial(ctx, child->next_sibling);
    }

    if (node->kind == CROSSTALK_HDL_NODE_B_ASSIGN || node->kind == CROSSTALK_HDL_NODE_NB_ASSIGN)
    {
        ret = crosstalk_hdl_ast_node_get(ctx->ast, node->first_child, &child);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (child->kind == CROSSTALK_HDL_NODE_EXPR_IDX) { return crosstalk_hdl_elab_initial_mem(ctx, node, child); }

        if (LDG_UNLIKELY(child->kind != CROSSTALK_HDL_NODE_EXPR_IDENT))
        {
            if (node->tok.line != 0) { ctx->err_tok = node->tok; }

            return CROSSTALK_HDL_ERR_ELAB;
        }

        ret = crosstalk_hdl_elab_sym_find(ctx, &child->tok, &sym_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->err_tok = child->tok; return CROSSTALK_HDL_ERR_ELAB; }

        ret = crosstalk_hdl_elab_const_eval(ctx, child->next_sibling, &init_val);
        if (ret != LDG_ERR_AOK)
        {
            const crosstalk_hdl_ast_node_t *rhs_node = 0x0;
            const crosstalk_hdl_ast_node_t *callee_node = 0x0;

            ret = crosstalk_hdl_ast_node_get(ctx->ast, child->next_sibling, &rhs_node);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (rhs_node->kind != CROSSTALK_HDL_NODE_EXPR_CALL) { ctx->err_tok = rhs_node->tok; return CROSSTALK_HDL_ERR_ELAB; }

            ret = crosstalk_hdl_ast_node_get(ctx->ast, rhs_node->first_child, &callee_node);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (LDG_UNLIKELY(ctx->src[callee_node->tok.off] != '$')) { ctx->err_tok = callee_node->tok; return CROSSTALK_HDL_ERR_ELAB; }

            init_val = 0;
        }

        if (LDG_UNLIKELY(ctx->syms[sym_idx].sig >= ctx->ir_ctx->sig_cunt)) { return LDG_ERR_BOUNDS; }

        ctx->ir_ctx->sigs[ctx->syms[sym_idx].sig].init_val = init_val;

        return LDG_ERR_AOK;
    }

    if (node->kind == CROSSTALK_HDL_NODE_STMT_EXPR)
    {
        uint32_t call_idx = node->first_child;

        ret = crosstalk_hdl_ast_node_get(ctx->ast, call_idx, &call_node);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (call_node->kind == CROSSTALK_HDL_NODE_EXPR_CAST)
        {
            ret = crosstalk_hdl_ast_node_get(ctx->ast, call_node->first_child, &fn_node);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (LDG_UNLIKELY(fn_node->kind != CROSSTALK_HDL_NODE_EXPR_IDENT || crosstalk_hdl_elab_tok_lit_eq_is(ctx->src, &fn_node->tok, CROSSTALK_HDL_ELAB_CAST_VOID_NAME) != LDG_TRUTH_TRUE))
            {
                if (call_node->tok.line != 0) { ctx->err_tok = call_node->tok; }

                return CROSSTALK_HDL_ERR_ELAB;
            }

            call_idx = fn_node->next_sibling;

            ret = crosstalk_hdl_ast_node_get(ctx->ast, call_idx, &call_node);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        if (LDG_UNLIKELY(call_node->kind != CROSSTALK_HDL_NODE_EXPR_CALL))
        {
            if (call_node->tok.line != 0) { ctx->err_tok = call_node->tok; }

            return CROSSTALK_HDL_ERR_ELAB;
        }

        ret = crosstalk_hdl_ast_node_get(ctx->ast, call_node->first_child, &fn_node);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (crosstalk_hdl_elab_tok_lit_eq_is(ctx->src, &fn_node->tok, "$readmemh") == LDG_TRUTH_TRUE || crosstalk_hdl_elab_tok_lit_eq_is(ctx->src, &fn_node->tok, "$readmemb") == LDG_TRUTH_TRUE)
        {
            const crosstalk_hdl_ast_node_t *mem_node = 0x0;

            ret = crosstalk_hdl_ast_node_get(ctx->ast, fn_node->next_sibling, &arg_node);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_ast_node_get(ctx->ast, arg_node->next_sibling, &mem_node);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_elab_sym_find(ctx, &mem_node->tok, &sym_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->err_tok = mem_node->tok; return CROSSTALK_HDL_ERR_ELAB; }

            if (arg_node->tok.kind == CROSSTALK_HDL_TOK_IDENT)
            {
                crosstalk_hdl_tok_t file_tok = arg_node->tok;
                uint32_t str_sym = 0;

                ret = crosstalk_hdl_elab_sym_find(ctx, &arg_node->tok, &str_sym);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK || ctx->syms[str_sym].kind != CROSSTALK_HDL_ELAB_SYM_STR)) { ctx->err_tok = arg_node->tok; return CROSSTALK_HDL_ERR_ELAB; }

                file_tok.off = (uint32_t)(ctx->syms[str_sym].val >> 32);
                file_tok.len = (uint32_t)ctx->syms[str_sym].val;

                return crosstalk_hdl_ir_sig_mem_file_set(ctx->ir_ctx, ctx->syms[sym_idx].sig, &file_tok);
            }

            return crosstalk_hdl_ir_sig_mem_file_set(ctx->ir_ctx, ctx->syms[sym_idx].sig, &arg_node->tok);
        }

        if (crosstalk_hdl_elab_tok_lit_eq_is(ctx->src, &fn_node->tok, CROSSTALK_HDL_ELAB_SYSTASK_DISPLAY_NAME) == LDG_TRUTH_TRUE || crosstalk_hdl_elab_tok_lit_eq_is(ctx->src, &fn_node->tok, CROSSTALK_HDL_ELAB_SYSTASK_WR_NAME) == LDG_TRUTH_TRUE || crosstalk_hdl_elab_tok_lit_eq_is(ctx->src, &fn_node->tok, CROSSTALK_HDL_ELAB_SYSTASK_FINISH_NAME) == LDG_TRUTH_TRUE) { return crosstalk_hdl_elab_systask(ctx, call_idx, CROSSTALK_HDL_IR_SYSTASK_WHERE_INITIAL); }

        if (crosstalk_hdl_elab_tok_lit_eq_is(ctx->src, &fn_node->tok, CROSSTALK_HDL_ELAB_PLUSARGS_TEST_NAME) == LDG_TRUTH_TRUE || crosstalk_hdl_elab_tok_lit_eq_is(ctx->src, &fn_node->tok, CROSSTALK_HDL_ELAB_PLUSARGS_VAL_NAME) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

        ctx->err_tok = fn_node->tok;

        return CROSSTALK_HDL_ERR_ELAB;
    }

    if (node->tok.line != 0) { if (node->tok.line != 0) { ctx->err_tok = node->tok; } }

    return CROSSTALK_HDL_ERR_ELAB;
}

static uint32_t crosstalk_hdl_elab_port_mem_alias_get(crosstalk_hdl_elab_ctx_t *ctx, const crosstalk_hdl_tok_t *name, uint32_t width, uint64_t depth, uint64_t dim_inner, uint32_t *out_sig)
{
    const crosstalk_hdl_elab_conn_t *conns = 0x0;
    const crosstalk_hdl_ir_sig_t *actual = 0x0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0 || out_sig == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_sig = UINT32_MAX;

    if (ctx->port_conn == 0x0) { return LDG_ERR_AOK; }

    conns = (const crosstalk_hdl_elab_conn_t *)ctx->port_conn;

    for (idx = 0; idx < ctx->port_conn_cunt; idx++)
    {
        if (conns[idx].mem_sig == UINT32_MAX || crosstalk_hdl_elab_tok_eq_is(ctx->src, &conns[idx].name, name) != LDG_TRUTH_TRUE) { continue; }

        actual = &ctx->ir_ctx->sigs[conns[idx].mem_sig];

        if (LDG_UNLIKELY(actual->width != width || (uint64_t)actual->depth != depth || (uint64_t)conns[idx].mem_inner != dim_inner)) { ctx->err_tok = *name; return CROSSTALK_HDL_ERR_ELAB_WIDTH; }

        *out_sig = conns[idx].mem_sig;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_elab_decl(crosstalk_hdl_elab_ctx_t *ctx, uint32_t node_idx, uint32_t port_dir)
{
    const crosstalk_hdl_ast_node_t *node = 0x0;
    const crosstalk_hdl_ast_node_t *child = 0x0;
    const crosstalk_hdl_ast_node_t *range_node = 0x0;
    const crosstalk_hdl_ast_node_t *range_lo = 0x0;
    uint64_t range_a = 0;
    uint64_t range_b = 0;
    uint32_t type_idx = UINT32_MAX;
    uint32_t width = 0x1;
    uint32_t child_idx = 0;
    uint32_t sig_idx = 0;
    uint32_t decl_styp = UINT32_MAX;
    uint32_t ret = 0;
    uint8_t genvar = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_ast_node_get(ctx->ast, node_idx, &node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (node->kind == CROSSTALK_HDL_NODE_GENVAR_DECL) { genvar = LDG_TRUTH_TRUE; }

    child_idx = node->first_child;
    while (child_idx != UINT32_MAX)
    {
        ret = crosstalk_hdl_ast_node_get(ctx->ast, child_idx, &child);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (child->kind == CROSSTALK_HDL_NODE_TYPE)
        {
            type_idx = child_idx;
            decl_styp = UINT32_MAX;

            if (child->tok.kind == CROSSTALK_HDL_TOK_IDENT)
            {
                uint32_t type_sym = 0;

                ret = crosstalk_hdl_elab_sym_find(ctx, &child->tok, &type_sym);
                if (ret == LDG_ERR_AOK && ctx->syms[type_sym].kind == CROSSTALK_HDL_ELAB_SYM_STYP) { decl_styp = (uint32_t)ctx->syms[type_sym].val; }
            }

            if (decl_styp == UINT32_MAX)
            {
                ret = crosstalk_hdl_elab_type_width_get(ctx, type_idx, &width);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
            else { width = ctx->styps[decl_styp].total_width; }
        }
        else if (child->kind == CROSSTALK_HDL_NODE_DIR) { if (child->tok.val == CROSSTALK_HDL_KW_INPUT) { port_dir = CROSSTALK_HDL_IR_PORT_IN; }
            else if (child->tok.val == CROSSTALK_HDL_KW_OUTPUT) { port_dir = CROSSTALK_HDL_IR_PORT_OUT; }
            else { port_dir = CROSSTALK_HDL_IR_PORT_INOUT; } }
        else if (child->kind == CROSSTALK_HDL_NODE_DECL_NAME)
        {
            uint64_t depth = 0;
            uint64_t dim_inner = 0;
            uint32_t init_idx = UINT32_MAX;
            uint32_t sub_idx = child->first_child;

            if (child->tok.line != 0) { ctx->err_tok = child->tok; }

            while (sub_idx != UINT32_MAX)
            {
                ret = crosstalk_hdl_ast_node_get(ctx->ast, sub_idx, &range_node);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                if (range_node->kind == CROSSTALK_HDL_NODE_UNPACKED_RANGE)
                {
                    uint64_t dim_now = 0;

                    ret = crosstalk_hdl_elab_const_eval(ctx, range_node->first_child, &range_a);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    ret = crosstalk_hdl_ast_node_get(ctx->ast, range_node->first_child, &range_lo);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    if (range_lo->next_sibling == UINT32_MAX) { dim_now = range_a; }
                    else
                    {
                        ret = crosstalk_hdl_elab_const_eval(ctx, range_lo->next_sibling, &range_b);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                        dim_now = range_a >= range_b ? range_a - range_b + 0x1 : range_b - range_a + 0x1;
                    }

                    dim_inner = depth == 0 ? 0x0 : dim_now;
                    depth = depth == 0 ? dim_now : depth * dim_now;
                }
                else if (range_node->kind != CROSSTALK_HDL_NODE_ATTR) { init_idx = sub_idx; }

                sub_idx = range_node->next_sibling;
            }

            if (genvar == LDG_TRUTH_TRUE)
            {
                uint64_t gen_val = 0;

                if (init_idx != UINT32_MAX)
                {
                    ret = crosstalk_hdl_elab_const_eval(ctx, init_idx, &gen_val);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
                }

                ret = crosstalk_hdl_elab_sym_add(ctx, &child->tok, CROSSTALK_HDL_ELAB_SYM_PARAM, UINT32_MAX, gen_val, 32);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
            else if (decl_styp != UINT32_MAX)
            {
                const crosstalk_hdl_elab_styp_t *styp = &ctx->styps[decl_styp];
                uint32_t base_sig = UINT32_MAX;
                uint32_t fsig = 0;
                uint32_t fld_idx = 0;
                uint32_t fld_sig_kind = node->kind == CROSSTALK_HDL_NODE_NET_DECL || node->kind == CROSSTALK_HDL_NODE_PORT ? CROSSTALK_HDL_IR_SIG_NET : CROSSTALK_HDL_IR_SIG_VAR;

                for (fld_idx = 0; fld_idx < styp->field_cunt; fld_idx++)
                {
                    const crosstalk_hdl_elab_sfield_t *fld = &ctx->sfields[styp->field_base + fld_idx];

                    if (depth > 0) { ret = crosstalk_hdl_ir_sig_add(ctx->ir_ctx, &fld->name, fld->width, (uint32_t)depth, CROSSTALK_HDL_IR_SIG_MEM, port_dir, 0x0, &fsig); }
                    else { ret = crosstalk_hdl_ir_sig_add(ctx->ir_ctx, &fld->name, fld->width, 0x0, fld_sig_kind, port_dir, 0x0, &fsig); }

                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    if (fld_idx == 0) { base_sig = fsig; }
                }

                ret = crosstalk_hdl_elab_sym_add(ctx, &child->tok, CROSSTALK_HDL_ELAB_SYM_SVAR, base_sig, (uint64_t)decl_styp | (depth << 32), (uint32_t)dim_inner);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
            else if (depth > 0)
            {
                ret = LDG_ERR_AOK;
                sig_idx = UINT32_MAX;

                if (node->kind == CROSSTALK_HDL_NODE_PORT) { ret = crosstalk_hdl_elab_port_mem_alias_get(ctx, &child->tok, width, depth, dim_inner, &sig_idx); }

                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                if (sig_idx == UINT32_MAX) { ret = crosstalk_hdl_ir_sig_add(ctx->ir_ctx, &child->tok, width, (uint32_t)depth, CROSSTALK_HDL_IR_SIG_MEM, port_dir, 0x0, &sig_idx); }

                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                ret = crosstalk_hdl_elab_sym_add(ctx, &child->tok, CROSSTALK_HDL_ELAB_SYM_MEM, sig_idx, dim_inner, width);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
            else
            {
                uint32_t sig_kind = node->kind == CROSSTALK_HDL_NODE_NET_DECL || node->kind == CROSSTALK_HDL_NODE_PORT ? CROSSTALK_HDL_IR_SIG_NET : CROSSTALK_HDL_IR_SIG_VAR;
                uint64_t init_val = 0;
                uint8_t init_const = LDG_TRUTH_FALSE;

                if (init_idx != UINT32_MAX && sig_kind == CROSSTALK_HDL_IR_SIG_VAR && ctx->stmt_decl != LDG_TRUTH_TRUE)
                {
                    ret = crosstalk_hdl_elab_const_eval(ctx, init_idx, &init_val);
                    if (ret == LDG_ERR_AOK) { init_const = LDG_TRUTH_TRUE; }
                }

                ret = crosstalk_hdl_ir_sig_add(ctx->ir_ctx, &child->tok, width, 0x0, sig_kind, port_dir, init_val, &sig_idx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                ret = crosstalk_hdl_elab_sym_add(ctx, &child->tok, CROSSTALK_HDL_ELAB_SYM_SIG, sig_idx, 0x0, width);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                if (init_idx != UINT32_MAX && init_const != LDG_TRUTH_TRUE)
                {
                    if (ctx->decl_defer == LDG_TRUTH_TRUE)
                    {
                        if (LDG_UNLIKELY(ctx->defer_cunt >= CROSSTALK_HDL_ELAB_DEFER_CAP)) { return LDG_ERR_FULL; }

                        ctx->defer_sig[ctx->defer_cunt] = sig_idx;
                        ctx->defer_node[ctx->defer_cunt] = init_idx;
                        ctx->defer_width[ctx->defer_cunt] = width;
                        ctx->defer_cunt = ctx->defer_cunt + 0x1;
                    }
                    else if (ctx->stmt_decl == LDG_TRUTH_TRUE)
                    {
                        crosstalk_hdl_elab_lv_t init_lv = LDG_STRUCT_ZERO_INIT;
                        uint32_t val_sig = 0;
                        uint32_t val_w = 0;
                        uint8_t val_sgn = 0;

                        ctx->cw = width;
                        ret = crosstalk_hdl_elab_expr(ctx, init_idx, &val_sig, &val_w, &val_sgn);
                        ctx->cw = 0x0;
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                        ret = crosstalk_hdl_elab_width_fit(ctx, val_sig, val_w, LDG_TRUTH_FALSE, width, &val_sig);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                        init_lv.kind = CROSSTALK_HDL_ELAB_LV_SIG;
                        init_lv.sig = sig_idx;
                        init_lv.width = width;

                        ret = crosstalk_hdl_elab_assign_apply(ctx, &init_lv, val_sig, width);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
                    }
                    else
                    {
                        crosstalk_hdl_ir_op_t op = LDG_STRUCT_ZERO_INIT;
                        uint32_t val_sig = 0;
                        uint32_t val_w = 0;
                        uint8_t val_sgn = 0;

                        ctx->cw = width;
                        ret = crosstalk_hdl_elab_expr(ctx, init_idx, &val_sig, &val_w, &val_sgn);
                        ctx->cw = 0x0;
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                        ret = crosstalk_hdl_elab_width_fit(ctx, val_sig, val_w, LDG_TRUTH_FALSE, width, &val_sig);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                        op.kind = CROSSTALK_HDL_IR_OP_COPY;
                        op.dst = sig_idx;
                        op.src0 = val_sig;
                        op.src1 = UINT32_MAX;
                        op.src2 = UINT32_MAX;
                        op.width = width;

                        ret = crosstalk_hdl_elab_op_emit(ctx, &op);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
                    }
                }
            }
        }

        child_idx = child->next_sibling;
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_elab_mod_find(crosstalk_hdl_elab_ctx_t *ctx, const crosstalk_hdl_tok_t *name, uint32_t *out_node)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0 || out_node == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_node = UINT32_MAX;

    for (idx = 0; idx < ctx->mod_cunt; idx++) { if (crosstalk_hdl_elab_tok_eq_is(ctx->src, &ctx->mods[idx].name, name) == LDG_TRUTH_TRUE)
        {
            *out_node = ctx->mods[idx].node;
            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t crosstalk_hdl_elab_inst(crosstalk_hdl_elab_ctx_t *ctx, uint32_t node_idx)
{
    const crosstalk_hdl_ast_node_t *node = 0x0;
    const crosstalk_hdl_ast_node_t *child = 0x0;
    const crosstalk_hdl_ast_node_t *conn_node = 0x0;
    crosstalk_hdl_elab_conn_t conns[CROSSTALK_HDL_ELAB_CONN_CAP];
    crosstalk_hdl_elab_pbind_t pbinds[CROSSTALK_HDL_ELAB_PARAM_CAP];
    uint32_t conn_cunt = 0;
    uint32_t pbind_cunt = 0;
    uint32_t mod_node = 0;
    uint32_t child_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_ast_node_get(ctx->ast, node_idx, &node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_elab_mod_find(ctx, &node->tok, &mod_node);
    if (ret != LDG_ERR_AOK)
    {
        if (ctx->unknown_mod_cunt < (uint32_t)CROSSTALK_HDL_ELAB_UNKNOWN_MOD_CAP)
        {
            ctx->unknown_mod[ctx->unknown_mod_cunt] = node->tok;
            ctx->unknown_mod_cunt = ctx->unknown_mod_cunt + 0x1;
        }

        ctx->warn_cunt++;

        return LDG_ERR_AOK;
    }

    child_idx = node->first_child;
    while (child_idx != UINT32_MAX)
    {
        ret = crosstalk_hdl_ast_node_get(ctx->ast, child_idx, &child);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (child->kind == CROSSTALK_HDL_NODE_PARAM_CONN)
        {
            if (LDG_UNLIKELY(pbind_cunt >= CROSSTALK_HDL_ELAB_PARAM_CAP)) { return LDG_ERR_FULL; }

            const crosstalk_hdl_ast_node_t *pval_node = 0x0;

            pbinds[pbind_cunt].name = child->tok;
            pbinds[pbind_cunt].str_is = LDG_TRUTH_FALSE;

            ret = crosstalk_hdl_ast_node_get(ctx->ast, child->first_child, &pval_node);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (pval_node->tok.kind == CROSSTALK_HDL_TOK_LIT_STR)
            {
                pbinds[pbind_cunt].val = ((uint64_t)pval_node->tok.off << 32) | (uint64_t)pval_node->tok.len;
                pbinds[pbind_cunt].str_is = LDG_TRUTH_TRUE;
            }
            else
            {
                uint32_t str_sym = 0;

                if (pval_node->kind == CROSSTALK_HDL_NODE_EXPR_IDENT && crosstalk_hdl_elab_sym_find(ctx, &pval_node->tok, &str_sym) == LDG_ERR_AOK && ctx->syms[str_sym].kind == CROSSTALK_HDL_ELAB_SYM_STR)
                {
                    pbinds[pbind_cunt].val = ctx->syms[str_sym].val;
                    pbinds[pbind_cunt].str_is = LDG_TRUTH_TRUE;
                }
                else
                {
                    ret = crosstalk_hdl_elab_const_eval(ctx, child->first_child, &pbinds[pbind_cunt].val);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
                }
            }

            pbind_cunt++;
        }
        else if (child->kind == CROSSTALK_HDL_NODE_INST_ITEM)
        {
            uint32_t conn_idx = child->first_child;

            ctx->inst_name = child->tok;

            while (conn_idx != UINT32_MAX)
            {
                ret = crosstalk_hdl_ast_node_get(ctx->ast, conn_idx, &conn_node);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                if (conn_node->kind == CROSSTALK_HDL_NODE_PORT_CONN)
                {
                    if (LDG_UNLIKELY(conn_cunt >= CROSSTALK_HDL_ELAB_CONN_CAP)) { return LDG_ERR_FULL; }

                    conns[conn_cunt].name = conn_node->tok;
                    conns[conn_cunt].val_sig = UINT32_MAX;
                    conns[conn_cunt].val_w = 0;
                    conns[conn_cunt].pattern_cunt = 0;
                    conns[conn_cunt].lv.kind = CROSSTALK_HDL_ELAB_LV_NONE;
                    conns[conn_cunt].styp = UINT32_MAX;
                    conns[conn_cunt].base = UINT32_MAX;
                    conns[conn_cunt].mem_sig = UINT32_MAX;
                    conns[conn_cunt].mem_inner = 0;

                    if (conn_node->first_child != UINT32_MAX)
                    {
                        const crosstalk_hdl_ast_node_t *actual_node = 0x0;
                        uint32_t conn_svar = 0;
                        uint32_t conn_addr = 0;
                        uint32_t conn_mem = 0;
                        uint8_t val_sgn = 0;

                        ret = crosstalk_hdl_ast_node_get(ctx->ast, conn_node->first_child, &actual_node);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                        if (actual_node->kind == CROSSTALK_HDL_NODE_EXPR_IDENT && crosstalk_hdl_elab_sym_find(ctx, &actual_node->tok, &conn_mem) == LDG_ERR_AOK && ctx->syms[conn_mem].kind == CROSSTALK_HDL_ELAB_SYM_MEM)
                        {
                            conns[conn_cunt].mem_sig = ctx->syms[conn_mem].sig;
                            conns[conn_cunt].mem_inner = (uint32_t)ctx->syms[conn_mem].val;

                            conn_cunt++;
                            conn_idx = conn_node->next_sibling;
                            continue;
                        }

                        if (actual_node->kind == CROSSTALK_HDL_NODE_EXPR_PATTERN)
                        {
                            uint32_t elem_idx = actual_node->first_child;

                            while (elem_idx != UINT32_MAX)
                            {
                                const crosstalk_hdl_ast_node_t *elem_node = 0x0;

                                if (LDG_UNLIKELY(conns[conn_cunt].pattern_cunt >= (uint32_t)CROSSTALK_HDL_ELAB_PATTERN_CAP)) { ctx->err_tok = actual_node->tok; return LDG_ERR_FULL; }

                                ret = crosstalk_hdl_elab_expr(ctx, elem_idx, &conns[conn_cunt].pattern_sig[conns[conn_cunt].pattern_cunt], &conns[conn_cunt].pattern_w[conns[conn_cunt].pattern_cunt], &val_sgn);
                                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                                conns[conn_cunt].pattern_cunt = conns[conn_cunt].pattern_cunt + 0x1;

                                ret = crosstalk_hdl_ast_node_get(ctx->ast, elem_idx, &elem_node);
                                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                                elem_idx = elem_node->next_sibling;
                            }

                            conn_cunt++;
                            conn_idx = conn_node->next_sibling;
                            continue;
                        }

                        if (crosstalk_hdl_elab_svar_probe(ctx, conn_node->first_child, &conn_svar, &conn_addr) == LDG_ERR_AOK && conn_addr == UINT32_MAX)
                        {
                            conns[conn_cunt].styp = (uint32_t)ctx->syms[conn_svar].val;
                            conns[conn_cunt].base = ctx->syms[conn_svar].sig;
                        }
                        else
                        {
                            ret = crosstalk_hdl_elab_expr(ctx, conn_node->first_child, &conns[conn_cunt].val_sig, &conns[conn_cunt].val_w, &val_sgn);
                            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                            ret = crosstalk_hdl_elab_lv_resolve(ctx, conn_node->first_child, &conns[conn_cunt].lv);
                            if (ret != LDG_ERR_AOK) { conns[conn_cunt].lv.kind = CROSSTALK_HDL_ELAB_LV_NONE; }
                        }
                    }

                    conn_cunt++;
                }

                conn_idx = conn_node->next_sibling;
            }
        }

        child_idx = child->next_sibling;
    }

    ctx->inst_module = node->tok;
    ctx->inst_pending = LDG_TRUTH_TRUE;

    {
        crosstalk_hdl_tok_t path_name = ctx->inst_name;
        char path[CROSSTALK_HDL_ELAB_QUAL_PATH_CAP];
        char gen_suffix[CROSSTALK_HDL_ELAB_GEN_SUFFIX_CAP] = LDG_ARR_ZERO_INIT;
        uint32_t sig_mark = ctx->ir_ctx->sig_cunt;
        uint32_t gen_len = 0;
        uint32_t gen_idx = 0;
        uint32_t sub_idx = 0;

        for (gen_idx = 0; gen_idx < ctx->gen_depth; gen_idx = gen_idx + 0x1)
        {
            char digits[CROSSTALK_HDL_ELAB_GEN_IDX_CAP] = LDG_ARR_ZERO_INIT;
            uint32_t val = ctx->gen_at[gen_idx];
            uint32_t digit_cunt = 0;
            uint32_t at = 0;

            do
            {
                digits[digit_cunt] = (char)(LDG_ASCII_DIGIT_ZERO + (val % (uint32_t)LDG_BASE_DEC));
                digit_cunt = digit_cunt + 0x1;
                val = val / (uint32_t)LDG_BASE_DEC;
            }
            while (val != 0 && digit_cunt < (uint32_t)CROSSTALK_HDL_ELAB_GEN_IDX_CAP);

            if (gen_len + digit_cunt + CROSSTALK_HDL_ELAB_GEN_BRACKET_SIZE > (uint32_t)CROSSTALK_HDL_ELAB_GEN_SUFFIX_CAP) { break; }

            gen_suffix[gen_len] = CROSSTALK_HDL_ELAB_GEN_OPEN;
            gen_len = gen_len + 0x1;

            for (at = 0; at < digit_cunt; at = at + 0x1) { gen_suffix[gen_len + at] = digits[digit_cunt - 0x1 - at]; }

            gen_len = gen_len + digit_cunt;
            gen_suffix[gen_len] = CROSSTALK_HDL_ELAB_GEN_CLOSE;
            gen_len = gen_len + 0x1;
        }

        ret = crosstalk_hdl_elab_module(ctx, mod_node, pbinds, pbind_cunt, conns, conn_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        for (sub_idx = sig_mark; sub_idx < ctx->ir_ctx->sig_cunt; sub_idx = sub_idx + 0x1)
        {
            crosstalk_hdl_ir_sig_t *sub_sig = &ctx->ir_ctx->sigs[sub_idx];
            const char *tail = 0x0;
            uint32_t tail_len = 0;
            uint32_t path_len = 0;
            uint32_t ch_idx = 0;

            if (sub_sig->port != CROSSTALK_HDL_IR_PORT_NONE) { sub_sig->port = CROSSTALK_HDL_IR_PORT_NONE; }

            if (path_name.len == 0) { continue; }

            if (sub_sig->qual_len != 0 && sub_sig->qual_off != UINT32_MAX)
            {
                tail = ctx->ir_ctx->qual_text + sub_sig->qual_off;
                tail_len = sub_sig->qual_len;
            }
            else if (sub_sig->name.len != 0)
            {
                tail = ctx->src + sub_sig->name.off;
                tail_len = sub_sig->name.len;
            }
            else { continue; }

            path_len = path_name.len + gen_len + CROSSTALK_HDL_ELAB_QUAL_SEP_SIZE + tail_len;
            if (path_len > (uint32_t)CROSSTALK_HDL_ELAB_QUAL_PATH_CAP) { ctx->warn_cunt++; continue; }

            for (ch_idx = 0; ch_idx < path_name.len; ch_idx = ch_idx + 0x1) { path[ch_idx] = ctx->src[path_name.off + ch_idx]; }

            for (ch_idx = 0; ch_idx < gen_len; ch_idx = ch_idx + 0x1) { path[path_name.len + ch_idx] = gen_suffix[ch_idx]; }

            path[path_name.len + gen_len] = CROSSTALK_HDL_ELAB_QUAL_SEP;

            for (ch_idx = 0; ch_idx < tail_len; ch_idx = ch_idx + 0x1) { path[path_name.len + gen_len + CROSSTALK_HDL_ELAB_QUAL_SEP_SIZE + ch_idx] = tail[ch_idx]; }

            if (crosstalk_hdl_ir_sig_qual_set(ctx->ir_ctx, sub_idx, path, path_len) != LDG_ERR_AOK) { ctx->warn_cunt++; }
        }
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_elab_enum_reg(crosstalk_hdl_elab_ctx_t *ctx, uint32_t enum_idx)
{
    const crosstalk_hdl_ast_node_t *enum_node = 0x0;
    const crosstalk_hdl_ast_node_t *item = 0x0;
    uint64_t item_val = 0;
    uint32_t item_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_ast_node_get(ctx->ast, enum_idx, &enum_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    item_idx = enum_node->first_child;
    while (item_idx != UINT32_MAX)
    {
        ret = crosstalk_hdl_ast_node_get(ctx->ast, item_idx, &item);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (item->kind == CROSSTALK_HDL_NODE_ENUM_ITEM)
        {
            if (item->first_child != UINT32_MAX)
            {
                ret = crosstalk_hdl_elab_const_eval(ctx, item->first_child, &item_val);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            ret = crosstalk_hdl_elab_sym_add(ctx, &item->tok, CROSSTALK_HDL_ELAB_SYM_PARAM, UINT32_MAX, item_val, 32);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            item_val = item_val + 0x1;
        }

        item_idx = item->next_sibling;
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_elab_typedef_reg(crosstalk_hdl_elab_ctx_t *ctx, uint32_t td_idx)
{
    const crosstalk_hdl_ast_node_t *td_node = 0x0;
    const crosstalk_hdl_ast_node_t *sub_node = 0x0;
    uint32_t sub_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_ast_node_get(ctx->ast, td_idx, &td_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    sub_idx = td_node->first_child;
    while (sub_idx != UINT32_MAX)
    {
        uint32_t td_width = 0;

        ret = crosstalk_hdl_ast_node_get(ctx->ast, sub_idx, &sub_node);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (sub_node->kind == CROSSTALK_HDL_NODE_STRUCT)
        {
            const crosstalk_hdl_ast_node_t *member_node = 0x0;
            const crosstalk_hdl_ast_node_t *part_node = 0x0;
            crosstalk_hdl_tok_t group_name[16];
            uint32_t group_styp[16];
            uint32_t group_leaf_base[16];
            uint32_t group_leaf_cunt[16];
            uint32_t group_local_cunt = 0;
            uint32_t member_idx = sub_node->first_child;
            uint32_t styp_idx = ctx->styp_cunt;
            uint32_t total = 0;
            uint32_t running = 0;
            uint32_t fld_idx = 0;

            if (LDG_UNLIKELY(styp_idx >= CROSSTALK_HDL_ELAB_STYP_CAP)) { return LDG_ERR_FULL; }

            ctx->styps[styp_idx].name = td_node->tok;
            ctx->styps[styp_idx].field_base = ctx->sfield_cunt;
            ctx->styps[styp_idx].field_cunt = 0;

            while (member_idx != UINT32_MAX)
            {
                const crosstalk_hdl_ast_node_t *type_node = 0x0;
                uint32_t type_idx = UINT32_MAX;
                uint32_t part_idx = 0;
                uint32_t member_width = 0;
                uint32_t member_styp = UINT32_MAX;

                ret = crosstalk_hdl_ast_node_get(ctx->ast, member_idx, &member_node);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                part_idx = member_node->first_child;
                while (part_idx != UINT32_MAX)
                {
                    ret = crosstalk_hdl_ast_node_get(ctx->ast, part_idx, &part_node);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    if (part_node->kind == CROSSTALK_HDL_NODE_TYPE && type_idx == UINT32_MAX) { type_idx = part_idx; }

                    part_idx = part_node->next_sibling;
                }

                if (type_idx != UINT32_MAX)
                {
                    ret = crosstalk_hdl_ast_node_get(ctx->ast, type_idx, &type_node);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    if (type_node->tok.kind == CROSSTALK_HDL_TOK_IDENT)
                    {
                        uint32_t type_sym = 0;

                        ret = crosstalk_hdl_elab_sym_find(ctx, &type_node->tok, &type_sym);
                        if (ret == LDG_ERR_AOK && ctx->syms[type_sym].kind == CROSSTALK_HDL_ELAB_SYM_STYP) { member_styp = (uint32_t)ctx->syms[type_sym].val; }
                    }
                }

                if (member_styp == UINT32_MAX)
                {
                    ret = crosstalk_hdl_elab_type_width_get(ctx, type_idx, &member_width);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
                }

                part_idx = member_node->first_child;
                while (part_idx != UINT32_MAX)
                {
                    ret = crosstalk_hdl_ast_node_get(ctx->ast, part_idx, &part_node);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    if (part_node->kind == CROSSTALK_HDL_NODE_DECL_NAME && member_styp != UINT32_MAX)
                    {
                        const crosstalk_hdl_elab_styp_t *sub_styp = &ctx->styps[member_styp];
                        uint32_t leaf_k = 0;

                        if (LDG_UNLIKELY(group_local_cunt >= 16)) { return LDG_ERR_FULL; }

                        group_name[group_local_cunt] = part_node->tok;
                        group_styp[group_local_cunt] = member_styp;
                        group_leaf_base[group_local_cunt] = ctx->styps[styp_idx].field_cunt;
                        group_leaf_cunt[group_local_cunt] = sub_styp->field_cunt;
                        group_local_cunt = group_local_cunt + 0x1;

                        for (leaf_k = 0; leaf_k < sub_styp->field_cunt; leaf_k++)
                        {
                            const crosstalk_hdl_elab_sfield_t *leaf = &ctx->sfields[sub_styp->field_base + leaf_k];

                            if (LDG_UNLIKELY(ctx->sfield_cunt >= CROSSTALK_HDL_ELAB_SFIELD_CAP)) { return LDG_ERR_FULL; }

                            ctx->sfields[ctx->sfield_cunt].name = leaf->name;
                            ctx->sfields[ctx->sfield_cunt].width = leaf->width;
                            ctx->sfields[ctx->sfield_cunt].off = 0;
                            ctx->sfields[ctx->sfield_cunt].sub_styp = UINT32_MAX;
                            ctx->sfield_cunt = ctx->sfield_cunt + 0x1;
                            ctx->styps[styp_idx].field_cunt = ctx->styps[styp_idx].field_cunt + 0x1;
                            total = total + leaf->width;
                        }
                    }
                    else if (part_node->kind == CROSSTALK_HDL_NODE_DECL_NAME)
                    {
                        if (LDG_UNLIKELY(ctx->sfield_cunt >= CROSSTALK_HDL_ELAB_SFIELD_CAP)) { return LDG_ERR_FULL; }

                        ctx->sfields[ctx->sfield_cunt].name = part_node->tok;
                        ctx->sfields[ctx->sfield_cunt].width = member_width;
                        ctx->sfields[ctx->sfield_cunt].off = 0;
                        ctx->sfields[ctx->sfield_cunt].sub_styp = UINT32_MAX;
                        ctx->sfield_cunt = ctx->sfield_cunt + 0x1;
                        ctx->styps[styp_idx].field_cunt = ctx->styps[styp_idx].field_cunt + 0x1;
                        total = total + member_width;
                    }

                    part_idx = part_node->next_sibling;
                }

                member_idx = member_node->next_sibling;
            }

            running = total;
            for (fld_idx = 0; fld_idx < ctx->styps[styp_idx].field_cunt; fld_idx++)
            {
                crosstalk_hdl_elab_sfield_t *fld = &ctx->sfields[ctx->styps[styp_idx].field_base + fld_idx];

                running = running - fld->width;
                fld->off = running;
            }

            ctx->styps[styp_idx].group_base = ctx->sfield_cunt;
            ctx->styps[styp_idx].group_cunt = group_local_cunt;

            for (fld_idx = 0; fld_idx < group_local_cunt; fld_idx++)
            {
                if (LDG_UNLIKELY(ctx->sfield_cunt >= CROSSTALK_HDL_ELAB_SFIELD_CAP)) { return LDG_ERR_FULL; }

                ctx->sfields[ctx->sfield_cunt].name = group_name[fld_idx];
                ctx->sfields[ctx->sfield_cunt].width = 0;
                ctx->sfields[ctx->sfield_cunt].off = 0;
                ctx->sfields[ctx->sfield_cunt].sub_styp = group_styp[fld_idx];
                ctx->sfields[ctx->sfield_cunt].sub_leaf_base = group_leaf_base[fld_idx];
                ctx->sfields[ctx->sfield_cunt].sub_leaf_cunt = group_leaf_cunt[fld_idx];
                ctx->sfield_cunt = ctx->sfield_cunt + 0x1;
            }

            ctx->styps[styp_idx].total_width = total;
            ctx->styp_cunt = ctx->styp_cunt + 0x1;

            ret = crosstalk_hdl_elab_sym_add(ctx, &td_node->tok, CROSSTALK_HDL_ELAB_SYM_STYP, UINT32_MAX, (uint64_t)styp_idx, total);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            sub_idx = sub_node->next_sibling;
            continue;
        }

        ret = crosstalk_hdl_elab_type_width_get(ctx, sub_idx, &td_width);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (sub_node->kind == CROSSTALK_HDL_NODE_ENUM)
        {
            const crosstalk_hdl_ast_node_t *range_ck = 0x0;
            uint32_t range_idx = sub_node->first_child;
            uint8_t range_has = LDG_TRUTH_FALSE;

            ret = crosstalk_hdl_elab_enum_reg(ctx, sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            while (range_idx != UINT32_MAX)
            {
                ret = crosstalk_hdl_ast_node_get(ctx->ast, range_idx, &range_ck);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                if (range_ck->kind == CROSSTALK_HDL_NODE_RANGE) { range_has = LDG_TRUTH_TRUE; }

                range_idx = range_ck->next_sibling;
            }

            if (range_has == LDG_TRUTH_FALSE) { td_width = 32; }
        }

        ret = crosstalk_hdl_elab_sym_add(ctx, &td_node->tok, CROSSTALK_HDL_ELAB_SYM_TYPE, UINT32_MAX, (uint64_t)td_width, td_width);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        sub_idx = sub_node->next_sibling;
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_elab_item(crosstalk_hdl_elab_ctx_t *ctx, uint32_t node_idx)
{
    const crosstalk_hdl_ast_node_t *node = 0x0;
    const crosstalk_hdl_ast_node_t *child = 0x0;
    uint32_t sub_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_ast_node_get(ctx->ast, node_idx, &node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (node->tok.line != 0) { if (node->tok.line != 0) { ctx->err_tok = node->tok; } }

    if (node->kind == CROSSTALK_HDL_NODE_IMPORT) { return LDG_ERR_AOK; }

    if (node->kind == CROSSTALK_HDL_NODE_NET_DECL || node->kind == CROSSTALK_HDL_NODE_VAR_DECL || node->kind == CROSSTALK_HDL_NODE_GENVAR_DECL) { return crosstalk_hdl_elab_decl(ctx, node_idx, CROSSTALK_HDL_IR_PORT_NONE); }

    if (node->kind == CROSSTALK_HDL_NODE_PARAM_DECL || node->kind == CROSSTALK_HDL_NODE_LOCALPARAM_DECL)
    {
        sub_idx = node->first_child;
        while (sub_idx != UINT32_MAX)
        {
            ret = crosstalk_hdl_ast_node_get(ctx->ast, sub_idx, &child);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (child->kind == CROSSTALK_HDL_NODE_DECL_NAME)
            {
                uint64_t pval = 0;
                uint32_t exist_idx = 0;

                ret = crosstalk_hdl_elab_sym_find(ctx, &child->tok, &exist_idx);
                if (ret != LDG_ERR_AOK || (ctx->syms[exist_idx].kind != CROSSTALK_HDL_ELAB_SYM_PARAM && ctx->syms[exist_idx].kind != CROSSTALK_HDL_ELAB_SYM_STR))
                {
                    const crosstalk_hdl_ast_node_t *init_node = 0x0;

                    if (LDG_UNLIKELY(child->first_child == UINT32_MAX)) { ctx->err_tok = child->tok; return CROSSTALK_HDL_ERR_ELAB; }

                    ret = crosstalk_hdl_ast_node_get(ctx->ast, child->first_child, &init_node);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    if (init_node->tok.kind == CROSSTALK_HDL_TOK_LIT_STR)
                    {
                        ret = crosstalk_hdl_elab_sym_add(ctx, &child->tok, CROSSTALK_HDL_ELAB_SYM_STR, UINT32_MAX, ((uint64_t)init_node->tok.off << 32) | (uint64_t)init_node->tok.len, 0x0);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
                    }
                    else
                    {
                        ret = crosstalk_hdl_elab_const_eval(ctx, child->first_child, &pval);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                        ret = crosstalk_hdl_elab_sym_add(ctx, &child->tok, CROSSTALK_HDL_ELAB_SYM_PARAM, UINT32_MAX, pval, 32);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
                    }
                }
            }

            sub_idx = child->next_sibling;
        }

        return LDG_ERR_AOK;
    }

    if (node->kind == CROSSTALK_HDL_NODE_FUNC_DECL || node->kind == CROSSTALK_HDL_NODE_TASK_DECL)
    {
        uint32_t func_idx = 0;

        for (func_idx = 0; func_idx < ctx->func_cunt; func_idx++) { if (ctx->funcs[func_idx].node == node_idx) { return LDG_ERR_AOK; } }

        if (LDG_UNLIKELY(ctx->func_cunt >= CROSSTALK_HDL_ELAB_FUNC_CAP)) { return LDG_ERR_FULL; }

        ctx->funcs[ctx->func_cunt].name = node->tok;
        ctx->funcs[ctx->func_cunt].node = node_idx;
        ctx->func_cunt = ctx->func_cunt + 0x1;

        return LDG_ERR_AOK;
    }

    if (node->kind == CROSSTALK_HDL_NODE_TYPEDEF) { return crosstalk_hdl_elab_typedef_reg(ctx, node_idx); }

    if (node->kind == CROSSTALK_HDL_NODE_ENUM) { return crosstalk_hdl_elab_enum_reg(ctx, node_idx); }

    if (node->kind == CROSSTALK_HDL_NODE_PORT) { return crosstalk_hdl_elab_decl(ctx, node_idx, CROSSTALK_HDL_IR_PORT_NONE); }

    if (node->kind == CROSSTALK_HDL_NODE_CONT_ASSIGN)
    {
        crosstalk_hdl_elab_lv_t lv = LDG_STRUCT_ZERO_INIT;
        crosstalk_hdl_ir_op_t op = LDG_STRUCT_ZERO_INIT;
        uint32_t val_sig = 0;
        uint32_t val_w = 0;
        uint8_t val_sgn = 0;

        ret = crosstalk_hdl_ast_node_get(ctx->ast, node->first_child, &child);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (child->kind == CROSSTALK_HDL_NODE_EXPR_CONCAT)
        {
            ret = crosstalk_hdl_elab_expr(ctx, child->next_sibling, &val_sig, &val_w, &val_sgn);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            return crosstalk_hdl_elab_concat_assign(ctx, node->first_child, val_sig);
        }

        {
            uint32_t lhs_svar = 0;
            uint32_t lhs_addr = 0;

            if (crosstalk_hdl_elab_svar_probe(ctx, node->first_child, &lhs_svar, &lhs_addr) == LDG_ERR_AOK)
            {
                uint32_t lhs_styp = (uint32_t)ctx->syms[lhs_svar].val;
                uint32_t lhs_base = ctx->syms[lhs_svar].sig;
                uint32_t rhs_vals[CROSSTALK_HDL_ELAB_SFLD_MAX];

                ret = crosstalk_hdl_elab_struct_vals_get(ctx, child->next_sibling, lhs_styp, rhs_vals);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
                {
                    if (ctx->err_tok.line == 0) { if (node->tok.line != 0) { ctx->err_tok = node->tok; } }

                    return ret;
                }

                ret = crosstalk_hdl_elab_struct_assign(ctx, lhs_styp, lhs_base, lhs_addr, rhs_vals);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK) && ctx->err_tok.line == 0) { if (node->tok.line != 0) { ctx->err_tok = node->tok; } }

                if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_elab_commit(ctx); }

                return ret;
            }
        }

        ret = crosstalk_hdl_elab_lv_resolve(ctx, node->first_child, &lv);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ctx->cw = lv.width;
        ret = crosstalk_hdl_elab_expr(ctx, child->next_sibling, &val_sig, &val_w, &val_sgn);
        ctx->cw = 0x0;
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_elab_width_fit(ctx, val_sig, val_w, LDG_TRUTH_FALSE, lv.width, &val_sig);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (lv.kind == CROSSTALK_HDL_ELAB_LV_SIG)
        {
            op.kind = CROSSTALK_HDL_IR_OP_COPY;
            op.dst = lv.sig;
            op.src0 = val_sig;
            op.src1 = UINT32_MAX;
            op.src2 = UINT32_MAX;
            op.width = lv.width;

            return crosstalk_hdl_elab_op_emit(ctx, &op);
        }

        if (lv.kind == CROSSTALK_HDL_ELAB_LV_PART || lv.kind == CROSSTALK_HDL_ELAB_LV_PART_DYN)
        {
            op.kind = lv.kind == CROSSTALK_HDL_ELAB_LV_PART ? CROSSTALK_HDL_IR_OP_INSERT : CROSSTALK_HDL_IR_OP_INSERT_DYN;
            op.dst = lv.sig;
            op.src0 = lv.sig;
            op.src1 = val_sig;
            op.src2 = lv.off_sig;
            op.imm = lv.off;
            op.width = lv.width;

            return crosstalk_hdl_elab_op_emit(ctx, &op);
        }

        if (lv.kind == CROSSTALK_HDL_ELAB_LV_MEM)
        {
            op.kind = CROSSTALK_HDL_IR_OP_MEM_WR;
            op.dst = UINT32_MAX;
            op.src0 = lv.sig;
            op.src1 = lv.addr_sig;
            op.src2 = val_sig;
            op.width = lv.width;

            return crosstalk_hdl_elab_op_emit(ctx, &op);
        }

        return CROSSTALK_HDL_ERR_ELAB;
    }

    if (node->kind == CROSSTALK_HDL_NODE_ALWAYS || node->kind == CROSSTALK_HDL_NODE_ALWAYS_FF || node->kind == CROSSTALK_HDL_NODE_ALWAYS_COMB) { return crosstalk_hdl_elab_proc(ctx, node_idx); }

    if (node->kind == CROSSTALK_HDL_NODE_INITIAL) { return crosstalk_hdl_elab_initial(ctx, node_idx); }

    if (node->kind == CROSSTALK_HDL_NODE_FINAL) { return crosstalk_hdl_elab_final(ctx, node_idx); }

    if (node->kind == CROSSTALK_HDL_NODE_INST) { return crosstalk_hdl_elab_inst(ctx, node_idx); }

    if (node->kind == CROSSTALK_HDL_NODE_FOR) { return crosstalk_hdl_elab_for_stmt_unroll(ctx, node_idx, LDG_TRUTH_TRUE); }

    if (node->kind == CROSSTALK_HDL_NODE_GENERATE || node->kind == CROSSTALK_HDL_NODE_BLK)
    {
        sub_idx = node->first_child;
        while (sub_idx != UINT32_MAX)
        {
            ret = crosstalk_hdl_elab_item(ctx, sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_ast_node_get(ctx->ast, sub_idx, &child);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            sub_idx = child->next_sibling;
        }

        return LDG_ERR_AOK;
    }

    if (node->kind == CROSSTALK_HDL_NODE_IF)
    {
        const crosstalk_hdl_ast_node_t *then_node = 0x0;
        uint64_t cond_val = 0;
        uint32_t taken_idx = 0;

        ret = crosstalk_hdl_ast_node_get(ctx->ast, node->first_child, &child);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_elab_const_eval(ctx, node->first_child, &cond_val);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        taken_idx = child->next_sibling;

        if (cond_val == 0)
        {
            if (taken_idx == UINT32_MAX) { return LDG_ERR_AOK; }

            ret = crosstalk_hdl_ast_node_get(ctx->ast, taken_idx, &then_node);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            taken_idx = then_node->next_sibling;
        }

        if (taken_idx == UINT32_MAX) { return LDG_ERR_AOK; }

        return crosstalk_hdl_elab_item(ctx, taken_idx);
    }

    if (node->kind == CROSSTALK_HDL_NODE_CASE)
    {
        uint64_t subject_val = 0;
        uint32_t default_idx = UINT32_MAX;

        ret = crosstalk_hdl_ast_node_get(ctx->ast, node->first_child, &child);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_elab_const_eval(ctx, node->first_child, &subject_val);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        sub_idx = child->next_sibling;
        while (sub_idx != UINT32_MAX)
        {
            const crosstalk_hdl_ast_node_t *label_node = 0x0;
            uint32_t label_idx = 0;

            ret = crosstalk_hdl_ast_node_get(ctx->ast, sub_idx, &child);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (child->kind == CROSSTALK_HDL_NODE_CASE_DEFAULT) { default_idx = child->first_child; }
            else
            {
                uint32_t body_idx = UINT32_MAX;

                label_idx = child->first_child;
                while (label_idx != UINT32_MAX)
                {
                    ret = crosstalk_hdl_ast_node_get(ctx->ast, label_idx, &label_node);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    body_idx = label_idx;
                    label_idx = label_node->next_sibling;
                }

                label_idx = child->first_child;
                while (label_idx != body_idx && label_idx != UINT32_MAX)
                {
                    uint64_t label_val = 0;

                    ret = crosstalk_hdl_ast_node_get(ctx->ast, label_idx, &label_node);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    ret = crosstalk_hdl_elab_const_eval(ctx, label_idx, &label_val);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    if (label_val == subject_val) { return crosstalk_hdl_elab_item(ctx, body_idx); }

                    label_idx = label_node->next_sibling;
                }
            }

            sub_idx = child->next_sibling;
        }

        if (default_idx == UINT32_MAX) { return LDG_ERR_AOK; }

        return crosstalk_hdl_elab_item(ctx, default_idx);
    }

    if (node->kind == CROSSTALK_HDL_NODE_ATTR) { return LDG_ERR_AOK; }

    if (node->tok.line != 0) { if (node->tok.line != 0) { ctx->err_tok = node->tok; } }

    return CROSSTALK_HDL_ERR_ELAB;
}

static uint8_t crosstalk_hdl_elab_jack_name_is(const char *src, const crosstalk_hdl_tok_t *tok)
{
    const char prefix[] = CROSSTALK_HDL_IR_JACK_PREFIX;
    uint32_t prefix_len = (uint32_t)(sizeof(prefix) - 0x1);
    uint32_t at = 0;

    if (LDG_UNLIKELY(src == 0x0 || tok == 0x0)) { return LDG_TRUTH_FALSE; }

    if (tok->len <= prefix_len) { return LDG_TRUTH_FALSE; }

    for (at = 0; at < prefix_len; at++) { if (src[tok->off + at] != prefix[at]) { return LDG_TRUTH_FALSE; } }

    return LDG_TRUTH_TRUE;
}

static uint32_t crosstalk_hdl_elab_module(crosstalk_hdl_elab_ctx_t *ctx, uint32_t mod_node, const crosstalk_hdl_elab_pbind_t *pbinds, uint32_t pbind_cunt, crosstalk_hdl_elab_conn_t *conns, uint32_t conn_cunt)
{
    const crosstalk_hdl_ast_node_t *node = 0x0;
    const crosstalk_hdl_ast_node_t *child = 0x0;
    const crosstalk_hdl_ast_node_t *sub_node = 0x0;
    crosstalk_hdl_ir_op_t op = LDG_STRUCT_ZERO_INIT;
    uint32_t glue_formal[CROSSTALK_HDL_ELAB_CONN_CAP];
    uint32_t glue_conn[CROSSTALK_HDL_ELAB_CONN_CAP];
    uint32_t glue_cunt = 0;
    uint32_t sym_mark = 0;
    uint32_t child_idx = 0;
    uint32_t inst_idx = UINT32_MAX;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->depth >= CROSSTALK_HDL_ELAB_DEPTH_MAX)) { return CROSSTALK_HDL_ERR_ELAB; }

    ctx->depth++;
    sym_mark = ctx->sym_cunt;

    if (ctx->inst_pending == LDG_TRUTH_TRUE)
    {
        ctx->inst_pending = LDG_TRUTH_FALSE;

        ret = crosstalk_hdl_ir_inst_add(ctx->ir_ctx, &ctx->inst_name, &ctx->inst_module, &inst_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_FULL)) { ctx->depth--; return ret; }
    }

    ret = crosstalk_hdl_ast_node_get(ctx->ast, mod_node, &node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

    for (idx = 0; idx < pbind_cunt; idx++)
    {
        ret = crosstalk_hdl_elab_sym_add(ctx, &pbinds[idx].name, pbinds[idx].str_is == LDG_TRUTH_TRUE ? CROSSTALK_HDL_ELAB_SYM_STR : CROSSTALK_HDL_ELAB_SYM_PARAM, UINT32_MAX, pbinds[idx].val, pbinds[idx].str_is == LDG_TRUTH_TRUE ? 0x0 : 32);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }
    }

    child_idx = node->first_child;
    while (child_idx != UINT32_MAX)
    {
        ret = crosstalk_hdl_ast_node_get(ctx->ast, child_idx, &child);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

        if (child->kind == CROSSTALK_HDL_NODE_PARAM_DECL || child->kind == CROSSTALK_HDL_NODE_LOCALPARAM_DECL || child->kind == CROSSTALK_HDL_NODE_FUNC_DECL || child->kind == CROSSTALK_HDL_NODE_TASK_DECL || child->kind == CROSSTALK_HDL_NODE_TYPEDEF || child->kind == CROSSTALK_HDL_NODE_ENUM)
        {
            ret = crosstalk_hdl_elab_item(ctx, child_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }
        }

        child_idx = child->next_sibling;
    }

    child_idx = node->first_child;
    while (child_idx != UINT32_MAX)
    {
        ret = crosstalk_hdl_ast_node_get(ctx->ast, child_idx, &child);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

        if (child->kind != CROSSTALK_HDL_NODE_PORT)
        {
            child_idx = child->next_sibling;
            continue;
        }

        {
            uint32_t before_cunt = ctx->sym_cunt;

            ctx->port_conn = conns;
            ctx->port_conn_cunt = conn_cunt;

            ret = crosstalk_hdl_elab_decl(ctx, child_idx, CROSSTALK_HDL_IR_PORT_NONE);

            ctx->port_conn = 0x0;
            ctx->port_conn_cunt = 0;

            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

            for (idx = before_cunt; idx < ctx->sym_cunt; idx++)
            {
                uint32_t conn_idx = 0;
                uint32_t jack_sig = UINT32_MAX;

                if (ctx->syms[idx].kind == CROSSTALK_HDL_ELAB_SYM_PARAM) { continue; }

                for (conn_idx = 0; conn_idx < conn_cunt; conn_idx++)
                {
                    if (crosstalk_hdl_elab_tok_eq_is(ctx->src, &conns[conn_idx].name, &ctx->syms[idx].name) != LDG_TRUTH_TRUE) { continue; }

                    if (conns[conn_idx].mem_sig != UINT32_MAX)
                    {
                        if (LDG_UNLIKELY(ctx->syms[idx].kind != CROSSTALK_HDL_ELAB_SYM_MEM || ctx->syms[idx].sig != conns[conn_idx].mem_sig)) { ctx->err_tok = ctx->syms[idx].name; ctx->depth--; return CROSSTALK_HDL_ERR_ELAB; }

                        break;
                    }

                    if (ctx->syms[idx].kind == CROSSTALK_HDL_ELAB_SYM_SVAR)
                    {
                        if (LDG_UNLIKELY(conns[conn_idx].styp != (uint32_t)ctx->syms[idx].val)) { ctx->err_tok = ctx->syms[idx].name; ctx->depth--; return CROSSTALK_HDL_ERR_ELAB; }

                        if ((uint32_t)(ctx->syms[idx].val >> 32) > 0)
                        {
                            ctx->syms[idx].sig = conns[conn_idx].base;
                            break;
                        }

                        if (ctx->ir_ctx->sigs[ctx->syms[idx].sig].port == CROSSTALK_HDL_IR_PORT_IN)
                        {
                            const crosstalk_hdl_elab_styp_t *conn_styp = &ctx->styps[conns[conn_idx].styp];
                            uint32_t fld_idx = 0;

                            for (fld_idx = 0; fld_idx < conn_styp->field_cunt; fld_idx++)
                            {
                                op.kind = CROSSTALK_HDL_IR_OP_COPY;
                                op.dst = ctx->syms[idx].sig + fld_idx;
                                op.src0 = conns[conn_idx].base + fld_idx;
                                op.src1 = UINT32_MAX;
                                op.src2 = UINT32_MAX;
                                op.imm = 0;
                                op.width = ctx->sfields[conn_styp->field_base + fld_idx].width;

                                ret = crosstalk_hdl_elab_op_emit(ctx, &op);
                                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }
                            }
                        }
                        else
                        {
                            if (LDG_UNLIKELY(glue_cunt >= CROSSTALK_HDL_ELAB_CONN_CAP)) { ctx->depth--; return LDG_ERR_FULL; }

                            glue_formal[glue_cunt] = idx;
                            glue_conn[glue_cunt] = conn_idx;
                            glue_cunt++;
                        }

                        break;
                    }

                    if (conns[conn_idx].pattern_cunt != 0)
                    {
                        uint32_t elem_w = ctx->ir_ctx->sigs[ctx->syms[idx].sig].width;
                        uint32_t pattern_at = 0;

                        if (LDG_UNLIKELY(ctx->ir_ctx->sigs[ctx->syms[idx].sig].port != CROSSTALK_HDL_IR_PORT_IN)) { ctx->err_tok = ctx->syms[idx].name; ctx->depth--; return CROSSTALK_HDL_ERR_ELAB; }

                        if (LDG_UNLIKELY(ctx->ir_ctx->sigs[ctx->syms[idx].sig].depth < conns[conn_idx].pattern_cunt)) { ctx->err_tok = ctx->syms[idx].name; ctx->depth--; return CROSSTALK_HDL_ERR_ELAB; }

                        for (pattern_at = 0; pattern_at < conns[conn_idx].pattern_cunt; pattern_at++)
                        {
                            uint32_t addr_sig = 0;
                            uint32_t fit_sig = 0;

                            ret = crosstalk_hdl_elab_const_sig(ctx, (uint64_t)pattern_at, 32, &addr_sig);
                            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

                            ret = crosstalk_hdl_elab_width_fit(ctx, conns[conn_idx].pattern_sig[pattern_at], conns[conn_idx].pattern_w[pattern_at], LDG_TRUTH_FALSE, elem_w, &fit_sig);
                            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

                            op.kind = CROSSTALK_HDL_IR_OP_MEM_WR;
                            op.dst = UINT32_MAX;
                            op.src0 = ctx->syms[idx].sig;
                            op.src1 = addr_sig;
                            op.src2 = fit_sig;
                            op.imm = 0;
                            op.width = elem_w;

                            ret = crosstalk_hdl_elab_op_emit(ctx, &op);
                            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }
                        }

                        break;
                    }

                    jack_sig = conns[conn_idx].val_sig;

                    if (conns[conn_idx].val_sig == UINT32_MAX) { break; }

                    if (ctx->ir_ctx->sigs[ctx->syms[idx].sig].port == CROSSTALK_HDL_IR_PORT_IN)
                    {
                        uint32_t fit_sig = 0;

                        ret = crosstalk_hdl_elab_width_fit(ctx, conns[conn_idx].val_sig, conns[conn_idx].val_w, LDG_TRUTH_FALSE, ctx->syms[idx].width, &fit_sig);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

                        op.kind = CROSSTALK_HDL_IR_OP_COPY;
                        op.dst = ctx->syms[idx].sig;
                        op.src0 = fit_sig;
                        op.src1 = UINT32_MAX;
                        op.src2 = UINT32_MAX;
                        op.imm = 0;
                        op.width = ctx->syms[idx].width;

                        ret = crosstalk_hdl_elab_op_emit(ctx, &op);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }
                    }
                    else
                    {
                        if (LDG_UNLIKELY(glue_cunt >= CROSSTALK_HDL_ELAB_CONN_CAP)) { ctx->depth--; return LDG_ERR_FULL; }

                        glue_formal[glue_cunt] = idx;
                        glue_conn[glue_cunt] = conn_idx;
                        glue_cunt++;
                    }

                    break;
                }

                if (inst_idx != UINT32_MAX && ctx->syms[idx].kind == CROSSTALK_HDL_ELAB_SYM_SIG && ctx->syms[idx].sig != UINT32_MAX && crosstalk_hdl_elab_jack_name_is(ctx->src, &ctx->syms[idx].name) == LDG_TRUTH_TRUE)
                {
                    ret = crosstalk_hdl_ir_jack_add(ctx->ir_ctx, &ctx->syms[idx].name, inst_idx, jack_sig, ctx->syms[idx].width, ctx->ir_ctx->sigs[ctx->syms[idx].sig].port);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_FULL)) { ctx->depth--; return ret; }
                }
            }
        }
        child_idx = child->next_sibling;
    }

    ctx->decl_defer = LDG_TRUTH_TRUE;
    ctx->defer_cunt = 0;

    child_idx = node->first_child;
    while (child_idx != UINT32_MAX)
    {
        ret = crosstalk_hdl_ast_node_get(ctx->ast, child_idx, &child);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; ctx->decl_defer = LDG_TRUTH_FALSE; return ret; }

        if (child->kind == CROSSTALK_HDL_NODE_NET_DECL || child->kind == CROSSTALK_HDL_NODE_VAR_DECL)
        {
            ret = crosstalk_hdl_elab_item(ctx, child_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; ctx->decl_defer = LDG_TRUTH_FALSE; return ret; }
        }

        child_idx = child->next_sibling;
    }

    ctx->decl_defer = LDG_TRUTH_FALSE;

    for (idx = 0; idx < ctx->defer_cunt; idx++)
    {
        uint32_t val_sig = 0;
        uint32_t val_w = 0;
        uint8_t val_sgn = 0;

        ctx->cw = ctx->defer_width[idx];
        ret = crosstalk_hdl_elab_expr(ctx, ctx->defer_node[idx], &val_sig, &val_w, &val_sgn);
        ctx->cw = 0x0;
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

        ret = crosstalk_hdl_elab_width_fit(ctx, val_sig, val_w, LDG_TRUTH_FALSE, ctx->defer_width[idx], &val_sig);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

        op.kind = CROSSTALK_HDL_IR_OP_COPY;
        op.dst = ctx->defer_sig[idx];
        op.src0 = val_sig;
        op.src1 = UINT32_MAX;
        op.src2 = UINT32_MAX;
        op.imm = 0;
        op.width = ctx->defer_width[idx];

        ret = crosstalk_hdl_elab_op_emit(ctx, &op);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }
    }

    ctx->defer_cunt = 0;

    child_idx = node->first_child;
    while (child_idx != UINT32_MAX)
    {
        ret = crosstalk_hdl_ast_node_get(ctx->ast, child_idx, &child);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

        if (child->kind != CROSSTALK_HDL_NODE_PORT && child->kind != CROSSTALK_HDL_NODE_TYPEDEF && child->kind != CROSSTALK_HDL_NODE_ENUM && child->kind != CROSSTALK_HDL_NODE_NET_DECL && child->kind != CROSSTALK_HDL_NODE_VAR_DECL)
        {
            ret = crosstalk_hdl_elab_item(ctx, child_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }
        }

        child_idx = child->next_sibling;
    }

    for (idx = 0; idx < glue_cunt; idx++)
    {
        const crosstalk_hdl_elab_conn_t *conn = &conns[glue_conn[idx]];
        const crosstalk_hdl_elab_sym_t *formal = &ctx->syms[glue_formal[idx]];

        if (formal->kind == CROSSTALK_HDL_ELAB_SYM_SVAR && conn->styp != UINT32_MAX)
        {
            const crosstalk_hdl_elab_styp_t *conn_styp = &ctx->styps[conn->styp];
            uint32_t fld_idx = 0;

            for (fld_idx = 0; fld_idx < conn_styp->field_cunt; fld_idx++)
            {
                op.kind = CROSSTALK_HDL_IR_OP_COPY;
                op.dst = conn->base + fld_idx;
                op.src0 = formal->sig + fld_idx;
                op.src1 = UINT32_MAX;
                op.src2 = UINT32_MAX;
                op.imm = 0;
                op.width = ctx->sfields[conn_styp->field_base + fld_idx].width;

                ret = crosstalk_hdl_elab_op_emit(ctx, &op);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }
            }

            continue;
        }

        if (conn->lv.kind == CROSSTALK_HDL_ELAB_LV_SIG)
        {
            uint32_t fit_sig = 0;

            ret = crosstalk_hdl_elab_width_fit(ctx, formal->sig, formal->width, LDG_TRUTH_FALSE, conn->lv.width, &fit_sig);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

            op.kind = CROSSTALK_HDL_IR_OP_COPY;
            op.dst = conn->lv.sig;
            op.src0 = fit_sig;
            op.src1 = UINT32_MAX;
            op.src2 = UINT32_MAX;
            op.imm = 0;
            op.width = conn->lv.width;

            ret = crosstalk_hdl_elab_op_emit(ctx, &op);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }
        }
        else if (conn->lv.kind == CROSSTALK_HDL_ELAB_LV_MEM)
        {
            op.kind = CROSSTALK_HDL_IR_OP_MEM_WR;
            op.dst = UINT32_MAX;
            op.src0 = conn->lv.sig;
            op.src1 = conn->lv.addr_sig;
            op.src2 = formal->sig;
            op.imm = 0;
            op.width = conn->lv.width;

            ret = crosstalk_hdl_elab_op_emit(ctx, &op);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }
        }
        else if (conn->lv.kind == CROSSTALK_HDL_ELAB_LV_PART)
        {
            uint32_t fit_sig = 0;

            ret = crosstalk_hdl_elab_width_fit(ctx, formal->sig, formal->width, LDG_TRUTH_FALSE, conn->lv.width, &fit_sig);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

            op.kind = CROSSTALK_HDL_IR_OP_INSERT;
            op.dst = conn->lv.sig;
            op.src0 = conn->lv.sig;
            op.src1 = fit_sig;
            op.src2 = UINT32_MAX;
            op.imm = conn->lv.off;
            op.width = conn->lv.width;

            ret = crosstalk_hdl_elab_op_emit(ctx, &op);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }
        }
        else if (conn->lv.kind != CROSSTALK_HDL_ELAB_LV_NONE) { ctx->depth--; return CROSSTALK_HDL_ERR_ELAB; }
    }

    ctx->sym_cunt = sym_mark;
    ctx->depth--;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_elab_run(crosstalk_hdl_elab_ctx_t *ctx, const char *src, const crosstalk_hdl_ast_t *ast, void *pool, crosstalk_hdl_ir_t *ir_ctx, const char *top_name, uint32_t top_len)
{
    const crosstalk_hdl_ast_node_t *root = 0x0;
    const crosstalk_hdl_ast_node_t *child = 0x0;
    void *raw = 0x0;
    uint64_t bytes = 0;
    uint32_t child_idx = 0;
    uint32_t top_node = UINT32_MAX;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || src == 0x0 || ast == 0x0 || pool == 0x0 || ir_ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->src = src;
    ctx->ast = ast;
    ctx->ir_ctx = ir_ctx;
    ctx->pool = pool;
    ctx->sym_cunt = 0;
    ctx->mod_cunt = 0;
    ctx->guard = UINT32_MAX;
    ctx->ret_sig = UINT32_MAX;
    ctx->ret_width = 0;
    ctx->seq_blk = UINT32_MAX;
    ctx->mode = CROSSTALK_HDL_ELAB_MODE_COMB;
    ctx->depth = 0;
    ctx->warn_cunt = 0;
    ctx->cw = 0;
    ctx->inst_pending = LDG_TRUTH_FALSE;

    ret = ldg_arith_uint64_mul((uint64_t)CROSSTALK_HDL_ELAB_SYM_CAP, (uint64_t)sizeof(crosstalk_hdl_elab_sym_t), &bytes);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_OVERFLOW; }

    ret = ldg_mem_pool_alloc((ldg_mem_pool_t *)pool, bytes, &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->syms = (crosstalk_hdl_elab_sym_t *)raw;

    ret = ldg_arith_uint64_mul((uint64_t)CROSSTALK_HDL_ELAB_MOD_CAP, (uint64_t)sizeof(crosstalk_hdl_elab_mod_t), &bytes);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_OVERFLOW; }

    ret = ldg_mem_pool_alloc((ldg_mem_pool_t *)pool, bytes, &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->mods = (crosstalk_hdl_elab_mod_t *)raw;

    ret = ldg_arith_uint64_mul((uint64_t)CROSSTALK_HDL_ELAB_FUNC_CAP, (uint64_t)sizeof(crosstalk_hdl_elab_func_t), &bytes);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_OVERFLOW; }

    ret = ldg_mem_pool_alloc((ldg_mem_pool_t *)pool, bytes, &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->funcs = (crosstalk_hdl_elab_func_t *)raw;
    ctx->func_cunt = 0;

    ret = ldg_arith_uint64_mul((uint64_t)CROSSTALK_HDL_ELAB_STYP_CAP, (uint64_t)sizeof(crosstalk_hdl_elab_styp_t), &bytes);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_OVERFLOW; }

    ret = ldg_mem_pool_alloc((ldg_mem_pool_t *)pool, bytes, &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->styps = (crosstalk_hdl_elab_styp_t *)raw;
    ctx->styp_cunt = 0;

    ret = ldg_arith_uint64_mul((uint64_t)CROSSTALK_HDL_ELAB_SFIELD_CAP, (uint64_t)sizeof(crosstalk_hdl_elab_sfield_t), &bytes);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_OVERFLOW; }

    ret = ldg_mem_pool_alloc((ldg_mem_pool_t *)pool, bytes, &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->sfields = (crosstalk_hdl_elab_sfield_t *)raw;
    ctx->sfield_cunt = 0;

    ret = ldg_arith_uint64_mul((uint64_t)ir_ctx->sig_cap, (uint64_t)sizeof(uint32_t), &bytes);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_OVERFLOW; }

    ret = ldg_mem_pool_alloc((ldg_mem_pool_t *)pool, bytes, &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->current_map = (uint32_t *)raw;
    ctx->current_cap = ir_ctx->sig_cap;

    ret = ldg_mem_pool_alloc((ldg_mem_pool_t *)pool, bytes, &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->next_map = (uint32_t *)raw;
    ctx->nb_is = LDG_TRUTH_FALSE;
    ctx->decl_defer = LDG_TRUTH_FALSE;
    ctx->defer_cunt = 0;
    ctx->ret_svar_sym = UINT32_MAX;
    ctx->call_styp = UINT32_MAX;
    ctx->call_base = UINT32_MAX;

    for (idx = 0; idx < ctx->current_cap; idx++)
    {
        ctx->current_map[idx] = UINT32_MAX;
        ctx->next_map[idx] = UINT32_MAX;
    }

    ret = crosstalk_hdl_ast_node_get(ast, ast->root, &root);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    child_idx = root->first_child;
    while (child_idx != UINT32_MAX)
    {
        ret = crosstalk_hdl_ast_node_get(ast, child_idx, &child);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (child->kind == CROSSTALK_HDL_NODE_MODULE)
        {
            if (LDG_UNLIKELY(ctx->mod_cunt >= CROSSTALK_HDL_ELAB_MOD_CAP)) { return LDG_ERR_FULL; }

            ctx->mods[ctx->mod_cunt].name = child->tok;
            ctx->mods[ctx->mod_cunt].node = child_idx;
            ctx->mod_cunt++;

            if (top_name != 0x0 && top_len == child->tok.len && memcmp(src + child->tok.off, top_name, top_len) == 0) { top_node = child_idx; }

            if (top_name == 0x0 && top_node == UINT32_MAX) { top_node = child_idx; }
        }
        else if (child->kind == CROSSTALK_HDL_NODE_TYPEDEF)
        {
            ret = crosstalk_hdl_elab_typedef_reg(ctx, child_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
        else if (child->kind == CROSSTALK_HDL_NODE_ENUM)
        {
            ret = crosstalk_hdl_elab_enum_reg(ctx, child_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
        else if (child->kind == CROSSTALK_HDL_NODE_PACKAGE)
        {
            const crosstalk_hdl_ast_node_t *pkg_item = 0x0;
            uint32_t pkg_idx = child->first_child;

            while (pkg_idx != UINT32_MAX)
            {
                ret = crosstalk_hdl_ast_node_get(ast, pkg_idx, &pkg_item);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                ret = crosstalk_hdl_elab_item(ctx, pkg_idx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                pkg_idx = pkg_item->next_sibling;
            }
        }

        child_idx = child->next_sibling;
    }

    if (LDG_UNLIKELY(top_node == UINT32_MAX)) { return LDG_ERR_NOT_FOUND; }

    return crosstalk_hdl_elab_module(ctx, top_node, 0x0, 0x0, 0x0, 0x0);
}
