#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <windows.h>

#include <yder.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/dir.h>
#include <dangling/io/file.h>
#include <dangling/io/path.h>
#include <dangling/str/str.h>

#include "mngr/mngr.h"

#define MNGR_STEP 1
#define MNGR_WIN_SEP '\\'
#define MNGR_DIR_MODE 0755
#define MNGR_CREATE_INHERIT_NONE 0
#define MNGR_JOIN_FMT "%s%c%s"
#define MNGR_CMD_FIRST_FMT "\"%.1023s\""
#define MNGR_CMD_NEXT_FMT " \"%.1023s\""
#define MNGR_CMD_FLAG_FMT " %s"
#define MNGR_CMD_DISK_FMT " \"%u:%.1000s\""
#define MNGR_CMD_LD_FMT " \"%.1000s@%" PRIu64 "\""
#define MNGR_CMD_HZ_FMT " --hz %" PRIu64
#define MNGR_CMD_GAMMA_FMT " --gamma %.2f"
#define MNGR_ARG_CORE "--core"
#define MNGR_ARG_MEMLD "--memld"
#define MNGR_ARG_DISK "--disk"
#define MNGR_ARG_NET "--net"
#define MNGR_ARG_KEYS_RAW "--keys raw"
#define MNGR_ARG_NO_AUDIO "--no-audio"
#define MNGR_ARG_NO_SERIAL "--no-serial"
#define MNGR_ARG_NO_PERSIST "--no-persist"

static uint32_t mngr_ext_is(const char *name, uint8_t *out)
{
    uint64_t name_len = 0;
    uint64_t ext_len = 0;

    if (LDG_UNLIKELY(name == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_TRUTH_FALSE;

    name_len = (uint64_t)strlen(name);
    ext_len = (uint64_t)strlen(CROSSTALK_MNGR_CONF_EXT);

    if (name_len <= ext_len) { return LDG_ERR_AOK; }

    if (strcmp(name + name_len - ext_len, CROSSTALK_MNGR_CONF_EXT) == 0) { *out = LDG_TRUTH_TRUE; }

    return LDG_ERR_AOK;
}

static uint32_t mngr_data_dir_resolve(char *out, uint64_t out_size)
{
    char nxt[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    const char *seg[CROSSTALK_MNGR_CONF_SEG_CUNT] = { 0x0, 0x0, 0x0 };
    uint32_t idx = 0;
    uint8_t exists = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_io_path_data_get(out, out_size);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    seg[0] = ldg_io_path_sep_get() == MNGR_WIN_SEP ? CROSSTALK_MNGR_CONF_ORG_WIN : CROSSTALK_MNGR_CONF_ORG_UNIX;
    seg[1] = CROSSTALK_MNGR_CONF_APP;
    seg[2] = CROSSTALK_MNGR_CONF_DIR_REL;

    for (idx = 0; idx < (uint32_t)CROSSTALK_MNGR_CONF_SEG_CUNT; idx = idx + MNGR_STEP)
    {
        ret = ldg_io_path_join(out, seg[idx], nxt, (uint64_t)sizeof(nxt));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        exists = ldg_io_path_exists_is(nxt);
        if (LDG_UNLIKELY(exists != LDG_TRUTH_TRUE && exists != LDG_TRUTH_FALSE)) { return exists; }

        if (exists == LDG_TRUTH_FALSE)
        {
            ret = ldg_io_dir_create(nxt, (uint32_t)MNGR_DIR_MODE);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = ldg_str_rbrcpy(out, nxt, out_size);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_mngr_init(crosstalk_mngr_ctx_t *ctx)
{
    char module[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t module_len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->sel = (uint32_t)UINT32_MAX;

    module_len = (uint64_t)GetModuleFileNameA(0x0, module, (DWORD)sizeof(module));
    if (module_len == 0 || module_len >= (uint64_t)sizeof(module)) { return LDG_ERR_IO_NOT_FOUND; }

    while (module_len != 0 && module[module_len - MNGR_STEP] != MNGR_WIN_SEP) { module_len = module_len - MNGR_STEP; }

    if (module_len == 0) { return LDG_ERR_IO_NOT_FOUND; }

    module[module_len - MNGR_STEP] = LDG_STR_TERM;

    ret = ldg_str_rbrcpy(ctx->exe_dir, module, (uint64_t)sizeof(ctx->exe_dir));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = mngr_data_dir_resolve(ctx->dir, (uint64_t)sizeof(ctx->dir));
    if (ret != LDG_ERR_AOK)
    {
        y_log_message(Y_LOG_LEVEL_ERROR, "mngr: machine directory not resolved; err: %u", ret);

        return ret;
    }

    ctx->ok = LDG_TRUTH_TRUE;

    y_log_message(Y_LOG_LEVEL_INFO, "mngr: machine directory; path: %.191s", ctx->dir);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_mngr_scan(crosstalk_mngr_ctx_t *ctx)
{
    ldg_io_dir_t *dir = 0x0;
    ldg_io_entry_t entry = LDG_STRUCT_ZERO_INIT;
    const char *name = 0x0;
    uint8_t wanted = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ctx->cunt = 0;

    ret = ldg_io_dir_open(ctx->dir, &dir);
    if (ret != LDG_ERR_AOK) { return ret; }

    while (ldg_io_dir_rd(dir, &entry) == LDG_ERR_AOK)
    {
        if (ldg_io_entry_dir_is(&entry) == LDG_TRUTH_TRUE) { continue; }

        if (ldg_io_entry_name_get(&entry, &name) != LDG_ERR_AOK || name == 0x0) { continue; }

        if (mngr_ext_is(name, &wanted) != LDG_ERR_AOK || wanted != LDG_TRUTH_TRUE) { continue; }

        if (ctx->cunt >= (uint32_t)CROSSTALK_MNGR_MACHINE_MAX) { break; }

        if (ldg_io_path_join(ctx->dir, name, ctx->path[ctx->cunt], (uint64_t)sizeof(ctx->path[ctx->cunt])) != LDG_ERR_AOK) { continue; }

        if (crosstalk_mngr_conf_rd(&ctx->machine[ctx->cunt], ctx->path[ctx->cunt]) != LDG_ERR_AOK)
        {
            y_log_message(Y_LOG_LEVEL_WARNING, "mngr: machine not read; path: %.191s", ctx->path[ctx->cunt]);

            continue;
        }

        ctx->cunt = ctx->cunt + MNGR_STEP;
    }

    if (ldg_io_dir_close(dir) != LDG_ERR_AOK) { LDG_ERRLOG_WARN("mngr dir close failed"); }

    if (ctx->cunt == 0) { ctx->sel = (uint32_t)UINT32_MAX; }
    else if (ctx->sel == (uint32_t)UINT32_MAX || ctx->sel >= ctx->cunt) { ctx->sel = 0; }

    y_log_message(Y_LOG_LEVEL_INFO, "mngr: machines found; cunt: %u", ctx->cunt);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_mngr_machine_add(crosstalk_mngr_ctx_t *ctx, const char *name)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->cunt >= (uint32_t)CROSSTALK_MNGR_MACHINE_MAX) { return CROSSTALK_MNGR_ERR_FULL; }

    if (crosstalk_mngr_conf_name_ok_is(name) != LDG_TRUTH_TRUE) { return CROSSTALK_MNGR_ERR_NAME; }

    ret = crosstalk_mngr_conf_default_get(&ctx->machine[ctx->cunt]);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_str_rbrcpy(ctx->machine[ctx->cunt].name, name, (uint64_t)sizeof(ctx->machine[ctx->cunt].name));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_mngr_conf_path_get(ctx->dir, name, ctx->path[ctx->cunt], (uint64_t)sizeof(ctx->path[ctx->cunt]));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_mngr_conf_wr(&ctx->machine[ctx->cunt], ctx->path[ctx->cunt]);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->sel = ctx->cunt;
    ctx->cunt = ctx->cunt + MNGR_STEP;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_mngr_machine_save(crosstalk_mngr_ctx_t *ctx, uint32_t idx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (idx >= ctx->cunt) { return LDG_ERR_BOUNDS; }

    return crosstalk_mngr_conf_wr(&ctx->machine[idx], ctx->path[idx]);
}

uint32_t crosstalk_mngr_machine_drop(crosstalk_mngr_ctx_t *ctx, uint32_t idx)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (idx >= ctx->cunt) { return LDG_ERR_BOUNDS; }

    if (ldg_io_path_destroy(ctx->path[idx]) != LDG_ERR_AOK) { y_log_message(Y_LOG_LEVEL_WARNING, "mngr: machine file not removed; path: %.191s", ctx->path[idx]); }

    for (at = idx; at + MNGR_STEP < ctx->cunt; at = at + MNGR_STEP)
    {
        memcpy(&ctx->machine[at], &ctx->machine[at + MNGR_STEP], sizeof(ctx->machine[at]));
        memcpy(ctx->path[at], ctx->path[at + MNGR_STEP], sizeof(ctx->path[at]));
    }

    ctx->cunt = ctx->cunt - MNGR_STEP;

    if (ctx->cunt == 0) { ctx->sel = (uint32_t)UINT32_MAX; }
    else if (ctx->sel >= ctx->cunt) { ctx->sel = ctx->cunt - MNGR_STEP; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_mngr_launch(const crosstalk_mngr_ctx_t *ctx, uint32_t idx)
{
    char exe[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char line[CROSSTALK_MNGR_CMD_CAP] = LDG_ARR_ZERO_INIT;
    const crosstalk_mngr_machine_t *machine = 0x0;
    STARTUPINFOA si = LDG_STRUCT_ZERO_INIT;
    PROCESS_INFORMATION pi = LDG_STRUCT_ZERO_INIT;
    DWORD code = 0;
    uint64_t off = 0;
    int32_t written = 0;
    uint32_t drive = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (idx >= ctx->cunt) { return LDG_ERR_BOUNDS; }

    machine = &ctx->machine[idx];

    if (machine->core_path[0] == LDG_STR_TERM) { return CROSSTALK_EMU_ERR_PLUGIN; }

    written = snprintf(exe, sizeof(exe), MNGR_JOIN_FMT, ctx->exe_dir, MNGR_WIN_SEP, CROSSTALK_MNGR_EMU_EXE);
    if (written < 0 || (uint64_t)written >= (uint64_t)sizeof(exe)) { return LDG_ERR_STR_TRUNC; }

    written = snprintf(line, sizeof(line), MNGR_CMD_FIRST_FMT, exe);
    if (written < 0) { return LDG_ERR_STR_FMT; }

    off = (uint64_t)written;

    written = snprintf(line + off, sizeof(line) - off, MNGR_CMD_FLAG_FMT, MNGR_ARG_CORE);
    if (written < 0) { return LDG_ERR_STR_FMT; }

    off = off + (uint64_t)written;

    written = snprintf(line + off, sizeof(line) - off, MNGR_CMD_NEXT_FMT, machine->core_path);
    if (written < 0 || (uint64_t)written >= (uint64_t)sizeof(line) - off) { return LDG_ERR_STR_TRUNC; }

    off = off + (uint64_t)written;

    for (at = 0; at < (uint32_t)CROSSTALK_EMU_LD_MAX; at = at + MNGR_STEP)
    {
        if (machine->ld_path[at][0] == LDG_STR_TERM) { continue; }

        written = snprintf(line + off, sizeof(line) - off, MNGR_CMD_FLAG_FMT, MNGR_ARG_MEMLD);
        if (written < 0) { return LDG_ERR_STR_FMT; }

        off = off + (uint64_t)written;

        if (machine->ld_off[at] != 0) { written = snprintf(line + off, sizeof(line) - off, MNGR_CMD_LD_FMT, machine->ld_path[at], machine->ld_off[at]); }
        else { written = snprintf(line + off, sizeof(line) - off, MNGR_CMD_NEXT_FMT, machine->ld_path[at]); }

        if (written < 0 || (uint64_t)written >= (uint64_t)sizeof(line) - off) { return LDG_ERR_STR_TRUNC; }

        off = off + (uint64_t)written;
    }

    for (drive = 0; drive < (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX; drive = drive + MNGR_STEP)
    {
        if (machine->disk_path[drive][0] == LDG_STR_TERM) { continue; }

        written = snprintf(line + off, sizeof(line) - off, MNGR_CMD_FLAG_FMT, MNGR_ARG_DISK);
        if (written < 0) { return LDG_ERR_STR_FMT; }

        off = off + (uint64_t)written;

        written = snprintf(line + off, sizeof(line) - off, MNGR_CMD_DISK_FMT, drive, machine->disk_path[drive]);
        if (written < 0 || (uint64_t)written >= (uint64_t)sizeof(line) - off) { return LDG_ERR_STR_TRUNC; }

        off = off + (uint64_t)written;
    }

    if (machine->net_path[0] != LDG_STR_TERM)
    {
        written = snprintf(line + off, sizeof(line) - off, MNGR_CMD_FLAG_FMT, MNGR_ARG_NET);
        if (written < 0) { return LDG_ERR_STR_FMT; }

        off = off + (uint64_t)written;

        written = snprintf(line + off, sizeof(line) - off, MNGR_CMD_NEXT_FMT, machine->net_path);
        if (written < 0 || (uint64_t)written >= (uint64_t)sizeof(line) - off) { return LDG_ERR_STR_TRUNC; }

        off = off + (uint64_t)written;
    }

    written = snprintf(line + off, sizeof(line) - off, MNGR_CMD_HZ_FMT, machine->f_op_hz);
    if (written < 0 || (uint64_t)written >= (uint64_t)sizeof(line) - off) { return LDG_ERR_STR_TRUNC; }

    off = off + (uint64_t)written;

    written = snprintf(line + off, sizeof(line) - off, MNGR_CMD_GAMMA_FMT, machine->gamma);
    if (written < 0 || (uint64_t)written >= (uint64_t)sizeof(line) - off) { return LDG_ERR_STR_TRUNC; }

    off = off + (uint64_t)written;

    if (machine->key_mode != (uint32_t)CROSSTALK_EMU_INPUT_MODE_SCAN)
    {
        written = snprintf(line + off, sizeof(line) - off, MNGR_CMD_FLAG_FMT, MNGR_ARG_KEYS_RAW);
        if (written < 0 || (uint64_t)written >= (uint64_t)sizeof(line) - off) { return LDG_ERR_STR_TRUNC; }

        off = off + (uint64_t)written;
    }

    if (machine->audio_on != LDG_TRUTH_TRUE)
    {
        written = snprintf(line + off, sizeof(line) - off, MNGR_CMD_FLAG_FMT, MNGR_ARG_NO_AUDIO);
        if (written < 0 || (uint64_t)written >= (uint64_t)sizeof(line) - off) { return LDG_ERR_STR_TRUNC; }

        off = off + (uint64_t)written;
    }

    if (machine->serial_on != LDG_TRUTH_TRUE)
    {
        written = snprintf(line + off, sizeof(line) - off, MNGR_CMD_FLAG_FMT, MNGR_ARG_NO_SERIAL);
        if (written < 0 || (uint64_t)written >= (uint64_t)sizeof(line) - off) { return LDG_ERR_STR_TRUNC; }

        off = off + (uint64_t)written;
    }

    if (machine->disk_persist != LDG_TRUTH_TRUE)
    {
        written = snprintf(line + off, sizeof(line) - off, MNGR_CMD_FLAG_FMT, MNGR_ARG_NO_PERSIST);
        if (written < 0 || (uint64_t)written >= (uint64_t)sizeof(line) - off) { return LDG_ERR_STR_TRUNC; }

        off = off + (uint64_t)written;
    }

    si.cb = (DWORD)sizeof(si);

    if (CreateProcessA(0x0, line, 0x0, 0x0, FALSE, (DWORD)MNGR_CREATE_INHERIT_NONE, 0x0, ctx->exe_dir, &si, &pi) == 0)
    {
        y_log_message(Y_LOG_LEVEL_WARNING, "mngr: machine would not start; name: %.47s; exe: %.191s; last: %u", machine->name, exe, (uint32_t)GetLastError());

        return LDG_ERR_IO_OPEN;
    }

    y_log_message(Y_LOG_LEVEL_INFO, "mngr: machine started; name: %.47s; pid: %u", machine->name, (uint32_t)pi.dwProcessId);

    if (WaitForSingleObject(pi.hProcess, INFINITE) == WAIT_FAILED) { y_log_message(Y_LOG_LEVEL_WARNING, "mngr: machine wait failed; name: %.47s; last: %u", machine->name, (uint32_t)GetLastError()); }

    if (GetExitCodeProcess(pi.hProcess, &code) == 0) { code = (DWORD)UINT32_MAX; }

    y_log_message(Y_LOG_LEVEL_INFO, "mngr: machine exited; name: %.47s; code: %u", machine->name, (uint32_t)code);

    if (CloseHandle(pi.hThread) == 0) { LDG_ERRLOG_WARN("mngr thread handle close failed"); }

    if (CloseHandle(pi.hProcess) == 0) { LDG_ERRLOG_WARN("mngr process handle close failed"); }

    return LDG_ERR_AOK;
}

uint8_t crosstalk_mngr_sel_ok_is(const crosstalk_mngr_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return (ctx->sel != (uint32_t)UINT32_MAX && ctx->sel < ctx->cunt) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}
