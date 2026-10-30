#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "hdl/elab.h"

uint8_t crosstalk_hdl_elab_tok_lit_eq_is(const char *src, const crosstalk_hdl_tok_t *tok, const char *lit)
{
    uint64_t lit_len = 0;

    if (LDG_UNLIKELY(src == 0x0 || tok == 0x0 || lit == 0x0)) { return LDG_TRUTH_FALSE; }

    lit_len = strlen(lit);
    if ((uint64_t)tok->len != lit_len) { return LDG_TRUTH_FALSE; }

    if (memcmp(src + tok->off, lit, tok->len) != 0) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

uint8_t crosstalk_hdl_elab_tok_eq_is(const char *src, const crosstalk_hdl_tok_t *lhs, const crosstalk_hdl_tok_t *rhs)
{
    if (LDG_UNLIKELY(src == 0x0 || lhs == 0x0 || rhs == 0x0)) { return LDG_TRUTH_FALSE; }

    if (lhs->len != rhs->len) { return LDG_TRUTH_FALSE; }

    if (lhs->len == 0) { return LDG_TRUTH_FALSE; }

    if (memcmp(src + lhs->off, src + rhs->off, lhs->len) != 0) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static uint32_t crosstalk_hdl_elab_digit_val_get(char ch, uint32_t base, uint64_t *out_val)
{
    if (LDG_UNLIKELY(out_val == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_val = 0;

    if (ch == 'x' || ch == 'X' || ch == 'z' || ch == 'Z' || ch == '?') { return LDG_ERR_AOK; }

    if (ldg_char_digit_is(ch) == LDG_TRUTH_TRUE) { *out_val = (uint64_t)(uint8_t)ch - 0x30; }
    else if (ch >= 'a' && ch <= 'f') { *out_val = (uint64_t)(uint8_t)ch - 0x61 + 10; }
    else if (ch >= 'A' && ch <= 'F') { *out_val = (uint64_t)(uint8_t)ch - 0x41 + 10; }
    else { return CROSSTALK_HDL_ERR_ELAB; }

    if (*out_val >= (uint64_t)base) { return CROSSTALK_HDL_ERR_ELAB; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_elab_lit_decode(const char *src, const crosstalk_hdl_tok_t *tok, uint64_t *out_val, uint32_t *out_width)
{
    uint64_t val = 0;
    uint64_t digit = 0;
    uint64_t size = 0;
    uint32_t base = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;
    uint8_t sized = 0;
    char ch = 0;

    if (LDG_UNLIKELY(src == 0x0 || tok == 0x0 || out_val == 0x0 || out_width == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_val = 0;
    *out_width = 32;

    if (tok->kind == CROSSTALK_HDL_TOK_LIT_DEC)
    {
        for (idx = 0; idx < tok->len; idx++)
        {
            ch = src[tok->off + idx];
            if (ch == '_') { continue; }

            val = val * 10 + ((uint64_t)(uint8_t)ch - 0x30);
        }

        *out_val = val;

        return LDG_ERR_AOK;
    }

    if (tok->kind == CROSSTALK_HDL_TOK_LIT_UNBASED)
    {
        ch = src[tok->off + tok->len - 0x1];
        *out_width = 0x1;
        if ((uint8_t)ch == 0x31) { *out_val = 0x1; }

        return LDG_ERR_AOK;
    }

    if (tok->kind != CROSSTALK_HDL_TOK_LIT_BASED) { return CROSSTALK_HDL_ERR_ELAB; }

    idx = 0;
    while (idx < tok->len && src[tok->off + idx] != '\'')
    {
        ch = src[tok->off + idx];
        if (ch != '_')
        {
            size = size * 10 + ((uint64_t)(uint8_t)ch - 0x30);
            sized = LDG_TRUTH_TRUE;
        }

        idx++;
    }

    if (LDG_UNLIKELY(idx >= tok->len)) { return CROSSTALK_HDL_ERR_ELAB; }

    idx++;
    ch = src[tok->off + idx];
    if (ch == 's' || ch == 'S')
    {
        idx++;
        ch = src[tok->off + idx];
    }

    if (ch == 'b' || ch == 'B') { base = 2; }
    else if (ch == 'o' || ch == 'O') { base = 8; }
    else if (ch == 'd' || ch == 'D') { base = 10; }
    else if (ch == 'h' || ch == 'H') { base = 16; }
    else { return CROSSTALK_HDL_ERR_ELAB; }

    idx++;

    for (; idx < tok->len; idx++)
    {
        ch = src[tok->off + idx];
        if (ch == '_') { continue; }

        ret = crosstalk_hdl_elab_digit_val_get(ch, base, &digit);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        val = val * (uint64_t)base + digit;
    }

    if (sized == LDG_TRUTH_TRUE)
    {
        if (LDG_UNLIKELY(size == 0 || size > CROSSTALK_HDL_ELAB_WIDTH_MAX)) { return CROSSTALK_HDL_ERR_ELAB_WIDTH; }

        *out_width = (uint32_t)size;
        if (size < 64) { val = val & (((uint64_t)0x1 << size) - 0x1); }
    }

    *out_val = val;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_elab_lit_mask_decode(const char *src, const crosstalk_hdl_tok_t *tok, uint64_t *out_val, uint64_t *out_mask, uint32_t *out_width)
{
    uint64_t val = 0;
    uint64_t mask = 0;
    uint64_t digit = 0;
    uint64_t size = 0;
    uint32_t base = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;
    uint8_t sized = 0;
    char ch = 0;

    if (LDG_UNLIKELY(src == 0x0 || tok == 0x0 || out_val == 0x0 || out_mask == 0x0 || out_width == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_mask = ~(uint64_t)0x0;
    if (tok->kind != CROSSTALK_HDL_TOK_LIT_BASED) { return crosstalk_hdl_elab_lit_decode(src, tok, out_val, out_width); }

    *out_val = 0;
    *out_width = 32;

    idx = 0;
    while (idx < tok->len && src[tok->off + idx] != '\'')
    {
        ch = src[tok->off + idx];
        if (ch != '_')
        {
            size = size * 10 + ((uint64_t)(uint8_t)ch - 0x30);
            sized = LDG_TRUTH_TRUE;
        }

        idx++;
    }

    if (LDG_UNLIKELY(idx >= tok->len)) { return CROSSTALK_HDL_ERR_ELAB; }

    idx++;
    ch = src[tok->off + idx];
    if (ch == 's' || ch == 'S')
    {
        idx++;
        ch = src[tok->off + idx];
    }

    if (ch == 'b' || ch == 'B') { base = 2; }
    else if (ch == 'o' || ch == 'O') { base = 8; }
    else if (ch == 'd' || ch == 'D') { base = 10; }
    else if (ch == 'h' || ch == 'H') { base = 16; }
    else { return CROSSTALK_HDL_ERR_ELAB; }

    idx++;

    for (; idx < tok->len; idx++)
    {
        ch = src[tok->off + idx];
        if (ch == '_') { continue; }

        if (ch == '?' || ch == 'z' || ch == 'Z' || ch == 'x' || ch == 'X')
        {
            val = val * (uint64_t)base;
            mask = mask * (uint64_t)base;
            continue;
        }

        ret = crosstalk_hdl_elab_digit_val_get(ch, base, &digit);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        val = val * (uint64_t)base + digit;
        mask = mask * (uint64_t)base + ((uint64_t)base - 0x1);
    }

    if (sized == LDG_TRUTH_TRUE)
    {
        if (LDG_UNLIKELY(size == 0 || size > CROSSTALK_HDL_ELAB_WIDTH_MAX)) { return CROSSTALK_HDL_ERR_ELAB_WIDTH; }

        *out_width = (uint32_t)size;
        if (size < 64)
        {
            val = val & (((uint64_t)0x1 << size) - 0x1);
            mask = mask & (((uint64_t)0x1 << size) - 0x1);
        }
    }

    *out_val = val;
    *out_mask = mask;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_elab_sym_add(crosstalk_hdl_elab_ctx_t *ctx, const crosstalk_hdl_tok_t *name, uint32_t kind, uint32_t sig, uint64_t val, uint32_t width)
{
    crosstalk_hdl_elab_sym_t *sym = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->sym_cunt >= CROSSTALK_HDL_ELAB_SYM_CAP)) { return LDG_ERR_FULL; }

    sym = &ctx->syms[ctx->sym_cunt];
    sym->name = *name;
    sym->kind = kind;
    sym->sig = sig;
    sym->val = val;
    sym->width = width;
    ctx->sym_cunt++;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_elab_sym_find(crosstalk_hdl_elab_ctx_t *ctx, const crosstalk_hdl_tok_t *name, uint32_t *out_idx)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    idx = ctx->sym_cunt;
    while (idx > 0)
    {
        idx--;
        if (crosstalk_hdl_elab_tok_eq_is(ctx->src, &ctx->syms[idx].name, name) == LDG_TRUTH_TRUE)
        {
            *out_idx = idx;
            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_NOT_FOUND;
}

static uint64_t crosstalk_hdl_elab_clog2_get(uint64_t val)
{
    uint64_t res = 0;
    uint64_t edge = 0x1;

    while (edge < val)
    {
        edge = edge << 0x1;
        res++;
    }

    return res;
}

static uint32_t crosstalk_hdl_elab_cast_type_get(crosstalk_hdl_elab_ctx_t *ctx, const crosstalk_hdl_ast_node_t *type_node, uint32_t *out_width, uint8_t *out_sgn)
{
    uint32_t sym_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || type_node == 0x0 || out_width == 0x0 || out_sgn == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_width = CROSSTALK_HDL_ELAB_CAST_WIDTH_KEEP;
    *out_sgn = LDG_TRUTH_INDETERMINATE;

    if (type_node->kind == CROSSTALK_HDL_NODE_TYPE)
    {
        if (type_node->tok.val == CROSSTALK_HDL_KW_SIGNED) { *out_sgn = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

        if (type_node->tok.val == CROSSTALK_HDL_KW_UNSIGNED) { *out_sgn = LDG_TRUTH_FALSE; return LDG_ERR_AOK; }

        if (type_node->tok.val == CROSSTALK_HDL_KW_INT || type_node->tok.val == CROSSTALK_HDL_KW_INTEGER)
        {
            *out_width = CROSSTALK_HDL_ELAB_INT_WIDTH;
            *out_sgn = LDG_TRUTH_TRUE;
            return LDG_ERR_AOK;
        }

        if (type_node->tok.val == CROSSTALK_HDL_KW_LOGIC)
        {
            *out_width = CROSSTALK_HDL_ELAB_LOGIC_WIDTH;
            *out_sgn = LDG_TRUTH_FALSE;
            return LDG_ERR_AOK;
        }

        if (type_node->tok.line != 0) { ctx->err_tok = type_node->tok; }

        return CROSSTALK_HDL_ERR_ELAB;
    }

    if (type_node->kind != CROSSTALK_HDL_NODE_EXPR_IDENT) { return LDG_ERR_NOT_FOUND; }

    ret = crosstalk_hdl_elab_sym_find(ctx, &type_node->tok, &sym_idx);
    if (ret != LDG_ERR_AOK) { return LDG_ERR_NOT_FOUND; }

    if (ctx->syms[sym_idx].kind != CROSSTALK_HDL_ELAB_SYM_TYPE) { return LDG_ERR_NOT_FOUND; }

    *out_width = ctx->syms[sym_idx].width;
    *out_sgn = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_elab_const_eval(crosstalk_hdl_elab_ctx_t *ctx, uint32_t node_idx, uint64_t *out_val)
{
    const crosstalk_hdl_ast_node_t *node = 0x0;
    const crosstalk_hdl_ast_node_t *fn_node = 0x0;
    uint64_t lhs = 0;
    uint64_t rhs = 0;
    uint32_t width = 0;
    uint32_t sym_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_val == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_val = 0;

    ret = crosstalk_hdl_ast_node_get(ctx->ast, node_idx, &node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_LIT) { return crosstalk_hdl_elab_lit_decode(ctx->src, &node->tok, out_val, &width); }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_IDENT)
    {
        ret = crosstalk_hdl_elab_sym_find(ctx, &node->tok, &sym_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            if (node->tok.line != 0) { ctx->err_tok = node->tok; }

            return CROSSTALK_HDL_ERR_ELAB;
        }

        if (LDG_UNLIKELY(ctx->syms[sym_idx].kind != CROSSTALK_HDL_ELAB_SYM_PARAM))
        {
            if (node->tok.line != 0) { ctx->err_tok = node->tok; }

            return CROSSTALK_HDL_ERR_ELAB;
        }

        *out_val = ctx->syms[sym_idx].val;

        return LDG_ERR_AOK;
    }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_UN)
    {
        ret = crosstalk_hdl_elab_const_eval(ctx, node->first_child, &lhs);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (node->tok.val == CROSSTALK_HDL_OP_MINUS) { *out_val = ~lhs + 0x1; }
        else if (node->tok.val == CROSSTALK_HDL_OP_PLUS) { *out_val = lhs; }
        else if (node->tok.val == CROSSTALK_HDL_OP_TILDE) { *out_val = ~lhs; }
        else if (node->tok.val == CROSSTALK_HDL_OP_BANG) { *out_val = lhs == 0 ? 0x1 : 0x0; }
        else
        {
            if (node->tok.line != 0) { ctx->err_tok = node->tok; }

            return CROSSTALK_HDL_ERR_ELAB;
        }

        return LDG_ERR_AOK;
    }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_PART || node->kind == CROSSTALK_HDL_NODE_EXPR_IDX)
    {
        const crosstalk_hdl_ast_node_t *sel_base = 0x0;
        uint64_t msb_val = 0;
        uint64_t lsb_val = 0;

        ret = crosstalk_hdl_elab_const_eval(ctx, node->first_child, &lhs);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_node_get(ctx->ast, node->first_child, &sel_base);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_elab_const_eval(ctx, sel_base->next_sibling, &msb_val);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (node->kind == CROSSTALK_HDL_NODE_EXPR_IDX)
        {
            *out_val = msb_val >= 64 ? 0x0 : (lhs >> msb_val) & 0x1;

            return LDG_ERR_AOK;
        }

        ret = crosstalk_hdl_ast_node_get(ctx->ast, sel_base->next_sibling, &sel_base);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_elab_const_eval(ctx, sel_base->next_sibling, &lsb_val);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(msb_val < lsb_val || msb_val - lsb_val >= 63)) { ctx->err_tok = node->tok; return CROSSTALK_HDL_ERR_ELAB; }

        *out_val = lsb_val >= 64 ? 0x0 : (lhs >> lsb_val) & (((uint64_t)0x1 << (msb_val - lsb_val + 0x1)) - 0x1);

        return LDG_ERR_AOK;
    }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_CAST)
    {
        const crosstalk_hdl_ast_node_t *cast_w_node = 0x0;
        uint8_t cast_sgn = 0;

        ret = crosstalk_hdl_ast_node_get(ctx->ast, node->first_child, &cast_w_node);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_elab_cast_type_get(ctx, cast_w_node, &width, &cast_sgn);
        if (ret == LDG_ERR_AOK) { lhs = width; }
        else if (ret == LDG_ERR_NOT_FOUND)
        {
            ret = crosstalk_hdl_elab_const_eval(ctx, node->first_child, &lhs);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
        else { return ret; }

        ret = crosstalk_hdl_elab_const_eval(ctx, cast_w_node->next_sibling, &rhs);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (lhs == 0 || lhs >= 64) { *out_val = rhs; }
        else { *out_val = rhs & (((uint64_t)0x1 << lhs) - 0x1); }

        return LDG_ERR_AOK;
    }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_COND)
    {
        const crosstalk_hdl_ast_node_t *cond_node = 0x0;
        uint32_t true_idx = 0;

        ret = crosstalk_hdl_elab_const_eval(ctx, node->first_child, &lhs);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_node_get(ctx->ast, node->first_child, &cond_node);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        true_idx = cond_node->next_sibling;
        ret = crosstalk_hdl_ast_node_get(ctx->ast, true_idx, &cond_node);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (lhs != 0) { return crosstalk_hdl_elab_const_eval(ctx, true_idx, out_val); }

        return crosstalk_hdl_elab_const_eval(ctx, cond_node->next_sibling, out_val);
    }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_CALL)
    {
        ret = crosstalk_hdl_ast_node_get(ctx->ast, node->first_child, &fn_node);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (crosstalk_hdl_elab_tok_lit_eq_is(ctx->src, &fn_node->tok, CROSSTALK_HDL_ELAB_PLUSARGS_TEST_NAME) == LDG_TRUTH_TRUE || crosstalk_hdl_elab_tok_lit_eq_is(ctx->src, &fn_node->tok, CROSSTALK_HDL_ELAB_PLUSARGS_VAL_NAME) == LDG_TRUTH_TRUE) { *out_val = CROSSTALK_HDL_ELAB_PLUSARGS_ABSENT; return LDG_ERR_AOK; }

        ret = crosstalk_hdl_elab_const_eval(ctx, fn_node->next_sibling, &lhs);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (crosstalk_hdl_elab_tok_lit_eq_is(ctx->src, &fn_node->tok, "$clog2") == LDG_TRUTH_TRUE) { *out_val = crosstalk_hdl_elab_clog2_get(lhs); return LDG_ERR_AOK; }

        if (crosstalk_hdl_elab_tok_lit_eq_is(ctx->src, &fn_node->tok, "$bits") == LDG_TRUTH_TRUE) { *out_val = lhs; return LDG_ERR_AOK; }

        ctx->err_tok = fn_node->tok;

        return CROSSTALK_HDL_ERR_ELAB;
    }

    if (node->kind != CROSSTALK_HDL_NODE_EXPR_BIN)
    {
        if (node->tok.line != 0) { ctx->err_tok = node->tok; }

        return CROSSTALK_HDL_ERR_ELAB;
    }

    ret = crosstalk_hdl_elab_const_eval(ctx, node->first_child, &lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_ast_node_get(ctx->ast, node->first_child, &fn_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_elab_const_eval(ctx, fn_node->next_sibling, &rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (node->tok.val == CROSSTALK_HDL_OP_PLUS) { *out_val = lhs + rhs; }
    else if (node->tok.val == CROSSTALK_HDL_OP_MINUS) { *out_val = lhs - rhs; }
    else if (node->tok.val == CROSSTALK_HDL_OP_STAR) { *out_val = lhs * rhs; }
    else if (node->tok.val == CROSSTALK_HDL_OP_SLASH)
    {
        if (LDG_UNLIKELY(rhs == 0))
        {
            if (node->tok.line != 0) { ctx->err_tok = node->tok; }

            return CROSSTALK_HDL_ERR_ELAB;
        }

        *out_val = lhs / rhs;
    }
    else if (node->tok.val == CROSSTALK_HDL_OP_PERCENT)
    {
        if (LDG_UNLIKELY(rhs == 0))
        {
            if (node->tok.line != 0) { ctx->err_tok = node->tok; }

            return CROSSTALK_HDL_ERR_ELAB;
        }

        *out_val = lhs % rhs;
    }
    else if (node->tok.val == CROSSTALK_HDL_OP_SHL) { *out_val = rhs >= 64 ? 0x0 : lhs << rhs; }
    else if (node->tok.val == CROSSTALK_HDL_OP_SHR) { *out_val = rhs >= 64 ? 0x0 : lhs >> rhs; }
    else if (node->tok.val == CROSSTALK_HDL_OP_AMP) { *out_val = lhs & rhs; }
    else if (node->tok.val == CROSSTALK_HDL_OP_PIPE) { *out_val = lhs | rhs; }
    else if (node->tok.val == CROSSTALK_HDL_OP_CARET) { *out_val = lhs ^ rhs; }
    else if (node->tok.val == CROSSTALK_HDL_OP_EQ) { *out_val = lhs == rhs ? 0x1 : 0x0; }
    else if (node->tok.val == CROSSTALK_HDL_OP_NEQ) { *out_val = lhs != rhs ? 0x1 : 0x0; }
    else if (node->tok.val == CROSSTALK_HDL_OP_LT) { *out_val = lhs < rhs ? 0x1 : 0x0; }
    else if (node->tok.val == CROSSTALK_HDL_OP_LE) { *out_val = lhs <= rhs ? 0x1 : 0x0; }
    else if (node->tok.val == CROSSTALK_HDL_OP_GT) { *out_val = lhs > rhs ? 0x1 : 0x0; }
    else if (node->tok.val == CROSSTALK_HDL_OP_GE) { *out_val = lhs >= rhs ? 0x1 : 0x0; }
    else if (node->tok.val == CROSSTALK_HDL_OP_LAND) { *out_val = lhs != 0 && rhs != 0 ? 0x1 : 0x0; }
    else if (node->tok.val == CROSSTALK_HDL_OP_LOR) { *out_val = lhs != 0 || rhs != 0 ? 0x1 : 0x0; }
    else
    {
        if (node->tok.line != 0) { ctx->err_tok = node->tok; }

        return CROSSTALK_HDL_ERR_ELAB;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_elab_tmp_make(crosstalk_hdl_elab_ctx_t *ctx, uint32_t width, uint32_t *out_sig)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_sig == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_hdl_ir_sig_add(ctx->ir_ctx, 0x0, width, 0x0, CROSSTALK_HDL_IR_SIG_VAR, CROSSTALK_HDL_IR_PORT_NONE, 0x0, out_sig);
}

uint32_t crosstalk_hdl_elab_op_emit(crosstalk_hdl_elab_ctx_t *ctx, const crosstalk_hdl_ir_op_t *op)
{
    crosstalk_hdl_ir_op_t ranged = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(ctx == 0x0 || op == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY((op->kind == CROSSTALK_HDL_IR_OP_SLICE || op->kind == CROSSTALK_HDL_IR_OP_INSERT) && op->imm >= 64))
    {
        uint8_t narrow_is = op->src0 != UINT32_MAX && ctx->ir_ctx->sigs[op->src0].width <= 64 && op->dst != UINT32_MAX && ctx->ir_ctx->sigs[op->dst].width <= 64 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

        if (narrow_is == LDG_TRUTH_TRUE && op->kind == CROSSTALK_HDL_IR_OP_SLICE)
        {
            ranged = *op;
            ranged.kind = CROSSTALK_HDL_IR_OP_CONST;
            ranged.imm = 0;
            ranged.src0 = UINT32_MAX;
            ranged.src1 = UINT32_MAX;
            ranged.src2 = UINT32_MAX;
            op = &ranged;
        }
        else if (narrow_is == LDG_TRUTH_TRUE)
        {
            ranged = *op;
            ranged.kind = CROSSTALK_HDL_IR_OP_COPY;
            ranged.imm = 0;
            ranged.src1 = UINT32_MAX;
            ranged.src2 = UINT32_MAX;
            op = &ranged;
        }
        else { ctx->warn_cunt = ctx->warn_cunt + 0x1; }
    }

    if (ctx->guard != UINT32_MAX)
    {
        if (op != &ranged) { ranged = *op; }

        ranged.guard = ctx->guard;
        ranged.guard_is = LDG_TRUTH_TRUE;
        op = &ranged;
    }

    if (ctx->mode == CROSSTALK_HDL_ELAB_MODE_SEQ) { return crosstalk_hdl_ir_seq_op_add(ctx->ir_ctx, ctx->seq_blk, op); }

    return crosstalk_hdl_ir_comb_op_add(ctx->ir_ctx, op);
}

uint32_t crosstalk_hdl_elab_const_sig(crosstalk_hdl_elab_ctx_t *ctx, uint64_t val, uint32_t width, uint32_t *out_sig)
{
    crosstalk_hdl_ir_op_t op = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_sig == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_elab_tmp_make(ctx, width, out_sig);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    op.kind = CROSSTALK_HDL_IR_OP_CONST;
    op.dst = *out_sig;
    op.src0 = UINT32_MAX;
    op.src1 = UINT32_MAX;
    op.src2 = UINT32_MAX;
    op.imm = val;
    op.width = width;

    return crosstalk_hdl_elab_op_emit(ctx, &op);
}

uint32_t crosstalk_hdl_elab_width_fit(crosstalk_hdl_elab_ctx_t *ctx, uint32_t val_sig, uint32_t val_width, uint8_t sgn, uint32_t want_width, uint32_t *out_sig)
{
    crosstalk_hdl_ir_op_t op = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_sig == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_sig = val_sig;

    if (val_width == want_width || want_width == 0) { return LDG_ERR_AOK; }

    ret = crosstalk_hdl_elab_tmp_make(ctx, want_width, out_sig);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    op.dst = *out_sig;
    op.src0 = val_sig;
    op.src1 = UINT32_MAX;
    op.src2 = UINT32_MAX;
    op.width = want_width;

    if (want_width > val_width)
    {
        op.kind = sgn == LDG_TRUTH_TRUE ? CROSSTALK_HDL_IR_OP_SEXT : CROSSTALK_HDL_IR_OP_ZEXT;
        op.imm = val_width;
    }
    else
    {
        op.kind = CROSSTALK_HDL_IR_OP_SLICE;
        op.imm = 0;
    }

    return crosstalk_hdl_elab_op_emit(ctx, &op);
}

static uint32_t crosstalk_hdl_elab_bin_map_get(uint32_t op_id, uint8_t sgn, uint32_t *out_kind, uint8_t *out_cmp)
{
    if (LDG_UNLIKELY(out_kind == 0x0 || out_cmp == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_kind = CROSSTALK_HDL_IR_OP_NONE;
    *out_cmp = LDG_TRUTH_FALSE;

    if (op_id == CROSSTALK_HDL_OP_PLUS) { *out_kind = CROSSTALK_HDL_IR_OP_ADD; }
    else if (op_id == CROSSTALK_HDL_OP_MINUS) { *out_kind = CROSSTALK_HDL_IR_OP_SUB; }
    else if (op_id == CROSSTALK_HDL_OP_STAR) { *out_kind = CROSSTALK_HDL_IR_OP_MUL; }
    else if (op_id == CROSSTALK_HDL_OP_SLASH) { *out_kind = CROSSTALK_HDL_IR_OP_DIV; }
    else if (op_id == CROSSTALK_HDL_OP_PERCENT) { *out_kind = CROSSTALK_HDL_IR_OP_MOD; }
    else if (op_id == CROSSTALK_HDL_OP_AMP) { *out_kind = CROSSTALK_HDL_IR_OP_AND; }
    else if (op_id == CROSSTALK_HDL_OP_PIPE) { *out_kind = CROSSTALK_HDL_IR_OP_OR; }
    else if (op_id == CROSSTALK_HDL_OP_CARET) { *out_kind = CROSSTALK_HDL_IR_OP_XOR; }
    else if (op_id == CROSSTALK_HDL_OP_XNOR) { *out_kind = CROSSTALK_HDL_IR_OP_XNOR; }
    else if (op_id == CROSSTALK_HDL_OP_SHL || op_id == CROSSTALK_HDL_OP_ASHL) { *out_kind = CROSSTALK_HDL_IR_OP_SHL; }
    else if (op_id == CROSSTALK_HDL_OP_SHR) { *out_kind = CROSSTALK_HDL_IR_OP_SHR; }
    else if (op_id == CROSSTALK_HDL_OP_ASHR) { *out_kind = sgn == LDG_TRUTH_TRUE ? CROSSTALK_HDL_IR_OP_ASHR : CROSSTALK_HDL_IR_OP_SHR; }
    else if (op_id == CROSSTALK_HDL_OP_EQ || op_id == CROSSTALK_HDL_OP_CASE_EQ) { *out_kind = CROSSTALK_HDL_IR_OP_EQ; *out_cmp = LDG_TRUTH_TRUE; }
    else if (op_id == CROSSTALK_HDL_OP_NEQ || op_id == CROSSTALK_HDL_OP_CASE_NEQ) { *out_kind = CROSSTALK_HDL_IR_OP_NEQ; *out_cmp = LDG_TRUTH_TRUE; }
    else if (op_id == CROSSTALK_HDL_OP_LT) { *out_kind = sgn == LDG_TRUTH_TRUE ? CROSSTALK_HDL_IR_OP_LT_S : CROSSTALK_HDL_IR_OP_LT; *out_cmp = LDG_TRUTH_TRUE; }
    else if (op_id == CROSSTALK_HDL_OP_LE) { *out_kind = sgn == LDG_TRUTH_TRUE ? CROSSTALK_HDL_IR_OP_LE_S : CROSSTALK_HDL_IR_OP_LE; *out_cmp = LDG_TRUTH_TRUE; }
    else if (op_id == CROSSTALK_HDL_OP_GT) { *out_kind = sgn == LDG_TRUTH_TRUE ? CROSSTALK_HDL_IR_OP_GT_S : CROSSTALK_HDL_IR_OP_GT; *out_cmp = LDG_TRUTH_TRUE; }
    else if (op_id == CROSSTALK_HDL_OP_GE) { *out_kind = sgn == LDG_TRUTH_TRUE ? CROSSTALK_HDL_IR_OP_GE_S : CROSSTALK_HDL_IR_OP_GE; *out_cmp = LDG_TRUTH_TRUE; }
    else if (op_id == CROSSTALK_HDL_OP_LAND) { *out_kind = CROSSTALK_HDL_IR_OP_LAND; *out_cmp = LDG_TRUTH_TRUE; }
    else if (op_id == CROSSTALK_HDL_OP_LOR) { *out_kind = CROSSTALK_HDL_IR_OP_LOR; *out_cmp = LDG_TRUTH_TRUE; }
    else { return CROSSTALK_HDL_ERR_ELAB; }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_elab_ident_rd(crosstalk_hdl_elab_ctx_t *ctx, const crosstalk_hdl_tok_t *name, uint32_t *out_sig, uint32_t *out_width, uint32_t *out_sym)
{
    uint32_t sym_idx = 0;
    uint32_t sig = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0 || out_sig == 0x0 || out_width == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_sig = UINT32_MAX;
    *out_width = 0;

    ret = crosstalk_hdl_elab_sym_find(ctx, name, &sym_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->err_tok = *name; return CROSSTALK_HDL_ERR_ELAB; }

    if (out_sym != 0x0) { *out_sym = sym_idx; }

    if (ctx->syms[sym_idx].kind == CROSSTALK_HDL_ELAB_SYM_PARAM)
    {
        *out_width = ctx->syms[sym_idx].width == 0 ? 32 : ctx->syms[sym_idx].width;

        return crosstalk_hdl_elab_const_sig(ctx, ctx->syms[sym_idx].val, *out_width, out_sig);
    }

    if (LDG_UNLIKELY(ctx->syms[sym_idx].kind == CROSSTALK_HDL_ELAB_SYM_SVAR || ctx->syms[sym_idx].kind == CROSSTALK_HDL_ELAB_SYM_STYP || ctx->syms[sym_idx].kind == CROSSTALK_HDL_ELAB_SYM_STR)) { ctx->err_tok = *name; return CROSSTALK_HDL_ERR_ELAB; }

    sig = ctx->syms[sym_idx].sig;
    *out_width = ctx->syms[sym_idx].width;

    if (ctx->syms[sym_idx].kind == CROSSTALK_HDL_ELAB_SYM_SIG && sig < ctx->current_cap && ctx->current_map[sig] != UINT32_MAX) { sig = ctx->current_map[sig]; }

    *out_sig = sig;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_elab_expr(crosstalk_hdl_elab_ctx_t *ctx, uint32_t node_idx, uint32_t *out_sig, uint32_t *out_width, uint8_t *out_sgn)
{
    const crosstalk_hdl_ast_node_t *node = 0x0;
    const crosstalk_hdl_ast_node_t *child = 0x0;
    crosstalk_hdl_ir_op_t op = LDG_STRUCT_ZERO_INIT;
    uint64_t const_val = 0;
    uint32_t lhs_sig = 0;
    uint32_t rhs_sig = 0;
    uint32_t lhs_w = 0;
    uint32_t rhs_w = 0;
    uint32_t res_w = 0;
    uint32_t sym_idx = 0;
    uint32_t ret = 0;
    uint32_t cw_save = 0;
    uint8_t lhs_sgn = 0;
    uint8_t rhs_sgn = 0;
    uint8_t cmp = 0;
    uint8_t shift_is = 0;
    uint8_t selfw_is = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_sig == 0x0 || out_width == 0x0 || out_sgn == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_sig = UINT32_MAX;
    *out_width = 0;
    *out_sgn = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx->depth >= CROSSTALK_HDL_ELAB_DEPTH_MAX)) { return CROSSTALK_HDL_ERR_ELAB; }

    ctx->depth++;

    ret = crosstalk_hdl_ast_node_get(ctx->ast, node_idx, &node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

    if (node->tok.line != 0) { if (node->tok.line != 0) { ctx->err_tok = node->tok; } }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_LIT)
    {
        ret = crosstalk_hdl_elab_lit_decode(ctx->src, &node->tok, &const_val, &res_w);
        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_elab_const_sig(ctx, const_val, res_w, out_sig); }

        *out_width = res_w;
        ctx->depth--;

        return ret;
    }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_IDENT)
    {
        if (ctx->src[node->tok.off] == '$')
        {
            if (node->tok.line != 0) { ctx->err_tok = node->tok; }

            ctx->depth--;
            return CROSSTALK_HDL_ERR_ELAB;
        }

        ret = crosstalk_hdl_elab_ident_rd(ctx, &node->tok, out_sig, out_width, &sym_idx);
        if (ret == LDG_ERR_AOK && ctx->syms[sym_idx].kind == CROSSTALK_HDL_ELAB_SYM_MEM)
        {
            if (node->tok.line != 0) { ctx->err_tok = node->tok; }

            ret = CROSSTALK_HDL_ERR_ELAB;
        }

        ctx->depth--;

        return ret;
    }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_SCOPE)
    {
        ret = crosstalk_hdl_elab_ident_rd(ctx, &node->tok, out_sig, out_width, &sym_idx);
        ctx->depth--;

        return ret;
    }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_CAST)
    {
        uint64_t cast_w = 0;
        uint32_t type_w = 0;
        uint8_t cast_sgn = 0;

        ret = crosstalk_hdl_ast_node_get(ctx->ast, node->first_child, &child);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

        ret = crosstalk_hdl_elab_cast_type_get(ctx, child, &type_w, &cast_sgn);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_NOT_FOUND)) { ctx->depth--; return ret; }

        if (ret == LDG_ERR_AOK && type_w == CROSSTALK_HDL_ELAB_CAST_WIDTH_KEEP)
        {
            ret = crosstalk_hdl_elab_expr(ctx, child->next_sibling, out_sig, out_width, out_sgn);
            *out_sgn = cast_sgn;
            ctx->depth--;

            return ret;
        }

        if (ret == LDG_ERR_AOK)
        {
            if (LDG_UNLIKELY(type_w > CROSSTALK_HDL_ELAB_WIDTH_MAX))
            {
                if (node->tok.line != 0) { ctx->err_tok = node->tok; }

                ctx->depth--;
                return CROSSTALK_HDL_ERR_ELAB;
            }

            cw_save = ctx->cw;
            ctx->cw = type_w;
            ret = crosstalk_hdl_elab_expr(ctx, child->next_sibling, &lhs_sig, &lhs_w, &lhs_sgn);
            ctx->cw = cw_save;
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

            ret = crosstalk_hdl_elab_width_fit(ctx, lhs_sig, lhs_w, lhs_sgn, type_w, out_sig);
            *out_width = type_w;
            *out_sgn = cast_sgn;
            ctx->depth--;

            return ret;
        }

        ret = crosstalk_hdl_elab_const_eval(ctx, node->first_child, &cast_w);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK || cast_w == 0 || cast_w > CROSSTALK_HDL_ELAB_WIDTH_MAX))
        {
            if (node->tok.line != 0) { ctx->err_tok = node->tok; }

            ctx->depth--;
            return CROSSTALK_HDL_ERR_ELAB;
        }

        cw_save = ctx->cw;
        ctx->cw = (uint32_t)cast_w;
        ret = crosstalk_hdl_elab_expr(ctx, child->next_sibling, &lhs_sig, &lhs_w, &lhs_sgn);
        ctx->cw = cw_save;
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

        ret = crosstalk_hdl_elab_width_fit(ctx, lhs_sig, lhs_w, lhs_sgn, (uint32_t)cast_w, out_sig);
        *out_width = (uint32_t)cast_w;
        ctx->depth--;

        return ret;
    }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_DOT)
    {
        uint32_t member_sig = 0;
        uint32_t member_w = 0;
        uint32_t member_addr = UINT32_MAX;

        ret = crosstalk_hdl_elab_member_resolve(ctx, node_idx, &member_sig, &member_w, &member_addr);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            if (node->tok.line != 0) { ctx->err_tok = node->tok; }

            ctx->depth--;
            return CROSSTALK_HDL_ERR_ELAB;
        }

        if (member_addr != UINT32_MAX)
        {
            ret = crosstalk_hdl_elab_tmp_make(ctx, member_w, out_sig);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

            op.kind = CROSSTALK_HDL_IR_OP_MEM_RD;
            op.dst = *out_sig;
            op.src0 = member_sig;
            op.src1 = member_addr;
            op.src2 = UINT32_MAX;
            op.imm = 0;
            op.width = member_w;

            ret = crosstalk_hdl_elab_op_emit(ctx, &op);
            *out_width = member_w;
            ctx->depth--;

            return ret;
        }

        if (member_sig < ctx->current_cap && ctx->current_map[member_sig] != UINT32_MAX) { *out_sig = ctx->current_map[member_sig]; }
        else { *out_sig = member_sig; }

        *out_width = member_w;
        ctx->depth--;

        return ret;
    }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_CALL)
    {
        ret = crosstalk_hdl_ast_node_get(ctx->ast, node->first_child, &child);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

        if (crosstalk_hdl_elab_tok_lit_eq_is(ctx->src, &child->tok, "$sig" "ned") == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_elab_expr(ctx, child->next_sibling, out_sig, out_width, out_sgn);
            *out_sgn = LDG_TRUTH_TRUE;
            ctx->depth--;

            return ret;
        }

        if (crosstalk_hdl_elab_tok_lit_eq_is(ctx->src, &child->tok, CROSSTALK_HDL_ELAB_PLUSARGS_TEST_NAME) == LDG_TRUTH_TRUE || crosstalk_hdl_elab_tok_lit_eq_is(ctx->src, &child->tok, CROSSTALK_HDL_ELAB_PLUSARGS_VAL_NAME) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_elab_const_sig(ctx, CROSSTALK_HDL_ELAB_PLUSARGS_ABSENT, CROSSTALK_HDL_ELAB_INT_WIDTH, out_sig);
            *out_width = CROSSTALK_HDL_ELAB_INT_WIDTH;
            *out_sgn = LDG_TRUTH_TRUE;
            ctx->depth--;

            return ret;
        }

        if (crosstalk_hdl_elab_tok_lit_eq_is(ctx->src, &child->tok, "$unsig" "ned") == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_elab_expr(ctx, child->next_sibling, out_sig, out_width, out_sgn);
            *out_sgn = LDG_TRUTH_FALSE;
            ctx->depth--;

            return ret;
        }

        if (crosstalk_hdl_elab_tok_lit_eq_is(ctx->src, &child->tok, "$clog2") == LDG_TRUTH_TRUE || crosstalk_hdl_elab_tok_lit_eq_is(ctx->src, &child->tok, "$bits") == LDG_TRUTH_TRUE)
        {
            uint64_t const_val = 0;

            ret = crosstalk_hdl_elab_const_eval(ctx, node_idx, &const_val);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

            ret = crosstalk_hdl_elab_const_sig(ctx, const_val, 32, out_sig);
            *out_width = 32;
            ctx->depth--;

            return ret;
        }

        if (ctx->src[child->tok.off] != '$')
        {
            cw_save = ctx->cw;
            ctx->cw = 0x0;
            ret = crosstalk_hdl_elab_func_call(ctx, node_idx, out_sig, out_width, out_sgn);
            ctx->cw = cw_save;
            ctx->depth--;

            return ret;
        }

        ctx->err_tok = child->tok;
        ctx->depth--;

        return CROSSTALK_HDL_ERR_ELAB;
    }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_UN)
    {
        selfw_is = node->tok.val == CROSSTALK_HDL_OP_TILDE || node->tok.val == CROSSTALK_HDL_OP_MINUS || node->tok.val == CROSSTALK_HDL_OP_PLUS ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE;
        cw_save = ctx->cw;

        if (selfw_is == LDG_TRUTH_TRUE) { ctx->cw = 0x0; }

        ret = crosstalk_hdl_elab_expr(ctx, node->first_child, &lhs_sig, &lhs_w, &lhs_sgn);
        ctx->cw = cw_save;
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

        res_w = lhs_w;
        if (selfw_is == LDG_TRUTH_FALSE && cw_save > res_w) { res_w = cw_save; }

        op.kind = CROSSTALK_HDL_IR_OP_NONE;

        if (node->tok.val == CROSSTALK_HDL_OP_TILDE) { op.kind = CROSSTALK_HDL_IR_OP_NOT; }
        else if (node->tok.val == CROSSTALK_HDL_OP_MINUS) { op.kind = CROSSTALK_HDL_IR_OP_NEG; }
        else if (node->tok.val == CROSSTALK_HDL_OP_PLUS) { *out_sig = lhs_sig; *out_width = lhs_w; *out_sgn = lhs_sgn; ctx->depth--; return LDG_ERR_AOK; }
        else if (node->tok.val == CROSSTALK_HDL_OP_BANG) { op.kind = CROSSTALK_HDL_IR_OP_LNOT; res_w = 0x1; }
        else if (node->tok.val == CROSSTALK_HDL_OP_AMP) { op.kind = CROSSTALK_HDL_IR_OP_RED_AND; res_w = 0x1; }
        else if (node->tok.val == CROSSTALK_HDL_OP_PIPE) { op.kind = CROSSTALK_HDL_IR_OP_RED_OR; res_w = 0x1; }
        else if (node->tok.val == CROSSTALK_HDL_OP_CARET) { op.kind = CROSSTALK_HDL_IR_OP_RED_XOR; res_w = 0x1; }
        else if (node->tok.val == CROSSTALK_HDL_OP_NAND) { op.kind = CROSSTALK_HDL_IR_OP_RED_AND; res_w = 0x1; }
        else if (node->tok.val == CROSSTALK_HDL_OP_NOR) { op.kind = CROSSTALK_HDL_IR_OP_RED_OR; res_w = 0x1; }
        else if (node->tok.val == CROSSTALK_HDL_OP_XNOR) { op.kind = CROSSTALK_HDL_IR_OP_RED_XOR; res_w = 0x1; }
        else
        {
            if (node->tok.line != 0) { ctx->err_tok = node->tok; }

            ctx->depth--;
            return CROSSTALK_HDL_ERR_ELAB;
        }

        if (res_w > lhs_w)
        {
            ret = crosstalk_hdl_elab_width_fit(ctx, lhs_sig, lhs_w, lhs_sgn, res_w, &lhs_sig);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }
        }

        ret = crosstalk_hdl_elab_tmp_make(ctx, res_w, out_sig);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

        op.dst = *out_sig;
        op.src0 = lhs_sig;
        op.src1 = UINT32_MAX;
        op.src2 = UINT32_MAX;
        op.width = res_w;

        ret = crosstalk_hdl_elab_op_emit(ctx, &op);
        *out_width = res_w;

        if (ret == LDG_ERR_AOK && (node->tok.val == CROSSTALK_HDL_OP_NAND || node->tok.val == CROSSTALK_HDL_OP_NOR || node->tok.val == CROSSTALK_HDL_OP_XNOR))
        {
            lhs_sig = *out_sig;
            ret = crosstalk_hdl_elab_tmp_make(ctx, 0x1, out_sig);
            if (ret == LDG_ERR_AOK)
            {
                op.kind = CROSSTALK_HDL_IR_OP_LNOT;
                op.dst = *out_sig;
                op.src0 = lhs_sig;
                op.width = 0x1;
                ret = crosstalk_hdl_elab_op_emit(ctx, &op);
            }
        }

        ctx->depth--;

        return ret;
    }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_COND)
    {
        uint32_t cond_sig = 0;
        uint32_t cond_w = 0;
        uint32_t true_idx = 0;
        uint32_t false_idx = 0;
        uint8_t cond_sgn = 0;

        cw_save = ctx->cw;
        ctx->cw = 0x0;

        ret = crosstalk_hdl_elab_expr(ctx, node->first_child, &cond_sig, &cond_w, &cond_sgn);
        ctx->cw = cw_save;
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

        ret = crosstalk_hdl_ast_node_get(ctx->ast, node->first_child, &child);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

        true_idx = child->next_sibling;
        ret = crosstalk_hdl_ast_node_get(ctx->ast, true_idx, &child);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

        false_idx = child->next_sibling;

        ctx->cw = cw_save;
        ret = crosstalk_hdl_elab_expr(ctx, true_idx, &lhs_sig, &lhs_w, &lhs_sgn);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->cw = cw_save; ctx->depth--; return ret; }

        ctx->cw = cw_save;
        ret = crosstalk_hdl_elab_expr(ctx, false_idx, &rhs_sig, &rhs_w, &rhs_sgn);
        ctx->cw = cw_save;
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

        res_w = lhs_w > rhs_w ? lhs_w : rhs_w;
        if (cw_save > res_w) { res_w = cw_save; }

        ret = crosstalk_hdl_elab_width_fit(ctx, lhs_sig, lhs_w, lhs_sgn, res_w, &lhs_sig);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

        ret = crosstalk_hdl_elab_width_fit(ctx, rhs_sig, rhs_w, rhs_sgn, res_w, &rhs_sig);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

        ret = crosstalk_hdl_elab_tmp_make(ctx, res_w, out_sig);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

        op.kind = CROSSTALK_HDL_IR_OP_MUX;
        op.dst = *out_sig;
        op.src0 = cond_sig;
        op.src1 = lhs_sig;
        op.src2 = rhs_sig;
        op.width = res_w;

        ret = crosstalk_hdl_elab_op_emit(ctx, &op);
        *out_width = res_w;
        ctx->depth--;

        return ret;
    }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_CONCAT || node->kind == CROSSTALK_HDL_NODE_EXPR_REPL)
    {
        uint32_t part_idx = node->first_child;
        uint32_t acc_sig = UINT32_MAX;
        uint32_t acc_w = 0;
        uint64_t repl_cunt = 0x1;
        uint32_t repl_idx = 0;
        uint32_t whole_sig = UINT32_MAX;

        if (node->kind == CROSSTALK_HDL_NODE_EXPR_REPL)
        {
            ret = crosstalk_hdl_elab_const_eval(ctx, node->first_child, &repl_cunt);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

            ret = crosstalk_hdl_ast_node_get(ctx->ast, node->first_child, &child);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

            part_idx = child->next_sibling;
        }

        cw_save = ctx->cw;

        while (part_idx != UINT32_MAX)
        {
            ctx->cw = 0x0;
            ret = crosstalk_hdl_elab_expr(ctx, part_idx, &rhs_sig, &rhs_w, &rhs_sgn);
            ctx->cw = cw_save;
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

            if (acc_sig == UINT32_MAX)
            {
                acc_sig = rhs_sig;
                acc_w = rhs_w;
            }
            else
            {
                if (LDG_UNLIKELY(acc_w + rhs_w > CROSSTALK_HDL_ELAB_WIDTH_MAX))
                {
                    if (node->tok.line != 0) { ctx->err_tok = node->tok; }

                    ctx->depth--;
                    return CROSSTALK_HDL_ERR_ELAB_WIDTH;
                }

                ret = crosstalk_hdl_elab_tmp_make(ctx, acc_w + rhs_w, &lhs_sig);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

                op.kind = CROSSTALK_HDL_IR_OP_CONCAT;
                op.dst = lhs_sig;
                op.src0 = acc_sig;
                op.src1 = rhs_sig;
                op.src2 = UINT32_MAX;
                op.imm = rhs_w;
                op.width = acc_w + rhs_w;

                ret = crosstalk_hdl_elab_op_emit(ctx, &op);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

                acc_sig = lhs_sig;
                acc_w = acc_w + rhs_w;
            }

            ret = crosstalk_hdl_ast_node_get(ctx->ast, part_idx, &child);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

            part_idx = child->next_sibling;
        }

        if (LDG_UNLIKELY(acc_sig == UINT32_MAX))
        {
            if (node->tok.line != 0) { ctx->err_tok = node->tok; }

            ctx->depth--;
            return CROSSTALK_HDL_ERR_ELAB;
        }

        whole_sig = acc_sig;

        for (repl_idx = 0x1; repl_idx < (uint32_t)repl_cunt; repl_idx++)
        {
            if (LDG_UNLIKELY(acc_w * (repl_idx + 0x1) > CROSSTALK_HDL_ELAB_WIDTH_MAX))
            {
                if (node->tok.line != 0) { ctx->err_tok = node->tok; }

                ctx->depth--;
                return CROSSTALK_HDL_ERR_ELAB_WIDTH;
            }

            ret = crosstalk_hdl_elab_tmp_make(ctx, acc_w * (repl_idx + 0x1), &lhs_sig);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

            op.kind = CROSSTALK_HDL_IR_OP_CONCAT;
            op.dst = lhs_sig;
            op.src0 = whole_sig;
            op.src1 = acc_sig;
            op.src2 = UINT32_MAX;
            op.imm = acc_w;
            op.width = acc_w * (repl_idx + 0x1);

            ret = crosstalk_hdl_elab_op_emit(ctx, &op);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

            whole_sig = lhs_sig;
        }

        *out_sig = whole_sig;
        *out_width = acc_w * (uint32_t)repl_cunt;
        ctx->depth--;

        return LDG_ERR_AOK;
    }

    if (node->kind == CROSSTALK_HDL_NODE_EXPR_IDX || node->kind == CROSSTALK_HDL_NODE_EXPR_PART || node->kind == CROSSTALK_HDL_NODE_EXPR_PART_PLUS || node->kind == CROSSTALK_HDL_NODE_EXPR_PART_MINUS)
    {
        const crosstalk_hdl_ast_node_t *base_node = 0x0;
        uint32_t base_idx = node->first_child;
        uint32_t addr_idx = 0;

        ret = crosstalk_hdl_ast_node_get(ctx->ast, base_idx, &base_node);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

        addr_idx = base_node->next_sibling;

        if (base_node->kind == CROSSTALK_HDL_NODE_EXPR_IDENT && node->kind == CROSSTALK_HDL_NODE_EXPR_IDX)
        {
            ret = crosstalk_hdl_elab_sym_find(ctx, &base_node->tok, &sym_idx);
            if (ret == LDG_ERR_AOK && ctx->syms[sym_idx].kind == CROSSTALK_HDL_ELAB_SYM_MEM)
            {
                if (LDG_UNLIKELY(ctx->syms[sym_idx].val != 0))
                {
                    if (base_node->tok.line != 0) { ctx->err_tok = base_node->tok; }

                    ctx->depth--;
                    return CROSSTALK_HDL_ERR_ELAB;
                }

                cw_save = ctx->cw;
                ctx->cw = 0x0;
                ret = crosstalk_hdl_elab_expr(ctx, addr_idx, &rhs_sig, &rhs_w, &rhs_sgn);
                ctx->cw = cw_save;
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

                res_w = ctx->syms[sym_idx].width;

                ret = crosstalk_hdl_elab_tmp_make(ctx, res_w, out_sig);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

                op.kind = CROSSTALK_HDL_IR_OP_MEM_RD;
                op.dst = *out_sig;
                op.src0 = ctx->syms[sym_idx].sig;
                op.src1 = rhs_sig;
                op.src2 = UINT32_MAX;
                op.width = res_w;

                ret = crosstalk_hdl_elab_op_emit(ctx, &op);
                *out_width = res_w;
                ctx->depth--;

                return ret;
            }
        }

        cw_save = ctx->cw;
        ctx->cw = 0x0;
        ret = crosstalk_hdl_elab_expr(ctx, base_idx, &lhs_sig, &lhs_w, &lhs_sgn);
        ctx->cw = cw_save;
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

        if (node->kind == CROSSTALK_HDL_NODE_EXPR_IDX)
        {
            uint64_t bit_off = 0;

            ret = crosstalk_hdl_elab_const_eval(ctx, addr_idx, &bit_off);
            if (ret == LDG_ERR_AOK)
            {
                ret = crosstalk_hdl_elab_tmp_make(ctx, 0x1, out_sig);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

                op.kind = CROSSTALK_HDL_IR_OP_SLICE;
                op.imm = bit_off;
            }
            else
            {
                ctx->cw = 0x0;
                ret = crosstalk_hdl_elab_expr(ctx, addr_idx, &rhs_sig, &rhs_w, &rhs_sgn);
                ctx->cw = cw_save;
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

                ret = crosstalk_hdl_elab_tmp_make(ctx, 0x1, out_sig);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

                op.kind = CROSSTALK_HDL_IR_OP_SLICE_DYN;
                op.src1 = rhs_sig;
            }

            op.dst = *out_sig;
            op.src0 = lhs_sig;
            op.src2 = UINT32_MAX;
            op.width = 0x1;

            ret = crosstalk_hdl_elab_op_emit(ctx, &op);
            *out_width = 0x1;
            ctx->depth--;

            return ret;
        }

        {
            const crosstalk_hdl_ast_node_t *msb_node = 0x0;
            uint64_t msb_val = 0;
            uint64_t lsb_val = 0;
            uint32_t lsb_idx = 0;

            ret = crosstalk_hdl_ast_node_get(ctx->ast, addr_idx, &msb_node);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

            lsb_idx = msb_node->next_sibling;

            if ((node->kind == CROSSTALK_HDL_NODE_EXPR_PART_PLUS || node->kind == CROSSTALK_HDL_NODE_EXPR_PART_MINUS) && crosstalk_hdl_elab_const_eval(ctx, addr_idx, &msb_val) != LDG_ERR_AOK)
            {
                uint32_t off_sig = 0;
                uint32_t off_w = 0;
                uint8_t off_sgn = 0;

                ret = crosstalk_hdl_elab_const_eval(ctx, lsb_idx, &lsb_val);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

                res_w = (uint32_t)lsb_val;

                ctx->cw = 0x0;
                ret = crosstalk_hdl_elab_expr(ctx, addr_idx, &off_sig, &off_w, &off_sgn);
                ctx->cw = cw_save;
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

                if (node->kind == CROSSTALK_HDL_NODE_EXPR_PART_MINUS && res_w > 0x1)
                {
                    uint32_t adj_const = 0;
                    uint32_t adj_sig = 0;

                    ret = crosstalk_hdl_elab_const_sig(ctx, (uint64_t)res_w - 0x1, off_w, &adj_const);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

                    ret = crosstalk_hdl_elab_tmp_make(ctx, off_w, &adj_sig);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

                    op.kind = CROSSTALK_HDL_IR_OP_SUB;
                    op.dst = adj_sig;
                    op.src0 = off_sig;
                    op.src1 = adj_const;
                    op.src2 = UINT32_MAX;
                    op.imm = 0;
                    op.width = off_w;

                    ret = crosstalk_hdl_elab_op_emit(ctx, &op);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

                    off_sig = adj_sig;
                }

                ret = crosstalk_hdl_elab_tmp_make(ctx, res_w, out_sig);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

                op.kind = CROSSTALK_HDL_IR_OP_SLICE_DYN;
                op.dst = *out_sig;
                op.src0 = lhs_sig;
                op.src1 = off_sig;
                op.src2 = UINT32_MAX;
                op.imm = 0;
                op.width = res_w;

                ret = crosstalk_hdl_elab_op_emit(ctx, &op);
                *out_width = res_w;
                ctx->depth--;

                return ret;
            }

            ret = crosstalk_hdl_elab_const_eval(ctx, addr_idx, &msb_val);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

            ret = crosstalk_hdl_elab_const_eval(ctx, lsb_idx, &lsb_val);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

            if (node->kind == CROSSTALK_HDL_NODE_EXPR_PART)
            {
                if (LDG_UNLIKELY(msb_val < lsb_val))
                {
                    if (node->tok.line != 0) { ctx->err_tok = node->tok; }

                    ctx->depth--;
                    return CROSSTALK_HDL_ERR_ELAB;
                }

                res_w = (uint32_t)(msb_val - lsb_val) + 0x1;
                op.imm = lsb_val;
            }
            else if (node->kind == CROSSTALK_HDL_NODE_EXPR_PART_PLUS)
            {
                res_w = (uint32_t)lsb_val;
                op.imm = msb_val;
            }
            else
            {
                res_w = (uint32_t)lsb_val;
                if (LDG_UNLIKELY(msb_val + 0x1 < (uint64_t)res_w))
                {
                    if (node->tok.line != 0) { ctx->err_tok = node->tok; }

                    ctx->depth--;
                    return CROSSTALK_HDL_ERR_ELAB;
                }

                op.imm = msb_val + 0x1 - (uint64_t)res_w;
            }

            ret = crosstalk_hdl_elab_tmp_make(ctx, res_w, out_sig);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

            op.kind = CROSSTALK_HDL_IR_OP_SLICE;
            op.dst = *out_sig;
            op.src0 = lhs_sig;
            op.src1 = UINT32_MAX;
            op.src2 = UINT32_MAX;
            op.width = res_w;

            ret = crosstalk_hdl_elab_op_emit(ctx, &op);
            *out_width = res_w;
            ctx->depth--;

            return ret;
        }
    }

    if (node->kind != CROSSTALK_HDL_NODE_EXPR_BIN)
    {
        if (node->tok.line != 0) { ctx->err_tok = node->tok; }

        ctx->depth--;
        return CROSSTALK_HDL_ERR_ELAB;
    }

    shift_is = node->tok.val == CROSSTALK_HDL_OP_SHL || node->tok.val == CROSSTALK_HDL_OP_ASHL || node->tok.val == CROSSTALK_HDL_OP_SHR || node->tok.val == CROSSTALK_HDL_OP_ASHR ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    selfw_is = node->tok.val == CROSSTALK_HDL_OP_EQ || node->tok.val == CROSSTALK_HDL_OP_NEQ || node->tok.val == CROSSTALK_HDL_OP_CASE_EQ || node->tok.val == CROSSTALK_HDL_OP_CASE_NEQ || node->tok.val == CROSSTALK_HDL_OP_LT || node->tok.val == CROSSTALK_HDL_OP_LE || node->tok.val == CROSSTALK_HDL_OP_GT || node->tok.val == CROSSTALK_HDL_OP_GE || node->tok.val == CROSSTALK_HDL_OP_LAND || node->tok.val == CROSSTALK_HDL_OP_LOR ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    cw_save = ctx->cw;

    ctx->cw = selfw_is == LDG_TRUTH_TRUE ? 0x0 : cw_save;
    ret = crosstalk_hdl_elab_expr(ctx, node->first_child, &lhs_sig, &lhs_w, &lhs_sgn);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->cw = cw_save; ctx->depth--; return ret; }

    ret = crosstalk_hdl_ast_node_get(ctx->ast, node->first_child, &child);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->cw = cw_save; ctx->depth--; return ret; }

    ctx->cw = selfw_is == LDG_TRUTH_TRUE || shift_is == LDG_TRUTH_TRUE ? 0x0 : cw_save;
    ret = crosstalk_hdl_elab_expr(ctx, child->next_sibling, &rhs_sig, &rhs_w, &rhs_sgn);
    ctx->cw = cw_save;
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

    ret = crosstalk_hdl_elab_bin_map_get(node->tok.val, lhs_sgn, &op.kind, &cmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (node->tok.line != 0) { ctx->err_tok = node->tok; }

        ctx->depth--;
        return ret;
    }

    if (op.kind == CROSSTALK_HDL_IR_OP_SHL || op.kind == CROSSTALK_HDL_IR_OP_SHR || op.kind == CROSSTALK_HDL_IR_OP_ASHR)
    {
        res_w = lhs_w;
        if (cw_save > res_w) { res_w = cw_save; }

        ret = crosstalk_hdl_elab_width_fit(ctx, lhs_sig, lhs_w, lhs_sgn, res_w, &lhs_sig);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }
    }
    else
    {
        res_w = lhs_w > rhs_w ? lhs_w : rhs_w;
        if (cmp != LDG_TRUTH_TRUE && cw_save > res_w) { res_w = cw_save; }

        ret = crosstalk_hdl_elab_width_fit(ctx, lhs_sig, lhs_w, lhs_sgn, res_w, &lhs_sig);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

        ret = crosstalk_hdl_elab_width_fit(ctx, rhs_sig, rhs_w, rhs_sgn, res_w, &rhs_sig);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }
    }

    if (cmp == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_elab_tmp_make(ctx, 0x1, out_sig);
        *out_width = 0x1;
        op.width = res_w;
    }
    else
    {
        ret = crosstalk_hdl_elab_tmp_make(ctx, res_w, out_sig);
        *out_width = res_w;
        op.width = res_w;
    }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->depth--; return ret; }

    op.dst = *out_sig;
    op.src0 = lhs_sig;
    op.src1 = rhs_sig;
    op.src2 = UINT32_MAX;

    ret = crosstalk_hdl_elab_op_emit(ctx, &op);
    ctx->depth--;

    return ret;
}
