#ifndef CROSSTALK_RE_H
#define CROSSTALK_RE_H

#include <stdint.h>

#include "re/cc.h"
#include "re/db.h"
#include "re/diff.h"
#include "re/img.h"

#define CROSSTALK_RE_DB_SIBLING_EXT ".xts"

typedef struct crosstalk_re_ctx
{
    crosstalk_re_img_ctx_t img;
    crosstalk_re_db_ctx_t db;
    crosstalk_re_diff_ctx_t diff;
    crosstalk_re_cc_ctx_t cc;
    crosstalk_hdl_elf_ctx_t cand_elf;
    uint32_t db_gen_seen;
    uint8_t ok;
    uint8_t pudding[3];
} crosstalk_re_ctx_t;

uint32_t crosstalk_re_init(crosstalk_re_ctx_t *ctx);
uint32_t crosstalk_re_shutdown(crosstalk_re_ctx_t *ctx);
uint32_t crosstalk_re_db_path_build(const char *img_path, char *out, uint32_t cap);
uint32_t crosstalk_re_leaf_get(const char *path, char *out, uint32_t cap);

#endif
