#ifndef CROSSTALK_RENG_H
#define CROSSTALK_RENG_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/io/path.h>

#define CROSSTALK_RENG_ERR_FULL 819
#define CROSSTALK_RENG_ERR_REV 820
#define CROSSTALK_RENG_ERR_TRUNC 821
#define CROSSTALK_RENG_ERR_TEXT 822

#define CROSSTALK_RENG_PROJ_MAX 16
#define CROSSTALK_RENG_TASK_MAX 256
#define CROSSTALK_RENG_NOTE_MAX 256
#define CROSSTALK_RENG_MARK_MAX 128
#define CROSSTALK_RENG_SAMPLE_MAX 512
#define CROSSTALK_RENG_NAME_CAP 64
#define CROSSTALK_RENG_TEXT_CAP 160
#define CROSSTALK_RENG_REV 1
#define CROSSTALK_RENG_MAGIC "RENG"
#define CROSSTALK_RENG_MAGIC_LEN 4

#define CROSSTALK_RENG_KIND_DESIGN 0
#define CROSSTALK_RENG_KIND_ZONE 1
#define CROSSTALK_RENG_KIND_BOARD 2
#define CROSSTALK_RENG_KIND_REVERSE 3
#define CROSSTALK_RENG_KIND_CUNT 4

typedef struct crosstalk_reng_proj
{
    char name[CROSSTALK_RENG_NAME_CAP];
    char path[LDG_IO_PATH_MAX];
    uint64_t created_ms;
    uint64_t updated_ms;
    uint32_t kind;
    uint32_t img_hash;
} crosstalk_reng_proj_t;

typedef struct crosstalk_reng_task
{
    char text[CROSSTALK_RENG_TEXT_CAP];
    uint64_t created_ms;
    uint64_t done_ms;
    uint32_t proj;
    uint8_t done;
    uint8_t pudding[3];
} crosstalk_reng_task_t;

typedef struct crosstalk_reng_note
{
    char text[CROSSTALK_RENG_TEXT_CAP];
    uint64_t addr;
    uint64_t ms;
    uint32_t proj;
    uint8_t pudding[4];
} crosstalk_reng_note_t;

typedef struct crosstalk_reng_mark
{
    char name[CROSSTALK_RENG_NAME_CAP];
    uint64_t addr;
    uint32_t proj;
    uint8_t pudding[4];
} crosstalk_reng_mark_t;

typedef struct crosstalk_reng_sample
{
    uint64_t ms;
    uint32_t proj;
    uint32_t total;
    uint32_t named;
    uint32_t ported;
    uint32_t verified;
    uint8_t pudding[4];
} crosstalk_reng_sample_t;

typedef struct crosstalk_reng_hdr
{
    uint8_t magic[CROSSTALK_RENG_MAGIC_LEN];
    uint32_t rev;
    uint32_t proj_cunt;
    uint32_t task_cunt;
    uint32_t note_cunt;
    uint32_t mark_cunt;
    uint32_t sample_cunt;
    uint32_t sel;
} crosstalk_reng_hdr_t;

typedef struct crosstalk_reng_ctx
{
    crosstalk_reng_proj_t proj[CROSSTALK_RENG_PROJ_MAX];
    crosstalk_reng_task_t task[CROSSTALK_RENG_TASK_MAX];
    crosstalk_reng_note_t note[CROSSTALK_RENG_NOTE_MAX];
    crosstalk_reng_mark_t mark[CROSSTALK_RENG_MARK_MAX];
    crosstalk_reng_sample_t sample[CROSSTALK_RENG_SAMPLE_MAX];
    uint32_t proj_cunt;
    uint32_t task_cunt;
    uint32_t note_cunt;
    uint32_t mark_cunt;
    uint32_t sample_cunt;
    uint32_t sample_hd;
    uint32_t sel;
    uint32_t gen;
    uint8_t dirty;
    uint8_t ok;
    uint8_t pudding[6];
} crosstalk_reng_ctx_t;

uint32_t crosstalk_reng_init(crosstalk_reng_ctx_t *ctx);
uint32_t crosstalk_reng_shutdown(crosstalk_reng_ctx_t *ctx);
uint32_t crosstalk_reng_clear(crosstalk_reng_ctx_t *ctx);
uint32_t crosstalk_reng_proj_add(crosstalk_reng_ctx_t *ctx, const char *name, uint32_t kind, const char *path, uint32_t img_hash, uint32_t *out);
uint32_t crosstalk_reng_proj_find(const crosstalk_reng_ctx_t *ctx, const char *name, uint32_t *out);
uint32_t crosstalk_reng_proj_hash_find(const crosstalk_reng_ctx_t *ctx, uint32_t img_hash, uint32_t *out);
uint32_t crosstalk_reng_proj_get(const crosstalk_reng_ctx_t *ctx, uint32_t idx, const crosstalk_reng_proj_t **out);
uint32_t crosstalk_reng_proj_cunt_get(const crosstalk_reng_ctx_t *ctx);
uint32_t crosstalk_reng_proj_remove(crosstalk_reng_ctx_t *ctx, uint32_t idx);
uint32_t crosstalk_reng_sel_set(crosstalk_reng_ctx_t *ctx, uint32_t idx);
uint32_t crosstalk_reng_sel_get(const crosstalk_reng_ctx_t *ctx);
uint32_t crosstalk_reng_task_add(crosstalk_reng_ctx_t *ctx, uint32_t proj, const char *text, uint32_t *out);
uint32_t crosstalk_reng_task_done_set(crosstalk_reng_ctx_t *ctx, uint32_t idx, uint8_t done);
uint32_t crosstalk_reng_task_remove(crosstalk_reng_ctx_t *ctx, uint32_t idx);
uint32_t crosstalk_reng_task_get(const crosstalk_reng_ctx_t *ctx, uint32_t idx, const crosstalk_reng_task_t **out);
uint32_t crosstalk_reng_task_cunt_get(const crosstalk_reng_ctx_t *ctx);
uint32_t crosstalk_reng_note_add(crosstalk_reng_ctx_t *ctx, uint32_t proj, uint64_t addr, const char *text, uint32_t *out);
uint32_t crosstalk_reng_note_remove(crosstalk_reng_ctx_t *ctx, uint32_t idx);
uint32_t crosstalk_reng_note_get(const crosstalk_reng_ctx_t *ctx, uint32_t idx, const crosstalk_reng_note_t **out);
uint32_t crosstalk_reng_note_cunt_get(const crosstalk_reng_ctx_t *ctx);
uint32_t crosstalk_reng_mark_add(crosstalk_reng_ctx_t *ctx, uint32_t proj, const char *name, uint64_t addr, uint32_t *out);
uint32_t crosstalk_reng_mark_remove(crosstalk_reng_ctx_t *ctx, uint32_t idx);
uint32_t crosstalk_reng_mark_find(const crosstalk_reng_ctx_t *ctx, uint32_t proj, const char *name, uint32_t *out);
uint32_t crosstalk_reng_mark_get(const crosstalk_reng_ctx_t *ctx, uint32_t idx, const crosstalk_reng_mark_t **out);
uint32_t crosstalk_reng_mark_cunt_get(const crosstalk_reng_ctx_t *ctx);
uint32_t crosstalk_reng_sample_push(crosstalk_reng_ctx_t *ctx, uint32_t proj, uint32_t total, uint32_t named, uint32_t ported, uint32_t verified);
uint32_t crosstalk_reng_sample_get(const crosstalk_reng_ctx_t *ctx, uint32_t idx, const crosstalk_reng_sample_t **out);
uint32_t crosstalk_reng_sample_last_get(const crosstalk_reng_ctx_t *ctx, uint32_t proj, const crosstalk_reng_sample_t **out);
uint32_t crosstalk_reng_sample_cunt_get(const crosstalk_reng_ctx_t *ctx);
uint32_t crosstalk_reng_serialize(const crosstalk_reng_ctx_t *ctx, uint8_t *out, uint32_t cap, uint32_t *out_len);
uint32_t crosstalk_reng_deserialize(crosstalk_reng_ctx_t *ctx, const uint8_t *buff, uint32_t len);
uint32_t crosstalk_reng_kind_name_get(uint32_t kind, const char **out);
uint32_t crosstalk_reng_kind_parse(const char *name, uint32_t *out);
uint8_t crosstalk_reng_dirty_is(const crosstalk_reng_ctx_t *ctx);

#endif
