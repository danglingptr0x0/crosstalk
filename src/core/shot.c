#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/fmt/bmp.h>
#include <dangling/io/file.h>
#include <dangling/mem/alloc.h>
#include <dangling/str/fmt.h>
#include <dangling/str/str.h>
#include <dangling/term/render.h>
#include <dangling/term/term.h>

#include "core/shot.h"

#define SHOT_GLYPH_END (LDG_TERM_RENDER_GLYPH_FIRST + LDG_TERM_RENDER_GLYPH_CUNT)
#define SHOT_GLYPH_MSB 0x80U
#define SHOT_CH_EMPTY 0
#define SHOT_DIO_ALIGN (4ULL * LDG_KIB)
#define SHOT_NAME_FMT "shot_%X64_%u64.bmp"
#define SHOT_FILE_MODE 0644
#define SHOT_STEP 1

static uint64_t shot_align_get(uint64_t val)
{
    return (val + SHOT_DIO_ALIGN - SHOT_STEP) & ~(SHOT_DIO_ALIGN - SHOT_STEP);
}

static void shot_cell_raster(uint8_t *rgb, uint64_t stride, uint32_t px_x, uint32_t px_y, const ldg_term_cell_t *cell)
{
    const uint8_t *fg = cell->fg;
    const uint8_t *bg = cell->bg;
    const uint8_t *ink = 0x0;
    uint8_t *at = 0x0;
    uint32_t ch = cell->ch;
    uint32_t row = 0;
    uint32_t col = 0;
    uint8_t bits = 0;

    if ((cell->attr & LDG_TERM_ATTR_REVERSE) != 0)
    {
        fg = cell->bg;
        bg = cell->fg;
    }

    if (ch == SHOT_CH_EMPTY) { ch = LDG_TERM_RENDER_GLYPH_FIRST; }
    else if (ch < LDG_TERM_RENDER_GLYPH_FIRST || ch >= SHOT_GLYPH_END) { ch = (uint32_t)LDG_ASCII_NONPRINTABLE_CHAR; }

    for (row = 0; row < LDG_TERM_RENDER_GLYPH_ROW_CUNT; row = row + SHOT_STEP)
    {
        bits = LDG_TERM_RENDER_GLYPH_MASK[ch - LDG_TERM_RENDER_GLYPH_FIRST][row];
        at = rgb + (uint64_t)(px_y + row) * stride + (uint64_t)px_x * LDG_FMT_BMP_SRC_CHANNEL_CUNT;

        for (col = 0; col < LDG_TERM_RENDER_GLYPH_COL_CUNT; col = col + SHOT_STEP)
        {
            ink = (bits & (SHOT_GLYPH_MSB >> col)) != 0 ? fg : bg;
            memcpy(at, ink, LDG_FMT_BMP_SRC_CHANNEL_CUNT);
            at = at + LDG_FMT_BMP_SRC_CHANNEL_CUNT;
        }
    }
}

static uint32_t shot_file_wr(const char *path, uint8_t *data, uint64_t len, uint64_t cap)
{
    ldg_io_file_t *file = 0x0;
    uint64_t span = shot_align_get(len);
    uint64_t wrote = 0;
    uint32_t ret = 0;

    if (span > cap) { return LDG_ERR_FULL; }

    memset(data + len, 0x0, span - len);

    ret = ldg_io_file_open(path, LDG_IO_WR_ONLY | LDG_IO_CREATE | LDG_IO_TRUNCATE | LDG_IO_DIRECT, SHOT_FILE_MODE, &file);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_wr(file, data, span, &wrote);
    if (ret == LDG_ERR_AOK && wrote != span) { ret = LDG_ERR_IO_WR; }

    if (ret == LDG_ERR_AOK) { ret = ldg_io_file_truncate(file, len); }

    if (ldg_io_file_close(file) != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    return ret;
}

uint32_t crosstalk_core_shot_create(crosstalk_core_shot_t *shot, uint32_t col_max, uint32_t row_max)
{
    uint64_t bmp_size = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(shot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(shot, 0x0, sizeof(crosstalk_core_shot_t));

    if (col_max == 0 || row_max == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    shot->col_max = col_max;
    shot->row_max = row_max;
    shot->rgb_cap = (uint64_t)col_max * LDG_TERM_RENDER_GLYPH_COL_CUNT * (uint64_t)row_max * LDG_TERM_RENDER_GLYPH_ROW_CUNT * LDG_FMT_BMP_SRC_CHANNEL_CUNT;

    ret = ldg_fmt_bmp_size_get(col_max * LDG_TERM_RENDER_GLYPH_COL_CUNT, row_max * LDG_TERM_RENDER_GLYPH_ROW_CUNT, LDG_FMT_BMP_BPP_RGB, &bmp_size);
    if (ret != LDG_ERR_AOK) { return ret; }

    shot->bmp_cap = shot_align_get(bmp_size);

    ret = ldg_mem_alloc(shot->rgb_cap, (void **)&shot->rgb);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        shot->rgb = 0x0;
        return ret;
    }

    ret = ldg_mem_alloc(shot->bmp_cap + SHOT_DIO_ALIGN, (void **)&shot->bmp_raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        shot->bmp_raw = 0x0;
        if (ldg_mem_dealloc(shot->rgb) == LDG_ERR_AOK) { shot->rgb = 0x0; }

        return ret;
    }

    shot->bmp = (uint8_t *)(uintptr_t)shot_align_get((uint64_t)(uintptr_t)shot->bmp_raw);
    shot->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_core_shot_destroy(crosstalk_core_shot_t *shot)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(shot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (shot->rgb != 0x0 && ldg_mem_dealloc(shot->rgb) != LDG_ERR_AOK) { ret = LDG_ERR_INVALID; }

    if (shot->bmp_raw != 0x0 && ldg_mem_dealloc(shot->bmp_raw) != LDG_ERR_AOK) { ret = LDG_ERR_INVALID; }

    memset(shot, 0x0, sizeof(crosstalk_core_shot_t));

    return ret;
}

uint32_t crosstalk_core_shot_capture(crosstalk_core_shot_t *shot, const ldg_term_render_t *render, const ldg_term_rect_t *rect, uint64_t win_id, uint64_t now_ms, char *out_name, uint64_t name_cap, uint64_t *out_len)
{
    ldg_term_cell_t cell = LDG_STRUCT_ZERO_INIT;
    uint64_t stride = 0;
    uint64_t bmp_len = 0;
    uint64_t name_len = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t row = 0;
    uint32_t col = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(shot == 0x0 || render == 0x0 || rect == 0x0 || out_name == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (name_cap == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out_len = 0;
    out_name[0] = LDG_STR_TERM;

    if (shot->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (rect->col_cunt == 0 || rect->row_cunt == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (rect->col_cunt > shot->col_max || rect->row_cunt > shot->row_max) { return LDG_ERR_BOUNDS; }

    width = rect->col_cunt * LDG_TERM_RENDER_GLYPH_COL_CUNT;
    height = rect->row_cunt * LDG_TERM_RENDER_GLYPH_ROW_CUNT;
    stride = (uint64_t)width * LDG_FMT_BMP_SRC_CHANNEL_CUNT;

    for (row = 0; row < rect->row_cunt; row = row + SHOT_STEP) { for (col = 0; col < rect->col_cunt; col = col + SHOT_STEP)
        {
            ret = ldg_term_render_cell_get(render, rect->col + col, rect->row + row, &cell);
            if (ret != LDG_ERR_AOK) { return ret; }

            shot_cell_raster(shot->rgb, stride, col * LDG_TERM_RENDER_GLYPH_COL_CUNT, row * LDG_TERM_RENDER_GLYPH_ROW_CUNT, &cell);
        }
    }

    ret = ldg_fmt_bmp_encode(shot->rgb, width, height, stride, LDG_FMT_BMP_BPP_RGB, shot->bmp, shot->bmp_cap, &bmp_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_str_fmt(out_name, name_cap, &name_len, SHOT_NAME_FMT, win_id, now_ms);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = shot_file_wr(out_name, shot->bmp, bmp_len, shot->bmp_cap);
    if (ret != LDG_ERR_AOK) { return ret; }

    *out_len = bmp_len;

    return LDG_ERR_AOK;
}
