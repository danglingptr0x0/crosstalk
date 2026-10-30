#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/core/types.h>
#include <dangling/io/dir.h>
#include <dangling/io/file.h>
#include <dangling/io/path.h>
#include <dangling/str/str.h>

#include "core/log.h"
#include "game/board.h"
#include "game/save.h"

uint32_t crosstalk_game_board_init(crosstalk_game_board_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->cunt = 0;
    ctx->src_cunt = 0;
    ctx->truncated = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_game_board_src_dir_reg(crosstalk_game_board_ctx_t *ctx, const char *dir, uint32_t *out_idx)
{
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || dir == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = 0;

    for (idx = 0; idx < ctx->src_cunt; idx = idx + 1) { if (strcmp(ctx->src_dir[idx], dir) == 0)
        {
            *out_idx = idx;
            return LDG_ERR_AOK;
        }
    }

    if (LDG_UNLIKELY(ctx->src_cunt >= CROSSTALK_GAME_BOARD_SRC_CAP)) { return LDG_ERR_FULL; }

    ret = ldg_str_rbrcpy(ctx->src_dir[ctx->src_cunt], dir, (uint64_t)CROSSTALK_GAME_BOARD_DIR_CAP);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_idx = ctx->src_cunt;
    ctx->src_cunt++;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_board_file_parse(const char *file, char *out_name, uint32_t out_name_cap, uint64_t *out_epoch_sec, uint8_t *out_epoch_from_name)
{
    char stem[CROSSTALK_GAME_BOARD_FILE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t len = 0;
    uint32_t dot = 0;
    uint32_t sep = 0;
    uint32_t epoch_pos = 0;
    uint32_t idx = 0;
    uint32_t digits = 0;
    dword_t epoch = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(file == 0x0 || out_name == 0x0 || out_epoch_sec == 0x0 || out_epoch_from_name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(out_name_cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out_epoch_sec = 0;
    *out_epoch_from_name = LDG_TRUTH_FALSE;
    out_name[0] = LDG_STR_TERM;

    ret = ldg_str_rbrcpy(stem, file, (uint64_t)sizeof(stem));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    len = (uint32_t)strlen(stem);
    dot = len;

    for (idx = 0; idx < len; idx = idx + 1) { if (stem[idx] == '.') { dot = idx; } }

    stem[dot] = LDG_STR_TERM;
    len = dot;
    sep = len;

    for (idx = 0; idx < len; idx = idx + 1) { if (stem[idx] == '_') { sep = idx; } }

    if (sep < len)
    {
        epoch_pos = sep;
        epoch_pos++;

        for (idx = epoch_pos; idx < len; idx = idx + 1)
        {
            if (ldg_char_digit_is((uint8_t)stem[idx]) != LDG_TRUTH_TRUE)
            {
                digits = 0;
                break;
            }

            digits++;
        }

        if (digits > 0 && ldg_str_to_dec(&stem[epoch_pos], &epoch) == LDG_ERR_AOK)
        {
            *out_epoch_sec = (uint64_t)epoch;
            *out_epoch_from_name = LDG_TRUTH_TRUE;
            stem[sep] = LDG_STR_TERM;
        }
    }

    return ldg_str_rbrcpy(out_name, stem, (uint64_t)out_name_cap);
}

uint32_t crosstalk_game_board_scan(crosstalk_game_board_ctx_t *ctx, const char *dir, crosstalk_game_board_src_t src, const char *tag)
{
    char ext[CROSSTALK_GAME_BOARD_FILE_CAP] = LDG_ARR_ZERO_INIT;
    char full[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    ldg_io_info_t st = LDG_STRUCT_ZERO_INIT;
    ldg_io_dir_t *handle = 0x0;
    ldg_io_entry_t entry = LDG_STRUCT_ZERO_INIT;
    crosstalk_game_board_t *slot = 0x0;
    const char *name = 0x0;
    uint32_t dir_idx = 0;
    uint32_t match = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || dir == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(dir[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = crosstalk_game_board_src_dir_reg(ctx, dir, &dir_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_io_dir_open(dir, &handle);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (ldg_io_dir_rd(handle, &entry) == LDG_ERR_AOK)
    {
        if (ldg_io_entry_dir_is(&entry) == LDG_TRUTH_TRUE) { continue; }

        if (ldg_io_entry_name_get(&entry, &name) != LDG_ERR_AOK || name == 0x0) { continue; }

        if (ldg_io_path_ext_get(name, ext, (uint64_t)sizeof(ext)) != LDG_ERR_AOK) { continue; }

        if (ext[0] == '.') { match = (uint32_t)strcmp(ext, CROSSTALK_GAME_BOARD_EXT); }
        else { match = (uint32_t)strcmp(ext, CROSSTALK_GAME_BOARD_EXT_BARE); }

        if (match != 0) { continue; }

        if (ctx->cunt >= CROSSTALK_GAME_BOARD_CAP)
        {
            ctx->truncated = LDG_TRUTH_TRUE;
            break;
        }

        slot = &ctx->entry[ctx->cunt];

        if (ldg_str_rbrcpy(slot->file, name, (uint64_t)sizeof(slot->file)) != LDG_ERR_AOK) { continue; }

        if (crosstalk_game_board_file_parse(slot->file, slot->name, (uint32_t)sizeof(slot->name), &slot->epoch_sec, &slot->epoch_from_name) != LDG_ERR_AOK) { continue; }

        slot->src = src;
        slot->dir_idx = (uint8_t)dir_idx;
        slot->size_bytes = 0;
        slot->created_epoch_ms = 0;
        slot->author[0] = LDG_STR_TERM;
        slot->author_ok = LDG_TRUTH_FALSE;
        slot->editor_is = LDG_TRUTH_FALSE;
        slot->tag[0] = LDG_STR_TERM;
        slot->editors[0] = LDG_STR_TERM;

        if (tag != 0x0 && ldg_str_rbrcpy(slot->tag, tag, (uint64_t)sizeof(slot->tag)) != LDG_ERR_AOK) { slot->tag[0] = LDG_STR_TERM; }

        if (ldg_io_path_join(dir, slot->file, full, (uint64_t)sizeof(full)) == LDG_ERR_AOK)
        {
            if (ldg_io_path_info_get(full, &st) == LDG_ERR_AOK)
            {
                slot->size_bytes = st.size;

                if (slot->epoch_from_name == LDG_TRUTH_FALSE) { slot->epoch_sec = st.modified_ns / LDG_NS_PER_SEC; }
            }

            if (crosstalk_game_save_meta_rd(full, slot->author, (uint32_t)sizeof(slot->author), &slot->created_epoch_ms, slot->editors, (uint32_t)sizeof(slot->editors)) == LDG_ERR_AOK) { slot->author_ok = LDG_TRUTH_TRUE; }

            if (slot->editors[0] != LDG_STR_TERM) { slot->editor_is = LDG_TRUTH_TRUE; }
        }

        ctx->cunt++;
    }

    return ldg_io_dir_close(handle);
}

uint32_t crosstalk_game_board_sort(crosstalk_game_board_ctx_t *ctx)
{
    crosstalk_game_board_t tmp = LDG_STRUCT_ZERO_INIT;
    uint32_t idx = 0;
    uint32_t pos = 0;
    uint32_t prev = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < ctx->cunt; idx = idx + 1)
    {
        tmp = ctx->entry[idx];
        pos = idx;

        while (pos > 0)
        {
            prev = pos;
            prev--;

            if (ctx->entry[prev].epoch_sec >= tmp.epoch_sec) { break; }

            ctx->entry[pos] = ctx->entry[prev];
            pos--;
        }

        ctx->entry[pos] = tmp;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_board_path_build(const crosstalk_game_board_ctx_t *ctx, uint32_t idx, char *out, uint64_t out_size)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(idx >= ctx->cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(ctx->entry[idx].dir_idx >= ctx->src_cunt)) { return LDG_ERR_BOUNDS; }

    return ldg_io_path_join(ctx->src_dir[ctx->entry[idx].dir_idx], ctx->entry[idx].file, out, out_size);
}

uint32_t crosstalk_game_board_remove(crosstalk_game_board_ctx_t *ctx, uint32_t idx)
{
    char full[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char shot_full[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char shot_file[CROSSTALK_GAME_BOARD_FILE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(idx >= ctx->cunt)) { return LDG_ERR_BOUNDS; }

    if (ctx->entry[idx].src != CROSSTALK_GAME_BOARD_SRC_LOCAL) { return LDG_ERR_DENIED; }

    ret = crosstalk_game_board_path_build(ctx, idx, full, (uint64_t)sizeof(full));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_game_board_ext_swap(ctx->entry[idx].file, CROSSTALK_GAME_BOARD_SHOT_EXT, shot_file, (uint64_t)sizeof(shot_file));
    if (ret == LDG_ERR_AOK) { ret = ldg_io_path_join(ctx->src_dir[ctx->entry[idx].dir_idx], shot_file, shot_full, (uint64_t)sizeof(shot_full)); }

    if (ret == LDG_ERR_AOK && ldg_io_path_exists_is(shot_full) == LDG_TRUTH_TRUE) { ret = ldg_io_path_destroy(shot_full); }

    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "board shot remove failed"); }

    return ldg_io_path_destroy(full);
}

uint32_t crosstalk_game_board_name_sanitize(const char *name, char *out, uint32_t out_cap)
{
    uint32_t idx = 0;
    uint32_t kept = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(name == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(out_cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;

    for (idx = 0; name[idx] != LDG_STR_TERM; idx = idx + 1)
    {
        if (kept >= out_cap - LDG_STR_TERM_SIZE) { break; }

        if ((uint8_t)name[idx] < LDG_ASCII_PRINTABLE_MIN || (uint8_t)name[idx] > LDG_ASCII_PRINTABLE_MAX) { continue; }

        if (strchr(CROSSTALK_GAME_BOARD_NAME_ILLEGAL, name[idx]) != 0x0)
        {
            out[kept] = CROSSTALK_GAME_BOARD_NAME_SUB;
            kept++;
            continue;
        }

        if (kept == 0 && name[idx] == '.') { continue; }

        out[kept] = name[idx];
        kept++;
    }

    out[kept] = LDG_STR_TERM;

    if (kept == 0) { ret = ldg_str_rbrcpy(out, CROSSTALK_GAME_BOARD_NAME_DEFAULT, (uint64_t)out_cap); }

    return ret;
}

uint32_t crosstalk_game_board_file_compose(const char *name, uint64_t epoch_sec, char *out, uint64_t out_size)
{
    char safe[CROSSTALK_GAME_BOARD_NAME_CAP] = LDG_ARR_ZERO_INIT;
    int32_t written = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(name == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(out_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;

    ret = crosstalk_game_board_name_sanitize(name, safe, (uint32_t)sizeof(safe));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    written = snprintf(out, out_size, "%.63s_%" PRIu64 "%.8s", safe, epoch_sec, CROSSTALK_GAME_BOARD_EXT);
    if (LDG_UNLIKELY(written <= 0)) { return LDG_ERR_STR_FMT; }

    if (LDG_UNLIKELY((uint64_t)written >= out_size)) { return LDG_ERR_STR_TRUNC; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_board_ext_swap(const char *file, const char *ext, char *out, uint64_t out_size)
{
    char stem[CROSSTALK_GAME_BOARD_FILE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t len = 0;
    uint32_t dot = 0;
    uint32_t idx = 0;
    int32_t written = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(file == 0x0 || ext == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(out_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;

    ret = ldg_str_rbrcpy(stem, file, (uint64_t)sizeof(stem));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    len = (uint32_t)strlen(stem);
    dot = len;

    for (idx = 0; idx < len; idx = idx + 1) { if (stem[idx] == '.') { dot = idx; } }

    stem[dot] = LDG_STR_TERM;

    written = snprintf(out, out_size, "%.127s%.8s", stem, ext);
    if (LDG_UNLIKELY(written <= 0)) { return LDG_ERR_STR_FMT; }

    if (LDG_UNLIKELY((uint64_t)written >= out_size)) { return LDG_ERR_STR_TRUNC; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_board_names_build(crosstalk_game_board_ctx_t *ctx)
{
    char safe_name[CROSSTALK_GAME_BOARD_NAME_CAP] = LDG_ARR_ZERO_INIT;
    const char *sep = 0x0;
    uint64_t used = 0;
    uint64_t room = 0;
    uint32_t idx = 0;
    uint32_t col = 0;
    int32_t written = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->names[0] = LDG_STR_TERM;

    for (idx = 0; idx < ctx->cunt; idx = idx + 1)
    {
        if (ldg_str_rbrcpy(safe_name, ctx->entry[idx].name, (uint64_t)sizeof(safe_name)) != LDG_ERR_AOK) { continue; }

        for (col = 0; safe_name[col] != LDG_STR_TERM; col = col + 1) { if (safe_name[col] == ';') { safe_name[col] = '_'; } }

        sep = (idx > 0) ? ";" : "";
        room = (uint64_t)sizeof(ctx->names) - used;

        if (room == 0) { break; }

        written = snprintf(&ctx->names[used], room, "%.1s%.63s", sep, safe_name);
        if (written <= 0) { break; }

        if ((uint64_t)written >= room)
        {
            ctx->names[used] = LDG_STR_TERM;
            break;
        }

        used = used + (uint64_t)written;
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_game_board_time_fmt(uint64_t epoch_sec, char *out, uint64_t out_size)
{
    struct tm broken = LDG_STRUCT_ZERO_INIT;
    const struct tm *local = 0x0;
    time_t stamp = 0;
    uint64_t written = 0;

    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(out_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;
    stamp = (time_t)epoch_sec;
    local = localtime(&stamp);

    if (LDG_UNLIKELY(local == 0x0)) { return LDG_ERR_INVALID; }

    broken = *local;
    written = (uint64_t)strftime(out, out_size, CROSSTALK_GAME_BOARD_TIME_FMT, &broken);

    if (LDG_UNLIKELY(written == 0)) { return LDG_ERR_STR_TRUNC; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_board_size_fmt(uint64_t bytes, char *out, uint64_t out_size)
{
    int32_t written = 0;

    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(out_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (bytes >= LDG_GIB) { written = snprintf(out, out_size, "%" PRIu64 "G", bytes / LDG_GIB); }
    else if (bytes >= LDG_MIB) { written = snprintf(out, out_size, "%" PRIu64 "M", bytes / LDG_MIB); }
    else { written = snprintf(out, out_size, "%" PRIu64 "K", bytes / LDG_KIB); }

    if (LDG_UNLIKELY(written <= 0)) { return LDG_ERR_STR_FMT; }

    if (LDG_UNLIKELY((uint64_t)written >= out_size)) { return LDG_ERR_STR_TRUNC; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_board_items_build(crosstalk_game_board_ctx_t *ctx)
{
    char row[CROSSTALK_GAME_BOARD_ROW_CAP] = LDG_ARR_ZERO_INIT;
    char safe_name[CROSSTALK_GAME_BOARD_NAME_CAP] = LDG_ARR_ZERO_INIT;
    char safe_author[CROSSTALK_GAME_AUTHOR_NAME_CAP] = LDG_ARR_ZERO_INIT;
    char time_text[CROSSTALK_GAME_BOARD_TIME_CAP] = LDG_ARR_ZERO_INIT;
    char mark_text[CROSSTALK_GAME_BOARD_MARK_CAP] = LDG_ARR_ZERO_INIT;
    char safe_tag[CROSSTALK_GAME_BOARD_TAG_CAP] = LDG_ARR_ZERO_INIT;
    const char *mark = 0x0;
    const char *lock_mark = 0x0;
    const char *sep = 0x0;
    const char *author = 0x0;
    const char *contents = 0x0;
    uint64_t used = 0;
    uint32_t idx = 0;
    uint32_t col = 0;
    int32_t written = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->items[0] = LDG_STR_TERM;
    memset(ctx->lock, 0x0, sizeof(ctx->lock));

    for (idx = 0; idx < ctx->cunt; idx = idx + 1)
    {
        if (ldg_str_rbrcpy(safe_name, ctx->entry[idx].name, (uint64_t)sizeof(safe_name)) != LDG_ERR_AOK) { continue; }

        for (col = 0; safe_name[col] != LDG_STR_TERM; col = col + 1) { if (safe_name[col] == ';') { safe_name[col] = '_'; } }

        author = "-";

        if (ctx->entry[idx].author_ok == LDG_TRUTH_TRUE && ldg_str_rbrcpy(safe_author, ctx->entry[idx].author, (uint64_t)sizeof(safe_author)) == LDG_ERR_AOK)
        {
            for (col = 0; safe_author[col] != LDG_STR_TERM; col = col + 1) { if (safe_author[col] == ';') { safe_author[col] = '_'; } }

            author = safe_author;
        }

        mark_text[0] = LDG_STR_TERM;

        if (ctx->entry[idx].src == CROSSTALK_GAME_BOARD_SRC_WSHOP)
        {
            if (ctx->entry[idx].tag[0] != LDG_STR_TERM && ldg_str_rbrcpy(safe_tag, ctx->entry[idx].tag, (uint64_t)sizeof(safe_tag)) == LDG_ERR_AOK)
            {
                for (col = 0; safe_tag[col] != LDG_STR_TERM; col = col + 1) { if (safe_tag[col] == ';') { safe_tag[col] = '_'; } }

                written = snprintf(mark_text, sizeof(mark_text), "[%.31s] ", safe_tag);
                if (written <= 0) { mark_text[0] = LDG_STR_TERM; }
            }

            if (mark_text[0] == LDG_STR_TERM) { if (ldg_str_rbrcpy(mark_text, CROSSTALK_GAME_BOARD_WSHOP_MARK, (uint64_t)sizeof(mark_text)) != LDG_ERR_AOK) { mark_text[0] = LDG_STR_TERM; } }
        }

        mark = mark_text;
        sep = (idx > 0) ? ";" : "";
        ctx->lock[idx] = (ctx->entry[idx].editor_is != LDG_TRUTH_TRUE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
        lock_mark = ctx->lock[idx] == LDG_TRUTH_TRUE ? CROSSTALK_GAME_BOARD_LOCK_MARK : "";
        contents = ctx->entry[idx].editors[0] != LDG_STR_TERM ? ctx->entry[idx].editors : "-";

        if (crosstalk_game_board_time_fmt(ctx->entry[idx].epoch_sec, time_text, (uint64_t)sizeof(time_text)) != LDG_ERR_AOK) { time_text[0] = LDG_STR_TERM; }

        written = snprintf(row, sizeof(row), CROSSTALK_GAME_BOARD_ROW_FMT, sep, lock_mark, mark, safe_name, time_text, author, contents);
        if (written <= 0) { continue; }

        if (used + (uint64_t)written >= (uint64_t)sizeof(ctx->items))
        {
            ctx->truncated = LDG_TRUTH_TRUE;
            break;
        }

        if (ldg_str_rbrcpy(&ctx->items[used], row, (uint64_t)sizeof(ctx->items) - used) != LDG_ERR_AOK) { break; }

        used = used + (uint64_t)written;
    }

    return LDG_ERR_AOK;
}
