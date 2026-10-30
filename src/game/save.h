#ifndef GAME_SAVE_H
#define GAME_SAVE_H

#include <stdint.h>

#include "game/settings.h"

#define CROSSTALK_GAME_SAVE_SECT_ZONE "ZONE"
#define CROSSTALK_GAME_SAVE_SECT_LAYOUT "LAYOUT"
#define CROSSTALK_GAME_SAVE_SECT_RTL "RTL"
#define CROSSTALK_GAME_SAVE_SECT_SCRIPT "SCRIPT"
#define CROSSTALK_GAME_SAVE_SECT_CALC "CALC"
#define CROSSTALK_GAME_SAVE_SECT_TERM "TERM"
#define CROSSTALK_GAME_SAVE_SECT_CRT "CRT"
#define CROSSTALK_GAME_SAVE_SECT_RENG "RENG"
#define CROSSTALK_GAME_SAVE_SECT_EMU "EMU"
#define CROSSTALK_GAME_SAVE_SIZE_PROBE 0

typedef struct crosstalk_game_save_editor_sect
{
    const char *name;
    const uint8_t *blob;
    uint64_t work_ms;
    uint32_t blob_len;
    uint8_t pudding[4];
} crosstalk_game_save_editor_sect_t;

uint32_t crosstalk_game_save_wr(const crosstalk_game_settings_t *settings, const crosstalk_game_last_state_t *state, uint64_t author_steam_id, const char *author_name, uint64_t created_epoch_ms, uint64_t bench_ms_total, uint64_t sim_ms_total, const crosstalk_game_save_editor_sect_t *editor_sects, uint32_t editor_sect_cunt, const char *path);
uint32_t crosstalk_game_save_editor_sect_rd(const char *path, const char *name, uint8_t *buff, uint32_t cap, uint32_t *out_len);
uint32_t crosstalk_game_save_rd(const char *path, crosstalk_game_settings_t *out_settings, uint8_t *out_settings_present, crosstalk_game_last_state_t *out_state, uint8_t *out_state_present, uint64_t *out_author_steam_id, char *out_author_name, uint32_t author_name_cap, uint64_t *out_created_epoch_ms, uint64_t *out_ld_ms_total, uint64_t *out_sim_ms_total);
uint32_t crosstalk_game_save_meta_rd(const char *path, char *out_author_name, uint32_t author_name_cap, uint64_t *out_created_epoch_ms, char *out_editors, uint32_t editors_cap);
uint32_t crosstalk_game_save_settings_rd(const char *path, crosstalk_game_settings_t *out_settings, uint8_t *out_present);

#endif
