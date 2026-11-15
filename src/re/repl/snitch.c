#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/core/types.h>
#include <dangling/io/ipc.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/secure.h>
#include <dangling/str/fmt.h>
#include <dangling/str/str.h>

#include "re/repl/repl.h"
#include "re/repl/snitch.h"

#define SNITCH_STEP 1
#define SNITCH_NAME_FMT "libresteam-repl-%u32"
#define SNITCH_HELLO_TIMEOUT_MS 500
#define SNITCH_HELLO_CAP 128
#define SNITCH_HELLO_MAGIC "LSREPL"
#define SNITCH_HELLO_MAGIC_LEN 6
#define SNITCH_HELLO_MAGIC_SIZE 8
#define SNITCH_PROTO_VERSION 1
#define SNITCH_RESP_TEXT_CAP 32768
#define SNITCH_RESP_HDR_SIZE 16
#define SNITCH_RESP_MORE UINT32_MAX
#define SNITCH_DRAIN_CAP 8
#define SNITCH_PUDDING_SIZE 3

typedef struct snitch_hello
{
    char magic[SNITCH_HELLO_MAGIC_SIZE];
    uint32_t version;
    uint32_t size;
    uint64_t pid;
    uint64_t thread;
    uint32_t req_cap;
    uint32_t resp_cap;
} snitch_hello_t;

typedef struct snitch_resp
{
    uint64_t id;
    uint32_t res;
    uint32_t len;
    char text[SNITCH_RESP_TEXT_CAP];
} snitch_resp_t;

typedef struct snitch_ctx
{
    ldg_io_ipc_t pipe;
    snitch_resp_t resp;
    snitch_hello_t hello;
    uint8_t hello_buff[SNITCH_HELLO_CAP];
    uint32_t req_cap;
    uint32_t discard_cunt;
    uint8_t connected;
    uint8_t pudding[SNITCH_PUDDING_SIZE];
} snitch_ctx_t;

uint32_t crosstalk_re_repl_snitch_init(crosstalk_re_repl_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->snitch = 0x0;

    ret = ldg_mem_alloc((uint64_t)sizeof(snitch_ctx_t), &ctx->snitch);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_secure_zero(ctx->snitch, (uint64_t)sizeof(snitch_ctx_t));
}

static uint32_t snitch_close(crosstalk_re_repl_ctx_t *ctx, snitch_ctx_t *snitch)
{
    uint32_t ret = LDG_ERR_AOK;

    if (snitch->connected == LDG_TRUTH_TRUE) { ret = ldg_io_ipc_close(&snitch->pipe); }

    snitch->connected = LDG_TRUTH_FALSE;
    snitch->discard_cunt = 0;
    ctx->snitch_wait = LDG_TRUTH_FALSE;

    return ret;
}

uint32_t crosstalk_re_repl_snitch_rst(crosstalk_re_repl_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0 || ctx->snitch == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return snitch_close(ctx, (snitch_ctx_t *)ctx->snitch);
}

uint32_t crosstalk_re_repl_snitch_shutdown(crosstalk_re_repl_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->snitch == 0x0) { return LDG_ERR_AOK; }

    ret = snitch_close(ctx, (snitch_ctx_t *)ctx->snitch);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_mem_dealloc(ctx->snitch);
    if (ret == LDG_ERR_AOK) { ctx->snitch = 0x0; }

    return ret;
}

static uint32_t snitch_hello_validate(snitch_ctx_t *snitch, uint64_t got)
{
    uint32_t at = 0;
    uint32_t ret = 0;

    if (got < (uint64_t)sizeof(snitch->hello) || got > (uint64_t)sizeof(snitch->hello_buff)) { return LDG_ERR_INVALID; }

    ret = ldg_mem_secure_cpy(&snitch->hello, snitch->hello_buff, (uint64_t)sizeof(snitch->hello));
    if (ret != LDG_ERR_AOK) { return ret; }

    for (at = 0; at < (uint32_t)SNITCH_HELLO_MAGIC_LEN; at = at + (uint32_t)SNITCH_STEP) { if (snitch->hello.magic[at] != SNITCH_HELLO_MAGIC[at]) { return LDG_ERR_INVALID; } }

    for (at = (uint32_t)SNITCH_HELLO_MAGIC_LEN; at < (uint32_t)SNITCH_HELLO_MAGIC_SIZE; at = at + (uint32_t)SNITCH_STEP) { if (snitch->hello.magic[at] != LDG_STR_TERM) { return LDG_ERR_INVALID; } }

    if (snitch->hello.version != (uint32_t)SNITCH_PROTO_VERSION) { return LDG_ERR_UNSUPPORTED; }

    if ((uint64_t)snitch->hello.size != got) { return LDG_ERR_INVALID; }

    return LDG_ERR_AOK;
}

static uint32_t snitch_connect(crosstalk_re_repl_ctx_t *ctx, snitch_ctx_t *snitch)
{
    char name[LDG_IO_IPC_NAME_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t len = 0;
    uint64_t got = 0;
    uint32_t ret = 0;

    ret = ldg_str_fmt(name, (uint64_t)sizeof(name), &len, SNITCH_NAME_FMT, ctx->proc.pid);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_ipc_connect(name, &snitch->pipe);
    if (ret != LDG_ERR_AOK) { return ret; }

    snitch->connected = LDG_TRUTH_TRUE;

    ret = ldg_io_ipc_rd_timed(&snitch->pipe, (ldg_byte_t *)snitch->hello_buff, (uint64_t)sizeof(snitch->hello_buff), (uint64_t)SNITCH_HELLO_TIMEOUT_MS, &got);
    if (ret == LDG_ERR_AOK) { ret = snitch_hello_validate(snitch, got); }

    if (ret != LDG_ERR_AOK)
    {
        if (snitch_close(ctx, snitch) != LDG_ERR_AOK) { return LDG_ERR_IO_CLOSE; }

        return ret;
    }

    snitch->req_cap = snitch->hello.req_cap;

    return crosstalk_re_repl_emit(ctx, "snitch connected; pid: %u64; dispatch thread: %u64; request bytes: %u32", snitch->hello.pid, snitch->hello.thread, snitch->hello.req_cap);
}

uint32_t crosstalk_re_repl_snitch_line_send(crosstalk_re_repl_ctx_t *ctx, const char *line)
{
    snitch_ctx_t *snitch = 0x0;
    uint64_t len = 0;
    uint64_t sent = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || ctx->snitch == 0x0 || line == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->attached != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    snitch = (snitch_ctx_t *)ctx->snitch;

    ret = ldg_str_len_get(line, (uint64_t)CROSSTALK_RE_REPL_CMD_CAP, &len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (len == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (snitch->connected != LDG_TRUTH_TRUE)
    {
        ret = snitch_connect(ctx, snitch);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    if (len + LDG_STR_TERM_SIZE > (uint64_t)snitch->req_cap) { return LDG_ERR_STR_TRUNC; }

    ret = ldg_io_ipc_wr(&snitch->pipe, (const ldg_byte_t *)line, len + LDG_STR_TERM_SIZE, &sent);
    if (ret == LDG_ERR_AOK && sent != len + LDG_STR_TERM_SIZE) { ret = LDG_ERR_IO_WR; }

    if (ret != LDG_ERR_AOK)
    {
        if (snitch_close(ctx, snitch) != LDG_ERR_AOK) { return LDG_ERR_IO_CLOSE; }

        return ret;
    }

    ctx->snitch_wait = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_repl_snitch_cancel(crosstalk_re_repl_ctx_t *ctx)
{
    snitch_ctx_t *snitch = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0 || ctx->snitch == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->snitch_wait != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    snitch = (snitch_ctx_t *)ctx->snitch;
    snitch->discard_cunt = snitch->discard_cunt + (uint32_t)SNITCH_STEP;
    ctx->snitch_wait = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

static uint32_t snitch_resp_take(crosstalk_re_repl_ctx_t *ctx, snitch_ctx_t *snitch, uint64_t got)
{
    uint32_t ret = 0;

    if (got < (uint64_t)SNITCH_RESP_HDR_SIZE + LDG_STR_TERM_SIZE || snitch->resp.len >= (uint32_t)SNITCH_RESP_TEXT_CAP || got != (uint64_t)SNITCH_RESP_HDR_SIZE + (uint64_t)snitch->resp.len + LDG_STR_TERM_SIZE) { return LDG_ERR_INVALID; }

    if (snitch->discard_cunt != 0)
    {
        if (snitch->resp.res != SNITCH_RESP_MORE) { snitch->discard_cunt = snitch->discard_cunt - (uint32_t)SNITCH_STEP; }

        return LDG_ERR_AOK;
    }

    ret = crosstalk_re_repl_output_wr(ctx, snitch->resp.text, (uint64_t)snitch->resp.len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (snitch->resp.res == SNITCH_RESP_MORE) { return LDG_ERR_AOK; }

    ctx->snitch_wait = LDG_TRUTH_FALSE;

    if (snitch->resp.res == LDG_ERR_AOK) { return LDG_ERR_AOK; }

    return crosstalk_re_repl_emit(ctx, "err: %u32", snitch->resp.res);
}

uint32_t crosstalk_re_repl_snitch_tick(crosstalk_re_repl_ctx_t *ctx)
{
    snitch_ctx_t *snitch = 0x0;
    uint64_t got = 0;
    uint32_t step = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || ctx->snitch == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    snitch = (snitch_ctx_t *)ctx->snitch;

    if (snitch->connected != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    for (step = 0; step < (uint32_t)SNITCH_DRAIN_CAP; step = step + (uint32_t)SNITCH_STEP)
    {
        got = 0;

        ret = ldg_io_ipc_rd(&snitch->pipe, (ldg_byte_t *)&snitch->resp, (uint64_t)sizeof(snitch->resp), &got);
        if (ret == LDG_ERR_AOK && got == 0) { return LDG_ERR_AOK; }

        if (ret == LDG_ERR_AOK) { ret = snitch_resp_take(ctx, snitch, got); }

        if (ret == LDG_ERR_AOK) { continue; }

        if (snitch_close(ctx, snitch) != LDG_ERR_AOK) { return LDG_ERR_IO_CLOSE; }

        return crosstalk_re_repl_emit(ctx, "snitch link lost; err: %u32", ret);
    }

    return LDG_ERR_AOK;
}
