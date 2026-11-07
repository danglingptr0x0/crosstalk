#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>
#include <dangling/term/vt.h>

#include "ui/key.h"

#define KEY_US_UNSHIFTED "1234567890-=[];'`\\,./"
#define KEY_US_SHIFTED "!@#$%^&*()_+{}:\"~|<>?"
#define KEY_LOWER_FIRST 'a'
#define KEY_LOWER_LAST 'z'
#define KEY_UPPER_FIRST 'A'
#define KEY_UPPER_LAST 'Z'
#define KEY_DIGIT_FIRST '0'
#define KEY_DIGIT_LAST '9'
#define KEY_MNEMONIC_USED_CAP 128
#define KEY_MNEMONIC_NONE 0x0U
#define KEY_BASE_NONE 0
#define KEY_TEXT_MAPPED_CUNT 1
#define KEY_TEXT_FIRST 0

static uint32_t key_us_shifted_get(uint32_t ch)
{
    const char *table = KEY_US_UNSHIFTED;
    const char *hit = 0x0;

    if (ch >= (uint32_t)KEY_LOWER_FIRST && ch <= (uint32_t)KEY_LOWER_LAST) { return ch - (uint32_t)LDG_ASCII_CASE_GAP; }

    if (ch == (uint32_t)LDG_ASCII_SPACE || ch > (uint32_t)LDG_ASCII_MAX) { return ch; }

    hit = strchr(table, (char)ch);
    if (hit == 0x0 || *hit == LDG_STR_TERM) { return ch; }

    return (uint32_t)(uint8_t)KEY_US_SHIFTED[hit - table];
}

uint32_t crosstalk_ui_key_us_map(ldg_term_key_event_t *ev)
{
    uint32_t shifted = 0;

    if (LDG_UNLIKELY(ev == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ev->base == (uint32_t)KEY_BASE_NONE || ev->key != (uint8_t)LDG_TERM_KEY_NONE) { return LDG_ERR_AOK; }

    shifted = key_us_shifted_get(ev->base);

    ev->codepoint = ev->base;
    ev->shifted = shifted;

    if (ev->text_cunt != 0)
    {
        ev->text[KEY_TEXT_FIRST] = (ev->mods & (uint32_t)LDG_TERM_MOD_SHIFT) != 0 ? shifted : ev->base;
        ev->text_cunt = (uint32_t)KEY_TEXT_MAPPED_CUNT;
    }

    return LDG_ERR_AOK;
}

static uint8_t key_mnemonic_fold(uint32_t ch)
{
    if (ch >= (uint32_t)KEY_UPPER_FIRST && ch <= (uint32_t)KEY_UPPER_LAST) { return (uint8_t)(ch + (uint32_t)LDG_ASCII_CASE_GAP); }

    if (ch >= (uint32_t)KEY_LOWER_FIRST && ch <= (uint32_t)KEY_LOWER_LAST) { return (uint8_t)ch; }

    if (ch >= (uint32_t)KEY_DIGIT_FIRST && ch <= (uint32_t)KEY_DIGIT_LAST) { return (uint8_t)ch; }

    return (uint8_t)KEY_MNEMONIC_NONE;
}

uint32_t crosstalk_ui_key_mnemonic_assign(const char *const *label, const uint8_t *skip, uint32_t cunt, uint32_t *out_pos, uint8_t *out_ch)
{
    uint8_t used[KEY_MNEMONIC_USED_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t idx = 0;
    uint32_t at = 0;
    uint8_t ch = 0;

    if (LDG_UNLIKELY(label == 0x0 || out_pos == 0x0 || out_ch == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < cunt; idx = idx + 1)
    {
        out_pos[idx] = (uint32_t)UINT32_MAX;
        out_ch[idx] = (uint8_t)KEY_MNEMONIC_NONE;

        if (label[idx] == 0x0 || (skip != 0x0 && skip[idx] == LDG_TRUTH_TRUE)) { continue; }

        for (at = 0; label[idx][at] != LDG_STR_TERM; at = at + 1)
        {
            ch = key_mnemonic_fold((uint32_t)(uint8_t)label[idx][at]);

            if (ch == (uint8_t)KEY_MNEMONIC_NONE || used[ch] == LDG_TRUTH_TRUE) { continue; }

            used[ch] = LDG_TRUTH_TRUE;
            out_pos[idx] = at;
            out_ch[idx] = ch;
            break;
        }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_ui_key_mnemonic_ch_get(const ldg_term_key_event_t *irq, uint8_t *out)
{
    uint32_t cp = 0;

    if (LDG_UNLIKELY(irq == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (uint8_t)KEY_MNEMONIC_NONE;

    if (irq->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE || irq->key != (uint8_t)LDG_TERM_KEY_NONE) { return LDG_ERR_AOK; }

    if ((irq->mods & ((uint32_t)LDG_TERM_MOD_CTRL | (uint32_t)LDG_TERM_MOD_SUPER)) != 0) { return LDG_ERR_AOK; }

    cp = irq->base != 0 ? irq->base : irq->codepoint;
    *out = key_mnemonic_fold(cp);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_ui_key_mnemonic_find(const uint8_t *ch, uint32_t cunt, const ldg_term_key_event_t *irq, uint32_t *out_idx)
{
    uint32_t idx = 0;
    uint32_t ret = 0;
    uint8_t pressed = 0;

    if (LDG_UNLIKELY(ch == 0x0 || irq == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = (uint32_t)UINT32_MAX;

    ret = crosstalk_ui_key_mnemonic_ch_get(irq, &pressed);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (pressed == (uint8_t)KEY_MNEMONIC_NONE) { return LDG_ERR_AOK; }

    for (idx = 0; idx < cunt; idx = idx + 1)
    {
        if (ch[idx] != pressed) { continue; }

        *out_idx = idx;
        break;
    }

    return LDG_ERR_AOK;
}
