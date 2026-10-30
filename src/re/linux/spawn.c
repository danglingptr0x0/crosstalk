#include <spawn.h>
#include <stdint.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/logging/logging.h>
#include <dangling/str/str.h>

#include "core/log.h"
#include "re/spawn.h"

static uint32_t spawn_argv_split(char *line, char **argv, uint32_t cap, uint32_t *out_cunt)
{
    uint32_t at = 0;
    uint32_t cunt = 0;
    uint32_t wr = 0;
    uint8_t quoted = LDG_TRUTH_FALSE;
    uint8_t in_word = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(line == 0x0 || argv == 0x0 || out_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    while (line[at] != LDG_STR_TERM)
    {
        char ch = line[at];

        if (quoted != LDG_TRUTH_TRUE && (ch == ' ' || ch == '\t'))
        {
            if (in_word == LDG_TRUTH_TRUE)
            {
                line[wr] = LDG_STR_TERM;
                wr = wr + 0x1;
                in_word = LDG_TRUTH_FALSE;
            }

            at = at + 0x1;
            continue;
        }

        if (ch == LDG_ASCII_QUOTE)
        {
            quoted = quoted == LDG_TRUTH_TRUE ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE;

            if (in_word != LDG_TRUTH_TRUE)
            {
                if (cunt >= cap - LDG_STR_TERM_SIZE) { return CROSSTALK_RE_SPAWN_ERR_ARG; }

                argv[cunt] = &line[wr];
                cunt = cunt + 0x1;
                in_word = LDG_TRUTH_TRUE;
            }

            at = at + 0x1;
            continue;
        }

        if (in_word != LDG_TRUTH_TRUE)
        {
            if (cunt >= cap - LDG_STR_TERM_SIZE) { return CROSSTALK_RE_SPAWN_ERR_ARG; }

            argv[cunt] = &line[wr];
            cunt = cunt + 0x1;
            in_word = LDG_TRUTH_TRUE;
        }

        line[wr] = ch;
        wr = wr + 0x1;
        at = at + 0x1;
    }

    if (quoted == LDG_TRUTH_TRUE) { return CROSSTALK_RE_SPAWN_ERR_ARG; }

    line[wr] = LDG_STR_TERM;
    argv[cunt] = 0x0;
    *out_cunt = cunt;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_spawn_run(const char *line, uint32_t *out_exit)
{
    char buff[CROSSTALK_RE_SPAWN_LINE_CAP] = LDG_ARR_ZERO_INIT;
    char *argv[CROSSTALK_RE_SPAWN_ARG_MAX];
    pid_t child = 0;
    int32_t status = 0;
    uint32_t len = 0;
    uint32_t cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(line == 0x0 || out_exit == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_exit = (uint32_t)CROSSTALK_RE_SPAWN_ERR_EXIT;

    while (line[len] != LDG_STR_TERM) { len = len + 0x1; }

    if (len == 0 || len + LDG_STR_TERM_SIZE > (uint32_t)sizeof(buff)) { return CROSSTALK_RE_SPAWN_ERR_ARG; }

    memcpy(buff, line, (uint64_t)len + LDG_STR_TERM_SIZE);

    ret = spawn_argv_split(buff, argv, (uint32_t)CROSSTALK_RE_SPAWN_ARG_MAX, &cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (cunt == 0) { return CROSSTALK_RE_SPAWN_ERR_ARG; }

    if (posix_spawnp(&child, argv[0], 0x0, 0x0, argv, environ) != 0)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re spawn: failed; exe: %.255s", argv[0]);

        return CROSSTALK_RE_SPAWN_ERR_SPAWN;
    }

    if (waitpid(child, &status, (int32_t)CROSSTALK_RE_SPAWN_WAIT_FLAGS) != child) { return CROSSTALK_RE_SPAWN_ERR_SPAWN; }

    *out_exit = WIFEXITED(status) ? (uint32_t)WEXITSTATUS(status) : (uint32_t)CROSSTALK_RE_SPAWN_ERR_EXIT;

    return LDG_ERR_AOK;
}
