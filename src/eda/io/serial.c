#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "eda/io/serial.h"

static uint8_t serial_id_match_is(const ldg_io_serial_port_t *lhs, const ldg_io_serial_port_t *rhs)
{
    if (LDG_UNLIKELY(lhs == 0x0 || rhs == 0x0)) { return LDG_TRUTH_FALSE; }

    if (lhs->id[0] == LDG_STR_TERM || rhs->id[0] == LDG_STR_TERM) { return strncmp(lhs->path, rhs->path, (uint64_t)sizeof(lhs->path)) == 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    return strncmp(lhs->id, rhs->id, (uint64_t)LDG_IO_SERIAL_ID_MAX) == 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint8_t serial_id_listed_is(const ldg_io_serial_port_t *list, uint64_t cunt, const ldg_io_serial_port_t *want)
{
    uint64_t idx = 0;

    for (idx = 0; idx < cunt; idx++) { if (serial_id_match_is(&list[idx], want) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; } }

    return LDG_TRUTH_FALSE;
}

static uint32_t serial_event_push(crosstalk_eda_io_serial_ctx_t *ctx, const ldg_io_serial_port_t *port, uint32_t kind)
{
    crosstalk_eda_io_serial_event_t *ev = 0x0;
    uint32_t ret = 0;

    if (ctx->event_cunt >= (uint32_t)CROSSTALK_EDA_IO_SERIAL_EVENT_MAX) { return LDG_ERR_FULL; }

    ev = &ctx->event[ctx->event_cunt];
    ev->kind = kind;

    ret = ldg_str_rbrcpy(ev->path, port->path, (uint64_t)sizeof(ev->path));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    ret = ldg_str_rbrcpy(ev->id, port->id, (uint64_t)sizeof(ev->id));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    ctx->event_cunt = ctx->event_cunt + 0x1;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_serial_init(crosstalk_eda_io_serial_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ret = ldg_io_serial_conf_default_get(&ctx->conf);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->conf.baud = LDG_IO_SERIAL_BAUD_9600;

    ctx->link.handle = LDG_IO_SERIAL_HANDLE_NONE;
    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_serial_shutdown(crosstalk_eda_io_serial_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->attached == LDG_TRUTH_TRUE) { if (ldg_io_serial_close(&ctx->link) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "serial close failed"); } }

    memset(ctx, 0x0, sizeof(*ctx));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_serial_poll(crosstalk_eda_io_serial_ctx_t *ctx, double dt_secs)
{
    uint64_t cunt = 0;
    uint64_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ctx->poll_acc = ctx->poll_acc + dt_secs;
    if (ctx->poll_acc < CROSSTALK_EDA_IO_SERIAL_POLL_SECS) { return LDG_ERR_AOK; }

    ctx->poll_acc = 0.0;

    ret = ldg_io_serial_port_list(ctx->port, (uint64_t)LDG_IO_SERIAL_PORT_MAX, &cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->port_cunt = cunt;

    if (ctx->seeded != LDG_TRUTH_TRUE)
    {
        memcpy(ctx->prev, ctx->port, sizeof(ctx->prev));
        ctx->prev_cunt = ctx->port_cunt;
        ctx->seeded = LDG_TRUTH_TRUE;

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "serial: baseline; ports: %u64", ctx->port_cunt);
        return LDG_ERR_AOK;
    }

    for (idx = 0; idx < ctx->port_cunt; idx++)
    {
        if (serial_id_listed_is(ctx->prev, ctx->prev_cunt, &ctx->port[idx]) == LDG_TRUTH_TRUE) { continue; }

        if (serial_event_push(ctx, &ctx->port[idx], (uint32_t)CROSSTALK_EDA_IO_SERIAL_EVENT_ARRIVE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "serial event queue full"); }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "serial: arrive; path: %.63s; id: %.191s", ctx->port[idx].path, ctx->port[idx].id);
    }

    for (idx = 0; idx < ctx->prev_cunt; idx++)
    {
        if (serial_id_listed_is(ctx->port, ctx->port_cunt, &ctx->prev[idx]) == LDG_TRUTH_TRUE) { continue; }

        if (serial_event_push(ctx, &ctx->prev[idx], (uint32_t)CROSSTALK_EDA_IO_SERIAL_EVENT_DEPART) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "serial event queue full"); }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "serial: depart; path: %.63s; id: %.191s", ctx->prev[idx].path, ctx->prev[idx].id);

        if (ctx->attached == LDG_TRUTH_TRUE && ctx->remote != LDG_TRUTH_TRUE && (ctx->open_id[0] != LDG_STR_TERM ? strncmp(ctx->open_id, ctx->prev[idx].id, (uint64_t)LDG_IO_SERIAL_ID_MAX) : strncmp(ctx->open_path, ctx->prev[idx].path, (uint64_t)LDG_IO_SERIAL_PATH_MAX)) == 0)
        {
            ctx->busted = LDG_TRUTH_TRUE;
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "serial: attached port departed; path: %.63s", ctx->open_path);
        }
    }

    memcpy(ctx->prev, ctx->port, sizeof(ctx->prev));
    ctx->prev_cunt = ctx->port_cunt;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_serial_events_drain(crosstalk_eda_io_serial_ctx_t *ctx, crosstalk_eda_io_serial_event_t *out, uint32_t cap, uint32_t *out_cunt)
{
    uint32_t cunt = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0 || out_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    cunt = ctx->event_cunt < cap ? ctx->event_cunt : cap;

    for (idx = 0; idx < cunt; idx++) { out[idx] = ctx->event[idx]; }

    ctx->event_cunt = 0;
    *out_cunt = cunt;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_serial_ports_get(const crosstalk_eda_io_serial_ctx_t *ctx, const ldg_io_serial_port_t **out, uint64_t *out_cunt)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0 || out_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = ctx->port;
    *out_cunt = ctx->port_cunt;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_serial_attach(crosstalk_eda_io_serial_ctx_t *ctx, const char *path)
{
    uint64_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (ctx->attached == LDG_TRUTH_TRUE) { return CROSSTALK_EDA_IO_SERIAL_ERR_ATTACHED; }

    ret = ldg_io_serial_open(path, &ctx->conf, &ctx->link);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "serial: open failed; path: %.63s; err: %u32", path, ret);
        return ret;
    }

    ret = ldg_str_rbrcpy(ctx->open_path, path, (uint64_t)sizeof(ctx->open_path));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    ctx->open_id[0] = LDG_STR_TERM;

    for (idx = 0; idx < ctx->port_cunt; idx++)
    {
        if (strncmp(ctx->port[idx].path, path, (uint64_t)LDG_IO_SERIAL_PATH_MAX) != 0) { continue; }

        ret = ldg_str_rbrcpy(ctx->open_id, ctx->port[idx].id, (uint64_t)sizeof(ctx->open_id));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

        break;
    }

    ctx->attached = LDG_TRUTH_TRUE;
    ctx->busted = LDG_TRUTH_FALSE;
    ctx->tx_len = 0;
    ctx->rx_total = 0;
    ctx->tx_total = 0;
    ctx->tx_dropped = 0;

    if (ldg_io_serial_dtr_set(&ctx->link, LDG_TRUTH_TRUE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "serial dtr assert failed"); }

    if (ldg_io_serial_rts_set(&ctx->link, LDG_TRUTH_TRUE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "serial rts assert failed"); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "serial: attached; path: %.63s; id: %.191s; cts: %u32; dsr: %u32", ctx->open_path, ctx->open_id, ldg_io_serial_cts_is(&ctx->link), ldg_io_serial_dsr_is(&ctx->link));
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "serial: conf; baud: %u32; data: %u32; parity: %u32; stop: %u32; flow: %u32", (uint32_t)ctx->conf.baud, (uint32_t)ctx->conf.data, (uint32_t)ctx->conf.parity, (uint32_t)ctx->conf.stop, (uint32_t)ctx->conf.flow);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_serial_detach(crosstalk_eda_io_serial_ctx_t *ctx)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->attached != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_IO_SERIAL_ERR_NOT_ATTACHED; }

    if (ctx->remote != LDG_TRUTH_TRUE)
    {
        ret = ldg_io_serial_close(&ctx->link);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "serial close failed"); }
    }

    ctx->link.handle = LDG_IO_SERIAL_HANDLE_NONE;
    ctx->attached = LDG_TRUTH_FALSE;
    ctx->busted = LDG_TRUTH_FALSE;
    ctx->remote = LDG_TRUTH_FALSE;
    ctx->tx_len = 0;
    ctx->rx_len = 0;
    ctx->open_path[0] = LDG_STR_TERM;
    ctx->open_id[0] = LDG_STR_TERM;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "serial: detached");

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_serial_remote_attach(crosstalk_eda_io_serial_ctx_t *ctx, const char *id)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || id == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (ctx->attached == LDG_TRUTH_TRUE) { return CROSSTALK_EDA_IO_SERIAL_ERR_ATTACHED; }

    ret = ldg_str_rbrcpy(ctx->open_path, CROSSTALK_EDA_IO_SERIAL_REMOTE_PATH, (uint64_t)sizeof(ctx->open_path));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    ret = ldg_str_rbrcpy(ctx->open_id, id, (uint64_t)sizeof(ctx->open_id));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    ctx->link.handle = LDG_IO_SERIAL_HANDLE_NONE;
    ctx->attached = LDG_TRUTH_TRUE;
    ctx->remote = LDG_TRUTH_TRUE;
    ctx->busted = LDG_TRUTH_FALSE;
    ctx->tx_len = 0;
    ctx->rx_len = 0;
    ctx->rx_total = 0;
    ctx->tx_total = 0;
    ctx->tx_dropped = 0;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "serial: remote attached; id: %.191s", ctx->open_id);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_serial_remote_rx_put(crosstalk_eda_io_serial_ctx_t *ctx, const uint8_t *bytes, uint32_t len)
{
    if (LDG_UNLIKELY(ctx == 0x0 || bytes == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->attached != LDG_TRUTH_TRUE || ctx->remote != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_IO_SERIAL_ERR_NOT_ATTACHED; }

    if (ctx->rx_len + len > (uint32_t)CROSSTALK_EDA_IO_SERIAL_RX_CAP)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "serial: remote rx overflow; dropped: %u32", len);
        return LDG_ERR_FULL;
    }

    memcpy(&ctx->rx[ctx->rx_len], bytes, (uint64_t)len);
    ctx->rx_len = ctx->rx_len + len;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_serial_remote_tx_take(crosstalk_eda_io_serial_ctx_t *ctx, uint8_t *out, uint32_t cap, uint32_t *out_len)
{
    uint32_t take = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (ctx->attached != LDG_TRUTH_TRUE || ctx->remote != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_IO_SERIAL_ERR_NOT_ATTACHED; }

    if (ctx->tx_len == 0) { return LDG_ERR_AOK; }

    take = ctx->tx_len < cap ? ctx->tx_len : cap;
    memcpy(out, ctx->tx, (uint64_t)take);

    if (take < ctx->tx_len) { memmove(ctx->tx, &ctx->tx[take], (uint64_t)ctx->tx_len - (uint64_t)take); }

    ctx->tx_len = ctx->tx_len - take;
    ctx->tx_total = ctx->tx_total + (uint64_t)take;
    *out_len = take;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_serial_pump(crosstalk_eda_io_serial_ctx_t *ctx, crosstalk_eda_io_serial_sink_cb_t cb, void *user)
{
    uint64_t got = 0;
    uint64_t sent = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->attached != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (ctx->busted == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (ctx->remote == LDG_TRUTH_TRUE)
    {
        got = (uint64_t)ctx->rx_len;
        ctx->rx_len = 0;

        if (got != 0)
        {
            ctx->rx_total = ctx->rx_total + got;

            if (cb != 0x0)
            {
                ret = cb(user, ctx->rx, (uint32_t)got);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
        }

        return LDG_ERR_AOK;
    }

    ret = ldg_io_serial_rd(&ctx->link, ctx->rx, (uint64_t)CROSSTALK_EDA_IO_SERIAL_RX_CAP, &got);
    if (ret == LDG_ERR_WOULD_BLOCK || ret == LDG_ERR_AGAIN) { got = 0; }
    else if (ret != LDG_ERR_AOK)
    {
        ctx->busted = LDG_TRUTH_TRUE;
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "serial: read failed; path: %.63s; err: %u32", ctx->open_path, ret);
        return CROSSTALK_EDA_IO_SERIAL_ERR_BUSTED;
    }

    if (got != 0)
    {
        ctx->rx_total = ctx->rx_total + got;

        if (cb != 0x0)
        {
            ret = cb(user, ctx->rx, (uint32_t)got);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }

    if (ctx->tx_len == 0) { return LDG_ERR_AOK; }

    ret = ldg_io_serial_wr(&ctx->link, ctx->tx, (uint64_t)ctx->tx_len, &sent);
    if (ret == LDG_ERR_WOULD_BLOCK || ret == LDG_ERR_AGAIN) { return LDG_ERR_AOK; }

    if (ret != LDG_ERR_AOK)
    {
        ctx->busted = LDG_TRUTH_TRUE;
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "serial: write failed; path: %.63s; err: %u32", ctx->open_path, ret);
        return CROSSTALK_EDA_IO_SERIAL_ERR_BUSTED;
    }

    ctx->tx_total = ctx->tx_total + sent;

    if (sent >= (uint64_t)ctx->tx_len) { ctx->tx_len = 0; }
    else
    {
        memmove(ctx->tx, &ctx->tx[sent], (uint64_t)ctx->tx_len - sent);
        ctx->tx_len = ctx->tx_len - (uint32_t)sent;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_serial_sink_cb(void *user, const uint8_t *bytes, uint32_t len)
{
    crosstalk_eda_io_serial_ctx_t *ctx = (crosstalk_eda_io_serial_ctx_t *)user;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || bytes == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->attached != LDG_TRUTH_TRUE || ctx->busted == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = crosstalk_eda_io_serial_tx_put(ctx, bytes, len);
    if (ret == CROSSTALK_EDA_IO_SERIAL_ERR_TX_FULL)
    {
        ctx->tx_dropped = ctx->tx_dropped + (uint64_t)len;
        return LDG_ERR_AOK;
    }

    return ret;
}

uint32_t crosstalk_eda_io_serial_tx_put(crosstalk_eda_io_serial_ctx_t *ctx, const uint8_t *bytes, uint32_t len)
{
    if (LDG_UNLIKELY(ctx == 0x0 || bytes == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->attached != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_IO_SERIAL_ERR_NOT_ATTACHED; }

    if (ctx->tx_len + len > (uint32_t)CROSSTALK_EDA_IO_SERIAL_TX_CAP) { return CROSSTALK_EDA_IO_SERIAL_ERR_TX_FULL; }

    memcpy(&ctx->tx[ctx->tx_len], bytes, (uint64_t)len);
    ctx->tx_len = ctx->tx_len + len;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_serial_conf_apply(crosstalk_eda_io_serial_ctx_t *ctx, const ldg_io_serial_conf_t *conf)
{
    if (LDG_UNLIKELY(ctx == 0x0 || conf == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->conf = *conf;

    if (ctx->attached != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    return ldg_io_serial_conf_set(&ctx->link, &ctx->conf);
}

uint32_t crosstalk_eda_io_serial_id_tail_get(const char *id, const char **out)
{
    uint32_t at = 0;
    uint32_t last = 0;

    if (LDG_UNLIKELY(id == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = id;

    while (at < (uint32_t)LDG_IO_SERIAL_ID_MAX && id[at] != LDG_STR_TERM)
    {
        if (id[at] == CROSSTALK_EDA_IO_SERIAL_SEP) { last = at + 0x1; }

        at = at + 0x1;
    }

    *out = &id[last];

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_io_serial_attached_is(const crosstalk_eda_io_serial_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->attached == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint8_t crosstalk_eda_io_serial_busted_is(const crosstalk_eda_io_serial_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->busted == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint8_t crosstalk_eda_io_serial_remote_is(const crosstalk_eda_io_serial_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->remote == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}
