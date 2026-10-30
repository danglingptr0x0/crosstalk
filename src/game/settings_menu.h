#ifndef CROSSTALK_GAME_SETTINGS_MENU_H
#define CROSSTALK_GAME_SETTINGS_MENU_H

#include <stdint.h>

#include <dangling/term/layout.h>
#include <dangling/term/render.h>
#include <dangling/term/vt.h>

#include "game/settings.h"

#define CROSSTALK_GAME_SETTINGS_LINK_NONE 0
#define CROSSTALK_GAME_SETTINGS_LINK_DO 1
#define CROSSTALK_GAME_SETTINGS_LINK_UNDO 2

typedef struct crosstalk_game_settings_menu_ctx
{
    uint32_t sel;
    uint8_t link_action;
    uint8_t changed;
    uint8_t pudding[2];
} crosstalk_game_settings_menu_ctx_t;

uint32_t crosstalk_game_settings_menu_init(crosstalk_game_settings_menu_ctx_t *ctx);
uint32_t crosstalk_game_settings_menu_draw(const crosstalk_game_settings_menu_ctx_t *ctx, const crosstalk_game_settings_t *settings, uint8_t discord_linked, ldg_term_render_t *render, const ldg_term_rect_t *rect);
uint32_t crosstalk_game_settings_menu_key_isr(crosstalk_game_settings_menu_ctx_t *ctx, crosstalk_game_settings_t *settings, uint8_t discord_linked, const ldg_term_key_event_t *key, uint8_t *consumed);
uint32_t crosstalk_game_settings_menu_out_get(crosstalk_game_settings_menu_ctx_t *ctx, uint8_t *out_link_action, uint8_t *out_changed);

#endif
