#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/logging/logging.h>

#include <dangling/arch/dangcsa/dangcsa.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "eda/ui/progview.h"
#include "ui/cell.h"
#include "ui/theme.h"

#define PROGVIEW_STEP 1
#define PROGVIEW_ORIG_COL 0
#define PROGVIEW_HDR_ROW 0
#define PROGVIEW_BODY_ROW 2
#define PROGVIEW_DEAD_TEXT "no running design"
#define PROGVIEW_SIG_DEFAULT "cells"
#define PROGVIEW_HDR_FMT "%.31s loc 0x%08" PRIX64 " depth %" PRIu64
#define PROGVIEW_ROW_FMT "%c %08" PRIX64 " %08" PRIX64 " %.39s"
#define PROGVIEW_LINE_CAP 112
#define PROGVIEW_MARK_CH '>'
#define PROGVIEW_PLAIN_CH ' '
#define PROGVIEW_FOLLOW_LEAD 8
#define PROGVIEW_ASM_FAIL_TEXT "??"

static uint32_t progview_base_get(const crosstalk_eda_ui_progview_ctx_t *ctx, uint64_t *out_word, uint64_t *out_base, uint64_t *out_width, uint8_t *out_live)
{
    uint64_t loc = 0;
    uint64_t off = 0;
    uint64_t word = 0;
    uint64_t base = 0;
    uint64_t width = 0;
    uint32_t elem_width = 0;
    uint32_t ret = 0;

    *out_word = 0;
    *out_base = 0;
    *out_width = 0;
    *out_live = LDG_TRUTH_FALSE;

    ret = crosstalk_eda_ic_fpga_mem_width_get(ctx->fpga, ctx->slot, ctx->sig, &width, &elem_width);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (width == 0) { return LDG_ERR_EMPTY; }

    if (crosstalk_eda_ic_fpga_sig_peek(ctx->fpga, ctx->slot, ctx->loc_sig, &loc, &elem_width) == LDG_ERR_AOK)
    {
        if (crosstalk_eda_ic_fpga_mem_offset_get(ctx->fpga, ctx->slot, ctx->sig, &off) != LDG_ERR_AOK) { off = 0; }

        if (loc >= off) { word = (loc - off) / (uint64_t)LDG_DANGCSA_INSTR_SIZE; }

        if (word >= width && LDG_IS_POW2(width)) { word = word & (width - 0x1); }

        if (word < width) { *out_live = LDG_TRUTH_TRUE; }
        else { word = 0; }
    }

    if (*out_live == LDG_TRUTH_TRUE && word > (uint64_t)PROGVIEW_FOLLOW_LEAD) { base = word - (uint64_t)PROGVIEW_FOLLOW_LEAD; }

    if (width > (uint64_t)CROSSTALK_EDA_UI_PROGVIEW_ROW_CUNT && base + (uint64_t)CROSSTALK_EDA_UI_PROGVIEW_ROW_CUNT > width) { base = width - (uint64_t)CROSSTALK_EDA_UI_PROGVIEW_ROW_CUNT; }

    *out_word = word;
    *out_base = base;
    *out_width = width;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_progview_init(crosstalk_eda_ui_progview_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->win.scroll_on = LDG_TRUTH_TRUE;
    ctx->win.open = LDG_TRUTH_FALSE;
    ctx->open = LDG_TRUTH_FALSE;
    ctx->ok = LDG_TRUTH_TRUE;

    if (ldg_str_rbrcpy(ctx->sig, PROGVIEW_SIG_DEFAULT, sizeof(ctx->sig)) != LDG_ERR_AOK) { return LDG_ERR_STR_TRUNC; }

    if (ldg_str_rbrcpy(ctx->loc_sig, CROSSTALK_EDA_IC_FPGA_PROG_SIG, sizeof(ctx->loc_sig)) != LDG_ERR_AOK) { return LDG_ERR_STR_TRUNC; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_progview_shutdown(crosstalk_eda_ui_progview_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->ok = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_progview_bind(crosstalk_eda_ui_progview_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot)
{
    if (LDG_UNLIKELY(ctx == 0x0 || fpga == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->fpga = fpga;
    ctx->slot = slot;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_progview_sig_set(crosstalk_eda_ui_progview_ctx_t *ctx, const char *sig_name)
{
    if (LDG_UNLIKELY(ctx == 0x0 || sig_name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (sig_name[0] == LDG_STR_TERM) { return LDG_ERR_STR_EMPTY; }

    return ldg_str_rbrcpy(ctx->sig, sig_name, sizeof(ctx->sig));
}

uint32_t crosstalk_eda_ui_progview_open_set(crosstalk_eda_ui_progview_ctx_t *ctx, uint8_t open)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->open = open;
    ctx->win.open = open;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_ui_progview_open_is(const crosstalk_eda_ui_progview_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->win.open != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t progview_dead_draw(ldg_term_render_t *render, uint32_t row)
{
    if (LDG_UNLIKELY(render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_ui_cell_rgba_put(render, (uint32_t)PROGVIEW_ORIG_COL, row, PROGVIEW_DEAD_TEXT, (uint64_t)sizeof(PROGVIEW_DEAD_TEXT), (uint32_t)CROSSTALK_UI_THEME_DEAD_RGBA, 0x0);
}

static uint32_t progview_draw_cb(uint64_t win_id, void *user, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    crosstalk_eda_ui_progview_ctx_t *ctx = (crosstalk_eda_ui_progview_ctx_t *)user;
    uint64_t words[CROSSTALK_EDA_UI_PROGVIEW_ROW_CUNT] = LDG_ARR_ZERO_INIT;
    char asm_text[CROSSTALK_EDA_UI_PROGVIEW_ASM_CAP] = LDG_ARR_ZERO_INIT;
    char line[PROGVIEW_LINE_CAP] = LDG_ARR_ZERO_INIT;
    ldg_dangcsa_decoded_t decoded = LDG_STRUCT_ZERO_INIT;
    uint32_t row = PROGVIEW_BODY_ROW;
    uint64_t word = 0;
    uint64_t base = 0;
    uint64_t width = 0;
    uint64_t addr = 0;
    uint32_t got = 0;
    uint32_t idx = 0;
    uint8_t live = LDG_TRUTH_FALSE;
    uint8_t at_loc = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (win_id != (uint64_t)CROSSTALK_EDA_UI_PROGVIEW_WINDOW_ID || col_cunt == 0 || row_cunt == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->fpga == 0x0) { return progview_dead_draw(render, (uint32_t)PROGVIEW_HDR_ROW); }

    if (progview_base_get(ctx, &word, &base, &width, &live) != LDG_ERR_AOK) { return progview_dead_draw(render, (uint32_t)PROGVIEW_HDR_ROW); }

    if (snprintf(line, sizeof(line), PROGVIEW_HDR_FMT, ctx->sig, word * (uint64_t)LDG_DANGCSA_INSTR_SIZE, width) < 0) { return LDG_ERR_STR_FMT; }

    ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)PROGVIEW_ORIG_COL, (uint32_t)PROGVIEW_HDR_ROW, line, (uint64_t)sizeof(line), (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, 0x0);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_eda_ic_fpga_mem_rd(ctx->fpga, ctx->slot, ctx->sig, base, words, (uint32_t)CROSSTALK_EDA_UI_PROGVIEW_ROW_CUNT, &got);
    if (ret != LDG_ERR_AOK) { return progview_dead_draw(render, row); }

    for (idx = 0; idx < got; idx = idx + (uint32_t)PROGVIEW_STEP)
    {
        addr = (base + (uint64_t)idx) * (uint64_t)LDG_DANGCSA_INSTR_SIZE;
        at_loc = live == LDG_TRUTH_TRUE && base + (uint64_t)idx == word ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

        if (ldg_dangcsa_decode(words[idx], &decoded) != LDG_ERR_AOK || ldg_dangcsa_disasm(&decoded, asm_text, sizeof(asm_text)) != LDG_ERR_AOK) { if (ldg_str_rbrcpy(asm_text, PROGVIEW_ASM_FAIL_TEXT, sizeof(asm_text)) != LDG_ERR_AOK) { return LDG_ERR_STR_TRUNC; } }

        if (snprintf(line, sizeof(line), PROGVIEW_ROW_FMT, at_loc == LDG_TRUTH_TRUE ? PROGVIEW_MARK_CH : PROGVIEW_PLAIN_CH, addr, words[idx], asm_text) < 0) { return LDG_ERR_STR_FMT; }

        ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)PROGVIEW_ORIG_COL, row, line, (uint64_t)sizeof(line), at_loc == LDG_TRUTH_TRUE ? (uint32_t)CROSSTALK_UI_THEME_MARK_RGBA : (uint32_t)CROSSTALK_UI_THEME_DATA_RGBA, 0x0);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        row = row + (uint32_t)PROGVIEW_STEP;
    }

    ctx->win.content_row_cunt = row;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_progview_win_reg(crosstalk_eda_ui_progview_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot)
{
    if (LDG_UNLIKELY(ctx == 0x0 || win_nav == 0x0 || out_slot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_ui_window_reg(win_nav, (uint64_t)CROSSTALK_EDA_UI_PROGVIEW_WINDOW_ID, CROSSTALK_EDA_UI_PROGVIEW_WINDOW_TITLE, &ctx->win, progview_draw_cb, 0x0, ctx, out_slot);
}
