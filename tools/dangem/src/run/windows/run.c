#include <stdint.h>
#include <string.h>
#include <windows.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/path.h>
#include <dangling/sys/info.h>

#include "run/run.h"

#define DANGEM_RUN_QUOTE '"'
#define DANGEM_RUN_SEP ' '
#define DANGEM_RUN_STEP 1
#define DANGEM_RUN_CREATE_FLAGS_INHERIT_CONSOLE 0x0U
#define DANGEM_RUN_CREATE_FLAGS_TERM 0x0U
#define DANGEM_RUN_INHERIT_NONE FALSE
#define DANGEM_RUN_GUARD_ENV "DANGEM_TERM_SPAWNED"
#define DANGEM_RUN_GUARD_VAL "yes"
#define DANGEM_RUN_GUARD_CAP 16
#define DANGEM_RUN_PANE_ENV "WEZTERM_PANE"
#define DANGEM_RUN_PROG_ENV "ProgramFiles"
#define DANGEM_RUN_TERM_DIR "WezTerm"
#define DANGEM_RUN_TERM_EXE "wezterm-gui.exe"
#define DANGEM_RUN_TERM_CONF "res\\wezterm\\crosstalk.lua"
#define DANGEM_RUN_SEARCH_NONE 0U
#define DANGEM_RUN_EXIT_OK 0U

static uint32_t run_cmdline_put(char *dst, uint64_t cap, uint64_t *io_len, char ch)
{
    if (*io_len + DANGEM_RUN_STEP >= cap) { return LDG_ERR_FULL; }

    dst[*io_len] = ch;
    *io_len = *io_len + DANGEM_RUN_STEP;
    dst[*io_len] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

static uint32_t run_cmdline_build(const char *const *argv, char *dst, uint64_t cap)
{
    uint64_t len = 0;
    uint32_t idx = 0;
    uint32_t pos = 0;
    uint32_t ret = 0;

    dst[0] = LDG_STR_TERM;

    for (idx = 0; argv[idx] != 0x0; idx++)
    {
        if (idx != 0) { ret = run_cmdline_put(dst, cap, &len, DANGEM_RUN_SEP); }

        if (ret == LDG_ERR_AOK) { ret = run_cmdline_put(dst, cap, &len, DANGEM_RUN_QUOTE); }

        for (pos = 0; ret == LDG_ERR_AOK && argv[idx][pos] != LDG_STR_TERM; pos++) { ret = run_cmdline_put(dst, cap, &len, argv[idx][pos]); }

        if (ret == LDG_ERR_AOK) { ret = run_cmdline_put(dst, cap, &len, DANGEM_RUN_QUOTE); }

        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return LDG_ERR_AOK;
}

uint32_t dangem_run_exec(const char *path, const char *const *argv, const char *cwd, uint32_t *out_code)
{
    STARTUPINFOA start = LDG_STRUCT_ZERO_INIT;
    PROCESS_INFORMATION info = LDG_STRUCT_ZERO_INIT;
    char cmdline[DANGEM_RUN_CMDLINE_CAP] = LDG_ARR_ZERO_INIT;
    DWORD code = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(path == 0x0 || argv == 0x0 || out_code == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_code = 0;

    ret = run_cmdline_build(argv, cmdline, (uint64_t)sizeof(cmdline));
    if (ret != LDG_ERR_AOK) { return ret; }

    start.cb = (DWORD)sizeof(start);

    if (CreateProcessA(path, cmdline, 0x0, 0x0, TRUE, (DWORD)DANGEM_RUN_CREATE_FLAGS_INHERIT_CONSOLE, 0x0, cwd, &start, &info) == FALSE) { return LDG_ERR_IO_OPEN; }

    if (WaitForSingleObject(info.hProcess, INFINITE) == WAIT_FAILED) { ret = LDG_ERR_INVALID; }

    if (ret == LDG_ERR_AOK && GetExitCodeProcess(info.hProcess, &code) != FALSE) { *out_code = (uint32_t)code; }

    if (CloseHandle(info.hThread) == FALSE && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    if (CloseHandle(info.hProcess) == FALSE && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    return ret;
}

uint32_t dangem_run_pform_get(const char **out_name, const char **out_exe_ext)
{
    if (LDG_UNLIKELY(out_name == 0x0 || out_exe_ext == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_name = "windows";
    *out_exe_ext = ".exe";

    return LDG_ERR_AOK;
}

uint8_t dangem_run_ldg_bundled_is(void)
{
    return LDG_TRUTH_TRUE;
}

uint8_t dangem_run_shlib_local_is(void)
{
    return LDG_TRUTH_FALSE;
}

static uint32_t run_term_find(char *dst, uint64_t cap)
{
    char base[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char dir[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    DWORD got = 0;
    uint32_t ret = 0;

    dst[0] = LDG_STR_TERM;

    ret = ldg_sys_env_get(DANGEM_RUN_PROG_ENV, base, (uint64_t)sizeof(base));
    if (ret == LDG_ERR_AOK) { ret = ldg_io_path_join(base, DANGEM_RUN_TERM_DIR, dir, (uint64_t)sizeof(dir)); }

    if (ret == LDG_ERR_AOK) { ret = ldg_io_path_join(dir, DANGEM_RUN_TERM_EXE, dst, cap); }

    if (ret == LDG_ERR_AOK && ldg_io_path_exists_is(dst) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    got = SearchPathA(0x0, DANGEM_RUN_TERM_EXE, 0x0, (DWORD)cap, dst, 0x0);
    if (got == DANGEM_RUN_SEARCH_NONE || (uint64_t)got >= cap)
    {
        dst[0] = LDG_STR_TERM;
        return LDG_ERR_NOT_FOUND;
    }

    return LDG_ERR_AOK;
}

uint32_t dangem_run_tty_ensure(void)
{
    STARTUPINFOA start = LDG_STRUCT_ZERO_INIT;
    PROCESS_INFORMATION info = LDG_STRUCT_ZERO_INIT;
    char guard[DANGEM_RUN_GUARD_CAP] = LDG_ARR_ZERO_INIT;
    char term[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char self[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char root[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char conf[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char cmdline[DANGEM_RUN_CMDLINE_CAP] = LDG_ARR_ZERO_INIT;
    const char *argv[DANGEM_RUN_ARG_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    if (ldg_sys_env_get(DANGEM_RUN_GUARD_ENV, guard, (uint64_t)sizeof(guard)) == LDG_ERR_AOK) { return LDG_ERR_AOK; }

    if (ldg_sys_env_get(DANGEM_RUN_PANE_ENV, guard, (uint64_t)sizeof(guard)) == LDG_ERR_AOK) { return LDG_ERR_AOK; }

    if (run_term_find(term, (uint64_t)sizeof(term)) != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    ret = ldg_io_path_prog_get(self, (uint64_t)sizeof(self));
    if (ret == LDG_ERR_AOK) { ret = ldg_io_path_parent_get(self, root, (uint64_t)sizeof(root)); }

    if (ret == LDG_ERR_AOK) { ret = ldg_io_path_join(root, DANGEM_RUN_TERM_CONF, conf, (uint64_t)sizeof(conf)); }

    if (ret != LDG_ERR_AOK || ldg_io_path_exists_is(conf) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

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

    ret = run_cmdline_build(argv, cmdline, (uint64_t)sizeof(cmdline));
    if (ret != LDG_ERR_AOK) { return ret; }

    if (SetEnvironmentVariableA(DANGEM_RUN_GUARD_ENV, DANGEM_RUN_GUARD_VAL) == FALSE) { return LDG_ERR_INVALID; }

    start.cb = (DWORD)sizeof(start);

    if (CreateProcessA(term, cmdline, 0x0, 0x0, DANGEM_RUN_INHERIT_NONE, (DWORD)DANGEM_RUN_CREATE_FLAGS_TERM, 0x0, root, &start, &info) == FALSE)
    {
        if (SetEnvironmentVariableA(DANGEM_RUN_GUARD_ENV, 0x0) == FALSE) { return LDG_ERR_INVALID; }

        return LDG_ERR_AOK;
    }

    if (CloseHandle(info.hThread) == FALSE) { ret = LDG_ERR_IO_CLOSE; }

    if (CloseHandle(info.hProcess) == FALSE) { ret = LDG_ERR_IO_CLOSE; }

    if (ret != LDG_ERR_AOK) { return ret; }

    ExitProcess(DANGEM_RUN_EXIT_OK);

    return LDG_ERR_AOK;
}

uint32_t dangem_run_build_dir_get(const char **out)
{
    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = "build-win";

    return LDG_ERR_AOK;
}
