#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "eda/io/scancode.h"

#define SCANCODE_DIGIT_BASE 0x02
#define SCANCODE_DIGIT_ZERO 0x0B
#define SCANCODE_ASCII_BS 0x08
#define SCANCODE_ASCII_ONE '1'
#define SCANCODE_ASCII_NINE '9'
#define SCANCODE_KEY_S 0x1F
#define SCANCODE_KEY_V 0x2F
#define SCANCODE_KEY_TAB 0x0F

uint32_t crosstalk_eda_io_scancode_map(uint8_t ch, uint8_t *out_code, uint8_t *out_shift)
{
    uint8_t code = CROSSTALK_EDA_IO_SCANCODE_NONE;
    uint8_t shift = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(out_code == 0x0 || out_shift == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_code = CROSSTALK_EDA_IO_SCANCODE_NONE;
    *out_shift = LDG_TRUTH_FALSE;

    if (ch >= 'A' && ch <= 'Z')
    {
        shift = LDG_TRUTH_TRUE;
        ch = (uint8_t)(ch + (uint8_t)LDG_ASCII_CASE_GAP);
    }

    if (ch >= (uint8_t)SCANCODE_ASCII_ONE && ch <= (uint8_t)SCANCODE_ASCII_NINE) { code = (uint8_t)(SCANCODE_DIGIT_BASE + (ch - (uint8_t)SCANCODE_ASCII_ONE)); }
    else if (ch == (uint8_t)LDG_ASCII_DIGIT_ZERO) { code = SCANCODE_DIGIT_ZERO; }
    else
    {
        switch (ch)
        {
            case 'a':
                code = 0x1E;
                break;
            case 'b':
                code = 0x30;
                break;
            case 'c':
                code = 0x2E;
                break;
            case 'd':
                code = 0x20;
                break;
            case 'e':
                code = 0x12;
                break;
            case 'f':
                code = 0x21;
                break;
            case 'g':
                code = 0x22;
                break;
            case 'h':
                code = 0x23;
                break;
            case 'i':
                code = 0x17;
                break;
            case 'j':
                code = 0x24;
                break;
            case 'k':
                code = 0x25;
                break;
            case 'l':
                code = 0x26;
                break;
            case 'm':
                code = 0x32;
                break;
            case 'n':
                code = 0x31;
                break;
            case 'o':
                code = 0x18;
                break;
            case 'p':
                code = 0x19;
                break;
            case 'q':
                code = 0x10;
                break;
            case 'r':
                code = 0x13;
                break;
            case 's':
                code = SCANCODE_KEY_S;
                break;
            case 't':
                code = 0x14;
                break;
            case 'u':
                code = 0x16;
                break;
            case 'v':
                code = SCANCODE_KEY_V;
                break;
            case 'w':
                code = 0x11;
                break;
            case 'x':
                code = 0x2D;
                break;
            case 'y':
                code = 0x15;
                break;
            case 'z':
                code = 0x2C;
                break;
            case (uint8_t)LDG_ASCII_CR:
                code = 0x1C;
                break;
            case (uint8_t)LDG_ASCII_LF:
                code = 0x1C;
                break;
            case (uint8_t)SCANCODE_ASCII_BS:
                code = 0x0E;
                break;
            case (uint8_t)LDG_ASCII_TAB:
                code = SCANCODE_KEY_TAB;
                break;
            case (uint8_t)LDG_ASCII_ESC:
                code = 0x01;
                break;
            case ' ':
                code = 0x39;
                break;
            case '-':
                code = 0x0C;
                break;
            case '=':
                code = 0x0D;
                break;
            case '[':
                code = 0x1A;
                break;
            case ']':
                code = 0x1B;
                break;
            case ';':
                code = 0x27;
                break;
            case '\'':
                code = 0x28;
                break;
            case '`':
                code = 0x29;
                break;
            case '\\':
                code = 0x2B;
                break;
            case ',':
                code = 0x33;
                break;
            case '.':
                code = 0x34;
                break;
            case '/':
                code = 0x35;
                break;
            case '!':
                code = 0x02;
                shift = LDG_TRUTH_TRUE;
                break;
            case '@':
                code = 0x03;
                shift = LDG_TRUTH_TRUE;
                break;
            case '#':
                code = 0x04;
                shift = LDG_TRUTH_TRUE;
                break;
            case '$':
                code = 0x05;
                shift = LDG_TRUTH_TRUE;
                break;
            case '%':
                code = 0x06;
                shift = LDG_TRUTH_TRUE;
                break;
            case '^':
                code = 0x07;
                shift = LDG_TRUTH_TRUE;
                break;
            case '&':
                code = 0x08;
                shift = LDG_TRUTH_TRUE;
                break;
            case '*':
                code = 0x09;
                shift = LDG_TRUTH_TRUE;
                break;
            case '(':
                code = 0x0A;
                shift = LDG_TRUTH_TRUE;
                break;
            case ')':
                code = 0x0B;
                shift = LDG_TRUTH_TRUE;
                break;
            case '_':
                code = 0x0C;
                shift = LDG_TRUTH_TRUE;
                break;
            case '+':
                code = 0x0D;
                shift = LDG_TRUTH_TRUE;
                break;
            case '{':
                code = 0x1A;
                shift = LDG_TRUTH_TRUE;
                break;
            case '}':
                code = 0x1B;
                shift = LDG_TRUTH_TRUE;
                break;
            case ':':
                code = 0x27;
                shift = LDG_TRUTH_TRUE;
                break;
            case '"':
                code = 0x28;
                shift = LDG_TRUTH_TRUE;
                break;
            case '~':
                code = 0x29;
                shift = LDG_TRUTH_TRUE;
                break;
            case '|':
                code = 0x2B;
                shift = LDG_TRUTH_TRUE;
                break;
            case '<':
                code = 0x33;
                shift = LDG_TRUTH_TRUE;
                break;
            case '>':
                code = 0x34;
                shift = LDG_TRUTH_TRUE;
                break;
            case '?':
                code = 0x35;
                shift = LDG_TRUTH_TRUE;
                break;
            default:
                break;
        }
    }

    *out_code = code;
    *out_shift = shift;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_scancode_get(uint8_t ch, uint8_t *out)
{
    uint8_t code = CROSSTALK_EDA_IO_SCANCODE_NONE;
    uint8_t shift = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = CROSSTALK_EDA_IO_SCANCODE_NONE;

    ret = crosstalk_eda_io_scancode_map(ch, &code, &shift);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (code == (uint8_t)CROSSTALK_EDA_IO_SCANCODE_NONE) { return LDG_ERR_NOT_FOUND; }

    *out = code;

    return LDG_ERR_AOK;
}
