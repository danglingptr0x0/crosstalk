#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/logging/logging.h>

#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>
#include <dangling/time/time.h>

#include "core/log.h"
#include "ui/cell.h"
#include "ui/theme.h"
#include "ui/guide.h"

#define GUIDE_STEP 1
#define GUIDE_STEP_FIRST 0
#define GUIDE_ORIG_COL 0
#define GUIDE_ROW_TITLE 0
#define GUIDE_ROW_GAP 1
#define GUIDE_CENTER_DIV 2
#define GUIDE_HOT_MARK '`'
#define GUIDE_BACK_LABEL "Backward"
#define GUIDE_SHOW_LABEL "Highlight"
#define GUIDE_NEXT_LABEL "Forward"
#define GUIDE_IDLE_TEXT "no guide running; guide <name> to start"
#define GUIDE_STEP_FMT "%u of %u"
#define GUIDE_STEP_TEXT_CAP 32
#define GUIDE_KEY_NEXT 'l'
#define GUIDE_KEY_BACK 'h'
#define GUIDE_FLASH_MS 900
#define GUIDE_TITLE_ROWS 1
#define GUIDE_ACTION_ROWS 1
#define GUIDE_BTN_ROWS 1

static uint32_t guide_wrap(const char *text, uint32_t max_w, uint32_t *seg_start, uint32_t *seg_len, uint32_t *out_cunt)
{
    uint32_t total = (uint32_t)strlen(text);
    uint32_t pos = 0;
    uint32_t cunt = 0;

    *out_cunt = 0;

    if (max_w == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    while (pos < total && cunt < (uint32_t)CROSSTALK_UI_GUIDE_WRAP_CAP)
    {
        uint32_t take = total - pos < max_w ? total - pos : max_w;
        uint32_t brk = 0;

        if (pos + take < total)
        {
            for (brk = take; brk > 0; brk = brk - (uint32_t)GUIDE_STEP) { if (text[pos + brk - (uint32_t)GUIDE_STEP] == LDG_ASCII_SPACE) { break; } }

            if (brk > 0) { take = brk; }
        }

        seg_start[cunt] = pos;
        seg_len[cunt] = take;
        cunt = cunt + (uint32_t)GUIDE_STEP;
        pos = pos + take;

        while (pos < total && text[pos] == LDG_ASCII_SPACE) { pos = pos + (uint32_t)GUIDE_STEP; }
    }

    *out_cunt = cunt;

    return LDG_ERR_AOK;
}

static LDG_NOINLINE uint32_t guide_body_draw(ldg_term_render_t *render, const char *body, uint32_t col_cunt, uint32_t *row)
{
    uint32_t seg_start[CROSSTALK_UI_GUIDE_WRAP_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t seg_len[CROSSTALK_UI_GUIDE_WRAP_CAP] = LDG_ARR_ZERO_INIT;
    ldg_term_render_color_t body_col = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t hot_col = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t pane = LDG_STRUCT_ZERO_INIT;
    uint32_t seg_cunt = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;
    uint8_t hot = LDG_TRUTH_FALSE;

    ret = guide_wrap(body, col_cunt, seg_start, seg_len, &seg_cunt);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, &body_col); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_FOCUS_RGBA, &hot_col); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_CELL_PANE_RGBA, &pane); }

    if (ret != LDG_ERR_AOK) { return ret; }

    for (idx = 0; idx < seg_cunt; idx = idx + (uint32_t)GUIDE_STEP)
    {
        const char *line = body + seg_start[idx];
        uint32_t take = seg_len[idx];
        uint32_t run_start = 0;
        uint32_t draw_col = GUIDE_ORIG_COL;
        uint32_t at = 0;

        for (at = 0; at <= take; at = at + (uint32_t)GUIDE_STEP)
        {
            if (at != take && line[at] != GUIDE_HOT_MARK) { continue; }

            if (at > run_start)
            {
                ret = ldg_term_render_text_draw(render, draw_col, *row, line + run_start, (uint64_t)(at - run_start), hot == LDG_TRUTH_TRUE ? &hot_col : &body_col, &pane);
                if (ret != LDG_ERR_AOK) { return ret; }

                draw_col = draw_col + (at - run_start);
            }

            if (at != take) { hot = hot == LDG_TRUTH_TRUE ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE; }

            run_start = at + (uint32_t)GUIDE_STEP;
        }

        *row = *row + (uint32_t)GUIDE_STEP;
    }

    *row = *row + (uint32_t)GUIDE_ROW_GAP;

    return LDG_ERR_AOK;
}

static uint8_t guide_step_done_is(crosstalk_ui_guide_ctx_t *ctx)
{
    const crosstalk_ui_guide_step_t *step = 0x0;
    uint32_t bit = 0;

    if (ctx->live == 0x0 || ctx->step_idx >= ctx->live->step_cunt) { return LDG_ERR_NOT_INIT; }

    step = &ctx->live->step[ctx->step_idx];

    if (step->done_id == (uint64_t)UINT64_MAX) { return LDG_TRUTH_TRUE; }

    bit = (uint32_t)LDG_BIT(ctx->step_idx);

    if ((ctx->done_mask & bit) != 0) { return LDG_TRUTH_TRUE; }

    if (ctx->done_cb == 0x0) { return LDG_TRUTH_TRUE; }

    if (ctx->done_cb(ctx->tgt_user, step->done_id) != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    ctx->done_mask = ctx->done_mask | bit;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "guide: step done; name: %.31s; idx: %u32", ctx->live->name, ctx->step_idx);

    return LDG_TRUTH_TRUE;
}

static uint32_t guide_btns_draw(crosstalk_ui_guide_ctx_t *ctx, ldg_term_render_t *render, uint32_t row, uint32_t col_cunt)
{
    const crosstalk_ui_guide_step_t *step = &ctx->live->step[ctx->step_idx];
    uint32_t next_w = (uint32_t)strlen(GUIDE_NEXT_LABEL);
    uint32_t show_w = (uint32_t)strlen(GUIDE_SHOW_LABEL);
    uint32_t ret = LDG_ERR_AOK;

    if (ctx->step_idx > (uint32_t)GUIDE_STEP_FIRST) { ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)GUIDE_ORIG_COL, row, GUIDE_BACK_LABEL, (uint64_t)GUIDE_STEP_TEXT_CAP, (uint32_t)CROSSTALK_UI_THEME_STATUS_RGBA, 0x0); }

    if (ret == LDG_ERR_AOK && step->tgt_id != (uint64_t)UINT64_MAX && col_cunt > show_w) { ret = crosstalk_ui_cell_rgba_put(render, (col_cunt - show_w) / (uint32_t)GUIDE_CENTER_DIV, row, GUIDE_SHOW_LABEL, (uint64_t)GUIDE_STEP_TEXT_CAP, (uint32_t)CROSSTALK_UI_THEME_STATUS_RGBA, 0x0); }

    if (ret == LDG_ERR_AOK && ctx->step_idx + (uint32_t)GUIDE_STEP < ctx->live->step_cunt && guide_step_done_is(ctx) == LDG_TRUTH_TRUE && col_cunt > next_w) { ret = crosstalk_ui_cell_rgba_put(render, col_cunt - next_w, row, GUIDE_NEXT_LABEL, (uint64_t)GUIDE_STEP_TEXT_CAP, (uint32_t)CROSSTALK_UI_THEME_STATUS_RGBA, 0x0); }

    return ret;
}

static uint32_t guide_draw_cb(uint64_t win_id, void *user, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    crosstalk_ui_guide_ctx_t *ctx = (crosstalk_ui_guide_ctx_t *)user;
    char step_text[GUIDE_STEP_TEXT_CAP] = LDG_ARR_ZERO_INIT;
    const crosstalk_ui_guide_step_t *step = 0x0;
    uint64_t title_len = 0;
    uint32_t meta_w = 0;
    uint32_t row = GUIDE_ROW_TITLE;
    uint32_t ret = 0;
    int32_t wrote = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (win_id != (uint64_t)CROSSTALK_UI_GUIDE_WINDOW_ID || col_cunt == 0 || row_cunt == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->live == 0x0 || ctx->step_idx >= ctx->live->step_cunt)
    {
        ctx->win.content_row_cunt = (uint32_t)GUIDE_STEP;

        return crosstalk_ui_cell_rgba_put(render, (uint32_t)GUIDE_ORIG_COL, row, GUIDE_IDLE_TEXT, (uint64_t)CROSSTALK_UI_GUIDE_LINE_CAP, (uint32_t)CROSSTALK_UI_THEME_DEAD_RGBA, 0x0);
    }

    step = &ctx->live->step[ctx->step_idx];

    wrote = snprintf(step_text, sizeof(step_text), GUIDE_STEP_FMT, ctx->step_idx + (uint32_t)GUIDE_STEP, ctx->live->step_cunt);
    if (wrote < 0) { return LDG_ERR_STR_FMT; }

    meta_w = (uint32_t)wrote;

    ret = ldg_str_len_get(step->title, (uint64_t)CROSSTALK_UI_GUIDE_LINE_CAP, &title_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (col_cunt > meta_w + (uint32_t)GUIDE_ROW_GAP && title_len > (uint64_t)(col_cunt - meta_w - (uint32_t)GUIDE_ROW_GAP)) { title_len = (uint64_t)(col_cunt - meta_w - (uint32_t)GUIDE_ROW_GAP); }

    {
        ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
        ldg_term_render_color_t pane = LDG_STRUCT_ZERO_INIT;

        ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_TITLE_RGBA, &fg);
        if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_CELL_PANE_RGBA, &pane); }

        if (ret == LDG_ERR_AOK && title_len != 0) { ret = ldg_term_render_text_draw(render, (uint32_t)GUIDE_ORIG_COL, row, step->title, title_len, &fg, &pane); }
    }

    if (ret == LDG_ERR_AOK && col_cunt > meta_w) { ret = crosstalk_ui_cell_rgba_put(render, col_cunt - meta_w, row, step_text, (uint64_t)sizeof(step_text), (uint32_t)CROSSTALK_UI_THEME_STATUS_RGBA, 0x0); }

    if (ret != LDG_ERR_AOK) { return ret; }

    row = row + (uint32_t)GUIDE_STEP + (uint32_t)GUIDE_ROW_GAP;

    if (step->action != 0x0)
    {
        uint64_t action_len = 0;

        ret = ldg_str_len_get(step->action, (uint64_t)CROSSTALK_UI_GUIDE_LINE_CAP, &action_len);
        if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_rgba_put(render, action_len < (uint64_t)col_cunt ? (col_cunt - (uint32_t)action_len) / (uint32_t)GUIDE_CENTER_DIV : (uint32_t)GUIDE_ORIG_COL, row, step->action, (uint64_t)CROSSTALK_UI_GUIDE_LINE_CAP, (uint32_t)CROSSTALK_UI_THEME_STATUS_RGBA, 0x0); }

        if (ret != LDG_ERR_AOK) { return ret; }

        row = row + (uint32_t)GUIDE_STEP + (uint32_t)GUIDE_ROW_GAP;
    }

    ret = guide_body_draw(render, step->body, col_cunt, &row);
    if (ret == LDG_ERR_AOK) { ret = guide_btns_draw(ctx, render, row, col_cunt); }

    ctx->win.content_row_cunt = row + (uint32_t)GUIDE_STEP;

    return ret;
}

static uint32_t guide_key_isr(uint64_t win_id, void *user, const ldg_term_key_event_t *key, uint8_t *consumed)
{
    crosstalk_ui_guide_ctx_t *ctx = (crosstalk_ui_guide_ctx_t *)user;

    if (LDG_UNLIKELY(ctx == 0x0 || key == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    if (win_id != (uint64_t)CROSSTALK_UI_GUIDE_WINDOW_ID) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (key->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE || ctx->live == 0x0) { return LDG_ERR_AOK; }

    if ((key->mods & ((uint32_t)LDG_TERM_MOD_CTRL | (uint32_t)LDG_TERM_MOD_ALT | (uint32_t)LDG_TERM_MOD_SUPER)) != 0) { return LDG_ERR_AOK; }

    *consumed = LDG_TRUTH_TRUE;

    if (key->key == (uint8_t)LDG_TERM_KEY_RHS || (key->key == (uint8_t)LDG_TERM_KEY_NONE && key->codepoint == (uint32_t)GUIDE_KEY_NEXT)) { return crosstalk_ui_guide_next(ctx); }

    if (key->key == (uint8_t)LDG_TERM_KEY_LHS || (key->key == (uint8_t)LDG_TERM_KEY_NONE && key->codepoint == (uint32_t)GUIDE_KEY_BACK)) { return crosstalk_ui_guide_back(ctx); }

    if (key->key == (uint8_t)LDG_TERM_KEY_ENTER || (key->key == (uint8_t)LDG_TERM_KEY_NONE && key->codepoint == (uint32_t)LDG_ASCII_SPACE)) { return crosstalk_ui_guide_show(ctx); }

    if (ctx->step_idx < ctx->live->step_cunt && key->key == (uint8_t)LDG_TERM_KEY_NONE && key->codepoint > (uint32_t)LDG_ASCII_DIGIT_ZERO && key->codepoint <= (uint32_t)LDG_ASCII_DIGIT_ZERO + ctx->live->step[ctx->step_idx].pick_cunt)
    {
        ctx->pick = key->codepoint - (uint32_t)LDG_ASCII_DIGIT_ZERO;
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "guide: pick; name: %.31s; step: %u32; pick: %u32", ctx->live->name, ctx->step_idx, ctx->pick);

        return LDG_ERR_AOK;
    }

    *consumed = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_ui_guide_init(crosstalk_ui_guide_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->step_idx = (uint32_t)UINT32_MAX;
    ctx->pick = (uint32_t)UINT32_MAX;
    ctx->win.scroll_on = LDG_TRUTH_TRUE;
    ctx->win.open = LDG_TRUTH_FALSE;
    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_ui_guide_shutdown(crosstalk_ui_guide_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->live = 0x0;
    ctx->ok = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_ui_guide_bind(crosstalk_ui_guide_ctx_t *ctx, crosstalk_ui_window_ctx_t *nav, crosstalk_ui_guide_tgt_open_cb_t tgt_open_cb, crosstalk_ui_guide_done_cb_t done_cb, void *tgt_user)
{
    if (LDG_UNLIKELY(ctx == 0x0 || nav == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->nav = nav;
    ctx->tgt_open_cb = tgt_open_cb;
    ctx->done_cb = done_cb;
    ctx->tgt_user = tgt_user;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_ui_guide_win_reg(crosstalk_ui_guide_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || win_nav == 0x0 || out_slot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_ui_window_reg(win_nav, (uint64_t)CROSSTALK_UI_GUIDE_WINDOW_ID, CROSSTALK_UI_GUIDE_WINDOW_TITLE, &ctx->win, guide_draw_cb, guide_key_isr, ctx, out_slot);
    if (ret != LDG_ERR_AOK) { return ret; }

    return crosstalk_ui_window_top_set(win_nav, (uint64_t)CROSSTALK_UI_GUIDE_WINDOW_ID);
}

uint32_t crosstalk_ui_guide_reg(crosstalk_ui_guide_ctx_t *ctx, const crosstalk_ui_guide_t *guide)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || guide == 0x0 || guide->name == 0x0 || guide->step == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (guide->step_cunt == 0 || guide->step_cunt > (uint32_t)CROSSTALK_UI_GUIDE_STEP_CAP) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->reg_cunt >= (uint32_t)CROSSTALK_UI_GUIDE_REG_CAP) { return LDG_ERR_FULL; }

    for (idx = 0; idx < ctx->reg_cunt; idx = idx + (uint32_t)GUIDE_STEP) { if (strcmp(ctx->reg[idx]->name, guide->name) == 0) { return LDG_ERR_EXISTS; } }

    ctx->reg[ctx->reg_cunt] = guide;
    ctx->reg_cunt = ctx->reg_cunt + (uint32_t)GUIDE_STEP;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_ui_guide_extent_get(const crosstalk_ui_guide_ctx_t *ctx, uint32_t col_cunt, uint32_t *out_row_cunt)
{
    uint32_t seg_start[CROSSTALK_UI_GUIDE_WRAP_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t seg_len[CROSSTALK_UI_GUIDE_WRAP_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t seg_cunt = 0;
    uint32_t rows = 0;
    uint32_t guide_idx = 0;
    uint32_t step_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_row_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_row_cunt = 0;

    if (col_cunt == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (guide_idx = 0; guide_idx < ctx->reg_cunt; guide_idx = guide_idx + (uint32_t)GUIDE_STEP)
    {
        for (step_idx = 0; step_idx < ctx->reg[guide_idx]->step_cunt; step_idx = step_idx + (uint32_t)GUIDE_STEP)
        {
            const crosstalk_ui_guide_step_t *step = &ctx->reg[guide_idx]->step[step_idx];

            rows = (uint32_t)GUIDE_TITLE_ROWS + (uint32_t)GUIDE_ROW_GAP;

            if (step->action != 0x0) { rows = rows + (uint32_t)GUIDE_ACTION_ROWS + (uint32_t)GUIDE_ROW_GAP; }

            ret = guide_wrap(step->body, col_cunt, seg_start, seg_len, &seg_cunt);
            if (ret != LDG_ERR_AOK) { return ret; }

            rows = rows + seg_cunt + (uint32_t)GUIDE_ROW_GAP + (uint32_t)GUIDE_BTN_ROWS;

            if (rows > *out_row_cunt) { *out_row_cunt = rows; }
        }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_ui_guide_name_get(const crosstalk_ui_guide_ctx_t *ctx, uint32_t idx, const char **out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (idx >= ctx->reg_cunt) { return LDG_ERR_BOUNDS; }

    *out = ctx->reg[idx]->name;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_ui_guide_step_set(crosstalk_ui_guide_ctx_t *ctx, uint32_t step_idx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live == 0x0) { return LDG_ERR_NOT_INIT; }

    if (step_idx >= ctx->live->step_cunt) { return LDG_ERR_BOUNDS; }

    ctx->step_idx = step_idx;
    ctx->win.scroll_row = 0;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "guide: step; name: %.31s; idx: %u32; cunt: %u32", ctx->live->name, step_idx, ctx->live->step_cunt);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_ui_guide_at_start(crosstalk_ui_guide_ctx_t *ctx, uint32_t idx)
{
    uint64_t now_ms = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (idx >= ctx->reg_cunt) { return LDG_ERR_BOUNDS; }

    ctx->live = ctx->reg[idx];
    ctx->done_mask = 0;
    ctx->win.open = LDG_TRUTH_TRUE;

    ret = crosstalk_ui_guide_step_set(ctx, (uint32_t)GUIDE_STEP_FIRST);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (ctx->nav != 0x0 && ldg_time_monotonic_get(&now_ms) == LDG_ERR_AOK) { if (crosstalk_ui_window_focus_set(ctx->nav, (uint64_t)CROSSTALK_UI_GUIDE_WINDOW_ID, now_ms) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "guide focus failed"); } }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "guide: start; name: %.31s; steps: %u32", ctx->live->name, ctx->live->step_cunt);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_ui_guide_start(crosstalk_ui_guide_ctx_t *ctx, const char *name)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    for (idx = 0; idx < ctx->reg_cunt; idx = idx + (uint32_t)GUIDE_STEP)
    {
        if (strcmp(ctx->reg[idx]->name, name) != 0) { continue; }

        return crosstalk_ui_guide_at_start(ctx, idx);
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t crosstalk_ui_guide_next(crosstalk_ui_guide_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live == 0x0) { return LDG_ERR_NOT_INIT; }

    if (ctx->step_idx + (uint32_t)GUIDE_STEP >= ctx->live->step_cunt) { return LDG_ERR_AOK; }

    if (guide_step_done_is(ctx) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    return crosstalk_ui_guide_step_set(ctx, ctx->step_idx + (uint32_t)GUIDE_STEP);
}

uint32_t crosstalk_ui_guide_back(crosstalk_ui_guide_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live == 0x0) { return LDG_ERR_NOT_INIT; }

    if (ctx->step_idx == (uint32_t)GUIDE_STEP_FIRST) { return LDG_ERR_AOK; }

    return crosstalk_ui_guide_step_set(ctx, ctx->step_idx - (uint32_t)GUIDE_STEP);
}

uint32_t crosstalk_ui_guide_show(crosstalk_ui_guide_ctx_t *ctx)
{
    uint64_t tgt_id = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live == 0x0 || ctx->step_idx >= ctx->live->step_cunt) { return LDG_ERR_NOT_INIT; }

    tgt_id = ctx->live->step[ctx->step_idx].tgt_id;

    if (tgt_id == (uint64_t)UINT64_MAX) { return LDG_ERR_AOK; }

    if (ctx->tgt_open_cb != 0x0)
    {
        ret = ctx->tgt_open_cb(ctx->tgt_user, tgt_id);
        if (ret != LDG_ERR_AOK)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "guide: target open failed; id: 0x%X64; err: %u32", tgt_id, ret);
            return ret;
        }
    }

    if (ctx->nav == 0x0) { return LDG_ERR_AOK; }

    ret = crosstalk_ui_window_flash_set(ctx->nav, tgt_id, ldg_time_epoch_ms_get() + (uint64_t)GUIDE_FLASH_MS);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_DEBUG : LDG_LOGGING_LVL_WARN, "guide: show; id: 0x%X64; err: %u32", tgt_id, ret);

    return ret;
}

uint32_t crosstalk_ui_guide_close(crosstalk_ui_guide_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->live = 0x0;
    ctx->step_idx = (uint32_t)UINT32_MAX;
    ctx->win.open = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_ui_guide_tgt_get(const crosstalk_ui_guide_ctx_t *ctx, uint64_t *out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (uint64_t)UINT64_MAX;

    if (ctx->live == 0x0 || ctx->win.open == LDG_TRUTH_FALSE) { return LDG_ERR_NOT_INIT; }

    if (ctx->step_idx >= ctx->live->step_cunt) { return LDG_ERR_BOUNDS; }

    *out = ctx->live->step[ctx->step_idx].tgt_id;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_ui_guide_pick_get(crosstalk_ui_guide_ctx_t *ctx, uint32_t *out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = ctx->pick;
    ctx->pick = (uint32_t)UINT32_MAX;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_ui_guide_live_is(const crosstalk_ui_guide_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->live != 0x0 && ctx->win.open != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}
