#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/logging/logging.h>

#include <dangling/arch/dangcsa/dangcsa.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/str/fmt.h>
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
#define PROGVIEW_HDR_FMT "%.31s loc 0x%08X64 depth %u64"
#define PROGVIEW_ROW_FMT "%c %08X64 %08X64 %.39s"
#define PROGVIEW_LINE_CAP 112
#define PROGVIEW_MARK_CH '>'
#define PROGVIEW_PLAIN_CH ' '
#define PROGVIEW_FOLLOW_LEAD 8
#define PROGVIEW_ASM_FAIL_TEXT "??"
#define PROGVIEW_THREAD_HDR_FMT "thread %u32; loc 0x%016X64; bytes %u32"
#define PROGVIEW_THREAD_WAIT_FMT "thread %u32; registers unread; err: %u32"
#define PROGVIEW_THREAD_ADDR_FMT "%016X64 "
#define PROGVIEW_THREAD_BYTE_FMT " %02X32"
#define PROGVIEW_THREAD_ROW_BYTES 16
#define PROGVIEW_THREAD_GAP_COLS 2
#define PROGVIEW_THREAD_BYTE_COLS 3
#define PROGVIEW_THREAD_DOT '.'

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

uint32_t crosstalk_eda_ui_progview_thread_bind(crosstalk_eda_ui_progview_ctx_t *ctx, const crosstalk_re_repl_thread_t *thread)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->thread = thread;

    return LDG_ERR_AOK;
}

static uint32_t progview_thread_row_build(const crosstalk_re_repl_thread_t *thread, uint32_t first, char *line, uint32_t cap)
{
    uint64_t fmt_len = 0;
    uint32_t pos = 0;
    uint32_t at = 0;
    uint32_t pad = 0;
    int32_t wrote = 0;

    wrote = ldg_str_fmt(line, (uint64_t)cap, &fmt_len, PROGVIEW_THREAD_ADDR_FMT, thread->code_addr + (uint64_t)first) == LDG_ERR_AOK ? (int32_t)fmt_len : -1;
    if (wrote < 0 || (uint32_t)wrote >= cap) { return LDG_ERR_STR_FMT; }

    pos = (uint32_t)wrote;

    if (pos + (uint32_t)PROGVIEW_THREAD_ROW_BYTES * ((uint32_t)PROGVIEW_THREAD_BYTE_COLS + (uint32_t)PROGVIEW_STEP) + (uint32_t)PROGVIEW_THREAD_GAP_COLS + LDG_STR_TERM_SIZE >= cap) { return LDG_ERR_STR_TRUNC; }

    for (at = 0; at < (uint32_t)PROGVIEW_THREAD_ROW_BYTES; at = at + (uint32_t)PROGVIEW_STEP)
    {
        if (first + at < thread->code_cunt) { if (ldg_str_fmt(line + pos, (uint64_t)(cap - pos), &fmt_len, PROGVIEW_THREAD_BYTE_FMT, (uint32_t)thread->code[first + at]) != LDG_ERR_AOK) { return LDG_ERR_STR_FMT; } }
        else { for (pad = 0; pad < (uint32_t)PROGVIEW_THREAD_BYTE_COLS; pad = pad + (uint32_t)PROGVIEW_STEP) { line[pos + pad] = PROGVIEW_PLAIN_CH; } }

        pos = pos + (uint32_t)PROGVIEW_THREAD_BYTE_COLS;
    }

    for (pad = 0; pad < (uint32_t)PROGVIEW_THREAD_GAP_COLS; pad = pad + (uint32_t)PROGVIEW_STEP) { line[pos + pad] = PROGVIEW_PLAIN_CH; }

    pos = pos + (uint32_t)PROGVIEW_THREAD_GAP_COLS;

    for (at = 0; at < (uint32_t)PROGVIEW_THREAD_ROW_BYTES && first + at < thread->code_cunt; at = at + (uint32_t)PROGVIEW_STEP)
    {
        line[pos] = thread->code[first + at] >= (uint8_t)LDG_ASCII_PRINTABLE_MIN && thread->code[first + at] <= (uint8_t)LDG_ASCII_PRINTABLE_MAX ? (char)thread->code[first + at] : PROGVIEW_THREAD_DOT;
        pos = pos + (uint32_t)PROGVIEW_STEP;
    }

    line[pos] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

static uint32_t progview_thread_draw(crosstalk_eda_ui_progview_ctx_t *ctx, ldg_term_render_t *render)
{
    uint64_t fmt_len = 0;
    const crosstalk_re_repl_thread_t *thread = ctx->thread;
    char line[PROGVIEW_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t row = PROGVIEW_BODY_ROW;
    uint32_t first = 0;
    uint32_t ret = 0;

    if (thread->regs_ok != LDG_TRUTH_TRUE)
    {
        if (ldg_str_fmt(line, (uint64_t)sizeof(line), &fmt_len, PROGVIEW_THREAD_WAIT_FMT, thread->sel, thread->last_err) != LDG_ERR_AOK) { return LDG_ERR_STR_FMT; }

        ctx->win.content_row_cunt = (uint32_t)PROGVIEW_STEP;

        return crosstalk_ui_cell_rgba_put(render, (uint32_t)PROGVIEW_ORIG_COL, (uint32_t)PROGVIEW_HDR_ROW, line, (uint64_t)sizeof(line), (uint32_t)CROSSTALK_UI_THEME_DEAD_RGBA, 0x0);
    }

    if (ldg_str_fmt(line, (uint64_t)sizeof(line), &fmt_len, PROGVIEW_THREAD_HDR_FMT, thread->sel, thread->code_addr, thread->code_cunt) != LDG_ERR_AOK) { return LDG_ERR_STR_FMT; }

    ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)PROGVIEW_ORIG_COL, (uint32_t)PROGVIEW_HDR_ROW, line, (uint64_t)sizeof(line), (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, 0x0);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (first = 0; first < thread->code_cunt; first = first + (uint32_t)PROGVIEW_THREAD_ROW_BYTES)
    {
        ret = progview_thread_row_build(thread, first, line, (uint32_t)sizeof(line));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)PROGVIEW_ORIG_COL, row, line, (uint64_t)sizeof(line), first == 0 ? (uint32_t)CROSSTALK_UI_THEME_MARK_RGBA : (uint32_t)CROSSTALK_UI_THEME_DATA_RGBA, 0x0);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        row = row + (uint32_t)PROGVIEW_STEP;
    }

    ctx->win.content_row_cunt = row;

    return LDG_ERR_AOK;
}

static uint32_t progview_draw_cb(uint64_t win_id, void *user, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    uint64_t fmt_len = 0;
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

    if (ctx->thread != 0x0 && ctx->thread->live == LDG_TRUTH_TRUE) { return progview_thread_draw(ctx, render); }

    if (ctx->fpga == 0x0) { return progview_dead_draw(render, (uint32_t)PROGVIEW_HDR_ROW); }

    if (progview_base_get(ctx, &word, &base, &width, &live) != LDG_ERR_AOK) { return progview_dead_draw(render, (uint32_t)PROGVIEW_HDR_ROW); }

    if (ldg_str_fmt(line, (uint64_t)sizeof(line), &fmt_len, PROGVIEW_HDR_FMT, ctx->sig, word * (uint64_t)LDG_DANGCSA_INSTR_SIZE, width) != LDG_ERR_AOK) { return LDG_ERR_STR_FMT; }

    ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)PROGVIEW_ORIG_COL, (uint32_t)PROGVIEW_HDR_ROW, line, (uint64_t)sizeof(line), (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, 0x0);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_eda_ic_fpga_mem_rd(ctx->fpga, ctx->slot, ctx->sig, base, words, (uint32_t)CROSSTALK_EDA_UI_PROGVIEW_ROW_CUNT, &got);
    if (ret != LDG_ERR_AOK) { return progview_dead_draw(render, row); }

    for (idx = 0; idx < got; idx = idx + (uint32_t)PROGVIEW_STEP)
    {
        addr = (base + (uint64_t)idx) * (uint64_t)LDG_DANGCSA_INSTR_SIZE;
        at_loc = live == LDG_TRUTH_TRUE && base + (uint64_t)idx == word ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

        if (ldg_dangcsa_decode(words[idx], &decoded) != LDG_ERR_AOK || ldg_dangcsa_disasm(&decoded, asm_text, sizeof(asm_text)) != LDG_ERR_AOK) { if (ldg_str_rbrcpy(asm_text, PROGVIEW_ASM_FAIL_TEXT, sizeof(asm_text)) != LDG_ERR_AOK) { return LDG_ERR_STR_TRUNC; } }

        if (ldg_str_fmt(line, (uint64_t)sizeof(line), &fmt_len, PROGVIEW_ROW_FMT, at_loc == LDG_TRUTH_TRUE ? PROGVIEW_MARK_CH : PROGVIEW_PLAIN_CH, addr, words[idx], asm_text) != LDG_ERR_AOK) { return LDG_ERR_STR_FMT; }

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
