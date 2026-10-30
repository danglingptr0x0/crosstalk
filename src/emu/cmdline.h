#ifndef CROSSTALK_EMU_CMDLINE_H
#define CROSSTALK_EMU_CMDLINE_H

#include <stdint.h>

#include "emu/emu.h"

#define CROSSTALK_EMU_CMDLINE_ERR_ARG 1030
#define CROSSTALK_EMU_CMDLINE_ERR_DRIVE 1031

#define CROSSTALK_EMU_CMDLINE_ARG_CORE "--core"
#define CROSSTALK_EMU_CMDLINE_ARG_PROG "--prog"
#define CROSSTALK_EMU_CMDLINE_ARG_MEMLD "--memld"
#define CROSSTALK_EMU_CMDLINE_OFF_SEP '@'
#define CROSSTALK_EMU_CMDLINE_ARG_DISK "--disk"
#define CROSSTALK_EMU_CMDLINE_ARG_HZ "--hz"
#define CROSSTALK_EMU_CMDLINE_ARG_CLK "--clk"
#define CROSSTALK_EMU_CMDLINE_ARG_MEM "--mem"
#define CROSSTALK_EMU_CMDLINE_ARG_FONT "--font"
#define CROSSTALK_EMU_CMDLINE_ARG_WIN "--win"
#define CROSSTALK_EMU_CMDLINE_ARG_GAMMA "--gamma"
#define CROSSTALK_EMU_CMDLINE_ARG_KEYS "--keys"
#define CROSSTALK_EMU_CMDLINE_ARG_NET "--net"
#define CROSSTALK_EMU_CMDLINE_ARG_NO_AUDIO "--no-audio"
#define CROSSTALK_EMU_CMDLINE_ARG_NO_SERIAL "--no-serial"
#define CROSSTALK_EMU_CMDLINE_ARG_NO_PERSIST "--no-persist"
#define CROSSTALK_EMU_CMDLINE_ARG_HELP "--help"

#define CROSSTALK_EMU_CMDLINE_KEYS_RAW "raw"
#define CROSSTALK_EMU_CMDLINE_KEYS_SCAN "scan"

#define CROSSTALK_EMU_CMDLINE_DRIVE_SEP ':'
#define CROSSTALK_EMU_CMDLINE_WIN_SEP 'x'

uint32_t crosstalk_emu_cmdline_parse(uint32_t argc, char **argv, crosstalk_emu_conf_t *out, double *out_gamma, uint8_t *out_help);
uint32_t crosstalk_emu_cmdline_usage_put(void);

#endif
