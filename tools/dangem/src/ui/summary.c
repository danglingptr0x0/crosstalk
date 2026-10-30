#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/fmt.h>
#include <dangling/str/str.h>
#include <dangling/term/layout.h>
#include <dangling/term/render.h>

#include "compile/compile.h"
#include "ui/summary.h"
#include "ui/ui.h"

#define DANGEM_UI_SUMMARY_TITLE "DANGLING Ecosystem Manager r0.1"
#define DANGEM_UI_SUMMARY_KEYS "F2=Launch - F5=Rebuild - Esc=Menu - F10=Quit"
#define DANGEM_UI_SUMMARY_FACT_COL 1
#define DANGEM_UI_SUMMARY_FACT_ROW 3
#define DANGEM_UI_SUMMARY_BOX_COL 36
#define DANGEM_UI_SUMMARY_BOX_ROW 3
#define DANGEM_UI_SUMMARY_BOX_ROWS 9
#define DANGEM_UI_SUMMARY_BOX_PAD 2
#define DANGEM_UI_SUMMARY_GAUGE_ROW 13
#define DANGEM_UI_SUMMARY_GAUGE_GAP 4
#define DANGEM_UI_SUMMARY_TICK_CUNT 6
#define DANGEM_UI_SUMMARY_TIME_MIN 60
#define DANGEM_UI_SUMMARY_TIME_HEADROOM 2
#define DANGEM_UI_SUMMARY_COL_MIN 72
#define DANGEM_UI_SUMMARY_ROW_MIN 22
#define DANGEM_UI_SUMMARY_SUB_ROW 1
#define DANGEM_UI_SUMMARY_TOP_ROW 0
#define DANGEM_UI_SUMMARY_LINE_CAP 256
#define DANGEM_UI_SUMMARY_ELAPSED_NONE 0

static uint32_t summary_box_line(ldg_term_render_t *render, uint32_t box_col, uint32_t row, uint32_t col_cunt, const char *text, uint32_t tone)
{
    char line[DANGEM_UI_SUMMARY_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t width = col_cunt - DANGEM_UI_SUMMARY_BOX_PAD - DANGEM_UI_SUMMARY_BOX_PAD;
    uint32_t pos = 0;

    if (width >= (uint32_t)sizeof(line)) { width = (uint32_t)sizeof(line) - LDG_STR_TERM_SIZE; }

    for (pos = 0; pos < width && text[pos] != LDG_STR_TERM && text[pos] != '\n'; pos++) { line[pos] = (text[pos] < ' ' || text[pos] > '~') ? ' ' : text[pos]; }

    line[pos] = LDG_STR_TERM;

    return dangem_ui_text_put(render, box_col + DANGEM_UI_SUMMARY_BOX_PAD, row, line, tone);
}

static uint32_t summary_verdict_draw(ldg_term_render_t *render, const dangem_compile_t *build, uint32_t box_col, uint32_t col_cunt)
{
    char text[DANGEM_UI_SUMMARY_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t cap = (uint64_t)sizeof(build->fail_text);
    uint64_t off = 0;
    uint64_t idx = 0;
    uint64_t len = 0;
    uint32_t row = DANGEM_UI_SUMMARY_BOX_ROW + DANGEM_UI_STEP;
    uint32_t last = DANGEM_UI_SUMMARY_BOX_ROW + DANGEM_UI_SUMMARY_BOX_ROWS - DANGEM_UI_HALF;
    uint32_t ret = 0;

    if (build->phase == DANGEM_COMPILE_PHASE_COMPILE)
    {
        ret = ldg_str_fmt(text, (uint64_t)sizeof(text), &len, "Compiling %u32 of %u32 files.", build->src_done, build->src_total);
        if (ret != LDG_ERR_AOK) { return ret; }

        return summary_box_line(render, box_col, row, col_cunt, text, DANGEM_UI_TONE_NORMAL);
    }

    if (build->phase == DANGEM_COMPILE_PHASE_LINK) { return summary_box_line(render, box_col, row, col_cunt, "Linking.", DANGEM_UI_TONE_NORMAL); }

    if (build->phase == DANGEM_COMPILE_PHASE_DONE)
    {
        ret = ldg_str_fmt(text, (uint64_t)sizeof(text), &len, "Crosstalk %.63s is ready.", build->release);
        if (ret == LDG_ERR_AOK) { ret = summary_box_line(render, box_col, row, col_cunt, "Build succeeded.", DANGEM_UI_TONE_BRIGHT); }

        if (ret == LDG_ERR_AOK) { ret = summary_box_line(render, box_col, row + DANGEM_UI_STEP, col_cunt, text, DANGEM_UI_TONE_NORMAL); }

        if (ret == LDG_ERR_AOK) { ret = summary_box_line(render, box_col, row + DANGEM_UI_HALF + DANGEM_UI_STEP, col_cunt, "Press F2 to launch.", DANGEM_UI_TONE_NORMAL); }

        return ret;
    }

    if (build->phase != DANGEM_COMPILE_PHASE_FAIL) { return summary_box_line(render, box_col, row, col_cunt, "No build has run yet. Press F5.", DANGEM_UI_TONE_NORMAL); }

    ret = summary_box_line(render, box_col, row, col_cunt, "Build failed:", DANGEM_UI_TONE_BRIGHT);
    if (ret == LDG_ERR_AOK) { ret = summary_box_line(render, box_col, row + DANGEM_UI_STEP, col_cunt, build->fail_src, DANGEM_UI_TONE_NORMAL); }

    row = row + DANGEM_UI_HALF;

    while (ret == LDG_ERR_AOK && row <= last && off < cap && build->fail_text[off] != LDG_STR_TERM)
    {
        ret = summary_box_line(render, box_col, row, col_cunt, build->fail_text + off, DANGEM_UI_TONE_DIM);

        if (ldg_str_char_find(build->fail_text + off, '\n', cap - off, &idx) == LDG_ERR_AOK) { off = off + idx + DANGEM_UI_STEP; }
        else { off = cap; }

        row = row + DANGEM_UI_STEP;
    }

    return ret;
}

uint32_t dangem_ui_summary_draw(ldg_term_render_t *render, const dangem_compile_t *build, uint64_t now_ms)
{
    char text[DANGEM_UI_SUMMARY_LINE_CAP] = LDG_ARR_ZERO_INIT;
    char date[DANGEM_UI_DATE_CAP] = LDG_ARR_ZERO_INIT;
    ldg_term_rect_t box = LDG_STRUCT_ZERO_INIT;
    uint64_t end_ms = 0;
    uint64_t elapsed = 0;
    uint64_t scale = 0;
    uint64_t len = 0;
    uint32_t row = DANGEM_UI_SUMMARY_FACT_ROW;
    uint32_t box_col = DANGEM_UI_SUMMARY_BOX_COL;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || build == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (build->phase == DANGEM_COMPILE_PHASE_FAIL) { box_col = DANGEM_UI_SUMMARY_FACT_COL; }

    if (render->col_cunt < DANGEM_UI_SUMMARY_COL_MIN || render->row_cunt < DANGEM_UI_SUMMARY_ROW_MIN) { return dangem_ui_text_put(render, DANGEM_UI_ORIG_COL, DANGEM_UI_ORIG_ROW, DANGEM_UI_SMALL_TEXT, DANGEM_UI_TONE_BRIGHT); }

    ret = dangem_ui_text_put(render, DANGEM_UI_SUMMARY_FACT_COL, DANGEM_UI_SUMMARY_TOP_ROW, DANGEM_UI_SUMMARY_TITLE, DANGEM_UI_TONE_BRIGHT);

    if (ret == LDG_ERR_AOK && dangem_ui_date_get(date, (uint64_t)sizeof(date)) == LDG_ERR_AOK) { ret = dangem_ui_text_put(render, render->col_cunt - (uint32_t)strlen(date) - DANGEM_UI_STEP, DANGEM_UI_SUMMARY_TOP_ROW, date, DANGEM_UI_TONE_NORMAL); }

    if (ret == LDG_ERR_AOK) { ret = ldg_str_fmt(text, (uint64_t)sizeof(text), &len, "Crosstalk %.63s", build->release); }

    if (ret == LDG_ERR_AOK) { ret = dangem_ui_text_put(render, DANGEM_UI_SUMMARY_FACT_COL, DANGEM_UI_SUMMARY_SUB_ROW, text, DANGEM_UI_TONE_NORMAL); }

    if (box_col == DANGEM_UI_SUMMARY_BOX_COL)
    {
        if (ret == LDG_ERR_AOK) { ret = ldg_str_fmt(text, (uint64_t)sizeof(text), &len, "Release: %.31s", build->release); }

        if (ret == LDG_ERR_AOK) { ret = dangem_ui_text_put(render, DANGEM_UI_SUMMARY_FACT_COL, row, text, DANGEM_UI_TONE_NORMAL); }

        row = row + DANGEM_UI_STEP;
        if (ret == LDG_ERR_AOK) { ret = ldg_str_fmt(text, (uint64_t)sizeof(text), &len, "Compiler: %.20s", build->cc_ok == LDG_TRUTH_TRUE ? "bundled clang" : "missing"); }

        if (ret == LDG_ERR_AOK) { ret = dangem_ui_text_put(render, DANGEM_UI_SUMMARY_FACT_COL, row, text, DANGEM_UI_TONE_NORMAL); }

        row = row + DANGEM_UI_STEP;
        if (ret == LDG_ERR_AOK) { ret = ldg_str_fmt(text, (uint64_t)sizeof(text), &len, "Jobs: %u32", build->job_cunt); }

        if (ret == LDG_ERR_AOK) { ret = dangem_ui_text_put(render, DANGEM_UI_SUMMARY_FACT_COL, row, text, DANGEM_UI_TONE_NORMAL); }

        row = row + DANGEM_UI_STEP;
        if (ret == LDG_ERR_AOK) { ret = ldg_str_fmt(text, (uint64_t)sizeof(text), &len, "Files: %u32", build->src_total); }

        if (ret == LDG_ERR_AOK) { ret = dangem_ui_text_put(render, DANGEM_UI_SUMMARY_FACT_COL, row, text, DANGEM_UI_TONE_NORMAL); }
    }

    box.col = box_col;
    box.row = DANGEM_UI_SUMMARY_BOX_ROW;
    box.col_cunt = render->col_cunt - box_col - DANGEM_UI_STEP;
    box.row_cunt = DANGEM_UI_SUMMARY_BOX_ROWS;

    if (ret == LDG_ERR_AOK) { ret = dangem_ui_frame_draw(render, &box); }

    if (ret == LDG_ERR_AOK) { ret = summary_verdict_draw(render, build, box_col, box.col_cunt); }

    end_ms = dangem_compile_busy_is(build) == LDG_TRUTH_TRUE ? now_ms : build->end_ms;
    elapsed = end_ms > build->start_ms ? (end_ms - build->start_ms) / (uint64_t)LDG_MS_PER_SEC : (uint64_t)DANGEM_UI_SUMMARY_ELAPSED_NONE;
    scale = (build->prev_ms / (uint64_t)LDG_MS_PER_SEC) * (uint64_t)DANGEM_UI_SUMMARY_TIME_HEADROOM;

    if (scale < (uint64_t)DANGEM_UI_SUMMARY_TIME_MIN) { scale = (uint64_t)DANGEM_UI_SUMMARY_TIME_MIN; }

    while (elapsed > scale) { scale = scale * (uint64_t)DANGEM_UI_SUMMARY_TIME_HEADROOM; }

    if (ret == LDG_ERR_AOK) { ret = dangem_ui_gauge_draw(render, DANGEM_UI_SUMMARY_FACT_COL, DANGEM_UI_SUMMARY_GAUGE_ROW, render->col_cunt - DANGEM_UI_HALF, "Files", (uint64_t)build->src_done, (uint64_t)build->src_total, DANGEM_UI_SUMMARY_TICK_CUNT, "done"); }

    if (ret == LDG_ERR_AOK) { ret = dangem_ui_gauge_draw(render, DANGEM_UI_SUMMARY_FACT_COL, DANGEM_UI_SUMMARY_GAUGE_ROW + DANGEM_UI_SUMMARY_GAUGE_GAP, render->col_cunt - DANGEM_UI_HALF, "Time", elapsed, scale, DANGEM_UI_SUMMARY_TICK_CUNT, "s"); }

    if (ret == LDG_ERR_AOK) { ret = dangem_ui_text_put(render, DANGEM_UI_SUMMARY_FACT_COL, render->row_cunt - DANGEM_UI_STEP, DANGEM_UI_SUMMARY_KEYS, DANGEM_UI_TONE_NORMAL); }

    return ret;
}
