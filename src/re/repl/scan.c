#include <math.h>
#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/region.h>
#include <dangling/mem/secure.h>
#include <dangling/mem/vmem.h>
#include <dangling/str/str.h>
#include <dangling/time/time.h>

#include "re/repl/expr.h"
#include "re/repl/native.h"
#include "re/repl/repl.h"
#include "re/repl/scan.h"

#define SCAN_STEP 1
#define SCAN_CANDIDATE_CAP 32768
#define SCAN_STEP_CAP 256
#define SCAN_TIME_BUDGET_MS 2
#define SCAN_PAGE_BYTES (4 * LDG_KIB)
#define SCAN_RANGE_CAP (256 * LDG_MIB)
#define SCAN_SAMPLE_CAP 4096
#define SCAN_INTERVAL_MIN_MS 16
#define SCAN_INTERVAL_MAX_MS 60000
#define SCAN_BEST_CAP 64
#define SCAN_PASS_FIRST 1
#define SCAN_ARG_VERB 1
#define SCAN_ARG_TYPE 2
#define SCAN_ARG_ADDR 3
#define SCAN_ARG_LEN 4
#define SCAN_ARG_STRIDE 5
#define SCAN_ARG_PREDICATE 6
#define SCAN_ARG_EXPR 2
#define SCAN_ARG_INTERVAL 2
#define SCAN_ARG_PASSES 3
#define SCAN_ARG_TIMED_EXPR 4
#define SCAN_ARG_START 2
#define SCAN_ARGC_ROOT 1
#define SCAN_ARGC_VERB 2
#define SCAN_ARGC_LIST_MAX 3
#define SCAN_ARGC_TIMED_MIN 5
#define SCAN_ARGC_BEGIN_MIN 6
#define SCAN_ITEM_PUDDING_SIZE 7
#define SCAN_CTX_PUDDING_SIZE 4
#define SCAN_CH_TRUTH '1'
#define SCAN_WORD_HELP "help"
#define SCAN_WORD_STATUS "status"
#define SCAN_WORD_STOP "stop"
#define SCAN_WORD_BEGIN "begin"
#define SCAN_WORD_WHERE "where"
#define SCAN_WORD_WATCH "watch"
#define SCAN_WORD_FIT "fit"
#define SCAN_WORD_REBASE "rebase"
#define SCAN_WORD_LIST "list"
#define SCAN_WORD_BEST "best"
#define SCAN_WORD_CLEAR "clear"
#define SCAN_SIGN_NEG "-"
#define SCAN_SIGN_NONE ""

typedef enum scan_mode
{
    SCAN_MODE_IDLE = 0,
    SCAN_MODE_BASE,
    SCAN_MODE_WHERE,
    SCAN_MODE_WATCH,
    SCAN_MODE_FIT,
    SCAN_MODE_REBASE
} scan_mode_t;

typedef struct scan_item
{
    uint64_t addr;
    uint64_t alloc_base;
    uint64_t base;
    uint64_t prev;
    uint64_t base_ms;
    uint64_t prev_ms;
    double score;
    uint32_t sample_cunt;
    uint32_t kind;
    uint8_t alive;
    uint8_t pudding[SCAN_ITEM_PUDDING_SIZE];
} scan_item_t;

typedef struct scan_ctx
{
    scan_item_t item[SCAN_CANDIDATE_CAP];
    crosstalk_re_repl_expr_t expr;
    crosstalk_re_repl_expr_t pending;
    crosstalk_re_repl_expr_env_t env;
    uint8_t page[SCAN_PAGE_BYTES];
    char expr_text[CROSSTALK_RE_REPL_EXPR_TEXT_CAP];
    uint32_t best[SCAN_BEST_CAP];
    uint64_t cursor;
    uint64_t end;
    uint64_t skipped;
    uint64_t page_addr;
    uint64_t page_end;
    uint64_t page_alloc_base;
    uint64_t page_ms;
    uint64_t start_ms;
    uint64_t next_ms;
    uint64_t pass_ms;
    uint64_t pass_span_ms;
    uint64_t max_dt_ms;
    uint32_t page_kind;
    uint32_t mode;
    uint32_t bytes;
    uint32_t stride;
    uint32_t cunt;
    uint32_t alive_cunt;
    uint32_t idx;
    uint32_t pass;
    uint32_t pass_cunt;
    uint32_t interval_ms;
    uint32_t err_cunt;
    uint32_t lost_cunt;
    uint32_t last_err;
    uint32_t best_cunt;
    uint8_t fp;
    uint8_t signed_val;
    uint8_t ready;
    uint8_t ranking;
    uint8_t pudding[SCAN_CTX_PUDDING_SIZE];
} scan_ctx_t;

static uint64_t scan_ms_get(void)
{
    uint64_t ns = 0;

    if (ldg_time_monotonic_ns_get(&ns) != LDG_ERR_AOK) { return UINT64_MAX; }

    return ns / (uint64_t)LDG_NS_PER_MS;
}

uint32_t crosstalk_re_repl_scan_init(crosstalk_re_repl_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->scan = 0x0;

    ret = ldg_mem_alloc((uint64_t)sizeof(scan_ctx_t), &ctx->scan);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_secure_zero(ctx->scan, (uint64_t)sizeof(scan_ctx_t));
}

uint32_t crosstalk_re_repl_scan_shutdown(crosstalk_re_repl_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->scan == 0x0) { return LDG_ERR_AOK; }

    ret = ldg_mem_dealloc(ctx->scan);
    if (ret == LDG_ERR_AOK) { ctx->scan = 0x0; }

    return ret;
}

static void scan_clear(scan_ctx_t *scan)
{
    scan->mode = (uint32_t)SCAN_MODE_IDLE;
    scan->cunt = 0;
    scan->alive_cunt = 0;
    scan->ready = LDG_TRUTH_FALSE;
    scan->ranking = LDG_TRUTH_FALSE;
    scan->best_cunt = 0;
    scan->cursor = 0;
    scan->end = 0;
    scan->idx = 0;
    scan->pass = 0;
    scan->pass_cunt = 0;
    scan->skipped = 0;
    scan->err_cunt = 0;
    scan->lost_cunt = 0;
    scan->pass_span_ms = 0;
    scan->max_dt_ms = 0;
    scan->page_end = 0;
    scan->last_err = LDG_ERR_AOK;
}

uint32_t crosstalk_re_repl_scan_rst(crosstalk_re_repl_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0 || ctx->scan == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    scan_clear((scan_ctx_t *)ctx->scan);

    return LDG_ERR_AOK;
}

static uint32_t scan_page_ld(crosstalk_re_repl_ctx_t *ctx, scan_ctx_t *scan, uint64_t addr, uint64_t *next)
{
    ldg_mem_region_t region = LDG_STRUCT_ZERO_INIT;
    uint64_t end = 0;
    uint64_t got = 0;

    scan->page_end = 0;

    if (ldg_mem_region_get(&ctx->proc, addr, &region) != LDG_ERR_AOK)
    {
        *next = scan->end;
        return LDG_ERR_IO_RD;
    }

    if (region.size > UINT64_MAX - region.base) { return LDG_ERR_OVERFLOW; }

    end = region.base + region.size;
    if (end <= addr) { return LDG_ERR_INVALID; }

    *next = end;

    if (region.committed != LDG_TRUTH_TRUE || region.guard == LDG_TRUTH_TRUE || region.readable != LDG_TRUTH_TRUE) { return LDG_ERR_DENIED; }

    if (end - addr > (uint64_t)SCAN_PAGE_BYTES) { end = addr + (uint64_t)SCAN_PAGE_BYTES; }

    if (end > scan->end) { end = scan->end; }

    *next = end;

    if (end - addr < (uint64_t)scan->bytes) { return LDG_ERR_IO_RD; }

    if (ldg_mem_vmem_rd(&ctx->proc, addr, scan->page, end - addr, &got) != LDG_ERR_AOK || got != end - addr) { return LDG_ERR_IO_RD; }

    scan->page_addr = addr;
    scan->page_end = end;
    scan->page_alloc_base = region.alloc_base;
    scan->page_kind = (uint32_t)region.kind;
    scan->page_ms = scan_ms_get();

    return LDG_ERR_AOK;
}

static uint32_t scan_bits_rd(crosstalk_re_repl_ctx_t *ctx, scan_ctx_t *scan, uint64_t addr, uint64_t *bits, uint64_t *next)
{
    uint32_t ret = 0;

    *bits = 0;
    *next = addr + (uint64_t)scan->stride;

    if (addr < scan->page_addr || addr + (uint64_t)scan->bytes > scan->page_end)
    {
        ret = scan_page_ld(ctx, scan, addr, next);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return ldg_mem_secure_cpy(bits, scan->page + (addr - scan->page_addr), (uint64_t)scan->bytes);
}

static uint32_t scan_env_set(scan_ctx_t *scan, const scan_item_t *item, uint64_t bits)
{
    uint64_t orig_ms = scan->mode == (uint32_t)SCAN_MODE_WATCH || scan->mode == (uint32_t)SCAN_MODE_FIT ? scan->start_ms : item->base_ms;
    uint32_t ret = 0;

    ret = ldg_mem_secure_zero(&scan->env, (uint64_t)sizeof(scan->env));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_re_repl_expr_decode(bits, scan->bytes, scan->fp, scan->signed_val, &scan->env.var[CROSSTALK_RE_REPL_EXPR_VAR_V]);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_re_repl_expr_decode(item->prev, scan->bytes, scan->fp, scan->signed_val, &scan->env.var[CROSSTALK_RE_REPL_EXPR_VAR_P]);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_re_repl_expr_decode(item->base, scan->bytes, scan->fp, scan->signed_val, &scan->env.var[CROSSTALK_RE_REPL_EXPR_VAR_B]);
    if (ret != LDG_ERR_AOK) { return ret; }

    scan->env.var[CROSSTALK_RE_REPL_EXPR_VAR_T].fp = LDG_TRUTH_TRUE;
    scan->env.var[CROSSTALK_RE_REPL_EXPR_VAR_T].real = (double)(scan->page_ms - orig_ms) / (double)LDG_MS_PER_SEC;
    scan->env.var[CROSSTALK_RE_REPL_EXPR_VAR_DT].fp = LDG_TRUTH_TRUE;
    scan->env.var[CROSSTALK_RE_REPL_EXPR_VAR_DT].real = (double)(scan->page_ms - item->prev_ms) / (double)LDG_MS_PER_SEC;
    scan->env.var[CROSSTALK_RE_REPL_EXPR_VAR_N].mag = (uint64_t)scan->pass;

    return LDG_ERR_AOK;
}

static void scan_best_add(scan_ctx_t *scan, uint32_t idx)
{
    uint32_t pos = 0;
    uint32_t mov = 0;

    for (pos = 0; pos < scan->best_cunt; pos = pos + (uint32_t)SCAN_STEP) { if (scan->item[idx].score < scan->item[scan->best[pos]].score) { break; } }

    if (pos == (uint32_t)SCAN_BEST_CAP) { return; }

    if (scan->best_cunt < (uint32_t)SCAN_BEST_CAP) { scan->best_cunt = scan->best_cunt + (uint32_t)SCAN_STEP; }

    for (mov = scan->best_cunt - (uint32_t)SCAN_STEP; mov > pos; mov = mov - (uint32_t)SCAN_STEP) { scan->best[mov] = scan->best[mov - (uint32_t)SCAN_STEP]; }

    scan->best[pos] = idx;
}

static void scan_base_skip(scan_ctx_t *scan, uint64_t next)
{
    uint64_t steps = 0;

    if (next > scan->end) { next = scan->end; }

    scan->skipped = scan->skipped + (next - scan->cursor);
    steps = (next - scan->cursor + (uint64_t)scan->stride - (uint64_t)SCAN_STEP) / (uint64_t)scan->stride;

    if (steps > (scan->end - scan->cursor) / (uint64_t)scan->stride) { scan->cursor = scan->end; }
    else { scan->cursor = scan->cursor + steps * (uint64_t)scan->stride; }
}

static uint32_t scan_base_step(crosstalk_re_repl_ctx_t *ctx, scan_ctx_t *scan)
{
    scan_item_t item = LDG_STRUCT_ZERO_INIT;
    crosstalk_re_repl_expr_val_t val = LDG_STRUCT_ZERO_INIT;
    uint64_t bits = 0;
    uint64_t next = 0;
    uint32_t ret = 0;

    if (scan->end - scan->cursor < (uint64_t)scan->bytes)
    {
        scan->cursor = scan->end;
        return LDG_ERR_AOK;
    }

    ret = scan_bits_rd(ctx, scan, scan->cursor, &bits, &next);
    if (ret != LDG_ERR_AOK)
    {
        if (next <= scan->cursor) { return LDG_ERR_INVALID; }

        scan_base_skip(scan, next);

        return LDG_ERR_AOK;
    }

    item.addr = scan->cursor;
    item.alloc_base = scan->page_alloc_base;
    item.kind = scan->page_kind;
    item.base = bits;
    item.prev = bits;
    item.base_ms = scan->page_ms;
    item.prev_ms = scan->page_ms;
    item.alive = LDG_TRUTH_TRUE;

    ret = scan_env_set(scan, &item, bits);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_re_repl_expr_eval(&scan->expr, &scan->env, &val); }

    if (ret != LDG_ERR_AOK) { scan->err_cunt = scan->err_cunt + (uint32_t)SCAN_STEP; }
    else if (crosstalk_re_repl_expr_truth_is(&val) == LDG_TRUTH_TRUE)
    {
        if (scan->cunt == (uint32_t)SCAN_CANDIDATE_CAP) { return LDG_ERR_FULL; }

        scan->item[scan->cunt] = item;
        scan->cunt = scan->cunt + (uint32_t)SCAN_STEP;
        scan->alive_cunt = scan->alive_cunt + (uint32_t)SCAN_STEP;
    }

    if ((uint64_t)scan->stride > scan->end - scan->cursor) { scan->cursor = scan->end; }
    else { scan->cursor = scan->cursor + (uint64_t)scan->stride; }

    return LDG_ERR_AOK;
}

static uint32_t scan_score_put(scan_item_t *item, const crosstalk_re_repl_expr_val_t *val)
{
    double score = 0x0;
    uint32_t ret = 0;

    ret = crosstalk_re_repl_expr_real_get(val, &score);
    if (ret != LDG_ERR_AOK) { return ret; }

    item->sample_cunt = item->sample_cunt + (uint32_t)SCAN_STEP;
    item->score = item->score + (fabs(score) - item->score) / (double)item->sample_cunt;

    return isfinite(item->score) ? LDG_ERR_AOK : LDG_ERR_RANGE;
}

static void scan_item_drop(scan_ctx_t *scan, scan_item_t *item)
{
    if (item->alive != LDG_TRUTH_TRUE) { return; }

    item->alive = LDG_TRUTH_FALSE;
    scan->alive_cunt = scan->alive_cunt - (uint32_t)SCAN_STEP;
}

static uint32_t scan_sample_step(crosstalk_re_repl_ctx_t *ctx, scan_ctx_t *scan)
{
    scan_item_t *item = &scan->item[scan->idx];
    crosstalk_re_repl_expr_val_t val = LDG_STRUCT_ZERO_INIT;
    uint64_t bits = 0;
    uint64_t next = 0;
    uint32_t ret = 0;

    if (item->alive != LDG_TRUTH_TRUE)
    {
        scan->idx = scan->idx + (uint32_t)SCAN_STEP;
        return LDG_ERR_AOK;
    }

    ret = scan_bits_rd(ctx, scan, item->addr, &bits, &next);
    if (ret != LDG_ERR_AOK || item->alloc_base != scan->page_alloc_base || item->kind != scan->page_kind)
    {
        scan_item_drop(scan, item);
        scan->lost_cunt = scan->lost_cunt + (uint32_t)SCAN_STEP;
        scan->idx = scan->idx + (uint32_t)SCAN_STEP;

        return LDG_ERR_AOK;
    }

    ret = scan_env_set(scan, item, bits);
    if (ret == LDG_ERR_AOK && scan->mode != (uint32_t)SCAN_MODE_REBASE) { ret = crosstalk_re_repl_expr_eval(&scan->expr, &scan->env, &val); }

    if (ret == LDG_ERR_AOK && scan->mode == (uint32_t)SCAN_MODE_FIT) { ret = scan_score_put(item, &val); }

    if (ret != LDG_ERR_AOK)
    {
        scan->err_cunt = scan->err_cunt + (uint32_t)SCAN_STEP;
        scan_item_drop(scan, item);
    }
    else if (scan->mode != (uint32_t)SCAN_MODE_FIT && scan->mode != (uint32_t)SCAN_MODE_REBASE && crosstalk_re_repl_expr_truth_is(&val) != LDG_TRUTH_TRUE) { scan_item_drop(scan, item); }

    if (scan->page_ms - item->prev_ms > scan->max_dt_ms) { scan->max_dt_ms = scan->page_ms - item->prev_ms; }

    item->prev = bits;
    item->prev_ms = scan->page_ms;

    if (scan->mode == (uint32_t)SCAN_MODE_REBASE)
    {
        item->base = bits;
        item->base_ms = scan->page_ms;
        item->score = 0;
        item->sample_cunt = 0;
    }

    if (scan->mode == (uint32_t)SCAN_MODE_FIT && scan->pass == scan->pass_cunt && item->alive == LDG_TRUTH_TRUE) { scan_best_add(scan, scan->idx); }

    scan->idx = scan->idx + (uint32_t)SCAN_STEP;

    return LDG_ERR_AOK;
}

static uint8_t scan_pass_done_is(const scan_ctx_t *scan)
{
    if (scan->mode == (uint32_t)SCAN_MODE_BASE) { return scan->cursor == scan->end ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    return scan->idx == scan->cunt ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t scan_pass_end(scan_ctx_t *scan, uint32_t step_err)
{
    if (scan->mode == (uint32_t)SCAN_MODE_BASE) { scan->ready = LDG_TRUTH_TRUE; }

    scan->pass_span_ms = scan_ms_get() - scan->pass_ms;

    if (step_err != LDG_ERR_AOK || scan->mode == (uint32_t)SCAN_MODE_BASE || scan->pass >= scan->pass_cunt || scan->alive_cunt == 0)
    {
        scan->last_err = step_err;
        scan->ranking = scan->mode == (uint32_t)SCAN_MODE_FIT && step_err == LDG_ERR_AOK ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
        scan->mode = (uint32_t)SCAN_MODE_IDLE;

        return step_err;
    }

    scan->idx = 0;
    scan->pass = scan->pass + (uint32_t)SCAN_STEP;
    scan->next_ms = scan->pass_ms + (uint64_t)scan->interval_ms;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_repl_scan_tick(crosstalk_re_repl_ctx_t *ctx)
{
    scan_ctx_t *scan = 0x0;
    uint64_t now = scan_ms_get();
    uint32_t step = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || ctx->scan == 0x0)) { return LDG_ERR_NOT_INIT; }

    scan = (scan_ctx_t *)ctx->scan;

    if (scan->mode == (uint32_t)SCAN_MODE_IDLE || now < scan->next_ms) { return scan->last_err; }

    if (scan->idx == 0 && scan->mode != (uint32_t)SCAN_MODE_BASE) { scan->pass_ms = now; }

    scan->page_end = 0;

    for (step = 0; step < (uint32_t)SCAN_STEP_CAP; step = step + (uint32_t)SCAN_STEP)
    {
        ret = scan->mode == (uint32_t)SCAN_MODE_BASE ? scan_base_step(ctx, scan) : scan_sample_step(ctx, scan);

        if (ret != LDG_ERR_AOK || scan_pass_done_is(scan) == LDG_TRUTH_TRUE) { return scan_pass_end(scan, ret); }

        if (scan_ms_get() - now >= (uint64_t)SCAN_TIME_BUDGET_MS) { break; }
    }

    return LDG_ERR_AOK;
}

static uint32_t scan_status(crosstalk_re_repl_ctx_t *ctx, const scan_ctx_t *scan)
{
    return crosstalk_re_repl_emit(ctx, "scan; active: %u32; ready: %u32; mode: %u32; candidates: %u32; retained: %u32; sample: %u32/%u32; cursor: 0x%x64; remaining bytes: %u64; skipped bytes: %u64; expression errors: %u32; unreadable/remapped: %u32; pass span ms: %u64; max dt ms: %u64; ranked: %u32; err: %u32", scan->mode != (uint32_t)SCAN_MODE_IDLE ? (uint32_t)LDG_TRUTH_TRUE : (uint32_t)LDG_TRUTH_FALSE, (uint32_t)scan->ready, scan->mode, scan->cunt, scan->alive_cunt, scan->pass, scan->pass_cunt, scan->cursor, scan->end - scan->cursor, scan->skipped, scan->err_cunt, scan->lost_cunt, scan->pass_span_ms, scan->max_dt_ms, (uint32_t)scan->ranking, scan->last_err);
}

static uint32_t scan_expr_compile(crosstalk_re_repl_ctx_t *ctx, scan_ctx_t *scan, uint32_t argc, char **args, uint32_t start)
{
    uint32_t at = 0;
    uint32_t pos = 0;
    uint32_t ch = 0;
    uint32_t ret = 0;

    for (at = start; at < argc; at = at + (uint32_t)SCAN_STEP)
    {
        if (at != start)
        {
            if (pos + LDG_STR_TERM_SIZE >= (uint32_t)sizeof(scan->expr_text)) { return LDG_ERR_FULL; }

            scan->expr_text[pos] = LDG_ASCII_SPACE;
            pos = pos + (uint32_t)SCAN_STEP;
        }

        for (ch = 0; args[at][ch] != LDG_STR_TERM; ch = ch + (uint32_t)SCAN_STEP)
        {
            if (pos + LDG_STR_TERM_SIZE >= (uint32_t)sizeof(scan->expr_text)) { return LDG_ERR_FULL; }

            scan->expr_text[pos] = args[at][ch];
            pos = pos + (uint32_t)SCAN_STEP;
        }
    }

    if (start >= argc)
    {
        scan->expr_text[pos] = SCAN_CH_TRUTH;
        pos = pos + (uint32_t)SCAN_STEP;
    }

    scan->expr_text[pos] = LDG_STR_TERM;

    ret = crosstalk_re_repl_expr_compile(&scan->pending, scan->expr_text);
    if (ret == LDG_ERR_AOK) { return LDG_ERR_AOK; }

    if (crosstalk_re_repl_emit(ctx, "expression rejected; offset: %u32; err: %u32", scan->pending.err_pos, ret) != LDG_ERR_AOK) { return LDG_ERR_FULL; }

    return ret;
}

static uint32_t scan_begin(crosstalk_re_repl_ctx_t *ctx, scan_ctx_t *scan, uint32_t argc, char **args)
{
    uint64_t addr = 0;
    uint64_t len = 0;
    uint64_t stride = 0;
    uint32_t bytes = 0;
    uint32_t ret = 0;
    uint8_t fp = LDG_TRUTH_FALSE;
    uint8_t signed_val = LDG_TRUTH_FALSE;

    if (argc < (uint32_t)SCAN_ARGC_BEGIN_MIN) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = crosstalk_re_repl_native_type_get(args[SCAN_ARG_TYPE], &bytes, &fp, &signed_val);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_re_repl_native_addr_get(ctx, args[SCAN_ARG_ADDR], &addr);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_re_repl_num_get(args[SCAN_ARG_LEN], &len);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_re_repl_num_get(args[SCAN_ARG_STRIDE], &stride);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (len < (uint64_t)bytes || len > (uint64_t)SCAN_RANGE_CAP || len > UINT64_MAX - addr || stride == 0 || stride > (uint64_t)SCAN_PAGE_BYTES) { return LDG_ERR_RANGE; }

    ret = scan_expr_compile(ctx, scan, argc, args, (uint32_t)SCAN_ARG_PREDICATE);
    if (ret != LDG_ERR_AOK) { return ret; }

    scan_clear(scan);
    scan->expr = scan->pending;
    scan->cursor = addr;
    scan->end = addr + len;
    scan->stride = (uint32_t)stride;
    scan->bytes = bytes;
    scan->fp = fp;
    scan->signed_val = signed_val;
    scan->start_ms = scan_ms_get();
    scan->next_ms = scan->start_ms;
    scan->pass_ms = scan->start_ms;
    scan->mode = (uint32_t)SCAN_MODE_BASE;

    return scan_status(ctx, scan);
}

static uint32_t scan_timing_get(uint32_t argc, char **args, uint64_t *out_interval, uint64_t *out_passes)
{
    uint32_t ret = 0;

    if (argc < (uint32_t)SCAN_ARGC_TIMED_MIN) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = crosstalk_re_repl_num_get(args[SCAN_ARG_INTERVAL], out_interval);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_re_repl_num_get(args[SCAN_ARG_PASSES], out_passes);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (*out_interval < (uint64_t)SCAN_INTERVAL_MIN_MS || *out_interval > (uint64_t)SCAN_INTERVAL_MAX_MS || *out_passes == 0 || *out_passes > (uint64_t)SCAN_SAMPLE_CAP) { return LDG_ERR_RANGE; }

    return LDG_ERR_AOK;
}

static uint32_t scan_run(crosstalk_re_repl_ctx_t *ctx, scan_ctx_t *scan, uint32_t argc, char **args, uint32_t mode)
{
    uint64_t interval = 0;
    uint64_t passes = (uint64_t)SCAN_PASS_FIRST;
    uint32_t start = (uint32_t)SCAN_ARG_EXPR;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (scan->ready != LDG_TRUTH_TRUE || scan->alive_cunt == 0) { return LDG_ERR_EMPTY; }

    if (mode == (uint32_t)SCAN_MODE_REBASE && argc != (uint32_t)SCAN_ARGC_VERB) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (mode == (uint32_t)SCAN_MODE_WATCH || mode == (uint32_t)SCAN_MODE_FIT)
    {
        ret = scan_timing_get(argc, args, &interval, &passes);
        if (ret != LDG_ERR_AOK) { return ret; }

        start = (uint32_t)SCAN_ARG_TIMED_EXPR;
    }

    if (mode != (uint32_t)SCAN_MODE_REBASE && argc == start) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = scan_expr_compile(ctx, scan, argc, args, start);
    if (ret != LDG_ERR_AOK) { return ret; }

    scan->expr = scan->pending;
    scan->idx = 0;
    scan->pass = (uint32_t)SCAN_PASS_FIRST;
    scan->pass_cunt = (uint32_t)passes;
    scan->interval_ms = (uint32_t)interval;
    scan->err_cunt = 0;
    scan->lost_cunt = 0;
    scan->max_dt_ms = 0;
    scan->pass_span_ms = 0;
    scan->last_err = LDG_ERR_AOK;
    scan->ranking = LDG_TRUTH_FALSE;
    scan->best_cunt = 0;

    for (at = 0; at < scan->cunt; at = at + (uint32_t)SCAN_STEP)
    {
        scan->item[at].score = 0;
        scan->item[at].sample_cunt = 0;
    }

    scan->start_ms = scan_ms_get();
    scan->next_ms = scan->start_ms + interval;
    scan->mode = mode;

    return scan_status(ctx, scan);
}

static uint32_t scan_row_emit(crosstalk_re_repl_ctx_t *ctx, scan_ctx_t *scan, uint32_t idx, uint32_t row)
{
    const scan_item_t *item = &scan->item[idx];
    crosstalk_re_repl_expr_val_t val = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    ret = crosstalk_re_repl_expr_decode(item->prev, scan->bytes, scan->fp, scan->signed_val, &val);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (val.fp == LDG_TRUTH_TRUE) { return crosstalk_re_repl_emit(ctx, "candidate: %u32; slot: %u32; address: 0x%x64; value: %f; bits: 0x%x64; baseline bits: 0x%x64; mean absolute error: %f; samples: %u32", row, idx, item->addr, val.real, item->prev, item->base, item->score, item->sample_cunt); }

    return crosstalk_re_repl_emit(ctx, "candidate: %u32; slot: %u32; address: 0x%x64; value: %.1s%u64; bits: 0x%x64; baseline bits: 0x%x64; mean absolute error: %f; samples: %u32", row, idx, item->addr, val.neg == LDG_TRUTH_TRUE ? SCAN_SIGN_NEG : SCAN_SIGN_NONE, val.mag, item->prev, item->base, item->score, item->sample_cunt);
}

static uint32_t scan_list(crosstalk_re_repl_ctx_t *ctx, scan_ctx_t *scan, uint32_t argc, char **args, uint8_t best)
{
    uint64_t start = 0;
    uint32_t limit = best == LDG_TRUTH_TRUE ? scan->best_cunt : scan->cunt;
    uint32_t at = 0;
    uint32_t rows = 0;
    uint32_t ret = LDG_ERR_AOK;

    if (argc > (uint32_t)SCAN_ARGC_LIST_MAX) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (argc == (uint32_t)SCAN_ARGC_LIST_MAX) { ret = crosstalk_re_repl_num_get(args[SCAN_ARG_START], &start); }

    if (ret != LDG_ERR_AOK) { return ret; }

    if (start > (uint64_t)limit) { return LDG_ERR_RANGE; }

    if (best == LDG_TRUTH_TRUE && scan->ranking != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    for (at = (uint32_t)start; at < limit && rows < (uint32_t)CROSSTALK_RE_REPL_ROW_CAP; at = at + (uint32_t)SCAN_STEP)
    {
        if (best != LDG_TRUTH_TRUE && scan->item[at].alive != LDG_TRUTH_TRUE) { continue; }

        ret = scan_row_emit(ctx, scan, best == LDG_TRUTH_TRUE ? scan->best[at] : at, at);
        if (ret != LDG_ERR_AOK) { return ret; }

        rows = rows + (uint32_t)SCAN_STEP;
    }

    return crosstalk_re_repl_emit(ctx, "rows: %u32; next start: %u32; retained: %u32; cached samples only", rows, at, scan->alive_cunt);
}

static uint32_t scan_help(crosstalk_re_repl_ctx_t *ctx)
{
    uint32_t ret = 0;

    ret = crosstalk_re_repl_emit(ctx, "scan begin <u8/u16/u32/u64/i8/i16/i32/i64/f32/f64> <address> <bytes> <stride> [predicate]");
    if (ret == LDG_ERR_AOK) { ret = crosstalk_re_repl_emit(ctx, "scan where <predicate> | scan watch <interval-ms> <samples> <predicate> | scan fit <interval-ms> <samples> <residual>"); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_re_repl_emit(ctx, "scan status | scan list [start-slot] | scan best [start-rank] | scan rebase | scan stop | scan clear"); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_re_repl_emit(ctx, "v=current; p=previous retained sample; b=baseline; d=v-p; t=seconds since baseline (watch/fit: job start); dt=seconds since previous sample; n=sample number, baseline=0"); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_re_repl_emit(ctx, "Expressions: decimal/hex/scientific literals, parentheses, + - * / %% ^, < <= > >= == !=, && || !, abs/sin/cos/sqrt/floor/min/max. ^ means power; / yields real. No quotes needed; omit spaces in lengthy expressions."); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_re_repl_emit(ctx, "Examples: scan where d==2 | scan where v<p | scan where abs(d)<0.01 | scan watch 50 40 v>=b | scan fit 50 40 (v-b)-4*t*(2-t)"); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_re_repl_emit(ctx, "Predicates retain nonzero results; watch requires every sample to pass. Fit ranks lowest mean absolute residual; use scan best. No automatic scale/time fitting."); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_re_repl_emit(ctx, "Async jobs: status reports completion/errors; stop preserves partial changes. Capacity 32768 candidates; full retains the partial baseline and reports err. Narrow the range or the first predicate. Max range 256 MiB; stride starts at the supplied address."); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_re_repl_emit(ctx, "At most 256 slots/tick, 2 ms budget. Samples are not simultaneous; actual t/dt and pass span matter. 16..60000 ms requested interval, 1..4096 samples. No catch-up samples."); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_re_repl_emit(ctx, "64-bit integer +,-,*,%% and comparisons are exact with overflow rejection. Mixed real arithmetic rejects integer magnitudes above 2^53. NaN/Inf, invalid arithmetic and unreadable/remapped candidates are discarded and counted."); }

    return ret;
}

static uint32_t scan_stop(crosstalk_re_repl_ctx_t *ctx, scan_ctx_t *scan)
{
    if (scan->mode == (uint32_t)SCAN_MODE_BASE) { scan->ready = LDG_TRUTH_TRUE; }

    if (scan->mode != (uint32_t)SCAN_MODE_IDLE) { scan->last_err = LDG_ERR_CANCELED; }

    scan->mode = (uint32_t)SCAN_MODE_IDLE;

    return scan_status(ctx, scan);
}

uint32_t crosstalk_re_repl_scan_cmd(crosstalk_re_repl_ctx_t *ctx, uint32_t argc, char **args)
{
    scan_ctx_t *scan = 0x0;
    const char *verb = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0 || ctx->scan == 0x0 || args == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(argc == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    scan = (scan_ctx_t *)ctx->scan;

    if (argc == (uint32_t)SCAN_ARGC_ROOT) { return scan_help(ctx); }

    verb = args[SCAN_ARG_VERB];

    if (crosstalk_re_repl_cmd_eq_is(verb, SCAN_WORD_HELP) == LDG_TRUTH_TRUE) { return argc == (uint32_t)SCAN_ARGC_VERB ? scan_help(ctx) : LDG_ERR_FUNC_ARG_INVALID; }

    if (crosstalk_re_repl_cmd_eq_is(verb, SCAN_WORD_STATUS) == LDG_TRUTH_TRUE) { return argc == (uint32_t)SCAN_ARGC_VERB ? scan_status(ctx, scan) : LDG_ERR_FUNC_ARG_INVALID; }

    if (crosstalk_re_repl_cmd_eq_is(verb, SCAN_WORD_STOP) == LDG_TRUTH_TRUE) { return argc == (uint32_t)SCAN_ARGC_VERB ? scan_stop(ctx, scan) : LDG_ERR_FUNC_ARG_INVALID; }

    if (scan->mode != (uint32_t)SCAN_MODE_IDLE) { return LDG_ERR_BUSY; }

    if (crosstalk_re_repl_cmd_eq_is(verb, SCAN_WORD_BEGIN) == LDG_TRUTH_TRUE) { return scan_begin(ctx, scan, argc, args); }

    if (crosstalk_re_repl_cmd_eq_is(verb, SCAN_WORD_WHERE) == LDG_TRUTH_TRUE) { return scan_run(ctx, scan, argc, args, (uint32_t)SCAN_MODE_WHERE); }

    if (crosstalk_re_repl_cmd_eq_is(verb, SCAN_WORD_WATCH) == LDG_TRUTH_TRUE) { return scan_run(ctx, scan, argc, args, (uint32_t)SCAN_MODE_WATCH); }

    if (crosstalk_re_repl_cmd_eq_is(verb, SCAN_WORD_FIT) == LDG_TRUTH_TRUE) { return scan_run(ctx, scan, argc, args, (uint32_t)SCAN_MODE_FIT); }

    if (crosstalk_re_repl_cmd_eq_is(verb, SCAN_WORD_REBASE) == LDG_TRUTH_TRUE) { return scan_run(ctx, scan, argc, args, (uint32_t)SCAN_MODE_REBASE); }

    if (crosstalk_re_repl_cmd_eq_is(verb, SCAN_WORD_LIST) == LDG_TRUTH_TRUE) { return scan_list(ctx, scan, argc, args, LDG_TRUTH_FALSE); }

    if (crosstalk_re_repl_cmd_eq_is(verb, SCAN_WORD_BEST) == LDG_TRUTH_TRUE) { return scan_list(ctx, scan, argc, args, LDG_TRUTH_TRUE); }

    if (crosstalk_re_repl_cmd_eq_is(verb, SCAN_WORD_CLEAR) == LDG_TRUTH_TRUE)
    {
        if (argc != (uint32_t)SCAN_ARGC_VERB) { return LDG_ERR_FUNC_ARG_INVALID; }

        scan_clear(scan);

        return scan_status(ctx, scan);
    }

    return LDG_ERR_FUNC_ARG_INVALID;
}
