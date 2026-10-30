#ifndef CROSSTALK_EDA_IC_FPGA_BUILD_H
#define CROSSTALK_EDA_IC_FPGA_BUILD_H

#include <stdint.h>

#include <dangling/io/path.h>
#include <dangling/mem/alloc.h>
#include <dangling/thread/pool.h>

#include "hdl/abi.h"
#include "hdl/cc.h"
#include "hdl/emit.h"
#include "hdl/guard.h"
#include "hdl/img.h"

#define CROSSTALK_EDA_IC_FPGA_BUILD_ERR_LIVE 935
#define CROSSTALK_EDA_IC_FPGA_BUILD_ERR_IDLE 936
#define CROSSTALK_EDA_IC_FPGA_BUILD_ERR_BLOB 937

#define CROSSTALK_EDA_IC_FPGA_BUILD_WORKERS 1
#define CROSSTALK_EDA_IC_FPGA_BUILD_YIELD_NS 200000
#define CROSSTALK_EDA_IC_FPGA_BUILD_STEP 1
#define CROSSTALK_EDA_IC_FPGA_BUILD_TOP_CAP 64
#define CROSSTALK_EDA_IC_FPGA_BUILD_STATUS_CAP 96
#define CROSSTALK_EDA_IC_FPGA_BUILD_BLOB_CAP 33554432
#define CROSSTALK_EDA_IC_FPGA_BUILD_MS_PER_SEC 1000
#define CROSSTALK_EDA_IC_FPGA_BUILD_POOL_MIB 8
#define CROSSTALK_EDA_IC_FPGA_BUILD_POOL_BYTES_MAX ((uint64_t)4 * LDG_GIB)
#define CROSSTALK_EDA_IC_FPGA_BUILD_POOL_GROW_FACTOR 2
#define CROSSTALK_EDA_IC_FPGA_BUILD_SRC_MODE 0644

typedef enum crosstalk_eda_ic_fpga_build_phase
{
    CROSSTALK_EDA_IC_FPGA_BUILD_PHASE_IDLE = 0,
    CROSSTALK_EDA_IC_FPGA_BUILD_PHASE_EMIT,
    CROSSTALK_EDA_IC_FPGA_BUILD_PHASE_CC,
    CROSSTALK_EDA_IC_FPGA_BUILD_PHASE_LD,
    CROSSTALK_EDA_IC_FPGA_BUILD_PHASE_BIND,
    CROSSTALK_EDA_IC_FPGA_BUILD_PHASE_READY,
    CROSSTALK_EDA_IC_FPGA_BUILD_PHASE_FAIL
} crosstalk_eda_ic_fpga_build_phase_t;

typedef struct crosstalk_eda_ic_fpga_build_ctx
{
    ldg_thread_pool_t thread_pool;
    crosstalk_hdl_cc_ctx_t cc;
    crosstalk_hdl_img_t img;
    crosstalk_hdl_abi_t abi;
    crosstalk_hdl_guard_ctx_t guard;
    ldg_mem_pool_t *mem_pool;
    ldg_byte_t *blob;
    const crosstalk_hdl_ir_t *emit_ir;
    const char *emit_src;
    uint32_t emit_mem_words;
    uint32_t emit_len;
    char src_path[LDG_IO_PATH_MAX];
    char out_path[LDG_IO_PATH_MAX];
    char top[CROSSTALK_EDA_IC_FPGA_BUILD_TOP_CAP];
    char status[CROSSTALK_EDA_IC_FPGA_BUILD_STATUS_CAP];
    uint64_t blob_span;
    uint64_t start_ms;
    uint64_t phase_ms;
    uint32_t phase;
    uint32_t phase_seen;
    uint32_t busy;
    uint32_t done;
    uint32_t stop;
    uint32_t err;
    uint32_t slot;
    uint8_t live;
    uint8_t ok;
    uint8_t pf_is;
    uint8_t pudding[5];
} crosstalk_eda_ic_fpga_build_ctx_t;

uint32_t crosstalk_eda_ic_fpga_build_init(crosstalk_eda_ic_fpga_build_ctx_t *ctx, const char *cc_dir);
uint32_t crosstalk_eda_ic_fpga_build_shutdown(crosstalk_eda_ic_fpga_build_ctx_t *ctx);
uint32_t crosstalk_eda_ic_fpga_build_start(crosstalk_eda_ic_fpga_build_ctx_t *ctx, const crosstalk_hdl_ir_t *ir, const char *elab_src, const char *src_path, const char *out_path, const char *top, uint32_t slot, uint8_t pf_is);
uint32_t crosstalk_eda_ic_fpga_build_tick(crosstalk_eda_ic_fpga_build_ctx_t *ctx, uint32_t *out);
uint32_t crosstalk_eda_ic_fpga_build_status_get(crosstalk_eda_ic_fpga_build_ctx_t *ctx, const char **out);
uint8_t crosstalk_eda_ic_fpga_build_live_is(const crosstalk_eda_ic_fpga_build_ctx_t *ctx);

#endif
