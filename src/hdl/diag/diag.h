#ifndef CROSSTALK_HDL_DIAG_H
#define CROSSTALK_HDL_DIAG_H

#include <stdint.h>

#define CROSSTALK_HDL_DIAG_ERR_ANY UINT32_MAX
#define CROSSTALK_HDL_DIAG_TEXT_CAP 256
#define CROSSTALK_HDL_DIAG_LINE_MAX 120
#define CROSSTALK_HDL_DIAG_ENTRY_FMT " <-- %.127s (%.63s)"
#define CROSSTALK_HDL_DIAG_ERR_FMT " <-- err: %" PRIu32

typedef struct crosstalk_hdl_diag_entry
{
    const char *prev_text;
    const char *cur_text;
    const char *msg;
    const char *cite;
    uint32_t err;
    uint8_t pudding[4];
} crosstalk_hdl_diag_entry_t;

typedef struct crosstalk_hdl_diag
{
    const crosstalk_hdl_diag_entry_t *entry;
    const char *src;
    uint32_t err;
    uint32_t line;
    uint32_t col;
    uint32_t line_at;
    uint32_t line_len;
    uint8_t pudding[4];
} crosstalk_hdl_diag_t;

uint32_t crosstalk_hdl_diag_text_fmt(const crosstalk_hdl_diag_t *diag, char *out, uint32_t cap);

#endif
