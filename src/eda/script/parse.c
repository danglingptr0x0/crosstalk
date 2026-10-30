#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "eda/script/parse.h"

#define PARSE_STEP 1
#define PARSE_MUL_CUNT 3
#define PARSE_ADD_CUNT 2
#define PARSE_SHIFT_CUNT 6
#define PARSE_REL_CUNT 4
#define PARSE_EQ_CUNT 2
#define PARSE_BAND_CUNT 2
#define PARSE_BXOR_CUNT 3
#define PARSE_BOR_CUNT 2
#define PARSE_LAND_CUNT 1
#define PARSE_LOR_CUNT 1
#define PARSE_BICOND_CUNT 1
#define PARSE_STR_WIDTH 8
#define PARSE_NOTCH_MIN 2
#define PARSE_DIAG_FMT "%s"

typedef struct parse_ctx
{
    crosstalk_eda_script_lex_t *lex;
    crosstalk_eda_script_ast_t *ast;
    crosstalk_eda_script_diag_t *diag;
    crosstalk_eda_script_tok_t tok;
} parse_ctx_t;

typedef struct parse_lvl
{
    uint32_t tok;
    uint32_t op;
} parse_lvl_t;

static uint32_t parse_expr(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out);
static uint32_t parse_assign_expr(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out);
static uint32_t parse_stmt(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out);

static uint32_t parse_diag_set(parse_ctx_t *ctx, const crosstalk_eda_script_tok_t *tok, uint32_t code, const char *msg, const char *suggestion)
{
    ctx->diag->line = tok->line;
    ctx->diag->col = tok->col;

    if (snprintf(ctx->diag->msg, sizeof(ctx->diag->msg), PARSE_DIAG_FMT, msg) < 0) { return code; }

    if (snprintf(ctx->diag->suggestion, sizeof(ctx->diag->suggestion), PARSE_DIAG_FMT, suggestion) < 0) { return code; }

    return code;
}

static uint32_t parse_advance(parse_ctx_t *ctx)
{
    uint32_t ret = crosstalk_eda_script_lex_next(ctx->lex, &ctx->tok);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return parse_diag_set(ctx, &ctx->tok, ret, "invalid token", "remove or correct the flagged characters"); }

    return LDG_ERR_AOK;
}

static uint32_t parse_node_new(parse_ctx_t *ctx, uint32_t kind, const crosstalk_eda_script_tok_t *tok, crosstalk_eda_script_node_t **out)
{
    uint32_t ret = crosstalk_eda_script_ast_node_new(ctx->ast, kind, tok, out);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return parse_diag_set(ctx, tok, ret, "script is too large", "shorten the script or raise the script pool size"); }

    return LDG_ERR_AOK;
}

static uint32_t parse_semi_want(parse_ctx_t *ctx, const crosstalk_eda_script_tok_t *start_tok)
{
    if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_SEMI)) { return parse_diag_set(ctx, start_tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "statement is not terminated", "end every statement with a semicolon"); }

    return parse_advance(ctx);
}

static uint32_t parse_arg_list(parse_ctx_t *ctx, crosstalk_eda_script_list_t *list)
{
    uint32_t ret = parse_advance(ctx);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_RPAREN) { return parse_advance(ctx); }

    while (LDG_TRUTH_TRUE)
    {
        crosstalk_eda_script_node_t *arg = 0x0;

        if (ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_STR)
        {
            crosstalk_eda_script_tok_t str_tok = ctx->tok;

            ret = parse_node_new(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_STR, &str_tok, &arg);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_eda_script_ast_str_decode(ctx->ast, str_tok.start, str_tok.len, &arg->bytes, &arg->bytes_len);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return parse_diag_set(ctx, &str_tok, ret, "invalid string literal", "use only the escapes n t r backslash quote and zero"); }

            arg->width = (uint32_t)PARSE_STR_WIDTH;

            ret = parse_advance(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
        else
        {
            ret = parse_assign_expr(ctx, &arg);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_eda_script_ast_list_push(list, arg);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_COMMA)
        {
            ret = parse_advance(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            continue;
        }

        break;
    }

    if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_RPAREN)) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "argument list is not closed", "close the call with a right parenthesis"); }

    return parse_advance(ctx);
}

static uint32_t parse_call(parse_ctx_t *ctx, const crosstalk_eda_script_tok_t *name_tok, crosstalk_eda_script_node_t **out)
{
    uint32_t ret = parse_node_new(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_CALL, name_tok, out);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_eda_script_ast_str_copy(ctx->ast, name_tok->start, name_tok->len, &(*out)->name);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    (*out)->name_len = name_tok->len;

    return parse_arg_list(ctx, &(*out)->list);
}

static uint32_t parse_primary(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out)
{
    crosstalk_eda_script_tok_t tok = ctx->tok;
    uint32_t ret = 0;

    if (tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_KW_CALL)
    {
        crosstalk_eda_script_tok_t name_tok = LDG_STRUCT_ZERO_INIT;

        ret = parse_advance(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_IDENT)) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "call is missing a subroutine name", "write the call as call name(args)"); }

        name_tok = ctx->tok;

        ret = parse_advance(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_LPAREN)) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "call is missing its argument list", "write the call as call name(args)"); }

        return parse_call(ctx, &name_tok, out);
    }

    if (tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_IDENT)
    {
        crosstalk_eda_script_tok_t peek = LDG_STRUCT_ZERO_INIT;

        ret = crosstalk_eda_script_lex_peek(ctx->lex, &peek);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return parse_diag_set(ctx, &ctx->tok, ret, "invalid token", "remove or correct the flagged characters"); }

        if (peek.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_LPAREN)
        {
            ret = parse_advance(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            return parse_call(ctx, &tok, out);
        }

        ret = parse_node_new(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_IDENT, &tok, out);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_eda_script_ast_str_copy(ctx->ast, tok.start, tok.len, &(*out)->name);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        (*out)->name_len = tok.len;

        return parse_advance(ctx);
    }

    if (tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_INT || tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_CHAR)
    {
        if (LDG_UNLIKELY(tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_INT && tok.width == 0)) { return parse_diag_set(ctx, &tok, CROSSTALK_EDA_SCRIPT_ERR_LIT_BARE, "literal has no width", "prefix the literal with its width, as in 32'0x2A"); }

        ret = parse_node_new(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_NUM, &tok, out);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        (*out)->val = tok.val;
        (*out)->width = tok.width;

        return parse_advance(ctx);
    }

    if (tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_STR) { return parse_diag_set(ctx, &tok, CROSSTALK_EDA_SCRIPT_ERR_TYPE, "string literal used as a value", "strings are allowed only as call arguments or array initializers"); }

    if (tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_LPAREN)
    {
        ret = parse_advance(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = parse_expr(ctx, out);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_RPAREN)) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "parenthesized expression is not closed", "close the expression with a right parenthesis"); }

        return parse_advance(ctx);
    }

    if (tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_LBRACE)
    {
        ret = parse_node_new(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_CONCAT, &tok, out);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = parse_advance(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        while (LDG_TRUTH_TRUE)
        {
            crosstalk_eda_script_node_t *elem = 0x0;

            ret = parse_expr(ctx, &elem);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_eda_script_ast_list_push(&(*out)->list, elem);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_COMMA)
            {
                ret = parse_advance(ctx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                continue;
            }

            break;
        }

        if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_RBRACE)) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "concatenation is not closed", "close the concatenation with a right brace"); }

        return parse_advance(ctx);
    }

    return parse_diag_set(ctx, &tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "expected an expression", "write a sized literal, a variable, a call, or a parenthesized expression");
}

static uint32_t parse_postfix(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out)
{
    uint32_t ret = parse_primary(ctx, out);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (LDG_TRUTH_TRUE)
    {
        crosstalk_eda_script_tok_t open_tok = ctx->tok;
        crosstalk_eda_script_node_t *first = 0x0;

        if (ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_DOT) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_PROFILE, "member access is not in the script profile", "the script profile has no structures"); }

        if (ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_LBRACKET) { break; }

        ret = parse_advance(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = parse_expr(ctx, &first);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_COLON)
        {
            crosstalk_eda_script_node_t *lo = 0x0;
            crosstalk_eda_script_node_t *slice = 0x0;

            ret = parse_advance(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = parse_expr(ctx, &lo);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (LDG_UNLIKELY(first->kind != (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_NUM || lo->kind != (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_NUM)) { return parse_diag_set(ctx, &open_tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "slice bounds are not constant", "use sized literals for both slice bounds"); }

            ret = parse_node_new(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_SLICE, &open_tok, &slice);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            slice->lhs = *out;
            slice->rhs = first;
            slice->third = lo;
            *out = slice;
        }
        else
        {
            crosstalk_eda_script_node_t *idx = 0x0;

            ret = parse_node_new(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_IDX, &open_tok, &idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            idx->lhs = *out;
            idx->rhs = first;
            *out = idx;
        }

        if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_RBRACKET)) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "subscript is not closed", "close the subscript with a right bracket"); }

        ret = parse_advance(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t parse_unary(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out)
{
    crosstalk_eda_script_tok_t tok = ctx->tok;
    crosstalk_eda_script_node_t *node = 0x0;
    uint32_t op = (uint32_t)CROSSTALK_EDA_SCRIPT_OP_NEG;
    uint8_t un_is = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_AMP || tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_STAR) { return parse_diag_set(ctx, &tok, CROSSTALK_EDA_SCRIPT_ERR_PROFILE, "pointer operators are not in the script profile", "the script profile has no pointers"); }

    if (tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_MINUS) { op = (uint32_t)CROSSTALK_EDA_SCRIPT_OP_NEG; un_is = LDG_TRUTH_TRUE; }
    else if (tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_TILDE || tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_KW_NOT) { op = (uint32_t)CROSSTALK_EDA_SCRIPT_OP_BNOT; un_is = LDG_TRUTH_TRUE; }
    else if (tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_BANG) { op = (uint32_t)CROSSTALK_EDA_SCRIPT_OP_LNOT; un_is = LDG_TRUTH_TRUE; }

    if (un_is != LDG_TRUTH_TRUE) { return parse_postfix(ctx, out); }

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = parse_node_new(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_UN, &tok, &node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    node->op = op;

    ret = parse_unary(ctx, &node->lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = node;

    return LDG_ERR_AOK;
}

static uint32_t parse_bin(parse_ctx_t *ctx, const parse_lvl_t *ops, uint32_t ops_cunt, uint32_t (*sub)(parse_ctx_t *, crosstalk_eda_script_node_t **), crosstalk_eda_script_node_t **out)
{
    uint32_t ret = sub(ctx, out);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (LDG_TRUTH_TRUE)
    {
        uint32_t idx = 0;
        uint8_t hit = LDG_TRUTH_FALSE;

        for (idx = 0; idx < ops_cunt; idx = idx + (uint32_t)PARSE_STEP)
        {
            crosstalk_eda_script_tok_t op_tok = ctx->tok;
            crosstalk_eda_script_node_t *node = 0x0;

            if (ctx->tok.kind != ops[idx].tok) { continue; }

            ret = parse_advance(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = parse_node_new(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_BIN, &op_tok, &node);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            node->op = ops[idx].op;
            node->lhs = *out;

            ret = sub(ctx, &node->rhs);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            *out = node;
            hit = LDG_TRUTH_TRUE;

            break;
        }

        if (hit != LDG_TRUTH_TRUE) { break; }
    }

    return LDG_ERR_AOK;
}

static uint32_t parse_mul(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out)
{
    static const parse_lvl_t ops[] = { { CROSSTALK_EDA_SCRIPT_TOK_STAR, CROSSTALK_EDA_SCRIPT_OP_MUL }, { CROSSTALK_EDA_SCRIPT_TOK_SLASH, CROSSTALK_EDA_SCRIPT_OP_DIV }, { CROSSTALK_EDA_SCRIPT_TOK_PERCENT, CROSSTALK_EDA_SCRIPT_OP_MOD } };

    return parse_bin(ctx, ops, (uint32_t)PARSE_MUL_CUNT, parse_unary, out);
}

static uint32_t parse_add(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out)
{
    static const parse_lvl_t ops[] = { { CROSSTALK_EDA_SCRIPT_TOK_PLUS, CROSSTALK_EDA_SCRIPT_OP_ADD }, { CROSSTALK_EDA_SCRIPT_TOK_MINUS, CROSSTALK_EDA_SCRIPT_OP_SUB } };

    return parse_bin(ctx, ops, (uint32_t)PARSE_ADD_CUNT, parse_mul, out);
}

static uint32_t parse_shift(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out)
{
    static const parse_lvl_t ops[] = { { CROSSTALK_EDA_SCRIPT_TOK_SHL, CROSSTALK_EDA_SCRIPT_OP_SHL }, { CROSSTALK_EDA_SCRIPT_TOK_SHR, CROSSTALK_EDA_SCRIPT_OP_SHR }, { CROSSTALK_EDA_SCRIPT_TOK_SHLA, CROSSTALK_EDA_SCRIPT_OP_SHLA }, { CROSSTALK_EDA_SCRIPT_TOK_SHRA, CROSSTALK_EDA_SCRIPT_OP_SHRA }, { CROSSTALK_EDA_SCRIPT_TOK_KW_SHL, CROSSTALK_EDA_SCRIPT_OP_SHL }, { CROSSTALK_EDA_SCRIPT_TOK_KW_SHR, CROSSTALK_EDA_SCRIPT_OP_SHR } };

    return parse_bin(ctx, ops, (uint32_t)PARSE_SHIFT_CUNT, parse_add, out);
}

static uint32_t parse_rel(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out)
{
    static const parse_lvl_t ops[] = { { CROSSTALK_EDA_SCRIPT_TOK_LT, CROSSTALK_EDA_SCRIPT_OP_LT }, { CROSSTALK_EDA_SCRIPT_TOK_GT, CROSSTALK_EDA_SCRIPT_OP_GT }, { CROSSTALK_EDA_SCRIPT_TOK_LTE, CROSSTALK_EDA_SCRIPT_OP_LTE }, { CROSSTALK_EDA_SCRIPT_TOK_GTE, CROSSTALK_EDA_SCRIPT_OP_GTE } };

    return parse_bin(ctx, ops, (uint32_t)PARSE_REL_CUNT, parse_shift, out);
}

static uint32_t parse_eq(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out)
{
    static const parse_lvl_t ops[] = { { CROSSTALK_EDA_SCRIPT_TOK_EQ, CROSSTALK_EDA_SCRIPT_OP_EQ }, { CROSSTALK_EDA_SCRIPT_TOK_NEQ, CROSSTALK_EDA_SCRIPT_OP_NEQ } };

    return parse_bin(ctx, ops, (uint32_t)PARSE_EQ_CUNT, parse_rel, out);
}

static uint32_t parse_band(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out)
{
    static const parse_lvl_t ops[] = { { CROSSTALK_EDA_SCRIPT_TOK_AMP, CROSSTALK_EDA_SCRIPT_OP_BAND }, { CROSSTALK_EDA_SCRIPT_TOK_KW_AND, CROSSTALK_EDA_SCRIPT_OP_BAND } };

    return parse_bin(ctx, ops, (uint32_t)PARSE_BAND_CUNT, parse_eq, out);
}

static uint32_t parse_bxor(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out)
{
    static const parse_lvl_t ops[] = { { CROSSTALK_EDA_SCRIPT_TOK_CARET, CROSSTALK_EDA_SCRIPT_OP_BXOR }, { CROSSTALK_EDA_SCRIPT_TOK_XNOR, CROSSTALK_EDA_SCRIPT_OP_BXNOR }, { CROSSTALK_EDA_SCRIPT_TOK_KW_XOR, CROSSTALK_EDA_SCRIPT_OP_BXOR } };

    return parse_bin(ctx, ops, (uint32_t)PARSE_BXOR_CUNT, parse_band, out);
}

static uint32_t parse_bor(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out)
{
    static const parse_lvl_t ops[] = { { CROSSTALK_EDA_SCRIPT_TOK_PIPE, CROSSTALK_EDA_SCRIPT_OP_BOR }, { CROSSTALK_EDA_SCRIPT_TOK_KW_OR, CROSSTALK_EDA_SCRIPT_OP_BOR } };

    return parse_bin(ctx, ops, (uint32_t)PARSE_BOR_CUNT, parse_bxor, out);
}

static uint32_t parse_land(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out)
{
    static const parse_lvl_t ops[] = { { CROSSTALK_EDA_SCRIPT_TOK_LAND, CROSSTALK_EDA_SCRIPT_OP_LAND } };

    return parse_bin(ctx, ops, (uint32_t)PARSE_LAND_CUNT, parse_bor, out);
}

static uint32_t parse_lor(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out)
{
    static const parse_lvl_t ops[] = { { CROSSTALK_EDA_SCRIPT_TOK_LOR, CROSSTALK_EDA_SCRIPT_OP_LOR } };

    return parse_bin(ctx, ops, (uint32_t)PARSE_LOR_CUNT, parse_land, out);
}

static uint32_t parse_impl(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out)
{
    crosstalk_eda_script_tok_t op_tok = LDG_STRUCT_ZERO_INIT;
    crosstalk_eda_script_node_t *node = 0x0;
    uint32_t ret = parse_lor(ctx, out);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_IMPL) { return LDG_ERR_AOK; }

    op_tok = ctx->tok;

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = parse_node_new(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_BIN, &op_tok, &node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    node->op = (uint32_t)CROSSTALK_EDA_SCRIPT_OP_IMPL;
    node->lhs = *out;

    ret = parse_impl(ctx, &node->rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = node;

    return LDG_ERR_AOK;
}

static uint32_t parse_bicond(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out)
{
    static const parse_lvl_t ops[] = { { CROSSTALK_EDA_SCRIPT_TOK_BICOND, CROSSTALK_EDA_SCRIPT_OP_BICOND } };

    return parse_bin(ctx, ops, (uint32_t)PARSE_BICOND_CUNT, parse_impl, out);
}

static uint32_t parse_ternary(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out)
{
    crosstalk_eda_script_tok_t op_tok = LDG_STRUCT_ZERO_INIT;
    crosstalk_eda_script_node_t *node = 0x0;
    uint32_t ret = parse_bicond(ctx, out);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_QUESTION) { return LDG_ERR_AOK; }

    op_tok = ctx->tok;

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = parse_node_new(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_TERNARY, &op_tok, &node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    node->lhs = *out;

    ret = parse_expr(ctx, &node->rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_COLON)) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "conditional expression is missing its colon", "write the conditional as cond ? a : b"); }

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = parse_ternary(ctx, &node->third);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = node;

    return LDG_ERR_AOK;
}

static uint8_t parse_lvalue_is(const crosstalk_eda_script_node_t *node)
{
    if (node->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_IDENT) { return LDG_TRUTH_TRUE; }

    if (node->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_IDX) { return LDG_TRUTH_TRUE; }

    if (node->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_SLICE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t parse_assign_op_get(uint32_t tok_kind, uint32_t *out)
{
    if (tok_kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_ASSIGN) { *out = CROSSTALK_EDA_SCRIPT_OP_ASSIGN; return LDG_ERR_AOK; }

    if (tok_kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_PLUS_ASSIGN) { *out = CROSSTALK_EDA_SCRIPT_OP_ADD_ASSIGN; return LDG_ERR_AOK; }

    if (tok_kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_MINUS_ASSIGN) { *out = CROSSTALK_EDA_SCRIPT_OP_SUB_ASSIGN; return LDG_ERR_AOK; }

    if (tok_kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_STAR_ASSIGN) { *out = CROSSTALK_EDA_SCRIPT_OP_MUL_ASSIGN; return LDG_ERR_AOK; }

    if (tok_kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_SLASH_ASSIGN) { *out = CROSSTALK_EDA_SCRIPT_OP_DIV_ASSIGN; return LDG_ERR_AOK; }

    if (tok_kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_PERCENT_ASSIGN) { *out = CROSSTALK_EDA_SCRIPT_OP_MOD_ASSIGN; return LDG_ERR_AOK; }

    if (tok_kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_AMP_ASSIGN) { *out = CROSSTALK_EDA_SCRIPT_OP_AND_ASSIGN; return LDG_ERR_AOK; }

    if (tok_kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_PIPE_ASSIGN) { *out = CROSSTALK_EDA_SCRIPT_OP_OR_ASSIGN; return LDG_ERR_AOK; }

    if (tok_kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_CARET_ASSIGN) { *out = CROSSTALK_EDA_SCRIPT_OP_XOR_ASSIGN; return LDG_ERR_AOK; }

    if (tok_kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_SHL_ASSIGN) { *out = CROSSTALK_EDA_SCRIPT_OP_SHL_ASSIGN; return LDG_ERR_AOK; }

    if (tok_kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_SHR_ASSIGN) { *out = CROSSTALK_EDA_SCRIPT_OP_SHR_ASSIGN; return LDG_ERR_AOK; }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t parse_assign_expr(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out)
{
    crosstalk_eda_script_tok_t op_tok = LDG_STRUCT_ZERO_INIT;
    crosstalk_eda_script_node_t *node = 0x0;
    uint32_t op = 0;
    uint32_t ret = parse_ternary(ctx, out);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (parse_assign_op_get(ctx->tok.kind, &op) != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    op_tok = ctx->tok;

    if (LDG_UNLIKELY(parse_lvalue_is(*out) != LDG_TRUTH_TRUE)) { return parse_diag_set(ctx, &op_tok, CROSSTALK_EDA_SCRIPT_ERR_LVALUE, "assignment target cannot be assigned to", "assign to a variable, an array element, or a bit slice"); }

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = parse_node_new(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_ASSIGN, &op_tok, &node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    node->op = op;
    node->lhs = *out;

    ret = parse_assign_expr(ctx, &node->rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = node;

    return LDG_ERR_AOK;
}

static uint32_t parse_expr(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out)
{
    return parse_assign_expr(ctx, out);
}

static uint8_t parse_decl_start_is(const parse_ctx_t *ctx)
{
    if (ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_KW_CONST) { return LDG_TRUTH_TRUE; }

    if (ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_INT) { return LDG_TRUTH_FALSE; }

    if (ctx->tok.width != 0) { return LDG_TRUTH_FALSE; }

    return crosstalk_eda_script_lex_width_valid_is(ctx->tok.val);
}

static uint32_t parse_range(parse_ctx_t *ctx, crosstalk_eda_script_node_t *decl)
{
    uint32_t ret = 0;

    decl->has_range = LDG_TRUTH_TRUE;
    decl->r_lo_open = ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_LPAREN ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_INT || ctx->tok.width == 0)) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "range bound has no width", "write each bound as a width-prefixed literal, as in 16'0x0"); }

    decl->r_lo = ctx->tok.val;

    if (decl->width == 0) { decl->width = ctx->tok.width; }

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_COMMA)) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "range is missing its comma", "write the range as [lo, hi]"); }

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_INT || ctx->tok.width == 0)) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "range bound has no width", "write each bound as a width-prefixed literal, as in 16'0x3E8"); }

    decl->r_hi = ctx->tok.val;

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_RBRACKET) { decl->r_hi_open = LDG_TRUTH_FALSE; }
    else if (ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_RPAREN) { decl->r_hi_open = LDG_TRUTH_TRUE; }
    else { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "range is not closed", "close the range with a bracket or a parenthesis"); }

    return parse_advance(ctx);
}

static uint32_t parse_decl_init(parse_ctx_t *ctx, crosstalk_eda_script_node_t *decl)
{
    uint32_t ret = 0;

    if (ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_STR)
    {
        crosstalk_eda_script_tok_t str_tok = ctx->tok;

        if (LDG_UNLIKELY(decl->arr_cunt == 0 || decl->width != (uint32_t)PARSE_STR_WIDTH)) { return parse_diag_set(ctx, &str_tok, CROSSTALK_EDA_SCRIPT_ERR_TYPE, "string initializer needs an eight-wide array", "declare the variable as 8 name[len]"); }

        ret = crosstalk_eda_script_ast_str_decode(ctx->ast, str_tok.start, str_tok.len, &decl->bytes, &decl->bytes_len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return parse_diag_set(ctx, &str_tok, ret, "invalid string literal", "use only the escapes n t r backslash quote and zero"); }

        if (LDG_UNLIKELY(decl->bytes_len != decl->arr_cunt)) { return parse_diag_set(ctx, &str_tok, CROSSTALK_EDA_SCRIPT_ERR_WIDTH, "string initializer length does not match the array size", "make the array size equal the string length"); }

        return parse_advance(ctx);
    }

    if (ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_LBRACE && decl->arr_cunt != 0)
    {
        crosstalk_eda_script_tok_t open_tok = ctx->tok;
        crosstalk_eda_script_node_t *list = 0x0;

        ret = parse_node_new(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_INIT_LIST, &open_tok, &list);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = parse_advance(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        while (LDG_TRUTH_TRUE)
        {
            crosstalk_eda_script_node_t *elem = 0x0;

            ret = parse_expr(ctx, &elem);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_eda_script_ast_list_push(&list->list, elem);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_COMMA)
            {
                ret = parse_advance(ctx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                continue;
            }

            break;
        }

        if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_RBRACE)) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "initializer list is not closed", "close the initializer list with a right brace"); }

        if (LDG_UNLIKELY(list->list.cunt != decl->arr_cunt)) { return parse_diag_set(ctx, &open_tok, CROSSTALK_EDA_SCRIPT_ERR_WIDTH, "initializer list length does not match the array size", "give one initializer per array element"); }

        decl->rhs = list;

        return parse_advance(ctx);
    }

    return parse_expr(ctx, &decl->lhs);
}

static uint32_t parse_decl(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out)
{
    crosstalk_eda_script_tok_t start_tok = ctx->tok;
    crosstalk_eda_script_node_t *decl = 0x0;
    uint8_t const_is = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_KW_CONST)
    {
        const_is = LDG_TRUTH_TRUE;

        ret = parse_advance(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_INT || ctx->tok.width != 0 || crosstalk_eda_script_lex_width_valid_is(ctx->tok.val) != LDG_TRUTH_TRUE)) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "declaration is missing its bit width", "declare as width name = value, with width a power of two up to 64"); }

    ret = parse_node_new(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_DECL, &start_tok, &decl);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    decl->is_const = const_is;
    decl->width = (uint32_t)ctx->tok.val;

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_LBRACKET || ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_LPAREN)
    {
        ret = parse_range(ctx, decl);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_IDENT)) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "declaration is missing a variable name", "declare as width name = value"); }

    ret = crosstalk_eda_script_ast_str_copy(ctx->ast, ctx->tok.start, ctx->tok.len, &decl->name);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    decl->name_len = ctx->tok.len;

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_LBRACKET)
    {
        ret = parse_advance(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_INT || ctx->tok.val == 0)) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "array size is not a positive constant", "give the array a constant size greater than zero"); }

        decl->arr_cunt = (uint32_t)ctx->tok.val;

        ret = parse_advance(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_RBRACKET)) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "array size is not closed", "close the array size with a right bracket"); }

        ret = parse_advance(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_ASSIGN)) { return parse_diag_set(ctx, &start_tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "declaration has no initializer", "every variable needs an explicit initial value"); }

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = parse_decl_init(ctx, decl);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = parse_semi_want(ctx, &start_tok);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = decl;

    return LDG_ERR_AOK;
}

static uint32_t parse_guard_tail(parse_ctx_t *ctx, crosstalk_eda_script_node_t **iff_out)
{
    uint32_t ret = 0;

    if (ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_KW_IFF) { return LDG_ERR_AOK; }

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_LPAREN)) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "guard is missing its parentheses", "write the guard as iff (expression)"); }

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = parse_expr(ctx, iff_out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_RPAREN)) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "guard is not closed", "close the guard with a right parenthesis"); }

    return parse_advance(ctx);
}

static uint32_t parse_guard(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out)
{
    crosstalk_eda_script_tok_t start_tok = ctx->tok;
    crosstalk_eda_script_node_t *guard = 0x0;
    uint32_t ret = parse_node_new(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_GUARD, &start_tok, &guard);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    guard->action_call = start_tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_KW_CALL ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_IDENT)) { return parse_diag_set(ctx, &start_tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "action is missing its target", "write jmp section iff (cond) or call sub(args)"); }

    ret = crosstalk_eda_script_ast_str_copy(ctx->ast, ctx->tok.start, ctx->tok.len, &guard->name);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    guard->name_len = ctx->tok.len;

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_LPAREN)
    {
        guard->has_args = LDG_TRUTH_TRUE;

        ret = parse_arg_list(ctx, &guard->list);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = parse_guard_tail(ctx, &guard->lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_BANG)
    {
        ret = parse_advance(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_IDENT)) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "alternative action is missing its target", "write the alternative as ! target"); }

        ret = crosstalk_eda_script_ast_str_copy(ctx->ast, ctx->tok.start, ctx->tok.len, &guard->alt_name);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        guard->alt_name_len = ctx->tok.len;

        ret = parse_advance(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_LPAREN)
        {
            guard->alt_has_args = LDG_TRUTH_TRUE;

            ret = parse_arg_list(ctx, &guard->alt_list);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = parse_guard_tail(ctx, &guard->rhs);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = parse_semi_want(ctx, &start_tok);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = guard;

    return LDG_ERR_AOK;
}

static uint32_t parse_section(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out)
{
    crosstalk_eda_script_tok_t start_tok = ctx->tok;
    crosstalk_eda_script_node_t *section = 0x0;
    uint32_t ret = parse_node_new(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_SECTION, &start_tok, &section);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_IDENT)) { return parse_diag_set(ctx, &start_tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "section is missing a name", "open the section as init name:"); }

    ret = crosstalk_eda_script_ast_str_copy(ctx->ast, ctx->tok.start, ctx->tok.len, &section->name);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    section->name_len = ctx->tok.len;

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_COLON)) { return parse_diag_set(ctx, &start_tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "section name is not followed by a colon", "open the section as init name:"); }

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_KW_RACK)
    {
        crosstalk_eda_script_node_t *stmt = 0x0;

        if (LDG_UNLIKELY(ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_EOF)) { return parse_diag_set(ctx, &start_tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "section is never closed", "close the section with rack and the same name"); }

        ret = parse_stmt(ctx, &stmt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_eda_script_ast_list_push(&section->list, stmt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_IDENT || ctx->tok.len != section->name_len || memcmp(ctx->tok.start, section->name, ctx->tok.len) != 0)) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_SECTION, "term name does not match the section", "close the section with term and the same name"); }

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = parse_semi_want(ctx, &start_tok);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = section;

    return LDG_ERR_AOK;
}

static uint8_t parse_switch_kw_is(uint32_t tok_kind)
{
    if (tok_kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_KW_LEVER) { return LDG_TRUTH_TRUE; }

    if (tok_kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_KW_ROTARY) { return LDG_TRUTH_TRUE; }

    if (tok_kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_KW_ENCODER) { return LDG_TRUTH_TRUE; }

    if (tok_kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_KW_BUTTON) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t parse_switch_kind_get(uint32_t tok_kind)
{
    if (tok_kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_KW_LEVER) { return (uint32_t)CROSSTALK_EDA_SCRIPT_SWITCH_LEVER; }

    if (tok_kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_KW_ROTARY) { return (uint32_t)CROSSTALK_EDA_SCRIPT_SWITCH_ROTARY; }

    if (tok_kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_KW_ENCODER) { return (uint32_t)CROSSTALK_EDA_SCRIPT_SWITCH_ENCODER; }

    return (uint32_t)CROSSTALK_EDA_SCRIPT_SWITCH_BUTTON;
}

static uint32_t parse_switch_notches(parse_ctx_t *ctx, crosstalk_eda_script_node_t *switch_ref)
{
    crosstalk_eda_script_tok_t open_tok = ctx->tok;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_LBRACKET)) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_SWITCH, "rotary has no notch list", "write the notches as [v0, v1, ...] before the name"); }

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (LDG_TRUTH_TRUE)
    {
        crosstalk_eda_script_node_t *notch = 0x0;

        if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_INT)) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_SWITCH, "notch is not a numeric literal", "give every notch as a width-prefixed literal, as in 8'0x1"); }

        if (LDG_UNLIKELY(ctx->tok.width == 0)) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_SWITCH, "notch has no width", "write each notch as a width-prefixed literal, as in 8'0x1"); }

        if (LDG_UNLIKELY(switch_ref->width != 0 && ctx->tok.width != switch_ref->width)) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_SWITCH, "notch widths differ", "give every notch the same width prefix"); }

        switch_ref->width = ctx->tok.width;

        ret = parse_node_new(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_NUM, &ctx->tok, &notch);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        notch->val = ctx->tok.val;
        notch->width = ctx->tok.width;

        ret = crosstalk_eda_script_ast_list_push(&switch_ref->list, notch);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = parse_advance(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_COMMA)
        {
            ret = parse_advance(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            continue;
        }

        break;
    }

    if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_RBRACKET)) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_SWITCH, "notch list is not closed", "close the notch list with a right bracket"); }

    if (LDG_UNLIKELY(switch_ref->list.cunt < (uint32_t)PARSE_NOTCH_MIN)) { return parse_diag_set(ctx, &open_tok, CROSSTALK_EDA_SCRIPT_ERR_SWITCH, "rotary needs at least two notches", "give the rotary two or more notch values"); }

    return parse_advance(ctx);
}

static uint32_t parse_switch(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out)
{
    crosstalk_eda_script_tok_t start_tok = ctx->tok;
    crosstalk_eda_script_node_t *switch_ref = 0x0;
    uint32_t kind = parse_switch_kind_get(start_tok.kind);
    uint32_t ret = parse_node_new(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_SWITCH, &start_tok, &switch_ref);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    switch_ref->op = kind;

    if (kind == (uint32_t)CROSSTALK_EDA_SCRIPT_SWITCH_LEVER || kind == (uint32_t)CROSSTALK_EDA_SCRIPT_SWITCH_BUTTON) { switch_ref->width = 0x1; }

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (kind == (uint32_t)CROSSTALK_EDA_SCRIPT_SWITCH_ENCODER)
    {
        if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_LBRACKET && ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_LPAREN)) { return parse_diag_set(ctx, &start_tok, CROSSTALK_EDA_SCRIPT_ERR_SWITCH, "encoder has no range", "write the sweep as [lo, hi] before the name"); }

        ret = parse_range(ctx, switch_ref);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (kind == (uint32_t)CROSSTALK_EDA_SCRIPT_SWITCH_ROTARY)
    {
        ret = parse_switch_notches(ctx, switch_ref);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_IDENT)) { return parse_diag_set(ctx, &start_tok, CROSSTALK_EDA_SCRIPT_ERR_SWITCH, "switch is missing a name", "name the switch after its kind and geometry"); }

    ret = crosstalk_eda_script_ast_str_copy(ctx->ast, ctx->tok.start, ctx->tok.len, &switch_ref->name);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    switch_ref->name_len = ctx->tok.len;

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (kind != (uint32_t)CROSSTALK_EDA_SCRIPT_SWITCH_BUTTON)
    {
        if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_ASSIGN)) { return parse_diag_set(ctx, &start_tok, CROSSTALK_EDA_SCRIPT_ERR_SWITCH, "switch has no initial position", "give the switch an initial position with = value"); }

        ret = parse_advance(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_INT)) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_SWITCH, "initial position is not a numeric literal", "initialize the switch with a plain numeric literal"); }

        switch_ref->val = ctx->tok.val;

        ret = parse_advance(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_LBRACE)
    {
        const char *body_start = ctx->tok.start;

        ret = parse_stmt(ctx, &switch_ref->third);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(ctx->tok.start == 0x0 || ctx->tok.start < body_start)) { return parse_diag_set(ctx, &start_tok, CROSSTALK_EDA_SCRIPT_ERR_SWITCH, "switch action span cannot be captured", "close the rack with term and the same name"); }

        switch_ref->bytes = body_start;
        switch_ref->bytes_len = (uint32_t)(ctx->tok.start - body_start);

        *out = switch_ref;

        return LDG_ERR_AOK;
    }

    ret = parse_semi_want(ctx, &start_tok);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = switch_ref;

    return LDG_ERR_AOK;
}

static uint32_t parse_rack(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out)
{
    crosstalk_eda_script_tok_t start_tok = ctx->tok;
    crosstalk_eda_script_node_t *rack = 0x0;
    uint32_t ret = parse_node_new(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_RACK, &start_tok, &rack);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_IDENT)) { return parse_diag_set(ctx, &start_tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "rack is missing a name", "open the rack as rack name:"); }

    ret = crosstalk_eda_script_ast_str_copy(ctx->ast, ctx->tok.start, ctx->tok.len, &rack->name);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    rack->name_len = ctx->tok.len;

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_COLON)) { return parse_diag_set(ctx, &start_tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "rack name is not followed by a colon", "open the rack as rack name:"); }

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_KW_RACK)
    {
        crosstalk_eda_script_node_t *switch_ref = 0x0;

        if (LDG_UNLIKELY(ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_EOF)) { return parse_diag_set(ctx, &start_tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "rack is never closed", "close the rack with rack and the same name"); }

        if (LDG_UNLIKELY(parse_switch_kw_is(ctx->tok.kind) != LDG_TRUTH_TRUE)) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_SWITCH, "only switches belong in a rack", "declare lever rotary encoder or button controls"); }

        ret = parse_switch(ctx, &switch_ref);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_eda_script_ast_list_push(&rack->list, switch_ref);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_IDENT || ctx->tok.len != rack->name_len || memcmp(ctx->tok.start, rack->name, ctx->tok.len) != 0)) { return parse_diag_set(ctx, &ctx->tok, CROSSTALK_EDA_SCRIPT_ERR_SECTION, "term name does not match the rack", "close the rack with term and the same name"); }

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = parse_semi_want(ctx, &start_tok);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = rack;

    return LDG_ERR_AOK;
}

static uint8_t parse_cmd_arg_char_is(char ch)
{
    if (ldg_char_alnum_is(ch) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    if (ch == ';') { return LDG_TRUTH_TRUE; }

    if (ch == LDG_STR_TERM || ch == '\n') { return LDG_TRUTH_TRUE; }

    if (ch == '_') { return LDG_TRUTH_TRUE; }

    if (ch == '"') { return LDG_TRUTH_TRUE; }

    if (ch == '\'') { return LDG_TRUTH_TRUE; }

    if (ch == '\\') { return LDG_TRUTH_TRUE; }

    if (ch == '@') { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t parse_cmd_path_start_is(const parse_ctx_t *ctx, char next)
{
    char gap = LDG_STR_TERM;

    if (next != '.' && next != '/' && next != '~') { return LDG_TRUTH_FALSE; }

    if (ctx->lex->at >= ctx->lex->src_len) { return LDG_TRUTH_FALSE; }

    gap = ctx->lex->src[ctx->lex->at];

    return (gap == ' ' || gap == '\t') ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint8_t parse_cmd_start_is(const parse_ctx_t *ctx, char next)
{
    if (ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_IDENT) { return LDG_TRUTH_FALSE; }

    if (parse_cmd_path_start_is(ctx, next) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return parse_cmd_arg_char_is(next);
}

static uint32_t parse_cmd(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out)
{
    crosstalk_eda_script_tok_t start_tok = ctx->tok;
    crosstalk_eda_script_node_t *call = 0x0;
    uint32_t ret = parse_node_new(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_CALL, &start_tok, &call);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_eda_script_ast_str_copy(ctx->ast, start_tok.start, start_tok.len, &call->name);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    call->name_len = start_tok.len;

    while (LDG_TRUTH_TRUE)
    {
        crosstalk_eda_script_node_t *arg = 0x0;
        const char *word = 0x0;
        uint32_t word_len = 0;
        uint8_t word_quoted = LDG_TRUTH_FALSE;

        ret = crosstalk_eda_script_lex_word_next(ctx->lex, &word, &word_len, &word_quoted);
        if (ret == LDG_ERR_EOF) { break; }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return parse_diag_set(ctx, &start_tok, ret, "command argument is malformed", "quote arguments that contain spaces"); }

        ret = parse_node_new(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_STR, &start_tok, &arg);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_eda_script_ast_str_copy(ctx->ast, word, word_len, &arg->bytes);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        arg->bytes_len = word_len;
        arg->width = (uint32_t)PARSE_STR_WIDTH;
        arg->quoted = word_quoted;

        ret = crosstalk_eda_script_ast_list_push(&call->list, arg);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = parse_semi_want(ctx, &start_tok);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = call;

    return LDG_ERR_AOK;
}

static uint32_t parse_step_stmt(parse_ctx_t *ctx, crosstalk_eda_script_node_t *seed, const crosstalk_eda_script_tok_t *start_tok, crosstalk_eda_script_node_t **out)
{
    crosstalk_eda_script_tok_t step_tok = ctx->tok;
    crosstalk_eda_script_node_t *step = 0x0;
    uint32_t kind = step_tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_PLUS ? (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_INC : (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_DEC;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(parse_lvalue_is(seed) != LDG_TRUTH_TRUE)) { return parse_diag_set(ctx, &step_tok, CROSSTALK_EDA_SCRIPT_ERR_LVALUE, "increment target cannot be assigned to", "increment a variable or an array element"); }

    ret = parse_node_new(ctx, kind, &step_tok, &step);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    step->lhs = seed;

    ret = parse_advance(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = parse_semi_want(ctx, start_tok);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = step;

    return LDG_ERR_AOK;
}

static uint32_t parse_stmt(parse_ctx_t *ctx, crosstalk_eda_script_node_t **out)
{
    crosstalk_eda_script_tok_t start_tok = ctx->tok;
    crosstalk_eda_script_lex_t save_lex = LDG_STRUCT_ZERO_INIT;
    char next_ch = 0;
    crosstalk_eda_script_node_t *expr = 0x0;
    crosstalk_eda_script_node_t *stmt = 0x0;
    uint32_t ret = 0;

    if (parse_decl_start_is(ctx) == LDG_TRUTH_TRUE) { return parse_decl(ctx, out); }

    if (ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_KW_INIT) { return parse_section(ctx, out); }

    if (ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_KW_RACK) { return parse_rack(ctx, out); }

    if (LDG_UNLIKELY(parse_switch_kw_is(ctx->tok.kind) == LDG_TRUTH_TRUE)) { return parse_diag_set(ctx, &start_tok, CROSSTALK_EDA_SCRIPT_ERR_SWITCH, "switches live inside racks", "wrap the switch in rack name: and term name;"); }

    if (ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_KW_JMP) { return parse_guard(ctx, out); }

    if (ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_KW_CALL) { return parse_guard(ctx, out); }

    if (ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_KW_RET)
    {
        ret = parse_node_new(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_RET, &start_tok, out);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = parse_advance(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        return parse_semi_want(ctx, &start_tok);
    }

    if (ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_LBRACE)
    {
        ret = parse_node_new(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_BLK, &start_tok, &stmt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = parse_advance(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        while (ctx->tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_RBRACE)
        {
            crosstalk_eda_script_node_t *child = 0x0;

            if (LDG_UNLIKELY(ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_EOF)) { return parse_diag_set(ctx, &start_tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "compound statement is never closed", "close the compound statement with a right brace"); }

            ret = parse_stmt(ctx, &child);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_eda_script_ast_list_push(&stmt->list, child);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = parse_advance(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        *out = stmt;

        return LDG_ERR_AOK;
    }

    if (LDG_UNLIKELY(ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_SEMI)) { return parse_diag_set(ctx, &start_tok, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "statement is empty", "remove the stray semicolon"); }

    ret = crosstalk_eda_script_lex_raw_peek(ctx->lex, &next_ch);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return parse_diag_set(ctx, &start_tok, ret, "invalid token", "remove or correct the flagged characters"); }

    if (parse_cmd_start_is(ctx, next_ch) == LDG_TRUTH_TRUE) { return parse_cmd(ctx, out); }

    save_lex = *ctx->lex;

    ret = parse_postfix(ctx, &expr);
    if (ret == LDG_ERR_AOK && (ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_PLUS || ctx->tok.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_MINUS))
    {
        crosstalk_eda_script_tok_t after = LDG_STRUCT_ZERO_INIT;

        ret = crosstalk_eda_script_lex_peek(ctx->lex, &after);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return parse_diag_set(ctx, &start_tok, ret, "invalid token", "remove or correct the flagged characters"); }

        if (after.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_SEMI) { return parse_step_stmt(ctx, expr, &start_tok, out); }
    }

    *ctx->lex = save_lex;
    ctx->tok = start_tok;
    expr = 0x0;

    ret = parse_expr(ctx, &expr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = parse_node_new(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_EXPR_STMT, &start_tok, &stmt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    stmt->lhs = expr;

    ret = parse_semi_want(ctx, &start_tok);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = stmt;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_script_parse_program(crosstalk_eda_script_lex_t *lex, crosstalk_eda_script_ast_t *ast, crosstalk_eda_script_node_t **out, crosstalk_eda_script_diag_t *diag)
{
    parse_ctx_t ctx = LDG_STRUCT_ZERO_INIT;
    crosstalk_eda_script_node_t *program = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(lex == 0x0 || ast == 0x0 || out == 0x0 || diag == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;
    memset(diag, 0x0, sizeof(*diag));

    ctx.lex = lex;
    ctx.ast = ast;
    ctx.diag = diag;

    ret = parse_advance(&ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_eda_script_ast_node_new(ast, (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_PROGRAM, &ctx.tok, &program);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (ctx.tok.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_EOF)
    {
        crosstalk_eda_script_node_t *stmt = 0x0;

        ret = parse_stmt(&ctx, &stmt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_eda_script_ast_list_push(&program->list, stmt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    *out = program;

    return LDG_ERR_AOK;
}
