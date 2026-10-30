#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "eda/script/eval.h"

#define EVAL_STEP 1
#define EVAL_BIT 1
#define EVAL_DIAG_FMT "%s"
#define EVAL_ASCII_NINE '9'
#define EVAL_RADIX_DEC 10
#define EVAL_RADIX_HEX 16
#define EVAL_HEX_ALPHA 10
#define EVAL_BYTE_BITS 8

typedef struct eval_lv
{
    crosstalk_eda_script_slot_t *slot;
    uint32_t elem_idx;
    uint32_t bit_hi;
    uint32_t bit_lo;
    uint8_t elem_is;
    uint8_t bits_is;
    uint8_t pudding[6];
} eval_lv_t;

static uint32_t eval_expr(crosstalk_eda_script_eval_t *ctx, const crosstalk_eda_script_node_t *node, crosstalk_eda_script_val_t *out);

static uint64_t eval_mask(uint32_t width)
{
    if (width >= (uint32_t)CROSSTALK_EDA_SCRIPT_WIDTH_MAX) { return UINT64_MAX; }

    return (((uint64_t)EVAL_BIT) << width) - (uint64_t)EVAL_BIT;
}

static uint32_t eval_width_fit(uint32_t bits)
{
    uint32_t width = (uint32_t)EVAL_BIT;

    while (width < (uint32_t)CROSSTALK_EDA_SCRIPT_WIDTH_MAX && width < bits) { width = width + width; }

    return width;
}

static uint32_t eval_diag_set(crosstalk_eda_script_eval_t *ctx, const crosstalk_eda_script_node_t *node, uint32_t code, const char *msg, const char *suggestion)
{
    ctx->diag.line = node != 0x0 ? node->line : 0x0;
    ctx->diag.col = node != 0x0 ? node->col : 0x0;

    if (snprintf(ctx->diag.msg, sizeof(ctx->diag.msg), EVAL_DIAG_FMT, msg) < 0) { return code; }

    if (snprintf(ctx->diag.suggestion, sizeof(ctx->diag.suggestion), EVAL_DIAG_FMT, suggestion) < 0) { return code; }

    return code;
}

static crosstalk_eda_script_slot_t* eval_slot_find(crosstalk_eda_script_eval_t *ctx, const char *name, uint32_t name_len)
{
    uint32_t idx = 0;

    for (idx = 0; idx < ctx->slots_cunt; idx = idx + (uint32_t)EVAL_STEP)
    {
        if (ctx->slot[idx].name_len != name_len) { continue; }

        if (memcmp(ctx->slot[idx].name, name, name_len) != 0) { continue; }

        return &ctx->slot[idx];
    }

    return 0x0;
}

static const crosstalk_eda_script_builtin_t* eval_builtin_find(const crosstalk_eda_script_eval_t *ctx, const char *name, uint32_t name_len)
{
    uint32_t idx = 0;

    for (idx = 0; idx < ctx->builtins_cunt; idx = idx + (uint32_t)EVAL_STEP)
    {
        if (ctx->builtin[idx].name_len != name_len) { continue; }

        if (memcmp(ctx->builtin[idx].name, name, name_len) != 0) { continue; }

        return &ctx->builtin[idx];
    }

    return 0x0;
}

static const crosstalk_eda_script_section_t* eval_section_find(const crosstalk_eda_script_eval_t *ctx, const char *name, uint32_t name_len)
{
    uint32_t idx = 0;

    for (idx = 0; idx < ctx->sections_cunt; idx = idx + (uint32_t)EVAL_STEP)
    {
        if (ctx->section[idx].name_len != name_len) { continue; }

        if (memcmp(ctx->section[idx].name, name, name_len) != 0) { continue; }

        return &ctx->section[idx];
    }

    return 0x0;
}

static uint32_t eval_elem_size(uint32_t width)
{
    if (width <= (uint32_t)EVAL_BYTE_BITS) { return (uint32_t)EVAL_STEP; }

    return width / (uint32_t)EVAL_BYTE_BITS;
}

static uint64_t eval_elem_ld(const crosstalk_eda_script_slot_t *slot, uint32_t idx)
{
    uint64_t val = 0;
    uint32_t size = eval_elem_size(slot->width);

    memcpy(&val, slot->bytes + (uint64_t)idx * size, size);

    return val & eval_mask(slot->width);
}

static void eval_elem_st(crosstalk_eda_script_slot_t *slot, uint32_t idx, uint64_t val)
{
    uint32_t size = eval_elem_size(slot->width);
    uint64_t masked = val & eval_mask(slot->width);

    memcpy(slot->bytes + (uint64_t)idx * size, &masked, size);
}

static uint8_t eval_range_ok_is(const crosstalk_eda_script_slot_t *slot, uint64_t val)
{
    if (slot->has_range != LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    if (slot->r_lo_open == LDG_TRUTH_TRUE && val <= slot->r_lo) { return LDG_TRUTH_FALSE; }

    if (slot->r_lo_open != LDG_TRUTH_TRUE && val < slot->r_lo) { return LDG_TRUTH_FALSE; }

    if (slot->r_hi_open == LDG_TRUTH_TRUE && val >= slot->r_hi) { return LDG_TRUTH_FALSE; }

    if (slot->r_hi_open != LDG_TRUTH_TRUE && val > slot->r_hi) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static crosstalk_eda_script_switch_t* eval_switch_lookup(crosstalk_eda_script_eval_t *ctx, const char *name, uint32_t name_len)
{
    uint32_t idx = 0;

    for (idx = 0; idx < ctx->switches_cunt; idx = idx + (uint32_t)EVAL_STEP)
    {
        if (ctx->switches[idx].name_len != name_len) { continue; }

        if (memcmp(ctx->switches[idx].name, name, name_len) != 0) { continue; }

        return &ctx->switches[idx];
    }

    return 0x0;
}

static uint32_t eval_rack_idx_find(const crosstalk_eda_script_eval_t *ctx, const char *name, uint32_t name_len)
{
    uint32_t idx = 0;

    for (idx = 0; idx < ctx->racks_cunt; idx = idx + (uint32_t)EVAL_STEP)
    {
        if (ctx->rack[idx].name_len != name_len) { continue; }

        if (memcmp(ctx->rack[idx].name, name, name_len) != 0) { continue; }

        return idx;
    }

    return UINT32_MAX;
}

static uint8_t eval_switch_notch_ok_is(const crosstalk_eda_script_switch_t *switch_ref, uint64_t val)
{
    uint32_t idx = 0;

    for (idx = 0; idx < switch_ref->notch_cunt; idx = idx + (uint32_t)EVAL_STEP) { if (switch_ref->notch[idx] == val) { return LDG_TRUTH_TRUE; } }

    return LDG_TRUTH_FALSE;
}

static uint8_t eval_switch_pos_ok_is(const crosstalk_eda_script_switch_t *switch_ref, const crosstalk_eda_script_slot_t *slot, uint64_t val)
{
    if (switch_ref->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_SWITCH_LEVER || switch_ref->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_SWITCH_BUTTON) { return val <= (uint64_t)EVAL_BIT ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    if (switch_ref->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_SWITCH_ROTARY) { return eval_switch_notch_ok_is(switch_ref, val); }

    return eval_range_ok_is(slot, val);
}

static void eval_rack_switches_drop(crosstalk_eda_script_eval_t *ctx, uint32_t rack_idx)
{
    uint32_t at = 0;
    uint32_t idx = 0;

    for (idx = 0; idx < ctx->switches_cunt; idx = idx + (uint32_t)EVAL_STEP)
    {
        if (ctx->switches[idx].rack_idx == rack_idx) { continue; }

        if (at != idx) { ctx->switches[at] = ctx->switches[idx]; }

        at = at + (uint32_t)EVAL_STEP;
    }

    ctx->switches_cunt = at;
}

static uint32_t eval_switch_reg(crosstalk_eda_script_eval_t *ctx, const crosstalk_eda_script_node_t *stmt, uint32_t rack_idx)
{
    crosstalk_eda_script_switch_t *switch_ref = eval_switch_lookup(ctx, stmt->name, stmt->name_len);
    crosstalk_eda_script_slot_t *slot = 0x0;
    const crosstalk_eda_script_node_t *notch = 0x0;
    uint64_t init_val = stmt->val & eval_mask(stmt->width);

    if (LDG_UNLIKELY(stmt->name_len >= (uint32_t)CROSSTALK_EDA_SCRIPT_SLOT_NAME_CAP)) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_FULL, "switch name exceeds the name cap", "trim the switch name"); }

    if (LDG_UNLIKELY(eval_builtin_find(ctx, stmt->name, stmt->name_len) != 0x0)) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_REDECL, "switch name is already a command", "rename the switch"); }

    if (LDG_UNLIKELY(switch_ref != 0x0 && switch_ref->rack_idx != rack_idx)) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_REDECL, "switch name is taken by another rack", "give each switch its own name across racks"); }

    if (switch_ref == 0x0)
    {
        if (LDG_UNLIKELY(ctx->switches_cunt >= (uint32_t)CROSSTALK_EDA_SCRIPT_SWITCH_MAX)) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_FULL, "too many switches", "describe fewer switches across the racks"); }

        switch_ref = &ctx->switches[ctx->switches_cunt];
        ctx->switches_cunt = ctx->switches_cunt + (uint32_t)EVAL_STEP;
    }

    memset(switch_ref, 0x0, sizeof(*switch_ref));
    memcpy(switch_ref->name, stmt->name, stmt->name_len);

    switch_ref->name_len = stmt->name_len;
    switch_ref->kind = stmt->op;
    switch_ref->width = stmt->width;
    switch_ref->rack_idx = rack_idx;
    switch_ref->has_range = stmt->has_range;
    switch_ref->r_lo = stmt->r_lo & eval_mask(stmt->width);
    switch_ref->r_hi = stmt->r_hi & eval_mask(stmt->width);
    switch_ref->r_lo_open = stmt->r_lo_open;
    switch_ref->r_hi_open = stmt->r_hi_open;

    notch = stmt->list.hd;

    while (notch != 0x0)
    {
        if (LDG_UNLIKELY(switch_ref->notch_cunt >= (uint32_t)CROSSTALK_EDA_SCRIPT_NOTCH_MAX)) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_FULL, "too many notches", "give the rotary fewer notch values"); }

        switch_ref->notch[switch_ref->notch_cunt] = notch->val & eval_mask(stmt->width);
        switch_ref->notch_cunt = switch_ref->notch_cunt + (uint32_t)EVAL_STEP;
        notch = notch->next;
    }

    if (stmt->bytes != 0x0)
    {
        if (LDG_UNLIKELY(stmt->bytes_len >= (uint32_t)CROSSTALK_EDA_SCRIPT_ACTION_CAP)) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_FULL, "switch action exceeds the action cap", "trim the action body"); }

        memcpy(switch_ref->action, stmt->bytes, stmt->bytes_len);

        switch_ref->action_len = stmt->bytes_len;
    }

    slot = eval_slot_find(ctx, stmt->name, stmt->name_len);

    if (slot == 0x0)
    {
        if (LDG_UNLIKELY(ctx->slots_cunt >= (uint32_t)CROSSTALK_EDA_SCRIPT_SLOT_MAX)) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_FULL, "too many variables in this session", "clear the session or use fewer variables"); }

        slot = &ctx->slot[ctx->slots_cunt];
        memset(slot, 0x0, sizeof(*slot));
        memcpy(slot->name, stmt->name, stmt->name_len);

        slot->name_len = stmt->name_len;
        ctx->slots_cunt = ctx->slots_cunt + (uint32_t)EVAL_STEP;
    }
    else if (slot->const_is == LDG_TRUTH_TRUE || slot->arr_is == LDG_TRUTH_TRUE) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_REDECL, "switch name is taken by a constant or an array", "rename the switch"); }

    slot->width = stmt->width;
    slot->has_range = switch_ref->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_SWITCH_ENCODER ? stmt->has_range : LDG_TRUTH_FALSE;
    slot->r_lo = switch_ref->r_lo;
    slot->r_hi = switch_ref->r_hi;
    slot->r_lo_open = stmt->r_lo_open;
    slot->r_hi_open = stmt->r_hi_open;

    if (LDG_UNLIKELY(eval_switch_pos_ok_is(switch_ref, slot, init_val) != LDG_TRUTH_TRUE)) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_RANGE, "initial position is outside the switch geometry", "keep the initial position inside the range or on a notch"); }

    if (slot->inited != LDG_TRUTH_TRUE || eval_switch_pos_ok_is(switch_ref, slot, slot->val) != LDG_TRUTH_TRUE) { slot->val = init_val; }

    slot->inited = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t eval_rack_reg(crosstalk_eda_script_eval_t *ctx, const crosstalk_eda_script_node_t *node)
{
    const crosstalk_eda_script_node_t *switch_node = 0x0;
    uint32_t rack_idx = eval_rack_idx_find(ctx, node->name, node->name_len);
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx->action_is == LDG_TRUTH_TRUE)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_SWITCH, "racks cannot be described inside switch actions", "describe racks at the script level"); }

    if (LDG_UNLIKELY(node->name_len >= (uint32_t)CROSSTALK_EDA_SCRIPT_SLOT_NAME_CAP)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_FULL, "rack name exceeds the name cap", "trim the rack name"); }

    if (rack_idx == UINT32_MAX)
    {
        if (LDG_UNLIKELY(ctx->racks_cunt >= (uint32_t)CROSSTALK_EDA_SCRIPT_RACK_MAX)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_FULL, "too many racks", "describe fewer racks in this session"); }

        rack_idx = ctx->racks_cunt;
        memset(&ctx->rack[rack_idx], 0x0, sizeof(ctx->rack[0]));
        memcpy(ctx->rack[rack_idx].name, node->name, node->name_len);

        ctx->rack[rack_idx].name_len = node->name_len;
        ctx->racks_cunt = ctx->racks_cunt + (uint32_t)EVAL_STEP;
    }
    else { eval_rack_switches_drop(ctx, rack_idx); }

    switch_node = node->list.hd;

    while (switch_node != 0x0)
    {
        ret = eval_switch_reg(ctx, switch_node, rack_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        switch_node = switch_node->next;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_script_val_num_get(const crosstalk_eda_script_val_t *arg, uint64_t *out)
{
    uint64_t val = 0;
    uint64_t radix = (uint64_t)EVAL_RADIX_DEC;
    uint32_t at = 0;
    uint32_t digits = 0;

    if (LDG_UNLIKELY(arg == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (arg->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_SCALAR)
    {
        *out = arg->val;

        return LDG_ERR_AOK;
    }

    if (arg->kind != (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_STR || arg->bytes == 0x0) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (arg->bytes_len > (uint32_t)EVAL_STEP && arg->bytes[0] == (uint8_t)LDG_ASCII_DIGIT_ZERO && (arg->bytes[1] == (uint8_t)'x' || arg->bytes[1] == (uint8_t)'X'))
    {
        radix = (uint64_t)EVAL_RADIX_HEX;
        at = (uint32_t)EVAL_STEP + (uint32_t)EVAL_STEP;
    }

    while (at < arg->bytes_len)
    {
        char ch = (char)arg->bytes[at];
        uint64_t dig = 0;

        if (ch >= LDG_ASCII_DIGIT_ZERO && ch <= EVAL_ASCII_NINE) { dig = (uint64_t)(ch - LDG_ASCII_DIGIT_ZERO); }
        else if (radix == (uint64_t)EVAL_RADIX_HEX && ch >= 'a' && ch <= 'f') { dig = (uint64_t)(ch - 'a') + (uint64_t)EVAL_HEX_ALPHA; }
        else if (radix == (uint64_t)EVAL_RADIX_HEX && ch >= 'A' && ch <= 'F') { dig = (uint64_t)(ch - 'A') + (uint64_t)EVAL_HEX_ALPHA; }
        else { return LDG_ERR_FUNC_ARG_INVALID; }

        val = val * radix + dig;
        digits = digits + (uint32_t)EVAL_STEP;
        at = at + (uint32_t)EVAL_STEP;
    }

    if (digits == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = val;

    return LDG_ERR_AOK;
}

static uint32_t eval_builtin_call(crosstalk_eda_script_eval_t *ctx, const crosstalk_eda_script_node_t *node, const char *name, uint32_t name_len, const crosstalk_eda_script_list_t *args, crosstalk_eda_script_val_t *out)
{
    crosstalk_eda_script_val_t vals[CROSSTALK_EDA_SCRIPT_ARGS_MAX] = LDG_ARR_ZERO_INIT;
    crosstalk_eda_script_val_t ret_val = LDG_STRUCT_ZERO_INIT;
    const crosstalk_eda_script_builtin_t *builtin = eval_builtin_find(ctx, name, name_len);
    const crosstalk_eda_script_node_t *arg = 0x0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(builtin == 0x0)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_UNDEF_FUNC, "unknown command", "check the spelling, or type help for the command list"); }

    if (LDG_UNLIKELY(args->cunt > (uint32_t)CROSSTALK_EDA_SCRIPT_ARGS_MAX)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_ARG_CUNT, "too many arguments", "pass at most sixteen arguments"); }

    if (LDG_UNLIKELY(args->cunt < builtin->min_args || args->cunt > builtin->max_args)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_ARG_CUNT, "wrong number of arguments", "check the command usage with help"); }

    arg = args->hd;

    while (arg != 0x0)
    {
        if (arg->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_STR)
        {
            vals[idx].kind = (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_STR;
            vals[idx].bytes = (const uint8_t *)arg->bytes;
            vals[idx].bytes_len = arg->bytes_len;
            vals[idx].width = arg->width;
            vals[idx].quoted = arg->quoted;
        }
        else
        {
            crosstalk_eda_script_val_t got = LDG_STRUCT_ZERO_INIT;

            ret = eval_expr(ctx, arg, &got);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (LDG_UNLIKELY(got.kind == (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_VOID)) { return eval_diag_set(ctx, arg, CROSSTALK_EDA_SCRIPT_ERR_TYPE, "argument has no value", "pass a number, a string, or an array"); }

            vals[idx] = got;
        }

        idx = idx + (uint32_t)EVAL_STEP;
        arg = arg->next;
    }

    ret = builtin->cb(ctx->user, builtin->name, builtin->name_len, vals, args->cunt, &ret_val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return eval_diag_set(ctx, node, ret, "the command failed", "see the log for the reported error code"); }

    if (builtin->ret_width == 0)
    {
        out->kind = (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_VOID;

        return LDG_ERR_AOK;
    }

    out->kind = (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_SCALAR;
    out->width = builtin->ret_width;
    out->val = ret_val.val & eval_mask(builtin->ret_width);

    return LDG_ERR_AOK;
}

static uint32_t eval_lv_resolve(crosstalk_eda_script_eval_t *ctx, const crosstalk_eda_script_node_t *node, eval_lv_t *out)
{
    const crosstalk_eda_script_node_t *obj = 0x0;
    crosstalk_eda_script_slot_t *slot = 0x0;
    uint32_t ret = 0;

    memset(out, 0x0, sizeof(*out));

    if (node->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_IDENT)
    {
        slot = eval_slot_find(ctx, node->name, node->name_len);

        if (LDG_UNLIKELY(slot == 0x0)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_UNDEF_VAR, "undeclared name", "declare the variable before using it"); }

        if (LDG_UNLIKELY(slot->arr_is == LDG_TRUTH_TRUE)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_LVALUE, "whole arrays cannot be assigned", "assign one element at a time"); }

        out->slot = slot;

        return LDG_ERR_AOK;
    }

    if (node->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_IDX)
    {
        crosstalk_eda_script_val_t idx_val = LDG_STRUCT_ZERO_INIT;

        obj = node->lhs;

        if (LDG_UNLIKELY(obj->kind != (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_IDENT)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_LVALUE, "subscript target is not a variable", "subscript a declared variable"); }

        slot = eval_slot_find(ctx, obj->name, obj->name_len);

        if (LDG_UNLIKELY(slot == 0x0)) { return eval_diag_set(ctx, obj, CROSSTALK_EDA_SCRIPT_ERR_UNDEF_VAR, "undeclared name", "declare the variable before using it"); }

        if (slot->arr_is == LDG_TRUTH_TRUE)
        {
            ret = eval_expr(ctx, node->rhs, &idx_val);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (LDG_UNLIKELY(idx_val.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_SCALAR)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_TYPE, "subscript is not a number", "subscript with a numeric expression"); }

            if (LDG_UNLIKELY(idx_val.val >= slot->arr_cunt)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_BOUNDS, "subscript is past the end of the array", "keep the subscript below the declared size"); }

            out->slot = slot;
            out->elem_is = LDG_TRUTH_TRUE;
            out->elem_idx = (uint32_t)idx_val.val;

            return LDG_ERR_AOK;
        }

        if (LDG_UNLIKELY(node->rhs->kind != (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_NUM)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "bit position is not constant", "use a sized literal as the bit position"); }

        if (LDG_UNLIKELY(node->rhs->val >= (uint64_t)slot->width)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_BOUNDS, "bit position is past the operand width", "keep the bit position below the declared width"); }

        out->slot = slot;
        out->bits_is = LDG_TRUTH_TRUE;
        out->bit_hi = (uint32_t)node->rhs->val;
        out->bit_lo = out->bit_hi;

        return LDG_ERR_AOK;
    }

    if (node->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_SLICE)
    {
        obj = node->lhs;

        if (LDG_UNLIKELY(obj->kind != (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_IDENT)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_LVALUE, "slice target is not a variable", "slice a declared variable"); }

        slot = eval_slot_find(ctx, obj->name, obj->name_len);

        if (LDG_UNLIKELY(slot == 0x0)) { return eval_diag_set(ctx, obj, CROSSTALK_EDA_SCRIPT_ERR_UNDEF_VAR, "undeclared name", "declare the variable before using it"); }

        if (LDG_UNLIKELY(slot->arr_is == LDG_TRUTH_TRUE)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_TYPE, "arrays cannot be sliced", "slice a plain variable"); }

        out->slot = slot;
        out->bits_is = LDG_TRUTH_TRUE;
        out->bit_hi = (uint32_t)node->rhs->val;
        out->bit_lo = (uint32_t)node->third->val;

        if (LDG_UNLIKELY(out->bit_hi >= slot->width || out->bit_hi < out->bit_lo)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_BOUNDS, "slice bounds are out of order or too wide", "write the slice as hi:lo inside the declared width"); }

        return LDG_ERR_AOK;
    }

    return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_LVALUE, "target cannot be assigned to", "assign to a variable, an array element, or a bit slice");
}

static uint32_t eval_lv_ld(crosstalk_eda_script_eval_t *ctx, const crosstalk_eda_script_node_t *node, const eval_lv_t *lv, uint64_t *out_val, uint32_t *out_width)
{
    if (LDG_UNLIKELY(lv->slot->inited != LDG_TRUTH_TRUE)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_UNDEF_VAR, "variable is used before it holds anything", "run the declaration before this line"); }

    if (lv->elem_is == LDG_TRUTH_TRUE)
    {
        *out_val = eval_elem_ld(lv->slot, lv->elem_idx);
        *out_width = lv->slot->width;

        return LDG_ERR_AOK;
    }

    if (lv->bits_is == LDG_TRUTH_TRUE)
    {
        uint32_t bits = lv->bit_hi - lv->bit_lo + (uint32_t)EVAL_STEP;

        *out_val = (lv->slot->val >> lv->bit_lo) & eval_mask(bits);
        *out_width = eval_width_fit(bits);

        return LDG_ERR_AOK;
    }

    *out_val = lv->slot->val;
    *out_width = lv->slot->width;

    return LDG_ERR_AOK;
}

static uint32_t eval_lv_st(crosstalk_eda_script_eval_t *ctx, const crosstalk_eda_script_node_t *node, const eval_lv_t *lv, uint64_t val)
{
    uint64_t masked = 0;

    if (LDG_UNLIKELY(lv->slot->const_is == LDG_TRUTH_TRUE && lv->slot->inited == LDG_TRUTH_TRUE)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_CONST, "this variable is const", "drop the const, or remove the assignment"); }

    if (lv->elem_is == LDG_TRUTH_TRUE)
    {
        masked = val & eval_mask(lv->slot->width);

        if (LDG_UNLIKELY(eval_range_ok_is(lv->slot, masked) != LDG_TRUTH_TRUE)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_RANGE, "assigned outside the declared range", "keep the assignment inside the range annotation"); }

        eval_elem_st(lv->slot, lv->elem_idx, masked);

        return LDG_ERR_AOK;
    }

    if (lv->bits_is == LDG_TRUTH_TRUE)
    {
        uint32_t bits = lv->bit_hi - lv->bit_lo + (uint32_t)EVAL_STEP;
        uint64_t field = eval_mask(bits);
        uint64_t full = 0;

        if (LDG_UNLIKELY(lv->slot->inited != LDG_TRUTH_TRUE)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_UNDEF_VAR, "variable is used before it holds anything", "run the declaration before this line"); }

        full = (lv->slot->val & ~(field << lv->bit_lo)) | ((val & field) << lv->bit_lo);

        if (LDG_UNLIKELY(eval_range_ok_is(lv->slot, full) != LDG_TRUTH_TRUE)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_RANGE, "assigned outside the declared range", "keep the assignment inside the range annotation"); }

        lv->slot->val = full;

        return LDG_ERR_AOK;
    }

    masked = val & eval_mask(lv->slot->width);

    if (LDG_UNLIKELY(eval_range_ok_is(lv->slot, masked) != LDG_TRUTH_TRUE)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_RANGE, "assigned outside the declared range", "keep the assignment inside the range annotation"); }

    lv->slot->val = masked;
    lv->slot->inited = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t eval_bin_apply(crosstalk_eda_script_eval_t *ctx, const crosstalk_eda_script_node_t *node, uint32_t op, uint64_t lhs, uint64_t rhs, uint32_t width, uint64_t *out)
{
    uint64_t mask = eval_mask(width);

    if (op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_ADD) { *out = (lhs + rhs) & mask; return LDG_ERR_AOK; }

    if (op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_SUB) { *out = (lhs - rhs) & mask; return LDG_ERR_AOK; }

    if (op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_MUL) { *out = (lhs * rhs) & mask; return LDG_ERR_AOK; }

    if (op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_DIV)
    {
        if (LDG_UNLIKELY(rhs == 0)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_DIV_ZERO, "division by zero", "guard the divisor before dividing"); }

        *out = (lhs / rhs) & mask;

        return LDG_ERR_AOK;
    }

    if (op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_MOD)
    {
        if (LDG_UNLIKELY(rhs == 0)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_DIV_ZERO, "remainder by zero", "guard the divisor before taking a remainder"); }

        *out = (lhs % rhs) & mask;

        return LDG_ERR_AOK;
    }

    if (op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_BAND) { *out = lhs & rhs; return LDG_ERR_AOK; }

    if (op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_BOR) { *out = lhs | rhs; return LDG_ERR_AOK; }

    if (op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_BXOR) { *out = lhs ^ rhs; return LDG_ERR_AOK; }

    if (op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_BXNOR) { *out = (~(lhs ^ rhs)) & mask; return LDG_ERR_AOK; }

    return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "operator is not supported here", "use a supported operator");
}

static uint32_t eval_shift_apply(uint32_t op, uint64_t lhs, uint64_t rhs, uint32_t width, uint64_t *out)
{
    uint64_t shift = width != 0 ? rhs % (uint64_t)width : 0x0;
    uint64_t sign = 0;
    uint64_t shifted = 0;

    if (op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_SHL || op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_SHLA)
    {
        *out = (lhs << shift) & eval_mask(width);

        return LDG_ERR_AOK;
    }

    if (op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_SHR)
    {
        *out = lhs >> shift;

        return LDG_ERR_AOK;
    }

    sign = (lhs >> (width - (uint32_t)EVAL_STEP)) & (uint64_t)EVAL_BIT;
    shifted = lhs >> shift;

    if (sign != 0 && shift != 0) { shifted = shifted | (eval_mask(width) << (width - shift)); }

    *out = shifted & eval_mask(width);

    return LDG_ERR_AOK;
}

static uint32_t eval_cmp_apply(uint32_t op, uint64_t lhs, uint64_t rhs, uint64_t *out)
{
    if (op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_LT) { *out = lhs < rhs ? (uint64_t)EVAL_BIT : 0x0; return LDG_ERR_AOK; }

    if (op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_GT) { *out = lhs > rhs ? (uint64_t)EVAL_BIT : 0x0; return LDG_ERR_AOK; }

    if (op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_LTE) { *out = lhs <= rhs ? (uint64_t)EVAL_BIT : 0x0; return LDG_ERR_AOK; }

    if (op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_GTE) { *out = lhs >= rhs ? (uint64_t)EVAL_BIT : 0x0; return LDG_ERR_AOK; }

    if (op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_EQ) { *out = lhs == rhs ? (uint64_t)EVAL_BIT : 0x0; return LDG_ERR_AOK; }

    *out = lhs != rhs ? (uint64_t)EVAL_BIT : 0x0;

    return LDG_ERR_AOK;
}

static uint8_t eval_cmp_op_is(uint32_t op)
{
    if (op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_LT || op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_GT) { return LDG_TRUTH_TRUE; }

    if (op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_LTE || op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_GTE) { return LDG_TRUTH_TRUE; }

    if (op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_EQ || op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_NEQ) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t eval_shift_op_is(uint32_t op)
{
    if (op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_SHL || op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_SHR) { return LDG_TRUTH_TRUE; }

    if (op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_SHLA || op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_SHRA) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t eval_logic_bin(crosstalk_eda_script_eval_t *ctx, const crosstalk_eda_script_node_t *node, const crosstalk_eda_script_val_t *lhs, crosstalk_eda_script_val_t *out)
{
    crosstalk_eda_script_val_t rhs = LDG_STRUCT_ZERO_INIT;
    uint32_t op = node->op;
    uint32_t ret = 0;

    if (op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_LAND && lhs->val == 0)
    {
        out->kind = (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_SCALAR;
        out->width = (uint32_t)EVAL_BIT;
        out->val = 0x0;

        return LDG_ERR_AOK;
    }

    if (op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_LOR && lhs->val != 0)
    {
        out->kind = (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_SCALAR;
        out->width = (uint32_t)EVAL_BIT;
        out->val = (uint64_t)EVAL_BIT;

        return LDG_ERR_AOK;
    }

    ret = eval_expr(ctx, node->rhs, &rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(rhs.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_SCALAR)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_TYPE, "operator needs a number", "apply operators to numeric values"); }

    out->kind = (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_SCALAR;
    out->width = (uint32_t)EVAL_BIT;

    if (op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_IMPL) { out->val = (lhs->val == 0 || rhs.val != 0) ? (uint64_t)EVAL_BIT : 0x0; }
    else if (op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_BICOND) { out->val = ((lhs->val != 0) == (rhs.val != 0)) ? (uint64_t)EVAL_BIT : 0x0; }
    else { out->val = rhs.val != 0 ? (uint64_t)EVAL_BIT : 0x0; }

    return LDG_ERR_AOK;
}

static uint32_t eval_bin_node(crosstalk_eda_script_eval_t *ctx, const crosstalk_eda_script_node_t *node, crosstalk_eda_script_val_t *out)
{
    crosstalk_eda_script_val_t lhs = LDG_STRUCT_ZERO_INIT;
    crosstalk_eda_script_val_t rhs = LDG_STRUCT_ZERO_INIT;
    uint32_t op = node->op;
    uint32_t ret = eval_expr(ctx, node->lhs, &lhs);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(lhs.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_SCALAR)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_TYPE, "operator needs a number", "apply operators to numeric values"); }

    if (op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_LAND || op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_LOR || op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_IMPL || op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_BICOND) { return eval_logic_bin(ctx, node, &lhs, out); }

    ret = eval_expr(ctx, node->rhs, &rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(rhs.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_SCALAR)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_TYPE, "operator needs a number", "apply operators to numeric values"); }

    if (eval_shift_op_is(op) == LDG_TRUTH_TRUE)
    {
        out->kind = (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_SCALAR;
        out->width = lhs.width;

        return eval_shift_apply(op, lhs.val, rhs.val, lhs.width, &out->val);
    }

    if (LDG_UNLIKELY(lhs.width != rhs.width)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_WIDTH, "operand widths differ", "make both sides the same width"); }

    if (eval_cmp_op_is(op) == LDG_TRUTH_TRUE)
    {
        out->kind = (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_SCALAR;
        out->width = (uint32_t)EVAL_BIT;

        return eval_cmp_apply(op, lhs.val, rhs.val, &out->val);
    }

    out->kind = (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_SCALAR;
    out->width = lhs.width;

    return eval_bin_apply(ctx, node, op, lhs.val, rhs.val, lhs.width, &out->val);
}

static uint32_t eval_assign_node(crosstalk_eda_script_eval_t *ctx, const crosstalk_eda_script_node_t *node, crosstalk_eda_script_val_t *out)
{
    eval_lv_t lv = LDG_STRUCT_ZERO_INIT;
    crosstalk_eda_script_val_t rhs = LDG_STRUCT_ZERO_INIT;
    uint64_t new_val = 0;
    uint32_t ret = eval_lv_resolve(ctx, node->lhs, &lv);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = eval_expr(ctx, node->rhs, &rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(rhs.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_SCALAR)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_TYPE, "assigned value is not a number", "assign a numeric value"); }

    if (node->op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_ASSIGN)
    {
        if (lv.bits_is != LDG_TRUTH_TRUE && LDG_UNLIKELY(rhs.width != lv.slot->width)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_WIDTH, "assigned width does not match the variable", "make the value the same width as the variable"); }

        new_val = rhs.val;
    }
    else
    {
        uint64_t cur = 0;
        uint32_t cur_width = 0;
        uint32_t base_op = (uint32_t)CROSSTALK_EDA_SCRIPT_OP_ADD;

        ret = eval_lv_ld(ctx, node, &lv, &cur, &cur_width);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (node->op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_SHL_ASSIGN) { ret = eval_shift_apply((uint32_t)CROSSTALK_EDA_SCRIPT_OP_SHL, cur, rhs.val, cur_width, &new_val); }
        else if (node->op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_SHR_ASSIGN) { ret = eval_shift_apply((uint32_t)CROSSTALK_EDA_SCRIPT_OP_SHR, cur, rhs.val, cur_width, &new_val); }
        else
        {
            if (node->op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_SUB_ASSIGN) { base_op = (uint32_t)CROSSTALK_EDA_SCRIPT_OP_SUB; }
            else if (node->op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_MUL_ASSIGN) { base_op = (uint32_t)CROSSTALK_EDA_SCRIPT_OP_MUL; }
            else if (node->op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_DIV_ASSIGN) { base_op = (uint32_t)CROSSTALK_EDA_SCRIPT_OP_DIV; }
            else if (node->op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_MOD_ASSIGN) { base_op = (uint32_t)CROSSTALK_EDA_SCRIPT_OP_MOD; }
            else if (node->op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_AND_ASSIGN) { base_op = (uint32_t)CROSSTALK_EDA_SCRIPT_OP_BAND; }
            else if (node->op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_OR_ASSIGN) { base_op = (uint32_t)CROSSTALK_EDA_SCRIPT_OP_BOR; }
            else if (node->op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_XOR_ASSIGN) { base_op = (uint32_t)CROSSTALK_EDA_SCRIPT_OP_BXOR; }

            if (LDG_UNLIKELY(rhs.width != cur_width)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_WIDTH, "operand widths differ", "make both sides the same width"); }

            ret = eval_bin_apply(ctx, node, base_op, cur, rhs.val, cur_width, &new_val);
        }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = eval_lv_st(ctx, node, &lv, new_val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->kind = (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_SCALAR;
    out->width = lv.bits_is == LDG_TRUTH_TRUE ? eval_width_fit(lv.bit_hi - lv.bit_lo + (uint32_t)EVAL_STEP) : lv.slot->width;
    out->val = new_val & eval_mask(out->width);

    return LDG_ERR_AOK;
}

static uint32_t eval_unary_node(crosstalk_eda_script_eval_t *ctx, const crosstalk_eda_script_node_t *node, crosstalk_eda_script_val_t *out)
{
    crosstalk_eda_script_val_t sub = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = eval_expr(ctx, node->lhs, &sub);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(sub.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_SCALAR)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_TYPE, "operator needs a number", "apply operators to numeric values"); }

    out->kind = (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_SCALAR;

    if (node->op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_NEG)
    {
        out->width = sub.width;
        out->val = (~sub.val + (uint64_t)EVAL_BIT) & eval_mask(sub.width);

        return LDG_ERR_AOK;
    }

    if (node->op == (uint32_t)CROSSTALK_EDA_SCRIPT_OP_BNOT)
    {
        out->width = sub.width;
        out->val = (~sub.val) & eval_mask(sub.width);

        return LDG_ERR_AOK;
    }

    out->width = (uint32_t)EVAL_BIT;
    out->val = sub.val == 0 ? (uint64_t)EVAL_BIT : 0x0;

    return LDG_ERR_AOK;
}

static uint32_t eval_concat_node(crosstalk_eda_script_eval_t *ctx, const crosstalk_eda_script_node_t *node, crosstalk_eda_script_val_t *out)
{
    const crosstalk_eda_script_node_t *elem = node->list.hd;
    uint64_t packed = 0;
    uint32_t bits = 0;
    uint32_t ret = 0;

    while (elem != 0x0)
    {
        crosstalk_eda_script_val_t got = LDG_STRUCT_ZERO_INIT;

        ret = eval_expr(ctx, elem, &got);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(got.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_SCALAR)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_TYPE, "concatenation needs numbers", "concatenate numeric values"); }

        if (LDG_UNLIKELY(bits + got.width > (uint32_t)CROSSTALK_EDA_SCRIPT_WIDTH_MAX)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_WIDTH, "concatenation is wider than 64 bits", "keep the total width at or below 64"); }

        packed = (packed << got.width) | (got.val & eval_mask(got.width));
        bits = bits + got.width;
        elem = elem->next;
    }

    out->kind = (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_SCALAR;
    out->width = eval_width_fit(bits);
    out->val = packed;

    return LDG_ERR_AOK;
}

static uint32_t eval_expr(crosstalk_eda_script_eval_t *ctx, const crosstalk_eda_script_node_t *node, crosstalk_eda_script_val_t *out)
{
    uint32_t ret = 0;

    memset(out, 0x0, sizeof(*out));

    if (node->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_NUM)
    {
        out->kind = (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_SCALAR;
        out->width = node->width;
        out->val = node->val;

        return LDG_ERR_AOK;
    }

    if (node->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_STR)
    {
        out->kind = (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_STR;
        out->bytes = (const uint8_t *)node->bytes;
        out->bytes_len = node->bytes_len;
        out->width = node->width;

        return LDG_ERR_AOK;
    }

    if (node->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_IDENT)
    {
        crosstalk_eda_script_slot_t *slot = eval_slot_find(ctx, node->name, node->name_len);

        if (LDG_UNLIKELY(slot == 0x0)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_UNDEF_VAR, "undeclared name", "declare the variable before using it"); }

        if (LDG_UNLIKELY(slot->inited != LDG_TRUTH_TRUE)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_UNDEF_VAR, "variable is used before it holds anything", "run the declaration before this line"); }

        if (slot->arr_is == LDG_TRUTH_TRUE)
        {
            out->kind = (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_STR;
            out->bytes = slot->bytes;
            out->bytes_len = slot->arr_cunt;
            out->width = slot->width;

            return LDG_ERR_AOK;
        }

        out->kind = (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_SCALAR;
        out->width = slot->width;
        out->val = slot->val;

        return LDG_ERR_AOK;
    }

    if (node->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_CALL) { return eval_builtin_call(ctx, node, node->name, node->name_len, &node->list, out); }

    if (node->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_IDX || node->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_SLICE)
    {
        eval_lv_t lv = LDG_STRUCT_ZERO_INIT;
        uint64_t val = 0;
        uint32_t width = 0;

        ret = eval_lv_resolve(ctx, node, &lv);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = eval_lv_ld(ctx, node, &lv, &val, &width);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        out->kind = (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_SCALAR;
        out->width = width;
        out->val = val;

        return LDG_ERR_AOK;
    }

    if (node->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_UN) { return eval_unary_node(ctx, node, out); }

    if (node->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_BIN) { return eval_bin_node(ctx, node, out); }

    if (node->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_ASSIGN) { return eval_assign_node(ctx, node, out); }

    if (node->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_TERNARY)
    {
        crosstalk_eda_script_val_t cond = LDG_STRUCT_ZERO_INIT;

        ret = eval_expr(ctx, node->lhs, &cond);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(cond.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_SCALAR)) { return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_TYPE, "condition is not a number", "use a numeric condition"); }

        return eval_expr(ctx, cond.val != 0 ? node->rhs : node->third, out);
    }

    if (node->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_CONCAT) { return eval_concat_node(ctx, node, out); }

    return eval_diag_set(ctx, node, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "expression form is not supported", "use a supported expression");
}

static uint32_t eval_flatten(crosstalk_eda_script_eval_t *ctx, const crosstalk_eda_script_list_t *list)
{
    crosstalk_eda_script_node_t *stmt = list->hd;
    uint32_t ret = 0;

    while (stmt != 0x0)
    {
        if (stmt->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_SECTION)
        {
            if (LDG_UNLIKELY(eval_section_find(ctx, stmt->name, stmt->name_len) != 0x0)) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_SECTION, "section name is already taken", "give each section its own name"); }

            if (LDG_UNLIKELY(ctx->sections_cunt >= (uint32_t)CROSSTALK_EDA_SCRIPT_SECTION_MAX)) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_FULL, "too many sections", "split the script into more files"); }

            ctx->section[ctx->sections_cunt].name = stmt->name;
            ctx->section[ctx->sections_cunt].name_len = stmt->name_len;
            ctx->section[ctx->sections_cunt].pc = ctx->flat_cunt;
            ctx->sections_cunt = ctx->sections_cunt + (uint32_t)EVAL_STEP;

            ret = eval_flatten(ctx, &stmt->list);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            stmt = stmt->next;

            continue;
        }

        if (stmt->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_RACK)
        {
            ret = eval_rack_reg(ctx, stmt);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            stmt = stmt->next;

            continue;
        }

        if (stmt->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_BLK)
        {
            ret = eval_flatten(ctx, &stmt->list);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            stmt = stmt->next;

            continue;
        }

        if (LDG_UNLIKELY(ctx->flat_cunt >= (uint32_t)CROSSTALK_EDA_SCRIPT_FLAT_MAX)) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_FULL, "script has too many statements", "split the script into more files"); }

        ctx->flat[ctx->flat_cunt] = stmt;
        ctx->flat_cunt = ctx->flat_cunt + (uint32_t)EVAL_STEP;
        stmt = stmt->next;
    }

    return LDG_ERR_AOK;
}

static uint32_t eval_slots_collect(crosstalk_eda_script_eval_t *ctx)
{
    crosstalk_eda_script_slot_t *slot = 0x0;
    uint32_t idx = 0;

    for (idx = 0; idx < ctx->flat_cunt; idx = idx + (uint32_t)EVAL_STEP)
    {
        const crosstalk_eda_script_node_t *stmt = ctx->flat[idx];

        if (stmt->kind != (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_DECL) { continue; }

        if (eval_slot_find(ctx, stmt->name, stmt->name_len) != 0x0) { continue; }

        if (LDG_UNLIKELY(eval_builtin_find(ctx, stmt->name, stmt->name_len) != 0x0)) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_REDECL, "name is already a command", "rename the variable"); }

        if (LDG_UNLIKELY(stmt->name_len >= (uint32_t)CROSSTALK_EDA_SCRIPT_SLOT_NAME_CAP)) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_FULL, "variable name exceeds the name cap", "shorten the variable name"); }

        if (LDG_UNLIKELY(ctx->slots_cunt >= (uint32_t)CROSSTALK_EDA_SCRIPT_SLOT_MAX)) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_FULL, "too many variables in this session", "clear the session or use fewer variables"); }

        slot = &ctx->slot[ctx->slots_cunt];
        memset(slot, 0x0, sizeof(*slot));
        memcpy(slot->name, stmt->name, stmt->name_len);

        slot->name_len = stmt->name_len;
        slot->width = stmt->width;
        slot->const_is = stmt->is_const;
        slot->has_range = stmt->has_range;
        slot->r_lo = stmt->r_lo;
        slot->r_hi = stmt->r_hi;
        slot->r_lo_open = stmt->r_lo_open;
        slot->r_hi_open = stmt->r_hi_open;

        if (stmt->arr_cunt != 0)
        {
            if (LDG_UNLIKELY((uint64_t)stmt->arr_cunt * eval_elem_size(stmt->width) > (uint64_t)CROSSTALK_EDA_SCRIPT_SLOT_BYTES_CAP)) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_FULL, "array does not fit a slot", "declare a smaller array"); }

            slot->arr_is = LDG_TRUTH_TRUE;
            slot->arr_cunt = stmt->arr_cunt;
        }

        ctx->slots_cunt = ctx->slots_cunt + (uint32_t)EVAL_STEP;
    }

    return LDG_ERR_AOK;
}

static uint32_t eval_decl_exec(crosstalk_eda_script_eval_t *ctx, const crosstalk_eda_script_node_t *stmt)
{
    crosstalk_eda_script_slot_t *slot = eval_slot_find(ctx, stmt->name, stmt->name_len);
    crosstalk_eda_script_val_t init = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(slot == 0x0)) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_UNDEF_VAR, "declaration has no storage", "report this as a script engine defect"); }

    if (slot->arr_is == LDG_TRUTH_TRUE)
    {
        if (stmt->bytes != 0x0)
        {
            memcpy(slot->bytes, stmt->bytes, stmt->bytes_len);
            slot->inited = LDG_TRUTH_TRUE;

            return LDG_ERR_AOK;
        }

        if (stmt->rhs != 0x0)
        {
            const crosstalk_eda_script_node_t *elem = stmt->rhs->list.hd;
            uint32_t at = 0;

            while (elem != 0x0)
            {
                ret = eval_expr(ctx, elem, &init);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                if (LDG_UNLIKELY(init.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_SCALAR)) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_TYPE, "array initializer is not a number", "initialize with numeric values"); }

                if (LDG_UNLIKELY(init.width != slot->width)) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_WIDTH, "initializer width does not match the array", "give every element the declared width"); }

                if (LDG_UNLIKELY(eval_range_ok_is(slot, init.val) != LDG_TRUTH_TRUE)) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_RANGE, "initializer is outside the declared range", "keep initializers inside the range annotation"); }

                eval_elem_st(slot, at, init.val);
                at = at + (uint32_t)EVAL_STEP;
                elem = elem->next;
            }

            slot->inited = LDG_TRUTH_TRUE;

            return LDG_ERR_AOK;
        }

        return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "array declaration has no initializer", "initialize the array with a string or a list");
    }

    ret = eval_expr(ctx, stmt->lhs, &init);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(init.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_SCALAR)) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_TYPE, "initializer is not a number", "initialize with a numeric value"); }

    if (LDG_UNLIKELY(init.width != slot->width)) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_WIDTH, "initializer width does not match the variable", "make the initializer the declared width"); }

    if (LDG_UNLIKELY(eval_range_ok_is(slot, init.val) != LDG_TRUTH_TRUE)) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_RANGE, "initializer is outside the declared range", "keep the initializer inside the range annotation"); }

    slot->val = init.val;
    slot->inited = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t eval_guard_fire(crosstalk_eda_script_eval_t *ctx, const crosstalk_eda_script_node_t *stmt, const char *tgt, uint32_t tgt_len, const crosstalk_eda_script_list_t *args, uint8_t args_has, uint32_t *pc, uint8_t *jmp_hit)
{
    const crosstalk_eda_script_section_t *section = 0x0;
    crosstalk_eda_script_val_t res = LDG_STRUCT_ZERO_INIT;

    *jmp_hit = LDG_TRUTH_FALSE;

    if (stmt->action_call == LDG_TRUTH_TRUE) { return eval_builtin_call(ctx, stmt, tgt, tgt_len, args, &res); }

    if (LDG_UNLIKELY(args_has == LDG_TRUTH_TRUE)) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_SECTION, "a jump takes no arguments", "jump to a section by name alone"); }

    section = eval_section_find(ctx, tgt, tgt_len);

    if (LDG_UNLIKELY(section == 0x0)) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_SECTION, "unknown section", "open the section with init name: before jumping to it"); }

    *pc = section->pc;
    *jmp_hit = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t eval_guard_exec(crosstalk_eda_script_eval_t *ctx, const crosstalk_eda_script_node_t *stmt, uint32_t *pc, uint8_t *jmp_hit)
{
    crosstalk_eda_script_val_t cond = LDG_STRUCT_ZERO_INIT;
    uint8_t fire = LDG_TRUTH_TRUE;
    uint32_t ret = 0;

    if (stmt->lhs != 0x0)
    {
        ret = eval_expr(ctx, stmt->lhs, &cond);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(cond.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_SCALAR)) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_TYPE, "guard is not a number", "guard with a numeric expression"); }

        fire = cond.val != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    }

    if (fire == LDG_TRUTH_TRUE) { return eval_guard_fire(ctx, stmt, stmt->name, stmt->name_len, &stmt->list, stmt->has_args, pc, jmp_hit); }

    if (stmt->alt_name == 0x0) { return LDG_ERR_AOK; }

    if (stmt->rhs != 0x0)
    {
        ret = eval_expr(ctx, stmt->rhs, &cond);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(cond.kind != (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_SCALAR)) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_TYPE, "guard is not a number", "guard with a numeric expression"); }

        if (cond.val == 0) { return LDG_ERR_AOK; }
    }

    return eval_guard_fire(ctx, stmt, stmt->alt_name, stmt->alt_name_len, &stmt->alt_list, stmt->alt_has_args, pc, jmp_hit);
}

uint32_t crosstalk_eda_script_eval_init(crosstalk_eda_script_eval_t *ctx, void *user)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->user = user;
    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_script_eval_shutdown(crosstalk_eda_script_eval_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->ok = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_script_eval_slots_clear(crosstalk_eda_script_eval_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->slots_cunt = 0;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_script_eval_builtin_reg(crosstalk_eda_script_eval_t *ctx, const char *name, crosstalk_eda_script_builtin_cb_t cb, uint32_t min_args, uint32_t max_args, uint32_t ret_width)
{
    uint32_t name_len = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0 || cb == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (name[name_len] != LDG_STR_TERM) { name_len = name_len + (uint32_t)EVAL_STEP; }

    if (LDG_UNLIKELY(name_len == 0 || name_len >= (uint32_t)CROSSTALK_EDA_SCRIPT_BUILTIN_NAME_CAP)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(eval_builtin_find(ctx, name, name_len) != 0x0)) { return LDG_ERR_EXISTS; }

    if (LDG_UNLIKELY(ctx->builtins_cunt >= (uint32_t)CROSSTALK_EDA_SCRIPT_BUILTIN_MAX)) { return CROSSTALK_EDA_SCRIPT_ERR_FULL; }

    memset(&ctx->builtin[ctx->builtins_cunt], 0x0, sizeof(ctx->builtin[0]));
    memcpy(ctx->builtin[ctx->builtins_cunt].name, name, name_len);

    ctx->builtin[ctx->builtins_cunt].name_len = name_len;
    ctx->builtin[ctx->builtins_cunt].cb = cb;
    ctx->builtin[ctx->builtins_cunt].min_args = min_args;
    ctx->builtin[ctx->builtins_cunt].max_args = max_args;
    ctx->builtin[ctx->builtins_cunt].ret_width = ret_width;
    ctx->builtins_cunt = ctx->builtins_cunt + (uint32_t)EVAL_STEP;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_script_eval_run(crosstalk_eda_script_eval_t *ctx, crosstalk_eda_script_node_t *program)
{
    uint64_t steps = 0;
    uint32_t pc = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || program == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    memset(&ctx->diag, 0x0, sizeof(ctx->diag));

    ctx->flat_cunt = 0;
    ctx->sections_cunt = 0;

    ret = eval_flatten(ctx, &program->list);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = eval_slots_collect(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    while (pc < ctx->flat_cunt)
    {
        const crosstalk_eda_script_node_t *stmt = ctx->flat[pc];
        crosstalk_eda_script_val_t res = LDG_STRUCT_ZERO_INIT;
        uint8_t jmp_hit = LDG_TRUTH_FALSE;

        steps = steps + (uint64_t)EVAL_STEP;

        if (LDG_UNLIKELY(steps > (uint64_t)CROSSTALK_EDA_SCRIPT_STEP_MAX)) { return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_STEP_MAX, "the script exceeded its step budget", "break the loop; a jump is cycling forever"); }

        if (stmt->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_DECL)
        {
            ret = eval_decl_exec(ctx, stmt);
            if (ret != LDG_ERR_AOK) { return ret; }

            pc = pc + (uint32_t)EVAL_STEP;

            continue;
        }

        if (stmt->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_EXPR_STMT)
        {
            ret = eval_expr(ctx, stmt->lhs, &res);
            if (ret != LDG_ERR_AOK) { return ret; }

            pc = pc + (uint32_t)EVAL_STEP;

            continue;
        }

        if (stmt->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_CALL)
        {
            ret = eval_builtin_call(ctx, stmt, stmt->name, stmt->name_len, &stmt->list, &res);
            if (ret != LDG_ERR_AOK) { return ret; }

            pc = pc + (uint32_t)EVAL_STEP;

            continue;
        }

        if (stmt->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_INC || stmt->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_DEC)
        {
            eval_lv_t lv = LDG_STRUCT_ZERO_INIT;
            uint64_t cur = 0;
            uint32_t cur_width = 0;

            ret = eval_lv_resolve(ctx, stmt->lhs, &lv);
            if (ret != LDG_ERR_AOK) { return ret; }

            ret = eval_lv_ld(ctx, stmt, &lv, &cur, &cur_width);
            if (ret != LDG_ERR_AOK) { return ret; }

            ret = eval_lv_st(ctx, stmt, &lv, stmt->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_INC ? cur + (uint64_t)EVAL_STEP : cur - (uint64_t)EVAL_STEP);
            if (ret != LDG_ERR_AOK) { return ret; }

            pc = pc + (uint32_t)EVAL_STEP;

            continue;
        }

        if (stmt->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_GUARD)
        {
            ret = eval_guard_exec(ctx, stmt, &pc, &jmp_hit);
            if (ret != LDG_ERR_AOK) { return ret; }

            if (jmp_hit == LDG_TRUTH_TRUE) { continue; }

            pc = pc + (uint32_t)EVAL_STEP;

            continue;
        }

        if (stmt->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_NODE_RET) { return LDG_ERR_AOK; }

        return eval_diag_set(ctx, stmt, CROSSTALK_EDA_SCRIPT_ERR_PARSE, "statement form is not supported", "use a supported statement");
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_script_eval_slot_get(const crosstalk_eda_script_eval_t *ctx, const char *name, uint32_t name_len, crosstalk_eda_script_val_t *out)
{
    const crosstalk_eda_script_slot_t *slot = 0x0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(out, 0x0, sizeof(*out));

    for (idx = 0; idx < ctx->slots_cunt; idx = idx + (uint32_t)EVAL_STEP)
    {
        if (ctx->slot[idx].name_len != name_len) { continue; }

        if (memcmp(ctx->slot[idx].name, name, name_len) != 0) { continue; }

        slot = &ctx->slot[idx];

        break;
    }

    if (slot == 0x0) { return LDG_ERR_NOT_FOUND; }

    if (slot->inited != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (slot->arr_is == LDG_TRUTH_TRUE)
    {
        out->kind = (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_STR;
        out->bytes = slot->bytes;
        out->bytes_len = slot->arr_cunt;
        out->width = slot->width;

        return LDG_ERR_AOK;
    }

    out->kind = (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_SCALAR;
    out->width = slot->width;
    out->val = slot->val;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_script_eval_racks_clear(crosstalk_eda_script_eval_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->racks_cunt = 0;
    ctx->switches_cunt = 0;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_script_eval_switch_find(const crosstalk_eda_script_eval_t *ctx, const char *name, uint32_t name_len, const crosstalk_eda_script_switch_t **out)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    for (idx = 0; idx < ctx->switches_cunt; idx = idx + (uint32_t)EVAL_STEP)
    {
        if (ctx->switches[idx].name_len != name_len) { continue; }

        if (memcmp(ctx->switches[idx].name, name, name_len) != 0) { continue; }

        *out = &ctx->switches[idx];

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t crosstalk_eda_script_eval_switch_pos_set(crosstalk_eda_script_eval_t *ctx, const char *name, uint32_t name_len, uint64_t val, const char **out_action, uint32_t *out_action_len)
{
    crosstalk_eda_script_switch_t *switch_ref = 0x0;
    crosstalk_eda_script_slot_t *slot = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0 || out_action == 0x0 || out_action_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_action = 0x0;
    *out_action_len = 0;

    switch_ref = eval_switch_lookup(ctx, name, name_len);
    if (LDG_UNLIKELY(switch_ref == 0x0)) { return LDG_ERR_NOT_FOUND; }

    slot = eval_slot_find(ctx, name, name_len);
    if (LDG_UNLIKELY(slot == 0x0)) { return LDG_ERR_NOT_FOUND; }

    if (switch_ref->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_SWITCH_BUTTON) { slot->val = (uint64_t)EVAL_BIT; }
    else
    {
        if (LDG_UNLIKELY(eval_switch_pos_ok_is(switch_ref, slot, val) != LDG_TRUTH_TRUE)) { return CROSSTALK_EDA_SCRIPT_ERR_RANGE; }

        slot->val = val;
    }

    if (switch_ref->action_len != 0)
    {
        *out_action = switch_ref->action;
        *out_action_len = switch_ref->action_len;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_script_eval_switch_release(crosstalk_eda_script_eval_t *ctx, const char *name, uint32_t name_len)
{
    crosstalk_eda_script_switch_t *switch_ref = 0x0;
    crosstalk_eda_script_slot_t *slot = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    switch_ref = eval_switch_lookup(ctx, name, name_len);
    if (LDG_UNLIKELY(switch_ref == 0x0)) { return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(switch_ref->kind != (uint32_t)CROSSTALK_EDA_SCRIPT_SWITCH_BUTTON)) { return LDG_ERR_FUNC_ARG_INVALID; }

    slot = eval_slot_find(ctx, name, name_len);
    if (LDG_UNLIKELY(slot == 0x0)) { return LDG_ERR_NOT_FOUND; }

    slot->val = 0x0;

    return LDG_ERR_AOK;
}
