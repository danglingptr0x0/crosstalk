#ifndef CROSSTALK_HDL_TYPE_H
#define CROSSTALK_HDL_TYPE_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>

#define CROSSTALK_HDL_TYPE_NAME_CAP 96
#define CROSSTALK_HDL_TYPE_PATH_CAP 128
#define CROSSTALK_HDL_TYPE_PATH_SEP '.'
#define CROSSTALK_HDL_TYPE_IDX_OPEN '['
#define CROSSTALK_HDL_TYPE_IDX_CLOSE ']'
#define CROSSTALK_HDL_TYPE_MAX 8192
#define CROSSTALK_HDL_TYPE_MEMBER_MAX 65536
#define CROSSTALK_HDL_TYPE_VAR_MAX 16384
#define CROSSTALK_HDL_TYPE_DEPTH_MAX 8

#define CROSSTALK_HDL_TYPE_KIND_NONE 0
#define CROSSTALK_HDL_TYPE_KIND_INT 1
#define CROSSTALK_HDL_TYPE_KIND_UINT 2
#define CROSSTALK_HDL_TYPE_KIND_CHAR 3
#define CROSSTALK_HDL_TYPE_KIND_BOOL 4
#define CROSSTALK_HDL_TYPE_KIND_PTR 5
#define CROSSTALK_HDL_TYPE_KIND_STRUCT 6
#define CROSSTALK_HDL_TYPE_KIND_UNION 7
#define CROSSTALK_HDL_TYPE_KIND_ARR 8
#define CROSSTALK_HDL_TYPE_KIND_ENUM 9
#define CROSSTALK_HDL_TYPE_KIND_FUNC 10

typedef struct crosstalk_hdl_type_rec
{
    char name[CROSSTALK_HDL_TYPE_NAME_CAP];
    char alias[CROSSTALK_HDL_TYPE_NAME_CAP];
    uint64_t size;
    uint32_t kind;
    uint32_t ref;
    uint32_t elem_cunt;
    uint32_t member_at;
    uint32_t member_cunt;
    uint8_t pudding[4];
} crosstalk_hdl_type_rec_t;

typedef struct crosstalk_hdl_type_member
{
    char name[CROSSTALK_HDL_TYPE_NAME_CAP];
    uint64_t off;
    uint64_t size;
    uint32_t owner;
    uint32_t type_idx;
} crosstalk_hdl_type_member_t;

typedef struct crosstalk_hdl_type_var
{
    char name[CROSSTALK_HDL_TYPE_NAME_CAP];
    uint64_t addr;
    uint64_t size;
    uint32_t type_idx;
    uint8_t pudding[4];
} crosstalk_hdl_type_var_t;

typedef struct crosstalk_hdl_type_field
{
    char path[CROSSTALK_HDL_TYPE_PATH_CAP];
    uint64_t off;
    uint64_t size;
    uint32_t kind;
    uint32_t type_idx;
    uint32_t ref;
    uint8_t pudding[4];
} crosstalk_hdl_type_field_t;

typedef struct crosstalk_hdl_type_ctx
{
    ldg_mem_pool_t *pool;
    crosstalk_hdl_type_rec_t *type;
    crosstalk_hdl_type_member_t *member;
    crosstalk_hdl_type_var_t *var;
    uint64_t gen;
    uint32_t type_cunt;
    uint32_t member_cunt;
    uint32_t var_cunt;
    uint8_t ok;
    uint8_t pudding[3];
} crosstalk_hdl_type_ctx_t;

uint32_t crosstalk_hdl_type_init(crosstalk_hdl_type_ctx_t *ctx);
uint32_t crosstalk_hdl_type_shutdown(crosstalk_hdl_type_ctx_t *ctx);
uint32_t crosstalk_hdl_type_ld(crosstalk_hdl_type_ctx_t *ctx, const char *path);
uint32_t crosstalk_hdl_type_clear(crosstalk_hdl_type_ctx_t *ctx);
uint8_t crosstalk_hdl_type_ld_is(const crosstalk_hdl_type_ctx_t *ctx);
uint32_t crosstalk_hdl_type_get(const crosstalk_hdl_type_ctx_t *ctx, uint32_t idx, const crosstalk_hdl_type_rec_t **out);
uint32_t crosstalk_hdl_type_find(const crosstalk_hdl_type_ctx_t *ctx, const char *name, uint32_t *out);
uint32_t crosstalk_hdl_type_member_get(const crosstalk_hdl_type_ctx_t *ctx, uint32_t idx, const crosstalk_hdl_type_member_t **out);
uint32_t crosstalk_hdl_type_field_at(const crosstalk_hdl_type_ctx_t *ctx, uint32_t type_idx, uint64_t off, crosstalk_hdl_type_field_t *out);
uint32_t crosstalk_hdl_type_field_find(const crosstalk_hdl_type_ctx_t *ctx, uint32_t type_idx, const char *path, crosstalk_hdl_type_field_t *out);
uint32_t crosstalk_hdl_type_var_at(const crosstalk_hdl_type_ctx_t *ctx, uint64_t addr, const crosstalk_hdl_type_var_t **out, uint64_t *out_off);
uint32_t crosstalk_hdl_type_var_get(const crosstalk_hdl_type_ctx_t *ctx, uint32_t idx, const crosstalk_hdl_type_var_t **out);
uint32_t crosstalk_hdl_type_label_get(const crosstalk_hdl_type_rec_t *rec, const char **out);
uint32_t crosstalk_hdl_type_cunt_get(const crosstalk_hdl_type_ctx_t *ctx);
uint32_t crosstalk_hdl_type_var_cunt_get(const crosstalk_hdl_type_ctx_t *ctx);

#endif
