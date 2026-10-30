#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/core/types.h>
#include <dangling/sys/proc.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "hdl/proc.h"

static uint32_t stream_drain(ldg_sys_proc_t *proc, ldg_byte_t *dst, uint32_t *io_len, uint32_t cap, uint8_t *io_eof, uint8_t err_is)
{
    uint64_t got = 0;
    uint64_t room = 0;
    uint32_t pass = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(proc == 0x0 || dst == 0x0 || io_len == 0x0 || io_eof == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (*io_eof == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    for (pass = 0; pass < CROSSTALK_HDL_PROC_DRAIN_MAX; pass = pass + 0x1)
    {
        room = (uint64_t)(cap - *io_len);
        if (room == 0) { return LDG_ERR_AOK; }

        if (room > (uint64_t)CROSSTALK_HDL_PROC_RD_CAP) { room = (uint64_t)CROSSTALK_HDL_PROC_RD_CAP; }

        got = 0;

        if (err_is == LDG_TRUTH_TRUE) { ret = ldg_sys_proc_stderr_rd(proc, dst + *io_len, room, &got); }
        else { ret = ldg_sys_proc_stdout_rd(proc, dst + *io_len, room, &got); }

        if (ret == LDG_ERR_EOF)
        {
            *io_eof = LDG_TRUTH_TRUE;
            return LDG_ERR_AOK;
        }

        if (ret == LDG_ERR_WOULD_BLOCK || ret == LDG_ERR_INTERRUPTED) { return LDG_ERR_AOK; }

        if (ret == LDG_ERR_CLOSED)
        {
            *io_eof = LDG_TRUTH_TRUE;
            return LDG_ERR_AOK;
        }

        if (ret != LDG_ERR_AOK) { return ret; }

        *io_len = *io_len + (uint32_t)got;
    }

    return LDG_ERR_AOK;
}

static uint32_t stdin_flush(crosstalk_hdl_proc_ctx_t *ctx)
{
    uint64_t sent = 0;
    uint32_t pass = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (pass = 0; pass < CROSSTALK_HDL_PROC_DRAIN_MAX; pass = pass + 0x1)
    {
        if (ctx->in_pos >= ctx->in_len)
        {
            ctx->in_pos = 0x0;
            ctx->in_len = 0x0;
            return LDG_ERR_AOK;
        }

        sent = 0;
        ret = ldg_sys_proc_stdin_wr(&ctx->proc, ctx->in + ctx->in_pos, (uint64_t)(ctx->in_len - ctx->in_pos), &sent);

        if (ret == LDG_ERR_WOULD_BLOCK || ret == LDG_ERR_INTERRUPTED) { return LDG_ERR_AOK; }

        if (ret == LDG_ERR_CLOSED)
        {
            ctx->in_pos = 0x0;
            ctx->in_len = 0x0;
            return LDG_ERR_AOK;
        }

        if (ret != LDG_ERR_AOK) { return ret; }

        ctx->in_pos = ctx->in_pos + (uint32_t)sent;
        ctx->wr_total = ctx->wr_total + sent;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_proc_init(crosstalk_hdl_proc_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *ctx = (crosstalk_hdl_proc_ctx_t)LDG_STRUCT_ZERO_INIT;
    ctx->state = CROSSTALK_HDL_PROC_STATE_IDLE;
    ctx->exit_code = UINT32_MAX;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_proc_start(crosstalk_hdl_proc_ctx_t *ctx, const char *path, const char *const *argv, const char *cwd)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || path == 0x0 || argv == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->state != CROSSTALK_HDL_PROC_STATE_IDLE) { return CROSSTALK_HDL_PROC_ERR_STATE; }

    if (argv[0] == 0x0) { return CROSSTALK_HDL_PROC_ERR_ARGV; }

    ctx->out_len = 0x0;
    ctx->err_len = 0x0;
    ctx->in_len = 0x0;
    ctx->in_pos = 0x0;
    ctx->rd_total = 0x0;
    ctx->wr_total = 0x0;
    ctx->out_eof = LDG_TRUTH_FALSE;
    ctx->err_eof = LDG_TRUTH_FALSE;
    ctx->exit_code = UINT32_MAX;
    ctx->proc = (ldg_sys_proc_t)LDG_STRUCT_ZERO_INIT;

    ret = ldg_sys_proc_spawn(path, argv, cwd, &ctx->proc);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "proc: spawn failed; path: %s; err: %u32", path, ret);
        return ret;
    }

    ctx->state = CROSSTALK_HDL_PROC_STATE_RUN;
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "proc: spawned; path: %s; pid: %u32", path, ctx->proc.pid);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_proc_pump(crosstalk_hdl_proc_ctx_t *ctx)
{
    uint8_t alive = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->state == CROSSTALK_HDL_PROC_STATE_IDLE) { return CROSSTALK_HDL_PROC_ERR_STATE; }

    alive = ldg_sys_proc_alive_is(&ctx->proc);

    ret = stdin_flush(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = stream_drain(&ctx->proc, ctx->out, &ctx->out_len, CROSSTALK_HDL_PROC_OUT_CAP, &ctx->out_eof, LDG_TRUTH_FALSE);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = stream_drain(&ctx->proc, ctx->err, &ctx->err_len, CROSSTALK_HDL_PROC_ERR_CAP, &ctx->err_eof, LDG_TRUTH_TRUE);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (alive != LDG_TRUTH_TRUE && ctx->state == CROSSTALK_HDL_PROC_STATE_RUN)
    {
        ret = ldg_sys_proc_exit_code_get(&ctx->proc, &ctx->exit_code);
        if (ret == LDG_ERR_AGAIN) { return LDG_ERR_AOK; }

        if (ret != LDG_ERR_AOK) { return ret; }

        ctx->state = CROSSTALK_HDL_PROC_STATE_EXIT;
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "proc: exited; pid: %u32; code: %u32; out: %u32; err: %u32", ctx->proc.pid, ctx->exit_code, ctx->out_len, ctx->err_len);
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_proc_wr(crosstalk_hdl_proc_ctx_t *ctx, const ldg_byte_t *buff, uint32_t len, uint32_t *out_taken)
{
    uint32_t room = 0;
    uint32_t take = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || buff == 0x0 || out_taken == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_taken = 0x0;

    if (ctx->state != CROSSTALK_HDL_PROC_STATE_RUN) { return CROSSTALK_HDL_PROC_ERR_STATE; }

    if (ctx->in_pos > 0)
    {
        memmove(ctx->in, ctx->in + ctx->in_pos, (uint64_t)(ctx->in_len - ctx->in_pos));
        ctx->in_len = ctx->in_len - ctx->in_pos;
        ctx->in_pos = 0x0;
    }

    room = CROSSTALK_HDL_PROC_IN_CAP - ctx->in_len;
    if (room == 0) { return LDG_ERR_WOULD_BLOCK; }

    take = (len < room) ? len : room;
    memcpy(ctx->in + ctx->in_len, buff, (uint64_t)take);
    ctx->in_len = ctx->in_len + take;
    *out_taken = take;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_proc_rd(crosstalk_hdl_proc_ctx_t *ctx, ldg_byte_t *buff, uint32_t cap, uint32_t *out_got)
{
    uint32_t take = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || buff == 0x0 || out_got == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_got = 0x0;

    if (ctx->out_len == 0)
    {
        if (ctx->out_eof == LDG_TRUTH_TRUE) { return LDG_ERR_EOF; }

        return LDG_ERR_WOULD_BLOCK;
    }

    take = (cap < ctx->out_len) ? cap : ctx->out_len;
    memcpy(buff, ctx->out, (uint64_t)take);

    if (take < ctx->out_len) { memmove(ctx->out, ctx->out + take, (uint64_t)(ctx->out_len - take)); }

    ctx->out_len = ctx->out_len - take;
    ctx->rd_total = ctx->rd_total + (uint64_t)take;
    *out_got = take;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_proc_err_get(const crosstalk_hdl_proc_ctx_t *ctx, const ldg_byte_t **out_err, uint32_t *out_len)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_err == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_err = ctx->err;
    *out_len = ctx->err_len;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_proc_exit_code_get(const crosstalk_hdl_proc_ctx_t *ctx, uint32_t *out_code)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_code == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_code = ctx->exit_code;

    if (ctx->state != CROSSTALK_HDL_PROC_STATE_EXIT) { return LDG_ERR_AGAIN; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_proc_stop(crosstalk_hdl_proc_ctx_t *ctx)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->state == CROSSTALK_HDL_PROC_STATE_IDLE) { return LDG_ERR_AOK; }

    if (ldg_sys_proc_alive_is(&ctx->proc) == LDG_TRUTH_TRUE)
    {
        ret = ldg_sys_proc_kill(&ctx->proc);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "proc: kill failed; pid: %u32; err: %u32", ctx->proc.pid, ret); }
    }

    ret = ldg_sys_proc_close(&ctx->proc);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "proc: close failed; pid: %u32; err: %u32", ctx->proc.pid, ret); }

    ctx->proc = (ldg_sys_proc_t)LDG_STRUCT_ZERO_INIT;
    ctx->state = CROSSTALK_HDL_PROC_STATE_IDLE;
    ctx->in_len = 0x0;
    ctx->in_pos = 0x0;

    return ret;
}

uint8_t crosstalk_hdl_proc_alive_is(const crosstalk_hdl_proc_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->state == CROSSTALK_HDL_PROC_STATE_RUN) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}
