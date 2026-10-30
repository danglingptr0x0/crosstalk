#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include <dangling/logging/logging.h>

#include "core/log.h"
#include "eda/ui/term.h"
#include "ui/cell.h"
#include "ui/theme.h"

#define TERM_CAPTURE_FULL_MSG "window capture registry full"
#define TERM_STATUS_FMT "%.47s %" PRIu64 " bytes"
#define TERM_STATUS_IDLE "no session attached"
#define TERM_HEX_DIGITS "0123456789ABCDEF"
#define TERM_BIT_SET '1'
#define TERM_BIT_CLR '0'
#define TERM_STEP 1
#define TERM_ORIG_COL 0
#define TERM_TAB_ROW 0
#define TERM_TAB_GAP 1
#define TERM_TAB_FMT "[%.1s%u %.24s]"
#define TERM_TAB_MARK "*"
#define TERM_STATUS_ROWS 1
#define TERM_PTY_COL_CAP 128
#define TERM_PTY_RUN_CAP 129
#define TERM_PTY_CURSOR_W 1
#define TERM_PTY_EXIT_FMT "shell exited; code: %u"
#define TERM_PTY_DEAD_TEXT "no shell; open one from the console"
#define TERM_PTY_ALPHA_SHIFT 8
#define TERM_PTY_ALPHA_BYTE 0xFF
#define TERM_PTY_DIM_NUM 3
#define TERM_PTY_DIM_DEN 5
#define TERM_IMG_SAMPLE_W 1
#define TERM_IMG_GREEN_AT 1
#define TERM_IMG_BLUE_AT 2
#define TERM_SEP_W_SAMPLES 1
#define TERM_SEP_CENTER_DIV 2
#define TERM_SGR_STATE_IDLE 0
#define TERM_SGR_STATE_ESC 1
#define TERM_SGR_STATE_CSI 2
#define TERM_SGR_PARAM_EMPTY 0

static void term_sgr_apply(crosstalk_eda_ui_term_sesh_t *sesh)
{
    uint32_t at = 0;

    for (at = 0; at < sesh->sgr_param_cunt; at = at + (uint32_t)TERM_STEP)
    {
        uint32_t param = sesh->sgr_param[at];

        if (param == (uint32_t)CROSSTALK_EDA_UI_TERM_SGR_RST)
        {
            sesh->sgr_idx = (uint8_t)CROSSTALK_EDA_UI_TERM_SGR_IDX_NONE;
            sesh->sgr_bold = LDG_TRUTH_FALSE;
        }
        else if (param == (uint32_t)CROSSTALK_EDA_UI_TERM_SGR_BOLD) { sesh->sgr_bold = LDG_TRUTH_TRUE; }
        else if (param == (uint32_t)CROSSTALK_EDA_UI_TERM_SGR_FG_DEFAULT) { sesh->sgr_idx = (uint8_t)CROSSTALK_EDA_UI_TERM_SGR_IDX_NONE; }
        else if (param >= (uint32_t)CROSSTALK_EDA_UI_TERM_SGR_FG_LO && param <= (uint32_t)CROSSTALK_EDA_UI_TERM_SGR_FG_HI)
        {
            sesh->sgr_idx = (uint8_t)(param - (uint32_t)CROSSTALK_EDA_UI_TERM_SGR_FG_LO);

            if (sesh->sgr_bold == LDG_TRUTH_TRUE) { sesh->sgr_idx = sesh->sgr_idx + (uint8_t)CROSSTALK_EDA_UI_TERM_SGR_BRIGHT_STEP; }
        }
        else if (param >= (uint32_t)CROSSTALK_EDA_UI_TERM_SGR_BRIGHT_LO && param <= (uint32_t)CROSSTALK_EDA_UI_TERM_SGR_BRIGHT_HI) { sesh->sgr_idx = (uint8_t)(param - (uint32_t)CROSSTALK_EDA_UI_TERM_SGR_BRIGHT_LO + (uint32_t)CROSSTALK_EDA_UI_TERM_SGR_BRIGHT_STEP); }
    }
}

static void term_sgr_step(crosstalk_eda_ui_term_sesh_t *sesh, uint8_t val, uint8_t *out)
{
    *out = (sesh->sgr_state != (uint32_t)TERM_SGR_STATE_IDLE || val == (uint8_t)LDG_ASCII_ESC) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (sesh->sgr_state == (uint32_t)TERM_SGR_STATE_IDLE)
    {
        if (val == (uint8_t)LDG_ASCII_ESC) { sesh->sgr_state = (uint32_t)TERM_SGR_STATE_ESC; }

        return;
    }

    if (sesh->sgr_state == (uint32_t)TERM_SGR_STATE_ESC)
    {
        sesh->sgr_param_cunt = (uint32_t)TERM_SGR_PARAM_EMPTY;
        sesh->sgr_param[0] = (uint32_t)TERM_SGR_PARAM_EMPTY;
        sesh->sgr_state = val == (uint8_t)CROSSTALK_EDA_UI_TERM_CSI ? (uint32_t)TERM_SGR_STATE_CSI : (uint32_t)TERM_SGR_STATE_IDLE;

        return;
    }

    if (ldg_char_digit_is((char)val) == LDG_TRUTH_TRUE)
    {
        if (sesh->sgr_param_cunt == (uint32_t)TERM_SGR_PARAM_EMPTY) { sesh->sgr_param_cunt = (uint32_t)TERM_STEP; }

        sesh->sgr_param[sesh->sgr_param_cunt - (uint32_t)TERM_STEP] = sesh->sgr_param[sesh->sgr_param_cunt - (uint32_t)TERM_STEP] * (uint32_t)LDG_BASE_DEC + (uint32_t)(val - (uint8_t)LDG_ASCII_DIGIT_ZERO);

        return;
    }

    if (val == (uint8_t)CROSSTALK_EDA_UI_TERM_SGR_SEP)
    {
        if (sesh->sgr_param_cunt == (uint32_t)TERM_SGR_PARAM_EMPTY) { sesh->sgr_param_cunt = (uint32_t)TERM_STEP; }

        if (sesh->sgr_param_cunt < (uint32_t)CROSSTALK_EDA_UI_TERM_SGR_PARAM_MAX)
        {
            sesh->sgr_param[sesh->sgr_param_cunt] = (uint32_t)TERM_SGR_PARAM_EMPTY;
            sesh->sgr_param_cunt = sesh->sgr_param_cunt + (uint32_t)TERM_STEP;
        }

        return;
    }

    if (val >= (uint8_t)CROSSTALK_EDA_UI_TERM_ESC_FINAL_LO && val <= (uint8_t)CROSSTALK_EDA_UI_TERM_ESC_FINAL_HI)
    {
        if (val == (uint8_t)CROSSTALK_EDA_UI_TERM_SGR_FINAL) { term_sgr_apply(sesh); }

        sesh->sgr_state = (uint32_t)TERM_SGR_STATE_IDLE;
    }
}

static void term_raw_put(crosstalk_eda_ui_term_sesh_t *sesh, uint8_t val)
{
    uint32_t slot = (sesh->raw_hd + sesh->raw_cunt) % (uint32_t)CROSSTALK_EDA_UI_TERM_RAW_CAP;
    uint8_t ctrl_is = LDG_TRUTH_FALSE;

    term_sgr_step(sesh, val, &ctrl_is);

    sesh->raw[slot] = val;
    sesh->raw_col[slot] = ctrl_is == LDG_TRUTH_TRUE ? (uint8_t)CROSSTALK_EDA_UI_TERM_SGR_IDX_CTRL : sesh->sgr_idx;

    if (sesh->raw_cunt < (uint32_t)CROSSTALK_EDA_UI_TERM_RAW_CAP) { sesh->raw_cunt = sesh->raw_cunt + (uint32_t)TERM_STEP; }
    else { sesh->raw_hd = (sesh->raw_hd + (uint32_t)TERM_STEP) % (uint32_t)CROSSTALK_EDA_UI_TERM_RAW_CAP; }
}

static uint8_t term_raw_col_at_get(const crosstalk_eda_ui_term_sesh_t *sesh, uint32_t idx)
{
    return sesh->raw_col[(sesh->raw_hd + idx) % (uint32_t)CROSSTALK_EDA_UI_TERM_RAW_CAP];
}

static uint32_t term_sgr_col_get(const crosstalk_eda_ui_term_sesh_t *sesh, uint8_t idx, uint32_t fallback)
{
    uint8_t rgb[CROSSTALK_EDA_UI_TERM_RGB_CUNT] = LDG_ARR_ZERO_INIT;

    if (idx == (uint8_t)CROSSTALK_EDA_UI_TERM_SGR_IDX_NONE || idx == (uint8_t)CROSSTALK_EDA_UI_TERM_SGR_IDX_CTRL || sesh->term == 0x0) { return fallback; }

    if (ldg_term_palette_get(sesh->term, (uint32_t)idx, rgb) != LDG_ERR_AOK) { return fallback; }

    return ((uint32_t)rgb[0] << (uint32_t)(LDG_BYTE_BITS * 0x3)) | ((uint32_t)rgb[1] << (uint32_t)(LDG_BYTE_BITS * 0x2)) | ((uint32_t)rgb[2] << (uint32_t)LDG_BYTE_BITS) | (uint32_t)CROSSTALK_EDA_UI_TERM_COL_ALPHA;
}

static uint8_t term_raw_at_get(const crosstalk_eda_ui_term_sesh_t *sesh, uint32_t idx)
{
    return sesh->raw[(sesh->raw_hd + idx) % (uint32_t)CROSSTALK_EDA_UI_TERM_RAW_CAP];
}

static uint32_t term_byte_col_get(uint8_t val)
{
    if (val == 0) { return CROSSTALK_UI_THEME_DEAD_RGBA; }

    if (val == (uint8_t)LDG_ASCII_SPACE) { return CROSSTALK_UI_THEME_STATUS_RGBA; }

    if (val >= (uint8_t)CROSSTALK_EDA_UI_TERM_WS_MIN && val <= (uint8_t)CROSSTALK_EDA_UI_TERM_WS_MAX) { return CROSSTALK_UI_THEME_STATUS_RGBA; }

    if (val >= (uint8_t)LDG_ASCII_PRINTABLE_MIN && val <= (uint8_t)LDG_ASCII_PRINTABLE_MAX) { return CROSSTALK_UI_THEME_TEXT_RGBA; }

    if (val < (uint8_t)LDG_ASCII_PRINTABLE_MIN) { return CROSSTALK_UI_THEME_ACCENT_RGBA; }

    return CROSSTALK_UI_THEME_FOCUS_RGBA;
}

static char term_byte_glyph_get(uint8_t val)
{
    if (val >= (uint8_t)LDG_ASCII_PRINTABLE_MIN && val <= (uint8_t)LDG_ASCII_PRINTABLE_MAX) { return (char)val; }

    if (val >= (uint8_t)CROSSTALK_EDA_UI_TERM_WS_MIN && val <= (uint8_t)CROSSTALK_EDA_UI_TERM_WS_MAX) { return (char)CROSSTALK_EDA_UI_TERM_LOW_DASH; }

    return (char)CROSSTALK_EDA_UI_TERM_DOT;
}

static uint32_t term_pty_cell_col_get(const ldg_term_cell_t *cell)
{
    uint32_t col = 0;

    col = (uint32_t)cell->fg[0];
    col = (col << TERM_PTY_ALPHA_SHIFT) | (uint32_t)cell->fg[1];
    col = (col << TERM_PTY_ALPHA_SHIFT) | (uint32_t)cell->fg[2];
    col = (col << TERM_PTY_ALPHA_SHIFT) | (uint32_t)TERM_PTY_ALPHA_BYTE;

    return col;
}

static uint32_t term_pty_cell_bg_col_get(const ldg_term_cell_t *cell)
{
    uint32_t col = 0;

    col = (uint32_t)cell->bg[0];
    col = (col << TERM_PTY_ALPHA_SHIFT) | (uint32_t)cell->bg[1];
    col = (col << TERM_PTY_ALPHA_SHIFT) | (uint32_t)cell->bg[2];
    col = (col << TERM_PTY_ALPHA_SHIFT) | (uint32_t)TERM_PTY_ALPHA_BYTE;

    return col;
}

static uint8_t term_pty_cell_bg_on_is(const ldg_term_cell_t *cell)
{
    if (cell->bg[0] == 0 && cell->bg[1] == 0 && cell->bg[2] == 0) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static uint32_t term_pty_col_dim(uint32_t col)
{
    uint32_t red = (col >> (TERM_PTY_ALPHA_SHIFT * 0x3)) & (uint32_t)TERM_PTY_ALPHA_BYTE;
    uint32_t green = (col >> (TERM_PTY_ALPHA_SHIFT * 0x2)) & (uint32_t)TERM_PTY_ALPHA_BYTE;
    uint32_t blue = (col >> TERM_PTY_ALPHA_SHIFT) & (uint32_t)TERM_PTY_ALPHA_BYTE;

    red = red * (uint32_t)TERM_PTY_DIM_NUM / (uint32_t)TERM_PTY_DIM_DEN;
    green = green * (uint32_t)TERM_PTY_DIM_NUM / (uint32_t)TERM_PTY_DIM_DEN;
    blue = blue * (uint32_t)TERM_PTY_DIM_NUM / (uint32_t)TERM_PTY_DIM_DEN;

    return (red << (TERM_PTY_ALPHA_SHIFT * 0x3)) | (green << (TERM_PTY_ALPHA_SHIFT * 0x2)) | (blue << TERM_PTY_ALPHA_SHIFT) | (uint32_t)TERM_PTY_ALPHA_BYTE;
}

static void term_pty_cell_style_get(const ldg_term_cell_t *cell, uint8_t screen_reverse, uint32_t *out_fg, uint32_t *out_bg, uint8_t *out_bg_on)
{
    uint32_t fg = term_pty_cell_col_get(cell);
    uint32_t bg = term_pty_cell_bg_col_get(cell);
    uint8_t bg_on = term_pty_cell_bg_on_is(cell);
    uint8_t flip = LDG_TRUTH_FALSE;

    if ((cell->attr & (uint8_t)LDG_TERM_ATTR_REVERSE) != 0) { flip = LDG_TRUTH_TRUE; }

    if (screen_reverse == LDG_TRUTH_TRUE) { flip = (flip == LDG_TRUTH_TRUE) ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE; }

    if (flip == LDG_TRUTH_TRUE)
    {
        uint32_t swap = fg;

        fg = bg;
        bg = swap;
        bg_on = LDG_TRUTH_TRUE;
    }

    if ((cell->attr & (uint8_t)LDG_TERM_ATTR_DIM) != 0) { fg = term_pty_col_dim(fg); }

    if ((cell->attr & (uint8_t)LDG_TERM_ATTR_HIDDEN) != 0) { fg = bg; }

    *out_fg = fg;
    *out_bg = bg;
    *out_bg_on = bg_on;
}

static void term_cell_build(uint8_t val, char *hex, char *ascii)
{
    const char *digit = TERM_HEX_DIGITS;

    hex[0] = digit[(val >> (uint32_t)CROSSTALK_EDA_UI_TERM_NIBBLE_HI) & (uint8_t)LDG_NIBBLE_MASK];
    hex[(uint32_t)TERM_STEP] = digit[val & (uint8_t)LDG_NIBBLE_MASK];
    hex[(uint32_t)TERM_STEP + (uint32_t)TERM_STEP] = LDG_STR_TERM;

    ascii[0] = term_byte_glyph_get(val);
    ascii[(uint32_t)TERM_STEP] = LDG_STR_TERM;
}

uint32_t crosstalk_eda_ui_term_init(crosstalk_eda_ui_term_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->win.open = LDG_TRUTH_FALSE;
    ctx->open = LDG_TRUTH_FALSE;
    ctx->sesh_active = 0x0;
    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_term_shutdown(crosstalk_eda_ui_term_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_term_state_serialize(const crosstalk_eda_ui_term_ctx_t *ctx, uint8_t *out, uint32_t cap, uint32_t *out_len)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (ctx->pty == 0x0 || ctx->pty->live != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    *out_len = (uint32_t)sizeof(uint8_t);

    if (out == 0x0) { return LDG_ERR_AOK; }

    if (cap < (uint32_t)sizeof(uint8_t)) { return LDG_ERR_BOUNDS; }

    out[0] = LDG_TRUTH_TRUE;
    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_term_state_deserialize(const uint8_t *buff, uint32_t len, uint8_t *out_host_live)
{
    if (LDG_UNLIKELY(buff == 0x0 || out_host_live == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_host_live = LDG_TRUTH_FALSE;

    if (len != (uint32_t)sizeof(uint8_t)) { return LDG_ERR_INVALID; }

    *out_host_live = buff[0] != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_term_open_set(crosstalk_eda_ui_term_ctx_t *ctx, uint8_t open)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->open = open;
    ctx->win.open = open;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_ui_term_open_is(const crosstalk_eda_ui_term_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->win.open != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_eda_ui_term_focus_set(crosstalk_eda_ui_term_ctx_t *ctx, uint8_t focus)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->focus = focus;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_ui_term_focus_is(const crosstalk_eda_ui_term_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->focus == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint8_t crosstalk_eda_ui_term_serial_live_is(const crosstalk_eda_ui_term_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->sesh_active >= CROSSTALK_EDA_UI_TERM_SESH_MAX) { return LDG_TRUTH_FALSE; }

    if (ctx->sesh[ctx->sesh_active].src != (uint32_t)CROSSTALK_EDA_UI_TERM_SRC_SERIAL) { return LDG_TRUTH_FALSE; }

    return ctx->sesh[ctx->sesh_active].live == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

const char* crosstalk_eda_ui_term_title_get(const crosstalk_eda_ui_term_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return ""; }

    if (ctx->sesh_active >= CROSSTALK_EDA_UI_TERM_SESH_MAX) { return ""; }

    return ctx->sesh[ctx->sesh_active].title;
}

uint32_t crosstalk_eda_ui_term_sesh_bind(crosstalk_eda_ui_term_ctx_t *ctx, uint32_t sesh_idx, crosstalk_eda_ui_term_src_t src, uint32_t src_handle, const char *title)
{
    crosstalk_eda_ui_term_sesh_t *sesh = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || title == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (sesh_idx >= (uint32_t)CROSSTALK_EDA_UI_TERM_SESH_MAX) { return CROSSTALK_EDA_UI_TERM_ERR_SESH; }

    sesh = &ctx->sesh[sesh_idx];
    sesh->src = (uint32_t)src;
    sesh->src_handle = src_handle;
    sesh->owner = ctx;
    sesh->self_idx = sesh_idx;
    sesh->live = src != CROSSTALK_EDA_UI_TERM_SRC_NONE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    ret = ldg_str_rbrcpy(sesh->title, title, (uint64_t)sizeof(sesh->title));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    sesh->sgr_idx = (uint8_t)CROSSTALK_EDA_UI_TERM_SGR_IDX_NONE;
    sesh->sgr_state = (uint32_t)TERM_SGR_STATE_IDLE;
    sesh->sgr_bold = LDG_TRUTH_FALSE;

    if (src != CROSSTALK_EDA_UI_TERM_SRC_PTY && src != CROSSTALK_EDA_UI_TERM_SRC_NONE && sesh->term == 0x0 && ctx->pool != 0x0)
    {
        ldg_term_conf_t grid_conf = LDG_STRUCT_ZERO_INIT;

        if (ldg_term_conf_default_get(&grid_conf) == LDG_ERR_AOK)
        {
            grid_conf.col_cunt = CROSSTALK_EDA_UI_TERM_COOKED_COLS;
            grid_conf.row_cunt = CROSSTALK_EDA_UI_TERM_COOKED_ROWS;

            if (ldg_term_create(ctx->pool, &grid_conf, &sesh->term) == LDG_ERR_AOK && ldg_term_vt_init(&sesh->vt, sesh->term) == LDG_ERR_AOK)
            {
                sesh->cooked = LDG_TRUTH_TRUE;

                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "term: session grid ready; sesh: %u32; cols: %u32; rows: %u32", sesh_idx, (uint32_t)grid_conf.col_cunt, (uint32_t)grid_conf.row_cunt);
            }
            else
            {
                sesh->term = 0x0;

                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term: session grid refused; sesh: %u32; pool left: %u64", sesh_idx, ldg_mem_pool_remaining_get(ctx->pool));
            }
        }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_term_sesh_next_get(const crosstalk_eda_ui_term_ctx_t *ctx, uint32_t *out)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;

    for (idx = 0; idx < (uint32_t)CROSSTALK_EDA_UI_TERM_SESH_MAX; idx++) { if (ctx->sesh[idx].src == (uint32_t)CROSSTALK_EDA_UI_TERM_SRC_NONE)
        {
            *out = idx;
            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_FULL;
}

uint32_t crosstalk_eda_ui_term_sesh_src_find(const crosstalk_eda_ui_term_ctx_t *ctx, crosstalk_eda_ui_term_src_t src, uint32_t *out)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;

    for (idx = 0; idx < (uint32_t)CROSSTALK_EDA_UI_TERM_SESH_MAX; idx++) { if (ctx->sesh[idx].src == (uint32_t)src)
        {
            *out = idx;
            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t crosstalk_eda_ui_term_sesh_close(crosstalk_eda_ui_term_ctx_t *ctx, uint32_t sesh_idx)
{
    crosstalk_eda_ui_term_sesh_t *sesh = 0x0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (sesh_idx >= (uint32_t)CROSSTALK_EDA_UI_TERM_SESH_MAX) { return CROSSTALK_EDA_UI_TERM_ERR_SESH; }

    sesh = &ctx->sesh[sesh_idx];
    sesh->src = (uint32_t)CROSSTALK_EDA_UI_TERM_SRC_NONE;
    sesh->src_handle = 0x0;
    sesh->live = LDG_TRUTH_FALSE;
    sesh->title[0] = LDG_STR_TERM;
    sesh->raw_hd = 0x0;
    sesh->raw_cunt = 0x0;
    sesh->bytes_total = 0x0;

    if (ctx->sesh_active == sesh_idx)
    {
        for (idx = 0; idx < (uint32_t)CROSSTALK_EDA_UI_TERM_SESH_MAX; idx++) { if (ctx->sesh[idx].src != (uint32_t)CROSSTALK_EDA_UI_TERM_SRC_NONE)
            {
                ctx->sesh_active = idx;
                return LDG_ERR_AOK;
            }
        }

        ctx->sesh_active = 0;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_term_active_set(crosstalk_eda_ui_term_ctx_t *ctx, uint32_t sesh_idx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (sesh_idx >= (uint32_t)CROSSTALK_EDA_UI_TERM_SESH_MAX) { return CROSSTALK_EDA_UI_TERM_ERR_SESH; }

    if (ctx->sesh[sesh_idx].src == (uint32_t)CROSSTALK_EDA_UI_TERM_SRC_NONE) { return CROSSTALK_EDA_UI_TERM_ERR_SESH; }

    ctx->sesh_active = sesh_idx;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_term_sesh_ptr_get(crosstalk_eda_ui_term_ctx_t *ctx, uint32_t sesh_idx, crosstalk_eda_ui_term_sesh_t **out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (sesh_idx >= (uint32_t)CROSSTALK_EDA_UI_TERM_SESH_MAX) { return CROSSTALK_EDA_UI_TERM_ERR_SESH; }

    *out = &ctx->sesh[sesh_idx];

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_term_sesh_sink_cb(void *user, const uint8_t *bytes, uint32_t len)
{
    crosstalk_eda_ui_term_sesh_t *sesh = (crosstalk_eda_ui_term_sesh_t *)user;

    if (LDG_UNLIKELY(sesh == 0x0 || bytes == 0x0 || sesh->owner == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_eda_ui_term_bytes_put((crosstalk_eda_ui_term_ctx_t *)sesh->owner, sesh->self_idx, bytes, len);
}

uint32_t crosstalk_eda_ui_term_sesh_clear(crosstalk_eda_ui_term_ctx_t *ctx, uint32_t sesh_idx)
{
    crosstalk_eda_ui_term_sesh_t *sesh = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (sesh_idx >= (uint32_t)CROSSTALK_EDA_UI_TERM_SESH_MAX) { return CROSSTALK_EDA_UI_TERM_ERR_SESH; }

    sesh = &ctx->sesh[sesh_idx];
    sesh->raw_hd = 0x0;
    sesh->raw_cunt = 0x0;
    sesh->bytes_total = 0x0;
    sesh->sgr_idx = (uint8_t)CROSSTALK_EDA_UI_TERM_SGR_IDX_NONE;
    sesh->sgr_state = (uint32_t)TERM_SGR_STATE_IDLE;
    sesh->sgr_bold = LDG_TRUTH_FALSE;

    if (sesh->term != 0x0) { if (ldg_term_rst(sesh->term) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term session grid reset failed"); } }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_term_bytes_put(crosstalk_eda_ui_term_ctx_t *ctx, uint32_t sesh_idx, const uint8_t *bytes, uint32_t len)
{
    crosstalk_eda_ui_term_sesh_t *sesh = 0x0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || bytes == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (sesh_idx >= (uint32_t)CROSSTALK_EDA_UI_TERM_SESH_MAX) { return CROSSTALK_EDA_UI_TERM_ERR_SESH; }

    sesh = &ctx->sesh[sesh_idx];

    for (idx = 0; idx < len; idx++) { term_raw_put(sesh, bytes[idx]); }

    sesh->bytes_total = sesh->bytes_total + (uint64_t)len;

    if (sesh->term != 0x0) { if (ldg_term_vt_feed(&sesh->vt, bytes, (uint64_t)len) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term session vt feed failed"); } }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_term_sink_cb(void *user, const uint8_t *bytes, uint32_t len)
{
    crosstalk_eda_ui_term_ctx_t *ctx = (crosstalk_eda_ui_term_ctx_t *)user;
    uint32_t sesh_idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || bytes == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (crosstalk_eda_ui_term_sesh_src_find(ctx, CROSSTALK_EDA_UI_TERM_SRC_SERIAL, &sesh_idx) != LDG_ERR_AOK) { sesh_idx = ctx->sesh_active; }

    return crosstalk_eda_ui_term_bytes_put(ctx, sesh_idx, bytes, len);
}

static uint32_t term_arrow_put(uint8_t *out, uint32_t cap, uint32_t cunt, uint8_t final_byte)
{
    if (cunt + (uint32_t)CROSSTALK_EDA_UI_TERM_ARROW_BYTES > cap) { return cunt; }

    out[cunt] = (uint8_t)LDG_ASCII_ESC;
    out[cunt + (uint32_t)TERM_STEP] = (uint8_t)CROSSTALK_EDA_UI_TERM_CSI;
    out[cunt + (uint32_t)TERM_STEP + (uint32_t)TERM_STEP] = final_byte;

    return cunt + (uint32_t)CROSSTALK_EDA_UI_TERM_ARROW_BYTES;
}

static uint32_t term_byte_out_put(crosstalk_eda_ui_term_ctx_t *ctx, uint8_t val)
{
    if (ctx->key_out_cunt >= (uint32_t)CROSSTALK_EDA_UI_TERM_KEY_OUT_CAP) { return LDG_ERR_FULL; }

    ctx->key_out[ctx->key_out_cunt] = val;
    ctx->key_out_cunt = ctx->key_out_cunt + (uint32_t)TERM_STEP;

    return LDG_ERR_AOK;
}

static uint32_t term_raw_key_put(crosstalk_eda_ui_term_ctx_t *ctx, const ldg_term_key_event_t *key)
{
    uint32_t at = 0;
    uint32_t ret = LDG_ERR_AOK;

    if (key->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE) { return LDG_ERR_AOK; }

    if (key->key == (uint8_t)LDG_TERM_KEY_ENTER) { return term_byte_out_put(ctx, (uint8_t)LDG_ASCII_CR); }

    if (key->key == (uint8_t)LDG_TERM_KEY_BACKSPACE) { return term_byte_out_put(ctx, (uint8_t)CROSSTALK_EDA_UI_TERM_BS); }

    if (key->key == (uint8_t)LDG_TERM_KEY_TAB) { return term_byte_out_put(ctx, (uint8_t)LDG_ASCII_TAB); }

    if (key->key == (uint8_t)LDG_TERM_KEY_ESCAPE) { return term_byte_out_put(ctx, (uint8_t)LDG_ASCII_ESC); }

    if (key->key == (uint8_t)LDG_TERM_KEY_UP) { ctx->key_out_cunt = term_arrow_put(ctx->key_out, (uint32_t)CROSSTALK_EDA_UI_TERM_KEY_OUT_CAP, ctx->key_out_cunt, (uint8_t)CROSSTALK_EDA_UI_TERM_ARROW_UP); return LDG_ERR_AOK; }

    if (key->key == (uint8_t)LDG_TERM_KEY_DOWN) { ctx->key_out_cunt = term_arrow_put(ctx->key_out, (uint32_t)CROSSTALK_EDA_UI_TERM_KEY_OUT_CAP, ctx->key_out_cunt, (uint8_t)CROSSTALK_EDA_UI_TERM_ARROW_DOWN); return LDG_ERR_AOK; }

    if (key->key == (uint8_t)LDG_TERM_KEY_RHS) { ctx->key_out_cunt = term_arrow_put(ctx->key_out, (uint32_t)CROSSTALK_EDA_UI_TERM_KEY_OUT_CAP, ctx->key_out_cunt, (uint8_t)CROSSTALK_EDA_UI_TERM_ARROW_RHS); return LDG_ERR_AOK; }

    if (key->key == (uint8_t)LDG_TERM_KEY_LHS) { ctx->key_out_cunt = term_arrow_put(ctx->key_out, (uint32_t)CROSSTALK_EDA_UI_TERM_KEY_OUT_CAP, ctx->key_out_cunt, (uint8_t)CROSSTALK_EDA_UI_TERM_ARROW_LHS); return LDG_ERR_AOK; }

    for (at = 0; at < key->text_cunt && at < (uint32_t)LDG_TERM_KEY_TEXT_MAX && ret == LDG_ERR_AOK; at = at + (uint32_t)TERM_STEP)
    {
        if (key->text[at] > (uint32_t)LDG_ASCII_PRINTABLE_MAX) { continue; }

        ret = term_byte_out_put(ctx, (uint8_t)key->text[at]);
    }

    return ret;
}

static uint32_t term_key_isr(uint64_t win_id, void *user, const ldg_term_key_event_t *key, uint8_t *consumed)
{
    crosstalk_eda_ui_term_ctx_t *ctx = (crosstalk_eda_ui_term_ctx_t *)user;
    uint64_t enc_len = 0;
    uint32_t src = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || key == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    if (win_id != (uint64_t)CROSSTALK_EDA_UI_TERM_WINDOW_ID) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->sesh_active >= (uint32_t)CROSSTALK_EDA_UI_TERM_SESH_MAX) { return LDG_ERR_AOK; }

    src = ctx->sesh[ctx->sesh_active].src;

    if (src == (uint32_t)CROSSTALK_EDA_UI_TERM_SRC_NONE) { return LDG_ERR_AOK; }

    *consumed = LDG_TRUTH_TRUE;

    if (src != (uint32_t)CROSSTALK_EDA_UI_TERM_SRC_PTY || ctx->pty == 0x0) { return term_raw_key_put(ctx, key); }

    if (ctx->key_out_cunt >= (uint32_t)CROSSTALK_EDA_UI_TERM_KEY_OUT_CAP) { return LDG_ERR_FULL; }

    if (ldg_term_key_encode(&ctx->pty->vt, key, ctx->key_out + ctx->key_out_cunt, (uint64_t)((uint32_t)CROSSTALK_EDA_UI_TERM_KEY_OUT_CAP - ctx->key_out_cunt), &enc_len) != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    ctx->key_out_cunt = ctx->key_out_cunt + (uint32_t)enc_len;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_term_keys_drain(crosstalk_eda_ui_term_ctx_t *ctx, uint8_t *out, uint32_t cap, uint32_t *out_cunt)
{
    uint32_t take = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0 || out_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    take = ctx->key_out_cunt < cap ? ctx->key_out_cunt : cap;

    if (take == 0) { return LDG_ERR_AOK; }

    memcpy(out, ctx->key_out, (uint64_t)take);
    memmove(ctx->key_out, ctx->key_out + take, (uint64_t)(ctx->key_out_cunt - take));

    ctx->key_out_cunt = ctx->key_out_cunt - take;
    *out_cunt = take;

    return LDG_ERR_AOK;
}

static uint32_t term_span_put(ldg_term_render_t *render, uint32_t col, uint32_t row, const char *text, uint64_t len, uint32_t fg_rgba, uint32_t bg_rgba)
{
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t bg = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (len == 0) { return LDG_ERR_AOK; }

    ret = crosstalk_ui_cell_color_get(fg_rgba, &fg);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get(bg_rgba, &bg); }

    if (ret != LDG_ERR_AOK) { return ret; }

    return ldg_term_render_text_draw(render, col, row, text, len, &fg, &bg);
}

static uint32_t term_img_rgba_get(const uint8_t *pixel, uint64_t at, uint32_t px_bytes)
{
    uint64_t base = at * (uint64_t)px_bytes;

    return ((uint32_t)pixel[base] << (uint32_t)CROSSTALK_UI_CELL_RGBA_RED_SHIFT) | ((uint32_t)pixel[base + (uint64_t)TERM_IMG_GREEN_AT] << (uint32_t)CROSSTALK_UI_CELL_RGBA_GREEN_SHIFT) | ((uint32_t)pixel[base + (uint64_t)TERM_IMG_BLUE_AT] << (uint32_t)CROSSTALK_UI_CELL_RGBA_BLUE_SHIFT) | (uint32_t)TERM_PTY_ALPHA_BYTE;
}

static LDG_NOINLINE uint32_t term_img_draw(ldg_term_render_t *render, const ldg_term_img_t *img, const uint8_t *pixel, uint32_t pixel_size, const ldg_term_placement_t *placement, uint32_t row_base)
{
    uint32_t cell_w = render->sample_col_cunt;
    uint32_t cell_h = render->sample_row_cunt;
    uint32_t out_w = placement->col_span * cell_w;
    uint32_t out_h = placement->row_span * cell_h;
    uint32_t pos_x = placement->col * cell_w;
    uint32_t pos_y = (row_base + placement->row) * cell_h;
    uint32_t px_bytes = 0;
    uint32_t sy = 0;
    uint32_t ret = 0;

    if (img->fmt == (uint8_t)LDG_TERM_IMG_FMT_RGBA) { px_bytes = (uint32_t)CROSSTALK_EDA_UI_TERM_IMG_RGBA_BYTES; }
    else if (img->fmt == (uint8_t)LDG_TERM_IMG_FMT_RGB) { px_bytes = (uint32_t)CROSSTALK_EDA_UI_TERM_IMG_RGB_BYTES; }
    else { return LDG_ERR_UNSUPPORTED; }

    if (img->width == 0 || img->height == 0 || out_w == 0 || out_h == 0) { return LDG_ERR_AOK; }

    if ((uint64_t)pixel_size < (uint64_t)img->width * (uint64_t)img->height * (uint64_t)px_bytes) { return LDG_ERR_BOUNDS; }

    for (sy = 0; sy < out_h; sy++)
    {
        uint64_t py = ((uint64_t)sy * (uint64_t)img->height) / (uint64_t)out_h;
        uint64_t row_at = py * (uint64_t)img->width;
        uint32_t run_x = 0;
        uint32_t run_rgba = term_img_rgba_get(pixel, row_at, px_bytes);
        uint32_t sx = 0;

        for (sx = (uint32_t)TERM_STEP; sx <= out_w; sx++)
        {
            uint32_t rgba = sx < out_w ? term_img_rgba_get(pixel, row_at + ((uint64_t)sx * (uint64_t)img->width) / (uint64_t)out_w, px_bytes) : ~run_rgba;

            if (rgba == run_rgba) { continue; }

            ret = crosstalk_ui_cell_sample_fill(render, pos_x + run_x, pos_y + sy, sx - run_x, (uint32_t)TERM_IMG_SAMPLE_W, run_rgba);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            run_x = sx;
            run_rgba = rgba;
        }
    }

    return LDG_ERR_AOK;
}

static LDG_NOINLINE uint32_t term_placement_draw(ldg_term_render_t *render, const ldg_term_t *term, uint32_t row_base, uint8_t front)
{
    ldg_term_placement_t placement = LDG_STRUCT_ZERO_INIT;
    ldg_term_img_t img = LDG_STRUCT_ZERO_INIT;
    const uint8_t *pixel = 0x0;
    uint32_t pixel_size = 0;
    uint32_t cunt = 0;
    uint32_t idx = 0;

    if (ldg_term_placement_cunt_get(term, &cunt) != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    for (idx = 0; idx < cunt; idx = idx + (uint32_t)TERM_STEP)
    {
        if (ldg_term_placement_get(term, idx, &placement) != LDG_ERR_AOK) { continue; }

        if (placement.alive != LDG_TRUTH_TRUE) { continue; }

        if (front == LDG_TRUTH_TRUE && placement.layer < 0) { continue; }

        if (front != LDG_TRUTH_TRUE && placement.layer >= 0) { continue; }

        if (ldg_term_img_get(term, placement.img_id, &img) != LDG_ERR_AOK) { continue; }

        if (ldg_term_img_pixel_get(term, placement.img_id, &pixel, &pixel_size) != LDG_ERR_AOK || pixel == 0x0) { continue; }

        if (term_img_draw(render, &img, pixel, pixel_size, &placement, row_base) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term img draw failed"); }
    }

    return LDG_ERR_AOK;
}

static LDG_NOINLINE uint32_t term_pty_grid_draw(crosstalk_eda_ui_term_ctx_t *ctx, ldg_term_render_t *render, ldg_term_t *grid, const crosstalk_eda_io_pty_ctx_t *exit_pty, uint32_t row_base, uint32_t col_cunt, uint32_t row_avail)
{
    ldg_term_cell_t row_cell[CROSSTALK_EDA_IO_PTY_COL_MAX];
    char run[TERM_PTY_RUN_CAP] = LDG_ARR_ZERO_INIT;
    char status[CROSSTALK_EDA_UI_TERM_STATUS_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t cursor_col = 0;
    uint32_t cursor_row = 0;
    uint32_t cols = 0;
    uint32_t rows = 0;
    uint32_t row = 0;
    uint32_t ret = 0;
    uint8_t screen_reverse = LDG_TRUTH_FALSE;
    uint8_t cursor_on = LDG_TRUTH_FALSE;

    if (grid == 0x0) { return crosstalk_ui_cell_rgba_put(render, (uint32_t)TERM_ORIG_COL, row_base, TERM_PTY_DEAD_TEXT, (uint64_t)CROSSTALK_EDA_UI_TERM_STATUS_CAP, (uint32_t)CROSSTALK_UI_THEME_STATUS_RGBA, 0x0); }

    screen_reverse = ldg_term_reverse_is(grid);
    cols = grid->col_cunt;
    rows = grid->row_cunt;

    if (cols > (uint32_t)CROSSTALK_EDA_IO_PTY_COL_MAX) { cols = (uint32_t)CROSSTALK_EDA_IO_PTY_COL_MAX; }

    if (cols > col_cunt) { cols = col_cunt; }

    if (rows > row_avail) { rows = row_avail; }

    ctx->pty_cols_want = col_cunt < (uint32_t)CROSSTALK_EDA_IO_PTY_COL_MAX ? col_cunt : (uint32_t)CROSSTALK_EDA_IO_PTY_COL_MAX;
    ctx->pty_rows_want = row_avail < (uint32_t)CROSSTALK_EDA_IO_PTY_ROW_MAX ? row_avail : (uint32_t)CROSSTALK_EDA_IO_PTY_ROW_MAX;

    if (cols == 0) { return LDG_ERR_AOK; }

    if (ctx->pty_draw_logged == LDG_TRUTH_FALSE && exit_pty != 0x0 && exit_pty->rd_total > 0)
    {
        ctx->pty_draw_logged = LDG_TRUTH_TRUE;
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "term: pty draw; rows: %u32; cols: %u32; avail: %u32x%u32", rows, cols, col_cunt, row_avail);
    }

    if (term_placement_draw(render, grid, row_base, LDG_TRUTH_FALSE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term placement back draw failed"); }

    if ((exit_pty == 0x0 || exit_pty->exited != LDG_TRUTH_TRUE) && ldg_term_cursor_visible_is(grid) == LDG_TRUTH_TRUE && ldg_term_cursor_get(grid, &cursor_col, &cursor_row) == LDG_ERR_AOK && cursor_row < rows && cursor_col < cols) { cursor_on = LDG_TRUTH_TRUE; }

    for (row = 0; row < rows; row++)
    {
        uint32_t at = 0;

        ret = ldg_term_row_get(grid, row, row_cell, (uint32_t)CROSSTALK_EDA_IO_PTY_COL_MAX);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term: pty row get failed; row: %u32; err: %u32", row, ret); return ret; }

        while (at < cols)
        {
            uint32_t run_start = at;
            uint32_t run_len = 0;
            uint32_t run_col = 0;
            uint32_t run_bg = 0;
            uint32_t cell_col = 0;
            uint32_t cell_bg = 0;
            uint8_t run_bg_on = LDG_TRUTH_FALSE;
            uint8_t cell_bg_on = LDG_TRUTH_FALSE;

            term_pty_cell_style_get(&row_cell[at], screen_reverse, &run_col, &run_bg, &run_bg_on);

            while (at < cols && run_len < (uint32_t)TERM_PTY_COL_CAP)
            {
                uint8_t ch = row_cell[at].ch;

                term_pty_cell_style_get(&row_cell[at], screen_reverse, &cell_col, &cell_bg, &cell_bg_on);

                if (cell_col != run_col || cell_bg != run_bg || cell_bg_on != run_bg_on) { break; }

                if (ch < (uint8_t)LDG_ASCII_SPACE || ch > (uint8_t)LDG_ASCII_PRINTABLE_MAX) { ch = (uint8_t)LDG_ASCII_SPACE; }

                run[run_len] = (char)ch;
                run_len = run_len + (uint32_t)TERM_STEP;
                at = at + (uint32_t)TERM_STEP;
            }

            ret = term_span_put(render, run_start, row_base + row, run, (uint64_t)run_len, run_col, run_bg_on == LDG_TRUTH_TRUE ? run_bg : (uint32_t)CROSSTALK_UI_CELL_PANE_RGBA);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        if (cursor_on == LDG_TRUTH_TRUE && row == cursor_row)
        {
            char ch = (char)row_cell[cursor_col].ch;

            if ((uint8_t)ch < (uint8_t)LDG_ASCII_SPACE || (uint8_t)ch > (uint8_t)LDG_ASCII_PRINTABLE_MAX) { ch = (char)LDG_ASCII_SPACE; }

            ret = term_span_put(render, cursor_col, row_base + row, &ch, (uint64_t)TERM_PTY_CURSOR_W, (uint32_t)CROSSTALK_UI_CELL_PANE_RGBA, (uint32_t)CROSSTALK_UI_THEME_ACCENT_RGBA);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }

    if (term_placement_draw(render, grid, row_base, LDG_TRUTH_TRUE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term placement front draw failed"); }

    if (exit_pty == 0x0 || exit_pty->exited != LDG_TRUTH_TRUE || rows >= row_avail) { return LDG_ERR_AOK; }

    if (snprintf(status, sizeof(status), TERM_PTY_EXIT_FMT, exit_pty->exit_code) < 0) { return LDG_ERR_STR_FMT; }

    return crosstalk_ui_cell_rgba_put(render, (uint32_t)TERM_ORIG_COL, row_base + rows, status, (uint64_t)sizeof(status), (uint32_t)CROSSTALK_UI_THEME_STATUS_RGBA, 0x0);
}

static LDG_NOINLINE uint32_t term_raw_draw(const crosstalk_eda_ui_term_sesh_t *sesh, ldg_term_render_t *render, uint32_t row_base, uint32_t col_cunt, uint32_t row_cunt)
{
    char ascii[CROSSTALK_EDA_UI_TERM_CELL_CAP] = LDG_ARR_ZERO_INIT;
    char hex[CROSSTALK_EDA_UI_TERM_CELL_CAP] = LDG_ARR_ZERO_INIT;
    char offset[CROSSTALK_EDA_UI_TERM_CELL_CAP] = LDG_ARR_ZERO_INIT;
    char status[CROSSTALK_EDA_UI_TERM_STATUS_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t cell_w = render->sample_col_cunt;
    uint32_t cell_h = render->sample_row_cunt;
    uint32_t per_byte = (uint32_t)CROSSTALK_EDA_UI_TERM_ASCII_PER_BYTE + (uint32_t)CROSSTALK_EDA_UI_TERM_HEX_PER_BYTE;
    uint32_t fixed_w = (uint32_t)CROSSTALK_EDA_UI_TERM_OFF_CHARS + (uint32_t)CROSSTALK_EDA_UI_TERM_SEP_COLS * ((uint32_t)CROSSTALK_EDA_UI_TERM_PANE_CUNT - (uint32_t)TERM_STEP);
    uint32_t ascii_col = (uint32_t)CROSSTALK_EDA_UI_TERM_OFF_CHARS + (uint32_t)CROSSTALK_EDA_UI_TERM_SEP_COLS;
    uint32_t status_row = row_cunt - (uint32_t)TERM_STATUS_ROWS;
    uint32_t bytes_row = 0;
    uint32_t rows_fit = 0;
    uint32_t hex_col = 0;
    uint32_t first = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (sesh->live == LDG_TRUTH_TRUE) { snprintf(status, sizeof(status), TERM_STATUS_FMT, sesh->title, sesh->bytes_total); }
    else { snprintf(status, sizeof(status), TERM_STATUS_IDLE); }

    ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)TERM_ORIG_COL, status_row, status, (uint64_t)sizeof(status), (uint32_t)CROSSTALK_UI_THEME_STATUS_RGBA, 0x0);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (col_cunt <= fixed_w || status_row <= row_base) { return LDG_ERR_AOK; }

    bytes_row = (col_cunt - fixed_w) / per_byte;
    if (bytes_row > (uint32_t)CROSSTALK_EDA_UI_TERM_BYTE_ROW_MAX) { bytes_row = (uint32_t)CROSSTALK_EDA_UI_TERM_BYTE_ROW_MAX; }

    bytes_row = (bytes_row / (uint32_t)CROSSTALK_EDA_UI_TERM_BYTE_ROW_QUANT) * (uint32_t)CROSSTALK_EDA_UI_TERM_BYTE_ROW_QUANT;

    if (bytes_row < (uint32_t)CROSSTALK_EDA_UI_TERM_BYTE_ROW_MIN) { return LDG_ERR_AOK; }

    rows_fit = status_row - row_base;
    hex_col = ascii_col + bytes_row * (uint32_t)CROSSTALK_EDA_UI_TERM_ASCII_PER_BYTE + (uint32_t)CROSSTALK_EDA_UI_TERM_SEP_COLS;

    ret = crosstalk_ui_cell_sample_fill(render, ascii_col * cell_w - ((uint32_t)CROSSTALK_EDA_UI_TERM_SEP_COLS * cell_w) / (uint32_t)TERM_SEP_CENTER_DIV, row_base * cell_h, (uint32_t)TERM_SEP_W_SAMPLES, rows_fit * cell_h, (uint32_t)CROSSTALK_UI_THEME_SEP_RGBA);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_sample_fill(render, hex_col * cell_w - ((uint32_t)CROSSTALK_EDA_UI_TERM_SEP_COLS * cell_w) / (uint32_t)TERM_SEP_CENTER_DIV, row_base * cell_h, (uint32_t)TERM_SEP_W_SAMPLES, rows_fit * cell_h, (uint32_t)CROSSTALK_UI_THEME_SEP_RGBA); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (sesh->raw_cunt > rows_fit * bytes_row) { first = sesh->raw_cunt - rows_fit * bytes_row; }

    first = (first / bytes_row) * bytes_row;

    for (idx = first; idx < sesh->raw_cunt; idx = idx + bytes_row)
    {
        uint32_t cunt = sesh->raw_cunt - idx;
        uint32_t row = row_base + (idx - first) / bytes_row;
        uint64_t off = sesh->bytes_total - (uint64_t)sesh->raw_cunt + (uint64_t)idx;
        uint32_t run = 0;
        uint32_t best = 0;
        uint32_t at = 0;

        if (cunt > bytes_row) { cunt = bytes_row; }

        if (snprintf(offset, sizeof(offset), CROSSTALK_EDA_UI_TERM_OFF_FMT, off) < 0) { return LDG_ERR_STR_FMT; }

        for (at = 0; at < cunt; at++)
        {
            uint8_t val = term_raw_at_get(sesh, idx + at);
            uint8_t col_idx = term_raw_col_at_get(sesh, idx + at);

            if (col_idx != (uint8_t)CROSSTALK_EDA_UI_TERM_SGR_IDX_CTRL && val >= (uint8_t)LDG_ASCII_SPACE && val <= (uint8_t)LDG_ASCII_PRINTABLE_MAX) { run = run + (uint32_t)TERM_STEP; }
            else { run = 0; }

            if (run > best) { best = run; }
        }

        ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)TERM_ORIG_COL, row, offset, (uint64_t)sizeof(offset), best >= (uint32_t)CROSSTALK_EDA_UI_TERM_OFF_TEXT_RUN ? (uint32_t)CROSSTALK_UI_THEME_LIT_RGBA : (uint32_t)CROSSTALK_UI_THEME_DEAD_RGBA, 0x0);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        for (at = 0; at < cunt; at++)
        {
            uint8_t val = term_raw_at_get(sesh, idx + at);
            uint8_t col_idx = term_raw_col_at_get(sesh, idx + at);
            uint32_t col = term_byte_col_get(val);
            uint32_t text_col = term_sgr_col_get(sesh, col_idx, col);

            term_cell_build(val, hex, ascii);

            if (col_idx == (uint8_t)CROSSTALK_EDA_UI_TERM_SGR_IDX_CTRL)
            {
                ascii[0] = (char)CROSSTALK_EDA_UI_TERM_DOT;
                text_col = (uint32_t)CROSSTALK_UI_THEME_ACCENT_RGBA;
            }

            ret = crosstalk_ui_cell_rgba_put(render, ascii_col + at, row, ascii, (uint64_t)sizeof(ascii), text_col, 0x0);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_ui_cell_rgba_put(render, hex_col + at * (uint32_t)CROSSTALK_EDA_UI_TERM_HEX_PER_BYTE, row, hex, (uint64_t)sizeof(hex), col, 0x0);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }

    return LDG_ERR_AOK;
}

static LDG_NOINLINE uint32_t term_tabs_draw(const crosstalk_eda_ui_term_ctx_t *ctx, ldg_term_render_t *render, uint32_t *out_rows)
{
    char tab_text[CROSSTALK_EDA_UI_TERM_TITLE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t pos_col = TERM_ORIG_COL;
    uint32_t seg_w = 0;
    uint32_t tab_idx = 0;
    uint32_t ret = 0;

    *out_rows = 0;

    for (tab_idx = 0; tab_idx < (uint32_t)CROSSTALK_EDA_UI_TERM_SESH_MAX; tab_idx++)
    {
        if (ctx->sesh[tab_idx].src == (uint32_t)CROSSTALK_EDA_UI_TERM_SRC_NONE) { continue; }

        if (snprintf(tab_text, sizeof(tab_text), TERM_TAB_FMT, tab_idx == ctx->sesh_active ? TERM_TAB_MARK : "", tab_idx, ctx->sesh[tab_idx].title) < 0) { return LDG_ERR_STR_FMT; }

        ret = crosstalk_ui_cell_rgba_put(render, pos_col, (uint32_t)TERM_TAB_ROW, tab_text, (uint64_t)sizeof(tab_text), tab_idx == ctx->sesh_active ? (uint32_t)CROSSTALK_UI_THEME_LIT_RGBA : (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, &seg_w);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        pos_col = pos_col + seg_w + (uint32_t)TERM_TAB_GAP;
        *out_rows = (uint32_t)TERM_STEP;
    }

    return LDG_ERR_AOK;
}

static uint32_t term_draw_cb(uint64_t win_id, void *user, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    crosstalk_eda_ui_term_ctx_t *ctx = (crosstalk_eda_ui_term_ctx_t *)user;
    const crosstalk_eda_io_pty_ctx_t *pty = 0x0;
    uint32_t row_base = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    pty = ctx->pty;
    if (LDG_UNLIKELY(pty == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (win_id != (uint64_t)CROSSTALK_EDA_UI_TERM_WINDOW_ID || col_cunt == 0 || row_cunt == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    ctx->win.content_row_cunt = row_cunt;

    if (pty->term != 0x0)
    {
        char vt_title[CROSSTALK_EDA_UI_TERM_TITLE_CAP] = LDG_ARR_ZERO_INIT;

        if (ldg_term_vt_title_get(&pty->vt, vt_title, (uint64_t)sizeof(vt_title)) == LDG_ERR_AOK && vt_title[0] != LDG_STR_TERM && strncmp(vt_title, ctx->title_seen, sizeof(vt_title)) != 0)
        {
            if (ldg_str_rbrcpy(ctx->title_seen, vt_title, (uint64_t)sizeof(ctx->title_seen)) != LDG_ERR_AOK) { ctx->title_seen[0] = LDG_STR_TERM; }

            if (ctx->win_nav != 0x0 && crosstalk_ui_window_title_set(ctx->win_nav, (uint64_t)CROSSTALK_EDA_UI_TERM_WINDOW_ID, vt_title) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term title set failed"); }
        }
    }

    ret = term_tabs_draw(ctx, render, &row_base);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (row_cunt <= row_base || ctx->sesh_active >= (uint32_t)CROSSTALK_EDA_UI_TERM_SESH_MAX) { return LDG_ERR_AOK; }

    if (ctx->sesh[ctx->sesh_active].src == (uint32_t)CROSSTALK_EDA_UI_TERM_SRC_PTY) { return term_pty_grid_draw(ctx, render, pty->term, pty, row_base, col_cunt, row_cunt - row_base); }

    return term_raw_draw(&ctx->sesh[ctx->sesh_active], render, row_base, col_cunt, row_cunt);
}

uint32_t crosstalk_eda_ui_term_win_reg(crosstalk_eda_ui_term_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, ldg_mem_pool_t *pool, const crosstalk_eda_io_pty_ctx_t *pty, uint32_t *out_slot)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || win_nav == 0x0 || pty == 0x0 || out_slot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->pty = pty;
    ctx->pool = pool;
    ctx->win_nav = win_nav;

    ret = crosstalk_ui_window_reg(win_nav, (uint64_t)CROSSTALK_EDA_UI_TERM_WINDOW_ID, CROSSTALK_EDA_UI_TERM_WINDOW_TITLE, &ctx->win, term_draw_cb, term_key_isr, ctx, out_slot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->slot = *out_slot;

    return LDG_ERR_AOK;
}
