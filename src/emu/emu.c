#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/logging/logging.h>

#include <dangling/arch/amd64/atomic.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/file.h>
#include <dangling/mem/alloc.h>
#include <dangling/str/str.h>
#include <dangling/thread/yield.h>
#include <dangling/time/time.h>

#include "core/log.h"
#include "emu/emu.h"

static uint8_t emu_hex_path_is(const char *path)
{
    uint64_t len = 0;

    if (LDG_UNLIKELY(path == 0x0)) { return LDG_TRUTH_FALSE; }

    len = (uint64_t)strlen(path);

    if (len < (uint64_t)CROSSTALK_EMU_HEX_EXT_LEN) { return LDG_TRUTH_FALSE; }

    return strncmp(&path[len - (uint64_t)CROSSTALK_EMU_HEX_EXT_LEN], CROSSTALK_EMU_HEX_EXT, (uint64_t)CROSSTALK_EMU_HEX_EXT_LEN) == 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t emu_file_slurp(const char *path, ldg_byte_t **out, uint64_t *out_span)
{
    ldg_io_info_t st = LDG_STRUCT_ZERO_INIT;
    ldg_io_file_t *file = 0x0;
    uint64_t got = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(path == 0x0 || out == 0x0 || out_span == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;
    *out_span = 0;

    ret = ldg_io_path_info_get(path, &st);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (st.size == 0) { return LDG_ERR_EMPTY; }

    ret = ldg_mem_alloc(st.size, (void **)out);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, CROSSTALK_EDA_IC_FPGA_EMU_FILE_MODE, &file);
    if (ret == LDG_ERR_AOK)
    {
        ret = ldg_io_file_rd(file, *out, st.size, &got);

        if (ldg_io_file_close(file) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu program close failed"); }
    }

    if (ret != LDG_ERR_AOK)
    {
        ldg_mem_dealloc(*out);
        *out = 0x0;

        return ret;
    }

    if (got != st.size)
    {
        ldg_mem_dealloc(*out);
        *out = 0x0;

        return LDG_ERR_IO_RD;
    }

    *out_span = st.size;

    return LDG_ERR_AOK;
}

static uint32_t emu_prog_st(const crosstalk_hdl_abi_t *abi, uint32_t mem_sig, const ldg_byte_t *raw, uint64_t span, uint8_t hex_is, uint64_t word_off, uint64_t *out_words)
{
    uint64_t addr = word_off;
    uint64_t pack = 0;
    uint64_t at = 0;
    uint32_t held = 0;

    if (LDG_UNLIKELY(abi == 0x0 || raw == 0x0 || out_words == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_words = 0;

    if (abi->mem_st == 0x0) { return LDG_ERR_UNSUPPORTED; }

    if (mem_sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { return CROSSTALK_EMU_ERR_SIG; }

    for (at = 0; at < span; at = at + (uint64_t)CROSSTALK_EMU_STEP)
    {
        if (hex_is == LDG_TRUTH_TRUE)
        {
            ldg_byte_t nib = 0;

            if (ldg_char_hex_is((char)raw[at]) == LDG_TRUTH_TRUE)
            {
                if (ldg_hex_to_nipple((char)raw[at], &nib) != LDG_ERR_AOK) { continue; }

                pack = pack * (uint64_t)LDG_BASE_HEX + (uint64_t)nib;
                held = held + (uint32_t)CROSSTALK_EMU_STEP;

                continue;
            }

            if (held == 0) { continue; }

            abi->mem_st(mem_sig, addr, pack);
            addr = addr + (uint64_t)CROSSTALK_EMU_STEP;
            pack = 0;
            held = 0;

            continue;
        }

        pack = pack | ((uint64_t)raw[at] << (uint64_t)((uint32_t)LDG_BYTE_BITS * held));
        held = held + (uint32_t)CROSSTALK_EMU_STEP;

        if (held < (uint32_t)CROSSTALK_EMU_WORD_BYTES) { continue; }

        abi->mem_st(mem_sig, addr, pack);
        addr = addr + (uint64_t)CROSSTALK_EMU_STEP;
        pack = 0;
        held = 0;
    }

    if (held != 0)
    {
        abi->mem_st(mem_sig, addr, pack);
        addr = addr + (uint64_t)CROSSTALK_EMU_STEP;
    }

    *out_words = addr - word_off;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_conf_default_get(crosstalk_emu_conf_t *out)
{
    uint32_t drive = 0;

    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(out, 0x0, sizeof(*out));

    for (drive = 0; drive < (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX; drive = drive + (uint32_t)CROSSTALK_EMU_STEP) { out->disk_path[drive] = 0x0; }

    out->clk_sig = CROSSTALK_EMU_CLK_SIG_DEFAULT;
    out->mem_sig = CROSSTALK_EMU_MEM_SIG_DEFAULT;
    out->f_op_hz = (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_CLK_HZ;
    out->win_w = (uint32_t)CROSSTALK_EMU_WIN_W_DEFAULT;
    out->win_h = (uint32_t)CROSSTALK_EMU_WIN_H_DEFAULT;
    out->key_mode = (uint32_t)CROSSTALK_EMU_INPUT_MODE_SCAN;
    out->audio_on = LDG_TRUTH_TRUE;
    out->disk_persist = LDG_TRUTH_TRUE;
    out->serial_on = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_rst(crosstalk_emu_ctx_t *ctx)
{
    const crosstalk_hdl_abi_t *abi = 0x0;
    ldg_byte_t *prog = 0x0;
    uint64_t prog_span = 0;
    uint64_t words = 0;
    uint32_t imgs = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_eda_ic_fpga_emu_rst(&ctx->emu);
    if (ret != LDG_ERR_AOK) { return ret; }

    LDG_ST_RELEASE(ctx->cycles_total, 0);
    LDG_ST_RELEASE(ctx->cycles_gen, LDG_LD_ACQUIRE(ctx->cycles_gen) + (uint64_t)CROSSTALK_EMU_STEP);
    ctx->serial_bytes = 0;
    ctx->hlt = LDG_TRUTH_FALSE;

    ret = crosstalk_eda_ic_fpga_emu_abi_get(&ctx->emu, &abi);
    if (ret != LDG_ERR_AOK) { return ret; }

    for (at = 0; at < ctx->ld_cunt; at = at + (uint32_t)CROSSTALK_EMU_STEP)
    {
        ret = emu_file_slurp(ctx->ld_path[at], &prog, &prog_span);
        if (ret != LDG_ERR_AOK)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: reset image not readable; path: %.127s; err: %u32", ctx->ld_path[at], ret);

            continue;
        }

        ret = emu_prog_st(abi, ctx->mem_sig, prog, prog_span, emu_hex_path_is(ctx->ld_path[at]), ctx->ld_off[at], &words);

        ldg_mem_dealloc(prog);

        if (ret != LDG_ERR_AOK)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: reset image store failed; path: %.127s; err: %u32", ctx->ld_path[at], ret);

            continue;
        }

        imgs = imgs + (uint32_t)CROSSTALK_EMU_STEP;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu: reset; images reloaded: %u32", imgs);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_init(crosstalk_emu_ctx_t *ctx, const crosstalk_emu_conf_t *conf)
{
    const crosstalk_hdl_abi_t *abi = 0x0;
    ldg_byte_t *prog = 0x0;
    uint64_t prog_span = 0;
    uint64_t words = 0;
    uint32_t drive = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || conf == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (conf->plugin_path == 0x0) { return CROSSTALK_EMU_ERR_PLUGIN; }

    memset(ctx, 0x0, sizeof(*ctx));

    ctx->f_op_hz = conf->f_op_hz != 0 ? conf->f_op_hz : (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_CLK_HZ;
    ctx->serial_on = conf->serial_on;
    ctx->audio_on = conf->audio_on;
    ctx->clk_sig = (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE;
    ctx->mem_sig = (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE;

    if (conf->font_path != 0x0) { if (ldg_str_rbrcpy(ctx->font_path, conf->font_path, (uint64_t)sizeof(ctx->font_path)) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu font path truncated"); } }

    ret = crosstalk_eda_ic_fpga_emu_init(&ctx->emu);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->emu.disk_persist = conf->disk_persist;

    ret = crosstalk_eda_ic_fpga_emu_attach(&ctx->emu, conf->plugin_path);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "emu: the core would not attach; path: %.191s; err: %u32", conf->plugin_path, ret);

        return CROSSTALK_EMU_ERR_PLUGIN;
    }

    ret = crosstalk_eda_ic_fpga_emu_abi_get(&ctx->emu, &abi);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (abi->sig_find == 0x0) { return CROSSTALK_EMU_ERR_SIG; }

    ctx->clk_sig = abi->sig_find(conf->clk_sig != 0x0 ? conf->clk_sig : CROSSTALK_EMU_CLK_SIG_DEFAULT);
    ctx->mem_sig = abi->sig_find(conf->mem_sig != 0x0 ? conf->mem_sig : CROSSTALK_EMU_MEM_SIG_DEFAULT);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu: sigs; clk: %u32; mem: %u32; hz: %u64", ctx->clk_sig, ctx->mem_sig, ctx->f_op_hz);

    for (at = 0; at < conf->ld_cunt && at < (uint32_t)CROSSTALK_EMU_LD_MAX; at = at + (uint32_t)CROSSTALK_EMU_STEP)
    {
        if (conf->ld_path[at] == 0x0) { continue; }

        ret = emu_file_slurp(conf->ld_path[at], &prog, &prog_span);
        if (ret != LDG_ERR_AOK)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "emu: image not readable; path: %.191s; err: %u32", conf->ld_path[at], ret);

            return CROSSTALK_EMU_ERR_PROG;
        }

        ret = emu_prog_st(abi, ctx->mem_sig, prog, prog_span, emu_hex_path_is(conf->ld_path[at]), conf->ld_off[at], &words);

        ldg_mem_dealloc(prog);

        if (ret != LDG_ERR_AOK)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "emu: image store failed; path: %.191s; err: %u32", conf->ld_path[at], ret);

            return CROSSTALK_EMU_ERR_PROG;
        }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu: image stored; path: %.127s; bytes: %u64; words: %u64; word off: %u64", conf->ld_path[at], prog_span, words, conf->ld_off[at]);

        if (ldg_str_rbrcpy(ctx->ld_path[ctx->ld_cunt], conf->ld_path[at], (uint64_t)sizeof(ctx->ld_path[ctx->ld_cunt])) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu image path truncated"); continue; }

        ctx->ld_off[ctx->ld_cunt] = conf->ld_off[at];
        ctx->ld_cunt = ctx->ld_cunt + (uint32_t)CROSSTALK_EMU_STEP;
    }

    for (drive = 0; drive < (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX; drive = drive + (uint32_t)CROSSTALK_EMU_STEP)
    {
        if (conf->disk_path[drive] == 0x0) { continue; }

        if (crosstalk_eda_ic_fpga_emu_disk_ld(&ctx->emu, drive, conf->disk_path[drive]) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: disk refused; drive: %u32; path: %.127s", drive, conf->disk_path[drive]); }
    }

    ret = crosstalk_emu_input_init(&ctx->input, conf->key_mode);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (crosstalk_emu_input_bind(&ctx->input, abi, CROSSTALK_EMU_INPUT_DATA_SIG_DEFAULT, CROSSTALK_EMU_INPUT_RDY_SIG_DEFAULT, CROSSTALK_EMU_INPUT_ACK_SIG_DEFAULT) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu: the core takes no keys; input stays idle"); }

    ret = crosstalk_emu_audio_init(&ctx->audio);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (crosstalk_emu_net_init(&ctx->net, conf->net_path) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: network stays detached; path: %.127s", conf->net_path); }

    if (conf->net_path != 0x0 && (abi->net_drain == 0x0 || abi->net_feed == 0x0)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: the core has no network exports; drain: %u32; feed: %u32", abi->net_drain != 0x0 ? 0x1 : 0x0, abi->net_feed != 0x0 ? 0x1 : 0x0); }

    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_shutdown(crosstalk_emu_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (crosstalk_emu_run_hlt(ctx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu run halt failed"); }

    if (crosstalk_emu_audio_shutdown(&ctx->audio) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu audio shutdown failed"); }

    if (crosstalk_emu_net_shutdown(&ctx->net) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu net shutdown failed"); }

    ctx->ok = LDG_TRUTH_FALSE;

    return crosstalk_eda_ic_fpga_emu_shutdown(&ctx->emu);
}

static uint32_t emu_run_worker(uint32_t worker_id, void *arg)
{
    crosstalk_emu_ctx_t *ctx = (crosstalk_emu_ctx_t *)arg;
    const crosstalk_hdl_abi_t *abi = 0x0;
    uint64_t mark = 0;
    uint64_t owed = 0;
    uint32_t drive = 0;
    uint32_t alive_now = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_eda_ic_fpga_emu_abi_get(&ctx->emu, &abi);
    if (ret != LDG_ERR_AOK) { return ret; }

    alive_now = LDG_ADD_FETCH(ctx->alive, 0x1);

    for (drive = 0; drive < (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX; drive = drive + (uint32_t)CROSSTALK_EMU_STEP)
    {
        if (crosstalk_eda_ic_fpga_emu_disk_live_is(&ctx->emu, drive) != LDG_TRUTH_TRUE) { continue; }

        if (crosstalk_eda_ic_fpga_emu_disk_bind(&ctx->emu, drive) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu run: disk rebind on the worker failed; drive: %u32", drive); }
        else { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu run: disk rebound on the worker; drive: %u32", drive); }
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu run: worker enter; id: %u32; alive: %u32", worker_id, alive_now);

    mark = ldg_time_epoch_ns_get();

    while (LDG_LD_ACQUIRE(ctx->stop) != LDG_TRUTH_TRUE)
    {
        uint64_t now = ldg_time_epoch_ns_get();
        uint64_t cap = crosstalk_emu_input_idle_is(&ctx->input) == LDG_TRUTH_TRUE ? (uint64_t)CROSSTALK_EMU_CHUNK_CYCLES : (uint64_t)CROSSTALK_EMU_INPUT_CHUNK_CYCLES;
        uint64_t hz = LDG_LD_ACQUIRE(ctx->f_op_hz);
        uint64_t slice = 0;
        uint64_t ran = 0;

        owed = owed + (hz * (now - mark)) / (uint64_t)LDG_NS_PER_SEC;
        mark = now;

        if (LDG_LD_ACQUIRE(ctx->rst_req) == LDG_TRUTH_TRUE)
        {
            LDG_ST_RELEASE(ctx->rst_req, LDG_TRUTH_FALSE);

            owed = 0;

            if (crosstalk_emu_rst(ctx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu reset failed"); }
            else { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu: reset; cycles: %u64", LDG_LD_ACQUIRE(ctx->cycles_total)); }

            continue;
        }

        if (owed > hz) { owed = hz; }

        if (ctx->hlt == LDG_TRUTH_TRUE || owed == 0)
        {
            if (ldg_thread_yield((uint64_t)CROSSTALK_EMU_IDLE_YIELD_NS) != LDG_ERR_AOK) { break; }

            continue;
        }

        slice = owed > cap ? cap : owed;
        owed = owed - slice;

        if (abi->time_set != 0x0) { if (abi->time_set(now) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu host time set failed"); } }

        if (abi->n_cycle != 0x0)
        {
            ret = abi->n_cycle(ctx->clk_sig, slice, &ran);

            LDG_ST_RELEASE(ctx->cycles_total, LDG_LD_ACQUIRE(ctx->cycles_total) + ran);
        }
        else if (abi->cycle != 0x0)
        {
            uint64_t at = 0;

            for (at = 0; at < slice; at = at + (uint64_t)CROSSTALK_EMU_STEP)
            {
                ret = abi->cycle(ctx->clk_sig);

                LDG_ST_RELEASE(ctx->cycles_total, LDG_LD_ACQUIRE(ctx->cycles_total) + (uint64_t)CROSSTALK_EMU_STEP);

                if (ret != LDG_ERR_AOK) { break; }
            }
        }
        else { break; }

        if (ret == LDG_ERR_AOK) { if (crosstalk_emu_input_cycle_drive(&ctx->input, abi) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu input drive failed"); } }

        if (ret != LDG_ERR_AOK)
        {
            ctx->hlt = LDG_TRUTH_TRUE;

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: hlt; cycles: %u64; err: %u32", LDG_LD_ACQUIRE(ctx->cycles_total), ret);
        }
    }

    alive_now = LDG_SUB_FETCH(ctx->alive, 0x1);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu run: worker exit; id: %u32; alive: %u32; cycles: %u64", worker_id, alive_now, LDG_LD_ACQUIRE(ctx->cycles_total));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_run_start(crosstalk_emu_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (ctx->live == LDG_TRUTH_TRUE) { return LDG_ERR_BUSY; }

    ret = ldg_thread_pool_init(&ctx->pool, (uint32_t)CROSSTALK_EMU_WORKERS);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_thread_pool_priority_set(&ctx->pool, LDG_THREAD_POOL_PRIORITY_REALTIME);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "emu run: worker priority; lvl: realtime; err: %u32", ret);

    LDG_ST_RELEASE(ctx->stop, LDG_TRUTH_FALSE);

    ret = ldg_thread_pool_start(&ctx->pool, emu_run_worker, ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->live = LDG_TRUTH_TRUE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu run: started; chunk: %u32 cycles", (uint32_t)CROSSTALK_EMU_CHUNK_CYCLES);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_run_hlt(crosstalk_emu_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    LDG_ST_RELEASE(ctx->stop, LDG_TRUTH_TRUE);

    while (LDG_LD_ACQUIRE(ctx->alive) != 0) { if (ldg_thread_yield((uint64_t)CROSSTALK_EMU_HLT_WAIT_NS) != LDG_ERR_AOK) { break; } }

    if (ldg_thread_pool_shutdown(&ctx->pool) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu run pool shutdown failed"); }

    ctx->live = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_rst_req(crosstalk_emu_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->live != LDG_TRUTH_TRUE) { return crosstalk_emu_rst(ctx); }

    LDG_ST_RELEASE(ctx->rst_req, LDG_TRUTH_TRUE);

    return LDG_ERR_AOK;
}

uint8_t crosstalk_emu_run_live_is(const crosstalk_emu_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->live == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_emu_pump(crosstalk_emu_ctx_t *ctx, double now)
{
    const crosstalk_hdl_abi_t *abi = 0x0;
    uint64_t got = 0;
    double rate_now = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ret = crosstalk_eda_ic_fpga_emu_abi_get(&ctx->emu, &abi);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (crosstalk_emu_screen_tick(&ctx->screen, abi, ctx->mem_sig, now) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu screen tick failed"); }

    if (ctx->serial_on == LDG_TRUTH_TRUE && abi->serial_drain != 0x0) { if (abi->serial_drain(ctx->serial, (uint64_t)CROSSTALK_EMU_SERIAL_CHUNK - (uint64_t)LDG_STR_TERM_SIZE, &got) == LDG_ERR_AOK && got != 0)
        {
            ctx->serial[got] = LDG_STR_TERM;
            ctx->serial_bytes = ctx->serial_bytes + got;
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu serial: %s", (const char *)ctx->serial);
        }
    }

    if (ctx->audio_on == LDG_TRUTH_TRUE) { if (crosstalk_emu_audio_tick(&ctx->audio, abi) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu audio tick failed"); } }

    ret = crosstalk_emu_net_tick(&ctx->net, abi);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu net: tick failed; err: %u32", ret); }

    rate_now = now;

    if (rate_now >= ctx->rate_next || rate_now < ctx->rate_mark)
    {
        uint64_t gen = 0;
        uint64_t total = 0;

        gen = LDG_LD_ACQUIRE(ctx->cycles_gen);
        total = LDG_LD_ACQUIRE(ctx->cycles_total);

        if (gen != LDG_LD_ACQUIRE(ctx->cycles_gen)) { return LDG_ERR_AOK; }

        if (ctx->rate_next != 0.0 && rate_now > ctx->rate_mark && gen == ctx->cycles_gen_mark && total >= ctx->cycles_mark)
        {
            uint32_t pending = 0;

            ctx->rate_hz = (double)(total - ctx->cycles_mark) / (rate_now - ctx->rate_mark);

            if (crosstalk_emu_input_pending_get(&ctx->input, &pending) != LDG_ERR_AOK) { pending = 0; }

            if (ctx->rate_log_cuntr % (uint64_t)CROSSTALK_EMU_RATE_LOG_EVERY == 0) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu: rate: %.0f cyc/s; keys pending: %u32; keys sent: %u64; keys dropped: %u64", ctx->rate_hz, pending, ctx->input.bytes_total, ctx->input.dropped); }

            ctx->rate_log_cuntr = ctx->rate_log_cuntr + (uint64_t)CROSSTALK_EMU_STEP;
        }
        else { ctx->rate_hz = 0.0; }

        ctx->rate_next = rate_now + CROSSTALK_EMU_RATE_SECS;
        ctx->rate_mark = rate_now;
        ctx->cycles_mark = total;
        ctx->cycles_gen_mark = gen;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_rate_get(const crosstalk_emu_ctx_t *ctx, double *out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = ctx->rate_hz;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_emu_hz_set(crosstalk_emu_ctx_t *ctx, uint64_t hz)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (hz < (uint64_t)CROSSTALK_EMU_HZ_MIN || hz > (uint64_t)CROSSTALK_EMU_HZ_MAX) { return LDG_ERR_RANGE; }

    LDG_ST_RELEASE(ctx->f_op_hz, hz);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu: clock set; hz: %u64", hz);

    return LDG_ERR_AOK;
}

uint64_t crosstalk_emu_hz_get(const crosstalk_emu_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return UINT64_MAX; }

    return LDG_LD_ACQUIRE(ctx->f_op_hz);
}

uint8_t crosstalk_emu_live_is(const crosstalk_emu_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->ok == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint8_t crosstalk_emu_hlt_is(const crosstalk_emu_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->hlt == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}
