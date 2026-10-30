#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>
#include <dangling/sys/clipboard.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "eda/io/pty.h"

#define PTY_TITLE_DEFAULT "host"
#define PTY_RD_ROUND_MAX 8
#define PTY_REPLY_CAP 256
#define PTY_PASTE_CAP 4096
#define PTY_PASTE_ENC_CAP 6144
#define PTY_TERM_NAME "xterm-kitty"
#define PTY_QUIET_TICK_REPORT 300
#define PTY_DUMP_BYTES 32
#define PTY_DUMP_STRIDE 3
#define PTY_DUMP_CAP (PTY_DUMP_BYTES * PTY_DUMP_STRIDE + LDG_STR_TERM_SIZE)
#define PTY_HEX_PAIR_CAP 3
#define PTY_FG_R 0xCF
#define PTY_FG_G 0xD2
#define PTY_FG_B 0xFF
#define PTY_GRID_DUMP_MIN 100
#define PTY_GRID_DUMP_ROWS_LOGGED 24
#define PTY_GRID_ROW_TEXT_CAP 81
#define PTY_UNHANDLED_CAP 64
#define PTY_UNHANDLED_TEXT_CAP 192
#define PTY_OSC52_MARK "\x1b]52;"
#define PTY_OSC52_MARK_LEN 5
#define PTY_UTF_CONT_MIN 0x80
#define PTY_UTF_CONT_MAX 0xBF
#define PTY_UTF_LEAD2_MIN 0xC2
#define PTY_UTF_LEAD2_MAX 0xDF
#define PTY_UTF_LEAD3_MIN 0xE0
#define PTY_UTF_LEAD3_MAX 0xEF
#define PTY_UTF_LEAD4_MIN 0xF0
#define PTY_UTF_LEAD4_MAX 0xF4
#define PTY_UTF_CONT_BITS 6
#define PTY_UTF_CONT_MASK 0x3F
#define PTY_UTF_LEAD2_MASK 0x1F
#define PTY_UTF_LEAD3_MASK 0x0F
#define PTY_UTF_LEAD4_MASK 0x07
#define PTY_UTF_SEQ2_LEN 2
#define PTY_UTF_SEQ3_LEN 3
#define PTY_UTF_SEQ4_LEN 4
#define PTY_FOLD_SUB_CH '?'
#define PTY_FOLD_CELLS_NONE 0
#define PTY_FOLD_CELLS_NARROW 1
#define PTY_FOLD_CELLS_WIDE 2

typedef struct pty_fold_range
{
    uint32_t lo;
    uint32_t hi;
    char ch;
    uint8_t cunt;
    uint8_t pudding[2];
} pty_fold_range_t;

static uint32_t pty_cp_fold(uint32_t cp, char *out)
{
    const pty_fold_range_t ranges[] = {
        { 0x0300, 0x036F, PTY_FOLD_SUB_CH, PTY_FOLD_CELLS_NONE, LDG_ARR_ZERO_INIT },
        { 0x200B, 0x200F, PTY_FOLD_SUB_CH, PTY_FOLD_CELLS_NONE, LDG_ARR_ZERO_INIT },
        { 0x2060, 0x2064, PTY_FOLD_SUB_CH, PTY_FOLD_CELLS_NONE, LDG_ARR_ZERO_INIT },
        { 0x20D0, 0x20FF, PTY_FOLD_SUB_CH, PTY_FOLD_CELLS_NONE, LDG_ARR_ZERO_INIT },
        { 0xFE00, 0xFE0F, PTY_FOLD_SUB_CH, PTY_FOLD_CELLS_NONE, LDG_ARR_ZERO_INIT },
        { 0x00A0, 0x00A0, ' ', PTY_FOLD_CELLS_NARROW, LDG_ARR_ZERO_INIT },
        { 0x2010, 0x2015, '-', PTY_FOLD_CELLS_NARROW, LDG_ARR_ZERO_INIT },
        { 0x2018, 0x201B, '\'', PTY_FOLD_CELLS_NARROW, LDG_ARR_ZERO_INIT },
        { 0x201C, 0x201F, '"', PTY_FOLD_CELLS_NARROW, LDG_ARR_ZERO_INIT },
        { 0x2022, 0x2026, '.', PTY_FOLD_CELLS_NARROW, LDG_ARR_ZERO_INIT },
        { 0x2190, 0x21FF, '*', PTY_FOLD_CELLS_NARROW, LDG_ARR_ZERO_INIT },
        { 0x2500, 0x2501, '-', PTY_FOLD_CELLS_NARROW, LDG_ARR_ZERO_INIT },
        { 0x2502, 0x2503, '|', PTY_FOLD_CELLS_NARROW, LDG_ARR_ZERO_INIT },
        { 0x2550, 0x2550, '-', PTY_FOLD_CELLS_NARROW, LDG_ARR_ZERO_INIT },
        { 0x2551, 0x2551, '|', PTY_FOLD_CELLS_NARROW, LDG_ARR_ZERO_INIT },
        { 0x2504, 0x257F, '+', PTY_FOLD_CELLS_NARROW, LDG_ARR_ZERO_INIT },
        { 0x2580, 0x259F, '#', PTY_FOLD_CELLS_NARROW, LDG_ARR_ZERO_INIT },
        { 0x25A0, 0x25FF, '*', PTY_FOLD_CELLS_NARROW, LDG_ARR_ZERO_INIT },
        { 0x1100, 0x115F, PTY_FOLD_SUB_CH, PTY_FOLD_CELLS_WIDE, LDG_ARR_ZERO_INIT },
        { 0x2E80, 0x303E, PTY_FOLD_SUB_CH, PTY_FOLD_CELLS_WIDE, LDG_ARR_ZERO_INIT },
        { 0x3041, 0x33FF, PTY_FOLD_SUB_CH, PTY_FOLD_CELLS_WIDE, LDG_ARR_ZERO_INIT },
        { 0x3400, 0x4DBF, PTY_FOLD_SUB_CH, PTY_FOLD_CELLS_WIDE, LDG_ARR_ZERO_INIT },
        { 0x4E00, 0x9FFF, PTY_FOLD_SUB_CH, PTY_FOLD_CELLS_WIDE, LDG_ARR_ZERO_INIT },
        { 0xA000, 0xA4CF, PTY_FOLD_SUB_CH, PTY_FOLD_CELLS_WIDE, LDG_ARR_ZERO_INIT },
        { 0xAC00, 0xD7A3, PTY_FOLD_SUB_CH, PTY_FOLD_CELLS_WIDE, LDG_ARR_ZERO_INIT },
        { 0xF900, 0xFAFF, PTY_FOLD_SUB_CH, PTY_FOLD_CELLS_WIDE, LDG_ARR_ZERO_INIT },
        { 0xFE30, 0xFE4F, PTY_FOLD_SUB_CH, PTY_FOLD_CELLS_WIDE, LDG_ARR_ZERO_INIT },
        { 0xFF00, 0xFF60, PTY_FOLD_SUB_CH, PTY_FOLD_CELLS_WIDE, LDG_ARR_ZERO_INIT },
        { 0xFFE0, 0xFFE6, PTY_FOLD_SUB_CH, PTY_FOLD_CELLS_WIDE, LDG_ARR_ZERO_INIT },
        { 0x20000, 0x3FFFD, PTY_FOLD_SUB_CH, PTY_FOLD_CELLS_WIDE, LDG_ARR_ZERO_INIT }
    };
    uint32_t idx = 0;

    for (idx = 0; idx < (uint32_t)(sizeof(ranges) / sizeof(*ranges)); idx = idx + 0x1)
    {
        if (cp < ranges[idx].lo || cp > ranges[idx].hi) { continue; }

        if (ranges[idx].cunt == (uint8_t)PTY_FOLD_CELLS_NONE) { return PTY_FOLD_CELLS_NONE; }

        out[0] = ranges[idx].ch;

        if (ranges[idx].cunt == (uint8_t)PTY_FOLD_CELLS_WIDE)
        {
            out[0x1] = ranges[idx].ch;
            return PTY_FOLD_CELLS_WIDE;
        }

        return PTY_FOLD_CELLS_NARROW;
    }

    out[0] = PTY_FOLD_SUB_CH;
    return PTY_FOLD_CELLS_NARROW;
}

static uint32_t pty_utf_carry_decode(crosstalk_eda_io_pty_ctx_t *ctx)
{
    uint32_t cp = 0;
    uint32_t idx = 0;

    if (ctx->utf_need == (uint32_t)PTY_UTF_SEQ2_LEN) { cp = (uint32_t)(ctx->utf_carry[0] & (uint8_t)PTY_UTF_LEAD2_MASK); }
    else if (ctx->utf_need == (uint32_t)PTY_UTF_SEQ3_LEN) { cp = (uint32_t)(ctx->utf_carry[0] & (uint8_t)PTY_UTF_LEAD3_MASK); }
    else { cp = (uint32_t)(ctx->utf_carry[0] & (uint8_t)PTY_UTF_LEAD4_MASK); }

    for (idx = 0x1; idx < ctx->utf_carry_len; idx = idx + 0x1) { cp = (cp << PTY_UTF_CONT_BITS) | (uint32_t)(ctx->utf_carry[idx] & (uint8_t)PTY_UTF_CONT_MASK); }

    return cp;
}

static void pty_utf_fold(crosstalk_eda_io_pty_ctx_t *ctx, const uint8_t *in, uint64_t in_len, uint8_t *out, uint64_t *out_len)
{
    char piece[PTY_FOLD_CELLS_WIDE] = LDG_ARR_ZERO_INIT;
    uint64_t at = 0;
    uint64_t wr = 0;
    uint32_t piece_cunt = 0;
    uint32_t piece_at = 0;

    while (at < in_len)
    {
        uint8_t by = in[at];

        if (ctx->utf_need != 0)
        {
            if (by >= (uint8_t)PTY_UTF_CONT_MIN && by <= (uint8_t)PTY_UTF_CONT_MAX)
            {
                ctx->utf_carry[ctx->utf_carry_len] = by;
                ctx->utf_carry_len = ctx->utf_carry_len + 0x1;
                at = at + 0x1;

                if (ctx->utf_carry_len == ctx->utf_need)
                {
                    piece_cunt = pty_cp_fold(pty_utf_carry_decode(ctx), piece);
                    for (piece_at = 0; piece_at < piece_cunt; piece_at = piece_at + 0x1) { out[wr] = (uint8_t)piece[piece_at]; wr = wr + 0x1; }

                    ctx->utf_need = 0;
                    ctx->utf_carry_len = 0;
                }

                continue;
            }

            out[wr] = (uint8_t)PTY_FOLD_SUB_CH;
            wr = wr + 0x1;
            ctx->utf_need = 0;
            ctx->utf_carry_len = 0;
            continue;
        }

        if (by <= (uint8_t)LDG_ASCII_MAX)
        {
            out[wr] = by;
            wr = wr + 0x1;
            at = at + 0x1;
            continue;
        }

        if (by >= (uint8_t)PTY_UTF_LEAD2_MIN && by <= (uint8_t)PTY_UTF_LEAD2_MAX) { ctx->utf_need = (uint32_t)PTY_UTF_SEQ2_LEN; }
        else if (by >= (uint8_t)PTY_UTF_LEAD3_MIN && by <= (uint8_t)PTY_UTF_LEAD3_MAX) { ctx->utf_need = (uint32_t)PTY_UTF_SEQ3_LEN; }
        else if (by >= (uint8_t)PTY_UTF_LEAD4_MIN && by <= (uint8_t)PTY_UTF_LEAD4_MAX) { ctx->utf_need = (uint32_t)PTY_UTF_SEQ4_LEN; }
        else
        {
            out[wr] = (uint8_t)PTY_FOLD_SUB_CH;
            wr = wr + 0x1;
            at = at + 0x1;
            continue;
        }

        ctx->utf_carry[0] = by;
        ctx->utf_carry_len = 0x1;
        at = at + 0x1;
    }

    *out_len = wr;
}

static void pty_grid_dump(crosstalk_eda_io_pty_ctx_t *ctx)
{
    ldg_term_cell_t cell[CROSSTALK_EDA_IO_PTY_COL_MAX];
    char text[PTY_GRID_ROW_TEXT_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t cursor_col = 0;
    uint32_t cursor_row = 0;
    uint32_t row = 0;
    uint32_t col = 0;
    uint32_t nonspace_total = 0;
    uint32_t rows_logged = 0;

    if (ldg_term_cursor_get(ctx->term, &cursor_col, &cursor_row) != LDG_ERR_AOK) { cursor_col = UINT32_MAX; cursor_row = UINT32_MAX; }

    for (row = 0; row < ctx->row_cunt; row = row + 0x1)
    {
        uint32_t nonspace = 0;
        uint32_t text_len = 0;
        uint32_t first_at = UINT32_MAX;

        if (ldg_term_row_get(ctx->term, row, cell, (uint32_t)CROSSTALK_EDA_IO_PTY_COL_MAX) != LDG_ERR_AOK) { break; }

        for (col = 0; col < ctx->col_cunt; col = col + 0x1)
        {
            if (cell[col].ch > (uint8_t)LDG_ASCII_PRINTABLE_MIN && cell[col].ch <= (uint8_t)LDG_ASCII_PRINTABLE_MAX)
            {
                nonspace = nonspace + 0x1;
                if (first_at == UINT32_MAX) { first_at = col; }
            }

            if (cell[col].ch >= (uint8_t)LDG_ASCII_PRINTABLE_MIN && cell[col].ch <= (uint8_t)LDG_ASCII_PRINTABLE_MAX && text_len < (uint32_t)PTY_GRID_ROW_TEXT_CAP - LDG_STR_TERM_SIZE) { text[text_len] = (char)cell[col].ch; text_len = text_len + 0x1; }
        }

        text[text_len] = LDG_STR_TERM;
        nonspace_total = nonspace_total + nonspace;

        if (nonspace > 0 && rows_logged < (uint32_t)PTY_GRID_DUMP_ROWS_LOGGED)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "pty: grid row; row: %u32; nonspace: %u32; fg: %u32 %u32 %u32; text: %s", row, nonspace, (uint32_t)cell[first_at].fg[0], (uint32_t)cell[first_at].fg[1], (uint32_t)cell[first_at].fg[2], text);
            rows_logged = rows_logged + 0x1;
        }
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "pty: grid dump; cursor col: %u32; cursor row: %u32; nonspace total: %u32; rows: %u32; cols: %u32", cursor_col, cursor_row, nonspace_total, ctx->row_cunt, ctx->col_cunt);

    {
        ldg_byte_t final[PTY_UNHANDLED_CAP] = LDG_ARR_ZERO_INIT;
        char final_text[PTY_UNHANDLED_TEXT_CAP] = LDG_ARR_ZERO_INIT;
        uint64_t final_cunt = 0;
        uint64_t final_at = 0;
        uint32_t text_at = 0;

        if (ldg_term_vt_unhandled_get(&ctx->vt, final, (uint64_t)sizeof(final), &final_cunt) == LDG_ERR_AOK)
        {
            for (final_at = 0; final_at < final_cunt && text_at + LDG_STR_TERM_SIZE < (uint32_t)sizeof(final_text); final_at = final_at + 0x1)
            {
                final_text[text_at] = (char)final[final_at];
                text_at = text_at + 0x1;
                final_text[text_at] = ' ';
                text_at = text_at + 0x1;
            }

            final_text[text_at] = LDG_STR_TERM;
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "pty: vt unhandled finals; cunt: %u64; finals: %s", final_cunt, final_text);
        }
    }
}

static uint32_t pty_reply_pump(crosstalk_eda_io_pty_ctx_t *ctx)
{
    ldg_byte_t reply[PTY_REPLY_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t reply_len = 0;
    uint64_t sent = 0;
    uint32_t ret = 0;

    ret = ldg_term_vt_reply_drain(&ctx->vt, reply, (uint64_t)sizeof(reply), &reply_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (reply_len > 0) { return ldg_io_pty_wr(&ctx->pty, reply, reply_len, &sent); }

    return LDG_ERR_AOK;
}

static uint32_t pty_clip_pump(crosstalk_eda_io_pty_ctx_t *ctx)
{
    ldg_byte_t incoming[CROSSTALK_EDA_IO_PTY_CLIP_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t incoming_len = 0;
    uint32_t ret = 0;

    ret = ldg_term_vt_clipboard_get(&ctx->vt, incoming, (uint64_t)sizeof(incoming), &incoming_len);
    if (ret == LDG_ERR_NOT_FOUND || ret == LDG_ERR_EMPTY) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (incoming_len > 0)
    {
        if (incoming_len == (uint64_t)ctx->clip_len && memcmp(incoming, ctx->clip, ctx->clip_len) == 0x0) { return LDG_ERR_AOK; }

        if (incoming_len >= (uint64_t)sizeof(ctx->clip)) { incoming_len = (uint64_t)sizeof(ctx->clip) - LDG_STR_TERM_SIZE; }

        memcpy(ctx->clip, incoming, incoming_len);
        ctx->clip[incoming_len] = LDG_STR_TERM;
        ctx->clip_len = (uint32_t)incoming_len;
        ctx->clip_new = LDG_TRUTH_TRUE;

        ret = ldg_sys_clipboard_set(incoming, incoming_len);
        if (ret != LDG_ERR_AOK && ret != LDG_ERR_UNSUPPORTED) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty: clipboard set failed; err: %u32; len: %u64", ret, incoming_len); }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "pty: clipboard from editor; len: %u64", incoming_len);
        return LDG_ERR_AOK;
    }

    {
        ldg_byte_t text[PTY_PASTE_CAP] = LDG_ARR_ZERO_INIT;
        ldg_byte_t enc[PTY_PASTE_ENC_CAP] = LDG_ARR_ZERO_INIT;
        uint64_t text_len = 0;
        uint64_t enc_len = 0;
        uint64_t sent = 0;

        ret = ldg_sys_clipboard_get(text, (uint64_t)sizeof(text), &text_len);
        if (ret == LDG_ERR_UNSUPPORTED || ret == LDG_ERR_EMPTY) { return LDG_ERR_AOK; }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_term_clipboard_encode(text, text_len, enc, (uint64_t)sizeof(enc), &enc_len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (enc_len > 0) { ret = ldg_io_pty_wr(&ctx->pty, enc, enc_len, &sent); }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "pty: clipboard to editor; len: %u64; err: %u32", text_len, ret);
        return ret;
    }
}

uint32_t crosstalk_eda_io_pty_init(crosstalk_eda_io_pty_ctx_t *ctx, ldg_mem_pool_t *pool)
{
    if (LDG_UNLIKELY(ctx == 0x0 || pool == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->pool = pool;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_pty_shutdown(crosstalk_eda_io_pty_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live == LDG_TRUTH_TRUE) { return crosstalk_eda_io_pty_close(ctx); }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_pty_open(crosstalk_eda_io_pty_ctx_t *ctx, const char *shell, const char *cwd)
{
    ldg_io_pty_conf_t pty_conf = LDG_STRUCT_ZERO_INIT;
    ldg_term_conf_t term_conf = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->pool == 0x0) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty: open refused; stage: pool; err: %u32", (uint32_t)LDG_ERR_NOT_INIT); return LDG_ERR_NOT_INIT; }

    if (ctx->live == LDG_TRUTH_TRUE) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty: open refused; stage: live; err: %u32", (uint32_t)LDG_ERR_BUSY); return LDG_ERR_BUSY; }

    ret = ldg_mem_pool_rst(ctx->pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty: open failed; stage: pool rst; err: %u32", ret); return ret; }

    ret = ldg_term_conf_default_get(&term_conf);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty: open failed; stage: term conf; err: %u32", ret); return ret; }

    term_conf.col_cunt = CROSSTALK_EDA_IO_PTY_COL_MAX;
    term_conf.row_cunt = CROSSTALK_EDA_IO_PTY_ROW_MAX;
    term_conf.fg[0] = PTY_FG_R;
    term_conf.fg[1] = PTY_FG_G;
    term_conf.fg[2] = PTY_FG_B;

    ret = ldg_term_create(ctx->pool, &term_conf, &ctx->term);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty: open failed; stage: term create; err: %u32; cols: %u32; rows: %u32; pool left: %u64", ret, (uint32_t)term_conf.col_cunt, (uint32_t)term_conf.row_cunt, ldg_mem_pool_remaining_get(ctx->pool)); return ret; }

    ret = ldg_term_vt_init(&ctx->vt, ctx->term);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty: open failed; stage: vt init; err: %u32", ret); return ret; }

    ret = ldg_io_pty_conf_default_get(&pty_conf);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty: open failed; stage: pty conf; err: %u32", ret); return ret; }

    if (shell != 0x0 && shell[0] != LDG_STR_TERM) { pty_conf.shell = shell; }

    if (cwd != 0x0 && cwd[0] != LDG_STR_TERM) { pty_conf.cwd = cwd; }

    pty_conf.term_name = PTY_TERM_NAME;
    pty_conf.col_cunt = term_conf.col_cunt;
    pty_conf.row_cunt = term_conf.row_cunt;

    ret = ldg_io_pty_open(&pty_conf, &ctx->pty);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty: open failed; stage: pty open; err: %u32; shell: %s", ret, pty_conf.shell != 0x0 ? pty_conf.shell : "default");
        return ret;
    }

    if (ldg_str_rbrcpy(ctx->title, PTY_TITLE_DEFAULT, (uint64_t)sizeof(ctx->title)) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty title copy failed"); }

    ctx->col_cunt = term_conf.col_cunt;
    ctx->row_cunt = term_conf.row_cunt;
    ctx->clip_len = 0;
    ctx->clip_new = LDG_TRUTH_FALSE;
    ctx->rd_total = 0;
    ctx->quiet_ticks = 0;
    ctx->utf_carry_len = 0;
    ctx->utf_need = 0;
    ctx->grid_dumped = LDG_TRUTH_FALSE;
    ctx->refused_cols = 0;
    ctx->refused_rows = 0;
    ctx->exit_code = 0;
    ctx->exited = LDG_TRUTH_FALSE;
    ctx->live = LDG_TRUTH_TRUE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "pty: open; pid: %u32; cols: %u32; rows: %u32; term: %s; passthrough: %u32", ctx->pty.pid, pty_conf.col_cunt, pty_conf.row_cunt, PTY_TERM_NAME, (uint32_t)ctx->pty.passthrough);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_pty_close(crosstalk_eda_io_pty_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = ldg_io_pty_close(&ctx->pty);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty close failed"); }

    if (ctx->term != 0x0)
    {
        ret = ldg_term_destroy(&ctx->term);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty term destroy failed"); }
    }

    ctx->live = LDG_TRUTH_FALSE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "pty: closed; exit: %u32", ctx->exit_code);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_pty_tick(crosstalk_eda_io_pty_ctx_t *ctx)
{
    ldg_byte_t buff[CROSSTALK_EDA_IO_PTY_RD_CAP];
    uint64_t got = 0;
    uint32_t round = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live != LDG_TRUTH_TRUE || ctx->exited == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    for (round = 0; round < (uint32_t)PTY_RD_ROUND_MAX; round++)
    {
        got = 0;
        ret = ldg_io_pty_rd(&ctx->pty, buff, (uint64_t)sizeof(buff), &got);

        if (ret == LDG_ERR_WOULD_BLOCK)
        {
            ctx->quiet_ticks = ctx->quiet_ticks + 0x1;
            if (ctx->quiet_ticks == (uint32_t)PTY_QUIET_TICK_REPORT) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty: output stalled; total: %u64; pid: %u32", ctx->rd_total, ctx->pty.pid); }

            return LDG_ERR_AOK;
        }

        if (ret == LDG_ERR_EOF)
        {
            if (ldg_io_pty_exit_code_get(&ctx->pty, &ctx->exit_code) != LDG_ERR_AOK) { ctx->exit_code = 0; }

            ctx->exited = LDG_TRUTH_TRUE;

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "pty: shell exited; code: %u32", ctx->exit_code);

            return LDG_ERR_AOK;
        }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (got == 0) { return LDG_ERR_AOK; }

        {
            uint64_t scan_at = 0;

            for (scan_at = 0; scan_at + PTY_OSC52_MARK_LEN <= got; scan_at = scan_at + 0x1) { if (memcmp(buff + scan_at, PTY_OSC52_MARK, (uint64_t)PTY_OSC52_MARK_LEN) == 0)
                {
                    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "pty: osc52 in stream; at: %u64; got: %u64", scan_at, got);
                    break;
                }
            }
        }

        if (ctx->rd_total == 0)
        {
            char dump[PTY_DUMP_CAP] = LDG_ARR_ZERO_INIT;
            char pair[PTY_HEX_PAIR_CAP] = LDG_ARR_ZERO_INIT;
            uint64_t dump_at = 0;
            uint64_t dump_len = got;

            if (dump_len > (uint64_t)PTY_DUMP_BYTES) { dump_len = (uint64_t)PTY_DUMP_BYTES; }

            for (dump_at = 0; dump_at < dump_len; dump_at = dump_at + 0x1)
            {
                if (ldg_byte_to_hex(buff[dump_at], pair) != LDG_ERR_AOK) { break; }

                dump[dump_at * (uint64_t)PTY_DUMP_STRIDE] = pair[0];
                dump[dump_at * (uint64_t)PTY_DUMP_STRIDE + 0x1] = pair[0x1];
                dump[dump_at * (uint64_t)PTY_DUMP_STRIDE + 0x2] = ' ';
            }

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "pty: first output; bytes: %u64; hex: %s", got, dump);
        }

        ctx->rd_total = ctx->rd_total + got;
        ctx->quiet_ticks = 0;

        {
            uint8_t fold[CROSSTALK_EDA_IO_PTY_RD_CAP + CROSSTALK_EDA_IO_PTY_UTF_SEQ_CAP];
            uint64_t fold_len = 0;

            pty_utf_fold(ctx, buff, got, fold, &fold_len);

            if (fold_len != 0)
            {
                ret = ldg_term_vt_feed(&ctx->vt, fold, fold_len);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
        }

        if (ctx->grid_dumped == LDG_TRUTH_FALSE && ctx->rd_total >= (uint64_t)PTY_GRID_DUMP_MIN)
        {
            pty_grid_dump(ctx);
            ctx->grid_dumped = LDG_TRUTH_TRUE;
        }

        ret = pty_reply_pump(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty: reply pump failed; err: %u32", ret); }

        ret = pty_clip_pump(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty: clip pump failed; err: %u32", ret); }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_pty_grid_dump(crosstalk_eda_io_pty_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live != LDG_TRUTH_TRUE || ctx->term == 0x0) { return LDG_ERR_NOT_INIT; }

    pty_grid_dump(ctx);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_pty_keys_wr(crosstalk_eda_io_pty_ctx_t *ctx, const uint8_t *bytes, uint32_t len)
{
    uint64_t sent = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || bytes == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live != LDG_TRUTH_TRUE || ctx->exited == LDG_TRUTH_TRUE) { return LDG_ERR_CLOSED; }

    if (len == 0) { return LDG_ERR_AOK; }

    return ldg_io_pty_wr(&ctx->pty, bytes, (uint64_t)len, &sent);
}

uint32_t crosstalk_eda_io_pty_grid_resize(crosstalk_eda_io_pty_ctx_t *ctx, uint32_t col_cunt, uint32_t row_cunt)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live != LDG_TRUTH_TRUE || ctx->exited == LDG_TRUTH_TRUE) { return LDG_ERR_CLOSED; }

    if (col_cunt == 0 || row_cunt == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (col_cunt > (uint32_t)CROSSTALK_EDA_IO_PTY_COL_MAX) { col_cunt = CROSSTALK_EDA_IO_PTY_COL_MAX; }

    if (row_cunt > (uint32_t)CROSSTALK_EDA_IO_PTY_ROW_MAX) { row_cunt = CROSSTALK_EDA_IO_PTY_ROW_MAX; }

    if (col_cunt == ctx->col_cunt && row_cunt == ctx->row_cunt) { return LDG_ERR_AOK; }

    if (col_cunt == ctx->refused_cols && row_cunt == ctx->refused_rows) { return LDG_ERR_AOK; }

    ret = ldg_term_resize(ctx->term, col_cunt, row_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ctx->refused_cols = col_cunt;
        ctx->refused_rows = row_cunt;
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty: term resize refused; cols: %u32; rows: %u32; from cols: %u32; from rows: %u32; err: %u32", col_cunt, row_cunt, ctx->col_cunt, ctx->row_cunt, ret);
        return ret;
    }

    ctx->refused_cols = 0;
    ctx->refused_rows = 0;

    ret = ldg_io_pty_resize(&ctx->pty, col_cunt, row_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty: child resize failed; err: %u32", ret); }

    ctx->col_cunt = col_cunt;
    ctx->row_cunt = row_cunt;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "pty: resized; cols: %u32; rows: %u32", col_cunt, row_cunt);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_pty_focus_put(crosstalk_eda_io_pty_ctx_t *ctx, uint8_t focused)
{
    ldg_byte_t enc[PTY_REPLY_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t enc_len = 0;
    uint64_t sent = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live != LDG_TRUTH_TRUE || ctx->exited == LDG_TRUTH_TRUE) { return LDG_ERR_CLOSED; }

    ret = ldg_term_focus_encode(&ctx->vt, focused, enc, (uint64_t)sizeof(enc), &enc_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (enc_len > 0) { return ldg_io_pty_wr(&ctx->pty, enc, enc_len, &sent); }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_pty_clip_take(crosstalk_eda_io_pty_ctx_t *ctx, char *out, uint32_t cap, uint32_t *out_len)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;
    if (cap == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;
    if (ctx->clip_new != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_FOUND; }

    ctx->clip_new = LDG_TRUTH_FALSE;

    ret = ldg_str_rbrcpy(out, ctx->clip, (uint64_t)cap);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    *out_len = (uint32_t)strlen(out);
    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_io_pty_live_is(const crosstalk_eda_io_pty_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->live == LDG_TRUTH_TRUE && ctx->exited != LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}
