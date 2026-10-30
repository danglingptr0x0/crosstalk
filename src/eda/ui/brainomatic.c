#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#include "eda/ui/brainomatic.h"
#include "ui/cell.h"
#include "ui/theme.h"

#define BRAINOMATIC_STEP 1
#define BRAINOMATIC_ORIG_COL 0
#define BRAINOMATIC_SECT_GAP 1
#define BRAINOMATIC_LINE_CAP 128
#define BRAINOMATIC_FIELD_CUNT 4
#define BRAINOMATIC_FIELD_FIRST_FMT "%s"
#define BRAINOMATIC_FIELD_NEXT_FMT "; %s"

typedef struct brainomatic_row
{
    const char *hd;
    const char *enc;
    const char *ops;
    const char *note;
} brainomatic_row_t;

static uint32_t brainomatic_rows_draw(crosstalk_eda_ui_brainomatic_ctx_t *ctx, ldg_term_render_t *render, const brainomatic_row_t *rows, uint32_t row_cunt)
{
    char line[BRAINOMATIC_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t row = 0;
    uint32_t idx = 0;
    uint8_t sect = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0 || rows == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < row_cunt; idx = idx + (uint32_t)BRAINOMATIC_STEP)
    {
        sect = rows[idx].enc == 0x0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

        if (sect == LDG_TRUTH_TRUE && idx != 0) { row = row + (uint32_t)BRAINOMATIC_SECT_GAP; }

        if (sect == LDG_TRUTH_TRUE) { ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)BRAINOMATIC_ORIG_COL, row, rows[idx].hd, (uint64_t)BRAINOMATIC_LINE_CAP, (uint32_t)CROSSTALK_UI_THEME_LIVE_RGBA, 0x0); }
        else
        {
            const char *field[BRAINOMATIC_FIELD_CUNT] = { rows[idx].hd, rows[idx].enc, rows[idx].ops, rows[idx].note };
            uint32_t field_idx = 0;
            uint32_t pos = 0;
            int32_t wrote = 0;

            line[0] = LDG_STR_TERM;

            for (field_idx = 0; field_idx < (uint32_t)BRAINOMATIC_FIELD_CUNT; field_idx = field_idx + (uint32_t)BRAINOMATIC_STEP)
            {
                if (field[field_idx] == 0x0 || field[field_idx][0] == LDG_STR_TERM) { continue; }

                wrote = snprintf(&line[pos], sizeof(line) - pos, pos == 0 ? BRAINOMATIC_FIELD_FIRST_FMT : BRAINOMATIC_FIELD_NEXT_FMT, field[field_idx]);
                if (wrote < 0) { return LDG_ERR_STR_FMT; }

                pos = pos + (uint32_t)wrote;
                if (pos >= (uint32_t)sizeof(line)) { break; }
            }

            ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)BRAINOMATIC_ORIG_COL, row, line, (uint64_t)sizeof(line), (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, 0x0);
        }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        row = row + (uint32_t)BRAINOMATIC_STEP;
    }

    ctx->win.content_row_cunt = row;

    return LDG_ERR_AOK;
}

static uint32_t brainomatic_dangcsa_draw(crosstalk_eda_ui_brainomatic_ctx_t *ctx, ldg_term_render_t *render)
{
    const brainomatic_row_t rows[] = {
        { "DANGCSA 0.5.0; 64-bit; word 8 bytes; align per width", 0x0, 0x0, 0x0 },
        { "opcodes", 0x0, 0x0, 0x0 },
        { "CPY", "0x0", "dr, dr|imm", "dst = src; F0 imm sel, F1 quarter sel" },
        { "LD", "0x1", "dr, [dr+disp]", "sized; width in conf, F2 sign fill" },
        { "ST", "0x2", "[dr+disp], dr", "sized; width in conf" },
        { "ADD", "0x3", "dr, dr|imm", "dst += src; sets C; F0 carry in, F1 imm sel" },
        { "SUB", "0x4", "dr, dr|imm", "dst -= src; sets C; F0 borrow in, F1 imm sel" },
        { "AND", "0x5", "dr, dr", "dst = dst & src" },
        { "OR", "0x6", "dr, dr", "dst = dst | src" },
        { "XOR", "0x7", "dr, dr", "dst = dst ^ src" },
        { "SH", "0x8", "dr, amt|dr", "F0 right sel, F1 arith fill, F2 imm sel" },
        { "MUL", "0x9", "dr, dr", "dst = low half of dst * src; F0 high sel, F1 sign sel" },
        { "DIV", "0xA", "dr, dr", "quotient or rem; F0 sign sel, F1 rem sel" },
        { "CLZ", "0xB", "dr, dr", "dst = leading zeros of src; F0 counts trailing" },
        { "JMP", "0xC", "dr, dr", "cond in conf, F3 relative; offset counts instructions" },
        { "SETCC", "0xD", "dr, dr", "dst = 0x1 when cond holds, else 0x0" },
        { "CALL", "0xE", "dr", "push the next loc; loc = dst" },
        { "RET", "0xF", "-", "pop loc" },
        { "encoding (one 64-bit word)", 0x0, 0x0, 0x0 },
        { "[63:60]", "", "opcode", "every encoding is defined" },
        { "[59:54]", "", "dst", "" },
        { "[53:48]", "", "src", "" },
        { "conf", "", "low 48 bits", "instruction-specific" },
        { "conf [47:44]", "", "F0 F1 F2 F3", "flag meanings are per opcode" },
        { "conf [47:46]", "", "width", "LD/ST width sel" },
        { "conf [47:45]", "", "cond", "JMP/SETCC cond sel; all-ones reserved, raises 0x1" },
        { "conf [45:44]", "", "quarter", "CPY quarter sel" },
        { "conf low 32", "", "imm or disp", "sign filled; JMP offset counts instructions" },
        { "conf low 6", "", "shift amt", "SH imm form; max 0x3F" },
        { "registers", 0x0, 0x0, 0x0 },
        { "dr0-dr15", "", "general", "allocatable" },
        { "tdr0-tdr5", "", "temporary", "caller-saved, never callee-saved" },
        { "loc", "", "idx 0x16", "rd as src only; illegal as dst" },
        { "sp", "", "idx 0x17", "stack top addr; grows down; 8-byte slots" },
        { "idx 0x18 up", "", "reserved", "any reference raises 0x6" },
        { "C", "", "carry", "single bit; set by every ADD and SUB" },
        { "mem map", 0x0, 0x0, 0x0 },
        { "RAM", "", "0x0000000000000000", "top 0x0000FFFFFFFFFFFF; RST vec at 0x0" },
        { "I/O", "", "0x0001000000000000", "bits [63:48] select the page" },
        { "ctrl", "", "0x0001000080000000", "page one holds the device map" },
        { "exc", "", "0x0001000080000000", "handler, 64-bit; cause at +8" },
        { "exceptions", 0x0, 0x0, 0x0 },
        { "0x1", "", "invalid op", "reserved cond code" },
        { "0x2", "", "unaligned", "per width; byte access free" },
        { "0x3", "", "stack over", "sp past the top on RET" },
        { "0x4", "", "stack under", "sp below a slot on CALL" },
        { "0x5", "", "div by zero", "" },
        { "0x6", "", "reserved reg", "loc as dst, or a reserved idx" }
    };

    return brainomatic_rows_draw(ctx, render, rows, (uint32_t)(sizeof(rows) / sizeof(*rows)));
}

uint32_t crosstalk_eda_ui_brainomatic_init(crosstalk_eda_ui_brainomatic_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->win.scroll_on = LDG_TRUTH_TRUE;
    ctx->win.open = LDG_TRUTH_FALSE;
    ctx->arch = (uint32_t)CROSSTALK_EDA_UI_BRAINOMATIC_ARCH_DANGCSA;
    ctx->open = LDG_TRUTH_FALSE;
    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_brainomatic_arch_set(crosstalk_eda_ui_brainomatic_ctx_t *ctx, uint32_t arch)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (arch != (uint32_t)CROSSTALK_EDA_UI_BRAINOMATIC_ARCH_DANGCSA) { return LDG_ERR_UNSUPPORTED; }

    ctx->arch = arch;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_brainomatic_open_set(crosstalk_eda_ui_brainomatic_ctx_t *ctx, uint8_t open)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->open = open;
    ctx->win.open = open;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_ui_brainomatic_open_is(const crosstalk_eda_ui_brainomatic_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->win.open != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t brainomatic_draw_cb(uint64_t win_id, void *user, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    crosstalk_eda_ui_brainomatic_ctx_t *ctx = (crosstalk_eda_ui_brainomatic_ctx_t *)user;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (win_id != (uint64_t)CROSSTALK_EDA_UI_BRAINOMATIC_WINDOW_ID || col_cunt == 0 || row_cunt == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->arch == (uint32_t)CROSSTALK_EDA_UI_BRAINOMATIC_ARCH_DANGCSA) { return brainomatic_dangcsa_draw(ctx, render); }

    return LDG_ERR_UNSUPPORTED;
}

uint32_t crosstalk_eda_ui_brainomatic_win_reg(crosstalk_eda_ui_brainomatic_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot)
{
    if (LDG_UNLIKELY(ctx == 0x0 || win_nav == 0x0 || out_slot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_ui_window_reg(win_nav, (uint64_t)CROSSTALK_EDA_UI_BRAINOMATIC_WINDOW_ID, CROSSTALK_EDA_UI_BRAINOMATIC_WINDOW_TITLE, &ctx->win, brainomatic_draw_cb, 0x0, ctx, out_slot);
}
