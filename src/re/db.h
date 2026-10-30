#ifndef CROSSTALK_RE_DB_H
#define CROSSTALK_RE_DB_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/io/path.h>
#include <dangling/mem/alloc.h>

#define CROSSTALK_RE_DB_ERR_KIND 806
#define CROSSTALK_RE_DB_ERR_FULL 807
#define CROSSTALK_RE_DB_ERR_TEXT 808
#define CROSSTALK_RE_DB_ERR_IMG 809

#define CROSSTALK_RE_DB_KIND_NONE 0
#define CROSSTALK_RE_DB_KIND_NAME 1
#define CROSSTALK_RE_DB_KIND_NOTE 2
#define CROSSTALK_RE_DB_KIND_FUNC 3
#define CROSSTALK_RE_DB_KIND_STRUCT 4
#define CROSSTALK_RE_DB_KIND_FIELD 5
#define CROSSTALK_RE_DB_KIND_PATCH 6
#define CROSSTALK_RE_DB_KIND_CC 7
#define CROSSTALK_RE_DB_KIND_SRC 8
#define CROSSTALK_RE_DB_KIND_VERDICT 9
#define CROSSTALK_RE_DB_KIND_CUNT 10

#define CROSSTALK_RE_DB_BY_HAND 0
#define CROSSTALK_RE_DB_BY_ELF 1
#define CROSSTALK_RE_DB_BY_TRACE 2
#define CROSSTALK_RE_DB_BY_IMPORT 3
#define CROSSTALK_RE_DB_BY_CUNT 4

#define CROSSTALK_RE_DB_OP_ADD 0
#define CROSSTALK_RE_DB_OP_REMOVE 1

#define CROSSTALK_RE_DB_REC_MAX 32768
#define CROSSTALK_RE_DB_NAME_CAP 64
#define CROSSTALK_RE_DB_TEXT_CAP 160
#define CROSSTALK_RE_DB_UNDO_MAX 256
#define CROSSTALK_RE_DB_LINE_CAP 512
#define CROSSTALK_RE_DB_FILE_MODE 0644
#define CROSSTALK_RE_DB_VERB "re"
#define CROSSTALK_RE_DB_EXT ".xts"
#define CROSSTALK_RE_DB_EOL "\n"
#define CROSSTALK_RE_DB_IMG_LINE_FMT "re img 0x%08X %.63s;\n"
#define CROSSTALK_RE_DB_STEP_MIN 1

typedef struct crosstalk_re_db_rec
{
    uint64_t addr;
    uint64_t size;
    char owner[CROSSTALK_RE_DB_NAME_CAP];
    char text[CROSSTALK_RE_DB_TEXT_CAP];
    uint32_t kind;
    uint32_t by;
} crosstalk_re_db_rec_t;

typedef struct crosstalk_re_db_undo
{
    crosstalk_re_db_rec_t rec;
    crosstalk_re_db_rec_t prev;
    uint8_t op;
    uint8_t had_prev;
    uint8_t pudding[6];
} crosstalk_re_db_undo_t;

typedef struct crosstalk_re_db_ctx
{
    ldg_mem_pool_t *pool;
    crosstalk_re_db_rec_t *rec;
    crosstalk_re_db_undo_t undo[CROSSTALK_RE_DB_UNDO_MAX];
    char path[LDG_IO_PATH_MAX];
    char img_leaf[CROSSTALK_RE_DB_NAME_CAP];
    uint32_t img_hash;
    uint32_t rec_cunt;
    uint32_t undo_hd;
    uint32_t undo_cunt;
    uint32_t gen;
    uint8_t img_set;
    uint8_t dirty;
    uint8_t ok;
    uint8_t pudding[1];
} crosstalk_re_db_ctx_t;

uint32_t crosstalk_re_db_init(crosstalk_re_db_ctx_t *ctx);
uint32_t crosstalk_re_db_shutdown(crosstalk_re_db_ctx_t *ctx);
uint32_t crosstalk_re_db_clear(crosstalk_re_db_ctx_t *ctx);
uint32_t crosstalk_re_db_rec_add(crosstalk_re_db_ctx_t *ctx, const crosstalk_re_db_rec_t *rec);
uint32_t crosstalk_re_db_rec_remove(crosstalk_re_db_ctx_t *ctx, uint32_t kind, uint64_t addr, const char *owner, const char *text);
uint32_t crosstalk_re_db_rec_find(const crosstalk_re_db_ctx_t *ctx, uint32_t kind, uint64_t addr, const char *owner, const char *text, uint32_t *out);
uint32_t crosstalk_re_db_rec_get(const crosstalk_re_db_ctx_t *ctx, uint32_t idx, const crosstalk_re_db_rec_t **out);
uint32_t crosstalk_re_db_rec_cunt_get(const crosstalk_re_db_ctx_t *ctx);
uint32_t crosstalk_re_db_name_get(const crosstalk_re_db_ctx_t *ctx, uint64_t addr, const char **out);
uint32_t crosstalk_re_db_addr_get(const crosstalk_re_db_ctx_t *ctx, const char *name, uint64_t *out);
uint32_t crosstalk_re_db_func_find(const crosstalk_re_db_ctx_t *ctx, uint64_t addr, const crosstalk_re_db_rec_t **out);
uint32_t crosstalk_re_db_undo(crosstalk_re_db_ctx_t *ctx);
uint32_t crosstalk_re_db_line_fmt(const crosstalk_re_db_rec_t *rec, char *out, uint32_t cap);
uint32_t crosstalk_re_db_st(crosstalk_re_db_ctx_t *ctx, const char *path);
uint32_t crosstalk_re_db_img_set(crosstalk_re_db_ctx_t *ctx, uint32_t hash, const char *leaf);
uint32_t crosstalk_re_db_kind_name_get(uint32_t kind, const char **out);
uint32_t crosstalk_re_db_kind_parse(const char *name, uint32_t *out);
uint32_t crosstalk_re_db_by_name_get(uint32_t by, const char **out);
uint32_t crosstalk_re_db_by_parse(const char *name, uint32_t *out);
uint8_t crosstalk_re_db_dirty_is(const crosstalk_re_db_ctx_t *ctx);

#endif
