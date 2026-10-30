#include <stdint.h>

#include <dangling/logging/logging.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/net/steamworks.h>

#include "core/log.h"
#include "game/progression/queue.h"

static uint32_t progression_queue_app_ck(uint32_t appid)
{
    if (appid != ldg_net_steamworks_app_id_get()) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

static uint32_t progression_queue_res_note(crosstalk_game_progression_queue_ctx_t *ctx, const char *name, uint32_t ret)
{
    if (ret == LDG_ERR_AOK)
    {
        ctx->sent_cunt++;
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "progression: sent; name: %.31s", name);
        return ret;
    }

    ctx->fail_cunt++;
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "progression: send failed; name: %.31s; err: %u32", name, ret);
    return ret;
}

uint32_t crosstalk_game_progression_queue_init(crosstalk_game_progression_queue_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->sent_cunt = 0;
    ctx->fail_cunt = 0;
    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_progression_queue_ach_push(crosstalk_game_progression_queue_ctx_t *ctx, const char *name, uint32_t appid)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0 || name[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ret = progression_queue_app_ck(appid);
    if (ret == LDG_ERR_AOK) { ret = ldg_net_steamworks_achievement_set(name); }

    if (ret == LDG_ERR_AOK) { ret = ldg_net_steamworks_stat_st(); }

    return progression_queue_res_note(ctx, name, ret);
}

uint32_t crosstalk_game_progression_queue_stat_push(crosstalk_game_progression_queue_ctx_t *ctx, const char *name, int32_t val, uint32_t appid)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0 || name[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ret = progression_queue_app_ck(appid);
    if (ret == LDG_ERR_AOK) { ret = ldg_net_steamworks_stat_int_set(name, val); }

    if (ret == LDG_ERR_AOK) { ret = ldg_net_steamworks_stat_st(); }

    return progression_queue_res_note(ctx, name, ret);
}

uint32_t crosstalk_game_progression_queue_shutdown(crosstalk_game_progression_queue_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "progression: shutdown; sent: %u32; failed: %u32", ctx->sent_cunt, ctx->fail_cunt);
    ctx->ok = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}
