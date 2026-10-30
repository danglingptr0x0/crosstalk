#ifndef CROSSTALK_CORE_SHOT_H
#define CROSSTALK_CORE_SHOT_H

#include <stdint.h>

#include <dangling/term/render.h>

typedef struct crosstalk_core_shot
{
    uint8_t *rgb;
    uint8_t *bmp_raw;
    uint8_t *bmp;
    uint64_t rgb_cap;
    uint64_t bmp_cap;
    uint32_t col_max;
    uint32_t row_max;
    uint8_t pending;
    uint8_t ok;
    uint8_t pudding[6];
} crosstalk_core_shot_t;

uint32_t crosstalk_core_shot_create(crosstalk_core_shot_t *shot, uint32_t col_max, uint32_t row_max);
uint32_t crosstalk_core_shot_destroy(crosstalk_core_shot_t *shot);
uint32_t crosstalk_core_shot_capture(crosstalk_core_shot_t *shot, const ldg_term_render_t *render, const ldg_term_rect_t *rect, uint64_t win_id, uint64_t now_ms, char *out_name, uint64_t name_cap, uint64_t *out_len);

#endif
