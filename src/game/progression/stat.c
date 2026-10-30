#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#include <dangling/net/steamworks.h>

#include "game/progression/progression.h"
#include "game/progression/stat.h"

#define STAT_NONE 0

uint32_t crosstalk_game_progression_stat_int_get(const char *name, int32_t *out)
{
    if (LDG_UNLIKELY(!name || !out || name[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = STAT_NONE;
    return ldg_net_steamworks_stat_int_get(name, out);
}

uint32_t crosstalk_game_progression_stat_int_set_req(const char *name, int32_t val, uint32_t appid, crosstalk_game_progression_queue_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!name || name[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_NOT_INIT; }

    return crosstalk_game_progression_queue_stat_push(ctx, name, val, appid);
}

uint32_t crosstalk_game_progression_stat_int_inc_req(const char *name, int32_t delta, uint32_t appid, crosstalk_game_progression_queue_ctx_t *ctx)
{
    int32_t curr = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!name || name[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_NOT_INIT; }

    ret = ldg_net_steamworks_stat_int_get(name, &curr);
    if (ret != LDG_ERR_AOK) { curr = STAT_NONE; }

    return crosstalk_game_progression_queue_stat_push(ctx, name, curr + delta, appid);
}
