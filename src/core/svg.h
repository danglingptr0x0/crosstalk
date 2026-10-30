#ifndef CROSSTALK_CORE_SVG_H
#define CROSSTALK_CORE_SVG_H

#include <stdint.h>

#define CROSSTALK_CORE_SVG_VERT_CAP 2048
#define CROSSTALK_CORE_SVG_SUB_CAP 64
#define CROSSTALK_CORE_SVG_ARC_SEG_MAX 64
#define CROSSTALK_CORE_SVG_CUBIC_SEG 16
#define CROSSTALK_CORE_SVG_ARC_STEP_RAD 0.20

typedef struct crosstalk_core_svg_sub
{
    uint32_t first;
    uint32_t cunt;
    uint8_t closed;
    uint8_t pudding[3];
} crosstalk_core_svg_sub_t;

typedef struct crosstalk_core_svg_path
{
    double vert_x[CROSSTALK_CORE_SVG_VERT_CAP];
    double vert_y[CROSSTALK_CORE_SVG_VERT_CAP];
    crosstalk_core_svg_sub_t sub[CROSSTALK_CORE_SVG_SUB_CAP];
    uint32_t vert_cunt;
    uint32_t sub_cunt;
} crosstalk_core_svg_path_t;

uint32_t crosstalk_core_svg_path_parse(const char *desc, double off_x, double off_y, crosstalk_core_svg_path_t *out);
uint32_t crosstalk_core_svg_path_bounds_get(const crosstalk_core_svg_path_t *path, double *out_min_x, double *out_min_y, double *out_max_x, double *out_max_y);

#endif
