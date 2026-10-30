#ifndef CROSSTALK_EDA_H
#define CROSSTALK_EDA_H

#include <stdint.h>

#include <dangling/mem/alloc.h>

#include "eda/clk/clk.h"
#include "eda/ic/fpga/build.h"
#include "eda/ic/fpga/emu.h"
#include "eda/ic/fpga/fpga.h"
#include "eda/ic/fpga/run.h"
#include "eda/io/cast.h"
#include "eda/io/display.h"
#include "eda/io/midi.h"
#include "eda/io/pty.h"
#include "eda/io/receptacle.h"
#include "eda/io/serial.h"
#include "eda/logic/probe.h"
#include "eda/logic/stream.h"
#include "eda/script/script.h"
#include "eda/sound/sound.h"
#include "eda/ui/analyzer.h"
#include "eda/ui/brainomatic.h"
#include "eda/ui/calc.h"
#include "eda/ui/feed.h"
#include "eda/ui/pipeview.h"
#include "eda/ui/progview.h"
#include "eda/ui/regview.h"
#include "eda/ui/stackview.h"
#include "eda/ui/indicator.h"
#include "eda/ui/scope.h"
#include "eda/ui/memview.h"
#include "eda/ui/profile.h"
#include "eda/ui/wshop_browser.h"
#include "eda/ui/wshop_publish.h"
#include "eda/ui/display/crt.h"
#include "eda/ui/term.h"

typedef struct crosstalk_eda_ctx
{
    crosstalk_eda_ic_fpga_ctx_t fpga;
    crosstalk_eda_ic_fpga_run_ctx_t fpga_run;
    crosstalk_eda_ic_fpga_build_ctx_t fpga_build;
    crosstalk_eda_ic_fpga_emu_ctx_t fpga_emu;
    crosstalk_eda_logic_probe_ctx_t logic_probe;
    crosstalk_eda_io_display_ctx_t display;
    crosstalk_eda_io_receptacle_ctx_t receptacle;
    crosstalk_eda_io_midi_ctx_t midi;
    crosstalk_eda_script_ctx_t script;
    ldg_mem_pool_t *script_pool;
    crosstalk_eda_ui_display_crt_ctx_t crt;
    crosstalk_eda_logic_stream_ctx_t logic_stream;
    crosstalk_eda_ui_term_ctx_t term;
    crosstalk_eda_ui_indicator_ctx_t indicator;
    crosstalk_eda_ui_analyzer_ctx_t analyzer;
    crosstalk_eda_ui_brainomatic_ctx_t brainomatic;
    crosstalk_eda_ui_calc_ctx_t calc;
    crosstalk_eda_ui_feed_ctx_t feed;
    crosstalk_eda_ui_pipeview_ctx_t pipeview;
    crosstalk_eda_ui_progview_ctx_t progview;
    crosstalk_eda_ui_regview_ctx_t regview;
    crosstalk_eda_ui_stackview_ctx_t stackview;
    crosstalk_eda_ui_scope_ctx_t scope;
    crosstalk_eda_ui_memview_ctx_t memview;
    crosstalk_eda_ui_profile_ctx_t profile;
    crosstalk_eda_ui_wshop_browser_ctx_t wshop_browser;
    crosstalk_eda_ui_wshop_publish_ctx_t wshop_publish;
    crosstalk_eda_io_serial_ctx_t serial;
    crosstalk_eda_io_cast_ctx_t cast;
    crosstalk_eda_io_pty_ctx_t pty;
    ldg_mem_pool_t *pty_pool;
    ldg_mem_pool_t *term_pool;
    crosstalk_eda_sound_ctx_t sound;
    crosstalk_eda_clk_ctx_t clk;
} crosstalk_eda_ctx_t;

#endif
