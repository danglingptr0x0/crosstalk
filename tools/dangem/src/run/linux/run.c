#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/path.h>
#include <dangling/str/fmt.h>
#include <dangling/sys/info.h>
#include <dangling/sys/tty.h>

#include "run/run.h"

#define DANGEM_RUN_TERM_ENV "TERMINAL"
#define DANGEM_RUN_GUARD_ENV "DANGEM_TERM_SPAWNED"
#define DANGEM_RUN_GUARD_VAL "yes"
#define DANGEM_RUN_GUARD_CAP 16
#define DANGEM_RUN_ENV_OVERWRITE 1
#define DANGEM_RUN_TERM_XFCE "xfce4-terminal"
#define DANGEM_RUN_TERM_NAME_CAP 64
#define DANGEM_RUN_TERM_FLAG_CAP 32
#define DANGEM_RUN_TERM_FLAG_MAX 3
#define DANGEM_RUN_TERM_EXEC "-e"
#define DANGEM_RUN_TERM_ARG_CUNT 6
#define DANGEM_RUN_TERM_ARG_NAME 0
#define DANGEM_RUN_TERM_ARG_FLAG 1

static uint32_t run_term_try(const char *name, const char *const *flag, char *self)
{
    char name_buff[DANGEM_RUN_TERM_NAME_CAP] = LDG_ARR_ZERO_INIT;
    char flag_buff[DANGEM_RUN_TERM_FLAG_MAX][DANGEM_RUN_TERM_FLAG_CAP] = LDG_ARR_ZERO_INIT;
    char *arg[DANGEM_RUN_TERM_ARG_CUNT] = LDG_ARR_ZERO_INIT;
    uint64_t len = 0;
    uint32_t at = DANGEM_RUN_TERM_ARG_FLAG;
    uint32_t idx = 0;
    uint32_t ret = 0;

    ret = ldg_str_fmt(name_buff, (uint64_t)sizeof(name_buff), &len, "%.63s", name);
    if (ret != LDG_ERR_AOK) { return ret; }

    arg[DANGEM_RUN_TERM_ARG_NAME] = name_buff;

    for (idx = 0; idx < (uint32_t)DANGEM_RUN_TERM_FLAG_MAX && flag[idx] != 0x0; idx++)
    {
        ret = ldg_str_fmt(flag_buff[idx], (uint64_t)sizeof(flag_buff[idx]), &len, "%.31s", flag[idx]);
        if (ret != LDG_ERR_AOK) { return ret; }

        arg[at] = flag_buff[idx];
        at = at + DANGEM_RUN_STEP;
    }

    arg[at] = self;
    arg[at + DANGEM_RUN_STEP] = 0x0;

    if (execvp(name_buff, arg) != 0) { return LDG_ERR_NOT_FOUND; }

    return LDG_ERR_NOT_FOUND;
}

uint32_t dangem_run_tty_ensure(void)
{
    const char *const term_name[] = { "wezterm", "x-terminal-emulator", "konsole", "gnome-terminal", DANGEM_RUN_TERM_XFCE, "alacritty", "kitty", "xterm" };
    const char *const term_flag[][DANGEM_RUN_TERM_FLAG_MAX + DANGEM_RUN_STEP] = {
        { "start", "--always-new-process", "--", 0x0 },
        { DANGEM_RUN_TERM_EXEC, 0x0, 0x0, 0x0 },
        { DANGEM_RUN_TERM_EXEC, 0x0, 0x0, 0x0 },
        { "--", 0x0, 0x0, 0x0 },
        { "-x", 0x0, 0x0, 0x0 },
        { DANGEM_RUN_TERM_EXEC, 0x0, 0x0, 0x0 },
        { "--", 0x0, 0x0, 0x0 },
        { DANGEM_RUN_TERM_EXEC, 0x0, 0x0, 0x0 }
    };
    const char *const env_flag[] = { DANGEM_RUN_TERM_EXEC, 0x0 };
    char self[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char env_term[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char guard[DANGEM_RUN_GUARD_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (ldg_sys_tty_stdout_is() == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (ldg_sys_env_get(DANGEM_RUN_GUARD_ENV, guard, (uint64_t)sizeof(guard)) == LDG_ERR_AOK) { return LDG_ERR_UNSUPPORTED; }

    ret = ldg_io_path_prog_get(self, (uint64_t)sizeof(self));
    if (ret != LDG_ERR_AOK) { return ret; }

    if (setenv(DANGEM_RUN_GUARD_ENV, DANGEM_RUN_GUARD_VAL, DANGEM_RUN_ENV_OVERWRITE) != 0) { return LDG_ERR_INVALID; }

    if (ldg_sys_env_get(DANGEM_RUN_TERM_ENV, env_term, (uint64_t)sizeof(env_term)) == LDG_ERR_AOK && env_term[0] != LDG_STR_TERM && run_term_try(env_term, env_flag, self) != LDG_ERR_NOT_FOUND) { return LDG_ERR_INVALID; }

    for (idx = 0; idx < (uint32_t)(sizeof(term_name) / sizeof(term_name[0])); idx++)
    {
        ret = run_term_try(term_name[idx], term_flag[idx], self);
        if (ret != LDG_ERR_NOT_FOUND) { return ret; }
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t dangem_run_exec(const char *path, const char *const *argv, const char *cwd, uint32_t *out_code)
{
    char text[DANGEM_RUN_CMDLINE_CAP] = LDG_ARR_ZERO_INIT;
    char *arg[DANGEM_RUN_ARG_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t used = 0;
    uint64_t len = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(path == 0x0 || argv == 0x0 || out_code == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_code = 0;

    for (idx = 0; argv[idx] != 0x0; idx++)
    {
        if (idx + DANGEM_RUN_STEP >= (uint32_t)DANGEM_RUN_ARG_MAX) { return LDG_ERR_FULL; }

        len = (uint64_t)strlen(argv[idx]);
        if (used + len + LDG_STR_TERM_SIZE > (uint64_t)sizeof(text)) { return LDG_ERR_STR_TRUNC; }

        memcpy(text + used, argv[idx], len + LDG_STR_TERM_SIZE);
        arg[idx] = text + used;
        used = used + len + LDG_STR_TERM_SIZE;
    }

    arg[idx] = 0x0;

    if (cwd != 0x0 && chdir(cwd) != 0) { return LDG_ERR_IO_NOT_FOUND; }

    if (execv(path, arg) != 0) { return LDG_ERR_IO_OPEN; }

    return LDG_ERR_IO_OPEN;
}

uint32_t dangem_run_pform_get(const char **out_name, const char **out_exe_ext)
{
    if (LDG_UNLIKELY(out_name == 0x0 || out_exe_ext == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_name = "linux";
    *out_exe_ext = "";

    return LDG_ERR_AOK;
}

uint8_t dangem_run_ldg_bundled_is(void)
{
    return LDG_TRUTH_TRUE;
}

uint8_t dangem_run_shlib_local_is(void)
{
    return LDG_TRUTH_TRUE;
}

uint32_t dangem_run_build_dir_get(const char **out)
{
    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = "build";

    return LDG_ERR_AOK;
}
