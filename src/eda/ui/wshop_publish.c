#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "eda/ui/wshop_publish.h"
#include "ui/cell.h"
#include "ui/theme.h"

#define WSHOP_PUBLISH_STEP 1
#define WSHOP_PUBLISH_ISA_DEFAULT 0
#define WSHOP_PUBLISH_LABEL_CAP 96
#define WSHOP_PUBLISH_WINDOW_TITLE "Share this design"
#define WSHOP_PUBLISH_BTN_TEXT "Publish to Workshop"
#define WSHOP_PUBLISH_ORIG_COL 0
#define WSHOP_PUBLISH_FIELD_TITLE 0
#define WSHOP_PUBLISH_FIELD_ISA 1
#define WSHOP_PUBLISH_FIELD_LICENSE 2
#define WSHOP_PUBLISH_FIELD_GO 3
#define WSHOP_PUBLISH_FIELD_CUNT 4
#define WSHOP_PUBLISH_STATUS_GAP 1
#define WSHOP_PUBLISH_CURSOR_W 1

static const char* publish_isa_text_get(uint32_t idx)
{
    const char *tbl[CROSSTALK_EDA_UI_WSHOP_PUBLISH_ISA_CUNT] = { "DANGCSA", "i386", "AMD64", "RISC-V", "8086", "8088", "SPARC V8", "SPARC V9", "Custom" };

    if (idx >= (uint32_t)CROSSTALK_EDA_UI_WSHOP_PUBLISH_ISA_CUNT) { return tbl[WSHOP_PUBLISH_ISA_DEFAULT]; }

    return tbl[idx];
}

uint32_t crosstalk_eda_ui_wshop_publish_init(crosstalk_eda_ui_wshop_publish_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->field = (uint32_t)WSHOP_PUBLISH_FIELD_TITLE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_wshop_publish_open_set(crosstalk_eda_ui_wshop_publish_ctx_t *ctx, uint8_t open, const char *seed_title)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->open = open != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    ctx->win.open = ctx->open;

    if (ctx->open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ctx->field = (uint32_t)WSHOP_PUBLISH_FIELD_TITLE;

    if (seed_title != 0x0 && ctx->title[0] == LDG_STR_TERM) { if (snprintf(ctx->title, sizeof(ctx->title), "%.127s", seed_title) < 0) { ctx->title[0] = LDG_STR_TERM; } }

    return LDG_ERR_AOK;
}

const char* crosstalk_eda_ui_wshop_publish_isa_get(const crosstalk_eda_ui_wshop_publish_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return publish_isa_text_get(WSHOP_PUBLISH_ISA_DEFAULT); }

    return publish_isa_text_get(ctx->isa_idx);
}

static uint32_t wshop_publish_field_draw(const crosstalk_eda_ui_wshop_publish_ctx_t *ctx, ldg_term_render_t *render, uint32_t field, const char *text, uint64_t cap, uint32_t col_cunt, uint8_t edit_is)
{
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t bg = LDG_STRUCT_ZERO_INIT;
    uint64_t len = 0;
    uint32_t ret = 0;

    ret = ldg_str_len_get(text, cap, &len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->field != field) { return crosstalk_ui_cell_rgba_put(render, (uint32_t)WSHOP_PUBLISH_ORIG_COL, field, text, cap, field == (uint32_t)WSHOP_PUBLISH_FIELD_GO ? (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA : (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, 0x0); }

    ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_LIT_RGBA, &fg);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_SEL_BG_RGBA, &bg); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_span_fill(render, (uint32_t)WSHOP_PUBLISH_ORIG_COL, field, col_cunt, &fg, &bg); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_text_put(render, (uint32_t)WSHOP_PUBLISH_ORIG_COL, field, text, cap, &fg, &bg); }

    if (ret != LDG_ERR_AOK || edit_is != LDG_TRUTH_TRUE) { return ret; }

    return crosstalk_ui_cell_span_fill(render, (uint32_t)WSHOP_PUBLISH_ORIG_COL + (uint32_t)len, field, (uint32_t)WSHOP_PUBLISH_CURSOR_W, &bg, &fg);
}

static uint32_t wshop_publish_draw_cb(uint64_t win_id, void *user, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    crosstalk_eda_ui_wshop_publish_ctx_t *ctx = (crosstalk_eda_ui_wshop_publish_ctx_t *)user;
    char isa_label[WSHOP_PUBLISH_LABEL_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t status_row = (uint32_t)WSHOP_PUBLISH_FIELD_CUNT + (uint32_t)WSHOP_PUBLISH_STATUS_GAP;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (win_id != (uint64_t)CROSSTALK_EDA_UI_WSHOP_PUBLISH_WINDOW_ID || col_cunt == 0 || row_cunt == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (snprintf(isa_label, sizeof(isa_label), "ISA: %.31s", publish_isa_text_get(ctx->isa_idx)) < 0) { return LDG_ERR_IO_FMT; }

    ret = wshop_publish_field_draw(ctx, render, (uint32_t)WSHOP_PUBLISH_FIELD_TITLE, ctx->title, (uint64_t)sizeof(ctx->title), col_cunt, LDG_TRUTH_TRUE);
    if (ret == LDG_ERR_AOK) { ret = wshop_publish_field_draw(ctx, render, (uint32_t)WSHOP_PUBLISH_FIELD_ISA, isa_label, (uint64_t)sizeof(isa_label), col_cunt, LDG_TRUTH_FALSE); }

    if (ret == LDG_ERR_AOK) { ret = wshop_publish_field_draw(ctx, render, (uint32_t)WSHOP_PUBLISH_FIELD_LICENSE, ctx->license, (uint64_t)sizeof(ctx->license), col_cunt, LDG_TRUTH_TRUE); }

    if (ret == LDG_ERR_AOK) { ret = wshop_publish_field_draw(ctx, render, (uint32_t)WSHOP_PUBLISH_FIELD_GO, WSHOP_PUBLISH_BTN_TEXT, (uint64_t)WSHOP_PUBLISH_LABEL_CAP, col_cunt, LDG_TRUTH_FALSE); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->win.content_row_cunt = status_row + (uint32_t)WSHOP_PUBLISH_STEP;

    return crosstalk_ui_cell_rgba_put(render, (uint32_t)WSHOP_PUBLISH_ORIG_COL, status_row, ctx->status_text != 0x0 ? ctx->status_text : WSHOP_PUBLISH_WINDOW_TITLE, (uint64_t)WSHOP_PUBLISH_LABEL_CAP, (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, 0x0);
}

static uint32_t wshop_publish_text_key(char *text, uint64_t cap, const ldg_term_key_event_t *key, uint8_t *consumed)
{
    uint64_t len = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    ret = ldg_str_len_get(text, cap, &len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (key->key == (uint8_t)LDG_TERM_KEY_BACKSPACE)
    {
        if (len != 0) { text[len - (uint64_t)WSHOP_PUBLISH_STEP] = LDG_STR_TERM; }

        *consumed = LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    for (at = 0; at < key->text_cunt && at < (uint32_t)LDG_TERM_KEY_TEXT_MAX; at = at + (uint32_t)WSHOP_PUBLISH_STEP)
    {
        if (key->text[at] < (uint32_t)LDG_ASCII_PRINTABLE_MIN || key->text[at] > (uint32_t)LDG_ASCII_PRINTABLE_MAX) { continue; }

        *consumed = LDG_TRUTH_TRUE;

        if (len + (uint64_t)LDG_STR_TERM_SIZE >= cap) { break; }

        text[len] = (char)key->text[at];
        len = len + (uint64_t)WSHOP_PUBLISH_STEP;
        text[len] = LDG_STR_TERM;
    }

    return LDG_ERR_AOK;
}

static uint32_t wshop_publish_key_isr(uint64_t win_id, void *user, const ldg_term_key_event_t *key, uint8_t *consumed)
{
    crosstalk_eda_ui_wshop_publish_ctx_t *ctx = (crosstalk_eda_ui_wshop_publish_ctx_t *)user;

    if (LDG_UNLIKELY(ctx == 0x0 || key == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    if (win_id != (uint64_t)CROSSTALK_EDA_UI_WSHOP_PUBLISH_WINDOW_ID) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (key->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE) { return LDG_ERR_AOK; }

    if ((key->mods & ((uint32_t)LDG_TERM_MOD_CTRL | (uint32_t)LDG_TERM_MOD_ALT | (uint32_t)LDG_TERM_MOD_SUPER)) != 0) { return LDG_ERR_AOK; }

    if (key->key == (uint8_t)LDG_TERM_KEY_UP || key->key == (uint8_t)LDG_TERM_KEY_DOWN || key->key == (uint8_t)LDG_TERM_KEY_TAB)
    {
        if (key->key == (uint8_t)LDG_TERM_KEY_UP) { ctx->field = ctx->field == 0 ? (uint32_t)WSHOP_PUBLISH_FIELD_CUNT - (uint32_t)WSHOP_PUBLISH_STEP : ctx->field - (uint32_t)WSHOP_PUBLISH_STEP; }
        else { ctx->field = (ctx->field + (uint32_t)WSHOP_PUBLISH_STEP) % (uint32_t)WSHOP_PUBLISH_FIELD_CUNT; }

        *consumed = LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    if (key->key == (uint8_t)LDG_TERM_KEY_ENTER)
    {
        if (ctx->field == (uint32_t)WSHOP_PUBLISH_FIELD_GO) { if (ctx->title[0] != LDG_STR_TERM) { ctx->out_publish = LDG_TRUTH_TRUE; } }
        else { ctx->field = ctx->field + (uint32_t)WSHOP_PUBLISH_STEP; }

        *consumed = LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    if (ctx->field == (uint32_t)WSHOP_PUBLISH_FIELD_ISA && (key->key == (uint8_t)LDG_TERM_KEY_LHS || key->key == (uint8_t)LDG_TERM_KEY_RHS))
    {
        if (key->key == (uint8_t)LDG_TERM_KEY_LHS) { ctx->isa_idx = ctx->isa_idx == 0 ? (uint32_t)CROSSTALK_EDA_UI_WSHOP_PUBLISH_ISA_CUNT - (uint32_t)WSHOP_PUBLISH_STEP : ctx->isa_idx - (uint32_t)WSHOP_PUBLISH_STEP; }
        else { ctx->isa_idx = (ctx->isa_idx + (uint32_t)WSHOP_PUBLISH_STEP) % (uint32_t)CROSSTALK_EDA_UI_WSHOP_PUBLISH_ISA_CUNT; }

        *consumed = LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    if (ctx->field == (uint32_t)WSHOP_PUBLISH_FIELD_TITLE) { return wshop_publish_text_key(ctx->title, (uint64_t)sizeof(ctx->title), key, consumed); }

    if (ctx->field == (uint32_t)WSHOP_PUBLISH_FIELD_LICENSE) { return wshop_publish_text_key(ctx->license, (uint64_t)sizeof(ctx->license), key, consumed); }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_wshop_publish_win_reg(crosstalk_eda_ui_wshop_publish_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot)
{
    if (LDG_UNLIKELY(ctx == 0x0 || win_nav == 0x0 || out_slot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_ui_window_reg(win_nav, (uint64_t)CROSSTALK_EDA_UI_WSHOP_PUBLISH_WINDOW_ID, WSHOP_PUBLISH_WINDOW_TITLE, &ctx->win, wshop_publish_draw_cb, wshop_publish_key_isr, ctx, out_slot);
}

uint32_t crosstalk_eda_ui_wshop_publish_frame_set(crosstalk_eda_ui_wshop_publish_ctx_t *ctx, const char *status_text)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->status_text = status_text;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_wshop_publish_out_get(crosstalk_eda_ui_wshop_publish_ctx_t *ctx, uint8_t *out_publish)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_publish == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_publish = ctx->out_publish;
    ctx->out_publish = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}
