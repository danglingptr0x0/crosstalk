#ifndef CROSSTALK_EMU_INPUT_H
#define CROSSTALK_EMU_INPUT_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/core/types.h>
#include <dangling/term/vt.h>

#include "hdl/abi.h"

#define CROSSTALK_EMU_INPUT_ERR_PORT 1026
#define CROSSTALK_EMU_INPUT_ERR_FULL 1027

#define CROSSTALK_EMU_INPUT_QUEUE_CAP 1024
#define CROSSTALK_EMU_INPUT_QUEUE_MASK (CROSSTALK_EMU_INPUT_QUEUE_CAP - 1)
#define CROSSTALK_EMU_INPUT_PORT_CAP 64
#define CROSSTALK_EMU_INPUT_HOLD_MAX 4096

#define CROSSTALK_EMU_INPUT_DATA_SIG_DEFAULT "serial_in"
#define CROSSTALK_EMU_INPUT_RDY_SIG_DEFAULT "serial_rdy"
#define CROSSTALK_EMU_INPUT_ACK_SIG_DEFAULT 0x0

#define CROSSTALK_EMU_INPUT_STATE_IDLE 0
#define CROSSTALK_EMU_INPUT_STATE_HELD 1

#define CROSSTALK_EMU_INPUT_MODE_RAW 0
#define CROSSTALK_EMU_INPUT_MODE_SCAN 1

typedef struct crosstalk_emu_input_ctx
{
    uint8_t queue[CROSSTALK_EMU_INPUT_QUEUE_CAP];
    uint64_t bytes_total;
    uint64_t dropped;
    uint32_t data_sig;
    uint32_t rdy_sig;
    uint32_t ack_sig;
    uint32_t wr_at;
    uint32_t rd_at;
    uint32_t state;
    uint32_t held;
    uint32_t mode;
    uint8_t ack_has;
    uint8_t live;
    uint8_t pudding[6];
} crosstalk_emu_input_ctx_t;

uint32_t crosstalk_emu_input_init(crosstalk_emu_input_ctx_t *ctx, uint32_t mode);
uint32_t crosstalk_emu_input_bind(crosstalk_emu_input_ctx_t *ctx, const crosstalk_hdl_abi_t *abi, const char *data, const char *rdy, const char *ack);
uint32_t crosstalk_emu_input_put(crosstalk_emu_input_ctx_t *ctx, const uint8_t *bytes, uint32_t len);
uint32_t crosstalk_emu_input_scan_edge_put(crosstalk_emu_input_ctx_t *ctx, uint8_t code, uint8_t down);
uint32_t crosstalk_emu_input_cycle_drive(crosstalk_emu_input_ctx_t *ctx, const crosstalk_hdl_abi_t *abi);
uint32_t crosstalk_emu_input_key_isr(crosstalk_emu_input_ctx_t *ctx, const ldg_term_key_event_t *key, uint8_t release_report);
uint32_t crosstalk_emu_input_pending_get(const crosstalk_emu_input_ctx_t *ctx, uint32_t *out);
uint8_t crosstalk_emu_input_live_is(const crosstalk_emu_input_ctx_t *ctx);
uint8_t crosstalk_emu_input_idle_is(const crosstalk_emu_input_ctx_t *ctx);

#endif
