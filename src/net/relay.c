#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/net/sock.h>
#include <dangling/net/curl.h>

#include <dangling/logging/logging.h>

#include "core/log.h"
#include "net/relay.h"

static uint32_t relay_word_rd(const uint8_t *src)
{
    return (uint32_t)src[0] | ((uint32_t)src[1] << 8) | ((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 24);
}

static void relay_word_wr(uint8_t *dst, uint32_t val)
{
    dst[0] = (uint8_t)(val & 0xFF);
    dst[1] = (uint8_t)((val >> 8) & 0xFF);
    dst[2] = (uint8_t)((val >> 16) & 0xFF);
    dst[3] = (uint8_t)((val >> 24) & 0xFF);
}

static uint16_t relay_half_rd(const uint8_t *src)
{
    return (uint16_t)((uint16_t)src[0] | ((uint16_t)src[1] << 8));
}

static uint32_t relay_payload_max_get(const crosstalk_net_relay_ctx_t *ctx)
{
    if (ctx->payload_cap <= (uint32_t)CROSSTALK_NET_RELAY_HDR_BYTES) { return (uint32_t)CROSSTALK_NET_RELAY_LEN_NONE; }

    return ctx->payload_cap - (uint32_t)CROSSTALK_NET_RELAY_HDR_BYTES;
}

static void relay_reply_set(crosstalk_net_relay_ctx_t *ctx, uint32_t op, uint32_t status, const uint8_t *payload, uint32_t len)
{
    uint32_t room = relay_payload_max_get(ctx);

    if (len > room) { len = room; }

    relay_word_wr(&ctx->reply[CROSSTALK_NET_RELAY_OP_AT], op);
    relay_word_wr(&ctx->reply[CROSSTALK_NET_RELAY_ID_AT], status);
    relay_word_wr(&ctx->reply[CROSSTALK_NET_RELAY_LEN_AT], len);

    if (len != CROSSTALK_NET_RELAY_LEN_NONE && payload != 0x0) { memcpy(&ctx->reply[CROSSTALK_NET_RELAY_HDR_BYTES], payload, (uint64_t)len); }

    ctx->reply_len = (uint32_t)CROSSTALK_NET_RELAY_HDR_BYTES + len;
    ctx->reply_held = LDG_TRUTH_TRUE;
}

static void relay_reply_bare_set(crosstalk_net_relay_ctx_t *ctx, uint32_t op, uint32_t status)
{
    relay_reply_set(ctx, op, status, 0x0, (uint32_t)CROSSTALK_NET_RELAY_LEN_NONE);
}

static uint32_t relay_conn_status_get(uint32_t err)
{
    if (err == LDG_ERR_NET_DNS || err == LDG_ERR_NET_REFUSED || err == LDG_ERR_NET_UNREACHABLE || err == LDG_ERR_NET_CONN || err == LDG_ERR_NET_TIMEOUT) { return (uint32_t)CROSSTALK_NET_RELAY_ST_CONN; }

    return (uint32_t)CROSSTALK_NET_RELAY_ST_TRANSPORT;
}

static uint32_t relay_stream_free_find(const crosstalk_net_relay_ctx_t *ctx)
{
    for (uint32_t idx = 0; idx < (uint32_t)CROSSTALK_NET_RELAY_STREAM_CAP; idx++) { if (ctx->stream[idx].used != LDG_TRUTH_TRUE) { return idx; } }

    return UINT32_MAX;
}

static uint8_t relay_stream_ready_is(const crosstalk_net_relay_ctx_t *ctx, uint32_t id)
{
    if (id >= (uint32_t)CROSSTALK_NET_RELAY_STREAM_CAP) { return LDG_TRUTH_FALSE; }

    return ctx->stream[id].used == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static void relay_stream_drop(crosstalk_net_relay_ctx_t *ctx, uint32_t idx)
{
    if (ctx->stream[idx].used != LDG_TRUTH_TRUE) { return; }

    if (ldg_net_sock_close(&ctx->stream[idx].sock, (uint64_t)CROSSTALK_NET_RELAY_CLOSE_TIMEOUT) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "relay: close failed; id: %u32", idx); }

    memset(&ctx->stream[idx], 0x0, sizeof(ctx->stream[idx]));
}

static void relay_open_service(crosstalk_net_relay_ctx_t *ctx, const uint8_t *payload, uint32_t len)
{
    char host[CROSSTALK_NET_RELAY_HOST_CAP] = LDG_ARR_ZERO_INIT;
    uint8_t id_buff[CROSSTALK_NET_RELAY_WORD_BYTES] = LDG_ARR_ZERO_INIT;
    uint32_t name_len = 0;
    uint32_t slot = 0;
    uint16_t port = 0;
    uint32_t ret = 0;

    if (len <= (uint32_t)CROSSTALK_NET_RELAY_PORT_BYTES)
    {
        relay_reply_bare_set(ctx, (uint32_t)CROSSTALK_NET_RELAY_OP_OPEN, (uint32_t)CROSSTALK_NET_RELAY_ST_BAD_REQ);
        return;
    }

    port = relay_half_rd(payload);
    name_len = len - (uint32_t)CROSSTALK_NET_RELAY_PORT_BYTES;

    if (name_len >= (uint32_t)sizeof(host))
    {
        relay_reply_bare_set(ctx, (uint32_t)CROSSTALK_NET_RELAY_OP_OPEN, (uint32_t)CROSSTALK_NET_RELAY_ST_BAD_REQ);
        return;
    }

    memcpy(host, &payload[CROSSTALK_NET_RELAY_PORT_BYTES], (uint64_t)name_len);
    host[name_len] = LDG_STR_TERM;

    slot = relay_stream_free_find(ctx);

    if (slot == UINT32_MAX)
    {
        relay_reply_bare_set(ctx, (uint32_t)CROSSTALK_NET_RELAY_OP_OPEN, (uint32_t)CROSSTALK_NET_RELAY_ST_NO_STREAM);
        return;
    }

    ret = ldg_net_sock_open(host, port, (uint64_t)CROSSTALK_NET_RELAY_OPEN_TIMEOUT, &ctx->stream[slot].sock);

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "relay: open failed; host: %.180s; port: %u32; err: %u32", host, (uint32_t)port, ret);
        memset(&ctx->stream[slot], 0x0, sizeof(ctx->stream[slot]));
        relay_reply_bare_set(ctx, (uint32_t)CROSSTALK_NET_RELAY_OP_OPEN, relay_conn_status_get(ret));
        return;
    }

    ctx->stream[slot].used = LDG_TRUTH_TRUE;
    relay_word_wr(id_buff, slot);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "relay: stream open; id: %u32; host: %.180s; port: %u32", slot, host, (uint32_t)port);
    relay_reply_set(ctx, (uint32_t)CROSSTALK_NET_RELAY_OP_OPEN, (uint32_t)CROSSTALK_NET_RELAY_ST_OK, id_buff, (uint32_t)sizeof(id_buff));
}

static void relay_send_service(crosstalk_net_relay_ctx_t *ctx, uint32_t id, const uint8_t *payload, uint32_t len)
{
    uint64_t bytes_wr = 0;
    uint32_t ret = 0;

    if (relay_stream_ready_is(ctx, id) != LDG_TRUTH_TRUE)
    {
        relay_reply_bare_set(ctx, (uint32_t)CROSSTALK_NET_RELAY_OP_SEND, (uint32_t)CROSSTALK_NET_RELAY_ST_CLOSED);
        return;
    }

    ret = ldg_net_sock_wr(&ctx->stream[id].sock, payload, (uint64_t)len, (uint64_t)CROSSTALK_NET_RELAY_SEND_TIMEOUT, &bytes_wr);

    if (ret == LDG_ERR_CLOSED || ret == LDG_ERR_NET_RST)
    {
        relay_stream_drop(ctx, id);
        relay_reply_bare_set(ctx, (uint32_t)CROSSTALK_NET_RELAY_OP_SEND, (uint32_t)CROSSTALK_NET_RELAY_ST_CLOSED);
        return;
    }

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "relay: send failed; id: %u32; err: %u32", id, ret);
        relay_reply_bare_set(ctx, (uint32_t)CROSSTALK_NET_RELAY_OP_SEND, (uint32_t)CROSSTALK_NET_RELAY_ST_TRANSPORT);
        return;
    }

    relay_reply_bare_set(ctx, (uint32_t)CROSSTALK_NET_RELAY_OP_SEND, (uint32_t)CROSSTALK_NET_RELAY_ST_OK);
}

static void relay_poll_service(crosstalk_net_relay_ctx_t *ctx, uint32_t id)
{
    uint8_t buff[CROSSTALK_NET_RELAY_PAYLOAD_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t bytes_rd = 0;
    uint32_t room = 0;
    uint32_t ret = 0;

    if (relay_stream_ready_is(ctx, id) != LDG_TRUTH_TRUE)
    {
        relay_reply_bare_set(ctx, (uint32_t)CROSSTALK_NET_RELAY_OP_POLL, (uint32_t)CROSSTALK_NET_RELAY_ST_CLOSED);
        return;
    }

    room = relay_payload_max_get(ctx);

    if (room > (uint32_t)sizeof(buff)) { room = (uint32_t)sizeof(buff); }

    ret = ldg_net_sock_rd(&ctx->stream[id].sock, buff, (uint64_t)room, (uint64_t)CROSSTALK_NET_RELAY_POLL_TIMEOUT, &bytes_rd);

    if (ret == LDG_ERR_CLOSED || ret == LDG_ERR_NET_RST)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "relay: poll closed; id: %u32; err: %u32; got: %u64; wr total: %u64; rd total: %u64; serviced: %u64", id, ret, bytes_rd, ctx->stream[id].sock.wr_total, ctx->stream[id].sock.rd_total, ctx->serviced);
        relay_stream_drop(ctx, id);
        relay_reply_bare_set(ctx, (uint32_t)CROSSTALK_NET_RELAY_OP_POLL, (uint32_t)CROSSTALK_NET_RELAY_ST_CLOSED);
        return;
    }

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "relay: poll failed; id: %u32; err: %u32", id, ret);
        relay_reply_bare_set(ctx, (uint32_t)CROSSTALK_NET_RELAY_OP_POLL, (uint32_t)CROSSTALK_NET_RELAY_ST_TRANSPORT);
        return;
    }

    relay_reply_set(ctx, (uint32_t)CROSSTALK_NET_RELAY_OP_POLL, (uint32_t)CROSSTALK_NET_RELAY_ST_OK, buff, (uint32_t)bytes_rd);
}

static void relay_close_service(crosstalk_net_relay_ctx_t *ctx, uint32_t id)
{
    if (relay_stream_ready_is(ctx, id) != LDG_TRUTH_TRUE)
    {
        relay_reply_bare_set(ctx, (uint32_t)CROSSTALK_NET_RELAY_OP_CLOSE, (uint32_t)CROSSTALK_NET_RELAY_ST_CLOSED);
        return;
    }

    relay_stream_drop(ctx, id);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "relay: stream closed; id: %u32", id);
    relay_reply_bare_set(ctx, (uint32_t)CROSSTALK_NET_RELAY_OP_CLOSE, (uint32_t)CROSSTALK_NET_RELAY_ST_OK);
}

static void relay_post_service(crosstalk_net_relay_ctx_t *ctx, const uint8_t *payload, uint32_t len)
{
    char url[CROSSTALK_NET_RELAY_URL_CAP] = LDG_ARR_ZERO_INIT;
    ldg_net_curl_easy_ctx_t curl = LDG_STRUCT_ZERO_INIT;
    ldg_net_curl_resp_t resp = LDG_STRUCT_ZERO_INIT;
    uint8_t len_buff[CROSSTALK_NET_RELAY_WORD_BYTES] = LDG_ARR_ZERO_INIT;
    void *hdrs = 0x0;
    const uint8_t *body = 0x0;
    uint32_t body_len = 0;
    uint32_t url_len = 0;
    uint32_t ret = 0;

    ctx->body_len = (uint32_t)CROSSTALK_NET_RELAY_LEN_NONE;

    while (url_len < len && payload[url_len] != LDG_STR_TERM) { url_len++; }

    if (url_len == CROSSTALK_NET_RELAY_LEN_NONE || url_len >= len || url_len >= (uint32_t)sizeof(url))
    {
        relay_reply_bare_set(ctx, (uint32_t)CROSSTALK_NET_RELAY_OP_POST, (uint32_t)CROSSTALK_NET_RELAY_ST_BAD_REQ);
        return;
    }

    memcpy(url, payload, (uint64_t)url_len);
    url[url_len] = LDG_STR_TERM;
    body = &payload[url_len + (uint32_t)CROSSTALK_NET_RELAY_NUL_BYTES];
    body_len = len - url_len - (uint32_t)CROSSTALK_NET_RELAY_NUL_BYTES;

    ret = ldg_net_curl_easy_ctx_create(&curl);

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "relay: curl ctx failed; err: %u32", ret);
        relay_reply_bare_set(ctx, (uint32_t)CROSSTALK_NET_RELAY_OP_POST, (uint32_t)CROSSTALK_NET_RELAY_ST_TRANSPORT);
        return;
    }

    if (ldg_net_curl_hdr_append(&hdrs, CROSSTALK_NET_RELAY_CONTENT_HDR) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "relay content header failed"); }

    ldg_net_curl_resp_init(&resp);
    ret = ldg_net_curl_easy_raw_post(&curl, url, body, (uint64_t)body_len, hdrs, &resp);

    if (resp.data != 0x0 && resp.size != CROSSTALK_NET_RELAY_LEN_NONE)
    {
        ctx->body_len = resp.size > (uint64_t)CROSSTALK_NET_RELAY_BODY_CAP ? (uint32_t)CROSSTALK_NET_RELAY_BODY_CAP : (uint32_t)resp.size;
        memcpy(ctx->body, resp.data, (uint64_t)ctx->body_len);
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "relay: post done; payload len: %u32; url len: %u32; body len: %u32; url: %.180s; err: %u32; status: %u32; bytes: %u32", len, url_len, body_len, url, ret, resp.status, ctx->body_len);

    ldg_net_curl_resp_free(&resp);
    ldg_net_curl_hdr_destroy(&hdrs);
    ldg_net_curl_easy_ctx_destroy(&curl);

    if (ctx->body_len == CROSSTALK_NET_RELAY_LEN_NONE && ret != LDG_ERR_AOK)
    {
        relay_reply_bare_set(ctx, (uint32_t)CROSSTALK_NET_RELAY_OP_POST, (uint32_t)CROSSTALK_NET_RELAY_ST_TRANSPORT);
        return;
    }

    relay_word_wr(len_buff, ctx->body_len);
    relay_reply_set(ctx, (uint32_t)CROSSTALK_NET_RELAY_OP_POST, (uint32_t)CROSSTALK_NET_RELAY_ST_OK, len_buff, (uint32_t)sizeof(len_buff));
}

static void relay_body_service(crosstalk_net_relay_ctx_t *ctx, uint32_t off)
{
    uint32_t room = 0;
    uint32_t span = 0;

    if (off > ctx->body_len)
    {
        relay_reply_bare_set(ctx, (uint32_t)CROSSTALK_NET_RELAY_OP_BODY, (uint32_t)CROSSTALK_NET_RELAY_ST_BAD_REQ);
        return;
    }

    room = relay_payload_max_get(ctx);
    span = ctx->body_len - off;

    if (span > room) { span = room; }

    relay_reply_set(ctx, (uint32_t)CROSSTALK_NET_RELAY_OP_BODY, (uint32_t)CROSSTALK_NET_RELAY_ST_OK, &ctx->body[off], span);
}

static void relay_req_service(crosstalk_net_relay_ctx_t *ctx, const uint8_t *req, uint32_t got, uint8_t blk_ok)
{
    const uint8_t *payload = 0x0;
    uint32_t op = 0;
    uint32_t id = 0;
    uint32_t len = 0;

    if (got < (uint32_t)CROSSTALK_NET_RELAY_HDR_BYTES)
    {
        relay_reply_bare_set(ctx, (uint32_t)CROSSTALK_NET_RELAY_OP_NONE, (uint32_t)CROSSTALK_NET_RELAY_ST_BAD_REQ);
        return;
    }

    op = relay_word_rd(&req[CROSSTALK_NET_RELAY_OP_AT]);
    id = relay_word_rd(&req[CROSSTALK_NET_RELAY_ID_AT]);
    len = relay_word_rd(&req[CROSSTALK_NET_RELAY_LEN_AT]);
    payload = &req[CROSSTALK_NET_RELAY_HDR_BYTES];

    if (op == (uint32_t)CROSSTALK_NET_RELAY_OP_POST && blk_ok != LDG_TRUTH_TRUE)
    {
        memcpy(ctx->post_req, req, (uint64_t)got);
        ctx->post_req_len = got;
        ctx->post_held = LDG_TRUTH_TRUE;

        return;
    }

    if (len > got - (uint32_t)CROSSTALK_NET_RELAY_HDR_BYTES)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "relay: request length mismatch; op: %u32; id: %u32; declared: %u32; got: %u32", op, id, len, got);
        relay_reply_bare_set(ctx, op, (uint32_t)CROSSTALK_NET_RELAY_ST_BAD_REQ);
        return;
    }

    if (op == (uint32_t)CROSSTALK_NET_RELAY_OP_OPEN) { relay_open_service(ctx, payload, len); }
    else if (op == (uint32_t)CROSSTALK_NET_RELAY_OP_SEND) { relay_send_service(ctx, id, payload, len); }
    else if (op == (uint32_t)CROSSTALK_NET_RELAY_OP_POLL) { relay_poll_service(ctx, id); }
    else if (op == (uint32_t)CROSSTALK_NET_RELAY_OP_CLOSE) { relay_close_service(ctx, id); }
    else if (op == (uint32_t)CROSSTALK_NET_RELAY_OP_POST) { relay_post_service(ctx, payload, len); }
    else if (op == (uint32_t)CROSSTALK_NET_RELAY_OP_BODY) { relay_body_service(ctx, id); }
    else { relay_reply_bare_set(ctx, op, (uint32_t)CROSSTALK_NET_RELAY_ST_BAD_REQ); }
}

uint32_t crosstalk_net_relay_init(crosstalk_net_relay_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(crosstalk_net_relay_ctx_t));

    ret = ldg_net_sock_init();

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "relay: sock init failed; err: %u32", ret);
        return ret;
    }

    ret = ldg_mut_init(&ctx->mut, LDG_TRUTH_FALSE);

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "relay: mutex init failed; err: %u32", ret);
        ldg_net_sock_shutdown();

        return ret;
    }

    ctx->mut_ok = LDG_TRUTH_TRUE;
    ctx->payload_cap = (uint32_t)CROSSTALK_NET_RELAY_PAYLOAD_CAP;
    ctx->ok = LDG_TRUTH_TRUE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "relay: ready; streams: %u32; payload cap: %u32", (uint32_t)CROSSTALK_NET_RELAY_STREAM_CAP, ctx->payload_cap);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_relay_shutdown(crosstalk_net_relay_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    for (uint32_t idx = 0; idx < (uint32_t)CROSSTALK_NET_RELAY_STREAM_CAP; idx++) { relay_stream_drop(ctx, idx); }

    ctx->ok = LDG_TRUTH_FALSE;
    ctx->reply_held = LDG_TRUTH_FALSE;
    ctx->post_held = LDG_TRUTH_FALSE;
    ctx->body_len = (uint32_t)CROSSTALK_NET_RELAY_LEN_NONE;

    if (ctx->mut_ok == LDG_TRUTH_TRUE)
    {
        if (ldg_mut_destroy(&ctx->mut) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "relay mutex destroy failed"); }

        ctx->mut_ok = LDG_TRUTH_FALSE;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "relay: shutdown; serviced: %u64", ctx->serviced);

    return ldg_net_sock_shutdown();
}

uint8_t crosstalk_net_relay_live_is(const crosstalk_net_relay_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->ok == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t relay_tick_locked(crosstalk_net_relay_ctx_t *ctx, const crosstalk_hdl_abi_t *abi, uint8_t blk_ok)
{
    uint32_t ret = 0;

    if (ctx->payload_cap == (uint32_t)CROSSTALK_NET_RELAY_PAYLOAD_CAP && abi->relay_payload_cap_get != 0x0)
    {
        uint32_t cap = abi->relay_payload_cap_get();

        if (cap != CROSSTALK_NET_RELAY_LEN_NONE && cap <= (uint32_t)CROSSTALK_NET_RELAY_PAYLOAD_CAP) { ctx->payload_cap = cap; }
    }

    if (ctx->post_held == LDG_TRUTH_TRUE && blk_ok == LDG_TRUTH_TRUE && ctx->reply_held != LDG_TRUTH_TRUE)
    {
        ctx->post_held = LDG_TRUTH_FALSE;

        relay_req_service(ctx, ctx->post_req, ctx->post_req_len, blk_ok);
    }

    for (uint32_t turn = 0; turn < (uint32_t)CROSSTALK_NET_RELAY_SERVICE_CAP; turn++)
    {
        uint32_t got = 0;

        if (ctx->reply_held == LDG_TRUTH_TRUE)
        {
            ret = abi->relay_reply_push(ctx->reply, ctx->reply_len);

            if (ret == LDG_ERR_BUSY) { return LDG_ERR_AOK; }

            if (ret != LDG_ERR_AOK)
            {
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "relay: reply push failed; err: %u32; len: %u32", ret, ctx->reply_len);
                return ret;
            }

            ctx->reply_held = LDG_TRUTH_FALSE;
            ctx->serviced++;
        }

        ret = abi->relay_req_take(ctx->req, ctx->payload_cap, &got);

        if (ret == LDG_ERR_BOUNDS)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "relay: request over cap; cap: %u32", ctx->payload_cap);
            return ret;
        }

        if (ret != LDG_ERR_AOK) { return ret; }

        if (got == CROSSTALK_NET_RELAY_LEN_NONE) { return LDG_ERR_AOK; }

        relay_req_service(ctx, ctx->req, got, blk_ok);

        if (ctx->post_held == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_relay_tick(crosstalk_net_relay_ctx_t *ctx, const crosstalk_hdl_abi_t *abi, uint8_t blk_ok)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || abi == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (ctx->mut_ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (abi->relay_req_take == 0x0 || abi->relay_reply_push == 0x0) { return LDG_ERR_UNSUPPORTED; }

    if (blk_ok == LDG_TRUTH_TRUE)
    {
        ret = ldg_mut_lock(&ctx->mut);
        if (ret != LDG_ERR_AOK) { return ret; }
    }
    else { if (ldg_mut_trylock(&ctx->mut) != LDG_ERR_AOK) { return LDG_ERR_AOK; } }

    ret = relay_tick_locked(ctx, abi, blk_ok);

    if (ldg_mut_unlock(&ctx->mut) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "relay mutex unlock failed"); }

    return ret;
}
