#ifndef CROSSTALK_EDA_SCRIPT_AST_H
#define CROSSTALK_EDA_SCRIPT_AST_H

#include <stdint.h>

#include <dangling/mem/alloc.h>

#include "eda/script/lex.h"

typedef enum crosstalk_eda_script_node_kind
{
    CROSSTALK_EDA_SCRIPT_NODE_PROGRAM = 0,
    CROSSTALK_EDA_SCRIPT_NODE_SECTION,
    CROSSTALK_EDA_SCRIPT_NODE_BLK,
    CROSSTALK_EDA_SCRIPT_NODE_DECL,
    CROSSTALK_EDA_SCRIPT_NODE_EXPR_STMT,
    CROSSTALK_EDA_SCRIPT_NODE_GUARD,
    CROSSTALK_EDA_SCRIPT_NODE_RET,
    CROSSTALK_EDA_SCRIPT_NODE_INC,
    CROSSTALK_EDA_SCRIPT_NODE_DEC,
    CROSSTALK_EDA_SCRIPT_NODE_NUM,
    CROSSTALK_EDA_SCRIPT_NODE_STR,
    CROSSTALK_EDA_SCRIPT_NODE_IDENT,
    CROSSTALK_EDA_SCRIPT_NODE_CALL,
    CROSSTALK_EDA_SCRIPT_NODE_IDX,
    CROSSTALK_EDA_SCRIPT_NODE_SLICE,
    CROSSTALK_EDA_SCRIPT_NODE_UN,
    CROSSTALK_EDA_SCRIPT_NODE_BIN,
    CROSSTALK_EDA_SCRIPT_NODE_TERNARY,
    CROSSTALK_EDA_SCRIPT_NODE_CONCAT,
    CROSSTALK_EDA_SCRIPT_NODE_ASSIGN,
    CROSSTALK_EDA_SCRIPT_NODE_INIT_LIST,
    CROSSTALK_EDA_SCRIPT_NODE_RACK,
    CROSSTALK_EDA_SCRIPT_NODE_SWITCH
} crosstalk_eda_script_node_kind_t;

typedef enum crosstalk_eda_script_switch_kind
{
    CROSSTALK_EDA_SCRIPT_SWITCH_LEVER = 0,
    CROSSTALK_EDA_SCRIPT_SWITCH_ROTARY,
    CROSSTALK_EDA_SCRIPT_SWITCH_ENCODER,
    CROSSTALK_EDA_SCRIPT_SWITCH_BUTTON
} crosstalk_eda_script_switch_kind_t;

typedef enum crosstalk_eda_script_op
{
    CROSSTALK_EDA_SCRIPT_OP_ADD = 0,
    CROSSTALK_EDA_SCRIPT_OP_SUB,
    CROSSTALK_EDA_SCRIPT_OP_MUL,
    CROSSTALK_EDA_SCRIPT_OP_DIV,
    CROSSTALK_EDA_SCRIPT_OP_MOD,
    CROSSTALK_EDA_SCRIPT_OP_BAND,
    CROSSTALK_EDA_SCRIPT_OP_BOR,
    CROSSTALK_EDA_SCRIPT_OP_BXOR,
    CROSSTALK_EDA_SCRIPT_OP_BXNOR,
    CROSSTALK_EDA_SCRIPT_OP_SHL,
    CROSSTALK_EDA_SCRIPT_OP_SHR,
    CROSSTALK_EDA_SCRIPT_OP_SHLA,
    CROSSTALK_EDA_SCRIPT_OP_SHRA,
    CROSSTALK_EDA_SCRIPT_OP_LT,
    CROSSTALK_EDA_SCRIPT_OP_GT,
    CROSSTALK_EDA_SCRIPT_OP_LTE,
    CROSSTALK_EDA_SCRIPT_OP_GTE,
    CROSSTALK_EDA_SCRIPT_OP_EQ,
    CROSSTALK_EDA_SCRIPT_OP_NEQ,
    CROSSTALK_EDA_SCRIPT_OP_LAND,
    CROSSTALK_EDA_SCRIPT_OP_LOR,
    CROSSTALK_EDA_SCRIPT_OP_IMPL,
    CROSSTALK_EDA_SCRIPT_OP_BICOND,
    CROSSTALK_EDA_SCRIPT_OP_NEG,
    CROSSTALK_EDA_SCRIPT_OP_BNOT,
    CROSSTALK_EDA_SCRIPT_OP_LNOT,
    CROSSTALK_EDA_SCRIPT_OP_ASSIGN,
    CROSSTALK_EDA_SCRIPT_OP_ADD_ASSIGN,
    CROSSTALK_EDA_SCRIPT_OP_SUB_ASSIGN,
    CROSSTALK_EDA_SCRIPT_OP_MUL_ASSIGN,
    CROSSTALK_EDA_SCRIPT_OP_DIV_ASSIGN,
    CROSSTALK_EDA_SCRIPT_OP_MOD_ASSIGN,
    CROSSTALK_EDA_SCRIPT_OP_AND_ASSIGN,
    CROSSTALK_EDA_SCRIPT_OP_OR_ASSIGN,
    CROSSTALK_EDA_SCRIPT_OP_XOR_ASSIGN,
    CROSSTALK_EDA_SCRIPT_OP_SHL_ASSIGN,
    CROSSTALK_EDA_SCRIPT_OP_SHR_ASSIGN
} crosstalk_eda_script_op_t;

typedef struct crosstalk_eda_script_list
{
    struct crosstalk_eda_script_node *hd;
    struct crosstalk_eda_script_node *tail;
    uint32_t cunt;
    uint8_t pudding[4];
} crosstalk_eda_script_list_t;

typedef struct crosstalk_eda_script_node
{
    struct crosstalk_eda_script_node *next;
    struct crosstalk_eda_script_node *lhs;
    struct crosstalk_eda_script_node *rhs;
    struct crosstalk_eda_script_node *third;
    crosstalk_eda_script_list_t list;
    crosstalk_eda_script_list_t alt_list;
    const char *name;
    const char *alt_name;
    const char *bytes;
    uint64_t val;
    uint64_t r_lo;
    uint64_t r_hi;
    uint32_t name_len;
    uint32_t alt_name_len;
    uint32_t bytes_len;
    uint32_t width;
    uint32_t arr_cunt;
    uint32_t line;
    uint32_t col;
    uint32_t kind;
    uint32_t op;
    uint8_t is_const;
    uint8_t has_range;
    uint8_t r_lo_open;
    uint8_t r_hi_open;
    uint8_t action_call;
    uint8_t has_args;
    uint8_t alt_has_args;
    uint8_t quoted;
} crosstalk_eda_script_node_t;

typedef struct crosstalk_eda_script_ast
{
    ldg_mem_pool_t *pool;
    uint32_t nodes_cunt;
    uint8_t pudding[4];
} crosstalk_eda_script_ast_t;

uint32_t crosstalk_eda_script_ast_init(crosstalk_eda_script_ast_t *ctx, ldg_mem_pool_t *pool);
uint32_t crosstalk_eda_script_ast_rst(crosstalk_eda_script_ast_t *ctx);
uint32_t crosstalk_eda_script_ast_node_new(crosstalk_eda_script_ast_t *ctx, uint32_t kind, const crosstalk_eda_script_tok_t *tok, crosstalk_eda_script_node_t **out);
uint32_t crosstalk_eda_script_ast_str_copy(crosstalk_eda_script_ast_t *ctx, const char *src, uint32_t len, const char **out);
uint32_t crosstalk_eda_script_ast_str_decode(crosstalk_eda_script_ast_t *ctx, const char *src, uint32_t len, const char **out, uint32_t *out_len);
uint32_t crosstalk_eda_script_ast_list_push(crosstalk_eda_script_list_t *list, crosstalk_eda_script_node_t *node);

#endif
