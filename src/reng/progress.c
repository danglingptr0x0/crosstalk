#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "reng/progress.h"
#include "ui/cell.h"
#include "ui/theme.h"

#define PROGRESS_STEP 1
#define PROGRESS_ORIG_COL 0
#define PROGRESS_LABEL_W 9
#define PROGRESS_RHS_GAP 1
#define PROGRESS_CURVE_ROWS 4
#define PROGRESS_CURVE_GAP_SAMPLES 1
#define PROGRESS_FILL_NONE 0
#define PROGRESS_TRACK_COL 0x202030FFU
#define PROGRESS_LINE_CAP 384
#define PROGRESS_LABEL_FMT "%-8.8s"
#define PROGRESS_NONE_TEXT "no project; enter through the Reverse notch on the main menu"
#define PROGRESS_EMPTY_TEXT "no samples yet; import symbols with re import"
#define PROGRESS_KIND_NONE "?"

static uint32_t progress_bar_draw(ldg_term_render_t *render, uint32_t row, uint32_t col_cunt, const char *label, uint32_t part, uint32_t total)
{
    char text[PROGRESS_LINE_CAP] = LDG_ARR_ZERO_INIT;
    char rhs[PROGRESS_LINE_CAP] = LDG_ARR_ZERO_INIT;
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t fill = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t track = LDG_STRUCT_ZERO_INIT;
    uint64_t rhs_len = 0;
    uint32_t bar_w = 0;
    uint32_t fill_w = 0;
    uint32_t ret = 0;

    if (snprintf(text, sizeof(text), PROGRESS_LABEL_FMT, label) < 0) { return LDG_ERR_STR_FMT; }

    if (snprintf(rhs, sizeof(rhs), "%u/%u", part, total) < 0) { return LDG_ERR_STR_FMT; }

    ret = ldg_str_len_get(rhs, (uint64_t)sizeof(rhs), &rhs_len);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)PROGRESS_ORIG_COL, row, text, (uint64_t)sizeof(text), (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, 0x0); }

    if (ret != LDG_ERR_AOK) { return ret; }

    if (col_cunt <= (uint32_t)PROGRESS_LABEL_W + (uint32_t)rhs_len + (uint32_t)PROGRESS_RHS_GAP) { return LDG_ERR_AOK; }

    bar_w = col_cunt - (uint32_t)PROGRESS_LABEL_W - (uint32_t)rhs_len - (uint32_t)PROGRESS_RHS_GAP;
    fill_w = total == 0 ? (uint32_t)PROGRESS_FILL_NONE : (uint32_t)(((uint64_t)bar_w * (uint64_t)part) / (uint64_t)total);

    ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, &fg);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_LIVE_RGBA, &fill); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)PROGRESS_TRACK_COL, &track); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_span_fill(render, (uint32_t)PROGRESS_LABEL_W, row, bar_w, &fg, &track); }

    if (ret == LDG_ERR_AOK && fill_w != 0) { ret = crosstalk_ui_cell_span_fill(render, (uint32_t)PROGRESS_LABEL_W, row, fill_w, &fg, &fill); }

    if (ret != LDG_ERR_AOK) { return ret; }

    return crosstalk_ui_cell_rgba_put(render, (uint32_t)PROGRESS_LABEL_W + bar_w + (uint32_t)PROGRESS_RHS_GAP, row, rhs, (uint64_t)sizeof(rhs), (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, 0x0);
}

static uint32_t progress_bars_draw(crosstalk_reng_progress_ctx_t *ctx, ldg_term_render_t *render, uint32_t proj, uint32_t col_cunt, uint32_t *row)
{
    const crosstalk_reng_sample_t *last = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0 || row == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (crosstalk_reng_sample_last_get(ctx->reng, proj, &last) != LDG_ERR_AOK || last == 0x0 || last->total == 0)
    {
        ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)PROGRESS_ORIG_COL, *row, PROGRESS_EMPTY_TEXT, (uint64_t)PROGRESS_LINE_CAP, (uint32_t)CROSSTALK_UI_THEME_DEAD_RGBA, 0x0);
        *row = *row + (uint32_t)PROGRESS_STEP;

        return ret;
    }

    ret = progress_bar_draw(render, *row, col_cunt, "named", last->named, last->total);
    if (ret == LDG_ERR_AOK) { ret = progress_bar_draw(render, *row + (uint32_t)PROGRESS_STEP, col_cunt, "ported", last->ported, last->total); }

    if (ret == LDG_ERR_AOK) { ret = progress_bar_draw(render, *row + (uint32_t)PROGRESS_STEP + (uint32_t)PROGRESS_STEP, col_cunt, "verified", last->verified, last->total); }

    *row = *row + (uint32_t)PROGRESS_STEP + (uint32_t)PROGRESS_STEP + (uint32_t)PROGRESS_STEP;

    return ret;
}

static uint32_t progress_curve_draw(crosstalk_reng_progress_ctx_t *ctx, ldg_term_render_t *render, uint32_t proj, uint32_t col_cunt, uint32_t *row)
{
    uint32_t cell_h = render->sample_row_cunt;
    uint32_t curve_w = col_cunt * render->sample_col_cunt;
    uint32_t curve_h = (uint32_t)PROGRESS_CURVE_ROWS * cell_h;
    uint32_t base_y = 0;
    uint32_t cunt = 0;
    uint32_t idx = 0;
    uint32_t shown = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0 || row == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < crosstalk_reng_sample_cunt_get(ctx->reng); idx = idx + (uint32_t)PROGRESS_STEP)
    {
        const crosstalk_reng_sample_t *sample = 0x0;

        if (crosstalk_reng_sample_get(ctx->reng, idx, &sample) == LDG_ERR_AOK && sample != 0x0 && sample->proj == proj && sample->total != 0) { cunt = cunt + (uint32_t)PROGRESS_STEP; }
    }

    if (cunt == 0 || curve_w == 0) { return LDG_ERR_AOK; }

    base_y = *row * cell_h + curve_h;

    ret = crosstalk_ui_cell_sample_fill(render, (uint32_t)PROGRESS_ORIG_COL, *row * cell_h, curve_w, curve_h, (uint32_t)CROSSTALK_UI_THEME_DEAD_RGBA);
    if (ret != LDG_ERR_AOK) { return ret; }

    for (idx = 0; idx < crosstalk_reng_sample_cunt_get(ctx->reng); idx = idx + (uint32_t)PROGRESS_STEP)
    {
        const crosstalk_reng_sample_t *sample = 0x0;
        uint32_t x_lhs = 0;
        uint32_t bar_w = 0;
        uint32_t named_h = 0;
        uint32_t ported_h = 0;
        uint32_t verified_h = 0;

        if (crosstalk_reng_sample_get(ctx->reng, idx, &sample) != LDG_ERR_AOK || sample == 0x0 || sample->proj != proj || sample->total == 0) { continue; }

        x_lhs = (uint32_t)(((uint64_t)shown * (uint64_t)curve_w) / (uint64_t)cunt);
        bar_w = (uint32_t)((((uint64_t)shown + (uint64_t)PROGRESS_STEP) * (uint64_t)curve_w) / (uint64_t)cunt) - x_lhs;
        shown = shown + (uint32_t)PROGRESS_STEP;

        if (bar_w > (uint32_t)PROGRESS_CURVE_GAP_SAMPLES) { bar_w = bar_w - (uint32_t)PROGRESS_CURVE_GAP_SAMPLES; }

        if (bar_w == 0) { continue; }

        named_h = (uint32_t)(((uint64_t)curve_h * (uint64_t)sample->named) / (uint64_t)sample->total);
        ported_h = (uint32_t)(((uint64_t)curve_h * (uint64_t)sample->ported) / (uint64_t)sample->total);
        verified_h = (uint32_t)(((uint64_t)curve_h * (uint64_t)sample->verified) / (uint64_t)sample->total);

        if (named_h != 0) { ret = crosstalk_ui_cell_sample_fill(render, x_lhs, base_y - named_h, bar_w, named_h, (uint32_t)CROSSTALK_UI_THEME_DEAD_RGBA); }

        if (ret == LDG_ERR_AOK && ported_h != 0) { ret = crosstalk_ui_cell_sample_fill(render, x_lhs, base_y - ported_h, bar_w, ported_h, (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA); }

        if (ret == LDG_ERR_AOK && verified_h != 0) { ret = crosstalk_ui_cell_sample_fill(render, x_lhs, base_y - verified_h, bar_w, verified_h, (uint32_t)CROSSTALK_UI_THEME_LIVE_RGBA); }

        if (ret != LDG_ERR_AOK) { return ret; }
    }

    *row = *row + (uint32_t)PROGRESS_CURVE_ROWS;

    return LDG_ERR_AOK;
}

static uint32_t progress_draw_cb(uint64_t win_id, void *user, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    crosstalk_reng_progress_ctx_t *ctx = (crosstalk_reng_progress_ctx_t *)user;
    const crosstalk_reng_proj_t *proj = 0x0;
    const char *kind = 0x0;
    char line[PROGRESS_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t row = 0;
    uint32_t sel = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE || ctx->reng == 0x0) { return LDG_ERR_NOT_INIT; }

    if (win_id != (uint64_t)CROSSTALK_RENG_PROGRESS_ID || col_cunt == 0 || row_cunt == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    sel = crosstalk_reng_sel_get(ctx->reng);

    if (sel == (uint32_t)UINT32_MAX || crosstalk_reng_proj_get(ctx->reng, sel, &proj) != LDG_ERR_AOK || proj == 0x0)
    {
        ctx->win.content_row_cunt = (uint32_t)PROGRESS_STEP;

        return crosstalk_ui_cell_rgba_put(render, (uint32_t)PROGRESS_ORIG_COL, row, PROGRESS_NONE_TEXT, (uint64_t)PROGRESS_LINE_CAP, (uint32_t)CROSSTALK_UI_THEME_DEAD_RGBA, 0x0);
    }

    if (crosstalk_reng_kind_name_get(proj->kind, &kind) != LDG_ERR_AOK) { kind = PROGRESS_KIND_NONE; }

    if (snprintf(line, sizeof(line), "%.63s; %.7s", proj->name, kind) < 0) { return LDG_ERR_STR_FMT; }

    ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)PROGRESS_ORIG_COL, row, line, (uint64_t)sizeof(line), (uint32_t)CROSSTALK_UI_THEME_LIVE_RGBA, 0x0);
    if (ret != LDG_ERR_AOK) { return ret; }

    row = row + (uint32_t)PROGRESS_STEP + (uint32_t)PROGRESS_STEP;

    ret = progress_bars_draw(ctx, render, sel, col_cunt, &row);
    if (ret != LDG_ERR_AOK) { return ret; }

    row = row + (uint32_t)PROGRESS_STEP;

    ret = progress_curve_draw(ctx, render, sel, col_cunt, &row);

    ctx->win.content_row_cunt = row;

    return ret;
}

uint32_t crosstalk_reng_progress_init(crosstalk_reng_progress_ctx_t *ctx, crosstalk_reng_ctx_t *reng)
{
    if (LDG_UNLIKELY(ctx == 0x0 || reng == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->reng = reng;
    ctx->win.open = LDG_TRUTH_FALSE;
    ctx->win.scroll_on = LDG_TRUTH_TRUE;
    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_reng_progress_open_set(crosstalk_reng_progress_ctx_t *ctx, uint8_t open)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->win.open = open != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_reng_progress_open_is(const crosstalk_reng_progress_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->win.open != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_reng_progress_reg(crosstalk_reng_progress_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot)
{
    if (LDG_UNLIKELY(ctx == 0x0 || win_nav == 0x0 || out_slot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_ui_window_reg(win_nav, (uint64_t)CROSSTALK_RENG_PROGRESS_ID, CROSSTALK_RENG_PROGRESS_TITLE, &ctx->win, progress_draw_cb, 0x0, ctx, out_slot);
}
