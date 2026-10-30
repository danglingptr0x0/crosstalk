#ifndef CROSSTALK_HDL_PROC_H
#define CROSSTALK_HDL_PROC_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/core/types.h>
#include <dangling/sys/proc.h>

#define CROSSTALK_HDL_PROC_ERR_STATE 990
#define CROSSTALK_HDL_PROC_ERR_ARGV 991
#define CROSSTALK_HDL_PROC_ERR_SPAWN 992

#define CROSSTALK_HDL_PROC_OUT_CAP 32768
#define CROSSTALK_HDL_PROC_ERR_CAP 16384
#define CROSSTALK_HDL_PROC_IN_CAP 4096
#define CROSSTALK_HDL_PROC_RD_CAP 4096
#define CROSSTALK_HDL_PROC_DRAIN_MAX 64

typedef enum crosstalk_hdl_proc_state
{
    CROSSTALK_HDL_PROC_STATE_IDLE = 0,
    CROSSTALK_HDL_PROC_STATE_RUN,
    CROSSTALK_HDL_PROC_STATE_EXIT
} crosstalk_hdl_proc_state_t;

typedef struct crosstalk_hdl_proc_ctx
{
    ldg_sys_proc_t proc;
    uint64_t rd_total;
    uint64_t wr_total;
    uint32_t out_len;
    uint32_t err_len;
    uint32_t in_len;
    uint32_t in_pos;
    uint32_t exit_code;
    uint32_t state;
    uint8_t out_eof;
    uint8_t err_eof;
    uint8_t pudding[6];
    ldg_byte_t out[CROSSTALK_HDL_PROC_OUT_CAP];
    ldg_byte_t err[CROSSTALK_HDL_PROC_ERR_CAP];
    ldg_byte_t in[CROSSTALK_HDL_PROC_IN_CAP];
} crosstalk_hdl_proc_ctx_t;

uint32_t crosstalk_hdl_proc_init(crosstalk_hdl_proc_ctx_t *ctx);
uint32_t crosstalk_hdl_proc_start(crosstalk_hdl_proc_ctx_t *ctx, const char *path, const char *const *argv, const char *cwd);
uint32_t crosstalk_hdl_proc_pump(crosstalk_hdl_proc_ctx_t *ctx);
uint32_t crosstalk_hdl_proc_wr(crosstalk_hdl_proc_ctx_t *ctx, const ldg_byte_t *buff, uint32_t len, uint32_t *out_taken);
uint32_t crosstalk_hdl_proc_rd(crosstalk_hdl_proc_ctx_t *ctx, ldg_byte_t *buff, uint32_t cap, uint32_t *out_got);
uint32_t crosstalk_hdl_proc_err_get(const crosstalk_hdl_proc_ctx_t *ctx, const ldg_byte_t **out_err, uint32_t *out_len);
uint32_t crosstalk_hdl_proc_exit_code_get(const crosstalk_hdl_proc_ctx_t *ctx, uint32_t *out_code);
uint32_t crosstalk_hdl_proc_stop(crosstalk_hdl_proc_ctx_t *ctx);
uint8_t crosstalk_hdl_proc_alive_is(const crosstalk_hdl_proc_ctx_t *ctx);

#endif
