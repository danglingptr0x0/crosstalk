#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/file.h>
#include <dangling/io/path.h>
#include <dangling/mem/secure.h>
#include <dangling/str/str.h>

#include "re/repl/native.h"
#include "re/repl/repl.h"
#include "re/repl/sesh.h"

#define SESH_STEP 1
#define SESH_PROMPT "repl> "
#define SESH_EOL "\r\n"
#define SESH_ERASE "\b \b"
#define SESH_LINE_CLEAR "\r\x1B[K"
#define SESH_FILE_ACCESS 0x180
#define SESH_FILE_KIND_CMD 'C'
#define SESH_FILE_KIND_ALIAS 'A'
#define SESH_FILE_LEAD_LEN 2
#define SESH_FILE_KIND_AT 0
#define SESH_FILE_SEP_AT 1
#define SESH_CH_EQ '='
#define SESH_CSI_UP 'A'
#define SESH_CSI_DOWN 'B'
#define SESH_CH_CANCEL 0x03
#define SESH_CH_BS 0x08
#define SESH_CH_DEL 0x7F
#define SESH_CH_CSI '['
#define SESH_CSI_FINAL_MIN 0x40
#define SESH_CSI_FINAL_MAX 0x7E

typedef enum sesh_esc
{
    SESH_ESC_NONE = 0,
    SESH_ESC_LEAD,
    SESH_ESC_CSI
} sesh_esc_t;

static uint32_t sesh_out(crosstalk_re_repl_sesh_t *sesh, const char *text, uint32_t len)
{
    if (sesh->out_cb == 0x0 || len == 0) { return LDG_ERR_AOK; }

    return sesh->out_cb(sesh->out_user, (const uint8_t *)text, len);
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

        ret = sesh_out(sesh, (const char *)bytes + start, at - start);
        if (ret != LDG_ERR_AOK) { return ret; }

        ret = sesh_out(sesh, SESH_EOL, (uint32_t)sizeof(SESH_EOL) - LDG_STR_TERM_SIZE);
        if (ret != LDG_ERR_AOK) { return ret; }

        start = at + (uint32_t)SESH_STEP;
    }

    return sesh_out(sesh, (const char *)bytes + start, len - start);
}

uint32_t crosstalk_re_repl_sesh_prompt_put(crosstalk_re_repl_sesh_t *sesh)
{
    if (LDG_UNLIKELY(sesh == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    sesh->prompt_due = LDG_TRUTH_FALSE;

    return sesh_out(sesh, SESH_PROMPT, (uint32_t)sizeof(SESH_PROMPT) - LDG_STR_TERM_SIZE);
}

uint32_t crosstalk_re_repl_sesh_open(crosstalk_re_repl_sesh_t *sesh, crosstalk_re_repl_ctx_t *repl, crosstalk_re_repl_subst_t *subst, crosstalk_re_repl_sink_cb_t out_cb, void *out_user)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(sesh == 0x0 || repl == 0x0 || subst == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    sesh->len = 0;
    sesh->hist_sel = sesh->hist_cunt;
    sesh->prompt_due = LDG_TRUTH_FALSE;
    sesh->repl = repl;
    sesh->subst = subst;
    sesh->out_cb = out_cb;
    sesh->out_user = out_user;
    sesh->esc_state = (uint8_t)SESH_ESC_NONE;
    sesh->live = LDG_TRUTH_TRUE;

    ret = crosstalk_re_repl_sink_set(repl, sesh_repl_sink_cb, sesh);

    return ret;
}

uint32_t crosstalk_re_repl_sesh_init(crosstalk_re_repl_sesh_t *sesh)
{
    if (LDG_UNLIKELY(sesh == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(sesh, (uint64_t)sizeof(*sesh)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    return crosstalk_re_repl_complete_init(&sesh->complete);
}

static uint32_t sesh_line_draw(crosstalk_re_repl_sesh_t *sesh)
{
    uint32_t ret = 0;

    ret = sesh_out(sesh, SESH_LINE_CLEAR, (uint32_t)sizeof(SESH_LINE_CLEAR) - LDG_STR_TERM_SIZE);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = sesh_out(sesh, SESH_PROMPT, (uint32_t)sizeof(SESH_PROMPT) - LDG_STR_TERM_SIZE);
    if (ret != LDG_ERR_AOK) { return ret; }

    return sesh_out(sesh, sesh->line, sesh->len);
}

static uint32_t sesh_line_ld(crosstalk_re_repl_sesh_t *sesh, const char *text)
{
    uint64_t len = 0;
    uint32_t ret = 0;

    ret = ldg_str_rbrcpy(sesh->line, text, (uint64_t)sizeof(sesh->line));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_str_len_get(sesh->line, (uint64_t)sizeof(sesh->line), &len);
    if (ret != LDG_ERR_AOK) { return ret; }

    sesh->len = (uint32_t)len;

    return sesh_line_draw(sesh);
}

static uint32_t sesh_hist_add(crosstalk_re_repl_sesh_t *sesh)
{
    uint32_t at = 0;

    if (sesh->line[0] == LDG_STR_TERM) { return LDG_ERR_AOK; }

    if (sesh->hist_cunt != 0 && crosstalk_re_repl_cmd_eq_is(sesh->hist[sesh->hist_cunt - (uint32_t)SESH_STEP].text, sesh->line) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (sesh->hist_cunt == (uint32_t)CROSSTALK_RE_REPL_SESH_HIST_CAP)
    {
        for (at = 0; at + (uint32_t)SESH_STEP < sesh->hist_cunt; at = at + (uint32_t)SESH_STEP) { sesh->hist[at] = sesh->hist[at + (uint32_t)SESH_STEP]; }

        sesh->hist_cunt = sesh->hist_cunt - (uint32_t)SESH_STEP;
    }

    sesh->hist_cunt = sesh->hist_cunt + (uint32_t)SESH_STEP;

    return ldg_str_rbrcpy(sesh->hist[sesh->hist_cunt - (uint32_t)SESH_STEP].text, sesh->line, (uint64_t)sizeof(sesh->hist[0].text));
}

static uint32_t sesh_hist_mov(crosstalk_re_repl_sesh_t *sesh, uint8_t older)
{
    uint32_t ret = 0;

    if (older == LDG_TRUTH_TRUE)
    {
        if (sesh->hist_sel == 0) { return LDG_ERR_AOK; }

        if (sesh->hist_sel == sesh->hist_cunt)
        {
            sesh->line[sesh->len] = LDG_STR_TERM;

            ret = ldg_str_rbrcpy(sesh->draft, sesh->line, (uint64_t)sizeof(sesh->draft));
            if (ret != LDG_ERR_AOK) { return ret; }
        }

        sesh->hist_sel = sesh->hist_sel - (uint32_t)SESH_STEP;

        return sesh_line_ld(sesh, sesh->hist[sesh->hist_sel].text);
    }

    if (sesh->hist_sel >= sesh->hist_cunt) { return LDG_ERR_AOK; }

    sesh->hist_sel = sesh->hist_sel + (uint32_t)SESH_STEP;

    return sesh_line_ld(sesh, sesh->hist_sel == sesh->hist_cunt ? sesh->draft : sesh->hist[sesh->hist_sel].text);
}

static uint8_t* sesh_file_buff_get(crosstalk_re_repl_sesh_t *sesh)
{
    uintptr_t at = (uintptr_t)sesh->file_buff + (uintptr_t)CROSSTALK_RE_REPL_SESH_FILE_BLK - (uintptr_t)SESH_STEP;

    return (uint8_t *)(at & ~((uintptr_t)CROSSTALK_RE_REPL_SESH_FILE_BLK - (uintptr_t)SESH_STEP));
}

static uint32_t sesh_file_ch_put(uint8_t *buff, uint32_t *pos, char ch)
{
    if (*pos >= (uint32_t)CROSSTALK_RE_REPL_SESH_FILE_CAP) { return LDG_ERR_FULL; }

    buff[*pos] = (uint8_t)ch;
    *pos = *pos + (uint32_t)SESH_STEP;

    return LDG_ERR_AOK;
}

static uint32_t sesh_file_text_put(uint8_t *buff, uint32_t *pos, const char *text)
{
    uint64_t len = 0;
    uint32_t ret = 0;

    ret = ldg_str_len_get(text, (uint64_t)CROSSTALK_RE_REPL_CMD_CAP, &len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (len > (uint64_t)((uint32_t)CROSSTALK_RE_REPL_SESH_FILE_CAP - *pos)) { return LDG_ERR_FULL; }

    if (len == 0) { return LDG_ERR_AOK; }

    ret = ldg_mem_secure_cpy(buff + *pos, text, len);
    if (ret != LDG_ERR_AOK) { return ret; }

    *pos = *pos + (uint32_t)len;

    return LDG_ERR_AOK;
}

static uint32_t sesh_file_line_put(uint8_t *buff, uint32_t *pos, char kind, const char *name, const char *text)
{
    uint32_t ret = 0;

    ret = sesh_file_ch_put(buff, pos, kind);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = sesh_file_ch_put(buff, pos, LDG_ASCII_TAB);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (name != 0x0)
    {
        ret = sesh_file_text_put(buff, pos, name);
        if (ret != LDG_ERR_AOK) { return ret; }

        ret = sesh_file_ch_put(buff, pos, SESH_CH_EQ);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    ret = sesh_file_text_put(buff, pos, text);
    if (ret != LDG_ERR_AOK) { return ret; }

    return sesh_file_ch_put(buff, pos, LDG_ASCII_LF);
}

static uint32_t sesh_hist_encode(crosstalk_re_repl_sesh_t *sesh, uint8_t *buff, uint32_t *out_len)
{
    uint32_t pos = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    *out_len = 0;

    ret = ldg_mem_secure_zero(buff, (uint64_t)CROSSTALK_RE_REPL_SESH_FILE_CAP);
    if (ret != LDG_ERR_AOK) { return ret; }

    for (at = 0; sesh->subst != 0x0 && at < (uint32_t)CROSSTALK_RE_REPL_SUBST_CAP; at = at + (uint32_t)SESH_STEP)
    {
        if (sesh->subst->alias[at].name[0] == LDG_STR_TERM) { continue; }

        ret = sesh_file_line_put(buff, &pos, SESH_FILE_KIND_ALIAS, sesh->subst->alias[at].name, sesh->subst->alias[at].text);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    for (at = 0; at < sesh->hist_cunt; at = at + (uint32_t)SESH_STEP)
    {
        ret = sesh_file_line_put(buff, &pos, SESH_FILE_KIND_CMD, 0x0, sesh->hist[at].text);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    *out_len = pos;

    return LDG_ERR_AOK;
}

static uint32_t sesh_hist_st(crosstalk_re_repl_sesh_t *sesh)
{
    ldg_io_file_t *file = 0x0;
    uint8_t *buff = sesh_file_buff_get(sesh);
    uint64_t padded = 0;
    uint64_t done = 0;
    uint32_t len = 0;
    uint32_t ret = 0;

    if (sesh->hist_path[0] == LDG_STR_TERM) { return LDG_ERR_AOK; }

    ret = sesh_hist_encode(sesh, buff, &len);
    if (ret != LDG_ERR_AOK) { return ret; }

    padded = ((uint64_t)len + (uint64_t)CROSSTALK_RE_REPL_SESH_FILE_BLK - (uint64_t)SESH_STEP) & ~((uint64_t)CROSSTALK_RE_REPL_SESH_FILE_BLK - (uint64_t)SESH_STEP);

    ret = ldg_io_file_open(sesh->hist_path, LDG_IO_WR_ONLY | LDG_IO_CREATE | LDG_IO_TRUNCATE | LDG_IO_DIRECT, (uint32_t)SESH_FILE_ACCESS, &file);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (padded != 0)
    {
        ret = ldg_io_file_wr(file, buff, padded, &done);
        if (ret == LDG_ERR_AOK && done != padded) { ret = LDG_ERR_IO_WR; }
    }

    if (ret == LDG_ERR_AOK) { ret = ldg_io_file_truncate(file, (uint64_t)len); }

    if (ret == LDG_ERR_AOK) { ret = ldg_io_file_commit(file); }

    if (ldg_io_file_close(file) != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    return ret;
}

static uint32_t sesh_file_line_ld(crosstalk_re_repl_sesh_t *sesh, crosstalk_re_repl_ctx_t *repl, crosstalk_re_repl_subst_t *subst, const uint8_t *text, uint32_t len)
{
    uint32_t at = 0;
    uint8_t handled = LDG_TRUTH_FALSE;

    if (len <= (uint32_t)SESH_FILE_LEAD_LEN || len - (uint32_t)SESH_FILE_LEAD_LEN >= (uint32_t)sizeof(sesh->line) || text[SESH_FILE_SEP_AT] != (uint8_t)LDG_ASCII_TAB) { return LDG_ERR_AOK; }

    for (at = (uint32_t)SESH_FILE_LEAD_LEN; at < len; at = at + (uint32_t)SESH_STEP)
    {
        if (text[at] < (uint8_t)LDG_ASCII_PRINTABLE_MIN || text[at] > (uint8_t)LDG_ASCII_PRINTABLE_MAX) { return LDG_ERR_AOK; }

        sesh->line[at - (uint32_t)SESH_FILE_LEAD_LEN] = (char)text[at];
    }

    sesh->line[len - (uint32_t)SESH_FILE_LEAD_LEN] = LDG_STR_TERM;

    if (text[SESH_FILE_KIND_AT] == (uint8_t)SESH_FILE_KIND_CMD) { return sesh_hist_add(sesh); }

    if (text[SESH_FILE_KIND_AT] != (uint8_t)SESH_FILE_KIND_ALIAS) { return LDG_ERR_AOK; }

    if (crosstalk_re_repl_subst_apply(subst, repl, sesh->line, (uint32_t)sizeof(sesh->line), &handled) != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_repl_sesh_hist_ld(crosstalk_re_repl_sesh_t *sesh, crosstalk_re_repl_ctx_t *repl, crosstalk_re_repl_subst_t *subst, const char *path)
{
    ldg_io_file_t *file = 0x0;
    uint8_t *buff = 0x0;
    uint64_t got = 0;
    uint32_t start = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(sesh == 0x0 || repl == 0x0 || subst == 0x0 || path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_str_rbrcpy(sesh->hist_path, path, (uint64_t)sizeof(sesh->hist_path));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ldg_io_path_exists_is(path) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    buff = sesh_file_buff_get(sesh);

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY | LDG_IO_DIRECT, (uint32_t)SESH_FILE_ACCESS, &file);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_rd(file, buff, (uint64_t)CROSSTALK_RE_REPL_SESH_FILE_CAP, &got);

    if (ldg_io_file_close(file) != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    if (ret != LDG_ERR_AOK) { return ret; }

    for (at = 0; at < (uint32_t)got; at = at + (uint32_t)SESH_STEP)
    {
        if (buff[at] != (uint8_t)LDG_ASCII_LF) { continue; }

        ret = sesh_file_line_ld(sesh, repl, subst, buff + start, at - start);
        if (ret != LDG_ERR_AOK) { return ret; }

        start = at + (uint32_t)SESH_STEP;
    }

    sesh->line[0] = LDG_STR_TERM;
    sesh->len = 0;
    sesh->hist_sel = sesh->hist_cunt;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_repl_sesh_close(crosstalk_re_repl_sesh_t *sesh)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(sesh == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (sesh->repl != 0x0) { ret = crosstalk_re_repl_sink_set(sesh->repl, 0x0, 0x0); }

    sesh->out_cb = 0x0;
    sesh->out_user = 0x0;
    sesh->len = 0;
    sesh->live = LDG_TRUTH_FALSE;

    return ret;
}

uint8_t crosstalk_re_repl_sesh_live_is(const crosstalk_re_repl_sesh_t *sesh)
{
    if (LDG_UNLIKELY(sesh == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return sesh->live == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t sesh_line_exec(crosstalk_re_repl_sesh_t *sesh)
{
    uint32_t ret = 0;
    uint8_t handled = LDG_TRUTH_FALSE;

    ret = sesh_out(sesh, SESH_EOL, (uint32_t)sizeof(SESH_EOL) - LDG_STR_TERM_SIZE);
    if (ret != LDG_ERR_AOK) { return ret; }

    sesh->line[sesh->len] = LDG_STR_TERM;
    sesh->len = 0;

    ret = sesh_hist_add(sesh);
    if (ret != LDG_ERR_AOK) { return ret; }

    sesh->hist_sel = sesh->hist_cunt;

    ret = crosstalk_re_repl_subst_apply(sesh->subst, sesh->repl, sesh->line, (uint32_t)sizeof(sesh->line), &handled);
    if (ret != LDG_ERR_AOK && crosstalk_re_repl_emit(sesh->repl, "err: %u32", ret) != LDG_ERR_AOK) { return LDG_ERR_IO_WR; }

    if (ret != LDG_ERR_AOK || handled == LDG_TRUTH_TRUE) { return crosstalk_re_repl_sesh_prompt_put(sesh); }

    if (crosstalk_re_repl_line_run(sesh->repl, sesh->line) == LDG_ERR_IO_WR) { return LDG_ERR_IO_WR; }

    if (crosstalk_re_repl_busy_is(sesh->repl) == LDG_TRUTH_TRUE)
    {
        sesh->prompt_due = LDG_TRUTH_TRUE;
        return LDG_ERR_AOK;
    }

    return crosstalk_re_repl_sesh_prompt_put(sesh);
}

static uint32_t sesh_line_run(crosstalk_re_repl_sesh_t *sesh)
{
    uint32_t ret = 0;

    ret = sesh_line_exec(sesh);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = sesh_hist_st(sesh);
    if (ret == LDG_ERR_AOK) { return LDG_ERR_AOK; }

    sesh->hist_path[0] = LDG_STR_TERM;

    return crosstalk_re_repl_emit(sesh->repl, "history write failed; err: %u32", ret);
}

static uint32_t sesh_cancel(crosstalk_re_repl_sesh_t *sesh)
{
    uint32_t ret = 0;

    sesh->len = 0;

    ret = sesh_out(sesh, SESH_EOL, (uint32_t)sizeof(SESH_EOL) - LDG_STR_TERM_SIZE);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (crosstalk_re_repl_busy_is(sesh->repl) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_re_repl_cancel(sesh->repl);
        if (ret != LDG_ERR_AOK) { return ret; }

        ret = crosstalk_re_repl_emit(sesh->repl, "search cancelled");
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return crosstalk_re_repl_sesh_prompt_put(sesh);
}

static uint8_t sesh_esc_take_is(crosstalk_re_repl_sesh_t *sesh, uint8_t val)
{
    if (sesh->esc_state == (uint8_t)SESH_ESC_NONE)
    {
        if (val != (uint8_t)LDG_ASCII_ESC) { return LDG_TRUTH_FALSE; }

        sesh->esc_state = (uint8_t)SESH_ESC_LEAD;

        return LDG_TRUTH_TRUE;
    }

    if (sesh->esc_state == (uint8_t)SESH_ESC_LEAD)
    {
        sesh->esc_state = val == (uint8_t)SESH_CH_CSI ? (uint8_t)SESH_ESC_CSI : (uint8_t)SESH_ESC_NONE;

        return LDG_TRUTH_TRUE;
    }

    if (val >= (uint8_t)SESH_CSI_FINAL_MIN && val <= (uint8_t)SESH_CSI_FINAL_MAX) { sesh->esc_state = (uint8_t)SESH_ESC_NONE; }

    return LDG_TRUTH_TRUE;
}

static uint32_t sesh_cand_list(crosstalk_re_repl_sesh_t *sesh)
{
    uint32_t at = 0;
    uint32_t ret = 0;

    ret = sesh_out(sesh, SESH_EOL, (uint32_t)sizeof(SESH_EOL) - LDG_STR_TERM_SIZE);

    for (at = 0; ret == LDG_ERR_AOK && at < sesh->complete.cunt && at < (uint32_t)CROSSTALK_RE_REPL_COMPLETE_ROWS; at = at + (uint32_t)SESH_STEP) { ret = crosstalk_re_repl_emit(sesh->repl, "%.255s; %.191s", sesh->complete.cand[at].val, sesh->complete.cand[at].label); }

    if (ret != LDG_ERR_AOK) { return ret; }

    return crosstalk_re_repl_emit(sesh->repl, "matches: %u32; more: %u32; Tab cycles", sesh->complete.cunt, (uint32_t)sesh->complete.more);
}

static uint32_t sesh_tab(crosstalk_re_repl_sesh_t *sesh)
{
    uint32_t ret = 0;
    uint8_t list = LDG_TRUTH_FALSE;

    ret = crosstalk_re_repl_complete_tab(&sesh->complete, sesh->subst, sesh->line, (uint32_t)sizeof(sesh->line), &sesh->len, &list);
    if (ret != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    if (list == LDG_TRUTH_TRUE)
    {
        ret = sesh_cand_list(sesh);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return sesh_line_draw(sesh);
}

static uint32_t sesh_byte_put(crosstalk_re_repl_sesh_t *sesh, uint8_t val)
{
    char ch = (char)val;

    if (sesh->esc_state == (uint8_t)SESH_ESC_CSI && crosstalk_re_repl_busy_is(sesh->repl) != LDG_TRUTH_TRUE && (val == (uint8_t)SESH_CSI_UP || val == (uint8_t)SESH_CSI_DOWN))
    {
        sesh->esc_state = (uint8_t)SESH_ESC_NONE;

        return sesh_hist_mov(sesh, val == (uint8_t)SESH_CSI_UP ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE);
    }

    if (sesh_esc_take_is(sesh, val) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (val == (uint8_t)SESH_CH_CANCEL) { return sesh_cancel(sesh); }

    if (crosstalk_re_repl_busy_is(sesh->repl) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (val == (uint8_t)LDG_ASCII_TAB) { return sesh_tab(sesh); }

    if (crosstalk_re_repl_complete_rst(&sesh->complete) != LDG_ERR_AOK) { return LDG_ERR_INVALID; }

    if (val == (uint8_t)LDG_ASCII_CR || val == (uint8_t)LDG_ASCII_LF) { return sesh_line_run(sesh); }

    if (val == (uint8_t)SESH_CH_BS || val == (uint8_t)SESH_CH_DEL)
    {
        if (sesh->len == 0) { return LDG_ERR_AOK; }

        sesh->len = sesh->len - (uint32_t)SESH_STEP;

        return sesh_out(sesh, SESH_ERASE, (uint32_t)sizeof(SESH_ERASE) - LDG_STR_TERM_SIZE);
    }

    if (val < (uint8_t)LDG_ASCII_PRINTABLE_MIN || val > (uint8_t)LDG_ASCII_PRINTABLE_MAX) { return LDG_ERR_AOK; }

    if (sesh->len + LDG_STR_TERM_SIZE >= (uint32_t)sizeof(sesh->line)) { return LDG_ERR_AOK; }

    sesh->line[sesh->len] = ch;
    sesh->len = sesh->len + (uint32_t)SESH_STEP;

    return sesh_out(sesh, &ch, (uint32_t)SESH_STEP);
}

uint32_t crosstalk_re_repl_sesh_bytes_put(crosstalk_re_repl_sesh_t *sesh, const uint8_t *bytes, uint32_t len)
{
    uint32_t at = 0;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(sesh == 0x0 || bytes == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(sesh->live != LDG_TRUTH_TRUE || sesh->repl == 0x0)) { return LDG_ERR_NOT_INIT; }

    for (at = 0; at < len && ret == LDG_ERR_AOK; at = at + (uint32_t)SESH_STEP) { ret = sesh_byte_put(sesh, bytes[at]); }

    return ret;
}

uint32_t crosstalk_re_repl_sesh_tick(crosstalk_re_repl_sesh_t *sesh)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(sesh == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (sesh->live != LDG_TRUTH_TRUE || sesh->repl == 0x0) { return LDG_ERR_AOK; }

    ret = crosstalk_re_repl_tick(sesh->repl);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (sesh->prompt_due != LDG_TRUTH_TRUE || crosstalk_re_repl_busy_is(sesh->repl) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    return crosstalk_re_repl_sesh_prompt_put(sesh);
}
