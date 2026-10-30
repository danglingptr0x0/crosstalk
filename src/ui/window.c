#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/logging/logging.h>
#include <dangling/time/time.h>

#include "core/log.h"
#include "ui/cell.h"
#include "ui/theme.h"
#include "ui/window.h"

#define WINDOW_STEP 1
#define WINDOW_MID 2.0
#define WINDOW_DIST_NONE (-1.0)
#define WINDOW_FRAC_MIN 0.0
#define WINDOW_FRAC_MAX 1.0
#define WINDOW_FRAME_SIDES 2
#define WINDOW_EXTENT_NONE 0
#define WINDOW_SCROLL_COL_NONE 0
#define WINDOW_SCROLL_TOP 0
#define WINDOW_NAME_FMT "%.63s"
#define WINDOW_FRAME_BG_LEVEL 0x00
#define WINDOW_FLASH_PERIOD_MS 150
#define WINDOW_FLASH_PHASE_CUNT 2
#define WINDOW_FLASH_PHASE_LIT 0
#define WINDOW_FOCUS_BG_RED 0x19
#define WINDOW_FOCUS_BG_GREEN 0x19
#define WINDOW_FOCUS_BG_BLUE 0x32

static uint8_t window_dir_ahead_is(uint32_t dir, double from_x, double from_y, double to_x, double to_y)
{
    if (dir == (uint32_t)CROSSTALK_UI_WINDOW_DIR_LHS) { return to_x < from_x ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    if (dir == (uint32_t)CROSSTALK_UI_WINDOW_DIR_RHS) { return to_x > from_x ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    if (dir == (uint32_t)CROSSTALK_UI_WINDOW_DIR_UP) { return to_y < from_y ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    return to_y > from_y ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static double window_dir_cost_get(uint32_t dir, double from_x, double from_y, double to_x, double to_y)
{
    double along = 0.0;
    double across = 0.0;

    if (dir == (uint32_t)CROSSTALK_UI_WINDOW_DIR_LHS || dir == (uint32_t)CROSSTALK_UI_WINDOW_DIR_RHS)
    {
        along = to_x > from_x ? to_x - from_x : from_x - to_x;
        across = to_y > from_y ? to_y - from_y : from_y - to_y;
    }
    else
    {
        along = to_y > from_y ? to_y - from_y : from_y - to_y;
        across = to_x > from_x ? to_x - from_x : from_x - to_x;
    }

    return along + across * WINDOW_MID;
}

static double window_frac_clamp(double frac)
{
    if (frac < WINDOW_FRAC_MIN) { return WINDOW_FRAC_MIN; }

    if (frac > WINDOW_FRAC_MAX) { return WINDOW_FRAC_MAX; }

    return frac;
}

static uint32_t window_level_set(ldg_term_render_color_t *out, uint8_t red, uint8_t green, uint8_t blue)
{
    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->red = red;
    out->green = green;
    out->blue = blue;
    out->pudding[0] = 0;

    return LDG_ERR_AOK;
}

static uint32_t window_scroll_key(crosstalk_ui_window_state_t *state, const ldg_term_key_event_t *key, uint8_t *consumed)
{
    uint32_t bottom = WINDOW_SCROLL_TOP;

    if (LDG_UNLIKELY(state == 0x0 || key == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (state->scroll_on != LDG_TRUTH_TRUE || key->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE) { return LDG_ERR_AOK; }

    if (state->content_row_cunt > state->view_row_cunt) { bottom = state->content_row_cunt - state->view_row_cunt; }

    if (key->key == (uint8_t)LDG_TERM_KEY_PAGE_UP)
    {
        state->scroll_row = state->scroll_row > state->view_row_cunt ? state->scroll_row - state->view_row_cunt : (uint32_t)WINDOW_SCROLL_TOP;
        *consumed = LDG_TRUTH_TRUE;
    }
    else if (key->key == (uint8_t)LDG_TERM_KEY_PAGE_DOWN)
    {
        state->scroll_row = state->scroll_row + state->view_row_cunt < bottom ? state->scroll_row + state->view_row_cunt : bottom;
        *consumed = LDG_TRUTH_TRUE;
    }
    else if (key->key == (uint8_t)LDG_TERM_KEY_HOME)
    {
        state->scroll_row = (uint32_t)WINDOW_SCROLL_TOP;
        *consumed = LDG_TRUTH_TRUE;
    }
    else if (key->key == (uint8_t)LDG_TERM_KEY_END)
    {
        state->scroll_row = bottom;
        *consumed = LDG_TRUTH_TRUE;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_ui_window_row_show(crosstalk_ui_window_state_t *state, uint32_t row)
{
    if (LDG_UNLIKELY(state == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (row < state->scroll_row) { state->scroll_row = row; return LDG_ERR_AOK; }

    if (state->view_row_cunt != 0 && row >= state->scroll_row + state->view_row_cunt) { state->scroll_row = row - state->view_row_cunt + (uint32_t)WINDOW_STEP; }

    return LDG_ERR_AOK;
}

static uint32_t window_node_key_cb(uint32_t node_id, const ldg_term_key_event_t *event, void *user, uint8_t *consumed)
{
    crosstalk_ui_window_slot_t *slot = (crosstalk_ui_window_slot_t *)user;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(event == 0x0 || slot == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    if (node_id >= (uint32_t)CROSSTALK_UI_WINDOW_SLOT_CAP) { return LDG_ERR_BOUNDS; }

    if (slot->state == 0x0 || slot->state->open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (slot->key_cb != 0x0)
    {
        ret = slot->key_cb(slot->id, slot->user, event, consumed);
        if (ret != LDG_ERR_AOK || *consumed == LDG_TRUTH_TRUE) { return ret; }
    }

    return window_scroll_key(slot->state, event, consumed);
}

uint32_t crosstalk_ui_window_init(crosstalk_ui_window_ctx_t *ctx, crosstalk_core_task_ctx_t *task)
{
    ldg_term_focus_conf_t conf = LDG_STRUCT_ZERO_INIT;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || task == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    for (at = 0; at < (uint32_t)CROSSTALK_UI_WINDOW_SLOT_CAP; at = at + (uint32_t)WINDOW_STEP) { ctx->slot[at].id = (uint64_t)UINT64_MAX; }

    ret = ldg_term_focus_conf_default_get(&conf);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    conf.node_cap = (uint32_t)CROSSTALK_UI_WINDOW_NODE_CAP;

    ret = ldg_term_focus_create(&conf, &ctx->focus);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_term_focus_node_add(ctx->focus, (uint32_t)CROSSTALK_UI_WINDOW_SLOT_CAP, (uint32_t)LDG_TERM_FOCUS_PARENT_NONE, 0x0, 0x0);
    if (ret == LDG_ERR_AOK) { ret = ldg_term_focus_set(ctx->focus, (uint32_t)CROSSTALK_UI_WINDOW_SLOT_CAP); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (ldg_term_focus_destroy(ctx->focus) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "window focus destroy failed"); }

        ctx->focus = 0x0;

        return ret;
    }

    ctx->task = task;
    ctx->focus_id = (uint64_t)UINT64_MAX;
    ctx->top_id = (uint64_t)UINT64_MAX;
    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_ui_window_shutdown(crosstalk_ui_window_ctx_t *ctx)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->focus != 0x0) { ret = ldg_term_focus_destroy(ctx->focus); }

    ctx->focus = 0x0;
    ctx->ok = LDG_TRUTH_FALSE;

    return ret;
}

uint32_t crosstalk_ui_window_reg(crosstalk_ui_window_ctx_t *ctx, uint64_t win_id, const char *title, crosstalk_ui_window_state_t *state, crosstalk_ui_window_draw_cb_t draw_cb, crosstalk_ui_window_key_cb_t key_cb, void *user, uint32_t *out_slot)
{
    crosstalk_ui_window_slot_t *slot = 0x0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || title == 0x0 || state == 0x0 || draw_cb == 0x0 || out_slot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_slot = (uint32_t)UINT32_MAX;

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (ctx->slot_cunt >= (uint32_t)CROSSTALK_UI_WINDOW_SLOT_CAP) { return CROSSTALK_UI_WINDOW_ERR_FULL; }

    idx = ctx->slot_cunt;
    slot = &ctx->slot[idx];

    if (snprintf(slot->title, sizeof(slot->title), WINDOW_NAME_FMT, title) < 0) { return LDG_ERR_STR_FMT; }

    slot->id = win_id;
    slot->state = state;
    slot->draw_cb = draw_cb;
    slot->key_cb = key_cb;
    slot->user = user;

    ret = ldg_term_focus_node_add(ctx->focus, idx, (uint32_t)CROSSTALK_UI_WINDOW_SLOT_CAP, window_node_key_cb, slot);
    if (ret != LDG_ERR_AOK)
    {
        memset(slot, 0x0, sizeof(*slot));
        slot->id = (uint64_t)UINT64_MAX;

        return ret;
    }

    ctx->slot_cunt = ctx->slot_cunt + (uint32_t)WINDOW_STEP;

    *out_slot = ctx->slot_cunt;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_ui_window_slot_get(const crosstalk_ui_window_ctx_t *ctx, uint64_t win_id, uint32_t *out_slot)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_slot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_slot = (uint32_t)UINT32_MAX;

    for (at = 0; at < ctx->slot_cunt; at = at + (uint32_t)WINDOW_STEP)
    {
        if (ctx->slot[at].id != win_id) { continue; }

        *out_slot = at + (uint32_t)CROSSTALK_UI_WINDOW_SLOT_FIRST;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t crosstalk_ui_window_id_get(const crosstalk_ui_window_ctx_t *ctx, uint32_t slot, uint64_t *out_id)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_id == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_id = (uint64_t)UINT64_MAX;

    if (slot < (uint32_t)CROSSTALK_UI_WINDOW_SLOT_FIRST || slot - (uint32_t)CROSSTALK_UI_WINDOW_SLOT_FIRST >= ctx->slot_cunt) { return CROSSTALK_UI_WINDOW_ERR_SLOT; }

    *out_id = ctx->slot[slot - (uint32_t)CROSSTALK_UI_WINDOW_SLOT_FIRST].id;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_ui_window_title_set(crosstalk_ui_window_ctx_t *ctx, uint64_t win_id, const char *title)
{
    uint32_t slot = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || title == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_ui_window_slot_get(ctx, win_id, &slot);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (snprintf(ctx->slot[slot - (uint32_t)CROSSTALK_UI_WINDOW_SLOT_FIRST].title, sizeof(ctx->slot[0].title), WINDOW_NAME_FMT, title) < 0) { return LDG_ERR_STR_FMT; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_ui_window_all_close(crosstalk_ui_window_ctx_t *ctx)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (at = 0; at < ctx->slot_cunt; at = at + (uint32_t)WINDOW_STEP) { if (ctx->slot[at].state != 0x0) { ctx->slot[at].state->open = LDG_TRUTH_FALSE; } }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_ui_window_focus_set(crosstalk_ui_window_ctx_t *ctx, uint64_t win_id, uint64_t now_ms)
{
    uint32_t slot = (uint32_t)UINT32_MAX;
    uint32_t node = (uint32_t)CROSSTALK_UI_WINDOW_SLOT_CAP;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (win_id == ctx->focus_id)
    {
        ctx->kbd_pending = LDG_TRUTH_FALSE;

        return LDG_ERR_AOK;
    }

    if (win_id != (uint64_t)UINT64_MAX)
    {
        ret = crosstalk_ui_window_slot_get(ctx, win_id, &slot);
        if (ret != LDG_ERR_AOK) { return ret; }

        node = slot - (uint32_t)CROSSTALK_UI_WINDOW_SLOT_FIRST;
    }

    ret = ldg_term_focus_set(ctx->focus, node);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->focus_id = win_id;
    ctx->focus_kbd = ctx->kbd_pending;
    ctx->focus_changed = LDG_TRUTH_TRUE;
    ctx->kbd_pending = LDG_TRUTH_FALSE;

    if (win_id == (uint64_t)UINT64_MAX) { return crosstalk_core_task_visit_close(ctx->task, now_ms); }

    return crosstalk_core_task_visit_enter(ctx->task, win_id, now_ms);
}

uint8_t crosstalk_ui_window_focused_is(const crosstalk_ui_window_ctx_t *ctx, uint64_t win_id)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->focus_id == win_id ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_ui_window_focus_slot(crosstalk_ui_window_ctx_t *ctx, uint32_t slot, uint64_t now_ms)
{
    uint64_t win_id = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_ui_window_id_get(ctx, slot, &win_id);
    if (ret != LDG_ERR_AOK) { return ret; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "window: focus slot; slot: %u32; id: 0x%X64", slot, win_id);

    ctx->kbd_pending = LDG_TRUTH_TRUE;

    return crosstalk_ui_window_focus_set(ctx, win_id, now_ms);
}

uint32_t crosstalk_ui_window_recent_focus(crosstalk_ui_window_ctx_t *ctx, uint64_t now_ms)
{
    crosstalk_core_task_visit_t visit = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_core_task_visit_recent_get(ctx->task, (uint32_t)CROSSTALK_UI_WINDOW_RECENT_BACK, &visit);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->kbd_pending = LDG_TRUTH_TRUE;

    return crosstalk_ui_window_focus_set(ctx, visit.win_id, now_ms);
}

uint32_t crosstalk_ui_window_focus_dir(crosstalk_ui_window_ctx_t *ctx, uint32_t dir, uint64_t now_ms)
{
    uint64_t best_id = (uint64_t)UINT64_MAX;
    double from_x = 0.0;
    double from_y = 0.0;
    double best_cost = WINDOW_DIST_NONE;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (at = 0; at < ctx->slot_cunt; at = at + (uint32_t)WINDOW_STEP)
    {
        const crosstalk_ui_window_state_t *state = ctx->slot[at].state;

        if (ctx->slot[at].id != ctx->focus_id || state == 0x0) { continue; }

        from_x = state->rect_x + state->rect_w / WINDOW_MID;
        from_y = state->rect_y + state->rect_h / WINDOW_MID;
    }

    for (at = 0; at < ctx->slot_cunt; at = at + (uint32_t)WINDOW_STEP)
    {
        const crosstalk_ui_window_state_t *state = ctx->slot[at].state;
        double to_x = 0.0;
        double to_y = 0.0;
        double cost = 0.0;

        if (state == 0x0 || ctx->slot[at].id == ctx->focus_id) { continue; }

        if (state->open != LDG_TRUTH_TRUE) { continue; }

        to_x = state->rect_x + state->rect_w / WINDOW_MID;
        to_y = state->rect_y + state->rect_h / WINDOW_MID;

        if (window_dir_ahead_is(dir, from_x, from_y, to_x, to_y) != LDG_TRUTH_TRUE) { continue; }

        cost = window_dir_cost_get(dir, from_x, from_y, to_x, to_y);

        if (best_cost != WINDOW_DIST_NONE && cost >= best_cost) { continue; }

        best_cost = cost;
        best_id = ctx->slot[at].id;
    }

    if (best_id == (uint64_t)UINT64_MAX) { return LDG_ERR_NOT_FOUND; }

    ctx->kbd_pending = LDG_TRUTH_TRUE;

    return crosstalk_ui_window_focus_set(ctx, best_id, now_ms);
}

uint32_t crosstalk_ui_window_digit_put(crosstalk_ui_window_ctx_t *ctx, uint32_t digit, uint64_t now_ms)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (ctx->digit_on != LDG_TRUTH_TRUE) { ctx->digit_val = (uint32_t)CROSSTALK_UI_WINDOW_DIGIT_NONE; }

    ctx->digit_val = ctx->digit_val * (uint32_t)CROSSTALK_UI_WINDOW_DIGIT_BASE + digit;
    ctx->digit_deadline_ms = now_ms + (uint64_t)CROSSTALK_UI_WINDOW_DIGIT_HOLD_MS;
    ctx->digit_on = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_ui_window_digit_step(crosstalk_ui_window_ctx_t *ctx, uint64_t now_ms)
{
    uint32_t slot = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->digit_on != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (now_ms < ctx->digit_deadline_ms) { return LDG_ERR_AOK; }

    slot = ctx->digit_val;

    ctx->digit_on = LDG_TRUTH_FALSE;
    ctx->digit_val = (uint32_t)CROSSTALK_UI_WINDOW_DIGIT_NONE;

    return crosstalk_ui_window_focus_slot(ctx, slot, now_ms);
}

uint32_t crosstalk_ui_window_key_route(crosstalk_ui_window_ctx_t *ctx, const ldg_term_key_event_t *key, uint8_t *consumed)
{
    if (LDG_UNLIKELY(ctx == 0x0 || key == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    return ldg_term_focus_key_route(ctx->focus, key, consumed);
}

uint32_t crosstalk_ui_window_rect_get(const crosstalk_ui_window_state_t *state, const ldg_term_rect_t *desk, ldg_term_rect_t *out)
{
    uint32_t col_lhs = 0;
    uint32_t col_rhs = 0;
    uint32_t row_top = 0;
    uint32_t row_bottom = 0;

    if (LDG_UNLIKELY(state == 0x0 || desk == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(out, 0x0, sizeof(*out));

    col_lhs = (uint32_t)(window_frac_clamp(state->rect_x) * (double)desk->col_cunt);
    col_rhs = (uint32_t)(window_frac_clamp(state->rect_x + state->rect_w) * (double)desk->col_cunt);
    row_top = (uint32_t)(window_frac_clamp(state->rect_y) * (double)desk->row_cunt);
    row_bottom = (uint32_t)(window_frac_clamp(state->rect_y + state->rect_h) * (double)desk->row_cunt);

    out->col = desk->col + col_lhs;
    out->row = desk->row + row_top;
    out->col_cunt = col_rhs > col_lhs ? col_rhs - col_lhs : (uint32_t)WINDOW_EXTENT_NONE;
    out->row_cunt = row_bottom > row_top ? row_bottom - row_top : (uint32_t)WINDOW_EXTENT_NONE;

    return LDG_ERR_AOK;
}

static uint32_t window_one_draw(crosstalk_ui_window_ctx_t *ctx, uint32_t idx, ldg_term_render_t *render, const ldg_term_rect_t *desk)
{
    char title[CROSSTALK_UI_WINDOW_TITLE_CAP] = LDG_ARR_ZERO_INIT;
    crosstalk_ui_window_slot_t *slot = 0x0;
    crosstalk_ui_window_state_t *state = 0x0;
    ldg_term_render_frame_style_t style = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t pane = LDG_STRUCT_ZERO_INIT;
    ldg_term_rect_t rect = LDG_STRUCT_ZERO_INIT;
    ldg_term_rect_t inner = LDG_STRUCT_ZERO_INIT;
    uint32_t frame_span = (uint32_t)CROSSTALK_UI_WINDOW_FRAME_CELLS * (uint32_t)WINDOW_FRAME_SIDES;
    uint32_t bottom = WINDOW_SCROLL_TOP;
    int32_t title_len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0 || desk == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (idx >= ctx->slot_cunt) { return LDG_ERR_BOUNDS; }

    slot = &ctx->slot[idx];
    state = slot->state;

    if (state == 0x0 || state->open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = crosstalk_ui_window_rect_get(state, desk, &rect);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (rect.col_cunt <= frame_span || rect.row_cunt <= frame_span) { return LDG_ERR_AOK; }

    if (slot->id == ctx->focus_id)
    {
        ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_FOCUS_RGBA, &style.border);
        if (ret == LDG_ERR_AOK) { ret = window_level_set(&style.bg, (uint8_t)WINDOW_FOCUS_BG_RED, (uint8_t)WINDOW_FOCUS_BG_GREEN, (uint8_t)WINDOW_FOCUS_BG_BLUE); }
    }
    else
    {
        ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_BORDER_RGBA, &style.border);
        if (ret == LDG_ERR_AOK) { ret = window_level_set(&style.bg, (uint8_t)WINDOW_FRAME_BG_LEVEL, (uint8_t)WINDOW_FRAME_BG_LEVEL, (uint8_t)WINDOW_FRAME_BG_LEVEL); }
    }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_ACCENT_RGBA, &style.accent); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_STATUS_RGBA, &style.title_fg); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_TITLE_BAR_RGBA, &style.title_bg); }

    if (ret != LDG_ERR_AOK) { return ret; }

    style.accent_edge = (uint8_t)LDG_TERM_RENDER_FRAME_EDGE_TOP_LHS;

    if (slot->id == ctx->flash_id)
    {
        uint64_t now_ms = ldg_time_epoch_ms_get();

        if (now_ms >= ctx->flash_from_ms && now_ms < ctx->flash_until_ms && ((now_ms - ctx->flash_from_ms) / (uint64_t)WINDOW_FLASH_PERIOD_MS) % (uint64_t)WINDOW_FLASH_PHASE_CUNT == (uint64_t)WINDOW_FLASH_PHASE_LIT)
        {
            ldg_term_render_color_t swap = style.title_bg;

            style.accent_edge = (uint8_t)LDG_TERM_RENDER_FRAME_EDGE_BOTTOM_RHS;
            style.title_bg = style.accent;
            style.title_fg = swap;
        }
    }

    title_len = (int32_t)snprintf(title, sizeof(title), CROSSTALK_UI_WINDOW_TITLE_FMT, idx + (uint32_t)CROSSTALK_UI_WINDOW_SLOT_FIRST, slot->title);
    if (title_len < 0) { return LDG_ERR_STR_FMT; }

    if ((uint32_t)title_len >= (uint32_t)sizeof(title)) { title_len = (int32_t)(sizeof(title) - LDG_STR_TERM_SIZE); }

    ret = ldg_term_render_clip_set(render, &rect, (uint32_t)WINDOW_SCROLL_COL_NONE, (uint32_t)WINDOW_SCROLL_TOP);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_CELL_PANE_RGBA, &pane);
    if (ret == LDG_ERR_AOK) { ret = ldg_term_render_clear(render, &pane); }

    if (ret == LDG_ERR_AOK) { ret = ldg_term_render_frame_draw(render, title, (uint64_t)title_len, &style); }

    if (ret != LDG_ERR_AOK)
    {
        if (ldg_term_render_clip_rst(render) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "window clip reset failed"); }

        return ret;
    }

    inner.col = rect.col + (uint32_t)CROSSTALK_UI_WINDOW_FRAME_CELLS;
    inner.row = rect.row + (uint32_t)CROSSTALK_UI_WINDOW_FRAME_CELLS;
    inner.col_cunt = rect.col_cunt - frame_span;
    inner.row_cunt = rect.row_cunt - frame_span;

    state->view_row_cunt = inner.row_cunt;

    if (state->scroll_on == LDG_TRUTH_TRUE && state->content_row_cunt > inner.row_cunt) { bottom = state->content_row_cunt - inner.row_cunt; }

    if (state->scroll_row > bottom) { state->scroll_row = bottom; }

    ret = ldg_term_render_clip_set(render, &inner, (uint32_t)WINDOW_SCROLL_COL_NONE, state->scroll_row);
    if (ret == LDG_ERR_AOK) { ret = slot->draw_cb(slot->id, slot->user, render, inner.col_cunt, inner.row_cunt); }

    if (ldg_term_render_clip_rst(render) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "window clip reset failed"); }

    return ret;
}

uint32_t crosstalk_ui_window_focus_rect_get(const crosstalk_ui_window_ctx_t *ctx, const ldg_term_rect_t *desk, ldg_term_rect_t *out_rect, uint64_t *out_id)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || desk == 0x0 || out_rect == 0x0 || out_id == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(out_rect, 0x0, sizeof(ldg_term_rect_t));
    *out_id = (uint64_t)UINT64_MAX;

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    for (at = 0; at < ctx->slot_cunt; at = at + (uint32_t)WINDOW_STEP)
    {
        if (ctx->slot[at].id != ctx->focus_id || ctx->slot[at].state == 0x0 || ctx->slot[at].state->open != LDG_TRUTH_TRUE) { continue; }

        *out_id = ctx->slot[at].id;

        return crosstalk_ui_window_rect_get(ctx->slot[at].state, desk, out_rect);
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t crosstalk_ui_window_top_set(crosstalk_ui_window_ctx_t *ctx, uint64_t win_id)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->top_id = win_id;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_ui_window_flash_set(crosstalk_ui_window_ctx_t *ctx, uint64_t win_id, uint64_t until_ms)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->flash_id = win_id;
    ctx->flash_from_ms = ldg_time_epoch_ms_get();
    ctx->flash_until_ms = until_ms;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_ui_window_all_draw(crosstalk_ui_window_ctx_t *ctx, ldg_term_render_t *render, const ldg_term_rect_t *desk)
{
    uint32_t focus_idx = (uint32_t)UINT32_MAX;
    uint32_t top_idx = (uint32_t)UINT32_MAX;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0 || desk == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    for (at = 0; at < ctx->slot_cunt; at = at + (uint32_t)WINDOW_STEP)
    {
        if (ctx->slot[at].id == ctx->top_id) { top_idx = at; continue; }

        if (ctx->slot[at].id == ctx->focus_id) { focus_idx = at; continue; }

        ret = window_one_draw(ctx, at, render, desk);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "window: draw failed; id: 0x%X64; err: %u32", ctx->slot[at].id, ret); }
    }

    if (focus_idx != (uint32_t)UINT32_MAX)
    {
        ret = window_one_draw(ctx, focus_idx, render, desk);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "window: draw failed; id: 0x%X64; err: %u32", ctx->slot[focus_idx].id, ret); }
    }

    if (top_idx == (uint32_t)UINT32_MAX) { return LDG_ERR_AOK; }

    return window_one_draw(ctx, top_idx, render, desk);
}
