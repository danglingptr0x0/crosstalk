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

#define CROSSTALK_CORE_SCAN_SPAN_MAX ((uint64_t)512 * LDG_MIB)
#define CROSSTALK_CORE_SCAN_APP_NAME "Crosstalk"

typedef struct crosstalk_core_scan_ctx
{
    void *provider;
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
