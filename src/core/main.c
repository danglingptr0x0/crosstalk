#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/sys/info.h>
#include <dangling/io/path.h>

#include <dangling/logging/logging.h>

#include "core/crosstalk.h"

#define CROSSTALK_CONNECT_LOBBY_ARG "+connect_lobby"
#define CROSSTALK_CONNECT_ARG_STRIDE 1
#define CROSSTALK_CONNECT_SCAN_CUNT 1
#define CROSSTALK_CONNECT_LOBBY_NONE 0
#define CROSSTALK_CONNECT_STR_MATCH 0
#define CROSSTALK_EXIT_OK 0
#define CROSSTALK_ARGV_SELF 0
#define CROSSTALK_SCHEME_APP_NAME "Crosstalk"

static uint64_t crosstalk_connect_lobby_parse(uint32_t argc, char **argv)
{
    uint64_t lobby_id = 0;

    if (argv == 0x0) { return CROSSTALK_CONNECT_LOBBY_NONE; }

    for (uint32_t idx = 0; idx + CROSSTALK_CONNECT_ARG_STRIDE < argc; idx++)
    {
        if (argv[idx] == 0x0 || argv[idx + CROSSTALK_CONNECT_ARG_STRIDE] == 0x0) { continue; }

        if (strcmp(argv[idx], CROSSTALK_CONNECT_LOBBY_ARG) != CROSSTALK_CONNECT_STR_MATCH) { continue; }

        if (sscanf(argv[idx + CROSSTALK_CONNECT_ARG_STRIDE], "%" SCNu64, &lobby_id) != CROSSTALK_CONNECT_SCAN_CUNT) { continue; }

        return lobby_id;
    }

    return CROSSTALK_CONNECT_LOBBY_NONE;
}

int main(int argc, char **argv)
{
    crosstalk_ctx_t *ctx = 0x0;
    uint32_t launch_app_id = 0;
    uint64_t join_lobby = 0;
    uint32_t err = 0;

    err = ldg_mem_init();
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        fprintf(stdout, "crosstalk: mem init failed; err: %u\n", err);
        return (int32_t)(err & 0xFFu);
    }

    launch_app_id = crosstalk_core_launch_app_id_get();

    if (launch_app_id != UINT32_MAX && ldg_net_steamworks_restart_needed_is(launch_app_id) == LDG_TRUTH_TRUE)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: relaunching through Steam; app: %u32", launch_app_id);
        ldg_mem_shutdown();
        return CROSSTALK_EXIT_OK;
    }

    err = crosstalk_core_create(&ctx);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "create failed");
        ldg_mem_shutdown();
        return (int32_t)(err & 0xFFu);
    }

    join_lobby = crosstalk_connect_lobby_parse((uint32_t)argc, argv);
    if (join_lobby != CROSSTALK_CONNECT_LOBBY_NONE) { crosstalk_core_join_lobby_set(ctx, join_lobby); }

    err = crosstalk_core_run(ctx);
    if (err != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "run exited"); }

    crosstalk_core_destroy(ctx);
    ldg_sys_notify_close();
    ldg_mem_shutdown();
    return (int32_t)(err & 0xFFu);
}
