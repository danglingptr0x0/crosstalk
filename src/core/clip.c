#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "core/clip.h"

uint32_t crosstalk_core_clip_init(crosstalk_core_clip_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_clip_set(crosstalk_core_clip_ctx_t *ctx, const char *text)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (text[0] == LDG_STR_TERM) { return crosstalk_core_clip_clear(ctx); }

    ret = ldg_str_rbrcpy(ctx->text, text, (uint64_t)sizeof(ctx->text));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    ctx->has = LDG_TRUTH_TRUE;
    ctx->gen = ctx->gen + 0x1;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_clip_get(const crosstalk_core_clip_ctx_t *ctx, char *out, uint32_t cap)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_BOUNDS; }

    out[0] = LDG_STR_TERM;

    if (ctx->has != LDG_TRUTH_TRUE) { return LDG_ERR_EMPTY; }

    ret = ldg_str_rbrcpy(out, ctx->text, (uint64_t)cap);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_clip_clear(crosstalk_core_clip_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->text[0] = LDG_STR_TERM;
    ctx->has = LDG_TRUTH_FALSE;
    ctx->gen = ctx->gen + 0x1;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_core_clip_has(const crosstalk_core_clip_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->has;
}
