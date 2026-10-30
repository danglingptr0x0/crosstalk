#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/logging/logging.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#include "core/log.h"
#include "eda/ui/regview.h"
#include "ui/cell.h"
#include "ui/theme.h"

#define REGVIEW_STEP 1
#define REGVIEW_ORIG_COL 0
#define REGVIEW_DEAD_ROW 0
#define REGVIEW_FLASH_FRAMES 20
#define REGVIEW_REG_PATH_FMT "dr%u"
#define REGVIEW_LABEL_FMT "dr%u; "
#define REGVIEW_LOC_IDX 14
#define REGVIEW_PATH_CAP 32
#define REGVIEW_VAL_FMT "0x%08" PRIX64
#define REGVIEW_MMIO_BASE 0x90000000ULL
#define REGVIEW_MMIO_END 0xA0000000ULL
#define REGVIEW_WORD_ALIGN 4
#define REGVIEW_SP_IDX 15
#define REGVIEW_NOTE_MMIO_TEXT " mmio"
#define REGVIEW_NOTE_STACK_TEXT " stack"
#define REGVIEW_NOTE_PTR_TEXT " ptr"
#define REGVIEW_NOTE_LOC_TEXT " loc"
#define REGVIEW_NOTE_CAP 8
#define REGVIEW_DEAD_TEXT "no running design"

static uint8_t regview_reg_val_get(crosstalk_eda_ui_regview_ctx_t *ctx, uint32_t idx, uint64_t *out)
{
    char path[REGVIEW_PATH_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t width = 0;
    uint32_t ret = 0;

    *out = 0;

    if (ctx->fpga == 0x0) { return LDG_TRUTH_FALSE; }

    if (idx == (uint32_t)REGVIEW_LOC_IDX) { return crosstalk_eda_ic_fpga_sig_peek(ctx->fpga, ctx->slot, CROSSTALK_EDA_IC_FPGA_PROG_SIG, out, &width) == LDG_ERR_AOK ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    if (snprintf(path, sizeof(path), REGVIEW_REG_PATH_FMT, idx) < 0) { return LDG_TRUTH_FALSE; }

    ret = crosstalk_eda_ic_fpga_sig_peek(ctx->fpga, ctx->slot, path, out, &width);
    if (ret == LDG_ERR_AOK) { return LDG_TRUTH_TRUE; }

    if (ctx->miss_logged != LDG_TRUTH_TRUE)
    {
        ctx->miss_logged = LDG_TRUTH_TRUE;
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "regview: register signal unreadable; slot: %u32; path: %.31s; err: %u32", ctx->slot, path, ret);
    }

    return LDG_TRUTH_FALSE;
}

uint32_t crosstalk_eda_ui_regview_init(crosstalk_eda_ui_regview_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->win.scroll_on = LDG_TRUTH_TRUE;
    ctx->win.open = LDG_TRUTH_FALSE;
    ctx->open = LDG_TRUTH_FALSE;
    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_regview_bind(crosstalk_eda_ui_regview_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot)
{
    if (LDG_UNLIKELY(ctx == 0x0 || fpga == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->fpga = fpga;
    ctx->slot = slot;
    ctx->miss_logged = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_regview_open_set(crosstalk_eda_ui_regview_ctx_t *ctx, uint8_t open)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->open = open;
    ctx->win.open = open;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_ui_regview_open_is(const crosstalk_eda_ui_regview_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->win.open != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t regview_draw_cb(uint64_t win_id, void *user, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    crosstalk_eda_ui_regview_ctx_t *ctx = (crosstalk_eda_ui_regview_ctx_t *)user;
    uint64_t val = 0;
    uint64_t sp_val = 0;
    uint32_t label_w = 0;
    uint32_t val_w = 0;
    uint32_t row = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (win_id != (uint64_t)CROSSTALK_EDA_UI_REGVIEW_WINDOW_ID || col_cunt == 0 || row_cunt == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (regview_reg_val_get(ctx, (uint32_t)REGVIEW_SP_IDX, &sp_val) != LDG_TRUTH_TRUE) { sp_val = 0; }

    for (idx = 0; idx < (uint32_t)CROSSTALK_EDA_UI_REGVIEW_REG_CUNT; idx = idx + (uint32_t)REGVIEW_STEP)
    {
        const char *note = 0x0;
        uint32_t note_col = CROSSTALK_UI_THEME_STATUS_RGBA;
        uint32_t text_col = 0;

        if (regview_reg_val_get(ctx, idx, &val) != LDG_TRUTH_TRUE) { continue; }

        if (val != ctx->prev[idx])
        {
            ctx->prev[idx] = val;
            ctx->flash[idx] = REGVIEW_FLASH_FRAMES;
        }

        text_col = ctx->flash[idx] != 0 ? (uint32_t)CROSSTALK_UI_THEME_LIVE_RGBA : (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA;

        ret = crosstalk_ui_cell_rgba_fmt_put(render, (uint32_t)REGVIEW_ORIG_COL, row, text_col, &label_w, REGVIEW_LABEL_FMT, idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_ui_cell_rgba_fmt_put(render, label_w, row, text_col, &val_w, REGVIEW_VAL_FMT, val);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (idx == (uint32_t)REGVIEW_LOC_IDX) { note = REGVIEW_NOTE_LOC_TEXT; note_col = CROSSTALK_UI_THEME_ACCENT_RGBA; }
        else if (val >= REGVIEW_MMIO_BASE && val < REGVIEW_MMIO_END) { note = REGVIEW_NOTE_MMIO_TEXT; note_col = CROSSTALK_UI_THEME_ACCENT_RGBA; }
        else if (sp_val != 0 && val >= sp_val && val < sp_val + (uint64_t)LDG_MIB) { note = REGVIEW_NOTE_STACK_TEXT; }
        else if (val != 0 && val % (uint64_t)REGVIEW_WORD_ALIGN == 0 && val < CROSSTALK_EDA_IC_FPGA_RAM_WINDOW_TOP) { note = REGVIEW_NOTE_PTR_TEXT; }

        if (note != 0x0)
        {
            ret = crosstalk_ui_cell_rgba_put(render, label_w + val_w, row, note, (uint64_t)REGVIEW_NOTE_CAP, note_col, 0x0);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        if (ctx->flash[idx] != 0) { ctx->flash[idx] = ctx->flash[idx] - (uint32_t)REGVIEW_STEP; }

        row = row + (uint32_t)REGVIEW_STEP;
    }

    ctx->win.content_row_cunt = row;

    if (row != 0) { return LDG_ERR_AOK; }

    return crosstalk_ui_cell_rgba_put(render, (uint32_t)REGVIEW_ORIG_COL, (uint32_t)REGVIEW_DEAD_ROW, REGVIEW_DEAD_TEXT, (uint64_t)sizeof(REGVIEW_DEAD_TEXT), (uint32_t)CROSSTALK_UI_THEME_DEAD_RGBA, 0x0);
}

uint32_t crosstalk_eda_ui_regview_win_reg(crosstalk_eda_ui_regview_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot)
{
    if (LDG_UNLIKELY(ctx == 0x0 || win_nav == 0x0 || out_slot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_ui_window_reg(win_nav, (uint64_t)CROSSTALK_EDA_UI_REGVIEW_WINDOW_ID, CROSSTALK_EDA_UI_REGVIEW_WINDOW_TITLE, &ctx->win, regview_draw_cb, 0x0, ctx, out_slot);
}
