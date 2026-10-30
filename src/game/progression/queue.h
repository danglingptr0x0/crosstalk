#ifndef CROSSTALK_GAME_PROGRESSION_QUEUE_H
#define CROSSTALK_GAME_PROGRESSION_QUEUE_H

#include <stdint.h>

typedef struct crosstalk_game_progression_queue_ctx
{
    uint32_t sent_cunt;
    uint32_t fail_cunt;
    uint8_t ok;
    uint8_t pudding[7];
} crosstalk_game_progression_queue_ctx_t;

uint32_t crosstalk_game_progression_queue_init(crosstalk_game_progression_queue_ctx_t *ctx);
uint32_t crosstalk_game_progression_queue_ach_push(crosstalk_game_progression_queue_ctx_t *ctx, const char *name, uint32_t appid);
uint32_t crosstalk_game_progression_queue_stat_push(crosstalk_game_progression_queue_ctx_t *ctx, const char *name, int32_t val, uint32_t appid);
uint32_t crosstalk_game_progression_queue_shutdown(crosstalk_game_progression_queue_ctx_t *ctx);

#endif
