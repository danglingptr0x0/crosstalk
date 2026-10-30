#ifndef CROSSTALK_EDA_LOGIC_STREAM_H
#define CROSSTALK_EDA_LOGIC_STREAM_H

#include <stdint.h>

#include "eda/logic/probe.h"

#define CROSSTALK_EDA_LOGIC_STREAM_ERR_HANDLE 943
#define CROSSTALK_EDA_LOGIC_STREAM_ERR_FULL 944
#define CROSSTALK_EDA_LOGIC_STREAM_ERR_SINK_FULL 945

#define CROSSTALK_EDA_LOGIC_STREAM_MAX 4
#define CROSSTALK_EDA_LOGIC_STREAM_SINK_MAX 4
#define CROSSTALK_EDA_LOGIC_STREAM_DRAIN_CAP 256
#define CROSSTALK_EDA_LOGIC_STREAM_BYTE_MASK 0xFFU
#define CROSSTALK_EDA_LOGIC_STREAM_SAMPLE_BYTES 2
#define CROSSTALK_EDA_LOGIC_STREAM_SAMPLE_BITS 16
#define CROSSTALK_EDA_LOGIC_STREAM_LSB 1
#define CROSSTALK_EDA_LOGIC_STREAM_BYTE_CAP (CROSSTALK_EDA_LOGIC_STREAM_DRAIN_CAP * CROSSTALK_EDA_LOGIC_STREAM_SAMPLE_BYTES)

typedef enum crosstalk_eda_logic_stream_decode
{
    CROSSTALK_EDA_LOGIC_STREAM_DECODE_BYTE_PORT = 0,
    CROSSTALK_EDA_LOGIC_STREAM_DECODE_LEVEL,
    CROSSTALK_EDA_LOGIC_STREAM_DECODE_SAMPLE
} crosstalk_eda_logic_stream_decode_t;

typedef uint32_t (*crosstalk_eda_logic_stream_sink_cb_t)(void *user, const uint8_t *bytes, uint32_t len);

typedef struct crosstalk_eda_logic_stream_sink
{
    crosstalk_eda_logic_stream_sink_cb_t cb;
    void *user;
} crosstalk_eda_logic_stream_sink_t;

typedef struct crosstalk_eda_logic_stream
{
    crosstalk_eda_logic_stream_sink_t sink[CROSSTALK_EDA_LOGIC_STREAM_SINK_MAX];
    uint64_t bytes_total;
    uint64_t level_val;
    uint64_t level_cycle;
    uint32_t probe_handle;
    uint32_t decode;
    uint32_t sink_cunt;
    uint32_t width;
    uint8_t live;
    uint8_t pudding[3];
} crosstalk_eda_logic_stream_t;

typedef struct crosstalk_eda_logic_stream_ctx
{
    crosstalk_eda_logic_stream_t stream[CROSSTALK_EDA_LOGIC_STREAM_MAX];
    crosstalk_eda_logic_probe_sample_t drain[CROSSTALK_EDA_LOGIC_STREAM_DRAIN_CAP];
    uint8_t byte[CROSSTALK_EDA_LOGIC_STREAM_BYTE_CAP];
    uint8_t ok;
    uint8_t pudding[7];
} crosstalk_eda_logic_stream_ctx_t;

uint32_t crosstalk_eda_logic_stream_init(crosstalk_eda_logic_stream_ctx_t *ctx);
uint32_t crosstalk_eda_logic_stream_shutdown(crosstalk_eda_logic_stream_ctx_t *ctx);
uint32_t crosstalk_eda_logic_stream_open(crosstalk_eda_logic_stream_ctx_t *ctx, uint32_t probe_handle, crosstalk_eda_logic_stream_decode_t decode, uint32_t *out);
uint32_t crosstalk_eda_logic_stream_close(crosstalk_eda_logic_stream_ctx_t *ctx, uint32_t handle);
uint32_t crosstalk_eda_logic_stream_sink_add(crosstalk_eda_logic_stream_ctx_t *ctx, uint32_t handle, crosstalk_eda_logic_stream_sink_cb_t cb, void *user);
uint32_t crosstalk_eda_logic_stream_width_set(crosstalk_eda_logic_stream_ctx_t *ctx, uint32_t handle, uint32_t width);
uint32_t crosstalk_eda_logic_stream_pump(crosstalk_eda_logic_stream_ctx_t *ctx, crosstalk_eda_logic_probe_ctx_t *probes);
uint32_t crosstalk_eda_logic_stream_level_get(const crosstalk_eda_logic_stream_ctx_t *ctx, uint32_t handle, uint64_t *out_val, uint64_t *out_cycle);
uint32_t crosstalk_eda_logic_stream_bytes_total_get(const crosstalk_eda_logic_stream_ctx_t *ctx, uint32_t handle, uint64_t *out);
uint8_t crosstalk_eda_logic_stream_live_is(const crosstalk_eda_logic_stream_ctx_t *ctx, uint32_t handle);

#endif
