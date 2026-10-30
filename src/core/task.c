#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/logging/logging.h>

#include "core/task.h"

#define TASK_STEP 1

static uint32_t task_slot_get(const crosstalk_core_task_ctx_t *ctx, uint32_t back)
{
    return (ctx->hd + (uint32_t)CROSSTALK_CORE_TASK_VISIT_CAP - (uint32_t)TASK_STEP - back) % (uint32_t)CROSSTALK_CORE_TASK_VISIT_CAP;
}

static uint64_t task_dwell_get(const crosstalk_core_task_visit_t *slot, uint64_t now_ms)
{
    return now_ms > slot->enter_ms ? now_ms - slot->enter_ms : (uint64_t)CROSSTALK_CORE_TASK_DWELL_NONE;
}

uint32_t crosstalk_core_task_init(crosstalk_core_task_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_task_visit_enter(crosstalk_core_task_ctx_t *ctx, uint64_t win_id, uint64_t now_ms)
{
    crosstalk_core_task_visit_t *slot = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (win_id == (uint64_t)UINT64_MAX) { return crosstalk_core_task_visit_close(ctx, now_ms); }

    if (ctx->open_is == LDG_TRUTH_TRUE)
    {
        slot = &ctx->visit[task_slot_get(ctx, (uint32_t)CROSSTALK_CORE_TASK_BACK_CURRENT)];

        if (slot->win_id == win_id) { return LDG_ERR_AOK; }

        slot->dwell_ms = task_dwell_get(slot, now_ms);
    }

    slot = &ctx->visit[ctx->hd];

    slot->win_id = win_id;
    slot->enter_ms = now_ms;
    slot->dwell_ms = (uint64_t)CROSSTALK_CORE_TASK_DWELL_NONE;

    ctx->hd = (ctx->hd + (uint32_t)TASK_STEP) % (uint32_t)CROSSTALK_CORE_TASK_VISIT_CAP;

    if (ctx->cunt < (uint32_t)CROSSTALK_CORE_TASK_VISIT_CAP) { ctx->cunt = ctx->cunt + (uint32_t)TASK_STEP; }

    ctx->visit_total = ctx->visit_total + (uint64_t)TASK_STEP;
    ctx->open_is = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_task_visit_close(crosstalk_core_task_ctx_t *ctx, uint64_t now_ms)
{
    crosstalk_core_task_visit_t *slot = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (ctx->open_is != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    slot = &ctx->visit[task_slot_get(ctx, (uint32_t)CROSSTALK_CORE_TASK_BACK_CURRENT)];

    slot->dwell_ms = task_dwell_get(slot, now_ms);

    ctx->open_is = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_task_visit_recent_get(const crosstalk_core_task_ctx_t *ctx, uint32_t back, crosstalk_core_task_visit_t *out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(out, 0x0, sizeof(*out));

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (ctx->cunt == (uint32_t)CROSSTALK_CORE_TASK_CUNT_NONE) { return CROSSTALK_CORE_TASK_ERR_EMPTY; }

    if (back >= ctx->cunt) { return CROSSTALK_CORE_TASK_ERR_RANGE; }

    *out = ctx->visit[task_slot_get(ctx, back)];

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_task_visit_cunt_get(const crosstalk_core_task_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return (uint32_t)CROSSTALK_CORE_TASK_CUNT_NONE; }

    return ctx->cunt;
}

uint64_t crosstalk_core_task_visit_total_get(const crosstalk_core_task_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return (uint64_t)CROSSTALK_CORE_TASK_CUNT_NONE; }

    return ctx->visit_total;
}

uint32_t crosstalk_core_task_dwell_get(const crosstalk_core_task_ctx_t *ctx, uint64_t win_id, uint64_t now_ms, uint64_t *out_ms, uint32_t *out_visits)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_ms == 0x0 || out_visits == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_ms = (uint64_t)CROSSTALK_CORE_TASK_DWELL_NONE;
    *out_visits = (uint32_t)CROSSTALK_CORE_TASK_CUNT_NONE;

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    for (at = 0; at < ctx->cunt; at = at + (uint32_t)TASK_STEP)
    {
        const crosstalk_core_task_visit_t *slot = &ctx->visit[task_slot_get(ctx, at)];

        if (slot->win_id != win_id) { continue; }

        if (at == (uint32_t)CROSSTALK_CORE_TASK_BACK_CURRENT && ctx->open_is == LDG_TRUTH_TRUE) { *out_ms = *out_ms + task_dwell_get(slot, now_ms); }
        else { *out_ms = *out_ms + slot->dwell_ms; }

        *out_visits = *out_visits + (uint32_t)TASK_STEP;
    }

    return LDG_ERR_AOK;
}
