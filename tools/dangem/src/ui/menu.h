#ifndef DANGEM_UI_MENU_H
#define DANGEM_UI_MENU_H

#include <stdint.h>

#include <dangling/term/render.h>

#define DANGEM_UI_MENU_LINE_CAP 128

typedef struct dangem_ui_menu_view
{
    char launch[DANGEM_UI_MENU_LINE_CAP];
    char build[DANGEM_UI_MENU_LINE_CAP];
    char deps[DANGEM_UI_MENU_LINE_CAP];
    char notice[DANGEM_UI_MENU_LINE_CAP];
    uint8_t build_alert;
    uint8_t pudding[7];
} dangem_ui_menu_view_t;

uint32_t dangem_ui_menu_draw(ldg_term_render_t *render, const dangem_ui_menu_view_t *view);

#endif
