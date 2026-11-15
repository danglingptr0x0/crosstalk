#ifndef CROSSTALK_RE_REPL_EXPR_H
#define CROSSTALK_RE_REPL_EXPR_H

#include <stdint.h>

#define CROSSTALK_RE_REPL_EXPR_NODE_CAP 96
#define CROSSTALK_RE_REPL_EXPR_TEXT_CAP 256
#define CROSSTALK_RE_REPL_EXPR_DEPTH_CAP 16
#define CROSSTALK_RE_REPL_EXPR_VAL_PUDDING_SIZE 6
#define CROSSTALK_RE_REPL_EXPR_NODE_NONE UINT32_MAX

typedef enum crosstalk_re_repl_expr_var
{
    CROSSTALK_RE_REPL_EXPR_VAR_V = 0,
    CROSSTALK_RE_REPL_EXPR_VAR_P,
    CROSSTALK_RE_REPL_EXPR_VAR_B,
    CROSSTALK_RE_REPL_EXPR_VAR_D,
    CROSSTALK_RE_REPL_EXPR_VAR_T,
    CROSSTALK_RE_REPL_EXPR_VAR_DT,
    CROSSTALK_RE_REPL_EXPR_VAR_N,
    CROSSTALK_RE_REPL_EXPR_VAR_CUNT
} crosstalk_re_repl_expr_var_t;

typedef struct crosstalk_re_repl_expr_val
{
    uint64_t mag;
    double real;
    uint8_t fp;
    uint8_t neg;
    uint8_t pudding[CROSSTALK_RE_REPL_EXPR_VAL_PUDDING_SIZE];
} crosstalk_re_repl_expr_val_t;

typedef struct crosstalk_re_repl_expr_node
{
    crosstalk_re_repl_expr_val_t val;
    uint32_t op;
    uint32_t lhs;
    uint32_t rhs;
    uint32_t depth;
} crosstalk_re_repl_expr_node_t;

typedef struct crosstalk_re_repl_expr
{
    crosstalk_re_repl_expr_node_t node[CROSSTALK_RE_REPL_EXPR_NODE_CAP];
    char text[CROSSTALK_RE_REPL_EXPR_TEXT_CAP];
    uint32_t cunt;
    uint32_t root;
    uint32_t cursor;
    uint32_t err_pos;
} crosstalk_re_repl_expr_t;

typedef struct crosstalk_re_repl_expr_env
{
    crosstalk_re_repl_expr_val_t var[CROSSTALK_RE_REPL_EXPR_VAR_CUNT];
} crosstalk_re_repl_expr_env_t;

uint32_t crosstalk_re_repl_expr_node_parse(crosstalk_re_repl_expr_t *expr, uint32_t min_prec, uint32_t depth, uint32_t *out);
uint32_t crosstalk_re_repl_expr_compile(crosstalk_re_repl_expr_t *expr, const char *text);
uint32_t crosstalk_re_repl_expr_eval(const crosstalk_re_repl_expr_t *expr, const crosstalk_re_repl_expr_env_t *env, crosstalk_re_repl_expr_val_t *out);
uint32_t crosstalk_re_repl_expr_real_get(const crosstalk_re_repl_expr_val_t *val, double *out);
uint8_t crosstalk_re_repl_expr_truth_is(const crosstalk_re_repl_expr_val_t *val);
uint32_t crosstalk_re_repl_expr_decode(uint64_t bits, uint32_t bytes, uint8_t fp, uint8_t signed_val, crosstalk_re_repl_expr_val_t *out);

#endif
