#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "hdl/diag/diag.h"

uint32_t crosstalk_hdl_diag_text_fmt(const crosstalk_hdl_diag_t *diag, char *out, uint32_t cap)
{
    uint32_t at = 0;
    uint32_t end = 0;
    uint32_t take = 0;

    if (LDG_UNLIKELY(diag == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (cap == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;

    if (diag->src == 0x0) { return LDG_ERR_NOT_INIT; }

    at = diag->line_at;
    end = diag->line_at + diag->line_len;

    while (at < end && (diag->src[at] == LDG_ASCII_SPACE || diag->src[at] == LDG_ASCII_TAB)) { at = at + 0x1; }

    take = end - at;

    if (take > (uint32_t)CROSSTALK_HDL_DIAG_LINE_MAX) { take = (uint32_t)CROSSTALK_HDL_DIAG_LINE_MAX; }

    if (take >= cap) { take = cap - (uint32_t)LDG_STR_TERM_SIZE; }

    memcpy(out, &diag->src[at], (uint64_t)take);
    out[take] = LDG_STR_TERM;

    if (diag->entry != 0x0)
    {
        if (snprintf(&out[take], (uint64_t)(cap - take), CROSSTALK_HDL_DIAG_ENTRY_FMT, diag->entry->msg, diag->entry->cite) < 0) { return LDG_ERR_STR_FMT; }

        return LDG_ERR_AOK;
    }

    if (snprintf(&out[take], (uint64_t)(cap - take), CROSSTALK_HDL_DIAG_ERR_FMT, diag->err) < 0) { return LDG_ERR_STR_FMT; }

    return LDG_ERR_AOK;
}
