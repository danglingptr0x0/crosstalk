#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/logging/logging.h>

#include <dangling/arch/dangcsa/dangcsa.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "core/log.h"
#include "eda/ui/stackview.h"
#include "ui/cell.h"
#include "ui/theme.h"

#define STACKVIEW_STEP 1
#define STACKVIEW_ORIG_COL 0
#define STACKVIEW_WORD_BYTES 8
#define STACKVIEW_DRAIN_CAP 128
#define STACKVIEW_HDR_LIVE_FMT "sp: "
#define STACKVIEW_HDR_TAIL_FMT "; base: 0x%" PRIX64 "; words: %u%.16s"
#define STACKVIEW_HDR_FROZEN_TEXT "; frozen at fault"
#define STACKVIEW_HDR_RUN_TEXT ""
#define STACKVIEW_ADDR_FMT "0x%08" PRIX64
#define STACKVIEW_WORD_FMT " 0x%08" PRIX64
#define STACKVIEW_RET_TEXT " ret from call"
#define STACKVIEW_EMPTY_TEXT "stack empty; sp is at the base"
#define STACKVIEW_DEAD_TEXT "no running design"
#define STACKVIEW_HINT_TEXT "type a signal name to record its history; type it again to release"
#define STACKVIEW_LANE_HDR_FMT "history %.31s; events: %u"
#define STACKVIEW_LANE_ROW_CYCLE_FMT "%12" PRIu64 " "
#define STACKVIEW_LANE_ROW_VAL_FMT "0x%" PRIX64
#define STACKVIEW_PROMPT_FMT "> %.47s_"
#define STACKVIEW_FRAME_NUM_FMT "#%u "
#define STACKVIEW_FRAME_OFF_FMT " %" PRIu64 "Mi+%" PRIX64
#define STACKVIEW_LINE_CAP 128
#define STACKVIEW_MMIO_BASE 0x90000000ULL
#define STACKVIEW_MMIO_END 0xA0000000ULL
#define STACKVIEW_NOTE_TEXT_WORDS 4
#define STACKVIEW_NOTE_STR_MIN 4
#define STACKVIEW_NOTE_MMIO_TEXT " mmio"
#define STACKVIEW_NOTE_STACK_TEXT " stack"
#define STACKVIEW_NOTE_STR_FMT " -> \"%.16s\""
#define STACKVIEW_NOTE_PTR_FMT " -> 0x%" PRIX64
#define STACKVIEW_RING_HDR_FMT "branches; taken: %u; runs: %u; newest first"
#define STACKVIEW_RING_SPAN_FMT "0x%08" PRIX64 " -> 0x%08" PRIX64
#define STACKVIEW_RING_RUN_FMT " x%u"
#define STACKVIEW_RING_SP_FMT " sp: 0x%" PRIX64
#define STACKVIEW_RING_ASM_FMT " %.31s"
#define STACKVIEW_RING_DEAD_TEXT "no control transfers recorded"
#define STACKVIEW_ASM_CAP 48
#define STACKVIEW_ASM_FAIL_TEXT "bad"

static void stackview_ring_push(crosstalk_eda_ui_stackview_lane_t *lane, uint64_t cycle, uint64_t val)
{
    uint32_t at = 0;

    if (lane->cunt < (uint32_t)CROSSTALK_EDA_UI_STACKVIEW_LANE_HIST_CAP)
    {
        at = (lane->hd + lane->cunt) % (uint32_t)CROSSTALK_EDA_UI_STACKVIEW_LANE_HIST_CAP;
        lane->cunt = lane->cunt + 0x1;
    }
    else
    {
        at = lane->hd;
        lane->hd = (lane->hd + 0x1) % (uint32_t)CROSSTALK_EDA_UI_STACKVIEW_LANE_HIST_CAP;
    }

    lane->cycle[at] = cycle;
    lane->val[at] = val;
}

static void stackview_pump(crosstalk_eda_ui_stackview_ctx_t *ctx)
{
    crosstalk_eda_logic_probe_sample_t drain[STACKVIEW_DRAIN_CAP];
    uint32_t got = 0;
    uint32_t lane_idx = 0;
    uint32_t idx = 0;

    if (ctx->probes == 0x0) { return; }

    for (lane_idx = 0; lane_idx < ctx->lane_cunt; lane_idx = lane_idx + 0x1)
    {
        if (ctx->lane[lane_idx].live != LDG_TRUTH_TRUE) { continue; }

        if (crosstalk_eda_logic_probe_live_is(ctx->probes, ctx->lane[lane_idx].probe_handle) != LDG_TRUTH_TRUE) { continue; }

        if (crosstalk_eda_logic_probe_drain(ctx->probes, ctx->lane[lane_idx].probe_handle, drain, (uint32_t)STACKVIEW_DRAIN_CAP, &got) != LDG_ERR_AOK) { continue; }

        for (idx = 0; idx < got; idx = idx + 0x1) { stackview_ring_push(&ctx->lane[lane_idx], drain[idx].cycle, drain[idx].val); }
    }
}

static void stackview_refresh(crosstalk_eda_ui_stackview_ctx_t *ctx)
{
    uint64_t sp = 0;
    uint64_t span = 0;
    uint32_t width = 0;
    uint32_t got = 0;
    uint32_t idx = 0;

    ctx->word_cunt = 0;
    ctx->ring_cunt = 0;
    ctx->run_cunt = 0;

    if (ctx->fpga == 0x0) { return; }

    if (crosstalk_eda_ic_fpga_ring_get(ctx->fpga, ctx->slot, ctx->ring, (uint32_t)CROSSTALK_EDA_UI_STACKVIEW_RING_CAP, &ctx->ring_cunt) != LDG_ERR_AOK) { ctx->ring_cunt = 0; }

    {
        uint32_t entries = ctx->ring_cunt / (uint32_t)CROSSTALK_HDL_ABI_RING_STRIDE;
        uint32_t at = 0;

        for (at = 0; at < entries; at = at + 0x1)
        {
            uint64_t entry_from = ctx->ring[at * (uint32_t)CROSSTALK_HDL_ABI_RING_STRIDE + (uint32_t)CROSSTALK_HDL_ABI_RING_FROM_AT];
            uint64_t entry_instr = ctx->ring[at * (uint32_t)CROSSTALK_HDL_ABI_RING_STRIDE + (uint32_t)CROSSTALK_HDL_ABI_RING_INSTR_AT];
            uint64_t entry_to = ctx->ring[at * (uint32_t)CROSSTALK_HDL_ABI_RING_STRIDE + (uint32_t)CROSSTALK_HDL_ABI_RING_TO_AT];
            uint64_t entry_sp = ctx->ring[at * (uint32_t)CROSSTALK_HDL_ABI_RING_STRIDE + (uint32_t)CROSSTALK_HDL_ABI_RING_SP_AT];

            if (ctx->run_cunt != 0)
            {
                crosstalk_eda_ui_stackview_run_t *tail = &ctx->run[ctx->run_cunt - 0x1];

                if (tail->from == entry_from && tail->to == entry_to && tail->instr == entry_instr && tail->sp == entry_sp)
                {
                    tail->cunt = tail->cunt + 0x1;

                    continue;
                }
            }

            if (ctx->run_cunt >= (uint32_t)CROSSTALK_HDL_ABI_RING_ENTRY_MAX) { break; }

            ctx->run[ctx->run_cunt].from = entry_from;
            ctx->run[ctx->run_cunt].to = entry_to;
            ctx->run[ctx->run_cunt].instr = entry_instr;
            ctx->run[ctx->run_cunt].sp = entry_sp;
            ctx->run[ctx->run_cunt].cunt = 0x1;
            ctx->run_cunt = ctx->run_cunt + 0x1;
        }
    }

    if (crosstalk_eda_ic_fpga_sig_peek(ctx->fpga, ctx->slot, CROSSTALK_EDA_UI_STACKVIEW_SP_SIG, &sp, &width) != LDG_ERR_AOK) { return; }

    if (sp == 0) { return; }

    ctx->sp = sp;

    if (sp > ctx->stack_base) { ctx->stack_base = sp; }

    span = (ctx->stack_base - sp) / (uint64_t)STACKVIEW_WORD_BYTES;
    if (span > (uint64_t)CROSSTALK_EDA_UI_STACKVIEW_ROW_CAP) { span = (uint64_t)CROSSTALK_EDA_UI_STACKVIEW_ROW_CAP; }

    if (span == 0) { return; }

    if (crosstalk_eda_ic_fpga_mem_rd(ctx->fpga, ctx->slot, ctx->mem_sig, sp / (uint64_t)STACKVIEW_WORD_BYTES, ctx->word, (uint32_t)span, &got) != LDG_ERR_AOK) { return; }

    ctx->word_cunt = got;

    for (idx = 0; idx < got; idx = idx + 0x1)
    {
        ldg_dangcsa_decoded_t decoded = LDG_STRUCT_ZERO_INIT;
        uint64_t caller = 0;
        uint32_t caller_got = 0;

        ctx->call_mark[idx] = LDG_TRUTH_FALSE;

        if (ctx->word[idx] < (uint64_t)STACKVIEW_WORD_BYTES) { continue; }

        if (ctx->word[idx] % (uint64_t)STACKVIEW_WORD_BYTES != 0) { continue; }

        if (crosstalk_eda_ic_fpga_mem_rd(ctx->fpga, ctx->slot, ctx->mem_sig, (ctx->word[idx] - (uint64_t)STACKVIEW_WORD_BYTES) / (uint64_t)STACKVIEW_WORD_BYTES, &caller, 0x1, &caller_got) != LDG_ERR_AOK || caller_got == 0) { continue; }

        if (ldg_dangcsa_decode(caller, &decoded) != LDG_ERR_AOK) { continue; }

        if (decoded.opcode == (uint8_t)LDG_DANGCSA_OP_CALL) { ctx->call_mark[idx] = LDG_TRUTH_TRUE; }
    }
}

static void stackview_query_exec(crosstalk_eda_ui_stackview_ctx_t *ctx)
{
    uint32_t handle = UINT32_MAX;

    if (ctx->query[0] == LDG_STR_TERM) { return; }

    if (ctx->req != (uint32_t)CROSSTALK_EDA_UI_STACKVIEW_REQ_NONE)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "stackview: request pending; sig: %.63s", ctx->req_sig);

        return;
    }

    if (ldg_str_rbrcpy(ctx->req_sig, ctx->query, (uint64_t)sizeof(ctx->req_sig)) != LDG_ERR_AOK) { return; }

    ctx->req = crosstalk_eda_ui_stackview_lane_handle_get(ctx, ctx->req_sig, &handle) == LDG_ERR_AOK ? (uint32_t)CROSSTALK_EDA_UI_STACKVIEW_REQ_REMOVE : (uint32_t)CROSSTALK_EDA_UI_STACKVIEW_REQ_ADD;
}

static uint32_t stackview_key_isr(uint64_t win_id, void *user, const ldg_term_key_event_t *key, uint8_t *consumed)
{
    crosstalk_eda_ui_stackview_ctx_t *ctx = (crosstalk_eda_ui_stackview_ctx_t *)user;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || key == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    if (win_id != (uint64_t)CROSSTALK_EDA_UI_STACKVIEW_WINDOW_ID) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (key->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE) { return LDG_ERR_AOK; }

    if (key->key == (uint8_t)LDG_TERM_KEY_BACKSPACE)
    {
        if (ctx->query_len != 0)
        {
            ctx->query_len = ctx->query_len - (uint32_t)STACKVIEW_STEP;
            ctx->query[ctx->query_len] = LDG_STR_TERM;
        }

        *consumed = LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    if (key->key == (uint8_t)LDG_TERM_KEY_ESCAPE || key->key == (uint8_t)LDG_TERM_KEY_ENTER)
    {
        if (key->key == (uint8_t)LDG_TERM_KEY_ENTER) { stackview_query_exec(ctx); }

        ctx->query_len = 0;
        ctx->query[0] = LDG_STR_TERM;
        *consumed = LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    for (at = 0; at < key->text_cunt && at < (uint32_t)LDG_TERM_KEY_TEXT_MAX; at = at + (uint32_t)STACKVIEW_STEP)
    {
        if (key->text[at] < (uint32_t)LDG_ASCII_PRINTABLE_MIN || key->text[at] > (uint32_t)LDG_ASCII_PRINTABLE_MAX) { continue; }

        *consumed = LDG_TRUTH_TRUE;

        if (ctx->query_len + LDG_STR_TERM_SIZE >= (uint32_t)CROSSTALK_EDA_UI_STACKVIEW_QUERY_CAP) { break; }

        ctx->query[ctx->query_len] = (char)key->text[at];
        ctx->query_len = ctx->query_len + (uint32_t)STACKVIEW_STEP;
        ctx->query[ctx->query_len] = LDG_STR_TERM;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_stackview_init(crosstalk_eda_ui_stackview_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->win.scroll_on = LDG_TRUTH_TRUE;
    ctx->win.open = LDG_TRUTH_FALSE;
    ctx->open = LDG_TRUTH_FALSE;
    ctx->ok = LDG_TRUTH_TRUE;

    return ldg_str_rbrcpy(ctx->mem_sig, CROSSTALK_EDA_UI_STACKVIEW_MEM_SIG, (uint64_t)sizeof(ctx->mem_sig));
}

uint32_t crosstalk_eda_ui_stackview_mem_set(crosstalk_eda_ui_stackview_ctx_t *ctx, const char *sig)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || sig == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(sig[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_str_rbrcpy(ctx->mem_sig, sig, (uint64_t)sizeof(ctx->mem_sig));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->stack_base = 0;
    ctx->word_cunt = 0;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_stackview_bind(crosstalk_eda_ui_stackview_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, crosstalk_eda_logic_probe_ctx_t *probes, uint32_t slot)
{
    if (LDG_UNLIKELY(ctx == 0x0 || fpga == 0x0 || probes == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->fpga = fpga;
    ctx->probes = probes;
    ctx->slot = slot;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_stackview_open_set(crosstalk_eda_ui_stackview_ctx_t *ctx, uint8_t open)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->open = open;
    ctx->win.open = open;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_ui_stackview_open_is(const crosstalk_eda_ui_stackview_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->win.open != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_eda_ui_stackview_fault_set(crosstalk_eda_ui_stackview_ctx_t *ctx, const uint64_t *rec, uint32_t cunt)
{
    if (LDG_UNLIKELY(ctx == 0x0 || rec == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (cunt < (uint32_t)CROSSTALK_EDA_UI_STACKVIEW_FAULT_CUNT) { return LDG_ERR_BOUNDS; }

    if (rec[CROSSTALK_HDL_ABI_FAULT_VALID_AT] == 0)
    {
        ctx->fault_has = LDG_TRUTH_FALSE;
        ctx->frozen = LDG_TRUTH_FALSE;

        return LDG_ERR_AOK;
    }

    if (ctx->fault_has == LDG_TRUTH_TRUE && ctx->fault[CROSSTALK_HDL_ABI_FAULT_STEP_AT] == rec[CROSSTALK_HDL_ABI_FAULT_STEP_AT]) { return LDG_ERR_AOK; }

    memcpy(ctx->fault, rec, sizeof(ctx->fault));
    ctx->fault_has = LDG_TRUTH_TRUE;
    ctx->frozen = LDG_TRUTH_TRUE;
    ctx->capture_want = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_stackview_rst(crosstalk_eda_ui_stackview_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->frozen = LDG_TRUTH_FALSE;
    ctx->capture_want = LDG_TRUTH_FALSE;
    ctx->fault_has = LDG_TRUTH_FALSE;
    ctx->stack_base = 0;
    ctx->word_cunt = 0;
    ctx->ring_cunt = 0;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_stackview_req_get(const crosstalk_eda_ui_stackview_ctx_t *ctx, char *out_sig, uint64_t out_size, uint32_t *out_kind)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_sig == 0x0 || out_kind == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_kind = (uint32_t)CROSSTALK_EDA_UI_STACKVIEW_REQ_NONE;

    if (ctx->req == (uint32_t)CROSSTALK_EDA_UI_STACKVIEW_REQ_NONE) { return LDG_ERR_NOT_FOUND; }

    if (ldg_str_rbrcpy(out_sig, ctx->req_sig, out_size) != LDG_ERR_AOK) { return LDG_ERR_STR_TRUNC; }

    *out_kind = ctx->req;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_stackview_req_clear(crosstalk_eda_ui_stackview_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->req = (uint32_t)CROSSTALK_EDA_UI_STACKVIEW_REQ_NONE;
    ctx->req_sig[0] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_stackview_lane_add(crosstalk_eda_ui_stackview_ctx_t *ctx, const char *name, uint32_t probe_handle)
{
    crosstalk_eda_ui_stackview_lane_t *lane = 0x0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (at = 0; at < ctx->lane_cunt; at = at + 0x1) { if (strcmp(ctx->lane[at].name, name) == 0) { return LDG_ERR_EXISTS; } }

    if (ctx->lane_cunt >= (uint32_t)CROSSTALK_EDA_UI_STACKVIEW_LANE_CAP) { return LDG_ERR_FULL; }

    lane = &ctx->lane[ctx->lane_cunt];
    memset(lane, 0x0, sizeof(*lane));

    if (ldg_str_rbrcpy(lane->name, name, sizeof(lane->name)) != LDG_ERR_AOK) { return LDG_ERR_STR_TRUNC; }

    lane->probe_handle = probe_handle;
    lane->live = LDG_TRUTH_TRUE;
    ctx->lane_cunt = ctx->lane_cunt + 0x1;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_stackview_lane_handle_get(const crosstalk_eda_ui_stackview_ctx_t *ctx, const char *name, uint32_t *out)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;

    for (at = 0; at < ctx->lane_cunt; at = at + 0x1)
    {
        if (strcmp(ctx->lane[at].name, name) != 0) { continue; }

        *out = ctx->lane[at].probe_handle;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t crosstalk_eda_ui_stackview_lane_remove(crosstalk_eda_ui_stackview_ctx_t *ctx, const char *name)
{
    uint32_t at = 0;
    uint32_t tail = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (at = 0; at < ctx->lane_cunt; at = at + 0x1)
    {
        if (strcmp(ctx->lane[at].name, name) != 0) { continue; }

        for (tail = at; tail + 0x1 < ctx->lane_cunt; tail = tail + 0x1) { ctx->lane[tail] = ctx->lane[tail + 0x1]; }

        ctx->lane_cunt = ctx->lane_cunt - 0x1;
        memset(&ctx->lane[ctx->lane_cunt], 0x0, sizeof(ctx->lane[ctx->lane_cunt]));

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t stackview_note_build(crosstalk_eda_ui_stackview_ctx_t *ctx, uint64_t val, char *out, uint32_t cap, uint32_t *out_col)
{
    uint64_t tgt = 0;
    uint64_t chain = 0;
    uint32_t got = 0;
    uint32_t pos = 0;
    uint32_t at = 0;
    int32_t wrote = 0;

    out[0] = LDG_STR_TERM;
    *out_col = CROSSTALK_UI_THEME_DIM_RGBA;

    if (val == 0) { return LDG_ERR_AOK; }

    if (val >= STACKVIEW_MMIO_BASE && val < STACKVIEW_MMIO_END)
    {
        wrote = snprintf(out, cap, STACKVIEW_NOTE_MMIO_TEXT);
        *out_col = CROSSTALK_UI_THEME_ACCENT_RGBA;

        return wrote < 0 ? LDG_ERR_STR_FMT : LDG_ERR_AOK;
    }

    if (ctx->sp != 0 && val >= ctx->sp && val <= ctx->stack_base)
    {
        wrote = snprintf(out, cap, STACKVIEW_NOTE_STACK_TEXT);
        *out_col = CROSSTALK_UI_THEME_STATUS_RGBA;

        return wrote < 0 ? LDG_ERR_STR_FMT : LDG_ERR_AOK;
    }

    if (val % (uint64_t)STACKVIEW_WORD_BYTES != 0 || val >= CROSSTALK_EDA_IC_FPGA_RAM_WINDOW_TOP) { return LDG_ERR_AOK; }

    if (crosstalk_eda_ic_fpga_mem_rd(ctx->fpga, ctx->slot, ctx->mem_sig, val / (uint64_t)STACKVIEW_WORD_BYTES, &tgt, 0x1, &got) != LDG_ERR_AOK || got == 0) { return LDG_ERR_AOK; }

    {
        uint64_t text_words[STACKVIEW_NOTE_TEXT_WORDS] = LDG_ARR_ZERO_INIT;
        char text[STACKVIEW_NOTE_TEXT_WORDS * STACKVIEW_WORD_BYTES + LDG_STR_TERM_SIZE] = LDG_ARR_ZERO_INIT;
        uint32_t printable = 0;

        if (crosstalk_eda_ic_fpga_mem_rd(ctx->fpga, ctx->slot, ctx->mem_sig, val / (uint64_t)STACKVIEW_WORD_BYTES, text_words, (uint32_t)STACKVIEW_NOTE_TEXT_WORDS, &got) == LDG_ERR_AOK)
        {
            for (at = 0; at < got * (uint32_t)STACKVIEW_WORD_BYTES; at = at + 0x1)
            {
                char ch = (char)((text_words[at / (uint32_t)STACKVIEW_WORD_BYTES] >> ((at % (uint32_t)STACKVIEW_WORD_BYTES) * (uint32_t)LDG_BYTE_BITS)) & (uint64_t)LDG_BYTE_MASK);

                if (ch < (char)LDG_ASCII_PRINTABLE_MIN || ch > (char)LDG_ASCII_PRINTABLE_MAX) { break; }

                text[printable] = ch;
                printable = printable + 0x1;
            }

            text[printable] = LDG_STR_TERM;

            if (printable >= (uint32_t)STACKVIEW_NOTE_STR_MIN)
            {
                wrote = snprintf(out, cap, STACKVIEW_NOTE_STR_FMT, text);
                *out_col = CROSSTALK_UI_THEME_PALE_RGBA;

                return wrote < 0 ? LDG_ERR_STR_FMT : LDG_ERR_AOK;
            }
        }
    }

    wrote = snprintf(out, cap, STACKVIEW_NOTE_PTR_FMT, tgt);
    if (wrote < 0) { return LDG_ERR_STR_FMT; }

    pos = (uint32_t)wrote;
    *out_col = CROSSTALK_UI_THEME_STATUS_RGBA;

    if (tgt != 0 && tgt % (uint64_t)STACKVIEW_WORD_BYTES == 0 && tgt < CROSSTALK_EDA_IC_FPGA_RAM_WINDOW_TOP) { if (crosstalk_eda_ic_fpga_mem_rd(ctx->fpga, ctx->slot, ctx->mem_sig, tgt / (uint64_t)STACKVIEW_WORD_BYTES, &chain, 0x1, &got) == LDG_ERR_AOK && got != 0) { if (snprintf(&out[pos], cap - pos, STACKVIEW_NOTE_PTR_FMT, chain) < 0) { return LDG_ERR_STR_FMT; } } }

    return LDG_ERR_AOK;
}

static uint32_t stackview_seg_put(ldg_term_render_t *render, uint32_t *pos_col, uint32_t row, const char *text, uint32_t rgba)
{
    uint32_t seg_w = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || pos_col == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_ui_cell_rgba_put(render, *pos_col, row, text, (uint64_t)STACKVIEW_LINE_CAP, rgba, &seg_w);
    if (ret != LDG_ERR_AOK) { return ret; }

    *pos_col = *pos_col + seg_w;

    return LDG_ERR_AOK;
}

static uint32_t stackview_frame_draw(ldg_term_render_t *render, uint32_t frame, uint64_t addr, uint32_t row)
{
    char line[STACKVIEW_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t pos_col = STACKVIEW_ORIG_COL;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (snprintf(line, sizeof(line), STACKVIEW_FRAME_NUM_FMT, frame) < 0) { return LDG_ERR_STR_FMT; }

    ret = stackview_seg_put(render, &pos_col, row, line, (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (snprintf(line, sizeof(line), STACKVIEW_ADDR_FMT, addr) < 0) { return LDG_ERR_STR_FMT; }

    ret = stackview_seg_put(render, &pos_col, row, line, frame == 0 ? (uint32_t)CROSSTALK_UI_THEME_ACCENT_RGBA : (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (addr < (uint64_t)LDG_MIB) { return LDG_ERR_AOK; }

    if (snprintf(line, sizeof(line), STACKVIEW_FRAME_OFF_FMT, addr / (uint64_t)LDG_MIB, addr % (uint64_t)LDG_MIB) < 0) { return LDG_ERR_STR_FMT; }

    return stackview_seg_put(render, &pos_col, row, line, (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA);
}

static uint32_t stackview_ring_draw(crosstalk_eda_ui_stackview_ctx_t *ctx, ldg_term_render_t *render, uint32_t *row)
{
    char line[STACKVIEW_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t entries = 0;
    uint32_t shown = 0;
    uint32_t pos_col = STACKVIEW_ORIG_COL;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0 || row == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    entries = ctx->ring_cunt / (uint32_t)CROSSTALK_HDL_ABI_RING_STRIDE;

    if (snprintf(line, sizeof(line), STACKVIEW_RING_HDR_FMT, entries, ctx->run_cunt) < 0) { return LDG_ERR_STR_FMT; }

    ret = stackview_seg_put(render, &pos_col, *row, line, (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *row = *row + (uint32_t)STACKVIEW_STEP;

    if (entries == 0)
    {
        pos_col = STACKVIEW_ORIG_COL;

        ret = stackview_seg_put(render, &pos_col, *row, STACKVIEW_RING_DEAD_TEXT, (uint32_t)CROSSTALK_UI_THEME_DEAD_RGBA);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        *row = *row + (uint32_t)STACKVIEW_STEP;
    }

    for (shown = 0; shown < ctx->run_cunt; shown = shown + (uint32_t)STACKVIEW_STEP)
    {
        const crosstalk_eda_ui_stackview_run_t *run = &ctx->run[shown];
        ldg_dangcsa_decoded_t decoded = LDG_STRUCT_ZERO_INIT;
        char asm_text[STACKVIEW_ASM_CAP] = LDG_ARR_ZERO_INIT;

        pos_col = STACKVIEW_ORIG_COL;

        if (snprintf(line, sizeof(line), STACKVIEW_RING_SPAN_FMT, run->from, run->to) < 0) { return LDG_ERR_STR_FMT; }

        ret = stackview_seg_put(render, &pos_col, *row, line, shown == 0 ? (uint32_t)CROSSTALK_UI_THEME_ACCENT_RGBA : (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (run->cunt != (uint32_t)STACKVIEW_STEP)
        {
            if (snprintf(line, sizeof(line), STACKVIEW_RING_RUN_FMT, run->cunt) < 0) { return LDG_ERR_STR_FMT; }

            ret = stackview_seg_put(render, &pos_col, *row, line, (uint32_t)CROSSTALK_UI_THEME_ACCENT_RGBA);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        if (ldg_dangcsa_decode(run->instr, &decoded) != LDG_ERR_AOK || ldg_dangcsa_disasm(&decoded, asm_text, sizeof(asm_text)) != LDG_ERR_AOK) { if (ldg_str_rbrcpy(asm_text, STACKVIEW_ASM_FAIL_TEXT, sizeof(asm_text)) != LDG_ERR_AOK) { return LDG_ERR_STR_TRUNC; } }

        if (snprintf(line, sizeof(line), STACKVIEW_RING_ASM_FMT, asm_text) < 0) { return LDG_ERR_STR_FMT; }

        ret = stackview_seg_put(render, &pos_col, *row, line, (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (snprintf(line, sizeof(line), STACKVIEW_RING_SP_FMT, run->sp) < 0) { return LDG_ERR_STR_FMT; }

        ret = stackview_seg_put(render, &pos_col, *row, line, (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        *row = *row + (uint32_t)STACKVIEW_STEP;
    }

    return LDG_ERR_AOK;
}

static uint32_t stackview_words_draw(crosstalk_eda_ui_stackview_ctx_t *ctx, ldg_term_render_t *render, uint32_t *row)
{
    char line[STACKVIEW_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0 || row == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < ctx->word_cunt; idx = idx + (uint32_t)STACKVIEW_STEP)
    {
        uint32_t pos_col = STACKVIEW_ORIG_COL;
        uint32_t note_col = CROSSTALK_UI_THEME_DIM_RGBA;

        if (snprintf(line, sizeof(line), STACKVIEW_ADDR_FMT, ctx->sp + (uint64_t)idx * (uint64_t)STACKVIEW_WORD_BYTES) < 0) { return LDG_ERR_STR_FMT; }

        ret = stackview_seg_put(render, &pos_col, *row, line, idx == 0 ? (uint32_t)CROSSTALK_UI_THEME_ACCENT_RGBA : (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (snprintf(line, sizeof(line), STACKVIEW_WORD_FMT, ctx->word[idx]) < 0) { return LDG_ERR_STR_FMT; }

        ret = stackview_seg_put(render, &pos_col, *row, line, ctx->call_mark[idx] == LDG_TRUTH_TRUE ? (uint32_t)CROSSTALK_UI_THEME_ACCENT_RGBA : (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (ctx->call_mark[idx] == LDG_TRUTH_TRUE) { ret = stackview_seg_put(render, &pos_col, *row, STACKVIEW_RET_TEXT, (uint32_t)CROSSTALK_UI_THEME_ACCENT_RGBA); }
        else if (stackview_note_build(ctx, ctx->word[idx], line, (uint32_t)sizeof(line), &note_col) == LDG_ERR_AOK && line[0] != LDG_STR_TERM) { ret = stackview_seg_put(render, &pos_col, *row, line, note_col); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        *row = *row + (uint32_t)STACKVIEW_STEP;
    }

    return LDG_ERR_AOK;
}

static uint32_t stackview_lanes_draw(crosstalk_eda_ui_stackview_ctx_t *ctx, ldg_term_render_t *render, uint32_t *row)
{
    char line[STACKVIEW_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t lane_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0 || row == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (lane_idx = 0; lane_idx < ctx->lane_cunt; lane_idx = lane_idx + (uint32_t)STACKVIEW_STEP)
    {
        const crosstalk_eda_ui_stackview_lane_t *lane = &ctx->lane[lane_idx];
        uint32_t pos_col = STACKVIEW_ORIG_COL;
        uint32_t shown = 0;
        uint32_t show_cunt = lane->cunt;

        if (show_cunt > (uint32_t)CROSSTALK_EDA_UI_STACKVIEW_LANE_SHOW_CAP) { show_cunt = (uint32_t)CROSSTALK_EDA_UI_STACKVIEW_LANE_SHOW_CAP; }

        if (snprintf(line, sizeof(line), STACKVIEW_LANE_HDR_FMT, lane->name, lane->cunt) < 0) { return LDG_ERR_STR_FMT; }

        ret = stackview_seg_put(render, &pos_col, *row, line, (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        *row = *row + (uint32_t)STACKVIEW_STEP;

        for (shown = 0; shown < show_cunt; shown = shown + (uint32_t)STACKVIEW_STEP)
        {
            uint32_t at = (lane->hd + lane->cunt - (uint32_t)STACKVIEW_STEP - shown) % (uint32_t)CROSSTALK_EDA_UI_STACKVIEW_LANE_HIST_CAP;

            pos_col = STACKVIEW_ORIG_COL;

            if (snprintf(line, sizeof(line), STACKVIEW_LANE_ROW_CYCLE_FMT, lane->cycle[at]) < 0) { return LDG_ERR_STR_FMT; }

            ret = stackview_seg_put(render, &pos_col, *row, line, (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (snprintf(line, sizeof(line), STACKVIEW_LANE_ROW_VAL_FMT, lane->val[at]) < 0) { return LDG_ERR_STR_FMT; }

            ret = stackview_seg_put(render, &pos_col, *row, line, (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            *row = *row + (uint32_t)STACKVIEW_STEP;
        }
    }

    return LDG_ERR_AOK;
}

static uint32_t stackview_hdr_draw(crosstalk_eda_ui_stackview_ctx_t *ctx, ldg_term_render_t *render, uint32_t row)
{
    char line[STACKVIEW_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t pos_col = STACKVIEW_ORIG_COL;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = stackview_seg_put(render, &pos_col, row, STACKVIEW_HDR_LIVE_FMT, (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (snprintf(line, sizeof(line), STACKVIEW_ADDR_FMT, ctx->sp) < 0) { return LDG_ERR_STR_FMT; }

    ret = stackview_seg_put(render, &pos_col, row, line, ctx->frozen == LDG_TRUTH_TRUE ? (uint32_t)CROSSTALK_UI_THEME_ACCENT_RGBA : (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (snprintf(line, sizeof(line), STACKVIEW_HDR_TAIL_FMT, ctx->stack_base, ctx->word_cunt, ctx->frozen == LDG_TRUTH_TRUE ? STACKVIEW_HDR_FROZEN_TEXT : STACKVIEW_HDR_RUN_TEXT) < 0) { return LDG_ERR_STR_FMT; }

    return stackview_seg_put(render, &pos_col, row, line, (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA);
}

static uint32_t stackview_draw_cb(uint64_t win_id, void *user, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    crosstalk_eda_ui_stackview_ctx_t *ctx = (crosstalk_eda_ui_stackview_ctx_t *)user;
    char line[STACKVIEW_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t pos_col = STACKVIEW_ORIG_COL;
    uint32_t row = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (win_id != (uint64_t)CROSSTALK_EDA_UI_STACKVIEW_WINDOW_ID || col_cunt == 0 || row_cunt == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    stackview_pump(ctx);

    if (ctx->frozen != LDG_TRUTH_TRUE || ctx->capture_want == LDG_TRUTH_TRUE)
    {
        stackview_refresh(ctx);
        ctx->capture_want = LDG_TRUTH_FALSE;
    }

    if (ctx->fpga == 0x0 || ctx->sp == 0)
    {
        ctx->win.content_row_cunt = (uint32_t)STACKVIEW_STEP;

        return stackview_seg_put(render, &pos_col, row, STACKVIEW_DEAD_TEXT, (uint32_t)CROSSTALK_UI_THEME_DEAD_RGBA);
    }

    ret = stackview_hdr_draw(ctx, render, row);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    row = row + (uint32_t)STACKVIEW_STEP;

    if (ctx->frozen == LDG_TRUTH_TRUE && ctx->fault_has == LDG_TRUTH_TRUE)
    {
        uint32_t frame = 0;

        ret = stackview_frame_draw(render, frame, ctx->fault[CROSSTALK_HDL_ABI_FAULT_LOC_AT], row);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        row = row + (uint32_t)STACKVIEW_STEP;

        for (idx = 0; idx < ctx->word_cunt; idx = idx + (uint32_t)STACKVIEW_STEP)
        {
            if (ctx->call_mark[idx] != LDG_TRUTH_TRUE) { continue; }

            frame = frame + (uint32_t)STACKVIEW_STEP;

            ret = stackview_frame_draw(render, frame, ctx->word[idx], row);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            row = row + (uint32_t)STACKVIEW_STEP;
        }
    }

    if (ctx->word_cunt == 0)
    {
        pos_col = STACKVIEW_ORIG_COL;

        ret = stackview_seg_put(render, &pos_col, row, STACKVIEW_EMPTY_TEXT, (uint32_t)CROSSTALK_UI_THEME_DEAD_RGBA);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        row = row + (uint32_t)STACKVIEW_STEP;
    }

    ret = stackview_ring_draw(ctx, render, &row);
    if (ret == LDG_ERR_AOK) { ret = stackview_words_draw(ctx, render, &row); }

    if (ret == LDG_ERR_AOK) { ret = stackview_lanes_draw(ctx, render, &row); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->lane_cunt == 0)
    {
        pos_col = STACKVIEW_ORIG_COL;

        ret = stackview_seg_put(render, &pos_col, row, STACKVIEW_HINT_TEXT, (uint32_t)CROSSTALK_UI_THEME_DEAD_RGBA);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        row = row + (uint32_t)STACKVIEW_STEP;
    }

    if (snprintf(line, sizeof(line), STACKVIEW_PROMPT_FMT, ctx->query) < 0) { return LDG_ERR_STR_FMT; }

    pos_col = STACKVIEW_ORIG_COL;

    ret = stackview_seg_put(render, &pos_col, row, line, (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->win.content_row_cunt = row + (uint32_t)STACKVIEW_STEP;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_stackview_win_reg(crosstalk_eda_ui_stackview_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot)
{
    if (LDG_UNLIKELY(ctx == 0x0 || win_nav == 0x0 || out_slot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_ui_window_reg(win_nav, (uint64_t)CROSSTALK_EDA_UI_STACKVIEW_WINDOW_ID, CROSSTALK_EDA_UI_STACKVIEW_WINDOW_TITLE, &ctx->win, stackview_draw_cb, stackview_key_isr, ctx, out_slot);
}
