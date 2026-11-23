#include <stdarg.h>
#include <stdint.h>

#include <dangling/core/arith.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/secure.h>
#include <dangling/str/fmt.h>
#include <dangling/str/str.h>
#include <dangling/sys/proc.h>
#include <dangling/time/time.h>

#include "re/repl/native.h"
#include "re/repl/repl.h"
#include "re/repl/scan.h"
#include "re/repl/snitch.h"
#include "re/repl/thread.h"

#define REPL_WORD_SNITCH "snitch"

#define REPL_WORD_THREADS "threads"
#define REPL_WORD_THREAD "thread"
#define REPL_STEP 1
#define REPL_ESCAPE_SIZE 4
#define REPL_HEX_PREFIX_LEN 2
#define REPL_HEX_PAIR_CAP 3
#define REPL_HEX_HI 0
#define REPL_HEX_LO 1
#define REPL_ALIVE_CK_NS LDG_NS_PER_SEC
#define REPL_ARG_VERB 0
#define REPL_ARG_X 1
#define REPL_ARGC_VERB 1
#define REPL_ARGC_X 2
#define REPL_CH_DIGIT_FIRST '0'
#define REPL_CH_HEX_LOWER 'x'
#define REPL_CH_HEX_UPPER 'X'
#define REPL_NAME_NONE "-"
#define REPL_EOL "\n"
#define REPL_WORD_HELP "help"
#define REPL_WORD_STATUS "status"
#define REPL_WORD_NATIVE "native"
#define REPL_WORD_ATTACH "attach"
#define REPL_WORD_DETACH "detach"

uint32_t crosstalk_re_repl_output_wr(crosstalk_re_repl_ctx_t *ctx, const char *text, uint64_t len)
{
    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->sink_cb == 0x0 || len == 0) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(len > (uint64_t)UINT32_MAX)) { return LDG_ERR_RANGE; }

    return ctx->sink_cb(ctx->sink_user, (const uint8_t *)text, (uint32_t)len);
}

uint32_t crosstalk_re_repl_emit(crosstalk_re_repl_ctx_t *ctx, const char *fmt, ...)
{
    va_list args;
    uint64_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || fmt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    va_start(args, fmt);
    ret = ldg_str_var_fmt(ctx->line, (uint64_t)sizeof(ctx->line), &len, fmt, args);
    va_end(args);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_re_repl_output_wr(ctx, ctx->line, len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_re_repl_output_wr(ctx, REPL_EOL, (uint64_t)sizeof(REPL_EOL) - LDG_STR_TERM_SIZE);
}

uint32_t crosstalk_re_repl_name_get(const char *src, char *out, uint32_t cap)
{
    char hex[REPL_HEX_PAIR_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t at = 0;
    uint32_t dst = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = LDG_STR_TERM;

    if (src == 0x0) { return ldg_str_rbrcpy(out, REPL_NAME_NONE, (uint64_t)cap); }

    for (at = 0; src[at] != LDG_STR_TERM; at = at + (uint32_t)REPL_STEP)
    {
        if ((uint8_t)src[at] >= (uint8_t)LDG_ASCII_PRINTABLE_MIN && (uint8_t)src[at] <= (uint8_t)LDG_ASCII_PRINTABLE_MAX)
        {
            if (dst + LDG_STR_TERM_SIZE >= cap) { break; }

            out[dst] = src[at];
            dst = dst + (uint32_t)REPL_STEP;
            continue;
        }

        if (dst + (uint32_t)REPL_ESCAPE_SIZE >= cap) { break; }

        ret = ldg_byte_to_hex((uint8_t)src[at], hex);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        out[dst] = LDG_ASCII_BSLASH;
        out[dst + (uint32_t)REPL_STEP] = REPL_CH_HEX_LOWER;
        out[dst + (uint32_t)REPL_HEX_PREFIX_LEN] = hex[REPL_HEX_HI];
        out[dst + (uint32_t)REPL_HEX_PAIR_CAP] = hex[REPL_HEX_LO];
        dst = dst + (uint32_t)REPL_ESCAPE_SIZE;
    }

    out[dst] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_repl_num_get(const char *text, uint64_t *out)
{
    uint32_t base = LDG_BASE_DEC;
    uint32_t skip = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(text == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (text[REPL_HEX_HI] == REPL_CH_DIGIT_FIRST && (text[REPL_HEX_LO] == REPL_CH_HEX_LOWER || text[REPL_HEX_LO] == REPL_CH_HEX_UPPER))
    {
        base = LDG_BASE_HEX;
        skip = (uint32_t)REPL_HEX_PREFIX_LEN;
    }

    if (text[skip] == LDG_STR_TERM) { return LDG_ERR_STR_INVALID; }

    for (at = skip; text[at] != LDG_STR_TERM; at = at + (uint32_t)REPL_STEP)
    {
        if (base == LDG_BASE_DEC && ldg_char_digit_is((uint8_t)text[at]) != LDG_TRUTH_TRUE) { return LDG_ERR_STR_INVALID; }

        if (base == LDG_BASE_HEX && ldg_char_hex_is((uint8_t)text[at]) != LDG_TRUTH_TRUE) { return LDG_ERR_STR_INVALID; }
    }

    return ldg_str_uint_parse(text + skip, base, (uint64_t)(at - skip), out);
}

uint8_t crosstalk_re_repl_cmd_eq_is(const char *lhs, const char *rhs)
{
    uint32_t cmp = 0;

    if (LDG_UNLIKELY(lhs == 0x0 || rhs == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ldg_str_cmp(lhs, rhs, (uint64_t)CROSSTALK_RE_REPL_CMD_CAP, &cmp) != LDG_ERR_AOK) { return LDG_TRUTH_FALSE; }

    return cmp == LDG_ARITH_ORD_EQ ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t repl_args_get(crosstalk_re_repl_ctx_t *ctx, char **args, uint32_t *out_cunt)
{
    uint32_t at = 0;
    uint32_t cunt = 0;
    uint8_t in_word = LDG_TRUTH_FALSE;

    *out_cunt = 0;

    for (at = 0; ctx->req[at] != LDG_STR_TERM; at = at + (uint32_t)REPL_STEP)
    {
        if (ctx->req[at] == LDG_ASCII_SPACE || ctx->req[at] == LDG_ASCII_TAB)
        {
            ctx->req[at] = LDG_STR_TERM;
            in_word = LDG_TRUTH_FALSE;
            continue;
        }

        if ((uint8_t)ctx->req[at] < (uint8_t)LDG_ASCII_PRINTABLE_MIN || (uint8_t)ctx->req[at] > (uint8_t)LDG_ASCII_PRINTABLE_MAX) { return LDG_ERR_STR_INVALID; }

        if (in_word == LDG_TRUTH_TRUE) { continue; }

        if (cunt == (uint32_t)CROSSTALK_RE_REPL_ARG_CAP) { return LDG_ERR_FULL; }

        args[cunt] = &ctx->req[at];
        cunt = cunt + (uint32_t)REPL_STEP;
        in_word = LDG_TRUTH_TRUE;
    }

    *out_cunt = cunt;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_repl_init(crosstalk_re_repl_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_secure_zero(ctx, (uint64_t)sizeof(*ctx));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_re_repl_native_init(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_re_repl_scan_init(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_re_repl_thread_init(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_re_repl_snitch_init(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_repl_shutdown(crosstalk_re_repl_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->sink_cb = 0x0;
    ctx->sink_user = 0x0;

    ret = crosstalk_re_repl_detach(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_re_repl_scan_shutdown(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_re_repl_thread_shutdown(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_re_repl_snitch_shutdown(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_re_repl_native_shutdown(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->ok = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_repl_sink_set(crosstalk_re_repl_ctx_t *ctx, crosstalk_re_repl_sink_cb_t cb, void *user)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->sink_cb = cb;
    ctx->sink_user = user;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_re_repl_attached_is(const crosstalk_re_repl_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->attached == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint8_t crosstalk_re_repl_busy_is(const crosstalk_re_repl_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->searching == LDG_TRUTH_TRUE || ctx->snitch_wait == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_re_repl_cancel(crosstalk_re_repl_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_re_repl_native_rst(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    return crosstalk_re_repl_snitch_cancel(ctx);
}

static const char* repl_raw_rest_get(const crosstalk_re_repl_ctx_t *ctx)
{
    uint32_t at = 0;

    while (ctx->raw[at] == LDG_ASCII_SPACE || ctx->raw[at] == LDG_ASCII_TAB) { at = at + (uint32_t)REPL_STEP; }

    while (ctx->raw[at] != LDG_STR_TERM && ctx->raw[at] != LDG_ASCII_SPACE && ctx->raw[at] != LDG_ASCII_TAB) { at = at + (uint32_t)REPL_STEP; }

    while (ctx->raw[at] == LDG_ASCII_SPACE || ctx->raw[at] == LDG_ASCII_TAB) { at = at + (uint32_t)REPL_STEP; }

    return &ctx->raw[at];
}

uint32_t crosstalk_re_repl_detach(crosstalk_re_repl_ctx_t *ctx)
{
    uint32_t pid = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->attached != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    pid = ctx->proc.pid;

    ret = crosstalk_re_repl_native_rst(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_re_repl_scan_rst(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_re_repl_snitch_rst(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_sys_proc_close(&ctx->proc);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->attached = LDG_TRUTH_FALSE;

    ret = crosstalk_re_repl_thread_rst(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    return crosstalk_re_repl_emit(ctx, "detached; pid: %u32", pid);
}

uint32_t crosstalk_re_repl_attach(crosstalk_re_repl_ctx_t *ctx, uint32_t pid)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->ok != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = crosstalk_re_repl_detach(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_sys_proc_open(pid, &ctx->proc);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->attached = LDG_TRUTH_TRUE;
    ctx->alive_ck_ns = 0;

    ret = crosstalk_re_repl_native_rst(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_re_repl_scan_rst(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_re_repl_thread_rst(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (ldg_sys_proc_img_path_get(&ctx->proc, ctx->name, (uint64_t)sizeof(ctx->name)) != LDG_ERR_AOK)
    {
        ret = ldg_str_rbrcpy(ctx->name, REPL_NAME_NONE, (uint64_t)sizeof(ctx->name));
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return crosstalk_re_repl_emit(ctx, "attached; pid: %u32; exe: %.191s", pid, ctx->name);
}

static uint32_t repl_help(crosstalk_re_repl_ctx_t *ctx, char **args)
{
    uint32_t ret = 0;

    ret = crosstalk_re_repl_emit(ctx, "attach <pid> | detach | help | status | threads [start] | thread [idx] | native <command...> | snitch <command...>; any other command goes to snitch");
    if (ret != LDG_ERR_AOK) { return ret; }

    return crosstalk_re_repl_native_cmd(ctx, (uint32_t)REPL_ARGC_VERB, args);
}

static uint32_t repl_status(crosstalk_re_repl_ctx_t *ctx, char **args)
{
    uint32_t ret = 0;

    ret = crosstalk_re_repl_emit(ctx, "attached: %u32; pid: %u32; searching: %u32", (uint32_t)ctx->attached, ctx->attached == LDG_TRUTH_TRUE ? ctx->proc.pid : LDG_SYS_PROC_PID_NONE, (uint32_t)ctx->searching);
    if (ret != LDG_ERR_AOK || ctx->attached != LDG_TRUTH_TRUE) { return ret; }

    return crosstalk_re_repl_native_cmd(ctx, (uint32_t)REPL_ARGC_VERB, args);
}

static uint32_t repl_attach_cmd(crosstalk_re_repl_ctx_t *ctx, uint32_t argc, char **args)
{
    uint64_t pid = 0;
    uint32_t ret = 0;

    if (argc != (uint32_t)REPL_ARGC_X) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = crosstalk_re_repl_num_get(args[REPL_ARG_X], &pid);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (pid >= (uint64_t)LDG_SYS_PROC_PID_NONE) { return LDG_ERR_RANGE; }

    return crosstalk_re_repl_attach(ctx, (uint32_t)pid);
}

static uint32_t repl_dispatch(crosstalk_re_repl_ctx_t *ctx, uint32_t argc, char **args)
{
    uint32_t ret = 0;

    if (crosstalk_re_repl_busy_is(ctx) == LDG_TRUTH_TRUE) { return LDG_ERR_BUSY; }

    if (crosstalk_re_repl_cmd_eq_is(args[REPL_ARG_VERB], REPL_WORD_ATTACH) == LDG_TRUTH_TRUE) { return repl_attach_cmd(ctx, argc, args); }

    if (crosstalk_re_repl_cmd_eq_is(args[REPL_ARG_VERB], REPL_WORD_DETACH) == LDG_TRUTH_TRUE) { return argc == (uint32_t)REPL_ARGC_VERB ? crosstalk_re_repl_detach(ctx) : LDG_ERR_FUNC_ARG_INVALID; }

    if (crosstalk_re_repl_cmd_eq_is(args[REPL_ARG_VERB], REPL_WORD_HELP) == LDG_TRUTH_TRUE && argc == (uint32_t)REPL_ARGC_VERB) { return repl_help(ctx, args); }

    if (crosstalk_re_repl_cmd_eq_is(args[REPL_ARG_VERB], REPL_WORD_STATUS) == LDG_TRUTH_TRUE && argc == (uint32_t)REPL_ARGC_VERB) { return repl_status(ctx, args); }

    if (ctx->attached != LDG_TRUTH_TRUE)
    {
        ret = crosstalk_re_repl_emit(ctx, "not attached; use attach <pid>");
        return ret == LDG_ERR_AOK ? LDG_ERR_NOT_INIT : ret;
    }

    if (crosstalk_re_repl_cmd_eq_is(args[REPL_ARG_VERB], REPL_WORD_THREADS) == LDG_TRUTH_TRUE || crosstalk_re_repl_cmd_eq_is(args[REPL_ARG_VERB], REPL_WORD_THREAD) == LDG_TRUTH_TRUE) { return crosstalk_re_repl_thread_cmd(ctx, argc, args); }

    if (crosstalk_re_repl_cmd_eq_is(args[REPL_ARG_VERB], REPL_WORD_NATIVE) == LDG_TRUTH_TRUE)
    {
        if (argc < (uint32_t)REPL_ARGC_X) { return LDG_ERR_FUNC_ARG_INVALID; }

        return crosstalk_re_repl_native_cmd(ctx, argc - (uint32_t)REPL_STEP, args + REPL_STEP);
    }

    if (crosstalk_re_repl_cmd_eq_is(args[REPL_ARG_VERB], REPL_WORD_SNITCH) == LDG_TRUTH_TRUE)
    {
        if (argc < (uint32_t)REPL_ARGC_X) { return LDG_ERR_FUNC_ARG_INVALID; }

        return crosstalk_re_repl_snitch_line_send(ctx, repl_raw_rest_get(ctx));
    }

    ret = crosstalk_re_repl_native_cmd(ctx, argc, args);
    if (ret != LDG_ERR_NOT_FOUND) { return ret; }

    ret = crosstalk_re_repl_snitch_line_send(ctx, ctx->raw);
    if (ret == LDG_ERR_AOK) { return LDG_ERR_AOK; }

    if (crosstalk_re_repl_emit(ctx, "no such command; snitch err: %u32", ret) != LDG_ERR_AOK) { return LDG_ERR_IO_WR; }

    return LDG_ERR_NOT_FOUND;
}

uint32_t crosstalk_re_repl_line_run(crosstalk_re_repl_ctx_t *ctx, const char *text)
{
    char *args[CROSSTALK_RE_REPL_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t argc = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->ok != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = ldg_str_rbrcpy(ctx->req, text, (uint64_t)sizeof(ctx->req));
    if (ret == LDG_ERR_AOK) { ret = ldg_str_rbrcpy(ctx->raw, text, (uint64_t)sizeof(ctx->raw)); }

    if (ret == LDG_ERR_AOK) { ret = repl_args_get(ctx, args, &argc); }

    if (ret == LDG_ERR_AOK && argc == 0) { return LDG_ERR_AOK; }

    if (ret == LDG_ERR_AOK) { ret = repl_dispatch(ctx, argc, args); }

    if (ret == LDG_ERR_AOK) { return LDG_ERR_AOK; }

    if (crosstalk_re_repl_emit(ctx, "err: %u32", ret) != LDG_ERR_AOK) { return LDG_ERR_IO_WR; }

    return ret;
}

static uint32_t repl_alive_ck(crosstalk_re_repl_ctx_t *ctx)
{
    uint64_t now = 0;
    uint32_t ret = 0;

    ret = ldg_time_monotonic_ns_get(&now);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (now - ctx->alive_ck_ns < (uint64_t)REPL_ALIVE_CK_NS) { return LDG_ERR_AOK; }

    ctx->alive_ck_ns = now;

    if (ldg_sys_proc_alive_is(&ctx->proc) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = crosstalk_re_repl_emit(ctx, "process exited; pid: %u32", ctx->proc.pid);
    if (ret != LDG_ERR_AOK) { return ret; }

    return crosstalk_re_repl_detach(ctx);
}

uint32_t crosstalk_re_repl_tick(crosstalk_re_repl_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE || ctx->attached != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = repl_alive_ck(ctx);
    if (ret != LDG_ERR_AOK || ctx->attached != LDG_TRUTH_TRUE) { return ret; }

    if (ctx->searching == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_re_repl_native_search_tick(ctx);
        if (ret != LDG_ERR_AOK && crosstalk_re_repl_emit(ctx, "err: %u32", ret) != LDG_ERR_AOK) { return LDG_ERR_IO_WR; }
    }

    ret = crosstalk_re_repl_thread_tick(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_re_repl_snitch_tick(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_re_repl_scan_tick(ctx);

    return ret == LDG_ERR_NOT_INIT ? ret : LDG_ERR_AOK;
}
