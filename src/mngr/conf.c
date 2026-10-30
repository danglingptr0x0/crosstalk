#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <yder.h>

#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/file.h>
#include <dangling/io/path.h>
#include <dangling/str/str.h>

#include "mngr/conf.h"

#define MNGR_CONF_STEP 1
#define MNGR_CONF_MACHINE_WORDS 10
#define MNGR_CONF_JOIN_FMT "%s%s"
#define MNGR_CONF_DOT '.'
#define MNGR_CONF_SLASH '/'
#define MNGR_CONF_WIN_W_DEFAULT 1280
#define MNGR_CONF_WIN_H_DEFAULT 960
#define MNGR_CONF_GAMMA_DEFAULT 1.0

static void mngr_conf_u32_put(uint8_t *at, uint32_t val)
{
    at[0] = (uint8_t)(val & (uint32_t)LDG_BYTE_MASK);
    at[1] = (uint8_t)((val >> (uint32_t)LDG_BYTE_BITS) & (uint32_t)LDG_BYTE_MASK);
    at[2] = (uint8_t)((val >> (uint32_t)LDG_BYTE_SHIFT_2) & (uint32_t)LDG_BYTE_MASK);
    at[3] = (uint8_t)((val >> (uint32_t)LDG_BYTE_SHIFT_3) & (uint32_t)LDG_BYTE_MASK);
}

static uint32_t mngr_conf_u32_get(const uint8_t *at)
{
    return (uint32_t)at[0] | ((uint32_t)at[1] << (uint32_t)LDG_BYTE_BITS) | ((uint32_t)at[2] << (uint32_t)LDG_BYTE_SHIFT_2) | ((uint32_t)at[3] << (uint32_t)LDG_BYTE_SHIFT_3);
}

static void mngr_conf_u64_put(uint8_t *at, uint64_t val)
{
    mngr_conf_u32_put(at, (uint32_t)(val & (uint64_t)UINT32_MAX));
    mngr_conf_u32_put(at + (uint64_t)LDG_WORD_BYTES, (uint32_t)(val >> (uint64_t)(LDG_BYTE_BITS * LDG_WORD_BYTES)));
}

static uint64_t mngr_conf_u64_get(const uint8_t *at)
{
    return (uint64_t)mngr_conf_u32_get(at) | ((uint64_t)mngr_conf_u32_get(at + (uint64_t)LDG_WORD_BYTES) << (uint64_t)(LDG_BYTE_BITS * LDG_WORD_BYTES));
}

uint32_t crosstalk_mngr_conf_default_get(crosstalk_mngr_machine_t *out)
{
    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(out, 0x0, sizeof(*out));

    out->ram_bytes = (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_RAM_BYTES;
    out->f_op_hz = (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_CLK_HZ;
    out->gamma = MNGR_CONF_GAMMA_DEFAULT;
    out->win_w = (uint32_t)MNGR_CONF_WIN_W_DEFAULT;
    out->win_h = (uint32_t)MNGR_CONF_WIN_H_DEFAULT;
    out->key_mode = (uint32_t)CROSSTALK_EMU_INPUT_MODE_SCAN;
    out->audio_on = LDG_TRUTH_TRUE;
    out->serial_on = LDG_TRUTH_TRUE;
    out->disk_persist = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_mngr_conf_name_ok_is(const char *name)
{
    uint64_t at = 0;

    if (LDG_UNLIKELY(name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (name[0] == LDG_STR_TERM) { return LDG_TRUTH_FALSE; }

    for (at = 0; name[at] != LDG_STR_TERM; at = at + MNGR_CONF_STEP)
    {
        if (name[at] == MNGR_CONF_SLASH || name[at] == LDG_ASCII_BSLASH) { return LDG_TRUTH_FALSE; }

        if (name[at] == MNGR_CONF_DOT && name[at + MNGR_CONF_STEP] == MNGR_CONF_DOT) { return LDG_TRUTH_FALSE; }
    }

    if (at >= (uint64_t)CROSSTALK_MNGR_CONF_NAME_CAP) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

uint32_t crosstalk_mngr_conf_path_get(const char *dir, const char *name, char *out, uint64_t cap)
{
    char leaf[CROSSTALK_MNGR_CONF_NAME_CAP + sizeof(CROSSTALK_MNGR_CONF_EXT)] = LDG_ARR_ZERO_INIT;
    int32_t written = 0;

    if (LDG_UNLIKELY(dir == 0x0 || name == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (crosstalk_mngr_conf_name_ok_is(name) != LDG_TRUTH_TRUE) { return CROSSTALK_MNGR_ERR_NAME; }

    written = snprintf(leaf, sizeof(leaf), MNGR_CONF_JOIN_FMT, name, CROSSTALK_MNGR_CONF_EXT);
    if (written < 0 || (uint64_t)written >= (uint64_t)sizeof(leaf)) { return LDG_ERR_STR_TRUNC; }

    return ldg_io_path_join(dir, leaf, out, cap);
}

uint32_t crosstalk_mngr_conf_serialize(const crosstalk_mngr_machine_t *machine, uint8_t *out, uint32_t cap, uint32_t *out_len)
{
    uint32_t at = 0;
    uint32_t blk_at = 0;
    uint32_t drive = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(machine == 0x0 || out == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (cap < (uint32_t)CROSSTALK_MNGR_CONF_HDR_BYTES) { return CROSSTALK_MNGR_ERR_FULL; }

    mngr_conf_u32_put(out, (uint32_t)CROSSTALK_MNGR_CONF_MAGIC);
    mngr_conf_u32_put(out + LDG_WORD_BYTES, (uint32_t)CROSSTALK_MNGR_CONF_REV);
    mngr_conf_u32_put(out + (uint32_t)(LDG_WORD_BYTES * 2), (uint32_t)CROSSTALK_MNGR_CONF_BLK_CUNT);

    at = (uint32_t)CROSSTALK_MNGR_CONF_HDR_BYTES;

    blk_at = at;
    at = at + (uint32_t)CROSSTALK_MNGR_CONF_BLK_HDR_BYTES;

    if (cap < at + (uint32_t)sizeof(machine->name) + (uint32_t)sizeof(machine->note) + (uint32_t)sizeof(machine->core_path) + (uint32_t)sizeof(machine->font_path) + (uint32_t)(LDG_WORD_BYTES * MNGR_CONF_MACHINE_WORDS)) { return CROSSTALK_MNGR_ERR_FULL; }

    memcpy(out + at, machine->name, sizeof(machine->name));
    at = at + (uint32_t)sizeof(machine->name);

    memcpy(out + at, machine->note, sizeof(machine->note));
    at = at + (uint32_t)sizeof(machine->note);

    memcpy(out + at, machine->core_path, sizeof(machine->core_path));
    at = at + (uint32_t)sizeof(machine->core_path);

    memcpy(out + at, machine->font_path, sizeof(machine->font_path));
    at = at + (uint32_t)sizeof(machine->font_path);

    mngr_conf_u64_put(out + at, machine->ram_bytes);
    at = at + (uint32_t)(LDG_WORD_BYTES * 2);

    mngr_conf_u64_put(out + at, machine->f_op_hz);
    at = at + (uint32_t)(LDG_WORD_BYTES * 2);

    mngr_conf_u64_put(out + at, (uint64_t)(machine->gamma * (double)LDG_MS_PER_SEC));
    at = at + (uint32_t)(LDG_WORD_BYTES * 2);

    mngr_conf_u32_put(out + at, machine->win_w);
    at = at + (uint32_t)LDG_WORD_BYTES;

    mngr_conf_u32_put(out + at, machine->win_h);
    at = at + (uint32_t)LDG_WORD_BYTES;

    mngr_conf_u32_put(out + at, machine->key_mode);
    at = at + (uint32_t)LDG_WORD_BYTES;

    mngr_conf_u32_put(out + at, (uint32_t)machine->audio_on | ((uint32_t)machine->serial_on << (uint32_t)LDG_BYTE_BITS) | ((uint32_t)machine->disk_persist << (uint32_t)LDG_BYTE_SHIFT_2));
    at = at + (uint32_t)LDG_WORD_BYTES;

    mngr_conf_u32_put(out + blk_at, (uint32_t)CROSSTALK_MNGR_CONF_BLK_MACHINE);
    mngr_conf_u32_put(out + blk_at + LDG_WORD_BYTES, at - blk_at - (uint32_t)CROSSTALK_MNGR_CONF_BLK_HDR_BYTES);

    blk_at = at;
    at = at + (uint32_t)CROSSTALK_MNGR_CONF_BLK_HDR_BYTES;

    if (cap < at + (uint32_t)LDG_WORD_BYTES + (uint32_t)CROSSTALK_EMU_LD_MAX * ((uint32_t)LDG_IO_PATH_MAX + (uint32_t)(LDG_WORD_BYTES * 2))) { return CROSSTALK_MNGR_ERR_FULL; }

    mngr_conf_u32_put(out + at, machine->ld_cunt);
    at = at + (uint32_t)LDG_WORD_BYTES;

    for (idx = 0; idx < (uint32_t)CROSSTALK_EMU_LD_MAX; idx = idx + MNGR_CONF_STEP)
    {
        memcpy(out + at, machine->ld_path[idx], sizeof(machine->ld_path[idx]));
        at = at + (uint32_t)sizeof(machine->ld_path[idx]);

        mngr_conf_u64_put(out + at, machine->ld_off[idx]);
        at = at + (uint32_t)(LDG_WORD_BYTES * 2);
    }

    mngr_conf_u32_put(out + blk_at, (uint32_t)CROSSTALK_MNGR_CONF_BLK_IMGS);
    mngr_conf_u32_put(out + blk_at + LDG_WORD_BYTES, at - blk_at - (uint32_t)CROSSTALK_MNGR_CONF_BLK_HDR_BYTES);

    blk_at = at;
    at = at + (uint32_t)CROSSTALK_MNGR_CONF_BLK_HDR_BYTES;

    if (cap < at + (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX * (uint32_t)LDG_IO_PATH_MAX) { return CROSSTALK_MNGR_ERR_FULL; }

    for (drive = 0; drive < (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX; drive = drive + MNGR_CONF_STEP)
    {
        memcpy(out + at, machine->disk_path[drive], sizeof(machine->disk_path[drive]));
        at = at + (uint32_t)sizeof(machine->disk_path[drive]);
    }

    mngr_conf_u32_put(out + blk_at, (uint32_t)CROSSTALK_MNGR_CONF_BLK_DISKS);
    mngr_conf_u32_put(out + blk_at + LDG_WORD_BYTES, at - blk_at - (uint32_t)CROSSTALK_MNGR_CONF_BLK_HDR_BYTES);

    blk_at = at;
    at = at + (uint32_t)CROSSTALK_MNGR_CONF_BLK_HDR_BYTES;

    if (cap < at + (uint32_t)sizeof(machine->net_path)) { return CROSSTALK_MNGR_ERR_FULL; }

    memcpy(out + at, machine->net_path, sizeof(machine->net_path));
    at = at + (uint32_t)sizeof(machine->net_path);

    mngr_conf_u32_put(out + blk_at, (uint32_t)CROSSTALK_MNGR_CONF_BLK_NET);
    mngr_conf_u32_put(out + blk_at + LDG_WORD_BYTES, at - blk_at - (uint32_t)CROSSTALK_MNGR_CONF_BLK_HDR_BYTES);

    *out_len = at;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_mngr_conf_deserialize(crosstalk_mngr_machine_t *out, const uint8_t *buff, uint32_t len)
{
    uint32_t at = 0;
    uint32_t blks = 0;
    uint32_t seen = 0;
    uint32_t drive = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(out == 0x0 || buff == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len < (uint32_t)CROSSTALK_MNGR_CONF_HDR_BYTES) { return CROSSTALK_MNGR_ERR_TRUNC; }

    if (mngr_conf_u32_get(buff) != (uint32_t)CROSSTALK_MNGR_CONF_MAGIC) { return CROSSTALK_MNGR_ERR_MAGIC; }

    if (mngr_conf_u32_get(buff + LDG_WORD_BYTES) != (uint32_t)CROSSTALK_MNGR_CONF_REV) { return CROSSTALK_MNGR_ERR_REV; }

    blks = mngr_conf_u32_get(buff + (uint32_t)(LDG_WORD_BYTES * 2));

    if (crosstalk_mngr_conf_default_get(out) != LDG_ERR_AOK) { return LDG_ERR_FUNC_ARG_INVALID; }

    at = (uint32_t)CROSSTALK_MNGR_CONF_HDR_BYTES;

    for (seen = 0; seen < blks; seen = seen + MNGR_CONF_STEP)
    {
        uint32_t kind = 0;
        uint32_t span = 0;
        uint32_t body = 0;

        if (at + (uint32_t)CROSSTALK_MNGR_CONF_BLK_HDR_BYTES > len) { return CROSSTALK_MNGR_ERR_TRUNC; }

        kind = mngr_conf_u32_get(buff + at);
        span = mngr_conf_u32_get(buff + at + LDG_WORD_BYTES);
        body = at + (uint32_t)CROSSTALK_MNGR_CONF_BLK_HDR_BYTES;

        if (body + span > len) { return CROSSTALK_MNGR_ERR_TRUNC; }

        if (kind == (uint32_t)CROSSTALK_MNGR_CONF_BLK_MACHINE)
        {
            uint32_t flags = 0;
            uint32_t want = (uint32_t)sizeof(out->name) + (uint32_t)sizeof(out->note) + (uint32_t)sizeof(out->core_path) + (uint32_t)sizeof(out->font_path) + (uint32_t)(LDG_WORD_BYTES * MNGR_CONF_MACHINE_WORDS);

            if (span < want) { return CROSSTALK_MNGR_ERR_BLK; }

            memcpy(out->name, buff + body, sizeof(out->name));
            body = body + (uint32_t)sizeof(out->name);

            memcpy(out->note, buff + body, sizeof(out->note));
            body = body + (uint32_t)sizeof(out->note);

            memcpy(out->core_path, buff + body, sizeof(out->core_path));
            body = body + (uint32_t)sizeof(out->core_path);

            memcpy(out->font_path, buff + body, sizeof(out->font_path));
            body = body + (uint32_t)sizeof(out->font_path);

            out->ram_bytes = mngr_conf_u64_get(buff + body);
            body = body + (uint32_t)(LDG_WORD_BYTES * 2);

            out->f_op_hz = mngr_conf_u64_get(buff + body);
            body = body + (uint32_t)(LDG_WORD_BYTES * 2);

            out->gamma = (double)mngr_conf_u64_get(buff + body) / (double)LDG_MS_PER_SEC;
            body = body + (uint32_t)(LDG_WORD_BYTES * 2);

            out->win_w = mngr_conf_u32_get(buff + body);
            body = body + (uint32_t)LDG_WORD_BYTES;

            out->win_h = mngr_conf_u32_get(buff + body);
            body = body + (uint32_t)LDG_WORD_BYTES;

            out->key_mode = mngr_conf_u32_get(buff + body);
            body = body + (uint32_t)LDG_WORD_BYTES;

            flags = mngr_conf_u32_get(buff + body);

            out->audio_on = (uint8_t)(flags & (uint32_t)LDG_BYTE_MASK);
            out->serial_on = (uint8_t)((flags >> (uint32_t)LDG_BYTE_BITS) & (uint32_t)LDG_BYTE_MASK);
            out->disk_persist = (uint8_t)((flags >> (uint32_t)LDG_BYTE_SHIFT_2) & (uint32_t)LDG_BYTE_MASK);

            out->name[sizeof(out->name) - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
            out->note[sizeof(out->note) - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
            out->core_path[sizeof(out->core_path) - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
            out->font_path[sizeof(out->font_path) - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
        }
        else if (kind == (uint32_t)CROSSTALK_MNGR_CONF_BLK_IMGS)
        {
            if (span < (uint32_t)LDG_WORD_BYTES + (uint32_t)CROSSTALK_EMU_LD_MAX * ((uint32_t)LDG_IO_PATH_MAX + (uint32_t)(LDG_WORD_BYTES * 2))) { return CROSSTALK_MNGR_ERR_BLK; }

            out->ld_cunt = mngr_conf_u32_get(buff + body);
            body = body + (uint32_t)LDG_WORD_BYTES;

            if (out->ld_cunt > (uint32_t)CROSSTALK_EMU_LD_MAX) { out->ld_cunt = (uint32_t)CROSSTALK_EMU_LD_MAX; }

            for (idx = 0; idx < (uint32_t)CROSSTALK_EMU_LD_MAX; idx = idx + MNGR_CONF_STEP)
            {
                memcpy(out->ld_path[idx], buff + body, sizeof(out->ld_path[idx]));
                out->ld_path[idx][sizeof(out->ld_path[idx]) - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
                body = body + (uint32_t)sizeof(out->ld_path[idx]);

                out->ld_off[idx] = mngr_conf_u64_get(buff + body);
                body = body + (uint32_t)(LDG_WORD_BYTES * 2);
            }
        }
        else if (kind == (uint32_t)CROSSTALK_MNGR_CONF_BLK_DISKS)
        {
            if (span < (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX * (uint32_t)LDG_IO_PATH_MAX) { return CROSSTALK_MNGR_ERR_BLK; }

            for (drive = 0; drive < (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX; drive = drive + MNGR_CONF_STEP)
            {
                memcpy(out->disk_path[drive], buff + body, sizeof(out->disk_path[drive]));
                out->disk_path[drive][sizeof(out->disk_path[drive]) - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
                body = body + (uint32_t)sizeof(out->disk_path[drive]);
            }
        }
        else if (kind == (uint32_t)CROSSTALK_MNGR_CONF_BLK_NET)
        {
            if (span < (uint32_t)sizeof(out->net_path)) { return CROSSTALK_MNGR_ERR_BLK; }

            memcpy(out->net_path, buff + body, sizeof(out->net_path));
            out->net_path[sizeof(out->net_path) - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
        }

        at = at + (uint32_t)CROSSTALK_MNGR_CONF_BLK_HDR_BYTES + span;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_mngr_conf_wr(const crosstalk_mngr_machine_t *machine, const char *path)
{
    uint8_t blob[CROSSTALK_MNGR_CONF_BLOB_CAP] = LDG_ARR_ZERO_INIT;
    ldg_io_file_t *file = 0x0;
    uint64_t done_len = 0;
    uint32_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(machine == 0x0 || path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_mngr_conf_serialize(machine, blob, (uint32_t)sizeof(blob), &len);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_open(path, LDG_IO_WR_ONLY | LDG_IO_CREATE | LDG_IO_TRUNCATE, CROSSTALK_EDA_IC_FPGA_EMU_FILE_MODE, &file);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_wr(file, blob, (uint64_t)len, &done_len);

    if (ldg_io_file_close(file) != LDG_ERR_AOK) { LDG_ERRLOG_WARN("mngr conf close failed"); }

    if (ret != LDG_ERR_AOK) { return ret; }

    if (done_len != (uint64_t)len) { return LDG_ERR_IO_WR; }

    y_log_message(Y_LOG_LEVEL_INFO, "mngr: machine written; name: %.47s; path: %.191s; bytes: %u", machine->name, path, len);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_mngr_conf_rd(crosstalk_mngr_machine_t *out, const char *path)
{
    uint8_t blob[CROSSTALK_MNGR_CONF_BLOB_CAP] = LDG_ARR_ZERO_INIT;
    ldg_io_info_t st = LDG_STRUCT_ZERO_INIT;
    ldg_io_file_t *file = 0x0;
    uint64_t got = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(out == 0x0 || path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_io_path_info_get(path, &st);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (st.size == 0 || st.size > (uint64_t)sizeof(blob)) { return CROSSTALK_MNGR_ERR_TRUNC; }

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, CROSSTALK_EDA_IC_FPGA_EMU_FILE_MODE, &file);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_rd(file, blob, st.size, &got);

    if (ldg_io_file_close(file) != LDG_ERR_AOK) { LDG_ERRLOG_WARN("mngr conf close failed"); }

    if (ret != LDG_ERR_AOK) { return ret; }

    if (got != st.size) { return LDG_ERR_IO_RD; }

    return crosstalk_mngr_conf_deserialize(out, blob, (uint32_t)st.size);
}
