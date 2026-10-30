#ifndef CROSSTALK_EMU_NET_H
#define CROSSTALK_EMU_NET_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/core/types.h>
#include <dangling/io/serial.h>

#include "hdl/abi.h"

#define CROSSTALK_EMU_NET_CHUNK 4096

typedef struct crosstalk_emu_net_ctx
{
    ldg_io_serial_t port;
    uint64_t pipe;
    ldg_byte_t to_host[CROSSTALK_EMU_NET_CHUNK];
    ldg_byte_t to_guest[CROSSTALK_EMU_NET_CHUNK];
    uint64_t to_host_len;
    uint64_t to_host_off;
    uint64_t to_guest_len;
    uint64_t to_guest_off;
    uint64_t sent_total;
    uint64_t recv_total;
    uint8_t open;
    uint8_t pudding[7];
} crosstalk_emu_net_ctx_t;

uint32_t crosstalk_emu_net_init(crosstalk_emu_net_ctx_t *ctx, const char *path);
uint32_t crosstalk_emu_net_shutdown(crosstalk_emu_net_ctx_t *ctx);
uint32_t crosstalk_emu_net_tick(crosstalk_emu_net_ctx_t *ctx, const crosstalk_hdl_abi_t *abi);

#endif
