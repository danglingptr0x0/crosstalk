#ifndef CROSSTALK_GAME_MENU_H
#define CROSSTALK_GAME_MENU_H

#include <stdint.h>

#include <dangling/term/layout.h>
#include <dangling/term/render.h>
#include <dangling/term/vt.h>

typedef enum crosstalk_game_menu_action
{
    CROSSTALK_GAME_MENU_ACTION_NONE = 0,
    CROSSTALK_GAME_MENU_ACTION_LD_LATEST,
    CROSSTALK_GAME_MENU_ACTION_NEW,
    CROSSTALK_GAME_MENU_ACTION_SETTINGS,
    CROSSTALK_GAME_MENU_ACTION_HUB,
    CROSSTALK_GAME_MENU_ACTION_QUIT
} crosstalk_game_menu_action_t;

typedef enum crosstalk_game_menu_board_sel_action
{
    CROSSTALK_GAME_MENU_BOARD_SEL_NONE = 0,
    CROSSTALK_GAME_MENU_BOARD_SEL_LD,
    CROSSTALK_GAME_MENU_BOARD_SEL_REMOVE,
    CROSSTALK_GAME_MENU_BOARD_SEL_BACK
} crosstalk_game_menu_board_sel_action_t;

typedef enum crosstalk_game_menu_mp_action
{
    CROSSTALK_GAME_MENU_MP_NONE = 0,
    CROSSTALK_GAME_MENU_MP_REFRESH,
    CROSSTALK_GAME_MENU_MP_JOIN,
    CROSSTALK_GAME_MENU_MP_HOST,
    CROSSTALK_GAME_MENU_MP_BACK
} crosstalk_game_menu_mp_action_t;

typedef enum crosstalk_game_menu_room_action
{
    CROSSTALK_GAME_MENU_ROOM_NONE = 0,
    CROSSTALK_GAME_MENU_ROOM_RDY,
    CROSSTALK_GAME_MENU_ROOM_START,
    CROSSTALK_GAME_MENU_ROOM_LEAVE,
    CROSSTALK_GAME_MENU_ROOM_BUILD
} crosstalk_game_menu_room_action_t;

typedef enum crosstalk_game_menu_extras_action
{
    CROSSTALK_GAME_MENU_EXTRAS_NONE = 0,
    CROSSTALK_GAME_MENU_EXTRAS_PICK,
    CROSSTALK_GAME_MENU_EXTRAS_BACK
} crosstalk_game_menu_extras_action_t;

typedef enum crosstalk_game_menu_credits_action
{
    CROSSTALK_GAME_MENU_CREDITS_NONE = 0,
    CROSSTALK_GAME_MENU_CREDITS_BACK
} crosstalk_game_menu_credits_action_t;

typedef enum crosstalk_game_menu_editorpick_action
{
    CROSSTALK_GAME_MENU_EDITORPICK_NONE = 0,
    CROSSTALK_GAME_MENU_EDITORPICK_OPEN,
    CROSSTALK_GAME_MENU_EDITORPICK_BACK
} crosstalk_game_menu_editorpick_action_t;

typedef enum crosstalk_game_menu_newproject_action
{
    CROSSTALK_GAME_MENU_NEWPROJECT_NONE = 0,
    CROSSTALK_GAME_MENU_NEWPROJECT_CREATE,
    CROSSTALK_GAME_MENU_NEWPROJECT_BACK
} crosstalk_game_menu_newproject_action_t;

typedef enum crosstalk_game_menu_newdesign_action
{
    CROSSTALK_GAME_MENU_NEWDESIGN_NONE = 0,
    CROSSTALK_GAME_MENU_NEWDESIGN_CREATE,
    CROSSTALK_GAME_MENU_NEWDESIGN_BACK
} crosstalk_game_menu_newdesign_action_t;

typedef struct crosstalk_game_menu_nav
{
    uint32_t field;
    uint32_t row;
    uint32_t scroll;
    uint8_t pudding[4];
} crosstalk_game_menu_nav_t;

uint32_t crosstalk_game_menu_nav_rst(crosstalk_game_menu_nav_t *nav);

uint32_t crosstalk_game_menu_draw(ldg_term_render_t *render, const ldg_term_rect_t *rect, crosstalk_game_menu_nav_t *nav);
uint32_t crosstalk_game_menu_key_isr(const ldg_term_key_event_t *key, crosstalk_game_menu_nav_t *nav, crosstalk_game_menu_action_t *out_action, uint8_t *consumed);

uint32_t crosstalk_game_menu_newdesign_draw(ldg_term_render_t *render, const ldg_term_rect_t *rect, crosstalk_game_menu_nav_t *nav, uint32_t size_idx, uint32_t layers_idx, uint32_t difficulty_idx, const char *board_name);
uint32_t crosstalk_game_menu_newdesign_key_isr(const ldg_term_key_event_t *key, crosstalk_game_menu_nav_t *nav, uint32_t *size_idx, uint32_t *layers_idx, uint32_t *difficulty_idx, char *board_name, uint32_t board_name_cap, crosstalk_game_menu_newdesign_action_t *out_action, uint8_t *consumed);

uint32_t crosstalk_game_menu_editorpick_draw(ldg_term_render_t *render, const ldg_term_rect_t *rect, crosstalk_game_menu_nav_t *nav, const char *board_name, const char *items, uint32_t pick_idx);
uint32_t crosstalk_game_menu_editorpick_key_isr(const ldg_term_key_event_t *key, crosstalk_game_menu_nav_t *nav, const char *items, uint32_t *pick_idx, crosstalk_game_menu_editorpick_action_t *out_action, uint8_t *consumed);

uint32_t crosstalk_game_menu_newproject_draw(ldg_term_render_t *render, const ldg_term_rect_t *rect, crosstalk_game_menu_nav_t *nav, const char *proj_name, const char *pick_items, uint32_t pick_cunt, uint32_t pick_idx);
uint32_t crosstalk_game_menu_newproject_key_isr(const ldg_term_key_event_t *key, crosstalk_game_menu_nav_t *nav, char *proj_name, uint32_t proj_name_cap, const char *pick_items, uint32_t pick_cunt, uint32_t *pick_idx, crosstalk_game_menu_newproject_action_t *out_action, uint8_t *consumed);

uint32_t crosstalk_game_menu_mp_draw(ldg_term_render_t *render, const ldg_term_rect_t *rect, crosstalk_game_menu_nav_t *nav, const char *lobby_items, uint32_t lobby_cunt, const char *board_names, uint32_t board_cunt, uint32_t board_idx, uint32_t players_idx, uint32_t submode_idx, uint32_t transport_idx);
uint32_t crosstalk_game_menu_mp_key_isr(const ldg_term_key_event_t *key, crosstalk_game_menu_nav_t *nav, uint32_t lobby_cunt, const char *board_names, uint32_t board_cunt, uint32_t *board_idx, uint32_t *players_idx, uint32_t *submode_idx, uint32_t *transport_idx, crosstalk_game_menu_mp_action_t *out_action, uint32_t *out_row, uint8_t *consumed);

uint32_t crosstalk_game_menu_room_draw(ldg_term_render_t *render, const ldg_term_rect_t *rect, crosstalk_game_menu_nav_t *nav, const char *member_items, uint32_t member_cunt, uint8_t is_host, uint8_t rdy);
uint32_t crosstalk_game_menu_room_key_isr(const ldg_term_key_event_t *key, crosstalk_game_menu_nav_t *nav, uint32_t member_cunt, uint8_t is_host, uint8_t rdy, crosstalk_game_menu_room_action_t *out_action, uint8_t *consumed);

uint32_t crosstalk_game_menu_extras_draw(ldg_term_render_t *render, const ldg_term_rect_t *rect, crosstalk_game_menu_nav_t *nav, const char *items, uint32_t item_cunt);
uint32_t crosstalk_game_menu_extras_key_isr(const ldg_term_key_event_t *key, crosstalk_game_menu_nav_t *nav, uint32_t item_cunt, crosstalk_game_menu_extras_action_t *out_action, uint32_t *out_row, uint8_t *consumed);

uint32_t crosstalk_game_menu_credits_draw(ldg_term_render_t *render, const ldg_term_rect_t *rect, crosstalk_game_menu_nav_t *nav, const char *items, uint32_t item_cunt);
uint32_t crosstalk_game_menu_credits_key_isr(const ldg_term_key_event_t *key, crosstalk_game_menu_nav_t *nav, uint32_t item_cunt, crosstalk_game_menu_credits_action_t *out_action, uint8_t *consumed);

uint32_t crosstalk_game_menu_board_sel_draw(ldg_term_render_t *render, const ldg_term_rect_t *rect, crosstalk_game_menu_nav_t *nav, const char *items, const uint8_t *lock, uint32_t item_cunt, uint8_t truncated, uint32_t armed_row, uint8_t wshop_on);
uint32_t crosstalk_game_menu_board_sel_key_isr(const ldg_term_key_event_t *key, crosstalk_game_menu_nav_t *nav, const uint8_t *lock, uint32_t item_cunt, uint8_t *wshop_on, crosstalk_game_menu_board_sel_action_t *out_action, uint32_t *out_row, uint8_t *consumed);

uint32_t crosstalk_game_menu_ld_draw(ldg_term_render_t *render, const ldg_term_rect_t *rect, const char *status, double frac);

#endif
