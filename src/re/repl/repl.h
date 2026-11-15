#ifndef CROSSTALK_RE_REPL_H
#define CROSSTALK_RE_REPL_H

#include <stdint.h>

#include <dangling/sys/proc.h>

#define CROSSTALK_RE_REPL_CMD_CAP 256
#define CROSSTALK_RE_REPL_LINE_CAP 1024
#define CROSSTALK_RE_REPL_NAME_CAP 192
#define CROSSTALK_RE_REPL_ROW_CAP 64
#define CROSSTALK_RE_REPL_ARG_CAP 13
#define CROSSTALK_RE_REPL_PUDDING_SIZE 4

typedef uint32_t (*crosstalk_re_repl_sink_cb_t)(void *user, const uint8_t *bytes, uint32_t len);

typedef struct crosstalk_re_repl_ctx
{
    ldg_sys_proc_t proc;
    crosstalk_re_repl_sink_cb_t sink_cb;
    void *sink_user;
    void *native;
    void *scan;
    void *thread;
    void *snitch;
    uint64_t alive_ck_ns;
    char req[CROSSTALK_RE_REPL_CMD_CAP];
    char raw[CROSSTALK_RE_REPL_CMD_CAP];
    char line[CROSSTALK_RE_REPL_LINE_CAP];
    char name[CROSSTALK_RE_REPL_NAME_CAP];
    uint8_t attached;
    uint8_t searching;
    uint8_t snitch_wait;
    uint8_t ok;
    uint8_t pudding[CROSSTALK_RE_REPL_PUDDING_SIZE];
} crosstalk_re_repl_ctx_t;

uint32_t crosstalk_re_repl_init(crosstalk_re_repl_ctx_t *ctx);
uint32_t crosstalk_re_repl_shutdown(crosstalk_re_repl_ctx_t *ctx);
uint32_t crosstalk_re_repl_sink_set(crosstalk_re_repl_ctx_t *ctx, crosstalk_re_repl_sink_cb_t cb, void *user);
uint32_t crosstalk_re_repl_attach(crosstalk_re_repl_ctx_t *ctx, uint32_t pid);
uint32_t crosstalk_re_repl_detach(crosstalk_re_repl_ctx_t *ctx);
uint8_t crosstalk_re_repl_attached_is(const crosstalk_re_repl_ctx_t *ctx);
uint8_t crosstalk_re_repl_busy_is(const crosstalk_re_repl_ctx_t *ctx);
uint32_t crosstalk_re_repl_cancel(crosstalk_re_repl_ctx_t *ctx);
uint32_t crosstalk_re_repl_line_run(crosstalk_re_repl_ctx_t *ctx, const char *text);
uint32_t crosstalk_re_repl_tick(crosstalk_re_repl_ctx_t *ctx);
uint32_t crosstalk_re_repl_output_wr(crosstalk_re_repl_ctx_t *ctx, const char *text, uint64_t len);
uint32_t crosstalk_re_repl_emit(crosstalk_re_repl_ctx_t *ctx, const char *fmt, ...);
uint32_t crosstalk_re_repl_name_get(const char *src, char *out, uint32_t cap);
uint32_t crosstalk_re_repl_num_get(const char *text, uint64_t *out);
uint8_t crosstalk_re_repl_cmd_eq_is(const char *lhs, const char *rhs);

#endif
