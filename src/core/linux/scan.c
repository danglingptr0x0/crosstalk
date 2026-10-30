#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "core/scan.h"

uint32_t crosstalk_core_scan_init(crosstalk_core_scan_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scan: no antimalware provider on this platform");

    return CROSSTALK_CORE_SCAN_ERR_PROVIDER;
}

uint32_t crosstalk_core_scan_shutdown(crosstalk_core_scan_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_scan_file(crosstalk_core_scan_ctx_t *ctx, const char *path, uint32_t *out_verdict)
{
    if (LDG_UNLIKELY(ctx == 0x0 || path == 0x0 || out_verdict == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_verdict = (uint32_t)CROSSTALK_CORE_SCAN_VERDICT_CLEAN;

    return LDG_ERR_NOT_INIT;
}

uint8_t crosstalk_core_scan_ready_is(const crosstalk_core_scan_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return LDG_TRUTH_FALSE;
}
