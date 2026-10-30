#ifndef CROSSTALK_HDL_ELF_H
#define CROSSTALK_HDL_ELF_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>

#define CROSSTALK_HDL_ELF_NAME_CAP 96
#define CROSSTALK_HDL_ELF_PATH_CAP 512
#define CROSSTALK_HDL_ELF_SYM_MAX 262144
#define CROSSTALK_HDL_ELF_LABEL_CAP 128
#define CROSSTALK_HDL_ELF_SIZELESS_SPAN_MAX 4096
#define CROSSTALK_HDL_ELF_KIND_NONE 0
#define CROSSTALK_HDL_ELF_KIND_FUNC 1
#define CROSSTALK_HDL_ELF_KIND_OBJECT 2
#define CROSSTALK_HDL_ELF_ARCH_CAP 48

typedef struct crosstalk_hdl_elf_sym
{
    uint64_t addr;
    uint64_t size;
    char name[CROSSTALK_HDL_ELF_NAME_CAP];
    uint8_t kind;
    uint8_t pudding[7];
} crosstalk_hdl_elf_sym_t;

typedef struct crosstalk_hdl_elf_ctx
{
    ldg_mem_pool_t *pool;
    crosstalk_hdl_elf_sym_t *sym;
    char path[CROSSTALK_HDL_ELF_PATH_CAP];
    char arch[CROSSTALK_HDL_ELF_ARCH_CAP];
    uint64_t gen;
    uint64_t base;
    uint32_t sym_cunt;
    uint8_t ok;
    uint8_t pudding[3];
} crosstalk_hdl_elf_ctx_t;

uint32_t crosstalk_hdl_elf_init(crosstalk_hdl_elf_ctx_t *ctx);
uint32_t crosstalk_hdl_elf_shutdown(crosstalk_hdl_elf_ctx_t *ctx);
uint32_t crosstalk_hdl_elf_ld(crosstalk_hdl_elf_ctx_t *ctx, const char *path);
uint32_t crosstalk_hdl_elf_clear(crosstalk_hdl_elf_ctx_t *ctx);
uint8_t crosstalk_hdl_elf_ld_is(const crosstalk_hdl_elf_ctx_t *ctx);
uint32_t crosstalk_hdl_elf_sym_get(const crosstalk_hdl_elf_ctx_t *ctx, uint64_t addr, const crosstalk_hdl_elf_sym_t **out, uint64_t *out_off);
uint32_t crosstalk_hdl_elf_label_get(const crosstalk_hdl_elf_ctx_t *ctx, uint64_t addr, char *out, uint32_t cap);
uint32_t crosstalk_hdl_elf_sym_cunt_get(const crosstalk_hdl_elf_ctx_t *ctx);
uint32_t crosstalk_hdl_elf_base_set(crosstalk_hdl_elf_ctx_t *ctx, uint64_t base);
uint64_t crosstalk_hdl_elf_base_get(const crosstalk_hdl_elf_ctx_t *ctx);
uint32_t crosstalk_hdl_elf_addr_get(const crosstalk_hdl_elf_ctx_t *ctx, const char *name, uint64_t *out);
uint32_t crosstalk_hdl_elf_arch_get(const crosstalk_hdl_elf_ctx_t *ctx, const char **out);

#endif
