#ifndef CROSSTALK_HDL_IR_H
#define CROSSTALK_HDL_IR_H

#include <stdint.h>

#include "hdl/lex.h"

#define CROSSTALK_HDL_ERR_ELAB 718
#define CROSSTALK_HDL_ERR_ELAB_WIDTH 719

#define CROSSTALK_HDL_IR_INST_MAX 64
#define CROSSTALK_HDL_IR_JACK_MAX 512
#define CROSSTALK_HDL_IR_JACK_PREFIX "patch_"
#define CROSSTALK_HDL_IR_QUAL_AVG 48
#define CROSSTALK_HDL_IR_MEM_INIT_MAX 16384
#define CROSSTALK_HDL_IR_SYSTASK_MAX 1024
#define CROSSTALK_HDL_IR_SYSTASK_ARG_MAX 4096

typedef enum crosstalk_hdl_ir_port
{
    CROSSTALK_HDL_IR_PORT_NONE = 0,
    CROSSTALK_HDL_IR_PORT_IN,
    CROSSTALK_HDL_IR_PORT_OUT,
    CROSSTALK_HDL_IR_PORT_INOUT
} crosstalk_hdl_ir_port_t;

typedef enum crosstalk_hdl_ir_sig_kind
{
    CROSSTALK_HDL_IR_SIG_NET = 0,
    CROSSTALK_HDL_IR_SIG_VAR,
    CROSSTALK_HDL_IR_SIG_MEM
} crosstalk_hdl_ir_sig_kind_t;

typedef enum crosstalk_hdl_ir_op_kind
{
    CROSSTALK_HDL_IR_OP_NONE = 0,
    CROSSTALK_HDL_IR_OP_CONST,
    CROSSTALK_HDL_IR_OP_COPY,
    CROSSTALK_HDL_IR_OP_AND,
    CROSSTALK_HDL_IR_OP_OR,
    CROSSTALK_HDL_IR_OP_XOR,
    CROSSTALK_HDL_IR_OP_XNOR,
    CROSSTALK_HDL_IR_OP_NOT,
    CROSSTALK_HDL_IR_OP_ADD,
    CROSSTALK_HDL_IR_OP_SUB,
    CROSSTALK_HDL_IR_OP_MUL,
    CROSSTALK_HDL_IR_OP_DIV,
    CROSSTALK_HDL_IR_OP_MOD,
    CROSSTALK_HDL_IR_OP_NEG,
    CROSSTALK_HDL_IR_OP_SHL,
    CROSSTALK_HDL_IR_OP_SHR,
    CROSSTALK_HDL_IR_OP_ASHR,
    CROSSTALK_HDL_IR_OP_EQ,
    CROSSTALK_HDL_IR_OP_NEQ,
    CROSSTALK_HDL_IR_OP_LT,
    CROSSTALK_HDL_IR_OP_LE,
    CROSSTALK_HDL_IR_OP_GT,
    CROSSTALK_HDL_IR_OP_GE,
    CROSSTALK_HDL_IR_OP_LT_S,
    CROSSTALK_HDL_IR_OP_LE_S,
    CROSSTALK_HDL_IR_OP_GT_S,
    CROSSTALK_HDL_IR_OP_GE_S,
    CROSSTALK_HDL_IR_OP_LAND,
    CROSSTALK_HDL_IR_OP_LOR,
    CROSSTALK_HDL_IR_OP_LNOT,
    CROSSTALK_HDL_IR_OP_RED_AND,
    CROSSTALK_HDL_IR_OP_RED_OR,
    CROSSTALK_HDL_IR_OP_RED_XOR,
    CROSSTALK_HDL_IR_OP_CONCAT,
    CROSSTALK_HDL_IR_OP_SLICE,
    CROSSTALK_HDL_IR_OP_SLICE_DYN,
    CROSSTALK_HDL_IR_OP_INSERT,
    CROSSTALK_HDL_IR_OP_INSERT_DYN,
    CROSSTALK_HDL_IR_OP_MUX,
    CROSSTALK_HDL_IR_OP_ZEXT,
    CROSSTALK_HDL_IR_OP_SEXT,
    CROSSTALK_HDL_IR_OP_MEM_RD,
    CROSSTALK_HDL_IR_OP_MEM_WR,
    CROSSTALK_HDL_IR_OP_REG_NEXT
} crosstalk_hdl_ir_op_kind_t;

typedef enum crosstalk_hdl_ir_edge
{
    CROSSTALK_HDL_IR_EDGE_POS = 0,
    CROSSTALK_HDL_IR_EDGE_NEG
} crosstalk_hdl_ir_edge_t;

typedef struct crosstalk_hdl_ir_sig
{
    crosstalk_hdl_tok_t name;
    crosstalk_hdl_tok_t mem_file;
    uint32_t width;
    uint32_t depth;
    uint32_t kind;
    uint32_t port;
    uint64_t init_val;
    uint32_t qual_off;
    uint32_t qual_len;
} crosstalk_hdl_ir_sig_t;

typedef struct crosstalk_hdl_ir_op
{
    uint64_t imm;
    uint32_t kind;
    uint32_t dst;
    uint32_t src0;
    uint32_t src1;
    uint32_t src2;
    uint32_t width;
    uint32_t guard;
    uint8_t guard_is;
    uint8_t pudding[3];
} crosstalk_hdl_ir_op_t;

typedef struct crosstalk_hdl_ir_inst
{
    crosstalk_hdl_tok_t name;
    crosstalk_hdl_tok_t module_name;
} crosstalk_hdl_ir_inst_t;

typedef struct crosstalk_hdl_ir_jack
{
    crosstalk_hdl_tok_t name;
    uint32_t inst;
    uint32_t sig;
    uint32_t width;
    uint32_t dir;
} crosstalk_hdl_ir_jack_t;

typedef struct crosstalk_hdl_ir_mem_init
{
    uint64_t addr;
    uint64_t val;
    uint32_t sig;
    uint8_t pudding[4];
} crosstalk_hdl_ir_mem_init_t;

typedef enum crosstalk_hdl_ir_systask_kind
{
    CROSSTALK_HDL_IR_SYSTASK_DISPLAY = 0,
    CROSSTALK_HDL_IR_SYSTASK_WR,
    CROSSTALK_HDL_IR_SYSTASK_FINISH
} crosstalk_hdl_ir_systask_kind_t;

typedef enum crosstalk_hdl_ir_systask_where
{
    CROSSTALK_HDL_IR_SYSTASK_WHERE_SEQ = 0,
    CROSSTALK_HDL_IR_SYSTASK_WHERE_COMB,
    CROSSTALK_HDL_IR_SYSTASK_WHERE_INITIAL,
    CROSSTALK_HDL_IR_SYSTASK_WHERE_FINAL
} crosstalk_hdl_ir_systask_where_t;

typedef struct crosstalk_hdl_ir_systask
{
    crosstalk_hdl_tok_t call;
    uint32_t kind;
    uint32_t where;
    uint32_t blk;
    uint32_t guard;
    uint32_t arg_start;
    uint32_t arg_cunt;
} crosstalk_hdl_ir_systask_t;

typedef struct crosstalk_hdl_ir_systask_arg
{
    crosstalk_hdl_tok_t str;
    uint32_t sig;
    uint32_t width;
    uint8_t sgn;
    uint8_t str_is;
    uint8_t pudding[2];
} crosstalk_hdl_ir_systask_arg_t;

typedef struct crosstalk_hdl_ir_seq_blk
{
    uint32_t clk_sig;
    uint32_t clk_edge;
    uint32_t rst_sig;
    uint32_t rst_edge;
    uint32_t op_start;
    uint32_t op_cunt;
} crosstalk_hdl_ir_seq_blk_t;

typedef struct crosstalk_hdl_ir
{
    crosstalk_hdl_ir_sig_t *sigs;
    crosstalk_hdl_ir_op_t *comb_ops;
    crosstalk_hdl_ir_op_t *seq_ops;
    crosstalk_hdl_ir_seq_blk_t *seq_blks;
    crosstalk_hdl_ir_inst_t *insts;
    crosstalk_hdl_ir_jack_t *jacks;
    crosstalk_hdl_ir_mem_init_t *mem_inits;
    crosstalk_hdl_ir_systask_t *systasks;
    crosstalk_hdl_ir_systask_arg_t *systask_args;
    char *qual_text;
    uint32_t qual_text_len;
    uint32_t qual_text_cap;
    uint32_t inst_cunt;
    uint32_t jack_cunt;
    uint32_t sig_cunt;
    uint32_t sig_cap;
    uint32_t comb_cunt;
    uint32_t comb_cap;
    uint32_t seq_op_cunt;
    uint32_t seq_op_cap;
    uint32_t seq_blk_cunt;
    uint32_t seq_blk_cap;
    uint32_t mem_init_cunt;
    uint32_t mem_init_cap;
    uint32_t systask_cunt;
    uint32_t systask_arg_cunt;
    uint32_t final_cunt;
    uint32_t tmp_base;
} crosstalk_hdl_ir_t;

uint32_t crosstalk_hdl_ir_init(crosstalk_hdl_ir_t *ir_ctx, void *pool, uint32_t sig_cap, uint32_t comb_cap, uint32_t seq_op_cap, uint32_t seq_blk_cap);
uint32_t crosstalk_hdl_ir_systask_arg_add(crosstalk_hdl_ir_t *ir_ctx, const crosstalk_hdl_ir_systask_arg_t *arg);
uint32_t crosstalk_hdl_ir_systask_add(crosstalk_hdl_ir_t *ir_ctx, const crosstalk_hdl_ir_systask_t *task);
uint32_t crosstalk_hdl_ir_sig_add(crosstalk_hdl_ir_t *ir_ctx, const crosstalk_hdl_tok_t *name, uint32_t width, uint32_t depth, uint32_t kind, uint32_t port, uint64_t init_val, uint32_t *out_idx);
uint32_t crosstalk_hdl_ir_comb_op_add(crosstalk_hdl_ir_t *ir_ctx, const crosstalk_hdl_ir_op_t *op);
uint32_t crosstalk_hdl_ir_seq_blk_open(crosstalk_hdl_ir_t *ir_ctx, uint32_t clk_sig, uint32_t clk_edge, uint32_t rst_sig, uint32_t rst_edge, uint32_t *out_idx);
uint32_t crosstalk_hdl_ir_seq_op_add(crosstalk_hdl_ir_t *ir_ctx, uint32_t blk_idx, const crosstalk_hdl_ir_op_t *op);
uint32_t crosstalk_hdl_ir_sig_mem_file_set(crosstalk_hdl_ir_t *ir_ctx, uint32_t sig_idx, const crosstalk_hdl_tok_t *file_tok);
uint32_t crosstalk_hdl_ir_mem_init_add(crosstalk_hdl_ir_t *ir_ctx, uint32_t sig_idx, uint64_t addr, uint64_t val);
uint32_t crosstalk_hdl_ir_sig_qual_set(crosstalk_hdl_ir_t *ir_ctx, uint32_t sig_idx, const char *text, uint32_t len);
uint32_t crosstalk_hdl_ir_inst_add(crosstalk_hdl_ir_t *ir_ctx, const crosstalk_hdl_tok_t *name, const crosstalk_hdl_tok_t *module_name, uint32_t *out_idx);
uint32_t crosstalk_hdl_ir_jack_add(crosstalk_hdl_ir_t *ir_ctx, const crosstalk_hdl_tok_t *name, uint32_t inst, uint32_t sig, uint32_t width, uint32_t dir);

#endif
