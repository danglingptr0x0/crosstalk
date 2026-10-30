#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/arch/dangcsa/dangcsa.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "eda/ui/pipeview.h"
#include "ui/cell.h"
#include "ui/theme.h"

#define PIPEVIEW_STEP 1
#define PIPEVIEW_ORIG_COL 0
#define PIPEVIEW_HINT_ROW 0
#define PIPEVIEW_DEAD_FMT "%-12.12s %.39s"
#define PIPEVIEW_FLASH_FRAMES 20
#define PIPEVIEW_ROW_FMT "%-12.12s %08" PRIX64 " %.39s"
#define PIPEVIEW_LINE_CAP 112
#define PIPEVIEW_HINT_TEXT "pipeview <stage sigs>; one insn-word sig per stage (in order)"
#define PIPEVIEW_DEAD_TEXT "stage sig not found"
#define PIPEVIEW_ASM_FAIL_TEXT "??"

uint32_t crosstalk_eda_ui_pipeview_init(crosstalk_eda_ui_pipeview_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->win.scroll_on = LDG_TRUTH_TRUE;
    ctx->win.open = LDG_TRUTH_FALSE;
    ctx->open = LDG_TRUTH_FALSE;
    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_pipeview_bind(crosstalk_eda_ui_pipeview_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot)
{
    if (LDG_UNLIKELY(ctx == 0x0 || fpga == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->fpga = fpga;
    ctx->slot = slot;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_pipeview_stages_set(crosstalk_eda_ui_pipeview_ctx_t *ctx, const char *args)
{
    uint32_t at = 0;
    uint32_t pos = 0;
    uint32_t cunt = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || args == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (args[at] != LDG_STR_TERM && cunt < (uint32_t)CROSSTALK_EDA_UI_PIPEVIEW_STAGE_CAP)
    {
        while (args[at] == ' ') { at = at + 0x1; }

        if (args[at] == LDG_STR_TERM) { break; }

        pos = 0;

        while (args[at] != LDG_STR_TERM && args[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)CROSSTALK_EDA_UI_PIPEVIEW_SIG_CAP)
        {
            ctx->stage_sig[cunt][pos] = args[at];
            pos = pos + 0x1;
            at = at + 0x1;
        }

        ctx->stage_sig[cunt][pos] = LDG_STR_TERM;

        while (args[at] != LDG_STR_TERM && args[at] != ' ') { at = at + 0x1; }

        cunt = cunt + 0x1;
    }

    if (cunt == 0) { return LDG_ERR_STR_EMPTY; }

    ctx->stage_cunt = cunt;
    memset(ctx->prev, 0x0, sizeof(ctx->prev));
    memset(ctx->flash, 0x0, sizeof(ctx->flash));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_pipeview_open_set(crosstalk_eda_ui_pipeview_ctx_t *ctx, uint8_t open)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->open = open;
    ctx->win.open = open;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_ui_pipeview_open_is(const crosstalk_eda_ui_pipeview_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->win.open != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t pipeview_draw_cb(uint64_t win_id, void *user, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    crosstalk_eda_ui_pipeview_ctx_t *ctx = (crosstalk_eda_ui_pipeview_ctx_t *)user;
    char asm_text[LDG_DANGCSA_DISASM_BUFF_MIN] = LDG_ARR_ZERO_INIT;
    char line[PIPEVIEW_LINE_CAP] = LDG_ARR_ZERO_INIT;
    ldg_dangcsa_decoded_t decoded = LDG_STRUCT_ZERO_INIT;
    uint64_t word = 0;
    uint32_t width = 0;
    uint32_t row = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (win_id != (uint64_t)CROSSTALK_EDA_UI_PIPEVIEW_WINDOW_ID || col_cunt == 0 || row_cunt == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->stage_cunt == 0 || ctx->fpga == 0x0) { return crosstalk_ui_cell_rgba_put(render, (uint32_t)PIPEVIEW_ORIG_COL, (uint32_t)PIPEVIEW_HINT_ROW, PIPEVIEW_HINT_TEXT, (uint64_t)sizeof(PIPEVIEW_HINT_TEXT), (uint32_t)CROSSTALK_UI_THEME_DEAD_RGBA, 0x0); }

    for (idx = 0; idx < ctx->stage_cunt; idx = idx + (uint32_t)PIPEVIEW_STEP)
    {
        if (crosstalk_eda_ic_fpga_sig_peek(ctx->fpga, ctx->slot, ctx->stage_sig[idx], &word, &width) != LDG_ERR_AOK)
        {
            if (snprintf(line, sizeof(line), PIPEVIEW_DEAD_FMT, ctx->stage_sig[idx], PIPEVIEW_DEAD_TEXT) < 0) { return LDG_ERR_STR_FMT; }

            ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)PIPEVIEW_ORIG_COL, row, line, (uint64_t)sizeof(line), (uint32_t)CROSSTALK_UI_THEME_DEAD_RGBA, 0x0);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            row = row + (uint32_t)PIPEVIEW_STEP;
            continue;
        }

        if (word != ctx->prev[idx])
        {
            ctx->prev[idx] = word;
            ctx->flash[idx] = PIPEVIEW_FLASH_FRAMES;
        }

        if (ldg_dangcsa_decode(word, &decoded) != LDG_ERR_AOK || ldg_dangcsa_disasm(&decoded, asm_text, sizeof(asm_text)) != LDG_ERR_AOK) { if (ldg_str_rbrcpy(asm_text, PIPEVIEW_ASM_FAIL_TEXT, sizeof(asm_text)) != LDG_ERR_AOK) { return LDG_ERR_STR_TRUNC; } }

        if (snprintf(line, sizeof(line), PIPEVIEW_ROW_FMT, ctx->stage_sig[idx], word, asm_text) < 0) { return LDG_ERR_STR_FMT; }

        ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)PIPEVIEW_ORIG_COL, row, line, (uint64_t)sizeof(line), ctx->flash[idx] != 0 ? (uint32_t)CROSSTALK_UI_THEME_LIVE_RGBA : (uint32_t)CROSSTALK_UI_THEME_DATA_RGBA, 0x0);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (ctx->flash[idx] != 0) { ctx->flash[idx] = ctx->flash[idx] - (uint32_t)PIPEVIEW_STEP; }

        row = row + (uint32_t)PIPEVIEW_STEP;
    }

    ctx->win.content_row_cunt = row;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_pipeview_win_reg(crosstalk_eda_ui_pipeview_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot)
{
    if (LDG_UNLIKELY(ctx == 0x0 || win_nav == 0x0 || out_slot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_ui_window_reg(win_nav, (uint64_t)CROSSTALK_EDA_UI_PIPEVIEW_WINDOW_ID, CROSSTALK_EDA_UI_PIPEVIEW_WINDOW_TITLE, &ctx->win, pipeview_draw_cb, 0x0, ctx, out_slot);
}
