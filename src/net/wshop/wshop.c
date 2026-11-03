#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/dir.h>
#include <dangling/io/path.h>
#include <dangling/logging/logging.h>
#include <dangling/str/str.h>

#include <libresteam/cb/cb.h>
#include <libresteam/utils/utils.h>
#include <libresteam/wshop/wshop.h>

#include "core/log.h"
#include "net/wshop/wshop.h"

#define WSHOP_STEP 1U
#define WSHOP_WIN_SEP '\\'
#define WSHOP_DOWNLOAD_PRIO_NORMAL 0U
#define WSHOP_TAG_CUNT 1U

static uint32_t wshop_tag_get(uint32_t type, const char **out)
{
    const char *const tag[CROSSTALK_NET_WSHOP_TYPE_CUNT] = { CROSSTALK_NET_WSHOP_TAG_BOARD, CROSSTALK_NET_WSHOP_TAG_CORE, CROSSTALK_NET_WSHOP_TAG_SOFTWARE };

    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (type >= CROSSTALK_NET_WSHOP_TYPE_CUNT) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = tag[type];

    return LDG_ERR_AOK;
}

static uint8_t wshop_suffix_is(const char *name, const char *suffix)
{
    uint64_t name_len = 0;
    uint64_t suffix_len = 0;

    if (LDG_UNLIKELY(name == 0x0 || suffix == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    name_len = (uint64_t)strlen(name);
    suffix_len = (uint64_t)strlen(suffix);

    if (name_len < suffix_len) { return LDG_TRUTH_FALSE; }

    return strcmp(name + name_len - suffix_len, suffix) == 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t wshop_type_find(const char *path, uint32_t *out_type)
{
    const char *const marker[CROSSTALK_NET_WSHOP_TYPE_CUNT] = { CROSSTALK_NET_WSHOP_MARKER_BOARD, CROSSTALK_NET_WSHOP_MARKER_CORE, CROSSTALK_NET_WSHOP_MARKER_SOFTWARE };
    ldg_io_dir_t *dir = 0x0;
    ldg_io_entry_t entry = LDG_STRUCT_ZERO_INIT;
    const char *name = 0x0;
    uint32_t type = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(path == 0x0 || out_type == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_type = CROSSTALK_NET_WSHOP_TYPE_NONE;

    ret = ldg_io_dir_open(path, &dir);
    if (ret != LDG_ERR_AOK) { return ret; }

    while (*out_type == CROSSTALK_NET_WSHOP_TYPE_NONE && ldg_io_dir_rd(dir, &entry) == LDG_ERR_AOK)
    {
        if (ldg_io_entry_dir_is(&entry) == LDG_TRUTH_TRUE) { continue; }

        if (ldg_io_entry_name_get(&entry, &name) != LDG_ERR_AOK || name == 0x0) { continue; }

        for (type = 0; type < CROSSTALK_NET_WSHOP_TYPE_CUNT; type = type + WSHOP_STEP)
        {
            if (wshop_suffix_is(name, marker[type]) != LDG_TRUTH_TRUE) { continue; }

            *out_type = type;
            break;
        }
    }

    if (ldg_io_dir_close(dir) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "wshop: item dir close failed; path: %.191s", path); }

    if (*out_type == CROSSTALK_NET_WSHOP_TYPE_NONE) { return LDG_ERR_NOT_FOUND; }

    return LDG_ERR_AOK;
}

static uint32_t wshop_u32_rd(const uint8_t *at)
{
    return (uint32_t)at[0] | ((uint32_t)at[1] << (uint32_t)LDG_BYTE_BITS) | ((uint32_t)at[2] << (uint32_t)LDG_BYTE_SHIFT_2) | ((uint32_t)at[3] << (uint32_t)LDG_BYTE_SHIFT_3);
}

static uint64_t wshop_u64_rd(const uint8_t *at)
{
    return (uint64_t)wshop_u32_rd(at) | ((uint64_t)wshop_u32_rd(at + (uint64_t)LDG_WORD_BYTES) << (uint64_t)(LDG_BYTE_BITS * LDG_WORD_BYTES));
}

static uint32_t wshop_call_res_get(const crosstalk_net_wshop_ctx_t *ctx, uint64_t call, uint32_t cb_id, uint8_t *res, uint8_t *out_done)
{
    uint8_t done = LDG_TRUTH_FALSE;
    uint8_t failed = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || res == 0x0 || out_done == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_done = LDG_TRUTH_FALSE;

    ret = libresteam_utils_call_completed_get(ctx->set, call, &done, &failed);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (done != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    *out_done = LDG_TRUTH_TRUE;

    if (failed == LDG_TRUTH_TRUE) { return LDG_ERR_IO_WR; }

    ret = libresteam_cb_call_res_get(ctx->set, call, res, CROSSTALK_NET_WSHOP_RES_CAP, cb_id, &failed);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (failed == LDG_TRUTH_TRUE) { return LDG_ERR_IO_WR; }

    return LDG_ERR_AOK;
}

static uint32_t wshop_update_submit(crosstalk_net_wshop_ctx_t *ctx, crosstalk_net_wshop_upload_t *up)
{
    const char *tag_one[WSHOP_TAG_CUNT] = { 0x0 };
    libresteam_wshop_str_arr_t tags = LDG_STRUCT_ZERO_INIT;
    uint64_t update = LIBRESTEAM_WSHOP_HANDLE_INVALID;
    uint32_t app_id = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || up == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = wshop_tag_get(up->type, &tag_one[0]);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = libresteam_utils_app_id_get(ctx->set, &app_id);
    if (ret == LDG_ERR_AOK) { ret = libresteam_wshop_item_update_start(ctx->set, app_id, up->file_id, &update); }

    if (ret == LDG_ERR_AOK) { ret = libresteam_wshop_item_title_set(ctx->set, update, up->title); }

    if (ret == LDG_ERR_AOK && up->desc[0] != LDG_STR_TERM) { ret = libresteam_wshop_item_desc_set(ctx->set, update, up->desc); }

    if (ret == LDG_ERR_AOK) { ret = libresteam_wshop_item_content_set(ctx->set, update, up->content_path); }

    if (ret == LDG_ERR_AOK && up->preview_path[0] != LDG_STR_TERM) { ret = libresteam_wshop_item_preview_set(ctx->set, update, up->preview_path); }

    tags.str = tag_one;
    tags.cunt = WSHOP_TAG_CUNT;

    if (ret == LDG_ERR_AOK) { ret = libresteam_wshop_item_tags_set(ctx->set, update, &tags, LDG_TRUTH_FALSE); }

    if (ret == LDG_ERR_AOK) { ret = libresteam_wshop_item_update_submit(ctx->set, update, up->title, &up->call); }

    return ret;
}

uint32_t crosstalk_net_wshop_init(crosstalk_net_wshop_ctx_t *ctx, const libresteam_backend_set_t *set)
{
    if (LDG_UNLIKELY(ctx == 0x0 || set == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->set = set;
    ctx->init_ok = LDG_TRUTH_TRUE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "wshop: init; types: %u32", (uint32_t)CROSSTALK_NET_WSHOP_TYPE_CUNT);

    return LDG_ERR_AOK;
}

void crosstalk_net_wshop_shutdown(crosstalk_net_wshop_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    ctx->init_ok = LDG_TRUTH_FALSE;
}

uint32_t crosstalk_net_wshop_refresh(crosstalk_net_wshop_ctx_t *ctx)
{
    uint64_t file_id[CROSSTALK_NET_WSHOP_ITEM_MAX] = LDG_ARR_ZERO_INIT;
    crosstalk_net_wshop_item_t *item = 0x0;
    const char *tag = 0x0;
    uint64_t disk_size = 0;
    uint32_t file_cunt = 0;
    uint32_t stamp = 0;
    uint32_t type = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->init_ok == LDG_TRUTH_FALSE) { return LDG_ERR_NOT_INIT; }

    ctx->item_cunt = 0;
    memset(ctx->type_cunt, 0x0, sizeof(ctx->type_cunt));

    ret = libresteam_wshop_subscribed_items_get(ctx->set, file_id, CROSSTALK_NET_WSHOP_ITEM_MAX, LDG_TRUTH_FALSE, &file_cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (file_cunt > CROSSTALK_NET_WSHOP_ITEM_MAX) { file_cunt = CROSSTALK_NET_WSHOP_ITEM_MAX; }

    for (idx = 0; idx < file_cunt; idx = idx + WSHOP_STEP)
    {
        item = &ctx->item[ctx->item_cunt];
        memset(item, 0x0, sizeof(*item));
        item->file_id = file_id[idx];
        item->type = CROSSTALK_NET_WSHOP_TYPE_NONE;

        if (libresteam_wshop_item_state_get(ctx->set, item->file_id, &item->state) != LDG_ERR_AOK) { continue; }

        if ((item->state & LIBRESTEAM_WSHOP_ITEM_STATE_INSTALLED) == 0)
        {
            if ((item->state & (LIBRESTEAM_WSHOP_ITEM_STATE_DOWNLOADING | LIBRESTEAM_WSHOP_ITEM_STATE_DOWNLOAD_PENDING)) == 0) { if (libresteam_wshop_item_download(ctx->set, item->file_id, WSHOP_DOWNLOAD_PRIO_NORMAL) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "wshop: download refused; id: %u64", item->file_id); } }

            continue;
        }

        if (libresteam_wshop_item_install_info_get(ctx->set, item->file_id, &disk_size, item->path, (uint32_t)sizeof(item->path), &stamp) != LDG_ERR_AOK) { continue; }

        if (wshop_type_find(item->path, &type) != LDG_ERR_AOK) { continue; }

        if (wshop_tag_get(type, &tag) != LDG_ERR_AOK) { continue; }

        if (ldg_str_rbrcpy(item->tag, tag, (uint64_t)sizeof(item->tag)) != LDG_ERR_AOK) { continue; }

        item->type = type;
        item->installed = LDG_TRUTH_TRUE;

        ctx->type_idx[type][ctx->type_cunt[type]] = ctx->item_cunt;
        ctx->type_cunt[type] = ctx->type_cunt[type] + WSHOP_STEP;
        ctx->item_cunt = ctx->item_cunt + WSHOP_STEP;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "wshop: refresh; subscribed: %u32; installed: %u32", file_cunt, ctx->item_cunt);

    return LDG_ERR_AOK;
}

void crosstalk_net_wshop_poll(crosstalk_net_wshop_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return; }
}

uint32_t crosstalk_net_wshop_item_cunt_get(const crosstalk_net_wshop_ctx_t *ctx, uint32_t type)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return CROSSTALK_NET_WSHOP_CUNT_NONE; }

    if (ctx->init_ok == LDG_TRUTH_FALSE) { return CROSSTALK_NET_WSHOP_CUNT_NONE; }

    if (type >= CROSSTALK_NET_WSHOP_TYPE_CUNT) { return CROSSTALK_NET_WSHOP_CUNT_NONE; }

    return ctx->type_cunt[type];
}

const crosstalk_net_wshop_item_t* crosstalk_net_wshop_item_get(const crosstalk_net_wshop_ctx_t *ctx, uint32_t type, uint32_t idx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return 0x0; }

    if (ctx->init_ok == LDG_TRUTH_FALSE) { return 0x0; }

    if (type >= CROSSTALK_NET_WSHOP_TYPE_CUNT || idx >= ctx->type_cunt[type]) { return 0x0; }

    return &ctx->item[ctx->type_idx[type][idx]];
}

uint8_t crosstalk_net_wshop_item_rdy_is(const crosstalk_net_wshop_item_t *item)
{
    if (LDG_UNLIKELY(item == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (item->installed != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    if (item->path[0] == LDG_STR_TERM) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

uint32_t crosstalk_net_wshop_item_pull(const crosstalk_net_wshop_ctx_t *ctx, const crosstalk_net_wshop_item_t *item)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || item == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->init_ok == LDG_TRUTH_FALSE) { return LDG_ERR_NOT_INIT; }

    if (crosstalk_net_wshop_item_rdy_is(item) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = libresteam_wshop_item_download(ctx->set, item->file_id, WSHOP_DOWNLOAD_PRIO_NORMAL);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "wshop: pull; id: %u64; state: %u32; err: %u32", item->file_id, item->state, ret);

    if (ret == LDG_ERR_AOK) { return LDG_ERR_AGAIN; }

    return ret;
}

uint32_t crosstalk_net_wshop_upload_start(crosstalk_net_wshop_ctx_t *ctx, crosstalk_net_wshop_upload_t *up, uint32_t type, const char *title, const char *desc, const char *content_path, const char *preview_path)
{
    uint32_t app_id = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || up == 0x0 || title == 0x0 || content_path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->init_ok == LDG_TRUTH_FALSE) { return LDG_ERR_NOT_INIT; }

    if (type >= CROSSTALK_NET_WSHOP_TYPE_CUNT) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (up->state == CROSSTALK_NET_WSHOP_UPLOAD_CREATING || up->state == CROSSTALK_NET_WSHOP_UPLOAD_UPDATING) { return LDG_ERR_BUSY; }

    memset(up, 0x0, sizeof(*up));
    up->type = type;

    ret = ldg_str_rbrcpy(up->title, title, (uint64_t)sizeof(up->title));
    if (ret == LDG_ERR_AOK && desc != 0x0) { ret = ldg_str_rbrcpy(up->desc, desc, (uint64_t)sizeof(up->desc)); }

    if (ret == LDG_ERR_AOK) { ret = ldg_str_rbrcpy(up->content_path, content_path, (uint64_t)sizeof(up->content_path)); }

    if (ret == LDG_ERR_AOK && preview_path != 0x0) { ret = ldg_str_rbrcpy(up->preview_path, preview_path, (uint64_t)sizeof(up->preview_path)); }

    if (ret == LDG_ERR_AOK) { ret = libresteam_utils_app_id_get(ctx->set, &app_id); }

    if (ret == LDG_ERR_AOK) { ret = libresteam_wshop_item_create(ctx->set, app_id, CROSSTALK_NET_WSHOP_FILE_TYPE_COMMUNITY, &up->call); }

    up->state = ret == LDG_ERR_AOK ? CROSSTALK_NET_WSHOP_UPLOAD_CREATING : CROSSTALK_NET_WSHOP_UPLOAD_ERR;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "wshop: upload start; type: %u32; title: %.63s; content: %.191s; err: %u32", type, title, content_path, ret);

    return ret;
}

uint32_t crosstalk_net_wshop_upload_state_get(crosstalk_net_wshop_ctx_t *ctx, crosstalk_net_wshop_upload_t *up, uint32_t *out_state)
{
    uint8_t res[CROSSTALK_NET_WSHOP_RES_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t file_id_off = 0;
    uint8_t done = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || up == 0x0 || out_state == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_state = up->state;

    if (up->state == CROSSTALK_NET_WSHOP_UPLOAD_CREATING)
    {
        ret = wshop_call_res_get(ctx, up->call, CROSSTALK_NET_WSHOP_CB_ITEM_CREATED, res, &done);
        if (ret == LDG_ERR_AOK && done == LDG_TRUTH_TRUE)
        {
            file_id_off = ldg_io_path_sep_get() == WSHOP_WIN_SEP ? CROSSTALK_NET_WSHOP_RES_CREATED_FILE_ID_OFF_PACK8 : CROSSTALK_NET_WSHOP_RES_CREATED_FILE_ID_OFF_PACK4;

            if (wshop_u32_rd(res + CROSSTALK_NET_WSHOP_RES_ERES_OFF) != CROSSTALK_NET_WSHOP_ERES_OK) { ret = LDG_ERR_IO_WR; }
            else
            {
                up->file_id = wshop_u64_rd(res + file_id_off);
                ret = wshop_update_submit(ctx, up);
                if (ret == LDG_ERR_AOK) { up->state = CROSSTALK_NET_WSHOP_UPLOAD_UPDATING; }
            }
        }

        if (ret != LDG_ERR_AOK)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "wshop: item create failed; err: %u32", ret);
            up->state = CROSSTALK_NET_WSHOP_UPLOAD_ERR;
        }
    }
    else if (up->state == CROSSTALK_NET_WSHOP_UPLOAD_UPDATING)
    {
        ret = wshop_call_res_get(ctx, up->call, CROSSTALK_NET_WSHOP_CB_ITEM_SUBMITTED, res, &done);
        if (ret == LDG_ERR_AOK && done == LDG_TRUTH_TRUE)
        {
            up->state = wshop_u32_rd(res + CROSSTALK_NET_WSHOP_RES_ERES_OFF) == CROSSTALK_NET_WSHOP_ERES_OK ? CROSSTALK_NET_WSHOP_UPLOAD_DONE : CROSSTALK_NET_WSHOP_UPLOAD_ERR;

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "wshop: item submitted; id: %u64; state: %u32", up->file_id, up->state);
        }

        if (ret != LDG_ERR_AOK)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "wshop: item submit failed; err: %u32", ret);
            up->state = CROSSTALK_NET_WSHOP_UPLOAD_ERR;
        }
    }

    *out_state = up->state;

    if (*out_state == CROSSTALK_NET_WSHOP_UPLOAD_ERR) { return LDG_ERR_IO_WR; }

    return LDG_ERR_AOK;
}
