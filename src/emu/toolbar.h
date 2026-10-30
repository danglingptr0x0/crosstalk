#ifndef CROSSTALK_EMU_TOOLBAR_H
#define CROSSTALK_EMU_TOOLBAR_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/term/render.h>
#include <dangling/term/vt.h>

#include "emu/emu.h"

#define CROSSTALK_EMU_TOOLBAR_ROW_CUNT 1
#define CROSSTALK_EMU_TOOLBAR_HZ_TEXT_CAP 24

typedef struct crosstalk_emu_toolbar_ctx
{
    char hz_text[CROSSTALK_EMU_TOOLBAR_HZ_TEXT_CAP];
    double hz_mhz;
    uint8_t open;
    uint8_t close_req;
    uint8_t pudding[6];
} crosstalk_emu_toolbar_ctx_t;

uint32_t crosstalk_emu_toolbar_init(crosstalk_emu_toolbar_ctx_t *ctx, const crosstalk_emu_ctx_t *emu);
uint32_t crosstalk_emu_toolbar_key_isr(crosstalk_emu_toolbar_ctx_t *ctx, crosstalk_emu_ctx_t *emu, const ldg_term_key_event_t *key, uint8_t *out_consumed);
uint32_t crosstalk_emu_toolbar_draw(const crosstalk_emu_toolbar_ctx_t *ctx, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt);
uint8_t crosstalk_emu_toolbar_close_req_is(const crosstalk_emu_toolbar_ctx_t *ctx);

#endif
