#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/logging/logging.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/dir.h>
#include <dangling/io/file.h>
#include <dangling/io/path.h>
#include <dangling/str/str.h>
#include <dangling/sys/proc.h>

#include "mngr/mngr.h"

#define MNGR_STEP 1
#define MNGR_WIN_SEP '\\'
#define MNGR_DIR_MODE 0755
#define MNGR_ARG_DISK_FMT "%u:%s"
#define MNGR_ARG_LD_FMT "%s@%" PRIu64
#define MNGR_ARG_HZ_FMT "%" PRIu64
#define MNGR_ARG_GAMMA_FMT "%.2f"
#define MNGR_ARG_CORE "--core"
#define MNGR_ARG_MEMLD "--memld"
#define MNGR_ARG_DISK "--disk"
#define MNGR_ARG_NET "--net"
#define MNGR_ARG_HZ "--hz"
#define MNGR_ARG_GAMMA "--gamma"
#define MNGR_ARG_KEYS "--keys"
#define MNGR_ARG_KEYS_RAW "raw"
#define MNGR_ARG_NO_AUDIO "--no-audio"
#define MNGR_ARG_NO_SERIAL "--no-serial"
#define MNGR_ARG_NO_PERSIST "--no-persist"

typedef struct mngr_argv
{
    const char *arg[LDG_SYS_PROC_ARG_MAX];
    char st[CROSSTALK_MNGR_CMD_CAP];
    uint64_t st_off;
    uint32_t cunt;
    uint8_t pudding[4];
} mngr_argv_t;

static uint32_t mngr_argv_push(mngr_argv_t *argv, const char *arg)
{
    if (LDG_UNLIKELY(argv == 0x0 || arg == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (argv->cunt + MNGR_STEP >= (uint32_t)LDG_SYS_PROC_ARG_MAX) { return LDG_ERR_FULL; }

    argv->arg[argv->cunt] = arg;
    argv->cunt = argv->cunt + MNGR_STEP;
    argv->arg[argv->cunt] = 0x0;

    return LDG_ERR_AOK;
}

static uint32_t mngr_argv_fmt_push(mngr_argv_t *argv, const char *fmt, ...)
{
    va_list args;
    char *at = 0x0;
    uint64_t room = 0;
    int32_t written = 0;

    if (LDG_UNLIKELY(argv == 0x0 || fmt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (argv->st_off >= (uint64_t)sizeof(argv->st)) { return LDG_ERR_STR_TRUNC; }

    at = argv->st + argv->st_off;
    room = (uint64_t)sizeof(argv->st) - argv->st_off;

    va_start(args, fmt);
    written = vsnprintf(at, room, fmt, args);
    va_end(args);

    if (written < 0) { return LDG_ERR_STR_FMT; }

    if ((uint64_t)written >= room) { return LDG_ERR_STR_TRUNC; }

    argv->st_off = argv->st_off + (uint64_t)written + (uint64_t)LDG_STR_TERM_SIZE;

    return mngr_argv_push(argv, at);
}

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
    char prog[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->sel = (uint32_t)UINT32_MAX;

    ret = ldg_io_path_prog_get(prog, (uint64_t)sizeof(prog));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_path_parent_get(prog, ctx->exe_dir, (uint64_t)sizeof(ctx->exe_dir));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = mngr_data_dir_resolve(ctx->dir, (uint64_t)sizeof(ctx->dir));
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_MNGR_LOG_PATH, LDG_LOGGING_LVL_ERR, "mngr: machine directory not resolved; err: %u32", ret);

        return ret;
    }

    ctx->ok = LDG_TRUTH_TRUE;

    ldg_logging_line_wr(CROSSTALK_MNGR_LOG_PATH, LDG_LOGGING_LVL_INFO, "mngr: machine directory; path: %.191s", ctx->dir);

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
            ldg_logging_line_wr(CROSSTALK_MNGR_LOG_PATH, LDG_LOGGING_LVL_WARN, "mngr: machine not read; path: %.191s", ctx->path[ctx->cunt]);

            continue;
        }

        ctx->cunt = ctx->cunt + MNGR_STEP;
    }

    if (ldg_io_dir_close(dir) != LDG_ERR_AOK) { LDG_ERRLOG_WARN("mngr dir close failed"); }

    if (ctx->cunt == 0) { ctx->sel = (uint32_t)UINT32_MAX; }
    else if (ctx->sel == (uint32_t)UINT32_MAX || ctx->sel >= ctx->cunt) { ctx->sel = 0; }

    ldg_logging_line_wr(CROSSTALK_MNGR_LOG_PATH, LDG_LOGGING_LVL_INFO, "mngr: machines found; cunt: %u32", ctx->cunt);

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

    if (ldg_io_path_destroy(ctx->path[idx]) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_MNGR_LOG_PATH, LDG_LOGGING_LVL_WARN, "mngr: machine file not removed; path: %.191s", ctx->path[idx]); }

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

static LDG_NOINLINE uint32_t mngr_argv_build(const crosstalk_mngr_machine_t *machine, const char *exe, mngr_argv_t *argv)
{
    uint32_t drive = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(machine == 0x0 || exe == 0x0 || argv == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = mngr_argv_push(argv, exe);
    if (ret == LDG_ERR_AOK) { ret = mngr_argv_push(argv, MNGR_ARG_CORE); }

    if (ret == LDG_ERR_AOK) { ret = mngr_argv_push(argv, machine->core_path); }

    for (at = 0; ret == LDG_ERR_AOK && at < (uint32_t)CROSSTALK_EMU_LD_MAX; at = at + MNGR_STEP)
    {
        if (machine->ld_path[at][0] == LDG_STR_TERM) { continue; }

        ret = mngr_argv_push(argv, MNGR_ARG_MEMLD);
        if (ret != LDG_ERR_AOK) { break; }

        if (machine->ld_off[at] != 0) { ret = mngr_argv_fmt_push(argv, MNGR_ARG_LD_FMT, machine->ld_path[at], machine->ld_off[at]); }
        else { ret = mngr_argv_push(argv, machine->ld_path[at]); }
    }

    for (drive = 0; ret == LDG_ERR_AOK && drive < (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX; drive = drive + MNGR_STEP)
    {
        if (machine->disk_path[drive][0] == LDG_STR_TERM) { continue; }

        ret = mngr_argv_push(argv, MNGR_ARG_DISK);
        if (ret == LDG_ERR_AOK) { ret = mngr_argv_fmt_push(argv, MNGR_ARG_DISK_FMT, drive, machine->disk_path[drive]); }
    }

    if (ret == LDG_ERR_AOK && machine->net_path[0] != LDG_STR_TERM)
    {
        ret = mngr_argv_push(argv, MNGR_ARG_NET);
        if (ret == LDG_ERR_AOK) { ret = mngr_argv_push(argv, machine->net_path); }
    }

    if (ret == LDG_ERR_AOK) { ret = mngr_argv_push(argv, MNGR_ARG_HZ); }

    if (ret == LDG_ERR_AOK) { ret = mngr_argv_fmt_push(argv, MNGR_ARG_HZ_FMT, machine->f_op_hz); }

    if (ret == LDG_ERR_AOK) { ret = mngr_argv_push(argv, MNGR_ARG_GAMMA); }

    if (ret == LDG_ERR_AOK) { ret = mngr_argv_fmt_push(argv, MNGR_ARG_GAMMA_FMT, machine->gamma); }

    if (ret == LDG_ERR_AOK && machine->key_mode != (uint32_t)CROSSTALK_EMU_INPUT_MODE_SCAN)
    {
        ret = mngr_argv_push(argv, MNGR_ARG_KEYS);
        if (ret == LDG_ERR_AOK) { ret = mngr_argv_push(argv, MNGR_ARG_KEYS_RAW); }
    }

    if (ret == LDG_ERR_AOK && machine->audio_on != LDG_TRUTH_TRUE) { ret = mngr_argv_push(argv, MNGR_ARG_NO_AUDIO); }

    if (ret == LDG_ERR_AOK && machine->serial_on != LDG_TRUTH_TRUE) { ret = mngr_argv_push(argv, MNGR_ARG_NO_SERIAL); }

    if (ret == LDG_ERR_AOK && machine->disk_persist != LDG_TRUTH_TRUE) { ret = mngr_argv_push(argv, MNGR_ARG_NO_PERSIST); }

    return ret;
}

uint32_t crosstalk_mngr_launch(const crosstalk_mngr_ctx_t *ctx, uint32_t idx)
{
    char exe[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    mngr_argv_t argv = LDG_STRUCT_ZERO_INIT;
    ldg_sys_proc_t proc = LDG_STRUCT_ZERO_INIT;
    const crosstalk_mngr_machine_t *machine = 0x0;
    const char *exe_name = 0x0;
    uint32_t code = (uint32_t)UINT32_MAX;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (idx >= ctx->cunt) { return LDG_ERR_BOUNDS; }

    machine = &ctx->machine[idx];

    if (machine->core_path[0] == LDG_STR_TERM) { return CROSSTALK_EMU_ERR_PLUGIN; }

    exe_name = ldg_io_path_sep_get() == MNGR_WIN_SEP ? CROSSTALK_MNGR_EMU_EXE_WIN : CROSSTALK_MNGR_EMU_EXE_UNIX;

    ret = ldg_io_path_join(ctx->exe_dir, exe_name, exe, (uint64_t)sizeof(exe));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = mngr_argv_build(machine, exe, &argv);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_MNGR_LOG_PATH, LDG_LOGGING_LVL_WARN, "mngr: machine args not built; name: %.47s; err: %u32", machine->name, ret);

        return ret;
    }

    ret = ldg_sys_proc_spawn(exe, argv.arg, ctx->exe_dir, LDG_SYS_PROC_SPAWN_STDIO_INHERIT, &proc);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_MNGR_LOG_PATH, LDG_LOGGING_LVL_WARN, "mngr: machine would not start; name: %.47s; exe: %.191s; err: %u32", machine->name, exe, ret);

        return ret;
    }

    ldg_logging_line_wr(CROSSTALK_MNGR_LOG_PATH, LDG_LOGGING_LVL_INFO, "mngr: machine started; name: %.47s; pid: %u32", machine->name, proc.pid);

    do { ret = ldg_sys_proc_wait(&proc, LDG_SYS_PROC_WAIT_INFINITE, &code); } while (ret == LDG_ERR_INTERRUPTED);

    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_MNGR_LOG_PATH, LDG_LOGGING_LVL_WARN, "mngr: machine wait failed; name: %.47s; err: %u32", machine->name, ret); }
    else { ldg_logging_line_wr(CROSSTALK_MNGR_LOG_PATH, LDG_LOGGING_LVL_INFO, "mngr: machine exited; name: %.47s; code: %u32", machine->name, code); }

    if (ldg_sys_proc_close(&proc) != LDG_ERR_AOK) { LDG_ERRLOG_WARN("mngr process close failed"); }

    return ret;
}

uint8_t crosstalk_mngr_sel_ok_is(const crosstalk_mngr_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return (ctx->sel != (uint32_t)UINT32_MAX && ctx->sel < ctx->cunt) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}
