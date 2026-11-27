#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/secure.h>
#include <dangling/str/fmt.h>
#include <dangling/str/str.h>
#include <dangling/term/key.h>

#include "re/repl/native.h"
#include "re/repl/repl.h"
#include "re/repl/sesh.h"

#define SESH_STEP 1
#define SESH_PROMPT "repl> "
#define SESH_EOL "\r\n"
#define SESH_CANCEL_CODEPOINT 'c'
#define SESH_CANCEL_CODEPOINT_UPPER 'C'
#define SESH_CANCEL_CTRL_CODE 0x03
#define SESH_VERB_HELP ""
#define SESH_PAGE_CLEAR "\x1B[H\x1B[2J\x1B[3J"
#define SESH_LN_CLEAR "\r\x1B[2K"
#define SESH_ECHO_FMT "%.15s%.511s"
#define SESH_WIPE "wipe"

static uint32_t sesh_out(crosstalk_re_repl_sesh_t *sesh, const char *text, uint32_t len)
{
    if (sesh->out_cb == 0x0 || len == 0) { return LDG_ERR_AOK; }

    return sesh->out_cb(sesh->out_user, (const uint8_t *)text, len);
}

static uint32_t sesh_eol_put(crosstalk_re_repl_sesh_t *sesh)
{
    return sesh_out(sesh, SESH_EOL, (uint32_t)sizeof(SESH_EOL) - LDG_STR_TERM_SIZE);
}

static uint32_t sesh_ring_push(crosstalk_re_repl_sesh_t *sesh, const uint8_t *bytes, uint32_t len)
{
    char text[CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t take = len;

    if (take == 0) { return LDG_ERR_AOK; }

    if (take > (uint32_t)sizeof(text) - LDG_STR_TERM_SIZE) { take = (uint32_t)sizeof(text) - LDG_STR_TERM_SIZE; }

    memcpy(text, bytes, (uint64_t)take);
    text[take] = LDG_STR_TERM;

    return crosstalk_core_cmdline_out_push(&sesh->input, text);
}

static uint32_t sesh_repl_sink_cb(void *user, const uint8_t *bytes, uint32_t len)
{
    crosstalk_re_repl_sesh_t *sesh = (crosstalk_re_repl_sesh_t *)user;
    uint32_t start = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(sesh == 0x0 || bytes == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_re_repl_complete_addr_capture(&sesh->complete, (const char *)bytes, len);
    if (ret != LDG_ERR_AOK) { return ret; }

    for (at = 0; at < len; at = at + (uint32_t)SESH_STEP)
    {
        if (bytes[at] != (uint8_t)LDG_ASCII_LF) { continue; }

        ret = sesh_ring_push(sesh, bytes + start, at - start);
        if (ret != LDG_ERR_AOK) { return ret; }

        start = at + (uint32_t)SESH_STEP;
    }

    return sesh_ring_push(sesh, bytes + start, len - start);
}

static uint32_t sesh_line_draw(crosstalk_re_repl_sesh_t *sesh)
{
    uint64_t len = 0;
    uint32_t ret = 0;

    if (sesh->input_ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ret = crosstalk_core_cmdline_encode(&sesh->input, sesh->col_cunt != 0 ? sesh->col_cunt : (uint32_t)CROSSTALK_RE_REPL_SESH_COL_CUNT, sesh->enc, (uint64_t)sizeof(sesh->enc), &len);
    if (ret != LDG_ERR_AOK) { return ret; }

    return sesh_out(sesh, (const char *)sesh->enc, (uint32_t)len);
}

static uint32_t sesh_render(crosstalk_re_repl_sesh_t *sesh)
{
    char text[CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t seq = 0;
    uint64_t at = 0;
    uint64_t got = 0;
    uint64_t len = 0;
    uint32_t ret = 0;

    ret = crosstalk_core_cmdline_out_seq_get(&sesh->input, &seq);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (sesh->page_due == LDG_TRUTH_TRUE || sesh->out_seen > seq)
    {
        ret = sesh_out(sesh, SESH_PAGE_CLEAR, (uint32_t)sizeof(SESH_PAGE_CLEAR) - LDG_STR_TERM_SIZE);
        if (ret != LDG_ERR_AOK) { return ret; }

        if (seq > (uint64_t)LDG_SHELL_OUT_LN_MAX_DEFAULT) { at = seq - (uint64_t)LDG_SHELL_OUT_LN_MAX_DEFAULT; }

        sesh->page_due = LDG_TRUTH_FALSE;
    }
    else
    {
        ret = sesh_out(sesh, SESH_LN_CLEAR, (uint32_t)sizeof(SESH_LN_CLEAR) - LDG_STR_TERM_SIZE);
        if (ret != LDG_ERR_AOK) { return ret; }

        at = sesh->out_seen;
    }

    sesh->out_seen = seq;

    if (at < sesh->input.out_floor) { at = sesh->input.out_floor; }

    for (; at < seq; at++)
    {
        if (crosstalk_core_cmdline_out_get(&sesh->input, at, text, (uint32_t)sizeof(text), &got) != LDG_ERR_AOK || got != at + (uint64_t)SESH_STEP) { continue; }

        ret = crosstalk_core_cmdline_ln_trim(text, (uint64_t)sizeof(text), &len);
        if (ret != LDG_ERR_AOK) { return ret; }

        ret = sesh_out(sesh, text, (uint32_t)len);
        if (ret != LDG_ERR_AOK) { return ret; }

        ret = sesh_eol_put(sesh);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    if (sesh->prompt_due == LDG_TRUTH_TRUE || (sesh->repl != 0x0 && crosstalk_re_repl_busy_is(sesh->repl) == LDG_TRUTH_TRUE)) { return LDG_ERR_AOK; }

    return sesh_line_draw(sesh);
}

uint32_t crosstalk_re_repl_sesh_wipe(crosstalk_re_repl_sesh_t *sesh)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(sesh == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (sesh->input_ok != LDG_TRUTH_TRUE || sesh->input.shell == 0x0) { return LDG_ERR_NOT_INIT; }

    ret = ldg_shell_ln_exec(sesh->input.shell, SESH_WIPE);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_core_cmdline_wipe_ck(&sesh->input);
    if (ret != LDG_ERR_AOK) { return ret; }

    sesh->page_due = LDG_TRUTH_TRUE;

    return crosstalk_re_repl_sesh_prompt_put(sesh);
}

uint32_t crosstalk_re_repl_sesh_prompt_put(crosstalk_re_repl_sesh_t *sesh)
{
    if (LDG_UNLIKELY(sesh == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    sesh->prompt_due = LDG_TRUTH_FALSE;

    if (crosstalk_core_cmdline_open_is(&sesh->input) != LDG_TRUTH_TRUE && crosstalk_core_cmdline_open(&sesh->input) != LDG_ERR_AOK) { return LDG_ERR_NOT_INIT; }

    return sesh_render(sesh);
}

uint32_t crosstalk_re_repl_sesh_col_cunt_set(crosstalk_re_repl_sesh_t *sesh, uint32_t col_cunt)
{
    if (LDG_UNLIKELY(sesh == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    sesh->col_cunt = col_cunt;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_repl_sesh_verb_reg(crosstalk_re_repl_sesh_t *sesh, const char *name, const char *help)
{
    if (LDG_UNLIKELY(sesh == 0x0 || name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (sesh->input_ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    return crosstalk_core_cmdline_verb_reg(&sesh->input, name, help != 0x0 ? help : SESH_VERB_HELP, (uint32_t)LDG_SHELL_ARG_MAX);
}

uint32_t crosstalk_re_repl_sesh_open(crosstalk_re_repl_sesh_t *sesh, crosstalk_re_repl_ctx_t *repl, crosstalk_re_repl_subst_t *subst, crosstalk_re_repl_sink_cb_t out_cb, void *out_user)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(sesh == 0x0 || repl == 0x0 || subst == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (sesh->input_ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ret = crosstalk_core_cmdline_open(&sesh->input);
    if (ret != LDG_ERR_AOK) { return ret; }

    sesh->prompt_due = LDG_TRUTH_FALSE;
    sesh->page_due = LDG_TRUTH_TRUE;
    sesh->repl = repl;
    sesh->subst = subst;
    sesh->out_cb = out_cb;
    sesh->out_user = out_user;
    sesh->live = LDG_TRUTH_TRUE;

    return crosstalk_re_repl_sink_set(repl, sesh_repl_sink_cb, sesh);
}

uint32_t crosstalk_re_repl_sesh_exec_set(crosstalk_re_repl_sesh_t *sesh, crosstalk_re_repl_sesh_exec_cb_t cb, void *user)
{
    if (LDG_UNLIKELY(sesh == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    sesh->exec_cb = cb;
    sesh->exec_user = user;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_repl_sesh_init(crosstalk_re_repl_sesh_t *sesh)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(sesh == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(sesh, (uint64_t)sizeof(*sesh)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    ret = crosstalk_re_repl_complete_init(&sesh->complete);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_core_cmdline_drawer_init(&sesh->input);
    if (ret != LDG_ERR_AOK) { return ret; }

    sesh->input_ok = LDG_TRUTH_TRUE;

    ret = crosstalk_core_cmdline_prompt_set(&sesh->input, SESH_PROMPT);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_core_cmdline_echo_set(&sesh->input, LDG_TRUTH_FALSE); }

    return ret;
}

uint32_t crosstalk_re_repl_sesh_shutdown(crosstalk_re_repl_sesh_t *sesh)
{
    if (LDG_UNLIKELY(sesh == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (sesh->input_ok != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    sesh->input_ok = LDG_TRUTH_FALSE;

    return crosstalk_core_cmdline_drawer_shutdown(&sesh->input);
}

uint32_t crosstalk_re_repl_sesh_hist_ld(crosstalk_re_repl_sesh_t *sesh, crosstalk_re_repl_ctx_t *repl, crosstalk_re_repl_subst_t *subst, const char *path)
{
    if (LDG_UNLIKELY(sesh == 0x0 || repl == 0x0 || subst == 0x0 || path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (sesh->input_ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    return crosstalk_core_cmdline_hist_ld(&sesh->input, path);
}

uint32_t crosstalk_re_repl_sesh_close(crosstalk_re_repl_sesh_t *sesh)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(sesh == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (sesh->repl != 0x0) { ret = crosstalk_re_repl_sink_set(sesh->repl, 0x0, 0x0); }

    sesh->out_cb = 0x0;
    sesh->out_user = 0x0;
    sesh->live = LDG_TRUTH_FALSE;

    if (sesh->input_ok == LDG_TRUTH_TRUE && crosstalk_core_cmdline_close(&sesh->input) != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = LDG_ERR_INVALID; }

    return ret;
}

uint8_t crosstalk_re_repl_sesh_live_is(const crosstalk_re_repl_sesh_t *sesh)
{
    if (LDG_UNLIKELY(sesh == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return sesh->live == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t sesh_prompt_or_wait(crosstalk_re_repl_sesh_t *sesh)
{
    if (crosstalk_re_repl_busy_is(sesh->repl) == LDG_TRUTH_TRUE)
    {
        sesh->prompt_due = LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    return crosstalk_re_repl_sesh_prompt_put(sesh);
}

static uint32_t sesh_line_exec(crosstalk_re_repl_sesh_t *sesh)
{
    uint32_t ret = 0;
    uint8_t handled = LDG_TRUTH_FALSE;

    if (sesh->exec_cb != 0x0)
    {
        if (sesh->exec_cb(sesh->exec_user, sesh->line) == LDG_ERR_IO_WR) { return LDG_ERR_IO_WR; }

        return sesh_prompt_or_wait(sesh);
    }

    ret = crosstalk_re_repl_subst_apply(sesh->subst, sesh->repl, sesh->line, (uint32_t)sizeof(sesh->line), &handled);
    if (ret != LDG_ERR_AOK && crosstalk_re_repl_emit(sesh->repl, "err: %u32", ret) != LDG_ERR_AOK) { return LDG_ERR_IO_WR; }

    if (ret != LDG_ERR_AOK || handled == LDG_TRUTH_TRUE) { return crosstalk_re_repl_sesh_prompt_put(sesh); }

    if (crosstalk_re_repl_line_run(sesh->repl, sesh->line) == LDG_ERR_IO_WR) { return LDG_ERR_IO_WR; }

    return sesh_prompt_or_wait(sesh);
}

static uint32_t sesh_cancel(crosstalk_re_repl_sesh_t *sesh)
{
    uint32_t ret = 0;

    ret = sesh_eol_put(sesh);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (crosstalk_re_repl_busy_is(sesh->repl) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_re_repl_cancel(sesh->repl);
        if (ret != LDG_ERR_AOK) { return ret; }

        ret = crosstalk_re_repl_emit(sesh->repl, "search cancelled");
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    ret = crosstalk_core_cmdline_open(&sesh->input);
    if (ret != LDG_ERR_AOK) { return ret; }

    return crosstalk_re_repl_sesh_prompt_put(sesh);
}

static uint8_t sesh_cancel_is(const ldg_term_key_event_t *event)
{
    if (event->codepoint == (uint32_t)SESH_CANCEL_CTRL_CODE) { return LDG_TRUTH_TRUE; }

    if ((event->mods & (uint32_t)LDG_TERM_MOD_CTRL) == 0) { return LDG_TRUTH_FALSE; }

    return (event->codepoint == (uint32_t)SESH_CANCEL_CODEPOINT || event->codepoint == (uint32_t)SESH_CANCEL_CODEPOINT_UPPER) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t sesh_key_put(crosstalk_re_repl_sesh_t *sesh, const ldg_term_key_event_t *event, uint8_t *out_redraw)
{
    uint32_t ret = 0;
    uint8_t consumed = LDG_TRUTH_FALSE;

    *out_redraw = LDG_TRUTH_FALSE;

    if (event->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE) { return LDG_ERR_AOK; }

    if (sesh_cancel_is(event) == LDG_TRUTH_TRUE) { return sesh_cancel(sesh); }

    if (crosstalk_re_repl_busy_is(sesh->repl) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = crosstalk_core_cmdline_key_isr(&sesh->input, event, &consumed);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (crosstalk_core_cmdline_line_take(&sesh->input, sesh->line, (uint32_t)sizeof(sesh->line)) == LDG_ERR_AOK)
    {
        char echo[CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CAP] = LDG_ARR_ZERO_INIT;
        uint64_t echo_len = 0;

        ret = ldg_str_fmt(echo, (uint64_t)sizeof(echo), &echo_len, SESH_ECHO_FMT, SESH_PROMPT, sesh->line);
        if (ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC) { return ret; }

        ret = crosstalk_core_cmdline_out_push(&sesh->input, echo);
        if (ret != LDG_ERR_AOK) { return ret; }

        ret = crosstalk_core_cmdline_open(&sesh->input);
        if (ret != LDG_ERR_AOK) { return ret; }

        return sesh_line_exec(sesh);
    }

    if (crosstalk_core_cmdline_open_is(&sesh->input) != LDG_TRUTH_TRUE)
    {
        ret = sesh_eol_put(sesh);
        if (ret != LDG_ERR_AOK) { return ret; }

        return crosstalk_re_repl_sesh_prompt_put(sesh);
    }

    *out_redraw = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_repl_sesh_bytes_put(crosstalk_re_repl_sesh_t *sesh, const uint8_t *bytes, uint32_t len)
{
    ldg_term_key_event_t event = LDG_STRUCT_ZERO_INIT;
    uint64_t used = 0;
    uint32_t at = 0;
    uint32_t ret = LDG_ERR_AOK;
    uint8_t redraw = LDG_TRUTH_FALSE;
    uint8_t redraw_due = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(sesh == 0x0 || bytes == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(sesh->live != LDG_TRUTH_TRUE || sesh->repl == 0x0 || sesh->input_ok != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    while (at < len && ret == LDG_ERR_AOK)
    {
        used = 0;

        if (ldg_term_key_decode(bytes + at, (uint64_t)(len - at), LDG_TRUTH_TRUE, &event, &used) != LDG_ERR_AOK || used == 0)
        {
            at = at + (uint32_t)SESH_STEP;

            continue;
        }

        at = at + (uint32_t)used;

        ret = sesh_key_put(sesh, &event, &redraw);
        if (redraw == LDG_TRUTH_TRUE) { redraw_due = LDG_TRUTH_TRUE; }
    }

    if (ret != LDG_ERR_AOK || redraw_due != LDG_TRUTH_TRUE || sesh->live != LDG_TRUTH_TRUE) { return ret; }

    if (crosstalk_re_repl_busy_is(sesh->repl) == LDG_TRUTH_TRUE || sesh->prompt_due == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    return sesh_line_draw(sesh);
}

uint32_t crosstalk_re_repl_sesh_tick(crosstalk_re_repl_sesh_t *sesh)
{
    uint64_t seq = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(sesh == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (sesh->live != LDG_TRUTH_TRUE || sesh->repl == 0x0) { return LDG_ERR_AOK; }

    ret = crosstalk_re_repl_tick(sesh->repl);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (sesh->prompt_due == LDG_TRUTH_TRUE && crosstalk_re_repl_busy_is(sesh->repl) != LDG_TRUTH_TRUE) { return crosstalk_re_repl_sesh_prompt_put(sesh); }

    ret = crosstalk_core_cmdline_out_seq_get(&sesh->input, &seq);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (seq == sesh->out_seen) { return LDG_ERR_AOK; }

    return sesh_render(sesh);
}
