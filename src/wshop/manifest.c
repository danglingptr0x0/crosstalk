#include <stdint.h>
#include <string.h>

#include <dangling/core/arith.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/parse/parse.h>
#include <dangling/str/str.h>

#include "wshop/manifest.h"

#define MANIFEST_SEP ':'
#define MANIFEST_COMMENT '#'
#define MANIFEST_DOT '.'
#define MANIFEST_ASCII_LOWER_A 'a'
#define MANIFEST_ASCII_UPPER_A 'A'
#define MANIFEST_HEX_MARK_LOWER 'x'
#define MANIFEST_HEX_MARK_UPPER 'X'
#define MANIFEST_HEX_PREFIX_LEN 2
#define MANIFEST_IDX_DIG_MAX 3
#define MANIFEST_STEP 1

static uint8_t manifest_key_is(const char *key, uint64_t key_len, const char *want)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(key == 0x0 || want == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (idx < key_len)
    {
        if (want[idx] == LDG_STR_TERM) { return LDG_TRUTH_FALSE; }

        if (key[idx] != want[idx]) { return LDG_TRUTH_FALSE; }

        idx = idx + MANIFEST_STEP;
    }

    if (want[key_len] != LDG_STR_TERM) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static uint32_t manifest_str_set(char *dst, uint64_t cap, const char *val, uint64_t val_len)
{
    if (LDG_UNLIKELY(dst == 0x0 || val == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (val_len >= cap) { return CROSSTALK_WSHOP_MANIFEST_ERR_VAL; }

    memcpy(dst, val, val_len);
    dst[val_len] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

static uint32_t manifest_u64_parse(const char *val, uint64_t val_len, uint64_t *out)
{
    uint64_t idx = 0;
    uint64_t acc = 0;
    uint64_t base = LDG_BASE_DEC;
    uint64_t dig = 0;
    uint32_t ret = 0;
    char ch = 0;

    if (LDG_UNLIKELY(val == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (val_len == 0) { return CROSSTALK_WSHOP_MANIFEST_ERR_VAL; }

    if (val_len > MANIFEST_HEX_PREFIX_LEN && val[0] == LDG_ASCII_DIGIT_ZERO && (val[1] == MANIFEST_HEX_MARK_LOWER || val[1] == MANIFEST_HEX_MARK_UPPER))
    {
        base = LDG_BASE_HEX;
        idx = MANIFEST_HEX_PREFIX_LEN;
    }

    while (idx < val_len)
    {
        ch = val[idx];

        if (ldg_char_digit_is(ch) == LDG_TRUTH_TRUE) { dig = (uint64_t)(ch - LDG_ASCII_DIGIT_ZERO); }
        else if (base == LDG_BASE_HEX && ldg_char_hex_is(ch) == LDG_TRUTH_TRUE) { if (ch >= MANIFEST_ASCII_LOWER_A) { dig = (uint64_t)(ch - MANIFEST_ASCII_LOWER_A) + LDG_BASE_DEC; }
            else { dig = (uint64_t)(ch - MANIFEST_ASCII_UPPER_A) + LDG_BASE_DEC; } }
        else { return CROSSTALK_WSHOP_MANIFEST_ERR_VAL; }

        ret = ldg_arith_uint64_mul(acc, base, &acc);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_arith_uint64_add(acc, dig, &acc);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        idx = idx + MANIFEST_STEP;
    }

    *out = acc;

    return LDG_ERR_AOK;
}

static uint32_t manifest_u32_parse(const char *val, uint64_t val_len, uint32_t *out)
{
    uint64_t wide = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = manifest_u64_parse(val, val_len, &wide);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (wide > UINT32_MAX) { return LDG_ERR_OVERFLOW; }

    *out = (uint32_t)wide;

    return LDG_ERR_AOK;
}

static uint32_t manifest_kind_parse(const char *val, uint64_t val_len, uint32_t *out)
{
    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = CROSSTALK_WSHOP_MANIFEST_KIND_NONE;

    if (manifest_key_is(val, val_len, "core") == LDG_TRUTH_TRUE) { *out = CROSSTALK_WSHOP_MANIFEST_KIND_CORE; return LDG_ERR_AOK; }

    if (manifest_key_is(val, val_len, "software") == LDG_TRUTH_TRUE) { *out = CROSSTALK_WSHOP_MANIFEST_KIND_SOFTWARE; return LDG_ERR_AOK; }

    return CROSSTALK_WSHOP_MANIFEST_ERR_KIND;
}

static uint32_t manifest_role_parse(const char *val, uint64_t val_len, uint32_t *out)
{
    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = CROSSTALK_WSHOP_MANIFEST_ROLE_NONE;

    if (manifest_key_is(val, val_len, "firmware") == LDG_TRUTH_TRUE) { *out = CROSSTALK_WSHOP_MANIFEST_ROLE_FIRMWARE; return LDG_ERR_AOK; }

    if (manifest_key_is(val, val_len, "bios") == LDG_TRUTH_TRUE) { *out = CROSSTALK_WSHOP_MANIFEST_ROLE_BIOS; return LDG_ERR_AOK; }

    if (manifest_key_is(val, val_len, "kernel") == LDG_TRUTH_TRUE) { *out = CROSSTALK_WSHOP_MANIFEST_ROLE_KERNEL; return LDG_ERR_AOK; }

    if (manifest_key_is(val, val_len, "os") == LDG_TRUTH_TRUE) { *out = CROSSTALK_WSHOP_MANIFEST_ROLE_OS; return LDG_ERR_AOK; }

    if (manifest_key_is(val, val_len, "userland") == LDG_TRUTH_TRUE) { *out = CROSSTALK_WSHOP_MANIFEST_ROLE_USERLAND; return LDG_ERR_AOK; }

    return CROSSTALK_WSHOP_MANIFEST_ERR_VAL;
}

static uint32_t manifest_ent_parse(const char *val, uint64_t val_len, uint32_t *out)
{
    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = CROSSTALK_WSHOP_MANIFEST_ENT_FREE;

    if (manifest_key_is(val, val_len, "free") == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (manifest_key_is(val, val_len, "paid") == LDG_TRUTH_TRUE)
    {
        *out = CROSSTALK_WSHOP_MANIFEST_ENT_PAID;
        return LDG_ERR_AOK;
    }

    return CROSSTALK_WSHOP_MANIFEST_ERR_VAL;
}

static uint32_t manifest_idx_key_split(const char *key, uint64_t key_len, const char *prefix, uint64_t *out_idx, const char **out_tail, uint64_t *out_tail_len)
{
    uint64_t pos = 0;
    uint64_t acc = 0;
    uint64_t dig_cunt = 0;

    if (LDG_UNLIKELY(key == 0x0 || prefix == 0x0 || out_idx == 0x0 || out_tail == 0x0 || out_tail_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = 0;
    *out_tail = 0x0;
    *out_tail_len = 0;

    while (prefix[pos] != LDG_STR_TERM)
    {
        if (pos >= key_len) { return LDG_ERR_NOT_FOUND; }

        if (key[pos] != prefix[pos]) { return LDG_ERR_NOT_FOUND; }

        pos = pos + MANIFEST_STEP;
    }

    while (pos < key_len && ldg_char_digit_is(key[pos]) == LDG_TRUTH_TRUE && dig_cunt < MANIFEST_IDX_DIG_MAX)
    {
        acc = acc * LDG_BASE_DEC + (uint64_t)(key[pos] - LDG_ASCII_DIGIT_ZERO);
        dig_cunt = dig_cunt + MANIFEST_STEP;
        pos = pos + MANIFEST_STEP;
    }

    if (dig_cunt == 0) { return LDG_ERR_NOT_FOUND; }

    if (pos >= key_len) { return LDG_ERR_NOT_FOUND; }

    if (key[pos] != MANIFEST_DOT) { return LDG_ERR_NOT_FOUND; }

    pos = pos + MANIFEST_STEP;

    *out_idx = acc;
    *out_tail = &key[pos];
    *out_tail_len = key_len - pos;

    return LDG_ERR_AOK;
}

static uint32_t manifest_mem_field_set(crosstalk_wshop_manifest_t *out, uint64_t idx, const char *tail, uint64_t tail_len, const char *val, uint64_t val_len)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(out == 0x0 || tail == 0x0 || val == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (idx >= CROSSTALK_WSHOP_MANIFEST_MEM_MAX) { return CROSSTALK_WSHOP_MANIFEST_ERR_FULL; }

    if (manifest_key_is(tail, tail_len, "sig") == LDG_TRUTH_TRUE) { ret = manifest_str_set(out->mem[idx].sig, CROSSTALK_WSHOP_MANIFEST_SIG_CAP, val, val_len); }
    else if (manifest_key_is(tail, tail_len, "words") == LDG_TRUTH_TRUE) { ret = manifest_u64_parse(val, val_len, &out->mem[idx].words); }
    else if (manifest_key_is(tail, tail_len, "base") == LDG_TRUTH_TRUE) { ret = manifest_u64_parse(val, val_len, &out->mem[idx].base); }
    else { return LDG_ERR_AOK; }

    if (ret != LDG_ERR_AOK) { return ret; }

    if (idx + MANIFEST_STEP > (uint64_t)out->mem_cunt) { out->mem_cunt = (uint32_t)(idx + MANIFEST_STEP); }

    return LDG_ERR_AOK;
}

static uint32_t manifest_img_field_set(crosstalk_wshop_manifest_t *out, uint64_t idx, const char *tail, uint64_t tail_len, const char *val, uint64_t val_len)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(out == 0x0 || tail == 0x0 || val == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (idx >= CROSSTALK_WSHOP_MANIFEST_IMG_MAX) { return CROSSTALK_WSHOP_MANIFEST_ERR_FULL; }

    if (manifest_key_is(tail, tail_len, "path") == LDG_TRUTH_TRUE) { ret = manifest_str_set(out->img[idx].path, CROSSTALK_WSHOP_MANIFEST_PATH_CAP, val, val_len); }
    else if (manifest_key_is(tail, tail_len, "mem") == LDG_TRUTH_TRUE) { ret = manifest_str_set(out->img[idx].mem, CROSSTALK_WSHOP_MANIFEST_SIG_CAP, val, val_len); }
    else if (manifest_key_is(tail, tail_len, "word_off") == LDG_TRUTH_TRUE) { ret = manifest_u64_parse(val, val_len, &out->img[idx].word_off); }
    else { return LDG_ERR_AOK; }

    if (ret != LDG_ERR_AOK) { return ret; }

    if (idx + MANIFEST_STEP > (uint64_t)out->img_cunt) { out->img_cunt = (uint32_t)(idx + MANIFEST_STEP); }

    return LDG_ERR_AOK;
}

static uint32_t manifest_common_field_set(crosstalk_wshop_manifest_t *out, const char *key, uint64_t key_len, const char *val, uint64_t val_len)
{
    if (LDG_UNLIKELY(out == 0x0 || key == 0x0 || val == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (manifest_key_is(key, key_len, "rev") == LDG_TRUTH_TRUE) { return manifest_u32_parse(val, val_len, &out->rev); }

    if (manifest_key_is(key, key_len, "kind") == LDG_TRUTH_TRUE) { return manifest_kind_parse(val, val_len, &out->kind); }

    if (manifest_key_is(key, key_len, "name") == LDG_TRUTH_TRUE) { return manifest_str_set(out->name, CROSSTALK_WSHOP_MANIFEST_NAME_CAP, val, val_len); }

    if (manifest_key_is(key, key_len, "author") == LDG_TRUTH_TRUE) { return manifest_str_set(out->author, CROSSTALK_WSHOP_MANIFEST_AUTHOR_CAP, val, val_len); }

    if (manifest_key_is(key, key_len, "author_id") == LDG_TRUTH_TRUE) { return manifest_u64_parse(val, val_len, &out->author_id); }

    if (manifest_key_is(key, key_len, "license") == LDG_TRUTH_TRUE) { return manifest_str_set(out->license, CROSSTALK_WSHOP_MANIFEST_LICENSE_CAP, val, val_len); }

    if (manifest_key_is(key, key_len, "entitlement") == LDG_TRUTH_TRUE) { return manifest_ent_parse(val, val_len, &out->entitlement); }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t manifest_core_field_set(crosstalk_wshop_manifest_t *out, const char *key, uint64_t key_len, const char *val, uint64_t val_len)
{
    const char *tail = 0x0;
    uint64_t tail_len = 0;
    uint64_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(out == 0x0 || key == 0x0 || val == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (manifest_key_is(key, key_len, "top") == LDG_TRUTH_TRUE) { return manifest_str_set(out->top, CROSSTALK_EDA_IC_FPGA_TOP_CAP, val, val_len); }

    if (manifest_key_is(key, key_len, "files") == LDG_TRUTH_TRUE) { return manifest_str_set(out->files, CROSSTALK_WSHOP_MANIFEST_FILES_CAP, val, val_len); }

    if (manifest_key_is(key, key_len, "clk") == LDG_TRUTH_TRUE) { return manifest_str_set(out->clk, CROSSTALK_WSHOP_MANIFEST_SIG_CAP, val, val_len); }

    if (manifest_key_is(key, key_len, "part") == LDG_TRUTH_TRUE) { return manifest_str_set(out->part, CROSSTALK_WSHOP_MANIFEST_PART_CAP, val, val_len); }

    if (manifest_key_is(key, key_len, "isa") == LDG_TRUTH_TRUE) { return manifest_str_set(out->isa, CROSSTALK_WSHOP_MANIFEST_ISA_CAP, val, val_len); }

    if (manifest_key_is(key, key_len, "isa_rev") == LDG_TRUTH_TRUE) { return manifest_u32_parse(val, val_len, &out->isa_rev); }

    if (manifest_key_is(key, key_len, "fingerprint") == LDG_TRUTH_TRUE) { return manifest_u64_parse(val, val_len, &out->fingerprint); }

    if (manifest_key_is(key, key_len, "toolchain_rev") == LDG_TRUTH_TRUE) { return manifest_u32_parse(val, val_len, &out->toolchain_rev); }

    if (manifest_key_is(key, key_len, "port.serial") == LDG_TRUTH_TRUE) { return manifest_str_set(out->port_serial, CROSSTALK_WSHOP_MANIFEST_SIG_CAP, val, val_len); }

    if (manifest_key_is(key, key_len, "port.led") == LDG_TRUTH_TRUE) { return manifest_str_set(out->port_led, CROSSTALK_WSHOP_MANIFEST_SIG_CAP, val, val_len); }

    if (manifest_key_is(key, key_len, "port.keys") == LDG_TRUTH_TRUE) { return manifest_str_set(out->port_keys, CROSSTALK_WSHOP_MANIFEST_SIG_CAP, val, val_len); }

    if (manifest_key_is(key, key_len, "port.fb") == LDG_TRUTH_TRUE) { return manifest_str_set(out->port_fb, CROSSTALK_WSHOP_MANIFEST_SIG_CAP, val, val_len); }

    if (manifest_key_is(key, key_len, "port.hlt") == LDG_TRUTH_TRUE) { return manifest_str_set(out->port_hlt, CROSSTALK_WSHOP_MANIFEST_SIG_CAP, val, val_len); }

    ret = manifest_idx_key_split(key, key_len, "mem.", &idx, &tail, &tail_len);
    if (ret == LDG_ERR_AOK) { return manifest_mem_field_set(out, idx, tail, tail_len, val, val_len); }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t manifest_software_field_set(crosstalk_wshop_manifest_t *out, const char *key, uint64_t key_len, const char *val, uint64_t val_len)
{
    const char *tail = 0x0;
    uint64_t tail_len = 0;
    uint64_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(out == 0x0 || key == 0x0 || val == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (manifest_key_is(key, key_len, "role") == LDG_TRUTH_TRUE) { return manifest_role_parse(val, val_len, &out->role); }

    if (manifest_key_is(key, key_len, "tgt.isa") == LDG_TRUTH_TRUE) { return manifest_str_set(out->tgt_isa, CROSSTALK_WSHOP_MANIFEST_ISA_CAP, val, val_len); }

    if (manifest_key_is(key, key_len, "tgt.isa_rev") == LDG_TRUTH_TRUE) { return manifest_u32_parse(val, val_len, &out->tgt_isa_rev); }

    if (manifest_key_is(key, key_len, "tgt.core") == LDG_TRUTH_TRUE) { return manifest_u64_parse(val, val_len, &out->tgt_core); }

    if (manifest_key_is(key, key_len, "tgt.fingerprint_min") == LDG_TRUTH_TRUE) { return manifest_u64_parse(val, val_len, &out->tgt_fingerprint_min); }

    if (manifest_key_is(key, key_len, "tgt.fingerprint_max") == LDG_TRUTH_TRUE) { return manifest_u64_parse(val, val_len, &out->tgt_fingerprint_max); }

    ret = manifest_idx_key_split(key, key_len, "img.", &idx, &tail, &tail_len);
    if (ret == LDG_ERR_AOK) { return manifest_img_field_set(out, idx, tail, tail_len, val, val_len); }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t manifest_field_set(crosstalk_wshop_manifest_t *out, const char *key, uint64_t key_len, const char *val, uint64_t val_len)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(out == 0x0 || key == 0x0 || val == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = manifest_common_field_set(out, key, key_len, val, val_len);
    if (ret != LDG_ERR_NOT_FOUND) { return ret; }

    ret = manifest_core_field_set(out, key, key_len, val, val_len);
    if (ret != LDG_ERR_NOT_FOUND) { return ret; }

    ret = manifest_software_field_set(out, key, key_len, val, val_len);
    if (ret != LDG_ERR_NOT_FOUND) { return ret; }

    return LDG_ERR_AOK;
}

static uint32_t manifest_validate(const crosstalk_wshop_manifest_t *out)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (out->rev == 0) { return CROSSTALK_WSHOP_MANIFEST_ERR_FIELD; }

    if (out->rev > CROSSTALK_WSHOP_MANIFEST_REV) { return CROSSTALK_WSHOP_MANIFEST_ERR_REV; }

    if (out->kind == CROSSTALK_WSHOP_MANIFEST_KIND_CORE)
    {
        if (out->top[0] == LDG_STR_TERM) { return CROSSTALK_WSHOP_MANIFEST_ERR_FIELD; }

        if (out->files[0] == LDG_STR_TERM) { return CROSSTALK_WSHOP_MANIFEST_ERR_FIELD; }

        if (out->clk[0] == LDG_STR_TERM) { return CROSSTALK_WSHOP_MANIFEST_ERR_FIELD; }

        if (out->isa[0] == LDG_STR_TERM) { return CROSSTALK_WSHOP_MANIFEST_ERR_FIELD; }

        for (idx = 0; idx < out->mem_cunt; idx = idx + MANIFEST_STEP) { if (out->mem[idx].sig[0] == LDG_STR_TERM) { return CROSSTALK_WSHOP_MANIFEST_ERR_FIELD; } }

        return LDG_ERR_AOK;
    }

    if (out->kind == CROSSTALK_WSHOP_MANIFEST_KIND_SOFTWARE)
    {
        if (out->tgt_isa[0] == LDG_STR_TERM) { return CROSSTALK_WSHOP_MANIFEST_ERR_FIELD; }

        if (out->img_cunt == 0) { return CROSSTALK_WSHOP_MANIFEST_ERR_FIELD; }

        for (idx = 0; idx < out->img_cunt; idx = idx + MANIFEST_STEP)
        {
            if (out->img[idx].path[0] == LDG_STR_TERM) { return CROSSTALK_WSHOP_MANIFEST_ERR_FIELD; }

            if (out->img[idx].mem[0] == LDG_STR_TERM) { return CROSSTALK_WSHOP_MANIFEST_ERR_FIELD; }
        }

        return LDG_ERR_AOK;
    }

    return CROSSTALK_WSHOP_MANIFEST_ERR_KIND;
}

static uint8_t manifest_isa_family_is(const char *lhs, const char *rhs)
{
    if (LDG_UNLIKELY(lhs == 0x0 || rhs == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ldg_parse_streq_is(lhs, rhs) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    if (ldg_parse_streq_is(lhs, "8086") == LDG_TRUTH_TRUE && ldg_parse_streq_is(rhs, "8088") == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    if (ldg_parse_streq_is(lhs, "8088") == LDG_TRUTH_TRUE && ldg_parse_streq_is(rhs, "8086") == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

uint32_t crosstalk_wshop_manifest_parse(const char *src, uint64_t len, crosstalk_wshop_manifest_t *out)
{
    uint64_t pos = 0;
    uint64_t line_end = 0;
    uint64_t sep = 0;
    uint64_t key_beg = 0;
    uint64_t key_end = 0;
    uint64_t val_beg = 0;
    uint64_t val_end = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(src == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(out, 0x0, sizeof(*out));

    while (pos < len)
    {
        line_end = pos;

        while (line_end < len && src[line_end] != LDG_ASCII_LF) { line_end = line_end + MANIFEST_STEP; }

        key_beg = pos;
        val_end = line_end;

        while (val_end > key_beg && (src[val_end - MANIFEST_STEP] == LDG_ASCII_CR || src[val_end - MANIFEST_STEP] == LDG_ASCII_SPACE || src[val_end - MANIFEST_STEP] == LDG_ASCII_TAB)) { val_end = val_end - MANIFEST_STEP; }
        while (key_beg < val_end && (src[key_beg] == LDG_ASCII_SPACE || src[key_beg] == LDG_ASCII_TAB)) { key_beg = key_beg + MANIFEST_STEP; }

        if (key_beg < val_end && src[key_beg] != MANIFEST_COMMENT)
        {
            sep = key_beg;

            while (sep < val_end && src[sep] != MANIFEST_SEP) { sep = sep + MANIFEST_STEP; }

            if (sep < val_end)
            {
                key_end = sep;

                while (key_end > key_beg && (src[key_end - MANIFEST_STEP] == LDG_ASCII_SPACE || src[key_end - MANIFEST_STEP] == LDG_ASCII_TAB)) { key_end = key_end - MANIFEST_STEP; }

                val_beg = sep + MANIFEST_STEP;

                while (val_beg < val_end && (src[val_beg] == LDG_ASCII_SPACE || src[val_beg] == LDG_ASCII_TAB)) { val_beg = val_beg + MANIFEST_STEP; }

                if (key_end > key_beg)
                {
                    ret = manifest_field_set(out, &src[key_beg], key_end - key_beg, &src[val_beg], val_end - val_beg);
                    if (ret != LDG_ERR_AOK) { return ret; }
                }
            }
        }

        pos = line_end + MANIFEST_STEP;
    }

    return manifest_validate(out);
}

uint8_t crosstalk_wshop_manifest_isa_compat_is(const crosstalk_wshop_manifest_t *core, const crosstalk_wshop_manifest_t *img)
{
    if (LDG_UNLIKELY(core == 0x0 || img == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (core->kind != CROSSTALK_WSHOP_MANIFEST_KIND_CORE) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (img->kind != CROSSTALK_WSHOP_MANIFEST_KIND_SOFTWARE) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (manifest_isa_family_is(core->isa, img->tgt_isa) != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    if (img->tgt_isa_rev != 0 && core->isa_rev != 0 && img->tgt_isa_rev > core->isa_rev) { return LDG_TRUTH_FALSE; }

    if (img->tgt_fingerprint_min != 0 && core->fingerprint < img->tgt_fingerprint_min) { return LDG_TRUTH_FALSE; }

    if (img->tgt_fingerprint_max != 0 && core->fingerprint > img->tgt_fingerprint_max) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}
