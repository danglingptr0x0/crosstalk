#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "hdl/diag/sv.h"

static uint8_t diag_sv_tok_match_is(const char *src, uint32_t src_len, const crosstalk_hdl_tok_t *tok, const char *text)
{
    uint64_t text_len = 0;

    if (LDG_UNLIKELY(src == 0x0 || tok == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (text == 0x0) { return LDG_TRUTH_TRUE; }

    text_len = (uint64_t)strlen(text);

    if ((uint64_t)tok->len != text_len) { return LDG_TRUTH_FALSE; }

    if ((uint64_t)tok->off + text_len > (uint64_t)src_len) { return LDG_TRUTH_FALSE; }

    if (memcmp(&src[tok->off], text, text_len) != 0) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

uint32_t crosstalk_hdl_diag_sv_entry_find(const crosstalk_hdl_parse_ctx_t *parse, uint32_t err, crosstalk_hdl_diag_t *out)
{
    static const crosstalk_hdl_diag_entry_t entry[] = {
        { 0x0, 0x0, 0x0, 0x0, (uint32_t)LDG_ERR_AOK, LDG_ARR_ZERO_INIT }
    };
    const char *src = 0x0;
    uint32_t src_len = 0;
    uint32_t pos = 0;
    uint32_t end = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(parse == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (crosstalk_hdl_diag_t)LDG_STRUCT_ZERO_INIT;

    src = parse->lex.src;
    src_len = parse->lex.len;

    if (src == 0x0) { return LDG_ERR_NOT_INIT; }

    out->src = src;
    out->err = err;

    if (err >= (uint32_t)CROSSTALK_HDL_ERR_LEX_CHAR && err <= (uint32_t)CROSSTALK_HDL_ERR_LEX_ESC_UNTERM)
    {
        out->line = parse->lex.line;
        out->col = parse->lex.col;
        pos = parse->lex.pos;
    }
    else
    {
        out->line = parse->cur.line;
        out->col = parse->cur.col;
        pos = parse->cur.off;
    }

    if (pos > src_len) { pos = src_len; }

    out->line_at = pos;

    while (out->line_at > 0 && src[out->line_at - (uint32_t)LDG_STR_TERM_SIZE] != LDG_ASCII_LF) { out->line_at = out->line_at - 0x1; }

    end = pos;

    while (end < src_len && src[end] != LDG_ASCII_LF) { end = end + 0x1; }

    while (end > out->line_at && src[end - (uint32_t)LDG_STR_TERM_SIZE] == LDG_ASCII_CR) { end = end - 0x1; }

    out->line_len = end - out->line_at;

    for (idx = 0; idx < (uint32_t)(sizeof(entry) / sizeof(entry[0])) && entry[idx].msg != 0x0; idx = idx + 0x1)
    {
        if (entry[idx].err != (uint32_t)CROSSTALK_HDL_DIAG_ERR_ANY && entry[idx].err != err) { continue; }

        if (diag_sv_tok_match_is(src, src_len, &parse->prev, entry[idx].prev_text) != LDG_TRUTH_TRUE) { continue; }

        if (diag_sv_tok_match_is(src, src_len, &parse->cur, entry[idx].cur_text) != LDG_TRUTH_TRUE) { continue; }

        out->entry = &entry[idx];
        break;
    }

    return LDG_ERR_AOK;
}
