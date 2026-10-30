#ifndef DANGEM_UI_H
#define DANGEM_UI_H

#include <stdint.h>

#include <dangling/term/layout.h>
#include <dangling/term/render.h>

#define DANGEM_UI_BG_RED 0x0AU
#define DANGEM_UI_BG_GREEN 0x0AU
#define DANGEM_UI_BG_BLUE 0x14U
#define DANGEM_UI_TEXT_RED 0xCFU
#define DANGEM_UI_TEXT_GREEN 0xD2U
#define DANGEM_UI_TEXT_BLUE 0xFFU
#define DANGEM_UI_BRIGHT_RED 0xEAU
#define DANGEM_UI_BRIGHT_GREEN 0xF0U
#define DANGEM_UI_BRIGHT_BLUE 0xFFU
#define DANGEM_UI_DIM_RED 0x8FU
#define DANGEM_UI_DIM_GREEN 0xB0U
#define DANGEM_UI_DIM_BLUE 0xFFU
#define DANGEM_UI_LIME_RED 0xBFU
#define DANGEM_UI_LIME_GREEN 0xFFU
#define DANGEM_UI_LIME_BLUE 0x00U
#define DANGEM_UI_ACCENT_RED 0x3DU
#define DANGEM_UI_ACCENT_GREEN 0x6CU
#define DANGEM_UI_ACCENT_BLUE 0xFFU
#define DANGEM_UI_PRESSED_RED 0xFFU
#define DANGEM_UI_PRESSED_GREEN 0xFFU
#define DANGEM_UI_PRESSED_BLUE 0xFFU
#define DANGEM_UI_ALERT_RED 0xC8U
#define DANGEM_UI_ALERT_GREEN 0x32U
#define DANGEM_UI_ALERT_BLUE 0x32U
#define DANGEM_UI_TEXT_CAP 256
#define DANGEM_UI_DATE_CAP 64
#define DANGEM_UI_DATE_FMT "%.9s, %.3s %u32, %u32 %u32:%u32%u32 %.2s UTC"
#define DANGEM_UI_HOURS_PER_HALF 12U
#define DANGEM_UI_AM "AM"
#define DANGEM_UI_PM "PM"
#define DANGEM_UI_STEP 1
#define DANGEM_UI_HALF 2
#define DANGEM_UI_CORNER '+'
#define DANGEM_UI_EDGE_FLAT '='
#define DANGEM_UI_EDGE_TALL '|'
#define DANGEM_UI_TICK '+'
#define DANGEM_UI_AXIS '-'
#define DANGEM_UI_BAR '#'
#define DANGEM_UI_BLANK ' '
#define DANGEM_UI_ORIG_COL 0
#define DANGEM_UI_ORIG_ROW 0
#define DANGEM_UI_SMALL_TEXT "terminal too small"

typedef enum dangem_ui_tone
{
    DANGEM_UI_TONE_NORMAL = 0,
    DANGEM_UI_TONE_BRIGHT,
    DANGEM_UI_TONE_DIM,
    DANGEM_UI_TONE_INVERSE,
    DANGEM_UI_TONE_EDGE,
    DANGEM_UI_TONE_FILL,
    DANGEM_UI_TONE_ALERT
} dangem_ui_tone_t;

uint32_t dangem_ui_bg_get(ldg_term_render_color_t *out);

uint32_t dangem_ui_text_put(ldg_term_render_t *render, uint32_t col, uint32_t row, const char *text, uint32_t tone);
uint32_t dangem_ui_ch_put(ldg_term_render_t *render, uint32_t col, uint32_t row, char ch, uint32_t tone);
uint32_t dangem_ui_row_fill(ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t col_cunt, uint32_t tone);
uint32_t dangem_ui_frame_draw(ldg_term_render_t *render, const ldg_term_rect_t *rect);
uint32_t dangem_ui_vline_draw(ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t row_cunt);
uint32_t dangem_ui_date_get(char *dst, uint64_t cap);
uint32_t dangem_ui_gauge_draw(ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t col_cunt, const char *label, uint64_t val, uint64_t max, uint32_t tick_cunt, const char *unit);

#endif
