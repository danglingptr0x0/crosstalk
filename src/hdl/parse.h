#ifndef CROSSTALK_HDL_PARSE_H
#define CROSSTALK_HDL_PARSE_H

#include <stdint.h>

#include "hdl/ast.h"
#include "hdl/lex.h"

#define CROSSTALK_HDL_ERR_PARSE_TOK 715
#define CROSSTALK_HDL_ERR_PARSE_SUBSET 716
#define CROSSTALK_HDL_ERR_PARSE_DEPTH 717

#define CROSSTALK_HDL_PARSE_DEPTH_MAX 64

typedef struct crosstalk_hdl_parse_ctx
{
    crosstalk_hdl_lex_ctx_t lex;
    crosstalk_hdl_tok_t prev;
    crosstalk_hdl_tok_t cur;
    crosstalk_hdl_tok_t nxt;
    crosstalk_hdl_ast_t *ast;
    uint32_t depth;
    uint8_t pudding[4];
} crosstalk_hdl_parse_ctx_t;

uint32_t crosstalk_hdl_parse_run(crosstalk_hdl_parse_ctx_t *ctx, const char *src, uint32_t len, crosstalk_hdl_ast_t *ast);

#endif
