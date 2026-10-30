#ifndef CROSSTALK_UI_CELL_H
#define CROSSTALK_UI_CELL_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/term/render.h>

#define CROSSTALK_UI_CELL_LINE_CAP 512
#define CROSSTALK_UI_CELL_RGBA_RED_SHIFT 24
#define CROSSTALK_UI_CELL_RGBA_GREEN_SHIFT 16
#define CROSSTALK_UI_CELL_RGBA_BLUE_SHIFT 8
#define CROSSTALK_UI_CELL_PANE_RGBA 0x000000FFU

uint32_t crosstalk_ui_cell_color_get(uint32_t rgba, ldg_term_render_color_t *out);
uint32_t crosstalk_ui_cell_rgba_put(ldg_term_render_t *render, uint32_t col, uint32_t row, const char *text, uint64_t cap, uint32_t fg_rgba, uint32_t *out_w);
uint32_t crosstalk_ui_cell_rgba_fmt_put(ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t fg_rgba, uint32_t *out_w, const char *fmt, ...);
uint32_t crosstalk_ui_cell_text_put(ldg_term_render_t *render, uint32_t col, uint32_t row, const char *text, uint64_t cap, const ldg_term_render_color_t *fg, const ldg_term_render_color_t *bg);
uint32_t crosstalk_ui_cell_fmt_put(ldg_term_render_t *render, uint32_t col, uint32_t row, const ldg_term_render_color_t *fg, const ldg_term_render_color_t *bg, const char *fmt, ...);
uint32_t crosstalk_ui_cell_line_draw(ldg_term_render_t *render, uint32_t x_lhs, uint32_t y_lhs, uint32_t x_rhs, uint32_t y_rhs, uint32_t rgba);
uint32_t crosstalk_ui_cell_sample_fill(ldg_term_render_t *render, uint32_t pos_x, uint32_t pos_y, uint32_t extent_w, uint32_t extent_h, uint32_t rgba);
uint32_t crosstalk_ui_cell_dot_draw(ldg_term_render_t *render, uint32_t pos_x, uint32_t pos_y, uint32_t rgba);
uint32_t crosstalk_ui_cell_span_fill(ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t width, const ldg_term_render_color_t *fg, const ldg_term_render_color_t *bg);

#endif
