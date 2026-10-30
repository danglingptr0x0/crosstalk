#include <stdint.h>
#include <string.h>

#include <dangling/logging/logging.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include <dangling/net/steamworks.h>

#include "core/log.h"
#include "game/progression/progression.h"

#define PROGRESSION_BIT_BASE 1
#define PROGRESSION_BIT_CAP (CROSSTALK_GAME_PROGRESSION_BIT_WORD_BITS * CROSSTALK_GAME_PROGRESSION_BIT_WORD_CUNT)

uint32_t crosstalk_game_progression_achievement_unlock_req(const char *name, uint32_t appid, crosstalk_game_progression_queue_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!name || name[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx == 0x0 || appid == 0)) { return LDG_ERR_NOT_INIT; }

    /*
    uint8_t unlocked = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    ret = ldg_net_steamworks_achievement_get(name, &unlocked);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (unlocked == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    return crosstalk_game_progression_queue_ach_push(ctx, name, appid);
    */

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_progression_achievement_progress_req(const char *name, uint32_t curr, uint32_t max, uint32_t appid, crosstalk_game_progression_queue_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!name || name[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(curr > max)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (curr == max) { return crosstalk_game_progression_achievement_unlock_req(name, appid, ctx); }

    return LDG_ERR_AOK;
}

uint8_t crosstalk_game_progression_achievement_unlocked_is(const char *name)
{
    if (LDG_UNLIKELY(name == 0x0 || name[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_NULL; }

    /*
    uint8_t unlocked = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    ret = ldg_net_steamworks_achievement_get(name, &unlocked);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_NOT_INIT; }

    return unlocked == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    */

    return LDG_TRUTH_INDETERMINATE;
}

uint32_t crosstalk_game_progression_achievement_once_req(crosstalk_game_progression_state_t *state, const char *name, uint32_t appid, crosstalk_game_progression_queue_ctx_t *ctx)
{
    const char *digits = 0x0;
    uint64_t bit = 0;
    uint32_t word = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(state == 0x0 || name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    digits = strrchr(name, '_');
    if (LDG_UNLIKELY(digits == 0x0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_str_to_dec(&digits[1], &idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(idx >= PROGRESSION_BIT_CAP)) { return LDG_ERR_FUNC_ARG_INVALID; }

    word = idx / CROSSTALK_GAME_PROGRESSION_BIT_WORD_BITS;
    bit = (uint64_t)PROGRESSION_BIT_BASE << (idx % CROSSTALK_GAME_PROGRESSION_BIT_WORD_BITS);
    if ((state->fired[word] & bit) != 0) { return LDG_ERR_AOK; }

    ret = crosstalk_game_progression_achievement_unlock_req(name, appid, ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "achievement: req failed; name: %s; err: %u32", name, ret);
        return ret;
    }

    state->fired[word] = state->fired[word] | bit;
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "achievement: fired; name: %s", name);
    return LDG_ERR_AOK;
}
