#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/arch/dangcsa/dangcsa.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "eda/ui/calc.h"
#include "ui/cell.h"
#include "ui/theme.h"

#define CALC_STEP 1
#define CALC_ORIG_COL 0
#define CALC_PROMPT_TEXT "> "
#define CALC_HINT_TEXT "x = expr; \"text\" for ascii; dis expr; sig names live; expr[hi:lo]; mem[idx]; 1.3ms at clk; res = last; drag a result out"
#define CALC_BYTE_FMT "%02X"
#define CALC_BYTE_GAP_FMT " %02X"
#define CALC_TEXT_CAP 32
#define CALC_ORDER_LE_FMT "le: %.87s"
#define CALC_ORDER_BE_FMT "be: %.87s"
#define CALC_TEXT_LEN_FMT "text %u bytes; ascii: %.*s"
#define CALC_PREC_LOR 1
#define CALC_PREC_XOR 2
#define CALC_PREC_LAND 3
#define CALC_PREC_SHIFT 4
#define CALC_PREC_ADD 5
#define CALC_PREC_MUL 6
#define CALC_PREC_UNARY 7
#define CALC_PREC_POW 8
#define CALC_POW_EXP_MAX 64
#define CALC_GROUP_HEX 0
#define CALC_GROUP_BIN 1
#define CALC_GROUP_DEC 2
#define CALC_GROUP_SEP_BAR '_'
#define CALC_BIT_CH_CLEAR '0'
#define CALC_BIT_CH_SET '1'
#define CALC_OP_POW '^'
#define CALC_OP_XOR 'x'
#define CALC_NIBBLE_GROUP 4
#define CALC_FP32_BITS 32
#define CALC_FP32_MASK 0xFFFFFFFFULL
#define CALC_FP32_SIGN_SHIFT 31
#define CALC_FP32_EXP_SHIFT 23
#define CALC_FP32_EXP_MASK 0xFF
#define CALC_FP32_MANT_MASK 0x7FFFFF
#define CALC_FP32_MANT_SCALE 8388608.0
#define CALC_FP32_BIAS 127
#define CALC_FP32_SUBNORM_EXP -126
#define CALC_FP32_DIGITS 9
#define CALC_FP64_DIGITS 17
#define CALC_FP_TEXT_CAP 40
#define CALC_FP_FRAC_SUFFIX ".0"
#define CALC_FP_FRAC_CHARS 2
#define CALC_FP_NAN_TEXT "nan"
#define CALC_FP_INF_TEXT "inf"
#define CALC_FP_NEG_INF_TEXT "-inf"
#define CALC_FIXED_SCALE 65536.0
#define CALC_FIXED_DIGITS 9
#define CALC_SHIFT_BITS_MAX 64
#define CALC_SHIFT_RES_EMPTY 0
#define CALC_BIT_MAX 63
#define CALC_NAME_CAP 64
#define CALC_DIS_PIECE_CAP 56
#define CALC_HIST_SHOWN_MAX 10
#define CALC_MEM_WORD_BYTES 4
#define CALC_TIME_US_NS 1000.0
#define CALC_TIME_MS_NS 1000000.0
#define CALC_TIME_S_NS 1000000000.0
#define CALC_FRAC_BASE 10.0
#define CALC_DIS_TOK "dis "
#define CALC_DIS_TOK_LEN 4
#define CALC_RES_NAME "res"

typedef struct calc_cursor
{
    const char *text;
    uint32_t at;
} calc_cursor_t;

typedef struct calc_env
{
    const crosstalk_eda_ui_calc_ctx_t *calc;
    uint64_t rate_hz;
} calc_env_t;

static void calc_space_skip(calc_cursor_t *cur)
{
    while (cur->text[cur->at] == ' ' || cur->text[cur->at] == '\t') { cur->at = cur->at + 0x1; }
}

static uint8_t calc_digit_zero_is(char ch)
{
    uint8_t nibble = 0;

    if (ldg_char_digit_is((uint8_t)ch) != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    if (ldg_hex_to_nipple(ch, &nibble) != LDG_ERR_AOK) { return LDG_TRUTH_FALSE; }

    return nibble == 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint8_t calc_ident_first_is(char ch)
{
    if (ldg_char_alpha_is((uint8_t)ch) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return ch == '_' ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint8_t calc_ident_rest_is(char ch)
{
    if (ldg_char_alnum_is((uint8_t)ch) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return ch == '_' || ch == '.' ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t calc_ident_parse(calc_cursor_t *cur, char *out, uint32_t cap)
{
    uint32_t len = 0;

    if (calc_ident_first_is(cur->text[cur->at]) != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_UI_CALC_ERR_PARSE; }

    while (calc_ident_rest_is(cur->text[cur->at]) == LDG_TRUTH_TRUE)
    {
        if (len + LDG_STR_TERM_SIZE >= cap) { return CROSSTALK_EDA_UI_CALC_ERR_PARSE; }

        out[len] = cur->text[cur->at];
        len = len + 0x1;
        cur->at = cur->at + 0x1;
    }

    out[len] = LDG_STR_TERM;
    return LDG_ERR_AOK;
}

static uint8_t calc_var_find(const crosstalk_eda_ui_calc_ctx_t *calc, const char *name, uint32_t *out_idx)
{
    uint32_t idx = 0;

    for (idx = 0; idx < calc->var_cunt; idx = idx + 0x1) { if (strcmp(calc->var[idx].name, name) == 0x0)
        {
            *out_idx = idx;
            return LDG_TRUTH_TRUE;
        }
    }

    return LDG_TRUTH_FALSE;
}

static uint32_t calc_ident_val_get(const calc_env_t *env, const char *name, uint64_t *out)
{
    uint64_t val = 0;
    uint32_t width = 0;
    uint32_t idx = 0;

    *out = 0;

    if (strcmp(name, CALC_RES_NAME) == 0x0)
    {
        *out = env->calc->res_last;
        return LDG_ERR_AOK;
    }

    if (calc_var_find(env->calc, name, &idx) == LDG_TRUTH_TRUE)
    {
        *out = env->calc->var[idx].val;
        return LDG_ERR_AOK;
    }

    if (env->calc->fpga != 0x0 && crosstalk_eda_ic_fpga_sig_peek(env->calc->fpga, env->calc->slot, name, &val, &width) == LDG_ERR_AOK)
    {
        *out = val;
        return LDG_ERR_AOK;
    }

    return CROSSTALK_EDA_UI_CALC_ERR_IDENT;
}

static uint8_t calc_group_digit_is(char ch, uint32_t mode)
{
    if (mode == (uint32_t)CALC_GROUP_HEX) { return ldg_char_hex_is((uint8_t)ch); }

    if (mode == (uint32_t)CALC_GROUP_BIN) { return ch == CALC_BIT_CH_CLEAR || ch == CALC_BIT_CH_SET ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    return ldg_char_digit_is((uint8_t)ch);
}

static void calc_group_skip(calc_cursor_t *cur, uint32_t mode)
{
    uint32_t at = cur->at;

    while (cur->text[at] == CALC_GROUP_SEP_BAR || (cur->text[at] == LDG_ASCII_SPACE && mode != (uint32_t)CALC_GROUP_DEC)) { at = at + 0x1; }

    if (at == cur->at) { return; }

    if (calc_group_digit_is(cur->text[at], mode) == LDG_TRUTH_TRUE) { cur->at = at; }
}

static uint32_t calc_num_parse(const calc_env_t *env, calc_cursor_t *cur, uint64_t *out)
{
    double frac = 0.0;
    double frac_div = 1.0;
    double scale_ns = 0.0;
    uint64_t val = 0;
    uint32_t digits = 0;
    uint8_t dec_is = LDG_TRUTH_FALSE;

    *out = 0;

    if (calc_digit_zero_is(cur->text[cur->at]) == LDG_TRUTH_TRUE && (cur->text[cur->at + 0x1] == 'x' || cur->text[cur->at + 0x1] == 'X'))
    {
        cur->at = cur->at + 0x2;
        for (;;)
        {
            uint8_t nibble = 0;

            if (digits != 0) { calc_group_skip(cur, (uint32_t)CALC_GROUP_HEX); }

            if (ldg_char_hex_is((uint8_t)cur->text[cur->at]) != LDG_TRUTH_TRUE) { break; }

            if (ldg_hex_to_nipple(cur->text[cur->at], &nibble) != LDG_ERR_AOK) { break; }

            val = (val << CALC_NIBBLE_GROUP) | (uint64_t)nibble;
            cur->at = cur->at + 0x1;
            digits = digits + 0x1;
        }
    }
    else if (calc_digit_zero_is(cur->text[cur->at]) == LDG_TRUTH_TRUE && (cur->text[cur->at + 0x1] == 'b' || cur->text[cur->at + 0x1] == 'B'))
    {
        cur->at = cur->at + 0x2;
        for (;;)
        {
            uint8_t bit = 0;

            if (digits != 0) { calc_group_skip(cur, (uint32_t)CALC_GROUP_BIN); }

            if (ldg_char_digit_is((uint8_t)cur->text[cur->at]) != LDG_TRUTH_TRUE) { break; }

            if (ldg_hex_to_nipple(cur->text[cur->at], &bit) != LDG_ERR_AOK) { break; }

            if (bit > 0x1) { break; }

            val = (val << 0x1) | (uint64_t)bit;
            cur->at = cur->at + 0x1;
            digits = digits + 0x1;
        }
    }
    else
    {
        dec_is = LDG_TRUTH_TRUE;
        for (;;)
        {
            uint8_t digit = 0;

            if (digits != 0) { calc_group_skip(cur, (uint32_t)CALC_GROUP_DEC); }

            if (ldg_char_digit_is((uint8_t)cur->text[cur->at]) != LDG_TRUTH_TRUE) { break; }

            if (ldg_hex_to_nipple(cur->text[cur->at], &digit) != LDG_ERR_AOK) { break; }

            val = val * (uint64_t)LDG_BASE_DEC + (uint64_t)digit;
            cur->at = cur->at + 0x1;
            digits = digits + 0x1;
        }
    }

    if (digits == 0) { return CROSSTALK_EDA_UI_CALC_ERR_PARSE; }

    if (dec_is == LDG_TRUTH_TRUE && cur->text[cur->at] == '.' && ldg_char_digit_is((uint8_t)cur->text[cur->at + 0x1]) == LDG_TRUTH_TRUE)
    {
        cur->at = cur->at + 0x1;
        while (ldg_char_digit_is((uint8_t)cur->text[cur->at]) == LDG_TRUTH_TRUE)
        {
            uint8_t digit = 0;

            if (ldg_hex_to_nipple(cur->text[cur->at], &digit) != LDG_ERR_AOK) { break; }

            frac_div = frac_div * CALC_FRAC_BASE;
            frac = frac + (double)digit / frac_div;
            cur->at = cur->at + 0x1;
        }

        if (cur->text[cur->at] != 'n' && cur->text[cur->at] != 'u' && cur->text[cur->at] != 'm' && cur->text[cur->at] != 's') { return CROSSTALK_EDA_UI_CALC_ERR_PARSE; }
    }

    if (dec_is == LDG_TRUTH_TRUE) { if (cur->text[cur->at] == 'n' && cur->text[cur->at + 0x1] == 's') { scale_ns = 1.0; cur->at = cur->at + 0x2; }
        else if (cur->text[cur->at] == 'u' && cur->text[cur->at + 0x1] == 's') { scale_ns = CALC_TIME_US_NS; cur->at = cur->at + 0x2; }
        else if (cur->text[cur->at] == 'm' && cur->text[cur->at + 0x1] == 's') { scale_ns = CALC_TIME_MS_NS; cur->at = cur->at + 0x2; }
        else if (cur->text[cur->at] == 's' && calc_ident_rest_is(cur->text[cur->at + 0x1]) != LDG_TRUTH_TRUE) { scale_ns = CALC_TIME_S_NS; cur->at = cur->at + 0x1; } }

    if (scale_ns > 0.0)
    {
        double secs = ((double)val + frac) * scale_ns / CALC_TIME_S_NS;

        if (env->rate_hz == 0) { return CROSSTALK_EDA_UI_CALC_ERR_NO_CLK; }

        *out = (uint64_t)(secs * (double)env->rate_hz + 0.5);
        return LDG_ERR_AOK;
    }

    *out = val;
    return LDG_ERR_AOK;
}

static uint32_t calc_pow_get(uint64_t base, uint64_t exp, uint64_t *out)
{
    uint64_t res = 0x1;
    uint64_t at = 0;

    *out = 0;

    if (exp > (uint64_t)CALC_POW_EXP_MAX) { return LDG_ERR_RANGE; }

    for (at = 0; at < exp; at = at + 0x1) { res = res * base; }

    *out = res;

    return LDG_ERR_AOK;
}

static uint32_t calc_op_peek(const calc_cursor_t *cur, uint32_t *out_prec, uint32_t *out_len, char *out_op)
{
    char first = cur->text[cur->at];
    char second = cur->text[cur->at + 0x1];

    *out_prec = 0;
    *out_len = 0;
    *out_op = first;

    if (first == 'x' && second == 'o' && cur->text[cur->at + 0x2] == 'r' && calc_ident_rest_is(cur->text[cur->at + 0x3]) != LDG_TRUTH_TRUE)
    {
        *out_prec = CALC_PREC_XOR;
        *out_len = 0x3;
        *out_op = CALC_OP_XOR;

        return LDG_ERR_AOK;
    }

    if (first == '*' && second == '*') { *out_prec = CALC_PREC_POW; *out_len = 0x2; *out_op = CALC_OP_POW; return LDG_ERR_AOK; }

    if (first == '<' && second == '<') { *out_prec = CALC_PREC_SHIFT; *out_len = 0x2; return LDG_ERR_AOK; }

    if (first == '>' && second == '>') { *out_prec = CALC_PREC_SHIFT; *out_len = 0x2; return LDG_ERR_AOK; }

    if (first == '|') { *out_prec = CALC_PREC_LOR; *out_len = 0x1; return LDG_ERR_AOK; }

    if (first == '^') { *out_prec = CALC_PREC_POW; *out_len = 0x1; return LDG_ERR_AOK; }

    if (first == '&') { *out_prec = CALC_PREC_LAND; *out_len = 0x1; return LDG_ERR_AOK; }

    if (first == '+' || first == '-') { *out_prec = CALC_PREC_ADD; *out_len = 0x1; return LDG_ERR_AOK; }

    if (first == '*' || first == '/' || first == '%') { *out_prec = CALC_PREC_MUL; *out_len = 0x1; return LDG_ERR_AOK; }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t calc_expr_parse(const calc_env_t *env, calc_cursor_t *cur, uint32_t min_prec, uint64_t *out)
{
    uint64_t lhs = 0;
    uint32_t ret = 0;

    *out = 0;

    calc_space_skip(cur);
    if (cur->text[cur->at] == '~')
    {
        cur->at = cur->at + 0x1;
        ret = calc_expr_parse(env, cur, CALC_PREC_UNARY, &lhs);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        lhs = ~lhs;
    }
    else if (cur->text[cur->at] == '-')
    {
        cur->at = cur->at + 0x1;
        ret = calc_expr_parse(env, cur, CALC_PREC_UNARY, &lhs);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        lhs = ~lhs + 0x1;
    }
    else if (cur->text[cur->at] == '(')
    {
        cur->at = cur->at + 0x1;
        ret = calc_expr_parse(env, cur, CALC_PREC_LOR, &lhs);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        calc_space_skip(cur);
        if (cur->text[cur->at] != ')') { return CROSSTALK_EDA_UI_CALC_ERR_PARSE; }

        cur->at = cur->at + 0x1;
    }
    else if (calc_ident_first_is(cur->text[cur->at]) == LDG_TRUTH_TRUE)
    {
        char name[CALC_NAME_CAP] = LDG_ARR_ZERO_INIT;

        ret = calc_ident_parse(cur, name, (uint32_t)sizeof(name));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        calc_space_skip(cur);
        if (cur->text[cur->at] == '[')
        {
            uint64_t first_idx = 0;

            cur->at = cur->at + 0x1;
            ret = calc_expr_parse(env, cur, CALC_PREC_LOR, &first_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            calc_space_skip(cur);
            if (cur->text[cur->at] == ':')
            {
                uint64_t lo = 0;
                uint64_t hi = first_idx;
                uint64_t span = 0;

                cur->at = cur->at + 0x1;
                ret = calc_expr_parse(env, cur, CALC_PREC_LOR, &lo);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                calc_space_skip(cur);
                if (cur->text[cur->at] != ']') { return CROSSTALK_EDA_UI_CALC_ERR_PARSE; }

                cur->at = cur->at + 0x1;

                if (hi > (uint64_t)CALC_BIT_MAX) { hi = CALC_BIT_MAX; }

                if (lo > hi) { return CROSSTALK_EDA_UI_CALC_ERR_PARSE; }

                ret = calc_ident_val_get(env, name, &lhs);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                span = hi - lo + 0x1;
                lhs = span >= (uint64_t)CALC_SHIFT_BITS_MAX ? lhs >> lo : (lhs >> lo) & (((uint64_t)0x1 << span) - 0x1);
            }
            else
            {
                uint64_t word = 0;
                uint32_t got = 0;

                if (cur->text[cur->at] != ']') { return CROSSTALK_EDA_UI_CALC_ERR_PARSE; }

                cur->at = cur->at + 0x1;

                if (env->calc->fpga != 0x0 && crosstalk_eda_ic_fpga_mem_rd(env->calc->fpga, env->calc->slot, name, first_idx, &word, 0x1, &got) == LDG_ERR_AOK && got == 0x1) { lhs = word; }
                else
                {
                    ret = calc_ident_val_get(env, name, &lhs);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                    lhs = (lhs >> (first_idx & (uint64_t)CALC_BIT_MAX)) & 0x1;
                }
            }
        }
        else
        {
            ret = calc_ident_val_get(env, name, &lhs);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }
    else
    {
        ret = calc_num_parse(env, cur, &lhs);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (;;)
    {
        uint64_t rhs = 0;
        uint32_t prec = 0;
        uint32_t op_len = 0;
        char op = 0;

        calc_space_skip(cur);
        ret = calc_op_peek(cur, &prec, &op_len, &op);
        if (ret != LDG_ERR_AOK) { break; }

        if (prec < min_prec) { break; }

        cur->at = cur->at + op_len;
        ret = calc_expr_parse(env, cur, op == CALC_OP_POW ? prec : prec + 0x1, &rhs);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (op == '|') { lhs = lhs | rhs; }
        else if (op == CALC_OP_XOR) { lhs = lhs ^ rhs; }
        else if (op == CALC_OP_POW)
        {
            ret = calc_pow_get(lhs, rhs, &lhs);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
        else if (op == '&') { lhs = lhs & rhs; }
        else if (op == '<') { lhs = rhs < (uint64_t)CALC_SHIFT_BITS_MAX ? lhs << rhs : (uint64_t)CALC_SHIFT_RES_EMPTY; }
        else if (op == '>') { lhs = rhs < (uint64_t)CALC_SHIFT_BITS_MAX ? lhs >> rhs : (uint64_t)CALC_SHIFT_RES_EMPTY; }
        else if (op == '+') { lhs = lhs + rhs; }
        else if (op == '-') { lhs = lhs - rhs; }
        else if (op == '*') { lhs = lhs * rhs; }
        else if (op == '/')
        {
            if (rhs == 0) { return CROSSTALK_EDA_UI_CALC_ERR_DIV_ZERO; }

            lhs = lhs / rhs;
        }
        else if (op == '%')
        {
            if (rhs == 0) { return CROSSTALK_EDA_UI_CALC_ERR_DIV_ZERO; }

            lhs = lhs % rhs;
        }
    }

    *out = lhs;
    return LDG_ERR_AOK;
}

static void calc_env_fill(const crosstalk_eda_ui_calc_ctx_t *ctx, calc_env_t *env)
{
    uint64_t pin = 0;
    uint64_t rate = 0;

    env->calc = ctx;
    env->rate_hz = 0;

    if (ctx->run == 0x0) { return; }

    if (crosstalk_eda_ic_fpga_run_pin_get(ctx->run, &pin) == LDG_ERR_AOK && pin != 0)
    {
        env->rate_hz = pin;
        return;
    }

    if (crosstalk_eda_ic_fpga_run_rate_get(ctx->run, &rate) == LDG_ERR_AOK) { env->rate_hz = rate; }
}

static uint32_t calc_eval(const crosstalk_eda_ui_calc_ctx_t *ctx, const char *expr, uint64_t *out)
{
    calc_env_t env = LDG_STRUCT_ZERO_INIT;
    calc_cursor_t cur = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    calc_env_fill(ctx, &env);

    cur.text = expr;
    ret = calc_expr_parse(&env, &cur, CALC_PREC_LOR, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    calc_space_skip(&cur);
    if (cur.text[cur.at] != LDG_STR_TERM) { return CROSSTALK_EDA_UI_CALC_ERR_PARSE; }

    return LDG_ERR_AOK;
}

static uint32_t calc_bin_fmt(uint64_t val, char *out, uint32_t cap)
{
    char bit_hex[3] = LDG_ARR_ZERO_INIT;
    uint32_t nibble_cunt = 0x1;
    uint32_t used = 0;
    uint32_t idx = 0;

    while (nibble_cunt < (uint32_t)(sizeof(uint64_t) * 0x2) && (val >> (nibble_cunt * CALC_NIBBLE_GROUP)) != 0) { nibble_cunt = nibble_cunt + 0x1; }

    for (idx = 0; idx < nibble_cunt * CALC_NIBBLE_GROUP; idx = idx + 0x1)
    {
        uint32_t bit = nibble_cunt * CALC_NIBBLE_GROUP - 0x1 - idx;

        if (idx != 0 && bit % CALC_NIBBLE_GROUP == 0x3)
        {
            if (used + LDG_STR_TERM_SIZE >= cap) { return LDG_ERR_STR_TRUNC; }

            out[used] = ' ';
            used = used + 0x1;
        }

        if (used + LDG_STR_TERM_SIZE >= cap) { return LDG_ERR_STR_TRUNC; }

        if (ldg_byte_to_hex((byte_t)((val >> bit) & 0x1), bit_hex) != LDG_ERR_AOK) { return LDG_ERR_STR_FMT; }

        out[used] = bit_hex[0x1];
        used = used + 0x1;
    }

    out[used] = LDG_STR_TERM;
    return LDG_ERR_AOK;
}

static uint32_t calc_width_get(uint64_t val)
{
    uint32_t width = 0x1;

    while (width < (uint32_t)CALC_SHIFT_BITS_MAX && (val >> width) != 0) { width = width + 0x1; }

    return width;
}

static uint64_t calc_align_get(uint64_t val)
{
    uint64_t align = 0;

    if (val == 0) { return align; }

    align = val & (~val + 0x1);

    return align;
}

static uint32_t calc_ones_get(uint64_t val)
{
    uint32_t ones = 0;

    while (val != 0)
    {
        ones = ones + (uint32_t)(val & 0x1);
        val = val >> 0x1;
    }

    return ones;
}

static void calc_ascii_fmt(uint64_t val, char *out, uint32_t cap)
{
    uint32_t byte_cunt = (calc_width_get(val) + (uint32_t)LDG_BYTE_BITS - 0x1) / (uint32_t)LDG_BYTE_BITS;
    uint32_t idx = 0;
    uint32_t used = 0;

    for (idx = 0; idx < byte_cunt && used + LDG_STR_TERM_SIZE < cap; idx = idx + 0x1)
    {
        uint8_t by = (uint8_t)(val >> (idx * (uint32_t)LDG_BYTE_BITS));

        out[used] = by >= (uint8_t)LDG_ASCII_PRINTABLE_MIN && by <= (uint8_t)LDG_ASCII_PRINTABLE_MAX ? (char)by : '.';
        used = used + 0x1;
    }

    out[used] = LDG_STR_TERM;
}

static uint32_t calc_bytes_fmt(uint64_t val, uint32_t byte_cunt, uint8_t rev, char *out, uint32_t cap)
{
    uint32_t used = 0;
    uint32_t idx = 0;

    out[0] = LDG_STR_TERM;

    for (idx = 0; idx < byte_cunt; idx = idx + 0x1)
    {
        uint32_t take = rev == LDG_TRUTH_TRUE ? byte_cunt - 0x1 - idx : idx;
        uint8_t by = (uint8_t)(val >> (take * (uint32_t)LDG_BYTE_BITS));
        int32_t wrote = snprintf(out + used, cap - used, idx == 0 ? CALC_BYTE_FMT : CALC_BYTE_GAP_FMT, (uint32_t)by);

        if (wrote < 0 || (uint32_t)wrote >= cap - used) { return LDG_ERR_STR_TRUNC; }

        used = used + (uint32_t)wrote;
    }

    return LDG_ERR_AOK;
}

static uint32_t calc_text_parse(const char *expr, uint8_t *out, uint32_t cap, uint32_t *out_len)
{
    uint32_t at = LDG_STR_TERM_SIZE;
    uint32_t used = 0;

    *out_len = 0;

    while (expr[at] != LDG_STR_TERM && expr[at] != LDG_ASCII_QUOTE && used < cap)
    {
        out[used] = (uint8_t)expr[at];
        used = used + 0x1;
        at = at + 0x1;
    }

    if (used == 0) { return LDG_ERR_EMPTY; }

    *out_len = used;

    return LDG_ERR_AOK;
}

static uint32_t calc_text_hex_fmt(const uint8_t *bytes, uint32_t len, uint8_t rev, char *out, uint32_t cap)
{
    uint32_t used = 0;
    uint32_t idx = 0;

    out[0] = LDG_STR_TERM;

    for (idx = 0; idx < len; idx = idx + 0x1)
    {
        uint32_t take = rev == LDG_TRUTH_TRUE ? len - 0x1 - idx : idx;
        int32_t wrote = snprintf(out + used, cap - used, idx == 0 ? CALC_BYTE_FMT : CALC_BYTE_GAP_FMT, (uint32_t)bytes[take]);

        if (wrote < 0 || (uint32_t)wrote >= cap - used) { return LDG_ERR_STR_TRUNC; }

        used = used + (uint32_t)wrote;
    }

    return LDG_ERR_AOK;
}

static void calc_whole_mark(char *out, uint32_t cap)
{
    uint32_t len = (uint32_t)strnlen(out, (uint64_t)cap);
    uint32_t at = 0;

    for (at = 0; at < len; at = at + 0x1) { if (out[at] == '.' || out[at] == 'e' || out[at] == 'n' || out[at] == 'i') { return; } }

    if (len + (uint32_t)CALC_FP_FRAC_CHARS + LDG_STR_TERM_SIZE > cap) { return; }

    if (ldg_str_rbrcpy(out + len, CALC_FP_FRAC_SUFFIX, (uint64_t)(cap - len)) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "calc whole mark failed"); }
}

static uint32_t calc_fp32_fmt(uint64_t val, char *out, uint32_t cap)
{
    uint64_t bits = val & (uint64_t)CALC_FP32_MASK;
    uint32_t mant = (uint32_t)(bits & (uint64_t)CALC_FP32_MANT_MASK);
    uint32_t expo = (uint32_t)((bits >> (uint32_t)CALC_FP32_EXP_SHIFT) & (uint64_t)CALC_FP32_EXP_MASK);
    uint8_t neg_is = ((bits >> (uint32_t)CALC_FP32_SIGN_SHIFT) & 0x1) != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    double num = 0.0;

    if (expo == (uint32_t)CALC_FP32_EXP_MASK)
    {
        const char *special = CALC_FP_INF_TEXT;

        if (mant != 0) { special = CALC_FP_NAN_TEXT; }
        else if (neg_is == LDG_TRUTH_TRUE) { special = CALC_FP_NEG_INF_TEXT; }

        return ldg_str_rbrcpy(out, special, (uint64_t)cap);
    }

    if (expo == 0) { num = ldexp((double)mant / CALC_FP32_MANT_SCALE, (int32_t)CALC_FP32_SUBNORM_EXP); }
    else { num = ldexp(1.0 + (double)mant / CALC_FP32_MANT_SCALE, (int32_t)expo - (int32_t)CALC_FP32_BIAS); }

    if (neg_is == LDG_TRUTH_TRUE) { num = 0.0 - num; }

    if (snprintf(out, cap, "%.*g", (int32_t)CALC_FP32_DIGITS, num) < 0) { return LDG_ERR_STR_FMT; }

    calc_whole_mark(out, cap);

    return LDG_ERR_AOK;
}

static uint32_t calc_fixed_fmt(uint64_t val, char *out, uint32_t cap)
{
    int32_t raw = (int32_t)(uint32_t)(val & (uint64_t)CALC_FP32_MASK);
    double num = (double)raw / CALC_FIXED_SCALE;

    if (snprintf(out, cap, "%.*g", (int32_t)CALC_FIXED_DIGITS, num) < 0) { return LDG_ERR_STR_FMT; }

    calc_whole_mark(out, cap);

    return LDG_ERR_AOK;
}

static uint32_t calc_fp64_fmt(uint64_t val, char *out, uint32_t cap)
{
    double num = 0.0;

    memcpy(&num, &val, sizeof(num));

    if (snprintf(out, cap, "%.*g", (int32_t)CALC_FP64_DIGITS, num) < 0) { return LDG_ERR_STR_FMT; }

    calc_whole_mark(out, cap);

    return LDG_ERR_AOK;
}

static uint32_t calc_time_fmt(uint64_t cycles, uint64_t rate_hz, char *out, uint32_t cap)
{
    double secs = 0.0;

    if (rate_hz == 0) { out[0] = LDG_STR_TERM; return LDG_ERR_AOK; }

    secs = (double)cycles / (double)rate_hz;

    if (secs < CALC_TIME_US_NS / CALC_TIME_S_NS) { return snprintf(out, cap, "%.4g ns @ %" PRIu64 " Hz", secs * CALC_TIME_S_NS, rate_hz) < 0 ? LDG_ERR_STR_FMT : LDG_ERR_AOK; }

    if (secs < CALC_TIME_MS_NS / CALC_TIME_S_NS) { return snprintf(out, cap, "%.4g us @ %" PRIu64 " Hz", secs * CALC_TIME_S_NS / CALC_TIME_US_NS, rate_hz) < 0 ? LDG_ERR_STR_FMT : LDG_ERR_AOK; }

    if (secs < 1.0) { return snprintf(out, cap, "%.4g ms @ %" PRIu64 " Hz", secs * CALC_TIME_S_NS / CALC_TIME_MS_NS, rate_hz) < 0 ? LDG_ERR_STR_FMT : LDG_ERR_AOK; }

    return snprintf(out, cap, "%.4g s @ %" PRIu64 " Hz", secs, rate_hz) < 0 ? LDG_ERR_STR_FMT : LDG_ERR_AOK;
}

static void calc_map_fmt(const crosstalk_eda_ui_calc_ctx_t *ctx, uint64_t val, char *out, uint32_t cap)
{
    const crosstalk_hdl_ir_t *ir = 0x0;
    const char *src = 0x0;
    uint32_t idx = 0;

    out[0] = LDG_STR_TERM;

    if (ctx->fpga == 0x0) { return; }

    if (crosstalk_eda_ic_fpga_slot_ir_get(ctx->fpga, ctx->slot, &ir, &src) != LDG_ERR_AOK || ir == 0x0 || src == 0x0) { return; }

    for (idx = 0; idx < ir->sig_cunt; idx = idx + 0x1)
    {
        char name[CALC_NAME_CAP] = LDG_ARR_ZERO_INIT;
        uint64_t base = 0;
        uint64_t span = 0;
        uint32_t name_len = 0;

        if (ir->sigs[idx].kind != CROSSTALK_HDL_IR_SIG_MEM) { continue; }

        name_len = ir->sigs[idx].name.len;
        if (name_len == 0 || name_len >= (uint32_t)sizeof(name)) { continue; }

        memcpy(name, src + ir->sigs[idx].name.off, name_len);
        name[name_len] = LDG_STR_TERM;

        if (crosstalk_eda_ic_fpga_mem_offset_get(ctx->fpga, ctx->slot, name, &base) != LDG_ERR_AOK) { continue; }

        span = (uint64_t)ir->sigs[idx].depth * (uint64_t)CALC_MEM_WORD_BYTES;
        if (val < base || val >= base + span) { continue; }

        if (snprintf(out, cap, "map: %.47s word 0x%" PRIX64, name, (val - base) / (uint64_t)CALC_MEM_WORD_BYTES) < 0) { out[0] = LDG_STR_TERM; }

        return;
    }
}

static uint32_t calc_dis_text(uint64_t val, char *out, uint32_t cap)
{
    ldg_dangcsa_decoded_t decoded = LDG_STRUCT_ZERO_INIT;
    char dangcsa_piece[CALC_DIS_PIECE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    ret = ldg_dangcsa_decode(val, &decoded);
    if (ret == LDG_ERR_AOK) { ret = ldg_dangcsa_disasm(&decoded, dangcsa_piece, (uint64_t)sizeof(dangcsa_piece)); }

    if (ret != LDG_ERR_AOK) { snprintf(dangcsa_piece, sizeof(dangcsa_piece), "invalid"); }

    return snprintf(out, cap, "dangcsa: %.40s", dangcsa_piece) < 0 ? LDG_ERR_STR_FMT : LDG_ERR_AOK;
}

static void calc_hist_push(crosstalk_eda_ui_calc_ctx_t *ctx, const char *expr, const char *text, uint64_t val, uint8_t text_is)
{
    crosstalk_eda_ui_calc_hist_t *rec = 0x0;

    if (ctx->hist_cunt >= (uint32_t)CROSSTALK_EDA_UI_CALC_HIST_CAP)
    {
        memmove(&ctx->hist[0], &ctx->hist[0x1], sizeof(ctx->hist) - sizeof(ctx->hist[0]));
        ctx->hist_cunt = (uint32_t)CROSSTALK_EDA_UI_CALC_HIST_CAP - 0x1;
    }

    rec = &ctx->hist[ctx->hist_cunt];
    memset(rec, 0x0, sizeof(*rec));

    if (snprintf(rec->expr, sizeof(rec->expr), "%.63s", expr) < 0) { rec->expr[0] = LDG_STR_TERM; }

    if (text != 0x0) { if (snprintf(rec->text, sizeof(rec->text), "%.95s", text) < 0) { rec->text[0] = LDG_STR_TERM; } }

    rec->val = val;
    rec->text_is = text_is;
    ctx->hist_cunt = ctx->hist_cunt + 0x1;
}

static void calc_submit(crosstalk_eda_ui_calc_ctx_t *ctx)
{
    calc_cursor_t cur = LDG_STRUCT_ZERO_INIT;
    char name[CROSSTALK_EDA_UI_CALC_VAR_NAME_CAP] = LDG_ARR_ZERO_INIT;
    char text[CROSSTALK_EDA_UI_CALC_HIST_TEXT_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t val = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (ctx->expr[0] == LDG_STR_TERM) { return; }

    if (memcmp(ctx->expr, CALC_DIS_TOK, CALC_DIS_TOK_LEN) == 0x0)
    {
        ret = calc_eval(ctx, ctx->expr + CALC_DIS_TOK_LEN, &val);
        if (ret == LDG_ERR_AOK)
        {
            if (calc_dis_text(val, text, (uint32_t)sizeof(text)) != LDG_ERR_AOK) { return; }

            calc_hist_push(ctx, ctx->expr, text, val, LDG_TRUTH_TRUE);
            ctx->res_last = val;
            ctx->res_last_ok = LDG_TRUTH_TRUE;
            ctx->expr[0] = LDG_STR_TERM;
        }

        return;
    }

    cur.text = ctx->expr;
    calc_space_skip(&cur);
    if (calc_ident_first_is(cur.text[cur.at]) == LDG_TRUTH_TRUE && calc_ident_parse(&cur, name, (uint32_t)sizeof(name)) == LDG_ERR_AOK)
    {
        calc_space_skip(&cur);
        if (cur.text[cur.at] == '=' && cur.text[cur.at + 0x1] != '=' && strcmp(name, CALC_RES_NAME) != 0x0)
        {
            cur.at = cur.at + 0x1;
            ret = calc_eval(ctx, cur.text + cur.at, &val);
            if (ret != LDG_ERR_AOK) { return; }

            if (calc_var_find(ctx, name, &idx) != LDG_TRUTH_TRUE)
            {
                if (ctx->var_cunt >= (uint32_t)CROSSTALK_EDA_UI_CALC_VAR_CAP) { return; }

                idx = ctx->var_cunt;
                ctx->var_cunt = ctx->var_cunt + 0x1;
                if (snprintf(ctx->var[idx].name, sizeof(ctx->var[idx].name), "%.23s", name) < 0) { ctx->var[idx].name[0] = LDG_STR_TERM; }
            }

            ctx->var[idx].val = val;
            calc_hist_push(ctx, ctx->expr, 0x0, val, LDG_TRUTH_FALSE);
            ctx->res_last = val;
            ctx->res_last_ok = LDG_TRUTH_TRUE;
            ctx->expr[0] = LDG_STR_TERM;
            return;
        }
    }

    ret = calc_eval(ctx, ctx->expr, &val);
    if (ret != LDG_ERR_AOK) { return; }

    calc_hist_push(ctx, ctx->expr, 0x0, val, LDG_TRUTH_FALSE);
    ctx->res_last = val;
    ctx->res_last_ok = LDG_TRUTH_TRUE;
    ctx->expr[0] = LDG_STR_TERM;
}

uint32_t crosstalk_eda_ui_calc_init(crosstalk_eda_ui_calc_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->win.open = LDG_TRUTH_FALSE;
    ctx->win.scroll_on = LDG_TRUTH_TRUE;
    ctx->open = LDG_TRUTH_FALSE;
    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_calc_bind(crosstalk_eda_ui_calc_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, const crosstalk_eda_ic_fpga_run_ctx_t *run, uint32_t slot)
{
    if (LDG_UNLIKELY(ctx == 0x0 || fpga == 0x0 || run == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->fpga = fpga;
    ctx->run = run;
    ctx->slot = slot;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_calc_open_set(crosstalk_eda_ui_calc_ctx_t *ctx, uint8_t open)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->open = open;
    ctx->win.open = open;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_ui_calc_open_is(const crosstalk_eda_ui_calc_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->win.open != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_eda_ui_calc_var_get(const crosstalk_eda_ui_calc_ctx_t *ctx, const char *name, uint64_t *out)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (calc_var_find(ctx, name, &idx) != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_FOUND; }

    *out = ctx->var[idx].val;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_calc_focus_set(crosstalk_eda_ui_calc_ctx_t *ctx, uint8_t focused)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->edit = focused == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_calc_route_set(crosstalk_eda_ui_calc_ctx_t *ctx, uint8_t focused)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->focused = focused == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_calc_var_at_get(const crosstalk_eda_ui_calc_ctx_t *ctx, uint64_t addr, char *out, uint32_t cap)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0 || cap == 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    out[0] = LDG_STR_TERM;

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    for (idx = 0; idx < ctx->var_cunt; idx = idx + 0x1)
    {
        if (ctx->var[idx].val != addr) { continue; }

        return ldg_str_rbrcpy(out, ctx->var[idx].name, (uint64_t)cap);
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t crosstalk_eda_ui_calc_state_serialize(const crosstalk_eda_ui_calc_ctx_t *ctx, uint8_t *out, uint32_t cap, uint32_t *out_len)
{
    uint32_t need = 0;
    uint32_t off = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (ctx->var_cunt == 0 && ctx->hist_cunt == 0) { return LDG_ERR_AOK; }

    need = (uint32_t)(sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(ctx->var) + sizeof(ctx->hist));
    *out_len = need;

    if (out == 0x0) { return LDG_ERR_AOK; }

    if (cap < need) { return LDG_ERR_BOUNDS; }

    memcpy(out + off, &ctx->var_cunt, sizeof(uint32_t));
    off = off + (uint32_t)sizeof(uint32_t);
    memcpy(out + off, &ctx->hist_cunt, sizeof(uint32_t));
    off = off + (uint32_t)sizeof(uint32_t);
    memcpy(out + off, &ctx->res_last, sizeof(uint64_t));
    off = off + (uint32_t)sizeof(uint64_t);
    memcpy(out + off, ctx->var, sizeof(ctx->var));
    off = off + (uint32_t)sizeof(ctx->var);
    memcpy(out + off, ctx->hist, sizeof(ctx->hist));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_calc_state_deserialize(crosstalk_eda_ui_calc_ctx_t *ctx, const uint8_t *buff, uint32_t len)
{
    uint32_t need = 0;
    uint32_t off = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || buff == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    need = (uint32_t)(sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(ctx->var) + sizeof(ctx->hist));
    if (len != need) { return LDG_ERR_INVALID; }

    memcpy(&ctx->var_cunt, buff + off, sizeof(uint32_t));
    off = off + (uint32_t)sizeof(uint32_t);
    memcpy(&ctx->hist_cunt, buff + off, sizeof(uint32_t));
    off = off + (uint32_t)sizeof(uint32_t);
    memcpy(&ctx->res_last, buff + off, sizeof(uint64_t));
    off = off + (uint32_t)sizeof(uint64_t);
    memcpy(ctx->var, buff + off, sizeof(ctx->var));
    off = off + (uint32_t)sizeof(ctx->var);
    memcpy(ctx->hist, buff + off, sizeof(ctx->hist));

    if (ctx->var_cunt > (uint32_t)CROSSTALK_EDA_UI_CALC_VAR_CAP) { ctx->var_cunt = CROSSTALK_EDA_UI_CALC_VAR_CAP; }

    if (ctx->hist_cunt > (uint32_t)CROSSTALK_EDA_UI_CALC_HIST_CAP) { ctx->hist_cunt = CROSSTALK_EDA_UI_CALC_HIST_CAP; }

    for (idx = 0; idx < (uint32_t)CROSSTALK_EDA_UI_CALC_VAR_CAP; idx = idx + 0x1) { ctx->var[idx].name[sizeof(ctx->var[idx].name) - LDG_STR_TERM_SIZE] = LDG_STR_TERM; }

    for (idx = 0; idx < (uint32_t)CROSSTALK_EDA_UI_CALC_HIST_CAP; idx = idx + 0x1)
    {
        ctx->hist[idx].expr[sizeof(ctx->hist[idx].expr) - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
        ctx->hist[idx].text[sizeof(ctx->hist[idx].text) - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
    }

    ctx->res_last_ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t calc_key_isr(uint64_t win_id, void *user, const ldg_term_key_event_t *key, uint8_t *consumed)
{
    crosstalk_eda_ui_calc_ctx_t *ctx = (crosstalk_eda_ui_calc_ctx_t *)user;
    uint64_t len = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || key == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    if (win_id != (uint64_t)CROSSTALK_EDA_UI_CALC_WINDOW_ID) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (key->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE) { return LDG_ERR_AOK; }

    if (key->key == (uint8_t)LDG_TERM_KEY_ENTER) { calc_submit(ctx); *consumed = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

    if (key->key == (uint8_t)LDG_TERM_KEY_ESCAPE) { ctx->expr[0] = LDG_STR_TERM; *consumed = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

    ret = ldg_str_len_get(ctx->expr, (uint64_t)sizeof(ctx->expr), &len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (key->key == (uint8_t)LDG_TERM_KEY_BACKSPACE)
    {
        if (len != 0) { ctx->expr[len - (uint64_t)CALC_STEP] = LDG_STR_TERM; }

        *consumed = LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    for (at = 0; at < key->text_cunt && at < (uint32_t)LDG_TERM_KEY_TEXT_MAX; at = at + (uint32_t)CALC_STEP)
    {
        if (key->text[at] < (uint32_t)LDG_ASCII_PRINTABLE_MIN || key->text[at] > (uint32_t)LDG_ASCII_PRINTABLE_MAX) { continue; }

        *consumed = LDG_TRUTH_TRUE;

        if (len + (uint64_t)LDG_STR_TERM_SIZE >= (uint64_t)sizeof(ctx->expr)) { break; }

        ctx->expr[len] = (char)key->text[at];
        len = len + (uint64_t)CALC_STEP;
        ctx->expr[len] = LDG_STR_TERM;
    }

    return LDG_ERR_AOK;
}

static uint32_t calc_line_put(ldg_term_render_t *render, uint32_t *row, const char *text, uint32_t rgba)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || row == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)CALC_ORIG_COL, *row, text, (uint64_t)CROSSTALK_EDA_UI_CALC_LINE_CAP, rgba, 0x0);
    if (ret != LDG_ERR_AOK) { return ret; }

    *row = *row + (uint32_t)CALC_STEP;

    return LDG_ERR_AOK;
}

static uint32_t calc_draw_cb(uint64_t win_id, void *user, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    crosstalk_eda_ui_calc_ctx_t *ctx = (crosstalk_eda_ui_calc_ctx_t *)user;
    char line[CROSSTALK_EDA_UI_CALC_LINE_CAP] = LDG_ARR_ZERO_INIT;
    char piece[CROSSTALK_EDA_UI_CALC_LINE_CAP] = LDG_ARR_ZERO_INIT;
    calc_env_t env = LDG_STRUCT_ZERO_INIT;
    const uint32_t label_col = CROSSTALK_UI_THEME_DIM_RGBA;
    const uint32_t val_col = CROSSTALK_UI_THEME_LIVE_RGBA;
    const uint32_t fail_col = CROSSTALK_UI_THEME_DANGER_RGBA;
    const uint32_t hist_col = CROSSTALK_UI_THEME_STATUS_RGBA;
    uint32_t prompt_w = 0;
    uint32_t row = 0;
    uint8_t live_is = LDG_TRUTH_TRUE;
    uint32_t idx = 0;
    uint32_t shown = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (win_id != (uint64_t)CROSSTALK_EDA_UI_CALC_WINDOW_ID || col_cunt == 0 || row_cunt == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)CALC_ORIG_COL, row, CALC_PROMPT_TEXT, (uint64_t)sizeof(CALC_PROMPT_TEXT), (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, &prompt_w);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_rgba_put(render, prompt_w, row, ctx->expr, (uint64_t)sizeof(ctx->expr), (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, 0x0); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    row = row + (uint32_t)CALC_STEP;

    if (ctx->expr[0] == LDG_ASCII_QUOTE)
    {
        uint8_t bytes[CALC_TEXT_CAP] = LDG_ARR_ZERO_INIT;
        uint32_t len = 0;

        if (calc_text_parse(ctx->expr, bytes, (uint32_t)sizeof(bytes), &len) == LDG_ERR_AOK)
        {
            if (calc_text_hex_fmt(bytes, len, LDG_TRUTH_FALSE, piece, (uint32_t)sizeof(piece)) == LDG_ERR_AOK && snprintf(line, sizeof(line), CALC_ORDER_LE_FMT, piece) > 0)
            {
                ret = calc_line_put(render, &row, line, val_col);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            if (calc_text_hex_fmt(bytes, len, LDG_TRUTH_TRUE, piece, (uint32_t)sizeof(piece)) == LDG_ERR_AOK && snprintf(line, sizeof(line), CALC_ORDER_BE_FMT, piece) > 0)
            {
                ret = calc_line_put(render, &row, line, val_col);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            if (snprintf(line, sizeof(line), CALC_TEXT_LEN_FMT, len, (int32_t)len, &ctx->expr[LDG_STR_TERM_SIZE]) > 0)
            {
                ret = calc_line_put(render, &row, line, label_col);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
        }

        live_is = LDG_TRUTH_FALSE;
    }

    if (memcmp(ctx->expr, CALC_DIS_TOK, CALC_DIS_TOK_LEN) == 0x0) { live_is = LDG_TRUTH_FALSE; }

    for (idx = 0; ctx->expr[idx] != LDG_STR_TERM; idx = idx + 0x1) { if (ctx->expr[idx] == '=') { live_is = LDG_TRUTH_FALSE; break; } }

    ctx->res_ok = LDG_TRUTH_FALSE;
    if (ctx->expr[0] != LDG_STR_TERM && live_is == LDG_TRUTH_TRUE)
    {
        ctx->ret = calc_eval(ctx, ctx->expr, &ctx->res);
        if (ctx->ret == LDG_ERR_AOK) { ctx->res_ok = LDG_TRUTH_TRUE; }
    }
    else if (ctx->expr[0] == LDG_STR_TERM && ctx->res_last_ok == LDG_TRUTH_TRUE)
    {
        ctx->res = ctx->res_last;
        ctx->res_ok = LDG_TRUTH_TRUE;
    }

    if (ctx->res_ok == LDG_TRUTH_TRUE)
    {
        calc_env_fill(ctx, &env);

        if (snprintf(line, sizeof(line), "hex: 0x%" PRIX64, ctx->res) < 0) { return LDG_ERR_STR_FMT; }

        ret = calc_line_put(render, &row, line, val_col);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        {
            uint32_t byte_cunt = (calc_width_get(ctx->res) + (uint32_t)LDG_BYTE_BITS - 0x1) / (uint32_t)LDG_BYTE_BITS;

            if (byte_cunt == 0) { byte_cunt = LDG_STR_TERM_SIZE; }

            if (calc_bytes_fmt(ctx->res, byte_cunt, LDG_TRUTH_FALSE, piece, (uint32_t)sizeof(piece)) == LDG_ERR_AOK && snprintf(line, sizeof(line), CALC_ORDER_LE_FMT, piece) > 0)
            {
                ret = calc_line_put(render, &row, line, val_col);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            if (calc_bytes_fmt(ctx->res, byte_cunt, LDG_TRUTH_TRUE, piece, (uint32_t)sizeof(piece)) == LDG_ERR_AOK && snprintf(line, sizeof(line), CALC_ORDER_BE_FMT, piece) > 0)
            {
                ret = calc_line_put(render, &row, line, val_col);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
        }

        if ((ctx->res >> CALC_BIT_MAX) != 0) { ret = snprintf(line, sizeof(line), "dec: %" PRIu64 "; sig" "ned: %" PRId64, ctx->res, (int64_t)ctx->res) < 0 ? LDG_ERR_STR_FMT : LDG_ERR_AOK; }
        else { ret = snprintf(line, sizeof(line), "dec: %" PRIu64, ctx->res) < 0 ? LDG_ERR_STR_FMT : LDG_ERR_AOK; }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = calc_line_put(render, &row, line, val_col);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = calc_bin_fmt(ctx->res, piece, (uint32_t)sizeof(piece));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (snprintf(line, sizeof(line), "bin: %.87s", piece) < 0) { return LDG_ERR_STR_FMT; }

        ret = calc_line_put(render, &row, line, val_col);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        calc_ascii_fmt(ctx->res, piece, (uint32_t)sizeof(piece));
        if (snprintf(line, sizeof(line), "width: %u; align: %" PRIu64 "; ones: %u; ascii: %.15s", calc_width_get(ctx->res), calc_align_get(ctx->res), calc_ones_get(ctx->res), piece) < 0) { return LDG_ERR_STR_FMT; }

        ret = calc_line_put(render, &row, line, val_col);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        {
            char fp32_text[CALC_FP_TEXT_CAP] = LDG_ARR_ZERO_INIT;
            char fp64_text[CALC_FP_TEXT_CAP] = LDG_ARR_ZERO_INIT;
            char fixed_text[CALC_FP_TEXT_CAP] = LDG_ARR_ZERO_INIT;

            ret = calc_fp32_fmt(ctx->res, fp32_text, (uint32_t)sizeof(fp32_text));
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = calc_fixed_fmt(ctx->res, fixed_text, (uint32_t)sizeof(fixed_text));
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (calc_width_get(ctx->res) > (uint32_t)CALC_FP32_BITS)
            {
                ret = calc_fp64_fmt(ctx->res, fp64_text, (uint32_t)sizeof(fp64_text));
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                if (snprintf(line, sizeof(line), "fp32: %.39s; fp64: %.39s; fixed: %.39s", fp32_text, fp64_text, fixed_text) < 0) { return LDG_ERR_STR_FMT; }
            }
            else if (snprintf(line, sizeof(line), "fp32: %.39s; fixed: %.39s", fp32_text, fixed_text) < 0) { return LDG_ERR_STR_FMT; }

            ret = calc_line_put(render, &row, line, val_col);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        if (env.rate_hz != 0)
        {
            ret = calc_time_fmt(ctx->res, env.rate_hz, piece, (uint32_t)sizeof(piece));
            if (ret == LDG_ERR_AOK && piece[0] != LDG_STR_TERM)
            {
                if (snprintf(line, sizeof(line), "time: %.87s", piece) < 0) { return LDG_ERR_STR_FMT; }

                ret = calc_line_put(render, &row, line, val_col);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
        }

        calc_map_fmt(ctx, ctx->res, piece, (uint32_t)sizeof(piece));
        if (piece[0] != LDG_STR_TERM)
        {
            ret = calc_line_put(render, &row, piece, val_col);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }
    else if (ctx->expr[0] != LDG_STR_TERM && live_is == LDG_TRUTH_TRUE)
    {
        const char *why = "malformed expression";

        if (ctx->ret == CROSSTALK_EDA_UI_CALC_ERR_DIV_ZERO) { why = "division by zero"; }
        else if (ctx->ret == CROSSTALK_EDA_UI_CALC_ERR_NO_CLK) { why = "no running clock for time literals"; }
        else if (ctx->ret == CROSSTALK_EDA_UI_CALC_ERR_IDENT) { why = "unknown name"; }

        ret = calc_line_put(render, &row, why, fail_col);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (idx = ctx->hist_cunt; idx > 0 && shown < (uint32_t)CALC_HIST_SHOWN_MAX; idx = idx - 0x1)
    {
        const crosstalk_eda_ui_calc_hist_t *rec = &ctx->hist[idx - 0x1];

        if (rec->text_is == LDG_TRUTH_TRUE) { ret = snprintf(line, sizeof(line), "%.40s: %.79s", rec->expr, rec->text) < 0 ? LDG_ERR_STR_FMT : LDG_ERR_AOK; }
        else { ret = snprintf(line, sizeof(line), "%.56s = 0x%" PRIX64 " (%" PRIu64 ")", rec->expr, rec->val, rec->val) < 0 ? LDG_ERR_STR_FMT : LDG_ERR_AOK; }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = calc_line_put(render, &row, line, hist_col);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        shown = shown + (uint32_t)CALC_STEP;
    }

    ret = calc_line_put(render, &row, CALC_HINT_TEXT, label_col);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->win.content_row_cunt = row;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_calc_win_reg(crosstalk_eda_ui_calc_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot)
{
    if (LDG_UNLIKELY(ctx == 0x0 || win_nav == 0x0 || out_slot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_ui_window_reg(win_nav, (uint64_t)CROSSTALK_EDA_UI_CALC_WINDOW_ID, CROSSTALK_EDA_UI_CALC_WINDOW_TITLE, &ctx->win, calc_draw_cb, calc_key_isr, ctx, out_slot);
}
