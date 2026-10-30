#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dangling/logging/logging.h>

#include <dangling/audio/audio.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/sys/tty.h>
#include <dangling/term/input.h>
#include <dangling/term/render.h>
#include <dangling/term/sesh.h>
#include <dangling/time/time.h>

#include "core/log.h"
#include "emu/cmdline.h"
#include "emu/emu.h"
#include "emu/toolbar.h"
#include "ui/key.h"

#define MAIN_EXIT_OK 0
#define MAIN_EXIT_FAIL 1
#define MAIN_TITLE_CAP 128
#define MAIN_TITLE_SECS 1.0
#define MAIN_TITLE_FMT "%s; %.2f MHz; %.2f ms"
#define MAIN_HZ_PER_MHZ 1000000.0
#define MAIN_MS_PER_SEC 1000.0
#define MAIN_FRAME_MS 16
#define MAIN_TIME_NONE 0.0
#define MAIN_RATE_NONE 0.0
#define MAIN_FRAME_STEP 1
#define MAIN_RST_CODEPOINT 'r'
#define MAIN_RST_MODS (LDG_TERM_MOD_CTRL | LDG_TERM_MOD_SHIFT)
#define MAIN_CLEAR_LEVEL 0

typedef struct main_frame_ctx
{
    crosstalk_emu_ctx_t *emu;
    crosstalk_emu_toolbar_ctx_t *toolbar;
    ldg_term_input_t *input;
} main_frame_ctx_t;

static uint32_t main_key_isr(main_frame_ctx_t *frame, const ldg_term_key_event_t *key)
{
    uint8_t consumed = LDG_TRUTH_FALSE;
    uint8_t release_report = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(frame == 0x0 || key == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_emu_toolbar_key_isr(frame->toolbar, frame->emu, key, &consumed);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (consumed == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (key->codepoint == (uint32_t)MAIN_RST_CODEPOINT && (key->mods & (uint32_t)MAIN_RST_MODS) == (uint32_t)MAIN_RST_MODS)
    {
        if (key->kind != (uint8_t)LDG_TERM_KEY_KIND_PRESS) { return LDG_ERR_AOK; }

        return crosstalk_emu_rst_req(frame->emu);
    }

    release_report = ldg_term_input_release_report_is(frame->input) == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    return crosstalk_emu_input_key_isr(&frame->emu->input, key, release_report);
}

static uint32_t main_keys_drain(main_frame_ctx_t *frame)
{
    ldg_term_key_event_t key = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(frame == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_term_input_poll(frame->input);
    if (ret != LDG_ERR_AOK) { return ret; }

    while (ldg_term_input_event_next(frame->input, &key) == LDG_ERR_AOK)
    {
        ret = crosstalk_ui_key_us_map(&key);
        if (ret == LDG_ERR_AOK) { ret = main_key_isr(frame, &key); }

        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangcsaemu: key isr failed; key: %u32; codepoint: %u32; err: %u32", (uint32_t)key.key, key.codepoint, ret); }
    }

    return LDG_ERR_AOK;
}

static uint32_t main_size_sync(ldg_term_sesh_t *sesh, ldg_term_render_t **render)
{
    uint32_t col_cunt = 0;
    uint32_t row_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(sesh == 0x0 || render == 0x0 || *render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_sys_tty_size_get(&col_cunt, &row_cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (col_cunt > (uint32_t)LDG_TERM_SESH_COL_MAX_DEFAULT) { col_cunt = (uint32_t)LDG_TERM_SESH_COL_MAX_DEFAULT; }

    if (row_cunt > (uint32_t)LDG_TERM_SESH_ROW_MAX_DEFAULT) { row_cunt = (uint32_t)LDG_TERM_SESH_ROW_MAX_DEFAULT; }

    if (col_cunt == (*render)->col_cunt && row_cunt == (*render)->row_cunt) { return LDG_ERR_AOK; }

    ret = ldg_term_sesh_resize(sesh, col_cunt, row_cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    return ldg_term_sesh_render_get(sesh, render);
}

static uint32_t main_title_put(ldg_term_sesh_t *sesh, const crosstalk_emu_ctx_t *emu, double frame_ms)
{
    char title[MAIN_TITLE_CAP] = LDG_ARR_ZERO_INIT;
    double rate_hz = MAIN_RATE_NONE;
    int32_t len = 0;

    if (LDG_UNLIKELY(sesh == 0x0 || emu == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (crosstalk_emu_rate_get(emu, &rate_hz) != LDG_ERR_AOK) { rate_hz = MAIN_RATE_NONE; }

    len = (int32_t)snprintf(title, sizeof(title), MAIN_TITLE_FMT, CROSSTALK_EMU_TITLE, rate_hz / MAIN_HZ_PER_MHZ, frame_ms);
    if (len < 0) { return LDG_ERR_STR_FMT; }

    if ((uint32_t)len >= (uint32_t)sizeof(title)) { len = (int32_t)(sizeof(title) - LDG_STR_TERM_SIZE); }

    return ldg_term_sesh_title_set(sesh, title, (uint64_t)len);
}

static uint32_t main_frame_draw(const crosstalk_emu_ctx_t *emu, const crosstalk_emu_toolbar_ctx_t *toolbar, ldg_term_render_t *render)
{
    ldg_term_render_color_t clear = LDG_STRUCT_ZERO_INIT;
    uint32_t screen_row_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(emu == 0x0 || toolbar == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    clear.red = (uint8_t)MAIN_CLEAR_LEVEL;
    clear.green = (uint8_t)MAIN_CLEAR_LEVEL;
    clear.blue = (uint8_t)MAIN_CLEAR_LEVEL;

    ret = ldg_term_render_clear(render, &clear);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (render->row_cunt > (uint32_t)CROSSTALK_EMU_TOOLBAR_ROW_CUNT)
    {
        screen_row_cunt = render->row_cunt - (uint32_t)CROSSTALK_EMU_TOOLBAR_ROW_CUNT;

        ret = crosstalk_emu_screen_draw(&emu->screen, render, render->col_cunt, screen_row_cunt);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangcsaemu: screen draw failed; err: %u32", ret); }
    }

    return crosstalk_emu_toolbar_draw(toolbar, render, render->col_cunt, render->row_cunt);
}

static uint32_t main_frame_wait(uint64_t frame_beg_ms)
{
    uint64_t now_ms = 0;
    uint32_t ret = 0;

    ret = ldg_time_monotonic_get(&now_ms);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (now_ms < frame_beg_ms || now_ms - frame_beg_ms >= (uint64_t)MAIN_FRAME_MS) { return LDG_ERR_AOK; }

    return ldg_time_ns_delay(((uint64_t)MAIN_FRAME_MS - (now_ms - frame_beg_ms)) * (uint64_t)LDG_NS_PER_MS);
}

int main(int argc, char **argv)
{
    crosstalk_emu_conf_t conf = LDG_STRUCT_ZERO_INIT;
    crosstalk_emu_ctx_t emu = LDG_STRUCT_ZERO_INIT;
    crosstalk_emu_toolbar_ctx_t toolbar = LDG_STRUCT_ZERO_INIT;
    ldg_term_sesh_conf_t sesh_conf = LDG_STRUCT_ZERO_INIT;
    ldg_term_input_conf_t input_conf = LDG_STRUCT_ZERO_INIT;
    main_frame_ctx_t frame = LDG_STRUCT_ZERO_INIT;
    ldg_term_sesh_t *sesh = 0x0;
    ldg_term_render_t *render = 0x0;
    ldg_term_input_t *input = 0x0;
    double gamma = CROSSTALK_EMU_SCREEN_GAMMA_DEFAULT;
    double title_next = MAIN_TIME_NONE;
    double title_mark = MAIN_TIME_NONE;
    uint64_t frames = 0;
    uint8_t help = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    ret = ldg_mem_init();
    if (ret != LDG_ERR_AOK) { return MAIN_EXIT_FAIL; }

    if (crosstalk_emu_cmdline_parse((uint32_t)argc, argv, &conf, &gamma, &help) != LDG_ERR_AOK) { if (crosstalk_emu_conf_default_get(&conf) != LDG_ERR_AOK) { return MAIN_EXIT_FAIL; } }

    if (help == LDG_TRUTH_TRUE)
    {
        if (crosstalk_emu_cmdline_usage_put() != LDG_ERR_AOK) { return MAIN_EXIT_FAIL; }

        return MAIN_EXIT_OK;
    }

    ret = ldg_term_sesh_conf_default_get(&sesh_conf);
    if (ret == LDG_ERR_AOK) { ret = ldg_term_sesh_create(&sesh_conf, &sesh); }

    if (ret == LDG_ERR_AOK) { ret = ldg_term_sesh_render_get(sesh, &render); }

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "dangcsaemu: terminal session init failed; err: %u32", ret);

        if (sesh != 0x0) { if (ldg_term_sesh_destroy(sesh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangcsaemu session destroy failed"); } }

        return MAIN_EXIT_FAIL;
    }

    ret = ldg_term_input_conf_default_get(&input_conf);
    if (ret == LDG_ERR_AOK) { ret = ldg_term_input_create(&input_conf, &input); }

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "dangcsaemu: input init failed; err: %u32", ret);

        if (ldg_term_sesh_destroy(sesh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangcsaemu session destroy failed"); }

        return MAIN_EXIT_FAIL;
    }

    if (main_size_sync(sesh, &render) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangcsaemu size sync failed"); }

    ret = ldg_audio_init();
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangcsaemu: audio init failed; sound stays off; err: %u32", ret); }

    if (conf.plugin_path != 0x0)
    {
        ret = crosstalk_emu_init(&emu, &conf);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "dangcsaemu: the core would not start; err: %u32", ret); }
        else
        {
            ret = crosstalk_emu_run_start(&emu);
            if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "dangcsaemu: the run worker would not start; err: %u32", ret); }
        }
    }

    ret = crosstalk_emu_screen_init(&emu.screen);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "dangcsaemu: screen init failed; err: %u32", ret);

        if (ldg_term_input_destroy(input) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangcsaemu input destroy failed"); }

        if (ldg_term_sesh_destroy(sesh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangcsaemu session destroy failed"); }

        return MAIN_EXIT_FAIL;
    }

    if (crosstalk_emu_screen_gamma_set(&emu.screen, gamma) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangcsaemu: gamma refused; gamma: %.2f", gamma); }

    if (crosstalk_emu_toolbar_init(&toolbar, &emu) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangcsaemu toolbar init failed"); }

    frame.emu = &emu;
    frame.toolbar = &toolbar;
    frame.input = input;

    while (crosstalk_emu_toolbar_close_req_is(&toolbar) != LDG_TRUTH_TRUE)
    {
        uint64_t now_ms = 0;
        double now = MAIN_TIME_NONE;

        if (ldg_time_monotonic_get(&now_ms) != LDG_ERR_AOK) { now_ms = 0; }

        now = (double)now_ms / MAIN_MS_PER_SEC;

        if (main_size_sync(sesh, &render) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangcsaemu size sync failed"); }

        if (main_keys_drain(&frame) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangcsaemu key drain failed"); }

        if (crosstalk_emu_live_is(&emu) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_emu_pump(&emu, now);
            if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangcsaemu: pump failed; err: %u32", ret); }
        }

        ret = main_frame_draw(&emu, &toolbar, render);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangcsaemu: frame draw failed; err: %u32", ret); }

        ret = ldg_term_sesh_wr(sesh);
        if (ret != LDG_ERR_AOK)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "dangcsaemu: terminal write failed; err: %u32", ret);

            break;
        }

        frames = frames + (uint64_t)MAIN_FRAME_STEP;

        if (now >= title_next)
        {
            if (title_next != MAIN_TIME_NONE && frames != 0 && now > title_mark) { if (main_title_put(sesh, &emu, (now - title_mark) * MAIN_MS_PER_SEC / (double)frames) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangcsaemu title set failed"); } }

            title_next = now + MAIN_TITLE_SECS;
            title_mark = now;
            frames = 0;
        }

        if (main_frame_wait(now_ms) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangcsaemu frame wait failed"); }
    }

    if (crosstalk_emu_screen_shutdown(&emu.screen) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangcsaemu screen shutdown failed"); }

    if (crosstalk_emu_shutdown(&emu) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangcsaemu shutdown failed"); }

    if (ldg_term_input_destroy(input) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangcsaemu input destroy failed"); }

    if (ldg_term_sesh_destroy(sesh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangcsaemu session destroy failed"); }

    ldg_audio_shutdown();

    if (ldg_mem_shutdown() != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangcsaemu memory shutdown failed"); }

    return MAIN_EXIT_OK;
}
