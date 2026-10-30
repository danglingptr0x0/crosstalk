#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/str/str.h>

#include "hdl/lex.h"
#include "hdl/preproc.h"

static uint32_t crosstalk_hdl_preproc_scan(crosstalk_hdl_preproc_ctx_t *ctx, const char *src, uint32_t len);

static uint8_t crosstalk_hdl_preproc_ident_start_is(char ch)
{
    if (ldg_char_alpha_is(ch) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    if (ch == '_') { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t crosstalk_hdl_preproc_ident_char_is(char ch)
{
    if (ldg_char_alnum_is(ch) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    if (ch == '_' || ch == '$') { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t crosstalk_hdl_preproc_active_is(const crosstalk_hdl_preproc_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_TRUTH_FALSE; }

    if (ctx->cond_depth == 0) { return LDG_TRUTH_TRUE; }

    if (ctx->cond[ctx->cond_depth - 0x1].live == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t crosstalk_hdl_preproc_word_is(const char *ptr, uint32_t len, const char *word)
{
    uint32_t word_len = 0;

    if (LDG_UNLIKELY(ptr == 0x0 || word == 0x0)) { return LDG_TRUTH_FALSE; }

    word_len = (uint32_t)strlen(word);

    if (len != word_len) { return LDG_TRUTH_FALSE; }

    if (memcmp(ptr, word, len) != 0) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static uint32_t crosstalk_hdl_preproc_out_wr(crosstalk_hdl_preproc_ctx_t *ctx, const char *ptr, uint32_t wr_len)
{
    if (LDG_UNLIKELY(ctx == 0x0 || ptr == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (wr_len == 0) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(ctx->out_len + wr_len > ctx->out_cap)) { return LDG_ERR_FULL; }

    memcpy(ctx->out + ctx->out_len, ptr, wr_len);
    ctx->out_len = ctx->out_len + wr_len;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_preproc_out_ch_wr(crosstalk_hdl_preproc_ctx_t *ctx, char ch)
{
    return crosstalk_hdl_preproc_out_wr(ctx, &ch, 0x1);
}

static void crosstalk_hdl_preproc_space_skip(const char *src, uint32_t len, uint32_t *pos)
{
    while (*pos < len)
    {
        char ch = src[*pos];

        if (ch != ' ' && ch != '\t' && ch != '\r') { break; }

        *pos = *pos + 0x1;
    }
}

static uint32_t crosstalk_hdl_preproc_name_scan(const char *src, uint32_t len, uint32_t *pos, const char **out_ptr, uint32_t *out_len)
{
    uint32_t start = 0;

    if (LDG_UNLIKELY(src == 0x0 || pos == 0x0 || out_ptr == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_ptr = 0x0;
    *out_len = 0;

    crosstalk_hdl_preproc_space_skip(src, len, pos);

    if (*pos >= len) { return CROSSTALK_HDL_ERR_PREPROC_MACRO; }

    if (crosstalk_hdl_preproc_ident_start_is(src[*pos]) != LDG_TRUTH_TRUE) { return CROSSTALK_HDL_ERR_PREPROC_MACRO; }

    start = *pos;

    while (*pos < len && crosstalk_hdl_preproc_ident_char_is(src[*pos]) == LDG_TRUTH_TRUE) { *pos = *pos + 0x1; }

    *out_ptr = src + start;
    *out_len = *pos - start;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_preproc_macro_idx_get(const crosstalk_hdl_preproc_ctx_t *ctx, const char *name, uint32_t name_len)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0)) { return UINT32_MAX; }

    for (idx = 0; idx < ctx->macro_cunt; idx++)
    {
        if (ctx->macros[idx].name_len != name_len) { continue; }

        if (memcmp(ctx->macros[idx].name, name, name_len) == 0) { return idx; }
    }

    return UINT32_MAX;
}

static uint32_t crosstalk_hdl_preproc_line_skip(crosstalk_hdl_preproc_ctx_t *ctx, const char *src, uint32_t len, uint32_t *pos)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || src == 0x0 || pos == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (*pos < len)
    {
        char ch = src[*pos];

        if (ch == '\\' && *pos + 0x1 < len && src[*pos + 0x1] == '\n')
        {
            ret = crosstalk_hdl_preproc_out_ch_wr(ctx, '\n');
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            *pos = *pos + 0x2;
            continue;
        }

        if (ch == '\n') { break; }

        *pos = *pos + 0x1;
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_preproc_cond_push(crosstalk_hdl_preproc_ctx_t *ctx, uint8_t cond_val)
{
    crosstalk_hdl_preproc_cond_t *frame = 0x0;
    uint8_t parent = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->cond_depth >= CROSSTALK_HDL_PREPROC_COND_MAX) { return CROSSTALK_HDL_ERR_PREPROC_DEPTH; }

    parent = crosstalk_hdl_preproc_active_is(ctx);
    frame = &ctx->cond[ctx->cond_depth];

    frame->parent_live = parent;
    frame->live = LDG_TRUTH_FALSE;
    frame->taken = cond_val;
    frame->else_seen = LDG_TRUTH_FALSE;

    if (parent == LDG_TRUTH_TRUE && cond_val == LDG_TRUTH_TRUE) { frame->live = LDG_TRUTH_TRUE; }

    ctx->cond_depth = ctx->cond_depth + 0x1;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_preproc_cond_elsif(crosstalk_hdl_preproc_ctx_t *ctx, uint8_t cond_val)
{
    crosstalk_hdl_preproc_cond_t *frame = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->cond_depth == 0) { return CROSSTALK_HDL_ERR_PREPROC_COND; }

    frame = &ctx->cond[ctx->cond_depth - 0x1];

    if (frame->else_seen == LDG_TRUTH_TRUE) { return CROSSTALK_HDL_ERR_PREPROC_COND; }

    frame->live = LDG_TRUTH_FALSE;

    if (frame->parent_live == LDG_TRUTH_TRUE && frame->taken != LDG_TRUTH_TRUE && cond_val == LDG_TRUTH_TRUE) { frame->live = LDG_TRUTH_TRUE; }

    if (cond_val == LDG_TRUTH_TRUE) { frame->taken = LDG_TRUTH_TRUE; }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_preproc_cond_else(crosstalk_hdl_preproc_ctx_t *ctx)
{
    crosstalk_hdl_preproc_cond_t *frame = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->cond_depth == 0) { return CROSSTALK_HDL_ERR_PREPROC_COND; }

    frame = &ctx->cond[ctx->cond_depth - 0x1];

    if (frame->else_seen == LDG_TRUTH_TRUE) { return CROSSTALK_HDL_ERR_PREPROC_COND; }

    frame->else_seen = LDG_TRUTH_TRUE;
    frame->live = LDG_TRUTH_FALSE;

    if (frame->parent_live == LDG_TRUTH_TRUE && frame->taken != LDG_TRUTH_TRUE) { frame->live = LDG_TRUTH_TRUE; }

    frame->taken = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_preproc_cond_pop(crosstalk_hdl_preproc_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->cond_depth == 0) { return CROSSTALK_HDL_ERR_PREPROC_COND; }

    ctx->cond_depth = ctx->cond_depth - 0x1;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_preproc_define_rd(crosstalk_hdl_preproc_ctx_t *ctx, const char *src, uint32_t len, uint32_t *pos)
{
    crosstalk_hdl_preproc_macro_t *macro = 0x0;
    const char *name = 0x0;
    const char *param_ptr = 0x0;
    char *body_copy = 0x0;
    void *raw = 0x0;
    uint32_t name_len = 0;
    uint32_t param_len = 0;
    uint32_t idx = 0;
    uint32_t body_start = 0;
    uint32_t body_end = 0;
    uint32_t walk = 0;
    uint32_t fill = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || src == 0x0 || pos == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_preproc_name_scan(src, len, pos, &name, &name_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    idx = crosstalk_hdl_preproc_macro_idx_get(ctx, name, name_len);

    if (idx == UINT32_MAX)
    {
        if (ctx->macro_cunt >= CROSSTALK_HDL_PREPROC_MACRO_MAX) { return LDG_ERR_FULL; }

        idx = ctx->macro_cunt;
        ctx->macro_cunt = ctx->macro_cunt + 0x1;
    }

    macro = &ctx->macros[idx];
    macro->name = name;
    macro->name_len = name_len;
    macro->body = 0x0;
    macro->body_len = 0;
    macro->param_cunt = 0;
    macro->func_is = LDG_TRUTH_FALSE;

    if (*pos < len && src[*pos] == '(')
    {
        macro->func_is = LDG_TRUTH_TRUE;
        *pos = *pos + 0x1;

        crosstalk_hdl_preproc_space_skip(src, len, pos);

        if (*pos < len && src[*pos] == ')') { *pos = *pos + 0x1; }
        else
        {
            while (*pos < len)
            {
                ret = crosstalk_hdl_preproc_name_scan(src, len, pos, &param_ptr, &param_len);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                if (macro->param_cunt >= CROSSTALK_HDL_PREPROC_PARAM_MAX) { return LDG_ERR_FULL; }

                macro->param[macro->param_cunt] = param_ptr;
                macro->param_len[macro->param_cunt] = param_len;
                macro->param_cunt = macro->param_cunt + 0x1;

                crosstalk_hdl_preproc_space_skip(src, len, pos);

                if (*pos < len && src[*pos] == ',')
                {
                    *pos = *pos + 0x1;
                    continue;
                }

                if (*pos < len && src[*pos] == ')')
                {
                    *pos = *pos + 0x1;
                    break;
                }

                return CROSSTALK_HDL_ERR_PREPROC_MACRO;
            }
        }
    }

    crosstalk_hdl_preproc_space_skip(src, len, pos);
    body_start = *pos;
    walk = body_start;

    while (walk < len)
    {
        if (src[walk] == '\\' && walk + 0x1 < len && src[walk + 0x1] == '\n')
        {
            walk = walk + 0x2;
            continue;
        }

        if (src[walk] == '\n') { break; }

        if (src[walk] == '/' && walk + 0x1 < len && src[walk + 0x1] == '/') { break; }

        walk = walk + 0x1;
    }

    body_end = walk;

    if (body_end > body_start)
    {
        ret = ldg_mem_pool_alloc(ctx->pool, body_end - body_start, &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        body_copy = (char *)raw;
        fill = 0;
        walk = body_start;

        while (walk < body_end)
        {
            if (src[walk] == '\\' && walk + 0x1 < len && src[walk + 0x1] == '\n')
            {
                body_copy[fill] = ' ';
                fill = fill + 0x1;
                walk = walk + 0x2;

                ret = crosstalk_hdl_preproc_out_ch_wr(ctx, '\n');
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                continue;
            }

            body_copy[fill] = src[walk];
            fill = fill + 0x1;
            walk = walk + 0x1;
        }

        while (fill > 0)
        {
            char tail = body_copy[fill - 0x1];

            if (tail != ' ' && tail != '\t' && tail != '\r') { break; }

            fill = fill - 0x1;
        }

        macro->body = body_copy;
        macro->body_len = fill;
    }

    *pos = body_end;

    while (*pos < len && src[*pos] != '\n') { *pos = *pos + 0x1; }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_preproc_include_rd(crosstalk_hdl_preproc_ctx_t *ctx, const char *src, uint32_t len, uint32_t *pos)
{
    const char *name = 0x0;
    char *text = 0x0;
    char close_ch = '"';
    uint32_t name_len = 0;
    uint32_t start = 0;
    uint32_t text_len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || src == 0x0 || pos == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    crosstalk_hdl_preproc_space_skip(src, len, pos);

    if (*pos >= len) { return CROSSTALK_HDL_ERR_PREPROC_INCLUDE; }

    if (src[*pos] == '<') { close_ch = '>'; }
    else if (src[*pos] != '"') { return CROSSTALK_HDL_ERR_PREPROC_INCLUDE; }

    *pos = *pos + 0x1;
    start = *pos;

    while (*pos < len && src[*pos] != close_ch && src[*pos] != '\n') { *pos = *pos + 0x1; }

    if (*pos >= len || src[*pos] != close_ch) { return CROSSTALK_HDL_ERR_PREPROC_INCLUDE; }

    name = src + start;
    name_len = *pos - start;
    *pos = *pos + 0x1;

    if (ctx->include_cb == 0x0) { return CROSSTALK_HDL_ERR_PREPROC_INCLUDE; }

    if (ctx->include_depth >= CROSSTALK_HDL_PREPROC_INCLUDE_MAX) { return CROSSTALK_HDL_ERR_PREPROC_DEPTH; }

    ret = ctx->include_cb(ctx->include_user, name, name_len, &text, &text_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(text == 0x0)) { return CROSSTALK_HDL_ERR_PREPROC_INCLUDE; }

    ctx->include_depth = ctx->include_depth + 0x1;
    ret = crosstalk_hdl_preproc_scan(ctx, text, text_len);
    ctx->include_depth = ctx->include_depth - 0x1;

    return ret;
}

static uint32_t crosstalk_hdl_preproc_expand(crosstalk_hdl_preproc_ctx_t *ctx, const char *src, uint32_t len, uint32_t *pos, uint32_t macro_idx)
{
    crosstalk_hdl_preproc_macro_t macro;
    const char *arg[CROSSTALK_HDL_PREPROC_PARAM_MAX];
    uint32_t arg_len[CROSSTALK_HDL_PREPROC_PARAM_MAX];
    char *buff = 0x0;
    void *raw = 0x0;
    uint32_t arg_cunt = 0;
    uint32_t nest = 0;
    uint32_t start = 0;
    uint32_t end = 0;
    uint32_t walk = 0;
    uint32_t fill = 0;
    uint32_t need = 0;
    uint32_t ident_len = 0;
    uint32_t param_idx = 0;
    uint32_t hit = 0;
    uint32_t ret = 0;
    uint8_t done = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0 || src == 0x0 || pos == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->expand_depth >= CROSSTALK_HDL_PREPROC_EXPAND_MAX) { return CROSSTALK_HDL_ERR_PREPROC_DEPTH; }

    macro = ctx->macros[macro_idx];

    if (macro.func_is == LDG_TRUTH_TRUE)
    {
        crosstalk_hdl_preproc_space_skip(src, len, pos);

        if (*pos >= len || src[*pos] != '(') { return CROSSTALK_HDL_ERR_PREPROC_MACRO; }

        *pos = *pos + 0x1;
        start = *pos;

        while (*pos < len && done != LDG_TRUTH_TRUE)
        {
            char ch = src[*pos];

            if (ch == '"')
            {
                *pos = *pos + 0x1;

                while (*pos < len)
                {
                    if (src[*pos] == '\\' && *pos + 0x1 < len)
                    {
                        *pos = *pos + 0x2;
                        continue;
                    }

                    if (src[*pos] == '"')
                    {
                        *pos = *pos + 0x1;
                        break;
                    }

                    *pos = *pos + 0x1;
                }

                continue;
            }

            if (ch == '(' || ch == '[' || ch == '{')
            {
                nest = nest + 0x1;
                *pos = *pos + 0x1;
                continue;
            }

            if (ch == ']' || ch == '}')
            {
                if (nest > 0) { nest = nest - 0x1; }

                *pos = *pos + 0x1;
                continue;
            }

            if (ch == ')' && nest > 0)
            {
                nest = nest - 0x1;
                *pos = *pos + 0x1;
                continue;
            }

            if (ch == '\n')
            {
                ret = crosstalk_hdl_preproc_out_ch_wr(ctx, '\n');
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                *pos = *pos + 0x1;
                continue;
            }

            if ((ch == ',' && nest == 0) || ch == ')')
            {
                end = *pos;

                while (start < end && (src[start] == ' ' || src[start] == '\t' || src[start] == '\r' || src[start] == '\n')) { start = start + 0x1; }

                while (end > start && (src[end - 0x1] == ' ' || src[end - 0x1] == '\t' || src[end - 0x1] == '\r' || src[end - 0x1] == '\n')) { end = end - 0x1; }

                if (arg_cunt >= CROSSTALK_HDL_PREPROC_PARAM_MAX) { return LDG_ERR_FULL; }

                arg[arg_cunt] = src + start;
                arg_len[arg_cunt] = end - start;
                arg_cunt = arg_cunt + 0x1;

                if (ch == ')') { done = LDG_TRUTH_TRUE; }

                *pos = *pos + 0x1;
                start = *pos;
                continue;
            }

            *pos = *pos + 0x1;
        }

        if (done != LDG_TRUTH_TRUE) { return CROSSTALK_HDL_ERR_PREPROC_MACRO; }

        if (macro.param_cunt == 0 && arg_cunt == 0x1 && arg_len[0] == 0) { arg_cunt = 0; }

        if (arg_cunt != macro.param_cunt) { return CROSSTALK_HDL_ERR_PREPROC_MACRO; }
    }

    if (macro.body_len == 0) { return LDG_ERR_AOK; }

    if (macro.param_cunt == 0)
    {
        ctx->expand_depth = ctx->expand_depth + 0x1;
        ret = crosstalk_hdl_preproc_scan(ctx, macro.body, macro.body_len);
        ctx->expand_depth = ctx->expand_depth - 0x1;

        return ret;
    }

    need = 0;
    walk = 0;

    while (walk < macro.body_len)
    {
        if (crosstalk_hdl_preproc_ident_start_is(macro.body[walk]) == LDG_TRUTH_TRUE)
        {
            start = walk;

            while (walk < macro.body_len && crosstalk_hdl_preproc_ident_char_is(macro.body[walk]) == LDG_TRUTH_TRUE) { walk = walk + 0x1; }

            ident_len = walk - start;
            hit = UINT32_MAX;

            for (param_idx = 0; param_idx < macro.param_cunt; param_idx++)
            {
                if (macro.param_len[param_idx] != ident_len) { continue; }

                if (memcmp(macro.body + start, macro.param[param_idx], ident_len) == 0)
                {
                    hit = param_idx;
                    break;
                }
            }

            if (hit != UINT32_MAX) { need = need + arg_len[hit]; }
            else { need = need + ident_len; }

            continue;
        }

        need = need + 0x1;
        walk = walk + 0x1;
    }

    if (need == 0) { return LDG_ERR_AOK; }

    ret = ldg_mem_pool_alloc(ctx->pool, need, &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    buff = (char *)raw;
    fill = 0;
    walk = 0;

    while (walk < macro.body_len)
    {
        if (crosstalk_hdl_preproc_ident_start_is(macro.body[walk]) == LDG_TRUTH_TRUE)
        {
            start = walk;

            while (walk < macro.body_len && crosstalk_hdl_preproc_ident_char_is(macro.body[walk]) == LDG_TRUTH_TRUE) { walk = walk + 0x1; }

            ident_len = walk - start;
            hit = UINT32_MAX;

            for (param_idx = 0; param_idx < macro.param_cunt; param_idx++)
            {
                if (macro.param_len[param_idx] != ident_len) { continue; }

                if (memcmp(macro.body + start, macro.param[param_idx], ident_len) == 0)
                {
                    hit = param_idx;
                    break;
                }
            }

            if (hit != UINT32_MAX)
            {
                memcpy(buff + fill, arg[hit], arg_len[hit]);
                fill = fill + arg_len[hit];
            }
            else
            {
                memcpy(buff + fill, macro.body + start, ident_len);
                fill = fill + ident_len;
            }

            continue;
        }

        buff[fill] = macro.body[walk];
        fill = fill + 0x1;
        walk = walk + 0x1;
    }

    ctx->expand_depth = ctx->expand_depth + 0x1;
    ret = crosstalk_hdl_preproc_scan(ctx, buff, fill);
    ctx->expand_depth = ctx->expand_depth - 0x1;

    return ret;
}

static uint32_t crosstalk_hdl_preproc_directive(crosstalk_hdl_preproc_ctx_t *ctx, const char *src, uint32_t len, uint32_t *pos)
{
    const char *name = 0x0;
    const char *sub_name = 0x0;
    uint32_t name_len = 0;
    uint32_t sub_len = 0;
    uint32_t start = 0;
    uint32_t macro_idx = 0;
    uint32_t ret = 0;
    uint8_t live = LDG_TRUTH_FALSE;
    uint8_t cond_val = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0 || src == 0x0 || pos == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *pos = *pos + 0x1;

    if (*pos >= len || crosstalk_hdl_preproc_ident_start_is(src[*pos]) != LDG_TRUTH_TRUE) { return CROSSTALK_HDL_ERR_PREPROC_DIRECTIVE; }

    start = *pos;

    while (*pos < len && crosstalk_hdl_preproc_ident_char_is(src[*pos]) == LDG_TRUTH_TRUE) { *pos = *pos + 0x1; }

    name = src + start;
    name_len = *pos - start;
    live = crosstalk_hdl_preproc_active_is(ctx);

    if (crosstalk_hdl_preproc_word_is(name, name_len, "ifdef") == LDG_TRUTH_TRUE || crosstalk_hdl_preproc_word_is(name, name_len, "ifndef") == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_preproc_name_scan(src, len, pos, &sub_name, &sub_len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        cond_val = LDG_TRUTH_FALSE;

        if (crosstalk_hdl_preproc_macro_idx_get(ctx, sub_name, sub_len) != UINT32_MAX) { cond_val = LDG_TRUTH_TRUE; }

        if (name_len == 0x6) { cond_val = (cond_val == LDG_TRUTH_TRUE) ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE; }

        return crosstalk_hdl_preproc_cond_push(ctx, cond_val);
    }

    if (crosstalk_hdl_preproc_word_is(name, name_len, "elsif") == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_preproc_name_scan(src, len, pos, &sub_name, &sub_len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        cond_val = LDG_TRUTH_FALSE;

        if (crosstalk_hdl_preproc_macro_idx_get(ctx, sub_name, sub_len) != UINT32_MAX) { cond_val = LDG_TRUTH_TRUE; }

        return crosstalk_hdl_preproc_cond_elsif(ctx, cond_val);
    }

    if (crosstalk_hdl_preproc_word_is(name, name_len, "else") == LDG_TRUTH_TRUE) { return crosstalk_hdl_preproc_cond_else(ctx); }

    if (crosstalk_hdl_preproc_word_is(name, name_len, "endif") == LDG_TRUTH_TRUE) { return crosstalk_hdl_preproc_cond_pop(ctx); }

    if (crosstalk_hdl_preproc_word_is(name, name_len, "define") == LDG_TRUTH_TRUE)
    {
        if (live != LDG_TRUTH_TRUE) { return crosstalk_hdl_preproc_line_skip(ctx, src, len, pos); }

        return crosstalk_hdl_preproc_define_rd(ctx, src, len, pos);
    }

    if (crosstalk_hdl_preproc_word_is(name, name_len, "undef") == LDG_TRUTH_TRUE)
    {
        if (live != LDG_TRUTH_TRUE) { return crosstalk_hdl_preproc_line_skip(ctx, src, len, pos); }

        ret = crosstalk_hdl_preproc_name_scan(src, len, pos, &sub_name, &sub_len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        macro_idx = crosstalk_hdl_preproc_macro_idx_get(ctx, sub_name, sub_len);

        if (macro_idx != UINT32_MAX)
        {
            ctx->macros[macro_idx] = ctx->macros[ctx->macro_cunt - 0x1];
            ctx->macro_cunt = ctx->macro_cunt - 0x1;
        }

        return LDG_ERR_AOK;
    }

    if (crosstalk_hdl_preproc_word_is(name, name_len, "include") == LDG_TRUTH_TRUE)
    {
        if (live != LDG_TRUTH_TRUE) { return crosstalk_hdl_preproc_line_skip(ctx, src, len, pos); }

        return crosstalk_hdl_preproc_include_rd(ctx, src, len, pos);
    }

    if (crosstalk_hdl_preproc_word_is(name, name_len, "timescale") == LDG_TRUTH_TRUE || crosstalk_hdl_preproc_word_is(name, name_len, "default_nettype") == LDG_TRUTH_TRUE || crosstalk_hdl_preproc_word_is(name, name_len, "resetall") == LDG_TRUTH_TRUE) { return crosstalk_hdl_preproc_line_skip(ctx, src, len, pos); }

    if (live != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    macro_idx = crosstalk_hdl_preproc_macro_idx_get(ctx, name, name_len);

    if (macro_idx == UINT32_MAX)
    {
        ctx->err_name = name;
        ctx->err_name_len = name_len;
        return CROSSTALK_HDL_ERR_PREPROC_DIRECTIVE;
    }

    return crosstalk_hdl_preproc_expand(ctx, src, len, pos, macro_idx);
}

static uint32_t crosstalk_hdl_preproc_scan(crosstalk_hdl_preproc_ctx_t *ctx, const char *src, uint32_t len)
{
    uint32_t pos = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || src == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (pos < len)
    {
        char ch = src[pos];

        if (ch == '/' && pos + 0x1 < len && src[pos + 0x1] == '/')
        {
            while (pos < len && src[pos] != '\n') { pos = pos + 0x1; }

            continue;
        }

        if (ch == '/' && pos + 0x1 < len && src[pos + 0x1] == '*')
        {
            uint8_t term_is = LDG_TRUTH_FALSE;

            pos = pos + 0x2;

            while (pos < len)
            {
                if (src[pos] == '*' && pos + 0x1 < len && src[pos + 0x1] == '/')
                {
                    pos = pos + 0x2;
                    term_is = LDG_TRUTH_TRUE;
                    break;
                }

                if (src[pos] == '\n')
                {
                    ret = crosstalk_hdl_preproc_out_ch_wr(ctx, '\n');
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
                }

                pos = pos + 0x1;
            }

            if (term_is != LDG_TRUTH_TRUE) { return CROSSTALK_HDL_ERR_LEX_COMMENT_UNTERM; }

            continue;
        }

        if (ch == '"')
        {
            uint32_t start = pos;
            uint8_t term_is = LDG_TRUTH_FALSE;

            pos = pos + 0x1;

            while (pos < len)
            {
                if (src[pos] == '\\' && pos + 0x1 < len)
                {
                    pos = pos + 0x2;
                    continue;
                }

                if (src[pos] == '"')
                {
                    pos = pos + 0x1;
                    term_is = LDG_TRUTH_TRUE;
                    break;
                }

                pos = pos + 0x1;
            }

            if (term_is != LDG_TRUTH_TRUE) { return CROSSTALK_HDL_ERR_LEX_STR_UNTERM; }

            if (crosstalk_hdl_preproc_active_is(ctx) == LDG_TRUTH_TRUE)
            {
                ret = crosstalk_hdl_preproc_out_wr(ctx, src + start, pos - start);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            continue;
        }

        if (ch == '`')
        {
            ret = crosstalk_hdl_preproc_directive(ctx, src, len, &pos);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            continue;
        }

        if (crosstalk_hdl_preproc_ident_start_is(ch) == LDG_TRUTH_TRUE)
        {
            uint32_t start = pos;

            pos = pos + 0x1;

            while (pos < len && crosstalk_hdl_preproc_ident_char_is(src[pos]) == LDG_TRUTH_TRUE) { pos = pos + 0x1; }

            if (crosstalk_hdl_preproc_active_is(ctx) == LDG_TRUTH_TRUE)
            {
                ret = crosstalk_hdl_preproc_out_wr(ctx, src + start, pos - start);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            continue;
        }

        if (ch == '\n')
        {
            ret = crosstalk_hdl_preproc_out_ch_wr(ctx, '\n');
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            pos = pos + 0x1;
            continue;
        }

        if (crosstalk_hdl_preproc_active_is(ctx) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_hdl_preproc_out_ch_wr(ctx, ch);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        pos = pos + 0x1;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_preproc_init(crosstalk_hdl_preproc_ctx_t *ctx, void *pool, crosstalk_hdl_preproc_include_cb_t include_cb, void *include_user)
{
    void *raw = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || pool == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ret = ldg_mem_pool_alloc(pool, CROSSTALK_HDL_PREPROC_MACRO_MAX * sizeof(crosstalk_hdl_preproc_macro_t), &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->pool = pool;
    ctx->macros = (crosstalk_hdl_preproc_macro_t *)raw;
    ctx->include_cb = include_cb;
    ctx->include_user = include_user;

    return crosstalk_hdl_preproc_define_set(ctx, "CROSSTALK_HDL", 0xD, "\x31", 0x1);
}

uint32_t crosstalk_hdl_preproc_define_set(crosstalk_hdl_preproc_ctx_t *ctx, const char *name, uint32_t name_len, const char *body, uint32_t body_len)
{
    crosstalk_hdl_preproc_macro_t *macro = 0x0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    idx = crosstalk_hdl_preproc_macro_idx_get(ctx, name, name_len);

    if (idx == UINT32_MAX)
    {
        if (ctx->macro_cunt >= CROSSTALK_HDL_PREPROC_MACRO_MAX) { return LDG_ERR_FULL; }

        idx = ctx->macro_cunt;
        ctx->macro_cunt = ctx->macro_cunt + 0x1;
    }

    macro = &ctx->macros[idx];
    macro->name = name;
    macro->name_len = name_len;
    macro->body = body;
    macro->body_len = body_len;
    macro->param_cunt = 0;
    macro->func_is = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_preproc_run(crosstalk_hdl_preproc_ctx_t *ctx, const char *src, uint32_t len, char **out_src, uint32_t *out_len)
{
    void *raw = 0x0;
    uint64_t cap = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || src == 0x0 || out_src == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_src = 0x0;
    *out_len = 0;

    cap = (uint64_t)len * 0x2 + CROSSTALK_HDL_PREPROC_OUT_SLACK;

    if (LDG_UNLIKELY(cap > UINT32_MAX)) { return LDG_ERR_OVERFLOW; }

    ret = ldg_mem_pool_alloc(ctx->pool, cap, &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->out = (char *)raw;
    ctx->out_len = 0;
    ctx->out_cap = (uint32_t)cap - LDG_STR_TERM_SIZE;

    ret = crosstalk_hdl_preproc_scan(ctx, src, len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->cond_depth != 0) { return CROSSTALK_HDL_ERR_PREPROC_COND; }

    ctx->out[ctx->out_len] = LDG_STR_TERM;

    *out_src = ctx->out;
    *out_len = ctx->out_len;

    return LDG_ERR_AOK;
}
