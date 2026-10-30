#ifndef CROSSTALK_CORE_CMDLINE_H
#define CROSSTALK_CORE_CMDLINE_H

#include <stdint.h>
#include <time.h>

#include <dangling/io/file.h>
#include <dangling/io/path.h>
#include <dangling/thread/sync.h>

#include <dangling/term/render.h>
#include <dangling/term/vt.h>

#define CROSSTALK_CORE_CMDLINE_CAP 640
#define CROSSTALK_CORE_CMDLINE_HINT_CAP 192
#define CROSSTALK_CORE_CMDLINE_VERB_CUNT 96
#define CROSSTALK_CORE_CMDLINE_VERB_NAME_CAP 32
#define CROSSTALK_CORE_CMDLINE_VERB_HELP_CAP 192
#define CROSSTALK_CORE_CMDLINE_HIST_MAX 192
#define CROSSTALK_CORE_CMDLINE_HIST_STEP 1
#define CROSSTALK_CORE_CMDLINE_HIST_FILE "cmd_hist.log"
#define CROSSTALK_CORE_CMDLINE_HIST_EOL "\n"
#define CROSSTALK_CORE_CMDLINE_HIST_CHUNK 512
#define CROSSTALK_CORE_CMDLINE_HIST_MODE 0644
#define CROSSTALK_CORE_CMDLINE_PROMPT_FRAC 0.42
#define CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CUNT 128
#define CROSSTALK_CORE_CMDLINE_DRAWER_SHOW_CUNT 16
#define CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CAP 144
#define CROSSTALK_CORE_CMDLINE_LOG_LVL_T __typeof__(1UL)
#define CROSSTALK_CORE_CMDLINE_CANDIDATE_CUNT 256
#define CROSSTALK_CORE_CMDLINE_CANDIDATE_CAP 64

typedef uint32_t (*crosstalk_core_cmdline_candidate_cb_t)(void *user, const char *line, uint32_t arg_idx, const char *word, char (*out)[CROSSTALK_CORE_CMDLINE_CANDIDATE_CAP], uint32_t out_cap, uint32_t *out_cunt);

typedef struct crosstalk_core_cmdline_ctx
{
    char buff[CROSSTALK_CORE_CMDLINE_CAP];
    char ghost[CROSSTALK_CORE_CMDLINE_CAP];
    char candidate[CROSSTALK_CORE_CMDLINE_CANDIDATE_CUNT][CROSSTALK_CORE_CMDLINE_CANDIDATE_CAP];
    char hint[CROSSTALK_CORE_CMDLINE_HINT_CAP];
    char hist[CROSSTALK_CORE_CMDLINE_HIST_MAX][CROSSTALK_CORE_CMDLINE_CAP];
    char draft[CROSSTALK_CORE_CMDLINE_CAP];
    char isearch[CROSSTALK_CORE_CMDLINE_CAP];
    char line[CROSSTALK_CORE_CMDLINE_CAP];
    char hist_path[LDG_IO_PATH_MAX];
    char drawer[CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CUNT][CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CAP];
    char verb_name[CROSSTALK_CORE_CMDLINE_VERB_CUNT][CROSSTALK_CORE_CMDLINE_VERB_NAME_CAP];
    char verb_help[CROSSTALK_CORE_CMDLINE_VERB_CUNT][CROSSTALK_CORE_CMDLINE_VERB_HELP_CAP];
    uint32_t verb_arg_max[CROSSTALK_CORE_CMDLINE_VERB_CUNT];
    ldg_mut_t drawer_mut;
    crosstalk_core_cmdline_candidate_cb_t candidate_cb;
    void *candidate_user;
    uint32_t candidate_cunt;
    uint32_t candidate_sel;
    uint32_t candidate_at;
    uint32_t drawer_col;
    uint32_t drawer_row;
    uint32_t drawer_col_cunt;
    uint32_t verbs_cunt;
    uint32_t drawer_hd;
    uint32_t drawer_cunt;
    uint32_t drawer_scroll;
    uint32_t hist_hd;
    uint32_t hist_cunt;
    uint32_t hist_sel;
    uint32_t isearch_sel;
    uint32_t cursor;
    uint8_t open;
    uint8_t isearch_on;
    uint8_t drawer_ok;
    uint8_t line_rdy;
    uint8_t candidate_on;
    uint8_t pudding[7];
} crosstalk_core_cmdline_ctx_t;

uint32_t crosstalk_core_cmdline_open(crosstalk_core_cmdline_ctx_t *ctx);
uint32_t crosstalk_core_cmdline_close(crosstalk_core_cmdline_ctx_t *ctx);
uint32_t crosstalk_core_cmdline_esc(crosstalk_core_cmdline_ctx_t *ctx);
uint8_t crosstalk_core_cmdline_open_is(const crosstalk_core_cmdline_ctx_t *ctx);
uint32_t crosstalk_core_cmdline_verb_reg(crosstalk_core_cmdline_ctx_t *ctx, const char *name, const char *help, uint32_t arg_max);
uint32_t crosstalk_core_cmdline_verb_cunt_get(const crosstalk_core_cmdline_ctx_t *ctx, uint32_t *out);
uint32_t crosstalk_core_cmdline_verb_get(const crosstalk_core_cmdline_ctx_t *ctx, uint32_t idx, const char **out_name, const char **out_help);
uint32_t crosstalk_core_cmdline_candidate_cb_set(crosstalk_core_cmdline_ctx_t *ctx, crosstalk_core_cmdline_candidate_cb_t cb, void *user);
uint32_t crosstalk_core_cmdline_hist_push(crosstalk_core_cmdline_ctx_t *ctx, const char *text);
uint32_t crosstalk_core_cmdline_hist_ld(crosstalk_core_cmdline_ctx_t *ctx, const char *path);
uint32_t crosstalk_core_cmdline_hist_st(crosstalk_core_cmdline_ctx_t *ctx);
uint32_t crosstalk_core_cmdline_hist_get(const crosstalk_core_cmdline_ctx_t *ctx, uint32_t idx, const char **out);
uint32_t crosstalk_core_cmdline_drawer_init(crosstalk_core_cmdline_ctx_t *ctx);
uint32_t crosstalk_core_cmdline_drawer_shutdown(crosstalk_core_cmdline_ctx_t *ctx);
uint32_t crosstalk_core_cmdline_drawer_overlay_draw(crosstalk_core_cmdline_ctx_t *ctx, ldg_term_render_t *render);
uint32_t crosstalk_core_cmdline_out_push(crosstalk_core_cmdline_ctx_t *ctx, const char *text);
void crosstalk_core_cmdline_log_cb(void *cls, const char *app_name, const time_t date, const CROSSTALK_CORE_CMDLINE_LOG_LVL_T level, const char *msg);
uint32_t crosstalk_core_cmdline_draw(crosstalk_core_cmdline_ctx_t *ctx, ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t col_cunt);
uint32_t crosstalk_core_cmdline_key_isr(crosstalk_core_cmdline_ctx_t *ctx, const ldg_term_key_event_t *irq, uint8_t *consumed);
uint32_t crosstalk_core_cmdline_line_take(crosstalk_core_cmdline_ctx_t *ctx, char *out, uint32_t out_cap);

#endif
