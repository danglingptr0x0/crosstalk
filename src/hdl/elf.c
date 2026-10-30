#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/macros.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/mem/alloc.h>
#include <dangling/io/file.h>

#include "core/log.h"
#include "hdl/elf.h"

#define ELF_FILE_MODE 0644
#define ELF_IDENT_LEN 16
#define ELF_MAG0 0x7F
#define ELF_MAG1 'E'
#define ELF_MAG2 'L'
#define ELF_MAG3 'F'
#define ELF_CLASS_AT 4
#define ELF_DATA_AT 5
#define ELF_CLASS32 1
#define ELF_CLASS64 2
#define ELF_DATA_LSB 1
#define ELF_HDR32_LEN 52
#define ELF_HDR64_LEN 64
#define ELF_SHOFF32_AT 32
#define ELF_SHENTSIZE32_AT 46
#define ELF_SHNUM32_AT 48
#define ELF_SHOFF64_AT 40
#define ELF_SHENTSIZE64_AT 58
#define ELF_SHNUM64_AT 60
#define ELF_SHENT32_LEN 40
#define ELF_SHENT64_LEN 64
#define ELF_SH_TYPE_AT 4
#define ELF_SH_OFF32_AT 16
#define ELF_SH_SIZE32_AT 20
#define ELF_SH_LINK32_AT 24
#define ELF_SH_OFF64_AT 24
#define ELF_SH_SIZE64_AT 32
#define ELF_SH_LINK64_AT 40
#define ELF_SHT_SYMTAB 2
#define ELF_SHT_STRTAB 3
#define ELF_SYM32_LEN 16
#define ELF_SYM64_LEN 24
#define ELF_SYM32_NAME_AT 0
#define ELF_SYM32_VALUE_AT 4
#define ELF_SYM32_SIZE_AT 8
#define ELF_SYM32_INFO_AT 12
#define ELF_SYM64_NAME_AT 0
#define ELF_SYM64_INFO_AT 4
#define ELF_SYM64_VALUE_AT 8
#define ELF_SYM64_SIZE_AT 16
#define ELF_STT_MASK 0xF
#define ELF_STT_OBJECT 1
#define ELF_STT_FUNC 2
#define ELF_SHNUM_MAX 65535
#define ELF_SECT_BYTES_MAX (64 * LDG_MIB)
#define ELF_POOL_SLACK 64
#define ELF_POOL_ITEM_VAR 0
#define ELF_HDR_AT 0
#define ELF_HEAP_ROOT_AT 0
#define ELF_LABEL_PLAIN_FMT "%s"
#define ELF_LABEL_OFF_FMT "%s+0x%" PRIX64
#define ELF_OSABI_AT 7
#define ELF_MACHINE32_AT 18
#define ELF_MACHINE64_AT 18
#define ELF_EM_386 3
#define ELF_EM_MIPS 8
#define ELF_EM_PPC 20
#define ELF_EM_ARM 40
#define ELF_EM_AMD64 62
#define ELF_EM_AARCH64 183
#define ELF_EM_RISCV 243
#define ELF_EM_DANGCSA 0xBABE
#define ELF_OSABI_DANGLING 0x44
#define ELF_ARCH_386_TEXT "x86"
#define ELF_ARCH_AMD64_TEXT "x86-64"
#define ELF_ARCH_ARM_TEXT "arm"
#define ELF_ARCH_AARCH64_TEXT "aarch64"
#define ELF_ARCH_MIPS_TEXT "mips"
#define ELF_ARCH_PPC_TEXT "ppc"
#define ELF_ARCH_RISCV_TEXT "riscv"
#define ELF_ARCH_DANGCSA_TEXT "DANGCSA"
#define ELF_ARCH_UNKNOWN_FMT "em%u"
#define ELF_ARCH_CLASS32_TEXT "elf32"
#define ELF_ARCH_CLASS64_TEXT "elf64"
#define ELF_ARCH_PLAIN_FMT "%s %s"
#define ELF_ARCH_ABI_FMT "%s %s %s"
#define ELF_ABI_DANGLING_TEXT "DANGLING"

static uint16_t elf_u16_get(const uint8_t *at)
{
    return (uint16_t)((uint16_t)at[0] | ((uint16_t)at[0x1] << LDG_BYTE_BITS));
}

static uint32_t elf_u32_get(const uint8_t *at)
{
    return (uint32_t)at[0] | ((uint32_t)at[0x1] << 8) | ((uint32_t)at[0x2] << 16) | ((uint32_t)at[0x3] << 24);
}

static uint64_t elf_u64_get(const uint8_t *at)
{
    return (uint64_t)elf_u32_get(at) | ((uint64_t)elf_u32_get(at + 0x4) << 32);
}

static uint32_t elf_rd_at(ldg_io_file_t *file, uint64_t off, void *dst, uint64_t len)
{
    uint64_t bytes_rd = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(file == 0x0 || dst == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len == 0) { return LDG_ERR_AOK; }

    ret = ldg_io_file_seek(file, off, LDG_IO_SEEK_START);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_io_file_rd(file, dst, len, &bytes_rd);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(bytes_rd != len)) { return LDG_ERR_IO_RD; }

    return LDG_ERR_AOK;
}

static void elf_name_cpy(char *dst, const char *src, uint32_t cap)
{
    uint32_t at = 0;

    while (at + LDG_STR_TERM_SIZE < cap && src[at] != LDG_STR_TERM)
    {
        dst[at] = src[at];
        at = at + 0x1;
    }

    dst[at] = LDG_STR_TERM;
}

static void elf_arch_fmt(char *out, uint32_t cap, uint32_t machine, uint8_t class_is64, uint8_t osabi)
{
    char machine_text[CROSSTALK_HDL_ELF_ARCH_CAP] = LDG_ARR_ZERO_INIT;
    const char *class_text = class_is64 == LDG_TRUTH_TRUE ? ELF_ARCH_CLASS64_TEXT : ELF_ARCH_CLASS32_TEXT;

    if (machine == (uint32_t)ELF_EM_DANGCSA) { elf_name_cpy(machine_text, ELF_ARCH_DANGCSA_TEXT, (uint32_t)sizeof(machine_text)); }
    else if (machine == (uint32_t)ELF_EM_386) { elf_name_cpy(machine_text, ELF_ARCH_386_TEXT, (uint32_t)sizeof(machine_text)); }
    else if (machine == (uint32_t)ELF_EM_AMD64) { elf_name_cpy(machine_text, ELF_ARCH_AMD64_TEXT, (uint32_t)sizeof(machine_text)); }
    else if (machine == (uint32_t)ELF_EM_ARM) { elf_name_cpy(machine_text, ELF_ARCH_ARM_TEXT, (uint32_t)sizeof(machine_text)); }
    else if (machine == (uint32_t)ELF_EM_AARCH64) { elf_name_cpy(machine_text, ELF_ARCH_AARCH64_TEXT, (uint32_t)sizeof(machine_text)); }
    else if (machine == (uint32_t)ELF_EM_MIPS) { elf_name_cpy(machine_text, ELF_ARCH_MIPS_TEXT, (uint32_t)sizeof(machine_text)); }
    else if (machine == (uint32_t)ELF_EM_PPC) { elf_name_cpy(machine_text, ELF_ARCH_PPC_TEXT, (uint32_t)sizeof(machine_text)); }
    else if (machine == (uint32_t)ELF_EM_RISCV) { elf_name_cpy(machine_text, ELF_ARCH_RISCV_TEXT, (uint32_t)sizeof(machine_text)); }
    else { snprintf(machine_text, sizeof(machine_text), ELF_ARCH_UNKNOWN_FMT, machine); }

    if (osabi == (uint8_t)ELF_OSABI_DANGLING) { snprintf(out, cap, ELF_ARCH_ABI_FMT, machine_text, class_text, ELF_ABI_DANGLING_TEXT); }
    else { snprintf(out, cap, ELF_ARCH_PLAIN_FMT, machine_text, class_text); }
}

static void elf_sym_swap(crosstalk_hdl_elf_sym_t *lhs, crosstalk_hdl_elf_sym_t *rhs)
{
    crosstalk_hdl_elf_sym_t tmp = LDG_STRUCT_ZERO_INIT;

    memcpy(&tmp, lhs, sizeof(tmp));
    memcpy(lhs, rhs, sizeof(tmp));
    memcpy(rhs, &tmp, sizeof(tmp));
}

static void elf_sift(crosstalk_hdl_elf_sym_t *arr, uint32_t root, uint32_t cunt)
{
    uint32_t top = root;
    uint32_t kid = 0;

    while (top * 0x2 + 0x1 < cunt)
    {
        kid = top * 0x2 + 0x1;

        if (kid + 0x1 < cunt && arr[kid + 0x1].addr > arr[kid].addr) { kid = kid + 0x1; }

        if (arr[top].addr >= arr[kid].addr) { return; }

        elf_sym_swap(&arr[top], &arr[kid]);
        top = kid;
    }
}

static void elf_sort(crosstalk_hdl_elf_sym_t *arr, uint32_t cunt)
{
    uint32_t at = 0;

    if (cunt < 0x2) { return; }

    at = cunt / 0x2;
    while (at > 0)
    {
        at = at - 0x1;
        elf_sift(arr, at, cunt);
    }

    at = cunt;
    while (at > 0x1)
    {
        at = at - 0x1;
        elf_sym_swap(&arr[0], &arr[at]);
        elf_sift(arr, (uint32_t)ELF_HEAP_ROOT_AT, at);
    }
}

static uint32_t elf_sym_scan(const uint8_t *tab, uint64_t tab_len, const char *str, uint64_t str_len, uint8_t class_is64, crosstalk_hdl_elf_sym_t *out, uint32_t cap, uint32_t *out_cunt)
{
    uint64_t ent_len = class_is64 == LDG_TRUTH_TRUE ? (uint64_t)ELF_SYM64_LEN : (uint64_t)ELF_SYM32_LEN;
    uint64_t at = 0;
    uint32_t cunt = 0;

    if (LDG_UNLIKELY(tab == 0x0 || str == 0x0 || out_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    while (at + ent_len <= tab_len)
    {
        const uint8_t *ent = tab + at;
        uint64_t value = 0;
        uint64_t size = 0;
        uint32_t name_off = 0;
        uint8_t info = 0;
        uint8_t kind = CROSSTALK_HDL_ELF_KIND_NONE;

        at = at + ent_len;

        if (class_is64 == LDG_TRUTH_TRUE)
        {
            name_off = elf_u32_get(ent + ELF_SYM64_NAME_AT);
            info = ent[ELF_SYM64_INFO_AT];
            value = elf_u64_get(ent + ELF_SYM64_VALUE_AT);
            size = elf_u64_get(ent + ELF_SYM64_SIZE_AT);
        }
        else
        {
            name_off = elf_u32_get(ent + ELF_SYM32_NAME_AT);
            value = (uint64_t)elf_u32_get(ent + ELF_SYM32_VALUE_AT);
            size = (uint64_t)elf_u32_get(ent + ELF_SYM32_SIZE_AT);
            info = ent[ELF_SYM32_INFO_AT];
        }

        if ((info & ELF_STT_MASK) == ELF_STT_FUNC) { kind = CROSSTALK_HDL_ELF_KIND_FUNC; }
        else if ((info & ELF_STT_MASK) == ELF_STT_OBJECT) { kind = CROSSTALK_HDL_ELF_KIND_OBJECT; }

        if (kind == CROSSTALK_HDL_ELF_KIND_NONE) { continue; }

        if (value == 0) { continue; }

        if ((uint64_t)name_off >= str_len) { continue; }

        if (str[name_off] == LDG_STR_TERM) { continue; }

        if (cunt >= cap) { break; }

        if (out != 0x0)
        {
            out[cunt].addr = value;
            out[cunt].size = size;
            out[cunt].kind = kind;
            elf_name_cpy(out[cunt].name, str + name_off, (uint32_t)CROSSTALK_HDL_ELF_NAME_CAP);
        }

        cunt = cunt + 0x1;
    }

    *out_cunt = cunt;
    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_elf_init(crosstalk_hdl_elf_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_elf_clear(crosstalk_hdl_elf_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->pool != 0x0) { ret = ldg_mem_pool_destroy(&ctx->pool); }

    ctx->pool = 0x0;
    ctx->sym = 0x0;
    ctx->sym_cunt = 0;
    ctx->ok = LDG_TRUTH_FALSE;
    ctx->base = 0x0;
    ctx->path[0] = LDG_STR_TERM;
    ctx->arch[0] = LDG_STR_TERM;

    return ret;
}

uint32_t crosstalk_hdl_elf_arch_get(const crosstalk_hdl_elf_ctx_t *ctx, const char **out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (ctx->ok != LDG_TRUTH_TRUE || ctx->arch[0] == LDG_STR_TERM) { return LDG_ERR_NOT_FOUND; }

    *out = ctx->arch;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_elf_shutdown(crosstalk_hdl_elf_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_hdl_elf_clear(ctx);
}

uint8_t crosstalk_hdl_elf_ld_is(const crosstalk_hdl_elf_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->ok != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_hdl_elf_sym_cunt_get(const crosstalk_hdl_elf_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return UINT32_MAX; }

    return ctx->sym_cunt;
}

uint32_t crosstalk_hdl_elf_ld(crosstalk_hdl_elf_ctx_t *ctx, const char *path)
{
    uint8_t ident[ELF_IDENT_LEN] = LDG_ARR_ZERO_INIT;
    uint8_t hdr[ELF_HDR64_LEN] = LDG_ARR_ZERO_INIT;
    ldg_io_file_t *file = 0x0;
    uint8_t *sect = 0x0;
    uint8_t *tab = 0x0;
    char *str = 0x0;
    uint64_t shoff = 0;
    uint64_t tab_off = 0;
    uint64_t tab_len = 0;
    uint64_t str_off = 0;
    uint64_t str_len = 0;
    uint64_t sect_bytes = 0;
    uint64_t hdr_len = 0;
    uint64_t ent_len = 0;
    uint32_t link = 0;
    uint32_t shnum = 0;
    uint32_t shentsize = 0;
    uint32_t at = 0;
    uint32_t cunt = 0;
    uint32_t ret = 0;
    uint8_t class_is64 = LDG_TRUTH_FALSE;
    uint8_t found = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0 || path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_elf_clear(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, ELF_FILE_MODE, &file);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = elf_rd_at(file, (uint64_t)ELF_HDR_AT, ident, (uint64_t)ELF_IDENT_LEN);

    if (ret == LDG_ERR_AOK && (ident[0] != ELF_MAG0 || ident[0x1] != ELF_MAG1 || ident[0x2] != ELF_MAG2 || ident[0x3] != ELF_MAG3)) { ret = LDG_ERR_IO_FMT; }

    if (ret == LDG_ERR_AOK && ident[ELF_DATA_AT] != ELF_DATA_LSB) { ret = LDG_ERR_UNSUPPORTED; }

    if (ret == LDG_ERR_AOK && ident[ELF_CLASS_AT] != ELF_CLASS32 && ident[ELF_CLASS_AT] != ELF_CLASS64) { ret = LDG_ERR_UNSUPPORTED; }

    if (ret == LDG_ERR_AOK)
    {
        class_is64 = ident[ELF_CLASS_AT] == ELF_CLASS64 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
        hdr_len = class_is64 == LDG_TRUTH_TRUE ? (uint64_t)ELF_HDR64_LEN : (uint64_t)ELF_HDR32_LEN;
        ent_len = class_is64 == LDG_TRUTH_TRUE ? (uint64_t)ELF_SHENT64_LEN : (uint64_t)ELF_SHENT32_LEN;
        ret = elf_rd_at(file, (uint64_t)ELF_HDR_AT, hdr, hdr_len);
    }

    if (ret == LDG_ERR_AOK)
    {
        if (class_is64 == LDG_TRUTH_TRUE)
        {
            shoff = elf_u64_get(hdr + ELF_SHOFF64_AT);
            shentsize = (uint32_t)elf_u16_get(hdr + ELF_SHENTSIZE64_AT);
            shnum = (uint32_t)elf_u16_get(hdr + ELF_SHNUM64_AT);
        }
        else
        {
            shoff = (uint64_t)elf_u32_get(hdr + ELF_SHOFF32_AT);
            shentsize = (uint32_t)elf_u16_get(hdr + ELF_SHENTSIZE32_AT);
            shnum = (uint32_t)elf_u16_get(hdr + ELF_SHNUM32_AT);
        }

        if (shoff == 0 || shnum == 0 || shnum > (uint32_t)ELF_SHNUM_MAX) { ret = LDG_ERR_NOT_FOUND; }
        else if ((uint64_t)shentsize != ent_len) { ret = LDG_ERR_IO_FMT; }
    }

    if (ret == LDG_ERR_AOK)
    {
        sect_bytes = (uint64_t)shnum * ent_len;
        if (sect_bytes > (uint64_t)ELF_SECT_BYTES_MAX) { ret = LDG_ERR_BOUNDS; }
    }

    if (ret == LDG_ERR_AOK) { ret = ldg_mem_alloc(sect_bytes, (void **)&sect); }

    if (ret == LDG_ERR_AOK) { ret = elf_rd_at(file, shoff, sect, sect_bytes); }

    if (ret == LDG_ERR_AOK)
    {
        for (at = 0; at < shnum; at = at + 0x1)
        {
            const uint8_t *ent = sect + (uint64_t)at * ent_len;

            if (elf_u32_get(ent + ELF_SH_TYPE_AT) != (uint32_t)ELF_SHT_SYMTAB) { continue; }

            if (class_is64 == LDG_TRUTH_TRUE)
            {
                tab_off = elf_u64_get(ent + ELF_SH_OFF64_AT);
                tab_len = elf_u64_get(ent + ELF_SH_SIZE64_AT);
                link = elf_u32_get(ent + ELF_SH_LINK64_AT);
            }
            else
            {
                tab_off = (uint64_t)elf_u32_get(ent + ELF_SH_OFF32_AT);
                tab_len = (uint64_t)elf_u32_get(ent + ELF_SH_SIZE32_AT);
                link = elf_u32_get(ent + ELF_SH_LINK32_AT);
            }

            found = LDG_TRUTH_TRUE;
            break;
        }

        if (found != LDG_TRUTH_TRUE) { ret = LDG_ERR_NOT_FOUND; }
        else if (link >= shnum) { ret = LDG_ERR_IO_FMT; }
        else if (tab_len == 0 || tab_len > (uint64_t)ELF_SECT_BYTES_MAX) { ret = LDG_ERR_BOUNDS; }
    }

    if (ret == LDG_ERR_AOK)
    {
        const uint8_t *ent = sect + (uint64_t)link * ent_len;

        if (elf_u32_get(ent + ELF_SH_TYPE_AT) != (uint32_t)ELF_SHT_STRTAB) { ret = LDG_ERR_IO_FMT; }
        else if (class_is64 == LDG_TRUTH_TRUE)
        {
            str_off = elf_u64_get(ent + ELF_SH_OFF64_AT);
            str_len = elf_u64_get(ent + ELF_SH_SIZE64_AT);
        }
        else
        {
            str_off = (uint64_t)elf_u32_get(ent + ELF_SH_OFF32_AT);
            str_len = (uint64_t)elf_u32_get(ent + ELF_SH_SIZE32_AT);
        }

        if (ret == LDG_ERR_AOK && (str_len == 0 || str_len > (uint64_t)ELF_SECT_BYTES_MAX)) { ret = LDG_ERR_BOUNDS; }
    }

    if (ret == LDG_ERR_AOK) { ret = ldg_mem_alloc(tab_len, (void **)&tab); }

    if (ret == LDG_ERR_AOK) { ret = ldg_mem_alloc(str_len, (void **)&str); }

    if (ret == LDG_ERR_AOK) { ret = elf_rd_at(file, tab_off, tab, tab_len); }

    if (ret == LDG_ERR_AOK) { ret = elf_rd_at(file, str_off, str, str_len); }

    if (ret == LDG_ERR_AOK) { str[str_len - LDG_STR_TERM_SIZE] = LDG_STR_TERM; }

    if (ret == LDG_ERR_AOK) { ret = elf_sym_scan(tab, tab_len, str, str_len, class_is64, 0x0, (uint32_t)CROSSTALK_HDL_ELF_SYM_MAX, &cunt); }

    if (ret == LDG_ERR_AOK && cunt == 0) { ret = LDG_ERR_EMPTY; }

    if (ret == LDG_ERR_AOK)
    {
        uint64_t need = (uint64_t)cunt * (uint64_t)sizeof(crosstalk_hdl_elf_sym_t);

        ret = ldg_mem_pool_create((uint64_t)ELF_POOL_ITEM_VAR, need + (uint64_t)ELF_POOL_SLACK, &ctx->pool);
        if (ret == LDG_ERR_AOK) { ret = ldg_mem_pool_alloc(ctx->pool, need, (void **)&ctx->sym); }

        if (ret == LDG_ERR_AOK) { ret = elf_sym_scan(tab, tab_len, str, str_len, class_is64, ctx->sym, cunt, &ctx->sym_cunt); }
    }

    if (ret == LDG_ERR_AOK)
    {
        elf_sort(ctx->sym, ctx->sym_cunt);
        elf_arch_fmt(ctx->arch, (uint32_t)CROSSTALK_HDL_ELF_ARCH_CAP, (uint32_t)elf_u16_get(hdr + ELF_MACHINE32_AT), class_is64, ident[ELF_OSABI_AT]);
        elf_name_cpy(ctx->path, path, (uint32_t)CROSSTALK_HDL_ELF_PATH_CAP);
        ctx->gen = ctx->gen + 0x1;
        ctx->ok = LDG_TRUTH_TRUE;
    }

    if (str != 0x0) { ldg_mem_dealloc(str); }

    if (tab != 0x0) { ldg_mem_dealloc(tab); }

    if (sect != 0x0) { ldg_mem_dealloc(sect); }

    if (ldg_io_file_close(file) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "elf close failed"); }

    if (ret != LDG_ERR_AOK)
    {
        if (crosstalk_hdl_elf_clear(ctx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "elf clear failed"); }

        return ret;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_elf_sym_get(const crosstalk_hdl_elf_ctx_t *ctx, uint64_t addr, const crosstalk_hdl_elf_sym_t **out, uint64_t *out_off)
{
    uint32_t lo = 0;
    uint32_t hi = 0;
    uint32_t best = UINT32_MAX;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0 || out_off == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;
    *out_off = 0;

    if (ctx->ok != LDG_TRUTH_TRUE || ctx->sym == 0x0 || ctx->sym_cunt == 0) { return LDG_ERR_NOT_INIT; }

    if (addr < ctx->base) { return LDG_ERR_NOT_FOUND; }

    addr = addr - ctx->base;

    hi = ctx->sym_cunt;
    while (lo < hi)
    {
        uint32_t mid = lo + (hi - lo) / 0x2;

        if (ctx->sym[mid].addr <= addr)
        {
            best = mid;
            lo = mid + 0x1;
        }
        else { hi = mid; }
    }

    if (best == UINT32_MAX) { return LDG_ERR_NOT_FOUND; }

    if (ctx->sym[best].size > 0 && addr >= ctx->sym[best].addr + ctx->sym[best].size) { return LDG_ERR_NOT_FOUND; }

    *out = &ctx->sym[best];
    *out_off = addr - ctx->sym[best].addr;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_elf_label_get(const crosstalk_hdl_elf_ctx_t *ctx, uint64_t addr, char *out, uint32_t cap)
{
    const crosstalk_hdl_elf_sym_t *sym = 0x0;
    uint64_t off = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_BOUNDS; }

    out[0] = LDG_STR_TERM;

    ret = crosstalk_hdl_elf_sym_get(ctx, addr, &sym, &off);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (off == 0) { snprintf(out, cap, ELF_LABEL_PLAIN_FMT, sym->name); }
    else { snprintf(out, cap, ELF_LABEL_OFF_FMT, sym->name, off); }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_elf_base_set(crosstalk_hdl_elf_ctx_t *ctx, uint64_t base)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->base = base;

    return LDG_ERR_AOK;
}

uint64_t crosstalk_hdl_elf_base_get(const crosstalk_hdl_elf_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return UINT64_MAX; }

    return ctx->base;
}

uint32_t crosstalk_hdl_elf_addr_get(const crosstalk_hdl_elf_ctx_t *ctx, const char *name, uint64_t *out)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (name[0] == LDG_STR_TERM) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->ok != LDG_TRUTH_TRUE || ctx->sym == 0x0 || ctx->sym_cunt == 0) { return LDG_ERR_NOT_INIT; }

    for (at = 0; at < ctx->sym_cunt; at = at + 0x1)
    {
        if (strncmp(ctx->sym[at].name, name, (uint64_t)CROSSTALK_HDL_ELF_NAME_CAP) != 0) { continue; }

        *out = ctx->base + ctx->sym[at].addr;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}
