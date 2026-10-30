#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#include "hdl/parse.h"

#define CROSSTALK_HDL_PARSE_PREC_NONE 0x0
#define CROSSTALK_HDL_PARSE_PREC_LOW 0x2

static uint32_t crosstalk_hdl_parse_expr(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx);
static uint32_t crosstalk_hdl_parse_postfix(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx);
static uint32_t crosstalk_hdl_parse_stmt(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx);
static uint32_t crosstalk_hdl_parse_module_item(crosstalk_hdl_parse_ctx_t *ctx, uint32_t parent_idx);
static uint32_t crosstalk_hdl_parse_type(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx);

static uint32_t crosstalk_hdl_parse_adv(crosstalk_hdl_parse_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->prev = ctx->cur;
    ctx->cur = ctx->nxt;

    return crosstalk_hdl_lex_next(&ctx->lex, &ctx->nxt);
}

static uint8_t crosstalk_hdl_parse_op_is(const crosstalk_hdl_parse_ctx_t *ctx, uint32_t op)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_TRUTH_FALSE; }

    if (ctx->cur.kind != CROSSTALK_HDL_TOK_OP) { return LDG_TRUTH_FALSE; }

    if (ctx->cur.val != op) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static uint8_t crosstalk_hdl_parse_kw_is(const crosstalk_hdl_parse_ctx_t *ctx, uint32_t kw)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_TRUTH_FALSE; }

    if (ctx->cur.kind != CROSSTALK_HDL_TOK_KEYWORD) { return LDG_TRUTH_FALSE; }

    if (ctx->cur.val != kw) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static uint8_t crosstalk_hdl_parse_next_op_is(const crosstalk_hdl_parse_ctx_t *ctx, uint32_t op)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_TRUTH_FALSE; }

    if (ctx->nxt.kind != CROSSTALK_HDL_TOK_OP) { return LDG_TRUTH_FALSE; }

    if (ctx->nxt.val != op) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static uint32_t crosstalk_hdl_parse_op_expect(crosstalk_hdl_parse_ctx_t *ctx, uint32_t op)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (crosstalk_hdl_parse_op_is(ctx, op) != LDG_TRUTH_TRUE) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

    return crosstalk_hdl_parse_adv(ctx);
}

static uint32_t crosstalk_hdl_parse_kw_expect(crosstalk_hdl_parse_ctx_t *ctx, uint32_t kw)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (crosstalk_hdl_parse_kw_is(ctx, kw) != LDG_TRUTH_TRUE) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

    return crosstalk_hdl_parse_adv(ctx);
}

static uint32_t crosstalk_hdl_parse_mk(crosstalk_hdl_parse_ctx_t *ctx, uint32_t kind, const crosstalk_hdl_tok_t *tok, uint32_t *out_idx)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_hdl_ast_node_add(ctx->ast, kind, tok, out_idx);
}

static uint32_t crosstalk_hdl_parse_depth_enter(crosstalk_hdl_parse_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->depth >= CROSSTALK_HDL_PARSE_DEPTH_MAX)) { return CROSSTALK_HDL_ERR_PARSE_DEPTH; }

    ctx->depth++;

    return LDG_ERR_AOK;
}

static void crosstalk_hdl_parse_depth_leave(crosstalk_hdl_parse_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (ctx->depth > 0) { ctx->depth--; }
}

static uint32_t crosstalk_hdl_parse_bin_prec_get(const crosstalk_hdl_parse_ctx_t *ctx)
{
    uint32_t op = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return CROSSTALK_HDL_PARSE_PREC_NONE; }

    if (ctx->cur.kind != CROSSTALK_HDL_TOK_OP) { return CROSSTALK_HDL_PARSE_PREC_NONE; }

    op = ctx->cur.val;

    if (op == CROSSTALK_HDL_OP_LOR) { return 2; }

    if (op == CROSSTALK_HDL_OP_LAND) { return 3; }

    if (op == CROSSTALK_HDL_OP_PIPE) { return 4; }

    if (op == CROSSTALK_HDL_OP_CARET || op == CROSSTALK_HDL_OP_XNOR) { return 5; }

    if (op == CROSSTALK_HDL_OP_AMP) { return 6; }

    if (op == CROSSTALK_HDL_OP_EQ || op == CROSSTALK_HDL_OP_NEQ || op == CROSSTALK_HDL_OP_CASE_EQ || op == CROSSTALK_HDL_OP_CASE_NEQ) { return 7; }

    if (op == CROSSTALK_HDL_OP_LT || op == CROSSTALK_HDL_OP_LE || op == CROSSTALK_HDL_OP_GT || op == CROSSTALK_HDL_OP_GE) { return 8; }

    if (op == CROSSTALK_HDL_OP_SHL || op == CROSSTALK_HDL_OP_SHR || op == CROSSTALK_HDL_OP_ASHL || op == CROSSTALK_HDL_OP_ASHR) { return 9; }

    if (op == CROSSTALK_HDL_OP_PLUS || op == CROSSTALK_HDL_OP_MINUS)
    {
        if (ctx->nxt.kind == CROSSTALK_HDL_TOK_OP && ctx->nxt.val == CROSSTALK_HDL_OP_COLON) { return CROSSTALK_HDL_PARSE_PREC_NONE; }

        return 10;
    }

    if (op == CROSSTALK_HDL_OP_STAR || op == CROSSTALK_HDL_OP_SLASH || op == CROSSTALK_HDL_OP_PERCENT) { return 11; }

    return CROSSTALK_HDL_PARSE_PREC_NONE;
}

static uint8_t crosstalk_hdl_parse_un_op_is(const crosstalk_hdl_parse_ctx_t *ctx)
{
    uint32_t op = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_TRUTH_FALSE; }

    if (ctx->cur.kind != CROSSTALK_HDL_TOK_OP) { return LDG_TRUTH_FALSE; }

    op = ctx->cur.val;

    if (op == CROSSTALK_HDL_OP_BANG || op == CROSSTALK_HDL_OP_TILDE || op == CROSSTALK_HDL_OP_PLUS || op == CROSSTALK_HDL_OP_MINUS) { return LDG_TRUTH_TRUE; }

    if (op == CROSSTALK_HDL_OP_AMP || op == CROSSTALK_HDL_OP_PIPE || op == CROSSTALK_HDL_OP_CARET) { return LDG_TRUTH_TRUE; }

    if (op == CROSSTALK_HDL_OP_NAND || op == CROSSTALK_HDL_OP_NOR || op == CROSSTALK_HDL_OP_XNOR) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t crosstalk_hdl_parse_call_args(crosstalk_hdl_parse_ctx_t *ctx, uint32_t call_idx)
{
    uint32_t arg_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_LPAREN);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_RPAREN) == LDG_TRUTH_TRUE) { return crosstalk_hdl_parse_adv(ctx); }

    while (LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_expr(ctx, &arg_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, call_idx, arg_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_COMMA) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            continue;
        }

        break;
    }

    return crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_RPAREN);
}

static uint32_t crosstalk_hdl_parse_concat(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx)
{
    crosstalk_hdl_tok_t brace_tok = LDG_STRUCT_ZERO_INIT;
    uint32_t first_idx = 0;
    uint32_t inner_idx = 0;
    uint32_t elem_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;
    brace_tok = ctx->cur;

    ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_LBRACE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_expr(ctx, &first_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_LBRACE) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_EXPR_REPL, &brace_tok, out_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, first_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_concat(ctx, &inner_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, inner_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        return crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_RBRACE);
    }

    ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_EXPR_CONCAT, &brace_tok, out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, first_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_COMMA) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_expr(ctx, &elem_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, elem_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_RBRACE);
}

static uint32_t crosstalk_hdl_parse_pattern(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx)
{
    crosstalk_hdl_tok_t tick_tok = LDG_STRUCT_ZERO_INIT;
    uint32_t elem_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;
    tick_tok = ctx->cur;

    ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_TICK);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_LBRACE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_EXPR_PATTERN, &tick_tok, out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_expr(ctx, &elem_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_COLON) == LDG_TRUTH_TRUE) { return CROSSTALK_HDL_ERR_PARSE_SUBSET; }

    ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, elem_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_COMMA) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_expr(ctx, &elem_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_COLON) == LDG_TRUTH_TRUE) { return CROSSTALK_HDL_ERR_PARSE_SUBSET; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, elem_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_RBRACE);
}

static uint32_t crosstalk_hdl_parse_primary(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx, uint8_t *out_callable)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0 || out_callable == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;
    *out_callable = LDG_TRUTH_FALSE;

    if (ctx->cur.kind == CROSSTALK_HDL_TOK_LIT_DEC || ctx->cur.kind == CROSSTALK_HDL_TOK_LIT_BASED || ctx->cur.kind == CROSSTALK_HDL_TOK_LIT_UNBASED || ctx->cur.kind == CROSSTALK_HDL_TOK_LIT_REAL || ctx->cur.kind == CROSSTALK_HDL_TOK_LIT_STR)
    {
        ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_EXPR_LIT, &ctx->cur, out_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        return crosstalk_hdl_parse_adv(ctx);
    }

    if (ctx->cur.kind == CROSSTALK_HDL_TOK_IDENT || ctx->cur.kind == CROSSTALK_HDL_TOK_SYS_IDENT)
    {
        ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_EXPR_IDENT, &ctx->cur, out_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        *out_callable = LDG_TRUTH_TRUE;

        return crosstalk_hdl_parse_adv(ctx);
    }

    if (ctx->cur.kind == CROSSTALK_HDL_TOK_KEYWORD && crosstalk_hdl_parse_next_op_is(ctx, CROSSTALK_HDL_OP_TICK) == LDG_TRUTH_TRUE && (ctx->cur.val == CROSSTALK_HDL_KW_INT || ctx->cur.val == CROSSTALK_HDL_KW_INTEGER || ctx->cur.val == CROSSTALK_HDL_KW_LOGIC || ctx->cur.val == CROSSTALK_HDL_KW_SIGNED || ctx->cur.val == CROSSTALK_HDL_KW_UNSIGNED))
    {
        ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_TYPE, &ctx->cur, out_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        return crosstalk_hdl_parse_adv(ctx);
    }

    if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_LPAREN) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_expr(ctx, out_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        return crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_RPAREN);
    }

    if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_TICK) == LDG_TRUTH_TRUE && crosstalk_hdl_parse_next_op_is(ctx, CROSSTALK_HDL_OP_LBRACE) == LDG_TRUTH_TRUE) { return crosstalk_hdl_parse_pattern(ctx, out_idx); }

    if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_LBRACE) == LDG_TRUTH_TRUE) { return crosstalk_hdl_parse_concat(ctx, out_idx); }

    if (ctx->cur.kind == CROSSTALK_HDL_TOK_DIRECTIVE) { return CROSSTALK_HDL_ERR_PARSE_SUBSET; }

    return CROSSTALK_HDL_ERR_PARSE_TOK;
}

static uint32_t crosstalk_hdl_parse_postfix(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx)
{
    crosstalk_hdl_tok_t op_tok = LDG_STRUCT_ZERO_INIT;
    uint32_t base_idx = 0;
    uint32_t sub_idx = 0;
    uint32_t msb_idx = 0;
    uint32_t lsb_idx = 0;
    uint32_t part_kind = 0;
    uint8_t callable = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    ret = crosstalk_hdl_parse_depth_enter(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_primary(ctx, &base_idx, &callable);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { crosstalk_hdl_parse_depth_leave(ctx); return ret; }

    while (LDG_TRUTH_TRUE)
    {
        if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_LPAREN) == LDG_TRUTH_TRUE && callable == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_EXPR_CALL, 0x0, &sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

            ret = crosstalk_hdl_ast_child_append(ctx->ast, sub_idx, base_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

            ret = crosstalk_hdl_parse_call_args(ctx, sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

            base_idx = sub_idx;
            callable = LDG_TRUTH_FALSE;
            continue;
        }

        if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_LBRACK) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

            ret = crosstalk_hdl_parse_expr(ctx, &msb_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

            part_kind = CROSSTALK_HDL_NODE_NONE;

            if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_COLON) == LDG_TRUTH_TRUE) { part_kind = CROSSTALK_HDL_NODE_EXPR_PART; }
            else if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_PLUS) == LDG_TRUTH_TRUE && crosstalk_hdl_parse_next_op_is(ctx, CROSSTALK_HDL_OP_COLON) == LDG_TRUTH_TRUE) { part_kind = CROSSTALK_HDL_NODE_EXPR_PART_PLUS; }
            else if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_MINUS) == LDG_TRUTH_TRUE && crosstalk_hdl_parse_next_op_is(ctx, CROSSTALK_HDL_OP_COLON) == LDG_TRUTH_TRUE) { part_kind = CROSSTALK_HDL_NODE_EXPR_PART_MINUS; }

            if (part_kind != CROSSTALK_HDL_NODE_NONE)
            {
                if (part_kind != CROSSTALK_HDL_NODE_EXPR_PART)
                {
                    ret = crosstalk_hdl_parse_adv(ctx);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }
                }

                ret = crosstalk_hdl_parse_adv(ctx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

                ret = crosstalk_hdl_parse_expr(ctx, &lsb_idx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

                ret = crosstalk_hdl_parse_mk(ctx, part_kind, 0x0, &sub_idx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

                ret = crosstalk_hdl_ast_child_append(ctx->ast, sub_idx, base_idx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

                ret = crosstalk_hdl_ast_child_append(ctx->ast, sub_idx, msb_idx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

                ret = crosstalk_hdl_ast_child_append(ctx->ast, sub_idx, lsb_idx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }
            }
            else
            {
                ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_EXPR_IDX, 0x0, &sub_idx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

                ret = crosstalk_hdl_ast_child_append(ctx->ast, sub_idx, base_idx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

                ret = crosstalk_hdl_ast_child_append(ctx->ast, sub_idx, msb_idx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }
            }

            ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_RBRACK);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

            base_idx = sub_idx;
            continue;
        }

        if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_DOT) == LDG_TRUTH_TRUE && ctx->nxt.kind == CROSSTALK_HDL_TOK_IDENT)
        {
            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

            op_tok = ctx->cur;

            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

            ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_EXPR_DOT, &op_tok, &sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

            ret = crosstalk_hdl_ast_child_append(ctx->ast, sub_idx, base_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

            base_idx = sub_idx;
            callable = LDG_TRUTH_TRUE;
            continue;
        }

        if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_TICK) == LDG_TRUTH_TRUE && ctx->nxt.kind == CROSSTALK_HDL_TOK_OP && ctx->nxt.val == CROSSTALK_HDL_OP_LPAREN)
        {
            uint32_t cast_val_idx = 0;

            op_tok = ctx->cur;

            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

            ret = crosstalk_hdl_parse_expr(ctx, &cast_val_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

            ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_RPAREN);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

            ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_EXPR_CAST, &op_tok, &sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

            ret = crosstalk_hdl_ast_child_append(ctx->ast, sub_idx, base_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

            ret = crosstalk_hdl_ast_child_append(ctx->ast, sub_idx, cast_val_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

            base_idx = sub_idx;
            continue;
        }

        if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_SCOPE) == LDG_TRUTH_TRUE && ctx->nxt.kind == CROSSTALK_HDL_TOK_IDENT)
        {
            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

            op_tok = ctx->cur;

            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

            ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_EXPR_SCOPE, &op_tok, &sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

            ret = crosstalk_hdl_ast_child_append(ctx->ast, sub_idx, base_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

            base_idx = sub_idx;
            callable = LDG_TRUTH_TRUE;
            continue;
        }

        break;
    }

    crosstalk_hdl_parse_depth_leave(ctx);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_idx = base_idx;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_parse_un(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx)
{
    crosstalk_hdl_tok_t op_tok = LDG_STRUCT_ZERO_INIT;
    uint32_t child_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    if (crosstalk_hdl_parse_un_op_is(ctx) == LDG_TRUTH_TRUE)
    {
        op_tok = ctx->cur;

        ret = crosstalk_hdl_parse_depth_enter(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_adv(ctx);
        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_un(ctx, &child_idx); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_EXPR_UN, &op_tok, out_idx); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, child_idx); }

        crosstalk_hdl_parse_depth_leave(ctx);

        return ret;
    }

    return crosstalk_hdl_parse_postfix(ctx, out_idx);
}

static uint32_t crosstalk_hdl_parse_bin(crosstalk_hdl_parse_ctx_t *ctx, uint32_t min_prec, uint32_t *out_idx)
{
    crosstalk_hdl_tok_t op_tok = LDG_STRUCT_ZERO_INIT;
    uint32_t lhs_idx = 0;
    uint32_t rhs_idx = 0;
    uint32_t bin_idx = 0;
    uint32_t prec = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    ret = crosstalk_hdl_parse_un(ctx, &lhs_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (LDG_TRUTH_TRUE)
    {
        prec = crosstalk_hdl_parse_bin_prec_get(ctx);
        if (prec == CROSSTALK_HDL_PARSE_PREC_NONE || prec < min_prec) { break; }

        op_tok = ctx->cur;

        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_bin(ctx, prec + 0x1, &rhs_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_EXPR_BIN, &op_tok, &bin_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, bin_idx, lhs_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, bin_idx, rhs_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        lhs_idx = bin_idx;
    }

    *out_idx = lhs_idx;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_parse_expr(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx)
{
    crosstalk_hdl_tok_t quest_tok = LDG_STRUCT_ZERO_INIT;
    uint32_t cond_idx = 0;
    uint32_t true_idx = 0;
    uint32_t false_idx = 0;
    uint32_t node_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    ret = crosstalk_hdl_parse_depth_enter(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_bin(ctx, CROSSTALK_HDL_PARSE_PREC_LOW, &cond_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { crosstalk_hdl_parse_depth_leave(ctx); return ret; }

    if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_QUESTION) != LDG_TRUTH_TRUE)
    {
        crosstalk_hdl_parse_depth_leave(ctx);
        *out_idx = cond_idx;

        return LDG_ERR_AOK;
    }

    quest_tok = ctx->cur;

    ret = crosstalk_hdl_parse_adv(ctx);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_expr(ctx, &true_idx); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_COLON); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_expr(ctx, &false_idx); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_EXPR_COND, &quest_tok, &node_idx); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_ast_child_append(ctx->ast, node_idx, cond_idx); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_ast_child_append(ctx->ast, node_idx, true_idx); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_ast_child_append(ctx->ast, node_idx, false_idx); }

    crosstalk_hdl_parse_depth_leave(ctx);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_idx = node_idx;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_parse_range(crosstalk_hdl_parse_ctx_t *ctx, uint32_t kind, uint32_t *out_idx)
{
    uint32_t msb_idx = 0;
    uint32_t lsb_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_LBRACK);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_expr(ctx, &msb_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (kind == CROSSTALK_HDL_NODE_UNPACKED_RANGE && crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_RBRACK) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_mk(ctx, kind, 0x0, out_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        return crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, msb_idx);
    }

    ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_COLON);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_expr(ctx, &lsb_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_RBRACK);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_mk(ctx, kind, 0x0, out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, msb_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, lsb_idx);
}

static uint8_t crosstalk_hdl_parse_type_kw_is(const crosstalk_hdl_parse_ctx_t *ctx)
{
    uint32_t kw = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_TRUTH_FALSE; }

    if (ctx->cur.kind != CROSSTALK_HDL_TOK_KEYWORD) { return LDG_TRUTH_FALSE; }

    kw = ctx->cur.val;

    if (kw == CROSSTALK_HDL_KW_WIRE || kw == CROSSTALK_HDL_KW_REG || kw == CROSSTALK_HDL_KW_LOGIC) { return LDG_TRUTH_TRUE; }

    if (kw == CROSSTALK_HDL_KW_INT || kw == CROSSTALK_HDL_KW_INTEGER || kw == CROSSTALK_HDL_KW_GENVAR) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t crosstalk_hdl_parse_enum_type(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx)
{
    crosstalk_hdl_tok_t name_tok = LDG_STRUCT_ZERO_INIT;
    uint32_t item_idx = 0;
    uint32_t init_idx = 0;
    uint32_t base_idx = 0;
    uint32_t range_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_ENUM, &ctx->cur, out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_kw_expect(ctx, CROSSTALK_HDL_KW_ENUM);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (crosstalk_hdl_parse_type_kw_is(ctx) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_TYPE, &ctx->cur, &base_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_LBRACK) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_range(ctx, CROSSTALK_HDL_NODE_RANGE, &range_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_ast_child_append(ctx->ast, base_idx, range_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, base_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_LBRACE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (LDG_TRUTH_TRUE)
    {
        if (ctx->cur.kind != CROSSTALK_HDL_TOK_IDENT) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

        name_tok = ctx->cur;

        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_ENUM_ITEM, &name_tok, &item_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_ASSIGN) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_parse_expr(ctx, &init_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_ast_child_append(ctx->ast, item_idx, init_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, item_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_COMMA) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            continue;
        }

        break;
    }

    return crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_RBRACE);
}

static uint32_t crosstalk_hdl_parse_type(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx)
{
    uint32_t mark_idx = 0;
    uint32_t range_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_ENUM) == LDG_TRUTH_TRUE) { return crosstalk_hdl_parse_enum_type(ctx, out_idx); }

    if (crosstalk_hdl_parse_type_kw_is(ctx) == LDG_TRUTH_TRUE || ctx->cur.kind == CROSSTALK_HDL_TOK_IDENT)
    {
        ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_TYPE, &ctx->cur, out_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    else
    {
        ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_TYPE, 0x0, out_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_SIGNED) == LDG_TRUTH_TRUE || crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_UNSIGNED) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_SIGNED_MARK, &ctx->cur, &mark_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, mark_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    while (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_LBRACK) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_range(ctx, CROSSTALK_HDL_NODE_RANGE, &range_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, range_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_parse_decl_name(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx)
{
    crosstalk_hdl_tok_t name_tok = LDG_STRUCT_ZERO_INIT;
    uint32_t range_idx = 0;
    uint32_t init_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    if (ctx->cur.kind != CROSSTALK_HDL_TOK_IDENT) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

    name_tok = ctx->cur;

    ret = crosstalk_hdl_parse_adv(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_DECL_NAME, &name_tok, out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_LBRACK) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_range(ctx, CROSSTALK_HDL_NODE_UNPACKED_RANGE, &range_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, range_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_ASSIGN) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_expr(ctx, &init_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, init_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_parse_decl_names(crosstalk_hdl_parse_ctx_t *ctx, uint32_t parent_idx)
{
    uint32_t name_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_decl_name(ctx, &name_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, parent_idx, name_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_COMMA) == LDG_TRUTH_TRUE && ctx->nxt.kind == CROSSTALK_HDL_TOK_IDENT)
        {
            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            continue;
        }

        break;
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_parse_data_decl(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx)
{
    uint32_t decl_kind = 0;
    uint32_t type_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    decl_kind = CROSSTALK_HDL_NODE_VAR_DECL;
    if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_WIRE) == LDG_TRUTH_TRUE) { decl_kind = CROSSTALK_HDL_NODE_NET_DECL; }

    if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_GENVAR) == LDG_TRUTH_TRUE) { decl_kind = CROSSTALK_HDL_NODE_GENVAR_DECL; }

    ret = crosstalk_hdl_parse_mk(ctx, decl_kind, &ctx->cur, out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_type(ctx, &type_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, type_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_decl_names(ctx, *out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_SEMI);
}

static uint32_t crosstalk_hdl_parse_param_decl(crosstalk_hdl_parse_ctx_t *ctx, uint8_t expect_semi, uint32_t *out_idx)
{
    uint32_t decl_kind = 0;
    uint32_t type_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    decl_kind = CROSSTALK_HDL_NODE_PARAM_DECL;
    if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_LOCALPARAM) == LDG_TRUTH_TRUE) { decl_kind = CROSSTALK_HDL_NODE_LOCALPARAM_DECL; }

    ret = crosstalk_hdl_parse_mk(ctx, decl_kind, &ctx->cur, out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_adv(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->cur.kind != CROSSTALK_HDL_TOK_IDENT || ctx->nxt.kind == CROSSTALK_HDL_TOK_IDENT)
    {
        ret = crosstalk_hdl_parse_type(ctx, &type_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, type_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_parse_decl_names(ctx, *out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (expect_semi == LDG_TRUTH_TRUE) { return crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_SEMI); }

    return LDG_ERR_AOK;
}

static uint8_t crosstalk_hdl_parse_dir_kw_is(const crosstalk_hdl_parse_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_TRUTH_FALSE; }

    if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_INPUT) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_OUTPUT) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_INOUT) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t crosstalk_hdl_parse_port_group(crosstalk_hdl_parse_ctx_t *ctx, uint8_t multi, uint32_t *out_idx)
{
    uint32_t dir_idx = 0;
    uint32_t type_idx = 0;
    uint32_t name_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_PORT, 0x0, out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (crosstalk_hdl_parse_dir_kw_is(ctx) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_DIR, &ctx->cur, &dir_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, dir_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (crosstalk_hdl_parse_type_kw_is(ctx) == LDG_TRUTH_TRUE || crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_SIGNED) == LDG_TRUTH_TRUE || crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_LBRACK) == LDG_TRUTH_TRUE || (ctx->cur.kind == CROSSTALK_HDL_TOK_IDENT && ctx->nxt.kind == CROSSTALK_HDL_TOK_IDENT))
    {
        ret = crosstalk_hdl_parse_type(ctx, &type_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, type_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    while (LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_decl_name(ctx, &name_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, name_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (multi != LDG_TRUTH_TRUE) { break; }

        if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_COMMA) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            continue;
        }

        break;
    }

    if (multi == LDG_TRUTH_TRUE) { return crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_SEMI); }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_parse_sens(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx)
{
    crosstalk_hdl_tok_t edge_tok = LDG_STRUCT_ZERO_INIT;
    crosstalk_hdl_tok_t blank = LDG_STRUCT_ZERO_INIT;
    uint32_t edge_idx = 0;
    uint32_t expr_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_SENS, 0x0, out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_AT) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = crosstalk_hdl_parse_adv(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_STAR) == LDG_TRUTH_TRUE) { return crosstalk_hdl_parse_adv(ctx); }

    ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_LPAREN);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_STAR) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        return crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_RPAREN);
    }

    while (LDG_TRUTH_TRUE)
    {
        edge_tok = blank;

        if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_POSEDGE) == LDG_TRUTH_TRUE || crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_NEGEDGE) == LDG_TRUTH_TRUE)
        {
            edge_tok = ctx->cur;

            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_EDGE, &edge_tok, &edge_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_expr(ctx, &expr_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, edge_idx, expr_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, edge_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_OR) == LDG_TRUTH_TRUE || crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_COMMA) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            continue;
        }

        break;
    }

    return crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_RPAREN);
}

static uint8_t crosstalk_hdl_parse_assign_op_is(const crosstalk_hdl_parse_ctx_t *ctx)
{
    uint32_t op = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_TRUTH_FALSE; }

    if (ctx->cur.kind != CROSSTALK_HDL_TOK_OP) { return LDG_TRUTH_FALSE; }

    op = ctx->cur.val;

    if (op == CROSSTALK_HDL_OP_ASSIGN || op == CROSSTALK_HDL_OP_LE) { return LDG_TRUTH_TRUE; }

    if (op == CROSSTALK_HDL_OP_PLUS_ASSIGN || op == CROSSTALK_HDL_OP_MINUS_ASSIGN || op == CROSSTALK_HDL_OP_STAR_ASSIGN || op == CROSSTALK_HDL_OP_SLASH_ASSIGN) { return LDG_TRUTH_TRUE; }

    if (op == CROSSTALK_HDL_OP_PERCENT_ASSIGN || op == CROSSTALK_HDL_OP_AMP_ASSIGN || op == CROSSTALK_HDL_OP_PIPE_ASSIGN || op == CROSSTALK_HDL_OP_CARET_ASSIGN) { return LDG_TRUTH_TRUE; }

    if (op == CROSSTALK_HDL_OP_SHL_ASSIGN || op == CROSSTALK_HDL_OP_SHR_ASSIGN) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t crosstalk_hdl_parse_assign_tail(crosstalk_hdl_parse_ctx_t *ctx, uint32_t lhs_idx, uint8_t expect_semi, uint32_t *out_idx)
{
    crosstalk_hdl_tok_t op_tok = LDG_STRUCT_ZERO_INIT;
    uint32_t rhs_idx = 0;
    uint32_t node_kind = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_INC) == LDG_TRUTH_TRUE || crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_DEC) == LDG_TRUTH_TRUE)
    {
        op_tok = ctx->cur;

        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_B_ASSIGN, &op_tok, out_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, lhs_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (expect_semi == LDG_TRUTH_TRUE) { return crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_SEMI); }

        return LDG_ERR_AOK;
    }

    if (crosstalk_hdl_parse_assign_op_is(ctx) != LDG_TRUTH_TRUE) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

    op_tok = ctx->cur;
    node_kind = op_tok.val == CROSSTALK_HDL_OP_LE ? CROSSTALK_HDL_NODE_NB_ASSIGN : CROSSTALK_HDL_NODE_B_ASSIGN;

    ret = crosstalk_hdl_parse_adv(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_expr(ctx, &rhs_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_mk(ctx, node_kind, &op_tok, out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, lhs_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, rhs_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (expect_semi == LDG_TRUTH_TRUE) { return crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_SEMI); }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_parse_case(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx)
{
    uint32_t case_kind = 0;
    uint32_t subject_idx = 0;
    uint32_t item_idx = 0;
    uint32_t label_idx = 0;
    uint32_t stmt_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    case_kind = CROSSTALK_HDL_NODE_CASE;
    if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_CASEZ) == LDG_TRUTH_TRUE) { case_kind = CROSSTALK_HDL_NODE_CASEZ; }

    if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_CASEX) == LDG_TRUTH_TRUE) { case_kind = CROSSTALK_HDL_NODE_CASEX; }

    ret = crosstalk_hdl_parse_mk(ctx, case_kind, &ctx->cur, out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_adv(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_LPAREN);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_expr(ctx, &subject_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, subject_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_RPAREN);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_ENDCASE) != LDG_TRUTH_TRUE)
    {
        if (ctx->cur.kind == CROSSTALK_HDL_TOK_EOF) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

        if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_DEFAULT) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_CASE_DEFAULT, &ctx->cur, &item_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_COLON) == LDG_TRUTH_TRUE)
            {
                ret = crosstalk_hdl_parse_adv(ctx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
        }
        else
        {
            ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_CASE_ITEM, &ctx->cur, &item_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            while (LDG_TRUTH_TRUE)
            {
                ret = crosstalk_hdl_parse_expr(ctx, &label_idx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                ret = crosstalk_hdl_ast_child_append(ctx->ast, item_idx, label_idx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_COMMA) == LDG_TRUTH_TRUE)
                {
                    ret = crosstalk_hdl_parse_adv(ctx);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    continue;
                }

                break;
            }

            ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_COLON);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_parse_stmt(ctx, &stmt_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, item_idx, stmt_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, item_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return crosstalk_hdl_parse_adv(ctx);
}

static uint32_t crosstalk_hdl_parse_gen_body(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx)
{
    crosstalk_hdl_tok_t label_tok = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_BEGIN) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_COLON) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (ctx->cur.kind != CROSSTALK_HDL_TOK_IDENT) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

            label_tok = ctx->cur;

            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_BLK, &label_tok, out_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        while (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_END) != LDG_TRUTH_TRUE)
        {
            if (ctx->cur.kind == CROSSTALK_HDL_TOK_EOF) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

            ret = crosstalk_hdl_parse_module_item(ctx, *out_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        return crosstalk_hdl_parse_adv(ctx);
    }

    ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_BLK, &label_tok, out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_parse_module_item(ctx, *out_idx);
}

static uint32_t crosstalk_hdl_parse_for(crosstalk_hdl_parse_ctx_t *ctx, uint8_t item_body, uint32_t *out_idx)
{
    uint32_t init_idx = 0;
    uint32_t cond_idx = 0;
    uint32_t step_idx = 0;
    uint32_t body_idx = 0;
    uint32_t lhs_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_FOR, &ctx->cur, out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_kw_expect(ctx, CROSSTALK_HDL_KW_FOR);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_LPAREN);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (crosstalk_hdl_parse_type_kw_is(ctx) == LDG_TRUTH_TRUE)
    {
        uint32_t type_idx = 0;

        ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_VAR_DECL, &ctx->cur, &init_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_type(ctx, &type_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, init_idx, type_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_decl_names(ctx, init_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    else
    {
        ret = crosstalk_hdl_parse_postfix(ctx, &lhs_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_assign_tail(ctx, lhs_idx, LDG_TRUTH_FALSE, &init_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, init_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_SEMI);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_expr(ctx, &cond_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, cond_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_SEMI);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_INC) == LDG_TRUTH_TRUE || crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_DEC) == LDG_TRUTH_TRUE)
    {
        crosstalk_hdl_tok_t step_op_tok = ctx->cur;

        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_postfix(ctx, &lhs_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_B_ASSIGN, &step_op_tok, &step_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, step_idx, lhs_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    else
    {
        ret = crosstalk_hdl_parse_postfix(ctx, &lhs_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_assign_tail(ctx, lhs_idx, LDG_TRUTH_FALSE, &step_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, step_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_RPAREN);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (item_body == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_parse_gen_body(ctx, &body_idx); }
    else { ret = crosstalk_hdl_parse_stmt(ctx, &body_idx); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, body_idx);
}

static uint32_t crosstalk_hdl_parse_stmt(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx)
{
    crosstalk_hdl_tok_t kw_tok = LDG_STRUCT_ZERO_INIT;
    uint32_t sub_idx = 0;
    uint32_t cond_idx = 0;
    uint32_t lhs_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    ret = crosstalk_hdl_parse_depth_enter(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_BEGIN) == LDG_TRUTH_TRUE)
    {
        crosstalk_hdl_tok_t label_tok = LDG_STRUCT_ZERO_INIT;

        ret = crosstalk_hdl_parse_adv(ctx);

        if (ret == LDG_ERR_AOK && crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_COLON) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_adv(ctx);
            if (ret == LDG_ERR_AOK && ctx->cur.kind == CROSSTALK_HDL_TOK_IDENT)
            {
                label_tok = ctx->cur;
                ret = crosstalk_hdl_parse_adv(ctx);
            }
            else if (ret == LDG_ERR_AOK) { ret = CROSSTALK_HDL_ERR_PARSE_TOK; }
        }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_BLK, &label_tok, out_idx); }

        while (ret == LDG_ERR_AOK && crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_END) != LDG_TRUTH_TRUE)
        {
            if (ctx->cur.kind == CROSSTALK_HDL_TOK_EOF) { ret = CROSSTALK_HDL_ERR_PARSE_TOK; break; }

            ret = crosstalk_hdl_parse_stmt(ctx, &sub_idx);
            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, sub_idx); }
        }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_adv(ctx); }

        crosstalk_hdl_parse_depth_leave(ctx);

        return ret;
    }

    if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_AUTOMATIC) == LDG_TRUTH_TRUE || crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_UNIQUE) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { crosstalk_hdl_parse_depth_leave(ctx); return ret; }
    }

    if (crosstalk_hdl_parse_type_kw_is(ctx) == LDG_TRUTH_TRUE || (ctx->cur.kind == CROSSTALK_HDL_TOK_IDENT && ctx->nxt.kind == CROSSTALK_HDL_TOK_IDENT))
    {
        ret = crosstalk_hdl_parse_data_decl(ctx, out_idx);
        crosstalk_hdl_parse_depth_leave(ctx);

        return ret;
    }

    if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_ASSIGN) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_adv(ctx);
        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_postfix(ctx, &lhs_idx); }

        if (ret == LDG_ERR_AOK && crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_ASSIGN) != LDG_TRUTH_TRUE) { ret = CROSSTALK_HDL_ERR_PARSE_TOK; }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_assign_tail(ctx, lhs_idx, LDG_TRUTH_TRUE, out_idx); }

        crosstalk_hdl_parse_depth_leave(ctx);

        return ret;
    }

    if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_IF) == LDG_TRUTH_TRUE)
    {
        kw_tok = ctx->cur;

        ret = crosstalk_hdl_parse_adv(ctx);
        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_LPAREN); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_expr(ctx, &cond_idx); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_RPAREN); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_IF, &kw_tok, out_idx); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, cond_idx); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_stmt(ctx, &sub_idx); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, sub_idx); }

        if (ret == LDG_ERR_AOK && crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_ELSE) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_adv(ctx);
            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_stmt(ctx, &sub_idx); }

            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, sub_idx); }
        }

        crosstalk_hdl_parse_depth_leave(ctx);

        return ret;
    }

    if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_CASE) == LDG_TRUTH_TRUE || crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_CASEZ) == LDG_TRUTH_TRUE || crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_CASEX) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_case(ctx, out_idx);
        crosstalk_hdl_parse_depth_leave(ctx);

        return ret;
    }

    if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_FOR) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_for(ctx, LDG_TRUTH_FALSE, out_idx);
        crosstalk_hdl_parse_depth_leave(ctx);

        return ret;
    }

    if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_WHILE) == LDG_TRUTH_TRUE || crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_REPEAT) == LDG_TRUTH_TRUE)
    {
        uint32_t loop_kind = crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_WHILE) == LDG_TRUTH_TRUE ? CROSSTALK_HDL_NODE_WHILE : CROSSTALK_HDL_NODE_REPEAT;

        kw_tok = ctx->cur;

        ret = crosstalk_hdl_parse_adv(ctx);
        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_LPAREN); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_expr(ctx, &cond_idx); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_RPAREN); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_mk(ctx, loop_kind, &kw_tok, out_idx); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, cond_idx); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_stmt(ctx, &sub_idx); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, sub_idx); }

        crosstalk_hdl_parse_depth_leave(ctx);

        return ret;
    }

    if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_RET) == LDG_TRUTH_TRUE)
    {
        kw_tok = ctx->cur;

        ret = crosstalk_hdl_parse_adv(ctx);
        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_RET_STMT, &kw_tok, out_idx); }

        if (ret == LDG_ERR_AOK && crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_SEMI) != LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_expr(ctx, &sub_idx);
            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, sub_idx); }
        }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_SEMI); }

        crosstalk_hdl_parse_depth_leave(ctx);

        return ret;
    }

    if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_SEMI) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_BLK, 0x0, out_idx);
        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_adv(ctx); }

        crosstalk_hdl_parse_depth_leave(ctx);

        return ret;
    }

    if (crosstalk_hdl_parse_type_kw_is(ctx) == LDG_TRUTH_TRUE || crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_ENUM) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_data_decl(ctx, out_idx);
        crosstalk_hdl_parse_depth_leave(ctx);

        return ret;
    }

    ret = crosstalk_hdl_parse_postfix(ctx, &lhs_idx);

    if (ret == LDG_ERR_AOK && crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_SEMI) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_STMT_EXPR, 0x0, out_idx);
        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, lhs_idx); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_adv(ctx); }

        crosstalk_hdl_parse_depth_leave(ctx);

        return ret;
    }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_assign_tail(ctx, lhs_idx, LDG_TRUTH_TRUE, out_idx); }

    crosstalk_hdl_parse_depth_leave(ctx);

    return ret;
}

static uint32_t crosstalk_hdl_parse_conn_list(crosstalk_hdl_parse_ctx_t *ctx, uint32_t parent_idx, uint32_t conn_kind)
{
    crosstalk_hdl_tok_t name_tok = LDG_STRUCT_ZERO_INIT;
    crosstalk_hdl_tok_t blank = LDG_STRUCT_ZERO_INIT;
    uint32_t conn_idx = 0;
    uint32_t expr_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_LPAREN);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_RPAREN) == LDG_TRUTH_TRUE) { return crosstalk_hdl_parse_adv(ctx); }

    while (LDG_TRUTH_TRUE)
    {
        name_tok = blank;

        if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_DOT) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (ctx->cur.kind != CROSSTALK_HDL_TOK_IDENT) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

            name_tok = ctx->cur;

            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_parse_mk(ctx, conn_kind, &name_tok, &conn_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_LPAREN) == LDG_TRUTH_TRUE)
            {
                ret = crosstalk_hdl_parse_adv(ctx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_RPAREN) != LDG_TRUTH_TRUE)
                {
                    ret = crosstalk_hdl_parse_expr(ctx, &expr_idx);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    ret = crosstalk_hdl_ast_child_append(ctx->ast, conn_idx, expr_idx);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
                }

                ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_RPAREN);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
        }
        else
        {
            ret = crosstalk_hdl_parse_mk(ctx, conn_kind, &name_tok, &conn_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_parse_expr(ctx, &expr_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_ast_child_append(ctx->ast, conn_idx, expr_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, parent_idx, conn_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_COMMA) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            continue;
        }

        break;
    }

    return crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_RPAREN);
}

static uint32_t crosstalk_hdl_parse_inst_items(crosstalk_hdl_parse_ctx_t *ctx, uint32_t inst_idx)
{
    crosstalk_hdl_tok_t item_tok = LDG_STRUCT_ZERO_INIT;
    uint32_t item_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (LDG_TRUTH_TRUE)
    {
        if (ctx->cur.kind != CROSSTALK_HDL_TOK_IDENT) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

        item_tok = ctx->cur;

        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_INST_ITEM, &item_tok, &item_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_conn_list(ctx, item_idx, CROSSTALK_HDL_NODE_PORT_CONN);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, inst_idx, item_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_COMMA) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            continue;
        }

        break;
    }

    return crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_SEMI);
}

static uint32_t crosstalk_hdl_parse_inst(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx)
{
    crosstalk_hdl_tok_t mod_tok = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;
    mod_tok = ctx->cur;

    ret = crosstalk_hdl_parse_adv(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_INST, &mod_tok, out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_HASH) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_conn_list(ctx, *out_idx, CROSSTALK_HDL_NODE_PARAM_CONN);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return crosstalk_hdl_parse_inst_items(ctx, *out_idx);
}

static uint32_t crosstalk_hdl_parse_always(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx)
{
    uint32_t proc_kind = 0;
    uint32_t sens_idx = 0;
    uint32_t stmt_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    proc_kind = CROSSTALK_HDL_NODE_ALWAYS;
    if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_ALWAYS_FF) == LDG_TRUTH_TRUE) { proc_kind = CROSSTALK_HDL_NODE_ALWAYS_FF; }

    if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_ALWAYS_COMB) == LDG_TRUTH_TRUE) { proc_kind = CROSSTALK_HDL_NODE_ALWAYS_COMB; }

    ret = crosstalk_hdl_parse_mk(ctx, proc_kind, &ctx->cur, out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_adv(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_sens(ctx, &sens_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, sens_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_stmt(ctx, &stmt_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, stmt_idx);
}

static uint32_t crosstalk_hdl_parse_attr(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx)
{
    crosstalk_hdl_tok_t name_tok = LDG_STRUCT_ZERO_INIT;
    uint32_t item_idx = 0;
    uint32_t expr_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_ATTR, &ctx->cur, out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_adv(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (LDG_TRUTH_TRUE)
    {
        if (ctx->cur.kind != CROSSTALK_HDL_TOK_IDENT) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

        name_tok = ctx->cur;

        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_ATTR_ITEM, &name_tok, &item_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_ASSIGN) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_parse_expr(ctx, &expr_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_ast_child_append(ctx->ast, item_idx, expr_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, item_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_COMMA) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            continue;
        }

        break;
    }

    if (ctx->cur.kind != CROSSTALK_HDL_TOK_ATTR_CLOSE) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

    return crosstalk_hdl_parse_adv(ctx);
}

static uint32_t crosstalk_hdl_parse_func(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx)
{
    crosstalk_hdl_tok_t name_tok = LDG_STRUCT_ZERO_INIT;
    uint32_t decl_kind = 0;
    uint32_t end_kw = 0;
    uint32_t type_idx = 0;
    uint32_t sub_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;
    type_idx = UINT32_MAX;

    decl_kind = CROSSTALK_HDL_NODE_FUNC_DECL;
    end_kw = CROSSTALK_HDL_KW_ENDFUNC;
    if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_TASK) == LDG_TRUTH_TRUE)
    {
        decl_kind = CROSSTALK_HDL_NODE_TASK_DECL;
        end_kw = CROSSTALK_HDL_KW_ENDTASK;
    }

    ret = crosstalk_hdl_parse_adv(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_AUTOMATIC) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (decl_kind == CROSSTALK_HDL_NODE_FUNC_DECL && (crosstalk_hdl_parse_type_kw_is(ctx) == LDG_TRUTH_TRUE || crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_LBRACK) == LDG_TRUTH_TRUE || (ctx->cur.kind == CROSSTALK_HDL_TOK_IDENT && ctx->nxt.kind == CROSSTALK_HDL_TOK_IDENT)))
    {
        ret = crosstalk_hdl_parse_type(ctx, &type_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (ctx->cur.kind != CROSSTALK_HDL_TOK_IDENT) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

    name_tok = ctx->cur;

    ret = crosstalk_hdl_parse_adv(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_mk(ctx, decl_kind, &name_tok, out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (type_idx != UINT32_MAX)
    {
        ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, type_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_LPAREN) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        while (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_RPAREN) != LDG_TRUTH_TRUE)
        {
            if (ctx->cur.kind == CROSSTALK_HDL_TOK_EOF) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

            if (ctx->cur.kind == CROSSTALK_HDL_TOK_ATTR_OPEN)
            {
                uint32_t attr_idx = 0;

                ret = crosstalk_hdl_parse_attr(ctx, &attr_idx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                continue;
            }

            ret = crosstalk_hdl_parse_port_group(ctx, LDG_TRUTH_FALSE, &sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_COMMA) == LDG_TRUTH_TRUE)
            {
                ret = crosstalk_hdl_parse_adv(ctx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
        }

        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_SEMI);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (crosstalk_hdl_parse_kw_is(ctx, end_kw) != LDG_TRUTH_TRUE)
    {
        if (ctx->cur.kind == CROSSTALK_HDL_TOK_EOF) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

        if (crosstalk_hdl_parse_dir_kw_is(ctx) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_port_group(ctx, LDG_TRUTH_TRUE, &sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
        else
        {
            ret = crosstalk_hdl_parse_stmt(ctx, &sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, sub_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return crosstalk_hdl_parse_adv(ctx);
}

static uint32_t crosstalk_hdl_parse_typedef(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx)
{
    crosstalk_hdl_tok_t name_tok = LDG_STRUCT_ZERO_INIT;
    uint32_t type_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    ret = crosstalk_hdl_parse_kw_expect(ctx, CROSSTALK_HDL_KW_TYPEDEF);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_UNION) == LDG_TRUTH_TRUE) { return CROSSTALK_HDL_ERR_PARSE_SUBSET; }

    if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_STRUCT) == LDG_TRUTH_TRUE)
    {
        crosstalk_hdl_tok_t struct_tok = ctx->cur;
        uint32_t struct_idx = 0;
        uint32_t member_idx = 0;

        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_PACKED) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_LBRACE);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_STRUCT, &struct_tok, &struct_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        while (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_RBRACE) != LDG_TRUTH_TRUE)
        {
            if (ctx->cur.kind == CROSSTALK_HDL_TOK_EOF) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

            ret = crosstalk_hdl_parse_data_decl(ctx, &member_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_ast_child_append(ctx->ast, struct_idx, member_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (ctx->cur.kind != CROSSTALK_HDL_TOK_IDENT) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

        name_tok = ctx->cur;

        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_TYPEDEF, &name_tok, out_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, struct_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        return crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_SEMI);
    }

    ret = crosstalk_hdl_parse_type(ctx, &type_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->cur.kind != CROSSTALK_HDL_TOK_IDENT) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

    name_tok = ctx->cur;

    ret = crosstalk_hdl_parse_adv(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_TYPEDEF, &name_tok, out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, type_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_SEMI);
}

static uint32_t crosstalk_hdl_parse_import_item(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx)
{
    crosstalk_hdl_tok_t pkg_tok = LDG_STRUCT_ZERO_INIT;
    uint32_t name_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    if (ctx->cur.kind != CROSSTALK_HDL_TOK_IDENT) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

    pkg_tok = ctx->cur;

    ret = crosstalk_hdl_parse_adv(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_IMPORT, &pkg_tok, out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_SCOPE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_STAR) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    else if (ctx->cur.kind == CROSSTALK_HDL_TOK_IDENT)
    {
        ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_EXPR_IDENT, &ctx->cur, &name_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, name_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    else { return CROSSTALK_HDL_ERR_PARSE_TOK; }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_parse_import(crosstalk_hdl_parse_ctx_t *ctx, uint32_t parent_idx, uint32_t *out_idx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    ret = crosstalk_hdl_parse_kw_expect(ctx, CROSSTALK_HDL_KW_IMPORT);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_import_item(ctx, out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_COMMA) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, parent_idx, *out_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_import_item(ctx, out_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_SEMI);
}

static uint32_t crosstalk_hdl_parse_gen_if(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx)
{
    crosstalk_hdl_tok_t kw_tok = LDG_STRUCT_ZERO_INIT;
    uint32_t cond_idx = 0;
    uint32_t body_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;
    kw_tok = ctx->cur;

    ret = crosstalk_hdl_parse_kw_expect(ctx, CROSSTALK_HDL_KW_IF);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_LPAREN);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_expr(ctx, &cond_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_RPAREN);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_IF, &kw_tok, out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, cond_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_gen_body(ctx, &body_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, body_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_ELSE) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_IF) == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_parse_gen_if(ctx, &body_idx); }
        else { ret = crosstalk_hdl_parse_gen_body(ctx, &body_idx); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, body_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_parse_gen_case(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx)
{
    uint32_t subject_idx = 0;
    uint32_t item_idx = 0;
    uint32_t label_idx = 0;
    uint32_t body_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_CASE, &ctx->cur, out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_kw_expect(ctx, CROSSTALK_HDL_KW_CASE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_LPAREN);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_expr(ctx, &subject_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, subject_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_RPAREN);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_ENDCASE) != LDG_TRUTH_TRUE)
    {
        if (ctx->cur.kind == CROSSTALK_HDL_TOK_EOF) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

        if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_DEFAULT) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_CASE_DEFAULT, &ctx->cur, &item_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_COLON) == LDG_TRUTH_TRUE)
            {
                ret = crosstalk_hdl_parse_adv(ctx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
        }
        else
        {
            ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_CASE_ITEM, &ctx->cur, &item_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            while (LDG_TRUTH_TRUE)
            {
                ret = crosstalk_hdl_parse_expr(ctx, &label_idx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                ret = crosstalk_hdl_ast_child_append(ctx->ast, item_idx, label_idx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_COMMA) == LDG_TRUTH_TRUE)
                {
                    ret = crosstalk_hdl_parse_adv(ctx);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    continue;
                }

                break;
            }

            ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_COLON);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_parse_gen_body(ctx, &body_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, item_idx, body_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, item_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return crosstalk_hdl_parse_adv(ctx);
}

static uint32_t crosstalk_hdl_parse_modport(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx)
{
    crosstalk_hdl_tok_t name_tok = LDG_STRUCT_ZERO_INIT;
    uint32_t group_idx = 0;
    uint32_t dir_idx = 0;
    uint32_t name_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    ret = crosstalk_hdl_parse_kw_expect(ctx, CROSSTALK_HDL_KW_MODPORT);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->cur.kind != CROSSTALK_HDL_TOK_IDENT) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

    name_tok = ctx->cur;

    ret = crosstalk_hdl_parse_adv(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_MODPORT, &name_tok, out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_LPAREN);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_RPAREN) != LDG_TRUTH_TRUE)
    {
        if (crosstalk_hdl_parse_dir_kw_is(ctx) != LDG_TRUTH_TRUE) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

        ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_PORT, 0x0, &group_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_DIR, &ctx->cur, &dir_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, group_idx, dir_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        while (LDG_TRUTH_TRUE)
        {
            if (ctx->cur.kind != CROSSTALK_HDL_TOK_IDENT) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

            ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_DECL_NAME, &ctx->cur, &name_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_ast_child_append(ctx->ast, group_idx, name_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_COMMA) == LDG_TRUTH_TRUE && ctx->nxt.kind == CROSSTALK_HDL_TOK_IDENT)
            {
                ret = crosstalk_hdl_parse_adv(ctx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                continue;
            }

            break;
        }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, group_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_COMMA) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }

    ret = crosstalk_hdl_parse_adv(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_SEMI);
}

static uint32_t crosstalk_hdl_parse_package(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx)
{
    crosstalk_hdl_tok_t name_tok = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    ret = crosstalk_hdl_parse_kw_expect(ctx, CROSSTALK_HDL_KW_PACKAGE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->cur.kind != CROSSTALK_HDL_TOK_IDENT) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

    name_tok = ctx->cur;

    ret = crosstalk_hdl_parse_adv(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_PACKAGE, &name_tok, out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_SEMI);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_ENDPACKAGE) != LDG_TRUTH_TRUE)
    {
        if (ctx->cur.kind == CROSSTALK_HDL_TOK_EOF) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

        ret = crosstalk_hdl_parse_module_item(ctx, *out_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return crosstalk_hdl_parse_adv(ctx);
}

static uint32_t crosstalk_hdl_parse_interface(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx)
{
    crosstalk_hdl_tok_t name_tok = LDG_STRUCT_ZERO_INIT;
    uint32_t sub_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    ret = crosstalk_hdl_parse_kw_expect(ctx, CROSSTALK_HDL_KW_INTERFACE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->cur.kind != CROSSTALK_HDL_TOK_IDENT) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

    name_tok = ctx->cur;

    ret = crosstalk_hdl_parse_adv(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_INTERFACE, &name_tok, out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_LPAREN) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        while (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_RPAREN) != LDG_TRUTH_TRUE)
        {
            if (ctx->cur.kind == CROSSTALK_HDL_TOK_EOF) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

            if (ctx->cur.kind == CROSSTALK_HDL_TOK_ATTR_OPEN)
            {
                uint32_t attr_idx = 0;

                ret = crosstalk_hdl_parse_attr(ctx, &attr_idx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                continue;
            }

            ret = crosstalk_hdl_parse_port_group(ctx, LDG_TRUTH_FALSE, &sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_COMMA) == LDG_TRUTH_TRUE)
            {
                ret = crosstalk_hdl_parse_adv(ctx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
        }

        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_SEMI);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_ENDINTERFACE) != LDG_TRUTH_TRUE)
    {
        if (ctx->cur.kind == CROSSTALK_HDL_TOK_EOF) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

        ret = crosstalk_hdl_parse_module_item(ctx, *out_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return crosstalk_hdl_parse_adv(ctx);
}

static uint32_t crosstalk_hdl_parse_module_item(crosstalk_hdl_parse_ctx_t *ctx, uint32_t parent_idx)
{
    uint32_t item_idx = 0;
    uint32_t attr_idx = 0;
    uint32_t sub_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_parse_depth_enter(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    attr_idx = UINT32_MAX;
    if (ctx->cur.kind == CROSSTALK_HDL_TOK_ATTR_OPEN)
    {
        ret = crosstalk_hdl_parse_attr(ctx, &attr_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { crosstalk_hdl_parse_depth_leave(ctx); return ret; }
    }

    if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_PARAMETER) == LDG_TRUTH_TRUE || crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_LOCALPARAM) == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_parse_param_decl(ctx, LDG_TRUTH_TRUE, &item_idx); }
    else if (crosstalk_hdl_parse_dir_kw_is(ctx) == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_parse_port_group(ctx, LDG_TRUTH_TRUE, &item_idx); }
    else if (crosstalk_hdl_parse_type_kw_is(ctx) == LDG_TRUTH_TRUE || crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_ENUM) == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_parse_data_decl(ctx, &item_idx); }
    else if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_ASSIGN) == LDG_TRUTH_TRUE)
    {
        crosstalk_hdl_tok_t assign_tok = ctx->cur;
        uint32_t lhs_idx = 0;
        uint32_t rhs_idx = 0;

        ret = crosstalk_hdl_parse_adv(ctx);
        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_postfix(ctx, &lhs_idx); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_ASSIGN); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_expr(ctx, &rhs_idx); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_CONT_ASSIGN, &assign_tok, &item_idx); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_ast_child_append(ctx->ast, item_idx, lhs_idx); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_ast_child_append(ctx->ast, item_idx, rhs_idx); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_SEMI); }
    }
    else if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_ALWAYS) == LDG_TRUTH_TRUE || crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_ALWAYS_FF) == LDG_TRUTH_TRUE || crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_ALWAYS_COMB) == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_parse_always(ctx, &item_idx); }
    else if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_INITIAL) == LDG_TRUTH_TRUE || crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_FINAL) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_mk(ctx, crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_FINAL) == LDG_TRUTH_TRUE ? CROSSTALK_HDL_NODE_FINAL : CROSSTALK_HDL_NODE_INITIAL, &ctx->cur, &item_idx);
        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_adv(ctx); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_stmt(ctx, &sub_idx); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_ast_child_append(ctx->ast, item_idx, sub_idx); }
    }
    else if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_FUNC) == LDG_TRUTH_TRUE || crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_TASK) == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_parse_func(ctx, &item_idx); }
    else if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_TYPEDEF) == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_parse_typedef(ctx, &item_idx); }
    else if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_IMPORT) == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_parse_import(ctx, parent_idx, &item_idx); }
    else if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_GENERATE) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_GENERATE, &ctx->cur, &item_idx);
        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_adv(ctx); }

        while (ret == LDG_ERR_AOK && crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_ENDGENERATE) != LDG_TRUTH_TRUE)
        {
            if (ctx->cur.kind == CROSSTALK_HDL_TOK_EOF) { ret = CROSSTALK_HDL_ERR_PARSE_TOK; break; }

            ret = crosstalk_hdl_parse_module_item(ctx, item_idx);
        }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_adv(ctx); }
    }
    else if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_FOR) == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_parse_for(ctx, LDG_TRUTH_TRUE, &item_idx); }
    else if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_IF) == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_parse_gen_if(ctx, &item_idx); }
    else if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_CASE) == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_parse_gen_case(ctx, &item_idx); }
    else if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_MODPORT) == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_parse_modport(ctx, &item_idx); }
    else if (ctx->cur.kind == CROSSTALK_HDL_TOK_IDENT)
    {
        if (ctx->nxt.kind == CROSSTALK_HDL_TOK_IDENT)
        {
            crosstalk_hdl_tok_t first_tok = ctx->cur;

            ret = crosstalk_hdl_parse_adv(ctx);
            if (ret == LDG_ERR_AOK && crosstalk_hdl_parse_next_op_is(ctx, CROSSTALK_HDL_OP_LPAREN) == LDG_TRUTH_TRUE)
            {
                ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_INST, &first_tok, &item_idx);
                if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_inst_items(ctx, item_idx); }
            }
            else if (ret == LDG_ERR_AOK)
            {
                uint32_t type_idx = 0;

                ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_VAR_DECL, &first_tok, &item_idx);
                if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_TYPE, &first_tok, &type_idx); }

                if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_ast_child_append(ctx->ast, item_idx, type_idx); }

                if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_decl_names(ctx, item_idx); }

                if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_SEMI); }
            }
        }
        else { ret = crosstalk_hdl_parse_inst(ctx, &item_idx); }
    }
    else if (ctx->cur.kind == CROSSTALK_HDL_TOK_DIRECTIVE) { ret = CROSSTALK_HDL_ERR_PARSE_SUBSET; }
    else { ret = CROSSTALK_HDL_ERR_PARSE_TOK; }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_ast_child_append(ctx->ast, parent_idx, item_idx); }

    if (ret == LDG_ERR_AOK && attr_idx != UINT32_MAX) { ret = crosstalk_hdl_ast_child_append(ctx->ast, item_idx, attr_idx); }

    crosstalk_hdl_parse_depth_leave(ctx);

    return ret;
}

static uint32_t crosstalk_hdl_parse_module(crosstalk_hdl_parse_ctx_t *ctx, uint32_t *out_idx)
{
    crosstalk_hdl_tok_t name_tok = LDG_STRUCT_ZERO_INIT;
    uint32_t sub_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    ret = crosstalk_hdl_parse_kw_expect(ctx, CROSSTALK_HDL_KW_MODULE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->cur.kind != CROSSTALK_HDL_TOK_IDENT) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

    name_tok = ctx->cur;

    ret = crosstalk_hdl_parse_adv(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_MODULE, &name_tok, out_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_IMPORT) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_import(ctx, *out_idx, &sub_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, sub_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_HASH) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_LPAREN);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        while (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_RPAREN) != LDG_TRUTH_TRUE)
        {
            if (ctx->cur.kind == CROSSTALK_HDL_TOK_EOF) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

            if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_PARAMETER) == LDG_TRUTH_TRUE || crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_LOCALPARAM) == LDG_TRUTH_TRUE)
            {
                ret = crosstalk_hdl_parse_param_decl(ctx, LDG_TRUTH_FALSE, &sub_idx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
            else
            {
                crosstalk_hdl_tok_t blank = LDG_STRUCT_ZERO_INIT;

                ret = crosstalk_hdl_parse_mk(ctx, CROSSTALK_HDL_NODE_PARAM_DECL, &blank, &sub_idx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                ret = crosstalk_hdl_parse_decl_names(ctx, sub_idx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_COMMA) == LDG_TRUTH_TRUE)
            {
                ret = crosstalk_hdl_parse_adv(ctx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
        }

        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_LPAREN) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        while (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_RPAREN) != LDG_TRUTH_TRUE)
        {
            if (ctx->cur.kind == CROSSTALK_HDL_TOK_EOF) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

            if (ctx->cur.kind == CROSSTALK_HDL_TOK_ATTR_OPEN)
            {
                uint32_t attr_idx = 0;

                ret = crosstalk_hdl_parse_attr(ctx, &attr_idx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                continue;
            }

            ret = crosstalk_hdl_parse_port_group(ctx, LDG_TRUTH_FALSE, &sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_ast_child_append(ctx->ast, *out_idx, sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_COMMA) == LDG_TRUTH_TRUE)
            {
                ret = crosstalk_hdl_parse_adv(ctx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
        }

        ret = crosstalk_hdl_parse_adv(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_parse_op_expect(ctx, CROSSTALK_HDL_OP_SEMI);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_ENDMODULE) != LDG_TRUTH_TRUE)
    {
        if (ctx->cur.kind == CROSSTALK_HDL_TOK_EOF) { return CROSSTALK_HDL_ERR_PARSE_TOK; }

        ret = crosstalk_hdl_parse_module_item(ctx, *out_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return crosstalk_hdl_parse_adv(ctx);
}

uint32_t crosstalk_hdl_parse_run(crosstalk_hdl_parse_ctx_t *ctx, const char *src, uint32_t len, crosstalk_hdl_ast_t *ast)
{
    uint32_t root_idx = 0;
    uint32_t sub_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || src == 0x0 || ast == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->ast = ast;
    ctx->depth = 0x0;
    ctx->prev = (crosstalk_hdl_tok_t)LDG_STRUCT_ZERO_INIT;

    ret = crosstalk_hdl_lex_init(&ctx->lex, src, len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_lex_next(&ctx->lex, &ctx->cur);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_lex_next(&ctx->lex, &ctx->nxt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_ast_node_add(ast, CROSSTALK_HDL_NODE_SRC, 0x0, &root_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ast->root = root_idx;

    while (ctx->cur.kind != CROSSTALK_HDL_TOK_EOF)
    {
        if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_MODULE) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_module(ctx, &sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_ast_child_append(ctx->ast, root_idx, sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            continue;
        }

        if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_TYPEDEF) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_typedef(ctx, &sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_ast_child_append(ctx->ast, root_idx, sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            continue;
        }

        if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_IMPORT) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_import(ctx, root_idx, &sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_ast_child_append(ctx->ast, root_idx, sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            continue;
        }

        if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_PACKAGE) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_package(ctx, &sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_ast_child_append(ctx->ast, root_idx, sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            continue;
        }

        if (crosstalk_hdl_parse_kw_is(ctx, CROSSTALK_HDL_KW_INTERFACE) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_interface(ctx, &sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_ast_child_append(ctx->ast, root_idx, sub_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            continue;
        }

        if (crosstalk_hdl_parse_op_is(ctx, CROSSTALK_HDL_OP_SEMI) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_parse_adv(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            continue;
        }

        if (ctx->cur.kind == CROSSTALK_HDL_TOK_DIRECTIVE) { return CROSSTALK_HDL_ERR_PARSE_SUBSET; }

        return CROSSTALK_HDL_ERR_PARSE_TOK;
    }

    return LDG_ERR_AOK;
}
