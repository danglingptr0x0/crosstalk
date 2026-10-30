#ifndef CROSSTALK_EDA_UI_PROFILE_H
#define CROSSTALK_EDA_UI_PROFILE_H

#include <stdint.h>

#include <dangling/core/macros.h>

#include "eda/ic/fpga/fpga.h"
#include "hdl/elf.h"
#include "ui/window.h"

#define CROSSTALK_EDA_UI_PROFILE_WINDOW_ID 0x50524F46ULL
#define CROSSTALK_EDA_UI_PROFILE_TITLE "Profile"
#define CROSSTALK_EDA_UI_PROFILE_RING_CAP 4096
#define CROSSTALK_EDA_UI_PROFILE_SLOT_CAP 130
#define CROSSTALK_EDA_UI_PROFILE_META_CAP 16
#define CROSSTALK_EDA_UI_PROFILE_SUB_CAP 8
#define CROSSTALK_EDA_UI_PROFILE_NM_BYTES 32
#define CROSSTALK_EDA_UI_PROFILE_EVT_CAP 8
#define CROSSTALK_EDA_UI_PROFILE_EVT_NAME_CAP 24
#define CROSSTALK_EDA_UI_PROFILE_REFRESH_SECS 0.25
#define CROSSTALK_EDA_UI_PROFILE_PROG_TOP_CAP 512
#define CROSSTALK_EDA_UI_PROFILE_PROG_FRAME_CAP 4096
#define CROSSTALK_EDA_UI_PROFILE_PROG_DRAIN_CAP 512
#define CROSSTALK_EDA_UI_PROFILE_TAB_SIM 0
#define CROSSTALK_EDA_UI_PROFILE_TAB_PROG 1
#define CROSSTALK_EDA_UI_PROFILE_TAB_EMU 2
#define CROSSTALK_EDA_UI_PROFILE_TAB_CUNT 3
#define CROSSTALK_EDA_UI_PROFILE_FN_CAP 256
#define CROSSTALK_EDA_UI_PROFILE_FN_WORD_CUNT 1024
#define CROSSTALK_EDA_UI_PROFILE_FN_MEM_SIG "mem"

typedef struct crosstalk_eda_ui_profile_evt
{
    uint64_t agg_t[CROSSTALK_EDA_UI_PROFILE_SLOT_CAP];
    uint64_t agg_c[CROSSTALK_EDA_UI_PROFILE_SLOT_CAP];
    uint64_t sub[CROSSTALK_EDA_UI_PROFILE_SUB_CAP];
    uint64_t cyc;
    char name[CROSSTALK_EDA_UI_PROFILE_EVT_NAME_CAP];
    uint8_t used;
    uint8_t pudding[7];
} crosstalk_eda_ui_profile_evt_t;

typedef struct crosstalk_eda_ui_profile_ctx
{
    crosstalk_ui_window_state_t win;
    const crosstalk_hdl_elf_ctx_t *elf;
    uint64_t agg_t[CROSSTALK_EDA_UI_PROFILE_SLOT_CAP];
    uint64_t agg_c[CROSSTALK_EDA_UI_PROFILE_SLOT_CAP];
    uint64_t sub[CROSSTALK_EDA_UI_PROFILE_SUB_CAP];
    uint64_t base_t[CROSSTALK_EDA_UI_PROFILE_SLOT_CAP];
    uint64_t base_c[CROSSTALK_EDA_UI_PROFILE_SLOT_CAP];
    uint64_t base_sub[CROSSTALK_EDA_UI_PROFILE_SUB_CAP];
    uint64_t base_cyc;
    uint64_t snap_t[CROSSTALK_EDA_UI_PROFILE_SLOT_CAP];
    uint64_t snap_c[CROSSTALK_EDA_UI_PROFILE_SLOT_CAP];
    uint64_t snap_sub[CROSSTALK_EDA_UI_PROFILE_SUB_CAP];
    uint64_t snap_cyc;
    uint64_t ring_tot[CROSSTALK_EDA_UI_PROFILE_RING_CAP];
    uint64_t ring_comb[CROSSTALK_EDA_UI_PROFILE_RING_CAP];
    uint64_t ring_blk[CROSSTALK_EDA_UI_PROFILE_RING_CAP];
    uint64_t ring_cmt[CROSSTALK_EDA_UI_PROFILE_RING_CAP];
    uint64_t meta[CROSSTALK_EDA_UI_PROFILE_META_CAP];
    uint64_t emu_op[CROSSTALK_HDL_ABI_OP_CUNT];
    uint64_t emu_mem[CROSSTALK_HDL_ABI_MEM_CUNT];
    uint64_t emu_trap[CROSSTALK_HDL_ABI_TRAP_CUNT];
    uint64_t emu_host[CROSSTALK_HDL_ABI_HOST_CUNT];
    uint64_t sel_cyc;
    uint64_t strip_step;
    crosstalk_eda_ui_profile_evt_t evt[CROSSTALK_EDA_UI_PROFILE_EVT_CAP];
    char nm[CROSSTALK_EDA_UI_PROFILE_SLOT_CAP][CROSSTALK_EDA_UI_PROFILE_NM_BYTES];
    char snap_name[CROSSTALK_EDA_UI_PROFILE_EVT_NAME_CAP];
    char pending_name[CROSSTALK_EDA_UI_PROFILE_EVT_NAME_CAP];
    uint64_t prog_addr[CROSSTALK_EDA_UI_PROFILE_PROG_TOP_CAP];
    uint64_t prog_hits[CROSSTALK_EDA_UI_PROFILE_PROG_TOP_CAP];
    uint64_t prog_total;
    uint64_t prog_drop;
    uint64_t fn_addr[CROSSTALK_EDA_UI_PROFILE_FN_CAP];
    uint64_t fn_hits[CROSSTALK_EDA_UI_PROFILE_FN_CAP];
    uint64_t fn_total;
    uint32_t fn_cunt;
    uint32_t fn_pudding;
    uint32_t prog_frame[CROSSTALK_EDA_UI_PROFILE_PROG_FRAME_CAP];
    double refresh_next;
    uint32_t slot_cunt;
    uint32_t blk_cunt;
    uint32_t view_evt;
    uint32_t evt_next;
    uint32_t agg_sel;
    uint32_t agg_sel_slot;
    uint8_t expand[CROSSTALK_EDA_UI_PROFILE_SLOT_CAP];
    uint8_t open;
    uint8_t bound;
    uint8_t evt_run;
    uint8_t pending_start;
    uint8_t pending_end;
    uint8_t pending_rst;
    uint8_t tab;
    uint8_t emu_bound;
    uint8_t pudding[4];
} crosstalk_eda_ui_profile_ctx_t;

uint32_t crosstalk_eda_ui_profile_init(crosstalk_eda_ui_profile_ctx_t *ctx);
uint32_t crosstalk_eda_ui_profile_open_set(crosstalk_eda_ui_profile_ctx_t *ctx, uint8_t open);
uint8_t crosstalk_eda_ui_profile_open_is(const crosstalk_eda_ui_profile_ctx_t *ctx);
uint32_t crosstalk_eda_ui_profile_mark_start(crosstalk_eda_ui_profile_ctx_t *ctx, const char *name);
uint32_t crosstalk_eda_ui_profile_mark_end(crosstalk_eda_ui_profile_ctx_t *ctx);
uint32_t crosstalk_eda_ui_profile_rst(crosstalk_eda_ui_profile_ctx_t *ctx);
uint32_t crosstalk_eda_ui_profile_tick(crosstalk_eda_ui_profile_ctx_t *ctx, crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, double now);
uint32_t crosstalk_eda_ui_profile_elf_bind(crosstalk_eda_ui_profile_ctx_t *ctx, const crosstalk_hdl_elf_ctx_t *elf);
uint32_t crosstalk_eda_ui_profile_fn_top_get(const crosstalk_eda_ui_profile_ctx_t *ctx, uint64_t *out);
uint32_t crosstalk_eda_ui_profile_win_reg(crosstalk_eda_ui_profile_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot);

#endif
