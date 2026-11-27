#ifndef CROSSTALK_RE_H
#define CROSSTALK_RE_H

#include <stdint.h>

#include "re/cc.h"
#include "re/db.h"
#include "re/diff.h"
#include "re/img.h"

#define CROSSTALK_RE_DB_SIBLING_EXT ".aww"

#define CROSSTALK_RE_STAT_EXE_MAX 64
#define CROSSTALK_RE_STAT_NAME_CAP 64
#define CROSSTALK_RE_STAT_MAGIC 0x31534552U
#define CROSSTALK_RE_STAT_REV 1
#define CROSSTALK_RE_STAT_HDR_BYTES 12
#define CROSSTALK_RE_STAT_BLK_HDR_BYTES 8
#define CROSSTALK_RE_STAT_BLK_TOTALS 1
#define CROSSTALK_RE_STAT_BLK_EXES 2
#define CROSSTALK_RE_STAT_BLK_CUNT 2
#define CROSSTALK_RE_STAT_HDR_MAGIC 0
#define CROSSTALK_RE_STAT_HDR_REV 1
#define CROSSTALK_RE_STAT_HDR_BLK_CUNT 2
#define CROSSTALK_RE_STAT_BLK_TAG 0
#define CROSSTALK_RE_STAT_BLK_SPAN 1
#define CROSSTALK_RE_STAT_TOTALS_WORDS 3
#define CROSSTALK_RE_STAT_EXE_WORDS 3
#define CROSSTALK_RE_STAT_GAP_MAX 4000
#define CROSSTALK_RE_STAT_PUDDING_SIZE 7
#define CROSSTALK_RE_STAT_EXE_BYTES (CROSSTALK_RE_STAT_NAME_CAP + CROSSTALK_RE_STAT_EXE_WORDS * 8)
#define CROSSTALK_RE_STAT_BLOB_CAP (CROSSTALK_RE_STAT_HDR_BYTES + CROSSTALK_RE_STAT_BLK_CUNT * CROSSTALK_RE_STAT_BLK_HDR_BYTES + CROSSTALK_RE_STAT_TOTALS_WORDS * 8 + 4 + CROSSTALK_RE_STAT_EXE_MAX * CROSSTALK_RE_STAT_EXE_BYTES)

typedef struct crosstalk_re_stat_exe
{
    char name[CROSSTALK_RE_STAT_NAME_CAP];
    uint64_t attach_time;
    uint64_t attaches;
    uint64_t last_at;
} crosstalk_re_stat_exe_t;

typedef struct crosstalk_re_stat
{
    crosstalk_re_stat_exe_t exe[CROSSTALK_RE_STAT_EXE_MAX];
    uint64_t attach_time;
    uint64_t sesh_cunt;
    uint64_t first_at;
    uint64_t step_at;
    uint32_t exe_cunt;
    uint32_t curr;
    uint8_t live_prev;
    uint8_t pudding[CROSSTALK_RE_STAT_PUDDING_SIZE];
} crosstalk_re_stat_t;

typedef struct crosstalk_re_ctx
{
    crosstalk_re_img_ctx_t img;
    crosstalk_re_db_ctx_t db;
    crosstalk_re_diff_ctx_t diff;
    uint8_t diff_pudding[16];
    crosstalk_re_cc_ctx_t cc;
    crosstalk_hdl_elf_ctx_t cand_elf;
    uint32_t db_gen_seen;
    uint8_t ok;
    uint8_t pudding[35];
} crosstalk_re_ctx_t;

uint32_t crosstalk_re_init(crosstalk_re_ctx_t *ctx);
uint32_t crosstalk_re_shutdown(crosstalk_re_ctx_t *ctx);
uint32_t crosstalk_re_db_path_build(const char *img_path, char *out, uint32_t cap);
uint32_t crosstalk_re_leaf_get(const char *path, char *out, uint32_t cap);
uint32_t crosstalk_re_stat_init(crosstalk_re_stat_t *stat);
uint32_t crosstalk_re_stat_step(crosstalk_re_stat_t *stat, uint8_t live_is, uint64_t now);
uint32_t crosstalk_re_stat_exe_open(crosstalk_re_stat_t *stat, const char *name, uint64_t now);
uint32_t crosstalk_re_stat_exe_close(crosstalk_re_stat_t *stat);
uint32_t crosstalk_re_stat_serialize(const crosstalk_re_stat_t *stat, uint8_t *out, uint32_t cap, uint32_t *out_len);
uint32_t crosstalk_re_stat_deserialize(crosstalk_re_stat_t *stat, const uint8_t *buff, uint32_t len);

#endif
