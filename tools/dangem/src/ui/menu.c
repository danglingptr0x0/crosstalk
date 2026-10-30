#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/term/layout.h>
#include <dangling/term/render.h>

#include "ui/menu.h"
#include "ui/ui.h"

#define DANGEM_UI_MENU_TITLE " DANGLING Ecosystem Manager r0.1"
#define DANGEM_UI_MENU_EXIT "(F10 = Exit)"
#define DANGEM_UI_MENU_FRAME_COL 1
#define DANGEM_UI_MENU_FRAME_ROW 0
#define DANGEM_UI_MENU_FRAME_TRIM 2
#define DANGEM_UI_MENU_TITLE_ROW 1
#define DANGEM_UI_MENU_RULE_ROW 2
#define DANGEM_UI_MENU_ITEM_ROW 4
#define DANGEM_UI_MENU_ITEM_GAP 3
#define DANGEM_UI_MENU_ITEM_INDENT 3
#define DANGEM_UI_MENU_KEY_GAP 4
#define DANGEM_UI_MENU_INNER 2
#define DANGEM_UI_MENU_COL_MIN 40
#define DANGEM_UI_MENU_ROW_MIN 14
#define DANGEM_UI_MENU_NAME_LAUNCH "- Launch Crosstalk"
#define DANGEM_UI_MENU_NAME_BUILD "- Build Crosstalk"
#define DANGEM_UI_MENU_NAME_DEPS "- Obtain dependencies"
#define DANGEM_UI_MENU_KEY_LAUNCH 'L'
#define DANGEM_UI_MENU_KEY_BUILD 'B'
#define DANGEM_UI_MENU_KEY_DEPS 'O'

static uint32_t menu_item_draw(ldg_term_render_t *render, uint32_t col, uint32_t row, char key, const char *name, const char *status, uint32_t status_tone)
{
    uint32_t ret = 0;

    ret = dangem_ui_ch_put(render, col, row, key, DANGEM_UI_TONE_INVERSE);
    if (ret == LDG_ERR_AOK) { ret = dangem_ui_text_put(render, col + DANGEM_UI_MENU_INNER, row, name, DANGEM_UI_TONE_BRIGHT); }

    if (ret == LDG_ERR_AOK) { ret = dangem_ui_text_put(render, col + DANGEM_UI_MENU_KEY_GAP, row + DANGEM_UI_STEP, status, status_tone); }

    return ret;
}

uint32_t dangem_ui_menu_draw(ldg_term_render_t *render, const dangem_ui_menu_view_t *view)
{
    char date[DANGEM_UI_DATE_CAP] = LDG_ARR_ZERO_INIT;
    ldg_term_rect_t frame = LDG_STRUCT_ZERO_INIT;
    uint32_t inner_col = 0;
    uint32_t inner_w = 0;
    uint32_t mid = 0;
    uint32_t base = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || view == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (render->col_cunt < DANGEM_UI_MENU_COL_MIN || render->row_cunt < DANGEM_UI_MENU_ROW_MIN) { return dangem_ui_text_put(render, DANGEM_UI_ORIG_COL, DANGEM_UI_ORIG_ROW, DANGEM_UI_SMALL_TEXT, DANGEM_UI_TONE_BRIGHT); }

    frame.col = DANGEM_UI_MENU_FRAME_COL;
    frame.row = DANGEM_UI_MENU_FRAME_ROW;
    frame.col_cunt = render->col_cunt - DANGEM_UI_MENU_FRAME_TRIM;
    frame.row_cunt = render->row_cunt - DANGEM_UI_MENU_FRAME_TRIM;

    inner_col = frame.col + DANGEM_UI_STEP;
    inner_w = frame.col_cunt - DANGEM_UI_MENU_INNER;
    mid = frame.col + frame.col_cunt / DANGEM_UI_HALF;
    base = frame.row + frame.row_cunt - DANGEM_UI_STEP;

    ret = dangem_ui_frame_draw(render, &frame);

    if (ret == LDG_ERR_AOK) { ret = dangem_ui_row_fill(render, inner_col, DANGEM_UI_MENU_TITLE_ROW, inner_w, DANGEM_UI_TONE_INVERSE); }

    if (ret == LDG_ERR_AOK) { ret = dangem_ui_text_put(render, inner_col, DANGEM_UI_MENU_TITLE_ROW, DANGEM_UI_MENU_TITLE, DANGEM_UI_TONE_INVERSE); }

    if (ret == LDG_ERR_AOK && dangem_ui_date_get(date, (uint64_t)sizeof(date)) == LDG_ERR_AOK && (uint64_t)strlen(date) < (uint64_t)inner_w) { ret = dangem_ui_text_put(render, inner_col + inner_w - (uint32_t)strlen(date) - DANGEM_UI_STEP, DANGEM_UI_MENU_TITLE_ROW, date, DANGEM_UI_TONE_INVERSE); }

    for (at = inner_col; ret == LDG_ERR_AOK && at < inner_col + inner_w; at++) { ret = dangem_ui_ch_put(render, at, DANGEM_UI_MENU_RULE_ROW, DANGEM_UI_EDGE_FLAT, DANGEM_UI_TONE_EDGE); }

    if (ret == LDG_ERR_AOK) { ret = dangem_ui_vline_draw(render, mid, DANGEM_UI_MENU_RULE_ROW + DANGEM_UI_STEP, base - DANGEM_UI_MENU_RULE_ROW - DANGEM_UI_STEP); }

    if (ret == LDG_ERR_AOK) { ret = menu_item_draw(render, inner_col + DANGEM_UI_MENU_ITEM_INDENT, DANGEM_UI_MENU_ITEM_ROW, DANGEM_UI_MENU_KEY_LAUNCH, DANGEM_UI_MENU_NAME_LAUNCH, view->launch, DANGEM_UI_TONE_DIM); }

    if (ret == LDG_ERR_AOK) { ret = menu_item_draw(render, inner_col + DANGEM_UI_MENU_ITEM_INDENT, DANGEM_UI_MENU_ITEM_ROW + DANGEM_UI_MENU_ITEM_GAP, DANGEM_UI_MENU_KEY_BUILD, DANGEM_UI_MENU_NAME_BUILD, view->build, view->build_alert == LDG_TRUTH_TRUE ? DANGEM_UI_TONE_ALERT : DANGEM_UI_TONE_DIM); }

    if (ret == LDG_ERR_AOK) { ret = menu_item_draw(render, mid + DANGEM_UI_MENU_ITEM_INDENT, DANGEM_UI_MENU_ITEM_ROW, DANGEM_UI_MENU_KEY_DEPS, DANGEM_UI_MENU_NAME_DEPS, view->deps, DANGEM_UI_TONE_DIM); }

    if (ret == LDG_ERR_AOK) { ret = dangem_ui_text_put(render, inner_col, base + DANGEM_UI_STEP, view->notice, DANGEM_UI_TONE_NORMAL); }

    if (ret == LDG_ERR_AOK) { ret = dangem_ui_text_put(render, frame.col + frame.col_cunt - (uint32_t)(sizeof(DANGEM_UI_MENU_EXIT) - LDG_STR_TERM_SIZE), base + DANGEM_UI_STEP, DANGEM_UI_MENU_EXIT, DANGEM_UI_TONE_NORMAL); }

    return ret;
}
