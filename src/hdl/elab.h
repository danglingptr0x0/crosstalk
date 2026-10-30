#ifndef CROSSTALK_HDL_ELAB_H
#define CROSSTALK_HDL_ELAB_H

#include <stdint.h>

#include "hdl/ast.h"
#include "hdl/ir.h"
#include "hdl/lex.h"

#define CROSSTALK_HDL_ELAB_SYM_CAP 4096
#define CROSSTALK_HDL_ELAB_MOD_CAP 256
#define CROSSTALK_HDL_ELAB_FUNC_CAP 128
#define CROSSTALK_HDL_ELAB_DEPTH_MAX 32
#define CROSSTALK_HDL_ELAB_UNROLL_MAX 4096
#define CROSSTALK_HDL_ELAB_WIDTH_MAX 128
#define CROSSTALK_HDL_ELAB_INT_WIDTH 32
#define CROSSTALK_HDL_ELAB_LOGIC_WIDTH 1
#define CROSSTALK_HDL_ELAB_CAST_WIDTH_KEEP 0
#define CROSSTALK_HDL_ELAB_PLUSARGS_TEST_NAME "$test$plusargs"
#define CROSSTALK_HDL_ELAB_PLUSARGS_VAL_NAME "$value$plusargs"
#define CROSSTALK_HDL_ELAB_PLUSARGS_ABSENT 0
#define CROSSTALK_HDL_ELAB_CAST_VOID_NAME "void"
#define CROSSTALK_HDL_ELAB_STYP_CAP 64
#define CROSSTALK_HDL_ELAB_SFIELD_CAP 1024
#define CROSSTALK_HDL_ELAB_DEFER_CAP 256
#define CROSSTALK_HDL_ELAB_SFLD_MAX 64
#define CROSSTALK_HDL_ELAB_UNKNOWN_MOD_CAP 16
#define CROSSTALK_HDL_ELAB_GEN_DEPTH_MAX 8
#define CROSSTALK_HDL_ELAB_GEN_IDX_CAP 12
#define CROSSTALK_HDL_ELAB_PATTERN_CAP 32

typedef enum crosstalk_hdl_elab_sym_kind
{
    CROSSTALK_HDL_ELAB_SYM_SIG = 0,
    CROSSTALK_HDL_ELAB_SYM_MEM,
    CROSSTALK_HDL_ELAB_SYM_PARAM,
    CROSSTALK_HDL_ELAB_SYM_TYPE,
    CROSSTALK_HDL_ELAB_SYM_STYP,
    CROSSTALK_HDL_ELAB_SYM_SVAR,
    CROSSTALK_HDL_ELAB_SYM_STR
} crosstalk_hdl_elab_sym_kind_t;

typedef enum crosstalk_hdl_elab_mode
{
    CROSSTALK_HDL_ELAB_MODE_COMB = 0,
    CROSSTALK_HDL_ELAB_MODE_SEQ,
    CROSSTALK_HDL_ELAB_MODE_INITIAL,
    CROSSTALK_HDL_ELAB_MODE_FINAL
} crosstalk_hdl_elab_mode_t;

typedef struct crosstalk_hdl_elab_sym
{
    crosstalk_hdl_tok_t name;
    uint64_t val;
    uint32_t kind;
    uint32_t sig;
    uint32_t width;
    uint8_t pudding[4];
} crosstalk_hdl_elab_sym_t;

typedef struct crosstalk_hdl_elab_mod
{
    crosstalk_hdl_tok_t name;
    uint32_t node;
    uint8_t pudding[4];
} crosstalk_hdl_elab_mod_t;

typedef struct crosstalk_hdl_elab_func
{
    crosstalk_hdl_tok_t name;
    uint32_t node;
    uint8_t pudding[4];
} crosstalk_hdl_elab_func_t;

typedef struct crosstalk_hdl_elab_sfield
{
    crosstalk_hdl_tok_t name;
    uint32_t width;
    uint32_t off;
    uint32_t sub_styp;
    uint32_t sub_leaf_base;
    uint32_t sub_leaf_cunt;
    uint8_t pudding[4];
} crosstalk_hdl_elab_sfield_t;

typedef struct crosstalk_hdl_elab_styp
{
    crosstalk_hdl_tok_t name;
    uint32_t field_base;
    uint32_t field_cunt;
    uint32_t group_base;
    uint32_t group_cunt;
    uint32_t total_width;
    uint8_t pudding[4];
} crosstalk_hdl_elab_styp_t;

typedef struct crosstalk_hdl_elab_ctx
{
    const char *src;
    const crosstalk_hdl_ast_t *ast;
    crosstalk_hdl_ir_t *ir_ctx;
    void *pool;
    crosstalk_hdl_elab_sym_t *syms;
    crosstalk_hdl_elab_mod_t *mods;
    crosstalk_hdl_elab_func_t *funcs;
    crosstalk_hdl_elab_styp_t *styps;
    crosstalk_hdl_elab_sfield_t *sfields;
    const void *port_conn;
    uint32_t *current_map;
    uint32_t *next_map;
    crosstalk_hdl_tok_t err_tok;
    crosstalk_hdl_tok_t inst_name;
    crosstalk_hdl_tok_t inst_module;
    uint32_t sym_cunt;
    uint32_t mod_cunt;
    uint32_t func_cunt;
    uint32_t styp_cunt;
    uint32_t sfield_cunt;
    uint32_t current_cap;
    uint32_t guard;
    uint32_t ret_sig;
    uint32_t ret_width;
    uint32_t ret_svar_sym;
    uint32_t call_styp;
    uint32_t call_base;
    uint32_t seq_blk;
    uint32_t mode;
    uint32_t depth;
    uint32_t warn_cunt;
    uint32_t cw;
    uint32_t defer_sig[CROSSTALK_HDL_ELAB_DEFER_CAP];
    uint32_t defer_node[CROSSTALK_HDL_ELAB_DEFER_CAP];
    uint32_t defer_width[CROSSTALK_HDL_ELAB_DEFER_CAP];
    uint32_t defer_cunt;
    uint32_t gen_at[CROSSTALK_HDL_ELAB_GEN_DEPTH_MAX];
    uint32_t gen_depth;
    crosstalk_hdl_tok_t unknown_mod[CROSSTALK_HDL_ELAB_UNKNOWN_MOD_CAP];
    uint32_t unknown_mod_cunt;
    uint32_t port_conn_cunt;
    uint8_t nb_is;
    uint8_t decl_defer;
    uint8_t stmt_decl;
    uint8_t inst_pending;
} crosstalk_hdl_elab_ctx_t;

uint32_t crosstalk_hdl_elab_run(crosstalk_hdl_elab_ctx_t *ctx, const char *src, const crosstalk_hdl_ast_t *ast, void *pool, crosstalk_hdl_ir_t *ir_ctx, const char *top_name, uint32_t top_len);
uint32_t crosstalk_hdl_elab_member_resolve(crosstalk_hdl_elab_ctx_t *ctx, uint32_t dot_idx, uint32_t *out_sig, uint32_t *out_width, uint32_t *out_addr_sig);

uint8_t crosstalk_hdl_elab_tok_lit_eq_is(const char *src, const crosstalk_hdl_tok_t *tok, const char *lit);
uint8_t crosstalk_hdl_elab_tok_eq_is(const char *src, const crosstalk_hdl_tok_t *lhs, const crosstalk_hdl_tok_t *rhs);
uint32_t crosstalk_hdl_elab_lit_decode(const char *src, const crosstalk_hdl_tok_t *tok, uint64_t *out_val, uint32_t *out_width);
uint32_t crosstalk_hdl_elab_lit_mask_decode(const char *src, const crosstalk_hdl_tok_t *tok, uint64_t *out_val, uint64_t *out_mask, uint32_t *out_width);
uint32_t crosstalk_hdl_elab_sym_add(crosstalk_hdl_elab_ctx_t *ctx, const crosstalk_hdl_tok_t *name, uint32_t kind, uint32_t sig, uint64_t val, uint32_t width);
uint32_t crosstalk_hdl_elab_sym_find(crosstalk_hdl_elab_ctx_t *ctx, const crosstalk_hdl_tok_t *name, uint32_t *out_idx);
uint32_t crosstalk_hdl_elab_const_eval(crosstalk_hdl_elab_ctx_t *ctx, uint32_t node_idx, uint64_t *out_val);
uint32_t crosstalk_hdl_elab_tmp_make(crosstalk_hdl_elab_ctx_t *ctx, uint32_t width, uint32_t *out_sig);
uint32_t crosstalk_hdl_elab_op_emit(crosstalk_hdl_elab_ctx_t *ctx, const crosstalk_hdl_ir_op_t *op);
uint32_t crosstalk_hdl_elab_const_sig(crosstalk_hdl_elab_ctx_t *ctx, uint64_t val, uint32_t width, uint32_t *out_sig);
uint32_t crosstalk_hdl_elab_expr(crosstalk_hdl_elab_ctx_t *ctx, uint32_t node_idx, uint32_t *out_sig, uint32_t *out_width, uint8_t *out_sgn);
uint32_t crosstalk_hdl_elab_width_fit(crosstalk_hdl_elab_ctx_t *ctx, uint32_t val_sig, uint32_t val_width, uint8_t sgn, uint32_t want_width, uint32_t *out_sig);
uint32_t crosstalk_hdl_elab_func_call(crosstalk_hdl_elab_ctx_t *ctx, uint32_t call_idx, uint32_t *out_sig, uint32_t *out_width, uint8_t *out_sgn);

#endif
