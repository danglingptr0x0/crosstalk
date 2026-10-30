#ifndef CROSSTALK_EDA_IC_FPGA_H
#define CROSSTALK_EDA_IC_FPGA_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>

#include "hdl/abi.h"
#include "hdl/guard.h"
#include "eda/ic/fpga/emu.h"
#include "hdl/ast.h"
#include "hdl/diag/diag.h"
#include "hdl/elab.h"
#include "hdl/exec.h"
#include "hdl/ir.h"
#include "hdl/parse.h"
#include "hdl/preproc.h"
#include "hdl/systask.h"

#include "eda/io/display.h"
#include "eda/io/midi.h"
#include "eda/io/receptacle.h"
#include "eda/logic/probe.h"

#define CROSSTALK_EDA_IC_FPGA_ERR_SLOT 950
#define CROSSTALK_EDA_IC_FPGA_ERR_SRC_CAP 951
#define CROSSTALK_EDA_IC_FPGA_ERR_COMP 952
#define CROSSTALK_EDA_IC_FPGA_ERR_OVER_CAP 953
#define CROSSTALK_EDA_IC_FPGA_ERR_INOUT 954
#define CROSSTALK_EDA_IC_FPGA_ERR_CLOCK 955
#define CROSSTALK_EDA_IC_FPGA_ERR_BIND 956
#define CROSSTALK_EDA_IC_FPGA_ERR_POOL 957

#define CROSSTALK_EDA_IC_FPGA_SLOT_MAX 8
#define CROSSTALK_EDA_IC_FPGA_SRC_POOL_CAP (CROSSTALK_EDA_IC_FPGA_SLOT_MAX + 1)
#define CROSSTALK_EDA_IC_FPGA_SRC_CAP 1048576
#define CROSSTALK_EDA_IC_FPGA_SRC_CAP_MAX (CROSSTALK_EDA_IC_FPGA_SRC_CAP * 64ULL)
#define CROSSTALK_EDA_IC_FPGA_TOP_CAP 64
#define CROSSTALK_EDA_IC_FPGA_ELAB_POOL_BYTES (8 * LDG_MIB)
#define CROSSTALK_EDA_IC_FPGA_ELAB_POOL_BYTES_MAX ((uint64_t)4 * LDG_GIB)
#define CROSSTALK_EDA_IC_FPGA_POOL_GROW_FACTOR 2
#define CROSSTALK_EDA_IC_FPGA_AST_CAP 65536
#define CROSSTALK_EDA_IC_FPGA_IR_SIG_CAP 131072
#define CROSSTALK_EDA_IC_FPGA_IR_COMB_CAP 131072
#define CROSSTALK_EDA_IC_FPGA_IR_SEQ_OP_CAP 65536
#define CROSSTALK_EDA_IC_FPGA_IR_SEQ_BLK_CAP 256
#define CROSSTALK_EDA_IC_FPGA_BUILD_CAP_GROWTH_MAX 64ULL
#define CROSSTALK_EDA_IC_FPGA_ELAB_ERR_TOK_LOG_MAX 48
#define CROSSTALK_EDA_IC_FPGA_ELAB_ERR_LINE_LOG_MAX 255

#define CROSSTALK_EDA_IC_FPGA_COST_SCALE_ORD 2
#define CROSSTALK_EDA_IC_FPGA_COST_SCALE_SHIFT 3
#define CROSSTALK_EDA_IC_FPGA_COST_MUL_DIV 8
#define CROSSTALK_EDA_IC_FPGA_COST_DIVMOD_DIV 4

#define CROSSTALK_EDA_IC_FPGA_BUDGET_CYCLES 4096

#define CROSSTALK_EDA_IC_FPGA_PROG_STRIDE 64
#define CROSSTALK_EDA_IC_FPGA_STEP_CB_STRIDE 4096
#define CROSSTALK_EDA_IC_FPGA_PROG_RING_CAP 4096
#define CROSSTALK_EDA_IC_FPGA_PROG_SIG "loc"
#define CROSSTALK_EDA_IC_FPGA_PC_SIG "eip"
#define CROSSTALK_EDA_IC_FPGA_PC_CANDIDATE_MAX 24
#define CROSSTALK_EDA_IC_FPGA_EMU_SIG_WIDTH 64
#define CROSSTALK_EDA_IC_FPGA_EMU_ADDR_SPAN_BYTES 0x0001000000000000ULL
#define CROSSTALK_EDA_IC_FPGA_EMU_MEM_WIDTH_MAX 1073741824
#define CROSSTALK_EDA_IC_FPGA_EMU_STEP_GFX 4096
#define CROSSTALK_EDA_IC_FPGA_EMU_STEP_TEXT 1
#define CROSSTALK_EDA_IC_FPGA_SINGLE_STEP_CUNT 4
#define CROSSTALK_EDA_IC_FPGA_MEM_LD_MARK 1000000
#define CROSSTALK_EDA_IC_FPGA_EXC_CAUSE_SIG "exc_cause"

#define CROSSTALK_EDA_IC_FPGA_CLK_SIG "clk"

#define CROSSTALK_EDA_IC_FPGA_MEM_CHUNK 1024
#define CROSSTALK_EDA_IC_FPGA_MEM_WORD_BYTES 4
#define CROSSTALK_EDA_IC_FPGA_MEM_PAD 0x80
#define CROSSTALK_EDA_IC_FPGA_MEM_BYTE_CHUNK 1024
#define CROSSTALK_EDA_IC_FPGA_MEM_WATCH_MAX 8
#define CROSSTALK_EDA_IC_FPGA_MEM_WATCH_CK_SECS 1.0
#define CROSSTALK_EDA_IC_FPGA_MEM_WATCH_SETTLE_NS ((uint64_t)500 * LDG_NS_PER_MS)
#define CROSSTALK_EDA_IC_FPGA_MEM_HEX_EXT ".hex"
#define CROSSTALK_EDA_IC_FPGA_MEM_HEX_EXT_LEN 4
#define CROSSTALK_EDA_IC_FPGA_MEM_ADDR_MARK '@'
#define CROSSTALK_EDA_IC_FPGA_ERR_MEM_SIG 958
#define CROSSTALK_EDA_IC_FPGA_ERR_MEM_ROOM 959
#define CROSSTALK_EDA_IC_FPGA_REFUSED_CAP 16
#define CROSSTALK_EDA_IC_FPGA_REFUSED_SIG_ABSENT 0x0
#define CROSSTALK_EDA_IC_FPGA_REFUSED_SIG_NOT_MEM 0x1
#define CROSSTALK_EDA_IC_FPGA_REFUSED_ABI_ABSENT 0x2

#define CROSSTALK_EDA_IC_FPGA_DIR_CAP 512
#define CROSSTALK_EDA_IC_FPGA_INC_CAP 128
#define CROSSTALK_EDA_IC_FPGA_INC_BYTES_MAX 1048576
#define CROSSTALK_EDA_IC_FPGA_FILESET_MAX 128
#define CROSSTALK_EDA_IC_FPGA_FILESET_NAME_CAP 128
#define CROSSTALK_EDA_IC_FPGA_FILESET_SEP ','
#define CROSSTALK_EDA_IC_FPGA_FILESET_JOIN "\n"
#define CROSSTALK_EDA_IC_FPGA_Z386_TOP "z386_soc"
#define CROSSTALK_EDA_IC_FPGA_Z386_CLK "clk"
#define CROSSTALK_EDA_IC_FPGA_Z386_FILES "z386_soc.sv,z386_pkg.sv,z386.sv,alu.sv,biu.sv,decoder.sv,dsp_mul.sv,l1_cache.sv,l1_icache.sv,paging_tlb.sv,paging_unit.sv,paging_walker.sv,prefetch.sv,protection.sv,segmentation_unit.sv,ucode_rom.sv"

typedef struct crosstalk_eda_ic_fpga_comp
{
    uint64_t logic_cells;
    uint64_t user_io;
    uint64_t ram_bits;
    uint64_t blk_ram;
    uint64_t global_clks;
    uint64_t f_op_hz;
} crosstalk_eda_ic_fpga_comp_t;

typedef struct crosstalk_eda_ic_fpga_refused
{
    char sig[CROSSTALK_EDA_IC_FPGA_TOP_CAP];
    uint32_t cause;
    uint32_t slot_idx;
} crosstalk_eda_ic_fpga_refused_t;

typedef struct crosstalk_eda_ic_fpga_refused_log
{
    crosstalk_eda_ic_fpga_refused_t entry[CROSSTALK_EDA_IC_FPGA_REFUSED_CAP];
    uint32_t prog_gen[CROSSTALK_EDA_IC_FPGA_SLOT_MAX];
    uint32_t cunt;
    uint8_t pudding[4];
} crosstalk_eda_ic_fpga_refused_log_t;

typedef struct crosstalk_eda_ic_fpga_verdict
{
    uint64_t ff_bits;
    uint64_t lut_est;
    uint64_t cells_used;
    uint64_t cells_avail;
    uint64_t mem_bits;
    uint64_t mem_avail;
    uint64_t io_bits;
    uint64_t io_avail;
    uint8_t cells_fit;
    uint8_t mem_fit;
    uint8_t io_fit;
    uint8_t fits;
    uint8_t pudding[4];
} crosstalk_eda_ic_fpga_verdict_t;

typedef struct crosstalk_eda_ic_fpga_slot
{
    crosstalk_hdl_ir_t ir;
    crosstalk_hdl_ast_t ast;
    crosstalk_hdl_exec_ctx_t exec;
    crosstalk_hdl_level_res_t level_res;
    crosstalk_eda_ic_fpga_verdict_t verdict;
    crosstalk_hdl_diag_t diag;
    crosstalk_eda_ic_fpga_comp_t comp;
    const crosstalk_hdl_abi_t *abi;
    crosstalk_hdl_systask_ctx_t *systask;
    char *src;
    char *elab_src;
    uint64_t acc_ns;
    uint64_t cycles_run;
    uint64_t prog_ring[CROSSTALK_EDA_IC_FPGA_PROG_RING_CAP];
    uint32_t prog_rd;
    uint32_t prog_wr;
    uint32_t prog_exec_sig;
    uint32_t prog_abi_sig;
    uint32_t pc_exec_sig;
    uint32_t pc_abi_sig;
    uint32_t src_len;
    uint32_t elab_src_len;
    uint32_t clk_sig;
    uint32_t abi_clk_sig;
    uint32_t clk_domain;
    uint32_t prog_gen;
    uint32_t hlt_mask;
    uint32_t build_res;
    uint32_t gate_res;
    char top[CROSSTALK_EDA_IC_FPGA_TOP_CAP];
    char clk[CROSSTALK_EDA_IC_FPGA_TOP_CAP];
    char fileset_dir[CROSSTALK_EDA_IC_FPGA_DIR_CAP];
    char fileset_files[CROSSTALK_EDA_IC_FPGA_FILESET_MAX * CROSSTALK_EDA_IC_FPGA_FILESET_NAME_CAP];
    char diag_text[CROSSTALK_HDL_DIAG_TEXT_CAP];
    uint8_t live;
    uint8_t runnable;
    uint8_t hlt;
    uint8_t emu;
} crosstalk_eda_ic_fpga_slot_t;

typedef struct crosstalk_eda_ic_fpga_fileset_state
{
    char dir[CROSSTALK_EDA_IC_FPGA_DIR_CAP];
    char files[CROSSTALK_EDA_IC_FPGA_FILESET_MAX * CROSSTALK_EDA_IC_FPGA_FILESET_NAME_CAP];
    char top[CROSSTALK_EDA_IC_FPGA_TOP_CAP];
    char clk[CROSSTALK_EDA_IC_FPGA_TOP_CAP];
    char emu_path[CROSSTALK_EDA_IC_FPGA_DIR_CAP];
    uint64_t hold_hz;
    uint8_t emitted;
    uint8_t emu;
    uint8_t pudding[6];
} crosstalk_eda_ic_fpga_fileset_state_t;

typedef struct crosstalk_eda_ic_fpga_mem_watch
{
    char sig[CROSSTALK_EDA_IC_FPGA_TOP_CAP];
    char path[CROSSTALK_EDA_IC_FPGA_DIR_CAP];
    uint64_t word_off;
    uint64_t modified_ns;
    uint64_t span;
    uint32_t slot_idx;
    uint8_t used;
    uint8_t pudding[3];
} crosstalk_eda_ic_fpga_mem_watch_t;

typedef void (*crosstalk_eda_ic_fpga_step_cb_t)(const crosstalk_hdl_abi_t *abi, void *user);

typedef struct crosstalk_eda_ic_fpga_ctx
{
    ldg_mem_pool_t *src_pool;
    ldg_mem_pool_t *elab_pool;
    ldg_mem_pool_t *trial_pool;
    crosstalk_eda_logic_probe_ctx_t *probes;
    crosstalk_eda_io_display_ctx_t *display;
    crosstalk_eda_io_receptacle_ctx_t *receptacle;
    crosstalk_eda_io_midi_ctx_t *midi;
    crosstalk_hdl_guard_ctx_t *guard;
    crosstalk_hdl_preproc_include_cb_t include_cb;
    void *include_user;
    crosstalk_eda_ic_fpga_step_cb_t step_cb;
    void *step_user;
    crosstalk_hdl_systask_out_cb_t systask_cb;
    void *systask_user;
    uint64_t step_cb_acc;
    char src_dir[CROSSTALK_EDA_IC_FPGA_DIR_CAP];
    crosstalk_eda_ic_fpga_slot_t slot[CROSSTALK_EDA_IC_FPGA_SLOT_MAX];
    crosstalk_eda_ic_fpga_slot_t trial;
    crosstalk_eda_ic_fpga_mem_watch_t mem_watch[CROSSTALK_EDA_IC_FPGA_MEM_WATCH_MAX];
    crosstalk_eda_ic_fpga_refused_log_t refused_st;
    crosstalk_eda_ic_fpga_refused_log_t *refused;
    double mem_watch_next;
    uint64_t emu_step;
    uint64_t src_cap;
    uint32_t mem_watch_cunt;
    uint32_t live_cunt;
    uint32_t ast_cap;
    uint32_t ir_sig_cap;
    uint32_t ir_comb_cap;
    uint32_t ir_seq_op_cap;
    uint32_t ir_seq_blk_cap;
    uint8_t prof_on;
    uint8_t ok;
    uint8_t pudding[2];
} crosstalk_eda_ic_fpga_ctx_t;

uint32_t crosstalk_eda_ic_fpga_init(crosstalk_eda_ic_fpga_ctx_t *ctx, crosstalk_hdl_preproc_include_cb_t include_cb, void *include_user);
uint32_t crosstalk_eda_ic_fpga_shutdown(crosstalk_eda_ic_fpga_ctx_t *ctx);
uint32_t crosstalk_eda_ic_fpga_systask_cb_set(crosstalk_eda_ic_fpga_ctx_t *ctx, crosstalk_hdl_systask_out_cb_t cb, void *user);
uint32_t crosstalk_eda_ic_fpga_probes_bind(crosstalk_eda_ic_fpga_ctx_t *ctx, crosstalk_eda_logic_probe_ctx_t *probes);
uint32_t crosstalk_eda_ic_fpga_display_bind(crosstalk_eda_ic_fpga_ctx_t *ctx, crosstalk_eda_io_display_ctx_t *display);
uint32_t crosstalk_eda_ic_fpga_receptacle_bind(crosstalk_eda_ic_fpga_ctx_t *ctx, crosstalk_eda_io_receptacle_ctx_t *receptacle);
uint32_t crosstalk_eda_ic_fpga_midi_bind(crosstalk_eda_ic_fpga_ctx_t *ctx, crosstalk_eda_io_midi_ctx_t *midi);
uint32_t crosstalk_eda_ic_fpga_guard_bind(crosstalk_eda_ic_fpga_ctx_t *ctx, crosstalk_hdl_guard_ctx_t *guard);
uint32_t crosstalk_eda_ic_fpga_step_cb_set(crosstalk_eda_ic_fpga_ctx_t *ctx, crosstalk_eda_ic_fpga_step_cb_t cb, void *user);
uint32_t crosstalk_eda_ic_fpga_slot_exec_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const crosstalk_hdl_exec_ctx_t **out_exec);
uint32_t crosstalk_eda_ic_fpga_slot_gate_set(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t on);
uint32_t crosstalk_eda_ic_fpga_slot_ir_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const crosstalk_hdl_ir_t **out_ir, const char **out_src);
uint32_t crosstalk_eda_ic_fpga_slot_abi_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const crosstalk_hdl_abi_t **out_abi);
uint32_t crosstalk_eda_ic_fpga_slot_hlt_set(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t on);
uint8_t crosstalk_eda_ic_fpga_slot_hlt_is(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx);
uint32_t crosstalk_eda_ic_fpga_slot_freq_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t *out);
uint32_t crosstalk_eda_ic_fpga_program(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *src, uint32_t src_len, const char *top, const crosstalk_eda_ic_fpga_comp_t *caps, const char *clk_name);
uint32_t crosstalk_eda_ic_fpga_unprogram(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx);
uint32_t crosstalk_eda_ic_fpga_advance(crosstalk_eda_ic_fpga_ctx_t *ctx, double dt_secs, uint64_t budget_cycles);
uint32_t crosstalk_eda_ic_fpga_clk_domain_set(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint32_t domain);
uint32_t crosstalk_eda_ic_fpga_abi_set(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const crosstalk_hdl_abi_t *abi);
uint32_t crosstalk_eda_ic_fpga_emu_engage(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const crosstalk_hdl_abi_t *abi, const char *clk_name, uint64_t f_op_hz);
uint32_t crosstalk_eda_ic_fpga_cycles_run(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t cycles, uint64_t *out_done);
uint32_t crosstalk_eda_ic_fpga_cycles_total_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t *out_cycles);
uint32_t crosstalk_eda_ic_fpga_mem_rd(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t addr, uint64_t *out, uint32_t cunt, uint32_t *out_rd);
uint32_t crosstalk_eda_ic_fpga_mem_width_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t *out_width, uint32_t *out_elem_width);
uint32_t crosstalk_eda_ic_fpga_mem_offset_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t *out);
uint32_t crosstalk_eda_ic_fpga_mem_wr(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t addr, const uint64_t *vals, uint32_t cunt, uint32_t *out_wrote);
uint32_t crosstalk_eda_ic_fpga_verdict_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, crosstalk_eda_ic_fpga_verdict_t *out);
uint32_t crosstalk_eda_ic_fpga_pin_wr(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *port, uint64_t val);
uint32_t crosstalk_eda_ic_fpga_pin_rd(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *port, uint64_t *out_val);
uint8_t crosstalk_eda_ic_fpga_slot_live_is(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx);
uint8_t crosstalk_eda_ic_fpga_running_is(const crosstalk_eda_ic_fpga_ctx_t *ctx);
uint32_t crosstalk_eda_ic_fpga_prof_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint32_t kind, uint64_t **out_base, uint32_t *out_cunt);
uint32_t crosstalk_eda_ic_fpga_fault_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t *out, uint32_t cap);
uint32_t crosstalk_eda_ic_fpga_dsu_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t *out, uint32_t cap);
uint32_t crosstalk_eda_ic_fpga_mem_acc_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t *out, uint32_t cap);
uint32_t crosstalk_eda_ic_fpga_emu_state_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint32_t kind, uint64_t *out, uint32_t cap);
uint32_t crosstalk_eda_ic_fpga_mem_byte_rd(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t byte_at, uint64_t *out, uint32_t cunt, uint32_t *out_rd);
uint32_t crosstalk_eda_ic_fpga_prof_en_set(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t on);
uint32_t crosstalk_eda_ic_fpga_audio_drain(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t *dst, uint64_t cap, uint64_t *out_got);
uint32_t crosstalk_eda_ic_fpga_audio_rate_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint32_t *out);
uint32_t crosstalk_eda_ic_fpga_mem_watch_stale_get(crosstalk_eda_ic_fpga_ctx_t *ctx, double now, uint32_t *out_idx);
uint32_t crosstalk_eda_ic_fpga_mem_watch_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t idx, const crosstalk_eda_ic_fpga_mem_watch_t **out);
uint32_t crosstalk_eda_ic_fpga_mem_watch_clear(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx);
uint32_t crosstalk_eda_ic_fpga_hlt_mask_set(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint32_t mask);
uint32_t crosstalk_eda_ic_fpga_ring_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t *out, uint32_t cap, uint32_t *out_cunt);
uint32_t crosstalk_eda_ic_fpga_prof_en(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t on);
uint32_t crosstalk_eda_ic_fpga_emu_step_set(crosstalk_eda_ic_fpga_ctx_t *ctx, uint64_t cunt);
uint32_t crosstalk_eda_ic_fpga_ddu_frame_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t *dst, uint64_t cap, uint32_t *out_w, uint32_t *out_h);
uint32_t crosstalk_eda_ic_fpga_ddu_text_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t *dst, uint64_t cap, uint32_t *out_cols, uint32_t *out_rows);
uint32_t crosstalk_eda_ic_fpga_io_rd(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t addr, uint64_t *out);
uint32_t crosstalk_eda_ic_fpga_io_map_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint32_t idx, uint64_t *out_base, uint64_t *out_span, const char **out_name);
uint32_t crosstalk_eda_ic_fpga_io_wr(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t addr, uint64_t val);
uint32_t crosstalk_eda_ic_fpga_brk_set(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t addr, uint8_t on);
uint32_t crosstalk_eda_ic_fpga_brk_skip_set(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t cunt);
uint32_t crosstalk_eda_ic_fpga_brk_cond_set(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t reg, uint64_t min);
uint32_t crosstalk_eda_ic_fpga_watch_set(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t lo, uint64_t hi, uint8_t on);
uint32_t crosstalk_eda_ic_fpga_prog_drain(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t *out, uint32_t cap, uint32_t *out_cunt);
const char* crosstalk_eda_ic_fpga_running_top_get(const crosstalk_eda_ic_fpga_ctx_t *ctx);
uint32_t crosstalk_eda_ic_fpga_fileset_ck(crosstalk_eda_ic_fpga_ctx_t *ctx, const char *dir, const char *files, const char *top, const crosstalk_eda_ic_fpga_comp_t *caps, const char *clk_name);
uint32_t crosstalk_eda_ic_fpga_fileset_program(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *dir, const char *files, const char *top, const crosstalk_eda_ic_fpga_comp_t *caps, const char *clk_name, uint32_t *out_bytes);
uint32_t crosstalk_eda_ic_fpga_fileset_state_serialize(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t hold_hz, const char *emu_path, uint8_t *out, uint32_t cap, uint32_t *out_len);
uint32_t crosstalk_eda_ic_fpga_fileset_state_deserialize(const uint8_t *buff, uint32_t len, crosstalk_eda_ic_fpga_fileset_state_t *out);
uint32_t crosstalk_eda_ic_fpga_mem_ld(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, const char *path, uint64_t word_off, uint64_t *out_words, uint64_t *out_bytes);
uint32_t crosstalk_eda_ic_fpga_sig_poke(crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t val, uint8_t quiet);
uint32_t crosstalk_eda_ic_fpga_sig_peek(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t *out_val, uint32_t *out_width);

#endif
