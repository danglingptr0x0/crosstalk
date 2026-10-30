#ifndef CROSSTALK_EDA_SCRIPT_EVAL_H
#define CROSSTALK_EDA_SCRIPT_EVAL_H

#include <stdint.h>

#include <dangling/mem/alloc.h>

#include "eda/script/ast.h"
#include "eda/script/parse.h"

#define CROSSTALK_EDA_SCRIPT_SLOT_MAX 64
#define CROSSTALK_EDA_SCRIPT_SLOT_NAME_CAP 64
#define CROSSTALK_EDA_SCRIPT_SLOT_BYTES_CAP 512
#define CROSSTALK_EDA_SCRIPT_BUILTIN_MAX 128
#define CROSSTALK_EDA_SCRIPT_BUILTIN_NAME_CAP 32
#define CROSSTALK_EDA_SCRIPT_SECTION_MAX 64
#define CROSSTALK_EDA_SCRIPT_FLAT_MAX 2048
#define CROSSTALK_EDA_SCRIPT_ARGS_MAX 16
#define CROSSTALK_EDA_SCRIPT_STEP_MAX 1000000
#define CROSSTALK_EDA_SCRIPT_RACK_MAX 64
#define CROSSTALK_EDA_SCRIPT_SWITCH_MAX 64
#define CROSSTALK_EDA_SCRIPT_NOTCH_MAX 16
#define CROSSTALK_EDA_SCRIPT_ACTION_CAP 512

#define CROSSTALK_EDA_SCRIPT_VAL_VOID 0
#define CROSSTALK_EDA_SCRIPT_VAL_SCALAR 1
#define CROSSTALK_EDA_SCRIPT_VAL_STR 2

typedef struct crosstalk_eda_script_val
{
    const uint8_t *bytes;
    uint64_t val;
    uint32_t bytes_len;
    uint32_t width;
    uint32_t kind;
    uint8_t quoted;
    uint8_t pudding[3];
} crosstalk_eda_script_val_t;

typedef uint32_t (*crosstalk_eda_script_builtin_cb_t)(void *user, const char *name, uint32_t name_len, const crosstalk_eda_script_val_t *arg, uint32_t arg_cunt, crosstalk_eda_script_val_t *out);

typedef struct crosstalk_eda_script_builtin
{
    char name[CROSSTALK_EDA_SCRIPT_BUILTIN_NAME_CAP];
    crosstalk_eda_script_builtin_cb_t cb;
    uint32_t name_len;
    uint32_t min_args;
    uint32_t max_args;
    uint32_t ret_width;
} crosstalk_eda_script_builtin_t;

typedef struct crosstalk_eda_script_slot
{
    char name[CROSSTALK_EDA_SCRIPT_SLOT_NAME_CAP];
    uint8_t bytes[CROSSTALK_EDA_SCRIPT_SLOT_BYTES_CAP];
    uint64_t val;
    uint64_t r_lo;
    uint64_t r_hi;
    uint32_t name_len;
    uint32_t width;
    uint32_t arr_cunt;
    uint8_t arr_is;
    uint8_t const_is;
    uint8_t inited;
    uint8_t has_range;
    uint8_t r_lo_open;
    uint8_t r_hi_open;
    uint8_t pudding[2];
} crosstalk_eda_script_slot_t;

typedef struct crosstalk_eda_script_section
{
    const char *name;
    uint32_t name_len;
    uint32_t pc;
} crosstalk_eda_script_section_t;

typedef struct crosstalk_eda_script_switch
{
    char name[CROSSTALK_EDA_SCRIPT_SLOT_NAME_CAP];
    char action[CROSSTALK_EDA_SCRIPT_ACTION_CAP];
    uint64_t notch[CROSSTALK_EDA_SCRIPT_NOTCH_MAX];
    uint64_t r_lo;
    uint64_t r_hi;
    uint32_t name_len;
    uint32_t action_len;
    uint32_t kind;
    uint32_t width;
    uint32_t notch_cunt;
    uint32_t rack_idx;
    uint8_t has_range;
    uint8_t r_lo_open;
    uint8_t r_hi_open;
    uint8_t pudding[5];
} crosstalk_eda_script_switch_t;

typedef struct crosstalk_eda_script_rack
{
    char name[CROSSTALK_EDA_SCRIPT_SLOT_NAME_CAP];
    uint32_t name_len;
    uint8_t pudding[4];
} crosstalk_eda_script_rack_t;

typedef struct crosstalk_eda_script_eval
{
    crosstalk_eda_script_slot_t slot[CROSSTALK_EDA_SCRIPT_SLOT_MAX];
    crosstalk_eda_script_builtin_t builtin[CROSSTALK_EDA_SCRIPT_BUILTIN_MAX];
    crosstalk_eda_script_section_t section[CROSSTALK_EDA_SCRIPT_SECTION_MAX];
    crosstalk_eda_script_switch_t switches[CROSSTALK_EDA_SCRIPT_SWITCH_MAX];
    crosstalk_eda_script_rack_t rack[CROSSTALK_EDA_SCRIPT_RACK_MAX];
    crosstalk_eda_script_node_t *flat[CROSSTALK_EDA_SCRIPT_FLAT_MAX];
    crosstalk_eda_script_diag_t diag;
    void *user;
    uint32_t slots_cunt;
    uint32_t builtins_cunt;
    uint32_t sections_cunt;
    uint32_t flat_cunt;
    uint32_t switches_cunt;
    uint32_t racks_cunt;
    uint8_t ok;
    uint8_t action_is;
    uint8_t pudding[6];
} crosstalk_eda_script_eval_t;

uint32_t crosstalk_eda_script_eval_init(crosstalk_eda_script_eval_t *ctx, void *user);
uint32_t crosstalk_eda_script_eval_shutdown(crosstalk_eda_script_eval_t *ctx);
uint32_t crosstalk_eda_script_eval_slots_clear(crosstalk_eda_script_eval_t *ctx);
uint32_t crosstalk_eda_script_eval_builtin_reg(crosstalk_eda_script_eval_t *ctx, const char *name, crosstalk_eda_script_builtin_cb_t cb, uint32_t min_args, uint32_t max_args, uint32_t ret_width);
uint32_t crosstalk_eda_script_eval_run(crosstalk_eda_script_eval_t *ctx, crosstalk_eda_script_node_t *program);
uint32_t crosstalk_eda_script_eval_slot_get(const crosstalk_eda_script_eval_t *ctx, const char *name, uint32_t name_len, crosstalk_eda_script_val_t *out);
uint32_t crosstalk_eda_script_val_num_get(const crosstalk_eda_script_val_t *arg, uint64_t *out);
uint32_t crosstalk_eda_script_eval_racks_clear(crosstalk_eda_script_eval_t *ctx);
uint32_t crosstalk_eda_script_eval_switch_find(const crosstalk_eda_script_eval_t *ctx, const char *name, uint32_t name_len, const crosstalk_eda_script_switch_t **out);
uint32_t crosstalk_eda_script_eval_switch_pos_set(crosstalk_eda_script_eval_t *ctx, const char *name, uint32_t name_len, uint64_t val, const char **out_action, uint32_t *out_action_len);
uint32_t crosstalk_eda_script_eval_switch_release(crosstalk_eda_script_eval_t *ctx, const char *name, uint32_t name_len);

#endif
