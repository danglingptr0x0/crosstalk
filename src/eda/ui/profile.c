#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

#include "hdl/abi.h"
#include "eda/ui/profile.h"
#include "ui/cell.h"
#include "ui/theme.h"

#define PROFILE_STEP 1
#define PROFILE_ORIG_COL 0
#define PROFILE_STRIP_ROW_CUNT 3
#define PROFILE_BAR_NONE 0
#define PROFILE_EVT_FIRST 0
#define PROFILE_TOP_ROWS 12
#define PROFILE_TEXT_CAP 192
#define PROFILE_BAR_COL 0xADF240FFU
#define PROFILE_BAR_DIM_COL 0x2E4A14FFU
#define PROFILE_COMB_COL 0x599EF2FFU
#define PROFILE_TEXT_COL 0xC8D2DCFF
#define PROFILE_DIM_COL 0x6E7A8AFF
#define PROFILE_LIT_COL 0xAEE79BFF
#define PROFILE_PCT_SCALE 100.0
#define PROFILE_ROW_LABEL_CAP 40
#define PROFILE_BTN_GAP 1
#define PROFILE_BTN_CUNT 4
#define PROFILE_EVT_CHIP_GAP 2
#define PROFILE_SUB_INDENT 2
#define PROFILE_KEY_START 's'
#define PROFILE_KEY_END 'e'
#define PROFILE_KEY_RST 'r'
#define PROFILE_KEY_LIVE 'l'
#define PROFILE_KEY_EVT_PREV '['
#define PROFILE_KEY_EVT_NEXT ']'
#define PROFILE_NO_DATA_TEXT "no profile data; emit and run a design"
#define PROFILE_SEL_FMT "cycle %" PRIu64 ": total %" PRIu64 " host cyc | comb %" PRIu64 " (%5.1F%%) | blocks %" PRIu64 " (%5.1F%%) | commit %" PRIu64 " (%5.1F%%)"
#define PROFILE_SEL_NONE_TEXT "click the timeline to inspect a simulated cycle"
#define PROFILE_AGG_HDR_LIVE_FMT "live totals over %" PRIu64 " cycles; host cyc per site; click a row for detail:"
#define PROFILE_AGG_HDR_EVT_FMT "capture %.23s over %" PRIu64 " cycles; host cyc per site; click a row for detail:"
#define PROFILE_AGG_ROW_FMT "%-20.20s %14" PRIu64 " %12" PRIu64 " x %5.1F%%"
#define PROFILE_DENSITY_FMT "design %" PRIu64 " ops; %.1F host cyc per sim cycle; %.3F evaluations per host cycle"
#define PROFILE_DENSITY_NONE_TEXT "evaluation density: needs a run; no host cycles recorded yet"
#define PROFILE_GATING_FMT "gating: %.1F of %" PRIu64 " regions per sim cycle; %.1F%% of the design touched"
#define PROFILE_GATING_NONE_TEXT "gating: no activity regions in this design; re-emit if this is unexpected"
#define PROFILE_BTN_START_TEXT "[start]"
#define PROFILE_BTN_END_TEXT "[end]"
#define PROFILE_BTN_RST_TEXT "[reset]"
#define PROFILE_BTN_LIVE_TEXT "[live]"
#define PROFILE_STATE_REC_FMT "recording: %.23s"
#define PROFILE_STATE_VIEW_FMT "viewing capture: %.23s"
#define PROFILE_STATE_LIVE_TEXT "live"
#define PROFILE_EVT_CHIP_FMT "%.10s %9" PRIu64
#define PROFILE_EVT_NAME_FMT "evt%u"
#define PROFILE_NAME_FMT "%.23s"
#define PROFILE_LBL_COMB_TEXT "comb"
#define PROFILE_LBL_CMT_TEXT "commit"
#define PROFILE_LBL_BLK_FMT "b%u %.31s"
#define PROFILE_SUB_ORDER_FMT "order stream %14" PRIu64 " %5.1F%% | ops %" PRIu64
#define PROFILE_SUB_HD_FMT "residue hea" "ds %14" PRIu64 " %5.1F%% | residue ops %" PRIu64
#define PROFILE_SUB_CORE_FMT "residue core %14" PRIu64 " %5.1F%% | passes avg %5.2F | core ops %" PRIu64
#define PROFILE_SUB_TAILS_FMT "residue tails %14" PRIu64 " %5.1F%%"
#define PROFILE_SUB_REGWR_FMT "reg writes %" PRIu64 " | avg %6.2F per commit | staged slots %" PRIu64
#define PROFILE_SUB_MEMWR_FMT "mem writes %" PRIu64
#define PROFILE_SUB_BLK_FMT "fires %" PRIu64 " | avg %" PRIu64 " host cyc per fire"
#define PROFILE_SUB_ORDER_AT 0
#define PROFILE_SUB_HD_AT 1
#define PROFILE_SUB_CORE_AT 2
#define PROFILE_SUB_PASS_AT 3
#define PROFILE_SUB_TAILS_AT 4
#define PROFILE_SUB_REGWR_AT 5
#define PROFILE_SUB_MEMWR_AT 6
#define PROFILE_SUB_RGN_AT 7
#define PROFILE_META_CYC_AT 1
#define PROFILE_META_RGN_AT 8
#define PROFILE_META_BLK_AT 2
#define PROFILE_META_STAGED_AT 4
#define PROFILE_META_ORDER_AT 5
#define PROFILE_META_RES_AT 6
#define PROFILE_META_CORE_AT 7
#define PROFILE_TAB_CUNT 3
#define PROFILE_TAB_SIM_TEXT "[sim]"
#define PROFILE_TAB_PROG_TEXT "[prog]"
#define PROFILE_TAB_EMU_TEXT "[emu]"
#define PROFILE_EMU_NONE_TEXT "no emulator counters; needs a plugin with prof en and kinds 16 to 19"
#define PROFILE_EMU_HOST_FMT "retired %" PRIu64 "; host %" PRIu64 " ms; %.2F MHz; %.1F host cyc per insn"
#define PROFILE_EMU_MEM_FMT "ram rd %" PRIu64 "; ram wr %" PRIu64 "; io rd %" PRIu64 "; io wr %" PRIu64
#define PROFILE_EMU_TRAP_HDR_TEXT "traps by cause:"
#define PROFILE_EMU_TRAP_FMT "%.15s %" PRIu64 " x"
#define PROFILE_EMU_TRAP_NONE_TEXT "no traps taken"
#define PROFILE_EMU_OP_HDR_TEXT "opcode histogram, hottest first:"
#define PROFILE_EMU_OP_FMT "opcode %02" PRIX32 "; %" PRIu64 " x; %.1F%%"
#define PROFILE_EMU_OP_ROWS 12
#define PROFILE_EMU_MS_PER_SEC 1000000.0
#define PROFILE_EMU_HZ_PER_MHZ 1000000.0
#define PROFILE_PROG_HASH_MUL 0x9E3779B97F4A7C15ULL
#define PROFILE_PROG_PROBE_MAX 8
#define PROFILE_PROG_ROUND_MAX 16
#define PROFILE_PROG_FRAME_SHIFT 12
#define PROFILE_PROG_TOP_ROWS 12
#define PROFILE_PROG_FRAME_ROWS 6
#define PROFILE_PROG_HDR_FMT "guest samples %" PRIu64 " over the program counter; 1 per 64 cycles; dropped %" PRIu64 "; reset clears"
#define PROFILE_PROG_ADDR_HDR_TEXT "hot addresses:"
#define PROFILE_PROG_FN_HDR_TEXT "hot functions (from the call frames):"
#define PROFILE_PROG_FN_NONE_TEXT "no functions found; needs symbols and a call frame in view"
#define PROFILE_PROG_FN_ROW_FMT "%08" PRIX64 ": %.63s; %" PRIu64 " x; %.1F%%"
#define PROFILE_PROG_FN_ROWS 12
#define PROFILE_PROG_FRAME_HDR_TEXT "hot frames (4K):"
#define PROFILE_PROG_ROW_FMT "%012" PRIX64 "; %" PRIu64 " x; %.1F%%"
#define PROFILE_PROG_ROW_SYM_FMT "%012" PRIX64 ": %s; %" PRIu64 " x; %.1F%%"
#define PROFILE_PROG_FRAME_FMT "%012" PRIX64 "; %" PRIu32 " x; %.1F%%"
#define PROFILE_PROG_FRAME_SYM_FMT "%012" PRIX64 ": %s; %" PRIu32 " x; %.1F%%"
#define PROFILE_PROG_NONE_TEXT "no guest samples; needs a running design with a loc signal"

static uint32_t profile_state_pull(crosstalk_eda_ui_profile_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, uint32_t kind, uint64_t *dst, uint32_t cap)
{
    uint64_t *base = 0x0;
    uint32_t cunt = 0;
    uint32_t ret = 0;

    ret = crosstalk_eda_ic_fpga_prof_get(fpga, slot_idx, kind, &base, &cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (cunt > cap) { cunt = cap; }

    memcpy(dst, base, (uint64_t)cunt * sizeof(uint64_t));

    return LDG_ERR_AOK;
}

static void profile_fn_tally(crosstalk_eda_ui_profile_ctx_t *ctx, uint64_t sym_addr)
{
    uint32_t at = 0;

    for (at = 0; at < ctx->fn_cunt; at++)
    {
        if (ctx->fn_addr[at] != sym_addr) { continue; }

        ctx->fn_hits[at] = ctx->fn_hits[at] + 0x1;
        ctx->fn_total = ctx->fn_total + 0x1;

        return;
    }

    if (ctx->fn_cunt >= (uint32_t)CROSSTALK_EDA_UI_PROFILE_FN_CAP) { return; }

    ctx->fn_addr[ctx->fn_cunt] = sym_addr;
    ctx->fn_hits[ctx->fn_cunt] = 0x1;
    ctx->fn_cunt = ctx->fn_cunt + 0x1;
    ctx->fn_total = ctx->fn_total + 0x1;
}

static LDG_NOINLINE uint32_t profile_fn_scan(crosstalk_eda_ui_profile_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx)
{
    uint64_t bytes[CROSSTALK_EDA_UI_PROFILE_FN_WORD_CUNT] = LDG_ARR_ZERO_INIT;
    const crosstalk_hdl_elf_sym_t *sym = 0x0;
    uint64_t base = 0;
    uint64_t off = 0;
    uint64_t val = 0;
    uint64_t best = 0;
    uint32_t best_at = UINT32_MAX;
    uint32_t rd = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || fpga == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->elf == 0x0 || crosstalk_hdl_elf_ld_is(ctx->elf) != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    for (at = 0; at < (uint32_t)CROSSTALK_EDA_UI_PROFILE_PROG_TOP_CAP; at++)
    {
        if (ctx->prog_hits[at] <= best) { continue; }

        best = ctx->prog_hits[at];
        best_at = at;
    }

    if (best_at == UINT32_MAX || best == 0) { return LDG_ERR_EMPTY; }

    base = ctx->prog_addr[best_at];
    if (base == 0) { return LDG_ERR_EMPTY; }

    if (crosstalk_eda_ic_fpga_mem_offset_get(fpga, slot_idx, CROSSTALK_EDA_UI_PROFILE_FN_MEM_SIG, &off) != LDG_ERR_AOK) { off = 0; }

    if (base < off) { return LDG_ERR_RANGE; }

    if (crosstalk_eda_ic_fpga_mem_byte_rd(fpga, slot_idx, CROSSTALK_EDA_UI_PROFILE_FN_MEM_SIG, base - off, bytes, (uint32_t)CROSSTALK_EDA_UI_PROFILE_FN_WORD_CUNT, &rd) != LDG_ERR_AOK) { return LDG_ERR_IO_RD; }

    if (rd < (uint32_t)sizeof(uint32_t)) { return LDG_ERR_EMPTY; }

    for (at = 0; at + (uint32_t)sizeof(uint32_t) <= rd; at++)
    {
        val = (bytes[at] & (uint64_t)LDG_BYTE_MASK) | ((bytes[at + 0x1] & (uint64_t)LDG_BYTE_MASK) << 8) | ((bytes[at + 0x2] & (uint64_t)LDG_BYTE_MASK) << 16) | ((bytes[at + 0x3] & (uint64_t)LDG_BYTE_MASK) << 24);

        if (crosstalk_hdl_elf_sym_get(ctx->elf, val, &sym, &off) != LDG_ERR_AOK) { continue; }

        if (sym->kind != (uint8_t)CROSSTALK_HDL_ELF_KIND_FUNC) { continue; }

        profile_fn_tally(ctx, sym->addr);
    }

    return LDG_ERR_AOK;
}

static uint64_t profile_delta_get(uint64_t now_v, uint64_t base_v)
{
    if (now_v < base_v) { return now_v; }

    return now_v - base_v;
}

uint32_t crosstalk_eda_ui_profile_init(crosstalk_eda_ui_profile_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->win.scroll_on = LDG_TRUTH_TRUE;
    ctx->agg_sel_slot = UINT32_MAX;
    ctx->strip_step = (uint64_t)PROFILE_STEP;
    ctx->sel_cyc = UINT64_MAX;
    ctx->view_evt = UINT32_MAX;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_profile_open_set(crosstalk_eda_ui_profile_ctx_t *ctx, uint8_t open)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->open = open;
    ctx->win.open = open;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_ui_profile_open_is(const crosstalk_eda_ui_profile_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->win.open != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_eda_ui_profile_mark_start(crosstalk_eda_ui_profile_ctx_t *ctx, const char *name)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (name == 0x0 || name[0] == LDG_STR_TERM) { snprintf(ctx->pending_name, sizeof(ctx->pending_name), PROFILE_EVT_NAME_FMT, ctx->evt_next); }
    else { snprintf(ctx->pending_name, sizeof(ctx->pending_name), PROFILE_NAME_FMT, name); }

    ctx->pending_start = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_profile_mark_end(crosstalk_eda_ui_profile_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->evt_run == 0 && ctx->pending_start == 0) { return LDG_ERR_EMPTY; }

    ctx->pending_end = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_profile_rst(crosstalk_eda_ui_profile_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->pending_rst = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static void profile_pending_apply(crosstalk_eda_ui_profile_ctx_t *ctx)
{
    uint32_t slot = 0;
    uint32_t at = 0;

    if (ctx->pending_rst != 0)
    {
        memcpy(ctx->base_t, ctx->agg_t, sizeof(ctx->base_t));
        memcpy(ctx->base_c, ctx->agg_c, sizeof(ctx->base_c));
        memcpy(ctx->base_sub, ctx->sub, sizeof(ctx->base_sub));
        ctx->base_cyc = ctx->meta[PROFILE_META_CYC_AT];
        memset(ctx->prog_addr, 0x0, sizeof(ctx->prog_addr));
        memset(ctx->prog_hits, 0x0, sizeof(ctx->prog_hits));
        memset(ctx->prog_frame, 0x0, sizeof(ctx->prog_frame));
        ctx->prog_total = 0;
        ctx->prog_drop = 0;
        ctx->pending_rst = LDG_TRUTH_FALSE;
    }

    if (ctx->pending_start != 0)
    {
        memcpy(ctx->snap_t, ctx->agg_t, sizeof(ctx->snap_t));
        memcpy(ctx->snap_c, ctx->agg_c, sizeof(ctx->snap_c));
        memcpy(ctx->snap_sub, ctx->sub, sizeof(ctx->snap_sub));
        ctx->snap_cyc = ctx->meta[PROFILE_META_CYC_AT];
        memcpy(ctx->snap_name, ctx->pending_name, sizeof(ctx->snap_name));
        ctx->evt_run = LDG_TRUTH_TRUE;
        ctx->pending_start = LDG_TRUTH_FALSE;
    }

    if (ctx->pending_end != 0)
    {
        if (ctx->evt_run != 0)
        {
            crosstalk_eda_ui_profile_evt_t *evt = &ctx->evt[ctx->evt_next];

            for (slot = 0; slot < (uint32_t)CROSSTALK_EDA_UI_PROFILE_SLOT_CAP; slot++)
            {
                evt->agg_t[slot] = profile_delta_get(ctx->agg_t[slot], ctx->snap_t[slot]);
                evt->agg_c[slot] = profile_delta_get(ctx->agg_c[slot], ctx->snap_c[slot]);
            }

            for (at = 0; at < (uint32_t)CROSSTALK_EDA_UI_PROFILE_SUB_CAP; at++) { evt->sub[at] = profile_delta_get(ctx->sub[at], ctx->snap_sub[at]); }

            evt->cyc = profile_delta_get(ctx->meta[PROFILE_META_CYC_AT], ctx->snap_cyc);
            memcpy(evt->name, ctx->snap_name, sizeof(evt->name));
            evt->used = LDG_TRUTH_TRUE;
            ctx->view_evt = ctx->evt_next;
            ctx->evt_next = (ctx->evt_next + 0x1) % (uint32_t)CROSSTALK_EDA_UI_PROFILE_EVT_CAP;
            ctx->evt_run = LDG_TRUTH_FALSE;
        }

        ctx->pending_end = LDG_TRUTH_FALSE;
    }
}

static void profile_prog_pull(crosstalk_eda_ui_profile_ctx_t *ctx, crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx)
{
    uint64_t prog_buff[CROSSTALK_EDA_UI_PROFILE_PROG_DRAIN_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t got = 0;
    uint32_t at = 0;
    uint32_t round = 0;

    for (round = 0; round < (uint32_t)PROFILE_PROG_ROUND_MAX; round++)
    {
        if (crosstalk_eda_ic_fpga_prog_drain(fpga, slot_idx, prog_buff, (uint32_t)CROSSTALK_EDA_UI_PROFILE_PROG_DRAIN_CAP, &got) != LDG_ERR_AOK) { return; }

        if (got == 0) { return; }

        for (at = 0; at < got; at++)
        {
            uint64_t addr = prog_buff[at];
            uint64_t frame = addr >> PROFILE_PROG_FRAME_SHIFT;
            uint32_t hash = (uint32_t)((addr * (uint64_t)PROFILE_PROG_HASH_MUL) >> 32) & ((uint32_t)CROSSTALK_EDA_UI_PROFILE_PROG_TOP_CAP - 0x1);
            uint32_t probe = 0;

            ctx->prog_total = ctx->prog_total + 0x1;

            if (frame < (uint64_t)CROSSTALK_EDA_UI_PROFILE_PROG_FRAME_CAP) { ctx->prog_frame[frame] = ctx->prog_frame[frame] + 0x1; }

            for (probe = 0; probe < (uint32_t)PROFILE_PROG_PROBE_MAX; probe++)
            {
                uint32_t slot = (hash + probe) & ((uint32_t)CROSSTALK_EDA_UI_PROFILE_PROG_TOP_CAP - 0x1);

                if (ctx->prog_hits[slot] == 0) { ctx->prog_addr[slot] = addr; ctx->prog_hits[slot] = 0x1; break; }

                if (ctx->prog_addr[slot] == addr) { ctx->prog_hits[slot] = ctx->prog_hits[slot] + 0x1; break; }
            }

            if (probe >= (uint32_t)PROFILE_PROG_PROBE_MAX) { ctx->prog_drop = ctx->prog_drop + 0x1; }
        }
    }
}

static void profile_emu_pull(crosstalk_eda_ui_profile_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx)
{
    ctx->emu_bound = LDG_TRUTH_FALSE;

    if (crosstalk_eda_ic_fpga_emu_state_get(fpga, slot_idx, (uint32_t)CROSSTALK_HDL_ABI_STATE_EMU_HOST, ctx->emu_host, (uint32_t)CROSSTALK_HDL_ABI_HOST_CUNT) != LDG_ERR_AOK) { return; }

    if (crosstalk_eda_ic_fpga_emu_state_get(fpga, slot_idx, (uint32_t)CROSSTALK_HDL_ABI_STATE_EMU_OP, ctx->emu_op, (uint32_t)CROSSTALK_HDL_ABI_OP_CUNT) != LDG_ERR_AOK) { return; }

    if (crosstalk_eda_ic_fpga_emu_state_get(fpga, slot_idx, (uint32_t)CROSSTALK_HDL_ABI_STATE_EMU_MEM, ctx->emu_mem, (uint32_t)CROSSTALK_HDL_ABI_MEM_CUNT) != LDG_ERR_AOK) { return; }

    if (crosstalk_eda_ic_fpga_emu_state_get(fpga, slot_idx, (uint32_t)CROSSTALK_HDL_ABI_STATE_EMU_TRAP, ctx->emu_trap, (uint32_t)CROSSTALK_HDL_ABI_TRAP_CUNT) != LDG_ERR_AOK) { return; }

    ctx->emu_bound = LDG_TRUTH_TRUE;
}

uint32_t crosstalk_eda_ui_profile_tick(crosstalk_eda_ui_profile_ctx_t *ctx, crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, double now)
{
    uint32_t slot = 0;
    uint8_t pending_has = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || fpga == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    pending_has = ctx->pending_start != 0 || ctx->pending_end != 0 || ctx->pending_rst != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (ctx->open == 0 && pending_has == LDG_TRUTH_FALSE) { return LDG_ERR_AOK; }

    if (now < ctx->refresh_next && pending_has == LDG_TRUTH_FALSE) { return LDG_ERR_AOK; }

    ctx->refresh_next = now + CROSSTALK_EDA_UI_PROFILE_REFRESH_SECS;

    profile_prog_pull(ctx, fpga, slot_idx);

    if (profile_fn_scan(ctx, fpga, slot_idx) != LDG_ERR_AOK) { ctx->fn_total = ctx->fn_total; }

    profile_emu_pull(ctx, fpga, slot_idx);

    ret = profile_state_pull(ctx, fpga, slot_idx, (uint32_t)CROSSTALK_HDL_ABI_STATE_PF_META, ctx->meta, (uint32_t)CROSSTALK_EDA_UI_PROFILE_META_CAP);
    if (ret != LDG_ERR_AOK) { ctx->bound = LDG_TRUTH_FALSE; return LDG_ERR_AOK; }

    ctx->blk_cunt = (uint32_t)ctx->meta[PROFILE_META_BLK_AT];
    ctx->slot_cunt = ctx->blk_cunt + 0x2;
    if (ctx->slot_cunt > (uint32_t)CROSSTALK_EDA_UI_PROFILE_SLOT_CAP) { ctx->slot_cunt = (uint32_t)CROSSTALK_EDA_UI_PROFILE_SLOT_CAP; }

    ret = profile_state_pull(ctx, fpga, slot_idx, (uint32_t)CROSSTALK_HDL_ABI_STATE_PF_T, ctx->agg_t, (uint32_t)CROSSTALK_EDA_UI_PROFILE_SLOT_CAP);
    if (ret != LDG_ERR_AOK) { ctx->bound = LDG_TRUTH_FALSE; return LDG_ERR_AOK; }

    ret = profile_state_pull(ctx, fpga, slot_idx, (uint32_t)CROSSTALK_HDL_ABI_STATE_PF_C, ctx->agg_c, (uint32_t)CROSSTALK_EDA_UI_PROFILE_SLOT_CAP);
    if (ret != LDG_ERR_AOK) { ctx->bound = LDG_TRUTH_FALSE; return LDG_ERR_AOK; }

    ret = profile_state_pull(ctx, fpga, slot_idx, (uint32_t)CROSSTALK_HDL_ABI_STATE_PF_SUB, ctx->sub, (uint32_t)CROSSTALK_EDA_UI_PROFILE_SUB_CAP);
    if (ret != LDG_ERR_AOK) { ctx->bound = LDG_TRUTH_FALSE; return LDG_ERR_AOK; }

    ret = profile_state_pull(ctx, fpga, slot_idx, (uint32_t)CROSSTALK_HDL_ABI_STATE_PF_NM, (uint64_t *)(void *)ctx->nm, (uint32_t)CROSSTALK_EDA_UI_PROFILE_SLOT_CAP * (uint32_t)CROSSTALK_EDA_UI_PROFILE_NM_BYTES / (uint32_t)sizeof(uint64_t));
    if (ret != LDG_ERR_AOK) { ctx->bound = LDG_TRUTH_FALSE; return LDG_ERR_AOK; }

    for (slot = 0; slot < (uint32_t)CROSSTALK_EDA_UI_PROFILE_SLOT_CAP; slot++) { ctx->nm[slot][CROSSTALK_EDA_UI_PROFILE_NM_BYTES - 0x1] = LDG_STR_TERM; }

    ret = profile_state_pull(ctx, fpga, slot_idx, (uint32_t)CROSSTALK_HDL_ABI_STATE_PF_RING_TOT, ctx->ring_tot, (uint32_t)CROSSTALK_EDA_UI_PROFILE_RING_CAP);
    if (ret != LDG_ERR_AOK) { ctx->bound = LDG_TRUTH_FALSE; return LDG_ERR_AOK; }

    ret = profile_state_pull(ctx, fpga, slot_idx, (uint32_t)CROSSTALK_HDL_ABI_STATE_PF_RING_COMB, ctx->ring_comb, (uint32_t)CROSSTALK_EDA_UI_PROFILE_RING_CAP);
    if (ret != LDG_ERR_AOK) { ctx->bound = LDG_TRUTH_FALSE; return LDG_ERR_AOK; }

    ret = profile_state_pull(ctx, fpga, slot_idx, (uint32_t)CROSSTALK_HDL_ABI_STATE_PF_RING_BLK, ctx->ring_blk, (uint32_t)CROSSTALK_EDA_UI_PROFILE_RING_CAP);
    if (ret != LDG_ERR_AOK) { ctx->bound = LDG_TRUTH_FALSE; return LDG_ERR_AOK; }

    ret = profile_state_pull(ctx, fpga, slot_idx, (uint32_t)CROSSTALK_HDL_ABI_STATE_PF_RING_CMT, ctx->ring_cmt, (uint32_t)CROSSTALK_EDA_UI_PROFILE_RING_CAP);
    if (ret != LDG_ERR_AOK) { ctx->bound = LDG_TRUTH_FALSE; return LDG_ERR_AOK; }

    ctx->bound = LDG_TRUTH_TRUE;

    profile_pending_apply(ctx);

    return LDG_ERR_AOK;
}

static double profile_pct_get(uint64_t part, uint64_t total)
{
    if (total == 0) { return 0.0; }

    return ((double)part / (double)total) * PROFILE_PCT_SCALE;
}

static void profile_view_get(const crosstalk_eda_ui_profile_ctx_t *ctx, uint64_t *out_t, uint64_t *out_c, uint64_t *out_sub, uint64_t *out_cyc, const char **out_name)
{
    uint32_t slot = 0;
    uint32_t at = 0;

    *out_cyc = 0;
    *out_name = 0x0;

    if (ctx->view_evt != UINT32_MAX && ctx->evt[ctx->view_evt].used != 0)
    {
        const crosstalk_eda_ui_profile_evt_t *evt = &ctx->evt[ctx->view_evt];

        memcpy(out_t, evt->agg_t, (uint64_t)CROSSTALK_EDA_UI_PROFILE_SLOT_CAP * sizeof(uint64_t));
        memcpy(out_c, evt->agg_c, (uint64_t)CROSSTALK_EDA_UI_PROFILE_SLOT_CAP * sizeof(uint64_t));
        memcpy(out_sub, evt->sub, (uint64_t)CROSSTALK_EDA_UI_PROFILE_SUB_CAP * sizeof(uint64_t));
        *out_cyc = evt->cyc;
        *out_name = evt->name;

        return;
    }

    for (slot = 0; slot < (uint32_t)CROSSTALK_EDA_UI_PROFILE_SLOT_CAP; slot++)
    {
        out_t[slot] = profile_delta_get(ctx->agg_t[slot], ctx->base_t[slot]);
        out_c[slot] = profile_delta_get(ctx->agg_c[slot], ctx->base_c[slot]);
    }

    for (at = 0; at < (uint32_t)CROSSTALK_EDA_UI_PROFILE_SUB_CAP; at++) { out_sub[at] = profile_delta_get(ctx->sub[at], ctx->base_sub[at]); }

    *out_cyc = profile_delta_get(ctx->meta[PROFILE_META_CYC_AT], ctx->base_cyc);
}

static uint32_t profile_line_put(ldg_term_render_t *render, uint32_t col, uint32_t *row, const char *text, uint32_t rgba)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || row == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_ui_cell_rgba_put(render, col, *row, text, (uint64_t)PROFILE_TEXT_CAP, rgba, 0x0);
    if (ret != LDG_ERR_AOK) { return ret; }

    *row = *row + (uint32_t)PROFILE_STEP;

    return LDG_ERR_AOK;
}

static uint32_t profile_bar_row_put(ldg_term_render_t *render, uint32_t row, const char *text, uint32_t bar_cells, uint32_t fg_rgba, uint32_t bar_rgba)
{
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t bar = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t pane = LDG_STRUCT_ZERO_INIT;
    uint64_t len = 0;
    uint64_t lead = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_ui_cell_color_get(fg_rgba, &fg);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get(bar_rgba, &bar); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_CELL_PANE_RGBA, &pane); }

    if (ret == LDG_ERR_AOK) { ret = ldg_str_len_get(text, (uint64_t)PROFILE_TEXT_CAP, &len); }

    if (ret == LDG_ERR_AOK && bar_cells != 0) { ret = crosstalk_ui_cell_span_fill(render, (uint32_t)PROFILE_ORIG_COL, row, bar_cells, &fg, &bar); }

    if (ret != LDG_ERR_AOK) { return ret; }

    lead = len < (uint64_t)bar_cells ? len : (uint64_t)bar_cells;

    if (lead != 0)
    {
        ret = ldg_term_render_text_draw(render, (uint32_t)PROFILE_ORIG_COL, row, text, lead, &fg, &bar);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    if (len <= lead) { return LDG_ERR_AOK; }

    return ldg_term_render_text_draw(render, (uint32_t)lead, row, text + lead, len - lead, &fg, &pane);
}

static LDG_NOINLINE uint32_t profile_sub_rows_draw(crosstalk_eda_ui_profile_ctx_t *ctx, ldg_term_render_t *render, uint32_t slot, const uint64_t *eff_t, const uint64_t *eff_c, const uint64_t *eff_sub, uint32_t *row)
{
    char text[PROFILE_TEXT_CAP] = LDG_ARR_ZERO_INIT;
    double avg = 0.0;
    uint64_t fires = 0;
    uint64_t fire_avg = 0;
    uint32_t line = 0;
    uint32_t line_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0 || row == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (slot == ctx->blk_cunt) { line_cunt = 0x4; }
    else if (slot == ctx->blk_cunt + 0x1) { line_cunt = 0x2; }
    else { line_cunt = 0x1; }

    for (line = 0; line < line_cunt; line++)
    {
        if (slot == ctx->blk_cunt) { if (line == 0) { snprintf(text, sizeof(text), PROFILE_SUB_ORDER_FMT, eff_sub[PROFILE_SUB_ORDER_AT], profile_pct_get(eff_sub[PROFILE_SUB_ORDER_AT], eff_t[slot]), ctx->meta[PROFILE_META_ORDER_AT]); }
            else if (line == 0x1) { snprintf(text, sizeof(text), PROFILE_SUB_HD_FMT, eff_sub[PROFILE_SUB_HD_AT], profile_pct_get(eff_sub[PROFILE_SUB_HD_AT], eff_t[slot]), ctx->meta[PROFILE_META_RES_AT]); }
            else if (line == 0x2)
            {
                avg = eff_c[slot] == 0 ? 0.0 : (double)eff_sub[PROFILE_SUB_PASS_AT] / (double)eff_c[slot];
                snprintf(text, sizeof(text), PROFILE_SUB_CORE_FMT, eff_sub[PROFILE_SUB_CORE_AT], profile_pct_get(eff_sub[PROFILE_SUB_CORE_AT], eff_t[slot]), avg, ctx->meta[PROFILE_META_CORE_AT]);
            }
            else { snprintf(text, sizeof(text), PROFILE_SUB_TAILS_FMT, eff_sub[PROFILE_SUB_TAILS_AT], profile_pct_get(eff_sub[PROFILE_SUB_TAILS_AT], eff_t[slot])); } }
        else if (slot == ctx->blk_cunt + 0x1) { if (line == 0)
            {
                avg = eff_c[slot] == 0 ? 0.0 : (double)eff_sub[PROFILE_SUB_REGWR_AT] / (double)eff_c[slot];
                snprintf(text, sizeof(text), PROFILE_SUB_REGWR_FMT, eff_sub[PROFILE_SUB_REGWR_AT], avg, ctx->meta[PROFILE_META_STAGED_AT]);
            }
            else { snprintf(text, sizeof(text), PROFILE_SUB_MEMWR_FMT, eff_sub[PROFILE_SUB_MEMWR_AT]); } }
        else
        {
            fires = eff_c[slot];
            fire_avg = 0;

            if (fires > 0) { fire_avg = eff_t[slot] / fires; }

            snprintf(text, sizeof(text), PROFILE_SUB_BLK_FMT, fires, fire_avg);
        }

        ret = profile_line_put(render, (uint32_t)PROFILE_SUB_INDENT, row, text, (uint32_t)PROFILE_DIM_COL);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static LDG_NOINLINE uint32_t profile_agg_draw(crosstalk_eda_ui_profile_ctx_t *ctx, ldg_term_render_t *render, uint32_t *row_at, uint32_t col_cunt)
{
    char text[PROFILE_TEXT_CAP] = LDG_ARR_ZERO_INIT;
    char label[PROFILE_ROW_LABEL_CAP] = LDG_ARR_ZERO_INIT;
    uint8_t used[CROSSTALK_EDA_UI_PROFILE_SLOT_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t eff_t[CROSSTALK_EDA_UI_PROFILE_SLOT_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t eff_c[CROSSTALK_EDA_UI_PROFILE_SLOT_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t eff_sub[CROSSTALK_EDA_UI_PROFILE_SUB_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t eff_cyc = 0;
    const char *view_name = 0x0;
    uint64_t grand = 0;
    uint64_t ops = 0;
    double host_per_cyc = 0.0;
    double evals_per_host = 0.0;
    double rgn_per_cyc = 0.0;
    uint32_t row = 0;
    uint32_t slot = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0 || row_at == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    profile_view_get(ctx, eff_t, eff_c, eff_sub, &eff_cyc, &view_name);

    for (slot = 0; slot < ctx->slot_cunt; slot++) { grand = grand + eff_t[slot]; }

    if (view_name != 0x0) { snprintf(text, sizeof(text), PROFILE_AGG_HDR_EVT_FMT, view_name, eff_cyc); }
    else { snprintf(text, sizeof(text), PROFILE_AGG_HDR_LIVE_FMT, eff_cyc); }

    ret = profile_line_put(render, (uint32_t)PROFILE_ORIG_COL, row_at, text, (uint32_t)PROFILE_DIM_COL);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ops = ctx->meta[PROFILE_META_ORDER_AT] + ctx->meta[PROFILE_META_RES_AT];

    if (eff_cyc > 0 && grand > 0)
    {
        host_per_cyc = (double)grand / (double)eff_cyc;
        evals_per_host = (double)ops / host_per_cyc;
        snprintf(text, sizeof(text), PROFILE_DENSITY_FMT, ops, host_per_cyc, evals_per_host);
    }
    else { snprintf(text, sizeof(text), PROFILE_DENSITY_NONE_TEXT); }

    ret = profile_line_put(render, (uint32_t)PROFILE_ORIG_COL, row_at, text, (uint32_t)PROFILE_LIT_COL);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (eff_cyc > 0 && ctx->meta[PROFILE_META_RGN_AT] > 0)
    {
        rgn_per_cyc = (double)eff_sub[PROFILE_SUB_RGN_AT] / (double)eff_cyc;
        snprintf(text, sizeof(text), PROFILE_GATING_FMT, rgn_per_cyc, ctx->meta[PROFILE_META_RGN_AT], profile_pct_get(eff_sub[PROFILE_SUB_RGN_AT], eff_cyc * ctx->meta[PROFILE_META_RGN_AT]));
    }
    else { snprintf(text, sizeof(text), PROFILE_GATING_NONE_TEXT); }

    ret = profile_line_put(render, (uint32_t)PROFILE_ORIG_COL, row_at, text, (uint32_t)PROFILE_LIT_COL);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->agg_sel_slot = UINT32_MAX;

    for (row = 0; row < (uint32_t)PROFILE_TOP_ROWS; row++)
    {
        uint64_t best = 0;
        uint32_t best_slot = UINT32_MAX;
        uint32_t bar_cells = 0;
        double share = 0.0;

        for (slot = 0; slot < ctx->slot_cunt; slot++)
        {
            if (used[slot] != 0) { continue; }

            if (eff_t[slot] >= best) { best = eff_t[slot]; best_slot = slot; }
        }

        if (best_slot == UINT32_MAX || best == 0) { break; }

        used[best_slot] = LDG_TRUTH_TRUE;

        if (best_slot == ctx->blk_cunt) { snprintf(label, sizeof(label), PROFILE_LBL_COMB_TEXT); }
        else if (best_slot == ctx->blk_cunt + 0x1) { snprintf(label, sizeof(label), PROFILE_LBL_CMT_TEXT); }
        else { snprintf(label, sizeof(label), PROFILE_LBL_BLK_FMT, best_slot, ctx->nm[best_slot]); }

        share = profile_pct_get(best, grand);
        bar_cells = (uint32_t)(((double)col_cunt * share) / PROFILE_PCT_SCALE);

        if (row == ctx->agg_sel) { ctx->agg_sel_slot = best_slot; }

        snprintf(text, sizeof(text), PROFILE_AGG_ROW_FMT, label, eff_t[best_slot], eff_c[best_slot], share);

        ret = profile_bar_row_put(render, *row_at, text, bar_cells, row == ctx->agg_sel ? (uint32_t)CROSSTALK_UI_THEME_WHITE_RGBA : (uint32_t)PROFILE_TEXT_COL, (uint32_t)PROFILE_BAR_DIM_COL);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        *row_at = *row_at + (uint32_t)PROFILE_STEP;

        if (ctx->expand[best_slot] != 0)
        {
            ret = profile_sub_rows_draw(ctx, render, best_slot, eff_t, eff_c, eff_sub, row_at);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }

    return LDG_ERR_AOK;
}

static LDG_NOINLINE uint32_t profile_controls_draw(crosstalk_eda_ui_profile_ctx_t *ctx, ldg_term_render_t *render, uint32_t row)
{
    char text[PROFILE_TEXT_CAP] = LDG_ARR_ZERO_INIT;
    const char *btn[PROFILE_BTN_CUNT] = { PROFILE_BTN_START_TEXT, PROFILE_BTN_END_TEXT, PROFILE_BTN_RST_TEXT, PROFILE_BTN_LIVE_TEXT };
    uint32_t pos_col = PROFILE_ORIG_COL;
    uint32_t seg_w = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (at = 0; at < (uint32_t)PROFILE_BTN_CUNT; at++)
    {
        ret = crosstalk_ui_cell_rgba_put(render, pos_col, row, btn[at], (uint64_t)PROFILE_TEXT_CAP, (uint32_t)PROFILE_LIT_COL, &seg_w);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        pos_col = pos_col + seg_w + (uint32_t)PROFILE_BTN_GAP;
    }

    if (ctx->evt_run != 0) { snprintf(text, sizeof(text), PROFILE_STATE_REC_FMT, ctx->snap_name); }
    else if (ctx->view_evt != UINT32_MAX && ctx->evt[ctx->view_evt].used != 0) { snprintf(text, sizeof(text), PROFILE_STATE_VIEW_FMT, ctx->evt[ctx->view_evt].name); }
    else { snprintf(text, sizeof(text), PROFILE_STATE_LIVE_TEXT); }

    return crosstalk_ui_cell_rgba_put(render, pos_col, row, text, (uint64_t)sizeof(text), (uint32_t)PROFILE_TEXT_COL, 0x0);
}

static LDG_NOINLINE uint32_t profile_evt_row_draw(crosstalk_eda_ui_profile_ctx_t *ctx, ldg_term_render_t *render, uint32_t row)
{
    char text[PROFILE_TEXT_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t pos_col = PROFILE_ORIG_COL;
    uint32_t seg_w = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (at = 0; at < (uint32_t)CROSSTALK_EDA_UI_PROFILE_EVT_CAP; at++)
    {
        if (ctx->evt[at].used == 0) { continue; }

        snprintf(text, sizeof(text), PROFILE_EVT_CHIP_FMT, ctx->evt[at].name, ctx->evt[at].cyc);

        ret = crosstalk_ui_cell_rgba_put(render, pos_col, row, text, (uint64_t)sizeof(text), at == ctx->view_evt ? (uint32_t)PROFILE_LIT_COL : (uint32_t)PROFILE_DIM_COL, &seg_w);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        pos_col = pos_col + seg_w + (uint32_t)PROFILE_EVT_CHIP_GAP;
    }

    return LDG_ERR_AOK;
}

static LDG_NOINLINE uint32_t profile_tab_draw(crosstalk_eda_ui_profile_ctx_t *ctx, ldg_term_render_t *render, uint32_t row)
{
    const char *tab_text[PROFILE_TAB_CUNT] = { PROFILE_TAB_SIM_TEXT, PROFILE_TAB_PROG_TEXT, PROFILE_TAB_EMU_TEXT };
    uint32_t pos_col = PROFILE_ORIG_COL;
    uint32_t seg_w = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (at = 0; at < (uint32_t)PROFILE_TAB_CUNT; at++)
    {
        ret = crosstalk_ui_cell_rgba_put(render, pos_col, row, tab_text[at], (uint64_t)PROFILE_TEXT_CAP, ctx->tab == (uint8_t)at ? (uint32_t)PROFILE_LIT_COL : (uint32_t)PROFILE_DIM_COL, &seg_w);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        pos_col = pos_col + seg_w + (uint32_t)PROFILE_BTN_GAP;
    }

    return LDG_ERR_AOK;
}

static LDG_NOINLINE uint32_t profile_emu_draw(crosstalk_eda_ui_profile_ctx_t *ctx, ldg_term_render_t *render, uint32_t *row_at)
{
    static const char *trap_text[CROSSTALK_HDL_ABI_TRAP_CUNT] = { "invalid op", "unaligned", "stack over", "stack under", "divide by zero", "dr14 write", "TRAP6", "TRAP7" };
    char text[PROFILE_TEXT_CAP] = LDG_ARR_ZERO_INIT;
    uint8_t used[CROSSTALK_HDL_ABI_OP_CUNT] = LDG_ARR_ZERO_INIT;
    uint64_t total = 0;
    uint64_t traps = 0;
    double mhz = 0.0;
    double cyc_per = 0.0;
    uint32_t row = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0 || row_at == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->emu_bound != LDG_TRUTH_TRUE) { return profile_line_put(render, (uint32_t)PROFILE_ORIG_COL, row_at, PROFILE_EMU_NONE_TEXT, (uint32_t)PROFILE_DIM_COL); }

    if (ctx->emu_host[CROSSTALK_HDL_ABI_HOST_NS_AT] != 0) { mhz = (double)ctx->emu_host[CROSSTALK_HDL_ABI_HOST_RETIRED_AT] * (double)LDG_NS_PER_SEC / (double)ctx->emu_host[CROSSTALK_HDL_ABI_HOST_NS_AT] / PROFILE_EMU_HZ_PER_MHZ; }

    if (ctx->emu_host[CROSSTALK_HDL_ABI_HOST_RETIRED_AT] != 0) { cyc_per = (double)ctx->emu_host[CROSSTALK_HDL_ABI_HOST_CYCLES_AT] / (double)ctx->emu_host[CROSSTALK_HDL_ABI_HOST_RETIRED_AT]; }

    if (snprintf(text, sizeof(text), PROFILE_EMU_HOST_FMT, ctx->emu_host[CROSSTALK_HDL_ABI_HOST_RETIRED_AT], ctx->emu_host[CROSSTALK_HDL_ABI_HOST_NS_AT] / (uint64_t)LDG_NS_PER_MS, mhz, cyc_per) < 0) { return LDG_ERR_STR_FMT; }

    ret = profile_line_put(render, (uint32_t)PROFILE_ORIG_COL, row_at, text, (uint32_t)PROFILE_LIT_COL);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (snprintf(text, sizeof(text), PROFILE_EMU_MEM_FMT, ctx->emu_mem[CROSSTALK_HDL_ABI_MEM_RAM_RD_AT], ctx->emu_mem[CROSSTALK_HDL_ABI_MEM_RAM_WR_AT], ctx->emu_mem[CROSSTALK_HDL_ABI_MEM_MMIO_RD_AT], ctx->emu_mem[CROSSTALK_HDL_ABI_MEM_MMIO_WR_AT]) < 0) { return LDG_ERR_STR_FMT; }

    ret = profile_line_put(render, (uint32_t)PROFILE_ORIG_COL, row_at, text, (uint32_t)PROFILE_TEXT_COL);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *row_at = *row_at + (uint32_t)PROFILE_STEP;

    ret = profile_line_put(render, (uint32_t)PROFILE_ORIG_COL, row_at, PROFILE_EMU_TRAP_HDR_TEXT, (uint32_t)PROFILE_DIM_COL);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (at = 0; at < (uint32_t)CROSSTALK_HDL_ABI_TRAP_CUNT; at = at + 0x1)
    {
        if (ctx->emu_trap[at] == 0) { continue; }

        traps = traps + ctx->emu_trap[at];

        if (snprintf(text, sizeof(text), PROFILE_EMU_TRAP_FMT, trap_text[at], ctx->emu_trap[at]) < 0) { return LDG_ERR_STR_FMT; }

        ret = profile_line_put(render, (uint32_t)PROFILE_SUB_INDENT, row_at, text, (uint32_t)PROFILE_LIT_COL);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (traps == 0)
    {
        ret = profile_line_put(render, (uint32_t)PROFILE_SUB_INDENT, row_at, PROFILE_EMU_TRAP_NONE_TEXT, (uint32_t)PROFILE_DIM_COL);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    *row_at = *row_at + (uint32_t)PROFILE_STEP;

    ret = profile_line_put(render, (uint32_t)PROFILE_ORIG_COL, row_at, PROFILE_EMU_OP_HDR_TEXT, (uint32_t)PROFILE_DIM_COL);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (at = 0; at < (uint32_t)CROSSTALK_HDL_ABI_OP_CUNT; at = at + 0x1) { total = total + ctx->emu_op[at]; }

    for (row = 0; row < (uint32_t)PROFILE_EMU_OP_ROWS; row = row + 0x1)
    {
        uint64_t best = 0;
        uint32_t best_at = UINT32_MAX;

        for (at = 0; at < (uint32_t)CROSSTALK_HDL_ABI_OP_CUNT; at = at + 0x1)
        {
            if (used[at] != 0 || ctx->emu_op[at] <= best) { continue; }

            best = ctx->emu_op[at];
            best_at = at;
        }

        if (best_at == UINT32_MAX || best == 0) { break; }

        used[best_at] = LDG_TRUTH_TRUE;

        if (snprintf(text, sizeof(text), PROFILE_EMU_OP_FMT, best_at, best, profile_pct_get(best, total)) < 0) { return LDG_ERR_STR_FMT; }

        ret = profile_line_put(render, (uint32_t)PROFILE_SUB_INDENT, row_at, text, (uint32_t)PROFILE_TEXT_COL);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static LDG_NOINLINE uint32_t profile_prog_draw(crosstalk_eda_ui_profile_ctx_t *ctx, ldg_term_render_t *render, uint32_t *row_at, uint32_t col_cunt)
{
    char text[PROFILE_TEXT_CAP] = LDG_ARR_ZERO_INIT;
    uint8_t used[CROSSTALK_EDA_UI_PROFILE_PROG_TOP_CAP] = LDG_ARR_ZERO_INIT;
    uint8_t fn_used[CROSSTALK_EDA_UI_PROFILE_FN_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t frame_used[PROFILE_PROG_FRAME_ROWS] = LDG_ARR_ZERO_INIT;
    uint32_t frame_used_cunt = 0;
    uint32_t row = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0 || row_at == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->prog_total == 0) { return profile_line_put(render, (uint32_t)PROFILE_ORIG_COL, row_at, PROFILE_PROG_NONE_TEXT, (uint32_t)PROFILE_DIM_COL); }

    snprintf(text, sizeof(text), PROFILE_PROG_HDR_FMT, ctx->prog_total, ctx->prog_drop);

    ret = profile_line_put(render, (uint32_t)PROFILE_ORIG_COL, row_at, text, (uint32_t)PROFILE_DIM_COL);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = profile_line_put(render, (uint32_t)PROFILE_ORIG_COL, row_at, PROFILE_PROG_FN_HDR_TEXT, (uint32_t)PROFILE_DIM_COL);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->fn_cunt == 0)
    {
        ret = profile_line_put(render, (uint32_t)PROFILE_SUB_INDENT, row_at, PROFILE_PROG_FN_NONE_TEXT, (uint32_t)PROFILE_DIM_COL);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (row = 0; row < (uint32_t)PROFILE_PROG_FN_ROWS && ctx->fn_cunt > 0; row++)
    {
        char label[CROSSTALK_HDL_ELF_LABEL_CAP] = LDG_ARR_ZERO_INIT;
        uint64_t best = 0;
        uint32_t best_at = UINT32_MAX;
        double share = 0.0;

        for (at = 0; at < ctx->fn_cunt; at++)
        {
            if (fn_used[at] != 0 || ctx->fn_hits[at] == 0) { continue; }

            if (ctx->fn_hits[at] >= best) { best = ctx->fn_hits[at]; best_at = at; }
        }

        if (best_at == UINT32_MAX || best == 0) { break; }

        fn_used[best_at] = LDG_TRUTH_TRUE;
        share = ctx->fn_total == 0 ? 0.0 : ((double)best / (double)ctx->fn_total) * PROFILE_PCT_SCALE;

        if (ctx->elf == 0x0 || crosstalk_hdl_elf_label_get(ctx->elf, ctx->fn_addr[best_at], label, (uint32_t)sizeof(label)) != LDG_ERR_AOK) { continue; }

        snprintf(text, sizeof(text), PROFILE_PROG_FN_ROW_FMT, ctx->fn_addr[best_at], label, best, share);

        ret = profile_line_put(render, (uint32_t)PROFILE_SUB_INDENT, row_at, text, (uint32_t)PROFILE_TEXT_COL);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = profile_line_put(render, (uint32_t)PROFILE_ORIG_COL, row_at, PROFILE_PROG_ADDR_HDR_TEXT, (uint32_t)PROFILE_DIM_COL);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (row = 0; row < (uint32_t)PROFILE_PROG_TOP_ROWS; row++)
    {
        char label[CROSSTALK_HDL_ELF_LABEL_CAP] = LDG_ARR_ZERO_INIT;
        uint64_t best = 0;
        uint32_t best_at = UINT32_MAX;
        uint32_t bar_cells = 0;
        double share = 0.0;
        uint8_t label_ok = LDG_TRUTH_FALSE;

        for (at = 0; at < (uint32_t)CROSSTALK_EDA_UI_PROFILE_PROG_TOP_CAP; at++)
        {
            if (used[at] != 0 || ctx->prog_hits[at] == 0) { continue; }

            if (ctx->prog_hits[at] >= best) { best = ctx->prog_hits[at]; best_at = at; }
        }

        if (best_at == UINT32_MAX || best == 0) { break; }

        used[best_at] = LDG_TRUTH_TRUE;
        share = ctx->prog_total == 0 ? 0.0 : ((double)best / (double)ctx->prog_total) * PROFILE_PCT_SCALE;
        bar_cells = (uint32_t)(((double)col_cunt * share) / PROFILE_PCT_SCALE);

        if (ctx->elf != 0x0 && crosstalk_hdl_elf_label_get(ctx->elf, ctx->prog_addr[best_at], label, (uint32_t)sizeof(label)) == LDG_ERR_AOK) { label_ok = LDG_TRUTH_TRUE; }

        if (label_ok == LDG_TRUTH_TRUE) { snprintf(text, sizeof(text), PROFILE_PROG_ROW_SYM_FMT, ctx->prog_addr[best_at], label, ctx->prog_hits[best_at], share); }
        else { snprintf(text, sizeof(text), PROFILE_PROG_ROW_FMT, ctx->prog_addr[best_at], ctx->prog_hits[best_at], share); }

        ret = profile_bar_row_put(render, *row_at, text, bar_cells, (uint32_t)PROFILE_TEXT_COL, (uint32_t)PROFILE_BAR_DIM_COL);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        *row_at = *row_at + (uint32_t)PROFILE_STEP;
    }

    ret = profile_line_put(render, (uint32_t)PROFILE_ORIG_COL, row_at, PROFILE_PROG_FRAME_HDR_TEXT, (uint32_t)PROFILE_DIM_COL);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (row = 0; row < (uint32_t)PROFILE_PROG_FRAME_ROWS; row++)
    {
        char label[CROSSTALK_HDL_ELF_LABEL_CAP] = LDG_ARR_ZERO_INIT;
        uint64_t frame_addr = 0;
        uint32_t best = 0;
        uint32_t best_frame = UINT32_MAX;
        uint32_t skip = 0;
        uint32_t pu = 0;
        double share = 0.0;
        uint8_t label_ok = LDG_TRUTH_FALSE;

        for (at = 0; at < (uint32_t)CROSSTALK_EDA_UI_PROFILE_PROG_FRAME_CAP; at++)
        {
            skip = 0;

            for (pu = 0; pu < frame_used_cunt; pu++) { if (frame_used[pu] == at) { skip = 0x1; break; } }

            if (skip != 0 || ctx->prog_frame[at] == 0) { continue; }

            if (ctx->prog_frame[at] >= best) { best = ctx->prog_frame[at]; best_frame = at; }
        }

        if (best_frame == UINT32_MAX || best == 0) { break; }

        frame_used[frame_used_cunt] = best_frame;
        frame_used_cunt++;
        share = ctx->prog_total == 0 ? 0.0 : ((double)best / (double)ctx->prog_total) * PROFILE_PCT_SCALE;

        frame_addr = (uint64_t)best_frame << PROFILE_PROG_FRAME_SHIFT;

        if (ctx->elf != 0x0 && crosstalk_hdl_elf_label_get(ctx->elf, frame_addr, label, (uint32_t)sizeof(label)) == LDG_ERR_AOK) { label_ok = LDG_TRUTH_TRUE; }

        if (label_ok == LDG_TRUTH_TRUE) { snprintf(text, sizeof(text), PROFILE_PROG_FRAME_SYM_FMT, frame_addr, label, ctx->prog_frame[best_frame], share); }
        else { snprintf(text, sizeof(text), PROFILE_PROG_FRAME_FMT, frame_addr, ctx->prog_frame[best_frame], share); }

        ret = profile_line_put(render, (uint32_t)PROFILE_SUB_INDENT, row_at, text, (uint32_t)PROFILE_TEXT_COL);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static LDG_NOINLINE uint32_t profile_strip_draw(crosstalk_eda_ui_profile_ctx_t *ctx, ldg_term_render_t *render, uint32_t strip_row, uint32_t col_cunt, uint64_t have, uint64_t first_cyc)
{
    uint64_t peak = 0;
    uint32_t cell_w = 0;
    uint32_t cell_h = 0;
    uint32_t strip_h = 0;
    uint32_t strip_y = 0;
    uint32_t col = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    cell_w = render->sample_col_cunt;
    cell_h = render->sample_row_cunt;
    strip_h = (uint32_t)PROFILE_STRIP_ROW_CUNT * cell_h;
    strip_y = strip_row * cell_h;
    ctx->strip_step = have / (uint64_t)col_cunt;

    if (ctx->strip_step == 0) { ctx->strip_step = (uint64_t)PROFILE_STEP; }

    for (at = 0; at < (uint32_t)have; at++)
    {
        uint64_t sample = ctx->ring_tot[(first_cyc + (uint64_t)at) & ((uint64_t)CROSSTALK_EDA_UI_PROFILE_RING_CAP - 0x1)];

        if (sample > peak) { peak = sample; }
    }

    if (peak == 0) { peak = 0x1; }

    for (col = 0; col < col_cunt; col++)
    {
        uint64_t lo = ((uint64_t)col * have) / (uint64_t)col_cunt;
        uint64_t hi = (((uint64_t)col + 0x1) * have) / (uint64_t)col_cunt;
        uint64_t tot_max = 0;
        uint64_t comb_max = 0;
        uint64_t walk = 0;
        uint32_t bar_h = 0;
        uint32_t comb_h = 0;

        if (hi <= lo) { hi = lo + 0x1; }

        for (walk = lo; walk < hi && walk < have; walk++)
        {
            uint64_t ring_at = (first_cyc + walk) & ((uint64_t)CROSSTALK_EDA_UI_PROFILE_RING_CAP - 0x1);

            if (ctx->ring_tot[ring_at] > tot_max) { tot_max = ctx->ring_tot[ring_at]; comb_max = ctx->ring_comb[ring_at]; }
        }

        bar_h = (uint32_t)(((uint64_t)strip_h * tot_max) / peak);
        comb_h = tot_max == 0 ? (uint32_t)PROFILE_BAR_NONE : (uint32_t)(((uint64_t)bar_h * comb_max) / tot_max);

        if (bar_h != 0)
        {
            ret = crosstalk_ui_cell_sample_fill(render, col * cell_w, strip_y + strip_h - bar_h, cell_w, bar_h, (uint32_t)PROFILE_BAR_COL);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        if (comb_h != 0)
        {
            ret = crosstalk_ui_cell_sample_fill(render, col * cell_w, strip_y + strip_h - comb_h, cell_w, comb_h, (uint32_t)PROFILE_COMB_COL);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }

    if (ctx->sel_cyc == UINT64_MAX || ctx->sel_cyc < first_cyc || ctx->sel_cyc >= first_cyc + have) { return LDG_ERR_AOK; }

    col = (uint32_t)(((ctx->sel_cyc - first_cyc) * (uint64_t)col_cunt) / have);

    return crosstalk_ui_cell_sample_fill(render, col * cell_w, strip_y, cell_w, strip_h, (uint32_t)CROSSTALK_UI_THEME_WHITE_RGBA);
}

static uint32_t profile_draw_cb(uint64_t win_id, void *user, ldg_term_render_t *render, uint32_t col_cunt, uint32_t row_cunt)
{
    crosstalk_eda_ui_profile_ctx_t *ctx = (crosstalk_eda_ui_profile_ctx_t *)user;
    char text[PROFILE_TEXT_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t have = 0;
    uint64_t first_cyc = 0;
    uint64_t sel_at = 0;
    uint32_t row = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (win_id != (uint64_t)CROSSTALK_EDA_UI_PROFILE_WINDOW_ID || col_cunt == 0 || row_cunt == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = profile_tab_draw(ctx, render, row);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    row = row + (uint32_t)PROFILE_STEP;

    if (ctx->tab == (uint8_t)CROSSTALK_EDA_UI_PROFILE_TAB_PROG)
    {
        ret = profile_controls_draw(ctx, render, row);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        row = row + (uint32_t)PROFILE_STEP + (uint32_t)PROFILE_STEP;

        ret = profile_prog_draw(ctx, render, &row, col_cunt);
        ctx->win.content_row_cunt = row;

        return ret;
    }

    if (ctx->tab == (uint8_t)CROSSTALK_EDA_UI_PROFILE_TAB_EMU)
    {
        row = row + (uint32_t)PROFILE_STEP;

        ret = profile_emu_draw(ctx, render, &row);
        ctx->win.content_row_cunt = row;

        return ret;
    }

    have = ctx->meta[PROFILE_META_CYC_AT] < (uint64_t)CROSSTALK_EDA_UI_PROFILE_RING_CAP ? ctx->meta[PROFILE_META_CYC_AT] : (uint64_t)CROSSTALK_EDA_UI_PROFILE_RING_CAP;
    first_cyc = ctx->meta[PROFILE_META_CYC_AT] - have;

    if (ctx->bound != LDG_TRUTH_TRUE || have == 0)
    {
        ret = profile_line_put(render, (uint32_t)PROFILE_ORIG_COL, &row, PROFILE_NO_DATA_TEXT, (uint32_t)PROFILE_DIM_COL);
        ctx->win.content_row_cunt = row;

        return ret;
    }

    ret = profile_controls_draw(ctx, render, row);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    row = row + (uint32_t)PROFILE_STEP + (uint32_t)PROFILE_STEP;

    ret = profile_strip_draw(ctx, render, row, col_cunt, have, first_cyc);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    row = row + (uint32_t)PROFILE_STRIP_ROW_CUNT + (uint32_t)PROFILE_STEP;

    if (ctx->sel_cyc != UINT64_MAX && ctx->sel_cyc >= first_cyc && ctx->sel_cyc < ctx->meta[PROFILE_META_CYC_AT])
    {
        sel_at = ctx->sel_cyc & ((uint64_t)CROSSTALK_EDA_UI_PROFILE_RING_CAP - 0x1);
        snprintf(text, sizeof(text), PROFILE_SEL_FMT, ctx->sel_cyc, ctx->ring_tot[sel_at], ctx->ring_comb[sel_at], profile_pct_get(ctx->ring_comb[sel_at], ctx->ring_tot[sel_at]), ctx->ring_blk[sel_at], profile_pct_get(ctx->ring_blk[sel_at], ctx->ring_tot[sel_at]), ctx->ring_cmt[sel_at], profile_pct_get(ctx->ring_cmt[sel_at], ctx->ring_tot[sel_at]));
    }
    else { snprintf(text, sizeof(text), PROFILE_SEL_NONE_TEXT); }

    ret = profile_line_put(render, (uint32_t)PROFILE_ORIG_COL, &row, text, (uint32_t)PROFILE_TEXT_COL);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = profile_evt_row_draw(ctx, render, row);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    row = row + (uint32_t)PROFILE_STEP + (uint32_t)PROFILE_STEP;

    ret = profile_agg_draw(ctx, render, &row, col_cunt);
    ctx->win.content_row_cunt = row;

    return ret;
}

static uint32_t profile_sel_mov(crosstalk_eda_ui_profile_ctx_t *ctx, uint8_t back)
{
    uint64_t have = 0;
    uint64_t first_cyc = 0;
    uint64_t last_cyc = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    have = ctx->meta[PROFILE_META_CYC_AT] < (uint64_t)CROSSTALK_EDA_UI_PROFILE_RING_CAP ? ctx->meta[PROFILE_META_CYC_AT] : (uint64_t)CROSSTALK_EDA_UI_PROFILE_RING_CAP;
    if (have == 0) { return LDG_ERR_EMPTY; }

    first_cyc = ctx->meta[PROFILE_META_CYC_AT] - have;
    last_cyc = ctx->meta[PROFILE_META_CYC_AT] - (uint64_t)PROFILE_STEP;

    if (ctx->sel_cyc == UINT64_MAX || ctx->sel_cyc < first_cyc || ctx->sel_cyc > last_cyc) { ctx->sel_cyc = last_cyc; return LDG_ERR_AOK; }

    if (back == LDG_TRUTH_TRUE) { ctx->sel_cyc = ctx->sel_cyc - first_cyc < ctx->strip_step ? first_cyc : ctx->sel_cyc - ctx->strip_step; }
    else { ctx->sel_cyc = last_cyc - ctx->sel_cyc < ctx->strip_step ? last_cyc : ctx->sel_cyc + ctx->strip_step; }

    return LDG_ERR_AOK;
}

static void profile_evt_cycle(crosstalk_eda_ui_profile_ctx_t *ctx, uint8_t back)
{
    uint32_t at = ctx->view_evt;
    uint32_t step = 0;

    for (step = 0; step < (uint32_t)CROSSTALK_EDA_UI_PROFILE_EVT_CAP; step++)
    {
        if (at == UINT32_MAX) { at = back == LDG_TRUTH_TRUE ? (uint32_t)CROSSTALK_EDA_UI_PROFILE_EVT_CAP - (uint32_t)PROFILE_STEP : (uint32_t)PROFILE_EVT_FIRST; }
        else if (back == LDG_TRUTH_TRUE) { at = at == 0 ? (uint32_t)CROSSTALK_EDA_UI_PROFILE_EVT_CAP - (uint32_t)PROFILE_STEP : at - (uint32_t)PROFILE_STEP; }
        else { at = (at + (uint32_t)PROFILE_STEP) % (uint32_t)CROSSTALK_EDA_UI_PROFILE_EVT_CAP; }

        if (ctx->evt[at].used != 0) { ctx->view_evt = at; return; }
    }
}

static uint32_t profile_key_isr(uint64_t win_id, void *user, const ldg_term_key_event_t *key, uint8_t *consumed)
{
    crosstalk_eda_ui_profile_ctx_t *ctx = (crosstalk_eda_ui_profile_ctx_t *)user;
    uint32_t ch = 0;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(ctx == 0x0 || key == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    if (win_id != (uint64_t)CROSSTALK_EDA_UI_PROFILE_WINDOW_ID) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (key->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE) { return LDG_ERR_AOK; }

    if ((key->mods & ((uint32_t)LDG_TERM_MOD_CTRL | (uint32_t)LDG_TERM_MOD_ALT | (uint32_t)LDG_TERM_MOD_SUPER)) != 0) { return LDG_ERR_AOK; }

    *consumed = LDG_TRUTH_TRUE;

    if (key->key == (uint8_t)LDG_TERM_KEY_LHS) { return profile_sel_mov(ctx, LDG_TRUTH_TRUE) == LDG_ERR_FUNC_ARG_NULL ? LDG_ERR_FUNC_ARG_NULL : LDG_ERR_AOK; }

    if (key->key == (uint8_t)LDG_TERM_KEY_RHS) { return profile_sel_mov(ctx, LDG_TRUTH_FALSE) == LDG_ERR_FUNC_ARG_NULL ? LDG_ERR_FUNC_ARG_NULL : LDG_ERR_AOK; }

    if (key->key == (uint8_t)LDG_TERM_KEY_UP)
    {
        if (ctx->agg_sel != 0) { ctx->agg_sel = ctx->agg_sel - (uint32_t)PROFILE_STEP; }

        return LDG_ERR_AOK;
    }

    if (key->key == (uint8_t)LDG_TERM_KEY_DOWN)
    {
        if (ctx->agg_sel + (uint32_t)PROFILE_STEP < (uint32_t)PROFILE_TOP_ROWS) { ctx->agg_sel = ctx->agg_sel + (uint32_t)PROFILE_STEP; }

        return LDG_ERR_AOK;
    }

    if (key->key == (uint8_t)LDG_TERM_KEY_ENTER)
    {
        if (ctx->agg_sel_slot < (uint32_t)CROSSTALK_EDA_UI_PROFILE_SLOT_CAP) { ctx->expand[ctx->agg_sel_slot] = ctx->expand[ctx->agg_sel_slot] != 0 ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE; }

        return LDG_ERR_AOK;
    }

    if (key->text_cunt == 0) { *consumed = LDG_TRUTH_FALSE; return LDG_ERR_AOK; }

    ch = key->text[0];

    if (ch == (uint32_t)PROFILE_KEY_START) { ret = crosstalk_eda_ui_profile_mark_start(ctx, 0x0); }
    else if (ch == (uint32_t)PROFILE_KEY_END) { ret = crosstalk_eda_ui_profile_mark_end(ctx); }
    else if (ch == (uint32_t)PROFILE_KEY_RST) { ret = crosstalk_eda_ui_profile_rst(ctx); }
    else if (ch == (uint32_t)PROFILE_KEY_LIVE) { ctx->view_evt = UINT32_MAX; }
    else if (ch == (uint32_t)PROFILE_KEY_EVT_PREV) { profile_evt_cycle(ctx, LDG_TRUTH_TRUE); }
    else if (ch == (uint32_t)PROFILE_KEY_EVT_NEXT) { profile_evt_cycle(ctx, LDG_TRUTH_FALSE); }
    else { *consumed = LDG_TRUTH_FALSE; }

    if (ret == LDG_ERR_EMPTY) { return LDG_ERR_AOK; }

    return ret;
}

uint32_t crosstalk_eda_ui_profile_fn_top_get(const crosstalk_eda_ui_profile_ctx_t *ctx, uint64_t *out)
{
    uint64_t best = 0;
    uint32_t best_at = UINT32_MAX;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    for (at = 0; at < ctx->fn_cunt; at++)
    {
        if (ctx->fn_hits[at] <= best) { continue; }

        best = ctx->fn_hits[at];
        best_at = at;
    }

    if (best_at == UINT32_MAX) { return LDG_ERR_NOT_FOUND; }

    *out = ctx->fn_addr[best_at];

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_profile_elf_bind(crosstalk_eda_ui_profile_ctx_t *ctx, const crosstalk_hdl_elf_ctx_t *elf)
{
    if (LDG_UNLIKELY(ctx == 0x0 || elf == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->elf = elf;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ui_profile_win_reg(crosstalk_eda_ui_profile_ctx_t *ctx, crosstalk_ui_window_ctx_t *win_nav, uint32_t *out_slot)
{
    if (LDG_UNLIKELY(ctx == 0x0 || win_nav == 0x0 || out_slot == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_ui_window_reg(win_nav, (uint64_t)CROSSTALK_EDA_UI_PROFILE_WINDOW_ID, CROSSTALK_EDA_UI_PROFILE_TITLE, &ctx->win, profile_draw_cb, profile_key_isr, ctx, out_slot);
}
