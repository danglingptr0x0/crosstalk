#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/fmt.h>
#include <dangling/term/render.h>
#include <dangling/time/cal.h>
#include <dangling/time/time.h>

#include "ui/ui.h"

#define DANGEM_UI_GAUGE_LABEL_W 10
#define DANGEM_UI_GAUGE_TAIL_W 22
#define DANGEM_UI_GAUGE_ROW_TICK 0
#define DANGEM_UI_GAUGE_ROW_AXIS 1
#define DANGEM_UI_GAUGE_ROW_BAR 2
#define DANGEM_UI_GAUGE_GAP 2
#define DANGEM_UI_GAUGE_AXIS_MIN 8

static void ui_tone_get(uint32_t tone, ldg_term_render_color_t *fg, ldg_term_render_color_t *bg)
{
    fg->red = (uint8_t)DANGEM_UI_TEXT_RED;
    fg->green = (uint8_t)DANGEM_UI_TEXT_GREEN;
    fg->blue = (uint8_t)DANGEM_UI_TEXT_BLUE;
    bg->red = (uint8_t)DANGEM_UI_BG_RED;
    bg->green = (uint8_t)DANGEM_UI_BG_GREEN;
    bg->blue = (uint8_t)DANGEM_UI_BG_BLUE;

    if (tone == (uint32_t)DANGEM_UI_TONE_BRIGHT)
    {
        fg->red = (uint8_t)DANGEM_UI_BRIGHT_RED;
        fg->green = (uint8_t)DANGEM_UI_BRIGHT_GREEN;
        fg->blue = (uint8_t)DANGEM_UI_BRIGHT_BLUE;
    }
    else if (tone == (uint32_t)DANGEM_UI_TONE_DIM)
    {
        fg->red = (uint8_t)DANGEM_UI_DIM_RED;
        fg->green = (uint8_t)DANGEM_UI_DIM_GREEN;
        fg->blue = (uint8_t)DANGEM_UI_DIM_BLUE;
    }
    else if (tone == (uint32_t)DANGEM_UI_TONE_EDGE)
    {
        fg->red = (uint8_t)DANGEM_UI_LIME_RED;
        fg->green = (uint8_t)DANGEM_UI_LIME_GREEN;
        fg->blue = (uint8_t)DANGEM_UI_LIME_BLUE;
    }
    else if (tone == (uint32_t)DANGEM_UI_TONE_FILL)
    {
        fg->red = (uint8_t)DANGEM_UI_ACCENT_RED;
        fg->green = (uint8_t)DANGEM_UI_ACCENT_GREEN;
        fg->blue = (uint8_t)DANGEM_UI_ACCENT_BLUE;
    }
    else if (tone == (uint32_t)DANGEM_UI_TONE_ALERT)
    {
        fg->red = (uint8_t)DANGEM_UI_ALERT_RED;
        fg->green = (uint8_t)DANGEM_UI_ALERT_GREEN;
        fg->blue = (uint8_t)DANGEM_UI_ALERT_BLUE;
    }
    else if (tone == (uint32_t)DANGEM_UI_TONE_INVERSE)
    {
        fg->red = (uint8_t)DANGEM_UI_PRESSED_RED;
        fg->green = (uint8_t)DANGEM_UI_PRESSED_GREEN;
        fg->blue = (uint8_t)DANGEM_UI_PRESSED_BLUE;
        bg->red = (uint8_t)DANGEM_UI_ACCENT_RED;
        bg->green = (uint8_t)DANGEM_UI_ACCENT_GREEN;
        bg->blue = (uint8_t)DANGEM_UI_ACCENT_BLUE;
    }
}

uint32_t dangem_ui_bg_get(ldg_term_render_color_t *out)
{
    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->red = (uint8_t)DANGEM_UI_BG_RED;
    out->green = (uint8_t)DANGEM_UI_BG_GREEN;
    out->blue = (uint8_t)DANGEM_UI_BG_BLUE;

    return LDG_ERR_AOK;
}

uint32_t dangem_ui_text_put(ldg_term_render_t *render, uint32_t col, uint32_t row, const char *text, uint32_t tone)
{
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t bg = LDG_STRUCT_ZERO_INIT;
    uint64_t len = 0;

    if (LDG_UNLIKELY(render == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (row >= render->row_cunt || col >= render->col_cunt) { return LDG_ERR_AOK; }

    len = (uint64_t)strlen(text);
    if (col + len > (uint64_t)render->col_cunt) { len = (uint64_t)(render->col_cunt - col); }

    if (len == 0) { return LDG_ERR_AOK; }

    ui_tone_get(tone, &fg, &bg);

    return ldg_term_render_text_draw(render, col, row, text, len, &fg, &bg);
}

uint32_t dangem_ui_ch_put(ldg_term_render_t *render, uint32_t col, uint32_t row, char ch, uint32_t tone)
{
    char text[DANGEM_UI_HALF] = LDG_ARR_ZERO_INIT;

    text[0] = ch;

    return dangem_ui_text_put(render, col, row, text, tone);
}

uint32_t dangem_ui_row_fill(ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t col_cunt, uint32_t tone)
{
    uint32_t at = 0;
    uint32_t ret = 0;

    for (at = 0; at < col_cunt; at++)
    {
        ret = dangem_ui_ch_put(render, col + at, row, DANGEM_UI_BLANK, tone);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return LDG_ERR_AOK;
}

uint32_t dangem_ui_frame_draw(ldg_term_render_t *render, const ldg_term_rect_t *rect)
{
    uint32_t rhs = 0;
    uint32_t base = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || rect == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (rect->col_cunt < DANGEM_UI_HALF || rect->row_cunt < DANGEM_UI_HALF) { return LDG_ERR_AOK; }

    rhs = rect->col + rect->col_cunt - DANGEM_UI_STEP;
    base = rect->row + rect->row_cunt - DANGEM_UI_STEP;

    for (at = rect->col; ret == LDG_ERR_AOK && at <= rhs; at++)
    {
        ret = dangem_ui_ch_put(render, at, rect->row, (at == rect->col || at == rhs) ? DANGEM_UI_CORNER : DANGEM_UI_EDGE_FLAT, DANGEM_UI_TONE_EDGE);
        if (ret == LDG_ERR_AOK) { ret = dangem_ui_ch_put(render, at, base, (at == rect->col || at == rhs) ? DANGEM_UI_CORNER : DANGEM_UI_EDGE_FLAT, DANGEM_UI_TONE_EDGE); }
    }

    for (at = rect->row + DANGEM_UI_STEP; ret == LDG_ERR_AOK && at < base; at++)
    {
        ret = dangem_ui_ch_put(render, rect->col, at, DANGEM_UI_EDGE_TALL, DANGEM_UI_TONE_EDGE);
        if (ret == LDG_ERR_AOK) { ret = dangem_ui_ch_put(render, rhs, at, DANGEM_UI_EDGE_TALL, DANGEM_UI_TONE_EDGE); }
    }

    return ret;
}

uint32_t dangem_ui_vline_draw(ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t row_cunt)
{
    uint32_t at = 0;
    uint32_t ret = 0;

    for (at = 0; ret == LDG_ERR_AOK && at < row_cunt; at++) { ret = dangem_ui_ch_put(render, col, row + at, DANGEM_UI_EDGE_TALL, DANGEM_UI_TONE_EDGE); }

    return ret;
}

uint32_t dangem_ui_date_get(char *dst, uint64_t cap)
{
    const char *const day_name[LDG_TIME_DAYS_PER_WEEK] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
    const char *const month_name[LDG_TIME_MONTHS_PER_YEAR] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    ldg_time_cal_t cal = LDG_STRUCT_ZERO_INIT;
    uint64_t len = 0;
    uint32_t day = 0;
    uint32_t hour = 0;
    uint32_t minute = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(dst == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    dst[0] = LDG_STR_TERM;

    ret = ldg_time_cal_get(ldg_time_epoch_ms_get() / (uint64_t)LDG_MS_PER_SEC, &cal);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (cal.weekday >= LDG_TIME_DAYS_PER_WEEK || cal.month < LDG_TIME_MONTH_JANUARY || cal.month > LDG_TIME_MONTH_DECEMBER) { return LDG_ERR_RANGE; }

    day = (uint32_t)cal.day;
    hour = (uint32_t)cal.hour % DANGEM_UI_HOURS_PER_HALF;
    minute = (uint32_t)cal.minute;

    if (hour == 0) { hour = DANGEM_UI_HOURS_PER_HALF; }

    return ldg_str_fmt(dst, cap, &len, DANGEM_UI_DATE_FMT, day_name[cal.weekday], month_name[cal.month - LDG_TIME_MONTH_JANUARY], day, (uint32_t)cal.year, hour, minute / LDG_BASE_DEC, minute % LDG_BASE_DEC, (uint32_t)cal.hour < DANGEM_UI_HOURS_PER_HALF ? DANGEM_UI_AM : DANGEM_UI_PM);
}

uint32_t dangem_ui_gauge_draw(ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t col_cunt, const char *label, uint64_t val, uint64_t max, uint32_t tick_cunt, const char *unit)
{
    char text[DANGEM_UI_TEXT_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t axis_col = col + DANGEM_UI_GAUGE_LABEL_W;
    uint64_t len = 0;
    uint32_t axis_w = 0;
    uint32_t fill = 0;
    uint32_t tick = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || label == 0x0 || unit == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (col_cunt < DANGEM_UI_GAUGE_LABEL_W + DANGEM_UI_GAUGE_TAIL_W + DANGEM_UI_GAUGE_AXIS_MIN || tick_cunt == 0 || max == 0) { return LDG_ERR_AOK; }

    axis_w = col_cunt - DANGEM_UI_GAUGE_LABEL_W - DANGEM_UI_GAUGE_TAIL_W;

    for (tick = 0; ret == LDG_ERR_AOK && tick <= tick_cunt; tick++)
    {
        at = axis_col + (uint32_t)(((uint64_t)tick * (uint64_t)(axis_w - DANGEM_UI_STEP)) / (uint64_t)tick_cunt);
        ret = ldg_str_fmt(text, (uint64_t)sizeof(text), &len, "%u64", (max * (uint64_t)tick) / (uint64_t)tick_cunt);
        if (ret == LDG_ERR_AOK) { ret = dangem_ui_text_put(render, at, row + DANGEM_UI_GAUGE_ROW_TICK, text, DANGEM_UI_TONE_DIM); }
    }

    if (ret == LDG_ERR_AOK) { ret = ldg_str_fmt(text, (uint64_t)sizeof(text), &len, "%.8s :", label); }

    if (ret == LDG_ERR_AOK) { ret = dangem_ui_text_put(render, col, row + DANGEM_UI_GAUGE_ROW_AXIS, text, DANGEM_UI_TONE_NORMAL); }

    for (at = 0; ret == LDG_ERR_AOK && at < axis_w; at++) { ret = dangem_ui_ch_put(render, axis_col + at, row + DANGEM_UI_GAUGE_ROW_AXIS, DANGEM_UI_AXIS, DANGEM_UI_TONE_EDGE); }

    for (tick = 0; ret == LDG_ERR_AOK && tick <= tick_cunt; tick++)
    {
        at = axis_col + (uint32_t)(((uint64_t)tick * (uint64_t)(axis_w - DANGEM_UI_STEP)) / (uint64_t)tick_cunt);
        ret = dangem_ui_ch_put(render, at, row + DANGEM_UI_GAUGE_ROW_AXIS, DANGEM_UI_TICK, DANGEM_UI_TONE_EDGE);
    }

    fill = val >= max ? axis_w : (uint32_t)((val * (uint64_t)axis_w) / max);

    for (at = 0; ret == LDG_ERR_AOK && at < fill; at++) { ret = dangem_ui_ch_put(render, axis_col + at, row + DANGEM_UI_GAUGE_ROW_BAR, DANGEM_UI_BAR, DANGEM_UI_TONE_FILL); }

    if (ret == LDG_ERR_AOK) { ret = ldg_str_fmt(text, (uint64_t)sizeof(text), &len, "%u64 %.15s", val, unit); }

    if (ret == LDG_ERR_AOK) { ret = dangem_ui_text_put(render, axis_col + axis_w + DANGEM_UI_GAUGE_GAP, row + DANGEM_UI_GAUGE_ROW_BAR, text, DANGEM_UI_TONE_NORMAL); }

    return ret;
}
