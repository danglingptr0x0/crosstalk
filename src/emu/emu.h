#ifndef CROSSTALK_EMU_H
#define CROSSTALK_EMU_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/core/types.h>
#include <dangling/io/path.h>
#include <dangling/thread/pool.h>

#include "eda/ic/fpga/emu.h"
#include "emu/audio.h"
#include "emu/input.h"
#include "emu/net.h"
#include "emu/screen.h"

#define CROSSTALK_EMU_ERR_PLUGIN 1020
#define CROSSTALK_EMU_ERR_SIG 1021
#define CROSSTALK_EMU_ERR_PROG 1022
#define CROSSTALK_EMU_ERR_RENDER 1023

#define CROSSTALK_EMU_TITLE "Monitor"
#define CROSSTALK_EMU_TITLE_HLT "Monitor [hlt'd]"

#define CROSSTALK_EMU_WIN_W_DEFAULT 1280
#define CROSSTALK_EMU_WIN_H_DEFAULT 960

#define CROSSTALK_EMU_CLK_SIG_DEFAULT "clk"
#define CROSSTALK_EMU_MEM_SIG_DEFAULT "cells"

#define CROSSTALK_EMU_HEX_EXT ".hex"
#define CROSSTALK_EMU_HEX_EXT_LEN 4
#define CROSSTALK_EMU_WORD_BYTES 8
#define CROSSTALK_EMU_LD_MAX 8
#define CROSSTALK_EMU_LD_OFF_NONE 0

#define CROSSTALK_EMU_DT_MAX 0.25
#define CROSSTALK_EMU_BUDGET_MIN 1024
#define CROSSTALK_EMU_SLICE_CYCLES 4096
#define CROSSTALK_EMU_STEP_INPUT 1
#define CROSSTALK_EMU_SLICE_NS ((uint64_t)8 * LDG_NS_PER_MS)
#define CROSSTALK_EMU_SERIAL_CHUNK 4096
#define CROSSTALK_EMU_RATE_SECS 1.0
#define CROSSTALK_EMU_RATE_LOG_EVERY 30
#define CROSSTALK_EMU_HZ_MIN 1000000
#define CROSSTALK_EMU_HZ_MAX 2000000000
#define CROSSTALK_EMU_STEP 1
#define CROSSTALK_EMU_WORKERS 1
#define CROSSTALK_EMU_CHUNK_CYCLES 4096
#define CROSSTALK_EMU_INPUT_CHUNK_CYCLES 1
#define CROSSTALK_EMU_HLT_WAIT_NS 100000
#define CROSSTALK_EMU_IDLE_YIELD_NS 1000

typedef struct crosstalk_emu_conf
{
    const char *plugin_path;
    const char *ld_path[CROSSTALK_EMU_LD_MAX];
    uint64_t ld_off[CROSSTALK_EMU_LD_MAX];
    uint32_t ld_cunt;
    uint32_t ld_pudding;
    const char *disk_path[CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX];
    const char *clk_sig;
    const char *mem_sig;
    const char *font_path;
    const char *net_path;
    uint64_t f_op_hz;
    uint32_t win_w;
    uint32_t win_h;
    uint32_t key_mode;
    uint8_t audio_on;
    uint8_t disk_persist;
    uint8_t serial_on;
    uint8_t pudding[5];
} crosstalk_emu_conf_t;

typedef struct crosstalk_emu_ctx
{
    crosstalk_eda_ic_fpga_emu_ctx_t emu;
    crosstalk_emu_screen_ctx_t screen;
    crosstalk_emu_input_ctx_t input;
    crosstalk_emu_audio_ctx_t audio;
    crosstalk_emu_net_ctx_t net;
    ldg_thread_pool_t pool;
    char ld_path[CROSSTALK_EMU_LD_MAX][LDG_IO_PATH_MAX];
    uint64_t ld_off[CROSSTALK_EMU_LD_MAX];
    uint32_t ld_cunt;
    uint32_t ld_pudding;
    uint8_t serial[CROSSTALK_EMU_SERIAL_CHUNK];
    char font_path[LDG_IO_PATH_MAX];
    uint64_t f_op_hz;
    uint64_t cycles_total;
    uint64_t cycles_mark;
    uint64_t cycles_gen;
    uint64_t cycles_gen_mark;
    uint64_t serial_bytes;
    uint64_t rate_log_cuntr;
    double rate_next;
    double rate_mark;
    double rate_hz;
    uint32_t clk_sig;
    uint32_t mem_sig;
    uint32_t stop;
    uint32_t alive;
    uint32_t rst_req;
    uint8_t serial_on;
    uint8_t audio_on;
    uint8_t hlt;
    uint8_t ok;
    uint8_t live;
    uint8_t pudding[3];
} crosstalk_emu_ctx_t;

uint32_t crosstalk_emu_conf_default_get(crosstalk_emu_conf_t *out);
uint32_t crosstalk_emu_init(crosstalk_emu_ctx_t *ctx, const crosstalk_emu_conf_t *conf);
uint32_t crosstalk_emu_shutdown(crosstalk_emu_ctx_t *ctx);
uint32_t crosstalk_emu_run_start(crosstalk_emu_ctx_t *ctx);
uint32_t crosstalk_emu_run_hlt(crosstalk_emu_ctx_t *ctx);
uint32_t crosstalk_emu_pump(crosstalk_emu_ctx_t *ctx, double now);
uint8_t crosstalk_emu_run_live_is(const crosstalk_emu_ctx_t *ctx);
uint32_t crosstalk_emu_rst_req(crosstalk_emu_ctx_t *ctx);
uint32_t crosstalk_emu_rst(crosstalk_emu_ctx_t *ctx);
uint32_t crosstalk_emu_rate_get(const crosstalk_emu_ctx_t *ctx, double *out);
uint32_t crosstalk_emu_hz_set(crosstalk_emu_ctx_t *ctx, uint64_t hz);
uint64_t crosstalk_emu_hz_get(const crosstalk_emu_ctx_t *ctx);
uint8_t crosstalk_emu_live_is(const crosstalk_emu_ctx_t *ctx);
uint8_t crosstalk_emu_hlt_is(const crosstalk_emu_ctx_t *ctx);

#endif
