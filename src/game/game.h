#ifndef CROSSTALK_GAME_H
#define CROSSTALK_GAME_H

#include <stdint.h>

#include <dangling/sys/progress.h>
#include <dangling/thread/pool.h>

#include "game/totals.h"
#include "game/progression/progression.h"
#include "game/fx/sfx.h"
#include "game/caption.h"
#include "game/settings.h"

#define CROSSTALK_GAME_FX_EXPOSURE 1.58
#define CROSSTALK_GAME_FX_BLOOM_THRESH 0.65
#define CROSSTALK_GAME_FX_BLOOM_INTENSITY 0.0
#define CROSSTALK_GAME_FX_BLOOM_RADIUS 0.55
#define CROSSTALK_GAME_FX_CHROMA_AMOUNT 0.001
#define CROSSTALK_GAME_FX_FLARE_R 1.0
#define CROSSTALK_GAME_FX_FLARE_G 0.95
#define CROSSTALK_GAME_FX_FLARE_B 0.9
#define CROSSTALK_GAME_FX_FLARE_THRESH 0.75
#define CROSSTALK_GAME_FX_FLARE_INTENSITY 0.0
#define CROSSTALK_GAME_FX_FLARE_DISPERSAL 0.3
#define CROSSTALK_GAME_FX_FLARE_HALO 0.4
#define CROSSTALK_GAME_FX_DOF_NEAR 1.0
#define CROSSTALK_GAME_FX_DOF_FAR 2000.0
#define CROSSTALK_GAME_FX_DOF_FOCUS_RANGE 40.0
#define CROSSTALK_GAME_FX_DOF_BLUR_MAX 1.15
#define CROSSTALK_GAME_FX_GODRAY_TINT_R 1.0
#define CROSSTALK_GAME_FX_GODRAY_TINT_G 0.95
#define CROSSTALK_GAME_FX_GODRAY_TINT_B 0.85
#define CROSSTALK_GAME_FX_GODRAY_DENSITY 0.9
#define CROSSTALK_GAME_FX_GODRAY_DECAY 0.95
#define CROSSTALK_GAME_FX_GODRAY_WEIGHT 0.0
#define CROSSTALK_GAME_FX_GODRAY_EXPOSURE 0.3
#define CROSSTALK_GAME_FX_GRADE_CONTRAST 1.08
#define CROSSTALK_GAME_FX_GRADE_SATURATION 0.0
#define CROSSTALK_GAME_FX_GRADE_BRIGHTNESS 1.0
#define CROSSTALK_GAME_FX_GRADE_TEMP 0.02
#define CROSSTALK_GAME_FX_GRADE_VIGNETTE 0.4
#define CROSSTALK_GAME_FX_AO_RADIUS 0.6
#define CROSSTALK_GAME_FX_AO_INTENSITY 1.45
#define CROSSTALK_GAME_FX_AO_POWER 1.5
#define CROSSTALK_GAME_FX_SSR_STRENGTH 0.0
#define CROSSTALK_GAME_FX_SSR_THICKNESS 0.0
#define CROSSTALK_GAME_FX_SHARP_AMOUNT 0.85
#define CROSSTALK_GAME_FX_AA_THRESH 0.0
#define CROSSTALK_GAME_FX_GRAIN_AMOUNT 0.0
#define CROSSTALK_GAME_FX_GRADE_LUT_INTENSITY 0.15
#define CROSSTALK_GAME_FX_LIGHT_INTENSITY 12.0
#define CROSSTALK_GAME_FX_SSAA_DEFAULT 0
#define CROSSTALK_GAME_FX_DOF_QUALITY_DEFAULT 0

typedef enum crosstalk_game_screen
{
    CROSSTALK_GAME_SCREEN_MENU = 0,
    CROSSTALK_GAME_SCREEN_LD,
    CROSSTALK_GAME_SCREEN_NEWDESIGN,
    CROSSTALK_GAME_SCREEN_BOARD_SEL,
    CROSSTALK_GAME_SCREEN_MP,
    CROSSTALK_GAME_SCREEN_EDITOR,
    CROSSTALK_GAME_SCREEN_EXTRAS,
    CROSSTALK_GAME_SCREEN_CREDITS,
    CROSSTALK_GAME_SCREEN_NEWPROJECT,
    CROSSTALK_GAME_SCREEN_EDITORPICK
} crosstalk_game_screen_t;

#define CROSSTALK_GAME_SHADOW_QUALITY_DEFAULT 4096

typedef enum crosstalk_game_ld_kind
{
    CROSSTALK_LD_STARTUP = 0,
    CROSSTALK_LD_NEW,
    CROSSTALK_LD_FILE,
    CROSSTALK_LD_BARE
} crosstalk_game_ld_kind_t;

typedef struct crosstalk_game_ld
{
    ldg_sys_progress_t bar;
    uint32_t phase;
    uint32_t total;
    uint32_t done;
    uint32_t err;
    crosstalk_game_ld_kind_t kind;
    crosstalk_game_screen_t tgt;
    uint8_t settings_present;
    uint8_t state_present;
    uint8_t preloaded;
    uint8_t pudding[5];
} crosstalk_game_ld_t;

#define CROSSTALK_GAME_AUTHOR_NAME_CAP 64

typedef struct crosstalk_game_ctx
{
    crosstalk_game_ld_t ld;
    ldg_thread_pool_t ld_pool;
    ldg_thread_pool_t music_pool;
    crosstalk_game_totals_ctx_t totals;
    crosstalk_game_progression_state_t progression;
    crosstalk_game_settings_t settings;
    crosstalk_game_last_state_t last_state;
    crosstalk_game_fx_sfx_ctx_t sfx_sound;
    crosstalk_game_caption_ctx_t caption;
    uint64_t author_steam_id;
    uint64_t created_epoch_ms;
    uint64_t bench_ms_total;
    uint64_t sim_ms_total;
    char author_name[CROSSTALK_GAME_AUTHOR_NAME_CAP];
    crosstalk_game_screen_t screen;
    uint8_t quit;
    uint8_t ld_active;
    uint8_t ld_finalized;
    uint8_t paused;
    uint8_t net_join_phase;
    uint8_t pudding[7];
} crosstalk_game_ctx_t;

#endif
