#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <yder.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/str/str.h>
#include <dangling/sys/tty.h>
#include <dangling/term/input.h>
#include <dangling/term/render.h>
#include <dangling/term/sesh.h>
#include <dangling/time/time.h>

#include "mngr/mngr.h"

#define MAIN_EXIT_OK 0
#define MAIN_EXIT_FAIL 1
#define MAIN_STEP 1
#define MAIN_LOG_NAME "dangcsaemumngr"
#define MAIN_LOG_FILE "dangcsaemumngr.log"
#define MAIN_LOG_START "dangcsaemumngr logging started"
#define MAIN_TITLE "DANGCSA emulator manager"
#define MAIN_FRAME_MS 33

#define MAIN_LIST_W 32
#define MAIN_CAT_W 18
#define MAIN_LABEL_W 26
#define MAIN_HDR_ROW 0
#define MAIN_BODY_ROW 1
#define MAIN_CHROME_ROW_CUNT 2
#define MAIN_GAP 1
#define MAIN_NAME_CAP 48
#define MAIN_LINE_CAP 256
#define MAIN_LABEL_CAP 32
#define MAIN_NUM_CAP 16
#define MAIN_BLANK_CAP 32
#define MAIN_BLANK_CH ' '
#define MAIN_ORIG_COL 0
#define MAIN_CAP_NONE 0
#define MAIN_LABEL_FMT "%.31s"

#define MAIN_FG_LEVEL 0xFF
#define MAIN_BG_LEVEL 0x00
#define MAIN_BAR_LEVEL 0x40
#define MAIN_SEL_FG_LEVEL 0x00
#define MAIN_SEL_BG_LEVEL 0xC0

#define MAIN_MODE_LIST 0
#define MAIN_MODE_SETTINGS 1

#define MAIN_CAT_MACHINE 0
#define MAIN_CAT_DISPLAY 1
#define MAIN_CAT_INPUT 2
#define MAIN_CAT_STORAGE 3
#define MAIN_CAT_BOOT 4
#define MAIN_CAT_NET 5
#define MAIN_CAT_CUNT 6

#define MAIN_KIND_TEXT 0
#define MAIN_KIND_NUM 1
#define MAIN_KIND_REAL 2
#define MAIN_KIND_FLAG 3
#define MAIN_KIND_KEYS 4

#define MAIN_FIELD_MAX (CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX + CROSSTALK_EMU_LD_MAX + 1)

#define MAIN_CAT_MACHINE_TEXT "Machine"
#define MAIN_CAT_DISPLAY_TEXT "Display"
#define MAIN_CAT_INPUT_TEXT "Input and sound"
#define MAIN_CAT_STORAGE_TEXT "Storage"
#define MAIN_CAT_BOOT_TEXT "Boot images"
#define MAIN_CAT_NET_TEXT "Network"
#define MAIN_KEYS_SCAN_TEXT "Scancodes"
#define MAIN_KEYS_RAW_TEXT "Raw bytes"
#define MAIN_FLAG_ON_TEXT "[x]"
#define MAIN_FLAG_OFF_TEXT "[ ]"
#define MAIN_EMPTY "(none)"

#define MAIN_LIST_HINT "s Start; e Settings; n New machine; d Remove; r Rescan; q Quit"
#define MAIN_SETTINGS_HINT "Tab Next field; Left/Right Category; Space Toggle; Enter OK; Esc Cancel"

#define MAIN_KEY_START 's'
#define MAIN_KEY_EDIT 'e'
#define MAIN_KEY_NEW 'n'
#define MAIN_KEY_DROP 'd'
#define MAIN_KEY_RESCAN 'r'
#define MAIN_KEY_QUIT 'q'
#define MAIN_KEY_TOGGLE ' '
#define MAIN_KEY_POINT '.'
#define MAIN_KEY_MOD_CMD (LDG_TERM_MOD_CTRL | LDG_TERM_MOD_ALT | LDG_TERM_MOD_SUPER)

#define MAIN_LBL_NAME "Name:"
#define MAIN_LBL_CORE "Core:"
#define MAIN_LBL_RAM "Memory (MiB):"
#define MAIN_LBL_HZ "Clock (MHz):"
#define MAIN_LBL_NOTE "Note:"
#define MAIN_LBL_GAMMA "Gamma:"
#define MAIN_LBL_KEYS "Key delivery:"
#define MAIN_LBL_AUDIO "Sound on"
#define MAIN_LBL_SERIAL "Mirror serial to the log"
#define MAIN_LBL_PERSIST "Write disks back on exit"
#define MAIN_LBL_NET "Network endpoint:"

#define MAIN_TRACE_FMT "dangcsaemumngr: %s; err: %u\n"
#define MAIN_TRACE_NO_BUFF 0
#define MAIN_TRACE_ENTER "entered main; argc"
#define MAIN_TRACE_MEM_OK "memory up"
#define MAIN_TRACE_MNGR_OK "manager up; machines"
#define MAIN_TRACE_MEM "memory init failed"
#define MAIN_TRACE_LOGS "logging not started"
#define MAIN_TRACE_MNGR "manager init failed"
#define MAIN_TRACE_TERM "terminal init failed"
#define MAIN_TRACE_ALLOC "context allocation failed"
#define MAIN_NEW_NAME_FMT "machine%u"
#define MAIN_SUM_FMT "%.13s %.191s"
#define MAIN_SUM_HZ_FMT "Clock: %" PRIu64 " MHz"
#define MAIN_SUM_RAM_FMT "Memory: %" PRIu64 " MiB"
#define MAIN_SUM_DRIVE_FMT "Drive %u: %.191s"
#define MAIN_SUM_IMGS_FMT "Images: %u"
#define MAIN_SUM_NET_FMT "Network: %.191s"
#define MAIN_DRIVE_FMT "Drive %u:"
#define MAIN_IMG_FMT "Image %u:"
#define MAIN_UINT_FMT "%" PRIu64
#define MAIN_GAMMA_FMT "%.2f"
#define MAIN_HZ_PER_MHZ 1000000
#define MAIN_GAMMA_MIN 0.5
#define MAIN_GAMMA_MAX 4.0
#define MAIN_RAM_MIN_MIB 4
#define MAIN_RAM_MAX_MIB 512
#define MAIN_HZ_MIN_MHZ 1
#define MAIN_HZ_MAX_MHZ 2000

typedef struct main_edit
{
    crosstalk_mngr_machine_t machine;
    char ram_text[MAIN_NUM_CAP];
    char hz_text[MAIN_NUM_CAP];
    char gamma_text[MAIN_NUM_CAP];
    uint32_t cat;
    uint32_t field;
    uint32_t idx;
    uint8_t keys_raw;
    uint8_t live;
    uint8_t pudding[2];
} main_edit_t;

typedef struct main_field
{
    char label[MAIN_LABEL_CAP];
    char *text;
    uint8_t *flag;
    uint32_t cap;
    uint32_t kind;
} main_field_t;

typedef struct main_state
{
    crosstalk_mngr_ctx_t *mngr;
    main_edit_t *edit;
    uint32_t mode;
    uint32_t list_top;
    uint8_t quit;
    uint8_t launch_req;
    uint8_t pudding[6];
} main_state_t;

typedef struct main_term
{
    ldg_term_sesh_t *sesh;
    ldg_term_render_t *render;
    ldg_term_input_t *input;
} main_term_t;

typedef struct main_palette
{
    ldg_term_render_color_t fg;
    ldg_term_render_color_t bg;
    ldg_term_render_color_t bar;
    ldg_term_render_color_t sel_fg;
    ldg_term_render_color_t sel_bg;
} main_palette_t;

static uint32_t main_color_set(ldg_term_render_color_t *out, uint8_t level)
{
    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->red = level;
    out->green = level;
    out->blue = level;
    out->pudding[0] = 0;

    return LDG_ERR_AOK;
}

static uint32_t main_palette_get(main_palette_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = main_color_set(&out->fg, (uint8_t)MAIN_FG_LEVEL);
    if (ret == LDG_ERR_AOK) { ret = main_color_set(&out->bg, (uint8_t)MAIN_BG_LEVEL); }

    if (ret == LDG_ERR_AOK) { ret = main_color_set(&out->bar, (uint8_t)MAIN_BAR_LEVEL); }

    if (ret == LDG_ERR_AOK) { ret = main_color_set(&out->sel_fg, (uint8_t)MAIN_SEL_FG_LEVEL); }

    if (ret == LDG_ERR_AOK) { ret = main_color_set(&out->sel_bg, (uint8_t)MAIN_SEL_BG_LEVEL); }

    return ret;
}

static uint32_t main_text_put(ldg_term_render_t *render, uint32_t col, uint32_t row, const char *text, uint64_t cap, const ldg_term_render_color_t *fg, const ldg_term_render_color_t *bg)
{
    uint64_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || text == 0x0 || fg == 0x0 || bg == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (col >= render->col_cunt || row >= render->row_cunt) { return LDG_ERR_AOK; }

    ret = ldg_str_len_get(text, cap, &len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (len == 0) { return LDG_ERR_AOK; }

    if (len > (uint64_t)(render->col_cunt - col)) { len = (uint64_t)(render->col_cunt - col); }

    return ldg_term_render_text_draw(render, col, row, text, len, fg, bg);
}

static uint32_t main_span_fill(ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t width, const ldg_term_render_color_t *fg, const ldg_term_render_color_t *bg)
{
    char blank[MAIN_BLANK_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || fg == 0x0 || bg == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (col >= render->col_cunt || row >= render->row_cunt) { return LDG_ERR_AOK; }

    if (width > render->col_cunt - col) { width = render->col_cunt - col; }

    memset(&blank[0], MAIN_BLANK_CH, sizeof(blank));

    for (at = 0; at < width; at = at + (uint32_t)MAIN_BLANK_CAP)
    {
        uint32_t len = width - at < (uint32_t)MAIN_BLANK_CAP ? width - at : (uint32_t)MAIN_BLANK_CAP;

        ret = ldg_term_render_text_draw(render, col + at, row, &blank[0], (uint64_t)len, fg, bg);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t main_cat_name_get(uint32_t cat, const char **out)
{
    const char *name[MAIN_CAT_CUNT] = { MAIN_CAT_MACHINE_TEXT, MAIN_CAT_DISPLAY_TEXT, MAIN_CAT_INPUT_TEXT, MAIN_CAT_STORAGE_TEXT, MAIN_CAT_BOOT_TEXT, MAIN_CAT_NET_TEXT };

    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (cat >= (uint32_t)MAIN_CAT_CUNT) { return LDG_ERR_BOUNDS; }

    *out = name[cat];

    return LDG_ERR_AOK;
}

static uint32_t main_edit_begin(main_edit_t *edit, const crosstalk_mngr_ctx_t *mngr, uint32_t idx)
{
    if (LDG_UNLIKELY(edit == 0x0 || mngr == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (idx >= mngr->cunt) { return LDG_ERR_BOUNDS; }

    memset(edit, 0x0, sizeof(*edit));

    memcpy(&edit->machine, &mngr->machine[idx], sizeof(edit->machine));

    edit->idx = idx;
    edit->cat = (uint32_t)MAIN_CAT_MACHINE;
    edit->keys_raw = edit->machine.key_mode == (uint32_t)CROSSTALK_EMU_INPUT_MODE_SCAN ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE;

    if (snprintf(edit->ram_text, sizeof(edit->ram_text), MAIN_UINT_FMT, edit->machine.ram_bytes / (uint64_t)LDG_MIB) < 0) { return LDG_ERR_STR_FMT; }

    if (snprintf(edit->hz_text, sizeof(edit->hz_text), MAIN_UINT_FMT, edit->machine.f_op_hz / (uint64_t)MAIN_HZ_PER_MHZ) < 0) { return LDG_ERR_STR_FMT; }

    if (snprintf(edit->gamma_text, sizeof(edit->gamma_text), MAIN_GAMMA_FMT, edit->machine.gamma) < 0) { return LDG_ERR_STR_FMT; }

    edit->live = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint64_t main_uint_clamped_get(const char *text, uint64_t cap, uint64_t min_val, uint64_t max_val, uint64_t fallback)
{
    uint64_t val = 0;

    if (text == 0x0) { return fallback; }

    if (ldg_str_uint_parse(text, (uint32_t)LDG_BASE_DEC, cap, &val) != LDG_ERR_AOK) { return fallback; }

    if (val < min_val) { return min_val; }

    if (val > max_val) { return max_val; }

    return val;
}

static uint32_t main_edit_commit(main_edit_t *edit, crosstalk_mngr_ctx_t *mngr)
{
    double gamma = MAIN_GAMMA_MIN;
    char *gamma_end = 0x0;
    uint64_t ram_mib = 0;
    uint64_t hz_mhz = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(edit == 0x0 || mngr == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (edit->idx >= mngr->cunt) { return LDG_ERR_BOUNDS; }

    edit->machine.ld_cunt = 0;

    for (at = 0; at < (uint32_t)CROSSTALK_EMU_LD_MAX; at = at + MAIN_STEP)
    {
        if (edit->machine.ld_path[at][0] == LDG_STR_TERM) { continue; }

        edit->machine.ld_cunt = at + MAIN_STEP;
    }

    ram_mib = main_uint_clamped_get(edit->ram_text, (uint64_t)sizeof(edit->ram_text), (uint64_t)MAIN_RAM_MIN_MIB, (uint64_t)MAIN_RAM_MAX_MIB, edit->machine.ram_bytes / (uint64_t)LDG_MIB);
    hz_mhz = main_uint_clamped_get(edit->hz_text, (uint64_t)sizeof(edit->hz_text), (uint64_t)MAIN_HZ_MIN_MHZ, (uint64_t)MAIN_HZ_MAX_MHZ, edit->machine.f_op_hz / (uint64_t)MAIN_HZ_PER_MHZ);

    gamma = strtod(edit->gamma_text, &gamma_end);

    if (gamma_end == edit->gamma_text) { gamma = edit->machine.gamma; }

    if (gamma < MAIN_GAMMA_MIN) { gamma = MAIN_GAMMA_MIN; }

    if (gamma > MAIN_GAMMA_MAX) { gamma = MAIN_GAMMA_MAX; }

    edit->machine.ram_bytes = ram_mib * (uint64_t)LDG_MIB;
    edit->machine.f_op_hz = hz_mhz * (uint64_t)MAIN_HZ_PER_MHZ;
    edit->machine.gamma = gamma;
    edit->machine.key_mode = edit->keys_raw == LDG_TRUTH_TRUE ? (uint32_t)CROSSTALK_EMU_INPUT_MODE_RAW : (uint32_t)CROSSTALK_EMU_INPUT_MODE_SCAN;

    memcpy(&mngr->machine[edit->idx], &edit->machine, sizeof(mngr->machine[edit->idx]));

    edit->live = LDG_TRUTH_FALSE;

    return crosstalk_mngr_machine_save(mngr, edit->idx);
}

static uint32_t main_field_set(main_field_t *out, const char *label, uint32_t kind, char *text, uint32_t cap, uint8_t *flag)
{
    if (LDG_UNLIKELY(out == 0x0 || label == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(out, 0x0, sizeof(*out));

    if (snprintf(out->label, sizeof(out->label), MAIN_LABEL_FMT, label) < 0) { return LDG_ERR_STR_FMT; }

    out->kind = kind;
    out->text = text;
    out->cap = cap;
    out->flag = flag;

    return LDG_ERR_AOK;
}

static uint32_t main_fields_build(main_edit_t *edit, main_field_t *out, uint32_t *out_cunt)
{
    char label[MAIN_LABEL_CAP] = LDG_ARR_ZERO_INIT;
    crosstalk_mngr_machine_t *machine = 0x0;
    uint32_t cunt = 0;
    uint32_t at = 0;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(edit == 0x0 || out == 0x0 || out_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;
    machine = &edit->machine;

    if (edit->cat == (uint32_t)MAIN_CAT_MACHINE)
    {
        ret = main_field_set(&out[cunt++], MAIN_LBL_NAME, (uint32_t)MAIN_KIND_TEXT, machine->name, (uint32_t)sizeof(machine->name), 0x0);
        if (ret == LDG_ERR_AOK) { ret = main_field_set(&out[cunt++], MAIN_LBL_CORE, (uint32_t)MAIN_KIND_TEXT, machine->core_path, (uint32_t)sizeof(machine->core_path), 0x0); }

        if (ret == LDG_ERR_AOK) { ret = main_field_set(&out[cunt++], MAIN_LBL_RAM, (uint32_t)MAIN_KIND_NUM, edit->ram_text, (uint32_t)sizeof(edit->ram_text), 0x0); }

        if (ret == LDG_ERR_AOK) { ret = main_field_set(&out[cunt++], MAIN_LBL_HZ, (uint32_t)MAIN_KIND_NUM, edit->hz_text, (uint32_t)sizeof(edit->hz_text), 0x0); }

        if (ret == LDG_ERR_AOK) { ret = main_field_set(&out[cunt++], MAIN_LBL_NOTE, (uint32_t)MAIN_KIND_TEXT, machine->note, (uint32_t)sizeof(machine->note), 0x0); }
    }
    else if (edit->cat == (uint32_t)MAIN_CAT_DISPLAY) { ret = main_field_set(&out[cunt++], MAIN_LBL_GAMMA, (uint32_t)MAIN_KIND_REAL, edit->gamma_text, (uint32_t)sizeof(edit->gamma_text), 0x0); }
    else if (edit->cat == (uint32_t)MAIN_CAT_INPUT)
    {
        ret = main_field_set(&out[cunt++], MAIN_LBL_KEYS, (uint32_t)MAIN_KIND_KEYS, 0x0, (uint32_t)MAIN_CAP_NONE, &edit->keys_raw);
        if (ret == LDG_ERR_AOK) { ret = main_field_set(&out[cunt++], MAIN_LBL_AUDIO, (uint32_t)MAIN_KIND_FLAG, 0x0, (uint32_t)MAIN_CAP_NONE, &machine->audio_on); }

        if (ret == LDG_ERR_AOK) { ret = main_field_set(&out[cunt++], MAIN_LBL_SERIAL, (uint32_t)MAIN_KIND_FLAG, 0x0, (uint32_t)MAIN_CAP_NONE, &machine->serial_on); }
    }
    else if (edit->cat == (uint32_t)MAIN_CAT_STORAGE)
    {
        for (at = 0; at < (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX && ret == LDG_ERR_AOK; at = at + MAIN_STEP)
        {
            if (snprintf(label, sizeof(label), MAIN_DRIVE_FMT, at) < 0) { return LDG_ERR_STR_FMT; }

            ret = main_field_set(&out[cunt++], label, (uint32_t)MAIN_KIND_TEXT, machine->disk_path[at], (uint32_t)sizeof(machine->disk_path[at]), 0x0);
        }

        if (ret == LDG_ERR_AOK) { ret = main_field_set(&out[cunt++], MAIN_LBL_PERSIST, (uint32_t)MAIN_KIND_FLAG, 0x0, (uint32_t)MAIN_CAP_NONE, &machine->disk_persist); }
    }
    else if (edit->cat == (uint32_t)MAIN_CAT_BOOT) { for (at = 0; at < (uint32_t)CROSSTALK_EMU_LD_MAX && ret == LDG_ERR_AOK; at = at + MAIN_STEP)
        {
            if (snprintf(label, sizeof(label), MAIN_IMG_FMT, at) < 0) { return LDG_ERR_STR_FMT; }

            ret = main_field_set(&out[cunt++], label, (uint32_t)MAIN_KIND_TEXT, machine->ld_path[at], (uint32_t)sizeof(machine->ld_path[at]), 0x0);
        }
    }
    else { ret = main_field_set(&out[cunt++], MAIN_LBL_NET, (uint32_t)MAIN_KIND_TEXT, machine->net_path, (uint32_t)sizeof(machine->net_path), 0x0); }

    if (ret != LDG_ERR_AOK) { return ret; }

    *out_cunt = cunt;

    return LDG_ERR_AOK;
}

static uint8_t main_ch_fits_is(uint32_t kind, uint32_t ch)
{
    if (ch < (uint32_t)LDG_ASCII_PRINTABLE_MIN || ch > (uint32_t)LDG_ASCII_PRINTABLE_MAX) { return LDG_TRUTH_FALSE; }

    if (kind == (uint32_t)MAIN_KIND_TEXT) { return LDG_TRUTH_TRUE; }

    if (ldg_char_digit_is((char)ch) == LDG_TRUTH_TRUE) { return kind == (uint32_t)MAIN_KIND_NUM || kind == (uint32_t)MAIN_KIND_REAL ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    if (ch == (uint32_t)MAIN_KEY_POINT && kind == (uint32_t)MAIN_KIND_REAL) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t main_field_key_apply(main_field_t *field, const ldg_term_key_event_t *key)
{
    uint64_t len = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(field == 0x0 || key == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (field->kind == (uint32_t)MAIN_KIND_FLAG || field->kind == (uint32_t)MAIN_KIND_KEYS)
    {
        if (field->flag == 0x0 || key->codepoint != (uint32_t)MAIN_KEY_TOGGLE) { return LDG_ERR_AOK; }

        *field->flag = *field->flag == LDG_TRUTH_TRUE ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    if (field->text == 0x0 || field->cap == 0) { return LDG_ERR_AOK; }

    ret = ldg_str_len_get(field->text, (uint64_t)field->cap, &len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (key->key == (uint8_t)LDG_TERM_KEY_BACKSPACE)
    {
        if (len != 0) { field->text[len - (uint64_t)MAIN_STEP] = LDG_STR_TERM; }

        return LDG_ERR_AOK;
    }

    for (at = 0; at < key->text_cunt && at < (uint32_t)LDG_TERM_KEY_TEXT_MAX; at = at + MAIN_STEP)
    {
        if (main_ch_fits_is(field->kind, key->text[at]) != LDG_TRUTH_TRUE) { continue; }

        if (len + (uint64_t)LDG_STR_TERM_SIZE >= (uint64_t)field->cap) { break; }

        field->text[len] = (char)key->text[at];
        len = len + (uint64_t)MAIN_STEP;
        field->text[len] = LDG_STR_TERM;
    }

    return LDG_ERR_AOK;
}

static uint32_t main_list_key_isr(main_state_t *state, const ldg_term_key_event_t *key)
{
    crosstalk_mngr_ctx_t *mngr = 0x0;
    char new_name[MAIN_NAME_CAP] = LDG_ARR_ZERO_INIT;

    if (LDG_UNLIKELY(state == 0x0 || key == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    mngr = state->mngr;

    if (key->key == (uint8_t)LDG_TERM_KEY_UP)
    {
        if (crosstalk_mngr_sel_ok_is(mngr) == LDG_TRUTH_TRUE && mngr->sel != 0) { mngr->sel = mngr->sel - MAIN_STEP; }
        else if (mngr->cunt != 0 && crosstalk_mngr_sel_ok_is(mngr) != LDG_TRUTH_TRUE) { mngr->sel = 0; }

        return LDG_ERR_AOK;
    }

    if (key->key == (uint8_t)LDG_TERM_KEY_DOWN)
    {
        if (crosstalk_mngr_sel_ok_is(mngr) == LDG_TRUTH_TRUE && mngr->sel + MAIN_STEP < mngr->cunt) { mngr->sel = mngr->sel + MAIN_STEP; }
        else if (mngr->cunt != 0 && crosstalk_mngr_sel_ok_is(mngr) != LDG_TRUTH_TRUE) { mngr->sel = 0; }

        return LDG_ERR_AOK;
    }

    if (key->kind != (uint8_t)LDG_TERM_KEY_KIND_PRESS || (key->mods & (uint32_t)MAIN_KEY_MOD_CMD) != 0) { return LDG_ERR_AOK; }

    if (key->codepoint == (uint32_t)MAIN_KEY_QUIT) { state->quit = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

    if (key->codepoint == (uint32_t)MAIN_KEY_RESCAN) { return crosstalk_mngr_scan(mngr); }

    if (key->codepoint == (uint32_t)MAIN_KEY_NEW)
    {
        if (snprintf(new_name, sizeof(new_name), MAIN_NEW_NAME_FMT, mngr->cunt) < 0) { return LDG_ERR_STR_FMT; }

        return crosstalk_mngr_machine_add(mngr, new_name);
    }

    if (crosstalk_mngr_sel_ok_is(mngr) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (key->codepoint == (uint32_t)MAIN_KEY_START) { state->launch_req = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

    if (key->codepoint == (uint32_t)MAIN_KEY_DROP) { return crosstalk_mngr_machine_drop(mngr, mngr->sel); }

    if (key->codepoint == (uint32_t)MAIN_KEY_EDIT)
    {
        if (main_edit_begin(state->edit, mngr, mngr->sel) == LDG_ERR_AOK) { state->mode = (uint32_t)MAIN_MODE_SETTINGS; }

        return LDG_ERR_AOK;
    }

    return LDG_ERR_AOK;
}

static uint32_t main_settings_key_isr(main_state_t *state, const ldg_term_key_event_t *key)
{
    main_field_t field[MAIN_FIELD_MAX];
    main_edit_t *edit = 0x0;
    uint32_t field_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(state == 0x0 || key == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    edit = state->edit;

    if (key->key == (uint8_t)LDG_TERM_KEY_ESCAPE)
    {
        edit->live = LDG_TRUTH_FALSE;
        state->mode = (uint32_t)MAIN_MODE_LIST;

        return LDG_ERR_AOK;
    }

    if (key->key == (uint8_t)LDG_TERM_KEY_ENTER)
    {
        state->mode = (uint32_t)MAIN_MODE_LIST;

        return main_edit_commit(edit, state->mngr);
    }

    if (key->key == (uint8_t)LDG_TERM_KEY_LHS || key->key == (uint8_t)LDG_TERM_KEY_RHS)
    {
        if (key->key == (uint8_t)LDG_TERM_KEY_RHS) { edit->cat = (edit->cat + MAIN_STEP) % (uint32_t)MAIN_CAT_CUNT; }
        else { edit->cat = (edit->cat + (uint32_t)MAIN_CAT_CUNT - MAIN_STEP) % (uint32_t)MAIN_CAT_CUNT; }

        edit->field = 0;

        return LDG_ERR_AOK;
    }

    ret = main_fields_build(edit, &field[0], &field_cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (field_cunt == 0) { return LDG_ERR_AOK; }

    if (edit->field >= field_cunt) { edit->field = 0; }

    if (key->key == (uint8_t)LDG_TERM_KEY_DOWN || (key->key == (uint8_t)LDG_TERM_KEY_TAB && (key->mods & (uint32_t)LDG_TERM_MOD_SHIFT) == 0))
    {
        edit->field = (edit->field + MAIN_STEP) % field_cunt;

        return LDG_ERR_AOK;
    }

    if (key->key == (uint8_t)LDG_TERM_KEY_UP || key->key == (uint8_t)LDG_TERM_KEY_TAB)
    {
        edit->field = (edit->field + field_cunt - MAIN_STEP) % field_cunt;

        return LDG_ERR_AOK;
    }

    return main_field_key_apply(&field[edit->field], key);
}

static uint32_t main_key_isr(main_state_t *state, const ldg_term_key_event_t *key)
{
    if (LDG_UNLIKELY(state == 0x0 || key == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (key->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE) { return LDG_ERR_AOK; }

    if (state->mode == (uint32_t)MAIN_MODE_SETTINGS) { return main_settings_key_isr(state, key); }

    return main_list_key_isr(state, key);
}

static uint32_t main_summary_draw(ldg_term_render_t *render, const crosstalk_mngr_machine_t *machine, const main_palette_t *pal, uint32_t col)
{
    char line[MAIN_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t row = (uint32_t)MAIN_BODY_ROW;
    uint32_t drive = 0;

    if (LDG_UNLIKELY(render == 0x0 || machine == 0x0 || pal == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (main_text_put(render, col, row, machine->name, (uint64_t)sizeof(machine->name), &pal->fg, &pal->bg) != LDG_ERR_AOK) { LDG_ERRLOG_WARN("mngr summary row failed"); }

    row = row + MAIN_STEP + (uint32_t)MAIN_GAP;

    if (snprintf(line, sizeof(line), MAIN_SUM_FMT, MAIN_LBL_CORE, machine->core_path[0] != LDG_STR_TERM ? machine->core_path : MAIN_EMPTY) > 0) { if (main_text_put(render, col, row, line, (uint64_t)sizeof(line), &pal->fg, &pal->bg) != LDG_ERR_AOK) { LDG_ERRLOG_WARN("mngr summary row failed"); } }

    row = row + MAIN_STEP;

    if (snprintf(line, sizeof(line), MAIN_SUM_HZ_FMT, machine->f_op_hz / (uint64_t)MAIN_HZ_PER_MHZ) > 0) { if (main_text_put(render, col, row, line, (uint64_t)sizeof(line), &pal->fg, &pal->bg) != LDG_ERR_AOK) { LDG_ERRLOG_WARN("mngr summary row failed"); } }

    row = row + MAIN_STEP;

    if (snprintf(line, sizeof(line), MAIN_SUM_RAM_FMT, machine->ram_bytes / (uint64_t)LDG_MIB) > 0) { if (main_text_put(render, col, row, line, (uint64_t)sizeof(line), &pal->fg, &pal->bg) != LDG_ERR_AOK) { LDG_ERRLOG_WARN("mngr summary row failed"); } }

    row = row + MAIN_STEP;

    if (snprintf(line, sizeof(line), MAIN_SUM_IMGS_FMT, machine->ld_cunt) > 0) { if (main_text_put(render, col, row, line, (uint64_t)sizeof(line), &pal->fg, &pal->bg) != LDG_ERR_AOK) { LDG_ERRLOG_WARN("mngr summary row failed"); } }

    row = row + MAIN_STEP;

    for (drive = 0; drive < (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX; drive = drive + MAIN_STEP)
    {
        if (machine->disk_path[drive][0] == LDG_STR_TERM) { continue; }

        if (snprintf(line, sizeof(line), MAIN_SUM_DRIVE_FMT, drive, machine->disk_path[drive]) > 0) { if (main_text_put(render, col, row, line, (uint64_t)sizeof(line), &pal->fg, &pal->bg) != LDG_ERR_AOK) { LDG_ERRLOG_WARN("mngr summary row failed"); } }

        row = row + MAIN_STEP;
    }

    if (machine->net_path[0] != LDG_STR_TERM) { if (snprintf(line, sizeof(line), MAIN_SUM_NET_FMT, machine->net_path) > 0) { if (main_text_put(render, col, row, line, (uint64_t)sizeof(line), &pal->fg, &pal->bg) != LDG_ERR_AOK) { LDG_ERRLOG_WARN("mngr summary row failed"); } } }

    return LDG_ERR_AOK;
}

static uint32_t main_list_draw(main_state_t *state, ldg_term_render_t *render, const main_palette_t *pal)
{
    const crosstalk_mngr_ctx_t *mngr = 0x0;
    uint32_t body_cunt = 0;
    uint32_t row = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(state == 0x0 || render == 0x0 || pal == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    mngr = state->mngr;

    if (render->row_cunt <= (uint32_t)MAIN_CHROME_ROW_CUNT) { return LDG_ERR_AOK; }

    body_cunt = render->row_cunt - (uint32_t)MAIN_CHROME_ROW_CUNT;

    if (crosstalk_mngr_sel_ok_is(mngr) == LDG_TRUTH_TRUE)
    {
        if (mngr->sel < state->list_top) { state->list_top = mngr->sel; }

        if (mngr->sel >= state->list_top + body_cunt) { state->list_top = mngr->sel - body_cunt + MAIN_STEP; }
    }

    if (state->list_top >= mngr->cunt) { state->list_top = 0; }

    for (row = 0; row < body_cunt && state->list_top + row < mngr->cunt; row = row + MAIN_STEP)
    {
        uint32_t idx = state->list_top + row;
        uint8_t sel = idx == mngr->sel ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
        const ldg_term_render_color_t *fg = sel == LDG_TRUTH_TRUE ? &pal->sel_fg : &pal->fg;
        const ldg_term_render_color_t *bg = sel == LDG_TRUTH_TRUE ? &pal->sel_bg : &pal->bg;

        if (sel == LDG_TRUTH_TRUE)
        {
            ret = main_span_fill(render, (uint32_t)MAIN_ORIG_COL, (uint32_t)MAIN_BODY_ROW + row, (uint32_t)MAIN_LIST_W, fg, bg);
            if (ret != LDG_ERR_AOK) { return ret; }
        }

        ret = main_text_put(render, (uint32_t)MAIN_ORIG_COL, (uint32_t)MAIN_BODY_ROW + row, mngr->machine[idx].name, (uint64_t)sizeof(mngr->machine[idx].name), fg, bg);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    if (crosstalk_mngr_sel_ok_is(mngr) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    return main_summary_draw(render, &mngr->machine[mngr->sel], pal, (uint32_t)MAIN_LIST_W + (uint32_t)MAIN_GAP);
}

static uint32_t main_settings_draw(main_edit_t *edit, ldg_term_render_t *render, const main_palette_t *pal)
{
    main_field_t field[MAIN_FIELD_MAX];
    uint32_t field_cunt = 0;
    uint32_t val_col = (uint32_t)MAIN_CAT_W + (uint32_t)MAIN_GAP + (uint32_t)MAIN_LABEL_W;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(edit == 0x0 || render == 0x0 || pal == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (at = 0; at < (uint32_t)MAIN_CAT_CUNT; at = at + MAIN_STEP)
    {
        const char *name = 0x0;
        uint8_t sel = at == edit->cat ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

        ret = main_cat_name_get(at, &name);
        if (ret != LDG_ERR_AOK) { return ret; }

        if (sel == LDG_TRUTH_TRUE)
        {
            ret = main_span_fill(render, (uint32_t)MAIN_ORIG_COL, (uint32_t)MAIN_BODY_ROW + at, (uint32_t)MAIN_CAT_W, &pal->sel_fg, &pal->sel_bg);
            if (ret != LDG_ERR_AOK) { return ret; }
        }

        ret = main_text_put(render, (uint32_t)MAIN_ORIG_COL, (uint32_t)MAIN_BODY_ROW + at, name, (uint64_t)MAIN_LINE_CAP, sel == LDG_TRUTH_TRUE ? &pal->sel_fg : &pal->fg, sel == LDG_TRUTH_TRUE ? &pal->sel_bg : &pal->bg);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    ret = main_fields_build(edit, &field[0], &field_cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    for (at = 0; at < field_cunt; at = at + MAIN_STEP)
    {
        uint32_t row = (uint32_t)MAIN_BODY_ROW + at;
        uint8_t sel = at == edit->field ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
        const ldg_term_render_color_t *fg = sel == LDG_TRUTH_TRUE ? &pal->sel_fg : &pal->fg;
        const ldg_term_render_color_t *bg = sel == LDG_TRUTH_TRUE ? &pal->sel_bg : &pal->bg;
        const char *val = 0x0;

        ret = main_text_put(render, (uint32_t)MAIN_CAT_W + (uint32_t)MAIN_GAP, row, field[at].label, (uint64_t)sizeof(field[at].label), &pal->fg, &pal->bg);
        if (ret != LDG_ERR_AOK) { return ret; }

        if (field[at].kind == (uint32_t)MAIN_KIND_FLAG) { val = *field[at].flag == LDG_TRUTH_TRUE ? MAIN_FLAG_ON_TEXT : MAIN_FLAG_OFF_TEXT; }
        else if (field[at].kind == (uint32_t)MAIN_KIND_KEYS) { val = *field[at].flag == LDG_TRUTH_TRUE ? MAIN_KEYS_RAW_TEXT : MAIN_KEYS_SCAN_TEXT; }
        else { val = field[at].text; }

        if (sel == LDG_TRUTH_TRUE && val_col < render->col_cunt)
        {
            ret = main_span_fill(render, val_col, row, render->col_cunt - val_col, fg, bg);
            if (ret != LDG_ERR_AOK) { return ret; }
        }

        ret = main_text_put(render, val_col, row, val, field[at].cap != 0 ? (uint64_t)field[at].cap : (uint64_t)MAIN_LINE_CAP, fg, bg);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t main_frame_draw(main_state_t *state, ldg_term_render_t *render)
{
    main_palette_t pal = LDG_STRUCT_ZERO_INIT;
    const char *hdr = MAIN_TITLE;
    const char *hint = MAIN_LIST_HINT;
    uint64_t hdr_cap = (uint64_t)sizeof(MAIN_TITLE);
    uint32_t hint_row = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(state == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = main_palette_get(&pal);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_term_render_clear(render, &pal.bg);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (render->row_cunt < (uint32_t)MAIN_CHROME_ROW_CUNT) { return LDG_ERR_AOK; }

    if (state->mode == (uint32_t)MAIN_MODE_SETTINGS)
    {
        hdr = state->edit->machine.name;
        hdr_cap = (uint64_t)sizeof(state->edit->machine.name);
        hint = MAIN_SETTINGS_HINT;
    }

    ret = main_span_fill(render, (uint32_t)MAIN_ORIG_COL, (uint32_t)MAIN_HDR_ROW, render->col_cunt, &pal.fg, &pal.bar);
    if (ret == LDG_ERR_AOK) { ret = main_text_put(render, (uint32_t)MAIN_ORIG_COL, (uint32_t)MAIN_HDR_ROW, hdr, hdr_cap, &pal.fg, &pal.bar); }

    if (ret != LDG_ERR_AOK) { return ret; }

    hint_row = render->row_cunt - MAIN_STEP;

    ret = main_span_fill(render, (uint32_t)MAIN_ORIG_COL, hint_row, render->col_cunt, &pal.fg, &pal.bar);
    if (ret == LDG_ERR_AOK) { ret = main_text_put(render, (uint32_t)MAIN_ORIG_COL, hint_row, hint, (uint64_t)MAIN_LINE_CAP, &pal.fg, &pal.bar); }

    if (ret != LDG_ERR_AOK) { return ret; }

    if (state->mode == (uint32_t)MAIN_MODE_SETTINGS) { return main_settings_draw(state->edit, render, &pal); }

    return main_list_draw(state, render, &pal);
}

static uint32_t main_size_sync(main_term_t *term)
{
    uint32_t col_cunt = 0;
    uint32_t row_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(term == 0x0 || term->sesh == 0x0 || term->render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_sys_tty_size_get(&col_cunt, &row_cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (col_cunt > (uint32_t)LDG_TERM_SESH_COL_MAX_DEFAULT) { col_cunt = (uint32_t)LDG_TERM_SESH_COL_MAX_DEFAULT; }

    if (row_cunt > (uint32_t)LDG_TERM_SESH_ROW_MAX_DEFAULT) { row_cunt = (uint32_t)LDG_TERM_SESH_ROW_MAX_DEFAULT; }

    if (col_cunt == term->render->col_cunt && row_cunt == term->render->row_cunt) { return LDG_ERR_AOK; }

    ret = ldg_term_sesh_resize(term->sesh, col_cunt, row_cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    return ldg_term_sesh_render_get(term->sesh, &term->render);
}

static uint32_t main_term_close(main_term_t *term)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(term == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (term->input != 0x0) { ret = ldg_term_input_destroy(term->input); }

    if (term->sesh != 0x0) { if (ldg_term_sesh_destroy(term->sesh) != LDG_ERR_AOK) { ret = LDG_ERR_CLOSED; } }

    term->input = 0x0;
    term->sesh = 0x0;
    term->render = 0x0;

    return ret;
}

static uint32_t main_term_open(main_term_t *term)
{
    ldg_term_sesh_conf_t sesh_conf = LDG_STRUCT_ZERO_INIT;
    ldg_term_input_conf_t input_conf = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(term == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_term_sesh_conf_default_get(&sesh_conf);
    if (ret == LDG_ERR_AOK) { ret = ldg_term_sesh_create(&sesh_conf, &term->sesh); }

    if (ret == LDG_ERR_AOK) { ret = ldg_term_sesh_render_get(term->sesh, &term->render); }

    if (ret == LDG_ERR_AOK) { ret = ldg_term_input_conf_default_get(&input_conf); }

    if (ret == LDG_ERR_AOK) { ret = ldg_term_input_create(&input_conf, &term->input); }

    if (ret == LDG_ERR_AOK) { ret = ldg_term_sesh_title_set(term->sesh, MAIN_TITLE, (uint64_t)(sizeof(MAIN_TITLE) - LDG_STR_TERM_SIZE)); }

    if (ret != LDG_ERR_AOK)
    {
        if (main_term_close(term) != LDG_ERR_AOK) { LDG_ERRLOG_WARN("mngr terminal close failed"); }

        return ret;
    }

    if (main_size_sync(term) != LDG_ERR_AOK) { LDG_ERRLOG_WARN("mngr size sync failed"); }

    return LDG_ERR_AOK;
}

static uint32_t main_keys_drain(main_state_t *state, ldg_term_input_t *input)
{
    ldg_term_key_event_t key = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(state == 0x0 || input == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_term_input_poll(input);
    if (ret != LDG_ERR_AOK) { return ret; }

    while (ldg_term_input_event_next(input, &key) == LDG_ERR_AOK)
    {
        ret = main_key_isr(state, &key);
        if (ret != LDG_ERR_AOK) { y_log_message(Y_LOG_LEVEL_WARNING, "mngr: key isr failed; mode: %u; key: %u; codepoint: %u; err: %u", state->mode, (uint32_t)key.key, key.codepoint, ret); }
    }

    return LDG_ERR_AOK;
}

static uint32_t main_launch(main_state_t *state, main_term_t *term)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(state == 0x0 || term == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    state->launch_req = LDG_TRUTH_FALSE;

    ret = main_term_close(term);
    if (ret != LDG_ERR_AOK) { y_log_message(Y_LOG_LEVEL_WARNING, "mngr: terminal handover failed; err: %u", ret); }

    ret = crosstalk_mngr_launch(state->mngr, state->mngr->sel);
    if (ret != LDG_ERR_AOK) { y_log_message(Y_LOG_LEVEL_WARNING, "mngr: launch failed; sel: %u; err: %u", state->mngr->sel, ret); }

    return main_term_open(term);
}

static uint32_t main_frame_wait(uint64_t frame_beg_ms)
{
    uint64_t now_ms = 0;
    uint32_t ret = 0;

    ret = ldg_time_monotonic_get(&now_ms);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (now_ms < frame_beg_ms || now_ms - frame_beg_ms >= (uint64_t)MAIN_FRAME_MS) { return LDG_ERR_AOK; }

    return ldg_time_ns_delay(((uint64_t)MAIN_FRAME_MS - (now_ms - frame_beg_ms)) * (uint64_t)LDG_NS_PER_MS);
}

int main(int argc, char **argv)
{
    crosstalk_mngr_ctx_t *mngr = 0x0;
    main_edit_t *edit = 0x0;
    main_state_t state = LDG_STRUCT_ZERO_INIT;
    main_term_t term = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (setvbuf(stdout, 0x0, _IONBF, (uint64_t)MAIN_TRACE_NO_BUFF) != 0) { return MAIN_EXIT_FAIL; }

    printf(MAIN_TRACE_FMT, MAIN_TRACE_ENTER, (uint32_t)argc);

    if (argc < 0 || argv == 0x0) { return MAIN_EXIT_FAIL; }

    ret = ldg_mem_init();
    if (ret != LDG_ERR_AOK)
    {
        printf(MAIN_TRACE_FMT, MAIN_TRACE_MEM, ret);

        return MAIN_EXIT_FAIL;
    }

    if (y_init_logs(MAIN_LOG_NAME, Y_LOG_MODE_FILE, Y_LOG_LEVEL_INFO, MAIN_LOG_FILE, MAIN_LOG_START) == 0) { printf(MAIN_TRACE_FMT, MAIN_TRACE_LOGS, ret); }

    printf(MAIN_TRACE_FMT, MAIN_TRACE_MEM_OK, ret);

    ret = ldg_mem_alloc(sizeof(*mngr), (void **)&mngr);
    if (ret != LDG_ERR_AOK)
    {
        printf(MAIN_TRACE_FMT, MAIN_TRACE_ALLOC, ret);

        return MAIN_EXIT_FAIL;
    }

    ret = ldg_mem_alloc(sizeof(*edit), (void **)&edit);
    if (ret != LDG_ERR_AOK)
    {
        printf(MAIN_TRACE_FMT, MAIN_TRACE_ALLOC, ret);

        return MAIN_EXIT_FAIL;
    }

    memset(mngr, 0x0, sizeof(*mngr));
    memset(edit, 0x0, sizeof(*edit));

    ret = crosstalk_mngr_init(mngr);
    if (ret != LDG_ERR_AOK)
    {
        printf(MAIN_TRACE_FMT, MAIN_TRACE_MNGR, ret);

        return MAIN_EXIT_FAIL;
    }

    printf(MAIN_TRACE_FMT, MAIN_TRACE_MNGR_OK, mngr->cunt);

    if (crosstalk_mngr_scan(mngr) != LDG_ERR_AOK) { LDG_ERRLOG_WARN("mngr scan failed"); }

    ret = main_term_open(&term);
    if (ret != LDG_ERR_AOK)
    {
        printf(MAIN_TRACE_FMT, MAIN_TRACE_TERM, ret);

        return MAIN_EXIT_FAIL;
    }

    state.mngr = mngr;
    state.edit = edit;
    state.mode = (uint32_t)MAIN_MODE_LIST;

    while (state.quit != LDG_TRUTH_TRUE)
    {
        uint64_t now_ms = 0;

        if (ldg_time_monotonic_get(&now_ms) != LDG_ERR_AOK) { now_ms = 0; }

        if (main_size_sync(&term) != LDG_ERR_AOK) { LDG_ERRLOG_WARN("mngr size sync failed"); }

        if (main_keys_drain(&state, term.input) != LDG_ERR_AOK) { LDG_ERRLOG_WARN("mngr key drain failed"); }

        if (state.launch_req == LDG_TRUTH_TRUE)
        {
            ret = main_launch(&state, &term);
            if (ret != LDG_ERR_AOK)
            {
                y_log_message(Y_LOG_LEVEL_ERROR, "mngr: terminal reopen failed; err: %u", ret);

                break;
            }
        }

        ret = main_frame_draw(&state, term.render);
        if (ret != LDG_ERR_AOK) { y_log_message(Y_LOG_LEVEL_WARNING, "mngr: frame draw failed; mode: %u; err: %u", state.mode, ret); }

        ret = ldg_term_sesh_wr(term.sesh);
        if (ret != LDG_ERR_AOK)
        {
            y_log_message(Y_LOG_LEVEL_ERROR, "mngr: terminal write failed; err: %u", ret);

            break;
        }

        if (main_frame_wait(now_ms) != LDG_ERR_AOK) { LDG_ERRLOG_WARN("mngr frame wait failed"); }
    }

    if (main_term_close(&term) != LDG_ERR_AOK) { LDG_ERRLOG_WARN("mngr terminal close failed"); }

    if (ldg_mem_dealloc(edit) != LDG_ERR_AOK) { LDG_ERRLOG_WARN("mngr edit dealloc failed"); }

    if (ldg_mem_dealloc(mngr) != LDG_ERR_AOK) { LDG_ERRLOG_WARN("mngr ctx dealloc failed"); }

    if (ldg_mem_shutdown() != LDG_ERR_AOK) { LDG_ERRLOG_WARN("mngr memory shutdown failed"); }

    y_close_logs();

    return MAIN_EXIT_OK;
}
