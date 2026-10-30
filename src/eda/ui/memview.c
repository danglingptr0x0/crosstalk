#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include <dangling/logging/logging.h>

#include <dangling/arch/dangcsa/dangcsa.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/core/bits.h>
#include <dangling/str/str.h>

#include "core/log.h"
#include "eda/ui/memview.h"
#include "ui/cell.h"
#include "ui/theme.h"

#define MEMVIEW_ORIG_COL 0
#define MEMVIEW_ROW_TAB 0
#define MEMVIEW_ROW_HDR 1
#define MEMVIEW_ROW_OVERVIEW 2
#define MEMVIEW_ROW_IO 3
#define MEMVIEW_ROW_RULER 4
#define MEMVIEW_ROW_GRID 5
#define MEMVIEW_ROW_REPL_GAP 1
#define MEMVIEW_MUT_PANE_CUNT 2
#define MEMVIEW_MUT_PANE_GAP 2
#define MEMVIEW_MUT_TITLE "most recent mem mutation: none"
#define MEMVIEW_MUT_ADDR_FMT "most recent mem mutation: 0x%016" PRIX64
#define MEMVIEW_MUT_IDLE "No mem mutations yet"
#define MEMVIEW_RULER_FMT "+%02X"
#define MEMVIEW_RULER_NEG_FMT "-%02X"
#define MEMVIEW_HOVER_FMT "at 0x%016" PRIX64
#define MEMVIEW_HOVER_TEXT_CAP 224
#define MEMVIEW_HOVER_SYM_FMT "; %.191s"
#define MEMVIEW_RULER_TEXT_CAP 8
#define MEMVIEW_SEP_GROUP 4
#define MEMVIEW_SEP_W_SAMPLES 1
#define MEMVIEW_SEP_DASH_SAMPLES 3
#define MEMVIEW_SEP_GAP_SAMPLES 3
#define MEMVIEW_SEP_CENTER_DIV 2
#define MEMVIEW_IO_SEG_GAP_SAMPLES 2
#define MEMVIEW_IO_BAND_DIV 2
#define MEMVIEW_IO_R 0.24
#define MEMVIEW_IO_G 0.42
#define MEMVIEW_IO_B 1.0
#define MEMVIEW_IO_DIM 0.35
#define MEMVIEW_IO_LIT 1.0
#define MEMVIEW_CHURN_CELL_INSET 1
#define MEMVIEW_CHURN_FULL 1.0
#define MEMVIEW_CHURN_HOT_COL 0xFF3B30FFU
#define MEMVIEW_CHURN_CHAN_MASK 0xFFU
#define MEMVIEW_CHURN_SHIFT_R 24
#define MEMVIEW_CHURN_SHIFT_G 16
#define MEMVIEW_CHURN_SHIFT_B 8
#define MEMVIEW_CELL_TEXT_CAP 8
#define MEMVIEW_CELL_FMT "%02" PRIX64
#define MEMVIEW_ANNOT_TEXT_CAP 512
#define MEMVIEW_ANNOT_FMT "+%02X %.47s; "
#define MEMVIEW_ANNOT_FIELD_FMT "+%02X %.95s; "
#define MEMVIEW_ANNOT_PTR_FMT "+%02X %.95s -> 0x%016" PRIX64 "; "
#define MEMVIEW_ANNOT_RAW_PTR_FMT "+%02X -> 0x%016" PRIX64 "; "
#define MEMVIEW_ANNOT_PTR_COL 0x5BD6E8FFU
#define MEMVIEW_ANNOT_PAN_STEP 6
#define MEMVIEW_ANNOT_PAN_NONE 0
#define MEMVIEW_ANNOT_PAN_SLACK 8
#define MEMVIEW_ANNOT_AVAIL_NONE 0
#define MEMVIEW_FIELD_FMT "%.63s.%.95s"
#define MEMVIEW_FIELD_TEXT_CAP 192
#define MEMVIEW_PTR_BYTES_DEFAULT sizeof(uint32_t)
#define MEMVIEW_ANNOT_GAP_CHARS 2
#define MEMVIEW_ANNOT_TINT_COL 0x1E2B1BFFU
#define MEMVIEW_STEP 1
#define MEMVIEW_SYM_PLUS '+'
#define MEMVIEW_SYM_MINUS '-'
#define MEMVIEW_FIND_BOUND_CUNT 2
#define MEMVIEW_ADDR_CHARS 16
#define MEMVIEW_BYTE_CHARS 3
#define MEMVIEW_HEX_CHARS 2
#define MEMVIEW_CAT_SPACE_COL 0xB0C8D8FFU
#define MEMVIEW_CAT_NULL_VAL 0x00
#define MEMVIEW_BKT_R 0.25
#define MEMVIEW_BKT_G 0.85
#define MEMVIEW_BKT_B 0.55
#define MEMVIEW_BKT_EMPTY 0.12
#define MEMVIEW_BKT_FULL 1.0
#define MEMVIEW_HEAT_R 1.0
#define MEMVIEW_HEAT_G 0.76
#define MEMVIEW_HEAT_B 0.29
#define MEMVIEW_CHAN_NONE 0.0
#define MEMVIEW_CHAN_FULL 1.0
#define MEMVIEW_CHAN_CUNT 3
#define MEMVIEW_HLT_PERIOD_MS 3000
#define MEMVIEW_HLT_BURST_MS 1200
#define MEMVIEW_HLT_BLINK_MS 400
#define MEMVIEW_HLT_ON_MS 200
#define MEMVIEW_HLT_START_NONE 0.0
#define MEMVIEW_MARK_SAMPLES 2
#define MEMVIEW_ROW_TEXT_CAP 256
#define MEMVIEW_ROW_FMT "%016" PRIX64
#define MEMVIEW_GO_BG_COL 0x5F7F00FFU
#define MEMVIEW_HDR_FMT "%.31s width %" PRIu64 " base 0x%" PRIX64 " %.63s %.23s"
#define MEMVIEW_TAB_GAP 1
#define MEMVIEW_TAB_CUNT 2
#define MEMVIEW_TAB_TEXT_CAP 32
#define MEMVIEW_TAB_HEX_TEXT "[hex]"
#define MEMVIEW_TAB_HITS_TEXT "[results]"
#define MEMVIEW_HITS_NONE_TEXT "no results; run find first"
#define MEMVIEW_HIT_CELL_W 21
#define MEMVIEW_HIT_GEN_CELLS 1
#define MEMVIEW_HIT_GEN_TEXT " "
#define MEMVIEW_HIT_FMT "0x%016" PRIX64
#define MEMVIEW_RUN_SIG_FIND_FMT "find:%u:%" PRIX64 ":%" PRIX64 ":%.63s"
#define MEMVIEW_RUN_SIG_REFS_FMT "refs:%" PRIX64 ":%" PRIX64 ":%" PRIX64 ":%" PRIX64
#define MEMVIEW_HIT_GEN_NONE 0
#define MEMVIEW_SEL_FIRST 0
#define MEMVIEW_IO_SEG_FIRST 0
#define MEMVIEW_HIT_GEN_COL1 0xADE53DFFU
#define MEMVIEW_HIT_GEN_COL2 0x5CD6E8FFU
#define MEMVIEW_HIT_GEN_COL3 0xFABF4AFFU
#define MEMVIEW_HIT_GEN_COL4 0xD973E5FFU
#define MEMVIEW_HIT_GEN_COL5 0xFF6B61FFU
#define MEMVIEW_FIND_RUN_TEXT "searching"
#define MEMVIEW_WATCH_RUN_TEXT "watching"
#define MEMVIEW_SNAP_IDLE_TEXT "snap"
#define MEMVIEW_SNAP_WORD_SIZE 8
#define MEMVIEW_SNAP_FIND_BUDGET 4096
#define MEMVIEW_REPL_MARK ">"
#define MEMVIEW_REPL_MARK_W 2
#define MEMVIEW_REPL_CURSOR_W 1
#define MEMVIEW_REPL_VERB_CAP 16
#define MEMVIEW_REPL_TOK_FIND "find"
#define MEMVIEW_REPL_TOK_GO "go"
#define MEMVIEW_REPL_TOK_WATCH "watch"
#define MEMVIEW_REPL_TOK_BREAK "break"
#define MEMVIEW_REPL_TOK_STEP "step"
#define MEMVIEW_REPL_TOK_HLT "hlt"
#define MEMVIEW_REPL_TOK_RUN "run"
#define MEMVIEW_REPL_TOK_POKE "poke"
#define MEMVIEW_REPL_TOK_FREEZE "freeze"
#define MEMVIEW_REPL_TOK_WHERE "where"
#define MEMVIEW_WHERE_AND "&&"
#define MEMVIEW_WHERE_VAL_BYTES_MAX 8
#define MEMVIEW_WHERE_SIG_FMT "where:%.119s"
#define MEMVIEW_IDX_TEXT_CAP 24
#define MEMVIEW_REPL_TOK_SNAP "snap"
#define MEMVIEW_REPL_TOK_LIVE "live"
#define MEMVIEW_REPL_TOK_DIFF "diff"
#define MEMVIEW_REPL_TOK_OFF "off"
#define MEMVIEW_REPL_TOK_HELP "help"
#define MEMVIEW_REPL_TOK_STRINGS "strings"
#define MEMVIEW_REPL_TOK_REFS "refs"
#define MEMVIEW_REFS_ARG_CUNT 4
#define MEMVIEW_REFS_ARG_WINDOW 1
#define MEMVIEW_REFS_ARG_LO 2
#define MEMVIEW_REFS_ARG_HI 3
#define MEMVIEW_REPL_TOK_PTRS "ptrs"
#define MEMVIEW_REPL_TOK_ON "on"
#define MEMVIEW_REPL_HELP_TEXT "where <field> <op> <val> [&& <field> <op> <val>] | freeze 0xADDR|<symbol> [len] | freeze off [0xADDR] | refs 0xADDR|<symbol> [0xWINDOW] [0xLO 0xHI] | ptrs [on|off] | find [0xLO 0xHI] <0xNN [0xNN ...]|\"text\"> |strings [min] | dis [0xADDR|loc|@sig] [n] [dangcsa|m16|m32] | go 0xADDR|<symbol>[+0xOFF] | watch [0xLO|<symbol> [0xHI|0xVAL]] [0xNN ...|\"text\"|+N|-N|+|-|*|~N|off] | break 0xLO|<symbol> [0xHI] | break off | step [n] | hlt | run | poke 0xADDR|<symbol> 0xNN [0xNN ...] | snap [name] | diff [name|off] | live"
#define MEMVIEW_STRINGS_MIN_DEFAULT 4
#define MEMVIEW_STRINGS_RUN_TEXT "strings"
#define MEMVIEW_STRINGS_CHUNK 256
#define MEMVIEW_REPL_TOK_DIS "dis"
#define MEMVIEW_DIS_CUNT_DEFAULT 16
#define MEMVIEW_DIS_CUNT_MAX 32
#define MEMVIEW_DIS_ASM_CAP 64
#define MEMVIEW_DIS_DANGCSA_ROW_FMT "0x%016" PRIX64 ": %016" PRIX64 "; %.47s"
#define MEMVIEW_DIS_COL_CUNT 2
#define MEMVIEW_DIS_ROW_PER_COL 16
#define MEMVIEW_DIS_COL_GAP 1
#define MEMVIEW_DIS_DANGCSA_FAIL_TEXT "bad word"
#define MEMVIEW_DIS_STEP_CUNT 1
#define MEMVIEW_DIFF_STAT_FMT "%u diff"
#define MEMVIEW_WATCH_QUERY_QUOTE "\""
#define MEMVIEW_WATCH_QUERY_BARE ""
#define MEMVIEW_WATCH_QUERY_MUT_FMT "watch 0x%" PRIX64
#define MEMVIEW_WATCH_QUERY_AT_FMT "watch 0x%" PRIX64 " %s%.24s%s"
#define MEMVIEW_WATCH_QUERY_SCAN_FMT "watch %s%.32s%s"
#define MEMVIEW_WATCH_QUERY_SPAN_FMT "watch 0x%" PRIX64 "-0x%" PRIX64 " %s%.16s%s"
#define MEMVIEW_FIND_DONE_FMT "%u hits"
#define MEMVIEW_FIND_IDLE_TEXT ""
#define MEMVIEW_STAT_CAP 24
#define MEMVIEW_SCROLL_ROWS 3
#define MEMVIEW_HIT_TEXT_CAP 896
#define MEMVIEW_HIT_LOG_FMT " 0x%" PRIX64
#define MEMVIEW_SCROLL_NONE 0.0

uint32_t crosstalk_eda_ui_memview_init(crosstalk_eda_ui_memview_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->ptr_bytes = (uint32_t)MEMVIEW_PTR_BYTES_DEFAULT;
    ctx->hist_sel = (uint32_t)UINT32_MAX;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_memview_open_set(crosstalk_eda_ui_memview_ctx_t *ctx, uint8_t open)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->open = open != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    ctx->win.open = ctx->open;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_ui_memview_open_is(const crosstalk_eda_ui_memview_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->win.open == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_eda_ui_memview_bind(crosstalk_eda_ui_memview_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, const char *sig_name)
{
    uint64_t width = 0;
    uint32_t elem_width = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || fpga == 0x0 || sig_name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_eda_ic_fpga_mem_width_get(fpga, slot_idx, sig_name, &width, &elem_width);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (width == 0) { return CROSSTALK_EDA_UI_MEMVIEW_ERR_BIND; }

    ret = ldg_str_rbrcpy(ctx->sig, sig_name, (uint64_t)sizeof(ctx->sig));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    if (crosstalk_eda_ui_memview_snap_clear(ctx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview snap clear failed"); }

    if (crosstalk_eda_ui_memview_diff_clear(ctx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview diff clear failed"); }

    ctx->elem_bytes = elem_width / (uint32_t)LDG_BYTE_BITS;
    if (ctx->elem_bytes == 0) { ctx->elem_bytes = (uint32_t)MEMVIEW_STEP; }

    ctx->ptr_bytes = fpga->slot[slot_idx].emu == LDG_TRUTH_TRUE ? (uint32_t)sizeof(uint64_t) : (uint32_t)MEMVIEW_PTR_BYTES_DEFAULT;

    ctx->extent_cap = width * (uint64_t)ctx->elem_bytes;
    ctx->extent_at = 0;

    ctx->width = ctx->extent_cap;

    ctx->io_cunt = 0;
    ctx->io_on = LDG_TRUTH_FALSE;

    {
        uint64_t io_base = 0;
        uint64_t io_span = 0;
        const char *io_name = 0x0;
        uint32_t at = 0;

        while (ctx->io_cunt < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_IO_RANGE_CAP && crosstalk_eda_ic_fpga_io_map_get(fpga, slot_idx, at, &io_base, &io_span, &io_name) == LDG_ERR_AOK)
        {
            ctx->io_base[ctx->io_cunt] = io_base;
            ctx->io_span[ctx->io_cunt] = io_span;

            if (io_name == 0x0 || ldg_str_rbrcpy(ctx->io_name[ctx->io_cunt], io_name, (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_IO_NAME_CAP) != LDG_ERR_AOK) { ctx->io_name[ctx->io_cunt][0] = LDG_STR_TERM; }

            ctx->io_cunt = ctx->io_cunt + (uint32_t)MEMVIEW_STEP;
            at = at + (uint32_t)MEMVIEW_STEP;
        }

        if (ctx->io_cunt != 0)
        {
            ctx->io_on = LDG_TRUTH_TRUE;

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview: io space mapped; ranges: %u32", ctx->io_cunt);
        }
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview: bound; sig: %.63s; elems: %u64; elem bytes: %u32; byte width: %u64", sig_name, width, ctx->elem_bytes, ctx->width);

    ctx->base = 0x0;
    ctx->bound = LDG_TRUTH_TRUE;
    ctx->overview_ok = LDG_TRUTH_FALSE;
    ctx->finding = LDG_TRUTH_FALSE;
    ctx->watch_mode = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WATCH_OFF;
    ctx->watch_hit = LDG_TRUTH_FALSE;
    ctx->hit_cunt = 0;
    ctx->page_cunt = 0;
    ctx->refresh_next = 0.0;

    ctx->mut_addr = UINT64_MAX;
    ctx->mut_base = 0x0;
    ctx->mut_page_cunt = 0x0;
    ctx->mut_supported = LDG_TRUTH_FALSE;
    ctx->mut_visible = LDG_TRUTH_FALSE;
    ctx->mut_pane_col = UINT32_MAX;

    if (fpga->slot[slot_idx].emu == LDG_TRUTH_TRUE)
    {
        uint64_t state[CROSSTALK_HDL_ABI_MEM_MUT_CUNT] = LDG_ARR_ZERO_INIT;

        if (crosstalk_eda_ic_fpga_emu_state_get(fpga, slot_idx, CROSSTALK_HDL_ABI_STATE_EMU_MEM_MUT, state, CROSSTALK_HDL_ABI_MEM_MUT_CUNT) == LDG_ERR_AOK) { ctx->mut_supported = LDG_TRUTH_TRUE; }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_memview_addr_set(crosstalk_eda_ui_memview_ctx_t *ctx, uint64_t addr)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->bound != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_UI_MEMVIEW_ERR_BIND; }

    if (ctx->io_on == LDG_TRUTH_TRUE && ctx->snap_mode != LDG_TRUTH_TRUE)
    {
        ctx->base = addr;
        ctx->follow = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_FOLLOW_FIXED;
        ctx->refresh_next = 0.0;

        return LDG_ERR_AOK;
    }

    if (addr >= ctx->extent_cap) { return CROSSTALK_EDA_UI_MEMVIEW_ERR_RANGE; }

    if (addr >= ctx->width)
    {
        ctx->width = addr + (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_PAGE_CAP < ctx->extent_cap ? addr + (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_PAGE_CAP : ctx->extent_cap;
        ctx->overview_ok = LDG_TRUTH_FALSE;
    }

    ctx->base = addr;
    ctx->follow = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_FOLLOW_FIXED;
    ctx->refresh_next = 0.0;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_memview_follow_set(crosstalk_eda_ui_memview_ctx_t *ctx, const char *sig_name)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (sig_name == 0x0 || sig_name[0] == LDG_STR_TERM)
    {
        ctx->follow = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_FOLLOW_FIXED;
        ctx->follow_sig[0] = LDG_STR_TERM;

        return LDG_ERR_AOK;
    }

    ret = ldg_str_rbrcpy(ctx->follow_sig, sig_name, (uint64_t)sizeof(ctx->follow_sig));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    ctx->follow = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_FOLLOW_SIG;

    return LDG_ERR_AOK;
}

static uint32_t memview_pattern_parse(const char *text, uint8_t ascii_is, uint8_t *pattern, uint32_t *out_len)
{
    uint32_t len = 0;
    uint32_t at = 0;

    *out_len = 0;

    if (ascii_is == LDG_TRUTH_TRUE) { while (text[len] != LDG_STR_TERM && text[len] != LDG_ASCII_QUOTE && len < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_PATTERN_CAP)
        {
            pattern[len] = (uint8_t)text[len];
            len = len + (uint32_t)MEMVIEW_STEP;
        }
    }
    else
    {
        while (text[at] != LDG_STR_TERM && len < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_PATTERN_CAP)
        {
            uint8_t hi = 0;
            uint8_t lo = 0;

            if (text[at] == ' ') { at = at + (uint32_t)MEMVIEW_STEP; continue; }

            if (text[at] != CROSSTALK_EDA_UI_MEMVIEW_HEX_LEAD[0] || (text[at + LDG_STR_TERM_SIZE] != 'x' && text[at + LDG_STR_TERM_SIZE] != 'X')) { return CROSSTALK_EDA_UI_MEMVIEW_ERR_PATTERN; }

            at = at + LDG_STR_TERM_SIZE + LDG_STR_TERM_SIZE;

            if (ldg_char_hex_is(text[at]) != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_UI_MEMVIEW_ERR_PATTERN; }

            while (ldg_char_hex_is(text[at]) == LDG_TRUTH_TRUE && len < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_PATTERN_CAP)
            {
                if (ldg_hex_to_nipple(text[at], &hi) != LDG_ERR_AOK) { return CROSSTALK_EDA_UI_MEMVIEW_ERR_PATTERN; }

                at = at + (uint32_t)MEMVIEW_STEP;

                if (ldg_char_hex_is(text[at]) != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_UI_MEMVIEW_ERR_PATTERN; }

                if (ldg_hex_to_nipple(text[at], &lo) != LDG_ERR_AOK) { return CROSSTALK_EDA_UI_MEMVIEW_ERR_PATTERN; }

                at = at + (uint32_t)MEMVIEW_STEP;

                pattern[len] = (uint8_t)((uint32_t)hi * (uint32_t)LDG_BASE_HEX + (uint32_t)lo);
                len = len + (uint32_t)MEMVIEW_STEP;
            }
        }
    }

    if (len == 0) { return CROSSTALK_EDA_UI_MEMVIEW_ERR_PATTERN; }

    *out_len = len;

    return LDG_ERR_AOK;
}

static uint32_t memview_run_gen_begin(crosstalk_eda_ui_memview_ctx_t *ctx, const char *sig)
{
    if (LDG_UNLIKELY(ctx == 0x0 || sig == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (strncmp(sig, ctx->run_sig, (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_RUN_SIG_CAP) == 0 && ctx->seen_cunt != 0)
    {
        if (ctx->run_gen + (uint32_t)MEMVIEW_STEP < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_RUN_GEN_MAX) { ctx->run_gen = ctx->run_gen + (uint32_t)MEMVIEW_STEP; }

        return LDG_ERR_AOK;
    }

    if (ldg_str_rbrcpy(ctx->run_sig, sig, (uint64_t)sizeof(ctx->run_sig)) != LDG_ERR_AOK) { ctx->run_sig[0] = LDG_STR_TERM; }

    ctx->run_gen = 0;
    ctx->seen_cunt = 0;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_memview_find_start(crosstalk_eda_ui_memview_ctx_t *ctx, const char *text, uint8_t ascii_is, uint64_t lo, uint64_t hi)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->bound != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_UI_MEMVIEW_ERR_BIND; }

    ret = memview_pattern_parse(text, ascii_is, ctx->pattern, &ctx->pattern_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (hi <= lo || hi > ctx->width) { hi = ctx->width; }

    if (lo >= hi) { lo = 0x0; }

    {
        char sig[CROSSTALK_EDA_UI_MEMVIEW_RUN_SIG_CAP] = LDG_ARR_ZERO_INIT;

        snprintf(sig, sizeof(sig), MEMVIEW_RUN_SIG_FIND_FMT, ascii_is, lo, hi, text);

        if (memview_run_gen_begin(ctx, sig) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview run gen begin failed"); }
    }

    ctx->find_lo = lo;
    ctx->find_hi = hi;
    ctx->find_at = lo;
    ctx->find_mode = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_FIND_MODE_PATTERN;
    ctx->hit_cunt = 0;
    ctx->hit_at = 0;
    ctx->stringing = LDG_TRUTH_FALSE;
    ctx->finding = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_memview_refs_start(crosstalk_eda_ui_memview_ctx_t *ctx, uint64_t tgt, uint64_t window, uint64_t lo, uint64_t hi)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->bound != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_UI_MEMVIEW_ERR_BIND; }

    if (tgt == 0x0) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (hi <= lo || hi > ctx->width) { hi = ctx->width; }

    if (lo >= hi) { lo = 0x0; }

    {
        char sig[CROSSTALK_EDA_UI_MEMVIEW_RUN_SIG_CAP] = LDG_ARR_ZERO_INIT;

        snprintf(sig, sizeof(sig), MEMVIEW_RUN_SIG_REFS_FMT, tgt, window, lo, hi);

        if (memview_run_gen_begin(ctx, sig) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview run gen begin failed"); }
    }

    ctx->ref_tgt = tgt;
    ctx->ref_window = window;
    ctx->find_lo = lo;
    ctx->find_hi = hi;
    ctx->find_at = lo;
    ctx->find_mode = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_FIND_MODE_REF;
    ctx->hit_cunt = 0;
    ctx->hit_at = 0;
    ctx->stringing = LDG_TRUTH_FALSE;
    ctx->finding = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_memview_watch_start(crosstalk_eda_ui_memview_ctx_t *ctx, const char *text, uint8_t ascii_is, uint64_t lo, uint64_t hi, uint32_t mode)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->bound != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_UI_MEMVIEW_ERR_BIND; }

    if (mode == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WATCH_MUT)
    {
        if (lo >= ctx->width) { return CROSSTALK_EDA_UI_MEMVIEW_ERR_RANGE; }

        ctx->watch_addr = lo;
        ctx->watch_seeded = LDG_TRUTH_FALSE;
    }
    else if (mode == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WATCH_AT)
    {
        ret = memview_pattern_parse(text, ascii_is, ctx->watch_pattern, &ctx->watch_pattern_len);
        if (ret != LDG_ERR_AOK) { return ret; }

        if (lo + (uint64_t)ctx->watch_pattern_len > ctx->width) { return CROSSTALK_EDA_UI_MEMVIEW_ERR_RANGE; }

        ctx->watch_addr = lo;
    }
    else if (mode == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WATCH_SCAN)
    {
        ret = memview_pattern_parse(text, ascii_is, ctx->watch_pattern, &ctx->watch_pattern_len);
        if (ret != LDG_ERR_AOK) { return ret; }

        if (hi > ctx->width) { hi = ctx->width; }

        if (lo + (uint64_t)ctx->watch_pattern_len > hi) { return CROSSTALK_EDA_UI_MEMVIEW_ERR_RANGE; }

        ctx->watch_lo = lo;
        ctx->watch_hi = hi;
        ctx->watch_at = lo;
    }
    else { return LDG_ERR_FUNC_ARG_INVALID; }

    ctx->watch_hit = LDG_TRUTH_FALSE;
    ctx->watch_hit_mut = LDG_TRUTH_FALSE;
    ctx->watch_hlt = LDG_TRUTH_FALSE;
    ctx->watch_hit_addr = 0x0;
    ctx->watch_mode = mode;

    {
        const char *quote = ascii_is == LDG_TRUTH_TRUE ? MEMVIEW_WATCH_QUERY_QUOTE : MEMVIEW_WATCH_QUERY_BARE;
        int32_t wrote = 0;

        if (mode == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WATCH_MUT) { wrote = snprintf(ctx->watch_query, sizeof(ctx->watch_query), MEMVIEW_WATCH_QUERY_MUT_FMT, ctx->watch_addr); }
        else if (mode == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WATCH_AT) { wrote = snprintf(ctx->watch_query, sizeof(ctx->watch_query), MEMVIEW_WATCH_QUERY_AT_FMT, ctx->watch_addr, quote, text, quote); }
        else if (ctx->watch_lo == 0 && ctx->watch_hi >= ctx->width) { wrote = snprintf(ctx->watch_query, sizeof(ctx->watch_query), MEMVIEW_WATCH_QUERY_SCAN_FMT, quote, text, quote); }
        else { wrote = snprintf(ctx->watch_query, sizeof(ctx->watch_query), MEMVIEW_WATCH_QUERY_SPAN_FMT, ctx->watch_lo, ctx->watch_hi, quote, text, quote); }

        if (wrote < 0) { ctx->watch_query[0] = LDG_STR_TERM; }
    }

    return LDG_ERR_AOK;
}

static uint32_t memview_elem_rd(crosstalk_eda_ui_memview_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, uint64_t elem_at, uint64_t *out, uint32_t cunt, uint32_t *out_rd)
{
    uint64_t elem_width = 0;
    uint64_t want = 0;
    uint64_t got = 0;
    uint32_t ret = 0;

    *out_rd = 0;

    if (ctx->snap_mode != LDG_TRUTH_TRUE) { return crosstalk_eda_ic_fpga_mem_rd(fpga, slot_idx, ctx->sig, elem_at, out, cunt, out_rd); }

    elem_width = ctx->elem_bytes != 0 ? ctx->width / (uint64_t)ctx->elem_bytes : ctx->width;

    if (ctx->snap_fh == 0x0 || elem_at >= elem_width) { return CROSSTALK_EDA_UI_MEMVIEW_ERR_RANGE; }

    if ((uint64_t)cunt > elem_width - elem_at) { cunt = (uint32_t)(elem_width - elem_at); }

    if (cunt == 0) { return LDG_ERR_AOK; }

    want = (uint64_t)cunt * (uint64_t)MEMVIEW_SNAP_WORD_SIZE;

    ret = ldg_io_file_seek(ctx->snap_fh, ctx->snap_base + elem_at * (uint64_t)MEMVIEW_SNAP_WORD_SIZE, (uint32_t)LDG_IO_SEEK_START);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_io_file_rd(ctx->snap_fh, out, want, &got);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_rd = (uint32_t)(got / (uint64_t)MEMVIEW_SNAP_WORD_SIZE);

    return LDG_ERR_AOK;
}

static uint32_t memview_file_bytes_rd(crosstalk_eda_ui_memview_ctx_t *ctx, ldg_io_file_t *fh, uint64_t base, uint64_t addr, uint64_t *out, uint32_t cunt, uint32_t *out_rd)
{
    uint64_t elem_at = 0;
    uint64_t got = 0;
    uint64_t want = 0;
    uint32_t elem_bytes = 0;
    uint32_t lead = 0;
    uint32_t elem_got = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || fh == 0x0 || out == 0x0 || out_rd == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_rd = 0;
    if (addr >= ctx->width || cunt == 0) { return LDG_ERR_AOK; }

    elem_bytes = ctx->elem_bytes != 0 ? ctx->elem_bytes : (uint32_t)MEMVIEW_STEP;
    if (LDG_UNLIKELY(elem_bytes > sizeof(uint64_t))) { return LDG_ERR_UNSUPPORTED; }

    if ((uint64_t)cunt > ctx->width - addr) { cunt = (uint32_t)(ctx->width - addr); }

    elem_at = addr / (uint64_t)elem_bytes;
    lead = (uint32_t)(addr % (uint64_t)elem_bytes);
    want = ((uint64_t)lead + (uint64_t)cunt + (uint64_t)elem_bytes - MEMVIEW_STEP) / (uint64_t)elem_bytes;
    if (want > (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_PAGE_CAP) { want = (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_PAGE_CAP; }

    if (LDG_UNLIKELY(elem_at > (UINT64_MAX - base) / (uint64_t)MEMVIEW_SNAP_WORD_SIZE)) { return LDG_ERR_BOUNDS; }

    ret = ldg_io_file_seek(fh, base + elem_at * (uint64_t)MEMVIEW_SNAP_WORD_SIZE, (uint32_t)LDG_IO_SEEK_START);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_io_file_rd(fh, ctx->lane, want * (uint64_t)MEMVIEW_SNAP_WORD_SIZE, &got);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    elem_got = (uint32_t)(got / (uint64_t)MEMVIEW_SNAP_WORD_SIZE);

    for (at = 0; at < cunt; at = at + (uint32_t)MEMVIEW_STEP)
    {
        uint32_t src = 0;
        uint32_t shift = 0;

        src = (lead + at) / elem_bytes;
        shift = ((lead + at) % elem_bytes) * (uint32_t)LDG_BYTE_BITS;
        if (src >= elem_got) { break; }

        out[at] = (ctx->lane[src] >> shift) & (uint64_t)LDG_BYTE_MASK;
        *out_rd = at + (uint32_t)MEMVIEW_STEP;
    }

    return LDG_ERR_AOK;
}

static uint32_t memview_src_rd(crosstalk_eda_ui_memview_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, uint64_t addr, uint64_t *out, uint32_t cunt, uint32_t *out_rd)
{
    uint64_t elem_at = 0;
    uint32_t elem_bytes = 0;
    uint32_t lead = 0;
    uint32_t elem_want = 0;
    uint32_t elem_got = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0 || out_rd == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_rd = 0;

    if (ctx->snap_mode == LDG_TRUTH_TRUE) { return memview_file_bytes_rd(ctx, ctx->snap_fh, ctx->snap_base, addr, out, cunt, out_rd); }

    if (ctx->io_on == LDG_TRUTH_TRUE && (addr >= (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_IO_FIRST || ctx->elem_bytes == sizeof(uint64_t)))
    {
        uint64_t word = 0x0;
        uint64_t word_addr = UINT64_MAX;

        for (at = 0; at < cunt; at = at + (uint32_t)MEMVIEW_STEP)
        {
            uint64_t byte_addr = 0x0;
            uint64_t aligned_addr = 0x0;
            uint32_t got = 0x0;

            if ((uint64_t)at > UINT64_MAX - addr) { break; }

            byte_addr = addr + (uint64_t)at;
            aligned_addr = byte_addr & ~((uint64_t)sizeof(uint64_t) - (uint64_t)MEMVIEW_STEP);

            if (aligned_addr != word_addr)
            {
                if (byte_addr >= (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_IO_FIRST)
                {
                    ret = crosstalk_eda_ic_fpga_io_rd(fpga, slot_idx, aligned_addr, &word);
                    if (ret != LDG_ERR_AOK) { break; }
                }
                else
                {
                    ret = memview_elem_rd(ctx, fpga, slot_idx, aligned_addr / sizeof(uint64_t), &word, MEMVIEW_STEP, &got);
                    if (ret != LDG_ERR_AOK || got != MEMVIEW_STEP) { break; }
                }

                word_addr = aligned_addr;
            }

            out[at] = (word >> ((byte_addr % sizeof(uint64_t)) * LDG_BYTE_BITS)) & (uint64_t)LDG_BYTE_MASK;

            *out_rd = at + (uint32_t)MEMVIEW_STEP;
        }

        return LDG_ERR_AOK;
    }

    elem_bytes = ctx->elem_bytes != 0 ? ctx->elem_bytes : (uint32_t)MEMVIEW_STEP;

    if (elem_bytes == (uint32_t)MEMVIEW_STEP) { return memview_elem_rd(ctx, fpga, slot_idx, addr, out, cunt, out_rd); }

    elem_at = addr / (uint64_t)elem_bytes;
    lead = (uint32_t)(addr % (uint64_t)elem_bytes);
    elem_want = (lead + cunt + elem_bytes - (uint32_t)MEMVIEW_STEP) / elem_bytes;

    if (elem_want > (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_PAGE_CAP) { elem_want = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_PAGE_CAP; }

    ret = memview_elem_rd(ctx, fpga, slot_idx, elem_at, ctx->lane, elem_want, &elem_got);
    if (ret != LDG_ERR_AOK) { return ret; }

    for (at = 0; at < cunt; at = at + (uint32_t)MEMVIEW_STEP)
    {
        uint32_t src = (lead + at) / elem_bytes;
        uint32_t lane = (lead + at) % elem_bytes;

        if (src >= elem_got) { break; }

        out[at] = (ctx->lane[src] >> ((uint64_t)lane * (uint64_t)LDG_BYTE_BITS)) & (uint64_t)LDG_BYTE_MASK;

        *out_rd = at + (uint32_t)MEMVIEW_STEP;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_memview_snap_set(crosstalk_eda_ui_memview_ctx_t *ctx, ldg_io_file_t *fh, uint64_t base)
{
    if (LDG_UNLIKELY(ctx == 0x0 || fh == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->bound != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_UI_MEMVIEW_ERR_BIND; }

    if (ctx->snap_fh != 0x0) { if (ldg_io_file_close(ctx->snap_fh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview snap close failed"); } }

    ctx->snap_fh = fh;
    ctx->snap_base = base;
    ctx->snap_mode = LDG_TRUTH_TRUE;
    ctx->overview_ok = LDG_TRUTH_FALSE;
    ctx->finding = LDG_TRUTH_FALSE;
    ctx->watch_mode = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WATCH_OFF;
    ctx->watch_hit = LDG_TRUTH_FALSE;
    ctx->hit_cunt = 0;
    ctx->page_cunt = 0;
    ctx->refresh_next = 0.0;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_memview_snap_clear(crosstalk_eda_ui_memview_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->snap_fh != 0x0) { if (ldg_io_file_close(ctx->snap_fh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview snap close failed"); } }

    ctx->snap_fh = 0x0;
    ctx->snap_base = 0x0;
    ctx->snap_mode = LDG_TRUTH_FALSE;
    ctx->overview_ok = LDG_TRUTH_FALSE;
    ctx->refresh_next = 0.0;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_memview_diff_set(crosstalk_eda_ui_memview_ctx_t *ctx, ldg_io_file_t *fh, uint64_t base)
{
    if (LDG_UNLIKELY(ctx == 0x0 || fh == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->bound != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_UI_MEMVIEW_ERR_BIND; }

    if (ctx->diff_fh != 0x0) { if (ldg_io_file_close(ctx->diff_fh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview diff close failed"); } }

    ctx->diff_fh = fh;
    ctx->diff_base = base;
    ctx->diff_mode = LDG_TRUTH_TRUE;
    ctx->diff_page_cunt = 0;
    ctx->diff_cunt = 0;
    ctx->refresh_next = 0.0;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_memview_diff_clear(crosstalk_eda_ui_memview_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->diff_fh != 0x0) { if (ldg_io_file_close(ctx->diff_fh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview diff close failed"); } }

    ctx->diff_fh = 0x0;
    ctx->diff_base = 0x0;
    ctx->diff_mode = LDG_TRUTH_FALSE;
    ctx->diff_page_cunt = 0;
    ctx->diff_cunt = 0;
    ctx->refresh_next = 0.0;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_memview_action_get(crosstalk_eda_ui_memview_ctx_t *ctx, uint32_t *out_action, char *out_arg, uint32_t arg_cap)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_action == 0x0 || out_arg == 0x0 || arg_cap == 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_action = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ACTION_NONE;
    out_arg[0] = LDG_STR_TERM;

    if (ctx->repl_action == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ACTION_NONE) { return LDG_ERR_NOT_FOUND; }

    ret = ldg_str_rbrcpy(out_arg, ctx->repl_arg, (uint64_t)arg_cap);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    *out_action = ctx->repl_action;
    ctx->repl_action = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ACTION_NONE;
    ctx->repl_arg[0] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_memview_motion_stop(crosstalk_eda_ui_memview_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->motion_kind = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_MOTION_OFF;
    ctx->motion_phase = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_MOTION_PHASE_SAMPLE;
    ctx->motion_span = 0;
    ctx->motion_at = 0;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_memview_motion_start(crosstalk_eda_ui_memview_ctx_t *ctx, uint32_t kind, int64_t step, uint64_t lo, uint64_t hi, uint32_t run)
{
    uint64_t elem_bytes = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->bound != LDG_TRUTH_TRUE) { return CROSSTALK_EDA_UI_MEMVIEW_ERR_BIND; }

    if (kind == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_MOTION_OFF || kind > (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_MOTION_BOUND) { return LDG_ERR_FUNC_ARG_INVALID; }

    elem_bytes = ctx->elem_bytes != 0 ? (uint64_t)ctx->elem_bytes : (uint64_t)MEMVIEW_STEP;

    if (hi <= lo || hi > ctx->extent_cap) { hi = ctx->width; }

    if (hi <= lo) { return CROSSTALK_EDA_UI_MEMVIEW_ERR_RANGE; }

    ctx->motion_lo = lo / elem_bytes;
    ctx->motion_hi = hi / elem_bytes;
    ctx->motion_at = ctx->motion_lo;
    ctx->motion_kind = kind;
    ctx->motion_step = step;
    ctx->motion_run = run != 0 ? run : (uint32_t)MEMVIEW_STEP;
    ctx->motion_phase = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_MOTION_PHASE_SAMPLE;
    ctx->motion_span = 0;
    ctx->hit_cunt = 0;
    ctx->hit_at = 0;
    ctx->finding = LDG_TRUTH_FALSE;
    ctx->stringing = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

static uint8_t memview_motion_match_is(const crosstalk_eda_ui_memview_ctx_t *ctx, int64_t delta)
{
    if (ctx->motion_kind == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_MOTION_CHANGED) { return delta != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    if (ctx->motion_kind == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_MOTION_UP) { return delta > 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    if (ctx->motion_kind == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_MOTION_DOWN) { return delta < 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    if (ctx->motion_kind == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_MOTION_STEP) { return delta == ctx->motion_step ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    if (delta == 0) { return LDG_TRUTH_FALSE; }

    return (delta <= ctx->motion_step && delta >= 0 - ctx->motion_step) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t memview_motion_step(crosstalk_eda_ui_memview_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx)
{
    uint64_t elem_bytes = 0;
    uint32_t run_len = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || fpga == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->motion_kind == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_MOTION_OFF) { return LDG_ERR_AOK; }

    elem_bytes = ctx->elem_bytes != 0 ? (uint64_t)ctx->elem_bytes : (uint64_t)MEMVIEW_STEP;

    if (ctx->motion_phase == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_MOTION_PHASE_SAMPLE)
    {
        uint64_t remain = ctx->motion_hi - ctx->motion_at;

        ctx->motion_span = remain > (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_MOTION_WINDOW ? (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_MOTION_WINDOW : (uint32_t)remain;

        for (at = 0; at < ctx->motion_span; at = at + (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_PAGE_CAP)
        {
            uint32_t want = ctx->motion_span - at < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_PAGE_CAP ? ctx->motion_span - at : (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_PAGE_CAP;
            uint32_t got = 0;
            uint32_t idx = 0;

            if (crosstalk_eda_ic_fpga_mem_rd(fpga, slot_idx, ctx->sig, ctx->motion_at + (uint64_t)at, ctx->lane, want, &got) != LDG_ERR_AOK) { break; }

            for (idx = 0; idx < got; idx = idx + (uint32_t)MEMVIEW_STEP) { ctx->motion_prev[at + idx] = (uint32_t)ctx->lane[idx]; }
        }

        ctx->motion_phase = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_MOTION_PHASE_CMP;

        return LDG_ERR_AOK;
    }

    for (at = 0; at < ctx->motion_span; at = at + (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_PAGE_CAP)
    {
        uint32_t want = ctx->motion_span - at < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_PAGE_CAP ? ctx->motion_span - at : (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_PAGE_CAP;
        uint32_t got = 0;
        uint32_t idx = 0;

        if (crosstalk_eda_ic_fpga_mem_rd(fpga, slot_idx, ctx->sig, ctx->motion_at + (uint64_t)at, ctx->lane, want, &got) != LDG_ERR_AOK) { break; }

        for (idx = 0; idx < got && ctx->hit_cunt < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_HIT_CAP; idx = idx + (uint32_t)MEMVIEW_STEP)
        {
            int64_t delta = (int64_t)(int32_t)((uint32_t)ctx->lane[idx] - ctx->motion_prev[at + idx]);
            uint64_t hit_addr = 0;
            uint32_t seen = 0;
            uint8_t dup_is = LDG_TRUTH_FALSE;

            if (memview_motion_match_is(ctx, delta) != LDG_TRUTH_TRUE) { run_len = 0; continue; }

            run_len = run_len + (uint32_t)MEMVIEW_STEP;

            if (run_len < ctx->motion_run) { continue; }

            hit_addr = (ctx->motion_at + (uint64_t)(at + idx) - (uint64_t)(ctx->motion_run - (uint32_t)MEMVIEW_STEP)) * elem_bytes;

            for (seen = 0; seen < ctx->hit_cunt; seen = seen + (uint32_t)MEMVIEW_STEP) { if (ctx->hit[seen] == hit_addr) { dup_is = LDG_TRUTH_TRUE; break; } }

            if (dup_is == LDG_TRUTH_TRUE) { continue; }

            ctx->hit[ctx->hit_cunt] = hit_addr;
            ctx->hit_gen[ctx->hit_cunt] = 0;
            ctx->hit_cunt = ctx->hit_cunt + (uint32_t)MEMVIEW_STEP;
            ctx->res_kind = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_RES_HITS;
        }
    }

    if (ctx->motion_span != 0 && (uint64_t)ctx->motion_span >= ctx->motion_hi - ctx->motion_lo && ctx->hit_cunt < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_HIT_CAP) { return LDG_ERR_AOK; }

    ctx->motion_at = ctx->motion_at + (uint64_t)ctx->motion_span;
    ctx->motion_phase = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_MOTION_PHASE_SAMPLE;

    if (ctx->motion_at >= ctx->motion_hi || ctx->hit_cunt >= (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_HIT_CAP)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview: motion done; kind: %u32; step: %i64; hits: %u32", ctx->motion_kind, ctx->motion_step, ctx->hit_cunt);

        ctx->tab = (uint8_t)CROSSTALK_EDA_UI_MEMVIEW_TAB_HITS;

        return crosstalk_eda_ui_memview_motion_stop(ctx);
    }

    return LDG_ERR_AOK;
}

static uint32_t memview_find_log(const crosstalk_eda_ui_memview_ctx_t *ctx)
{
    char list[MEMVIEW_HIT_TEXT_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t pos = 0;
    uint32_t at = 0;

    for (at = 0; at < ctx->hit_cunt; at = at + (uint32_t)MEMVIEW_STEP)
    {
        int32_t wrote = snprintf(list + pos, sizeof(list) - pos, MEMVIEW_HIT_LOG_FMT, ctx->hit[at]);

        if (wrote < 0 || (uint32_t)wrote >= (uint32_t)sizeof(list) - pos) { break; }

        pos = pos + (uint32_t)wrote;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview: find done; sig: %.63s; bytes: %u32; scanned: %u64; hits: %u32;%.895s", ctx->sig, ctx->pattern_len, ctx->find_at, ctx->hit_cunt, list);

    return LDG_ERR_AOK;
}

static uint32_t memview_hit_gen_col_get(uint32_t gen)
{
    if (gen == (uint32_t)MEMVIEW_STEP) { return (uint32_t)MEMVIEW_HIT_GEN_COL1; }

    if (gen == (uint32_t)MEMVIEW_STEP + (uint32_t)MEMVIEW_STEP) { return (uint32_t)MEMVIEW_HIT_GEN_COL2; }

    if (gen == (uint32_t)MEMVIEW_SEP_GROUP - (uint32_t)MEMVIEW_STEP) { return (uint32_t)MEMVIEW_HIT_GEN_COL3; }

    if (gen == (uint32_t)MEMVIEW_SEP_GROUP) { return (uint32_t)MEMVIEW_HIT_GEN_COL4; }

    return (uint32_t)MEMVIEW_HIT_GEN_COL5;
}

static uint32_t memview_rgba_get(double red, double green, double blue)
{
    double chan[MEMVIEW_CHAN_CUNT] = { red, green, blue };
    uint32_t shift[MEMVIEW_CHAN_CUNT] = { CROSSTALK_UI_CELL_RGBA_RED_SHIFT, CROSSTALK_UI_CELL_RGBA_GREEN_SHIFT, CROSSTALK_UI_CELL_RGBA_BLUE_SHIFT };
    uint32_t rgba = (uint32_t)LDG_BYTE_MASK;
    uint32_t at = 0;

    for (at = 0; at < (uint32_t)MEMVIEW_CHAN_CUNT; at++)
    {
        double val = chan[at];

        if (val < MEMVIEW_CHAN_NONE) { val = MEMVIEW_CHAN_NONE; }

        if (val > MEMVIEW_CHAN_FULL) { val = MEMVIEW_CHAN_FULL; }

        rgba = rgba | ((uint32_t)(val * (double)LDG_BYTE_MASK) << shift[at]);
    }

    return rgba;
}

static uint32_t memview_hit_put(crosstalk_eda_ui_memview_ctx_t *ctx, uint64_t addr)
{
    uint32_t at = 0;
    uint32_t gen = ctx->run_gen;

    for (at = 0; at < ctx->seen_cunt; at = at + (uint32_t)MEMVIEW_STEP)
    {
        if (ctx->seen_addr[at] != addr) { continue; }

        gen = ctx->seen_gen[at];

        break;
    }

    if (at == ctx->seen_cunt && ctx->seen_cunt < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_HIT_CAP)
    {
        ctx->seen_addr[ctx->seen_cunt] = addr;
        ctx->seen_gen[ctx->seen_cunt] = gen;
        ctx->seen_cunt = ctx->seen_cunt + (uint32_t)MEMVIEW_STEP;
    }

    ctx->hit[ctx->hit_cunt] = addr;
    ctx->hit_gen[ctx->hit_cunt] = 0;
    ctx->hit_gen[ctx->hit_cunt] = gen;
    ctx->hit_cunt = ctx->hit_cunt + (uint32_t)MEMVIEW_STEP;
    ctx->res_kind = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_RES_HITS;

    return LDG_ERR_AOK;
}

static uint32_t memview_find_step(crosstalk_eda_ui_memview_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx)
{
    uint64_t chunk[CROSSTALK_EDA_UI_MEMVIEW_PATTERN_CAP];
    uint64_t done = 0;
    uint64_t budget = ctx->snap_mode == LDG_TRUTH_TRUE ? (uint64_t)MEMVIEW_SNAP_FIND_BUDGET : (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_FIND_BUDGET;
    uint32_t rd = 0;

    if (ctx->find_mode == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_FIND_MODE_REF)
    {
        while (done < budget)
        {
            uint64_t val = 0;
            uint32_t byte = 0;

            if (ctx->find_at >= ctx->find_hi || (uint64_t)ctx->ptr_bytes > ctx->find_hi - ctx->find_at) { ctx->finding = LDG_TRUTH_FALSE; break; }

            if (memview_src_rd(ctx, fpga, slot_idx, ctx->find_at, chunk, (uint32_t)ctx->ptr_bytes, &rd) != LDG_ERR_AOK || rd < (uint32_t)ctx->ptr_bytes) { ctx->finding = LDG_TRUTH_FALSE; break; }

            for (byte = 0; byte < (uint32_t)ctx->ptr_bytes; byte = byte + (uint32_t)MEMVIEW_STEP) { val = val | ((chunk[byte] & (uint64_t)LDG_BYTE_MASK) << (byte * (uint32_t)LDG_BYTE_BITS)); }

            if (val <= ctx->ref_tgt && ctx->ref_tgt - val <= ctx->ref_window && ctx->hit_cunt < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_HIT_CAP) { if (memview_hit_put(ctx, ctx->find_at) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview hit put failed"); } }

            ctx->find_at = ctx->find_at + (uint64_t)ctx->ptr_bytes;
            done = done + (uint64_t)ctx->ptr_bytes;

            if (ctx->hit_cunt >= (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_HIT_CAP) { ctx->finding = LDG_TRUTH_FALSE; break; }
        }

        return LDG_ERR_AOK;
    }

    while (done < budget)
    {
        uint32_t at = 0;
        uint8_t match = LDG_TRUTH_TRUE;

        if (ctx->find_at + (uint64_t)ctx->pattern_len > ctx->find_hi) { ctx->finding = LDG_TRUTH_FALSE; break; }

        if (memview_src_rd(ctx, fpga, slot_idx, ctx->find_at, chunk, ctx->pattern_len, &rd) != LDG_ERR_AOK || rd < ctx->pattern_len) { ctx->finding = LDG_TRUTH_FALSE; break; }

        for (at = 0; at < ctx->pattern_len; at = at + (uint32_t)MEMVIEW_STEP) { if ((uint8_t)(chunk[at] & (uint64_t)LDG_BYTE_MASK) != ctx->pattern[at]) { match = LDG_TRUTH_FALSE; break; } }

        if (match == LDG_TRUTH_TRUE && ctx->hit_cunt < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_HIT_CAP) { if (memview_hit_put(ctx, ctx->find_at) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview hit put failed"); } }

        ctx->find_at = ctx->find_at + (uint64_t)MEMVIEW_STEP;
        done = done + (uint64_t)MEMVIEW_STEP;

        if (ctx->hit_cunt >= (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_HIT_CAP) { ctx->finding = LDG_TRUTH_FALSE; break; }
    }

    return LDG_ERR_AOK;
}

static uint32_t memview_overview_build(crosstalk_eda_ui_memview_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx)
{
    uint64_t elem_bytes = 0;
    uint64_t elem_width = 0;
    uint64_t span = 0;
    uint64_t stride = 0;
    uint32_t bkt = 0;

    if (ctx->width == 0) { return LDG_ERR_AOK; }

    elem_bytes = ctx->elem_bytes != 0 ? (uint64_t)ctx->elem_bytes : (uint64_t)MEMVIEW_STEP;
    elem_width = ctx->width / elem_bytes;

    if (elem_width == 0) { return LDG_ERR_AOK; }

    span = elem_width / (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_BKT_CUNT;
    if (span == 0) { span = (uint64_t)MEMVIEW_STEP; }

    stride = span / (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_BKT_SAMPLE_MAX;
    if (stride < (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_SAMPLE_STRIDE) { stride = (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_SAMPLE_STRIDE; }

    for (bkt = 0; bkt < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_BKT_CUNT; bkt = bkt + (uint32_t)MEMVIEW_STEP)
    {
        uint64_t at = (uint64_t)bkt * span;
        uint64_t end = at + span;
        uint32_t seen = 0;
        uint32_t live = 0;

        while (at < end && at < elem_width)
        {
            uint64_t val = 0;
            uint32_t rd = 0;

            if (memview_elem_rd(ctx, fpga, slot_idx, at, &val, (uint32_t)MEMVIEW_STEP, &rd) != LDG_ERR_AOK || rd == 0) { break; }

            if (val != 0) { live = live + (uint32_t)MEMVIEW_STEP; }

            seen = seen + (uint32_t)MEMVIEW_STEP;
            at = at + stride;
        }

        ctx->bkt[bkt] = seen != 0 ? (uint8_t)((live * (uint32_t)LDG_BYTE_MASK) / seen) : 0x0;
    }

    ctx->overview_ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_memview_heat_sig_set(crosstalk_eda_ui_memview_ctx_t *ctx, const char *sig_name)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || sig_name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_str_rbrcpy(ctx->heat_sig, sig_name, (uint64_t)sizeof(ctx->heat_sig));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    memset(ctx->heat, 0x0, sizeof(ctx->heat));

    ctx->seq_cunt = 0;
    ctx->jmp_cunt = 0;
    ctx->heat_cunt = 0;

    return LDG_ERR_AOK;
}

static uint32_t memview_heat_sample(crosstalk_eda_ui_memview_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx)
{
    const char *sig = ctx->heat_sig[0] != LDG_STR_TERM ? ctx->heat_sig : CROSSTALK_EDA_IC_FPGA_PROG_SIG;
    uint64_t span = 0;
    uint64_t addr = 0;
    uint64_t delta = 0;
    uint32_t width = 0;
    uint32_t bkt = 0;

    if (ctx->width == 0) { return LDG_ERR_AOK; }

    if (crosstalk_eda_ic_fpga_sig_peek(fpga, slot_idx, sig, &addr, &width) != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    if (addr >= ctx->width) { return LDG_ERR_AOK; }

    span = ctx->width / (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_BKT_CUNT;
    if (span == 0) { span = (uint64_t)MEMVIEW_STEP; }

    bkt = (uint32_t)(addr / span);
    if (bkt >= (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_BKT_CUNT) { bkt = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_BKT_CUNT - (uint32_t)MEMVIEW_STEP; }

    ctx->heat[bkt] = ctx->heat[bkt] + (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_HEAT_HIT > (uint32_t)LDG_BYTE_MASK ? (uint8_t)LDG_BYTE_MASK : (uint8_t)(ctx->heat[bkt] + (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_HEAT_HIT);

    delta = addr > ctx->heat_prev ? addr - ctx->heat_prev : ctx->heat_prev - addr;

    if (delta <= (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_SEQ_SPAN) { ctx->seq_cunt = ctx->seq_cunt + (uint32_t)MEMVIEW_STEP; }
    else { ctx->jmp_cunt = ctx->jmp_cunt + (uint32_t)MEMVIEW_STEP; }

    ctx->heat_prev = addr;
    ctx->heat_cunt = ctx->heat_cunt + (uint32_t)MEMVIEW_STEP;

    if (ctx->heat_cunt % (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_HEAT_DECAY_EVERY == 0)
    {
        uint32_t at = 0;

        for (at = 0; at < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_BKT_CUNT; at = at + (uint32_t)MEMVIEW_STEP) { if (ctx->heat[at] != 0) { ctx->heat[at] = ctx->heat[at] - (uint8_t)MEMVIEW_STEP; } }
    }

    return LDG_ERR_AOK;
}

static uint8_t memview_watch_match_is(crosstalk_eda_ui_memview_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, uint64_t addr)
{
    uint64_t chunk[CROSSTALK_EDA_UI_MEMVIEW_PATTERN_CAP];
    uint32_t rd = 0;
    uint32_t at = 0;

    if (memview_src_rd(ctx, fpga, slot_idx, addr, chunk, ctx->watch_pattern_len, &rd) != LDG_ERR_AOK || rd < ctx->watch_pattern_len) { return LDG_TRUTH_FALSE; }

    for (at = 0; at < ctx->watch_pattern_len; at = at + (uint32_t)MEMVIEW_STEP) { if ((uint8_t)(chunk[at] & (uint64_t)LDG_BYTE_MASK) != ctx->watch_pattern[at]) { return LDG_TRUTH_FALSE; } }

    return LDG_TRUTH_TRUE;
}

static uint32_t memview_watch_hit_mark(crosstalk_eda_ui_memview_ctx_t *ctx, uint64_t addr, uint8_t mut_is)
{
    uint64_t row = addr - addr % (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_COL_CUNT;

    ctx->watch_hit = LDG_TRUTH_TRUE;
    ctx->watch_hit_mut = mut_is;
    ctx->watch_hit_addr = addr;

    if (mut_is == LDG_TRUTH_TRUE) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "memview: watch mutation; sig: %.63s; addr: 0x%X64", ctx->sig, addr); }
    else
    {
        ctx->watch_mode = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WATCH_OFF;
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview: watch hit; sig: %.63s; addr: 0x%X64", ctx->sig, addr);
    }

    return crosstalk_eda_ui_memview_addr_set(ctx, row);
}

static uint32_t memview_watch_step(crosstalk_eda_ui_memview_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx)
{
    uint64_t done = 0;
    uint64_t budget = ctx->snap_mode == LDG_TRUTH_TRUE ? (uint64_t)MEMVIEW_SNAP_FIND_BUDGET : (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_FIND_BUDGET;

    if (ctx->watch_mode == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WATCH_MUT)
    {
        uint64_t val = 0;
        uint32_t rd = 0;

        if (memview_src_rd(ctx, fpga, slot_idx, ctx->watch_addr, &val, (uint32_t)MEMVIEW_STEP, &rd) != LDG_ERR_AOK || rd == 0) { return LDG_ERR_AOK; }

        if (ctx->watch_seeded != LDG_TRUTH_TRUE)
        {
            ctx->watch_prev = val;
            ctx->watch_seeded = LDG_TRUTH_TRUE;

            return LDG_ERR_AOK;
        }

        if (val == ctx->watch_prev) { return LDG_ERR_AOK; }

        ctx->watch_prev = val;

        return memview_watch_hit_mark(ctx, ctx->watch_addr, LDG_TRUTH_TRUE);
    }

    if (ctx->watch_mode == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WATCH_AT)
    {
        if (memview_watch_match_is(ctx, fpga, slot_idx, ctx->watch_addr) == LDG_TRUTH_TRUE) { return memview_watch_hit_mark(ctx, ctx->watch_addr, LDG_TRUTH_FALSE); }

        return LDG_ERR_AOK;
    }

    while (done < budget)
    {
        if (ctx->watch_at + (uint64_t)ctx->watch_pattern_len > ctx->watch_hi) { ctx->watch_at = ctx->watch_lo; break; }

        if (memview_watch_match_is(ctx, fpga, slot_idx, ctx->watch_at) == LDG_TRUTH_TRUE) { return memview_watch_hit_mark(ctx, ctx->watch_at, LDG_TRUTH_FALSE); }

        ctx->watch_at = ctx->watch_at + (uint64_t)MEMVIEW_STEP;
        done = done + (uint64_t)MEMVIEW_STEP;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_memview_watch_hit_get(crosstalk_eda_ui_memview_ctx_t *ctx, uint64_t *out_addr, uint8_t *out_mut_is, uint8_t *out_hlt_is)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_addr == 0x0 || out_mut_is == 0x0 || out_hlt_is == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_addr = 0x0;
    *out_mut_is = LDG_TRUTH_FALSE;
    *out_hlt_is = LDG_TRUTH_FALSE;

    if (ctx->watch_hit != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_FOUND; }

    *out_addr = ctx->watch_hit_addr;
    *out_mut_is = ctx->watch_hit_mut;
    *out_hlt_is = ctx->watch_hlt;
    ctx->watch_hit = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_memview_hlt_mark_set(crosstalk_eda_ui_memview_ctx_t *ctx, uint64_t lo, uint64_t hi)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (hi < lo) { hi = lo; }

    ctx->hlt_mark = LDG_TRUTH_TRUE;
    ctx->hlt_mark_seen = LDG_TRUTH_FALSE;
    ctx->hlt_mark_addr = lo - lo % (uint64_t)LDG_WORD_BYTES;
    ctx->hlt_mark_end = hi - hi % (uint64_t)LDG_WORD_BYTES + (uint64_t)LDG_WORD_BYTES;
    ctx->hlt_mark_start = MEMVIEW_HLT_START_NONE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview: hlt mark; lo: 0x%X64; end: 0x%X64; base: 0x%X64; page: %u32", ctx->hlt_mark_addr, ctx->hlt_mark_end, ctx->base, ctx->page_cunt);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_memview_brk_set(crosstalk_eda_ui_memview_ctx_t *ctx, uint64_t lo, uint64_t hi, uint8_t on)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->brk_lo = lo;
    ctx->brk_hi = hi;
    ctx->brk_on = on == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (ctx->brk_on != LDG_TRUTH_TRUE) { ctx->hlt_mark = LDG_TRUTH_FALSE; }

    return LDG_ERR_AOK;
}

static uint8_t memview_hlt_blink_is(const crosstalk_eda_ui_memview_ctx_t *ctx)
{
    uint64_t ms = 0;
    uint64_t phase = 0;

    if (ctx->hlt_mark != LDG_TRUTH_TRUE || ctx->hlt_mark_start == MEMVIEW_HLT_START_NONE) { return LDG_TRUTH_FALSE; }

    ms = (uint64_t)((ctx->hlt_now - ctx->hlt_mark_start) * (double)LDG_MS_PER_SEC);
    phase = ms % (uint64_t)MEMVIEW_HLT_PERIOD_MS;

    if (phase >= (uint64_t)MEMVIEW_HLT_BURST_MS) { return LDG_TRUTH_FALSE; }

    return phase % (uint64_t)MEMVIEW_HLT_BLINK_MS < (uint64_t)MEMVIEW_HLT_ON_MS ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_eda_ui_memview_dis_loc_req(crosstalk_eda_ui_memview_ctx_t *ctx, uint32_t cunt)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (cunt == 0 || cunt > (uint32_t)MEMVIEW_DIS_CUNT_MAX) { cunt = (uint32_t)MEMVIEW_DIS_STEP_CUNT; }

    ctx->dis_at = (uint64_t)UINT64_MAX;
    ctx->dis_cunt = cunt;
    ctx->dis_req = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t memview_hit_nav(crosstalk_eda_ui_memview_ctx_t *ctx, uint64_t addr)
{
    return crosstalk_eda_ui_memview_addr_set(ctx, addr - addr % (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_COL_CUNT);
}

static uint32_t memview_str_flush(crosstalk_eda_ui_memview_ctx_t *ctx)
{
    uint32_t text_len = 0;

    if (ctx->str_run_len >= ctx->strings_min && ctx->hit_cunt < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_HIT_CAP)
    {
        text_len = ctx->str_run_len < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_STRING_CAP - LDG_STR_TERM_SIZE ? ctx->str_run_len : (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_STRING_CAP - LDG_STR_TERM_SIZE;
        ctx->str_buff[text_len] = LDG_STR_TERM;

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview: string; addr: 0x%X64; len: %u32; text: %.47s", ctx->str_run_start, ctx->str_run_len, ctx->str_buff);

        ctx->hit[ctx->hit_cunt] = ctx->str_run_start;
        ctx->hit_gen[ctx->hit_cunt] = 0;
        ctx->hit_cunt = ctx->hit_cunt + (uint32_t)MEMVIEW_STEP;
    }

    ctx->str_run_len = 0;

    return LDG_ERR_AOK;
}

static uint32_t memview_strings_done(crosstalk_eda_ui_memview_ctx_t *ctx)
{
    if (memview_str_flush(ctx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview string flush failed"); }

    ctx->stringing = LDG_TRUTH_FALSE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview: strings done; scanned: %u64; hits: %u32", ctx->strings_at, ctx->hit_cunt);

    if (ctx->hit_cunt != 0)
    {
        ctx->hit_at = 0;

        if (memview_hit_nav(ctx, ctx->hit[0]) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview hit nav failed"); }
    }

    return LDG_ERR_AOK;
}

static uint32_t memview_strings_step(crosstalk_eda_ui_memview_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx)
{
    uint64_t chunk[MEMVIEW_STRINGS_CHUNK];
    uint64_t budget = ctx->snap_mode == LDG_TRUTH_TRUE ? (uint64_t)MEMVIEW_SNAP_FIND_BUDGET : (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_FIND_BUDGET;
    uint64_t done = 0;

    while (done < budget)
    {
        uint32_t want = (uint32_t)MEMVIEW_STRINGS_CHUNK;
        uint32_t rd = 0;
        uint32_t at = 0;

        if (ctx->strings_at >= ctx->width || ctx->hit_cunt >= (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_HIT_CAP) { return memview_strings_done(ctx); }

        if ((uint64_t)want > ctx->width - ctx->strings_at) { want = (uint32_t)(ctx->width - ctx->strings_at); }

        if (memview_src_rd(ctx, fpga, slot_idx, ctx->strings_at, chunk, want, &rd) != LDG_ERR_AOK || rd == 0) { return memview_strings_done(ctx); }

        for (at = 0; at < rd; at = at + (uint32_t)MEMVIEW_STEP)
        {
            uint8_t val = (uint8_t)(chunk[at] & (uint64_t)LDG_BYTE_MASK);

            if (val >= (uint8_t)LDG_ASCII_PRINTABLE_MIN && val <= (uint8_t)LDG_ASCII_PRINTABLE_MAX)
            {
                if (ctx->str_run_len == 0) { ctx->str_run_start = ctx->strings_at + (uint64_t)at; }

                if (ctx->str_run_len < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_STRING_CAP - LDG_STR_TERM_SIZE) { ctx->str_buff[ctx->str_run_len] = (char)val; }

                ctx->str_run_len = ctx->str_run_len + (uint32_t)MEMVIEW_STEP;
            }
            else if (ctx->str_run_len != 0) { if (memview_str_flush(ctx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview string flush failed"); } }
        }

        ctx->strings_at = ctx->strings_at + (uint64_t)rd;
        done = done + (uint64_t)rd;
    }

    return LDG_ERR_AOK;
}

static uint32_t memview_dis_step(crosstalk_eda_ui_memview_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx)
{
    uint64_t words[MEMVIEW_DIS_CUNT_MAX * LDG_DANGCSA_INSTR_SIZE] = LDG_ARR_ZERO_INIT;
    uint8_t bytes[MEMVIEW_DIS_CUNT_MAX * LDG_DANGCSA_INSTR_SIZE] = LDG_ARR_ZERO_INIT;
    char asm_text[MEMVIEW_DIS_ASM_CAP] = LDG_ARR_ZERO_INIT;
    ldg_dangcsa_decoded_t decoded = LDG_STRUCT_ZERO_INIT;
    uint64_t word = 0;
    uint64_t got = 0;
    uint32_t want = ctx->dis_cunt * (uint32_t)LDG_DANGCSA_INSTR_SIZE;
    uint32_t rd = 0;
    uint32_t at = 0;

    ctx->dis_req = LDG_TRUTH_FALSE;
    ctx->dis_row_cunt = 0;

    if (ctx->dis_at == (uint64_t)UINT64_MAX)
    {
        const char *pc_sig = ctx->dis_sig[0] != LDG_STR_TERM ? ctx->dis_sig : CROSSTALK_EDA_IC_FPGA_PROG_SIG;
        uint64_t pc_addr = 0;
        uint32_t pc_width = 0;

        if (crosstalk_eda_ic_fpga_sig_peek(fpga, slot_idx, pc_sig, &pc_addr, &pc_width) != LDG_ERR_AOK)
        {
            const char *fall_sig = ctx->heat_sig[0] != LDG_STR_TERM ? ctx->heat_sig : CROSSTALK_EDA_IC_FPGA_PROG_SIG;

            if (ctx->dis_sig[0] != LDG_STR_TERM || crosstalk_eda_ic_fpga_sig_peek(fpga, slot_idx, fall_sig, &pc_addr, &pc_width) != LDG_ERR_AOK)
            {
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview: dis sig unavailable; sig: %.63s", pc_sig);

                return LDG_ERR_AOK;
            }
        }

        ctx->dis_at = pc_addr;
    }

    if (ctx->dis_at >= ctx->width) { return CROSSTALK_EDA_UI_MEMVIEW_ERR_RANGE; }

    if ((uint64_t)want > ctx->width - ctx->dis_at) { want = (uint32_t)(ctx->width - ctx->dis_at); }

    if (memview_src_rd(ctx, fpga, slot_idx, ctx->dis_at, words, want, &rd) != LDG_ERR_AOK || rd == 0) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview: dis read failed; addr: 0x%X64", ctx->dis_at); return LDG_ERR_AOK; }

    for (at = 0; at < rd; at = at + (uint32_t)MEMVIEW_STEP) { bytes[at] = (uint8_t)(words[at] & (uint64_t)LDG_BYTE_MASK); }

    for (at = 0; at + (uint32_t)LDG_DANGCSA_INSTR_SIZE <= rd && got < (uint64_t)ctx->dis_cunt; at = at + (uint32_t)LDG_DANGCSA_INSTR_SIZE)
    {
        uint32_t byte = 0;

        word = 0;
        for (byte = 0; byte < LDG_DANGCSA_INSTR_SIZE; byte++) { word |= (uint64_t)bytes[at + byte] << (byte * LDG_BYTE_BITS); }

        if (ldg_dangcsa_decode(word, &decoded) != LDG_ERR_AOK || ldg_dangcsa_disasm(&decoded, asm_text, sizeof(asm_text)) != LDG_ERR_AOK) { if (ldg_str_rbrcpy(asm_text, MEMVIEW_DIS_DANGCSA_FAIL_TEXT, sizeof(asm_text)) != LDG_ERR_AOK) { return LDG_ERR_STR_TRUNC; } }

        if (ctx->dis_row_cunt < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_DIS_ROW_CAP && snprintf(ctx->dis_row[ctx->dis_row_cunt], sizeof(ctx->dis_row[ctx->dis_row_cunt]), MEMVIEW_DIS_DANGCSA_ROW_FMT, ctx->dis_at + (uint64_t)at, word, asm_text) > 0)
        {
            ctx->dis_row_addr[ctx->dis_row_cunt] = ctx->dis_at + (uint64_t)at;
            ctx->dis_row_cunt = ctx->dis_row_cunt + (uint32_t)MEMVIEW_STEP;
        }

        got = got + (uint64_t)MEMVIEW_STEP;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview: dis done; arch: dangcsa; addr: 0x%X64; cunt: %u64", ctx->dis_at, got);

    if (ctx->dis_row_cunt != 0)
    {
        ctx->res_kind = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_RES_DIS;
        ctx->tab = (uint8_t)CROSSTALK_EDA_UI_MEMVIEW_TAB_HITS;
    }

    return LDG_ERR_AOK;
}

static uint32_t memview_heap_add(crosstalk_eda_ui_memview_ctx_t *ctx, uint64_t addr, uint32_t type_idx)
{
    const crosstalk_hdl_type_rec_t *rec = 0x0;
    uint64_t size = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (crosstalk_hdl_type_get(ctx->types, type_idx, &rec) != LDG_ERR_AOK || rec->size == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    size = rec->size;

    for (at = 0; at < ctx->heap_cunt; at = at + (uint32_t)MEMVIEW_STEP)
    {
        const crosstalk_hdl_type_rec_t *held = 0x0;

        if (ctx->heap_addr[at] == addr) { return LDG_ERR_EXISTS; }

        if (crosstalk_hdl_type_get(ctx->types, ctx->heap_type[at], &held) != LDG_ERR_AOK) { continue; }

        if (addr + size <= ctx->heap_addr[at] || ctx->heap_addr[at] + held->size <= addr) { continue; }

        return LDG_ERR_EXISTS;
    }

    if (ctx->heap_cunt >= (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_HEAP_CAP) { return LDG_ERR_FULL; }

    ctx->heap_addr[ctx->heap_cunt] = addr;
    ctx->heap_type[ctx->heap_cunt] = type_idx;
    ctx->heap_cunt = ctx->heap_cunt + (uint32_t)MEMVIEW_STEP;

    return LDG_ERR_AOK;
}

static uint32_t memview_heap_find(const crosstalk_eda_ui_memview_ctx_t *ctx, uint64_t addr, uint32_t *out_type, uint64_t *out_off)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_type == 0x0 || out_off == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_type = (uint32_t)UINT32_MAX;
    *out_off = 0x0;

    if (ctx->types == 0x0) { return LDG_ERR_NOT_INIT; }

    for (at = 0; at < ctx->heap_cunt; at = at + (uint32_t)MEMVIEW_STEP)
    {
        const crosstalk_hdl_type_rec_t *rec = 0x0;

        if (addr < ctx->heap_addr[at]) { continue; }

        if (crosstalk_hdl_type_get(ctx->types, ctx->heap_type[at], &rec) != LDG_ERR_AOK) { continue; }

        if (rec->size == 0 || addr >= ctx->heap_addr[at] + rec->size) { continue; }

        *out_type = ctx->heap_type[at];
        *out_off = addr - ctx->heap_addr[at];

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t memview_heap_expand(crosstalk_eda_ui_memview_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, uint32_t slot)
{
    const crosstalk_hdl_type_rec_t *rec = 0x0;
    uint64_t addr = 0;
    uint64_t off = 0;
    uint32_t type_idx = 0;
    uint32_t got = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    addr = ctx->heap_addr[slot];
    type_idx = ctx->heap_type[slot];

    if (crosstalk_hdl_type_get(ctx->types, type_idx, &rec) != LDG_ERR_AOK) { return LDG_ERR_NOT_FOUND; }

    if (rec->size == 0 || rec->size > (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_HEAP_REC_MAX) { return LDG_ERR_BOUNDS; }

    if (memview_src_rd(ctx, fpga, slot_idx, addr, ctx->heap_rec, (uint32_t)rec->size, &got) != LDG_ERR_AOK) { return LDG_ERR_IO_RD; }

    if ((uint64_t)got < rec->size) { return LDG_ERR_IO_RD; }

    for (off = 0; off < rec->size; off = off + (uint64_t)MEMVIEW_STEP)
    {
        crosstalk_hdl_type_field_t field = LDG_STRUCT_ZERO_INIT;
        const crosstalk_hdl_type_rec_t *pointee = 0x0;
        uint64_t val = 0;
        uint32_t byte = 0;

        if (crosstalk_hdl_type_field_at(ctx->types, type_idx, off, &field) != LDG_ERR_AOK) { continue; }

        if (field.off != off || field.kind != (uint32_t)CROSSTALK_HDL_TYPE_KIND_PTR) { continue; }

        if (field.size == 0 || field.size > sizeof(uint64_t) || field.size > rec->size - off) { continue; }

        if (field.ref == (uint32_t)UINT32_MAX) { continue; }

        if (crosstalk_hdl_type_get(ctx->types, field.ref, &pointee) != LDG_ERR_AOK) { continue; }

        if (pointee->size == 0) { continue; }

        for (byte = 0; byte < (uint32_t)field.size; byte = byte + (uint32_t)MEMVIEW_STEP) { val = val | ((ctx->heap_rec[off + (uint64_t)byte] & (uint64_t)LDG_BYTE_MASK) << (byte * (uint32_t)LDG_BYTE_BITS)); }

        if (val < (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_PTR_FLOOR) { continue; }

        if (ctx->width != 0 && (val >= ctx->width || pointee->size > ctx->width - val)) { continue; }

        if (memview_heap_add(ctx, val, field.ref) == LDG_ERR_FULL) { return LDG_ERR_FULL; }
    }

    return LDG_ERR_AOK;
}

static uint32_t memview_heap_scan(crosstalk_eda_ui_memview_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx)
{
    uint64_t base = 0;
    uint32_t hd = 0;
    uint32_t depth = 0;
    uint32_t at = 0;
    uint32_t var_cunt = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->heap_cunt = 0;

    if (ctx->types == 0x0 || ctx->prog_elf == 0x0) { return LDG_ERR_NOT_INIT; }

    if (crosstalk_hdl_type_ld_is(ctx->types) != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    base = crosstalk_hdl_elf_base_get(ctx->prog_elf);

    if (base == UINT64_MAX) { return LDG_ERR_NOT_INIT; }

    var_cunt = crosstalk_hdl_type_var_cunt_get(ctx->types);

    if (var_cunt == UINT32_MAX) { return LDG_ERR_NOT_INIT; }

    for (at = 0; at < var_cunt; at = at + (uint32_t)MEMVIEW_STEP)
    {
        const crosstalk_hdl_type_var_t *var = 0x0;
        const crosstalk_hdl_type_rec_t *rec = 0x0;

        if (crosstalk_hdl_type_var_get(ctx->types, at, &var) != LDG_ERR_AOK) { continue; }

        if (var->type_idx == (uint32_t)UINT32_MAX || var->size == 0) { continue; }

        if (crosstalk_hdl_type_get(ctx->types, var->type_idx, &rec) != LDG_ERR_AOK) { continue; }

        if (rec->kind != (uint32_t)CROSSTALK_HDL_TYPE_KIND_STRUCT && rec->kind != (uint32_t)CROSSTALK_HDL_TYPE_KIND_UNION && rec->kind != (uint32_t)CROSSTALK_HDL_TYPE_KIND_ARR && rec->kind != (uint32_t)CROSSTALK_HDL_TYPE_KIND_PTR) { continue; }

        if (memview_heap_add(ctx, base + var->addr, var->type_idx) == LDG_ERR_FULL) { break; }
    }

    for (depth = 0; depth < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_HEAP_DEPTH; depth = depth + (uint32_t)MEMVIEW_STEP)
    {
        uint32_t end = ctx->heap_cunt;

        if (hd >= end) { break; }

        while (hd < end)
        {
            if (memview_heap_expand(ctx, fpga, slot_idx, hd) == LDG_ERR_FULL) { return LDG_ERR_AOK; }

            hd = hd + (uint32_t)MEMVIEW_STEP;
        }
    }

    return LDG_ERR_AOK;
}

static uint8_t memview_where_cmp_is(uint64_t lhs, uint32_t op, uint64_t rhs)
{
    if (op == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WHERE_OP_EQ) { return lhs == rhs ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    if (op == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WHERE_OP_NE) { return lhs != rhs ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    if (op == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WHERE_OP_LT) { return lhs < rhs ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    if (op == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WHERE_OP_GT) { return lhs > rhs ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    if (op == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WHERE_OP_LE) { return lhs <= rhs ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    return lhs >= rhs ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t memview_where_step(crosstalk_eda_ui_memview_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx)
{
    uint64_t buff[MEMVIEW_WHERE_VAL_BYTES_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t slot = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->where_pending = LDG_TRUTH_FALSE;
    ctx->hit_cunt = 0;
    ctx->hit_at = 0;
    ctx->res_kind = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_RES_HITS;

    if (ctx->types == 0x0 || crosstalk_hdl_type_ld_is(ctx->types) != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    for (slot = 0; slot < ctx->heap_cunt; slot = slot + (uint32_t)MEMVIEW_STEP)
    {
        uint32_t term_at = 0;
        uint8_t pass = LDG_TRUTH_TRUE;

        for (term_at = 0; term_at < ctx->where_cunt; term_at = term_at + (uint32_t)MEMVIEW_STEP)
        {
            crosstalk_hdl_type_field_t field = LDG_STRUCT_ZERO_INIT;
            uint64_t val = 0;
            uint32_t byte = 0;
            uint32_t rd = 0;

            if (crosstalk_hdl_type_field_find(ctx->types, ctx->heap_type[slot], ctx->where_term[term_at].path, &field) != LDG_ERR_AOK) { pass = LDG_TRUTH_FALSE; break; }

            if (field.size == 0 || field.size > (uint64_t)MEMVIEW_WHERE_VAL_BYTES_MAX) { pass = LDG_TRUTH_FALSE; break; }

            if (memview_src_rd(ctx, fpga, slot_idx, ctx->heap_addr[slot] + field.off, buff, (uint32_t)field.size, &rd) != LDG_ERR_AOK || (uint64_t)rd < field.size) { pass = LDG_TRUTH_FALSE; break; }

            for (byte = 0; byte < (uint32_t)field.size; byte = byte + (uint32_t)MEMVIEW_STEP) { val = val | ((buff[byte] & (uint64_t)LDG_BYTE_MASK) << (byte * (uint32_t)LDG_BYTE_BITS)); }

            if (memview_where_cmp_is(val, ctx->where_term[term_at].op, ctx->where_term[term_at].val) != LDG_TRUTH_TRUE) { pass = LDG_TRUTH_FALSE; break; }
        }

        if (pass != LDG_TRUTH_TRUE) { continue; }

        if (ctx->hit_cunt >= (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_HIT_CAP) { break; }

        if (memview_hit_put(ctx, ctx->heap_addr[slot]) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview where hit put failed"); }
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview: where done; terms: %u32; regions: %u32; hits: %u32", ctx->where_cunt, ctx->heap_cunt, ctx->hit_cunt);

    return LDG_ERR_AOK;
}

static uint32_t memview_page_ptr_get(const crosstalk_eda_ui_memview_ctx_t *ctx, uint64_t addr, uint32_t ptr_bytes, uint64_t *out)
{
    uint64_t at = 0;
    uint32_t byte = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(ptr_bytes == 0 || ptr_bytes > sizeof(uint64_t))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (addr < ctx->base) { return LDG_ERR_BOUNDS; }

    at = addr - ctx->base;

    if (at >= (uint64_t)ctx->page_cunt || (uint64_t)ptr_bytes > (uint64_t)ctx->page_cunt - at) { return LDG_ERR_BOUNDS; }

    for (byte = 0; byte < (uint32_t)ptr_bytes; byte = byte + (uint32_t)MEMVIEW_STEP) { *out = *out | ((ctx->page[at + (uint64_t)byte] & (uint64_t)LDG_BYTE_MASK) << (byte * (uint32_t)LDG_BYTE_BITS)); }

    return LDG_ERR_AOK;
}

static uint8_t memview_ptr_shape_is(const crosstalk_eda_ui_memview_ctx_t *ctx, uint64_t addr, uint64_t *out_tgt)
{
    uint64_t val = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_tgt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_tgt = 0x0;

    if (memview_page_ptr_get(ctx, addr, ctx->ptr_bytes, &val) != LDG_ERR_AOK) { return LDG_TRUTH_FALSE; }

    if (val < (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_PTR_FLOOR) { return LDG_TRUTH_FALSE; }

    if (ctx->width != 0 && val >= ctx->width) { return LDG_TRUTH_FALSE; }

    if ((val & ((uint64_t)ctx->ptr_bytes - (uint64_t)MEMVIEW_STEP)) != 0) { return LDG_TRUTH_FALSE; }

    *out_tgt = val;

    return LDG_TRUTH_TRUE;
}

static uint32_t memview_field_annot_get(const crosstalk_eda_ui_memview_ctx_t *ctx, uint64_t addr, char *out, uint32_t cap, uint32_t *out_kind, uint64_t *out_tgt, uint8_t exact_is)
{
    crosstalk_hdl_type_field_t field = LDG_STRUCT_ZERO_INIT;
    const crosstalk_hdl_type_var_t *var = 0x0;
    const char *owner = 0x0;
    uint64_t base = 0;
    uint64_t rel = 0;
    uint64_t off = 0;
    uint32_t type_idx = (uint32_t)UINT32_MAX;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0 || out_kind == 0x0 || out_tgt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    out[0] = LDG_STR_TERM;
    *out_kind = (uint32_t)CROSSTALK_HDL_TYPE_KIND_NONE;
    *out_tgt = 0x0;

    if (ctx->types == 0x0 || ctx->prog_elf == 0x0) { return LDG_ERR_NOT_INIT; }

    if (crosstalk_hdl_type_ld_is(ctx->types) != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    base = crosstalk_hdl_elf_base_get(ctx->prog_elf);

    if (base == UINT64_MAX || addr < base) { return LDG_ERR_NOT_FOUND; }

    rel = addr - base;

    if (crosstalk_hdl_type_var_at(ctx->types, rel, &var, &off) == LDG_ERR_AOK) { owner = var->name; type_idx = var->type_idx; }
    else if (memview_heap_find(ctx, addr, &type_idx, &off) == LDG_ERR_AOK)
    {
        const crosstalk_hdl_type_rec_t *rec = 0x0;

        if (crosstalk_hdl_type_get(ctx->types, type_idx, &rec) != LDG_ERR_AOK) { return LDG_ERR_NOT_FOUND; }

        if (crosstalk_hdl_type_label_get(rec, &owner) != LDG_ERR_AOK) { return LDG_ERR_NOT_FOUND; }
    }
    else { return LDG_ERR_NOT_FOUND; }

    if (crosstalk_hdl_type_field_at(ctx->types, type_idx, off, &field) != LDG_ERR_AOK) { return LDG_ERR_NOT_FOUND; }

    if (exact_is == LDG_TRUTH_TRUE && field.off != off) { return LDG_ERR_NOT_FOUND; }

    if (snprintf(out, cap, MEMVIEW_FIELD_FMT, owner, field.path) < 0) { return LDG_ERR_STR_FMT; }

    *out_kind = field.kind;

    if (field.kind == (uint32_t)CROSSTALK_HDL_TYPE_KIND_PTR) { if (field.size == 0 || field.size > sizeof(uint64_t) || memview_page_ptr_get(ctx, addr, (uint32_t)field.size, out_tgt) != LDG_ERR_AOK) { *out_tgt = 0x0; } }

    return LDG_ERR_AOK;
}

static uint32_t memview_annot_at_get(const crosstalk_eda_ui_memview_ctx_t *ctx, uint64_t addr, char *out, uint32_t cap)
{
    const crosstalk_hdl_elf_sym_t *sym = 0x0;
    uint64_t off = 0;
    uint32_t at = 0;

    out[0] = LDG_STR_TERM;

    if (ctx->io_on == LDG_TRUTH_TRUE && addr >= (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_IO_FIRST)
    {
        for (at = 0; at < ctx->io_cunt; at = at + (uint32_t)MEMVIEW_STEP)
        {
            if (addr != ctx->io_base[at] || ctx->io_name[at][0] == LDG_STR_TERM) { continue; }

            return ldg_str_rbrcpy(out, ctx->io_name[at], (uint64_t)cap);
        }

        return LDG_ERR_NOT_FOUND;
    }

    if (ctx->prog_elf != 0x0 && crosstalk_hdl_elf_sym_get(ctx->prog_elf, addr, &sym, &off) == LDG_ERR_AOK && sym != 0x0 && off == 0) { return ldg_str_rbrcpy(out, sym->name, (uint64_t)cap); }

    if (ctx->elf != 0x0 && crosstalk_hdl_elf_sym_get(ctx->elf, addr, &sym, &off) == LDG_ERR_AOK && sym != 0x0 && off == 0) { return ldg_str_rbrcpy(out, sym->name, (uint64_t)cap); }

    if (ctx->calc != 0x0) { return crosstalk_eda_ui_calc_var_at_get(ctx->calc, addr, out, cap); }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t memview_cat_col_get(uint64_t val)
{
    if (val == (uint64_t)MEMVIEW_CAT_NULL_VAL) { return (uint32_t)CROSSTALK_UI_THEME_DEAD_RGBA; }

    if (val == (uint64_t)LDG_ASCII_TAB || val == (uint64_t)LDG_ASCII_LF || val == (uint64_t)LDG_ASCII_CR || val == (uint64_t)LDG_ASCII_SPACE) { return (uint32_t)MEMVIEW_CAT_SPACE_COL; }

    if (val >= (uint64_t)LDG_ASCII_PRINTABLE_MIN && val <= (uint64_t)LDG_ASCII_PRINTABLE_MAX) { return (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA; }

    if (val < (uint64_t)LDG_ASCII_PRINTABLE_MIN || val == (uint64_t)LDG_ASCII_DELETE) { return (uint32_t)CROSSTALK_UI_THEME_STATUS_RGBA; }

    return (uint32_t)CROSSTALK_UI_THEME_PALE_RGBA;
}

static uint32_t memview_churn_col_get(uint8_t churn, uint32_t cold_col)
{
    double hot = 0.0;
    uint32_t chan_r = 0;
    uint32_t chan_g = 0;
    uint32_t chan_b = 0;

    if (churn == 0) { return cold_col; }

    hot = (double)churn / (double)CROSSTALK_EDA_UI_MEMVIEW_CHURN_MAX;
    if (hot > MEMVIEW_CHURN_FULL) { hot = MEMVIEW_CHURN_FULL; }

    chan_r = (uint32_t)((double)(((uint32_t)CROSSTALK_UI_THEME_LIVE_RGBA >> MEMVIEW_CHURN_SHIFT_R) & (uint32_t)MEMVIEW_CHURN_CHAN_MASK) + ((double)(((uint32_t)MEMVIEW_CHURN_HOT_COL >> MEMVIEW_CHURN_SHIFT_R) & (uint32_t)MEMVIEW_CHURN_CHAN_MASK) - (double)(((uint32_t)CROSSTALK_UI_THEME_LIVE_RGBA >> MEMVIEW_CHURN_SHIFT_R) & (uint32_t)MEMVIEW_CHURN_CHAN_MASK)) * hot);
    chan_g = (uint32_t)((double)(((uint32_t)CROSSTALK_UI_THEME_LIVE_RGBA >> MEMVIEW_CHURN_SHIFT_G) & (uint32_t)MEMVIEW_CHURN_CHAN_MASK) + ((double)(((uint32_t)MEMVIEW_CHURN_HOT_COL >> MEMVIEW_CHURN_SHIFT_G) & (uint32_t)MEMVIEW_CHURN_CHAN_MASK) - (double)(((uint32_t)CROSSTALK_UI_THEME_LIVE_RGBA >> MEMVIEW_CHURN_SHIFT_G) & (uint32_t)MEMVIEW_CHURN_CHAN_MASK)) * hot);
    chan_b = (uint32_t)((double)(((uint32_t)CROSSTALK_UI_THEME_LIVE_RGBA >> MEMVIEW_CHURN_SHIFT_B) & (uint32_t)MEMVIEW_CHURN_CHAN_MASK) + ((double)(((uint32_t)MEMVIEW_CHURN_HOT_COL >> MEMVIEW_CHURN_SHIFT_B) & (uint32_t)MEMVIEW_CHURN_CHAN_MASK) - (double)(((uint32_t)CROSSTALK_UI_THEME_LIVE_RGBA >> MEMVIEW_CHURN_SHIFT_B) & (uint32_t)MEMVIEW_CHURN_CHAN_MASK)) * hot);

    return (chan_r << MEMVIEW_CHURN_SHIFT_R) | (chan_g << MEMVIEW_CHURN_SHIFT_G) | (chan_b << MEMVIEW_CHURN_SHIFT_B) | (uint32_t)MEMVIEW_CHURN_CHAN_MASK;
}

static uint32_t memview_churn_step(crosstalk_eda_ui_memview_ctx_t *ctx, double now)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->base != ctx->churn_base || ctx->churn_seeded != LDG_TRUTH_TRUE)
    {
        memset(ctx->churn, 0x0, sizeof(ctx->churn));

        for (at = 0; at < ctx->page_cunt; at = at + (uint32_t)MEMVIEW_STEP) { ctx->page_prev[at] = (uint8_t)(ctx->page[at] & (uint64_t)LDG_BYTE_MASK); }

        ctx->churn_base = ctx->base;
        ctx->churn_next = now + CROSSTALK_EDA_UI_MEMVIEW_CHURN_WINDOW_SECS;
        ctx->churn_seeded = LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    if (now >= ctx->churn_next)
    {
        memset(ctx->churn, 0x0, sizeof(ctx->churn));

        ctx->churn_next = now + CROSSTALK_EDA_UI_MEMVIEW_CHURN_WINDOW_SECS;
    }

    for (at = 0; at < ctx->page_cunt; at = at + (uint32_t)MEMVIEW_STEP)
    {
        uint8_t val = (uint8_t)(ctx->page[at] & (uint64_t)LDG_BYTE_MASK);

        if (val != ctx->page_prev[at] && ctx->churn[at] < (uint8_t)CROSSTALK_EDA_UI_MEMVIEW_CHURN_MAX) { ctx->churn[at] = ctx->churn[at] + (uint8_t)MEMVIEW_STEP; }

        ctx->page_prev[at] = val;
    }

    return LDG_ERR_AOK;
}

static uint32_t memview_mut_step(crosstalk_eda_ui_memview_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx)
{
    uint64_t state[CROSSTALK_HDL_ABI_MEM_MUT_CUNT] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0x0;

    if (ctx->mut_visible != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ctx->mut_page_cunt = 0x0;
    ctx->mut_addr = UINT64_MAX;
    if (fpga->slot[slot_idx].emu != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = crosstalk_eda_ic_fpga_emu_state_get(fpga, slot_idx, CROSSTALK_HDL_ABI_STATE_EMU_MEM_MUT, state, CROSSTALK_HDL_ABI_MEM_MUT_CUNT);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->mut_addr = state[CROSSTALK_HDL_ABI_MEM_MUT_ADDR];
    if (ctx->mut_addr == UINT64_MAX) { return LDG_ERR_AOK; }

    ctx->mut_base = ctx->mut_addr - ctx->mut_addr % CROSSTALK_EDA_UI_MEMVIEW_COL_CUNT;
    return crosstalk_eda_ic_fpga_mem_byte_rd(fpga, slot_idx, ctx->sig, ctx->mut_base, ctx->mut_page, CROSSTALK_EDA_UI_MEMVIEW_PAGE_CAP, &ctx->mut_page_cunt);
}

uint32_t crosstalk_eda_ui_memview_tick(crosstalk_eda_ui_memview_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, double now)
{
    if (LDG_UNLIKELY(ctx == 0x0 || fpga == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->open != LDG_TRUTH_TRUE || ctx->bound != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (memview_mut_step(ctx, fpga, slot_idx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview latest RAM change read failed"); }

    ctx->hlt_is = crosstalk_eda_ic_fpga_slot_hlt_is(fpga, slot_idx);
    ctx->hlt_now = now;

    if (ctx->hlt_mark == LDG_TRUTH_TRUE && ctx->hlt_is == LDG_TRUTH_TRUE)
    {
        if (ctx->hlt_mark_start == MEMVIEW_HLT_START_NONE) { ctx->hlt_mark_start = now; }

        ctx->hlt_mark_seen = LDG_TRUTH_TRUE;
    }

    if (ctx->hlt_mark == LDG_TRUTH_TRUE && ctx->hlt_mark_seen == LDG_TRUTH_TRUE && ctx->hlt_is != LDG_TRUTH_TRUE)
    {
        ctx->hlt_mark = LDG_TRUTH_FALSE;
        ctx->hlt_mark_seen = LDG_TRUTH_FALSE;
    }

    if (ctx->snap_mode != LDG_TRUTH_TRUE) { memview_heat_sample(ctx, fpga, slot_idx); }

    if (ctx->finding == LDG_TRUTH_TRUE)
    {
        memview_find_step(ctx, fpga, slot_idx);

        if (ctx->finding != LDG_TRUTH_TRUE)
        {
            memview_find_log(ctx);

            if (ctx->hit_cunt != 0)
            {
                ctx->hit_at = 0;

                if (memview_hit_nav(ctx, ctx->hit[0]) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview hit nav failed"); }
            }
        }
    }

    if (ctx->motion_kind != (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_MOTION_OFF) { if (memview_motion_step(ctx, fpga, slot_idx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview motion step failed"); } }

    if (ctx->stringing == LDG_TRUTH_TRUE) { if (memview_strings_step(ctx, fpga, slot_idx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview strings step failed"); } }

    if (ctx->dis_req == LDG_TRUTH_TRUE) { if (memview_dis_step(ctx, fpga, slot_idx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview dis failed"); } }

    if (ctx->watch_mode != (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WATCH_OFF) { if (memview_watch_step(ctx, fpga, slot_idx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview watch step failed"); } }

    if (now < ctx->refresh_next) { return LDG_ERR_AOK; }

    ctx->refresh_next = now + CROSSTALK_EDA_UI_MEMVIEW_REFRESH_SECS;

    if (ctx->snap_mode != LDG_TRUTH_TRUE && ctx->follow == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_FOLLOW_SIG && ctx->follow_sig[0] != LDG_STR_TERM)
    {
        uint64_t val = 0;
        uint32_t width = 0;

        if (crosstalk_eda_ic_fpga_sig_peek(fpga, slot_idx, ctx->follow_sig, &val, &width) == LDG_ERR_AOK && (ctx->io_on == LDG_TRUTH_TRUE || val < ctx->width)) { ctx->base = val; }
    }

    if (ctx->overview_ok != LDG_TRUTH_TRUE || now >= ctx->overview_next)
    {
        memview_overview_build(ctx, fpga, slot_idx);

        ctx->overview_next = now + CROSSTALK_EDA_UI_MEMVIEW_OVERVIEW_SECS;
    }

    {
        uint32_t ret = memview_src_rd(ctx, fpga, slot_idx, ctx->base, ctx->page, (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_PAGE_CAP, &ctx->page_cunt);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (now >= ctx->heap_next)
    {
        if (memview_heap_scan(ctx, fpga, slot_idx) != LDG_ERR_AOK) { ctx->heap_cunt = 0; }

        ctx->heap_next = now + CROSSTALK_EDA_UI_MEMVIEW_HEAP_SECS;
    }

    if (ctx->where_pending == LDG_TRUTH_TRUE) { if (memview_where_step(ctx, fpga, slot_idx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview where step failed"); } }

    if (memview_churn_step(ctx, now) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview churn step failed"); }

    if (ctx->diff_mode == LDG_TRUTH_TRUE && ctx->diff_fh != 0x0)
    {
        uint32_t idx = 0;
        uint32_t span = 0;

        ctx->diff_page_cunt = 0;
        ctx->diff_cunt = 0;

        if (memview_file_bytes_rd(ctx, ctx->diff_fh, ctx->diff_base, ctx->base, ctx->diff_page, (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_PAGE_CAP, &ctx->diff_page_cunt) != LDG_ERR_AOK) { return LDG_ERR_IO_RD; }

        span = ctx->page_cunt < ctx->diff_page_cunt ? ctx->page_cunt : ctx->diff_page_cunt;

        for (idx = 0; idx < span; idx = idx + (uint32_t)MEMVIEW_STEP) { if (((ctx->page[idx] ^ ctx->diff_page[idx]) & (uint64_t)LDG_BYTE_MASK) != 0) { ctx->diff_cunt = ctx->diff_cunt + (uint32_t)MEMVIEW_STEP; } }
    }

    return LDG_ERR_AOK;
}

static uint32_t memview_where_parse(crosstalk_eda_ui_memview_ctx_t *ctx, const char *expr)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || expr == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->where_cunt = 0;

    while (expr[at] != LDG_STR_TERM && ctx->where_cunt < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WHERE_TERM_MAX)
    {
        crosstalk_eda_ui_memview_where_term_t *term = &ctx->where_term[ctx->where_cunt];
        char val_text[CROSSTALK_EDA_UI_MEMVIEW_WHERE_PATH_CAP] = LDG_ARR_ZERO_INIT;
        uint32_t pos = 0;

        while (expr[at] == ' ') { at = at + (uint32_t)MEMVIEW_STEP; }

        if (expr[at] == LDG_STR_TERM) { break; }

        memset(term, 0x0, sizeof(*term));

        while (expr[at] != LDG_STR_TERM && expr[at] != ' ' && expr[at] != '=' && expr[at] != '!' && expr[at] != '<' && expr[at] != '>' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(term->path))
        {
            term->path[pos] = expr[at];
            pos = pos + (uint32_t)MEMVIEW_STEP;
            at = at + (uint32_t)MEMVIEW_STEP;
        }

        term->path[pos] = LDG_STR_TERM;

        while (expr[at] == ' ') { at = at + (uint32_t)MEMVIEW_STEP; }

        if (expr[at] == '=' && expr[at + LDG_STR_TERM_SIZE] == '=') { term->op = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WHERE_OP_EQ; at = at + LDG_STR_TERM_SIZE + LDG_STR_TERM_SIZE; }
        else if (expr[at] == '!' && expr[at + LDG_STR_TERM_SIZE] == '=') { term->op = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WHERE_OP_NE; at = at + LDG_STR_TERM_SIZE + LDG_STR_TERM_SIZE; }
        else if (expr[at] == '<' && expr[at + LDG_STR_TERM_SIZE] == '=') { term->op = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WHERE_OP_LE; at = at + LDG_STR_TERM_SIZE + LDG_STR_TERM_SIZE; }
        else if (expr[at] == '>' && expr[at + LDG_STR_TERM_SIZE] == '=') { term->op = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WHERE_OP_GE; at = at + LDG_STR_TERM_SIZE + LDG_STR_TERM_SIZE; }
        else if (expr[at] == '<') { term->op = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WHERE_OP_LT; at = at + LDG_STR_TERM_SIZE; }
        else if (expr[at] == '>') { term->op = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WHERE_OP_GT; at = at + LDG_STR_TERM_SIZE; }
        else { return CROSSTALK_EDA_UI_MEMVIEW_ERR_PATTERN; }

        while (expr[at] == ' ') { at = at + (uint32_t)MEMVIEW_STEP; }

        pos = 0;

        while (expr[at] != LDG_STR_TERM && expr[at] != ' ' && expr[at] != '&' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(val_text))
        {
            val_text[pos] = expr[at];
            pos = pos + (uint32_t)MEMVIEW_STEP;
            at = at + (uint32_t)MEMVIEW_STEP;
        }

        val_text[pos] = LDG_STR_TERM;

        if (ldg_str_rbrcpy(term->val_text, val_text, (uint64_t)sizeof(term->val_text)) != LDG_ERR_AOK) { return CROSSTALK_EDA_UI_MEMVIEW_ERR_PATTERN; }

        ctx->where_cunt = ctx->where_cunt + (uint32_t)MEMVIEW_STEP;

        while (expr[at] == ' ') { at = at + (uint32_t)MEMVIEW_STEP; }

        if (expr[at] == '&' && expr[at + LDG_STR_TERM_SIZE] == '&') { at = at + LDG_STR_TERM_SIZE + LDG_STR_TERM_SIZE; continue; }

        break;
    }

    if (ctx->where_cunt == 0) { return CROSSTALK_EDA_UI_MEMVIEW_ERR_PATTERN; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_memview_syms_bind(crosstalk_eda_ui_memview_ctx_t *ctx, const crosstalk_hdl_elf_ctx_t *prog_elf, const crosstalk_hdl_elf_ctx_t *elf, const crosstalk_hdl_type_ctx_t *types)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->prog_elf = prog_elf;
    ctx->elf = elf;
    ctx->types = types;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_memview_focus_set(crosstalk_eda_ui_memview_ctx_t *ctx, uint8_t focused)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->focused = focused == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_memview_calc_bind(crosstalk_eda_ui_memview_ctx_t *ctx, const crosstalk_eda_ui_calc_ctx_t *calc)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->calc = calc;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_memview_hit_jmp(crosstalk_eda_ui_memview_ctx_t *ctx, uint32_t hit_idx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (hit_idx >= ctx->hit_cunt) { return CROSSTALK_EDA_UI_MEMVIEW_ERR_RANGE; }

    return crosstalk_eda_ui_memview_addr_set(ctx, ctx->hit[hit_idx]);
}

static uint32_t memview_step_rows(crosstalk_eda_ui_memview_ctx_t *ctx, uint32_t rows, uint8_t back_is)
{
    uint64_t delta = (uint64_t)rows * (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_COL_CUNT;
    uint64_t last = 0x0;

    if (rows == 0) { return LDG_ERR_AOK; }

    if (ctx->io_on == LDG_TRUTH_TRUE && ctx->snap_mode != LDG_TRUTH_TRUE) { last = UINT64_MAX; }
    else
    {
        if (ctx->width == 0x0) { return LDG_ERR_AOK; }

        last = ctx->width - MEMVIEW_STEP;
    }

    if (back_is == LDG_TRUTH_TRUE) { ctx->base = ctx->base > delta ? ctx->base - delta : 0x0; }
    else { ctx->base = ctx->base >= last || delta > last - ctx->base ? last : ctx->base + delta; }

    ctx->follow = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_FOLLOW_FIXED;
    ctx->refresh_next = MEMVIEW_SCROLL_NONE;

    return LDG_ERR_AOK;
}

static uint32_t memview_num_parse(const char *text, uint64_t *out)
{
    uint64_t parsed = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(text == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (text[0] == CROSSTALK_EDA_UI_MEMVIEW_HEX_LEAD[0] && text[LDG_STR_TERM_SIZE] == CROSSTALK_EDA_UI_MEMVIEW_HEX_LEAD[LDG_STR_TERM_SIZE]) { ret = ldg_str_uint_parse(&text[LDG_STR_TERM_SIZE + LDG_STR_TERM_SIZE], LDG_BASE_HEX, UINT64_MAX, &parsed); }
    else { ret = ldg_str_uint_parse(text, LDG_BASE_DEC, UINT64_MAX, &parsed); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = (uint64_t)parsed;
    return LDG_ERR_AOK;
}

static uint32_t memview_path_off_get(const crosstalk_eda_ui_memview_ctx_t *ctx, uint32_t type_idx, const char *text, uint64_t *out_off, uint32_t *out_type)
{
    uint64_t off = 0;
    uint32_t cur = type_idx;
    uint32_t at = 0;
    uint32_t depth = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0 || out_off == 0x0 || out_type == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_off = 0x0;
    *out_type = type_idx;

    if (ctx->types == 0x0) { return LDG_ERR_NOT_INIT; }

    while (text[at] != LDG_STR_TERM && depth < (uint32_t)CROSSTALK_HDL_TYPE_DEPTH_MAX)
    {
        const crosstalk_hdl_type_rec_t *rec = 0x0;

        if (crosstalk_hdl_type_get(ctx->types, cur, &rec) != LDG_ERR_AOK) { return LDG_ERR_NOT_FOUND; }

        if (rec->member_cunt == 0 && rec->kind == (uint32_t)CROSSTALK_HDL_TYPE_KIND_STRUCT && rec->ref != (uint32_t)UINT32_MAX)
        {
            cur = rec->ref;
            depth = depth + (uint32_t)MEMVIEW_STEP;

            continue;
        }

        if (text[at] == (char)CROSSTALK_HDL_TYPE_IDX_OPEN)
        {
            char digits[MEMVIEW_IDX_TEXT_CAP] = LDG_ARR_ZERO_INIT;
            const crosstalk_hdl_type_rec_t *elem = 0x0;
            dword_t slot = 0;
            uint32_t pos = 0;

            at = at + (uint32_t)MEMVIEW_STEP;

            while (ldg_char_digit_is(text[at]) == LDG_TRUTH_TRUE && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(digits))
            {
                digits[pos] = text[at];
                pos = pos + (uint32_t)MEMVIEW_STEP;
                at = at + (uint32_t)MEMVIEW_STEP;
            }

            digits[pos] = LDG_STR_TERM;

            if (pos == 0 || text[at] != (char)CROSSTALK_HDL_TYPE_IDX_CLOSE) { return CROSSTALK_EDA_UI_MEMVIEW_ERR_PATTERN; }

            if (ldg_str_to_dec(digits, &slot) != LDG_ERR_AOK) { return CROSSTALK_EDA_UI_MEMVIEW_ERR_PATTERN; }

            at = at + (uint32_t)MEMVIEW_STEP;

            if (rec->ref == (uint32_t)UINT32_MAX || crosstalk_hdl_type_get(ctx->types, rec->ref, &elem) != LDG_ERR_AOK || elem->size == 0) { return LDG_ERR_NOT_FOUND; }

            off = off + (uint64_t)slot * elem->size;
            cur = rec->ref;
            depth = depth + (uint32_t)MEMVIEW_STEP;

            continue;
        }

        if (text[at] == (char)CROSSTALK_HDL_TYPE_PATH_SEP)
        {
            char name[CROSSTALK_EDA_UI_MEMVIEW_WHERE_PATH_CAP] = LDG_ARR_ZERO_INIT;
            crosstalk_hdl_type_field_t field = LDG_STRUCT_ZERO_INIT;
            uint32_t pos = 0;

            at = at + (uint32_t)MEMVIEW_STEP;

            while (text[at] != LDG_STR_TERM && text[at] != (char)CROSSTALK_HDL_TYPE_PATH_SEP && text[at] != (char)CROSSTALK_HDL_TYPE_IDX_OPEN && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(name))
            {
                name[pos] = text[at];
                pos = pos + (uint32_t)MEMVIEW_STEP;
                at = at + (uint32_t)MEMVIEW_STEP;
            }

            name[pos] = LDG_STR_TERM;

            if (crosstalk_hdl_type_field_find(ctx->types, cur, name, &field) != LDG_ERR_AOK) { return LDG_ERR_NOT_FOUND; }

            off = off + field.off;
            cur = field.type_idx;
            depth = depth + (uint32_t)MEMVIEW_STEP;

            continue;
        }

        return CROSSTALK_EDA_UI_MEMVIEW_ERR_PATTERN;
    }

    *out_off = off;
    *out_type = cur;

    return LDG_ERR_AOK;
}

static uint32_t memview_sym_parse(const crosstalk_eda_ui_memview_ctx_t *ctx, const char *text, uint64_t *out)
{
    char name[CROSSTALK_HDL_ELF_NAME_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t addr = 0;
    uint64_t off = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    while (text[at] != LDG_STR_TERM && text[at] != MEMVIEW_SYM_PLUS && text[at] != ' ' && text[at] != (char)CROSSTALK_HDL_TYPE_PATH_SEP && text[at] != (char)CROSSTALK_HDL_TYPE_IDX_OPEN && at + LDG_STR_TERM_SIZE < (uint32_t)sizeof(name)) { name[at] = text[at]; at = at + (uint32_t)MEMVIEW_STEP; }

    name[at] = LDG_STR_TERM;

    if (ctx->prog_elf == 0x0 || crosstalk_hdl_elf_addr_get(ctx->prog_elf, name, &addr) != LDG_ERR_AOK) { if (ctx->elf == 0x0 || crosstalk_hdl_elf_addr_get(ctx->elf, name, &addr) != LDG_ERR_AOK) { if (ctx->calc == 0x0 || crosstalk_eda_ui_calc_var_get(ctx->calc, name, &addr) != LDG_ERR_AOK) { return LDG_ERR_NOT_FOUND; } } }

    if (text[at] == MEMVIEW_SYM_PLUS) { if (memview_num_parse(&text[at + LDG_STR_TERM_SIZE], &off) != LDG_ERR_AOK) { return LDG_ERR_FUNC_ARG_INVALID; } }
    else if (text[at] == (char)CROSSTALK_HDL_TYPE_PATH_SEP || text[at] == (char)CROSSTALK_HDL_TYPE_IDX_OPEN)
    {
        const crosstalk_hdl_type_var_t *var = 0x0;
        uint64_t elf_base = 0;
        uint64_t var_off = 0;
        uint32_t type_idx = (uint32_t)UINT32_MAX;

        if (ctx->types == 0x0 || ctx->prog_elf == 0x0) { return LDG_ERR_NOT_INIT; }

        elf_base = crosstalk_hdl_elf_base_get(ctx->prog_elf);

        if (elf_base == UINT64_MAX) { return LDG_ERR_NOT_INIT; }

        if (addr >= elf_base && crosstalk_hdl_type_var_at(ctx->types, addr - elf_base, &var, &var_off) == LDG_ERR_AOK) { type_idx = var->type_idx; }
        else if (memview_heap_find(ctx, addr, &type_idx, &var_off) != LDG_ERR_AOK) { return LDG_ERR_NOT_FOUND; }

        if (memview_path_off_get(ctx, type_idx, &text[at], &off, &type_idx) != LDG_ERR_AOK) { return LDG_ERR_NOT_FOUND; }
    }

    *out = addr + off;

    return LDG_ERR_AOK;
}

static uint32_t memview_addr_parse(const crosstalk_eda_ui_memview_ctx_t *ctx, const char *text, uint64_t *out)
{
    char base[CROSSTALK_HDL_ELF_NAME_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t addr = 0;
    uint64_t off = 0;
    uint32_t split = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    for (at = (uint32_t)MEMVIEW_STEP; text[at] != LDG_STR_TERM && text[at] != ' '; at = at + (uint32_t)MEMVIEW_STEP) { if (text[at] == MEMVIEW_SYM_PLUS || text[at] == MEMVIEW_SYM_MINUS) { split = at; break; } }

    if (split == 0)
    {
        if (memview_num_parse(text, out) == LDG_ERR_AOK) { return LDG_ERR_AOK; }

        return memview_sym_parse(ctx, text, out);
    }

    if (split + LDG_STR_TERM_SIZE >= (uint32_t)sizeof(base)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (at = 0; at < split; at = at + (uint32_t)MEMVIEW_STEP) { base[at] = text[at]; }

    base[split] = LDG_STR_TERM;

    if (memview_num_parse(base, &addr) != LDG_ERR_AOK) { if (memview_sym_parse(ctx, base, &addr) != LDG_ERR_AOK) { return LDG_ERR_NOT_FOUND; } }

    if (memview_num_parse(&text[split + LDG_STR_TERM_SIZE], &off) != LDG_ERR_AOK) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (text[split] == MEMVIEW_SYM_MINUS)
    {
        if (off > addr) { return CROSSTALK_EDA_UI_MEMVIEW_ERR_RANGE; }

        *out = addr - off;

        return LDG_ERR_AOK;
    }

    *out = addr + off;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_memview_addr_resolve(const crosstalk_eda_ui_memview_ctx_t *ctx, const char *text, uint64_t *out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return memview_addr_parse(ctx, text, out);
}

uint32_t crosstalk_eda_ui_memview_where_start(crosstalk_eda_ui_memview_ctx_t *ctx, const char *expr)
{
    char sig[CROSSTALK_EDA_UI_MEMVIEW_RUN_SIG_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || expr == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = memview_where_parse(ctx, expr);
    if (ret != LDG_ERR_AOK) { return ret; }

    for (at = 0; at < ctx->where_cunt; at = at + (uint32_t)MEMVIEW_STEP) { if (memview_addr_parse(ctx, ctx->where_term[at].val_text, &ctx->where_term[at].val) != LDG_ERR_AOK) { ctx->where_cunt = 0; return CROSSTALK_EDA_UI_MEMVIEW_ERR_PATTERN; } }

    snprintf(sig, sizeof(sig), MEMVIEW_WHERE_SIG_FMT, expr);

    if (memview_run_gen_begin(ctx, sig) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview run gen begin failed"); }

    ctx->where_pending = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t memview_repl_find(crosstalk_eda_ui_memview_ctx_t *ctx, const char *rest)
{
    char tok[MEMVIEW_REPL_VERB_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t bound[MEMVIEW_FIND_BOUND_CUNT] = LDG_ARR_ZERO_INIT;
    uint64_t lo = 0;
    uint64_t hi = 0;
    uint32_t bound_cunt = 0;
    uint32_t at = 0;
    uint32_t pos = 0;

    while (bound_cunt < (uint32_t)MEMVIEW_FIND_BOUND_CUNT)
    {
        uint32_t scan = at;

        while (rest[scan] == ' ') { scan = scan + (uint32_t)MEMVIEW_STEP; }

        if (rest[scan] == LDG_STR_TERM || rest[scan] == '"') { break; }

        pos = 0;

        while (rest[scan] != LDG_STR_TERM && rest[scan] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(tok)) { tok[pos] = rest[scan]; pos = pos + (uint32_t)MEMVIEW_STEP; scan = scan + (uint32_t)MEMVIEW_STEP; }

        tok[pos] = LDG_STR_TERM;

        if (memview_addr_parse(ctx, tok, &bound[bound_cunt]) != LDG_ERR_AOK) { break; }

        if (bound[bound_cunt] <= (uint64_t)LDG_BYTE_MASK) { break; }

        at = scan;
        bound_cunt = bound_cunt + (uint32_t)MEMVIEW_STEP;
    }

    if (bound_cunt == (uint32_t)MEMVIEW_FIND_BOUND_CUNT)
    {
        lo = bound[0];
        hi = bound[LDG_STR_TERM_SIZE];
    }
    else { at = 0; }

    while (rest[at] == ' ') { at = at + (uint32_t)MEMVIEW_STEP; }

    if (rest[at] == '"') { return crosstalk_eda_ui_memview_find_start(ctx, &rest[at + LDG_STR_TERM_SIZE], LDG_TRUTH_TRUE, lo, hi); }

    return crosstalk_eda_ui_memview_find_start(ctx, &rest[at], LDG_TRUTH_FALSE, lo, hi);
}

static uint32_t memview_repl_refs(crosstalk_eda_ui_memview_ctx_t *ctx, const char *rest)
{
    char tok[MEMVIEW_REPL_VERB_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t arg[MEMVIEW_REFS_ARG_CUNT] = LDG_ARR_ZERO_INIT;
    uint64_t window = (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_REF_WINDOW_DEFAULT;
    uint64_t lo = 0;
    uint64_t hi = 0;
    uint32_t arg_cunt = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || rest == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (arg_cunt < (uint32_t)MEMVIEW_REFS_ARG_CUNT && rest[at] != LDG_STR_TERM)
    {
        uint32_t pos = 0;

        while (rest[at] == ' ') { at = at + (uint32_t)MEMVIEW_STEP; }

        while (rest[at] != LDG_STR_TERM && rest[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(tok)) { tok[pos] = rest[at]; pos = pos + (uint32_t)MEMVIEW_STEP; at = at + (uint32_t)MEMVIEW_STEP; }

        tok[pos] = LDG_STR_TERM;

        if (tok[0] == LDG_STR_TERM) { break; }

        if (memview_addr_parse(ctx, tok, &arg[arg_cunt]) != LDG_ERR_AOK) { break; }

        arg_cunt = arg_cunt + (uint32_t)MEMVIEW_STEP;
    }

    if (arg_cunt == 0)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview: refs; need an address; arg: %.63s", rest);

        return LDG_ERR_AOK;
    }

    if (arg_cunt >= (uint32_t)MEMVIEW_REFS_ARG_WINDOW + (uint32_t)MEMVIEW_STEP) { window = arg[MEMVIEW_REFS_ARG_WINDOW]; }

    if (arg_cunt == (uint32_t)MEMVIEW_REFS_ARG_CUNT)
    {
        lo = arg[MEMVIEW_REFS_ARG_LO];
        hi = arg[MEMVIEW_REFS_ARG_HI];
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview: refs; tgt: 0x%X64; window: 0x%X64; lo: 0x%X64; hi: 0x%X64", arg[0], window, lo, hi);

    return crosstalk_eda_ui_memview_refs_start(ctx, arg[0], window, lo, hi);
}

static uint32_t memview_repl_action_put(crosstalk_eda_ui_memview_ctx_t *ctx, uint32_t action, const char *rest)
{
    uint32_t ret = 0;

    ret = ldg_str_rbrcpy(ctx->repl_arg, rest, (uint64_t)sizeof(ctx->repl_arg));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    ctx->repl_action = action;

    return LDG_ERR_AOK;
}

static uint32_t memview_repl_exec(crosstalk_eda_ui_memview_ctx_t *ctx)
{
    char verb[MEMVIEW_REPL_VERB_CAP] = LDG_ARR_ZERO_INIT;
    const char *rest = 0x0;
    uint32_t at = 0;
    uint32_t pos = 0;
    uint32_t ret = 0;

    while (ctx->query[at] == ' ') { at = at + (uint32_t)MEMVIEW_STEP; }

    while (ctx->query[at] != LDG_STR_TERM && ctx->query[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(verb)) { verb[pos] = ctx->query[at]; pos = pos + (uint32_t)MEMVIEW_STEP; at = at + (uint32_t)MEMVIEW_STEP; }

    verb[pos] = LDG_STR_TERM;

    while (ctx->query[at] == ' ') { at = at + (uint32_t)MEMVIEW_STEP; }

    rest = &ctx->query[at];

    if (verb[0] == LDG_STR_TERM) { return LDG_ERR_AOK; }

    if (strcmp(verb, MEMVIEW_REPL_TOK_HELP) == 0)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview: repl verbs; %s", MEMVIEW_REPL_HELP_TEXT);

        return LDG_ERR_AOK;
    }

    if (strcmp(verb, MEMVIEW_REPL_TOK_GO) == 0)
    {
        uint64_t addr = 0;

        if (memview_addr_parse(ctx, rest, &addr) != LDG_ERR_AOK)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview: go; not an address and not a known symbol; arg: %.63s", rest);

            return LDG_ERR_AOK;
        }

        if (crosstalk_eda_ui_memview_addr_set(ctx, addr) != LDG_ERR_AOK)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview go refused");

            return LDG_ERR_AOK;
        }

        ctx->go_addr = addr;
        ctx->go_has = LDG_TRUTH_TRUE;

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview: go; arg: %.63s; addr: 0x%X64", rest, addr);

        return LDG_ERR_AOK;
    }

    if (strcmp(verb, MEMVIEW_REPL_TOK_FIND) == 0) { return memview_repl_find(ctx, rest); }

    if (strcmp(verb, MEMVIEW_REPL_TOK_REFS) == 0) { return memview_repl_refs(ctx, rest); }

    if (strcmp(verb, MEMVIEW_REPL_TOK_WHERE) == 0)
    {
        if (crosstalk_eda_ui_memview_where_start(ctx, rest) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview: where; bad expression; arg: %.63s", rest); }

        return LDG_ERR_AOK;
    }

    if (strcmp(verb, MEMVIEW_REPL_TOK_PTRS) == 0)
    {
        if (strcmp(rest, MEMVIEW_REPL_TOK_OFF) == 0) { ctx->ptrs_on = LDG_TRUTH_FALSE; }
        else if (strcmp(rest, MEMVIEW_REPL_TOK_ON) == 0) { ctx->ptrs_on = LDG_TRUTH_TRUE; }
        else { ctx->ptrs_on = ctx->ptrs_on == LDG_TRUTH_TRUE ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE; }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview: ptrs; on: %u32", (uint32_t)ctx->ptrs_on);

        return LDG_ERR_AOK;
    }

    if (strcmp(verb, MEMVIEW_REPL_TOK_DIS) == 0)
    {
        char tok[MEMVIEW_REPL_VERB_CAP] = LDG_ARR_ZERO_INIT;
        uint64_t addr = (uint64_t)UINT64_MAX;
        uint64_t num = 0;
        uint64_t dis_cunt = (uint64_t)MEMVIEW_DIS_CUNT_DEFAULT;
        uint32_t tat = 0;
        uint32_t tpos = 0;

        while (rest[tat] != LDG_STR_TERM)
        {
            while (rest[tat] == ' ') { tat = tat + (uint32_t)MEMVIEW_STEP; }

            tpos = 0;

            while (rest[tat] != LDG_STR_TERM && rest[tat] != ' ' && tpos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(tok)) { tok[tpos] = rest[tat]; tpos = tpos + (uint32_t)MEMVIEW_STEP; tat = tat + (uint32_t)MEMVIEW_STEP; }

            tok[tpos] = LDG_STR_TERM;

            if (tok[0] == LDG_STR_TERM) { break; }

            if (strcmp(tok, CROSSTALK_EDA_IC_FPGA_PROG_SIG) == 0) { addr = (uint64_t)UINT64_MAX; }
            else if (tok[0] == '@')
            {
                ret = ldg_str_rbrcpy(ctx->dis_sig, &tok[LDG_STR_TERM_SIZE], (uint64_t)sizeof(ctx->dis_sig));
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { ctx->dis_sig[0] = LDG_STR_TERM; }

                addr = (uint64_t)UINT64_MAX;
            }
            else if (tok[0] == CROSSTALK_EDA_UI_MEMVIEW_HEX_LEAD[0] && tok[LDG_STR_TERM_SIZE] == CROSSTALK_EDA_UI_MEMVIEW_HEX_LEAD[LDG_STR_TERM_SIZE]) { if (memview_num_parse(tok, &addr) != LDG_ERR_AOK) { addr = (uint64_t)UINT64_MAX; } }
            else if (memview_num_parse(tok, &num) == LDG_ERR_AOK) { dis_cunt = num; }
            else if (memview_sym_parse(ctx, tok, &num) == LDG_ERR_AOK) { addr = num; }
        }

        if (dis_cunt == 0 || dis_cunt > (uint64_t)MEMVIEW_DIS_CUNT_MAX) { dis_cunt = (uint64_t)MEMVIEW_DIS_CUNT_DEFAULT; }

        if (addr != (uint64_t)UINT64_MAX && addr >= ctx->width) { addr = ctx->base; }

        ctx->dis_at = addr;
        ctx->dis_cunt = (uint32_t)dis_cunt;
        ctx->dis_req = LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    if (strcmp(verb, MEMVIEW_REPL_TOK_STRINGS) == 0)
    {
        uint64_t min_len = (uint64_t)MEMVIEW_STRINGS_MIN_DEFAULT;

        if (rest[0] != LDG_STR_TERM && memview_num_parse(rest, &min_len) != LDG_ERR_AOK) { min_len = (uint64_t)MEMVIEW_STRINGS_MIN_DEFAULT; }

        if (min_len == 0 || min_len >= (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_STRING_CAP) { min_len = (uint64_t)MEMVIEW_STRINGS_MIN_DEFAULT; }

        ctx->strings_min = (uint32_t)min_len;
        ctx->strings_at = 0x0;
        ctx->str_run_len = 0;
        ctx->hit_cunt = 0;
        ctx->hit_at = 0;
        ctx->finding = LDG_TRUTH_FALSE;
        ctx->stringing = LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    if (strcmp(verb, MEMVIEW_REPL_TOK_LIVE) == 0)
    {
        if (crosstalk_eda_ui_memview_snap_clear(ctx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview snap clear failed"); }

        return crosstalk_eda_ui_memview_diff_clear(ctx);
    }

    if (strcmp(verb, MEMVIEW_REPL_TOK_DIFF) == 0)
    {
        if (strcmp(rest, MEMVIEW_REPL_TOK_OFF) == 0) { return crosstalk_eda_ui_memview_diff_clear(ctx); }

        return memview_repl_action_put(ctx, (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ACTION_DIFF, rest);
    }

    if (strcmp(verb, MEMVIEW_REPL_TOK_WATCH) == 0 || strcmp(verb, MEMVIEW_REPL_TOK_BREAK) == 0)
    {
        if (strcmp(rest, MEMVIEW_REPL_TOK_OFF) == 0)
        {
            ctx->watch_mode = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WATCH_OFF;
            ctx->watch_hlt = LDG_TRUTH_FALSE;
            ctx->watch_hit = LDG_TRUTH_FALSE;
            ctx->watch_query[0] = LDG_STR_TERM;

            return LDG_ERR_AOK;
        }

        if (strcmp(verb, MEMVIEW_REPL_TOK_BREAK) == 0) { return memview_repl_action_put(ctx, (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ACTION_BREAK, rest); }

        return memview_repl_action_put(ctx, (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ACTION_WATCH, rest);
    }

    if (strcmp(verb, MEMVIEW_REPL_TOK_STEP) == 0) { return memview_repl_action_put(ctx, (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ACTION_STEP, rest); }

    if (strcmp(verb, MEMVIEW_REPL_TOK_HLT) == 0) { return memview_repl_action_put(ctx, (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ACTION_HLT, rest); }

    if (strcmp(verb, MEMVIEW_REPL_TOK_RUN) == 0) { return memview_repl_action_put(ctx, (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ACTION_RUN, rest); }

    if (strcmp(verb, MEMVIEW_REPL_TOK_POKE) == 0) { return memview_repl_action_put(ctx, (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ACTION_POKE, rest); }

    if (strcmp(verb, MEMVIEW_REPL_TOK_FREEZE) == 0) { return memview_repl_action_put(ctx, (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ACTION_FREEZE, rest); }

    if (strcmp(verb, MEMVIEW_REPL_TOK_SNAP) == 0) { return memview_repl_action_put(ctx, (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ACTION_SNAP, rest); }

    if (ctx->query[0] == CROSSTALK_EDA_UI_MEMVIEW_HEX_LEAD[0] && ctx->query[LDG_STR_TERM_SIZE] == CROSSTALK_EDA_UI_MEMVIEW_HEX_LEAD[LDG_STR_TERM_SIZE]) { return crosstalk_eda_ui_memview_find_start(ctx, ctx->query, LDG_TRUTH_FALSE, 0x0, ctx->width); }

    return crosstalk_eda_ui_memview_find_start(ctx, ctx->query, LDG_TRUTH_TRUE, 0x0, ctx->width);
}

static uint32_t memview_hist_push(crosstalk_eda_ui_memview_ctx_t *ctx, const char *text)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (text[0] == LDG_STR_TERM) { return LDG_ERR_AOK; }

    if (ctx->hist_cunt != 0 && strcmp(ctx->hist[ctx->hist_cunt - (uint32_t)MEMVIEW_STEP], text) == 0) { return LDG_ERR_AOK; }

    if (ctx->hist_cunt == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_HIST_MAX)
    {
        for (at = 0; at + (uint32_t)MEMVIEW_STEP < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_HIST_MAX; at = at + (uint32_t)MEMVIEW_STEP) { memcpy(ctx->hist[at], ctx->hist[at + (uint32_t)MEMVIEW_STEP], sizeof(ctx->hist[at])); }

        ctx->hist_cunt = ctx->hist_cunt - (uint32_t)MEMVIEW_STEP;
    }

    if (ldg_str_rbrcpy(ctx->hist[ctx->hist_cunt], text, (uint64_t)sizeof(ctx->hist[ctx->hist_cunt])) == LDG_ERR_STR_TRUNC) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview history line truncated"); }

    ctx->hist_cunt = ctx->hist_cunt + (uint32_t)MEMVIEW_STEP;

    return LDG_ERR_AOK;
}

static uint32_t memview_hist_recall(crosstalk_eda_ui_memview_ctx_t *ctx, uint32_t sel)
{
    const char *line = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    line = sel == (uint32_t)UINT32_MAX ? ctx->draft : ctx->hist[sel];

    if (ldg_str_rbrcpy(ctx->query, line, (uint64_t)sizeof(ctx->query)) == LDG_ERR_STR_TRUNC) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview history recall truncated"); }

    ctx->query_len = (uint32_t)strlen(ctx->query);
    ctx->hist_sel = sel;

    return LDG_ERR_AOK;
}

static uint32_t memview_hist_step(crosstalk_eda_ui_memview_ctx_t *ctx, uint8_t back_is)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->hist_cunt == 0) { return LDG_ERR_AOK; }

    if (back_is == LDG_TRUTH_TRUE)
    {
        if (ctx->hist_sel == (uint32_t)UINT32_MAX)
        {
            if (ldg_str_rbrcpy(ctx->draft, ctx->query, (uint64_t)sizeof(ctx->draft)) == LDG_ERR_STR_TRUNC) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview draft truncated"); }

            return memview_hist_recall(ctx, ctx->hist_cunt - (uint32_t)MEMVIEW_STEP);
        }

        if (ctx->hist_sel == 0) { return LDG_ERR_AOK; }

        return memview_hist_recall(ctx, ctx->hist_sel - (uint32_t)MEMVIEW_STEP);
    }

    if (ctx->hist_sel == (uint32_t)UINT32_MAX) { return LDG_ERR_AOK; }

    if (ctx->hist_sel + (uint32_t)MEMVIEW_STEP >= ctx->hist_cunt) { return memview_hist_recall(ctx, (uint32_t)UINT32_MAX); }

    return memview_hist_recall(ctx, ctx->hist_sel + (uint32_t)MEMVIEW_STEP);
}

static uint32_t memview_query_key(crosstalk_eda_ui_memview_ctx_t *ctx, uint8_t key)
{
    ctx->query_len = (uint32_t)strnlen(ctx->query, (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_QUERY_CAP);

    if (key == (uint8_t)LDG_TERM_KEY_UP) { return memview_hist_step(ctx, LDG_TRUTH_TRUE); }

    if (key == (uint8_t)LDG_TERM_KEY_DOWN) { return memview_hist_step(ctx, LDG_TRUTH_FALSE); }

    if (key == (uint8_t)LDG_TERM_KEY_ESCAPE)
    {
        ctx->query_len = 0;
        ctx->query[0] = LDG_STR_TERM;
        ctx->draft[0] = LDG_STR_TERM;
        ctx->hist_sel = (uint32_t)UINT32_MAX;
        ctx->hit_cunt = 0;
        ctx->hit_at = 0;
        ctx->finding = LDG_TRUTH_FALSE;
        ctx->stringing = LDG_TRUTH_FALSE;
        ctx->edit = LDG_TRUTH_FALSE;

        return LDG_ERR_AOK;
    }

    if (key == (uint8_t)LDG_TERM_KEY_BACKSPACE)
    {
        if (ctx->query_len != 0)
        {
            ctx->query_len = ctx->query_len - (uint32_t)MEMVIEW_STEP;
            ctx->query[ctx->query_len] = LDG_STR_TERM;
        }

        return LDG_ERR_AOK;
    }

    if (key != (uint8_t)LDG_TERM_KEY_ENTER) { return LDG_ERR_AOK; }

    if (ctx->query_len == 0)
    {
        if (ctx->hit_cunt != 0)
        {
            ctx->hit_at = (ctx->hit_at + (uint32_t)MEMVIEW_STEP) % ctx->hit_cunt;

            if (memview_hit_nav(ctx, ctx->hit[ctx->hit_at]) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview hit nav failed"); }
        }

        return LDG_ERR_AOK;
    }

    if (memview_hist_push(ctx, ctx->query) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview history push failed"); }

    if (memview_repl_exec(ctx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview repl refused"); }

    ctx->query_len = 0;
    ctx->query[0] = LDG_STR_TERM;
    ctx->draft[0] = LDG_STR_TERM;
    ctx->hist_sel = (uint32_t)UINT32_MAX;

    return LDG_ERR_AOK;
}

static uint32_t memview_span_put(ldg_term_render_t *render, uint32_t col, uint32_t row, const char *text, uint64_t len, uint32_t fg_rgba, uint32_t bg_rgba)
{
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t bg = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len == 0) { return LDG_ERR_AOK; }

    ret = crosstalk_ui_cell_color_get(fg_rgba, &fg);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get(bg_rgba, &bg); }

    if (ret != LDG_ERR_AOK) { return ret; }

    return ldg_term_render_text_draw(render, col, row, text, len, &fg, &bg);
}

static uint32_t memview_hex_col_get(uint32_t byte_col)
{
    return (uint32_t)MEMVIEW_ADDR_CHARS + (uint32_t)MEMVIEW_CHURN_CELL_INSET + byte_col * (uint32_t)MEMVIEW_BYTE_CHARS;
}

static uint32_t memview_ascii_col_get(void)
{
    return memview_hex_col_get((uint32_t)CROSSTALK_EDA_UI_MEMVIEW_COL_CUNT);
}

static uint32_t memview_cell_draw(const crosstalk_eda_ui_memview_ctx_t *ctx, ldg_term_render_t *render, uint32_t pane_col, uint32_t row, uint32_t idx, uint32_t row_col, uint32_t bg_rgba)
{
    char cell[MEMVIEW_CELL_TEXT_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t addr = ctx->base + (uint64_t)idx;
    uint64_t val = ctx->page[idx] & (uint64_t)LDG_BYTE_MASK;
    uint32_t col = pane_col + memview_hex_col_get(idx % (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_COL_CUNT);

    if (snprintf(cell, sizeof(cell), MEMVIEW_CELL_FMT, val) < 0) { return LDG_ERR_STR_FMT; }

    if (memview_hlt_blink_is(ctx) == LDG_TRUTH_TRUE && addr >= ctx->hlt_mark_addr && addr < ctx->hlt_mark_end) { return memview_span_put(render, col, row, cell, (uint64_t)MEMVIEW_HEX_CHARS, (uint32_t)CROSSTALK_UI_THEME_LIT_RGBA, (uint32_t)CROSSTALK_UI_THEME_HOT_RGBA); }

    return memview_span_put(render, col, row, cell, (uint64_t)MEMVIEW_HEX_CHARS, memview_churn_col_get(ctx->churn[idx], row_col == (uint32_t)CROSSTALK_UI_THEME_HOT_RGBA ? row_col : memview_cat_col_get(val)), bg_rgba);
}

static uint32_t memview_seps_draw(ldg_term_render_t *render, uint32_t pane_col, uint32_t rel_col)
{
    char ruler[MEMVIEW_RULER_TEXT_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t cell_w = render->sample_col_cunt;
    uint32_t cell_h = render->sample_row_cunt;
    uint32_t grid_y = (uint32_t)MEMVIEW_ROW_GRID * cell_h;
    uint32_t grid_end = grid_y + (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ROW_CUNT * cell_h;
    uint32_t group = 0;
    uint32_t ret = 0;

    for (group = 0; group < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_COL_CUNT; group = group + (uint32_t)MEMVIEW_SEP_GROUP)
    {
        uint32_t hex_col = pane_col + memview_hex_col_get(group);
        uint32_t sep_x = (hex_col - (uint32_t)MEMVIEW_CHURN_CELL_INSET) * cell_w + cell_w / (uint32_t)MEMVIEW_SEP_CENTER_DIV;
        uint32_t dash_y = grid_y;

        while (group != 0 && dash_y < grid_end)
        {
            uint32_t dash_h = grid_end - dash_y < (uint32_t)MEMVIEW_SEP_DASH_SAMPLES ? grid_end - dash_y : (uint32_t)MEMVIEW_SEP_DASH_SAMPLES;

            ret = crosstalk_ui_cell_sample_fill(render, sep_x, dash_y, (uint32_t)MEMVIEW_SEP_W_SAMPLES, dash_h, (uint32_t)CROSSTALK_UI_THEME_SEP_RGBA);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            dash_y = dash_y + (uint32_t)MEMVIEW_SEP_DASH_SAMPLES + (uint32_t)MEMVIEW_SEP_GAP_SAMPLES;
        }

        if (rel_col == UINT32_MAX || group >= rel_col) { if (snprintf(ruler, sizeof(ruler), MEMVIEW_RULER_FMT, rel_col == UINT32_MAX ? group : group - rel_col) < 0) { return LDG_ERR_STR_FMT; } }
        else if (snprintf(ruler, sizeof(ruler), MEMVIEW_RULER_NEG_FMT, rel_col - group) < 0) { return LDG_ERR_STR_FMT; }

        ret = crosstalk_ui_cell_rgba_put(render, hex_col, (uint32_t)MEMVIEW_ROW_RULER, ruler, (uint64_t)sizeof(ruler), (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, 0x0);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static LDG_NOINLINE uint32_t memview_overview_draw(const crosstalk_eda_ui_memview_ctx_t *ctx, ldg_term_render_t *render, uint32_t col_cunt)
{
    uint32_t cell_h = render->sample_row_cunt;
    uint32_t band_w = col_cunt * render->sample_col_cunt;
    uint32_t band_y = (uint32_t)MEMVIEW_ROW_OVERVIEW * cell_h;
    double scatter = ctx->heat_cunt != 0 ? (double)ctx->jmp_cunt / (double)(ctx->seq_cunt + ctx->jmp_cunt) : MEMVIEW_CHAN_NONE;
    uint32_t heat_col = memview_rgba_get(MEMVIEW_BKT_R + (MEMVIEW_HEAT_R - MEMVIEW_BKT_R) * scatter, MEMVIEW_BKT_G + (MEMVIEW_HEAT_G - MEMVIEW_BKT_G) * scatter, MEMVIEW_BKT_B + (MEMVIEW_HEAT_B - MEMVIEW_BKT_B) * scatter);
    uint32_t bkt = 0;
    uint32_t ret = 0;

    for (bkt = 0; bkt < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_BKT_CUNT; bkt = bkt + (uint32_t)MEMVIEW_STEP)
    {
        uint32_t x_lhs = (uint32_t)(((uint64_t)bkt * (uint64_t)band_w) / (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_BKT_CUNT);
        uint32_t x_rhs = (uint32_t)((((uint64_t)bkt + (uint64_t)MEMVIEW_STEP) * (uint64_t)band_w) / (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_BKT_CUNT);
        double lit = MEMVIEW_BKT_EMPTY + ((double)ctx->bkt[bkt] / (double)LDG_BYTE_MASK) * (MEMVIEW_BKT_FULL - MEMVIEW_BKT_EMPTY);
        uint32_t heat_h = (uint32_t)(((uint64_t)cell_h * (uint64_t)ctx->heat[bkt]) / (uint64_t)LDG_BYTE_MASK);

        if (x_rhs <= x_lhs) { continue; }

        ret = crosstalk_ui_cell_sample_fill(render, x_lhs, band_y, x_rhs - x_lhs, cell_h, memview_rgba_get(MEMVIEW_BKT_R * lit, MEMVIEW_BKT_G * lit, MEMVIEW_BKT_B * lit));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (heat_h == 0) { continue; }

        ret = crosstalk_ui_cell_sample_fill(render, x_lhs, band_y, x_rhs - x_lhs, heat_h, heat_col);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (ctx->width != 0 && ctx->base < ctx->width)
    {
        uint32_t mark_x = (uint32_t)(((double)ctx->base / (double)ctx->width) * (double)band_w);

        ret = crosstalk_ui_cell_sample_fill(render, mark_x, band_y, (uint32_t)MEMVIEW_MARK_SAMPLES, cell_h, (uint32_t)CROSSTALK_UI_THEME_MARK_RGBA);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (ctx->io_on == LDG_TRUTH_TRUE && ctx->io_cunt != 0)
    {
        uint32_t io_y = (uint32_t)MEMVIEW_ROW_IO * cell_h;
        uint32_t gaps = (uint32_t)MEMVIEW_IO_SEG_GAP_SAMPLES * (ctx->io_cunt - (uint32_t)MEMVIEW_STEP);
        uint32_t seg_w = band_w > gaps ? (band_w - gaps) / ctx->io_cunt : (uint32_t)MEMVIEW_STEP;
        uint32_t seg = 0;

        for (seg = 0; seg < ctx->io_cunt; seg = seg + (uint32_t)MEMVIEW_STEP)
        {
            uint8_t here = ctx->base >= ctx->io_base[seg] && ctx->base < ctx->io_base[seg] + ctx->io_span[seg] ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
            double lit = here == LDG_TRUTH_TRUE ? MEMVIEW_IO_LIT : MEMVIEW_IO_DIM;

            ret = crosstalk_ui_cell_sample_fill(render, seg * (seg_w + (uint32_t)MEMVIEW_IO_SEG_GAP_SAMPLES), io_y, seg_w, cell_h / (uint32_t)MEMVIEW_IO_BAND_DIV, memview_rgba_get(MEMVIEW_IO_R * lit, MEMVIEW_IO_G * lit, MEMVIEW_IO_B * lit));
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }

    return LDG_ERR_AOK;
}

static LDG_NOINLINE uint32_t memview_hdr_draw(const crosstalk_eda_ui_memview_ctx_t *ctx, ldg_term_render_t *render, uint32_t pane_end, uint32_t go_col)
{
    char text[MEMVIEW_ROW_TEXT_CAP] = LDG_ARR_ZERO_INIT;
    char stat[MEMVIEW_STAT_CAP] = LDG_ARR_ZERO_INIT;
    char hover_txt[MEMVIEW_HOVER_TEXT_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t len = 0;
    uint32_t ret = 0;

    if (ctx->finding == LDG_TRUTH_TRUE) { snprintf(stat, sizeof(stat), MEMVIEW_FIND_RUN_TEXT); }
    else if (ctx->stringing == LDG_TRUTH_TRUE) { snprintf(stat, sizeof(stat), MEMVIEW_STRINGS_RUN_TEXT); }
    else if (ctx->watch_mode != (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WATCH_OFF) { snprintf(stat, sizeof(stat), MEMVIEW_WATCH_RUN_TEXT); }
    else if (ctx->hit_cunt != 0) { snprintf(stat, sizeof(stat), MEMVIEW_FIND_DONE_FMT, ctx->hit_cunt); }
    else if (ctx->diff_mode == LDG_TRUTH_TRUE) { snprintf(stat, sizeof(stat), MEMVIEW_DIFF_STAT_FMT, ctx->diff_cunt); }
    else if (ctx->snap_mode == LDG_TRUTH_TRUE) { snprintf(stat, sizeof(stat), MEMVIEW_SNAP_IDLE_TEXT); }
    else { snprintf(stat, sizeof(stat), MEMVIEW_FIND_IDLE_TEXT); }

    if (go_col != UINT32_MAX)
    {
        char hover_name[MEMVIEW_FIELD_TEXT_CAP] = LDG_ARR_ZERO_INIT;
        uint64_t hover_tgt = 0;
        uint32_t hover_kind = 0;
        uint32_t hover_pos = 0;
        int32_t wrote = 0;

        wrote = snprintf(hover_txt, sizeof(hover_txt), MEMVIEW_HOVER_FMT, ctx->go_addr);

        if (wrote > 0 && (uint32_t)wrote < (uint32_t)sizeof(hover_txt)) { hover_pos = (uint32_t)wrote; }

        if (memview_field_annot_get(ctx, ctx->go_addr, hover_name, (uint32_t)sizeof(hover_name), &hover_kind, &hover_tgt, LDG_TRUTH_FALSE) != LDG_ERR_AOK || hover_name[0] == LDG_STR_TERM) { if (memview_annot_at_get(ctx, ctx->go_addr, hover_name, (uint32_t)sizeof(hover_name)) != LDG_ERR_AOK) { hover_name[0] = LDG_STR_TERM; } }

        if (hover_name[0] != LDG_STR_TERM && hover_pos != 0) { snprintf(hover_txt + hover_pos, sizeof(hover_txt) - hover_pos, MEMVIEW_HOVER_SYM_FMT, hover_name); }
    }

    if (snprintf(text, sizeof(text), MEMVIEW_HDR_FMT, ctx->sig, ctx->width, ctx->base, stat, hover_txt) < 0) { return LDG_ERR_STR_FMT; }

    ret = ldg_str_len_get(text, (uint64_t)sizeof(text), &len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (len > (uint64_t)pane_end) { len = (uint64_t)pane_end; }

    return memview_span_put(render, (uint32_t)MEMVIEW_ORIG_COL, (uint32_t)MEMVIEW_ROW_HDR, text, len, (uint32_t)CROSSTALK_UI_THEME_DATA_RGBA, (uint32_t)CROSSTALK_UI_CELL_PANE_RGBA);
}

static LDG_NOINLINE uint32_t memview_mut_draw(const crosstalk_eda_ui_memview_ctx_t *ctx, ldg_term_render_t *render)
{
    char text[MEMVIEW_ROW_TEXT_CAP] = LDG_ARR_ZERO_INIT;
    char ascii[CROSSTALK_EDA_UI_MEMVIEW_COL_CUNT + LDG_STR_TERM_SIZE] = LDG_ARR_ZERO_INIT;
    uint32_t pane_col = ctx->mut_pane_col;
    uint32_t cell_w = render->sample_col_cunt;
    uint32_t cell_h = render->sample_row_cunt;
    uint32_t row = 0;
    uint32_t col = 0;
    uint32_t ret = 0;

    ret = crosstalk_ui_cell_sample_fill(render, (pane_col - (uint32_t)MEMVIEW_STEP) * cell_w + cell_w / (uint32_t)MEMVIEW_SEP_CENTER_DIV, (uint32_t)MEMVIEW_ROW_HDR * cell_h, (uint32_t)MEMVIEW_SEP_W_SAMPLES, ((uint32_t)MEMVIEW_ROW_GRID + (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ROW_CUNT - (uint32_t)MEMVIEW_ROW_HDR) * cell_h, (uint32_t)CROSSTALK_UI_THEME_SEP_RGBA);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->mut_addr == UINT64_MAX)
    {
        ret = crosstalk_ui_cell_rgba_put(render, pane_col, (uint32_t)MEMVIEW_ROW_HDR, MEMVIEW_MUT_TITLE, (uint64_t)MEMVIEW_ROW_TEXT_CAP, (uint32_t)CROSSTALK_UI_THEME_DATA_RGBA, 0x0);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        return crosstalk_ui_cell_rgba_put(render, pane_col, (uint32_t)MEMVIEW_ROW_GRID, MEMVIEW_MUT_IDLE, (uint64_t)MEMVIEW_ROW_TEXT_CAP, (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, 0x0);
    }

    if (LDG_UNLIKELY(snprintf(text, sizeof(text), MEMVIEW_MUT_ADDR_FMT, ctx->mut_addr) < 0)) { return LDG_ERR_STR_FMT; }

    ret = crosstalk_ui_cell_rgba_put(render, pane_col, (uint32_t)MEMVIEW_ROW_HDR, text, (uint64_t)sizeof(text), (uint32_t)CROSSTALK_UI_THEME_DATA_RGBA, 0x0);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (row = 0; row < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ROW_CUNT; row++)
    {
        uint32_t first = row * (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_COL_CUNT;
        uint32_t grid_row = (uint32_t)MEMVIEW_ROW_GRID + row;

        if (first >= ctx->mut_page_cunt) { break; }

        if (LDG_UNLIKELY(snprintf(text, sizeof(text), MEMVIEW_ROW_FMT, ctx->mut_base + first) < 0)) { return LDG_ERR_STR_FMT; }

        ret = crosstalk_ui_cell_rgba_put(render, pane_col, grid_row, text, (uint64_t)sizeof(text), (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, 0x0);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        for (col = 0; col < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_COL_CUNT && first + col < ctx->mut_page_cunt; col++)
        {
            uint64_t val = ctx->mut_page[first + col] & (uint64_t)LDG_BYTE_MASK;
            uint32_t color = ctx->mut_base + first + col == ctx->mut_addr ? (uint32_t)MEMVIEW_CHURN_HOT_COL : memview_cat_col_get(val);

            ascii[col] = val >= (uint64_t)LDG_ASCII_PRINTABLE_MIN && val <= (uint64_t)LDG_ASCII_PRINTABLE_MAX ? (char)val : (char)LDG_ASCII_NONPRINTABLE_CHAR;

            if (LDG_UNLIKELY(snprintf(text, sizeof(text), MEMVIEW_CELL_FMT, val) < 0)) { return LDG_ERR_STR_FMT; }

            ret = crosstalk_ui_cell_rgba_put(render, pane_col + memview_hex_col_get(col), grid_row, text, (uint64_t)sizeof(text), color, 0x0);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ascii[col] = LDG_STR_TERM;

        ret = crosstalk_ui_cell_rgba_put(render, pane_col + memview_ascii_col_get(), grid_row, ascii, (uint64_t)sizeof(ascii), (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, 0x0);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return memview_seps_draw(render, pane_col, UINT32_MAX);
}

static LDG_NOINLINE uint32_t memview_annot_draw(crosstalk_eda_ui_memview_ctx_t *ctx, ldg_term_render_t *render, uint32_t row, uint32_t pane_end, uint32_t *out_w)
{
    char frag[MEMVIEW_ANNOT_TEXT_CAP] = LDG_ARR_ZERO_INIT;
    char name[MEMVIEW_FIELD_TEXT_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t first = row * (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_COL_CUNT;
    uint64_t row_addr = ctx->base + (uint64_t)first;
    uint32_t grid_row = (uint32_t)MEMVIEW_ROW_GRID + row;
    uint32_t annot_col = memview_ascii_col_get() + (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_COL_CUNT + (uint32_t)MEMVIEW_ANNOT_GAP_CHARS;
    uint32_t avail = pane_end > annot_col ? pane_end - annot_col : (uint32_t)MEMVIEW_ANNOT_AVAIL_NONE;
    uint32_t pos = 0;
    uint32_t col = 0;
    uint32_t ret = 0;

    *out_w = 0;

    for (col = 0; col < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_COL_CUNT; col = col + (uint32_t)MEMVIEW_SEP_GROUP)
    {
        uint64_t tgt = 0;
        uint32_t kind = (uint32_t)CROSSTALK_HDL_TYPE_KIND_NONE;
        uint32_t frag_col = (uint32_t)CROSSTALK_UI_THEME_LIT_RGBA;
        uint32_t span = 0;
        uint32_t vis_lo = 0;
        uint32_t vis_hi = 0;
        uint8_t churn_max = 0;
        uint8_t field_is = LDG_TRUTH_FALSE;
        int32_t wrote = 0;

        if (first + col >= ctx->page_cunt) { break; }

        if (memview_field_annot_get(ctx, row_addr + (uint64_t)col, name, (uint32_t)sizeof(name), &kind, &tgt, LDG_TRUTH_TRUE) == LDG_ERR_AOK && name[0] != LDG_STR_TERM) { field_is = LDG_TRUTH_TRUE; }
        else if (memview_annot_at_get(ctx, row_addr + (uint64_t)col, name, (uint32_t)sizeof(name)) == LDG_ERR_AOK && name[0] != LDG_STR_TERM) { field_is = LDG_TRUTH_FALSE; }
        else if (ctx->ptrs_on == LDG_TRUTH_TRUE && memview_ptr_shape_is(ctx, row_addr + (uint64_t)col, &tgt) == LDG_TRUTH_TRUE)
        {
            name[0] = LDG_STR_TERM;
            kind = (uint32_t)CROSSTALK_HDL_TYPE_KIND_PTR;
            field_is = LDG_TRUTH_TRUE;
        }
        else { continue; }

        if (field_is == LDG_TRUTH_TRUE && kind == (uint32_t)CROSSTALK_HDL_TYPE_KIND_PTR) { frag_col = (uint32_t)MEMVIEW_ANNOT_PTR_COL; }

        for (span = 0; span < (uint32_t)MEMVIEW_SEP_GROUP && first + col + span < ctx->page_cunt; span = span + (uint32_t)MEMVIEW_STEP)
        {
            if (ctx->churn[first + col + span] > churn_max) { churn_max = ctx->churn[first + col + span]; }

            ret = memview_cell_draw(ctx, render, (uint32_t)MEMVIEW_ORIG_COL, grid_row, first + col + span, (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, (uint32_t)MEMVIEW_ANNOT_TINT_COL);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        frag_col = memview_churn_col_get(churn_max, frag_col);

        if (field_is == LDG_TRUTH_TRUE && kind == (uint32_t)CROSSTALK_HDL_TYPE_KIND_PTR && name[0] == LDG_STR_TERM) { wrote = snprintf(frag, sizeof(frag), MEMVIEW_ANNOT_RAW_PTR_FMT, col, tgt); }
        else if (field_is == LDG_TRUTH_TRUE && kind == (uint32_t)CROSSTALK_HDL_TYPE_KIND_PTR) { wrote = snprintf(frag, sizeof(frag), MEMVIEW_ANNOT_PTR_FMT, col, name, tgt); }
        else if (field_is == LDG_TRUTH_TRUE) { wrote = snprintf(frag, sizeof(frag), MEMVIEW_ANNOT_FIELD_FMT, col, name); }
        else { wrote = snprintf(frag, sizeof(frag), MEMVIEW_ANNOT_FMT, col, name); }

        if (wrote < 0 || (uint32_t)wrote >= (uint32_t)sizeof(frag)) { break; }

        vis_lo = pos > ctx->annot_pan ? pos : ctx->annot_pan;
        vis_hi = pos + (uint32_t)wrote < ctx->annot_pan + avail ? pos + (uint32_t)wrote : ctx->annot_pan + avail;

        if (vis_hi > vis_lo)
        {
            ret = memview_span_put(render, annot_col + vis_lo - ctx->annot_pan, grid_row, frag + (vis_lo - pos), (uint64_t)(vis_hi - vis_lo), frag_col, (uint32_t)CROSSTALK_UI_CELL_PANE_RGBA);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        pos = pos + (uint32_t)wrote;
    }

    *out_w = pos;

    return LDG_ERR_AOK;
}

static LDG_NOINLINE uint32_t memview_rows_draw(crosstalk_eda_ui_memview_ctx_t *ctx, ldg_term_render_t *render, uint32_t pane_end, uint32_t go_row, uint32_t go_col)
{
    char text[MEMVIEW_ROW_TEXT_CAP] = LDG_ARR_ZERO_INIT;
    char ascii[CROSSTALK_EDA_UI_MEMVIEW_COL_CUNT + LDG_STR_TERM_SIZE] = LDG_ARR_ZERO_INIT;
    uint32_t annot_max = 0;
    uint32_t row = 0;
    uint32_t ret = 0;

    for (row = 0; row < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ROW_CUNT; row = row + (uint32_t)MEMVIEW_STEP)
    {
        uint32_t first = row * (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_COL_CUNT;
        uint32_t grid_row = (uint32_t)MEMVIEW_ROW_GRID + row;
        uint32_t row_col = (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA;
        uint32_t annot_w = 0;
        uint32_t col = 0;

        if (first >= ctx->page_cunt) { break; }

        if (ctx->diff_mode == LDG_TRUTH_TRUE) { for (col = 0; col < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_COL_CUNT; col = col + (uint32_t)MEMVIEW_STEP)
            {
                uint32_t idx = first + col;
                if (idx >= ctx->page_cunt || idx >= ctx->diff_page_cunt) { break; }

                if (((ctx->page[idx] ^ ctx->diff_page[idx]) & (uint64_t)LDG_BYTE_MASK) != 0) { row_col = (uint32_t)CROSSTALK_UI_THEME_HOT_RGBA; break; }
            }
        }

        if (snprintf(text, sizeof(text), MEMVIEW_ROW_FMT, ctx->base + (uint64_t)first) < 0) { return LDG_ERR_STR_FMT; }

        ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)MEMVIEW_ORIG_COL, grid_row, text, (uint64_t)sizeof(text), row_col, 0x0);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        for (col = 0; col < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_COL_CUNT; col = col + (uint32_t)MEMVIEW_STEP)
        {
            uint32_t idx = first + col;
            uint64_t val = 0;

            if (idx >= ctx->page_cunt) { break; }

            val = ctx->page[idx] & (uint64_t)LDG_BYTE_MASK;
            ascii[col] = (val >= (uint64_t)LDG_ASCII_PRINTABLE_MIN && val <= (uint64_t)LDG_ASCII_PRINTABLE_MAX) ? (char)val : (char)LDG_ASCII_NONPRINTABLE_CHAR;

            ret = memview_cell_draw(ctx, render, (uint32_t)MEMVIEW_ORIG_COL, grid_row, idx, row_col, (uint32_t)CROSSTALK_UI_CELL_PANE_RGBA);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ascii[col] = LDG_STR_TERM;

        ret = crosstalk_ui_cell_rgba_put(render, memview_ascii_col_get(), grid_row, ascii, (uint64_t)sizeof(ascii), row_col, 0x0);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = memview_annot_draw(ctx, render, row, pane_end, &annot_w);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (annot_w > annot_max) { annot_max = annot_w; }

        if (row == go_row)
        {
            ret = memview_cell_draw(ctx, render, (uint32_t)MEMVIEW_ORIG_COL, grid_row, first + go_col, row_col, (uint32_t)MEMVIEW_GO_BG_COL);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }

    ctx->annot_span = annot_max > (uint32_t)MEMVIEW_ANNOT_PAN_SLACK ? annot_max - (uint32_t)MEMVIEW_ANNOT_PAN_SLACK : (uint32_t)MEMVIEW_ANNOT_PAN_NONE;

    if (ctx->annot_pan > ctx->annot_span) { ctx->annot_pan = ctx->annot_span; }

    return LDG_ERR_AOK;
}

static uint32_t memview_tabs_draw(const crosstalk_eda_ui_memview_ctx_t *ctx, ldg_term_render_t *render)
{
    const char *tab_text[MEMVIEW_TAB_CUNT] = { MEMVIEW_TAB_HEX_TEXT, MEMVIEW_TAB_HITS_TEXT };
    uint32_t pos_col = MEMVIEW_ORIG_COL;
    uint32_t seg_w = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    for (at = 0; at < (uint32_t)MEMVIEW_TAB_CUNT; at = at + (uint32_t)MEMVIEW_STEP)
    {
        ret = crosstalk_ui_cell_rgba_put(render, pos_col, (uint32_t)MEMVIEW_ROW_TAB, tab_text[at], (uint64_t)MEMVIEW_TAB_TEXT_CAP, ctx->tab == (uint8_t)at ? (uint32_t)CROSSTALK_UI_THEME_LIT_RGBA : (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, &seg_w);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        pos_col = pos_col + seg_w + (uint32_t)MEMVIEW_TAB_GAP;
    }

    return LDG_ERR_AOK;
}

static LDG_NOINLINE uint32_t memview_dis_list_draw(crosstalk_eda_ui_memview_ctx_t *ctx, ldg_term_render_t *render, uint32_t col_cunt)
{
    uint32_t col_w = col_cunt / (uint32_t)MEMVIEW_DIS_COL_CUNT;
    uint32_t text_w = col_w > (uint32_t)MEMVIEW_DIS_COL_GAP ? col_w - (uint32_t)MEMVIEW_DIS_COL_GAP : (uint32_t)MEMVIEW_STEP;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (ctx->dis_sel >= ctx->dis_row_cunt) { ctx->dis_sel = ctx->dis_row_cunt - (uint32_t)MEMVIEW_STEP; }

    for (at = 0; at < ctx->dis_row_cunt && at < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_DIS_ROW_CAP; at = at + (uint32_t)MEMVIEW_STEP)
    {
        uint64_t len = (uint64_t)strnlen(ctx->dis_row[at], (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_DIS_ROW_TEXT_CAP);
        uint8_t sel_is = at == ctx->dis_sel ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

        if (len > (uint64_t)text_w) { len = (uint64_t)text_w; }

        ret = memview_span_put(render, (at / (uint32_t)MEMVIEW_DIS_ROW_PER_COL) * col_w, (uint32_t)MEMVIEW_ROW_HDR + at % (uint32_t)MEMVIEW_DIS_ROW_PER_COL, ctx->dis_row[at], len, sel_is == LDG_TRUTH_TRUE ? (uint32_t)CROSSTALK_UI_THEME_LIT_RGBA : (uint32_t)CROSSTALK_UI_THEME_DATA_RGBA, sel_is == LDG_TRUTH_TRUE ? (uint32_t)CROSSTALK_UI_THEME_SEL_BG_RGBA : (uint32_t)CROSSTALK_UI_CELL_PANE_RGBA);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ctx->win.content_row_cunt = (uint32_t)MEMVIEW_ROW_HDR + (ctx->dis_row_cunt < (uint32_t)MEMVIEW_DIS_ROW_PER_COL ? ctx->dis_row_cunt : (uint32_t)MEMVIEW_DIS_ROW_PER_COL);

    return LDG_ERR_AOK;
}

static LDG_NOINLINE uint32_t memview_hit_list_draw(crosstalk_eda_ui_memview_ctx_t *ctx, ldg_term_render_t *render, uint32_t col_cunt)
{
    char hit_text[MEMVIEW_TAB_TEXT_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t cols_fit = col_cunt / (uint32_t)MEMVIEW_HIT_CELL_W;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (ctx->hit_cunt == 0)
    {
        ctx->win.content_row_cunt = (uint32_t)MEMVIEW_ROW_HDR + (uint32_t)MEMVIEW_STEP;

        return crosstalk_ui_cell_rgba_put(render, (uint32_t)MEMVIEW_ORIG_COL, (uint32_t)MEMVIEW_ROW_HDR, MEMVIEW_HITS_NONE_TEXT, (uint64_t)MEMVIEW_ROW_TEXT_CAP, (uint32_t)CROSSTALK_UI_THEME_DATA_RGBA, 0x0);
    }

    if (cols_fit == 0) { cols_fit = (uint32_t)MEMVIEW_STEP; }

    if (ctx->hit_at >= ctx->hit_cunt) { ctx->hit_at = (uint32_t)MEMVIEW_SEL_FIRST; }

    ctx->hit_rows = (ctx->hit_cunt + cols_fit - (uint32_t)MEMVIEW_STEP) / cols_fit;

    for (at = 0; at < ctx->hit_cunt && at < (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_HIT_CAP; at = at + (uint32_t)MEMVIEW_STEP)
    {
        uint32_t col = (at / ctx->hit_rows) * (uint32_t)MEMVIEW_HIT_CELL_W;
        uint32_t row = (uint32_t)MEMVIEW_ROW_HDR + at % ctx->hit_rows;
        uint8_t sel_is = at == ctx->hit_at ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
        int32_t wrote = 0;

        if (ctx->hit_gen[at] != (uint32_t)MEMVIEW_HIT_GEN_NONE)
        {
            uint32_t gen_col = memview_hit_gen_col_get(ctx->hit_gen[at]);

            ret = memview_span_put(render, col, row, MEMVIEW_HIT_GEN_TEXT, (uint64_t)MEMVIEW_HIT_GEN_CELLS, gen_col, gen_col);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        wrote = snprintf(hit_text, sizeof(hit_text), MEMVIEW_HIT_FMT, ctx->hit[at]);
        if (wrote < 0) { return LDG_ERR_STR_FMT; }

        ret = memview_span_put(render, col + (uint32_t)MEMVIEW_HIT_GEN_CELLS, row, hit_text, (uint64_t)wrote, sel_is == LDG_TRUTH_TRUE ? (uint32_t)CROSSTALK_UI_THEME_LIT_RGBA : (uint32_t)CROSSTALK_UI_THEME_DATA_RGBA, sel_is == LDG_TRUTH_TRUE ? (uint32_t)CROSSTALK_UI_THEME_SEL_BG_RGBA : (uint32_t)CROSSTALK_UI_CELL_PANE_RGBA);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ctx->win.content_row_cunt = (uint32_t)MEMVIEW_ROW_HDR + ctx->hit_rows;

    return LDG_ERR_AOK;
}

static uint32_t memview_repl_draw(const crosstalk_eda_ui_memview_ctx_t *ctx, ldg_term_render_t *render, uint32_t row)
{
    ldg_term_render_color_t cursor = LDG_STRUCT_ZERO_INIT;
    uint64_t len = 0;
    uint32_t ret = 0;

    ret = crosstalk_ui_cell_rgba_put(render, (uint32_t)MEMVIEW_ORIG_COL, row, MEMVIEW_REPL_MARK, (uint64_t)MEMVIEW_TAB_TEXT_CAP, (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, 0x0);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_str_len_get(ctx->query, (uint64_t)sizeof(ctx->query), &len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = memview_span_put(render, (uint32_t)MEMVIEW_REPL_MARK_W, row, ctx->query, len, (uint32_t)CROSSTALK_UI_THEME_DATA_RGBA, (uint32_t)CROSSTALK_UI_CELL_PANE_RGBA);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->focused != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_DATA_RGBA, &cursor);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return crosstalk_ui_cell_span_fill(render, (uint32_t)MEMVIEW_REPL_MARK_W + (uint32_t)len, row, (uint32_t)MEMVIEW_REPL_CURSOR_W, &cursor, &cursor);
}

static uint32_t memview_draw_cb(uint64_t win_id, void *user, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    crosstalk_eda_ui_memview_ctx_t *ctx = (crosstalk_eda_ui_memview_ctx_t *)user;
    uint32_t pane_end = col_cunt;
    uint32_t min_w = memview_ascii_col_get() + (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_COL_CUNT + (uint32_t)MEMVIEW_MUT_PANE_GAP;
    uint32_t repl_row = (uint32_t)MEMVIEW_ROW_GRID + (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ROW_CUNT + (uint32_t)MEMVIEW_ROW_REPL_GAP;
    uint32_t go_row = UINT32_MAX;
    uint32_t go_col = UINT32_MAX;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (win_id != (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_WINDOW_ID || col_cunt == 0 || row_cunt == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = memview_tabs_draw(ctx, render);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->tab == (uint8_t)CROSSTALK_EDA_UI_MEMVIEW_TAB_HITS)
    {
        if (ctx->res_kind == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_RES_DIS && ctx->dis_row_cunt != 0) { return memview_dis_list_draw(ctx, render, col_cunt); }

        return memview_hit_list_draw(ctx, render, col_cunt);
    }

    ctx->mut_visible = LDG_TRUTH_FALSE;
    ctx->mut_pane_col = col_cunt;

    if (ctx->mut_supported == LDG_TRUTH_TRUE && col_cunt >= (uint32_t)MEMVIEW_MUT_PANE_CUNT * min_w)
    {
        ctx->mut_visible = LDG_TRUTH_TRUE;
        ctx->mut_pane_col = col_cunt / (uint32_t)MEMVIEW_MUT_PANE_CUNT;
        pane_end = ctx->mut_pane_col - (uint32_t)MEMVIEW_MUT_PANE_GAP;
    }

    if (ctx->go_has == LDG_TRUTH_TRUE && ctx->go_addr >= ctx->base && ctx->go_addr - ctx->base < (uint64_t)ctx->page_cunt)
    {
        go_row = (uint32_t)((ctx->go_addr - ctx->base) / (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_COL_CUNT);
        go_col = (uint32_t)((ctx->go_addr - ctx->base) % (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_COL_CUNT);
    }

    ret = memview_hdr_draw(ctx, render, pane_end, go_col);
    if (ret == LDG_ERR_AOK) { ret = memview_overview_draw(ctx, render, col_cunt); }

    if (ret == LDG_ERR_AOK) { ret = memview_rows_draw(ctx, render, pane_end, go_row, go_col); }

    if (ret == LDG_ERR_AOK) { ret = memview_seps_draw(render, (uint32_t)MEMVIEW_ORIG_COL, go_col); }

    if (ret == LDG_ERR_AOK && ctx->mut_visible == LDG_TRUTH_TRUE) { ret = memview_mut_draw(ctx, render); }

    if (ret == LDG_ERR_AOK) { ret = memview_repl_draw(ctx, render, repl_row); }

    ctx->win.content_row_cunt = repl_row + (uint32_t)MEMVIEW_STEP;

    return ret;
}

static uint32_t memview_io_cycle(crosstalk_eda_ui_memview_ctx_t *ctx, uint8_t back)
{
    uint32_t seg = ctx->io_cunt;
    uint32_t at = 0;

    if (ctx->io_on != LDG_TRUTH_TRUE || ctx->io_cunt == 0) { return LDG_ERR_EMPTY; }

    for (at = 0; at < ctx->io_cunt; at = at + (uint32_t)MEMVIEW_STEP) { if (ctx->base >= ctx->io_base[at] && ctx->base < ctx->io_base[at] + ctx->io_span[at]) { seg = at; break; } }

    if (seg == ctx->io_cunt) { seg = back == LDG_TRUTH_TRUE ? ctx->io_cunt - (uint32_t)MEMVIEW_STEP : (uint32_t)MEMVIEW_IO_SEG_FIRST; }
    else if (back == LDG_TRUTH_TRUE) { seg = seg == 0 ? ctx->io_cunt - (uint32_t)MEMVIEW_STEP : seg - (uint32_t)MEMVIEW_STEP; }
    else { seg = (seg + (uint32_t)MEMVIEW_STEP) % ctx->io_cunt; }

    ctx->base = ctx->io_base[seg];
    ctx->follow = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_FOLLOW_FIXED;
    ctx->refresh_next = MEMVIEW_SCROLL_NONE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview: io range; name: %.23s; base: 0x%X64; span: 0x%X64", ctx->io_name[seg], ctx->io_base[seg], ctx->io_span[seg]);

    return LDG_ERR_AOK;
}

static uint32_t memview_results_key(crosstalk_eda_ui_memview_ctx_t *ctx, uint8_t key, uint8_t *consumed)
{
    uint8_t dis_is = ctx->res_kind == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_RES_DIS && ctx->dis_row_cunt != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    uint32_t cunt = dis_is == LDG_TRUTH_TRUE ? ctx->dis_row_cunt : ctx->hit_cunt;
    uint32_t page = dis_is == LDG_TRUTH_TRUE ? (uint32_t)MEMVIEW_DIS_ROW_PER_COL : ctx->hit_rows;
    uint32_t *sel = dis_is == LDG_TRUTH_TRUE ? &ctx->dis_sel : &ctx->hit_at;

    *consumed = LDG_TRUTH_TRUE;

    if (key == (uint8_t)LDG_TERM_KEY_ESCAPE) { ctx->tab = (uint8_t)CROSSTALK_EDA_UI_MEMVIEW_TAB_HEX; return LDG_ERR_AOK; }

    if (cunt == 0) { *consumed = LDG_TRUTH_FALSE; return LDG_ERR_AOK; }

    if (page == 0) { page = (uint32_t)MEMVIEW_STEP; }

    if (key == (uint8_t)LDG_TERM_KEY_UP) { if (*sel != 0) { *sel = *sel - (uint32_t)MEMVIEW_STEP; } }
    else if (key == (uint8_t)LDG_TERM_KEY_DOWN) { if (*sel + (uint32_t)MEMVIEW_STEP < cunt) { *sel = *sel + (uint32_t)MEMVIEW_STEP; } }
    else if (key == (uint8_t)LDG_TERM_KEY_LHS) { *sel = *sel > page ? *sel - page : (uint32_t)MEMVIEW_SEL_FIRST; }
    else if (key == (uint8_t)LDG_TERM_KEY_RHS) { *sel = *sel + page < cunt ? *sel + page : cunt - (uint32_t)MEMVIEW_STEP; }
    else if (key == (uint8_t)LDG_TERM_KEY_ENTER)
    {
        if (dis_is == LDG_TRUTH_TRUE) { if (crosstalk_eda_ui_memview_addr_set(ctx, ctx->dis_row_addr[*sel] - ctx->dis_row_addr[*sel] % (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_COL_CUNT) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview dis row jmp failed"); } }
        else if (crosstalk_eda_ui_memview_hit_jmp(ctx, *sel) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview hit jmp failed"); }

        ctx->tab = (uint8_t)CROSSTALK_EDA_UI_MEMVIEW_TAB_HEX;
    }
    else { *consumed = LDG_TRUTH_FALSE; }

    return LDG_ERR_AOK;
}

static uint32_t memview_key_isr(uint64_t win_id, void *user, const ldg_term_key_event_t *key, uint8_t *consumed)
{
    crosstalk_eda_ui_memview_ctx_t *ctx = (crosstalk_eda_ui_memview_ctx_t *)user;
    uint8_t shift_is = LDG_TRUTH_FALSE;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || key == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    if (win_id != (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_WINDOW_ID) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (key->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE || ctx->bound != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if ((key->mods & ((uint32_t)LDG_TERM_MOD_CTRL | (uint32_t)LDG_TERM_MOD_ALT | (uint32_t)LDG_TERM_MOD_SUPER)) != 0) { return LDG_ERR_AOK; }

    if (ctx->tab == (uint8_t)CROSSTALK_EDA_UI_MEMVIEW_TAB_HITS) { return memview_results_key(ctx, key->key, consumed); }

    shift_is = (key->mods & (uint32_t)LDG_TERM_MOD_SHIFT) != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    ctx->query_len = (uint32_t)strnlen(ctx->query, (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_QUERY_CAP);
    *consumed = LDG_TRUTH_TRUE;

    if (key->key == (uint8_t)LDG_TERM_KEY_PAGE_DOWN) { return memview_step_rows(ctx, (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ROW_CUNT, LDG_TRUTH_FALSE); }

    if (key->key == (uint8_t)LDG_TERM_KEY_PAGE_UP) { return memview_step_rows(ctx, (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ROW_CUNT, LDG_TRUTH_TRUE); }

    if (key->key == (uint8_t)LDG_TERM_KEY_HOME)
    {
        if (ctx->query_len == 0)
        {
            ctx->base = 0x0;
            ctx->follow = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_FOLLOW_FIXED;
            ctx->refresh_next = MEMVIEW_SCROLL_NONE;
        }

        return LDG_ERR_AOK;
    }

    if (key->key == (uint8_t)LDG_TERM_KEY_LHS || key->key == (uint8_t)LDG_TERM_KEY_RHS)
    {
        uint8_t back = key->key == (uint8_t)LDG_TERM_KEY_LHS ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

        if (shift_is != LDG_TRUTH_TRUE) { return memview_step_rows(ctx, (uint32_t)MEMVIEW_SCROLL_ROWS, back); }

        if (back == LDG_TRUTH_TRUE) { ctx->annot_pan = ctx->annot_pan > (uint32_t)MEMVIEW_ANNOT_PAN_STEP ? ctx->annot_pan - (uint32_t)MEMVIEW_ANNOT_PAN_STEP : (uint32_t)MEMVIEW_ANNOT_PAN_NONE; }
        else { ctx->annot_pan = ctx->annot_pan + (uint32_t)MEMVIEW_ANNOT_PAN_STEP < ctx->annot_span ? ctx->annot_pan + (uint32_t)MEMVIEW_ANNOT_PAN_STEP : ctx->annot_span; }

        return LDG_ERR_AOK;
    }

    if (key->key == (uint8_t)LDG_TERM_KEY_TAB)
    {
        if (memview_io_cycle(ctx, shift_is) == LDG_ERR_EMPTY) { *consumed = LDG_TRUTH_FALSE; }

        return LDG_ERR_AOK;
    }

    if (key->key == (uint8_t)LDG_TERM_KEY_UP || key->key == (uint8_t)LDG_TERM_KEY_DOWN || key->key == (uint8_t)LDG_TERM_KEY_ESCAPE || key->key == (uint8_t)LDG_TERM_KEY_BACKSPACE || key->key == (uint8_t)LDG_TERM_KEY_ENTER) { return memview_query_key(ctx, key->key); }

    *consumed = LDG_TRUTH_FALSE;

    for (at = 0; at < key->text_cunt && at < (uint32_t)LDG_TERM_KEY_TEXT_MAX; at = at + (uint32_t)MEMVIEW_STEP)
    {
        if (key->text[at] < (uint32_t)LDG_ASCII_PRINTABLE_MIN || key->text[at] > (uint32_t)LDG_ASCII_PRINTABLE_MAX) { continue; }

        *consumed = LDG_TRUTH_TRUE;

        if (ctx->query_len + (uint32_t)LDG_STR_TERM_SIZE >= (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_QUERY_CAP) { break; }

        ctx->query[ctx->query_len] = (char)key->text[at];
        ctx->query_len = ctx->query_len + (uint32_t)MEMVIEW_STEP;
        ctx->query[ctx->query_len] = LDG_STR_TERM;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_memview_win_reg(crosstalk_eda_ui_memview_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot)
{
    if (LDG_UNLIKELY(ctx == 0x0 || win_nav == 0x0 || out_slot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_ui_window_reg(win_nav, (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_WINDOW_ID, CROSSTALK_EDA_UI_MEMVIEW_TITLE, &ctx->win, memview_draw_cb, memview_key_isr, ctx, out_slot);
}
