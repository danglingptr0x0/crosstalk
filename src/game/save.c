#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "core/log.h"

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>
#include <dangling/mem/alloc.h>
#include <dangling/io/file.h>
#include <dangling/io/path.h>
#include <dangling/time/time.h>
#include <dangling/proto/emiemi.h>

#include "game/save.h"

#define GAME_SAVE_REVISION 2U
#define GAME_SAVE_FILE_MODE 0644
#define GAME_SAVE_AUTHOR_NAME_CAP 64
#define GAME_SAVE_PROJ_NAME_CAP 64
#define GAME_SAVE_COMPANY_CAP 64
#define GAME_SAVE_BKP_SUFFIX ".bkp"
#define GAME_SAVE_BKP_CHUNK 4096
#define GAME_SAVE_SIZE_PROBE 0
#define GAME_SAVE_MAGIC_LEN 4
#define GAME_SAVE_DIFFICULTY_NONE 0
#define GAME_SAVE_SECT_MAX 16
#define GAME_SAVE_SECT_NAME_CAP 8
#define GAME_SAVE_SECT_ECON "ECON"
#define GAME_SAVE_SECT_PCB "PCB"
#define GAME_SAVE_SECT_DISTRICT "DISTRICT"
#define GAME_SAVE_SECT_SETTINGS "SETTINGS"
#define GAME_SAVE_SECT_LASTSTAT "LASTSTAT"
#define GAME_SAVE_SECT_CREDIT "CREDIT"
#define GAME_SAVE_SECT_BENCH "BENCH"

typedef struct game_save_hdr
{
    ldg_byte_t magic[GAME_SAVE_MAGIC_LEN];
    uint32_t revision;
    uint32_t sect_cunt;
    uint32_t sect_table_off;
    uint32_t difficulty;
    char author[GAME_SAVE_AUTHOR_NAME_CAP];
    char proj_name[GAME_SAVE_PROJ_NAME_CAP];
    char company[GAME_SAVE_COMPANY_CAP];
    uint8_t pudding[4];
    uint64_t author_steam_id;
    uint64_t created_epoch_ms;
    uint64_t work_ms_total;
} game_save_hdr_t;

typedef struct game_save_sect_desc
{
    uint32_t off;
    uint32_t len;
    char name[GAME_SAVE_SECT_NAME_CAP];
} game_save_sect_desc_t;

typedef struct game_save_sect_stats
{
    uint64_t work_ms;
    uint64_t last_active_epoch_ms;
} game_save_sect_stats_t;

typedef struct game_save_bench_stats
{
    uint64_t bench_ms;
    uint64_t sim_ms;
} game_save_bench_stats_t;

typedef struct game_save_sect
{
    const char *name;
    ldg_byte_t *buff;
    uint32_t len;
    uint8_t pudding[4];
} game_save_sect_t;

static uint32_t save_hdr_parse(const ldg_byte_t *payload, ldg_dword_t payload_len, game_save_hdr_t *hdr)
{
    uint64_t table_end = 0;

    if (LDG_UNLIKELY(payload == 0x0 || hdr == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(payload_len < (ldg_dword_t)sizeof(game_save_hdr_t))) { return LDG_ERR_IO_FMT; }

    memcpy(hdr, payload, sizeof(game_save_hdr_t));
    if (LDG_UNLIKELY(hdr->magic[0] != 'X' || hdr->magic[1] != 'T' || hdr->magic[2] != 'L' || hdr->magic[3] != 'K')) { return LDG_ERR_IO_FMT; }

    if (LDG_UNLIKELY(hdr->revision != GAME_SAVE_REVISION)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(hdr->sect_cunt > GAME_SAVE_SECT_MAX)) { return LDG_ERR_IO_FMT; }

    if (LDG_UNLIKELY(hdr->sect_table_off < (uint32_t)sizeof(game_save_hdr_t))) { return LDG_ERR_IO_FMT; }

    table_end = (uint64_t)hdr->sect_table_off + (uint64_t)hdr->sect_cunt * (uint64_t)sizeof(game_save_sect_desc_t);
    if (LDG_UNLIKELY(table_end != (uint64_t)payload_len)) { return LDG_ERR_IO_FMT; }

    return LDG_ERR_AOK;
}

static uint32_t save_sect_find(const ldg_byte_t *payload, ldg_dword_t payload_len, const char *name, const ldg_byte_t **out_data, uint32_t *out_len)
{
    game_save_hdr_t hdr = LDG_STRUCT_ZERO_INIT;
    game_save_sect_desc_t desc = LDG_STRUCT_ZERO_INIT;
    char key[GAME_SAVE_SECT_NAME_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t name_len = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(payload == 0x0 || name == 0x0 || out_data == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_data = 0x0;
    *out_len = 0;

    name_len = strlen(name);
    if (LDG_UNLIKELY(name_len > (uint64_t)sizeof(key))) { return LDG_ERR_FUNC_ARG_INVALID; }

    memcpy(key, name, name_len);

    if (LDG_UNLIKELY(payload_len < (ldg_dword_t)sizeof(hdr))) { return LDG_ERR_IO_FMT; }

    memcpy(&hdr, payload, sizeof(hdr));
    for (idx = 0; idx < hdr.sect_cunt; idx = idx + 0x1)
    {
        uint64_t desc_at = (uint64_t)hdr.sect_table_off + (uint64_t)idx * (uint64_t)sizeof(desc);

        if (LDG_UNLIKELY(desc_at + (uint64_t)sizeof(desc) > (uint64_t)payload_len)) { return LDG_ERR_IO_FMT; }

        memcpy(&desc, payload + desc_at, sizeof(desc));
        if (memcmp(desc.name, key, sizeof(key)) != 0x0) { continue; }

        if (LDG_UNLIKELY(desc.off > payload_len || desc.len > payload_len - desc.off)) { return LDG_ERR_IO_FMT; }

        *out_data = payload + desc.off;
        *out_len = desc.len;
        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t save_sect_add(game_save_sect_t *sects, uint32_t *cunt, const char *name, ldg_byte_t *buff, uint32_t len)
{
    if (LDG_UNLIKELY(sects == 0x0 || cunt == 0x0 || name == 0x0 || buff == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(strlen(name) > (uint64_t)GAME_SAVE_SECT_NAME_CAP)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(*cunt >= GAME_SAVE_SECT_MAX)) { return LDG_ERR_FULL; }

    sects[*cunt].name = name;
    sects[*cunt].buff = buff;
    sects[*cunt].len = len;
    *cunt = *cunt + 0x1;
    return LDG_ERR_AOK;
}

static uint32_t save_proj_name_get(const char *path, char *out, uint64_t cap)
{
    char base[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char *dot = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(path == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;

    ret = ldg_io_path_leaf_get(path, base, (uint64_t)sizeof(base));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    dot = strrchr(base, '.');
    if (dot != 0x0) { *dot = LDG_STR_TERM; }

    ret = ldg_str_rbrcpy(out, base, cap);
    if (ret == LDG_ERR_STR_TRUNC) { ret = LDG_ERR_AOK; }

    return ret;
}

static uint32_t bkp_wr(const char *path)
{
    ldg_io_file_t *src = 0x0;
    ldg_io_file_t *dst = 0x0;
    ldg_byte_t chunk[GAME_SAVE_BKP_CHUNK] = LDG_ARR_ZERO_INIT;
    char bkp_path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t bytes_rd = 0;
    uint64_t bytes_wr = 0;
    int32_t written = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ldg_io_path_exists_is(path) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(strlen(path) + sizeof(GAME_SAVE_BKP_SUFFIX) > sizeof(bkp_path))) { return LDG_ERR_STR_TRUNC; }

    written = snprintf(bkp_path, sizeof(bkp_path), "%.*s" GAME_SAVE_BKP_SUFFIX, (int32_t)(sizeof(bkp_path) - sizeof(GAME_SAVE_BKP_SUFFIX)), path);
    if (LDG_UNLIKELY(written <= 0)) { return LDG_ERR_STR_FMT; }

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, GAME_SAVE_FILE_MODE, &src);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_io_file_open(bkp_path, LDG_IO_WR_ONLY | LDG_IO_CREATE | LDG_IO_TRUNCATE, GAME_SAVE_FILE_MODE, &dst);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_io_file_close(src);
        return ret;
    }

    for (;;)
    {
        bytes_rd = 0;
        ret = ldg_io_file_rd(src, chunk, sizeof(chunk), &bytes_rd);
        if (ret == LDG_ERR_EOF || (ret == LDG_ERR_AOK && bytes_rd == 0))
        {
            ret = LDG_ERR_AOK;
            break;
        }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

        bytes_wr = 0;
        ret = ldg_io_file_wr(dst, chunk, bytes_rd, &bytes_wr);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

        if (LDG_UNLIKELY(bytes_wr != bytes_rd))
        {
            ret = LDG_ERR_IO_WR;
            break;
        }
    }

    if (ret == LDG_ERR_AOK) { ret = ldg_io_file_commit(dst); }

    if (ret == LDG_ERR_AOK) { ret = ldg_io_file_close(dst); }
    else { ldg_io_file_close(dst); }

    ldg_io_file_close(src);
    return ret;
}

uint32_t crosstalk_game_save_wr(const crosstalk_game_settings_t *settings, const crosstalk_game_last_state_t *state, uint64_t author_steam_id, const char *author_name, uint64_t created_epoch_ms, uint64_t bench_ms_total, uint64_t sim_ms_total, const crosstalk_game_save_editor_sect_t *editor_sects, uint32_t editor_sect_cunt, const char *path)
{
    game_save_hdr_t hdr = LDG_STRUCT_ZERO_INIT;
    game_save_sect_t sects[GAME_SAVE_SECT_MAX] = LDG_ARR_ZERO_INIT;
    ldg_byte_t *payload = 0x0;
    ldg_byte_t *frame = 0x0;
    ldg_io_file_t *file = 0x0;
    uint64_t bytes_wr = 0;
    uint32_t sect_cunt = 0;
    uint32_t payload_len = 0;
    uint32_t idx = 0;
    ldg_dword_t frame_len = 0;
    ldg_dword_t checksum = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    {
        game_save_bench_stats_t bench = LDG_STRUCT_ZERO_INIT;
        ldg_byte_t *bench_buff = 0x0;

        bench.bench_ms = bench_ms_total;
        bench.sim_ms = sim_ms_total;

        ret = ldg_mem_alloc((uint64_t)sizeof(bench), (void **)&bench_buff);
        if (ret == LDG_ERR_AOK)
        {
            memcpy(bench_buff, &bench, sizeof(bench));
            ret = save_sect_add(sects, &sect_cunt, GAME_SAVE_SECT_BENCH, bench_buff, (uint32_t)sizeof(bench));
        }

        if (ret != LDG_ERR_AOK && bench_buff != 0x0) { ldg_mem_dealloc(bench_buff); }
    }

    if (ret == LDG_ERR_AOK && settings != 0x0)
    {
        ldg_byte_t *settings_buff = 0x0;
        uint32_t settings_len = 0;
        ret = crosstalk_game_settings_serialize(settings, 0x0, GAME_SAVE_SIZE_PROBE, &settings_len);
        if (ret == LDG_ERR_AOK && settings_len > 0) { ret = ldg_mem_alloc((uint64_t)settings_len, (void **)&settings_buff); }

        if (ret == LDG_ERR_AOK && settings_buff != 0x0) { ret = crosstalk_game_settings_serialize(settings, settings_buff, settings_len, &settings_len); }

        if (ret == LDG_ERR_AOK && settings_buff != 0x0) { ret = save_sect_add(sects, &sect_cunt, GAME_SAVE_SECT_SETTINGS, settings_buff, settings_len); }

        if (ret != LDG_ERR_AOK && settings_buff != 0x0) { ldg_mem_dealloc(settings_buff); }
    }

    if (ret == LDG_ERR_AOK && state != 0x0)
    {
        ldg_byte_t *state_buff = 0x0;
        uint32_t state_len = 0;
        ret = crosstalk_game_last_state_serialize(state, 0x0, GAME_SAVE_SIZE_PROBE, &state_len);
        if (ret == LDG_ERR_AOK && state_len > 0) { ret = ldg_mem_alloc((uint64_t)state_len, (void **)&state_buff); }

        if (ret == LDG_ERR_AOK && state_buff != 0x0) { ret = crosstalk_game_last_state_serialize(state, state_buff, state_len, &state_len); }

        if (ret == LDG_ERR_AOK && state_buff != 0x0) { ret = save_sect_add(sects, &sect_cunt, GAME_SAVE_SECT_LASTSTAT, state_buff, state_len); }

        if (ret != LDG_ERR_AOK && state_buff != 0x0) { ldg_mem_dealloc(state_buff); }
    }

    if (ret == LDG_ERR_AOK && editor_sects != 0x0)
    {
        for (idx = 0; idx < editor_sect_cunt; idx = idx + 0x1)
        {
            game_save_sect_stats_t stats = LDG_STRUCT_ZERO_INIT;
            ldg_byte_t *editor_buff = 0x0;
            uint32_t editor_len = 0;

            if (editor_sects[idx].blob == 0x0 || editor_sects[idx].blob_len == 0) { continue; }

            stats.work_ms = editor_sects[idx].work_ms;
            stats.last_active_epoch_ms = ldg_time_epoch_ms_get();
            editor_len = (uint32_t)sizeof(stats) + editor_sects[idx].blob_len;
            ret = ldg_mem_alloc((uint64_t)editor_len, (void **)&editor_buff);
            if (ret == LDG_ERR_AOK)
            {
                memcpy(editor_buff, &stats, sizeof(stats));
                memcpy(editor_buff + sizeof(stats), editor_sects[idx].blob, editor_sects[idx].blob_len);
                ret = save_sect_add(sects, &sect_cunt, editor_sects[idx].name, editor_buff, editor_len);
            }

            if (ret != LDG_ERR_AOK)
            {
                if (editor_buff != 0x0) { ldg_mem_dealloc(editor_buff); }

                break;
            }
        }
    }

    if (ret == LDG_ERR_AOK)
    {
        hdr.magic[0] = 'X';
        hdr.magic[1] = 'T';
        hdr.magic[2] = 'L';
        hdr.magic[3] = 'K';
        hdr.revision = GAME_SAVE_REVISION;
        hdr.sect_cunt = sect_cunt;
        hdr.difficulty = GAME_SAVE_DIFFICULTY_NONE;
        snprintf(hdr.author, sizeof(hdr.author), "%.63s", author_name != 0x0 ? author_name : "");
        ret = save_proj_name_get(path, hdr.proj_name, (uint64_t)sizeof(hdr.proj_name));
        if (ret != LDG_ERR_AOK)
        {
            hdr.proj_name[0] = LDG_STR_TERM;
            ret = LDG_ERR_AOK;
        }

        hdr.author_steam_id = author_steam_id;
        hdr.created_epoch_ms = created_epoch_ms;
        hdr.work_ms_total = bench_ms_total;
        hdr.sect_table_off = (uint32_t)sizeof(hdr);
        for (idx = 0; idx < sect_cunt; idx = idx + 0x1) { hdr.sect_table_off = hdr.sect_table_off + sects[idx].len; }

        payload_len = hdr.sect_table_off + sect_cunt * (uint32_t)sizeof(game_save_sect_desc_t);
        ret = ldg_mem_alloc((uint64_t)payload_len, (void **)&payload);
    }

    if (ret == LDG_ERR_AOK)
    {
        uint32_t at = (uint32_t)sizeof(hdr);
        uint32_t table_at = hdr.sect_table_off;
        memcpy(payload, &hdr, sizeof(hdr));
        for (idx = 0; idx < sect_cunt; idx = idx + 0x1)
        {
            game_save_sect_desc_t desc = LDG_STRUCT_ZERO_INIT;
            uint64_t name_len = strlen(sects[idx].name);
            desc.off = at;
            desc.len = sects[idx].len;
            memcpy(desc.name, sects[idx].name, name_len);
            if (sects[idx].len > 0) { memcpy(payload + at, sects[idx].buff, sects[idx].len); }

            memcpy(payload + table_at, &desc, sizeof(desc));
            at = at + sects[idx].len;
            table_at = table_at + (uint32_t)sizeof(desc);
        }
    }

    for (idx = 0; idx < sect_cunt; idx = idx + 0x1) { if (sects[idx].buff != 0x0)
        {
            ldg_mem_dealloc(sects[idx].buff);
            sects[idx].buff = 0x0;
        }
    }

    if (ret == LDG_ERR_AOK)
    {
        frame_len = ldg_emiemi_encoded_size_get(payload_len);
        ret = ldg_mem_alloc((uint64_t)frame_len, (void **)&frame);
    }

    if (ret == LDG_ERR_AOK) { ret = ldg_emiemi_encode(payload, payload_len, frame, frame_len, &checksum); }

    if (payload != 0x0)
    {
        ldg_mem_dealloc(payload);
        payload = 0x0;
    }

    if (ret == LDG_ERR_AOK)
    {
        ret = bkp_wr(path);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "save bkp copy failed; refusing overwrite"); }
    }

    if (ret == LDG_ERR_AOK) { ret = ldg_io_file_open(path, LDG_IO_WR_ONLY | LDG_IO_CREATE | LDG_IO_TRUNCATE, GAME_SAVE_FILE_MODE, &file); }

    if (ret == LDG_ERR_AOK && file != 0x0)
    {
        ret = ldg_io_file_wr(file, frame, (uint64_t)frame_len, &bytes_wr);
        if (ret == LDG_ERR_AOK) { ret = ldg_io_file_commit(file); }

        if (ret == LDG_ERR_AOK) { ret = ldg_io_file_close(file); }
        else { ldg_io_file_close(file); }
    }

    if (frame != 0x0) { ldg_mem_dealloc(frame); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(bytes_wr != (uint64_t)frame_len)) { return LDG_ERR_IO_WR; }

    return LDG_ERR_AOK;
}

static uint32_t save_payload_sect_rd(const ldg_byte_t *payload, ldg_dword_t payload_len, crosstalk_game_settings_t *out_settings, uint8_t *out_settings_present, crosstalk_game_last_state_t *out_state, uint8_t *out_state_present, uint64_t *out_author_steam_id, char *out_author_name, uint32_t author_name_cap, uint64_t *out_created_epoch_ms, uint64_t *out_ld_ms_total, uint64_t *out_sim_ms_total)
{
    game_save_hdr_t hdr = LDG_STRUCT_ZERO_INIT;
    const ldg_byte_t *data = 0x0;
    uint32_t len = 0;
    uint8_t found = 0;
    uint32_t ret = 0;

    ret = save_hdr_parse(payload, payload_len, &hdr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: save hdr rejected; err: %u32; payload_len: %u32", ret, payload_len);
        return ret;
    }

    if (out_author_steam_id != 0x0) { *out_author_steam_id = hdr.author_steam_id; }

    if (out_created_epoch_ms != 0x0) { *out_created_epoch_ms = hdr.created_epoch_ms; }

    if (out_author_name != 0x0 && author_name_cap > 0)
    {
        hdr.author[sizeof(hdr.author) - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
        ret = ldg_str_rbrcpy(out_author_name, hdr.author, (uint64_t)author_name_cap);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }
    }

    found = 0;
    ret = save_sect_find(payload, payload_len, GAME_SAVE_SECT_BENCH, &data, &len);
    if (ret == LDG_ERR_AOK && len >= (uint32_t)sizeof(game_save_bench_stats_t)) { found = LDG_TRUTH_TRUE; }
    else if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_NOT_FOUND)) { return ret; }

    if (found != LDG_TRUTH_TRUE)
    {
        ret = save_sect_find(payload, payload_len, GAME_SAVE_SECT_PCB, &data, &len);
        if (ret == LDG_ERR_AOK && len >= (uint32_t)(sizeof(game_save_sect_stats_t) + sizeof(game_save_bench_stats_t)))
        {
            data = data + sizeof(game_save_sect_stats_t);
            found = LDG_TRUTH_TRUE;
        }
        else if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_NOT_FOUND)) { return ret; }
    }

    if (found == LDG_TRUTH_TRUE)
    {
        game_save_bench_stats_t bench = LDG_STRUCT_ZERO_INIT;

        memcpy(&bench, data, sizeof(bench));
        if (out_ld_ms_total != 0x0) { *out_ld_ms_total = bench.bench_ms; }

        if (out_sim_ms_total != 0x0) { *out_sim_ms_total = bench.sim_ms; }
    }

    found = 0;
    ret = save_sect_find(payload, payload_len, GAME_SAVE_SECT_SETTINGS, &data, &len);
    if (ret == LDG_ERR_AOK) { found = LDG_TRUTH_TRUE; }
    else if (LDG_UNLIKELY(ret != LDG_ERR_NOT_FOUND)) { return ret; }

    if (found == LDG_TRUTH_TRUE && out_settings != 0x0 && out_settings_present != 0x0 && len > 0) { if (crosstalk_game_settings_deserialize(out_settings, data, len) == LDG_ERR_AOK) { *out_settings_present = LDG_TRUTH_TRUE; } }

    found = 0;
    ret = save_sect_find(payload, payload_len, GAME_SAVE_SECT_LASTSTAT, &data, &len);
    if (ret == LDG_ERR_AOK) { found = LDG_TRUTH_TRUE; }
    else if (LDG_UNLIKELY(ret != LDG_ERR_NOT_FOUND)) { return ret; }

    if (found == LDG_TRUTH_TRUE && out_state != 0x0 && out_state_present != 0x0 && len > 0) { if (crosstalk_game_last_state_deserialize(out_state, data, len) == LDG_ERR_AOK) { *out_state_present = LDG_TRUTH_TRUE; } }

    return LDG_ERR_AOK;
}

static uint8_t save_sect_name_eq_is(const char *desc_name, const char *name)
{
    char key[GAME_SAVE_SECT_NAME_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t name_len = 0;

    if (LDG_UNLIKELY(desc_name == 0x0 || name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    name_len = strlen(name);
    if (name_len > (uint64_t)sizeof(key)) { return LDG_TRUTH_FALSE; }

    memcpy(key, name, name_len);
    return memcmp(desc_name, key, sizeof(key)) == 0x0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t save_editors_append(char *out, uint32_t cap, const char *name)
{
    uint64_t used = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(out == 0x0 || name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    used = strlen(out);
    if (used > 0)
    {
        if (LDG_UNLIKELY(used + LDG_STR_TERM_SIZE >= (uint64_t)cap)) { return LDG_ERR_STR_TRUNC; }

        out[used] = ' ';
        used = used + LDG_STR_TERM_SIZE;
        out[used] = LDG_STR_TERM;
    }

    for (at = 0; at < (uint32_t)GAME_SAVE_SECT_NAME_CAP && name[at] != LDG_STR_TERM; at = at + 0x1)
    {
        char low = name[at];

        if (low >= 'A' && low <= 'Z') { low = (char)(low + ('a' - 'A')); }

        if (LDG_UNLIKELY(used + LDG_STR_TERM_SIZE >= (uint64_t)cap)) { break; }

        out[used] = low;
        used = used + LDG_STR_TERM_SIZE;
    }

    out[used] = LDG_STR_TERM;
    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_save_meta_rd(const char *path, char *out_author_name, uint32_t author_name_cap, uint64_t *out_created_epoch_ms, char *out_editors, uint32_t editors_cap)
{
    game_save_hdr_t hdr = LDG_STRUCT_ZERO_INIT;
    ldg_io_info_t st = LDG_STRUCT_ZERO_INIT;
    ldg_io_file_t *file = 0x0;
    uint64_t payload_len_disk = 0;
    uint64_t bytes_rd = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(path == 0x0 || out_author_name == 0x0 || out_created_epoch_ms == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(author_name_cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out_author_name[0] = LDG_STR_TERM;
    *out_created_epoch_ms = 0;

    if (out_editors != 0x0 && editors_cap > 0) { out_editors[0] = LDG_STR_TERM; }

    ret = ldg_io_path_info_get(path, &st);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(st.size < (uint64_t)LDG_EMIEMI_OVERHEAD + (uint64_t)sizeof(hdr))) { return LDG_ERR_IO_FMT; }

    payload_len_disk = st.size - (uint64_t)LDG_EMIEMI_OVERHEAD;

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, GAME_SAVE_FILE_MODE, &file);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_io_file_seek(file, (uint64_t)LDG_EMIEMI_HDR_LEN, LDG_IO_SEEK_START);
    if (ret == LDG_ERR_AOK) { ret = ldg_io_file_rd(file, &hdr, (uint64_t)sizeof(hdr), &bytes_rd); }

    if (ret == LDG_ERR_AOK && bytes_rd != (uint64_t)sizeof(hdr)) { ret = LDG_ERR_IO_RD; }

    if (ret == LDG_ERR_AOK && (hdr.magic[0] != 'X' || hdr.magic[1] != 'T' || hdr.magic[2] != 'L' || hdr.magic[3] != 'K')) { ret = LDG_ERR_INVALID; }

    if (ret == LDG_ERR_AOK && hdr.revision != GAME_SAVE_REVISION) { ret = LDG_ERR_UNSUPPORTED; }

    if (ret == LDG_ERR_AOK && (hdr.sect_cunt > GAME_SAVE_SECT_MAX || hdr.sect_table_off < (uint32_t)sizeof(hdr) || (uint64_t)hdr.sect_table_off + (uint64_t)hdr.sect_cunt * (uint64_t)sizeof(game_save_sect_desc_t) != payload_len_disk)) { ret = LDG_ERR_IO_FMT; }

    if (ret == LDG_ERR_AOK && out_editors != 0x0)
    {
        game_save_sect_desc_t descs[GAME_SAVE_SECT_MAX] = LDG_ARR_ZERO_INIT;
        uint64_t table_bytes = (uint64_t)hdr.sect_cunt * (uint64_t)sizeof(game_save_sect_desc_t);
        uint32_t idx = 0;

        ret = ldg_io_file_seek(file, (uint64_t)LDG_EMIEMI_HDR_LEN + (uint64_t)hdr.sect_table_off, LDG_IO_SEEK_START);
        if (ret == LDG_ERR_AOK) { ret = ldg_io_file_rd(file, descs, table_bytes, &bytes_rd); }

        if (ret == LDG_ERR_AOK && bytes_rd != table_bytes) { ret = LDG_ERR_IO_RD; }

        if (ret == LDG_ERR_AOK)
        {
            for (idx = 0; idx < hdr.sect_cunt; idx = idx + 0x1)
            {
                uint8_t game_hit = LDG_TRUTH_FALSE;

                if (save_sect_name_eq_is(descs[idx].name, GAME_SAVE_SECT_PCB) == LDG_TRUTH_TRUE) { game_hit = LDG_TRUTH_TRUE; }

                if (save_sect_name_eq_is(descs[idx].name, GAME_SAVE_SECT_BENCH) == LDG_TRUTH_TRUE) { game_hit = LDG_TRUTH_TRUE; }

                if (save_sect_name_eq_is(descs[idx].name, GAME_SAVE_SECT_ECON) == LDG_TRUTH_TRUE) { game_hit = LDG_TRUTH_TRUE; }

                if (save_sect_name_eq_is(descs[idx].name, GAME_SAVE_SECT_DISTRICT) == LDG_TRUTH_TRUE) { game_hit = LDG_TRUTH_TRUE; }

                if (save_sect_name_eq_is(descs[idx].name, GAME_SAVE_SECT_SETTINGS) == LDG_TRUTH_TRUE) { game_hit = LDG_TRUTH_TRUE; }

                if (save_sect_name_eq_is(descs[idx].name, GAME_SAVE_SECT_LASTSTAT) == LDG_TRUTH_TRUE) { game_hit = LDG_TRUTH_TRUE; }

                if (save_sect_name_eq_is(descs[idx].name, GAME_SAVE_SECT_CREDIT) == LDG_TRUTH_TRUE) { game_hit = LDG_TRUTH_TRUE; }

                if (save_sect_name_eq_is(descs[idx].name, CROSSTALK_GAME_SAVE_SECT_LAYOUT) == LDG_TRUTH_TRUE) { game_hit = LDG_TRUTH_TRUE; }

                if (save_sect_name_eq_is(descs[idx].name, CROSSTALK_GAME_SAVE_SECT_SCRIPT) == LDG_TRUTH_TRUE) { game_hit = LDG_TRUTH_TRUE; }

                if (save_sect_name_eq_is(descs[idx].name, CROSSTALK_GAME_SAVE_SECT_CALC) == LDG_TRUTH_TRUE) { game_hit = LDG_TRUTH_TRUE; }

                if (save_sect_name_eq_is(descs[idx].name, CROSSTALK_GAME_SAVE_SECT_TERM) == LDG_TRUTH_TRUE) { game_hit = LDG_TRUTH_TRUE; }

                if (save_sect_name_eq_is(descs[idx].name, CROSSTALK_GAME_SAVE_SECT_CRT) == LDG_TRUTH_TRUE) { game_hit = LDG_TRUTH_TRUE; }

                if (editors_cap > 0 && game_hit != LDG_TRUTH_TRUE && descs[idx].len > 0) { if (save_editors_append(out_editors, editors_cap, descs[idx].name) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "save editors text truncated"); } }
            }
        }
    }

    if (ldg_io_file_close(file) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "save meta close failed"); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    hdr.author[sizeof(hdr.author) - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
    *out_created_epoch_ms = hdr.created_epoch_ms;

    return ldg_str_rbrcpy(out_author_name, hdr.author, (uint64_t)author_name_cap);
}

uint32_t crosstalk_game_save_editor_sect_rd(const char *path, const char *name, uint8_t *buff, uint32_t cap, uint32_t *out_len)
{
    game_save_hdr_t hdr = LDG_STRUCT_ZERO_INIT;
    game_save_sect_desc_t descs[GAME_SAVE_SECT_MAX] = LDG_ARR_ZERO_INIT;
    char key[GAME_SAVE_SECT_NAME_CAP] = LDG_ARR_ZERO_INIT;
    ldg_io_info_t st = LDG_STRUCT_ZERO_INIT;
    ldg_io_file_t *file = 0x0;
    uint64_t payload_len_disk = 0;
    uint64_t table_bytes = 0;
    uint64_t name_len = 0;
    uint64_t bytes_rd = 0;
    uint32_t blob_len = 0;
    uint32_t blob_off = 0;
    uint32_t idx = 0;
    uint8_t found = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(path == 0x0 || name == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    name_len = strlen(name);
    if (LDG_UNLIKELY(name_len > (uint64_t)sizeof(key))) { return LDG_ERR_FUNC_ARG_INVALID; }

    memcpy(key, name, name_len);

    ret = ldg_io_path_info_get(path, &st);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(st.size < (uint64_t)LDG_EMIEMI_OVERHEAD + (uint64_t)sizeof(hdr))) { return LDG_ERR_IO_FMT; }

    payload_len_disk = st.size - (uint64_t)LDG_EMIEMI_OVERHEAD;

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, GAME_SAVE_FILE_MODE, &file);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_io_file_seek(file, (uint64_t)LDG_EMIEMI_HDR_LEN, LDG_IO_SEEK_START);
    if (ret == LDG_ERR_AOK) { ret = ldg_io_file_rd(file, &hdr, (uint64_t)sizeof(hdr), &bytes_rd); }

    if (ret == LDG_ERR_AOK && bytes_rd != (uint64_t)sizeof(hdr)) { ret = LDG_ERR_IO_RD; }

    if (ret == LDG_ERR_AOK && (hdr.magic[0] != 'X' || hdr.magic[1] != 'T' || hdr.magic[2] != 'L' || hdr.magic[3] != 'K')) { ret = LDG_ERR_INVALID; }

    if (ret == LDG_ERR_AOK && hdr.revision != GAME_SAVE_REVISION) { ret = LDG_ERR_UNSUPPORTED; }

    if (ret == LDG_ERR_AOK && (hdr.sect_cunt > GAME_SAVE_SECT_MAX || hdr.sect_table_off < (uint32_t)sizeof(hdr) || (uint64_t)hdr.sect_table_off + (uint64_t)hdr.sect_cunt * (uint64_t)sizeof(game_save_sect_desc_t) != payload_len_disk)) { ret = LDG_ERR_IO_FMT; }

    if (ret == LDG_ERR_AOK)
    {
        table_bytes = (uint64_t)hdr.sect_cunt * (uint64_t)sizeof(game_save_sect_desc_t);
        ret = ldg_io_file_seek(file, (uint64_t)LDG_EMIEMI_HDR_LEN + (uint64_t)hdr.sect_table_off, LDG_IO_SEEK_START);
        if (ret == LDG_ERR_AOK) { ret = ldg_io_file_rd(file, descs, table_bytes, &bytes_rd); }

        if (ret == LDG_ERR_AOK && bytes_rd != table_bytes) { ret = LDG_ERR_IO_RD; }
    }

    if (ret == LDG_ERR_AOK)
    {
        for (idx = 0; idx < hdr.sect_cunt; idx = idx + 0x1)
        {
            if (memcmp(descs[idx].name, key, sizeof(key)) != 0x0) { continue; }

            if (descs[idx].len < (uint32_t)sizeof(game_save_sect_stats_t))
            {
                ret = LDG_ERR_IO_FMT;
                break;
            }

            blob_len = descs[idx].len - (uint32_t)sizeof(game_save_sect_stats_t);
            blob_off = descs[idx].off + (uint32_t)sizeof(game_save_sect_stats_t);
            found = LDG_TRUTH_TRUE;
            break;
        }

        if (ret == LDG_ERR_AOK && found != LDG_TRUTH_TRUE) { ret = LDG_ERR_NOT_FOUND; }
    }

    if (ret == LDG_ERR_AOK && buff != 0x0 && blob_len > 0)
    {
        if (cap < blob_len) { ret = LDG_ERR_BOUNDS; }

        if (ret == LDG_ERR_AOK) { ret = ldg_io_file_seek(file, (uint64_t)LDG_EMIEMI_HDR_LEN + (uint64_t)blob_off, LDG_IO_SEEK_START); }

        if (ret == LDG_ERR_AOK) { ret = ldg_io_file_rd(file, buff, (uint64_t)blob_len, &bytes_rd); }

        if (ret == LDG_ERR_AOK && bytes_rd != (uint64_t)blob_len) { ret = LDG_ERR_IO_RD; }
    }

    if (ldg_io_file_close(file) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "save editor sect close failed"); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_len = blob_len;
    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_save_rd(const char *path, crosstalk_game_settings_t *out_settings, uint8_t *out_settings_present, crosstalk_game_last_state_t *out_state, uint8_t *out_state_present, uint64_t *out_author_steam_id, char *out_author_name, uint32_t author_name_cap, uint64_t *out_created_epoch_ms, uint64_t *out_ld_ms_total, uint64_t *out_sim_ms_total)
{
    ldg_io_info_t st = LDG_STRUCT_ZERO_INIT;
    ldg_io_file_t *file = 0x0;
    ldg_byte_t *frame = 0x0;
    const ldg_byte_t *payload = 0x0;
    uint64_t bytes_rd = 0;
    uint32_t frame_len = 0;
    ldg_dword_t payload_len = 0;
    ldg_dword_t checksum = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (out_settings_present != 0x0) { *out_settings_present = 0; }

    if (out_state_present != 0x0) { *out_state_present = 0; }

    ret = ldg_io_path_info_get(path, &st);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(st.size == 0)) { return LDG_ERR_IO_RD; }

    frame_len = (uint32_t)st.size;
    ret = ldg_mem_alloc((uint64_t)frame_len, (void **)&frame);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, GAME_SAVE_FILE_MODE, &file);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(frame);
        return ret;
    }

    ret = ldg_io_file_rd(file, frame, (uint64_t)frame_len, &bytes_rd);
    if (ret == LDG_ERR_AOK) { ret = ldg_io_file_close(file); }
    else { ldg_io_file_close(file); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(frame);
        return ret;
    }

    if (LDG_UNLIKELY(bytes_rd != (uint64_t)frame_len))
    {
        ldg_mem_dealloc(frame);
        return LDG_ERR_IO_RD;
    }

    ret = ldg_emiemi_decode(frame, frame_len, &payload, &payload_len, &checksum);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: save frame decode failed; err: %u32; frame_len: %u32", ret, frame_len);
        ldg_mem_dealloc(frame);
        return ret;
    }

    ret = save_payload_sect_rd(payload, payload_len, out_settings, out_settings_present, out_state, out_state_present, out_author_steam_id, out_author_name, author_name_cap, out_created_epoch_ms, out_ld_ms_total, out_sim_ms_total);
    ldg_mem_dealloc(frame);
    return ret;
}

uint32_t crosstalk_game_save_settings_rd(const char *path, crosstalk_game_settings_t *out_settings, uint8_t *out_present)
{
    ldg_io_info_t st = LDG_STRUCT_ZERO_INIT;
    ldg_io_file_t *file = 0x0;
    ldg_byte_t *frame = 0x0;
    const ldg_byte_t *payload = 0x0;
    uint64_t bytes_rd = 0;
    uint32_t frame_len = 0;
    ldg_dword_t payload_len = 0;
    ldg_dword_t checksum = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(path == 0x0 || out_settings == 0x0 || out_present == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_present = 0;

    ret = ldg_io_path_info_get(path, &st);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(st.size == 0)) { return LDG_ERR_IO_RD; }

    frame_len = (uint32_t)st.size;
    ret = ldg_mem_alloc((uint64_t)frame_len, (void **)&frame);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, GAME_SAVE_FILE_MODE, &file);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(frame);
        return ret;
    }

    ret = ldg_io_file_rd(file, frame, (uint64_t)frame_len, &bytes_rd);
    if (ret == LDG_ERR_AOK) { ret = ldg_io_file_close(file); }
    else { ldg_io_file_close(file); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(frame);
        return ret;
    }

    if (LDG_UNLIKELY(bytes_rd != (uint64_t)frame_len))
    {
        ldg_mem_dealloc(frame);
        return LDG_ERR_IO_RD;
    }

    ret = ldg_emiemi_decode(frame, frame_len, &payload, &payload_len, &checksum);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(frame);
        return ret;
    }

    {
        game_save_hdr_t hdr = LDG_STRUCT_ZERO_INIT;
        const ldg_byte_t *data = 0x0;
        uint32_t len = 0;

        ret = save_hdr_parse(payload, payload_len, &hdr);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            ldg_mem_dealloc(frame);
            return ret;
        }

        ret = save_sect_find(payload, payload_len, GAME_SAVE_SECT_SETTINGS, &data, &len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_NOT_FOUND))
        {
            ldg_mem_dealloc(frame);
            return ret;
        }

        if (ret == LDG_ERR_AOK && len > 0) { if (crosstalk_game_settings_deserialize(out_settings, data, len) == LDG_ERR_AOK) { *out_present = LDG_TRUTH_TRUE; } }
    }

    ldg_mem_dealloc(frame);
    return LDG_ERR_AOK;
}
