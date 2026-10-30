#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "ui/cell.h"

#define CELL_BLANK_CAP 32
#define CELL_BLANK_CH ' '

uint32_t crosstalk_ui_cell_color_get(uint32_t rgba, ldg_term_render_color_t *out)
{
    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->red = (uint8_t)((rgba >> (uint32_t)CROSSTALK_UI_CELL_RGBA_RED_SHIFT) & (uint32_t)LDG_BYTE_MASK);
    out->green = (uint8_t)((rgba >> (uint32_t)CROSSTALK_UI_CELL_RGBA_GREEN_SHIFT) & (uint32_t)LDG_BYTE_MASK);
    out->blue = (uint8_t)((rgba >> (uint32_t)CROSSTALK_UI_CELL_RGBA_BLUE_SHIFT) & (uint32_t)LDG_BYTE_MASK);
    out->pudding[0] = 0;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_ui_cell_text_put(ldg_term_render_t *render, uint32_t col, uint32_t row, const char *text, uint64_t cap, const ldg_term_render_color_t *fg, const ldg_term_render_color_t *bg)
{
    uint64_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || text == 0x0 || fg == 0x0 || bg == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_str_len_get(text, cap, &len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (len == 0) { return LDG_ERR_AOK; }

    return ldg_term_render_text_draw(render, col, row, text, len, fg, bg);
}

uint32_t crosstalk_ui_cell_fmt_put(ldg_term_render_t *render, uint32_t col, uint32_t row, const ldg_term_render_color_t *fg, const ldg_term_render_color_t *bg, const char *fmt, ...)
{
    char line[CROSSTALK_UI_CELL_LINE_CAP] = LDG_ARR_ZERO_INIT;
    va_list args;
    int32_t len = 0;

    if (LDG_UNLIKELY(render == 0x0 || fmt == 0x0 || fg == 0x0 || bg == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    va_start(args, fmt);
    len = (int32_t)vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    if (len < 0) { return LDG_ERR_STR_FMT; }

    return crosstalk_ui_cell_text_put(render, col, row, line, (uint64_t)sizeof(line), fg, bg);
}

uint32_t crosstalk_ui_cell_rgba_put(ldg_term_render_t *render, uint32_t col, uint32_t row, const char *text, uint64_t cap, uint32_t fg_rgba, uint32_t *out_w)
{
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t bg = LDG_STRUCT_ZERO_INIT;
    uint64_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (out_w != 0x0) { *out_w = 0; }

    ret = crosstalk_ui_cell_color_get(fg_rgba, &fg);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_CELL_PANE_RGBA, &bg); }

    if (ret == LDG_ERR_AOK) { ret = ldg_str_len_get(text, cap, &len); }

    if (ret != LDG_ERR_AOK) { return ret; }

    if (len == 0) { return LDG_ERR_AOK; }

    if (len > (uint64_t)UINT32_MAX) { len = (uint64_t)UINT32_MAX; }

    if (out_w != 0x0) { *out_w = (uint32_t)len; }

    return ldg_term_render_text_draw(render, col, row, text, len, &fg, &bg);
}

uint32_t crosstalk_ui_cell_rgba_fmt_put(ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t fg_rgba, uint32_t *out_w, const char *fmt, ...)
{
    char line[CROSSTALK_UI_CELL_LINE_CAP] = LDG_ARR_ZERO_INIT;
    va_list args;
    int32_t len = 0;

    if (LDG_UNLIKELY(render == 0x0 || fmt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (out_w != 0x0) { *out_w = 0; }

    va_start(args, fmt);
    len = (int32_t)vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    if (len < 0) { return LDG_ERR_STR_FMT; }

    return crosstalk_ui_cell_rgba_put(render, col, row, line, (uint64_t)sizeof(line), fg_rgba, out_w);
}

uint32_t crosstalk_ui_cell_line_draw(ldg_term_render_t *render, uint32_t x_lhs, uint32_t y_lhs, uint32_t x_rhs, uint32_t y_rhs, uint32_t rgba)
{
    ldg_term_render_vert2_t lhs = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_vert2_t rhs = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_ui_cell_color_get(rgba, &lhs.color);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    rhs.color = lhs.color;
    lhs.pos[0] = (ldg_term_render_fixed_t)(x_lhs << LDG_TERM_RENDER_FIXED_SHIFT);
    lhs.pos[1] = (ldg_term_render_fixed_t)(y_lhs << LDG_TERM_RENDER_FIXED_SHIFT);
    rhs.pos[0] = (ldg_term_render_fixed_t)(x_rhs << LDG_TERM_RENDER_FIXED_SHIFT);
    rhs.pos[1] = (ldg_term_render_fixed_t)(y_rhs << LDG_TERM_RENDER_FIXED_SHIFT);

    return ldg_term_render_line_draw(render, &lhs, &rhs);
}

uint32_t crosstalk_ui_cell_sample_fill(ldg_term_render_t *render, uint32_t pos_x, uint32_t pos_y, uint32_t extent_w, uint32_t extent_h, uint32_t rgba)
{
    ldg_term_render_fixed_t pos[LDG_TERM_RENDER_PLANE_CUNT] = LDG_ARR_ZERO_INIT;
    ldg_term_render_fixed_t extent[LDG_TERM_RENDER_PLANE_CUNT] = LDG_ARR_ZERO_INIT;
    ldg_term_render_color_t color = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_ui_cell_color_get(rgba, &color);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    pos[0] = (ldg_term_render_fixed_t)(pos_x << LDG_TERM_RENDER_FIXED_SHIFT);
    pos[1] = (ldg_term_render_fixed_t)(pos_y << LDG_TERM_RENDER_FIXED_SHIFT);
    extent[0] = (ldg_term_render_fixed_t)(extent_w << LDG_TERM_RENDER_FIXED_SHIFT);
    extent[1] = (ldg_term_render_fixed_t)(extent_h << LDG_TERM_RENDER_FIXED_SHIFT);

    return ldg_term_render_rect_fill(render, &pos[0], &extent[0], &color);
}

uint32_t crosstalk_ui_cell_dot_draw(ldg_term_render_t *render, uint32_t pos_x, uint32_t pos_y, uint32_t rgba)
{
    ldg_term_render_vert2_t pt = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_ui_cell_color_get(rgba, &pt.color);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    pt.pos[0] = (ldg_term_render_fixed_t)(pos_x << LDG_TERM_RENDER_FIXED_SHIFT);
    pt.pos[1] = (ldg_term_render_fixed_t)(pos_y << LDG_TERM_RENDER_FIXED_SHIFT);

    return ldg_term_render_point_draw(render, &pt);
}

uint32_t crosstalk_ui_cell_span_fill(ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t width, const ldg_term_render_color_t *fg, const ldg_term_render_color_t *bg)
{
    char blank[CELL_BLANK_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || fg == 0x0 || bg == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(&blank[0], CELL_BLANK_CH, sizeof(blank));

    for (at = 0; at < width; at = at + (uint32_t)CELL_BLANK_CAP)
    {
        uint32_t len = width - at < (uint32_t)CELL_BLANK_CAP ? width - at : (uint32_t)CELL_BLANK_CAP;

        ret = ldg_term_render_text_draw(render, col + at, row, &blank[0], (uint64_t)len, fg, bg);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return LDG_ERR_AOK;
}
