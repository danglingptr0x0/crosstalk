#ifndef CROSSTALK_CORE_WATCH_H
#define CROSSTALK_CORE_WATCH_H

#include <stdint.h>

#include <dangling/io/path.h>

typedef struct crosstalk_core_watch
{
    char src[LDG_IO_PATH_MAX];
    char dst[LDG_IO_PATH_MAX];
    uint8_t *buff_raw;
    uint8_t *buff;
    uint64_t cap;
    uint64_t size;
    uint64_t modified_ns;
    uint64_t pend_size;
    uint64_t pend_modified_ns;
    uint64_t next_ms;
    uint8_t on;
    uint8_t pend;
    uint8_t ok;
    uint8_t pudding[5];
} crosstalk_core_watch_t;

#define CROSSTALK_CORE_WATCH_SET_MAX 128
#define CROSSTALK_CORE_WATCH_SET_NAME_CAP 128
#define CROSSTALK_CORE_WATCH_SET_SEP ','

typedef struct crosstalk_core_watch_set
{
    uint64_t size[CROSSTALK_CORE_WATCH_SET_MAX];
    uint64_t modified_ns[CROSSTALK_CORE_WATCH_SET_MAX];
    uint64_t pend_size[CROSSTALK_CORE_WATCH_SET_MAX];
    uint64_t pend_modified_ns[CROSSTALK_CORE_WATCH_SET_MAX];
    uint64_t next_ms;
    uint32_t cunt;
    uint8_t on;
    uint8_t pend;
    uint8_t pudding[2];
} crosstalk_core_watch_set_t;

uint32_t crosstalk_core_watch_create(crosstalk_core_watch_t *watch, uint64_t cap);
uint32_t crosstalk_core_watch_destroy(crosstalk_core_watch_t *watch);
uint32_t crosstalk_core_watch_arm(crosstalk_core_watch_t *watch, const char *src, const char *dst);
uint32_t crosstalk_core_watch_tick(crosstalk_core_watch_t *watch, uint64_t now_ms, uint8_t *out_copied);
uint32_t crosstalk_core_watch_set_arm(crosstalk_core_watch_set_t *set, const char *dir, const char *files);
uint32_t crosstalk_core_watch_set_disarm(crosstalk_core_watch_set_t *set);
uint32_t crosstalk_core_watch_set_tick(crosstalk_core_watch_set_t *set, const char *dir, const char *files, uint64_t now_ms, uint8_t *out_changed);

#endif
