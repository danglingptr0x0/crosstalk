#ifndef CROSSTALK_GAME_SETTINGS_MENU_H
#define CROSSTALK_GAME_SETTINGS_MENU_H

#include <stdint.h>

#include <dangling/term/layout.h>
#include <dangling/term/render.h>
#include <dangling/term/vt.h>

#include "game/music.h"
#include "game/settings.h"

#define CROSSTALK_GAME_SETTINGS_WHITELIST_ROW_CAP (CROSSTALK_GAME_SETTINGS_WHITELIST_CAP + CROSSTALK_GAME_MUSIC_SESH_CAP)
#define CROSSTALK_GAME_SETTINGS_MENU_PUDDING_SIZE 4
#define CROSSTALK_GAME_SETTINGS_LINK_NONE 0
#define CROSSTALK_GAME_SETTINGS_LINK_DO 1
#define CROSSTALK_GAME_SETTINGS_LINK_UNDO 2

#define CROSSTALK_GAME_SETTINGS_KEYBIND_NOTE_NONE 0
#define CROSSTALK_GAME_SETTINGS_KEYBIND_NOTE_TAKEN 1
#define CROSSTALK_GAME_SETTINGS_KEYBIND_NOTE_MOD 2

typedef struct crosstalk_game_settings_menu_ctx
{
    uint64_t kb_esc_ms;
    uint64_t kb_note_ms;
    uint64_t whitelist_esc_at;
    ldg_sys_media_info_t whitelist_sesh[CROSSTALK_GAME_MUSIC_SESH_CAP];
    uint32_t sel;
    uint32_t kb_sel;
    uint32_t kb_note_row;
    uint32_t kb_pending[CROSSTALK_GAME_KEYBIND_CUNT];
    uint32_t whitelist_sel;
    uint32_t whitelist_row_cunt;
    char whitelist_pending[CROSSTALK_GAME_SETTINGS_WHITELIST_CAP][CROSSTALK_GAME_SETTINGS_WHITELIST_NAME_CAP];
    char whitelist_row[CROSSTALK_GAME_SETTINGS_WHITELIST_ROW_CAP][CROSSTALK_GAME_SETTINGS_WHITELIST_NAME_CAP];
    uint8_t whitelist_row_live[CROSSTALK_GAME_SETTINGS_WHITELIST_ROW_CAP];
    uint8_t link_action;
    uint8_t changed;
    uint8_t kb_open;
    uint8_t kb_armed;
    uint8_t kb_esc_armed;
    uint8_t kb_note;
    uint8_t whitelist_open;
    uint8_t whitelist_esc_armed;
    uint8_t pudding[CROSSTALK_GAME_SETTINGS_MENU_PUDDING_SIZE];
} crosstalk_game_settings_menu_ctx_t;

uint32_t crosstalk_game_settings_menu_init(crosstalk_game_settings_menu_ctx_t *ctx);
uint32_t crosstalk_game_settings_menu_draw(const crosstalk_game_settings_menu_ctx_t *ctx, const crosstalk_game_settings_t *settings, uint8_t discord_linked, ldg_term_render_t *render, const ldg_term_rect_t *rect);
uint32_t crosstalk_game_settings_menu_key_isr(crosstalk_game_settings_menu_ctx_t *ctx, crosstalk_game_settings_t *settings, uint8_t discord_linked, const ldg_term_key_event_t *key, uint8_t *consumed);
uint32_t crosstalk_game_settings_menu_out_get(crosstalk_game_settings_menu_ctx_t *ctx, uint8_t *out_link_action, uint8_t *out_changed);
uint8_t crosstalk_game_settings_menu_keybind_open_is(const crosstalk_game_settings_menu_ctx_t *ctx);
uint8_t crosstalk_game_settings_menu_whitelist_open_is(const crosstalk_game_settings_menu_ctx_t *ctx);

#endif
