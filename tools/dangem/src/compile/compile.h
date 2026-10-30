#ifndef DANGEM_COMPILE_H
#define DANGEM_COMPILE_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/io/path.h>
#include <dangling/sys/proc.h>

#include "compile/list.h"

#define DANGEM_COMPILE_JOB_MAX 64
#define DANGEM_COMPILE_ERR_CAP (16 * LDG_KIB)
#define DANGEM_COMPILE_RSP_CAP (1024 * LDG_KIB)
#define DANGEM_COMPILE_ARG_CAP 8
#define DANGEM_COMPILE_RELEASE_CAP 64
#define DANGEM_COMPILE_STAMP_CAP 128
#define DANGEM_COMPILE_DRAIN_CAP 4096
#define DANGEM_COMPILE_FILE_MODE 0644
#define DANGEM_COMPILE_DIR_MODE 0755
#define DANGEM_COMPILE_STEP 1
#define DANGEM_COMPILE_LIST_DIR "dangem"
#define DANGEM_COMPILE_LIST_PREFIX "build-"
#define DANGEM_COMPILE_LIST_EXT ".lst"
#define DANGEM_COMPILE_RELEASE_FILE "RELEASE"
#define DANGEM_COMPILE_STAMP_FILE "stamp"
#define DANGEM_COMPILE_OBJ_DIR "obj"
#define DANGEM_COMPILE_SRC_DIR "src"
#define DANGEM_COMPILE_LDG_DIR "ldg"
#define DANGEM_COMPILE_GEN_DIR "gen"
#define DANGEM_COMPILE_HIDDEN_MARK '.'
#define DANGEM_COMPILE_WALK_DEPTH_FIRST 0
#define DANGEM_COMPILE_WALK_DEPTH_MAX 16
#define DANGEM_COMPILE_CFLAGS_RSP "cflags.rsp"
#define DANGEM_COMPILE_LINK_RSP "link.rsp"
#define DANGEM_COMPILE_CC_DIR "res/utils/cc"
#define DANGEM_COMPILE_CC_BIN "bin"
#define DANGEM_COMPILE_CC_NAME "clang"
#define DANGEM_COMPILE_OBJ_EXT ".o"
#define DANGEM_COMPILE_RC_EXT ".rc"
#define DANGEM_COMPILE_LINKER_FLAG "-fuse-ld=lld"
#define DANGEM_COMPILE_MARCH_FLAG "-march=native"
#define DANGEM_COMPILE_RPATH_FLAG "-Wl,-rpath,$ORIGIN:$ORIGIN/ldg/lib"

typedef enum dangem_compile_phase
{
    DANGEM_COMPILE_PHASE_IDLE = 0,
    DANGEM_COMPILE_PHASE_COMPILE,
    DANGEM_COMPILE_PHASE_LINK,
    DANGEM_COMPILE_PHASE_DONE,
    DANGEM_COMPILE_PHASE_FAIL
} dangem_compile_phase_t;

typedef struct dangem_compile_job
{
    ldg_sys_proc_t proc;
    char err[DANGEM_COMPILE_ERR_CAP];
    uint32_t err_len;
    uint32_t item;
    uint8_t live;
    uint8_t pudding[7];
} dangem_compile_job_t;

typedef struct dangem_compile
{
    dangem_compile_list_t list;
    dangem_compile_job_t job[DANGEM_COMPILE_JOB_MAX];
    char rsp[DANGEM_COMPILE_RSP_CAP];
    char fail_text[DANGEM_COMPILE_ERR_CAP];
    char fail_src[LDG_IO_PATH_MAX];
    char root[LDG_IO_PATH_MAX];
    char build_dir[LDG_IO_PATH_MAX];
    char obj_dir[LDG_IO_PATH_MAX];
    char cc[LDG_IO_PATH_MAX];
    char cflags_rsp[LDG_IO_PATH_MAX];
    char link_rsp[LDG_IO_PATH_MAX];
    char out_path[LDG_IO_PATH_MAX];
    char stamp_path[LDG_IO_PATH_MAX];
    char release[DANGEM_COMPILE_RELEASE_CAP];
    uint64_t start_ms;
    uint64_t end_ms;
    uint64_t prev_ms;
    uint32_t fail_len;
    uint32_t job_cunt;
    uint32_t live_cunt;
    uint32_t item_next;
    uint32_t src_done;
    uint32_t src_total;
    uint32_t phase;
    uint8_t list_ok;
    uint8_t cc_ok;
    uint8_t src_newer;
    uint8_t pudding[1];
} dangem_compile_t;

uint32_t dangem_compile_init(dangem_compile_t *build, const char *root);
uint32_t dangem_compile_start(dangem_compile_t *build);
uint32_t dangem_compile_pump(dangem_compile_t *build);
uint8_t dangem_compile_stale_is(const dangem_compile_t *build);
uint8_t dangem_compile_busy_is(const dangem_compile_t *build);

#endif
