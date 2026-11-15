#include <stdint.h>

#include <dangling/core/arith.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/secure.h>
#include <dangling/str/fmt.h>
#include <dangling/str/str.h>

#include "re/repl/complete.h"
#include "re/repl/repl.h"
#include "re/repl/subst.h"

#define COMPLETE_STEP 1
#define COMPLETE_HEX_PREFIX_LEN 2
#define COMPLETE_ADDR_DIGITS_MIN 5
#define COMPLETE_ADDR_DIGITS_MAX 16
#define COMPLETE_TOK_FIRST 0
#define COMPLETE_TOK_ARG 1
#define COMPLETE_TOK_ARG_NEXT 2
#define COMPLETE_SET_LEAD_LEN 4
#define COMPLETE_UNSET_LEAD_LEN 6
#define COMPLETE_CH_UNDERSCORE '_'
#define COMPLETE_CH_VAR '$'
#define COMPLETE_CH_BRACE_OPEN '{'
#define COMPLETE_CH_DIGIT_FIRST '0'
#define COMPLETE_CH_HEX_LOWER 'x'
#define COMPLETE_LABEL_NONE ""
#define COMPLETE_LABEL_SHELL "shell"
#define COMPLETE_LABEL_NATIVE "native"
#define COMPLETE_LABEL_ADDR "observed address"
#define COMPLETE_WORDS_SHELL "aliases vars unset set help status attach detach threads thread"
#define COMPLETE_WORDS_NATIVE "modules symbol peek address regions read find findstr findptr xrefs scan terminate write poke"
#define COMPLETE_WORDS_SCAN "help begin status where watch fit list best rebase stop clear"
#define COMPLETE_WORDS_TYPE "u8 u16 u32 u64 i8 i16 i32 i64 ptr f32 f64"
#define COMPLETE_WORD_SCAN "scan"
#define COMPLETE_WORD_BEGIN "begin"
#define COMPLETE_WORD_RD "read"
#define COMPLETE_WORD_WR "write"
#define COMPLETE_WORD_NATIVE "native"
#define COMPLETE_LEAD_SET "set "
#define COMPLETE_LEAD_UNSET "unset "
#define COMPLETE_FMT_VAR "$%.31s"
#define COMPLETE_FMT_VAR_BRACED "${%.31s}"
#define COMPLETE_FMT_VAR_SET "%.31s="
#define COMPLETE_FMT_VAR_NAME "%.31s"

static uint8_t complete_name_ch_is(char ch)
{
    return ch == COMPLETE_CH_UNDERSCORE || ldg_char_alnum_is((uint8_t)ch) == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t complete_add(crosstalk_re_repl_complete_t *complete, const char *line, const char *val, const char *label)
{
    uint32_t lead = complete->end - complete->start;
    uint32_t at = 0;
    uint32_t ret = 0;

    for (at = 0; at < lead; at = at + (uint32_t)COMPLETE_STEP) { if (val[at] != line[complete->start + at]) { return LDG_ERR_AOK; } }

    for (at = 0; at < complete->cunt; at = at + (uint32_t)COMPLETE_STEP) { if (crosstalk_re_repl_cmd_eq_is(val, complete->cand[at].val) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; } }

    if (complete->cunt == (uint32_t)CROSSTALK_RE_REPL_COMPLETE_CAP)
    {
        complete->more = LDG_TRUTH_TRUE;
        return LDG_ERR_AOK;
    }

    ret = ldg_str_rbrcpy(complete->cand[complete->cunt].val, val, (uint64_t)sizeof(complete->cand[0].val));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_re_repl_name_get(label, complete->cand[complete->cunt].label, (uint32_t)sizeof(complete->cand[0].label));
    if (ret != LDG_ERR_AOK) { return ret; }

    complete->cunt = complete->cunt + (uint32_t)COMPLETE_STEP;

    return LDG_ERR_AOK;
}

static uint32_t complete_words_add(crosstalk_re_repl_complete_t *complete, const char *line, const char *words, const char *label)
{
    char word[CROSSTALK_RE_REPL_SUBST_NAME_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t pos = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    while (words[pos] != LDG_STR_TERM)
    {
        at = 0;

        while (words[pos] != LDG_STR_TERM && words[pos] != LDG_ASCII_SPACE)
        {
            if (at + LDG_STR_TERM_SIZE >= (uint32_t)sizeof(word)) { return LDG_ERR_FULL; }

            word[at] = words[pos];
            at = at + (uint32_t)COMPLETE_STEP;
            pos = pos + (uint32_t)COMPLETE_STEP;
        }

        word[at] = LDG_STR_TERM;

        if (words[pos] != LDG_STR_TERM) { pos = pos + (uint32_t)COMPLETE_STEP; }

        ret = complete_add(complete, line, word, label);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t complete_sort(crosstalk_re_repl_complete_t *complete)
{
    uint32_t at = 0;
    uint32_t prior = 0;
    uint32_t cmp = 0;
    uint32_t ret = 0;

    for (at = (uint32_t)COMPLETE_STEP; at < complete->cunt; at = at + (uint32_t)COMPLETE_STEP)
    {
        complete->held = complete->cand[at];
        prior = at;

        while (prior != 0)
        {
            ret = ldg_str_cmp(complete->cand[prior - (uint32_t)COMPLETE_STEP].val, complete->held.val, (uint64_t)sizeof(complete->held.val), &cmp);
            if (ret != LDG_ERR_AOK) { return ret; }

            if (cmp != LDG_ARITH_ORD_GT) { break; }

            complete->cand[prior] = complete->cand[prior - (uint32_t)COMPLETE_STEP];
            prior = prior - (uint32_t)COMPLETE_STEP;
        }

        complete->cand[prior] = complete->held;
    }

    return LDG_ERR_AOK;
}

static uint32_t complete_vars_add(crosstalk_re_repl_complete_t *complete, const crosstalk_re_repl_subst_t *subst, const char *line, const char *fmt)
{
    uint64_t len = 0;
    uint32_t slot = 0;
    uint32_t ret = 0;

    for (slot = 0; slot < (uint32_t)CROSSTALK_RE_REPL_SUBST_CAP; slot = slot + (uint32_t)COMPLETE_STEP)
    {
        if (subst->var[slot].name[0] == LDG_STR_TERM) { continue; }

        ret = ldg_str_fmt(complete->buff, (uint64_t)sizeof(complete->buff), &len, fmt, subst->var[slot].name);
        if (ret != LDG_ERR_AOK) { return ret; }

        ret = complete_add(complete, line, complete->buff, subst->var[slot].text);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t complete_addrs_add(crosstalk_re_repl_complete_t *complete, const crosstalk_re_repl_subst_t *subst, const char *line)
{
    uint32_t slot = 0;
    uint32_t ret = 0;

    for (slot = 0; slot < (uint32_t)CROSSTALK_RE_REPL_COMPLETE_ADDR_CAP; slot = slot + (uint32_t)COMPLETE_STEP)
    {
        if (complete->addr[slot][0] == LDG_STR_TERM) { continue; }

        ret = complete_add(complete, line, complete->addr[slot], COMPLETE_LABEL_ADDR);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    for (slot = 0; slot < (uint32_t)CROSSTALK_RE_REPL_SUBST_CAP; slot = slot + (uint32_t)COMPLETE_STEP)
    {
        if (subst->var[slot].name[0] == LDG_STR_TERM || subst->var[slot].text[0] != COMPLETE_CH_DIGIT_FIRST || subst->var[slot].text[COMPLETE_STEP] != COMPLETE_CH_HEX_LOWER) { continue; }

        ret = complete_add(complete, line, subst->var[slot].text, subst->var[slot].name);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t complete_aliases_add(crosstalk_re_repl_complete_t *complete, const crosstalk_re_repl_subst_t *subst, const char *line)
{
    uint32_t slot = 0;
    uint32_t ret = 0;

    for (slot = 0; slot < (uint32_t)CROSSTALK_RE_REPL_SUBST_CAP; slot = slot + (uint32_t)COMPLETE_STEP)
    {
        if (subst->alias[slot].name[0] == LDG_STR_TERM) { continue; }

        ret = complete_add(complete, line, subst->alias[slot].name, subst->alias[slot].text);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint8_t complete_lead_is(const char *line, uint32_t len, const char *lead, uint32_t lead_len)
{
    uint32_t at = 0;

    if (len != lead_len) { return LDG_TRUTH_FALSE; }

    for (at = 0; at < lead_len; at = at + (uint32_t)COMPLETE_STEP) { if (line[at] != lead[at]) { return LDG_TRUTH_FALSE; } }

    return LDG_TRUTH_TRUE;
}

static uint8_t complete_tok_eq_is(const char *line, uint32_t start, const char *word)
{
    uint32_t at = 0;

    while (word[at] != LDG_STR_TERM)
    {
        if (line[start + at] != word[at]) { return LDG_TRUTH_FALSE; }

        at = at + (uint32_t)COMPLETE_STEP;
    }

    return line[start + at] == LDG_ASCII_SPACE || line[start + at] == LDG_ASCII_TAB ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static void complete_toks_get(const char *line, uint32_t end, uint32_t *out_cunt, uint32_t *out_first, uint32_t *out_arg)
{
    uint32_t at = 0;
    uint32_t cunt = 0;
    uint8_t in_word = LDG_TRUTH_FALSE;

    *out_first = 0;
    *out_arg = 0;

    for (at = 0; at < end; at = at + (uint32_t)COMPLETE_STEP)
    {
        if (line[at] == LDG_ASCII_SPACE || line[at] == LDG_ASCII_TAB)
        {
            in_word = LDG_TRUTH_FALSE;
            continue;
        }

        if (in_word == LDG_TRUTH_TRUE) { continue; }

        if (cunt == (uint32_t)COMPLETE_TOK_FIRST) { *out_first = at; }

        if (cunt == (uint32_t)COMPLETE_TOK_ARG) { *out_arg = at; }

        cunt = cunt + (uint32_t)COMPLETE_STEP;
        in_word = LDG_TRUTH_TRUE;
    }

    *out_cunt = cunt;
}

static uint32_t complete_cmd_gather(crosstalk_re_repl_complete_t *complete, const char *line)
{
    uint32_t cunt = 0;
    uint32_t first = 0;
    uint32_t arg = 0;
    uint32_t ret = 0;

    complete_toks_get(line, complete->start, &cunt, &first, &arg);

    if (cunt != 0 && complete_tok_eq_is(line, first, COMPLETE_WORD_NATIVE) == LDG_TRUTH_TRUE)
    {
        cunt = cunt - (uint32_t)COMPLETE_STEP;
        first = arg;

        if (cunt == (uint32_t)COMPLETE_TOK_FIRST) { return complete_words_add(complete, line, COMPLETE_WORDS_NATIVE, COMPLETE_LABEL_NATIVE); }
    }

    if (cunt == (uint32_t)COMPLETE_TOK_FIRST)
    {
        ret = complete_words_add(complete, line, COMPLETE_WORDS_SHELL, COMPLETE_LABEL_SHELL);
        if (ret != LDG_ERR_AOK) { return ret; }

        return complete_words_add(complete, line, COMPLETE_WORDS_NATIVE " " COMPLETE_WORD_NATIVE, COMPLETE_LABEL_NATIVE);
    }

    if (cunt == (uint32_t)COMPLETE_TOK_ARG && complete_tok_eq_is(line, first, COMPLETE_WORD_SCAN) == LDG_TRUTH_TRUE) { return complete_words_add(complete, line, COMPLETE_WORDS_SCAN, COMPLETE_LABEL_NONE); }

    if (cunt == (uint32_t)COMPLETE_TOK_ARG && (complete_tok_eq_is(line, first, COMPLETE_WORD_RD) == LDG_TRUTH_TRUE || complete_tok_eq_is(line, first, COMPLETE_WORD_WR) == LDG_TRUTH_TRUE)) { return complete_words_add(complete, line, COMPLETE_WORDS_TYPE, COMPLETE_LABEL_NONE); }

    if (cunt == (uint32_t)COMPLETE_TOK_ARG_NEXT && complete_tok_eq_is(line, first, COMPLETE_WORD_SCAN) == LDG_TRUTH_TRUE && complete_tok_eq_is(line, arg, COMPLETE_WORD_BEGIN) == LDG_TRUTH_TRUE) { return complete_words_add(complete, line, COMPLETE_WORDS_TYPE, COMPLETE_LABEL_NONE); }

    return LDG_ERR_AOK;
}

static uint8_t complete_var_ref_is(const char *line, uint32_t tok_start, uint32_t cursor, uint32_t *out_mark, uint8_t *out_braced)
{
    uint32_t at = cursor;
    uint32_t name = 0;

    *out_braced = LDG_TRUTH_FALSE;

    while (at > tok_start && line[at - (uint32_t)COMPLETE_STEP] != COMPLETE_CH_VAR) { at = at - (uint32_t)COMPLETE_STEP; }

    if (at == tok_start) { return LDG_TRUTH_FALSE; }

    *out_mark = at - (uint32_t)COMPLETE_STEP;
    name = at;

    if (name < cursor && line[name] == COMPLETE_CH_BRACE_OPEN)
    {
        *out_braced = LDG_TRUTH_TRUE;
        name = name + (uint32_t)COMPLETE_STEP;
    }

    while (name < cursor)
    {
        if (complete_name_ch_is(line[name]) != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

        name = name + (uint32_t)COMPLETE_STEP;
    }

    return LDG_TRUTH_TRUE;
}

static uint8_t complete_hex_ref_is(const char *line, uint32_t cursor, uint32_t *out_mark)
{
    uint32_t at = cursor;

    while (at != 0 && ldg_char_hex_is((uint8_t)line[at - (uint32_t)COMPLETE_STEP]) == LDG_TRUTH_TRUE) { at = at - (uint32_t)COMPLETE_STEP; }

    if (at < (uint32_t)COMPLETE_HEX_PREFIX_LEN || line[at - (uint32_t)COMPLETE_STEP] != COMPLETE_CH_HEX_LOWER || line[at - (uint32_t)COMPLETE_HEX_PREFIX_LEN] != COMPLETE_CH_DIGIT_FIRST) { return LDG_TRUTH_FALSE; }

    *out_mark = at - (uint32_t)COMPLETE_HEX_PREFIX_LEN;

    if (*out_mark != 0 && complete_name_ch_is(line[*out_mark - (uint32_t)COMPLETE_STEP]) == LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static uint32_t complete_gather(crosstalk_re_repl_complete_t *complete, const crosstalk_re_repl_subst_t *subst, const char *line, uint32_t cursor)
{
    uint32_t mark = 0;
    uint32_t ret = 0;
    uint8_t braced = LDG_TRUTH_FALSE;

    complete->cunt = 0;
    complete->more = LDG_TRUTH_FALSE;
    complete->sel = CROSSTALK_RE_REPL_COMPLETE_SEL_NONE;
    complete->start = cursor;
    complete->end = cursor;
    complete->space = LDG_TRUTH_TRUE;

    while (complete->start != 0 && line[complete->start - (uint32_t)COMPLETE_STEP] != LDG_ASCII_SPACE && line[complete->start - (uint32_t)COMPLETE_STEP] != LDG_ASCII_TAB) { complete->start = complete->start - (uint32_t)COMPLETE_STEP; }

    if (complete_var_ref_is(line, complete->start, cursor, &mark, &braced) == LDG_TRUTH_TRUE)
    {
        complete->start = mark;
        complete->space = LDG_TRUTH_FALSE;
        ret = complete_vars_add(complete, subst, line, braced == LDG_TRUTH_TRUE ? COMPLETE_FMT_VAR_BRACED : COMPLETE_FMT_VAR);
    }
    else if (complete_hex_ref_is(line, cursor, &mark) == LDG_TRUTH_TRUE)
    {
        complete->start = mark;
        complete->space = LDG_TRUTH_FALSE;
        ret = complete_addrs_add(complete, subst, line);
    }
    else if (complete_lead_is(line, complete->start, COMPLETE_LEAD_SET, (uint32_t)COMPLETE_SET_LEAD_LEN) == LDG_TRUTH_TRUE)
    {
        complete->space = LDG_TRUTH_FALSE;
        ret = complete_vars_add(complete, subst, line, COMPLETE_FMT_VAR_SET);
    }
    else if (complete_lead_is(line, complete->start, COMPLETE_LEAD_UNSET, (uint32_t)COMPLETE_UNSET_LEAD_LEN) == LDG_TRUTH_TRUE) { ret = complete_vars_add(complete, subst, line, COMPLETE_FMT_VAR_NAME); }
    else
    {
        if (complete->start == 0) { ret = complete_aliases_add(complete, subst, line); }

        if (ret == LDG_ERR_AOK) { ret = complete_cmd_gather(complete, line); }
    }

    if (ret != LDG_ERR_AOK) { return ret; }

    return complete_sort(complete);
}

static uint32_t complete_put(crosstalk_re_repl_complete_t *complete, char *line, uint32_t cap, uint32_t *len, const char *val, uint32_t size, uint8_t space)
{
    uint32_t ret = 0;

    if (complete->start > *len) { return LDG_ERR_BOUNDS; }

    if (complete->start + size + (uint32_t)COMPLETE_HEX_PREFIX_LEN >= cap) { return LDG_ERR_FULL; }

    if (size != 0)
    {
        ret = ldg_mem_secure_cpy(line + complete->start, val, (uint64_t)size);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    *len = complete->start + size;
    complete->end = *len;

    if (space == LDG_TRUTH_TRUE)
    {
        line[*len] = LDG_ASCII_SPACE;
        *len = *len + (uint32_t)COMPLETE_STEP;
    }

    line[*len] = LDG_STR_TERM;

    return ldg_str_rbrcpy(complete->last, line, (uint64_t)sizeof(complete->last));
}

static uint32_t complete_common_get(const crosstalk_re_repl_complete_t *complete, uint32_t size)
{
    uint32_t common = complete->more == LDG_TRUTH_TRUE ? complete->end - complete->start : size;
    uint32_t at = 0;
    uint32_t pos = 0;

    for (at = (uint32_t)COMPLETE_STEP; at < complete->cunt; at = at + (uint32_t)COMPLETE_STEP)
    {
        pos = 0;

        while (pos < common && complete->cand[0].val[pos] == complete->cand[at].val[pos]) { pos = pos + (uint32_t)COMPLETE_STEP; }

        common = pos;
    }

    return common;
}

uint32_t crosstalk_re_repl_complete_init(crosstalk_re_repl_complete_t *complete)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(complete == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_secure_zero(complete, (uint64_t)sizeof(*complete));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    complete->sel = CROSSTALK_RE_REPL_COMPLETE_SEL_NONE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_repl_complete_rst(crosstalk_re_repl_complete_t *complete)
{
    if (LDG_UNLIKELY(complete == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    complete->on = LDG_TRUTH_FALSE;
    complete->sel = CROSSTALK_RE_REPL_COMPLETE_SEL_NONE;

    return LDG_ERR_AOK;
}

static uint32_t complete_addr_add(crosstalk_re_repl_complete_t *complete, const char *text, uint32_t len)
{
    uint32_t slot = 0;
    uint32_t pos = 0;
    uint32_t ret = 0;

    if (len == 0 || len >= (uint32_t)CROSSTALK_RE_REPL_COMPLETE_ADDR_TEXT_CAP) { return LDG_ERR_AOK; }

    for (slot = 0; slot < (uint32_t)CROSSTALK_RE_REPL_COMPLETE_ADDR_CAP; slot = slot + (uint32_t)COMPLETE_STEP)
    {
        pos = 0;

        while (pos < len && complete->addr[slot][pos] == text[pos]) { pos = pos + (uint32_t)COMPLETE_STEP; }

        if (pos == len && complete->addr[slot][pos] == LDG_STR_TERM) { return LDG_ERR_AOK; }
    }

    ret = ldg_mem_secure_cpy(complete->addr[complete->addr_next], text, (uint64_t)len);
    if (ret != LDG_ERR_AOK) { return ret; }

    complete->addr[complete->addr_next][len] = LDG_STR_TERM;
    complete->addr_next = (complete->addr_next + (uint32_t)COMPLETE_STEP) % (uint32_t)CROSSTALK_RE_REPL_COMPLETE_ADDR_CAP;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_repl_complete_addr_capture(crosstalk_re_repl_complete_t *complete, const char *text, uint32_t len)
{
    uint32_t at = 0;
    uint32_t end = 0;
    uint32_t digits = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(complete == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (at = 0; at + (uint32_t)COMPLETE_HEX_PREFIX_LEN < len; at = at + (uint32_t)COMPLETE_STEP)
    {
        if (text[at] != COMPLETE_CH_DIGIT_FIRST || text[at + (uint32_t)COMPLETE_STEP] != COMPLETE_CH_HEX_LOWER) { continue; }

        if (at != 0 && complete_name_ch_is(text[at - (uint32_t)COMPLETE_STEP]) == LDG_TRUTH_TRUE) { continue; }

        end = at + (uint32_t)COMPLETE_HEX_PREFIX_LEN;

        while (end < len && ldg_char_hex_is((uint8_t)text[end]) == LDG_TRUTH_TRUE) { end = end + (uint32_t)COMPLETE_STEP; }

        digits = end - at - (uint32_t)COMPLETE_HEX_PREFIX_LEN;

        if (digits >= (uint32_t)COMPLETE_ADDR_DIGITS_MIN && digits <= (uint32_t)COMPLETE_ADDR_DIGITS_MAX && (end == len || ldg_char_alnum_is((uint8_t)text[end]) != LDG_TRUTH_TRUE))
        {
            ret = complete_addr_add(complete, text + at, end - at);
            if (ret != LDG_ERR_AOK) { return ret; }
        }

        at = end - (uint32_t)COMPLETE_STEP;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_repl_complete_tab(crosstalk_re_repl_complete_t *complete, const crosstalk_re_repl_subst_t *subst, char *line, uint32_t cap, uint32_t *len, uint8_t *out_list)
{
    uint64_t size = 0;
    uint32_t common = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(complete == 0x0 || subst == 0x0 || line == 0x0 || len == 0x0 || out_list == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_list = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(*len >= cap)) { return LDG_ERR_BOUNDS; }

    line[*len] = LDG_STR_TERM;

    if (complete->on == LDG_TRUTH_TRUE && complete->cunt != 0 && crosstalk_re_repl_cmd_eq_is(line, complete->last) == LDG_TRUTH_TRUE)
    {
        complete->sel = complete->sel == CROSSTALK_RE_REPL_COMPLETE_SEL_NONE ? (uint32_t)COMPLETE_TOK_FIRST : (complete->sel + (uint32_t)COMPLETE_STEP) % complete->cunt;

        ret = ldg_str_len_get(complete->cand[complete->sel].val, (uint64_t)sizeof(complete->cand[0].val), &size);
        if (ret != LDG_ERR_AOK) { return ret; }

        return complete_put(complete, line, cap, len, complete->cand[complete->sel].val, (uint32_t)size, LDG_TRUTH_FALSE);
    }

    complete->on = LDG_TRUTH_FALSE;

    ret = complete_gather(complete, subst, line, *len);
    if (ret != LDG_ERR_AOK || complete->cunt == 0) { return ret; }

    ret = ldg_str_len_get(complete->cand[0].val, (uint64_t)sizeof(complete->cand[0].val), &size);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (complete->cunt == (uint32_t)COMPLETE_STEP && complete->more != LDG_TRUTH_TRUE) { return complete_put(complete, line, cap, len, complete->cand[0].val, (uint32_t)size, complete->space); }

    common = complete_common_get(complete, (uint32_t)size);

    ret = complete_put(complete, line, cap, len, complete->cand[0].val, common > complete->end - complete->start ? common : complete->end - complete->start, LDG_TRUTH_FALSE);
    if (ret != LDG_ERR_AOK) { return ret; }

    complete->on = LDG_TRUTH_TRUE;
    *out_list = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}
