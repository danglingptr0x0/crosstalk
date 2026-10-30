#ifndef CROSSTALK_CORE_CLIP_H
#define CROSSTALK_CORE_CLIP_H

#include <stdint.h>

#include <dangling/core/macros.h>

#define CROSSTALK_CORE_CLIP_CAP 256

typedef struct crosstalk_core_clip_ctx
{
    char text[CROSSTALK_CORE_CLIP_CAP];
    uint64_t gen;
    uint8_t has;
    uint8_t pudding[7];
} crosstalk_core_clip_ctx_t;

uint32_t crosstalk_core_clip_init(crosstalk_core_clip_ctx_t *ctx);
uint32_t crosstalk_core_clip_set(crosstalk_core_clip_ctx_t *ctx, const char *text);
uint32_t crosstalk_core_clip_get(const crosstalk_core_clip_ctx_t *ctx, char *out, uint32_t cap);
uint32_t crosstalk_core_clip_clear(crosstalk_core_clip_ctx_t *ctx);
uint8_t crosstalk_core_clip_has(const crosstalk_core_clip_ctx_t *ctx);

#endif
