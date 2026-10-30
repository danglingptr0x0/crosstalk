#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "core/log.h"
#include "game/chat.h"
#include "game/hub.h"
#include "ui/cell.h"
#include "ui/theme.h"

#define CROSSTALK_GAME_CHAT_TITLE "Chat"
#define CROSSTALK_GAME_CHAT_TITLE_FMT "%.15s: %.15s"
#define CROSSTALK_GAME_CHAT_LINE_CAP 320
#define CROSSTALK_GAME_CHAT_WRAP_CAP 16
#define CROSSTALK_GAME_CHAT_STEP 1
#define CROSSTALK_GAME_CHAT_ORIG_COL 0
#define CROSSTALK_GAME_CHAT_ROW_GROUP 0
#define CROSSTALK_GAME_CHAT_ROW_CONV 1
#define CROSSTALK_GAME_CHAT_ROW_PEER 2
#define CROSSTALK_GAME_CHAT_TAB_GAP 1
#define CROSSTALK_GAME_CHAT_UNSEEN_MARK " *"
#define CROSSTALK_GAME_CHAT_TAB_FMT "[%.15s%.3s]"
#define CROSSTALK_GAME_CHAT_EMPTY_TEXT "nothing here yet"
#define CROSSTALK_GAME_CHAT_INPUT_MARK "> "
#define CROSSTALK_GAME_CHAT_INPUT_MARK_W 2
#define CROSSTALK_GAME_CHAT_CURSOR_W 1
#define CROSSTALK_GAME_CHAT_MSG_FMT "%.63s: %.239s"
#define CROSSTALK_GAME_CHAT_SEL_FIRST 0

static const char* crosstalk_game_chat_group_name_get(uint8_t group)
{
    if (group == (uint8_t)CROSSTALK_GAME_CHAT_GROUP_SESH) { return "session"; }

    if (group == (uint8_t)CROSSTALK_GAME_CHAT_GROUP_STEAM) { return "steam"; }

    return "discord";
}

static uint8_t crosstalk_game_chat_group_live_is(uint8_t group, uint8_t sesh_live, uint8_t discord_live)
{
    if (group == (uint8_t)CROSSTALK_GAME_CHAT_GROUP_SESH) { return sesh_live; }

    if (group == (uint8_t)CROSSTALK_GAME_CHAT_GROUP_DISCORD) { return discord_live; }

    return LDG_TRUTH_TRUE;
}

static uint8_t crosstalk_game_chat_any_unseen_is(const crosstalk_game_chat_ctx_t *ctx)
{
    for (uint32_t idx = 0; idx < ctx->conv_cunt; idx++) { if (ctx->conv[idx].unseen != LDG_TRUTH_FALSE) { return LDG_TRUTH_TRUE; } }

    return LDG_TRUTH_FALSE;
}

static uint8_t crosstalk_game_chat_group_unseen_is(const crosstalk_game_chat_ctx_t *ctx, uint8_t group)
{
    for (uint32_t idx = 0; idx < ctx->conv_cunt; idx++)
    {
        if (ctx->conv[idx].group != group) { continue; }

        if (ctx->conv[idx].unseen != LDG_TRUTH_FALSE) { return LDG_TRUTH_TRUE; }
    }

    return LDG_TRUTH_FALSE;
}

uint32_t crosstalk_game_chat_init(crosstalk_game_chat_ctx_t *ctx)
{
    crosstalk_game_chat_ctx_t empty = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *ctx = empty;
    ctx->win.scroll_on = LDG_TRUTH_TRUE;
    ctx->win.open = LDG_TRUTH_FALSE;
    ctx->line_seen = UINT32_MAX;
    ctx->group_seen = UINT8_MAX;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_chat_open_set(crosstalk_game_chat_ctx_t *ctx, uint8_t open)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->win.open = (open != LDG_TRUTH_FALSE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_chat_rst(crosstalk_game_chat_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->conv_cunt = 0;
    ctx->unseen = LDG_TRUTH_FALSE;
    ctx->draft[0] = LDG_STR_TERM;

    for (uint32_t idx = 0; idx < (uint32_t)CROSSTALK_GAME_CHAT_GROUP_CUNT; idx++) { ctx->conv_sel[idx] = 0; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_chat_conv_get(crosstalk_game_chat_ctx_t *ctx, uint8_t group, uint64_t peer_id, const char *name, uint32_t *out_idx)
{
    crosstalk_game_chat_conv_t *slot = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(group >= (uint8_t)CROSSTALK_GAME_CHAT_GROUP_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out_idx = 0;

    for (uint32_t idx = 0; idx < ctx->conv_cunt; idx++)
    {
        if (ctx->conv[idx].group != group || ctx->conv[idx].peer_id != peer_id) { continue; }

        *out_idx = idx;

        if (name != 0x0 && name[0] != LDG_STR_TERM) { ret = ldg_str_rbrcpy(ctx->conv[idx].name, name, (uint64_t)sizeof(ctx->conv[idx].name)); }

        return (ret == LDG_ERR_STR_TRUNC) ? LDG_ERR_AOK : ret;
    }

    if (ctx->conv_cunt >= (uint32_t)CROSSTALK_GAME_CHAT_CONV_CAP) { return LDG_ERR_FULL; }

    slot = &ctx->conv[ctx->conv_cunt];
    slot->group = group;
    slot->peer_id = peer_id;
    slot->hd = 0;
    slot->cunt = 0;
    slot->unseen = LDG_TRUTH_FALSE;
    slot->live = LDG_TRUTH_TRUE;
    slot->reach = (uint8_t)CROSSTALK_GAME_HUB_REACH_ONLINE;
    slot->name[0] = LDG_STR_TERM;

    if (name != 0x0 && name[0] != LDG_STR_TERM)
    {
        ret = ldg_str_rbrcpy(slot->name, name, (uint64_t)sizeof(slot->name));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }
    }

    *out_idx = ctx->conv_cunt;
    ctx->conv_cunt = ctx->conv_cunt + CROSSTALK_GAME_CHAT_STEP;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_chat_conv_sel(crosstalk_game_chat_ctx_t *ctx, uint8_t group, uint64_t peer_id, const char *name)
{
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_game_chat_conv_get(ctx, group, peer_id, name, &idx);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->group = group;
    ctx->conv_sel[group] = idx;
    ctx->conv[idx].unseen = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_chat_conv_reach_set(crosstalk_game_chat_ctx_t *ctx, uint8_t group, uint64_t peer_id, uint8_t reach)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(group >= (uint8_t)CROSSTALK_GAME_CHAT_GROUP_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(reach >= (uint8_t)CROSSTALK_GAME_HUB_REACH_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (uint32_t idx = 0; idx < ctx->conv_cunt; idx++)
    {
        if (ctx->conv[idx].group != group || ctx->conv[idx].peer_id != peer_id) { continue; }

        ctx->conv[idx].reach = reach;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

uint64_t crosstalk_game_chat_peer_get(const crosstalk_game_chat_ctx_t *ctx, uint8_t group)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return CROSSTALK_GAME_CHAT_PEER_NONE; }

    if (LDG_UNLIKELY(group >= (uint8_t)CROSSTALK_GAME_CHAT_GROUP_CUNT)) { return CROSSTALK_GAME_CHAT_PEER_NONE; }

    idx = ctx->conv_sel[group];
    if (idx >= ctx->conv_cunt) { return CROSSTALK_GAME_CHAT_PEER_NONE; }

    if (ctx->conv[idx].group != group) { return CROSSTALK_GAME_CHAT_PEER_NONE; }

    return ctx->conv[idx].peer_id;
}

uint32_t crosstalk_game_chat_push(crosstalk_game_chat_ctx_t *ctx, uint8_t group, uint64_t peer_id, const char *conv_name, const char *name, const char *text)
{
    crosstalk_game_chat_conv_t *conv = 0x0;
    uint32_t conv_idx = 0;
    uint32_t slot = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_game_chat_conv_get(ctx, group, peer_id, conv_name, &conv_idx);
    if (ret != LDG_ERR_AOK) { return ret; }

    conv = &ctx->conv[conv_idx];
    slot = (conv->hd + conv->cunt) % CROSSTALK_GAME_CHAT_HISTORY_CAP;

    if (conv->cunt < CROSSTALK_GAME_CHAT_HISTORY_CAP) { conv->cunt = conv->cunt + CROSSTALK_GAME_CHAT_STEP; }
    else { conv->hd = (conv->hd + CROSSTALK_GAME_CHAT_STEP) % CROSSTALK_GAME_CHAT_HISTORY_CAP; }

    ret = ldg_str_rbrcpy(conv->msg[slot].name, name, (uint64_t)sizeof(conv->msg[slot].name));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    ret = ldg_str_rbrcpy(conv->msg[slot].text, text, (uint64_t)sizeof(conv->msg[slot].text));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    if (ctx->win.open == LDG_TRUTH_FALSE || ctx->group != group || ctx->conv_sel[group] != conv_idx)
    {
        conv->unseen = LDG_TRUTH_TRUE;
        ctx->unseen = LDG_TRUTH_TRUE;
    }

    if (ctx->win.open == LDG_TRUTH_TRUE && ctx->draft[0] == LDG_STR_TERM && ctx->group == group) { ctx->conv_sel[group] = conv_idx; }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_game_chat_wrap(const char *text, uint32_t max_w, uint32_t *seg_start, uint32_t *seg_len, uint32_t *out_cunt)
{
    uint32_t total = (uint32_t)strlen(text);
    uint32_t pos = 0;
    uint32_t cunt = 0;

    *out_cunt = 0;

    if (max_w == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    while (pos < total && cunt < (uint32_t)CROSSTALK_GAME_CHAT_WRAP_CAP)
    {
        uint32_t take = total - pos < max_w ? total - pos : max_w;
        uint32_t brk = 0;

        if (pos + take < total)
        {
            for (brk = take; brk > 0; brk--) { if (text[pos + brk - CROSSTALK_GAME_CHAT_STEP] == LDG_ASCII_SPACE) { break; } }

            if (brk > 0) { take = brk; }
        }

        seg_start[cunt] = pos;
        seg_len[cunt] = take;
        cunt++;
        pos = pos + take;

        while (pos < total && text[pos] == LDG_ASCII_SPACE) { pos++; }
    }

    *out_cunt = cunt;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_game_chat_tab_put(ldg_term_render_t *render, uint32_t *pos_col, uint32_t row, const char *name, uint8_t unseen, uint8_t lit)
{
    char line[CROSSTALK_GAME_CHAT_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t seg_w = 0;
    uint32_t ret = 0;

    if (snprintf(line, sizeof(line), CROSSTALK_GAME_CHAT_TAB_FMT, name, unseen != LDG_TRUTH_FALSE ? CROSSTALK_GAME_CHAT_UNSEEN_MARK : "") < 0) { return LDG_ERR_STR_FMT; }

    ret = crosstalk_ui_cell_rgba_put(render, *pos_col, row, line, (uint64_t)sizeof(line), lit == LDG_TRUTH_TRUE ? (uint32_t)CROSSTALK_UI_THEME_LIT_RGBA : (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, &seg_w);

    *pos_col = *pos_col + seg_w + (uint32_t)CROSSTALK_GAME_CHAT_TAB_GAP;

    return ret;
}

static LDG_NOINLINE uint32_t crosstalk_game_chat_tabs_draw(const crosstalk_game_chat_ctx_t *ctx, ldg_term_render_t *render)
{
    uint32_t pos_col = CROSSTALK_GAME_CHAT_ORIG_COL;
    uint32_t ret = LDG_ERR_AOK;

    for (uint32_t idx = 0; idx < (uint32_t)CROSSTALK_GAME_CHAT_GROUP_CUNT && ret == LDG_ERR_AOK; idx++)
    {
        if (crosstalk_game_chat_group_live_is((uint8_t)idx, ctx->sesh_live, ctx->discord_live) != LDG_TRUTH_TRUE) { continue; }

        ret = crosstalk_game_chat_tab_put(render, &pos_col, (uint32_t)CROSSTALK_GAME_CHAT_ROW_GROUP, crosstalk_game_chat_group_name_get((uint8_t)idx), crosstalk_game_chat_group_unseen_is(ctx, (uint8_t)idx), ctx->group == (uint8_t)idx ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE);
    }

    pos_col = CROSSTALK_GAME_CHAT_ORIG_COL;

    for (uint32_t idx = 0; idx < ctx->conv_cunt && ret == LDG_ERR_AOK; idx++)
    {
        if (ctx->conv[idx].group != ctx->group) { continue; }

        ret = crosstalk_game_chat_tab_put(render, &pos_col, (uint32_t)CROSSTALK_GAME_CHAT_ROW_CONV, ctx->conv[idx].name[0] != LDG_STR_TERM ? ctx->conv[idx].name : "?", ctx->conv[idx].unseen, ctx->conv_sel[ctx->group] == idx ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE);
    }

    return ret;
}

static LDG_NOINLINE uint32_t crosstalk_game_chat_msgs_draw(const crosstalk_game_chat_conv_t *conv, ldg_term_render_t *render, uint32_t col_cunt, uint32_t *row)
{
    char line[CROSSTALK_GAME_CHAT_LINE_CAP] = LDG_ARR_ZERO_INIT;
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t pane = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, &fg);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_CELL_PANE_RGBA, &pane); }

    if (ret != LDG_ERR_AOK) { return ret; }

    for (uint32_t idx = 0; idx < conv->cunt; idx = idx + CROSSTALK_GAME_CHAT_STEP)
    {
        uint32_t seg_start[CROSSTALK_GAME_CHAT_WRAP_CAP] = LDG_ARR_ZERO_INIT;
        uint32_t seg_len[CROSSTALK_GAME_CHAT_WRAP_CAP] = LDG_ARR_ZERO_INIT;
        uint32_t seg_cunt = 0;
        uint32_t slot = (conv->hd + idx) % CROSSTALK_GAME_CHAT_HISTORY_CAP;

        snprintf(line, sizeof(line), CROSSTALK_GAME_CHAT_MSG_FMT, conv->msg[slot].name, conv->msg[slot].text);

        ret = crosstalk_game_chat_wrap(line, col_cunt, seg_start, seg_len, &seg_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        for (uint32_t seg = 0; seg < seg_cunt; seg++)
        {
            if (seg_len[seg] != 0)
            {
                ret = ldg_term_render_text_draw(render, (uint32_t)CROSSTALK_GAME_CHAT_ORIG_COL, *row, line + seg_start[seg], (uint64_t)seg_len[seg], &fg, &pane);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            *row = *row + CROSSTALK_GAME_CHAT_STEP;
        }
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_game_chat_input_draw(const crosstalk_game_chat_ctx_t *ctx, ldg_term_render_t *render, uint32_t row)
{
    ldg_term_render_color_t cursor = LDG_STRUCT_ZERO_INIT;
    uint64_t len = 0;
    uint32_t ret = 0;

    ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)CROSSTALK_GAME_CHAT_ORIG_COL, row, CROSSTALK_GAME_CHAT_INPUT_MARK, (uint64_t)CROSSTALK_GAME_CHAT_LINE_CAP, (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, 0x0);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)CROSSTALK_GAME_CHAT_INPUT_MARK_W, row, ctx->draft, (uint64_t)sizeof(ctx->draft), (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, 0x0); }

    if (ret == LDG_ERR_AOK) { ret = ldg_str_len_get(ctx->draft, (uint64_t)sizeof(ctx->draft), &len); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, &cursor); }

    if (ret != LDG_ERR_AOK) { return ret; }

    return crosstalk_ui_cell_span_fill(render, (uint32_t)CROSSTALK_GAME_CHAT_INPUT_MARK_W + (uint32_t)len, row, (uint32_t)CROSSTALK_GAME_CHAT_CURSOR_W, &cursor, &cursor);
}

static uint32_t chat_draw_cb(uint64_t win_id, void *user, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    crosstalk_game_chat_ctx_t *ctx = (crosstalk_game_chat_ctx_t *)user;
    crosstalk_game_chat_conv_t *conv = 0x0;
    uint32_t row = CROSSTALK_GAME_CHAT_ROW_PEER;
    uint32_t sel = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (win_id != (uint64_t)CROSSTALK_GAME_CHAT_WINDOW_ID || col_cunt == 0 || row_cunt == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    ctx->unseen = LDG_TRUTH_FALSE;

    ret = crosstalk_game_chat_tabs_draw(ctx, render);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    sel = ctx->conv_sel[ctx->group];
    if (sel < ctx->conv_cunt && ctx->conv[sel].group == ctx->group) { conv = &ctx->conv[sel]; }

    if (conv != 0x0) { conv->unseen = LDG_TRUTH_FALSE; }

    if (conv != 0x0 && conv->name[0] != LDG_STR_TERM)
    {
        uint32_t peer_col = (ctx->group == (uint8_t)CROSSTALK_GAME_CHAT_GROUP_SESH) ? (uint32_t)CROSSTALK_UI_THEME_ACCENT_RGBA : crosstalk_game_hub_reach_col_get(conv->reach);

        ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)CROSSTALK_GAME_CHAT_ORIG_COL, row, conv->name, (uint64_t)sizeof(conv->name), peer_col, 0x0);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        row = row + CROSSTALK_GAME_CHAT_STEP;
    }

    if (conv == 0x0 || conv->cunt == 0)
    {
        ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)CROSSTALK_GAME_CHAT_ORIG_COL, row, CROSSTALK_GAME_CHAT_EMPTY_TEXT, (uint64_t)CROSSTALK_GAME_CHAT_LINE_CAP, (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, 0x0);
        row = row + CROSSTALK_GAME_CHAT_STEP;
    }
    else { ret = crosstalk_game_chat_msgs_draw(conv, render, col_cunt, &row); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_game_chat_input_draw(ctx, render, row); }

    ctx->win.content_row_cunt = row + CROSSTALK_GAME_CHAT_STEP;

    if (row != ctx->line_seen || ctx->group != ctx->group_seen)
    {
        ctx->line_seen = row;
        ctx->group_seen = ctx->group;

        if (crosstalk_ui_window_row_show(&ctx->win, row) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "chat scroll failed"); }
    }

    return ret;
}

static void crosstalk_game_chat_group_cycle(crosstalk_game_chat_ctx_t *ctx)
{
    for (uint32_t step = 0; step < (uint32_t)CROSSTALK_GAME_CHAT_GROUP_CUNT; step = step + CROSSTALK_GAME_CHAT_STEP)
    {
        ctx->group = (uint8_t)(((uint32_t)ctx->group + CROSSTALK_GAME_CHAT_STEP) % (uint32_t)CROSSTALK_GAME_CHAT_GROUP_CUNT);

        if (crosstalk_game_chat_group_live_is(ctx->group, ctx->sesh_live, ctx->discord_live) == LDG_TRUTH_TRUE) { return; }
    }
}

static void crosstalk_game_chat_conv_cycle(crosstalk_game_chat_ctx_t *ctx, uint8_t back)
{
    uint32_t at = ctx->conv_sel[ctx->group];

    if (ctx->conv_cunt == 0) { return; }

    if (at >= ctx->conv_cunt) { at = (uint32_t)CROSSTALK_GAME_CHAT_SEL_FIRST; }

    for (uint32_t step = 0; step < ctx->conv_cunt; step = step + CROSSTALK_GAME_CHAT_STEP)
    {
        if (back == LDG_TRUTH_TRUE) { at = at == 0 ? ctx->conv_cunt - CROSSTALK_GAME_CHAT_STEP : at - CROSSTALK_GAME_CHAT_STEP; }
        else { at = (at + CROSSTALK_GAME_CHAT_STEP) % ctx->conv_cunt; }

        if (ctx->conv[at].group == ctx->group)
        {
            ctx->conv_sel[ctx->group] = at;
            return;
        }
    }
}

static uint32_t crosstalk_game_chat_send(crosstalk_game_chat_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (ctx->draft[0] == LDG_STR_TERM) { return LDG_ERR_AOK; }

    ret = ldg_str_rbrcpy(ctx->out_text, ctx->draft, (uint64_t)CROSSTALK_GAME_CHAT_TEXT_CAP);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    ctx->out_send = LDG_TRUTH_TRUE;
    ctx->draft[0] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

static uint32_t chat_key_isr(uint64_t win_id, void *user, const ldg_term_key_event_t *key, uint8_t *consumed)
{
    crosstalk_game_chat_ctx_t *ctx = (crosstalk_game_chat_ctx_t *)user;
    uint64_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || key == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    if (win_id != (uint64_t)CROSSTALK_GAME_CHAT_WINDOW_ID) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (key->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE) { return LDG_ERR_AOK; }

    if ((key->mods & ((uint32_t)LDG_TERM_MOD_CTRL | (uint32_t)LDG_TERM_MOD_ALT | (uint32_t)LDG_TERM_MOD_SUPER)) != 0) { return LDG_ERR_AOK; }

    ret = ldg_str_len_get(ctx->draft, (uint64_t)sizeof(ctx->draft), &len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *consumed = LDG_TRUTH_TRUE;

    if (key->key == (uint8_t)LDG_TERM_KEY_ENTER) { return crosstalk_game_chat_send(ctx); }

    if (key->key == (uint8_t)LDG_TERM_KEY_ESCAPE) { ctx->draft[0] = LDG_STR_TERM; return LDG_ERR_AOK; }

    if (key->key == (uint8_t)LDG_TERM_KEY_BACKSPACE)
    {
        if (len != 0) { ctx->draft[len - (uint64_t)CROSSTALK_GAME_CHAT_STEP] = LDG_STR_TERM; }

        return LDG_ERR_AOK;
    }

    if (key->key == (uint8_t)LDG_TERM_KEY_TAB) { crosstalk_game_chat_group_cycle(ctx); return LDG_ERR_AOK; }

    if (key->key == (uint8_t)LDG_TERM_KEY_LHS || key->key == (uint8_t)LDG_TERM_KEY_RHS)
    {
        crosstalk_game_chat_conv_cycle(ctx, key->key == (uint8_t)LDG_TERM_KEY_LHS ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE);
        return LDG_ERR_AOK;
    }

    *consumed = LDG_TRUTH_FALSE;

    for (uint32_t at = 0; at < key->text_cunt && at < (uint32_t)LDG_TERM_KEY_TEXT_MAX; at = at + CROSSTALK_GAME_CHAT_STEP)
    {
        if (key->text[at] < (uint32_t)LDG_ASCII_PRINTABLE_MIN || key->text[at] > (uint32_t)LDG_ASCII_PRINTABLE_MAX) { continue; }

        *consumed = LDG_TRUTH_TRUE;

        if (len + (uint64_t)LDG_STR_TERM_SIZE >= (uint64_t)sizeof(ctx->draft)) { break; }

        ctx->draft[len] = (char)key->text[at];
        len = len + (uint64_t)CROSSTALK_GAME_CHAT_STEP;
        ctx->draft[len] = LDG_STR_TERM;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_chat_frame_set(crosstalk_game_chat_ctx_t *ctx, uint8_t sesh_live, uint8_t discord_live)
{
    char title[CROSSTALK_GAME_CHAT_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint8_t group_prev = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->sesh_live = sesh_live;
    ctx->discord_live = discord_live;
    ctx->win.highlighted = crosstalk_game_chat_any_unseen_is(ctx);

    group_prev = ctx->group;

    if (crosstalk_game_chat_group_live_is(ctx->group, sesh_live, discord_live) != LDG_TRUTH_TRUE) { ctx->group = (uint8_t)CROSSTALK_GAME_CHAT_GROUP_STEAM; }

    if (ctx->win_nav == 0x0 || (ctx->group == group_prev && ctx->group == ctx->group_seen)) { return LDG_ERR_AOK; }

    snprintf(title, sizeof(title), CROSSTALK_GAME_CHAT_TITLE_FMT, CROSSTALK_GAME_CHAT_TITLE, crosstalk_game_chat_group_name_get(ctx->group));

    return crosstalk_ui_window_title_set(ctx->win_nav, (uint64_t)CROSSTALK_GAME_CHAT_WINDOW_ID, title);
}

uint32_t crosstalk_game_chat_win_reg(crosstalk_game_chat_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot)
{
    if (LDG_UNLIKELY(ctx == 0x0 || win_nav == 0x0 || out_slot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->win_nav = win_nav;

    return crosstalk_ui_window_reg(win_nav, (uint64_t)CROSSTALK_GAME_CHAT_WINDOW_ID, CROSSTALK_GAME_CHAT_TITLE, &ctx->win, chat_draw_cb, chat_key_isr, ctx, out_slot);
}

uint32_t crosstalk_game_chat_out_get(crosstalk_game_chat_ctx_t *ctx, char *out_text, uint32_t out_cap, uint8_t *out_send)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_text == 0x0 || out_send == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    out_text[0] = LDG_STR_TERM;
    *out_send = ctx->out_send;

    if (ctx->out_send == LDG_TRUTH_FALSE) { return LDG_ERR_AOK; }

    ret = ldg_str_rbrcpy(out_text, ctx->out_text, (uint64_t)out_cap);
    ctx->out_send = LDG_TRUTH_FALSE;
    ctx->out_text[0] = LDG_STR_TERM;

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    return LDG_ERR_AOK;
}
