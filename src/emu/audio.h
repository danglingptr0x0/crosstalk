#ifndef CROSSTALK_EMU_AUDIO_H
#define CROSSTALK_EMU_AUDIO_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/core/types.h>

#include "hdl/abi.h"

#define CROSSTALK_EMU_AUDIO_ERR_OPEN 1028
#define CROSSTALK_EMU_AUDIO_ERR_RATE 1029

#define CROSSTALK_EMU_AUDIO_RING_CAP 16384
#define CROSSTALK_EMU_AUDIO_RING_MASK (CROSSTALK_EMU_AUDIO_RING_CAP - 1)
#define CROSSTALK_EMU_AUDIO_RATE_MIN 1
#define CROSSTALK_EMU_AUDIO_RATE_MAX 96000
#define CROSSTALK_EMU_AUDIO_CHANNEL_CUNT 1
#define CROSSTALK_EMU_AUDIO_BIT_DEPTH 16
#define CROSSTALK_EMU_AUDIO_SIGN_FLIP 0x80
#define CROSSTALK_EMU_AUDIO_SAMPLE_SHIFT 8
#define CROSSTALK_EMU_AUDIO_SPAN_MAX 65536
#define CROSSTALK_EMU_AUDIO_DRAIN_CAP 1024
#define CROSSTALK_EMU_AUDIO_DEPTH_MAX 1200
#define CROSSTALK_EMU_AUDIO_WORD_BYTES 8

typedef struct crosstalk_emu_audio_ctx
{
    uint16_t ring[CROSSTALK_EMU_AUDIO_RING_CAP];
    uint64_t curr;
    uint64_t produced;
    uint64_t pulled;
    uint64_t dropped;
    uint64_t underrun;
    uint32_t rate;
    uint32_t hd;
    uint32_t tl;
    uint8_t streaming;
    uint8_t live;
    uint8_t pudding[2];
} crosstalk_emu_audio_ctx_t;

uint32_t crosstalk_emu_audio_init(crosstalk_emu_audio_ctx_t *ctx);
uint32_t crosstalk_emu_audio_shutdown(crosstalk_emu_audio_ctx_t *ctx);
uint32_t crosstalk_emu_audio_stop(crosstalk_emu_audio_ctx_t *ctx);
uint32_t crosstalk_emu_audio_tick(crosstalk_emu_audio_ctx_t *ctx, const crosstalk_hdl_abi_t *abi);
uint32_t crosstalk_emu_audio_frames_pull(void *user, void *buff, uint32_t frame_cunt, uint32_t *out);
uint8_t crosstalk_emu_audio_live_is(const crosstalk_emu_audio_ctx_t *ctx);

#endif
