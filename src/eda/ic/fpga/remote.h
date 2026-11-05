#ifndef CROSSTALK_EDA_IC_FPGA_REMOTE_H
#define CROSSTALK_EDA_IC_FPGA_REMOTE_H

#include <stdint.h>

#include "eda/ic/fpga/fpga.h"
#include "eda/ic/fpga/run.h"

#define CROSSTALK_EDA_IC_FPGA_REMOTE_IO_MAP_CAP 32
#define CROSSTALK_EDA_IC_FPGA_REMOTE_IO_NAME_CAP 64
#define CROSSTALK_EDA_IC_FPGA_REMOTE_FB_WORDS_MAX 262144
#define CROSSTALK_EDA_IC_FPGA_REMOTE_ENTRY_CAP 1024
#define CROSSTALK_EDA_IC_FPGA_REMOTE_KEY_CAP 96
#define CROSSTALK_EDA_IC_FPGA_REMOTE_VAL_MAX 196608
#define CROSSTALK_EDA_IC_FPGA_REMOTE_ARENA_SIZE 33554432
#define CROSSTALK_EDA_IC_FPGA_REMOTE_REFRESH_MS 100
#define CROSSTALK_EDA_IC_FPGA_REMOTE_EVICT_MS 5000
#define CROSSTALK_EDA_IC_FPGA_REMOTE_WR_CAP 64
#define CROSSTALK_EDA_IC_FPGA_REMOTE_WR_ARGS_CAP 4096
#define CROSSTALK_EDA_IC_FPGA_REMOTE_IDX_NONE UINT32_MAX
#define CROSSTALK_EDA_IC_FPGA_REMOTE_HDR_PUDDING_SIZE 4
#define CROSSTALK_EDA_IC_FPGA_REMOTE_ENTRY_PUDDING_SIZE 6
#define CROSSTALK_EDA_IC_FPGA_REMOTE_PUDDING_SIZE 2
#define CROSSTALK_EDA_IC_FPGA_REMOTE_IR_MAX 4194304

typedef enum crosstalk_eda_ic_fpga_remote_kind
{
    CROSSTALK_EDA_IC_FPGA_REMOTE_KIND_NONE = 0,
    CROSSTALK_EDA_IC_FPGA_REMOTE_KIND_REQ,
    CROSSTALK_EDA_IC_FPGA_REMOTE_KIND_WR
} crosstalk_eda_ic_fpga_remote_kind_t;

typedef struct crosstalk_eda_ic_fpga_remote_req_hdr
{
    uint32_t func;
    uint32_t slot;
    uint32_t key_len;
    uint32_t val_cap;
} crosstalk_eda_ic_fpga_remote_req_hdr_t;

typedef struct crosstalk_eda_ic_fpga_remote_resp_hdr
{
    uint32_t func;
    uint32_t slot;
    uint32_t key_len;
    uint32_t ret;
    uint32_t val_len;
    uint8_t pudding[CROSSTALK_EDA_IC_FPGA_REMOTE_HDR_PUDDING_SIZE];
} crosstalk_eda_ic_fpga_remote_resp_hdr_t;

typedef struct crosstalk_eda_ic_fpga_remote_wr_hdr
{
    uint32_t func;
    uint32_t slot;
    uint32_t args_len;
    uint8_t pudding[CROSSTALK_EDA_IC_FPGA_REMOTE_HDR_PUDDING_SIZE];
} crosstalk_eda_ic_fpga_remote_wr_hdr_t;

typedef struct crosstalk_eda_ic_fpga_remote_entry
{
    uint8_t key[CROSSTALK_EDA_IC_FPGA_REMOTE_KEY_CAP];
    uint64_t key_hash;
    uint64_t val_off;
    uint64_t recv_ms;
    uint64_t req_ms;
    uint64_t used_ms;
    uint32_t func;
    uint32_t slot;
    uint32_t key_len;
    uint32_t val_cap;
    uint32_t val_len;
    uint32_t ret;
    uint8_t live;
    uint8_t recv;
    uint8_t pudding[CROSSTALK_EDA_IC_FPGA_REMOTE_ENTRY_PUDDING_SIZE];
} crosstalk_eda_ic_fpga_remote_entry_t;

typedef struct crosstalk_eda_ic_fpga_remote_wr
{
    uint8_t args[CROSSTALK_EDA_IC_FPGA_REMOTE_WR_ARGS_CAP];
    uint32_t func;
    uint32_t slot;
    uint32_t args_len;
    uint8_t pudding[CROSSTALK_EDA_IC_FPGA_REMOTE_HDR_PUDDING_SIZE];
} crosstalk_eda_ic_fpga_remote_wr_t;

typedef struct crosstalk_eda_ic_fpga_remote
{
    crosstalk_eda_ic_fpga_remote_entry_t entry[CROSSTALK_EDA_IC_FPGA_REMOTE_ENTRY_CAP];
    crosstalk_eda_ic_fpga_remote_wr_t wr[CROSSTALK_EDA_IC_FPGA_REMOTE_WR_CAP];
    crosstalk_eda_ic_fpga_mem_watch_t watch[CROSSTALK_EDA_IC_FPGA_MEM_WATCH_MAX];
    char io_name[CROSSTALK_EDA_IC_FPGA_REMOTE_IO_MAP_CAP][CROSSTALK_EDA_IC_FPGA_REMOTE_IO_NAME_CAP];
    char top[CROSSTALK_EDA_IC_FPGA_TOP_CAP];
    crosstalk_hdl_ir_t ir;
    crosstalk_eda_ic_fpga_run_ctx_t *run;
    uint8_t *arena;
    uint8_t *buff;
    uint64_t *fb;
    uint8_t *ir_blob;
    uint64_t arena_used;
    uint64_t now_ms;
    uint32_t entry_cunt;
    uint32_t req_cursor;
    uint32_t wr_hd;
    uint32_t wr_cunt;
    uint32_t ir_len;
    uint32_t ir_gen;
    uint32_t ir_slot;
    uint8_t on;
    uint8_t ir_ok;
    uint8_t pudding[CROSSTALK_EDA_IC_FPGA_REMOTE_PUDDING_SIZE];
} crosstalk_eda_ic_fpga_remote_t;

uint32_t crosstalk_eda_ic_fpga_remote_init(crosstalk_eda_ic_fpga_remote_t *remote);
uint32_t crosstalk_eda_ic_fpga_remote_shutdown(crosstalk_eda_ic_fpga_remote_t *remote);
uint32_t crosstalk_eda_ic_fpga_remote_on_set(crosstalk_eda_ic_fpga_remote_t *remote, uint8_t on);
uint8_t crosstalk_eda_ic_fpga_remote_on_is(const crosstalk_eda_ic_fpga_remote_t *remote);
uint32_t crosstalk_eda_ic_fpga_remote_tick(crosstalk_eda_ic_fpga_remote_t *remote, uint64_t now_ms);
uint32_t crosstalk_eda_ic_fpga_remote_get(crosstalk_eda_ic_fpga_remote_t *remote, uint32_t func, uint32_t slot, const void *key, uint32_t key_len, void *out_val, uint32_t val_cap, uint32_t *out_len);
uint32_t crosstalk_eda_ic_fpga_remote_wr(crosstalk_eda_ic_fpga_remote_t *remote, uint32_t func, uint32_t slot, const void *args, uint32_t args_len);
uint32_t crosstalk_eda_ic_fpga_remote_out_next(crosstalk_eda_ic_fpga_remote_t *remote, uint8_t *out, uint32_t cap, uint32_t *out_len, uint32_t *out_kind);
uint32_t crosstalk_eda_ic_fpga_remote_resp_put(crosstalk_eda_ic_fpga_remote_t *remote, const uint8_t *payload, uint32_t len);
uint32_t crosstalk_eda_ic_fpga_remote_req_decode(const uint8_t *payload, uint32_t len, crosstalk_eda_ic_fpga_remote_req_hdr_t *out_hdr, const uint8_t **out_key);
uint32_t crosstalk_eda_ic_fpga_remote_wr_decode(const uint8_t *payload, uint32_t len, crosstalk_eda_ic_fpga_remote_wr_hdr_t *out_hdr, const uint8_t **out_args);
uint32_t crosstalk_eda_ic_fpga_remote_resp_encode(uint8_t *out, uint32_t cap, const crosstalk_eda_ic_fpga_remote_req_hdr_t *req, const uint8_t *key, uint32_t ret, const void *val, uint32_t val_len, uint32_t *out_len);

uint8_t crosstalk_eda_ic_fpga_remote_active_is(const crosstalk_eda_ic_fpga_ctx_t *ctx);
uint32_t crosstalk_eda_ic_fpga_remote_bind(crosstalk_eda_ic_fpga_ctx_t *ctx, crosstalk_eda_ic_fpga_remote_t *remote);
uint32_t crosstalk_eda_ic_fpga_remote_run_bind(crosstalk_eda_ic_fpga_remote_t *remote, crosstalk_eda_ic_fpga_run_ctx_t *run);
uint32_t crosstalk_eda_ic_fpga_remote_serve(const crosstalk_eda_ic_fpga_ctx_t *ctx, crosstalk_eda_ic_fpga_remote_t *srv, const uint8_t *payload, uint32_t len, const uint8_t **out_frame, uint32_t *out_len);
uint32_t crosstalk_eda_ic_fpga_remote_apply(crosstalk_eda_ic_fpga_ctx_t *ctx, const uint8_t *payload, uint32_t len);

uint32_t crosstalk_eda_ic_fpga_remote_slot_freq_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t *out);
uint8_t crosstalk_eda_ic_fpga_remote_slot_hlt_is(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx);
uint8_t crosstalk_eda_ic_fpga_remote_slot_live_is(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx);
uint8_t crosstalk_eda_ic_fpga_remote_running_is(const crosstalk_eda_ic_fpga_ctx_t *ctx);
const char* crosstalk_eda_ic_fpga_remote_running_top_get(const crosstalk_eda_ic_fpga_ctx_t *ctx);
uint32_t crosstalk_eda_ic_fpga_remote_cycles_total_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t *out_cycles);
uint32_t crosstalk_eda_ic_fpga_remote_verdict_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, crosstalk_eda_ic_fpga_verdict_t *out);
uint32_t crosstalk_eda_ic_fpga_remote_pin_rd(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *port, uint64_t *out_val);
uint32_t crosstalk_eda_ic_fpga_remote_sig_peek(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t *out_val, uint32_t *out_width);
uint32_t crosstalk_eda_ic_fpga_remote_mem_width_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t *out_width, uint32_t *out_elem_width);
uint32_t crosstalk_eda_ic_fpga_remote_mem_offset_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t *out);
uint32_t crosstalk_eda_ic_fpga_remote_mem_rd(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t addr, uint64_t *out, uint32_t cunt, uint32_t *out_rd);
uint32_t crosstalk_eda_ic_fpga_remote_mem_byte_rd(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t byte_at, uint64_t *out, uint32_t cunt, uint32_t *out_rd);
uint32_t crosstalk_eda_ic_fpga_remote_dsu_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t *out, uint32_t cap);
uint32_t crosstalk_eda_ic_fpga_remote_emu_state_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint32_t kind, uint64_t *out, uint32_t cap);
uint32_t crosstalk_eda_ic_fpga_remote_mem_acc_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t *out, uint32_t cap);
uint32_t crosstalk_eda_ic_fpga_remote_fault_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t *out, uint32_t cap);
uint32_t crosstalk_eda_ic_fpga_remote_ring_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t *out, uint32_t cap, uint32_t *out_cunt);
uint32_t crosstalk_eda_ic_fpga_remote_prof_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint32_t kind, uint64_t **out_base, uint32_t *out_cunt);
uint32_t crosstalk_eda_ic_fpga_remote_audio_rate_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint32_t *out);
uint32_t crosstalk_eda_ic_fpga_remote_mem_watch_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t idx, const crosstalk_eda_ic_fpga_mem_watch_t **out);
uint32_t crosstalk_eda_ic_fpga_remote_ddu_frame_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t *dst, uint64_t cap, uint32_t *out_w, uint32_t *out_h);
uint32_t crosstalk_eda_ic_fpga_remote_ddu_text_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t *dst, uint64_t cap, uint32_t *out_cols, uint32_t *out_rows);
uint32_t crosstalk_eda_ic_fpga_remote_io_rd(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t addr, uint64_t *out);
uint32_t crosstalk_eda_ic_fpga_remote_io_map_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint32_t idx, uint64_t *out_base, uint64_t *out_span, const char **out_name);
uint32_t crosstalk_eda_ic_fpga_remote_slot_ir_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const crosstalk_hdl_ir_t **out_ir, const char **out_src);
uint32_t crosstalk_eda_ic_fpga_remote_slot_hlt_set(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t on);
uint32_t crosstalk_eda_ic_fpga_remote_slot_gate_set(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t on);
uint32_t crosstalk_eda_ic_fpga_remote_clk_domain_set(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint32_t domain);
uint32_t crosstalk_eda_ic_fpga_remote_pin_wr(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *port, uint64_t val);
uint32_t crosstalk_eda_ic_fpga_remote_sig_poke(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t val, uint8_t quiet);
uint32_t crosstalk_eda_ic_fpga_remote_prof_en_set(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t on);
uint32_t crosstalk_eda_ic_fpga_remote_prof_en(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t on);
uint32_t crosstalk_eda_ic_fpga_remote_mem_watch_clear(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx);
uint32_t crosstalk_eda_ic_fpga_remote_brk_set(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t addr, uint8_t on);
uint32_t crosstalk_eda_ic_fpga_remote_io_wr(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t addr, uint64_t val);
uint32_t crosstalk_eda_ic_fpga_remote_watch_set(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t lo, uint64_t hi, uint8_t on);
uint32_t crosstalk_eda_ic_fpga_remote_brk_skip_set(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t cunt);
uint32_t crosstalk_eda_ic_fpga_remote_brk_cond_set(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t reg, uint64_t min);
uint32_t crosstalk_eda_ic_fpga_remote_emu_step_set(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint64_t cunt);
uint32_t crosstalk_eda_ic_fpga_remote_hlt_mask_set(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint32_t mask);
uint32_t crosstalk_eda_ic_fpga_remote_mem_wr(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t addr, const uint64_t *vals, uint32_t cunt, uint32_t *out_wrote);
uint32_t crosstalk_eda_ic_fpga_remote_run_rate_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint64_t *out_hz);
uint32_t crosstalk_eda_ic_fpga_remote_run_pin_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint64_t *out);
uint32_t crosstalk_eda_ic_fpga_remote_run_cycles_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint64_t *out_cycles);
uint8_t crosstalk_eda_ic_fpga_remote_run_live_is(const crosstalk_eda_ic_fpga_ctx_t *ctx);
uint32_t crosstalk_eda_ic_fpga_remote_run_start(const crosstalk_eda_ic_fpga_ctx_t *ctx);
uint32_t crosstalk_eda_ic_fpga_remote_run_hlt(const crosstalk_eda_ic_fpga_ctx_t *ctx);
uint32_t crosstalk_eda_ic_fpga_remote_run_pin_set(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint64_t hz);

#endif
