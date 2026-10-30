#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "game/menu.h"
#include "ui/cell.h"
#include "ui/theme.h"

#define MENU_STEP 1
#define MENU_FIELD_FIRST 0
#define MENU_ROW_FIRST 0
#define MENU_LINE_CAP 256
#define MENU_SEG_CAP 128
#define MENU_SEG_SEP ';'
#define MENU_LIST_ROWS 12
#define MENU_ROW_GAP 1
#define MENU_LD_BAR_COLS 60
#define MENU_LD_PCT_SCALE 100.0
#define MENU_LD_FRAC_MIN 0.0
#define MENU_LD_FRAC_MAX 1.0
#define MENU_LD_PCT_FMT "%u%%"
#define MENU_BTN_FMT "[%.47s]"
#define MENU_CHOICE_FMT "%-20.20s < %.47s >"
#define MENU_TEXT_FMT "%-20.20s %.127s"
#define MENU_TOGGLE_ON_FMT "[x] %.63s"
#define MENU_TOGGLE_OFF_FMT "[ ] %.63s"
#define MENU_LABEL_W 21
#define MENU_CURSOR_W 1
#define MENU_COL_LOCKED 0x6E6E6EFFU
#define MENU_DIFFICULTY_EASY 0
#define MENU_DIFFICULTY_HARD 2
#define MENU_SIZE_ITEMS "50x50;100x100;100x160;160x100"
#define MENU_DIFFICULTY_ITEMS "Easy;Medium;Hard"
#define MENU_LAYERS_ITEMS_MEDIUM "2;4;6;8;10;12"
#define MENU_LAYERS_ITEMS_HARD "2;4;6;8;10;12;14;16;18;20;22;24"
#define MENU_NEW_NAME_LABEL "Board name"
#define MENU_NEW_PICK_LABEL "Existing project"
#define MENU_EDITORPICK_TITLE_TEXT "OPEN AS"
#define MENU_EDITORPICK_LABEL "This save has more than one editor"
#define MENU_EDITORPICK_OPEN_LABEL "Open"
#define MENU_MP_TITLE_TEXT "MULTIPLAYER"
#define MENU_MP_EMPTY_TEXT "No sessions found"
#define MENU_MP_EMPTY_LAN_TEXT "No LAN sessions found"
#define MENU_MP_HOST_LABEL "Host a session"
#define MENU_MP_BOARD_LABEL "Board"
#define MENU_MP_PLAYERS_LABEL "Max players"
#define MENU_MP_PLAYERS_ITEMS "2;4;6;8;10"
#define MENU_MP_TRANSPORT_LABEL "Network"
#define MENU_MP_TRANSPORT_ITEMS "online;LAN"
#define MENU_MP_MODE_LABEL "Mode"
#define MENU_MP_MODE_ITEMS "party;adversarial"
#define MENU_MP_REFRESH_TEXT "Refresh"
#define MENU_MP_JOIN_TEXT "Join"
#define MENU_MP_HOST_TEXT "Host"
#define MENU_MP_NO_BOARD_TEXT "No boards to host"
#define MENU_MP_TRANSPORT_LAN 1
#define MENU_ROOM_TITLE_TEXT "LOBBY"
#define MENU_ROOM_RDY_TEXT "Ready"
#define MENU_ROOM_START_TEXT "Start"
#define MENU_ROOM_LEAVE_TEXT "Leave"
#define MENU_ROOM_BUILD_TEXT "Keep building"
#define MENU_SEL_EMPTY_TEXT "No boards found"
#define MENU_SEL_TRUNC_TEXT "List truncated; some boards not shown"
#define MENU_SEL_WSHOP_TEXT "Show Workshop items"
#define MENU_SEL_TITLE_TEXT "SELECT BOARD"
#define MENU_SEL_OPEN_TEXT "Open"
#define MENU_SEL_BACK_TEXT "Back"
#define MENU_EXTRAS_TITLE_TEXT "EXTRAS"
#define MENU_EXTRAS_EMPTY_TEXT "Nothing here yet"
#define MENU_EXTRAS_OPEN_TEXT "Open"
#define MENU_CREDITS_TITLE_TEXT "CREDITS"
#define MENU_CREDITS_EMPTY_TEXT "No credits file found"
#define MENU_HUB_TEXT "Friends Hub"
#define MENU_NEW_TITLE_TEXT "NEW DESIGN"
#define MENU_NEWPROJECT_TITLE_TEXT "NEW PROJECT"
#define MENU_SIZE_LABEL "Board size (mm)"
#define MENU_DIFFICULTY_LABEL "Difficulty"
#define MENU_LAYERS_LABEL "Copper layers"
#define MENU_CREATE_TEXT "Create"

#define MENU_MAIN_FIELD_CUNT 5

#define MENU_ND_NAME 0
#define MENU_ND_SIZE 1
#define MENU_ND_DIFF 2
#define MENU_ND_LAYERS 3
#define MENU_ND_CREATE 4
#define MENU_ND_BACK 5
#define MENU_ND_CUNT 6

#define MENU_EP_PICK 0
#define MENU_EP_OPEN 1
#define MENU_EP_BACK 2
#define MENU_EP_CUNT 3

#define MENU_NP_PICK 0
#define MENU_NP_NAME 1
#define MENU_NP_CREATE 2
#define MENU_NP_BACK 3
#define MENU_NP_CUNT 4

#define MENU_MP_LIST 0
#define MENU_MP_REFRESH 1
#define MENU_MP_JOIN 2
#define MENU_MP_TRANSPORT 3
#define MENU_MP_BOARD 4
#define MENU_MP_PLAYERS 5
#define MENU_MP_MODE 6
#define MENU_MP_HOST 7
#define MENU_MP_BACK 8
#define MENU_MP_CUNT 9

#define MENU_RM_LIST 0
#define MENU_RM_RDY 1
#define MENU_RM_START 2
#define MENU_RM_LEAVE 3
#define MENU_RM_BUILD 4
#define MENU_RM_CUNT 5

#define MENU_EX_LIST 0
#define MENU_EX_OPEN 1
#define MENU_EX_BACK 2
#define MENU_EX_CUNT 3

#define MENU_CR_LIST 0
#define MENU_CR_BACK 1
#define MENU_CR_CUNT 2

#define MENU_BS_LIST 0
#define MENU_BS_WSHOP 1
#define MENU_BS_OPEN 2
#define MENU_BS_BACK 3
#define MENU_BS_CUNT 4

static uint32_t menu_seg_cunt(const char *items)
{
    uint32_t cunt = MENU_STEP;

    if (items == 0x0 || items[0] == LDG_STR_TERM) { return MENU_ROW_FIRST; }

    for (const char *at = items; *at != LDG_STR_TERM; at++) { if (*at == MENU_SEG_SEP) { cunt++; } }

    return cunt;
}

static void menu_seg_get(const char *items, uint32_t idx, char *out, uint32_t cap)
{
    uint32_t seg = 0;
    uint32_t len = 0;

    out[0] = LDG_STR_TERM;

    if (items == 0x0) { return; }

    for (const char *at = items; *at != LDG_STR_TERM; at++)
    {
        if (*at == MENU_SEG_SEP) { seg++; continue; }

        if (seg != idx) { continue; }

        if (len + LDG_STR_TERM_SIZE >= cap) { break; }

        out[len] = *at;
        len++;
    }

    out[len] = LDG_STR_TERM;
}

static uint8_t menu_key_skip_is(const ldg_term_key_event_t *key)
{
    if (key->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE) { return LDG_TRUTH_TRUE; }

    if ((key->mods & ((uint32_t)LDG_TERM_MOD_CTRL | (uint32_t)LDG_TERM_MOD_ALT | (uint32_t)LDG_TERM_MOD_SUPER)) != 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static void menu_field_mov(crosstalk_game_menu_nav_t *nav, uint32_t cunt, uint32_t mask, uint8_t back)
{
    uint32_t at = nav->field;

    for (uint32_t step = 0; step < cunt; step++)
    {
        if (back == LDG_TRUTH_TRUE) { at = at == 0 ? cunt - MENU_STEP : at - MENU_STEP; }
        else { at = (at + MENU_STEP) % cunt; }

        if ((mask & (uint32_t)LDG_BIT(at)) != 0) { nav->field = at; return; }
    }
}

static void menu_field_fix(crosstalk_game_menu_nav_t *nav, uint32_t cunt, uint32_t mask)
{
    if (nav->field < cunt && (mask & (uint32_t)LDG_BIT(nav->field)) != 0) { return; }

    nav->field = cunt - MENU_STEP;
    menu_field_mov(nav, cunt, mask, LDG_TRUTH_FALSE);
}

static uint8_t menu_tab_key(const ldg_term_key_event_t *key, crosstalk_game_menu_nav_t *nav, uint32_t cunt, uint32_t mask)
{
    if (key->key != (uint8_t)LDG_TERM_KEY_TAB) { return LDG_TRUTH_FALSE; }

    menu_field_mov(nav, cunt, mask, (key->mods & (uint32_t)LDG_TERM_MOD_SHIFT) != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE);

    return LDG_TRUTH_TRUE;
}

static uint8_t menu_vert_key(const ldg_term_key_event_t *key, crosstalk_game_menu_nav_t *nav, uint32_t cunt, uint32_t mask)
{
    if (key->key == (uint8_t)LDG_TERM_KEY_UP) { menu_field_mov(nav, cunt, mask, LDG_TRUTH_TRUE); return LDG_TRUTH_TRUE; }

    if (key->key == (uint8_t)LDG_TERM_KEY_DOWN) { menu_field_mov(nav, cunt, mask, LDG_TRUTH_FALSE); return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t menu_list_key(const ldg_term_key_event_t *key, crosstalk_game_menu_nav_t *nav, uint32_t cunt)
{
    if (cunt == 0) { return LDG_TRUTH_FALSE; }

    if (nav->row >= cunt) { nav->row = cunt - MENU_STEP; }

    if (key->key == (uint8_t)LDG_TERM_KEY_UP)
    {
        if (nav->row != 0) { nav->row = nav->row - MENU_STEP; }

        return LDG_TRUTH_TRUE;
    }

    if (key->key == (uint8_t)LDG_TERM_KEY_DOWN)
    {
        if (nav->row + MENU_STEP < cunt) { nav->row = nav->row + MENU_STEP; }

        return LDG_TRUTH_TRUE;
    }

    if (key->key == (uint8_t)LDG_TERM_KEY_PAGE_UP) { nav->row = nav->row > (uint32_t)MENU_LIST_ROWS ? nav->row - (uint32_t)MENU_LIST_ROWS : (uint32_t)MENU_ROW_FIRST; return LDG_TRUTH_TRUE; }

    if (key->key == (uint8_t)LDG_TERM_KEY_PAGE_DOWN) { nav->row = nav->row + (uint32_t)MENU_LIST_ROWS < cunt ? nav->row + (uint32_t)MENU_LIST_ROWS : cunt - MENU_STEP; return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static void menu_choice_key(const ldg_term_key_event_t *key, uint32_t *idx, uint32_t cunt, uint8_t *consumed)
{
    if (cunt == 0) { return; }

    if (key->key == (uint8_t)LDG_TERM_KEY_LHS) { *idx = *idx == 0 ? cunt - MENU_STEP : (*idx >= cunt ? cunt - MENU_STEP : *idx - MENU_STEP); *consumed = LDG_TRUTH_TRUE; }
    else if (key->key == (uint8_t)LDG_TERM_KEY_RHS || key->key == (uint8_t)LDG_TERM_KEY_ENTER) { *idx = *idx + MENU_STEP >= cunt ? (uint32_t)MENU_ROW_FIRST : *idx + MENU_STEP; *consumed = LDG_TRUTH_TRUE; }
}

static void menu_text_key(const ldg_term_key_event_t *key, char *text, uint32_t cap, uint8_t *consumed)
{
    uint32_t len = (uint32_t)strnlen(text, (uint64_t)cap);

    if (key->key == (uint8_t)LDG_TERM_KEY_BACKSPACE)
    {
        if (len != 0) { text[len - MENU_STEP] = LDG_STR_TERM; }

        *consumed = LDG_TRUTH_TRUE;

        return;
    }

    for (uint32_t at = 0; at < key->text_cunt && at < (uint32_t)LDG_TERM_KEY_TEXT_MAX; at++)
    {
        if (key->text[at] < (uint32_t)LDG_ASCII_PRINTABLE_MIN || key->text[at] > (uint32_t)LDG_ASCII_PRINTABLE_MAX) { continue; }

        *consumed = LDG_TRUTH_TRUE;

        if (len + LDG_STR_TERM_SIZE >= cap) { break; }

        text[len] = (char)key->text[at];
        len++;
        text[len] = LDG_STR_TERM;
    }
}

static uint32_t menu_line_put(ldg_term_render_t *render, uint32_t col, uint32_t row, const char *text, uint32_t rgba)
{
    return crosstalk_ui_cell_rgba_put(render, col, row, text, (uint64_t)MENU_LINE_CAP, rgba, 0x0);
}

static uint32_t menu_lit_put(ldg_term_render_t *render, uint32_t col, uint32_t row, const char *text, uint8_t lit)
{
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t bg = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (lit != LDG_TRUTH_TRUE) { return menu_line_put(render, col, row, text, (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA); }

    ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_LIT_RGBA, &fg);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_SEL_BG_RGBA, &bg); }

    if (ret != LDG_ERR_AOK) { return ret; }

    return crosstalk_ui_cell_text_put(render, col, row, text, (uint64_t)MENU_LINE_CAP, &fg, &bg);
}

static uint32_t menu_btn_put(ldg_term_render_t *render, uint32_t col, uint32_t *row, const char *label, uint8_t lit)
{
    char line[MENU_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    if (snprintf(line, sizeof(line), MENU_BTN_FMT, label) < 0) { return LDG_ERR_STR_FMT; }

    ret = menu_lit_put(render, col, *row, line, lit);
    *row = *row + MENU_STEP;

    return ret;
}

static uint32_t menu_choice_put(ldg_term_render_t *render, uint32_t col, uint32_t *row, const char *label, const char *items, uint32_t idx, uint8_t lit)
{
    char seg[MENU_SEG_CAP] = LDG_ARR_ZERO_INIT;
    char line[MENU_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    menu_seg_get(items, idx, seg, (uint32_t)sizeof(seg));

    if (snprintf(line, sizeof(line), MENU_CHOICE_FMT, label, seg) < 0) { return LDG_ERR_STR_FMT; }

    ret = menu_lit_put(render, col, *row, line, lit);
    *row = *row + MENU_STEP;

    return ret;
}

static uint32_t menu_text_put(ldg_term_render_t *render, uint32_t col, uint32_t *row, const char *label, const char *text, uint8_t lit)
{
    char line[MENU_LINE_CAP] = LDG_ARR_ZERO_INIT;
    ldg_term_render_color_t cursor = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;
    int32_t wrote = 0;

    wrote = snprintf(line, sizeof(line), MENU_TEXT_FMT, label, text);
    if (wrote < 0) { return LDG_ERR_STR_FMT; }

    ret = menu_lit_put(render, col, *row, line, lit);

    if (ret == LDG_ERR_AOK && lit == LDG_TRUTH_TRUE) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, &cursor); }

    if (ret == LDG_ERR_AOK && lit == LDG_TRUTH_TRUE) { ret = crosstalk_ui_cell_span_fill(render, col + (uint32_t)wrote, *row, (uint32_t)MENU_CURSOR_W, &cursor, &cursor); }

    *row = *row + MENU_STEP;

    return ret;
}

static uint32_t menu_toggle_put(ldg_term_render_t *render, uint32_t col, uint32_t *row, const char *label, uint8_t on, uint8_t lit)
{
    char line[MENU_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    if (snprintf(line, sizeof(line), on == LDG_TRUTH_TRUE ? MENU_TOGGLE_ON_FMT : MENU_TOGGLE_OFF_FMT, label) < 0) { return LDG_ERR_STR_FMT; }

    ret = menu_lit_put(render, col, *row, line, lit);
    *row = *row + MENU_STEP;

    return ret;
}

static uint32_t menu_title_put(ldg_term_render_t *render, const ldg_term_rect_t *rect, uint32_t *row, const char *title)
{
    uint32_t ret = menu_line_put(render, rect->col, rect->row, title, (uint32_t)CROSSTALK_UI_THEME_TITLE_RGBA);

    *row = rect->row + MENU_STEP + MENU_ROW_GAP;

    return ret;
}

static LDG_NOINLINE uint32_t menu_list_put(ldg_term_render_t *render, uint32_t col, uint32_t *row, uint32_t width, crosstalk_game_menu_nav_t *nav, const char *items, uint32_t cunt, uint8_t lit, const uint8_t *lock, uint32_t armed_row, const char *empty_text)
{
    char seg[MENU_SEG_CAP] = LDG_ARR_ZERO_INIT;
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t bg = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (cunt == 0)
    {
        ret = menu_line_put(render, col, *row, empty_text, (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA);
        *row = *row + (uint32_t)MENU_LIST_ROWS;

        return ret;
    }

    if (nav->row >= cunt) { nav->row = cunt - MENU_STEP; }

    if (nav->row < nav->scroll) { nav->scroll = nav->row; }

    if (nav->row >= nav->scroll + (uint32_t)MENU_LIST_ROWS) { nav->scroll = nav->row - (uint32_t)MENU_LIST_ROWS + MENU_STEP; }

    for (uint32_t at = 0; at < (uint32_t)MENU_LIST_ROWS && nav->scroll + at < cunt; at++)
    {
        uint32_t idx = nav->scroll + at;
        uint32_t fg_rgba = CROSSTALK_UI_THEME_TEXT_RGBA;
        uint32_t bg_rgba = CROSSTALK_UI_CELL_PANE_RGBA;

        menu_seg_get(items, idx, seg, (uint32_t)sizeof(seg));

        if (lock != 0x0 && lock[idx] == LDG_TRUTH_TRUE) { fg_rgba = MENU_COL_LOCKED; }

        if (idx == nav->row)
        {
            if (fg_rgba == (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA) { fg_rgba = CROSSTALK_UI_THEME_LIT_RGBA; }

            if (lit == LDG_TRUTH_TRUE) { bg_rgba = CROSSTALK_UI_THEME_SEL_BG_RGBA; }
        }

        if (idx == armed_row)
        {
            fg_rgba = CROSSTALK_UI_THEME_TEXT_RGBA;
            bg_rgba = CROSSTALK_UI_THEME_DANGER_RGBA;
        }

        ret = crosstalk_ui_cell_color_get(fg_rgba, &fg);
        if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get(bg_rgba, &bg); }

        if (ret == LDG_ERR_AOK && ((idx == nav->row && lit == LDG_TRUTH_TRUE) || idx == armed_row)) { ret = crosstalk_ui_cell_span_fill(render, col, *row + at, width, &fg, &bg); }

        if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_text_put(render, col, *row + at, seg, (uint64_t)sizeof(seg), &fg, &bg); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    *row = *row + (uint32_t)MENU_LIST_ROWS;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_menu_nav_rst(crosstalk_game_menu_nav_t *nav)
{
    crosstalk_game_menu_nav_t empty = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(nav == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *nav = empty;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_menu_draw(ldg_term_render_t *render, const ldg_term_rect_t *rect, crosstalk_game_menu_nav_t *nav)
{
    const char *label[MENU_MAIN_FIELD_CUNT] = { "Load Latest", "New Design", "Settings", MENU_HUB_TEXT, "Quit" };
    uint32_t row = 0;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(render == 0x0 || rect == 0x0 || nav == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (nav->field >= (uint32_t)MENU_MAIN_FIELD_CUNT) { nav->field = (uint32_t)MENU_FIELD_FIRST; }

    row = rect->row;

    for (uint32_t at = 0; at < (uint32_t)MENU_MAIN_FIELD_CUNT && ret == LDG_ERR_AOK; at++) { ret = menu_btn_put(render, rect->col, &row, label[at], nav->field == at ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    return ret;
}

uint32_t crosstalk_game_menu_key_isr(const ldg_term_key_event_t *key, crosstalk_game_menu_nav_t *nav, crosstalk_game_menu_action_t *out_action, uint8_t *consumed)
{
    const crosstalk_game_menu_action_t act[MENU_MAIN_FIELD_CUNT] = { CROSSTALK_GAME_MENU_ACTION_LD_LATEST, CROSSTALK_GAME_MENU_ACTION_NEW, CROSSTALK_GAME_MENU_ACTION_SETTINGS, CROSSTALK_GAME_MENU_ACTION_HUB, CROSSTALK_GAME_MENU_ACTION_QUIT };
    uint32_t mask = (uint32_t)LDG_BIT(MENU_MAIN_FIELD_CUNT) - MENU_STEP;

    if (LDG_UNLIKELY(key == 0x0 || nav == 0x0 || out_action == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_action = CROSSTALK_GAME_MENU_ACTION_NONE;
    *consumed = LDG_TRUTH_FALSE;

    if (menu_key_skip_is(key) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (nav->field >= (uint32_t)MENU_MAIN_FIELD_CUNT) { nav->field = (uint32_t)MENU_FIELD_FIRST; }

    if (menu_vert_key(key, nav, (uint32_t)MENU_MAIN_FIELD_CUNT, mask) == LDG_TRUTH_TRUE || menu_tab_key(key, nav, (uint32_t)MENU_MAIN_FIELD_CUNT, mask) == LDG_TRUTH_TRUE) { *consumed = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

    if (key->key != (uint8_t)LDG_TERM_KEY_ENTER) { return LDG_ERR_AOK; }

    *out_action = act[nav->field];
    *consumed = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t menu_nd_mask_get(uint32_t difficulty_idx)
{
    uint32_t mask = (uint32_t)LDG_BIT(MENU_ND_CUNT) - MENU_STEP;

    if (difficulty_idx == (uint32_t)MENU_DIFFICULTY_EASY) { mask = mask & ~(uint32_t)LDG_BIT(MENU_ND_LAYERS); }

    return mask;
}

static const char* menu_nd_layers_items_get(uint32_t difficulty_idx)
{
    return difficulty_idx == (uint32_t)MENU_DIFFICULTY_HARD ? MENU_LAYERS_ITEMS_HARD : MENU_LAYERS_ITEMS_MEDIUM;
}

uint32_t crosstalk_game_menu_newdesign_draw(ldg_term_render_t *render, const ldg_term_rect_t *rect, crosstalk_game_menu_nav_t *nav, uint32_t size_idx, uint32_t layers_idx, uint32_t difficulty_idx, const char *board_name)
{
    uint32_t row = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || rect == 0x0 || nav == 0x0 || board_name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    menu_field_fix(nav, (uint32_t)MENU_ND_CUNT, menu_nd_mask_get(difficulty_idx));

    ret = menu_title_put(render, rect, &row, MENU_NEW_TITLE_TEXT);
    if (ret == LDG_ERR_AOK) { ret = menu_text_put(render, rect->col, &row, MENU_NEW_NAME_LABEL, board_name, nav->field == (uint32_t)MENU_ND_NAME ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    if (ret == LDG_ERR_AOK) { ret = menu_choice_put(render, rect->col, &row, MENU_SIZE_LABEL, MENU_SIZE_ITEMS, size_idx, nav->field == (uint32_t)MENU_ND_SIZE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    if (ret == LDG_ERR_AOK) { ret = menu_choice_put(render, rect->col, &row, MENU_DIFFICULTY_LABEL, MENU_DIFFICULTY_ITEMS, difficulty_idx, nav->field == (uint32_t)MENU_ND_DIFF ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    if (ret == LDG_ERR_AOK && difficulty_idx != (uint32_t)MENU_DIFFICULTY_EASY) { ret = menu_choice_put(render, rect->col, &row, MENU_LAYERS_LABEL, menu_nd_layers_items_get(difficulty_idx), layers_idx, nav->field == (uint32_t)MENU_ND_LAYERS ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    row = row + MENU_ROW_GAP;

    if (ret == LDG_ERR_AOK) { ret = menu_btn_put(render, rect->col, &row, MENU_CREATE_TEXT, nav->field == (uint32_t)MENU_ND_CREATE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    if (ret == LDG_ERR_AOK) { ret = menu_btn_put(render, rect->col, &row, MENU_SEL_BACK_TEXT, nav->field == (uint32_t)MENU_ND_BACK ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    return ret;
}

uint32_t crosstalk_game_menu_newdesign_key_isr(const ldg_term_key_event_t *key, crosstalk_game_menu_nav_t *nav, uint32_t *size_idx, uint32_t *layers_idx, uint32_t *difficulty_idx, char *board_name, uint32_t board_name_cap, crosstalk_game_menu_newdesign_action_t *out_action, uint8_t *consumed)
{
    uint32_t mask = 0;

    if (LDG_UNLIKELY(key == 0x0 || nav == 0x0 || size_idx == 0x0 || layers_idx == 0x0 || difficulty_idx == 0x0 || board_name == 0x0 || out_action == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_action = CROSSTALK_GAME_MENU_NEWDESIGN_NONE;
    *consumed = LDG_TRUTH_FALSE;

    if (menu_key_skip_is(key) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    mask = menu_nd_mask_get(*difficulty_idx);
    menu_field_fix(nav, (uint32_t)MENU_ND_CUNT, mask);

    if (menu_vert_key(key, nav, (uint32_t)MENU_ND_CUNT, mask) == LDG_TRUTH_TRUE || menu_tab_key(key, nav, (uint32_t)MENU_ND_CUNT, mask) == LDG_TRUTH_TRUE) { *consumed = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

    if (nav->field == (uint32_t)MENU_ND_NAME) { menu_text_key(key, board_name, board_name_cap, consumed); }
    else if (nav->field == (uint32_t)MENU_ND_SIZE) { menu_choice_key(key, size_idx, menu_seg_cunt(MENU_SIZE_ITEMS), consumed); }
    else if (nav->field == (uint32_t)MENU_ND_DIFF)
    {
        menu_choice_key(key, difficulty_idx, menu_seg_cunt(MENU_DIFFICULTY_ITEMS), consumed);

        if (*difficulty_idx == (uint32_t)MENU_DIFFICULTY_EASY) { *layers_idx = (uint32_t)MENU_ROW_FIRST; }
        else if (*layers_idx >= menu_seg_cunt(menu_nd_layers_items_get(*difficulty_idx))) { *layers_idx = menu_seg_cunt(menu_nd_layers_items_get(*difficulty_idx)) - MENU_STEP; }
    }
    else if (nav->field == (uint32_t)MENU_ND_LAYERS) { menu_choice_key(key, layers_idx, menu_seg_cunt(menu_nd_layers_items_get(*difficulty_idx)), consumed); }
    else if (key->key == (uint8_t)LDG_TERM_KEY_ENTER)
    {
        *out_action = nav->field == (uint32_t)MENU_ND_CREATE ? CROSSTALK_GAME_MENU_NEWDESIGN_CREATE : CROSSTALK_GAME_MENU_NEWDESIGN_BACK;
        *consumed = LDG_TRUTH_TRUE;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_menu_editorpick_draw(ldg_term_render_t *render, const ldg_term_rect_t *rect, crosstalk_game_menu_nav_t *nav, const char *board_name, const char *items, uint32_t pick_idx)
{
    uint32_t mask = (uint32_t)LDG_BIT(MENU_EP_CUNT) - MENU_STEP;
    uint32_t row = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || rect == 0x0 || nav == 0x0 || board_name == 0x0 || items == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    menu_field_fix(nav, (uint32_t)MENU_EP_CUNT, mask);

    ret = menu_title_put(render, rect, &row, MENU_EDITORPICK_TITLE_TEXT);
    if (ret == LDG_ERR_AOK) { ret = menu_line_put(render, rect->col, row, board_name, (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA); }

    row = row + MENU_STEP + MENU_ROW_GAP;

    if (ret == LDG_ERR_AOK) { ret = menu_line_put(render, rect->col, row, MENU_EDITORPICK_LABEL, (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA); }

    row = row + MENU_STEP;

    if (ret == LDG_ERR_AOK) { ret = menu_choice_put(render, rect->col, &row, "", items, pick_idx, nav->field == (uint32_t)MENU_EP_PICK ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    row = row + MENU_ROW_GAP;

    if (ret == LDG_ERR_AOK) { ret = menu_btn_put(render, rect->col, &row, MENU_EDITORPICK_OPEN_LABEL, nav->field == (uint32_t)MENU_EP_OPEN ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    if (ret == LDG_ERR_AOK) { ret = menu_btn_put(render, rect->col, &row, MENU_SEL_BACK_TEXT, nav->field == (uint32_t)MENU_EP_BACK ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    return ret;
}

uint32_t crosstalk_game_menu_editorpick_key_isr(const ldg_term_key_event_t *key, crosstalk_game_menu_nav_t *nav, const char *items, uint32_t *pick_idx, crosstalk_game_menu_editorpick_action_t *out_action, uint8_t *consumed)
{
    uint32_t mask = (uint32_t)LDG_BIT(MENU_EP_CUNT) - MENU_STEP;

    if (LDG_UNLIKELY(key == 0x0 || nav == 0x0 || items == 0x0 || pick_idx == 0x0 || out_action == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_action = CROSSTALK_GAME_MENU_EDITORPICK_NONE;
    *consumed = LDG_TRUTH_FALSE;

    if (menu_key_skip_is(key) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    menu_field_fix(nav, (uint32_t)MENU_EP_CUNT, mask);

    if (menu_vert_key(key, nav, (uint32_t)MENU_EP_CUNT, mask) == LDG_TRUTH_TRUE || menu_tab_key(key, nav, (uint32_t)MENU_EP_CUNT, mask) == LDG_TRUTH_TRUE) { *consumed = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

    if (nav->field == (uint32_t)MENU_EP_PICK) { menu_choice_key(key, pick_idx, menu_seg_cunt(items), consumed); }
    else if (key->key == (uint8_t)LDG_TERM_KEY_ENTER)
    {
        *out_action = nav->field == (uint32_t)MENU_EP_OPEN ? CROSSTALK_GAME_MENU_EDITORPICK_OPEN : CROSSTALK_GAME_MENU_EDITORPICK_BACK;
        *consumed = LDG_TRUTH_TRUE;
    }

    return LDG_ERR_AOK;
}

static uint32_t menu_np_mask_get(uint32_t pick_cunt, uint32_t pick_idx)
{
    uint32_t mask = (uint32_t)LDG_BIT(MENU_NP_CUNT) - MENU_STEP;

    if (pick_cunt == 0) { mask = mask & ~(uint32_t)LDG_BIT(MENU_NP_PICK); }

    if (pick_idx != 0) { mask = mask & ~(uint32_t)LDG_BIT(MENU_NP_NAME); }

    return mask;
}

uint32_t crosstalk_game_menu_newproject_draw(ldg_term_render_t *render, const ldg_term_rect_t *rect, crosstalk_game_menu_nav_t *nav, const char *proj_name, const char *pick_items, uint32_t pick_cunt, uint32_t pick_idx)
{
    uint32_t row = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || rect == 0x0 || nav == 0x0 || proj_name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pick_cunt > 0 && pick_items == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    menu_field_fix(nav, (uint32_t)MENU_NP_CUNT, menu_np_mask_get(pick_cunt, pick_idx));

    ret = menu_title_put(render, rect, &row, MENU_NEWPROJECT_TITLE_TEXT);

    if (ret == LDG_ERR_AOK && pick_cunt > 0) { ret = menu_choice_put(render, rect->col, &row, MENU_NEW_PICK_LABEL, pick_items, pick_idx, nav->field == (uint32_t)MENU_NP_PICK ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    if (ret == LDG_ERR_AOK && pick_idx == 0) { ret = menu_text_put(render, rect->col, &row, MENU_NEW_NAME_LABEL, proj_name, nav->field == (uint32_t)MENU_NP_NAME ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    row = row + MENU_ROW_GAP;

    if (ret == LDG_ERR_AOK) { ret = menu_btn_put(render, rect->col, &row, MENU_CREATE_TEXT, nav->field == (uint32_t)MENU_NP_CREATE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    if (ret == LDG_ERR_AOK) { ret = menu_btn_put(render, rect->col, &row, MENU_SEL_BACK_TEXT, nav->field == (uint32_t)MENU_NP_BACK ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    return ret;
}

uint32_t crosstalk_game_menu_newproject_key_isr(const ldg_term_key_event_t *key, crosstalk_game_menu_nav_t *nav, char *proj_name, uint32_t proj_name_cap, const char *pick_items, uint32_t pick_cunt, uint32_t *pick_idx, crosstalk_game_menu_newproject_action_t *out_action, uint8_t *consumed)
{
    uint32_t mask = 0;

    if (LDG_UNLIKELY(key == 0x0 || nav == 0x0 || proj_name == 0x0 || pick_idx == 0x0 || out_action == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_action = CROSSTALK_GAME_MENU_NEWPROJECT_NONE;
    *consumed = LDG_TRUTH_FALSE;

    if (menu_key_skip_is(key) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    mask = menu_np_mask_get(pick_cunt, *pick_idx);
    menu_field_fix(nav, (uint32_t)MENU_NP_CUNT, mask);

    if (menu_vert_key(key, nav, (uint32_t)MENU_NP_CUNT, mask) == LDG_TRUTH_TRUE || menu_tab_key(key, nav, (uint32_t)MENU_NP_CUNT, mask) == LDG_TRUTH_TRUE) { *consumed = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

    if (nav->field == (uint32_t)MENU_NP_PICK) { menu_choice_key(key, pick_idx, menu_seg_cunt(pick_items), consumed); }
    else if (nav->field == (uint32_t)MENU_NP_NAME) { menu_text_key(key, proj_name, proj_name_cap, consumed); }
    else if (key->key == (uint8_t)LDG_TERM_KEY_ENTER)
    {
        *out_action = nav->field == (uint32_t)MENU_NP_CREATE ? CROSSTALK_GAME_MENU_NEWPROJECT_CREATE : CROSSTALK_GAME_MENU_NEWPROJECT_BACK;
        *consumed = LDG_TRUTH_TRUE;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_menu_mp_draw(ldg_term_render_t *render, const ldg_term_rect_t *rect, crosstalk_game_menu_nav_t *nav, const char *lobby_items, uint32_t lobby_cunt, const char *board_names, uint32_t board_cunt, uint32_t board_idx, uint32_t players_idx, uint32_t submode_idx, uint32_t transport_idx)
{
    uint32_t mask = (uint32_t)LDG_BIT(MENU_MP_CUNT) - MENU_STEP;
    uint32_t row = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || rect == 0x0 || nav == 0x0 || lobby_items == 0x0 || board_names == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    menu_field_fix(nav, (uint32_t)MENU_MP_CUNT, mask);

    ret = menu_title_put(render, rect, &row, MENU_MP_TITLE_TEXT);
    if (ret == LDG_ERR_AOK) { ret = menu_list_put(render, rect->col, &row, rect->col_cunt, nav, lobby_items, lobby_cunt, nav->field == (uint32_t)MENU_MP_LIST ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE, 0x0, UINT32_MAX, transport_idx == (uint32_t)MENU_MP_TRANSPORT_LAN ? MENU_MP_EMPTY_LAN_TEXT : MENU_MP_EMPTY_TEXT); }

    if (ret == LDG_ERR_AOK) { ret = menu_btn_put(render, rect->col, &row, MENU_MP_REFRESH_TEXT, nav->field == (uint32_t)MENU_MP_REFRESH ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    if (ret == LDG_ERR_AOK) { ret = menu_btn_put(render, rect->col, &row, MENU_MP_JOIN_TEXT, nav->field == (uint32_t)MENU_MP_JOIN ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    if (ret == LDG_ERR_AOK) { ret = menu_choice_put(render, rect->col, &row, MENU_MP_TRANSPORT_LABEL, MENU_MP_TRANSPORT_ITEMS, transport_idx, nav->field == (uint32_t)MENU_MP_TRANSPORT ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    row = row + MENU_ROW_GAP;

    if (ret == LDG_ERR_AOK) { ret = menu_line_put(render, rect->col, row, MENU_MP_HOST_LABEL, (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA); }

    row = row + MENU_STEP;

    if (ret == LDG_ERR_AOK && board_cunt == 0) { ret = menu_line_put(render, rect->col, row, MENU_MP_NO_BOARD_TEXT, (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA); row = row + MENU_STEP; }
    else if (ret == LDG_ERR_AOK) { ret = menu_choice_put(render, rect->col, &row, MENU_MP_BOARD_LABEL, board_names, board_idx, nav->field == (uint32_t)MENU_MP_BOARD ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    if (ret == LDG_ERR_AOK) { ret = menu_choice_put(render, rect->col, &row, MENU_MP_PLAYERS_LABEL, MENU_MP_PLAYERS_ITEMS, players_idx, nav->field == (uint32_t)MENU_MP_PLAYERS ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    if (ret == LDG_ERR_AOK) { ret = menu_choice_put(render, rect->col, &row, MENU_MP_MODE_LABEL, MENU_MP_MODE_ITEMS, submode_idx, nav->field == (uint32_t)MENU_MP_MODE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    row = row + MENU_ROW_GAP;

    if (ret == LDG_ERR_AOK) { ret = menu_btn_put(render, rect->col, &row, MENU_MP_HOST_TEXT, nav->field == (uint32_t)MENU_MP_HOST ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    if (ret == LDG_ERR_AOK) { ret = menu_btn_put(render, rect->col, &row, MENU_SEL_BACK_TEXT, nav->field == (uint32_t)MENU_MP_BACK ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    return ret;
}

uint32_t crosstalk_game_menu_mp_key_isr(const ldg_term_key_event_t *key, crosstalk_game_menu_nav_t *nav, uint32_t lobby_cunt, const char *board_names, uint32_t board_cunt, uint32_t *board_idx, uint32_t *players_idx, uint32_t *submode_idx, uint32_t *transport_idx, crosstalk_game_menu_mp_action_t *out_action, uint32_t *out_row, uint8_t *consumed)
{
    uint32_t mask = (uint32_t)LDG_BIT(MENU_MP_CUNT) - MENU_STEP;

    if (LDG_UNLIKELY(key == 0x0 || nav == 0x0 || board_names == 0x0 || board_idx == 0x0 || players_idx == 0x0 || submode_idx == 0x0 || transport_idx == 0x0 || out_action == 0x0 || out_row == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_action = CROSSTALK_GAME_MENU_MP_NONE;
    *out_row = UINT32_MAX;
    *consumed = LDG_TRUTH_FALSE;

    if (menu_key_skip_is(key) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    menu_field_fix(nav, (uint32_t)MENU_MP_CUNT, mask);

    if (menu_tab_key(key, nav, (uint32_t)MENU_MP_CUNT, mask) == LDG_TRUTH_TRUE) { *consumed = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

    if (nav->field == (uint32_t)MENU_MP_LIST)
    {
        if (menu_list_key(key, nav, lobby_cunt) == LDG_TRUTH_TRUE) { *consumed = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

        if (key->key == (uint8_t)LDG_TERM_KEY_ENTER && lobby_cunt > 0 && nav->row < lobby_cunt)
        {
            *out_action = CROSSTALK_GAME_MENU_MP_JOIN;
            *out_row = nav->row;
            *consumed = LDG_TRUTH_TRUE;
        }

        return LDG_ERR_AOK;
    }

    if (menu_vert_key(key, nav, (uint32_t)MENU_MP_CUNT, mask) == LDG_TRUTH_TRUE) { *consumed = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

    if (nav->field == (uint32_t)MENU_MP_TRANSPORT) { menu_choice_key(key, transport_idx, menu_seg_cunt(MENU_MP_TRANSPORT_ITEMS), consumed); }
    else if (nav->field == (uint32_t)MENU_MP_BOARD) { menu_choice_key(key, board_idx, board_cunt, consumed); }
    else if (nav->field == (uint32_t)MENU_MP_PLAYERS) { menu_choice_key(key, players_idx, menu_seg_cunt(MENU_MP_PLAYERS_ITEMS), consumed); }
    else if (nav->field == (uint32_t)MENU_MP_MODE) { menu_choice_key(key, submode_idx, menu_seg_cunt(MENU_MP_MODE_ITEMS), consumed); }
    else if (key->key == (uint8_t)LDG_TERM_KEY_ENTER)
    {
        *consumed = LDG_TRUTH_TRUE;

        if (nav->field == (uint32_t)MENU_MP_REFRESH) { *out_action = CROSSTALK_GAME_MENU_MP_REFRESH; }
        else if (nav->field == (uint32_t)MENU_MP_JOIN && lobby_cunt > 0 && nav->row < lobby_cunt)
        {
            *out_action = CROSSTALK_GAME_MENU_MP_JOIN;
            *out_row = nav->row;
        }
        else if (nav->field == (uint32_t)MENU_MP_HOST && board_cunt > 0)
        {
            *out_action = CROSSTALK_GAME_MENU_MP_HOST;
            *out_row = *board_idx;
        }
        else if (nav->field == (uint32_t)MENU_MP_BACK) { *out_action = CROSSTALK_GAME_MENU_MP_BACK; }
    }

    return LDG_ERR_AOK;
}

static uint32_t menu_rm_mask_get(uint8_t is_host, uint8_t rdy)
{
    uint32_t mask = (uint32_t)LDG_BIT(MENU_RM_CUNT) - MENU_STEP;

    if (is_host != LDG_TRUTH_TRUE) { mask = mask & ~(uint32_t)LDG_BIT(MENU_RM_START); }

    if (rdy != LDG_TRUTH_FALSE) { mask = mask & ~(uint32_t)LDG_BIT(MENU_RM_BUILD); }

    return mask;
}

uint32_t crosstalk_game_menu_room_draw(ldg_term_render_t *render, const ldg_term_rect_t *rect, crosstalk_game_menu_nav_t *nav, const char *member_items, uint32_t member_cunt, uint8_t is_host, uint8_t rdy)
{
    uint32_t row = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || rect == 0x0 || nav == 0x0 || member_items == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    menu_field_fix(nav, (uint32_t)MENU_RM_CUNT, menu_rm_mask_get(is_host, rdy));

    ret = menu_title_put(render, rect, &row, MENU_ROOM_TITLE_TEXT);
    if (ret == LDG_ERR_AOK) { ret = menu_list_put(render, rect->col, &row, rect->col_cunt, nav, member_items, member_cunt, nav->field == (uint32_t)MENU_RM_LIST ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE, 0x0, UINT32_MAX, ""); }

    if (ret == LDG_ERR_AOK) { ret = menu_toggle_put(render, rect->col, &row, MENU_ROOM_RDY_TEXT, rdy != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE, nav->field == (uint32_t)MENU_RM_RDY ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    row = row + MENU_ROW_GAP;

    if (ret == LDG_ERR_AOK && is_host == LDG_TRUTH_TRUE) { ret = menu_btn_put(render, rect->col, &row, MENU_ROOM_START_TEXT, nav->field == (uint32_t)MENU_RM_START ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    if (ret == LDG_ERR_AOK) { ret = menu_btn_put(render, rect->col, &row, MENU_ROOM_LEAVE_TEXT, nav->field == (uint32_t)MENU_RM_LEAVE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    if (ret == LDG_ERR_AOK && rdy == LDG_TRUTH_FALSE) { ret = menu_btn_put(render, rect->col, &row, MENU_ROOM_BUILD_TEXT, nav->field == (uint32_t)MENU_RM_BUILD ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    return ret;
}

uint32_t crosstalk_game_menu_room_key_isr(const ldg_term_key_event_t *key, crosstalk_game_menu_nav_t *nav, uint32_t member_cunt, uint8_t is_host, uint8_t rdy, crosstalk_game_menu_room_action_t *out_action, uint8_t *consumed)
{
    uint32_t mask = menu_rm_mask_get(is_host, rdy);

    if (LDG_UNLIKELY(key == 0x0 || nav == 0x0 || out_action == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_action = CROSSTALK_GAME_MENU_ROOM_NONE;
    *consumed = LDG_TRUTH_FALSE;

    if (menu_key_skip_is(key) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    menu_field_fix(nav, (uint32_t)MENU_RM_CUNT, mask);

    if (menu_tab_key(key, nav, (uint32_t)MENU_RM_CUNT, mask) == LDG_TRUTH_TRUE) { *consumed = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

    if (nav->field == (uint32_t)MENU_RM_LIST)
    {
        if (menu_list_key(key, nav, member_cunt) == LDG_TRUTH_TRUE) { *consumed = LDG_TRUTH_TRUE; }

        return LDG_ERR_AOK;
    }

    if (menu_vert_key(key, nav, (uint32_t)MENU_RM_CUNT, mask) == LDG_TRUTH_TRUE) { *consumed = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

    if (key->key != (uint8_t)LDG_TERM_KEY_ENTER) { return LDG_ERR_AOK; }

    *consumed = LDG_TRUTH_TRUE;

    if (nav->field == (uint32_t)MENU_RM_RDY) { *out_action = CROSSTALK_GAME_MENU_ROOM_RDY; }
    else if (nav->field == (uint32_t)MENU_RM_START) { *out_action = CROSSTALK_GAME_MENU_ROOM_START; }
    else if (nav->field == (uint32_t)MENU_RM_LEAVE) { *out_action = CROSSTALK_GAME_MENU_ROOM_LEAVE; }
    else { *out_action = CROSSTALK_GAME_MENU_ROOM_BUILD; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_menu_extras_draw(ldg_term_render_t *render, const ldg_term_rect_t *rect, crosstalk_game_menu_nav_t *nav, const char *items, uint32_t item_cunt)
{
    uint32_t row = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || rect == 0x0 || nav == 0x0 || items == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    menu_field_fix(nav, (uint32_t)MENU_EX_CUNT, (uint32_t)LDG_BIT(MENU_EX_CUNT) - MENU_STEP);

    ret = menu_title_put(render, rect, &row, MENU_EXTRAS_TITLE_TEXT);
    if (ret == LDG_ERR_AOK) { ret = menu_list_put(render, rect->col, &row, rect->col_cunt, nav, items, item_cunt, nav->field == (uint32_t)MENU_EX_LIST ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE, 0x0, UINT32_MAX, MENU_EXTRAS_EMPTY_TEXT); }

    if (ret == LDG_ERR_AOK) { ret = menu_btn_put(render, rect->col, &row, MENU_EXTRAS_OPEN_TEXT, nav->field == (uint32_t)MENU_EX_OPEN ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    if (ret == LDG_ERR_AOK) { ret = menu_btn_put(render, rect->col, &row, MENU_SEL_BACK_TEXT, nav->field == (uint32_t)MENU_EX_BACK ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    return ret;
}

uint32_t crosstalk_game_menu_extras_key_isr(const ldg_term_key_event_t *key, crosstalk_game_menu_nav_t *nav, uint32_t item_cunt, crosstalk_game_menu_extras_action_t *out_action, uint32_t *out_row, uint8_t *consumed)
{
    uint32_t mask = (uint32_t)LDG_BIT(MENU_EX_CUNT) - MENU_STEP;

    if (LDG_UNLIKELY(key == 0x0 || nav == 0x0 || out_action == 0x0 || out_row == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_action = CROSSTALK_GAME_MENU_EXTRAS_NONE;
    *out_row = UINT32_MAX;
    *consumed = LDG_TRUTH_FALSE;

    if (menu_key_skip_is(key) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    menu_field_fix(nav, (uint32_t)MENU_EX_CUNT, mask);

    if (menu_tab_key(key, nav, (uint32_t)MENU_EX_CUNT, mask) == LDG_TRUTH_TRUE) { *consumed = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

    if (nav->field == (uint32_t)MENU_EX_LIST && menu_list_key(key, nav, item_cunt) == LDG_TRUTH_TRUE) { *consumed = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

    if (nav->field != (uint32_t)MENU_EX_LIST && menu_vert_key(key, nav, (uint32_t)MENU_EX_CUNT, mask) == LDG_TRUTH_TRUE) { *consumed = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

    if (key->key != (uint8_t)LDG_TERM_KEY_ENTER) { return LDG_ERR_AOK; }

    *consumed = LDG_TRUTH_TRUE;

    if (nav->field == (uint32_t)MENU_EX_BACK) { *out_action = CROSSTALK_GAME_MENU_EXTRAS_BACK; return LDG_ERR_AOK; }

    if (item_cunt > 0 && nav->row < item_cunt)
    {
        *out_action = CROSSTALK_GAME_MENU_EXTRAS_PICK;
        *out_row = nav->row;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_menu_credits_draw(ldg_term_render_t *render, const ldg_term_rect_t *rect, crosstalk_game_menu_nav_t *nav, const char *items, uint32_t item_cunt)
{
    uint32_t row = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || rect == 0x0 || nav == 0x0 || items == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    menu_field_fix(nav, (uint32_t)MENU_CR_CUNT, (uint32_t)LDG_BIT(MENU_CR_CUNT) - MENU_STEP);

    ret = menu_title_put(render, rect, &row, MENU_CREDITS_TITLE_TEXT);
    if (ret == LDG_ERR_AOK) { ret = menu_list_put(render, rect->col, &row, rect->col_cunt, nav, items, item_cunt, nav->field == (uint32_t)MENU_CR_LIST ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE, 0x0, UINT32_MAX, MENU_CREDITS_EMPTY_TEXT); }

    if (ret == LDG_ERR_AOK) { ret = menu_btn_put(render, rect->col, &row, MENU_SEL_BACK_TEXT, nav->field == (uint32_t)MENU_CR_BACK ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    return ret;
}

uint32_t crosstalk_game_menu_credits_key_isr(const ldg_term_key_event_t *key, crosstalk_game_menu_nav_t *nav, uint32_t item_cunt, crosstalk_game_menu_credits_action_t *out_action, uint8_t *consumed)
{
    uint32_t mask = (uint32_t)LDG_BIT(MENU_CR_CUNT) - MENU_STEP;

    if (LDG_UNLIKELY(key == 0x0 || nav == 0x0 || out_action == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_action = CROSSTALK_GAME_MENU_CREDITS_NONE;
    *consumed = LDG_TRUTH_FALSE;

    if (menu_key_skip_is(key) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    menu_field_fix(nav, (uint32_t)MENU_CR_CUNT, mask);

    if (menu_tab_key(key, nav, (uint32_t)MENU_CR_CUNT, mask) == LDG_TRUTH_TRUE) { *consumed = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

    if (nav->field == (uint32_t)MENU_CR_LIST && menu_list_key(key, nav, item_cunt) == LDG_TRUTH_TRUE) { *consumed = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

    if (nav->field == (uint32_t)MENU_CR_BACK && key->key == (uint8_t)LDG_TERM_KEY_ENTER)
    {
        *out_action = CROSSTALK_GAME_MENU_CREDITS_BACK;
        *consumed = LDG_TRUTH_TRUE;
    }

    return LDG_ERR_AOK;
}

static uint8_t menu_sel_row_lock_is(const uint8_t *lock, uint32_t item_cunt, uint32_t row)
{
    if (lock == 0x0) { return LDG_TRUTH_FALSE; }

    if (row >= item_cunt) { return LDG_TRUTH_FALSE; }

    return lock[row] == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_game_menu_board_sel_draw(ldg_term_render_t *render, const ldg_term_rect_t *rect, crosstalk_game_menu_nav_t *nav, const char *items, const uint8_t *lock, uint32_t item_cunt, uint8_t truncated, uint32_t armed_row, uint8_t wshop_on)
{
    uint32_t row = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || rect == 0x0 || nav == 0x0 || items == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    menu_field_fix(nav, (uint32_t)MENU_BS_CUNT, (uint32_t)LDG_BIT(MENU_BS_CUNT) - MENU_STEP);

    ret = menu_title_put(render, rect, &row, MENU_SEL_TITLE_TEXT);
    if (ret == LDG_ERR_AOK) { ret = menu_list_put(render, rect->col, &row, rect->col_cunt, nav, items, item_cunt, nav->field == (uint32_t)MENU_BS_LIST ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE, lock, armed_row, MENU_SEL_EMPTY_TEXT); }

    if (ret == LDG_ERR_AOK && truncated != 0)
    {
        ret = menu_line_put(render, rect->col, row, MENU_SEL_TRUNC_TEXT, (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA);
        row = row + MENU_STEP;
    }

    if (ret == LDG_ERR_AOK) { ret = menu_toggle_put(render, rect->col, &row, MENU_SEL_WSHOP_TEXT, wshop_on != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE, nav->field == (uint32_t)MENU_BS_WSHOP ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    row = row + MENU_ROW_GAP;

    if (ret == LDG_ERR_AOK) { ret = menu_btn_put(render, rect->col, &row, MENU_SEL_OPEN_TEXT, nav->field == (uint32_t)MENU_BS_OPEN ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    if (ret == LDG_ERR_AOK) { ret = menu_btn_put(render, rect->col, &row, MENU_SEL_BACK_TEXT, nav->field == (uint32_t)MENU_BS_BACK ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    return ret;
}

uint32_t crosstalk_game_menu_board_sel_key_isr(const ldg_term_key_event_t *key, crosstalk_game_menu_nav_t *nav, const uint8_t *lock, uint32_t item_cunt, uint8_t *wshop_on, crosstalk_game_menu_board_sel_action_t *out_action, uint32_t *out_row, uint8_t *consumed)
{
    uint32_t mask = (uint32_t)LDG_BIT(MENU_BS_CUNT) - MENU_STEP;

    if (LDG_UNLIKELY(key == 0x0 || nav == 0x0 || wshop_on == 0x0 || out_action == 0x0 || out_row == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_action = CROSSTALK_GAME_MENU_BOARD_SEL_NONE;
    *out_row = UINT32_MAX;
    *consumed = LDG_TRUTH_FALSE;

    if (menu_key_skip_is(key) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    menu_field_fix(nav, (uint32_t)MENU_BS_CUNT, mask);

    if (menu_tab_key(key, nav, (uint32_t)MENU_BS_CUNT, mask) == LDG_TRUTH_TRUE) { *consumed = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

    if (nav->field == (uint32_t)MENU_BS_LIST)
    {
        if (menu_list_key(key, nav, item_cunt) == LDG_TRUTH_TRUE) { *consumed = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

        if (item_cunt == 0 || nav->row >= item_cunt) { return LDG_ERR_AOK; }

        if (key->key == (uint8_t)LDG_TERM_KEY_DELETE)
        {
            *out_action = CROSSTALK_GAME_MENU_BOARD_SEL_REMOVE;
            *out_row = nav->row;
            *consumed = LDG_TRUTH_TRUE;
        }
        else if (key->key == (uint8_t)LDG_TERM_KEY_ENTER && menu_sel_row_lock_is(lock, item_cunt, nav->row) != LDG_TRUTH_TRUE)
        {
            *out_action = CROSSTALK_GAME_MENU_BOARD_SEL_LD;
            *out_row = nav->row;
            *consumed = LDG_TRUTH_TRUE;
        }

        return LDG_ERR_AOK;
    }

    if (menu_vert_key(key, nav, (uint32_t)MENU_BS_CUNT, mask) == LDG_TRUTH_TRUE) { *consumed = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

    if (key->key != (uint8_t)LDG_TERM_KEY_ENTER) { return LDG_ERR_AOK; }

    *consumed = LDG_TRUTH_TRUE;

    if (nav->field == (uint32_t)MENU_BS_WSHOP) { *wshop_on = *wshop_on != LDG_TRUTH_FALSE ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE; }
    else if (nav->field == (uint32_t)MENU_BS_BACK) { *out_action = CROSSTALK_GAME_MENU_BOARD_SEL_BACK; }
    else if (item_cunt > 0 && nav->row < item_cunt && menu_sel_row_lock_is(lock, item_cunt, nav->row) != LDG_TRUTH_TRUE)
    {
        *out_action = CROSSTALK_GAME_MENU_BOARD_SEL_LD;
        *out_row = nav->row;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_menu_ld_draw(ldg_term_render_t *render, const ldg_term_rect_t *rect, const char *status, double frac)
{
    char pct_text[MENU_SEG_CAP] = LDG_ARR_ZERO_INIT;
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t fill = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t track = LDG_STRUCT_ZERO_INIT;
    uint32_t bar_w = MENU_LD_BAR_COLS;
    uint32_t fill_w = 0;
    uint32_t pct = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || rect == 0x0 || status == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (frac < MENU_LD_FRAC_MIN) { frac = MENU_LD_FRAC_MIN; }

    if (frac > MENU_LD_FRAC_MAX) { frac = MENU_LD_FRAC_MAX; }

    if (bar_w > rect->col_cunt) { bar_w = rect->col_cunt; }

    pct = (uint32_t)(frac * MENU_LD_PCT_SCALE);
    fill_w = (uint32_t)(frac * (double)bar_w);

    if (snprintf(pct_text, sizeof(pct_text), MENU_LD_PCT_FMT, pct) < 0) { pct_text[0] = LDG_STR_TERM; }

    ret = menu_line_put(render, rect->col, rect->row, status, (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, &fg); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_LIT_RGBA, &fill); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_SEL_BG_RGBA, &track); }

    if (ret == LDG_ERR_AOK && bar_w != 0) { ret = crosstalk_ui_cell_span_fill(render, rect->col, rect->row + MENU_STEP, bar_w, &fg, &track); }

    if (ret == LDG_ERR_AOK && fill_w != 0) { ret = crosstalk_ui_cell_span_fill(render, rect->col, rect->row + MENU_STEP, fill_w, &fg, &fill); }

    if (ret != LDG_ERR_AOK) { return ret; }

    return menu_line_put(render, rect->col + bar_w + MENU_ROW_GAP, rect->row + MENU_STEP, pct_text, (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA);
}
