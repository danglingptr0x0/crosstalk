#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>
#include <dangling/mem/alloc.h>

#include "eda/script/ast.h"

#define AST_STEP 1
#define AST_ESC '\\'
#define AST_TICK '\''

uint32_t crosstalk_eda_script_ast_init(crosstalk_eda_script_ast_t *ctx, ldg_mem_pool_t *pool)
{
    if (LDG_UNLIKELY(ctx == 0x0 || pool == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->pool = pool;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_script_ast_rst(crosstalk_eda_script_ast_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->pool == 0x0) { return LDG_ERR_NOT_INIT; }

    ret = ldg_mem_pool_rst(ctx->pool);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->nodes_cunt = 0;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_script_ast_node_new(crosstalk_eda_script_ast_t *ctx, uint32_t kind, const crosstalk_eda_script_tok_t *tok, crosstalk_eda_script_node_t **out)
{
    void *mem = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (ctx->pool == 0x0) { return LDG_ERR_NOT_INIT; }

    ret = ldg_mem_pool_alloc(ctx->pool, sizeof(crosstalk_eda_script_node_t), &mem);
    if (ret != LDG_ERR_AOK) { return CROSSTALK_EDA_SCRIPT_ERR_FULL; }

    memset(mem, 0x0, sizeof(crosstalk_eda_script_node_t));

    *out = (crosstalk_eda_script_node_t *)mem;
    (*out)->kind = kind;

    if (tok != 0x0)
    {
        (*out)->line = tok->line;
        (*out)->col = tok->col;
    }

    ctx->nodes_cunt = ctx->nodes_cunt + (uint32_t)AST_STEP;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_script_ast_str_copy(crosstalk_eda_script_ast_t *ctx, const char *src, uint32_t len, const char **out)
{
    void *mem = 0x0;
    char *dst = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (ctx->pool == 0x0) { return LDG_ERR_NOT_INIT; }

    ret = ldg_mem_pool_alloc(ctx->pool, (uint64_t)len + (uint64_t)LDG_STR_TERM_SIZE, &mem);
    if (ret != LDG_ERR_AOK) { return CROSSTALK_EDA_SCRIPT_ERR_FULL; }

    dst = (char *)mem;

    if (src != 0x0 && len != 0) { memcpy(dst, src, len); }

    dst[len] = LDG_STR_TERM;
    *out = dst;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_script_ast_str_decode(crosstalk_eda_script_ast_t *ctx, const char *src, uint32_t len, const char **out, uint32_t *out_len)
{
    void *mem = 0x0;
    char *dst = 0x0;
    uint32_t src_at = 0;
    uint32_t dst_at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || src == 0x0 || out == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;
    *out_len = 0;

    if (ctx->pool == 0x0) { return LDG_ERR_NOT_INIT; }

    ret = ldg_mem_pool_alloc(ctx->pool, (uint64_t)len + (uint64_t)LDG_STR_TERM_SIZE, &mem);
    if (ret != LDG_ERR_AOK) { return CROSSTALK_EDA_SCRIPT_ERR_FULL; }

    dst = (char *)mem;

    while (src_at < len)
    {
        char ch = src[src_at];

        if (ch == AST_ESC && src_at + (uint32_t)AST_STEP < len)
        {
            char esc = src[src_at + (uint32_t)AST_STEP];

            src_at = src_at + (uint32_t)AST_STEP + (uint32_t)AST_STEP;

            if (esc == 'n') { dst[dst_at] = '\n'; }
            else if (esc == 't') { dst[dst_at] = '\t'; }
            else if (esc == 'r') { dst[dst_at] = '\r'; }
            else if (esc == AST_ESC) { dst[dst_at] = AST_ESC; }
            else if (esc == LDG_ASCII_QUOTE) { dst[dst_at] = LDG_ASCII_QUOTE; }
            else if (esc == AST_TICK) { dst[dst_at] = AST_TICK; }
            else if (esc == LDG_ASCII_DIGIT_ZERO) { dst[dst_at] = LDG_STR_TERM; }
            else { return CROSSTALK_EDA_SCRIPT_ERR_LEX; }

            dst_at = dst_at + (uint32_t)AST_STEP;

            continue;
        }

        dst[dst_at] = ch;
        dst_at = dst_at + (uint32_t)AST_STEP;
        src_at = src_at + (uint32_t)AST_STEP;
    }

    dst[dst_at] = LDG_STR_TERM;
    *out = dst;
    *out_len = dst_at;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_script_ast_list_push(crosstalk_eda_script_list_t *list, crosstalk_eda_script_node_t *node)
{
    if (LDG_UNLIKELY(list == 0x0 || node == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    node->next = 0x0;

    if (list->tail == 0x0) { list->hd = node; }
    else { list->tail->next = node; }

    list->tail = node;
    list->cunt = list->cunt + (uint32_t)AST_STEP;

    return LDG_ERR_AOK;
}
