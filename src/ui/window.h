#ifndef CROSSTALK_UI_WINDOW_H
#define CROSSTALK_UI_WINDOW_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/term/focus.h>
#include <dangling/term/layout.h>
#include <dangling/term/render.h>
#include <dangling/term/vt.h>

#include "core/task.h"

#define CROSSTALK_UI_WINDOW_ERR_FULL 950
#define CROSSTALK_UI_WINDOW_ERR_SLOT 951

#define CROSSTALK_UI_WINDOW_SLOT_CAP 40
#define CROSSTALK_UI_WINDOW_SLOT_FIRST 1
#define CROSSTALK_UI_WINDOW_DIGIT_HOLD_MS 150
#define CROSSTALK_UI_WINDOW_DIGIT_BASE 10
#define CROSSTALK_UI_WINDOW_DIGIT_NONE 0
#define CROSSTALK_UI_WINDOW_RECENT_BACK 1
#define CROSSTALK_UI_WINDOW_NAME_CAP 64
#define CROSSTALK_UI_WINDOW_TITLE_CAP 96
#define CROSSTALK_UI_WINDOW_TITLE_FMT "[%u] %.63s"
#define CROSSTALK_UI_WINDOW_FRAME_CELLS 1
#define CROSSTALK_UI_WINDOW_NODE_CAP (CROSSTALK_UI_WINDOW_SLOT_CAP + 1)

typedef enum crosstalk_ui_window_dir
{
    CROSSTALK_UI_WINDOW_DIR_LHS = 0,
    CROSSTALK_UI_WINDOW_DIR_RHS,
    CROSSTALK_UI_WINDOW_DIR_UP,
    CROSSTALK_UI_WINDOW_DIR_DOWN
} crosstalk_ui_window_dir_t;

typedef struct crosstalk_ui_window_state
{
    double rect_x;
    double rect_y;
    double rect_w;
    double rect_h;
    uint32_t scroll_row;
    uint32_t content_row_cunt;
    uint32_t view_row_cunt;
    uint8_t open;
    uint8_t highlighted;
    uint8_t scroll_on;
    uint8_t pudding[1];
} crosstalk_ui_window_state_t;

typedef uint32_t (*crosstalk_ui_window_draw_cb_t)(uint64_t win_id, void *user, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt);
typedef uint32_t (*crosstalk_ui_window_key_cb_t)(uint64_t win_id, void *user, const ldg_term_key_event_t *key, uint8_t *consumed);

typedef struct crosstalk_ui_window_slot
{
    char title[CROSSTALK_UI_WINDOW_NAME_CAP];
    crosstalk_ui_window_state_t *state;
    crosstalk_ui_window_draw_cb_t draw_cb;
    crosstalk_ui_window_key_cb_t key_cb;
    void *user;
    uint64_t id;
} crosstalk_ui_window_slot_t;

typedef struct crosstalk_ui_window_ctx
{
    crosstalk_ui_window_slot_t slot[CROSSTALK_UI_WINDOW_SLOT_CAP];
    ldg_term_focus_t *focus;
    crosstalk_core_task_ctx_t *task;
    uint64_t focus_id;
    uint64_t digit_deadline_ms;
    uint64_t top_id;
    uint64_t flash_id;
    uint64_t flash_from_ms;
    uint64_t flash_until_ms;
    uint32_t slot_cunt;
    uint32_t digit_val;
    uint8_t digit_on;
    uint8_t kbd_pending;
    uint8_t focus_kbd;
    uint8_t focus_changed;
    uint8_t ok;
    uint8_t pudding[3];
} crosstalk_ui_window_ctx_t;

uint32_t crosstalk_ui_window_init(crosstalk_ui_window_ctx_t *ctx, crosstalk_core_task_ctx_t *task);
uint32_t crosstalk_ui_window_shutdown(crosstalk_ui_window_ctx_t *ctx);
uint32_t crosstalk_ui_window_reg(crosstalk_ui_window_ctx_t *ctx, uint64_t win_id, const char *title, crosstalk_ui_window_state_t *state, crosstalk_ui_window_draw_cb_t draw_cb, crosstalk_ui_window_key_cb_t key_cb, void *user, uint32_t *out_slot);
uint32_t crosstalk_ui_window_slot_get(const crosstalk_ui_window_ctx_t *ctx, uint64_t win_id, uint32_t *out_slot);
uint32_t crosstalk_ui_window_id_get(const crosstalk_ui_window_ctx_t *ctx, uint32_t slot, uint64_t *out_id);
uint32_t crosstalk_ui_window_title_set(crosstalk_ui_window_ctx_t *ctx, uint64_t win_id, const char *title);
uint32_t crosstalk_ui_window_all_close(crosstalk_ui_window_ctx_t *ctx);
uint32_t crosstalk_ui_window_focus_set(crosstalk_ui_window_ctx_t *ctx, uint64_t win_id, uint64_t now_ms);
uint8_t crosstalk_ui_window_focused_is(const crosstalk_ui_window_ctx_t *ctx, uint64_t win_id);
uint32_t crosstalk_ui_window_focus_slot(crosstalk_ui_window_ctx_t *ctx, uint32_t slot, uint64_t now_ms);
uint32_t crosstalk_ui_window_recent_focus(crosstalk_ui_window_ctx_t *ctx, uint64_t now_ms);
uint32_t crosstalk_ui_window_focus_dir(crosstalk_ui_window_ctx_t *ctx, uint32_t dir, uint64_t now_ms);
uint32_t crosstalk_ui_window_digit_put(crosstalk_ui_window_ctx_t *ctx, uint32_t digit, uint64_t now_ms);
uint32_t crosstalk_ui_window_digit_step(crosstalk_ui_window_ctx_t *ctx, uint64_t now_ms);
uint32_t crosstalk_ui_window_key_route(crosstalk_ui_window_ctx_t *ctx, const ldg_term_key_event_t *key, uint8_t *consumed);
uint32_t crosstalk_ui_window_row_show(crosstalk_ui_window_state_t *state, uint32_t row);
uint32_t crosstalk_ui_window_rect_get(const crosstalk_ui_window_state_t *state, const ldg_term_rect_t *desk, ldg_term_rect_t *out);
uint32_t crosstalk_ui_window_focus_rect_get(const crosstalk_ui_window_ctx_t *ctx, const ldg_term_rect_t *desk, ldg_term_rect_t *out_rect, uint64_t *out_id);
uint32_t crosstalk_ui_window_top_set(crosstalk_ui_window_ctx_t *ctx, uint64_t win_id);
uint32_t crosstalk_ui_window_flash_set(crosstalk_ui_window_ctx_t *ctx, uint64_t win_id, uint64_t until_ms);
uint32_t crosstalk_ui_window_all_draw(crosstalk_ui_window_ctx_t *ctx, ldg_term_render_t *render, const ldg_term_rect_t *desk);

#endif
