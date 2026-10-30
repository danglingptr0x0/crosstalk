#include <stdint.h>
#include <string.h>

#include <windows.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "re/spawn.h"

uint32_t crosstalk_re_spawn_run(const char *line, uint32_t *out_exit)
{
    char cmd[CROSSTALK_RE_SPAWN_LINE_CAP] = LDG_ARR_ZERO_INIT;
    STARTUPINFOA si = LDG_STRUCT_ZERO_INIT;
    PROCESS_INFORMATION pi = LDG_STRUCT_ZERO_INIT;
    DWORD status = 0;
    uint32_t len = 0;

    if (LDG_UNLIKELY(line == 0x0 || out_exit == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_exit = (uint32_t)CROSSTALK_RE_SPAWN_ERR_EXIT;

    while (line[len] != LDG_STR_TERM) { len = len + 0x1; }

    if (len == 0 || len + LDG_STR_TERM_SIZE > (uint32_t)sizeof(cmd)) { return CROSSTALK_RE_SPAWN_ERR_ARG; }

    memcpy(cmd, line, (uint64_t)len + LDG_STR_TERM_SIZE);

    si.cb = (DWORD)sizeof(si);

    if (CreateProcessA(0x0, cmd, 0x0, 0x0, FALSE, CREATE_NO_WINDOW, 0x0, 0x0, &si, &pi) == 0)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re spawn: failed; last: %u32; line: %.255s", (uint32_t)GetLastError(), line);

        return CROSSTALK_RE_SPAWN_ERR_SPAWN;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    if (GetExitCodeProcess(pi.hProcess, &status) == 0) { status = (DWORD)CROSSTALK_RE_SPAWN_ERR_EXIT; }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    *out_exit = (uint32_t)status;

    return LDG_ERR_AOK;
}
