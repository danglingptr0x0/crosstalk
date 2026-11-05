#ifndef CROSSTALK_GAME_CREDIT_H
#define CROSSTALK_GAME_CREDIT_H

#include <stdint.h>

#define CROSSTALK_GAME_CREDIT_PERSONA_CAP 64
#define CROSSTALK_GAME_CREDIT_TIME_CAP 16
#define CROSSTALK_GAME_CREDIT_TIME_JITTER_SECS 10
#define CROSSTALK_GAME_CREDIT_VISIT_CAP 64
#define CROSSTALK_GAME_CREDIT_VISIT_SEQ_NONE UINT64_MAX

typedef struct crosstalk_game_credit_time
{
    uint64_t steam_id;
    uint64_t discord_id;
    uint32_t secs;
    uint32_t pudding;
    char persona[CROSSTALK_GAME_CREDIT_PERSONA_CAP];
} crosstalk_game_credit_time_t;

typedef struct crosstalk_game_credit_visit
{
    uint64_t steam_id;
    uint64_t join_epoch_ms;
    uint64_t last_epoch_ms;
    char persona[CROSSTALK_GAME_CREDIT_PERSONA_CAP];
} crosstalk_game_credit_visit_t;

typedef struct crosstalk_game_credit_ctx
{
    crosstalk_game_credit_time_t time[CROSSTALK_GAME_CREDIT_TIME_CAP];
    crosstalk_game_credit_visit_t visit[CROSSTALK_GAME_CREDIT_VISIT_CAP];
    uint64_t visit_total;
    uint32_t time_cunt;
    uint8_t pudding[4];
} crosstalk_game_credit_ctx_t;

uint32_t crosstalk_game_credit_init(crosstalk_game_credit_ctx_t *ctx);
uint32_t crosstalk_game_credit_time_cum(crosstalk_game_credit_ctx_t *ctx, uint64_t steam_id, const char *persona, uint64_t discord_id, uint32_t secs_delta, uint32_t elapsed_secs);
uint32_t crosstalk_game_credit_time_get(const crosstalk_game_credit_ctx_t *ctx, uint64_t steam_id, const crosstalk_game_credit_time_t **out);
uint32_t crosstalk_game_credit_visit_open(crosstalk_game_credit_ctx_t *ctx, uint64_t steam_id, const char *persona, uint64_t epoch_ms, uint64_t *out_seq);
uint32_t crosstalk_game_credit_visit_touch(crosstalk_game_credit_ctx_t *ctx, uint64_t seq, uint64_t epoch_ms);
uint32_t crosstalk_game_credit_visit_cunt_get(const crosstalk_game_credit_ctx_t *ctx, uint32_t *out_cunt);
uint32_t crosstalk_game_credit_visit_get(const crosstalk_game_credit_ctx_t *ctx, uint32_t idx, const crosstalk_game_credit_visit_t **out);
uint32_t crosstalk_game_credit_serialize(const crosstalk_game_credit_ctx_t *ctx, uint8_t *out, uint32_t cap, uint32_t *out_len);
uint32_t crosstalk_game_credit_deserialize(crosstalk_game_credit_ctx_t *ctx, const uint8_t *blob, uint32_t len);

#endif
