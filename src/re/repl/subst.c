#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/secure.h>
#include <dangling/str/str.h>

#include "re/repl/repl.h"
#include "re/repl/subst.h"

#define SUBST_STEP 1
#define SUBST_PAIR 2
#define SUBST_LINE_ORIG 0
#define SUBST_CH_UNDERSCORE '_'
#define SUBST_CH_EQ '='
#define SUBST_CH_VAR '$'
#define SUBST_CH_BRACE_OPEN '{'
#define SUBST_CH_BRACE_CLOSE '}'
#define SUBST_CH_QUOTE_SINGLE '\''
#define SUBST_CH_QUOTE_DOUBLE '"'
#define SUBST_CH_BRACKET_OPEN '['
#define SUBST_CH_BRACKET_CLOSE ']'
#define SUBST_CH_MINUS '-'
#define SUBST_CH_PLUS '+'
#define SUBST_CH_POINT '.'
#define SUBST_CH_EXP_LOWER 'e'
#define SUBST_CH_EXP_UPPER 'E'
#define SUBST_CH_HEX_LOWER 'x'
#define SUBST_CH_HEX_UPPER 'X'
#define SUBST_CH_DIGIT_FIRST '0'
#define SUBST_WORD_SET "set"
#define SUBST_WORD_UNSET "unset"
#define SUBST_WORD_VARS "vars"
#define SUBST_WORD_ALIASES "aliases"
#define SUBST_WORD_HELP "help"
#define SUBST_WORD_QUIT "quit"
#define SUBST_WORD_TRUE "true"
#define SUBST_WORD_FALSE "false"
#define SUBST_WORD_NIL "nil"
#define SUBST_SET_LEN 3
#define SUBST_UNSET_LEN 5

static uint8_t subst_name_ch_is(char ch, uint8_t first)
{
    if (ch == SUBST_CH_UNDERSCORE) { return LDG_TRUTH_TRUE; }

    if (first == LDG_TRUTH_TRUE) { return ldg_char_alpha_is((uint8_t)ch) == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    return ldg_char_alnum_is((uint8_t)ch) == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t subst_name_validate(const char *name, uint32_t len)
{
    uint32_t at = 0;

    if (len == 0 || len >= (uint32_t)CROSSTALK_RE_REPL_SUBST_NAME_CAP) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (at = 0; at < len; at = at + (uint32_t)SUBST_STEP) { if (subst_name_ch_is(name[at], at == 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE) != LDG_TRUTH_TRUE) { return LDG_ERR_FUNC_ARG_INVALID; } }

    return LDG_ERR_AOK;
}

static uint8_t subst_span_eq_is(const char *name, const char *text, uint32_t len)
{
    uint32_t at = 0;

    for (at = 0; at < len; at = at + (uint32_t)SUBST_STEP) { if (name[at] != text[at]) { return LDG_TRUTH_FALSE; } }

    return name[len] == LDG_STR_TERM ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t subst_entry_find(const crosstalk_re_repl_subst_entry_t *entry, const char *name, uint32_t len)
{
    uint32_t slot = 0;

    for (slot = 0; slot < (uint32_t)CROSSTALK_RE_REPL_SUBST_CAP; slot = slot + (uint32_t)SUBST_STEP) { if (entry[slot].name[0] != LDG_STR_TERM && subst_span_eq_is(entry[slot].name, name, len) == LDG_TRUTH_TRUE) { return slot; } }

    return UINT32_MAX;
}

static uint32_t subst_entry_vacant_find(const crosstalk_re_repl_subst_entry_t *entry)
{
    uint32_t slot = 0;

    for (slot = 0; slot < (uint32_t)CROSSTALK_RE_REPL_SUBST_CAP; slot = slot + (uint32_t)SUBST_STEP) { if (entry[slot].name[0] == LDG_STR_TERM) { return slot; } }

    return UINT32_MAX;
}

static uint32_t subst_entry_put(crosstalk_re_repl_subst_entry_t *entry, const char *name, uint32_t name_len, const char *text)
{
    uint32_t ret = 0;

    ret = ldg_str_rbrcpy(entry->text, text, (uint64_t)sizeof(entry->text));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_mem_secure_cpy(entry->name, name, (uint64_t)name_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    entry->name[name_len] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

static uint32_t subst_quote_end(const char *text, uint32_t len, uint32_t start, uint32_t *end)
{
    uint32_t at = start + (uint32_t)SUBST_STEP;

    while (at < len)
    {
        if (text[at] == LDG_ASCII_BSLASH && at + (uint32_t)SUBST_STEP < len)
        {
            at = at + (uint32_t)SUBST_PAIR;
            continue;
        }

        if (text[at] == text[start])
        {
            *end = at + (uint32_t)SUBST_STEP;
            return LDG_ERR_AOK;
        }

        at = at + (uint32_t)SUBST_STEP;
    }

    return LDG_ERR_STR_INVALID;
}

static uint32_t subst_literal_end(const char *text, uint32_t len, uint32_t start, uint32_t *end)
{
    uint32_t at = start + (uint32_t)SUBST_STEP;
    uint32_t probe = 0;
    uint32_t level = 0;

    *end = start;

    if (text[start] == SUBST_CH_QUOTE_SINGLE || text[start] == SUBST_CH_QUOTE_DOUBLE) { return subst_quote_end(text, len, start, end); }

    if (text[start] != SUBST_CH_BRACKET_OPEN) { return LDG_ERR_AOK; }

    while (at < len && text[at] == SUBST_CH_EQ) { at = at + (uint32_t)SUBST_STEP; }

    if (at == len || text[at] != SUBST_CH_BRACKET_OPEN) { return LDG_ERR_AOK; }

    level = at - start - (uint32_t)SUBST_STEP;

    for (at = at + (uint32_t)SUBST_STEP; at < len; at = at + (uint32_t)SUBST_STEP)
    {
        if (text[at] != SUBST_CH_BRACKET_CLOSE) { continue; }

        probe = at + (uint32_t)SUBST_STEP;

        while (probe < len && text[probe] == SUBST_CH_EQ) { probe = probe + (uint32_t)SUBST_STEP; }

        if (probe < len && probe - at - (uint32_t)SUBST_STEP == level && text[probe] == SUBST_CH_BRACKET_CLOSE)
        {
            *end = probe + (uint32_t)SUBST_STEP;
            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_STR_INVALID;
}

static uint32_t subst_digits_skip(const char *text, uint32_t len, uint32_t at)
{
    while (at < len && ldg_char_digit_is((uint8_t)text[at]) == LDG_TRUTH_TRUE) { at = at + (uint32_t)SUBST_STEP; }

    return at;
}

static uint32_t subst_num_validate(const char *text, uint32_t len)
{
    uint32_t at = 0;
    uint32_t mark = 0;
    uint32_t digits = 0;

    if (text[at] == SUBST_CH_PLUS || text[at] == SUBST_CH_MINUS) { at = at + (uint32_t)SUBST_STEP; }

    if (at + (uint32_t)SUBST_PAIR <= len && text[at] == SUBST_CH_DIGIT_FIRST && (text[at + (uint32_t)SUBST_STEP] == SUBST_CH_HEX_LOWER || text[at + (uint32_t)SUBST_STEP] == SUBST_CH_HEX_UPPER))
    {
        at = at + (uint32_t)SUBST_PAIR;
        mark = at;

        while (at < len && ldg_char_hex_is((uint8_t)text[at]) == LDG_TRUTH_TRUE) { at = at + (uint32_t)SUBST_STEP; }

        return at == len && at > mark ? LDG_ERR_AOK : LDG_ERR_STR_INVALID;
    }

    mark = at;
    at = subst_digits_skip(text, len, at);
    digits = at - mark;

    if (at < len && text[at] == SUBST_CH_POINT)
    {
        mark = at + (uint32_t)SUBST_STEP;
        at = subst_digits_skip(text, len, mark);
        digits = digits + (at - mark);
    }

    if (digits == 0) { return LDG_ERR_STR_INVALID; }

    if (at < len && (text[at] == SUBST_CH_EXP_LOWER || text[at] == SUBST_CH_EXP_UPPER))
    {
        at = at + (uint32_t)SUBST_STEP;

        if (at < len && (text[at] == SUBST_CH_PLUS || text[at] == SUBST_CH_MINUS)) { at = at + (uint32_t)SUBST_STEP; }

        mark = at;
        at = subst_digits_skip(text, len, at);

        if (at == mark) { return LDG_ERR_STR_INVALID; }
    }

    return at == len ? LDG_ERR_AOK : LDG_ERR_STR_INVALID;
}

static uint32_t subst_val_validate(const char *text, uint32_t len)
{
    uint32_t end = 0;
    uint32_t ret = 0;

    if (len == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (crosstalk_re_repl_cmd_eq_is(text, SUBST_WORD_TRUE) == LDG_TRUTH_TRUE || crosstalk_re_repl_cmd_eq_is(text, SUBST_WORD_FALSE) == LDG_TRUTH_TRUE || crosstalk_re_repl_cmd_eq_is(text, SUBST_WORD_NIL) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = subst_literal_end(text, len, SUBST_LINE_ORIG, &end);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (end != 0) { return end == len ? LDG_ERR_AOK : LDG_ERR_STR_INVALID; }

    return subst_num_validate(text, len);
}

static uint32_t subst_var_ref_get(const crosstalk_re_repl_subst_t *subst, const char *line, uint32_t len, uint32_t src, uint32_t *end, const char **part, uint64_t *part_len)
{
    uint32_t start = src + (uint32_t)SUBST_STEP;
    uint32_t stop = 0;
    uint32_t slot = 0;
    uint32_t ret = 0;
    uint8_t braced = LDG_TRUTH_FALSE;

    if (start < len && line[start] == SUBST_CH_VAR)
    {
        *end = start + (uint32_t)SUBST_STEP;
        return LDG_ERR_AOK;
    }

    if (start < len && line[start] == SUBST_CH_BRACE_OPEN)
    {
        braced = LDG_TRUTH_TRUE;
        start = start + (uint32_t)SUBST_STEP;
    }

    stop = start;

    while (stop < len && subst_name_ch_is(line[stop], LDG_TRUTH_FALSE) == LDG_TRUTH_TRUE) { stop = stop + (uint32_t)SUBST_STEP; }

    ret = subst_name_validate(line + start, stop - start);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (braced == LDG_TRUTH_TRUE && (stop == len || line[stop] != SUBST_CH_BRACE_CLOSE)) { return LDG_ERR_STR_INVALID; }

    slot = subst_entry_find(subst->var, line + start, stop - start);
    if (slot == UINT32_MAX) { return LDG_ERR_NOT_FOUND; }

    *end = braced == LDG_TRUTH_TRUE ? stop + (uint32_t)SUBST_STEP : stop;
    *part = subst->var[slot].text;

    return ldg_str_len_get(*part, (uint64_t)sizeof(subst->var[slot].text), part_len);
}

static uint32_t subst_part_end(const char *line, uint32_t len, uint32_t src, uint32_t *end)
{
    uint32_t ret = 0;

    ret = subst_literal_end(line, len, src, end);
    if (ret != LDG_ERR_AOK || *end != src) { return ret; }

    if (line[src] != SUBST_CH_MINUS || src + (uint32_t)SUBST_STEP >= len || line[src + (uint32_t)SUBST_STEP] != SUBST_CH_MINUS) { return LDG_ERR_AOK; }

    if (src + (uint32_t)SUBST_PAIR >= len || line[src + (uint32_t)SUBST_PAIR] != SUBST_CH_BRACKET_OPEN)
    {
        *end = len;
        return LDG_ERR_AOK;
    }

    ret = subst_literal_end(line, len, src + (uint32_t)SUBST_PAIR, end);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (*end <= src + (uint32_t)SUBST_PAIR) { *end = len; }

    return LDG_ERR_AOK;
}

static uint32_t subst_var_expand(crosstalk_re_repl_subst_t *subst, char *line, uint32_t cap, uint32_t *len)
{
    const char *part = 0x0;
    uint64_t part_len = 0;
    uint32_t src = 0;
    uint32_t dst = 0;
    uint32_t end = 0;
    uint32_t ret = 0;

    while (src < *len)
    {
        end = src;

        ret = subst_part_end(line, *len, src, &end);
        if (ret != LDG_ERR_AOK) { return ret; }

        part = line + src;
        part_len = end > src ? (uint64_t)(end - src) : (uint64_t)SUBST_STEP;

        if (end == src && line[src] == SUBST_CH_VAR)
        {
            ret = subst_var_ref_get(subst, line, *len, src, &end, &part, &part_len);
            if (ret != LDG_ERR_AOK) { return ret; }
        }

        if (part_len >= (uint64_t)(cap - dst)) { return LDG_ERR_STR_TRUNC; }

        ret = ldg_mem_secure_cpy(subst->buff + dst, part, part_len);
        if (ret != LDG_ERR_AOK) { return ret; }

        dst = dst + (uint32_t)part_len;
        src = end > src ? end : src + (uint32_t)SUBST_STEP;
    }

    subst->buff[dst] = LDG_STR_TERM;

    ret = ldg_str_rbrcpy(line, subst->buff, (uint64_t)cap);
    if (ret == LDG_ERR_AOK) { *len = dst; }

    return ret;
}

static uint32_t subst_list(crosstalk_re_repl_ctx_t *repl, const crosstalk_re_repl_subst_entry_t *entry, const char *vacant_text)
{
    uint32_t slot = 0;
    uint32_t cunt = 0;
    uint32_t ret = 0;

    for (slot = 0; slot < (uint32_t)CROSSTALK_RE_REPL_SUBST_CAP; slot = slot + (uint32_t)SUBST_STEP)
    {
        if (entry[slot].name[0] == LDG_STR_TERM) { continue; }

        ret = crosstalk_re_repl_emit(repl, "%.31s=%.255s", entry[slot].name, entry[slot].text);
        if (ret != LDG_ERR_AOK) { return ret; }

        cunt = cunt + (uint32_t)SUBST_STEP;
    }

    return cunt != 0 ? LDG_ERR_AOK : crosstalk_re_repl_emit(repl, "%.95s", vacant_text);
}

static uint32_t subst_spaces_skip(const char *line, uint32_t len, uint32_t at)
{
    while (at < len && ldg_char_space_is((uint8_t)line[at]) == LDG_TRUTH_TRUE) { at = at + (uint32_t)SUBST_STEP; }

    return at;
}

static uint8_t subst_word_lead_is(const char *line, uint32_t len, const char *word, uint32_t word_len)
{
    uint32_t at = 0;

    if (len < word_len) { return LDG_TRUTH_FALSE; }

    for (at = 0; at < word_len; at = at + (uint32_t)SUBST_STEP) { if (line[at] != word[at]) { return LDG_TRUTH_FALSE; } }

    return len == word_len || ldg_char_space_is((uint8_t)line[word_len]) == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t subst_var_unset(crosstalk_re_repl_subst_t *subst, crosstalk_re_repl_ctx_t *repl, const char *line, uint32_t len)
{
    uint32_t start = subst_spaces_skip(line, len, (uint32_t)SUBST_UNSET_LEN);
    uint32_t end = start;
    uint32_t slot = 0;
    uint32_t ret = 0;

    while (end < len && ldg_char_space_is((uint8_t)line[end]) != LDG_TRUTH_TRUE) { end = end + (uint32_t)SUBST_STEP; }

    if (subst_spaces_skip(line, len, end) != len) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = subst_name_validate(line + start, end - start);
    if (ret != LDG_ERR_AOK) { return ret; }

    slot = subst_entry_find(subst->var, line + start, end - start);
    if (slot == UINT32_MAX) { return LDG_ERR_NOT_FOUND; }

    subst->var[slot].name[0] = LDG_STR_TERM;
    subst->var[slot].text[0] = LDG_STR_TERM;

    return crosstalk_re_repl_emit(repl, "variable removed");
}

static uint32_t subst_var_set(crosstalk_re_repl_subst_t *subst, crosstalk_re_repl_ctx_t *repl, const char *line, uint32_t len, uint8_t *out_handled)
{
    uint32_t start = subst_spaces_skip(line, len, (uint32_t)SUBST_SET_LEN);
    uint32_t end = start;
    uint32_t mark = 0;
    uint32_t slot = 0;
    uint32_t ret = 0;

    while (end < len && line[end] != SUBST_CH_EQ && ldg_char_space_is((uint8_t)line[end]) != LDG_TRUTH_TRUE) { end = end + (uint32_t)SUBST_STEP; }

    mark = subst_spaces_skip(line, len, end);
    if (mark == len || line[mark] != SUBST_CH_EQ) { return LDG_ERR_AOK; }

    *out_handled = LDG_TRUTH_TRUE;

    ret = subst_name_validate(line + start, end - start);
    if (ret != LDG_ERR_AOK) { return ret; }

    mark = subst_spaces_skip(line, len, mark + (uint32_t)SUBST_STEP);

    ret = subst_val_validate(line + mark, len - mark);
    if (ret != LDG_ERR_AOK) { return ret; }

    slot = subst_entry_find(subst->var, line + start, end - start);
    if (slot == UINT32_MAX) { slot = subst_entry_vacant_find(subst->var); }

    if (slot == UINT32_MAX) { return LDG_ERR_FULL; }

    ret = subst_entry_put(&subst->var[slot], line + start, end - start, line + mark);
    if (ret != LDG_ERR_AOK) { return ret; }

    return crosstalk_re_repl_emit(repl, "variable saved: %.31s=%.255s", subst->var[slot].name, subst->var[slot].text);
}

static uint32_t subst_var_cmd(crosstalk_re_repl_subst_t *subst, crosstalk_re_repl_ctx_t *repl, const char *line, uint32_t len, uint8_t *out_handled)
{
    if (crosstalk_re_repl_cmd_eq_is(line, SUBST_WORD_VARS) == LDG_TRUTH_TRUE)
    {
        *out_handled = LDG_TRUTH_TRUE;
        return subst_list(repl, subst->var, "no variables defined; use set name=value");
    }

    if (subst_word_lead_is(line, len, SUBST_WORD_UNSET, (uint32_t)SUBST_UNSET_LEN) == LDG_TRUTH_TRUE)
    {
        *out_handled = LDG_TRUTH_TRUE;
        return subst_var_unset(subst, repl, line, len);
    }

    if (len > (uint32_t)SUBST_SET_LEN && subst_word_lead_is(line, len, SUBST_WORD_SET, (uint32_t)SUBST_SET_LEN) == LDG_TRUTH_TRUE) { return subst_var_set(subst, repl, line, len, out_handled); }

    return LDG_ERR_AOK;
}

static uint8_t subst_alias_reserved_is(const char *name)
{
    if (crosstalk_re_repl_cmd_eq_is(name, SUBST_WORD_QUIT) == LDG_TRUTH_TRUE || crosstalk_re_repl_cmd_eq_is(name, SUBST_WORD_HELP) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    if (crosstalk_re_repl_cmd_eq_is(name, SUBST_WORD_ALIASES) == LDG_TRUTH_TRUE || crosstalk_re_repl_cmd_eq_is(name, SUBST_WORD_VARS) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return crosstalk_re_repl_cmd_eq_is(name, SUBST_WORD_UNSET) == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t subst_alias_set(crosstalk_re_repl_subst_t *subst, crosstalk_re_repl_ctx_t *repl, char *line, uint32_t name_len, uint32_t mark, uint32_t len)
{
    uint32_t text_start = subst_spaces_skip(line, len, mark + (uint32_t)SUBST_STEP);
    uint32_t slot = 0;
    uint32_t ret = 0;

    ret = subst_name_validate(line, name_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    line[name_len] = LDG_STR_TERM;

    if (subst_alias_reserved_is(line) == LDG_TRUTH_TRUE) { return LDG_ERR_DENIED; }

    slot = subst_entry_find(subst->alias, line, name_len);

    if (text_start == len)
    {
        if (slot == UINT32_MAX) { return LDG_ERR_NOT_FOUND; }

        subst->alias[slot].name[0] = LDG_STR_TERM;
        subst->alias[slot].text[0] = LDG_STR_TERM;

        return crosstalk_re_repl_emit(repl, "alias removed");
    }

    if (slot == UINT32_MAX) { slot = subst_entry_vacant_find(subst->alias); }

    if (slot == UINT32_MAX) { return LDG_ERR_FULL; }

    ret = subst_entry_put(&subst->alias[slot], line, name_len, line + text_start);
    if (ret != LDG_ERR_AOK) { return ret; }

    return crosstalk_re_repl_emit(repl, "alias saved: %.31s=%.255s", subst->alias[slot].name, subst->alias[slot].text);
}

static uint32_t subst_line_trim(crosstalk_re_repl_subst_t *subst, char *line, uint32_t cap, uint32_t *len)
{
    uint32_t start = subst_spaces_skip(line, *len, SUBST_LINE_ORIG);
    uint32_t end = *len;
    uint32_t ret = 0;

    while (end > start && ldg_char_space_is((uint8_t)line[end - (uint32_t)SUBST_STEP]) == LDG_TRUTH_TRUE) { end = end - (uint32_t)SUBST_STEP; }

    *len = end - start;

    if (*len != 0)
    {
        ret = ldg_mem_secure_cpy(subst->buff, line + start, (uint64_t)*len);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    subst->buff[*len] = LDG_STR_TERM;

    return ldg_str_rbrcpy(line, subst->buff, (uint64_t)cap);
}

static uint32_t subst_alias_resolve(crosstalk_re_repl_subst_t *subst, crosstalk_re_repl_ctx_t *repl, char *line, uint32_t cap, uint8_t *out_handled)
{
    uint64_t len = 0;
    uint32_t depth = 0;
    uint32_t slot = 0;
    uint32_t ret = 0;

    for (depth = 0; depth <= (uint32_t)CROSSTALK_RE_REPL_SUBST_CAP; depth = depth + (uint32_t)SUBST_STEP)
    {
        if (crosstalk_re_repl_cmd_eq_is(line, SUBST_WORD_ALIASES) == LDG_TRUTH_TRUE)
        {
            *out_handled = LDG_TRUTH_TRUE;
            return subst_list(repl, subst->alias, "no aliases defined; use name=command to create one");
        }

        if (crosstalk_re_repl_cmd_eq_is(line, SUBST_WORD_HELP) == LDG_TRUTH_TRUE) { return crosstalk_re_repl_emit(repl, "Aliases: name=command saves; name runs; name= removes; aliases lists. Variables: set name=value; $name or ${name} substitutes; vars lists; unset name removes. Values: decimal/hex numbers, booleans, nil, quoted strings. Substitution skips quoted text; $$ is a literal dollar. Up to 32 aliases and 32 variables, names up to 31 characters."); }

        ret = ldg_str_len_get(line, (uint64_t)cap, &len);
        if (ret != LDG_ERR_AOK) { return ret; }

        slot = subst_entry_find(subst->alias, line, (uint32_t)len);
        if (slot == UINT32_MAX) { return LDG_ERR_AOK; }

        if (depth == (uint32_t)CROSSTALK_RE_REPL_SUBST_CAP) { return LDG_ERR_INVALID; }

        ret = ldg_str_rbrcpy(line, subst->alias[slot].text, (uint64_t)cap);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return LDG_ERR_INVALID;
}

uint32_t crosstalk_re_repl_subst_init(crosstalk_re_repl_subst_t *subst)
{
    if (LDG_UNLIKELY(subst == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ldg_mem_secure_zero(subst, (uint64_t)sizeof(*subst));
}

uint32_t crosstalk_re_repl_subst_apply(crosstalk_re_repl_subst_t *subst, crosstalk_re_repl_ctx_t *repl, char *line, uint32_t cap, uint8_t *out_handled)
{
    uint64_t line_len = 0;
    uint32_t len = 0;
    uint32_t name_len = 0;
    uint32_t mark = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(subst == 0x0 || repl == 0x0 || line == 0x0 || out_handled == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_handled = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(cap == 0 || cap > (uint32_t)sizeof(subst->buff))) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_str_len_get(line, (uint64_t)cap, &line_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    len = (uint32_t)line_len;

    ret = subst_var_expand(subst, line, cap, &len);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = subst_line_trim(subst, line, cap, &len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (len == 0)
    {
        *out_handled = LDG_TRUTH_TRUE;
        return LDG_ERR_AOK;
    }

    ret = subst_var_cmd(subst, repl, line, len, out_handled);
    if (ret != LDG_ERR_AOK || *out_handled == LDG_TRUTH_TRUE) { return ret; }

    while (name_len < len && subst_name_ch_is(line[name_len], name_len == 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE) == LDG_TRUTH_TRUE) { name_len = name_len + (uint32_t)SUBST_STEP; }

    mark = subst_spaces_skip(line, len, name_len);

    if (name_len != 0 && mark < len && line[mark] == SUBST_CH_EQ && line[mark + (uint32_t)SUBST_STEP] != SUBST_CH_EQ)
    {
        *out_handled = LDG_TRUTH_TRUE;
        return subst_alias_set(subst, repl, line, name_len, mark, len);
    }

    return subst_alias_resolve(subst, repl, line, cap, out_handled);
}
