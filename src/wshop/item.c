#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/dir.h>
#include <dangling/io/file.h>
#include <dangling/io/path.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "wshop/item.h"

#define ITEM_STEP 1
#define ITEM_OPEN_MODE 0

static uint8_t item_ext_is(const char *name, const char *ext)
{
    uint64_t name_len = 0;
    uint64_t ext_len = 0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(name == 0x0 || ext == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (name[name_len] != LDG_STR_TERM) { name_len = name_len + ITEM_STEP; }
    while (ext[ext_len] != LDG_STR_TERM) { ext_len = ext_len + ITEM_STEP; }

    if (name_len <= ext_len) { return LDG_TRUTH_FALSE; }

    for (idx = 0; idx < ext_len; idx = idx + ITEM_STEP) { if (name[name_len - ext_len + idx] != ext[idx]) { return LDG_TRUTH_FALSE; } }

    return LDG_TRUTH_TRUE;
}

static uint32_t item_marker_find(const char *dir, char *out_name, uint64_t out_cap)
{
    ldg_io_dir_t *dh = 0x0;
    ldg_io_entry_t ent = LDG_STRUCT_ZERO_INIT;
    const char *name = 0x0;
    uint64_t name_len = 0;
    uint32_t ret = 0;
    uint32_t hit = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(dir == 0x0 || out_name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    out_name[0] = LDG_STR_TERM;

    ret = ldg_io_dir_open(dir, &dh);
    if (ret != LDG_ERR_AOK) { return ret; }

    while (ldg_io_dir_rd(dh, &ent) == LDG_ERR_AOK)
    {
        if (ldg_io_entry_dir_is(&ent) == LDG_TRUTH_TRUE) { continue; }

        if (ldg_io_entry_name_get(&ent, &name) != LDG_ERR_AOK) { continue; }

        if (name == 0x0) { continue; }

        if (item_ext_is(name, CROSSTALK_WSHOP_MANIFEST_EXT_CORE) != LDG_TRUTH_TRUE && item_ext_is(name, CROSSTALK_WSHOP_MANIFEST_EXT_SOFTWARE) != LDG_TRUTH_TRUE) { continue; }

        name_len = 0;
        while (name[name_len] != LDG_STR_TERM) { name_len = name_len + ITEM_STEP; }

        if (name_len >= out_cap) { continue; }

        memcpy(out_name, name, name_len);
        out_name[name_len] = LDG_STR_TERM;
        hit = LDG_TRUTH_TRUE;
        break;
    }

    if (ldg_io_dir_close(dh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "wshop item dir close failed"); }

    if (hit != LDG_TRUTH_TRUE) { return CROSSTALK_WSHOP_ITEM_ERR_MARKER; }

    return LDG_ERR_AOK;
}

static uint32_t item_src_rd(const char *path, char *src_buff, uint64_t src_cap, uint64_t *out_len)
{
    ldg_io_file_t *fh = 0x0;
    ldg_io_info_t st = LDG_STRUCT_ZERO_INIT;
    uint64_t got = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(path == 0x0 || src_buff == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    ret = ldg_io_path_info_get(path, &st);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (st.size == 0) { return CROSSTALK_WSHOP_ITEM_ERR_SIZE; }

    if (st.size > src_cap) { return CROSSTALK_WSHOP_ITEM_ERR_SIZE; }

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, ITEM_OPEN_MODE, &fh);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_rd(fh, src_buff, st.size, &got);

    if (ldg_io_file_close(fh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "wshop item src close failed"); }

    if (ret != LDG_ERR_AOK) { return ret; }

    if (got != st.size) { return CROSSTALK_WSHOP_ITEM_ERR_SIZE; }

    *out_len = got;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_wshop_item_ld(const char *dir, char *src_buff, uint64_t src_cap, crosstalk_wshop_item_t *out)
{
    char name[CROSSTALK_WSHOP_ITEM_NAME_CAP] = LDG_ARR_ZERO_INIT;
    char path[CROSSTALK_WSHOP_ITEM_PATH_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t dir_len = 0;
    uint64_t src_len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(dir == 0x0 || src_buff == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(out, 0x0, sizeof(*out));

    ret = item_marker_find(dir, name, (uint64_t)sizeof(name));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_path_join(dir, name, path, (uint64_t)sizeof(path));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = item_src_rd(path, src_buff, src_cap, &src_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_wshop_manifest_parse(src_buff, src_len, &out->manifest);
    if (ret != LDG_ERR_AOK) { return ret; }

    while (dir[dir_len] != LDG_STR_TERM) { dir_len = dir_len + ITEM_STEP; }

    if (dir_len >= (uint64_t)sizeof(out->dir)) { return LDG_ERR_BOUNDS; }

    memcpy(out->dir, dir, dir_len);
    out->dir[dir_len] = LDG_STR_TERM;

    if (out->manifest.kind == CROSSTALK_WSHOP_MANIFEST_KIND_CORE) { out->type = CROSSTALK_NET_WSHOP_TYPE_CORE; }
    else { out->type = CROSSTALK_NET_WSHOP_TYPE_SOFTWARE; }

    out->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_wshop_list_build(crosstalk_wshop_list_t *out, const crosstalk_net_wshop_ctx_t *net)
{
    const uint32_t type_arr[CROSSTALK_WSHOP_ITEM_SCAN_CUNT] = { CROSSTALK_NET_WSHOP_TYPE_CORE, CROSSTALK_NET_WSHOP_TYPE_SOFTWARE };
    const ldg_net_steamworks_wshop_item_t *ent = 0x0;
    crosstalk_wshop_item_t *slot = 0x0;
    uint32_t type_idx = 0;
    uint32_t ent_cunt = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(out == 0x0 || net == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(out, 0x0, sizeof(*out));

    for (type_idx = 0; type_idx < CROSSTALK_WSHOP_ITEM_SCAN_CUNT; type_idx = type_idx + ITEM_STEP)
    {
        ent_cunt = crosstalk_net_wshop_item_cunt_get(net, type_arr[type_idx]);

        for (idx = 0; idx < ent_cunt; idx = idx + ITEM_STEP)
        {
            ent = crosstalk_net_wshop_item_get(net, type_arr[type_idx], idx);
            if (ent == 0x0) { continue; }

            if (crosstalk_net_wshop_item_rdy_is(ent) != LDG_TRUTH_TRUE) { continue; }

            if (out->cunt >= CROSSTALK_WSHOP_ITEM_MAX) { return CROSSTALK_WSHOP_ITEM_ERR_FULL; }

            slot = &out->item[out->cunt];

            ret = crosstalk_wshop_item_ld(ent->path, out->src, (uint64_t)sizeof(out->src), slot);
            if (ret != LDG_ERR_AOK)
            {
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "wshop: item load failed; id: %u64; err: %u32; path: %.127s", ent->file_id, ret, ent->path);
                continue;
            }

            slot->file_id = ent->file_id;
            out->cunt = out->cunt + ITEM_STEP;

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "wshop: item; id: %u64; kind: %u32; isa: %.31s; name: %.63s", slot->file_id, slot->manifest.kind, slot->manifest.kind == CROSSTALK_WSHOP_MANIFEST_KIND_CORE ? slot->manifest.isa : slot->manifest.tgt_isa, slot->manifest.name);
        }
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "wshop: list built; items: %u32", out->cunt);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_wshop_list_cunt_get(const crosstalk_wshop_list_t *list, uint32_t kind)
{
    uint32_t cunt = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(list == 0x0)) { return CROSSTALK_NET_WSHOP_CUNT_NONE; }

    for (idx = 0; idx < list->cunt; idx = idx + ITEM_STEP)
    {
        if (list->item[idx].manifest.kind != kind) { continue; }

        cunt = cunt + ITEM_STEP;
    }

    return cunt;
}

const crosstalk_wshop_item_t* crosstalk_wshop_list_get(const crosstalk_wshop_list_t *list, uint32_t kind, uint32_t idx)
{
    uint32_t seen = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(list == 0x0)) { return 0x0; }

    for (at = 0; at < list->cunt; at = at + ITEM_STEP)
    {
        if (list->item[at].manifest.kind != kind) { continue; }

        if (seen == idx) { return &list->item[at]; }

        seen = seen + ITEM_STEP;
    }

    return 0x0;
}
