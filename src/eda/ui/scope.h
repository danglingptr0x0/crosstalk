#ifndef CROSSTALK_EDA_UI_SCOPE_H
#define CROSSTALK_EDA_UI_SCOPE_H

#include <stdint.h>

#include "eda/ic/fpga/fpga.h"
#include "ui/window.h"
#include "ui/theme.h"

#define CROSSTALK_EDA_UI_SCOPE_ERR_SRC 930
#define CROSSTALK_EDA_UI_SCOPE_ERR_TIMEBASE 931
#define CROSSTALK_EDA_UI_SCOPE_ERR_NO_SIG 932
#define CROSSTALK_EDA_UI_SCOPE_ERR_MODE 935
#define CROSSTALK_EDA_UI_SCOPE_ERR_TRACE 936

#define CROSSTALK_EDA_UI_SCOPE_MEM_TRACE_MAX 4
#define CROSSTALK_EDA_UI_SCOPE_MEM_SAMPLE_CAP 512
#define CROSSTALK_EDA_UI_SCOPE_MEM_LABEL_CAP 48
#define CROSSTALK_EDA_UI_SCOPE_MEM_WORD_BYTES 4
#define CROSSTALK_EDA_UI_SCOPE_FIXED_SCALE 65536.0
#define CROSSTALK_EDA_UI_SCOPE_TRACE_B_COL 0xFFC24BFFU
#define CROSSTALK_EDA_UI_SCOPE_XY_FADE_MIN 40.0
#define CROSSTALK_EDA_UI_SCOPE_XY_FADE_MAX 255.0
#define CROSSTALK_EDA_UI_SCOPE_RANGE_FLAT 1.0

#define CROSSTALK_EDA_UI_SCOPE_SIG_CAP 64
#define CROSSTALK_EDA_UI_SCOPE_SIG_FULL 65536.0

#define CROSSTALK_EDA_UI_SCOPE_SAMPLE_CAP 1024
#define CROSSTALK_EDA_UI_SCOPE_REF_CAP 16
#define CROSSTALK_EDA_UI_SCOPE_PIN_CAP 16
#define CROSSTALK_EDA_UI_SCOPE_STATUS_CAP 160
#define CROSSTALK_EDA_UI_SCOPE_EDGE_MIN 2

#define CROSSTALK_EDA_UI_SCOPE_WINDOW_ID 0x53434F50ULL
#define CROSSTALK_EDA_UI_SCOPE_WINDOW_TITLE "Oscilloscope"
#define CROSSTALK_EDA_UI_SCOPE_DIV_SAMPLES 32
#define CROSSTALK_EDA_UI_SCOPE_STATUS_ROW_CUNT 1
#define CROSSTALK_EDA_UI_SCOPE_DIV_MIN 1
#define CROSSTALK_EDA_UI_SCOPE_NS_PER_DIV_DEFAULT 100
#define CROSSTALK_EDA_UI_SCOPE_NS_PER_DIV_MIN 1
#define CROSSTALK_EDA_UI_SCOPE_NS_PER_DIV_MAX 10000000
#define CROSSTALK_EDA_UI_SCOPE_TIMEBASE_STEP 2
#define CROSSTALK_EDA_UI_SCOPE_ZOOM_IN (-1)
#define CROSSTALK_EDA_UI_SCOPE_ZOOM_OUT 1
#define CROSSTALK_EDA_UI_SCOPE_VOLT_PER_DIV_DEFAULT 0.5
#define CROSSTALK_EDA_UI_SCOPE_IDLE_TEXT "no scope attached; hover a pad and run :scope"
#define CROSSTALK_EDA_UI_SCOPE_DEAD_TEXT "no signal at this pad"
#define CROSSTALK_EDA_UI_SCOPE_LOGIC_VOLT 3.3
#define CROSSTALK_EDA_UI_SCOPE_HALF 2
#define CROSSTALK_EDA_UI_SCOPE_STEP 1
#define CROSSTALK_EDA_UI_SCOPE_HZ_NONE 0
#define CROSSTALK_EDA_UI_SCOPE_TIME_ORIG 0
#define CROSSTALK_EDA_UI_SCOPE_INSET 10.0
#define CROSSTALK_EDA_UI_SCOPE_FRAC_MAX 1.0
#define CROSSTALK_EDA_UI_SCOPE_PERIOD_SAMPLES 16
#define CROSSTALK_EDA_UI_SCOPE_TAU 6.283185307179586
#define CROSSTALK_EDA_UI_SCOPE_MID 0.5

typedef enum crosstalk_eda_ui_scope_src
{
    CROSSTALK_EDA_UI_SCOPE_SRC_PIN = 0,
    CROSSTALK_EDA_UI_SCOPE_SRC_SIG
} crosstalk_eda_ui_scope_src_t;

typedef enum crosstalk_eda_ui_scope_wave
{
    CROSSTALK_EDA_UI_SCOPE_WAVE_SQUARE = 0,
    CROSSTALK_EDA_UI_SCOPE_WAVE_SINE
} crosstalk_eda_ui_scope_wave_t;

typedef enum crosstalk_eda_ui_scope_width
{
    CROSSTALK_EDA_UI_SCOPE_WIDTH_UINT8 = 0,
    CROSSTALK_EDA_UI_SCOPE_WIDTH_UINT16,
    CROSSTALK_EDA_UI_SCOPE_WIDTH_UINT32,
    CROSSTALK_EDA_UI_SCOPE_WIDTH_INT8,
    CROSSTALK_EDA_UI_SCOPE_WIDTH_INT16,
    CROSSTALK_EDA_UI_SCOPE_WIDTH_INT32
} crosstalk_eda_ui_scope_width_t;

typedef enum crosstalk_eda_ui_scope_mode
{
    CROSSTALK_EDA_UI_SCOPE_MODE_YT = 0,
    CROSSTALK_EDA_UI_SCOPE_MODE_XY
} crosstalk_eda_ui_scope_mode_t;

typedef struct crosstalk_eda_ui_scope_sample
{
    uint64_t sim_ns;
    double volt;
} crosstalk_eda_ui_scope_sample_t;

typedef struct crosstalk_eda_ui_scope_mem_trace
{
    double val[CROSSTALK_EDA_UI_SCOPE_MEM_SAMPLE_CAP];
    char label[CROSSTALK_EDA_UI_SCOPE_MEM_LABEL_CAP];
    uint64_t addr;
    double last;
    double lo;
    double hi;
    uint32_t width;
    uint32_t hd;
    uint32_t cunt;
    uint8_t live;
    uint8_t fixed;
    uint8_t pudding[2];
} crosstalk_eda_ui_scope_mem_trace_t;

typedef struct crosstalk_eda_ui_scope_ctx
{
    crosstalk_eda_ui_scope_sample_t sample[CROSSTALK_EDA_UI_SCOPE_SAMPLE_CAP];
    crosstalk_eda_ui_scope_mem_trace_t mem_trace[CROSSTALK_EDA_UI_SCOPE_MEM_TRACE_MAX];
    uint32_t mem_cunt;
    uint32_t mem_mode;
    uint32_t xy_x;
    uint32_t xy_y;
    crosstalk_ui_window_state_t win;
    char ref[CROSSTALK_EDA_UI_SCOPE_REF_CAP];
    char pin[CROSSTALK_EDA_UI_SCOPE_PIN_CAP];
    char sig[CROSSTALK_EDA_UI_SCOPE_SIG_CAP];
    uint64_t sig_step_ns;
    uint64_t sig_ns;
    uint64_t src_f_hz;
    uint64_t ns_per_div;
    uint64_t last_ns;
    double amplitude;
    double volt_per_div;
    uint32_t hd;
    uint32_t cunt;
    uint32_t wave;
    uint32_t src;
    uint8_t open;
    uint8_t live;
    uint8_t ok;
    uint8_t pudding[1];
} crosstalk_eda_ui_scope_ctx_t;

uint32_t crosstalk_eda_ui_scope_init(crosstalk_eda_ui_scope_ctx_t *ctx);
uint32_t crosstalk_eda_ui_scope_shutdown(crosstalk_eda_ui_scope_ctx_t *ctx);
uint32_t crosstalk_eda_ui_scope_open_set(crosstalk_eda_ui_scope_ctx_t *ctx, uint8_t open);
uint8_t crosstalk_eda_ui_scope_open_is(const crosstalk_eda_ui_scope_ctx_t *ctx);
uint32_t crosstalk_eda_ui_scope_detach(crosstalk_eda_ui_scope_ctx_t *ctx);
uint32_t crosstalk_eda_ui_scope_timebase_step(crosstalk_eda_ui_scope_ctx_t *ctx, int32_t dir);
uint32_t crosstalk_eda_ui_scope_sig_attach(crosstalk_eda_ui_scope_ctx_t *ctx, const char *sig, uint64_t step_ns);
uint32_t crosstalk_eda_ui_scope_sink_cb(void *user, const uint8_t *bytes, uint32_t len);
uint32_t crosstalk_eda_ui_scope_capture(crosstalk_eda_ui_scope_ctx_t *ctx, uint64_t sim_ns);
uint32_t crosstalk_eda_ui_scope_freq_measure(const crosstalk_eda_ui_scope_ctx_t *ctx, uint64_t *out);
uint32_t crosstalk_eda_ui_scope_win_reg(crosstalk_eda_ui_scope_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot);
uint32_t crosstalk_eda_ui_scope_mem_attach(crosstalk_eda_ui_scope_ctx_t *ctx, uint64_t addr, uint32_t width, uint8_t fixed, const char *label, uint32_t *out_idx);
uint32_t crosstalk_eda_ui_scope_mem_clear(crosstalk_eda_ui_scope_ctx_t *ctx);
uint32_t crosstalk_eda_ui_scope_mem_capture(crosstalk_eda_ui_scope_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, const char *sig);
uint32_t crosstalk_eda_ui_scope_xy_set(crosstalk_eda_ui_scope_ctx_t *ctx, uint32_t trace_x, uint32_t trace_y);
uint8_t crosstalk_eda_ui_scope_mem_live_is(const crosstalk_eda_ui_scope_ctx_t *ctx);

#endif
