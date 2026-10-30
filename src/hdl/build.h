#ifndef CROSSTALK_HDL_BUILD_H
#define CROSSTALK_HDL_BUILD_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/io/path.h>
#include <dangling/mem/alloc.h>

#include "hdl/ir.h"
#include "hdl/proc.h"

#define CROSSTALK_HDL_BUILD_ERR_STAGE 994
#define CROSSTALK_HDL_BUILD_ERR_WR 995
#define CROSSTALK_HDL_BUILD_ERR_CC 996
#define CROSSTALK_HDL_BUILD_ERR_POOL 997

#define CROSSTALK_HDL_BUILD_POOL_BYTES 0x8000000
#define CROSSTALK_HDL_BUILD_FILE_MODE 0644
#define CROSSTALK_HDL_BUILD_ARGV_MAX 8

#define CROSSTALK_HDL_BUILD_SRC_NAME "design_gen.c"
#define CROSSTALK_HDL_BUILD_CC_ARGV0 "cc"
#define CROSSTALK_HDL_BUILD_CC_STD "-std=c99"
#define CROSSTALK_HDL_BUILD_CC_MAIN "-DHDL_GEN_MAIN"
#define CROSSTALK_HDL_BUILD_CC_OUT "-o"

#ifdef _WIN32
#define CROSSTALK_HDL_BUILD_BIN_NAME "design_gen.exe"
#else
#define CROSSTALK_HDL_BUILD_BIN_NAME "design_gen"
#endif

typedef enum crosstalk_hdl_build_stage
{
    CROSSTALK_HDL_BUILD_STAGE_IDLE = 0,
    CROSSTALK_HDL_BUILD_STAGE_CC,
    CROSSTALK_HDL_BUILD_STAGE_READY,
    CROSSTALK_HDL_BUILD_STAGE_FAIL
} crosstalk_hdl_build_stage_t;

typedef struct crosstalk_hdl_build_ctx
{
    crosstalk_hdl_proc_ctx_t proc;
    ldg_mem_pool_t *pool;
    uint64_t start_ns;
    uint64_t emit_ns;
    uint64_t end_ns;
    uint32_t gen_bytes;
    uint32_t placed;
    uint32_t residue;
    uint32_t fused;
    uint32_t fast_ok;
    uint32_t exit_code;
    uint32_t stage;
    char dir[LDG_IO_PATH_MAX];
    char cc_path[LDG_IO_PATH_MAX];
    char src_path[LDG_IO_PATH_MAX];
    char bin_path[LDG_IO_PATH_MAX];
    uint8_t ok;
    uint8_t pudding[7];
} crosstalk_hdl_build_ctx_t;

uint32_t crosstalk_hdl_build_init(crosstalk_hdl_build_ctx_t *ctx, const char *dir, const char *cc_path);
uint32_t crosstalk_hdl_build_shutdown(crosstalk_hdl_build_ctx_t *ctx);
uint32_t crosstalk_hdl_build_start(crosstalk_hdl_build_ctx_t *ctx, const crosstalk_hdl_ir_t *ir, const char *src);
uint32_t crosstalk_hdl_build_poll(crosstalk_hdl_build_ctx_t *ctx);
uint32_t crosstalk_hdl_build_cancel(crosstalk_hdl_build_ctx_t *ctx);
uint32_t crosstalk_hdl_build_stage_get(const crosstalk_hdl_build_ctx_t *ctx, uint32_t *out_stage);
uint32_t crosstalk_hdl_build_diag_get(const crosstalk_hdl_build_ctx_t *ctx, const ldg_byte_t **out_diag, uint32_t *out_len);
uint32_t crosstalk_hdl_build_bin_path_get(const crosstalk_hdl_build_ctx_t *ctx, const char **out_path);

#endif
