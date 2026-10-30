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
