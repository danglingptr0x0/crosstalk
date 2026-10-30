#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/file.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/mem.h>
#include <dangling/str/str.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "re/db.h"

#define DB_CMP_LT 0
#define DB_CMP_EQ 1
#define DB_CMP_GT 2

static uint32_t db_str_cmp(const char *lhs, const char *rhs)
{
    uint32_t at = 0;

    while (lhs[at] != LDG_STR_TERM && lhs[at] == rhs[at]) { at = at + 0x1; }

    if (lhs[at] == rhs[at]) { return DB_CMP_EQ; }

    return (uint8_t)lhs[at] < (uint8_t)rhs[at] ? DB_CMP_LT : DB_CMP_GT;
}

static uint32_t db_key_cmp(uint32_t kind, uint64_t addr, const char *owner, const char *text, const crosstalk_re_db_rec_t *rec)
{
    uint32_t order = 0;

    if (addr != rec->addr) { return addr < rec->addr ? DB_CMP_LT : DB_CMP_GT; }

    if (kind != rec->kind) { return kind < rec->kind ? DB_CMP_LT : DB_CMP_GT; }

    order = db_str_cmp(owner, rec->owner);
    if (order != DB_CMP_EQ) { return order; }

    if (kind != (uint32_t)CROSSTALK_RE_DB_KIND_STRUCT) { return DB_CMP_EQ; }

    return db_str_cmp(text, rec->text);
}

static uint32_t db_slot_find(const crosstalk_re_db_ctx_t *ctx, uint32_t kind, uint64_t addr, const char *owner, const char *text, uint32_t *out_idx, uint8_t *out_hit)
{
    uint32_t lo = 0;
    uint32_t hi = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || owner == 0x0 || text == 0x0 || out_idx == 0x0 || out_hit == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    hi = ctx->rec_cunt;
    *out_hit = LDG_TRUTH_FALSE;

    while (lo < hi)
    {
        uint32_t mid = lo + (hi - lo) / 0x2;
        uint32_t order = db_key_cmp(kind, addr, owner, text, &ctx->rec[mid]);

        if (order == DB_CMP_EQ)
        {
            *out_idx = mid;
            *out_hit = LDG_TRUTH_TRUE;

            return LDG_ERR_AOK;
        }

        if (order == DB_CMP_LT) { hi = mid; }
        else { lo = mid + 0x1; }
    }

    *out_idx = lo;

    return LDG_ERR_AOK;
}

static uint32_t db_text_ck(const char *text)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (text[at] != LDG_STR_TERM)
    {
        if (text[at] == LDG_ASCII_QUOTE || text[at] == '\n' || text[at] == '\r' || text[at] == ';') { return CROSSTALK_RE_DB_ERR_TEXT; }

        at = at + 0x1;
    }

    return LDG_ERR_AOK;
}

static uint32_t db_word_ck(const char *word)
{
    uint32_t at = 0;
    uint32_t ret = db_text_ck(word);

    if (ret != LDG_ERR_AOK) { return ret; }

    while (word[at] != LDG_STR_TERM)
    {
        if (word[at] == ' ' || word[at] == '\t') { return CROSSTALK_RE_DB_ERR_TEXT; }

        at = at + 0x1;
    }

    return LDG_ERR_AOK;
}

static uint32_t db_rec_ck(const crosstalk_re_db_rec_t *rec)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(rec == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (rec->kind == (uint32_t)CROSSTALK_RE_DB_KIND_NONE || rec->kind >= (uint32_t)CROSSTALK_RE_DB_KIND_CUNT) { return CROSSTALK_RE_DB_ERR_KIND; }

    if (rec->by >= (uint32_t)CROSSTALK_RE_DB_BY_CUNT) { return CROSSTALK_RE_DB_ERR_KIND; }

    ret = db_word_ck(rec->owner);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (rec->kind == (uint32_t)CROSSTALK_RE_DB_KIND_NOTE || rec->kind == (uint32_t)CROSSTALK_RE_DB_KIND_CC || rec->kind == (uint32_t)CROSSTALK_RE_DB_KIND_SRC) { return db_text_ck(rec->text); }

    ret = db_word_ck(rec->text);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (rec->kind == (uint32_t)CROSSTALK_RE_DB_KIND_NAME || rec->kind == (uint32_t)CROSSTALK_RE_DB_KIND_STRUCT || rec->kind == (uint32_t)CROSSTALK_RE_DB_KIND_FIELD || rec->kind == (uint32_t)CROSSTALK_RE_DB_KIND_PATCH) { if (rec->text[0] == LDG_STR_TERM) { return CROSSTALK_RE_DB_ERR_TEXT; } }

    if (rec->kind == (uint32_t)CROSSTALK_RE_DB_KIND_FIELD && rec->owner[0] == LDG_STR_TERM) { return CROSSTALK_RE_DB_ERR_TEXT; }

    return LDG_ERR_AOK;
}

static void db_undo_push(crosstalk_re_db_ctx_t *ctx, uint8_t op, const crosstalk_re_db_rec_t *rec, const crosstalk_re_db_rec_t *prev)
{
    crosstalk_re_db_undo_t *slot = 0x0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || rec == 0x0)) { return; }

    at = (ctx->undo_hd + ctx->undo_cunt) % (uint32_t)CROSSTALK_RE_DB_UNDO_MAX;

    if (ctx->undo_cunt < (uint32_t)CROSSTALK_RE_DB_UNDO_MAX) { ctx->undo_cunt = ctx->undo_cunt + 0x1; }
    else { ctx->undo_hd = (ctx->undo_hd + 0x1) % (uint32_t)CROSSTALK_RE_DB_UNDO_MAX; }

    slot = &ctx->undo[at];

    memset(slot, 0x0, sizeof(*slot));

    slot->op = op;
    slot->rec = *rec;

    if (prev != 0x0)
    {
        slot->prev = *prev;
        slot->had_prev = LDG_TRUTH_TRUE;
    }
}

static uint32_t db_insert_at(crosstalk_re_db_ctx_t *ctx, uint32_t idx, const crosstalk_re_db_rec_t *rec)
{
    if (LDG_UNLIKELY(ctx == 0x0 || rec == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->rec_cunt >= (uint32_t)CROSSTALK_RE_DB_REC_MAX) { return CROSSTALK_RE_DB_ERR_FULL; }

    if (idx > ctx->rec_cunt) { return LDG_ERR_BOUNDS; }

    if (idx < ctx->rec_cunt) { memmove(&ctx->rec[idx + 0x1], &ctx->rec[idx], (uint64_t)(ctx->rec_cunt - idx) * sizeof(*rec)); }

    ctx->rec[idx] = *rec;
    ctx->rec_cunt = ctx->rec_cunt + 0x1;

    return LDG_ERR_AOK;
}

static uint32_t db_remove_at(crosstalk_re_db_ctx_t *ctx, uint32_t idx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (idx >= ctx->rec_cunt) { return LDG_ERR_BOUNDS; }

    if (idx + 0x1 < ctx->rec_cunt) { memmove(&ctx->rec[idx], &ctx->rec[idx + 0x1], (uint64_t)(ctx->rec_cunt - idx - 0x1) * sizeof(ctx->rec[0])); }

    ctx->rec_cunt = ctx->rec_cunt - 0x1;

    return LDG_ERR_AOK;
}

static uint32_t db_apply(crosstalk_re_db_ctx_t *ctx, const crosstalk_re_db_rec_t *rec, uint8_t journal)
{
    crosstalk_re_db_rec_t prev = LDG_STRUCT_ZERO_INIT;
    uint32_t idx = 0;
    uint32_t ret = 0;
    uint8_t hit = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0 || rec == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = db_slot_find(ctx, rec->kind, rec->addr, rec->owner, rec->text, &idx, &hit);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (hit == LDG_TRUTH_TRUE)
    {
        prev = ctx->rec[idx];
        ctx->rec[idx] = *rec;
    }
    else
    {
        ret = db_insert_at(ctx, idx, rec);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    if (journal == LDG_TRUTH_TRUE) { db_undo_push(ctx, (uint8_t)CROSSTALK_RE_DB_OP_ADD, rec, hit == LDG_TRUTH_TRUE ? &prev : 0x0); }

    ctx->gen = ctx->gen + 0x1;
    ctx->dirty = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t db_line_wr(ldg_io_file_t *fh, const char *line)
{
    uint64_t len = 0;
    uint64_t wrote = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(fh == 0x0 || line == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (line[len] != LDG_STR_TERM) { len = len + 0x1; }

    ret = ldg_io_file_wr(fh, line, len, &wrote);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (wrote != len) { return LDG_ERR_IO_WR; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_db_init(crosstalk_re_db_ctx_t *ctx)
{
    uint64_t need = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    need = (uint64_t)CROSSTALK_RE_DB_REC_MAX * sizeof(crosstalk_re_db_rec_t);

    ret = ldg_mem_pool_create(0x0, need + (uint64_t)LDG_MEM_POOL_VAR_ALIGN, &ctx->pool);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_mem_pool_alloc(ctx->pool, need, (void **)&ctx->rec);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_db_clear(crosstalk_re_db_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->rec_cunt = 0;
    ctx->undo_hd = 0;
    ctx->undo_cunt = 0;
    ctx->gen = ctx->gen + 0x1;
    ctx->img_hash = 0;
    ctx->img_set = LDG_TRUTH_FALSE;
    ctx->dirty = LDG_TRUTH_FALSE;
    ctx->path[0] = LDG_STR_TERM;
    ctx->img_leaf[0] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_db_shutdown(crosstalk_re_db_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->pool != 0x0) { ret = ldg_mem_pool_destroy(&ctx->pool); }

    memset(ctx, 0x0, sizeof(*ctx));

    return ret;
}

uint8_t crosstalk_re_db_dirty_is(const crosstalk_re_db_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->dirty == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_re_db_rec_cunt_get(const crosstalk_re_db_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return UINT32_MAX; }

    return ctx->rec_cunt;
}

uint32_t crosstalk_re_db_rec_get(const crosstalk_re_db_ctx_t *ctx, uint32_t idx, const crosstalk_re_db_rec_t **out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (idx >= ctx->rec_cunt) { return LDG_ERR_BOUNDS; }

    *out = &ctx->rec[idx];

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_db_rec_find(const crosstalk_re_db_ctx_t *ctx, uint32_t kind, uint64_t addr, const char *owner, const char *text, uint32_t *out)
{
    uint32_t idx = 0;
    uint32_t ret = 0;
    uint8_t hit = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ret = db_slot_find(ctx, kind, addr, owner != 0x0 ? owner : "", text != 0x0 ? text : "", &idx, &hit);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (hit != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_FOUND; }

    *out = idx;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_db_rec_add(crosstalk_re_db_ctx_t *ctx, const crosstalk_re_db_rec_t *rec)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || rec == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ret = db_rec_ck(rec);
    if (ret != LDG_ERR_AOK) { return ret; }

    return db_apply(ctx, rec, LDG_TRUTH_TRUE);
}

uint32_t crosstalk_re_db_rec_remove(crosstalk_re_db_ctx_t *ctx, uint32_t kind, uint64_t addr, const char *owner, const char *text)
{
    crosstalk_re_db_rec_t gone = LDG_STRUCT_ZERO_INIT;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_re_db_rec_find(ctx, kind, addr, owner, text, &idx);
    if (ret != LDG_ERR_AOK) { return ret; }

    gone = ctx->rec[idx];

    ret = db_remove_at(ctx, idx);
    if (ret != LDG_ERR_AOK) { return ret; }

    db_undo_push(ctx, (uint8_t)CROSSTALK_RE_DB_OP_REMOVE, &gone, 0x0);

    ctx->gen = ctx->gen + 0x1;
    ctx->dirty = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_db_undo(crosstalk_re_db_ctx_t *ctx)
{
    crosstalk_re_db_undo_t entry = LDG_STRUCT_ZERO_INIT;
    uint32_t at = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;
    uint8_t hit = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->undo_cunt == 0) { return LDG_ERR_EMPTY; }

    at = (ctx->undo_hd + ctx->undo_cunt - 0x1) % (uint32_t)CROSSTALK_RE_DB_UNDO_MAX;
    entry = ctx->undo[at];
    ctx->undo_cunt = ctx->undo_cunt - 0x1;

    if (entry.op == (uint8_t)CROSSTALK_RE_DB_OP_REMOVE) { return db_apply(ctx, &entry.rec, LDG_TRUTH_FALSE); }

    ret = db_slot_find(ctx, entry.rec.kind, entry.rec.addr, entry.rec.owner, entry.rec.text, &idx, &hit);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (hit == LDG_TRUTH_TRUE) { if (entry.had_prev == LDG_TRUTH_TRUE) { ctx->rec[idx] = entry.prev; }
        else
        {
            ret = db_remove_at(ctx, idx);
            if (ret != LDG_ERR_AOK) { return ret; }
        } }

    ctx->gen = ctx->gen + 0x1;
    ctx->dirty = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_db_name_get(const crosstalk_re_db_ctx_t *ctx, uint64_t addr, const char **out)
{
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    ret = crosstalk_re_db_rec_find(ctx, (uint32_t)CROSSTALK_RE_DB_KIND_NAME, addr, "", "", &idx);
    if (ret != LDG_ERR_AOK) { return ret; }

    *out = ctx->rec[idx].text;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_db_addr_get(const crosstalk_re_db_ctx_t *ctx, const char *name, uint64_t *out)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    for (idx = 0; idx < ctx->rec_cunt; idx = idx + 0x1)
    {
        if (ctx->rec[idx].kind != (uint32_t)CROSSTALK_RE_DB_KIND_NAME) { continue; }

        if (db_str_cmp(ctx->rec[idx].text, name) != DB_CMP_EQ) { continue; }

        *out = ctx->rec[idx].addr;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t crosstalk_re_db_func_find(const crosstalk_re_db_ctx_t *ctx, uint64_t addr, const crosstalk_re_db_rec_t **out)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    for (idx = 0; idx < ctx->rec_cunt; idx = idx + 0x1)
    {
        const crosstalk_re_db_rec_t *rec = &ctx->rec[idx];

        if (rec->kind != (uint32_t)CROSSTALK_RE_DB_KIND_FUNC) { continue; }

        if (rec->addr > addr) { break; }

        if (addr < rec->addr + rec->size)
        {
            *out = rec;

            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t crosstalk_re_db_kind_name_get(uint32_t kind, const char **out)
{
    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (kind == (uint32_t)CROSSTALK_RE_DB_KIND_NAME) { *out = "name"; }
    else if (kind == (uint32_t)CROSSTALK_RE_DB_KIND_NOTE) { *out = "note"; }
    else if (kind == (uint32_t)CROSSTALK_RE_DB_KIND_FUNC) { *out = "func"; }
    else if (kind == (uint32_t)CROSSTALK_RE_DB_KIND_STRUCT) { *out = "struct"; }
    else if (kind == (uint32_t)CROSSTALK_RE_DB_KIND_FIELD) { *out = "field"; }
    else if (kind == (uint32_t)CROSSTALK_RE_DB_KIND_PATCH) { *out = "patch"; }
    else if (kind == (uint32_t)CROSSTALK_RE_DB_KIND_CC) { *out = "cc"; }
    else if (kind == (uint32_t)CROSSTALK_RE_DB_KIND_SRC) { *out = "src"; }
    else if (kind == (uint32_t)CROSSTALK_RE_DB_KIND_VERDICT) { *out = "verdict"; }
    else { return CROSSTALK_RE_DB_ERR_KIND; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_db_kind_parse(const char *name, uint32_t *out)
{
    const char *known = 0x0;
    uint32_t kind = 0;

    if (LDG_UNLIKELY(name == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (uint32_t)CROSSTALK_RE_DB_KIND_NONE;

    for (kind = (uint32_t)CROSSTALK_RE_DB_KIND_NAME; kind < (uint32_t)CROSSTALK_RE_DB_KIND_CUNT; kind = kind + 0x1)
    {
        if (crosstalk_re_db_kind_name_get(kind, &known) != LDG_ERR_AOK) { continue; }

        if (db_str_cmp(known, name) != DB_CMP_EQ) { continue; }

        *out = kind;

        return LDG_ERR_AOK;
    }

    return CROSSTALK_RE_DB_ERR_KIND;
}

uint32_t crosstalk_re_db_by_name_get(uint32_t by, const char **out)
{
    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (by == (uint32_t)CROSSTALK_RE_DB_BY_HAND) { *out = "hand"; }
    else if (by == (uint32_t)CROSSTALK_RE_DB_BY_ELF) { *out = "elf"; }
    else if (by == (uint32_t)CROSSTALK_RE_DB_BY_TRACE) { *out = "trace"; }
    else if (by == (uint32_t)CROSSTALK_RE_DB_BY_IMPORT) { *out = "import"; }
    else { return CROSSTALK_RE_DB_ERR_KIND; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_db_by_parse(const char *name, uint32_t *out)
{
    const char *known = 0x0;
    uint32_t by = 0;

    if (LDG_UNLIKELY(name == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (uint32_t)CROSSTALK_RE_DB_BY_HAND;

    for (by = (uint32_t)CROSSTALK_RE_DB_BY_HAND; by < (uint32_t)CROSSTALK_RE_DB_BY_CUNT; by = by + 0x1)
    {
        if (crosstalk_re_db_by_name_get(by, &known) != LDG_ERR_AOK) { continue; }

        if (db_str_cmp(known, name) != DB_CMP_EQ) { continue; }

        *out = by;

        return LDG_ERR_AOK;
    }

    return CROSSTALK_RE_DB_ERR_KIND;
}

uint32_t crosstalk_re_db_line_fmt(const crosstalk_re_db_rec_t *rec, char *out, uint32_t cap)
{
    const char *by = "";
    const char *by_sep = "";
    int32_t wrote = 0;

    if (LDG_UNLIKELY(rec == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    out[0] = LDG_STR_TERM;

    if (rec->by != (uint32_t)CROSSTALK_RE_DB_BY_HAND)
    {
        if (crosstalk_re_db_by_name_get(rec->by, &by) != LDG_ERR_AOK) { return CROSSTALK_RE_DB_ERR_KIND; }

        by_sep = " ";
    }

    if (rec->kind == (uint32_t)CROSSTALK_RE_DB_KIND_NAME) { wrote = snprintf(out, cap, "re name 0x%" PRIX64 " %.159s%.1s%.7s;", rec->addr, rec->text, by_sep, by); }
    else if (rec->kind == (uint32_t)CROSSTALK_RE_DB_KIND_NOTE) { wrote = snprintf(out, cap, "re note 0x%" PRIX64 " \"%.159s\"%.1s%.7s;", rec->addr, rec->text, by_sep, by); }
    else if (rec->kind == (uint32_t)CROSSTALK_RE_DB_KIND_FUNC) { wrote = snprintf(out, cap, "re func 0x%" PRIX64 " 0x%" PRIX64 "%.1s%.7s;", rec->addr, rec->size, by_sep, by); }
    else if (rec->kind == (uint32_t)CROSSTALK_RE_DB_KIND_STRUCT) { wrote = snprintf(out, cap, "re struct %.159s 0x%" PRIX64 "%.1s%.7s;", rec->text, rec->size, by_sep, by); }
    else if (rec->kind == (uint32_t)CROSSTALK_RE_DB_KIND_FIELD) { wrote = snprintf(out, cap, "re field %.63s 0x%" PRIX64 " 0x%" PRIX64 " %.159s%.1s%.7s;", rec->owner, rec->addr, rec->size, rec->text, by_sep, by); }
    else if (rec->kind == (uint32_t)CROSSTALK_RE_DB_KIND_PATCH) { wrote = snprintf(out, cap, "re patch 0x%" PRIX64 " %.159s%.1s%.7s;", rec->addr, rec->text, by_sep, by); }
    else if (rec->kind == (uint32_t)CROSSTALK_RE_DB_KIND_CC) { wrote = snprintf(out, cap, "re cc %" PRIu64 " \"%.159s\";", rec->addr, rec->text); }
    else if (rec->kind == (uint32_t)CROSSTALK_RE_DB_KIND_SRC) { wrote = snprintf(out, cap, "re src 0x%" PRIX64 " \"%.159s\"%.1s%.7s;", rec->addr, rec->text, by_sep, by); }
    else if (rec->kind == (uint32_t)CROSSTALK_RE_DB_KIND_VERDICT) { wrote = snprintf(out, cap, "re verdict 0x%" PRIX64 " %" PRIu64 " %.159s%.1s%.7s;", rec->addr, rec->size, rec->text, by_sep, by); }
    else { return CROSSTALK_RE_DB_ERR_KIND; }

    if (wrote < 0) { return LDG_ERR_STR_FMT; }

    if ((uint32_t)wrote >= cap) { return LDG_ERR_STR_TRUNC; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_db_img_set(crosstalk_re_db_ctx_t *ctx, uint32_t hash, const char *leaf)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || leaf == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = db_word_ck(leaf);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_str_rbrcpy(ctx->img_leaf, leaf, (uint64_t)sizeof(ctx->img_leaf));
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->img_hash = hash;
    ctx->img_set = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_db_st(crosstalk_re_db_ctx_t *ctx, const char *path)
{
    char line[CROSSTALK_RE_DB_LINE_CAP] = LDG_ARR_ZERO_INIT;
    ldg_io_file_t *fh = 0x0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ret = ldg_io_file_open(path, LDG_IO_WR_ONLY | LDG_IO_CREATE | LDG_IO_TRUNCATE, (uint32_t)CROSSTALK_RE_DB_FILE_MODE, &fh);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (ctx->img_set == LDG_TRUTH_TRUE) { if (snprintf(line, sizeof(line), CROSSTALK_RE_DB_IMG_LINE_FMT, ctx->img_hash, ctx->img_leaf) < 0) { ret = LDG_ERR_STR_FMT; }
        else { ret = db_line_wr(fh, line); } }

    for (idx = 0; ret == LDG_ERR_AOK && idx < ctx->rec_cunt; idx = idx + 0x1)
    {
        ret = crosstalk_re_db_line_fmt(&ctx->rec[idx], line, (uint32_t)sizeof(line));
        if (ret != LDG_ERR_AOK) { break; }

        ret = db_line_wr(fh, line);
        if (ret != LDG_ERR_AOK) { break; }

        ret = db_line_wr(fh, CROSSTALK_RE_DB_EOL);
    }

    if (ldg_io_file_close(fh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re db close failed"); }

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re db: st failed; path: %.127s; err: %u32", path, ret);

        return ret;
    }

    if (ldg_str_rbrcpy(ctx->path, path, (uint64_t)sizeof(ctx->path)) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re db path truncated"); }

    ctx->dirty = LDG_TRUTH_FALSE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "re db: st; path: %.127s; records: %u32; img: 0x%08X32", path, ctx->rec_cunt, ctx->img_hash);

    return LDG_ERR_AOK;
}
