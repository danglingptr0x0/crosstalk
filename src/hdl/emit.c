#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>

#include "hdl/emit.h"

#define CROSSTALK_HDL_EMIT_DEPS_PER_OP 4
#define CROSSTALK_HDL_EMIT_SETTLE_MAX 16
#define CROSSTALK_HDL_EMIT_ALIAS_CAP 256
#define CROSSTALK_HDL_EMIT_ALIAS_DEPTH 64
#define CROSSTALK_HDL_EMIT_PF_EXTRA 2
#define CROSSTALK_HDL_EMIT_PF_RING 4096
#define CROSSTALK_HDL_EMIT_PF_NM_BYTES 32
#define CROSSTALK_HDL_EMIT_RGN_OPS 64

static uint64_t crosstalk_hdl_emit_mask_get(uint32_t width)
{
    if (width >= 64) { return ~(uint64_t)0x0; }

    if (width == 0) { return 0x0; }

    return ((uint64_t)0x1 << width) - 0x1;
}

static uint8_t crosstalk_hdl_emit_op_wide_is(const crosstalk_hdl_ir_t *ir_ctx, const crosstalk_hdl_ir_op_t *op)
{
    if (op->dst != UINT32_MAX && ir_ctx->sigs[op->dst].width > 64) { return LDG_TRUTH_TRUE; }

    if (op->src0 != UINT32_MAX && ir_ctx->sigs[op->src0].width > 64) { return LDG_TRUTH_TRUE; }

    if (op->src1 != UINT32_MAX && ir_ctx->sigs[op->src1].width > 64) { return LDG_TRUTH_TRUE; }

    if (op->src2 != UINT32_MAX && ir_ctx->sigs[op->src2].width > 64) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t crosstalk_hdl_bytes_emit(crosstalk_hdl_emit_ctx_t *ctx, const char *ptr, uint32_t wr_len)
{
    if (LDG_UNLIKELY(ctx == 0x0 || ptr == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (wr_len == 0) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(ctx->out_len + wr_len > ctx->out_cap)) { return LDG_ERR_FULL; }

    memcpy(ctx->out + ctx->out_len, ptr, wr_len);
    ctx->out_len = ctx->out_len + wr_len;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_str_emit(crosstalk_hdl_emit_ctx_t *ctx, const char *text)
{
    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_hdl_bytes_emit(ctx, text, (uint32_t)strlen(text));
}

static uint32_t crosstalk_hdl_dec_emit(crosstalk_hdl_emit_ctx_t *ctx, uint64_t val)
{
    char digits[24];
    uint32_t cunt = 0;
    uint32_t idx = 0;
    char swap_ch = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (val == 0)
    {
        digits[0] = 0x30;

        return crosstalk_hdl_bytes_emit(ctx, digits, 0x1);
    }

    while (val != 0)
    {
        digits[cunt] = (char)(0x30 + (val % 10));
        cunt++;
        val = val / 10;
    }

    for (idx = 0; idx < cunt / 0x2; idx++)
    {
        swap_ch = digits[idx];
        digits[idx] = digits[cunt - 0x1 - idx];
        digits[cunt - 0x1 - idx] = swap_ch;
    }

    return crosstalk_hdl_bytes_emit(ctx, digits, cunt);
}

static uint32_t crosstalk_hdl_hex_emit(crosstalk_hdl_emit_ctx_t *ctx, uint64_t val)
{
    char digits[16];
    uint32_t cunt = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;
    char swap_ch = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_str_emit(ctx, "0x");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (val == 0)
    {
        digits[0] = 0x30;

        return crosstalk_hdl_bytes_emit(ctx, digits, 0x1);
    }

    while (val != 0)
    {
        uint64_t nib = val & 0xF;

        if (nib < 10) { digits[cunt] = (char)(0x30 + nib); }
        else { digits[cunt] = (char)(0x41 + nib - 10); }

        cunt++;
        val = val >> 0x4;
    }

    for (idx = 0; idx < cunt / 0x2; idx++)
    {
        swap_ch = digits[idx];
        digits[idx] = digits[cunt - 0x1 - idx];
        digits[cunt - 0x1 - idx] = swap_ch;
    }

    return crosstalk_hdl_bytes_emit(ctx, digits, cunt);
}

static uint32_t crosstalk_hdl_vref_emit(crosstalk_hdl_emit_ctx_t *ctx, uint32_t sig_idx)
{
    uint32_t ret = 0;

    if (sig_idx == UINT32_MAX) { return crosstalk_hdl_str_emit(ctx, "0x0"); }

    ret = crosstalk_hdl_str_emit(ctx, "v[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, sig_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_str_emit(ctx, "]");
}

static uint32_t crosstalk_hdl_fref_emit(crosstalk_hdl_emit_ctx_t *ctx, uint32_t sig_idx);

static uint32_t crosstalk_hdl_sx_ref_emit(crosstalk_hdl_emit_ctx_t *ctx, uint32_t sig_idx, uint32_t width)
{
    uint32_t ret = 0;

    ret = crosstalk_hdl_str_emit(ctx, "((hdl_i64)hdl_gen_sx(");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_fref_emit(ctx, sig_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, ", ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, width);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_str_emit(ctx, "))");
}

static uint32_t crosstalk_hdl_bin_emit(crosstalk_hdl_emit_ctx_t *ctx, const crosstalk_hdl_ir_op_t *op, const char *sep)
{
    uint32_t ret = 0;

    ret = crosstalk_hdl_str_emit(ctx, "(");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_fref_emit(ctx, op->src0);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, sep);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_fref_emit(ctx, op->src1);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_str_emit(ctx, ")");
}

static uint32_t crosstalk_hdl_cmp_emit(crosstalk_hdl_emit_ctx_t *ctx, const crosstalk_hdl_ir_op_t *op, const char *sep)
{
    uint32_t ret = 0;

    ret = crosstalk_hdl_bin_emit(ctx, op, sep);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_str_emit(ctx, " ? 0x1 : 0x0");
}

static uint32_t crosstalk_hdl_scmp_emit(crosstalk_hdl_emit_ctx_t *ctx, const crosstalk_hdl_ir_op_t *op, const char *sep)
{
    uint32_t ret = 0;

    ret = crosstalk_hdl_str_emit(ctx, "(");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_sx_ref_emit(ctx, op->src0, op->width);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, sep);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_sx_ref_emit(ctx, op->src1, op->width);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_str_emit(ctx, ") ? 0x1 : 0x0");
}

static uint32_t crosstalk_hdl_wcall_emit(crosstalk_hdl_emit_ctx_t *ctx, const crosstalk_hdl_ir_op_t *op)
{
    uint32_t ret = 0;

    ret = crosstalk_hdl_str_emit(ctx, "hdl_wop(");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, op->kind);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, ", ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, op->dst);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, ", ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, op->src0);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, ", ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, op->src1);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, ", ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, op->src2);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, ", ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_hex_emit(ctx, op->imm);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, ", ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, op->width);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, ", ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, ctx->ir_ctx->sigs[op->dst].width);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_str_emit(ctx, ")");
}

static uint32_t crosstalk_hdl_wcase_emit(crosstalk_hdl_emit_ctx_t *ctx, uint32_t kind, const char *body)
{
    uint32_t ret = 0;

    ret = crosstalk_hdl_str_emit(ctx, "case ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, kind);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, ":\n{\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, body);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_str_emit(ctx, "break;\n}\n");
}

static uint32_t crosstalk_hdl_expr_emit(crosstalk_hdl_emit_ctx_t *ctx, const crosstalk_hdl_ir_op_t *op)
{
    uint64_t op_mask = 0;
    uint64_t imm_mask = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || op == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    op_mask = crosstalk_hdl_emit_mask_get(op->width);
    imm_mask = crosstalk_hdl_emit_mask_get((uint32_t)op->imm);

    switch (op->kind)
    {
        case CROSSTALK_HDL_IR_OP_CONST:
            return crosstalk_hdl_hex_emit(ctx, op->imm);
        case CROSSTALK_HDL_IR_OP_COPY:
            return crosstalk_hdl_fref_emit(ctx, op->src0);
        case CROSSTALK_HDL_IR_OP_ZEXT:
            return crosstalk_hdl_fref_emit(ctx, op->src0);
        case CROSSTALK_HDL_IR_OP_SEXT:
        {
            ret = crosstalk_hdl_str_emit(ctx, "(hdl_u64)");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            return crosstalk_hdl_sx_ref_emit(ctx, op->src0, (uint32_t)op->imm);
        }
        case CROSSTALK_HDL_IR_OP_AND:
            return crosstalk_hdl_bin_emit(ctx, op, " & ");
        case CROSSTALK_HDL_IR_OP_OR:
            return crosstalk_hdl_bin_emit(ctx, op, " | ");
        case CROSSTALK_HDL_IR_OP_XOR:
            return crosstalk_hdl_bin_emit(ctx, op, " ^ ");
        case CROSSTALK_HDL_IR_OP_XNOR:
        {
            ret = crosstalk_hdl_str_emit(ctx, "~");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            return crosstalk_hdl_bin_emit(ctx, op, " ^ ");
        }
        case CROSSTALK_HDL_IR_OP_NOT:
        {
            ret = crosstalk_hdl_str_emit(ctx, "~");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            return crosstalk_hdl_fref_emit(ctx, op->src0);
        }
        case CROSSTALK_HDL_IR_OP_ADD:
            return crosstalk_hdl_bin_emit(ctx, op, " + ");
        case CROSSTALK_HDL_IR_OP_SUB:
            return crosstalk_hdl_bin_emit(ctx, op, " - ");
        case CROSSTALK_HDL_IR_OP_MUL:
            return crosstalk_hdl_bin_emit(ctx, op, " * ");
        case CROSSTALK_HDL_IR_OP_DIV:
        case CROSSTALK_HDL_IR_OP_MOD:
        {
            ret = crosstalk_hdl_str_emit(ctx, "(");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src1);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " == 0x0 ? 0x0 : ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_bin_emit(ctx, op, op->kind == CROSSTALK_HDL_IR_OP_DIV ? " / " : " % ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            return crosstalk_hdl_str_emit(ctx, ")");
        }
        case CROSSTALK_HDL_IR_OP_NEG:
        {
            ret = crosstalk_hdl_str_emit(ctx, "(~");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src0);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            return crosstalk_hdl_str_emit(ctx, " + 0x1)");
        }
        case CROSSTALK_HDL_IR_OP_SHL:
        case CROSSTALK_HDL_IR_OP_SHR:
        {
            ret = crosstalk_hdl_str_emit(ctx, "(");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src1);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " >= 64 ? 0x0 : ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_bin_emit(ctx, op, op->kind == CROSSTALK_HDL_IR_OP_SHL ? " << " : " >> ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            return crosstalk_hdl_str_emit(ctx, ")");
        }
        case CROSSTALK_HDL_IR_OP_ASHR:
        {
            ret = crosstalk_hdl_str_emit(ctx, "(");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src1);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " >= 64 ? (");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_sx_ref_emit(ctx, op->src0, op->width);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " < 0x0 ? ~(hdl_u64)0x0 : 0x0) : (hdl_u64)(");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_sx_ref_emit(ctx, op->src0, op->width);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " >> ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src1);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            return crosstalk_hdl_str_emit(ctx, "))");
        }
        case CROSSTALK_HDL_IR_OP_EQ:
            return crosstalk_hdl_cmp_emit(ctx, op, " == ");
        case CROSSTALK_HDL_IR_OP_NEQ:
            return crosstalk_hdl_cmp_emit(ctx, op, " != ");
        case CROSSTALK_HDL_IR_OP_LT:
            return crosstalk_hdl_cmp_emit(ctx, op, " < ");
        case CROSSTALK_HDL_IR_OP_LE:
            return crosstalk_hdl_cmp_emit(ctx, op, " <= ");
        case CROSSTALK_HDL_IR_OP_GT:
            return crosstalk_hdl_cmp_emit(ctx, op, " > ");
        case CROSSTALK_HDL_IR_OP_GE:
            return crosstalk_hdl_cmp_emit(ctx, op, " >= ");
        case CROSSTALK_HDL_IR_OP_LT_S:
            return crosstalk_hdl_scmp_emit(ctx, op, " < ");
        case CROSSTALK_HDL_IR_OP_LE_S:
            return crosstalk_hdl_scmp_emit(ctx, op, " <= ");
        case CROSSTALK_HDL_IR_OP_GT_S:
            return crosstalk_hdl_scmp_emit(ctx, op, " > ");
        case CROSSTALK_HDL_IR_OP_GE_S:
            return crosstalk_hdl_scmp_emit(ctx, op, " >= ");
        case CROSSTALK_HDL_IR_OP_LAND:
        case CROSSTALK_HDL_IR_OP_LOR:
        {
            ret = crosstalk_hdl_str_emit(ctx, "(");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src0);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, op->kind == CROSSTALK_HDL_IR_OP_LAND ? " != 0x0 && " : " != 0x0 || ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src1);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            return crosstalk_hdl_str_emit(ctx, " != 0x0 ? 0x1 : 0x0)");
        }
        case CROSSTALK_HDL_IR_OP_LNOT:
        {
            ret = crosstalk_hdl_str_emit(ctx, "(");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src0);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            return crosstalk_hdl_str_emit(ctx, " == 0x0 ? 0x1 : 0x0)");
        }
        case CROSSTALK_HDL_IR_OP_RED_AND:
        {
            ret = crosstalk_hdl_str_emit(ctx, "(");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src0);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " == ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_hex_emit(ctx, imm_mask);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            return crosstalk_hdl_str_emit(ctx, " ? 0x1 : 0x0)");
        }
        case CROSSTALK_HDL_IR_OP_RED_OR:
        {
            ret = crosstalk_hdl_str_emit(ctx, "(");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src0);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            return crosstalk_hdl_str_emit(ctx, " != 0x0 ? 0x1 : 0x0)");
        }
        case CROSSTALK_HDL_IR_OP_RED_XOR:
        {
            ret = crosstalk_hdl_str_emit(ctx, "hdl_gen_rx(");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src0);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            return crosstalk_hdl_str_emit(ctx, ")");
        }
        case CROSSTALK_HDL_IR_OP_CONCAT:
        {
            if (op->imm >= 64)
            {
                ret = crosstalk_hdl_str_emit(ctx, "(");
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                ret = crosstalk_hdl_fref_emit(ctx, op->src1);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                return crosstalk_hdl_str_emit(ctx, " & ~(hdl_u64)0x0)");
            }

            ret = crosstalk_hdl_str_emit(ctx, "((");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src0);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " << ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_dec_emit(ctx, op->imm);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, ") | (");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src1);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " & ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_hex_emit(ctx, imm_mask);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            return crosstalk_hdl_str_emit(ctx, "))");
        }
        case CROSSTALK_HDL_IR_OP_SLICE:
        {
            if (op->imm >= 64) { return crosstalk_hdl_str_emit(ctx, "0x0"); }

            ret = crosstalk_hdl_str_emit(ctx, "((");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src0);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " >> ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_dec_emit(ctx, op->imm);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, ") & ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_hex_emit(ctx, op_mask);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            return crosstalk_hdl_str_emit(ctx, ")");
        }
        case CROSSTALK_HDL_IR_OP_SLICE_DYN:
        {
            ret = crosstalk_hdl_str_emit(ctx, "(");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src1);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " >= 64 ? 0x0 : ((");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src0);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " >> ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src1);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, ") & ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_hex_emit(ctx, op_mask);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            return crosstalk_hdl_str_emit(ctx, "))");
        }
        case CROSSTALK_HDL_IR_OP_INSERT:
        {
            if (op->imm >= 64) { return crosstalk_hdl_fref_emit(ctx, op->src0); }

            ret = crosstalk_hdl_str_emit(ctx, "((");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src0);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " & ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_hex_emit(ctx, ~(op_mask << op->imm));
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, ") | ((");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src1);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " & ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_hex_emit(ctx, op_mask);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, ") << ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_dec_emit(ctx, op->imm);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            return crosstalk_hdl_str_emit(ctx, "))");
        }
        case CROSSTALK_HDL_IR_OP_INSERT_DYN:
        {
            ret = crosstalk_hdl_str_emit(ctx, "(");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src2);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " >= 64 ? ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src0);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " : ((");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src0);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " & ~(");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_hex_emit(ctx, op_mask);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " << ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src2);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, ")) | ((");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src1);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " & ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_hex_emit(ctx, op_mask);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, ") << ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src2);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            return crosstalk_hdl_str_emit(ctx, ")))");
        }
        case CROSSTALK_HDL_IR_OP_MUX:
        {
            ret = crosstalk_hdl_str_emit(ctx, "(");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src0);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " != 0x0 ? ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src1);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " : ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src2);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            return crosstalk_hdl_str_emit(ctx, ")");
        }
        case CROSSTALK_HDL_IR_OP_MEM_RD:
        {
            ret = crosstalk_hdl_str_emit(ctx, "(");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src1);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " < ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_dec_emit(ctx, ctx->ir_ctx->sigs[op->src0].depth);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " ? m[");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_dec_emit(ctx, ctx->mem_off[op->src0]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " + ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_fref_emit(ctx, op->src1);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            return crosstalk_hdl_str_emit(ctx, "] : 0x0)");
        }
        default:
            return CROSSTALK_HDL_ERR_EMIT;
    }
}

static uint32_t crosstalk_hdl_rgn_or_emit(crosstalk_hdl_emit_ctx_t *ctx, uint32_t sig_idx)
{
    uint32_t wj = 0;
    uint32_t ret = 0;

    if (sig_idx == UINT32_MAX || ctx->rgn_rd == 0x0) { return LDG_ERR_AOK; }

    for (wj = 0; wj < ctx->rw_cunt; wj++)
    {
        uint64_t mval = ctx->rgn_rd[(uint64_t)sig_idx * ctx->rw_cunt + wj];

        if (mval == 0) { continue; }

        ret = crosstalk_hdl_str_emit(ctx, ctx->rgn_or_ra != 0x0 ? " ra[" : " ab[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, wj);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "] |= ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_hex_emit(ctx, mval);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, ";");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_rgn_gate_emit(crosstalk_hdl_emit_ctx_t *ctx, uint32_t rgn)
{
    uint32_t ret = 0;

    ret = crosstalk_hdl_str_emit(ctx, "if ((ab[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, rgn / 64);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "] & ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_hex_emit(ctx, (uint64_t)0x1 << (rgn & 63));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, ") != 0x0)\n{\nab[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, rgn / 64);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "] &= ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_hex_emit(ctx, ~((uint64_t)0x1 << (rgn & 63)));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->pf_is == LDG_TRUTH_TRUE) { return crosstalk_hdl_str_emit(ctx, ";\npf_s[7] += 0x1;\n"); }

    return crosstalk_hdl_str_emit(ctx, ";\n");
}

static uint32_t crosstalk_hdl_rgn_close_emit(crosstalk_hdl_emit_ctx_t *ctx, uint32_t rgn, uint8_t union_is)
{
    uint32_t wj = 0;
    uint32_t ret = 0;

    if (union_is == LDG_TRUTH_TRUE && ctx->rgn_un != 0x0)
    {
        for (wj = 0; wj < ctx->rw_cunt; wj++)
        {
            uint64_t mval = ctx->rgn_un[(uint64_t)rgn * ctx->rw_cunt + wj];

            if (mval == 0x0) { continue; }

            ret = crosstalk_hdl_str_emit(ctx, "ab[");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_dec_emit(ctx, wj);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, "] |= ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_hex_emit(ctx, mval);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, ";\n");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }

    return crosstalk_hdl_str_emit(ctx, "}\n");
}

static uint32_t crosstalk_hdl_jrn_gate_emit(crosstalk_hdl_emit_ctx_t *ctx, uint32_t slot)
{
    uint32_t ret = 0;

    ret = crosstalk_hdl_str_emit(ctx, " || (sjb[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, slot / 64);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "] & ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_hex_emit(ctx, (uint64_t)0x1 << (slot & 63));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_str_emit(ctx, ") != 0x0)\n{\n");
}

static uint32_t crosstalk_hdl_jrn_close_emit(crosstalk_hdl_emit_ctx_t *ctx, uint32_t slot)
{
    uint32_t ret = 0;

    ret = crosstalk_hdl_str_emit(ctx, "stg = 0x1;\nif ((sjb[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, slot / 64);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "] & ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_hex_emit(ctx, (uint64_t)0x1 << (slot & 63));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, ") == 0x0) { sjb[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, slot / 64);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "] |= ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_hex_emit(ctx, (uint64_t)0x1 << (slot & 63));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "; sj[sjn] = ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, slot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_str_emit(ctx, "; sjn = sjn + 0x1; }\n}\n");
}

static uint32_t crosstalk_hdl_memwr_body_emit(crosstalk_hdl_emit_ctx_t *ctx, const crosstalk_hdl_ir_op_t *op)
{
    uint32_t ret = 0;

    ret = crosstalk_hdl_fref_emit(ctx, op->src1);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, " < ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, ctx->ir_ctx->sigs[op->src0].depth);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, ") { m[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, ctx->mem_off[op->src0]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, " + ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_fref_emit(ctx, op->src1);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "] = ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_fref_emit(ctx, op->src2);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, " & ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_hex_emit(ctx, crosstalk_hdl_emit_mask_get(ctx->ir_ctx->sigs[op->src0].width));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_str_emit(ctx, "; }\n");
}

static uint32_t crosstalk_hdl_wmem_idx_emit(crosstalk_hdl_emit_ctx_t *ctx, const crosstalk_hdl_ir_op_t *op, const char *arena)
{
    uint32_t ret = 0;

    ret = crosstalk_hdl_str_emit(ctx, arena);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, ctx->mem_off[op->src0]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, " + ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_fref_emit(ctx, op->src1);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_str_emit(ctx, "]");
}

static uint32_t crosstalk_hdl_wbound_emit(crosstalk_hdl_emit_ctx_t *ctx, const crosstalk_hdl_ir_op_t *op)
{
    uint32_t ret = 0;

    ret = crosstalk_hdl_fref_emit(ctx, op->src1);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, " < ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_dec_emit(ctx, ctx->ir_ctx->sigs[op->src0].depth);
}

static uint32_t crosstalk_hdl_wstmt_emit(crosstalk_hdl_emit_ctx_t *ctx, const crosstalk_hdl_ir_op_t *op)
{
    uint64_t mem_mask = 0;
    uint64_t mem_mask_hi = 0;
    uint32_t mem_w = 0;
    uint32_t ret = 0;

    if (op->kind == CROSSTALK_HDL_IR_OP_MEM_WR)
    {
        mem_w = ctx->ir_ctx->sigs[op->src0].width;
        mem_mask = crosstalk_hdl_emit_mask_get(mem_w);
        mem_mask_hi = mem_w <= 64 ? 0x0 : crosstalk_hdl_emit_mask_get(mem_w - 64);

        ret = crosstalk_hdl_str_emit(ctx, "if (");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (op->dst != UINT32_MAX)
        {
            ret = crosstalk_hdl_fref_emit(ctx, op->dst);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " != 0x0 && ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_wbound_emit(ctx, op);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, ") { nx = ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_fref_emit(ctx, op->src2);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, " & ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_hex_emit(ctx, mem_mask);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "; nxh = vh[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, op->src2);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "] & ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_hex_emit(ctx, mem_mask_hi);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "; if (");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wmem_idx_emit(ctx, op, "m");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, " != nx || ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wmem_idx_emit(ctx, op, "mh");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, " != nxh) { ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wmem_idx_emit(ctx, op, "m");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, " = nx; ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wmem_idx_emit(ctx, op, "mh");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (ctx->rgn_chk != 0x0)
        {
            ret = crosstalk_hdl_str_emit(ctx, " = nxh;");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_rgn_or_emit(ctx, op->src0);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            return crosstalk_hdl_str_emit(ctx, " } }\n");
        }

        return crosstalk_hdl_str_emit(ctx, " = nxh; } }\n");
    }

    if (op->kind == CROSSTALK_HDL_IR_OP_MEM_RD)
    {
        ret = crosstalk_hdl_str_emit(ctx, "nx = (");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wbound_emit(ctx, op);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, ") ? ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wmem_idx_emit(ctx, op, "m");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, " : 0x0; nxh = (");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wbound_emit(ctx, op);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, ") ? ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wmem_idx_emit(ctx, op, "mh");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, " : 0x0; if (");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_vref_emit(ctx, op->dst);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, " != nx || vh[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, op->dst);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "] != nxh) { ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_vref_emit(ctx, op->dst);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, " = nx; vh[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, op->dst);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (ctx->rgn_chk != 0x0)
        {
            ret = crosstalk_hdl_str_emit(ctx, "] = nxh;");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_rgn_or_emit(ctx, op->dst);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            return crosstalk_hdl_str_emit(ctx, " }\n");
        }

        return crosstalk_hdl_str_emit(ctx, "] = nxh; }\n");
    }

    if (ctx->rgn_chk != 0x0)
    {
        ret = crosstalk_hdl_str_emit(ctx, "if (");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcall_emit(ctx, op);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, " != 0x0) {");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_rgn_or_emit(ctx, op->dst);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        return crosstalk_hdl_str_emit(ctx, " }\n");
    }

    ret = crosstalk_hdl_wcall_emit(ctx, op);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_str_emit(ctx, ";\n");
}

static uint8_t crosstalk_hdl_emit_mask_skip_is(const crosstalk_hdl_ir_t *ir_src, const crosstalk_hdl_ir_op_t *op)
{
    uint32_t dst_w = 0;

    if (op->dst == UINT32_MAX) { return LDG_TRUTH_FALSE; }

    dst_w = ir_src->sigs[op->dst].width;

    switch (op->kind)
    {
        case CROSSTALK_HDL_IR_OP_EQ:
        case CROSSTALK_HDL_IR_OP_NEQ:
        case CROSSTALK_HDL_IR_OP_LT:
        case CROSSTALK_HDL_IR_OP_LE:
        case CROSSTALK_HDL_IR_OP_GT:
        case CROSSTALK_HDL_IR_OP_GE:
        case CROSSTALK_HDL_IR_OP_LT_S:
        case CROSSTALK_HDL_IR_OP_LE_S:
        case CROSSTALK_HDL_IR_OP_GT_S:
        case CROSSTALK_HDL_IR_OP_GE_S:
        case CROSSTALK_HDL_IR_OP_LAND:
        case CROSSTALK_HDL_IR_OP_LOR:
        case CROSSTALK_HDL_IR_OP_LNOT:
        case CROSSTALK_HDL_IR_OP_RED_AND:
        case CROSSTALK_HDL_IR_OP_RED_OR:
        case CROSSTALK_HDL_IR_OP_RED_XOR:
            return dst_w >= 0x1 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
        case CROSSTALK_HDL_IR_OP_AND:
        case CROSSTALK_HDL_IR_OP_OR:
        case CROSSTALK_HDL_IR_OP_XOR:
        {
            if (op->src0 == UINT32_MAX || op->src1 == UINT32_MAX) { return LDG_TRUTH_FALSE; }

            if (ir_src->sigs[op->src0].width <= dst_w && ir_src->sigs[op->src1].width <= dst_w) { return LDG_TRUTH_TRUE; }

            return LDG_TRUTH_FALSE;
        }
        case CROSSTALK_HDL_IR_OP_COPY:
        case CROSSTALK_HDL_IR_OP_ZEXT:
        {
            if (op->src0 == UINT32_MAX) { return LDG_TRUTH_FALSE; }

            if (ir_src->sigs[op->src0].width <= dst_w) { return LDG_TRUTH_TRUE; }

            return LDG_TRUTH_FALSE;
        }
        case CROSSTALK_HDL_IR_OP_SLICE:
        case CROSSTALK_HDL_IR_OP_SLICE_DYN:
            return op->width <= dst_w ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
        case CROSSTALK_HDL_IR_OP_MUX:
        {
            if (op->src1 == UINT32_MAX || op->src2 == UINT32_MAX) { return LDG_TRUTH_FALSE; }

            if (ir_src->sigs[op->src1].width <= dst_w && ir_src->sigs[op->src2].width <= dst_w) { return LDG_TRUTH_TRUE; }

            return LDG_TRUTH_FALSE;
        }
        default:
            return LDG_TRUTH_FALSE;
    }
}

static uint32_t crosstalk_hdl_fref_emit(crosstalk_hdl_emit_ctx_t *ctx, uint32_t sig_idx)
{
    const crosstalk_hdl_ir_op_t *fop = 0x0;
    uint32_t ret = 0;

    if (sig_idx == UINT32_MAX) { return crosstalk_hdl_str_emit(ctx, "0x0"); }

    if (ctx->vf_op != UINT32_MAX && sig_idx == ctx->vf_sig)
    {
        const crosstalk_hdl_ir_op_t *pop = &ctx->ir_ctx->comb_ops[ctx->vf_op];
        uint32_t pidx = ctx->vf_op;

        ctx->vf_op = ctx->vf_prev[pidx];

        ret = crosstalk_hdl_str_emit(ctx, "((hdl_u64)(");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->vf_op = pidx; return ret; }

        ret = crosstalk_hdl_expr_emit(ctx, pop);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->vf_op = pidx; return ret; }

        if (crosstalk_hdl_emit_mask_skip_is(ctx->ir_ctx, pop) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_str_emit(ctx, "))");
            ctx->vf_op = pidx;
            return ret;
        }

        ret = crosstalk_hdl_str_emit(ctx, ") & ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->vf_op = pidx; return ret; }

        ret = crosstalk_hdl_hex_emit(ctx, crosstalk_hdl_emit_mask_get(ctx->ir_ctx->sigs[pop->dst].width));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->vf_op = pidx; return ret; }

        ret = crosstalk_hdl_str_emit(ctx, ")");
        ctx->vf_op = pidx;
        return ret;
    }

    if (ctx->loc_act != 0x0 && ctx->loc_act[sig_idx] != UINT32_MAX)
    {
        uint32_t lv = ctx->loc_act[sig_idx];

        if (lv >= ctx->ir_ctx->comb_cunt)
        {
            ret = crosstalk_hdl_str_emit(ctx, "ts");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            return crosstalk_hdl_dec_emit(ctx, lv - ctx->ir_ctx->comb_cunt);
        }

        ret = crosstalk_hdl_str_emit(ctx, "tl");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        return crosstalk_hdl_dec_emit(ctx, lv);
    }

    if (ctx->cst_has != 0x0 && ctx->cst_has[sig_idx] != 0x0)
    {
        ret = crosstalk_hdl_str_emit(ctx, "((hdl_u64)");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_hex_emit(ctx, ctx->cst_val[sig_idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        return crosstalk_hdl_str_emit(ctx, ")");
    }

    if (ctx->fuse_map != 0x0 && ctx->fuse_map[sig_idx] != UINT32_MAX) { fop = &ctx->ir_ctx->comb_ops[ctx->fuse_map[sig_idx]]; }
    else if (ctx->fuse_map_seq != 0x0 && ctx->fuse_map_seq[sig_idx] != UINT32_MAX) { fop = &ctx->ir_ctx->seq_ops[ctx->fuse_map_seq[sig_idx]]; }
    else { return crosstalk_hdl_vref_emit(ctx, sig_idx); }

    if (crosstalk_hdl_emit_mask_skip_is(ctx->ir_ctx, fop) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_str_emit(ctx, "((hdl_u64)(");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_expr_emit(ctx, fop);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        return crosstalk_hdl_str_emit(ctx, "))");
    }

    ret = crosstalk_hdl_str_emit(ctx, "((hdl_u64)(");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_expr_emit(ctx, fop);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, ") & ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_hex_emit(ctx, crosstalk_hdl_emit_mask_get(ctx->ir_ctx->sigs[fop->dst].width));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_str_emit(ctx, ")");
}

static uint32_t crosstalk_hdl_comb_stmt_emit(crosstalk_hdl_emit_ctx_t *ctx, const crosstalk_hdl_ir_op_t *op)
{
    uint64_t dst_mask = 0;
    uint32_t ret = 0;

    if (crosstalk_hdl_emit_op_wide_is(ctx->ir_ctx, op) == LDG_TRUTH_TRUE) { return crosstalk_hdl_wstmt_emit(ctx, op); }

    if (op->kind == CROSSTALK_HDL_IR_OP_MEM_WR)
    {
        ret = crosstalk_hdl_str_emit(ctx, "if (");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (op->dst != UINT32_MAX)
        {
            ret = crosstalk_hdl_fref_emit(ctx, op->dst);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " != 0x0 && ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        if (ctx->rgn_chk == 0x0) { return crosstalk_hdl_memwr_body_emit(ctx, op); }

        ret = crosstalk_hdl_wbound_emit(ctx, op);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, ")\n{\nnx = ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_fref_emit(ctx, op->src2);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, " & ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_hex_emit(ctx, crosstalk_hdl_emit_mask_get(ctx->ir_ctx->sigs[op->src0].width));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, ";\nif (");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wmem_idx_emit(ctx, op, "m");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, " != nx) { ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wmem_idx_emit(ctx, op, "m");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, " = nx;");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_rgn_or_emit(ctx, op->src0);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        return crosstalk_hdl_str_emit(ctx, " }\n}\n");
    }

    dst_mask = crosstalk_hdl_emit_mask_get(ctx->ir_ctx->sigs[op->dst].width);

    if (ctx->rgn_chk != 0x0)
    {
        ret = crosstalk_hdl_str_emit(ctx, "nx = (");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_expr_emit(ctx, op);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (crosstalk_hdl_emit_mask_skip_is(ctx->ir_ctx, op) == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_str_emit(ctx, ")"); }
        else
        {
            ret = crosstalk_hdl_str_emit(ctx, ") & ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_hex_emit(ctx, dst_mask);
        }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "; if (");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_vref_emit(ctx, op->dst);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, " != nx) { ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_vref_emit(ctx, op->dst);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, " = nx;");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_rgn_or_emit(ctx, op->dst);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        return crosstalk_hdl_str_emit(ctx, " }\n");
    }

    ret = crosstalk_hdl_vref_emit(ctx, op->dst);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, " = (");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_expr_emit(ctx, op);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (crosstalk_hdl_emit_mask_skip_is(ctx->ir_ctx, op) == LDG_TRUTH_TRUE) { return crosstalk_hdl_str_emit(ctx, ");\n"); }

    ret = crosstalk_hdl_str_emit(ctx, ") & ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_hex_emit(ctx, dst_mask);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_str_emit(ctx, ";\n");
}

static uint32_t crosstalk_hdl_seq_stmt_emit(crosstalk_hdl_emit_ctx_t *ctx, const crosstalk_hdl_ir_op_t *op, uint32_t op_idx)
{
    uint32_t ret = 0;

    if (op->kind == CROSSTALK_HDL_IR_OP_REG_NEXT)
    {
        uint32_t rn_last = 0x1;
        uint32_t rn_scan = 0;

        for (rn_scan = op_idx + 0x1; rn_scan < ctx->ir_ctx->seq_op_cunt; rn_scan++) { if (ctx->ir_ctx->seq_ops[rn_scan].kind == CROSSTALK_HDL_IR_OP_REG_NEXT && ctx->ir_ctx->seq_ops[rn_scan].dst == op->dst) { rn_last = 0x0; break; } }

        ret = crosstalk_hdl_str_emit(ctx, "nx = ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_fref_emit(ctx, op->src0);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (ctx->rn_direct[op_idx] == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_str_emit(ctx, " & ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_hex_emit(ctx, crosstalk_hdl_emit_mask_get(ctx->ir_ctx->sigs[op->dst].width));
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, ";\nif (");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_vref_emit(ctx, op->dst);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " != nx)\n{\n");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_vref_emit(ctx, op->dst);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " = nx;\ncmt = 0x1;\nstg = 0x1;\n");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (ctx->pf_is == LDG_TRUTH_TRUE)
            {
                ret = crosstalk_hdl_str_emit(ctx, "pf_s[5] += 0x1;\n");
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            ret = crosstalk_hdl_str_emit(ctx, "hdl_rgn_sig(");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_dec_emit(ctx, op->dst);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            return crosstalk_hdl_str_emit(ctx, ");\n}\n");
        }

        if (rn_last == 0x0)
        {
            ret = crosstalk_hdl_str_emit(ctx, ";\nrn[");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_dec_emit(ctx, ctx->rn_slot[op_idx]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, "] = nx;\n");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (crosstalk_hdl_emit_op_wide_is(ctx->ir_ctx, op) == LDG_TRUTH_TRUE)
            {
                ret = crosstalk_hdl_str_emit(ctx, "rnh[");
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                ret = crosstalk_hdl_dec_emit(ctx, ctx->rn_slot[op_idx]);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                ret = crosstalk_hdl_str_emit(ctx, "] = vh[");
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                ret = crosstalk_hdl_dec_emit(ctx, op->src0);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                ret = crosstalk_hdl_str_emit(ctx, "];\n");
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            ret = crosstalk_hdl_str_emit(ctx, "{\n");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            return crosstalk_hdl_jrn_close_emit(ctx, ctx->rn_slot[op_idx]);
        }

        if (crosstalk_hdl_emit_op_wide_is(ctx->ir_ctx, op) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_str_emit(ctx, ";\nnxh = vh[");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_dec_emit(ctx, op->src0);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, "];\nif (");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_vref_emit(ctx, op->dst);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " != nx || vh[");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_dec_emit(ctx, op->dst);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, "] != nxh");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
        else
        {
            ret = crosstalk_hdl_str_emit(ctx, ";\nif (");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_vref_emit(ctx, op->dst);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " != nx");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_jrn_gate_emit(ctx, ctx->rn_slot[op_idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "rn[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, ctx->rn_slot[op_idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "] = nx;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (crosstalk_hdl_emit_op_wide_is(ctx->ir_ctx, op) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_str_emit(ctx, "rnh[");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_dec_emit(ctx, ctx->rn_slot[op_idx]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, "] = nxh;\n");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        return crosstalk_hdl_jrn_close_emit(ctx, ctx->rn_slot[op_idx]);
    }

    if (op->kind == CROSSTALK_HDL_IR_OP_MEM_WR)
    {
        uint32_t slot = ctx->mw_slot[op_idx];

        ret = crosstalk_hdl_str_emit(ctx, "mwe[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, slot);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "] = ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (op->dst == UINT32_MAX) { ret = crosstalk_hdl_str_emit(ctx, "0x1"); }
        else { ret = crosstalk_hdl_fref_emit(ctx, op->dst); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "; if (mwe[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, slot);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "] != 0x0) { mwa[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, slot);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "] = ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_fref_emit(ctx, op->src1);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "; mwv[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, slot);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "] = ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_fref_emit(ctx, op->src2);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (crosstalk_hdl_emit_op_wide_is(ctx->ir_ctx, op) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_str_emit(ctx, "; mwvh[");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_dec_emit(ctx, slot);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, "] = vh[");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_dec_emit(ctx, op->src2);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, "]");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_str_emit(ctx, ";\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        return crosstalk_hdl_jrn_close_emit(ctx, ctx->rn_cunt + slot);
    }

    return crosstalk_hdl_comb_stmt_emit(ctx, op);
}

static uint32_t crosstalk_hdl_loc_stmt_emit(crosstalk_hdl_emit_ctx_t *ctx, const crosstalk_hdl_ir_op_t *op, uint32_t op_idx, const char *nm)
{
    uint32_t ret = 0;

    ret = crosstalk_hdl_str_emit(ctx, nm);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, op_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, " = (");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_expr_emit(ctx, op);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (crosstalk_hdl_emit_mask_skip_is(ctx->ir_ctx, op) == LDG_TRUTH_TRUE) { return crosstalk_hdl_str_emit(ctx, ");\n"); }

    ret = crosstalk_hdl_str_emit(ctx, ") & ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_hex_emit(ctx, crosstalk_hdl_emit_mask_get(ctx->ir_ctx->sigs[op->dst].width));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_str_emit(ctx, ";\n");
}

static uint32_t crosstalk_hdl_emit_wsig_get(const crosstalk_hdl_ir_op_t *op)
{
    if (op->kind == CROSSTALK_HDL_IR_OP_MEM_WR) { return op->src0; }

    return op->dst;
}

static uint32_t crosstalk_hdl_emit_rsigs_get(const crosstalk_hdl_ir_op_t *op, uint32_t *out_arr)
{
    uint32_t cunt = 0;

    if (op->kind == CROSSTALK_HDL_IR_OP_MEM_WR)
    {
        if (op->dst != UINT32_MAX) { out_arr[cunt] = op->dst; cunt++; }

        if (op->src1 != UINT32_MAX) { out_arr[cunt] = op->src1; cunt++; }

        if (op->src2 != UINT32_MAX) { out_arr[cunt] = op->src2; cunt++; }

        return cunt;
    }

    if (op->kind == CROSSTALK_HDL_IR_OP_CONST) { return cunt; }

    if (op->src0 != UINT32_MAX) { out_arr[cunt] = op->src0; cunt++; }

    switch (op->kind)
    {
        case CROSSTALK_HDL_IR_OP_COPY:
        case CROSSTALK_HDL_IR_OP_ZEXT:
        case CROSSTALK_HDL_IR_OP_SEXT:
        case CROSSTALK_HDL_IR_OP_NOT:
        case CROSSTALK_HDL_IR_OP_NEG:
        case CROSSTALK_HDL_IR_OP_LNOT:
        case CROSSTALK_HDL_IR_OP_RED_AND:
        case CROSSTALK_HDL_IR_OP_RED_OR:
        case CROSSTALK_HDL_IR_OP_RED_XOR:
        case CROSSTALK_HDL_IR_OP_SLICE:
            return cunt;
        default:
            break;
    }

    if (op->src1 != UINT32_MAX) { out_arr[cunt] = op->src1; cunt++; }

    if (op->kind == CROSSTALK_HDL_IR_OP_INSERT) { return cunt; }

    if (op->kind != CROSSTALK_HDL_IR_OP_MEM_RD && op->src2 != UINT32_MAX) { out_arr[cunt] = op->src2; cunt++; }

    return cunt;
}

static uint32_t crosstalk_hdl_emit_topo_build(crosstalk_hdl_emit_ctx_t *ctx)
{
    const crosstalk_hdl_ir_t *ir_ctx = ctx->ir_ctx;
    uint32_t *last_wr = 0x0;
    uint32_t *run_wr = 0x0;
    void *raw = 0x0;
    uint32_t rd_arr[4];
    uint32_t idx = 0;
    uint32_t dep_idx = 0;
    uint32_t rd_cunt = 0;
    uint32_t ws = 0;
    uint32_t progress = 0;
    uint32_t ret = 0;

    ret = ldg_mem_pool_alloc(ctx->pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt) * sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    last_wr = (uint32_t *)raw;

    ret = ldg_mem_pool_alloc(ctx->pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt) * sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    run_wr = (uint32_t *)raw;

    for (idx = 0; idx < ir_ctx->sig_cunt; idx++) { last_wr[idx] = UINT32_MAX; run_wr[idx] = UINT32_MAX; }

    for (idx = 0; idx < ir_ctx->comb_cunt; idx++)
    {
        ws = crosstalk_hdl_emit_wsig_get(&ir_ctx->comb_ops[idx]);
        ctx->deps[idx * CROSSTALK_HDL_EMIT_DEPS_PER_OP] = ws == UINT32_MAX ? UINT32_MAX : last_wr[ws];

        if (ws != UINT32_MAX) { last_wr[ws] = idx; }
    }

    for (idx = 0; idx < ir_ctx->comb_cunt; idx++)
    {
        uint32_t fill = 0x1;

        ws = crosstalk_hdl_emit_wsig_get(&ir_ctx->comb_ops[idx]);
        rd_cunt = crosstalk_hdl_emit_rsigs_get(&ir_ctx->comb_ops[idx], rd_arr);

        for (dep_idx = 0; dep_idx < rd_cunt; dep_idx++)
        {
            uint32_t dep_op = 0;

            if (rd_arr[dep_idx] == ws) { continue; }

            dep_op = run_wr[rd_arr[dep_idx]];

            if (dep_op == UINT32_MAX) { dep_op = last_wr[rd_arr[dep_idx]]; }

            if (dep_op == UINT32_MAX || dep_op == idx) { continue; }

            ctx->deps[idx * CROSSTALK_HDL_EMIT_DEPS_PER_OP + fill] = dep_op;
            fill++;
        }

        while (fill < CROSSTALK_HDL_EMIT_DEPS_PER_OP)
        {
            ctx->deps[idx * CROSSTALK_HDL_EMIT_DEPS_PER_OP + fill] = UINT32_MAX;
            fill++;
        }

        if (ws != UINT32_MAX) { run_wr[ws] = idx; }
    }

    for (idx = 0; idx < ir_ctx->comb_cunt; idx++) { ctx->placed[idx] = LDG_TRUTH_FALSE; }

    ctx->order_cunt = 0;
    progress = 0x1;

    while (progress != 0)
    {
        progress = 0;

        for (idx = 0; idx < ir_ctx->comb_cunt; idx++)
        {
            uint32_t ready = 0x1;

            if (ctx->placed[idx] == LDG_TRUTH_TRUE) { continue; }

            for (dep_idx = 0; dep_idx < CROSSTALK_HDL_EMIT_DEPS_PER_OP; dep_idx++)
            {
                uint32_t dep_op = ctx->deps[idx * CROSSTALK_HDL_EMIT_DEPS_PER_OP + dep_idx];

                if (dep_op == UINT32_MAX) { continue; }

                if (ctx->placed[dep_op] != LDG_TRUTH_TRUE) { ready = 0; break; }
            }

            if (ready != 0)
            {
                ctx->order[ctx->order_cunt] = idx;
                ctx->order_cunt++;
                ctx->placed[idx] = LDG_TRUTH_TRUE;
                progress = 0x1;
            }
        }
    }

    ctx->residue_cunt = 0;

    for (;;)
    {
        uint32_t forced = UINT32_MAX;

        for (idx = 0; idx < ir_ctx->comb_cunt; idx++) { if (ctx->placed[idx] != LDG_TRUTH_TRUE) { forced = idx; break; } }

        if (forced == UINT32_MAX) { break; }

        ctx->residue[ctx->residue_cunt] = forced;
        ctx->residue_cunt++;
        ctx->placed[forced] = LDG_TRUTH_TRUE;

        progress = 0x1;

        while (progress != 0)
        {
            progress = 0;

            for (idx = 0; idx < ir_ctx->comb_cunt; idx++)
            {
                uint32_t ready = 0x1;

                if (ctx->placed[idx] == LDG_TRUTH_TRUE) { continue; }

                for (dep_idx = 0; dep_idx < CROSSTALK_HDL_EMIT_DEPS_PER_OP; dep_idx++)
                {
                    uint32_t dep_op = ctx->deps[idx * CROSSTALK_HDL_EMIT_DEPS_PER_OP + dep_idx];

                    if (dep_op == UINT32_MAX) { continue; }

                    if (ctx->placed[dep_op] != LDG_TRUTH_TRUE) { ready = 0; break; }
                }

                if (ready != 0)
                {
                    ctx->residue[ctx->residue_cunt] = idx;
                    ctx->residue_cunt++;
                    ctx->placed[idx] = LDG_TRUTH_TRUE;
                    progress = 0x1;
                }
            }
        }
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_scan_blk_emit(crosstalk_hdl_emit_ctx_t *ctx, uint32_t blk_idx, uint8_t second_is)
{
    const crosstalk_hdl_ir_seq_blk_t *blk = &ctx->ir_ctx->seq_blks[blk_idx];
    uint32_t ret = 0;

    ret = crosstalk_hdl_str_emit(ctx, "cv = ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (blk->clk_sig == UINT32_MAX) { ret = crosstalk_hdl_str_emit(ctx, "0x0"); }
    else { ret = crosstalk_hdl_vref_emit(ctx, blk->clk_sig); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "; rv = ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (blk->rst_sig == UINT32_MAX) { ret = crosstalk_hdl_str_emit(ctx, "0x0"); }
    else { ret = crosstalk_hdl_vref_emit(ctx, blk->rst_sig); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "; f = 0x0;\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (blk->clk_sig != UINT32_MAX && (second_is != LDG_TRUTH_TRUE || blk->clk_edge == CROSSTALK_HDL_IR_EDGE_NEG))
    {
        ret = crosstalk_hdl_str_emit(ctx, "if (pc[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, blk_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (blk->clk_edge == CROSSTALK_HDL_IR_EDGE_POS) { ret = crosstalk_hdl_str_emit(ctx, "] == 0x0 && cv != 0x0) { f = 0x1; }\n"); }
        else { ret = crosstalk_hdl_str_emit(ctx, "] != 0x0 && cv == 0x0) { f = 0x1; }\n"); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (blk->rst_sig != UINT32_MAX)
    {
        ret = crosstalk_hdl_str_emit(ctx, "if (pr[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, blk_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (blk->rst_edge == CROSSTALK_HDL_IR_EDGE_POS) { ret = crosstalk_hdl_str_emit(ctx, "] == 0x0 && rv != 0x0) { f = 0x1; }\n"); }
        else { ret = crosstalk_hdl_str_emit(ctx, "] != 0x0 && rv == 0x0) { f = 0x1; }\n"); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (ctx->pf_is == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_str_emit(ctx, "if (f != 0x0) { hdl_u64 pfb; fired[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, blk_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "] = 0x1; pfb = hdl_pf_now(); hdl_gen_b");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, blk_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "(); pfb = hdl_pf_now() - pfb; pf_t[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, blk_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "] += pfb; pf_c[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, blk_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "] += 0x1; pf_cur_blk += pfb; }\npc[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    else
    {
        ret = crosstalk_hdl_str_emit(ctx, "if (f != 0x0) { fired[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, blk_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "] = 0x1; hdl_gen_b");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, blk_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "(); }\npc[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_dec_emit(ctx, blk_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "] = cv; pr[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, blk_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_str_emit(ctx, "] = rv;\n");
}

static uint8_t crosstalk_hdl_emit_blk_direct_is(const crosstalk_hdl_ir_t *ir_ctx, uint32_t blk_idx, uint32_t fast_ok)
{
    const crosstalk_hdl_ir_seq_blk_t *blk = &ir_ctx->seq_blks[blk_idx];

    if (fast_ok == 0x0) { return LDG_TRUTH_FALSE; }

    if (blk->clk_sig == UINT32_MAX || blk->rst_sig != UINT32_MAX) { return LDG_TRUTH_FALSE; }

    if (blk->clk_edge != CROSSTALK_HDL_IR_EDGE_POS) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static uint8_t crosstalk_hdl_emit_blk_guard_get(const crosstalk_hdl_emit_ctx_t *ctx, uint32_t blk_idx, uint32_t *out_guards, uint32_t *out_cunt)
{
    const crosstalk_hdl_ir_t *ir_ctx = ctx->ir_ctx;
    const crosstalk_hdl_ir_seq_blk_t *blk = &ir_ctx->seq_blks[blk_idx];
    uint32_t op_at = 0;
    uint32_t g_idx = 0;

    *out_cunt = 0;

    for (op_at = blk->op_start; op_at < blk->op_start + blk->op_cunt; op_at = op_at + 0x1)
    {
        const crosstalk_hdl_ir_op_t *op = &ir_ctx->seq_ops[op_at];
        uint32_t guard_sig = UINT32_MAX;
        uint8_t seen = LDG_TRUTH_FALSE;

        if (op->kind == CROSSTALK_HDL_IR_OP_MEM_WR)
        {
            if (op->guard_is != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

            guard_sig = op->guard;
        }
        else if (op->kind == CROSSTALK_HDL_IR_OP_REG_NEXT)
        {
            uint32_t chase_sig = op->src0;
            uint32_t chase_at = op_at;
            uint32_t hop = 0;

            if (chase_sig == UINT32_MAX) { return LDG_TRUTH_FALSE; }

            for (hop = 0; hop < blk->op_cunt; hop = hop + 0x1)
            {
                const crosstalk_hdl_ir_op_t *back_op = 0x0;
                uint32_t back_at = chase_at;
                uint8_t found = LDG_TRUTH_FALSE;

                while (back_at > blk->op_start && found == LDG_TRUTH_FALSE)
                {
                    back_at = back_at - 0x1;
                    back_op = &ir_ctx->seq_ops[back_at];
                    if (back_op->kind == CROSSTALK_HDL_IR_OP_REG_NEXT || back_op->kind == CROSSTALK_HDL_IR_OP_MEM_WR) { continue; }

                    if (back_op->dst == chase_sig) { found = LDG_TRUTH_TRUE; }
                }

                if (found == LDG_TRUTH_FALSE)
                {
                    if (chase_sig != op->dst) { return LDG_TRUTH_FALSE; }

                    break;
                }

                if (back_op->kind != CROSSTALK_HDL_IR_OP_MUX || back_op->guard_is != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

                guard_sig = back_op->guard;

                for (g_idx = 0; g_idx < ir_ctx->seq_op_cunt; g_idx = g_idx + 0x1)
                {
                    const crosstalk_hdl_ir_op_t *gw_op = &ir_ctx->seq_ops[g_idx];

                    if (gw_op->kind == CROSSTALK_HDL_IR_OP_REG_NEXT || gw_op->kind == CROSSTALK_HDL_IR_OP_MEM_WR) { continue; }

                    if (gw_op->dst == guard_sig) { return LDG_TRUTH_FALSE; }
                }

                seen = LDG_TRUTH_FALSE;
                for (g_idx = 0; g_idx < *out_cunt; g_idx = g_idx + 0x1) { if (out_guards[g_idx] == guard_sig) { seen = LDG_TRUTH_TRUE; } }

                if (seen == LDG_TRUTH_FALSE)
                {
                    if (*out_cunt >= 0x8) { return LDG_TRUTH_FALSE; }

                    out_guards[*out_cunt] = guard_sig;
                    *out_cunt = *out_cunt + 0x1;
                }

                chase_sig = back_op->src2;
                chase_at = back_at;
                if (chase_sig == UINT32_MAX) { return LDG_TRUTH_FALSE; }
            }

            guard_sig = UINT32_MAX;
        }
        else { continue; }

        if (guard_sig == UINT32_MAX) { continue; }

        for (g_idx = 0; g_idx < ir_ctx->seq_op_cunt; g_idx = g_idx + 0x1)
        {
            const crosstalk_hdl_ir_op_t *gw_op = &ir_ctx->seq_ops[g_idx];

            if (gw_op->kind == CROSSTALK_HDL_IR_OP_REG_NEXT || gw_op->kind == CROSSTALK_HDL_IR_OP_MEM_WR) { continue; }

            if (gw_op->dst == guard_sig) { return LDG_TRUTH_FALSE; }
        }

        for (g_idx = 0; g_idx < *out_cunt; g_idx = g_idx + 0x1) { if (out_guards[g_idx] == guard_sig) { seen = LDG_TRUTH_TRUE; } }

        if (seen == LDG_TRUTH_FALSE)
        {
            if (*out_cunt >= 0x8) { return LDG_TRUTH_FALSE; }

            out_guards[*out_cunt] = guard_sig;
            *out_cunt = *out_cunt + 0x1;
        }
    }

    if (*out_cunt == 0) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static uint32_t crosstalk_hdl_fire_blk_emit(crosstalk_hdl_emit_ctx_t *ctx, uint32_t blk_idx)
{
    uint32_t ret = 0;

    if (ctx->pf_is == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_str_emit(ctx, "{ hdl_u64 pfb; fired[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, blk_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "] = 0x1; pfb = hdl_pf_now(); hdl_gen_b");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, blk_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "(); pfb = hdl_pf_now() - pfb; pf_t[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, blk_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "] += pfb; pf_c[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, blk_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        return crosstalk_hdl_str_emit(ctx, "] += 0x1; pf_cur_blk += pfb; }\n");
    }

    ret = crosstalk_hdl_str_emit(ctx, "fired[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, blk_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "] = 0x1; hdl_gen_b");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, blk_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_str_emit(ctx, "();\n");
}

static uint32_t crosstalk_hdl_u64_tbl_emit(crosstalk_hdl_emit_ctx_t *ctx, const char *hd, uint32_t cunt, uint8_t hex_is, uint64_t (*val_get)(const crosstalk_hdl_emit_ctx_t *, uint32_t))
{
    uint32_t idx = 0;
    uint32_t ret = 0;

    ret = crosstalk_hdl_str_emit(ctx, hd);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < cunt; idx++)
    {
        if (idx > 0)
        {
            ret = crosstalk_hdl_str_emit(ctx, ", ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        if (hex_is == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_hex_emit(ctx, val_get(ctx, idx)); }
        else { ret = crosstalk_hdl_dec_emit(ctx, val_get(ctx, idx)); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return crosstalk_hdl_str_emit(ctx, "};\n");
}

static uint64_t crosstalk_hdl_emit_mask_val_get(const crosstalk_hdl_emit_ctx_t *ctx, uint32_t idx)
{
    return crosstalk_hdl_emit_mask_get(ctx->ir_ctx->sigs[idx].width);
}

static uint64_t crosstalk_hdl_emit_depth_val_get(const crosstalk_hdl_emit_ctx_t *ctx, uint32_t idx)
{
    return (uint64_t)ctx->ir_ctx->sigs[idx].depth;
}

static uint64_t crosstalk_hdl_emit_moff_val_get(const crosstalk_hdl_emit_ctx_t *ctx, uint32_t idx)
{
    if (ctx->mem_off[idx] == UINT32_MAX) { return 0x0; }

    return (uint64_t)ctx->mem_off[idx];
}

static uint32_t crosstalk_hdl_emit_blk_nm_emit(crosstalk_hdl_emit_ctx_t *ctx, uint32_t blk_idx)
{
    const crosstalk_hdl_ir_seq_blk_t *blk = &ctx->ir_ctx->seq_blks[blk_idx];
    uint32_t pick = UINT32_MAX;
    uint32_t op_idx = 0;
    uint32_t nm_len = 0;

    for (op_idx = 0; op_idx < blk->op_cunt; op_idx++)
    {
        const crosstalk_hdl_ir_op_t *op = &ctx->ir_ctx->seq_ops[blk->op_start + op_idx];

        if (op->dst == UINT32_MAX || ctx->ir_ctx->sigs[op->dst].name.len == 0) { continue; }

        if (op->kind == CROSSTALK_HDL_IR_OP_REG_NEXT) { pick = op->dst; break; }

        if (pick == UINT32_MAX) { pick = op->dst; }
    }

    if (pick == UINT32_MAX) { return LDG_ERR_AOK; }

    nm_len = ctx->ir_ctx->sigs[pick].name.len;
    if (nm_len > CROSSTALK_HDL_EMIT_PF_NM_BYTES - 0x1) { nm_len = CROSSTALK_HDL_EMIT_PF_NM_BYTES - 0x1; }

    return crosstalk_hdl_bytes_emit(ctx, ctx->src + ctx->ir_ctx->sigs[pick].name.off, nm_len);
}

static uint8_t crosstalk_hdl_emit_alias_op_is(const crosstalk_hdl_ir_op_t *op)
{
    if (op->kind == CROSSTALK_HDL_IR_OP_COPY) { return LDG_TRUTH_TRUE; }

    if (op->kind == CROSSTALK_HDL_IR_OP_ZEXT) { return LDG_TRUTH_TRUE; }

    if (op->kind == CROSSTALK_HDL_IR_OP_SLICE && op->imm == 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t crosstalk_hdl_emit_alias_has(const uint32_t *alias_arr, uint32_t alias_cunt, uint32_t sig)
{
    uint32_t idx = 0;

    for (idx = 0; idx < alias_cunt; idx++) { if (alias_arr[idx] == sig) { return LDG_TRUTH_TRUE; } }

    return LDG_TRUTH_FALSE;
}

static uint32_t crosstalk_hdl_emit_fast_analyze(crosstalk_hdl_emit_ctx_t *ctx, uint32_t *alias_arr, uint32_t *out_alias_cunt, uint32_t *out_clk_root, uint32_t *out_fast)
{
    const crosstalk_hdl_ir_t *ir_ctx = 0x0;
    uint32_t *wr_tally = 0x0;
    uint32_t *wr_op = 0x0;
    uint8_t *seq_wr = 0x0;
    void *raw = 0x0;
    uint32_t rd_arr[4];
    uint32_t rd_cunt = 0;
    uint32_t rd_i = 0;
    uint32_t alias_cunt = 0;
    uint32_t clk_root = UINT32_MAX;
    uint32_t walk_sig = 0;
    uint32_t depth = 0;
    uint32_t rst_pass = 0;
    uint32_t idx = 0;
    uint32_t op_idx = 0;
    uint32_t ws = 0;
    uint32_t fast = 0x1;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || alias_arr == 0x0 || out_alias_cunt == 0x0 || out_clk_root == 0x0 || out_fast == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_alias_cunt = 0;
    *out_clk_root = UINT32_MAX;
    *out_fast = 0;
    ctx->fast_why = 0;
    ctx->fast_blocker = UINT32_MAX;
    ir_ctx = ctx->ir_ctx;

    ret = ldg_mem_pool_alloc(ctx->pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt) * sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    wr_tally = (uint32_t *)raw;

    ret = ldg_mem_pool_alloc(ctx->pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt) * sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    wr_op = (uint32_t *)raw;

    ret = ldg_mem_pool_alloc(ctx->pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    seq_wr = (uint8_t *)raw;

    for (idx = 0; idx < ir_ctx->sig_cunt; idx++)
    {
        wr_tally[idx] = 0;
        wr_op[idx] = UINT32_MAX;
        seq_wr[idx] = 0x0;
    }

    for (op_idx = 0; op_idx < ir_ctx->comb_cunt; op_idx++)
    {
        ws = crosstalk_hdl_emit_wsig_get(&ir_ctx->comb_ops[op_idx]);

        if (ws == UINT32_MAX) { continue; }

        wr_tally[ws] = wr_tally[ws] + 0x1;
        wr_op[ws] = op_idx;
    }

    for (op_idx = 0; op_idx < ir_ctx->seq_op_cunt; op_idx++)
    {
        if (ir_ctx->seq_ops[op_idx].kind != CROSSTALK_HDL_IR_OP_REG_NEXT) { continue; }

        if (ir_ctx->seq_ops[op_idx].dst == UINT32_MAX) { continue; }

        seq_wr[ir_ctx->seq_ops[op_idx].dst] = 0x1;
    }

    for (idx = 0; idx < ir_ctx->seq_blk_cunt; idx++) { if (ir_ctx->seq_blks[idx].clk_sig != UINT32_MAX && ir_ctx->seq_blks[idx].clk_edge == CROSSTALK_HDL_IR_EDGE_NEG) { fast = 0x0; if (ctx->fast_why == 0) { ctx->fast_why = 0x1; ctx->fast_blocker = idx; } } }

    for (rst_pass = 0; rst_pass < 0x2 && fast != 0x0; rst_pass++)
    {
        for (idx = 0; idx < ir_ctx->seq_blk_cunt && fast != 0x0; idx++)
        {
            walk_sig = rst_pass == 0 ? ir_ctx->seq_blks[idx].clk_sig : ir_ctx->seq_blks[idx].rst_sig;

            if (walk_sig == UINT32_MAX) { continue; }

            for (depth = 0; depth < CROSSTALK_HDL_EMIT_ALIAS_DEPTH; depth++)
            {
                if (wr_tally[walk_sig] == 0) { break; }

                if (wr_tally[walk_sig] != 0x1)
                {
                    fast = 0x0;
                    if (ctx->fast_why == 0) { ctx->fast_why = 0x2; ctx->fast_blocker = walk_sig; }

                    break;
                }

                if (crosstalk_hdl_emit_alias_op_is(&ir_ctx->comb_ops[wr_op[walk_sig]]) != LDG_TRUTH_TRUE)
                {
                    fast = 0x0;
                    if (ctx->fast_why == 0) { ctx->fast_why = 0x3; ctx->fast_blocker = walk_sig; }

                    break;
                }

                if (rst_pass == 0 && crosstalk_hdl_emit_alias_has(alias_arr, alias_cunt, walk_sig) != LDG_TRUTH_TRUE)
                {
                    if (alias_cunt >= CROSSTALK_HDL_EMIT_ALIAS_CAP) { fast = 0x0; break; }

                    alias_arr[alias_cunt] = walk_sig;
                    alias_cunt++;
                }

                walk_sig = ir_ctx->comb_ops[wr_op[walk_sig]].src0;

                if (walk_sig == UINT32_MAX) { fast = 0x0; break; }
            }

            if (depth >= CROSSTALK_HDL_EMIT_ALIAS_DEPTH) { fast = 0x0; if (ctx->fast_why == 0) { ctx->fast_why = 0x4; ctx->fast_blocker = walk_sig; } }

            if (fast == 0x0) { break; }

            if (seq_wr[walk_sig] != 0x0)
            {
                fast = 0x0;
                if (ctx->fast_why == 0) { ctx->fast_why = 0x5; ctx->fast_blocker = walk_sig; }

                break;
            }

            if (rst_pass != 0) { continue; }

            if (clk_root == UINT32_MAX) { clk_root = walk_sig; }
            else if (clk_root != walk_sig)
            {
                fast = 0x0;
                if (ctx->fast_why == 0) { ctx->fast_why = 0x6; ctx->fast_blocker = walk_sig; }

                break;
            }
        }
    }

    depth = 0x1;

    while (depth != 0 && fast != 0x0 && clk_root != UINT32_MAX)
    {
        depth = 0;

        for (op_idx = 0; op_idx < ir_ctx->comb_cunt; op_idx++)
        {
            if (crosstalk_hdl_emit_alias_op_is(&ir_ctx->comb_ops[op_idx]) != LDG_TRUTH_TRUE) { continue; }

            ws = crosstalk_hdl_emit_wsig_get(&ir_ctx->comb_ops[op_idx]);

            if (ws == UINT32_MAX || wr_tally[ws] != 0x1 || wr_op[ws] != op_idx) { continue; }

            if (crosstalk_hdl_emit_alias_has(alias_arr, alias_cunt, ws) == LDG_TRUTH_TRUE) { continue; }

            walk_sig = ir_ctx->comb_ops[op_idx].src0;

            if (walk_sig != clk_root && crosstalk_hdl_emit_alias_has(alias_arr, alias_cunt, walk_sig) != LDG_TRUTH_TRUE) { continue; }

            if (alias_cunt >= CROSSTALK_HDL_EMIT_ALIAS_CAP)
            {
                fast = 0x0;
                if (ctx->fast_why == 0) { ctx->fast_why = 0x4; ctx->fast_blocker = ws; }

                break;
            }

            alias_arr[alias_cunt] = ws;
            alias_cunt++;
            depth = 0x1;
        }
    }

    for (op_idx = 0; op_idx < ir_ctx->comb_cunt && fast != 0x0; op_idx++)
    {
        ws = crosstalk_hdl_emit_wsig_get(&ir_ctx->comb_ops[op_idx]);

        if (clk_root != UINT32_MAX && ws == clk_root)
        {
            fast = 0x0;
            if (ctx->fast_why == 0) { ctx->fast_why = 0x7; ctx->fast_blocker = op_idx; }

            break;
        }

        rd_cunt = crosstalk_hdl_emit_rsigs_get(&ir_ctx->comb_ops[op_idx], rd_arr);

        for (rd_i = 0; rd_i < rd_cunt; rd_i++)
        {
            uint8_t member = 0x0;

            if (clk_root != UINT32_MAX && rd_arr[rd_i] == clk_root) { member = 0x1; }

            if (crosstalk_hdl_emit_alias_has(alias_arr, alias_cunt, rd_arr[rd_i]) == LDG_TRUTH_TRUE) { member = 0x1; }

            if (member == 0x0) { continue; }

            if (crosstalk_hdl_emit_alias_op_is(&ir_ctx->comb_ops[op_idx]) == LDG_TRUTH_TRUE && ws != UINT32_MAX && wr_tally[ws] == 0x1 && wr_op[ws] == op_idx && crosstalk_hdl_emit_alias_has(alias_arr, alias_cunt, ws) == LDG_TRUTH_TRUE) { continue; }

            fast = 0x0;

            if (ctx->fast_why == 0) { ctx->fast_why = 0x8; ctx->fast_blocker = op_idx; }

            break;
        }
    }

    *out_alias_cunt = alias_cunt;
    *out_clk_root = clk_root;
    *out_fast = fast;

    return LDG_ERR_AOK;
}

static void crosstalk_hdl_emit_fuse_mark(crosstalk_hdl_emit_ctx_t *ctx, const uint32_t *arr, uint32_t cunt, const uint32_t *rdt, const uint32_t *wrt)
{
    const crosstalk_hdl_ir_t *ir_ctx = ctx->ir_ctx;
    uint32_t rd_arr[4];
    uint32_t rd_cunt = 0;
    uint32_t rd_i = 0;
    uint32_t idx = 0;
    uint32_t run = 0;

    if (cunt < 0x2) { return; }

    for (idx = 0; idx + 0x1 < cunt; idx++)
    {
        const crosstalk_hdl_ir_op_t *pop = &ir_ctx->comb_ops[arr[idx]];
        const crosstalk_hdl_ir_op_t *cop = &ir_ctx->comb_ops[arr[idx + 0x1]];
        uint32_t pd = pop->dst;
        uint8_t rd_has = LDG_TRUTH_FALSE;
        uint8_t self_rd = LDG_TRUTH_FALSE;

        if (run >= 96) { run = 0; continue; }

        if (pop->kind == CROSSTALK_HDL_IR_OP_MEM_WR) { run = 0; continue; }

        if (crosstalk_hdl_emit_op_wide_is(ir_ctx, pop) == LDG_TRUTH_TRUE || crosstalk_hdl_emit_op_wide_is(ir_ctx, cop) == LDG_TRUTH_TRUE) { run = 0; continue; }

        if (pd == UINT32_MAX || ir_ctx->sigs[pd].name.len > 0 || ir_ctx->sigs[pd].port != CROSSTALK_HDL_IR_PORT_NONE || ir_ctx->sigs[pd].depth > 0) { run = 0; continue; }

        if (ctx->guard_need[pd] != 0x0) { run = 0; continue; }

        if (wrt[pd] != 0x1 || rdt[pd] != 0x1) { run = 0; continue; }

        rd_cunt = crosstalk_hdl_emit_rsigs_get(pop, rd_arr);

        for (rd_i = 0; rd_i < rd_cunt; rd_i++) { if (rd_arr[rd_i] == pd) { self_rd = LDG_TRUTH_TRUE; } }

        if (self_rd == LDG_TRUTH_TRUE) { run = 0; continue; }

        rd_cunt = crosstalk_hdl_emit_rsigs_get(cop, rd_arr);

        for (rd_i = 0; rd_i < rd_cunt; rd_i++) { if (rd_arr[rd_i] == pd) { rd_has = LDG_TRUTH_TRUE; } }

        if (rd_has != LDG_TRUTH_TRUE) { run = 0; continue; }

        ctx->fused[arr[idx]] = 0x1;
        ctx->fuse_map[pd] = arr[idx];
        ctx->fuse_cunt++;
        run++;
    }
}

static void crosstalk_hdl_emit_fuse_seq_mark(crosstalk_hdl_emit_ctx_t *ctx, uint32_t op_start, uint32_t op_cunt, const uint32_t *rdt, const uint32_t *wrt, const uint32_t *swrt)
{
    const crosstalk_hdl_ir_t *ir_ctx = ctx->ir_ctx;
    uint32_t rd_arr[4];
    uint32_t rd_cunt = 0;
    uint32_t rd_i = 0;
    uint32_t idx = 0;
    uint32_t run = 0;

    if (op_cunt < 0x2) { return; }

    for (idx = op_start; idx + 0x1 < op_start + op_cunt; idx++)
    {
        const crosstalk_hdl_ir_op_t *pop = &ir_ctx->seq_ops[idx];
        const crosstalk_hdl_ir_op_t *cop = &ir_ctx->seq_ops[idx + 0x1];
        uint32_t pd = pop->dst;
        uint8_t rd_has = LDG_TRUTH_FALSE;
        uint8_t self_rd = LDG_TRUTH_FALSE;

        if (run >= 96) { run = 0; continue; }

        if (pop->kind == CROSSTALK_HDL_IR_OP_MEM_WR || pop->kind == CROSSTALK_HDL_IR_OP_REG_NEXT) { run = 0; continue; }

        if (crosstalk_hdl_emit_op_wide_is(ir_ctx, pop) == LDG_TRUTH_TRUE || crosstalk_hdl_emit_op_wide_is(ir_ctx, cop) == LDG_TRUTH_TRUE) { run = 0; continue; }

        if (pd == UINT32_MAX || ir_ctx->sigs[pd].name.len > 0 || ir_ctx->sigs[pd].port != CROSSTALK_HDL_IR_PORT_NONE || ir_ctx->sigs[pd].depth > 0) { run = 0; continue; }

        if (ctx->guard_need[pd] != 0x0) { run = 0; continue; }

        if (wrt[pd] != 0 || swrt[pd] != 0x1 || rdt[pd] != 0x1) { run = 0; continue; }

        rd_cunt = crosstalk_hdl_emit_rsigs_get(pop, rd_arr);

        for (rd_i = 0; rd_i < rd_cunt; rd_i++) { if (rd_arr[rd_i] == pd) { self_rd = LDG_TRUTH_TRUE; } }

        if (self_rd == LDG_TRUTH_TRUE) { run = 0; continue; }

        rd_cunt = crosstalk_hdl_emit_rsigs_get(cop, rd_arr);

        for (rd_i = 0; rd_i < rd_cunt; rd_i++) { if (rd_arr[rd_i] == pd) { rd_has = LDG_TRUTH_TRUE; } }

        if (rd_has != LDG_TRUTH_TRUE) { run = 0; continue; }

        ctx->fused_seq[idx] = 0x1;
        ctx->fuse_map_seq[pd] = idx;
        ctx->fuse_cunt++;
        run++;
    }
}

static uint32_t crosstalk_hdl_emit_rmw_cone_get(const crosstalk_hdl_emit_ctx_t *ctx, uint32_t op_idx, uint32_t *out_arr, uint32_t cap)
{
    const crosstalk_hdl_ir_t *ir_ctx = ctx->ir_ctx;
    uint32_t stk[128];
    uint32_t stk_cunt = 0;
    uint32_t seen = 0;
    uint32_t rd_arr[4];
    uint32_t rd_cunt = 0;
    uint32_t rd_i = 0;
    uint32_t cur = op_idx;
    uint32_t cunt = 0;

    while (cur != UINT32_MAX)
    {
        if (stk_cunt >= 128) { return UINT32_MAX; }

        stk[stk_cunt] = cur;
        stk_cunt++;
        cur = ctx->vf_prev[cur];
    }

    while (stk_cunt > 0)
    {
        if (seen >= 256) { return UINT32_MAX; }

        seen++;
        stk_cunt--;
        rd_cunt = crosstalk_hdl_emit_rsigs_get(&ir_ctx->comb_ops[stk[stk_cunt]], rd_arr);

        for (rd_i = 0; rd_i < rd_cunt; rd_i++)
        {
            uint32_t rsig = rd_arr[rd_i];

            if (cunt >= cap) { return UINT32_MAX; }

            out_arr[cunt] = rsig;
            cunt++;

            if (ctx->fuse_map[rsig] != UINT32_MAX)
            {
                if (stk_cunt >= 128) { return UINT32_MAX; }

                stk[stk_cunt] = ctx->fuse_map[rsig];
                stk_cunt++;
            }
        }
    }

    return cunt;
}

static void crosstalk_hdl_emit_rmw_mark(crosstalk_hdl_emit_ctx_t *ctx, const uint32_t *arr, uint32_t cunt, const uint32_t *ver_rdt, const uint32_t *ver_rdr)
{
    const crosstalk_hdl_ir_t *ir_ctx = ctx->ir_ctx;
    uint32_t cone_arr[384];
    uint32_t cone_cunt = 0;
    uint32_t cone_i = 0;
    uint32_t rd_arr[4];
    uint32_t rd_cunt = 0;
    uint32_t rd_i = 0;
    uint32_t idx = 0;
    uint32_t gap_j = 0;
    uint32_t gap_end = 0;

    if (cunt < 0x2) { return; }

    for (idx = 0; idx + 0x1 < cunt; idx++)
    {
        const crosstalk_hdl_ir_op_t *pop = &ir_ctx->comb_ops[arr[idx]];
        const crosstalk_hdl_ir_op_t *cop = 0x0;
        uint32_t pd = pop->dst;
        uint32_t c_op = ver_rdr[arr[idx]];
        uint8_t found = LDG_TRUTH_FALSE;
        uint8_t clean = LDG_TRUTH_TRUE;

        if (pop->kind == CROSSTALK_HDL_IR_OP_MEM_WR) { continue; }

        if (crosstalk_hdl_emit_op_wide_is(ir_ctx, pop) == LDG_TRUTH_TRUE) { continue; }

        if (pd == UINT32_MAX || ir_ctx->sigs[pd].depth > 0) { continue; }

        if (ctx->guard_need[pd] != 0x0) { continue; }

        if (ctx->fused[arr[idx]] != 0x0) { continue; }

        if (ver_rdt[arr[idx]] != 0x1 || c_op == UINT32_MAX) { continue; }

        cop = &ir_ctx->comb_ops[c_op];

        if (cop->kind == CROSSTALK_HDL_IR_OP_MEM_WR || cop->dst != pd) { continue; }

        if (crosstalk_hdl_emit_op_wide_is(ir_ctx, cop) == LDG_TRUTH_TRUE) { continue; }

        if (ctx->fused[c_op] != 0x0 || ctx->vf_prev[c_op] != UINT32_MAX) { continue; }

        cone_cunt = crosstalk_hdl_emit_rmw_cone_get(ctx, arr[idx], cone_arr, 384);

        if (cone_cunt == UINT32_MAX) { continue; }

        gap_end = idx + 0x1 + 64;

        if (gap_end > cunt) { gap_end = cunt; }

        for (gap_j = idx + 0x1; gap_j < gap_end; gap_j++)
        {
            const crosstalk_hdl_ir_op_t *xop = 0x0;
            uint32_t xw = 0;

            if (arr[gap_j] == c_op) { found = LDG_TRUTH_TRUE; break; }

            xop = &ir_ctx->comb_ops[arr[gap_j]];
            xw = crosstalk_hdl_emit_wsig_get(xop);

            if (xw == pd) { clean = LDG_TRUTH_FALSE; break; }

            rd_cunt = crosstalk_hdl_emit_rsigs_get(xop, rd_arr);

            for (rd_i = 0; rd_i < rd_cunt; rd_i++) { if (rd_arr[rd_i] == pd) { clean = LDG_TRUTH_FALSE; } }

            if (xw != UINT32_MAX) { for (cone_i = 0; cone_i < cone_cunt; cone_i++) { if (cone_arr[cone_i] == xw) { clean = LDG_TRUTH_FALSE; } } }

            if (clean != LDG_TRUTH_TRUE) { break; }
        }

        if (found != LDG_TRUTH_TRUE || clean != LDG_TRUTH_TRUE) { continue; }

        ctx->fused[arr[idx]] = 0x1;
        ctx->vf_prev[c_op] = arr[idx];
        ctx->fuse_cunt++;
    }
}

static uint8_t crosstalk_hdl_emit_alias_safe_is(const crosstalk_hdl_ir_t *ir_src, const crosstalk_hdl_ir_op_t *op)
{
    uint32_t dst_w = 0;
    uint32_t src_w = 0;

    if (op->dst == UINT32_MAX || op->src0 == UINT32_MAX) { return LDG_TRUTH_FALSE; }

    if (ir_src->sigs[op->dst].depth > 0 || ir_src->sigs[op->src0].depth > 0) { return LDG_TRUTH_FALSE; }

    dst_w = ir_src->sigs[op->dst].width;
    src_w = ir_src->sigs[op->src0].width;

    if (dst_w < src_w) { return LDG_TRUTH_FALSE; }

    if (op->kind == CROSSTALK_HDL_IR_OP_COPY) { return LDG_TRUTH_TRUE; }

    if (op->kind == CROSSTALK_HDL_IR_OP_ZEXT) { return LDG_TRUTH_TRUE; }

    if (op->kind == CROSSTALK_HDL_IR_OP_SLICE && op->imm == 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t crosstalk_hdl_emit_opt_run(crosstalk_hdl_ir_t *ir2, const crosstalk_hdl_ir_t *ir_src, void *pool, uint8_t strip_is)
{
    crosstalk_hdl_ir_op_t *comb2 = 0x0;
    crosstalk_hdl_ir_op_t *seq2 = 0x0;
    uint32_t *wr_tally = 0x0;
    uint32_t *wr_op = 0x0;
    uint32_t *alias_map = 0x0;
    uint32_t *rd_tally = 0x0;
    uint8_t *seq_wrf = 0x0;
    uint8_t *dropped = 0x0;
    void *raw = 0x0;
    uint32_t rd_arr[4];
    uint32_t rd_cunt = 0;
    uint32_t rd_i = 0;
    uint32_t idx = 0;
    uint32_t depth = 0;
    uint32_t pass = 0;
    uint32_t pack_i = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ir2 == 0x0 || ir_src == 0x0 || pool == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *ir2 = *ir_src;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_src->comb_cunt == 0 ? 0x1 : ir_src->comb_cunt) * sizeof(crosstalk_hdl_ir_op_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    comb2 = (crosstalk_hdl_ir_op_t *)raw;

    for (idx = 0; idx < ir_src->comb_cunt; idx++) { comb2[idx] = ir_src->comb_ops[idx]; }

    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_src->seq_op_cunt == 0 ? 0x1 : ir_src->seq_op_cunt) * sizeof(crosstalk_hdl_ir_op_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    seq2 = (crosstalk_hdl_ir_op_t *)raw;

    for (idx = 0; idx < ir_src->seq_op_cunt; idx++) { seq2[idx] = ir_src->seq_ops[idx]; }

    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_src->sig_cunt == 0 ? 0x1 : ir_src->sig_cunt) * sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    wr_tally = (uint32_t *)raw;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_src->sig_cunt == 0 ? 0x1 : ir_src->sig_cunt) * sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    wr_op = (uint32_t *)raw;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_src->sig_cunt == 0 ? 0x1 : ir_src->sig_cunt) * sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    alias_map = (uint32_t *)raw;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_src->sig_cunt == 0 ? 0x1 : ir_src->sig_cunt) * sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    rd_tally = (uint32_t *)raw;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_src->sig_cunt == 0 ? 0x1 : ir_src->sig_cunt), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    seq_wrf = (uint8_t *)raw;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_src->comb_cunt == 0 ? 0x1 : ir_src->comb_cunt), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    dropped = (uint8_t *)raw;

    for (idx = 0; idx < ir_src->sig_cunt; idx++)
    {
        wr_tally[idx] = 0;
        wr_op[idx] = UINT32_MAX;
        alias_map[idx] = UINT32_MAX;
        rd_tally[idx] = 0;
        seq_wrf[idx] = 0x0;
    }

    for (idx = 0; idx < ir_src->comb_cunt; idx++)
    {
        uint32_t ws = crosstalk_hdl_emit_wsig_get(&comb2[idx]);

        dropped[idx] = 0x0;

        if (ws == UINT32_MAX) { continue; }

        wr_tally[ws] = wr_tally[ws] + 0x1;
        wr_op[ws] = idx;
    }

    for (idx = 0; idx < ir_src->seq_op_cunt; idx++)
    {
        uint32_t ws = crosstalk_hdl_emit_wsig_get(&seq2[idx]);

        if (ws != UINT32_MAX) { seq_wrf[ws] = 0x1; }
    }

    for (idx = 0; idx < ir_src->sig_cunt; idx++)
    {
        uint32_t walk = idx;

        for (depth = 0; depth < 64; depth++)
        {
            if (seq_wrf[walk] != 0x0) { break; }

            if (wr_tally[walk] != 0x1) { break; }

            if (crosstalk_hdl_emit_alias_safe_is(ir_src, &comb2[wr_op[walk]]) != LDG_TRUTH_TRUE) { break; }

            walk = comb2[wr_op[walk]].src0;
        }

        if (walk != idx) { alias_map[idx] = walk; }
    }

    for (idx = 0; idx < ir_src->comb_cunt; idx++)
    {
        crosstalk_hdl_ir_op_t *op2 = &comb2[idx];

        if (op2->kind == CROSSTALK_HDL_IR_OP_MEM_WR) { if (op2->dst != UINT32_MAX && alias_map[op2->dst] != UINT32_MAX) { op2->dst = alias_map[op2->dst]; } }
        else if (op2->kind != CROSSTALK_HDL_IR_OP_MEM_RD && op2->src0 != UINT32_MAX && alias_map[op2->src0] != UINT32_MAX) { op2->src0 = alias_map[op2->src0]; }

        if (op2->src1 != UINT32_MAX && alias_map[op2->src1] != UINT32_MAX) { op2->src1 = alias_map[op2->src1]; }

        if (op2->src2 != UINT32_MAX && alias_map[op2->src2] != UINT32_MAX) { op2->src2 = alias_map[op2->src2]; }
    }

    for (idx = 0; idx < ir_src->seq_op_cunt; idx++)
    {
        crosstalk_hdl_ir_op_t *op2 = &seq2[idx];

        if (op2->kind == CROSSTALK_HDL_IR_OP_MEM_WR) { if (op2->dst != UINT32_MAX && alias_map[op2->dst] != UINT32_MAX) { op2->dst = alias_map[op2->dst]; } }
        else if (op2->kind != CROSSTALK_HDL_IR_OP_MEM_RD && op2->src0 != UINT32_MAX && alias_map[op2->src0] != UINT32_MAX) { op2->src0 = alias_map[op2->src0]; }

        if (op2->src1 != UINT32_MAX && alias_map[op2->src1] != UINT32_MAX) { op2->src1 = alias_map[op2->src1]; }

        if (op2->src2 != UINT32_MAX && alias_map[op2->src2] != UINT32_MAX) { op2->src2 = alias_map[op2->src2]; }
    }

    {
        uint8_t *stable = 0x0;
        uint32_t *cse_map = 0x0;
        uint32_t *htab = 0x0;
        uint32_t hmix = 2654435761;
        uint32_t hcap = 0x2;
        uint32_t sweep = 0;

        while (hcap < ir_src->comb_cunt * 0x2 + 0x2) { hcap = hcap << 0x1; }

        ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_src->sig_cunt == 0 ? 0x1 : ir_src->sig_cunt), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        stable = (uint8_t *)raw;

        ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_src->sig_cunt == 0 ? 0x1 : ir_src->sig_cunt) * sizeof(uint32_t), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        cse_map = (uint32_t *)raw;

        ret = ldg_mem_pool_alloc(pool, (uint64_t)hcap * sizeof(uint32_t), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        htab = (uint32_t *)raw;

        for (idx = 0; idx < ir_src->sig_cunt; idx++) { stable[idx] = wr_tally[idx] == 0 ? 0x1 : 0x0; cse_map[idx] = UINT32_MAX; }

        for (idx = 0; idx < hcap; idx++) { htab[idx] = UINT32_MAX; }

        for (sweep = 0; sweep < 64; sweep++)
        {
            uint32_t grew = 0;

            for (idx = 0; idx < ir_src->comb_cunt; idx++)
            {
                const crosstalk_hdl_ir_op_t *cop2 = &comb2[idx];
                uint8_t all_st = 0x1;

                if (cop2->kind == CROSSTALK_HDL_IR_OP_MEM_WR || cop2->kind == CROSSTALK_HDL_IR_OP_MEM_RD) { continue; }

                if (cop2->dst == UINT32_MAX || stable[cop2->dst] != 0x0) { continue; }

                if (wr_tally[cop2->dst] != 0x1) { continue; }

                rd_cunt = crosstalk_hdl_emit_rsigs_get(cop2, rd_arr);

                for (rd_i = 0; rd_i < rd_cunt; rd_i++) { if (stable[rd_arr[rd_i]] == 0x0) { all_st = 0x0; } }

                if (all_st == 0x0) { continue; }

                stable[cop2->dst] = 0x1;
                grew = 0x1;
            }

            if (grew == 0) { break; }
        }

        for (idx = 0; idx < ir_src->comb_cunt; idx++)
        {
            const crosstalk_hdl_ir_op_t *cop2 = &comb2[idx];
            uint32_t ws = cop2->dst;
            uint32_t hh = 0;
            uint32_t probe = 0;

            if (cop2->kind == CROSSTALK_HDL_IR_OP_MEM_WR || cop2->kind == CROSSTALK_HDL_IR_OP_MEM_RD || cop2->kind == CROSSTALK_HDL_IR_OP_CONST) { continue; }

            if (ws == UINT32_MAX || ir_src->sigs[ws].depth > 0) { continue; }

            if (wr_tally[ws] != 0x1 || seq_wrf[ws] != 0x0 || stable[ws] == 0x0) { continue; }

            hh = cop2->kind;
            hh = hh * hmix ^ cop2->src0;
            hh = hh * hmix ^ cop2->src1;
            hh = hh * hmix ^ cop2->src2;
            hh = hh * hmix ^ (uint32_t)cop2->imm;
            hh = hh * hmix ^ (uint32_t)(cop2->imm >> 32);
            hh = hh * hmix ^ cop2->width;
            hh = hh * hmix ^ ir_src->sigs[ws].width;

            for (probe = 0; probe < hcap; probe++)
            {
                uint32_t slot = (hh + probe) & (hcap - 0x1);
                const crosstalk_hdl_ir_op_t *xop = 0x0;

                if (htab[slot] == UINT32_MAX) { htab[slot] = idx; break; }

                xop = &comb2[htab[slot]];

                if (xop->kind != cop2->kind || xop->src0 != cop2->src0 || xop->src1 != cop2->src1 || xop->src2 != cop2->src2 || xop->imm != cop2->imm || xop->width != cop2->width) { continue; }

                if (ir_src->sigs[xop->dst].width != ir_src->sigs[ws].width || ir_src->sigs[xop->dst].init_val != ir_src->sigs[ws].init_val) { continue; }

                if (ir_src->sigs[ws].name.len == 0 && ir_src->sigs[ws].port == CROSSTALK_HDL_IR_PORT_NONE) { cse_map[ws] = xop->dst; }

                break;
            }
        }

        for (idx = 0; idx < ir_src->comb_cunt; idx++)
        {
            crosstalk_hdl_ir_op_t *op2 = &comb2[idx];

            if (op2->kind == CROSSTALK_HDL_IR_OP_MEM_WR) { if (op2->dst != UINT32_MAX && cse_map[op2->dst] != UINT32_MAX) { op2->dst = cse_map[op2->dst]; } }
            else if (op2->kind != CROSSTALK_HDL_IR_OP_MEM_RD && op2->src0 != UINT32_MAX && cse_map[op2->src0] != UINT32_MAX) { op2->src0 = cse_map[op2->src0]; }

            if (op2->src1 != UINT32_MAX && cse_map[op2->src1] != UINT32_MAX) { op2->src1 = cse_map[op2->src1]; }

            if (op2->src2 != UINT32_MAX && cse_map[op2->src2] != UINT32_MAX) { op2->src2 = cse_map[op2->src2]; }
        }

        for (idx = 0; idx < ir_src->seq_op_cunt; idx++)
        {
            crosstalk_hdl_ir_op_t *op2 = &seq2[idx];

            if (op2->kind == CROSSTALK_HDL_IR_OP_MEM_WR) { if (op2->dst != UINT32_MAX && cse_map[op2->dst] != UINT32_MAX) { op2->dst = cse_map[op2->dst]; } }
            else if (op2->kind != CROSSTALK_HDL_IR_OP_MEM_RD && op2->src0 != UINT32_MAX && cse_map[op2->src0] != UINT32_MAX) { op2->src0 = cse_map[op2->src0]; }

            if (op2->src1 != UINT32_MAX && cse_map[op2->src1] != UINT32_MAX) { op2->src1 = cse_map[op2->src1]; }

            if (op2->src2 != UINT32_MAX && cse_map[op2->src2] != UINT32_MAX) { op2->src2 = cse_map[op2->src2]; }
        }
    }

    {
        uint64_t *cp_val = 0x0;
        uint8_t *cp_has = 0x0;
        uint32_t cp_pass = 0;
        uint32_t cp_chg = 0;

        ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_src->sig_cunt == 0 ? 0x1 : ir_src->sig_cunt) * sizeof(uint64_t), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        cp_val = (uint64_t *)raw;

        ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_src->sig_cunt == 0 ? 0x1 : ir_src->sig_cunt), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        cp_has = (uint8_t *)raw;

        for (idx = 0; idx < ir_src->sig_cunt; idx++)
        {
            cp_has[idx] = 0x0;
            cp_val[idx] = 0;

            if (ir_src->sigs[idx].port != CROSSTALK_HDL_IR_PORT_NONE || ir_src->sigs[idx].depth > 0 || ir_src->sigs[idx].width > 64 || seq_wrf[idx] != 0x0) { continue; }

            if (wr_tally[idx] == 0x0) { cp_has[idx] = 0x1; cp_val[idx] = ir_src->sigs[idx].init_val & crosstalk_hdl_emit_mask_get(ir_src->sigs[idx].width); }
            else if (wr_tally[idx] == 0x1 && comb2[wr_op[idx]].kind == CROSSTALK_HDL_IR_OP_CONST) { cp_has[idx] = 0x1; cp_val[idx] = comb2[wr_op[idx]].imm & crosstalk_hdl_emit_mask_get(ir_src->sigs[idx].width); }
        }

        for (cp_pass = 0; cp_pass < 0x8; cp_pass++)
        {
            cp_chg = 0;

            for (idx = 0; idx < ir_src->comb_cunt; idx++)
            {
                crosstalk_hdl_ir_op_t *cp_op = &comb2[idx];
                uint64_t v0 = 0;
                uint64_t v1 = 0;
                uint64_t cp_r = 0;
                uint8_t c0_ok = 0x0;
                uint8_t c1_ok = 0x0;
                uint8_t fold_ok = 0x0;

                if (cp_op->dst == UINT32_MAX || ir_src->sigs[cp_op->dst].width > 64) { continue; }

                if (crosstalk_hdl_emit_op_wide_is(ir_src, cp_op) == LDG_TRUTH_TRUE) { continue; }

                if (cp_op->src0 != UINT32_MAX && cp_has[cp_op->src0] != 0x0) { c0_ok = 0x1; v0 = cp_val[cp_op->src0]; }

                if (cp_op->src1 != UINT32_MAX && cp_has[cp_op->src1] != 0x0) { c1_ok = 0x1; v1 = cp_val[cp_op->src1]; }

                if (cp_op->kind == CROSSTALK_HDL_IR_OP_MUX && c0_ok != 0x0 && (v0 != 0x0 ? cp_op->src1 : cp_op->src2) != UINT32_MAX)
                {
                    cp_op->kind = CROSSTALK_HDL_IR_OP_COPY;
                    cp_op->src0 = v0 != 0x0 ? cp_op->src1 : cp_op->src2;
                    cp_op->src1 = UINT32_MAX;
                    cp_op->src2 = UINT32_MAX;
                    cp_chg = 0x1;
                    continue;
                }

                switch (cp_op->kind)
                {
                    case CROSSTALK_HDL_IR_OP_COPY:
                    case CROSSTALK_HDL_IR_OP_ZEXT:
                        if (c0_ok != 0x0) { cp_r = v0; fold_ok = 0x1; }

                        break;
                    case CROSSTALK_HDL_IR_OP_NOT:
                        if (c0_ok != 0x0) { cp_r = ~v0; fold_ok = 0x1; }

                        break;
                    case CROSSTALK_HDL_IR_OP_LNOT:
                        if (c0_ok != 0x0) { cp_r = v0 == 0x0 ? 0x1 : 0x0; fold_ok = 0x1; }

                        break;
                    case CROSSTALK_HDL_IR_OP_NEG:
                        if (c0_ok != 0x0) { cp_r = (uint64_t)0x0 - v0; fold_ok = 0x1; }

                        break;
                    case CROSSTALK_HDL_IR_OP_RED_OR:
                        if (c0_ok != 0x0) { cp_r = v0 != 0x0 ? 0x1 : 0x0; fold_ok = 0x1; }

                        break;
                    case CROSSTALK_HDL_IR_OP_RED_AND:
                        if (c0_ok != 0x0) { cp_r = v0 == crosstalk_hdl_emit_mask_get(ir_src->sigs[cp_op->src0].width) ? 0x1 : 0x0; fold_ok = 0x1; }

                        break;
                    case CROSSTALK_HDL_IR_OP_RED_XOR:
                        if (c0_ok != 0x0)
                        {
                            uint64_t px = v0;

                            px = px ^ (px >> 32);
                            px = px ^ (px >> 16);
                            px = px ^ (px >> 0x8);
                            px = px ^ (px >> 0x4);
                            px = px ^ (px >> 0x2);
                            px = px ^ (px >> 0x1);
                            cp_r = px & 0x1;
                            fold_ok = 0x1;
                        }

                        break;
                    case CROSSTALK_HDL_IR_OP_AND:
                        if (c0_ok != 0x0 && c1_ok != 0x0) { cp_r = v0 & v1; fold_ok = 0x1; }

                        break;
                    case CROSSTALK_HDL_IR_OP_OR:
                        if (c0_ok != 0x0 && c1_ok != 0x0) { cp_r = v0 | v1; fold_ok = 0x1; }

                        break;
                    case CROSSTALK_HDL_IR_OP_XOR:
                        if (c0_ok != 0x0 && c1_ok != 0x0) { cp_r = v0 ^ v1; fold_ok = 0x1; }

                        break;
                    case CROSSTALK_HDL_IR_OP_ADD:
                        if (c0_ok != 0x0 && c1_ok != 0x0) { cp_r = v0 + v1; fold_ok = 0x1; }

                        break;
                    case CROSSTALK_HDL_IR_OP_SUB:
                        if (c0_ok != 0x0 && c1_ok != 0x0) { cp_r = v0 - v1; fold_ok = 0x1; }

                        break;
                    case CROSSTALK_HDL_IR_OP_MUL:
                        if (c0_ok != 0x0 && c1_ok != 0x0) { cp_r = v0 * v1; fold_ok = 0x1; }

                        break;
                    case CROSSTALK_HDL_IR_OP_SHL:
                        if (c0_ok != 0x0 && c1_ok != 0x0) { cp_r = v1 >= 64 ? 0x0 : v0 << v1; fold_ok = 0x1; }

                        break;
                    case CROSSTALK_HDL_IR_OP_SHR:
                        if (c0_ok != 0x0 && c1_ok != 0x0) { cp_r = v1 >= 64 ? 0x0 : v0 >> v1; fold_ok = 0x1; }

                        break;
                    case CROSSTALK_HDL_IR_OP_EQ:
                        if (c0_ok != 0x0 && c1_ok != 0x0) { cp_r = v0 == v1 ? 0x1 : 0x0; fold_ok = 0x1; }

                        break;
                    case CROSSTALK_HDL_IR_OP_NEQ:
                        if (c0_ok != 0x0 && c1_ok != 0x0) { cp_r = v0 != v1 ? 0x1 : 0x0; fold_ok = 0x1; }

                        break;
                    case CROSSTALK_HDL_IR_OP_LT:
                        if (c0_ok != 0x0 && c1_ok != 0x0) { cp_r = v0 < v1 ? 0x1 : 0x0; fold_ok = 0x1; }

                        break;
                    case CROSSTALK_HDL_IR_OP_LE:
                        if (c0_ok != 0x0 && c1_ok != 0x0) { cp_r = v0 <= v1 ? 0x1 : 0x0; fold_ok = 0x1; }

                        break;
                    case CROSSTALK_HDL_IR_OP_GT:
                        if (c0_ok != 0x0 && c1_ok != 0x0) { cp_r = v0 > v1 ? 0x1 : 0x0; fold_ok = 0x1; }

                        break;
                    case CROSSTALK_HDL_IR_OP_GE:
                        if (c0_ok != 0x0 && c1_ok != 0x0) { cp_r = v0 >= v1 ? 0x1 : 0x0; fold_ok = 0x1; }

                        break;
                    case CROSSTALK_HDL_IR_OP_SLICE:
                        if (c0_ok != 0x0) { cp_r = cp_op->imm >= 64 ? 0x0 : v0 >> cp_op->imm; fold_ok = 0x1; }

                        break;
                    default:
                        break;
                }

                if (fold_ok == 0x0) { continue; }

                cp_r = cp_r & crosstalk_hdl_emit_mask_get(ir_src->sigs[cp_op->dst].width);
                cp_op->kind = CROSSTALK_HDL_IR_OP_CONST;
                cp_op->imm = cp_r;
                cp_op->src0 = UINT32_MAX;
                cp_op->src1 = UINT32_MAX;
                cp_op->src2 = UINT32_MAX;
                cp_chg = 0x1;

                if (wr_tally[cp_op->dst] == 0x1 && seq_wrf[cp_op->dst] == 0x0 && ir_src->sigs[cp_op->dst].port == CROSSTALK_HDL_IR_PORT_NONE && ir_src->sigs[cp_op->dst].depth == 0x0) { cp_has[cp_op->dst] = 0x1; cp_val[cp_op->dst] = cp_r; }
            }

            if (cp_chg == 0x0) { break; }
        }
    }

    for (idx = 0; idx < ir_src->comb_cunt + ir_src->seq_op_cunt; idx++)
    {
        crosstalk_hdl_ir_op_t *taut_op = idx < ir_src->comb_cunt ? &comb2[idx] : &seq2[idx - ir_src->comb_cunt];
        uint64_t taut_val = 0;
        uint8_t taut_is = 0x0;

        if (taut_op->src0 == UINT32_MAX || taut_op->src0 != taut_op->src1) { continue; }

        if (taut_op->dst == UINT32_MAX || ir_src->sigs[taut_op->dst].width > 64) { continue; }

        switch (taut_op->kind)
        {
            case CROSSTALK_HDL_IR_OP_EQ:
            case CROSSTALK_HDL_IR_OP_LE:
            case CROSSTALK_HDL_IR_OP_GE:
            case CROSSTALK_HDL_IR_OP_LE_S:
            case CROSSTALK_HDL_IR_OP_GE_S:
                taut_val = 0x1;
                taut_is = 0x1;
                break;
            case CROSSTALK_HDL_IR_OP_NEQ:
            case CROSSTALK_HDL_IR_OP_LT:
            case CROSSTALK_HDL_IR_OP_GT:
            case CROSSTALK_HDL_IR_OP_LT_S:
            case CROSSTALK_HDL_IR_OP_GT_S:
            case CROSSTALK_HDL_IR_OP_XOR:
            case CROSSTALK_HDL_IR_OP_SUB:
                taut_val = 0x0;
                taut_is = 0x1;
                break;
            default:
                break;
        }

        if (taut_is == 0x0) { continue; }

        taut_op->kind = CROSSTALK_HDL_IR_OP_CONST;
        taut_op->imm = taut_val;
        taut_op->src0 = UINT32_MAX;
        taut_op->src1 = UINT32_MAX;
        taut_op->src2 = UINT32_MAX;
    }

    for (idx = 0; idx < ir_src->comb_cunt; idx++)
    {
        rd_cunt = crosstalk_hdl_emit_rsigs_get(&comb2[idx], rd_arr);

        for (rd_i = 0; rd_i < rd_cunt; rd_i++) { rd_tally[rd_arr[rd_i]] = rd_tally[rd_arr[rd_i]] + 0x1; }

        if (comb2[idx].guard_is != 0x0 && comb2[idx].guard != UINT32_MAX) { rd_tally[comb2[idx].guard] = rd_tally[comb2[idx].guard] + 0x1; }
    }

    for (idx = 0; idx < ir_src->seq_op_cunt; idx++)
    {
        rd_cunt = crosstalk_hdl_emit_rsigs_get(&seq2[idx], rd_arr);

        for (rd_i = 0; rd_i < rd_cunt; rd_i++) { rd_tally[rd_arr[rd_i]] = rd_tally[rd_arr[rd_i]] + 0x1; }

        if (seq2[idx].guard_is != 0x0 && seq2[idx].guard != UINT32_MAX) { rd_tally[seq2[idx].guard] = rd_tally[seq2[idx].guard] + 0x1; }

        if (seq2[idx].kind == CROSSTALK_HDL_IR_OP_REG_NEXT && seq2[idx].src0 != UINT32_MAX) { rd_tally[seq2[idx].src0] = rd_tally[seq2[idx].src0] + 0x1; }
    }

    for (idx = 0; idx < ir_src->seq_blk_cunt; idx++)
    {
        if (ir_src->seq_blks[idx].clk_sig != UINT32_MAX) { rd_tally[ir_src->seq_blks[idx].clk_sig] = rd_tally[ir_src->seq_blks[idx].clk_sig] + 0x1; }

        if (ir_src->seq_blks[idx].rst_sig != UINT32_MAX) { rd_tally[ir_src->seq_blks[idx].rst_sig] = rd_tally[ir_src->seq_blks[idx].rst_sig] + 0x1; }
    }

    for (pass = 0; pass < 64; pass++)
    {
        uint32_t any = 0;

        for (idx = 0; idx < ir_src->comb_cunt; idx++)
        {
            uint32_t ws = 0;

            if (dropped[idx] != 0x0) { continue; }

            if (comb2[idx].kind == CROSSTALK_HDL_IR_OP_MEM_WR) { continue; }

            ws = comb2[idx].dst;

            if (ws == UINT32_MAX) { continue; }

            if (strip_is != LDG_TRUTH_TRUE && ir_src->sigs[ws].name.len > 0) { continue; }

            if (ir_src->sigs[ws].port != CROSSTALK_HDL_IR_PORT_NONE) { continue; }

            if (rd_tally[ws] > 0) { continue; }

            dropped[idx] = 0x1;
            any = 0x1;

            rd_cunt = crosstalk_hdl_emit_rsigs_get(&comb2[idx], rd_arr);

            for (rd_i = 0; rd_i < rd_cunt; rd_i++) { rd_tally[rd_arr[rd_i]] = rd_tally[rd_arr[rd_i]] - 0x1; }

            if (comb2[idx].guard_is != 0x0 && comb2[idx].guard != UINT32_MAX) { rd_tally[comb2[idx].guard] = rd_tally[comb2[idx].guard] - 0x1; }
        }

        if (any == 0) { break; }
    }

    for (idx = 0; idx < ir_src->comb_cunt; idx++)
    {
        if (dropped[idx] != 0x0) { continue; }

        comb2[pack_i] = comb2[idx];
        pack_i++;
    }

    ir2->comb_ops = comb2;
    ir2->comb_cunt = pack_i;
    ir2->seq_ops = seq2;

    return LDG_ERR_AOK;
}

static uint8_t crosstalk_hdl_emit_guard_sig_is(const crosstalk_hdl_ir_t *ir_ctx, uint32_t blk_start, uint32_t blk_cunt, uint32_t sig)
{
    uint32_t at = 0;

    if (sig == UINT32_MAX) { return LDG_TRUTH_FALSE; }

    for (at = 0; at < blk_cunt; at++)
    {
        const crosstalk_hdl_ir_op_t *scan = &ir_ctx->seq_ops[blk_start + at];

        if (scan->guard_is == LDG_TRUTH_TRUE && scan->guard == sig) { return LDG_TRUTH_TRUE; }
    }

    return LDG_TRUTH_FALSE;
}

static uint8_t crosstalk_hdl_emit_hoist_is(const crosstalk_hdl_ir_t *ir_ctx, uint32_t blk_start, uint32_t blk_cunt, uint32_t op_at)
{
    const crosstalk_hdl_ir_op_t *op = &ir_ctx->seq_ops[op_at];

    if (op->guard_is != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    if (op->dst == UINT32_MAX) { return LDG_TRUTH_FALSE; }

    if (op->kind == CROSSTALK_HDL_IR_OP_REG_NEXT || op->kind == CROSSTALK_HDL_IR_OP_MEM_WR) { return LDG_TRUTH_FALSE; }

    if (op->kind == CROSSTALK_HDL_IR_OP_MUX && op->src0 == op->guard) { return LDG_TRUTH_FALSE; }

    return crosstalk_hdl_emit_guard_sig_is(ir_ctx, blk_start, blk_cunt, op->dst) == LDG_TRUTH_TRUE ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE;
}

static uint32_t crosstalk_hdl_emit_guard_open_emit(crosstalk_hdl_emit_ctx_t *ctx, uint32_t guard)
{
    uint32_t ret = crosstalk_hdl_str_emit(ctx, "if (");

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_vref_emit(ctx, guard);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_hdl_str_emit(ctx, " != 0x0)\n{\n");
}

static uint32_t crosstalk_hdl_emit_export_emit(crosstalk_hdl_emit_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_str_emit(ctx, "#ifndef HDL_GEN_MAIN\n#if defined(_WIN32)\n#define HDL_ABI_EXPORT __declspec(dllexport)\n#else\n#define HDL_ABI_EXPORT __attribute__((visibility(\"default\")))\n#endif\n\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "void *memcpy(void *dd, const void *ss, " "siz" "e_t nn)\n{\n" "unsig" "ned char *d2 = (" "unsig" "ned char *)dd;\nconst " "unsig" "ned char *s2 = (const " "unsig" "ned char *)ss;\n" "siz" "e_t i2;\nfor (i2 = 0; i2 < nn; i2++) { d2[i2] = s2[i2]; }\nreturn dd;\n}\n\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "void *memset(void *dd, " "in" "t cc, " "siz" "e_t nn)\n{\n" "unsig" "ned char *d2 = (" "unsig" "ned char *)dd;\n" "siz" "e_t i2;\nfor (i2 = 0; i2 < nn; i2++) { d2[i2] = (" "unsig" "ned char)cc; }\nreturn dd;\n}\n\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "in" "t strcmp(const char *aa, const char *bb)\n{\n" "siz" "e_t i2 = 0;\nwhile (aa[i2] != 0 && aa[i2] == bb[i2]) { i2++; }\nreturn (" "in" "t)((" "unsig" "ned char)aa[i2] - (" "unsig" "ned char)bb[i2]);\n}\n\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "HDL_ABI_EXPORT hdl_u32 hdl_abi_rev(void) { return 0x1; }\n\nHDL_ABI_EXPORT void hdl_abi_init(void) { hdl_gen_init(); }\n\nHDL_ABI_EXPORT hdl_u32 hdl_abi_cycle(hdl_u32 ck) { return hdl_gen_cycle(ck); }\n\nHDL_ABI_EXPORT hdl_u32 hdl_abi_sig_find(const char *snm) { return hdl_gen_sig_find(snm); }\n\nHDL_ABI_EXPORT void hdl_abi_poke(hdl_u32 i2, hdl_u64 pv) { hdl_gen_poke(i2, pv); }\n\nHDL_ABI_EXPORT hdl_u64 hdl_abi_peek(hdl_u32 i2) { return hdl_gen_peek(i2); }\n\nHDL_ABI_EXPORT void hdl_abi_mem_st(hdl_u32 i2, hdl_u64 ad, hdl_u64 sv) { hdl_gen_mem_st(i2, ad, sv); }\n\nHDL_ABI_EXPORT hdl_u64 hdl_abi_mem_ld(hdl_u32 i2, hdl_u64 ad) { return hdl_gen_mem_ld(i2, ad); }\n\nHDL_ABI_EXPORT void hdl_abi_touch(void) { hdl_gen_touch(); }\n\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->pf_is == LDG_TRUTH_TRUE) { return crosstalk_hdl_str_emit(ctx, "HDL_ABI_EXPORT hdl_u32 hdl_abi_state_get(hdl_u32 kd, hdl_u64 **ob, hdl_u32 *oc)\n{\nif (ob == 0x0 || oc == 0x0) { return 0x4; }\n*ob = 0x0;\n*oc = 0x0;\nif (kd == 0x0) { *ob = v; *oc = HDL_GEN_SIG_CUNT; return 0x0; }\nif (kd == 0x1) { *ob = m; *oc = HDL_GEN_MEM_WORDS; return 0x0; }\nif (kd == 0x2) { *ob = pc; *oc = HDL_GEN_BLK_CUNT; return 0x0; }\nif (kd == 0x3) { *ob = vh; *oc = HDL_GEN_SIG_CUNT; return 0x0; }\nif (kd == 0x4) { *ob = mh; *oc = HDL_GEN_MEM_WORDS; return 0x0; }\nif (kd == 0x5) { *ob = pr; *oc = HDL_GEN_BLK_CUNT; return 0x0; }\nif (kd == 0x6) { *ob = v; *oc = HDL_GEN_SIG_CUNT; return 0x0; }\nif (kd == 0x7) { *ob = pf_t; *oc = HDL_GEN_BLK_CUNT + 0x2; return 0x0; }\nif (kd == 0x8) { *ob = pf_c; *oc = HDL_GEN_BLK_CUNT + 0x2; return 0x0; }\nif (kd == 0x9) { *ob = pfr_tot; *oc = 4096; return 0x0; }\nif (kd == 0xA) { *ob = pfr_comb; *oc = 4096; return 0x0; }\nif (kd == 0xB) { *ob = pfr_blk; *oc = 4096; return 0x0; }\nif (kd == 0xC) { *ob = pfr_cmt; *oc = 4096; return 0x0; }\nif (kd == 0xD) { *ob = pfr_meta; *oc = 0x10; return 0x0; }\nif (kd == 0xE) { *ob = pf_s; *oc = 0x8; return 0x0; }\nif (kd == 0xF) { *ob = (hdl_u64 *)(void *)pf_nm; *oc = HDL_GEN_BLK_CUNT * 0x4; return 0x0; }\nreturn 0x4;\n}\n#endif\n\n"); }

    return crosstalk_hdl_str_emit(ctx, "HDL_ABI_EXPORT hdl_u32 hdl_abi_state_get(hdl_u32 kd, hdl_u64 **ob, hdl_u32 *oc)\n{\nif (ob == 0x0 || oc == 0x0) { return 0x4; }\n*ob = 0x0;\n*oc = 0x0;\nif (kd == 0x0) { *ob = v; *oc = HDL_GEN_SIG_CUNT; return 0x0; }\nif (kd == 0x1) { *ob = m; *oc = HDL_GEN_MEM_WORDS; return 0x0; }\nif (kd == 0x2) { *ob = pc; *oc = HDL_GEN_BLK_CUNT; return 0x0; }\nif (kd == 0x3) { *ob = vh; *oc = HDL_GEN_SIG_CUNT; return 0x0; }\nif (kd == 0x4) { *ob = mh; *oc = HDL_GEN_MEM_WORDS; return 0x0; }\nif (kd == 0x5) { *ob = pr; *oc = HDL_GEN_BLK_CUNT; return 0x0; }\nif (kd == 0x6) { *ob = v; *oc = HDL_GEN_SIG_CUNT; return 0x0; }\nreturn 0x4;\n}\n#endif\n\n");
}

uint32_t crosstalk_hdl_emit_run(crosstalk_hdl_emit_ctx_t *ctx, const crosstalk_hdl_ir_t *ir_ctx, const char *src, void *pool, uint8_t strip_is, uint8_t pf_is, char **out_src, uint32_t *out_len)
{
    crosstalk_hdl_ir_t ir2;
    void *raw = 0x0;
    uint64_t cap = 0;
    uint64_t words = 0;
    uint32_t idx = 0;
    uint32_t op_idx = 0;
    uint32_t hex_cunt = 0;
    uint32_t alias_arr[CROSSTALK_HDL_EMIT_ALIAS_CAP];
    uint8_t *rcls = 0x0;
    uint8_t *konst_ok = 0x0;
    uint32_t *konst_wr = 0x0;
    uint32_t krd[4];
    uint32_t alias_cunt = 0;
    uint32_t alias_i = 0;
    uint32_t clk_root = UINT32_MAX;
    uint32_t fast_ok = 0;
    uint32_t pf_core_cunt = 0;
    uint32_t cur_rgn = UINT32_MAX;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || ir_ctx == 0x0 || src == 0x0 || pool == 0x0 || out_src == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_src = 0x0;
    *out_len = 0;

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->pf_is = pf_is;

    ret = crosstalk_hdl_emit_opt_run(&ir2, ir_ctx, pool, strip_is);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ir_ctx = &ir2;
    ctx->ir_ctx = ir_ctx;
    ctx->src = src;
    ctx->pool = pool;
    ctx->vf_op = UINT32_MAX;
    ctx->vf_sig = UINT32_MAX;

    cap = 32768 + (uint64_t)ir_ctx->sig_cunt * 256 + (uint64_t)ir_ctx->comb_cunt * 256 + (uint64_t)ir_ctx->seq_op_cunt * 256 + (uint64_t)ir_ctx->seq_blk_cunt * 1024;

    if (LDG_UNLIKELY(cap > UINT32_MAX)) { return LDG_ERR_OVERFLOW; }

    ret = ldg_mem_pool_alloc(pool, cap, &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->out = (char *)raw;
    ctx->out_cap = (uint32_t)cap - LDG_STR_TERM_SIZE;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt) * sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->mem_off = (uint32_t *)raw;

    for (idx = 0; idx < ir_ctx->sig_cunt; idx++)
    {
        ctx->mem_off[idx] = UINT32_MAX;

        if (ir_ctx->sigs[idx].width > 64) { ctx->wide_has = LDG_TRUTH_TRUE; }

        if (ir_ctx->sigs[idx].depth > 0)
        {
            ctx->mem_off[idx] = (uint32_t)words;
            words = words + (uint64_t)ir_ctx->sigs[idx].depth;

            if (ir_ctx->sigs[idx].mem_file.len > 2) { hex_cunt++; }
        }
    }

    ctx->mem_words = (uint32_t)words;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->comb_cunt == 0 ? 0x1 : ir_ctx->comb_cunt) * sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->order = (uint32_t *)raw;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->comb_cunt == 0 ? 0x1 : ir_ctx->comb_cunt) * sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->residue = (uint32_t *)raw;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->comb_cunt == 0 ? 0x1 : ir_ctx->comb_cunt) * CROSSTALK_HDL_EMIT_DEPS_PER_OP * sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->deps = (uint32_t *)raw;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->comb_cunt == 0 ? 0x1 : ir_ctx->comb_cunt), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->placed = (uint8_t *)raw;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->seq_op_cunt == 0 ? 0x1 : ir_ctx->seq_op_cunt) * sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->rn_slot = (uint32_t *)raw;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->seq_op_cunt == 0 ? 0x1 : ir_ctx->seq_op_cunt) * sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->mw_slot = (uint32_t *)raw;

    for (idx = 0; idx < ir_ctx->seq_op_cunt; idx++)
    {
        ctx->rn_slot[idx] = UINT32_MAX;
        ctx->mw_slot[idx] = UINT32_MAX;

        if (ir_ctx->seq_ops[idx].kind == CROSSTALK_HDL_IR_OP_REG_NEXT)
        {
            ctx->rn_slot[idx] = ctx->rn_cunt;
            ctx->rn_cunt++;
        }

        if (ir_ctx->seq_ops[idx].kind == CROSSTALK_HDL_IR_OP_MEM_WR)
        {
            ctx->mw_slot[idx] = ctx->mw_cunt;
            ctx->mw_cunt++;
        }
    }

    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->seq_op_cunt == 0 ? 0x1 : ir_ctx->seq_op_cunt) * sizeof(uint8_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->rn_direct = (uint8_t *)raw;

    for (idx = 0; idx < ir_ctx->seq_op_cunt; idx++) { ctx->rn_direct[idx] = LDG_TRUTH_FALSE; }

    {
        uint32_t *rd_max = 0x0;
        uint32_t *rd_blk = 0x0;
        uint32_t *blk_of = 0x0;
        uint32_t *rn_wr_cunt = 0x0;
        uint32_t blk_i = 0;
        uint32_t blk_op = 0;

        ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt) * sizeof(uint32_t), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        rd_max = (uint32_t *)raw;

        ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt) * sizeof(uint32_t), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        rd_blk = (uint32_t *)raw;

        ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt) * sizeof(uint32_t), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        rn_wr_cunt = (uint32_t *)raw;

        ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->seq_op_cunt == 0 ? 0x1 : ir_ctx->seq_op_cunt) * sizeof(uint32_t), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        blk_of = (uint32_t *)raw;

        for (idx = 0; idx < ir_ctx->sig_cunt; idx++)
        {
            rd_max[idx] = UINT32_MAX;
            rd_blk[idx] = UINT32_MAX;
            rn_wr_cunt[idx] = 0;
        }

        for (idx = 0; idx < ir_ctx->seq_op_cunt; idx++) { blk_of[idx] = UINT32_MAX; }

        for (blk_i = 0; blk_i < ir_ctx->seq_blk_cunt; blk_i++) { for (blk_op = 0; blk_op < ir_ctx->seq_blks[blk_i].op_cunt; blk_op++)
            {
                uint32_t at = ir_ctx->seq_blks[blk_i].op_start + blk_op;
                if (at < ir_ctx->seq_op_cunt) { blk_of[at] = blk_i; }
            }
        }

        for (idx = 0; idx < ir_ctx->seq_op_cunt; idx++)
        {
            const crosstalk_hdl_ir_op_t *sop = &ir_ctx->seq_ops[idx];
            uint32_t rds[4];
            uint32_t rd_i = 0;

            rds[0] = sop->src0;
            rds[1] = sop->src1;
            rds[2] = sop->src2;
            rds[3] = sop->kind == CROSSTALK_HDL_IR_OP_MEM_WR ? sop->dst : sop->guard;

            for (rd_i = 0; rd_i < 0x4; rd_i++)
            {
                if (rds[rd_i] >= ir_ctx->sig_cunt) { continue; }

                if (rd_max[rds[rd_i]] == UINT32_MAX || rd_max[rds[rd_i]] < idx) { rd_max[rds[rd_i]] = idx; }

                if (rd_blk[rds[rd_i]] == UINT32_MAX) { rd_blk[rds[rd_i]] = blk_of[idx]; }
                else if (rd_blk[rds[rd_i]] != blk_of[idx]) { rd_blk[rds[rd_i]] = CROSSTALK_HDL_EMIT_BLK_MANY; }
            }

            if (sop->guard < ir_ctx->sig_cunt && sop->kind == CROSSTALK_HDL_IR_OP_MEM_WR)
            {
                if (rd_max[sop->guard] == UINT32_MAX || rd_max[sop->guard] < idx) { rd_max[sop->guard] = idx; }

                if (rd_blk[sop->guard] == UINT32_MAX) { rd_blk[sop->guard] = blk_of[idx]; }
                else if (rd_blk[sop->guard] != blk_of[idx]) { rd_blk[sop->guard] = CROSSTALK_HDL_EMIT_BLK_MANY; }
            }

            if (sop->kind == CROSSTALK_HDL_IR_OP_REG_NEXT && sop->dst < ir_ctx->sig_cunt) { rn_wr_cunt[sop->dst]++; }
        }

        for (idx = 0; idx < ir_ctx->seq_op_cunt; idx++)
        {
            const crosstalk_hdl_ir_op_t *sop = &ir_ctx->seq_ops[idx];

            if (sop->kind != CROSSTALK_HDL_IR_OP_REG_NEXT) { continue; }

            if (sop->dst >= ir_ctx->sig_cunt) { continue; }

            if (rn_wr_cunt[sop->dst] != 0x1) { continue; }

            if (ir_ctx->sigs[sop->dst].depth > 0) { continue; }

            if (crosstalk_hdl_emit_op_wide_is(ir_ctx, sop) == LDG_TRUTH_TRUE) { continue; }

            if (rd_max[sop->dst] != UINT32_MAX)
            {
                if (rd_blk[sop->dst] != blk_of[idx]) { continue; }

                if (rd_max[sop->dst] >= idx) { continue; }
            }

            ctx->rn_direct[idx] = LDG_TRUTH_TRUE;
        }
    }

    ret = crosstalk_hdl_emit_topo_build(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->comb_cunt == 0 ? 0x1 : ir_ctx->comb_cunt), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    rcls = (uint8_t *)raw;

    {
        uint8_t *ract = 0x0;
        uint8_t *sig_wact = 0x0;
        uint8_t *sig_ract = 0x0;
        uint32_t *rwr_cunt = 0x0;
        uint32_t *res2 = 0x0;
        uint32_t rd_arr5[4];
        uint32_t rd_cunt5 = 0;
        uint32_t rd_k = 0;
        uint32_t sweep2 = 0;
        uint32_t out_i = 0;
        uint32_t cls_i = 0;

        ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->comb_cunt == 0 ? 0x1 : ir_ctx->comb_cunt), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ract = (uint8_t *)raw;

        ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        sig_wact = (uint8_t *)raw;

        ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        sig_ract = (uint8_t *)raw;

        ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt) * sizeof(uint32_t), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        rwr_cunt = (uint32_t *)raw;

        ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->comb_cunt == 0 ? 0x1 : ir_ctx->comb_cunt) * sizeof(uint32_t), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        res2 = (uint32_t *)raw;

        ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->comb_cunt == 0 ? 0x1 : ir_ctx->comb_cunt) * sizeof(uint32_t), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        {
            uint32_t *peel_sw = (uint32_t *)raw;

            for (idx = 0; idx < ir_ctx->comb_cunt; idx++) { rcls[idx] = 0x1; ract[idx] = 0x0; peel_sw[idx] = 0; }

            for (idx = 0; idx < ir_ctx->sig_cunt; idx++) { rwr_cunt[idx] = 0; }

            for (idx = 0; idx < ctx->residue_cunt; idx++)
            {
                uint32_t rws = crosstalk_hdl_emit_wsig_get(&ir_ctx->comb_ops[ctx->residue[idx]]);

                ract[ctx->residue[idx]] = 0x1;

                if (rws != UINT32_MAX) { rwr_cunt[rws] = rwr_cunt[rws] + 0x1; }
            }

            for (sweep2 = 0; sweep2 < 256; sweep2++)
            {
                uint32_t peeled = 0;

                for (idx = 0; idx < ir_ctx->sig_cunt; idx++) { sig_wact[idx] = 0x0; sig_ract[idx] = 0x0; }

                for (idx = 0; idx < ctx->residue_cunt; idx++)
                {
                    uint32_t aop = ctx->residue[idx];
                    uint32_t aws = 0;

                    if (ract[aop] == 0x0) { continue; }

                    aws = crosstalk_hdl_emit_wsig_get(&ir_ctx->comb_ops[aop]);

                    if (aws != UINT32_MAX) { sig_wact[aws] = 0x1; }

                    rd_cunt5 = crosstalk_hdl_emit_rsigs_get(&ir_ctx->comb_ops[aop], rd_arr5);

                    for (rd_k = 0; rd_k < rd_cunt5; rd_k++) { sig_ract[rd_arr5[rd_k]] = 0x1; }
                }

                for (idx = 0; idx < ctx->residue_cunt; idx++)
                {
                    uint32_t aop = ctx->residue[idx];
                    uint32_t aws = 0;
                    uint8_t hd_is = 0x1;

                    if (ract[aop] == 0x0) { continue; }

                    aws = crosstalk_hdl_emit_wsig_get(&ir_ctx->comb_ops[aop]);

                    if (aws != UINT32_MAX && rwr_cunt[aws] > 0x1) { continue; }

                    rd_cunt5 = crosstalk_hdl_emit_rsigs_get(&ir_ctx->comb_ops[aop], rd_arr5);

                    for (rd_k = 0; rd_k < rd_cunt5; rd_k++)
                    {
                        if (rd_arr5[rd_k] == aws) { continue; }

                        if (sig_wact[rd_arr5[rd_k]] != 0x0) { hd_is = 0x0; }
                    }

                    if (hd_is != 0x0)
                    {
                        rcls[aop] = 0x0;
                        ract[aop] = 0x0;
                        peel_sw[aop] = sweep2;
                        peeled = 0x1;
                        continue;
                    }

                    if (aws == UINT32_MAX || sig_ract[aws] == 0x0)
                    {
                        rcls[aop] = 0x2;
                        ract[aop] = 0x0;
                        peel_sw[aop] = sweep2;
                        peeled = 0x1;
                    }
                }

                if (peeled == 0) { break; }
            }

            out_i = 0;

            for (cls_i = 0; cls_i < 256; cls_i++) { for (idx = 0; idx < ctx->residue_cunt; idx++)
                {
                    if (rcls[ctx->residue[idx]] != 0x0 || peel_sw[ctx->residue[idx]] != cls_i) { continue; }

                    res2[out_i] = ctx->residue[idx];
                    out_i++;
                }
            }

            for (idx = 0; idx < ctx->residue_cunt; idx++)
            {
                if (rcls[ctx->residue[idx]] != 0x1) { continue; }

                res2[out_i] = ctx->residue[idx];
                out_i++;
            }

            cls_i = 256;

            while (cls_i > 0)
            {
                cls_i--;

                for (idx = 0; idx < ctx->residue_cunt; idx++)
                {
                    if (rcls[ctx->residue[idx]] != 0x2 || peel_sw[ctx->residue[idx]] != cls_i) { continue; }

                    res2[out_i] = ctx->residue[idx];
                    out_i++;
                }
            }

            for (idx = 0; idx < ctx->residue_cunt; idx++) { ctx->residue[idx] = res2[idx]; }
        }
    }

    ret = crosstalk_hdl_emit_fast_analyze(ctx, alias_arr, &alias_cunt, &clk_root, &fast_ok);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->fast_ok = fast_ok;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->comb_cunt == 0 ? 0x1 : ir_ctx->comb_cunt), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->fused = (uint8_t *)raw;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt) * sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->fuse_map = (uint32_t *)raw;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->seq_op_cunt == 0 ? 0x1 : ir_ctx->seq_op_cunt), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->fused_seq = (uint8_t *)raw;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt) * sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->fuse_map_seq = (uint32_t *)raw;

    for (idx = 0; idx < ir_ctx->comb_cunt; idx++) { ctx->fused[idx] = 0x0; }

    for (idx = 0; idx < ir_ctx->seq_op_cunt; idx++) { ctx->fused_seq[idx] = 0x0; }

    for (idx = 0; idx < ir_ctx->sig_cunt; idx++) { ctx->fuse_map[idx] = UINT32_MAX; ctx->fuse_map_seq[idx] = UINT32_MAX; }

    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->guard_need = (uint8_t *)raw;

    memset(ctx->guard_need, 0x0, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt));

    {
        uint32_t blk_at = 0;
        uint32_t gv_at = 0;

        for (blk_at = 0; blk_at < ir_ctx->seq_blk_cunt; blk_at++)
        {
            uint32_t blk_guards[8];
            uint32_t blk_guard_cunt = 0;

            if (crosstalk_hdl_emit_blk_guard_get(ctx, blk_at, blk_guards, &blk_guard_cunt) != LDG_TRUTH_TRUE) { continue; }

            for (gv_at = 0; gv_at < blk_guard_cunt; gv_at++) { ctx->guard_need[blk_guards[gv_at]] = 0x1; }
        }
    }

    {
        uint32_t g_at = 0;

        for (g_at = 0; g_at < ir_ctx->comb_cunt; g_at++)
        {
            const crosstalk_hdl_ir_op_t *gop = &ir_ctx->comb_ops[g_at];

            if (gop->guard_is != 0x0 && gop->guard < ir_ctx->sig_cunt) { ctx->guard_need[gop->guard] = 0x1; }
        }

        for (g_at = 0; g_at < ir_ctx->seq_op_cunt; g_at++)
        {
            const crosstalk_hdl_ir_op_t *gop = &ir_ctx->seq_ops[g_at];

            if (gop->guard_is != 0x0 && gop->guard < ir_ctx->sig_cunt) { ctx->guard_need[gop->guard] = 0x1; }
        }
    }

    {
        uint32_t *fu_rdt = 0x0;
        uint32_t *fu_wrt = 0x0;
        uint32_t *fu_wop = 0x0;
        uint32_t rd_arr4[4];
        uint32_t rd_cunt4 = 0;
        uint32_t rd_j = 0;

        ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt) * sizeof(uint32_t), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        fu_rdt = (uint32_t *)raw;

        ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt) * sizeof(uint32_t), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        fu_wrt = (uint32_t *)raw;

        ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt) * sizeof(uint32_t), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        fu_wop = (uint32_t *)raw;

        for (idx = 0; idx < ir_ctx->sig_cunt; idx++) { fu_rdt[idx] = 0; fu_wrt[idx] = 0; fu_wop[idx] = UINT32_MAX; }

        for (idx = 0; idx < ir_ctx->comb_cunt; idx++)
        {
            uint32_t ws4 = crosstalk_hdl_emit_wsig_get(&ir_ctx->comb_ops[idx]);

            if (ws4 != UINT32_MAX) { fu_wrt[ws4] = fu_wrt[ws4] + 0x1; fu_wop[ws4] = idx; }

            rd_cunt4 = crosstalk_hdl_emit_rsigs_get(&ir_ctx->comb_ops[idx], rd_arr4);

            for (rd_j = 0; rd_j < rd_cunt4; rd_j++) { fu_rdt[rd_arr4[rd_j]] = fu_rdt[rd_arr4[rd_j]] + 0x1; }
        }

        for (idx = 0; idx < ir_ctx->seq_op_cunt; idx++)
        {
            rd_cunt4 = crosstalk_hdl_emit_rsigs_get(&ir_ctx->seq_ops[idx], rd_arr4);

            for (rd_j = 0; rd_j < rd_cunt4; rd_j++) { fu_rdt[rd_arr4[rd_j]] = fu_rdt[rd_arr4[rd_j]] + 0x1; }
        }

        for (idx = 0; idx < ir_ctx->seq_blk_cunt; idx++)
        {
            if (ir_ctx->seq_blks[idx].clk_sig != UINT32_MAX) { fu_rdt[ir_ctx->seq_blks[idx].clk_sig] = fu_rdt[ir_ctx->seq_blks[idx].clk_sig] + 0x1; }

            if (ir_ctx->seq_blks[idx].rst_sig != UINT32_MAX) { fu_rdt[ir_ctx->seq_blks[idx].rst_sig] = fu_rdt[ir_ctx->seq_blks[idx].rst_sig] + 0x1; }
        }

        crosstalk_hdl_emit_fuse_mark(ctx, ctx->order, ctx->order_cunt, fu_rdt, fu_wrt);
        crosstalk_hdl_emit_fuse_mark(ctx, ctx->residue, ctx->residue_cunt, fu_rdt, fu_wrt);

        ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt) * sizeof(uint32_t), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        {
            uint32_t *fu_swrt = (uint32_t *)raw;

            for (idx = 0; idx < ir_ctx->sig_cunt; idx++) { fu_swrt[idx] = 0; }

            for (idx = 0; idx < ir_ctx->seq_op_cunt; idx++)
            {
                const crosstalk_hdl_ir_op_t *sop = &ir_ctx->seq_ops[idx];

                if (sop->kind == CROSSTALK_HDL_IR_OP_MEM_WR || sop->kind == CROSSTALK_HDL_IR_OP_REG_NEXT) { continue; }

                if (sop->dst != UINT32_MAX) { fu_swrt[sop->dst] = fu_swrt[sop->dst] + 0x1; }
            }

            for (idx = 0; idx < ir_ctx->seq_blk_cunt; idx++) { crosstalk_hdl_emit_fuse_seq_mark(ctx, ir_ctx->seq_blks[idx].op_start, ir_ctx->seq_blks[idx].op_cunt, fu_rdt, fu_wrt, fu_swrt); }
        }

        ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ctx->cst_has = (uint8_t *)raw;

        ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt) * sizeof(uint64_t), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ctx->cst_val = (uint64_t *)raw;

        ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        {
            uint8_t *seq_aw = (uint8_t *)raw;

            for (idx = 0; idx < ir_ctx->sig_cunt; idx++) { seq_aw[idx] = 0x0; ctx->cst_has[idx] = 0x0; ctx->cst_val[idx] = 0; }

            for (idx = 0; idx < ir_ctx->seq_op_cunt; idx++)
            {
                uint32_t ws5 = crosstalk_hdl_emit_wsig_get(&ir_ctx->seq_ops[idx]);

                if (ws5 != UINT32_MAX) { seq_aw[ws5] = 0x1; }
            }

            for (idx = 0; idx < ir_ctx->sig_cunt; idx++)
            {
                if (ir_ctx->sigs[idx].port != CROSSTALK_HDL_IR_PORT_NONE || ir_ctx->sigs[idx].depth > 0 || seq_aw[idx] != 0x0) { continue; }

                if (ir_ctx->sigs[idx].width > 64) { continue; }

                if (fu_wrt[idx] == 0)
                {
                    ctx->cst_has[idx] = 0x1;
                    ctx->cst_val[idx] = ir_ctx->sigs[idx].init_val & crosstalk_hdl_emit_mask_get(ir_ctx->sigs[idx].width);
                }
                else if (fu_wrt[idx] == 0x1 && ir_ctx->comb_ops[fu_wop[idx]].kind == CROSSTALK_HDL_IR_OP_CONST)
                {
                    ctx->cst_has[idx] = 0x1;
                    ctx->cst_val[idx] = ir_ctx->comb_ops[fu_wop[idx]].imm & crosstalk_hdl_emit_mask_get(ir_ctx->sigs[idx].width);
                }
            }
        }

        ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->comb_cunt == 0 ? 0x1 : ir_ctx->comb_cunt) * sizeof(uint32_t), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ctx->vf_prev = (uint32_t *)raw;

        ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->comb_cunt == 0 ? 0x1 : ir_ctx->comb_cunt) * sizeof(uint32_t), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        {
            uint32_t *ver_rdt = (uint32_t *)raw;
            uint32_t *ver_rdr = 0x0;
            uint32_t *cur_wr = 0x0;

            ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->comb_cunt == 0 ? 0x1 : ir_ctx->comb_cunt) * sizeof(uint32_t), &raw);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ver_rdr = (uint32_t *)raw;

            ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt) * sizeof(uint32_t), &raw);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            cur_wr = (uint32_t *)raw;

            for (idx = 0; idx < ir_ctx->comb_cunt; idx++) { ctx->vf_prev[idx] = UINT32_MAX; ver_rdt[idx] = 0; ver_rdr[idx] = UINT32_MAX; }

            for (idx = 0; idx < ir_ctx->sig_cunt; idx++) { cur_wr[idx] = UINT32_MAX; }

            for (idx = 0; idx < ir_ctx->comb_cunt; idx++)
            {
                uint32_t ws6 = crosstalk_hdl_emit_wsig_get(&ir_ctx->comb_ops[idx]);

                rd_cunt4 = crosstalk_hdl_emit_rsigs_get(&ir_ctx->comb_ops[idx], rd_arr4);

                for (rd_j = 0; rd_j < rd_cunt4; rd_j++)
                {
                    uint32_t vw = cur_wr[rd_arr4[rd_j]];

                    if (vw != UINT32_MAX) { ver_rdt[vw] = ver_rdt[vw] + 0x1; ver_rdr[vw] = idx; }
                }

                if (ws6 != UINT32_MAX) { cur_wr[ws6] = idx; }
            }

            crosstalk_hdl_emit_rmw_mark(ctx, ctx->order, ctx->order_cunt, ver_rdt, ver_rdr);
            crosstalk_hdl_emit_rmw_mark(ctx, ctx->residue, ctx->residue_cunt, ver_rdt, ver_rdr);

            ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->comb_cunt == 0 ? 0x1 : ir_ctx->comb_cunt), &raw);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ctx->loc_ok = (uint8_t *)raw;

            ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt) * sizeof(uint32_t), &raw);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ctx->loc_act = (uint32_t *)raw;

            ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->comb_cunt == 0 ? 0x1 : ir_ctx->comb_cunt) * sizeof(uint32_t), &raw);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            {
                uint32_t *pos_of = (uint32_t *)raw;
                uint8_t *strm_of = 0x0;
                uint8_t *cwrap = 0x0;
                uint32_t *nxw = 0x0;
                uint32_t *last_slot = 0x0;
                uint32_t bk_i = 0;

                ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->comb_cunt == 0 ? 0x1 : ir_ctx->comb_cunt), &raw);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                strm_of = (uint8_t *)raw;

                ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->comb_cunt == 0 ? 0x1 : ir_ctx->comb_cunt) * sizeof(uint32_t), &raw);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                nxw = (uint32_t *)raw;

                ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt) * sizeof(uint32_t), &raw);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                last_slot = (uint32_t *)raw;

                ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt), &raw);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                cwrap = (uint8_t *)raw;

                for (idx = 0; idx < ir_ctx->sig_cunt; idx++) { cwrap[idx] = 0x0; }

                for (idx = 0; idx < ir_ctx->comb_cunt; idx++) { ctx->loc_ok[idx] = 0x0; pos_of[idx] = 0; strm_of[idx] = 0x0; nxw[idx] = UINT32_MAX; }

                for (idx = 0; idx < ir_ctx->sig_cunt; idx++) { ctx->loc_act[idx] = UINT32_MAX; }

                for (idx = 0; idx < ctx->order_cunt; idx++) { pos_of[ctx->order[idx]] = idx; strm_of[ctx->order[idx]] = 0x0; }

                for (idx = 0; idx < ctx->residue_cunt; idx++) { pos_of[ctx->residue[idx]] = idx; strm_of[ctx->residue[idx]] = 0x1; }

                for (idx = 0; idx < ir_ctx->sig_cunt; idx++) { last_slot[idx] = UINT32_MAX; }

                bk_i = ctx->order_cunt;

                while (bk_i > 0)
                {
                    uint32_t bws = 0;

                    bk_i--;
                    bws = crosstalk_hdl_emit_wsig_get(&ir_ctx->comb_ops[ctx->order[bk_i]]);

                    if (bws != UINT32_MAX) { nxw[ctx->order[bk_i]] = last_slot[bws]; last_slot[bws] = bk_i; }
                }

                for (idx = 0; idx < ir_ctx->sig_cunt; idx++) { last_slot[idx] = UINT32_MAX; }

                bk_i = ctx->residue_cunt;

                while (bk_i > 0)
                {
                    uint32_t bws = 0;

                    bk_i--;
                    bws = crosstalk_hdl_emit_wsig_get(&ir_ctx->comb_ops[ctx->residue[bk_i]]);

                    if (bws != UINT32_MAX) { nxw[ctx->residue[bk_i]] = last_slot[bws]; last_slot[bws] = bk_i; }
                }

                for (idx = 0; idx < ir_ctx->comb_cunt; idx++)
                {
                    const crosstalk_hdl_ir_op_t *lop = &ir_ctx->comb_ops[idx];

                    if (ctx->fused[idx] != 0x0) { continue; }

                    if (lop->kind == CROSSTALK_HDL_IR_OP_MEM_WR || lop->dst == UINT32_MAX) { continue; }

                    if (ctx->guard_need[lop->dst] != 0x0) { continue; }

                    if (ir_ctx->sigs[lop->dst].depth > 0) { continue; }

                    if (crosstalk_hdl_emit_op_wide_is(ir_ctx, lop) == LDG_TRUTH_TRUE) { continue; }

                    if (lop->kind == CROSSTALK_HDL_IR_OP_CONST) { continue; }

                    if (nxw[idx] == UINT32_MAX && (ir_ctx->sigs[lop->dst].name.len > 0 || ir_ctx->sigs[lop->dst].port != CROSSTALK_HDL_IR_PORT_NONE)) { continue; }

                    ctx->loc_ok[idx] = 0x1;
                }

                {
                    uint32_t *wrs_cunt = 0x0;
                    uint32_t *wrs_seen = 0x0;
                    uint32_t *epos = 0x0;
                    uint32_t *fstk = 0x0;
                    uint32_t sect_at[3];
                    uint32_t ep_cunt = 0;
                    uint32_t rgn_fill = 0;
                    uint32_t rgn_id = 0;
                    uint32_t open_cunt = 0;
                    uint32_t cls_j = 0;
                    uint32_t p2 = 0;
                    uint32_t sect_j = 0;

                    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt) * sizeof(uint32_t), &raw);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    wrs_cunt = (uint32_t *)raw;

                    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt) * sizeof(uint32_t), &raw);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    wrs_seen = (uint32_t *)raw;

                    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->comb_cunt == 0 ? 0x1 : ir_ctx->comb_cunt) * sizeof(uint32_t), &raw);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    epos = (uint32_t *)raw;

                    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->comb_cunt == 0 ? 0x1 : ir_ctx->comb_cunt) * sizeof(uint32_t), &raw);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    fstk = (uint32_t *)raw;

                    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->comb_cunt == 0 ? 0x1 : ir_ctx->comb_cunt) * sizeof(uint32_t), &raw);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    ctx->rgn_of = (uint32_t *)raw;

                    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt), &raw);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    ctx->rgn_wspan = (uint8_t *)raw;

                    for (p2 = 0; p2 < ctx->order_cunt; p2++) { epos[ep_cunt] = ctx->order[p2]; ep_cunt++; }

                    sect_at[0] = ep_cunt;

                    for (cls_j = 0; cls_j < 0x3; cls_j++)
                    {
                        for (p2 = 0; p2 < ctx->residue_cunt; p2++)
                        {
                            if (rcls[ctx->residue[p2]] != cls_j) { continue; }

                            epos[ep_cunt] = ctx->residue[p2];
                            ep_cunt++;
                        }

                        if (cls_j < 0x2) { sect_at[cls_j + 0x1] = ep_cunt; }
                    }

                    for (p2 = 0; p2 < ir_ctx->sig_cunt; p2++) { wrs_cunt[p2] = 0; wrs_seen[p2] = 0; ctx->rgn_wspan[p2] = 0x0; }

                    for (p2 = 0; p2 < ep_cunt; p2++)
                    {
                        const crosstalk_hdl_ir_op_t *wop = &ir_ctx->comb_ops[epos[p2]];
                        uint32_t ws8 = crosstalk_hdl_emit_wsig_get(wop);

                        if (ctx->fused[epos[p2]] != 0x0 || wop->kind == CROSSTALK_HDL_IR_OP_MEM_WR || ws8 == UINT32_MAX) { continue; }

                        wrs_cunt[ws8] = wrs_cunt[ws8] + 0x1;
                    }

                    ctx->rgn_core_lo = UINT32_MAX;
                    ctx->rgn_core_hi = 0;
                    sect_j = 0;

                    for (p2 = 0; p2 < ep_cunt; p2++)
                    {
                        const crosstalk_hdl_ir_op_t *wop = &ir_ctx->comb_ops[epos[p2]];
                        uint32_t ws8 = crosstalk_hdl_emit_wsig_get(wop);
                        uint8_t bound_is = 0x0;

                        while (sect_j < 0x3 && p2 == sect_at[sect_j]) { bound_is = 0x1; sect_j++; }

                        if (bound_is != 0x0 && rgn_fill > 0) { rgn_id++; rgn_fill = 0; }

                        if (rgn_fill >= CROSSTALK_HDL_EMIT_RGN_OPS && open_cunt == 0) { rgn_id++; rgn_fill = 0; }

                        ctx->rgn_of[epos[p2]] = rgn_id;

                        if (ctx->fused[epos[p2]] == 0x0) { rgn_fill++; }

                        if (sect_j == 0x2 && ctx->fused[epos[p2]] == 0x0)
                        {
                            if (rgn_id < ctx->rgn_core_lo) { ctx->rgn_core_lo = rgn_id; }

                            if (rgn_id + 0x1 > ctx->rgn_core_hi) { ctx->rgn_core_hi = rgn_id + 0x1; }
                        }

                        if (ctx->fused[epos[p2]] != 0x0 || wop->kind == CROSSTALK_HDL_IR_OP_MEM_WR || ws8 == UINT32_MAX) { continue; }

                        if (wrs_cunt[ws8] < 0x2) { continue; }

                        wrs_seen[ws8] = wrs_seen[ws8] + 0x1;

                        if (wrs_seen[ws8] == 0x1) { open_cunt++; }

                        if (wrs_seen[ws8] == wrs_cunt[ws8]) { open_cunt--; }
                    }

                    ctx->rgn_cunt = rgn_fill > 0 || rgn_id > 0 ? rgn_id + 0x1 : 0x0;

                    if (ctx->rgn_core_lo == UINT32_MAX) { ctx->rgn_core_lo = 0; ctx->rgn_core_hi = 0; }

                    ctx->rw_cunt = ctx->rgn_cunt == 0 ? 0x1 : (ctx->rgn_cunt + 63) / 64;

                    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt) * (uint64_t)ctx->rw_cunt * sizeof(uint64_t), &raw);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    ctx->rgn_rd = (uint64_t *)raw;

                    for (p2 = 0; p2 < ir_ctx->sig_cunt * ctx->rw_cunt; p2++) { ctx->rgn_rd[p2] = 0; }

                    for (p2 = 0; p2 < ep_cunt; p2++)
                    {
                        uint32_t stk_n = 0;
                        uint32_t rgn_bit = 0;

                        if (ctx->fused[epos[p2]] != 0x0) { continue; }

                        rgn_bit = ctx->rgn_of[epos[p2]];
                        fstk[stk_n] = epos[p2];
                        stk_n++;

                        while (stk_n > 0)
                        {
                            uint32_t rd_arr8[4];
                            uint32_t rd_cunt8 = 0;
                            uint32_t rd_k8 = 0;
                            uint32_t cur8 = 0;

                            stk_n--;
                            cur8 = fstk[stk_n];
                            rd_cunt8 = crosstalk_hdl_emit_rsigs_get(&ir_ctx->comb_ops[cur8], rd_arr8);

                            for (rd_k8 = 0; rd_k8 < rd_cunt8; rd_k8++)
                            {
                                uint32_t rs8 = rd_arr8[rd_k8];

                                if (ctx->fuse_map[rs8] != UINT32_MAX && stk_n < ir_ctx->comb_cunt)
                                {
                                    fstk[stk_n] = ctx->fuse_map[rs8];
                                    stk_n++;
                                    continue;
                                }

                                ctx->rgn_rd[(uint64_t)rs8 * ctx->rw_cunt + (rgn_bit / 64)] |= (uint64_t)0x1 << (rgn_bit & 63);
                            }
                        }
                    }

                    for (p2 = 0; p2 < ep_cunt; p2++)
                    {
                        const crosstalk_hdl_ir_op_t *wop = &ir_ctx->comb_ops[epos[p2]];
                        uint32_t ws8 = crosstalk_hdl_emit_wsig_get(wop);

                        if (ctx->fused[epos[p2]] != 0x0 || wop->kind == CROSSTALK_HDL_IR_OP_MEM_WR || ws8 == UINT32_MAX) { continue; }

                        if (wrs_cunt[ws8] >= 0x2) { ctx->rgn_wspan[ws8] = 0x1; }
                    }

                    for (p2 = 0; p2 < ep_cunt; p2++)
                    {
                        const crosstalk_hdl_ir_op_t *wop = &ir_ctx->comb_ops[epos[p2]];
                        uint32_t ws8 = crosstalk_hdl_emit_wsig_get(wop);
                        uint32_t rgn_bit = 0;

                        if (ctx->fused[epos[p2]] != 0x0 || wop->kind == CROSSTALK_HDL_IR_OP_MEM_WR || ws8 == UINT32_MAX) { continue; }

                        if (ctx->rgn_wspan[ws8] == 0x0) { continue; }

                        rgn_bit = ctx->rgn_of[epos[p2]];
                        ctx->rgn_rd[(uint64_t)ws8 * ctx->rw_cunt + (rgn_bit / 64)] |= (uint64_t)0x1 << (rgn_bit & 63);
                    }

                    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ctx->rgn_cunt == 0 ? 0x1 : ctx->rgn_cunt) * (uint64_t)ctx->rw_cunt * sizeof(uint64_t), &raw);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    ctx->rgn_un = (uint64_t *)raw;

                    for (p2 = 0; p2 < (ctx->rgn_cunt == 0 ? 0x1 : ctx->rgn_cunt) * ctx->rw_cunt; p2++) { ctx->rgn_un[p2] = 0; }

                    for (p2 = 0; p2 < ep_cunt; p2++)
                    {
                        const crosstalk_hdl_ir_op_t *wop = &ir_ctx->comb_ops[epos[p2]];
                        uint32_t ws8 = crosstalk_hdl_emit_wsig_get(wop);
                        uint32_t rgn_at = 0;
                        uint32_t uw_j = 0;

                        if (ctx->fused[epos[p2]] != 0x0 || ws8 == UINT32_MAX) { continue; }

                        rgn_at = ctx->rgn_of[epos[p2]];

                        for (uw_j = 0; uw_j < ctx->rw_cunt; uw_j++) { ctx->rgn_un[(uint64_t)rgn_at * ctx->rw_cunt + uw_j] |= ctx->rgn_rd[(uint64_t)ws8 * ctx->rw_cunt + uw_j]; }

                        ctx->rgn_un[(uint64_t)rgn_at * ctx->rw_cunt + (rgn_at / 64)] &= ~((uint64_t)0x1 << (rgn_at & 63));
                    }
                }

                for (idx = 0; idx < ir_ctx->sig_cunt; idx++) { cur_wr[idx] = UINT32_MAX; }

                for (idx = 0; idx < ir_ctx->comb_cunt; idx++)
                {
                    uint32_t ws7 = crosstalk_hdl_emit_wsig_get(&ir_ctx->comb_ops[idx]);

                    rd_cunt4 = crosstalk_hdl_emit_rsigs_get(&ir_ctx->comb_ops[idx], rd_arr4);

                    for (rd_j = 0; rd_j < rd_cunt4; rd_j++)
                    {
                        uint32_t vw = cur_wr[rd_arr4[rd_j]];

                        if (vw == UINT32_MAX) { cwrap[rd_arr4[rd_j]] = 0x1; continue; }

                        if (ctx->loc_ok[vw] == 0x0) { continue; }

                        if (crosstalk_hdl_emit_op_wide_is(ir_ctx, &ir_ctx->comb_ops[idx]) == LDG_TRUTH_TRUE) { ctx->loc_ok[vw] = 0x0; continue; }

                        if (strm_of[idx] != strm_of[vw] || pos_of[idx] <= pos_of[vw] || pos_of[idx] > nxw[vw] || ctx->rgn_of[idx] != ctx->rgn_of[vw]) { ctx->loc_ok[vw] = 0x0; }
                    }

                    if (ws7 != UINT32_MAX) { cur_wr[ws7] = idx; }
                }

                for (idx = 0; idx < ir_ctx->sig_cunt; idx++)
                {
                    if (cwrap[idx] == 0x0 || cur_wr[idx] == UINT32_MAX) { continue; }

                    ctx->loc_ok[cur_wr[idx]] = 0x0;
                }

                for (idx = 0; idx < ir_ctx->seq_op_cunt; idx++)
                {
                    rd_cunt4 = crosstalk_hdl_emit_rsigs_get(&ir_ctx->seq_ops[idx], rd_arr4);

                    for (rd_j = 0; rd_j < rd_cunt4; rd_j++)
                    {
                        uint32_t vw = cur_wr[rd_arr4[rd_j]];

                        if (vw != UINT32_MAX) { ctx->loc_ok[vw] = 0x0; }
                    }
                }

                for (idx = 0; idx < ir_ctx->seq_blk_cunt; idx++)
                {
                    uint32_t cvw = ir_ctx->seq_blks[idx].clk_sig == UINT32_MAX ? UINT32_MAX : cur_wr[ir_ctx->seq_blks[idx].clk_sig];
                    uint32_t rvw = ir_ctx->seq_blks[idx].rst_sig == UINT32_MAX ? UINT32_MAX : cur_wr[ir_ctx->seq_blks[idx].rst_sig];

                    if (cvw != UINT32_MAX) { ctx->loc_ok[cvw] = 0x0; }

                    if (rvw != UINT32_MAX) { ctx->loc_ok[rvw] = 0x0; }
                }

                for (idx = 0; idx < ir_ctx->comb_cunt; idx++)
                {
                    uint32_t vfp = ctx->vf_prev[idx];

                    if (vfp == UINT32_MAX || ctx->loc_ok[vfp] == 0x0) { continue; }

                    if (ctx->rgn_of[idx] != ctx->rgn_of[vfp]) { ctx->loc_ok[vfp] = 0x0; }
                }

                ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->seq_op_cunt == 0 ? 0x1 : ir_ctx->seq_op_cunt), &raw);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                ctx->loc_seq_ok = (uint8_t *)raw;

                for (idx = 0; idx < ir_ctx->seq_op_cunt; idx++) { ctx->loc_seq_ok[idx] = 0x0; }

                for (idx = 0; idx < ir_ctx->seq_blk_cunt; idx++)
                {
                    uint32_t sb_j = 0;

                    for (sb_j = 0; sb_j < ir_ctx->sig_cunt; sb_j++) { last_slot[sb_j] = UINT32_MAX; }

                    sb_j = ir_ctx->seq_blks[idx].op_cunt;

                    while (sb_j > 0)
                    {
                        uint32_t sq = 0;
                        const crosstalk_hdl_ir_op_t *sqop = 0x0;

                        sb_j--;
                        sq = ir_ctx->seq_blks[idx].op_start + sb_j;
                        sqop = &ir_ctx->seq_ops[sq];

                        if (sqop->kind == CROSSTALK_HDL_IR_OP_REG_NEXT || sqop->kind == CROSSTALK_HDL_IR_OP_MEM_WR) { continue; }

                        if (sqop->dst == UINT32_MAX) { continue; }

                        if (last_slot[sqop->dst] != UINT32_MAX && ir_ctx->sigs[sqop->dst].depth == 0 && ctx->fused_seq[sq] == 0x0 && ctx->guard_need[sqop->dst] == 0x0 && crosstalk_hdl_emit_op_wide_is(ir_ctx, sqop) != LDG_TRUTH_TRUE) { ctx->loc_seq_ok[sq] = 0x1; }

                        last_slot[sqop->dst] = sq;
                    }
                }

                ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->seq_op_cunt == 0 ? 0x1 : ir_ctx->seq_op_cunt) * sizeof(uint32_t), &raw);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                {
                    uint32_t *blk_of = (uint32_t *)raw;
                    uint8_t *wrap_rd = 0x0;
                    uint32_t sv_j = 0;

                    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt), &raw);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    wrap_rd = (uint8_t *)raw;

                    for (idx = 0; idx < ir_ctx->seq_op_cunt; idx++) { blk_of[idx] = 0; }

                    for (idx = 0; idx < ir_ctx->seq_blk_cunt; idx++) { for (sv_j = 0; sv_j < ir_ctx->seq_blks[idx].op_cunt; sv_j++) { blk_of[ir_ctx->seq_blks[idx].op_start + sv_j] = idx; } }

                    for (idx = 0; idx < ir_ctx->seq_op_cunt; idx++)
                    {
                        const crosstalk_hdl_ir_op_t *uop = &ir_ctx->seq_ops[idx];

                        if (uop->kind == CROSSTALK_HDL_IR_OP_REG_NEXT || uop->kind == CROSSTALK_HDL_IR_OP_MEM_WR) { continue; }

                        if (uop->dst == UINT32_MAX || ctx->fused_seq[idx] != 0x0 || ctx->loc_seq_ok[idx] != 0x0) { continue; }

                        if (ctx->guard_need[uop->dst] != 0x0) { continue; }

                        if (ir_ctx->sigs[uop->dst].depth > 0) { continue; }

                        if (crosstalk_hdl_emit_op_wide_is(ir_ctx, uop) == LDG_TRUTH_TRUE) { continue; }

                        if (ir_ctx->sigs[uop->dst].name.len > 0 || ir_ctx->sigs[uop->dst].port != CROSSTALK_HDL_IR_PORT_NONE) { continue; }

                        ctx->loc_seq_ok[idx] = 0x1;
                    }

                    for (idx = 0; idx < ir_ctx->sig_cunt; idx++) { last_slot[idx] = UINT32_MAX; wrap_rd[idx] = 0x0; }

                    for (idx = 0; idx < ir_ctx->seq_op_cunt; idx++)
                    {
                        const crosstalk_hdl_ir_op_t *vop = &ir_ctx->seq_ops[idx];

                        rd_cunt4 = crosstalk_hdl_emit_rsigs_get(vop, rd_arr4);

                        for (rd_j = 0; rd_j < rd_cunt4; rd_j++)
                        {
                            uint32_t vw = last_slot[rd_arr4[rd_j]];

                            if (vw == UINT32_MAX) { wrap_rd[rd_arr4[rd_j]] = 0x1; continue; }

                            if (ctx->loc_seq_ok[vw] == 0x0) { continue; }

                            if (blk_of[idx] != blk_of[vw] || idx <= vw || crosstalk_hdl_emit_op_wide_is(ir_ctx, vop) == LDG_TRUTH_TRUE) { ctx->loc_seq_ok[vw] = 0x0; }
                        }

                        if (vop->kind != CROSSTALK_HDL_IR_OP_REG_NEXT && vop->kind != CROSSTALK_HDL_IR_OP_MEM_WR && vop->dst != UINT32_MAX) { last_slot[vop->dst] = idx; }
                    }

                    for (idx = 0; idx < ir_ctx->comb_cunt; idx++)
                    {
                        rd_cunt4 = crosstalk_hdl_emit_rsigs_get(&ir_ctx->comb_ops[idx], rd_arr4);

                        for (rd_j = 0; rd_j < rd_cunt4; rd_j++)
                        {
                            uint32_t vw = last_slot[rd_arr4[rd_j]];

                            if (vw != UINT32_MAX) { ctx->loc_seq_ok[vw] = 0x0; }
                        }
                    }

                    for (idx = 0; idx < ir_ctx->seq_blk_cunt; idx++)
                    {
                        uint32_t scw = ir_ctx->seq_blks[idx].clk_sig == UINT32_MAX ? UINT32_MAX : last_slot[ir_ctx->seq_blks[idx].clk_sig];
                        uint32_t srw = ir_ctx->seq_blks[idx].rst_sig == UINT32_MAX ? UINT32_MAX : last_slot[ir_ctx->seq_blks[idx].rst_sig];

                        if (scw != UINT32_MAX) { ctx->loc_seq_ok[scw] = 0x0; }

                        if (srw != UINT32_MAX) { ctx->loc_seq_ok[srw] = 0x0; }
                    }

                    for (idx = 0; idx < ir_ctx->sig_cunt; idx++)
                    {
                        if (wrap_rd[idx] == 0x0 || last_slot[idx] == UINT32_MAX) { continue; }

                        ctx->loc_seq_ok[last_slot[idx]] = 0x0;
                    }
                }
            }
        }
    }

    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->comb_cunt == 0 ? 0x1 : ir_ctx->comb_cunt) * sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->eval_ord = (uint32_t *)raw;

    for (idx = 0; idx < ctx->order_cunt; idx++) { ctx->eval_ord[idx] = ctx->order[idx]; }

    for (idx = 0; idx < ctx->residue_cunt; idx++) { ctx->eval_ord[ctx->order_cunt + idx] = ctx->residue[idx]; }

    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt) * sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->wr_last = (uint32_t *)raw;

    for (idx = 0; idx < ir_ctx->sig_cunt; idx++) { ctx->wr_last[idx] = UINT32_MAX; }

    for (idx = 0; idx < ir_ctx->comb_cunt; idx++)
    {
        uint32_t ws_sig = crosstalk_hdl_emit_wsig_get(&ir_ctx->comb_ops[ctx->eval_ord[idx]]);

        if (ws_sig != UINT32_MAX) { ctx->wr_last[ws_sig] = idx; }
    }

    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->comb_cunt == 0 ? 0x1 : ir_ctx->comb_cunt) * sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->sh_slot = (uint32_t *)raw;
    ctx->sh_cunt = 0;

    for (idx = 0; idx < ir_ctx->comb_cunt; idx++)
    {
        const crosstalk_hdl_ir_op_t *sh_op = &ir_ctx->comb_ops[ctx->eval_ord[idx]];
        uint32_t ws_sig = crosstalk_hdl_emit_wsig_get(sh_op);

        ctx->sh_slot[idx] = UINT32_MAX;

        if (sh_op->kind == CROSSTALK_HDL_IR_OP_MEM_WR) { continue; }

        if (ws_sig == UINT32_MAX) { continue; }

        if (ctx->wr_last[ws_sig] == idx) { continue; }

        ctx->sh_slot[idx] = ctx->sh_cunt;
        ctx->sh_cunt++;
    }

    ret = crosstalk_hdl_str_emit(ctx, "#include <stdin" "t.h>\n#ifdef HDL_GEN_MAIN\n#include <string.h>\n#else\n#include <stddef.h>\n" "in" "t strcmp(const char *aa, const char *bb);\nvoid *memcpy(void *dd, const void *ss, " "siz" "e_t nn);\nvoid *memset(void *dd, " "in" "t cc, " "siz" "e_t nn);\n#endif\n\ntypedef uin" "t64_t hdl_u64;\ntypedef uin" "t32_t hdl_u32;\ntypedef in" "t64_t hdl_i64;\n\n#define HDL_GEN_SIG_CUNT ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, ir_ctx->sig_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "\n#define HDL_GEN_HEX_CUNT ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, hex_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "\n#define HDL_GEN_MEM_WORDS ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, ctx->mem_words == 0 ? 0x1 : ctx->mem_words);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "\n#define HDL_GEN_BLK_CUNT ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, ir_ctx->seq_blk_cunt == 0 ? 0x1 : ir_ctx->seq_blk_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "\n#define HDL_GEN_RGN_CUNT ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, ctx->rgn_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "\n#define HDL_GEN_RW_CUNT ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, ctx->rw_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "\n\nstatic hdl_u64 v[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "];\nstatic hdl_u64 m[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, ctx->mem_words == 0 ? 0x1 : ctx->mem_words);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "];\nstatic hdl_u64 pc[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, ir_ctx->seq_blk_cunt == 0 ? 0x1 : ir_ctx->seq_blk_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "];\nstatic hdl_u64 pr[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, ir_ctx->seq_blk_cunt == 0 ? 0x1 : ir_ctx->seq_blk_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "];\nstatic hdl_u32 fired[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, ir_ctx->seq_blk_cunt == 0 ? 0x1 : ir_ctx->seq_blk_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "];\nstatic hdl_u64 rn[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, ctx->rn_cunt == 0 ? 0x1 : ctx->rn_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "];\nstatic hdl_u64 mwe[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, ctx->mw_cunt == 0 ? 0x1 : ctx->mw_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "];\nstatic hdl_u64 mwa[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, ctx->mw_cunt == 0 ? 0x1 : ctx->mw_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "];\nstatic hdl_u64 mwv[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, ctx->mw_cunt == 0 ? 0x1 : ctx->mw_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "];\nstatic hdl_u64 vh[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "];\nstatic hdl_u64 mh[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, ctx->mem_words == 0 ? 0x1 : ctx->mem_words);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "];\nstatic hdl_u64 rnh[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, ctx->rn_cunt == 0 ? 0x1 : ctx->rn_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "];\nstatic hdl_u64 mwvh[");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, ctx->mw_cunt == 0 ? 0x1 : ctx->mw_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->pf_is == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_str_emit(ctx, "];\nstatic hdl_u32 stg;\nstatic hdl_u32 pkd;\nstatic hdl_u32 cmt;\nstatic hdl_u64 pf_t[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, (ir_ctx->seq_blk_cunt == 0 ? 0x1 : ir_ctx->seq_blk_cunt) + CROSSTALK_HDL_EMIT_PF_EXTRA);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "];\nstatic hdl_u64 pf_c[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, (ir_ctx->seq_blk_cunt == 0 ? 0x1 : ir_ctx->seq_blk_cunt) + CROSSTALK_HDL_EMIT_PF_EXTRA);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "];\nstatic hdl_u64 pfr_tot[4096];\nstatic hdl_u64 pfr_comb[4096];\nstatic hdl_u64 pfr_blk[4096];\nstatic hdl_u64 pfr_cmt[4096];\nstatic hdl_u64 pfr_meta[16];\nstatic hdl_u64 pf_s[8];\nstatic hdl_u64 ra[HDL_GEN_RW_CUNT];\nstatic hdl_u64 pf_cur_comb;\nstatic hdl_u64 pf_cur_blk;\nstatic hdl_u64 pf_cur_cmt;\n\nstatic hdl_u64 hdl_pf_now(void)\n{\nreturn (hdl_u64)__builtin_readcyclecounter();\n}\n\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    else
    {
        ret = crosstalk_hdl_str_emit(ctx, "];\nstatic hdl_u32 stg;\nstatic hdl_u32 pkd;\nstatic hdl_u32 cmt;\nstatic hdl_u64 ra[HDL_GEN_RW_CUNT];\n\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_u64_tbl_emit(ctx, "static const hdl_u64 mk[] = {", ir_ctx->sig_cunt, LDG_TRUTH_TRUE, crosstalk_hdl_emit_mask_val_get);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_u64_tbl_emit(ctx, "static const hdl_u32 dp[] = {", ir_ctx->sig_cunt, LDG_TRUTH_FALSE, crosstalk_hdl_emit_depth_val_get);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_u64_tbl_emit(ctx, "static const hdl_u32 mo[] = {", ir_ctx->sig_cunt, LDG_TRUTH_FALSE, crosstalk_hdl_emit_moff_val_get);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    {
        uint32_t rgo_off = 0;
        uint32_t rgl_has = 0;
        uint32_t sig_j = 0;
        uint32_t wj2 = 0;
        uint32_t bj2 = 0;

        ret = crosstalk_hdl_str_emit(ctx, "static const hdl_u32 rgo[] = {");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        for (sig_j = 0; sig_j <= ir_ctx->sig_cunt; sig_j++)
        {
            ret = crosstalk_hdl_dec_emit(ctx, rgo_off);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, ",");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (sig_j == ir_ctx->sig_cunt) { break; }

            for (wj2 = 0; wj2 < ctx->rw_cunt; wj2++)
            {
                uint64_t mval2 = ctx->rgn_rd[(uint64_t)sig_j * ctx->rw_cunt + wj2];

                while (mval2 != 0x0) { mval2 = mval2 & (mval2 - 0x1); rgo_off++; }
            }
        }

        ret = crosstalk_hdl_str_emit(ctx, "};\nstatic const hdl_u32 rgl[] = {");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        for (sig_j = 0; sig_j < ir_ctx->sig_cunt; sig_j++)
        {
            for (wj2 = 0; wj2 < ctx->rw_cunt; wj2++)
            {
                uint64_t mval2 = ctx->rgn_rd[(uint64_t)sig_j * ctx->rw_cunt + wj2];

                for (bj2 = 0; bj2 < 64 && mval2 != 0x0; bj2++)
                {
                    if ((mval2 & ((uint64_t)0x1 << bj2)) == 0x0) { continue; }

                    mval2 = mval2 & ~((uint64_t)0x1 << bj2);

                    ret = crosstalk_hdl_dec_emit(ctx, wj2 * 64 + bj2);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    ret = crosstalk_hdl_str_emit(ctx, ",");
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    rgl_has = 0x1;
                }
            }
        }

        if (rgl_has == 0x0)
        {
            ret = crosstalk_hdl_str_emit(ctx, "0x0");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_str_emit(ctx, "};\n\nstatic void hdl_rgn_sig(hdl_u32 i2)\n{\nhdl_u32 j2;\nif (i2 >= HDL_GEN_SIG_CUNT) { return; }\nfor (j2 = rgo[i2]; j2 < rgo[i2 + 0x1]; j2++) { ra[rgl[j2] >> 6] |= (hdl_u64)0x1 << (rgl[j2] & 63); }\n}\n\nstatic void hdl_gen_touch(void)\n{\nhdl_u32 j2;\nfor (j2 = 0x0; j2 < HDL_GEN_RW_CUNT; j2++) { ra[j2] = ~(hdl_u64)0x0; }\npkd = 0x1;\n}\n\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    {
        uint64_t *jmk_arr = 0x0;
        uint64_t *jmh_arr = 0x0;
        uint32_t *jsg_arr = 0x0;
        uint8_t *jwd_arr = 0x0;
        uint32_t *jmo_arr = 0x0;
        uint32_t *jdp_arr = 0x0;
        uint32_t jtotal = ctx->rn_cunt + ctx->mw_cunt;
        uint32_t jalloc = jtotal == 0 ? 0x1 : jtotal;
        uint32_t malloc2 = ctx->mw_cunt == 0 ? 0x1 : ctx->mw_cunt;
        uint32_t jat = 0;

        ret = ldg_mem_pool_alloc(pool, (uint64_t)jalloc * sizeof(uint64_t), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        jmk_arr = (uint64_t *)raw;

        ret = ldg_mem_pool_alloc(pool, (uint64_t)jalloc * sizeof(uint64_t), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        jmh_arr = (uint64_t *)raw;

        ret = ldg_mem_pool_alloc(pool, (uint64_t)jalloc * sizeof(uint32_t), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        jsg_arr = (uint32_t *)raw;

        ret = ldg_mem_pool_alloc(pool, (uint64_t)jalloc, &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        jwd_arr = (uint8_t *)raw;

        ret = ldg_mem_pool_alloc(pool, (uint64_t)malloc2 * sizeof(uint32_t), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        jmo_arr = (uint32_t *)raw;

        ret = ldg_mem_pool_alloc(pool, (uint64_t)malloc2 * sizeof(uint32_t), &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        jdp_arr = (uint32_t *)raw;

        for (jat = 0; jat < jalloc; jat++) { jmk_arr[jat] = 0; jmh_arr[jat] = 0; jsg_arr[jat] = 0; jwd_arr[jat] = 0x0; }

        for (jat = 0; jat < malloc2; jat++) { jmo_arr[jat] = 0; jdp_arr[jat] = 0; }

        for (jat = 0; jat < ir_ctx->seq_op_cunt; jat++)
        {
            const crosstalk_hdl_ir_op_t *jop = &ir_ctx->seq_ops[jat];

            if (ctx->rn_slot[jat] != UINT32_MAX)
            {
                uint32_t js = ctx->rn_slot[jat];

                jsg_arr[js] = jop->dst;
                jmk_arr[js] = crosstalk_hdl_emit_mask_get(ir_ctx->sigs[jop->dst].width);
                jmh_arr[js] = ir_ctx->sigs[jop->dst].width > 64 ? crosstalk_hdl_emit_mask_get(ir_ctx->sigs[jop->dst].width - 64) : 0x0;
                jwd_arr[js] = ir_ctx->sigs[jop->dst].width > 64 ? 0x1 : 0x0;
            }

            if (ctx->mw_slot[jat] != UINT32_MAX)
            {
                uint32_t js = ctx->rn_cunt + ctx->mw_slot[jat];

                jsg_arr[js] = jop->src0;
                jmk_arr[js] = crosstalk_hdl_emit_mask_get(ir_ctx->sigs[jop->src0].width);
                jmh_arr[js] = ir_ctx->sigs[jop->src0].width > 64 ? crosstalk_hdl_emit_mask_get(ir_ctx->sigs[jop->src0].width - 64) : 0x0;
                jwd_arr[js] = ir_ctx->sigs[jop->src0].width > 64 ? 0x1 : 0x0;
                jmo_arr[ctx->mw_slot[jat]] = ctx->mem_off[jop->src0];
                jdp_arr[ctx->mw_slot[jat]] = ir_ctx->sigs[jop->src0].depth;
            }
        }

        ret = crosstalk_hdl_str_emit(ctx, "static hdl_u32 sj[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, jalloc);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "];\nstatic hdl_u64 sjb[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, (jalloc + 63) / 64);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "];\nstatic hdl_u32 sjn;\nstatic const hdl_u32 jsg[] = {");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        for (jat = 0; jat < jalloc; jat++)
        {
            ret = crosstalk_hdl_dec_emit(ctx, jsg_arr[jat]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, ",");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_str_emit(ctx, "};\nstatic const hdl_u64 jmk[] = {");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        for (jat = 0; jat < jalloc; jat++)
        {
            ret = crosstalk_hdl_hex_emit(ctx, jmk_arr[jat]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, ",");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_str_emit(ctx, "};\nstatic const hdl_u64 jmh[] = {");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        for (jat = 0; jat < jalloc; jat++)
        {
            ret = crosstalk_hdl_hex_emit(ctx, jmh_arr[jat]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, ",");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_str_emit(ctx, "};\nstatic const hdl_u32 jwd[] = {");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        for (jat = 0; jat < jalloc; jat++)
        {
            ret = crosstalk_hdl_dec_emit(ctx, jwd_arr[jat]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, ",");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_str_emit(ctx, "};\nstatic const hdl_u32 jmo[] = {");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        for (jat = 0; jat < malloc2; jat++)
        {
            ret = crosstalk_hdl_dec_emit(ctx, jmo_arr[jat]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, ",");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_str_emit(ctx, "};\nstatic const hdl_u32 jdp[] = {");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        for (jat = 0; jat < malloc2; jat++)
        {
            ret = crosstalk_hdl_dec_emit(ctx, jdp_arr[jat]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, ",");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_str_emit(ctx, "};\n\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }


    ret = crosstalk_hdl_str_emit(ctx, "static const char *const nmt[] = {");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < ir_ctx->sig_cunt; idx++)
    {
        if (idx > 0)
        {
            ret = crosstalk_hdl_str_emit(ctx, ", ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_str_emit(ctx, "\"");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_bytes_emit(ctx, src + ir_ctx->sigs[idx].name.off, ir_ctx->sigs[idx].name.len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "\"");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_str_emit(ctx, "};\nstatic const hdl_u32 hx_sig[] = {");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (hex_cunt == 0) { ret = crosstalk_hdl_str_emit(ctx, "4294967295"); }
    else
    {
        uint32_t first = 0x1;

        for (idx = 0; idx < ir_ctx->sig_cunt; idx++)
        {
            if (ir_ctx->sigs[idx].depth == 0 || ir_ctx->sigs[idx].mem_file.len <= 2) { continue; }

            if (first != 0x1)
            {
                ret = crosstalk_hdl_str_emit(ctx, ", ");
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            first = 0x0;
            ret = crosstalk_hdl_dec_emit(ctx, idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "};\nstatic const char *const hx_path[] = {");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (hex_cunt == 0) { ret = crosstalk_hdl_str_emit(ctx, "\"\""); }
    else
    {
        uint32_t first = 0x1;

        for (idx = 0; idx < ir_ctx->sig_cunt; idx++)
        {
            if (ir_ctx->sigs[idx].depth == 0 || ir_ctx->sigs[idx].mem_file.len <= 2) { continue; }

            if (first != 0x1)
            {
                ret = crosstalk_hdl_str_emit(ctx, ", ");
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            first = 0x0;

            ret = crosstalk_hdl_str_emit(ctx, "\"");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_bytes_emit(ctx, src + ir_ctx->sigs[idx].mem_file.off + 0x1, ir_ctx->sigs[idx].mem_file.len - 0x2);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, "\"");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->pf_is == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_str_emit(ctx, "};\nstatic char pf_nm[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, ir_ctx->seq_blk_cunt == 0 ? 0x1 : ir_ctx->seq_blk_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "][");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, CROSSTALK_HDL_EMIT_PF_NM_BYTES);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "] = {");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (ir_ctx->seq_blk_cunt == 0)
        {
            ret = crosstalk_hdl_str_emit(ctx, "\"\"");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        for (idx = 0; idx < ir_ctx->seq_blk_cunt; idx++)
        {
            if (idx > 0)
            {
                ret = crosstalk_hdl_str_emit(ctx, ", ");
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            ret = crosstalk_hdl_str_emit(ctx, "\"");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_emit_blk_nm_emit(ctx, idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, "\"");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }

    ret = crosstalk_hdl_str_emit(ctx, "};\n\nstatic hdl_u64 hdl_gen_sx(hdl_u64 vx, hdl_u32 wd)\n{\nhdl_u64 sb;\nif (wd == 0x0 || wd >= 64) { return vx; }\nsb = (hdl_u64)0x1 << (wd - 0x1);\nif ((vx & sb) != 0x0) { return vx | ~(((hdl_u64)0x1 << wd) - 0x1); }\nreturn vx;\n}\n\nstatic hdl_u64 hdl_gen_rx(hdl_u64 vx)\n{\nvx = vx ^ (vx >> 32);\nvx = vx ^ (vx >> 16);\nvx = vx ^ (vx >> 8);\nvx = vx ^ (vx >> 4);\nvx = vx ^ (vx >> 2);\nvx = vx ^ (vx >> 0x1);\nreturn vx & 0x1;\n}\n\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->wide_has == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_str_emit(ctx, "static hdl_u64 hdl_wmk(hdl_u32 wd2)\n{\nif (wd2 == 0x0) { return 0x0; }\nif (wd2 >= 64) { return ~(hdl_u64)0x0; }\nreturn ((hdl_u64)0x1 << wd2) - 0x1;\n}\n\nstatic hdl_u64 hdl_wmkh(hdl_u32 wd2)\n{\nif (wd2 <= 64) { return 0x0; }\nreturn hdl_wmk(wd2 - 64);\n}\n\nstatic void hdl_wshl(hdl_u64 lo2, hdl_u64 hi2, hdl_u64 amt, hdl_u64 *olo, hdl_u64 *ohi)\n{\nif (amt >= 128) { *olo = 0x0; *ohi = 0x0; return; }\nif (amt >= 64) { *ohi = lo2 << (amt - 64); *olo = 0x0; return; }\nif (amt == 0x0) { *olo = lo2; *ohi = hi2; return; }\n*ohi = (hi2 << amt) | (lo2 >> (64 - amt));\n*olo = lo2 << amt;\n}\n\nstatic void hdl_wshr(hdl_u64 lo2, hdl_u64 hi2, hdl_u64 amt, hdl_u64 *olo, hdl_u64 *ohi)\n{\nif (amt >= 128) { *olo = 0x0; *ohi = 0x0; return; }\nif (amt >= 64) { *olo = hi2 >> (amt - 64); *ohi = 0x0; return; }\nif (amt == 0x0) { *olo = lo2; *ohi = hi2; return; }\n*olo = (lo2 >> amt) | (hi2 << (64 - amt));\n*ohi = hi2 >> amt;\n}\n\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "static hdl_u32 hdl_wltu(hdl_u64 xl, hdl_u64 xh, hdl_u64 yl, hdl_u64 yh)\n{\nif (xh != yh) { return xh < yh ? 0x1 : 0x0; }\nreturn xl < yl ? 0x1 : 0x0;\n}\n\nstatic void hdl_wsub2(hdl_u64 xl, hdl_u64 xh, hdl_u64 yl, hdl_u64 yh, hdl_u64 *olo, hdl_u64 *ohi)\n{\n*ohi = xh - yh - (xl < yl ? 0x1 : 0x0);\n*olo = xl - yl;\n}\n\nstatic void hdl_wmul(hdl_u64 xl, hdl_u64 xh, hdl_u64 yl, hdl_u64 yh, hdl_u64 *olo, hdl_u64 *ohi)\n{\nhdl_u64 a0;\nhdl_u64 a1;\nhdl_u64 b0;\nhdl_u64 b1;\nhdl_u64 p00;\nhdl_u64 p01;\nhdl_u64 p10;\nhdl_u64 p11;\nhdl_u64 mid;\nhdl_u64 mc;\nhdl_u64 lw;\nhdl_u64 c2;\na0 = xl & 4294967295;\na1 = xl >> 32;\nb0 = yl & 4294967295;\nb1 = yl >> 32;\np00 = a0 * b0;\np01 = a0 * b1;\np10 = a1 * b0;\np11 = a1 * b1;\nmid = p01 + p10;\nmc = mid < p01 ? 0x1 : 0x0;\nlw = p00 + (mid << 32);\nc2 = lw < p00 ? 0x1 : 0x0;\n*olo = lw;\n*ohi = p11 + (mid >> 32) + (mc << 32) + c2 + xl * yh + xh * yl;\n}\n\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "static void hdl_wdivmod(hdl_u64 xl, hdl_u64 xh, hdl_u64 yl, hdl_u64 yh, hdl_u64 *qlo, hdl_u64 *qhi, hdl_u64 *rlo2, hdl_u64 *rhi2)\n{\nhdl_u64 ql;\nhdl_u64 qh;\nhdl_u64 rl2;\nhdl_u64 rh2;\nhdl_u64 tl;\nhdl_u64 th;\nhdl_u64 ab;\nhdl_u32 bp;\nql = 0x0;\nqh = 0x0;\nrl2 = 0x0;\nrh2 = 0x0;\n*qlo = 0x0;\n*qhi = 0x0;\n*rlo2 = 0x0;\n*rhi2 = 0x0;\nif ((yl | yh) == 0x0) { return; }\nbp = 128;\nwhile (bp > 0x0)\n{\nbp--;\nrh2 = (rh2 << 0x1) | (rl2 >> 63);\nrl2 = rl2 << 0x1;\nab = bp >= 64 ? ((xh >> (bp - 64)) & 0x1) : ((xl >> bp) & 0x1);\nrl2 = rl2 | ab;\nif (hdl_wltu(rl2, rh2, yl, yh) == 0x0)\n{\nhdl_wsub2(rl2, rh2, yl, yh, &tl, &th);\nrl2 = tl;\nrh2 = th;\nif (bp >= 64) { qh = qh | ((hdl_u64)0x1 << (bp - 64)); }\nelse { ql = ql | ((hdl_u64)0x1 << bp); }\n}\n}\n*qlo = ql;\n*qhi = qh;\n*rlo2 = rl2;\n*rhi2 = rh2;\n}\n\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "static hdl_u32 hdl_wop(hdl_u32 kd, hdl_u32 di, hdl_u32 ai, hdl_u32 bi2, hdl_u32 ci, hdl_u64 im, hdl_u32 wd, hdl_u32 dw)\n{\nhdl_u64 al;\nhdl_u64 ah;\nhdl_u64 bl;\nhdl_u64 bh;\nhdl_u64 cl2;\nhdl_u64 ch2;\nhdl_u64 rl;\nhdl_u64 rh;\nhdl_u64 t0;\nhdl_u64 t1;\nhdl_u64 t2;\nhdl_u64 t3;\nal = ai == 4294967295 ? 0x0 : v[ai];\nah = ai == 4294967295 ? 0x0 : vh[ai];\nbl = bi2 == 4294967295 ? 0x0 : v[bi2];\nbh = bi2 == 4294967295 ? 0x0 : vh[bi2];\ncl2 = ci == 4294967295 ? 0x0 : v[ci];\nch2 = ci == 4294967295 ? 0x0 : vh[ci];\nrl = 0x0;\nrh = 0x0;\nt2 = 0x0;\nt3 = 0x0;\nswitch (kd)\n{\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_CONST, "rl = im;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_COPY, "rl = al;\nrh = ah;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_ZEXT, "rl = al;\nrh = ah;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_SEXT, "if (im <= 64) { rl = hdl_gen_sx(al, (hdl_u32)im); rh = (hdl_i64)rl < (hdl_i64)0x0 ? ~(hdl_u64)0x0 : 0x0; }\nelse { t0 = (ah >> ((hdl_u32)im - 65)) & 0x1; rl = al; rh = t0 != 0x0 ? ah | ~hdl_wmkh((hdl_u32)im) : ah; }\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_AND, "rl = al & bl;\nrh = ah & bh;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_OR, "rl = al | bl;\nrh = ah | bh;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_XOR, "rl = al ^ bl;\nrh = ah ^ bh;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_XNOR, "rl = ~(al ^ bl);\nrh = ~(ah ^ bh);\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_NOT, "rl = ~al;\nrh = ~ah;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_ADD, "rl = al + bl;\nrh = ah + bh + (rl < al ? 0x1 : 0x0);\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_SUB, "rl = al - bl;\nrh = ah - bh - (al < bl ? 0x1 : 0x0);\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_MUL, "hdl_wmul(al, ah, bl, bh, &rl, &rh);\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_DIV, "if ((bl | bh) != 0x0) { hdl_wdivmod(al, ah, bl, bh, &rl, &rh, &t0, &t1); }\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_MOD, "if ((bl | bh) != 0x0) { hdl_wdivmod(al, ah, bl, bh, &t0, &t1, &rl, &rh); }\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_NEG, "rl = ~al + 0x1;\nrh = ~ah + (al == 0x0 ? 0x1 : 0x0);\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_SHL, "hdl_wshl(al, ah, bh != 0x0 ? 128 : bl, &rl, &rh);\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_SHR, "hdl_wshr(al, ah, bh != 0x0 ? 128 : bl, &rl, &rh);\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_ASHR, "if (wd <= 64) { t0 = hdl_gen_sx(al, wd); if (bh != 0x0 || bl >= 64) { rl = (hdl_i64)t0 < (hdl_i64)0x0 ? ~(hdl_u64)0x0 : 0x0; } else { rl = (hdl_u64)((hdl_i64)t0 >> bl); } rh = (hdl_i64)t0 < (hdl_i64)0x0 ? ~(hdl_u64)0x0 : 0x0; }\nelse { t0 = (ah >> (wd - 65)) & 0x1; if (bh != 0x0 || bl >= (hdl_u64)wd) { rl = t0 != 0x0 ? ~(hdl_u64)0x0 : 0x0; rh = rl; } else { hdl_wshr(al, ah, bl, &rl, &rh); if (t0 != 0x0) { hdl_wshl(hdl_wmk((hdl_u32)bl), bl > 64 ? hdl_wmk((hdl_u32)bl - 64) : 0x0, (hdl_u64)wd - bl, &t2, &t3); rl = rl | t2; rh = rh | t3; } } }\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_EQ, "rl = al == bl && ah == bh ? 0x1 : 0x0;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_NEQ, "rl = al != bl || ah != bh ? 0x1 : 0x0;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_LT, "rl = ah != bh ? (ah < bh ? 0x1 : 0x0) : (al < bl ? 0x1 : 0x0);\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_LE, "rl = ah != bh ? (ah < bh ? 0x1 : 0x0) : (al <= bl ? 0x1 : 0x0);\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_GT, "rl = ah != bh ? (ah > bh ? 0x1 : 0x0) : (al > bl ? 0x1 : 0x0);\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_GE, "rl = ah != bh ? (ah > bh ? 0x1 : 0x0) : (al >= bl ? 0x1 : 0x0);\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_LT_S, "if (wd <= 64) { t0 = al ^ ((hdl_u64)0x1 << (wd - 0x1)); t2 = bl ^ ((hdl_u64)0x1 << (wd - 0x1)); t1 = 0x0; t3 = 0x0; } else { t0 = al; t2 = bl; t1 = ah ^ ((hdl_u64)0x1 << (wd - 65)); t3 = bh ^ ((hdl_u64)0x1 << (wd - 65)); }\nrl = t1 != t3 ? (t1 < t3 ? 0x1 : 0x0) : (t0 < t2 ? 0x1 : 0x0);\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_LE_S, "if (wd <= 64) { t0 = al ^ ((hdl_u64)0x1 << (wd - 0x1)); t2 = bl ^ ((hdl_u64)0x1 << (wd - 0x1)); t1 = 0x0; t3 = 0x0; } else { t0 = al; t2 = bl; t1 = ah ^ ((hdl_u64)0x1 << (wd - 65)); t3 = bh ^ ((hdl_u64)0x1 << (wd - 65)); }\nrl = (t1 != t3 ? (t1 < t3 ? 0x1 : 0x0) : (t0 < t2 ? 0x1 : 0x0)) != 0x0 || (t1 == t3 && t0 == t2) ? 0x1 : 0x0;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_GT_S, "if (wd <= 64) { t0 = al ^ ((hdl_u64)0x1 << (wd - 0x1)); t2 = bl ^ ((hdl_u64)0x1 << (wd - 0x1)); t1 = 0x0; t3 = 0x0; } else { t0 = al; t2 = bl; t1 = ah ^ ((hdl_u64)0x1 << (wd - 65)); t3 = bh ^ ((hdl_u64)0x1 << (wd - 65)); }\nrl = (t1 != t3 ? (t1 < t3 ? 0x1 : 0x0) : (t0 < t2 ? 0x1 : 0x0)) == 0x0 && !(t1 == t3 && t0 == t2) ? 0x1 : 0x0;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_GE_S, "if (wd <= 64) { t0 = al ^ ((hdl_u64)0x1 << (wd - 0x1)); t2 = bl ^ ((hdl_u64)0x1 << (wd - 0x1)); t1 = 0x0; t3 = 0x0; } else { t0 = al; t2 = bl; t1 = ah ^ ((hdl_u64)0x1 << (wd - 65)); t3 = bh ^ ((hdl_u64)0x1 << (wd - 65)); }\nrl = (t1 != t3 ? (t1 < t3 ? 0x1 : 0x0) : (t0 < t2 ? 0x1 : 0x0)) == 0x0 ? 0x1 : 0x0;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_LAND, "rl = (al | ah) != 0x0 && (bl | bh) != 0x0 ? 0x1 : 0x0;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_LOR, "rl = (al | ah) != 0x0 || (bl | bh) != 0x0 ? 0x1 : 0x0;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_LNOT, "rl = (al | ah) == 0x0 ? 0x1 : 0x0;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_RED_AND, "rl = al == hdl_wmk((hdl_u32)im) && ah == hdl_wmkh((hdl_u32)im) ? 0x1 : 0x0;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_RED_OR, "rl = (al | ah) != 0x0 ? 0x1 : 0x0;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_RED_XOR, "t0 = al ^ ah;\nt0 = t0 ^ (t0 >> 32);\nt0 = t0 ^ (t0 >> 16);\nt0 = t0 ^ (t0 >> 8);\nt0 = t0 ^ (t0 >> 4);\nt0 = t0 ^ (t0 >> 2);\nt0 = t0 ^ (t0 >> 0x1);\nrl = t0 & 0x1;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_CONCAT, "hdl_wshl(al, ah, im, &rl, &rh);\nrl = rl | (bl & hdl_wmk((hdl_u32)im));\nrh = rh | (bh & hdl_wmkh((hdl_u32)im));\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_SLICE, "hdl_wshr(al, ah, im, &rl, &rh);\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_SLICE_DYN, "hdl_wshr(al, ah, bh != 0x0 ? 128 : bl, &rl, &rh);\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_INSERT, "hdl_wshl(hdl_wmk(wd), hdl_wmkh(wd), im, &t0, &t1);\nhdl_wshl(bl & hdl_wmk(wd), bh & hdl_wmkh(wd), im, &t2, &t3);\nrl = (al & ~t0) | t2;\nrh = (ah & ~t1) | t3;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_INSERT_DYN, "if (ch2 != 0x0 || cl2 >= 128) { rl = al; rh = ah; }\nelse { hdl_wshl(hdl_wmk(wd), hdl_wmkh(wd), cl2, &t0, &t1); hdl_wshl(bl & hdl_wmk(wd), bh & hdl_wmkh(wd), cl2, &t2, &t3); rl = (al & ~t0) | t2; rh = (ah & ~t1) | t3; }\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_wcase_emit(ctx, CROSSTALK_HDL_IR_OP_MUX, "rl = (al | ah) != 0x0 ? bl : cl2;\nrh = (al | ah) != 0x0 ? bh : ch2;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "default:\nbreak;\n}\nt0 = v[di];\nt1 = vh[di];\nrl = rl & hdl_wmk(dw);\nrh = rh & hdl_wmkh(dw);\nv[di] = rl;\nvh[di] = rh;\nreturn t0 != rl || t1 != rh ? 0x1 : 0x0;\n}\n\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->comb_cunt == 0 ? 0x1 : ir_ctx->comb_cunt), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    konst_ok = (uint8_t *)raw;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)(ir_ctx->sig_cunt == 0 ? 0x1 : ir_ctx->sig_cunt) * sizeof(uint32_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    konst_wr = (uint32_t *)raw;

    for (idx = 0; idx < ir_ctx->sig_cunt; idx++) { konst_wr[idx] = 0; }

    for (idx = 0; idx < ir_ctx->seq_op_cunt; idx++)
    {
        uint32_t sws = crosstalk_hdl_emit_wsig_get(&ir_ctx->seq_ops[idx]);

        if (sws != UINT32_MAX) { konst_wr[sws] = konst_wr[sws] + 0x2; }
    }

    for (idx = 0; idx < ir_ctx->comb_cunt; idx++)
    {
        uint32_t cws = crosstalk_hdl_emit_wsig_get(&ir_ctx->comb_ops[idx]);

        if (cws != UINT32_MAX) { konst_wr[cws] = konst_wr[cws] + 0x1; }
    }

    for (idx = 0; idx < ir_ctx->comb_cunt; idx++)
    {
        const crosstalk_hdl_ir_op_t *kop = &ir_ctx->comb_ops[idx];
        uint32_t kws = 0;

        konst_ok[idx] = LDG_TRUTH_FALSE;

        if (kop->kind == CROSSTALK_HDL_IR_OP_NONE || kop->kind == CROSSTALK_HDL_IR_OP_MEM_RD || kop->kind == CROSSTALK_HDL_IR_OP_MEM_WR || kop->kind == CROSSTALK_HDL_IR_OP_REG_NEXT) { continue; }

        if (crosstalk_hdl_emit_op_wide_is(ir_ctx, kop) == LDG_TRUTH_TRUE) { continue; }

        if (ctx->loc_ok[idx] != 0x0 || ctx->fused[idx] != 0x0 || ctx->vf_prev[idx] != UINT32_MAX) { continue; }

        if (crosstalk_hdl_emit_rsigs_get(kop, krd) != 0) { continue; }

        kws = crosstalk_hdl_emit_wsig_get(kop);

        if (kws == UINT32_MAX || konst_wr[kws] != 0x1) { continue; }

        konst_ok[idx] = LDG_TRUTH_TRUE;
    }

    ret = crosstalk_hdl_str_emit(ctx, "static void hdl_gen_konst(void)\n{\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < ir_ctx->comb_cunt; idx++)
    {
        if (konst_ok[idx] == LDG_TRUTH_FALSE) { continue; }

        ret = crosstalk_hdl_comb_stmt_emit(ctx, &ir_ctx->comb_ops[idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_str_emit(ctx, "}\n\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->pf_is == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_str_emit(ctx, "static hdl_u32 hdl_gen_comb(void)\n{\nhdl_u64 pfa;\nhdl_u64 pfk;\nhdl_u64 ab[HDL_GEN_RW_CUNT];\nhdl_u64 nx;\nhdl_u32 wj;\n"); }
    else { ret = crosstalk_hdl_str_emit(ctx, "static hdl_u32 hdl_gen_comb(void)\n{\nhdl_u64 ab[HDL_GEN_RW_CUNT];\nhdl_u64 nx;\nhdl_u32 wj;\n"); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->residue_cunt > 0)
    {
        ret = crosstalk_hdl_str_emit(ctx, "hdl_u32 pass;\nhdl_u64 aw;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (ctx->wide_has == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_str_emit(ctx, "hdl_u64 nxh;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (idx = 0; idx < ir_ctx->comb_cunt; idx++)
    {
        if (ctx->loc_ok[idx] == 0x0) { continue; }

        ret = crosstalk_hdl_str_emit(ctx, "hdl_u64 tl");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, ";\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (ctx->pf_is == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_str_emit(ctx, "nx = 0x0;\nnx = nx;\nfor (wj = 0x0; wj < HDL_GEN_RW_CUNT; wj++) { ab[wj] = ra[wj]; ra[wj] = 0x0; }\npfa = hdl_pf_now();\npfk = pfa;\n"); }
    else { ret = crosstalk_hdl_str_emit(ctx, "nx = 0x0;\nnx = nx;\nfor (wj = 0x0; wj < HDL_GEN_RW_CUNT; wj++) { ab[wj] = ra[wj]; ra[wj] = 0x0; }\n"); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    cur_rgn = UINT32_MAX;

    for (idx = 0; idx < ctx->order_cunt; idx++)
    {
        uint32_t lop_idx = ctx->order[idx];
        uint32_t lws = 0;

        if (ctx->fused[lop_idx] != 0x0) { continue; }

        if (konst_ok[lop_idx] != LDG_TRUTH_FALSE) { continue; }

        if (ctx->rgn_of[lop_idx] != cur_rgn)
        {
            if (cur_rgn != UINT32_MAX)
            {
                ret = crosstalk_hdl_rgn_close_emit(ctx, cur_rgn, LDG_TRUTH_TRUE);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            cur_rgn = ctx->rgn_of[lop_idx];
            ret = crosstalk_hdl_rgn_gate_emit(ctx, cur_rgn);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        lws = crosstalk_hdl_emit_wsig_get(&ir_ctx->comb_ops[lop_idx]);

        ctx->rgn_chk = 0x0;

        if (ctx->vf_prev[lop_idx] != UINT32_MAX) { ctx->vf_op = ctx->vf_prev[lop_idx]; ctx->vf_sig = ir_ctx->comb_ops[ctx->vf_op].dst; }

        if (ctx->loc_ok[lop_idx] != 0x0) { ret = crosstalk_hdl_loc_stmt_emit(ctx, &ir_ctx->comb_ops[lop_idx], lop_idx, "tl"); }
        else { ret = crosstalk_hdl_comb_stmt_emit(ctx, &ir_ctx->comb_ops[lop_idx]); }

        ctx->vf_op = UINT32_MAX;
        ctx->vf_sig = UINT32_MAX;
        ctx->rgn_chk = 0x0;

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (lws != UINT32_MAX) { ctx->loc_act[lws] = ctx->loc_ok[lop_idx] != 0x0 ? lop_idx : UINT32_MAX; }
    }

    if (cur_rgn != UINT32_MAX)
    {
        ret = crosstalk_hdl_rgn_close_emit(ctx, cur_rgn, LDG_TRUTH_TRUE);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (idx = 0; idx < ir_ctx->sig_cunt; idx++) { ctx->loc_act[idx] = UINT32_MAX; }

    if (ctx->pf_is == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_str_emit(ctx, "pfk = hdl_pf_now();\npf_s[0] += pfk - pfa;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (ctx->residue_cunt > 0)
    {
        uint32_t cls_pass = 0;

        cur_rgn = UINT32_MAX;

        for (cls_pass = 0; cls_pass < 0x3; cls_pass++)
        {
            if (cls_pass == 0x1)
            {
                uint32_t cw_j = 0;
                uint32_t cw_has = 0;

                if (cur_rgn != UINT32_MAX)
                {
                    ret = crosstalk_hdl_rgn_close_emit(ctx, cur_rgn, LDG_TRUTH_TRUE);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    cur_rgn = UINT32_MAX;
                }

                if (ctx->pf_is == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_str_emit(ctx, "pfa = hdl_pf_now();\npf_s[1] += pfa - pfk;\nfor (pass = 0x0; pass < "); }
                else { ret = crosstalk_hdl_str_emit(ctx, "for (pass = 0x0; pass < "); }

                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                ret = crosstalk_hdl_dec_emit(ctx, CROSSTALK_HDL_EMIT_SETTLE_MAX);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                ret = crosstalk_hdl_str_emit(ctx, "; pass++)\n{\naw = ");
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                for (cw_j = 0; cw_j < ctx->rw_cunt; cw_j++)
                {
                    uint64_t cmask = 0;
                    uint32_t cb_j = 0;

                    for (cb_j = 0; cb_j < 64; cb_j++)
                    {
                        uint32_t crgn = cw_j * 64 + cb_j;

                        if (crgn >= ctx->rgn_core_lo && crgn < ctx->rgn_core_hi) { cmask |= (uint64_t)0x1 << cb_j; }
                    }

                    if (cmask == 0x0) { continue; }

                    if (cw_has != 0x0)
                    {
                        ret = crosstalk_hdl_str_emit(ctx, " | ");
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
                    }

                    ret = crosstalk_hdl_str_emit(ctx, "(ab[");
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    ret = crosstalk_hdl_dec_emit(ctx, cw_j);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    ret = crosstalk_hdl_str_emit(ctx, "] & ");
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    ret = crosstalk_hdl_hex_emit(ctx, cmask);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    ret = crosstalk_hdl_str_emit(ctx, ")");
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    cw_has = 0x1;
                }

                if (cw_has == 0x0)
                {
                    ret = crosstalk_hdl_str_emit(ctx, "0x0");
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
                }

                ret = crosstalk_hdl_str_emit(ctx, ";\nif (aw == 0x0) { break; }\n");
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            if (cls_pass == 0x2)
            {
                if (cur_rgn != UINT32_MAX)
                {
                    ret = crosstalk_hdl_rgn_close_emit(ctx, cur_rgn, LDG_TRUTH_FALSE);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    cur_rgn = UINT32_MAX;
                }

                ret = crosstalk_hdl_str_emit(ctx, "}\nif (pass >= ");
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                ret = crosstalk_hdl_dec_emit(ctx, CROSSTALK_HDL_EMIT_SETTLE_MAX);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                ret = crosstalk_hdl_str_emit(ctx, ") { return ");
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                ret = crosstalk_hdl_dec_emit(ctx, LDG_ERR_BUSY);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                if (ctx->pf_is == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_str_emit(ctx, "; }\npfk = hdl_pf_now();\npf_s[2] += pfk - pfa;\npf_s[3] += (hdl_u64)pass;\n"); }
                else { ret = crosstalk_hdl_str_emit(ctx, "; }\n"); }

                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            for (idx = 0; idx < ctx->residue_cunt; idx++)
            {
                uint32_t rop_idx = ctx->residue[idx];
                uint32_t rws = 0;

                if (rcls[rop_idx] != cls_pass) { continue; }

                if (ctx->fused[rop_idx] != 0x0) { continue; }

                if (konst_ok[rop_idx] != LDG_TRUTH_FALSE) { continue; }

                if (ctx->rgn_of[rop_idx] != cur_rgn)
                {
                    if (cur_rgn != UINT32_MAX)
                    {
                        ret = crosstalk_hdl_rgn_close_emit(ctx, cur_rgn, cls_pass == 0x1 ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
                    }

                    cur_rgn = ctx->rgn_of[rop_idx];
                    ret = crosstalk_hdl_rgn_gate_emit(ctx, cur_rgn);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
                }

                rws = crosstalk_hdl_emit_wsig_get(&ir_ctx->comb_ops[rop_idx]);

                ctx->rgn_chk = 0x0;

                if (cls_pass == 0x1 && ctx->loc_ok[rop_idx] == 0x0) { ctx->rgn_chk = 0x1; }

                if (ctx->vf_prev[rop_idx] != UINT32_MAX) { ctx->vf_op = ctx->vf_prev[rop_idx]; ctx->vf_sig = ir_ctx->comb_ops[ctx->vf_op].dst; }

                if (ctx->loc_ok[rop_idx] != 0x0) { ret = crosstalk_hdl_loc_stmt_emit(ctx, &ir_ctx->comb_ops[rop_idx], rop_idx, "tl"); }
                else { ret = crosstalk_hdl_comb_stmt_emit(ctx, &ir_ctx->comb_ops[rop_idx]); }

                ctx->vf_op = UINT32_MAX;
                ctx->vf_sig = UINT32_MAX;
                ctx->rgn_chk = 0x0;

                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                if (rws != UINT32_MAX) { ctx->loc_act[rws] = ctx->loc_ok[rop_idx] != 0x0 ? rop_idx : UINT32_MAX; }
            }
        }

        if (cur_rgn != UINT32_MAX)
        {
            ret = crosstalk_hdl_rgn_close_emit(ctx, cur_rgn, LDG_TRUTH_FALSE);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        for (idx = 0; idx < ir_ctx->sig_cunt; idx++) { ctx->loc_act[idx] = UINT32_MAX; }

        if (ctx->pf_is == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_str_emit(ctx, "pf_s[4] += hdl_pf_now() - pfk;\nreturn 0x0;\n}\n\n"); }
        else { ret = crosstalk_hdl_str_emit(ctx, "return 0x0;\n}\n\n"); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    else
    {
        ret = crosstalk_hdl_str_emit(ctx, "return 0x0;\n}\n\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (ctx->pf_is == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_str_emit(ctx, "static hdl_u32 hdl_gen_comb_pf(void)\n{\nhdl_u64 t2;\nhdl_u32 r2;\nt2 = hdl_pf_now();\nr2 = hdl_gen_comb();\nt2 = hdl_pf_now() - t2;\npf_t[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, ir_ctx->seq_blk_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "] += t2;\npf_c[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, ir_ctx->seq_blk_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "] += 0x1;\npf_cur_comb += t2;\nreturn r2;\n}\n\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    else
    {
        ret = crosstalk_hdl_str_emit(ctx, "static hdl_u32 hdl_gen_comb_pf(void)\n{\nreturn hdl_gen_comb();\n}\n\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (idx = 0; idx < ir_ctx->seq_blk_cunt; idx++)
    {
        ret = crosstalk_hdl_str_emit(ctx, "static void hdl_gen_b");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "(void)\n{\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "hdl_u64 nx;\nnx = 0x0;\nnx = nx;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (ctx->wide_has == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_str_emit(ctx, "hdl_u64 nxh;\nnxh = 0x0;\nnxh = nxh;\n");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        for (op_idx = 0; op_idx < ir_ctx->seq_blks[idx].op_cunt; op_idx++)
        {
            uint32_t seq_idx = ir_ctx->seq_blks[idx].op_start + op_idx;

            if (ctx->loc_seq_ok[seq_idx] == 0x0) { continue; }

            ret = crosstalk_hdl_str_emit(ctx, "hdl_u64 ts");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_dec_emit(ctx, seq_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, ";\n");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        {
            uint32_t hoist_guard = UINT32_MAX;

            for (op_idx = 0; op_idx < ir_ctx->seq_blks[idx].op_cunt; op_idx++)
            {
                uint32_t seq_idx = ir_ctx->seq_blks[idx].op_start + op_idx;
                const crosstalk_hdl_ir_op_t *sqp = &ir_ctx->seq_ops[seq_idx];
                uint32_t want = UINT32_MAX;

                if (ctx->fused_seq[seq_idx] != 0x0) { continue; }

                if (ctx->loc_seq_ok[seq_idx] == 0x0 && crosstalk_hdl_emit_hoist_is(ir_ctx, ir_ctx->seq_blks[idx].op_start, ir_ctx->seq_blks[idx].op_cunt, seq_idx) == LDG_TRUTH_TRUE) { want = sqp->guard; }

                if (want != hoist_guard)
                {
                    if (hoist_guard != UINT32_MAX)
                    {
                        ret = crosstalk_hdl_str_emit(ctx, "}\n");
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
                    }

                    if (want != UINT32_MAX)
                    {
                        ret = crosstalk_hdl_emit_guard_open_emit(ctx, want);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
                    }

                    hoist_guard = want;
                }

                if (ctx->loc_seq_ok[seq_idx] != 0x0) { ret = crosstalk_hdl_loc_stmt_emit(ctx, sqp, seq_idx, "ts"); }
                else { ret = crosstalk_hdl_seq_stmt_emit(ctx, sqp, seq_idx); }

                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                if (sqp->kind != CROSSTALK_HDL_IR_OP_REG_NEXT && sqp->kind != CROSSTALK_HDL_IR_OP_MEM_WR && sqp->dst != UINT32_MAX) { ctx->loc_act[sqp->dst] = ctx->loc_seq_ok[seq_idx] != 0x0 ? ir_ctx->comb_cunt + seq_idx : UINT32_MAX; }
            }

            if (hoist_guard != UINT32_MAX)
            {
                ret = crosstalk_hdl_str_emit(ctx, "}\n");
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
        }

        for (op_idx = 0; op_idx < ir_ctx->sig_cunt; op_idx++) { ctx->loc_act[op_idx] = UINT32_MAX; }

        ret = crosstalk_hdl_str_emit(ctx, "}\n\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_str_emit(ctx, "static void hdl_gen_commit(void)\n{\nhdl_u64 nx;\nnx = 0x0;\nnx = nx;\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->rgn_or_ra = 0x1;

    if (ctx->wide_has == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_str_emit(ctx, "hdl_u64 nxh;\nnxh = 0x0;\nnxh = nxh;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_str_emit(ctx, "{\nhdl_u32 j3;\nhdl_u32 s3;\nhdl_u32 m3;\nm3 = 0x0;\nm3 = m3;\nfor (j3 = 0x0; j3 < sjn; j3++)\n{\ns3 = sj[j3];\nsjb[s3 >> 6] &= ~((hdl_u64)0x1 << (s3 & 63));\nif (s3 < ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, ctx->rn_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, ")\n{\nnx = rn[s3] & jmk[s3];\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->wide_has == LDG_TRUTH_TRUE)
    {
        if (ctx->pf_is == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_str_emit(ctx, "if (jwd[s3] == 0x0)\n{\nif (v[jsg[s3]] != nx) { v[jsg[s3]] = nx; cmt = 0x1; pf_s[5] += 0x1; hdl_rgn_sig(jsg[s3]); }\n}\nelse\n{\nnxh = rnh[s3] & jmh[s3];\nif (v[jsg[s3]] != nx || vh[jsg[s3]] != nxh) { v[jsg[s3]] = nx; vh[jsg[s3]] = nxh; cmt = 0x1; pf_s[5] += 0x1; hdl_rgn_sig(jsg[s3]); }\n}\n"); }
        else { ret = crosstalk_hdl_str_emit(ctx, "if (jwd[s3] == 0x0)\n{\nif (v[jsg[s3]] != nx) { v[jsg[s3]] = nx; cmt = 0x1; hdl_rgn_sig(jsg[s3]); }\n}\nelse\n{\nnxh = rnh[s3] & jmh[s3];\nif (v[jsg[s3]] != nx || vh[jsg[s3]] != nxh) { v[jsg[s3]] = nx; vh[jsg[s3]] = nxh; cmt = 0x1; hdl_rgn_sig(jsg[s3]); }\n}\n"); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    else
    {
        if (ctx->pf_is == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_str_emit(ctx, "if (v[jsg[s3]] != nx) { v[jsg[s3]] = nx; cmt = 0x1; pf_s[5] += 0x1; hdl_rgn_sig(jsg[s3]); }\n"); }
        else { ret = crosstalk_hdl_str_emit(ctx, "if (v[jsg[s3]] != nx) { v[jsg[s3]] = nx; cmt = 0x1; hdl_rgn_sig(jsg[s3]); }\n"); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_str_emit(ctx, "}\nelse\n{\nm3 = s3 - ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_dec_emit(ctx, ctx->rn_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, ";\nif (mwe[m3] != 0x0 && mwa[m3] < (hdl_u64)jdp[m3])\n{\nnx = mwv[m3] & jmk[s3];\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->wide_has == LDG_TRUTH_TRUE)
    {
        if (ctx->pf_is == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_str_emit(ctx, "if (jwd[s3] == 0x0)\n{\nif (m[jmo[m3] + mwa[m3]] != nx) { m[jmo[m3] + mwa[m3]] = nx; cmt = 0x1; pf_s[6] += 0x1; hdl_rgn_sig(jsg[s3]); }\n}\nelse\n{\nnxh = mwvh[m3] & jmh[s3];\nif (m[jmo[m3] + mwa[m3]] != nx || mh[jmo[m3] + mwa[m3]] != nxh) { m[jmo[m3] + mwa[m3]] = nx; mh[jmo[m3] + mwa[m3]] = nxh; cmt = 0x1; pf_s[6] += 0x1; hdl_rgn_sig(jsg[s3]); }\n}\n"); }
        else { ret = crosstalk_hdl_str_emit(ctx, "if (jwd[s3] == 0x0)\n{\nif (m[jmo[m3] + mwa[m3]] != nx) { m[jmo[m3] + mwa[m3]] = nx; cmt = 0x1; hdl_rgn_sig(jsg[s3]); }\n}\nelse\n{\nnxh = mwvh[m3] & jmh[s3];\nif (m[jmo[m3] + mwa[m3]] != nx || mh[jmo[m3] + mwa[m3]] != nxh) { m[jmo[m3] + mwa[m3]] = nx; mh[jmo[m3] + mwa[m3]] = nxh; cmt = 0x1; hdl_rgn_sig(jsg[s3]); }\n}\n"); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    else
    {
        if (ctx->pf_is == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_str_emit(ctx, "if (m[jmo[m3] + mwa[m3]] != nx) { m[jmo[m3] + mwa[m3]] = nx; cmt = 0x1; pf_s[6] += 0x1; hdl_rgn_sig(jsg[s3]); }\n"); }
        else { ret = crosstalk_hdl_str_emit(ctx, "if (m[jmo[m3] + mwa[m3]] != nx) { m[jmo[m3] + mwa[m3]] = nx; cmt = 0x1; hdl_rgn_sig(jsg[s3]); }\n"); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_str_emit(ctx, "}\n}\n}\nsjn = 0x0;\n}\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < ir_ctx->seq_blk_cunt; idx++)
    {
        ret = crosstalk_hdl_str_emit(ctx, "fired[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "] = 0x0;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (ctx->pf_is == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_str_emit(ctx, "}\n\nstatic void hdl_gen_commit_pf(void)\n{\nhdl_u64 t3;\nt3 = hdl_pf_now();\nhdl_gen_commit();\nt3 = hdl_pf_now() - t3;\npf_t[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, ir_ctx->seq_blk_cunt + 0x1);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "] += t3;\npf_c[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, ir_ctx->seq_blk_cunt + 0x1);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "] += 0x1;\npf_cur_cmt += t3;\n}\n\nstatic hdl_u32 hdl_gen_step(void)\n{\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    else
    {
        ret = crosstalk_hdl_str_emit(ctx, "}\n\nstatic void hdl_gen_commit_pf(void)\n{\nhdl_gen_commit();\n}\n\nstatic hdl_u32 hdl_gen_step(void)\n{\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (ir_ctx->seq_blk_cunt > 0)
    {
        ret = crosstalk_hdl_str_emit(ctx, "hdl_u64 cv;\nhdl_u64 rv;\nhdl_u32 f;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_str_emit(ctx, "hdl_u32 rr;\nrr = hdl_gen_comb_pf();\nif (rr != 0x0) { return rr; }\nstg = 0x0;\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < ir_ctx->seq_blk_cunt; idx++)
    {
        ret = crosstalk_hdl_scan_blk_emit(ctx, idx, LDG_TRUTH_FALSE);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (ctx->pf_is == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_str_emit(ctx, "hdl_gen_commit_pf();\nreturn hdl_gen_comb_pf();\n}\n\nstatic hdl_u32 hdl_gen_cycle(hdl_u32 ck)\n{\nhdl_u64 pft0;\n"); }
    else { ret = crosstalk_hdl_str_emit(ctx, "hdl_gen_commit_pf();\nreturn hdl_gen_comb_pf();\n}\n\nstatic hdl_u32 hdl_gen_cycle(hdl_u32 ck)\n{\n"); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ir_ctx->seq_blk_cunt > 0)
    {
        ret = crosstalk_hdl_str_emit(ctx, "hdl_u64 cv;\nhdl_u64 rv;\nhdl_u32 f;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (fast_ok != 0x0)
    {
        if (ctx->pf_is == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_str_emit(ctx, "hdl_u32 rr;\npft0 = hdl_pf_now();\npf_cur_comb = 0x0;\npf_cur_blk = 0x0;\npf_cur_cmt = 0x0;\nif (pkd != 0x0) { rr = hdl_gen_comb_pf(); if (rr != 0x0) { return rr; } pkd = 0x0; }\n"); }
        else { ret = crosstalk_hdl_str_emit(ctx, "hdl_u32 rr;\nif (pkd != 0x0) { rr = hdl_gen_comb_pf(); if (rr != 0x0) { return rr; } pkd = 0x0; }\n"); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (clk_root != UINT32_MAX)
        {
            ret = crosstalk_hdl_str_emit(ctx, "if (ck != ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_dec_emit(ctx, clk_root);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, ") { return ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_dec_emit(ctx, LDG_ERR_FUNC_ARG_INVALID);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, "; }\n");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_str_emit(ctx, "v[ck] = 0x1;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (clk_root != UINT32_MAX) { ret = crosstalk_hdl_rgn_or_emit(ctx, clk_root); }
        else { ret = crosstalk_hdl_str_emit(ctx, "hdl_rgn_sig(ck);"); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        for (alias_i = 0; alias_i < alias_cunt; alias_i++)
        {
            ret = crosstalk_hdl_vref_emit(ctx, alias_arr[alias_i]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " = 0x1;");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_rgn_or_emit(ctx, alias_arr[alias_i]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, "\n");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_str_emit(ctx, "stg = 0x0;\n");
    }
    else if (ctx->pf_is == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_str_emit(ctx, "hdl_u32 rr;\npft0 = hdl_pf_now();\npf_cur_comb = 0x0;\npf_cur_blk = 0x0;\npf_cur_cmt = 0x0;\nv[ck] = 0x1;\nhdl_rgn_sig(ck);\nrr = hdl_gen_comb_pf();\nif (rr != 0x0) { return rr; }\nstg = 0x0;\n"); }
    else { ret = crosstalk_hdl_str_emit(ctx, "hdl_u32 rr;\nv[ck] = 0x1;\nhdl_rgn_sig(ck);\nrr = hdl_gen_comb_pf();\nif (rr != 0x0) { return rr; }\nstg = 0x0;\n"); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < ir_ctx->seq_blk_cunt; idx++)
    {
        if (crosstalk_hdl_emit_blk_direct_is(ir_ctx, idx, fast_ok) == LDG_TRUTH_TRUE)
        {
            uint32_t blk_guards[8];
            uint32_t blk_guard_cunt = 0;
            uint32_t g_at = 0;

            if (crosstalk_hdl_emit_blk_guard_get(ctx, idx, blk_guards, &blk_guard_cunt) == LDG_TRUTH_TRUE)
            {
                ret = crosstalk_hdl_str_emit(ctx, "if (");
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                for (g_at = 0; g_at < blk_guard_cunt; g_at = g_at + 0x1)
                {
                    if (g_at != 0)
                    {
                        ret = crosstalk_hdl_str_emit(ctx, " || ");
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
                    }

                    ret = crosstalk_hdl_vref_emit(ctx, blk_guards[g_at]);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    ret = crosstalk_hdl_str_emit(ctx, " != 0x0");
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
                }

                ret = crosstalk_hdl_str_emit(ctx, ") { ");
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                ret = crosstalk_hdl_fire_blk_emit(ctx, idx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                ret = crosstalk_hdl_str_emit(ctx, "}\n");
            }
            else { ret = crosstalk_hdl_fire_blk_emit(ctx, idx); }
        }
        else { ret = crosstalk_hdl_scan_blk_emit(ctx, idx, LDG_TRUTH_FALSE); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (fast_ok != 0x0)
    {
        ret = crosstalk_hdl_str_emit(ctx, "hdl_gen_commit_pf();\nv[ck] = 0x0;\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (clk_root != UINT32_MAX) { ret = crosstalk_hdl_rgn_or_emit(ctx, clk_root); }
        else { ret = crosstalk_hdl_str_emit(ctx, "hdl_rgn_sig(ck);"); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        for (alias_i = 0; alias_i < alias_cunt; alias_i++)
        {
            ret = crosstalk_hdl_vref_emit(ctx, alias_arr[alias_i]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " = 0x0;");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_rgn_or_emit(ctx, alias_arr[alias_i]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, "\n");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = crosstalk_hdl_str_emit(ctx, "stg = 0x0;\n");
    }
    else { ret = crosstalk_hdl_str_emit(ctx, "hdl_gen_commit_pf();\nv[ck] = 0x0;\nhdl_rgn_sig(ck);\nrr = hdl_gen_comb_pf();\nif (rr != 0x0) { return rr; }\nstg = 0x0;\n"); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < ir_ctx->seq_blk_cunt; idx++)
    {
        if (crosstalk_hdl_emit_blk_direct_is(ir_ctx, idx, fast_ok) == LDG_TRUTH_TRUE) { continue; }

        ret = crosstalk_hdl_scan_blk_emit(ctx, idx, LDG_TRUTH_TRUE);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (fast_ok != 0x0 && ctx->pf_is == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_str_emit(ctx, "hdl_gen_commit_pf();\nrr = 0x0;\nif (cmt != 0x0) { cmt = 0x0; rr = hdl_gen_comb_pf(); }\npfr_meta[0] = pfr_meta[1] & (pfr_meta[3] - 0x1);\npfr_tot[pfr_meta[0]] = hdl_pf_now() - pft0;\npfr_comb[pfr_meta[0]] = pf_cur_comb;\npfr_blk[pfr_meta[0]] = pf_cur_blk;\npfr_cmt[pfr_meta[0]] = pf_cur_cmt;\npfr_meta[1] = pfr_meta[1] + 0x1;\nreturn rr;\n}\n\n"); }
    else if (fast_ok != 0x0) { ret = crosstalk_hdl_str_emit(ctx, "hdl_gen_commit_pf();\nrr = 0x0;\nif (cmt != 0x0) { cmt = 0x0; rr = hdl_gen_comb_pf(); }\nreturn rr;\n}\n\n"); }
    else if (ctx->pf_is == LDG_TRUTH_TRUE) { ret = crosstalk_hdl_str_emit(ctx, "hdl_gen_commit_pf();\nif (stg != 0x0) { rr = hdl_gen_comb_pf(); if (rr != 0x0) { return rr; } }\npfr_meta[0] = pfr_meta[1] & (pfr_meta[3] - 0x1);\npfr_tot[pfr_meta[0]] = hdl_pf_now() - pft0;\npfr_comb[pfr_meta[0]] = pf_cur_comb;\npfr_blk[pfr_meta[0]] = pf_cur_blk;\npfr_cmt[pfr_meta[0]] = pf_cur_cmt;\npfr_meta[1] = pfr_meta[1] + 0x1;\nreturn 0x0;\n}\n\n"); }
    else { ret = crosstalk_hdl_str_emit(ctx, "hdl_gen_commit_pf();\nif (stg != 0x0) { rr = hdl_gen_comb_pf(); if (rr != 0x0) { return rr; } }\nreturn 0x0;\n}\n\n"); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "static void hdl_gen_init(void)\n{\nhdl_u32 mi = 0x0;\nmi = mi;\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < ir_ctx->sig_cunt; idx++)
    {
        uint64_t masked = ir_ctx->sigs[idx].init_val & crosstalk_hdl_emit_mask_get(ir_ctx->sigs[idx].width);

        if (masked == 0) { continue; }

        if (ir_ctx->sigs[idx].depth > 0)
        {
            if (ctx->mem_off[idx] == UINT32_MAX) { continue; }

            ret = crosstalk_hdl_str_emit(ctx, "for (mi = 0x0; mi < ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_dec_emit(ctx, ir_ctx->sigs[idx].depth);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, "; mi++) { m[");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_dec_emit(ctx, ctx->mem_off[idx]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, " + mi] = ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_hex_emit(ctx, masked);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_hdl_str_emit(ctx, "; }\n");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            continue;
        }

        ret = crosstalk_hdl_vref_emit(ctx, idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, " = ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_hex_emit(ctx, masked);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, ";\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (idx = 0; idx < ir_ctx->mem_init_cunt; idx++)
    {
        const crosstalk_hdl_ir_mem_init_t *init = &ir_ctx->mem_inits[idx];

        if (init->sig >= ir_ctx->sig_cunt || ctx->mem_off[init->sig] == UINT32_MAX || init->addr >= (uint64_t)ir_ctx->sigs[init->sig].depth) { return LDG_ERR_BOUNDS; }

        ret = crosstalk_hdl_str_emit(ctx, "m[");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, ctx->mem_off[init->sig] + (uint32_t)init->addr);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, "] = ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_hex_emit(ctx, init->val & crosstalk_hdl_emit_mask_get(ir_ctx->sigs[init->sig].width));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, ";\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (ctx->pf_is == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_str_emit(ctx, "hdl_gen_konst();\npfr_meta[2] = ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, ir_ctx->seq_blk_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        for (idx = 0; idx < ctx->residue_cunt; idx++) { if (rcls[ctx->residue[idx]] == 0x1) { pf_core_cunt++; } }

        ret = crosstalk_hdl_str_emit(ctx, ";\npfr_meta[3] = 4096;\npfr_meta[4] = ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, ctx->rn_cunt + ctx->mw_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, ";\npfr_meta[5] = ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, ctx->order_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, ";\npfr_meta[6] = ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, ctx->residue_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, ";\npfr_meta[7] = ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, pf_core_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, ";\npfr_meta[8] = ");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_dec_emit(ctx, ctx->rgn_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = crosstalk_hdl_str_emit(ctx, ";\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    else
    {
        ret = crosstalk_hdl_str_emit(ctx, "hdl_gen_konst();\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_str_emit(ctx, "hdl_gen_touch();\n}\n\nstatic hdl_u32 hdl_gen_sig_find(const char *snm)\n{\nhdl_u32 i;\nfor (i = 0x0; i < HDL_GEN_SIG_CUNT; i++) { if (nmt[i][0x0] != 0x0 && strcmp(nmt[i], snm) == 0x0) { return i; } }\nreturn 4294967295;\n}\n\nstatic void hdl_gen_poke(hdl_u32 i, hdl_u64 pv)\n{\nif (i < HDL_GEN_SIG_CUNT) { v[i] = pv & mk[i]; hdl_rgn_sig(i); pkd = 0x1; }\n}\n\nstatic hdl_u64 hdl_gen_peek(hdl_u32 i)\n{\nif (i < HDL_GEN_SIG_CUNT) { return v[i]; }\nreturn 0x0;\n}\n\nstatic void hdl_gen_mem_st(hdl_u32 i, hdl_u64 ad, hdl_u64 sv)\n{\nif (i < HDL_GEN_SIG_CUNT && dp[i] > 0x0 && ad < (hdl_u64)dp[i]) { m[mo[i] + ad] = sv & mk[i]; hdl_rgn_sig(i); pkd = 0x1; }\n}\n\nstatic hdl_u64 hdl_gen_mem_ld(hdl_u32 i, hdl_u64 ad)\n{\nif (i < HDL_GEN_SIG_CUNT && dp[i] > 0x0 && ad < (hdl_u64)dp[i]) { return m[mo[i] + ad]; }\nreturn 0x0;\n}\n\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_emit_export_emit(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "#ifdef HDL_GEN_MAIN\n#include <stdio.h>\n#include <stdlib.h>\n#include <in" "ttypes.h>\n#include <termios.h>\n#include <unistd.h>\n#include <fcntl.h>\n\n#ifndef HDL_GEN_CYCLES\n#define HDL_GEN_CYCLES 8000000\n#endif\n\nstatic const hdl_u32 hdl_scan_tbl[128] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 14, 15, 28, 0x0, 0x0, 28, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 57, 0x2, 40, 0x4, 0x5, 0x6, 0x8, 40, 10, 11, 0x9, 13, 51, 12, 52, 53, 11, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 10, 39, 39, 51, 13, 52, 53, 0x3, 30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38, 50, 49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44, 26, 43, 27, 0x7, 12, 41, 30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38, 50, 49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44, 26, 43, 27, 41, 14};\n\nstatic void hdl_gen_fb_dump(hdl_u32 fmi, hdl_u32 fpi, hdl_u64 fba, hdl_u32 fbn)\n{\nFILE *ff2;\nchar fnb[64];\nchar pxb[3];\nhdl_u32 px2;\nhdl_u64 cvl;\nsnprin" "tf(fnb, sizeof(fnb), \"fb_%06\" \"u.ppm\", fbn);\nff2 = fopen(fnb, \"wb\");\nif (ff2 == 0x0) { return; }\nfprin" "tf(ff2, \"P6\\n320 200\\n255\\n\");\nfor (px2 = 0x0; px2 < 64000; px2++)\n{\ncvl = hdl_gen_mem_ld(fmi, fba + px2) & 255;\npxb[0x0] = (char)((hdl_gen_mem_ld(fpi, cvl * 0x3) & 63) << 0x2);\npxb[0x1] = (char)((hdl_gen_mem_ld(fpi, cvl * 0x3 + 0x1) & 63) << 0x2);\npxb[0x2] = (char)((hdl_gen_mem_ld(fpi, cvl * 0x3 + 0x2) & 63) << 0x2);\nfwr" "ite(pxb, 0x1, 0x3, ff2);\n}\nfclose(ff2);\n}\n\nin" "t main(in" "t argc, char **argv)\n{\nFILE *hf;\nFILE *kfp = 0x0;\nhdl_i64 gc;\nhdl_i64 knb = 0x0;\nhdl_u64 kpv = 0x0;\nhdl_u64 kav = 0x0;\nhdl_u64 kgp = 0x0;\nhdl_u64 sca = 0x0;\nhdl_u64 chv = 0x0;\nhdl_u32 scn = 0x0;\nhdl_u32 kcs = 4294967295;\nhdl_u32 krs = 4294967295;\nhdl_u32 kas = 4294967295;\nhdl_u32 ai2;\nhdl_u32 mi2;\nhdl_u32 rw2;\nhdl_u32 co2;\nchar pch;\nFILE *stf = 0x0;\nconst char *svp = 0x0;\nconst char *ldp = 0x0;\nhdl_u32 sdn = 0x0;\nhdl_u32 dmp = 0x0;\nhdl_u32 rawmode = 0x0;\nhdl_u32 kq2[64];\nhdl_u32 kqh = 0x0;\nhdl_u32 kqt = 0x0;\nhdl_u64 srn = 0x0;\nhdl_u32 cyc_lim = HDL_GEN_CYCLES;\nhdl_u64 fba = 655360;\nhdl_u32 fbe = 0x0;\nhdl_u32 fbf = 0x0;\nhdl_u32 fbn = 0x0;\nhdl_u32 fmi = 4294967295;\nhdl_u32 fpi = 4294967295;\nstruct termios tio_o;\nstruct termios tio_r;\nhdl_u64 wv = 0x0;\nhdl_u64 aw = 0x0;\nhdl_u64 sp = 0x0;\nhdl_u64 sv = 0x0;\nhdl_u64 lp = 0x0;\nhdl_u64 lv = 0x0;\nhdl_u64 lo = 0x0;\nhdl_u32 dc = 0x0;\nhdl_u32 hj;\nhdl_u32 am;\nhdl_u64 sw;\nhdl_u32 ck;\nhdl_u32 rs;\nhdl_u32 se;\nhdl_u32 le;\nhdl_u32 lc;\nhdl_u32 hb;\nhdl_u32 cy;\nhdl_u32 rr;\nhdl_gen_init();\nfor (ai2 = 0x1; (hdl_u32)argc > ai2; ai2++)\n{\nif (strcmp(argv[ai2], \"-stdin\") == 0x0) { sdn = 0x1; }\nelse if (strcmp(argv[ai2], \"-dump\") == 0x0) { dmp = 0x1; }\nelse if ((hdl_u32)argc > ai2 + 0x1 && strcmp(argv[ai2], \"-keys\") == 0x0) { kfp = fopen(argv[ai2 + 0x1], \"rb\"); if (kfp == 0x0) { prin" "tf(\"keys file missing: %s\\n\", argv[ai2 + 0x1]); } ai2++; }\nelse if ((hdl_u32)argc > ai2 + 0x1 && strcmp(argv[ai2], \"-screen\") == 0x0) { sca = strtoull(argv[ai2 + 0x1], 0x0, 0x0); scn = 0x1; ai2++; }\nelse if ((hdl_u32)argc > ai2 + 0x1 && strcmp(argv[ai2], \"-save\") == 0x0) { svp = argv[ai2 + 0x1]; ai2++; }\nelse if ((hdl_u32)argc > ai2 + 0x1 && strcmp(argv[ai2], \"-load\") == 0x0) { ldp = argv[ai2 + 0x1]; ai2++; }\nelse if ((hdl_u32)argc > ai2 + 0x1 && strcmp(argv[ai2], \"-cycles\") == 0x0) { cyc_lim = (hdl_u32)strtoull(argv[ai2 + 0x1], 0x0, 0x0); ai2++; }\nelse if ((hdl_u32)argc > ai2 + 0x1 && strcmp(argv[ai2], \"-fb\") == 0x0) { fba = strtoull(argv[ai2 + 0x1], 0x0, 16); fbf = 0x1; ai2++; }\nelse if ((hdl_u32)argc > ai2 + 0x1 && strcmp(argv[ai2], \"-fbevery\") == 0x0) { fbe = (hdl_u32)strtoull(argv[ai2 + 0x1], 0x0, 0x0); fbf = 0x1; ai2++; }\n}\nfor (hj = 0x0; hj < HDL_GEN_HEX_CUNT && ldp == 0x0; hj++)\n{\nhf = fopen(hx_path[hj], \"rb\");\nif (hf == 0x0) { prin" "tf(\"hex img missing: %s\\n\", hx_path[hj]); continue; }\naw = 0x0;\nwv = 0x0;\ndc = 0x0;\nam = 0x0;\nsw = 0x0;\ngc = (hdl_i64)fgetc(hf);\nwhile (gc != (hdl_i64)EOF)\n{\nchar hc = (char)gc;\nhdl_u32 hx_is = 0x0;\nhdl_u64 nb = 0x0;\nif (hc >= 0x30 && hc <= 0x39) { nb = (hdl_u64)(hdl_u32)(hc - 0x30); hx_is = 0x1; }\nelse if (hc >= 0x61 && hc <= 0x66) { nb = (hdl_u64)(hdl_u32)(hc - 0x61) + 10; hx_is = 0x1; }\nelse if (hc >= 0x41 && hc <= 0x46) { nb = (hdl_u64)(hdl_u32)(hc - 0x41) + 10; hx_is = 0x1; }\nif (hx_is != 0x0) { wv = wv * 16 + nb; dc++; }\nelse if (dc > 0x0)\n{\nif (am != 0x0) { aw = wv; am = 0x0; }\nelse { hdl_gen_mem_st(hx_sig[hj], aw, wv); aw++; sw++; }\nwv = 0x0;\ndc = 0x0;\n}\nif (hc == 0x40) { am = 0x1; }\ngc = (hdl_i64)fgetc(hf);\n}\nif (dc > 0x0 && am == 0x0) { hdl_gen_mem_st(hx_sig[hj], aw, wv); sw++; }\nfclose(hf);\nprin" "tf(\"hex img loaded; words: %\" PRIu64 \"\\n\", sw);\n}\nck = hdl_gen_sig_find(\"clk\");\nif (ck == 4294967295) { prin" "tf(\"clk sig missing\\n\"); return 0x1; }\nrs = hdl_gen_sig_find(\"btnrst\");\nif (rs == 4294967295) { prin" "tf(\"btnrst sig missing\\n\"); return 0x1; }\nse = hdl_gen_sig_find(\"serial_out\");\nif (se == 4294967295) { prin" "tf(\"serial_out sig missing\\n\"); }\nle = hdl_gen_sig_find(\"led_out\");\nif (le == 4294967295) { prin" "tf(\"led_out sig missing\\n\"); }\nlc = hdl_gen_sig_find(\"loc\");\nif (lc == 4294967295) { prin" "tf(\"loc sig missing\\n\"); }\nhb = hdl_gen_sig_find(\"hlt_out\");\nif (hb == 4294967295) { prin" "tf(\"hlt_out sig missing\\n\"); }\nif (ldp != 0x0)\n{\nstf = fopen(ldp, \"rb\");\nif (stf == 0x0) { prin" "tf(\"state file missing: %s\\n\", ldp); return 0x1; }\nsrn = srn + (hdl_u64)fr" "ead(v, sizeof(hdl_u64), HDL_GEN_SIG_CUNT, stf);\nsrn = srn + (hdl_u64)fr" "ead(vh, sizeof(hdl_u64), HDL_GEN_SIG_CUNT, stf);\nsrn = srn + (hdl_u64)fr" "ead(m, sizeof(hdl_u64), HDL_GEN_MEM_WORDS, stf);\nsrn = srn + (hdl_u64)fr" "ead(mh, sizeof(hdl_u64), HDL_GEN_MEM_WORDS, stf);\nsrn = srn + (hdl_u64)fr" "ead(pc, sizeof(hdl_u64), HDL_GEN_BLK_CUNT, stf);\nsrn = srn + (hdl_u64)fr" "ead(pr, sizeof(hdl_u64), HDL_GEN_BLK_CUNT, stf);\nfclose(stf);\nhdl_gen_touch();\nprin" "tf(\"state ld; items: %\" PRIu64 \"\\n\", srn);\n}\nelse\n{\nhdl_gen_poke(rs, 0x0);\nfor (cy = 0x0; cy < 8; cy++)\n{\nhdl_gen_poke(ck, 0x1);\nrr = hdl_gen_step();\nif (rr != 0x0) { prin" "tf(\"rst cycle failed; err: %u\\n\", rr); return 0x1; }\nhdl_gen_poke(ck, 0x0);\nrr = hdl_gen_step();\nif (rr != 0x0) { prin" "tf(\"rst cycle failed; err: %u\\n\", rr); return 0x1; }\n}\nhdl_gen_poke(rs, 0x1);\n}\nif (fbf != 0x0)\n{\nfor (mi2 = 0x0; mi2 < HDL_GEN_SIG_CUNT; mi2++) { if (dp[mi2] > 0x0 && strcmp(nmt[mi2], \"mem\") == 0x0) { fmi = mi2; break; } }\nfor (mi2 = 0x0; mi2 < HDL_GEN_SIG_CUNT; mi2++) { if (dp[mi2] > 0x0 && strcmp(nmt[mi2], \"vga_pal\") == 0x0) { fpi = mi2; break; } }\nif (fmi == 4294967295 || fpi == 4294967295) { fbf = 0x0; fbe = 0x0; prin" "tf(\"fb sigs missing\\n\"); }\n}\nif (kfp != 0x0 || sdn != 0x0)\n{\nkcs = hdl_gen_sig_find(\"key_code\");\nkrs = hdl_gen_sig_find(\"key_rdy\");\nkas = hdl_gen_sig_find(\"key_ack\");\nif (kcs == 4294967295 || krs == 4294967295 || kas == 4294967295) { if (kfp != 0x0) { fclose(kfp); } kfp = 0x0; sdn = 0x0; prin" "tf(\"key sigs missing\\n\"); }\n}\nif (kfp != 0x0)\n{\nknb = (hdl_i64)fgetc(kfp);\nif (knb != (hdl_i64)EOF) { hdl_gen_poke(kcs, (hdl_u64)knb); hdl_gen_poke(krs, 0x1); }\n}\nif (sdn != 0x0)\n{\ntcgetattr(0x0, &tio_o);\ntio_r = tio_o;\ntio_r.c_lflag = tio_r.c_lflag & (hdl_u32)~(ICANON | ECHO);\ntio_r.c_cc[VMIN] = 0x0;\ntio_r.c_cc[VTIME] = 0x0;\ntcsetattr(0x0, TCSANOW, &tio_r);\nfcntl(0x0, F_SETFL, fcntl(0x0, F_GETFL) | O_NONBLOCK);\nrawmode = 0x1;\n}\nif (scn != 0x0 && sdn != 0x0)\n{\nfor (mi2 = 0x0; mi2 < HDL_GEN_SIG_CUNT; mi2++) { if (dp[mi2] > 0x0 && strcmp(nmt[mi2], \"mem\") == 0x0) { break; } }\nif (mi2 < HDL_GEN_SIG_CUNT)\n{\nfor (rw2 = 0x0; rw2 < 25; rw2++)\n{\nprin" "tf(\"|\");\nfor (co2 = 0x0; co2 < 80; co2++)\n{\nchv = hdl_gen_mem_ld(mi2, sca + ((hdl_u64)rw2 * 80 + co2) * 0x2);\npch = (char)(chv & 255);\nif (pch < 32 || pch > 126) { pch = 46; }\nprin" "tf(\"%c\", pch);\n}\nprin" "tf(\"\\n\");\n}\n}\n}\nfor (cy = 0x0; cy < cyc_lim; cy++)\n{\nrr = hdl_gen_cycle(ck);\nif (rr != 0x0) { prin" "tf(\"cycle %u failed; err: %u\\n\", cy, rr); return 0x1; }\nif (sdn != 0x0 && (cy & 4095) == 0x0)\n{\nchar rb2;\nwhile (re" "ad(0x0, &rb2, 0x1) == 0x1)\n{\nhdl_u32 uc2 = ((hdl_u32)rb2) & 255;\nhdl_u32 sft = 0x0;\nif (uc2 == 29) { sdn = 0x0; cy = cyc_lim - 0x1; break; }\nif (uc2 == 127) { uc2 = 0x8; }\nif (uc2 >= 128 || hdl_scan_tbl[uc2] == 0x0) { continue; }\nif (uc2 >= 65 && uc2 <= 90) { sft = 0x1; }\nelse if (strchr(\"!@#$%^&*()_+{}:\\\"~|<>?\", (char)uc2) != 0x0) { sft = 0x1; }\nif (((kqt + 0x4) & 63) == kqh || ((kqt + 0x5) & 63) == kqh) { break; }\nif (sft != 0x0) { kq2[kqt] = 42; kqt = (kqt + 0x1) & 63; }\nkq2[kqt] = hdl_scan_tbl[uc2]; kqt = (kqt + 0x1) & 63;\nkq2[kqt] = hdl_scan_tbl[uc2] | 128; kqt = (kqt + 0x1) & 63;\nif (sft != 0x0) { kq2[kqt] = 170; kqt = (kqt + 0x1) & 63; }\n}\n}\nif (kfp != 0x0 || sdn != 0x0)\n{\nkav = hdl_gen_peek(kas);\nif (kav != 0x0 && kpv == 0x0)\n{\nhdl_gen_poke(krs, 0x0);\nif (kfp != 0x0) { knb = (hdl_i64)fgetc(kfp); }\nelse if (kqh != kqt) { kqh = (kqh + 0x1) & 63; }\nkgp = 64;\n}\nkpv = kav;\nif (kgp > 0x0) { kgp--; }\nif (kgp == 0x0 && hdl_gen_peek(krs) == 0x0)\n{\nif (kfp != 0x0 && knb != (hdl_i64)EOF) { hdl_gen_poke(kcs, (hdl_u64)knb); hdl_gen_poke(krs, 0x1); }\nelse if (sdn != 0x0 && kqh != kqt) { hdl_gen_poke(kcs, (hdl_u64)kq2[kqh]); hdl_gen_poke(krs, 0x1); }\n}\n}\nif (se != 4294967295)\n{\nsv = hdl_gen_peek(se);\nif (sv != sp)\n{\nif (sv != 0x0) { prin" "tf(\"%c\", (char)sv); }\nsp = sv;\n}\n}\nif (le != 4294967295)\n{\nlv = hdl_gen_peek(le);\nif (lv != lp)\n{\nprin" "tf(\"led: %\" PRIX64 \"; cycle: %u\\n\", lv, cy);\nlp = lv;\n}\n}\nif (fbe != 0x0 && (cy % fbe) == 0x0) { hdl_gen_fb_dump(fmi, fpi, fba, fbn); fbn++; }\nif (hb != 4294967295 && hdl_gen_peek(hb) != 0x0) { prin" "tf(\"\\nhlt; cycle: %u\", cy); break; }\n}\nif (rawmode != 0x0) { tcsetattr(0x0, TCSANOW, &tio_o); }\nif (svp != 0x0)\n{\nstf = fopen(svp, \"wb\");\nif (stf == 0x0) { prin" "tf(\"state save failed: %s\\n\", svp); }\nelse\n{\nsrn = 0x0;\nsrn = srn + (hdl_u64)fwr" "ite(v, sizeof(hdl_u64), HDL_GEN_SIG_CUNT, stf);\nsrn = srn + (hdl_u64)fwr" "ite(vh, sizeof(hdl_u64), HDL_GEN_SIG_CUNT, stf);\nsrn = srn + (hdl_u64)fwr" "ite(m, sizeof(hdl_u64), HDL_GEN_MEM_WORDS, stf);\nsrn = srn + (hdl_u64)fwr" "ite(mh, sizeof(hdl_u64), HDL_GEN_MEM_WORDS, stf);\nsrn = srn + (hdl_u64)fwr" "ite(pc, sizeof(hdl_u64), HDL_GEN_BLK_CUNT, stf);\nsrn = srn + (hdl_u64)fwr" "ite(pr, sizeof(hdl_u64), HDL_GEN_BLK_CUNT, stf);\nfclose(stf);\nprin" "tf(\"\\nstate saved; items: %\" PRIu64 \"\\n\", srn);\n}\n}\nif (scn != 0x0)\n{\nfor (mi2 = 0x0; mi2 < HDL_GEN_SIG_CUNT; mi2++) { if (dp[mi2] > 0x0 && strcmp(nmt[mi2], \"mem\") == 0x0) { break; } }\nif (mi2 < HDL_GEN_SIG_CUNT)\n{\nprin" "tf(\"\\nscreen; cycle: %u\\n\", cy);\nfor (rw2 = 0x0; rw2 < 25; rw2++)\n{\nprin" "tf(\"|\");\nfor (co2 = 0x0; co2 < 80; co2++)\n{\nchv = hdl_gen_mem_ld(mi2, sca + ((hdl_u64)rw2 * 80 + co2) * 0x2);\npch = (char)(chv & 255);\nif (pch < 32 || pch > 126) { pch = 46; }\nprin" "tf(\"%c\", pch);\n}\nprin" "tf(\"\\n\");\n}\n}\n}\nif (fbf != 0x0) { hdl_gen_fb_dump(fmi, fpi, fba, fbn); fbn++; ");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->pf_is == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_str_emit(ctx, "}\nprin" "tf(\"\\npf sub; order: %\" PRIu64 \"; heads: %\" PRIu64 \"; core: %\" PRIu64 \"; passes: %\" PRIu64 \"; tails: %\" PRIu64 \"; regw: %\" PRIu64 \"; memw: %\" PRIu64 \"; rgn execs: %\" PRIu64 \"; rgn cunt: %u\\n\", pf_s[0], pf_s[1], pf_s[2], pf_s[3], pf_s[4], pf_s[5], pf_s[6], pf_s[7], (hdl_u32)HDL_GEN_RGN_CUNT);\nprin" "tf(\"pf agg; comb t: %\" PRIu64 \"; comb calls: %\" PRIu64 \"; commit t: %\" PRIu64 \"; commit calls: %\" PRIu64 \"\\n\", pf_t[HDL_GEN_BLK_CUNT], pf_c[HDL_GEN_BLK_CUNT], pf_t[HDL_GEN_BLK_CUNT + 0x1], pf_c[HDL_GEN_BLK_CUNT + 0x1]);\n{\nhdl_u64 bt2 = 0x0;\nhdl_u32 bi2;\nfor (bi2 = 0x0; bi2 < HDL_GEN_BLK_CUNT; bi2++) { bt2 += pf_t[bi2]; }\nprin" "tf(\"pf blk; total t: %\" PRIu64 \"\\n\", bt2);\n}\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    else
    {
        ret = crosstalk_hdl_str_emit(ctx, "}\n");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = crosstalk_hdl_str_emit(ctx, "if (dmp != 0x0)\n{\nhdl_u32 ds2;\nhdl_u64 de2;\nprin" "tf(\"\\n\");\nfor (ds2 = 0x0; ds2 < HDL_GEN_SIG_CUNT; ds2++)\n{\nif (nmt[ds2][0x0] == 0x0 || dp[ds2] != 0x0) { continue; }\nprin" "tf(\"d %u %s: %\" PRIX64 \"\\n\", ds2, nmt[ds2], v[ds2]);\n}\nfor (ds2 = 0x0; ds2 < HDL_GEN_SIG_CUNT; ds2++)\n{\nif (nmt[ds2][0x0] == 0x0 || dp[ds2] == 0x0 || dp[ds2] > 256) { continue; }\nfor (de2 = 0x0; de2 < (hdl_u64)dp[ds2]; de2++)\n{\nprin" "tf(\"d %u %s %\" PRIu64 \": %\" PRIX64 \"\\n\", ds2, nmt[ds2], de2, hdl_gen_mem_ld(ds2, de2));\n}\n}\n}\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_str_emit(ctx, "prin" "tf(\"\\nrun done; cycles: %u\", cy);\nif (lc != 4294967295)\n{\nlo = hdl_gen_peek(lc);\nprin" "tf(\"; loc: %\" PRIX64, lo);\n}\nprin" "tf(\"\\n\");\nreturn 0x0;\n}\n#endif\n");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->out[ctx->out_len] = LDG_STR_TERM;

    *out_src = ctx->out;
    *out_len = ctx->out_len;

    return LDG_ERR_AOK;
}
