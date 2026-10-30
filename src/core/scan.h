#ifndef CROSSTALK_CORE_SCAN_H
#define CROSSTALK_CORE_SCAN_H

#include <stdint.h>

#include <dangling/core/types.h>
#include <dangling/io/path.h>

#define CROSSTALK_CORE_SCAN_ERR_SPAWN 1000
#define CROSSTALK_CORE_SCAN_ERR_EXIT 1001
#define CROSSTALK_CORE_SCAN_ERR_PROVIDER 1002
#define CROSSTALK_CORE_SCAN_ERR_SPAN 1003

#define CROSSTALK_CORE_SCAN_VERDICT_CLEAN 0
#define CROSSTALK_CORE_SCAN_VERDICT_DETECTED 1

#define CROSSTALK_CORE_SCAN_EXE_UNIX "clamdscan"
#define CROSSTALK_CORE_SCAN_ARG_QUIET "--quiet"
#define CROSSTALK_CORE_SCAN_ARG_STDOUT "--stdout"
#define CROSSTALK_CORE_SCAN_ARG_MAX 8
#define CROSSTALK_CORE_SCAN_WAIT_FLAGS 0
#define CROSSTALK_CORE_SCAN_EXIT_CLEAN 0
#define CROSSTALK_CORE_SCAN_EXIT_FOUND 1
#define CROSSTALK_CORE_SCAN_SPAN_MAX ((uint64_t)512 * LDG_MIB)
#define CROSSTALK_CORE_SCAN_TREE_DEPTH_MAX 24
#define CROSSTALK_CORE_SCAN_TREE_ROOT 0
#define CROSSTALK_CORE_SCAN_DIR_SELF "."
#define CROSSTALK_CORE_SCAN_DIR_UP ".."
#define CROSSTALK_CORE_SCAN_APP_NAME "Crosstalk"
#define CROSSTALK_CORE_SCAN_AMSI_DLL "amsi.dll"
#define CROSSTALK_CORE_SCAN_AMSI_INIT "AmsiInitialize"
#define CROSSTALK_CORE_SCAN_AMSI_UNINIT "AmsiUninitialize"
#define CROSSTALK_CORE_SCAN_AMSI_OPEN "AmsiOpenSession"
#define CROSSTALK_CORE_SCAN_AMSI_CLOSE "AmsiCloseSession"
#define CROSSTALK_CORE_SCAN_AMSI_SCAN "AmsiScanBuffer"
#define CROSSTALK_CORE_SCAN_AMSI_DETECTED 32768

typedef struct crosstalk_core_scan_ctx
{
    void *provider;
    void *ctx_handle;
    void *init_cb;
    void *uninit_cb;
    void *open_cb;
    void *close_cb;
    void *scan_cb;
    char exe[LDG_IO_PATH_MAX];
    uint64_t runs;
    uint64_t hits;
    uint8_t ok;
    uint8_t pudding[7];
} crosstalk_core_scan_ctx_t;

uint32_t crosstalk_core_scan_init(crosstalk_core_scan_ctx_t *ctx);
uint32_t crosstalk_core_scan_shutdown(crosstalk_core_scan_ctx_t *ctx);
uint32_t crosstalk_core_scan_file(crosstalk_core_scan_ctx_t *ctx, const char *path, uint32_t *out_verdict);
uint8_t crosstalk_core_scan_ready_is(const crosstalk_core_scan_ctx_t *ctx);

#endif
