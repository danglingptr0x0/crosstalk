#ifndef CROSSTALK_EDA_UI_TERM_H
#define CROSSTALK_EDA_UI_TERM_H

#include <inttypes.h>
#include <stdint.h>

#include <dangling/mem/alloc.h>
#include <dangling/term/term.h>
#include <dangling/term/vt.h>

#include "eda/io/pty.h"
#include "ui/window.h"
#include "ui/theme.h"

#define CROSSTALK_EDA_UI_TERM_ERR_SESH 946
#define CROSSTALK_EDA_UI_TERM_ERR_SRC 947

#define CROSSTALK_EDA_UI_TERM_SESH_MAX 8
#define CROSSTALK_EDA_UI_TERM_RAW_CAP 8192
#define CROSSTALK_EDA_UI_TERM_TITLE_CAP 64
#define CROSSTALK_EDA_UI_TERM_STATUS_CAP 128
#define CROSSTALK_EDA_UI_TERM_BYTE_ROW_MAX 96
#define CROSSTALK_EDA_UI_TERM_BYTE_ROW_MIN 1
#define CROSSTALK_EDA_UI_TERM_BYTE_ROW_QUANT 4
#define CROSSTALK_EDA_UI_TERM_ASCII_PER_BYTE 1
#define CROSSTALK_EDA_UI_TERM_HEX_PER_BYTE 3
#define CROSSTALK_EDA_UI_TERM_BIN_PER_BYTE 9
#define CROSSTALK_EDA_UI_TERM_PANE_CUNT 3
#define CROSSTALK_EDA_UI_TERM_OFF_FMT "%08" PRIX64
#define CROSSTALK_EDA_UI_TERM_SEP_COLS 2
#define CROSSTALK_EDA_UI_TERM_DOT '.'
#define CROSSTALK_EDA_UI_TERM_LOW_DASH '_'
#define CROSSTALK_EDA_UI_TERM_NIBBLE_HI 4
#define CROSSTALK_EDA_UI_TERM_OFF_CHARS 8
#define CROSSTALK_EDA_UI_TERM_WS_MIN 0x09
#define CROSSTALK_EDA_UI_TERM_WS_MAX 0x0D
#define CROSSTALK_EDA_UI_TERM_CELL_CAP 16
#define CROSSTALK_EDA_UI_TERM_KEY_CAP 64
#define CROSSTALK_EDA_UI_TERM_BS 0x08
#define CROSSTALK_EDA_UI_TERM_CSI 0x5B
#define CROSSTALK_EDA_UI_TERM_ARROW_UP 0x41
#define CROSSTALK_EDA_UI_TERM_ARROW_DOWN 0x42
#define CROSSTALK_EDA_UI_TERM_ARROW_RHS 0x43
#define CROSSTALK_EDA_UI_TERM_ARROW_LHS 0x44
#define CROSSTALK_EDA_UI_TERM_ARROW_BYTES 3
#define CROSSTALK_EDA_UI_TERM_IMG_RGBA_BYTES 4
#define CROSSTALK_EDA_UI_TERM_IMG_RGB_BYTES 3
#define CROSSTALK_EDA_UI_TERM_COOKED_COLS 120
#define CROSSTALK_EDA_UI_TERM_COOKED_ROWS 48
#define CROSSTALK_EDA_UI_TERM_SGR_IDX_NONE 0xFF
#define CROSSTALK_EDA_UI_TERM_SGR_IDX_CTRL 0xFE
#define CROSSTALK_EDA_UI_TERM_SGR_PARAM_MAX 8
#define CROSSTALK_EDA_UI_TERM_SGR_FINAL 'm'
#define CROSSTALK_EDA_UI_TERM_SGR_SEP ';'
#define CROSSTALK_EDA_UI_TERM_SGR_RST 0
#define CROSSTALK_EDA_UI_TERM_SGR_BOLD 1
#define CROSSTALK_EDA_UI_TERM_SGR_FG_LO 30
#define CROSSTALK_EDA_UI_TERM_SGR_FG_HI 37
#define CROSSTALK_EDA_UI_TERM_SGR_FG_DEFAULT 39
#define CROSSTALK_EDA_UI_TERM_SGR_BRIGHT_LO 90
#define CROSSTALK_EDA_UI_TERM_SGR_BRIGHT_HI 97
#define CROSSTALK_EDA_UI_TERM_SGR_BRIGHT_STEP 8
#define CROSSTALK_EDA_UI_TERM_ESC_FINAL_LO 0x40
#define CROSSTALK_EDA_UI_TERM_ESC_FINAL_HI 0x7E
#define CROSSTALK_EDA_UI_TERM_OFF_TEXT_RUN 5
#define CROSSTALK_EDA_UI_TERM_RGB_CUNT 3
#define CROSSTALK_EDA_UI_TERM_COL_ALPHA 0xFFU

#define CROSSTALK_EDA_UI_TERM_WINDOW_ID 0x5445524DULL
#define CROSSTALK_EDA_UI_TERM_WINDOW_TITLE "Terminal"
#define CROSSTALK_EDA_UI_TERM_KEY_OUT_CAP 512

typedef enum crosstalk_eda_ui_term_src
{
    CROSSTALK_EDA_UI_TERM_SRC_NONE = 0,
    CROSSTALK_EDA_UI_TERM_SRC_STREAM,
    CROSSTALK_EDA_UI_TERM_SRC_SERIAL,
    CROSSTALK_EDA_UI_TERM_SRC_PTY
} crosstalk_eda_ui_term_src_t;

typedef struct crosstalk_eda_ui_term_sesh
{
    uint8_t raw[CROSSTALK_EDA_UI_TERM_RAW_CAP];
    uint8_t raw_col[CROSSTALK_EDA_UI_TERM_RAW_CAP];
    char title[CROSSTALK_EDA_UI_TERM_TITLE_CAP];
    ldg_term_t *term;
    ldg_term_vt_t vt;
    void *owner;
    uint64_t bytes_total;
    uint32_t raw_hd;
    uint32_t raw_cunt;
    uint32_t src;
    uint32_t src_handle;
    uint32_t self_idx;
    uint32_t sgr_param[CROSSTALK_EDA_UI_TERM_SGR_PARAM_MAX];
    uint32_t sgr_param_cunt;
    uint32_t sgr_state;
    uint8_t sgr_idx;
    uint8_t sgr_bold;
    uint8_t live;
    uint8_t cooked;
    uint8_t pudding[4];
} crosstalk_eda_ui_term_sesh_t;

typedef struct crosstalk_eda_ui_term_ctx
{
    crosstalk_eda_ui_term_sesh_t sesh[CROSSTALK_EDA_UI_TERM_SESH_MAX];
    crosstalk_ui_window_state_t win;
    crosstalk_ui_window_ctx_t *win_nav;
    ldg_mem_pool_t *pool;
    const crosstalk_eda_io_pty_ctx_t *pty;
    uint8_t key_out[CROSSTALK_EDA_UI_TERM_KEY_OUT_CAP];
    uint32_t key_out_cunt;
    uint32_t sesh_active;
    uint32_t pty_cols_want;
    uint32_t pty_rows_want;
    uint32_t slot;
    char title_seen[CROSSTALK_EDA_UI_TERM_TITLE_CAP];
    uint8_t open;
    uint8_t ok;
    uint8_t focus;
    uint8_t pty_draw_logged;
    uint8_t pudding[4];
} crosstalk_eda_ui_term_ctx_t;

uint32_t crosstalk_eda_ui_term_init(crosstalk_eda_ui_term_ctx_t *ctx);
uint32_t crosstalk_eda_ui_term_shutdown(crosstalk_eda_ui_term_ctx_t *ctx);
uint32_t crosstalk_eda_ui_term_state_serialize(const crosstalk_eda_ui_term_ctx_t *ctx, uint8_t *out, uint32_t cap, uint32_t *out_len);
uint32_t crosstalk_eda_ui_term_state_deserialize(const uint8_t *buff, uint32_t len, uint8_t *out_host_live);
uint32_t crosstalk_eda_ui_term_open_set(crosstalk_eda_ui_term_ctx_t *ctx, uint8_t open);
uint8_t crosstalk_eda_ui_term_open_is(const crosstalk_eda_ui_term_ctx_t *ctx);
uint32_t crosstalk_eda_ui_term_sesh_bind(crosstalk_eda_ui_term_ctx_t *ctx, uint32_t sesh_idx, crosstalk_eda_ui_term_src_t src, uint32_t src_handle, const char *title);
uint32_t crosstalk_eda_ui_term_sesh_clear(crosstalk_eda_ui_term_ctx_t *ctx, uint32_t sesh_idx);
uint32_t crosstalk_eda_ui_term_sesh_next_get(const crosstalk_eda_ui_term_ctx_t *ctx, uint32_t *out);
uint32_t crosstalk_eda_ui_term_sesh_src_find(const crosstalk_eda_ui_term_ctx_t *ctx, crosstalk_eda_ui_term_src_t src, uint32_t *out);
uint32_t crosstalk_eda_ui_term_sesh_close(crosstalk_eda_ui_term_ctx_t *ctx, uint32_t sesh_idx);
uint32_t crosstalk_eda_ui_term_active_set(crosstalk_eda_ui_term_ctx_t *ctx, uint32_t sesh_idx);
uint32_t crosstalk_eda_ui_term_sesh_ptr_get(crosstalk_eda_ui_term_ctx_t *ctx, uint32_t sesh_idx, crosstalk_eda_ui_term_sesh_t **out);
uint32_t crosstalk_eda_ui_term_sesh_sink_cb(void *user, const uint8_t *bytes, uint32_t len);
uint32_t crosstalk_eda_ui_term_bytes_put(crosstalk_eda_ui_term_ctx_t *ctx, uint32_t sesh_idx, const uint8_t *bytes, uint32_t len);
uint32_t crosstalk_eda_ui_term_sink_cb(void *user, const uint8_t *bytes, uint32_t len);
uint32_t crosstalk_eda_ui_term_keys_drain(crosstalk_eda_ui_term_ctx_t *ctx, uint8_t *out, uint32_t cap, uint32_t *out_cunt);
uint32_t crosstalk_eda_ui_term_focus_set(crosstalk_eda_ui_term_ctx_t *ctx, uint8_t focus);
uint8_t crosstalk_eda_ui_term_focus_is(const crosstalk_eda_ui_term_ctx_t *ctx);
uint8_t crosstalk_eda_ui_term_serial_live_is(const crosstalk_eda_ui_term_ctx_t *ctx);
const char* crosstalk_eda_ui_term_title_get(const crosstalk_eda_ui_term_ctx_t *ctx);
uint32_t crosstalk_eda_ui_term_win_reg(crosstalk_eda_ui_term_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, ldg_mem_pool_t *pool, const crosstalk_eda_io_pty_ctx_t *pty, uint32_t *out_slot);

#endif
