#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/file.h>

#include "hdl/netarr.h"

#define CROSSTALK_HDL_NETARR_LINE_CAP 512
#define CROSSTALK_HDL_NETARR_FILE_MODE 420

static uint32_t crosstalk_hdl_netarr_text_wr(ldg_io_file_t *out_file, const char *text)
{
    uint64_t wrote = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(out_file == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_io_file_wr(out_file, text, (uint64_t)strlen(text), &wrote);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_netarr_lit_wr(ldg_io_file_t *out_file, uint32_t lit)
{
    char line[CROSSTALK_HDL_NETARR_LINE_CAP] = LDG_ARR_ZERO_INIT;
    int32_t wrote = 0;

    if ((lit >> 0x1) == 0x0) { wrote = snprintf(line, sizeof(line), "%u'b%u", 0x1, (lit & 0x1) != 0x0 ? 0x1 : 0x0); }
    else if ((lit & 0x1) != 0x0) { wrote = snprintf(line, sizeof(line), "~n%u", lit >> 0x1); }
    else { wrote = snprintf(line, sizeof(line), "n%u", lit >> 0x1); }

    if (wrote < 0x0) { return LDG_ERR_STR_FMT; }

    return crosstalk_hdl_netarr_text_wr(out_file, line);
}

static uint32_t crosstalk_hdl_netarr_lit_vec_wr(ldg_io_file_t *out_file, const uint32_t *lits, uint32_t width)
{
    uint32_t bit_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(out_file == 0x0 || lits == 0x0 || width == 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_netarr_text_wr(out_file, "{");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (bit_idx = width; bit_idx != 0x0; bit_idx = bit_idx - 0x1)
    {
        ret = crosstalk_hdl_netarr_lit_wr(out_file, lits[bit_idx - 0x1]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (bit_idx != 0x1)
        {
            ret = crosstalk_hdl_netarr_text_wr(out_file, ", ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }

    return crosstalk_hdl_netarr_text_wr(out_file, "}");
}

static uint32_t crosstalk_hdl_netarr_name_wr(ldg_io_file_t *out_file, const char *src, const crosstalk_hdl_ir_sig_t *sig)
{
    char line[CROSSTALK_HDL_NETARR_LINE_CAP] = LDG_ARR_ZERO_INIT;

    if (LDG_UNLIKELY(out_file == 0x0 || src == 0x0 || sig == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (snprintf(line, sizeof(line), "%.*s", (int32_t)sig->name.len, src + sig->name.off) < 0x0) { return LDG_ERR_STR_FMT; }

    return crosstalk_hdl_netarr_text_wr(out_file, line);
}

static uint32_t crosstalk_hdl_netarr_range_wr(ldg_io_file_t *out_file, uint32_t hi, uint32_t lo)
{
    char line[CROSSTALK_HDL_NETARR_LINE_CAP] = LDG_ARR_ZERO_INIT;

    if (snprintf(line, sizeof(line), "[%u:%u]", hi, lo) < 0x0) { return LDG_ERR_STR_FMT; }

    return crosstalk_hdl_netarr_text_wr(out_file, line);
}

uint32_t crosstalk_hdl_netarr_v_wr(const crosstalk_hdl_netarr_t *netarr, const char *src, const char *top, const char *clk_name, const char *path)
{
    const crosstalk_hdl_aig_ctx_t *aig_ctx = 0x0;
    const crosstalk_hdl_ir_t *ir_ctx = 0x0;
    ldg_io_file_t *out_file = 0x0;
    char line[CROSSTALK_HDL_NETARR_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t sig_idx = 0;
    uint32_t bit_idx = 0;
    uint32_t lut_idx = 0;
    uint32_t pi_idx = 0;
    uint32_t op_idx = 0;
    uint32_t root_at = 0;
    uint8_t first = LDG_TRUTH_TRUE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(netarr == 0x0 || src == 0x0 || top == 0x0 || clk_name == 0x0 || path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    aig_ctx = netarr->aig_ctx;
    if (LDG_UNLIKELY(aig_ctx == 0x0)) { return LDG_ERR_NOT_INIT; }

    ir_ctx = aig_ctx->ir_ctx;
    ret = ldg_io_file_open(path, LDG_IO_WR_ONLY | LDG_IO_CREATE | LDG_IO_TRUNCATE, CROSSTALK_HDL_NETARR_FILE_MODE, &out_file);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (snprintf(line, sizeof(line), "module %.63s_netarr(\n", top) < 0x0) { ret = LDG_ERR_STR_FMT; }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_text_wr(out_file, line); }

    for (sig_idx = 0; sig_idx < ir_ctx->sig_cunt && ret == LDG_ERR_AOK; sig_idx = sig_idx + 0x1)
    {
        const crosstalk_hdl_ir_sig_t *sig = &ir_ctx->sigs[sig_idx];
        const char *dir = 0x0;

        if (sig->port == CROSSTALK_HDL_IR_PORT_NONE || aig_ctx->lit_off[sig_idx] == UINT32_MAX) { continue; }

        if (sig->port == CROSSTALK_HDL_IR_PORT_IN) { dir = "input wire "; }
        else if (sig->port == CROSSTALK_HDL_IR_PORT_OUT) { dir = "output wire "; }
        else { dir = "inout wire "; }

        if (first == LDG_TRUTH_FALSE) { ret = crosstalk_hdl_netarr_text_wr(out_file, ",\n"); }

        first = LDG_TRUTH_FALSE;
        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_text_wr(out_file, dir); }

        if (ret == LDG_ERR_AOK && sig->width > 0x1)
        {
            ret = crosstalk_hdl_netarr_range_wr(out_file, sig->width - 0x1, 0x0);
            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_text_wr(out_file, " "); }
        }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_name_wr(out_file, src, sig); }
    }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_text_wr(out_file, "\n);\n\n"); }

    for (pi_idx = 0; pi_idx < aig_ctx->pi_cunt && ret == LDG_ERR_AOK; pi_idx = pi_idx + 0x1)
    {
        if (snprintf(line, sizeof(line), "wire n%u;\n", aig_ctx->pi_node[pi_idx] >> 0x1) < 0x0) { ret = LDG_ERR_STR_FMT; }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_text_wr(out_file, line); }
    }

    for (lut_idx = 0; lut_idx < netarr->lut_cunt && ret == LDG_ERR_AOK; lut_idx = lut_idx + 0x1)
    {
        if (snprintf(line, sizeof(line), "wire n%u;\n", netarr->luts[lut_idx].out_node) < 0x0) { ret = LDG_ERR_STR_FMT; }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_text_wr(out_file, line); }
    }

    for (sig_idx = 0; sig_idx < ir_ctx->sig_cunt && ret == LDG_ERR_AOK; sig_idx = sig_idx + 0x1)
    {
        const crosstalk_hdl_ir_sig_t *sig = &ir_ctx->sigs[sig_idx];

        if (aig_ctx->ff_off[sig_idx] == UINT32_MAX) { continue; }

        ret = crosstalk_hdl_netarr_text_wr(out_file, "reg ");
        if (ret == LDG_ERR_AOK && sig->width > 0x1)
        {
            ret = crosstalk_hdl_netarr_range_wr(out_file, sig->width - 0x1, 0x0);
            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_text_wr(out_file, " "); }
        }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_name_wr(out_file, src, sig); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_text_wr(out_file, "_q;\n"); }
    }

    for (sig_idx = 0; sig_idx < ir_ctx->sig_cunt && ret == LDG_ERR_AOK; sig_idx = sig_idx + 0x1)
    {
        const crosstalk_hdl_ir_sig_t *sig = &ir_ctx->sigs[sig_idx];

        if (sig->kind != CROSSTALK_HDL_IR_SIG_MEM) { continue; }

        ret = crosstalk_hdl_netarr_text_wr(out_file, "reg ");
        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_range_wr(out_file, sig->width - 0x1, 0x0); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_text_wr(out_file, " "); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_name_wr(out_file, src, sig); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_text_wr(out_file, " "); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_range_wr(out_file, 0x0, sig->depth - 0x1); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_text_wr(out_file, ";\n"); }
    }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_text_wr(out_file, "\n"); }

    for (pi_idx = 0; pi_idx < aig_ctx->pi_cunt && ret == LDG_ERR_AOK; pi_idx = pi_idx + 0x1)
    {
        const crosstalk_hdl_ir_sig_t *sig = &ir_ctx->sigs[aig_ctx->pi_sig[pi_idx]];
        uint8_t kind = aig_ctx->pi_kind[pi_idx];

        if (kind == (uint8_t)CROSSTALK_HDL_AIG_PI_KIND_MEM_RD) { continue; }

        if (snprintf(line, sizeof(line), "assign n%u = ", aig_ctx->pi_node[pi_idx] >> 0x1) < 0x0) { ret = LDG_ERR_STR_FMT; }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_text_wr(out_file, line); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_name_wr(out_file, src, sig); }

        if (ret == LDG_ERR_AOK && kind == (uint8_t)CROSSTALK_HDL_AIG_PI_KIND_FF) { ret = crosstalk_hdl_netarr_text_wr(out_file, "_q"); }

        if (ret == LDG_ERR_AOK) { if (sig->width > 0x1) { if (snprintf(line, sizeof(line), "[%u];\n", aig_ctx->pi_bit[pi_idx]) < 0x0) { ret = LDG_ERR_STR_FMT; }
                else { ret = crosstalk_hdl_netarr_text_wr(out_file, line); } }
            else { ret = crosstalk_hdl_netarr_text_wr(out_file, ";\n"); } }
    }

    for (lut_idx = 0; lut_idx < netarr->lut_cunt && ret == LDG_ERR_AOK; lut_idx = lut_idx + 0x1)
    {
        const crosstalk_hdl_netarr_lut_t *lut = &netarr->luts[lut_idx];
        uint32_t in_idx = 0;

        if (snprintf(line, sizeof(line), "assign n%u = %u'h%016" PRIX64 " >> {", lut->out_node, (uint32_t)CROSSTALK_HDL_NETARR_MASK_BITS, lut->mask) < 0x0) { ret = LDG_ERR_STR_FMT; }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_text_wr(out_file, line); }

        for (in_idx = lut->in_cunt; in_idx != 0x0 && ret == LDG_ERR_AOK; in_idx = in_idx - 0x1)
        {
            ret = crosstalk_hdl_netarr_lit_wr(out_file, lut->in_node[in_idx - 0x1] << 0x1);
            if (ret == LDG_ERR_AOK && in_idx != 0x1) { ret = crosstalk_hdl_netarr_text_wr(out_file, ", "); }
        }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_text_wr(out_file, "};\n"); }
    }

    root_at = aig_ctx->ff_root_base;
    for (sig_idx = 0; sig_idx < ir_ctx->sig_cunt && ret == LDG_ERR_AOK; sig_idx = sig_idx + 0x1)
    {
        const crosstalk_hdl_ir_sig_t *sig = &ir_ctx->sigs[sig_idx];

        if (aig_ctx->ff_off[sig_idx] != UINT32_MAX)
        {
            if (snprintf(line, sizeof(line), "always @(posedge %.63s) ", clk_name) < 0x0) { ret = LDG_ERR_STR_FMT; }

            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_text_wr(out_file, line); }

            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_name_wr(out_file, src, sig); }

            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_text_wr(out_file, "_q <= "); }

            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_lit_vec_wr(out_file, aig_ctx->roots + root_at, sig->width); }

            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_text_wr(out_file, ";\n"); }

            root_at = root_at + sig->width;
        }

        if (aig_ctx->lit_off[sig_idx] != UINT32_MAX && (sig->port == CROSSTALK_HDL_IR_PORT_OUT || sig->port == CROSSTALK_HDL_IR_PORT_INOUT))
        {
            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_text_wr(out_file, "assign "); }

            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_name_wr(out_file, src, sig); }

            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_text_wr(out_file, " = "); }

            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_lit_vec_wr(out_file, aig_ctx->roots + root_at, sig->width); }

            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_text_wr(out_file, ";\n"); }

            root_at = root_at + sig->width;
        }
    }

    for (op_idx = 0; op_idx < ir_ctx->comb_cunt + ir_ctx->seq_op_cunt && ret == LDG_ERR_AOK; op_idx = op_idx + 0x1)
    {
        const crosstalk_hdl_ir_op_t *op = op_idx < ir_ctx->comb_cunt ? &ir_ctx->comb_ops[op_idx] : &ir_ctx->seq_ops[op_idx - ir_ctx->comb_cunt];
        const crosstalk_hdl_ir_sig_t *mem_sig = 0x0;
        uint32_t off = aig_ctx->op_root_off[op_idx];
        uint32_t addr_w = 0;

        if (off == UINT32_MAX) { continue; }

        if (op->kind != CROSSTALK_HDL_IR_OP_MEM_RD && op->kind != CROSSTALK_HDL_IR_OP_MEM_WR) { continue; }

        mem_sig = &ir_ctx->sigs[op->src0];
        addr_w = op->src1 != UINT32_MAX ? ir_ctx->sigs[op->src1].width : 0x1;
        if (op->kind == CROSSTALK_HDL_IR_OP_MEM_RD)
        {
            uint32_t dst_w = ir_ctx->sigs[op->dst].width;

            ret = crosstalk_hdl_netarr_text_wr(out_file, "wire ");
            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_range_wr(out_file, addr_w - 0x1, 0x0); }

            if (ret == LDG_ERR_AOK) { if (snprintf(line, sizeof(line), " rda%u;\nassign rda%u = ", op_idx, op_idx) < 0x0) { ret = LDG_ERR_STR_FMT; }
                else { ret = crosstalk_hdl_netarr_text_wr(out_file, line); } }

            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_lit_vec_wr(out_file, aig_ctx->roots + off, addr_w); }

            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_text_wr(out_file, ";\n"); }

            for (bit_idx = 0; bit_idx < dst_w && ret == LDG_ERR_AOK; bit_idx = bit_idx + 0x1)
            {
                if (snprintf(line, sizeof(line), "assign n%u = ", aig_ctx->pi_node[aig_ctx->op_pi_base[op_idx] + bit_idx] >> 0x1) < 0x0) { ret = LDG_ERR_STR_FMT; }

                if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_text_wr(out_file, line); }

                if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_name_wr(out_file, src, mem_sig); }

                if (ret == LDG_ERR_AOK) { if (snprintf(line, sizeof(line), "[rda%u][%u];\n", op_idx, bit_idx) < 0x0) { ret = LDG_ERR_STR_FMT; }
                    else { ret = crosstalk_hdl_netarr_text_wr(out_file, line); } }
            }
        }
        else
        {
            uint32_t val_w = op->src2 != UINT32_MAX ? ir_ctx->sigs[op->src2].width : 0x1;

            ret = crosstalk_hdl_netarr_text_wr(out_file, "wire ");
            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_range_wr(out_file, addr_w - 0x1, 0x0); }

            if (ret == LDG_ERR_AOK) { if (snprintf(line, sizeof(line), " wra%u;\nassign wra%u = ", op_idx, op_idx) < 0x0) { ret = LDG_ERR_STR_FMT; }
                else { ret = crosstalk_hdl_netarr_text_wr(out_file, line); } }

            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_lit_vec_wr(out_file, aig_ctx->roots + off + 0x1, addr_w); }

            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_text_wr(out_file, ";\n"); }

            if (ret == LDG_ERR_AOK) { if (snprintf(line, sizeof(line), "always @(posedge %.63s) if (", clk_name) < 0x0) { ret = LDG_ERR_STR_FMT; }
                else { ret = crosstalk_hdl_netarr_text_wr(out_file, line); } }

            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_lit_wr(out_file, aig_ctx->roots[off]); }

            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_text_wr(out_file, ") "); }

            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_name_wr(out_file, src, mem_sig); }

            if (ret == LDG_ERR_AOK) { if (snprintf(line, sizeof(line), "[wra%u] <= ", op_idx) < 0x0) { ret = LDG_ERR_STR_FMT; }
                else { ret = crosstalk_hdl_netarr_text_wr(out_file, line); } }

            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_lit_vec_wr(out_file, aig_ctx->roots + off + 0x1 + addr_w, val_w); }

            if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_text_wr(out_file, ";\n"); }
        }
    }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_netarr_text_wr(out_file, "\nendmodule\n"); }

    if (ldg_io_file_close(out_file) != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    return ret;
}
