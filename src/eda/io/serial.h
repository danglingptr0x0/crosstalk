#ifndef CROSSTALK_EDA_IO_SERIAL_H
#define CROSSTALK_EDA_IO_SERIAL_H

#include <stdint.h>

#include <dangling/core/types.h>
#include <dangling/io/serial.h>

#include "eda/ic/fpga/fpga.h"

#define CROSSTALK_EDA_IO_SERIAL_ERR_ATTACHED 920
#define CROSSTALK_EDA_IO_SERIAL_ERR_NOT_ATTACHED 921
#define CROSSTALK_EDA_IO_SERIAL_ERR_PATH 922
#define CROSSTALK_EDA_IO_SERIAL_ERR_TX_FULL 923
#define CROSSTALK_EDA_IO_SERIAL_ERR_BUSTED 924
#define CROSSTALK_EDA_IO_SERIAL_ERR_FWD_SIG 925

#define CROSSTALK_EDA_IO_SERIAL_RX_CAP 4096
#define CROSSTALK_EDA_IO_SERIAL_TX_CAP 1024
#define CROSSTALK_EDA_IO_SERIAL_EVENT_MAX 16
#define CROSSTALK_EDA_IO_SERIAL_POLL_PERIOD LDG_MS_PER_SEC
#define CROSSTALK_EDA_IO_SERIAL_SEP '\\'
#define CROSSTALK_EDA_IO_SERIAL_REMOTE_PATH "session"
#define CROSSTALK_EDA_IO_SERIAL_FWD_RX_DATA_DEFAULT "serial_in"
#define CROSSTALK_EDA_IO_SERIAL_FWD_RX_RDY_DEFAULT "serial_rdy"
#define CROSSTALK_EDA_IO_SERIAL_FWD_RX_ACK_DEFAULT "serial_ack"
#define CROSSTALK_EDA_IO_SERIAL_FWD_TX_DATA_DEFAULT "serial_tx"
#define CROSSTALK_EDA_IO_SERIAL_FWD_TX_RDY_DEFAULT "serial_tx_rdy"
#define CROSSTALK_EDA_IO_SERIAL_FWD_NAME_CAP 64
#define CROSSTALK_EDA_IO_SERIAL_FWD_RING_CAP 256
#define CROSSTALK_EDA_IO_SERIAL_FWD_RING_MASK (CROSSTALK_EDA_IO_SERIAL_FWD_RING_CAP - 1U)

typedef enum crosstalk_eda_io_serial_event_kind
{
    CROSSTALK_EDA_IO_SERIAL_EVENT_NONE = 0,
    CROSSTALK_EDA_IO_SERIAL_EVENT_ARRIVE,
    CROSSTALK_EDA_IO_SERIAL_EVENT_DEPART
} crosstalk_eda_io_serial_event_kind_t;

typedef uint32_t (*crosstalk_eda_io_serial_sink_cb_t)(void *user, const uint8_t *bytes, uint32_t len);

typedef struct crosstalk_eda_io_serial_event
{
    char path[LDG_IO_SERIAL_PATH_MAX];
    char id[LDG_IO_SERIAL_ID_MAX];
    uint32_t kind;
    uint8_t pudding[4];
} crosstalk_eda_io_serial_event_t;

typedef struct crosstalk_eda_io_serial_ctx
{
    ldg_io_serial_port_t port[LDG_IO_SERIAL_PORT_MAX];
    ldg_io_serial_port_t prev[LDG_IO_SERIAL_PORT_MAX];
    crosstalk_eda_io_serial_event_t event[CROSSTALK_EDA_IO_SERIAL_EVENT_MAX];
    ldg_io_serial_t link;
    ldg_io_serial_conf_t conf;
    uint64_t poll_acc;
    uint64_t port_cunt;
    uint64_t prev_cunt;
    uint64_t rx_total;
    uint64_t tx_total;
    uint64_t tx_dropped;
    uint64_t fwd_tx_total;
    uint64_t fwd_tx_dropped;
    uint64_t fwd_tx_dropped_seen;
    uint32_t event_cunt;
    uint32_t tx_len;
    uint32_t rx_len;
    uint32_t fwd_gen;
    uint32_t fwd_slot;
    uint32_t fwd_ring_wr;
    uint32_t fwd_ring_rd;
    uint32_t fwd_data_sig;
    uint32_t fwd_rdy_sig;
    uint32_t fwd_data_abi;
    uint32_t fwd_rdy_abi;
    ldg_byte_t rx[CROSSTALK_EDA_IO_SERIAL_RX_CAP];
    ldg_byte_t tx[CROSSTALK_EDA_IO_SERIAL_TX_CAP];
    char open_path[LDG_IO_SERIAL_PATH_MAX];
    char open_id[LDG_IO_SERIAL_ID_MAX];
    char fwd_rx_data[CROSSTALK_EDA_IO_SERIAL_FWD_NAME_CAP];
    char fwd_rx_rdy[CROSSTALK_EDA_IO_SERIAL_FWD_NAME_CAP];
    char fwd_rx_ack[CROSSTALK_EDA_IO_SERIAL_FWD_NAME_CAP];
    char fwd_tx_data[CROSSTALK_EDA_IO_SERIAL_FWD_NAME_CAP];
    char fwd_tx_rdy[CROSSTALK_EDA_IO_SERIAL_FWD_NAME_CAP];
    uint8_t fwd_ring[CROSSTALK_EDA_IO_SERIAL_FWD_RING_CAP];
    uint8_t attached;
    uint8_t seeded;
    uint8_t busted;
    uint8_t ok;
    uint8_t remote;
    uint8_t fwd_on;
    uint8_t fwd_ck;
    uint8_t fwd_live;
    uint8_t fwd_abi_ck;
    uint8_t pudding[3];
} crosstalk_eda_io_serial_ctx_t;

uint32_t crosstalk_eda_io_serial_init(crosstalk_eda_io_serial_ctx_t *ctx);
uint32_t crosstalk_eda_io_serial_shutdown(crosstalk_eda_io_serial_ctx_t *ctx);
uint32_t crosstalk_eda_io_serial_poll(crosstalk_eda_io_serial_ctx_t *ctx, uint64_t dt);
uint32_t crosstalk_eda_io_serial_events_drain(crosstalk_eda_io_serial_ctx_t *ctx, crosstalk_eda_io_serial_event_t *out, uint32_t cap, uint32_t *out_cunt);
uint32_t crosstalk_eda_io_serial_ports_get(const crosstalk_eda_io_serial_ctx_t *ctx, const ldg_io_serial_port_t **out, uint64_t *out_cunt);
uint32_t crosstalk_eda_io_serial_attach(crosstalk_eda_io_serial_ctx_t *ctx, const char *path);
uint32_t crosstalk_eda_io_serial_detach(crosstalk_eda_io_serial_ctx_t *ctx);
uint32_t crosstalk_eda_io_serial_pump(crosstalk_eda_io_serial_ctx_t *ctx, crosstalk_eda_io_serial_sink_cb_t cb, void *user);
uint32_t crosstalk_eda_io_serial_tx_put(crosstalk_eda_io_serial_ctx_t *ctx, const uint8_t *bytes, uint32_t len);
uint32_t crosstalk_eda_io_serial_sink_cb(void *user, const uint8_t *bytes, uint32_t len);
uint32_t crosstalk_eda_io_serial_conf_apply(crosstalk_eda_io_serial_ctx_t *ctx, const ldg_io_serial_conf_t *conf);
uint32_t crosstalk_eda_io_serial_id_tail_get(const char *id, const char **out);
uint32_t crosstalk_eda_io_serial_remote_attach(crosstalk_eda_io_serial_ctx_t *ctx, const char *id);
uint32_t crosstalk_eda_io_serial_remote_rx_put(crosstalk_eda_io_serial_ctx_t *ctx, const uint8_t *bytes, uint32_t len);
uint32_t crosstalk_eda_io_serial_remote_tx_take(crosstalk_eda_io_serial_ctx_t *ctx, uint8_t *out, uint32_t cap, uint32_t *out_len);
uint32_t crosstalk_eda_io_serial_fwd_set(crosstalk_eda_io_serial_ctx_t *ctx, const char *rx_data, const char *rx_rdy, const char *rx_ack, const char *tx_data, const char *tx_rdy);
uint32_t crosstalk_eda_io_serial_fwd_clear(crosstalk_eda_io_serial_ctx_t *ctx);
uint32_t crosstalk_eda_io_serial_fwd_tick(crosstalk_eda_io_serial_ctx_t *ctx, crosstalk_eda_ic_fpga_ctx_t *fpga, crosstalk_eda_io_receptacle_ctx_t *receptacle, uint32_t slot_idx);
uint32_t crosstalk_eda_io_serial_fwd_cycle_cb(uint32_t slot_idx, const crosstalk_hdl_exec_ctx_t *exec, const crosstalk_hdl_abi_t *abi, void *user);
uint8_t crosstalk_eda_io_serial_fwd_on_is(const crosstalk_eda_io_serial_ctx_t *ctx);
uint8_t crosstalk_eda_io_serial_attached_is(const crosstalk_eda_io_serial_ctx_t *ctx);
uint8_t crosstalk_eda_io_serial_busted_is(const crosstalk_eda_io_serial_ctx_t *ctx);
uint8_t crosstalk_eda_io_serial_remote_is(const crosstalk_eda_io_serial_ctx_t *ctx);

#endif
