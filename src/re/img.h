#ifndef CROSSTALK_RE_IMG_H
#define CROSSTALK_RE_IMG_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/core/types.h>
#include <dangling/io/path.h>
#include <dangling/mem/alloc.h>

#include "hdl/elf.h"

#define CROSSTALK_RE_IMG_ERR_MAGIC 800
#define CROSSTALK_RE_IMG_ERR_SPAN 801
#define CROSSTALK_RE_IMG_ERR_TRUNC 802
#define CROSSTALK_RE_IMG_ERR_ISA 803
#define CROSSTALK_RE_IMG_ERR_SEG_FULL 804
#define CROSSTALK_RE_IMG_ERR_PHDR 805

#define CROSSTALK_RE_IMG_FILE_MAX ((uint64_t)256 * LDG_MIB)
#define CROSSTALK_RE_IMG_SPAN_MAX ((uint64_t)512 * LDG_MIB)
#define CROSSTALK_RE_IMG_FILE_MODE 0644
#define CROSSTALK_RE_IMG_POOL_SLACK 64
#define CROSSTALK_RE_IMG_WORD_BYTES 4
#define CROSSTALK_RE_IMG_SEG_MAX 32
#define CROSSTALK_RE_IMG_SEG_NAME_CAP 16

#define CROSSTALK_RE_IMG_SEG_FLAG_X 0x1
#define CROSSTALK_RE_IMG_SEG_FLAG_W 0x2
#define CROSSTALK_RE_IMG_SEG_FLAG_R 0x4

#define CROSSTALK_RE_IMG_FORM_NONE 0
#define CROSSTALK_RE_IMG_FORM_EMIRU 1
#define CROSSTALK_RE_IMG_FORM_ELF 2
#define CROSSTALK_RE_IMG_FORM_RAW 3

#define CROSSTALK_RE_IMG_ISA_NONE 0
#define CROSSTALK_RE_IMG_ISA_DANGCSA 1
#define CROSSTALK_RE_IMG_ISA_X86 2
#define CROSSTALK_RE_IMG_ISA_X86_64 3
#define CROSSTALK_RE_IMG_ISA_ARM 4
#define CROSSTALK_RE_IMG_ISA_AARCH64 5
#define CROSSTALK_RE_IMG_ISA_MIPS 6
#define CROSSTALK_RE_IMG_ISA_PPC 7
#define CROSSTALK_RE_IMG_ISA_PPC64 8
#define CROSSTALK_RE_IMG_ISA_RISCV 9
#define CROSSTALK_RE_IMG_ISA_CUNT 10

#define CROSSTALK_RE_IMG_EM_I386 3
#define CROSSTALK_RE_IMG_EM_MIPS 8
#define CROSSTALK_RE_IMG_EM_PPC 20
#define CROSSTALK_RE_IMG_EM_PPC64 21
#define CROSSTALK_RE_IMG_EM_ARM 40
#define CROSSTALK_RE_IMG_EM_X86_64 62
#define CROSSTALK_RE_IMG_EM_AARCH64 183
#define CROSSTALK_RE_IMG_EM_RISCV 243
#define CROSSTALK_RE_IMG_EM_DANGCSA 0xBABE

#define CROSSTALK_RE_IMG_SYMS_EXT ".elf"
#define CROSSTALK_RE_IMG_SYMS_DOT '.'
#define CROSSTALK_RE_IMG_SYMS_SEP_POSIX '/'
#define CROSSTALK_RE_IMG_SYMS_SEP_WIN '\\'

typedef struct crosstalk_re_img_seg
{
    char name[CROSSTALK_RE_IMG_SEG_NAME_CAP];
    uint64_t addr;
    uint64_t span;
    uint64_t file_off;
    uint64_t file_span;
    uint32_t flags;
    uint8_t pudding[4];
} crosstalk_re_img_seg_t;

typedef struct crosstalk_re_img_ctx
{
    crosstalk_hdl_elf_ctx_t elf;
    ldg_mem_pool_t *pool;
    ldg_byte_t *bytes;
    crosstalk_re_img_seg_t seg[CROSSTALK_RE_IMG_SEG_MAX];
    char path[LDG_IO_PATH_MAX];
    char syms_path[LDG_IO_PATH_MAX];
    uint64_t base;
    uint64_t lo;
    uint64_t hi;
    uint64_t entry;
    uint64_t file_span;
    uint32_t seg_cunt;
    uint32_t hash;
    uint32_t prog_rev;
    uint32_t form;
    uint32_t isa;
    uint8_t class_is64;
    uint8_t ring;
    uint8_t pic;
    uint8_t syms_ok;
    uint8_t ok;
    uint8_t pudding[7];
} crosstalk_re_img_ctx_t;

uint32_t crosstalk_re_img_init(crosstalk_re_img_ctx_t *ctx);
uint32_t crosstalk_re_img_shutdown(crosstalk_re_img_ctx_t *ctx);
uint32_t crosstalk_re_img_ld(crosstalk_re_img_ctx_t *ctx, const char *path, uint64_t base, uint32_t isa_hint);
uint32_t crosstalk_re_img_clear(crosstalk_re_img_ctx_t *ctx);
uint8_t crosstalk_re_img_ld_is(const crosstalk_re_img_ctx_t *ctx);
uint32_t crosstalk_re_img_rd(const crosstalk_re_img_ctx_t *ctx, uint64_t addr, uint8_t *out, uint64_t cap, uint64_t *out_got);
uint32_t crosstalk_re_img_wr(crosstalk_re_img_ctx_t *ctx, uint64_t addr, const uint8_t *src, uint64_t len);
uint32_t crosstalk_re_img_word_get(const crosstalk_re_img_ctx_t *ctx, uint64_t addr, uint32_t *out);
uint32_t crosstalk_re_img_seg_get(const crosstalk_re_img_ctx_t *ctx, uint32_t idx, const crosstalk_re_img_seg_t **out);
uint32_t crosstalk_re_img_seg_find(const crosstalk_re_img_ctx_t *ctx, uint64_t addr, uint32_t *out);
uint32_t crosstalk_re_img_isa_name_get(uint32_t isa, const char **out);
uint32_t crosstalk_re_img_isa_parse(const char *name, uint32_t *out);
uint32_t crosstalk_re_img_form_name_get(uint32_t form, const char **out);

#endif
