#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/file.h>
#include <dangling/io/path.h>
#include <dangling/logging/logging.h>
#include <dangling/mem/alloc.h>
#include <dangling/str/fmt.h>
#include <dangling/sys/tty.h>
#include <dangling/term/input.h>
#include <dangling/term/render.h>
#include <dangling/term/sesh.h>
#include <dangling/thread/yield.h>
#include <dangling/time/time.h>

#include "compile/compile.h"
#include "core/core.h"
#include "run/run.h"
#include "ui/menu.h"
#include "ui/summary.h"
#include "ui/ui.h"

#define DANGEM_CORE_KEY_LAUNCH 'l'
#define DANGEM_CORE_KEY_BUILD 'b'
#define DANGEM_CORE_KEY_DEPS 'o'
#define DANGEM_CORE_KEY_QUIT 'q'
#define DANGEM_CORE_LAUNCH_CONFIRM_TEXT "The latest source is newer than the compiled binaries. Launch anyway (press again)?"
#define DANGEM_CORE_LAUNCH_ARM_MS 3000
#define DANGEM_CORE_CASE_GAP 0x20U
#define DANGEM_CORE_ARG_CAP 4

static uint32_t core_root_find(char *dst, uint64_t cap)
{
    char prog[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    ldg_io_info_t info = LDG_STRUCT_ZERO_INIT;
    uint64_t len = 0;
    uint32_t ret = 0;

    ret = ldg_io_path_prog_get(prog, (uint64_t)sizeof(prog));
    if (ret != LDG_ERR_AOK) { return ret; }

    if (ldg_io_path_info_get(prog, &info) == LDG_ERR_AOK && info.is_dir != LDG_TRUTH_FALSE) { return ldg_str_fmt(dst, cap, &len, "%.4095s", prog); }

    return ldg_io_path_parent_get(prog, dst, cap);
}

static uint32_t core_view_fill(dangem_core_t *core)
{
    const dangem_compile_t *build = &core->build;
    uint8_t stale = dangem_compile_stale_is(build);
    uint64_t len = 0;
    uint32_t ret = 0;

    if (build->list_ok != LDG_TRUTH_TRUE) { ret = ldg_str_fmt(core->view.launch, (uint64_t)sizeof(core->view.launch), &len, "no build list in this install"); }
    else if (build->out_path[0] == LDG_STR_TERM || ldg_io_path_exists_is(build->out_path) != LDG_TRUTH_TRUE) { ret = ldg_str_fmt(core->view.launch, (uint64_t)sizeof(core->view.launch), &len, "%.63s; will compile first", build->release); }
    else if (stale == LDG_TRUTH_TRUE) { ret = ldg_str_fmt(core->view.launch, (uint64_t)sizeof(core->view.launch), &len, "%.63s; source newer than binaries", build->release); }
    else { ret = ldg_str_fmt(core->view.launch, (uint64_t)sizeof(core->view.launch), &len, "%.63s; built", build->release); }

    if (ret != LDG_ERR_AOK) { return ret; }

    core->view.build_alert = build->cc_ok == LDG_TRUTH_TRUE && stale == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (build->cc_ok != LDG_TRUTH_TRUE) { ret = ldg_str_fmt(core->view.build, (uint64_t)sizeof(core->view.build), &len, "bundled compiler not found"); }
    else if (stale == LDG_TRUTH_TRUE) { ret = ldg_str_fmt(core->view.build, (uint64_t)sizeof(core->view.build), &len, "The available source is newer than your latest compiled binary"); }
    else { ret = ldg_str_fmt(core->view.build, (uint64_t)sizeof(core->view.build), &len, "up to date"); }

    if (ret != LDG_ERR_AOK) { return ret; }

    return ldg_str_fmt(core->view.deps, (uint64_t)sizeof(core->view.deps), &len, "%.40s", core->deps_ok == LDG_TRUTH_TRUE ? (dangem_run_ldg_bundled_is() == LDG_TRUTH_TRUE ? "LDG bundled" : "LDG installed") : "LDG missing");
}

static void core_deps_ck(dangem_core_t *core)
{
    char path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    const char *lib = 0x0;

    core->deps_ok = LDG_TRUTH_FALSE;

    if (dangem_run_ldg_bundled_is() != LDG_TRUTH_TRUE)
    {
        if (core->build.list_ok != LDG_TRUTH_TRUE || dangem_compile_list_key_get(&core->build.list, DANGEM_COMPILE_LIST_KEY_LDGLIB, &lib) != LDG_ERR_AOK) { return; }

        core->deps_ok = ldg_io_path_exists_is(lib) == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
        return;
    }

    if (ldg_io_path_join(core->root, DANGEM_CORE_LDG_DIR, path, (uint64_t)sizeof(path)) != LDG_ERR_AOK) { return; }

    core->deps_ok = ldg_io_path_exists_is(path) == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static void core_build_kick(dangem_core_t *core, uint8_t launch_after)
{
    uint64_t len = 0;
    uint32_t ret = 0;

    ret = dangem_compile_start(&core->build);
    if (ret != LDG_ERR_AOK)
    {
        if (ldg_str_fmt(core->view.notice, (uint64_t)sizeof(core->view.notice), &len, "build could not start; err: %u32", ret) != LDG_ERR_AOK) { core->view.notice[0] = LDG_STR_TERM; }

        ldg_logging_line_wr(DANGEM_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "dangem: build start failed; err: %u32", ret);
        return;
    }

    core->launch_after = launch_after;
    core->screen = DANGEM_CORE_SCREEN_SUMMARY;
}

static void core_launch_req(dangem_core_t *core)
{
    if (core->build.list_ok != LDG_TRUTH_TRUE) { return; }

    if (dangem_compile_stale_is(&core->build) == LDG_TRUTH_TRUE)
    {
        core_build_kick(core, LDG_TRUTH_TRUE);
        return;
    }

    core->launch = LDG_TRUTH_TRUE;
}

static void core_launch_menu_req(dangem_core_t *core)
{
    uint64_t now_ms = ldg_time_epoch_ms_get();
    uint64_t len = 0;

    if (core->build.list_ok != LDG_TRUTH_TRUE) { return; }

    if (core->build.out_path[0] == LDG_STR_TERM || ldg_io_path_exists_is(core->build.out_path) != LDG_TRUTH_TRUE)
    {
        core_build_kick(core, LDG_TRUTH_TRUE);
        return;
    }

    if (dangem_compile_stale_is(&core->build) == LDG_TRUTH_TRUE && (core->launch_confirm != LDG_TRUTH_TRUE || now_ms - core->launch_confirm_ms > (uint64_t)DANGEM_CORE_LAUNCH_ARM_MS))
    {
        core->launch_confirm = LDG_TRUTH_TRUE;
        core->launch_confirm_ms = now_ms;

        if (ldg_str_fmt(core->view.notice, (uint64_t)sizeof(core->view.notice), &len, DANGEM_CORE_LAUNCH_CONFIRM_TEXT) != LDG_ERR_AOK) { core->view.notice[0] = LDG_STR_TERM; }

        return;
    }

    core->launch_confirm = LDG_TRUTH_FALSE;
    core->launch = LDG_TRUTH_TRUE;
}

static void core_key_route(dangem_core_t *core, const ldg_term_key_event_t *event)
{
    uint64_t len = 0;
    uint32_t ch = event->codepoint;

    if (event->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE) { return; }

    if (ch >= 'A' && ch <= 'Z') { ch = ch + DANGEM_CORE_CASE_GAP; }

    if (event->key == (uint8_t)LDG_TERM_KEY_F10 || (core->screen == DANGEM_CORE_SCREEN_MENU && (ch == DANGEM_CORE_KEY_QUIT || event->key == (uint8_t)LDG_TERM_KEY_ESCAPE)))
    {
        if (dangem_compile_busy_is(&core->build) != LDG_TRUTH_TRUE) { core->quit = LDG_TRUTH_TRUE; }

        return;
    }

    if (core->screen == DANGEM_CORE_SCREEN_SUMMARY)
    {
        if (dangem_compile_busy_is(&core->build) == LDG_TRUTH_TRUE) { return; }

        if (event->key == (uint8_t)LDG_TERM_KEY_ESCAPE) { core->screen = DANGEM_CORE_SCREEN_MENU; }
        else if (event->key == (uint8_t)LDG_TERM_KEY_F2) { core_launch_req(core); }
        else if (event->key == (uint8_t)LDG_TERM_KEY_F5) { core_build_kick(core, LDG_TRUTH_FALSE); }

        return;
    }

    if (ch == DANGEM_CORE_KEY_LAUNCH) { core_launch_menu_req(core); }
    else if (ch == DANGEM_CORE_KEY_BUILD) { core_build_kick(core, LDG_TRUTH_FALSE); }
    else if (ch == DANGEM_CORE_KEY_DEPS)
    {
        core_deps_ck(core);
        if (ldg_str_fmt(core->view.notice, (uint64_t)sizeof(core->view.notice), &len, "%.60s", core->deps_ok == LDG_TRUTH_TRUE ? "dependencies are in place" : "LDG is missing from this install") != LDG_ERR_AOK) { core->view.notice[0] = LDG_STR_TERM; }
    }
}

static uint32_t core_frame_draw(dangem_core_t *core)
{
    ldg_term_render_t *render = 0x0;
    ldg_term_render_color_t bg = LDG_STRUCT_ZERO_INIT;
    uint32_t col_cunt = 0;
    uint32_t row_cunt = 0;
    uint32_t ret = 0;

    if (ldg_sys_tty_size_get(&col_cunt, &row_cunt) == LDG_ERR_AOK && col_cunt != 0 && row_cunt != 0)
    {
        if (col_cunt > (uint32_t)DANGEM_CORE_COL_MAX) { col_cunt = (uint32_t)DANGEM_CORE_COL_MAX; }

        if (row_cunt > (uint32_t)DANGEM_CORE_ROW_MAX) { row_cunt = (uint32_t)DANGEM_CORE_ROW_MAX; }

        if (col_cunt != core->col_cunt || row_cunt != core->row_cunt)
        {
            ret = ldg_term_sesh_resize(core->sesh, col_cunt, row_cunt);
            if (ret != LDG_ERR_AOK) { return ret; }

            core->col_cunt = col_cunt;
            core->row_cunt = row_cunt;
        }
    }

    ret = ldg_term_sesh_render_get(core->sesh, &render);
    if (ret == LDG_ERR_AOK) { ret = dangem_ui_bg_get(&bg); }

    if (ret == LDG_ERR_AOK) { ret = ldg_term_render_clear(render, &bg); }

    if (ret != LDG_ERR_AOK) { return ret; }

    ret = core_view_fill(core);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (core->screen == DANGEM_CORE_SCREEN_SUMMARY) { ret = dangem_ui_summary_draw(render, &core->build, ldg_time_epoch_ms_get()); }
    else { ret = dangem_ui_menu_draw(render, &core->view); }

    if (ret != LDG_ERR_AOK) { return ret; }

    return ldg_term_sesh_wr(core->sesh);
}

uint32_t dangem_core_create(dangem_core_t **out)
{
    ldg_term_input_conf_t input_conf = LDG_STRUCT_ZERO_INIT;
    ldg_term_sesh_conf_t sesh_conf = LDG_STRUCT_ZERO_INIT;
    dangem_core_t *core = 0x0;
    void *alloc = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    ret = ldg_mem_alloc((uint64_t)sizeof(dangem_core_t), &alloc);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK || alloc == 0x0)) { return LDG_ERR_ALLOC_NULL; }

    core = (dangem_core_t *)alloc;
    memset(core, 0x0, sizeof(*core));
    core->screen = DANGEM_CORE_SCREEN_MENU;

    ret = core_root_find(core->root, (uint64_t)sizeof(core->root));
    if (ret == LDG_ERR_AOK) { ret = dangem_compile_init(&core->build, core->root); }

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(DANGEM_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "dangem: init failed; err: %u32", ret);
        if (ldg_mem_dealloc(core) != LDG_ERR_AOK) { ldg_logging_line_wr(DANGEM_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangem: dealloc failed"); }

        return ret;
    }

    core_deps_ck(core);

    ret = ldg_term_input_conf_default_get(&input_conf);
    if (ret == LDG_ERR_AOK) { ret = ldg_term_input_create(&input_conf, &core->input); }

    if (ret == LDG_ERR_AOK) { ret = ldg_term_sesh_conf_default_get(&sesh_conf); }

    sesh_conf.col_max = (uint32_t)DANGEM_CORE_COL_MAX;
    sesh_conf.row_max = (uint32_t)DANGEM_CORE_ROW_MAX;
    sesh_conf.render.sample_col_cunt = (uint32_t)DANGEM_CORE_SAMPLE_CUNT;
    sesh_conf.render.sample_row_cunt = (uint32_t)DANGEM_CORE_SAMPLE_CUNT;

    if (ret == LDG_ERR_AOK) { ret = ldg_term_sesh_create(&sesh_conf, &core->sesh); }

    if (ret == LDG_ERR_AOK) { ret = ldg_term_sesh_title_set(core->sesh, DANGEM_CORE_TITLE, (uint64_t)(sizeof(DANGEM_CORE_TITLE) - LDG_STR_TERM_SIZE)); }

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(DANGEM_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "dangem: terminal init failed; err: %u32", ret);
        if (dangem_core_destroy(core) != LDG_ERR_AOK) { ldg_logging_line_wr(DANGEM_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangem: destroy failed"); }

        return ret;
    }

    *out = core;

    return LDG_ERR_AOK;
}

uint32_t dangem_core_run(dangem_core_t *core)
{
    ldg_term_key_event_t event = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(core == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (core->quit != LDG_TRUTH_TRUE && core->launch != LDG_TRUTH_TRUE)
    {
        ret = core_frame_draw(core);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(DANGEM_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangem: frame failed; err: %u32", ret); }

        if (ldg_term_input_poll(core->input) == LDG_ERR_AOK) { while (ldg_term_input_event_next(core->input, &event) == LDG_ERR_AOK) { core_key_route(core, &event); } }

        ret = dangem_compile_pump(&core->build);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(DANGEM_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangem: build pump failed; err: %u32", ret); }

        if (core->launch_after == LDG_TRUTH_TRUE && core->build.phase == DANGEM_COMPILE_PHASE_DONE)
        {
            core->launch_after = LDG_TRUTH_FALSE;
            core->launch = LDG_TRUTH_TRUE;
        }

        ret = ldg_thread_yield(DANGEM_CORE_FRAME_NS);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(DANGEM_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangem: yield failed; err: %u32", ret); }
    }

    return LDG_ERR_AOK;
}

uint32_t dangem_core_destroy(dangem_core_t *core)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(core == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (core->sesh != 0x0 && ldg_term_sesh_destroy(core->sesh) != LDG_ERR_AOK) { ret = LDG_ERR_INVALID; }

    core->sesh = 0x0;

    if (core->input != 0x0 && ldg_term_input_destroy(core->input) != LDG_ERR_AOK) { ret = LDG_ERR_INVALID; }

    core->input = 0x0;

    if (ldg_mem_dealloc(core) != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = LDG_ERR_INVALID; }

    return ret;
}

uint32_t dangem_core_launch(dangem_core_t *core, uint32_t *out_code)
{
    char game[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char root[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    const char *argv[DANGEM_CORE_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(core == 0x0 || out_code == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_code = 0;

    ret = ldg_str_fmt(game, (uint64_t)sizeof(game), &len, "%.4095s", core->build.out_path);
    if (ret == LDG_ERR_AOK) { ret = ldg_str_fmt(root, (uint64_t)sizeof(root), &len, "%.4095s", core->root); }

    if (ret != LDG_ERR_AOK) { return ret; }

    ret = dangem_core_destroy(core);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(DANGEM_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangem: teardown before launch failed; err: %u32", ret); }

    argv[0] = game;
    argv[1] = 0x0;

    ldg_logging_line_wr(DANGEM_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "dangem: launching; path: %.255s", game);

    return dangem_run_exec(game, argv, root, out_code);
}

int main(void)
{
    dangem_core_t *core = 0x0;
    uint32_t code = 0;
    uint32_t ret = 0;

    ret = dangem_run_tty_ensure();
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_logging_line_wr(DANGEM_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "dangem: no terminal; err: %u32", ret);
        return (int32_t)(ret & DANGEM_CORE_EXIT_MASK);
    }

    ret = ldg_mem_init();
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (int32_t)(ret & DANGEM_CORE_EXIT_MASK); }

    ret = dangem_core_create(&core);
    if (ret != LDG_ERR_AOK)
    {
        ldg_mem_shutdown();
        return (int32_t)(ret & DANGEM_CORE_EXIT_MASK);
    }

    ret = dangem_core_run(core);

    if (core->launch == LDG_TRUTH_TRUE)
    {
        ret = dangem_core_launch(core, &code);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(DANGEM_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "dangem: launch failed; err: %u32", ret); }

        ldg_mem_shutdown();
        return (int32_t)((ret != LDG_ERR_AOK ? ret : code) & DANGEM_CORE_EXIT_MASK);
    }

    if (dangem_core_destroy(core) != LDG_ERR_AOK) { ldg_logging_line_wr(DANGEM_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dangem: destroy failed"); }

    ldg_mem_shutdown();

    return (int32_t)(ret & DANGEM_CORE_EXIT_MASK);
}
