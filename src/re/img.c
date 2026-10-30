#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/file.h>
#include <dangling/mem/alloc.h>
#include <dangling/proto/emiru.h>
#include <dangling/str/str.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "re/img.h"

#define IMG_POOL_ITEM_VAR 0
#define IMG_ELF_IDENT_LEN 16
#define IMG_ELF_MAG0 0x7F
#define IMG_ELF_MAG1 'E'
#define IMG_ELF_MAG2 'L'
#define IMG_ELF_MAG3 'F'
#define IMG_ELF_CLASS_AT 4
#define IMG_ELF_DATA_AT 5
#define IMG_ELF_CLASS32 1
#define IMG_ELF_CLASS64 2
#define IMG_ELF_DATA_LSB 1
#define IMG_ELF_MACHINE_AT 18
#define IMG_ELF_ENTRY_AT 24
#define IMG_ELF_HDR32_LEN 52
#define IMG_ELF_HDR64_LEN 64
#define IMG_ELF_PHOFF32_AT 28
#define IMG_ELF_PHENTSIZE32_AT 42
#define IMG_ELF_PHNUM32_AT 44
#define IMG_ELF_PHOFF64_AT 32
#define IMG_ELF_PHENTSIZE64_AT 54
#define IMG_ELF_PHNUM64_AT 56
#define IMG_ELF_PHENT32_LEN 32
#define IMG_ELF_PHENT64_LEN 56
#define IMG_ELF_PH_TYPE_AT 0
#define IMG_ELF_PH32_OFF_AT 4
#define IMG_ELF_PH32_VADDR_AT 8
#define IMG_ELF_PH32_FILESZ_AT 16
#define IMG_ELF_PH32_MEMSZ_AT 20
#define IMG_ELF_PH32_FLAGS_AT 24
#define IMG_ELF_PH64_FLAGS_AT 4
#define IMG_ELF_PH64_OFF_AT 8
#define IMG_ELF_PH64_VADDR_AT 16
#define IMG_ELF_PH64_FILESZ_AT 32
#define IMG_ELF_PH64_MEMSZ_AT 40
#define IMG_ELF_PT_LD 1
#define IMG_SEG_TEXT_NAME "text"
#define IMG_SEG_DATA_NAME "data"
#define IMG_SEG_BSS_NAME "bss"
#define IMG_SEG_RAW_NAME "raw"
#define IMG_SEG_LD_FMT "ld%u"

static uint16_t img_u16_get(const uint8_t *at)
{
    return (uint16_t)((uint16_t)at[0] | ((uint16_t)at[0x1] << LDG_BYTE_BITS));
}

static uint32_t img_u32_get(const uint8_t *at)
{
    return (uint32_t)img_u16_get(at) | ((uint32_t)img_u16_get(at + 0x2) << (LDG_BYTE_BITS * 0x2));
}

static uint64_t img_u64_get(const uint8_t *at)
{
    return (uint64_t)img_u32_get(at) | ((uint64_t)img_u32_get(at + 0x4) << (LDG_BYTE_BITS * 0x4));
}

static uint32_t img_file_rd(const char *path, ldg_byte_t **out, uint64_t *out_span)
{
    ldg_io_info_t st = LDG_STRUCT_ZERO_INIT;
    ldg_io_file_t *file = 0x0;
    ldg_byte_t *buff = 0x0;
    uint64_t got = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(path == 0x0 || out == 0x0 || out_span == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;
    *out_span = 0;

    ret = ldg_io_path_info_get(path, &st);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (st.size == 0 || st.size > (uint64_t)CROSSTALK_RE_IMG_FILE_MAX) { return CROSSTALK_RE_IMG_ERR_SPAN; }

    ret = ldg_mem_alloc(st.size, (void **)&buff);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, (uint32_t)CROSSTALK_RE_IMG_FILE_MODE, &file);
    if (ret == LDG_ERR_AOK)
    {
        ret = ldg_io_file_rd(file, buff, st.size, &got);

        if (ldg_io_file_close(file) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re img close failed"); }
    }

    if (ret == LDG_ERR_AOK && got != st.size) { ret = LDG_ERR_IO_RD; }

    if (ret != LDG_ERR_AOK)
    {
        ldg_mem_dealloc(buff);

        return ret;
    }

    *out = buff;
    *out_span = st.size;

    return LDG_ERR_AOK;
}

static uint32_t img_seg_push(crosstalk_re_img_ctx_t *ctx, const char *name, uint64_t addr, uint64_t span, uint64_t file_off, uint64_t file_span, uint32_t flags)
{
    crosstalk_re_img_seg_t *seg = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (span == 0) { return LDG_ERR_AOK; }

    if (file_span > span) { return CROSSTALK_RE_IMG_ERR_PHDR; }

    if (addr > UINT64_MAX - span) { return CROSSTALK_RE_IMG_ERR_SPAN; }

    if (ctx->seg_cunt >= (uint32_t)CROSSTALK_RE_IMG_SEG_MAX) { return CROSSTALK_RE_IMG_ERR_SEG_FULL; }

    seg = &ctx->seg[ctx->seg_cunt];

    memset(seg, 0x0, sizeof(*seg));

    ret = ldg_str_rbrcpy(seg->name, name, (uint64_t)sizeof(seg->name));
    if (ret != LDG_ERR_AOK) { return ret; }

    seg->addr = addr;
    seg->span = span;
    seg->file_off = file_off;
    seg->file_span = file_span;
    seg->flags = flags;

    if (addr < ctx->lo) { ctx->lo = addr; }

    if (addr + span > ctx->hi) { ctx->hi = addr + span; }

    ctx->seg_cunt = ctx->seg_cunt + 0x1;

    return LDG_ERR_AOK;
}

static uint32_t img_emiru_layout(crosstalk_re_img_ctx_t *ctx, const ldg_byte_t *file, uint64_t span, uint64_t base)
{
    ldg_emiru_decoded_t decoded = LDG_STRUCT_ZERO_INIT;
    const ldg_emiru_hdr_t *hdr = 0x0;
    uint64_t text_off = (uint64_t)LDG_EMIRU_HDR_SIZE;
    uint64_t no_file = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || file == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (span > (uint64_t)UINT32_MAX) { return CROSSTALK_RE_IMG_ERR_SPAN; }

    ret = ldg_emiru_decode(file, (ldg_dword_t)span, &decoded);
    if (ret != LDG_ERR_AOK) { return ret; }

    hdr = decoded.hdr;

    if (hdr == 0x0) { return CROSSTALK_RE_IMG_ERR_TRUNC; }

    ret = img_seg_push(ctx, IMG_SEG_TEXT_NAME, base, (uint64_t)hdr->text_size, text_off, (uint64_t)hdr->text_size, (uint32_t)(CROSSTALK_RE_IMG_SEG_FLAG_R | CROSSTALK_RE_IMG_SEG_FLAG_X));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = img_seg_push(ctx, IMG_SEG_DATA_NAME, base + (uint64_t)hdr->text_size, (uint64_t)hdr->data_size, text_off + (uint64_t)hdr->text_size, (uint64_t)hdr->data_size, (uint32_t)(CROSSTALK_RE_IMG_SEG_FLAG_R | CROSSTALK_RE_IMG_SEG_FLAG_W));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = img_seg_push(ctx, IMG_SEG_BSS_NAME, base + (uint64_t)hdr->text_size + (uint64_t)hdr->data_size, (uint64_t)hdr->bss_size, no_file, no_file, (uint32_t)(CROSSTALK_RE_IMG_SEG_FLAG_R | CROSSTALK_RE_IMG_SEG_FLAG_W));
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->form = (uint32_t)CROSSTALK_RE_IMG_FORM_EMIRU;
    ctx->isa = (uint32_t)CROSSTALK_RE_IMG_ISA_DANGCSA;
    ctx->entry = base + (uint64_t)hdr->entry;
    ctx->prog_rev = (uint32_t)hdr->prog_rev;
    ctx->ring = hdr->ring;
    ctx->pic = (hdr->flags & (uint8_t)LDG_EMIRU_FLAG_PIC) != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    ctx->class_is64 = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

static uint32_t img_elf_isa_get(uint32_t machine, uint32_t *out)
{
    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (uint32_t)CROSSTALK_RE_IMG_ISA_NONE;

    if (machine == (uint32_t)CROSSTALK_RE_IMG_EM_DANGCSA) { *out = (uint32_t)CROSSTALK_RE_IMG_ISA_DANGCSA; }
    else if (machine == (uint32_t)CROSSTALK_RE_IMG_EM_I386) { *out = (uint32_t)CROSSTALK_RE_IMG_ISA_X86; }
    else if (machine == (uint32_t)CROSSTALK_RE_IMG_EM_X86_64) { *out = (uint32_t)CROSSTALK_RE_IMG_ISA_X86_64; }
    else if (machine == (uint32_t)CROSSTALK_RE_IMG_EM_ARM) { *out = (uint32_t)CROSSTALK_RE_IMG_ISA_ARM; }
    else if (machine == (uint32_t)CROSSTALK_RE_IMG_EM_AARCH64) { *out = (uint32_t)CROSSTALK_RE_IMG_ISA_AARCH64; }
    else if (machine == (uint32_t)CROSSTALK_RE_IMG_EM_MIPS) { *out = (uint32_t)CROSSTALK_RE_IMG_ISA_MIPS; }
    else if (machine == (uint32_t)CROSSTALK_RE_IMG_EM_PPC) { *out = (uint32_t)CROSSTALK_RE_IMG_ISA_PPC; }
    else if (machine == (uint32_t)CROSSTALK_RE_IMG_EM_PPC64) { *out = (uint32_t)CROSSTALK_RE_IMG_ISA_PPC64; }
    else if (machine == (uint32_t)CROSSTALK_RE_IMG_EM_RISCV) { *out = (uint32_t)CROSSTALK_RE_IMG_ISA_RISCV; }
    else { return CROSSTALK_RE_IMG_ERR_ISA; }

    return LDG_ERR_AOK;
}

static uint32_t img_elf_layout(crosstalk_re_img_ctx_t *ctx, const ldg_byte_t *file, uint64_t span, uint64_t base)
{
    char name[CROSSTALK_RE_IMG_SEG_NAME_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t hdr_len = 0;
    uint64_t ent_len = 0;
    uint64_t phoff = 0;
    uint32_t phentsize = 0;
    uint32_t phnum = 0;
    uint32_t machine = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || file == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (span < (uint64_t)IMG_ELF_IDENT_LEN) { return CROSSTALK_RE_IMG_ERR_TRUNC; }

    if (file[IMG_ELF_DATA_AT] != IMG_ELF_DATA_LSB) { return LDG_ERR_UNSUPPORTED; }

    if (file[IMG_ELF_CLASS_AT] != IMG_ELF_CLASS32 && file[IMG_ELF_CLASS_AT] != IMG_ELF_CLASS64) { return CROSSTALK_RE_IMG_ERR_PHDR; }

    ctx->class_is64 = file[IMG_ELF_CLASS_AT] == IMG_ELF_CLASS64 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    hdr_len = ctx->class_is64 == LDG_TRUTH_TRUE ? (uint64_t)IMG_ELF_HDR64_LEN : (uint64_t)IMG_ELF_HDR32_LEN;
    ent_len = ctx->class_is64 == LDG_TRUTH_TRUE ? (uint64_t)IMG_ELF_PHENT64_LEN : (uint64_t)IMG_ELF_PHENT32_LEN;

    if (span < hdr_len) { return CROSSTALK_RE_IMG_ERR_TRUNC; }

    machine = (uint32_t)img_u16_get(file + IMG_ELF_MACHINE_AT);

    ret = img_elf_isa_get(machine, &ctx->isa);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re img: elf machine unknown; machine: %u32", machine); return ret; }

    if (ctx->class_is64 == LDG_TRUTH_TRUE)
    {
        ctx->entry = base + img_u64_get(file + IMG_ELF_ENTRY_AT);
        phoff = img_u64_get(file + IMG_ELF_PHOFF64_AT);
        phentsize = (uint32_t)img_u16_get(file + IMG_ELF_PHENTSIZE64_AT);
        phnum = (uint32_t)img_u16_get(file + IMG_ELF_PHNUM64_AT);
    }
    else
    {
        ctx->entry = base + (uint64_t)img_u32_get(file + IMG_ELF_ENTRY_AT);
        phoff = (uint64_t)img_u32_get(file + IMG_ELF_PHOFF32_AT);
        phentsize = (uint32_t)img_u16_get(file + IMG_ELF_PHENTSIZE32_AT);
        phnum = (uint32_t)img_u16_get(file + IMG_ELF_PHNUM32_AT);
    }

    if (phoff == 0 || phnum == 0 || (uint64_t)phentsize != ent_len) { return CROSSTALK_RE_IMG_ERR_PHDR; }

    if (phoff > span || (uint64_t)phnum * ent_len > span - phoff) { return CROSSTALK_RE_IMG_ERR_TRUNC; }

    for (at = 0; at < phnum; at = at + 0x1)
    {
        const uint8_t *ent = file + phoff + (uint64_t)at * ent_len;
        uint64_t off = 0;
        uint64_t vaddr = 0;
        uint64_t filesz = 0;
        uint64_t memsz = 0;
        uint32_t flags = 0;

        if (img_u32_get(ent + IMG_ELF_PH_TYPE_AT) != (uint32_t)IMG_ELF_PT_LD) { continue; }

        if (ctx->class_is64 == LDG_TRUTH_TRUE)
        {
            flags = img_u32_get(ent + IMG_ELF_PH64_FLAGS_AT);
            off = img_u64_get(ent + IMG_ELF_PH64_OFF_AT);
            vaddr = img_u64_get(ent + IMG_ELF_PH64_VADDR_AT);
            filesz = img_u64_get(ent + IMG_ELF_PH64_FILESZ_AT);
            memsz = img_u64_get(ent + IMG_ELF_PH64_MEMSZ_AT);
        }
        else
        {
            off = (uint64_t)img_u32_get(ent + IMG_ELF_PH32_OFF_AT);
            vaddr = (uint64_t)img_u32_get(ent + IMG_ELF_PH32_VADDR_AT);
            filesz = (uint64_t)img_u32_get(ent + IMG_ELF_PH32_FILESZ_AT);
            memsz = (uint64_t)img_u32_get(ent + IMG_ELF_PH32_MEMSZ_AT);
            flags = img_u32_get(ent + IMG_ELF_PH32_FLAGS_AT);
        }

        if (off > span || filesz > span - off) { return CROSSTALK_RE_IMG_ERR_TRUNC; }

        if (vaddr > UINT64_MAX - base) { return CROSSTALK_RE_IMG_ERR_SPAN; }

        if (snprintf(name, sizeof(name), IMG_SEG_LD_FMT, ctx->seg_cunt) < 0) { return LDG_ERR_STR_FMT; }

        ret = img_seg_push(ctx, name, base + vaddr, memsz, off, filesz, flags);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    if (ctx->seg_cunt == 0) { return CROSSTALK_RE_IMG_ERR_PHDR; }

    ctx->form = (uint32_t)CROSSTALK_RE_IMG_FORM_ELF;

    return LDG_ERR_AOK;
}

static uint32_t img_raw_layout(crosstalk_re_img_ctx_t *ctx, uint64_t span, uint64_t base, uint32_t isa_hint)
{
    uint64_t file_start = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (isa_hint == (uint32_t)CROSSTALK_RE_IMG_ISA_NONE || isa_hint >= (uint32_t)CROSSTALK_RE_IMG_ISA_CUNT) { return CROSSTALK_RE_IMG_ERR_ISA; }

    ret = img_seg_push(ctx, IMG_SEG_RAW_NAME, base, span, file_start, span, (uint32_t)(CROSSTALK_RE_IMG_SEG_FLAG_R | CROSSTALK_RE_IMG_SEG_FLAG_W | CROSSTALK_RE_IMG_SEG_FLAG_X));
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->form = (uint32_t)CROSSTALK_RE_IMG_FORM_RAW;
    ctx->isa = isa_hint;
    ctx->entry = base;
    ctx->class_is64 = (isa_hint == (uint32_t)CROSSTALK_RE_IMG_ISA_X86_64 || isa_hint == (uint32_t)CROSSTALK_RE_IMG_ISA_AARCH64 || isa_hint == (uint32_t)CROSSTALK_RE_IMG_ISA_PPC64) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

static uint32_t img_place(crosstalk_re_img_ctx_t *ctx, const ldg_byte_t *file)
{
    uint64_t span = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || file == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->seg_cunt == 0 || ctx->hi <= ctx->lo) { return CROSSTALK_RE_IMG_ERR_SPAN; }

    span = ctx->hi - ctx->lo;

    if (span > (uint64_t)CROSSTALK_RE_IMG_SPAN_MAX) { return CROSSTALK_RE_IMG_ERR_SPAN; }

    ret = ldg_mem_pool_create((uint64_t)IMG_POOL_ITEM_VAR, span + (uint64_t)CROSSTALK_RE_IMG_POOL_SLACK, &ctx->pool);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_mem_pool_alloc(ctx->pool, span, (void **)&ctx->bytes);
    if (ret != LDG_ERR_AOK) { return ret; }

    memset(ctx->bytes, 0x0, span);

    for (idx = 0; idx < ctx->seg_cunt; idx = idx + 0x1)
    {
        const crosstalk_re_img_seg_t *seg = &ctx->seg[idx];

        if (seg->file_span == 0) { continue; }

        memcpy(ctx->bytes + (seg->addr - ctx->lo), file + seg->file_off, seg->file_span);
    }

    return LDG_ERR_AOK;
}

static uint32_t img_syms_path_build(const char *path, char *out, uint32_t cap)
{
    uint32_t at = 0;
    uint32_t dot_at = UINT32_MAX;
    uint32_t sep_at = UINT32_MAX;

    if (LDG_UNLIKELY(path == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (at + LDG_STR_TERM_SIZE < cap && path[at] != LDG_STR_TERM)
    {
        if (path[at] == CROSSTALK_RE_IMG_SYMS_DOT) { dot_at = at; }

        if (path[at] == CROSSTALK_RE_IMG_SYMS_SEP_POSIX || path[at] == CROSSTALK_RE_IMG_SYMS_SEP_WIN) { sep_at = at; }

        out[at] = path[at];
        at = at + 0x1;
    }

    out[at] = LDG_STR_TERM;

    if (dot_at == UINT32_MAX) { return LDG_ERR_NOT_FOUND; }

    if (sep_at != UINT32_MAX && dot_at < sep_at) { return LDG_ERR_NOT_FOUND; }

    if ((uint64_t)dot_at + sizeof(CROSSTALK_RE_IMG_SYMS_EXT) > (uint64_t)cap) { return LDG_ERR_STR_TRUNC; }

    memcpy(out + dot_at, CROSSTALK_RE_IMG_SYMS_EXT, sizeof(CROSSTALK_RE_IMG_SYMS_EXT));

    return LDG_ERR_AOK;
}

static uint32_t img_syms_ld(crosstalk_re_img_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->syms_ok = LDG_TRUTH_FALSE;

    if (ctx->form == (uint32_t)CROSSTALK_RE_IMG_FORM_ELF)
    {
        ret = ldg_str_rbrcpy(ctx->syms_path, ctx->path, (uint64_t)sizeof(ctx->syms_path));
        if (ret != LDG_ERR_AOK) { return ret; }
    }
    else
    {
        ret = img_syms_path_build(ctx->path, ctx->syms_path, (uint32_t)sizeof(ctx->syms_path));
        if (ret != LDG_ERR_AOK) { ctx->syms_path[0] = LDG_STR_TERM; return ret; }

        if (ldg_io_path_exists_is(ctx->syms_path) != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_FOUND; }
    }

    ret = crosstalk_hdl_elf_ld(&ctx->elf, ctx->syms_path);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_hdl_elf_base_set(&ctx->elf, ctx->base);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->syms_ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_img_init(crosstalk_re_img_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->lo = UINT64_MAX;

    return crosstalk_hdl_elf_init(&ctx->elf);
}

uint32_t crosstalk_re_img_clear(crosstalk_re_img_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->pool != 0x0) { ret = ldg_mem_pool_destroy(&ctx->pool); }

    if (crosstalk_hdl_elf_clear(&ctx->elf) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re img elf clear failed"); }

    ctx->pool = 0x0;
    ctx->bytes = 0x0;
    memset(ctx->seg, 0x0, sizeof(ctx->seg));
    ctx->seg_cunt = 0;
    ctx->base = 0;
    ctx->lo = UINT64_MAX;
    ctx->hi = 0;
    ctx->entry = 0;
    ctx->file_span = 0;
    ctx->hash = 0;
    ctx->prog_rev = 0;
    ctx->form = (uint32_t)CROSSTALK_RE_IMG_FORM_NONE;
    ctx->isa = (uint32_t)CROSSTALK_RE_IMG_ISA_NONE;
    ctx->class_is64 = LDG_TRUTH_FALSE;
    ctx->ring = 0;
    ctx->pic = LDG_TRUTH_FALSE;
    ctx->syms_ok = LDG_TRUTH_FALSE;
    ctx->ok = LDG_TRUTH_FALSE;
    ctx->path[0] = LDG_STR_TERM;
    ctx->syms_path[0] = LDG_STR_TERM;

    return ret;
}

uint32_t crosstalk_re_img_shutdown(crosstalk_re_img_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_re_img_clear(ctx);

    if (crosstalk_hdl_elf_shutdown(&ctx->elf) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re img elf shutdown failed"); }

    return ret;
}

uint8_t crosstalk_re_img_ld_is(const crosstalk_re_img_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->ok == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_re_img_ld(crosstalk_re_img_ctx_t *ctx, const char *path, uint64_t base, uint32_t isa_hint)
{
    ldg_byte_t *file = 0x0;
    const char *form_name = 0x0;
    const char *isa_name = 0x0;
    uint64_t span = 0;
    uint32_t syms_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_re_img_clear(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = img_file_rd(path, &file, &span);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->base = base;

    if (span >= (uint64_t)LDG_EMIRU_MAGIC_LEN && memcmp(file, LDG_EMIRU_MAGIC, (uint64_t)LDG_EMIRU_MAGIC_LEN) == 0) { ret = img_emiru_layout(ctx, file, span, base); }
    else if (span >= (uint64_t)IMG_ELF_IDENT_LEN && file[0] == IMG_ELF_MAG0 && file[0x1] == IMG_ELF_MAG1 && file[0x2] == IMG_ELF_MAG2 && file[0x3] == IMG_ELF_MAG3) { ret = img_elf_layout(ctx, file, span, base); }
    else { ret = img_raw_layout(ctx, span, base, isa_hint); }

    if (ret == LDG_ERR_AOK) { ret = img_place(ctx, file); }

    if (ret == LDG_ERR_AOK) { ret = ldg_hash_fnv1a_32_compute(file, span, &ctx->hash); }

    ldg_mem_dealloc(file);

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re img: ld failed; path: %.127s; err: %u32", path, ret);

        if (crosstalk_re_img_clear(ctx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re img clear after failure failed"); }

        return ret;
    }

    ctx->file_span = span;

    if (ldg_str_rbrcpy(ctx->path, path, (uint64_t)sizeof(ctx->path)) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re img path truncated"); }

    ctx->ok = LDG_TRUTH_TRUE;

    ret = img_syms_ld(ctx);

    if (ctx->syms_ok == LDG_TRUTH_TRUE) { syms_cunt = crosstalk_hdl_elf_sym_cunt_get(&ctx->elf); }

    if (crosstalk_re_img_form_name_get(ctx->form, &form_name) != LDG_ERR_AOK) { form_name = "?"; }

    if (crosstalk_re_img_isa_name_get(ctx->isa, &isa_name) != LDG_ERR_AOK) { isa_name = "?"; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "re img: loaded; path: %.127s; form: %s; isa: %s; bytes: %u64; base: 0x%X64; lo: 0x%X64; hi: 0x%X64; segs: %u32; entry: 0x%X64; class64: %u32; ring: %u32; pic: %u32; prog rev: %u32; hash: 0x%08X32; syms: %u32; syms path: %.127s; syms err: %u32", path, form_name, isa_name, span, ctx->base, ctx->lo, ctx->hi, ctx->seg_cunt, ctx->entry, (uint32_t)ctx->class_is64, (uint32_t)ctx->ring, (uint32_t)ctx->pic, ctx->prog_rev, ctx->hash, syms_cunt, ctx->syms_path, ret);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_img_seg_find(const crosstalk_re_img_ctx_t *ctx, uint64_t addr, uint32_t *out)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    for (idx = 0; idx < ctx->seg_cunt; idx = idx + 0x1) { if (addr >= ctx->seg[idx].addr && addr < ctx->seg[idx].addr + ctx->seg[idx].span)
        {
            *out = idx;
            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t crosstalk_re_img_seg_get(const crosstalk_re_img_ctx_t *ctx, uint32_t idx, const crosstalk_re_img_seg_t **out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (idx >= ctx->seg_cunt) { return LDG_ERR_BOUNDS; }

    *out = &ctx->seg[idx];

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_img_rd(const crosstalk_re_img_ctx_t *ctx, uint64_t addr, uint8_t *out, uint64_t cap, uint64_t *out_got)
{
    uint64_t room = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0 || out_got == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_got = 0;

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (addr < ctx->lo || addr >= ctx->hi) { return LDG_ERR_BOUNDS; }

    room = ctx->hi - addr;

    if (cap > room) { cap = room; }

    memcpy(out, ctx->bytes + (addr - ctx->lo), cap);

    *out_got = cap;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_img_wr(crosstalk_re_img_ctx_t *ctx, uint64_t addr, const uint8_t *src, uint64_t len)
{
    if (LDG_UNLIKELY(ctx == 0x0 || src == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (len == 0) { return LDG_ERR_AOK; }

    if (addr < ctx->lo || addr >= ctx->hi) { return LDG_ERR_BOUNDS; }

    if (len > ctx->hi - addr) { return LDG_ERR_BOUNDS; }

    memcpy(ctx->bytes + (addr - ctx->lo), src, len);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_img_word_get(const crosstalk_re_img_ctx_t *ctx, uint64_t addr, uint32_t *out)
{
    uint8_t raw[CROSSTALK_RE_IMG_WORD_BYTES] = LDG_ARR_ZERO_INIT;
    uint64_t got = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = crosstalk_re_img_rd(ctx, addr, raw, (uint64_t)CROSSTALK_RE_IMG_WORD_BYTES, &got);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (got != (uint64_t)CROSSTALK_RE_IMG_WORD_BYTES) { return LDG_ERR_BOUNDS; }

    *out = (uint32_t)raw[0] | ((uint32_t)raw[0x1] << LDG_BYTE_SHIFT_1) | ((uint32_t)raw[0x2] << LDG_BYTE_SHIFT_2) | ((uint32_t)raw[0x3] << LDG_BYTE_SHIFT_3);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_img_isa_name_get(uint32_t isa, const char **out)
{
    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (isa == (uint32_t)CROSSTALK_RE_IMG_ISA_NONE) { *out = "none"; }
    else if (isa == (uint32_t)CROSSTALK_RE_IMG_ISA_DANGCSA) { *out = "dangcsa"; }
    else if (isa == (uint32_t)CROSSTALK_RE_IMG_ISA_X86) { *out = "x86"; }
    else if (isa == (uint32_t)CROSSTALK_RE_IMG_ISA_X86_64) { *out = "x86-64"; }
    else if (isa == (uint32_t)CROSSTALK_RE_IMG_ISA_ARM) { *out = "arm"; }
    else if (isa == (uint32_t)CROSSTALK_RE_IMG_ISA_AARCH64) { *out = "aarch64"; }
    else if (isa == (uint32_t)CROSSTALK_RE_IMG_ISA_MIPS) { *out = "mips"; }
    else if (isa == (uint32_t)CROSSTALK_RE_IMG_ISA_PPC) { *out = "ppc"; }
    else if (isa == (uint32_t)CROSSTALK_RE_IMG_ISA_PPC64) { *out = "ppc64"; }
    else if (isa == (uint32_t)CROSSTALK_RE_IMG_ISA_RISCV) { *out = "riscv"; }
    else { return CROSSTALK_RE_IMG_ERR_ISA; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_img_isa_parse(const char *name, uint32_t *out)
{
    const char *known = 0x0;
    uint32_t isa = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(name == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (uint32_t)CROSSTALK_RE_IMG_ISA_NONE;

    for (isa = (uint32_t)CROSSTALK_RE_IMG_ISA_DANGCSA; isa < (uint32_t)CROSSTALK_RE_IMG_ISA_CUNT; isa = isa + 0x1)
    {
        if (crosstalk_re_img_isa_name_get(isa, &known) != LDG_ERR_AOK) { continue; }

        at = 0;
        while (known[at] != LDG_STR_TERM && known[at] == name[at]) { at = at + 0x1; }

        if (known[at] != name[at]) { continue; }

        *out = isa;

        return LDG_ERR_AOK;
    }

    return CROSSTALK_RE_IMG_ERR_ISA;
}

uint32_t crosstalk_re_img_form_name_get(uint32_t form, const char **out)
{
    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (form == (uint32_t)CROSSTALK_RE_IMG_FORM_NONE) { *out = "none"; }
    else if (form == (uint32_t)CROSSTALK_RE_IMG_FORM_EMIRU) { *out = "emiru"; }
    else if (form == (uint32_t)CROSSTALK_RE_IMG_FORM_ELF) { *out = "elf"; }
    else if (form == (uint32_t)CROSSTALK_RE_IMG_FORM_RAW) { *out = "raw"; }
    else { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}
