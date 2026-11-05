#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "game/credit.h"

#define CREDIT_VISIT_SEQ_FIRST 0U
#define CREDIT_LEGACY_ENTRY_SIZE 96U
#define CREDIT_LEGACY_SEG_SIZE 120U
#define CREDIT_LEGACY_LOG_SIZE 104U
#define CREDIT_LEGACY_ENTRY_CAP 512U
#define CREDIT_LEGACY_SEG_CAP 4096U
#define CREDIT_LEGACY_LOG_CAP 32U

typedef struct credit_save_hdr
{
    uint32_t entry_cunt;
    uint32_t seg_cunt;
    uint32_t log_cunt;
    uint32_t time_cunt;
} credit_save_hdr_t;

typedef struct credit_visit_hdr
{
    uint32_t visit_cunt;
    uint32_t pudding;
} credit_visit_hdr_t;

static uint64_t credit_visit_oldest_get(const crosstalk_game_credit_ctx_t *ctx)
{
    if (ctx->visit_total > (uint64_t)CROSSTALK_GAME_CREDIT_VISIT_CAP) { return ctx->visit_total - (uint64_t)CROSSTALK_GAME_CREDIT_VISIT_CAP; }

    return CREDIT_VISIT_SEQ_FIRST;
}

static uint32_t credit_persona_cpy(char *dst, uint64_t dst_size, const char *persona)
{
    uint32_t ret = 0;

    if (persona == 0x0 || persona[0] == LDG_STR_TERM) { return LDG_ERR_AOK; }

    ret = ldg_str_rbrcpy(dst, persona, dst_size);
    if (ret == LDG_ERR_STR_TRUNC) { ret = LDG_ERR_AOK; }

    return ret;
}

uint32_t crosstalk_game_credit_init(crosstalk_game_credit_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));
    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_credit_time_cum(crosstalk_game_credit_ctx_t *ctx, uint64_t steam_id, const char *persona, uint64_t discord_id, uint32_t secs_delta, uint32_t elapsed_secs)
{
    crosstalk_game_credit_time_t *slot = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(steam_id == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (secs_delta == 0) { return LDG_ERR_AOK; }

    if (secs_delta > elapsed_secs + (uint32_t)CROSSTALK_GAME_CREDIT_TIME_JITTER_SECS) { return LDG_ERR_RANGE; }

    for (uint32_t idx = 0; idx < ctx->time_cunt; idx++) { if (ctx->time[idx].steam_id == steam_id) { slot = &ctx->time[idx]; } }

    if (slot == 0x0)
    {
        if (ctx->time_cunt >= (uint32_t)CROSSTALK_GAME_CREDIT_TIME_CAP) { return LDG_ERR_FULL; }

        slot = &ctx->time[ctx->time_cunt];
        memset(slot, 0x0, sizeof(*slot));
        slot->steam_id = steam_id;
        ctx->time_cunt++;
    }

    if (LDG_UNLIKELY(slot->secs > UINT32_MAX - secs_delta)) { slot->secs = UINT32_MAX; }
    else { slot->secs = slot->secs + secs_delta; }

    if (discord_id != 0) { slot->discord_id = discord_id; }

    return credit_persona_cpy(slot->persona, (uint64_t)sizeof(slot->persona), persona);
}

uint32_t crosstalk_game_credit_time_get(const crosstalk_game_credit_ctx_t *ctx, uint64_t steam_id, const crosstalk_game_credit_time_t **out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    for (uint32_t idx = 0; idx < ctx->time_cunt; idx++)
    {
        if (ctx->time[idx].steam_id != steam_id) { continue; }

        *out = &ctx->time[idx];
        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t crosstalk_game_credit_visit_open(crosstalk_game_credit_ctx_t *ctx, uint64_t steam_id, const char *persona, uint64_t epoch_ms, uint64_t *out_seq)
{
    crosstalk_game_credit_visit_t *slot = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_seq == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_seq = CROSSTALK_GAME_CREDIT_VISIT_SEQ_NONE;

    if (LDG_UNLIKELY(steam_id == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ctx->visit_total == CROSSTALK_GAME_CREDIT_VISIT_SEQ_NONE)) { return LDG_ERR_OVERFLOW; }

    slot = &ctx->visit[ctx->visit_total % (uint64_t)CROSSTALK_GAME_CREDIT_VISIT_CAP];
    memset(slot, 0x0, sizeof(*slot));
    slot->steam_id = steam_id;
    slot->join_epoch_ms = epoch_ms;
    slot->last_epoch_ms = epoch_ms;

    *out_seq = ctx->visit_total;
    ctx->visit_total++;

    return credit_persona_cpy(slot->persona, (uint64_t)sizeof(slot->persona), persona);
}

uint32_t crosstalk_game_credit_visit_touch(crosstalk_game_credit_ctx_t *ctx, uint64_t seq, uint64_t epoch_ms)
{
    crosstalk_game_credit_visit_t *slot = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (seq >= ctx->visit_total || seq < credit_visit_oldest_get(ctx)) { return LDG_ERR_NOT_FOUND; }

    slot = &ctx->visit[seq % (uint64_t)CROSSTALK_GAME_CREDIT_VISIT_CAP];
    if (epoch_ms > slot->last_epoch_ms) { slot->last_epoch_ms = epoch_ms; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_credit_visit_cunt_get(const crosstalk_game_credit_ctx_t *ctx, uint32_t *out_cunt)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = (uint32_t)(ctx->visit_total - credit_visit_oldest_get(ctx));
    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_credit_visit_get(const crosstalk_game_credit_ctx_t *ctx, uint32_t idx, const crosstalk_game_credit_visit_t **out)
{
    uint64_t oldest = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    oldest = credit_visit_oldest_get(ctx);
    if (oldest + (uint64_t)idx >= ctx->visit_total) { return LDG_ERR_BOUNDS; }

    *out = &ctx->visit[(oldest + (uint64_t)idx) % (uint64_t)CROSSTALK_GAME_CREDIT_VISIT_CAP];
    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_credit_serialize(const crosstalk_game_credit_ctx_t *ctx, uint8_t *out, uint32_t cap, uint32_t *out_len)
{
    credit_save_hdr_t hdr = LDG_STRUCT_ZERO_INIT;
    credit_visit_hdr_t visit_hdr = LDG_STRUCT_ZERO_INIT;
    uint64_t oldest = 0;
    uint32_t time_bytes = 0;
    uint32_t total = 0;
    uint32_t off = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (LDG_UNLIKELY(ctx->time_cunt > (uint32_t)CROSSTALK_GAME_CREDIT_TIME_CAP)) { return LDG_ERR_INVALID; }

    oldest = credit_visit_oldest_get(ctx);
    visit_hdr.visit_cunt = (uint32_t)(ctx->visit_total - oldest);
    time_bytes = ctx->time_cunt * (uint32_t)sizeof(crosstalk_game_credit_time_t);
    total = (uint32_t)sizeof(hdr) + time_bytes + (uint32_t)sizeof(visit_hdr) + visit_hdr.visit_cunt * (uint32_t)sizeof(crosstalk_game_credit_visit_t);

    *out_len = total;
    if (out == 0x0) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(cap < total)) { return LDG_ERR_BOUNDS; }

    hdr.time_cunt = ctx->time_cunt;
    memcpy(out, &hdr, sizeof(hdr));
    off = (uint32_t)sizeof(hdr);

    if (time_bytes > 0) { memcpy(out + off, ctx->time, time_bytes); }

    off = off + time_bytes;
    memcpy(out + off, &visit_hdr, sizeof(visit_hdr));
    off = off + (uint32_t)sizeof(visit_hdr);

    for (uint32_t idx = 0; idx < visit_hdr.visit_cunt; idx++)
    {
        memcpy(out + off, &ctx->visit[(oldest + (uint64_t)idx) % (uint64_t)CROSSTALK_GAME_CREDIT_VISIT_CAP], sizeof(crosstalk_game_credit_visit_t));
        off = off + (uint32_t)sizeof(crosstalk_game_credit_visit_t);
    }

    return LDG_ERR_AOK;
}

static uint32_t credit_legacy_deserialize(crosstalk_game_credit_ctx_t *ctx, const credit_save_hdr_t *hdr, const uint8_t *blob, uint32_t len)
{
    uint64_t skip = 0;
    uint64_t time_bytes = 0;

    if (LDG_UNLIKELY(hdr->entry_cunt > CREDIT_LEGACY_ENTRY_CAP || hdr->seg_cunt > CREDIT_LEGACY_SEG_CAP || hdr->log_cunt > CREDIT_LEGACY_LOG_CAP || hdr->time_cunt > (uint32_t)CROSSTALK_GAME_CREDIT_TIME_CAP)) { return LDG_ERR_IO_FMT; }

    skip = (uint64_t)sizeof(*hdr) + (uint64_t)hdr->entry_cunt * (uint64_t)CREDIT_LEGACY_ENTRY_SIZE + (uint64_t)hdr->seg_cunt * (uint64_t)CREDIT_LEGACY_SEG_SIZE + (uint64_t)hdr->log_cunt * (uint64_t)CREDIT_LEGACY_LOG_SIZE;
    if (LDG_UNLIKELY(skip > (uint64_t)len)) { return LDG_ERR_IO_FMT; }

    time_bytes = (uint64_t)hdr->time_cunt * (uint64_t)sizeof(crosstalk_game_credit_time_t);
    if ((uint64_t)len - skip < time_bytes) { return LDG_ERR_AOK; }

    if (time_bytes > 0) { memcpy(ctx->time, blob + skip, time_bytes); }

    for (uint32_t idx = 0; idx < hdr->time_cunt; idx++) { ctx->time[idx].persona[sizeof(ctx->time[idx].persona) - LDG_STR_TERM_SIZE] = LDG_STR_TERM; }

    ctx->time_cunt = hdr->time_cunt;
    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_credit_deserialize(crosstalk_game_credit_ctx_t *ctx, const uint8_t *blob, uint32_t len)
{
    credit_save_hdr_t hdr = LDG_STRUCT_ZERO_INIT;
    credit_visit_hdr_t visit_hdr = LDG_STRUCT_ZERO_INIT;
    uint32_t time_bytes = 0;
    uint32_t off = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || blob == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_game_credit_init(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(len < (uint32_t)sizeof(hdr))) { return LDG_ERR_IO_FMT; }

    memcpy(&hdr, blob, sizeof(hdr));

    if (hdr.entry_cunt != 0 || hdr.seg_cunt != 0 || hdr.log_cunt != 0) { return credit_legacy_deserialize(ctx, &hdr, blob, len); }

    if (LDG_UNLIKELY(hdr.time_cunt > (uint32_t)CROSSTALK_GAME_CREDIT_TIME_CAP)) { return LDG_ERR_IO_FMT; }

    time_bytes = hdr.time_cunt * (uint32_t)sizeof(crosstalk_game_credit_time_t);
    off = (uint32_t)sizeof(hdr);
    if (LDG_UNLIKELY(len - off < time_bytes)) { return LDG_ERR_IO_FMT; }

    if (time_bytes > 0) { memcpy(ctx->time, blob + off, time_bytes); }

    off = off + time_bytes;

    for (uint32_t idx = 0; idx < hdr.time_cunt; idx++) { ctx->time[idx].persona[sizeof(ctx->time[idx].persona) - LDG_STR_TERM_SIZE] = LDG_STR_TERM; }

    ctx->time_cunt = hdr.time_cunt;

    if (len - off < (uint32_t)sizeof(visit_hdr)) { return LDG_ERR_AOK; }

    memcpy(&visit_hdr, blob + off, sizeof(visit_hdr));
    off = off + (uint32_t)sizeof(visit_hdr);

    if (LDG_UNLIKELY(visit_hdr.visit_cunt > (uint32_t)CROSSTALK_GAME_CREDIT_VISIT_CAP)) { return LDG_ERR_IO_FMT; }

    if (LDG_UNLIKELY(len - off < visit_hdr.visit_cunt * (uint32_t)sizeof(crosstalk_game_credit_visit_t))) { return LDG_ERR_IO_FMT; }

    if (visit_hdr.visit_cunt > 0) { memcpy(ctx->visit, blob + off, (uint64_t)visit_hdr.visit_cunt * sizeof(crosstalk_game_credit_visit_t)); }

    for (uint32_t idx = 0; idx < visit_hdr.visit_cunt; idx++) { ctx->visit[idx].persona[sizeof(ctx->visit[idx].persona) - LDG_STR_TERM_SIZE] = LDG_STR_TERM; }

    ctx->visit_total = (uint64_t)visit_hdr.visit_cunt;
    return LDG_ERR_AOK;
}
