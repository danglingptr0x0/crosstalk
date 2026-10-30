#include "game/settings_menu.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "ui/cell.h"
#include "ui/theme.h"

#define SETTINGS_MENU_STEP 1
#define SETTINGS_MENU_ROW_FIRST 0
#define SETTINGS_MENU_TOGGLE_STEP_CUNT 2
#define SETTINGS_MENU_MUSIC_STEP_CUNT 3
#define SETTINGS_MENU_VOL_MIN 0.0
#define SETTINGS_MENU_VOL_MAX 200.0
#define SETTINGS_MENU_VOL_STEP_FRAC 0.05
#define SETTINGS_MENU_LINE_CAP 320
#define SETTINGS_MENU_LABEL_FMT "%-24.24s %.31s"
#define SETTINGS_MENU_VOL_FMT "%.0f"
#define SETTINGS_MENU_VAL_CAP 32
#define SETTINGS_MENU_SECTION_COL 0x9FCF40FFU
#define SETTINGS_MENU_TITLE "Settings"
#define SETTINGS_MENU_TIP_GAP 1
#define SETTINGS_MENU_TIP_ROW_MAX 4
#define SETTINGS_MENU_BOX_COL_CUNT 72
#define SETTINGS_MENU_BOX_PAD 1
#define SETTINGS_MENU_BOX_SIDES 2
#define SETTINGS_MENU_CENTER_DIV 2
#define SETTINGS_MENU_SECTION_ROWS 2
#define SETTINGS_MENU_TITLE_ROWS 1

typedef enum settings_menu_item
{
    SETTINGS_MENU_ITEM_FAB_SAFETY = 0,
    SETTINGS_MENU_ITEM_FAB_INTERLOCK,
    SETTINGS_MENU_ITEM_MUSIC_SRC,
    SETTINGS_MENU_ITEM_MUSIC_SHOW,
    SETTINGS_MENU_ITEM_DISCORD,
    SETTINGS_MENU_ITEM_PRESENCE,
    SETTINGS_MENU_ITEM_VOICE,
    SETTINGS_MENU_ITEM_LINK,
    SETTINGS_MENU_ITEM_MIC_VOL,
    SETTINGS_MENU_ITEM_VOICE_VOL,
    SETTINGS_MENU_ITEM_FOCUS_PAUSE,
    SETTINGS_MENU_ITEM_CUNT
} settings_menu_item_t;

static const char* settings_menu_section_get(uint32_t item)
{
    if (item == (uint32_t)SETTINGS_MENU_ITEM_FAB_SAFETY) { return "FABRICATION"; }

    if (item == (uint32_t)SETTINGS_MENU_ITEM_MUSIC_SRC) { return "AUDIO"; }

    if (item == (uint32_t)SETTINGS_MENU_ITEM_DISCORD) { return "DISCORD"; }

    if (item == (uint32_t)SETTINGS_MENU_ITEM_FOCUS_PAUSE) { return "GAME"; }

    return 0x0;
}

static const char* settings_menu_label_get(uint32_t item)
{
    if (item == (uint32_t)SETTINGS_MENU_ITEM_FAB_SAFETY) { return "Fabrication"; }

    if (item == (uint32_t)SETTINGS_MENU_ITEM_FAB_INTERLOCK) { return ""; }

    if (item == (uint32_t)SETTINGS_MENU_ITEM_MUSIC_SRC) { return "Music source"; }

    if (item == (uint32_t)SETTINGS_MENU_ITEM_MUSIC_SHOW) { return "Show music"; }

    if (item == (uint32_t)SETTINGS_MENU_ITEM_DISCORD) { return "Discord"; }

    if (item == (uint32_t)SETTINGS_MENU_ITEM_PRESENCE) { return "Discord presence"; }

    if (item == (uint32_t)SETTINGS_MENU_ITEM_VOICE) { return "Voice chat"; }

    if (item == (uint32_t)SETTINGS_MENU_ITEM_LINK) { return "Discord account"; }

    if (item == (uint32_t)SETTINGS_MENU_ITEM_MIC_VOL) { return "Mic volume"; }

    if (item == (uint32_t)SETTINGS_MENU_ITEM_VOICE_VOL) { return "Voice volume"; }

    return "Pause when unfocused";
}

static const char* settings_menu_tip_get(uint32_t item)
{
    if (item == (uint32_t)SETTINGS_MENU_ITEM_FAB_SAFETY) { return "Fabrication safety; rotate to Off as the first step toward custom fabrication"; }

    if (item == (uint32_t)SETTINGS_MENU_ITEM_FAB_INTERLOCK) { return "Fabrication interlock; while closed, boards stay exportable and manufacturable from library components alone; open it (safety off first) to allow custom components and edits to library ones"; }

    if (item == (uint32_t)SETTINGS_MENU_ITEM_MUSIC_SRC) { return "Where the playback panel gets its music; Steam controls the Steam client player, Native plays the game soundtrack, System controls whatever the machine is playing"; }

    if (item == (uint32_t)SETTINGS_MENU_ITEM_MUSIC_SHOW) { return "Adds what you are listening to onto your Discord presence, visible to anyone who can see it"; }

    if (item == (uint32_t)SETTINGS_MENU_ITEM_DISCORD) { return "Discord integration master switch"; }

    if (item == (uint32_t)SETTINGS_MENU_ITEM_PRESENCE) { return "Shows your city and status on your Discord profile"; }

    if (item == (uint32_t)SETTINGS_MENU_ITEM_VOICE) { return "In-game voice chat over Discord"; }

    if (item == (uint32_t)SETTINGS_MENU_ITEM_LINK) { return "Links or unlinks your Discord account"; }

    if (item == (uint32_t)SETTINGS_MENU_ITEM_MIC_VOL) { return "Voice capture level sent to others"; }

    if (item == (uint32_t)SETTINGS_MENU_ITEM_VOICE_VOL) { return "Voice playback level heard from others"; }

    return "Shows the darkened pause screen when the game window loses focus; the simulation itself keeps running either way, so leave this Off for recording or a second monitor";
}

static void settings_menu_val_fmt(const crosstalk_game_settings_t *settings, uint8_t discord_linked, uint32_t item, char *out, uint64_t cap)
{
    const char *off_on[SETTINGS_MENU_TOGGLE_STEP_CUNT] = { "Off", "On" };
    const char *safety[SETTINGS_MENU_TOGGLE_STEP_CUNT] = { "On", "Off" };
    const char *interlock[SETTINGS_MENU_TOGGLE_STEP_CUNT] = { "Shut", "Open" };
    const char *link[SETTINGS_MENU_TOGGLE_STEP_CUNT] = { "Unlinked", "Linked" };
    const char *music[SETTINGS_MENU_MUSIC_STEP_CUNT] = { "Steam", "Native", "System" };
    const char *text = 0x0;

    if (item == (uint32_t)SETTINGS_MENU_ITEM_MIC_VOL) { snprintf(out, cap, SETTINGS_MENU_VOL_FMT, settings->discord_vol_in); return; }

    if (item == (uint32_t)SETTINGS_MENU_ITEM_VOICE_VOL) { snprintf(out, cap, SETTINGS_MENU_VOL_FMT, settings->discord_vol_out); return; }

    if (item == (uint32_t)SETTINGS_MENU_ITEM_FAB_SAFETY) { text = safety[settings->fab_safety_off != LDG_TRUTH_FALSE ? SETTINGS_MENU_STEP : SETTINGS_MENU_ROW_FIRST]; }
    else if (item == (uint32_t)SETTINGS_MENU_ITEM_FAB_INTERLOCK) { text = interlock[settings->fab_interlock_off != LDG_TRUTH_FALSE ? SETTINGS_MENU_STEP : SETTINGS_MENU_ROW_FIRST]; }
    else if (item == (uint32_t)SETTINGS_MENU_ITEM_MUSIC_SRC) { text = music[settings->music_src < (uint8_t)SETTINGS_MENU_MUSIC_STEP_CUNT ? settings->music_src : SETTINGS_MENU_ROW_FIRST]; }
    else if (item == (uint32_t)SETTINGS_MENU_ITEM_LINK) { text = link[discord_linked != LDG_TRUTH_FALSE ? SETTINGS_MENU_STEP : SETTINGS_MENU_ROW_FIRST]; }
    else if (item == (uint32_t)SETTINGS_MENU_ITEM_MUSIC_SHOW) { text = off_on[settings->music_presence_on != LDG_TRUTH_FALSE ? SETTINGS_MENU_STEP : SETTINGS_MENU_ROW_FIRST]; }
    else if (item == (uint32_t)SETTINGS_MENU_ITEM_DISCORD) { text = off_on[settings->discord_on != LDG_TRUTH_FALSE ? SETTINGS_MENU_STEP : SETTINGS_MENU_ROW_FIRST]; }
    else if (item == (uint32_t)SETTINGS_MENU_ITEM_PRESENCE) { text = off_on[settings->discord_presence_on != LDG_TRUTH_FALSE ? SETTINGS_MENU_STEP : SETTINGS_MENU_ROW_FIRST]; }
    else if (item == (uint32_t)SETTINGS_MENU_ITEM_VOICE) { text = off_on[settings->discord_voice_on != LDG_TRUTH_FALSE ? SETTINGS_MENU_STEP : SETTINGS_MENU_ROW_FIRST]; }
    else { text = off_on[settings->focus_pause_on != LDG_TRUTH_FALSE ? SETTINGS_MENU_STEP : SETTINGS_MENU_ROW_FIRST]; }

    snprintf(out, cap, "%.31s", text);
}

uint32_t crosstalk_game_settings_menu_init(crosstalk_game_settings_menu_ctx_t *ctx)
{
    crosstalk_game_settings_menu_ctx_t empty = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *ctx = empty;
    ctx->link_action = CROSSTALK_GAME_SETTINGS_LINK_NONE;

    return LDG_ERR_AOK;
}

static uint32_t settings_menu_row_draw(const crosstalk_game_settings_menu_ctx_t *ctx, const crosstalk_game_settings_t *settings, uint8_t discord_linked, ldg_term_render_t *render, uint32_t item, uint32_t col, uint32_t row, uint32_t width)
{
    char val[SETTINGS_MENU_VAL_CAP] = LDG_ARR_ZERO_INIT;
    char line[SETTINGS_MENU_LINE_CAP] = LDG_ARR_ZERO_INIT;
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t bg = LDG_STRUCT_ZERO_INIT;
    uint8_t sel_is = ctx->sel == item ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    settings_menu_val_fmt(settings, discord_linked, item, val, (uint64_t)sizeof(val));

    if (snprintf(line, sizeof(line), SETTINGS_MENU_LABEL_FMT, settings_menu_label_get(item), val) < 0) { return LDG_ERR_STR_FMT; }

    if (sel_is != LDG_TRUTH_TRUE) { return crosstalk_ui_cell_rgba_put(render, col, row, line, (uint64_t)sizeof(line), (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, 0x0); }

    ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_LIT_RGBA, &fg);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_SEL_BG_RGBA, &bg); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_span_fill(render, col, row, width, &fg, &bg); }

    if (ret != LDG_ERR_AOK) { return ret; }

    return crosstalk_ui_cell_text_put(render, col, row, line, (uint64_t)sizeof(line), &fg, &bg);
}

static uint32_t settings_menu_tip_draw(ldg_term_render_t *render, const char *tip, uint32_t col, uint32_t row, uint32_t width)
{
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t bg = LDG_STRUCT_ZERO_INIT;
    uint64_t len = (uint64_t)strlen(tip);
    uint64_t at = 0;
    uint64_t span = 0;
    uint64_t cut = 0;
    uint32_t line = 0;
    uint32_t ret = 0;

    ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, &fg);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_CELL_PANE_RGBA, &bg); }

    if (ret != LDG_ERR_AOK) { return ret; }

    while (at < len && line < (uint32_t)SETTINGS_MENU_TIP_ROW_MAX)
    {
        while (at < len && tip[at] == (char)LDG_ASCII_SPACE) { at = at + SETTINGS_MENU_STEP; }

        if (at >= len) { break; }

        span = len - at;

        if (span > (uint64_t)width)
        {
            cut = (uint64_t)width;
            while (cut > 0 && tip[at + cut] != (char)LDG_ASCII_SPACE) { cut = cut - SETTINGS_MENU_STEP; }

            span = cut > 0 ? cut : (uint64_t)width;
        }

        ret = ldg_term_render_text_draw(render, col, row + line, tip + at, span, &fg, &bg);
        if (ret != LDG_ERR_AOK) { return ret; }

        at = at + span;
        line = line + SETTINGS_MENU_STEP;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_settings_menu_draw(const crosstalk_game_settings_menu_ctx_t *ctx, const crosstalk_game_settings_t *settings, uint8_t discord_linked, ldg_term_render_t *render, const ldg_term_rect_t *rect)
{
    ldg_term_render_color_t box_fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t box_bg = LDG_STRUCT_ZERO_INIT;
    uint32_t box_w = (uint32_t)SETTINGS_MENU_BOX_COL_CUNT;
    uint32_t box_h = (uint32_t)SETTINGS_MENU_TITLE_ROWS + (uint32_t)SETTINGS_MENU_ITEM_CUNT + (uint32_t)SETTINGS_MENU_TIP_GAP + (uint32_t)SETTINGS_MENU_TIP_ROW_MAX + (uint32_t)SETTINGS_MENU_BOX_PAD * (uint32_t)SETTINGS_MENU_BOX_SIDES;
    uint32_t box_col = 0;
    uint32_t box_row = 0;
    uint32_t inner_col = 0;
    uint32_t inner_w = 0;
    uint32_t row = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || settings == 0x0 || render == 0x0 || rect == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (rect->col_cunt == 0 || rect->row_cunt == 0) { return LDG_ERR_AOK; }

    for (uint32_t item = 0; item < (uint32_t)SETTINGS_MENU_ITEM_CUNT; item++) { if (settings_menu_section_get(item) != 0x0) { box_h = box_h + (uint32_t)SETTINGS_MENU_SECTION_ROWS; } }

    if (box_w > rect->col_cunt) { box_w = rect->col_cunt; }

    if (box_h > rect->row_cunt) { box_h = rect->row_cunt; }

    if (box_w <= (uint32_t)SETTINGS_MENU_BOX_PAD * (uint32_t)SETTINGS_MENU_BOX_SIDES) { return LDG_ERR_AOK; }

    box_col = rect->col + (rect->col_cunt - box_w) / (uint32_t)SETTINGS_MENU_CENTER_DIV;
    box_row = rect->row + (rect->row_cunt - box_h) / (uint32_t)SETTINGS_MENU_CENTER_DIV;
    inner_col = box_col + (uint32_t)SETTINGS_MENU_BOX_PAD;
    inner_w = box_w - (uint32_t)SETTINGS_MENU_BOX_PAD * (uint32_t)SETTINGS_MENU_BOX_SIDES;

    ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, &box_fg);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_CELL_PANE_RGBA, &box_bg); }

    for (row = box_row; ret == LDG_ERR_AOK && row < box_row + box_h; row = row + SETTINGS_MENU_STEP) { ret = crosstalk_ui_cell_span_fill(render, box_col, row, box_w, &box_fg, &box_bg); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    row = box_row + (uint32_t)SETTINGS_MENU_BOX_PAD;

    ret = crosstalk_ui_cell_rgba_put(render, inner_col, row, SETTINGS_MENU_TITLE, (uint64_t)SETTINGS_MENU_LINE_CAP, (uint32_t)CROSSTALK_UI_THEME_LIT_RGBA, 0x0);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    row = row + SETTINGS_MENU_STEP;

    for (uint32_t item = 0; item < (uint32_t)SETTINGS_MENU_ITEM_CUNT; item++)
    {
        const char *section = settings_menu_section_get(item);

        if (section != 0x0)
        {
            row = row + SETTINGS_MENU_STEP;

            ret = crosstalk_ui_cell_rgba_put(render, inner_col, row, section, (uint64_t)SETTINGS_MENU_LINE_CAP, (uint32_t)SETTINGS_MENU_SECTION_COL, 0x0);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            row = row + SETTINGS_MENU_STEP;
        }

        ret = settings_menu_row_draw(ctx, settings, discord_linked, render, item, inner_col, row, inner_w);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        row = row + SETTINGS_MENU_STEP;
    }

    if (ctx->sel >= (uint32_t)SETTINGS_MENU_ITEM_CUNT) { return LDG_ERR_AOK; }

    return settings_menu_tip_draw(render, settings_menu_tip_get(ctx->sel), inner_col, row + SETTINGS_MENU_TIP_GAP, inner_w);
}

static void settings_menu_vol_step(double *val, uint8_t back)
{
    double step = (SETTINGS_MENU_VOL_MAX - SETTINGS_MENU_VOL_MIN) * SETTINGS_MENU_VOL_STEP_FRAC;

    *val = back == LDG_TRUTH_TRUE ? *val - step : *val + step;

    if (*val < SETTINGS_MENU_VOL_MIN) { *val = SETTINGS_MENU_VOL_MIN; }

    if (*val > SETTINGS_MENU_VOL_MAX) { *val = SETTINGS_MENU_VOL_MAX; }
}

static void settings_menu_flip(uint8_t *val)
{
    *val = *val != LDG_TRUTH_FALSE ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE;
}

static void settings_menu_item_step(crosstalk_game_settings_menu_ctx_t *ctx, crosstalk_game_settings_t *settings, uint8_t discord_linked, uint8_t back)
{
    uint32_t item = ctx->sel;

    if (item == (uint32_t)SETTINGS_MENU_ITEM_MIC_VOL) { settings_menu_vol_step(&settings->discord_vol_in, back); }
    else if (item == (uint32_t)SETTINGS_MENU_ITEM_VOICE_VOL) { settings_menu_vol_step(&settings->discord_vol_out, back); }
    else if (item == (uint32_t)SETTINGS_MENU_ITEM_MUSIC_SRC) { if (back == LDG_TRUTH_TRUE) { settings->music_src = settings->music_src == 0 ? (uint8_t)(SETTINGS_MENU_MUSIC_STEP_CUNT - SETTINGS_MENU_STEP) : (uint8_t)(settings->music_src - SETTINGS_MENU_STEP); }
        else { settings->music_src = (uint8_t)((settings->music_src + SETTINGS_MENU_STEP) % SETTINGS_MENU_MUSIC_STEP_CUNT); } }
    else if (item == (uint32_t)SETTINGS_MENU_ITEM_FAB_SAFETY)
    {
        settings_menu_flip(&settings->fab_safety_off);

        if (settings->fab_safety_off == LDG_TRUTH_FALSE) { settings->fab_interlock_off = LDG_TRUTH_FALSE; }
    }
    else if (item == (uint32_t)SETTINGS_MENU_ITEM_FAB_INTERLOCK) { if (settings->fab_safety_off != LDG_TRUTH_FALSE) { settings_menu_flip(&settings->fab_interlock_off); } }
    else if (item == (uint32_t)SETTINGS_MENU_ITEM_LINK) { ctx->link_action = discord_linked != LDG_TRUTH_FALSE ? CROSSTALK_GAME_SETTINGS_LINK_UNDO : CROSSTALK_GAME_SETTINGS_LINK_DO; }
    else if (item == (uint32_t)SETTINGS_MENU_ITEM_MUSIC_SHOW) { settings_menu_flip(&settings->music_presence_on); }
    else if (item == (uint32_t)SETTINGS_MENU_ITEM_DISCORD) { settings_menu_flip(&settings->discord_on); }
    else if (item == (uint32_t)SETTINGS_MENU_ITEM_PRESENCE) { settings_menu_flip(&settings->discord_presence_on); }
    else if (item == (uint32_t)SETTINGS_MENU_ITEM_VOICE) { settings_menu_flip(&settings->discord_voice_on); }
    else { settings_menu_flip(&settings->focus_pause_on); }

    ctx->changed = LDG_TRUTH_TRUE;
}

uint32_t crosstalk_game_settings_menu_key_isr(crosstalk_game_settings_menu_ctx_t *ctx, crosstalk_game_settings_t *settings, uint8_t discord_linked, const ldg_term_key_event_t *key, uint8_t *consumed)
{
    if (LDG_UNLIKELY(ctx == 0x0 || settings == 0x0 || key == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    if (key->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE) { return LDG_ERR_AOK; }

    if ((key->mods & ((uint32_t)LDG_TERM_MOD_CTRL | (uint32_t)LDG_TERM_MOD_ALT | (uint32_t)LDG_TERM_MOD_SUPER)) != 0) { return LDG_ERR_AOK; }

    if (ctx->sel >= (uint32_t)SETTINGS_MENU_ITEM_CUNT) { ctx->sel = (uint32_t)SETTINGS_MENU_ROW_FIRST; }

    *consumed = LDG_TRUTH_TRUE;

    if (key->key == (uint8_t)LDG_TERM_KEY_UP)
    {
        if (ctx->sel != 0) { ctx->sel = ctx->sel - SETTINGS_MENU_STEP; }

        return LDG_ERR_AOK;
    }

    if (key->key == (uint8_t)LDG_TERM_KEY_DOWN)
    {
        if (ctx->sel + SETTINGS_MENU_STEP < (uint32_t)SETTINGS_MENU_ITEM_CUNT) { ctx->sel = ctx->sel + SETTINGS_MENU_STEP; }

        return LDG_ERR_AOK;
    }

    if (key->key == (uint8_t)LDG_TERM_KEY_LHS) { settings_menu_item_step(ctx, settings, discord_linked, LDG_TRUTH_TRUE); return LDG_ERR_AOK; }

    if (key->key == (uint8_t)LDG_TERM_KEY_RHS || key->key == (uint8_t)LDG_TERM_KEY_ENTER) { settings_menu_item_step(ctx, settings, discord_linked, LDG_TRUTH_FALSE); return LDG_ERR_AOK; }

    *consumed = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_settings_menu_out_get(crosstalk_game_settings_menu_ctx_t *ctx, uint8_t *out_link_action, uint8_t *out_changed)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_link_action == 0x0 || out_changed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_link_action = ctx->link_action;
    *out_changed = ctx->changed;
    ctx->link_action = CROSSTALK_GAME_SETTINGS_LINK_NONE;
    ctx->changed = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}
