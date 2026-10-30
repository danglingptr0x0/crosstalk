#ifndef CROSSTALK_HDL_PREPROC_H
#define CROSSTALK_HDL_PREPROC_H

#include <stdint.h>

#define CROSSTALK_HDL_ERR_PREPROC_DIRECTIVE 730
#define CROSSTALK_HDL_ERR_PREPROC_MACRO 731
#define CROSSTALK_HDL_ERR_PREPROC_COND 732
#define CROSSTALK_HDL_ERR_PREPROC_DEPTH 733
#define CROSSTALK_HDL_ERR_PREPROC_INCLUDE 734

#define CROSSTALK_HDL_PREPROC_MACRO_MAX 512
#define CROSSTALK_HDL_PREPROC_PARAM_MAX 16
#define CROSSTALK_HDL_PREPROC_COND_MAX 64
#define CROSSTALK_HDL_PREPROC_EXPAND_MAX 16
#define CROSSTALK_HDL_PREPROC_INCLUDE_MAX 8
#define CROSSTALK_HDL_PREPROC_OUT_SLACK 0x10000

typedef uint32_t (*crosstalk_hdl_preproc_include_cb_t)(void *user, const char *name, uint32_t name_len, char **out_text, uint32_t *out_len);

typedef struct crosstalk_hdl_preproc_macro
{
    const char *name;
    const char *body;
    const char *param[CROSSTALK_HDL_PREPROC_PARAM_MAX];
    uint32_t param_len[CROSSTALK_HDL_PREPROC_PARAM_MAX];
    uint32_t name_len;
    uint32_t body_len;
    uint32_t param_cunt;
    uint8_t func_is;
    uint8_t pudding[3];
} crosstalk_hdl_preproc_macro_t;

typedef struct crosstalk_hdl_preproc_cond
{
    uint8_t live;
    uint8_t taken;
    uint8_t else_seen;
    uint8_t parent_live;
} crosstalk_hdl_preproc_cond_t;

typedef struct crosstalk_hdl_preproc_ctx
{
    void *pool;
    crosstalk_hdl_preproc_macro_t *macros;
    char *out;
    crosstalk_hdl_preproc_include_cb_t include_cb;
    void *include_user;
    crosstalk_hdl_preproc_cond_t cond[CROSSTALK_HDL_PREPROC_COND_MAX];
    uint32_t macro_cunt;
    uint32_t cond_depth;
    uint32_t out_len;
    uint32_t out_cap;
    uint32_t expand_depth;
    uint32_t include_depth;
    const char *err_name;
    uint32_t err_name_len;
    uint8_t pudding[4];
} crosstalk_hdl_preproc_ctx_t;

uint32_t crosstalk_hdl_preproc_init(crosstalk_hdl_preproc_ctx_t *ctx, void *pool, crosstalk_hdl_preproc_include_cb_t include_cb, void *include_user);
uint32_t crosstalk_hdl_preproc_define_set(crosstalk_hdl_preproc_ctx_t *ctx, const char *name, uint32_t name_len, const char *body, uint32_t body_len);
uint32_t crosstalk_hdl_preproc_run(crosstalk_hdl_preproc_ctx_t *ctx, const char *src, uint32_t len, char **out_src, uint32_t *out_len);

#endif
