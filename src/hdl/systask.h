#ifndef CROSSTALK_HDL_SYSTASK_H
#define CROSSTALK_HDL_SYSTASK_H

#include <stdint.h>

#include "hdl/exec.h"
#include "hdl/ir.h"

#define CROSSTALK_HDL_SYSTASK_LINE_CAP 1024
#define CROSSTALK_HDL_SYSTASK_SCOPE_CAP 64

typedef void (*crosstalk_hdl_systask_out_cb_t)(void *user, const char *line);

typedef struct crosstalk_hdl_systask_ctx
{
    const crosstalk_hdl_ir_t *ir_ctx;
    const char *src;
    crosstalk_hdl_systask_out_cb_t out_cb;
    void *out_user;
    uint32_t *blk_hd;
    uint32_t *blk_cunt;
    uint32_t *comb_task;
    uint64_t *comb_lo;
    uint64_t *comb_hi;
    uint8_t *comb_seen;
    uint8_t *comb_guard;
    double ns_per_cycle;
    uint32_t comb_cunt;
    uint32_t pend_len;
    uint8_t initial_done;
    uint8_t finish_req;
    uint8_t final_done;
    uint8_t pudding[5];
    char scope[CROSSTALK_HDL_SYSTASK_SCOPE_CAP];
    char pend[CROSSTALK_HDL_SYSTASK_LINE_CAP];
} crosstalk_hdl_systask_ctx_t;

uint32_t crosstalk_hdl_systask_init(crosstalk_hdl_systask_ctx_t *ctx, const crosstalk_hdl_ir_t *ir_ctx, const char *src, void *pool);
uint32_t crosstalk_hdl_systask_out_cb_set(crosstalk_hdl_systask_ctx_t *ctx, crosstalk_hdl_systask_out_cb_t cb, void *user);
uint32_t crosstalk_hdl_systask_scope_set(crosstalk_hdl_systask_ctx_t *ctx, const char *scope, uint64_t f_op_hz);
uint32_t crosstalk_hdl_systask_blk_run(crosstalk_hdl_systask_ctx_t *ctx, const crosstalk_hdl_exec_ctx_t *exec, uint32_t blk_idx);
uint32_t crosstalk_hdl_systask_settle_run(crosstalk_hdl_systask_ctx_t *ctx, const crosstalk_hdl_exec_ctx_t *exec);
uint32_t crosstalk_hdl_systask_final_run(crosstalk_hdl_systask_ctx_t *ctx, const crosstalk_hdl_exec_ctx_t *exec);
uint8_t crosstalk_hdl_systask_finish_is(const crosstalk_hdl_systask_ctx_t *ctx);

#endif
