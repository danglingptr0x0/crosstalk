#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/dir.h>
#include <dangling/io/file.h>
#include <dangling/io/path.h>
#include <dangling/proto/emiemi.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "wshop/item.h"
#include "wshop/publish.h"

#define PUBLISH_STEP 1
#define PUBLISH_WR_MODE 0644
#define PUBLISH_KIND_CORE_TEXT "core"
#define PUBLISH_KIND_SOFT_TEXT "software"

static uint8_t publish_ext_is(const char *name, const char *ext)
{
    uint64_t name_len = 0;
    uint64_t ext_len = 0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(name == 0x0 || ext == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (name[name_len] != LDG_STR_TERM) { name_len = name_len + PUBLISH_STEP; }
    while (ext[ext_len] != LDG_STR_TERM) { ext_len = ext_len + PUBLISH_STEP; }

    if (name_len <= ext_len) { return LDG_TRUTH_FALSE; }

    for (idx = 0; idx < ext_len; idx = idx + PUBLISH_STEP) { if (name[name_len - ext_len + idx] != ext[idx]) { return LDG_TRUTH_FALSE; } }

    return LDG_TRUTH_TRUE;
}

static uint32_t publish_line_wr(ldg_io_file_t *fh, const char *line, uint64_t len)
{
    uint64_t put = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(fh == 0x0 || line == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_io_file_wr(fh, line, len, &put);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (put != len) { return CROSSTALK_WSHOP_PUBLISH_ERR_WR; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_wshop_publish_fileset_scan(const char *dir, char *out_files, uint64_t out_cap, uint32_t *out_cunt)
{
    ldg_io_dir_t *dh = 0x0;
    ldg_io_entry_t ent = LDG_STRUCT_ZERO_INIT;
    const char *name = 0x0;
    uint64_t name_len = 0;
    uint64_t pos = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(dir == 0x0 || out_files == 0x0 || out_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;
    out_files[0] = LDG_STR_TERM;

    ret = ldg_io_dir_open(dir, &dh);
    if (ret != LDG_ERR_AOK) { return ret; }

    while (ldg_io_dir_rd(dh, &ent) == LDG_ERR_AOK)
    {
        if (ldg_io_entry_dir_is(&ent) == LDG_TRUTH_TRUE) { continue; }

        if (ldg_io_entry_name_get(&ent, &name) != LDG_ERR_AOK) { continue; }

        if (name == 0x0) { continue; }

        if (publish_ext_is(name, CROSSTALK_WSHOP_PUBLISH_EXT_SV) != LDG_TRUTH_TRUE) { continue; }

        if (*out_cunt >= CROSSTALK_EDA_IC_FPGA_FILESET_MAX) { break; }

        name_len = 0;
        while (name[name_len] != LDG_STR_TERM) { name_len = name_len + PUBLISH_STEP; }

        if (name_len >= CROSSTALK_EDA_IC_FPGA_FILESET_NAME_CAP) { continue; }

        if (pos + name_len + PUBLISH_STEP + LDG_STR_TERM_SIZE > out_cap) { break; }

        if (*out_cunt != 0)
        {
            out_files[pos] = CROSSTALK_EDA_IC_FPGA_FILESET_SEP;
            pos = pos + PUBLISH_STEP;
        }

        memcpy(&out_files[pos], name, name_len);
        pos = pos + name_len;
        out_files[pos] = LDG_STR_TERM;

        *out_cunt = *out_cunt + PUBLISH_STEP;
    }

    if (ldg_io_dir_close(dh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "wshop publish dir close failed"); }

    if (*out_cunt == 0) { return CROSSTALK_WSHOP_PUBLISH_ERR_EMPTY; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_wshop_publish_manifest_wr(const char *dir, const crosstalk_wshop_manifest_t *manifest)
{
    char path[CROSSTALK_WSHOP_ITEM_PATH_CAP] = LDG_ARR_ZERO_INIT;
    char line[CROSSTALK_WSHOP_PUBLISH_LINE_CAP] = LDG_ARR_ZERO_INIT;
    ldg_io_file_t *fh = 0x0;
    const char *marker = 0x0;
    const char *kind_text = 0x0;
    uint32_t idx = 0;
    uint32_t ret = 0;
    int32_t len = 0;

    if (LDG_UNLIKELY(dir == 0x0 || manifest == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (manifest->kind == CROSSTALK_WSHOP_MANIFEST_KIND_CORE)
    {
        marker = CROSSTALK_WSHOP_PUBLISH_CORE_NAME;
        kind_text = PUBLISH_KIND_CORE_TEXT;
    }
    else if (manifest->kind == CROSSTALK_WSHOP_MANIFEST_KIND_SOFTWARE)
    {
        marker = CROSSTALK_WSHOP_PUBLISH_IMG_NAME;
        kind_text = PUBLISH_KIND_SOFT_TEXT;
    }
    else { return CROSSTALK_WSHOP_MANIFEST_ERR_KIND; }

    ret = ldg_io_path_join(dir, marker, path, (uint64_t)sizeof(path));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_open(path, LDG_IO_WR_ONLY | LDG_IO_CREATE | LDG_IO_TRUNCATE, PUBLISH_WR_MODE, &fh);
    if (ret != LDG_ERR_AOK) { return ret; }

    len = snprintf(line, sizeof(line), "rev: %u\nkind: %.15s\nname: %.127s\nauthor: %.127s\nauthor_id: %" PRIu64 "\nlicense: %.63s\nentitlement: free\n", manifest->rev, kind_text, manifest->name, manifest->author, manifest->author_id, manifest->license);
    if (len > 0) { ret = publish_line_wr(fh, line, (uint64_t)len); }
    else { ret = LDG_ERR_IO_FMT; }

    if (ret == LDG_ERR_AOK && manifest->kind == CROSSTALK_WSHOP_MANIFEST_KIND_CORE)
    {
        len = snprintf(line, sizeof(line), "top: %.63s\nclk: %.63s\npart: %.63s\nisa: %.31s\nisa_rev: %u\nfingerprint: %" PRIu64 "\ntoolchain_rev: %u\n", manifest->top, manifest->clk, manifest->part, manifest->isa, manifest->isa_rev, manifest->fingerprint, manifest->toolchain_rev);
        if (len > 0) { ret = publish_line_wr(fh, line, (uint64_t)len); }
        else { ret = LDG_ERR_IO_FMT; }

        if (ret == LDG_ERR_AOK)
        {
            len = snprintf(line, sizeof(line), "files: %.4095s\n", manifest->files);
            if (len > 0) { ret = publish_line_wr(fh, line, (uint64_t)len); }
            else { ret = LDG_ERR_IO_FMT; }
        }

        for (idx = 0; idx < manifest->mem_cunt && ret == LDG_ERR_AOK; idx = idx + PUBLISH_STEP)
        {
            len = snprintf(line, sizeof(line), "mem.%u.sig: %.63s\nmem.%u.words: %" PRIu64 "\nmem.%u.base: %" PRIu64 "\n", idx, manifest->mem[idx].sig, idx, manifest->mem[idx].words, idx, manifest->mem[idx].base);
            if (len > 0) { ret = publish_line_wr(fh, line, (uint64_t)len); }
            else { ret = LDG_ERR_IO_FMT; }
        }
    }

    if (ret == LDG_ERR_AOK && manifest->kind == CROSSTALK_WSHOP_MANIFEST_KIND_SOFTWARE)
    {
        len = snprintf(line, sizeof(line), "role: %u\ntgt.isa: %.31s\ntgt.isa_rev: %u\ntgt.core: %" PRIu64 "\n", manifest->role, manifest->tgt_isa, manifest->tgt_isa_rev, manifest->tgt_core);
        if (len > 0) { ret = publish_line_wr(fh, line, (uint64_t)len); }
        else { ret = LDG_ERR_IO_FMT; }

        for (idx = 0; idx < manifest->img_cunt && ret == LDG_ERR_AOK; idx = idx + PUBLISH_STEP)
        {
            len = snprintf(line, sizeof(line), "img.%u.path: %.255s\nimg.%u.mem: %.63s\nimg.%u.word_off: %" PRIu64 "\n", idx, manifest->img[idx].path, idx, manifest->img[idx].mem, idx, manifest->img[idx].word_off);
            if (len > 0) { ret = publish_line_wr(fh, line, (uint64_t)len); }
            else { ret = LDG_ERR_IO_FMT; }
        }
    }

    if (ldg_io_file_close(fh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "wshop publish manifest close failed"); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "wshop publish: manifest; path: %.255s; kind: %u32; err: %u32", path, manifest->kind, ret);

    return ret;
}

uint32_t crosstalk_wshop_publish_stage(const char *stage_root, const char *top, const char *src, uint32_t src_len, crosstalk_wshop_manifest_t *manifest, char *out_dir, uint64_t out_cap)
{
    char name[CROSSTALK_EDA_IC_FPGA_FILESET_NAME_CAP] = LDG_ARR_ZERO_INIT;
    char path[CROSSTALK_WSHOP_ITEM_PATH_CAP] = LDG_ARR_ZERO_INIT;
    ldg_io_file_t *fh = 0x0;
    uint64_t put = 0;
    uint32_t ret = 0;
    int32_t len = 0;

    if (LDG_UNLIKELY(stage_root == 0x0 || top == 0x0 || src == 0x0 || manifest == 0x0 || out_dir == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (src_len == 0) { return CROSSTALK_WSHOP_PUBLISH_ERR_EMPTY; }

    out_dir[0] = LDG_STR_TERM;

    ret = ldg_io_dir_create(stage_root, (uint32_t)CROSSTALK_WSHOP_PUBLISH_DIR_MODE);
    if (ret != LDG_ERR_AOK && ret != LDG_ERR_EXISTS) { return CROSSTALK_WSHOP_PUBLISH_ERR_STAGE; }

    ret = ldg_io_path_join(stage_root, top, out_dir, out_cap);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_dir_create(out_dir, (uint32_t)CROSSTALK_WSHOP_PUBLISH_DIR_MODE);
    if (ret != LDG_ERR_AOK && ret != LDG_ERR_EXISTS) { return CROSSTALK_WSHOP_PUBLISH_ERR_STAGE; }

    len = snprintf(name, sizeof(name), "%.100s%.7s", top, CROSSTALK_WSHOP_PUBLISH_SRC_EXT);
    if (len < 0) { return LDG_ERR_IO_FMT; }

    ret = ldg_io_path_join(out_dir, name, path, (uint64_t)sizeof(path));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_open(path, LDG_IO_WR_ONLY | LDG_IO_CREATE | LDG_IO_TRUNCATE, PUBLISH_WR_MODE, &fh);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_wr(fh, src, (uint64_t)src_len, &put);

    if (ldg_io_file_close(fh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "wshop publish stage close failed"); }

    if (ret != LDG_ERR_AOK) { return ret; }

    if (put != (uint64_t)src_len) { return CROSSTALK_WSHOP_PUBLISH_ERR_WR; }

    len = snprintf(manifest->files, sizeof(manifest->files), "%.127s", name);
    if (len < 0) { return LDG_ERR_IO_FMT; }

    manifest->fingerprint = (uint64_t)ldg_emiemi_fnv1a_compute((const ldg_byte_t *)src, (ldg_dword_t)src_len);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "wshop publish: staged; dir: %.191s; src: %.63s; bytes: %u32; fingerprint: %u64", out_dir, name, src_len, manifest->fingerprint);

    return crosstalk_wshop_publish_manifest_wr(out_dir, manifest);
}
