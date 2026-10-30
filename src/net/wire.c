#include <stdint.h>
#include <string.h>

#include <dangling/logging/logging.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/net/net.h>
#include <dangling/net/steamworks.h>

#include "core/log.h"
#include "net/wire.h"

#define CROSSTALK_NET_WIRE_SEG_NONE 0

static uint32_t wire_frame_send(steam_net_ctx_t *ctx, uint8_t kind, const void *hd, uint32_t hd_len, const void *body, uint32_t body_len, uint8_t reliable)
{
    crosstalk_net_wire_hdr_t hdr = LDG_STRUCT_ZERO_INIT;
    uint64_t total = 0;
    uint32_t len = 0;

    if (LDG_UNLIKELY(!ctx || !ctx->sesh)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(hd_len > 0 && hd == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(body_len > 0 && body == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    len = hd_len + body_len;
    if (LDG_UNLIKELY(len > CROSSTALK_NET_WIRE_PAYLOAD_MAX)) { return LDG_ERR_OVERFLOW; }

    if (LDG_UNLIKELY(ctx->sesh_live == 0)) { return LDG_ERR_NOT_INIT; }

    hdr.tag = CROSSTALK_NET_WIRE_TAG;
    hdr.kind = kind;
    hdr.rev = CROSSTALK_NET_WIRE_REV;
    hdr.magic = CROSSTALK_NET_WIRE_MAGIC;
    hdr.len = len;
    hdr.seq = ctx->wire_seq;
    hdr.sender = ldg_net_steamworks_local_steam_id_get();
    ctx->wire_seq = ctx->wire_seq + CROSSTALK_NET_WIRE_SEQ_STEP;

    memcpy(ctx->wire_buff, &hdr, sizeof(hdr));
    if (hd_len > 0) { memcpy(ctx->wire_buff + sizeof(hdr), hd, hd_len); }

    if (body_len > 0) { memcpy(ctx->wire_buff + sizeof(hdr) + hd_len, body, body_len); }

    total = (uint64_t)sizeof(hdr) + (uint64_t)len;
    return ldg_net_steamworks_send(ctx->sesh, ctx->wire_buff, total, reliable);
}

uint32_t crosstalk_net_wire_send(steam_net_ctx_t *ctx, uint8_t kind, const void *payload, uint32_t len, uint8_t reliable)
{
    return wire_frame_send(ctx, kind, payload, len, 0x0, CROSSTALK_NET_WIRE_SEG_NONE, reliable);
}

uint32_t crosstalk_net_wire_blob_send(steam_net_ctx_t *ctx, uint8_t kind, const void *blob, uint32_t len)
{
    crosstalk_net_wire_chunk_t chunk = LDG_STRUCT_ZERO_INIT;
    const uint8_t *src = 0x0;
    uint32_t sent = 0;
    uint32_t part = 0;
    uint32_t chunk_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !blob)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    src = (const uint8_t *)blob;
    chunk.total_len = len;
    while (sent < len)
    {
        part = len - sent;
        if (part > CROSSTALK_NET_WIRE_CHUNK_MAX) { part = CROSSTALK_NET_WIRE_CHUNK_MAX; }

        chunk.offset = sent;
        chunk.part_len = part;
        ret = wire_frame_send(ctx, kind, &chunk, (uint32_t)sizeof(chunk), src + sent, part, CROSSTALK_NET_WIRE_RELIABLE);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        sent = sent + part;
        chunk_cunt++;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "net: blob sent; kind: %u32; len: %u32; chunks: %u32", (uint32_t)kind, len, chunk_cunt);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_wire_blob_recv(steam_net_ctx_t *ctx, const uint8_t *payload, uint32_t len, uint8_t *out_complete)
{
    crosstalk_net_wire_chunk_t chunk = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!ctx || !payload || !out_complete)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_complete = LDG_TRUTH_FALSE;
    if (LDG_UNLIKELY(ctx->snap_buff == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(len < sizeof(chunk))) { return LDG_ERR_FUNC_ARG_INVALID; }

    memcpy(&chunk, payload, sizeof(chunk));
    if (LDG_UNLIKELY(chunk.total_len == 0 || chunk.total_len > ctx->snap_cap)) { return LDG_ERR_OVERFLOW; }

    if (LDG_UNLIKELY((uint64_t)len < (uint64_t)sizeof(chunk) + (uint64_t)chunk.part_len)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY((uint64_t)chunk.offset + (uint64_t)chunk.part_len > (uint64_t)chunk.total_len)) { return LDG_ERR_BOUNDS; }

    if (chunk.offset == 0)
    {
        ctx->snap_total = chunk.total_len;
        ctx->snap_len = 0;
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "net: blob recv begin; total: %u32; chunk: %u32", chunk.total_len, chunk.part_len);
    }

    if (LDG_UNLIKELY(chunk.total_len != ctx->snap_total)) { return LDG_ERR_INVALID; }

    if (LDG_UNLIKELY(chunk.offset != ctx->snap_len)) { return LDG_ERR_INVALID; }

    memcpy(ctx->snap_buff + chunk.offset, payload + sizeof(chunk), chunk.part_len);
    ctx->snap_len = ctx->snap_len + chunk.part_len;
    if (ctx->snap_len == ctx->snap_total) { *out_complete = LDG_TRUTH_TRUE; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_net_wire_relay(steam_net_ctx_t *ctx, const void *data, uint32_t len, uint8_t reliable)
{
    if (LDG_UNLIKELY(!ctx || !ctx->sesh || !data)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len < sizeof(crosstalk_net_wire_hdr_t))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ctx->sesh->net.mode != LDG_NET_MODE_SRV)) { return LDG_ERR_DENIED; }

    return ldg_net_steamworks_bcast(ctx->sesh, data, (uint64_t)len, reliable);
}

uint32_t crosstalk_net_wire_decode(const void *data, uint32_t len, crosstalk_net_wire_hdr_t *out_hdr, const uint8_t **out_payload)
{
    crosstalk_net_wire_hdr_t hdr = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!data || !out_hdr || !out_payload)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_hdr = hdr;
    *out_payload = 0x0;

    if (LDG_UNLIKELY(len < sizeof(hdr))) { return LDG_ERR_FUNC_ARG_INVALID; }

    memcpy(&hdr, data, sizeof(hdr));
    if (LDG_UNLIKELY(hdr.tag != CROSSTALK_NET_WIRE_TAG)) { return LDG_ERR_INVALID; }

    if (LDG_UNLIKELY(hdr.magic != CROSSTALK_NET_WIRE_MAGIC)) { return LDG_ERR_INVALID; }

    if (LDG_UNLIKELY(hdr.rev != CROSSTALK_NET_WIRE_REV)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(hdr.len > CROSSTALK_NET_WIRE_PAYLOAD_MAX)) { return LDG_ERR_OVERFLOW; }

    if (LDG_UNLIKELY((uint64_t)len < (uint64_t)sizeof(hdr) + (uint64_t)hdr.len)) { return LDG_ERR_BOUNDS; }

    *out_hdr = hdr;
    *out_payload = (const uint8_t *)data + sizeof(hdr);
    return LDG_ERR_AOK;
}
