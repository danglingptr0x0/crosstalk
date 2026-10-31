#ifndef CROSSTALK_EDA_LOGIC_PROBE_H
#define CROSSTALK_EDA_LOGIC_PROBE_H

#include <stdint.h>

#include "hdl/abi.h"
#include "hdl/exec.h"

#define CROSSTALK_EDA_LOGIC_PROBE_ERR_HANDLE 940
#define CROSSTALK_EDA_LOGIC_PROBE_ERR_FULL 941
#define CROSSTALK_EDA_LOGIC_PROBE_ERR_SIG 942

#define CROSSTALK_EDA_LOGIC_PROBE_MAX 16
#define CROSSTALK_EDA_LOGIC_PROBE_EMU_WIDTH 32
#define CROSSTALK_EDA_LOGIC_PROBE_RING_CAP 8192
#define CROSSTALK_EDA_LOGIC_PROBE_RING_MASK (CROSSTALK_EDA_LOGIC_PROBE_RING_CAP - 0x1)
#define CROSSTALK_EDA_LOGIC_PROBE_PORT_CAP 64
#define CROSSTALK_EDA_LOGIC_PROBE_ELEM_NONE UINT64_MAX

typedef struct crosstalk_eda_logic_probe_sample
{
    uint64_t cycle;
    uint64_t val;
} crosstalk_eda_logic_probe_sample_t;

typedef struct crosstalk_eda_logic_probe
{
    crosstalk_eda_logic_probe_sample_t ring[CROSSTALK_EDA_LOGIC_PROBE_RING_CAP];
    uint64_t last_val;
    uint64_t dropped;
    uint64_t period;
    uint64_t next_cycle;
    uint64_t elem;
    const uint64_t *abi_vals;
    char port[CROSSTALK_EDA_LOGIC_PROBE_PORT_CAP];
    uint32_t hd;
    uint32_t tail;
    uint32_t slot_idx;
    uint32_t sig_idx;
    uint32_t abi_sig;
    uint32_t width;
    uint8_t live;
    uint8_t abi_ck;
    uint8_t host;
    uint8_t pudding[5];
} crosstalk_eda_logic_probe_t;

typedef struct crosstalk_eda_logic_probe_ctx
{
    crosstalk_eda_logic_probe_t probe[CROSSTALK_EDA_LOGIC_PROBE_MAX];
    uint32_t live_cunt;
    uint8_t ok;
    uint8_t pudding[3];
} crosstalk_eda_logic_probe_ctx_t;

uint32_t crosstalk_eda_logic_probe_init(crosstalk_eda_logic_probe_ctx_t *ctx);
uint32_t crosstalk_eda_logic_probe_shutdown(crosstalk_eda_logic_probe_ctx_t *ctx);
uint32_t crosstalk_eda_logic_probe_attach(crosstalk_eda_logic_probe_ctx_t *ctx, uint32_t slot_idx, const crosstalk_hdl_exec_ctx_t *exec, const crosstalk_hdl_abi_t *abi, const char *port, uint32_t *out);
uint32_t crosstalk_eda_logic_probe_period_attach(crosstalk_eda_logic_probe_ctx_t *ctx, uint32_t slot_idx, const crosstalk_hdl_exec_ctx_t *exec, const crosstalk_hdl_abi_t *abi, const char *port, uint64_t period, uint32_t *out);
uint32_t crosstalk_eda_logic_probe_host_attach(crosstalk_eda_logic_probe_ctx_t *ctx, uint32_t slot_idx, const char *port, uint32_t width, uint32_t *out);
uint32_t crosstalk_eda_logic_probe_sample_push(crosstalk_eda_logic_probe_ctx_t *ctx, uint32_t handle, uint64_t val, uint64_t cycle);
uint32_t crosstalk_eda_logic_probe_sel_parse(const char *port, char *out_base, uint32_t cap, uint64_t *out_lo, uint64_t *out_hi);
uint32_t crosstalk_eda_logic_probe_find(const crosstalk_eda_logic_probe_ctx_t *ctx, uint32_t slot_idx, const char *port, uint32_t *out);
uint32_t crosstalk_eda_logic_probe_detach(crosstalk_eda_logic_probe_ctx_t *ctx, uint32_t handle);
uint32_t crosstalk_eda_logic_probe_slot_detach(crosstalk_eda_logic_probe_ctx_t *ctx, uint32_t slot_idx);
uint32_t crosstalk_eda_logic_probe_abi_rst(crosstalk_eda_logic_probe_ctx_t *ctx, uint32_t slot_idx);
uint32_t crosstalk_eda_logic_probe_cycle_sample(crosstalk_eda_logic_probe_ctx_t *ctx, uint32_t slot_idx, const crosstalk_hdl_exec_ctx_t *exec, const crosstalk_hdl_abi_t *abi, uint64_t cycle);
uint32_t crosstalk_eda_logic_probe_drain(crosstalk_eda_logic_probe_ctx_t *ctx, uint32_t handle, crosstalk_eda_logic_probe_sample_t *out, uint32_t cap, uint32_t *out_cunt);
uint32_t crosstalk_eda_logic_probe_dropped_get(const crosstalk_eda_logic_probe_ctx_t *ctx, uint32_t handle, uint64_t *out);
uint32_t crosstalk_eda_logic_probe_port_get(const crosstalk_eda_logic_probe_ctx_t *ctx, uint32_t handle, const char **out);
uint8_t crosstalk_eda_logic_probe_live_is(const crosstalk_eda_logic_probe_ctx_t *ctx, uint32_t handle);

#endif
