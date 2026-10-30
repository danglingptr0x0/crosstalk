#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/file.h>
#include <dangling/io/path.h>
#include <dangling/proto/emiemi.h>
#include <dangling/str/str.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "hdl/bake.h"

static uint32_t bake_state_get(const crosstalk_hdl_abi_t *abi, uint32_t kind, uint64_t **out_base, uint32_t *out_cunt)
{
    if (LDG_UNLIKELY(abi == 0x0 || out_base == 0x0 || out_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_base = 0x0;
    *out_cunt = 0x0;

    if (abi->ok != LDG_TRUTH_TRUE || abi->state_get == 0x0) { return LDG_ERR_NOT_INIT; }

    if (abi->state_get(kind, out_base, out_cunt) != LDG_ERR_AOK) { return CROSSTALK_HDL_ERR_BAKE_KIND; }

    if (*out_base == 0x0) { return CROSSTALK_HDL_ERR_BAKE_KIND; }

    return LDG_ERR_AOK;
}

static uint32_t bake_kind_rw(ldg_io_file_t *fh, uint64_t *base, uint32_t cunt, uint8_t wr_is)
{
    uint64_t want = (uint64_t)cunt * (uint64_t)CROSSTALK_HDL_BAKE_WORD_SIZE;
    uint64_t got = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(fh == 0x0 || base == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (cunt == 0) { return LDG_ERR_AOK; }

    if (wr_is == LDG_TRUTH_TRUE) { ret = ldg_io_file_wr(fh, base, want, &got); }
    else { ret = ldg_io_file_rd(fh, base, want, &got); }

    if (ret != LDG_ERR_AOK) { return ret; }

    if (got != want) { return CROSSTALK_HDL_ERR_BAKE_TRUNC; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_bake_ident_build(const char *src, uint32_t src_len, const char *top, const char *comp, crosstalk_hdl_bake_ident_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(src == 0x0 || top == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(out, 0x0, sizeof(*out));

    out->src_hash = (uint32_t)ldg_emiemi_fnv1a_compute((const ldg_byte_t *)src, (ldg_dword_t)src_len);

    ret = ldg_str_rbrcpy(out->top, top, (uint64_t)sizeof(out->top));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    if (comp != 0x0)
    {
        ret = ldg_str_rbrcpy(out->comp, comp, (uint64_t)sizeof(out->comp));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_bake_wr(const crosstalk_hdl_abi_t *abi, const crosstalk_hdl_bake_ident_t *ident, const char *path)
{
    crosstalk_hdl_bake_hdr_t hdr = LDG_STRUCT_ZERO_INIT;
    ldg_io_file_t *fh = 0x0;
    uint64_t *base = 0x0;
    uint64_t wrote = 0;
    uint32_t kind = 0;
    uint32_t cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(abi == 0x0 || ident == 0x0 || path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memcpy(hdr.magic, CROSSTALK_HDL_BAKE_MAGIC, (uint64_t)CROSSTALK_HDL_BAKE_MAGIC_SIZE);

    hdr.rev = (uint32_t)CROSSTALK_HDL_BAKE_REV;
    hdr.toolchain_rev = (uint32_t)CROSSTALK_HDL_BAKE_TOOLCHAIN_REV;
    hdr.src_hash = ident->src_hash;

    if (ident->top[0] != LDG_STR_TERM)
    {
        ret = ldg_str_rbrcpy(hdr.top, ident->top, (uint64_t)sizeof(hdr.top));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "bake: top copy failed; err: %u32", ret); return ret; }
    }

    if (ident->comp[0] != LDG_STR_TERM)
    {
        ret = ldg_str_rbrcpy(hdr.comp, ident->comp, (uint64_t)sizeof(hdr.comp));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "bake: comp copy failed; err: %u32", ret); return ret; }
    }

    ret = LDG_ERR_AOK;

    ret = bake_state_get(abi, (uint32_t)CROSSTALK_HDL_ABI_STATE_NODE, &base, &hdr.node_cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = bake_state_get(abi, (uint32_t)CROSSTALK_HDL_ABI_STATE_MEM, &base, &hdr.mem_cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = bake_state_get(abi, (uint32_t)CROSSTALK_HDL_ABI_STATE_CLK, &base, &hdr.clk_cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_open(path, LDG_IO_WR_ONLY | LDG_IO_CREATE | LDG_IO_TRUNCATE, (uint32_t)CROSSTALK_HDL_BAKE_MODE, &fh);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "bake: open failed; path: %.255s; err: %u32", path, ret);

        return ret;
    }

    ret = ldg_io_file_wr(fh, &hdr, (uint64_t)sizeof(hdr), &wrote);

    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "bake: header write failed; err: %u32; wrote: %u64", ret, wrote); }

    for (kind = 0; ret == LDG_ERR_AOK && kind < (uint32_t)CROSSTALK_HDL_ABI_STATE_KIND_CUNT; kind = kind + (uint32_t)CROSSTALK_HDL_BAKE_STEP)
    {
        if (bake_state_get(abi, kind, &base, &cunt) != LDG_ERR_AOK) { continue; }

        ret = bake_kind_rw(fh, base, cunt, LDG_TRUTH_TRUE);

        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "bake: kind write failed; kind: %u32; cunt: %u32; base: %p; err: %u32", kind, cunt, (void *)base, ret); }
    }

    if (ldg_io_file_close(fh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "bake close failed"); }

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "bake: write failed; path: %.255s; err: %u32", path, ret);

        return ret;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "bake: written; path: %.255s; top: %.63s; nodes: %u32; mem: %u32; hash: %08X32", path, hdr.top, hdr.node_cunt, hdr.mem_cunt, hdr.src_hash);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_bake_rd(const crosstalk_hdl_abi_t *abi, const crosstalk_hdl_bake_ident_t *ident, const char *path)
{
    crosstalk_hdl_bake_hdr_t hdr = LDG_STRUCT_ZERO_INIT;
    ldg_io_file_t *fh = 0x0;
    uint64_t *base = 0x0;
    uint64_t got = 0;
    uint32_t live[CROSSTALK_HDL_ABI_STATE_KIND_CUNT] = LDG_ARR_ZERO_INIT;
    uint32_t kind = 0;
    uint32_t cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(abi == 0x0 || ident == 0x0 || path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (kind = 0; kind < (uint32_t)CROSSTALK_HDL_ABI_STATE_KIND_CUNT; kind = kind + (uint32_t)CROSSTALK_HDL_BAKE_STEP) { if (bake_state_get(abi, kind, &base, &live[kind]) != LDG_ERR_AOK) { live[kind] = 0x0; } }

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, 0x0, &fh);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_rd(fh, &hdr, (uint64_t)sizeof(hdr), &got);

    if (ret == LDG_ERR_AOK && got != (uint64_t)sizeof(hdr)) { ret = CROSSTALK_HDL_ERR_BAKE_TRUNC; }

    if (ret == LDG_ERR_AOK && memcmp(hdr.magic, CROSSTALK_HDL_BAKE_MAGIC, (uint64_t)CROSSTALK_HDL_BAKE_MAGIC_SIZE) != 0) { ret = CROSSTALK_HDL_ERR_BAKE_MAGIC; }

    if (ret == LDG_ERR_AOK && hdr.rev != (uint32_t)CROSSTALK_HDL_BAKE_REV) { ret = CROSSTALK_HDL_ERR_BAKE_REV; }

    if (ret == LDG_ERR_AOK && hdr.toolchain_rev != (uint32_t)CROSSTALK_HDL_BAKE_TOOLCHAIN_REV) { ret = CROSSTALK_HDL_ERR_BAKE_TOOLCHAIN; }

    if (ret == LDG_ERR_AOK && hdr.src_hash != ident->src_hash) { ret = CROSSTALK_HDL_ERR_BAKE_SRC; }

    if (ret == LDG_ERR_AOK && strncmp(hdr.top, ident->top, (uint64_t)sizeof(hdr.top)) != 0) { ret = CROSSTALK_HDL_ERR_BAKE_TOP; }

    if (ret == LDG_ERR_AOK && ident->comp[0] != LDG_STR_TERM && strncmp(hdr.comp, ident->comp, (uint64_t)sizeof(hdr.comp)) != 0) { ret = CROSSTALK_HDL_ERR_BAKE_COMP; }

    if (ret == LDG_ERR_AOK && hdr.node_cunt != live[CROSSTALK_HDL_ABI_STATE_NODE]) { ret = CROSSTALK_HDL_ERR_BAKE_NODE_CUNT; }

    if (ret == LDG_ERR_AOK && hdr.mem_cunt != live[CROSSTALK_HDL_ABI_STATE_MEM]) { ret = CROSSTALK_HDL_ERR_BAKE_MEM_CUNT; }

    if (ret == LDG_ERR_AOK && hdr.clk_cunt != live[CROSSTALK_HDL_ABI_STATE_CLK]) { ret = CROSSTALK_HDL_ERR_BAKE_CLK_CUNT; }

    for (kind = 0; ret == LDG_ERR_AOK && kind < (uint32_t)CROSSTALK_HDL_ABI_STATE_KIND_CUNT; kind = kind + (uint32_t)CROSSTALK_HDL_BAKE_STEP)
    {
        if (bake_state_get(abi, kind, &base, &cunt) != LDG_ERR_AOK) { continue; }

        ret = bake_kind_rw(fh, base, cunt, LDG_TRUTH_FALSE);
    }

    if (ldg_io_file_close(fh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "bake close failed"); }

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "bake: resume refused; path: %.255s; err: %u32", path, ret);

        return ret;
    }

    if (abi->touch != 0x0) { abi->touch(); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "bake: resumed; path: %.255s; top: %.63s; nodes: %u32; mem: %u32", path, hdr.top, hdr.node_cunt, hdr.mem_cunt);

    return LDG_ERR_AOK;
}
