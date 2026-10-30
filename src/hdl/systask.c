#include <stdint.h>
#include <string.h>

#include <dangling/core/arith.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/mem.h>
#include <dangling/str/str.h>

#include "hdl/exec.h"
#include "hdl/ir.h"
#include "hdl/systask.h"

#define SYSTASK_STEP 1
#define SYSTASK_PAIR 2
#define SYSTASK_WORD_BITS 64U
#define SYSTASK_WORD_CLEAR 0ULL
#define SYSTASK_HEX_BITS 4U
#define SYSTASK_OCT_BITS 3U
#define SYSTASK_BIN_BITS 1U
#define SYSTASK_RADIX_DEC 10U
#define SYSTASK_RADIX_HEX 16U
#define SYSTASK_RADIX_OCT 8U
#define SYSTASK_RADIX_BIN 2U
#define SYSTASK_DIGIT_CAP 136
#define SYSTASK_TIME_FILL 20U
#define SYSTASK_NS_PER_SEC 1000000000.0
#define SYSTASK_NS_UNKNOWN 0.0
#define SYSTASK_ROUND_HALF 0.5
#define SYSTASK_FILL_NONE 0U
#define SYSTASK_LIMB_LO 0
#define SYSTASK_LIMB_HI 1
#define SYSTASK_DIGITS "0123456789abcdef"
#define SYSTASK_LF '\n'
#define SYSTASK_TAB '\t'
#define SYSTASK_SPEC '%'
#define SYSTASK_FILL_ZERO '0'
#define SYSTASK_FILL_SPACE ' '
#define SYSTASK_NEG_MARK '-'
#define SYSTASK_UNKNOWN_MARK '?'
#define SYSTASK_ESC_LF 'n'
#define SYSTASK_ESC_TAB 't'
#define SYSTASK_DIGIT_LO '0'
#define SYSTASK_DIGIT_HI '9'
#define SYSTASK_LOWER_BIT 0x20
#define SYSTASK_SPEC_DEC 'd'
#define SYSTASK_SPEC_HEX 'h'
#define SYSTASK_SPEC_HEX_ALT 'x'
#define SYSTASK_SPEC_OCT 'o'
#define SYSTASK_SPEC_BIN 'b'
#define SYSTASK_SPEC_CH 'c'
#define SYSTASK_SPEC_STR 's'
#define SYSTASK_SPEC_TIME 't'
#define SYSTASK_SPEC_SCOPE 'm'
#define SYSTASK_SPEC_LIB 'l'

typedef struct systask_val
{
    uint64_t lo;
    uint64_t hi;
    uint32_t width;
    uint8_t sgn;
    uint8_t pudding[3];
} systask_val_t;

static uint32_t systask_arr_alloc(void *pool, uint64_t cunt, uint64_t item_size, void **out)
{
    uint64_t bytes = 0;
    uint32_t ret = 0;

    *out = 0x0;

    if (cunt == 0) { cunt = SYSTASK_STEP; }

    ret = ldg_arith_uint64_mul(cunt, item_size, &bytes);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_OVERFLOW; }

    return ldg_mem_pool_alloc((ldg_mem_pool_t *)pool, bytes, out);
}

static uint64_t systask_mask_get(uint32_t width)
{
    if (width >= SYSTASK_WORD_BITS) { return ~(uint64_t)0x0; }

    if (width == 0) { return SYSTASK_WORD_CLEAR; }

    return ((uint64_t)SYSTASK_STEP << width) - (uint64_t)SYSTASK_STEP;
}

static void systask_pend_emit(crosstalk_hdl_systask_ctx_t *ctx, uint8_t force)
{
    uint32_t at = 0;

    while (ctx->pend_len != 0)
    {
        for (at = 0; at < ctx->pend_len && ctx->pend[at] != SYSTASK_LF; at = at + SYSTASK_STEP) {}

        if (at >= ctx->pend_len && force != LDG_TRUTH_TRUE && ctx->pend_len + SYSTASK_STEP < (uint32_t)CROSSTALK_HDL_SYSTASK_LINE_CAP) { return; }

        ctx->pend[at < ctx->pend_len ? at : ctx->pend_len] = LDG_STR_TERM;

        if (ctx->out_cb != 0x0) { ctx->out_cb(ctx->out_user, ctx->pend); }

        if (at >= ctx->pend_len)
        {
            ctx->pend_len = 0;
            return;
        }

        memmove(ctx->pend, &ctx->pend[at + SYSTASK_STEP], (uint64_t)(ctx->pend_len - at - SYSTASK_STEP));
        ctx->pend_len = ctx->pend_len - at - SYSTASK_STEP;
    }
}

static void systask_ch_put(crosstalk_hdl_systask_ctx_t *ctx, char ch)
{
    if (ctx->pend_len + SYSTASK_STEP >= (uint32_t)CROSSTALK_HDL_SYSTASK_LINE_CAP) { systask_pend_emit(ctx, LDG_TRUTH_TRUE); }

    ctx->pend[ctx->pend_len] = ch;
    ctx->pend_len = ctx->pend_len + SYSTASK_STEP;
}

static void systask_text_put(crosstalk_hdl_systask_ctx_t *ctx, const char *text, uint64_t len)
{
    uint64_t at = 0;

    for (at = 0; at < len && text[at] != LDG_STR_TERM; at = at + SYSTASK_STEP) { systask_ch_put(ctx, text[at]); }
}

static void systask_fill_put(crosstalk_hdl_systask_ctx_t *ctx, char ch, uint32_t cunt)
{
    uint32_t at = 0;

    for (at = 0; at < cunt; at = at + SYSTASK_STEP) { systask_ch_put(ctx, ch); }
}

static uint32_t systask_digits_make(uint64_t lo, uint64_t hi, uint32_t radix, char *out, uint32_t *out_len)
{
    ldg_uint128_t cur = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t base = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t quot = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t rem = LDG_STRUCT_ZERO_INIT;
    uint32_t len = 0;
    uint32_t ret = 0;

    *out_len = 0;

    cur.limb[SYSTASK_LIMB_LO] = lo;
    cur.limb[SYSTASK_LIMB_HI] = hi;
    base.limb[SYSTASK_LIMB_LO] = (uint64_t)radix;

    do
    {
        if (len >= (uint32_t)SYSTASK_DIGIT_CAP) { return LDG_ERR_FULL; }

        if (cur.limb[SYSTASK_LIMB_HI] == 0)
        {
            out[len] = SYSTASK_DIGITS[cur.limb[SYSTASK_LIMB_LO] % (uint64_t)radix];
            cur.limb[SYSTASK_LIMB_LO] = cur.limb[SYSTASK_LIMB_LO] / (uint64_t)radix;
        }
        else
        {
            ret = ldg_arith_uint128_div_compute(&cur, &base, &quot, &rem);
            if (ret != LDG_ERR_AOK) { return ret; }

            out[len] = SYSTASK_DIGITS[rem.limb[SYSTASK_LIMB_LO]];
            cur = quot;
        }

        len = len + SYSTASK_STEP;
    } while (cur.limb[SYSTASK_LIMB_LO] != 0 || cur.limb[SYSTASK_LIMB_HI] != 0);

    *out_len = len;

    return LDG_ERR_AOK;
}

static uint32_t systask_fill_default_get(uint32_t width, uint32_t radix, uint8_t sgn)
{
    char digits[SYSTASK_DIGIT_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t hi = width > SYSTASK_WORD_BITS ? systask_mask_get(width - SYSTASK_WORD_BITS) : SYSTASK_WORD_CLEAR;
    uint32_t len = 0;

    if (radix == SYSTASK_RADIX_HEX) { return (width + SYSTASK_HEX_BITS - SYSTASK_STEP) / SYSTASK_HEX_BITS; }

    if (radix == SYSTASK_RADIX_OCT) { return (width + SYSTASK_OCT_BITS - SYSTASK_STEP) / SYSTASK_OCT_BITS; }

    if (radix == SYSTASK_RADIX_BIN) { return width; }

    if (systask_digits_make(systask_mask_get(width), hi, SYSTASK_RADIX_DEC, digits, &len) != LDG_ERR_AOK) { return SYSTASK_FILL_NONE; }

    return sgn == LDG_TRUTH_TRUE ? len + SYSTASK_STEP : len;
}

static void systask_num_put(crosstalk_hdl_systask_ctx_t *ctx, const systask_val_t *val, uint32_t radix, uint32_t fill, uint8_t fill_set)
{
    char digits[SYSTASK_DIGIT_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t lo = val->lo;
    uint64_t hi = val->hi;
    uint32_t len = 0;
    uint32_t want = fill;
    uint8_t neg = LDG_TRUTH_FALSE;

    if (radix == SYSTASK_RADIX_DEC && val->sgn == LDG_TRUTH_TRUE && val->width != 0)
    {
        uint8_t top_set = val->width > SYSTASK_WORD_BITS ? (uint8_t)((hi >> (val->width - SYSTASK_WORD_BITS - SYSTASK_STEP)) & (uint64_t)SYSTASK_STEP) : (uint8_t)((lo >> (val->width - SYSTASK_STEP)) & (uint64_t)SYSTASK_STEP);

        if (top_set != 0)
        {
            neg = LDG_TRUTH_TRUE;
            lo = ~lo;
            hi = ~hi;
            lo = lo + (uint64_t)SYSTASK_STEP;

            if (lo == 0) { hi = hi + (uint64_t)SYSTASK_STEP; }

            lo = val->width >= SYSTASK_WORD_BITS ? lo : lo & systask_mask_get(val->width);
            hi = val->width > SYSTASK_WORD_BITS ? hi & systask_mask_get(val->width - SYSTASK_WORD_BITS) : SYSTASK_WORD_CLEAR;
        }
    }

    if (fill_set != LDG_TRUTH_TRUE) { want = systask_fill_default_get(val->width, radix, val->sgn); }

    if (systask_digits_make(lo, hi, radix, digits, &len) != LDG_ERR_AOK)
    {
        systask_ch_put(ctx, SYSTASK_UNKNOWN_MARK);
        return;
    }

    if (neg == LDG_TRUTH_TRUE) { len = len + SYSTASK_STEP; }

    if (want > len) { systask_fill_put(ctx, radix == SYSTASK_RADIX_DEC ? SYSTASK_FILL_SPACE : SYSTASK_FILL_ZERO, want - len); }

    if (neg == LDG_TRUTH_TRUE)
    {
        systask_ch_put(ctx, SYSTASK_NEG_MARK);
        len = len - SYSTASK_STEP;
    }

    while (len != 0)
    {
        len = len - SYSTASK_STEP;
        systask_ch_put(ctx, digits[len]);
    }
}

static void systask_str_val_put(crosstalk_hdl_systask_ctx_t *ctx, const systask_val_t *val)
{
    uint32_t bytes = (val->width + (uint32_t)LDG_BYTE_BITS - SYSTASK_STEP) / (uint32_t)LDG_BYTE_BITS;
    uint32_t at = 0;
    uint8_t lead = LDG_TRUTH_TRUE;

    for (at = bytes; at != 0; at = at - SYSTASK_STEP)
    {
        uint32_t byte_idx = at - SYSTASK_STEP;
        uint32_t shift = (byte_idx * (uint32_t)LDG_BYTE_BITS) % SYSTASK_WORD_BITS;
        uint64_t word = byte_idx * (uint32_t)LDG_BYTE_BITS >= SYSTASK_WORD_BITS ? val->hi : val->lo;
        char ch = (char)((word >> shift) & (uint64_t)LDG_BYTE_MASK);

        if (ch == LDG_STR_TERM && lead == LDG_TRUTH_TRUE) { continue; }

        lead = LDG_TRUTH_FALSE;

        systask_ch_put(ctx, ((uint8_t)ch < (uint8_t)LDG_ASCII_PRINTABLE_MIN || (uint8_t)ch > (uint8_t)LDG_ASCII_PRINTABLE_MAX) ? (char)LDG_ASCII_NONPRINTABLE_CHAR : ch);
    }
}

static void systask_lit_bounds_get(const char *src, const crosstalk_hdl_tok_t *tok, uint32_t *out_start, uint32_t *out_end)
{
    uint32_t start = tok->off;
    uint32_t end = tok->off + tok->len;

    if (start < end && src[start] == (char)LDG_ASCII_QUOTE) { start = start + SYSTASK_STEP; }

    if (end > start && src[end - SYSTASK_STEP] == (char)LDG_ASCII_QUOTE) { end = end - SYSTASK_STEP; }

    *out_start = start;
    *out_end = end;
}

static void systask_lit_put(crosstalk_hdl_systask_ctx_t *ctx, const crosstalk_hdl_tok_t *tok)
{
    uint32_t start = 0;
    uint32_t end = 0;

    systask_lit_bounds_get(ctx->src, tok, &start, &end);
    systask_text_put(ctx, &ctx->src[start], (uint64_t)(end - start));
}

static void systask_val_get(const crosstalk_hdl_exec_ctx_t *exec, const crosstalk_hdl_ir_systask_arg_t *arg, systask_val_t *out)
{
    out->width = arg->width;
    out->sgn = arg->sgn;
    out->lo = exec->vals[arg->sig];
    out->hi = arg->width > SYSTASK_WORD_BITS ? exec->vals_hi[arg->sig] : SYSTASK_WORD_CLEAR;

    if (arg->width < SYSTASK_WORD_BITS) { out->lo = out->lo & systask_mask_get(arg->width); }
}

static void systask_spec_put(crosstalk_hdl_systask_ctx_t *ctx, const crosstalk_hdl_exec_ctx_t *exec, const crosstalk_hdl_ir_systask_t *task, char spec, uint32_t fill, uint8_t fill_set, uint32_t *io_at)
{
    const crosstalk_hdl_ir_systask_arg_t *arg = 0x0;
    systask_val_t val = LDG_STRUCT_ZERO_INIT;
    uint32_t radix = SYSTASK_RADIX_DEC;

    if (spec == SYSTASK_SPEC)
    {
        systask_ch_put(ctx, SYSTASK_SPEC);
        return;
    }

    if (spec == SYSTASK_SPEC_SCOPE || spec == SYSTASK_SPEC_LIB)
    {
        systask_text_put(ctx, ctx->scope, (uint64_t)sizeof(ctx->scope));
        return;
    }

    if (spec == SYSTASK_SPEC_TIME)
    {
        val.lo = (uint64_t)((double)exec->cycle_cunt * ctx->ns_per_cycle + SYSTASK_ROUND_HALF);
        val.width = SYSTASK_WORD_BITS;

        systask_num_put(ctx, &val, SYSTASK_RADIX_DEC, fill_set == LDG_TRUTH_TRUE ? fill : SYSTASK_TIME_FILL, LDG_TRUTH_TRUE);
        return;
    }

    if (*io_at >= task->arg_cunt)
    {
        systask_ch_put(ctx, SYSTASK_UNKNOWN_MARK);
        return;
    }

    arg = &ctx->ir_ctx->systask_args[task->arg_start + *io_at];
    *io_at = *io_at + SYSTASK_STEP;

    if (arg->str_is == LDG_TRUTH_TRUE)
    {
        systask_lit_put(ctx, &arg->str);
        return;
    }

    systask_val_get(exec, arg, &val);

    if (spec == SYSTASK_SPEC_CH)
    {
        systask_ch_put(ctx, (char)(val.lo & (uint64_t)LDG_BYTE_MASK));
        return;
    }

    if (spec == SYSTASK_SPEC_STR)
    {
        systask_str_val_put(ctx, &val);
        return;
    }

    if (spec == SYSTASK_SPEC_HEX || spec == SYSTASK_SPEC_HEX_ALT) { radix = SYSTASK_RADIX_HEX; }
    else if (spec == SYSTASK_SPEC_OCT) { radix = SYSTASK_RADIX_OCT; }
    else if (spec == SYSTASK_SPEC_BIN) { radix = SYSTASK_RADIX_BIN; }
    else if (spec != SYSTASK_SPEC_DEC)
    {
        systask_ch_put(ctx, SYSTASK_UNKNOWN_MARK);
        return;
    }

    if (radix != SYSTASK_RADIX_DEC) { val.sgn = LDG_TRUTH_FALSE; }

    systask_num_put(ctx, &val, radix, fill, fill_set);
}

static void systask_fmt_run(crosstalk_hdl_systask_ctx_t *ctx, const crosstalk_hdl_exec_ctx_t *exec, const crosstalk_hdl_ir_systask_t *task, const crosstalk_hdl_tok_t *fmt_tok, uint32_t *io_at)
{
    uint32_t start = 0;
    uint32_t end = 0;
    uint32_t pos = 0;

    systask_lit_bounds_get(ctx->src, fmt_tok, &start, &end);

    for (pos = start; pos < end; pos = pos + SYSTASK_STEP)
    {
        char ch = ctx->src[pos];
        uint32_t fill = 0;
        uint8_t fill_set = LDG_TRUTH_FALSE;

        if (ch == (char)LDG_ASCII_BSLASH && pos + SYSTASK_STEP < end)
        {
            char esc = ctx->src[pos + SYSTASK_STEP];

            pos = pos + SYSTASK_STEP;

            if (esc == SYSTASK_ESC_LF) { systask_ch_put(ctx, SYSTASK_LF); }
            else if (esc == SYSTASK_ESC_TAB) { systask_ch_put(ctx, SYSTASK_TAB); }
            else { systask_ch_put(ctx, esc); }

            continue;
        }

        if (ch != SYSTASK_SPEC || pos + SYSTASK_STEP >= end)
        {
            systask_ch_put(ctx, ch);
            continue;
        }

        pos = pos + SYSTASK_STEP;

        while (pos < end && ctx->src[pos] >= SYSTASK_DIGIT_LO && ctx->src[pos] <= SYSTASK_DIGIT_HI)
        {
            fill = fill * SYSTASK_RADIX_DEC + (uint32_t)(ctx->src[pos] - SYSTASK_DIGIT_LO);
            fill_set = LDG_TRUTH_TRUE;
            pos = pos + SYSTASK_STEP;
        }

        if (pos >= end) { break; }

        ch = ctx->src[pos];

        if (ldg_char_alpha_is(ch) == LDG_TRUTH_TRUE) { ch = (char)(ch | SYSTASK_LOWER_BIT); }

        systask_spec_put(ctx, exec, task, ch, fill, fill_set, io_at);
    }
}

static void systask_task_run(crosstalk_hdl_systask_ctx_t *ctx, const crosstalk_hdl_exec_ctx_t *exec, const crosstalk_hdl_ir_systask_t *task)
{
    systask_val_t val = LDG_STRUCT_ZERO_INIT;
    uint32_t at = 0;

    if (task->kind == CROSSTALK_HDL_IR_SYSTASK_FINISH)
    {
        ctx->finish_req = LDG_TRUTH_TRUE;
        return;
    }

    while (at < task->arg_cunt)
    {
        const crosstalk_hdl_ir_systask_arg_t *arg = &ctx->ir_ctx->systask_args[task->arg_start + at];

        at = at + SYSTASK_STEP;

        if (arg->str_is == LDG_TRUTH_TRUE)
        {
            systask_fmt_run(ctx, exec, task, &arg->str, &at);
            continue;
        }

        systask_val_get(exec, arg, &val);
        systask_num_put(ctx, &val, SYSTASK_RADIX_DEC, SYSTASK_FILL_NONE, LDG_TRUTH_FALSE);
    }

    if (task->kind == CROSSTALK_HDL_IR_SYSTASK_DISPLAY) { systask_ch_put(ctx, SYSTASK_LF); }

    systask_pend_emit(ctx, LDG_TRUTH_FALSE);
}

static uint8_t systask_guard_is(const crosstalk_hdl_exec_ctx_t *exec, uint32_t guard)
{
    if (guard == UINT32_MAX) { return LDG_TRUTH_TRUE; }

    return exec->vals[guard] != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_hdl_systask_init(crosstalk_hdl_systask_ctx_t *ctx, const crosstalk_hdl_ir_t *ir_ctx, const char *src, void *pool)
{
    void *raw = 0x0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || ir_ctx == 0x0 || src == 0x0 || pool == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->ir_ctx = ir_ctx;
    ctx->src = src;

    ret = systask_arr_alloc(pool, (uint64_t)ir_ctx->seq_blk_cunt, (uint64_t)sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->blk_hd = (uint32_t *)raw;

    ret = systask_arr_alloc(pool, (uint64_t)ir_ctx->seq_blk_cunt, (uint64_t)sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->blk_cunt = (uint32_t *)raw;

    for (idx = 0; idx < ir_ctx->seq_blk_cunt; idx = idx + SYSTASK_STEP)
    {
        ctx->blk_hd[idx] = UINT32_MAX;
        ctx->blk_cunt[idx] = 0;
    }

    ret = systask_arr_alloc(pool, (uint64_t)ir_ctx->systask_cunt, (uint64_t)sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->comb_task = (uint32_t *)raw;

    ret = systask_arr_alloc(pool, (uint64_t)ir_ctx->systask_cunt, (uint64_t)sizeof(uint8_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->comb_seen = (uint8_t *)raw;

    ret = systask_arr_alloc(pool, (uint64_t)ir_ctx->systask_cunt, (uint64_t)sizeof(uint8_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->comb_guard = (uint8_t *)raw;

    ret = systask_arr_alloc(pool, (uint64_t)ir_ctx->systask_arg_cunt, (uint64_t)sizeof(uint64_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->comb_lo = (uint64_t *)raw;

    ret = systask_arr_alloc(pool, (uint64_t)ir_ctx->systask_arg_cunt, (uint64_t)sizeof(uint64_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->comb_hi = (uint64_t *)raw;

    for (idx = 0; idx < ir_ctx->systask_cunt; idx = idx + SYSTASK_STEP)
    {
        const crosstalk_hdl_ir_systask_t *task = &ir_ctx->systasks[idx];

        ctx->comb_seen[idx] = LDG_TRUTH_FALSE;
        ctx->comb_guard[idx] = LDG_TRUTH_FALSE;

        if (task->where == CROSSTALK_HDL_IR_SYSTASK_WHERE_COMB)
        {
            ctx->comb_task[ctx->comb_cunt] = idx;
            ctx->comb_cunt = ctx->comb_cunt + SYSTASK_STEP;
            continue;
        }

        if (task->where != CROSSTALK_HDL_IR_SYSTASK_WHERE_SEQ) { continue; }

        if (LDG_UNLIKELY(task->blk >= ir_ctx->seq_blk_cunt)) { return LDG_ERR_BOUNDS; }

        if (ctx->blk_hd[task->blk] == UINT32_MAX) { ctx->blk_hd[task->blk] = idx; }
        else if (LDG_UNLIKELY(ctx->blk_hd[task->blk] + ctx->blk_cunt[task->blk] != idx)) { return LDG_ERR_INVALID; }

        ctx->blk_cunt[task->blk] = ctx->blk_cunt[task->blk] + SYSTASK_STEP;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_systask_out_cb_set(crosstalk_hdl_systask_ctx_t *ctx, crosstalk_hdl_systask_out_cb_t cb, void *user)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->out_cb = cb;
    ctx->out_user = user;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_systask_scope_set(crosstalk_hdl_systask_ctx_t *ctx, const char *scope, uint64_t f_op_hz)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || scope == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_str_rbrcpy(ctx->scope, scope, (uint64_t)sizeof(ctx->scope));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    ctx->ns_per_cycle = f_op_hz == 0 ? SYSTASK_NS_UNKNOWN : SYSTASK_NS_PER_SEC / (double)f_op_hz;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_systask_blk_run(crosstalk_hdl_systask_ctx_t *ctx, const crosstalk_hdl_exec_ctx_t *exec, uint32_t blk_idx)
{
    uint32_t idx = 0;
    uint32_t hd = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || exec == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (blk_idx >= ctx->ir_ctx->seq_blk_cunt) { return LDG_ERR_AOK; }

    hd = ctx->blk_hd[blk_idx];
    if (hd == UINT32_MAX) { return LDG_ERR_AOK; }

    for (idx = hd; idx < hd + ctx->blk_cunt[blk_idx]; idx = idx + SYSTASK_STEP)
    {
        const crosstalk_hdl_ir_systask_t *task = &ctx->ir_ctx->systasks[idx];

        if (systask_guard_is(exec, task->guard) == LDG_TRUTH_TRUE) { systask_task_run(ctx, exec, task); }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_systask_settle_run(crosstalk_hdl_systask_ctx_t *ctx, const crosstalk_hdl_exec_ctx_t *exec)
{
    uint32_t idx = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || exec == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->initial_done != LDG_TRUTH_TRUE)
    {
        ctx->initial_done = LDG_TRUTH_TRUE;

        for (idx = 0; idx < ctx->ir_ctx->systask_cunt; idx = idx + SYSTASK_STEP) { if (ctx->ir_ctx->systasks[idx].where == CROSSTALK_HDL_IR_SYSTASK_WHERE_INITIAL) { systask_task_run(ctx, exec, &ctx->ir_ctx->systasks[idx]); } }
    }

    for (at = 0; at < ctx->comb_cunt; at = at + SYSTASK_STEP)
    {
        uint32_t task_idx = ctx->comb_task[at];
        const crosstalk_hdl_ir_systask_t *task = &ctx->ir_ctx->systasks[task_idx];
        uint8_t guard = systask_guard_is(exec, task->guard);
        uint8_t changed = ctx->comb_seen[task_idx] != LDG_TRUTH_TRUE || ctx->comb_guard[task_idx] != guard ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

        for (idx = task->arg_start; idx < task->arg_start + task->arg_cunt; idx = idx + SYSTASK_STEP)
        {
            const crosstalk_hdl_ir_systask_arg_t *arg = &ctx->ir_ctx->systask_args[idx];
            systask_val_t val = LDG_STRUCT_ZERO_INIT;

            if (arg->str_is == LDG_TRUTH_TRUE) { continue; }

            systask_val_get(exec, arg, &val);

            if (val.lo != ctx->comb_lo[idx] || val.hi != ctx->comb_hi[idx]) { changed = LDG_TRUTH_TRUE; }

            ctx->comb_lo[idx] = val.lo;
            ctx->comb_hi[idx] = val.hi;
        }

        ctx->comb_seen[task_idx] = LDG_TRUTH_TRUE;
        ctx->comb_guard[task_idx] = guard;

        if (changed == LDG_TRUTH_TRUE && guard == LDG_TRUTH_TRUE) { systask_task_run(ctx, exec, task); }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_systask_final_run(crosstalk_hdl_systask_ctx_t *ctx, const crosstalk_hdl_exec_ctx_t *exec)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || exec == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->final_done == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ctx->final_done = LDG_TRUTH_TRUE;

    for (idx = 0; idx < ctx->ir_ctx->systask_cunt; idx = idx + SYSTASK_STEP)
    {
        const crosstalk_hdl_ir_systask_t *task = &ctx->ir_ctx->systasks[idx];

        if (task->where != CROSSTALK_HDL_IR_SYSTASK_WHERE_FINAL) { continue; }

        if (systask_guard_is(exec, task->guard) == LDG_TRUTH_TRUE) { systask_task_run(ctx, exec, task); }
    }

    systask_pend_emit(ctx, LDG_TRUTH_TRUE);

    return LDG_ERR_AOK;
}

uint8_t crosstalk_hdl_systask_finish_is(const crosstalk_hdl_systask_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->finish_req == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}
