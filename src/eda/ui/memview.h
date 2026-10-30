#ifndef CROSSTALK_EDA_UI_MEMVIEW_H
#define CROSSTALK_EDA_UI_MEMVIEW_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/io/file.h>

#include "eda/ic/fpga/fpga.h"
#include "eda/ui/calc.h"
#include "hdl/elf.h"
#include "hdl/type.h"
#include "ui/window.h"

#define CROSSTALK_EDA_UI_MEMVIEW_ERR_BIND 985
#define CROSSTALK_EDA_UI_MEMVIEW_ERR_RANGE 986
#define CROSSTALK_EDA_UI_MEMVIEW_ERR_PATTERN 987

#define CROSSTALK_EDA_UI_MEMVIEW_WINDOW_ID 0x4D454D56ULL
#define CROSSTALK_EDA_UI_MEMVIEW_TITLE "Memory"
#define CROSSTALK_EDA_UI_MEMVIEW_TITLE_CAP 96

#define CROSSTALK_EDA_UI_MEMVIEW_SIG_CAP 64
#define CROSSTALK_EDA_UI_MEMVIEW_COL_CUNT 32
#define CROSSTALK_EDA_UI_MEMVIEW_ROW_CUNT 24
#define CROSSTALK_EDA_UI_MEMVIEW_PAGE_CAP (CROSSTALK_EDA_UI_MEMVIEW_COL_CUNT * CROSSTALK_EDA_UI_MEMVIEW_ROW_CUNT)
#define CROSSTALK_EDA_UI_MEMVIEW_BKT_CUNT 256
#define CROSSTALK_EDA_UI_MEMVIEW_SAMPLE_STRIDE 64
#define CROSSTALK_EDA_UI_MEMVIEW_BKT_SAMPLE_MAX 64
#define CROSSTALK_EDA_UI_MEMVIEW_PATTERN_CAP 32
#define CROSSTALK_EDA_UI_MEMVIEW_QUERY_CAP 192
#define CROSSTALK_EDA_UI_MEMVIEW_MOTION_WINDOW 65536
#define CROSSTALK_EDA_UI_MEMVIEW_HIST_MAX 32
#define CROSSTALK_EDA_UI_MEMVIEW_STRING_CAP 48
#define CROSSTALK_EDA_UI_MEMVIEW_HEX_LEAD "0x"
#define CROSSTALK_EDA_UI_MEMVIEW_FIND_BUDGET 65536
#define CROSSTALK_EDA_UI_MEMVIEW_HIT_CAP 512
#define CROSSTALK_EDA_UI_MEMVIEW_REFRESH_SECS 0.25
#define CROSSTALK_EDA_UI_MEMVIEW_CHURN_WINDOW_SECS 10.0
#define CROSSTALK_EDA_UI_MEMVIEW_CHURN_MAX 24
#define CROSSTALK_EDA_UI_MEMVIEW_IO_RANGE_CAP 16
#define CROSSTALK_EDA_UI_MEMVIEW_IO_NAME_CAP 24
#define CROSSTALK_EDA_UI_MEMVIEW_IO_FIRST 0x0001000000000000ULL
#define CROSSTALK_EDA_UI_MEMVIEW_OVERVIEW_SECS 4.0
#define CROSSTALK_EDA_UI_MEMVIEW_HEAP_CAP 2048
#define CROSSTALK_EDA_UI_MEMVIEW_HEAP_DEPTH 6
#define CROSSTALK_EDA_UI_MEMVIEW_HEAP_REC_MAX 4096
#define CROSSTALK_EDA_UI_MEMVIEW_HEAP_SECS 0.5
#define CROSSTALK_EDA_UI_MEMVIEW_FIND_MODE_PATTERN 0
#define CROSSTALK_EDA_UI_MEMVIEW_FIND_MODE_REF 1
#define CROSSTALK_EDA_UI_MEMVIEW_REF_WINDOW_DEFAULT 512
#define CROSSTALK_EDA_UI_MEMVIEW_PTR_FLOOR 0x1000
#define CROSSTALK_EDA_UI_MEMVIEW_RUN_SIG_CAP 128
#define CROSSTALK_EDA_UI_MEMVIEW_WHERE_TERM_MAX 4
#define CROSSTALK_EDA_UI_MEMVIEW_WHERE_PATH_CAP 64
#define CROSSTALK_EDA_UI_MEMVIEW_WHERE_OP_EQ 0
#define CROSSTALK_EDA_UI_MEMVIEW_WHERE_OP_NE 1
#define CROSSTALK_EDA_UI_MEMVIEW_WHERE_OP_LT 2
#define CROSSTALK_EDA_UI_MEMVIEW_WHERE_OP_GT 3
#define CROSSTALK_EDA_UI_MEMVIEW_WHERE_OP_LE 4
#define CROSSTALK_EDA_UI_MEMVIEW_WHERE_OP_GE 5
#define CROSSTALK_EDA_UI_MEMVIEW_RUN_GEN_MAX 6
#define CROSSTALK_EDA_UI_MEMVIEW_HEAT_HIT 24
#define CROSSTALK_EDA_UI_MEMVIEW_HEAT_DECAY_EVERY 16
#define CROSSTALK_EDA_UI_MEMVIEW_SEQ_SPAN 64
#define CROSSTALK_EDA_UI_MEMVIEW_DIS_ROW_CAP 32
#define CROSSTALK_EDA_UI_MEMVIEW_DIS_ROW_TEXT_CAP 96

typedef enum crosstalk_eda_ui_memview_res
{
    CROSSTALK_EDA_UI_MEMVIEW_RES_HITS = 0,
    CROSSTALK_EDA_UI_MEMVIEW_RES_DIS
} crosstalk_eda_ui_memview_res_t;

typedef enum crosstalk_eda_ui_memview_follow
{
    CROSSTALK_EDA_UI_MEMVIEW_FOLLOW_FIXED = 0,
    CROSSTALK_EDA_UI_MEMVIEW_FOLLOW_SIG
} crosstalk_eda_ui_memview_follow_t;

typedef enum crosstalk_eda_ui_memview_watch
{
    CROSSTALK_EDA_UI_MEMVIEW_WATCH_OFF = 0,
    CROSSTALK_EDA_UI_MEMVIEW_WATCH_AT,
    CROSSTALK_EDA_UI_MEMVIEW_WATCH_SCAN,
    CROSSTALK_EDA_UI_MEMVIEW_WATCH_MUT
} crosstalk_eda_ui_memview_watch_t;

typedef enum crosstalk_eda_ui_memview_motion
{
    CROSSTALK_EDA_UI_MEMVIEW_MOTION_OFF = 0,
    CROSSTALK_EDA_UI_MEMVIEW_MOTION_CHANGED,
    CROSSTALK_EDA_UI_MEMVIEW_MOTION_UP,
    CROSSTALK_EDA_UI_MEMVIEW_MOTION_DOWN,
    CROSSTALK_EDA_UI_MEMVIEW_MOTION_STEP,
    CROSSTALK_EDA_UI_MEMVIEW_MOTION_BOUND
} crosstalk_eda_ui_memview_motion_t;

typedef enum crosstalk_eda_ui_memview_motion_phase
{
    CROSSTALK_EDA_UI_MEMVIEW_MOTION_PHASE_SAMPLE = 0,
    CROSSTALK_EDA_UI_MEMVIEW_MOTION_PHASE_CMP
} crosstalk_eda_ui_memview_motion_phase_t;

typedef enum crosstalk_eda_ui_memview_tab
{
    CROSSTALK_EDA_UI_MEMVIEW_TAB_HEX = 0,
    CROSSTALK_EDA_UI_MEMVIEW_TAB_HITS
} crosstalk_eda_ui_memview_tab_t;

typedef enum crosstalk_eda_ui_memview_action
{
    CROSSTALK_EDA_UI_MEMVIEW_ACTION_NONE = 0,
    CROSSTALK_EDA_UI_MEMVIEW_ACTION_STEP,
    CROSSTALK_EDA_UI_MEMVIEW_ACTION_HLT,
    CROSSTALK_EDA_UI_MEMVIEW_ACTION_RUN,
    CROSSTALK_EDA_UI_MEMVIEW_ACTION_WATCH,
    CROSSTALK_EDA_UI_MEMVIEW_ACTION_BREAK,
    CROSSTALK_EDA_UI_MEMVIEW_ACTION_POKE,
    CROSSTALK_EDA_UI_MEMVIEW_ACTION_SNAP,
    CROSSTALK_EDA_UI_MEMVIEW_ACTION_DIFF,
    CROSSTALK_EDA_UI_MEMVIEW_ACTION_FREEZE
} crosstalk_eda_ui_memview_action_t;

typedef struct crosstalk_eda_ui_memview_where_term
{
    char path[CROSSTALK_EDA_UI_MEMVIEW_WHERE_PATH_CAP];
    char val_text[CROSSTALK_EDA_UI_MEMVIEW_WHERE_PATH_CAP];
    uint64_t val;
    uint32_t op;
    uint8_t pudding[4];
} crosstalk_eda_ui_memview_where_term_t;

typedef struct crosstalk_eda_ui_memview_ctx
{
    crosstalk_ui_window_state_t win;
    const crosstalk_hdl_elf_ctx_t *prog_elf;
    const crosstalk_hdl_elf_ctx_t *elf;
    const crosstalk_hdl_type_ctx_t *types;
    const crosstalk_eda_ui_calc_ctx_t *calc;
    uint64_t page[CROSSTALK_EDA_UI_MEMVIEW_PAGE_CAP];
    uint64_t mut_page[CROSSTALK_EDA_UI_MEMVIEW_PAGE_CAP];
    uint64_t mut_addr;
    uint64_t mut_base;
    uint32_t mut_pane_col;
    uint32_t mut_page_cunt;
    uint8_t mut_supported;
    uint8_t mut_visible;
    uint8_t mut_pudding[2];
    uint8_t page_prev[CROSSTALK_EDA_UI_MEMVIEW_PAGE_CAP];
    uint8_t churn[CROSSTALK_EDA_UI_MEMVIEW_PAGE_CAP];
    uint32_t motion_run;
    uint32_t motion_run_pudding;
    uint64_t io_base[CROSSTALK_EDA_UI_MEMVIEW_IO_RANGE_CAP];
    uint64_t io_span[CROSSTALK_EDA_UI_MEMVIEW_IO_RANGE_CAP];
    char io_name[CROSSTALK_EDA_UI_MEMVIEW_IO_RANGE_CAP][CROSSTALK_EDA_UI_MEMVIEW_IO_NAME_CAP];
    uint32_t io_cunt;
    uint8_t io_on;
    uint8_t io_pudding[3];
    uint64_t find_lo;
    uint64_t find_hi;
    uint64_t ref_tgt;
    uint64_t ref_window;
    uint32_t find_mode;
    uint8_t ptrs_on;
    uint8_t ptrs_pudding[3];
    uint64_t heap_addr[CROSSTALK_EDA_UI_MEMVIEW_HEAP_CAP];
    uint64_t heap_rec[CROSSTALK_EDA_UI_MEMVIEW_HEAP_REC_MAX];
    uint32_t heap_type[CROSSTALK_EDA_UI_MEMVIEW_HEAP_CAP];
    uint32_t heap_cunt;
    double heap_next;
    uint32_t annot_pan;
    uint32_t annot_span;
    uint64_t churn_base;
    double churn_next;
    uint8_t churn_seeded;
    uint8_t churn_pudding[7];
    uint64_t hit[CROSSTALK_EDA_UI_MEMVIEW_HIT_CAP];
    uint64_t seen_addr[CROSSTALK_EDA_UI_MEMVIEW_HIT_CAP];
    uint32_t seen_gen[CROSSTALK_EDA_UI_MEMVIEW_HIT_CAP];
    uint32_t hit_gen[CROSSTALK_EDA_UI_MEMVIEW_HIT_CAP];
    char run_sig[CROSSTALK_EDA_UI_MEMVIEW_RUN_SIG_CAP];
    uint32_t seen_cunt;
    uint32_t run_gen;
    crosstalk_eda_ui_memview_where_term_t where_term[CROSSTALK_EDA_UI_MEMVIEW_WHERE_TERM_MAX];
    uint32_t where_cunt;
    uint8_t where_pending;
    uint8_t where_pudding[3];
    uint8_t bkt[CROSSTALK_EDA_UI_MEMVIEW_BKT_CUNT];
    uint8_t heat[CROSSTALK_EDA_UI_MEMVIEW_BKT_CUNT];
    uint64_t heat_prev;
    uint32_t seq_cunt;
    uint32_t jmp_cunt;
    uint32_t heat_cunt;
    char heat_sig[CROSSTALK_EDA_UI_MEMVIEW_SIG_CAP];
    uint8_t pattern[CROSSTALK_EDA_UI_MEMVIEW_PATTERN_CAP];
    char query[CROSSTALK_EDA_UI_MEMVIEW_QUERY_CAP];
    char hist[CROSSTALK_EDA_UI_MEMVIEW_HIST_MAX][CROSSTALK_EDA_UI_MEMVIEW_QUERY_CAP];
    char draft[CROSSTALK_EDA_UI_MEMVIEW_QUERY_CAP];
    uint32_t hist_cunt;
    uint32_t hist_sel;
    uint32_t query_len;
    char sig[CROSSTALK_EDA_UI_MEMVIEW_SIG_CAP];
    char follow_sig[CROSSTALK_EDA_UI_MEMVIEW_SIG_CAP];
    uint64_t base;
    uint64_t width;
    uint64_t extent_cap;
    uint64_t extent_at;
    uint32_t elem_bytes;
    uint32_t ptr_bytes;
    uint32_t motion_prev[CROSSTALK_EDA_UI_MEMVIEW_MOTION_WINDOW];
    uint64_t motion_at;
    uint64_t motion_lo;
    uint64_t motion_hi;
    int64_t motion_step;
    uint32_t motion_kind;
    uint32_t motion_phase;
    uint32_t motion_span;
    uint64_t find_at;
    double refresh_next;
    double overview_next;
    uint32_t page_cunt;
    uint32_t hit_cunt;
    uint32_t pattern_len;
    uint32_t follow;
    uint64_t watch_lo;
    uint64_t watch_hi;
    uint64_t watch_addr;
    uint64_t watch_at;
    uint64_t watch_prev;
    uint64_t watch_hit_addr;
    uint8_t watch_pattern[CROSSTALK_EDA_UI_MEMVIEW_PATTERN_CAP];
    char watch_query[CROSSTALK_EDA_UI_MEMVIEW_QUERY_CAP];
    uint32_t watch_pattern_len;
    uint32_t watch_mode;
    ldg_io_file_t *snap_fh;
    uint64_t snap_base;
    ldg_io_file_t *diff_fh;
    uint64_t diff_base;
    uint64_t diff_page[CROSSTALK_EDA_UI_MEMVIEW_PAGE_CAP];
    uint64_t lane[CROSSTALK_EDA_UI_MEMVIEW_PAGE_CAP];
    uint32_t diff_page_cunt;
    uint32_t diff_cunt;
    char repl_arg[CROSSTALK_EDA_UI_MEMVIEW_QUERY_CAP];
    uint32_t repl_action;
    uint64_t strings_at;
    uint64_t str_run_start;
    char str_buff[CROSSTALK_EDA_UI_MEMVIEW_STRING_CAP];
    uint32_t str_run_len;
    uint32_t strings_min;
    uint32_t hit_at;
    uint8_t stringing;
    char dis_sig[CROSSTALK_EDA_UI_MEMVIEW_SIG_CAP];
    uint64_t dis_at;
    uint32_t dis_cunt;
    uint8_t dis_req;
    uint8_t dis_pudding[2];
    uint8_t diff_mode;
    uint8_t watch_hlt;
    uint8_t watch_hit;
    uint8_t watch_hit_mut;
    uint8_t watch_seeded;
    uint8_t open;
    uint8_t bound;
    uint8_t finding;
    uint8_t overview_ok;
    uint8_t snap_mode;
    uint8_t go_has;
    uint8_t hlt_is;
    uint8_t tab;
    uint8_t edit;
    uint8_t focused;
    uint8_t hlt_mark;
    uint8_t hlt_mark_seen;
    uint8_t brk_on;
    uint8_t brk_pudding[1];
    uint64_t go_addr;
    uint64_t hlt_mark_addr;
    uint64_t hlt_mark_end;
    uint64_t brk_lo;
    uint64_t brk_hi;
    double hlt_mark_start;
    double hlt_now;
    char dis_row[CROSSTALK_EDA_UI_MEMVIEW_DIS_ROW_CAP][CROSSTALK_EDA_UI_MEMVIEW_DIS_ROW_TEXT_CAP];
    uint64_t dis_row_addr[CROSSTALK_EDA_UI_MEMVIEW_DIS_ROW_CAP];
    uint32_t dis_row_cunt;
    uint32_t dis_sel;
    uint32_t hit_rows;
    uint32_t res_kind;
} crosstalk_eda_ui_memview_ctx_t;

uint32_t crosstalk_eda_ui_memview_init(crosstalk_eda_ui_memview_ctx_t *ctx);
uint32_t crosstalk_eda_ui_memview_open_set(crosstalk_eda_ui_memview_ctx_t *ctx, uint8_t open);
uint8_t crosstalk_eda_ui_memview_open_is(const crosstalk_eda_ui_memview_ctx_t *ctx);
uint32_t crosstalk_eda_ui_memview_bind(crosstalk_eda_ui_memview_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, const char *sig_name);
uint32_t crosstalk_eda_ui_memview_addr_set(crosstalk_eda_ui_memview_ctx_t *ctx, uint64_t addr);
uint32_t crosstalk_eda_ui_memview_follow_set(crosstalk_eda_ui_memview_ctx_t *ctx, const char *sig_name);
uint32_t crosstalk_eda_ui_memview_heat_sig_set(crosstalk_eda_ui_memview_ctx_t *ctx, const char *sig_name);
uint32_t crosstalk_eda_ui_memview_find_start(crosstalk_eda_ui_memview_ctx_t *ctx, const char *text, uint8_t ascii_is, uint64_t lo, uint64_t hi);
uint32_t crosstalk_eda_ui_memview_refs_start(crosstalk_eda_ui_memview_ctx_t *ctx, uint64_t tgt, uint64_t window, uint64_t lo, uint64_t hi);
uint32_t crosstalk_eda_ui_memview_addr_resolve(const crosstalk_eda_ui_memview_ctx_t *ctx, const char *text, uint64_t *out);
uint32_t crosstalk_eda_ui_memview_where_start(crosstalk_eda_ui_memview_ctx_t *ctx, const char *expr);
uint32_t crosstalk_eda_ui_memview_snap_set(crosstalk_eda_ui_memview_ctx_t *ctx, ldg_io_file_t *fh, uint64_t base);
uint32_t crosstalk_eda_ui_memview_snap_clear(crosstalk_eda_ui_memview_ctx_t *ctx);
uint32_t crosstalk_eda_ui_memview_diff_set(crosstalk_eda_ui_memview_ctx_t *ctx, ldg_io_file_t *fh, uint64_t base);
uint32_t crosstalk_eda_ui_memview_diff_clear(crosstalk_eda_ui_memview_ctx_t *ctx);
uint32_t crosstalk_eda_ui_memview_action_get(crosstalk_eda_ui_memview_ctx_t *ctx, uint32_t *out_action, char *out_arg, uint32_t arg_cap);
uint32_t crosstalk_eda_ui_memview_watch_start(crosstalk_eda_ui_memview_ctx_t *ctx, const char *text, uint8_t ascii_is, uint64_t lo, uint64_t hi, uint32_t mode);
uint32_t crosstalk_eda_ui_memview_watch_hit_get(crosstalk_eda_ui_memview_ctx_t *ctx, uint64_t *out_addr, uint8_t *out_mut_is, uint8_t *out_hlt_is);
uint32_t crosstalk_eda_ui_memview_hlt_mark_set(crosstalk_eda_ui_memview_ctx_t *ctx, uint64_t lo, uint64_t hi);
uint32_t crosstalk_eda_ui_memview_brk_set(crosstalk_eda_ui_memview_ctx_t *ctx, uint64_t lo, uint64_t hi, uint8_t on);
uint32_t crosstalk_eda_ui_memview_dis_loc_req(crosstalk_eda_ui_memview_ctx_t *ctx, uint32_t cunt);
uint32_t crosstalk_eda_ui_memview_tick(crosstalk_eda_ui_memview_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, double now);
uint32_t crosstalk_eda_ui_memview_win_reg(crosstalk_eda_ui_memview_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot);
uint32_t crosstalk_eda_ui_memview_hit_jmp(crosstalk_eda_ui_memview_ctx_t *ctx, uint32_t hit_idx);
uint32_t crosstalk_eda_ui_memview_syms_bind(crosstalk_eda_ui_memview_ctx_t *ctx, const crosstalk_hdl_elf_ctx_t *prog_elf, const crosstalk_hdl_elf_ctx_t *elf, const crosstalk_hdl_type_ctx_t *types);
uint32_t crosstalk_eda_ui_memview_calc_bind(crosstalk_eda_ui_memview_ctx_t *ctx, const crosstalk_eda_ui_calc_ctx_t *calc);
uint32_t crosstalk_eda_ui_memview_focus_set(crosstalk_eda_ui_memview_ctx_t *ctx, uint8_t focused);
uint32_t crosstalk_eda_ui_memview_motion_start(crosstalk_eda_ui_memview_ctx_t *ctx, uint32_t kind, int64_t step, uint64_t lo, uint64_t hi, uint32_t run);
uint32_t crosstalk_eda_ui_memview_motion_stop(crosstalk_eda_ui_memview_ctx_t *ctx);

#endif
