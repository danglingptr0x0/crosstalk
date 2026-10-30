#ifndef CROSSTALK_EDA_IO_RECEPTACLE_H
#define CROSSTALK_EDA_IO_RECEPTACLE_H

#include <stdint.h>

#include "hdl/abi.h"
#include "hdl/exec.h"

#include "eda/io/scancode.h"

#define CROSSTALK_EDA_IO_RECEPTACLE_ERR_PORT 946
#define CROSSTALK_EDA_IO_RECEPTACLE_ERR_LIVE 947
#define CROSSTALK_EDA_IO_RECEPTACLE_ERR_FULL 948

#define CROSSTALK_EDA_IO_RECEPTACLE_PORT_CAP 64
#define CROSSTALK_EDA_IO_RECEPTACLE_QUEUE_CAP 1024
#define CROSSTALK_EDA_IO_RECEPTACLE_QUEUE_MASK (CROSSTALK_EDA_IO_RECEPTACLE_QUEUE_CAP - 1)
#define CROSSTALK_EDA_IO_RECEPTACLE_HOLD_MAX 4096

#define CROSSTALK_EDA_IO_RECEPTACLE_PIN_DATA 0
#define CROSSTALK_EDA_IO_RECEPTACLE_PIN_RDY 1
#define CROSSTALK_EDA_IO_RECEPTACLE_PIN_ACK 2
#define CROSSTALK_EDA_IO_RECEPTACLE_PIN_CUNT 3

#define CROSSTALK_EDA_IO_RECEPTACLE_STATE_IDLE 0
#define CROSSTALK_EDA_IO_RECEPTACLE_STATE_HELD 1

#define CROSSTALK_EDA_IO_RECEPTACLE_MODE_RAW 0
#define CROSSTALK_EDA_IO_RECEPTACLE_MODE_SCAN 1

typedef struct crosstalk_eda_io_receptacle_pin
{
    char port[CROSSTALK_EDA_IO_RECEPTACLE_PORT_CAP];
    uint32_t sig_idx;
    uint32_t abi_sig;
    uint8_t bound;
    uint8_t abi_ck;
    uint8_t pudding[6];
} crosstalk_eda_io_receptacle_pin_t;

typedef struct crosstalk_eda_io_receptacle_ctx
{
    crosstalk_eda_io_receptacle_pin_t pin[CROSSTALK_EDA_IO_RECEPTACLE_PIN_CUNT];
    uint8_t queue[CROSSTALK_EDA_IO_RECEPTACLE_QUEUE_CAP];
    uint64_t bytes_total;
    uint64_t dropped;
    uint32_t wr_at;
    uint32_t rd_at;
    uint32_t slot_idx;
    uint32_t state;
    uint32_t held;
    uint32_t mode;
    uint8_t ack_has;
    uint8_t live;
    uint8_t rdy_warned;
    uint8_t pudding[5];
} crosstalk_eda_io_receptacle_ctx_t;

uint32_t crosstalk_eda_io_receptacle_init(crosstalk_eda_io_receptacle_ctx_t *ctx);
uint32_t crosstalk_eda_io_receptacle_attach(crosstalk_eda_io_receptacle_ctx_t *ctx, uint32_t slot_idx, const crosstalk_hdl_exec_ctx_t *exec, const char *data, const char *rdy, const char *ack, uint32_t mode);
uint32_t crosstalk_eda_io_receptacle_detach(crosstalk_eda_io_receptacle_ctx_t *ctx);
uint32_t crosstalk_eda_io_receptacle_put(crosstalk_eda_io_receptacle_ctx_t *ctx, const uint8_t *bytes, uint32_t len);
uint32_t crosstalk_eda_io_receptacle_scan_edge_put(crosstalk_eda_io_receptacle_ctx_t *ctx, uint8_t code, uint8_t down);
uint32_t crosstalk_eda_io_receptacle_cycle_drive(crosstalk_eda_io_receptacle_ctx_t *ctx, uint32_t slot_idx, crosstalk_hdl_exec_ctx_t *exec, const crosstalk_hdl_abi_t *abi);
uint32_t crosstalk_eda_io_receptacle_pending_get(const crosstalk_eda_io_receptacle_ctx_t *ctx, uint32_t *out);
uint8_t crosstalk_eda_io_receptacle_live_is(const crosstalk_eda_io_receptacle_ctx_t *ctx);
uint8_t crosstalk_eda_io_receptacle_idle_is(const crosstalk_eda_io_receptacle_ctx_t *ctx);

#endif
