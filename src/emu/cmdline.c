#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <dangling/logging/logging.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "core/log.h"
#include "emu/cmdline.h"

#define CMDLINE_STEP 1
#define CMDLINE_DRIVE_DIGIT_LEN 1
#define CMDLINE_SEP_LEN 1

static uint32_t cmdline_drive_parse(const char *val, uint32_t *out_drive, const char **out_path)
{
    uint32_t drive = 0;

    if (LDG_UNLIKELY(val == 0x0 || out_drive == 0x0 || out_path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_drive = 0;
    *out_path = val;

    if (val[0] == LDG_STR_TERM) { return CROSSTALK_EMU_CMDLINE_ERR_ARG; }

    if (ldg_char_digit_is(val[0]) != LDG_TRUTH_TRUE || val[CMDLINE_DRIVE_DIGIT_LEN] != (char)CROSSTALK_EMU_CMDLINE_DRIVE_SEP) { return LDG_ERR_AOK; }

    drive = (uint32_t)(val[0] - (char)LDG_ASCII_DIGIT_ZERO);

    if (drive >= (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX) { return CROSSTALK_EMU_CMDLINE_ERR_DRIVE; }

    *out_drive = drive;
    *out_path = &val[CMDLINE_DRIVE_DIGIT_LEN + CMDLINE_SEP_LEN];

    if ((*out_path)[0] == LDG_STR_TERM) { return CROSSTALK_EMU_CMDLINE_ERR_ARG; }

    return LDG_ERR_AOK;
}

static uint32_t cmdline_win_parse(const char *val, uint32_t *out_w, uint32_t *out_h)
{
    uint32_t sep_at = (uint32_t)UINT32_MAX;
    uint32_t at = 0;
    uint32_t ask_w = 0;
    uint32_t ask_h = 0;
    uint32_t fit_h = 0;

    if (LDG_UNLIKELY(val == 0x0 || out_w == 0x0 || out_h == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (val[at] != LDG_STR_TERM)
    {
        if (val[at] == (char)CROSSTALK_EMU_CMDLINE_WIN_SEP)
        {
            sep_at = at;

            break;
        }

        at = at + (uint32_t)CMDLINE_STEP;
    }

    if (sep_at == (uint32_t)UINT32_MAX || val[sep_at + CMDLINE_SEP_LEN] == LDG_STR_TERM) { return CROSSTALK_EMU_CMDLINE_ERR_ARG; }

    *out_w = (uint32_t)strtoul(val, 0x0, LDG_BASE_DEC);
    *out_h = (uint32_t)strtoul(&val[sep_at + CMDLINE_SEP_LEN], 0x0, LDG_BASE_DEC);

    if (*out_w == 0 || *out_h == 0) { return CROSSTALK_EMU_CMDLINE_ERR_ARG; }

    ask_w = *out_w;
    ask_h = *out_h;
    fit_h = (uint32_t)((uint64_t)ask_w * (uint64_t)CROSSTALK_EMU_SCREEN_ASPECT_DEN / (uint64_t)CROSSTALK_EMU_SCREEN_ASPECT_NUM);

    if (fit_h > ask_h) { *out_w = (uint32_t)((uint64_t)ask_h * (uint64_t)CROSSTALK_EMU_SCREEN_ASPECT_NUM / (uint64_t)CROSSTALK_EMU_SCREEN_ASPECT_DEN); }
    else { *out_h = fit_h; }

    if (*out_w == 0 || *out_h == 0) { return CROSSTALK_EMU_CMDLINE_ERR_ARG; }

    if (*out_w != ask_w || *out_h != ask_h) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "dangcsaemu: window locked to the display aspect; asked: %u32x%u32; win: %u32x%u32", ask_w, ask_h, *out_w, *out_h); }

    return LDG_ERR_AOK;
}

static uint32_t cmdline_memld_add(crosstalk_emu_conf_t *out, char *val)
{
    uint64_t word_off = (uint64_t)CROSSTALK_EMU_LD_OFF_NONE;
    uint32_t sep_at = (uint32_t)UINT32_MAX;
    uint32_t at = 0;

    if (LDG_UNLIKELY(out == 0x0 || val == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (out->ld_cunt >= (uint32_t)CROSSTALK_EMU_LD_MAX) { return LDG_ERR_FULL; }

    while (val[at] != LDG_STR_TERM)
    {
        if (val[at] == (char)CROSSTALK_EMU_CMDLINE_OFF_SEP) { sep_at = at; }

        at = at + (uint32_t)CMDLINE_STEP;
    }

    if (sep_at != (uint32_t)UINT32_MAX)
    {
        if (val[sep_at + CMDLINE_SEP_LEN] == LDG_STR_TERM) { return CROSSTALK_EMU_CMDLINE_ERR_ARG; }

        word_off = strtoull(&val[sep_at + CMDLINE_SEP_LEN], 0x0, LDG_BASE_DEC);
        val[sep_at] = LDG_STR_TERM;
    }

    out->ld_path[out->ld_cunt] = val;
    out->ld_off[out->ld_cunt] = word_off;
    out->ld_cunt = out->ld_cunt + (uint32_t)CMDLINE_STEP;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_cmdline_usage_put(void)
{
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "dangcsaemu: usage: dangcsaemu [--core <core.bin>] [options]");
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "dangcsaemu: --core <path>; emulator core to run");
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "dangcsaemu: --memld <path>[@<word off>]; image stored into guest memory at reset; repeatable");
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "dangcsaemu: --prog <path>; the same, at the start of memory");
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "dangcsaemu: --disk [N:]<path>; disk image for drive N; repeatable");
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "dangcsaemu: --hz <cycles>; operating frequency the guest is paced at");
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "dangcsaemu: --clk <name>; clock signal name; defaults to %s", CROSSTALK_EMU_CLK_SIG_DEFAULT);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "dangcsaemu: --mem <name>; memory signal name; defaults to %s", CROSSTALK_EMU_MEM_SIG_DEFAULT);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "dangcsaemu: --font <path>; console font");
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "dangcsaemu: --win <WxH>; window size; defaults to %u32x%u32", (uint32_t)CROSSTALK_EMU_WIN_W_DEFAULT, (uint32_t)CROSSTALK_EMU_WIN_H_DEFAULT);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "dangcsaemu: --gamma <value>; display gamma; %.1f to %.1f", CROSSTALK_EMU_SCREEN_GAMMA_MIN, CROSSTALK_EMU_SCREEN_GAMMA_MAX);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "dangcsaemu: --keys raw|scan; key delivery; defaults to scan");
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "dangcsaemu: --net <path>; serial endpoint the guest network bytes move through");
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "dangcsaemu: --no-audio; leave sound off");
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "dangcsaemu: --no-serial; do not mirror guest serial to the log");
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "dangcsaemu: --no-persist; do not write disk images back on exit");

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_cmdline_parse(uint32_t argc, char **argv, crosstalk_emu_conf_t *out, double *out_gamma, uint8_t *out_help)
{
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(argv == 0x0 || out == 0x0 || out_gamma == 0x0 || out_help == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_help = LDG_TRUTH_FALSE;
    *out_gamma = CROSSTALK_EMU_SCREEN_GAMMA_DEFAULT;

    ret = crosstalk_emu_conf_default_get(out);
    if (ret != LDG_ERR_AOK) { return ret; }

    for (at = (uint32_t)CMDLINE_STEP; at < argc; at = at + (uint32_t)CMDLINE_STEP)
    {
        const char *arg = argv[at];
        const char *val = 0x0;
        uint8_t wants_val = LDG_TRUTH_TRUE;

        if (arg == 0x0) { continue; }

        if (strcmp(arg, CROSSTALK_EMU_CMDLINE_ARG_HELP) == 0) { *out_help = LDG_TRUTH_TRUE; return LDG_ERR_AOK; }

        if (strcmp(arg, CROSSTALK_EMU_CMDLINE_ARG_NO_AUDIO) == 0) { out->audio_on = LDG_TRUTH_FALSE; wants_val = LDG_TRUTH_FALSE; }
        else if (strcmp(arg, CROSSTALK_EMU_CMDLINE_ARG_NO_SERIAL) == 0) { out->serial_on = LDG_TRUTH_FALSE; wants_val = LDG_TRUTH_FALSE; }
        else if (strcmp(arg, CROSSTALK_EMU_CMDLINE_ARG_NO_PERSIST) == 0) { out->disk_persist = LDG_TRUTH_FALSE; wants_val = LDG_TRUTH_FALSE; }

        if (wants_val != LDG_TRUTH_TRUE) { continue; }

        if (at + (uint32_t)CMDLINE_STEP >= argc)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "dangcsaemu: option needs a value; option: %s", arg);

            return CROSSTALK_EMU_CMDLINE_ERR_ARG;
        }

        at = at + (uint32_t)CMDLINE_STEP;
        val = argv[at];

        if (strcmp(arg, CROSSTALK_EMU_CMDLINE_ARG_CORE) == 0) { out->plugin_path = val; }
        else if (strcmp(arg, CROSSTALK_EMU_CMDLINE_ARG_PROG) == 0 || strcmp(arg, CROSSTALK_EMU_CMDLINE_ARG_MEMLD) == 0)
        {
            ret = cmdline_memld_add(out, argv[at]);
            if (ret != LDG_ERR_AOK)
            {
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "dangcsaemu: image not readable; value: %s; err: %u32", val, ret);

                return ret;
            }
        }
        else if (strcmp(arg, CROSSTALK_EMU_CMDLINE_ARG_CLK) == 0) { out->clk_sig = val; }
        else if (strcmp(arg, CROSSTALK_EMU_CMDLINE_ARG_MEM) == 0) { out->mem_sig = val; }
        else if (strcmp(arg, CROSSTALK_EMU_CMDLINE_ARG_FONT) == 0) { out->font_path = val; }
        else if (strcmp(arg, CROSSTALK_EMU_CMDLINE_ARG_NET) == 0) { out->net_path = val; }
        else if (strcmp(arg, CROSSTALK_EMU_CMDLINE_ARG_HZ) == 0) { out->f_op_hz = strtoull(val, 0x0, LDG_BASE_DEC); }
        else if (strcmp(arg, CROSSTALK_EMU_CMDLINE_ARG_GAMMA) == 0) { *out_gamma = strtod(val, 0x0); }
        else if (strcmp(arg, CROSSTALK_EMU_CMDLINE_ARG_KEYS) == 0) { if (strcmp(val, CROSSTALK_EMU_CMDLINE_KEYS_RAW) == 0) { out->key_mode = (uint32_t)CROSSTALK_EMU_INPUT_MODE_RAW; }
            else if (strcmp(val, CROSSTALK_EMU_CMDLINE_KEYS_SCAN) == 0) { out->key_mode = (uint32_t)CROSSTALK_EMU_INPUT_MODE_SCAN; }
            else
            {
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "dangcsaemu: key mode not known; mode: %s", val);
                return CROSSTALK_EMU_CMDLINE_ERR_ARG;
            } }
        else if (strcmp(arg, CROSSTALK_EMU_CMDLINE_ARG_WIN) == 0)
        {
            ret = cmdline_win_parse(val, &out->win_w, &out->win_h);
            if (ret != LDG_ERR_AOK)
            {
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "dangcsaemu: window size not readable; value: %s", val);

                return ret;
            }
        }
        else if (strcmp(arg, CROSSTALK_EMU_CMDLINE_ARG_DISK) == 0)
        {
            uint32_t drive = 0;
            const char *path = 0x0;

            ret = cmdline_drive_parse(val, &drive, &path);
            if (ret != LDG_ERR_AOK)
            {
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "dangcsaemu: disk not readable; value: %s", val);

                return ret;
            }

            out->disk_path[drive] = path;
        }
        else
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "dangcsaemu: option not known; option: %s", arg);

            return CROSSTALK_EMU_CMDLINE_ERR_ARG;
        }
    }

    if (out->plugin_path == 0x0) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "dangcsaemu: no core given; the display has no source until one is attached"); }

    return LDG_ERR_AOK;
}
