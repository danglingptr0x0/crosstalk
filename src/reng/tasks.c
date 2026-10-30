#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "reng/tasks.h"
#include "ui/cell.h"
#include "ui/theme.h"

#define TASKS_STEP 1
#define TASKS_ORIG_COL 0
#define TASKS_ROW_HDR 0
#define TASKS_ROW_ENTRY 1
#define TASKS_ROW_LIST 3
#define TASKS_ENTRY_MARK "+ "
#define TASKS_ENTRY_MARK_W 2
#define TASKS_CURSOR_W 1
#define TASKS_ACTIVE_FIRST 0
#define TASKS_LINE_CAP 384
#define TASKS_NONE_TEXT "no project; enter through the Reverse notch on the main menu"
#define TASKS_EMPTY_TEXT "no tasks yet"
#define TASKS_OPEN_TEXT "open"
#define TASKS_DONE_TEXT "done"

static uint32_t tasks_items_build(crosstalk_reng_tasks_ctx_t *ctx)
{
    uint32_t idx = 0;
    uint32_t proj = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || ctx->reng == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->rows_cunt = 0;
    proj = crosstalk_reng_sel_get(ctx->reng);

    if (proj == (uint32_t)UINT32_MAX) { return LDG_ERR_AOK; }

    for (idx = 0; idx < crosstalk_reng_task_cunt_get(ctx->reng) && ctx->rows_cunt < (uint32_t)CROSSTALK_RENG_TASKS_ROWS_MAX; idx = idx + (uint32_t)TASKS_STEP)
    {
        const crosstalk_reng_task_t *task = 0x0;

        if (crosstalk_reng_task_get(ctx->reng, idx, &task) != LDG_ERR_AOK || task == 0x0 || task->proj != proj) { continue; }

        if (snprintf(ctx->row_label[ctx->rows_cunt], sizeof(ctx->row_label[ctx->rows_cunt]), "%.4s; %.159s", task->done == LDG_TRUTH_TRUE ? TASKS_DONE_TEXT : TASKS_OPEN_TEXT, task->text) < 0) { break; }

        ctx->row_idx[ctx->rows_cunt] = idx;
        ctx->rows_cunt = ctx->rows_cunt + (uint32_t)TASKS_STEP;
    }

    if (ctx->active != UINT32_MAX && ctx->active >= ctx->rows_cunt) { ctx->active = ctx->rows_cunt == 0 ? UINT32_MAX : ctx->rows_cunt - (uint32_t)TASKS_STEP; }

    return LDG_ERR_AOK;
}

static uint32_t tasks_add_apply(crosstalk_reng_tasks_ctx_t *ctx)
{
    uint32_t proj = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || ctx->reng == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->entry[0] == LDG_STR_TERM) { return LDG_ERR_AOK; }

    proj = crosstalk_reng_sel_get(ctx->reng);
    if (proj == (uint32_t)UINT32_MAX) { return LDG_ERR_NOT_FOUND; }

    ret = crosstalk_reng_task_add(ctx->reng, proj, ctx->entry, &idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->entry[0] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

static uint32_t tasks_action_apply(crosstalk_reng_tasks_ctx_t *ctx, uint32_t row)
{
    const crosstalk_reng_task_t *task = 0x0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || ctx->reng == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (row >= ctx->rows_cunt) { return LDG_ERR_BOUNDS; }

    idx = ctx->row_idx[row];

    if (crosstalk_reng_task_get(ctx->reng, idx, &task) != LDG_ERR_AOK || task == 0x0) { return LDG_ERR_NOT_FOUND; }

    return crosstalk_reng_task_done_set(ctx->reng, idx, task->done == LDG_TRUTH_TRUE ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE);
}

static uint32_t tasks_entry_draw(const crosstalk_reng_tasks_ctx_t *ctx, ldg_term_render_t *render)
{
    ldg_term_render_color_t cursor = LDG_STRUCT_ZERO_INIT;
    uint64_t len = 0;
    uint32_t ret = 0;

    ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)TASKS_ORIG_COL, (uint32_t)TASKS_ROW_ENTRY, TASKS_ENTRY_MARK, (uint64_t)TASKS_LINE_CAP, (uint32_t)CROSSTALK_UI_THEME_DEAD_RGBA, 0x0);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)TASKS_ENTRY_MARK_W, (uint32_t)TASKS_ROW_ENTRY, ctx->entry, (uint64_t)sizeof(ctx->entry), (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, 0x0); }

    if (ret == LDG_ERR_AOK) { ret = ldg_str_len_get(ctx->entry, (uint64_t)sizeof(ctx->entry), &len); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, &cursor); }

    if (ret != LDG_ERR_AOK) { return ret; }

    return crosstalk_ui_cell_span_fill(render, (uint32_t)TASKS_ENTRY_MARK_W + (uint32_t)len, (uint32_t)TASKS_ROW_ENTRY, (uint32_t)TASKS_CURSOR_W, &cursor, &cursor);
}

static uint32_t tasks_list_draw(const crosstalk_reng_tasks_ctx_t *ctx, ldg_term_render_t *render, uint32_t col_cunt)
{
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t bg = LDG_STRUCT_ZERO_INIT;
    uint32_t row = 0;
    uint32_t ret = 0;

    ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_LIT_RGBA, &fg);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_SEL_BG_RGBA, &bg); }

    if (ret != LDG_ERR_AOK) { return ret; }

    for (row = 0; row < ctx->rows_cunt; row = row + (uint32_t)TASKS_STEP)
    {
        if (row == ctx->active)
        {
            ret = crosstalk_ui_cell_span_fill(render, (uint32_t)TASKS_ORIG_COL, (uint32_t)TASKS_ROW_LIST + row, col_cunt, &fg, &bg);
            if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_text_put(render, (uint32_t)TASKS_ORIG_COL, (uint32_t)TASKS_ROW_LIST + row, ctx->row_label[row], (uint64_t)CROSSTALK_RENG_TASKS_LABEL_CAP, &fg, &bg); }
        }
        else { ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)TASKS_ORIG_COL, (uint32_t)TASKS_ROW_LIST + row, ctx->row_label[row], (uint64_t)CROSSTALK_RENG_TASKS_LABEL_CAP, (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, 0x0); }

        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t tasks_draw_cb(uint64_t win_id, void *user, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    crosstalk_reng_tasks_ctx_t *ctx = (crosstalk_reng_tasks_ctx_t *)user;
    const crosstalk_reng_proj_t *proj = 0x0;
    char line[TASKS_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t sel = 0;
    uint32_t open_cunt = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE || ctx->reng == 0x0) { return LDG_ERR_NOT_INIT; }

    if (win_id != (uint64_t)CROSSTALK_RENG_TASKS_ID || col_cunt == 0 || row_cunt == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    sel = crosstalk_reng_sel_get(ctx->reng);

    if (sel == (uint32_t)UINT32_MAX || crosstalk_reng_proj_get(ctx->reng, sel, &proj) != LDG_ERR_AOK || proj == 0x0)
    {
        ctx->win.content_row_cunt = (uint32_t)TASKS_STEP;

        return crosstalk_ui_cell_rgba_put(render, (uint32_t)TASKS_ORIG_COL, (uint32_t)TASKS_ROW_HDR, TASKS_NONE_TEXT, (uint64_t)TASKS_LINE_CAP, (uint32_t)CROSSTALK_UI_THEME_DEAD_RGBA, 0x0);
    }

    if (ctx->gen_seen != ctx->reng->gen)
    {
        ret = tasks_items_build(ctx);
        if (ret != LDG_ERR_AOK) { return ret; }

        ctx->gen_seen = ctx->reng->gen;
    }

    for (idx = 0; idx < ctx->rows_cunt; idx = idx + (uint32_t)TASKS_STEP)
    {
        const crosstalk_reng_task_t *task = 0x0;

        if (crosstalk_reng_task_get(ctx->reng, ctx->row_idx[idx], &task) == LDG_ERR_AOK && task != 0x0 && task->done != LDG_TRUTH_TRUE) { open_cunt = open_cunt + (uint32_t)TASKS_STEP; }
    }

    if (snprintf(line, sizeof(line), "%.63s; %u open; %u total", proj->name, open_cunt, ctx->rows_cunt) < 0) { return LDG_ERR_STR_FMT; }

    ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)TASKS_ORIG_COL, (uint32_t)TASKS_ROW_HDR, line, (uint64_t)sizeof(line), (uint32_t)CROSSTALK_UI_THEME_LIVE_RGBA, 0x0);
    if (ret == LDG_ERR_AOK) { ret = tasks_entry_draw(ctx, render); }

    if (ret != LDG_ERR_AOK) { return ret; }

    if (ctx->rows_cunt == 0)
    {
        ctx->win.content_row_cunt = (uint32_t)TASKS_ROW_LIST + (uint32_t)TASKS_STEP;

        return crosstalk_ui_cell_rgba_put(render, (uint32_t)TASKS_ORIG_COL, (uint32_t)TASKS_ROW_LIST, TASKS_EMPTY_TEXT, (uint64_t)TASKS_LINE_CAP, (uint32_t)CROSSTALK_UI_THEME_DEAD_RGBA, 0x0);
    }

    ctx->win.content_row_cunt = (uint32_t)TASKS_ROW_LIST + ctx->rows_cunt;

    return tasks_list_draw(ctx, render, col_cunt);
}

static uint32_t tasks_key_isr(uint64_t win_id, void *user, const ldg_term_key_event_t *key, uint8_t *consumed)
{
    crosstalk_reng_tasks_ctx_t *ctx = (crosstalk_reng_tasks_ctx_t *)user;
    uint64_t len = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || key == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    if (win_id != (uint64_t)CROSSTALK_RENG_TASKS_ID) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (key->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE || ctx->reng == 0x0) { return LDG_ERR_AOK; }

    if ((key->mods & ((uint32_t)LDG_TERM_MOD_CTRL | (uint32_t)LDG_TERM_MOD_ALT | (uint32_t)LDG_TERM_MOD_SUPER)) != 0) { return LDG_ERR_AOK; }

    ret = ldg_str_len_get(ctx->entry, (uint64_t)sizeof(ctx->entry), &len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *consumed = LDG_TRUTH_TRUE;

    if (key->key == (uint8_t)LDG_TERM_KEY_UP || key->key == (uint8_t)LDG_TERM_KEY_DOWN)
    {
        if (ctx->rows_cunt == 0) { return LDG_ERR_AOK; }

        if (ctx->active == UINT32_MAX) { ctx->active = (uint32_t)TASKS_ACTIVE_FIRST; }
        else if (key->key == (uint8_t)LDG_TERM_KEY_UP) { if (ctx->active != 0) { ctx->active = ctx->active - (uint32_t)TASKS_STEP; } }
        else if (ctx->active + (uint32_t)TASKS_STEP < ctx->rows_cunt) { ctx->active = ctx->active + (uint32_t)TASKS_STEP; }

        return crosstalk_ui_window_row_show(&ctx->win, (uint32_t)TASKS_ROW_LIST + ctx->active);
    }

    if (key->key == (uint8_t)LDG_TERM_KEY_ENTER)
    {
        if (len != 0) { ret = tasks_add_apply(ctx); }
        else if (ctx->active != UINT32_MAX) { ret = tasks_action_apply(ctx, ctx->active); }

        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "reng: task key failed; err: %u32", ret); }

        return LDG_ERR_AOK;
    }

    if (key->key == (uint8_t)LDG_TERM_KEY_ESCAPE)
    {
        ctx->entry[0] = LDG_STR_TERM;

        return LDG_ERR_AOK;
    }

    if (key->key == (uint8_t)LDG_TERM_KEY_BACKSPACE)
    {
        if (len != 0) { ctx->entry[len - (uint64_t)TASKS_STEP] = LDG_STR_TERM; }

        return LDG_ERR_AOK;
    }

    *consumed = LDG_TRUTH_FALSE;

    for (at = 0; at < key->text_cunt && at < (uint32_t)LDG_TERM_KEY_TEXT_MAX; at = at + (uint32_t)TASKS_STEP)
    {
        if (key->text[at] < (uint32_t)LDG_ASCII_PRINTABLE_MIN || key->text[at] > (uint32_t)LDG_ASCII_PRINTABLE_MAX) { continue; }

        *consumed = LDG_TRUTH_TRUE;

        if (len + (uint64_t)LDG_STR_TERM_SIZE >= (uint64_t)sizeof(ctx->entry)) { break; }

        ctx->entry[len] = (char)key->text[at];
        len = len + (uint64_t)TASKS_STEP;
        ctx->entry[len] = LDG_STR_TERM;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_reng_tasks_init(crosstalk_reng_tasks_ctx_t *ctx, crosstalk_reng_ctx_t *reng)
{
    if (LDG_UNLIKELY(ctx == 0x0 || reng == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->reng = reng;
    ctx->win.open = LDG_TRUTH_FALSE;
    ctx->win.scroll_on = LDG_TRUTH_TRUE;
    ctx->active = UINT32_MAX;
    ctx->gen_seen = UINT32_MAX;
    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_reng_tasks_open_set(crosstalk_reng_tasks_ctx_t *ctx, uint8_t open)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->win.open = open != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_reng_tasks_open_is(const crosstalk_reng_tasks_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->win.open != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_reng_tasks_reg(crosstalk_reng_tasks_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot)
{
    if (LDG_UNLIKELY(ctx == 0x0 || win_nav == 0x0 || out_slot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_ui_window_reg(win_nav, (uint64_t)CROSSTALK_RENG_TASKS_ID, CROSSTALK_RENG_TASKS_TITLE, &ctx->win, tasks_draw_cb, tasks_key_isr, ctx, out_slot);
}
