#ifndef CROSSTALK_EDA_IO_PTY_H
#define CROSSTALK_EDA_IO_PTY_H

#include <stdint.h>

#include <dangling/io/pty.h>
#include <dangling/mem/alloc.h>
#include <dangling/term/term.h>
#include <dangling/term/vt.h>

#define CROSSTALK_EDA_IO_PTY_TITLE_CAP 64
#define CROSSTALK_EDA_IO_PTY_RD_CAP 16384
#define CROSSTALK_EDA_IO_PTY_CLIP_CAP 8192
#define CROSSTALK_EDA_IO_PTY_COL_MAX 512
#define CROSSTALK_EDA_IO_PTY_ROW_MAX 120
#define CROSSTALK_EDA_IO_PTY_UTF_SEQ_CAP 4

typedef struct crosstalk_eda_io_pty_ctx
{
    ldg_io_pty_t pty;
    ldg_term_vt_t vt;
    ldg_term_t *term;
    ldg_mem_pool_t *pool;
    char title[CROSSTALK_EDA_IO_PTY_TITLE_CAP];
    char clip[CROSSTALK_EDA_IO_PTY_CLIP_CAP];
    uint64_t rd_total;
    uint32_t quiet_ticks;
    uint32_t clip_len;
    uint32_t col_cunt;
    uint32_t row_cunt;
    uint32_t refused_cols;
    uint32_t refused_rows;
    uint32_t exit_code;
    uint8_t utf_carry[CROSSTALK_EDA_IO_PTY_UTF_SEQ_CAP];
    uint32_t utf_carry_len;
    uint32_t utf_need;
    uint8_t clip_new;
    uint8_t live;
    uint8_t exited;
    uint8_t grid_dumped;
    uint8_t pudding[4];
} crosstalk_eda_io_pty_ctx_t;

uint32_t crosstalk_eda_io_pty_init(crosstalk_eda_io_pty_ctx_t *ctx, ldg_mem_pool_t *pool);
uint32_t crosstalk_eda_io_pty_shutdown(crosstalk_eda_io_pty_ctx_t *ctx);
uint32_t crosstalk_eda_io_pty_open(crosstalk_eda_io_pty_ctx_t *ctx, const char *shell, const char *cwd);
uint32_t crosstalk_eda_io_pty_close(crosstalk_eda_io_pty_ctx_t *ctx);
uint32_t crosstalk_eda_io_pty_tick(crosstalk_eda_io_pty_ctx_t *ctx);
uint32_t crosstalk_eda_io_pty_grid_dump(crosstalk_eda_io_pty_ctx_t *ctx);
uint32_t crosstalk_eda_io_pty_keys_wr(crosstalk_eda_io_pty_ctx_t *ctx, const uint8_t *bytes, uint32_t len);
uint32_t crosstalk_eda_io_pty_grid_resize(crosstalk_eda_io_pty_ctx_t *ctx, uint32_t col_cunt, uint32_t row_cunt);
uint32_t crosstalk_eda_io_pty_focus_put(crosstalk_eda_io_pty_ctx_t *ctx, uint8_t focused);
uint32_t crosstalk_eda_io_pty_clip_take(crosstalk_eda_io_pty_ctx_t *ctx, char *out, uint32_t cap, uint32_t *out_len);
uint8_t crosstalk_eda_io_pty_live_is(const crosstalk_eda_io_pty_ctx_t *ctx);

#endif
