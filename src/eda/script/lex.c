#include <stdint.h>
#include <string.h>

#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "eda/script/lex.h"

#define LEX_STEP 1
#define LEX_PAIR 2
#define LEX_TRIPLE 3
#define LEX_HERE 0
#define LEX_TICK '\''
#define LEX_ESC '\\'
#define LEX_ASCII_ONE '1'
#define LEX_ASCII_NINE '9'
#define LEX_CHAR_WIDTH 8

typedef struct lex_kw
{
    const char *word;
    uint32_t len;
    uint32_t kind;
} lex_kw_t;

static uint8_t lex_digit_is(char ch)
{
    return (ch >= LDG_ASCII_DIGIT_ZERO && ch <= LEX_ASCII_NINE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint8_t lex_hex_digit_is(char ch)
{
    if (lex_digit_is(ch) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    if (ch >= 'a' && ch <= 'f') { return LDG_TRUTH_TRUE; }

    if (ch >= 'A' && ch <= 'F') { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint64_t lex_hex_digit_val(char ch)
{
    if (lex_digit_is(ch) == LDG_TRUTH_TRUE) { return (uint64_t)(ch - LDG_ASCII_DIGIT_ZERO); }

    if (ch >= 'a' && ch <= 'f') { return (uint64_t)(ch - 'a') + (uint64_t)CROSSTALK_EDA_SCRIPT_HEX_ALPHA_BASE; }

    return (uint64_t)(ch - 'A') + (uint64_t)CROSSTALK_EDA_SCRIPT_HEX_ALPHA_BASE;
}

static uint8_t lex_ident_start_is(char ch)
{
    if (ch >= 'a' && ch <= 'z') { return LDG_TRUTH_TRUE; }

    if (ch >= 'A' && ch <= 'Z') { return LDG_TRUTH_TRUE; }

    if (ch == '_') { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t lex_ident_cont_is(char ch)
{
    if (lex_ident_start_is(ch) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return lex_digit_is(ch);
}

static uint32_t lex_kw_find(const char *start, uint32_t len, uint32_t *out)
{
    static const lex_kw_t kw_tbl[] = {
        { "init", 4, CROSSTALK_EDA_SCRIPT_TOK_KW_INIT },
        { "iff", 3, CROSSTALK_EDA_SCRIPT_TOK_KW_IFF },
        { "ret", 3, CROSSTALK_EDA_SCRIPT_TOK_KW_RET },
        { "call", 4, CROSSTALK_EDA_SCRIPT_TOK_KW_CALL },
        { "jmp", 3, CROSSTALK_EDA_SCRIPT_TOK_KW_JMP },
        { "const", 5, CROSSTALK_EDA_SCRIPT_TOK_KW_CONST },
        { "AND", 3, CROSSTALK_EDA_SCRIPT_TOK_KW_AND },
        { "OR", 2, CROSSTALK_EDA_SCRIPT_TOK_KW_OR },
        { "XOR", 3, CROSSTALK_EDA_SCRIPT_TOK_KW_XOR },
        { "NOT", 3, CROSSTALK_EDA_SCRIPT_TOK_KW_NOT },
        { "SHL", 3, CROSSTALK_EDA_SCRIPT_TOK_KW_SHL },
        { "SHR", 3, CROSSTALK_EDA_SCRIPT_TOK_KW_SHR },
        { "rack", 4, CROSSTALK_EDA_SCRIPT_TOK_KW_RACK },
        { "lever", 5, CROSSTALK_EDA_SCRIPT_TOK_KW_LEVER },
        { "rotary", 6, CROSSTALK_EDA_SCRIPT_TOK_KW_ROTARY },
        { "encoder", 7, CROSSTALK_EDA_SCRIPT_TOK_KW_ENCODER },
        { "button", 6, CROSSTALK_EDA_SCRIPT_TOK_KW_BUTTON }
    };
    uint32_t idx = 0;

    for (idx = 0; idx < sizeof(kw_tbl) / sizeof(kw_tbl[0]); idx = idx + (uint32_t)LEX_STEP)
    {
        if (kw_tbl[idx].len != len) { continue; }

        if (memcmp(kw_tbl[idx].word, start, len) != 0) { continue; }

        *out = kw_tbl[idx].kind;

        return LDG_TRUTH_TRUE;
    }

    return LDG_TRUTH_FALSE;
}

uint8_t crosstalk_eda_script_lex_width_valid_is(uint64_t width)
{
    if (width == 0) { return LDG_TRUTH_FALSE; }

    if (width > (uint64_t)CROSSTALK_EDA_SCRIPT_WIDTH_MAX) { return LDG_TRUTH_FALSE; }

    return LDG_IS_POW2(width) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static char lex_at(const crosstalk_eda_script_lex_t *ctx, uint32_t off)
{
    if (ctx->at + off >= ctx->src_len) { return LDG_STR_TERM; }

    return ctx->src[ctx->at + off];
}

static void lex_advance(crosstalk_eda_script_lex_t *ctx, uint32_t cunt)
{
    uint32_t idx = 0;

    for (idx = 0; idx < cunt && ctx->at < ctx->src_len; idx = idx + (uint32_t)LEX_STEP)
    {
        if (ctx->src[ctx->at] == '\n')
        {
            ctx->line = ctx->line + (uint32_t)LEX_STEP;
            ctx->col = (uint32_t)LEX_STEP;
        }
        else { ctx->col = ctx->col + (uint32_t)LEX_STEP; }

        ctx->at = ctx->at + (uint32_t)LEX_STEP;
    }
}

static void lex_ws_skip(crosstalk_eda_script_lex_t *ctx)
{
    while (ctx->at < ctx->src_len)
    {
        char ch = ctx->src[ctx->at];

        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
        {
            lex_advance(ctx, (uint32_t)LEX_STEP);
            continue;
        }

        if (ch == '/' && lex_at(ctx, (uint32_t)LEX_STEP) == '/')
        {
            while (ctx->at < ctx->src_len && ctx->src[ctx->at] != '\n') { lex_advance(ctx, (uint32_t)LEX_STEP); }

            continue;
        }

        break;
    }
}

static uint32_t lex_num_body(crosstalk_eda_script_lex_t *ctx, uint64_t max_val, uint64_t *out)
{
    uint64_t val = 0;
    uint64_t radix = (uint64_t)CROSSTALK_EDA_SCRIPT_RADIX_DEC;
    uint32_t digits = 0;

    if (lex_at(ctx, (uint32_t)LEX_HERE) == LDG_ASCII_DIGIT_ZERO && (lex_at(ctx, (uint32_t)LEX_STEP) == 'x' || lex_at(ctx, (uint32_t)LEX_STEP) == 'X'))
    {
        radix = (uint64_t)CROSSTALK_EDA_SCRIPT_RADIX_HEX;
        lex_advance(ctx, (uint32_t)LEX_PAIR);
    }
    else if (lex_at(ctx, (uint32_t)LEX_HERE) == LDG_ASCII_DIGIT_ZERO && (lex_at(ctx, (uint32_t)LEX_STEP) == 'b' || lex_at(ctx, (uint32_t)LEX_STEP) == 'B'))
    {
        radix = (uint64_t)CROSSTALK_EDA_SCRIPT_RADIX_BIN;
        lex_advance(ctx, (uint32_t)LEX_PAIR);
    }

    while (ctx->at < ctx->src_len)
    {
        char ch = lex_at(ctx, (uint32_t)LEX_HERE);
        uint64_t dig = 0;

        if (radix == (uint64_t)CROSSTALK_EDA_SCRIPT_RADIX_HEX)
        {
            if (lex_hex_digit_is(ch) != LDG_TRUTH_TRUE) { break; }

            dig = lex_hex_digit_val(ch);
        }
        else if (radix == (uint64_t)CROSSTALK_EDA_SCRIPT_RADIX_BIN)
        {
            if (ch != LDG_ASCII_DIGIT_ZERO && ch != LEX_ASCII_ONE) { break; }

            dig = (uint64_t)(ch - LDG_ASCII_DIGIT_ZERO);
        }
        else
        {
            if (lex_digit_is(ch) != LDG_TRUTH_TRUE) { break; }

            dig = (uint64_t)(ch - LDG_ASCII_DIGIT_ZERO);
        }

        if (LDG_UNLIKELY(val > max_val / radix)) { return CROSSTALK_EDA_SCRIPT_ERR_LEX; }

        val = val * radix;

        if (LDG_UNLIKELY(val > max_val - dig)) { return CROSSTALK_EDA_SCRIPT_ERR_LEX; }

        val = val + dig;
        digits = digits + (uint32_t)LEX_STEP;
        lex_advance(ctx, (uint32_t)LEX_STEP);
    }

    if (LDG_UNLIKELY(digits == 0)) { return CROSSTALK_EDA_SCRIPT_ERR_LEX; }

    *out = val;

    return LDG_ERR_AOK;
}

static uint32_t lex_esc_decode(char esc, char *out)
{
    if (esc == 'n') { *out = '\n'; return LDG_ERR_AOK; }

    if (esc == 't') { *out = '\t'; return LDG_ERR_AOK; }

    if (esc == 'r') { *out = '\r'; return LDG_ERR_AOK; }

    if (esc == LEX_ESC) { *out = LEX_ESC; return LDG_ERR_AOK; }

    if (esc == LDG_ASCII_QUOTE) { *out = LDG_ASCII_QUOTE; return LDG_ERR_AOK; }

    if (esc == LEX_TICK) { *out = LEX_TICK; return LDG_ERR_AOK; }

    if (esc == LDG_ASCII_DIGIT_ZERO) { *out = LDG_STR_TERM; return LDG_ERR_AOK; }

    return CROSSTALK_EDA_SCRIPT_ERR_LEX;
}

uint32_t crosstalk_eda_script_lex_init(crosstalk_eda_script_lex_t *ctx, const char *src, uint32_t src_len)
{
    if (LDG_UNLIKELY(ctx == 0x0 || src == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->src = src;
    ctx->src_len = src_len;
    ctx->line = (uint32_t)LEX_STEP;
    ctx->col = (uint32_t)LEX_STEP;

    return LDG_ERR_AOK;
}

static uint32_t lex_op_next(crosstalk_eda_script_lex_t *ctx, char ch, char nx, crosstalk_eda_script_tok_t *out)
{
    uint32_t kind = 0;
    uint32_t len = (uint32_t)LEX_STEP;

    if (ch == '<') { if (nx == '<' && lex_at(ctx, (uint32_t)LEX_PAIR) == '=') { kind = CROSSTALK_EDA_SCRIPT_TOK_SHL_ASSIGN; len = (uint32_t)LEX_TRIPLE; }
        else if (nx == '<' && lex_at(ctx, (uint32_t)LEX_PAIR) == '<') { kind = CROSSTALK_EDA_SCRIPT_TOK_SHLA; len = (uint32_t)LEX_TRIPLE; }
        else if (nx == '=' && lex_at(ctx, (uint32_t)LEX_PAIR) == '>') { kind = CROSSTALK_EDA_SCRIPT_TOK_BICOND; len = (uint32_t)LEX_TRIPLE; }
        else if (nx == '<') { kind = CROSSTALK_EDA_SCRIPT_TOK_SHL; len = (uint32_t)LEX_PAIR; }
        else if (nx == '=') { kind = CROSSTALK_EDA_SCRIPT_TOK_LTE; len = (uint32_t)LEX_PAIR; }
        else { kind = CROSSTALK_EDA_SCRIPT_TOK_LT; } }
    else if (ch == '>') { if (nx == '>' && lex_at(ctx, (uint32_t)LEX_PAIR) == '=') { kind = CROSSTALK_EDA_SCRIPT_TOK_SHR_ASSIGN; len = (uint32_t)LEX_TRIPLE; }
        else if (nx == '>' && lex_at(ctx, (uint32_t)LEX_PAIR) == '>') { kind = CROSSTALK_EDA_SCRIPT_TOK_SHRA; len = (uint32_t)LEX_TRIPLE; }
        else if (nx == '>') { kind = CROSSTALK_EDA_SCRIPT_TOK_SHR; len = (uint32_t)LEX_PAIR; }
        else if (nx == '=') { kind = CROSSTALK_EDA_SCRIPT_TOK_GTE; len = (uint32_t)LEX_PAIR; }
        else { kind = CROSSTALK_EDA_SCRIPT_TOK_GT; } }
    else if (ch == '=') { if (nx == '=') { kind = CROSSTALK_EDA_SCRIPT_TOK_EQ; len = (uint32_t)LEX_PAIR; }
        else if (nx == '>') { kind = CROSSTALK_EDA_SCRIPT_TOK_IMPL; len = (uint32_t)LEX_PAIR; }
        else { kind = CROSSTALK_EDA_SCRIPT_TOK_ASSIGN; } }
    else if (ch == '!') { if (nx == '=') { kind = CROSSTALK_EDA_SCRIPT_TOK_NEQ; len = (uint32_t)LEX_PAIR; }
        else { kind = CROSSTALK_EDA_SCRIPT_TOK_BANG; } }
    else if (ch == '&') { if (nx == '&') { kind = CROSSTALK_EDA_SCRIPT_TOK_LAND; len = (uint32_t)LEX_PAIR; }
        else if (nx == '=') { kind = CROSSTALK_EDA_SCRIPT_TOK_AMP_ASSIGN; len = (uint32_t)LEX_PAIR; }
        else { kind = CROSSTALK_EDA_SCRIPT_TOK_AMP; } }
    else if (ch == '|') { if (nx == '|') { kind = CROSSTALK_EDA_SCRIPT_TOK_LOR; len = (uint32_t)LEX_PAIR; }
        else if (nx == '=') { kind = CROSSTALK_EDA_SCRIPT_TOK_PIPE_ASSIGN; len = (uint32_t)LEX_PAIR; }
        else { kind = CROSSTALK_EDA_SCRIPT_TOK_PIPE; } }
    else if (ch == '^') { if (nx == '~') { kind = CROSSTALK_EDA_SCRIPT_TOK_XNOR; len = (uint32_t)LEX_PAIR; }
        else if (nx == '=') { kind = CROSSTALK_EDA_SCRIPT_TOK_CARET_ASSIGN; len = (uint32_t)LEX_PAIR; }
        else { kind = CROSSTALK_EDA_SCRIPT_TOK_CARET; } }
    else if (ch == '~') { if (nx == '^') { kind = CROSSTALK_EDA_SCRIPT_TOK_XNOR; len = (uint32_t)LEX_PAIR; }
        else { kind = CROSSTALK_EDA_SCRIPT_TOK_TILDE; } }
    else if (ch == '+') { if (nx == '=') { kind = CROSSTALK_EDA_SCRIPT_TOK_PLUS_ASSIGN; len = (uint32_t)LEX_PAIR; }
        else { kind = CROSSTALK_EDA_SCRIPT_TOK_PLUS; } }
    else if (ch == '-') { if (nx == '=') { kind = CROSSTALK_EDA_SCRIPT_TOK_MINUS_ASSIGN; len = (uint32_t)LEX_PAIR; }
        else { kind = CROSSTALK_EDA_SCRIPT_TOK_MINUS; } }
    else if (ch == '*') { if (nx == '=') { kind = CROSSTALK_EDA_SCRIPT_TOK_STAR_ASSIGN; len = (uint32_t)LEX_PAIR; }
        else { kind = CROSSTALK_EDA_SCRIPT_TOK_STAR; } }
    else if (ch == '/') { if (nx == '=') { kind = CROSSTALK_EDA_SCRIPT_TOK_SLASH_ASSIGN; len = (uint32_t)LEX_PAIR; }
        else { kind = CROSSTALK_EDA_SCRIPT_TOK_SLASH; } }
    else if (ch == '%') { if (nx == '=') { kind = CROSSTALK_EDA_SCRIPT_TOK_PERCENT_ASSIGN; len = (uint32_t)LEX_PAIR; }
        else { kind = CROSSTALK_EDA_SCRIPT_TOK_PERCENT; } }
    else { return CROSSTALK_EDA_SCRIPT_ERR_LEX; }

    out->kind = kind;
    out->len = len;
    lex_advance(ctx, len);

    return LDG_ERR_AOK;
}

static uint32_t lex_punct_kind_get(char ch, uint32_t *out)
{
    if (ch == '(') { *out = CROSSTALK_EDA_SCRIPT_TOK_LPAREN; return LDG_ERR_AOK; }

    if (ch == ')') { *out = CROSSTALK_EDA_SCRIPT_TOK_RPAREN; return LDG_ERR_AOK; }

    if (ch == '{') { *out = CROSSTALK_EDA_SCRIPT_TOK_LBRACE; return LDG_ERR_AOK; }

    if (ch == '}') { *out = CROSSTALK_EDA_SCRIPT_TOK_RBRACE; return LDG_ERR_AOK; }

    if (ch == '[') { *out = CROSSTALK_EDA_SCRIPT_TOK_LBRACKET; return LDG_ERR_AOK; }

    if (ch == ']') { *out = CROSSTALK_EDA_SCRIPT_TOK_RBRACKET; return LDG_ERR_AOK; }

    if (ch == ';') { *out = CROSSTALK_EDA_SCRIPT_TOK_SEMI; return LDG_ERR_AOK; }

    if (ch == ',') { *out = CROSSTALK_EDA_SCRIPT_TOK_COMMA; return LDG_ERR_AOK; }

    if (ch == '.') { *out = CROSSTALK_EDA_SCRIPT_TOK_DOT; return LDG_ERR_AOK; }

    if (ch == ':') { *out = CROSSTALK_EDA_SCRIPT_TOK_COLON; return LDG_ERR_AOK; }

    if (ch == '?') { *out = CROSSTALK_EDA_SCRIPT_TOK_QUESTION; return LDG_ERR_AOK; }

    return CROSSTALK_EDA_SCRIPT_ERR_LEX;
}

static uint32_t lex_num_next(crosstalk_eda_script_lex_t *ctx, crosstalk_eda_script_tok_t *out)
{
    uint64_t prefix = 0;
    uint64_t val = 0;
    uint32_t prefix_digits = 0;
    uint32_t ret = 0;

    while (lex_digit_is(lex_at(ctx, prefix_digits)) == LDG_TRUTH_TRUE && prefix_digits < (uint32_t)CROSSTALK_EDA_SCRIPT_WIDTH_DIGITS_MAX)
    {
        prefix = prefix * (uint64_t)CROSSTALK_EDA_SCRIPT_RADIX_DEC + (uint64_t)(lex_at(ctx, prefix_digits) - LDG_ASCII_DIGIT_ZERO);
        prefix_digits = prefix_digits + (uint32_t)LEX_STEP;
    }

    if (lex_at(ctx, prefix_digits) == LEX_TICK)
    {
        uint64_t max_val = 0;

        if (LDG_UNLIKELY(crosstalk_eda_script_lex_width_valid_is(prefix) != LDG_TRUTH_TRUE)) { return CROSSTALK_EDA_SCRIPT_ERR_LEX; }

        lex_advance(ctx, prefix_digits + (uint32_t)LEX_STEP);
        max_val = prefix == (uint64_t)CROSSTALK_EDA_SCRIPT_WIDTH_MAX ? UINT64_MAX : ((((uint64_t)LEX_STEP) << prefix) - (uint64_t)LEX_STEP);

        ret = lex_num_body(ctx, max_val, &val);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        out->kind = CROSSTALK_EDA_SCRIPT_TOK_INT;
        out->width = (uint32_t)prefix;
        out->val = val;
        out->len = (uint32_t)((ctx->src + ctx->at) - out->start);

        return LDG_ERR_AOK;
    }

    ret = lex_num_body(ctx, UINT64_MAX, &val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->kind = CROSSTALK_EDA_SCRIPT_TOK_INT;
    out->width = 0;
    out->val = val;
    out->len = (uint32_t)((ctx->src + ctx->at) - out->start);

    return LDG_ERR_AOK;
}

static uint32_t lex_char_next(crosstalk_eda_script_lex_t *ctx, crosstalk_eda_script_tok_t *out)
{
    char cc = 0;

    lex_advance(ctx, (uint32_t)LEX_STEP);
    cc = lex_at(ctx, (uint32_t)LEX_HERE);

    if (LDG_UNLIKELY(cc == LDG_STR_TERM || cc == '\n')) { return CROSSTALK_EDA_SCRIPT_ERR_LEX; }

    if (cc == LEX_ESC)
    {
        uint32_t ret = lex_esc_decode(lex_at(ctx, (uint32_t)LEX_STEP), &cc);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        lex_advance(ctx, (uint32_t)LEX_PAIR);
    }
    else { lex_advance(ctx, (uint32_t)LEX_STEP); }

    if (LDG_UNLIKELY(lex_at(ctx, (uint32_t)LEX_HERE) != LEX_TICK)) { return CROSSTALK_EDA_SCRIPT_ERR_LEX; }

    lex_advance(ctx, (uint32_t)LEX_STEP);
    out->kind = CROSSTALK_EDA_SCRIPT_TOK_CHAR;
    out->width = (uint32_t)LEX_CHAR_WIDTH;
    out->val = (uint64_t)(uint8_t)cc;
    out->len = (uint32_t)((ctx->src + ctx->at) - out->start);

    return LDG_ERR_AOK;
}

static uint32_t lex_str_next(crosstalk_eda_script_lex_t *ctx, crosstalk_eda_script_tok_t *out)
{
    uint32_t len = (uint32_t)LEX_STEP;

    while (LDG_TRUTH_TRUE)
    {
        char sc = lex_at(ctx, len);

        if (LDG_UNLIKELY(sc == LDG_STR_TERM || sc == '\n')) { return CROSSTALK_EDA_SCRIPT_ERR_LEX; }

        if (sc == LEX_ESC) { len = len + (uint32_t)LEX_PAIR; continue; }

        if (sc == LDG_ASCII_QUOTE) { break; }

        len = len + (uint32_t)LEX_STEP;
    }

    if (LDG_UNLIKELY(len > (uint32_t)CROSSTALK_EDA_SCRIPT_STR_LEN_MAX)) { return CROSSTALK_EDA_SCRIPT_ERR_LEX; }

    out->kind = CROSSTALK_EDA_SCRIPT_TOK_STR;
    out->start = ctx->src + ctx->at + (uint32_t)LEX_STEP;
    out->len = len - (uint32_t)LEX_STEP;
    lex_advance(ctx, len + (uint32_t)LEX_STEP);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_script_lex_next(crosstalk_eda_script_lex_t *ctx, crosstalk_eda_script_tok_t *out)
{
    char ch = 0;
    char nx = 0;
    uint32_t kind = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(out, 0x0, sizeof(*out));
    lex_ws_skip(ctx);

    out->start = ctx->src + ctx->at;
    out->line = ctx->line;
    out->col = ctx->col;

    if (ctx->at >= ctx->src_len)
    {
        out->kind = CROSSTALK_EDA_SCRIPT_TOK_EOF;

        return LDG_ERR_AOK;
    }

    ch = ctx->src[ctx->at];
    nx = lex_at(ctx, (uint32_t)LEX_STEP);

    if (LDG_UNLIKELY((uint8_t)ch > (uint8_t)CROSSTALK_EDA_SCRIPT_ASCII_MAX)) { return CROSSTALK_EDA_SCRIPT_ERR_LEX; }

    if (lex_ident_start_is(ch) == LDG_TRUTH_TRUE)
    {
        uint32_t len = 0;

        while (lex_ident_cont_is(lex_at(ctx, len)) == LDG_TRUTH_TRUE) { len = len + (uint32_t)LEX_STEP; }

        if (LDG_UNLIKELY(len > (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_LEN_MAX)) { return CROSSTALK_EDA_SCRIPT_ERR_LEX; }

        out->len = len;
        out->kind = lex_kw_find(out->start, len, &kind) == LDG_TRUTH_TRUE ? kind : (uint32_t)CROSSTALK_EDA_SCRIPT_TOK_IDENT;
        lex_advance(ctx, len);

        return LDG_ERR_AOK;
    }

    if (lex_digit_is(ch) == LDG_TRUTH_TRUE) { return lex_num_next(ctx, out); }

    if (ch == LEX_TICK) { return lex_char_next(ctx, out); }

    if (ch == LDG_ASCII_QUOTE) { return lex_str_next(ctx, out); }

    if (lex_punct_kind_get(ch, &kind) == LDG_ERR_AOK)
    {
        out->kind = kind;
        out->len = (uint32_t)LEX_STEP;
        lex_advance(ctx, (uint32_t)LEX_STEP);

        return LDG_ERR_AOK;
    }

    return lex_op_next(ctx, ch, nx, out);
}

uint32_t crosstalk_eda_script_lex_peek(const crosstalk_eda_script_lex_t *ctx, crosstalk_eda_script_tok_t *out)
{
    crosstalk_eda_script_lex_t tmp = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    tmp = *ctx;

    return crosstalk_eda_script_lex_next(&tmp, out);
}

uint32_t crosstalk_eda_script_lex_raw_peek(const crosstalk_eda_script_lex_t *ctx, char *out)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_STR_TERM;
    at = ctx->at;

    while (at < ctx->src_len && (ctx->src[at] == ' ' || ctx->src[at] == '\t')) { at = at + (uint32_t)LEX_STEP; }

    if (at >= ctx->src_len) { return LDG_ERR_AOK; }

    *out = ctx->src[at];

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_script_lex_word_next(crosstalk_eda_script_lex_t *ctx, const char **out_start, uint32_t *out_len, uint8_t *out_quoted)
{
    uint32_t len = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_start == 0x0 || out_len == 0x0 || out_quoted == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_start = 0x0;
    *out_len = 0;
    *out_quoted = LDG_TRUTH_FALSE;

    while (ctx->at < ctx->src_len && (ctx->src[ctx->at] == ' ' || ctx->src[ctx->at] == '\t')) { lex_advance(ctx, (uint32_t)LEX_STEP); }

    if (ctx->at >= ctx->src_len) { return LDG_ERR_EOF; }

    if (ctx->src[ctx->at] == ';' || ctx->src[ctx->at] == '\n') { return LDG_ERR_EOF; }

    if (ctx->src[ctx->at] == LDG_ASCII_QUOTE)
    {
        crosstalk_eda_script_tok_t tok = LDG_STRUCT_ZERO_INIT;
        uint32_t ret = crosstalk_eda_script_lex_next(ctx, &tok);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        *out_start = tok.start;
        *out_len = tok.len;
        *out_quoted = LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    *out_start = ctx->src + ctx->at;

    while (ctx->at + len < ctx->src_len)
    {
        char ch = ctx->src[ctx->at + len];

        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == ';') { break; }

        len = len + (uint32_t)LEX_STEP;
    }

    if (LDG_UNLIKELY(len > (uint32_t)CROSSTALK_EDA_SCRIPT_STR_LEN_MAX)) { return CROSSTALK_EDA_SCRIPT_ERR_LEX; }

    lex_advance(ctx, len);
    *out_len = len;

    return LDG_ERR_AOK;
}
