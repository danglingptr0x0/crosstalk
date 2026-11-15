#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include <dangling/core/arith.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/secure.h>
#include <dangling/str/str.h>

#include "re/repl/expr.h"

#define EXPR_STEP 1
#define EXPR_EXACT_REAL_MAX 9007199254740992ULL
#define EXPR_REAL_NONE 0.0
#define EXPR_BYTES_BYTE 1
#define EXPR_BYTES_WORD 2
#define EXPR_BYTES_DWORD 4
#define EXPR_BYTES_QWORD 8
#define EXPR_FP32_FRACTION_BITS 23
#define EXPR_FP32_EXPONENT_MASK 0xFF
#define EXPR_FP32_FRACTION_MASK 0x7FFFFFULL
#define EXPR_FP32_IMPLICIT_BIT 0x800000ULL
#define EXPR_FP32_SIGN_BIT 0x80000000ULL
#define EXPR_FP32_SCALED_BIAS 150
#define EXPR_FP32_SUBNORMAL_EXPONENT (-149)
#define EXPR_BIT_LOW 1ULL
#define EXPR_PREC_NONE 0
#define EXPR_PREC_OR 1
#define EXPR_PREC_AND 2
#define EXPR_PREC_CMP 3
#define EXPR_PREC_ADD 4
#define EXPR_PREC_MUL 5
#define EXPR_PREC_UNARY 6
#define EXPR_PREC_POW 7
#define EXPR_OP_LEN_CH 1
#define EXPR_OP_LEN_DIGRAPH 2
#define EXPR_HEX_PREFIX_LEN 2
#define EXPR_DEPTH_ROOT 0
#define EXPR_DEPTH_LEAF 1
#define EXPR_CH_DIGIT_FIRST '0'
#define EXPR_CH_HEX_LOWER 'x'
#define EXPR_CH_HEX_UPPER 'X'
#define EXPR_CH_POINT '.'
#define EXPR_CH_EXP_LOWER 'e'
#define EXPR_CH_EXP_UPPER 'E'
#define EXPR_CH_PLUS '+'
#define EXPR_CH_MINUS '-'
#define EXPR_CH_NOT '!'
#define EXPR_CH_OPEN '('
#define EXPR_CH_CLOSE ')'
#define EXPR_CH_COMMA ','
#define EXPR_CH_PIPE '|'
#define EXPR_CH_AMP '&'
#define EXPR_CH_EQ '='
#define EXPR_CH_LT '<'
#define EXPR_CH_GT '>'
#define EXPR_CH_STAR '*'
#define EXPR_CH_SLASH '/'
#define EXPR_CH_PERCENT '%'
#define EXPR_CH_CARET '^'
#define EXPR_WORD_V "v"
#define EXPR_WORD_P "p"
#define EXPR_WORD_B "b"
#define EXPR_WORD_D "d"
#define EXPR_WORD_T "t"
#define EXPR_WORD_DT "dt"
#define EXPR_WORD_N "n"
#define EXPR_WORD_ABS "abs"
#define EXPR_WORD_SIN "sin"
#define EXPR_WORD_COS "cos"
#define EXPR_WORD_SQRT "sqrt"
#define EXPR_WORD_FLOOR "floor"
#define EXPR_WORD_MIN "min"
#define EXPR_WORD_MAX "max"

typedef enum expr_op
{
    EXPR_OP_LITERAL = 0,
    EXPR_OP_VAR,
    EXPR_OP_NEG,
    EXPR_OP_NOT,
    EXPR_OP_ABS,
    EXPR_OP_SIN,
    EXPR_OP_COS,
    EXPR_OP_SQRT,
    EXPR_OP_FLOOR,
    EXPR_OP_MIN,
    EXPR_OP_MAX,
    EXPR_OP_ADD,
    EXPR_OP_SUB,
    EXPR_OP_MUL,
    EXPR_OP_DIV,
    EXPR_OP_MOD,
    EXPR_OP_POW,
    EXPR_OP_LT,
    EXPR_OP_LE,
    EXPR_OP_GT,
    EXPR_OP_GE,
    EXPR_OP_EQ,
    EXPR_OP_NE,
    EXPR_OP_AND,
    EXPR_OP_OR
} expr_op_t;

uint8_t crosstalk_re_repl_expr_truth_is(const crosstalk_re_repl_expr_val_t *val)
{
    if (LDG_UNLIKELY(val == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (val->fp == LDG_TRUTH_TRUE) { return val->real != EXPR_REAL_NONE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    return val->mag != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_re_repl_expr_real_get(const crosstalk_re_repl_expr_val_t *val, double *out)
{
    if (LDG_UNLIKELY(val == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = EXPR_REAL_NONE;

    if (val->fp != LDG_TRUTH_TRUE && val->mag > EXPR_EXACT_REAL_MAX) { return LDG_ERR_RANGE; }

    if (val->fp == LDG_TRUTH_TRUE) { *out = val->real; }
    else { *out = val->neg == LDG_TRUTH_TRUE ? -(double)val->mag : (double)val->mag; }

    return isfinite(*out) ? LDG_ERR_AOK : LDG_ERR_RANGE;
}

static uint32_t expr_fp32_decode(uint64_t bits, double *out)
{
    uint64_t fraction = bits & EXPR_FP32_FRACTION_MASK;
    uint32_t exponent = (uint32_t)((bits >> EXPR_FP32_FRACTION_BITS) & (uint64_t)EXPR_FP32_EXPONENT_MASK);

    if (exponent == (uint32_t)EXPR_FP32_EXPONENT_MASK) { return LDG_ERR_RANGE; }

    if (exponent != 0) { *out = ldexp((double)(fraction | EXPR_FP32_IMPLICIT_BIT), (int32_t)exponent - EXPR_FP32_SCALED_BIAS); }
    else { *out = ldexp((double)fraction, EXPR_FP32_SUBNORMAL_EXPONENT); }

    if ((bits & EXPR_FP32_SIGN_BIT) != 0) { *out = -*out; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_repl_expr_decode(uint64_t bits, uint32_t bytes, uint8_t fp, uint8_t signed_val, crosstalk_re_repl_expr_val_t *out)
{
    uint64_t sign = 0;
    uint64_t mask = UINT64_MAX;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_secure_zero(out, (uint64_t)sizeof(*out));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (bytes != (uint32_t)EXPR_BYTES_BYTE && bytes != (uint32_t)EXPR_BYTES_WORD && bytes != (uint32_t)EXPR_BYTES_DWORD && bytes != (uint32_t)EXPR_BYTES_QWORD) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (bytes < (uint32_t)EXPR_BYTES_QWORD) { mask = (EXPR_BIT_LOW << (bytes * (uint32_t)LDG_BYTE_BITS)) - EXPR_BIT_LOW; }

    bits = bits & mask;

    if (fp != LDG_TRUTH_TRUE)
    {
        sign = EXPR_BIT_LOW << (bytes * (uint32_t)LDG_BYTE_BITS - (uint32_t)EXPR_STEP);
        out->neg = signed_val == LDG_TRUTH_TRUE && (bits & sign) != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
        out->mag = out->neg == LDG_TRUTH_TRUE ? (~bits & mask) + EXPR_BIT_LOW : bits;

        return LDG_ERR_AOK;
    }

    out->fp = LDG_TRUTH_TRUE;

    if (bytes == (uint32_t)EXPR_BYTES_QWORD) { ret = ldg_mem_secure_cpy(&out->real, &bits, (uint64_t)sizeof(bits)); }
    else if (bytes == (uint32_t)EXPR_BYTES_DWORD) { ret = expr_fp32_decode(bits, &out->real); }
    else { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ret != LDG_ERR_AOK) { return ret; }

    return isfinite(out->real) ? LDG_ERR_AOK : LDG_ERR_RANGE;
}

static void expr_space_skip(crosstalk_re_repl_expr_t *expr)
{
    while (expr->text[expr->cursor] == LDG_ASCII_SPACE || expr->text[expr->cursor] == LDG_ASCII_TAB) { expr->cursor = expr->cursor + (uint32_t)EXPR_STEP; }
}

static void expr_digits_skip(crosstalk_re_repl_expr_t *expr)
{
    while (ldg_char_digit_is((uint8_t)expr->text[expr->cursor]) == LDG_TRUTH_TRUE) { expr->cursor = expr->cursor + (uint32_t)EXPR_STEP; }
}

static uint8_t expr_word_is(const char *text, uint32_t len, const char *word)
{
    uint32_t at = 0;

    for (at = 0; at < len; at = at + (uint32_t)EXPR_STEP) { if (word[at] == LDG_STR_TERM || text[at] != word[at]) { return LDG_TRUTH_FALSE; } }

    return word[len] == LDG_STR_TERM ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t expr_exponent_skip(crosstalk_re_repl_expr_t *expr)
{
    expr->cursor = expr->cursor + (uint32_t)EXPR_STEP;

    if (expr->text[expr->cursor] == EXPR_CH_PLUS || expr->text[expr->cursor] == EXPR_CH_MINUS) { expr->cursor = expr->cursor + (uint32_t)EXPR_STEP; }

    if (ldg_char_digit_is((uint8_t)expr->text[expr->cursor]) != LDG_TRUTH_TRUE) { return LDG_ERR_STR_INVALID; }

    expr_digits_skip(expr);

    return LDG_ERR_AOK;
}

static uint32_t expr_real_parse(crosstalk_re_repl_expr_t *expr, uint32_t start, crosstalk_re_repl_expr_val_t *out)
{
    char *end = 0x0;

    errno = 0;
    out->real = strtod(expr->text + start, &end);
    out->fp = LDG_TRUTH_TRUE;

    if (errno == ERANGE || end != expr->text + expr->cursor || !isfinite(out->real)) { return LDG_ERR_RANGE; }

    return LDG_ERR_AOK;
}

static uint32_t expr_number_parse(crosstalk_re_repl_expr_t *expr, crosstalk_re_repl_expr_val_t *out)
{
    uint32_t start = expr->cursor;
    uint32_t ret = 0;
    uint8_t fp = LDG_TRUTH_FALSE;

    if (expr->text[expr->cursor] == EXPR_CH_DIGIT_FIRST && (expr->text[expr->cursor + (uint32_t)EXPR_STEP] == EXPR_CH_HEX_LOWER || expr->text[expr->cursor + (uint32_t)EXPR_STEP] == EXPR_CH_HEX_UPPER))
    {
        expr->cursor = expr->cursor + (uint32_t)EXPR_HEX_PREFIX_LEN;
        start = expr->cursor;

        while (ldg_char_hex_is((uint8_t)expr->text[expr->cursor]) == LDG_TRUTH_TRUE) { expr->cursor = expr->cursor + (uint32_t)EXPR_STEP; }

        if (expr->cursor == start) { return LDG_ERR_STR_INVALID; }

        return ldg_str_uint_parse(expr->text + start, LDG_BASE_HEX, expr->cursor - start, &out->mag);
    }

    expr_digits_skip(expr);

    if (expr->text[expr->cursor] == EXPR_CH_POINT)
    {
        fp = LDG_TRUTH_TRUE;
        expr->cursor = expr->cursor + (uint32_t)EXPR_STEP;
        expr_digits_skip(expr);
    }

    if (expr->text[expr->cursor] == EXPR_CH_EXP_LOWER || expr->text[expr->cursor] == EXPR_CH_EXP_UPPER)
    {
        fp = LDG_TRUTH_TRUE;

        ret = expr_exponent_skip(expr);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    if (expr->cursor == start) { return LDG_ERR_STR_INVALID; }

    if (fp != LDG_TRUTH_TRUE) { return ldg_str_uint_parse(expr->text + start, LDG_BASE_DEC, expr->cursor - start, &out->mag); }

    return expr_real_parse(expr, start, out);
}

static uint32_t expr_node_add(crosstalk_re_repl_expr_t *expr, uint32_t op, uint32_t lhs, uint32_t rhs, uint32_t *out)
{
    uint32_t depth = (uint32_t)EXPR_DEPTH_LEAF;

    if (expr->cunt == (uint32_t)CROSSTALK_RE_REPL_EXPR_NODE_CAP) { return LDG_ERR_FULL; }

    if (lhs != CROSSTALK_RE_REPL_EXPR_NODE_NONE) { depth = expr->node[lhs].depth + (uint32_t)EXPR_STEP; }

    if (rhs != CROSSTALK_RE_REPL_EXPR_NODE_NONE && expr->node[rhs].depth >= depth) { depth = expr->node[rhs].depth + (uint32_t)EXPR_STEP; }

    if (depth > (uint32_t)CROSSTALK_RE_REPL_EXPR_DEPTH_CAP) { return LDG_ERR_FULL; }

    *out = expr->cunt;
    expr->cunt = expr->cunt + (uint32_t)EXPR_STEP;
    expr->node[*out].op = op;
    expr->node[*out].lhs = lhs;
    expr->node[*out].rhs = rhs;
    expr->node[*out].depth = depth;

    return LDG_ERR_AOK;
}

static void expr_op_set(uint32_t *op, uint32_t *prec, uint32_t *len, uint32_t op_val, uint32_t prec_val, uint32_t len_val)
{
    *op = op_val;
    *prec = prec_val;
    *len = len_val;
}

static void expr_op_get(const char *text, uint32_t *op, uint32_t *prec, uint32_t *len)
{
    uint8_t pair_eq = text[EXPR_STEP] == EXPR_CH_EQ ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    uint32_t cmp_len = pair_eq == LDG_TRUTH_TRUE ? (uint32_t)EXPR_OP_LEN_DIGRAPH : (uint32_t)EXPR_OP_LEN_CH;

    *prec = (uint32_t)EXPR_PREC_NONE;
    *len = (uint32_t)EXPR_OP_LEN_CH;

    if (text[0] == EXPR_CH_PIPE && text[EXPR_STEP] == EXPR_CH_PIPE) { expr_op_set(op, prec, len, (uint32_t)EXPR_OP_OR, (uint32_t)EXPR_PREC_OR, (uint32_t)EXPR_OP_LEN_DIGRAPH); }
    else if (text[0] == EXPR_CH_AMP && text[EXPR_STEP] == EXPR_CH_AMP) { expr_op_set(op, prec, len, (uint32_t)EXPR_OP_AND, (uint32_t)EXPR_PREC_AND, (uint32_t)EXPR_OP_LEN_DIGRAPH); }
    else if (text[0] == EXPR_CH_EQ && pair_eq == LDG_TRUTH_TRUE) { expr_op_set(op, prec, len, (uint32_t)EXPR_OP_EQ, (uint32_t)EXPR_PREC_CMP, (uint32_t)EXPR_OP_LEN_DIGRAPH); }
    else if (text[0] == EXPR_CH_NOT && pair_eq == LDG_TRUTH_TRUE) { expr_op_set(op, prec, len, (uint32_t)EXPR_OP_NE, (uint32_t)EXPR_PREC_CMP, (uint32_t)EXPR_OP_LEN_DIGRAPH); }
    else if (text[0] == EXPR_CH_LT) { expr_op_set(op, prec, len, pair_eq == LDG_TRUTH_TRUE ? (uint32_t)EXPR_OP_LE : (uint32_t)EXPR_OP_LT, (uint32_t)EXPR_PREC_CMP, cmp_len); }
    else if (text[0] == EXPR_CH_GT) { expr_op_set(op, prec, len, pair_eq == LDG_TRUTH_TRUE ? (uint32_t)EXPR_OP_GE : (uint32_t)EXPR_OP_GT, (uint32_t)EXPR_PREC_CMP, cmp_len); }
    else if (text[0] == EXPR_CH_PLUS) { expr_op_set(op, prec, len, (uint32_t)EXPR_OP_ADD, (uint32_t)EXPR_PREC_ADD, (uint32_t)EXPR_OP_LEN_CH); }
    else if (text[0] == EXPR_CH_MINUS) { expr_op_set(op, prec, len, (uint32_t)EXPR_OP_SUB, (uint32_t)EXPR_PREC_ADD, (uint32_t)EXPR_OP_LEN_CH); }
    else if (text[0] == EXPR_CH_STAR) { expr_op_set(op, prec, len, (uint32_t)EXPR_OP_MUL, (uint32_t)EXPR_PREC_MUL, (uint32_t)EXPR_OP_LEN_CH); }
    else if (text[0] == EXPR_CH_SLASH) { expr_op_set(op, prec, len, (uint32_t)EXPR_OP_DIV, (uint32_t)EXPR_PREC_MUL, (uint32_t)EXPR_OP_LEN_CH); }
    else if (text[0] == EXPR_CH_PERCENT) { expr_op_set(op, prec, len, (uint32_t)EXPR_OP_MOD, (uint32_t)EXPR_PREC_MUL, (uint32_t)EXPR_OP_LEN_CH); }
    else if (text[0] == EXPR_CH_CARET) { expr_op_set(op, prec, len, (uint32_t)EXPR_OP_POW, (uint32_t)EXPR_PREC_POW, (uint32_t)EXPR_OP_LEN_CH); }
}

static uint32_t expr_var_find(const char *text, uint32_t len)
{
    if (expr_word_is(text, len, EXPR_WORD_V) == LDG_TRUTH_TRUE) { return (uint32_t)CROSSTALK_RE_REPL_EXPR_VAR_V; }

    if (expr_word_is(text, len, EXPR_WORD_P) == LDG_TRUTH_TRUE) { return (uint32_t)CROSSTALK_RE_REPL_EXPR_VAR_P; }

    if (expr_word_is(text, len, EXPR_WORD_B) == LDG_TRUTH_TRUE) { return (uint32_t)CROSSTALK_RE_REPL_EXPR_VAR_B; }

    if (expr_word_is(text, len, EXPR_WORD_D) == LDG_TRUTH_TRUE) { return (uint32_t)CROSSTALK_RE_REPL_EXPR_VAR_D; }

    if (expr_word_is(text, len, EXPR_WORD_T) == LDG_TRUTH_TRUE) { return (uint32_t)CROSSTALK_RE_REPL_EXPR_VAR_T; }

    if (expr_word_is(text, len, EXPR_WORD_DT) == LDG_TRUTH_TRUE) { return (uint32_t)CROSSTALK_RE_REPL_EXPR_VAR_DT; }

    if (expr_word_is(text, len, EXPR_WORD_N) == LDG_TRUTH_TRUE) { return (uint32_t)CROSSTALK_RE_REPL_EXPR_VAR_N; }

    return UINT32_MAX;
}

static uint32_t expr_func_find(const char *text, uint32_t len)
{
    if (expr_word_is(text, len, EXPR_WORD_ABS) == LDG_TRUTH_TRUE) { return (uint32_t)EXPR_OP_ABS; }

    if (expr_word_is(text, len, EXPR_WORD_SIN) == LDG_TRUTH_TRUE) { return (uint32_t)EXPR_OP_SIN; }

    if (expr_word_is(text, len, EXPR_WORD_COS) == LDG_TRUTH_TRUE) { return (uint32_t)EXPR_OP_COS; }

    if (expr_word_is(text, len, EXPR_WORD_SQRT) == LDG_TRUTH_TRUE) { return (uint32_t)EXPR_OP_SQRT; }

    if (expr_word_is(text, len, EXPR_WORD_FLOOR) == LDG_TRUTH_TRUE) { return (uint32_t)EXPR_OP_FLOOR; }

    if (expr_word_is(text, len, EXPR_WORD_MIN) == LDG_TRUTH_TRUE) { return (uint32_t)EXPR_OP_MIN; }

    if (expr_word_is(text, len, EXPR_WORD_MAX) == LDG_TRUTH_TRUE) { return (uint32_t)EXPR_OP_MAX; }

    return UINT32_MAX;
}

static uint32_t expr_func_parse(crosstalk_re_repl_expr_t *expr, uint32_t op, uint32_t depth, uint32_t *out)
{
    uint32_t lhs = CROSSTALK_RE_REPL_EXPR_NODE_NONE;
    uint32_t rhs = CROSSTALK_RE_REPL_EXPR_NODE_NONE;
    uint32_t ret = 0;

    expr_space_skip(expr);

    if (expr->text[expr->cursor] != EXPR_CH_OPEN) { return LDG_ERR_STR_INVALID; }

    expr->cursor = expr->cursor + (uint32_t)EXPR_STEP;

    ret = crosstalk_re_repl_expr_node_parse(expr, (uint32_t)EXPR_PREC_OR, depth + (uint32_t)EXPR_STEP, &lhs);
    if (ret != LDG_ERR_AOK) { return ret; }

    expr_space_skip(expr);

    if (op == (uint32_t)EXPR_OP_MIN || op == (uint32_t)EXPR_OP_MAX)
    {
        if (expr->text[expr->cursor] != EXPR_CH_COMMA) { return LDG_ERR_STR_INVALID; }

        expr->cursor = expr->cursor + (uint32_t)EXPR_STEP;

        ret = crosstalk_re_repl_expr_node_parse(expr, (uint32_t)EXPR_PREC_OR, depth + (uint32_t)EXPR_STEP, &rhs);
        if (ret != LDG_ERR_AOK) { return ret; }

        expr_space_skip(expr);
    }

    if (expr->text[expr->cursor] != EXPR_CH_CLOSE) { return LDG_ERR_STR_INVALID; }

    expr->cursor = expr->cursor + (uint32_t)EXPR_STEP;

    return expr_node_add(expr, op, lhs, rhs, out);
}

static uint32_t expr_word_parse(crosstalk_re_repl_expr_t *expr, uint32_t depth, uint32_t *out)
{
    uint32_t start = expr->cursor;
    uint32_t len = 0;
    uint32_t var = 0;
    uint32_t op = 0;
    uint32_t ret = 0;

    while (ldg_char_alpha_is((uint8_t)expr->text[expr->cursor]) == LDG_TRUTH_TRUE) { expr->cursor = expr->cursor + (uint32_t)EXPR_STEP; }

    len = expr->cursor - start;
    if (len == 0) { return LDG_ERR_STR_INVALID; }

    var = expr_var_find(expr->text + start, len);
    if (var != UINT32_MAX)
    {
        ret = expr_node_add(expr, (uint32_t)EXPR_OP_VAR, CROSSTALK_RE_REPL_EXPR_NODE_NONE, CROSSTALK_RE_REPL_EXPR_NODE_NONE, out);
        if (ret == LDG_ERR_AOK) { expr->node[*out].val.mag = (uint64_t)var; }

        return ret;
    }

    op = expr_func_find(expr->text + start, len);
    if (op == UINT32_MAX) { return LDG_ERR_NOT_FOUND; }

    return expr_func_parse(expr, op, depth, out);
}

static uint32_t expr_primary_parse(crosstalk_re_repl_expr_t *expr, uint32_t depth, uint32_t *out)
{
    char ch = expr->text[expr->cursor];
    uint32_t ret = 0;

    if (ch == EXPR_CH_PLUS || ch == EXPR_CH_MINUS || ch == EXPR_CH_NOT)
    {
        expr->cursor = expr->cursor + (uint32_t)EXPR_STEP;

        ret = crosstalk_re_repl_expr_node_parse(expr, (uint32_t)EXPR_PREC_UNARY, depth + (uint32_t)EXPR_STEP, out);
        if (ret != LDG_ERR_AOK || ch == EXPR_CH_PLUS) { return ret; }

        return expr_node_add(expr, ch == EXPR_CH_MINUS ? (uint32_t)EXPR_OP_NEG : (uint32_t)EXPR_OP_NOT, *out, CROSSTALK_RE_REPL_EXPR_NODE_NONE, out);
    }

    if (ch == EXPR_CH_OPEN)
    {
        expr->cursor = expr->cursor + (uint32_t)EXPR_STEP;

        ret = crosstalk_re_repl_expr_node_parse(expr, (uint32_t)EXPR_PREC_OR, depth + (uint32_t)EXPR_STEP, out);
        if (ret != LDG_ERR_AOK) { return ret; }

        expr_space_skip(expr);

        if (expr->text[expr->cursor] != EXPR_CH_CLOSE) { return LDG_ERR_STR_INVALID; }

        expr->cursor = expr->cursor + (uint32_t)EXPR_STEP;

        return LDG_ERR_AOK;
    }

    if (ldg_char_digit_is((uint8_t)ch) == LDG_TRUTH_TRUE || ch == EXPR_CH_POINT)
    {
        ret = expr_node_add(expr, (uint32_t)EXPR_OP_LITERAL, CROSSTALK_RE_REPL_EXPR_NODE_NONE, CROSSTALK_RE_REPL_EXPR_NODE_NONE, out);
        if (ret != LDG_ERR_AOK) { return ret; }

        return expr_number_parse(expr, &expr->node[*out].val);
    }

    return expr_word_parse(expr, depth, out);
}

uint32_t crosstalk_re_repl_expr_node_parse(crosstalk_re_repl_expr_t *expr, uint32_t min_prec, uint32_t depth, uint32_t *out)
{
    uint32_t lhs = CROSSTALK_RE_REPL_EXPR_NODE_NONE;
    uint32_t rhs = CROSSTALK_RE_REPL_EXPR_NODE_NONE;
    uint32_t op = (uint32_t)EXPR_OP_LITERAL;
    uint32_t prec = 0;
    uint32_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(expr == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (depth > (uint32_t)CROSSTALK_RE_REPL_EXPR_DEPTH_CAP) { return LDG_ERR_FULL; }

    expr_space_skip(expr);

    ret = expr_primary_parse(expr, depth, &lhs);
    if (ret != LDG_ERR_AOK) { return ret; }

    while (expr->text[expr->cursor] != LDG_STR_TERM)
    {
        expr_space_skip(expr);
        expr_op_get(expr->text + expr->cursor, &op, &prec, &len);

        if (prec < min_prec) { break; }

        expr->cursor = expr->cursor + len;

        ret = crosstalk_re_repl_expr_node_parse(expr, op == (uint32_t)EXPR_OP_POW ? prec : prec + (uint32_t)EXPR_STEP, depth + (uint32_t)EXPR_STEP, &rhs);
        if (ret != LDG_ERR_AOK) { return ret; }

        ret = expr_node_add(expr, op, lhs, rhs, &lhs);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    *out = lhs;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_repl_expr_compile(crosstalk_re_repl_expr_t *expr, const char *text)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(expr == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_secure_zero(expr, (uint64_t)sizeof(*expr));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_str_rbrcpy(expr->text, text, (uint64_t)sizeof(expr->text));
    if (ret == LDG_ERR_AOK) { ret = crosstalk_re_repl_expr_node_parse(expr, (uint32_t)EXPR_PREC_OR, (uint32_t)EXPR_DEPTH_ROOT, &expr->root); }

    expr_space_skip(expr);

    if (ret == LDG_ERR_AOK && expr->text[expr->cursor] != LDG_STR_TERM) { ret = LDG_ERR_STR_INVALID; }

    expr->err_pos = expr->cursor;

    if (ret != LDG_ERR_AOK) { expr->cunt = 0; }

    return ret;
}

static uint32_t expr_cmp(const crosstalk_re_repl_expr_val_t *lhs, const crosstalk_re_repl_expr_val_t *rhs, uint32_t *out)
{
    double lhs_real = 0x0;
    double rhs_real = 0x0;
    uint32_t ret = 0;

    *out = LDG_ARITH_ORD_EQ;

    if (lhs->fp == LDG_TRUTH_TRUE || rhs->fp == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_re_repl_expr_real_get(lhs, &lhs_real);
        if (ret != LDG_ERR_AOK) { return ret; }

        ret = crosstalk_re_repl_expr_real_get(rhs, &rhs_real);
        if (ret != LDG_ERR_AOK) { return ret; }

        if (lhs_real < rhs_real) { *out = LDG_ARITH_ORD_LT; }
        else if (lhs_real > rhs_real) { *out = LDG_ARITH_ORD_GT; }

        return LDG_ERR_AOK;
    }

    if (lhs->neg != rhs->neg) { *out = lhs->neg == LDG_TRUTH_TRUE ? LDG_ARITH_ORD_LT : LDG_ARITH_ORD_GT; }
    else if (lhs->mag != rhs->mag) { *out = (lhs->mag < rhs->mag) != (lhs->neg == LDG_TRUTH_TRUE) ? LDG_ARITH_ORD_LT : LDG_ARITH_ORD_GT; }

    return LDG_ERR_AOK;
}

static uint32_t expr_real_arith(uint32_t op, const crosstalk_re_repl_expr_val_t *lhs, const crosstalk_re_repl_expr_val_t *rhs, crosstalk_re_repl_expr_val_t *out)
{
    double lhs_real = 0x0;
    double rhs_real = 0x0;
    uint32_t ret = 0;

    ret = crosstalk_re_repl_expr_real_get(lhs, &lhs_real);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_re_repl_expr_real_get(rhs, &rhs_real);
    if (ret != LDG_ERR_AOK) { return ret; }

    out->fp = LDG_TRUTH_TRUE;

    if ((op == (uint32_t)EXPR_OP_DIV || op == (uint32_t)EXPR_OP_MOD) && rhs_real == EXPR_REAL_NONE) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (op == (uint32_t)EXPR_OP_ADD) { out->real = lhs_real + rhs_real; }
    else if (op == (uint32_t)EXPR_OP_SUB) { out->real = lhs_real - rhs_real; }
    else if (op == (uint32_t)EXPR_OP_MUL) { out->real = lhs_real * rhs_real; }
    else if (op == (uint32_t)EXPR_OP_DIV) { out->real = lhs_real / rhs_real; }
    else if (op == (uint32_t)EXPR_OP_MOD) { out->real = fmod(lhs_real, rhs_real); }
    else { out->real = pow(lhs_real, rhs_real); }

    return isfinite(out->real) ? LDG_ERR_AOK : LDG_ERR_RANGE;
}

static uint32_t expr_arith(uint32_t op, crosstalk_re_repl_expr_val_t lhs, crosstalk_re_repl_expr_val_t rhs, crosstalk_re_repl_expr_val_t *out)
{
    uint32_t ret = LDG_ERR_AOK;

    if (lhs.fp == LDG_TRUTH_TRUE || rhs.fp == LDG_TRUTH_TRUE || op == (uint32_t)EXPR_OP_DIV || op == (uint32_t)EXPR_OP_POW) { return expr_real_arith(op, &lhs, &rhs, out); }

    if (op == (uint32_t)EXPR_OP_SUB && rhs.mag != 0) { rhs.neg = rhs.neg == LDG_TRUTH_TRUE ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE; }

    if (op == (uint32_t)EXPR_OP_ADD || op == (uint32_t)EXPR_OP_SUB)
    {
        if (lhs.neg == rhs.neg)
        {
            out->neg = lhs.neg;
            ret = ldg_arith_uint64_add(lhs.mag, rhs.mag, &out->mag);
        }
        else if (lhs.mag >= rhs.mag)
        {
            out->neg = lhs.neg;
            out->mag = lhs.mag - rhs.mag;
        }
        else
        {
            out->neg = rhs.neg;
            out->mag = rhs.mag - lhs.mag;
        }
    }
    else if (op == (uint32_t)EXPR_OP_MUL)
    {
        out->neg = lhs.neg != rhs.neg ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
        ret = ldg_arith_uint64_mul(lhs.mag, rhs.mag, &out->mag);
    }
    else if (op == (uint32_t)EXPR_OP_MOD)
    {
        if (rhs.mag == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

        out->mag = lhs.mag % rhs.mag;
        out->neg = lhs.neg;
    }
    else { return LDG_ERR_FUNC_ARG_INVALID; }

    if (out->mag == 0) { out->neg = LDG_TRUTH_FALSE; }

    return ret;
}

static void expr_truth_put(crosstalk_re_repl_expr_val_t *out, uint8_t truth)
{
    out->mag = truth == LDG_TRUTH_TRUE ? (uint64_t)LDG_TRUTH_TRUE : (uint64_t)LDG_TRUTH_FALSE;
}

static uint32_t expr_func_eval(uint32_t op, const crosstalk_re_repl_expr_val_t *lhs, crosstalk_re_repl_expr_val_t *out)
{
    double real = 0x0;
    uint32_t ret = 0;

    ret = crosstalk_re_repl_expr_real_get(lhs, &real);
    if (ret != LDG_ERR_AOK) { return ret; }

    out->fp = LDG_TRUTH_TRUE;

    if (op == (uint32_t)EXPR_OP_SIN) { out->real = sin(real); }
    else if (op == (uint32_t)EXPR_OP_COS) { out->real = cos(real); }
    else if (op == (uint32_t)EXPR_OP_SQRT) { out->real = sqrt(real); }
    else { out->real = floor(real); }

    return isfinite(out->real) ? LDG_ERR_AOK : LDG_ERR_RANGE;
}

static uint32_t expr_cmp_eval(uint32_t op, const crosstalk_re_repl_expr_val_t *lhs, const crosstalk_re_repl_expr_val_t *rhs, crosstalk_re_repl_expr_val_t *out)
{
    uint32_t cmp = LDG_ARITH_ORD_EQ;
    uint32_t ret = 0;

    ret = expr_cmp(lhs, rhs, &cmp);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (op == (uint32_t)EXPR_OP_MIN) { *out = cmp == LDG_ARITH_ORD_GT ? *rhs : *lhs; }
    else if (op == (uint32_t)EXPR_OP_MAX) { *out = cmp == LDG_ARITH_ORD_LT ? *rhs : *lhs; }
    else if (op == (uint32_t)EXPR_OP_LT) { expr_truth_put(out, cmp == LDG_ARITH_ORD_LT ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }
    else if (op == (uint32_t)EXPR_OP_LE) { expr_truth_put(out, cmp != LDG_ARITH_ORD_GT ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }
    else if (op == (uint32_t)EXPR_OP_GT) { expr_truth_put(out, cmp == LDG_ARITH_ORD_GT ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }
    else if (op == (uint32_t)EXPR_OP_GE) { expr_truth_put(out, cmp != LDG_ARITH_ORD_LT ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }
    else if (op == (uint32_t)EXPR_OP_EQ) { expr_truth_put(out, cmp == LDG_ARITH_ORD_EQ ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }
    else { expr_truth_put(out, cmp != LDG_ARITH_ORD_EQ ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    return LDG_ERR_AOK;
}

static void expr_sign_eval(uint32_t op, const crosstalk_re_repl_expr_val_t *lhs, crosstalk_re_repl_expr_val_t *out)
{
    *out = *lhs;

    if (lhs->fp == LDG_TRUTH_TRUE)
    {
        out->real = op == (uint32_t)EXPR_OP_NEG ? -lhs->real : fabs(lhs->real);
        return;
    }

    out->neg = LDG_TRUTH_FALSE;

    if (lhs->mag != 0 && op == (uint32_t)EXPR_OP_NEG) { out->neg = lhs->neg == LDG_TRUTH_TRUE ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE; }
}

static uint32_t expr_node_eval(const crosstalk_re_repl_expr_t *expr, const crosstalk_re_repl_expr_env_t *env, uint32_t idx, uint32_t depth, crosstalk_re_repl_expr_val_t *out)
{
    const crosstalk_re_repl_expr_node_t *node = 0x0;
    crosstalk_re_repl_expr_val_t lhs = LDG_STRUCT_ZERO_INIT;
    crosstalk_re_repl_expr_val_t rhs = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (idx >= expr->cunt || depth > (uint32_t)CROSSTALK_RE_REPL_EXPR_DEPTH_CAP) { return LDG_ERR_BOUNDS; }

    node = &expr->node[idx];

    ret = ldg_mem_secure_zero(out, (uint64_t)sizeof(*out));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (node->op == (uint32_t)EXPR_OP_LITERAL)
    {
        *out = node->val;
        return LDG_ERR_AOK;
    }

    if (node->op == (uint32_t)EXPR_OP_VAR)
    {
        if (node->val.mag >= (uint64_t)CROSSTALK_RE_REPL_EXPR_VAR_CUNT) { return LDG_ERR_BOUNDS; }

        if (node->val.mag == (uint64_t)CROSSTALK_RE_REPL_EXPR_VAR_D) { return expr_arith((uint32_t)EXPR_OP_SUB, env->var[CROSSTALK_RE_REPL_EXPR_VAR_V], env->var[CROSSTALK_RE_REPL_EXPR_VAR_P], out); }

        *out = env->var[node->val.mag];

        return LDG_ERR_AOK;
    }

    ret = expr_node_eval(expr, env, node->lhs, depth + (uint32_t)EXPR_STEP, &lhs);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (node->op == (uint32_t)EXPR_OP_AND && crosstalk_re_repl_expr_truth_is(&lhs) == LDG_TRUTH_FALSE) { return LDG_ERR_AOK; }

    if (node->op == (uint32_t)EXPR_OP_OR && crosstalk_re_repl_expr_truth_is(&lhs) == LDG_TRUTH_TRUE)
    {
        expr_truth_put(out, LDG_TRUTH_TRUE);
        return LDG_ERR_AOK;
    }

    if (node->rhs != CROSSTALK_RE_REPL_EXPR_NODE_NONE)
    {
        ret = expr_node_eval(expr, env, node->rhs, depth + (uint32_t)EXPR_STEP, &rhs);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    if (node->op == (uint32_t)EXPR_OP_AND || node->op == (uint32_t)EXPR_OP_OR)
    {
        expr_truth_put(out, crosstalk_re_repl_expr_truth_is(&rhs));
        return LDG_ERR_AOK;
    }

    if (node->op == (uint32_t)EXPR_OP_NOT)
    {
        expr_truth_put(out, crosstalk_re_repl_expr_truth_is(&lhs) == LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE);
        return LDG_ERR_AOK;
    }

    if (node->op == (uint32_t)EXPR_OP_NEG || node->op == (uint32_t)EXPR_OP_ABS)
    {
        expr_sign_eval(node->op, &lhs, out);
        return LDG_ERR_AOK;
    }

    if (node->op >= (uint32_t)EXPR_OP_SIN && node->op <= (uint32_t)EXPR_OP_FLOOR) { return expr_func_eval(node->op, &lhs, out); }

    if ((node->op >= (uint32_t)EXPR_OP_LT && node->op <= (uint32_t)EXPR_OP_NE) || node->op == (uint32_t)EXPR_OP_MIN || node->op == (uint32_t)EXPR_OP_MAX) { return expr_cmp_eval(node->op, &lhs, &rhs, out); }

    return expr_arith(node->op, lhs, rhs, out);
}

uint32_t crosstalk_re_repl_expr_eval(const crosstalk_re_repl_expr_t *expr, const crosstalk_re_repl_expr_env_t *env, crosstalk_re_repl_expr_val_t *out)
{
    if (LDG_UNLIKELY(expr == 0x0 || env == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (expr->cunt == 0 || expr->cunt > (uint32_t)CROSSTALK_RE_REPL_EXPR_NODE_CAP) { return LDG_ERR_NOT_INIT; }

    return expr_node_eval(expr, env, expr->root, (uint32_t)EXPR_DEPTH_ROOT, out);
}
