#include <math.h>
#include <stdint.h>

#include <dangling/core/arith.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/region.h>
#include <dangling/mem/secure.h>
#include <dangling/mem/vmem.h>
#include <dangling/str/str.h>
#include <dangling/sys/mod.h>
#include <dangling/sys/proc.h>

#include "re/repl/expr.h"
#include "re/repl/native.h"
#include "re/repl/repl.h"
#include "re/repl/scan.h"

#define NATIVE_STEP 1
#define NATIVE_PEEK_CAP 256
#define NATIVE_PEEK_DEFAULT 64
#define NATIVE_HEX_ROW 16
#define NATIVE_HEX_PAIR 2
#define NATIVE_HEX_CELL 3
#define NATIVE_PATTERN_CAP 128
#define NATIVE_CHUNK (4 * LDG_KIB)
#define NATIVE_CHUNK_STEPS 4
#define NATIVE_SEARCH_MAX (256 * LDG_MIB)
#define NATIVE_MOD_CAP 1024
#define NATIVE_PATH_CAP 512
#define NATIVE_WR_CAP 128
#define NATIVE_PTR_BYTES 8
#define NATIVE_BYTES_BYTE 1
#define NATIVE_BYTES_WORD 2
#define NATIVE_BYTES_DWORD 4
#define NATIVE_BYTES_QWORD 8
#define NATIVE_ARG_VERB 0
#define NATIVE_ARG_X 1
#define NATIVE_ARG_Y 2
#define NATIVE_ARG_Z 3
#define NATIVE_ARGC_VERB 1
#define NATIVE_ARGC_X 2
#define NATIVE_ARGC_Y 3
#define NATIVE_ARGC_Z 4
#define NATIVE_EXIT_CODE 0
#define NATIVE_FP32_MANT_BITS 24
#define NATIVE_FP32_FRACTION_BITS 23
#define NATIVE_FP32_EXPONENT_MAX 127
#define NATIVE_FP32_EXPONENT_MIN (-126)
#define NATIVE_FP32_BIAS 127
#define NATIVE_FP32_SUBNORMAL_SHIFT 149
#define NATIVE_FP32_SIGN_BIT 0x80000000ULL
#define NATIVE_FP32_SIGN_CLEAR 0x0ULL
#define NATIVE_FP32_IMPLICIT_BIT 0x800000ULL
#define NATIVE_FP32_MANT_END 0x1000000ULL
#define NATIVE_REAL_NONE 0.0
#define NATIVE_CH_PLUS '+'
#define NATIVE_CH_MINUS '-'
#define NATIVE_CH_POINT '.'
#define NATIVE_CH_EXP_LOWER 'e'
#define NATIVE_CH_EXP_UPPER 'E'
#define NATIVE_CH_WILD '?'
#define NATIVE_TYPE_UNSIGNED "uint"
#define NATIVE_TYPE_SIGNED "int"
#define NATIVE_TYPE_REAL "fp"
#define NATIVE_TYPE_PREFIX_AT 0
#define NATIVE_WORD_EXE "exe"
#define NATIVE_WORD_PTR "ptr"
#define NATIVE_WORD_8 "8"
#define NATIVE_WORD_16 "16"
#define NATIVE_WORD_32 "32"
#define NATIVE_WORD_64 "64"
#define NATIVE_WORD_STATUS "status"
#define NATIVE_WORD_MODULES "modules"
#define NATIVE_WORD_SYM "symbol"
#define NATIVE_WORD_ADDR "address"
#define NATIVE_WORD_REGIONS "regions"
#define NATIVE_WORD_PEEK "peek"
#define NATIVE_WORD_RD "read"
#define NATIVE_WORD_WR "write"
#define NATIVE_WORD_POKE "poke"
#define NATIVE_WORD_FIND "find"
#define NATIVE_WORD_FINDSTR "findstr"
#define NATIVE_WORD_FINDPTR "findptr"
#define NATIVE_WORD_SCAN "scan"
#define NATIVE_WORD_TERMINATE "terminate"
#define NATIVE_KIND_PRIVATE "private"
#define NATIVE_KIND_MAPPED "mapped"
#define NATIVE_KIND_IMG "image"
#define NATIVE_KIND_NONE "none"
#define NATIVE_WORD_XREFS "xrefs"
#define NATIVE_XREF_REL_BYTES 4
#define NATIVE_XREF_NEAR_BYTES 8
#define NATIVE_XREF_REL_SIGN_BIT 0x80000000ULL
#define NATIVE_XREF_REL_MASK 0xFFFFFFFFULL
#define NATIVE_XREF_KIND_REL "rel32"
#define NATIVE_XREF_KIND_ABS "abs64"
#define NATIVE_XREF_PUDDING_SIZE 3
#define NATIVE_SIGN_NEG "-"
#define NATIVE_SIGN_NONE ""

typedef struct native_ctx
{
    ldg_sys_mod_t mods[NATIVE_MOD_CAP];
    ldg_sys_mod_t mod;
    ldg_mem_region_t region;
    crosstalk_re_repl_expr_t expr;
    uint64_t cursor;
    uint64_t end;
    uint64_t examined;
    uint64_t skipped;
    uint64_t xref_tgt;
    uint32_t pattern_len;
    uint32_t carry;
    uint32_t hits;
    uint8_t xrefs;
    uint8_t pudding[NATIVE_XREF_PUDDING_SIZE];
    uint8_t bytes[NATIVE_PEEK_CAP];
    uint8_t pattern[NATIVE_PATTERN_CAP];
    uint8_t mask[NATIVE_PATTERN_CAP];
    uint8_t tail[NATIVE_PATTERN_CAP];
    uint8_t chunk[NATIVE_CHUNK + NATIVE_PATTERN_CAP];
    uint8_t wr[NATIVE_WR_CAP];
    uint8_t verify[NATIVE_WR_CAP];
    char hex[NATIVE_HEX_ROW * NATIVE_HEX_CELL + LDG_STR_TERM_SIZE];
    char ascii[NATIVE_HEX_ROW + LDG_STR_TERM_SIZE];
    char path[NATIVE_PATH_CAP];
    uint8_t tail_pudding[6];
} native_ctx_t;

uint32_t crosstalk_re_repl_native_mod_cunt_get(crosstalk_re_repl_ctx_t *ctx, uint64_t *out)
{
    native_ctx_t *native = 0x0;
    uint64_t cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (ctx->native == 0x0 || ctx->attached != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    native = (native_ctx_t *)ctx->native;

    ret = ldg_sys_mod_list(&ctx->proc, native->mods, (uint64_t)NATIVE_MOD_CAP, &cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (cunt > (uint64_t)NATIVE_MOD_CAP) { cunt = (uint64_t)NATIVE_MOD_CAP; }

    *out = cunt;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_repl_native_init(crosstalk_re_repl_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->native = 0x0;

    ret = ldg_mem_alloc((uint64_t)sizeof(native_ctx_t), &ctx->native);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_secure_zero(ctx->native, (uint64_t)sizeof(native_ctx_t));
}

uint32_t crosstalk_re_repl_native_shutdown(crosstalk_re_repl_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->native == 0x0) { return LDG_ERR_AOK; }

    ret = ldg_mem_dealloc(ctx->native);
    if (ret == LDG_ERR_AOK) { ctx->native = 0x0; }

    return ret;
}

uint32_t crosstalk_re_repl_native_rst(crosstalk_re_repl_ctx_t *ctx)
{
    native_ctx_t *native = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0 || ctx->native == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    native = (native_ctx_t *)ctx->native;
    native->cursor = 0;
    native->end = 0;
    native->examined = 0;
    native->skipped = 0;
    native->pattern_len = 0;
    native->carry = 0;
    native->hits = 0;
    native->xref_tgt = 0;
    native->xrefs = LDG_TRUTH_FALSE;
    ctx->searching = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_repl_native_type_get(const char *text, uint32_t *out_bytes, uint8_t *out_fp, uint8_t *out_signed)
{
    const char *width = 0x0;
    uint64_t hit = UINT64_MAX;

    if (LDG_UNLIKELY(text == 0x0 || out_bytes == 0x0 || out_fp == 0x0 || out_signed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_bytes = 0;
    *out_fp = LDG_TRUTH_FALSE;
    *out_signed = LDG_TRUTH_FALSE;

    if (crosstalk_re_repl_cmd_eq_is(text, NATIVE_WORD_PTR) == LDG_TRUTH_TRUE)
    {
        *out_bytes = (uint32_t)NATIVE_PTR_BYTES;
        return LDG_ERR_AOK;
    }

    if (ldg_str_substring_find(text, NATIVE_TYPE_UNSIGNED, (uint64_t)sizeof(NATIVE_TYPE_UNSIGNED), &hit) == LDG_ERR_AOK && hit == (uint64_t)NATIVE_TYPE_PREFIX_AT) { width = text + sizeof(NATIVE_TYPE_UNSIGNED) - LDG_STR_TERM_SIZE; }
    else if (ldg_str_substring_find(text, NATIVE_TYPE_SIGNED, (uint64_t)sizeof(NATIVE_TYPE_SIGNED), &hit) == LDG_ERR_AOK && hit == (uint64_t)NATIVE_TYPE_PREFIX_AT)
    {
        width = text + sizeof(NATIVE_TYPE_SIGNED) - LDG_STR_TERM_SIZE;
        *out_signed = LDG_TRUTH_TRUE;
    }
    else if (ldg_str_substring_find(text, NATIVE_TYPE_REAL, (uint64_t)sizeof(NATIVE_TYPE_REAL), &hit) == LDG_ERR_AOK && hit == (uint64_t)NATIVE_TYPE_PREFIX_AT)
    {
        width = text + sizeof(NATIVE_TYPE_REAL) - LDG_STR_TERM_SIZE;
        *out_fp = LDG_TRUTH_TRUE;
    }
    else { return LDG_ERR_FUNC_ARG_INVALID; }

    if (crosstalk_re_repl_cmd_eq_is(width, NATIVE_WORD_8) == LDG_TRUTH_TRUE && *out_fp != LDG_TRUTH_TRUE) { *out_bytes = (uint32_t)NATIVE_BYTES_BYTE; }
    else if (crosstalk_re_repl_cmd_eq_is(width, NATIVE_WORD_16) == LDG_TRUTH_TRUE && *out_fp != LDG_TRUTH_TRUE) { *out_bytes = (uint32_t)NATIVE_BYTES_WORD; }
    else if (crosstalk_re_repl_cmd_eq_is(width, NATIVE_WORD_32) == LDG_TRUTH_TRUE) { *out_bytes = (uint32_t)NATIVE_BYTES_DWORD; }
    else if (crosstalk_re_repl_cmd_eq_is(width, NATIVE_WORD_64) == LDG_TRUTH_TRUE) { *out_bytes = (uint32_t)NATIVE_BYTES_QWORD; }
    else { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

static uint32_t native_img_get(crosstalk_re_repl_ctx_t *ctx, ldg_sys_mod_t *out)
{
    native_ctx_t *native = (native_ctx_t *)ctx->native;
    uint64_t cunt = 0;
    uint64_t at = 0;
    uint32_t cmp = 0;
    uint32_t ret = 0;

    ret = ldg_sys_proc_img_path_get(&ctx->proc, native->path, (uint64_t)sizeof(native->path));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_sys_mod_list(&ctx->proc, native->mods, (uint64_t)NATIVE_MOD_CAP, &cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (cunt > (uint64_t)NATIVE_MOD_CAP) { cunt = (uint64_t)NATIVE_MOD_CAP; }

    for (at = 0; at < cunt; at = at + (uint64_t)NATIVE_STEP)
    {
        ret = ldg_str_cmp(native->path, native->mods[at].path, (uint64_t)sizeof(native->path), &cmp);
        if (ret != LDG_ERR_AOK) { return ret; }

        if (cmp != LDG_ARITH_ORD_EQ) { continue; }

        *out = native->mods[at];

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t native_modules(crosstalk_re_repl_ctx_t *ctx, uint32_t start)
{
    native_ctx_t *native = (native_ctx_t *)ctx->native;
    uint64_t cunt = 0;
    uint64_t at = 0;
    uint32_t rows = 0;
    uint32_t ret = 0;

    ret = ldg_sys_mod_list(&ctx->proc, native->mods, (uint64_t)NATIVE_MOD_CAP, &cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (cunt > (uint64_t)NATIVE_MOD_CAP) { cunt = (uint64_t)NATIVE_MOD_CAP; }

    for (at = start; at < cunt && rows < (uint32_t)CROSSTALK_RE_REPL_ROW_CAP; at = at + (uint64_t)NATIVE_STEP)
    {
        ret = crosstalk_re_repl_name_get(native->mods[at].name, ctx->name, (uint32_t)sizeof(ctx->name));
        if (ret != LDG_ERR_AOK) { return ret; }

        ret = crosstalk_re_repl_emit(ctx, "module: %.191s; base: 0x%x64; bytes: %u64", ctx->name, native->mods[at].base, native->mods[at].size);
        if (ret != LDG_ERR_AOK) { return ret; }

        rows = rows + (uint32_t)NATIVE_STEP;
    }

    return crosstalk_re_repl_emit(ctx, "rows: %u32; next start: %u64; more: %u32", rows, at, at < cunt ? (uint32_t)LDG_TRUTH_TRUE : (uint32_t)LDG_TRUTH_FALSE);
}

static uint32_t native_sym(crosstalk_re_repl_ctx_t *ctx, const char *mod_name, const char *sym_name)
{
    native_ctx_t *native = (native_ctx_t *)ctx->native;
    uint64_t addr = 0;
    uint32_t ret = 0;

    ret = ldg_sys_mod_find(&ctx->proc, mod_name, &native->mod);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_sys_mod_sym_get(&ctx->proc, &native->mod, sym_name, &addr);
    if (ret != LDG_ERR_AOK) { return ret; }

    return crosstalk_re_repl_emit(ctx, "symbol: %.63s!%.127s; address: 0x%x64; module base: 0x%x64", mod_name, sym_name, addr, native->mod.base);
}

static uint8_t native_region_readable_is(const ldg_mem_region_t *region)
{
    return region->committed == LDG_TRUTH_TRUE && region->readable == LDG_TRUTH_TRUE && region->guard != LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_re_repl_native_rd(crosstalk_re_repl_ctx_t *ctx, uint64_t addr, void *out, uint64_t len)
{
    native_ctx_t *native = 0x0;
    uint64_t cursor = addr;
    uint64_t next = 0;
    uint64_t end = 0;
    uint64_t got = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || ctx->native == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    native = (native_ctx_t *)ctx->native;

    if (len == 0 || len > UINT64_MAX - addr) { return LDG_ERR_RANGE; }

    end = addr + len;

    while (cursor < end)
    {
        if (ldg_mem_region_get(&ctx->proc, cursor, &native->region) != LDG_ERR_AOK) { return LDG_ERR_IO_RD; }

        if (native_region_readable_is(&native->region) != LDG_TRUTH_TRUE) { return LDG_ERR_DENIED; }

        if (native->region.size > UINT64_MAX - native->region.base) { return LDG_ERR_OVERFLOW; }

        next = native->region.base + native->region.size;
        if (next <= cursor) { return LDG_ERR_INVALID; }

        cursor = next;
    }

    if (ldg_mem_vmem_rd(&ctx->proc, addr, out, len, &got) != LDG_ERR_AOK || got != len) { return LDG_ERR_IO_RD; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_repl_native_addr_get(crosstalk_re_repl_ctx_t *ctx, const char *text, uint64_t *out)
{
    native_ctx_t *native = 0x0;
    uint64_t off = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || ctx->native == 0x0 || text == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    native = (native_ctx_t *)ctx->native;

    if (crosstalk_re_repl_num_get(text, out) == LDG_ERR_AOK) { return LDG_ERR_AOK; }

    *out = 0;

    while (text[at] != LDG_STR_TERM && text[at] != NATIVE_CH_PLUS)
    {
        if (at + LDG_STR_TERM_SIZE >= (uint32_t)sizeof(native->path)) { return LDG_ERR_STR_TRUNC; }

        native->path[at] = text[at];
        at = at + (uint32_t)NATIVE_STEP;
    }

    native->path[at] = LDG_STR_TERM;
    if (at == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (text[at] == NATIVE_CH_PLUS)
    {
        ret = crosstalk_re_repl_num_get(text + at + NATIVE_STEP, &off);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    if (crosstalk_re_repl_cmd_eq_is(native->path, NATIVE_WORD_EXE) == LDG_TRUTH_TRUE) { ret = native_img_get(ctx, &native->mod); }
    else { ret = ldg_sys_mod_find(&ctx->proc, native->path, &native->mod); }

    if (ret != LDG_ERR_AOK) { return ret; }

    if (off > UINT64_MAX - native->mod.base) { return LDG_ERR_OVERFLOW; }

    *out = native->mod.base + off;

    return LDG_ERR_AOK;
}

static const char* native_region_kind_get(uint8_t kind)
{
    if (kind == (uint8_t)LDG_MEM_REGION_KIND_PRIVATE) { return NATIVE_KIND_PRIVATE; }

    if (kind == (uint8_t)LDG_MEM_REGION_KIND_MAPPED) { return NATIVE_KIND_MAPPED; }

    if (kind == (uint8_t)LDG_MEM_REGION_KIND_IMG) { return NATIVE_KIND_IMG; }

    return NATIVE_KIND_NONE;
}

static uint32_t native_regions(crosstalk_re_repl_ctx_t *ctx, uint64_t addr, uint8_t single)
{
    native_ctx_t *native = (native_ctx_t *)ctx->native;
    uint64_t next = addr;
    uint64_t end = 0;
    uint32_t rows = 0;
    uint32_t ret = 0;
    uint8_t done = LDG_TRUTH_FALSE;

    while (rows < (uint32_t)CROSSTALK_RE_REPL_ROW_CAP)
    {
        ret = ldg_mem_region_get(&ctx->proc, next, &native->region);
        if (ret == LDG_ERR_EOF)
        {
            done = LDG_TRUTH_TRUE;
            break;
        }

        if (ret != LDG_ERR_AOK) { return LDG_ERR_IO_RD; }

        if (native->region.size > UINT64_MAX - native->region.base) { return LDG_ERR_OVERFLOW; }

        end = native->region.base + native->region.size;
        if (end <= next) { return LDG_ERR_INVALID; }

        ret = crosstalk_re_repl_emit(ctx, "region: 0x%x64; bytes: %u64; allocation: 0x%x64; committed: %u32; readable: %u32; writable: %u32; executable: %u32; guard: %u32; kind: %.15s", native->region.base, native->region.size, native->region.alloc_base, (uint32_t)native->region.committed, (uint32_t)native->region.readable, (uint32_t)native->region.writable, (uint32_t)native->region.executable, (uint32_t)native->region.guard, native_region_kind_get(native->region.kind));
        if (ret != LDG_ERR_AOK) { return ret; }

        next = end;
        rows = rows + (uint32_t)NATIVE_STEP;

        if (single == LDG_TRUTH_TRUE) { break; }
    }

    return crosstalk_re_repl_emit(ctx, "rows: %u32; next address: 0x%x64; end: %u32", rows, next, (uint32_t)done);
}

static uint32_t native_val_rd(crosstalk_re_repl_ctx_t *ctx, const char *kind, uint64_t addr)
{
    crosstalk_re_repl_expr_val_t val = LDG_STRUCT_ZERO_INIT;
    uint64_t bits = 0;
    uint32_t bytes = 0;
    uint32_t ret = 0;
    uint8_t fp = LDG_TRUTH_FALSE;
    uint8_t signed_val = LDG_TRUTH_FALSE;

    ret = crosstalk_re_repl_native_type_get(kind, &bytes, &fp, &signed_val);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_re_repl_native_rd(ctx, addr, &bits, (uint64_t)bytes);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_re_repl_expr_decode(bits, bytes, fp, signed_val, &val);
    if (ret != LDG_ERR_AOK) { return crosstalk_re_repl_emit(ctx, "address: 0x%x64; type: %.7s; bits: 0x%x64; decode err: %u32", addr, kind, bits, ret); }

    if (fp == LDG_TRUTH_TRUE) { return crosstalk_re_repl_emit(ctx, "address: 0x%x64; type: %.7s; value: %f; bits: 0x%x64", addr, kind, val.real, bits); }

    return crosstalk_re_repl_emit(ctx, "address: 0x%x64; type: %.7s; value: %.1s%u64; hex: 0x%x64", addr, kind, val.neg == LDG_TRUTH_TRUE ? NATIVE_SIGN_NEG : NATIVE_SIGN_NONE, val.mag, bits);
}

static uint32_t native_peek(crosstalk_re_repl_ctx_t *ctx, uint64_t addr, uint64_t len)
{
    native_ctx_t *native = (native_ctx_t *)ctx->native;
    uint32_t at = 0;
    uint32_t row = 0;
    uint32_t col = 0;
    uint32_t pos = 0;
    uint32_t ret = 0;

    if (len == 0 || len > (uint64_t)sizeof(native->bytes) || addr > UINT64_MAX - len) { return LDG_ERR_RANGE; }

    ret = crosstalk_re_repl_native_rd(ctx, addr, native->bytes, len);
    if (ret != LDG_ERR_AOK) { return ret; }

    while (at < (uint32_t)len)
    {
        row = at;

        for (col = 0; col < (uint32_t)NATIVE_HEX_ROW && row + col < (uint32_t)len; col = col + (uint32_t)NATIVE_STEP)
        {
            pos = col * (uint32_t)NATIVE_HEX_CELL;

            ret = ldg_byte_to_hex(native->bytes[row + col], native->hex + pos);
            if (ret != LDG_ERR_AOK) { return ret; }

            native->hex[pos + (uint32_t)NATIVE_HEX_PAIR] = LDG_ASCII_SPACE;
            native->ascii[col] = native->bytes[row + col] >= (uint8_t)LDG_ASCII_PRINTABLE_MIN && native->bytes[row + col] <= (uint8_t)LDG_ASCII_PRINTABLE_MAX ? (char)native->bytes[row + col] : LDG_ASCII_NONPRINTABLE_CHAR;
        }

        native->ascii[col] = LDG_STR_TERM;
        at = row + col;

        for (pos = col * (uint32_t)NATIVE_HEX_CELL; pos < (uint32_t)(NATIVE_HEX_ROW * NATIVE_HEX_CELL); pos = pos + (uint32_t)NATIVE_STEP) { native->hex[pos] = LDG_ASCII_SPACE; }

        native->hex[NATIVE_HEX_ROW * NATIVE_HEX_CELL] = LDG_STR_TERM;

        ret = crosstalk_re_repl_emit(ctx, "0x%x64: %.48s| %.16s", addr + row, native->hex, native->ascii);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t native_pattern_hex_set(native_ctx_t *native, const char *text)
{
    char pair[NATIVE_HEX_PAIR + LDG_STR_TERM_SIZE] = LDG_ARR_ZERO_INIT;
    uint64_t val = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    for (at = 0; text[at] != LDG_STR_TERM; at = at + (uint32_t)NATIVE_HEX_PAIR)
    {
        if (text[at + (uint32_t)NATIVE_STEP] == LDG_STR_TERM) { return LDG_ERR_STR_INVALID; }

        if (native->pattern_len == (uint32_t)NATIVE_PATTERN_CAP) { return LDG_ERR_FULL; }

        if (text[at] == NATIVE_CH_WILD && text[at + (uint32_t)NATIVE_STEP] == NATIVE_CH_WILD)
        {
            native->pattern[native->pattern_len] = 0;
            native->mask[native->pattern_len] = 0;
            native->pattern_len = native->pattern_len + (uint32_t)NATIVE_STEP;
            continue;
        }

        pair[NATIVE_ARG_VERB] = text[at];
        pair[NATIVE_STEP] = text[at + (uint32_t)NATIVE_STEP];

        if (ldg_char_hex_is((uint8_t)pair[NATIVE_ARG_VERB]) != LDG_TRUTH_TRUE || ldg_char_hex_is((uint8_t)pair[NATIVE_STEP]) != LDG_TRUTH_TRUE) { return LDG_ERR_STR_INVALID; }

        ret = ldg_str_uint_parse(pair, LDG_BASE_HEX, (uint64_t)NATIVE_HEX_PAIR, &val);
        if (ret != LDG_ERR_AOK) { return ret; }

        native->pattern[native->pattern_len] = (uint8_t)val;
        native->mask[native->pattern_len] = (uint8_t)LDG_BYTE_MASK;
        native->pattern_len = native->pattern_len + (uint32_t)NATIVE_STEP;
    }

    return LDG_ERR_AOK;
}

static uint32_t native_pattern_text_set(native_ctx_t *native, const char *text)
{
    uint32_t at = 0;

    for (at = 0; text[at] != LDG_STR_TERM; at = at + (uint32_t)NATIVE_STEP)
    {
        if (at == (uint32_t)NATIVE_PATTERN_CAP) { return LDG_ERR_FULL; }

        native->pattern[at] = (uint8_t)text[at];
        native->mask[at] = (uint8_t)LDG_BYTE_MASK;
    }

    native->pattern_len = at;

    return LDG_ERR_AOK;
}

static uint32_t native_pattern_ptr_set(crosstalk_re_repl_ctx_t *ctx, native_ctx_t *native, const char *text)
{
    uint64_t val = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    ret = crosstalk_re_repl_native_addr_get(ctx, text, &val);
    if (ret != LDG_ERR_AOK) { return ret; }

    native->pattern_len = (uint32_t)NATIVE_PTR_BYTES;

    for (at = 0; at < native->pattern_len; at = at + (uint32_t)NATIVE_STEP)
    {
        native->pattern[at] = (uint8_t)(val >> (at * (uint32_t)LDG_BYTE_BITS));
        native->mask[at] = (uint8_t)LDG_BYTE_MASK;
    }

    return LDG_ERR_AOK;
}

static uint32_t native_search_begin(crosstalk_re_repl_ctx_t *ctx, uint32_t argc, char **args)
{
    native_ctx_t *native = (native_ctx_t *)ctx->native;
    uint64_t addr = 0;
    uint64_t len = 0;
    uint32_t ret = 0;

    if (argc != (uint32_t)NATIVE_ARGC_Z) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = crosstalk_re_repl_native_addr_get(ctx, args[NATIVE_ARG_X], &addr);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_re_repl_num_get(args[NATIVE_ARG_Y], &len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (len == 0 || len > (uint64_t)NATIVE_SEARCH_MAX || len > UINT64_MAX - addr) { return LDG_ERR_RANGE; }

    native->pattern_len = 0;

    if (crosstalk_re_repl_cmd_eq_is(args[NATIVE_ARG_VERB], NATIVE_WORD_FINDPTR) == LDG_TRUTH_TRUE) { ret = native_pattern_ptr_set(ctx, native, args[NATIVE_ARG_Z]); }
    else if (crosstalk_re_repl_cmd_eq_is(args[NATIVE_ARG_VERB], NATIVE_WORD_FINDSTR) == LDG_TRUTH_TRUE) { ret = native_pattern_text_set(native, args[NATIVE_ARG_Z]); }
    else { ret = native_pattern_hex_set(native, args[NATIVE_ARG_Z]); }

    if (ret != LDG_ERR_AOK) { return ret; }

    if (native->pattern_len == 0 || (uint64_t)native->pattern_len > len) { return LDG_ERR_RANGE; }

    native->cursor = addr;
    native->end = addr + len;
    native->examined = 0;
    native->skipped = 0;
    native->carry = 0;
    native->hits = 0;
    native->xrefs = LDG_TRUTH_FALSE;
    ctx->searching = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t native_xrefs_begin(crosstalk_re_repl_ctx_t *ctx, uint32_t argc, char **args)
{
    native_ctx_t *native = (native_ctx_t *)ctx->native;
    uint64_t tgt = 0;
    uint64_t addr = 0;
    uint64_t len = 0;
    uint32_t ret = 0;

    if (argc != (uint32_t)NATIVE_ARGC_X && argc != (uint32_t)NATIVE_ARGC_Z) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = crosstalk_re_repl_native_addr_get(ctx, args[NATIVE_ARG_X], &tgt);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (argc == (uint32_t)NATIVE_ARGC_Z)
    {
        ret = crosstalk_re_repl_native_addr_get(ctx, args[NATIVE_ARG_Y], &addr);
        if (ret != LDG_ERR_AOK) { return ret; }

        ret = crosstalk_re_repl_num_get(args[NATIVE_ARG_Z], &len);
        if (ret != LDG_ERR_AOK) { return ret; }
    }
    else
    {
        ret = native_img_get(ctx, &native->mod);
        if (ret != LDG_ERR_AOK) { return ret; }

        addr = native->mod.base;
        len = native->mod.size;
    }

    if (len < (uint64_t)NATIVE_PTR_BYTES || len > (uint64_t)NATIVE_SEARCH_MAX || len > UINT64_MAX - addr) { return LDG_ERR_RANGE; }

    ret = crosstalk_re_repl_emit(ctx, "xref candidates; target: 0x%x64; start: 0x%x64; bytes: %u64; executable pages only; byte scan, instruction boundaries unverified", tgt, addr, len);
    if (ret != LDG_ERR_AOK) { return ret; }

    native->pattern_len = (uint32_t)NATIVE_PTR_BYTES;
    native->cursor = addr;
    native->end = addr + len;
    native->examined = 0;
    native->skipped = 0;
    native->carry = 0;
    native->hits = 0;
    native->xref_tgt = tgt;
    native->xrefs = LDG_TRUTH_TRUE;
    ctx->searching = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint64_t native_le_get(const uint8_t *bytes, uint32_t cunt)
{
    uint64_t val = 0;
    uint32_t at = 0;

    for (at = 0; at < cunt; at = at + (uint32_t)NATIVE_STEP) { val = val | ((uint64_t)bytes[at] << (at * (uint32_t)LDG_BYTE_BITS)); }

    return val;
}

static uint32_t native_xref_emit(crosstalk_re_repl_ctx_t *ctx, uint64_t base, uint32_t at, uint32_t width, const char *kind)
{
    native_ctx_t *native = (native_ctx_t *)ctx->native;
    uint32_t first = 0;
    uint32_t byte = 0;
    uint32_t pos = 0;
    uint32_t ret = 0;

    if (at > (uint32_t)NATIVE_XREF_NEAR_BYTES) { first = at - (uint32_t)NATIVE_XREF_NEAR_BYTES; }

    for (byte = first; byte < at + width; byte = byte + (uint32_t)NATIVE_STEP)
    {
        ret = ldg_byte_to_hex(native->chunk[byte], native->hex + pos);
        if (ret != LDG_ERR_AOK) { return ret; }

        native->hex[pos + (uint32_t)NATIVE_HEX_PAIR] = LDG_ASCII_SPACE;
        pos = pos + (uint32_t)NATIVE_HEX_CELL;
    }

    native->hex[pos] = LDG_STR_TERM;

    return crosstalk_re_repl_emit(ctx, "xref candidate: 0x%x64; kind: %.7s; target: 0x%x64; bytes from: 0x%x64; operand offset: %u32; bytes: %.48s", base + at, kind, native->xref_tgt, base + first, at - first, native->hex);
}

static uint8_t native_xref_rel_is(const native_ctx_t *native, uint64_t base, uint32_t at)
{
    uint64_t disp = native_le_get(native->chunk + at, (uint32_t)NATIVE_XREF_REL_BYTES);
    uint64_t next = base + at + (uint64_t)NATIVE_XREF_REL_BYTES;
    uint64_t tgt = 0;

    if (at + (uint32_t)NATIVE_XREF_REL_BYTES <= native->carry) { return LDG_TRUTH_FALSE; }

    if ((disp & NATIVE_XREF_REL_SIGN_BIT) != 0) { tgt = next - ((~disp & NATIVE_XREF_REL_MASK) + (uint64_t)NATIVE_STEP); }
    else { tgt = next + disp; }

    return tgt == native->xref_tgt ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t native_xref_scan(crosstalk_re_repl_ctx_t *ctx, uint64_t base, uint32_t total)
{
    native_ctx_t *native = (native_ctx_t *)ctx->native;
    const char *kind = 0x0;
    uint32_t width = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    for (at = 0; at + (uint32_t)NATIVE_XREF_REL_BYTES <= total; at = at + (uint32_t)NATIVE_STEP)
    {
        kind = 0x0;

        if (native_xref_rel_is(native, base, at) == LDG_TRUTH_TRUE)
        {
            kind = NATIVE_XREF_KIND_REL;
            width = (uint32_t)NATIVE_XREF_REL_BYTES;
        }
        else if (at + (uint32_t)NATIVE_PTR_BYTES <= total && native_le_get(native->chunk + at, (uint32_t)NATIVE_PTR_BYTES) == native->xref_tgt)
        {
            kind = NATIVE_XREF_KIND_ABS;
            width = (uint32_t)NATIVE_PTR_BYTES;
        }

        if (kind == 0x0) { continue; }

        ret = native_xref_emit(ctx, base, at, width, kind);
        if (ret != LDG_ERR_AOK) { return ret; }

        native->hits = native->hits + (uint32_t)NATIVE_STEP;
        if (native->hits == (uint32_t)CROSSTALK_RE_REPL_ROW_CAP)
        {
            native->cursor = base + at + (uint64_t)NATIVE_STEP;
            native->carry = 0;
            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_AOK;
}

static void native_chunk_skip(native_ctx_t *native, uint64_t end)
{
    native->skipped = native->skipped + (end - native->cursor);
    native->cursor = end;
    native->carry = 0;
}

static uint8_t native_pattern_match_is(const native_ctx_t *native, uint32_t at)
{
    uint32_t pat = 0;

    for (pat = 0; pat < native->pattern_len; pat = pat + (uint32_t)NATIVE_STEP) { if ((native->chunk[at + pat] & native->mask[pat]) != native->pattern[pat]) { return LDG_TRUTH_FALSE; } }

    return LDG_TRUTH_TRUE;
}

static uint32_t native_search_chunk(crosstalk_re_repl_ctx_t *ctx)
{
    native_ctx_t *native = (native_ctx_t *)ctx->native;
    uint64_t end = 0;
    uint64_t len = 0;
    uint64_t got = 0;
    uint64_t base = 0;
    uint32_t total = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (ldg_mem_region_get(&ctx->proc, native->cursor, &native->region) != LDG_ERR_AOK) { return LDG_ERR_IO_RD; }

    if (native->region.size > UINT64_MAX - native->region.base) { return LDG_ERR_OVERFLOW; }

    end = native->region.base + native->region.size;
    if (end <= native->cursor) { return LDG_ERR_INVALID; }

    if (end > native->end) { end = native->end; }

    if (native_region_readable_is(&native->region) != LDG_TRUTH_TRUE || (native->xrefs == LDG_TRUTH_TRUE && native->region.executable != LDG_TRUTH_TRUE))
    {
        native_chunk_skip(native, end);
        return LDG_ERR_AOK;
    }

    len = end - native->cursor;
    if (len > (uint64_t)NATIVE_CHUNK) { len = (uint64_t)NATIVE_CHUNK; }

    end = native->cursor + len;

    if (native->carry != 0)
    {
        ret = ldg_mem_secure_cpy(native->chunk, native->tail, (uint64_t)native->carry);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    if (ldg_mem_vmem_rd(&ctx->proc, native->cursor, native->chunk + native->carry, len, &got) != LDG_ERR_AOK || got != len)
    {
        native_chunk_skip(native, end);
        return LDG_ERR_AOK;
    }

    base = native->cursor - native->carry;
    total = (uint32_t)len + native->carry;
    native->examined = native->examined + len;

    if (native->xrefs == LDG_TRUTH_TRUE)
    {
        ret = native_xref_scan(ctx, base, total);
        if (ret != LDG_ERR_AOK || native->hits == (uint32_t)CROSSTALK_RE_REPL_ROW_CAP) { return ret; }
    }

    for (at = 0; native->xrefs != LDG_TRUTH_TRUE && at + native->pattern_len <= total; at = at + (uint32_t)NATIVE_STEP)
    {
        if (native_pattern_match_is(native, at) != LDG_TRUTH_TRUE) { continue; }

        ret = crosstalk_re_repl_emit(ctx, "match: 0x%x64", base + at);
        if (ret != LDG_ERR_AOK) { return ret; }

        native->hits = native->hits + (uint32_t)NATIVE_STEP;
        if (native->hits == (uint32_t)CROSSTALK_RE_REPL_ROW_CAP)
        {
            native->cursor = base + at + (uint64_t)NATIVE_STEP;
            native->carry = 0;
            return LDG_ERR_AOK;
        }
    }

    native->carry = native->pattern_len - (uint32_t)NATIVE_STEP;
    if (native->carry > total) { native->carry = total; }

    if (native->carry != 0)
    {
        ret = ldg_mem_secure_cpy(native->tail, native->chunk + total - native->carry, (uint64_t)native->carry);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    native->cursor = end;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_repl_native_search_tick(crosstalk_re_repl_ctx_t *ctx)
{
    native_ctx_t *native = 0x0;
    uint32_t step = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || ctx->native == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    native = (native_ctx_t *)ctx->native;

    for (step = 0; step < (uint32_t)NATIVE_CHUNK_STEPS && native->cursor < native->end && native->hits < (uint32_t)CROSSTALK_RE_REPL_ROW_CAP; step = step + (uint32_t)NATIVE_STEP)
    {
        ret = native_search_chunk(ctx);
        if (ret != LDG_ERR_AOK)
        {
            ctx->searching = LDG_TRUTH_FALSE;
            return ret;
        }
    }

    if (native->cursor < native->end && native->hits < (uint32_t)CROSSTALK_RE_REPL_ROW_CAP) { return LDG_ERR_AOK; }

    ctx->searching = LDG_TRUTH_FALSE;

    return crosstalk_re_repl_emit(ctx, "matches: %u32; examined: %u64; skipped: %u64; next address: 0x%x64; remaining bytes: %u64; complete: %u32", native->hits, native->examined, native->skipped, native->cursor, native->end - native->cursor, native->cursor == native->end ? (uint32_t)LDG_TRUTH_TRUE : (uint32_t)LDG_TRUTH_FALSE);
}

static uint32_t native_fp32_encode(double real, uint64_t *out)
{
    double mag = fabs(real);
    double mant = 0;
    uint64_t bits = real < NATIVE_REAL_NONE ? NATIVE_FP32_SIGN_BIT : NATIVE_FP32_SIGN_CLEAR;
    uint64_t rounded = 0;
    int32_t exponent = 0;

    *out = bits;

    if (mag == NATIVE_REAL_NONE) { return LDG_ERR_AOK; }

    mant = frexp(mag, &exponent);
    exponent--;

    if (exponent < NATIVE_FP32_EXPONENT_MIN)
    {
        *out = bits | (uint64_t)rint(ldexp(mag, NATIVE_FP32_SUBNORMAL_SHIFT));
        return LDG_ERR_AOK;
    }

    rounded = (uint64_t)rint(ldexp(mant, NATIVE_FP32_MANT_BITS));
    if (rounded == NATIVE_FP32_MANT_END)
    {
        rounded = NATIVE_FP32_IMPLICIT_BIT;
        exponent++;
    }

    if (exponent > NATIVE_FP32_EXPONENT_MAX) { return LDG_ERR_RANGE; }

    *out = bits | ((uint64_t)(uint32_t)(exponent + NATIVE_FP32_BIAS) << NATIVE_FP32_FRACTION_BITS) | (rounded - NATIVE_FP32_IMPLICIT_BIT);

    return LDG_ERR_AOK;
}

static uint32_t native_real_parse(native_ctx_t *native, const char *text, uint32_t bytes, uint64_t *out)
{
    crosstalk_re_repl_expr_env_t env = LDG_STRUCT_ZERO_INIT;
    crosstalk_re_repl_expr_val_t val = LDG_STRUCT_ZERO_INIT;
    double real = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    for (at = 0; text[at] != LDG_STR_TERM; at = at + (uint32_t)NATIVE_STEP) { if (ldg_char_digit_is((uint8_t)text[at]) != LDG_TRUTH_TRUE && text[at] != NATIVE_CH_POINT && text[at] != NATIVE_CH_MINUS && text[at] != NATIVE_CH_PLUS && text[at] != NATIVE_CH_EXP_LOWER && text[at] != NATIVE_CH_EXP_UPPER) { return LDG_ERR_FUNC_ARG_INVALID; } }

    ret = crosstalk_re_repl_expr_compile(&native->expr, text);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_re_repl_expr_eval(&native->expr, &env, &val);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_re_repl_expr_real_get(&val, &real);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (bytes == (uint32_t)NATIVE_BYTES_QWORD) { return ldg_mem_secure_cpy(out, &real, (uint64_t)sizeof(real)); }

    return native_fp32_encode(real, out);
}

static uint32_t native_val_parse(crosstalk_re_repl_ctx_t *ctx, const char *kind, const char *text, uint32_t *out_bytes, uint64_t *out)
{
    native_ctx_t *native = (native_ctx_t *)ctx->native;
    uint64_t mag = 0;
    uint64_t limit = 0;
    uint32_t skip = 0;
    uint32_t ret = 0;
    uint8_t fp = LDG_TRUTH_FALSE;
    uint8_t signed_val = LDG_TRUTH_FALSE;
    uint8_t neg = text[NATIVE_ARG_VERB] == NATIVE_CH_MINUS ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    *out = 0;

    ret = crosstalk_re_repl_native_type_get(kind, out_bytes, &fp, &signed_val);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (crosstalk_re_repl_cmd_eq_is(kind, NATIVE_WORD_PTR) == LDG_TRUTH_TRUE) { return crosstalk_re_repl_native_addr_get(ctx, text, out); }

    if (fp == LDG_TRUTH_TRUE) { return native_real_parse(native, text, *out_bytes, out); }

    if (neg == LDG_TRUTH_TRUE && signed_val != LDG_TRUTH_TRUE) { return LDG_ERR_RANGE; }

    if (neg == LDG_TRUTH_TRUE || text[NATIVE_ARG_VERB] == NATIVE_CH_PLUS) { skip = (uint32_t)NATIVE_STEP; }

    ret = crosstalk_re_repl_num_get(text + skip, &mag);
    if (ret != LDG_ERR_AOK) { return ret; }

    limit = UINT64_MAX >> (((uint32_t)NATIVE_BYTES_QWORD - *out_bytes) * (uint32_t)LDG_BYTE_BITS);

    if (signed_val == LDG_TRUTH_TRUE) { limit = limit >> NATIVE_STEP; }

    if (signed_val == LDG_TRUTH_TRUE && neg == LDG_TRUTH_TRUE) { limit = limit + (uint64_t)NATIVE_STEP; }

    if (mag > limit) { return LDG_ERR_RANGE; }

    *out = neg == LDG_TRUTH_TRUE ? ~mag + (uint64_t)NATIVE_STEP : mag;

    return LDG_ERR_AOK;
}

static uint32_t native_wr_validate(crosstalk_re_repl_ctx_t *ctx, uint64_t addr, uint64_t len)
{
    ldg_mem_region_t region = LDG_STRUCT_ZERO_INIT;
    uint64_t end = 0;
    uint64_t next = 0;

    if (addr == 0 || len == 0 || len > UINT64_MAX - addr) { return LDG_ERR_RANGE; }

    end = addr + len;

    while (addr < end)
    {
        if (ldg_mem_region_get(&ctx->proc, addr, &region) != LDG_ERR_AOK) { return LDG_ERR_IO_RD; }

        if (region.committed != LDG_TRUTH_TRUE || region.guard == LDG_TRUTH_TRUE || region.writable != LDG_TRUTH_TRUE) { return LDG_ERR_DENIED; }

        if (region.size > UINT64_MAX - region.base) { return LDG_ERR_OVERFLOW; }

        next = region.base + region.size;
        if (next <= addr) { return LDG_ERR_INVALID; }

        addr = next;
    }

    return LDG_ERR_AOK;
}

static uint32_t native_wr_bytes_parse(native_ctx_t *native, const char *text, uint32_t *out_len)
{
    char pair[NATIVE_HEX_PAIR + LDG_STR_TERM_SIZE] = LDG_ARR_ZERO_INIT;
    uint32_t at = 0;
    uint32_t len = 0;
    uint32_t ret = 0;

    *out_len = 0;

    while (text[at] != LDG_STR_TERM)
    {
        if (len == (uint32_t)NATIVE_WR_CAP || text[at + (uint32_t)NATIVE_STEP] == LDG_STR_TERM) { return LDG_ERR_RANGE; }

        pair[NATIVE_ARG_VERB] = text[at];
        pair[NATIVE_STEP] = text[at + (uint32_t)NATIVE_STEP];

        ret = ldg_hex_to_bytes(pair, native->wr + len, (uint64_t)NATIVE_STEP);
        if (ret != LDG_ERR_AOK) { return ret; }

        len = len + (uint32_t)NATIVE_STEP;
        at = at + (uint32_t)NATIVE_HEX_PAIR;
    }

    *out_len = len;

    return LDG_ERR_AOK;
}

static uint32_t native_wr(crosstalk_re_repl_ctx_t *ctx, uint64_t addr, uint32_t len)
{
    native_ctx_t *native = (native_ctx_t *)ctx->native;
    uint64_t put = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    ret = native_wr_validate(ctx, addr, (uint64_t)len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (ldg_mem_vmem_wr(&ctx->proc, addr, native->wr, (uint64_t)len, &put) != LDG_ERR_AOK || put != (uint64_t)len) { ret = LDG_ERR_IO_WR; }

    if (put != 0 && ldg_mem_vmem_flush(&ctx->proc, addr, put) != LDG_ERR_AOK) { ret = LDG_ERR_IO_FLUSH; }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_re_repl_native_rd(ctx, addr, native->verify, (uint64_t)len); }

    for (at = 0; ret == LDG_ERR_AOK && at < len; at = at + (uint32_t)NATIVE_STEP) { if (native->wr[at] != native->verify[at]) { ret = LDG_ERR_IO_WR; } }

    if (crosstalk_re_repl_emit(ctx, "write; address: 0x%x64; bytes: %u32; written: %u64; err: %u32", addr, len, put, ret) != LDG_ERR_AOK && ret == LDG_ERR_AOK) { return LDG_ERR_IO_WR; }

    return ret;
}

static uint32_t native_wr_cmd(crosstalk_re_repl_ctx_t *ctx, uint32_t argc, char **args)
{
    native_ctx_t *native = (native_ctx_t *)ctx->native;
    uint64_t addr = 0;
    uint64_t val = 0;
    uint32_t bytes = 0;
    uint32_t ret = 0;

    if (argc != (uint32_t)NATIVE_ARGC_Z) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = crosstalk_re_repl_native_addr_get(ctx, args[NATIVE_ARG_Y], &addr);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = native_val_parse(ctx, args[NATIVE_ARG_X], args[NATIVE_ARG_Z], &bytes, &val);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_mem_secure_cpy(native->wr, &val, (uint64_t)bytes);
    if (ret != LDG_ERR_AOK) { return ret; }

    return native_wr(ctx, addr, bytes);
}

static uint32_t native_poke_cmd(crosstalk_re_repl_ctx_t *ctx, uint32_t argc, char **args)
{
    native_ctx_t *native = (native_ctx_t *)ctx->native;
    uint64_t addr = 0;
    uint32_t len = 0;
    uint32_t ret = 0;

    if (argc != (uint32_t)NATIVE_ARGC_Y) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = crosstalk_re_repl_native_addr_get(ctx, args[NATIVE_ARG_X], &addr);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = native_wr_bytes_parse(native, args[NATIVE_ARG_Y], &len);
    if (ret != LDG_ERR_AOK) { return ret; }

    return native_wr(ctx, addr, len);
}

static uint32_t native_list_cmd(crosstalk_re_repl_ctx_t *ctx, uint32_t argc, char **args, uint8_t regions)
{
    uint64_t val = 0;
    uint32_t ret = LDG_ERR_AOK;

    if (argc > (uint32_t)NATIVE_ARGC_X) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (regions == LDG_TRUTH_TRUE)
    {
        if (argc == (uint32_t)NATIVE_ARGC_X) { ret = crosstalk_re_repl_native_addr_get(ctx, args[NATIVE_ARG_X], &val); }

        return ret == LDG_ERR_AOK ? native_regions(ctx, val, LDG_TRUTH_FALSE) : ret;
    }

    if (argc == (uint32_t)NATIVE_ARGC_X) { ret = crosstalk_re_repl_num_get(args[NATIVE_ARG_X], &val); }

    if (ret != LDG_ERR_AOK || val > (uint64_t)UINT32_MAX) { return LDG_ERR_FUNC_ARG_INVALID; }

    return native_modules(ctx, (uint32_t)val);
}

static uint32_t native_addr_cmd(crosstalk_re_repl_ctx_t *ctx, uint32_t argc, char **args)
{
    uint64_t addr = 0;
    uint64_t len = (uint64_t)NATIVE_PEEK_DEFAULT;
    uint32_t ret = 0;
    uint8_t peek = crosstalk_re_repl_cmd_eq_is(args[NATIVE_ARG_VERB], NATIVE_WORD_PEEK);

    if (argc < (uint32_t)NATIVE_ARGC_X || argc > (peek == LDG_TRUTH_TRUE ? (uint32_t)NATIVE_ARGC_Y : (uint32_t)NATIVE_ARGC_X)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = crosstalk_re_repl_native_addr_get(ctx, args[NATIVE_ARG_X], &addr);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (peek == LDG_TRUTH_TRUE)
    {
        if (argc == (uint32_t)NATIVE_ARGC_Y) { ret = crosstalk_re_repl_num_get(args[NATIVE_ARG_Y], &len); }

        return ret == LDG_ERR_AOK ? native_peek(ctx, addr, len) : ret;
    }

    ret = crosstalk_re_repl_emit(ctx, "address: 0x%x64", addr);
    if (ret != LDG_ERR_AOK) { return ret; }

    return native_regions(ctx, addr, LDG_TRUTH_TRUE);
}

static uint32_t native_rd_cmd(crosstalk_re_repl_ctx_t *ctx, uint32_t argc, char **args)
{
    uint64_t addr = 0;
    uint32_t ret = 0;

    if (argc != (uint32_t)NATIVE_ARGC_Y) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = crosstalk_re_repl_native_addr_get(ctx, args[NATIVE_ARG_Y], &addr);
    if (ret != LDG_ERR_AOK) { return ret; }

    return native_val_rd(ctx, args[NATIVE_ARG_X], addr);
}

uint32_t crosstalk_re_repl_native_cmd(crosstalk_re_repl_ctx_t *ctx, uint32_t argc, char **args)
{
    const char *verb = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0 || ctx->native == 0x0 || args == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(argc == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    verb = args[NATIVE_ARG_VERB];


    if (crosstalk_re_repl_cmd_eq_is(verb, NATIVE_WORD_STATUS) == LDG_TRUTH_TRUE) { return crosstalk_re_repl_emit(ctx, "native ready: %u32; pid: %u32; pointer bytes: %u32; search bytes per tick: %u32", (uint32_t)LDG_TRUTH_TRUE, ctx->proc.pid, (uint32_t)NATIVE_PTR_BYTES, (uint32_t)(NATIVE_CHUNK * NATIVE_CHUNK_STEPS)); }

    if (crosstalk_re_repl_cmd_eq_is(verb, NATIVE_WORD_WR) == LDG_TRUTH_TRUE) { return native_wr_cmd(ctx, argc, args); }

    if (crosstalk_re_repl_cmd_eq_is(verb, NATIVE_WORD_POKE) == LDG_TRUTH_TRUE) { return native_poke_cmd(ctx, argc, args); }

    if (crosstalk_re_repl_cmd_eq_is(verb, NATIVE_WORD_TERMINATE) == LDG_TRUTH_TRUE) { return argc == (uint32_t)NATIVE_ARGC_VERB ? ldg_sys_proc_term(&ctx->proc, (uint32_t)NATIVE_EXIT_CODE) : LDG_ERR_FUNC_ARG_INVALID; }

    if (crosstalk_re_repl_cmd_eq_is(verb, NATIVE_WORD_SCAN) == LDG_TRUTH_TRUE) { return crosstalk_re_repl_scan_cmd(ctx, argc, args); }

    if (crosstalk_re_repl_cmd_eq_is(verb, NATIVE_WORD_MODULES) == LDG_TRUTH_TRUE) { return native_list_cmd(ctx, argc, args, LDG_TRUTH_FALSE); }

    if (crosstalk_re_repl_cmd_eq_is(verb, NATIVE_WORD_REGIONS) == LDG_TRUTH_TRUE) { return native_list_cmd(ctx, argc, args, LDG_TRUTH_TRUE); }

    if (crosstalk_re_repl_cmd_eq_is(verb, NATIVE_WORD_SYM) == LDG_TRUTH_TRUE) { return argc == (uint32_t)NATIVE_ARGC_Y ? native_sym(ctx, args[NATIVE_ARG_X], args[NATIVE_ARG_Y]) : LDG_ERR_FUNC_ARG_INVALID; }

    if (crosstalk_re_repl_cmd_eq_is(verb, NATIVE_WORD_RD) == LDG_TRUTH_TRUE) { return native_rd_cmd(ctx, argc, args); }

    if (crosstalk_re_repl_cmd_eq_is(verb, NATIVE_WORD_FIND) == LDG_TRUTH_TRUE || crosstalk_re_repl_cmd_eq_is(verb, NATIVE_WORD_FINDSTR) == LDG_TRUTH_TRUE || crosstalk_re_repl_cmd_eq_is(verb, NATIVE_WORD_FINDPTR) == LDG_TRUTH_TRUE) { return native_search_begin(ctx, argc, args); }

    if (crosstalk_re_repl_cmd_eq_is(verb, NATIVE_WORD_XREFS) == LDG_TRUTH_TRUE) { return native_xrefs_begin(ctx, argc, args); }

    if (crosstalk_re_repl_cmd_eq_is(verb, NATIVE_WORD_ADDR) == LDG_TRUTH_TRUE || crosstalk_re_repl_cmd_eq_is(verb, NATIVE_WORD_PEEK) == LDG_TRUTH_TRUE) { return native_addr_cmd(ctx, argc, args); }

    return LDG_ERR_NOT_FOUND;
}
