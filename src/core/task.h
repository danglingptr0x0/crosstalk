#ifndef CROSSTALK_CORE_TASK_H
#define CROSSTALK_CORE_TASK_H

#include <stdint.h>

#include <dangling/core/macros.h>

#define CROSSTALK_CORE_TASK_ERR_EMPTY 948
#define CROSSTALK_CORE_TASK_ERR_RANGE 949

#define CROSSTALK_CORE_TASK_VISIT_CAP 1024
#define CROSSTALK_CORE_TASK_BACK_CURRENT 0
#define CROSSTALK_CORE_TASK_DWELL_NONE 0
#define CROSSTALK_CORE_TASK_CUNT_NONE 0

typedef struct crosstalk_core_task_visit
{
    uint64_t win_id;
    uint64_t enter_ms;
    uint64_t dwell_ms;
} crosstalk_core_task_visit_t;

typedef struct crosstalk_core_task_ctx
{
    crosstalk_core_task_visit_t visit[CROSSTALK_CORE_TASK_VISIT_CAP];
    uint64_t visit_total;
    uint32_t hd;
    uint32_t cunt;
    uint8_t open_is;
    uint8_t ok;
    uint8_t pudding[6];
} crosstalk_core_task_ctx_t;

uint32_t crosstalk_core_task_init(crosstalk_core_task_ctx_t *ctx);
uint32_t crosstalk_core_task_visit_enter(crosstalk_core_task_ctx_t *ctx, uint64_t win_id, uint64_t now_ms);
uint32_t crosstalk_core_task_visit_close(crosstalk_core_task_ctx_t *ctx, uint64_t now_ms);
uint32_t crosstalk_core_task_visit_recent_get(const crosstalk_core_task_ctx_t *ctx, uint32_t back, crosstalk_core_task_visit_t *out);
uint32_t crosstalk_core_task_visit_cunt_get(const crosstalk_core_task_ctx_t *ctx);
uint64_t crosstalk_core_task_visit_total_get(const crosstalk_core_task_ctx_t *ctx);
uint32_t crosstalk_core_task_dwell_get(const crosstalk_core_task_ctx_t *ctx, uint64_t win_id, uint64_t now_ms, uint64_t *out_ms, uint32_t *out_visits);

#endif
