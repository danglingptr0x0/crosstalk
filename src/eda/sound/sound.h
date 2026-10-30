#ifndef CROSSTALK_EDA_SOUND_H
#define CROSSTALK_EDA_SOUND_H

#include <stdint.h>

#include <dangling/core/types.h>

#define CROSSTALK_EDA_SOUND_ERR_RATE 910
#define CROSSTALK_EDA_SOUND_ERR_NOT_LIVE 911
#define CROSSTALK_EDA_SOUND_ERR_STREAMING 912

#define CROSSTALK_EDA_SOUND_RING_CAP 16384
#define CROSSTALK_EDA_SOUND_RATE_DEFAULT 8000
#define CROSSTALK_EDA_SOUND_RATE_MIN 1
#define CROSSTALK_EDA_SOUND_RATE_MAX 96000
#define CROSSTALK_EDA_SOUND_CHANNEL_CUNT 1
#define CROSSTALK_EDA_SOUND_BIT_DEPTH 16
#define CROSSTALK_EDA_SOUND_SAMPLE_BYTES 2
#define CROSSTALK_EDA_SOUND_PORT_CAP 64
#define CROSSTALK_EDA_SOUND_REPORT_SECS 30.0
#define CROSSTALK_EDA_SOUND_STEP 1
#define CROSSTALK_EDA_SOUND_SILENCE 0
#define CROSSTALK_EDA_SOUND_RING_MASK (CROSSTALK_EDA_SOUND_RING_CAP - CROSSTALK_EDA_SOUND_STEP)
#define CROSSTALK_EDA_SOUND_PCM_CAP (CROSSTALK_EDA_SOUND_RING_CAP * CROSSTALK_EDA_SOUND_SAMPLE_BYTES)
#define CROSSTALK_EDA_SOUND_BLOB_SLACK 64
#define CROSSTALK_EDA_SOUND_BLOB_CAP (CROSSTALK_EDA_SOUND_PCM_CAP + CROSSTALK_EDA_SOUND_BLOB_SLACK)

typedef struct crosstalk_eda_sound_ctx
{
    uint16_t ring[CROSSTALK_EDA_SOUND_RING_CAP];
    ldg_byte_t pcm[CROSSTALK_EDA_SOUND_PCM_CAP];
    ldg_byte_t blob[CROSSTALK_EDA_SOUND_BLOB_CAP];
    char port[CROSSTALK_EDA_SOUND_PORT_CAP];
    uint64_t produced;
    uint64_t pulled;
    uint64_t dropped;
    uint64_t underrun;
    uint64_t report_mark;
    uint64_t made_prev;
    uint64_t dropped_prev;
    uint64_t underrun_prev;
    double report_acc;
    uint32_t rate;
    uint32_t hd;
    uint32_t tl;
    uint32_t pending;
    uint8_t pending_len;
    uint8_t live;
    uint8_t ok;
    uint8_t streaming;
    uint8_t pudding[4];
} crosstalk_eda_sound_ctx_t;

uint32_t crosstalk_eda_sound_init(crosstalk_eda_sound_ctx_t *ctx);
uint32_t crosstalk_eda_sound_shutdown(crosstalk_eda_sound_ctx_t *ctx);
uint32_t crosstalk_eda_sound_rate_set(crosstalk_eda_sound_ctx_t *ctx, uint32_t rate);
uint32_t crosstalk_eda_sound_bind(crosstalk_eda_sound_ctx_t *ctx, const char *port);
uint32_t crosstalk_eda_sound_unbind(crosstalk_eda_sound_ctx_t *ctx);
uint32_t crosstalk_eda_sound_sink_cb(void *user, const uint8_t *bytes, uint32_t len);
uint32_t crosstalk_eda_sound_frames_pull(void *user, void *buff, uint32_t frame_cunt, uint32_t *out);
uint32_t crosstalk_eda_sound_flush(crosstalk_eda_sound_ctx_t *ctx);
uint32_t crosstalk_eda_sound_depth_get(const crosstalk_eda_sound_ctx_t *ctx, uint32_t *out);
uint32_t crosstalk_eda_sound_stream_start(crosstalk_eda_sound_ctx_t *ctx);
uint32_t crosstalk_eda_sound_stream_stop(crosstalk_eda_sound_ctx_t *ctx);
uint8_t crosstalk_eda_sound_streaming_is(const crosstalk_eda_sound_ctx_t *ctx);
uint32_t crosstalk_eda_sound_fmt_get(const crosstalk_eda_sound_ctx_t *ctx, uint32_t *out_rate, uint16_t *out_channel_cunt, uint16_t *out_bit_depth);
uint32_t crosstalk_eda_sound_bake(crosstalk_eda_sound_ctx_t *ctx, const ldg_byte_t **out_blob, uint64_t *out_size);
uint32_t crosstalk_eda_sound_tick(crosstalk_eda_sound_ctx_t *ctx, double dt_secs);
uint8_t crosstalk_eda_sound_live_is(const crosstalk_eda_sound_ctx_t *ctx);

#endif
