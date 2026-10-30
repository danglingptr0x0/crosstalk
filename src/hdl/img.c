#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/logging/logging.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

#include "core/log.h"
#include "hdl/img.h"

static uint16_t img_word_rd(const ldg_byte_t *raw, uint64_t at)
{
    return (uint16_t)((uint32_t)raw[at] | ((uint32_t)raw[at + CROSSTALK_HDL_IMG_STEP] << (uint32_t)LDG_BYTE_BITS));
}

static uint32_t img_dword_rd(const ldg_byte_t *raw, uint64_t at)
{
    return (uint32_t)img_word_rd(raw, at) | ((uint32_t)img_word_rd(raw, at + (uint64_t)CROSSTALK_HDL_IMG_RELOC_ENTRY_SIZE) << (uint32_t)(LDG_BYTE_BITS * CROSSTALK_HDL_IMG_RELOC_ENTRY_SIZE));
}

static uint64_t img_qword_rd(const ldg_byte_t *raw, uint64_t at)
{
    return (uint64_t)img_dword_rd(raw, at) | ((uint64_t)img_dword_rd(raw, at + (uint64_t)CROSSTALK_HDL_IMG_DWORD_SIZE) << (uint64_t)(LDG_BYTE_BITS * CROSSTALK_HDL_IMG_DWORD_SIZE));
}

static void img_qword_wr(ldg_byte_t *raw, uint64_t at, uint64_t val)
{
    uint32_t idx = 0;

    for (idx = 0; idx < (uint32_t)CROSSTALK_HDL_IMG_QWORD_SIZE; idx = idx + (uint32_t)CROSSTALK_HDL_IMG_STEP) { raw[at + idx] = (ldg_byte_t)((val >> (uint64_t)(idx * LDG_BYTE_BITS)) & (uint64_t)LDG_BYTE_MASK); }
}

static uint32_t img_pages_take(uint64_t span, ldg_byte_t **out)
{
    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

#ifdef _WIN32
    *out = (ldg_byte_t *)VirtualAlloc(0x0, span, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
    {
        void *raw = mmap(0x0, span, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, CROSSTALK_HDL_IMG_MAP_FD_NONE, CROSSTALK_HDL_IMG_MAP_OFF_ORIG);

        if (raw != MAP_FAILED) { *out = (ldg_byte_t *)raw; }
    }
#endif

    if (*out == 0x0) { return CROSSTALK_HDL_ERR_IMG_MAP; }

    return LDG_ERR_AOK;
}

static uint32_t img_pages_drop(ldg_byte_t *base, uint64_t span)
{
    if (LDG_UNLIKELY(base == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

#ifdef _WIN32
    if (VirtualFree(base, 0x0, MEM_RELEASE) == 0) { return CROSSTALK_HDL_ERR_IMG_MAP; }

#else
    if (munmap(base, span) != 0) { return CROSSTALK_HDL_ERR_IMG_MAP; }

#endif

    return LDG_ERR_AOK;
}

static uint32_t img_pages_protect(ldg_byte_t *at, uint64_t span, uint32_t flags)
{
    uint8_t exec = (flags & (uint32_t)CROSSTALK_HDL_IMG_SCN_EXEC) != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    uint8_t wr = (flags & (uint32_t)CROSSTALK_HDL_IMG_SCN_WR) != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(at == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

#ifdef _WIN32
    {
        DWORD mode = PAGE_READONLY;
        DWORD prev = 0;

        if (exec == LDG_TRUTH_TRUE) { mode = wr == LDG_TRUTH_TRUE ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ; }
        else if (wr == LDG_TRUTH_TRUE) { mode = PAGE_READWRITE; }

        if (VirtualProtect(at, span, mode, &prev) == 0) { return CROSSTALK_HDL_ERR_IMG_PROTECT; }
    }
#else
    {
        int32_t mode = PROT_READ;

        if (exec == LDG_TRUTH_TRUE) { mode = mode | PROT_EXEC; }

        if (wr == LDG_TRUTH_TRUE) { mode = mode | PROT_WRITE; }

        if (mprotect(at, span, mode) != 0) { return CROSSTALK_HDL_ERR_IMG_PROTECT; }
    }
#endif

    return LDG_ERR_AOK;
}

static uint32_t img_reloc_apply(ldg_byte_t *base, uint32_t rva, uint32_t span, uint64_t delta, uint64_t *out)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(base == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (delta == 0) { return LDG_ERR_AOK; }

    if (rva == 0 || span == 0) { return CROSSTALK_HDL_ERR_IMG_RELOC_NONE; }

    while (at + (uint32_t)CROSSTALK_HDL_IMG_RELOC_HDR_SIZE <= span)
    {
        uint32_t blk_rva = img_dword_rd(base, (uint64_t)rva + at);
        uint32_t blk_span = img_dword_rd(base, (uint64_t)rva + at + (uint64_t)CROSSTALK_HDL_IMG_DWORD_SIZE);
        uint32_t pos = (uint32_t)CROSSTALK_HDL_IMG_RELOC_HDR_SIZE;

        if (blk_span < (uint32_t)CROSSTALK_HDL_IMG_RELOC_HDR_SIZE || at + blk_span > span) { return CROSSTALK_HDL_ERR_IMG_RELOC_BLK; }

        while (pos + (uint32_t)CROSSTALK_HDL_IMG_RELOC_ENTRY_SIZE <= blk_span)
        {
            uint16_t entry = img_word_rd(base, (uint64_t)rva + at + pos);
            uint32_t kind = (uint32_t)(entry >> (uint32_t)CROSSTALK_HDL_IMG_RELOC_TYPE_SHIFT);
            uint32_t off = (uint32_t)(entry & (uint16_t)CROSSTALK_HDL_IMG_RELOC_OFF_MASK);

            if (kind == (uint32_t)CROSSTALK_HDL_IMG_RELOC_DIR64)
            {
                uint64_t slot = (uint64_t)blk_rva + off;

                img_qword_wr(base, slot, img_qword_rd(base, slot) + delta);
                *out = *out + (uint64_t)CROSSTALK_HDL_IMG_STEP;
            }
            else if (kind != (uint32_t)CROSSTALK_HDL_IMG_RELOC_ABS) { return CROSSTALK_HDL_ERR_IMG_RELOC; }

            pos = pos + (uint32_t)CROSSTALK_HDL_IMG_RELOC_ENTRY_SIZE;
        }

        at = at + blk_span;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_img_ld(const ldg_byte_t *pe, uint64_t span, crosstalk_hdl_img_t *out)
{
    ldg_byte_t *base = 0x0;
    uint64_t nt = 0;
    uint64_t opt = 0;
    uint64_t sect = 0;
    uint64_t want = 0;
    uint64_t link_base = 0;
    uint64_t fixups = 0;
    uint64_t dir = 0;
    uint32_t hdr_span = 0;
    uint32_t dir_cunt = 0;
    uint32_t sect_cunt = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(pe == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(out, 0x0, sizeof(*out));

    if (span < (uint64_t)CROSSTALK_HDL_IMG_OFF_LFANEW + (uint64_t)CROSSTALK_HDL_IMG_DWORD_SIZE) { return CROSSTALK_HDL_ERR_IMG_TRUNC; }

    if (img_word_rd(pe, 0x0) != (uint16_t)CROSSTALK_HDL_IMG_DOS_MAGIC) { return CROSSTALK_HDL_ERR_IMG_MAGIC; }

    nt = (uint64_t)img_dword_rd(pe, (uint64_t)CROSSTALK_HDL_IMG_OFF_LFANEW);
    if (nt + (uint64_t)CROSSTALK_HDL_IMG_OFF_OPT + (uint64_t)CROSSTALK_HDL_IMG_OFF_OPT_DIR > span) { return CROSSTALK_HDL_ERR_IMG_TRUNC; }

    if (img_dword_rd(pe, nt) != (uint32_t)CROSSTALK_HDL_IMG_NT_MAGIC) { return CROSSTALK_HDL_ERR_IMG_MAGIC; }

    if (img_word_rd(pe, nt + (uint64_t)CROSSTALK_HDL_IMG_OFF_MACHINE) != (uint16_t)CROSSTALK_HDL_IMG_MACHINE_AMD64) { return CROSSTALK_HDL_ERR_IMG_MACHINE; }

    opt = nt + (uint64_t)CROSSTALK_HDL_IMG_OFF_OPT;
    dir = opt + (uint64_t)CROSSTALK_HDL_IMG_OFF_OPT_DIR;

    if (img_word_rd(pe, opt) != (uint16_t)CROSSTALK_HDL_IMG_OPT_MAGIC) { return CROSSTALK_HDL_ERR_IMG_FORM; }

    sect_cunt = (uint32_t)img_word_rd(pe, nt + (uint64_t)CROSSTALK_HDL_IMG_OFF_SECT_CUNT);
    want = (uint64_t)img_dword_rd(pe, opt + (uint64_t)CROSSTALK_HDL_IMG_OFF_OPT_SPAN);
    hdr_span = img_dword_rd(pe, opt + (uint64_t)CROSSTALK_HDL_IMG_OFF_OPT_HDR_SPAN);
    link_base = img_qword_rd(pe, opt + (uint64_t)CROSSTALK_HDL_IMG_OFF_OPT_BASE);
    dir_cunt = img_dword_rd(pe, opt + (uint64_t)CROSSTALK_HDL_IMG_OFF_OPT_DIR_CUNT);

    if (want == 0 || hdr_span == 0 || (uint64_t)hdr_span > span) { return CROSSTALK_HDL_ERR_IMG_FORM_SPAN; }

    if (dir_cunt <= (uint32_t)CROSSTALK_HDL_IMG_DIR_RELOC) { return CROSSTALK_HDL_ERR_IMG_FORM_DIR; }

    if (img_dword_rd(pe, dir + (uint64_t)CROSSTALK_HDL_IMG_DIR_IMPORT * (uint64_t)CROSSTALK_HDL_IMG_DIR_ENTRY_SIZE) != 0) { return CROSSTALK_HDL_ERR_IMG_IMPORT; }

    if (dir_cunt > (uint32_t)CROSSTALK_HDL_IMG_DIR_TLS && img_dword_rd(pe, dir + (uint64_t)CROSSTALK_HDL_IMG_DIR_TLS * (uint64_t)CROSSTALK_HDL_IMG_DIR_ENTRY_SIZE) != 0) { return CROSSTALK_HDL_ERR_IMG_TLS; }

    ret = img_pages_take(want, &base);
    if (ret != LDG_ERR_AOK) { return ret; }

    memcpy(base, pe, (uint64_t)hdr_span);

    sect = opt + (uint64_t)img_word_rd(pe, nt + (uint64_t)CROSSTALK_HDL_IMG_OFF_OPT_SIZE);

    for (idx = 0; idx < sect_cunt; idx = idx + (uint32_t)CROSSTALK_HDL_IMG_STEP)
    {
        uint64_t entry = sect + (uint64_t)idx * (uint64_t)CROSSTALK_HDL_IMG_SECT_SIZE;
        uint32_t vaddr = 0;
        uint32_t rspan = 0;
        uint32_t raddr = 0;

        if (entry + (uint64_t)CROSSTALK_HDL_IMG_SECT_SIZE > span) { ret = CROSSTALK_HDL_ERR_IMG_SECT; break; }

        vaddr = img_dword_rd(pe, entry + (uint64_t)CROSSTALK_HDL_IMG_SECT_OFF_VADDR);
        rspan = img_dword_rd(pe, entry + (uint64_t)CROSSTALK_HDL_IMG_SECT_OFF_RSPAN);
        raddr = img_dword_rd(pe, entry + (uint64_t)CROSSTALK_HDL_IMG_SECT_OFF_RADDR);

        if ((uint64_t)vaddr + rspan > want || (uint64_t)raddr + rspan > span) { ret = CROSSTALK_HDL_ERR_IMG_SECT; break; }

        if (rspan != 0) { memcpy(base + vaddr, pe + raddr, (uint64_t)rspan); }
    }

    if (ret == LDG_ERR_AOK)
    {
        uint32_t reloc_rva = img_dword_rd(pe, dir + (uint64_t)CROSSTALK_HDL_IMG_DIR_RELOC * (uint64_t)CROSSTALK_HDL_IMG_DIR_ENTRY_SIZE);
        uint32_t reloc_span = img_dword_rd(pe, dir + (uint64_t)CROSSTALK_HDL_IMG_DIR_RELOC * (uint64_t)CROSSTALK_HDL_IMG_DIR_ENTRY_SIZE + (uint64_t)CROSSTALK_HDL_IMG_DWORD_SIZE);

        ret = img_reloc_apply(base, reloc_rva, reloc_span, (uint64_t)base - link_base, &fixups);

        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "img: relocs missing; link base: 0x%X64; load base: %p; reloc rva: %u32; reloc span: %u32", link_base, (void *)base, reloc_rva, reloc_span); }
    }

    for (idx = 0; ret == LDG_ERR_AOK && idx < sect_cunt; idx = idx + (uint32_t)CROSSTALK_HDL_IMG_STEP)
    {
        uint64_t entry = sect + (uint64_t)idx * (uint64_t)CROSSTALK_HDL_IMG_SECT_SIZE;
        uint32_t vaddr = img_dword_rd(pe, entry + (uint64_t)CROSSTALK_HDL_IMG_SECT_OFF_VADDR);
        uint32_t vspan = img_dword_rd(pe, entry + (uint64_t)CROSSTALK_HDL_IMG_SECT_OFF_VSPAN);
        uint32_t flags = img_dword_rd(pe, entry + (uint64_t)CROSSTALK_HDL_IMG_SECT_OFF_FLAGS);

        if (vspan == 0) { continue; }

        ret = img_pages_protect(base + vaddr, (uint64_t)vspan, flags);
    }

    if (ret != LDG_ERR_AOK)
    {
        if (img_pages_drop(base, want) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "img unmap failed"); }

        return ret;
    }

    out->base = base;
    out->span = want;
    out->exp_rva = img_dword_rd(pe, dir + (uint64_t)CROSSTALK_HDL_IMG_DIR_EXPORT * (uint64_t)CROSSTALK_HDL_IMG_DIR_ENTRY_SIZE);
    out->exp_span = img_dword_rd(pe, dir + (uint64_t)CROSSTALK_HDL_IMG_DIR_EXPORT * (uint64_t)CROSSTALK_HDL_IMG_DIR_ENTRY_SIZE + (uint64_t)CROSSTALK_HDL_IMG_DWORD_SIZE);
    out->ok = LDG_TRUTH_TRUE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "img: mapped; base: %p; span: %u64; sections: %u32; exports: %u32 bytes; link base: 0x%X64; fixups: %u64", (void *)base, want, sect_cunt, out->exp_span, link_base, fixups);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_img_unld(crosstalk_hdl_img_t *img)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(img == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (img->ok != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = img_pages_drop(img->base, img->span);

    memset(img, 0x0, sizeof(*img));

    return ret;
}

uint32_t crosstalk_hdl_img_sym_get(const crosstalk_hdl_img_t *img, const char *name, void **out)
{
    uint32_t func_tbl = 0;
    uint32_t name_tbl = 0;
    uint32_t ord_tbl = 0;
    uint32_t name_cunt = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(img == 0x0 || name == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (img->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (img->exp_rva == 0 || img->exp_span == 0) { return CROSSTALK_HDL_ERR_IMG_SYM; }

    name_cunt = img_dword_rd(img->base, (uint64_t)img->exp_rva + (uint64_t)CROSSTALK_HDL_IMG_EXP_OFF_NAME_CUNT);
    func_tbl = img_dword_rd(img->base, (uint64_t)img->exp_rva + (uint64_t)CROSSTALK_HDL_IMG_EXP_OFF_FUNC_TBL);
    name_tbl = img_dword_rd(img->base, (uint64_t)img->exp_rva + (uint64_t)CROSSTALK_HDL_IMG_EXP_OFF_NAME_TBL);
    ord_tbl = img_dword_rd(img->base, (uint64_t)img->exp_rva + (uint64_t)CROSSTALK_HDL_IMG_EXP_OFF_ORD_TBL);

    for (idx = 0; idx < name_cunt; idx = idx + (uint32_t)CROSSTALK_HDL_IMG_STEP)
    {
        uint32_t name_rva = img_dword_rd(img->base, (uint64_t)name_tbl + (uint64_t)idx * (uint64_t)CROSSTALK_HDL_IMG_DWORD_SIZE);
        uint16_t ord = img_word_rd(img->base, (uint64_t)ord_tbl + (uint64_t)idx * (uint64_t)CROSSTALK_HDL_IMG_RELOC_ENTRY_SIZE);

        if (strcmp((const char *)img->base + name_rva, name) != 0) { continue; }

        *out = (void *)(img->base + img_dword_rd(img->base, (uint64_t)func_tbl + (uint64_t)ord * (uint64_t)CROSSTALK_HDL_IMG_DWORD_SIZE));

        return LDG_ERR_AOK;
    }

    return CROSSTALK_HDL_ERR_IMG_SYM;
}
