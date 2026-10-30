#ifndef CROSSTALK_RE_CC_H
#define CROSSTALK_RE_CC_H

#include <stdint.h>

#include <dangling/io/path.h>
#include <dangling/thread/pool.h>

#include "re/spawn.h"

#define CROSSTALK_RE_CC_ERR_BUSY 816
#define CROSSTALK_RE_CC_ERR_STEP 817
#define CROSSTALK_RE_CC_ERR_EMPTY 818

#define CROSSTALK_RE_CC_STEP_MAX 8
#define CROSSTALK_RE_CC_LINE_CAP 1024
#define CROSSTALK_RE_CC_WORKERS 1
#define CROSSTALK_RE_CC_YIELD_NS 200000
#define CROSSTALK_RE_CC_STATUS_CAP 96
#define CROSSTALK_RE_CC_MS_PER_SEC 1000
#define CROSSTALK_RE_CC_PLACE_SRC "{src}"
#define CROSSTALK_RE_CC_PLACE_OBJ "{obj}"
#define CROSSTALK_RE_CC_PLACE_OUT "{out}"
#define CROSSTALK_RE_CC_OBJ_EXT ".o"
#define CROSSTALK_RE_CC_OUT_EXT ".emiru"

typedef struct crosstalk_re_cc_ctx
{
    ldg_thread_pool_t thread_pool;
    char step[CROSSTALK_RE_CC_STEP_MAX][CROSSTALK_RE_CC_LINE_CAP];
    char src[LDG_IO_PATH_MAX];
    char obj[LDG_IO_PATH_MAX];
    char out[LDG_IO_PATH_MAX];
    char status[CROSSTALK_RE_CC_STATUS_CAP];
    uint64_t start_ms;
    uint64_t end_ms;
    uint32_t step_cunt;
    uint32_t step_at;
    uint32_t fail_step;
    uint32_t exit_code;
    uint32_t err;
    uint32_t busy;
    uint32_t done;
    uint32_t stop;
    uint8_t live;
    uint8_t ok;
    uint8_t pudding[6];
} crosstalk_re_cc_ctx_t;

uint32_t crosstalk_re_cc_init(crosstalk_re_cc_ctx_t *ctx);
uint32_t crosstalk_re_cc_shutdown(crosstalk_re_cc_ctx_t *ctx);
uint32_t crosstalk_re_cc_step_set(crosstalk_re_cc_ctx_t *ctx, uint32_t idx, const char *line);
uint32_t crosstalk_re_cc_step_get(const crosstalk_re_cc_ctx_t *ctx, uint32_t idx, const char **out);
uint32_t crosstalk_re_cc_step_cunt_get(const crosstalk_re_cc_ctx_t *ctx);
uint32_t crosstalk_re_cc_step_clear(crosstalk_re_cc_ctx_t *ctx);
uint32_t crosstalk_re_cc_start(crosstalk_re_cc_ctx_t *ctx, const char *src, const char *out);
uint32_t crosstalk_re_cc_tick(crosstalk_re_cc_ctx_t *ctx, uint8_t *out_done);
uint32_t crosstalk_re_cc_status_get(crosstalk_re_cc_ctx_t *ctx, const char **out);
uint32_t crosstalk_re_cc_line_expand(const crosstalk_re_cc_ctx_t *ctx, const char *line, char *out, uint32_t cap);
uint8_t crosstalk_re_cc_live_is(const crosstalk_re_cc_ctx_t *ctx);

#endif
