#ifndef CROSSTALK_WSHOP_MANIFEST_H
#define CROSSTALK_WSHOP_MANIFEST_H

#include <stdint.h>

#include "eda/ic/fpga/fpga.h"

#define CROSSTALK_WSHOP_MANIFEST_ERR_REV 965
#define CROSSTALK_WSHOP_MANIFEST_ERR_KIND 966
#define CROSSTALK_WSHOP_MANIFEST_ERR_FIELD 967
#define CROSSTALK_WSHOP_MANIFEST_ERR_VAL 968
#define CROSSTALK_WSHOP_MANIFEST_ERR_FULL 969

#define CROSSTALK_WSHOP_MANIFEST_REV 1

#define CROSSTALK_WSHOP_MANIFEST_EXT_CORE ".xtcore"
#define CROSSTALK_WSHOP_MANIFEST_EXT_SOFTWARE ".xtimg"

#define CROSSTALK_WSHOP_MANIFEST_KIND_NONE 0
#define CROSSTALK_WSHOP_MANIFEST_KIND_CORE 1
#define CROSSTALK_WSHOP_MANIFEST_KIND_SOFTWARE 2

#define CROSSTALK_WSHOP_MANIFEST_ROLE_NONE 0
#define CROSSTALK_WSHOP_MANIFEST_ROLE_FIRMWARE 1
#define CROSSTALK_WSHOP_MANIFEST_ROLE_BIOS 2
#define CROSSTALK_WSHOP_MANIFEST_ROLE_KERNEL 3
#define CROSSTALK_WSHOP_MANIFEST_ROLE_OS 4
#define CROSSTALK_WSHOP_MANIFEST_ROLE_USERLAND 5

#define CROSSTALK_WSHOP_MANIFEST_ENT_FREE 0
#define CROSSTALK_WSHOP_MANIFEST_ENT_PAID 1

#define CROSSTALK_WSHOP_MANIFEST_NAME_CAP 128
#define CROSSTALK_WSHOP_MANIFEST_AUTHOR_CAP 128
#define CROSSTALK_WSHOP_MANIFEST_LICENSE_CAP 64
#define CROSSTALK_WSHOP_MANIFEST_ISA_CAP 32
#define CROSSTALK_WSHOP_MANIFEST_SIG_CAP 64
#define CROSSTALK_WSHOP_MANIFEST_PART_CAP 64
#define CROSSTALK_WSHOP_MANIFEST_PATH_CAP 260
#define CROSSTALK_WSHOP_MANIFEST_FILES_CAP (CROSSTALK_EDA_IC_FPGA_FILESET_MAX * (CROSSTALK_EDA_IC_FPGA_FILESET_NAME_CAP + 1))
#define CROSSTALK_WSHOP_MANIFEST_MEM_MAX 8
#define CROSSTALK_WSHOP_MANIFEST_IMG_MAX 8

typedef struct crosstalk_wshop_manifest_mem
{
    uint64_t words;
    uint64_t base;
    char sig[CROSSTALK_WSHOP_MANIFEST_SIG_CAP];
} crosstalk_wshop_manifest_mem_t;

typedef struct crosstalk_wshop_manifest_img
{
    uint64_t word_off;
    char path[CROSSTALK_WSHOP_MANIFEST_PATH_CAP];
    char mem[CROSSTALK_WSHOP_MANIFEST_SIG_CAP];
    uint8_t pudding[4];
} crosstalk_wshop_manifest_img_t;

typedef struct crosstalk_wshop_manifest
{
    char name[CROSSTALK_WSHOP_MANIFEST_NAME_CAP];
    char author[CROSSTALK_WSHOP_MANIFEST_AUTHOR_CAP];
    char license[CROSSTALK_WSHOP_MANIFEST_LICENSE_CAP];
    char isa[CROSSTALK_WSHOP_MANIFEST_ISA_CAP];
    char tgt_isa[CROSSTALK_WSHOP_MANIFEST_ISA_CAP];
    char top[CROSSTALK_EDA_IC_FPGA_TOP_CAP];
    char clk[CROSSTALK_WSHOP_MANIFEST_SIG_CAP];
    char part[CROSSTALK_WSHOP_MANIFEST_PART_CAP];
    char files[CROSSTALK_WSHOP_MANIFEST_FILES_CAP];
    char port_serial[CROSSTALK_WSHOP_MANIFEST_SIG_CAP];
    char port_led[CROSSTALK_WSHOP_MANIFEST_SIG_CAP];
    char port_keys[CROSSTALK_WSHOP_MANIFEST_SIG_CAP];
    char port_fb[CROSSTALK_WSHOP_MANIFEST_SIG_CAP];
    char port_hlt[CROSSTALK_WSHOP_MANIFEST_SIG_CAP];
    crosstalk_wshop_manifest_mem_t mem[CROSSTALK_WSHOP_MANIFEST_MEM_MAX];
    crosstalk_wshop_manifest_img_t img[CROSSTALK_WSHOP_MANIFEST_IMG_MAX];
    uint64_t author_id;
    uint64_t fingerprint;
    uint64_t tgt_core;
    uint64_t tgt_fingerprint_min;
    uint64_t tgt_fingerprint_max;
    uint32_t rev;
    uint32_t kind;
    uint32_t role;
    uint32_t entitlement;
    uint32_t isa_rev;
    uint32_t tgt_isa_rev;
    uint32_t toolchain_rev;
    uint32_t mem_cunt;
    uint32_t img_cunt;
    uint8_t pudding[4];
} crosstalk_wshop_manifest_t;

uint32_t crosstalk_wshop_manifest_parse(const char *src, uint64_t len, crosstalk_wshop_manifest_t *out);
uint8_t crosstalk_wshop_manifest_isa_compat_is(const crosstalk_wshop_manifest_t *core, const crosstalk_wshop_manifest_t *img);

#endif
