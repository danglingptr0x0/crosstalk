#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/mem.h>
#include <dangling/str/str.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "re/diff.h"

#define DIFF_POOL_ITEM_VAR 0

static uint64_t diff_fnv_word(uint64_t hash, uint64_t word)
{
    uint32_t at = 0;

    for (at = 0; at < (uint32_t)LDG_WORD_BYTES * 0x2; at = at + 0x1)
    {
        hash = hash ^ (word & (uint64_t)LDG_BYTE_MASK);
        hash = hash * (uint64_t)CROSSTALK_RE_DIFF_FNV64_PRIME;
        word = word >> LDG_BYTE_BITS;
    }

    return hash;
}

static uint64_t diff_val_transform(uint64_t val, uint64_t base, uint64_t span)
{
    if (span != 0 && val >= base && val < base + span) { return (val - base) ^ (uint64_t)CROSSTALK_RE_DIFF_PTR_TAG; }

    return val;
}

static uint8_t diff_node_masked_is(const crosstalk_re_diff_ctx_t *ctx, uint32_t idx)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (at = 0; at < ctx->mask_cunt; at = at + 0x1) { if (ctx->mask_idx[at] == idx) { return LDG_TRUTH_TRUE; } }

    return LDG_TRUTH_FALSE;
}

static uint32_t diff_ram_word_get(const uint8_t *ram, uint64_t at)
{
    return (uint32_t)ram[at] | ((uint32_t)ram[at + 0x1] << LDG_BYTE_SHIFT_1) | ((uint32_t)ram[at + 0x2] << LDG_BYTE_SHIFT_2) | ((uint32_t)ram[at + 0x3] << LDG_BYTE_SHIFT_3);
}

uint32_t crosstalk_re_diff_init(crosstalk_re_diff_ctx_t *ctx)
{
    uint64_t node_bytes = 0;
    uint64_t page_bytes = 0;
    uint64_t need = 0;
    uint32_t side = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    node_bytes = (uint64_t)CROSSTALK_RE_DIFF_NODE_MAX * sizeof(uint64_t);
    page_bytes = (uint64_t)CROSSTALK_RE_DIFF_PAGE_MAX * sizeof(uint64_t);
    need = (node_bytes + page_bytes) * (uint64_t)CROSSTALK_RE_DIFF_SIDE_CUNT + (uint64_t)LDG_MEM_POOL_VAR_ALIGN * (uint64_t)CROSSTALK_RE_DIFF_SIDE_CUNT * 0x2;

    ret = ldg_mem_pool_create((uint64_t)DIFF_POOL_ITEM_VAR, need, &ctx->pool);
    if (ret != LDG_ERR_AOK) { return ret; }

    for (side = 0; side < (uint32_t)CROSSTALK_RE_DIFF_SIDE_CUNT; side = side + 0x1)
    {
        ret = ldg_mem_pool_alloc(ctx->pool, node_bytes, (void **)&ctx->side[side].node);
        if (ret != LDG_ERR_AOK) { return ret; }

        ret = ldg_mem_pool_alloc(ctx->pool, page_bytes, (void **)&ctx->side[side].page_hash);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    ctx->cycles = (uint64_t)CROSSTALK_RE_DIFF_CYCLES_DEFAULT;
    ctx->ok = LDG_TRUTH_TRUE;

    return crosstalk_re_diff_clear(ctx);
}

uint32_t crosstalk_re_diff_clear(crosstalk_re_diff_ctx_t *ctx)
{
    uint32_t side = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (side = 0; side < (uint32_t)CROSSTALK_RE_DIFF_SIDE_CUNT; side = side + 0x1)
    {
        crosstalk_re_diff_side_t *at = &ctx->side[side];

        at->path[0] = LDG_STR_TERM;
        at->word_off = 0;
        at->base = 0;
        at->span = 0;
        at->node_hash = 0;
        at->mem_hash = 0;
        at->ram_hash = 0;
        at->cycles_ran = 0;
        at->node_cunt = 0;
        at->mem_words = 0;
        at->page_cunt = 0;
        at->set = LDG_TRUTH_FALSE;
        at->taken = LDG_TRUTH_FALSE;

        if (ldg_str_rbrcpy(at->sig, CROSSTALK_RE_DIFF_SIG_DEFAULT, (uint64_t)sizeof(at->sig)) != LDG_ERR_AOK) { return LDG_ERR_STR_TRUNC; }
    }

    ctx->mask_cunt = 0;
    ctx->verdict = (uint32_t)CROSSTALK_RE_DIFF_VERDICT_NONE;
    ctx->node_diff_cunt = 0;
    ctx->node_first = UINT32_MAX;
    ctx->page_diff_cunt = 0;
    ctx->page_first = UINT32_MAX;
    ctx->mem_diff = LDG_TRUTH_FALSE;
    ctx->sp = (uint64_t)CROSSTALK_RE_DIFF_SP_NONE;

    return crosstalk_re_diff_func_clear(ctx);
}

uint32_t crosstalk_re_diff_func_clear(crosstalk_re_diff_ctx_t *ctx)
{
    uint32_t side = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->func[0] = LDG_STR_TERM;

    for (side = 0; side < (uint32_t)CROSSTALK_RE_DIFF_SIDE_CUNT; side = side + 0x1) { ctx->entry[side] = (uint64_t)UINT64_MAX; }

    memset(ctx->arg, 0x0, sizeof(ctx->arg));

    ctx->arg_cunt = 0;
    ctx->func_mode = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_diff_func_set(crosstalk_re_diff_ctx_t *ctx, const char *name, uint64_t entry_ref, uint64_t entry_cand)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (name[0] == LDG_STR_TERM) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_str_rbrcpy(ctx->func, name, (uint64_t)sizeof(ctx->func));
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->entry[CROSSTALK_RE_DIFF_SIDE_REF] = entry_ref;
    ctx->entry[CROSSTALK_RE_DIFF_SIDE_CAND] = entry_cand;
    ctx->func_mode = LDG_TRUTH_TRUE;
    ctx->verdict = (uint32_t)CROSSTALK_RE_DIFF_VERDICT_NONE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_diff_arg_set(crosstalk_re_diff_ctx_t *ctx, uint32_t idx, uint64_t val)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (idx >= (uint32_t)CROSSTALK_RE_DIFF_ARG_MAX) { return LDG_ERR_BOUNDS; }

    ctx->arg[idx] = val;

    if (idx + 0x1 > ctx->arg_cunt) { ctx->arg_cunt = idx + 0x1; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_diff_sp_set(crosstalk_re_diff_ctx_t *ctx, uint64_t sp)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->sp = sp;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_diff_shutdown(crosstalk_re_diff_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->pool != 0x0) { ret = ldg_mem_pool_destroy(&ctx->pool); }

    memset(ctx, 0x0, sizeof(*ctx));

    return ret;
}

uint32_t crosstalk_re_diff_side_set(crosstalk_re_diff_ctx_t *ctx, uint32_t side, const char *path, const char *sig, uint64_t word_off)
{
    crosstalk_re_diff_side_t *at = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (side >= (uint32_t)CROSSTALK_RE_DIFF_SIDE_CUNT) { return CROSSTALK_RE_DIFF_ERR_SIDE; }

    at = &ctx->side[side];

    ret = ldg_str_rbrcpy(at->path, path, (uint64_t)sizeof(at->path));
    if (ret != LDG_ERR_AOK) { return ret; }

    if (sig != 0x0 && sig[0] != LDG_STR_TERM)
    {
        ret = ldg_str_rbrcpy(at->sig, sig, (uint64_t)sizeof(at->sig));
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    at->word_off = word_off;
    at->set = LDG_TRUTH_TRUE;
    at->taken = LDG_TRUTH_FALSE;
    ctx->verdict = (uint32_t)CROSSTALK_RE_DIFF_VERDICT_NONE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_diff_side_get(const crosstalk_re_diff_ctx_t *ctx, uint32_t side, const crosstalk_re_diff_side_t **out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (side >= (uint32_t)CROSSTALK_RE_DIFF_SIDE_CUNT) { return CROSSTALK_RE_DIFF_ERR_SIDE; }

    *out = &ctx->side[side];

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_diff_cycles_set(crosstalk_re_diff_ctx_t *ctx, uint64_t cycles)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (cycles == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    ctx->cycles = cycles;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_diff_mask_add(crosstalk_re_diff_ctx_t *ctx, const char *sig)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || sig == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (sig[0] == LDG_STR_TERM) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->mask_cunt >= (uint32_t)CROSSTALK_RE_DIFF_MASK_MAX) { return CROSSTALK_RE_DIFF_ERR_MASK_FULL; }

    ret = ldg_str_rbrcpy(ctx->mask_name[ctx->mask_cunt], sig, (uint64_t)sizeof(ctx->mask_name[0]));
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->mask_idx[ctx->mask_cunt] = (uint32_t)UINT32_MAX;
    ctx->mask_cunt = ctx->mask_cunt + 0x1;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_diff_mask_idx_set(crosstalk_re_diff_ctx_t *ctx, uint32_t at, uint32_t idx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (at >= ctx->mask_cunt) { return LDG_ERR_BOUNDS; }

    ctx->mask_idx[at] = idx;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_diff_mask_clear(crosstalk_re_diff_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->mask_cunt = 0;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_re_diff_ready_is(const crosstalk_re_diff_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    return (ctx->side[CROSSTALK_RE_DIFF_SIDE_REF].set == LDG_TRUTH_TRUE && ctx->side[CROSSTALK_RE_DIFF_SIDE_CAND].set == LDG_TRUTH_TRUE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_re_diff_snap_take(crosstalk_re_diff_ctx_t *ctx, uint32_t side, const uint64_t *node, uint32_t node_cunt, const uint64_t *mem, uint32_t mem_words, const uint8_t *ram, uint64_t ram_span, uint64_t base, uint64_t span, uint64_t cycles_ran)
{
    crosstalk_re_diff_side_t *at = 0x0;
    uint64_t hash = 0;
    uint64_t page_at = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (side >= (uint32_t)CROSSTALK_RE_DIFF_SIDE_CUNT) { return CROSSTALK_RE_DIFF_ERR_SIDE; }

    if (node_cunt > (uint32_t)CROSSTALK_RE_DIFF_NODE_MAX) { return CROSSTALK_RE_DIFF_ERR_SNAP; }

    if (ram_span / (uint64_t)CROSSTALK_RE_DIFF_PAGE_BYTES > (uint64_t)CROSSTALK_RE_DIFF_PAGE_MAX) { return CROSSTALK_RE_DIFF_ERR_SNAP; }

    at = &ctx->side[side];
    at->base = base;
    at->span = span;
    at->cycles_ran = cycles_ran;
    at->node_cunt = node_cunt;
    at->mem_words = mem_words;
    at->page_cunt = 0;
    at->taken = LDG_TRUTH_FALSE;

    hash = (uint64_t)CROSSTALK_RE_DIFF_FNV64_INIT;

    for (idx = 0; idx < node_cunt; idx = idx + 0x1)
    {
        uint64_t val = 0;

        if (node != 0x0) { val = node[idx]; }

        if (diff_node_masked_is(ctx, idx) == LDG_TRUTH_TRUE) { val = 0; }
        else { val = diff_val_transform(val, base, span); }

        at->node[idx] = val;
        hash = diff_fnv_word(hash, val);
    }

    at->node_hash = hash;

    hash = (uint64_t)CROSSTALK_RE_DIFF_FNV64_INIT;

    for (idx = 0; mem != 0x0 && idx < mem_words; idx = idx + 0x1) { hash = diff_fnv_word(hash, diff_val_transform(mem[idx], base, span)); }

    at->mem_hash = hash;

    hash = (uint64_t)CROSSTALK_RE_DIFF_FNV64_INIT;

    for (page_at = 0; ram != 0x0 && page_at + (uint64_t)CROSSTALK_RE_DIFF_PAGE_BYTES <= ram_span; page_at = page_at + (uint64_t)CROSSTALK_RE_DIFF_PAGE_BYTES)
    {
        uint64_t page_hash = (uint64_t)CROSSTALK_RE_DIFF_FNV64_INIT;
        uint64_t word_at = 0;

        for (word_at = 0; word_at < (uint64_t)CROSSTALK_RE_DIFF_PAGE_BYTES; word_at = word_at + (uint64_t)CROSSTALK_RE_DIFF_RAM_WORD_BYTES) { page_hash = diff_fnv_word(page_hash, diff_val_transform((uint64_t)diff_ram_word_get(ram, page_at + word_at), base, span)); }

        at->page_hash[at->page_cunt] = page_hash;
        at->page_cunt = at->page_cunt + 0x1;
        hash = diff_fnv_word(hash, page_hash);
    }

    at->ram_hash = hash;
    at->taken = LDG_TRUTH_TRUE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "re diff: snapshot; side: %u32; nodes: %u32; mem words: %u32; ram pages: %u32; cycles: %u64; node hash: 0x%016X64; mem hash: 0x%016X64; ram hash: 0x%016X64", side, node_cunt, mem_words, at->page_cunt, cycles_ran, at->node_hash, at->mem_hash, at->ram_hash);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_diff_verdict_compute(crosstalk_re_diff_ctx_t *ctx)
{
    const crosstalk_re_diff_side_t *ref = 0x0;
    const crosstalk_re_diff_side_t *cand = 0x0;
    uint32_t idx = 0;
    uint32_t cunt = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ref = &ctx->side[CROSSTALK_RE_DIFF_SIDE_REF];
    cand = &ctx->side[CROSSTALK_RE_DIFF_SIDE_CAND];

    ctx->verdict = (uint32_t)CROSSTALK_RE_DIFF_VERDICT_NONE;
    ctx->node_diff_cunt = 0;
    ctx->node_first = UINT32_MAX;
    ctx->page_diff_cunt = 0;
    ctx->page_first = UINT32_MAX;
    ctx->mem_diff = LDG_TRUTH_FALSE;

    if (ref->taken != LDG_TRUTH_TRUE || cand->taken != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    cunt = ref->node_cunt < cand->node_cunt ? ref->node_cunt : cand->node_cunt;

    for (idx = 0; idx < cunt; idx = idx + 0x1)
    {
        if (ref->node[idx] == cand->node[idx]) { continue; }

        if (ctx->node_first == UINT32_MAX) { ctx->node_first = idx; }

        ctx->node_diff_cunt = ctx->node_diff_cunt + 0x1;
    }

    if (ref->node_cunt != cand->node_cunt) { ctx->node_diff_cunt = ctx->node_diff_cunt + (ref->node_cunt > cand->node_cunt ? ref->node_cunt - cand->node_cunt : cand->node_cunt - ref->node_cunt); }

    cunt = ref->page_cunt < cand->page_cunt ? ref->page_cunt : cand->page_cunt;

    for (idx = 0; idx < cunt; idx = idx + 0x1)
    {
        if (ref->page_hash[idx] == cand->page_hash[idx]) { continue; }

        if (ctx->page_first == UINT32_MAX) { ctx->page_first = idx; }

        ctx->page_diff_cunt = ctx->page_diff_cunt + 0x1;
    }

    if (ref->page_cunt != cand->page_cunt) { ctx->page_diff_cunt = ctx->page_diff_cunt + (ref->page_cunt > cand->page_cunt ? ref->page_cunt - cand->page_cunt : cand->page_cunt - ref->page_cunt); }

    ctx->mem_diff = ref->mem_hash != cand->mem_hash ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (ctx->node_diff_cunt == 0 && ctx->page_diff_cunt == 0 && ctx->mem_diff != LDG_TRUTH_TRUE) { ctx->verdict = (uint32_t)CROSSTALK_RE_DIFF_VERDICT_MATCH; }
    else { ctx->verdict = (uint32_t)CROSSTALK_RE_DIFF_VERDICT_DIVERGED; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_diff_verdict_name_get(uint32_t verdict, const char **out)
{
    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (verdict == (uint32_t)CROSSTALK_RE_DIFF_VERDICT_NONE) { *out = "none"; }
    else if (verdict == (uint32_t)CROSSTALK_RE_DIFF_VERDICT_MATCH) { *out = "match"; }
    else if (verdict == (uint32_t)CROSSTALK_RE_DIFF_VERDICT_DIVERGED) { *out = "diverged"; }
    else { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}
