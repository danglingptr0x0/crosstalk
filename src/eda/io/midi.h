#ifndef CROSSTALK_EDA_IO_MIDI_H
#define CROSSTALK_EDA_IO_MIDI_H

#include <stdint.h>

#include <dangling/io/midi.h>

#include "hdl/abi.h"
#include "hdl/exec.h"

#define CROSSTALK_EDA_IO_MIDI_ERR_PORT 925
#define CROSSTALK_EDA_IO_MIDI_ERR_LIVE 926
#define CROSSTALK_EDA_IO_MIDI_ERR_FULL 927
#define CROSSTALK_EDA_IO_MIDI_ERR_DEV 928
#define CROSSTALK_EDA_IO_MIDI_ERR_BUSTED 929

#define CROSSTALK_EDA_IO_MIDI_PORT_CAP 64
#define CROSSTALK_EDA_IO_MIDI_QUEUE_CAP 1024
#define CROSSTALK_EDA_IO_MIDI_QUEUE_MASK (CROSSTALK_EDA_IO_MIDI_QUEUE_CAP - 1)
#define CROSSTALK_EDA_IO_MIDI_MSG_CAP 64
#define CROSSTALK_EDA_IO_MIDI_WIRE_CAP 3
#define CROSSTALK_EDA_IO_MIDI_BAUD 31250
#define CROSSTALK_EDA_IO_MIDI_FRAME_BITS 10
#define CROSSTALK_EDA_IO_MIDI_DATA_SHIFT 1
#define CROSSTALK_EDA_IO_MIDI_STOP_BIT 0x200
#define CROSSTALK_EDA_IO_MIDI_PIN_LOW 0
#define CROSSTALK_EDA_IO_MIDI_PIN_HIGH 1
#define CROSSTALK_EDA_IO_MIDI_REFRESH_SECS 1.0

#define CROSSTALK_EDA_IO_MIDI_FILE_CAP 262144
#define CROSSTALK_EDA_IO_MIDI_TRACK_MAX 32
#define CROSSTALK_EDA_IO_MIDI_PATH_CAP 512
#define CROSSTALK_EDA_IO_MIDI_HDR_LEN 14
#define CROSSTALK_EDA_IO_MIDI_CHUNK_HDR 8
#define CROSSTALK_EDA_IO_MIDI_TEMPO_DEFAULT 500000
#define CROSSTALK_EDA_IO_MIDI_USEC_PER_SEC 1000000
#define CROSSTALK_EDA_IO_MIDI_FMT_MAX 1
#define CROSSTALK_EDA_IO_MIDI_SMPTE_BIT 0x8000
#define CROSSTALK_EDA_IO_MIDI_META 255
#define CROSSTALK_EDA_IO_MIDI_META_TEMPO 0x51
#define CROSSTALK_EDA_IO_MIDI_META_END 47
#define CROSSTALK_EDA_IO_MIDI_SYSEX 0xF0
#define CROSSTALK_EDA_IO_MIDI_SYSEX_ESC 0xF7
#define CROSSTALK_EDA_IO_MIDI_VLQ_MASK 127
#define CROSSTALK_EDA_IO_MIDI_VLQ_CONT 0x80
#define CROSSTALK_EDA_IO_MIDI_VLQ_SHIFT 7
#define CROSSTALK_EDA_IO_MIDI_CC_STATUS 0xB0
#define CROSSTALK_EDA_IO_MIDI_CC_ALL_OFF 123
#define CROSSTALK_EDA_IO_MIDI_NOTE_ON 0x90
#define CROSSTALK_EDA_IO_MIDI_NOTE_OFF 0x80
#define CROSSTALK_EDA_IO_MIDI_VEL_DEFAULT 100
#define CROSSTALK_EDA_IO_MIDI_TEMPO_BYTES 3

typedef struct crosstalk_eda_io_midi_track
{
    uint64_t tick;
    uint32_t at;
    uint32_t end;
    uint8_t status;
    uint8_t done;
    uint8_t pudding[6];
} crosstalk_eda_io_midi_track_t;

typedef struct crosstalk_eda_io_midi_pin
{
    char port[CROSSTALK_EDA_IO_MIDI_PORT_CAP];
    uint32_t sig_idx;
    uint32_t abi_sig;
    uint8_t bound;
    uint8_t pudding[7];
} crosstalk_eda_io_midi_pin_t;

typedef struct crosstalk_eda_io_midi_ctx
{
    ldg_io_midi_in_t dev;
    ldg_io_midi_port_t port[LDG_IO_MIDI_PORT_MAX];
    ldg_io_midi_msg_t msg[CROSSTALK_EDA_IO_MIDI_MSG_CAP];
    crosstalk_eda_io_midi_pin_t pin;
    char dev_name[LDG_IO_MIDI_PORT_NAME_MAX];
    uint8_t queue[CROSSTALK_EDA_IO_MIDI_QUEUE_CAP];
    uint8_t file[CROSSTALK_EDA_IO_MIDI_FILE_CAP];
    crosstalk_eda_io_midi_track_t track[CROSSTALK_EDA_IO_MIDI_TRACK_MAX];
    char file_path[CROSSTALK_EDA_IO_MIDI_PATH_CAP];
    double refresh_acc;
    uint64_t dev_idx;
    uint64_t freq;
    uint64_t phase;
    uint64_t msgs_total;
    uint64_t bytes_total;
    uint64_t dropped;
    uint64_t play_tick;
    uint64_t play_phase;
    uint64_t tick_rate;
    uint64_t tick_span;
    uint64_t tempo;
    uint32_t file_len;
    uint32_t tick_div;
    uint32_t track_cunt;
    uint32_t port_cunt;
    uint32_t wr_at;
    uint32_t rd_at;
    uint32_t frame;
    uint32_t bit_at;
    uint32_t slot_idx;
    uint8_t sending;
    uint8_t line;
    uint8_t dev_open;
    uint8_t busted;
    uint8_t live;
    uint8_t playing;
    uint8_t ok;
    uint8_t pudding[1];
} crosstalk_eda_io_midi_ctx_t;

uint32_t crosstalk_eda_io_midi_init(crosstalk_eda_io_midi_ctx_t *ctx);
uint32_t crosstalk_eda_io_midi_shutdown(crosstalk_eda_io_midi_ctx_t *ctx);
uint32_t crosstalk_eda_io_midi_ports_refresh(crosstalk_eda_io_midi_ctx_t *ctx);
uint32_t crosstalk_eda_io_midi_port_cunt_get(const crosstalk_eda_io_midi_ctx_t *ctx, uint32_t *out);
uint32_t crosstalk_eda_io_midi_port_name_get(const crosstalk_eda_io_midi_ctx_t *ctx, uint32_t idx, const char **out);
uint32_t crosstalk_eda_io_midi_dev_open(crosstalk_eda_io_midi_ctx_t *ctx, uint32_t idx);
uint32_t crosstalk_eda_io_midi_dev_close(crosstalk_eda_io_midi_ctx_t *ctx);
uint32_t crosstalk_eda_io_midi_dev_name_get(const crosstalk_eda_io_midi_ctx_t *ctx, const char **out);
uint32_t crosstalk_eda_io_midi_attach(crosstalk_eda_io_midi_ctx_t *ctx, uint32_t slot_idx, const crosstalk_hdl_exec_ctx_t *exec, const char *pin, uint64_t freq);
uint32_t crosstalk_eda_io_midi_detach(crosstalk_eda_io_midi_ctx_t *ctx);
uint32_t crosstalk_eda_io_midi_slot_detach(crosstalk_eda_io_midi_ctx_t *ctx, uint32_t slot_idx);
uint32_t crosstalk_eda_io_midi_msg_put(crosstalk_eda_io_midi_ctx_t *ctx, const ldg_io_midi_msg_t *msg);
uint32_t crosstalk_eda_io_midi_bytes_put(crosstalk_eda_io_midi_ctx_t *ctx, const uint8_t *bytes, uint32_t len);
uint32_t crosstalk_eda_io_midi_note_put(crosstalk_eda_io_midi_ctx_t *ctx, uint8_t note, uint8_t vel);
uint32_t crosstalk_eda_io_midi_quiet_put(crosstalk_eda_io_midi_ctx_t *ctx);
uint32_t crosstalk_eda_io_midi_file_play(crosstalk_eda_io_midi_ctx_t *ctx, const char *path);
uint32_t crosstalk_eda_io_midi_file_stop(crosstalk_eda_io_midi_ctx_t *ctx);
uint8_t crosstalk_eda_io_midi_playing_is(const crosstalk_eda_io_midi_ctx_t *ctx);
uint32_t crosstalk_eda_io_midi_pump(crosstalk_eda_io_midi_ctx_t *ctx);
uint32_t crosstalk_eda_io_midi_tick(crosstalk_eda_io_midi_ctx_t *ctx, double dt);
uint32_t crosstalk_eda_io_midi_cycle_drive(crosstalk_eda_io_midi_ctx_t *ctx, uint32_t slot_idx, crosstalk_hdl_exec_ctx_t *exec, const crosstalk_hdl_abi_t *abi);
uint32_t crosstalk_eda_io_midi_pending_get(const crosstalk_eda_io_midi_ctx_t *ctx, uint32_t *out);
uint8_t crosstalk_eda_io_midi_live_is(const crosstalk_eda_io_midi_ctx_t *ctx);
uint8_t crosstalk_eda_io_midi_dev_open_is(const crosstalk_eda_io_midi_ctx_t *ctx);

#endif
