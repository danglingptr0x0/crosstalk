#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "hdl/lex.h"

#define CROSSTALK_HDL_LEX_HERE 0x0
#define CROSSTALK_HDL_LEX_NEXT 0x1
#define CROSSTALK_HDL_LEX_NEXT2 0x2
#define CROSSTALK_HDL_LEX_LEN1 0x1
#define CROSSTALK_HDL_LEX_LEN2 0x2
#define CROSSTALK_HDL_LEX_LEN3 0x3
#define CROSSTALK_HDL_LEX_LINE_FIRST 0x1
#define CROSSTALK_HDL_LEX_COL_FIRST 0x1

static uint8_t crosstalk_hdl_lex_ident_start_is(char ch)
{
    if (ldg_char_alpha_is(ch) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    if (ch == '_') { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t crosstalk_hdl_lex_ident_char_is(char ch)
{
    if (ldg_char_alnum_is(ch) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    if (ch == '_' || ch == '$') { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t crosstalk_hdl_lex_base_char_is(char ch)
{
    if (ch == 'b' || ch == 'B' || ch == 'd' || ch == 'D' || ch == 'o' || ch == 'O' || ch == 'h' || ch == 'H') { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t crosstalk_hdl_lex_unbased_char_is(char ch)
{
    if (ch == 'x' || ch == 'X' || ch == 'z' || ch == 'Z') { return LDG_TRUTH_TRUE; }

    if (ldg_char_digit_is(ch) == LDG_TRUTH_TRUE && ch < '2') { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t crosstalk_hdl_lex_val_char_is(char ch)
{
    if (ldg_char_hex_is(ch) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    if (ch == 'x' || ch == 'X' || ch == 'z' || ch == 'Z' || ch == '?' || ch == '_') { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static char crosstalk_hdl_lex_peek_get(const crosstalk_hdl_lex_ctx_t *ctx, uint32_t ahead)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_STR_TERM; }

    if (ctx->pos + ahead >= ctx->len) { return LDG_STR_TERM; }

    return ctx->src[ctx->pos + ahead];
}

static void crosstalk_hdl_lex_advance(crosstalk_hdl_lex_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (ctx->pos >= ctx->len) { return; }

    if (ctx->src[ctx->pos] == '\n')
    {
        ctx->line++;
        ctx->col = CROSSTALK_HDL_LEX_COL_FIRST;
    }
    else { ctx->col++; }

    ctx->pos++;
}

static uint32_t crosstalk_hdl_lex_space_skip(crosstalk_hdl_lex_ctx_t *ctx)
{
    char ch = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (ctx->pos < ctx->len)
    {
        ch = ctx->src[ctx->pos];

        if (ldg_char_space_is(ch) == LDG_TRUTH_TRUE)
        {
            crosstalk_hdl_lex_advance(ctx);
            continue;
        }

        if (ch == '/' && crosstalk_hdl_lex_peek_get(ctx, CROSSTALK_HDL_LEX_NEXT) == '/')
        {
            while (ctx->pos < ctx->len && ctx->src[ctx->pos] != '\n') { crosstalk_hdl_lex_advance(ctx); }

            continue;
        }

        if (ch == '/' && crosstalk_hdl_lex_peek_get(ctx, CROSSTALK_HDL_LEX_NEXT) == '*')
        {
            crosstalk_hdl_lex_advance(ctx);
            crosstalk_hdl_lex_advance(ctx);

            while (LDG_TRUTH_TRUE)
            {
                if (ctx->pos >= ctx->len) { return CROSSTALK_HDL_ERR_LEX_COMMENT_UNTERM; }

                if (ctx->src[ctx->pos] == '*' && crosstalk_hdl_lex_peek_get(ctx, CROSSTALK_HDL_LEX_NEXT) == '/')
                {
                    crosstalk_hdl_lex_advance(ctx);
                    crosstalk_hdl_lex_advance(ctx);
                    break;
                }

                crosstalk_hdl_lex_advance(ctx);
            }

            continue;
        }

        break;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_lex_kw_get(const char *ptr, uint32_t len)
{
    if (LDG_UNLIKELY(ptr == 0x0)) { return UINT32_MAX; }

    if (len == 2)
    {
        if (memcmp(ptr, "if", 2) == 0) { return CROSSTALK_HDL_KW_IF; }

        if (memcmp(ptr, "or", 2) == 0) { return CROSSTALK_HDL_KW_OR; }
    }

    if (len == 3)
    {
        if (memcmp(ptr, "reg", 3) == 0) { return CROSSTALK_HDL_KW_REG; }

        if (memcmp(ptr, "bit", 3) == 0) { return CROSSTALK_HDL_KW_LOGIC; }

        if (memcmp(ptr, "in" "t", 3) == 0) { return CROSSTALK_HDL_KW_INT; }

        if (memcmp(ptr, "for", 3) == 0) { return CROSSTALK_HDL_KW_FOR; }

        if (memcmp(ptr, "end", 3) == 0) { return CROSSTALK_HDL_KW_END; }
    }

    if (len == 4)
    {
        if (memcmp(ptr, "wire", 4) == 0) { return CROSSTALK_HDL_KW_WIRE; }

        if (memcmp(ptr, "case", 4) == 0) { return CROSSTALK_HDL_KW_CASE; }

        if (memcmp(ptr, "else", 4) == 0) { return CROSSTALK_HDL_KW_ELSE; }

        if (memcmp(ptr, "enum", 4) == 0) { return CROSSTALK_HDL_KW_ENUM; }

        if (memcmp(ptr, "task", 4) == 0) { return CROSSTALK_HDL_KW_TASK; }
    }

    if (len == 5)
    {
        if (memcmp(ptr, "inout", 5) == 0) { return CROSSTALK_HDL_KW_INOUT; }

        if (memcmp(ptr, "logic", 5) == 0) { return CROSSTALK_HDL_KW_LOGIC; }

        if (memcmp(ptr, "begin", 5) == 0) { return CROSSTALK_HDL_KW_BEGIN; }

        if (memcmp(ptr, "input", 5) == 0) { return CROSSTALK_HDL_KW_INPUT; }

        if (memcmp(ptr, "while", 5) == 0) { return CROSSTALK_HDL_KW_WHILE; }

        if (memcmp(ptr, "casez", 5) == 0) { return CROSSTALK_HDL_KW_CASEZ; }

        if (memcmp(ptr, "casex", 5) == 0) { return CROSSTALK_HDL_KW_CASEX; }

        if (memcmp(ptr, "union", 5) == 0) { return CROSSTALK_HDL_KW_UNION; }

        if (memcmp(ptr, "final", len) == 0) { return CROSSTALK_HDL_KW_FINAL; }
    }

    if (len == 6)
    {
        if (memcmp(ptr, "module", 6) == 0) { return CROSSTALK_HDL_KW_MODULE; }

        if (memcmp(ptr, "output", 6) == 0) { return CROSSTALK_HDL_KW_OUTPUT; }

        if (memcmp(ptr, "genvar", 6) == 0) { return CROSSTALK_HDL_KW_GENVAR; }

        if (memcmp(ptr, "sig" "ned", 6) == 0) { return CROSSTALK_HDL_KW_SIGNED; }

        if (memcmp(ptr, "assign", 6) == 0) { return CROSSTALK_HDL_KW_ASSIGN; }

        if (memcmp(ptr, "always", 6) == 0) { return CROSSTALK_HDL_KW_ALWAYS; }

        if (memcmp(ptr, "repeat", 6) == 0) { return CROSSTALK_HDL_KW_REPEAT; }

        if (memcmp(ptr, "re" "turn", 6) == 0) { return CROSSTALK_HDL_KW_RET; }

        if (memcmp(ptr, "struct", 6) == 0) { return CROSSTALK_HDL_KW_STRUCT; }

        if (memcmp(ptr, "import", 6) == 0) { return CROSSTALK_HDL_KW_IMPORT; }

        if (memcmp(ptr, "packed", 6) == 0) { return CROSSTALK_HDL_KW_PACKED; }

        if (memcmp(ptr, "unique", 6) == 0) { return CROSSTALK_HDL_KW_UNIQUE; }
    }

    if (len == 7)
    {
        if (memcmp(ptr, "in" "teger", 7) == 0) { return CROSSTALK_HDL_KW_INTEGER; }

        if (memcmp(ptr, "unique0", 7) == 0) { return CROSSTALK_HDL_KW_UNIQUE; }

        if (memcmp(ptr, "initial", 7) == 0) { return CROSSTALK_HDL_KW_INITIAL; }

        if (memcmp(ptr, "endcase", 7) == 0) { return CROSSTALK_HDL_KW_ENDCASE; }

        if (memcmp(ptr, "default", 7) == 0) { return CROSSTALK_HDL_KW_DEFAULT; }

        if (memcmp(ptr, "typedef", 7) == 0) { return CROSSTALK_HDL_KW_TYPEDEF; }

        if (memcmp(ptr, "modport", 7) == 0) { return CROSSTALK_HDL_KW_MODPORT; }

        if (memcmp(ptr, "posedge", 7) == 0) { return CROSSTALK_HDL_KW_POSEDGE; }

        if (memcmp(ptr, "negedge", 7) == 0) { return CROSSTALK_HDL_KW_NEGEDGE; }

        if (memcmp(ptr, "endtask", 7) == 0) { return CROSSTALK_HDL_KW_ENDTASK; }

        if (memcmp(ptr, "package", 7) == 0) { return CROSSTALK_HDL_KW_PACKAGE; }
    }

    if (len == 8)
    {
        if (memcmp(ptr, "unsig" "ned", 8) == 0) { return CROSSTALK_HDL_KW_UNSIGNED; }

        if (memcmp(ptr, "priority", 8) == 0) { return CROSSTALK_HDL_KW_UNIQUE; }

        if (memcmp(ptr, "fun" "ction", 8) == 0) { return CROSSTALK_HDL_KW_FUNC; }

        if (memcmp(ptr, "generate", 8) == 0) { return CROSSTALK_HDL_KW_GENERATE; }
    }

    if (len == 9)
    {
        if (memcmp(ptr, "parameter", 9) == 0) { return CROSSTALK_HDL_KW_PARAMETER; }

        if (memcmp(ptr, "endmodule", 9) == 0) { return CROSSTALK_HDL_KW_ENDMODULE; }

        if (memcmp(ptr, "always_ff", 9) == 0) { return CROSSTALK_HDL_KW_ALWAYS_FF; }

        if (memcmp(ptr, "automatic", 9) == 0) { return CROSSTALK_HDL_KW_AUTOMATIC; }

        if (memcmp(ptr, "in" "terface", 9) == 0) { return CROSSTALK_HDL_KW_INTERFACE; }
    }

    if (len == 10)
    {
        if (memcmp(ptr, "localparam", 10) == 0) { return CROSSTALK_HDL_KW_LOCALPARAM; }

        if (memcmp(ptr, "endpackage", 10) == 0) { return CROSSTALK_HDL_KW_ENDPACKAGE; }
    }

    if (len == 11)
    {
        if (memcmp(ptr, "always_comb", 11) == 0) { return CROSSTALK_HDL_KW_ALWAYS_COMB; }

        if (memcmp(ptr, "endfun" "ction", 11) == 0) { return CROSSTALK_HDL_KW_ENDFUNC; }

        if (memcmp(ptr, "endgenerate", 11) == 0) { return CROSSTALK_HDL_KW_ENDGENERATE; }
    }

    if (len == 12) { if (memcmp(ptr, "endin" "terface", 12) == 0) { return CROSSTALK_HDL_KW_ENDINTERFACE; } }

    return UINT32_MAX;
}

static uint32_t crosstalk_hdl_lex_based_tail_scan(crosstalk_hdl_lex_ctx_t *ctx)
{
    uint32_t cunt = 0;
    char ch = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ch = crosstalk_hdl_lex_peek_get(ctx, CROSSTALK_HDL_LEX_HERE);
    if (ch == 's' || ch == 'S')
    {
        crosstalk_hdl_lex_advance(ctx);
        ch = crosstalk_hdl_lex_peek_get(ctx, CROSSTALK_HDL_LEX_HERE);
    }

    if (crosstalk_hdl_lex_base_char_is(ch) != LDG_TRUTH_TRUE) { return CROSSTALK_HDL_ERR_LEX_LITERAL; }

    crosstalk_hdl_lex_advance(ctx);

    while (crosstalk_hdl_lex_val_char_is(crosstalk_hdl_lex_peek_get(ctx, CROSSTALK_HDL_LEX_HERE)) == LDG_TRUTH_TRUE)
    {
        crosstalk_hdl_lex_advance(ctx);
        cunt++;
    }

    if (cunt == 0) { return CROSSTALK_HDL_ERR_LEX_LITERAL; }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_lex_num_scan(crosstalk_hdl_lex_ctx_t *ctx, uint32_t *out_kind)
{
    uint32_t ret = 0;
    char ch = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_kind == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_kind = CROSSTALK_HDL_TOK_LIT_DEC;

    while (LDG_TRUTH_TRUE)
    {
        ch = crosstalk_hdl_lex_peek_get(ctx, CROSSTALK_HDL_LEX_HERE);
        if (ldg_char_digit_is(ch) == LDG_TRUTH_TRUE || ch == '_')
        {
            crosstalk_hdl_lex_advance(ctx);
            continue;
        }

        break;
    }

    ch = crosstalk_hdl_lex_peek_get(ctx, CROSSTALK_HDL_LEX_HERE);

    if (ch == '.' && ldg_char_digit_is(crosstalk_hdl_lex_peek_get(ctx, CROSSTALK_HDL_LEX_NEXT)) == LDG_TRUTH_TRUE)
    {
        crosstalk_hdl_lex_advance(ctx);

        while (LDG_TRUTH_TRUE)
        {
            ch = crosstalk_hdl_lex_peek_get(ctx, CROSSTALK_HDL_LEX_HERE);
            if (ldg_char_digit_is(ch) == LDG_TRUTH_TRUE || ch == '_')
            {
                crosstalk_hdl_lex_advance(ctx);
                continue;
            }

            break;
        }

        ch = crosstalk_hdl_lex_peek_get(ctx, CROSSTALK_HDL_LEX_HERE);
        if (ch == 'e' || ch == 'E')
        {
            crosstalk_hdl_lex_advance(ctx);
            ch = crosstalk_hdl_lex_peek_get(ctx, CROSSTALK_HDL_LEX_HERE);
            if (ch == '+' || ch == '-')
            {
                crosstalk_hdl_lex_advance(ctx);
                ch = crosstalk_hdl_lex_peek_get(ctx, CROSSTALK_HDL_LEX_HERE);
            }

            if (ldg_char_digit_is(ch) != LDG_TRUTH_TRUE) { return CROSSTALK_HDL_ERR_LEX_LITERAL; }

            while (ldg_char_digit_is(crosstalk_hdl_lex_peek_get(ctx, CROSSTALK_HDL_LEX_HERE)) == LDG_TRUTH_TRUE) { crosstalk_hdl_lex_advance(ctx); }
        }

        *out_kind = CROSSTALK_HDL_TOK_LIT_REAL;

        return LDG_ERR_AOK;
    }

    if (ch == '\'' && crosstalk_hdl_lex_peek_get(ctx, CROSSTALK_HDL_LEX_NEXT) != '(' && crosstalk_hdl_lex_peek_get(ctx, CROSSTALK_HDL_LEX_NEXT) != '{')
    {
        crosstalk_hdl_lex_advance(ctx);

        ret = crosstalk_hdl_lex_based_tail_scan(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        *out_kind = CROSSTALK_HDL_TOK_LIT_BASED;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hdl_lex_op_scan(crosstalk_hdl_lex_ctx_t *ctx, uint32_t *out_kind, uint32_t *out_val)
{
    uint32_t op = UINT32_MAX;
    uint32_t width = CROSSTALK_HDL_LEX_LEN1;
    uint32_t idx = 0;
    char ch = 0;
    char ch1 = 0;
    char ch2 = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_kind == 0x0 || out_val == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_kind = CROSSTALK_HDL_TOK_OP;
    *out_val = UINT32_MAX;

    ch = crosstalk_hdl_lex_peek_get(ctx, CROSSTALK_HDL_LEX_HERE);
    ch1 = crosstalk_hdl_lex_peek_get(ctx, CROSSTALK_HDL_LEX_NEXT);
    ch2 = crosstalk_hdl_lex_peek_get(ctx, CROSSTALK_HDL_LEX_NEXT2);

    if (ch == '(') { if (ch1 == '*' && ch2 != ')')
        {
            *out_kind = CROSSTALK_HDL_TOK_ATTR_OPEN;
            ctx->in_attr = LDG_TRUTH_TRUE;
            width = CROSSTALK_HDL_LEX_LEN2;
        }
        else { op = CROSSTALK_HDL_OP_LPAREN; } }
    else if (ch == '*') { if (ctx->in_attr == LDG_TRUTH_TRUE && ch1 == ')')
        {
            *out_kind = CROSSTALK_HDL_TOK_ATTR_CLOSE;
            ctx->in_attr = LDG_TRUTH_FALSE;
            width = CROSSTALK_HDL_LEX_LEN2;
        }
        else if (ch1 == '=') { op = CROSSTALK_HDL_OP_STAR_ASSIGN; width = CROSSTALK_HDL_LEX_LEN2; }
        else { op = CROSSTALK_HDL_OP_STAR; } }
    else if (ch == '<') { if (ch1 == '<') { if (ch2 == '<') { op = CROSSTALK_HDL_OP_ASHL; width = CROSSTALK_HDL_LEX_LEN3; }
            else if (ch2 == '=') { op = CROSSTALK_HDL_OP_SHL_ASSIGN; width = CROSSTALK_HDL_LEX_LEN3; }
            else { op = CROSSTALK_HDL_OP_SHL; width = CROSSTALK_HDL_LEX_LEN2; } }
        else if (ch1 == '=') { op = CROSSTALK_HDL_OP_LE; width = CROSSTALK_HDL_LEX_LEN2; }
        else { op = CROSSTALK_HDL_OP_LT; } }
    else if (ch == '>') { if (ch1 == '>') { if (ch2 == '>') { op = CROSSTALK_HDL_OP_ASHR; width = CROSSTALK_HDL_LEX_LEN3; }
            else if (ch2 == '=') { op = CROSSTALK_HDL_OP_SHR_ASSIGN; width = CROSSTALK_HDL_LEX_LEN3; }
            else { op = CROSSTALK_HDL_OP_SHR; width = CROSSTALK_HDL_LEX_LEN2; } }
        else if (ch1 == '=') { op = CROSSTALK_HDL_OP_GE; width = CROSSTALK_HDL_LEX_LEN2; }
        else { op = CROSSTALK_HDL_OP_GT; } }
    else if (ch == '=') { if (ch1 == '=') { if (ch2 == '=') { op = CROSSTALK_HDL_OP_CASE_EQ; width = CROSSTALK_HDL_LEX_LEN3; }
            else { op = CROSSTALK_HDL_OP_EQ; width = CROSSTALK_HDL_LEX_LEN2; } }
        else { op = CROSSTALK_HDL_OP_ASSIGN; } }
    else if (ch == '!') { if (ch1 == '=') { if (ch2 == '=') { op = CROSSTALK_HDL_OP_CASE_NEQ; width = CROSSTALK_HDL_LEX_LEN3; }
            else { op = CROSSTALK_HDL_OP_NEQ; width = CROSSTALK_HDL_LEX_LEN2; } }
        else { op = CROSSTALK_HDL_OP_BANG; } }
    else if (ch == '&') { if (ch1 == '&') { op = CROSSTALK_HDL_OP_LAND; width = CROSSTALK_HDL_LEX_LEN2; }
        else if (ch1 == '=') { op = CROSSTALK_HDL_OP_AMP_ASSIGN; width = CROSSTALK_HDL_LEX_LEN2; }
        else { op = CROSSTALK_HDL_OP_AMP; } }
    else if (ch == '|') { if (ch1 == '|') { op = CROSSTALK_HDL_OP_LOR; width = CROSSTALK_HDL_LEX_LEN2; }
        else if (ch1 == '=') { op = CROSSTALK_HDL_OP_PIPE_ASSIGN; width = CROSSTALK_HDL_LEX_LEN2; }
        else { op = CROSSTALK_HDL_OP_PIPE; } }
    else if (ch == '^') { if (ch1 == '~') { op = CROSSTALK_HDL_OP_XNOR; width = CROSSTALK_HDL_LEX_LEN2; }
        else if (ch1 == '=') { op = CROSSTALK_HDL_OP_CARET_ASSIGN; width = CROSSTALK_HDL_LEX_LEN2; }
        else { op = CROSSTALK_HDL_OP_CARET; } }
    else if (ch == '~') { if (ch1 == '&') { op = CROSSTALK_HDL_OP_NAND; width = CROSSTALK_HDL_LEX_LEN2; }
        else if (ch1 == '|') { op = CROSSTALK_HDL_OP_NOR; width = CROSSTALK_HDL_LEX_LEN2; }
        else if (ch1 == '^') { op = CROSSTALK_HDL_OP_XNOR; width = CROSSTALK_HDL_LEX_LEN2; }
        else { op = CROSSTALK_HDL_OP_TILDE; } }
    else if (ch == '+') { if (ch1 == '+') { op = CROSSTALK_HDL_OP_INC; width = CROSSTALK_HDL_LEX_LEN2; }
        else if (ch1 == '=') { op = CROSSTALK_HDL_OP_PLUS_ASSIGN; width = CROSSTALK_HDL_LEX_LEN2; }
        else { op = CROSSTALK_HDL_OP_PLUS; } }
    else if (ch == '-') { if (ch1 == '-') { op = CROSSTALK_HDL_OP_DEC; width = CROSSTALK_HDL_LEX_LEN2; }
        else if (ch1 == '=') { op = CROSSTALK_HDL_OP_MINUS_ASSIGN; width = CROSSTALK_HDL_LEX_LEN2; }
        else { op = CROSSTALK_HDL_OP_MINUS; } }
    else if (ch == '/') { if (ch1 == '=') { op = CROSSTALK_HDL_OP_SLASH_ASSIGN; width = CROSSTALK_HDL_LEX_LEN2; }
        else { op = CROSSTALK_HDL_OP_SLASH; } }
    else if (ch == '%') { if (ch1 == '=') { op = CROSSTALK_HDL_OP_PERCENT_ASSIGN; width = CROSSTALK_HDL_LEX_LEN2; }
        else { op = CROSSTALK_HDL_OP_PERCENT; } }
    else if (ch == ':') { if (ch1 == ':') { op = CROSSTALK_HDL_OP_SCOPE; width = CROSSTALK_HDL_LEX_LEN2; }
        else { op = CROSSTALK_HDL_OP_COLON; } }
    else if (ch == ')') { op = CROSSTALK_HDL_OP_RPAREN; }
    else if (ch == '[') { op = CROSSTALK_HDL_OP_LBRACK; }
    else if (ch == ']') { op = CROSSTALK_HDL_OP_RBRACK; }
    else if (ch == '{') { op = CROSSTALK_HDL_OP_LBRACE; }
    else if (ch == '}') { op = CROSSTALK_HDL_OP_RBRACE; }
    else if (ch == ';') { op = CROSSTALK_HDL_OP_SEMI; }
    else if (ch == ',') { op = CROSSTALK_HDL_OP_COMMA; }
    else if (ch == '.') { op = CROSSTALK_HDL_OP_DOT; }
    else if (ch == '?') { op = CROSSTALK_HDL_OP_QUESTION; }
    else if (ch == '@') { op = CROSSTALK_HDL_OP_AT; }
    else if (ch == '#') { op = CROSSTALK_HDL_OP_HASH; }
    else { return CROSSTALK_HDL_ERR_LEX_CHAR; }

    for (idx = 0; idx < width; idx++) { crosstalk_hdl_lex_advance(ctx); }

    if (*out_kind == CROSSTALK_HDL_TOK_OP) { *out_val = op; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_lex_next(crosstalk_hdl_lex_ctx_t *ctx, crosstalk_hdl_tok_t *out)
{
    crosstalk_hdl_tok_t blank = LDG_STRUCT_ZERO_INIT;
    uint32_t off = 0;
    uint32_t line = 0;
    uint32_t col = 0;
    uint32_t kind = 0;
    uint32_t val = 0;
    uint32_t kw = 0;
    uint32_t cunt = 0;
    uint32_t ret = 0;
    char ch = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = blank;

    ret = crosstalk_hdl_lex_space_skip(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    off = ctx->pos;
    line = ctx->line;
    col = ctx->col;

    if (ctx->pos >= ctx->len)
    {
        out->kind = CROSSTALK_HDL_TOK_EOF;
        out->off = off;
        out->line = line;
        out->col = col;

        return LDG_ERR_AOK;
    }

    ch = ctx->src[ctx->pos];

    if (crosstalk_hdl_lex_ident_start_is(ch) == LDG_TRUTH_TRUE)
    {
        while (crosstalk_hdl_lex_ident_char_is(crosstalk_hdl_lex_peek_get(ctx, CROSSTALK_HDL_LEX_HERE)) == LDG_TRUTH_TRUE) { crosstalk_hdl_lex_advance(ctx); }

        kind = CROSSTALK_HDL_TOK_IDENT;
        kw = crosstalk_hdl_lex_kw_get(ctx->src + off, ctx->pos - off);
        if (kw != UINT32_MAX)
        {
            kind = CROSSTALK_HDL_TOK_KEYWORD;
            val = kw;
        }
    }
    else if (ldg_char_digit_is(ch) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_hdl_lex_num_scan(ctx, &kind);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    else if (ch == '\'')
    {
        crosstalk_hdl_lex_advance(ctx);
        ch = crosstalk_hdl_lex_peek_get(ctx, CROSSTALK_HDL_LEX_HERE);

        if (crosstalk_hdl_lex_base_char_is(ch) == LDG_TRUTH_TRUE || ch == 's' || ch == 'S')
        {
            ret = crosstalk_hdl_lex_based_tail_scan(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            kind = CROSSTALK_HDL_TOK_LIT_BASED;
        }
        else if (crosstalk_hdl_lex_unbased_char_is(ch) == LDG_TRUTH_TRUE)
        {
            crosstalk_hdl_lex_advance(ctx);
            kind = CROSSTALK_HDL_TOK_LIT_UNBASED;
        }
        else if (ch == '(' || ch == '{')
        {
            kind = CROSSTALK_HDL_TOK_OP;
            val = CROSSTALK_HDL_OP_TICK;
        }
        else { return CROSSTALK_HDL_ERR_LEX_LITERAL; }
    }
    else if (ch == '"')
    {
        crosstalk_hdl_lex_advance(ctx);

        while (LDG_TRUTH_TRUE)
        {
            if (ctx->pos >= ctx->len) { return CROSSTALK_HDL_ERR_LEX_STR_UNTERM; }

            ch = ctx->src[ctx->pos];

            if (ch == '\n') { return CROSSTALK_HDL_ERR_LEX_STR_UNTERM; }

            if (ch == '\\')
            {
                crosstalk_hdl_lex_advance(ctx);
                if (ctx->pos >= ctx->len) { return CROSSTALK_HDL_ERR_LEX_STR_UNTERM; }

                crosstalk_hdl_lex_advance(ctx);
                continue;
            }

            if (ch == '"')
            {
                crosstalk_hdl_lex_advance(ctx);
                break;
            }

            crosstalk_hdl_lex_advance(ctx);
        }

        kind = CROSSTALK_HDL_TOK_LIT_STR;
    }
    else if (ch == '\\')
    {
        crosstalk_hdl_lex_advance(ctx);

        while (ctx->pos < ctx->len && ldg_char_space_is(ctx->src[ctx->pos]) != LDG_TRUTH_TRUE)
        {
            crosstalk_hdl_lex_advance(ctx);
            cunt++;
        }

        if (cunt == 0) { return CROSSTALK_HDL_ERR_LEX_ESC_UNTERM; }

        kind = CROSSTALK_HDL_TOK_IDENT;
    }
    else if (ch == '$')
    {
        if (crosstalk_hdl_lex_ident_start_is(crosstalk_hdl_lex_peek_get(ctx, CROSSTALK_HDL_LEX_NEXT)) != LDG_TRUTH_TRUE) { return CROSSTALK_HDL_ERR_LEX_CHAR; }

        crosstalk_hdl_lex_advance(ctx);

        while (crosstalk_hdl_lex_ident_char_is(crosstalk_hdl_lex_peek_get(ctx, CROSSTALK_HDL_LEX_HERE)) == LDG_TRUTH_TRUE) { crosstalk_hdl_lex_advance(ctx); }

        kind = CROSSTALK_HDL_TOK_SYS_IDENT;
    }
    else if (ch == '`')
    {
        crosstalk_hdl_lex_advance(ctx);

        if (crosstalk_hdl_lex_ident_start_is(crosstalk_hdl_lex_peek_get(ctx, CROSSTALK_HDL_LEX_HERE)) != LDG_TRUTH_TRUE) { return CROSSTALK_HDL_ERR_LEX_CHAR; }

        while (crosstalk_hdl_lex_ident_char_is(crosstalk_hdl_lex_peek_get(ctx, CROSSTALK_HDL_LEX_HERE)) == LDG_TRUTH_TRUE) { crosstalk_hdl_lex_advance(ctx); }

        kind = CROSSTALK_HDL_TOK_DIRECTIVE;
    }
    else
    {
        ret = crosstalk_hdl_lex_op_scan(ctx, &kind, &val);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    out->kind = kind;
    out->val = val;
    out->off = off;
    out->len = ctx->pos - off;
    out->line = line;
    out->col = col;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_lex_init(crosstalk_hdl_lex_ctx_t *ctx, const char *src, uint32_t len)
{
    if (LDG_UNLIKELY(ctx == 0x0 || src == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->src = src;
    ctx->len = len;
    ctx->pos = 0x0;
    ctx->line = CROSSTALK_HDL_LEX_LINE_FIRST;
    ctx->col = CROSSTALK_HDL_LEX_COL_FIRST;
    ctx->in_attr = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}
