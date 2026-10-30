#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "game/hub.h"
#include "ui/cell.h"
#include "ui/theme.h"

#define CROSSTALK_GAME_HUB_TITLE "Friends"
#define CROSSTALK_GAME_HUB_STEAM_HD "Steam"
#define CROSSTALK_GAME_HUB_DISCORD_HD "Discord"
#define CROSSTALK_GAME_HUB_DM_TEXT "Msg"
#define CROSSTALK_GAME_HUB_INVITE_TEXT "Invite"
#define CROSSTALK_GAME_HUB_JOIN_TEXT "Join"
#define CROSSTALK_GAME_HUB_EMPTY_TEXT "nobody yet"
#define CROSSTALK_GAME_HUB_ROW_FMT "%c%u %-24.24s %.31s"
#define CROSSTALK_GAME_HUB_ACTION_FMT "[%.7s] [%.7s]"
#define CROSSTALK_GAME_HUB_ACTION_DM_FMT "[%.7s]"
#define CROSSTALK_GAME_HUB_LINE_CAP 160
#define CROSSTALK_GAME_HUB_ROW_STEP 1
#define CROSSTALK_GAME_HUB_CASE_SHIFT 32
#define CROSSTALK_GAME_HUB_ORIG_COL 0
#define CROSSTALK_GAME_HUB_SECTION_GAP 1
#define CROSSTALK_GAME_HUB_KEY_RHS_ACTION 'i'
#define CROSSTALK_GAME_HUB_KEY_STEAM 's'
#define CROSSTALK_GAME_HUB_KEY_STEAM_SHIFTED 'S'
#define CROSSTALK_GAME_HUB_KEY_DISCORD 'd'
#define CROSSTALK_GAME_HUB_KEY_DISCORD_SHIFTED 'D'
#define CROSSTALK_GAME_HUB_VIS_CAP (CROSSTALK_GAME_HUB_COL_CAP * CROSSTALK_GAME_HUB_SRC_CUNT)

uint32_t crosstalk_game_hub_init(crosstalk_game_hub_ctx_t *ctx)
{
    crosstalk_game_hub_ctx_t empty = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *ctx = empty;
    ctx->win.scroll_on = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_hub_open_set(crosstalk_game_hub_ctx_t *ctx, uint8_t open)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->win.open = (open != LDG_TRUTH_FALSE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_game_hub_open_is(const crosstalk_game_hub_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_TRUTH_FALSE; }

    return ctx->win.open;
}

uint32_t crosstalk_game_hub_rows_rst(crosstalk_game_hub_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->row_cunt = 0;

    return LDG_ERR_AOK;
}

static uint8_t crosstalk_game_hub_row_after_is(const crosstalk_game_hub_row_t *lhs, const crosstalk_game_hub_row_t *rhs)
{
    if (lhs->reach != rhs->reach) { return (lhs->reach > rhs->reach) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    for (uint32_t idx = 0; idx < CROSSTALK_GAME_HUB_NAME_CAP; idx++)
    {
        char lhs_ch = lhs->name[idx];
        char rhs_ch = rhs->name[idx];

        if (lhs_ch >= 'A' && lhs_ch <= 'Z') { lhs_ch = (char)(lhs_ch + CROSSTALK_GAME_HUB_CASE_SHIFT); }

        if (rhs_ch >= 'A' && rhs_ch <= 'Z') { rhs_ch = (char)(rhs_ch + CROSSTALK_GAME_HUB_CASE_SHIFT); }

        if (lhs_ch != rhs_ch) { return (lhs_ch > rhs_ch) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

        if (lhs_ch == LDG_STR_TERM) { break; }
    }

    return LDG_TRUTH_FALSE;
}

uint32_t crosstalk_game_hub_rows_sort(crosstalk_game_hub_ctx_t *ctx)
{
    crosstalk_game_hub_row_t tmp = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (uint32_t outer = 1; outer < ctx->row_cunt; outer++)
    {
        uint32_t inner = outer;

        tmp = ctx->row[outer];

        while (inner > 0 && crosstalk_game_hub_row_after_is(&ctx->row[inner - CROSSTALK_GAME_HUB_ROW_STEP], &tmp) == LDG_TRUTH_TRUE)
        {
            ctx->row[inner] = ctx->row[inner - CROSSTALK_GAME_HUB_ROW_STEP];
            inner--;
        }

        ctx->row[inner] = tmp;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_hub_row_add(crosstalk_game_hub_ctx_t *ctx, uint64_t user_id, const char *name, const char *status, uint8_t src, uint8_t reach, uint64_t lobby_id)
{
    crosstalk_game_hub_row_t *slot = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0 || status == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(src >= (uint8_t)CROSSTALK_GAME_HUB_SRC_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(reach >= (uint8_t)CROSSTALK_GAME_HUB_REACH_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->row_cunt >= CROSSTALK_GAME_HUB_ROW_CAP) { return LDG_ERR_FULL; }

    slot = &ctx->row[ctx->row_cunt];
    slot->user_id = user_id;
    slot->lobby_id = lobby_id;
    slot->src = src;
    slot->reach = reach;

    slot->name[0] = LDG_STR_TERM;
    slot->status[0] = LDG_STR_TERM;

    if (name[0] != LDG_STR_TERM)
    {
        ret = ldg_str_rbrcpy(slot->name, name, (uint64_t)sizeof(slot->name));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }
    }

    if (status[0] != LDG_STR_TERM)
    {
        ret = ldg_str_rbrcpy(slot->status, status, (uint64_t)sizeof(slot->status));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }
    }

    ctx->row_cunt = ctx->row_cunt + CROSSTALK_GAME_HUB_ROW_STEP;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_hub_reach_col_get(uint8_t reach)
{
    if (reach == (uint8_t)CROSSTALK_GAME_HUB_REACH_JOINABLE || reach == (uint8_t)CROSSTALK_GAME_HUB_REACH_IN_GAME) { return CROSSTALK_UI_THEME_ACCENT_RGBA; }

    if (reach == (uint8_t)CROSSTALK_GAME_HUB_REACH_OFFLINE) { return CROSSTALK_GAME_HUB_COL_OFFLINE; }

    return CROSSTALK_GAME_HUB_COL_ONLINE;
}

static uint32_t crosstalk_game_hub_vis_get(const crosstalk_game_hub_ctx_t *ctx, uint32_t *out_idx)
{
    uint32_t cunt = 0;

    for (uint32_t src = 0; src < (uint32_t)CROSSTALK_GAME_HUB_SRC_CUNT; src++)
    {
        uint8_t want = src == 0 ? (uint8_t)CROSSTALK_GAME_HUB_SRC_STEAM : (uint8_t)CROSSTALK_GAME_HUB_SRC_DISCORD;
        uint32_t shown = 0;

        for (uint32_t idx = 0; idx < ctx->row_cunt && shown < (uint32_t)CROSSTALK_GAME_HUB_COL_CAP; idx++)
        {
            if (ctx->row[idx].src != want) { continue; }

            out_idx[cunt] = idx;
            cunt++;
            shown++;
        }
    }

    return cunt;
}

static uint32_t crosstalk_game_hub_sel_pos_get(const crosstalk_game_hub_ctx_t *ctx, const uint32_t *vis, uint32_t vis_cunt)
{
    if (ctx->sel_on != LDG_TRUTH_TRUE) { return UINT32_MAX; }

    for (uint32_t at = 0; at < vis_cunt; at++) { if (ctx->row[vis[at]].user_id == ctx->sel_user_id && ctx->row[vis[at]].src == ctx->sel_src) { return at; } }

    return UINT32_MAX;
}

uint32_t crosstalk_game_hub_extent_get(const crosstalk_game_hub_ctx_t *ctx, uint32_t *out_row_cunt)
{
    uint32_t rows = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_row_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_row_cunt = 0;

    for (uint32_t src = 0; src < (uint32_t)CROSSTALK_GAME_HUB_SRC_CUNT; src++)
    {
        uint32_t shown = 0;

        for (uint32_t idx = 0; idx < ctx->row_cunt && shown < (uint32_t)CROSSTALK_GAME_HUB_COL_CAP; idx++) { if (ctx->row[idx].src == (uint8_t)src) { shown++; } }

        if (shown == 0) { shown = CROSSTALK_GAME_HUB_ROW_STEP; }

        rows = rows + CROSSTALK_GAME_HUB_ROW_STEP + shown;
    }

    *out_row_cunt = rows + CROSSTALK_GAME_HUB_SECTION_GAP;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_game_hub_row_draw(const crosstalk_game_hub_row_t *row_data, ldg_term_render_t *render, uint32_t row, uint32_t col_cunt, uint8_t sel_is, uint32_t pick)
{
    char tag = row_data->src == (uint8_t)CROSSTALK_GAME_HUB_SRC_STEAM ? CROSSTALK_GAME_HUB_KEY_STEAM_SHIFTED : CROSSTALK_GAME_HUB_KEY_DISCORD_SHIFTED;
    char line[CROSSTALK_GAME_HUB_LINE_CAP] = LDG_ARR_ZERO_INIT;
    char act[CROSSTALK_GAME_HUB_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t bg = sel_is == LDG_TRUTH_TRUE ? (uint32_t)CROSSTALK_UI_THEME_SEL_BG_RGBA : (uint32_t)CROSSTALK_UI_CELL_PANE_RGBA;
    ldg_term_render_color_t fg_col = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t bg_col = LDG_STRUCT_ZERO_INIT;
    uint64_t act_len = 0;
    uint32_t ret = 0;

    snprintf(line, sizeof(line), CROSSTALK_GAME_HUB_ROW_FMT, tag, pick, row_data->name, row_data->status);

    ret = crosstalk_ui_cell_color_get(crosstalk_game_hub_reach_col_get(row_data->reach), &fg_col);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get(bg, &bg_col); }

    if (ret == LDG_ERR_AOK && sel_is == LDG_TRUTH_TRUE) { ret = crosstalk_ui_cell_span_fill(render, (uint32_t)CROSSTALK_GAME_HUB_ORIG_COL, row, col_cunt, &fg_col, &bg_col); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_text_put(render, (uint32_t)CROSSTALK_GAME_HUB_ORIG_COL, row, line, (uint64_t)sizeof(line), &fg_col, &bg_col); }

    if (ret != LDG_ERR_AOK || sel_is != LDG_TRUTH_TRUE) { return ret; }

    if (row_data->reach == (uint8_t)CROSSTALK_GAME_HUB_REACH_OFFLINE) { snprintf(act, sizeof(act), CROSSTALK_GAME_HUB_ACTION_DM_FMT, CROSSTALK_GAME_HUB_DM_TEXT); }
    else { snprintf(act, sizeof(act), CROSSTALK_GAME_HUB_ACTION_FMT, CROSSTALK_GAME_HUB_DM_TEXT, row_data->reach == (uint8_t)CROSSTALK_GAME_HUB_REACH_JOINABLE ? CROSSTALK_GAME_HUB_JOIN_TEXT : CROSSTALK_GAME_HUB_INVITE_TEXT); }

    ret = ldg_str_len_get(act, (uint64_t)sizeof(act), &act_len);
    if (ret != LDG_ERR_AOK || act_len >= (uint64_t)col_cunt) { return ret; }

    ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_LIT_RGBA, &fg_col);
    if (ret != LDG_ERR_AOK) { return ret; }

    return crosstalk_ui_cell_text_put(render, col_cunt - (uint32_t)act_len, row, act, (uint64_t)sizeof(act), &fg_col, &bg_col);
}

static LDG_NOINLINE uint32_t crosstalk_game_hub_section_draw(crosstalk_game_hub_ctx_t *ctx, ldg_term_render_t *render, uint8_t src, const char *hd, uint32_t col_cunt, uint32_t *row)
{
    uint32_t shown = 0;
    uint32_t ret = 0;

    ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)CROSSTALK_GAME_HUB_ORIG_COL, *row, hd, (uint64_t)CROSSTALK_GAME_HUB_LINE_CAP, (uint32_t)CROSSTALK_UI_THEME_STATUS_RGBA, 0x0);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *row = *row + CROSSTALK_GAME_HUB_ROW_STEP;

    for (uint32_t idx = 0; idx < ctx->row_cunt && shown < (uint32_t)CROSSTALK_GAME_HUB_COL_CAP; idx++)
    {
        uint8_t sel_is = LDG_TRUTH_FALSE;

        if (ctx->row[idx].src != src) { continue; }

        if (ctx->sel_on == LDG_TRUTH_TRUE && ctx->row[idx].user_id == ctx->sel_user_id && ctx->row[idx].src == ctx->sel_src) { sel_is = LDG_TRUTH_TRUE; }

        ret = crosstalk_game_hub_row_draw(&ctx->row[idx], render, *row, col_cunt, sel_is, shown);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        *row = *row + CROSSTALK_GAME_HUB_ROW_STEP;
        shown++;
    }

    if (shown != 0) { return LDG_ERR_AOK; }

    ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)CROSSTALK_GAME_HUB_ORIG_COL, *row, CROSSTALK_GAME_HUB_EMPTY_TEXT, (uint64_t)CROSSTALK_GAME_HUB_LINE_CAP, (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, 0x0);
    *row = *row + CROSSTALK_GAME_HUB_ROW_STEP;

    return ret;
}

static uint32_t hub_draw_cb(uint64_t win_id, void *user, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    crosstalk_game_hub_ctx_t *ctx = (crosstalk_game_hub_ctx_t *)user;
    uint32_t row = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (win_id != (uint64_t)CROSSTALK_GAME_HUB_WINDOW_ID || col_cunt == 0 || row_cunt == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = crosstalk_game_hub_section_draw(ctx, render, (uint8_t)CROSSTALK_GAME_HUB_SRC_STEAM, CROSSTALK_GAME_HUB_STEAM_HD, col_cunt, &row);

    row = row + CROSSTALK_GAME_HUB_SECTION_GAP;

    if (ret == LDG_ERR_AOK) { ret = crosstalk_game_hub_section_draw(ctx, render, (uint8_t)CROSSTALK_GAME_HUB_SRC_DISCORD, CROSSTALK_GAME_HUB_DISCORD_HD, col_cunt, &row); }

    ctx->win.content_row_cunt = row;

    return ret;
}

static uint32_t crosstalk_game_hub_act(crosstalk_game_hub_ctx_t *ctx, const crosstalk_game_hub_row_t *row_data, uint8_t rhs_is)
{
    uint32_t ret = 0;

    if (rhs_is == LDG_TRUTH_TRUE && row_data->reach == (uint8_t)CROSSTALK_GAME_HUB_REACH_OFFLINE) { return LDG_ERR_AOK; }

    ctx->out_action = CROSSTALK_GAME_HUB_ACTION_DM;

    if (rhs_is == LDG_TRUTH_TRUE) { ctx->out_action = (row_data->reach == (uint8_t)CROSSTALK_GAME_HUB_REACH_JOINABLE) ? CROSSTALK_GAME_HUB_ACTION_JOIN : CROSSTALK_GAME_HUB_ACTION_INVITE; }

    ctx->out_user_id = row_data->user_id;
    ctx->out_lobby_id = row_data->lobby_id;
    ctx->out_src = row_data->src;

    ret = ldg_str_rbrcpy(ctx->out_name, row_data->name, (uint64_t)sizeof(ctx->out_name));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_game_hub_pick(crosstalk_game_hub_ctx_t *ctx, uint8_t src, uint32_t pick)
{
    uint32_t shown = 0;

    for (uint32_t idx = 0; idx < ctx->row_cunt && shown < (uint32_t)CROSSTALK_GAME_HUB_COL_CAP; idx++)
    {
        if (ctx->row[idx].src != src) { continue; }

        if (shown == pick)
        {
            ctx->sel_on = LDG_TRUTH_TRUE;
            ctx->sel_user_id = ctx->row[idx].user_id;
            ctx->sel_src = ctx->row[idx].src;

            return crosstalk_game_hub_act(ctx, &ctx->row[idx], LDG_TRUTH_FALSE);
        }

        shown++;
    }

    return LDG_ERR_AOK;
}

static uint32_t hub_key_isr(uint64_t win_id, void *user, const ldg_term_key_event_t *key, uint8_t *consumed)
{
    crosstalk_game_hub_ctx_t *ctx = (crosstalk_game_hub_ctx_t *)user;
    uint32_t vis[CROSSTALK_GAME_HUB_VIS_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t vis_cunt = 0;
    uint32_t pos = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || key == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    if (win_id != (uint64_t)CROSSTALK_GAME_HUB_WINDOW_ID) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (key->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE) { return LDG_ERR_AOK; }

    if ((key->mods & ((uint32_t)LDG_TERM_MOD_CTRL | (uint32_t)LDG_TERM_MOD_ALT | (uint32_t)LDG_TERM_MOD_SUPER)) != 0) { return LDG_ERR_AOK; }

    if (key->key == (uint8_t)LDG_TERM_KEY_NONE && key->codepoint == 0) { return LDG_ERR_AOK; }

    if (ctx->pick_on == LDG_TRUTH_TRUE)
    {
        ctx->pick_on = LDG_TRUTH_FALSE;

        if (key->codepoint >= (uint32_t)LDG_ASCII_DIGIT_ZERO && key->codepoint < (uint32_t)LDG_ASCII_DIGIT_ZERO + (uint32_t)LDG_BASE_DEC)
        {
            *consumed = LDG_TRUTH_TRUE;
            return crosstalk_game_hub_pick(ctx, ctx->pick_src, key->codepoint - (uint32_t)LDG_ASCII_DIGIT_ZERO);
        }
    }

    if (key->codepoint == (uint32_t)CROSSTALK_GAME_HUB_KEY_STEAM || key->codepoint == (uint32_t)CROSSTALK_GAME_HUB_KEY_STEAM_SHIFTED)
    {
        ctx->pick_on = LDG_TRUTH_TRUE;
        ctx->pick_src = (uint8_t)CROSSTALK_GAME_HUB_SRC_STEAM;
        *consumed = LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    if (key->codepoint == (uint32_t)CROSSTALK_GAME_HUB_KEY_DISCORD || key->codepoint == (uint32_t)CROSSTALK_GAME_HUB_KEY_DISCORD_SHIFTED)
    {
        ctx->pick_on = LDG_TRUTH_TRUE;
        ctx->pick_src = (uint8_t)CROSSTALK_GAME_HUB_SRC_DISCORD;
        *consumed = LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    vis_cunt = crosstalk_game_hub_vis_get(ctx, vis);
    if (vis_cunt == 0) { return LDG_ERR_AOK; }

    pos = crosstalk_game_hub_sel_pos_get(ctx, vis, vis_cunt);

    if (key->key == (uint8_t)LDG_TERM_KEY_UP || key->key == (uint8_t)LDG_TERM_KEY_DOWN)
    {
        if (pos == UINT32_MAX) { pos = 0; }
        else if (key->key == (uint8_t)LDG_TERM_KEY_UP) { if (pos != 0) { pos = pos - CROSSTALK_GAME_HUB_ROW_STEP; } }
        else if (pos + CROSSTALK_GAME_HUB_ROW_STEP < vis_cunt) { pos = pos + CROSSTALK_GAME_HUB_ROW_STEP; }

        ctx->sel_on = LDG_TRUTH_TRUE;
        ctx->sel_user_id = ctx->row[vis[pos]].user_id;
        ctx->sel_src = ctx->row[vis[pos]].src;
        *consumed = LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    if (pos == UINT32_MAX) { return LDG_ERR_AOK; }

    if (key->key == (uint8_t)LDG_TERM_KEY_ENTER)
    {
        *consumed = LDG_TRUTH_TRUE;
        return crosstalk_game_hub_act(ctx, &ctx->row[vis[pos]], LDG_TRUTH_FALSE);
    }

    if (key->text_cunt != 0 && key->text[0] == (uint32_t)CROSSTALK_GAME_HUB_KEY_RHS_ACTION)
    {
        *consumed = LDG_TRUTH_TRUE;
        return crosstalk_game_hub_act(ctx, &ctx->row[vis[pos]], LDG_TRUTH_TRUE);
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_hub_win_reg(crosstalk_game_hub_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot)
{
    if (LDG_UNLIKELY(ctx == 0x0 || win_nav == 0x0 || out_slot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_ui_window_reg(win_nav, (uint64_t)CROSSTALK_GAME_HUB_WINDOW_ID, CROSSTALK_GAME_HUB_TITLE, &ctx->win, hub_draw_cb, hub_key_isr, ctx, out_slot);
}

uint32_t crosstalk_game_hub_out_get(crosstalk_game_hub_ctx_t *ctx, crosstalk_game_hub_action_t *out_action, uint64_t *out_user_id, uint64_t *out_lobby_id, uint8_t *out_src, char *out_name, uint32_t out_name_cap)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_action == 0x0 || out_user_id == 0x0 || out_lobby_id == 0x0 || out_src == 0x0 || out_name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(out_name_cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out_action = ctx->out_action;
    *out_user_id = ctx->out_user_id;
    *out_lobby_id = ctx->out_lobby_id;
    *out_src = ctx->out_src;

    out_name[0] = LDG_STR_TERM;

    if (ctx->out_name[0] != LDG_STR_TERM) { ret = ldg_str_rbrcpy(out_name, ctx->out_name, (uint64_t)out_name_cap); }

    ctx->out_action = CROSSTALK_GAME_HUB_ACTION_NONE;

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    return LDG_ERR_AOK;
}
