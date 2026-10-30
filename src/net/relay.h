#ifndef CROSSTALK_NET_RELAY_H
#define CROSSTALK_NET_RELAY_H

#include <stdint.h>

#include <dangling/core/types.h>
#include <dangling/net/sock.h>
#include <dangling/thread/sync.h>

#include "hdl/abi.h"

#define CROSSTALK_NET_RELAY_STREAM_CAP 4
#define CROSSTALK_NET_RELAY_PAYLOAD_CAP 1508
#define CROSSTALK_NET_RELAY_HDR_BYTES 12
#define CROSSTALK_NET_RELAY_BODY_CAP 65536
#define CROSSTALK_NET_RELAY_URL_CAP 1024
#define CROSSTALK_NET_RELAY_HOST_CAP 256
#define CROSSTALK_NET_RELAY_SERVICE_CAP 16
#define CROSSTALK_NET_RELAY_OPEN_TIMEOUT 5000
#define CROSSTALK_NET_RELAY_CLOSE_TIMEOUT 1000
#define CROSSTALK_NET_RELAY_SEND_TIMEOUT 1000
#define CROSSTALK_NET_RELAY_POLL_TIMEOUT 1
#define CROSSTALK_NET_RELAY_PORT_BYTES 2
#define CROSSTALK_NET_RELAY_WORD_BYTES 4
#define CROSSTALK_NET_RELAY_NUL_BYTES 1
#define CROSSTALK_NET_RELAY_OP_AT 0
#define CROSSTALK_NET_RELAY_ID_AT 4
#define CROSSTALK_NET_RELAY_LEN_AT 8
#define CROSSTALK_NET_RELAY_LEN_NONE 0
#define CROSSTALK_NET_RELAY_OP_NONE 0
#define CROSSTALK_NET_RELAY_CONTENT_HDR "Content-Type: application/json"

#define CROSSTALK_NET_RELAY_OP_OPEN 1
#define CROSSTALK_NET_RELAY_OP_SEND 2
#define CROSSTALK_NET_RELAY_OP_POLL 3
#define CROSSTALK_NET_RELAY_OP_CLOSE 4
#define CROSSTALK_NET_RELAY_OP_POST 5
#define CROSSTALK_NET_RELAY_OP_BODY 6

#define CROSSTALK_NET_RELAY_ST_OK 0
#define CROSSTALK_NET_RELAY_ST_BAD_REQ 1
#define CROSSTALK_NET_RELAY_ST_NO_STREAM 2
#define CROSSTALK_NET_RELAY_ST_CONN 3
#define CROSSTALK_NET_RELAY_ST_CLOSED 4
#define CROSSTALK_NET_RELAY_ST_TRANSPORT 5

typedef struct crosstalk_net_relay_stream
{
    ldg_net_sock_t sock;
    uint8_t used;
    uint8_t pudding[7];
} crosstalk_net_relay_stream_t;

typedef struct crosstalk_net_relay_ctx
{
    ldg_mut_t mut;
    crosstalk_net_relay_stream_t stream[CROSSTALK_NET_RELAY_STREAM_CAP];
    uint8_t req[CROSSTALK_NET_RELAY_PAYLOAD_CAP];
    uint8_t reply[CROSSTALK_NET_RELAY_PAYLOAD_CAP];
    uint8_t post_req[CROSSTALK_NET_RELAY_PAYLOAD_CAP];
    uint8_t body[CROSSTALK_NET_RELAY_BODY_CAP];
    uint64_t serviced;
    uint32_t body_len;
    uint32_t payload_cap;
    uint32_t reply_len;
    uint32_t post_req_len;
    uint8_t reply_held;
    uint8_t post_held;
    uint8_t mut_ok;
    uint8_t ok;
    uint8_t pudding[4];
} crosstalk_net_relay_ctx_t;

uint32_t crosstalk_net_relay_init(crosstalk_net_relay_ctx_t *ctx);
uint32_t crosstalk_net_relay_shutdown(crosstalk_net_relay_ctx_t *ctx);
uint32_t crosstalk_net_relay_tick(crosstalk_net_relay_ctx_t *ctx, const crosstalk_hdl_abi_t *abi, uint8_t blk_ok);
uint8_t crosstalk_net_relay_live_is(const crosstalk_net_relay_ctx_t *ctx);

#endif
