#ifndef CROSSTALK_EDA_IO_CAST_H
#define CROSSTALK_EDA_IO_CAST_H

#include <stdint.h>

#include <dangling/core/macros.h>

#include "eda/ic/fpga/fpga.h"
#include "eda/io/serial.h"

#define CROSSTALK_EDA_IO_CAST_ERR_GEOMETRY 990

#define CROSSTALK_EDA_IO_CAST_COLS 80
#define CROSSTALK_EDA_IO_CAST_ROWS 25
#define CROSSTALK_EDA_IO_CAST_PAYLOAD (CROSSTALK_EDA_IO_CAST_COLS * CROSSTALK_EDA_IO_CAST_ROWS)
#define CROSSTALK_EDA_IO_CAST_SIG_CAP 64
#define CROSSTALK_EDA_IO_CAST_FRAME_CAP 2048
#define CROSSTALK_EDA_IO_CAST_SRC_W_MAX 640
#define CROSSTALK_EDA_IO_CAST_PERIOD_SECS 2.0

typedef struct crosstalk_eda_io_cast_ctx
{
    char sig[CROSSTALK_EDA_IO_CAST_SIG_CAP];
    uint8_t frame[CROSSTALK_EDA_IO_CAST_FRAME_CAP];
    uint64_t base;
    uint64_t prev_hash;
    uint64_t frame_cunt;
    double next;
    uint32_t src_w;
    uint32_t src_h;
    uint32_t frame_len;
    uint32_t frame_at;
    uint8_t on;
    uint8_t pudding[7];
} crosstalk_eda_io_cast_ctx_t;

uint32_t crosstalk_eda_io_cast_init(crosstalk_eda_io_cast_ctx_t *ctx);
uint32_t crosstalk_eda_io_cast_start(crosstalk_eda_io_cast_ctx_t *ctx, const char *sig, uint64_t base, uint32_t src_w, uint32_t src_h);
uint32_t crosstalk_eda_io_cast_stop(crosstalk_eda_io_cast_ctx_t *ctx);
uint8_t crosstalk_eda_io_cast_on_is(const crosstalk_eda_io_cast_ctx_t *ctx);
uint32_t crosstalk_eda_io_cast_tick(crosstalk_eda_io_cast_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, crosstalk_eda_io_serial_ctx_t *serial, double now);

#endif
