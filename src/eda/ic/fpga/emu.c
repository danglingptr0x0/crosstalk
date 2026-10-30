#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include <dangling/logging/logging.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/core/bits.h>
#include <dangling/mem/alloc.h>
#include <dangling/io/file.h>
#include <dangling/io/path.h>
#include <dangling/proto/emiemi.h>
#include <dangling/str/str.h>
#include <dangling/time/time.h>

#include "core/log.h"
#include "eda/ic/fpga/emu.h"
#include "eda/ic/fpga/dangemu_rva.h"

#define EMU_DISK_BKP_FMT "%s.%" PRIu64 ".bkp"

uint32_t crosstalk_eda_ic_fpga_emu_init(crosstalk_eda_ic_fpga_emu_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->disk_persist = LDG_TRUTH_TRUE;

    ret = ldg_mem_pool_create((uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_POOL_VAR_SIZE, (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_RAM_BYTES, &ctx->pool_backing);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: guest ram refused; bytes: %u64; err: %u32", (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_RAM_BYTES, ret);

        return ret;
    }

    ret = ldg_mem_pool_alloc(ctx->pool_backing, (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_RAM_BYTES, (void **)&ctx->pool);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: guest ram alloc refused; bytes: %u64; err: %u32", (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_RAM_BYTES, ret);

        return ret;
    }

    ctx->pool_span = (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_RAM_BYTES;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu: guest ram reserved; bytes: %u64", ctx->pool_span);

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_ic_fpga_emu_disk_live_is(const crosstalk_eda_ic_fpga_emu_ctx_t *ctx, uint32_t drive)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (drive >= (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX) { return LDG_ERR_FUNC_ARG_INVALID; }

    return ctx->disk_span[drive] != 0 && ctx->disk_path[drive][0] != LDG_STR_TERM ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_eda_ic_fpga_emu_disk_bind(crosstalk_eda_ic_fpga_emu_ctx_t *ctx, uint32_t drive)
{
    uint64_t span = 0;
    uint64_t blks = 0;
    uint32_t width = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (drive >= (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->disk_span[drive] == 0) { return LDG_ERR_EMPTY; }

    if (ctx->abi.ok != LDG_TRUTH_TRUE) { return LDG_ERR_UNSUPPORTED; }

    span = ctx->disk_span[drive];

    if (ctx->abi.disk_blk_width_get != 0x0) { width = ctx->abi.disk_blk_width_get(); }

    if (width != 0)
    {
        blks = span / (uint64_t)width;

        if (blks * (uint64_t)width != span)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: disk image is not a whole number of blocks; drive: %u32; path: %.127s; bytes: %u64; blk width: %u32; tail bytes: %u64", drive, ctx->disk_path[drive], span, width, span - blks * (uint64_t)width);

            span = blks * (uint64_t)width;
        }
    }

    if (span == 0)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: disk image is shorter than one block; drive: %u32; bytes: %u64; blk width: %u32", drive, ctx->disk_span[drive], width);

        return CROSSTALK_EDA_IC_FPGA_EMU_ERR_SPAN;
    }

    if (ctx->abi.disk_drive_set != 0x0) { ret = ctx->abi.disk_drive_set(drive, ctx->disk[drive], span); }
    else if (drive == 0 && ctx->abi.disk_set != 0x0) { ret = ctx->abi.disk_set(ctx->disk[drive], span); }
    else
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: the emulator has no drive export; drive: %u32", drive);

        return LDG_ERR_UNSUPPORTED;
    }

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: disk set refused; drive: %u32; bytes: %u64; err: %u32", drive, span, ret);

        return CROSSTALK_EDA_IC_FPGA_EMU_ERR_DISK;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu: disk attached; drive: %u32; path: %.127s; bytes: %u64; blk width: %u32; blks: %u64", drive, ctx->disk_path[drive], span, width, blks);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_emu_disk_ld(crosstalk_eda_ic_fpga_emu_ctx_t *ctx, uint32_t drive, const char *path)
{
    ldg_io_info_t st = LDG_STRUCT_ZERO_INIT;
    ldg_io_file_t *file = 0x0;
    ldg_byte_t *media = 0x0;
    ldg_byte_t *prev = 0x0;
    uint64_t bytes_rd = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (drive >= (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (path[0] == LDG_STR_TERM) { return CROSSTALK_EDA_IC_FPGA_EMU_ERR_PATH; }

    ret = ldg_io_path_info_get(path, &st);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: disk image not reachable; path: %.191s; err: %u32", path, ret);

        return CROSSTALK_EDA_IC_FPGA_EMU_ERR_PATH;
    }

    if (st.size == 0)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: disk image span refused; path: %.191s; bytes: %u64", path, st.size);

        return CROSSTALK_EDA_IC_FPGA_EMU_ERR_SPAN;
    }

    ret = ldg_mem_alloc(st.size, (void **)&media);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: disk buffer alloc refused; bytes: %u64; err: %u32", st.size, ret);

        return ret;
    }

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, CROSSTALK_EDA_IC_FPGA_EMU_FILE_MODE, &file);
    if (ret == LDG_ERR_AOK)
    {
        ret = ldg_io_file_rd(file, media, st.size, &bytes_rd);

        if (ldg_io_file_close(file) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu disk image close failed"); }
    }

    if (ret == LDG_ERR_AOK && bytes_rd != st.size) { ret = LDG_ERR_IO_RD; }

    if (ret != LDG_ERR_AOK)
    {
        ldg_mem_dealloc(media);

        return ret;
    }

    prev = ctx->disk[drive];

    ctx->disk[drive] = media;
    ctx->disk_span[drive] = st.size;

    if (ldg_str_rbrcpy(ctx->disk_path[drive], path, (uint64_t)sizeof(ctx->disk_path[drive])) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu disk path truncated"); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu: disk image rd; drive: %u32; path: %.127s; bytes: %u64", drive, path, ctx->disk_span[drive]);

    ret = crosstalk_eda_ic_fpga_emu_disk_bind(ctx, drive);

    if (prev != 0x0) { ldg_mem_dealloc(prev); }

    return ret;
}

static void emu_stat_u32_put(uint8_t *at, uint32_t val)
{
    at[0] = (uint8_t)(val & (uint32_t)LDG_BYTE_MASK);
    at[0x1] = (uint8_t)((val >> 8) & (uint32_t)LDG_BYTE_MASK);
    at[0x2] = (uint8_t)((val >> 16) & (uint32_t)LDG_BYTE_MASK);
    at[0x3] = (uint8_t)((val >> 24) & (uint32_t)LDG_BYTE_MASK);
}

static uint32_t emu_stat_u32_get(const uint8_t *at)
{
    return (uint32_t)at[0] | ((uint32_t)at[0x1] << 8) | ((uint32_t)at[0x2] << 16) | ((uint32_t)at[0x3] << 24);
}

static void emu_stat_u64_put(uint8_t *at, uint64_t val)
{
    emu_stat_u32_put(at, (uint32_t)(val & (uint64_t)UINT32_MAX));
    emu_stat_u32_put(at + 0x4, (uint32_t)(val >> 32));
}

static uint64_t emu_stat_u64_get(const uint8_t *at)
{
    return (uint64_t)emu_stat_u32_get(at) | ((uint64_t)emu_stat_u32_get(at + 0x4) << 32);
}

uint32_t crosstalk_eda_ic_fpga_emu_stat_init(crosstalk_eda_ic_fpga_emu_stat_t *stat)
{
    if (LDG_UNLIKELY(stat == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(stat, 0x0, sizeof(*stat));

    stat->cur = (uint32_t)UINT32_MAX;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_emu_stat_step(crosstalk_eda_ic_fpga_emu_stat_t *stat, uint8_t live_is, uint8_t hlt_is, uint64_t now_ms)
{
    uint64_t delta = 0;

    if (LDG_UNLIKELY(stat == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (live_is != LDG_TRUTH_TRUE)
    {
        stat->live_prev = LDG_TRUTH_FALSE;
        stat->step_at_ms = 0;

        return LDG_ERR_AOK;
    }

    if (stat->live_prev != LDG_TRUTH_TRUE)
    {
        stat->live_prev = LDG_TRUTH_TRUE;
        stat->step_at_ms = now_ms;
        stat->sesh_cunt = stat->sesh_cunt + 0x1;

        if (stat->first_epoch_ms == 0) { stat->first_epoch_ms = now_ms; }

        return LDG_ERR_AOK;
    }

    if (now_ms <= stat->step_at_ms) { return LDG_ERR_AOK; }

    delta = now_ms - stat->step_at_ms;
    stat->step_at_ms = now_ms;

    if (delta > (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_GAP_MS_MAX) { return LDG_ERR_AOK; }

    if (hlt_is == LDG_TRUTH_TRUE) { stat->hlt_ms = stat->hlt_ms + delta; }
    else { stat->run_ms = stat->run_ms + delta; }

    if (stat->cur >= stat->prog_cunt) { return LDG_ERR_AOK; }

    if (hlt_is == LDG_TRUTH_TRUE) { stat->prog[stat->cur].hlt_ms = stat->prog[stat->cur].hlt_ms + delta; }
    else { stat->prog[stat->cur].run_ms = stat->prog[stat->cur].run_ms + delta; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_emu_stat_prog_close(crosstalk_eda_ic_fpga_emu_stat_t *stat)
{
    if (LDG_UNLIKELY(stat == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    stat->cur = (uint32_t)UINT32_MAX;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_emu_stat_prog_open(crosstalk_eda_ic_fpga_emu_stat_t *stat, const char *name, uint64_t now_ms)
{
    uint32_t at = 0;
    uint32_t slot = (uint32_t)UINT32_MAX;

    if (LDG_UNLIKELY(stat == 0x0 || name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    stat->cur = (uint32_t)UINT32_MAX;

    if (name[0] == LDG_STR_TERM) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (at = 0; at < stat->prog_cunt; at = at + 0x1)
    {
        if (strncmp(stat->prog[at].name, name, (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_NAME_CAP) != 0) { continue; }

        slot = at;

        break;
    }

    if (slot == (uint32_t)UINT32_MAX && stat->prog_cunt < (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_PROG_MAX)
    {
        slot = stat->prog_cunt;
        stat->prog_cunt = stat->prog_cunt + 0x1;

        memset(&stat->prog[slot], 0x0, sizeof(stat->prog[slot]));

        if (ldg_str_rbrcpy(stat->prog[slot].name, name, (uint64_t)sizeof(stat->prog[slot].name)) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu stat prog name truncated"); }
    }

    if (slot == (uint32_t)UINT32_MAX)
    {
        uint64_t least = UINT64_MAX;

        for (at = 0; at < stat->prog_cunt; at = at + 0x1)
        {
            if (stat->prog[at].run_ms + stat->prog[at].hlt_ms > least) { continue; }

            least = stat->prog[at].run_ms + stat->prog[at].hlt_ms;
            slot = at;
        }

        if (slot == (uint32_t)UINT32_MAX) { return LDG_ERR_FULL; }

        memset(&stat->prog[slot], 0x0, sizeof(stat->prog[slot]));

        if (ldg_str_rbrcpy(stat->prog[slot].name, name, (uint64_t)sizeof(stat->prog[slot].name)) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu stat prog name truncated"); }
    }

    stat->prog[slot].launches = stat->prog[slot].launches + 0x1;
    stat->prog[slot].last_epoch_ms = now_ms;
    stat->cur = slot;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_emu_stat_typed_put(crosstalk_eda_ic_fpga_emu_stat_t *stat, const uint8_t *bytes, uint32_t len, uint64_t now_ms)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(stat == 0x0 || bytes == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (now_ms == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (at = 0; at < len; at = at + 0x1)
    {
        stat->keys = stat->keys + 0x1;

        if (bytes[at] == (uint8_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_BS) { stat->backspaces = stat->backspaces + 0x1; continue; }

        if (bytes[at] < (uint8_t)LDG_ASCII_PRINTABLE_MIN || bytes[at] > (uint8_t)LDG_ASCII_PRINTABLE_MAX) { continue; }

        stat->chars = stat->chars + 0x1;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_emu_stat_serialize(const crosstalk_eda_ic_fpga_emu_stat_t *stat, uint8_t *out, uint32_t cap, uint32_t *out_len)
{
    uint32_t totals_bytes = (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_TOTALS_WORDS * (uint32_t)sizeof(uint64_t);
    uint32_t typing_bytes = (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_TYPING_WORDS * (uint32_t)sizeof(uint64_t);
    uint32_t prog_bytes = (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_NAME_CAP + (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_PROG_WORDS * (uint32_t)sizeof(uint64_t);
    uint32_t progs_bytes = (uint32_t)sizeof(uint32_t) + stat->prog_cunt * prog_bytes;
    uint32_t need = 0;
    uint32_t at = 0;
    uint32_t pos = 0;

    if (LDG_UNLIKELY(stat == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    need = (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_HDR_BYTES + (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_BLK_CUNT * (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_BLK_HDR_BYTES + totals_bytes + progs_bytes + typing_bytes;

    *out_len = need;

    if (out == 0x0 || cap == 0) { return LDG_ERR_AOK; }

    if (cap < need) { return LDG_ERR_BOUNDS; }

    emu_stat_u32_put(out + pos, (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_MAGIC);
    emu_stat_u32_put(out + pos + 0x4, (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_REV);
    emu_stat_u32_put(out + pos + 0x8, (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_BLK_CUNT);
    pos = pos + (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_HDR_BYTES;

    emu_stat_u32_put(out + pos, (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_BLK_TOTALS);
    emu_stat_u32_put(out + pos + 0x4, totals_bytes);
    pos = pos + (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_BLK_HDR_BYTES;

    emu_stat_u64_put(out + pos, stat->run_ms);
    emu_stat_u64_put(out + pos + 0x8, stat->hlt_ms);
    emu_stat_u64_put(out + pos + 0x10, stat->sesh_cunt);
    emu_stat_u64_put(out + pos + 0x18, stat->instr);
    emu_stat_u64_put(out + pos + 0x20, stat->cycles);
    emu_stat_u64_put(out + pos + 0x28, stat->first_epoch_ms);
    pos = pos + totals_bytes;

    emu_stat_u32_put(out + pos, (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_BLK_PROGS);
    emu_stat_u32_put(out + pos + 0x4, progs_bytes);
    pos = pos + (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_BLK_HDR_BYTES;

    emu_stat_u32_put(out + pos, stat->prog_cunt);
    pos = pos + (uint32_t)sizeof(uint32_t);

    for (at = 0; at < stat->prog_cunt; at = at + 0x1)
    {
        memcpy(out + pos, stat->prog[at].name, (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_NAME_CAP);
        pos = pos + (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_NAME_CAP;

        emu_stat_u64_put(out + pos, stat->prog[at].run_ms);
        emu_stat_u64_put(out + pos + 0x8, stat->prog[at].hlt_ms);
        emu_stat_u64_put(out + pos + 0x10, stat->prog[at].launches);
        emu_stat_u64_put(out + pos + 0x18, stat->prog[at].last_epoch_ms);
        emu_stat_u64_put(out + pos + 0x20, stat->prog[at].instr);
        pos = pos + (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_PROG_WORDS * (uint32_t)sizeof(uint64_t);
    }

    emu_stat_u32_put(out + pos, (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_BLK_TYPING);
    emu_stat_u32_put(out + pos + 0x4, typing_bytes);
    pos = pos + (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_BLK_HDR_BYTES;

    emu_stat_u64_put(out + pos, stat->chars);
    emu_stat_u64_put(out + pos + 0x8, stat->keys);
    emu_stat_u64_put(out + pos + 0x10, stat->backspaces);
    emu_stat_u64_put(out + pos + 0x18, stat->rate_peak);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_emu_stat_deserialize(crosstalk_eda_ic_fpga_emu_stat_t *stat, const uint8_t *buff, uint32_t len)
{
    uint32_t prog_bytes = (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_NAME_CAP + (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_PROG_WORDS * (uint32_t)sizeof(uint64_t);
    uint32_t blk_cunt = 0;
    uint32_t blk_at = 0;
    uint32_t pos = 0;

    if (LDG_UNLIKELY(stat == 0x0 || buff == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len < (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_HDR_BYTES) { return LDG_ERR_BOUNDS; }

    if (emu_stat_u32_get(buff) != (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_MAGIC) { return LDG_ERR_IO_FMT; }

    if (emu_stat_u32_get(buff + 0x4) != (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_REV) { return LDG_ERR_UNSUPPORTED; }

    blk_cunt = emu_stat_u32_get(buff + 0x8);
    pos = (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_HDR_BYTES;

    for (blk_at = 0; blk_at < blk_cunt; blk_at = blk_at + 0x1)
    {
        uint32_t tag = 0;
        uint32_t span = 0;

        if (pos + (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_BLK_HDR_BYTES > len) { break; }

        tag = emu_stat_u32_get(buff + pos);
        span = emu_stat_u32_get(buff + pos + 0x4);
        pos = pos + (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_BLK_HDR_BYTES;

        if (pos + span > len) { break; }

        if (tag == (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_BLK_TOTALS && span >= (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_TOTALS_WORDS * (uint32_t)sizeof(uint64_t))
        {
            stat->run_ms = emu_stat_u64_get(buff + pos);
            stat->hlt_ms = emu_stat_u64_get(buff + pos + 0x8);
            stat->sesh_cunt = emu_stat_u64_get(buff + pos + 0x10);
            stat->instr = emu_stat_u64_get(buff + pos + 0x18);
            stat->cycles = emu_stat_u64_get(buff + pos + 0x20);
            stat->first_epoch_ms = emu_stat_u64_get(buff + pos + 0x28);
        }
        else if (tag == (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_BLK_TYPING && span >= (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_TYPING_WORDS * (uint32_t)sizeof(uint64_t))
        {
            stat->chars = emu_stat_u64_get(buff + pos);
            stat->keys = emu_stat_u64_get(buff + pos + 0x8);
            stat->backspaces = emu_stat_u64_get(buff + pos + 0x10);
            stat->rate_peak = emu_stat_u64_get(buff + pos + 0x18);
        }
        else if (tag == (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_BLK_PROGS && span >= (uint32_t)sizeof(uint32_t))
        {
            uint32_t cunt = emu_stat_u32_get(buff + pos);
            uint32_t rec_at = pos + (uint32_t)sizeof(uint32_t);
            uint32_t at = 0;

            if (cunt > (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_PROG_MAX) { cunt = (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_PROG_MAX; }

            for (at = 0; at < cunt; at = at + 0x1)
            {
                if (rec_at + prog_bytes > len) { break; }

                memcpy(stat->prog[at].name, buff + rec_at, (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_NAME_CAP);
                stat->prog[at].name[(uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_NAME_CAP - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
                rec_at = rec_at + (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_NAME_CAP;

                stat->prog[at].run_ms = emu_stat_u64_get(buff + rec_at);
                stat->prog[at].hlt_ms = emu_stat_u64_get(buff + rec_at + 0x8);
                stat->prog[at].launches = emu_stat_u64_get(buff + rec_at + 0x10);
                stat->prog[at].last_epoch_ms = emu_stat_u64_get(buff + rec_at + 0x18);
                stat->prog[at].instr = emu_stat_u64_get(buff + rec_at + 0x20);
                rec_at = rec_at + (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_PROG_WORDS * (uint32_t)sizeof(uint64_t);

                stat->prog_cunt = at + 0x1;
            }
        }

        pos = pos + span;
    }

    stat->cur = (uint32_t)UINT32_MAX;
    stat->live_prev = LDG_TRUTH_FALSE;
    stat->step_at_ms = 0;

    return LDG_ERR_AOK;
}

static uint32_t emu_disk_bkp_wr(crosstalk_eda_ic_fpga_emu_ctx_t *ctx, uint32_t drive)
{
    char bkp[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    ldg_io_info_t st = LDG_STRUCT_ZERO_INIT;
    ldg_io_file_t *file = 0x0;
    ldg_byte_t *bytes = 0x0;
    uint64_t got = 0;
    uint64_t done_len = 0;
    int32_t written = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    written = snprintf(bkp, sizeof(bkp), EMU_DISK_BKP_FMT, ctx->disk_path[drive], ldg_time_epoch_ms_get());
    if (written < 0 || (uint64_t)written >= (uint64_t)sizeof(bkp)) { return LDG_ERR_STR_TRUNC; }

    ret = ldg_io_path_info_get(ctx->disk_path[drive], &st);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (st.size == 0) { return LDG_ERR_EMPTY; }

    ret = ldg_mem_alloc(st.size, (void **)&bytes);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_open(ctx->disk_path[drive], LDG_IO_RD_ONLY, CROSSTALK_EDA_IC_FPGA_EMU_FILE_MODE, &file);
    if (ret == LDG_ERR_AOK)
    {
        ret = ldg_io_file_rd(file, bytes, st.size, &got);

        if (ldg_io_file_close(file) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu disk backup rd close failed"); }
    }

    if (ret == LDG_ERR_AOK && got != st.size) { ret = LDG_ERR_IO_RD; }

    if (ret == LDG_ERR_AOK)
    {
        ret = ldg_io_file_open(bkp, LDG_IO_WR_ONLY | LDG_IO_CREATE | LDG_IO_TRUNCATE, CROSSTALK_EDA_IC_FPGA_EMU_FILE_MODE, &file);
        if (ret == LDG_ERR_AOK)
        {
            ret = ldg_io_file_wr(file, bytes, st.size, &done_len);

            if (ldg_io_file_close(file) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu disk backup wr close failed"); }
        }
    }

    ldg_mem_dealloc(bytes);

    if (ret != LDG_ERR_AOK) { return ret; }

    if (done_len != st.size) { return LDG_ERR_IO_WR; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu: disk backed up before write; drive: %u32; path: %.191s; bytes: %u64", drive, bkp, st.size);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_emu_disk_sync(crosstalk_eda_ic_fpga_emu_ctx_t *ctx, uint32_t drive)
{
    ldg_io_file_t *file = 0x0;
    uint64_t bytes_wr = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (drive >= (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->disk_span[drive] == 0 || ctx->disk_path[drive][0] == LDG_STR_TERM) { return LDG_ERR_EMPTY; }

    if (ctx->abi.ok == LDG_TRUTH_TRUE && ctx->abi.disk_dirty_lo_get != 0x0 && ctx->abi.disk_dirty_hi_get != 0x0 && ctx->abi.disk_blk_width_get != 0x0)
    {
        uint64_t off = 0;
        uint64_t span = 0;
        uint32_t width = ctx->abi.disk_blk_width_get();
        uint64_t lo = ctx->abi.disk_dirty_lo_get(drive);
        uint64_t hi = ctx->abi.disk_dirty_hi_get(drive);

        if (lo == UINT64_MAX || hi == UINT64_MAX)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu: disk clean; drive: %u32; path: %.127s", drive, ctx->disk_path[drive]);

            return LDG_ERR_AOK;
        }

        if (width == 0 || hi < lo) { return LDG_ERR_IO_WR; }

        if (LDG_UNLIKELY(lo >= ctx->disk_span[drive] / width || hi >= ctx->disk_span[drive] / width)) { return LDG_ERR_BOUNDS; }

        off = (uint64_t)lo * (uint64_t)width;
        span = ((uint64_t)hi - (uint64_t)lo + (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_BLK_STEP) * (uint64_t)width;

        if (off >= ctx->disk_span[drive]) { return LDG_ERR_BOUNDS; }

        if (span > ctx->disk_span[drive] - off) { span = ctx->disk_span[drive] - off; }

        ret = emu_disk_bkp_wr(ctx, drive);
        if (ret != LDG_ERR_AOK)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: disk write back refused; the backup would not be written; drive: %u32; path: %.127s; err: %u32", drive, ctx->disk_path[drive], ret);

            return ret;
        }

        ret = ldg_io_file_open(ctx->disk_path[drive], LDG_IO_WR_ONLY, CROSSTALK_EDA_IC_FPGA_EMU_FILE_MODE, &file);
        if (ret != LDG_ERR_AOK) { return ret; }

        ret = ldg_io_file_seek(file, off, LDG_IO_SEEK_START);

        if (ret == LDG_ERR_AOK) { ret = ldg_io_file_wr(file, ctx->disk[drive] + off, span, &bytes_wr); }

        if (ldg_io_file_close(file) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu disk sync close failed"); }

        if (ret != LDG_ERR_AOK) { return ret; }

        if (bytes_wr != span) { return LDG_ERR_IO_WR; }

        if (ctx->abi.disk_dirty_clear != 0x0 && ctx->abi.disk_dirty_clear(drive) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu disk dirty clear failed"); }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu: disk range wr; drive: %u32; path: %.127s; blk lo: %u64; blk hi: %u64; off: %u64; bytes: %u64", drive, ctx->disk_path[drive], lo, hi, off, span);

        return LDG_ERR_AOK;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: disk write back skipped; the emulator cannot say which blocks are dirty, and a whole-image rewrite would drop anything past the block boundary; drive: %u32; path: %.127s; bytes: %u64", drive, ctx->disk_path[drive], ctx->disk_span[drive]);

    if (file != 0x0 || bytes_wr != 0) { return LDG_ERR_IO_WR; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_emu_disk_eject(crosstalk_eda_ic_fpga_emu_ctx_t *ctx, uint32_t drive)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (drive >= (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->disk[drive] != 0x0)
    {
        if (crosstalk_eda_ic_fpga_emu_disk_sync(ctx, drive) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: disk write back on eject failed; drive: %u32", drive); }

        if (ctx->abi.ok == LDG_TRUTH_TRUE && ctx->abi.disk_drive_set != 0x0) { if (ctx->abi.disk_drive_set(drive, 0x0, (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_DISK_SPAN_NONE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: disk unbind refused; media pointer still set in the emulator; drive: %u32", drive); } }

        ldg_mem_dealloc(ctx->disk[drive]);
    }

    ctx->disk[drive] = 0x0;
    ctx->disk_span[drive] = 0;
    ctx->disk_path[drive][0] = LDG_STR_TERM;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu: disk ejected; drive: %u32", drive);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_emu_detach(crosstalk_eda_ic_fpga_emu_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->img.ok == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_img_unld(&ctx->img); }

    if (ctx->blob != 0x0) { ldg_mem_dealloc(ctx->blob); }

    memset(&ctx->img, 0x0, sizeof(ctx->img));
    memset(&ctx->abi, 0x0, sizeof(ctx->abi));

    ctx->blob = 0x0;
    ctx->blob_span = 0;
    ctx->path[0] = LDG_STR_TERM;
    ctx->ok = LDG_TRUTH_FALSE;

    return ret;
}

uint32_t crosstalk_eda_ic_fpga_emu_shutdown(crosstalk_eda_ic_fpga_emu_ctx_t *ctx)
{
    uint32_t drive = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_eda_ic_fpga_emu_detach(ctx);

    if (ctx->pool_backing != 0x0) { if (ldg_mem_pool_destroy(&ctx->pool_backing) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu guest pool destroy failed"); } }

    for (drive = 0; drive < (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX; drive = drive + 0x1)
    {
        if (ctx->disk[drive] != 0x0)
        {
            if (ctx->disk_persist == LDG_TRUTH_TRUE && crosstalk_eda_ic_fpga_emu_disk_sync(ctx, drive) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: disk write back on shutdown failed; drive: %u32", drive); }

            ldg_mem_dealloc(ctx->disk[drive]);
        }

        ctx->disk[drive] = 0x0;
        ctx->disk_span[drive] = 0;
        ctx->disk_path[drive][0] = LDG_STR_TERM;
    }

    ctx->pool = 0x0;
    ctx->pool_span = 0;

    return ret;
}

uint8_t crosstalk_eda_ic_fpga_emu_live_is(const crosstalk_eda_ic_fpga_emu_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->ok == LDG_TRUTH_TRUE && ctx->abi.ok == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint8_t crosstalk_eda_ic_fpga_emu_stale_is(crosstalk_eda_ic_fpga_emu_ctx_t *ctx, double now)
{
    ldg_io_info_t st = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE || ctx->path[0] == LDG_STR_TERM) { return LDG_TRUTH_FALSE; }

    if (now < ctx->ck_next) { return LDG_TRUTH_FALSE; }

    ctx->ck_next = now + CROSSTALK_EDA_IC_FPGA_EMU_CK_SECS;

    if (ldg_io_path_info_get(ctx->path, &st) != LDG_ERR_AOK) { return LDG_TRUTH_FALSE; }

    if (st.size == 0) { return LDG_TRUTH_FALSE; }

    if (st.modified_ns == ctx->src_mtime_ns && st.size == ctx->src_span) { return LDG_TRUTH_FALSE; }

    if (ldg_time_epoch_ns_get() - st.modified_ns < (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_SETTLE_NS) { return LDG_TRUTH_FALSE; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu: the emulator changed on disk; path: %.127s; bytes: %u64 -> %u64", ctx->path, ctx->src_span, st.size);

    return LDG_TRUTH_TRUE;
}

uint32_t crosstalk_eda_ic_fpga_emu_abi_get(const crosstalk_eda_ic_fpga_emu_ctx_t *ctx, const crosstalk_hdl_abi_t **out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (ctx->ok != LDG_TRUTH_TRUE || ctx->abi.ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    *out = &ctx->abi;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_emu_rst(crosstalk_eda_ic_fpga_emu_ctx_t *ctx)
{
    uint32_t drive = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->abi.ok != LDG_TRUTH_TRUE || ctx->abi.init == 0x0) { return LDG_ERR_NOT_INIT; }

    if (ctx->abi.mem_init != 0x0 && ctx->pool != 0x0)
    {
        if (LDG_IS_POW2(ctx->pool_span) == 0 || ctx->pool_span < (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_POOL_BYTES_MIN || ctx->pool_span > (uint64_t)CROSSTALK_EDA_IC_FPGA_RAM_WINDOW_TOP)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "emu: the guest pool is not a legal memory width; bytes: %u64", ctx->pool_span);

            return CROSSTALK_EDA_IC_FPGA_EMU_ERR_POOL;
        }

        ret = ctx->abi.mem_init(ctx->pool, ctx->pool_span, (uint64_t)CROSSTALK_EDA_IC_FPGA_RAM_WINDOW_TOP);

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu: guest pool handed to the emulator; bytes: %u64; ram window top: 0x%X64; err: %u32", ctx->pool_span, (uint64_t)CROSSTALK_EDA_IC_FPGA_RAM_WINDOW_TOP, ret);

        if (ret != LDG_ERR_AOK) { return CROSSTALK_EDA_IC_FPGA_EMU_ERR_POOL; }
    }

    ctx->abi.init();

    if (ctx->abi.epoch_set != 0x0)
    {
        uint64_t epoch_ms = ldg_time_epoch_ms_get();

        if (LDG_UNLIKELY(epoch_ms == UINT64_MAX)) { return LDG_ERR_INVALID; }

        ret = ctx->abi.epoch_set(epoch_ms / LDG_MS_PER_SEC);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (ctx->pool != 0x0 && ctx->pool_span >= (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_CAP_MARK_SLOT)
    {
        uint64_t mark_at = ctx->pool_span - (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_CAP_MARK_SLOT;

        memcpy((ldg_byte_t *)ctx->pool + mark_at, CROSSTALK_EDA_IC_FPGA_EMU_CAP_MARK, (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_CAP_MARK_LEN);

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu: cap mark wr; mark: %s; addr: 0x%X64; word: %u64; pool bytes: %u64", CROSSTALK_EDA_IC_FPGA_EMU_CAP_MARK, mark_at, mark_at / (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_ELEM_BYTES, ctx->pool_span);
    }

    for (drive = 0; drive < (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX; drive = drive + 0x1)
    {
        if (ctx->disk_span[drive] == 0) { continue; }

        if (crosstalk_eda_ic_fpga_emu_disk_bind(ctx, drive) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: disk rebind failed; drive: %u32", drive); }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_emu_attach(crosstalk_eda_ic_fpga_emu_ctx_t *ctx, const char *path)
{
    ldg_io_info_t st = LDG_STRUCT_ZERO_INIT;
    ldg_io_file_t *file = 0x0;
    uint64_t bytes_rd = 0;
    uint32_t rev = CROSSTALK_EDA_IC_FPGA_EMU_REV_NONE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (path[0] == LDG_STR_TERM) { return CROSSTALK_EDA_IC_FPGA_EMU_ERR_PATH; }

    ret = crosstalk_eda_ic_fpga_emu_detach(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_io_path_info_get(path, &st);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: image not reachable; path: %.191s; err: %u32", path, ret);

        return CROSSTALK_EDA_IC_FPGA_EMU_ERR_PATH;
    }

    if (st.size == 0 || st.size > (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_SPAN_MAX)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: image span refused; path: %.191s; bytes: %u64", path, st.size);

        return CROSSTALK_EDA_IC_FPGA_EMU_ERR_SPAN;
    }

    ret = ldg_mem_alloc(st.size, (void **)&ctx->blob);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->blob_span = st.size;

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, CROSSTALK_EDA_IC_FPGA_EMU_FILE_MODE, &file);
    if (ret == LDG_ERR_AOK)
    {
        ret = ldg_io_file_rd(file, ctx->blob, ctx->blob_span, &bytes_rd);

        if (ldg_io_file_close(file) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu image close failed"); }
    }

    if (ret == LDG_ERR_AOK && bytes_rd != ctx->blob_span) { ret = LDG_ERR_IO_RD; }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_img_ld(ctx->blob, ctx->blob_span, &ctx->img); }

    if (ret == LDG_ERR_AOK)
    {
        static const uint32_t emu_rva[CROSSTALK_HDL_ABI_RVA_CUNT] = {
            DANGEMU_RVA_HDL_ABI_REV, DANGEMU_RVA_HDL_ABI_INIT, DANGEMU_RVA_HDL_ABI_MEM_INIT, DANGEMU_RVA_HDL_ABI_N_CYCLE, DANGEMU_RVA_HDL_ABI_SERIAL_DRAIN, DANGEMU_RVA_HDL_ABI_CYCLE, DANGEMU_RVA_HDL_ABI_SIG_FIND, DANGEMU_RVA_HDL_ABI_POKE, DANGEMU_RVA_HDL_ABI_PEEK, DANGEMU_RVA_HDL_ABI_MEM_ST, DANGEMU_RVA_HDL_ABI_MEM_LD, DANGEMU_RVA_HDL_ABI_STATE_GET, DANGEMU_RVA_HDL_ABI_TOUCH, DANGEMU_RVA_HDL_ABI_HLT_MASK_SET, DANGEMU_RVA_HDL_ABI_DISK_SET, DANGEMU_RVA_HDL_ABI_DISK_DRIVE_SET, DANGEMU_RVA_HDL_ABI_DISK_BLK_WIDTH_GET, DANGEMU_RVA_HDL_ABI_DISK_DIRTY_IS, DANGEMU_RVA_HDL_ABI_DISK_DIRTY_LO_GET, DANGEMU_RVA_HDL_ABI_DISK_DIRTY_HI_GET, DANGEMU_RVA_HDL_ABI_DISK_DIRTY_CLEAR, DANGEMU_RVA_HDL_ABI_AUDIO_DRAIN, DANGEMU_RVA_HDL_ABI_AUDIO_RATE_GET, DANGEMU_RVA_HDL_ABI_PROF_EN, DANGEMU_RVA_HDL_ABI_BRK_SET, DANGEMU_RVA_HDL_ABI_BRK_SKIP_SET, DANGEMU_RVA_HDL_ABI_BRK_COND_SET, DANGEMU_RVA_HDL_ABI_WATCH_SET, DANGEMU_RVA_HDL_ABI_TIME_SET, DANGEMU_RVA_HDL_ABI_IO_RD, DANGEMU_RVA_HDL_ABI_IO_MAP_GET, DANGEMU_RVA_HDL_ABI_IO_WR, DANGEMU_RVA_HDL_ABI_RELAY_REQ_TAKE, DANGEMU_RVA_HDL_ABI_RELAY_REPLY_PUSH, DANGEMU_RVA_HDL_ABI_RELAY_PAYLOAD_CAP_GET, [CROSSTALK_HDL_ABI_RVA_EPOCH_SET] = DANGEMU_RVA_HDL_ABI_EPOCH_SET, [CROSSTALK_HDL_ABI_RVA_DDU_FRAME_GET] = DANGEMU_RVA_HDL_ABI_DDU_FRAME_GET, [CROSSTALK_HDL_ABI_RVA_NET_DRAIN] = DANGEMU_RVA_HDL_ABI_NET_DRAIN, [CROSSTALK_HDL_ABI_RVA_NET_FEED] = DANGEMU_RVA_HDL_ABI_NET_FEED, [CROSSTALK_HDL_ABI_RVA_DDU_TEXT_GET] = DANGEMU_RVA_HDL_ABI_DDU_TEXT_GET
        };

        ret = crosstalk_hdl_abi_bind(&ctx->img, emu_rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_CUNT, &ctx->abi);
    }

    if (ret == LDG_ERR_AOK && ctx->abi.ok != LDG_TRUTH_TRUE) { ret = CROSSTALK_EDA_IC_FPGA_EMU_ERR_BIND; }

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: attach failed; path: %.127s; err: %u32", path, ret);

        if (crosstalk_eda_ic_fpga_emu_detach(ctx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu detach after failure failed"); }

        return ret;
    }

    if (ldg_str_rbrcpy(ctx->path, path, (uint64_t)sizeof(ctx->path)) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu path truncated"); }

    ctx->src_mtime_ns = st.modified_ns;
    ctx->src_span = st.size;
    ctx->ck_next = 0.0;

    if (ctx->abi.init == 0x0)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: the emulator has no init export; path: %.127s", path);

        if (crosstalk_eda_ic_fpga_emu_detach(ctx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu detach after failure failed"); }

        return CROSSTALK_EDA_IC_FPGA_EMU_ERR_BIND;
    }

    ret = crosstalk_eda_ic_fpga_emu_rst(ctx);
    if (ret != LDG_ERR_AOK)
    {
        if (crosstalk_eda_ic_fpga_emu_detach(ctx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu detach after failure failed"); }

        return ret;
    }

    ctx->ok = LDG_TRUTH_TRUE;

    if (ctx->abi.rev != 0x0) { rev = ctx->abi.rev(); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu: attached and reset; path: %.127s; bytes: %u64; abi rev: %u32", path, ctx->blob_span, rev);

    return LDG_ERR_AOK;
}
