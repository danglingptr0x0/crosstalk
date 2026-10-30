#include <stdint.h>
#include <math.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/math.h>

#include "core/svg.h"

#define CROSSTALK_CORE_SVG_MIDPOINT 0.5
#define CROSSTALK_CORE_SVG_CUBIC_TERM_CUNT 3.0
#define CROSSTALK_CORE_SVG_DIGIT_CUNT 10.0
#define CROSSTALK_CORE_SVG_SEG_MIN 1
#define CROSSTALK_CORE_SVG_SUB_LAST_OFF 1
#define CROSSTALK_CORE_SVG_EPS 1.0e-12
#define CROSSTALK_CORE_SVG_DIGIT_SET "0123456789"
#define CROSSTALK_CORE_SVG_DIGIT_LO 0
#define CROSSTALK_CORE_SVG_DIGIT_HI 9

static uint8_t crosstalk_core_svg_sep_is(char sym)
{
    if (sym == ' ' || sym == ',' || sym == '\n' || sym == '\r' || sym == '\t') { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t crosstalk_core_svg_digit_is(char sym)
{
    const char *digit = CROSSTALK_CORE_SVG_DIGIT_SET;

    if (sym >= digit[CROSSTALK_CORE_SVG_DIGIT_LO] && sym <= digit[CROSSTALK_CORE_SVG_DIGIT_HI]) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static void crosstalk_core_svg_sep_skip(const char **cursor)
{
    if (LDG_UNLIKELY(cursor == 0x0 || *cursor == 0x0)) { return; }

    while (**cursor != LDG_STR_TERM && crosstalk_core_svg_sep_is(**cursor) == LDG_TRUTH_TRUE) { (*cursor)++; }
}

static double crosstalk_core_svg_num_rd(const char **cursor)
{
    const char *digit = CROSSTALK_CORE_SVG_DIGIT_SET;
    double val = 0.0;
    double frac = 0.0;
    double scale = 1.0;
    double sign = 1.0;

    if (LDG_UNLIKELY(cursor == 0x0 || *cursor == 0x0)) { return val; }

    crosstalk_core_svg_sep_skip(cursor);
    if (**cursor == '-')
    {
        sign = -1.0;
        (*cursor)++;
    }
    else if (**cursor == '+') { (*cursor)++; }

    while (crosstalk_core_svg_digit_is(**cursor) == LDG_TRUTH_TRUE)
    {
        val = val * CROSSTALK_CORE_SVG_DIGIT_CUNT + (double)(**cursor - digit[CROSSTALK_CORE_SVG_DIGIT_LO]);
        (*cursor)++;
    }

    if (**cursor == '.')
    {
        (*cursor)++;
        while (crosstalk_core_svg_digit_is(**cursor) == LDG_TRUTH_TRUE)
        {
            scale = scale * CROSSTALK_CORE_SVG_DIGIT_CUNT;
            frac = frac * CROSSTALK_CORE_SVG_DIGIT_CUNT + (double)(**cursor - digit[CROSSTALK_CORE_SVG_DIGIT_LO]);
            (*cursor)++;
        }

        val = val + frac / scale;
    }

    return sign * val;
}

static uint8_t crosstalk_core_svg_flag_rd(const char **cursor)
{
    const char *digit = CROSSTALK_CORE_SVG_DIGIT_SET;
    uint8_t flag = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(cursor == 0x0 || *cursor == 0x0)) { return flag; }

    crosstalk_core_svg_sep_skip(cursor);
    if (crosstalk_core_svg_digit_is(**cursor) == LDG_TRUTH_TRUE)
    {
        if (**cursor != digit[CROSSTALK_CORE_SVG_DIGIT_LO]) { flag = LDG_TRUTH_TRUE; }

        (*cursor)++;
    }

    return flag;
}

static uint32_t crosstalk_core_svg_vert_push(crosstalk_core_svg_path_t *out, double vx, double vy)
{
    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (out->vert_cunt >= CROSSTALK_CORE_SVG_VERT_CAP) { return LDG_ERR_FULL; }

    out->vert_x[out->vert_cunt] = vx;
    out->vert_y[out->vert_cunt] = vy;
    out->vert_cunt++;
    if (out->sub_cunt > 0) { out->sub[out->sub_cunt - CROSSTALK_CORE_SVG_SUB_LAST_OFF].cunt++; }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_core_svg_sub_open(crosstalk_core_svg_path_t *out)
{
    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (out->sub_cunt >= CROSSTALK_CORE_SVG_SUB_CAP) { return LDG_ERR_FULL; }

    out->sub[out->sub_cunt].first = out->vert_cunt;
    out->sub[out->sub_cunt].cunt = 0;
    out->sub[out->sub_cunt].closed = LDG_TRUTH_FALSE;
    out->sub_cunt++;
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_core_svg_cubic_emit(crosstalk_core_svg_path_t *out, double x0, double y0, double cx0, double cy0, double cx1, double cy1, double x1, double y1)
{
    double step = 0.0;
    double omt = 0.0;
    double bx = 0.0;
    double by = 0.0;
    uint32_t ret = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 1; idx <= CROSSTALK_CORE_SVG_CUBIC_SEG; idx++)
    {
        step = (double)idx / (double)CROSSTALK_CORE_SVG_CUBIC_SEG;
        omt = 1.0 - step;
        bx = omt * omt * omt * x0 + CROSSTALK_CORE_SVG_CUBIC_TERM_CUNT * omt * omt * step * cx0 + CROSSTALK_CORE_SVG_CUBIC_TERM_CUNT * omt * step * step * cx1 + step * step * step * x1;
        by = omt * omt * omt * y0 + CROSSTALK_CORE_SVG_CUBIC_TERM_CUNT * omt * omt * step * cy0 + CROSSTALK_CORE_SVG_CUBIC_TERM_CUNT * omt * step * step * cy1 + step * step * step * y1;
        ret = crosstalk_core_svg_vert_push(out, bx, by);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_core_svg_arc_emit(crosstalk_core_svg_path_t *out, double x0, double y0, double rad_x, double rad_y, double rot_deg, uint8_t large, uint8_t sweep, double x1, double y1)
{
    double phi = rot_deg * LDG_MATH_DEG_TO_RAD;
    double cos_phi = cos(phi);
    double sin_phi = sin(phi);
    double dx2 = (x0 - x1) * CROSSTALK_CORE_SVG_MIDPOINT;
    double dy2 = (y0 - y1) * CROSSTALK_CORE_SVG_MIDPOINT;
    double x1p = cos_phi * dx2 + sin_phi * dy2;
    double y1p = -sin_phi * dx2 + cos_phi * dy2;
    double lambda = 0.0;
    double num = 0.0;
    double den = 0.0;
    double coef = 0.0;
    double cxp = 0.0;
    double cyp = 0.0;
    double ctr_x = 0.0;
    double ctr_y = 0.0;
    double ux = 0.0;
    double uy = 0.0;
    double vx = 0.0;
    double vy = 0.0;
    double theta = 0.0;
    double delta = 0.0;
    double ang = 0.0;
    double px = 0.0;
    double py = 0.0;
    uint32_t seg_cunt = 0;
    uint32_t ret = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    rad_x = fabs(rad_x);
    rad_y = fabs(rad_y);
    if (rad_x < CROSSTALK_CORE_SVG_EPS || rad_y < CROSSTALK_CORE_SVG_EPS) { return crosstalk_core_svg_vert_push(out, x1, y1); }

    lambda = (x1p * x1p) / (rad_x * rad_x) + (y1p * y1p) / (rad_y * rad_y);
    if (lambda > 1.0)
    {
        rad_x = rad_x * sqrt(lambda);
        rad_y = rad_y * sqrt(lambda);
    }

    num = rad_x * rad_x * rad_y * rad_y - rad_x * rad_x * y1p * y1p - rad_y * rad_y * x1p * x1p;
    den = rad_x * rad_x * y1p * y1p + rad_y * rad_y * x1p * x1p;
    if (num < 0.0) { num = 0.0; }

    if (den < CROSSTALK_CORE_SVG_EPS) { return crosstalk_core_svg_vert_push(out, x1, y1); }

    coef = sqrt(num / den);
    if (large == sweep) { coef = -coef; }

    cxp = coef * rad_x * y1p / rad_y;
    cyp = -coef * rad_y * x1p / rad_x;
    ctr_x = cos_phi * cxp - sin_phi * cyp + (x0 + x1) * CROSSTALK_CORE_SVG_MIDPOINT;
    ctr_y = sin_phi * cxp + cos_phi * cyp + (y0 + y1) * CROSSTALK_CORE_SVG_MIDPOINT;
    ux = (x1p - cxp) / rad_x;
    uy = (y1p - cyp) / rad_y;
    vx = (-x1p - cxp) / rad_x;
    vy = (-y1p - cyp) / rad_y;
    theta = atan2(uy, ux);
    delta = atan2(ux * vy - uy * vx, ux * vx + uy * vy);
    if (sweep == LDG_TRUTH_FALSE && delta > 0.0) { delta = delta - LDG_MATH_TAU; }

    if (sweep == LDG_TRUTH_TRUE && delta < 0.0) { delta = delta + LDG_MATH_TAU; }

    seg_cunt = (uint32_t)(fabs(delta) / CROSSTALK_CORE_SVG_ARC_STEP_RAD) + CROSSTALK_CORE_SVG_SEG_MIN;
    if (seg_cunt > CROSSTALK_CORE_SVG_ARC_SEG_MAX) { seg_cunt = CROSSTALK_CORE_SVG_ARC_SEG_MAX; }

    for (idx = 1; idx <= seg_cunt; idx++)
    {
        ang = theta + delta * ((double)idx / (double)seg_cunt);
        px = ctr_x + rad_x * cos(ang) * cos_phi - rad_y * sin(ang) * sin_phi;
        py = ctr_y + rad_x * cos(ang) * sin_phi + rad_y * sin(ang) * cos_phi;
        ret = crosstalk_core_svg_vert_push(out, px, py);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_svg_path_parse(const char *desc, double off_x, double off_y, crosstalk_core_svg_path_t *out)
{
    const char *cursor = desc;
    double cur_x = 0.0;
    double cur_y = 0.0;
    double start_x = 0.0;
    double start_y = 0.0;
    double arg[7] = LDG_ARR_ZERO_INIT;
    double ctrl_x = 0.0;
    double ctrl_y = 0.0;
    uint32_t ret = 0;
    char cmd = LDG_STR_TERM;
    char prev = LDG_STR_TERM;
    char prior = LDG_STR_TERM;
    uint8_t rel = LDG_TRUTH_FALSE;
    uint8_t large = LDG_TRUTH_FALSE;
    uint8_t sweep = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(desc == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->vert_cunt = 0;
    out->sub_cunt = 0;

    while (*cursor != LDG_STR_TERM)
    {
        crosstalk_core_svg_sep_skip(&cursor);
        if (*cursor == LDG_STR_TERM) { break; }

        if ((*cursor >= 'A' && *cursor <= 'Z') || (*cursor >= 'a' && *cursor <= 'z'))
        {
            cmd = *cursor;
            cursor++;
        }
        else { cmd = prev; }

        rel = (cmd >= 'a' && cmd <= 'z') ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
        prior = prev;
        prev = cmd;

        if (cmd == 'M' || cmd == 'm')
        {
            arg[0] = crosstalk_core_svg_num_rd(&cursor);
            arg[1] = crosstalk_core_svg_num_rd(&cursor);
            cur_x = rel == LDG_TRUTH_TRUE ? cur_x + arg[0] : arg[0];
            cur_y = rel == LDG_TRUTH_TRUE ? cur_y + arg[1] : arg[1];
            start_x = cur_x;
            start_y = cur_y;
            ret = crosstalk_core_svg_sub_open(out);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = crosstalk_core_svg_vert_push(out, cur_x + off_x, cur_y + off_y);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            prev = rel == LDG_TRUTH_TRUE ? 'l' : 'L';
        }
        else if (cmd == 'L' || cmd == 'l')
        {
            arg[0] = crosstalk_core_svg_num_rd(&cursor);
            arg[1] = crosstalk_core_svg_num_rd(&cursor);
            cur_x = rel == LDG_TRUTH_TRUE ? cur_x + arg[0] : arg[0];
            cur_y = rel == LDG_TRUTH_TRUE ? cur_y + arg[1] : arg[1];
            ret = crosstalk_core_svg_vert_push(out, cur_x + off_x, cur_y + off_y);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
        else if (cmd == 'H' || cmd == 'h')
        {
            arg[0] = crosstalk_core_svg_num_rd(&cursor);
            cur_x = rel == LDG_TRUTH_TRUE ? cur_x + arg[0] : arg[0];
            ret = crosstalk_core_svg_vert_push(out, cur_x + off_x, cur_y + off_y);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
        else if (cmd == 'V' || cmd == 'v')
        {
            arg[0] = crosstalk_core_svg_num_rd(&cursor);
            cur_y = rel == LDG_TRUTH_TRUE ? cur_y + arg[0] : arg[0];
            ret = crosstalk_core_svg_vert_push(out, cur_x + off_x, cur_y + off_y);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
        else if (cmd == 'C' || cmd == 'c')
        {
            arg[0] = crosstalk_core_svg_num_rd(&cursor);
            arg[1] = crosstalk_core_svg_num_rd(&cursor);
            arg[2] = crosstalk_core_svg_num_rd(&cursor);
            arg[3] = crosstalk_core_svg_num_rd(&cursor);
            arg[4] = crosstalk_core_svg_num_rd(&cursor);
            arg[5] = crosstalk_core_svg_num_rd(&cursor);
            if (rel == LDG_TRUTH_TRUE)
            {
                arg[0] = arg[0] + cur_x;
                arg[1] = arg[1] + cur_y;
                arg[2] = arg[2] + cur_x;
                arg[3] = arg[3] + cur_y;
                arg[4] = arg[4] + cur_x;
                arg[5] = arg[5] + cur_y;
            }

            ret = crosstalk_core_svg_cubic_emit(out, cur_x + off_x, cur_y + off_y, arg[0] + off_x, arg[1] + off_y, arg[2] + off_x, arg[3] + off_y, arg[4] + off_x, arg[5] + off_y);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ctrl_x = arg[2];
            ctrl_y = arg[3];
            cur_x = arg[4];
            cur_y = arg[5];
        }
        else if (cmd == 'S' || cmd == 's')
        {
            arg[2] = crosstalk_core_svg_num_rd(&cursor);
            arg[3] = crosstalk_core_svg_num_rd(&cursor);
            arg[4] = crosstalk_core_svg_num_rd(&cursor);
            arg[5] = crosstalk_core_svg_num_rd(&cursor);
            if (rel == LDG_TRUTH_TRUE)
            {
                arg[2] = arg[2] + cur_x;
                arg[3] = arg[3] + cur_y;
                arg[4] = arg[4] + cur_x;
                arg[5] = arg[5] + cur_y;
            }

            if (prior == 'C' || prior == 'c' || prior == 'S' || prior == 's')
            {
                arg[0] = cur_x + (cur_x - ctrl_x);
                arg[1] = cur_y + (cur_y - ctrl_y);
            }
            else
            {
                arg[0] = cur_x;
                arg[1] = cur_y;
            }

            ret = crosstalk_core_svg_cubic_emit(out, cur_x + off_x, cur_y + off_y, arg[0] + off_x, arg[1] + off_y, arg[2] + off_x, arg[3] + off_y, arg[4] + off_x, arg[5] + off_y);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ctrl_x = arg[2];
            ctrl_y = arg[3];
            cur_x = arg[4];
            cur_y = arg[5];
        }
        else if (cmd == 'A' || cmd == 'a')
        {
            arg[0] = crosstalk_core_svg_num_rd(&cursor);
            arg[1] = crosstalk_core_svg_num_rd(&cursor);
            arg[2] = crosstalk_core_svg_num_rd(&cursor);
            large = crosstalk_core_svg_flag_rd(&cursor);
            sweep = crosstalk_core_svg_flag_rd(&cursor);
            arg[5] = crosstalk_core_svg_num_rd(&cursor);
            arg[6] = crosstalk_core_svg_num_rd(&cursor);
            if (rel == LDG_TRUTH_TRUE)
            {
                arg[5] = arg[5] + cur_x;
                arg[6] = arg[6] + cur_y;
            }

            ret = crosstalk_core_svg_arc_emit(out, cur_x + off_x, cur_y + off_y, arg[0], arg[1], arg[2], large, sweep, arg[5] + off_x, arg[6] + off_y);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            cur_x = arg[5];
            cur_y = arg[6];
        }
        else if (cmd == 'Z' || cmd == 'z')
        {
            if (out->sub_cunt > 0) { out->sub[out->sub_cunt - CROSSTALK_CORE_SVG_SUB_LAST_OFF].closed = LDG_TRUTH_TRUE; }

            cur_x = start_x;
            cur_y = start_y;
        }
        else { return LDG_ERR_INVALID; }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_svg_path_bounds_get(const crosstalk_core_svg_path_t *path, double *out_min_x, double *out_min_y, double *out_max_x, double *out_max_y)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(path == 0x0 || out_min_x == 0x0 || out_min_y == 0x0 || out_max_x == 0x0 || out_max_y == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (path->vert_cunt == 0) { return LDG_ERR_EMPTY; }

    *out_min_x = path->vert_x[0];
    *out_max_x = path->vert_x[0];
    *out_min_y = path->vert_y[0];
    *out_max_y = path->vert_y[0];
    for (idx = 0; idx < path->vert_cunt; idx++)
    {
        if (path->vert_x[idx] < *out_min_x) { *out_min_x = path->vert_x[idx]; }

        if (path->vert_x[idx] > *out_max_x) { *out_max_x = path->vert_x[idx]; }

        if (path->vert_y[idx] < *out_min_y) { *out_min_y = path->vert_y[idx]; }

        if (path->vert_y[idx] > *out_max_y) { *out_max_y = path->vert_y[idx]; }
    }

    return LDG_ERR_AOK;
}
