#ifndef CROSSTALK_CORE_CMDLINE_H
#define CROSSTALK_CORE_CMDLINE_H

#include <stdint.h>
#include <time.h>

#include <dangling/core/types.h>
#include <dangling/io/file.h>
#include <dangling/io/path.h>
#include <dangling/mem/alloc.h>

#include <dangling/shell/shell.h>
#include <dangling/shell/cmd.h>
#include <dangling/shell/hist.h>

#include <dangling/term/ln.h>
#include <dangling/term/render.h>
#include <dangling/term/vt.h>

#define CROSSTALK_CORE_CMDLINE_CAP LDG_TERM_LN_CAP_DEFAULT
#define CROSSTALK_CORE_CMDLINE_VERB_CUNT LDG_SHELL_CMD_MAX_DEFAULT
#define CROSSTALK_CORE_CMDLINE_HIST_FILE "cmd_hist.log"
#define CROSSTALK_CORE_CMDLINE_HIST_MODE 0644
#define CROSSTALK_CORE_CMDLINE_IO_BLK 4096
#define CROSSTALK_CORE_CMDLINE_IO_CAP ((((uint64_t)LDG_SHELL_LN_CAP_DEFAULT + (uint64_t)LDG_SHELL_HIST_ROW_EXTRA) * (uint64_t)LDG_SHELL_HIST_MAX_DEFAULT + (uint64_t)CROSSTALK_CORE_CMDLINE_IO_BLK - 1ULL) & ~((uint64_t)CROSSTALK_CORE_CMDLINE_IO_BLK - 1ULL))
#define CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CAP LDG_SHELL_OUT_LN_CAP_DEFAULT
#define CROSSTALK_CORE_CMDLINE_DRAWER_SHOW_CUNT 16
#define CROSSTALK_CORE_CMDLINE_LOG_LVL_T __typeof__(1UL)
#define CROSSTALK_CORE_CMDLINE_CANDIDATE_CAP LDG_TERM_LN_CANDIDATE_CAP
#define CROSSTALK_CORE_CMDLINE_PUDDING_SIZE 7
#define CROSSTALK_CORE_CMDLINE_LF_ROOM 1
#define CROSSTALK_CORE_CMDLINE_LAST_CH_BACK 1

typedef struct crosstalk_core_cmdline_ctx
{
    char line[CROSSTALK_CORE_CMDLINE_CAP];
    char hist_path[LDG_IO_PATH_MAX];
    ldg_mem_pool_t *pool;
    ldg_mem_pool_t *edit_pool;
    ldg_shell_t *shell;
    ldg_term_ln_t *edit;
    ldg_term_ln_candidate_cb_t candidate_cb;
    void *candidate_user;
    void *io_raw;
    uint8_t *io_buff;
    uint64_t io_cap;
    uint64_t out_floor;
    uint64_t wipe_seen;
    uint32_t drawer_col;
    uint32_t drawer_row;
    uint32_t drawer_col_cunt;
    uint8_t open;
    uint8_t drawer_ok;
    uint8_t line_rdy;
    uint8_t echo_off;
    uint8_t verb_seen;
    uint8_t pudding[CROSSTALK_CORE_CMDLINE_PUDDING_SIZE];
} crosstalk_core_cmdline_ctx_t;

uint32_t crosstalk_core_cmdline_open(crosstalk_core_cmdline_ctx_t *ctx);
uint32_t crosstalk_core_cmdline_close(crosstalk_core_cmdline_ctx_t *ctx);
uint8_t crosstalk_core_cmdline_open_is(const crosstalk_core_cmdline_ctx_t *ctx);
uint32_t crosstalk_core_cmdline_verb_reg(crosstalk_core_cmdline_ctx_t *ctx, const char *name, const char *help, uint32_t arg_max);
uint32_t crosstalk_core_cmdline_ln_trim(char *text, uint64_t cap, uint64_t *out_len);
uint32_t crosstalk_core_cmdline_wipe_ck(crosstalk_core_cmdline_ctx_t *ctx);
uint32_t crosstalk_core_cmdline_candidate_cb_set(crosstalk_core_cmdline_ctx_t *ctx, ldg_term_ln_candidate_cb_t cb, void *user);
uint32_t crosstalk_core_cmdline_prompt_set(crosstalk_core_cmdline_ctx_t *ctx, const char *prompt);
uint32_t crosstalk_core_cmdline_hist_ld(crosstalk_core_cmdline_ctx_t *ctx, const char *path);
uint32_t crosstalk_core_cmdline_drawer_init(crosstalk_core_cmdline_ctx_t *ctx);
uint32_t crosstalk_core_cmdline_drawer_shutdown(crosstalk_core_cmdline_ctx_t *ctx);
uint32_t crosstalk_core_cmdline_drawer_overlay_draw(crosstalk_core_cmdline_ctx_t *ctx, ldg_term_render_t *render);
uint32_t crosstalk_core_cmdline_out_push(crosstalk_core_cmdline_ctx_t *ctx, const char *text);
uint32_t crosstalk_core_cmdline_out_seq_get(crosstalk_core_cmdline_ctx_t *ctx, uint64_t *out);
uint32_t crosstalk_core_cmdline_out_get(crosstalk_core_cmdline_ctx_t *ctx, uint64_t seq, char *out, uint32_t out_cap, uint64_t *out_seq);
uint32_t crosstalk_core_cmdline_echo_set(crosstalk_core_cmdline_ctx_t *ctx, uint8_t on);
void crosstalk_core_cmdline_log_cb(void *cls, const char *app_name, const time_t date, const CROSSTALK_CORE_CMDLINE_LOG_LVL_T level, const char *msg);
uint32_t crosstalk_core_cmdline_draw(crosstalk_core_cmdline_ctx_t *ctx, ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t col_cunt);
uint32_t crosstalk_core_cmdline_encode(crosstalk_core_cmdline_ctx_t *ctx, uint32_t col_cunt, ldg_byte_t *out, uint64_t cap, uint64_t *out_len);
uint32_t crosstalk_core_cmdline_key_isr(crosstalk_core_cmdline_ctx_t *ctx, const ldg_term_key_event_t *irq, uint8_t *consumed);
uint32_t crosstalk_core_cmdline_line_take(crosstalk_core_cmdline_ctx_t *ctx, char *out, uint32_t out_cap);

#endif
