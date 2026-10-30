#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>
#include <dangling/time/time.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "reng/reng.h"

static uint32_t reng_text_ck(const char *text, uint32_t cap)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (text[at] != LDG_STR_TERM)
    {
        if (text[at] == '\n' || text[at] == '\r') { return CROSSTALK_RENG_ERR_TEXT; }

        at = at + 0x1;

        if (at + LDG_STR_TERM_SIZE >= cap) { return LDG_ERR_STR_TRUNC; }
    }

    if (at == 0) { return CROSSTALK_RENG_ERR_TEXT; }

    return LDG_ERR_AOK;
}

static uint8_t reng_str_eq_is(const char *lhs, const char *rhs)
{
    uint32_t at = 0;

    while (lhs[at] != LDG_STR_TERM && lhs[at] == rhs[at]) { at = at + 0x1; }

    return lhs[at] == rhs[at] ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static void reng_touch(crosstalk_reng_ctx_t *ctx, uint32_t proj)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (proj < ctx->proj_cunt) { ctx->proj[proj].updated_ms = ldg_time_epoch_ms_get(); }

    ctx->gen = ctx->gen + 0x1;
    ctx->dirty = LDG_TRUTH_TRUE;
}

static void reng_proj_ref_shift(crosstalk_reng_ctx_t *ctx, uint32_t gone)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    at = 0;
    while (at < ctx->task_cunt)
    {
        if (ctx->task[at].proj == gone)
        {
            if (at + 0x1 < ctx->task_cunt) { memmove(&ctx->task[at], &ctx->task[at + 0x1], (uint64_t)(ctx->task_cunt - at - 0x1) * sizeof(ctx->task[0])); }

            ctx->task_cunt = ctx->task_cunt - 0x1;
            continue;
        }

        if (ctx->task[at].proj > gone) { ctx->task[at].proj = ctx->task[at].proj - 0x1; }

        at = at + 0x1;
    }

    at = 0;
    while (at < ctx->note_cunt)
    {
        if (ctx->note[at].proj == gone)
        {
            if (at + 0x1 < ctx->note_cunt) { memmove(&ctx->note[at], &ctx->note[at + 0x1], (uint64_t)(ctx->note_cunt - at - 0x1) * sizeof(ctx->note[0])); }

            ctx->note_cunt = ctx->note_cunt - 0x1;
            continue;
        }

        if (ctx->note[at].proj > gone) { ctx->note[at].proj = ctx->note[at].proj - 0x1; }

        at = at + 0x1;
    }

    at = 0;
    while (at < ctx->mark_cunt)
    {
        if (ctx->mark[at].proj == gone)
        {
            if (at + 0x1 < ctx->mark_cunt) { memmove(&ctx->mark[at], &ctx->mark[at + 0x1], (uint64_t)(ctx->mark_cunt - at - 0x1) * sizeof(ctx->mark[0])); }

            ctx->mark_cunt = ctx->mark_cunt - 0x1;
            continue;
        }

        if (ctx->mark[at].proj > gone) { ctx->mark[at].proj = ctx->mark[at].proj - 0x1; }

        at = at + 0x1;
    }

    for (at = 0; at < ctx->sample_cunt; at = at + 0x1)
    {
        crosstalk_reng_sample_t *sample = &ctx->sample[(ctx->sample_hd + at) % (uint32_t)CROSSTALK_RENG_SAMPLE_MAX];

        if (sample->proj == gone) { sample->proj = (uint32_t)UINT32_MAX; }
        else if (sample->proj != (uint32_t)UINT32_MAX && sample->proj > gone) { sample->proj = sample->proj - 0x1; }
    }
}

uint32_t crosstalk_reng_init(crosstalk_reng_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->sel = (uint32_t)UINT32_MAX;
    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_reng_clear(crosstalk_reng_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->proj_cunt = 0;
    ctx->task_cunt = 0;
    ctx->note_cunt = 0;
    ctx->mark_cunt = 0;
    ctx->sample_cunt = 0;
    ctx->sample_hd = 0;
    ctx->sel = (uint32_t)UINT32_MAX;
    ctx->gen = ctx->gen + 0x1;
    ctx->dirty = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_reng_shutdown(crosstalk_reng_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    return LDG_ERR_AOK;
}

uint8_t crosstalk_reng_dirty_is(const crosstalk_reng_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->dirty == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_reng_kind_name_get(uint32_t kind, const char **out)
{
    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (kind == (uint32_t)CROSSTALK_RENG_KIND_DESIGN) { *out = "design"; }
    else if (kind == (uint32_t)CROSSTALK_RENG_KIND_ZONE) { *out = "zone"; }
    else if (kind == (uint32_t)CROSSTALK_RENG_KIND_BOARD) { *out = "board"; }
    else if (kind == (uint32_t)CROSSTALK_RENG_KIND_REVERSE) { *out = "reverse"; }
    else { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_reng_kind_parse(const char *name, uint32_t *out)
{
    const char *known = 0x0;
    uint32_t kind = 0;

    if (LDG_UNLIKELY(name == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (uint32_t)CROSSTALK_RENG_KIND_DESIGN;

    for (kind = 0; kind < (uint32_t)CROSSTALK_RENG_KIND_CUNT; kind = kind + 0x1)
    {
        if (crosstalk_reng_kind_name_get(kind, &known) != LDG_ERR_AOK) { continue; }

        if (reng_str_eq_is(known, name) != LDG_TRUTH_TRUE) { continue; }

        *out = kind;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t crosstalk_reng_proj_add(crosstalk_reng_ctx_t *ctx, const char *name, uint32_t kind, const char *path, uint32_t img_hash, uint32_t *out)
{
    crosstalk_reng_proj_t *proj = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (uint32_t)UINT32_MAX;

    if (kind >= (uint32_t)CROSSTALK_RENG_KIND_CUNT) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = reng_text_ck(name, (uint32_t)CROSSTALK_RENG_NAME_CAP);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (crosstalk_reng_proj_find(ctx, name, out) == LDG_ERR_AOK) { return LDG_ERR_EXISTS; }

    if (ctx->proj_cunt >= (uint32_t)CROSSTALK_RENG_PROJ_MAX) { return CROSSTALK_RENG_ERR_FULL; }

    proj = &ctx->proj[ctx->proj_cunt];

    memset(proj, 0x0, sizeof(*proj));

    ret = ldg_str_rbrcpy(proj->name, name, (uint64_t)sizeof(proj->name));
    if (ret != LDG_ERR_AOK) { return ret; }

    if (path != 0x0)
    {
        ret = ldg_str_rbrcpy(proj->path, path, (uint64_t)sizeof(proj->path));
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    proj->kind = kind;
    proj->img_hash = img_hash;
    proj->created_ms = ldg_time_epoch_ms_get();
    proj->updated_ms = proj->created_ms;

    *out = ctx->proj_cunt;
    ctx->proj_cunt = ctx->proj_cunt + 0x1;

    reng_touch(ctx, *out);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_reng_proj_find(const crosstalk_reng_ctx_t *ctx, const char *name, uint32_t *out)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (uint32_t)UINT32_MAX;

    for (idx = 0; idx < ctx->proj_cunt; idx = idx + 0x1)
    {
        if (reng_str_eq_is(ctx->proj[idx].name, name) != LDG_TRUTH_TRUE) { continue; }

        *out = idx;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t crosstalk_reng_proj_hash_find(const crosstalk_reng_ctx_t *ctx, uint32_t img_hash, uint32_t *out)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (uint32_t)UINT32_MAX;

    if (img_hash == 0) { return LDG_ERR_NOT_FOUND; }

    for (idx = 0; idx < ctx->proj_cunt; idx = idx + 0x1)
    {
        if (ctx->proj[idx].img_hash != img_hash) { continue; }

        *out = idx;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t crosstalk_reng_proj_get(const crosstalk_reng_ctx_t *ctx, uint32_t idx, const crosstalk_reng_proj_t **out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (idx >= ctx->proj_cunt) { return LDG_ERR_BOUNDS; }

    *out = &ctx->proj[idx];

    return LDG_ERR_AOK;
}

uint32_t crosstalk_reng_proj_cunt_get(const crosstalk_reng_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return UINT32_MAX; }

    return ctx->proj_cunt;
}

uint32_t crosstalk_reng_proj_remove(crosstalk_reng_ctx_t *ctx, uint32_t idx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (idx >= ctx->proj_cunt) { return LDG_ERR_BOUNDS; }

    if (idx + 0x1 < ctx->proj_cunt) { memmove(&ctx->proj[idx], &ctx->proj[idx + 0x1], (uint64_t)(ctx->proj_cunt - idx - 0x1) * sizeof(ctx->proj[0])); }

    ctx->proj_cunt = ctx->proj_cunt - 0x1;

    reng_proj_ref_shift(ctx, idx);

    if (ctx->sel == idx) { ctx->sel = (uint32_t)UINT32_MAX; }
    else if (ctx->sel != (uint32_t)UINT32_MAX && ctx->sel > idx) { ctx->sel = ctx->sel - 0x1; }

    reng_touch(ctx, (uint32_t)UINT32_MAX);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_reng_sel_set(crosstalk_reng_ctx_t *ctx, uint32_t idx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (idx != (uint32_t)UINT32_MAX && idx >= ctx->proj_cunt) { return LDG_ERR_BOUNDS; }

    ctx->sel = idx;
    ctx->gen = ctx->gen + 0x1;
    ctx->dirty = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_reng_sel_get(const crosstalk_reng_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return (uint32_t)UINT32_MAX; }

    return ctx->sel;
}

uint32_t crosstalk_reng_task_add(crosstalk_reng_ctx_t *ctx, uint32_t proj, const char *text, uint32_t *out)
{
    crosstalk_reng_task_t *task = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (uint32_t)UINT32_MAX;

    if (proj >= ctx->proj_cunt) { return LDG_ERR_BOUNDS; }

    ret = reng_text_ck(text, (uint32_t)CROSSTALK_RENG_TEXT_CAP);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (ctx->task_cunt >= (uint32_t)CROSSTALK_RENG_TASK_MAX) { return CROSSTALK_RENG_ERR_FULL; }

    task = &ctx->task[ctx->task_cunt];

    memset(task, 0x0, sizeof(*task));

    ret = ldg_str_rbrcpy(task->text, text, (uint64_t)sizeof(task->text));
    if (ret != LDG_ERR_AOK) { return ret; }

    task->proj = proj;
    task->created_ms = ldg_time_epoch_ms_get();

    *out = ctx->task_cunt;
    ctx->task_cunt = ctx->task_cunt + 0x1;

    reng_touch(ctx, proj);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_reng_task_done_set(crosstalk_reng_ctx_t *ctx, uint32_t idx, uint8_t done)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (idx >= ctx->task_cunt) { return LDG_ERR_BOUNDS; }

    ctx->task[idx].done = done == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    ctx->task[idx].done_ms = 0;

    if (done == LDG_TRUTH_TRUE) { ctx->task[idx].done_ms = ldg_time_epoch_ms_get(); }

    reng_touch(ctx, ctx->task[idx].proj);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_reng_task_remove(crosstalk_reng_ctx_t *ctx, uint32_t idx)
{
    uint32_t proj = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (idx >= ctx->task_cunt) { return LDG_ERR_BOUNDS; }

    proj = ctx->task[idx].proj;

    if (idx + 0x1 < ctx->task_cunt) { memmove(&ctx->task[idx], &ctx->task[idx + 0x1], (uint64_t)(ctx->task_cunt - idx - 0x1) * sizeof(ctx->task[0])); }

    ctx->task_cunt = ctx->task_cunt - 0x1;

    reng_touch(ctx, proj);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_reng_task_get(const crosstalk_reng_ctx_t *ctx, uint32_t idx, const crosstalk_reng_task_t **out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (idx >= ctx->task_cunt) { return LDG_ERR_BOUNDS; }

    *out = &ctx->task[idx];

    return LDG_ERR_AOK;
}

uint32_t crosstalk_reng_task_cunt_get(const crosstalk_reng_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return UINT32_MAX; }

    return ctx->task_cunt;
}

uint32_t crosstalk_reng_note_add(crosstalk_reng_ctx_t *ctx, uint32_t proj, uint64_t addr, const char *text, uint32_t *out)
{
    crosstalk_reng_note_t *note = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (uint32_t)UINT32_MAX;

    if (proj >= ctx->proj_cunt) { return LDG_ERR_BOUNDS; }

    ret = reng_text_ck(text, (uint32_t)CROSSTALK_RENG_TEXT_CAP);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (ctx->note_cunt >= (uint32_t)CROSSTALK_RENG_NOTE_MAX) { return CROSSTALK_RENG_ERR_FULL; }

    note = &ctx->note[ctx->note_cunt];

    memset(note, 0x0, sizeof(*note));

    ret = ldg_str_rbrcpy(note->text, text, (uint64_t)sizeof(note->text));
    if (ret != LDG_ERR_AOK) { return ret; }

    note->proj = proj;
    note->addr = addr;
    note->ms = ldg_time_epoch_ms_get();

    *out = ctx->note_cunt;
    ctx->note_cunt = ctx->note_cunt + 0x1;

    reng_touch(ctx, proj);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_reng_note_remove(crosstalk_reng_ctx_t *ctx, uint32_t idx)
{
    uint32_t proj = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (idx >= ctx->note_cunt) { return LDG_ERR_BOUNDS; }

    proj = ctx->note[idx].proj;

    if (idx + 0x1 < ctx->note_cunt) { memmove(&ctx->note[idx], &ctx->note[idx + 0x1], (uint64_t)(ctx->note_cunt - idx - 0x1) * sizeof(ctx->note[0])); }

    ctx->note_cunt = ctx->note_cunt - 0x1;

    reng_touch(ctx, proj);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_reng_note_get(const crosstalk_reng_ctx_t *ctx, uint32_t idx, const crosstalk_reng_note_t **out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (idx >= ctx->note_cunt) { return LDG_ERR_BOUNDS; }

    *out = &ctx->note[idx];

    return LDG_ERR_AOK;
}

uint32_t crosstalk_reng_note_cunt_get(const crosstalk_reng_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return UINT32_MAX; }

    return ctx->note_cunt;
}

uint32_t crosstalk_reng_mark_add(crosstalk_reng_ctx_t *ctx, uint32_t proj, const char *name, uint64_t addr, uint32_t *out)
{
    crosstalk_reng_mark_t *mark = 0x0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (uint32_t)UINT32_MAX;

    if (proj >= ctx->proj_cunt) { return LDG_ERR_BOUNDS; }

    ret = reng_text_ck(name, (uint32_t)CROSSTALK_RENG_NAME_CAP);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (crosstalk_reng_mark_find(ctx, proj, name, &idx) == LDG_ERR_AOK)
    {
        ctx->mark[idx].addr = addr;
        *out = idx;

        reng_touch(ctx, proj);

        return LDG_ERR_AOK;
    }

    if (ctx->mark_cunt >= (uint32_t)CROSSTALK_RENG_MARK_MAX) { return CROSSTALK_RENG_ERR_FULL; }

    mark = &ctx->mark[ctx->mark_cunt];

    memset(mark, 0x0, sizeof(*mark));

    ret = ldg_str_rbrcpy(mark->name, name, (uint64_t)sizeof(mark->name));
    if (ret != LDG_ERR_AOK) { return ret; }

    mark->proj = proj;
    mark->addr = addr;

    *out = ctx->mark_cunt;
    ctx->mark_cunt = ctx->mark_cunt + 0x1;

    reng_touch(ctx, proj);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_reng_mark_remove(crosstalk_reng_ctx_t *ctx, uint32_t idx)
{
    uint32_t proj = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (idx >= ctx->mark_cunt) { return LDG_ERR_BOUNDS; }

    proj = ctx->mark[idx].proj;

    if (idx + 0x1 < ctx->mark_cunt) { memmove(&ctx->mark[idx], &ctx->mark[idx + 0x1], (uint64_t)(ctx->mark_cunt - idx - 0x1) * sizeof(ctx->mark[0])); }

    ctx->mark_cunt = ctx->mark_cunt - 0x1;

    reng_touch(ctx, proj);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_reng_mark_find(const crosstalk_reng_ctx_t *ctx, uint32_t proj, const char *name, uint32_t *out)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (uint32_t)UINT32_MAX;

    for (idx = 0; idx < ctx->mark_cunt; idx = idx + 0x1)
    {
        if (ctx->mark[idx].proj != proj) { continue; }

        if (reng_str_eq_is(ctx->mark[idx].name, name) != LDG_TRUTH_TRUE) { continue; }

        *out = idx;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t crosstalk_reng_mark_get(const crosstalk_reng_ctx_t *ctx, uint32_t idx, const crosstalk_reng_mark_t **out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (idx >= ctx->mark_cunt) { return LDG_ERR_BOUNDS; }

    *out = &ctx->mark[idx];

    return LDG_ERR_AOK;
}

uint32_t crosstalk_reng_mark_cunt_get(const crosstalk_reng_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return UINT32_MAX; }

    return ctx->mark_cunt;
}

uint32_t crosstalk_reng_sample_push(crosstalk_reng_ctx_t *ctx, uint32_t proj, uint32_t total, uint32_t named, uint32_t ported, uint32_t verified)
{
    const crosstalk_reng_sample_t *last = 0x0;
    crosstalk_reng_sample_t *slot = 0x0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (proj >= ctx->proj_cunt) { return LDG_ERR_BOUNDS; }

    if (crosstalk_reng_sample_last_get(ctx, proj, &last) == LDG_ERR_AOK && last != 0x0) { if (last->total == total && last->named == named && last->ported == ported && last->verified == verified) { return LDG_ERR_AOK; } }

    at = (ctx->sample_hd + ctx->sample_cunt) % (uint32_t)CROSSTALK_RENG_SAMPLE_MAX;

    if (ctx->sample_cunt < (uint32_t)CROSSTALK_RENG_SAMPLE_MAX) { ctx->sample_cunt = ctx->sample_cunt + 0x1; }
    else { ctx->sample_hd = (ctx->sample_hd + 0x1) % (uint32_t)CROSSTALK_RENG_SAMPLE_MAX; }

    slot = &ctx->sample[at];

    memset(slot, 0x0, sizeof(*slot));

    slot->ms = ldg_time_epoch_ms_get();
    slot->proj = proj;
    slot->total = total;
    slot->named = named;
    slot->ported = ported;
    slot->verified = verified;

    reng_touch(ctx, proj);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_reng_sample_get(const crosstalk_reng_ctx_t *ctx, uint32_t idx, const crosstalk_reng_sample_t **out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (idx >= ctx->sample_cunt) { return LDG_ERR_BOUNDS; }

    *out = &ctx->sample[(ctx->sample_hd + idx) % (uint32_t)CROSSTALK_RENG_SAMPLE_MAX];

    return LDG_ERR_AOK;
}

uint32_t crosstalk_reng_sample_last_get(const crosstalk_reng_ctx_t *ctx, uint32_t proj, const crosstalk_reng_sample_t **out)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    for (at = ctx->sample_cunt; at > 0; at = at - 0x1)
    {
        const crosstalk_reng_sample_t *sample = &ctx->sample[(ctx->sample_hd + at - 0x1) % (uint32_t)CROSSTALK_RENG_SAMPLE_MAX];

        if (sample->proj != proj) { continue; }

        *out = sample;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t crosstalk_reng_sample_cunt_get(const crosstalk_reng_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return UINT32_MAX; }

    return ctx->sample_cunt;
}

uint32_t crosstalk_reng_serialize(const crosstalk_reng_ctx_t *ctx, uint8_t *out, uint32_t cap, uint32_t *out_len)
{
    crosstalk_reng_hdr_t hdr = LDG_STRUCT_ZERO_INIT;
    uint64_t need = 0;
    uint64_t at = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (ctx->proj_cunt == 0) { return LDG_ERR_AOK; }

    need = sizeof(hdr) + (uint64_t)ctx->proj_cunt * sizeof(ctx->proj[0]) + (uint64_t)ctx->task_cunt * sizeof(ctx->task[0]) + (uint64_t)ctx->note_cunt * sizeof(ctx->note[0]) + (uint64_t)ctx->mark_cunt * sizeof(ctx->mark[0]) + (uint64_t)ctx->sample_cunt * sizeof(ctx->sample[0]);

    if (need > (uint64_t)UINT32_MAX) { return LDG_ERR_OVERFLOW; }

    *out_len = (uint32_t)need;

    if (out == 0x0) { return LDG_ERR_AOK; }

    if ((uint64_t)cap < need) { return LDG_ERR_BOUNDS; }

    memcpy(hdr.magic, CROSSTALK_RENG_MAGIC, (uint64_t)CROSSTALK_RENG_MAGIC_LEN);

    hdr.rev = (uint32_t)CROSSTALK_RENG_REV;
    hdr.proj_cunt = ctx->proj_cunt;
    hdr.task_cunt = ctx->task_cunt;
    hdr.note_cunt = ctx->note_cunt;
    hdr.mark_cunt = ctx->mark_cunt;
    hdr.sample_cunt = ctx->sample_cunt;
    hdr.sel = ctx->sel;

    memcpy(out + at, &hdr, sizeof(hdr));
    at = at + sizeof(hdr);

    memcpy(out + at, ctx->proj, (uint64_t)ctx->proj_cunt * sizeof(ctx->proj[0]));
    at = at + (uint64_t)ctx->proj_cunt * sizeof(ctx->proj[0]);

    memcpy(out + at, ctx->task, (uint64_t)ctx->task_cunt * sizeof(ctx->task[0]));
    at = at + (uint64_t)ctx->task_cunt * sizeof(ctx->task[0]);

    memcpy(out + at, ctx->note, (uint64_t)ctx->note_cunt * sizeof(ctx->note[0]));
    at = at + (uint64_t)ctx->note_cunt * sizeof(ctx->note[0]);

    memcpy(out + at, ctx->mark, (uint64_t)ctx->mark_cunt * sizeof(ctx->mark[0]));
    at = at + (uint64_t)ctx->mark_cunt * sizeof(ctx->mark[0]);

    for (idx = 0; idx < ctx->sample_cunt; idx = idx + 0x1)
    {
        memcpy(out + at, &ctx->sample[(ctx->sample_hd + idx) % (uint32_t)CROSSTALK_RENG_SAMPLE_MAX], sizeof(ctx->sample[0]));
        at = at + sizeof(ctx->sample[0]);
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_reng_deserialize(crosstalk_reng_ctx_t *ctx, const uint8_t *buff, uint32_t len)
{
    crosstalk_reng_hdr_t hdr = LDG_STRUCT_ZERO_INIT;
    uint64_t need = 0;
    uint64_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || buff == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len < sizeof(hdr)) { return CROSSTALK_RENG_ERR_TRUNC; }

    memcpy(&hdr, buff, sizeof(hdr));

    if (memcmp(hdr.magic, CROSSTALK_RENG_MAGIC, (uint64_t)CROSSTALK_RENG_MAGIC_LEN) != 0) { return CROSSTALK_RENG_ERR_REV; }

    if (hdr.rev != (uint32_t)CROSSTALK_RENG_REV) { return CROSSTALK_RENG_ERR_REV; }

    if (hdr.proj_cunt > (uint32_t)CROSSTALK_RENG_PROJ_MAX || hdr.task_cunt > (uint32_t)CROSSTALK_RENG_TASK_MAX || hdr.note_cunt > (uint32_t)CROSSTALK_RENG_NOTE_MAX || hdr.mark_cunt > (uint32_t)CROSSTALK_RENG_MARK_MAX || hdr.sample_cunt > (uint32_t)CROSSTALK_RENG_SAMPLE_MAX) { return CROSSTALK_RENG_ERR_TRUNC; }

    need = sizeof(hdr) + (uint64_t)hdr.proj_cunt * sizeof(ctx->proj[0]) + (uint64_t)hdr.task_cunt * sizeof(ctx->task[0]) + (uint64_t)hdr.note_cunt * sizeof(ctx->note[0]) + (uint64_t)hdr.mark_cunt * sizeof(ctx->mark[0]) + (uint64_t)hdr.sample_cunt * sizeof(ctx->sample[0]);

    if ((uint64_t)len < need) { return CROSSTALK_RENG_ERR_TRUNC; }

    ret = crosstalk_reng_clear(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    at = sizeof(hdr);

    memcpy(ctx->proj, buff + at, (uint64_t)hdr.proj_cunt * sizeof(ctx->proj[0]));
    at = at + (uint64_t)hdr.proj_cunt * sizeof(ctx->proj[0]);

    memcpy(ctx->task, buff + at, (uint64_t)hdr.task_cunt * sizeof(ctx->task[0]));
    at = at + (uint64_t)hdr.task_cunt * sizeof(ctx->task[0]);

    memcpy(ctx->note, buff + at, (uint64_t)hdr.note_cunt * sizeof(ctx->note[0]));
    at = at + (uint64_t)hdr.note_cunt * sizeof(ctx->note[0]);

    memcpy(ctx->mark, buff + at, (uint64_t)hdr.mark_cunt * sizeof(ctx->mark[0]));
    at = at + (uint64_t)hdr.mark_cunt * sizeof(ctx->mark[0]);

    memcpy(ctx->sample, buff + at, (uint64_t)hdr.sample_cunt * sizeof(ctx->sample[0]));

    ctx->proj_cunt = hdr.proj_cunt;
    ctx->task_cunt = hdr.task_cunt;
    ctx->note_cunt = hdr.note_cunt;
    ctx->mark_cunt = hdr.mark_cunt;
    ctx->sample_cunt = hdr.sample_cunt;
    ctx->sample_hd = 0;
    ctx->sel = hdr.sel != (uint32_t)UINT32_MAX && hdr.sel < hdr.proj_cunt ? hdr.sel : (uint32_t)UINT32_MAX;
    ctx->gen = ctx->gen + 0x1;
    ctx->dirty = LDG_TRUTH_FALSE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "reng: restored; projects: %u32; tasks: %u32; notes: %u32; marks: %u32; samples: %u32; sel: %u32", ctx->proj_cunt, ctx->task_cunt, ctx->note_cunt, ctx->mark_cunt, ctx->sample_cunt, ctx->sel);

    return LDG_ERR_AOK;
}
