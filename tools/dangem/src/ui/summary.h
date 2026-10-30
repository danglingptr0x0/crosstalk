#ifndef DANGEM_UI_SUMMARY_H
#define DANGEM_UI_SUMMARY_H

#include <stdint.h>

#include <dangling/term/render.h>

#include "compile/compile.h"

uint32_t dangem_ui_summary_draw(ldg_term_render_t *render, const dangem_compile_t *build, uint64_t now_ms);

#endif
