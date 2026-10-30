#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <dangling/arch/amd64/atomic.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/core/types.h>
#include <dangling/io/file.h>
#include <dangling/io/path.h>
#include <dangling/mem/alloc.h>
#include <dangling/parse/parse.h>
#include <dangling/str/str.h>
#include <dangling/time/time.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "eda/ic/fpga/fpga.h"
#include "hdl/aig.h"
#include "hdl/diag/sv.h"
#include "hdl/logic_gate.h"
#include "hdl/lutmap.h"

static void name_set(char *dst, uint32_t cap, const char *src)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(dst == 0x0 || cap == 0)) { return; }

    dst[0] = LDG_STR_TERM;
    if (src == 0x0) { return; }

    while (src[idx] != LDG_STR_TERM && idx < cap - LDG_STR_TERM_SIZE)
    {
        dst[idx] = src[idx];
        idx = idx + 0x1;
    }

    dst[idx] = LDG_STR_TERM;
}

static uint64_t op_cost_get(const crosstalk_hdl_ir_op_t *op)
{
    uint64_t width = 0;

    if (LDG_UNLIKELY(op == 0x0)) { return 0x0; }

    width = (uint64_t)op->width;
    if (width == 0) { return 0x0; }

    if (op->kind == CROSSTALK_HDL_IR_OP_CONST) { return 0x0; }

    if (op->kind == CROSSTALK_HDL_IR_OP_ADD || op->kind == CROSSTALK_HDL_IR_OP_SUB || op->kind == CROSSTALK_HDL_IR_OP_NEG) { return width * CROSSTALK_EDA_IC_FPGA_COST_SCALE_ORD; }

    if (op->kind == CROSSTALK_HDL_IR_OP_LT || op->kind == CROSSTALK_HDL_IR_OP_LE || op->kind == CROSSTALK_HDL_IR_OP_GT || op->kind == CROSSTALK_HDL_IR_OP_GE) { return width * CROSSTALK_EDA_IC_FPGA_COST_SCALE_ORD; }

    if (op->kind == CROSSTALK_HDL_IR_OP_LT_S || op->kind == CROSSTALK_HDL_IR_OP_LE_S || op->kind == CROSSTALK_HDL_IR_OP_GT_S || op->kind == CROSSTALK_HDL_IR_OP_GE_S) { return width * CROSSTALK_EDA_IC_FPGA_COST_SCALE_ORD; }

    if (op->kind == CROSSTALK_HDL_IR_OP_SHL || op->kind == CROSSTALK_HDL_IR_OP_SHR || op->kind == CROSSTALK_HDL_IR_OP_ASHR) { return width * CROSSTALK_EDA_IC_FPGA_COST_SCALE_SHIFT; }

    if (op->kind == CROSSTALK_HDL_IR_OP_MUL)
    {
        uint64_t cost = width * width / CROSSTALK_EDA_IC_FPGA_COST_MUL_DIV;
        if (cost < width) { cost = width; }

        return cost;
    }

    if (op->kind == CROSSTALK_HDL_IR_OP_DIV || op->kind == CROSSTALK_HDL_IR_OP_MOD)
    {
        uint64_t cost = width * width / CROSSTALK_EDA_IC_FPGA_COST_DIVMOD_DIV;
        if (cost < width * CROSSTALK_EDA_IC_FPGA_COST_SCALE_ORD) { cost = width * CROSSTALK_EDA_IC_FPGA_COST_SCALE_ORD; }

        return cost;
    }

    return width;
}

static uint64_t lut_est_get(const crosstalk_hdl_ir_t *ir)
{
    uint64_t total = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ir == 0x0)) { return 0x0; }

    for (idx = 0; idx < ir->comb_cunt; idx = idx + 0x1) { total = total + op_cost_get(&ir->comb_ops[idx]); }

    return total;
}

static uint64_t ff_bits_get(const crosstalk_hdl_ir_t *ir)
{
    uint64_t total = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ir == 0x0)) { return 0x0; }

    for (idx = 0; idx < ir->seq_op_cunt; idx = idx + 0x1) { if (ir->seq_ops[idx].kind == CROSSTALK_HDL_IR_OP_REG_NEXT) { total = total + (uint64_t)ir->seq_ops[idx].width; } }

    return total;
}

static uint64_t mem_bits_get(const crosstalk_hdl_ir_t *ir)
{
    uint64_t total = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ir == 0x0)) { return 0x0; }

    for (idx = 0; idx < ir->sig_cunt; idx = idx + 0x1) { if (ir->sigs[idx].kind == CROSSTALK_HDL_IR_SIG_MEM) { total = total + (uint64_t)ir->sigs[idx].depth * (uint64_t)ir->sigs[idx].width; } }

    return total;
}

static uint8_t tok_name_match_is(const char *src, const crosstalk_hdl_tok_t *tok, const char *name)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(src == 0x0 || tok == 0x0 || name == 0x0)) { return LDG_TRUTH_FALSE; }

    while (at < tok->len && name[at] != LDG_STR_TERM)
    {
        if (src[tok->off + at] != name[at]) { return LDG_TRUTH_FALSE; }

        at = at + 0x1;
    }

    return (at == tok->len && name[at] == LDG_STR_TERM) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t sig_width_by_tok_get(const crosstalk_hdl_ir_t *ir, const char *src, const crosstalk_hdl_tok_t *tok, uint64_t *out_width)
{
    uint32_t idx = 0;

    *out_width = 0x0;

    for (idx = 0; idx < ir->sig_cunt; idx = idx + 0x1)
    {
        if (ir->sigs[idx].name.len != tok->len) { continue; }

        if (memcmp(src + ir->sigs[idx].name.off, src + tok->off, (uint64_t)tok->len) != 0) { continue; }

        if (ir->sigs[idx].port == CROSSTALK_HDL_IR_PORT_INOUT) { return CROSSTALK_EDA_IC_FPGA_ERR_INOUT; }

        *out_width = (uint64_t)ir->sigs[idx].width;
        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t io_bits_get(const crosstalk_hdl_ir_t *ir, const crosstalk_hdl_ast_t *ast, const char *src, const char *top, uint64_t *out_bits)
{
    uint64_t total = 0;
    uint32_t mod = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ir == 0x0 || out_bits == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_bits = 0x0;

    for (idx = 0; idx < ir->sig_cunt; idx = idx + 0x1) { if (ir->sigs[idx].port == CROSSTALK_HDL_IR_PORT_INOUT) { return CROSSTALK_EDA_IC_FPGA_ERR_INOUT; } }

    if (ast == 0x0 || src == 0x0 || top == 0x0 || ast->nodes == 0x0) { return LDG_ERR_NOT_INIT; }

    for (mod = 0; mod < ast->cunt; mod = mod + 0x1)
    {
        uint32_t port = 0;

        if (ast->nodes[mod].kind != CROSSTALK_HDL_NODE_MODULE) { continue; }

        if (tok_name_match_is(src, &ast->nodes[mod].tok, top) != LDG_TRUTH_TRUE) { continue; }

        for (port = ast->nodes[mod].first_child; port != UINT32_MAX; port = ast->nodes[port].next_sibling)
        {
            uint32_t name = 0;

            if (ast->nodes[port].kind != CROSSTALK_HDL_NODE_PORT) { continue; }

            for (name = ast->nodes[port].first_child; name != UINT32_MAX; name = ast->nodes[name].next_sibling)
            {
                uint64_t width = 0;

                if (ast->nodes[name].kind != CROSSTALK_HDL_NODE_DECL_NAME) { continue; }

                if (sig_width_by_tok_get(ir, src, &ast->nodes[name].tok, &width) == CROSSTALK_EDA_IC_FPGA_ERR_INOUT) { return CROSSTALK_EDA_IC_FPGA_ERR_INOUT; }

                total = total + width;
            }
        }

        *out_bits = total;
        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t verdict_build(const crosstalk_hdl_ir_t *ir, const crosstalk_hdl_ast_t *ast, const char *src, const char *top, const crosstalk_eda_ic_fpga_comp_t *caps, crosstalk_eda_ic_fpga_verdict_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ir == 0x0 || caps == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (crosstalk_eda_ic_fpga_verdict_t)LDG_STRUCT_ZERO_INIT;

    ret = io_bits_get(ir, ast, src, top, &out->io_bits);
    if (ret == CROSSTALK_EDA_IC_FPGA_ERR_INOUT) { return ret; }

    out->ff_bits = ff_bits_get(ir);
    out->lut_est = lut_est_get(ir);
    out->cells_used = (out->lut_est > out->ff_bits) ? out->lut_est : out->ff_bits;
    out->mem_bits = mem_bits_get(ir);

    out->cells_avail = caps->logic_cells;
    out->mem_avail = caps->ram_bits;
    out->io_avail = caps->user_io;

    out->cells_fit = (out->cells_used <= out->cells_avail) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    out->mem_fit = (out->mem_bits <= out->mem_avail) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    out->io_fit = (out->io_bits <= out->io_avail) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    out->fits = (out->cells_fit == LDG_TRUTH_TRUE && out->mem_fit == LDG_TRUTH_TRUE && out->io_fit == LDG_TRUTH_TRUE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

typedef struct fpga_include_ctx
{
    const crosstalk_eda_ic_fpga_ctx_t *ctx;
    const crosstalk_eda_ic_fpga_slot_t *slot;
} fpga_include_ctx_t;

static uint32_t include_file_rd(ldg_mem_pool_t *pool, const char *path, char **out_text, uint32_t *out_len)
{
    ldg_io_info_t st = LDG_STRUCT_ZERO_INIT;
    ldg_io_file_t *fh = 0x0;
    char *text = 0x0;
    uint64_t got = 0;
    uint32_t ret = 0;

    ret = ldg_io_path_info_get(path, &st);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (st.is_file != LDG_TRUTH_TRUE) { return LDG_ERR_IO_IS_DIR; }

    if (st.size == 0 || st.size > (uint64_t)CROSSTALK_EDA_IC_FPGA_INC_BYTES_MAX)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: include size refused; path: %.255s; bytes: %u64", path, st.size);
        return CROSSTALK_HDL_ERR_PREPROC_INCLUDE;
    }

    ret = ldg_mem_pool_alloc(pool, st.size + (uint64_t)LDG_STR_TERM_SIZE, (void **)&text);
    if (ret != LDG_ERR_AOK) { return CROSSTALK_EDA_IC_FPGA_ERR_POOL; }

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, 0x0, &fh);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_rd(fh, text, st.size, &got);
    if (ldg_io_file_close(fh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga include close failed"); }

    if (ret != LDG_ERR_AOK) { return ret; }

    if (got != st.size) { return LDG_ERR_IO_RD; }

    text[got] = LDG_STR_TERM;
    *out_text = text;
    *out_len = (uint32_t)got;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "fpga: include; path: %.255s; bytes: %u32", path, *out_len);

    return LDG_ERR_AOK;
}

static uint32_t include_resolve_cb(void *user, const char *name, uint32_t name_len, char **out_text, uint32_t *out_len)
{
    const fpga_include_ctx_t *inc = (const fpga_include_ctx_t *)user;
    const char *files = 0x0;
    const char *base = 0x0;
    char want[CROSSTALK_EDA_IC_FPGA_INC_CAP] = LDG_ARR_ZERO_INIT;
    char rel[CROSSTALK_EDA_IC_FPGA_FILESET_NAME_CAP] = LDG_ARR_ZERO_INIT;
    char dir[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t pos = 0;
    uint32_t rel_len = 0;
    uint32_t sep_at = UINT32_MAX;

    if (LDG_UNLIKELY(inc == 0x0 || name == 0x0 || out_text == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_text = 0x0;
    *out_len = 0x0;

    if (name_len == 0 || name_len + LDG_STR_TERM_SIZE > (uint32_t)sizeof(want)) { return CROSSTALK_HDL_ERR_PREPROC_INCLUDE; }

    memcpy(want, name, name_len);
    want[name_len] = LDG_STR_TERM;

    if (ldg_io_path_absolute_is(want) == LDG_TRUTH_TRUE) { return include_file_rd(inc->ctx->elab_pool, want, out_text, out_len); }

    base = inc->slot->fileset_dir[0] != LDG_STR_TERM ? inc->slot->fileset_dir : inc->ctx->src_dir;

    if (base[0] == LDG_STR_TERM) { return CROSSTALK_HDL_ERR_PREPROC_INCLUDE; }

    if (ldg_io_path_join(base, want, path, (uint64_t)sizeof(path)) == LDG_ERR_AOK && ldg_io_path_exists_is(path) == LDG_TRUTH_TRUE) { return include_file_rd(inc->ctx->elab_pool, path, out_text, out_len); }

    files = inc->slot->fileset_files;

    for (pos = 0; ; pos = pos + 0x1)
    {
        char ch = files[pos];

        if (ch != CROSSTALK_EDA_IC_FPGA_FILESET_SEP && ch != LDG_STR_TERM)
        {
            if (rel_len + LDG_STR_TERM_SIZE < (uint32_t)sizeof(rel))
            {
                if (ch == '/' || ch == '\\') { sep_at = rel_len; }

                rel[rel_len] = ch;
                rel_len = rel_len + 0x1;
            }

            continue;
        }

        if (sep_at != UINT32_MAX)
        {
            rel[sep_at] = LDG_STR_TERM;

            if (ldg_io_path_join(base, rel, dir, (uint64_t)sizeof(dir)) == LDG_ERR_AOK && ldg_io_path_join(dir, want, path, (uint64_t)sizeof(path)) == LDG_ERR_AOK && ldg_io_path_exists_is(path) == LDG_TRUTH_TRUE) { return include_file_rd(inc->ctx->elab_pool, path, out_text, out_len); }
        }

        rel_len = 0;
        sep_at = UINT32_MAX;

        if (ch == LDG_STR_TERM) { break; }
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: include not found; name: %.127s; dir: %.191s", want, base);

    return CROSSTALK_HDL_ERR_PREPROC_INCLUDE;
}

static uint32_t slot_systask_open(crosstalk_eda_ic_fpga_ctx_t *ctx, crosstalk_eda_ic_fpga_slot_t *slot)
{
    crosstalk_hdl_systask_ctx_t *sys = 0x0;
    uint32_t ret = 0;

    ret = ldg_mem_pool_alloc(ctx->elab_pool, (uint64_t)sizeof(crosstalk_hdl_systask_ctx_t), (void **)&sys);
    if (ret != LDG_ERR_AOK) { return CROSSTALK_EDA_IC_FPGA_ERR_POOL; }

    ret = crosstalk_hdl_systask_init(sys, &slot->ir, slot->elab_src, ctx->elab_pool);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_systask_out_cb_set(sys, ctx->systask_cb, ctx->systask_user); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_systask_scope_set(sys, slot->top, slot->comp.f_op_hz); }

    if (ret != LDG_ERR_AOK) { return ret; }

    slot->systask = sys;
    slot->exec.systask = sys;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: system tasks; top: %.63s; tasks: %u32; args: %u32; finals: %u32", slot->top, slot->ir.systask_cunt, slot->ir.systask_arg_cunt, slot->ir.final_cunt);

    return LDG_ERR_AOK;
}

static void slot_final_run(crosstalk_eda_ic_fpga_slot_t *slot, uint32_t slot_idx)
{
    uint32_t ret = 0;

    if (slot->systask == 0x0 || slot->runnable != LDG_TRUTH_TRUE || slot->ir.final_cunt == 0) { return; }

    ret = crosstalk_hdl_systask_final_run(slot->systask, &slot->exec);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: final procedures failed; slot: %u32; err: %u32", slot_idx, ret); }
    else { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: final procedures run; slot: %u32; finals: %u32; cycle: %u64", slot_idx, slot->ir.final_cunt, slot->exec.cycle_cunt); }
}

static void slot_finish_apply(crosstalk_eda_ic_fpga_slot_t *slot, uint32_t slot_idx)
{
    if (slot->systask == 0x0 || crosstalk_hdl_systask_finish_is(slot->systask) != LDG_TRUTH_TRUE) { return; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: finish executed; slot: %u32; cycle: %u64", slot_idx, slot->exec.cycle_cunt);

    slot_final_run(slot, slot_idx);
    slot->hlt = LDG_TRUTH_TRUE;
}

static uint32_t slot_build(crosstalk_eda_ic_fpga_ctx_t *ctx, crosstalk_eda_ic_fpga_slot_t *slot)
{
    fpga_include_ctx_t inc = LDG_STRUCT_ZERO_INIT;
    crosstalk_hdl_preproc_ctx_t preproc = LDG_STRUCT_ZERO_INIT;
    crosstalk_hdl_parse_ctx_t parse = LDG_STRUCT_ZERO_INIT;
    crosstalk_hdl_elab_ctx_t *elab = 0x0;
    uint32_t ret = 0;
    uint32_t top_len = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || slot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    slot->runnable = LDG_TRUTH_FALSE;
    slot->hlt = LDG_TRUTH_FALSE;
    slot->systask = 0x0;
    slot->elab_src = 0x0;
    slot->elab_src_len = 0x0;
    slot->clk_sig = UINT32_MAX;
    slot->abi = 0x0;
    slot->abi_clk_sig = (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE;
    slot->diag = (crosstalk_hdl_diag_t)LDG_STRUCT_ZERO_INIT;
    slot->diag_text[0] = LDG_STR_TERM;

    inc.ctx = ctx;
    inc.slot = slot;

    if (ctx->include_cb != 0x0) { ret = crosstalk_hdl_preproc_init(&preproc, ctx->elab_pool, ctx->include_cb, ctx->include_user); }
    else { ret = crosstalk_hdl_preproc_init(&preproc, ctx->elab_pool, include_resolve_cb, &inc); }

    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_hdl_preproc_run(&preproc, slot->src, slot->src_len, &slot->elab_src, &slot->elab_src_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_hdl_ast_init(&slot->ast, ctx->elab_pool, ctx->ast_cap);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_hdl_parse_run(&parse, slot->elab_src, slot->elab_src_len, &slot->ast);
    if (ret != LDG_ERR_AOK)
    {
        if (crosstalk_hdl_diag_sv_entry_find(&parse, ret, &slot->diag) == LDG_ERR_AOK) { if (crosstalk_hdl_diag_text_fmt(&slot->diag, slot->diag_text, (uint32_t)sizeof(slot->diag_text)) != LDG_ERR_AOK) { slot->diag_text[0] = LDG_STR_TERM; } }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: parse failed; err: %u32; line: %u32; col: %u32; top: %.63s; diag: %.255s", ret, slot->diag.line, slot->diag.col, slot->top, slot->diag_text);
        return ret;
    }

    ret = crosstalk_hdl_ir_init(&slot->ir, ctx->elab_pool, ctx->ir_sig_cap, ctx->ir_comb_cap, ctx->ir_seq_op_cap, ctx->ir_seq_blk_cap);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_mem_pool_alloc(ctx->elab_pool, (uint64_t)sizeof(crosstalk_hdl_elab_ctx_t), (void **)&elab);
    if (ret != LDG_ERR_AOK) { return CROSSTALK_EDA_IC_FPGA_ERR_POOL; }

    memset(elab, 0x0, sizeof(crosstalk_hdl_elab_ctx_t));

    top_len = (uint32_t)strlen(slot->top);

    ret = crosstalk_hdl_elab_run(elab, slot->elab_src, &slot->ast, ctx->elab_pool, &slot->ir, slot->top, top_len);
    if (ret != LDG_ERR_AOK)
    {
        uint32_t line_start = elab->err_tok.off;
        uint32_t line_end = elab->err_tok.off;

        if (line_start > slot->elab_src_len) { line_start = slot->elab_src_len; line_end = slot->elab_src_len; }

        while (line_start > 0 && slot->elab_src[line_start - 0x1] != '\n' && elab->err_tok.off - line_start < CROSSTALK_EDA_IC_FPGA_ELAB_ERR_LINE_LOG_MAX) { line_start--; }

        while (line_end < slot->elab_src_len && slot->elab_src[line_end] != '\n' && line_end - line_start < CROSSTALK_EDA_IC_FPGA_ELAB_ERR_LINE_LOG_MAX) { line_end++; }

        while (line_start < line_end && (slot->elab_src[line_start] == ' ' || slot->elab_src[line_start] == '\t')) { line_start++; }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: elab failed; err: %u32; tok line: %u32; col: %u32; text: %.*s; line text: %.*s", ret, elab->err_tok.line, elab->err_tok.col, (uint32_t)(elab->err_tok.len < (uint32_t)CROSSTALK_EDA_IC_FPGA_ELAB_ERR_TOK_LOG_MAX ? elab->err_tok.len : (uint32_t)CROSSTALK_EDA_IC_FPGA_ELAB_ERR_TOK_LOG_MAX), slot->elab_src + elab->err_tok.off, line_end - line_start, slot->elab_src + line_start);
        return ret;
    }

    {
        uint32_t unknown_at = 0;

        for (unknown_at = 0; unknown_at < elab->unknown_mod_cunt; unknown_at = unknown_at + 0x1) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: unknown module tied off; name: %.*s; line: %u32; col: %u32; top: %.63s", (uint32_t)(elab->unknown_mod[unknown_at].len < (uint32_t)CROSSTALK_EDA_IC_FPGA_ELAB_ERR_TOK_LOG_MAX ? elab->unknown_mod[unknown_at].len : (uint32_t)CROSSTALK_EDA_IC_FPGA_ELAB_ERR_TOK_LOG_MAX), slot->elab_src + elab->unknown_mod[unknown_at].off, elab->unknown_mod[unknown_at].line, elab->unknown_mod[unknown_at].col, slot->top); }
    }

    ret = verdict_build(&slot->ir, &slot->ast, slot->elab_src, slot->top, &slot->comp, &slot->verdict);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_hdl_exec_init(&slot->exec, &slot->ir, slot->elab_src, ctx->elab_pool);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (slot->ir.systask_cunt != 0)
    {
        ret = slot_systask_open(ctx, slot);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

#if LDG_TRUTH_FALSE == LDG_TRUTH_TRUE
    if (slot->verdict.fits != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_IC_FPGA_ERR_OVER_CAP; }

#else
    if (slot->verdict.fits != LDG_TRUTH_TRUE) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: over cap, enforcement off; top: %.63s; cells: %u64/%u64; mem: %u64/%u64; io: %u64/%u64", slot->top, slot->verdict.cells_used, slot->verdict.cells_avail, slot->verdict.mem_bits, slot->verdict.mem_avail, slot->verdict.io_bits, slot->verdict.io_avail); }

#endif

    ret = crosstalk_hdl_exec_sig_find(&slot->exec, slot->clk, &slot->clk_sig);
    if (ret != LDG_ERR_AOK)
    {
        slot->clk_sig = UINT32_MAX;

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, slot->ir.seq_blk_cunt != 0 ? LDG_LOGGING_LVL_WARN : LDG_LOGGING_LVL_INFO, "fpga: clk sig absent; top: %.63s; clk: %.63s; seq blks: %u32", slot->top, slot->clk, slot->ir.seq_blk_cunt);

        if (slot->ir.seq_blk_cunt != 0) { return CROSSTALK_EDA_IC_FPGA_ERR_CLOCK; }
    }

    slot->prog_rd = 0;
    slot->prog_wr = 0;
    slot->prog_abi_sig = (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE;
    slot->pc_abi_sig = (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE;

    if (crosstalk_hdl_exec_sig_find(&slot->exec, CROSSTALK_EDA_IC_FPGA_PROG_SIG, &slot->prog_exec_sig) != LDG_ERR_AOK) { slot->prog_exec_sig = UINT32_MAX; }

    if (crosstalk_hdl_exec_sig_find(&slot->exec, CROSSTALK_EDA_IC_FPGA_PC_SIG, &slot->pc_exec_sig) != LDG_ERR_AOK) { slot->pc_exec_sig = UINT32_MAX; }

    {
        char pc_name[CROSSTALK_EDA_IC_FPGA_TOP_CAP] = LDG_ARR_ZERO_INIT;
        uint32_t hit = 0;
        uint32_t from = 0;
        uint32_t seen = 0;

        while (seen < (uint32_t)CROSSTALK_EDA_IC_FPGA_PC_CANDIDATE_MAX)
        {
            if (crosstalk_hdl_exec_sig_sub_find(&slot->exec, CROSSTALK_EDA_IC_FPGA_PROG_SIG, from, &hit) != LDG_ERR_AOK) { break; }

            if (crosstalk_hdl_exec_sig_name_get(&slot->exec, hit, pc_name, (uint32_t)sizeof(pc_name)) == LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: loc candidate; sig: %u32; name: %.63s; width: %u32; picked: %u32", hit, pc_name, slot->ir.sigs[hit].width, hit == slot->prog_exec_sig ? 0x1 : 0x0); }

            from = hit + 0x1;
            seen = seen + 0x1;
        }
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: prog sample; pc sig: %s; loc sig: %s", slot->pc_exec_sig != UINT32_MAX ? CROSSTALK_EDA_IC_FPGA_PC_SIG : "none", slot->prog_exec_sig != UINT32_MAX ? CROSSTALK_EDA_IC_FPGA_PROG_SIG : "none");

    if (slot->comp.f_op_hz == 0)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: component freq zero; top: %.63s", slot->top);
        return CROSSTALK_EDA_IC_FPGA_ERR_CLOCK;
    }

    ret = crosstalk_hdl_level_run(&slot->ir, ctx->elab_pool, &slot->level_res);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_exec_level_set(&slot->exec, &slot->level_res); }

    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: level off, sweep fallback; top: %s; err: %u32", slot->top, ret); }
    else
    {
        uint32_t fast_alias[CROSSTALK_HDL_EXEC_FAST_ALIAS_CAP] = LDG_ARR_ZERO_INIT;
        uint32_t fast_alias_cunt = 0;

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: levelized; units: %u32; levels: %u32; iter ops: %u32; comb: %u32; seq: %u32; blks: %u32; sigs: %u32", slot->level_res.unit_cunt, slot->level_res.level_max, slot->level_res.iter_op_cunt, slot->ir.comb_cunt, slot->ir.seq_op_cunt, slot->ir.seq_blk_cunt, slot->ir.sig_cunt);

        if (slot->clk_sig != UINT32_MAX && crosstalk_hdl_level_fast_is(&slot->ir, slot->clk_sig, fast_alias, CROSSTALK_HDL_EXEC_FAST_ALIAS_CAP, &fast_alias_cunt) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_exec_fast_set(&slot->exec, slot->clk_sig, fast_alias, fast_alias_cunt);
            if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: fast set failed; top: %s; err: %u32", slot->top, ret); }
            else { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: fast on; top: %s; aliases: %u32; narrow: %u32", slot->top, fast_alias_cunt, slot->exec.nrw != 0x0 ? 0x1 : 0x0); }
        }
        else { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: fast off; top: %s; narrow: %u32", slot->top, slot->exec.nrw != 0x0 ? 0x1 : 0x0); }
    }

    ret = crosstalk_hdl_exec_eval(&slot->exec);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: time zero settle failed; top: %.63s; err: %u32", slot->top, ret); }

    {
        crosstalk_hdl_aig_ctx_t *aig = 0x0;
        crosstalk_hdl_netarr_t *netarr = 0x0;
        crosstalk_hdl_logic_gate_ctx_t *gate = 0x0;
        crosstalk_hdl_lutmap_res_t map_res = LDG_STRUCT_ZERO_INIT;
        uint32_t sig_at = 0;
        uint32_t wide_cunt = 0;

        for (sig_at = 0; sig_at < slot->ir.sig_cunt; sig_at = sig_at + 0x1) { if (slot->ir.sigs[sig_at].width > (uint32_t)CROSSTALK_HDL_AIG_WIDTH_MAX) { wide_cunt = wide_cunt + 0x1; } }

        if (wide_cunt > 0)
        {
            slot->exec.gate = 0x0;
            slot->exec.gate_on = LDG_TRUTH_FALSE;
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: gate-level netlist skipped; wide sigs unsupported; top: %.63s; wide: %u32; limit: %u32", slot->top, wide_cunt, (uint32_t)CROSSTALK_HDL_AIG_WIDTH_MAX);
            slot->runnable = LDG_TRUTH_TRUE;
            return LDG_ERR_AOK;
        }

        ret = ldg_mem_pool_alloc(ctx->elab_pool, (uint64_t)sizeof(crosstalk_hdl_aig_ctx_t), (void **)&aig);
        if (ret == LDG_ERR_AOK) { ret = ldg_mem_pool_alloc(ctx->elab_pool, (uint64_t)sizeof(crosstalk_hdl_netarr_t), (void **)&netarr); }

        if (ret == LDG_ERR_AOK) { ret = ldg_mem_pool_alloc(ctx->elab_pool, (uint64_t)sizeof(crosstalk_hdl_logic_gate_ctx_t), (void **)&gate); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_aig_build(aig, &slot->ir, ctx->elab_pool, 0x0); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_lutmap_netarr_run(aig, ctx->elab_pool, (uint32_t)CROSSTALK_HDL_LUTMAP_K_MAX, netarr, &map_res); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_hdl_logic_gate_init(gate, aig, netarr, ctx->elab_pool); }

        if (ret == LDG_ERR_AOK)
        {
            slot->exec.gate = gate;
            slot->exec.gate_on = LDG_TRUTH_FALSE;
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: gate-level netlist ready behind the hdl sim; top: %.63s; luts: %u32; ffs: %u32; latch bits: %u32; mem ops: %u32; depth: %u32; fmax: %u32 kHz", slot->top, map_res.lut_cunt, gate->ff_cunt, gate->latch_cunt, gate->mem_op_cunt, map_res.depth, map_res.fmax_khz);
        }
        else
        {
            slot->exec.gate = 0x0;
            slot->exec.gate_on = LDG_TRUTH_FALSE;
            slot->gate_res = ret;
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: gate-level netlist unavailable; top: %.63s; err: %u32", slot->top, ret);
        }
    }

    slot->runnable = LDG_TRUTH_TRUE;
    return LDG_ERR_AOK;
}

static uint64_t hlt_loc_get(const crosstalk_eda_ic_fpga_slot_t *slot)
{
    if (LDG_UNLIKELY(slot == 0x0)) { return 0x0; }

    if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE) { return 0x0; }

    if (slot->pc_abi_sig != (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { return slot->abi->peek(slot->pc_abi_sig); }

    if (slot->prog_abi_sig != (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { return slot->abi->peek(slot->prog_abi_sig); }

    return 0x0;
}

static uint64_t hlt_cause_get(const crosstalk_eda_ic_fpga_slot_t *slot)
{
    uint32_t cause_sig = 0;

    if (LDG_UNLIKELY(slot == 0x0)) { return 0x0; }

    if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE) { return 0x0; }

    cause_sig = slot->abi->sig_find(CROSSTALK_EDA_IC_FPGA_EXC_CAUSE_SIG);
    if (cause_sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { return 0x0; }

    return slot->abi->peek(cause_sig);
}

static uint32_t abi_step_once(const crosstalk_eda_ic_fpga_slot_t *slot)
{
    uint64_t ran = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(slot == 0x0 || slot->abi == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (slot->abi->cycle != 0x0) { return slot->abi->cycle(slot->abi_clk_sig); }

    if (slot->abi->n_cycle == 0x0) { return LDG_ERR_UNSUPPORTED; }

    ret = slot->abi->n_cycle(slot->abi_clk_sig, (uint64_t)CROSSTALK_EDA_IC_FPGA_SINGLE_STEP_CUNT, &ran);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (ran == 0) { return LDG_ERR_CANCELED; }

    return LDG_ERR_AOK;
}

static LDG_NOINLINE void hlt_log(const crosstalk_eda_ic_fpga_slot_t *slot, uint32_t slot_idx, uint64_t cycle, uint32_t ret)
{
    uint64_t rec[CROSSTALK_HDL_ABI_FAULT_CUNT] = LDG_ARR_ZERO_INIT;
    uint64_t *base = 0x0;
    uint32_t cunt = 0;

    if (LDG_UNLIKELY(slot == 0x0)) { return; }

    if (ret != LDG_ERR_CANCELED)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: exec halted; slot: %u32; cycle: %u64; err: %u32; loc: 0x%X64; exc cause: %u64", slot_idx, cycle, ret, hlt_loc_get(slot), hlt_cause_get(slot));
        return;
    }

    if (slot->abi != 0x0 && slot->abi->ok == LDG_TRUTH_TRUE && slot->abi->state_get((uint32_t)CROSSTALK_HDL_ABI_STATE_EMU_FAULT, &base, &cunt) == LDG_ERR_AOK && base != 0x0 && cunt >= (uint32_t)CROSSTALK_HDL_ABI_FAULT_CUNT) { memcpy(rec, base, sizeof(rec)); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: fault halt; slot: %u32; cycle: %u64; cause: %u64; loc: 0x%X64; instr: 0x%08X64; addr: 0x%X64; size: %u64; sp: 0x%X64; step: %u64", slot_idx, cycle, rec[CROSSTALK_HDL_ABI_FAULT_CAUSE_AT], rec[CROSSTALK_HDL_ABI_FAULT_LOC_AT], rec[CROSSTALK_HDL_ABI_FAULT_INSTR_AT], rec[CROSSTALK_HDL_ABI_FAULT_ADDR_AT], rec[CROSSTALK_HDL_ABI_FAULT_SIZE_AT], rec[CROSSTALK_HDL_ABI_FAULT_SP_AT], rec[CROSSTALK_HDL_ABI_FAULT_STEP_AT]);
}

static void refused_log_wr(crosstalk_eda_ic_fpga_refused_log_t *log, uint32_t slot_idx, uint32_t prog_gen, const char *sig_name, uint32_t cause)
{
    crosstalk_eda_ic_fpga_refused_t *entry = 0x0;
    uint32_t at = 0;
    uint32_t kept = 0;

    if (LDG_UNLIKELY(log == 0x0 || sig_name == 0x0)) { return; }

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return; }

    if (log->prog_gen[slot_idx] != prog_gen)
    {
        for (at = 0; at < log->cunt; at = at + 0x1)
        {
            if (log->entry[at].slot_idx == slot_idx) { continue; }

            log->entry[kept] = log->entry[at];
            kept = kept + 0x1;
        }

        log->cunt = kept;
        log->prog_gen[slot_idx] = prog_gen;
    }

    for (at = 0; at < log->cunt; at = at + 0x1)
    {
        entry = &log->entry[at];

        if (entry->slot_idx == slot_idx && entry->cause == cause && strncmp(entry->sig, sig_name, (uint64_t)CROSSTALK_EDA_IC_FPGA_TOP_CAP - LDG_STR_TERM_SIZE) == 0) { return; }
    }

    if (log->cunt >= (uint32_t)CROSSTALK_EDA_IC_FPGA_REFUSED_CAP) { return; }

    entry = &log->entry[log->cunt];
    entry->slot_idx = slot_idx;
    entry->cause = cause;
    name_set(entry->sig, CROSSTALK_EDA_IC_FPGA_TOP_CAP, sig_name);
    log->cunt = log->cunt + 0x1;

    if (cause == (uint32_t)CROSSTALK_EDA_IC_FPGA_REFUSED_SIG_ABSENT) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: mem access refused; no such signal in the design; sig: %.63s", sig_name); }
    else if (cause == (uint32_t)CROSSTALK_EDA_IC_FPGA_REFUSED_SIG_NOT_MEM) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: mem access refused; the signal is not a memory; sig: %.63s", sig_name); }
    else { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: mem access refused; the compiled component has no such signal; sig: %.63s", sig_name); }
}

static uint8_t pool_starved_is(uint32_t ret)
{
    if (ret == LDG_ERR_MEM_POOL_FULL || ret == LDG_ERR_ALLOC_NULL || ret == (uint32_t)CROSSTALK_EDA_IC_FPGA_ERR_POOL) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t pool_grow(ldg_mem_pool_t **pool)
{
    uint64_t cap = 0;
    uint64_t grown = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(pool == 0x0 || *pool == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    cap = ldg_mem_pool_cap_get(*pool);
    grown = cap * (uint64_t)CROSSTALK_EDA_IC_FPGA_POOL_GROW_FACTOR;

    if (grown > CROSSTALK_EDA_IC_FPGA_ELAB_POOL_BYTES_MAX)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: elab pool at ceiling; cap: %u64; max: %u64", cap, CROSSTALK_EDA_IC_FPGA_ELAB_POOL_BYTES_MAX);
        return LDG_ERR_FULL;
    }

    ret = ldg_mem_pool_destroy(pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_pool_create(0x0, grown, pool);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: elab pool grow failed; want: %u64; err: %u32", grown, ret);
        ret = ldg_mem_pool_create(0x0, cap, pool);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        return LDG_ERR_FULL;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: elab pool grown; from: %u64; to: %u64", cap, grown);

    return LDG_ERR_AOK;
}

static uint32_t build_caps_grow(crosstalk_eda_ic_fpga_ctx_t *ctx)
{
    uint64_t ceiling = (uint64_t)CROSSTALK_EDA_IC_FPGA_AST_CAP * CROSSTALK_EDA_IC_FPGA_BUILD_CAP_GROWTH_MAX;
    uint32_t factor = (uint32_t)CROSSTALK_EDA_IC_FPGA_POOL_GROW_FACTOR;

    if ((uint64_t)ctx->ast_cap * (uint64_t)factor > ceiling)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: build caps at ceiling; ast: %u32; sigs: %u32; comb: %u32; seq ops: %u32; seq blks: %u32", ctx->ast_cap, ctx->ir_sig_cap, ctx->ir_comb_cap, ctx->ir_seq_op_cap, ctx->ir_seq_blk_cap);
        return LDG_ERR_FULL;
    }

    ctx->ast_cap = ctx->ast_cap * factor;
    ctx->ir_sig_cap = ctx->ir_sig_cap * factor;
    ctx->ir_comb_cap = ctx->ir_comb_cap * factor;
    ctx->ir_seq_op_cap = ctx->ir_seq_op_cap * factor;
    ctx->ir_seq_blk_cap = ctx->ir_seq_blk_cap * factor;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: build caps grown; ast: %u32; sigs: %u32; comb: %u32; seq ops: %u32; seq blks: %u32", ctx->ast_cap, ctx->ir_sig_cap, ctx->ir_comb_cap, ctx->ir_seq_op_cap, ctx->ir_seq_blk_cap);

    return LDG_ERR_AOK;
}

static uint32_t src_pool_grow(crosstalk_eda_ic_fpga_ctx_t *ctx, uint64_t need)
{
    char *staged[CROSSTALK_EDA_IC_FPGA_SRC_POOL_CAP] = LDG_ARR_ZERO_INIT;
    ldg_mem_pool_t *grown = 0x0;
    crosstalk_eda_ic_fpga_slot_t *held = 0x0;
    uint64_t cap = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (need <= ctx->src_cap) { return LDG_ERR_AOK; }

    if (need > CROSSTALK_EDA_IC_FPGA_SRC_CAP_MAX)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: src pool at ceiling; need: %u64; max: %u64", need, CROSSTALK_EDA_IC_FPGA_SRC_CAP_MAX);
        return CROSSTALK_EDA_IC_FPGA_ERR_SRC_CAP;
    }

    cap = ctx->src_cap;

    while (cap < need) { cap = cap * (uint64_t)CROSSTALK_EDA_IC_FPGA_POOL_GROW_FACTOR; }

    if (cap > CROSSTALK_EDA_IC_FPGA_SRC_CAP_MAX) { cap = CROSSTALK_EDA_IC_FPGA_SRC_CAP_MAX; }

    ret = ldg_mem_pool_create(cap, (uint64_t)CROSSTALK_EDA_IC_FPGA_SRC_POOL_CAP, &grown);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: src pool grow failed; want: %u64; err: %u32", cap, ret);
        return CROSSTALK_EDA_IC_FPGA_ERR_SRC_CAP;
    }

    for (idx = 0; idx < (uint32_t)CROSSTALK_EDA_IC_FPGA_SRC_POOL_CAP; idx = idx + 0x1)
    {
        held = idx < (uint32_t)CROSSTALK_EDA_IC_FPGA_SLOT_MAX ? &ctx->slot[idx] : &ctx->trial;

        if (held->src == 0x0) { continue; }

        ret = ldg_mem_pool_alloc(grown, cap, (void **)&staged[idx]);
        if (ret != LDG_ERR_AOK) { break; }

        memcpy(staged[idx], held->src, (uint64_t)held->src_len);
        staged[idx][held->src_len] = LDG_STR_TERM;
    }

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: src pool grow copy failed; want: %u64; err: %u32", cap, ret);

        if (ldg_mem_pool_destroy(&grown) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: src pool destroy failed"); }

        return CROSSTALK_EDA_IC_FPGA_ERR_SRC_CAP;
    }

    for (idx = 0; idx < (uint32_t)CROSSTALK_EDA_IC_FPGA_SRC_POOL_CAP; idx = idx + 0x1)
    {
        held = idx < (uint32_t)CROSSTALK_EDA_IC_FPGA_SLOT_MAX ? &ctx->slot[idx] : &ctx->trial;

        if (held->src != 0x0) { held->src = staged[idx]; }
    }

    if (ldg_mem_pool_destroy(&ctx->src_pool) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: src pool destroy failed"); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: src pool grown; from: %u64; to: %u64; need: %u64", ctx->src_cap, cap, need);

    ctx->src_pool = grown;
    ctx->src_cap = cap;

    return LDG_ERR_AOK;
}

static uint32_t slot_all_rebuild(crosstalk_eda_ic_fpga_ctx_t *ctx)
{
    uint32_t idx = 0;
    uint32_t ret = 0;
    uint8_t rerun = LDG_TRUTH_TRUE;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < CROSSTALK_EDA_IC_FPGA_SLOT_MAX; idx = idx + 0x1)
    {
        slot_final_run(&ctx->slot[idx], idx);

        ctx->slot[idx].prog_gen = ctx->slot[idx].prog_gen + 0x1;
        ctx->slot[idx].build_res = LDG_ERR_AOK;

        if (ctx->probes == 0x0) { continue; }

        if (crosstalk_eda_logic_probe_slot_detach(ctx->probes, idx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga probe slot detach failed"); }
    }

    while (rerun == LDG_TRUTH_TRUE)
    {
        uint8_t starved = LDG_TRUTH_FALSE;
        uint8_t full = LDG_TRUTH_FALSE;

        rerun = LDG_TRUTH_FALSE;

        ret = ldg_mem_pool_rst(ctx->elab_pool);
        if (ret != LDG_ERR_AOK) { return ret; }

        ctx->live_cunt = 0x0;

        for (idx = 0; idx < CROSSTALK_EDA_IC_FPGA_SLOT_MAX; idx = idx + 0x1)
        {
            if (ctx->slot[idx].live != LDG_TRUTH_TRUE) { continue; }

            ctx->slot[idx].gate_res = LDG_ERR_AOK;

            ret = slot_build(ctx, &ctx->slot[idx]);
            ctx->slot[idx].build_res = ret;

            if (pool_starved_is(ret) == LDG_TRUTH_TRUE || pool_starved_is(ctx->slot[idx].gate_res) == LDG_TRUTH_TRUE) { starved = LDG_TRUTH_TRUE; }

            if (ret == LDG_ERR_FULL) { full = LDG_TRUTH_TRUE; }

            if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: slot rebuild failed; slot: %u32; err: %u32; pool used: %u64; pool cap: %u64", idx, ret, ldg_mem_pool_used_get(ctx->elab_pool), ldg_mem_pool_cap_get(ctx->elab_pool)); }

            if (ctx->slot[idx].runnable == LDG_TRUTH_TRUE) { ctx->live_cunt = ctx->live_cunt + 0x1; }
        }

        if (starved == LDG_TRUTH_TRUE && pool_grow(&ctx->elab_pool) == LDG_ERR_AOK) { rerun = LDG_TRUTH_TRUE; }

        if (full == LDG_TRUTH_TRUE && build_caps_grow(ctx) == LDG_ERR_AOK) { rerun = LDG_TRUTH_TRUE; }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_init(crosstalk_eda_ic_fpga_ctx_t *ctx, crosstalk_hdl_preproc_include_cb_t include_cb, void *include_user)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *ctx = (crosstalk_eda_ic_fpga_ctx_t)LDG_STRUCT_ZERO_INIT;

    ret = ldg_mem_pool_create((uint64_t)CROSSTALK_EDA_IC_FPGA_SRC_CAP, (uint64_t)CROSSTALK_EDA_IC_FPGA_SRC_POOL_CAP, &ctx->src_pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->src_cap = (uint64_t)CROSSTALK_EDA_IC_FPGA_SRC_CAP;
    ctx->ast_cap = (uint32_t)CROSSTALK_EDA_IC_FPGA_AST_CAP;
    ctx->ir_sig_cap = (uint32_t)CROSSTALK_EDA_IC_FPGA_IR_SIG_CAP;
    ctx->ir_comb_cap = (uint32_t)CROSSTALK_EDA_IC_FPGA_IR_COMB_CAP;
    ctx->ir_seq_op_cap = (uint32_t)CROSSTALK_EDA_IC_FPGA_IR_SEQ_OP_CAP;
    ctx->ir_seq_blk_cap = (uint32_t)CROSSTALK_EDA_IC_FPGA_IR_SEQ_BLK_CAP;

    ret = ldg_mem_pool_create(0x0, (uint64_t)CROSSTALK_EDA_IC_FPGA_ELAB_POOL_BYTES, &ctx->elab_pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (ldg_mem_pool_destroy(&ctx->src_pool) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga src pool destroy failed"); }

        ctx->src_pool = 0x0;
        return ret;
    }

    ret = ldg_mem_pool_create(0x0, (uint64_t)CROSSTALK_EDA_IC_FPGA_ELAB_POOL_BYTES, &ctx->trial_pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (ldg_mem_pool_destroy(&ctx->elab_pool) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga elab pool destroy failed"); }

        if (ldg_mem_pool_destroy(&ctx->src_pool) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga src pool destroy failed"); }

        ctx->elab_pool = 0x0;
        ctx->src_pool = 0x0;
        return ret;
    }

    ctx->refused = &ctx->refused_st;
    ctx->include_cb = include_cb;
    ctx->include_user = include_user;

    ctx->ok = LDG_TRUTH_TRUE;
    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_systask_cb_set(crosstalk_eda_ic_fpga_ctx_t *ctx, crosstalk_hdl_systask_out_cb_t cb, void *user)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->systask_cb = cb;
    ctx->systask_user = user;

    for (idx = 0; idx < CROSSTALK_EDA_IC_FPGA_SLOT_MAX; idx = idx + 0x1) { if (ctx->slot[idx].systask != 0x0 && crosstalk_hdl_systask_out_cb_set(ctx->slot[idx].systask, cb, user) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: systask cb set failed; slot: %u32", idx); } }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_shutdown(crosstalk_eda_ic_fpga_ctx_t *ctx)
{
    uint32_t idx = 0;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    for (idx = 0; idx < CROSSTALK_EDA_IC_FPGA_SLOT_MAX; idx = idx + 0x1) { slot_final_run(&ctx->slot[idx], idx); }

    if (ctx->elab_pool != 0x0) { ret = ldg_mem_pool_destroy(&ctx->elab_pool); }

    if (ctx->trial_pool != 0x0 && ldg_mem_pool_destroy(&ctx->trial_pool) != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = LDG_ERR_INVALID; }

    if (ctx->src_pool != 0x0 && ret == LDG_ERR_AOK) { ret = ldg_mem_pool_destroy(&ctx->src_pool); }
    else if (ctx->src_pool != 0x0) { if (ldg_mem_pool_destroy(&ctx->src_pool) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: src pool destroy failed after elab pool failure"); } }

    *ctx = (crosstalk_eda_ic_fpga_ctx_t)LDG_STRUCT_ZERO_INIT;
    return ret;
}

uint32_t crosstalk_eda_ic_fpga_probes_bind(crosstalk_eda_ic_fpga_ctx_t *ctx, crosstalk_eda_logic_probe_ctx_t *probes)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ctx->probes = probes;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_display_bind(crosstalk_eda_ic_fpga_ctx_t *ctx, crosstalk_eda_io_display_ctx_t *display)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ctx->display = display;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_receptacle_bind(crosstalk_eda_ic_fpga_ctx_t *ctx, crosstalk_eda_io_receptacle_ctx_t *receptacle)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ctx->receptacle = receptacle;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_slot_freq_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t *out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    if (ctx->slot[slot_idx].live != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    *out = ctx->slot[slot_idx].comp.f_op_hz;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_midi_bind(crosstalk_eda_ic_fpga_ctx_t *ctx, crosstalk_eda_io_midi_ctx_t *midi)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ctx->midi = midi;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_guard_bind(crosstalk_eda_ic_fpga_ctx_t *ctx, crosstalk_hdl_guard_ctx_t *guard)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ctx->guard = guard;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_step_cb_set(crosstalk_eda_ic_fpga_ctx_t *ctx, crosstalk_eda_ic_fpga_step_cb_t cb, void *user)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ctx->step_user = user;
    ctx->step_cb = cb;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_slot_exec_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const crosstalk_hdl_exec_ctx_t **out_exec)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_exec == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_exec = 0x0;

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    if (ctx->slot[slot_idx].runnable != LDG_TRUTH_TRUE || ctx->slot[slot_idx].emu == LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    *out_exec = &ctx->slot[slot_idx].exec;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_slot_hlt_set(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t on)
{
    crosstalk_eda_ic_fpga_slot_t *slot = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];

    if (slot->runnable != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    slot->hlt = on != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: %s; slot: %u32; cycle: %u64; loc: 0x%X64", slot->hlt == LDG_TRUTH_TRUE ? "core halted; state retained" : "core released; execution resumed", slot_idx, slot->cycles_run, hlt_loc_get(slot));

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_ic_fpga_slot_hlt_is(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_TRUTH_FALSE; }

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return LDG_TRUTH_FALSE; }

    return ctx->slot[slot_idx].hlt == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_eda_ic_fpga_slot_abi_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const crosstalk_hdl_abi_t **out_abi)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_abi == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_abi = 0x0;

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    if (ctx->slot[slot_idx].runnable != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (ctx->slot[slot_idx].abi == 0x0 || ctx->slot[slot_idx].abi->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    *out_abi = ctx->slot[slot_idx].abi;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_slot_gate_set(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t on)
{
    crosstalk_eda_ic_fpga_slot_t *slot = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];

    if (slot->runnable != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (on == LDG_TRUTH_TRUE && slot->exec.gate == 0x0) { return LDG_ERR_UNSUPPORTED; }

    slot->exec.gate_on = on != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    slot->exec.poke_dirty = LDG_TRUTH_TRUE;

    if (slot->exec.gate_on == LDG_TRUTH_TRUE && slot->ir.systask_cunt != 0) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: system tasks skipped on the gate-level netlist; slot: %u32; tasks: %u32", slot_idx, slot->ir.systask_cunt); }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_slot_ir_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const crosstalk_hdl_ir_t **out_ir, const char **out_src)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_ir == 0x0 || out_src == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_ir = 0x0;
    *out_src = 0x0;

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    if (ctx->slot[slot_idx].runnable != LDG_TRUTH_TRUE || ctx->slot[slot_idx].emu == LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (ctx->slot[slot_idx].elab_src == 0x0) { return LDG_ERR_NOT_INIT; }

    *out_ir = &ctx->slot[slot_idx].ir;
    *out_src = ctx->slot[slot_idx].elab_src;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_program(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *src, uint32_t src_len, const char *top, const crosstalk_eda_ic_fpga_comp_t *caps, const char *clk_name)
{
    crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || src == 0x0 || top == 0x0 || caps == 0x0 || clk_name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    if (caps->logic_cells == 0 || caps->user_io == 0) { return CROSSTALK_EDA_IC_FPGA_ERR_COMP; }

    slot = &ctx->slot[slot_idx];

    if (slot->src == 0x0)
    {
        ret = ldg_mem_pool_alloc(ctx->src_pool, ctx->src_cap, (void **)&slot->src);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return CROSSTALK_EDA_IC_FPGA_ERR_POOL; }
    }

    ret = src_pool_grow(ctx, (uint64_t)src_len + (uint64_t)LDG_STR_TERM_SIZE);
    if (ret != LDG_ERR_AOK) { return ret; }

    memcpy(slot->src, src, src_len);
    slot->src[src_len] = LDG_STR_TERM;
    slot->src_len = src_len;
    slot->comp = *caps;
    slot->acc_ns = 0x0;
    slot->cycles_run = 0x0;
    slot->live = LDG_TRUTH_TRUE;
    name_set(slot->top, CROSSTALK_EDA_IC_FPGA_TOP_CAP, top);
    name_set(slot->clk, CROSSTALK_EDA_IC_FPGA_TOP_CAP, clk_name);

    ret = slot_all_rebuild(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    return slot->build_res;
}

uint32_t crosstalk_eda_ic_fpga_unprogram(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot_final_run(&ctx->slot[slot_idx], slot_idx);

    if (ctx->slot[slot_idx].src != 0x0) { if (ldg_mem_pool_dealloc(ctx->src_pool, ctx->slot[slot_idx].src) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga src slot dealloc failed"); } }

    ctx->slot[slot_idx] = (crosstalk_eda_ic_fpga_slot_t)LDG_STRUCT_ZERO_INIT;

    return slot_all_rebuild(ctx);
}

uint32_t crosstalk_eda_ic_fpga_emu_engage(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const crosstalk_hdl_abi_t *abi, const char *clk_name, uint64_t f_op_hz)
{
    crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    crosstalk_eda_ic_fpga_comp_t held = LDG_STRUCT_ZERO_INIT;
    char held_top[CROSSTALK_EDA_IC_FPGA_TOP_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || abi == 0x0 || clk_name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    if (abi->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    slot = &ctx->slot[slot_idx];
    held = slot->comp;
    name_set(held_top, CROSSTALK_EDA_IC_FPGA_TOP_CAP, slot->top);

    ret = crosstalk_eda_ic_fpga_unprogram(ctx, slot_idx);
    if (ret != LDG_ERR_AOK) { return ret; }

    slot->comp = held;
    slot->comp.f_op_hz = f_op_hz != 0 ? f_op_hz : (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_CLK_HZ;

    if (slot->comp.logic_cells == 0) { slot->comp.logic_cells = UINT64_MAX; }

    if (slot->comp.user_io == 0) { slot->comp.user_io = UINT64_MAX; }

    slot->clk_sig = UINT32_MAX;
    slot->prog_exec_sig = UINT32_MAX;
    slot->pc_exec_sig = UINT32_MAX;
    slot->abi = abi;
    slot->abi_clk_sig = abi->sig_find(clk_name);
    slot->prog_abi_sig = abi->sig_find(CROSSTALK_EDA_IC_FPGA_PROG_SIG);
    slot->pc_abi_sig = abi->sig_find(CROSSTALK_EDA_IC_FPGA_PC_SIG);

    name_set(slot->top, CROSSTALK_EDA_IC_FPGA_TOP_CAP, held_top);
    name_set(slot->clk, CROSSTALK_EDA_IC_FPGA_TOP_CAP, clk_name);

    if (slot->abi_clk_sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE)
    {
        slot->abi = 0x0;

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: the emulator has no clock; slot: %u32; clk: %.63s", slot_idx, clk_name);

        return CROSSTALK_EDA_IC_FPGA_ERR_CLOCK;
    }

    slot->emu = LDG_TRUTH_TRUE;
    slot->live = LDG_TRUTH_TRUE;
    slot->hlt = LDG_TRUTH_TRUE;
    slot->runnable = LDG_TRUTH_TRUE;
    ctx->live_cunt = ctx->live_cunt + 0x1;

    if (ctx->probes != 0x0) { if (crosstalk_eda_logic_probe_abi_rst(ctx->probes, slot_idx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "probe abi reset failed"); } }

    if (ctx->display != 0x0) { if (crosstalk_eda_io_display_abi_rst(ctx->display, slot_idx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "display abi reset failed"); } }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: emu engaged; design unloaded; slot held; slot: %u32; clk: %.63s; clk sig: %u32; loc sig: %u32; hz: %u64", slot_idx, clk_name, slot->abi_clk_sig, slot->prog_abi_sig, slot->comp.f_op_hz);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_advance(crosstalk_eda_ic_fpga_ctx_t *ctx, double dt_secs, uint64_t budget_cycles)
{
    uint64_t dt_ns = 0;
    uint64_t share = 0;
    uint32_t idx = 0;
    uint32_t ret = LDG_ERR_AOK;
    uint8_t probes_on = LDG_TRUTH_FALSE;
    uint8_t display_on = LDG_TRUTH_FALSE;
    uint8_t recept_on = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (ctx->live_cunt == 0) { return LDG_ERR_AOK; }

    if (dt_secs <= 0.0) { return LDG_ERR_AOK; }

    probes_on = (ctx->prof_on == LDG_TRUTH_TRUE && ctx->probes != 0x0 && ctx->probes->live_cunt != 0x0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    if (ctx->display != 0x0) { display_on = crosstalk_eda_io_display_live_is(ctx->display); }

    if (ctx->receptacle != 0x0) { recept_on = crosstalk_eda_io_receptacle_live_is(ctx->receptacle); }

    dt_ns = (uint64_t)(dt_secs * (double)LDG_NS_PER_SEC);
    share = budget_cycles / (uint64_t)ctx->live_cunt;

    for (idx = 0; idx < CROSSTALK_EDA_IC_FPGA_SLOT_MAX; idx = idx + 0x1)
    {
        crosstalk_eda_ic_fpga_slot_t *slot = &ctx->slot[idx];
        uint64_t want = 0;
        uint64_t done = 0;
        uint8_t packed = LDG_TRUTH_FALSE;

        if (slot->runnable != LDG_TRUTH_TRUE || slot->hlt == LDG_TRUTH_TRUE) { continue; }

        packed = (slot->abi != 0x0 && slot->abi->ok == LDG_TRUTH_TRUE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

        if (packed != LDG_TRUTH_TRUE && slot->clk_sig == UINT32_MAX)
        {
            slot->acc_ns = 0x0;

            if (slot->exec.poke_dirty != LDG_TRUTH_TRUE) { continue; }

            ret = crosstalk_hdl_exec_step(&slot->exec);
            if (ret != LDG_ERR_AOK)
            {
                slot->hlt = LDG_TRUTH_TRUE;
                hlt_log(slot, idx, slot->cycles_run, ret);
                continue;
            }

            slot->cycles_run = slot->cycles_run + 0x1;

            if (probes_on == LDG_TRUTH_TRUE) { crosstalk_eda_logic_probe_cycle_sample(ctx->probes, idx, &slot->exec, slot->abi, slot->cycles_run); }

            if (display_on == LDG_TRUTH_TRUE) { crosstalk_eda_io_display_cycle_sample(ctx->display, idx, &slot->exec, slot->abi); }

            continue;
        }

        slot->acc_ns = slot->acc_ns + dt_ns;

        want = (slot->acc_ns / (uint64_t)LDG_NS_PER_SEC) * slot->comp.f_op_hz;
        want = want + (slot->acc_ns % (uint64_t)LDG_NS_PER_SEC) * slot->comp.f_op_hz / (uint64_t)LDG_NS_PER_SEC;

        if (want > share) { want = share; }

        while (done < want)
        {
            uint64_t chunk = want - done;
            uint64_t step = 0;

            if (chunk > (uint64_t)CROSSTALK_EDA_IC_FPGA_PROG_STRIDE) { chunk = (uint64_t)CROSSTALK_EDA_IC_FPGA_PROG_STRIDE; }

            for (step = 0; step < chunk; step = step + 0x1)
            {
                if (packed == LDG_TRUTH_TRUE) { ret = abi_step_once(slot); }
                else if (slot->clk_sig == UINT32_MAX) { ret = crosstalk_hdl_exec_step(&slot->exec); }
                else { ret = crosstalk_hdl_exec_cycle(&slot->exec, slot->clk_sig); }

                if (ret != LDG_ERR_AOK)
                {
                    slot->hlt = LDG_TRUTH_TRUE;
                    hlt_log(slot, idx, slot->cycles_run + done, ret);
                    break;
                }

                done = done + 0x1;

                slot_finish_apply(slot, idx);

                if (slot->hlt == LDG_TRUTH_TRUE) { break; }

                if (probes_on == LDG_TRUTH_TRUE) { crosstalk_eda_logic_probe_cycle_sample(ctx->probes, idx, &slot->exec, slot->abi, slot->cycles_run + done); }

                if (display_on == LDG_TRUTH_TRUE) { crosstalk_eda_io_display_cycle_sample(ctx->display, idx, &slot->exec, slot->abi); }
            }

            if (slot->hlt == LDG_TRUTH_TRUE) { break; }

            {
                uint64_t prog_val = 0;
                uint8_t prog_got = LDG_TRUTH_FALSE;

                if (packed == LDG_TRUTH_TRUE) { if (slot->pc_abi_sig != (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { prog_val = slot->abi->peek(slot->pc_abi_sig); prog_got = LDG_TRUTH_TRUE; }
                    else if (slot->prog_abi_sig != (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { prog_val = slot->abi->peek(slot->prog_abi_sig); prog_got = LDG_TRUTH_TRUE; } }
                else if (slot->pc_exec_sig != UINT32_MAX) { if (crosstalk_hdl_exec_peek(&slot->exec, slot->pc_exec_sig, &prog_val) == LDG_ERR_AOK) { prog_got = LDG_TRUTH_TRUE; } }
                else if (slot->prog_exec_sig != UINT32_MAX) { if (crosstalk_hdl_exec_peek(&slot->exec, slot->prog_exec_sig, &prog_val) == LDG_ERR_AOK) { prog_got = LDG_TRUTH_TRUE; } }

                if (prog_got == LDG_TRUTH_TRUE)
                {
                    uint32_t prog_next = (slot->prog_wr + 0x1) & ((uint32_t)CROSSTALK_EDA_IC_FPGA_PROG_RING_CAP - 0x1);

                    if (prog_next != LDG_RD_ONCE(slot->prog_rd))
                    {
                        slot->prog_ring[slot->prog_wr] = prog_val;
                        LDG_ST_RELEASE(slot->prog_wr, prog_next);
                    }
                }
            }
        }

        slot->cycles_run = slot->cycles_run + done;
        slot->acc_ns = 0x0;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_clk_domain_set(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint32_t domain)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    ctx->slot[slot_idx].clk_domain = domain;
    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_abi_set(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const crosstalk_hdl_abi_t *abi)
{
    crosstalk_eda_ic_fpga_slot_t *slot = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];
    slot->abi = abi;
    slot->abi_clk_sig = (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE;
    slot->prog_abi_sig = (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE;
    slot->pc_abi_sig = (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE;

    if (abi != 0x0 && abi->ok == LDG_TRUTH_TRUE)
    {
        slot->prog_abi_sig = abi->sig_find(CROSSTALK_EDA_IC_FPGA_PROG_SIG);
        slot->pc_abi_sig = abi->sig_find(CROSSTALK_EDA_IC_FPGA_PC_SIG);
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: prog sample on the compiled component; pc sig: %u32; loc sig: %u32", slot->pc_abi_sig, slot->prog_abi_sig);
    }

    if (ctx->probes != 0x0) { if (crosstalk_eda_logic_probe_abi_rst(ctx->probes, slot_idx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "probe abi reset failed"); } }

    if (ctx->display != 0x0) { if (crosstalk_eda_io_display_abi_rst(ctx->display, slot_idx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "display abi reset failed"); } }

    if (abi == 0x0 || abi->ok != LDG_TRUTH_TRUE)
    {
        if (slot->emu == LDG_TRUTH_TRUE)
        {
            slot->emu = LDG_TRUTH_FALSE;
            slot->runnable = LDG_TRUTH_FALSE;
            slot->live = LDG_TRUTH_FALSE;

            if (ctx->live_cunt != 0) { ctx->live_cunt = ctx->live_cunt - 0x1; }

            if (crosstalk_eda_ic_fpga_mem_watch_clear(ctx, slot_idx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "mem watch clear failed"); }

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: emu detached; slot empty; slot: %u32", slot_idx);

            return LDG_ERR_AOK;
        }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: slot on exec engine; slot: %u32", slot_idx);

        return LDG_ERR_AOK;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: resolving clk on the compiled component; slot: %u32; clk: %.63s", slot_idx, slot->clk);

    slot->abi_clk_sig = abi->sig_find(slot->clk);

    if (slot->abi_clk_sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE)
    {
        slot->abi = 0x0;

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: compiled component has no clock; slot: %u32; clk: %.63s", slot_idx, slot->clk);

        return CROSSTALK_EDA_IC_FPGA_ERR_CLOCK;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: slot on the compiled component; slot: %u32; top: %.63s; clk sig: %u32", slot_idx, slot->top, slot->abi_clk_sig);

    if (slot->ir.systask_cunt != 0) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: system tasks skipped on the compiled component; slot: %u32; tasks: %u32", slot_idx, slot->ir.systask_cunt); }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_cycles_run(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t cycles, uint64_t *out_done)
{
    crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    uint64_t done = 0;
    uint32_t ret = LDG_ERR_AOK;
    uint8_t probes_on = LDG_TRUTH_FALSE;
    uint8_t display_on = LDG_TRUTH_FALSE;
    uint8_t recept_on = LDG_TRUTH_FALSE;
    uint8_t midi_on = LDG_TRUTH_FALSE;
    uint8_t packed = LDG_TRUTH_FALSE;
    uint8_t batched = LDG_TRUTH_FALSE;
    uint8_t emu_step = LDG_TRUTH_FALSE;
    uint64_t took = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_done == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_done = 0x0;

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];
    if (slot->runnable != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (slot->hlt == LDG_TRUTH_TRUE) { return LDG_ERR_CANCELED; }

    probes_on = (ctx->prof_on == LDG_TRUTH_TRUE && ctx->probes != 0x0 && ctx->probes->live_cunt != 0x0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (ctx->display != 0x0) { display_on = crosstalk_eda_io_display_live_is(ctx->display); }

    if (ctx->receptacle != 0x0) { recept_on = crosstalk_eda_io_receptacle_live_is(ctx->receptacle); }

    if (ctx->midi != 0x0) { midi_on = crosstalk_eda_io_midi_live_is(ctx->midi); }

    packed = (slot->abi != 0x0 && slot->abi->ok == LDG_TRUTH_TRUE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    emu_step = (packed == LDG_TRUTH_TRUE && slot->emu == LDG_TRUTH_TRUE && slot->abi->n_cycle != 0x0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (packed != LDG_TRUTH_TRUE && slot->clk_sig == UINT32_MAX)
    {
        if (cycles == 0 || slot->exec.poke_dirty != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

        ret = crosstalk_hdl_exec_step(&slot->exec);
        if (ret != LDG_ERR_AOK)
        {
            slot->hlt = LDG_TRUTH_TRUE;
            hlt_log(slot, slot_idx, slot->cycles_run, ret);
            return ret;
        }

        slot->cycles_run = slot->cycles_run + 0x1;
        *out_done = 0x1;

        if (probes_on == LDG_TRUTH_TRUE) { crosstalk_eda_logic_probe_cycle_sample(ctx->probes, slot_idx, &slot->exec, slot->abi, slot->cycles_run); }

        if (display_on == LDG_TRUTH_TRUE) { crosstalk_eda_io_display_cycle_sample(ctx->display, slot_idx, &slot->exec, slot->abi); }

        return LDG_ERR_AOK;
    }

    if (packed == LDG_TRUTH_TRUE && ctx->guard != 0x0)
    {
        if (CROSSTALK_HDL_GUARD_ENTER(ctx->guard) == 0)
        {
            CROSSTALK_HDL_GUARD_LEAVE(ctx->guard);

            ctx->slot[slot_idx].abi = 0x0;
            ctx->slot[slot_idx].abi_clk_sig = (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE;
            ctx->slot[slot_idx].runnable = LDG_TRUTH_FALSE;

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "fpga: compiled component faulted; slot: %u32; addr: 0x%X64", slot_idx, ctx->guard->fault_addr);

            return CROSSTALK_HDL_ERR_GUARD_FAULT;
        }
    }

    if (packed == LDG_TRUTH_TRUE && slot->abi->time_set != 0x0) { if (slot->abi->time_set(ldg_time_epoch_ns_get()) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu host time set failed"); } }

    while (done < cycles)
    {
        uint64_t chunk = cycles - done;
        uint64_t step = 0;
        uint64_t stride = (uint64_t)CROSSTALK_EDA_IC_FPGA_PROG_STRIDE;

        if (emu_step == LDG_TRUTH_TRUE && ctx->emu_step > stride) { stride = ctx->emu_step; }

        if (chunk > stride) { chunk = stride; }

        batched = (packed == LDG_TRUTH_TRUE && slot->abi->n_cycle != 0x0 && probes_on != LDG_TRUTH_TRUE && display_on != LDG_TRUTH_TRUE && midi_on != LDG_TRUTH_TRUE && (recept_on != LDG_TRUTH_TRUE || crosstalk_eda_io_receptacle_idle_is(ctx->receptacle) == LDG_TRUTH_TRUE)) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

        if (batched == LDG_TRUTH_TRUE)
        {
            uint64_t ran = 0;

            ret = slot->abi->n_cycle(slot->abi_clk_sig, chunk, &ran);

            done = done + ran;

            if (ctx->step_cb != 0x0)
            {
                ctx->step_cb_acc = ctx->step_cb_acc + ran;

                if (ctx->step_cb_acc >= (uint64_t)CROSSTALK_EDA_IC_FPGA_STEP_CB_STRIDE)
                {
                    ctx->step_cb_acc = 0;
                    ctx->step_cb(slot->abi, ctx->step_user);
                }
            }

            if (ret != LDG_ERR_AOK)
            {
                slot->hlt = LDG_TRUTH_TRUE;
                hlt_log(slot, slot_idx, slot->cycles_run + done, ret);
            }
            else if (ran == 0) { slot->hlt = LDG_TRUTH_TRUE; }
        }

        for (step = 0; batched != LDG_TRUTH_TRUE && step < chunk; step = step + took)
        {
            took = 0x1;

            if (emu_step == LDG_TRUTH_TRUE)
            {
                uint64_t want = chunk - step;
                uint64_t ran = 0;
                uint64_t emu_want = ctx->emu_step != 0 ? ctx->emu_step : (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_STEP_TEXT;

                if (want > emu_want) { want = emu_want; }

                ret = slot->abi->n_cycle(slot->abi_clk_sig, want, &ran);

                done = done + ran;

                if (ctx->step_cb != 0x0)
                {
                    ctx->step_cb_acc = ctx->step_cb_acc + ran;

                    if (ctx->step_cb_acc >= (uint64_t)CROSSTALK_EDA_IC_FPGA_STEP_CB_STRIDE)
                    {
                        ctx->step_cb_acc = 0;
                        ctx->step_cb(slot->abi, ctx->step_user);
                    }
                }

                if (ret != LDG_ERR_AOK)
                {
                    slot->hlt = LDG_TRUTH_TRUE;
                    hlt_log(slot, slot_idx, slot->cycles_run + done, ret);
                    break;
                }

                if (ran == 0) { slot->hlt = LDG_TRUTH_TRUE; break; }

                took = ran;
            }
            else
            {
                if (packed == LDG_TRUTH_TRUE) { ret = abi_step_once(slot); }
                else if (slot->clk_sig == UINT32_MAX) { ret = crosstalk_hdl_exec_step(&slot->exec); }
                else { ret = crosstalk_hdl_exec_cycle(&slot->exec, slot->clk_sig); }

                if (ret != LDG_ERR_AOK)
                {
                    slot->hlt = LDG_TRUTH_TRUE;
                    hlt_log(slot, slot_idx, slot->cycles_run + done, ret);
                    break;
                }

                done = done + 0x1;

                slot_finish_apply(slot, slot_idx);

                if (slot->hlt == LDG_TRUTH_TRUE) { break; }
            }

            if (probes_on == LDG_TRUTH_TRUE) { crosstalk_eda_logic_probe_cycle_sample(ctx->probes, slot_idx, &slot->exec, slot->abi, slot->cycles_run + done); }

            if (display_on == LDG_TRUTH_TRUE) { crosstalk_eda_io_display_cycle_sample(ctx->display, slot_idx, &slot->exec, slot->abi); }

            if (recept_on == LDG_TRUTH_TRUE && crosstalk_eda_io_receptacle_idle_is(ctx->receptacle) != LDG_TRUTH_TRUE) { crosstalk_eda_io_receptacle_cycle_drive(ctx->receptacle, slot_idx, &slot->exec, slot->abi); }

            if (midi_on == LDG_TRUTH_TRUE) { crosstalk_eda_io_midi_cycle_drive(ctx->midi, slot_idx, &slot->exec, slot->abi); }
        }

        if (slot->hlt == LDG_TRUTH_TRUE) { break; }

        {
            uint64_t prog_val = 0;
            uint8_t prog_got = LDG_TRUTH_FALSE;

            if (packed == LDG_TRUTH_TRUE) { if (slot->pc_abi_sig != (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { prog_val = slot->abi->peek(slot->pc_abi_sig); prog_got = LDG_TRUTH_TRUE; }
                else if (slot->prog_abi_sig != (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { prog_val = slot->abi->peek(slot->prog_abi_sig); prog_got = LDG_TRUTH_TRUE; } }
            else if (slot->pc_exec_sig != UINT32_MAX) { if (crosstalk_hdl_exec_peek(&slot->exec, slot->pc_exec_sig, &prog_val) == LDG_ERR_AOK) { prog_got = LDG_TRUTH_TRUE; } }
            else if (slot->prog_exec_sig != UINT32_MAX) { if (crosstalk_hdl_exec_peek(&slot->exec, slot->prog_exec_sig, &prog_val) == LDG_ERR_AOK) { prog_got = LDG_TRUTH_TRUE; } }

            if (prog_got == LDG_TRUTH_TRUE)
            {
                uint32_t prog_next = (slot->prog_wr + 0x1) & ((uint32_t)CROSSTALK_EDA_IC_FPGA_PROG_RING_CAP - 0x1);

                if (prog_next != LDG_RD_ONCE(slot->prog_rd))
                {
                    slot->prog_ring[slot->prog_wr] = prog_val;
                    LDG_ST_RELEASE(slot->prog_wr, prog_next);
                }
            }
        }
    }

    if (packed == LDG_TRUTH_TRUE && ctx->guard != 0x0) { CROSSTALK_HDL_GUARD_LEAVE(ctx->guard); }

    slot->cycles_run = slot->cycles_run + done;
    *out_done = done;
    return ret;
}

uint32_t crosstalk_eda_ic_fpga_cycles_total_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t *out_cycles)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_cycles == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cycles = 0x0;

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    *out_cycles = ctx->slot[slot_idx].cycles_run;
    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_verdict_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, crosstalk_eda_ic_fpga_verdict_t *out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    *out = ctx->slot[slot_idx].verdict;
    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_pin_wr(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *port, uint64_t val)
{
    uint32_t sig_idx = UINT32_MAX;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || port == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    if (ctx->slot[slot_idx].runnable != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ret = crosstalk_hdl_exec_sig_find(&ctx->slot[slot_idx].exec, port, &sig_idx);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_hdl_exec_poke(&ctx->slot[slot_idx].exec, sig_idx, val);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (ctx->slot[slot_idx].emu != LDG_TRUTH_TRUE && ctx->slot[slot_idx].clk_sig == UINT32_MAX)
    {
        ret = crosstalk_hdl_exec_eval(&ctx->slot[slot_idx].exec);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    if (ctx->slot[slot_idx].abi != 0x0 && ctx->slot[slot_idx].abi->ok == LDG_TRUTH_TRUE)
    {
        uint32_t abi_sig = ctx->slot[slot_idx].abi->sig_find(port);

        if (abi_sig != (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { ctx->slot[slot_idx].abi->poke(abi_sig, val); }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_pin_rd(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *port, uint64_t *out_val)
{
    uint32_t sig_idx = UINT32_MAX;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || port == 0x0 || out_val == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_val = 0x0;

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    if (ctx->slot[slot_idx].runnable != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ret = crosstalk_hdl_exec_sig_find(&ctx->slot[slot_idx].exec, port, &sig_idx);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (ctx->slot[slot_idx].abi != 0x0 && ctx->slot[slot_idx].abi->ok == LDG_TRUTH_TRUE)
    {
        uint32_t abi_sig = ctx->slot[slot_idx].abi->sig_find(port);

        if (abi_sig != (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE)
        {
            *out_val = ctx->slot[slot_idx].abi->peek(abi_sig);

            return LDG_ERR_AOK;
        }
    }

    return crosstalk_hdl_exec_peek(&ctx->slot[slot_idx].exec, sig_idx, out_val);
}

uint8_t crosstalk_eda_ic_fpga_slot_live_is(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return LDG_ERR_FUNC_ARG_INVALID; }

    return ctx->slot[slot_idx].live;
}

uint8_t crosstalk_eda_ic_fpga_running_is(const crosstalk_eda_ic_fpga_ctx_t *ctx)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < CROSSTALK_EDA_IC_FPGA_SLOT_MAX; idx++) { if (ctx->slot[idx].live == LDG_TRUTH_TRUE && ctx->slot[idx].runnable == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; } }

    return LDG_TRUTH_FALSE;
}

const char* crosstalk_eda_ic_fpga_running_top_get(const crosstalk_eda_ic_fpga_ctx_t *ctx)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return ""; }

    for (idx = 0; idx < CROSSTALK_EDA_IC_FPGA_SLOT_MAX; idx++) { if (ctx->slot[idx].live == LDG_TRUTH_TRUE && ctx->slot[idx].runnable == LDG_TRUTH_TRUE) { return ctx->slot[idx].top; } }

    return "";
}

uint32_t crosstalk_eda_ic_fpga_sig_poke(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t val, uint8_t quiet)
{
    crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    uint32_t sig_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || sig_name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];
    if (slot->runnable != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (slot->emu == LDG_TRUTH_TRUE)
    {
        uint32_t emu_sig = 0;

        if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

        emu_sig = slot->abi->sig_find(sig_name);
        if (emu_sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { return CROSSTALK_EDA_IC_FPGA_ERR_MEM_SIG; }

        slot->abi->poke(emu_sig, val);

        if (quiet != LDG_TRUTH_TRUE) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: poke; sig: %.63s; val: 0x%X64; width: %u32", sig_name, val, (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_SIG_WIDTH); }

        return LDG_ERR_AOK;
    }

    ret = crosstalk_hdl_exec_sig_find(&slot->exec, sig_name, &sig_idx);
    if (ret != LDG_ERR_AOK) { return CROSSTALK_EDA_IC_FPGA_ERR_MEM_SIG; }

    ret = crosstalk_hdl_exec_poke(&slot->exec, sig_idx, val);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (slot->clk_sig == UINT32_MAX)
    {
        ret = crosstalk_hdl_exec_eval(&slot->exec);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    if (slot->abi != 0x0 && slot->abi->ok == LDG_TRUTH_TRUE)
    {
        uint32_t abi_sig = slot->abi->sig_find(sig_name);

        if (abi_sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { return CROSSTALK_EDA_IC_FPGA_ERR_MEM_SIG; }

        slot->abi->poke(abi_sig, val);
    }

    if (quiet != LDG_TRUTH_TRUE) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: poke; sig: %.63s; val: 0x%X64; width: %u32", sig_name, val, slot->ir.sigs[sig_idx].width); }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_sig_peek(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t *out_val, uint32_t *out_width)
{
    const crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    uint32_t sig_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || sig_name == 0x0 || out_val == 0x0 || out_width == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_val = 0x0;
    *out_width = 0x0;

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];
    if (slot->runnable != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (slot->emu == LDG_TRUTH_TRUE) { ret = LDG_ERR_NOT_FOUND; }
    else { ret = crosstalk_hdl_exec_sig_find(&slot->exec, sig_name, &sig_idx); }

    if (ret != LDG_ERR_AOK)
    {
        uint32_t emu_sig = 0;

        if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_IC_FPGA_ERR_MEM_SIG; }

        emu_sig = slot->abi->sig_find(sig_name);
        if (emu_sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { return CROSSTALK_EDA_IC_FPGA_ERR_MEM_SIG; }

        *out_val = slot->abi->peek(emu_sig);
        *out_width = (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_SIG_WIDTH;

        return LDG_ERR_AOK;
    }

    *out_width = slot->ir.sigs[sig_idx].width;

    if (slot->abi != 0x0 && slot->abi->ok == LDG_TRUTH_TRUE)
    {
        uint32_t abi_sig = slot->abi->sig_find(sig_name);

        if (abi_sig != (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE)
        {
            *out_val = slot->abi->peek(abi_sig);

            return LDG_ERR_AOK;
        }

        {
            uint64_t *base = 0x0;
            uint32_t cunt = 0;

            if (slot->abi->state_get((uint32_t)CROSSTALK_HDL_ABI_STATE_SIG_VALS, &base, &cunt) == LDG_ERR_AOK && base != 0x0 && sig_idx < cunt)
            {
                *out_val = base[sig_idx];

                return LDG_ERR_AOK;
            }
        }

        return CROSSTALK_EDA_IC_FPGA_ERR_MEM_SIG;
    }

    return crosstalk_hdl_exec_peek(&slot->exec, sig_idx, out_val);
}

static uint64_t fpga_emu_mem_width_get(const crosstalk_eda_ic_fpga_slot_t *slot)
{
    uint64_t *base = 0x0;
    uint32_t cunt = 0;

    if (LDG_UNLIKELY(slot == 0x0)) { return (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_MEM_WIDTH_MAX; }

    if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE) { return (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_MEM_WIDTH_MAX; }

    if (slot->abi->state_get((uint32_t)CROSSTALK_HDL_ABI_STATE_MEM, &base, &cunt) != LDG_ERR_AOK || cunt == 0) { return (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_MEM_WIDTH_MAX; }

    return (uint64_t)cunt;
}

uint32_t crosstalk_eda_ic_fpga_mem_width_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t *out_width, uint32_t *out_elem_width)
{
    const crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    uint32_t sig_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || sig_name == 0x0 || out_width == 0x0 || out_elem_width == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_width = 0x0;
    *out_elem_width = 0x0;

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];
    if (slot->runnable != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (slot->emu == LDG_TRUTH_TRUE)
    {
        if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

        if (slot->abi->sig_find(sig_name) == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { return CROSSTALK_EDA_IC_FPGA_ERR_MEM_SIG; }

        *out_width = fpga_emu_mem_width_get(slot);
        *out_elem_width = (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_SIG_WIDTH;

        return LDG_ERR_AOK;
    }

    ret = crosstalk_hdl_exec_sig_find(&slot->exec, sig_name, &sig_idx);
    if (ret != LDG_ERR_AOK) { return CROSSTALK_EDA_IC_FPGA_ERR_MEM_SIG; }

    if (slot->ir.sigs[sig_idx].kind != CROSSTALK_HDL_IR_SIG_MEM) { return CROSSTALK_EDA_IC_FPGA_ERR_MEM_SIG; }

    *out_width = (uint64_t)slot->ir.sigs[sig_idx].depth;
    *out_elem_width = slot->ir.sigs[sig_idx].width;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_mem_offset_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t *out)
{
    const crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    uint64_t words = 0;
    uint32_t sig_idx = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || sig_name == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];
    if (slot->runnable != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ret = crosstalk_hdl_exec_sig_find(&slot->exec, sig_name, &sig_idx);
    if (ret != LDG_ERR_AOK) { return CROSSTALK_EDA_IC_FPGA_ERR_MEM_SIG; }

    if (slot->ir.sigs[sig_idx].kind != CROSSTALK_HDL_IR_SIG_MEM) { return CROSSTALK_EDA_IC_FPGA_ERR_MEM_SIG; }

    for (at = 0; at < sig_idx; at = at + 0x1) { if (slot->ir.sigs[at].depth > 0) { words = words + (uint64_t)slot->ir.sigs[at].depth; } }

    *out = words;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_mem_rd(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t addr, uint64_t *out, uint32_t cunt, uint32_t *out_rd)
{
    const crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    uint32_t abi_sig = CROSSTALK_HDL_ABI_SIG_NONE;
    uint32_t sig_idx = 0;
    uint32_t at = 0;
    uint32_t ret = 0;
    uint64_t width = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || sig_name == 0x0 || out == 0x0 || out_rd == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_rd = 0;

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];
    if (slot->runnable != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (slot->emu == LDG_TRUTH_TRUE)
    {
        if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

        abi_sig = slot->abi->sig_find(sig_name);
        if (abi_sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { return CROSSTALK_EDA_IC_FPGA_ERR_MEM_SIG; }

        width = CROSSTALK_EDA_IC_FPGA_EMU_ADDR_SPAN_BYTES / sizeof(uint64_t);
        sig_idx = UINT32_MAX;
    }
    else
    {
        ret = crosstalk_hdl_exec_sig_find(&slot->exec, sig_name, &sig_idx);
        if (ret != LDG_ERR_AOK)
        {
            refused_log_wr(ctx->refused, slot_idx, slot->prog_gen, sig_name, (uint32_t)CROSSTALK_EDA_IC_FPGA_REFUSED_SIG_ABSENT);

            return CROSSTALK_EDA_IC_FPGA_ERR_MEM_SIG;
        }

        if (slot->ir.sigs[sig_idx].kind != CROSSTALK_HDL_IR_SIG_MEM)
        {
            refused_log_wr(ctx->refused, slot_idx, slot->prog_gen, sig_name, (uint32_t)CROSSTALK_EDA_IC_FPGA_REFUSED_SIG_NOT_MEM);

            return CROSSTALK_EDA_IC_FPGA_ERR_MEM_SIG;
        }

        width = (uint64_t)slot->ir.sigs[sig_idx].depth;

        if (slot->abi != 0x0 && slot->abi->ok == LDG_TRUTH_TRUE)
        {
            abi_sig = slot->abi->sig_find(sig_name);

            if (abi_sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE)
            {
                refused_log_wr(ctx->refused, slot_idx, slot->prog_gen, sig_name, (uint32_t)CROSSTALK_EDA_IC_FPGA_REFUSED_ABI_ABSENT);

                return CROSSTALK_EDA_IC_FPGA_ERR_MEM_SIG;
            }
        }
    }

    if (addr >= width) { return CROSSTALK_EDA_IC_FPGA_ERR_MEM_ROOM; }

    for (at = 0; at < cunt; at = at + 0x1)
    {
        uint64_t at_addr = addr + (uint64_t)at;

        if (at_addr >= width) { break; }

        if (abi_sig != (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { out[at] = slot->abi->mem_ld(abi_sig, at_addr); }
        else if (crosstalk_hdl_exec_mem_ld(&slot->exec, sig_idx, at_addr, &out[at]) != LDG_ERR_AOK) { break; }
    }

    *out_rd = at;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_prof_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint32_t kind, uint64_t **out_base, uint32_t *out_cunt)
{
    const crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_base == 0x0 || out_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_base = 0x0;
    *out_cunt = 0;

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];

    if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ret = slot->abi->state_get(kind, out_base, out_cunt);
    if (ret != LDG_ERR_AOK) { return LDG_ERR_UNSUPPORTED; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_dsu_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t *out, uint32_t cap)
{
    const crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    uint64_t *base = 0x0;
    uint32_t cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (cap < (uint32_t)CROSSTALK_HDL_ABI_DSU_CUNT) { return LDG_ERR_BOUNDS; }

    memset(out, 0x0, (uint64_t)cap * sizeof(uint64_t));

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];

    if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ret = slot->abi->state_get((uint32_t)CROSSTALK_HDL_ABI_STATE_EMU_DSU, &base, &cunt);
    if (ret != LDG_ERR_AOK || base == 0x0) { return LDG_ERR_UNSUPPORTED; }

    if (cunt > cap) { cunt = cap; }

    memcpy(out, base, (uint64_t)cunt * sizeof(uint64_t));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_mem_byte_rd(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t byte_at, uint64_t *out, uint32_t cunt, uint32_t *out_rd)
{
    uint64_t elems[CROSSTALK_EDA_IC_FPGA_MEM_BYTE_CHUNK] = LDG_ARR_ZERO_INIT;
    uint64_t width = 0;
    uint64_t elem_at = 0;
    uint32_t elem_width = 0;
    uint32_t elem_bytes = 0;
    uint32_t lead = 0;
    uint32_t elem_want = 0;
    uint32_t elem_got = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || sig_name == 0x0 || out == 0x0 || out_rd == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_rd = 0;

    ret = crosstalk_eda_ic_fpga_mem_width_get(ctx, slot_idx, sig_name, &width, &elem_width);
    if (ret != LDG_ERR_AOK) { return ret; }

    elem_bytes = elem_width / (uint32_t)LDG_BYTE_BITS;
    if (elem_bytes == 0) { elem_bytes = 0x1; }

    if (elem_bytes == 0x1) { return crosstalk_eda_ic_fpga_mem_rd(ctx, slot_idx, sig_name, byte_at, out, cunt, out_rd); }

    elem_at = byte_at / (uint64_t)elem_bytes;
    lead = (uint32_t)(byte_at % (uint64_t)elem_bytes);
    elem_want = (lead + cunt + elem_bytes - 0x1) / elem_bytes;

    if (elem_want > (uint32_t)CROSSTALK_EDA_IC_FPGA_MEM_BYTE_CHUNK) { elem_want = (uint32_t)CROSSTALK_EDA_IC_FPGA_MEM_BYTE_CHUNK; }

    ret = crosstalk_eda_ic_fpga_mem_rd(ctx, slot_idx, sig_name, elem_at, elems, elem_want, &elem_got);
    if (ret != LDG_ERR_AOK) { return ret; }

    for (at = 0; at < cunt; at = at + 0x1)
    {
        uint32_t src = (lead + at) / elem_bytes;
        uint64_t shift = (uint64_t)((lead + at) % elem_bytes) * (uint64_t)LDG_BYTE_BITS;

        if (src >= elem_got) { break; }

        out[at] = (elems[src] >> shift) & (uint64_t)LDG_BYTE_MASK;

        *out_rd = at + 0x1;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_emu_state_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint32_t kind, uint64_t *out, uint32_t cap)
{
    const crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    uint64_t *base = 0x0;
    uint32_t cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (cap == 0) { return LDG_ERR_BOUNDS; }

    memset(out, 0x0, (uint64_t)cap * sizeof(uint64_t));

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];

    if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ret = slot->abi->state_get(kind, &base, &cunt);
    if (ret != LDG_ERR_AOK || base == 0x0) { return LDG_ERR_UNSUPPORTED; }

    if (cunt > cap) { cunt = cap; }

    memcpy(out, base, (uint64_t)cunt * sizeof(uint64_t));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_mem_acc_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t *out, uint32_t cap)
{
    const crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    uint64_t *base = 0x0;
    uint32_t cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (cap < (uint32_t)CROSSTALK_HDL_ABI_MEM_CUNT) { return LDG_ERR_BOUNDS; }

    memset(out, 0x0, (uint64_t)cap * sizeof(uint64_t));

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];

    if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ret = slot->abi->state_get((uint32_t)CROSSTALK_HDL_ABI_STATE_EMU_MEM, &base, &cunt);
    if (ret != LDG_ERR_AOK || base == 0x0) { return LDG_ERR_UNSUPPORTED; }

    if (cunt > cap) { cunt = cap; }

    memcpy(out, base, (uint64_t)cunt * sizeof(uint64_t));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_prof_en_set(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t on)
{
    crosstalk_eda_ic_fpga_slot_t *slot = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];

    ctx->prof_on = on == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE || slot->abi->prof_en == 0x0) { return LDG_ERR_AOK; }

    return slot->abi->prof_en(on == LDG_TRUTH_TRUE ? 0x1 : 0x0);
}

uint32_t crosstalk_eda_ic_fpga_audio_drain(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t *dst, uint64_t cap, uint64_t *out_got)
{
    const crosstalk_eda_ic_fpga_slot_t *slot = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0 || dst == 0x0 || out_got == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_got = 0x0;

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];

    if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (slot->abi->audio_drain == 0x0) { return LDG_ERR_UNSUPPORTED; }

    return slot->abi->audio_drain(dst, cap, out_got);
}

uint32_t crosstalk_eda_ic_fpga_audio_rate_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint32_t *out)
{
    const crosstalk_eda_ic_fpga_slot_t *slot = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];

    if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (slot->abi->audio_rate_get == 0x0) { return LDG_ERR_UNSUPPORTED; }

    *out = slot->abi->audio_rate_get();

    return LDG_ERR_AOK;
}

static void mem_watch_note(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, const char *path, uint64_t word_off)
{
    crosstalk_eda_ic_fpga_mem_watch_t *watch = 0x0;
    ldg_io_info_t st = LDG_STRUCT_ZERO_INIT;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || sig_name == 0x0 || path == 0x0)) { return; }

    for (at = 0; at < ctx->mem_watch_cunt; at = at + 0x1)
    {
        crosstalk_eda_ic_fpga_mem_watch_t *cur = &ctx->mem_watch[at];

        if (cur->used != LDG_TRUTH_TRUE || cur->slot_idx != slot_idx || cur->word_off != word_off) { continue; }

        if (strcmp(cur->sig, sig_name) != 0) { continue; }

        watch = cur;
        break;
    }

    if (watch == 0x0)
    {
        if (ctx->mem_watch_cunt >= (uint32_t)CROSSTALK_EDA_IC_FPGA_MEM_WATCH_MAX) { return; }

        watch = &ctx->mem_watch[ctx->mem_watch_cunt];
        ctx->mem_watch_cunt = ctx->mem_watch_cunt + 0x1;
    }

    if (ldg_str_rbrcpy(watch->sig, sig_name, (uint64_t)sizeof(watch->sig)) != LDG_ERR_AOK) { return; }

    if (ldg_str_rbrcpy(watch->path, path, (uint64_t)sizeof(watch->path)) != LDG_ERR_AOK) { return; }

    watch->slot_idx = slot_idx;
    watch->word_off = word_off;
    watch->used = LDG_TRUTH_TRUE;
    watch->modified_ns = 0x0;
    watch->span = 0x0;

    if (ldg_io_path_info_get(path, &st) == LDG_ERR_AOK)
    {
        watch->modified_ns = st.modified_ns;
        watch->span = st.size;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: mem watch; slot: %u32; sig: %.63s; path: %.127s; off: %u64", slot_idx, sig_name, path, word_off);
}

uint32_t crosstalk_eda_ic_fpga_mem_watch_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t idx, const crosstalk_eda_ic_fpga_mem_watch_t **out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (idx >= ctx->mem_watch_cunt) { return LDG_ERR_BOUNDS; }

    *out = &ctx->mem_watch[idx];

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_mem_watch_clear(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx)
{
    uint32_t at = 0;
    uint32_t keep = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (at = 0; at < ctx->mem_watch_cunt; at = at + 0x1)
    {
        if (ctx->mem_watch[at].slot_idx == slot_idx) { continue; }

        if (keep != at) { memcpy(&ctx->mem_watch[keep], &ctx->mem_watch[at], sizeof(ctx->mem_watch[keep])); }

        keep = keep + 0x1;
    }

    memset(&ctx->mem_watch[keep], 0x0, ((uint64_t)ctx->mem_watch_cunt - (uint64_t)keep) * sizeof(ctx->mem_watch[0]));
    ctx->mem_watch_cunt = keep;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_mem_watch_stale_get(crosstalk_eda_ic_fpga_ctx_t *ctx, double now, uint32_t *out_idx)
{
    ldg_io_info_t st = LDG_STRUCT_ZERO_INIT;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    if (ctx->mem_watch_cunt == 0) { return LDG_ERR_EMPTY; }

    if (now < ctx->mem_watch_next) { return LDG_ERR_AGAIN; }

    ctx->mem_watch_next = now + CROSSTALK_EDA_IC_FPGA_MEM_WATCH_CK_SECS;

    for (at = 0; at < ctx->mem_watch_cunt; at = at + 0x1)
    {
        crosstalk_eda_ic_fpga_mem_watch_t *watch = &ctx->mem_watch[at];

        if (watch->used != LDG_TRUTH_TRUE) { continue; }

        if (ldg_io_path_info_get(watch->path, &st) != LDG_ERR_AOK) { continue; }

        if (st.size == 0) { continue; }

        if (st.modified_ns == watch->modified_ns && st.size == watch->span) { continue; }

        if (ldg_time_epoch_ns_get() - st.modified_ns < (uint64_t)CROSSTALK_EDA_IC_FPGA_MEM_WATCH_SETTLE_NS) { continue; }

        watch->modified_ns = st.modified_ns;
        watch->span = st.size;
        *out_idx = at;

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: mem image changed on disk; slot: %u32; sig: %.63s; path: %.127s; bytes: %u64", watch->slot_idx, watch->sig, watch->path, st.size);

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t crosstalk_eda_ic_fpga_fault_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t *out, uint32_t cap)
{
    const crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    uint64_t *base = 0x0;
    uint32_t cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (cap < (uint32_t)CROSSTALK_HDL_ABI_FAULT_CUNT) { return LDG_ERR_BOUNDS; }

    memset(out, 0x0, (uint64_t)cap * sizeof(uint64_t));

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];

    if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ret = slot->abi->state_get((uint32_t)CROSSTALK_HDL_ABI_STATE_EMU_FAULT, &base, &cunt);
    if (ret != LDG_ERR_AOK || base == 0x0) { return LDG_ERR_UNSUPPORTED; }

    if (cunt > cap) { cunt = cap; }

    memcpy(out, base, (uint64_t)cunt * sizeof(uint64_t));

    if (out[CROSSTALK_HDL_ABI_FAULT_VALID_AT] != 0 && out[CROSSTALK_HDL_ABI_FAULT_CAUSE_AT] == (uint64_t)CROSSTALK_HDL_ABI_CAUSE_UNALIGNED && (slot->hlt_mask & ((uint32_t)0x1 << (uint32_t)CROSSTALK_HDL_ABI_CAUSE_UNALIGNED)) == 0) { memset(out, 0x0, (uint64_t)cap * sizeof(uint64_t)); }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_ring_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t *out, uint32_t cap, uint32_t *out_cunt)
{
    const crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    uint64_t *base = 0x0;
    uint32_t cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0 || out_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];

    if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ret = slot->abi->state_get((uint32_t)CROSSTALK_HDL_ABI_STATE_EMU_RING, &base, &cunt);
    if (ret != LDG_ERR_AOK || base == 0x0) { return LDG_ERR_UNSUPPORTED; }

    if (cunt > cap) { cunt = cap; }

    memcpy(out, base, (uint64_t)cunt * sizeof(uint64_t));

    *out_cunt = cunt;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_brk_set(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t addr, uint8_t on)
{
    crosstalk_eda_ic_fpga_slot_t *slot = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];

    if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (slot->abi->brk_set == 0x0) { return LDG_ERR_UNSUPPORTED; }

    return slot->abi->brk_set(addr, on);
}

uint32_t crosstalk_eda_ic_fpga_ddu_frame_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t *dst, uint64_t cap, uint32_t *out_w, uint32_t *out_h)
{
    const crosstalk_eda_ic_fpga_slot_t *slot = 0x0;

    if (LDG_UNLIKELY(out_w == 0x0 || out_h == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_w = 0x0;
    *out_h = 0x0;
    if (LDG_UNLIKELY(ctx == 0x0 || dst == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX)) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];
    if (LDG_UNLIKELY(slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (slot->abi->ddu_frame_get == 0x0) { return LDG_ERR_UNSUPPORTED; }

    return slot->abi->ddu_frame_get(dst, cap, out_w, out_h);
}

uint32_t crosstalk_eda_ic_fpga_ddu_text_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t *dst, uint64_t cap, uint32_t *out_cols, uint32_t *out_rows)
{
    const crosstalk_eda_ic_fpga_slot_t *slot = 0x0;

    if (LDG_UNLIKELY(out_cols == 0x0 || out_rows == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cols = 0x0;
    *out_rows = 0x0;
    if (LDG_UNLIKELY(ctx == 0x0 || dst == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX)) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];
    if (LDG_UNLIKELY(slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (slot->abi->ddu_text_get == 0x0) { return LDG_ERR_UNSUPPORTED; }

    return slot->abi->ddu_text_get(dst, cap, out_cols, out_rows);
}

uint32_t crosstalk_eda_ic_fpga_io_rd(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t addr, uint64_t *out)
{
    const crosstalk_eda_ic_fpga_slot_t *slot = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];

    if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (slot->abi->io_rd == 0x0) { return LDG_ERR_UNSUPPORTED; }

    return slot->abi->io_rd(addr, out);
}

uint32_t crosstalk_eda_ic_fpga_io_wr(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t addr, uint64_t val)
{
    const crosstalk_eda_ic_fpga_slot_t *slot = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];

    if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (slot->abi->io_wr == 0x0) { return LDG_ERR_UNSUPPORTED; }

    return slot->abi->io_wr(addr, val);
}

uint32_t crosstalk_eda_ic_fpga_io_map_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint32_t idx, uint64_t *out_base, uint64_t *out_span, const char **out_name)
{
    const crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    uint64_t base = 0x0;
    uint64_t span = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_base == 0x0 || out_span == 0x0 || out_name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_base = 0;
    *out_span = 0;
    *out_name = 0x0;

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];

    if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (slot->abi->io_map_get == 0x0) { return LDG_ERR_UNSUPPORTED; }

    ret = slot->abi->io_map_get(idx, &base, &span, out_name);
    if (ret != LDG_ERR_AOK) { return ret; }

    *out_base = (uint64_t)base;
    *out_span = (uint64_t)span;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_watch_set(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t lo, uint64_t hi, uint8_t on)
{
    crosstalk_eda_ic_fpga_slot_t *slot = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    if (hi < lo) { return LDG_ERR_FUNC_ARG_INVALID; }

    slot = &ctx->slot[slot_idx];

    if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (slot->abi->watch_set == 0x0) { return LDG_ERR_UNSUPPORTED; }

    return slot->abi->watch_set(lo, hi, on);
}

uint32_t crosstalk_eda_ic_fpga_brk_skip_set(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t cunt)
{
    crosstalk_eda_ic_fpga_slot_t *slot = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];

    if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (slot->abi->brk_skip_set == 0x0) { return LDG_ERR_UNSUPPORTED; }

    return slot->abi->brk_skip_set((uint32_t)cunt);
}

uint32_t crosstalk_eda_ic_fpga_brk_cond_set(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t reg, uint64_t min)
{
    crosstalk_eda_ic_fpga_slot_t *slot = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];

    if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (slot->abi->brk_cond_set == 0x0) { return LDG_ERR_UNSUPPORTED; }

    return slot->abi->brk_cond_set(reg, min);
}

uint32_t crosstalk_eda_ic_fpga_emu_step_set(crosstalk_eda_ic_fpga_ctx_t *ctx, uint64_t cunt)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (cunt == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    ctx->emu_step = cunt;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_prof_en(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t on)
{
    crosstalk_eda_ic_fpga_slot_t *slot = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];

    ctx->prof_on = on == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE || slot->abi->prof_en == 0x0) { return LDG_ERR_AOK; }

    return slot->abi->prof_en(on == LDG_TRUTH_TRUE ? (uint32_t)LDG_TRUTH_TRUE : (uint32_t)LDG_TRUTH_FALSE);
}

uint32_t crosstalk_eda_ic_fpga_hlt_mask_set(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint32_t mask)
{
    crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];

    if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (slot->abi->hlt_mask_set == 0x0) { return LDG_ERR_UNSUPPORTED; }

    ret = slot->abi->hlt_mask_set(mask);
    if (ret != LDG_ERR_AOK) { return ret; }

    slot->hlt_mask = mask;

    if (mask == 0x0) { slot->hlt = LDG_TRUTH_FALSE; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: fault halt mask set; slot: %u32; mask: 0x%X32; hlt: %u32", slot_idx, mask, slot->hlt);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_prog_drain(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t *out, uint32_t cap, uint32_t *out_cunt)
{
    crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    uint32_t take = 0;
    uint32_t rd = 0;
    uint32_t wr = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0 || out_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];
    rd = slot->prog_rd;
    wr = LDG_LD_ACQUIRE(slot->prog_wr);

    while (rd != wr && take < cap)
    {
        out[take] = slot->prog_ring[rd];
        rd = (rd + 0x1) & ((uint32_t)CROSSTALK_EDA_IC_FPGA_PROG_RING_CAP - 0x1);
        take = take + 0x1;
    }

    LDG_ST_RELEASE(slot->prog_rd, rd);
    *out_cunt = take;

    return LDG_ERR_AOK;
}

static uint32_t fpga_mem_word_st(crosstalk_eda_ic_fpga_slot_t *slot, uint32_t sig_idx, uint32_t abi_sig, uint64_t addr, uint64_t val)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(slot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (sig_idx != UINT32_MAX)
    {
        ret = crosstalk_hdl_exec_mem_st(&slot->exec, sig_idx, addr, val);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    if (abi_sig != (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE && slot->abi != 0x0 && slot->abi->ok == LDG_TRUTH_TRUE) { slot->abi->mem_st(abi_sig, addr, val); }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_mem_wr(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t addr, const uint64_t *vals, uint32_t cunt, uint32_t *out_wrote)
{
    crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    uint32_t abi_sig = CROSSTALK_HDL_ABI_SIG_NONE;
    uint32_t sig_idx = 0;
    uint32_t at = 0;
    uint32_t ret = 0;
    uint64_t width = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || sig_name == 0x0 || vals == 0x0 || out_wrote == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_wrote = 0;

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];
    if (slot->runnable != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (slot->emu == LDG_TRUTH_TRUE)
    {
        if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

        abi_sig = slot->abi->sig_find(sig_name);
        if (abi_sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { return CROSSTALK_EDA_IC_FPGA_ERR_MEM_SIG; }

        width = fpga_emu_mem_width_get(slot);
        sig_idx = UINT32_MAX;
    }
    else
    {
        ret = crosstalk_hdl_exec_sig_find(&slot->exec, sig_name, &sig_idx);
        if (ret != LDG_ERR_AOK)
        {
            refused_log_wr(ctx->refused, slot_idx, slot->prog_gen, sig_name, (uint32_t)CROSSTALK_EDA_IC_FPGA_REFUSED_SIG_ABSENT);

            return CROSSTALK_EDA_IC_FPGA_ERR_MEM_SIG;
        }

        if (slot->ir.sigs[sig_idx].kind != CROSSTALK_HDL_IR_SIG_MEM)
        {
            refused_log_wr(ctx->refused, slot_idx, slot->prog_gen, sig_name, (uint32_t)CROSSTALK_EDA_IC_FPGA_REFUSED_SIG_NOT_MEM);

            return CROSSTALK_EDA_IC_FPGA_ERR_MEM_SIG;
        }

        width = (uint64_t)slot->ir.sigs[sig_idx].depth;

        if (slot->abi != 0x0 && slot->abi->ok == LDG_TRUTH_TRUE)
        {
            abi_sig = slot->abi->sig_find(sig_name);

            if (abi_sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE)
            {
                refused_log_wr(ctx->refused, slot_idx, slot->prog_gen, sig_name, (uint32_t)CROSSTALK_EDA_IC_FPGA_REFUSED_ABI_ABSENT);

                return CROSSTALK_EDA_IC_FPGA_ERR_MEM_SIG;
            }
        }
    }

    if (addr >= width) { return CROSSTALK_EDA_IC_FPGA_ERR_MEM_ROOM; }

    for (at = 0; at < cunt; at = at + 0x1)
    {
        uint64_t at_addr = addr + (uint64_t)at;

        if (at_addr >= width) { break; }

        ret = fpga_mem_word_st(slot, sig_idx, abi_sig, at_addr, vals[at]);
        if (ret != LDG_ERR_AOK) { break; }
    }

    *out_wrote = at;

    return at == 0 && cunt != 0 ? ret : LDG_ERR_AOK;
}

static uint8_t mem_hex_path_is(const char *path)
{
    uint32_t len = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(path == 0x0)) { return LDG_TRUTH_FALSE; }

    while (path[len] != LDG_STR_TERM) { len = len + 0x1; }

    if (len < (uint32_t)CROSSTALK_EDA_IC_FPGA_MEM_HEX_EXT_LEN) { return LDG_TRUTH_FALSE; }

    at = len - (uint32_t)CROSSTALK_EDA_IC_FPGA_MEM_HEX_EXT_LEN;

    return strncmp(&path[at], CROSSTALK_EDA_IC_FPGA_MEM_HEX_EXT, (uint64_t)CROSSTALK_EDA_IC_FPGA_MEM_HEX_EXT_LEN) == 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

LDG_NOINLINE uint32_t crosstalk_eda_ic_fpga_mem_ld(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, const char *path, uint64_t word_off, uint64_t *out_words, uint64_t *out_bytes)
{
    ldg_byte_t chunk[CROSSTALK_EDA_IC_FPGA_MEM_CHUNK];
    crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    ldg_io_file_t *fh = 0x0;
    uint64_t words = 0;
    uint64_t total = 0;
    uint64_t got = 0;
    uint64_t pack = 0;
    uint64_t addr = 0;
    uint32_t held = 0;
    uint32_t elem_bytes = CROSSTALK_EDA_IC_FPGA_MEM_WORD_BYTES;
    uint32_t sig_idx = 0;
    uint32_t abi_sig = CROSSTALK_HDL_ABI_SIG_NONE;
    uint32_t width = 0;
    uint64_t marked = 0;
    uint32_t ret = 0;
    uint8_t hex_is = LDG_TRUTH_FALSE;
    uint8_t addr_mark = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0 || sig_name == 0x0 || path == 0x0 || out_words == 0x0 || out_bytes == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_words = 0x0;
    *out_bytes = 0x0;

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];
    if (slot->runnable != LDG_TRUTH_TRUE)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: mem ld refused; the slot is not runnable; slot: %u32; sig: %.63s", slot_idx, sig_name);

        return LDG_ERR_NOT_INIT;
    }

    if (slot->emu == LDG_TRUTH_TRUE)
    {
        if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: mem ld refused; the emulator is not bound; sig: %.63s", sig_name);

            return LDG_ERR_NOT_INIT;
        }

        abi_sig = slot->abi->sig_find(sig_name);

        if (abi_sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: mem ld refused; the emulator has no such memory; sig: %.63s", sig_name);

            return CROSSTALK_EDA_IC_FPGA_ERR_MEM_SIG;
        }

        width = (uint32_t)fpga_emu_mem_width_get(slot);
        elem_bytes = sizeof(uint64_t);
        sig_idx = UINT32_MAX;
    }
    else
    {
        ret = crosstalk_hdl_exec_sig_find(&slot->exec, sig_name, &sig_idx);
        if (ret != LDG_ERR_AOK)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: mem ld refused; no such signal in the design; sig: %.63s", sig_name);

            return CROSSTALK_EDA_IC_FPGA_ERR_MEM_SIG;
        }

        if (slot->ir.sigs[sig_idx].kind != CROSSTALK_HDL_IR_SIG_MEM)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: mem ld refused; the signal is not a memory; sig: %.63s; kind: %u32", sig_name, (uint32_t)slot->ir.sigs[sig_idx].kind);

            return CROSSTALK_EDA_IC_FPGA_ERR_MEM_SIG;
        }

        width = slot->ir.sigs[sig_idx].depth;

        if (slot->abi != 0x0 && slot->abi->ok == LDG_TRUTH_TRUE)
        {
            abi_sig = slot->abi->sig_find(sig_name);

            if (abi_sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE)
            {
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: mem ld refused; the compiled component has no such signal; sig: %.63s; width: %u32", sig_name, width);

                return CROSSTALK_EDA_IC_FPGA_ERR_MEM_SIG;
            }
        }
    }

    if (word_off >= (uint64_t)width)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: mem ld refused; offset past the memory; sig: %.63s; off: %u64; width: %u32", sig_name, word_off, width);

        return CROSSTALK_EDA_IC_FPGA_ERR_MEM_ROOM;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: mem ld start; sig: %.63s; width: %u32; off: %u64; abi sig: %u32", sig_name, width, word_off, abi_sig);

    hex_is = mem_hex_path_is(path);
    addr = word_off;

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, 0x0, &fh);
    if (ret != LDG_ERR_AOK) { return ret; }

    while (LDG_TRUTH_TRUE)
    {
        uint64_t at = 0;

        ret = ldg_io_file_rd(fh, chunk, (uint64_t)CROSSTALK_EDA_IC_FPGA_MEM_CHUNK, &got);
        if (ret != LDG_ERR_AOK) { break; }

        if (got == 0) { break; }

        total = total + got;

        for (at = 0; at < got; at++)
        {
            if (hex_is == LDG_TRUTH_TRUE)
            {
                ldg_byte_t nib = 0;

                if (chunk[at] == (ldg_byte_t)CROSSTALK_EDA_IC_FPGA_MEM_ADDR_MARK) { addr_mark = LDG_TRUTH_TRUE; continue; }

                if (ldg_char_hex_is((char)chunk[at]) == LDG_TRUTH_TRUE)
                {
                    if (ldg_hex_to_nipple((char)chunk[at], &nib) != LDG_ERR_AOK) { continue; }

                    pack = pack * (uint64_t)LDG_BASE_HEX + (uint64_t)nib;
                    held = held + 0x1;
                    continue;
                }

                if (held == 0) { continue; }

                if (addr_mark == LDG_TRUTH_TRUE)
                {
                    addr = word_off + pack;
                    addr_mark = LDG_TRUTH_FALSE;
                }
                else
                {
                    if (addr >= (uint64_t)width) { break; }

                    ret = fpga_mem_word_st(slot, sig_idx, abi_sig, addr, pack);
                    if (ret != LDG_ERR_AOK) { break; }

                    addr = addr + 0x1;
                    words = words + 0x1;
                }

                pack = 0x0;
                held = 0x0;
                continue;
            }

            pack = pack | ((uint64_t)chunk[at] << (uint64_t)((uint32_t)LDG_BYTE_BITS * held));
            held = held + 0x1;

            if (held < elem_bytes) { continue; }

            if (addr >= (uint64_t)width) { break; }

            ret = fpga_mem_word_st(slot, sig_idx, abi_sig, addr, pack);
            if (ret != LDG_ERR_AOK) { break; }

            addr = addr + 0x1;
            words = words + 0x1;
            pack = 0x0;
            held = 0x0;
        }

        if (words - marked >= (uint64_t)CROSSTALK_EDA_IC_FPGA_MEM_LD_MARK)
        {
            marked = words;
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: mem ld progress; sig: %.63s; words: %u64; bytes: %u64; addr: %u64", sig_name, words, total, addr);
        }

        if (ret != LDG_ERR_AOK || addr >= (uint64_t)width) { break; }
    }

    if (held != 0 && ret == LDG_ERR_AOK && addr < (uint64_t)width)
    {
        if (hex_is != LDG_TRUTH_TRUE) { while (held < elem_bytes)
            {
                pack = pack | ((uint64_t)CROSSTALK_EDA_IC_FPGA_MEM_PAD << (uint64_t)((uint32_t)LDG_BYTE_BITS * held));
                held = held + 0x1;
            }
        }

        if (fpga_mem_word_st(slot, sig_idx, abi_sig, addr, pack) == LDG_ERR_AOK) { words = words + 0x1; }
    }

    if (ldg_io_file_close(fh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga mem ld close failed"); }

    *out_words = words;
    *out_bytes = total;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: mem ld; sig: %.63s; path: %.127s; hex: %u32; bytes: %u64; words: %u64; off: %u64; width: %u32", sig_name, path, hex_is, total, words, word_off, width);

    mem_watch_note(ctx, slot_idx, sig_name, path, word_off);

    if (slot->hlt == LDG_TRUTH_TRUE && words != 0 && word_off == 0)
    {
        slot->hlt = LDG_TRUTH_FALSE;
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: slot armed; slot: %u32; sig: %.63s; words: %u64", slot_idx, sig_name, words);
    }

    if (word_off != 0) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: mem ld at offset; hlt unchanged; slot: %u32; sig: %.63s; words: %u64; off: %u64", slot_idx, sig_name, words, word_off); }

    return LDG_ERR_AOK;
}

static LDG_NOINLINE uint32_t file_append(const char *dir, const char *name, char *dst, uint32_t *io_len, uint64_t cap)
{
    char path[LDG_IO_PATH_MAX];
    ldg_io_info_t st = LDG_STRUCT_ZERO_INIT;
    ldg_io_file_t *fh = 0x0;
    uint64_t got = 0;
    uint64_t need = 0;
    uint32_t join_len = 0;
    uint32_t ret = 0;

    ret = ldg_io_path_join(dir, name, path, (uint64_t)LDG_IO_PATH_MAX);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_path_info_get(path, &st);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (st.is_file != LDG_TRUTH_TRUE) { return LDG_ERR_IO_IS_DIR; }

    join_len = (uint32_t)strlen(CROSSTALK_EDA_IC_FPGA_FILESET_JOIN);
    need = (uint64_t)*io_len + st.size + (uint64_t)join_len + (uint64_t)LDG_STR_TERM_SIZE;
    if (need > cap) { return CROSSTALK_EDA_IC_FPGA_ERR_SRC_CAP; }

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, 0x0, &fh);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_rd(fh, dst + *io_len, st.size, &got);
    if (ldg_io_file_close(fh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga set src close failed"); }

    if (ret != LDG_ERR_AOK) { return ret; }

    if (got != st.size) { return LDG_ERR_IO_RD; }

    *io_len = *io_len + (uint32_t)got;
    memcpy(dst + *io_len, CROSSTALK_EDA_IC_FPGA_FILESET_JOIN, (uint64_t)join_len);
    *io_len = *io_len + join_len;
    dst[*io_len] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

static uint32_t fileset_size_get(const char *dir, const char *files, uint64_t *out_need)
{
    char name[CROSSTALK_EDA_IC_FPGA_FILESET_NAME_CAP] = LDG_ARR_ZERO_INIT;
    char path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    ldg_io_info_t st = LDG_STRUCT_ZERO_INIT;
    uint64_t need = (uint64_t)LDG_STR_TERM_SIZE;
    uint64_t join_len = (uint64_t)strlen(CROSSTALK_EDA_IC_FPGA_FILESET_JOIN);
    uint32_t files_len = (uint32_t)strlen(files);
    uint32_t name_len = 0;
    uint32_t pos = 0;

    *out_need = 0;

    for (pos = 0; pos <= files_len; pos = pos + 0x1)
    {
        if (files[pos] != CROSSTALK_EDA_IC_FPGA_FILESET_SEP && files[pos] != LDG_STR_TERM)
        {
            if (name_len >= CROSSTALK_EDA_IC_FPGA_FILESET_NAME_CAP - LDG_STR_TERM_SIZE) { return LDG_ERR_FUNC_ARG_INVALID; }

            name[name_len] = files[pos];
            name_len = name_len + 0x1;
            continue;
        }

        if (name_len == 0) { continue; }

        name[name_len] = LDG_STR_TERM;
        name_len = 0;

        if (ldg_io_path_join(dir, name, path, (uint64_t)sizeof(path)) != LDG_ERR_AOK || ldg_io_path_info_get(path, &st) != LDG_ERR_AOK) { continue; }

        need = need + st.size + join_len;
    }

    *out_need = need;

    return LDG_ERR_AOK;
}

static LDG_NOINLINE uint32_t fileset_rd(const char *dir, const char *files, char *dst, uint64_t cap, uint32_t *out_len, uint32_t *out_cunt)
{
    char name[CROSSTALK_EDA_IC_FPGA_FILESET_NAME_CAP];
    uint32_t files_len = 0;
    uint32_t pos = 0;
    uint32_t name_len = 0;
    uint32_t file_cunt = 0;
    uint32_t src_len = 0;
    uint32_t ret = 0;

    *out_len = 0x0;
    *out_cunt = 0x0;

    dst[0] = LDG_STR_TERM;
    files_len = (uint32_t)strlen(files);

    while (pos <= files_len)
    {
        if (files[pos] == CROSSTALK_EDA_IC_FPGA_FILESET_SEP || files[pos] == LDG_STR_TERM)
        {
            if (name_len > 0)
            {
                name[name_len] = LDG_STR_TERM;

                if (file_cunt >= CROSSTALK_EDA_IC_FPGA_FILESET_MAX) { return LDG_ERR_FULL; }

                ret = file_append(dir, name, dst, &src_len, cap);
                if (ret != LDG_ERR_AOK)
                {
                    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "fpga: set source read failed; file: %.127s; err: %u32", name, ret);
                    return ret;
                }

                file_cunt = file_cunt + 0x1;
                name_len = 0x0;
            }
        }
        else
        {
            if (name_len >= CROSSTALK_EDA_IC_FPGA_FILESET_NAME_CAP - LDG_STR_TERM_SIZE) { return LDG_ERR_FUNC_ARG_INVALID; }

            name[name_len] = files[pos];
            name_len = name_len + 0x1;
        }

        pos = pos + 0x1;
    }

    if (file_cunt == 0) { return LDG_ERR_EMPTY; }

    *out_len = src_len;
    *out_cunt = file_cunt;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_fileset_ck(crosstalk_eda_ic_fpga_ctx_t *ctx, const char *dir, const char *files, const char *top, const crosstalk_eda_ic_fpga_comp_t *caps, const char *clk_name)
{
    crosstalk_eda_ic_fpga_slot_t *trial = 0x0;
    ldg_mem_pool_t *live_pool = 0x0;
    uint64_t need = 0;
    uint32_t file_cunt = 0;
    uint32_t ret = 0;
    uint8_t rerun = LDG_TRUTH_TRUE;

    if (LDG_UNLIKELY(ctx == 0x0 || dir == 0x0 || files == 0x0 || top == 0x0 || caps == 0x0 || clk_name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (caps->logic_cells == 0 || caps->user_io == 0) { return CROSSTALK_EDA_IC_FPGA_ERR_COMP; }

    trial = &ctx->trial;

    if (trial->src == 0x0)
    {
        ret = ldg_mem_pool_alloc(ctx->src_pool, ctx->src_cap, (void **)&trial->src);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return CROSSTALK_EDA_IC_FPGA_ERR_POOL; }
    }

    ret = fileset_size_get(dir, files, &need);
    if (ret == LDG_ERR_AOK) { ret = src_pool_grow(ctx, need); }

    if (ret != LDG_ERR_AOK) { return ret; }

    name_set(ctx->src_dir, CROSSTALK_EDA_IC_FPGA_DIR_CAP, dir);

    ret = fileset_rd(dir, files, trial->src, ctx->src_cap, &trial->src_len, &file_cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    trial->comp = *caps;
    name_set(trial->top, CROSSTALK_EDA_IC_FPGA_TOP_CAP, top);
    name_set(trial->clk, CROSSTALK_EDA_IC_FPGA_TOP_CAP, clk_name);
    name_set(trial->fileset_dir, CROSSTALK_EDA_IC_FPGA_DIR_CAP, dir);
    name_set(trial->fileset_files, (uint32_t)sizeof(trial->fileset_files), files);

    live_pool = ctx->elab_pool;

    while (rerun == LDG_TRUTH_TRUE)
    {
        rerun = LDG_TRUTH_FALSE;

        ret = ldg_mem_pool_rst(ctx->trial_pool);
        if (ret != LDG_ERR_AOK) { break; }

        ctx->elab_pool = ctx->trial_pool;
        ret = slot_build(ctx, trial);
        ctx->elab_pool = live_pool;

        if (pool_starved_is(ret) == LDG_TRUTH_TRUE && pool_grow(&ctx->trial_pool) == LDG_ERR_AOK) { rerun = LDG_TRUTH_TRUE; }
        else if (ret == LDG_ERR_FULL && build_caps_grow(ctx) == LDG_ERR_AOK) { rerun = LDG_TRUTH_TRUE; }
    }

    trial->runnable = LDG_TRUTH_FALSE;

    if (ldg_mem_pool_rst(ctx->trial_pool) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: trial pool rst failed"); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: fileset ck; top: %.63s; files: %u32; bytes: %u32; err: %u32", top, file_cunt, trial->src_len, ret);

    return ret;
}

uint32_t crosstalk_eda_ic_fpga_fileset_program(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *dir, const char *files, const char *top, const crosstalk_eda_ic_fpga_comp_t *caps, const char *clk_name, uint32_t *out_bytes)
{
    crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    uint64_t need = 0;
    uint32_t file_cunt = 0;
    uint32_t src_len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || dir == 0x0 || files == 0x0 || top == 0x0 || caps == 0x0 || clk_name == 0x0 || out_bytes == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_bytes = 0x0;

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    if (caps->logic_cells == 0 || caps->user_io == 0) { return CROSSTALK_EDA_IC_FPGA_ERR_COMP; }

    slot = &ctx->slot[slot_idx];

    if (slot->src == 0x0)
    {
        ret = ldg_mem_pool_alloc(ctx->src_pool, ctx->src_cap, (void **)&slot->src);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return CROSSTALK_EDA_IC_FPGA_ERR_POOL; }
    }

    ret = fileset_size_get(dir, files, &need);
    if (ret == LDG_ERR_AOK) { ret = src_pool_grow(ctx, need); }

    if (ret != LDG_ERR_AOK) { return ret; }

    name_set(ctx->src_dir, CROSSTALK_EDA_IC_FPGA_DIR_CAP, dir);

    ret = fileset_rd(dir, files, slot->src, ctx->src_cap, &src_len, &file_cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    *out_bytes = src_len;

    slot->src_len = src_len;
    slot->comp = *caps;
    slot->acc_ns = 0x0;
    slot->cycles_run = 0x0;
    slot->live = LDG_TRUTH_TRUE;
    name_set(slot->top, CROSSTALK_EDA_IC_FPGA_TOP_CAP, top);
    name_set(slot->clk, CROSSTALK_EDA_IC_FPGA_TOP_CAP, clk_name);
    name_set(slot->fileset_dir, CROSSTALK_EDA_IC_FPGA_DIR_CAP, dir);
    name_set(slot->fileset_files, (uint32_t)sizeof(slot->fileset_files), files);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: set loaded; files: %u32; bytes: %u32; top: %s", file_cunt, src_len, top);

    ret = slot_all_rebuild(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    return slot->build_res;
}

uint32_t crosstalk_eda_ic_fpga_fileset_state_serialize(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t hold_hz, const char *emu_path, uint8_t *out, uint32_t cap, uint32_t *out_len)
{
    crosstalk_eda_ic_fpga_fileset_state_t state = LDG_STRUCT_ZERO_INIT;
    const crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    uint8_t emu_is = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (slot_idx >= CROSSTALK_EDA_IC_FPGA_SLOT_MAX) { return CROSSTALK_EDA_IC_FPGA_ERR_SLOT; }

    slot = &ctx->slot[slot_idx];

    emu_is = (slot->emu == LDG_TRUTH_TRUE && emu_path != 0x0 && emu_path[0] != LDG_STR_TERM) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (slot->live != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (emu_is != LDG_TRUTH_TRUE && slot->fileset_dir[0] == LDG_STR_TERM) { return LDG_ERR_AOK; }

    *out_len = (uint32_t)sizeof(state);

    if (out == 0x0) { return LDG_ERR_AOK; }

    if (cap < (uint32_t)sizeof(state)) { return LDG_ERR_BOUNDS; }

    name_set(state.dir, (uint32_t)sizeof(state.dir), slot->fileset_dir);
    name_set(state.files, (uint32_t)sizeof(state.files), slot->fileset_files);
    name_set(state.top, (uint32_t)sizeof(state.top), slot->top);
    name_set(state.clk, (uint32_t)sizeof(state.clk), slot->clk);
    state.hold_hz = hold_hz;
    state.emu = emu_is;

    if (emu_is == LDG_TRUTH_TRUE) { name_set(state.emu_path, (uint32_t)sizeof(state.emu_path), emu_path); }

    state.emitted = (slot->abi != 0x0 && emu_is != LDG_TRUTH_TRUE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    memcpy(out, &state, sizeof(state));
    return LDG_ERR_AOK;
}

#define FPGA_FILESET_SAVE_V1_MAX 32

typedef struct fpga_fileset_state_v1
{
    char dir[CROSSTALK_EDA_IC_FPGA_DIR_CAP];
    char files[FPGA_FILESET_SAVE_V1_MAX * CROSSTALK_EDA_IC_FPGA_FILESET_NAME_CAP];
    char top[CROSSTALK_EDA_IC_FPGA_TOP_CAP];
    char clk[CROSSTALK_EDA_IC_FPGA_TOP_CAP];
    char emu_path[CROSSTALK_EDA_IC_FPGA_DIR_CAP];
    uint64_t hold_hz;
    uint8_t emitted;
    uint8_t emu;
    uint8_t pudding[6];
} fpga_fileset_state_v1_t;

static void fileset_state_v1_lift(const uint8_t *buff, crosstalk_eda_ic_fpga_fileset_state_t *out)
{
    fpga_fileset_state_v1_t v1 = LDG_STRUCT_ZERO_INIT;

    memcpy(&v1, buff, sizeof(v1));
    memset(out, 0x0, sizeof(*out));

    v1.dir[sizeof(v1.dir) - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
    v1.files[sizeof(v1.files) - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
    v1.top[sizeof(v1.top) - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
    v1.clk[sizeof(v1.clk) - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
    v1.emu_path[sizeof(v1.emu_path) - LDG_STR_TERM_SIZE] = LDG_STR_TERM;

    name_set(out->dir, (uint32_t)sizeof(out->dir), v1.dir);
    name_set(out->files, (uint32_t)sizeof(out->files), v1.files);
    name_set(out->top, (uint32_t)sizeof(out->top), v1.top);
    name_set(out->clk, (uint32_t)sizeof(out->clk), v1.clk);
    name_set(out->emu_path, (uint32_t)sizeof(out->emu_path), v1.emu_path);
    out->hold_hz = v1.hold_hz;
    out->emitted = v1.emitted;
    out->emu = v1.emu;
}

uint32_t crosstalk_eda_ic_fpga_fileset_state_deserialize(const uint8_t *buff, uint32_t len, crosstalk_eda_ic_fpga_fileset_state_t *out)
{
    if (LDG_UNLIKELY(buff == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len == (uint32_t)sizeof(fpga_fileset_state_v1_t))
    {
        fileset_state_v1_lift(buff, out);
        return LDG_ERR_AOK;
    }

    if (len != (uint32_t)sizeof(*out)) { return LDG_ERR_INVALID; }

    memcpy(out, buff, sizeof(*out));

    out->dir[sizeof(out->dir) - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
    out->files[sizeof(out->files) - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
    out->top[sizeof(out->top) - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
    out->clk[sizeof(out->clk) - LDG_STR_TERM_SIZE] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}
