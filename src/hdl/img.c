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
    return (uint32_t)img_word_rd(raw, at) | ((uint32_t)img_word_rd(raw, at + (uint64_t)CROSSTALK_HDL_IMG_WORD_SIZE) << (uint32_t)(LDG_BYTE_BITS * CROSSTALK_HDL_IMG_WORD_SIZE));
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
    uint8_t exec = (flags & (uint32_t)CROSSTALK_HDL_IMG_PF_EXEC) != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    uint8_t wr = (flags & (uint32_t)CROSSTALK_HDL_IMG_PF_WR) != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(at == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

#ifdef _WIN32
    {
        DWORD mode = PAGE_READONLY;
        DWORD prev = 0;

        if (exec == LDG_TRUTH_TRUE) { mode = PAGE_EXECUTE_READ; }
        else if (wr == LDG_TRUTH_TRUE) { mode = PAGE_READWRITE; }

        if (VirtualProtect(at, span, mode, &prev) == 0) { return CROSSTALK_HDL_ERR_IMG_PROTECT; }
    }
#else
    {
        int32_t mode = PROT_READ;

        if (exec == LDG_TRUTH_TRUE) { mode = mode | PROT_EXEC; }
        else if (wr == LDG_TRUTH_TRUE) { mode = mode | PROT_WRITE; }

        if (mprotect(at, span, mode) != 0) { return CROSSTALK_HDL_ERR_IMG_PROTECT; }
    }
#endif

    return LDG_ERR_AOK;
}

typedef struct img_dyn
{
    uint64_t hash;
    uint64_t sym;
    uint64_t str;
    uint64_t str_span;
    uint64_t rela;
    uint64_t rela_span;
    uint64_t plt;
    uint64_t plt_span;
} img_dyn_t;

static uint8_t img_range_is(uint64_t at, uint64_t len, uint64_t span)
{
    if (at > span) { return LDG_TRUTH_FALSE; }

    return len <= span - at ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t img_dyn_parse(const ldg_byte_t *base, uint64_t span, uint64_t at, uint64_t dyn_span, img_dyn_t *out)
{
    uint64_t pos = 0;

    if (LDG_UNLIKELY(base == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(out, 0x0, sizeof(*out));

    if (img_range_is(at, dyn_span, span) != LDG_TRUTH_TRUE) { return CROSSTALK_HDL_ERR_IMG_FORM_DIR; }

    for (pos = 0; pos + (uint64_t)CROSSTALK_HDL_IMG_DYN_SIZE <= dyn_span; pos = pos + (uint64_t)CROSSTALK_HDL_IMG_DYN_SIZE)
    {
        uint64_t tag = img_qword_rd(base, at + pos);
        uint64_t val = img_qword_rd(base, at + pos + (uint64_t)CROSSTALK_HDL_IMG_QWORD_SIZE);

        if (tag == (uint64_t)CROSSTALK_HDL_IMG_DT_NULL) { break; }

        switch (tag)
        {
            case CROSSTALK_HDL_IMG_DT_NEEDED:
                return CROSSTALK_HDL_ERR_IMG_IMPORT;
            case CROSSTALK_HDL_IMG_DT_REL:
                return CROSSTALK_HDL_ERR_IMG_RELOC;
            case CROSSTALK_HDL_IMG_DT_HASH:
                out->hash = val;
                break;
            case CROSSTALK_HDL_IMG_DT_SYMTAB:
                out->sym = val;
                break;
            case CROSSTALK_HDL_IMG_DT_STRTAB:
                out->str = val;
                break;
            case CROSSTALK_HDL_IMG_DT_STRSZ:
                out->str_span = val;
                break;
            case CROSSTALK_HDL_IMG_DT_RELA:
                out->rela = val;
                break;
            case CROSSTALK_HDL_IMG_DT_RELASZ:
                out->rela_span = val;
                break;
            case CROSSTALK_HDL_IMG_DT_JMPREL:
                out->plt = val;
                break;
            case CROSSTALK_HDL_IMG_DT_PLTRELSZ:
                out->plt_span = val;
                break;
            case CROSSTALK_HDL_IMG_DT_RELAENT:
                if (val != (uint64_t)CROSSTALK_HDL_IMG_RELA_SIZE) { return CROSSTALK_HDL_ERR_IMG_RELOC; }

                break;
            case CROSSTALK_HDL_IMG_DT_SYMENT:
                if (val != (uint64_t)CROSSTALK_HDL_IMG_SYM_SIZE) { return CROSSTALK_HDL_ERR_IMG_FORM; }

                break;
            case CROSSTALK_HDL_IMG_DT_PLTREL:
                if (val != (uint64_t)CROSSTALK_HDL_IMG_DT_RELA) { return CROSSTALK_HDL_ERR_IMG_RELOC; }

                break;
            default:
                break;
        }
    }

    return LDG_ERR_AOK;
}

static uint32_t img_rela_apply(const crosstalk_hdl_img_t *img, uint64_t tbl, uint64_t tbl_span, uint64_t *fixups)
{
    uint64_t at = 0;

    if (LDG_UNLIKELY(img == 0x0 || fixups == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (tbl_span == (uint64_t)CROSSTALK_HDL_IMG_NONE) { return LDG_ERR_AOK; }

    if (img_range_is(tbl, tbl_span, img->span) != LDG_TRUTH_TRUE) { return CROSSTALK_HDL_ERR_IMG_RELOC; }

    for (at = 0; at + (uint64_t)CROSSTALK_HDL_IMG_RELA_SIZE <= tbl_span; at = at + (uint64_t)CROSSTALK_HDL_IMG_RELA_SIZE)
    {
        uint64_t entry = tbl + at;
        uint64_t slot = img_qword_rd(img->base, entry);
        uint64_t info = img_qword_rd(img->base, entry + (uint64_t)CROSSTALK_HDL_IMG_RELA_OFF_INFO);
        uint64_t addend = img_qword_rd(img->base, entry + (uint64_t)CROSSTALK_HDL_IMG_RELA_OFF_ADDEND);
        uint64_t kind = info & CROSSTALK_HDL_IMG_RELA_KIND_MASK;
        uint64_t sym = info >> (uint64_t)CROSSTALK_HDL_IMG_RELA_SYM_SHIFT;
        uint64_t val = 0;

        if (kind == (uint64_t)CROSSTALK_HDL_IMG_R_NONE) { continue; }

        if (img_range_is(slot, (uint64_t)CROSSTALK_HDL_IMG_QWORD_SIZE, img->span) != LDG_TRUTH_TRUE) { return CROSSTALK_HDL_ERR_IMG_RELOC; }

        if (kind == (uint64_t)CROSSTALK_HDL_IMG_R_RELATIVE) { val = (uint64_t)(uintptr_t)img->base + addend; }
        else if (kind == (uint64_t)CROSSTALK_HDL_IMG_R_64 || kind == (uint64_t)CROSSTALK_HDL_IMG_R_GLOB_DAT || kind == (uint64_t)CROSSTALK_HDL_IMG_R_JMP_SLOT)
        {
            uint64_t ent = img->sym_rva + sym * (uint64_t)CROSSTALK_HDL_IMG_SYM_SIZE;

            if (sym >= (uint64_t)img->sym_cunt) { return CROSSTALK_HDL_ERR_IMG_SYM; }

            if (img_word_rd(img->base, ent + (uint64_t)CROSSTALK_HDL_IMG_SYM_OFF_SECT) == (uint16_t)CROSSTALK_HDL_IMG_SYM_SECT_UNDEF) { return CROSSTALK_HDL_ERR_IMG_IMPORT; }

            val = (uint64_t)(uintptr_t)img->base + img_qword_rd(img->base, ent + (uint64_t)CROSSTALK_HDL_IMG_SYM_OFF_VAL) + addend;
        }
        else { return CROSSTALK_HDL_ERR_IMG_RELOC; }

        img_qword_wr(img->base, slot, val);
        *fixups = *fixups + (uint64_t)CROSSTALK_HDL_IMG_STEP;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_img_ld(const ldg_byte_t *raw, uint64_t span, crosstalk_hdl_img_t *out)
{
    img_dyn_t dyn = LDG_STRUCT_ZERO_INIT;
    ldg_byte_t *base = 0x0;
    uint64_t ph = 0;
    uint64_t want = 0;
    uint64_t dyn_rva = 0;
    uint64_t dyn_span = 0;
    uint64_t fixups = 0;
    uint32_t ph_cunt = 0;
    uint32_t seg_cunt = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;
    uint8_t dyn_found = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(raw == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(out, 0x0, sizeof(*out));

    if (span < (uint64_t)CROSSTALK_HDL_IMG_HDR_SIZE) { return CROSSTALK_HDL_ERR_IMG_TRUNC; }

    if (img_dword_rd(raw, 0x0) != (uint32_t)CROSSTALK_HDL_IMG_ELF_MAGIC) { return CROSSTALK_HDL_ERR_IMG_MAGIC; }

    if (raw[CROSSTALK_HDL_IMG_OFF_CLASS] != (ldg_byte_t)CROSSTALK_HDL_IMG_ELF_CLASS_64 || raw[CROSSTALK_HDL_IMG_OFF_DATA] != (ldg_byte_t)CROSSTALK_HDL_IMG_ELF_DATA_LSB) { return CROSSTALK_HDL_ERR_IMG_FORM; }

    if (img_word_rd(raw, (uint64_t)CROSSTALK_HDL_IMG_OFF_TYPE) != (uint16_t)CROSSTALK_HDL_IMG_ELF_TYPE_DYN) { return CROSSTALK_HDL_ERR_IMG_FORM; }

    if (img_word_rd(raw, (uint64_t)CROSSTALK_HDL_IMG_OFF_MACHINE) != (uint16_t)CROSSTALK_HDL_IMG_ELF_MACHINE_X86_64) { return CROSSTALK_HDL_ERR_IMG_MACHINE; }

    ph = img_qword_rd(raw, (uint64_t)CROSSTALK_HDL_IMG_OFF_PH);
    ph_cunt = (uint32_t)img_word_rd(raw, (uint64_t)CROSSTALK_HDL_IMG_OFF_PH_CUNT);

    if (img_word_rd(raw, (uint64_t)CROSSTALK_HDL_IMG_OFF_PH_SIZE) != (uint16_t)CROSSTALK_HDL_IMG_PH_SIZE || ph_cunt == (uint32_t)CROSSTALK_HDL_IMG_NONE) { return CROSSTALK_HDL_ERR_IMG_FORM; }

    if (img_range_is(ph, (uint64_t)ph_cunt * (uint64_t)CROSSTALK_HDL_IMG_PH_SIZE, span) != LDG_TRUTH_TRUE) { return CROSSTALK_HDL_ERR_IMG_TRUNC; }

    for (idx = 0; idx < ph_cunt; idx = idx + (uint32_t)CROSSTALK_HDL_IMG_STEP)
    {
        uint64_t entry = ph + (uint64_t)idx * (uint64_t)CROSSTALK_HDL_IMG_PH_SIZE;
        uint32_t kind = img_dword_rd(raw, entry + (uint64_t)CROSSTALK_HDL_IMG_PH_OFF_TYPE);
        uint64_t file = img_qword_rd(raw, entry + (uint64_t)CROSSTALK_HDL_IMG_PH_OFF_FILE);
        uint64_t vaddr = img_qword_rd(raw, entry + (uint64_t)CROSSTALK_HDL_IMG_PH_OFF_VADDR);
        uint64_t file_span = img_qword_rd(raw, entry + (uint64_t)CROSSTALK_HDL_IMG_PH_OFF_FILE_SPAN);
        uint64_t mem_span = img_qword_rd(raw, entry + (uint64_t)CROSSTALK_HDL_IMG_PH_OFF_MEM_SPAN);

        if (kind == (uint32_t)CROSSTALK_HDL_IMG_PT_TLS) { return CROSSTALK_HDL_ERR_IMG_TLS; }

        if (kind == (uint32_t)CROSSTALK_HDL_IMG_PT_INTERP) { return CROSSTALK_HDL_ERR_IMG_IMPORT; }

        if (kind == (uint32_t)CROSSTALK_HDL_IMG_PT_DYNAMIC)
        {
            dyn_rva = vaddr;
            dyn_span = mem_span;
            dyn_found = LDG_TRUTH_TRUE;

            continue;
        }

        if (kind != (uint32_t)CROSSTALK_HDL_IMG_PT_LD) { continue; }

        if ((img_dword_rd(raw, entry + (uint64_t)CROSSTALK_HDL_IMG_PH_OFF_FLAGS) & ((uint32_t)CROSSTALK_HDL_IMG_PF_EXEC | (uint32_t)CROSSTALK_HDL_IMG_PF_WR)) == ((uint32_t)CROSSTALK_HDL_IMG_PF_EXEC | (uint32_t)CROSSTALK_HDL_IMG_PF_WR)) { return CROSSTALK_HDL_ERR_IMG_SECT; }

        if (file_span > mem_span || img_range_is(file, file_span, span) != LDG_TRUTH_TRUE || vaddr > UINT64_MAX - mem_span) { return CROSSTALK_HDL_ERR_IMG_SECT; }

        if (vaddr + mem_span > want) { want = vaddr + mem_span; }

        seg_cunt = seg_cunt + (uint32_t)CROSSTALK_HDL_IMG_STEP;
    }

    if (seg_cunt == (uint32_t)CROSSTALK_HDL_IMG_NONE || want == (uint64_t)CROSSTALK_HDL_IMG_NONE || want > UINT64_MAX - CROSSTALK_HDL_IMG_PAGE_MASK) { return CROSSTALK_HDL_ERR_IMG_FORM_SPAN; }

    if (dyn_found != LDG_TRUTH_TRUE) { return CROSSTALK_HDL_ERR_IMG_FORM_DIR; }

    want = (want + CROSSTALK_HDL_IMG_PAGE_MASK) & ~CROSSTALK_HDL_IMG_PAGE_MASK;

    ret = img_pages_take(want, &base);
    if (ret != LDG_ERR_AOK) { return ret; }

    for (idx = 0; idx < ph_cunt; idx = idx + (uint32_t)CROSSTALK_HDL_IMG_STEP)
    {
        uint64_t entry = ph + (uint64_t)idx * (uint64_t)CROSSTALK_HDL_IMG_PH_SIZE;
        uint64_t file_span = img_qword_rd(raw, entry + (uint64_t)CROSSTALK_HDL_IMG_PH_OFF_FILE_SPAN);
        uint64_t file = img_qword_rd(raw, entry + (uint64_t)CROSSTALK_HDL_IMG_PH_OFF_FILE);
        uint64_t vaddr = img_qword_rd(raw, entry + (uint64_t)CROSSTALK_HDL_IMG_PH_OFF_VADDR);

        if (img_dword_rd(raw, entry + (uint64_t)CROSSTALK_HDL_IMG_PH_OFF_TYPE) != (uint32_t)CROSSTALK_HDL_IMG_PT_LD || file_span == (uint64_t)CROSSTALK_HDL_IMG_NONE) { continue; }

        if (vaddr > want || file_span > want - vaddr || img_range_is(file, file_span, span) != LDG_TRUTH_TRUE)
        {
            ret = CROSSTALK_HDL_ERR_IMG_SECT;
            break;
        }

        memcpy(base + vaddr, raw + file, file_span);
    }

    if (ret != LDG_ERR_AOK)
    {
        if (img_pages_drop(base, want) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "img unmap failed"); }

        return ret;
    }

    out->base = base;
    out->span = want;

    ret = img_dyn_parse(base, want, dyn_rva, dyn_span, &dyn);

    if (ret == LDG_ERR_AOK && dyn.hash != (uint64_t)CROSSTALK_HDL_IMG_NONE) { if (img_range_is(dyn.hash, (uint64_t)CROSSTALK_HDL_IMG_HASH_OFF_CHAIN_CUNT + (uint64_t)CROSSTALK_HDL_IMG_DWORD_SIZE, want) != LDG_TRUTH_TRUE) { ret = CROSSTALK_HDL_ERR_IMG_FORM_DIR; }
        else { out->sym_cunt = img_dword_rd(base, dyn.hash + (uint64_t)CROSSTALK_HDL_IMG_HASH_OFF_CHAIN_CUNT); } }

    if (ret == LDG_ERR_AOK && (img_range_is(dyn.sym, (uint64_t)out->sym_cunt * (uint64_t)CROSSTALK_HDL_IMG_SYM_SIZE, want) != LDG_TRUTH_TRUE || img_range_is(dyn.str, dyn.str_span, want) != LDG_TRUTH_TRUE)) { ret = CROSSTALK_HDL_ERR_IMG_FORM_DIR; }

    if (ret == LDG_ERR_AOK)
    {
        out->sym_rva = dyn.sym;
        out->str_rva = dyn.str;
        out->str_span = dyn.str_span;

        ret = img_rela_apply(out, dyn.rela, dyn.rela_span, &fixups);
    }

    if (ret == LDG_ERR_AOK) { ret = img_rela_apply(out, dyn.plt, dyn.plt_span, &fixups); }

    for (idx = 0; ret == LDG_ERR_AOK && idx < ph_cunt; idx = idx + (uint32_t)CROSSTALK_HDL_IMG_STEP)
    {
        uint64_t entry = ph + (uint64_t)idx * (uint64_t)CROSSTALK_HDL_IMG_PH_SIZE;
        uint64_t vaddr = img_qword_rd(raw, entry + (uint64_t)CROSSTALK_HDL_IMG_PH_OFF_VADDR);
        uint64_t mem_span = img_qword_rd(raw, entry + (uint64_t)CROSSTALK_HDL_IMG_PH_OFF_MEM_SPAN);
        uint64_t lo = vaddr & ~CROSSTALK_HDL_IMG_PAGE_MASK;
        uint64_t hi = (vaddr + mem_span + CROSSTALK_HDL_IMG_PAGE_MASK) & ~CROSSTALK_HDL_IMG_PAGE_MASK;

        if (img_dword_rd(raw, entry + (uint64_t)CROSSTALK_HDL_IMG_PH_OFF_TYPE) != (uint32_t)CROSSTALK_HDL_IMG_PT_LD || mem_span == (uint64_t)CROSSTALK_HDL_IMG_NONE) { continue; }

        ret = img_pages_protect(base + lo, hi - lo, img_dword_rd(raw, entry + (uint64_t)CROSSTALK_HDL_IMG_PH_OFF_FLAGS));
    }

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "img: load refused; base: %p; span: %u64; segments: %u32; syms: %u32; fixups: %u64; err: %u32", (void *)base, want, seg_cunt, out->sym_cunt, fixups, ret);

        if (img_pages_drop(base, want) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "img unmap failed"); }

        memset(out, 0x0, sizeof(*out));

        return ret;
    }

    out->ok = LDG_TRUTH_TRUE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "img: mapped; base: %p; span: %u64; segments: %u32; syms: %u32; fixups: %u64", (void *)base, want, seg_cunt, out->sym_cunt, fixups);

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
    uint64_t name_span = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(img == 0x0 || name == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (img->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (img->sym_cunt == (uint32_t)CROSSTALK_HDL_IMG_NONE) { return CROSSTALK_HDL_ERR_IMG_SYM; }

    name_span = (uint64_t)strlen(name) + (uint64_t)LDG_STR_TERM_SIZE;

    for (idx = 0; idx < img->sym_cunt; idx = idx + (uint32_t)CROSSTALK_HDL_IMG_STEP)
    {
        uint64_t ent = img->sym_rva + (uint64_t)idx * (uint64_t)CROSSTALK_HDL_IMG_SYM_SIZE;
        uint64_t name_off = (uint64_t)img_dword_rd(img->base, ent + (uint64_t)CROSSTALK_HDL_IMG_SYM_OFF_NAME);
        uint64_t val = img_qword_rd(img->base, ent + (uint64_t)CROSSTALK_HDL_IMG_SYM_OFF_VAL);

        if (img_word_rd(img->base, ent + (uint64_t)CROSSTALK_HDL_IMG_SYM_OFF_SECT) == (uint16_t)CROSSTALK_HDL_IMG_SYM_SECT_UNDEF) { continue; }

        if (img_range_is(name_off, name_span, img->str_span) != LDG_TRUTH_TRUE) { continue; }

        if (memcmp(img->base + img->str_rva + name_off, name, name_span) != CROSSTALK_HDL_IMG_NONE) { continue; }

        if (val >= img->span) { return CROSSTALK_HDL_ERR_IMG_SYM; }

        *out = (void *)(img->base + val);

        return LDG_ERR_AOK;
    }

    return CROSSTALK_HDL_ERR_IMG_SYM;
}
