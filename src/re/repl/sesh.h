#ifndef CROSSTALK_RE_REPL_SESH_H
#define CROSSTALK_RE_REPL_SESH_H

#include <stdint.h>

#include <dangling/core/types.h>

#include "core/cmdline.h"
#include "re/repl/complete.h"
#include "re/repl/repl.h"
#include "re/repl/subst.h"

#define CROSSTALK_RE_REPL_SESH_TITLE "repl"
#define CROSSTALK_RE_REPL_SESH_PUDDING_SIZE 7
#define CROSSTALK_RE_REPL_SESH_HIST_FILE "replhist.log"
#define CROSSTALK_RE_REPL_SESH_COL_CUNT 80
#define CROSSTALK_RE_REPL_SESH_ENC_CAP 8192

typedef uint32_t (*crosstalk_re_repl_sesh_exec_cb_t)(void *user, const char *line);

typedef struct crosstalk_re_repl_sesh
{
    crosstalk_re_repl_complete_t complete;
    crosstalk_core_cmdline_ctx_t input;
    crosstalk_re_repl_ctx_t *repl;
    crosstalk_re_repl_subst_t *subst;
    crosstalk_re_repl_sink_cb_t out_cb;
    void *out_user;
    crosstalk_re_repl_sesh_exec_cb_t exec_cb;
    void *exec_user;
    char line[CROSSTALK_CORE_CMDLINE_CAP];
    ldg_byte_t enc[CROSSTALK_RE_REPL_SESH_ENC_CAP];
    uint32_t col_cunt;
    uint8_t prompt_due;
    uint8_t live;
    uint8_t input_ok;
    uint8_t pudding[CROSSTALK_RE_REPL_SESH_PUDDING_SIZE];
} crosstalk_re_repl_sesh_t;

uint32_t crosstalk_re_repl_sesh_init(crosstalk_re_repl_sesh_t *sesh);
uint32_t crosstalk_re_repl_sesh_shutdown(crosstalk_re_repl_sesh_t *sesh);
uint32_t crosstalk_re_repl_sesh_hist_ld(crosstalk_re_repl_sesh_t *sesh, crosstalk_re_repl_ctx_t *repl, crosstalk_re_repl_subst_t *subst, const char *path);
uint32_t crosstalk_re_repl_sesh_open(crosstalk_re_repl_sesh_t *sesh, crosstalk_re_repl_ctx_t *repl, crosstalk_re_repl_subst_t *subst, crosstalk_re_repl_sink_cb_t out_cb, void *out_user);
uint32_t crosstalk_re_repl_sesh_exec_set(crosstalk_re_repl_sesh_t *sesh, crosstalk_re_repl_sesh_exec_cb_t cb, void *user);
uint32_t crosstalk_re_repl_sesh_close(crosstalk_re_repl_sesh_t *sesh);
uint8_t crosstalk_re_repl_sesh_live_is(const crosstalk_re_repl_sesh_t *sesh);
uint32_t crosstalk_re_repl_sesh_bytes_put(crosstalk_re_repl_sesh_t *sesh, const uint8_t *bytes, uint32_t len);
uint32_t crosstalk_re_repl_sesh_prompt_put(crosstalk_re_repl_sesh_t *sesh);
uint32_t crosstalk_re_repl_sesh_col_cunt_set(crosstalk_re_repl_sesh_t *sesh, uint32_t col_cunt);
uint32_t crosstalk_re_repl_sesh_verb_reg(crosstalk_re_repl_sesh_t *sesh, const char *name);
uint32_t crosstalk_re_repl_sesh_tick(crosstalk_re_repl_sesh_t *sesh);

#endif
