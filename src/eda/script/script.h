#ifndef CROSSTALK_EDA_SCRIPT_H
#define CROSSTALK_EDA_SCRIPT_H

#include <stdint.h>

#include <dangling/mem/alloc.h>

#include "eda/script/ast.h"
#include "eda/script/eval.h"
#include "eda/script/lex.h"
#include "eda/script/parse.h"

#define CROSSTALK_EDA_SCRIPT_SRC_CAP 65536
#define CROSSTALK_EDA_SCRIPT_PATH_CAP 512
#define CROSSTALK_EDA_SCRIPT_EXT ".xts"

#define CROSSTALK_EDA_SCRIPT_EVENT_LD 0
#define CROSSTALK_EDA_SCRIPT_EVENT_ST 1
#define CROSSTALK_EDA_SCRIPT_EVENT_EXIT 2
#define CROSSTALK_EDA_SCRIPT_EVENT_FAULT 3
#define CROSSTALK_EDA_SCRIPT_EVENT_CUNT 4
#define CROSSTALK_EDA_SCRIPT_EVENT_NAME_LD "load"
#define CROSSTALK_EDA_SCRIPT_EVENT_NAME_ST "save"
#define CROSSTALK_EDA_SCRIPT_EVENT_NAME_EXIT "exit"
#define CROSSTALK_EDA_SCRIPT_EVENT_NAME_FAULT "fault"

typedef struct crosstalk_eda_script_ctx
{
    crosstalk_eda_script_eval_t eval;
    crosstalk_eda_script_ast_t ast;
    crosstalk_eda_script_diag_t diag;
    char src[CROSSTALK_EDA_SCRIPT_SRC_CAP];
    char path[CROSSTALK_EDA_SCRIPT_PATH_CAP];
    char run_path[CROSSTALK_EDA_SCRIPT_PATH_CAP];
    char event_path[CROSSTALK_EDA_SCRIPT_EVENT_CUNT][CROSSTALK_EDA_SCRIPT_PATH_CAP];
    uint32_t src_len;
    uint8_t ok;
    uint8_t pudding[3];
} crosstalk_eda_script_ctx_t;

uint32_t crosstalk_eda_script_init(crosstalk_eda_script_ctx_t *ctx, ldg_mem_pool_t *pool, void *user);
uint32_t crosstalk_eda_script_shutdown(crosstalk_eda_script_ctx_t *ctx);
uint32_t crosstalk_eda_script_builtin_reg(crosstalk_eda_script_ctx_t *ctx, const char *name, crosstalk_eda_script_builtin_cb_t cb, uint32_t min_args, uint32_t max_args, uint32_t ret_width);
uint32_t crosstalk_eda_script_exec(crosstalk_eda_script_ctx_t *ctx, const char *path, const char *src, uint32_t src_len);
uint32_t crosstalk_eda_script_file_exec(crosstalk_eda_script_ctx_t *ctx, const char *path);
uint32_t crosstalk_eda_script_sesh_clear(crosstalk_eda_script_ctx_t *ctx);
uint32_t crosstalk_eda_script_state_serialize(const crosstalk_eda_script_ctx_t *ctx, uint8_t *out, uint32_t cap, uint32_t *out_len);
uint32_t crosstalk_eda_script_state_deserialize(crosstalk_eda_script_ctx_t *ctx, const uint8_t *buff, uint32_t len);
uint32_t crosstalk_eda_script_switch_set(crosstalk_eda_script_ctx_t *ctx, const char *name, uint32_t name_len, uint64_t val);
uint32_t crosstalk_eda_script_switch_release(crosstalk_eda_script_ctx_t *ctx, const char *name, uint32_t name_len);
uint32_t crosstalk_eda_script_event_set(crosstalk_eda_script_ctx_t *ctx, uint32_t event, const char *path);
uint32_t crosstalk_eda_script_event_get(const crosstalk_eda_script_ctx_t *ctx, uint32_t event, const char **out);
uint32_t crosstalk_eda_script_event_id_get(const char *name, uint32_t *out);
uint32_t crosstalk_eda_script_event_name_get(uint32_t event, const char **out);
uint32_t crosstalk_eda_script_diag_get(const crosstalk_eda_script_ctx_t *ctx, const crosstalk_eda_script_diag_t **out);
uint8_t crosstalk_eda_script_path_is(const char *text, uint32_t len);

#endif
