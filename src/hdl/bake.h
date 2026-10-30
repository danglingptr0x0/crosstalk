#ifndef CROSSTALK_HDL_BAKE_H
#define CROSSTALK_HDL_BAKE_H

#include <stdint.h>

#include <dangling/core/types.h>

#include "hdl/abi.h"

#define CROSSTALK_HDL_ERR_BAKE_MAGIC 740
#define CROSSTALK_HDL_ERR_BAKE_REV 741
#define CROSSTALK_HDL_ERR_BAKE_TOOLCHAIN 742
#define CROSSTALK_HDL_ERR_BAKE_NODE_CUNT 743
#define CROSSTALK_HDL_ERR_BAKE_MEM_CUNT 744
#define CROSSTALK_HDL_ERR_BAKE_CLK_CUNT 745
#define CROSSTALK_HDL_ERR_BAKE_SRC 746
#define CROSSTALK_HDL_ERR_BAKE_TOP 747
#define CROSSTALK_HDL_ERR_BAKE_COMP 748
#define CROSSTALK_HDL_ERR_BAKE_TRUNC 749
#define CROSSTALK_HDL_ERR_BAKE_KIND 750

#define CROSSTALK_HDL_BAKE_MAGIC "CTBAKE\0\0"
#define CROSSTALK_HDL_BAKE_MAGIC_SIZE 8
#define CROSSTALK_HDL_BAKE_REV 1
#define CROSSTALK_HDL_BAKE_TOOLCHAIN_REV 1
#define CROSSTALK_HDL_BAKE_NAME_CAP 64
#define CROSSTALK_HDL_BAKE_COMP_CAP 48
#define CROSSTALK_HDL_BAKE_STEP 1
#define CROSSTALK_HDL_BAKE_MODE 0644
#define CROSSTALK_HDL_BAKE_WORD_SIZE 8

typedef struct crosstalk_hdl_bake_ident
{
    char top[CROSSTALK_HDL_BAKE_NAME_CAP];
    char comp[CROSSTALK_HDL_BAKE_COMP_CAP];
    uint32_t src_hash;
    uint32_t pudding;
} crosstalk_hdl_bake_ident_t;

typedef struct crosstalk_hdl_bake_hdr
{
    ldg_byte_t magic[CROSSTALK_HDL_BAKE_MAGIC_SIZE];
    char top[CROSSTALK_HDL_BAKE_NAME_CAP];
    char comp[CROSSTALK_HDL_BAKE_COMP_CAP];
    uint32_t rev;
    uint32_t toolchain_rev;
    uint32_t src_hash;
    uint32_t node_cunt;
    uint32_t mem_cunt;
    uint32_t clk_cunt;
} crosstalk_hdl_bake_hdr_t;

uint32_t crosstalk_hdl_bake_ident_build(const char *src, uint32_t src_len, const char *top, const char *comp, crosstalk_hdl_bake_ident_t *out);
uint32_t crosstalk_hdl_bake_wr(const crosstalk_hdl_abi_t *abi, const crosstalk_hdl_bake_ident_t *ident, const char *path);
uint32_t crosstalk_hdl_bake_rd(const crosstalk_hdl_abi_t *abi, const crosstalk_hdl_bake_ident_t *ident, const char *path);

#endif
