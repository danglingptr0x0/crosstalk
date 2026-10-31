#include <stdint.h>
#include <string.h>

#include <dangling/arch/amd64/atomic.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "eda/logic/probe.h"

#define PROBE_SEL_OPEN '['
#define PROBE_SEL_CLOSE ']'
#define PROBE_SEL_SPAN ':'
#define PROBE_SEL_TOK_LEN 1
#define PROBE_HEX_LEAD '0'
#define PROBE_HEX_MARK_AT 1
#define PROBE_HEX_MARK_LOWER 'x'
#define PROBE_HEX_MARK_UPPER 'X'
#define PROBE_HEX_PREFIX_LEN 2
#define PROBE_NUM_CAP 24

static uint32_t probe_num_parse(const char *text, uint64_t len, uint64_t *out)
{
    char num[PROBE_NUM_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t parsed = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(text == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = CROSSTALK_EDA_LOGIC_PROBE_ELEM_NONE;

    if (len == 0 || len + LDG_STR_TERM_SIZE > (uint64_t)sizeof(num)) { return LDG_ERR_FUNC_ARG_INVALID; }

    memcpy(num, text, len);
    num[len] = LDG_STR_TERM;

    if (len > (uint64_t)PROBE_HEX_PREFIX_LEN && num[0] == PROBE_HEX_LEAD && (num[PROBE_HEX_MARK_AT] == PROBE_HEX_MARK_LOWER || num[PROBE_HEX_MARK_AT] == PROBE_HEX_MARK_UPPER)) { ret = ldg_hex_to_dword(&num[PROBE_HEX_PREFIX_LEN], &parsed); }
    else { ret = ldg_str_to_dec(num, &parsed); }

    if (ret != LDG_ERR_AOK) { return ret; }

    *out = (uint64_t)parsed;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_logic_probe_sel_parse(const char *port, char *out_base, uint32_t cap, uint64_t *out_lo, uint64_t *out_hi)
{
    const char *open = 0x0;
    const char *span = 0x0;
    const char *close = 0x0;
    uint64_t base_len = 0;
    uint64_t lo = 0;
    uint64_t hi = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(port == 0x0 || out_base == 0x0 || out_lo == 0x0 || out_hi == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_lo = CROSSTALK_EDA_LOGIC_PROBE_ELEM_NONE;
    *out_hi = CROSSTALK_EDA_LOGIC_PROBE_ELEM_NONE;

    if (cap == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    out_base[0] = LDG_STR_TERM;

    open = strchr(port, PROBE_SEL_OPEN);
    base_len = open != 0x0 ? (uint64_t)(open - port) : (uint64_t)strlen(port);

    if (base_len == 0 || base_len + LDG_STR_TERM_SIZE > (uint64_t)cap) { return LDG_ERR_FUNC_ARG_INVALID; }

    memcpy(out_base, port, base_len);
    out_base[base_len] = LDG_STR_TERM;

    if (open == 0x0) { return LDG_ERR_AOK; }

    close = strchr(open, PROBE_SEL_CLOSE);
    if (close == 0x0 || close[PROBE_SEL_TOK_LEN] != LDG_STR_TERM) { return LDG_ERR_FUNC_ARG_INVALID; }

    span = strchr(open, PROBE_SEL_SPAN);

    if (span == 0x0)
    {
        ret = probe_num_parse(open + PROBE_SEL_TOK_LEN, (uint64_t)(close - open) - (uint64_t)PROBE_SEL_TOK_LEN, &lo);
        hi = lo;
    }
    else
    {
        ret = probe_num_parse(open + PROBE_SEL_TOK_LEN, (uint64_t)(span - open) - (uint64_t)PROBE_SEL_TOK_LEN, &lo);
        if (ret == LDG_ERR_AOK) { ret = probe_num_parse(span + PROBE_SEL_TOK_LEN, (uint64_t)(close - span) - (uint64_t)PROBE_SEL_TOK_LEN, &hi); }
    }

    if (ret != LDG_ERR_AOK) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out_lo = lo < hi ? lo : hi;
    *out_hi = lo < hi ? hi : lo;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_logic_probe_init(crosstalk_eda_logic_probe_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->live_cunt = 0x0;
    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_logic_probe_shutdown(crosstalk_eda_logic_probe_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_logic_probe_find(const crosstalk_eda_logic_probe_ctx_t *ctx, uint32_t slot_idx, const char *port, uint32_t *out)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || port == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;

    for (idx = 0; idx < CROSSTALK_EDA_LOGIC_PROBE_MAX; idx = idx + 0x1)
    {
        if (ctx->probe[idx].live != LDG_TRUTH_TRUE || ctx->probe[idx].slot_idx != slot_idx) { continue; }

        if (strcmp(ctx->probe[idx].port, port) != 0) { continue; }

        *out = idx;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t crosstalk_eda_logic_probe_host_attach(crosstalk_eda_logic_probe_ctx_t *ctx, uint32_t slot_idx, const char *port, uint32_t width, uint32_t *out)
{
    crosstalk_eda_logic_probe_t *probe = 0x0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || port == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (width == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (crosstalk_eda_logic_probe_find(ctx, slot_idx, port, out) == LDG_ERR_AOK) { return LDG_ERR_EXISTS; }

    for (idx = 0; idx < CROSSTALK_EDA_LOGIC_PROBE_MAX; idx = idx + 0x1) { if (ctx->probe[idx].live != LDG_TRUTH_TRUE) { break; } }

    if (idx >= CROSSTALK_EDA_LOGIC_PROBE_MAX) { return CROSSTALK_EDA_LOGIC_PROBE_ERR_FULL; }

    probe = &ctx->probe[idx];

    probe->hd = 0x0;
    probe->tail = 0x0;
    probe->dropped = 0x0;
    probe->slot_idx = slot_idx;
    probe->sig_idx = UINT32_MAX;
    probe->abi_sig = (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE;
    probe->abi_vals = 0x0;
    probe->abi_ck = LDG_TRUTH_TRUE;
    probe->width = width;
    probe->elem = CROSSTALK_EDA_LOGIC_PROBE_ELEM_NONE;
    probe->last_val = 0x0;
    probe->period = 0x0;
    probe->next_cycle = 0x0;

    ret = ldg_str_rbrcpy(probe->port, port, (uint64_t)sizeof(probe->port));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    probe->host = LDG_TRUTH_TRUE;
    probe->live = LDG_TRUTH_TRUE;
    ctx->live_cunt = ctx->live_cunt + 0x1;
    *out = idx;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "probe: host attach; slot: %u32; port: %.63s; width: %u32; probe: %u32", slot_idx, port, width, idx);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_logic_probe_sample_push(crosstalk_eda_logic_probe_ctx_t *ctx, uint32_t handle, uint64_t val, uint64_t cycle)
{
    crosstalk_eda_logic_probe_t *probe = 0x0;
    uint32_t slot = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (handle >= CROSSTALK_EDA_LOGIC_PROBE_MAX) { return CROSSTALK_EDA_LOGIC_PROBE_ERR_HANDLE; }

    probe = &ctx->probe[handle];

    if (probe->live != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (probe->hd - probe->tail >= (uint32_t)CROSSTALK_EDA_LOGIC_PROBE_RING_CAP)
    {
        probe->tail = probe->tail + 0x1;
        probe->dropped = probe->dropped + 0x1;
    }

    slot = probe->hd & (uint32_t)CROSSTALK_EDA_LOGIC_PROBE_RING_MASK;
    probe->ring[slot].cycle = cycle;
    probe->ring[slot].val = val;
    LDG_ST_RELEASE(probe->hd, probe->hd + 0x1);

    probe->last_val = val;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_logic_probe_attach(crosstalk_eda_logic_probe_ctx_t *ctx, uint32_t slot_idx, const crosstalk_hdl_exec_ctx_t *exec, const crosstalk_hdl_abi_t *abi, const char *port, uint32_t *out)
{
    crosstalk_eda_logic_probe_t *probe = 0x0;
    char base[CROSSTALK_EDA_LOGIC_PROBE_PORT_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t elem = 0;
    uint64_t elem_hi = 0;
    uint64_t last_val = 0;
    uint32_t sig_idx = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || port == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (crosstalk_eda_logic_probe_find(ctx, slot_idx, port, &idx) == LDG_ERR_AOK && ctx->probe[idx].host == LDG_TRUTH_TRUE) { *out = idx; return LDG_ERR_AOK; }

    idx = 0;

    ret = crosstalk_eda_logic_probe_sel_parse(port, base, (uint32_t)sizeof(base), &elem, &elem_hi);
    if (ret != LDG_ERR_AOK || elem != elem_hi) { return CROSSTALK_EDA_LOGIC_PROBE_ERR_SIG; }

    if (exec != 0x0 && (exec->ir_ctx == 0x0 || exec->vals == 0x0)) { exec = 0x0; }

    if (exec == 0x0 && (abi == 0x0 || abi->ok != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (exec != 0x0)
    {
        ret = crosstalk_hdl_exec_sig_find(exec, base, &sig_idx);
        if (ret != LDG_ERR_AOK) { return CROSSTALK_EDA_LOGIC_PROBE_ERR_SIG; }

        if (sig_idx >= exec->ir_ctx->sig_cunt) { return CROSSTALK_EDA_LOGIC_PROBE_ERR_SIG; }

        if (elem != CROSSTALK_EDA_LOGIC_PROBE_ELEM_NONE)
        {
            if (exec->ir_ctx->sigs[sig_idx].kind != (uint32_t)CROSSTALK_HDL_IR_SIG_MEM || elem >= (uint64_t)exec->ir_ctx->sigs[sig_idx].depth) { return CROSSTALK_EDA_LOGIC_PROBE_ERR_SIG; }

            ret = crosstalk_hdl_exec_mem_ld(exec, sig_idx, elem, &last_val);
            if (ret != LDG_ERR_AOK) { return CROSSTALK_EDA_LOGIC_PROBE_ERR_SIG; }
        }
        else { last_val = exec->vals[sig_idx]; }
    }
    else
    {
        if (abi->sig_find(base) == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { return CROSSTALK_EDA_LOGIC_PROBE_ERR_SIG; }

        sig_idx = UINT32_MAX;
    }

    for (idx = 0; idx < CROSSTALK_EDA_LOGIC_PROBE_MAX; idx = idx + 0x1) { if (ctx->probe[idx].live != LDG_TRUTH_TRUE) { break; } }

    if (idx >= CROSSTALK_EDA_LOGIC_PROBE_MAX) { return CROSSTALK_EDA_LOGIC_PROBE_ERR_FULL; }

    probe = &ctx->probe[idx];

    probe->hd = 0x0;
    probe->tail = 0x0;
    probe->dropped = 0x0;
    probe->slot_idx = slot_idx;
    probe->sig_idx = sig_idx;
    probe->abi_sig = (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE;
    probe->abi_vals = 0x0;
    probe->abi_ck = LDG_TRUTH_FALSE;
    probe->width = exec != 0x0 ? exec->ir_ctx->sigs[sig_idx].width : (uint32_t)CROSSTALK_EDA_LOGIC_PROBE_EMU_WIDTH;
    probe->elem = elem;
    probe->last_val = last_val;

    ret = ldg_str_rbrcpy(probe->port, port, (uint64_t)sizeof(probe->port));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    probe->period = 0x0;
    probe->next_cycle = 0x0;
    probe->live = LDG_TRUTH_TRUE;
    ctx->live_cunt = ctx->live_cunt + 0x1;
    *out = idx;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_logic_probe_period_attach(crosstalk_eda_logic_probe_ctx_t *ctx, uint32_t slot_idx, const crosstalk_hdl_exec_ctx_t *exec, const crosstalk_hdl_abi_t *abi, const char *port, uint64_t period, uint32_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (period == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = crosstalk_eda_logic_probe_attach(ctx, slot_idx, exec, abi, port, out);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->probe[*out].period = period;
    ctx->probe[*out].next_cycle = 0x0;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_logic_probe_detach(crosstalk_eda_logic_probe_ctx_t *ctx, uint32_t handle)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (handle >= CROSSTALK_EDA_LOGIC_PROBE_MAX) { return CROSSTALK_EDA_LOGIC_PROBE_ERR_HANDLE; }

    if (ctx->probe[handle].live != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ctx->probe[handle].live = LDG_TRUTH_FALSE;
    ctx->probe[handle].host = LDG_TRUTH_FALSE;
    ctx->probe[handle].hd = 0x0;
    ctx->probe[handle].tail = 0x0;
    ctx->probe[handle].port[0] = LDG_STR_TERM;

    if (ctx->live_cunt != 0x0) { ctx->live_cunt = ctx->live_cunt - 0x1; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_logic_probe_slot_detach(crosstalk_eda_logic_probe_ctx_t *ctx, uint32_t slot_idx)
{
    uint32_t idx = 0;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < CROSSTALK_EDA_LOGIC_PROBE_MAX; idx = idx + 0x1)
    {
        if (ctx->probe[idx].live != LDG_TRUTH_TRUE) { continue; }

        if (ctx->probe[idx].slot_idx != slot_idx) { continue; }

        ret = crosstalk_eda_logic_probe_detach(ctx, idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_logic_probe_abi_rst(crosstalk_eda_logic_probe_ctx_t *ctx, uint32_t slot_idx)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < CROSSTALK_EDA_LOGIC_PROBE_MAX; idx = idx + 0x1)
    {
        if (ctx->probe[idx].slot_idx != slot_idx) { continue; }

        ctx->probe[idx].abi_sig = (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE;
        ctx->probe[idx].abi_vals = 0x0;
        ctx->probe[idx].abi_ck = LDG_TRUTH_FALSE;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_logic_probe_cycle_sample(crosstalk_eda_logic_probe_ctx_t *ctx, uint32_t slot_idx, const crosstalk_hdl_exec_ctx_t *exec, const crosstalk_hdl_abi_t *abi, uint64_t cycle)
{
    uint8_t packed = (abi != 0x0 && abi->ok == LDG_TRUTH_TRUE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || (exec == 0x0 && packed != LDG_TRUTH_TRUE))) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < CROSSTALK_EDA_LOGIC_PROBE_MAX; idx = idx + 0x1)
    {
        crosstalk_eda_logic_probe_t *probe = &ctx->probe[idx];
        uint64_t val = 0;
        uint32_t slot = 0;

        if (probe->live != LDG_TRUTH_TRUE) { continue; }

        if (probe->slot_idx != slot_idx) { continue; }

        if (probe->host == LDG_TRUTH_TRUE) { continue; }

        if (packed == LDG_TRUTH_TRUE)
        {
            if (probe->abi_ck != LDG_TRUTH_TRUE)
            {
                char base[CROSSTALK_EDA_LOGIC_PROBE_PORT_CAP] = LDG_ARR_ZERO_INIT;
                uint64_t elem_lo = 0;
                uint64_t elem_hi = 0;

                probe->abi_sig = (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE;

                if (crosstalk_eda_logic_probe_sel_parse(probe->port, base, (uint32_t)sizeof(base), &elem_lo, &elem_hi) == LDG_ERR_AOK) { probe->abi_sig = abi->sig_find(base); }

                probe->abi_ck = LDG_TRUTH_TRUE;
                probe->abi_vals = 0x0;

                if (probe->abi_sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "probe: port absent from the compiled component; port: %.63s", probe->port); }
                else { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "probe: bound to the compiled component; port: %.63s; abi sig: %u32", probe->port, probe->abi_sig); }
            }

            if (probe->abi_sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { continue; }
        }

        if (probe->period != 0x0)
        {
            if (cycle < probe->next_cycle) { continue; }

            probe->next_cycle = cycle + probe->period;
        }

        if (probe->elem != CROSSTALK_EDA_LOGIC_PROBE_ELEM_NONE) { if (packed == LDG_TRUTH_TRUE) { val = abi->mem_ld(probe->abi_sig, probe->elem); }
            else if (crosstalk_hdl_exec_mem_ld(exec, probe->sig_idx, probe->elem, &val) != LDG_ERR_AOK) { continue; } }
        else if (probe->abi_vals != 0x0) { val = probe->abi_vals[probe->abi_sig]; }
        else if (packed == LDG_TRUTH_TRUE) { val = abi->peek(probe->abi_sig); }
        else { val = exec->vals[probe->sig_idx]; }

        if (probe->period == 0x0 && val == probe->last_val) { continue; }

        probe->last_val = val;

        if (probe->hd - probe->tail >= (uint32_t)CROSSTALK_EDA_LOGIC_PROBE_RING_CAP)
        {
            probe->tail = probe->tail + 0x1;
            probe->dropped = probe->dropped + 0x1;
        }

        slot = probe->hd & (uint32_t)CROSSTALK_EDA_LOGIC_PROBE_RING_MASK;
        probe->ring[slot].cycle = cycle;
        probe->ring[slot].val = val;
        LDG_ST_RELEASE(probe->hd, probe->hd + 0x1);
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_logic_probe_drain(crosstalk_eda_logic_probe_ctx_t *ctx, uint32_t handle, crosstalk_eda_logic_probe_sample_t *out, uint32_t cap, uint32_t *out_cunt)
{
    crosstalk_eda_logic_probe_t *probe = 0x0;
    uint32_t cunt = 0;
    uint32_t hd = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0 || out_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0x0;

    if (handle >= CROSSTALK_EDA_LOGIC_PROBE_MAX) { return CROSSTALK_EDA_LOGIC_PROBE_ERR_HANDLE; }

    probe = &ctx->probe[handle];
    if (probe->live != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    hd = LDG_LD_ACQUIRE(probe->hd);

    while (probe->tail != hd && cunt < cap)
    {
        out[cunt] = probe->ring[probe->tail & (uint32_t)CROSSTALK_EDA_LOGIC_PROBE_RING_MASK];
        LDG_ST_RELEASE(probe->tail, probe->tail + 0x1);
        cunt = cunt + 0x1;
    }

    *out_cunt = cunt;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_logic_probe_dropped_get(const crosstalk_eda_logic_probe_ctx_t *ctx, uint32_t handle, uint64_t *out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (handle >= CROSSTALK_EDA_LOGIC_PROBE_MAX) { return CROSSTALK_EDA_LOGIC_PROBE_ERR_HANDLE; }

    *out = ctx->probe[handle].dropped;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_logic_probe_port_get(const crosstalk_eda_logic_probe_ctx_t *ctx, uint32_t handle, const char **out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (handle >= CROSSTALK_EDA_LOGIC_PROBE_MAX) { return CROSSTALK_EDA_LOGIC_PROBE_ERR_HANDLE; }

    if (ctx->probe[handle].live != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    *out = ctx->probe[handle].port;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_logic_probe_live_is(const crosstalk_eda_logic_probe_ctx_t *ctx, uint32_t handle)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (handle >= CROSSTALK_EDA_LOGIC_PROBE_MAX) { return LDG_ERR_FUNC_ARG_INVALID; }

    return ctx->probe[handle].live == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}
