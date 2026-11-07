#ifndef GAME_SETTINGS_H
#define GAME_SETTINGS_H

#include <stdint.h>

#include <dangling/term/vt.h>

#define CROSSTALK_GAME_AA_MODE_OFF 0
#define CROSSTALK_GAME_AA_MODE_SMAA 1
#define CROSSTALK_GAME_AA_MODE_TAA 2
#define CROSSTALK_GAME_AA_MODE_IDX_MAX 2
#define CROSSTALK_GAME_LAST_STATE_LAYOUT_REV 1.0
#define CROSSTALK_GAME_KEYBIND_CODE_MASK 0xFFU
#define CROSSTALK_GAME_KEYBIND_SPECIAL 0x100U
#define CROSSTALK_GAME_KEYBIND_CTRL 0x200U
#define CROSSTALK_GAME_KEYBIND_SHIFT 0x400U
#define CROSSTALK_GAME_KEYBIND_ALT 0x800U
#define CROSSTALK_GAME_KEYBIND_MOD_MASK 0xE00U
#define CROSSTALK_GAME_KEYBIND_UNBOUND 0x0U
#define CROSSTALK_GAME_KEYBIND_FMT_CAP 48

typedef enum crosstalk_game_keybind_action
{
    CROSSTALK_GAME_KEYBIND_CMDLINE = 0,
    CROSSTALK_GAME_KEYBIND_HUB,
    CROSSTALK_GAME_KEYBIND_MUSIC_PREV,
    CROSSTALK_GAME_KEYBIND_MUSIC_NEXT,
    CROSSTALK_GAME_KEYBIND_MUSIC_TOGGLE,
    CROSSTALK_GAME_KEYBIND_WIN_RECENT,
    CROSSTALK_GAME_KEYBIND_WIN_CYCLE,
    CROSSTALK_GAME_KEYBIND_WIN_FOCUS,
    CROSSTALK_GAME_KEYBIND_WIN_DIGIT,
    CROSSTALK_GAME_KEYBIND_WIN_RESIZE,
    CROSSTALK_GAME_KEYBIND_WIN_MOV,
    CROSSTALK_GAME_KEYBIND_PAUSE,
    CROSSTALK_GAME_KEYBIND_SHOT,
    CROSSTALK_GAME_KEYBIND_SAVE,
    CROSSTALK_GAME_KEYBIND_OPEN,
    CROSSTALK_GAME_KEYBIND_INVITE,
    CROSSTALK_GAME_KEYBIND_CUNT
} crosstalk_game_keybind_action_t;

typedef struct crosstalk_game_settings
{
    double tonemap_exposure;
    double bloom_threshold;
    double bloom_intensity;
    double bloom_radius;
    double chroma_amount;
    double flare_col_r;
    double flare_col_g;
    double flare_col_b;
    double flare_threshold;
    double flare_intensity;
    double flare_dispersal;
    double flare_halo_width;
    double dof_focus_range;
    double dof_blur_max;
    double dof_near;
    double dof_far;
    double godray_col_r;
    double godray_col_g;
    double godray_col_b;
    double godray_density;
    double godray_decay;
    double godray_weight;
    double godray_exposure;
    double grade_contrast;
    double grade_saturation;
    double grade_brightness;
    double grade_temp;
    double grade_vignette;
    double ao_radius;
    double ao_intensity;
    double ao_power;
    double ssr_strength;
    double ssr_thickness;
    double sharp_amount;
    double aa_threshold;
    double grain_amount;
    double grade_lut_intensity;
    double light_intensity;
    double discord_vol_in;
    double discord_vol_out;
    uint32_t res_w;
    uint32_t res_h;
    uint32_t ssaa;
    uint32_t dof_quality;
    uint32_t tex_quality;
    uint32_t aniso;
    uint32_t shadow_quality;
    uint32_t aa_mode;
    uint8_t bloom_on;
    uint8_t chroma_on;
    uint8_t flare_on;
    uint8_t dof_on;
    uint8_t godray_on;
    uint8_t grade_on;
    uint8_t grade_lut_on;
    uint8_t ao_on;
    uint8_t ssr_on;
    uint8_t sharp_on;
    uint8_t grain_on;
    uint8_t discord_on;
    uint8_t discord_presence_on;
    uint8_t discord_voice_on;
    uint8_t discord_self_mute;
    uint8_t discord_self_deaf;
    uint8_t fab_safety_off;
    uint8_t fab_interlock_off;
    uint8_t discord_link_on;
    uint8_t music_src;
    uint8_t music_presence_on;
    uint8_t focus_pause_on;
    uint8_t pudding[2];
    uint32_t keybind[CROSSTALK_GAME_KEYBIND_CUNT];
} crosstalk_game_settings_t;

typedef struct crosstalk_game_settings_tip
{
    uint64_t dwell_since;
    uint32_t row_idx;
    uint8_t pudding[4];
} crosstalk_game_settings_tip_t;

typedef struct crosstalk_game_last_state
{
    double cam_hdg;
    double cam_pitch;
    double cam_range;
    double cam_offset_x;
    double cam_offset_y;
    double cam_offset_z;
    double net_win_x;
    double net_win_y;
    double layer_win_x;
    double layer_win_y;
    double city_win_x;
    double city_win_y;
    double net_win_w;
    double net_win_h;
    double layer_win_w;
    double layer_win_h;
    double city_win_w;
    double city_win_h;
    double city_tax_v;
    double layout_rev;
} crosstalk_game_last_state_t;

uint8_t crosstalk_game_settings_fab_unlocked_is(const crosstalk_game_settings_t *settings);
uint32_t crosstalk_game_settings_quantize(crosstalk_game_settings_t *settings);
uint32_t crosstalk_game_settings_serialize(const crosstalk_game_settings_t *settings, uint8_t *out, uint32_t cap, uint32_t *out_len);
uint32_t crosstalk_game_settings_deserialize(crosstalk_game_settings_t *settings, const uint8_t *buff, uint32_t len);
uint32_t crosstalk_game_last_state_serialize(const crosstalk_game_last_state_t *state, uint8_t *out, uint32_t cap, uint32_t *out_len);
uint32_t crosstalk_game_last_state_deserialize(crosstalk_game_last_state_t *state, const uint8_t *buff, uint32_t len);
uint32_t crosstalk_game_settings_keybind_default_fill(uint32_t *binds, uint32_t cunt);
uint32_t crosstalk_game_settings_keybind_chord_get(const ldg_term_key_event_t *irq, uint32_t *out);
uint8_t crosstalk_game_settings_keybind_family_is(uint32_t action);
uint8_t crosstalk_game_settings_keybind_match_is(const uint32_t *binds, uint32_t action, const ldg_term_key_event_t *irq);
uint32_t crosstalk_game_settings_keybind_capture_get(uint32_t action, const ldg_term_key_event_t *irq, uint32_t *out);
uint32_t crosstalk_game_settings_keybind_conflict_get(const uint32_t *binds, uint32_t action, uint32_t chord, uint32_t *out_owner);
uint32_t crosstalk_game_settings_keybind_fmt(uint32_t action, uint32_t chord, char *out, uint32_t cap);

#endif
