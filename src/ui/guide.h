#ifndef CROSSTALK_UI_GUIDE_H
#define CROSSTALK_UI_GUIDE_H

#include <stdint.h>

#include "ui/window.h"

#define CROSSTALK_UI_GUIDE_WINDOW_ID 0x47554944ULL
#define CROSSTALK_UI_GUIDE_WINDOW_TITLE "Guide"
#define CROSSTALK_UI_GUIDE_REG_CAP 16
#define CROSSTALK_UI_GUIDE_STEP_CAP 32
#define CROSSTALK_UI_GUIDE_NAME_CAP 32
#define CROSSTALK_UI_GUIDE_LINE_CAP 256
#define CROSSTALK_UI_GUIDE_WRAP_CAP 32

typedef uint32_t (*crosstalk_ui_guide_tgt_open_cb_t)(void *user, uint64_t win_id);
typedef uint8_t (*crosstalk_ui_guide_done_cb_t)(void *user, uint64_t done_id);

typedef struct crosstalk_ui_guide_step
{
    const char *title;
    const char *body;
    const char *img_rel;
    const char *action;
    uint64_t tgt_id;
    uint64_t done_id;
    uint32_t pick_cunt;
    uint8_t pudding[4];
} crosstalk_ui_guide_step_t;

typedef struct crosstalk_ui_guide
{
    const char *name;
    const crosstalk_ui_guide_step_t *step;
    uint32_t step_cunt;
    uint8_t pudding[4];
} crosstalk_ui_guide_t;

typedef struct crosstalk_ui_guide_ctx
{
    crosstalk_ui_window_state_t win;
    const crosstalk_ui_guide_t *reg[CROSSTALK_UI_GUIDE_REG_CAP];
    const crosstalk_ui_guide_t *live;
    crosstalk_ui_window_ctx_t *nav;
    crosstalk_ui_guide_tgt_open_cb_t tgt_open_cb;
    crosstalk_ui_guide_done_cb_t done_cb;
    void *tgt_user;
    uint32_t reg_cunt;
    uint32_t step_idx;
    uint32_t done_mask;
    uint32_t pick;
    uint8_t ok;
    uint8_t pudding[3];
} crosstalk_ui_guide_ctx_t;

uint32_t crosstalk_ui_guide_init(crosstalk_ui_guide_ctx_t *ctx);
uint32_t crosstalk_ui_guide_shutdown(crosstalk_ui_guide_ctx_t *ctx);
uint32_t crosstalk_ui_guide_bind(crosstalk_ui_guide_ctx_t *ctx, crosstalk_ui_window_ctx_t *nav, crosstalk_ui_guide_tgt_open_cb_t tgt_open_cb, crosstalk_ui_guide_done_cb_t done_cb, void *tgt_user);
uint32_t crosstalk_ui_guide_win_reg(crosstalk_ui_guide_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot);
uint32_t crosstalk_ui_guide_reg(crosstalk_ui_guide_ctx_t *ctx, const crosstalk_ui_guide_t *guide);
uint32_t crosstalk_ui_guide_extent_get(const crosstalk_ui_guide_ctx_t *ctx, uint32_t col_cunt, uint32_t *out_row_cunt);
uint32_t crosstalk_ui_guide_name_get(const crosstalk_ui_guide_ctx_t *ctx, uint32_t idx, const char **out);
uint32_t crosstalk_ui_guide_start(crosstalk_ui_guide_ctx_t *ctx, const char *name);
uint32_t crosstalk_ui_guide_at_start(crosstalk_ui_guide_ctx_t *ctx, uint32_t idx);
uint32_t crosstalk_ui_guide_step_set(crosstalk_ui_guide_ctx_t *ctx, uint32_t step_idx);
uint32_t crosstalk_ui_guide_next(crosstalk_ui_guide_ctx_t *ctx);
uint32_t crosstalk_ui_guide_back(crosstalk_ui_guide_ctx_t *ctx);
uint32_t crosstalk_ui_guide_show(crosstalk_ui_guide_ctx_t *ctx);
uint32_t crosstalk_ui_guide_close(crosstalk_ui_guide_ctx_t *ctx);
uint8_t crosstalk_ui_guide_live_is(const crosstalk_ui_guide_ctx_t *ctx);
uint32_t crosstalk_ui_guide_tgt_get(const crosstalk_ui_guide_ctx_t *ctx, uint64_t *out);
uint32_t crosstalk_ui_guide_pick_get(crosstalk_ui_guide_ctx_t *ctx, uint32_t *out);

#endif
