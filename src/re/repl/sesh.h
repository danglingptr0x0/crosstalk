#ifndef CROSSTALK_RE_REPL_SESH_H
#define CROSSTALK_RE_REPL_SESH_H

#include <stdint.h>

#include "re/repl/complete.h"
#include "re/repl/repl.h"
#include "re/repl/subst.h"

#define CROSSTALK_RE_REPL_SESH_TITLE "repl"
#define CROSSTALK_RE_REPL_SESH_PUDDING_SIZE 1
#define CROSSTALK_RE_REPL_SESH_HIST_CAP 64
#define CROSSTALK_RE_REPL_SESH_HIST_FILE "replhist.log"
#define CROSSTALK_RE_REPL_SESH_FILE_BLK 4096
#define CROSSTALK_RE_REPL_SESH_FILE_CAP 32768
#define CROSSTALK_RE_REPL_SESH_PATH_CAP 1024

typedef struct crosstalk_re_repl_sesh_hist
{
    char text[CROSSTALK_RE_REPL_CMD_CAP];
} crosstalk_re_repl_sesh_hist_t;

typedef struct crosstalk_re_repl_sesh
{
    crosstalk_re_repl_complete_t complete;
    crosstalk_re_repl_sesh_hist_t hist[CROSSTALK_RE_REPL_SESH_HIST_CAP];
    char draft[CROSSTALK_RE_REPL_CMD_CAP];
    char hist_path[CROSSTALK_RE_REPL_SESH_PATH_CAP];
    uint8_t file_buff[CROSSTALK_RE_REPL_SESH_FILE_CAP + CROSSTALK_RE_REPL_SESH_FILE_BLK];
    uint32_t hist_cunt;
    uint32_t hist_sel;
    crosstalk_re_repl_ctx_t *repl;
    crosstalk_re_repl_subst_t *subst;
    crosstalk_re_repl_sink_cb_t out_cb;
    void *out_user;
    char line[CROSSTALK_RE_REPL_CMD_CAP];
    uint32_t len;
    uint8_t esc_state;
    uint8_t prompt_due;
    uint8_t live;
    uint8_t pudding[CROSSTALK_RE_REPL_SESH_PUDDING_SIZE];
} crosstalk_re_repl_sesh_t;

uint32_t crosstalk_re_repl_sesh_init(crosstalk_re_repl_sesh_t *sesh);
uint32_t crosstalk_re_repl_sesh_hist_ld(crosstalk_re_repl_sesh_t *sesh, crosstalk_re_repl_ctx_t *repl, crosstalk_re_repl_subst_t *subst, const char *path);
uint32_t crosstalk_re_repl_sesh_open(crosstalk_re_repl_sesh_t *sesh, crosstalk_re_repl_ctx_t *repl, crosstalk_re_repl_subst_t *subst, crosstalk_re_repl_sink_cb_t out_cb, void *out_user);
uint32_t crosstalk_re_repl_sesh_close(crosstalk_re_repl_sesh_t *sesh);
uint8_t crosstalk_re_repl_sesh_live_is(const crosstalk_re_repl_sesh_t *sesh);
uint32_t crosstalk_re_repl_sesh_bytes_put(crosstalk_re_repl_sesh_t *sesh, const uint8_t *bytes, uint32_t len);
uint32_t crosstalk_re_repl_sesh_prompt_put(crosstalk_re_repl_sesh_t *sesh);
uint32_t crosstalk_re_repl_sesh_tick(crosstalk_re_repl_sesh_t *sesh);

#endif
