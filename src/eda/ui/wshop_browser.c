#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "eda/ui/wshop_browser.h"
#include "ui/cell.h"
#include "ui/theme.h"

#define WSHOP_BROWSER_STEP 1
#define WSHOP_BROWSER_ORIG_COL 0
#define WSHOP_BROWSER_ROW_KIND 0
#define WSHOP_BROWSER_ROW_SHARE 1
#define WSHOP_BROWSER_ROW_LIST 3
#define WSHOP_BROWSER_STATUS_GAP 1
#define WSHOP_BROWSER_CELL_GAP 2
#define WSHOP_BROWSER_STATUS_CAP 64
#define WSHOP_BROWSER_TITLE "Workshop"
#define WSHOP_BROWSER_SHARE_LABEL "Share this design"
#define WSHOP_BROWSER_MARK_NONE " "
#define WSHOP_BROWSER_MARK_MOUNTED ">"
#define WSHOP_BROWSER_MARK_BAD "x"
#define WSHOP_BROWSER_MARK_NO_CORE "?"
#define WSHOP_BROWSER_MOUNT_NONE 0
#define WSHOP_BROWSER_CORE_LABEL "Showing cores"
#define WSHOP_BROWSER_SOFT_LABEL "Showing software"
#define WSHOP_BROWSER_KEY_SHARE 's'
#define WSHOP_BROWSER_ACTIVE_FIRST 0
#define WSHOP_BROWSER_CORE_ROW_FMT "%.3s %-35.35s %-11.11s %.23s"
#define WSHOP_BROWSER_SOFT_ROW_FMT "%.3s %-35.35s %-11.11s %-11.11s %.11s"

static const char* browser_role_text_get(uint32_t role)
{
    if (role == CROSSTALK_WSHOP_MANIFEST_ROLE_FIRMWARE) { return "firmware"; }

    if (role == CROSSTALK_WSHOP_MANIFEST_ROLE_BIOS) { return "bios"; }

    if (role == CROSSTALK_WSHOP_MANIFEST_ROLE_KERNEL) { return "kernel"; }

    if (role == CROSSTALK_WSHOP_MANIFEST_ROLE_OS) { return "os"; }

    if (role == CROSSTALK_WSHOP_MANIFEST_ROLE_USERLAND) { return "app"; }

    return "image";
}

static const char* browser_core_mark_get(const crosstalk_wshop_item_t *item, const crosstalk_wshop_item_t *mounted)
{
    if (mounted == 0x0 || mounted->ok != LDG_TRUTH_TRUE) { return WSHOP_BROWSER_MARK_NONE; }

    if (mounted->file_id == item->file_id) { return WSHOP_BROWSER_MARK_MOUNTED; }

    return WSHOP_BROWSER_MARK_NONE;
}

static const char* browser_img_mark_get(const crosstalk_wshop_item_t *item, const crosstalk_wshop_item_t *mounted)
{
    if (mounted == 0x0 || mounted->ok != LDG_TRUTH_TRUE) { return WSHOP_BROWSER_MARK_NO_CORE; }

    if (crosstalk_wshop_manifest_isa_compat_is(&mounted->manifest, &item->manifest) != LDG_TRUTH_TRUE) { return WSHOP_BROWSER_MARK_BAD; }

    return WSHOP_BROWSER_MARK_NONE;
}

static uint32_t items_refresh(crosstalk_eda_ui_wshop_browser_ctx_t *ctx, const crosstalk_wshop_list_t *list, const crosstalk_wshop_item_t *mounted)
{
    const crosstalk_wshop_item_t *item = 0x0;
    uint32_t idx = 0;
    int32_t written = 0;

    ctx->rows_cunt = 0;

    for (idx = 0; idx < list->cunt && ctx->rows_cunt < CROSSTALK_WSHOP_ITEM_MAX; idx = idx + WSHOP_BROWSER_STEP)
    {
        char *label = ctx->row_label[ctx->rows_cunt];

        item = &list->item[idx];

        if (item->manifest.kind != ctx->kind) { continue; }

        if (ctx->kind == CROSSTALK_WSHOP_MANIFEST_KIND_CORE) { written = snprintf(label, (uint64_t)CROSSTALK_EDA_UI_WSHOP_BROWSER_LABEL_CAP, WSHOP_BROWSER_CORE_ROW_FMT, browser_core_mark_get(item, mounted), item->manifest.name, item->manifest.isa, item->manifest.author); }
        else { written = snprintf(label, (uint64_t)CROSSTALK_EDA_UI_WSHOP_BROWSER_LABEL_CAP, WSHOP_BROWSER_SOFT_ROW_FMT, browser_img_mark_get(item, mounted), item->manifest.name, item->manifest.tgt_isa, browser_role_text_get(item->manifest.role), item->manifest.author); }

        if (written < 0) { break; }

        ctx->row_item[ctx->rows_cunt] = idx;
        ctx->rows_cunt = ctx->rows_cunt + WSHOP_BROWSER_STEP;
    }

    if (ctx->active != UINT32_MAX && ctx->active >= ctx->rows_cunt) { ctx->active = ctx->rows_cunt == 0 ? UINT32_MAX : ctx->rows_cunt - WSHOP_BROWSER_STEP; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_wshop_browser_init(crosstalk_eda_ui_wshop_browser_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->active = UINT32_MAX;
    ctx->kind = CROSSTALK_WSHOP_MANIFEST_KIND_CORE;
    ctx->dirty = LDG_TRUTH_TRUE;
    ctx->win.scroll_on = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_wshop_browser_open_set(crosstalk_eda_ui_wshop_browser_ctx_t *ctx, uint8_t open)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->open = open != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    ctx->win.open = ctx->open;

    if (ctx->open == LDG_TRUTH_TRUE) { ctx->dirty = LDG_TRUTH_TRUE; }

    return LDG_ERR_AOK;
}

static uint32_t wshop_browser_status_draw(const crosstalk_eda_ui_wshop_browser_ctx_t *ctx, ldg_term_render_t *render, uint32_t row)
{
    char status[WSHOP_BROWSER_STATUS_CAP] = LDG_ARR_ZERO_INIT;
    char tally[WSHOP_BROWSER_STATUS_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t status_w = 0;
    uint32_t ret = 0;

    if (snprintf(status, sizeof(status), "cores: %u | software: %u", crosstalk_wshop_list_cunt_get(ctx->list, CROSSTALK_WSHOP_MANIFEST_KIND_CORE), crosstalk_wshop_list_cunt_get(ctx->list, CROSSTALK_WSHOP_MANIFEST_KIND_SOFTWARE)) < 0) { return LDG_ERR_IO_FMT; }

    if (snprintf(tally, sizeof(tally), "shown: %u", ctx->rows_cunt) < 0) { return LDG_ERR_IO_FMT; }

    ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)WSHOP_BROWSER_ORIG_COL, row, status, (uint64_t)sizeof(status), (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, &status_w);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_ui_cell_rgba_put(render, status_w + (uint32_t)WSHOP_BROWSER_CELL_GAP, row, tally, (uint64_t)sizeof(tally), (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, 0x0);
}

static uint32_t wshop_browser_draw_cb(uint64_t win_id, void *user, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    crosstalk_eda_ui_wshop_browser_ctx_t *ctx = (crosstalk_eda_ui_wshop_browser_ctx_t *)user;
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t bg = LDG_STRUCT_ZERO_INIT;
    uint64_t mount_id = WSHOP_BROWSER_MOUNT_NONE;
    uint32_t row = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->list == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (win_id != (uint64_t)CROSSTALK_EDA_UI_WSHOP_BROWSER_WINDOW_ID || col_cunt == 0 || row_cunt == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->mounted != 0x0) { mount_id = ctx->mounted->file_id; }

    if (ctx->seen_cunt != ctx->list->cunt)
    {
        ctx->seen_cunt = ctx->list->cunt;
        ctx->dirty = LDG_TRUTH_TRUE;
    }

    if (ctx->seen_mount_id != mount_id)
    {
        ctx->seen_mount_id = mount_id;
        ctx->dirty = LDG_TRUTH_TRUE;
    }

    if (ctx->dirty != LDG_TRUTH_FALSE)
    {
        ret = items_refresh(ctx, ctx->list, ctx->mounted);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ctx->dirty = LDG_TRUTH_FALSE;
    }

    ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)WSHOP_BROWSER_ORIG_COL, (uint32_t)WSHOP_BROWSER_ROW_KIND, ctx->kind == CROSSTALK_WSHOP_MANIFEST_KIND_CORE ? WSHOP_BROWSER_CORE_LABEL : WSHOP_BROWSER_SOFT_LABEL, (uint64_t)CROSSTALK_EDA_UI_WSHOP_BROWSER_LABEL_CAP, (uint32_t)CROSSTALK_UI_THEME_LIT_RGBA, 0x0);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)WSHOP_BROWSER_ORIG_COL, (uint32_t)WSHOP_BROWSER_ROW_SHARE, WSHOP_BROWSER_SHARE_LABEL, (uint64_t)CROSSTALK_EDA_UI_WSHOP_BROWSER_LABEL_CAP, (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, 0x0);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_LIT_RGBA, &fg);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_SEL_BG_RGBA, &bg); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (row = 0; row < ctx->rows_cunt; row = row + WSHOP_BROWSER_STEP)
    {
        if (row == ctx->active)
        {
            ret = crosstalk_ui_cell_span_fill(render, (uint32_t)WSHOP_BROWSER_ORIG_COL, (uint32_t)WSHOP_BROWSER_ROW_LIST + row, col_cunt, &fg, &bg);
            if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_text_put(render, (uint32_t)WSHOP_BROWSER_ORIG_COL, (uint32_t)WSHOP_BROWSER_ROW_LIST + row, ctx->row_label[row], (uint64_t)CROSSTALK_EDA_UI_WSHOP_BROWSER_LABEL_CAP, &fg, &bg); }
        }
        else { ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)WSHOP_BROWSER_ORIG_COL, (uint32_t)WSHOP_BROWSER_ROW_LIST + row, ctx->row_label[row], (uint64_t)CROSSTALK_EDA_UI_WSHOP_BROWSER_LABEL_CAP, (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, 0x0); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    row = (uint32_t)WSHOP_BROWSER_ROW_LIST + ctx->rows_cunt + (uint32_t)WSHOP_BROWSER_STATUS_GAP;
    ctx->win.content_row_cunt = row + WSHOP_BROWSER_STEP;

    return wshop_browser_status_draw(ctx, render, row);
}

static uint32_t wshop_browser_key_isr(uint64_t win_id, void *user, const ldg_term_key_event_t *key, uint8_t *consumed)
{
    crosstalk_eda_ui_wshop_browser_ctx_t *ctx = (crosstalk_eda_ui_wshop_browser_ctx_t *)user;

    if (LDG_UNLIKELY(ctx == 0x0 || key == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    if (win_id != (uint64_t)CROSSTALK_EDA_UI_WSHOP_BROWSER_WINDOW_ID) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (key->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE || ctx->list == 0x0) { return LDG_ERR_AOK; }

    if ((key->mods & ((uint32_t)LDG_TERM_MOD_CTRL | (uint32_t)LDG_TERM_MOD_ALT | (uint32_t)LDG_TERM_MOD_SUPER)) != 0) { return LDG_ERR_AOK; }

    *consumed = LDG_TRUTH_TRUE;

    if (key->key == (uint8_t)LDG_TERM_KEY_TAB)
    {
        ctx->kind = ctx->kind == CROSSTALK_WSHOP_MANIFEST_KIND_CORE ? CROSSTALK_WSHOP_MANIFEST_KIND_SOFTWARE : CROSSTALK_WSHOP_MANIFEST_KIND_CORE;
        ctx->active = UINT32_MAX;
        ctx->win.scroll_row = 0;

        return items_refresh(ctx, ctx->list, ctx->mounted);
    }

    if (key->key == (uint8_t)LDG_TERM_KEY_UP || key->key == (uint8_t)LDG_TERM_KEY_DOWN)
    {
        if (ctx->rows_cunt == 0) { return LDG_ERR_AOK; }

        if (ctx->active == UINT32_MAX) { ctx->active = (uint32_t)WSHOP_BROWSER_ACTIVE_FIRST; }
        else if (key->key == (uint8_t)LDG_TERM_KEY_UP) { if (ctx->active != 0) { ctx->active = ctx->active - WSHOP_BROWSER_STEP; } }
        else if (ctx->active + WSHOP_BROWSER_STEP < ctx->rows_cunt) { ctx->active = ctx->active + WSHOP_BROWSER_STEP; }

        return crosstalk_ui_window_row_show(&ctx->win, (uint32_t)WSHOP_BROWSER_ROW_LIST + ctx->active);
    }

    if (key->key == (uint8_t)LDG_TERM_KEY_ENTER)
    {
        if (ctx->active != UINT32_MAX && ctx->active < ctx->rows_cunt) { ctx->out_mount = &ctx->list->item[ctx->row_item[ctx->active]]; }

        return LDG_ERR_AOK;
    }

    if (key->text_cunt != 0 && key->text[0] == (uint32_t)WSHOP_BROWSER_KEY_SHARE)
    {
        ctx->out_share_req = LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    *consumed = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_wshop_browser_win_reg(crosstalk_eda_ui_wshop_browser_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot)
{
    if (LDG_UNLIKELY(ctx == 0x0 || win_nav == 0x0 || out_slot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_ui_window_reg(win_nav, (uint64_t)CROSSTALK_EDA_UI_WSHOP_BROWSER_WINDOW_ID, WSHOP_BROWSER_TITLE, &ctx->win, wshop_browser_draw_cb, wshop_browser_key_isr, ctx, out_slot);
}

uint32_t crosstalk_eda_ui_wshop_browser_frame_set(crosstalk_eda_ui_wshop_browser_ctx_t *ctx, const crosstalk_wshop_list_t *list, const crosstalk_wshop_item_t *mounted)
{
    if (LDG_UNLIKELY(ctx == 0x0 || list == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->list = list;
    ctx->mounted = mounted;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_wshop_browser_out_get(crosstalk_eda_ui_wshop_browser_ctx_t *ctx, const crosstalk_wshop_item_t **out_mount, uint8_t *out_share_req)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_mount == 0x0 || out_share_req == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_mount = ctx->out_mount;
    *out_share_req = ctx->out_share_req;
    ctx->out_mount = 0x0;
    ctx->out_share_req = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}
