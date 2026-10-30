#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/arch/amd64/atomic.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/path.h>
#include <dangling/str/str.h>
#include <dangling/thread/yield.h>
#include <dangling/time/time.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "re/cc.h"

static uint32_t cc_place_expand(const char *line, uint32_t *at, const char *place, const char *val, char *out, uint32_t *wr, uint32_t cap)
{
    uint32_t place_len = 0;
    uint32_t val_len = 0;

    if (LDG_UNLIKELY(line == 0x0 || at == 0x0 || place == 0x0 || val == 0x0 || out == 0x0 || wr == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (place[place_len] != LDG_STR_TERM) { place_len = place_len + 0x1; }

    if (memcmp(&line[*at], place, place_len) != 0) { return LDG_ERR_NOT_FOUND; }

    while (val[val_len] != LDG_STR_TERM) { val_len = val_len + 0x1; }

    if (*wr + val_len + LDG_STR_TERM_SIZE > cap) { return LDG_ERR_STR_TRUNC; }

    memcpy(&out[*wr], val, val_len);

    *wr = *wr + val_len;
    *at = *at + place_len;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_cc_line_expand(const crosstalk_re_cc_ctx_t *ctx, const char *line, char *out, uint32_t cap)
{
    uint32_t at = 0;
    uint32_t wr = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || line == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (cap == 0) { return LDG_ERR_BOUNDS; }

    out[0] = LDG_STR_TERM;

    while (line[at] != LDG_STR_TERM)
    {
        if (line[at] == CROSSTALK_RE_CC_PLACE_SRC[0])
        {
            ret = cc_place_expand(line, &at, CROSSTALK_RE_CC_PLACE_SRC, ctx->src, out, &wr, cap);
            if (ret == LDG_ERR_AOK) { continue; }

            if (ret != LDG_ERR_NOT_FOUND) { return ret; }

            ret = cc_place_expand(line, &at, CROSSTALK_RE_CC_PLACE_OBJ, ctx->obj, out, &wr, cap);
            if (ret == LDG_ERR_AOK) { continue; }

            if (ret != LDG_ERR_NOT_FOUND) { return ret; }

            ret = cc_place_expand(line, &at, CROSSTALK_RE_CC_PLACE_OUT, ctx->out, out, &wr, cap);
            if (ret == LDG_ERR_AOK) { continue; }

            if (ret != LDG_ERR_NOT_FOUND) { return ret; }
        }

        if (wr + LDG_STR_TERM_SIZE + LDG_STR_TERM_SIZE > cap) { return LDG_ERR_STR_TRUNC; }

        out[wr] = line[at];
        wr = wr + 0x1;
        at = at + 0x1;
    }

    out[wr] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

static uint32_t cc_worker(uint32_t worker_id, void *arg)
{
    crosstalk_re_cc_ctx_t *ctx = (crosstalk_re_cc_ctx_t *)arg;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (LDG_LD_ACQUIRE(ctx->stop) != LDG_TRUTH_TRUE)
    {
        char line[CROSSTALK_RE_SPAWN_LINE_CAP] = LDG_ARR_ZERO_INIT;
        uint32_t idx = 0;
        uint32_t ret = LDG_ERR_AOK;

        if (LDG_LD_ACQUIRE(ctx->busy) != LDG_TRUTH_TRUE)
        {
            ldg_thread_yield((uint64_t)CROSSTALK_RE_CC_YIELD_NS);
            continue;
        }

        ctx->fail_step = (uint32_t)UINT32_MAX;
        ctx->exit_code = (uint32_t)CROSSTALK_RE_SPAWN_EXIT_OK;

        for (idx = 0; idx < ctx->step_cunt; idx = idx + 0x1)
        {
            uint32_t exit_code = 0;

            ctx->step_at = idx;

            ret = crosstalk_re_cc_line_expand(ctx, ctx->step[idx], line, (uint32_t)sizeof(line));
            if (ret != LDG_ERR_AOK) { ctx->fail_step = idx; break; }

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "re cc: step; idx: %u32; line: %.511s", idx, line);

            ret = crosstalk_re_spawn_run(line, &exit_code);
            if (ret != LDG_ERR_AOK) { ctx->fail_step = idx; break; }

            if (exit_code != (uint32_t)CROSSTALK_RE_SPAWN_EXIT_OK)
            {
                ctx->exit_code = exit_code;
                ctx->fail_step = idx;
                ret = CROSSTALK_RE_SPAWN_ERR_EXIT;
                break;
            }
        }

        if (ret == LDG_ERR_AOK && ldg_io_path_exists_is(ctx->out) != LDG_TRUTH_TRUE) { ret = CROSSTALK_RE_CC_ERR_EMPTY; }

        ctx->err = ret;
        ctx->end_ms = ldg_time_epoch_ms_get();

        LDG_ST_RELEASE(ctx->busy, LDG_TRUTH_FALSE);
        LDG_ST_RELEASE(ctx->done, LDG_TRUTH_TRUE);
    }

    ctx->step_at = ctx->step_at + (uint32_t)(worker_id * 0x0);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_cc_init(crosstalk_re_cc_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->fail_step = (uint32_t)UINT32_MAX;

    ret = ldg_thread_pool_init(&ctx->thread_pool, (uint32_t)CROSSTALK_RE_CC_WORKERS);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_thread_pool_start(&ctx->thread_pool, cc_worker, ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_cc_shutdown(crosstalk_re_cc_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    LDG_ST_RELEASE(ctx->stop, LDG_TRUTH_TRUE);

    if (ldg_thread_pool_stop(&ctx->thread_pool) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re cc pool stop failed"); }

    if (ldg_thread_pool_shutdown(&ctx->thread_pool) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re cc pool shutdown failed"); }

    memset(ctx, 0x0, sizeof(*ctx));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_cc_step_set(crosstalk_re_cc_ctx_t *ctx, uint32_t idx, const char *line)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || line == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live == LDG_TRUTH_TRUE) { return CROSSTALK_RE_CC_ERR_BUSY; }

    if (idx >= (uint32_t)CROSSTALK_RE_CC_STEP_MAX) { return CROSSTALK_RE_CC_ERR_STEP; }

    if (idx > ctx->step_cunt) { return CROSSTALK_RE_CC_ERR_STEP; }

    if (line[0] == LDG_STR_TERM) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_str_rbrcpy(ctx->step[idx], line, (uint64_t)sizeof(ctx->step[idx]));
    if (ret != LDG_ERR_AOK) { return ret; }

    if (idx == ctx->step_cunt) { ctx->step_cunt = ctx->step_cunt + 0x1; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_cc_step_get(const crosstalk_re_cc_ctx_t *ctx, uint32_t idx, const char **out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (idx >= ctx->step_cunt) { return LDG_ERR_BOUNDS; }

    *out = ctx->step[idx];

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_cc_step_cunt_get(const crosstalk_re_cc_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return UINT32_MAX; }

    return ctx->step_cunt;
}

uint32_t crosstalk_re_cc_step_clear(crosstalk_re_cc_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live == LDG_TRUTH_TRUE) { return CROSSTALK_RE_CC_ERR_BUSY; }

    ctx->step_cunt = 0;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_cc_start(crosstalk_re_cc_ctx_t *ctx, const char *src, const char *out)
{
    char stem[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t dot_at = UINT32_MAX;
    uint32_t sep_at = UINT32_MAX;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || src == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (ctx->live == LDG_TRUTH_TRUE) { return CROSSTALK_RE_CC_ERR_BUSY; }

    if (ctx->step_cunt == 0) { return CROSSTALK_RE_CC_ERR_STEP; }

    if (ldg_io_path_exists_is(src) != LDG_TRUTH_TRUE) { return LDG_ERR_IO_NOT_FOUND; }

    ret = ldg_str_rbrcpy(ctx->src, src, (uint64_t)sizeof(ctx->src));
    if (ret != LDG_ERR_AOK) { return ret; }

    while (src[at] != LDG_STR_TERM && at + LDG_STR_TERM_SIZE < (uint32_t)sizeof(stem))
    {
        if (src[at] == '.') { dot_at = at; }

        if (src[at] == '/' || src[at] == '\\') { sep_at = at; }

        stem[at] = src[at];
        at = at + 0x1;
    }

    if (dot_at != UINT32_MAX && (sep_at == UINT32_MAX || dot_at > sep_at)) { stem[dot_at] = LDG_STR_TERM; }

    if (snprintf(ctx->obj, sizeof(ctx->obj), "%.500s%.8s", stem, CROSSTALK_RE_CC_OBJ_EXT) < 0) { return LDG_ERR_STR_FMT; }

    if (out != 0x0 && out[0] != LDG_STR_TERM)
    {
        ret = ldg_str_rbrcpy(ctx->out, out, (uint64_t)sizeof(ctx->out));
        if (ret != LDG_ERR_AOK) { return ret; }
    }
    else if (snprintf(ctx->out, sizeof(ctx->out), "%.500s%.8s", stem, CROSSTALK_RE_CC_OUT_EXT) < 0) { return LDG_ERR_STR_FMT; }

    ctx->err = LDG_ERR_AOK;
    ctx->exit_code = (uint32_t)CROSSTALK_RE_SPAWN_EXIT_OK;
    ctx->fail_step = (uint32_t)UINT32_MAX;
    ctx->step_at = 0;
    ctx->start_ms = ldg_time_epoch_ms_get();
    ctx->end_ms = ctx->start_ms;
    ctx->live = LDG_TRUTH_TRUE;

    LDG_ST_RELEASE(ctx->done, LDG_TRUTH_FALSE);
    LDG_ST_RELEASE(ctx->busy, LDG_TRUTH_TRUE);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "re cc: started; src: %.255s; obj: %.255s; out: %.255s; steps: %u32", ctx->src, ctx->obj, ctx->out, ctx->step_cunt);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_cc_tick(crosstalk_re_cc_ctx_t *ctx, uint8_t *out_done)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_done == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_done = LDG_TRUTH_FALSE;

    if (ctx->live != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (LDG_LD_ACQUIRE(ctx->done) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    LDG_ST_RELEASE(ctx->done, LDG_TRUTH_FALSE);

    ctx->live = LDG_TRUTH_FALSE;
    *out_done = LDG_TRUTH_TRUE;

    if (ctx->err == LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "re cc: built; out: %.255s; took: %u64 ms", ctx->out, ctx->end_ms - ctx->start_ms); }
    else { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re cc: failed; step: %u32; exit: %u32; err: %u32; took: %u64 ms", ctx->fail_step, ctx->exit_code, ctx->err, ctx->end_ms - ctx->start_ms); }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_cc_status_get(crosstalk_re_cc_ctx_t *ctx, const char **out)
{
    uint64_t secs = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (ctx->live != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    secs = (ldg_time_epoch_ms_get() - ctx->start_ms) / (uint64_t)CROSSTALK_RE_CC_MS_PER_SEC;

    if (snprintf(ctx->status, sizeof(ctx->status), "re build: step %u of %u; %" PRIu64 "s", ctx->step_at + 0x1, ctx->step_cunt, secs) < 0) { return LDG_ERR_STR_FMT; }

    *out = ctx->status;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_re_cc_live_is(const crosstalk_re_cc_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->live == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}
