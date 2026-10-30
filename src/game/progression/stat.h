#ifndef CROSSTALK_GAME_PROGRESSION_STAT_H
#define CROSSTALK_GAME_PROGRESSION_STAT_H

#include <stdint.h>

#include "game/progression/queue.h"

uint32_t crosstalk_game_progression_stat_int_get(const char *name, int32_t *out);
uint32_t crosstalk_game_progression_stat_int_set_req(const char *name, int32_t val, uint32_t appid, crosstalk_game_progression_queue_ctx_t *ctx);
uint32_t crosstalk_game_progression_stat_int_inc_req(const char *name, int32_t delta, uint32_t appid, crosstalk_game_progression_queue_ctx_t *ctx);

#endif
