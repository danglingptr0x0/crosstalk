#include "game/settings.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define SETTINGS_DOUBLE_CUNT 40
#define SETTINGS_UINT32_CUNT 8
#define SETTINGS_TOGGLE_CUNT 22
#define SETTINGS_KEYBIND_CUNT CROSSTALK_GAME_KEYBIND_CUNT
#define SETTINGS_BLOB_LEN_V7 (SETTINGS_DOUBLE_CUNT * 8 + SETTINGS_UINT32_CUNT * 4 + SETTINGS_TOGGLE_CUNT)
#define SETTINGS_BLOB_LEN (SETTINGS_BLOB_LEN_V7 + SETTINGS_KEYBIND_CUNT * 4)
#define SETTINGS_KEYBIND_CTRL_FIRST 0x01U
#define SETTINGS_KEYBIND_CTRL_LAST 0x1AU
#define SETTINGS_KEYBIND_CTRL_BASE 'a'
#define SETTINGS_KEYBIND_UPPER_FIRST 'A'
#define SETTINGS_KEYBIND_UPPER_LAST 'Z'
#define SETTINGS_KEYBIND_LOWER_FIRST 'a'
#define SETTINGS_KEYBIND_LOWER_LAST 'z'
#define SETTINGS_KEYBIND_CASE_GAP 0x20U
#define SETTINGS_KEYBIND_PRINT_FIRST 0x20U
#define SETTINGS_KEYBIND_PRINT_LAST 0x7EU
#define SETTINGS_KEYBIND_SPACE ' '
#define SETTINGS_KEYBIND_SHIFTED "~!@#$%^&*()_+{}|:\"<>?"
#define SETTINGS_KEYBIND_UNSHIFTED "`1234567890-=[]\\;',./"
#define SETTINGS_KEYBIND_SET_NONE 0x0U
#define SETTINGS_KEYBIND_SET_ARROWS 0x1U
#define SETTINGS_KEYBIND_SET_DIGITS 0x2U
#define SETTINGS_KEYBIND_SET_HJKL 0x4U
#define SETTINGS_KEYBIND_DIGIT_FIRST '0'
#define SETTINGS_KEYBIND_DIGIT_LAST '9'
#define SETTINGS_KEYBIND_LHS_CODE 'h'
#define SETTINGS_KEYBIND_DOWN_CODE 'j'
#define SETTINGS_KEYBIND_UP_CODE 'k'
#define SETTINGS_KEYBIND_RHS_CODE 'l'
#define SETTINGS_KEYBIND_CMDLINE_CODE ';'
#define SETTINGS_KEYBIND_HUB_CODE 'h'
#define SETTINGS_KEYBIND_MUSIC_PREV_CODE ','
#define SETTINGS_KEYBIND_MUSIC_NEXT_CODE '.'
#define SETTINGS_KEYBIND_MUSIC_TOGGLE_CODE '/'
#define SETTINGS_KEYBIND_SHOT_CODE 'p'
#define SETTINGS_KEYBIND_SAVE_CODE 's'
#define SETTINGS_KEYBIND_OPEN_CODE 'o'
#define SETTINGS_KEYBIND_FKEY_FIRST_NUM 1U
#define SETTINGS_KEYBIND_KEY_TEXT_CAP 16
#define SETTINGS_DOUBLE_CUNT_V1 38
#define SETTINGS_TOGGLE_CUNT_V1 11
#define SETTINGS_BLOB_LEN_V1 (SETTINGS_DOUBLE_CUNT_V1 * 8 + SETTINGS_UINT32_CUNT * 4 + SETTINGS_TOGGLE_CUNT_V1)
#define SETTINGS_TOGGLE_CUNT_V2 16
#define SETTINGS_BLOB_LEN_V2 (SETTINGS_DOUBLE_CUNT * 8 + SETTINGS_UINT32_CUNT * 4 + SETTINGS_TOGGLE_CUNT_V2)
#define SETTINGS_TOGGLE_CUNT_V3 18
#define SETTINGS_BLOB_LEN_V3 (SETTINGS_DOUBLE_CUNT * 8 + SETTINGS_UINT32_CUNT * 4 + SETTINGS_TOGGLE_CUNT_V3)
#define SETTINGS_TOGGLE_CUNT_V4 19
#define SETTINGS_BLOB_LEN_V4 (SETTINGS_DOUBLE_CUNT * 8 + SETTINGS_UINT32_CUNT * 4 + SETTINGS_TOGGLE_CUNT_V4)
#define SETTINGS_TOGGLE_CUNT_V5 20
#define SETTINGS_BLOB_LEN_V5 (SETTINGS_DOUBLE_CUNT * 8 + SETTINGS_UINT32_CUNT * 4 + SETTINGS_TOGGLE_CUNT_V5)
#define SETTINGS_TOGGLE_CUNT_V6 21
#define SETTINGS_BLOB_LEN_V6 (SETTINGS_DOUBLE_CUNT * 8 + SETTINGS_UINT32_CUNT * 4 + SETTINGS_TOGGLE_CUNT_V6)
#define SETTINGS_QUANT_SCALE 10000.0
#define LAST_STATE_DOUBLE_CUNT 20
#define LAST_STATE_BLOB_LEN (LAST_STATE_DOUBLE_CUNT * 8)

static void buff_wr(uint8_t *out, uint32_t *offset, const void *src, uint32_t len)
{
    memcpy(out + *offset, src, len);
    *offset = *offset + len;
}

static void buff_rd(const uint8_t *buff, uint32_t *offset, void *dst, uint32_t len)
{
    memcpy(dst, buff + *offset, len);
    *offset = *offset + len;
}

static const uint8_t* settings_double_at_get(const crosstalk_game_settings_t *settings, uint32_t idx)
{
    return (const uint8_t *)settings + offsetof(crosstalk_game_settings_t, tonemap_exposure) + (uint64_t)idx * (uint64_t)sizeof(double);
}

static const uint8_t* settings_uint32_at_get(const crosstalk_game_settings_t *settings, uint32_t idx)
{
    return (const uint8_t *)settings + offsetof(crosstalk_game_settings_t, res_w) + (uint64_t)idx * (uint64_t)sizeof(uint32_t);
}

static const uint8_t* settings_toggle_at_get(const crosstalk_game_settings_t *settings, uint32_t idx)
{
    return (const uint8_t *)settings + offsetof(crosstalk_game_settings_t, bloom_on) + (uint64_t)idx;
}

static uint8_t* settings_double_at_wr_get(crosstalk_game_settings_t *settings, uint32_t idx)
{
    return (uint8_t *)settings + offsetof(crosstalk_game_settings_t, tonemap_exposure) + (uint64_t)idx * (uint64_t)sizeof(double);
}

static uint8_t* settings_uint32_at_wr_get(crosstalk_game_settings_t *settings, uint32_t idx)
{
    return (uint8_t *)settings + offsetof(crosstalk_game_settings_t, res_w) + (uint64_t)idx * (uint64_t)sizeof(uint32_t);
}

static uint8_t* settings_toggle_at_wr_get(crosstalk_game_settings_t *settings, uint32_t idx)
{
    return (uint8_t *)settings + offsetof(crosstalk_game_settings_t, bloom_on) + (uint64_t)idx;
}

static const uint8_t* last_state_double_at_get(const crosstalk_game_last_state_t *state, uint32_t idx)
{
    return (const uint8_t *)state + offsetof(crosstalk_game_last_state_t, cam_hdg) + (uint64_t)idx * (uint64_t)sizeof(double);
}

static uint8_t* last_state_double_at_wr_get(crosstalk_game_last_state_t *state, uint32_t idx)
{
    return (uint8_t *)state + offsetof(crosstalk_game_last_state_t, cam_hdg) + (uint64_t)idx * (uint64_t)sizeof(double);
}

uint8_t crosstalk_game_settings_fab_unlocked_is(const crosstalk_game_settings_t *settings)
{
    if (LDG_UNLIKELY(settings == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return (settings->fab_safety_off != LDG_TRUTH_FALSE && settings->fab_interlock_off != LDG_TRUTH_FALSE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_game_settings_quantize(crosstalk_game_settings_t *settings)
{
    uint8_t *slot = 0x0;
    double val = 0.0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(settings == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < SETTINGS_DOUBLE_CUNT; idx = idx + 1)
    {
        slot = settings_double_at_wr_get(settings, idx);
        memcpy(&val, slot, sizeof(val));
        val = round(val * SETTINGS_QUANT_SCALE) / SETTINGS_QUANT_SCALE;
        memcpy(slot, &val, sizeof(val));
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_settings_serialize(const crosstalk_game_settings_t *settings, uint8_t *out, uint32_t cap, uint32_t *out_len)
{
    uint32_t offset = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(settings == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = SETTINGS_BLOB_LEN;
    if (out == 0x0) { return LDG_ERR_AOK; }

    if (cap < SETTINGS_BLOB_LEN) { return LDG_ERR_BOUNDS; }

    for (idx = 0; idx < SETTINGS_DOUBLE_CUNT; idx = idx + 1) { buff_wr(out, &offset, settings_double_at_get(settings, idx), 8); }

    for (idx = 0; idx < SETTINGS_UINT32_CUNT; idx = idx + 1) { buff_wr(out, &offset, settings_uint32_at_get(settings, idx), 4); }

    for (idx = 0; idx < SETTINGS_TOGGLE_CUNT; idx = idx + 1) { buff_wr(out, &offset, settings_toggle_at_get(settings, idx), 1); }

    for (idx = 0; idx < SETTINGS_KEYBIND_CUNT; idx = idx + 1) { buff_wr(out, &offset, &settings->keybind[idx], 4); }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_settings_deserialize(crosstalk_game_settings_t *settings, const uint8_t *buff, uint32_t len)
{
    uint32_t offset = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(settings == 0x0 || buff == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len != SETTINGS_BLOB_LEN)
    {
        ret = crosstalk_game_settings_keybind_default_fill(settings->keybind, (uint32_t)SETTINGS_KEYBIND_CUNT);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (len == SETTINGS_BLOB_LEN_V1)
    {
        for (idx = 0; idx < SETTINGS_DOUBLE_CUNT_V1; idx = idx + 1) { buff_rd(buff, &offset, settings_double_at_wr_get(settings, idx), 8); }

        for (idx = 0; idx < SETTINGS_UINT32_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_uint32_at_wr_get(settings, idx), 4); }

        for (idx = 0; idx < SETTINGS_TOGGLE_CUNT_V1; idx = idx + 1) { buff_rd(buff, &offset, settings_toggle_at_wr_get(settings, idx), 1); }

        return LDG_ERR_AOK;
    }

    if (len == SETTINGS_BLOB_LEN_V2)
    {
        for (idx = 0; idx < SETTINGS_DOUBLE_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_double_at_wr_get(settings, idx), 8); }

        for (idx = 0; idx < SETTINGS_UINT32_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_uint32_at_wr_get(settings, idx), 4); }

        for (idx = 0; idx < SETTINGS_TOGGLE_CUNT_V2; idx = idx + 1) { buff_rd(buff, &offset, settings_toggle_at_wr_get(settings, idx), 1); }

        return LDG_ERR_AOK;
    }

    if (len == SETTINGS_BLOB_LEN_V3)
    {
        for (idx = 0; idx < SETTINGS_DOUBLE_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_double_at_wr_get(settings, idx), 8); }

        for (idx = 0; idx < SETTINGS_UINT32_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_uint32_at_wr_get(settings, idx), 4); }

        for (idx = 0; idx < SETTINGS_TOGGLE_CUNT_V3; idx = idx + 1) { buff_rd(buff, &offset, settings_toggle_at_wr_get(settings, idx), 1); }

        return LDG_ERR_AOK;
    }

    if (len == SETTINGS_BLOB_LEN_V4)
    {
        for (idx = 0; idx < SETTINGS_DOUBLE_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_double_at_wr_get(settings, idx), 8); }

        for (idx = 0; idx < SETTINGS_UINT32_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_uint32_at_wr_get(settings, idx), 4); }

        for (idx = 0; idx < SETTINGS_TOGGLE_CUNT_V4; idx = idx + 1) { buff_rd(buff, &offset, settings_toggle_at_wr_get(settings, idx), 1); }

        return LDG_ERR_AOK;
    }

    if (len == SETTINGS_BLOB_LEN_V5)
    {
        for (idx = 0; idx < SETTINGS_DOUBLE_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_double_at_wr_get(settings, idx), 8); }

        for (idx = 0; idx < SETTINGS_UINT32_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_uint32_at_wr_get(settings, idx), 4); }

        for (idx = 0; idx < SETTINGS_TOGGLE_CUNT_V5; idx = idx + 1) { buff_rd(buff, &offset, settings_toggle_at_wr_get(settings, idx), 1); }

        return LDG_ERR_AOK;
    }

    if (len == SETTINGS_BLOB_LEN_V6)
    {
        for (idx = 0; idx < SETTINGS_DOUBLE_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_double_at_wr_get(settings, idx), 8); }

        for (idx = 0; idx < SETTINGS_UINT32_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_uint32_at_wr_get(settings, idx), 4); }

        for (idx = 0; idx < SETTINGS_TOGGLE_CUNT_V6; idx = idx + 1) { buff_rd(buff, &offset, settings_toggle_at_wr_get(settings, idx), 1); }

        return LDG_ERR_AOK;
    }

    if (len == SETTINGS_BLOB_LEN_V7)
    {
        for (idx = 0; idx < SETTINGS_DOUBLE_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_double_at_wr_get(settings, idx), 8); }

        for (idx = 0; idx < SETTINGS_UINT32_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_uint32_at_wr_get(settings, idx), 4); }

        for (idx = 0; idx < SETTINGS_TOGGLE_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_toggle_at_wr_get(settings, idx), 1); }

        return LDG_ERR_AOK;
    }

    if (len != SETTINGS_BLOB_LEN) { return LDG_ERR_IO_FMT; }

    for (idx = 0; idx < SETTINGS_DOUBLE_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_double_at_wr_get(settings, idx), 8); }

    for (idx = 0; idx < SETTINGS_UINT32_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_uint32_at_wr_get(settings, idx), 4); }

    for (idx = 0; idx < SETTINGS_TOGGLE_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_toggle_at_wr_get(settings, idx), 1); }

    for (idx = 0; idx < SETTINGS_KEYBIND_CUNT; idx = idx + 1) { buff_rd(buff, &offset, &settings->keybind[idx], 4); }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_last_state_serialize(const crosstalk_game_last_state_t *state, uint8_t *out, uint32_t cap, uint32_t *out_len)
{
    uint32_t offset = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(state == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = LAST_STATE_BLOB_LEN;
    if (out == 0x0) { return LDG_ERR_AOK; }

    if (cap < LAST_STATE_BLOB_LEN) { return LDG_ERR_BOUNDS; }

    for (idx = 0; idx < LAST_STATE_DOUBLE_CUNT; idx = idx + 1) { buff_wr(out, &offset, last_state_double_at_get(state, idx), 8); }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_last_state_deserialize(crosstalk_game_last_state_t *state, const uint8_t *buff, uint32_t len)
{
    uint32_t offset = 0;
    uint32_t idx = 0;
    uint32_t avail = 0;

    if (LDG_UNLIKELY(state == 0x0 || buff == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    avail = len / 8;
    if (avail > LAST_STATE_DOUBLE_CUNT) { avail = LAST_STATE_DOUBLE_CUNT; }

    for (idx = 0; idx < avail; idx = idx + 1) { buff_rd(buff, &offset, last_state_double_at_wr_get(state, idx), 8); }

    return LDG_ERR_AOK;
}

static uint32_t settings_keybind_set_get(uint32_t action)
{
    if (action == (uint32_t)CROSSTALK_GAME_KEYBIND_WIN_FOCUS) { return (uint32_t)SETTINGS_KEYBIND_SET_ARROWS; }

    if (action == (uint32_t)CROSSTALK_GAME_KEYBIND_WIN_DIGIT) { return (uint32_t)SETTINGS_KEYBIND_SET_DIGITS; }

    if (action == (uint32_t)CROSSTALK_GAME_KEYBIND_WIN_RESIZE || action == (uint32_t)CROSSTALK_GAME_KEYBIND_WIN_MOV) { return (uint32_t)SETTINGS_KEYBIND_SET_ARROWS | (uint32_t)SETTINGS_KEYBIND_SET_HJKL; }

    return (uint32_t)SETTINGS_KEYBIND_SET_NONE;
}

static uint8_t settings_keybind_in_set_is(uint32_t set, uint32_t chord)
{
    uint32_t code = chord & (uint32_t)CROSSTALK_GAME_KEYBIND_CODE_MASK;

    if ((chord & (uint32_t)CROSSTALK_GAME_KEYBIND_SPECIAL) != 0)
    {
        if ((set & (uint32_t)SETTINGS_KEYBIND_SET_ARROWS) == 0) { return LDG_TRUTH_FALSE; }

        if (code == (uint32_t)LDG_TERM_KEY_UP || code == (uint32_t)LDG_TERM_KEY_DOWN || code == (uint32_t)LDG_TERM_KEY_LHS || code == (uint32_t)LDG_TERM_KEY_RHS) { return LDG_TRUTH_TRUE; }

        return LDG_TRUTH_FALSE;
    }

    if ((set & (uint32_t)SETTINGS_KEYBIND_SET_DIGITS) != 0 && code >= (uint32_t)SETTINGS_KEYBIND_DIGIT_FIRST && code <= (uint32_t)SETTINGS_KEYBIND_DIGIT_LAST) { return LDG_TRUTH_TRUE; }

    if ((set & (uint32_t)SETTINGS_KEYBIND_SET_HJKL) != 0 && (code == (uint32_t)SETTINGS_KEYBIND_LHS_CODE || code == (uint32_t)SETTINGS_KEYBIND_DOWN_CODE || code == (uint32_t)SETTINGS_KEYBIND_UP_CODE || code == (uint32_t)SETTINGS_KEYBIND_RHS_CODE)) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t settings_keybind_collide_is(uint32_t a_action, uint32_t a_chord, uint32_t b_action, uint32_t b_chord)
{
    uint32_t a_set = settings_keybind_set_get(a_action);
    uint32_t b_set = settings_keybind_set_get(b_action);
    uint8_t mods_eq = (a_chord & (uint32_t)CROSSTALK_GAME_KEYBIND_MOD_MASK) == (b_chord & (uint32_t)CROSSTALK_GAME_KEYBIND_MOD_MASK) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (a_set == (uint32_t)SETTINGS_KEYBIND_SET_NONE && b_set == (uint32_t)SETTINGS_KEYBIND_SET_NONE) { return a_chord == b_chord ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    if (mods_eq != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    if (a_set != (uint32_t)SETTINGS_KEYBIND_SET_NONE && b_set != (uint32_t)SETTINGS_KEYBIND_SET_NONE) { return (a_set & b_set) != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    if (a_set != (uint32_t)SETTINGS_KEYBIND_SET_NONE) { return settings_keybind_in_set_is(a_set, b_chord); }

    return settings_keybind_in_set_is(b_set, a_chord);
}

static const char* settings_keybind_special_name_get(uint32_t key)
{
    if (key == (uint32_t)LDG_TERM_KEY_UP) { return "Up"; }

    if (key == (uint32_t)LDG_TERM_KEY_DOWN) { return "Down"; }

    if (key == (uint32_t)LDG_TERM_KEY_LHS) { return "Left"; }

    if (key == (uint32_t)LDG_TERM_KEY_RHS) { return "Right"; }

    if (key == (uint32_t)LDG_TERM_KEY_HOME) { return "Home"; }

    if (key == (uint32_t)LDG_TERM_KEY_END) { return "End"; }

    if (key == (uint32_t)LDG_TERM_KEY_INSERT) { return "Insert"; }

    if (key == (uint32_t)LDG_TERM_KEY_DELETE) { return "Delete"; }

    if (key == (uint32_t)LDG_TERM_KEY_PAGE_UP) { return "PgUp"; }

    if (key == (uint32_t)LDG_TERM_KEY_PAGE_DOWN) { return "PgDn"; }

    if (key == (uint32_t)LDG_TERM_KEY_BACKSPACE) { return "Backspace"; }

    if (key == (uint32_t)LDG_TERM_KEY_TAB) { return "Tab"; }

    if (key == (uint32_t)LDG_TERM_KEY_ENTER) { return "Return"; }

    if (key == (uint32_t)LDG_TERM_KEY_ESCAPE) { return "Esc"; }

    if (key == (uint32_t)LDG_TERM_KEY_BREAK) { return "Break"; }

    if (key == (uint32_t)LDG_TERM_KEY_PRINT) { return "Print"; }

    return 0x0;
}

uint32_t crosstalk_game_settings_keybind_default_fill(uint32_t *binds, uint32_t cunt)
{
    if (LDG_UNLIKELY(binds == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt < (uint32_t)CROSSTALK_GAME_KEYBIND_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    binds[CROSSTALK_GAME_KEYBIND_CMDLINE] = (uint32_t)CROSSTALK_GAME_KEYBIND_CTRL | (uint32_t)CROSSTALK_GAME_KEYBIND_SHIFT | (uint32_t)SETTINGS_KEYBIND_CMDLINE_CODE;
    binds[CROSSTALK_GAME_KEYBIND_HUB] = (uint32_t)CROSSTALK_GAME_KEYBIND_CTRL | (uint32_t)CROSSTALK_GAME_KEYBIND_SHIFT | (uint32_t)SETTINGS_KEYBIND_HUB_CODE;
    binds[CROSSTALK_GAME_KEYBIND_MUSIC_PREV] = (uint32_t)CROSSTALK_GAME_KEYBIND_CTRL | (uint32_t)CROSSTALK_GAME_KEYBIND_SHIFT | (uint32_t)SETTINGS_KEYBIND_MUSIC_PREV_CODE;
    binds[CROSSTALK_GAME_KEYBIND_MUSIC_NEXT] = (uint32_t)CROSSTALK_GAME_KEYBIND_CTRL | (uint32_t)CROSSTALK_GAME_KEYBIND_SHIFT | (uint32_t)SETTINGS_KEYBIND_MUSIC_NEXT_CODE;
    binds[CROSSTALK_GAME_KEYBIND_MUSIC_TOGGLE] = (uint32_t)CROSSTALK_GAME_KEYBIND_CTRL | (uint32_t)CROSSTALK_GAME_KEYBIND_SHIFT | (uint32_t)SETTINGS_KEYBIND_MUSIC_TOGGLE_CODE;
    binds[CROSSTALK_GAME_KEYBIND_WIN_RECENT] = (uint32_t)CROSSTALK_GAME_KEYBIND_CTRL | (uint32_t)CROSSTALK_GAME_KEYBIND_SPECIAL | (uint32_t)LDG_TERM_KEY_TAB;
    binds[CROSSTALK_GAME_KEYBIND_WIN_CYCLE] = (uint32_t)CROSSTALK_GAME_KEYBIND_CTRL | (uint32_t)CROSSTALK_GAME_KEYBIND_SHIFT | (uint32_t)CROSSTALK_GAME_KEYBIND_SPECIAL | (uint32_t)LDG_TERM_KEY_TAB;
    binds[CROSSTALK_GAME_KEYBIND_WIN_FOCUS] = (uint32_t)CROSSTALK_GAME_KEYBIND_CTRL;
    binds[CROSSTALK_GAME_KEYBIND_WIN_DIGIT] = (uint32_t)CROSSTALK_GAME_KEYBIND_CTRL;
    binds[CROSSTALK_GAME_KEYBIND_WIN_RESIZE] = (uint32_t)CROSSTALK_GAME_KEYBIND_ALT;
    binds[CROSSTALK_GAME_KEYBIND_WIN_MOV] = (uint32_t)CROSSTALK_GAME_KEYBIND_ALT | (uint32_t)CROSSTALK_GAME_KEYBIND_SHIFT;
    binds[CROSSTALK_GAME_KEYBIND_PAUSE] = (uint32_t)CROSSTALK_GAME_KEYBIND_SHIFT | (uint32_t)CROSSTALK_GAME_KEYBIND_SPECIAL | (uint32_t)LDG_TERM_KEY_ESCAPE;
    binds[CROSSTALK_GAME_KEYBIND_SHOT] = (uint32_t)CROSSTALK_GAME_KEYBIND_ALT | (uint32_t)SETTINGS_KEYBIND_SHOT_CODE;
    binds[CROSSTALK_GAME_KEYBIND_SAVE] = (uint32_t)CROSSTALK_GAME_KEYBIND_CTRL | (uint32_t)SETTINGS_KEYBIND_SAVE_CODE;
    binds[CROSSTALK_GAME_KEYBIND_OPEN] = (uint32_t)CROSSTALK_GAME_KEYBIND_CTRL | (uint32_t)SETTINGS_KEYBIND_OPEN_CODE;
    binds[CROSSTALK_GAME_KEYBIND_INVITE] = (uint32_t)CROSSTALK_GAME_KEYBIND_SPECIAL | (uint32_t)LDG_TERM_KEY_F4;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_settings_keybind_chord_get(const ldg_term_key_event_t *irq, uint32_t *out)
{
    const char *shifted = SETTINGS_KEYBIND_SHIFTED;
    const char *unshifted = SETTINGS_KEYBIND_UNSHIFTED;
    uint32_t mods = 0;
    uint32_t cp = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(irq == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (uint32_t)CROSSTALK_GAME_KEYBIND_UNBOUND;

    if (irq->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE) { return LDG_ERR_AOK; }

    if ((irq->mods & (uint32_t)LDG_TERM_MOD_CTRL) != 0) { mods = mods | (uint32_t)CROSSTALK_GAME_KEYBIND_CTRL; }

    if ((irq->mods & (uint32_t)LDG_TERM_MOD_SHIFT) != 0) { mods = mods | (uint32_t)CROSSTALK_GAME_KEYBIND_SHIFT; }

    if ((irq->mods & (uint32_t)LDG_TERM_MOD_ALT) != 0) { mods = mods | (uint32_t)CROSSTALK_GAME_KEYBIND_ALT; }

    if (irq->key != (uint8_t)LDG_TERM_KEY_NONE)
    {
        if (irq->key >= (uint8_t)LDG_TERM_KEY_CUNT) { return LDG_ERR_AOK; }

        *out = mods | (uint32_t)CROSSTALK_GAME_KEYBIND_SPECIAL | (uint32_t)irq->key;

        return LDG_ERR_AOK;
    }

    cp = irq->base != 0 ? irq->base : irq->codepoint;

    if (cp >= (uint32_t)SETTINGS_KEYBIND_CTRL_FIRST && cp <= (uint32_t)SETTINGS_KEYBIND_CTRL_LAST)
    {
        cp = (uint32_t)SETTINGS_KEYBIND_CTRL_BASE + cp - (uint32_t)SETTINGS_KEYBIND_CTRL_FIRST;
        mods = mods | (uint32_t)CROSSTALK_GAME_KEYBIND_CTRL;
    }

    if (cp >= (uint32_t)SETTINGS_KEYBIND_UPPER_FIRST && cp <= (uint32_t)SETTINGS_KEYBIND_UPPER_LAST)
    {
        cp = cp + (uint32_t)SETTINGS_KEYBIND_CASE_GAP;
        mods = mods | (uint32_t)CROSSTALK_GAME_KEYBIND_SHIFT;
    }

    for (at = 0; shifted[at] != LDG_STR_TERM; at = at + 1)
    {
        if (cp != (uint32_t)(uint8_t)shifted[at]) { continue; }

        cp = (uint32_t)(uint8_t)unshifted[at];
        mods = mods | (uint32_t)CROSSTALK_GAME_KEYBIND_SHIFT;
        break;
    }

    if (cp < (uint32_t)SETTINGS_KEYBIND_PRINT_FIRST || cp > (uint32_t)SETTINGS_KEYBIND_PRINT_LAST) { return LDG_ERR_AOK; }

    *out = mods | cp;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_game_settings_keybind_family_is(uint32_t action)
{
    return settings_keybind_set_get(action) != (uint32_t)SETTINGS_KEYBIND_SET_NONE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint8_t crosstalk_game_settings_keybind_match_is(const uint32_t *binds, uint32_t action, const ldg_term_key_event_t *irq)
{
    uint32_t chord = 0;
    uint32_t bind = 0;
    uint32_t set = 0;

    if (LDG_UNLIKELY(binds == 0x0 || irq == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(action >= (uint32_t)CROSSTALK_GAME_KEYBIND_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    bind = binds[action];
    if (bind == (uint32_t)CROSSTALK_GAME_KEYBIND_UNBOUND) { return LDG_TRUTH_FALSE; }

    if (crosstalk_game_settings_keybind_chord_get(irq, &chord) != LDG_ERR_AOK || chord == (uint32_t)CROSSTALK_GAME_KEYBIND_UNBOUND) { return LDG_TRUTH_FALSE; }

    set = settings_keybind_set_get(action);

    if (set == (uint32_t)SETTINGS_KEYBIND_SET_NONE) { return chord == bind ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    if ((chord & (uint32_t)CROSSTALK_GAME_KEYBIND_MOD_MASK) != (bind & (uint32_t)CROSSTALK_GAME_KEYBIND_MOD_MASK)) { return LDG_TRUTH_FALSE; }

    return settings_keybind_in_set_is(set, chord);
}

uint32_t crosstalk_game_settings_keybind_capture_get(uint32_t action, const ldg_term_key_event_t *irq, uint32_t *out)
{
    uint32_t chord = 0;
    uint32_t set = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(irq == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (uint32_t)CROSSTALK_GAME_KEYBIND_UNBOUND;

    if (LDG_UNLIKELY(action >= (uint32_t)CROSSTALK_GAME_KEYBIND_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = crosstalk_game_settings_keybind_chord_get(irq, &chord);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (chord == (uint32_t)CROSSTALK_GAME_KEYBIND_UNBOUND) { return LDG_ERR_AOK; }

    set = settings_keybind_set_get(action);

    if (set == (uint32_t)SETTINGS_KEYBIND_SET_NONE)
    {
        *out = chord;
        return LDG_ERR_AOK;
    }

    if (settings_keybind_in_set_is(set, chord) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if ((chord & (uint32_t)CROSSTALK_GAME_KEYBIND_MOD_MASK) == 0) { return LDG_ERR_INVALID; }

    *out = chord & (uint32_t)CROSSTALK_GAME_KEYBIND_MOD_MASK;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_settings_keybind_conflict_get(const uint32_t *binds, uint32_t action, uint32_t chord, uint32_t *out_owner)
{
    uint32_t other = 0;

    if (LDG_UNLIKELY(binds == 0x0 || out_owner == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_owner = (uint32_t)UINT32_MAX;

    if (LDG_UNLIKELY(action >= (uint32_t)CROSSTALK_GAME_KEYBIND_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (chord == (uint32_t)CROSSTALK_GAME_KEYBIND_UNBOUND) { return LDG_ERR_AOK; }

    for (other = 0; other < (uint32_t)CROSSTALK_GAME_KEYBIND_CUNT; other = other + 1)
    {
        if (other == action || binds[other] == (uint32_t)CROSSTALK_GAME_KEYBIND_UNBOUND) { continue; }

        if (settings_keybind_collide_is(action, chord, other, binds[other]) != LDG_TRUTH_TRUE) { continue; }

        *out_owner = other;
        break;
    }

    return LDG_ERR_AOK;
}

static uint32_t settings_keybind_fkey_fmt(uint32_t code, char *out, uint64_t cap)
{
    if (code < (uint32_t)LDG_TERM_KEY_F1 || code > (uint32_t)LDG_TERM_KEY_F12) { return LDG_ERR_NOT_FOUND; }

    return snprintf(out, cap, "F%u", code - (uint32_t)LDG_TERM_KEY_F1 + (uint32_t)SETTINGS_KEYBIND_FKEY_FIRST_NUM) < 0 ? LDG_ERR_STR_FMT : LDG_ERR_AOK;
}

static const char* settings_keybind_vim_name_get(uint32_t code)
{
    if (code == (uint32_t)LDG_TERM_KEY_INSERT) { return "Insert"; }

    if (code == (uint32_t)LDG_TERM_KEY_DELETE) { return "Del"; }

    if (code == (uint32_t)LDG_TERM_KEY_PAGE_UP) { return "PageUp"; }

    if (code == (uint32_t)LDG_TERM_KEY_PAGE_DOWN) { return "PageDown"; }

    if (code == (uint32_t)LDG_TERM_KEY_BACKSPACE) { return "BS"; }

    if (code == (uint32_t)LDG_TERM_KEY_ENTER) { return "CR"; }

    return settings_keybind_special_name_get(code);
}

static const char* settings_keybind_unix_byte_get(uint32_t code)
{
    if (code == (uint32_t)LDG_TERM_KEY_ESCAPE) { return "^["; }

    if (code == (uint32_t)LDG_TERM_KEY_ENTER) { return "^M"; }

    if (code == (uint32_t)LDG_TERM_KEY_TAB) { return "^I"; }

    if (code == (uint32_t)LDG_TERM_KEY_BACKSPACE) { return "^?"; }

    return 0x0;
}

static const char* settings_keybind_unix_name_get(uint32_t code)
{
    if (code == (uint32_t)LDG_TERM_KEY_INSERT) { return "Ins"; }

    if (code == (uint32_t)LDG_TERM_KEY_DELETE) { return "Del"; }

    return settings_keybind_special_name_get(code);
}

static uint32_t settings_keybind_shifted_get(uint32_t code)
{
    const char *shifted = SETTINGS_KEYBIND_SHIFTED;
    const char *unshifted = SETTINGS_KEYBIND_UNSHIFTED;
    uint32_t at = 0;

    if (code >= (uint32_t)SETTINGS_KEYBIND_LOWER_FIRST && code <= (uint32_t)SETTINGS_KEYBIND_LOWER_LAST) { return code - (uint32_t)SETTINGS_KEYBIND_CASE_GAP; }

    for (at = 0; unshifted[at] != LDG_STR_TERM; at = at + 1) { if (code == (uint32_t)(uint8_t)unshifted[at]) { return (uint32_t)(uint8_t)shifted[at]; } }

    return code;
}

static uint32_t settings_keybind_upper_get(uint32_t code)
{
    if (code >= (uint32_t)SETTINGS_KEYBIND_LOWER_FIRST && code <= (uint32_t)SETTINGS_KEYBIND_LOWER_LAST) { return code - (uint32_t)SETTINGS_KEYBIND_CASE_GAP; }

    return code;
}

uint32_t crosstalk_game_settings_keybind_vim_fmt(uint32_t action, uint32_t chord, char *out, uint32_t cap)
{
    char key[SETTINGS_KEYBIND_KEY_TEXT_CAP] = LDG_ARR_ZERO_INIT;
    const char *ctrl = (chord & (uint32_t)CROSSTALK_GAME_KEYBIND_CTRL) != 0 ? "C-" : "";
    const char *shift = (chord & (uint32_t)CROSSTALK_GAME_KEYBIND_SHIFT) != 0 ? "S-" : "";
    const char *alt = (chord & (uint32_t)CROSSTALK_GAME_KEYBIND_ALT) != 0 ? "A-" : "";
    const char *name = 0x0;
    uint32_t code = chord & (uint32_t)CROSSTALK_GAME_KEYBIND_CODE_MASK;
    uint32_t set = 0;

    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (chord == (uint32_t)CROSSTALK_GAME_KEYBIND_UNBOUND) { return snprintf(out, cap, "%.15s", "unbound") < 0 ? LDG_ERR_STR_FMT : LDG_ERR_AOK; }

    set = settings_keybind_set_get(action);

    if (set == (uint32_t)SETTINGS_KEYBIND_SET_ARROWS) { name = "arrows"; }
    else if (set == (uint32_t)SETTINGS_KEYBIND_SET_DIGITS) { name = "0..9"; }
    else if (set != (uint32_t)SETTINGS_KEYBIND_SET_NONE) { name = "hjkl/arrows"; }
    else if ((chord & (uint32_t)CROSSTALK_GAME_KEYBIND_SPECIAL) != 0)
    {
        name = settings_keybind_vim_name_get(code);

        if (name == 0x0 && settings_keybind_fkey_fmt(code, key, (uint64_t)sizeof(key)) == LDG_ERR_AOK) { name = key; }

        if (name == 0x0) { name = "?"; }
    }
    else if (code == (uint32_t)SETTINGS_KEYBIND_SPACE) { name = "Space"; }
    else
    {
        key[0] = (char)code;

        if ((chord & (uint32_t)CROSSTALK_GAME_KEYBIND_MOD_MASK) == 0) { return snprintf(out, cap, "%.15s", key) < 0 ? LDG_ERR_STR_FMT : LDG_ERR_AOK; }

        name = key;
    }

    return snprintf(out, cap, "<%.2s%.2s%.2s%.15s>", ctrl, shift, alt, name) < 0 ? LDG_ERR_STR_FMT : LDG_ERR_AOK;
}

uint32_t crosstalk_game_settings_keybind_unix_fmt(uint32_t action, uint32_t chord, char *out, uint32_t cap)
{
    char key[SETTINGS_KEYBIND_KEY_TEXT_CAP] = LDG_ARR_ZERO_INIT;
    const char *alt = (chord & (uint32_t)CROSSTALK_GAME_KEYBIND_ALT) != 0 ? "M-" : "";
    const char *hat = (chord & (uint32_t)CROSSTALK_GAME_KEYBIND_CTRL) != 0 ? "^" : "";
    const char *shift = (chord & (uint32_t)CROSSTALK_GAME_KEYBIND_SHIFT) != 0 ? "Shift" : "";
    const char *name = 0x0;
    uint8_t ctrl_on = (chord & (uint32_t)CROSSTALK_GAME_KEYBIND_CTRL) != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    uint8_t shift_on = (chord & (uint32_t)CROSSTALK_GAME_KEYBIND_SHIFT) != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    uint32_t code = chord & (uint32_t)CROSSTALK_GAME_KEYBIND_CODE_MASK;
    uint32_t set = 0;

    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (chord == (uint32_t)CROSSTALK_GAME_KEYBIND_UNBOUND) { return snprintf(out, cap, "%.15s", "unbound") < 0 ? LDG_ERR_STR_FMT : LDG_ERR_AOK; }

    set = settings_keybind_set_get(action);

    if (set == (uint32_t)SETTINGS_KEYBIND_SET_ARROWS) { name = "arrows"; }
    else if (set == (uint32_t)SETTINGS_KEYBIND_SET_DIGITS) { name = "0..9"; }
    else if (set != (uint32_t)SETTINGS_KEYBIND_SET_NONE)
    {
        if (ctrl_on != LDG_TRUTH_TRUE && shift_on == LDG_TRUTH_TRUE) { return snprintf(out, cap, "%.2s%.23s", alt, "HJKL/Shiftarrows") < 0 ? LDG_ERR_STR_FMT : LDG_ERR_AOK; }

        name = "hjkl/arrows";
    }
    else if ((chord & (uint32_t)CROSSTALK_GAME_KEYBIND_SPECIAL) != 0)
    {
        if (ctrl_on != LDG_TRUTH_TRUE) { name = settings_keybind_unix_byte_get(code); }

        if (name == 0x0) { name = settings_keybind_unix_name_get(code); }

        if (name == 0x0 && settings_keybind_fkey_fmt(code, key, (uint64_t)sizeof(key)) == LDG_ERR_AOK) { name = key; }

        if (name == 0x0) { name = "?"; }
    }
    else if (code == (uint32_t)SETTINGS_KEYBIND_SPACE) { name = "Space"; }
    else if (ctrl_on == LDG_TRUTH_TRUE)
    {
        key[0] = (char)settings_keybind_upper_get(code);
        name = key;
    }
    else
    {
        key[0] = (char)(shift_on == LDG_TRUTH_TRUE ? settings_keybind_shifted_get(code) : code);

        return snprintf(out, cap, "%.2s%.15s", alt, key) < 0 ? LDG_ERR_STR_FMT : LDG_ERR_AOK;
    }

    return snprintf(out, cap, "%.2s%.1s%.5s%.15s", alt, hat, shift, name) < 0 ? LDG_ERR_STR_FMT : LDG_ERR_AOK;
}
