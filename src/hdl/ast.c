#include <stdint.h>

#include <dangling/core/arith.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/mem.h>

#include "hdl/ast.h"

uint32_t crosstalk_hdl_ast_init(crosstalk_hdl_ast_t *ast, void *pool, uint32_t cap)
{
    ldg_mem_pool_t *node_pool = 0x0;
    void *raw = 0x0;
    uint64_t bytes = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ast == 0x0 || pool == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ast->nodes = 0x0;
    ast->cunt = 0;
    ast->cap = 0;
    ast->root = UINT32_MAX;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    node_pool = (ldg_mem_pool_t *)pool;

    ret = ldg_arith_uint64_mul((uint64_t)cap, (uint64_t)sizeof(crosstalk_hdl_ast_node_t), &bytes);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_OVERFLOW; }

    ret = ldg_mem_pool_alloc(node_pool, bytes, &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ast->nodes = (crosstalk_hdl_ast_node_t *)raw;
    ast->cap = cap;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_ast_node_add(crosstalk_hdl_ast_t *ast, uint32_t kind, const crosstalk_hdl_tok_t *tok, uint32_t *out_idx)
{
    crosstalk_hdl_tok_t blank = LDG_STRUCT_ZERO_INIT;
    crosstalk_hdl_ast_node_t *node = 0x0;

    if (LDG_UNLIKELY(ast == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    if (LDG_UNLIKELY(ast->nodes == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ast->cunt >= ast->cap)) { return LDG_ERR_FULL; }

    node = &ast->nodes[ast->cunt];
    node->tok = tok != 0x0 ? *tok : blank;
    node->kind = kind;
    node->first_child = UINT32_MAX;
    node->last_child = UINT32_MAX;
    node->next_sibling = UINT32_MAX;

    *out_idx = ast->cunt;
    ast->cunt++;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_ast_child_append(crosstalk_hdl_ast_t *ast, uint32_t parent_idx, uint32_t child_idx)
{
    crosstalk_hdl_ast_node_t *parent = 0x0;

    if (LDG_UNLIKELY(ast == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ast->nodes == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(parent_idx >= ast->cunt || child_idx >= ast->cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(parent_idx == child_idx)) { return LDG_ERR_FUNC_ARG_INVALID; }

    parent = &ast->nodes[parent_idx];

    if (parent->last_child == UINT32_MAX)
    {
        parent->first_child = child_idx;
        parent->last_child = child_idx;

        return LDG_ERR_AOK;
    }

    ast->nodes[parent->last_child].next_sibling = child_idx;
    parent->last_child = child_idx;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_ast_node_get(const crosstalk_hdl_ast_t *ast, uint32_t idx, const crosstalk_hdl_ast_node_t **out)
{
    if (LDG_UNLIKELY(ast == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(ast->nodes == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(idx >= ast->cunt)) { return LDG_ERR_BOUNDS; }

    *out = &ast->nodes[idx];

    return LDG_ERR_AOK;
}
