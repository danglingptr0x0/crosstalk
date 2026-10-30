#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/file.h>
#include <dangling/mem/alloc.h>
#include <dangling/str/str.h>

#include "core/watch.h"

#define WATCH_DIO_ALIGN (4ULL * LDG_KIB)
#define WATCH_FILE_MODE 0644
#define WATCH_PERIOD_MS 1000
#define WATCH_STEP 1

static uint64_t watch_align_get(uint64_t val)
{
    return (val + WATCH_DIO_ALIGN - WATCH_STEP) & ~(WATCH_DIO_ALIGN - WATCH_STEP);
}

static uint32_t watch_copy(crosstalk_core_watch_t *watch, uint64_t size)
{
    ldg_io_file_t *file = 0x0;
    uint64_t span = watch_align_get(size);
    uint64_t got = 0;
    uint64_t wrote = 0;
    uint32_t ret = 0;

    if (span > watch->cap) { return LDG_ERR_FULL; }

    ret = ldg_io_file_open(watch->src, LDG_IO_RD_ONLY | LDG_IO_DIRECT, 0x0, &file);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_rd(file, watch->buff, span, &got);

    if (ldg_io_file_close(file) != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    if (ret != LDG_ERR_AOK) { return ret; }

    if (got != size) { return LDG_ERR_IO_RD; }

    memset(watch->buff + size, 0x0, span - size);

    ret = ldg_io_file_open(watch->dst, LDG_IO_WR_ONLY | LDG_IO_CREATE | LDG_IO_TRUNCATE | LDG_IO_DIRECT, WATCH_FILE_MODE, &file);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_wr(file, watch->buff, span, &wrote);
    if (ret == LDG_ERR_AOK && wrote != span) { ret = LDG_ERR_IO_WR; }

    if (ret == LDG_ERR_AOK) { ret = ldg_io_file_truncate(file, size); }

    if (ldg_io_file_close(file) != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    return ret;
}

static LDG_NOINLINE uint32_t watch_set_sample(const char *dir, const char *files, uint64_t *size, uint64_t *modified_ns, uint32_t *out_cunt)
{
    char name[CROSSTALK_CORE_WATCH_SET_NAME_CAP];
    char path[LDG_IO_PATH_MAX];
    ldg_io_info_t info = LDG_STRUCT_ZERO_INIT;
    uint32_t files_len = (uint32_t)strlen(files);
    uint32_t pos = 0;
    uint32_t name_len = 0;
    uint32_t cunt = 0;
    uint32_t ret = 0;

    *out_cunt = 0x0;

    for (pos = 0; pos <= files_len; pos = pos + 0x1)
    {
        if (files[pos] != CROSSTALK_CORE_WATCH_SET_SEP && files[pos] != LDG_STR_TERM)
        {
            if (name_len >= CROSSTALK_CORE_WATCH_SET_NAME_CAP - LDG_STR_TERM_SIZE) { return LDG_ERR_FUNC_ARG_INVALID; }

            name[name_len] = files[pos];
            name_len = name_len + 0x1;
            continue;
        }

        if (name_len == 0) { continue; }

        if (cunt >= CROSSTALK_CORE_WATCH_SET_MAX) { return LDG_ERR_FULL; }

        name[name_len] = LDG_STR_TERM;
        name_len = 0x0;

        ret = ldg_io_path_join(dir, name, path, (uint64_t)sizeof(path));
        if (ret != LDG_ERR_AOK) { return ret; }

        ret = ldg_io_path_info_get(path, &info);
        if (ret != LDG_ERR_AOK) { return ret; }

        size[cunt] = info.size;
        modified_ns[cunt] = info.modified_ns;
        cunt = cunt + 0x1;
    }

    if (cunt == 0) { return LDG_ERR_EMPTY; }

    *out_cunt = cunt;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_watch_create(crosstalk_core_watch_t *watch, uint64_t cap)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(watch == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(watch, 0x0, sizeof(crosstalk_core_watch_t));

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    watch->cap = watch_align_get(cap);

    ret = ldg_mem_alloc(watch->cap + WATCH_DIO_ALIGN, (void **)&watch->buff_raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        watch->buff_raw = 0x0;
        return ret;
    }

    watch->buff = (uint8_t *)(uintptr_t)watch_align_get((uint64_t)(uintptr_t)watch->buff_raw);
    watch->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_watch_destroy(crosstalk_core_watch_t *watch)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(watch == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (watch->buff_raw != 0x0 && ldg_mem_dealloc(watch->buff_raw) != LDG_ERR_AOK) { ret = LDG_ERR_INVALID; }

    memset(watch, 0x0, sizeof(crosstalk_core_watch_t));

    return ret;
}

uint32_t crosstalk_core_watch_arm(crosstalk_core_watch_t *watch, const char *src, const char *dst)
{
    ldg_io_info_t info = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(watch == 0x0 || src == 0x0 || dst == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (watch->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    watch->on = LDG_TRUTH_FALSE;
    watch->pend = LDG_TRUTH_FALSE;

    ret = ldg_str_rbrcpy(watch->src, src, (uint64_t)sizeof(watch->src));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_str_rbrcpy(watch->dst, dst, (uint64_t)sizeof(watch->dst));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_path_info_get(watch->src, &info);
    if (ret != LDG_ERR_AOK) { return ret; }

    watch->size = info.size;
    watch->modified_ns = info.modified_ns;
    watch->next_ms = 0;
    watch->on = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_watch_tick(crosstalk_core_watch_t *watch, uint64_t now_ms, uint8_t *out_copied)
{
    ldg_io_info_t info = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(watch == 0x0 || out_copied == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_copied = LDG_TRUTH_FALSE;

    if (watch->on != LDG_TRUTH_TRUE || now_ms < watch->next_ms) { return LDG_ERR_AOK; }

    watch->next_ms = now_ms + (uint64_t)WATCH_PERIOD_MS;

    ret = ldg_io_path_info_get(watch->src, &info);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (info.size == watch->size && info.modified_ns == watch->modified_ns)
    {
        watch->pend = LDG_TRUTH_FALSE;
        return LDG_ERR_AOK;
    }

    if (watch->pend != LDG_TRUTH_TRUE || info.size != watch->pend_size || info.modified_ns != watch->pend_modified_ns)
    {
        watch->pend = LDG_TRUTH_TRUE;
        watch->pend_size = info.size;
        watch->pend_modified_ns = info.modified_ns;

        return LDG_ERR_AOK;
    }

    ret = watch_copy(watch, info.size);
    if (ret != LDG_ERR_AOK) { return ret; }

    watch->size = info.size;
    watch->modified_ns = info.modified_ns;
    watch->pend = LDG_TRUTH_FALSE;
    *out_copied = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_watch_set_arm(crosstalk_core_watch_set_t *set, const char *dir, const char *files)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(set == 0x0 || dir == 0x0 || files == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(set, 0x0, sizeof(crosstalk_core_watch_set_t));

    ret = watch_set_sample(dir, files, set->size, set->modified_ns, &set->cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    set->on = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_watch_set_disarm(crosstalk_core_watch_set_t *set)
{
    if (LDG_UNLIKELY(set == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(set, 0x0, sizeof(crosstalk_core_watch_set_t));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_watch_set_tick(crosstalk_core_watch_set_t *set, const char *dir, const char *files, uint64_t now_ms, uint8_t *out_changed)
{
    uint64_t size[CROSSTALK_CORE_WATCH_SET_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t modified_ns[CROSSTALK_CORE_WATCH_SET_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t span = 0;
    uint32_t cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(set == 0x0 || dir == 0x0 || files == 0x0 || out_changed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_changed = LDG_TRUTH_FALSE;

    if (set->on != LDG_TRUTH_TRUE || now_ms < set->next_ms) { return LDG_ERR_AOK; }

    set->next_ms = now_ms + (uint64_t)WATCH_PERIOD_MS;

    ret = watch_set_sample(dir, files, size, modified_ns, &cunt);
    if (ret != LDG_ERR_AOK)
    {
        set->pend = LDG_TRUTH_FALSE;
        return LDG_ERR_AOK;
    }

    if (cunt != set->cunt)
    {
        set->on = LDG_TRUTH_FALSE;
        return LDG_ERR_INVALID;
    }

    span = (uint64_t)cunt * (uint64_t)sizeof(uint64_t);

    if (memcmp(size, set->size, span) == 0 && memcmp(modified_ns, set->modified_ns, span) == 0)
    {
        set->pend = LDG_TRUTH_FALSE;
        return LDG_ERR_AOK;
    }

    if (set->pend != LDG_TRUTH_TRUE || memcmp(size, set->pend_size, span) != 0 || memcmp(modified_ns, set->pend_modified_ns, span) != 0)
    {
        set->pend = LDG_TRUTH_TRUE;
        memcpy(set->pend_size, size, span);
        memcpy(set->pend_modified_ns, modified_ns, span);

        return LDG_ERR_AOK;
    }

    memcpy(set->size, size, span);
    memcpy(set->modified_ns, modified_ns, span);
    set->pend = LDG_TRUTH_FALSE;
    *out_changed = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}
