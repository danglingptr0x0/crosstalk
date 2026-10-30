#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/core/types.h>
#include <dangling/io/file.h>
#include <dangling/str/str.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "eda/script/script.h"

#define SCRIPT_STEP 1
#define SCRIPT_PATH_FMT "%.511s"
#define SCRIPT_PROMPT_NAME "console"
#define SCRIPT_SWITCH_NAME "switch"
#define SCRIPT_EXT_LEN 4

uint32_t crosstalk_eda_script_init(crosstalk_eda_script_ctx_t *ctx, ldg_mem_pool_t *pool, void *user)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || pool == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ret = crosstalk_eda_script_ast_init(&ctx->ast, pool);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_eda_script_eval_init(&ctx->eval, user);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_script_shutdown(crosstalk_eda_script_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (crosstalk_eda_script_eval_shutdown(&ctx->eval) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "script eval shutdown failed"); }

    ctx->ok = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_script_builtin_reg(crosstalk_eda_script_ctx_t *ctx, const char *name, crosstalk_eda_script_builtin_cb_t cb, uint32_t min_args, uint32_t max_args, uint32_t ret_width)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_eda_script_eval_builtin_reg(&ctx->eval, name, cb, min_args, max_args, ret_width);
}

uint32_t crosstalk_eda_script_sesh_clear(crosstalk_eda_script_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_eda_script_eval_racks_clear(&ctx->eval);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_eda_script_eval_slots_clear(&ctx->eval);
}

uint32_t crosstalk_eda_script_switch_set(crosstalk_eda_script_ctx_t *ctx, const char *name, uint32_t name_len, uint64_t val)
{
    const char *action = 0x0;
    uint32_t action_len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ret = crosstalk_eda_script_eval_switch_pos_set(&ctx->eval, name, name_len, val, &action, &action_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (action == 0x0) { return LDG_ERR_AOK; }

    ctx->eval.action_is = LDG_TRUTH_TRUE;

    ret = crosstalk_eda_script_exec(ctx, SCRIPT_SWITCH_NAME, action, action_len);

    ctx->eval.action_is = LDG_TRUTH_FALSE;

    return ret;
}

uint32_t crosstalk_eda_script_switch_release(crosstalk_eda_script_ctx_t *ctx, const char *name, uint32_t name_len)
{
    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    return crosstalk_eda_script_eval_switch_release(&ctx->eval, name, name_len);
}

uint32_t crosstalk_eda_script_diag_get(const crosstalk_eda_script_ctx_t *ctx, const crosstalk_eda_script_diag_t **out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = &ctx->diag;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_script_path_is(const char *text, uint32_t len)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(text == 0x0)) { return LDG_TRUTH_FALSE; }

    if (len <= (uint32_t)SCRIPT_EXT_LEN) { return LDG_TRUTH_FALSE; }

    at = len - (uint32_t)SCRIPT_EXT_LEN;

    if (memcmp(&text[at], CROSSTALK_EDA_SCRIPT_EXT, (uint64_t)SCRIPT_EXT_LEN) != 0) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

uint32_t crosstalk_eda_script_exec(crosstalk_eda_script_ctx_t *ctx, const char *path, const char *src, uint32_t src_len)
{
    crosstalk_eda_script_lex_t lex = LDG_STRUCT_ZERO_INIT;
    crosstalk_eda_script_node_t *program = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || src == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    memset(&ctx->diag, 0x0, sizeof(ctx->diag));

    if (snprintf(ctx->path, sizeof(ctx->path), SCRIPT_PATH_FMT, path != 0x0 ? path : SCRIPT_PROMPT_NAME) < 0) { return LDG_ERR_IO_FMT; }

    ret = crosstalk_eda_script_ast_rst(&ctx->ast);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_eda_script_lex_init(&lex, src, src_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_eda_script_parse_program(&lex, &ctx->ast, &program, &ctx->diag);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "script: %.255s:%u32:%u32: %.159s", ctx->path, ctx->diag.line, ctx->diag.col, ctx->diag.msg);

        return ret;
    }

    ret = crosstalk_eda_script_eval_run(&ctx->eval, program);
    if (ret != LDG_ERR_AOK)
    {
        ctx->diag = ctx->eval.diag;

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "script: %.255s:%u32:%u32: %.159s", ctx->path, ctx->diag.line, ctx->diag.col, ctx->diag.msg);

        return ret;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_script_file_exec(crosstalk_eda_script_ctx_t *ctx, const char *path)
{
    ldg_io_file_t *fh = 0x0;
    uint64_t got = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, 0x0, &fh);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_rd(fh, (ldg_byte_t *)ctx->src, (uint64_t)CROSSTALK_EDA_SCRIPT_SRC_CAP - (uint64_t)LDG_STR_TERM_SIZE, &got);

    if (ldg_io_file_close(fh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "script file close failed"); }

    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->src_len = (uint32_t)got;
    ctx->src[ctx->src_len] = LDG_STR_TERM;

    if (snprintf(ctx->run_path, sizeof(ctx->run_path), SCRIPT_PATH_FMT, path) < 0) { return LDG_ERR_IO_FMT; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "script: run; bytes: %u32; path: %.255s", ctx->src_len, path);

    return crosstalk_eda_script_exec(ctx, path, ctx->src, ctx->src_len);
}

static uint8_t script_state_empty_is(const crosstalk_eda_script_ctx_t *ctx)
{
    uint32_t at = 0;

    if (ctx->run_path[0] != LDG_STR_TERM) { return LDG_TRUTH_FALSE; }

    for (at = 0; at < (uint32_t)CROSSTALK_EDA_SCRIPT_EVENT_CUNT; at = at + 0x1) { if (ctx->event_path[at][0] != LDG_STR_TERM) { return LDG_TRUTH_FALSE; } }

    return LDG_TRUTH_TRUE;
}

uint32_t crosstalk_eda_script_state_serialize(const crosstalk_eda_script_ctx_t *ctx, uint8_t *out, uint32_t cap, uint32_t *out_len)
{
    uint32_t need = (uint32_t)CROSSTALK_EDA_SCRIPT_PATH_CAP * ((uint32_t)CROSSTALK_EDA_SCRIPT_EVENT_CUNT + 0x1);
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (script_state_empty_is(ctx) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    *out_len = need;

    if (out == 0x0) { return LDG_ERR_AOK; }

    if (cap < need) { return LDG_ERR_BOUNDS; }

    memcpy(out, ctx->run_path, CROSSTALK_EDA_SCRIPT_PATH_CAP);

    for (at = 0; at < (uint32_t)CROSSTALK_EDA_SCRIPT_EVENT_CUNT; at = at + 0x1) { memcpy(out + ((uint64_t)at + 0x1) * (uint64_t)CROSSTALK_EDA_SCRIPT_PATH_CAP, ctx->event_path[at], CROSSTALK_EDA_SCRIPT_PATH_CAP); }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_script_state_deserialize(crosstalk_eda_script_ctx_t *ctx, const uint8_t *buff, uint32_t len)
{
    uint32_t slot_cunt = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || buff == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len < (uint32_t)CROSSTALK_EDA_SCRIPT_PATH_CAP || len % (uint32_t)CROSSTALK_EDA_SCRIPT_PATH_CAP != 0)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "script: state blob rejected; len: %u32; slot bytes: %u32", len, (uint32_t)CROSSTALK_EDA_SCRIPT_PATH_CAP);

        return LDG_ERR_INVALID;
    }

    slot_cunt = len / (uint32_t)CROSSTALK_EDA_SCRIPT_PATH_CAP - 0x1;
    if (slot_cunt > (uint32_t)CROSSTALK_EDA_SCRIPT_EVENT_CUNT) { slot_cunt = (uint32_t)CROSSTALK_EDA_SCRIPT_EVENT_CUNT; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "script: state blob; len: %u32; event slots: %u32", len, slot_cunt);

    memcpy(ctx->run_path, buff, CROSSTALK_EDA_SCRIPT_PATH_CAP);
    ctx->run_path[CROSSTALK_EDA_SCRIPT_PATH_CAP - LDG_STR_TERM_SIZE] = LDG_STR_TERM;

    for (at = 0; at < (uint32_t)CROSSTALK_EDA_SCRIPT_EVENT_CUNT; at = at + 0x1)
    {
        ctx->event_path[at][0] = LDG_STR_TERM;

        if (at >= slot_cunt) { continue; }

        memcpy(ctx->event_path[at], buff + ((uint64_t)at + 0x1) * (uint64_t)CROSSTALK_EDA_SCRIPT_PATH_CAP, CROSSTALK_EDA_SCRIPT_PATH_CAP);
        ctx->event_path[at][CROSSTALK_EDA_SCRIPT_PATH_CAP - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_script_event_set(crosstalk_eda_script_ctx_t *ctx, uint32_t event, const char *path)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (event >= (uint32_t)CROSSTALK_EDA_SCRIPT_EVENT_CUNT) { return LDG_ERR_RANGE; }

    if (path == 0x0 || path[0] == LDG_STR_TERM)
    {
        ctx->event_path[event][0] = LDG_STR_TERM;

        return LDG_ERR_AOK;
    }

    return ldg_str_rbrcpy(ctx->event_path[event], path, (uint64_t)CROSSTALK_EDA_SCRIPT_PATH_CAP);
}

uint32_t crosstalk_eda_script_event_get(const crosstalk_eda_script_ctx_t *ctx, uint32_t event, const char **out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (event >= (uint32_t)CROSSTALK_EDA_SCRIPT_EVENT_CUNT) { return LDG_ERR_RANGE; }

    if (ctx->event_path[event][0] == LDG_STR_TERM) { return LDG_ERR_NOT_FOUND; }

    *out = ctx->event_path[event];

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_script_event_id_get(const char *name, uint32_t *out)
{
    if (LDG_UNLIKELY(name == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;

    if (strcmp(name, CROSSTALK_EDA_SCRIPT_EVENT_NAME_LD) == 0) { *out = (uint32_t)CROSSTALK_EDA_SCRIPT_EVENT_LD; return LDG_ERR_AOK; }

    if (strcmp(name, CROSSTALK_EDA_SCRIPT_EVENT_NAME_ST) == 0) { *out = (uint32_t)CROSSTALK_EDA_SCRIPT_EVENT_ST; return LDG_ERR_AOK; }

    if (strcmp(name, CROSSTALK_EDA_SCRIPT_EVENT_NAME_EXIT) == 0) { *out = (uint32_t)CROSSTALK_EDA_SCRIPT_EVENT_EXIT; return LDG_ERR_AOK; }

    if (strcmp(name, CROSSTALK_EDA_SCRIPT_EVENT_NAME_FAULT) == 0) { *out = (uint32_t)CROSSTALK_EDA_SCRIPT_EVENT_FAULT; return LDG_ERR_AOK; }

    return LDG_ERR_NOT_FOUND;
}

uint32_t crosstalk_eda_script_event_name_get(uint32_t event, const char **out)
{
    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (event == (uint32_t)CROSSTALK_EDA_SCRIPT_EVENT_LD) { *out = CROSSTALK_EDA_SCRIPT_EVENT_NAME_LD; return LDG_ERR_AOK; }

    if (event == (uint32_t)CROSSTALK_EDA_SCRIPT_EVENT_ST) { *out = CROSSTALK_EDA_SCRIPT_EVENT_NAME_ST; return LDG_ERR_AOK; }

    if (event == (uint32_t)CROSSTALK_EDA_SCRIPT_EVENT_EXIT) { *out = CROSSTALK_EDA_SCRIPT_EVENT_NAME_EXIT; return LDG_ERR_AOK; }

    if (event == (uint32_t)CROSSTALK_EDA_SCRIPT_EVENT_FAULT) { *out = CROSSTALK_EDA_SCRIPT_EVENT_NAME_FAULT; return LDG_ERR_AOK; }

    return LDG_ERR_RANGE;
}
