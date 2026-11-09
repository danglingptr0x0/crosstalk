#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/path.h>
#include <dangling/sys/info.h>
#include <dangling/sys/proc.h>
#include <dangling/sys/tty.h>

#include "run/run.h"

#define DANGEM_RUN_GUARD_ENV "DANGEM_TERM_SPAWNED"
#define DANGEM_RUN_GUARD_VAL "yes"
#define DANGEM_RUN_GUARD_CAP 16
#define DANGEM_RUN_PANE_ENV "WEZTERM_PANE"
#define DANGEM_RUN_TERM_ENV "TERMINAL"
#define DANGEM_RUN_PROG_ENV "ProgramFiles"
#define DANGEM_RUN_WEZ_DIR "WezTerm"
#define DANGEM_RUN_WEZ_GUI "wezterm-gui"
#define DANGEM_RUN_WEZ_CLI "wezterm"
#define DANGEM_RUN_WEZ_CONF_DIR "res"
#define DANGEM_RUN_WEZ_CONF_SUB "wezterm"
#define DANGEM_RUN_WEZ_CONF_FILE "crosstalk.lua"
#define DANGEM_RUN_TERM_EXEC "-e"
#define DANGEM_RUN_TERM_NAME_CAP 64
#define DANGEM_RUN_TERM_FLAG_MAX 3
#define DANGEM_RUN_TERM_CUNT 7
#define DANGEM_RUN_EXIT_OK 0

static uint32_t run_spawn_leave(const char *path, const char *const *argv, const char *cwd)
{
    ldg_sys_proc_t proc = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    ret = ldg_sys_proc_spawn(path, argv, cwd, LDG_SYS_PROC_SPAWN_STDIO_INHERIT, &proc);
    if (ret != LDG_ERR_AOK) { return LDG_ERR_NOT_FOUND; }

    if (ldg_sys_proc_close(&proc) != LDG_ERR_AOK) { return LDG_ERR_IO_CLOSE; }

    exit(DANGEM_RUN_EXIT_OK);
}

static uint32_t run_wez_try(const char *self, const char *root)
{
    char base[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char dir[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char term[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char res[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char sub[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char conf[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    const char *argv[DANGEM_RUN_ARG_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    ret = ldg_io_path_join(root, DANGEM_RUN_WEZ_CONF_DIR, res, (uint64_t)sizeof(res));
    if (ret == LDG_ERR_AOK) { ret = ldg_io_path_join(res, DANGEM_RUN_WEZ_CONF_SUB, sub, (uint64_t)sizeof(sub)); }

    if (ret == LDG_ERR_AOK) { ret = ldg_io_path_join(sub, DANGEM_RUN_WEZ_CONF_FILE, conf, (uint64_t)sizeof(conf)); }

    if (ret != LDG_ERR_AOK || ldg_io_path_exists_is(conf) != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_FOUND; }

    ret = ldg_sys_env_get(DANGEM_RUN_PROG_ENV, base, (uint64_t)sizeof(base));
    if (ret == LDG_ERR_AOK) { ret = ldg_io_path_join(base, DANGEM_RUN_WEZ_DIR, dir, (uint64_t)sizeof(dir)); }

    if (ret == LDG_ERR_AOK) { ret = ldg_io_path_join(dir, DANGEM_RUN_WEZ_GUI DANGEM_RUN_EXE_EXT, term, (uint64_t)sizeof(term)); }

    if (ret != LDG_ERR_AOK || ldg_io_path_exists_is(term) != LDG_TRUTH_TRUE) { ret = ldg_sys_proc_path_find(DANGEM_RUN_WEZ_GUI, term, (uint64_t)sizeof(term)); }

    if (ret != LDG_ERR_AOK) { return LDG_ERR_NOT_FOUND; }

    argv[0] = term;
    argv[1] = "--config-file";
    argv[2] = conf;
    argv[3] = "start";
    argv[4] = "--always-new-process";
    argv[5] = "--cwd";
    argv[6] = root;
    argv[7] = "--";
    argv[8] = self;
    argv[9] = 0x0;

    return run_spawn_leave(term, argv, root);
}

static uint32_t run_term_try(const char *name, const char *const *flag, const char *self)
{
    char path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    const char *argv[DANGEM_RUN_ARG_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t at = 0;
    uint32_t idx = 0;

    if (ldg_sys_proc_path_find(name, path, (uint64_t)sizeof(path)) != LDG_ERR_AOK) { return LDG_ERR_NOT_FOUND; }

    argv[at] = path;
    at = at + DANGEM_RUN_STEP;

    for (idx = 0; idx < (uint32_t)DANGEM_RUN_TERM_FLAG_MAX && flag[idx] != 0x0; idx++)
    {
        argv[at] = flag[idx];
        at = at + DANGEM_RUN_STEP;
    }

    argv[at] = self;
    argv[at + DANGEM_RUN_STEP] = 0x0;

    return run_spawn_leave(path, argv, 0x0);
}

uint32_t dangem_run_tty_ensure(void)
{
    const char *const term_name[DANGEM_RUN_TERM_CUNT] = { DANGEM_RUN_WEZ_CLI, "x-terminal-emulator", "konsole", "gnome-terminal", "xfce4-terminal", "alacritty", "xterm" };
    const char *const term_flag[DANGEM_RUN_TERM_CUNT][DANGEM_RUN_TERM_FLAG_MAX + DANGEM_RUN_STEP] = {
        { "start", "--always-new-process", "--", 0x0 },
        { DANGEM_RUN_TERM_EXEC, 0x0, 0x0, 0x0 },
        { DANGEM_RUN_TERM_EXEC, 0x0, 0x0, 0x0 },
        { "--", 0x0, 0x0, 0x0 },
        { "-x", 0x0, 0x0, 0x0 },
        { DANGEM_RUN_TERM_EXEC, 0x0, 0x0, 0x0 },
        { DANGEM_RUN_TERM_EXEC, 0x0, 0x0, 0x0 }
    };
    const char *const env_flag[DANGEM_RUN_TERM_FLAG_MAX + DANGEM_RUN_STEP] = { DANGEM_RUN_TERM_EXEC, 0x0, 0x0, 0x0 };
    char guard[DANGEM_RUN_GUARD_CAP] = LDG_ARR_ZERO_INIT;
    char env_term[DANGEM_RUN_TERM_NAME_CAP] = LDG_ARR_ZERO_INIT;
    char self[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char root[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint8_t tty_on = LDG_TRUTH_FALSE;
    uint32_t idx = 0;
    uint32_t ret = 0;

    tty_on = ldg_sys_tty_stdout_is();

    if (ldg_sys_env_get(DANGEM_RUN_GUARD_ENV, guard, (uint64_t)sizeof(guard)) == LDG_ERR_AOK) { return (tty_on == LDG_TRUTH_TRUE || DANGEM_RUN_TERM_FORCE == LDG_TRUTH_TRUE) ? LDG_ERR_AOK : LDG_ERR_UNSUPPORTED; }

    if (ldg_sys_env_get(DANGEM_RUN_PANE_ENV, guard, (uint64_t)sizeof(guard)) == LDG_ERR_AOK) { return LDG_ERR_AOK; }

    if (DANGEM_RUN_TERM_FORCE != LDG_TRUTH_TRUE && tty_on == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = ldg_io_path_prog_get(self, (uint64_t)sizeof(self));
    if (ret == LDG_ERR_AOK) { ret = ldg_io_path_parent_get(self, root, (uint64_t)sizeof(root)); }

    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_sys_env_set(DANGEM_RUN_GUARD_ENV, DANGEM_RUN_GUARD_VAL);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = run_wez_try(self, root);
    if (ret != LDG_ERR_NOT_FOUND) { return ret; }

    if (ldg_sys_env_get(DANGEM_RUN_TERM_ENV, env_term, (uint64_t)sizeof(env_term)) == LDG_ERR_AOK && env_term[0] != LDG_STR_TERM)
    {
        ret = run_term_try(env_term, env_flag, self);
        if (ret != LDG_ERR_NOT_FOUND) { return ret; }
    }

    for (idx = 0; idx < (uint32_t)DANGEM_RUN_TERM_CUNT; idx++)
    {
        ret = run_term_try(term_name[idx], term_flag[idx], self);
        if (ret != LDG_ERR_NOT_FOUND) { return ret; }
    }

    ret = ldg_sys_env_set(DANGEM_RUN_GUARD_ENV, 0x0);
    if (ret != LDG_ERR_AOK) { return ret; }

    return DANGEM_RUN_TERM_FORCE == LDG_TRUTH_TRUE ? LDG_ERR_AOK : LDG_ERR_NOT_FOUND;
}

uint32_t dangem_run_exec(const char *path, const char *const *argv, const char *cwd, uint32_t *out_code)
{
    ldg_sys_proc_t proc = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(path == 0x0 || argv == 0x0 || out_code == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_code = 0;

    ret = ldg_sys_proc_spawn(path, argv, cwd, LDG_SYS_PROC_SPAWN_STDIO_INHERIT, &proc);
    if (ret != LDG_ERR_AOK) { return LDG_ERR_IO_OPEN; }

    ret = ldg_sys_proc_wait(&proc, LDG_SYS_PROC_WAIT_INFINITE, out_code);

    if (ldg_sys_proc_close(&proc) != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    return ret;
}

uint32_t dangem_run_pform_get(const char **out_name, const char **out_exe_ext)
{
    if (LDG_UNLIKELY(out_name == 0x0 || out_exe_ext == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_name = DANGEM_RUN_PFORM;
    *out_exe_ext = DANGEM_RUN_EXE_EXT;

    return LDG_ERR_AOK;
}

uint8_t dangem_run_ldg_bundled_is(void)
{
    return LDG_TRUTH_TRUE;
}

uint8_t dangem_run_shlib_local_is(void)
{
    return DANGEM_RUN_SHLIB_LOCAL == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t dangem_run_build_dir_get(const char **out)
{
    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = DANGEM_RUN_BUILD_DIR;

    return LDG_ERR_AOK;
}
