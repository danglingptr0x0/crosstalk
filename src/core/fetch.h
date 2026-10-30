#ifndef CROSSTALK_CORE_FETCH_H
#define CROSSTALK_CORE_FETCH_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/io/path.h>

#include "hdl/proc.h"

#define CROSSTALK_CORE_FETCH_URL_LEAD "https:/" "/"
#define CROSSTALK_CORE_FETCH_URL_CAP 512
#define CROSSTALK_CORE_FETCH_NAME_CAP 128
#define CROSSTALK_CORE_FETCH_TOP_CAP 64
#define CROSSTALK_CORE_FETCH_REASON_CAP 256
#define CROSSTALK_CORE_FETCH_DIR_MAX 128
#define CROSSTALK_CORE_FETCH_REL_CAP 256
#define CROSSTALK_CORE_FETCH_ENV_CAP 16384
#define CROSSTALK_CORE_FETCH_FILE_MAX 256
#define CROSSTALK_CORE_FETCH_DEF_MAX 2048
#define CROSSTALK_CORE_FETCH_DEF_SLOT_CAP 4096
#define CROSSTALK_CORE_FETCH_DEF_SLOT_MASK (CROSSTALK_CORE_FETCH_DEF_SLOT_CAP - 0x1)
#define CROSSTALK_CORE_FETCH_IDENT_CAP 64
#define CROSSTALK_CORE_FETCH_SCAN_CAP (1024ULL * LDG_KIB)

typedef struct crosstalk_core_fetch_def
{
    char name[CROSSTALK_CORE_FETCH_IDENT_CAP];
    uint32_t hash;
    uint32_t file_idx;
} crosstalk_core_fetch_def_t;

typedef enum crosstalk_core_fetch_phase
{
    CROSSTALK_CORE_FETCH_PHASE_NONE = 0,
    CROSSTALK_CORE_FETCH_PHASE_RUN,
    CROSSTALK_CORE_FETCH_PHASE_DONE,
    CROSSTALK_CORE_FETCH_PHASE_FAIL
} crosstalk_core_fetch_phase_t;

typedef struct crosstalk_core_fetch_ctx
{
    crosstalk_hdl_proc_ctx_t proc;
    char queue[CROSSTALK_CORE_FETCH_DIR_MAX][CROSSTALK_CORE_FETCH_REL_CAP];
    char file[CROSSTALK_CORE_FETCH_FILE_MAX][CROSSTALK_CORE_FETCH_REL_CAP];
    crosstalk_core_fetch_def_t def[CROSSTALK_CORE_FETCH_DEF_MAX];
    uint32_t def_slot[CROSSTALK_CORE_FETCH_DEF_SLOT_CAP];
    uint32_t file_order[CROSSTALK_CORE_FETCH_FILE_MAX];
    uint8_t file_kept[CROSSTALK_CORE_FETCH_FILE_MAX];
    uint8_t file_pkg[CROSSTALK_CORE_FETCH_FILE_MAX];
    char scan[CROSSTALK_CORE_FETCH_SCAN_CAP];
    char env[CROSSTALK_CORE_FETCH_ENV_CAP];
    char repo_dir[LDG_IO_PATH_MAX];
    char top[CROSSTALK_CORE_FETCH_TOP_CAP];
    char reason[CROSSTALK_CORE_FETCH_REASON_CAP];
    uint32_t file_cunt;
    uint32_t def_cunt;
    uint32_t phase;
    uint32_t exit_code;
} crosstalk_core_fetch_ctx_t;

uint8_t crosstalk_core_fetch_url_is(const char *text);
uint8_t crosstalk_core_fetch_name_valid_is(const char *name);
uint32_t crosstalk_core_fetch_start(crosstalk_core_fetch_ctx_t *ctx, const char *parent, const char *url, const char *top);
uint32_t crosstalk_core_fetch_tick(crosstalk_core_fetch_ctx_t *ctx, uint32_t *out_phase);
uint32_t crosstalk_core_fetch_files_get(crosstalk_core_fetch_ctx_t *ctx, char *out, uint64_t cap, uint32_t *out_cunt);
uint32_t crosstalk_core_fetch_stop(crosstalk_core_fetch_ctx_t *ctx);

#endif
