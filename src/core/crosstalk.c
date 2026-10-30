#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <time.h>

#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/path.h>
#include <dangling/io/file.h>
#include <dangling/io/dir.h>
#include <dangling/proto/emiemi.h>
#include <dangling/math/math.h>
#include <dangling/math/quaternion.h>
#include <dangling/str/str.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/secure.h>
#include <dangling/time/time.h>
#include <dangling/audio/audio.h>
#include <dangling/arch/amd64/atomic.h>
#include <dangling/arch/amd64/syscall.h>
#include <dangling/arch/dangcsa/dangcsa.h>

#define CROSSTALK_CORE_INSTABILITY_PORT_NAME "instability_in"
#define CROSSTALK_CORE_INSTABILITY_FEED_SECS 0.125

#include "net/steam.h"
#include "net/wire.h"
#include "net/presence.h"
#include "wshop/mount.h"
#include "wshop/publish.h"
#include <dangling/sys/cpuset.h>
#include <dangling/sys/info.h>
#include <dangling/sys/proc.h>
#include <dangling/sys/tty.h>
#include <dangling/term/render.h>
#include <dangling/term/sesh.h>

#include "core/crosstalk.h"
#include "game/menu.h"
#include "game/save.h"
#include "game/settings_menu.h"
#include "game/progression/progression.h"
#include "ui/cell.h"
#include "ui/theme.h"

#define CROSSTALK_WIN_W 2400U
#define CROSSTALK_WIN_H 1350U
#define CROSSTALK_NET_JOIN_PHASE_IDLE 0
#define CROSSTALK_NET_JOIN_PHASE_LIST 1
#define CROSSTALK_NET_JOIN_PHASE_CONN 2
#define CROSSTALK_NET_VIRTUAL_PORT 0
#define CROSSTALK_NET_LOBBY_MEMBERS_DEFAULT 0
#define CROSSTALK_APP_ID 4898790U
#define CROSSTALK_MENU_SWITCH_ITEM_CUNT 10
#define CROSSTALK_MENU_SOON_TEXT "[SOON]"
#define CROSSTALK_MENU_SWITCH_CAPTION "MENU"
#define CROSSTALK_PAUSE_SWITCH_CAPTION "PAUSED"
#define CROSSTALK_MENU_SWITCH_HD_ROWS 2
#define CROSSTALK_MENU_SWITCH_CENTER_DIV 2
#define CROSSTALK_MENU_SWITCH_COL_CUNT 28
#define CROSSTALK_MENU_SWITCH_INDENT 1
#define CROSSTALK_MENU_SWITCH_PAD 1
#define CROSSTALK_MENU_SEL_FIRST 0U
#define CROSSTALK_STATUSLINE_ROWS 1
#define CROSSTALK_STATUSLINE_BG_COL 0x10141CFFU
#define CROSSTALK_STATUSLINE_SEP " | "
#define CROSSTALK_SAVE_EDITOR_SECT_CAP 12
#define CROSSTALK_CORE_SV_HD_CAP 8192
#define CROSSTALK_CORE_SV_EXT_CAP 16
#define CROSSTALK_CORE_SV_FILE_MODE 0644
#define CROSSTALK_CORE_EDITOR_TOAST_CAP 64
#define CROSSTALK_CORE_ACH_RATE_HZ 500000000ULL
#define CROSSTALK_CORE_ACH_CYC_CUNT 1000000000ULL
#define CROSSTALK_MENU_IDX_LD 0U
#define CROSSTALK_MENU_IDX_STORY 1U
#define CROSSTALK_MENU_IDX_ISA 2U
#define CROSSTALK_MENU_IDX_RTL 3U
#define CROSSTALK_MENU_IDX_ZONE 4U
#define CROSSTALK_MENU_IDX_REVERSE 5U
#define CROSSTALK_MENU_IDX_MP 6U
#define CROSSTALK_MENU_IDX_EXTRAS 7U
#define CROSSTALK_MENU_IDX_CREDITS 8U
#define CROSSTALK_MENU_IDX_QUIT 9U
#define CROSSTALK_PAUSE_SWITCH_ITEM_CUNT 3
#define CROSSTALK_PAUSE_ITEM_RESUME 0
#define CROSSTALK_PAUSE_ITEM_SETTINGS 1
#define CROSSTALK_LD_POOL_WORKERS 1
#define CROSSTALK_LD_PHASE_BENCH_RDY 1
#define CROSSTALK_LD_PHASE_SAVE_RDY 2
#define CROSSTALK_LOBBY_WAVE_WIDTH_FRAC 1.2
#define CROSSTALK_LOBBY_WAVE_AMP_FRAC 0.22
#define CROSSTALK_LOBBY_WAVE_SPEED 0.9
#define CROSSTALK_LOBBY_WAVE_CYCLES 1.5
#define CROSSTALK_LOBBY_WAVE_SEG_CUNT 96
#define CROSSTALK_LOBBY_WAVE_FIELD_CUNT 2
#define CROSSTALK_LOBBY_WAVE_E_COL_R 0.20
#define CROSSTALK_LOBBY_WAVE_E_COL_G 0.55
#define CROSSTALK_LOBBY_WAVE_E_COL_B 0.95
#define CROSSTALK_LOBBY_WAVE_B_COL_R 0.95
#define CROSSTALK_LOBBY_WAVE_B_COL_G 0.55
#define CROSSTALK_LOBBY_WAVE_B_COL_B 0.20
#define CROSSTALK_NET_NAME_CAP 64
#define CROSSTALK_BAND_MARGIN 0.4
#define CROSSTALK_SAVE_ORG_WIN "Ongakken"
#define CROSSTALK_SAVE_APP_WIN "Crosstalk"
#define CROSSTALK_SAVE_ORG_UNIX "ongakken"
#define CROSSTALK_SAVE_APP_UNIX "crosstalk"
#define CROSSTALK_SAVE_SUBDIR "savefiles"
#define CROSSTALK_WSHOP_SUBDIR "wshop"
#define CROSSTALK_BOARD_SEL_ARM_MS 3000
#define CROSSTALK_BOARD_SEL_LOCK_MS 1000
#define CROSSTALK_BOARD_SEL_NEWEST_IDX 0U
#define CROSSTALK_MP_WAIT_TEXT "Connecting"
#define CROSSTALK_MP_WAIT_FRAC 0.0
#define CROSSTALK_MP_PLAYERS_MIN 2U
#define CROSSTALK_MP_PLAYERS_STEP 2U
#define CROSSTALK_SAVE_FILE "crosstalk_save.xtlk"
#define CROSSTALK_SAVE_DIR_MODE 0755
#define CROSSTALK_LOG_FILE "crosstalk.log"
#define CROSSTALK_STATUSLINE_MP_CAP 128
#define CROSSTALK_NET_ORDINAL_STEP 1
#define CROSSTALK_STATUSLINE_LOC_FMT_PLAIN "loc %0*" PRIX64
#define CROSSTALK_STATUSLINE_LOC_FMT_SCALED "loc %0*" PRIX64 " %" PRIu64 "%s+%0*" PRIX64
#define CROSSTALK_STATUSLINE_LOC_CAP 48
#define CROSSTALK_STATUSLINE_LOC_KIB_SHIFT 10
#define CROSSTALK_STATUSLINE_LOC_MIB_SHIFT 20
#define CROSSTALK_STATUSLINE_LOC_GIB_SHIFT 30
#define CROSSTALK_STATUSLINE_LOC_KIB_DIGITS 3
#define CROSSTALK_STATUSLINE_LOC_MIB_DIGITS 5
#define CROSSTALK_STATUSLINE_LOC_GIB_DIGITS 8
#define CROSSTALK_STATUSLINE_LOC_UNIT_KIB "Ki"
#define CROSSTALK_STATUSLINE_LOC_UNIT_MIB "Mi"
#define CROSSTALK_STATUSLINE_LOC_UNIT_GIB "Gi"
#define CROSSTALK_STATUSLINE_CYC_CAP 32
#define CROSSTALK_STATUSLINE_CYC_FMT_RAW "%" PRIu64 " cyc"
#define CROSSTALK_STATUSLINE_CYC_FMT_SCALED "%.2f%s cyc"
#define CROSSTALK_STATUSLINE_CYC_KILO 1000.0
#define CROSSTALK_STATUSLINE_CYC_MEGA 1000000.0
#define CROSSTALK_STATUSLINE_CYC_GIGA 1000000000.0
#define CROSSTALK_STATUSLINE_CYC_TERA 1000000000000.0
#define CROSSTALK_STATUSLINE_CYC_UNIT_KILO "k"
#define CROSSTALK_STATUSLINE_CYC_UNIT_MEGA "M"
#define CROSSTALK_STATUSLINE_CYC_UNIT_GIGA "G"
#define CROSSTALK_STATUSLINE_CYC_UNIT_TERA "T"
#define CROSSTALK_STATUSLINE_RATE_CAP 32
#define CROSSTALK_STATUSLINE_RATE_FMT_RAW "%" PRIu64 " Hz"
#define CROSSTALK_STATUSLINE_RATE_FMT_SCALED "%.2f%s Hz"
#define CROSSTALK_STATUSLINE_RATE_COMB_TEXT "0 Hz (comb)"
#define CROSSTALK_STATUSLINE_SIMCLK_CAP 32
#define CROSSTALK_STATUSLINE_SIMCLK_FMT "sim %" PRIu64 ":%02" PRIu64 ":%02" PRIu64
#define CROSSTALK_STATUSLINE_EMUCLK_FMT "emu %" PRIu64 ":%02" PRIu64 ":%02" PRIu64
#define CROSSTALK_STATUSLINE_CELL_CAP 16
#define CROSSTALK_STATUSLINE_PROG_CAP 72
#define CROSSTALK_STATUSLINE_PROG_FMT "%.23s; 0x%08" PRIX64 "; %" PRIu64 ":%02" PRIu64 ":%02" PRIu64
#define CROSSTALK_STATUSLINE_LOC_NIBBLE_DIV 4
#define CROSSTALK_STATUSLINE_CELL_STEP 1
#define CROSSTALK_WIN_PROBE_CUNT 9
#define CROSSTALK_PERF_WINDOW (5.0 * LDG_MS_PER_SEC)
#define CROSSTALK_AUTOSAVE_SUFFIX "_autosave"
#define CROSSTALK_FMT_SCALE_2DP 100
#define CROSSTALK_FMT_SCALE_3DP 1000
#define CROSSTALK_FMT_SCALE_4DP 10000
#define CROSSTALK_BOARD_PREVIEW_TIME_FMT "%Y-%m-%d %H:%M"
#define CROSSTALK_BOARD_PREVIEW_SRC_LOCAL "local"
#define CROSSTALK_BOARD_PREVIEW_SRC_WSHOP "workshop"
#define CROSSTALK_BOARD_PREVIEW_EDITORS_UNREAD 0
#define CROSSTALK_PRESENCE_BOARD_NONE 0
#define CROSSTALK_LOBBY_MODE_TEXT "party"
#define CROSSTALK_BOARD_PREVIEW_X_FRAC 0.70
#define CROSSTALK_BOARD_PREVIEW_Y_FRAC 0.30

static uint32_t crosstalk_res_dir_resolve(char *out, uint64_t out_size)
{
    char bin_dir[LDG_IO_PATH_MAX] = { 0 };
    uint32_t ret = 0;

    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_io_path_prog_get(out, out_size);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_io_path_parent_get(out, bin_dir, (uint64_t)sizeof(bin_dir));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_io_path_join(bin_dir, "res", out, out_size);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return LDG_ERR_AOK;
}

/*
static uint32_t crosstalk_install_dir_resolve(char *out, uint64_t out_size)
{
    char exe[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_io_path_prog_get(exe, (uint64_t)sizeof(exe));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_io_path_parent_get(exe, out, out_size);
}
*/

/*
static LDG_NOINLINE uint32_t crosstalk_scan_tree(crosstalk_ctx_t *ctx, const char *dir, uint32_t depth, uint64_t *out_files, uint64_t *out_hits, uint64_t *out_refused)
{
    char child[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    ldg_io_entry_t entry = LDG_STRUCT_ZERO_INIT;
    ldg_io_dir_t *handle = 0x0;
    const char *name = 0x0;
    uint32_t verdict = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || dir == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (depth >= (uint32_t)CROSSTALK_CORE_SCAN_TREE_DEPTH_MAX) { return LDG_ERR_BOUNDS; }

    ret = ldg_io_dir_open(dir, &handle);
    if (ret != LDG_ERR_AOK) { return ret; }

    while (ldg_io_dir_rd(handle, &entry) == LDG_ERR_AOK)
    {
        if (ldg_io_entry_name_get(&entry, &name) != LDG_ERR_AOK || name == 0x0) { continue; }

        if (strcmp(name, CROSSTALK_CORE_SCAN_DIR_SELF) == 0 || strcmp(name, CROSSTALK_CORE_SCAN_DIR_UP) == 0) { continue; }

        if (ldg_io_path_join(dir, name, child, (uint64_t)sizeof(child)) != LDG_ERR_AOK) { continue; }

        if (ldg_io_entry_dir_is(&entry) == LDG_TRUTH_TRUE)
        {
            if (crosstalk_scan_tree(ctx, child, depth + 0x1, out_files, out_hits, out_refused) != LDG_ERR_AOK) { *out_refused = *out_refused + 0x1; }

            continue;
        }

        ret = crosstalk_core_scan_file(&ctx->scan, child, &verdict);
        if (ret != LDG_ERR_AOK)
        {
            *out_refused = *out_refused + 0x1;

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scan: install file refused; path: %.191s; err: %u32", child, ret);

            continue;
        }

        *out_files = *out_files + 0x1;

        if (verdict == (uint32_t)CROSSTALK_CORE_SCAN_VERDICT_DETECTED)
        {
            *out_hits = *out_hits + 0x1;

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "scan: install file detected; path: %.191s", child);
        }
    }

    if (ldg_io_dir_close(handle) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scan tree dir close failed"); }

    return LDG_ERR_AOK;
}
*/

/*
static uint32_t crosstalk_install_scan(crosstalk_ctx_t *ctx, uint64_t *out_hits)
{
    char dir[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t files = 0;
    uint64_t hits = 0;
    uint64_t refused = 0;
    uint64_t span_ns = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_hits == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_hits = 0;

    if (crosstalk_core_scan_ready_is(&ctx->scan) != LDG_TRUTH_TRUE)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scan: install sweep skipped; no provider");

        return LDG_ERR_NOT_INIT;
    }

    ret = crosstalk_install_dir_resolve(dir, (uint64_t)sizeof(dir));
    if (ret != LDG_ERR_AOK) { return ret; }

    span_ns = ldg_time_epoch_ns_get();

    ret = crosstalk_scan_tree(ctx, dir, (uint32_t)CROSSTALK_CORE_SCAN_TREE_ROOT, &files, &hits, &refused);

    span_ns = ldg_time_epoch_ns_get() - span_ns;

    *out_hits = hits;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, hits != 0 ? LDG_LOGGING_LVL_ERR : LDG_LOGGING_LVL_INFO, "scan: install sweep done; dir: %.191s; files: %u64; hits: %u64; refused: %u64; ms: %u64; err: %u32", dir, files, hits, refused, span_ns / (uint64_t)LDG_NS_PER_MS, ret);

    return ret;
}
*/

static uint32_t crosstalk_data_subdir_resolve(const char *subdir, char *out, uint64_t out_size)
{
    char nxt[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    const char *seg[3] = { 0x0, 0x0, 0x0 };
    uint32_t idx = 0;
    uint8_t exists = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(subdir == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_io_path_data_get(out, out_size);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ldg_io_path_sep_get() == '\\')
    {
        seg[0] = CROSSTALK_SAVE_ORG_WIN;
        seg[1] = CROSSTALK_SAVE_APP_WIN;
    }
    else
    {
        seg[0] = CROSSTALK_SAVE_ORG_UNIX;
        seg[1] = CROSSTALK_SAVE_APP_UNIX;
    }

    seg[2] = subdir;

    for (idx = 0; idx < 3; idx = idx + 1)
    {
        ret = ldg_io_path_join(out, seg[idx], nxt, (uint64_t)sizeof(nxt));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        exists = ldg_io_path_exists_is(nxt);
        if (LDG_UNLIKELY(exists != LDG_TRUTH_TRUE && exists != LDG_TRUTH_FALSE)) { return exists; }

        if (exists == LDG_TRUTH_FALSE)
        {
            ret = ldg_io_dir_create(nxt, CROSSTALK_SAVE_DIR_MODE);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = ldg_str_rbrcpy(out, nxt, out_size);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_script_path_resolve(const crosstalk_ctx_t *ctx, const char *path, char *out, uint64_t out_size)
{
    char base[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;

    if (LDG_UNLIKELY(ctx == 0x0 || path == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ldg_io_path_absolute_is(path) == LDG_TRUTH_TRUE) { return ldg_str_rbrcpy(out, path, out_size); }

    if (ctx->eda.script.run_path[0] == LDG_STR_TERM) { return ldg_str_rbrcpy(out, path, out_size); }

    if (ldg_io_path_parent_get(ctx->eda.script.run_path, base, (uint64_t)sizeof(base)) != LDG_ERR_AOK) { return ldg_str_rbrcpy(out, path, out_size); }

    if (base[0] == LDG_STR_TERM) { return ldg_str_rbrcpy(out, path, out_size); }

    return ldg_io_path_join(base, path, out, out_size);
}

static uint32_t crosstalk_save_path_resolve(char *out, uint64_t out_size)
{
    char nxt[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_data_subdir_resolve(CROSSTALK_SAVE_SUBDIR, out, out_size);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_io_path_join(out, CROSSTALK_SAVE_FILE, nxt, (uint64_t)sizeof(nxt));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_str_rbrcpy(out, nxt, out_size);
}

static uint32_t crosstalk_persona_resolve(crosstalk_ctx_t *ctx, uint64_t steam_id, char *out, uint64_t out_size)
{
    const ldg_net_lobby_member_t *member = 0x0;
    const char *name = 0x0;
    uint32_t cunt = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    out[0] = LDG_STR_TERM;

    if (steam_id == ldg_net_steamworks_local_steam_id_get())
    {
        name = ldg_net_steamworks_local_name_get();
        if (name == 0x0) { return LDG_ERR_NOT_FOUND; }

        return ldg_str_rbrcpy(out, name, out_size);
    }

    cunt = crosstalk_net_steam_lobby_member_cunt_get(&ctx->steam_net);

    for (idx = 0; idx < cunt; idx = idx + 1)
    {
        member = crosstalk_net_steam_lobby_member_get(&ctx->steam_net, idx);
        if (member == 0x0) { continue; }

        if (member->user_id == steam_id) { return ldg_str_rbrcpy(out, member->name, out_size); }
    }

    cunt = crosstalk_net_steam_client_cunt_get(&ctx->steam_net);

    for (idx = 0; idx < cunt; idx = idx + 1)
    {
        if (ldg_net_steamworks_client_steam_id_get(&ctx->net, idx) != steam_id) { continue; }

        name = crosstalk_net_steam_client_name_get(&ctx->steam_net, idx);
        if (name == 0x0) { continue; }

        return ldg_str_rbrcpy(out, name, out_size);
    }

    name = ldg_net_steamworks_friend_name_get(steam_id);
    if (name != 0x0 && name[0] != LDG_STR_TERM) { return ldg_str_rbrcpy(out, name, out_size); }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t crosstalk_lobby_items_build(crosstalk_ctx_t *ctx)
{
    const ldg_net_lobby_info_t *info = 0x0;
    const ldg_net_srv_browser_entry_t *lan = 0x0;
    const char *sep = 0x0;
    uint64_t used = 0;
    uint64_t room = 0;
    uint32_t cunt = 0;
    uint32_t idx = 0;
    int32_t written = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->lobby_items[0] = LDG_STR_TERM;

    if (ctx->mp_transport_idx == (uint32_t)CROSSTALK_NET_TRANSPORT_LAN)
    {
        cunt = crosstalk_net_steam_lan_list_cunt_get(&ctx->steam_net);
        for (idx = 0; idx < cunt; idx = idx + 1)
        {
            lan = crosstalk_net_steam_lan_list_get(&ctx->steam_net, idx);
            if (lan == 0x0) { continue; }

            sep = (idx > 0) ? ";" : "";
            room = (uint64_t)sizeof(ctx->lobby_items) - used;
            if (room == 0) { break; }

            written = snprintf(&ctx->lobby_items[used], room, "%.1s%.63s  %.31s  %" PRIu32 "/%" PRIu32 "  %u.%u.%u.%u:%u", sep, lan->name, lan->map, (uint32_t)lan->client_cunt, (uint32_t)lan->client_max, (lan->ip >> 24) & 0xFFU, (lan->ip >> 16) & 0xFFU, (lan->ip >> 8) & 0xFFU, lan->ip & 0xFFU, (uint32_t)lan->port);
            if (written <= 0) { break; }

            if ((uint64_t)written >= room)
            {
                ctx->lobby_items[used] = LDG_STR_TERM;
                break;
            }

            used = used + (uint64_t)written;
        }

        return LDG_ERR_AOK;
    }

    cunt = crosstalk_net_steam_lobby_list_cunt_get(&ctx->steam_net);

    for (idx = 0; idx < cunt; idx = idx + 1)
    {
        info = crosstalk_net_steam_lobby_list_get(&ctx->steam_net, idx);
        if (info == 0x0) { continue; }

        sep = (idx > 0) ? ";" : "";
        room = (uint64_t)sizeof(ctx->lobby_items) - used;

        if (room == 0) { break; }

        written = snprintf(&ctx->lobby_items[used], room, "%.1s%.63s  %.63s  %" PRIu32 "/%" PRIu32, sep, info->host_name, info->map_name, (uint32_t)info->player_cunt, (uint32_t)info->player_max);
        if (written <= 0) { break; }

        if ((uint64_t)written >= room)
        {
            ctx->lobby_items[used] = LDG_STR_TERM;
            break;
        }

        used = used + (uint64_t)written;
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_lan_member_items_build(crosstalk_ctx_t *ctx)
{
    const char *name = 0x0;
    const char *sep = 0x0;
    uint64_t used = 0;
    uint64_t room = 0;
    uint32_t client_cunt = 0;
    uint32_t idx = 0;
    int32_t written = 0;
    uint8_t lan_sesh = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->member_items[0] = LDG_STR_TERM;
    lan_sesh = crosstalk_net_steam_lan_sesh_get(&ctx->steam_net);
    name = ldg_net_steamworks_local_name_get();
    if (name == 0x0 || name[0] == LDG_STR_TERM) { name = "you"; }

    room = (uint64_t)sizeof(ctx->member_items);
    written = snprintf(ctx->member_items, room, "%.63s  %s", name, (lan_sesh == (uint8_t)CROSSTALK_NET_LAN_SESH_HOSTING) ? "host" : "joined");
    if (written > 0 && (uint64_t)written < room) { used = (uint64_t)written; }
    else { ctx->member_items[0] = LDG_STR_TERM; return LDG_ERR_AOK; }

    client_cunt = crosstalk_net_steam_client_cunt_get(&ctx->steam_net);
    for (idx = 0; idx < client_cunt; idx = idx + 1)
    {
        name = crosstalk_net_steam_client_name_get(&ctx->steam_net, idx);
        if (name == 0x0 || name[0] == LDG_STR_TERM) { name = "player"; }

        sep = ";";
        room = (uint64_t)sizeof(ctx->member_items) - used;
        if (room == 0) { break; }

        written = snprintf(&ctx->member_items[used], room, "%.1s%.63s", sep, name);
        if (written <= 0) { break; }

        if ((uint64_t)written >= room)
        {
            ctx->member_items[used] = LDG_STR_TERM;
            break;
        }

        used = used + (uint64_t)written;
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_member_items_build(crosstalk_ctx_t *ctx)
{
    const ldg_net_lobby_member_t *member = 0x0;
    const char *sep = 0x0;
    const char *rdy_text = 0x0;
    uint64_t used = 0;
    uint64_t room = 0;
    uint32_t cunt = 0;
    uint32_t idx = 0;
    int32_t written = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (crosstalk_net_steam_lan_sesh_get(&ctx->steam_net) != (uint8_t)CROSSTALK_NET_LAN_SESH_NONE) { return crosstalk_lan_member_items_build(ctx); }

    ctx->member_items[0] = LDG_STR_TERM;
    cunt = crosstalk_net_steam_lobby_member_cunt_get(&ctx->steam_net);

    for (idx = 0; idx < cunt; idx = idx + 1)
    {
        member = crosstalk_net_steam_lobby_member_get(&ctx->steam_net, idx);
        if (member == 0x0) { continue; }

        sep = (idx > 0) ? ";" : "";
        rdy_text = (member->team != 0) ? "BUSY" : ((member->rdy != 0) ? "ready" : "waiting");
        room = (uint64_t)sizeof(ctx->member_items) - used;

        if (room == 0) { break; }

        written = snprintf(&ctx->member_items[used], room, "%.1s%.63s; %.7s", sep, member->name, rdy_text);
        if (written <= 0) { break; }

        if ((uint64_t)written >= room)
        {
            ctx->member_items[used] = LDG_STR_TERM;
            break;
        }

        used = used + (uint64_t)written;
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_wshop_sync(crosstalk_ctx_t *ctx)
{
    const ldg_net_steamworks_wshop_item_t *item = 0x0;
    uint32_t type_arr[CROSSTALK_WSHOP_ITEM_SCAN_CUNT] = { CROSSTALK_NET_WSHOP_TYPE_CORE, CROSSTALK_NET_WSHOP_TYPE_SOFTWARE };
    uint32_t wshop_cunt = 0;
    uint32_t type_idx = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_net_wshop_refresh(&ctx->wshop);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: wshop sync refresh failed; err: %u32", ret); }

    for (type_idx = 0; type_idx < CROSSTALK_WSHOP_ITEM_SCAN_CUNT; type_idx = type_idx + 1)
    {
        wshop_cunt = crosstalk_net_wshop_item_cunt_get(&ctx->wshop, type_arr[type_idx]);

        for (idx = 0; idx < wshop_cunt; idx = idx + 1)
        {
            item = crosstalk_net_wshop_item_get(&ctx->wshop, type_arr[type_idx], idx);
            if (item == 0x0) { continue; }

            if (crosstalk_net_wshop_item_rdy_is(item) == LDG_TRUTH_TRUE) { continue; }

            ret = crosstalk_net_wshop_item_pull(&ctx->wshop, item);
            if (ret != LDG_ERR_AGAIN) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: wshop sync pull failed; err: %u32; id: %u64", ret, item->file_id); }
        }
    }

    return crosstalk_wshop_list_build(&ctx->wshop_list, &ctx->wshop);
}

static uint32_t crosstalk_board_sel_refresh(crosstalk_ctx_t *ctx)
{
    const ldg_net_steamworks_wshop_item_t *item = 0x0;
    uint32_t wshop_cunt = 0;
    uint32_t type_idx = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_game_board_init(&ctx->board_sel);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->save_dir[0] != LDG_STR_TERM)
    {
        ret = crosstalk_game_board_scan(&ctx->board_sel, ctx->save_dir, CROSSTALK_GAME_BOARD_SRC_LOCAL, 0x0);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: board scan failed; err: %u32", ret); }
    }

    if (ctx->board_sel_wshop_on != LDG_TRUTH_FALSE)
    {
        if (ctx->wshop_dir[0] != LDG_STR_TERM)
        {
            ret = crosstalk_game_board_scan(&ctx->board_sel, ctx->wshop_dir, CROSSTALK_GAME_BOARD_SRC_WSHOP, 0x0);
            if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: wshop scan failed; err: %u32", ret); }
        }

        ret = crosstalk_net_wshop_refresh(&ctx->wshop);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: wshop refresh failed; err: %u32", ret); }

        for (type_idx = 0; type_idx < CROSSTALK_NET_WSHOP_TYPE_CUNT; type_idx = type_idx + 1)
        {
            wshop_cunt = crosstalk_net_wshop_item_cunt_get(&ctx->wshop, type_idx);
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: wshop enum; type: %u32; items: %u32", type_idx, wshop_cunt);

            for (idx = 0; idx < wshop_cunt; idx = idx + 1)
            {
                item = crosstalk_net_wshop_item_get(&ctx->wshop, type_idx, idx);
                if (item == 0x0) { continue; }

                if (crosstalk_net_wshop_item_rdy_is(item) != LDG_TRUTH_TRUE)
                {
                    ret = crosstalk_net_wshop_item_pull(&ctx->wshop, item);
                    if (ret != LDG_ERR_AGAIN) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: wshop pull failed; err: %u32; id: %u64", ret, item->file_id); }

                    continue;
                }

                if (type_idx != CROSSTALK_NET_WSHOP_TYPE_BOARD) { continue; }

                ret = crosstalk_game_board_scan(&ctx->board_sel, item->path, CROSSTALK_GAME_BOARD_SRC_WSHOP, item->tag);
                if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: wshop item scan failed; err: %u32; id: %u64", ret, item->file_id); }
            }
        }

        ret = crosstalk_wshop_list_build(&ctx->wshop_list, &ctx->wshop);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: wshop list build failed; err: %u32", ret); }
    }

    ret = crosstalk_game_board_sort(&ctx->board_sel);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->board_sel_active = (uint32_t)CROSSTALK_MENU_SEL_FIRST;
    ctx->menu_nav.row = (uint32_t)CROSSTALK_MENU_SEL_FIRST;
    ctx->menu_nav.scroll = (uint32_t)CROSSTALK_MENU_SEL_FIRST;
    ctx->board_sel_confirm_row = UINT32_MAX;
    ctx->board_preview_row = UINT32_MAX;
    ctx->board_preview_ok = LDG_TRUTH_FALSE;

    return crosstalk_game_board_items_build(&ctx->board_sel);
}

static uint32_t crosstalk_newproject_list_build(crosstalk_ctx_t *ctx)
{
    char safe_name[CROSSTALK_GAME_BOARD_NAME_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t used = 0;
    uint32_t idx = 0;
    uint32_t col = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->newproject_items[0] = LDG_STR_TERM;
    ctx->newproject_pick_cunt = 0;
    ctx->newproject_pick = CROSSTALK_CORE_NEWPROJECT_PICK_NEW;

    ret = ldg_str_rbrcpy(ctx->newproject_items, CROSSTALK_CORE_NEWPROJECT_PICK_NEW_TEXT, (uint64_t)sizeof(ctx->newproject_items));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    used = strlen(ctx->newproject_items);

    for (idx = 0; idx < ctx->board_sel.cunt; idx = idx + 0x1)
    {
        if (ctx->board_sel.entry[idx].src != CROSSTALK_GAME_BOARD_SRC_LOCAL) { continue; }

        if (ctx->newproject_pick_cunt >= (uint32_t)CROSSTALK_GAME_BOARD_CAP) { break; }

        if (ldg_str_rbrcpy(safe_name, ctx->board_sel.entry[idx].name, (uint64_t)sizeof(safe_name)) != LDG_ERR_AOK) { continue; }

        for (col = 0; safe_name[col] != LDG_STR_TERM; col = col + 0x1) { if (safe_name[col] == CROSSTALK_CORE_NEWPROJECT_SEP_CHAR) { safe_name[col] = CROSSTALK_CORE_NEWPROJECT_SEP_SUB; } }

        if (used + strlen(safe_name) + LDG_STR_TERM_SIZE >= (uint64_t)sizeof(ctx->newproject_items)) { break; }

        if (ldg_str_rbrcpy(&ctx->newproject_items[used], CROSSTALK_CORE_NEWPROJECT_SEP, (uint64_t)sizeof(ctx->newproject_items) - used) != LDG_ERR_AOK) { break; }

        used = used + LDG_STR_TERM_SIZE;
        if (ldg_str_rbrcpy(&ctx->newproject_items[used], safe_name, (uint64_t)sizeof(ctx->newproject_items) - used) != LDG_ERR_AOK) { break; }

        used = used + strlen(safe_name);
        ctx->newproject_row[ctx->newproject_pick_cunt] = idx;
        ctx->newproject_pick_cunt = ctx->newproject_pick_cunt + 0x1;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: newproject list; existing: %u32", ctx->newproject_pick_cunt);
    return LDG_ERR_AOK;
}

static uint32_t settings_preload(crosstalk_ctx_t *ctx)
{
    char path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint8_t present = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->save_dir[0] == LDG_STR_TERM) { return LDG_ERR_AOK; }

    ret = crosstalk_game_board_init(&ctx->board_sel);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_game_board_scan(&ctx->board_sel, ctx->save_dir, CROSSTALK_GAME_BOARD_SRC_LOCAL, 0x0);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_game_board_sort(&ctx->board_sel);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->board_sel.cunt == 0) { return LDG_ERR_AOK; }

    ret = crosstalk_game_board_path_build(&ctx->board_sel, CROSSTALK_BOARD_SEL_NEWEST_IDX, path, (uint64_t)sizeof(path));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_game_save_settings_rd(path, &ctx->game.settings, &present);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: settings preload; path: %s; err: %u32; present: %u32", path, ret, (uint32_t)present);

    return ret;
}

static uint32_t lobby_join_enter(crosstalk_ctx_t *ctx, uint64_t lobby_id)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_net_steam_lobby_join(&ctx->steam_net, lobby_id);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->game.net_join_phase = CROSSTALK_NET_JOIN_PHASE_CONN;
    ctx->game.screen = CROSSTALK_GAME_SCREEN_MP;
    ctx->mp_rdy = LDG_TRUTH_FALSE;
    ctx->snap_req_sent = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

static uint8_t extras_audio_ext_is(const char *name)
{
    static const char *ext_list[CROSSTALK_CORE_EXTRAS_EXT_CUNT] = { ".wav" };
    char ext[CROSSTALK_CORE_EXTRAS_NAME_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t idx = 0;

    if (ldg_io_path_ext_get(name, ext, (uint64_t)sizeof(ext)) != LDG_ERR_AOK) { return LDG_TRUTH_FALSE; }

    for (idx = 0; idx < CROSSTALK_CORE_EXTRAS_EXT_CUNT; idx = idx + 1)
    {
        if (strcmp(ext, ext_list[idx]) == 0) { return LDG_TRUTH_TRUE; }

        if (strcmp(ext, ext_list[idx] + LDG_STR_TERM_SIZE) == 0) { return LDG_TRUTH_TRUE; }
    }

    return LDG_TRUTH_FALSE;
}

static uint32_t crosstalk_credits_refresh(crosstalk_ctx_t *ctx)
{
    char path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    ldg_io_file_t *fh = 0x0;
    uint64_t got = 0;
    uint32_t pos = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->credits_cunt = 0;
    ctx->credits_items[0] = LDG_STR_TERM;

    ret = ldg_io_path_join(ctx->res_dir, CROSSTALK_CORE_CREDITS_FILE_REL, path, (uint64_t)sizeof(path));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, 0x0, &fh);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_rd(fh, (ldg_byte_t *)ctx->credits_items, (uint64_t)CROSSTALK_CORE_CREDITS_ITEMS_CAP - LDG_STR_TERM_SIZE, &got);

    if (ldg_io_file_close(fh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "credits close failed"); }

    if (ret != LDG_ERR_AOK) { ctx->credits_items[0] = LDG_STR_TERM; return ret; }

    ctx->credits_items[got] = LDG_STR_TERM;

    for (pos = 0; pos < (uint32_t)got; pos = pos + 0x1)
    {
        if (ctx->credits_items[pos] == '\r') { ctx->credits_items[pos] = ' '; }

        if (ctx->credits_items[pos] == CROSSTALK_CORE_CREDITS_SEP) { ctx->credits_items[pos] = ','; }

        if (ctx->credits_items[pos] != '\n') { continue; }

        ctx->credits_items[pos] = CROSSTALK_CORE_CREDITS_SEP;
        ctx->credits_cunt = ctx->credits_cunt + 0x1;
    }

    if (got != 0) { ctx->credits_cunt = ctx->credits_cunt + 0x1; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "credits: loaded; path: %.255s; bytes: %u64; lines: %u32", path, got, ctx->credits_cunt);

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_extras_title_get(const char *dir, const char *name, char *out, uint64_t out_size)
{
    char audio_path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char vtt_path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char vtt_hd[CROSSTALK_CORE_EXTRAS_VTT_HD_CAP] = LDG_ARR_ZERO_INIT;
    ldg_io_file_t *fh = 0x0;
    const char *line = 0x0;
    const char *title = 0x0;
    uint64_t got = 0;
    uint64_t title_len = 0;
    uint64_t pos = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(dir == 0x0 || name == 0x0 || out == 0x0 || out_size == 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    out[0] = LDG_STR_TERM;

    ret = ldg_io_path_join(dir, name, audio_path, (uint64_t)sizeof(audio_path));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_game_board_ext_swap(audio_path, CROSSTALK_GAME_CAPTION_VTT_EXT, vtt_path, (uint64_t)sizeof(vtt_path));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_open(vtt_path, LDG_IO_RD_ONLY, 0x0, &fh);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_rd(fh, (ldg_byte_t *)vtt_hd, (uint64_t)sizeof(vtt_hd) - LDG_STR_TERM_SIZE, &got);

    if (ldg_io_file_close(fh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "extras vtt close failed"); }

    if (ret != LDG_ERR_AOK) { return ret; }

    vtt_hd[got] = LDG_STR_TERM;
    line = vtt_hd;

    while (line[0] != LDG_STR_TERM)
    {
        if (strncmp(line, CROSSTALK_CORE_EXTRAS_TITLE_TAG, strlen(CROSSTALK_CORE_EXTRAS_TITLE_TAG)) == 0)
        {
            title = line + strlen(CROSSTALK_CORE_EXTRAS_TITLE_TAG);
            break;
        }

        while (line[0] != LDG_STR_TERM && line[0] != '\n') { line = line + LDG_STR_TERM_SIZE; }

        if (line[0] == '\n') { line = line + LDG_STR_TERM_SIZE; }
    }

    if (title == 0x0) { return LDG_ERR_NOT_FOUND; }

    while (ldg_char_space_is(title[0]) == LDG_TRUTH_TRUE) { title = title + LDG_STR_TERM_SIZE; }

    while (title[title_len] != LDG_STR_TERM && title[title_len] != '\n' && title[title_len] != '\r') { title_len = title_len + LDG_STR_TERM_SIZE; }

    while (title_len > 0 && ldg_char_space_is(title[title_len - LDG_STR_TERM_SIZE]) == LDG_TRUTH_TRUE) { title_len = title_len - LDG_STR_TERM_SIZE; }

    if (title_len == 0) { return LDG_ERR_EMPTY; }

    if (title_len >= out_size) { title_len = out_size - LDG_STR_TERM_SIZE; }

    for (pos = 0; pos < title_len; pos = pos + LDG_STR_TERM_SIZE) { out[pos] = title[pos] == CROSSTALK_CORE_EXTRAS_SEP ? ',' : title[pos]; }

    out[title_len] = LDG_STR_TERM;
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_extras_refresh(crosstalk_ctx_t *ctx)
{
    char dir[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    ldg_io_dir_t *handle = 0x0;
    ldg_io_entry_t entry = LDG_STRUCT_ZERO_INIT;
    const char *name = 0x0;
    uint32_t pos = 0;
    uint32_t ret = 0;
    int32_t written = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->extras_cunt = 0;
    ctx->extras_items[0] = LDG_STR_TERM;

    ret = ldg_io_path_join(ctx->res_dir, CROSSTALK_CORE_EXTRAS_DIR_REL, dir, (uint64_t)sizeof(dir));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_dir_open(dir, &handle);
    if (ret != LDG_ERR_AOK) { return ret; }

    while (ldg_io_dir_rd(handle, &entry) == LDG_ERR_AOK)
    {
        if (ctx->extras_cunt >= CROSSTALK_CORE_EXTRAS_CAP) { break; }

        if (ldg_io_entry_dir_is(&entry) == LDG_TRUTH_TRUE) { continue; }

        if (ldg_io_entry_name_get(&entry, &name) != LDG_ERR_AOK || name == 0x0) { continue; }

        if (extras_audio_ext_is(name) != LDG_TRUTH_TRUE) { continue; }

        {
            char label[CROSSTALK_CORE_EXTRAS_NAME_CAP] = LDG_ARR_ZERO_INIT;
            char ext[CROSSTALK_CORE_EXTRAS_NAME_CAP] = LDG_ARR_ZERO_INIT;
            uint64_t label_len = 0;
            uint64_t ext_len = 0;

            ret = ldg_str_rbrcpy(label, name, (uint64_t)sizeof(label));
            if (ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC) { break; }

            if (ldg_io_path_ext_get(label, ext, (uint64_t)sizeof(ext)) == LDG_ERR_AOK)
            {
                label_len = (uint64_t)strlen(label);
                ext_len = (uint64_t)strlen(ext);

                if (ext_len > 0 && label_len > ext_len) { label[label_len - ext_len] = LDG_STR_TERM; }

                label_len = (uint64_t)strlen(label);

                if (label_len > 0 && label[label_len - LDG_STR_TERM_SIZE] == '.') { label[label_len - LDG_STR_TERM_SIZE] = LDG_STR_TERM; }
            }

            {
                char title[CROSSTALK_CORE_EXTRAS_NAME_CAP] = LDG_ARR_ZERO_INIT;

                if (crosstalk_extras_title_get(dir, name, title, (uint64_t)sizeof(title)) == LDG_ERR_AOK) { ret = ldg_str_rbrcpy(label, title, (uint64_t)sizeof(label)); }
                else { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: extras title absent; file: %.95s", name); }
            }

            written = snprintf(ctx->extras_items + pos, (uint64_t)(CROSSTALK_CORE_EXTRAS_ITEMS_CAP - pos), "%.1s%.95s", ctx->extras_cunt == 0 ? "" : ";", label);
        }

        if (written < 0) { break; }

        if ((uint32_t)written >= CROSSTALK_CORE_EXTRAS_ITEMS_CAP - pos) { break; }

        pos = pos + (uint32_t)written;
        ret = ldg_str_rbrcpy(ctx->extras_file[ctx->extras_cunt], name, (uint64_t)sizeof(ctx->extras_file[ctx->extras_cunt]));
        if (ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC) { break; }

        ctx->extras_cunt = ctx->extras_cunt + CROSSTALK_CORE_EXTRAS_STEP;
    }

    ret = ldg_io_dir_close(handle);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: extras scan; dir: %.255s; cunt: %u32; err: %u32", dir, ctx->extras_cunt, ret);

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_ost_refresh(crosstalk_ctx_t *ctx)
{
    char bin_path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char base_dir[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char dir[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    ldg_io_dir_t *handle = 0x0;
    ldg_io_entry_t entry = LDG_STRUCT_ZERO_INIT;
    const char *name = 0x0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->ost_cunt = 0;

    ret = ldg_io_path_prog_get(bin_path, (uint64_t)sizeof(bin_path));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_path_parent_get(bin_path, base_dir, (uint64_t)sizeof(base_dir));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_path_parent_get(base_dir, bin_path, (uint64_t)sizeof(bin_path));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_path_parent_get(bin_path, base_dir, (uint64_t)sizeof(base_dir));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_path_join(base_dir, CROSSTALK_CORE_OST_DIR_REL, dir, (uint64_t)sizeof(dir));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_dir_open(dir, &handle);

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: ost scan; dir: %.255s; open failed; err: %u32", dir, ret);
        return ret;
    }

    while (ldg_io_dir_rd(handle, &entry) == LDG_ERR_AOK)
    {
        if (ctx->ost_cunt >= CROSSTALK_CORE_OST_TRACK_CAP) { break; }

        if (ldg_io_entry_dir_is(&entry) == LDG_TRUTH_TRUE) { continue; }

        if (ldg_io_entry_name_get(&entry, &name) != LDG_ERR_AOK || name == 0x0) { continue; }

        if (extras_audio_ext_is(name) != LDG_TRUTH_TRUE) { continue; }

        if (ldg_io_path_join(dir, name, ctx->ost_path[ctx->ost_cunt], (uint64_t)sizeof(ctx->ost_path[0])) != LDG_ERR_AOK) { continue; }

        ctx->ost_cunt = ctx->ost_cunt + CROSSTALK_CORE_EXTRAS_STEP;
    }

    ret = ldg_io_dir_close(handle);

    for (idx = CROSSTALK_CORE_EXTRAS_STEP; idx < ctx->ost_cunt; idx = idx + CROSSTALK_CORE_EXTRAS_STEP)
    {
        char hold[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
        uint32_t back = idx;

        snprintf(hold, sizeof(hold), "%.4095s", ctx->ost_path[idx]);

        while (back > 0 && strcmp(ctx->ost_path[back - CROSSTALK_CORE_EXTRAS_STEP], hold) > 0)
        {
            snprintf(ctx->ost_path[back], sizeof(ctx->ost_path[back]), "%.4095s", ctx->ost_path[back - CROSSTALK_CORE_EXTRAS_STEP]);
            back = back - CROSSTALK_CORE_EXTRAS_STEP;
        }

        snprintf(ctx->ost_path[back], sizeof(ctx->ost_path[back]), "%.4095s", hold);
    }

    for (idx = 0; idx < ctx->ost_cunt; idx = idx + CROSSTALK_CORE_EXTRAS_STEP) { ctx->ost_ptr[idx] = ctx->ost_path[idx]; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: ost scan; dir: %.255s; cunt: %u32; err: %u32", dir, ctx->ost_cunt, ret);

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_ost_start(crosstalk_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (crosstalk_game_music_native_is(&ctx->music) != LDG_TRUTH_TRUE) { return LDG_ERR_UNSUPPORTED; }

    if (ctx->ost_cunt == 0) { ret = crosstalk_ost_refresh(ctx); }

    if (ctx->ost_cunt == 0)
    {
        uint64_t got = 0;
        uint64_t total = 0;

        if (ctx->soundtrack_owned != LDG_TRUTH_TRUE)
        {
            ret = ldg_net_steamworks_overlay_store_activate(CROSSTALK_CORE_SOUNDTRACK_APP_ID, LDG_NET_STEAMWORKS_STORE_FLAG_NONE);
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "crosstalk: ost store overlay; app: %u32; err: %u32", (uint32_t)CROSSTALK_CORE_SOUNDTRACK_APP_ID, ret);

            return LDG_ERR_DENIED;
        }

        ret = ldg_net_steamworks_dlc_progress_get(CROSSTALK_CORE_SOUNDTRACK_APP_ID, &got, &total);
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: ost dlc state; app: %u32; installed: %u32; got: %u64; total: %u64; err: %u32", (uint32_t)CROSSTALK_CORE_SOUNDTRACK_APP_ID, (uint32_t)ldg_net_steamworks_dlc_installed_is(CROSSTALK_CORE_SOUNDTRACK_APP_ID), got, total, ret);

        if (ldg_net_steamworks_dlc_installed_is(CROSSTALK_CORE_SOUNDTRACK_APP_ID) == LDG_TRUTH_TRUE)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: ost dlc reports installed but no tracks at the scan path");

            return LDG_ERR_NOT_FOUND;
        }

        if (total > 0) { return LDG_ERR_AGAIN; }

        ret = ldg_net_steamworks_dlc_install(CROSSTALK_CORE_SOUNDTRACK_APP_ID);
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "crosstalk: ost dlc install req; app: %u32; err: %u32", (uint32_t)CROSSTALK_CORE_SOUNDTRACK_APP_ID, ret);

        return LDG_ERR_AGAIN;
    }

    if (ctx->soundtrack_owned != LDG_TRUTH_TRUE) { return LDG_ERR_DENIED; }

    ret = crosstalk_game_music_list_play(&ctx->music, ctx->ost_ptr, ctx->ost_cunt, (uint8_t)CROSSTALK_GAME_MUSIC_KIND_TRACKS);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "crosstalk: ost play; cunt: %u32; err: %u32", ctx->ost_cunt, ret);

    if (ret == LDG_ERR_AOK) { ctx->ost_started = LDG_TRUTH_TRUE; }

    return ret;
}

static uint32_t crosstalk_extras_play(crosstalk_ctx_t *ctx, uint32_t row)
{
    char full[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char dir[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (row >= ctx->extras_cunt) { return LDG_ERR_BOUNDS; }

    ret = ldg_io_path_join(ctx->res_dir, CROSSTALK_CORE_EXTRAS_DIR_REL, dir, (uint64_t)sizeof(dir));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_path_join(dir, ctx->extras_file[row], full, (uint64_t)sizeof(full));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_game_music_file_play(&ctx->music, full, (uint8_t)CROSSTALK_GAME_MUSIC_KIND_COMMENTARY);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "crosstalk: extras play; file: %.95s; err: %u32", ctx->extras_file[row], ret);
    if (ret != LDG_ERR_AOK) { return ret; }

    {
        char vtt[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;

        ret = crosstalk_game_board_ext_swap(full, CROSSTALK_GAME_CAPTION_VTT_EXT, vtt, (uint64_t)sizeof(vtt));
        if (ret == LDG_ERR_AOK) { ret = crosstalk_game_caption_vtt_ld(&ctx->game.caption, vtt); }

        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: extras captions absent; err: %u32", ret); }
    }

    ctx->extras_playing = LDG_TRUTH_TRUE;
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_ident_path_resolve(char *out, uint64_t out_size)
{
    char log_path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char dir[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    ret = crosstalk_core_log_path_resolve(log_path, (uint64_t)sizeof(log_path));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_path_parent_get(log_path, dir, (uint64_t)sizeof(dir));
    if (ret != LDG_ERR_AOK) { return ret; }

    return ldg_io_path_join(dir, CROSSTALK_CORE_IDENT_FILE, out, out_size);
}

static uint32_t crosstalk_ident_cache_ld(crosstalk_ctx_t *ctx)
{
    char path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    ldg_io_file_t *file = 0x0;
    uint64_t rd = 0;
    uint32_t ret = 0;

    ctx->ident_cunt = 0;

    ret = crosstalk_ident_path_resolve(path, (uint64_t)sizeof(path));
    if (ret != LDG_ERR_AOK) { return ret; }

    if (ldg_io_path_exists_is(path) != LDG_TRUTH_TRUE) { return LDG_ERR_EMPTY; }

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, CROSSTALK_CORE_IDENT_MODE, &file);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_rd(file, &ctx->ident[0], (uint64_t)sizeof(ctx->ident), &rd);
    ldg_io_file_close(file);

    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->ident_cunt = (uint32_t)(rd / sizeof(crosstalk_net_wire_ident_t));
    if (ctx->ident_cunt > (uint32_t)CROSSTALK_CORE_IDENT_CAP) { ctx->ident_cunt = (uint32_t)CROSSTALK_CORE_IDENT_CAP; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: ident cache loaded; cunt: %u32", ctx->ident_cunt);

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_ident_cache_wr(crosstalk_ctx_t *ctx)
{
    char path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    ldg_io_file_t *file = 0x0;
    uint64_t wrote = 0;
    uint32_t ret = 0;

    if (ctx->ident_cunt == 0) { return LDG_ERR_EMPTY; }

    ret = crosstalk_ident_path_resolve(path, (uint64_t)sizeof(path));
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_open(path, LDG_IO_WR_ONLY | LDG_IO_CREATE | LDG_IO_TRUNCATE, CROSSTALK_CORE_IDENT_MODE, &file);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_wr(file, &ctx->ident[0], (uint64_t)ctx->ident_cunt * sizeof(crosstalk_net_wire_ident_t), &wrote);
    ldg_io_file_close(file);

    return ret;
}

uint32_t crosstalk_core_runtime_path_resolve(const char *file, char *out, uint64_t out_size)
{
    char nxt[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    const char *seg[2] = { 0x0, 0x0 };
    uint32_t idx = 0;
    uint8_t exists = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(file == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_io_path_data_get(out, out_size);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ldg_io_path_sep_get() == '\\')
    {
        seg[0] = CROSSTALK_SAVE_ORG_WIN;
        seg[1] = CROSSTALK_SAVE_APP_WIN;
    }
    else
    {
        seg[0] = CROSSTALK_SAVE_ORG_UNIX;
        seg[1] = CROSSTALK_SAVE_APP_UNIX;
    }

    for (idx = 0; idx < 2; idx = idx + 1)
    {
        ret = ldg_io_path_join(out, seg[idx], nxt, (uint64_t)sizeof(nxt));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        exists = ldg_io_path_exists_is(nxt);
        if (LDG_UNLIKELY(exists != LDG_TRUTH_TRUE && exists != LDG_TRUTH_FALSE)) { return exists; }

        if (exists == LDG_TRUTH_FALSE)
        {
            ret = ldg_io_dir_create(nxt, CROSSTALK_SAVE_DIR_MODE);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = ldg_str_rbrcpy(out, nxt, out_size);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = ldg_io_path_join(out, file, nxt, (uint64_t)sizeof(nxt));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_str_rbrcpy(out, nxt, out_size);
}

uint32_t crosstalk_core_log_path_resolve(char *out, uint64_t out_size)
{
    return crosstalk_core_runtime_path_resolve(CROSSTALK_LOG_FILE, out, out_size);
}

uint32_t crosstalk_core_launch_app_id_get(void)
{
    const char *env_name[2] = { CROSSTALK_CORE_STEAM_APP_ID_ENV, CROSSTALK_CORE_STEAM_GAME_ID_ENV };
    char text[CROSSTALK_CORE_STEAM_APP_ID_CAP] = LDG_ARR_ZERO_INIT;
    dword_t parsed = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    for (idx = 0; idx < 2; idx = idx + 1)
    {
        text[0] = LDG_STR_TERM;

        ret = ldg_sys_env_get(env_name[idx], text, (uint64_t)sizeof(text));
        if (ret != LDG_ERR_AOK || text[0] == LDG_STR_TERM) { continue; }

        ret = ldg_str_to_dec(text, &parsed);
        if (ret != LDG_ERR_AOK || parsed == 0) { continue; }

        return (uint32_t)parsed;
    }

    return UINT32_MAX;
}

uint8_t crosstalk_core_soundtrack_owned_is(const crosstalk_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->soundtrack_owned;
}

#if 0

#endif

static uint8_t crosstalk_text_field_is(const crosstalk_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (crosstalk_core_cmdline_open_is(&ctx->cmdline) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    if (crosstalk_ui_window_focused_is(&ctx->win_nav, (uint64_t)CROSSTALK_GAME_CHAT_WINDOW_ID) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    if (crosstalk_ui_window_focused_is(&ctx->win_nav, (uint64_t)CROSSTALK_EDA_UI_WSHOP_PUBLISH_WINDOW_ID) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t crosstalk_sesh_joined_is(const crosstalk_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->steam_net.sesh_live != 0) { return LDG_TRUTH_TRUE; }

    if (crosstalk_net_steam_lobby_id_get(&ctx->steam_net) != 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static void crosstalk_sesh_leave(crosstalk_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: session leave; remote: %u32; busy: %u32; rdy: %u32", (uint32_t)ctx->remote_sesh, (uint32_t)ctx->mp_busy, (uint32_t)ctx->mp_rdy);

    crosstalk_net_steam_lobby_leave(&ctx->steam_net);
    crosstalk_net_steam_sesh_stop(&ctx->steam_net);

    ret = crosstalk_game_chat_rst(&ctx->chat);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "session leave chat rst failed"); }

    ret = crosstalk_game_chat_open_set(&ctx->chat, LDG_TRUTH_FALSE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "session leave chat close failed"); }

    ctx->mp_rdy = LDG_TRUTH_FALSE;
    ctx->mp_busy = LDG_TRUTH_FALSE;
    ctx->remote_sesh = LDG_TRUTH_FALSE;
    ctx->snap_req_sent = LDG_TRUTH_FALSE;
}

static uint8_t crosstalk_quit_stale_is(const crosstalk_ctx_t *ctx)
{
    uint64_t now_ms = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->game.screen != CROSSTALK_GAME_SCREEN_EDITOR) { return LDG_TRUTH_FALSE; }

    if (ctx->st_time_ms == 0) { return LDG_TRUTH_TRUE; }

    now_ms = ldg_time_epoch_ms_get();

    return (now_ms - ctx->st_time_ms >= (uint64_t)CROSSTALK_CORE_QUIT_GUARD_MS) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static void crosstalk_rtl_teardown(crosstalk_ctx_t *ctx)
{
    uint32_t slot_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    ret = crosstalk_eda_ic_fpga_run_hlt(&ctx->eda.fpga_run);
    if (ret != LDG_ERR_AOK && ret != CROSSTALK_EDA_IC_FPGA_RUN_ERR_IDLE) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "teardown run halt failed"); }

    for (slot_idx = 0; slot_idx < CROSSTALK_EDA_IC_FPGA_SLOT_MAX; slot_idx = slot_idx + 0x1)
    {
        ret = crosstalk_eda_ic_fpga_unprogram(&ctx->eda.fpga, slot_idx);
        if (ret != LDG_ERR_AOK && ret != CROSSTALK_EDA_IC_FPGA_ERR_SLOT) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "teardown unprogram failed"); }

        ret = crosstalk_eda_logic_probe_slot_detach(&ctx->eda.logic_probe, slot_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "teardown probe detach failed"); }
    }

    ret = crosstalk_core_watch_set_disarm(&ctx->design_watch);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "design watch: disarm failed"); }

    ret = crosstalk_core_fetch_stop(&ctx->fetch);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fetch: stop failed"); }

    ret = crosstalk_emu_net_shutdown(&ctx->emu_net);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu net shutdown failed"); }

    ret = crosstalk_eda_io_display_detach(&ctx->eda.display);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "teardown display detach failed"); }

    ret = crosstalk_eda_io_receptacle_detach(&ctx->eda.receptacle);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "teardown receptacle detach failed"); }

    ret = crosstalk_eda_io_midi_detach(&ctx->eda.midi);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "teardown midi detach failed"); }

    ret = crosstalk_ui_window_all_close(&ctx->win_nav);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "teardown window close failed"); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: rtl torn down");
}

static void crosstalk_menu_quit(crosstalk_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (crosstalk_eda_ic_fpga_build_live_is(&ctx->eda.fpga_build) == LDG_TRUTH_TRUE) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "menu quit refused; a bake is running"); return; }

    ctx->quit_armed = LDG_TRUTH_FALSE;

    if (crosstalk_sesh_joined_is(ctx) == LDG_TRUTH_TRUE) { crosstalk_sesh_leave(ctx); }

    crosstalk_rtl_teardown(ctx);

    ctx->game.paused = LDG_TRUTH_FALSE;
    ctx->game.screen = CROSSTALK_GAME_SCREEN_MENU;
    ctx->menu_sel = (uint32_t)CROSSTALK_MENU_SEL_FIRST;
}

static uint32_t ld_worker(uint32_t worker_id, void *arg)
{
    crosstalk_ctx_t *ctx = (crosstalk_ctx_t *)arg;
    crosstalk_game_ld_t *ld = 0x0;
    uint64_t now = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ld = &ctx->game.ld;
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "crosstalk: ld worker start; id: %u32; kind: %u32", worker_id, (uint32_t)ld->kind);

    if (ld->kind == CROSSTALK_LD_STARTUP)
    {
        if (ctx->save_path[0] != 0 && ldg_io_path_exists_is(ctx->save_path) == LDG_TRUTH_TRUE)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: ld save preload; path: %s", ctx->save_path);
            ret = crosstalk_game_save_rd(ctx->save_path, &ctx->game.settings, &ld->settings_present, &ctx->game.last_state, &ld->state_present, &ctx->game.author_steam_id, ctx->game.author_name, CROSSTALK_GAME_AUTHOR_NAME_CAP, &ctx->game.created_epoch_ms, &ctx->game.bench_ms_total, &ctx->game.sim_ms_total);
            if (ret == LDG_ERR_AOK) { ld->preloaded = LDG_TRUTH_TRUE; }
            else
            {
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: ld save preload failed; err: %u32", ret);
                ret = LDG_ERR_AOK;
            }
        }

        now = ldg_time_epoch_ms_get();
        LDG_WR_ONCE(ld->phase, CROSSTALK_LD_PHASE_SAVE_RDY);
        if (ldg_sys_progress_curr_set(&ld->bar, CROSSTALK_LD_PHASE_SAVE_RDY, now) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: ld progress set failed"); }
    }
    else if (ld->kind == CROSSTALK_LD_NEW || ld->kind == CROSSTALK_LD_BARE)
    {
        now = ldg_time_epoch_ms_get();
        LDG_WR_ONCE(ld->phase, CROSSTALK_LD_PHASE_BENCH_RDY);
        if (ldg_sys_progress_curr_set(&ld->bar, CROSSTALK_LD_PHASE_BENCH_RDY, now) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: ld progress set failed"); }
    }
    else
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: ld save path: %s", ctx->save_path);
        ret = crosstalk_game_save_rd(ctx->save_path, &ctx->game.settings, &ld->settings_present, &ctx->game.last_state, &ld->state_present, &ctx->game.author_steam_id, ctx->game.author_name, CROSSTALK_GAME_AUTHOR_NAME_CAP, &ctx->game.created_epoch_ms, &ctx->game.bench_ms_total, &ctx->game.sim_ms_total);
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "crosstalk: ld game_save_rd done; ret: %u32", ret);

        now = ldg_time_epoch_ms_get();
        LDG_WR_ONCE(ld->phase, CROSSTALK_LD_PHASE_SAVE_RDY);
        if (ldg_sys_progress_curr_set(&ld->bar, CROSSTALK_LD_PHASE_SAVE_RDY, now) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: ld progress set failed"); }
    }

    LDG_WR_ONCE(ld->err, ret);
    LDG_ST_RELEASE(ld->done, 1);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "crosstalk: ld worker done; kind: %u32; err: %u32", (uint32_t)ld->kind, ret);
    return ret;
}

static uint32_t ld_begin(crosstalk_ctx_t *ctx, crosstalk_game_ld_kind_t kind, crosstalk_game_screen_t tgt)
{
    ldg_sys_progress_conf_t conf = LDG_STRUCT_ZERO_INIT;
    uint64_t now = 0;
    uint32_t total = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->game.ld_active != 0) { return LDG_ERR_BUSY; }

    total = (kind == CROSSTALK_LD_NEW || kind == CROSSTALK_LD_BARE) ? CROSSTALK_LD_PHASE_BENCH_RDY : CROSSTALK_LD_PHASE_SAVE_RDY;
    now = ldg_time_epoch_ms_get();

    ret = ldg_sys_progress_conf_default_get(&conf);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    conf.total = total;
    conf.time_freq = LDG_MS_PER_SEC;
    conf.mode = LDG_SYS_PROGRESS_MODE_BAR;
    conf.flags = LDG_SYS_PROGRESS_FLAG_PCT | LDG_SYS_PROGRESS_FLAG_ETA;
    ret = ldg_sys_progress_init(&ctx->game.ld.bar, &conf, now);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->game.ld.kind = kind;
    ctx->game.ld.tgt = tgt;
    ctx->game.ld.total = total;
    ctx->game.ld.settings_present = 0;
    ctx->game.ld.state_present = 0;
    ctx->game.ld.preloaded = 0;
    LDG_WR_ONCE(ctx->game.ld.phase, 0);
    LDG_WR_ONCE(ctx->game.ld.err, 0);
    LDG_ST_RELEASE(ctx->game.ld.done, 0);
    ctx->game.ld_active = LDG_TRUTH_TRUE;
    ctx->game.ld_finalized = LDG_TRUTH_FALSE;
    ctx->game.screen = CROSSTALK_GAME_SCREEN_LD;
    ctx->ld_frames = 0;

    ret = ldg_thread_pool_submit(&ctx->game.ld_pool, ld_worker, ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ctx->game.ld_active = LDG_TRUTH_FALSE;
        return ret;
    }

    return LDG_ERR_AOK;
}

static void crosstalk_discord_settings_apply(crosstalk_ctx_t *ctx)
{
    if (ctx->discord_live != LDG_TRUTH_TRUE) { return; }

    crosstalk_net_discord_en_set(&ctx->discord, ctx->game.settings.discord_on);
    crosstalk_net_discord_presence_on_set(&ctx->discord, ctx->game.settings.discord_presence_on);
    crosstalk_net_discord_voice_self_mute_set(&ctx->discord, ctx->game.settings.discord_self_mute);
    crosstalk_net_discord_voice_self_deaf_set(&ctx->discord, ctx->game.settings.discord_self_deaf);
    crosstalk_net_discord_voice_vol_in_set(&ctx->discord, ctx->game.settings.discord_vol_in);
    crosstalk_net_discord_voice_vol_out_set(&ctx->discord, ctx->game.settings.discord_vol_out);
    crosstalk_net_discord_voice_on_set(&ctx->discord, ctx->game.settings.discord_voice_on);
}

static void settings_apply(crosstalk_ctx_t *ctx)
{
    uint32_t ret = 0;

    ret = crosstalk_game_settings_quantize(&ctx->game.settings);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "settings quantize failed"); }

    ret = crosstalk_game_music_src_set(&ctx->music, ctx->game.settings.music_src);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "music source apply failed"); }

    ctx->settings_applied = ctx->game.settings;
    ctx->settings_applied_ok = LDG_TRUTH_TRUE;
}

static void author_capture(crosstalk_ctx_t *ctx)
{
    const char *persona = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    persona = ldg_net_steamworks_local_name_get();
    ctx->game.author_steam_id = ldg_net_steamworks_local_steam_id_get();
    snprintf(ctx->game.author_name, sizeof(ctx->game.author_name), "%s", persona != 0x0 ? persona : "");
    ctx->game.created_epoch_ms = ldg_time_epoch_ms_get();
    ctx->game.bench_ms_total = 0;
    ctx->game.sim_ms_total = 0;
}

#define CROSSTALK_LAYOUT_WIN_CAP 24
#define CROSSTALK_SCRIPT_DIR_MAGIC "XTSC"
#define CROSSTALK_SCRIPT_DIR_MAGIC_LEN 4
#define CROSSTALK_SCRIPT_DIR_BYTES (CROSSTALK_SCRIPT_DIR_MAGIC_LEN + CROSSTALK_CORE_EDITOR_KIND_CUNT * sizeof(uint32_t))
#define CROSSTALK_LAYOUT_DIR_MAGIC "XLAY"
#define CROSSTALK_LAYOUT_DIR_MAGIC_LEN 4
#define CROSSTALK_LAYOUT_DIR_BYTES (CROSSTALK_LAYOUT_DIR_MAGIC_LEN + CROSSTALK_CORE_EDITOR_KIND_CUNT * sizeof(uint32_t))
#define CROSSTALK_LAYOUT_DEFAULT_CUNT 14
#define CROSSTALK_LAYOUT_GAP_NONE 0.0
#define CROSSTALK_LAYOUT_FRAC_CUNT 4
#define CROSSTALK_LAYOUT_REC_SIZE (sizeof(uint64_t) + CROSSTALK_LAYOUT_FRAC_CUNT * sizeof(double) + sizeof(uint8_t))

typedef struct crosstalk_layout_win
{
    uint64_t id;
    crosstalk_ui_window_state_t *win;
} crosstalk_layout_win_t;

typedef struct crosstalk_layout_default
{
    uint64_t id;
    double frac[CROSSTALK_LAYOUT_FRAC_CUNT];
    uint8_t open;
    uint8_t pudding[7];
} crosstalk_layout_default_t;

static uint32_t crosstalk_layout_wins_get(crosstalk_ctx_t *ctx, crosstalk_layout_win_t *out, uint32_t cap, uint32_t *out_cunt)
{
    uint32_t cunt = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0 || out_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (cap < (uint32_t)CROSSTALK_LAYOUT_WIN_CAP) { return LDG_ERR_BOUNDS; }

    out[cunt].id = (uint64_t)CROSSTALK_EDA_UI_TERM_WINDOW_ID;
    out[cunt].win = &ctx->eda.term.win;
    cunt = cunt + 0x1;
    out[cunt].id = (uint64_t)CROSSTALK_EDA_UI_SCOPE_WINDOW_ID;
    out[cunt].win = &ctx->eda.scope.win;
    cunt = cunt + 0x1;
    out[cunt].id = (uint64_t)CROSSTALK_EDA_UI_INDICATOR_WINDOW_ID;
    out[cunt].win = &ctx->eda.indicator.win;
    cunt = cunt + 0x1;
    out[cunt].id = (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_WINDOW_ID;
    out[cunt].win = &ctx->eda.memview.win;
    cunt = cunt + 0x1;
    out[cunt].id = (uint64_t)CROSSTALK_EDA_UI_CALC_WINDOW_ID;
    out[cunt].win = &ctx->eda.calc.win;
    cunt = cunt + 0x1;
    out[cunt].id = (uint64_t)CROSSTALK_EDA_UI_PROFILE_WINDOW_ID;
    out[cunt].win = &ctx->eda.profile.win;
    cunt = cunt + 0x1;
    out[cunt].id = (uint64_t)CROSSTALK_EDA_UI_REGVIEW_WINDOW_ID;
    out[cunt].win = &ctx->eda.regview.win;
    cunt = cunt + 0x1;
    out[cunt].id = (uint64_t)CROSSTALK_EDA_UI_STACKVIEW_WINDOW_ID;
    out[cunt].win = &ctx->eda.stackview.win;
    cunt = cunt + 0x1;
    out[cunt].id = (uint64_t)CROSSTALK_EDA_UI_BRAINOMATIC_WINDOW_ID;
    out[cunt].win = &ctx->eda.brainomatic.win;
    cunt = cunt + 0x1;
    out[cunt].id = (uint64_t)CROSSTALK_EDA_UI_PROGVIEW_WINDOW_ID;
    out[cunt].win = &ctx->eda.progview.win;
    cunt = cunt + 0x1;
    out[cunt].id = (uint64_t)CROSSTALK_EDA_UI_PIPEVIEW_WINDOW_ID;
    out[cunt].win = &ctx->eda.pipeview.win;
    cunt = cunt + 0x1;
    out[cunt].id = (uint64_t)CROSSTALK_EDA_UI_ANALYZER_WINDOW_ID;
    out[cunt].win = &ctx->eda.analyzer.win;
    cunt = cunt + 0x1;
    out[cunt].id = (uint64_t)CROSSTALK_EDA_UI_FEED_WINDOW_ID;
    out[cunt].win = &ctx->eda.feed.win;
    cunt = cunt + 0x1;
    out[cunt].id = (uint64_t)CROSSTALK_EDA_UI_DISPLAY_CRT_WINDOW_ID;
    out[cunt].win = &ctx->eda.crt.win;
    cunt = cunt + 0x1;
    out[cunt].id = (uint64_t)CROSSTALK_RENG_PROGRESS_ID;
    out[cunt].win = &ctx->reng_progress.win;
    cunt = cunt + 0x1;
    out[cunt].id = (uint64_t)CROSSTALK_RENG_TASKS_ID;
    out[cunt].win = &ctx->reng_tasks.win;
    cunt = cunt + 0x1;
    out[cunt].id = (uint64_t)CROSSTALK_UI_GUIDE_WINDOW_ID;
    out[cunt].win = &ctx->guide.win;
    cunt = cunt + 0x1;
    *out_cunt = cunt;
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_guide_tgt_open_cb(void *user, uint64_t win_id)
{
    crosstalk_ctx_t *ctx = (crosstalk_ctx_t *)user;
    crosstalk_layout_win_t wins[CROSSTALK_LAYOUT_WIN_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t win_cunt = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_layout_wins_get(ctx, wins, (uint32_t)CROSSTALK_LAYOUT_WIN_CAP, &win_cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    for (at = 0; at < win_cunt; at = at + 0x1)
    {
        if (wins[at].id != win_id || wins[at].win == 0x0) { continue; }

        wins[at].win->open = LDG_TRUTH_TRUE;

        if (win_id == (uint64_t)CROSSTALK_EDA_UI_REGVIEW_WINDOW_ID) { ctx->eda.regview.open = LDG_TRUTH_TRUE; }

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

static uint8_t crosstalk_guide_done_cb(void *user, uint64_t done_id)
{
    crosstalk_ctx_t *ctx = (crosstalk_ctx_t *)user;
    const crosstalk_eda_ic_fpga_slot_t *slot = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    slot = &ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT];

    if (done_id == (uint64_t)CROSSTALK_CORE_GUIDE_DONE_LD_EXAMPLE) { return slot->live == LDG_TRUTH_TRUE && strcmp(slot->top, CROSSTALK_CORE_GUIDE_EXAMPLE_TOP) == 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    if (done_id == (uint64_t)CROSSTALK_CORE_GUIDE_DONE_IND_EXAMPLE) { return crosstalk_eda_ui_indicator_open_is(&ctx->eda.indicator) == LDG_TRUTH_TRUE && strcmp(ctx->eda.indicator.port, CROSSTALK_CORE_GUIDE_EXAMPLE_PORT) == 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    return LDG_TRUTH_FALSE;
}

static double crosstalk_card_level_clamp(double level)
{
    double out = level;

    if (out < CROSSTALK_CORE_CARD_BLACK) { out = CROSSTALK_CORE_CARD_BLACK; }

    if (out > CROSSTALK_CORE_CARD_WHITE) { out = CROSSTALK_CORE_CARD_WHITE; }

    return out;
}

static uint32_t crosstalk_card_channel_get(double level)
{
    return (uint32_t)(crosstalk_card_level_clamp(level) * CROSSTALK_CORE_CARD_CHANNEL_MAX + CROSSTALK_CORE_CARD_ROUND);
}

static uint32_t crosstalk_card_sample_get(double pos)
{
    if (pos <= 0.0) { return (uint32_t)CROSSTALK_CORE_CARD_SAMPLE_ORIG; }

    return (uint32_t)(pos + CROSSTALK_CORE_CARD_ROUND);
}

static uint32_t crosstalk_card_rgb_fill(ldg_term_render_t *render, double rect_x, double rect_y, double rect_w, double rect_h, double col_r, double col_g, double col_b)
{
    uint32_t pos_x = crosstalk_card_sample_get(rect_x);
    uint32_t pos_y = crosstalk_card_sample_get(rect_y);
    uint32_t end_x = crosstalk_card_sample_get(rect_x + rect_w);
    uint32_t end_y = crosstalk_card_sample_get(rect_y + rect_h);
    uint32_t rgba = 0;

    if (LDG_UNLIKELY(render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (rect_w <= 0.0 || rect_h <= 0.0) { return LDG_ERR_AOK; }

    if (end_x <= pos_x) { end_x = pos_x + (uint32_t)CROSSTALK_CORE_CARD_SAMPLE_MIN; }

    if (end_y <= pos_y) { end_y = pos_y + (uint32_t)CROSSTALK_CORE_CARD_SAMPLE_MIN; }

    rgba = (crosstalk_card_channel_get(col_r) << CROSSTALK_UI_CELL_RGBA_RED_SHIFT) | (crosstalk_card_channel_get(col_g) << CROSSTALK_UI_CELL_RGBA_GREEN_SHIFT) | (crosstalk_card_channel_get(col_b) << CROSSTALK_UI_CELL_RGBA_BLUE_SHIFT) | (uint32_t)CROSSTALK_CORE_CARD_ALPHA_OPAQUE;

    return crosstalk_ui_cell_sample_fill(render, pos_x, pos_y, end_x - pos_x, end_y - pos_y, rgba);
}

static uint32_t crosstalk_card_yuv_fill(ldg_term_render_t *render, double rect_x, double rect_y, double rect_w, double rect_h, double luma, double diff_r, double diff_b)
{
    double col_r = crosstalk_card_level_clamp(luma + diff_r);
    double col_b = crosstalk_card_level_clamp(luma + diff_b);
    double col_g = crosstalk_card_level_clamp(luma - (CROSSTALK_CORE_CARD_LUMA_R * diff_r + CROSSTALK_CORE_CARD_LUMA_B * diff_b) / CROSSTALK_CORE_CARD_LUMA_G);

    return crosstalk_card_rgb_fill(render, rect_x, rect_y, rect_w, rect_h, col_r, col_g, col_b);
}

static uint32_t crosstalk_card_gray_fill(ldg_term_render_t *render, double rect_x, double rect_y, double rect_w, double rect_h, double level)
{
    return crosstalk_card_rgb_fill(render, rect_x, rect_y, rect_w, rect_h, level, level, level);
}

static uint32_t crosstalk_card_ring_draw(ldg_term_render_t *render, double circ_x, double circ_y, double circ_r, double line_px, double level)
{
    uint32_t idx = 0;
    uint32_t ret = 0;

    for (idx = 0; idx < (uint32_t)CROSSTALK_CORE_CARD_RING_STEP_CUNT; idx = idx + 0x1)
    {
        double angle = CROSSTALK_CORE_CARD_TURN * LDG_MATH_PI * (double)idx / (double)CROSSTALK_CORE_CARD_RING_STEP_CUNT;

        ret = crosstalk_card_gray_fill(render, circ_x + circ_r * cos(angle) - line_px * CROSSTALK_CORE_CARD_HALF, circ_y + circ_r * sin(angle) - line_px * CROSSTALK_CORE_CARD_HALF, line_px, line_px, level);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return LDG_ERR_AOK;
}

static double crosstalk_card_half_chord_get(double circ_r, double off_y)
{
    double span = circ_r * circ_r - off_y * off_y;

    if (span < 0.0) { span = 0.0; }

    return sqrt(span);
}

static uint32_t crosstalk_card_side_draw(ldg_term_render_t *render, double card_x, double card_y, double cell, double card_w, double diff_r, double diff_b, uint8_t mirror)
{
    double side_w = cell * CROSSTALK_CORE_CARD_SIDE_W_CELLS;
    double alt_x = card_x + cell * CROSSTALK_CORE_CARD_SIDE_ALT_X_CELLS;
    double bar_x = card_x + cell * CROSSTALK_CORE_CARD_SIDE_BAR_X_CELLS;
    double rect_x = card_x + cell * CROSSTALK_CORE_CARD_SIDE_RECT_X_CELLS;
    double top_y = card_y + cell * CROSSTALK_CORE_CARD_SIDE_TOP_CELLS;
    double side_h = cell * CROSSTALK_CORE_CARD_SIDE_H_CELLS;
    double rect_top_y = card_y + cell * CROSSTALK_CORE_CARD_SIDE_RECT_TOP_CELLS;
    double rect_h = cell * CROSSTALK_CORE_CARD_SIDE_RECT_H_CELLS;
    double rect_bot_y = card_y + cell * ((double)CROSSTALK_CORE_CARD_ROW_CUNT - CROSSTALK_CORE_CARD_SIDE_RECT_TOP_CELLS - CROSSTALK_CORE_CARD_SIDE_RECT_H_CELLS);
    double line_y = 0.0;
    double sign = 1.0;
    uint32_t ret = 0;

    if (mirror == LDG_TRUTH_TRUE)
    {
        alt_x = card_x + card_w - (alt_x - card_x) - side_w;
        bar_x = card_x + card_w - (bar_x - card_x) - side_w;
        rect_x = card_x + card_w - (rect_x - card_x) - side_w;
    }

    for (line_y = top_y; line_y < top_y + side_h; line_y = line_y + 1.0)
    {
        ret = crosstalk_card_yuv_fill(render, alt_x, line_y, side_w, 1.0, CROSSTALK_CORE_CARD_FIELD_GRAY, diff_r * sign, diff_b * sign);
        if (ret != LDG_ERR_AOK) { return ret; }

        sign = -sign;
    }

    ret = crosstalk_card_yuv_fill(render, bar_x, top_y, side_w, side_h * 0.5, CROSSTALK_CORE_CARD_FIELD_GRAY, diff_r, diff_b);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_card_yuv_fill(render, bar_x, top_y + side_h * 0.5, side_w, side_h * 0.5, CROSSTALK_CORE_CARD_FIELD_GRAY, -diff_r, -diff_b);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_card_yuv_fill(render, rect_x, rect_top_y, side_w, rect_h, CROSSTALK_CORE_CARD_FIELD_GRAY, -CROSSTALK_CORE_CARD_DIFF_AMPLITUDE * CROSSTALK_CORE_CARD_LUMA_B / CROSSTALK_CORE_CARD_LUMA_R, CROSSTALK_CORE_CARD_DIFF_AMPLITUDE);
    if (ret != LDG_ERR_AOK) { return ret; }

    return crosstalk_card_yuv_fill(render, rect_x, rect_bot_y, side_w, rect_h, CROSSTALK_CORE_CARD_FIELD_GRAY, CROSSTALK_CORE_CARD_DIFF_AMPLITUDE * CROSSTALK_CORE_CARD_LUMA_B / CROSSTALK_CORE_CARD_LUMA_R, -CROSSTALK_CORE_CARD_DIFF_AMPLITUDE);
}

static uint32_t crosstalk_card_needle_step_draw(ldg_term_render_t *render, double left_x, double span_w, double band_y, double band_h, double line_px)
{
    uint32_t ret = crosstalk_card_gray_fill(render, left_x, band_y, span_w * 0.5, band_h, CROSSTALK_CORE_CARD_WHITE);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_card_gray_fill(render, left_x + span_w * 0.5, band_y, span_w * 0.5, band_h, CROSSTALK_CORE_CARD_BLACK);
    if (ret != LDG_ERR_AOK) { return ret; }

    return crosstalk_card_gray_fill(render, left_x + span_w * CROSSTALK_CORE_CARD_NEEDLE_POS - line_px * 0.5, band_y, line_px, band_h, CROSSTALK_CORE_CARD_WHITE);
}

static void crosstalk_card_kind_resolve(crosstalk_ctx_t *ctx)
{
    char country[LDG_NET_STEAMWORKS_COUNTRY_SIZE] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    if (ctx->card_kind_set == LDG_TRUTH_TRUE) { return; }

    ret = ldg_net_steamworks_ip_country_get(country, sizeof(country));

    if (ret == LDG_ERR_NOT_FOUND) { return; }

    ctx->card_kind = (uint8_t)CROSSTALK_CORE_CARD_KIND_PAL;
    ctx->card_kind_set = LDG_TRUTH_TRUE;

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: card country; err: %u32", ret);
        return;
    }

    if (strcmp(country, CROSSTALK_CORE_CARD_COUNTRY_NTSC) == 0) { ctx->card_kind = (uint8_t)CROSSTALK_CORE_CARD_KIND_NTSC; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: card country: %s; kind: %u32", country, ctx->card_kind);
}

static uint32_t crosstalk_card_pal_draw(ldg_term_render_t *render)
{
    static const double bar_rgb[CROSSTALK_CORE_CARD_BAR_CUNT][3] = { { 1.0, 1.0, 0.0 }, { 0.0, 1.0, 1.0 }, { 0.0, 1.0, 0.0 }, { 1.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 } };
    static const double grating_mhz[CROSSTALK_CORE_CARD_GRATING_CUNT] = { 0.8, 1.8, 2.8, 3.8, 4.8 };
    uint32_t win_w = render != 0x0 ? render->col_cunt * render->sample_col_cunt : (uint32_t)CROSSTALK_CORE_CARD_SAMPLE_ORIG;
    uint32_t win_h = render != 0x0 ? render->row_cunt * render->sample_row_cunt : (uint32_t)CROSSTALK_CORE_CARD_SAMPLE_ORIG;
    double card_h = (double)win_h;
    double card_w = card_h * CROSSTALK_CORE_CARD_ASPECT_W / CROSSTALK_CORE_CARD_ASPECT_H;
    double card_x = 0.0;
    double card_y = 0.0;
    double cell = 0.0;
    double us_px = 0.0;
    double line_px = 0.0;
    double cast_top = 0.0;
    double cast_side = 0.0;
    double circ_x = 0.0;
    double circ_y = 0.0;
    double circ_r = 0.0;
    double band_y = 0.0;
    double band_h = 0.0;
    double half = 0.0;
    double run_x = 0.0;
    double run_w = 0.0;
    double seg_x0 = 0.0;
    double seg_x1 = 0.0;
    double level = 0.0;
    double sign = 1.0;
    uint32_t text_col = 0;
    uint32_t text_row = 0;
    uint32_t text_w = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (render->sample_col_cunt == 0 || render->sample_row_cunt == 0) { return LDG_ERR_NOT_INIT; }

    if (card_w > (double)win_w)
    {
        card_w = (double)win_w;
        card_h = card_w * CROSSTALK_CORE_CARD_ASPECT_H / CROSSTALK_CORE_CARD_ASPECT_W;
    }

    card_x = ((double)win_w - card_w) * 0.5;
    card_y = ((double)win_h - card_h) * 0.5;
    cell = card_w / (double)CROSSTALK_CORE_CARD_COL_CUNT;
    us_px = card_w / CROSSTALK_CORE_CARD_ACTIVE_LINE_US;
    line_px = us_px * CROSSTALK_CORE_CARD_LINE_US;
    cast_top = cell * CROSSTALK_CORE_CARD_CAST_TOP_CELLS;
    cast_side = cell * CROSSTALK_CORE_CARD_CAST_SIDE_CELLS;
    circ_x = card_x + card_w * 0.5;
    circ_y = card_y + card_h * 0.5;
    circ_r = card_h * CROSSTALK_CORE_CARD_CIRC_DIAMETER_FRAC * 0.5;

    ret = crosstalk_card_gray_fill(render, 0.0, 0.0, (double)win_w, (double)win_h, CROSSTALK_CORE_CARD_BLACK);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_card_gray_fill(render, card_x, card_y, card_w, card_h, CROSSTALK_CORE_CARD_FIELD_GRAY);
    if (ret != LDG_ERR_AOK) { return ret; }

    for (idx = 1; idx < (uint32_t)CROSSTALK_CORE_CARD_COL_CUNT; idx = idx + 0x1)
    {
        ret = crosstalk_card_gray_fill(render, card_x + (double)idx * cell - line_px * 0.5, card_y + cast_top, line_px, card_h - cast_top - cast_top, CROSSTALK_CORE_CARD_WHITE);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    for (idx = 1; idx < (uint32_t)CROSSTALK_CORE_CARD_ROW_CUNT; idx = idx + 0x1)
    {
        ret = crosstalk_card_gray_fill(render, card_x + cast_side, card_y + (double)idx * cell - line_px * 0.5, card_w - cast_side - cast_side, line_px, CROSSTALK_CORE_CARD_WHITE);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    for (idx = 0; idx < (uint32_t)CROSSTALK_CORE_CARD_COL_CUNT; idx = idx + 0x1)
    {
        level = (idx & 0x1) == 0 ? CROSSTALK_CORE_CARD_BLACK : CROSSTALK_CORE_CARD_WHITE;

        ret = crosstalk_card_gray_fill(render, card_x + (double)idx * cell, card_y, cell, cast_top, level);
        if (ret != LDG_ERR_AOK) { return ret; }

        ret = crosstalk_card_gray_fill(render, card_x + (double)idx * cell, card_y + card_h - cast_top, cell, cast_top, level);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    for (idx = 0; idx < (uint32_t)CROSSTALK_CORE_CARD_ROW_CUNT; idx = idx + 0x1)
    {
        level = (idx & 0x1) == 0 ? CROSSTALK_CORE_CARD_BLACK : CROSSTALK_CORE_CARD_WHITE;

        ret = crosstalk_card_gray_fill(render, card_x + card_w - cast_side, card_y + (double)idx * cell, cast_side, cell, level);
        if (ret != LDG_ERR_AOK) { return ret; }

        if (idx != (uint32_t)CROSSTALK_CORE_CARD_CAST_ALT_ROW_A && idx != (uint32_t)CROSSTALK_CORE_CARD_CAST_ALT_ROW_B)
        {
            ret = crosstalk_card_gray_fill(render, card_x, card_y + (double)idx * cell, cast_side, cell, level);
            if (ret != LDG_ERR_AOK) { return ret; }

            continue;
        }

        for (run_x = card_y + (double)idx * cell; run_x < card_y + (double)(idx + 0x1) * cell; run_x = run_x + 1.0)
        {
            ret = crosstalk_card_yuv_fill(render, card_x, run_x, cast_side, 1.0, CROSSTALK_CORE_CARD_FIELD_GRAY, CROSSTALK_CORE_CARD_DIFF_AMPLITUDE * sign, 0.0);
            if (ret != LDG_ERR_AOK) { return ret; }

            sign = -sign;
        }
    }

    ret = crosstalk_card_side_draw(render, card_x, card_y, cell, card_w, CROSSTALK_CORE_CARD_DIFF_AMPLITUDE, 0.0, LDG_TRUTH_FALSE);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_card_side_draw(render, card_x, card_y, cell, card_w, 0.0, CROSSTALK_CORE_CARD_DIFF_AMPLITUDE, LDG_TRUTH_TRUE);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_card_ring_draw(render, circ_x, circ_y, circ_r, line_px, CROSSTALK_CORE_CARD_WHITE);
    if (ret != LDG_ERR_AOK) { return ret; }

    band_y = card_y + cell * CROSSTALK_CORE_CARD_RECT_TOP_CELLS;
    band_h = cell * CROSSTALK_CORE_CARD_RECT_H_CELLS;
    half = crosstalk_card_half_chord_get(circ_r, band_y + band_h * 0.5 - circ_y);

    ret = crosstalk_card_gray_fill(render, circ_x - half, band_y, half + half, band_h, CROSSTALK_CORE_CARD_WHITE);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_card_gray_fill(render, circ_x - us_px * CROSSTALK_CORE_CARD_BLACK_RECT_US * 0.5, band_y, us_px * CROSSTALK_CORE_CARD_BLACK_RECT_US, band_h, CROSSTALK_CORE_CARD_BLACK);
    if (ret != LDG_ERR_AOK) { return ret; }

    band_y = band_y + band_h;
    band_h = cell * CROSSTALK_CORE_CARD_NEEDLE_H_CELLS;
    half = crosstalk_card_half_chord_get(circ_r, band_y + band_h * 0.5 - circ_y);

    ret = crosstalk_card_needle_step_draw(render, circ_x - half, half + half, band_y, band_h, line_px);
    if (ret != LDG_ERR_AOK) { return ret; }

    band_y = band_y + band_h;
    band_h = cell * CROSSTALK_CORE_CARD_SQUARE_H_CELLS;
    half = crosstalk_card_half_chord_get(circ_r, band_y + band_h * 0.5 - circ_y);
    run_w = us_px * (double)LDG_MS_PER_SEC / CROSSTALK_CORE_CARD_SQUARE_KHZ * 0.5;
    level = CROSSTALK_CORE_CARD_BLACK;

    for (run_x = card_x; run_x < circ_x + half; run_x = run_x + run_w)
    {
        seg_x0 = run_x > circ_x - half ? run_x : circ_x - half;
        seg_x1 = run_x + run_w < circ_x + half ? run_x + run_w : circ_x + half;

        if (seg_x1 > seg_x0)
        {
            ret = crosstalk_card_gray_fill(render, seg_x0, band_y, seg_x1 - seg_x0, band_h, level);
            if (ret != LDG_ERR_AOK) { return ret; }
        }

        level = level == CROSSTALK_CORE_CARD_BLACK ? CROSSTALK_CORE_CARD_BAR_AMPLITUDE : CROSSTALK_CORE_CARD_BLACK;
    }

    band_y = band_y + band_h;
    band_h = cell * CROSSTALK_CORE_CARD_BAR_H_CELLS;
    half = crosstalk_card_half_chord_get(circ_r, band_y + band_h * 0.5 - circ_y);
    run_w = (half + half) / (double)CROSSTALK_CORE_CARD_BAR_CUNT;

    for (idx = 0; idx < (uint32_t)CROSSTALK_CORE_CARD_BAR_CUNT; idx = idx + 0x1)
    {
        ret = crosstalk_card_rgb_fill(render, circ_x - half + run_w * (double)idx, band_y, run_w, band_h, bar_rgb[idx][0] * CROSSTALK_CORE_CARD_BAR_AMPLITUDE, bar_rgb[idx][1] * CROSSTALK_CORE_CARD_BAR_AMPLITUDE, bar_rgb[idx][2] * CROSSTALK_CORE_CARD_BAR_AMPLITUDE);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    band_y = band_y + band_h;
    band_h = cell * CROSSTALK_CORE_CARD_CROSS_H_CELLS;
    half = crosstalk_card_half_chord_get(circ_r, 0.0);

    ret = crosstalk_card_gray_fill(render, circ_x - half, circ_y - line_px * 0.5, half + half, line_px, CROSSTALK_CORE_CARD_WHITE);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_card_gray_fill(render, circ_x - line_px * 0.5, band_y, line_px, band_h, CROSSTALK_CORE_CARD_WHITE);
    if (ret != LDG_ERR_AOK) { return ret; }

    seg_x0 = circ_y + line_px * 0.5 + cell * CROSSTALK_CORE_CARD_IDENT_GAP_CELLS;
    seg_x1 = band_y + band_h - seg_x0;
    half = crosstalk_card_half_chord_get(circ_r, seg_x0 + seg_x1 * 0.5 - circ_y);

    ret = crosstalk_card_gray_fill(render, circ_x - half, seg_x0, half + half, seg_x1, CROSSTALK_CORE_CARD_BLACK);
    if (ret != LDG_ERR_AOK) { return ret; }

    text_w = (uint32_t)(sizeof(CROSSTALK_CORE_CARD_TEXT) - LDG_STR_TERM_SIZE);
    text_col = crosstalk_card_sample_get(circ_x / (double)render->sample_col_cunt - (double)text_w * CROSSTALK_CORE_CARD_HALF);
    text_row = crosstalk_card_sample_get((seg_x0 + seg_x1 * CROSSTALK_CORE_CARD_HALF) / (double)render->sample_row_cunt - CROSSTALK_CORE_CARD_HALF);

    ret = crosstalk_ui_cell_rgba_put(render, text_col, text_row, CROSSTALK_CORE_CARD_TEXT, (uint64_t)sizeof(CROSSTALK_CORE_CARD_TEXT), (uint32_t)CROSSTALK_UI_THEME_WHITE_RGBA, &text_w);
    if (ret != LDG_ERR_AOK) { return ret; }

    band_y = band_y + band_h;
    band_h = cell * CROSSTALK_CORE_CARD_GRATING_H_CELLS;
    half = crosstalk_card_half_chord_get(circ_r, band_y + band_h * 0.5 - circ_y);
    run_w = (half + half) / (double)CROSSTALK_CORE_CARD_GRATING_CUNT;

    for (idx = 0; idx < (uint32_t)CROSSTALK_CORE_CARD_GRATING_CUNT; idx = idx + 0x1) { for (run_x = circ_x - half + run_w * (double)idx; run_x < circ_x - half + run_w * (double)(idx + 0x1); run_x = run_x + 1.0)
        {
            level = CROSSTALK_CORE_CARD_GRATING_MID + CROSSTALK_CORE_CARD_GRATING_AMP * sin(2.0 * LDG_MATH_PI * grating_mhz[idx] * (run_x - card_x) / us_px);
            ret = crosstalk_card_gray_fill(render, run_x, band_y, 1.0, band_h, crosstalk_card_level_clamp(level));
            if (ret != LDG_ERR_AOK) { return ret; }
        }
    }

    band_y = band_y + band_h;
    band_h = cell * CROSSTALK_CORE_CARD_STEP_H_CELLS;
    half = crosstalk_card_half_chord_get(circ_r, band_y + band_h * 0.5 - circ_y);
    run_w = (half + half) / (double)CROSSTALK_CORE_CARD_STEP_CUNT;

    for (idx = 0; idx < (uint32_t)CROSSTALK_CORE_CARD_STEP_CUNT; idx = idx + 0x1)
    {
        level = (double)idx / (double)(CROSSTALK_CORE_CARD_STEP_CUNT - 0x1);

        ret = crosstalk_card_gray_fill(render, circ_x - half + run_w * (double)idx, band_y, run_w, band_h, level);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    band_y = band_y + band_h;
    band_h = cell * CROSSTALK_CORE_CARD_NEEDLE_H_CELLS;
    half = crosstalk_card_half_chord_get(circ_r, band_y + band_h * 0.5 - circ_y);

    ret = crosstalk_card_needle_step_draw(render, circ_x - half, half + half, band_y, band_h, line_px);
    if (ret != LDG_ERR_AOK) { return ret; }

    band_y = band_y + band_h;
    band_h = cell * CROSSTALK_CORE_CARD_COLSTEP_H_CELLS;
    half = crosstalk_card_half_chord_get(circ_r, band_y + band_h * 0.5 - circ_y);

    ret = crosstalk_card_rgb_fill(render, circ_x - half, band_y, half + half, band_h, CROSSTALK_CORE_CARD_BAR_AMPLITUDE, CROSSTALK_CORE_CARD_BAR_AMPLITUDE, CROSSTALK_CORE_CARD_BLACK);
    if (ret != LDG_ERR_AOK) { return ret; }

    return crosstalk_card_rgb_fill(render, circ_x - us_px * CROSSTALK_CORE_CARD_COLSTEP_US * 0.5, band_y, us_px * CROSSTALK_CORE_CARD_COLSTEP_US, band_h, CROSSTALK_CORE_CARD_BAR_AMPLITUDE, CROSSTALK_CORE_CARD_BLACK, CROSSTALK_CORE_CARD_BLACK);
}

static uint32_t crosstalk_crt_card_draw(crosstalk_ctx_t *ctx, ldg_term_render_t *render)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    switch (ctx->card_kind)
    {
        case CROSSTALK_CORE_CARD_KIND_NTSC:
            ret = crosstalk_card_pal_draw(render);
            break;
        default:
            ret = crosstalk_card_pal_draw(render);
            break;
    }

    return ret;
}

static uint32_t crosstalk_guides_reg(crosstalk_ctx_t *ctx)
{
    static const crosstalk_ui_guide_step_t tour_step[] = {
        { "Command line", "Pressing `^Shift;`, or in the vim format, `<C-S-;>`, opens the cmdline. Commands are semicolon-terminated. `help;` shows the commands that are available. The `ld` cmd pulls in .sv files explicitly named in the command. Typing the command shows help text to the right of the cmdline text input field. The `memld` cmd then loads into a design-specified memory signal (syntax in cmdline helper). For example, to load a SystemVerilog design: `ld <path_to_sv_dir> <file1.sv,file2.sv,file3.sv> <top_name>;` The files are comma-separated without a whitespace. The `rst;` cmd power-cycles the emulator.", 0x0, 0x0, (uint64_t)UINT64_MAX, (uint64_t)UINT64_MAX },
        { "Scripting", "Crosstalk reads `.xts` files as scripts. Scripts consist of multiple commands delimited by a semicolon. Strictly speaking, cmdline commands are single-command in-memory scripts. `run <script>.xts;` will load and execute a script serially. If the game is saved after a `run` invocation, then that same script will re-run on the next load of the same project within the same editor.", 0x0, 0x0, (uint64_t)UINT64_MAX, (uint64_t)UINT64_MAX },
        { "Simulation mode", "Once a SystemVerilog design is loaded, it will run in cycle-level simulation mode by default. It can be switched to gate-level simulation using the `gate;` cmd. Gate-level simulation is excruciatingly slow. For a performance improvement above the cycle-level simulation, the `emit;` cmd can be used, which will transliterate it into C99, invoke the included compiler, and fully compile, assemble, and link the design. Once emitted, a design snapshot can be taken using the `snap;` cmd, which takes an optional snapshot title argument. For example, `snap after_boot;`. Re-running `snap` with no arg will refuse to overwrite a previous invocation without an arg. An invocation using a pre-existing title does overwrite. For resumption, `resume;` can be used. Without an argument, it will restore a snapshot that was created by invoking `snap;` without an argument. It also takes an optional title argument and errors out on failure to find.", 0x0, 0x0, (uint64_t)UINT64_MAX, (uint64_t)UINT64_MAX },
        { "Emulation mode", "The only usable emulator is currently the DANGCSA emulator. Another emulator can be attached, but the ABI is not frozen yet, so this path is not supported right now. The `hlt;` cmd immediately halts emulation for easier state inspection and event detection. `hlt off;` unhalts.", 0x0, 0x0, (uint64_t)UINT64_MAX, (uint64_t)UINT64_MAX },
        { "Terminal", "The terminal allows interfacing with PTY and serial devices, both physical and virtual ones. When in the serial tab of the terminal, the terminal will often be seen empty, since, outside of profiling mode (cmdline: `prof on;`), it will not sample a design's serial port. Copy and paste currently do not work.", 0x0, 0x0, (uint64_t)CROSSTALK_EDA_UI_TERM_WINDOW_ID, (uint64_t)UINT64_MAX },
        { "Memory", "Memview shows the system memory and memory-mapped I/O (if the active design exposes it) of the active design. It works in both simulation and emulation mode (emulation mode currently only supports the DLU since it requires separate code). Memview has its own set of commands, which are independent of the cmdline and currently not scriptable. They are not terminated by semicolons. A few useful and commonly-used ones: `go <addr>|<symbol>[+<offset>]`, `find [<pattern>|\"string\"]`, `break [<addr>|<symbol>]`, `strings [min_chars]`, `refs [<addr>|<symbol>]`.", 0x0, 0x0, (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_WINDOW_ID, (uint64_t)UINT64_MAX },
        { "Registers", "Regview shows a design's registers, assuming they're exposed. By default, the program counter is annotated with `loc`, which is the case with the DLU.", 0x0, 0x0, (uint64_t)CROSSTALK_EDA_UI_REGVIEW_WINDOW_ID, (uint64_t)UINT64_MAX },
        { "Stack", "Stackview shows a continuous stack trace and stops on design faults, assuming those are configured in the design. It pauses in gfx mode and doesn't work outside of profile mode.", 0x0, 0x0, (uint64_t)CROSSTALK_EDA_UI_STACKVIEW_WINDOW_ID, (uint64_t)UINT64_MAX },
        { "Program", "Progview shows the currently running program, if Crosstalk can detect it.", 0x0, 0x0, (uint64_t)CROSSTALK_EDA_UI_PROGVIEW_WINDOW_ID, (uint64_t)UINT64_MAX },
        { "Calculator", "The calc supports conversion and simple calculations, plus symbol assignment at runtime, so that an address can be found by a string literal. For example: `health=mo+0x6c` or `posx=0x374A8`.", 0x0, 0x0, (uint64_t)CROSSTALK_EDA_UI_CALC_WINDOW_ID, (uint64_t)UINT64_MAX },
        { "Monitor", "Designs that have video output can use the monitor, which can operate as a CRT, a gas-plasma, and an LCD over CGA or VGA. Video output doesn't necessarily require gfx mode, but is considerably faster with it enabled.", 0x0, 0x0, (uint64_t)CROSSTALK_EDA_UI_DISPLAY_CRT_WINDOW_ID, (uint64_t)UINT64_MAX },
        { "Profile", "Both cores and emulators can be profiled using the profiler. `prof on;` enables profiling mode and populates the profiler window.", 0x0, 0x0, (uint64_t)CROSSTALK_EDA_UI_PROFILE_WINDOW_ID, (uint64_t)UINT64_MAX },
        { "Analyzer", "The logic analyzer can be used with arbitrary signals defined in the top module. It requires profiling to be enabled and also works in emulation mode. `analyzer <sig> <optional_another_sig> ...;` inserts a signal into the analyzer, up to its configured limit. `analyzer off;` empties it.", 0x0, 0x0, (uint64_t)CROSSTALK_EDA_UI_ANALYZER_WINDOW_ID, (uint64_t)UINT64_MAX },
        { "Scope", "The oscilloscope can plot an arbitrary signal that is declared in the top module and requires profiling to be enabled when scoping signals. It also works in emulation mode. It takes up to one signal, which can optionally be a memory address or symbol, such as: `scope mem <0xADDR|symbol> [width] [fixed];`.", 0x0, 0x0, (uint64_t)CROSSTALK_EDA_UI_SCOPE_WINDOW_ID, (uint64_t)UINT64_MAX },
        { "Feed", "The feed displays cycle-stamped value changes of the passed signals. `feed <sig1> <sig2> <sig3> ...;` can be used to insert signals. `feed off;` disables the feed. Profiling needs to be toggled on.", 0x0, 0x0, (uint64_t)CROSSTALK_EDA_UI_FEED_WINDOW_ID, (uint64_t)UINT64_MAX },
        { "Indicator", "The indicator contains a set of test lights that can be used to test or diagnose designs. Profiling needs to be enabled for the indicator to function. `ind <sig>;` assigns a signal to the indicator, which will then flash based on the signal's value.", 0x0, 0x0, (uint64_t)CROSSTALK_EDA_UI_INDICATOR_WINDOW_ID, (uint64_t)UINT64_MAX },
        { "Pipeview", "The pipeline view can be used to visualize and diagnose core pipelining issues. It displays pipelines based on their stage signals. `pipeview <stage_signal> <another_stage_signal> ...;` inserts stage signals into pipeview.", 0x0, 0x0, (uint64_t)CROSSTALK_EDA_UI_PIPEVIEW_WINDOW_ID, (uint64_t)UINT64_MAX },
        { "Brainomatic", "The Brain'o'Matic is a helper window used to show a persistent banner with ISA details. Currently, it only shows architectural details of the DANGCSA/DANGXA architecture, which is native to the ecosystem of Crosstalk.", 0x0, 0x0, (uint64_t)CROSSTALK_EDA_UI_BRAINOMATIC_WINDOW_ID, (uint64_t)UINT64_MAX },
        { "Chat", "Chat windows are closed using ESC while focused. Re-opening a chat window requires going through the social hub, the same way as such a chat window was initially opened.", 0x0, 0x0, (uint64_t)CROSSTALK_GAME_CHAT_WINDOW_ID, (uint64_t)UINT64_MAX },
        { "Friends", "`^ShiftH`, or `<C-S-h>`, opens the social hub. Here, Steam and Discord friend lists (Discord if connected in the in-editor settings, accessible via the pause menu) can be seen, with an option to open a chat by pressing S or D + the index of the friend, for Steam and Discord, respectively.", 0x0, 0x0, (uint64_t)CROSSTALK_GAME_HUB_WINDOW_ID, (uint64_t)UINT64_MAX },
        { "Guide", "Hey, that's me!! The guide can be invoked by issuing `guide tour;` at any time while in an editor.", 0x0, 0x0, (uint64_t)CROSSTALK_UI_GUIDE_WINDOW_ID, (uint64_t)UINT64_MAX },
        { "Windows", "Most windows can be toggled by invoking their codename without any arguments, such as `memview;`. Windows have fixed indices that can be used to address them using `^<idx>`. Double-digit indices work as well, assuming they're both pressed within 150ms of each other. `^TAB` switches between the two most recently focused windows. `^Shift+TAB` cycles a window's tabs.", 0x0, 0x0, (uint64_t)UINT64_MAX, (uint64_t)UINT64_MAX },
        { "Demo projects", "You can now load one of two demo projects. The first one is a very simple SystemVerilog CPU core that puts most of the default windows that you can see to use. While focused on this Guide window, pressing `1` will load this first demo. The second demo is the DANGCSA/DANGXA emulator running DANGLING, which is this ecosystem's custom ISA + CPU design. Pressing `2` will load that instead.", 0x0, 0x0, (uint64_t)UINT64_MAX, (uint64_t)UINT64_MAX, CROSSTALK_CORE_GUIDE_DEMO_PICK_CUNT, LDG_ARR_ZERO_INIT },
        { "Exiting", "Pressing `ShiftESC` or `<S-Esc>` opens the pause menu. Besides being able to quit the game through it, settings are also available there. `^S` (`<C-s>`) saves the current design within the current editor on the project level, which should be done prior to exiting; progress will otherwise be lost. However, saving a design that way doesn't store any state that exists within a simulation or an emulation. The guide can be closed with the cmdline cmd `guide close;`.", 0x0, 0x0, (uint64_t)UINT64_MAX, (uint64_t)UINT64_MAX }
    };
    static const crosstalk_ui_guide_t tour = { "tour", tour_step, (uint32_t)(sizeof(tour_step) / sizeof(tour_step[0])), LDG_ARR_ZERO_INIT };

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_ui_guide_reg(&ctx->guide, &tour);
}

static uint32_t crosstalk_layout_sect_dir_rd(const char *path, uint32_t *out_len_arr, uint8_t **out_buff, uint32_t *out_len)
{
    uint8_t *buff = 0x0;
    uint32_t blob_len = 0;
    uint32_t span = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(path == 0x0 || out_len_arr == 0x0 || out_buff == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_buff = 0x0;
    *out_len = 0;

    ret = crosstalk_game_save_editor_sect_rd(path, CROSSTALK_GAME_SAVE_SECT_LAYOUT, 0x0, CROSSTALK_GAME_SAVE_SIZE_PROBE, &blob_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (blob_len <= (uint32_t)CROSSTALK_LAYOUT_DIR_BYTES) { return LDG_ERR_IO_FMT; }

    ret = ldg_mem_alloc((uint64_t)blob_len, (void **)&buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_game_save_editor_sect_rd(path, CROSSTALK_GAME_SAVE_SECT_LAYOUT, buff, blob_len, &blob_len);
    if (ret == LDG_ERR_AOK && memcmp(buff, CROSSTALK_LAYOUT_DIR_MAGIC, (uint64_t)CROSSTALK_LAYOUT_DIR_MAGIC_LEN) != 0x0) { ret = LDG_ERR_IO_FMT; }

    if (ret == LDG_ERR_AOK)
    {
        span = (uint32_t)CROSSTALK_LAYOUT_DIR_BYTES;

        for (at = 0; at < (uint32_t)CROSSTALK_CORE_EDITOR_KIND_CUNT; at = at + 0x1)
        {
            memcpy(&out_len_arr[at], buff + (uint64_t)CROSSTALK_LAYOUT_DIR_MAGIC_LEN + (uint64_t)at * sizeof(uint32_t), sizeof(uint32_t));
            span = span + out_len_arr[at];
        }

        if (span != blob_len) { ret = LDG_ERR_IO_FMT; }
    }

    if (ret != LDG_ERR_AOK)
    {
        ldg_mem_dealloc(buff);
        return ret;
    }

    *out_buff = buff;
    *out_len = blob_len;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_save_editor_mask_get(const char *path, uint8_t *out)
{
    uint32_t dir_len[CROSSTALK_CORE_EDITOR_KIND_CUNT] = LDG_ARR_ZERO_INIT;
    uint8_t *buff = 0x0;
    uint32_t blob_len = 0;
    uint32_t rec_cunt = 0;
    uint32_t off = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(path == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = crosstalk_layout_sect_dir_rd(path, dir_len, &buff, &blob_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    off = (uint32_t)CROSSTALK_LAYOUT_DIR_BYTES;

    for (at = 0; at < (uint32_t)CROSSTALK_CORE_EDITOR_KIND_CUNT; at = at + 0x1)
    {
        if (dir_len[at] >= (uint32_t)sizeof(uint32_t))
        {
            memcpy(&rec_cunt, buff + off, sizeof(uint32_t));

            if (rec_cunt != 0) { *out = (uint8_t)(*out | LDG_BIT(at)); }
        }

        off = off + dir_len[at];
    }

    ldg_mem_dealloc(buff);

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_editorpick_items_build(crosstalk_ctx_t *ctx)
{
    const char *name[CROSSTALK_CORE_EDITOR_KIND_CUNT] = { CROSSTALK_CORE_EDITOR_RTL_TEXT, CROSSTALK_CORE_EDITOR_RETIRED_TEXT, CROSSTALK_CORE_EDITOR_REVERSE_TEXT };
    uint64_t used = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->editorpick_items[0] = LDG_STR_TERM;
    ctx->editorpick_cunt = 0;
    ctx->editorpick_idx = 0;

    for (at = 0; at < (uint32_t)CROSSTALK_CORE_EDITOR_KIND_CUNT; at = at + 0x1)
    {
        if ((ctx->editorpick_mask & LDG_BIT(at)) == 0 || at == (uint32_t)CROSSTALK_CORE_NEWPROJECT_KIND_RETIRED) { continue; }

        if (used > 0)
        {
            if (ldg_str_rbrcpy(&ctx->editorpick_items[used], CROSSTALK_CORE_NEWPROJECT_SEP, (uint64_t)sizeof(ctx->editorpick_items) - used) != LDG_ERR_AOK) { break; }

            used = used + LDG_STR_TERM_SIZE;
        }

        if (ldg_str_rbrcpy(&ctx->editorpick_items[used], name[at], (uint64_t)sizeof(ctx->editorpick_items) - used) != LDG_ERR_AOK) { break; }

        used = used + strlen(name[at]);
        ctx->editorpick_kind[ctx->editorpick_cunt] = (uint8_t)at;
        ctx->editorpick_cunt = ctx->editorpick_cunt + 0x1;
    }

    return LDG_ERR_AOK;
}

#define CROSSTALK_GUIDE_COL_CUNT 64
#define CROSSTALK_GUIDE_FRAME_SIDES 2
#define CROSSTALK_GUIDE_SLACK_CELLS 1
#define CROSSTALK_GUIDE_FRAC_FULL 1.0
#define CROSSTALK_GUIDE_CENTER_DIV 2.0

static void crosstalk_guide_fit(crosstalk_ctx_t *ctx)
{
    uint32_t content_rows = 0;
    uint32_t desk_rows = 0;
    uint32_t frame_span = (uint32_t)CROSSTALK_UI_WINDOW_FRAME_CELLS * (uint32_t)CROSSTALK_GUIDE_FRAME_SIDES + (uint32_t)CROSSTALK_GUIDE_SLACK_CELLS;
    double frac_w = 0.0;
    double frac_h = 0.0;
    uint32_t ret = 0;

    if (ctx->term_col_cunt == 0 || ctx->term_row_cunt <= (uint32_t)CROSSTALK_STATUSLINE_ROWS) { return; }

    ret = crosstalk_ui_guide_extent_get(&ctx->guide, (uint32_t)CROSSTALK_GUIDE_COL_CUNT, &content_rows);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "guide: extent failed; err: %u32", ret);
        return;
    }

    desk_rows = ctx->term_row_cunt - (uint32_t)CROSSTALK_STATUSLINE_ROWS;
    frac_w = (double)((uint32_t)CROSSTALK_GUIDE_COL_CUNT + frame_span) / (double)ctx->term_col_cunt;
    frac_h = (double)(content_rows + frame_span) / (double)desk_rows;

    if (frac_w > CROSSTALK_GUIDE_FRAC_FULL) { frac_w = CROSSTALK_GUIDE_FRAC_FULL; }

    if (frac_h > CROSSTALK_GUIDE_FRAC_FULL) { frac_h = CROSSTALK_GUIDE_FRAC_FULL; }

    ctx->guide.win.rect_w = frac_w;
    ctx->guide.win.rect_h = frac_h;
    ctx->guide.win.rect_x = (CROSSTALK_GUIDE_FRAC_FULL - frac_w) / CROSSTALK_GUIDE_CENTER_DIV;
    ctx->guide.win.rect_y = (CROSSTALK_GUIDE_FRAC_FULL - frac_h) / CROSSTALK_GUIDE_CENTER_DIV;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "guide: fit; cols: %u32; rows: %u32; w: %.6f; h: %.6f", (uint32_t)CROSSTALK_GUIDE_COL_CUNT, content_rows, frac_w, frac_h);
}

static uint32_t crosstalk_layout_default_apply(crosstalk_ctx_t *ctx)
{
    const crosstalk_layout_default_t defaults[CROSSTALK_LAYOUT_DEFAULT_CUNT] = {
        { (uint64_t)CROSSTALK_EDA_UI_TERM_WINDOW_ID, { 0.512402, 0.312934, 0.479786, 0.580692 }, LDG_TRUTH_TRUE },
        { (uint64_t)CROSSTALK_EDA_UI_SCOPE_WINDOW_ID, { 0.216553, 0.776145, 0.294731, 0.117480 }, LDG_TRUTH_TRUE },
        { (uint64_t)CROSSTALK_EDA_UI_INDICATOR_WINDOW_ID, { 0.722900, 0.897328, 0.269287, 0.097964 }, LDG_TRUTH_TRUE },
        { (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_WINDOW_ID, { 0.310303, 0.005669, 0.681885, 0.303563 }, LDG_TRUTH_TRUE },
        { (uint64_t)CROSSTALK_EDA_UI_CALC_WINDOW_ID, { 0.002441, 0.312934, 0.305908, 0.246645 }, LDG_TRUTH_TRUE },
        { (uint64_t)CROSSTALK_EDA_UI_PROFILE_WINDOW_ID, { 0.150936, 0.563281, 0.157413, 0.209162 }, LDG_TRUTH_TRUE },
        { (uint64_t)CROSSTALK_EDA_UI_REGVIEW_WINDOW_ID, { 0.110474, 0.776145, 0.106571, 0.219146 }, LDG_TRUTH_TRUE },
        { (uint64_t)CROSSTALK_EDA_UI_STACKVIEW_WINDOW_ID, { 0.002441, 0.563281, 0.148987, 0.209162 }, LDG_TRUTH_TRUE },
        { (uint64_t)CROSSTALK_EDA_UI_BRAINOMATIC_WINDOW_ID, { 0.002441, 0.001202, 0.305908, 0.308030 }, LDG_TRUTH_TRUE },
        { (uint64_t)CROSSTALK_EDA_UI_PROGVIEW_WINDOW_ID, { 0.002441, 0.776145, 0.108524, 0.219146 }, LDG_TRUTH_TRUE },
        { (uint64_t)CROSSTALK_EDA_UI_PIPEVIEW_WINDOW_ID, { 0.310509, 0.585166, 0.201542, 0.187440 }, LDG_TRUTH_TRUE },
        { (uint64_t)CROSSTALK_EDA_UI_ANALYZER_WINDOW_ID, { 0.453003, 0.897328, 0.270389, 0.097964 }, LDG_TRUTH_TRUE },
        { (uint64_t)CROSSTALK_EDA_UI_FEED_WINDOW_ID, { 0.216553, 0.897328, 0.236942, 0.097964 }, LDG_TRUTH_TRUE },
        { (uint64_t)CROSSTALK_EDA_UI_DISPLAY_CRT_WINDOW_ID, { 0.310303, 0.312934, 0.202592, 0.277652 }, LDG_TRUTH_TRUE }
    };
    crosstalk_layout_win_t wins[CROSSTALK_LAYOUT_WIN_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t win_cunt = 0;
    uint32_t at = 0;
    uint32_t applied = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_layout_wins_get(ctx, wins, (uint32_t)CROSSTALK_LAYOUT_WIN_CAP, &win_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (at = 0; at < win_cunt; at = at + 0x1)
    {
        if (wins[at].win == 0x0) { continue; }

        wins[at].win->open = LDG_TRUTH_FALSE;
    }

    if (ctx->game.ld.kind == CROSSTALK_LD_BARE || ctx->game.ld.kind == CROSSTALK_LD_NEW) { if (crosstalk_ui_guide_at_start(&ctx->guide, (uint32_t)CROSSTALK_CORE_GUIDE_TOUR_IDX) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "guide tour start failed"); }
        else { crosstalk_guide_fit(ctx); } }

    if (ctx->editor_kind != (uint8_t)CROSSTALK_CORE_NEWPROJECT_KIND_RTL)
    {
        ctx->eda.term.open = ctx->eda.term.win.open;
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "layout: empty desk; editor: %u32; wins: %u32", (uint32_t)ctx->editor_kind, win_cunt);
        return LDG_ERR_AOK;
    }

    for (at = 0; at < (uint32_t)CROSSTALK_LAYOUT_DEFAULT_CUNT; at = at + 0x1)
    {
        uint32_t win_at = 0;

        for (win_at = 0; win_at < win_cunt; win_at = win_at + 0x1)
        {
            if (wins[win_at].id != defaults[at].id || wins[win_at].win == 0x0) { continue; }

            wins[win_at].win->rect_x = defaults[at].frac[0];
            wins[win_at].win->rect_y = defaults[at].frac[0x1];
            wins[win_at].win->rect_w = defaults[at].frac[0x2];
            wins[win_at].win->rect_h = defaults[at].frac[0x3];
            wins[win_at].win->open = defaults[at].open;
            applied = applied + 0x1;
            break;
        }
    }

    ctx->eda.term.open = ctx->eda.term.win.open;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "layout: defaults applied; editor: %u32; wins: %u32", (uint32_t)ctx->editor_kind, applied);
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_layout_sect_build(crosstalk_ctx_t *ctx, uint8_t **out_buff, uint32_t *out_len)
{
    crosstalk_layout_win_t wins[CROSSTALK_LAYOUT_WIN_CAP] = LDG_ARR_ZERO_INIT;
    uint8_t *buff = 0x0;
    uint32_t dir_len[CROSSTALK_CORE_EDITOR_KIND_CUNT] = LDG_ARR_ZERO_INIT;
    uint8_t *disk_buff = 0x0;
    uint32_t cunt = 0;
    uint32_t need = 0;
    uint32_t slot_len = 0;
    uint32_t disk_len = 0;
    uint32_t off = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_buff == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_buff = 0x0;
    *out_len = 0;

    ret = crosstalk_layout_wins_get(ctx, wins, (uint32_t)CROSSTALK_LAYOUT_WIN_CAP, &cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (cunt == 0) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(ctx->editor_kind >= (uint8_t)CROSSTALK_CORE_EDITOR_KIND_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    slot_len = (uint32_t)sizeof(uint32_t) + cunt * (uint32_t)CROSSTALK_LAYOUT_REC_SIZE + (uint32_t)sizeof(double) + (uint32_t)sizeof(uint8_t);

    for (at = 0; at < (uint32_t)CROSSTALK_CORE_EDITOR_KIND_CUNT; at = at + 0x1) { dir_len[at] = 0; }

    if (ctx->save_path[0] != LDG_STR_TERM) { if (crosstalk_layout_sect_dir_rd(ctx->save_path, dir_len, &disk_buff, &disk_len) != LDG_ERR_AOK)
        {
            disk_buff = 0x0;
            disk_len = 0;
        }
    }

    dir_len[ctx->editor_kind] = slot_len;
    need = (uint32_t)CROSSTALK_LAYOUT_DIR_BYTES;
    for (at = 0; at < (uint32_t)CROSSTALK_CORE_EDITOR_KIND_CUNT; at = at + 0x1) { need = need + dir_len[at]; }

    ret = ldg_mem_alloc((uint64_t)need, (void **)&buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (disk_buff != 0x0) { ldg_mem_dealloc(disk_buff); }

        return ret;
    }

    memset(buff, 0x0, (uint64_t)need);
    memcpy(buff, CROSSTALK_LAYOUT_DIR_MAGIC, (uint64_t)CROSSTALK_LAYOUT_DIR_MAGIC_LEN);
    off = (uint32_t)CROSSTALK_LAYOUT_DIR_MAGIC_LEN;

    for (at = 0; at < (uint32_t)CROSSTALK_CORE_EDITOR_KIND_CUNT; at = at + 0x1)
    {
        memcpy(buff + off, &dir_len[at], sizeof(uint32_t));
        off = off + (uint32_t)sizeof(uint32_t);
    }

    if (disk_buff != 0x0)
    {
        uint32_t src_at = (uint32_t)CROSSTALK_LAYOUT_DIR_BYTES;
        uint32_t dst_at = (uint32_t)CROSSTALK_LAYOUT_DIR_BYTES;

        for (at = 0; at < (uint32_t)CROSSTALK_CORE_EDITOR_KIND_CUNT; at = at + 0x1)
        {
            uint32_t src_len = 0;

            memcpy(&src_len, disk_buff + (uint64_t)CROSSTALK_LAYOUT_DIR_MAGIC_LEN + (uint64_t)at * sizeof(uint32_t), sizeof(uint32_t));

            if (at != (uint32_t)ctx->editor_kind && src_len > 0 && src_len == dir_len[at] && src_at + src_len <= disk_len) { memcpy(buff + dst_at, disk_buff + src_at, src_len); }

            src_at = src_at + src_len;
            dst_at = dst_at + dir_len[at];
        }

        ldg_mem_dealloc(disk_buff);
    }

    off = (uint32_t)CROSSTALK_LAYOUT_DIR_BYTES;
    for (at = 0; at < (uint32_t)ctx->editor_kind; at = at + 0x1) { off = off + dir_len[at]; }

    memcpy(buff + off, &cunt, sizeof(uint32_t));
    off = off + (uint32_t)sizeof(uint32_t);

    for (at = 0; at < cunt; at = at + 0x1)
    {
        double frac[CROSSTALK_LAYOUT_FRAC_CUNT] = LDG_ARR_ZERO_INIT;
        uint8_t open = wins[at].win->open != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

        frac[0] = wins[at].win->rect_x;
        frac[0x1] = wins[at].win->rect_y;
        frac[0x2] = wins[at].win->rect_w;
        frac[0x3] = wins[at].win->rect_h;

        memcpy(buff + off, &wins[at].id, sizeof(uint64_t));
        off = off + (uint32_t)sizeof(uint64_t);
        memcpy(buff + off, frac, sizeof(frac));
        off = off + (uint32_t)sizeof(frac);
        buff[off] = open;
        off = off + (uint32_t)sizeof(uint8_t);
    }

    {
        double gap = CROSSTALK_LAYOUT_GAP_NONE;
        uint8_t gap_on = LDG_TRUTH_FALSE;

        memcpy(buff + off, &gap, sizeof(double));
        off = off + (uint32_t)sizeof(double);
        buff[off] = gap_on;
        off = off + (uint32_t)sizeof(uint8_t);
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "layout: saved; editor: %u32; wins: %u32; slot bytes: %u32; bytes: %u32; stackview open: %u32", (uint32_t)ctx->editor_kind, cunt, slot_len, need, (uint32_t)ctx->eda.stackview.win.open);

    *out_buff = buff;
    *out_len = need;
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_layout_sect_restore(crosstalk_ctx_t *ctx)
{
    crosstalk_layout_win_t wins[CROSSTALK_LAYOUT_WIN_CAP] = LDG_ARR_ZERO_INIT;
    uint8_t *buff = 0x0;
    uint32_t dir_len[CROSSTALK_CORE_EDITOR_KIND_CUNT] = LDG_ARR_ZERO_INIT;
    uint32_t blob_len = 0;
    uint32_t slot_len = 0;
    uint32_t win_cunt = 0;
    uint32_t rec_cunt = 0;
    uint32_t off = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->save_path[0] == LDG_STR_TERM) { return LDG_ERR_NOT_FOUND; }

    ret = crosstalk_game_save_editor_sect_rd(ctx->save_path, CROSSTALK_GAME_SAVE_SECT_LAYOUT, 0x0, CROSSTALK_GAME_SAVE_SIZE_PROBE, &blob_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (blob_len < (uint32_t)sizeof(uint32_t)) { return LDG_ERR_INVALID; }

    if (LDG_UNLIKELY(ctx->editor_kind >= (uint8_t)CROSSTALK_CORE_EDITOR_KIND_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = crosstalk_layout_sect_dir_rd(ctx->save_path, dir_len, &buff, &blob_len);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "layout: sect not a slot directory; err: %u32", ret);
        return ret;
    }

    slot_len = dir_len[ctx->editor_kind];
    off = (uint32_t)CROSSTALK_LAYOUT_DIR_BYTES;
    for (at = 0; at < (uint32_t)ctx->editor_kind; at = at + 0x1) { off = off + dir_len[at]; }

    if (slot_len < (uint32_t)sizeof(uint32_t))
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "layout: slot empty; editor: %u32", (uint32_t)ctx->editor_kind);
        ldg_mem_dealloc(buff);

        return LDG_ERR_AOK;
    }

    memcpy(&rec_cunt, buff + off, sizeof(uint32_t));
    off = off + (uint32_t)sizeof(uint32_t);

    if (rec_cunt == 0)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "layout: slot empty; editor: %u32", (uint32_t)ctx->editor_kind);
        ldg_mem_dealloc(buff);
        return LDG_ERR_AOK;
    }

    if (slot_len != (uint32_t)sizeof(uint32_t) + rec_cunt * (uint32_t)CROSSTALK_LAYOUT_REC_SIZE + (uint32_t)sizeof(double) + (uint32_t)sizeof(uint8_t))
    {
        ldg_mem_dealloc(buff);
        return LDG_ERR_INVALID;
    }

    ret = crosstalk_layout_wins_get(ctx, wins, (uint32_t)CROSSTALK_LAYOUT_WIN_CAP, &win_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(buff);
        return ret;
    }

    for (at = 0; at < rec_cunt; at = at + 0x1)
    {
        double frac[CROSSTALK_LAYOUT_FRAC_CUNT] = LDG_ARR_ZERO_INIT;
        uint64_t id = 0;
        uint32_t win_at = 0;
        uint8_t open = 0;

        memcpy(&id, buff + off, sizeof(uint64_t));
        off = off + (uint32_t)sizeof(uint64_t);
        memcpy(frac, buff + off, sizeof(frac));
        off = off + (uint32_t)sizeof(frac);
        open = buff[off];
        off = off + (uint32_t)sizeof(uint8_t);

        for (win_at = 0; win_at < win_cunt; win_at = win_at + 0x1)
        {
            if (wins[win_at].id != id) { continue; }

            if (frac[0x2] > LDG_MATH_EPSILON && frac[0x3] > LDG_MATH_EPSILON)
            {
                wins[win_at].win->rect_x = frac[0];
                wins[win_at].win->rect_y = frac[0x1];
                wins[win_at].win->rect_w = frac[0x2];
                wins[win_at].win->rect_h = frac[0x3];
            }

            wins[win_at].win->open = open != LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

            if (id == (uint64_t)CROSSTALK_EDA_UI_STACKVIEW_WINDOW_ID) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "layout: stackview restored; open: %u32", (uint32_t)wins[win_at].win->open); }

            break;
        }
    }

    ctx->eda.term.open = ctx->eda.term.win.open;

    ldg_mem_dealloc(buff);
    return LDG_ERR_AOK;
}

static void crosstalk_verb_layout(crosstalk_ctx_t *ctx, const char *arg)
{
    crosstalk_layout_win_t wins[CROSSTALK_LAYOUT_WIN_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t cunt = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (arg != 0x0 && arg[0] != LDG_STR_TERM && strcmp(arg, CROSSTALK_CORE_LAYOUT_DUMP_TEXT) != 0x0)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "layout: verb unknown; arg: %.31s", arg);
        return;
    }

    ret = crosstalk_layout_wins_get(ctx, wins, (uint32_t)CROSSTALK_LAYOUT_WIN_CAP, &cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "layout: window table unavailable; err: %u32", ret);
        return;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "layout: dump begin; windows: %u32", cunt);

    for (at = 0; at < cunt; at = at + 0x1)
    {
        if (wins[at].win == 0x0) { continue; }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "layout: { 0x%016X64ULL, %.6f, %.6f, %.6f, %.6f, %u32 },", wins[at].id, wins[at].win->rect_x, wins[at].win->rect_y, wins[at].win->rect_w, wins[at].win->rect_h, (uint32_t)wins[at].win->open);
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "layout: dump end; windows: %u32", cunt);
}

static uint32_t crosstalk_calc_sect_restore(crosstalk_ctx_t *ctx)
{
    uint8_t *buff = 0x0;
    uint32_t blob_len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->save_path[0] == LDG_STR_TERM) { return LDG_ERR_NOT_FOUND; }

    ret = crosstalk_game_save_editor_sect_rd(ctx->save_path, CROSSTALK_GAME_SAVE_SECT_CALC, 0x0, CROSSTALK_GAME_SAVE_SIZE_PROBE, &blob_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (blob_len == 0) { return LDG_ERR_AOK; }

    ret = ldg_mem_alloc((uint64_t)blob_len, (void **)&buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_game_save_editor_sect_rd(ctx->save_path, CROSSTALK_GAME_SAVE_SECT_CALC, buff, blob_len, &blob_len);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_eda_ui_calc_state_deserialize(&ctx->eda.calc, buff, blob_len); }

    ldg_mem_dealloc(buff);
    return ret;
}

static uint32_t crosstalk_emu_sect_restore(crosstalk_ctx_t *ctx)
{
    uint8_t *buff = 0x0;
    uint32_t blob_len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->save_path[0] == LDG_STR_TERM) { return LDG_ERR_NOT_FOUND; }

    ret = crosstalk_game_save_editor_sect_rd(ctx->save_path, CROSSTALK_GAME_SAVE_SECT_EMU, 0x0, CROSSTALK_GAME_SAVE_SIZE_PROBE, &blob_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (blob_len == 0) { return LDG_ERR_AOK; }

    ret = ldg_mem_alloc((uint64_t)blob_len, (void **)&buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_game_save_editor_sect_rd(ctx->save_path, CROSSTALK_GAME_SAVE_SECT_EMU, buff, blob_len, &blob_len);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_eda_ic_fpga_emu_stat_deserialize(&ctx->emu_stat, buff, blob_len); }

    if (ret == LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu stat: restored; progs: %u32; run ms: %u64; hlt ms: %u64", ctx->emu_stat.prog_cunt, ctx->emu_stat.run_ms, ctx->emu_stat.hlt_ms); }

    ldg_mem_dealloc(buff);
    return ret;
}

static uint32_t crosstalk_reng_sect_restore(crosstalk_ctx_t *ctx)
{
    uint8_t *buff = 0x0;
    uint32_t blob_len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->save_path[0] == LDG_STR_TERM) { return LDG_ERR_NOT_FOUND; }

    ret = crosstalk_game_save_editor_sect_rd(ctx->save_path, CROSSTALK_GAME_SAVE_SECT_RENG, 0x0, CROSSTALK_GAME_SAVE_SIZE_PROBE, &blob_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (blob_len == 0) { return LDG_ERR_AOK; }

    ret = ldg_mem_alloc((uint64_t)blob_len, (void **)&buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_game_save_editor_sect_rd(ctx->save_path, CROSSTALK_GAME_SAVE_SECT_RENG, buff, blob_len, &blob_len);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_reng_deserialize(&ctx->reng, buff, blob_len); }

    ldg_mem_dealloc(buff);
    return ret;
}

static uint32_t crosstalk_reng_sect_build(crosstalk_ctx_t *ctx)
{
    uint32_t need = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->reng_sect_buff != 0x0)
    {
        ldg_mem_dealloc(ctx->reng_sect_buff);
        ctx->reng_sect_buff = 0x0;
        ctx->reng_sect_len = 0;
    }

    ret = crosstalk_reng_serialize(&ctx->reng, 0x0, CROSSTALK_GAME_SAVE_SIZE_PROBE, &need);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (need == 0) { return LDG_ERR_AOK; }

    ret = ldg_mem_alloc((uint64_t)need, (void **)&ctx->reng_sect_buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_reng_serialize(&ctx->reng, ctx->reng_sect_buff, need, &ctx->reng_sect_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(ctx->reng_sect_buff);
        ctx->reng_sect_buff = 0x0;
        ctx->reng_sect_len = 0;

        return ret;
    }

    ctx->reng.dirty = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

static uint32_t ld_finalize(crosstalk_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = LDG_LD_ACQUIRE(ctx->game.ld.err);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "crosstalk: ld_finalize; kind: %u32; ld_err: %u32", (uint32_t)ctx->game.ld.kind, ret);

    if (ctx->game.ld.kind == CROSSTALK_LD_NEW)
    {
        author_capture(ctx);
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: project author; name: %s; steam_id: %u64; account: %u32", ctx->game.author_name, ctx->game.author_steam_id, (uint32_t)(ctx->game.author_steam_id & (uint64_t)UINT32_MAX));
    }

    if (ret == LDG_ERR_AOK && (ctx->game.ld.kind == CROSSTALK_LD_FILE || ctx->game.ld.preloaded != 0) && ctx->game.ld.settings_present != 0) { settings_apply(ctx); }

    if (ctx->game.ld.kind != CROSSTALK_LD_STARTUP || ctx->game.ld.preloaded != 0)
    {
        ret = crosstalk_layout_default_apply(ctx);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: window layout defaults failed; err: %u32", ret); }
    }

    if (ctx->game.ld.kind == CROSSTALK_LD_FILE || ctx->game.ld.preloaded != 0)
    {
        ret = crosstalk_layout_sect_restore(ctx);
        if (ret == LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: window layout restored"); }
        else if (ret != LDG_ERR_NOT_FOUND) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: window layout restore failed; err: %u32", ret); }

        ret = crosstalk_calc_sect_restore(ctx);
        if (ret == LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: calc state restored"); }
        else if (ret != LDG_ERR_NOT_FOUND) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: calc restore failed; err: %u32", ret); }

        ret = crosstalk_reng_sect_restore(ctx);
        if (ret == LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: project layer restored"); }
        else if (ret != LDG_ERR_NOT_FOUND) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: project layer restore failed; err: %u32", ret); }

        ret = crosstalk_emu_sect_restore(ctx);
        if (ret != LDG_ERR_AOK && ret != LDG_ERR_NOT_FOUND) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: emu stat restore failed; err: %u32", ret); }

        ctx->rtl_restore_pending = LDG_TRUTH_TRUE;
    }

    ctx->game.screen = ctx->game.ld.tgt;
    ctx->game.ld_finalized = LDG_TRUTH_TRUE;
    ctx->game.ld_active = LDG_TRUTH_FALSE;

    return ret;
}

static void settings_default_fill(crosstalk_game_settings_t *settings)
{
    settings->tonemap_exposure = CROSSTALK_GAME_FX_EXPOSURE;
    settings->bloom_threshold = CROSSTALK_GAME_FX_BLOOM_THRESH;
    settings->bloom_intensity = CROSSTALK_GAME_FX_BLOOM_INTENSITY;
    settings->bloom_radius = CROSSTALK_GAME_FX_BLOOM_RADIUS;
    settings->chroma_amount = CROSSTALK_GAME_FX_CHROMA_AMOUNT;
    settings->flare_col_r = CROSSTALK_GAME_FX_FLARE_R;
    settings->flare_col_g = CROSSTALK_GAME_FX_FLARE_G;
    settings->flare_col_b = CROSSTALK_GAME_FX_FLARE_B;
    settings->flare_threshold = CROSSTALK_GAME_FX_FLARE_THRESH;
    settings->flare_intensity = CROSSTALK_GAME_FX_FLARE_INTENSITY;
    settings->flare_dispersal = CROSSTALK_GAME_FX_FLARE_DISPERSAL;
    settings->flare_halo_width = CROSSTALK_GAME_FX_FLARE_HALO;
    settings->dof_focus_range = CROSSTALK_GAME_FX_DOF_FOCUS_RANGE;
    settings->dof_blur_max = CROSSTALK_GAME_FX_DOF_BLUR_MAX;
    settings->dof_near = CROSSTALK_GAME_FX_DOF_NEAR;
    settings->dof_far = CROSSTALK_GAME_FX_DOF_FAR;
    settings->godray_col_r = CROSSTALK_GAME_FX_GODRAY_TINT_R;
    settings->godray_col_g = CROSSTALK_GAME_FX_GODRAY_TINT_G;
    settings->godray_col_b = CROSSTALK_GAME_FX_GODRAY_TINT_B;
    settings->godray_density = CROSSTALK_GAME_FX_GODRAY_DENSITY;
    settings->godray_decay = CROSSTALK_GAME_FX_GODRAY_DECAY;
    settings->godray_weight = CROSSTALK_GAME_FX_GODRAY_WEIGHT;
    settings->godray_exposure = CROSSTALK_GAME_FX_GODRAY_EXPOSURE;
    settings->grade_contrast = CROSSTALK_GAME_FX_GRADE_CONTRAST;
    settings->grade_saturation = CROSSTALK_GAME_FX_GRADE_SATURATION;
    settings->grade_brightness = CROSSTALK_GAME_FX_GRADE_BRIGHTNESS;
    settings->grade_temp = CROSSTALK_GAME_FX_GRADE_TEMP;
    settings->grade_vignette = CROSSTALK_GAME_FX_GRADE_VIGNETTE;
    settings->ao_radius = CROSSTALK_GAME_FX_AO_RADIUS;
    settings->ao_intensity = CROSSTALK_GAME_FX_AO_INTENSITY;
    settings->ao_power = CROSSTALK_GAME_FX_AO_POWER;
    settings->sharp_amount = CROSSTALK_GAME_FX_SHARP_AMOUNT;
    settings->aa_threshold = CROSSTALK_GAME_FX_AA_THRESH;
    settings->ssr_strength = CROSSTALK_GAME_FX_SSR_STRENGTH;
    settings->ssr_thickness = CROSSTALK_GAME_FX_SSR_THICKNESS;
    settings->grain_amount = CROSSTALK_GAME_FX_GRAIN_AMOUNT;
    settings->grade_lut_intensity = CROSSTALK_GAME_FX_GRADE_LUT_INTENSITY;
    settings->light_intensity = CROSSTALK_GAME_FX_LIGHT_INTENSITY;
    settings->res_w = CROSSTALK_WIN_W;
    settings->res_h = CROSSTALK_WIN_H;
    settings->ssaa = CROSSTALK_GAME_FX_SSAA_DEFAULT;
    settings->dof_quality = CROSSTALK_GAME_FX_DOF_QUALITY_DEFAULT;
    settings->tex_quality = UINT32_MAX;
    settings->aniso = UINT32_MAX;
    settings->shadow_quality = CROSSTALK_GAME_SHADOW_QUALITY_DEFAULT;
    settings->bloom_on = LDG_TRUTH_TRUE;
    settings->chroma_on = LDG_TRUTH_FALSE;
    settings->flare_on = LDG_TRUTH_TRUE;
    settings->dof_on = LDG_TRUTH_TRUE;
    settings->godray_on = LDG_TRUTH_TRUE;
    settings->grade_on = LDG_TRUTH_TRUE;
    settings->grade_lut_on = LDG_TRUTH_TRUE;
    settings->ao_on = LDG_TRUTH_TRUE;
    settings->ssr_on = LDG_TRUTH_FALSE;
    settings->sharp_on = LDG_TRUTH_TRUE;
    settings->grain_on = LDG_TRUTH_FALSE;
    settings->discord_vol_in = CROSSTALK_NET_DISCORD_VOL_DEFAULT_PCT;
    settings->discord_vol_out = CROSSTALK_NET_DISCORD_VOL_DEFAULT_PCT;
    settings->discord_on = LDG_TRUTH_TRUE;
    settings->discord_presence_on = LDG_TRUTH_TRUE;
    settings->music_src = (uint8_t)CROSSTALK_GAME_MUSIC_SRC_NATIVE;
    settings->music_presence_on = LDG_TRUTH_TRUE;
    settings->discord_voice_on = LDG_TRUTH_TRUE;
    settings->discord_self_mute = LDG_TRUTH_FALSE;
    settings->discord_self_deaf = LDG_TRUTH_FALSE;
    settings->focus_pause_on = LDG_TRUTH_FALSE;
    settings->aa_mode = CROSSTALK_GAME_AA_MODE_TAA;
}

static uint32_t crosstalk_author_sync_send(crosstalk_ctx_t *ctx)
{
    crosstalk_net_wire_author_t wire_author = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    wire_author.steam_id = ctx->game.author_steam_id;
    wire_author.created_epoch_ms = ctx->game.created_epoch_ms;
    wire_author.bench_ms_total = ctx->game.bench_ms_total;
    wire_author.sim_ms_total = ctx->game.sim_ms_total;
    snprintf(wire_author.name, sizeof(wire_author.name), "%.63s", ctx->game.author_name);
    snprintf(wire_author.board_name, sizeof(wire_author.board_name), "%.63s", ctx->board_name);

    ret = crosstalk_net_wire_send(&ctx->steam_net, CROSSTALK_NET_WIRE_KIND_AUTHOR, &wire_author, (uint32_t)sizeof(wire_author), CROSSTALK_NET_WIRE_RELIABLE);
    if (ret == LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "net: author sent; name: %.63s; board: %.63s; steam_id: %u64", wire_author.name, wire_author.board_name, wire_author.steam_id); }

    return ret;
}

static void crosstalk_peer_state_apply(crosstalk_ctx_t *ctx, const crosstalk_net_wire_peer_state_t *in)
{
    if (in->steam_id == 0) { return; }

    for (uint32_t idx = 0; idx < ctx->peer_state_cunt; idx++)
    {
        if (ctx->peer_state[idx].steam_id != in->steam_id) { continue; }

        ctx->peer_state[idx] = *in;
        return;
    }

    if (ctx->peer_state_cunt >= CROSSTALK_CORE_IDENT_CAP) { return; }

    ctx->peer_state[ctx->peer_state_cunt] = *in;
    ctx->peer_state_cunt++;
}

static void toast_push(crosstalk_ctx_t *ctx, const char *text)
{
    uint32_t rc = 0;

    rc = ldg_str_rbrcpy(ctx->toast_text, text, (uint64_t)sizeof(ctx->toast_text));
    if (LDG_UNLIKELY(rc != LDG_ERR_AOK && rc != LDG_ERR_STR_TRUNC)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "toast statusline copy failed; err: %u32", rc); }
    else { ctx->toast_until_ms = ldg_time_epoch_ms_get() + (uint64_t)CROSSTALK_CORE_TOAST_SHOW_MS; }

    rc = crosstalk_core_cmdline_out_push(&ctx->cmdline, text);
    if (LDG_UNLIKELY(rc != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "toast drawer push failed"); return; }

    rc = crosstalk_game_fx_sfx_chirp_play(&ctx->game.sfx_sound);
    if (LDG_UNLIKELY(rc != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "toast chirp failed"); }
}

static uint32_t crosstalk_probe_src_get(crosstalk_ctx_t *ctx, const crosstalk_hdl_exec_ctx_t **out_exec, const crosstalk_hdl_abi_t **out_abi)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_exec == 0x0 || out_abi == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_exec = 0x0;
    *out_abi = 0x0;

    if (crosstalk_eda_ic_fpga_slot_exec_get(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, out_exec) != LDG_ERR_AOK) { *out_exec = 0x0; }

    if (crosstalk_eda_ic_fpga_slot_abi_get(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, out_abi) != LDG_ERR_AOK) { *out_abi = 0x0; }

    if (*out_exec == 0x0 && *out_abi == 0x0) { return LDG_ERR_NOT_INIT; }

    return LDG_ERR_AOK;
}

static void crosstalk_term_attach(crosstalk_ctx_t *ctx, const char *port)
{
    const crosstalk_hdl_exec_ctx_t *exec = 0x0;
    const crosstalk_hdl_abi_t *abi = 0x0;
    crosstalk_eda_ui_term_sesh_t *sesh_ptr = 0x0;
    uint32_t probe_handle = UINT32_MAX;
    uint32_t stream_handle = UINT32_MAX;
    uint32_t sesh_idx = UINT32_MAX;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || port == 0x0)) { return; }

    ret = crosstalk_probe_src_get(ctx, &exec, &abi);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, "no running design"); return; }

    ret = crosstalk_eda_ui_term_sesh_next_get(&ctx->eda.term, &sesh_idx);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, "no free terminal tab"); return; }

    ret = crosstalk_eda_logic_probe_attach(&ctx->eda.logic_probe, CROSSTALK_CORE_TERM_SLOT, exec, abi, port, &probe_handle);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term: attach failed; port: %.63s; err: %u32", port, ret);
        toast_push(ctx, "no such port in the design");
        return;
    }

    ret = crosstalk_eda_logic_stream_open(&ctx->eda.logic_stream, probe_handle, CROSSTALK_EDA_LOGIC_STREAM_DECODE_BYTE_PORT, &stream_handle);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, "no free stream"); return; }

    ret = crosstalk_eda_ui_term_sesh_clear(&ctx->eda.term, sesh_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term session clear failed"); }

    ret = crosstalk_eda_ui_term_sesh_bind(&ctx->eda.term, sesh_idx, CROSSTALK_EDA_UI_TERM_SRC_STREAM, stream_handle, port);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term session bind failed"); }

    ret = crosstalk_eda_ui_term_sesh_ptr_get(&ctx->eda.term, sesh_idx, &sesh_ptr);

    if (ret == LDG_ERR_AOK) { ret = crosstalk_eda_logic_stream_sink_add(&ctx->eda.logic_stream, stream_handle, crosstalk_eda_ui_term_sesh_sink_cb, sesh_ptr); }

    if (ret != LDG_ERR_AOK) { toast_push(ctx, "terminal sink refused"); return; }

    ret = crosstalk_eda_ui_term_active_set(&ctx->eda.term, sesh_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term active set failed"); }

    ret = crosstalk_eda_ui_term_open_set(&ctx->eda.term, LDG_TRUTH_TRUE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term open failed"); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "term: attached; slot: %u32; tab: %u32; port: %.63s; probe: %u32; stream: %u32", (uint32_t)CROSSTALK_CORE_TERM_SLOT, sesh_idx, port, probe_handle, stream_handle);
    toast_push(ctx, "terminal attached");
}

static void crosstalk_indicator_attach(crosstalk_ctx_t *ctx, const char *port)
{
    const crosstalk_hdl_exec_ctx_t *exec = 0x0;
    const crosstalk_hdl_abi_t *abi = 0x0;
    uint32_t probe_handle = UINT32_MAX;
    uint32_t stream_handle = UINT32_MAX;
    uint32_t width = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || port == 0x0)) { return; }

    ret = crosstalk_probe_src_get(ctx, &exec, &abi);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, "no running design"); return; }

    ret = crosstalk_eda_logic_probe_attach(&ctx->eda.logic_probe, CROSSTALK_CORE_TERM_SLOT, exec, abi, port, &probe_handle);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "ind: attach failed; port: %.63s; err: %u32", port, ret);
        toast_push(ctx, "no such port in the design");
        return;
    }

    width = ctx->eda.logic_probe.probe[probe_handle].width;

    ret = crosstalk_eda_logic_stream_open(&ctx->eda.logic_stream, probe_handle, CROSSTALK_EDA_LOGIC_STREAM_DECODE_LEVEL, &stream_handle);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, "no free stream"); return; }

    ret = crosstalk_eda_ui_indicator_bind(&ctx->eda.indicator, stream_handle, width, port);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, "signal too wide"); return; }

    ret = crosstalk_eda_ui_indicator_open_set(&ctx->eda.indicator, LDG_TRUTH_TRUE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "indicator open failed"); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "ind: attached; port: %.63s; width: %u32; probe: %u32; stream: %u32", port, width, probe_handle, stream_handle);
    toast_push(ctx, "indicators attached");
}

static void crosstalk_scope_release(crosstalk_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (ctx->scope_stream_handle != UINT32_MAX)
    {
        ret = crosstalk_eda_logic_stream_close(&ctx->eda.logic_stream, ctx->scope_stream_handle);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scope: stream close failed; stream: %u32; err: %u32", ctx->scope_stream_handle, ret); }

        ctx->scope_stream_handle = UINT32_MAX;
    }

    if (ctx->scope_probe_handle != UINT32_MAX)
    {
        if (ctx->scope_probe_owned == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_eda_logic_probe_detach(&ctx->eda.logic_probe, ctx->scope_probe_handle);
            if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scope: probe detach failed; probe: %u32; err: %u32", ctx->scope_probe_handle, ret); }
        }

        ctx->scope_probe_handle = UINT32_MAX;
        ctx->scope_probe_owned = LDG_TRUTH_FALSE;
    }
}

static void crosstalk_scope_sig_attach(crosstalk_ctx_t *ctx, const char *port, uint8_t quiet)
{
    const crosstalk_hdl_exec_ctx_t *exec = 0x0;
    const crosstalk_hdl_abi_t *abi = 0x0;
    uint64_t f_hz = 0;
    uint64_t freq = 0;
    uint64_t step_ns = 0;
    uint32_t probe_handle = UINT32_MAX;
    uint32_t stream_handle = UINT32_MAX;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || port == 0x0)) { return; }

    ret = crosstalk_probe_src_get(ctx, &exec, &abi);
    if (ret != LDG_ERR_AOK)
    {
        if (quiet != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_PIPE_NO_DESIGN_TEXT); }

        return;
    }

    f_hz = ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT].comp.f_op_hz;

    if (crosstalk_eda_clk_domain_freq_get(&ctx->eda.clk, ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT].clk_domain, &freq) == LDG_ERR_AOK && freq != 0) { f_hz = freq; }

    if (f_hz == 0)
    {
        if (quiet != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_SOUND_NO_CLK_TEXT); }

        return;
    }

    step_ns = (uint64_t)LDG_NS_PER_SEC / f_hz;
    if (step_ns == 0) { step_ns = (uint64_t)CROSSTALK_CORE_SCOPE_STEP_NS_MIN; }

    crosstalk_scope_release(ctx);

    ret = crosstalk_eda_logic_probe_attach(&ctx->eda.logic_probe, CROSSTALK_CORE_TERM_SLOT, exec, abi, port, &probe_handle);
    if (ret == LDG_ERR_AOK) { ctx->scope_probe_owned = LDG_TRUTH_TRUE; }
    else
    {
        ret = crosstalk_eda_logic_probe_find(&ctx->eda.logic_probe, CROSSTALK_CORE_TERM_SLOT, port, &probe_handle);
        ctx->scope_probe_owned = LDG_TRUTH_FALSE;
    }

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scope: attach failed; port: %.63s; err: %u32; quiet: %u32", port, ret, quiet);
        if (quiet != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_PIPE_NO_SIG_TEXT); }

        return;
    }

    ctx->scope_probe_handle = probe_handle;

    ret = crosstalk_eda_logic_stream_open(&ctx->eda.logic_stream, probe_handle, CROSSTALK_EDA_LOGIC_STREAM_DECODE_SAMPLE, &stream_handle);
    if (ret != LDG_ERR_AOK)
    {
        if (quiet != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_PIPE_NO_STREAM_TEXT); }

        return;
    }

    ctx->scope_stream_handle = stream_handle;

    ret = crosstalk_eda_logic_stream_width_set(&ctx->eda.logic_stream, stream_handle, ctx->eda.logic_probe.probe[probe_handle].width);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scope width set failed"); }

    ret = crosstalk_eda_ui_scope_sig_attach(&ctx->eda.scope, port, step_ns);
    if (ret != LDG_ERR_AOK)
    {
        if (quiet != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_SCOPE_FAIL_TEXT); }

        return;
    }

    ret = crosstalk_eda_logic_stream_sink_add(&ctx->eda.logic_stream, stream_handle, crosstalk_eda_ui_scope_sink_cb, &ctx->eda.scope);
    if (ret != LDG_ERR_AOK)
    {
        if (quiet != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_PIPE_NO_SINK_TEXT); }

        return;
    }

    ret = crosstalk_eda_ui_scope_open_set(&ctx->eda.scope, LDG_TRUTH_TRUE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scope open failed"); }

    ctx->scope_prog_gen = ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT].prog_gen;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "scope: signal probe; port: %.63s; width: %u32; step: %u64 ns; probe: %u32; stream: %u32; gen: %u32", port, ctx->eda.logic_probe.probe[probe_handle].width, step_ns, probe_handle, stream_handle, ctx->scope_prog_gen);
    if (quiet != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_SCOPE_SIG_TEXT); }
}

static void crosstalk_scope_rebind_tick(crosstalk_ctx_t *ctx)
{
    char port[CROSSTALK_EDA_UI_SCOPE_SIG_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t gen = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (ctx->eda.scope.src != (uint32_t)CROSSTALK_EDA_UI_SCOPE_SRC_SIG) { return; }

    if (ctx->eda.scope.sig[0] == LDG_STR_TERM) { return; }

    gen = ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT].prog_gen;
    if (gen == ctx->scope_prog_gen) { return; }

    ctx->scope_prog_gen = gen;

    ret = ldg_str_rbrcpy(port, ctx->eda.scope.sig, (uint64_t)sizeof(port));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return; }

    ret = crosstalk_eda_ui_scope_detach(&ctx->eda.scope);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scope rebind detach failed"); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "scope: rebinding after a rebuild; port: %.63s; gen: %u32", port, gen);

    crosstalk_scope_sig_attach(ctx, port, LDG_TRUTH_TRUE);
}

static void crosstalk_stackview_lane_attach(crosstalk_ctx_t *ctx, const char *sig)
{
    const crosstalk_hdl_exec_ctx_t *exec = 0x0;
    const crosstalk_hdl_abi_t *abi = 0x0;
    uint32_t probe_handle = UINT32_MAX;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || sig == 0x0)) { return; }

    ret = crosstalk_probe_src_get(ctx, &exec, &abi);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_PIPE_NO_DESIGN_TEXT); return; }

    ret = crosstalk_eda_logic_probe_attach(&ctx->eda.logic_probe, CROSSTALK_CORE_TERM_SLOT, exec, abi, sig, &probe_handle);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "stackview: probe attach failed; sig: %.63s; err: %u32", sig, ret);
        toast_push(ctx, CROSSTALK_CORE_PIPE_NO_SIG_TEXT);

        return;
    }

    ret = crosstalk_eda_ui_stackview_lane_add(&ctx->eda.stackview, sig, probe_handle);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "stackview: lane refused; sig: %.63s; err: %u32", sig, ret);

        if (crosstalk_eda_logic_probe_detach(&ctx->eda.logic_probe, probe_handle) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "stackview probe release failed"); }

        return;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "stackview: history lane added; sig: %.63s; probe: %u32", sig, probe_handle);
}

static void crosstalk_stackview_lane_release(crosstalk_ctx_t *ctx, const char *sig)
{
    uint32_t probe_handle = UINT32_MAX;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || sig == 0x0)) { return; }

    ret = crosstalk_eda_ui_stackview_lane_handle_get(&ctx->eda.stackview, sig, &probe_handle);
    if (ret != LDG_ERR_AOK) { return; }

    if (probe_handle != UINT32_MAX) { if (crosstalk_eda_logic_probe_detach(&ctx->eda.logic_probe, probe_handle) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "stackview probe detach failed"); } }

    ret = crosstalk_eda_ui_stackview_lane_remove(&ctx->eda.stackview, sig);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "stackview: lane remove failed; sig: %.63s; err: %u32", sig, ret); return; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "stackview: history lane released; sig: %.63s", sig);
}

static void crosstalk_ach_once(crosstalk_ctx_t *ctx, const char *name)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0)) { return; }

    if (crosstalk_game_progression_achievement_unlocked_is(name) == LDG_TRUTH_TRUE) { return; }

    ret = crosstalk_game_progression_achievement_unlock_req(name, CROSSTALK_APP_ID, &ctx->progression_queue);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "crosstalk: ach req; name: %s; err: %u32", name, ret); }
}

static void crosstalk_pack_start(crosstalk_ctx_t *ctx)
{
    crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    char src_path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char out_path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char cc_dir[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    slot = &ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT];

    if (slot->runnable != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_PIPE_NO_DESIGN_TEXT); return; }

    if (crosstalk_eda_ic_fpga_build_live_is(&ctx->eda.fpga_build) == LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_PACK_LIVE_TEXT); return; }

    if (slot->abi != 0x0)
    {
        ret = crosstalk_eda_ic_fpga_run_hlt(&ctx->eda.fpga_run);
        if (ret != LDG_ERR_AOK && ret != CROSSTALK_EDA_IC_FPGA_RUN_ERR_IDLE) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pack halt failed"); }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "pack: re-bake halts the running component");
    }

    ret = crosstalk_core_runtime_path_resolve(CROSSTALK_CORE_PACK_SRC_FILE, src_path, (uint64_t)sizeof(src_path));
    if (ret == LDG_ERR_AOK) { ret = crosstalk_core_runtime_path_resolve(CROSSTALK_CORE_PACK_OUT_FILE, out_path, (uint64_t)sizeof(out_path)); }

    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_PACK_FAIL_TEXT); return; }

    if (ldg_io_path_prog_get(cc_dir, (uint64_t)sizeof(cc_dir)) == LDG_ERR_AOK)
    {
        char cc_bin[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
        char cc_join[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;

        const char *cc_rel = (ldg_io_path_sep_get() == '\\') ? CROSSTALK_CORE_PACK_CC_REL_WIN : CROSSTALK_CORE_PACK_CC_REL_UNIX;

        if (ldg_io_path_parent_get(cc_dir, cc_bin, (uint64_t)sizeof(cc_bin)) == LDG_ERR_AOK && ldg_io_path_join(cc_bin, cc_rel, cc_join, (uint64_t)sizeof(cc_join)) == LDG_ERR_AOK) { ret = ldg_str_rbrcpy(cc_dir, cc_join, (uint64_t)sizeof(cc_dir)); }
    }

    if (ctx->eda.fpga_build.ok != LDG_TRUTH_TRUE)
    {
        ret = crosstalk_eda_ic_fpga_build_init(&ctx->eda.fpga_build, cc_dir);
        if (ret != LDG_ERR_AOK)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pack: build init failed; err: %u32", ret);
            toast_push(ctx, CROSSTALK_CORE_PACK_FAIL_TEXT);
            return;
        }

        if (crosstalk_eda_ic_fpga_guard_bind(&ctx->eda.fpga, &ctx->eda.fpga_build.guard) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga guard bind failed"); }
    }

    ret = crosstalk_eda_ic_fpga_build_start(&ctx->eda.fpga_build, &slot->ir, slot->elab_src, src_path, out_path, slot->top, CROSSTALK_CORE_TERM_SLOT, crosstalk_eda_ui_profile_open_is(&ctx->eda.profile));
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pack: build start failed; err: %u32", ret);
        toast_push(ctx, CROSSTALK_CORE_PACK_FAIL_TEXT);
        return;
    }

    ctx->pack_prog_gen = slot->prog_gen;

    toast_push(ctx, CROSSTALK_CORE_PACK_START_TEXT);
}

static void crosstalk_serial_lane_push(crosstalk_ctx_t *ctx, uint32_t handle, uint64_t *sample_at, const uint8_t *bytes, uint32_t len)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || sample_at == 0x0 || bytes == 0x0)) { return; }

    if (handle == UINT32_MAX) { return; }

    for (at = 0; at < len; at = at + 0x1)
    {
        if (crosstalk_eda_logic_probe_sample_push(&ctx->eda.logic_probe, handle, (uint64_t)bytes[at], *sample_at) != LDG_ERR_AOK) { break; }

        *sample_at = *sample_at + 0x1;
    }
}

static void crosstalk_serial_lanes_release(crosstalk_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (ctx->serial_rx_probe_handle != UINT32_MAX)
    {
        if (crosstalk_eda_logic_probe_detach(&ctx->eda.logic_probe, ctx->serial_rx_probe_handle) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "serial rx lane detach failed"); }

        ctx->serial_rx_probe_handle = UINT32_MAX;
    }

    if (ctx->serial_tx_probe_handle != UINT32_MAX)
    {
        if (crosstalk_eda_logic_probe_detach(&ctx->eda.logic_probe, ctx->serial_tx_probe_handle) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "serial tx lane detach failed"); }

        ctx->serial_tx_probe_handle = UINT32_MAX;
    }

    ctx->serial_rx_sample_at = 0;
    ctx->serial_tx_sample_at = 0;
}

static void crosstalk_serial_lanes_attach(crosstalk_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    crosstalk_serial_lanes_release(ctx);

    ret = crosstalk_eda_logic_probe_host_attach(&ctx->eda.logic_probe, CROSSTALK_CORE_TERM_SLOT, CROSSTALK_CORE_SERIAL_LANE_RX, (uint32_t)LDG_BYTE_BITS, &ctx->serial_rx_probe_handle);
    if (ret == LDG_ERR_EXISTS) { ret = crosstalk_eda_logic_probe_find(&ctx->eda.logic_probe, CROSSTALK_CORE_TERM_SLOT, CROSSTALK_CORE_SERIAL_LANE_RX, &ctx->serial_rx_probe_handle); }

    if (ret != LDG_ERR_AOK)
    {
        ctx->serial_rx_probe_handle = UINT32_MAX;

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "serial: rx lane refused; err: %u32", ret);
    }

    ret = crosstalk_eda_logic_probe_host_attach(&ctx->eda.logic_probe, CROSSTALK_CORE_TERM_SLOT, CROSSTALK_CORE_SERIAL_LANE_TX, (uint32_t)LDG_BYTE_BITS, &ctx->serial_tx_probe_handle);
    if (ret == LDG_ERR_EXISTS) { ret = crosstalk_eda_logic_probe_find(&ctx->eda.logic_probe, CROSSTALK_CORE_TERM_SLOT, CROSSTALK_CORE_SERIAL_LANE_TX, &ctx->serial_tx_probe_handle); }

    if (ret != LDG_ERR_AOK)
    {
        ctx->serial_tx_probe_handle = UINT32_MAX;

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "serial: tx lane refused; err: %u32", ret);
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "serial: lanes up; rx probe: %u32; tx probe: %u32; names: %s %s", ctx->serial_rx_probe_handle, ctx->serial_tx_probe_handle, CROSSTALK_CORE_SERIAL_LANE_RX, CROSSTALK_CORE_SERIAL_LANE_TX);
}

static void crosstalk_serial_access_data_send(crosstalk_ctx_t *ctx, uint32_t dir, const uint8_t *bytes, uint32_t len)
{
    crosstalk_net_wire_serial_data_t data = LDG_STRUCT_ZERO_INIT;
    uint32_t at = 0;
    uint32_t part = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || bytes == 0x0)) { return; }

    while (at < len)
    {
        part = len - at;
        if (part > (uint32_t)CROSSTALK_NET_WIRE_SERIAL_CHUNK_CAP) { part = (uint32_t)CROSSTALK_NET_WIRE_SERIAL_CHUNK_CAP; }

        data.dir = dir;
        data.len = part;
        memcpy(data.bytes, &bytes[at], (uint64_t)part);

        ret = crosstalk_net_wire_send(&ctx->steam_net, (uint8_t)CROSSTALK_NET_WIRE_KIND_SERIAL_DATA, &data, (uint32_t)CROSSTALK_NET_WIRE_SERIAL_DATA_HDR_SIZE + part, CROSSTALK_NET_WIRE_RELIABLE);
        if (ret != LDG_ERR_AOK && ret != LDG_ERR_NOT_INIT) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "serial: access data send failed; err: %u32; dir: %u32; len: %u32", ret, dir, part); }

        if (ret != LDG_ERR_AOK) { return; }

        at = at + part;
    }
}

static void crosstalk_serial_access_ctl_send(crosstalk_ctx_t *ctx, uint32_t op)
{
    crosstalk_net_wire_serial_ctl_t ctl = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    ctl.op = op;
    ret = ldg_str_rbrcpy(ctl.id, ctx->eda.serial.open_id, (uint64_t)sizeof(ctl.id));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return; }

    ret = crosstalk_net_wire_send(&ctx->steam_net, (uint8_t)CROSSTALK_NET_WIRE_KIND_SERIAL_CTL, &ctl, (uint32_t)sizeof(ctl), CROSSTALK_NET_WIRE_RELIABLE);
    if (ret != LDG_ERR_AOK && ret != LDG_ERR_NOT_INIT) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "serial: access ctl send failed; err: %u32; op: %u32", ret, op); }
}

static uint32_t crosstalk_serial_rx_cb(void *user, const uint8_t *bytes, uint32_t len)
{
    crosstalk_ctx_t *ctx = (crosstalk_ctx_t *)user;
    uint32_t at = 0;
    uint32_t map_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || bytes == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    crosstalk_serial_lane_push(ctx, ctx->serial_rx_probe_handle, &ctx->serial_rx_sample_at, bytes, len);

    for (at = 0; at < len && ctx->serial_map_cunt != 0; at = at + 0x1)
    {
        if (ctx->serial_map_window_len >= (uint32_t)CROSSTALK_CORE_SERIAL_MAP_PATTERN_CAP)
        {
            memmove(ctx->serial_map_window, &ctx->serial_map_window[LDG_STR_TERM_SIZE], (uint64_t)(ctx->serial_map_window_len - LDG_STR_TERM_SIZE));
            ctx->serial_map_window_len = ctx->serial_map_window_len - LDG_STR_TERM_SIZE;
        }

        ctx->serial_map_window[ctx->serial_map_window_len] = bytes[at];
        ctx->serial_map_window_len = ctx->serial_map_window_len + 0x1;

        for (map_idx = 0; map_idx < ctx->serial_map_cunt; map_idx = map_idx + 0x1)
        {
            uint32_t pattern_len = ctx->serial_map_pattern_len[map_idx];

            if (pattern_len == 0 || pattern_len > ctx->serial_map_window_len) { continue; }

            if (memcmp(&ctx->serial_map_window[ctx->serial_map_window_len - pattern_len], ctx->serial_map_pattern[map_idx], (uint64_t)pattern_len) != 0) { continue; }

            if (ctx->serial_map_pending_has == LDG_TRUTH_TRUE) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "serial: match dropped; a mapped line is already pending"); continue; }

            if (ctx->serial_map_action[map_idx][0] != LDG_STR_TERM) { if (ldg_str_rbrcpy(ctx->serial_map_pending, ctx->serial_map_action[map_idx], (uint64_t)sizeof(ctx->serial_map_pending)) != LDG_ERR_AOK) { continue; } }
            else
            {
                uint32_t line_len = pattern_len;

                while (line_len != 0 && (ctx->serial_map_pattern[map_idx][line_len - 0x1] == '\n' || ctx->serial_map_pattern[map_idx][line_len - 0x1] == '\r')) { line_len = line_len - 0x1; }

                if (line_len + LDG_STR_TERM_SIZE > (uint32_t)sizeof(ctx->serial_map_pending)) { continue; }

                memcpy(ctx->serial_map_pending, ctx->serial_map_pattern[map_idx], (uint64_t)line_len);
                ctx->serial_map_pending[line_len] = LDG_STR_TERM;
            }

            {
                uint32_t pend_len = 0;

                while (ctx->serial_map_pending[pend_len] != LDG_STR_TERM) { pend_len = pend_len + 0x1; }

                while (pend_len != 0 && ctx->serial_map_pending[pend_len - 0x1] == ' ') { pend_len = pend_len - 0x1; }

                if (pend_len == 0) { continue; }

                if (ctx->serial_map_pending[pend_len - 0x1] != CROSSTALK_CORE_SERIAL_MAP_TERM_CHAR)
                {
                    if (pend_len + LDG_STR_TERM_SIZE >= (uint32_t)sizeof(ctx->serial_map_pending)) { continue; }

                    ctx->serial_map_pending[pend_len] = CROSSTALK_CORE_SERIAL_MAP_TERM_CHAR;
                    pend_len = pend_len + 0x1;
                }

                ctx->serial_map_pending[pend_len] = LDG_STR_TERM;
            }

            ctx->serial_map_pending_has = LDG_TRUTH_TRUE;
            ctx->serial_map_window_len = 0;

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "serial: listener matched; map: %u32; line: %.63s", map_idx, ctx->serial_map_pending);
            break;
        }
    }

    if (crosstalk_eda_ui_term_serial_live_is(&ctx->eda.term) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_eda_ui_term_sink_cb(&ctx->eda.term, bytes, len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "serial rx term sink failed"); }
    }

    if (crosstalk_eda_io_receptacle_live_is(&ctx->eda.receptacle) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_eda_io_receptacle_put(&ctx->eda.receptacle, bytes, len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != CROSSTALK_EDA_IO_RECEPTACLE_ERR_FULL)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "serial rx receptacle put failed"); }
    }

    if (crosstalk_eda_io_serial_remote_is(&ctx->eda.serial) != LDG_TRUTH_TRUE) { crosstalk_serial_access_data_send(ctx, (uint32_t)CROSSTALK_NET_WIRE_SERIAL_DIR_RX, bytes, len); }

    return LDG_ERR_AOK;
}

static void crosstalk_serial_tick(crosstalk_ctx_t *ctx)
{
    crosstalk_eda_io_serial_event_t event[CROSSTALK_CORE_SERIAL_EVENT_CAP] = LDG_ARR_ZERO_INIT;
    uint8_t keys[CROSSTALK_EDA_UI_TERM_KEY_CAP] = LDG_ARR_ZERO_INIT;
    uint8_t txb[CROSSTALK_NET_WIRE_SERIAL_CHUNK_CAP] = LDG_ARR_ZERO_INIT;
    char toast[CROSSTALK_CORE_SERIAL_TOAST_CAP] = LDG_ARR_ZERO_INIT;
    const char *tail = 0x0;
    uint32_t txn = 0;
    uint32_t key_cunt = 0;
    uint32_t cunt = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    ret = crosstalk_eda_io_serial_poll(&ctx->eda.serial, ctx->dt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_NOT_INIT)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "serial poll failed"); }

    if (crosstalk_eda_io_serial_events_drain(&ctx->eda.serial, event, (uint32_t)CROSSTALK_CORE_SERIAL_EVENT_CAP, &cunt) == LDG_ERR_AOK) { for (idx = 0; idx < cunt; idx++)
        {
            if (crosstalk_eda_io_serial_id_tail_get(event[idx].id, &tail) != LDG_ERR_AOK) { tail = event[idx].id; }

            snprintf(toast, sizeof(toast), event[idx].kind == (uint32_t)CROSSTALK_EDA_IO_SERIAL_EVENT_ARRIVE ? CROSSTALK_CORE_SERIAL_ARRIVE_FMT : CROSSTALK_CORE_SERIAL_DEPART_FMT, event[idx].path, tail);
            toast_push(ctx, toast);
        }
    }

    if (crosstalk_eda_io_serial_attached_is(&ctx->eda.serial) != LDG_TRUTH_TRUE) { return; }

    if (crosstalk_eda_ui_term_serial_live_is(&ctx->eda.term) == LDG_TRUTH_TRUE) { if (crosstalk_eda_ui_term_keys_drain(&ctx->eda.term, keys, (uint32_t)CROSSTALK_EDA_UI_TERM_KEY_CAP, &key_cunt) == LDG_ERR_AOK && key_cunt != 0)
        {
            ret = crosstalk_eda_io_serial_tx_put(&ctx->eda.serial, keys, key_cunt);
            if (ret != LDG_ERR_AOK && ret != CROSSTALK_EDA_IO_SERIAL_ERR_TX_FULL) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "serial tx put failed"); }
            else if (ret == LDG_ERR_AOK) { crosstalk_serial_lane_push(ctx, ctx->serial_tx_probe_handle, &ctx->serial_tx_sample_at, keys, key_cunt); }
        }
    }

    ret = crosstalk_eda_io_cast_tick(&ctx->eda.cast, &ctx->eda.fpga, (uint32_t)CROSSTALK_CORE_TERM_SLOT, &ctx->eda.serial, ctx->time);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cast tick failed"); }

    ret = crosstalk_eda_io_serial_pump(&ctx->eda.serial, crosstalk_serial_rx_cb, ctx);

    if (ret == CROSSTALK_EDA_IO_SERIAL_ERR_BUSTED)
    {
        crosstalk_serial_access_ctl_send(ctx, (uint32_t)CROSSTALK_NET_WIRE_SERIAL_CTL_REVOKE);
        toast_push(ctx, CROSSTALK_CORE_SERIAL_BUSTED_TEXT);
    }

    if (crosstalk_eda_io_serial_remote_is(&ctx->eda.serial) == LDG_TRUTH_TRUE)
    {
        while (crosstalk_eda_io_serial_remote_tx_take(&ctx->eda.serial, txb, (uint32_t)CROSSTALK_NET_WIRE_SERIAL_CHUNK_CAP, &txn) == LDG_ERR_AOK && txn != 0) { crosstalk_serial_access_data_send(ctx, (uint32_t)CROSSTALK_NET_WIRE_SERIAL_DIR_TX, txb, txn); }

        if (ctx->serial_access_last != 0.0 && ctx->time - ctx->serial_access_last > CROSSTALK_CORE_SERIAL_ACCESS_TIMEOUT_SECS)
        {
            ret = crosstalk_eda_io_serial_detach(&ctx->eda.serial);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "serial remote detach failed"); }

            ctx->serial_access_owner = 0;
            ctx->serial_access_last = 0.0;
            toast_push(ctx, CROSSTALK_CORE_SERIAL_ACCESS_QUIET_TEXT);
        }
    }
    else if (crosstalk_eda_io_serial_attached_is(&ctx->eda.serial) == LDG_TRUTH_TRUE && crosstalk_eda_io_serial_busted_is(&ctx->eda.serial) != LDG_TRUTH_TRUE && ctx->time >= ctx->serial_access_grant_next)
    {
        crosstalk_serial_access_ctl_send(ctx, (uint32_t)CROSSTALK_NET_WIRE_SERIAL_CTL_GRANT);
        ctx->serial_access_grant_next = ctx->time + CROSSTALK_CORE_SERIAL_ACCESS_GRANT_SECS;
    }
}

static uint32_t crosstalk_receptacle_keys_drain(crosstalk_ctx_t *ctx, uint8_t *out, uint32_t cap, uint32_t *out_cunt)
{
    uint32_t cunt = 0;
    uint32_t at = 0;
    uint32_t text_at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0 || out_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    for (at = 0; at < ctx->recept_irq_cunt && cunt < cap; at = at + 0x1)
    {
        const ldg_term_key_event_t *irq = &ctx->recept_irq[at];

        if (irq->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE) { continue; }

        for (text_at = 0; text_at < irq->text_cunt && text_at < (uint32_t)LDG_TERM_KEY_TEXT_MAX && cunt < cap; text_at = text_at + 0x1)
        {
            if (irq->text[text_at] < (uint32_t)LDG_ASCII_PRINTABLE_MIN || irq->text[text_at] > (uint32_t)LDG_ASCII_PRINTABLE_MAX) { continue; }

            out[cunt] = (uint8_t)irq->text[text_at];
            cunt = cunt + 0x1;
        }

        if (irq->text_cunt != 0 || cunt >= cap) { continue; }

        if (irq->key == (uint8_t)LDG_TERM_KEY_ENTER) { out[cunt] = (uint8_t)LDG_ASCII_CR; }
        else if (irq->key == (uint8_t)LDG_TERM_KEY_BACKSPACE) { out[cunt] = (uint8_t)CROSSTALK_EDA_UI_TERM_BS; }
        else if (irq->key == (uint8_t)LDG_TERM_KEY_TAB) { out[cunt] = (uint8_t)LDG_ASCII_TAB; }
        else if (irq->key == (uint8_t)LDG_TERM_KEY_ESCAPE) { out[cunt] = (uint8_t)LDG_ASCII_ESC; }
        else { continue; }

        cunt = cunt + 0x1;
    }

    *out_cunt = cunt;

    return LDG_ERR_AOK;
}

static uint8_t crosstalk_emu_gfx_is(const crosstalk_ctx_t *ctx)
{
    uint64_t *fb = 0x0;
    uint32_t fb_cunt = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (crosstalk_eda_ic_fpga_prof_get(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, (uint32_t)CROSSTALK_HDL_ABI_STATE_EMU_FB, &fb, &fb_cunt) != LDG_ERR_AOK) { return ctx->eda.crt.src == (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_SRC_GFX ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    if (fb == 0x0 || fb_cunt < (uint32_t)CROSSTALK_HDL_ABI_FB_CUNT) { return LDG_TRUTH_FALSE; }

    return fb[CROSSTALK_HDL_ABI_FB_FMT_AT] == (uint64_t)CROSSTALK_HDL_ABI_FB_FMT_XRGB8888 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t crosstalk_receptacle_scan_of_key(uint32_t key, uint8_t *out)
{
    uint32_t special = key & ~(uint32_t)CROSSTALK_CORE_RECEPT_ID_SPECIAL;

    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (key == ((uint32_t)CROSSTALK_CORE_RECEPT_ID_MOD | (uint32_t)LDG_TERM_MOD_CTRL)) { *out = (uint8_t)CROSSTALK_CORE_RECEPT_SCAN_LCTRL; return LDG_ERR_AOK; }

    if (key == ((uint32_t)CROSSTALK_CORE_RECEPT_ID_MOD | (uint32_t)LDG_TERM_MOD_SHIFT)) { *out = (uint8_t)CROSSTALK_CORE_RECEPT_SCAN_LSHIFT; return LDG_ERR_AOK; }

    if (key == ((uint32_t)CROSSTALK_CORE_RECEPT_ID_MOD | (uint32_t)LDG_TERM_MOD_ALT)) { *out = (uint8_t)CROSSTALK_CORE_RECEPT_SCAN_LALT; return LDG_ERR_AOK; }

    if ((key & (uint32_t)CROSSTALK_CORE_RECEPT_ID_SPECIAL) != 0)
    {
        if (special == (uint32_t)LDG_TERM_KEY_UP) { *out = (uint8_t)CROSSTALK_CORE_RECEPT_SCAN_UP; return LDG_ERR_AOK; }

        if (special == (uint32_t)LDG_TERM_KEY_DOWN) { *out = (uint8_t)CROSSTALK_CORE_RECEPT_SCAN_DOWN; return LDG_ERR_AOK; }

        if (special == (uint32_t)LDG_TERM_KEY_LHS) { *out = (uint8_t)CROSSTALK_CORE_RECEPT_SCAN_LHS; return LDG_ERR_AOK; }

        if (special == (uint32_t)LDG_TERM_KEY_RHS) { *out = (uint8_t)CROSSTALK_CORE_RECEPT_SCAN_RHS; return LDG_ERR_AOK; }

        if (special == (uint32_t)LDG_TERM_KEY_TAB) { *out = (uint8_t)CROSSTALK_CORE_RECEPT_SCAN_TAB; return LDG_ERR_AOK; }

        if (special == (uint32_t)LDG_TERM_KEY_ESCAPE) { *out = (uint8_t)CROSSTALK_CORE_RECEPT_SCAN_ESC; return LDG_ERR_AOK; }

        if (special == (uint32_t)LDG_TERM_KEY_ENTER) { return crosstalk_eda_io_scancode_get((uint8_t)LDG_ASCII_CR, out); }

        if (special == (uint32_t)LDG_TERM_KEY_BACKSPACE) { return crosstalk_eda_io_scancode_get((uint8_t)CROSSTALK_EDA_UI_TERM_BS, out); }

        return LDG_ERR_NOT_FOUND;
    }

    if (key > (uint32_t)CROSSTALK_CORE_RECEPT_ASCII_MAX) { return LDG_ERR_NOT_FOUND; }

    return crosstalk_eda_io_scancode_get((uint8_t)key, out);
}

static uint32_t crosstalk_receptacle_id_get(const ldg_term_key_event_t *irq)
{
    uint32_t codepoint = 0;

    if (LDG_UNLIKELY(irq == 0x0)) { return (uint32_t)UINT32_MAX; }

    if (irq->key != (uint8_t)LDG_TERM_KEY_NONE) { return (uint32_t)CROSSTALK_CORE_RECEPT_ID_SPECIAL | (uint32_t)irq->key; }

    codepoint = irq->codepoint;

    if (codepoint >= (uint32_t)CROSSTALK_CORE_RECEPT_LOWER_FIRST && codepoint <= (uint32_t)CROSSTALK_CORE_RECEPT_LOWER_LAST) { codepoint = codepoint - (uint32_t)CROSSTALK_CORE_RECEPT_CASE_BIT; }

    if (codepoint == 0 || codepoint > (uint32_t)CROSSTALK_CORE_RECEPT_ASCII_MAX) { return (uint32_t)UINT32_MAX; }

    return codepoint;
}

static void crosstalk_receptacle_key_release(crosstalk_ctx_t *ctx, uint32_t at)
{
    if (LDG_UNLIKELY(ctx == 0x0 || at >= ctx->recept_key_cunt)) { return; }

    if (crosstalk_eda_io_receptacle_scan_edge_put(&ctx->eda.receptacle, ctx->recept_code[at], LDG_TRUTH_FALSE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "receptacle key release push failed"); }

    ctx->recept_key_cunt = ctx->recept_key_cunt - 0x1;
    ctx->recept_key[at] = ctx->recept_key[ctx->recept_key_cunt];
    ctx->recept_code[at] = ctx->recept_code[ctx->recept_key_cunt];
}

static uint32_t crosstalk_receptacle_held_find(const crosstalk_ctx_t *ctx, uint32_t key)
{
    uint32_t at = 0;

    for (at = 0; at < ctx->recept_key_cunt; at = at + 0x1) { if (ctx->recept_key[at] == key) { return at; } }

    return (uint32_t)UINT32_MAX;
}

static void crosstalk_receptacle_key_press(crosstalk_ctx_t *ctx, uint32_t key)
{
    uint8_t code = 0;
    uint8_t typed = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (crosstalk_receptacle_held_find(ctx, key) != (uint32_t)UINT32_MAX) { return; }

    if (ctx->recept_key_cunt >= (uint32_t)CROSSTALK_CORE_RECEPT_KEY_CAP) { return; }

    if (crosstalk_receptacle_scan_of_key(key, &code) != LDG_ERR_AOK) { return; }

    if (crosstalk_eda_io_receptacle_scan_edge_put(&ctx->eda.receptacle, code, LDG_TRUTH_TRUE) != LDG_ERR_AOK) { return; }

    if (key <= (uint32_t)CROSSTALK_CORE_RECEPT_ASCII_MAX) { typed = (uint8_t)key; }
    else if (key == ((uint32_t)CROSSTALK_CORE_RECEPT_ID_SPECIAL | (uint32_t)LDG_TERM_KEY_BACKSPACE)) { typed = (uint8_t)CROSSTALK_EDA_IC_FPGA_EMU_STAT_BS; }

    if (typed != 0 && crosstalk_eda_ic_fpga_emu_stat_typed_put(&ctx->emu_stat, &typed, (uint32_t)sizeof(typed), ldg_time_epoch_ms_get()) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu stat typed put failed"); }

    ctx->recept_key[ctx->recept_key_cunt] = key;
    ctx->recept_code[ctx->recept_key_cunt] = code;
    ctx->recept_key_cunt = ctx->recept_key_cunt + 0x1;
}

static void crosstalk_receptacle_mods_drive(crosstalk_ctx_t *ctx, uint32_t mods)
{
    const uint32_t mod_bit[CROSSTALK_CORE_RECEPT_MOD_CUNT] = { (uint32_t)LDG_TERM_MOD_CTRL, (uint32_t)LDG_TERM_MOD_SHIFT, (uint32_t)LDG_TERM_MOD_ALT };
    uint32_t at = 0;

    for (at = 0; at < (uint32_t)CROSSTALK_CORE_RECEPT_MOD_CUNT; at = at + 0x1)
    {
        uint32_t key = (uint32_t)CROSSTALK_CORE_RECEPT_ID_MOD | mod_bit[at];
        uint32_t held = crosstalk_receptacle_held_find(ctx, key);

        if ((mods & mod_bit[at]) != 0) { crosstalk_receptacle_key_press(ctx, key); }
        else if (held != (uint32_t)UINT32_MAX) { crosstalk_receptacle_key_release(ctx, held); }
    }
}

static void crosstalk_receptacle_keys_release_drive(crosstalk_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (ctx->irq_src != 0x0 && ldg_term_input_release_report_is(ctx->irq_src) == LDG_TRUTH_TRUE) { return; }

    while (ctx->recept_key_cunt != 0) { crosstalk_receptacle_key_release(ctx, ctx->recept_key_cunt - 0x1); }
}

static void crosstalk_receptacle_keys_edge_drive(crosstalk_ctx_t *ctx)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    for (at = 0; at < ctx->recept_irq_cunt; at = at + 0x1)
    {
        const ldg_term_key_event_t *irq = &ctx->recept_irq[at];
        uint32_t key = crosstalk_receptacle_id_get(irq);
        uint32_t held = (uint32_t)UINT32_MAX;

        crosstalk_receptacle_mods_drive(ctx, irq->mods);

        if (key == (uint32_t)UINT32_MAX) { continue; }

        if (irq->kind != (uint8_t)LDG_TERM_KEY_KIND_RELEASE)
        {
            crosstalk_receptacle_key_press(ctx, key);
            continue;
        }

        held = crosstalk_receptacle_held_find(ctx, key);

        if (held != (uint32_t)UINT32_MAX) { crosstalk_receptacle_key_release(ctx, held); }
    }
}

static uint32_t crosstalk_receptacle_key_isr(crosstalk_ctx_t *ctx, const ldg_term_key_event_t *irq, uint8_t *consumed)
{
    if (LDG_UNLIKELY(ctx == 0x0 || irq == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    if (crosstalk_eda_io_receptacle_live_is(&ctx->eda.receptacle) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (ctx->eda.crt.open != LDG_TRUTH_TRUE || crosstalk_ui_window_focused_is(&ctx->win_nav, (uint64_t)CROSSTALK_EDA_UI_DISPLAY_CRT_WINDOW_ID) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    *consumed = LDG_TRUTH_TRUE;

    if (ctx->recept_irq_cunt >= (uint32_t)CROSSTALK_CORE_RECEPT_IRQ_CAP) { return LDG_ERR_FULL; }

    ctx->recept_irq[ctx->recept_irq_cunt] = *irq;
    ctx->recept_irq_cunt = ctx->recept_irq_cunt + 0x1;

    return LDG_ERR_AOK;
}

static void crosstalk_receptacle_feed(crosstalk_ctx_t *ctx)
{
    uint8_t keys[CROSSTALK_EDA_UI_TERM_KEY_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t key_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (crosstalk_eda_io_receptacle_live_is(&ctx->eda.receptacle) != LDG_TRUTH_TRUE) { return; }

    crosstalk_receptacle_keys_release_drive(ctx);

    if (crosstalk_eda_ic_fpga_build_live_is(&ctx->eda.fpga_build) == LDG_TRUTH_TRUE) { return; }

    if (crosstalk_eda_io_serial_attached_is(&ctx->eda.serial) == LDG_TRUTH_TRUE && crosstalk_eda_ui_term_serial_live_is(&ctx->eda.term) == LDG_TRUTH_TRUE) { return; }

    if (ctx->eda.term.sesh[ctx->eda.term.sesh_active].src == (uint32_t)CROSSTALK_EDA_UI_TERM_SRC_PTY && crosstalk_eda_ui_term_focus_is(&ctx->eda.term) == LDG_TRUTH_TRUE) { return; }

    if (ctx->eda.crt.open == LDG_TRUTH_TRUE && crosstalk_ui_window_focused_is(&ctx->win_nav, (uint64_t)CROSSTALK_EDA_UI_DISPLAY_CRT_WINDOW_ID) == LDG_TRUTH_TRUE)
    {
        if (ctx->eda.receptacle.mode == (uint32_t)CROSSTALK_EDA_IO_RECEPTACLE_MODE_SCAN)
        {
            crosstalk_receptacle_keys_edge_drive(ctx);

            return;
        }

        if (crosstalk_receptacle_keys_drain(ctx, keys, (uint32_t)CROSSTALK_EDA_UI_TERM_KEY_CAP, &key_cunt) != LDG_ERR_AOK) { return; }
    }
    else if (crosstalk_eda_ui_term_open_is(&ctx->eda.term) == LDG_TRUTH_TRUE && crosstalk_eda_ui_term_focus_is(&ctx->eda.term) == LDG_TRUTH_TRUE) { if (crosstalk_eda_ui_term_keys_drain(&ctx->eda.term, keys, (uint32_t)CROSSTALK_EDA_UI_TERM_KEY_CAP, &key_cunt) != LDG_ERR_AOK) { return; } }
    else { return; }

    if (key_cunt == 0) { return; }

    ret = crosstalk_eda_io_receptacle_put(&ctx->eda.receptacle, keys, key_cunt);
    if (ret != LDG_ERR_AOK && ret != CROSSTALK_EDA_IO_RECEPTACLE_ERR_FULL) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "receptacle put failed"); }

    if (ret == LDG_ERR_AOK)
    {
        if (crosstalk_eda_ic_fpga_emu_stat_typed_put(&ctx->emu_stat, keys, key_cunt, ldg_time_epoch_ms_get()) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu stat typed put failed"); }

        crosstalk_ach_once(ctx, CROSSTALK_GAME_PROGRESSION_FIRST_RECEPTACLE);
    }
}

static void crosstalk_receptacle_tick(crosstalk_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    crosstalk_receptacle_feed(ctx);

    ctx->recept_irq_cunt = 0;
}

static uint32_t crosstalk_sv_top_get(const char *path, char *out, uint32_t cap)
{
    ldg_io_file_t *file = 0x0;
    char src_hd[CROSSTALK_CORE_SV_HD_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t got = 0;
    uint32_t at = 0;
    uint32_t used = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(path == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, CROSSTALK_CORE_SV_FILE_MODE, &file);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_io_file_rd(file, (ldg_byte_t *)src_hd, (uint64_t)(sizeof(src_hd) - LDG_STR_TERM_SIZE), &got);
    if (ldg_io_file_close(file) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sv top close failed"); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_EOF)) { return ret; }

    src_hd[got] = LDG_STR_TERM;

    for (at = 0; src_hd[at] != LDG_STR_TERM; at = at + 0x1)
    {
        if (strncmp(&src_hd[at], "module", strlen("module")) != 0x0) { continue; }

        if (at != 0 && (ldg_char_alnum_is((uint8_t)src_hd[at - 0x1]) == LDG_TRUTH_TRUE || src_hd[at - 0x1] == '_')) { continue; }

        at = at + (uint32_t)strlen("module");
        if (ldg_char_space_is((uint8_t)src_hd[at]) != LDG_TRUTH_TRUE) { continue; }

        while (ldg_char_space_is((uint8_t)src_hd[at]) == LDG_TRUTH_TRUE) { at = at + 0x1; }

        while (used + LDG_STR_TERM_SIZE < cap && (ldg_char_alnum_is((uint8_t)src_hd[at]) == LDG_TRUTH_TRUE || src_hd[at] == '_' || src_hd[at] == '$'))
        {
            out[used] = src_hd[at];
            used = used + 0x1;
            at = at + 0x1;
        }

        out[used] = LDG_STR_TERM;
        return used > 0 ? LDG_ERR_AOK : LDG_ERR_NOT_FOUND;
    }

    return LDG_ERR_NOT_FOUND;
}

static void crosstalk_editor_file_watch(crosstalk_ctx_t *ctx)
{
    char clip_text[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char ext[CROSSTALK_CORE_SV_EXT_CAP] = LDG_ARR_ZERO_INIT;
    char toast_text[CROSSTALK_CORE_EDITOR_TOAST_CAP] = LDG_ARR_ZERO_INIT;
    char base[CROSSTALK_CORE_LD_DIR_CAP] = LDG_ARR_ZERO_INIT;
    const char *ext_stem = 0x0;
    uint32_t clip_len = 0;
    uint32_t idx = 0;

    if (crosstalk_eda_io_pty_clip_take(&ctx->eda.pty, clip_text, (uint32_t)sizeof(clip_text), &clip_len) != LDG_ERR_AOK || clip_len == 0) { return; }

    for (idx = 0; idx < clip_len; idx = idx + 0x1) { if (clip_text[idx] == '\n' || clip_text[idx] == '\r') { return; } }

    if (ldg_io_path_ext_get(clip_text, ext, (uint64_t)sizeof(ext)) != LDG_ERR_AOK) { return; }

    ext_stem = ext[0] == '.' ? ext + 0x1 : ext;
    if (strcmp(ext_stem, "sv") != 0x0) { return; }

    if (ldg_io_path_exists_is(clip_text) != LDG_TRUTH_TRUE)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "crosstalk: editor file not reachable; path: %.259s", clip_text);
        return;
    }

    if (ldg_str_rbrcpy(ctx->editor_file, clip_text, (uint64_t)sizeof(ctx->editor_file)) != LDG_ERR_AOK) { return; }

    if (ldg_io_path_leaf_get(clip_text, base, (uint64_t)sizeof(base)) != LDG_ERR_AOK) { base[0] = LDG_STR_TERM; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: editor file detected; path: %.259s", ctx->editor_file);
    snprintf(toast_text, sizeof(toast_text), "editor file: %.47s", base);
    toast_push(ctx, toast_text);
}

static void crosstalk_pty_tick(crosstalk_ctx_t *ctx)
{
    uint8_t keys[CROSSTALK_EDA_UI_TERM_KEY_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t key_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    ret = crosstalk_eda_io_pty_tick(&ctx->eda.pty);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty tick failed"); }

    if (crosstalk_eda_io_pty_live_is(&ctx->eda.pty) != LDG_TRUTH_TRUE) { return; }

    if (ctx->eda.term.pty_cols_want > 0 && ctx->eda.term.pty_rows_want > 0)
    {
        if (ctx->eda.term.pty_cols_want == ctx->pty_want_cols_prev && ctx->eda.term.pty_rows_want == ctx->pty_want_rows_prev) { if (ctx->pty_want_stable < (uint32_t)CROSSTALK_CORE_PTY_SETTLE_FRAMES) { ctx->pty_want_stable = ctx->pty_want_stable + 0x1; } }
        else
        {
            ctx->pty_want_cols_prev = ctx->eda.term.pty_cols_want;
            ctx->pty_want_rows_prev = ctx->eda.term.pty_rows_want;
            ctx->pty_want_stable = 0;
        }

        if (ctx->pty_want_stable >= (uint32_t)CROSSTALK_CORE_PTY_SETTLE_FRAMES)
        {
            ret = crosstalk_eda_io_pty_grid_resize(&ctx->eda.pty, ctx->eda.term.pty_cols_want, ctx->eda.term.pty_rows_want);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_CLOSED)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty grid resize failed"); }

            if (ret == LDG_ERR_AOK)
            {
                uint64_t notify_len = 0;

                if (ldg_term_resize_encode(&ctx->eda.pty.vt, keys, (uint64_t)CROSSTALK_EDA_UI_TERM_KEY_CAP, &notify_len) == LDG_ERR_AOK && notify_len != 0)
                {
                    ret = crosstalk_eda_io_pty_keys_wr(&ctx->eda.pty, keys, (uint32_t)notify_len);
                    if (ret != LDG_ERR_AOK && ret != LDG_ERR_CLOSED) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty resize notify failed"); }
                }
            }
        }
    }

    crosstalk_editor_file_watch(ctx);

    if (ctx->eda.term.sesh[ctx->eda.term.sesh_active].src != (uint32_t)CROSSTALK_EDA_UI_TERM_SRC_PTY) { return; }

    if (crosstalk_eda_ui_term_keys_drain(&ctx->eda.term, keys, (uint32_t)CROSSTALK_EDA_UI_TERM_KEY_CAP, &key_cunt) != LDG_ERR_AOK || key_cunt == 0) { return; }

    ret = crosstalk_eda_io_pty_keys_wr(&ctx->eda.pty, keys, key_cunt);
    if (ret != LDG_ERR_AOK && ret != LDG_ERR_CLOSED) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty keys failed"); }
}

static void crosstalk_sound_attach(crosstalk_ctx_t *ctx, const char *arg)
{
    const crosstalk_hdl_exec_ctx_t *exec = 0x0;
    const crosstalk_hdl_abi_t *abi = 0x0;
    char port[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    char tok[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t period = 0;
    uint64_t freq = 0;
    uint32_t period_set = 0;
    uint32_t rate = 0;
    uint32_t probe_handle = UINT32_MAX;
    uint32_t stream_handle = UINT32_MAX;
    uint32_t width = 0;
    uint32_t at = 0;
    uint32_t tok_len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (at < (uint32_t)CROSSTALK_CORE_SOUND_ARG_CAP - LDG_STR_TERM_SIZE && arg[at] != LDG_STR_TERM && arg[at] != ' ')
    {
        port[at] = arg[at];
        at = at + 0x1;
    }

    port[at] = LDG_STR_TERM;

    if (arg[at] == ' ')
    {
        at = at + LDG_STR_TERM_SIZE;

        while (tok_len < (uint32_t)CROSSTALK_CORE_SOUND_ARG_CAP - LDG_STR_TERM_SIZE && arg[at] != LDG_STR_TERM && arg[at] != ' ')
        {
            tok[tok_len] = arg[at];
            tok_len = tok_len + 0x1;
            at = at + 0x1;
        }

        tok[tok_len] = LDG_STR_TERM;

        if (ldg_str_to_dec(tok, &rate) == LDG_ERR_AOK && rate != 0)
        {
            ret = crosstalk_eda_sound_rate_set(&ctx->eda.sound, rate);
            if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_SOUND_RATE_TEXT); return; }
        }
    }

    if (arg[at] == ' ')
    {
        at = at + LDG_STR_TERM_SIZE;
        tok_len = 0;

        while (tok_len < (uint32_t)CROSSTALK_CORE_SOUND_ARG_CAP - LDG_STR_TERM_SIZE && arg[at] != LDG_STR_TERM && arg[at] != ' ')
        {
            tok[tok_len] = arg[at];
            tok_len = tok_len + 0x1;
            at = at + 0x1;
        }

        tok[tok_len] = LDG_STR_TERM;

        if (ldg_str_to_dec(tok, &period_set) != LDG_ERR_AOK) { period_set = 0; }
    }

    ret = crosstalk_probe_src_get(ctx, &exec, &abi);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_PIPE_NO_DESIGN_TEXT); return; }

    if (ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT].comp.f_op_hz == 0) { toast_push(ctx, CROSSTALK_CORE_SOUND_NO_CLK_TEXT); return; }

    if (ctx->sound_stream_handle != UINT32_MAX)
    {
        if (crosstalk_eda_logic_stream_close(&ctx->eda.logic_stream, ctx->sound_stream_handle) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sound stream close failed"); }

        ctx->sound_stream_handle = UINT32_MAX;
    }

    if (ctx->sound_probe_handle != UINT32_MAX)
    {
        if (crosstalk_eda_logic_probe_detach(&ctx->eda.logic_probe, ctx->sound_probe_handle) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sound probe detach failed"); }

        ctx->sound_probe_handle = UINT32_MAX;
    }

    period = ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT].comp.f_op_hz;

    if (crosstalk_eda_clk_domain_freq_get(&ctx->eda.clk, ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT].clk_domain, &freq) == LDG_ERR_AOK && freq != 0) { period = freq; }

    period = period / (uint64_t)ctx->eda.sound.rate;

    if (period_set != 0) { period = period_set; }

    if (period == 0) { period = (uint64_t)CROSSTALK_EDA_SOUND_STEP; }

    ret = crosstalk_eda_logic_probe_period_attach(&ctx->eda.logic_probe, CROSSTALK_CORE_TERM_SLOT, exec, abi, port, period, &probe_handle);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sound: attach failed; port: %.63s; err: %u32", port, ret);
        toast_push(ctx, CROSSTALK_CORE_PIPE_NO_SIG_TEXT);
        return;
    }

    ret = crosstalk_eda_logic_stream_open(&ctx->eda.logic_stream, probe_handle, CROSSTALK_EDA_LOGIC_STREAM_DECODE_SAMPLE, &stream_handle);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_PIPE_NO_STREAM_TEXT); return; }

    width = ctx->eda.logic_probe.probe[probe_handle].width;

    ret = crosstalk_eda_logic_stream_width_set(&ctx->eda.logic_stream, stream_handle, width);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sound width set failed"); }

    ret = crosstalk_eda_sound_bind(&ctx->eda.sound, port);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sound bind failed"); }

    ret = crosstalk_eda_logic_stream_sink_add(&ctx->eda.logic_stream, stream_handle, crosstalk_eda_sound_sink_cb, &ctx->eda.sound);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_PIPE_NO_SINK_TEXT); return; }

    ret = crosstalk_eda_sound_stream_start(&ctx->eda.sound);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_SOUND_PASS_FAIL_TEXT); }

    ctx->sound_probe_handle = probe_handle;
    ctx->sound_stream_handle = stream_handle;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "sound: attached; port: %.63s; width: %u32; rate: %u32; f_op: %u64 Hz; period: %u64 cycles; probe: %u32; stream: %u32", port, width, ctx->eda.sound.rate, ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT].comp.f_op_hz, period, probe_handle, stream_handle);
    toast_push(ctx, CROSSTALK_CORE_SOUND_ATTACH_TEXT);
}

static uint8_t crosstalk_core_hlt_is(const crosstalk_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (crosstalk_eda_ic_fpga_slot_live_is(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT) != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    return crosstalk_eda_ic_fpga_slot_hlt_is(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT);
}

static uint32_t crosstalk_core_hlt_set(crosstalk_ctx_t *ctx, uint8_t on)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_eda_ic_fpga_slot_hlt_set(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, on);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (on == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (crosstalk_eda_ic_fpga_run_live_is(&ctx->eda.fpga_run) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (crosstalk_eda_ic_fpga_run_start(&ctx->eda.fpga_run) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "core resume; run start failed"); }

    return LDG_ERR_AOK;
}

static uint8_t crosstalk_fpga_hlt(crosstalk_ctx_t *ctx)
{
    uint8_t was_live = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_TRUTH_FALSE; }

    was_live = crosstalk_eda_ic_fpga_run_live_is(&ctx->eda.fpga_run);
    if (was_live != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    if (crosstalk_eda_ic_fpga_run_hlt(&ctx->eda.fpga_run) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga run halt failed"); }

    return LDG_TRUTH_TRUE;
}

static void crosstalk_fpga_in_ports_carry(crosstalk_ctx_t *ctx)
{
    crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    char name[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t val = 0;
    uint32_t carried = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    slot = &ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT];

    if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE || slot->elab_src == 0x0) { return; }

    for (idx = 0; idx < slot->ir.sig_cunt; idx++)
    {
        const crosstalk_hdl_ir_sig_t *sig = &slot->ir.sigs[idx];
        uint32_t abi_sig = 0;
        uint32_t name_len = 0;

        if (sig->port != (uint32_t)CROSSTALK_HDL_IR_PORT_IN) { continue; }

        if (idx == slot->clk_sig) { continue; }

        name_len = sig->name.len < (uint32_t)CROSSTALK_CORE_SOUND_ARG_CAP - LDG_STR_TERM_SIZE ? sig->name.len : (uint32_t)CROSSTALK_CORE_SOUND_ARG_CAP - LDG_STR_TERM_SIZE;

        memcpy(name, slot->elab_src + sig->name.off, name_len);

        name[name_len] = LDG_STR_TERM;

        abi_sig = slot->abi->sig_find(name);
        if (abi_sig == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { continue; }

        if (crosstalk_hdl_exec_peek(&slot->exec, idx, &val) != LDG_ERR_AOK) { continue; }

        slot->abi->poke(abi_sig, val);
        carried = carried + 0x1;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: input ports carried to the compiled component; cunt: %u32", carried);
}

static void crosstalk_fpga_resume(crosstalk_ctx_t *ctx, uint8_t was_live)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (was_live != LDG_TRUTH_TRUE) { return; }

    if (crosstalk_eda_ic_fpga_run_start(&ctx->eda.fpga_run) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga run restart failed"); }
}

static uint32_t crosstalk_snap_ready_get(crosstalk_ctx_t *ctx, const char *arg, char *out, uint64_t cap)
{
    const crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    char named[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    slot = &ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT];

    if (slot->runnable != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (slot->abi == 0x0 || slot->abi->ok != LDG_TRUTH_TRUE) { return CROSSTALK_HDL_ERR_ABI_SYM; }

    if (arg != 0x0 && arg[0] != LDG_STR_TERM)
    {
        if (strchr(arg, '\\') != 0x0 || strchr(arg, '/') != 0x0) { return ldg_str_rbrcpy(out, arg, cap); }

        if (snprintf(named, sizeof(named), CROSSTALK_CORE_SNAP_NAMED_FMT, arg) < 0) { return LDG_ERR_STR_FMT; }

        return crosstalk_core_runtime_path_resolve(named, out, cap);
    }

    return crosstalk_core_runtime_path_resolve(CROSSTALK_CORE_SNAP_FILE, out, cap);
}

static void crosstalk_snap_list(crosstalk_ctx_t *ctx)
{
    ldg_io_dir_t *dir = 0x0;
    ldg_io_entry_t ent = LDG_STRUCT_ZERO_INIT;
    char probe[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char dir_path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char note[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    ret = crosstalk_core_runtime_path_resolve(CROSSTALK_CORE_SNAP_FILE, probe, (uint64_t)sizeof(probe));
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_SNAP_LIST_FAIL_TEXT); return; }

    ret = ldg_io_path_parent_get(probe, dir_path, (uint64_t)sizeof(dir_path));
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_SNAP_LIST_FAIL_TEXT); return; }

    ret = ldg_io_dir_open(dir_path, &dir);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_SNAP_LIST_FAIL_TEXT); return; }

    while (ldg_io_dir_rd(dir, &ent) == LDG_ERR_AOK)
    {
        const char *name = 0x0;
        uint64_t name_len = 0;
        uint64_t ext_len = strlen(CROSSTALK_CORE_SNAP_LIST_EXT);

        if (ldg_io_entry_dir_is(&ent) == LDG_TRUTH_TRUE) { continue; }

        if (ldg_io_entry_name_get(&ent, &name) != LDG_ERR_AOK || name == 0x0) { continue; }

        name_len = strlen(name);

        if (name_len <= ext_len) { continue; }

        if (strcmp(&name[name_len - ext_len], CROSSTALK_CORE_SNAP_LIST_EXT) != 0) { continue; }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "snap: found; name: %.255s", name);
        cunt = cunt + 0x1;
    }

    if (ldg_io_dir_close(dir) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "snap dir close failed"); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "snap: list done; dir: %.255s; cunt: %u32", dir_path, cunt);

    if (snprintf(note, sizeof(note), CROSSTALK_CORE_SNAP_LIST_DONE_FMT, cunt) > 0) { toast_push(ctx, note); }
}

static void crosstalk_snap_wr(crosstalk_ctx_t *ctx, const char *arg)
{
    crosstalk_hdl_bake_ident_t ident = LDG_STRUCT_ZERO_INIT;
    const crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    char path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint8_t was_live = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (arg != 0x0 && strcmp(arg, CROSSTALK_CORE_SNAP_LIST_TOK) == 0) { crosstalk_snap_list(ctx); return; }

    ret = crosstalk_snap_ready_get(ctx, arg, path, (uint64_t)sizeof(path));
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_SNAP_NO_ABI_TEXT); return; }

    if ((arg == 0x0 || arg[0] == LDG_STR_TERM) && ldg_io_path_exists_is(path) == LDG_TRUTH_TRUE)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "snap: refused; unnamed snap exists; path: %.255s", path);
        toast_push(ctx, CROSSTALK_CORE_SNAP_EXISTS_TEXT);
        return;
    }

    slot = &ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT];

    ret = crosstalk_hdl_bake_ident_build(slot->elab_src, slot->elab_src_len, slot->top, 0x0, &ident);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_SNAP_FAIL_TEXT); return; }

    was_live = crosstalk_fpga_hlt(ctx);

    ret = crosstalk_hdl_bake_wr(slot->abi, &ident, path);

    crosstalk_fpga_resume(ctx, was_live);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "snap: write; top: %.63s; path: %.255s; err: %u32", slot->top, path, ret);

    toast_push(ctx, ret == LDG_ERR_AOK ? CROSSTALK_CORE_SNAP_OK_TEXT : CROSSTALK_CORE_SNAP_FAIL_TEXT);
}

static void crosstalk_snap_rd(crosstalk_ctx_t *ctx, const char *arg)
{
    crosstalk_hdl_bake_ident_t ident = LDG_STRUCT_ZERO_INIT;
    const crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    char path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint8_t was_live = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    ret = crosstalk_snap_ready_get(ctx, arg, path, (uint64_t)sizeof(path));
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_SNAP_NO_ABI_TEXT); return; }

    slot = &ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT];

    ret = crosstalk_hdl_bake_ident_build(slot->elab_src, slot->elab_src_len, slot->top, 0x0, &ident);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RESUME_FAIL_TEXT); return; }

    was_live = crosstalk_fpga_hlt(ctx);

    ret = crosstalk_hdl_bake_rd(slot->abi, &ident, path);

    crosstalk_fpga_resume(ctx, was_live);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "snap: read; top: %.63s; path: %.255s; err: %u32", slot->top, path, ret);

    toast_push(ctx, ret == LDG_ERR_AOK ? CROSSTALK_CORE_RESUME_OK_TEXT : CROSSTALK_CORE_RESUME_FAIL_TEXT);
}

static uint32_t crosstalk_tail_split(const char *arg, char *out_lhs, uint32_t lhs_cap, char *out_rhs, uint32_t rhs_cap)
{
    uint32_t len = 0;
    uint32_t cut = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(arg == 0x0 || out_lhs == 0x0 || out_rhs == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    out_lhs[0] = LDG_STR_TERM;
    out_rhs[0] = LDG_STR_TERM;

    while (arg[len] != LDG_STR_TERM) { len = len + 0x1; }

    while (len != 0 && arg[len - LDG_STR_TERM_SIZE] == ' ') { len = len - LDG_STR_TERM_SIZE; }

    if (len == 0) { return LDG_ERR_EMPTY; }

    cut = len;
    while (cut != 0 && arg[cut - LDG_STR_TERM_SIZE] != ' ') { cut = cut - LDG_STR_TERM_SIZE; }

    if (cut == 0) { return LDG_ERR_NOT_FOUND; }

    for (at = cut; at < len && at - cut + LDG_STR_TERM_SIZE < rhs_cap; at++) { out_rhs[at - cut] = arg[at]; }

    out_rhs[len - cut < rhs_cap - LDG_STR_TERM_SIZE ? len - cut : rhs_cap - LDG_STR_TERM_SIZE] = LDG_STR_TERM;

    while (cut != 0 && arg[cut - LDG_STR_TERM_SIZE] == ' ') { cut = cut - LDG_STR_TERM_SIZE; }

    for (at = 0; at < cut && at + LDG_STR_TERM_SIZE < lhs_cap; at++) { out_lhs[at] = arg[at]; }

    out_lhs[cut < lhs_cap - LDG_STR_TERM_SIZE ? cut : lhs_cap - LDG_STR_TERM_SIZE] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

static void crosstalk_bench_caps_fill(crosstalk_eda_ic_fpga_comp_t *caps)
{
    memset(caps, 0x0, sizeof(*caps));

    caps->logic_cells = UINT64_MAX;
    caps->user_io = UINT64_MAX;
    caps->ram_bits = UINT64_MAX;
    caps->blk_ram = UINT64_MAX;
    caps->global_clks = UINT64_MAX;
    caps->f_op_hz = CROSSTALK_CORE_BENCH_CLK_HZ;
}

static LDG_NOINLINE void crosstalk_fileset_apply(crosstalk_ctx_t *ctx, const char *dir, const char *files, const char *top)
{
    crosstalk_eda_ic_fpga_comp_t caps = LDG_STRUCT_ZERO_INIT;
    uint32_t bytes = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || dir == 0x0 || files == 0x0 || top == 0x0)) { return; }

    crosstalk_bench_caps_fill(&caps);

    if (crosstalk_fpga_hlt(ctx) == LDG_TRUTH_TRUE) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga reprogram resets the component"); }

    ret = crosstalk_eda_ic_fpga_fileset_program(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, dir, files, top, &caps, CROSSTALK_EDA_IC_FPGA_CLK_SIG, &bytes);

    {
        const crosstalk_eda_ic_fpga_slot_t *slot = &ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT];

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: fileset program; top: %.63s; bytes: %u32; err: %u32", top, bytes, ret);
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: fileset dir; dir: %.191s", dir);
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: fileset files; files: %.191s", files);
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: fileset fit; cells: %u64/%u64; mem: %u64/%u64; io: %u64/%u64", slot->verdict.cells_used, slot->verdict.cells_avail, slot->verdict.mem_bits, slot->verdict.mem_avail, slot->verdict.io_bits, slot->verdict.io_avail);
    }

    if (crosstalk_core_watch_set_arm(&ctx->design_watch, dir, files) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "design watch: arm failed; dir: %.191s", dir); }

    if (ret == LDG_ERR_AOK)
    {
        uint32_t domain_idx = UINT32_MAX;

        if (crosstalk_eda_clk_domain_add(&ctx->eda.clk, 0x0, caps.f_op_hz, &domain_idx) == LDG_ERR_AOK) { if (crosstalk_eda_ic_fpga_clk_domain_set(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, domain_idx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dir program clk bind failed"); } }

        if (crosstalk_eda_ic_fpga_run_start(&ctx->eda.fpga_run) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga run start failed"); }

        toast_push(ctx, CROSSTALK_CORE_LD_OK_TEXT);
        return;
    }

    if (ret == CROSSTALK_EDA_IC_FPGA_ERR_OVER_CAP) { toast_push(ctx, "design exceeds component capacity"); return; }

    {
        const crosstalk_eda_ic_fpga_slot_t *diag_slot = &ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT];
        char diag_toast[CROSSTALK_CORE_LD_DIAG_TOAST_CAP] = LDG_ARR_ZERO_INIT;

        if (diag_slot->diag_text[0] != LDG_STR_TERM)
        {
            if (crosstalk_core_cmdline_out_push(&ctx->cmdline, diag_slot->diag_text) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "ld: diag drawer push failed"); }

            if (diag_slot->diag.entry != 0x0 && snprintf(diag_toast, sizeof(diag_toast), CROSSTALK_CORE_LD_DIAG_TOAST_FMT, diag_slot->diag.line, diag_slot->diag.entry->msg) > 0)
            {
                toast_push(ctx, diag_toast);
                return;
            }
        }
    }

    toast_push(ctx, CROSSTALK_CORE_LD_FAIL_TEXT);
}

#define CROSSTALK_CORE_FETCH_START_TEXT "fetching the repository with git"
#define CROSSTALK_CORE_FETCH_BUSY_TEXT "a fetch is already running"
#define CROSSTALK_CORE_FETCH_DIR_TEXT "with a URL, ld takes a plain directory name under the game data dir"
#define CROSSTALK_CORE_FETCH_GIT_TEXT "git not found on PATH; a fetch needs git installed"
#define CROSSTALK_CORE_FETCH_URL_TEXT "that URL does not name a repository"
#define CROSSTALK_CORE_FETCH_FAIL_TEXT "fetch failed; see the log"
#define CROSSTALK_CORE_FETCH_GIT_FAIL_TEXT "git failed; see the cmdline"
#define CROSSTALK_CORE_FETCH_EMPTY_TEXT "no .sv or .v files in that repository"
#define CROSSTALK_CORE_FETCH_FULL_TEXT "that design needs more files than one fileset holds"
#define CROSSTALK_CORE_FETCH_TOP_TEXT "no module by that top name in that repository"

static void crosstalk_dir_fetch(crosstalk_ctx_t *ctx, const char *dir, const char *url, const char *top)
{
    char parent[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    if (crosstalk_core_fetch_name_valid_is(dir) != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_FETCH_DIR_TEXT); return; }

    ret = crosstalk_data_subdir_resolve(dir, parent, (uint64_t)sizeof(parent));
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fetch: data dir unavailable; dir: %.127s; err: %u32", dir, ret);
        toast_push(ctx, CROSSTALK_CORE_FETCH_FAIL_TEXT);
        return;
    }

    ret = crosstalk_core_fetch_start(&ctx->fetch, parent, url, top);
    if (ret == LDG_ERR_AOK)
    {
        toast_push(ctx, CROSSTALK_CORE_FETCH_START_TEXT);
        return;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fetch: start failed; parent: %.191s; err: %u32", parent, ret);

    if (ret == LDG_ERR_BUSY) { toast_push(ctx, CROSSTALK_CORE_FETCH_BUSY_TEXT); return; }

    if (ret == LDG_ERR_NOT_FOUND) { toast_push(ctx, CROSSTALK_CORE_FETCH_GIT_TEXT); return; }

    if (ret == LDG_ERR_FUNC_ARG_INVALID) { toast_push(ctx, CROSSTALK_CORE_FETCH_URL_TEXT); return; }

    if (ret == LDG_ERR_EXISTS && ctx->fetch.reason[0] != LDG_STR_TERM)
    {
        if (crosstalk_core_cmdline_out_push(&ctx->cmdline, ctx->fetch.reason) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fetch: reason push failed"); }

        toast_push(ctx, CROSSTALK_CORE_FETCH_GIT_FAIL_TEXT);
        return;
    }

    toast_push(ctx, CROSSTALK_CORE_FETCH_FAIL_TEXT);
}

static LDG_NOINLINE void crosstalk_dir_ld(crosstalk_ctx_t *ctx, const char *arg)
{
    char lead[CROSSTALK_CORE_LD_DIR_CAP] = LDG_ARR_ZERO_INIT;
    char dir[CROSSTALK_CORE_LD_DIR_CAP] = LDG_ARR_ZERO_INIT;
    char full[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char files[CROSSTALK_CORE_LD_FILES_CAP] = LDG_ARR_ZERO_INIT;
    char top[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    if (crosstalk_tail_split(arg, lead, (uint32_t)CROSSTALK_CORE_LD_DIR_CAP, top, (uint32_t)CROSSTALK_CORE_SOUND_ARG_CAP) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_LD_USAGE_TEXT); return; }

    if (crosstalk_tail_split(lead, dir, (uint32_t)CROSSTALK_CORE_LD_DIR_CAP, files, (uint32_t)CROSSTALK_CORE_LD_FILES_CAP) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_LD_USAGE_TEXT); return; }

    if (dir[0] == LDG_STR_TERM || files[0] == LDG_STR_TERM || top[0] == LDG_STR_TERM) { toast_push(ctx, CROSSTALK_CORE_LD_USAGE_TEXT); return; }

    if (crosstalk_core_fetch_url_is(files) == LDG_TRUTH_TRUE)
    {
        crosstalk_dir_fetch(ctx, dir, files, top);
        return;
    }

    if (crosstalk_script_path_resolve(ctx, dir, full, (uint64_t)sizeof(full)) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_LD_USAGE_TEXT); return; }

    crosstalk_fileset_apply(ctx, full, files, top);
}

static void crosstalk_fetch_tick(crosstalk_ctx_t *ctx)
{
    char files[CROSSTALK_EDA_IC_FPGA_FILESET_MAX * CROSSTALK_EDA_IC_FPGA_FILESET_NAME_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t phase = 0;
    uint32_t cunt = 0;
    uint32_t ret = 0;

    if (ctx->fetch.phase != (uint32_t)CROSSTALK_CORE_FETCH_PHASE_RUN) { return; }

    ret = crosstalk_core_fetch_tick(&ctx->fetch, &phase);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fetch: tick failed; err: %u32", ret); }

    if (phase == (uint32_t)CROSSTALK_CORE_FETCH_PHASE_RUN) { return; }

    if (phase != (uint32_t)CROSSTALK_CORE_FETCH_PHASE_DONE)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fetch: git failed; code: %u32; reason: %.191s", ctx->fetch.exit_code, ctx->fetch.reason);

        if (ctx->fetch.reason[0] != LDG_STR_TERM && crosstalk_core_cmdline_out_push(&ctx->cmdline, ctx->fetch.reason) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fetch: reason push failed"); }

        toast_push(ctx, CROSSTALK_CORE_FETCH_GIT_FAIL_TEXT);
    }
    else
    {
        ret = crosstalk_core_fetch_files_get(&ctx->fetch, files, (uint64_t)sizeof(files), &cunt);
        if (ret == LDG_ERR_AOK && cunt > (uint32_t)CROSSTALK_EDA_IC_FPGA_FILESET_MAX) { ret = LDG_ERR_FULL; }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fetch: files; dir: %.191s; cunt: %u32; err: %u32", ctx->fetch.repo_dir, cunt, ret);

        if (ret == LDG_ERR_EMPTY) { toast_push(ctx, CROSSTALK_CORE_FETCH_EMPTY_TEXT); }
        else if (ret == LDG_ERR_NOT_FOUND) { toast_push(ctx, CROSSTALK_CORE_FETCH_TOP_TEXT); }
        else if (ret == LDG_ERR_FULL) { toast_push(ctx, CROSSTALK_CORE_FETCH_FULL_TEXT); }
        else if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_FETCH_FAIL_TEXT); }
        else { crosstalk_fileset_apply(ctx, ctx->fetch.repo_dir, files, ctx->fetch.top); }
    }

    if (crosstalk_core_fetch_stop(&ctx->fetch) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fetch: stop failed"); }
}

static LDG_NOINLINE void crosstalk_design_probes_reattach(crosstalk_ctx_t *ctx, char port[CROSSTALK_EDA_LOGIC_PROBE_MAX][CROSSTALK_EDA_LOGIC_PROBE_PORT_CAP], const uint64_t *period, const uint32_t *width, const uint8_t *host, const uint8_t *held)
{
    const crosstalk_hdl_exec_ctx_t *exec = 0x0;
    const crosstalk_hdl_abi_t *abi = 0x0;
    uint32_t idx = 0;

    if (crosstalk_eda_ic_fpga_slot_exec_get(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, &exec) != LDG_ERR_AOK) { exec = 0x0; }

    if (crosstalk_eda_ic_fpga_slot_abi_get(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, &abi) != LDG_ERR_AOK) { abi = 0x0; }

    for (idx = 0; idx < CROSSTALK_EDA_LOGIC_PROBE_MAX; idx = idx + 0x1)
    {
        uint32_t handle = UINT32_MAX;
        uint32_t ret = 0;

        if (held[idx] != LDG_TRUTH_TRUE) { continue; }

        if (host[idx] == LDG_TRUTH_TRUE) { ret = crosstalk_eda_logic_probe_host_attach(&ctx->eda.logic_probe, CROSSTALK_CORE_TERM_SLOT, port[idx], width[idx], &handle); }
        else if (period[idx] != 0) { ret = crosstalk_eda_logic_probe_period_attach(&ctx->eda.logic_probe, CROSSTALK_CORE_TERM_SLOT, exec, abi, port[idx], period[idx], &handle); }
        else { ret = crosstalk_eda_logic_probe_attach(&ctx->eda.logic_probe, CROSSTALK_CORE_TERM_SLOT, exec, abi, port[idx], &handle); }

        if (ret != LDG_ERR_AOK && host[idx] != LDG_TRUTH_TRUE)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "design reload: probe signal gone; port: %.63s; err: %u32", port[idx], ret);
            ret = crosstalk_eda_logic_probe_host_attach(&ctx->eda.logic_probe, CROSSTALK_CORE_TERM_SLOT, port[idx], width[idx], &handle);
        }

        if (ret != LDG_ERR_AOK || handle != idx) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "design reload: probe reattach; port: %.63s; was: %u32; now: %u32; err: %u32", port[idx], idx, handle, ret); }
    }
}

static LDG_NOINLINE void crosstalk_design_reload(crosstalk_ctx_t *ctx)
{
    char dir[CROSSTALK_EDA_IC_FPGA_DIR_CAP] = LDG_ARR_ZERO_INIT;
    char files[CROSSTALK_EDA_IC_FPGA_FILESET_MAX * CROSSTALK_EDA_IC_FPGA_FILESET_NAME_CAP] = LDG_ARR_ZERO_INIT;
    char top[CROSSTALK_EDA_IC_FPGA_TOP_CAP] = LDG_ARR_ZERO_INIT;
    char port[CROSSTALK_EDA_LOGIC_PROBE_MAX][CROSSTALK_EDA_LOGIC_PROBE_PORT_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t period[CROSSTALK_EDA_LOGIC_PROBE_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t width[CROSSTALK_EDA_LOGIC_PROBE_MAX] = LDG_ARR_ZERO_INIT;
    uint8_t host[CROSSTALK_EDA_LOGIC_PROBE_MAX] = LDG_ARR_ZERO_INIT;
    uint8_t held[CROSSTALK_EDA_LOGIC_PROBE_MAX] = LDG_ARR_ZERO_INIT;
    crosstalk_eda_ic_fpga_comp_t caps = LDG_STRUCT_ZERO_INIT;
    const crosstalk_eda_ic_fpga_slot_t *slot = &ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT];
    uint32_t idx = 0;
    uint32_t ret = 0;

    ret = ldg_str_rbrcpy(dir, slot->fileset_dir, (uint64_t)sizeof(dir));
    if (ret == LDG_ERR_AOK) { ret = ldg_str_rbrcpy(files, slot->fileset_files, (uint64_t)sizeof(files)); }

    if (ret == LDG_ERR_AOK) { ret = ldg_str_rbrcpy(top, slot->top, (uint64_t)sizeof(top)); }

    if (ret != LDG_ERR_AOK || dir[0] == LDG_STR_TERM || files[0] == LDG_STR_TERM || top[0] == LDG_STR_TERM)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "design reload: no file set on the slot; err: %u32", ret);
        return;
    }

    crosstalk_bench_caps_fill(&caps);

    ret = crosstalk_eda_ic_fpga_fileset_ck(&ctx->eda.fpga, dir, files, top, &caps, CROSSTALK_EDA_IC_FPGA_CLK_SIG);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "design reload: skipped, design does not build; top: %.63s; err: %u32", top, ret);
        toast_push(ctx, CROSSTALK_CORE_RELOAD_SKIP_TEXT);
        return;
    }

    for (idx = 0; idx < CROSSTALK_EDA_LOGIC_PROBE_MAX; idx = idx + 0x1)
    {
        const crosstalk_eda_logic_probe_t *probe = &ctx->eda.logic_probe.probe[idx];

        if (probe->live != LDG_TRUTH_TRUE || probe->slot_idx != (uint32_t)CROSSTALK_CORE_TERM_SLOT) { continue; }

        if (ldg_str_rbrcpy(port[idx], probe->port, (uint64_t)CROSSTALK_EDA_LOGIC_PROBE_PORT_CAP) != LDG_ERR_AOK) { continue; }

        period[idx] = probe->period;
        width[idx] = probe->width;
        host[idx] = probe->host;
        held[idx] = LDG_TRUTH_TRUE;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "design reload: start; top: %.63s; dir: %.191s", top, dir);

    crosstalk_fileset_apply(ctx, dir, files, top);

    crosstalk_design_probes_reattach(ctx, port, period, width, host, held);
}

static void crosstalk_wshop_mount_apply(crosstalk_ctx_t *ctx, const crosstalk_wshop_item_t *item)
{
    crosstalk_eda_ic_fpga_comp_t caps = LDG_STRUCT_ZERO_INIT;
    uint32_t domain_idx = UINT32_MAX;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || item == 0x0)) { return; }

    if (item->manifest.kind == CROSSTALK_WSHOP_MANIFEST_KIND_SOFTWARE)
    {
        if (ctx->wshop_core.ok != LDG_TRUTH_TRUE) { toast_push(ctx, "mount a core first"); return; }

        if (crosstalk_core_watch_set_disarm(&ctx->design_watch) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "design watch: disarm failed"); }

        ret = crosstalk_wshop_mount_img(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, &ctx->wshop_core, item);

        if (ret == CROSSTALK_WSHOP_MOUNT_ERR_COMPAT) { toast_push(ctx, "image does not target this core"); return; }

        if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_LD_FAIL_TEXT); return; }

        toast_push(ctx, CROSSTALK_CORE_LD_OK_TEXT);
        return;
    }

    if (item->manifest.kind != CROSSTALK_WSHOP_MANIFEST_KIND_CORE) { toast_push(ctx, "pick a core to mount"); return; }

    crosstalk_bench_caps_fill(&caps);

    if (crosstalk_fpga_hlt(ctx) == LDG_TRUTH_TRUE) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga reprogram resets the component"); }

    ret = crosstalk_wshop_mount_core(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, item, &caps);

    if (ret == CROSSTALK_EDA_IC_FPGA_ERR_OVER_CAP) { toast_push(ctx, "core exceeds component capacity"); return; }

    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_LD_FAIL_TEXT); return; }

    if (crosstalk_eda_clk_domain_add(&ctx->eda.clk, 0x0, caps.f_op_hz, &domain_idx) == LDG_ERR_AOK) { if (crosstalk_eda_ic_fpga_clk_domain_set(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, domain_idx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "wshop mount clk bind failed"); } }

    if (crosstalk_eda_ic_fpga_run_start(&ctx->eda.fpga_run) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga run start failed"); }

    ctx->wshop_core = *item;

    toast_push(ctx, CROSSTALK_CORE_LD_OK_TEXT);
}

static void crosstalk_wshop_publish(crosstalk_ctx_t *ctx, const char *title, const char *isa, const char *license)
{
    const crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    const char *persona = 0x0;
    const char *preview = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || title == 0x0 || isa == 0x0)) { return; }

    if (title[0] == LDG_STR_TERM || isa[0] == LDG_STR_TERM) { toast_push(ctx, CROSSTALK_CORE_PUBLISH_USAGE_TEXT); return; }

    if (crosstalk_eda_ic_fpga_slot_live_is(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT) != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_PUBLISH_NOT_LIVE_TEXT); return; }

    if (crosstalk_eda_ic_fpga_run_live_is(&ctx->eda.fpga_run) != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_PUBLISH_NOT_LIVE_TEXT); return; }

    slot = &ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT];

    if (slot->src == 0x0 || slot->src_len == 0) { toast_push(ctx, CROSSTALK_CORE_PUBLISH_NOT_LIVE_TEXT); return; }

    memset(&ctx->wshop_pub, 0x0, sizeof(ctx->wshop_pub));

    ctx->wshop_pub.rev = CROSSTALK_WSHOP_MANIFEST_REV;
    ctx->wshop_pub.kind = CROSSTALK_WSHOP_MANIFEST_KIND_CORE;
    ctx->wshop_pub.author_id = ldg_net_steamworks_local_steam_id_get();

    persona = ldg_net_steamworks_local_name_get();
    if (persona != 0x0 && snprintf(ctx->wshop_pub.author, sizeof(ctx->wshop_pub.author), "%.127s", persona) < 0) { toast_push(ctx, CROSSTALK_CORE_PUBLISH_USAGE_TEXT); return; }

    if (snprintf(ctx->wshop_pub.name, sizeof(ctx->wshop_pub.name), "%.127s", title) < 0) { toast_push(ctx, CROSSTALK_CORE_PUBLISH_USAGE_TEXT); return; }

    if (snprintf(ctx->wshop_pub.top, sizeof(ctx->wshop_pub.top), "%.63s", slot->top) < 0) { toast_push(ctx, CROSSTALK_CORE_PUBLISH_USAGE_TEXT); return; }

    if (snprintf(ctx->wshop_pub.clk, sizeof(ctx->wshop_pub.clk), "%.63s", slot->clk) < 0) { toast_push(ctx, CROSSTALK_CORE_PUBLISH_USAGE_TEXT); return; }

    if (snprintf(ctx->wshop_pub.isa, sizeof(ctx->wshop_pub.isa), "%.31s", isa) < 0) { toast_push(ctx, CROSSTALK_CORE_PUBLISH_USAGE_TEXT); return; }

    if (license != 0x0 && snprintf(ctx->wshop_pub.license, sizeof(ctx->wshop_pub.license), "%.63s", license) < 0) { toast_push(ctx, CROSSTALK_CORE_PUBLISH_USAGE_TEXT); return; }

    ret = crosstalk_data_subdir_resolve(CROSSTALK_WSHOP_PUBLISH_STAGE_SUBDIR, ctx->wshop_stage_root, (uint64_t)sizeof(ctx->wshop_stage_root));
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_PUBLISH_FAIL_TEXT); return; }

    ret = crosstalk_wshop_publish_stage(ctx->wshop_stage_root, ctx->wshop_pub.top, slot->src, slot->src_len, &ctx->wshop_pub, ctx->wshop_stage_dir, (uint64_t)sizeof(ctx->wshop_stage_dir));
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "publish: stage failed; err: %u32; root: %.191s", ret, ctx->wshop_stage_root);
        toast_push(ctx, CROSSTALK_CORE_PUBLISH_FAIL_TEXT);
        return;
    }

    ret = crosstalk_net_wshop_upload_start(&ctx->wshop, &ctx->wshop_up, CROSSTALK_NET_WSHOP_TYPE_CORE, ctx->wshop_pub.name, ctx->wshop_pub.isa, ctx->wshop_stage_dir, preview);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "publish: core; dir: %.191s; top: %.63s; isa: %.31s; bytes: %u32; preview: %u32; err: %u32", ctx->wshop_stage_dir, ctx->wshop_pub.top, isa, slot->src_len, preview != 0x0 ? 0x1 : 0x0, ret);

    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_PUBLISH_FAIL_TEXT); return; }

    ctx->wshop_up_state = (uint32_t)LDG_NET_STEAMWORKS_WSHOP_UPLOAD_IDLE;

    toast_push(ctx, CROSSTALK_CORE_PUBLISH_OK_TEXT);
}

static void crosstalk_display_attach(crosstalk_ctx_t *ctx, const char *arg)
{
    char lead[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    char hsync[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    char vsync[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    char video[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    const crosstalk_hdl_exec_ctx_t *exec = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    if (crosstalk_tail_split(arg, lead, (uint32_t)CROSSTALK_CORE_SOUND_ARG_CAP, video, (uint32_t)CROSSTALK_CORE_SOUND_ARG_CAP) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_DISPLAY_USAGE_TEXT); return; }

    if (crosstalk_tail_split(lead, hsync, (uint32_t)CROSSTALK_CORE_SOUND_ARG_CAP, vsync, (uint32_t)CROSSTALK_CORE_SOUND_ARG_CAP) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_DISPLAY_USAGE_TEXT); return; }

    if (hsync[0] == LDG_STR_TERM || vsync[0] == LDG_STR_TERM || video[0] == LDG_STR_TERM) { toast_push(ctx, CROSSTALK_CORE_DISPLAY_USAGE_TEXT); return; }

    ret = crosstalk_eda_ic_fpga_slot_exec_get(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, &exec);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_PIPE_NO_DESIGN_TEXT); return; }

    ret = crosstalk_eda_io_display_attach(&ctx->eda.display, CROSSTALK_CORE_TERM_SLOT, exec, CROSSTALK_EDA_IO_DISPLAY_MODE_MONO, hsync, vsync, video);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "display: attach failed; err: %u32; hsync: %.31s; vsync: %.31s; video: %.31s", ret, hsync, vsync, video);
        toast_push(ctx, CROSSTALK_CORE_PIPE_NO_SIG_TEXT);
        return;
    }

    if (crosstalk_eda_ui_display_crt_open_set(&ctx->eda.crt, LDG_TRUTH_TRUE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crt open failed"); }

    toast_push(ctx, CROSSTALK_CORE_DISPLAY_OK_TEXT);
}

static const char* crosstalk_wshop_up_state_text_get(uint32_t state)
{
    if (state == (uint32_t)LDG_NET_STEAMWORKS_WSHOP_UPLOAD_CREATING) { return "creating the item"; }

    if (state == (uint32_t)LDG_NET_STEAMWORKS_WSHOP_UPLOAD_UPDATING) { return "uploading"; }

    if (state == (uint32_t)LDG_NET_STEAMWORKS_WSHOP_UPLOAD_DONE) { return CROSSTALK_CORE_PUBLISH_DONE_TEXT; }

    if (state == (uint32_t)LDG_NET_STEAMWORKS_WSHOP_UPLOAD_ERR) { return CROSSTALK_CORE_PUBLISH_FAIL_TEXT; }

    return "name it, pick an ISA, then publish";
}

static void crosstalk_wshop_upload_watch(crosstalk_ctx_t *ctx)
{
    uint32_t state = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    ret = crosstalk_net_wshop_upload_state_get(&ctx->wshop_up, &state);

    if (state == ctx->wshop_up_state) { return; }

    ctx->wshop_up_state = state;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "wshop upload: state; now: %u32; id: %u64; done: %u64; total: %u64; err: %u32", state, ctx->wshop_up.file_id, ctx->wshop_up.bytes_done, ctx->wshop_up.bytes_total, ret);

    if (state == (uint32_t)LDG_NET_STEAMWORKS_WSHOP_UPLOAD_DONE) { toast_push(ctx, CROSSTALK_CORE_PUBLISH_DONE_TEXT); }

    if (state == (uint32_t)LDG_NET_STEAMWORKS_WSHOP_UPLOAD_ERR) { toast_push(ctx, CROSSTALK_CORE_PUBLISH_FAIL_TEXT); }
}

static uint32_t crosstalk_num_parse(const char *text, uint64_t *out_val)
{
    uint32_t parsed = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(text == 0x0 || out_val == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_val = 0x0;

    if (text[0] == CROSSTALK_CORE_HEX_LEAD && (text[LDG_STR_TERM_SIZE] == 'x' || text[LDG_STR_TERM_SIZE] == 'X')) { ret = ldg_hex_to_dword(&text[LDG_STR_TERM_SIZE + LDG_STR_TERM_SIZE], &parsed); }
    else { ret = ldg_str_to_dec(text, &parsed); }

    if (ret != LDG_ERR_AOK) { return ret; }

    *out_val = (uint64_t)parsed;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_addr_sym_parse(const crosstalk_ctx_t *ctx, const char *text, uint64_t *out_val)
{
    char name[CROSSTALK_HDL_ELF_NAME_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t addr = 0;
    uint64_t off = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0 || out_val == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_val = 0x0;

    while (text[at] != LDG_STR_TERM && text[at] != CROSSTALK_CORE_ADDR_SYM_PLUS && text[at] != ' ' && at + LDG_STR_TERM_SIZE < (uint32_t)sizeof(name)) { name[at] = text[at]; at = at + 0x1; }

    name[at] = LDG_STR_TERM;

    if (name[0] == LDG_STR_TERM) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (crosstalk_hdl_elf_addr_get(&ctx->prog_elf, name, &addr) != LDG_ERR_AOK) { if (crosstalk_hdl_elf_addr_get(&ctx->elf, name, &addr) != LDG_ERR_AOK) { if (crosstalk_eda_ui_calc_var_get(&ctx->eda.calc, name, &addr) != LDG_ERR_AOK) { return LDG_ERR_NOT_FOUND; } } }

    if (text[at] == CROSSTALK_CORE_ADDR_SYM_PLUS) { if (crosstalk_num_parse(&text[at + LDG_STR_TERM_SIZE], &off) != LDG_ERR_AOK) { return LDG_ERR_FUNC_ARG_INVALID; } }

    *out_val = addr + off;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_addr_parse(const crosstalk_ctx_t *ctx, const char *text, uint64_t *out_val)
{
    char base[CROSSTALK_HDL_ELF_NAME_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t addr = 0;
    uint64_t off = 0;
    uint32_t split = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0 || out_val == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_val = 0x0;

    for (at = 0x1; text[at] != LDG_STR_TERM && text[at] != ' '; at = at + 0x1) { if (text[at] == CROSSTALK_CORE_ADDR_SYM_PLUS || text[at] == CROSSTALK_CORE_ADDR_SYM_MINUS) { split = at; break; } }

    if (split == 0)
    {
        if (crosstalk_num_parse(text, out_val) == LDG_ERR_AOK) { return LDG_ERR_AOK; }

        if (crosstalk_addr_sym_parse(ctx, text, out_val) == LDG_ERR_AOK) { return LDG_ERR_AOK; }

        return crosstalk_eda_ui_memview_addr_resolve(&ctx->eda.memview, text, out_val);
    }

    if (split + LDG_STR_TERM_SIZE >= (uint32_t)sizeof(base)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (at = 0; at < split; at = at + 0x1) { base[at] = text[at]; }

    base[split] = LDG_STR_TERM;

    if (crosstalk_num_parse(base, &addr) != LDG_ERR_AOK) { if (crosstalk_addr_sym_parse(ctx, base, &addr) != LDG_ERR_AOK) { return LDG_ERR_NOT_FOUND; } }

    if (crosstalk_num_parse(&text[split + LDG_STR_TERM_SIZE], &off) != LDG_ERR_AOK) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (text[split] == CROSSTALK_CORE_ADDR_SYM_MINUS)
    {
        if (off > addr) { return LDG_ERR_RANGE; }

        *out_val = addr - off;

        return LDG_ERR_AOK;
    }

    *out_val = addr + off;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_addr_tok_parse(const crosstalk_ctx_t *ctx, const char *text, uint64_t *out_val)
{
    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0 || out_val == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (text[0] == CROSSTALK_CORE_HEX_LEAD && (text[LDG_STR_TERM_SIZE] == 'x' || text[LDG_STR_TERM_SIZE] == 'X')) { return crosstalk_addr_parse(ctx, text, out_val); }

    if (ldg_char_digit_is(text[0]) == LDG_TRUTH_TRUE) { return LDG_ERR_NOT_FOUND; }

    return crosstalk_addr_parse(ctx, text, out_val);
}

static void crosstalk_render_dword_put(uint8_t *buff, uint32_t off, uint32_t val)
{
    memcpy(&buff[off], &val, sizeof(val));
}

static void crosstalk_render_word_put(uint8_t *buff, uint32_t off, uint16_t val)
{
    memcpy(&buff[off], &val, sizeof(val));
}

static uint32_t crosstalk_render_sample_conv(uint64_t val, uint32_t width)
{
    uint32_t out = 0;

    if (width >= 0x20) { out = (uint32_t)(val >> (width - 0x20)); }
    else { out = (uint32_t)val << (0x20 - width); }

    return out ^ 0x80000000;
}

static LDG_NOINLINE void crosstalk_verb_render(crosstalk_ctx_t *ctx, const char *arg)
{
    uint8_t chunk[CROSSTALK_CORE_RENDER_CHUNK_SAMPLES * CROSSTALK_CORE_RENDER_BYTES_PER_SAMPLE];
    uint8_t wav_hdr[CROSSTALK_CORE_RENDER_HDR_SIZE] = LDG_ARR_ZERO_INIT;
    char port[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    char secs_text[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    char rate_text[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    ldg_io_file_t *fh = 0x0;
    const char *path = 0x0;
    uint64_t secs = 0;
    uint64_t rate = 0;
    uint64_t clk_hz = 0;
    uint64_t cps = 0;
    uint64_t total = 0;
    uint64_t done_cycles = 0;
    uint64_t sample_at = 0;
    uint64_t peek_val = 0;
    uint32_t peek_width = 0;
    uint32_t chunk_at = 0;
    uint32_t at = 0;
    uint32_t pos = 0;
    uint8_t was_live = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (at < (uint32_t)CROSSTALK_CORE_SOUND_ARG_CAP - LDG_STR_TERM_SIZE && arg[at] != LDG_STR_TERM && arg[at] != ' ')
    {
        port[at] = arg[at];
        at = at + 0x1;
    }

    port[at] = LDG_STR_TERM;

    if (port[0] == LDG_STR_TERM || arg[at] != ' ') { toast_push(ctx, CROSSTALK_CORE_RENDER_USAGE_TEXT); return; }

    at = at + 0x1;
    pos = 0;

    while (pos < (uint32_t)CROSSTALK_CORE_SOUND_ARG_CAP - LDG_STR_TERM_SIZE && arg[at] != LDG_STR_TERM && arg[at] != ' ')
    {
        secs_text[pos] = arg[at];
        at = at + 0x1;
        pos = pos + 0x1;
    }

    secs_text[pos] = LDG_STR_TERM;

    if (secs_text[0] == LDG_STR_TERM || arg[at] != ' ') { toast_push(ctx, CROSSTALK_CORE_RENDER_USAGE_TEXT); return; }

    at = at + 0x1;
    pos = 0;

    while (pos < (uint32_t)CROSSTALK_CORE_SOUND_ARG_CAP - LDG_STR_TERM_SIZE && arg[at] != LDG_STR_TERM && arg[at] != ' ')
    {
        rate_text[pos] = arg[at];
        at = at + 0x1;
        pos = pos + 0x1;
    }

    rate_text[pos] = LDG_STR_TERM;

    if (rate_text[0] == LDG_STR_TERM || arg[at] != ' ') { toast_push(ctx, CROSSTALK_CORE_RENDER_USAGE_TEXT); return; }

    path = &arg[at + LDG_STR_TERM_SIZE];

    if (path[0] == LDG_STR_TERM) { toast_push(ctx, CROSSTALK_CORE_RENDER_USAGE_TEXT); return; }

    if (crosstalk_num_parse(secs_text, &secs) != LDG_ERR_AOK || crosstalk_num_parse(rate_text, &rate) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RENDER_USAGE_TEXT); return; }

    if (secs == 0 || secs > (uint64_t)CROSSTALK_CORE_RENDER_SECS_MAX) { toast_push(ctx, "render length is out of bounds"); return; }

    if (rate < (uint64_t)CROSSTALK_CORE_RENDER_RATE_MIN || rate > (uint64_t)CROSSTALK_CORE_RENDER_RATE_MAX) { toast_push(ctx, "render rate is out of bounds"); return; }

    ret = crosstalk_eda_ic_fpga_sig_peek(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, port, &peek_val, &peek_width);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, "no such port in the design"); return; }

    clk_hz = ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT].comp.f_op_hz;
    cps = clk_hz / rate;

    if (cps == 0) { toast_push(ctx, "render rate is above the design clock"); return; }

    total = secs * rate;

    ret = ldg_io_file_open(path, LDG_IO_WR_ONLY | LDG_IO_CREATE | LDG_IO_TRUNCATE, 0x0, &fh);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, "render file open failed"); return; }

    memcpy(&wav_hdr[0x0], "RIFF", 0x4);
    crosstalk_render_dword_put(wav_hdr, 0x4, (uint32_t)(total * CROSSTALK_CORE_RENDER_BYTES_PER_SAMPLE) + (uint32_t)CROSSTALK_CORE_RENDER_HDR_SIZE - 0x8);
    memcpy(&wav_hdr[0x8], "WAVE", 0x4);
    memcpy(&wav_hdr[0xC], "fmt ", 0x4);
    crosstalk_render_dword_put(wav_hdr, 0x10, 0x10);
    crosstalk_render_word_put(wav_hdr, 0x14, 0x1);
    crosstalk_render_word_put(wav_hdr, 0x16, 0x1);
    crosstalk_render_dword_put(wav_hdr, 0x18, (uint32_t)rate);
    crosstalk_render_dword_put(wav_hdr, 0x1C, (uint32_t)(rate * CROSSTALK_CORE_RENDER_BYTES_PER_SAMPLE));
    crosstalk_render_word_put(wav_hdr, 0x20, (uint16_t)CROSSTALK_CORE_RENDER_BYTES_PER_SAMPLE);
    crosstalk_render_word_put(wav_hdr, 0x22, 0x20);
    memcpy(&wav_hdr[0x24], "data", 0x4);
    crosstalk_render_dword_put(wav_hdr, 0x28, (uint32_t)(total * CROSSTALK_CORE_RENDER_BYTES_PER_SAMPLE));

    ret = ldg_io_file_wr(fh, wav_hdr, (uint64_t)sizeof(wav_hdr), 0x0);
    if (ret != LDG_ERR_AOK)
    {
        if (ldg_io_file_close(fh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "render file close failed"); }

        toast_push(ctx, CROSSTALK_CORE_RENDER_FAIL_TEXT);
        return;
    }

    was_live = crosstalk_fpga_hlt(ctx);

    for (sample_at = 0; sample_at < total; sample_at++)
    {
        uint32_t conv = 0;

        ret = crosstalk_eda_ic_fpga_cycles_run(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, cps, &done_cycles);
        if (ret != LDG_ERR_AOK) { break; }

        ret = crosstalk_eda_ic_fpga_sig_peek(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, port, &peek_val, &peek_width);
        if (ret != LDG_ERR_AOK) { break; }

        conv = crosstalk_render_sample_conv(peek_val, peek_width);
        crosstalk_render_dword_put(chunk, chunk_at * (uint32_t)CROSSTALK_CORE_RENDER_BYTES_PER_SAMPLE, conv);
        chunk_at = chunk_at + 0x1;

        if (chunk_at == (uint32_t)CROSSTALK_CORE_RENDER_CHUNK_SAMPLES)
        {
            ret = ldg_io_file_wr(fh, chunk, (uint64_t)chunk_at * (uint64_t)CROSSTALK_CORE_RENDER_BYTES_PER_SAMPLE, 0x0);
            if (ret != LDG_ERR_AOK) { break; }

            chunk_at = 0;
        }
    }

    if (ret == LDG_ERR_AOK && chunk_at != 0) { ret = ldg_io_file_wr(fh, chunk, (uint64_t)chunk_at * (uint64_t)CROSSTALK_CORE_RENDER_BYTES_PER_SAMPLE, 0x0); }

    if (ldg_io_file_close(fh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "render file close failed"); }

    crosstalk_fpga_resume(ctx, was_live);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "render: %s; port: %.63s; secs: %u64; rate: %u64; clk: %u64; cps: %u64; samples: %u64; err: %u32", ret == LDG_ERR_AOK ? "done" : "failed", port, secs, rate, clk_hz, cps, total, ret);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "render: out; path: %.191s", path);

    toast_push(ctx, ret == LDG_ERR_AOK ? CROSSTALK_CORE_RENDER_OK_TEXT : CROSSTALK_CORE_RENDER_FAIL_TEXT);
}

static void crosstalk_poke(crosstalk_ctx_t *ctx, const char *arg)
{
    char sig[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t val = 0;
    uint64_t got = 0;
    uint32_t width = 0;
    uint32_t at = 0;
    uint32_t ret = 0;
    uint8_t was_live = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (at < (uint32_t)CROSSTALK_CORE_SOUND_ARG_CAP - LDG_STR_TERM_SIZE && arg[at] != LDG_STR_TERM && arg[at] != ' ')
    {
        sig[at] = arg[at];
        at = at + 0x1;
    }

    sig[at] = LDG_STR_TERM;

    if (sig[0] == LDG_STR_TERM) { toast_push(ctx, CROSSTALK_CORE_POKE_USAGE_TEXT); return; }

    if (arg[at] != ' ')
    {
        ret = crosstalk_eda_ic_fpga_sig_peek(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, sig, &got, &width);
        if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_MEMLD_NO_SIG_TEXT); return; }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "peek: sig: %.63s; val: 0x%X64; width: %u32", sig, got, width);
        toast_push(ctx, CROSSTALK_CORE_POKE_RD_TEXT);
        return;
    }

    if (crosstalk_num_parse(&arg[at + LDG_STR_TERM_SIZE], &val) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_POKE_USAGE_TEXT); return; }

    was_live = crosstalk_fpga_hlt(ctx);

    ret = crosstalk_eda_ic_fpga_sig_poke(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, sig, val, LDG_TRUTH_FALSE);

    crosstalk_fpga_resume(ctx, was_live);

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "poke: failed; sig: %.63s; err: %u32", sig, ret);
        toast_push(ctx, ret == CROSSTALK_EDA_IC_FPGA_ERR_MEM_SIG ? CROSSTALK_CORE_MEMLD_NO_SIG_TEXT : CROSSTALK_CORE_POKE_FAIL_TEXT);
        return;
    }

    toast_push(ctx, CROSSTALK_CORE_POKE_OK_TEXT);
}

static uint32_t crosstalk_defer_push(crosstalk_ctx_t *ctx, uint32_t kind, const char *text, uint32_t len)
{
    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->defer_cunt >= (uint32_t)CROSSTALK_CORE_DEFER_CAP) { return LDG_ERR_FULL; }

    if (len >= (uint32_t)CROSSTALK_CORE_DEFER_TEXT_CAP) { return LDG_ERR_FULL; }

    memcpy(ctx->defer_text[ctx->defer_cunt], text, len);

    ctx->defer_text[ctx->defer_cunt][len] = LDG_STR_TERM;
    ctx->defer_kind[ctx->defer_cunt] = kind;
    ctx->defer_cunt = ctx->defer_cunt + 0x1;

    return LDG_ERR_AOK;
}

static void crosstalk_verb_switch(crosstalk_ctx_t *ctx, const char *arg)
{
    char name[CROSSTALK_CORE_SWITCH_NAME_CAP] = LDG_ARR_ZERO_INIT;
    const crosstalk_eda_script_switch_t *switch_ref = 0x0;
    crosstalk_eda_script_val_t pos = LDG_STRUCT_ZERO_INIT;
    const char *action = 0x0;
    uint64_t val = 0;
    uint32_t action_len = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (at < (uint32_t)CROSSTALK_CORE_SWITCH_NAME_CAP - LDG_STR_TERM_SIZE && arg[at] != LDG_STR_TERM && arg[at] != ' ')
    {
        name[at] = arg[at];
        at = at + 0x1;
    }

    name[at] = LDG_STR_TERM;

    if (name[0] == LDG_STR_TERM) { toast_push(ctx, CROSSTALK_CORE_SWITCH_USAGE_TEXT); return; }

    ret = crosstalk_eda_script_eval_switch_find(&ctx->eda.script.eval, name, at, &switch_ref);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_SWITCH_NONE_TEXT); return; }

    if (arg[at] != ' ')
    {
        ret = crosstalk_eda_script_eval_slot_get(&ctx->eda.script.eval, name, at, &pos);
        if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_SWITCH_NONE_TEXT); return; }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "switch: %.63s; pos: 0x%X64; kind: %u32", name, pos.val, switch_ref->kind);
        toast_push(ctx, CROSSTALK_CORE_SWITCH_RD_TEXT);
        return;
    }

    if (crosstalk_num_parse(&arg[at + LDG_STR_TERM_SIZE], &val) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_SWITCH_USAGE_TEXT); return; }

    ret = crosstalk_eda_script_eval_switch_pos_set(&ctx->eda.script.eval, name, at, val, &action, &action_len);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "switch: refused; name: %.63s; val: 0x%X64; err: %u32", name, val, ret);
        toast_push(ctx, CROSSTALK_CORE_SWITCH_FAIL_TEXT);
        return;
    }

    if (action_len == 0)
    {
        if (switch_ref->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_SWITCH_BUTTON) { if (crosstalk_eda_script_eval_switch_release(&ctx->eda.script.eval, name, at) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "switch: release failed; name: %.63s", name); } }

        toast_push(ctx, CROSSTALK_CORE_SWITCH_OK_TEXT);
        return;
    }

    ret = crosstalk_defer_push(ctx, (uint32_t)CROSSTALK_CORE_DEFER_SWITCH, name, at);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "switch: cascade dropped; name: %.63s; err: %u32", name, ret);
        toast_push(ctx, CROSSTALK_CORE_SWITCH_DEEP_TEXT);
        return;
    }

    toast_push(ctx, CROSSTALK_CORE_SWITCH_OK_TEXT);
}

static void crosstalk_defer_drain(crosstalk_ctx_t *ctx)
{
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    while (idx < ctx->defer_cunt)
    {
        const char *text = ctx->defer_text[idx];
        const crosstalk_eda_script_switch_t *switch_ref = 0x0;
        uint32_t kind = ctx->defer_kind[idx];
        uint32_t text_len = 0;

        idx = idx + 0x1;

        while (text[text_len] != LDG_STR_TERM) { text_len = text_len + 0x1; }

        if (kind == (uint32_t)CROSSTALK_CORE_DEFER_RUN || kind == (uint32_t)CROSSTALK_CORE_DEFER_EVENT)
        {
            char note[CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CAP] = LDG_ARR_ZERO_INIT;

            if (kind == (uint32_t)CROSSTALK_CORE_DEFER_EVENT) { ctx->event_depth = ctx->event_depth + 0x1; }

            ret = crosstalk_eda_script_file_exec(&ctx->eda.script, text);

            if (kind == (uint32_t)CROSSTALK_CORE_DEFER_EVENT && ctx->event_depth != 0) { ctx->event_depth = ctx->event_depth - 0x1; }

            if (ret == LDG_ERR_AOK) { continue; }

            if (snprintf(note, sizeof(note), CROSSTALK_CORE_SCRIPT_RUN_FAIL_FMT, ret, text) < 0) { continue; }

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "script: %.128s", note);

            if (crosstalk_core_cmdline_out_push(&ctx->cmdline, note) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "script run note push failed"); }

            continue;
        }

        if (crosstalk_eda_script_eval_switch_find(&ctx->eda.script.eval, text, text_len, &switch_ref) != LDG_ERR_AOK) { continue; }

        if (switch_ref->action_len != 0)
        {
            ctx->eda.script.eval.action_is = LDG_TRUTH_TRUE;

            ret = crosstalk_eda_script_exec(&ctx->eda.script, CROSSTALK_CORE_SWITCH_EXEC_NAME, switch_ref->action, switch_ref->action_len);

            ctx->eda.script.eval.action_is = LDG_TRUTH_FALSE;

            if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "switch: action failed; name: %.63s; err: %u32", text, ret); }
        }

        if (switch_ref->kind == (uint32_t)CROSSTALK_EDA_SCRIPT_SWITCH_BUTTON) { if (crosstalk_eda_script_eval_switch_release(&ctx->eda.script.eval, text, text_len) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "switch: release failed; name: %.63s", text); } }
    }

    ctx->defer_cunt = 0;
}

static void crosstalk_event_fire(crosstalk_ctx_t *ctx, uint32_t event)
{
    const char *path = 0x0;
    const char *name = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (crosstalk_eda_script_event_get(&ctx->eda.script, event, &path) != LDG_ERR_AOK || path == 0x0) { return; }

    if (crosstalk_eda_script_event_name_get(event, &name) != LDG_ERR_AOK) { name = 0x0; }

    if (ctx->event_depth >= (uint32_t)CROSSTALK_CORE_EVENT_DEPTH_MAX)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "event: refused; nested; event: %.7s; depth: %u32", name != 0x0 ? name : CROSSTALK_CORE_EMU_OSC_NONE_TEXT, ctx->event_depth);

        return;
    }

    ret = crosstalk_defer_push(ctx, (uint32_t)CROSSTALK_CORE_DEFER_EVENT, path, (uint32_t)strlen(path));
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "event: queue full; event: %.7s; err: %u32", name != 0x0 ? name : CROSSTALK_CORE_EMU_OSC_NONE_TEXT, ret);

        return;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "event: fired; event: %.7s; path: %.127s", name != 0x0 ? name : CROSSTALK_CORE_EMU_OSC_NONE_TEXT, path);
}

static void crosstalk_exit_event_run(crosstalk_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    crosstalk_event_fire(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_EVENT_EXIT);

    crosstalk_defer_drain(ctx);
}

static void crosstalk_syms_sibling_ld(crosstalk_ctx_t *ctx, const char *path)
{
    char elf_path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t at = 0;
    uint32_t dot_at = UINT32_MAX;
    uint32_t sep_at = UINT32_MAX;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || path == 0x0)) { return; }

    while (at < (uint32_t)LDG_IO_PATH_MAX - LDG_STR_TERM_SIZE && path[at] != LDG_STR_TERM)
    {
        if (path[at] == CROSSTALK_CORE_SYMS_DOT) { dot_at = at; }

        if (path[at] == CROSSTALK_CORE_SYMS_SEP_POSIX || path[at] == CROSSTALK_CORE_SYMS_SEP_WIN) { sep_at = at; }

        elf_path[at] = path[at];
        at = at + 0x1;
    }

    if (dot_at == UINT32_MAX) { return; }

    if (sep_at != UINT32_MAX && dot_at < sep_at) { return; }

    if ((uint64_t)dot_at + sizeof(CROSSTALK_CORE_SYMS_EXT) > (uint64_t)LDG_IO_PATH_MAX) { return; }

    memcpy(elf_path + dot_at, CROSSTALK_CORE_SYMS_EXT, sizeof(CROSSTALK_CORE_SYMS_EXT));

    if (ldg_io_path_exists_is(elf_path) != LDG_TRUTH_TRUE) { return; }

    ret = crosstalk_hdl_elf_ld(&ctx->elf, elf_path);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "syms: ld failed; path: %.127s; err: %u32", elf_path, ret);
        return;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "syms: ready; path: %.127s; syms: %u32", elf_path, crosstalk_hdl_elf_sym_cunt_get(&ctx->elf));
}

static void crosstalk_verb_syms(crosstalk_ctx_t *ctx, const char *arg)
{
    char path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    if (arg[0] == LDG_STR_TERM)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "syms: prog; path: %.127s; syms: %u32; types: %u32; vars: %u32; base: 0x%X64", ctx->prog_elf.path, crosstalk_hdl_elf_sym_cunt_get(&ctx->prog_elf), crosstalk_hdl_type_cunt_get(&ctx->prog_type), crosstalk_hdl_type_var_cunt_get(&ctx->prog_type), crosstalk_hdl_elf_base_get(&ctx->prog_elf));

        crosstalk_ach_once(ctx, CROSSTALK_GAME_PROGRESSION_FIRST_SYMS);
        return;
    }

    if (ldg_str_rbrcpy(path, arg, (uint64_t)sizeof(path)) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_SYMS_USAGE_TEXT); return; }

    if (ldg_io_path_exists_is(path) != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_SYMS_NO_FILE_TEXT); return; }

    ret = crosstalk_hdl_elf_ld(&ctx->prog_elf, path);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "syms: prog ld failed; path: %.127s; err: %u32", path, ret);
        toast_push(ctx, CROSSTALK_CORE_SYMS_FAIL_TEXT);
        return;
    }

    ret = crosstalk_hdl_type_ld(&ctx->prog_type, path);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "syms: no types; path: %.127s; err: %u32", path, ret); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "syms: prog ready; path: %.127s; syms: %u32; types: %u32; vars: %u32", path, crosstalk_hdl_elf_sym_cunt_get(&ctx->prog_elf), crosstalk_hdl_type_cunt_get(&ctx->prog_type), crosstalk_hdl_type_var_cunt_get(&ctx->prog_type));

    toast_push(ctx, CROSSTALK_CORE_SYMS_OK_TEXT);
}

static uint32_t crosstalk_re_word_next(const char *arg, uint32_t *at, char *out, uint32_t cap, uint8_t *out_quoted)
{
    uint32_t wr = 0;
    uint8_t quoted = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(arg == 0x0 || at == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    out[0] = LDG_STR_TERM;

    if (out_quoted != 0x0) { *out_quoted = LDG_TRUTH_FALSE; }

    while (arg[*at] == ' ') { *at = *at + 0x1; }

    if (arg[*at] == LDG_STR_TERM) { return LDG_ERR_EOF; }

    if (arg[*at] == LDG_ASCII_QUOTE)
    {
        quoted = LDG_TRUTH_TRUE;
        *at = *at + 0x1;
    }

    while (arg[*at] != LDG_STR_TERM)
    {
        if (quoted == LDG_TRUTH_TRUE && arg[*at] == LDG_ASCII_QUOTE) { *at = *at + 0x1; break; }

        if (quoted != LDG_TRUTH_TRUE && arg[*at] == ' ') { break; }

        if (wr + LDG_STR_TERM_SIZE >= cap) { return LDG_ERR_STR_TRUNC; }

        out[wr] = arg[*at];
        wr = wr + 0x1;
        *at = *at + 0x1;
    }

    out[wr] = LDG_STR_TERM;

    if (out_quoted != 0x0) { *out_quoted = quoted; }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_re_num_parse(const char *text, uint64_t *out)
{
    if (LDG_UNLIKELY(text == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (text[0] == LDG_STR_TERM) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (text[0] == CROSSTALK_CORE_HEX_LEAD && (text[LDG_STR_TERM_SIZE] == 'x' || text[LDG_STR_TERM_SIZE] == 'X')) { return ldg_str_uint_parse(&text[LDG_STR_TERM_SIZE + LDG_STR_TERM_SIZE], (uint32_t)LDG_BASE_HEX, (uint64_t)CROSSTALK_CORE_RE_WORD_CAP, out); }

    return ldg_str_uint_parse(text, (uint32_t)LDG_BASE_DEC, (uint64_t)CROSSTALK_CORE_RE_WORD_CAP, out);
}

static uint32_t crosstalk_re_addr_parse(const crosstalk_ctx_t *ctx, const char *text, uint64_t *out)
{
    char base[CROSSTALK_RE_DB_NAME_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t addr = 0;
    uint64_t off = 0;
    uint32_t split = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (text[0] == LDG_STR_TERM) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (at = 0x1; text[at] != LDG_STR_TERM; at = at + 0x1) { if (text[at] == CROSSTALK_CORE_ADDR_SYM_PLUS) { split = at; break; } }

    if (split + LDG_STR_TERM_SIZE >= (uint32_t)sizeof(base)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (at = 0; at < split; at = at + 0x1) { base[at] = text[at]; }

    if (split == 0) { for (at = 0; text[at] != LDG_STR_TERM && at + LDG_STR_TERM_SIZE < (uint32_t)sizeof(base); at = at + 0x1) { base[at] = text[at]; } }

    base[at] = LDG_STR_TERM;

    if (crosstalk_re_num_parse(base, &addr) != LDG_ERR_AOK) { if (crosstalk_re_db_addr_get(&ctx->re.db, base, &addr) != LDG_ERR_AOK && crosstalk_hdl_elf_addr_get(&ctx->re.img.elf, base, &addr) != LDG_ERR_AOK) { return LDG_ERR_NOT_FOUND; } }

    if (split != 0) { if (crosstalk_re_num_parse(&text[split + LDG_STR_TERM_SIZE], &off) != LDG_ERR_AOK) { return LDG_ERR_FUNC_ARG_INVALID; } }

    *out = addr + off;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_re_label_get(const crosstalk_ctx_t *ctx, uint64_t addr, char *out, uint32_t cap)
{
    const char *name = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (cap == 0) { return LDG_ERR_BOUNDS; }

    out[0] = LDG_STR_TERM;

    if (crosstalk_re_db_name_get(&ctx->re.db, addr, &name) == LDG_ERR_AOK && name != 0x0) { return ldg_str_rbrcpy(out, name, (uint64_t)cap); }

    if (ctx->re.img.syms_ok == LDG_TRUTH_TRUE) { return crosstalk_hdl_elf_label_get(&ctx->re.img.elf, addr, out, cap); }

    return LDG_ERR_NOT_FOUND;
}

static void crosstalk_re_open(crosstalk_ctx_t *ctx, const char *rest)
{
    char path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char word[CROSSTALK_CORE_RE_WORD_CAP] = LDG_ARR_ZERO_INIT;
    char db_path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char leaf[CROSSTALK_RE_DB_NAME_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t base = 0;
    uint32_t isa = (uint32_t)CROSSTALK_RE_IMG_ISA_NONE;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || rest == 0x0)) { return; }

    if (crosstalk_re_word_next(rest, &at, path, (uint32_t)sizeof(path), 0x0) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_NO_IMG_TEXT); return; }

    if (crosstalk_re_word_next(rest, &at, word, (uint32_t)sizeof(word), 0x0) == LDG_ERR_AOK) { if (crosstalk_re_num_parse(word, &base) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; } }

    if (crosstalk_re_word_next(rest, &at, word, (uint32_t)sizeof(word), 0x0) == LDG_ERR_AOK) { if (crosstalk_re_img_isa_parse(word, &isa) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_OPEN_ISA_TEXT); return; } }

    if (ldg_io_path_exists_is(path) != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_SYMS_NO_FILE_TEXT); return; }

    ret = crosstalk_re_img_ld(&ctx->re.img, path, base, isa);
    if (ret == CROSSTALK_RE_IMG_ERR_ISA) { toast_push(ctx, CROSSTALK_CORE_RE_OPEN_ISA_TEXT); return; }

    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_OPEN_FAIL_TEXT); return; }

    if (crosstalk_re_db_clear(&ctx->re.db) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re db clear failed"); }

    if (crosstalk_re_diff_clear(&ctx->re.diff) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re diff clear failed"); }

    if (crosstalk_re_cc_step_clear(&ctx->re.cc) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re cc clear failed"); }

    if (crosstalk_re_leaf_get(path, leaf, (uint32_t)sizeof(leaf)) == LDG_ERR_AOK)
    {
        uint32_t proj = (uint32_t)UINT32_MAX;

        if (crosstalk_re_db_img_set(&ctx->re.db, ctx->re.img.hash, leaf) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re db img pin failed"); }

        if (crosstalk_reng_proj_hash_find(&ctx->reng, ctx->re.img.hash, &proj) != LDG_ERR_AOK)
        {
            ret = crosstalk_reng_proj_add(&ctx->reng, leaf, (uint32_t)CROSSTALK_RENG_KIND_REVERSE, path, ctx->re.img.hash, &proj);

            if (ret == LDG_ERR_EXISTS && crosstalk_reng_proj_find(&ctx->reng, leaf, &proj) == LDG_ERR_AOK) { ctx->reng.proj[proj].img_hash = ctx->re.img.hash; ret = LDG_ERR_AOK; }

            if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re: project record refused; leaf: %.63s; err: %u32", leaf, ret); }
        }

        if (proj != (uint32_t)UINT32_MAX && crosstalk_reng_sel_set(&ctx->reng, proj) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "reng sel set failed"); }
    }

    if (crosstalk_re_db_path_build(path, db_path, (uint32_t)sizeof(db_path)) == LDG_ERR_AOK)
    {
        if (ldg_str_rbrcpy(ctx->re.db.path, db_path, (uint64_t)sizeof(ctx->re.db.path)) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re db path truncated"); }

        if (ldg_io_path_exists_is(db_path) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_defer_push(ctx, (uint32_t)CROSSTALK_CORE_DEFER_RUN, db_path, (uint32_t)strlen(db_path));

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "re: db replay queued; path: %.127s; err: %u32", db_path, ret);
        }
    }

    toast_push(ctx, CROSSTALK_CORE_RE_OPEN_OK_TEXT);
}

static void crosstalk_re_close(crosstalk_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (crosstalk_re_db_dirty_is(&ctx->re.db) == LDG_TRUTH_TRUE) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re: closing with unsaved database changes; records: %u32", crosstalk_re_db_rec_cunt_get(&ctx->re.db)); }

    if (crosstalk_re_img_clear(&ctx->re.img) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re img clear failed"); }

    if (crosstalk_re_db_clear(&ctx->re.db) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re db clear failed"); }

    toast_push(ctx, CROSSTALK_CORE_RE_CLOSED_TEXT);
}

static void crosstalk_re_state_log(crosstalk_ctx_t *ctx)
{
    const char *verdict = 0x0;
    const char *form = 0x0;
    const char *isa = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (crosstalk_re_img_ld_is(&ctx->re.img) != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_RE_NO_IMG_TEXT); return; }

    if (crosstalk_re_diff_verdict_name_get(ctx->re.diff.verdict, &verdict) != LDG_ERR_AOK) { verdict = CROSSTALK_CORE_EMU_OSC_NONE_TEXT; }

    if (crosstalk_re_img_form_name_get(ctx->re.img.form, &form) != LDG_ERR_AOK) { form = CROSSTALK_CORE_EMU_OSC_NONE_TEXT; }

    if (crosstalk_re_img_isa_name_get(ctx->re.img.isa, &isa) != LDG_ERR_AOK) { isa = CROSSTALK_CORE_EMU_OSC_NONE_TEXT; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "re: artifact: %.127s; form: %s; isa: %s; base: 0x%X64; lo: 0x%X64; hi: 0x%X64; segs: %u32; entry: 0x%X64; hash: 0x%08X32; syms: %u32; records: %u32; dirty: %u32; db: %.127s; build steps: %u32; verdict: %s", ctx->re.img.path, form, isa, ctx->re.img.base, ctx->re.img.lo, ctx->re.img.hi, ctx->re.img.seg_cunt, ctx->re.img.entry, ctx->re.img.hash, ctx->re.img.syms_ok == LDG_TRUTH_TRUE ? crosstalk_hdl_elf_sym_cunt_get(&ctx->re.img.elf) : 0x0, crosstalk_re_db_rec_cunt_get(&ctx->re.db), (uint32_t)crosstalk_re_db_dirty_is(&ctx->re.db), ctx->re.db.path, crosstalk_re_cc_step_cunt_get(&ctx->re.cc), verdict);
}

static void crosstalk_re_seg_log(crosstalk_ctx_t *ctx)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (crosstalk_re_img_ld_is(&ctx->re.img) != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_RE_NO_IMG_TEXT); return; }

    for (idx = 0; idx < ctx->re.img.seg_cunt; idx = idx + 0x1)
    {
        const crosstalk_re_img_seg_t *seg = 0x0;

        if (crosstalk_re_img_seg_get(&ctx->re.img, idx, &seg) != LDG_ERR_AOK || seg == 0x0) { continue; }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "re seg: %.15s; addr: 0x%X64; span: 0x%X64; file off: 0x%X64; file span: 0x%X64; flags: %c%c%c", seg->name, seg->addr, seg->span, seg->file_off, seg->file_span, (seg->flags & (uint32_t)CROSSTALK_RE_IMG_SEG_FLAG_R) != 0 ? 'r' : '-', (seg->flags & (uint32_t)CROSSTALK_RE_IMG_SEG_FLAG_W) != 0 ? 'w' : '-', (seg->flags & (uint32_t)CROSSTALK_RE_IMG_SEG_FLAG_X) != 0 ? 'x' : '-');
    }
}

static void crosstalk_re_rd(crosstalk_ctx_t *ctx, const char *rest)
{
    char word[CROSSTALK_CORE_RE_WORD_CAP] = LDG_ARR_ZERO_INIT;
    char line[CROSSTALK_CORE_RE_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint8_t bytes[CROSSTALK_CORE_RE_RD_BYTES_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t addr = 0;
    uint64_t want = (uint64_t)CROSSTALK_CORE_RE_RD_BYTES_DEFAULT;
    uint64_t got = 0;
    uint64_t row = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || rest == 0x0)) { return; }

    if (crosstalk_re_img_ld_is(&ctx->re.img) != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_RE_NO_IMG_TEXT); return; }

    if (crosstalk_re_word_next(rest, &at, word, (uint32_t)sizeof(word), 0x0) != LDG_ERR_AOK || crosstalk_re_addr_parse(ctx, word, &addr) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; }

    if (crosstalk_re_word_next(rest, &at, word, (uint32_t)sizeof(word), 0x0) == LDG_ERR_AOK) { if (crosstalk_re_num_parse(word, &want) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; } }

    if (want > (uint64_t)CROSSTALK_CORE_RE_RD_BYTES_MAX) { want = (uint64_t)CROSSTALK_CORE_RE_RD_BYTES_MAX; }

    if (crosstalk_re_img_rd(&ctx->re.img, addr, bytes, want, &got) != LDG_ERR_AOK || got == 0) { toast_push(ctx, CROSSTALK_CORE_RE_ADDR_TEXT); return; }

    for (row = 0; row < got; row = row + (uint64_t)CROSSTALK_CORE_RE_RD_ROW_BYTES)
    {
        uint64_t col = 0;
        uint32_t pos = 0;
        int32_t wrote = snprintf(line, sizeof(line), "re rd: 0x%08" PRIX64 ":", addr + row);

        if (wrote < 0) { break; }

        pos = (uint32_t)wrote;

        for (col = 0; col < (uint64_t)CROSSTALK_CORE_RE_RD_ROW_BYTES && row + col < got; col = col + 0x1)
        {
            wrote = snprintf(&line[pos], sizeof(line) - pos, CROSSTALK_CORE_MEMRD_BYTE_FMT, (uint64_t)bytes[row + col]);
            if (wrote < 0 || (uint32_t)wrote >= sizeof(line) - pos) { break; }

            pos = pos + (uint32_t)wrote;
        }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "%.383s", line);
    }
}

static void crosstalk_re_dis(crosstalk_ctx_t *ctx, const char *rest)
{
    char word[CROSSTALK_CORE_RE_WORD_CAP] = LDG_ARR_ZERO_INIT;
    char label[CROSSTALK_HDL_ELF_LABEL_CAP] = LDG_ARR_ZERO_INIT;
    char text[CROSSTALK_CORE_RE_DIS_TEXT_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t addr = 0;
    uint64_t rows = (uint64_t)CROSSTALK_CORE_RE_DIS_ROWS_DEFAULT;
    uint64_t row = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || rest == 0x0)) { return; }

    if (crosstalk_re_img_ld_is(&ctx->re.img) != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_RE_NO_IMG_TEXT); return; }

    if (crosstalk_re_word_next(rest, &at, word, (uint32_t)sizeof(word), 0x0) != LDG_ERR_AOK) { addr = ctx->re.img.entry; }
    else if (crosstalk_re_addr_parse(ctx, word, &addr) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; }

    if (crosstalk_re_word_next(rest, &at, word, (uint32_t)sizeof(word), 0x0) == LDG_ERR_AOK) { if (crosstalk_re_num_parse(word, &rows) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; } }

    if (rows > (uint64_t)CROSSTALK_CORE_RE_DIS_ROWS_MAX) { rows = (uint64_t)CROSSTALK_CORE_RE_DIS_ROWS_MAX; }

    if (ctx->re.img.isa != (uint32_t)CROSSTALK_RE_IMG_ISA_DANGCSA) { toast_push(ctx, CROSSTALK_CORE_RE_DIS_NONE_TEXT); return; }

    for (row = 0; row < rows; row = row + 0x1)
    {
        ldg_dangcsa_decoded_t decoded = LDG_STRUCT_ZERO_INIT;
        uint64_t here = addr + row * (uint64_t)CROSSTALK_RE_IMG_WORD_BYTES;
        uint32_t raw = 0;

        if (crosstalk_re_img_word_get(&ctx->re.img, here, &raw) != LDG_ERR_AOK)
        {
            if (row == 0) { toast_push(ctx, CROSSTALK_CORE_RE_ADDR_TEXT); }

            break;
        }

        if (ldg_dangcsa_decode(raw, &decoded) != LDG_ERR_AOK || ldg_dangcsa_disasm(&decoded, text, sizeof(text)) != LDG_ERR_AOK) { if (ldg_str_rbrcpy(text, CROSSTALK_CORE_EMU_OSC_NONE_TEXT, sizeof(text)) != LDG_ERR_AOK) { break; } }

        if (crosstalk_re_label_get(ctx, here, label, (uint32_t)sizeof(label)) != LDG_ERR_AOK) { label[0] = LDG_STR_TERM; }

        if (label[0] != LDG_STR_TERM) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "re dis: 0x%08X64; %08X32; %.127s; %.95s", here, raw, label, text); }
        else { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "re dis: 0x%08X64; %08X32; %.95s", here, raw, text); }
    }
}

static void crosstalk_re_rec_push(crosstalk_ctx_t *ctx, const crosstalk_re_db_rec_t *rec)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || rec == 0x0)) { return; }

    ret = crosstalk_re_db_rec_add(&ctx->re.db, rec);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re db: add refused; kind: %u32; addr: 0x%X64; err: %u32", rec->kind, rec->addr, ret);

        toast_push(ctx, CROSSTALK_CORE_RE_REC_FAIL_TEXT);

        return;
    }

    if (ctx->console_exec_is == LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_RE_REC_OK_TEXT); }
}

static void crosstalk_re_rec_drop(crosstalk_ctx_t *ctx, uint32_t kind, uint64_t addr, const char *owner, const char *text)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    ret = crosstalk_re_db_rec_remove(&ctx->re.db, kind, addr, owner, text);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_REC_NONE_TEXT); return; }

    toast_push(ctx, CROSSTALK_CORE_RE_REC_GONE_TEXT);
}

static void crosstalk_re_rec_verb(crosstalk_ctx_t *ctx, uint32_t kind, const char *rest)
{
    crosstalk_re_db_rec_t rec = LDG_STRUCT_ZERO_INIT;
    char word[CROSSTALK_CORE_RE_WORD_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || rest == 0x0)) { return; }

    rec.kind = kind;
    rec.by = (uint32_t)CROSSTALK_RE_DB_BY_HAND;

    if (kind == (uint32_t)CROSSTALK_RE_DB_KIND_STRUCT)
    {
        if (crosstalk_re_word_next(rest, &at, rec.text, (uint32_t)sizeof(rec.text), 0x0) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; }

        if (crosstalk_re_word_next(rest, &at, word, (uint32_t)sizeof(word), 0x0) != LDG_ERR_AOK || crosstalk_re_num_parse(word, &rec.size) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; }
    }
    else if (kind == (uint32_t)CROSSTALK_RE_DB_KIND_FIELD)
    {
        if (crosstalk_re_word_next(rest, &at, rec.owner, (uint32_t)sizeof(rec.owner), 0x0) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; }

        if (crosstalk_re_word_next(rest, &at, word, (uint32_t)sizeof(word), 0x0) != LDG_ERR_AOK || crosstalk_re_num_parse(word, &rec.addr) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; }

        if (crosstalk_re_word_next(rest, &at, word, (uint32_t)sizeof(word), 0x0) != LDG_ERR_AOK || crosstalk_re_num_parse(word, &rec.size) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; }

        if (crosstalk_re_word_next(rest, &at, rec.text, (uint32_t)sizeof(rec.text), 0x0) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; }
    }
    else if (kind == (uint32_t)CROSSTALK_RE_DB_KIND_CC)
    {
        if (crosstalk_re_word_next(rest, &at, word, (uint32_t)sizeof(word), 0x0) != LDG_ERR_AOK || crosstalk_re_num_parse(word, &rec.addr) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_BUILD_NO_STEPS_TEXT); return; }

        if (crosstalk_re_word_next(rest, &at, rec.text, (uint32_t)sizeof(rec.text), 0x0) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_BUILD_NO_STEPS_TEXT); return; }

        ret = crosstalk_re_cc_step_set(&ctx->re.cc, (uint32_t)rec.addr, rec.text);
        if (ret != LDG_ERR_AOK)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re cc: step refused; idx: %u64; err: %u32", rec.addr, ret);

            toast_push(ctx, CROSSTALK_CORE_RE_REC_FAIL_TEXT);

            return;
        }
    }
    else
    {
        if (crosstalk_re_word_next(rest, &at, word, (uint32_t)sizeof(word), 0x0) != LDG_ERR_AOK || crosstalk_re_addr_parse(ctx, word, &rec.addr) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; }

        if (kind == (uint32_t)CROSSTALK_RE_DB_KIND_FUNC || kind == (uint32_t)CROSSTALK_RE_DB_KIND_VERDICT)
        {
            if (crosstalk_re_word_next(rest, &at, word, (uint32_t)sizeof(word), 0x0) != LDG_ERR_AOK || crosstalk_re_num_parse(word, &rec.size) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; }

            if (kind == (uint32_t)CROSSTALK_RE_DB_KIND_VERDICT && crosstalk_re_word_next(rest, &at, rec.text, (uint32_t)sizeof(rec.text), 0x0) != LDG_ERR_AOK) { rec.text[0] = LDG_STR_TERM; }
        }
        else if (crosstalk_re_word_next(rest, &at, rec.text, (uint32_t)sizeof(rec.text), 0x0) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; }

        if (kind == (uint32_t)CROSSTALK_RE_DB_KIND_PATCH)
        {
            uint8_t bytes[CROSSTALK_CORE_RE_PATCH_BYTES_MAX] = LDG_ARR_ZERO_INIT;
            uint64_t len = 0;

            if (ldg_str_len_get(rec.text, (uint64_t)sizeof(rec.text), &len) != LDG_ERR_AOK || len == 0 || (len % 0x2) != 0 || len / 0x2 > (uint64_t)CROSSTALK_CORE_RE_PATCH_BYTES_MAX) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; }

            if (ldg_hex_to_bytes(rec.text, bytes, len / 0x2) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; }

            ret = crosstalk_re_img_wr(&ctx->re.img, rec.addr, bytes, len / 0x2);
            if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_ADDR_TEXT); return; }

            rec.size = len / 0x2;
        }
    }

    if (crosstalk_re_word_next(rest, &at, word, (uint32_t)sizeof(word), 0x0) == LDG_ERR_AOK) { if (crosstalk_re_db_by_parse(word, &rec.by) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; } }

    crosstalk_re_rec_push(ctx, &rec);
}

static void crosstalk_re_unrec_verb(crosstalk_ctx_t *ctx, uint32_t kind, const char *rest)
{
    char word[CROSSTALK_CORE_RE_WORD_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t addr = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || rest == 0x0)) { return; }

    if (crosstalk_re_word_next(rest, &at, word, (uint32_t)sizeof(word), 0x0) != LDG_ERR_AOK || crosstalk_re_addr_parse(ctx, word, &addr) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; }

    crosstalk_re_rec_drop(ctx, kind, addr, "", "");
}

static void crosstalk_re_img_pin(crosstalk_ctx_t *ctx, const char *rest)
{
    char word[CROSSTALK_CORE_RE_WORD_CAP] = LDG_ARR_ZERO_INIT;
    char leaf[CROSSTALK_RE_DB_NAME_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t hash = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || rest == 0x0)) { return; }

    if (crosstalk_re_word_next(rest, &at, word, (uint32_t)sizeof(word), 0x0) != LDG_ERR_AOK || crosstalk_re_num_parse(word, &hash) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; }

    if (crosstalk_re_word_next(rest, &at, leaf, (uint32_t)sizeof(leaf), 0x0) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; }

    if (crosstalk_re_img_ld_is(&ctx->re.img) == LDG_TRUTH_TRUE && (uint32_t)hash != ctx->re.img.hash)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re db: pinned to a different artifact; db: 0x%08X32 %.63s; open: 0x%08X32 %.127s", (uint32_t)hash, leaf, ctx->re.img.hash, ctx->re.img.path);

        toast_push(ctx, CROSSTALK_CORE_RE_IMG_MISMATCH_TEXT);
    }

    if (crosstalk_re_db_img_set(&ctx->re.db, (uint32_t)hash, leaf) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_REC_FAIL_TEXT); }
}

static void crosstalk_re_import(crosstalk_ctx_t *ctx)
{
    uint32_t idx = 0;
    uint32_t added = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (crosstalk_re_img_ld_is(&ctx->re.img) != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_RE_NO_IMG_TEXT); return; }

    if (ctx->re.img.syms_ok != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_IDENT_NONE_TEXT); return; }

    for (idx = 0; idx < ctx->re.img.elf.sym_cunt; idx = idx + 0x1)
    {
        const crosstalk_hdl_elf_sym_t *sym = &ctx->re.img.elf.sym[idx];
        crosstalk_re_db_rec_t rec = LDG_STRUCT_ZERO_INIT;

        if (sym->kind == (uint8_t)CROSSTALK_HDL_ELF_KIND_NONE || sym->name[0] == LDG_STR_TERM) { continue; }

        rec.kind = (uint32_t)CROSSTALK_RE_DB_KIND_NAME;
        rec.by = (uint32_t)CROSSTALK_RE_DB_BY_ELF;
        rec.addr = ctx->re.img.base + sym->addr;

        if (ldg_str_rbrcpy(rec.text, sym->name, (uint64_t)sizeof(rec.text)) != LDG_ERR_AOK) { continue; }

        if (crosstalk_re_db_rec_add(&ctx->re.db, &rec) != LDG_ERR_AOK) { continue; }

        added = added + 0x1;

        if (sym->kind != (uint8_t)CROSSTALK_HDL_ELF_KIND_FUNC || sym->size == 0) { continue; }

        rec.kind = (uint32_t)CROSSTALK_RE_DB_KIND_FUNC;
        rec.size = sym->size;
        rec.text[0] = LDG_STR_TERM;

        if (crosstalk_re_db_rec_add(&ctx->re.db, &rec) == LDG_ERR_AOK) { added = added + 0x1; }
    }

    {
        char toast[CROSSTALK_CORE_EMU_TOAST_CAP] = LDG_ARR_ZERO_INIT;

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "re db: import; syms: %u32; records added: %u32", ctx->re.img.elf.sym_cunt, added);

        if (snprintf(toast, sizeof(toast), CROSSTALK_CORE_RE_IMPORT_FMT, added) > 0) { toast_push(ctx, toast); }
    }
}

static void crosstalk_re_save(crosstalk_ctx_t *ctx, const char *rest)
{
    char path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || rest == 0x0)) { return; }

    if (crosstalk_re_word_next(rest, &at, path, (uint32_t)sizeof(path), 0x0) != LDG_ERR_AOK)
    {
        if (ctx->re.db.path[0] == LDG_STR_TERM) { toast_push(ctx, CROSSTALK_CORE_RE_NO_IMG_TEXT); return; }

        if (ldg_str_rbrcpy(path, ctx->re.db.path, (uint64_t)sizeof(path)) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_SAVE_FAIL_TEXT); return; }
    }

    if (crosstalk_re_db_st(&ctx->re.db, path) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_SAVE_FAIL_TEXT); return; }

    toast_push(ctx, CROSSTALK_CORE_RE_SAVE_OK_TEXT);
}

static void crosstalk_re_assert(crosstalk_ctx_t *ctx, const char *rest)
{
    char path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char line[CROSSTALK_CORE_RE_ASSERT_LINE_CAP] = LDG_ARR_ZERO_INIT;
    ldg_io_file_t *fh = 0x0;
    uint64_t wrote = 0;
    uint32_t idx = 0;
    uint32_t at = 0;
    uint32_t cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || rest == 0x0)) { return; }

    if (crosstalk_re_word_next(rest, &at, path, (uint32_t)sizeof(path), 0x0) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; }

    ret = ldg_io_file_open(path, LDG_IO_WR_ONLY | LDG_IO_CREATE | LDG_IO_TRUNCATE, (uint32_t)CROSSTALK_CORE_RE_ASSERT_MODE, &fh);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_ASSERT_FAIL_TEXT); return; }

    ret = ldg_io_file_wr(fh, "#ifndef CROSSTALK_RE_ASSERT_H\n#define CROSSTALK_RE_ASSERT_H\n\n#include <stddef.h>\n\n", sizeof("#ifndef CROSSTALK_RE_ASSERT_H\n#define CROSSTALK_RE_ASSERT_H\n\n#include <stddef.h>\n\n") - LDG_STR_TERM_SIZE, &wrote);

    for (idx = 0; ret == LDG_ERR_AOK && idx < crosstalk_re_db_rec_cunt_get(&ctx->re.db); idx = idx + 0x1)
    {
        const crosstalk_re_db_rec_t *rec = 0x0;
        int32_t len = 0;

        if (crosstalk_re_db_rec_get(&ctx->re.db, idx, &rec) != LDG_ERR_AOK || rec == 0x0) { continue; }

        if (rec->kind == (uint32_t)CROSSTALK_RE_DB_KIND_STRUCT) { len = snprintf(line, sizeof(line), "_Static_assert(sizeof(struct %.63s) == 0x%" PRIX64 ", \"%.63s size\");\n", rec->text, rec->size, rec->text); }
        else if (rec->kind == (uint32_t)CROSSTALK_RE_DB_KIND_FIELD) { len = snprintf(line, sizeof(line), "_Static_assert(offsetof(struct %.63s, %.63s) == 0x%" PRIX64 ", \"%.63s.%.63s offset\");\n_Static_assert(sizeof(((struct %.63s *)0x0)->%.63s) == 0x%" PRIX64 ", \"%.63s.%.63s size\");\n", rec->owner, rec->text, rec->addr, rec->owner, rec->text, rec->owner, rec->text, rec->size, rec->owner, rec->text); }
        else { continue; }

        if (len < 0 || (uint32_t)len >= sizeof(line)) { ret = LDG_ERR_STR_TRUNC; break; }

        ret = ldg_io_file_wr(fh, line, (uint64_t)len, &wrote);
        cunt = cunt + 0x1;
    }

    if (ret == LDG_ERR_AOK) { ret = ldg_io_file_wr(fh, "\n#endif\n", sizeof("\n#endif\n") - LDG_STR_TERM_SIZE, &wrote); }

    if (ldg_io_file_close(fh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re assert close failed"); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "re assert: path: %.127s; layouts: %u32; err: %u32", path, cunt, ret);

    toast_push(ctx, ret == LDG_ERR_AOK ? CROSSTALK_CORE_RE_ASSERT_OK_TEXT : CROSSTALK_CORE_RE_ASSERT_FAIL_TEXT);
}

static void crosstalk_re_build(crosstalk_ctx_t *ctx, const char *rest)
{
    char src[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char out[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || rest == 0x0)) { return; }

    if (crosstalk_re_word_next(rest, &at, src, (uint32_t)sizeof(src), 0x0) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; }

    if (crosstalk_re_word_next(rest, &at, out, (uint32_t)sizeof(out), 0x0) != LDG_ERR_AOK) { out[0] = LDG_STR_TERM; }

    ret = crosstalk_re_cc_start(&ctx->re.cc, src, out);
    if (ret == CROSSTALK_RE_CC_ERR_BUSY) { toast_push(ctx, CROSSTALK_CORE_RE_BUILD_BUSY_TEXT); return; }

    if (ret == CROSSTALK_RE_CC_ERR_STEP) { toast_push(ctx, CROSSTALK_CORE_RE_BUILD_NO_STEPS_TEXT); return; }

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re cc: start refused; src: %.127s; err: %u32", src, ret);

        toast_push(ctx, CROSSTALK_CORE_RE_BUILD_FAIL_TEXT);

        return;
    }

    toast_push(ctx, CROSSTALK_CORE_RE_BUILD_START_TEXT);
}

static void crosstalk_re_ls(crosstalk_ctx_t *ctx, const char *rest)
{
    char word[CROSSTALK_CORE_RE_WORD_CAP] = LDG_ARR_ZERO_INIT;
    char line[CROSSTALK_RE_DB_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t kind = (uint32_t)CROSSTALK_RE_DB_KIND_NONE;
    uint32_t idx = 0;
    uint32_t at = 0;
    uint32_t shown = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || rest == 0x0)) { return; }

    if (crosstalk_re_word_next(rest, &at, word, (uint32_t)sizeof(word), 0x0) == LDG_ERR_AOK) { if (crosstalk_re_db_kind_parse(word, &kind) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; } }

    for (idx = 0; idx < crosstalk_re_db_rec_cunt_get(&ctx->re.db); idx = idx + 0x1)
    {
        const crosstalk_re_db_rec_t *rec = 0x0;

        if (crosstalk_re_db_rec_get(&ctx->re.db, idx, &rec) != LDG_ERR_AOK || rec == 0x0) { continue; }

        if (kind != (uint32_t)CROSSTALK_RE_DB_KIND_NONE && rec->kind != kind) { continue; }

        if (crosstalk_re_db_line_fmt(rec, line, (uint32_t)sizeof(line)) != LDG_ERR_AOK) { continue; }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "re ls: %.383s", line);

        shown = shown + 0x1;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "re ls: records: %u32; shown: %u32", crosstalk_re_db_rec_cunt_get(&ctx->re.db), shown);
}

static uint32_t crosstalk_re_cand_syms_ld(crosstalk_ctx_t *ctx)
{
    char path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    const crosstalk_re_diff_side_t *cand = 0x0;
    uint32_t at = 0;
    uint32_t dot_at = UINT32_MAX;
    uint32_t sep_at = UINT32_MAX;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_re_diff_side_get(&ctx->re.diff, (uint32_t)CROSSTALK_RE_DIFF_SIDE_CAND, &cand);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (cand->set != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    while (cand->path[at] != LDG_STR_TERM && at + LDG_STR_TERM_SIZE < (uint32_t)sizeof(path))
    {
        if (cand->path[at] == CROSSTALK_RE_IMG_SYMS_DOT) { dot_at = at; }

        if (cand->path[at] == CROSSTALK_RE_IMG_SYMS_SEP_POSIX || cand->path[at] == CROSSTALK_RE_IMG_SYMS_SEP_WIN) { sep_at = at; }

        path[at] = cand->path[at];
        at = at + 0x1;
    }

    path[at] = LDG_STR_TERM;

    if (dot_at == UINT32_MAX || (sep_at != UINT32_MAX && dot_at < sep_at)) { return LDG_ERR_NOT_FOUND; }

    if ((uint64_t)dot_at + sizeof(CROSSTALK_RE_IMG_SYMS_EXT) > (uint64_t)sizeof(path)) { return LDG_ERR_STR_TRUNC; }

    memcpy(path + dot_at, CROSSTALK_RE_IMG_SYMS_EXT, sizeof(CROSSTALK_RE_IMG_SYMS_EXT));

    if (ldg_io_path_exists_is(path) != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_FOUND; }

    ret = crosstalk_hdl_elf_ld(&ctx->re.cand_elf, path);
    if (ret != LDG_ERR_AOK) { return ret; }

    return crosstalk_hdl_elf_base_set(&ctx->re.cand_elf, cand->word_off * (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_ELEM_BYTES);
}

static void crosstalk_re_func_arm(crosstalk_ctx_t *ctx, const char *rest)
{
    char name[CROSSTALK_RE_DIFF_NAME_CAP] = LDG_ARR_ZERO_INIT;
    char word[CROSSTALK_CORE_RE_WORD_CAP] = LDG_ARR_ZERO_INIT;
    const crosstalk_re_diff_side_t *cand = 0x0;
    uint64_t entry_ref = 0;
    uint64_t entry_cand = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || rest == 0x0)) { return; }

    if (crosstalk_re_img_ld_is(&ctx->re.img) != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_RE_NO_IMG_TEXT); return; }

    if (ctx->re.img.isa != (uint32_t)CROSSTALK_RE_IMG_ISA_DANGCSA) { toast_push(ctx, CROSSTALK_CORE_RE_NO_ORACLE_TEXT); return; }

    if (crosstalk_re_word_next(rest, &at, name, (uint32_t)sizeof(name), 0x0) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; }

    if (crosstalk_re_diff_side_get(&ctx->re.diff, (uint32_t)CROSSTALK_RE_DIFF_SIDE_CAND, &cand) != LDG_ERR_AOK) { return; }

    if (cand->set != LDG_TRUTH_TRUE && ctx->re.cc.out[0] != LDG_STR_TERM && ldg_io_path_exists_is(ctx->re.cc.out) == LDG_TRUTH_TRUE)
    {
        const crosstalk_re_diff_side_t *ref = 0x0;

        if (crosstalk_re_diff_side_get(&ctx->re.diff, (uint32_t)CROSSTALK_RE_DIFF_SIDE_REF, &ref) == LDG_ERR_AOK && crosstalk_re_diff_side_set(&ctx->re.diff, (uint32_t)CROSSTALK_RE_DIFF_SIDE_CAND, ctx->re.cc.out, ref->sig, ref->word_off) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re diff cand side set failed"); }
    }

    if (cand->set != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_RE_DIFF_NOT_READY_TEXT); return; }

    if (crosstalk_re_addr_parse(ctx, name, &entry_ref) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_FUNC_NONE_TEXT); return; }

    ret = crosstalk_re_cand_syms_ld(ctx);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re diff: candidate symbols unavailable; path: %.127s; err: %u32", cand->path, ret);

        toast_push(ctx, CROSSTALK_CORE_RE_CAND_SYMS_NONE_TEXT);

        return;
    }

    if (crosstalk_hdl_elf_addr_get(&ctx->re.cand_elf, name, &entry_cand) != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re diff: function absent from the candidate; name: %.63s", name);

        toast_push(ctx, CROSSTALK_CORE_RE_FUNC_NONE_TEXT);

        return;
    }

    if (crosstalk_re_word_next(rest, &at, word, (uint32_t)sizeof(word), 0x0) == LDG_ERR_AOK)
    {
        uint64_t sp = 0;

        if (crosstalk_re_num_parse(word, &sp) != LDG_ERR_AOK || crosstalk_re_diff_sp_set(&ctx->re.diff, sp) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; }
    }

    ret = crosstalk_re_diff_func_set(&ctx->re.diff, name, entry_ref, entry_cand);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_REC_FAIL_TEXT); return; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "re diff: function armed; name: %.63s; ref entry: 0x%X64; cand entry: 0x%X64; args: %u32; sp: 0x%X64", name, entry_ref, entry_cand, ctx->re.diff.arg_cunt, ctx->re.diff.sp);

    toast_push(ctx, CROSSTALK_CORE_RE_FUNC_ARMED_TEXT);
}

static void crosstalk_re_diff_verb(crosstalk_ctx_t *ctx, const char *rest)
{
    char word[CROSSTALK_CORE_RE_WORD_CAP] = LDG_ARR_ZERO_INIT;
    char path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char sig[CROSSTALK_RE_DIFF_SIG_CAP] = LDG_ARR_ZERO_INIT;
    const char *verdict = 0x0;
    uint64_t num = 0;
    uint32_t side = (uint32_t)CROSSTALK_RE_DIFF_SIDE_CUNT;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || rest == 0x0)) { return; }

    if (crosstalk_re_word_next(rest, &at, word, (uint32_t)sizeof(word), 0x0) != LDG_ERR_AOK)
    {
        if (crosstalk_re_diff_verdict_name_get(ctx->re.diff.verdict, &verdict) != LDG_ERR_AOK) { verdict = CROSSTALK_CORE_EMU_OSC_NONE_TEXT; }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "re diff: verdict: %s; cycles: %u64; ref: %.127s; cand: %.127s; masks: %u32", verdict, ctx->re.diff.cycles, ctx->re.diff.side[CROSSTALK_RE_DIFF_SIDE_REF].path, ctx->re.diff.side[CROSSTALK_RE_DIFF_SIDE_CAND].path, ctx->re.diff.mask_cunt);

        return;
    }

    if (strcmp(word, "run") == 0)
    {
        if (crosstalk_re_img_ld_is(&ctx->re.img) == LDG_TRUTH_TRUE && ctx->re.img.isa != (uint32_t)CROSSTALK_RE_IMG_ISA_DANGCSA) { toast_push(ctx, CROSSTALK_CORE_RE_NO_ORACLE_TEXT); return; }

        if (crosstalk_re_diff_ready_is(&ctx->re.diff) != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_RE_DIFF_NOT_READY_TEXT); return; }

        ctx->re_diff_pending = (uint8_t)CROSSTALK_CORE_RE_DIFF_PENDING_FRAMES;

        toast_push(ctx, CROSSTALK_CORE_RE_DIFF_ARMED_TEXT);

        return;
    }

    if (strcmp(word, "cycles") == 0)
    {
        if (crosstalk_re_word_next(rest, &at, word, (uint32_t)sizeof(word), 0x0) != LDG_ERR_AOK || crosstalk_re_num_parse(word, &num) != LDG_ERR_AOK || crosstalk_re_diff_cycles_set(&ctx->re.diff, num) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; }

        toast_push(ctx, CROSSTALK_CORE_RE_REC_OK_TEXT);

        return;
    }

    if (strcmp(word, "mask") == 0)
    {
        if (crosstalk_re_word_next(rest, &at, sig, (uint32_t)sizeof(sig), 0x0) != LDG_ERR_AOK || crosstalk_re_diff_mask_add(&ctx->re.diff, sig) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; }

        toast_push(ctx, CROSSTALK_CORE_RE_REC_OK_TEXT);

        return;
    }

    if (strcmp(word, "unmask") == 0)
    {
        if (crosstalk_re_diff_mask_clear(&ctx->re.diff) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re diff mask clear failed"); }

        toast_push(ctx, CROSSTALK_CORE_RE_REC_GONE_TEXT);

        return;
    }

    if (strcmp(word, "func") == 0) { crosstalk_re_func_arm(ctx, &rest[at]); return; }

    if (strcmp(word, "whole") == 0)
    {
        if (crosstalk_re_diff_func_clear(&ctx->re.diff) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re diff func clear failed"); }

        toast_push(ctx, CROSSTALK_CORE_RE_REC_GONE_TEXT);

        return;
    }

    if (strcmp(word, "arg") == 0)
    {
        uint64_t idx = 0;

        if (crosstalk_re_word_next(rest, &at, word, (uint32_t)sizeof(word), 0x0) != LDG_ERR_AOK || crosstalk_re_num_parse(word, &idx) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; }

        if (crosstalk_re_word_next(rest, &at, word, (uint32_t)sizeof(word), 0x0) != LDG_ERR_AOK || crosstalk_re_addr_parse(ctx, word, &num) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; }

        if (crosstalk_re_diff_arg_set(&ctx->re.diff, (uint32_t)idx, num) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; }

        toast_push(ctx, CROSSTALK_CORE_RE_REC_OK_TEXT);

        return;
    }

    if (strcmp(word, "sp") == 0)
    {
        if (crosstalk_re_word_next(rest, &at, word, (uint32_t)sizeof(word), 0x0) != LDG_ERR_AOK || crosstalk_re_num_parse(word, &num) != LDG_ERR_AOK || crosstalk_re_diff_sp_set(&ctx->re.diff, num) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; }

        toast_push(ctx, CROSSTALK_CORE_RE_REC_OK_TEXT);

        return;
    }

    if (strcmp(word, "ref") == 0) { side = (uint32_t)CROSSTALK_RE_DIFF_SIDE_REF; }
    else if (strcmp(word, "cand") == 0) { side = (uint32_t)CROSSTALK_RE_DIFF_SIDE_CAND; }
    else { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; }

    if (crosstalk_re_word_next(rest, &at, path, (uint32_t)sizeof(path), 0x0) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_DIFF_NOT_READY_TEXT); return; }

    if (ldg_io_path_exists_is(path) != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_SYMS_NO_FILE_TEXT); return; }

    if (crosstalk_re_word_next(rest, &at, sig, (uint32_t)sizeof(sig), 0x0) != LDG_ERR_AOK) { sig[0] = LDG_STR_TERM; }

    if (crosstalk_re_word_next(rest, &at, word, (uint32_t)sizeof(word), 0x0) == LDG_ERR_AOK) { if (crosstalk_re_num_parse(word, &num) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT); return; } }

    ret = crosstalk_re_diff_side_set(&ctx->re.diff, side, path, sig, num);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_RE_REC_FAIL_TEXT); return; }

    toast_push(ctx, CROSSTALK_CORE_RE_REC_OK_TEXT);
}

static void crosstalk_re_progress_push(crosstalk_ctx_t *ctx)
{
    uint32_t idx = 0;
    uint32_t total = 0;
    uint32_t named = 0;
    uint32_t ported = 0;
    uint32_t verified = 0;
    uint32_t proj = (uint32_t)UINT32_MAX;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (crosstalk_reng_proj_hash_find(&ctx->reng, ctx->re.db.img_hash, &proj) != LDG_ERR_AOK) { return; }

    for (idx = 0; idx < crosstalk_re_db_rec_cunt_get(&ctx->re.db); idx = idx + 0x1)
    {
        const crosstalk_re_db_rec_t *rec = 0x0;
        uint32_t hit = 0;

        if (crosstalk_re_db_rec_get(&ctx->re.db, idx, &rec) != LDG_ERR_AOK || rec == 0x0) { continue; }

        if (rec->kind != (uint32_t)CROSSTALK_RE_DB_KIND_FUNC) { continue; }

        total = total + 0x1;

        if (crosstalk_re_db_rec_find(&ctx->re.db, (uint32_t)CROSSTALK_RE_DB_KIND_NAME, rec->addr, "", "", &hit) == LDG_ERR_AOK) { named = named + 0x1; }

        if (crosstalk_re_db_rec_find(&ctx->re.db, (uint32_t)CROSSTALK_RE_DB_KIND_SRC, rec->addr, "", "", &hit) == LDG_ERR_AOK) { ported = ported + 0x1; }

        if (crosstalk_re_db_rec_find(&ctx->re.db, (uint32_t)CROSSTALK_RE_DB_KIND_VERDICT, rec->addr, "", "", &hit) == LDG_ERR_AOK)
        {
            const crosstalk_re_db_rec_t *verdict = 0x0;

            if (crosstalk_re_db_rec_get(&ctx->re.db, hit, &verdict) == LDG_ERR_AOK && verdict != 0x0 && verdict->size == (uint64_t)CROSSTALK_RE_DIFF_VERDICT_MATCH) { verified = verified + 0x1; }
        }
    }

    if (crosstalk_reng_sample_push(&ctx->reng, proj, total, named, ported, verified) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "reng sample push failed"); }
}

static void crosstalk_verb_progress(crosstalk_ctx_t *ctx, const char *arg)
{
    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    if (crosstalk_reng_progress_open_set(&ctx->reng_progress, crosstalk_reng_progress_open_is(&ctx->reng_progress) == LDG_TRUTH_TRUE ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "progress window toggle failed"); }
}

static void crosstalk_verb_tasks(crosstalk_ctx_t *ctx, const char *arg)
{
    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    if (crosstalk_reng_tasks_open_set(&ctx->reng_tasks, crosstalk_reng_tasks_open_is(&ctx->reng_tasks) == LDG_TRUTH_TRUE ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "tasks window toggle failed"); }
}

static void crosstalk_verb_re(crosstalk_ctx_t *ctx, const char *arg)
{
    char sub[CROSSTALK_CORE_RE_WORD_CAP] = LDG_ARR_ZERO_INIT;
    const char *rest = 0x0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    if (crosstalk_re_word_next(arg, &at, sub, (uint32_t)sizeof(sub), 0x0) != LDG_ERR_AOK) { crosstalk_re_state_log(ctx); return; }

    rest = &arg[at];

    if (strcmp(sub, "open") == 0) { crosstalk_re_open(ctx, rest); return; }

    if (strcmp(sub, "close") == 0) { crosstalk_re_close(ctx); return; }

    if (strcmp(sub, "seg") == 0) { crosstalk_re_seg_log(ctx); return; }

    if (strcmp(sub, "rd") == 0) { crosstalk_re_rd(ctx, rest); return; }

    if (strcmp(sub, "dis") == 0) { crosstalk_re_dis(ctx, rest); return; }

    if (strcmp(sub, "name") == 0) { crosstalk_re_rec_verb(ctx, (uint32_t)CROSSTALK_RE_DB_KIND_NAME, rest); return; }

    if (strcmp(sub, "unname") == 0) { crosstalk_re_unrec_verb(ctx, (uint32_t)CROSSTALK_RE_DB_KIND_NAME, rest); return; }

    if (strcmp(sub, "note") == 0) { crosstalk_re_rec_verb(ctx, (uint32_t)CROSSTALK_RE_DB_KIND_NOTE, rest); return; }

    if (strcmp(sub, "unnote") == 0) { crosstalk_re_unrec_verb(ctx, (uint32_t)CROSSTALK_RE_DB_KIND_NOTE, rest); return; }

    if (strcmp(sub, "func") == 0) { crosstalk_re_rec_verb(ctx, (uint32_t)CROSSTALK_RE_DB_KIND_FUNC, rest); return; }

    if (strcmp(sub, "unfunc") == 0) { crosstalk_re_unrec_verb(ctx, (uint32_t)CROSSTALK_RE_DB_KIND_FUNC, rest); return; }

    if (strcmp(sub, "struct") == 0) { crosstalk_re_rec_verb(ctx, (uint32_t)CROSSTALK_RE_DB_KIND_STRUCT, rest); return; }

    if (strcmp(sub, "field") == 0) { crosstalk_re_rec_verb(ctx, (uint32_t)CROSSTALK_RE_DB_KIND_FIELD, rest); return; }

    if (strcmp(sub, "patch") == 0) { crosstalk_re_rec_verb(ctx, (uint32_t)CROSSTALK_RE_DB_KIND_PATCH, rest); return; }

    if (strcmp(sub, "unpatch") == 0) { crosstalk_re_unrec_verb(ctx, (uint32_t)CROSSTALK_RE_DB_KIND_PATCH, rest); return; }

    if (strcmp(sub, "cc") == 0) { crosstalk_re_rec_verb(ctx, (uint32_t)CROSSTALK_RE_DB_KIND_CC, rest); return; }

    if (strcmp(sub, "src") == 0) { crosstalk_re_rec_verb(ctx, (uint32_t)CROSSTALK_RE_DB_KIND_SRC, rest); return; }

    if (strcmp(sub, "unsrc") == 0) { crosstalk_re_unrec_verb(ctx, (uint32_t)CROSSTALK_RE_DB_KIND_SRC, rest); return; }

    if (strcmp(sub, "verdict") == 0) { crosstalk_re_rec_verb(ctx, (uint32_t)CROSSTALK_RE_DB_KIND_VERDICT, rest); return; }

    if (strcmp(sub, "img") == 0) { crosstalk_re_img_pin(ctx, rest); return; }

    if (strcmp(sub, "import") == 0) { crosstalk_re_import(ctx); return; }

    if (strcmp(sub, "save") == 0) { crosstalk_re_save(ctx, rest); return; }

    if (strcmp(sub, "undo") == 0) { toast_push(ctx, crosstalk_re_db_undo(&ctx->re.db) == LDG_ERR_AOK ? CROSSTALK_CORE_RE_UNDO_OK_TEXT : CROSSTALK_CORE_RE_UNDO_NONE_TEXT); return; }

    if (strcmp(sub, "assert") == 0) { crosstalk_re_assert(ctx, rest); return; }

    if (strcmp(sub, "build") == 0) { crosstalk_re_build(ctx, rest); return; }

    if (strcmp(sub, "diff") == 0) { crosstalk_re_diff_verb(ctx, rest); return; }

    if (strcmp(sub, "ls") == 0) { crosstalk_re_ls(ctx, rest); return; }

    toast_push(ctx, CROSSTALK_CORE_RE_USAGE_TEXT);
}

static uint32_t crosstalk_win_nav_reg(crosstalk_ctx_t *ctx)
{
    crosstalk_core_win_nav_entry_t order[CROSSTALK_CORE_WIN_NAV_CUNT] = {
        { (uint64_t)CROSSTALK_EDA_UI_TERM_WINDOW_ID, CROSSTALK_EDA_UI_TERM_WINDOW_TITLE },
        { (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_WINDOW_ID, CROSSTALK_EDA_UI_MEMVIEW_TITLE },
        { (uint64_t)CROSSTALK_EDA_UI_CALC_WINDOW_ID, CROSSTALK_EDA_UI_CALC_WINDOW_TITLE },
        { (uint64_t)CROSSTALK_EDA_UI_REGVIEW_WINDOW_ID, CROSSTALK_EDA_UI_REGVIEW_WINDOW_TITLE },
        { (uint64_t)CROSSTALK_EDA_UI_STACKVIEW_WINDOW_ID, CROSSTALK_EDA_UI_STACKVIEW_WINDOW_TITLE },
        { (uint64_t)CROSSTALK_EDA_UI_PROGVIEW_WINDOW_ID, CROSSTALK_EDA_UI_PROGVIEW_WINDOW_TITLE },
        { (uint64_t)CROSSTALK_EDA_UI_PROFILE_WINDOW_ID, CROSSTALK_EDA_UI_PROFILE_TITLE },
        { (uint64_t)CROSSTALK_EDA_UI_DISPLAY_CRT_WINDOW_ID, "Monitor" },
        { (uint64_t)CROSSTALK_EDA_UI_ANALYZER_WINDOW_ID, CROSSTALK_EDA_UI_ANALYZER_WINDOW_TITLE },
        { (uint64_t)CROSSTALK_EDA_UI_SCOPE_WINDOW_ID, CROSSTALK_EDA_UI_SCOPE_WINDOW_TITLE },
        { (uint64_t)CROSSTALK_EDA_UI_FEED_WINDOW_ID, CROSSTALK_EDA_UI_FEED_WINDOW_TITLE },
        { (uint64_t)CROSSTALK_EDA_UI_INDICATOR_WINDOW_ID, CROSSTALK_EDA_UI_INDICATOR_WINDOW_TITLE },
        { (uint64_t)CROSSTALK_EDA_UI_PIPEVIEW_WINDOW_ID, CROSSTALK_EDA_UI_PIPEVIEW_WINDOW_TITLE },
        { (uint64_t)CROSSTALK_EDA_UI_BRAINOMATIC_WINDOW_ID, CROSSTALK_EDA_UI_BRAINOMATIC_WINDOW_TITLE },
        { (uint64_t)CROSSTALK_RENG_PROGRESS_ID, CROSSTALK_RENG_PROGRESS_TITLE },
        { (uint64_t)CROSSTALK_RENG_TASKS_ID, CROSSTALK_RENG_TASKS_TITLE },
        { (uint64_t)CROSSTALK_EDA_UI_WSHOP_BROWSER_WINDOW_ID, "Workshop" },
        { (uint64_t)CROSSTALK_EDA_UI_WSHOP_PUBLISH_WINDOW_ID, "Publish" },
        { (uint64_t)CROSSTALK_GAME_CHAT_WINDOW_ID, "Chat" },
        { (uint64_t)CROSSTALK_GAME_TOTALS_WINDOW_ID, "City totals" },
        { (uint64_t)CROSSTALK_GAME_HUB_WINDOW_ID, "Friends" },
        { (uint64_t)CROSSTALK_UI_GUIDE_WINDOW_ID, CROSSTALK_UI_GUIDE_WINDOW_TITLE }
    };
    crosstalk_ui_window_ctx_t *nav = 0x0;
    uint32_t slot = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    nav = &ctx->win_nav;

    ret = crosstalk_core_task_init(&ctx->task);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_ui_window_init(nav, &ctx->task);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_eda_ui_term_win_reg(&ctx->eda.term, nav, ctx->eda.term_pool, &ctx->eda.pty, &slot);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->eda.term.slot = slot;

    ret = crosstalk_eda_ui_memview_win_reg(&ctx->eda.memview, nav, &slot);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_eda_ui_calc_win_reg(&ctx->eda.calc, nav, &slot); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_eda_ui_regview_win_reg(&ctx->eda.regview, nav, &slot); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_eda_ui_stackview_win_reg(&ctx->eda.stackview, nav, &slot); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_eda_ui_progview_win_reg(&ctx->eda.progview, nav, &slot); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_eda_ui_profile_win_reg(&ctx->eda.profile, nav, &slot); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_eda_ui_display_crt_win_reg(&ctx->eda.crt, nav, &ctx->eda.display, &slot); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_eda_ui_analyzer_win_reg(&ctx->eda.analyzer, nav, &slot); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_eda_ui_scope_win_reg(&ctx->eda.scope, nav, &slot); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_eda_ui_feed_win_reg(&ctx->eda.feed, nav, &slot); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_eda_ui_indicator_win_reg(&ctx->eda.indicator, nav, &ctx->eda.logic_stream, &slot); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_eda_ui_pipeview_win_reg(&ctx->eda.pipeview, nav, &slot); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_eda_ui_brainomatic_win_reg(&ctx->eda.brainomatic, nav, &slot); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_reng_progress_reg(&ctx->reng_progress, nav, &slot); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_reng_tasks_reg(&ctx->reng_tasks, nav, &slot); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_eda_ui_wshop_browser_win_reg(&ctx->eda.wshop_browser, nav, &slot); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_eda_ui_wshop_publish_win_reg(&ctx->eda.wshop_publish, nav, &slot); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_game_chat_win_reg(&ctx->chat, nav, &slot); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_game_totals_win_reg(&ctx->game.totals, nav, &slot); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_game_hub_win_reg(&ctx->hub, nav, &slot); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_guide_win_reg(&ctx->guide, nav, &slot); }

    if (ret != LDG_ERR_AOK) { return ret; }

    for (at = 0; at < (uint32_t)CROSSTALK_CORE_WIN_NAV_CUNT; at = at + 0x1) { if (crosstalk_ui_window_title_set(nav, order[at].win_id, order[at].title) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "window title number failed"); } }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "window: nav slots ready; cunt: %u32", (uint32_t)CROSSTALK_CORE_WIN_NAV_CUNT);

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_win_tab_cycle(crosstalk_ctx_t *ctx)
{
    uint32_t at = 0;
    uint32_t seen = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (crosstalk_ui_window_focused_is(&ctx->win_nav, (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_WINDOW_ID) == LDG_TRUTH_TRUE)
    {
        ctx->eda.memview.tab = ctx->eda.memview.tab == (uint8_t)CROSSTALK_EDA_UI_MEMVIEW_TAB_HEX ? (uint8_t)CROSSTALK_EDA_UI_MEMVIEW_TAB_HITS : (uint8_t)CROSSTALK_EDA_UI_MEMVIEW_TAB_HEX;

        return LDG_ERR_AOK;
    }

    if (crosstalk_ui_window_focused_is(&ctx->win_nav, (uint64_t)CROSSTALK_EDA_UI_PROFILE_WINDOW_ID) == LDG_TRUTH_TRUE)
    {
        ctx->eda.profile.tab = (uint8_t)((ctx->eda.profile.tab + 0x1) % (uint32_t)CROSSTALK_EDA_UI_PROFILE_TAB_CUNT);

        return LDG_ERR_AOK;
    }

    if (crosstalk_ui_window_focused_is(&ctx->win_nav, (uint64_t)CROSSTALK_EDA_UI_TERM_WINDOW_ID) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    for (at = 0x1; at <= (uint32_t)CROSSTALK_EDA_UI_TERM_SESH_MAX; at = at + 0x1)
    {
        seen = (ctx->eda.term.sesh_active + at) % (uint32_t)CROSSTALK_EDA_UI_TERM_SESH_MAX;

        if (ctx->eda.term.sesh[seen].src == (uint32_t)CROSSTALK_EDA_UI_TERM_SRC_NONE) { continue; }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "term: tab cycle; sesh: %u32", seen);

        return crosstalk_eda_ui_term_active_set(&ctx->eda.term, seen);
    }

    return LDG_ERR_AOK;
}

#define CROSSTALK_CORE_IRQ_CMDLINE_CODEPOINT ';'
#define CROSSTALK_CORE_IRQ_CMDLINE_CODEPOINT_SHIFTED ':'

static uint32_t crosstalk_win_nav_key_isr(crosstalk_ctx_t *ctx, const ldg_term_key_event_t *irq, uint8_t *consumed)
{
    uint64_t now_ms = 0;
    uint32_t dir = (uint32_t)UINT32_MAX;
    uint32_t phys = 0;
    uint32_t ret = 0;
    uint8_t ctrl_is = LDG_TRUTH_FALSE;
    uint8_t shift_is = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0 || irq == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    if (irq->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE) { return LDG_ERR_AOK; }

    ctrl_is = (irq->mods & (uint32_t)LDG_TERM_MOD_CTRL) != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    shift_is = (irq->mods & (uint32_t)LDG_TERM_MOD_SHIFT) != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (ctrl_is != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    now_ms = ldg_time_epoch_ms_get();
    phys = irq->base != 0 ? irq->base : irq->codepoint;

    if (shift_is == LDG_TRUTH_TRUE && (phys == (uint32_t)CROSSTALK_CORE_IRQ_CMDLINE_CODEPOINT || phys == (uint32_t)CROSSTALK_CORE_IRQ_CMDLINE_CODEPOINT_SHIFTED))
    {
        *consumed = LDG_TRUTH_TRUE;
        ctx->win_nav.kbd_pending = LDG_TRUTH_TRUE;

        if (crosstalk_ui_window_focus_set(&ctx->win_nav, (uint64_t)UINT64_MAX, now_ms) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cmdline focus release failed"); }

        if (crosstalk_core_cmdline_open_is(&ctx->cmdline) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

        return crosstalk_core_cmdline_open(&ctx->cmdline);
    }

    if (irq->key == (uint8_t)LDG_TERM_KEY_TAB)
    {
        *consumed = LDG_TRUTH_TRUE;

        if (shift_is == LDG_TRUTH_TRUE) { return crosstalk_win_tab_cycle(ctx); }

        return crosstalk_ui_window_recent_focus(&ctx->win_nav, now_ms);
    }

    if ((irq->key == (uint8_t)LDG_TERM_KEY_RHS || irq->key == (uint8_t)LDG_TERM_KEY_LHS) && crosstalk_core_cmdline_open_is(&ctx->cmdline) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (irq->key == (uint8_t)LDG_TERM_KEY_LHS) { dir = (uint32_t)CROSSTALK_UI_WINDOW_DIR_LHS; }
    else if (irq->key == (uint8_t)LDG_TERM_KEY_RHS) { dir = (uint32_t)CROSSTALK_UI_WINDOW_DIR_RHS; }
    else if (irq->key == (uint8_t)LDG_TERM_KEY_UP) { dir = (uint32_t)CROSSTALK_UI_WINDOW_DIR_UP; }
    else if (irq->key == (uint8_t)LDG_TERM_KEY_DOWN) { dir = (uint32_t)CROSSTALK_UI_WINDOW_DIR_DOWN; }

    if (dir != (uint32_t)UINT32_MAX)
    {
        *consumed = LDG_TRUTH_TRUE;

        ret = crosstalk_ui_window_focus_dir(&ctx->win_nav, dir, now_ms);
        if (ret == LDG_ERR_NOT_FOUND) { return LDG_ERR_AOK; }

        return ret;
    }

    if (irq->key != (uint8_t)LDG_TERM_KEY_NONE || irq->codepoint < (uint32_t)LDG_ASCII_DIGIT_ZERO || irq->codepoint >= (uint32_t)LDG_ASCII_DIGIT_ZERO + (uint32_t)CROSSTALK_UI_WINDOW_DIGIT_BASE) { return LDG_ERR_AOK; }

    *consumed = LDG_TRUTH_TRUE;

    return crosstalk_ui_window_digit_put(&ctx->win_nav, irq->codepoint - (uint32_t)LDG_ASCII_DIGIT_ZERO, now_ms);
}

#define CROSSTALK_CORE_RESIZE_CODEPOINT_GROW_W 'l'
#define CROSSTALK_CORE_RESIZE_CODEPOINT_SHRINK_W 'h'
#define CROSSTALK_CORE_RESIZE_CODEPOINT_GROW_H 'j'
#define CROSSTALK_CORE_RESIZE_CODEPOINT_SHRINK_H 'k'
#define CROSSTALK_CORE_RESIZE_CELLS_MIN 3.0
#define CROSSTALK_CORE_RESIZE_FRAC_FULL 1.0
#define CROSSTALK_CORE_RESIZE_SLACK_DIV 2.0
#define CROSSTALK_CORE_RESIZE_UPPER_FIRST 'A'
#define CROSSTALK_CORE_RESIZE_UPPER_LAST 'Z'

#define CROSSTALK_CORE_SHOT_CODEPOINT 'p'
#define CROSSTALK_CORE_SHOT_CODEPOINT_SHIFTED 'P'
#define CROSSTALK_CORE_SHOT_FLASH_MS 150
#define CROSSTALK_CORE_SHOT_NAME_CAP 64

#define CROSSTALK_HUB_FRAME_SIDES 2
#define CROSSTALK_HUB_CHAT_COL_CUNT 64
#define CROSSTALK_HUB_CHAT_ROW_MIN 12
#define CROSSTALK_HUB_TOGGLE_CODEPOINT 'h'
#define CROSSTALK_HUB_TOGGLE_CODEPOINT_SHIFTED 'H'

static double crosstalk_hub_frac_get(uint32_t cells, uint32_t total)
{
    double frac = (double)cells / (double)total;

    return frac > CROSSTALK_CORE_DESK_FRAC_FULL ? CROSSTALK_CORE_DESK_FRAC_FULL : frac;
}

static void crosstalk_hub_fit(crosstalk_ctx_t *ctx)
{
    uint32_t frame_span = (uint32_t)CROSSTALK_UI_WINDOW_FRAME_CELLS * (uint32_t)CROSSTALK_HUB_FRAME_SIDES;
    uint32_t content_rows = 0;
    uint32_t chat_rows = 0;
    uint32_t desk_rows = 0;
    uint32_t ret = 0;

    if (ctx->term_col_cunt == 0 || ctx->term_row_cunt <= (uint32_t)CROSSTALK_STATUSLINE_ROWS) { return; }

    ret = crosstalk_game_hub_extent_get(&ctx->hub, &content_rows);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "hub: extent failed; err: %u32", ret);
        return;
    }

    desk_rows = ctx->term_row_cunt - (uint32_t)CROSSTALK_STATUSLINE_ROWS;
    chat_rows = content_rows > (uint32_t)CROSSTALK_HUB_CHAT_ROW_MIN ? content_rows : (uint32_t)CROSSTALK_HUB_CHAT_ROW_MIN;

    ctx->hub.win.rect_w = crosstalk_hub_frac_get((uint32_t)CROSSTALK_GAME_HUB_COL_CUNT + frame_span, ctx->term_col_cunt);
    ctx->hub.win.rect_h = crosstalk_hub_frac_get(content_rows + frame_span, desk_rows);
    ctx->hub.win.rect_x = CROSSTALK_CORE_DESK_FRAC_ORIG;
    ctx->hub.win.rect_y = CROSSTALK_CORE_DESK_FRAC_FULL - ctx->hub.win.rect_h;

    ctx->chat.win.rect_x = ctx->hub.win.rect_w;
    ctx->chat.win.rect_w = crosstalk_hub_frac_get((uint32_t)CROSSTALK_HUB_CHAT_COL_CUNT + frame_span, ctx->term_col_cunt);
    if (ctx->chat.win.rect_x + ctx->chat.win.rect_w > CROSSTALK_CORE_DESK_FRAC_FULL) { ctx->chat.win.rect_w = CROSSTALK_CORE_DESK_FRAC_FULL - ctx->chat.win.rect_x; }

    ctx->chat.win.rect_h = crosstalk_hub_frac_get(chat_rows + frame_span, desk_rows);
    ctx->chat.win.rect_y = CROSSTALK_CORE_DESK_FRAC_FULL - ctx->chat.win.rect_h;
}

static void crosstalk_hub_chat_show(crosstalk_ctx_t *ctx)
{
    crosstalk_hub_fit(ctx);
    ctx->win_nav.kbd_pending = LDG_TRUTH_TRUE;

    if (crosstalk_ui_window_focus_set(&ctx->win_nav, (uint64_t)CROSSTALK_GAME_CHAT_WINDOW_ID, ldg_time_epoch_ms_get()) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "hub chat focus failed"); }
}

static uint32_t crosstalk_hub_toggle(crosstalk_ctx_t *ctx)
{
    uint64_t now_ms = ldg_time_epoch_ms_get();
    uint32_t ret = 0;

    if (crosstalk_game_hub_open_is(&ctx->hub) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_game_chat_open_set(&ctx->chat, LDG_TRUTH_FALSE);
        if (ret == LDG_ERR_AOK) { ret = crosstalk_game_hub_open_set(&ctx->hub, LDG_TRUTH_FALSE); }

        if (ret != LDG_ERR_AOK) { return ret; }

        if (crosstalk_ui_window_focused_is(&ctx->win_nav, (uint64_t)CROSSTALK_GAME_HUB_WINDOW_ID) != LDG_TRUTH_TRUE && crosstalk_ui_window_focused_is(&ctx->win_nav, (uint64_t)CROSSTALK_GAME_CHAT_WINDOW_ID) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

        return crosstalk_ui_window_focus_set(&ctx->win_nav, (uint64_t)UINT64_MAX, now_ms);
    }

    ret = crosstalk_game_hub_open_set(&ctx->hub, LDG_TRUTH_TRUE);
    if (ret != LDG_ERR_AOK) { return ret; }

    crosstalk_hub_fit(ctx);
    ctx->win_nav.kbd_pending = LDG_TRUTH_TRUE;

    return crosstalk_ui_window_focus_set(&ctx->win_nav, (uint64_t)CROSSTALK_GAME_HUB_WINDOW_ID, now_ms);
}

static uint32_t crosstalk_hub_key_isr(crosstalk_ctx_t *ctx, const ldg_term_key_event_t *irq, uint8_t *consumed)
{
    uint32_t phys = 0;
    uint32_t mods = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || irq == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    if (irq->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE) { return LDG_ERR_AOK; }

    mods = irq->mods & ((uint32_t)LDG_TERM_MOD_ALT | (uint32_t)LDG_TERM_MOD_CTRL | (uint32_t)LDG_TERM_MOD_SHIFT);

    if (irq->key == (uint8_t)LDG_TERM_KEY_ESCAPE && mods == 0 && crosstalk_game_hub_open_is(&ctx->hub) == LDG_TRUTH_TRUE && ctx->chat.win.open == LDG_TRUTH_TRUE && crosstalk_ui_window_focused_is(&ctx->win_nav, (uint64_t)CROSSTALK_GAME_CHAT_WINDOW_ID) == LDG_TRUTH_TRUE)
    {
        *consumed = LDG_TRUTH_TRUE;

        ret = crosstalk_game_chat_open_set(&ctx->chat, LDG_TRUTH_FALSE);
        if (ret != LDG_ERR_AOK) { return ret; }

        ctx->win_nav.kbd_pending = LDG_TRUTH_TRUE;

        return crosstalk_ui_window_focus_set(&ctx->win_nav, (uint64_t)CROSSTALK_GAME_HUB_WINDOW_ID, ldg_time_epoch_ms_get());
    }

    if (mods != ((uint32_t)LDG_TERM_MOD_CTRL | (uint32_t)LDG_TERM_MOD_SHIFT)) { return LDG_ERR_AOK; }

    phys = irq->base != 0 ? irq->base : irq->codepoint;

    if (phys != (uint32_t)CROSSTALK_HUB_TOGGLE_CODEPOINT && phys != (uint32_t)CROSSTALK_HUB_TOGGLE_CODEPOINT_SHIFTED) { return LDG_ERR_AOK; }

    *consumed = LDG_TRUTH_TRUE;

    return crosstalk_hub_toggle(ctx);
}

static uint32_t crosstalk_pause_key_isr(crosstalk_ctx_t *ctx, const ldg_term_key_event_t *irq, uint8_t *consumed)
{
    uint32_t mods = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || irq == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    if (irq->kind != (uint8_t)LDG_TERM_KEY_KIND_PRESS || irq->key != (uint8_t)LDG_TERM_KEY_ESCAPE || ctx->game.screen != CROSSTALK_GAME_SCREEN_EDITOR) { return LDG_ERR_AOK; }

    mods = irq->mods & ((uint32_t)LDG_TERM_MOD_ALT | (uint32_t)LDG_TERM_MOD_CTRL | (uint32_t)LDG_TERM_MOD_SHIFT);

    if (mods != (uint32_t)LDG_TERM_MOD_SHIFT) { return LDG_ERR_AOK; }

    *consumed = LDG_TRUTH_TRUE;
    ctx->key_esc = LDG_TRUTH_TRUE;
    ctx->key_esc_shift = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_win_shot_key_isr(crosstalk_ctx_t *ctx, const ldg_term_key_event_t *irq, uint8_t *consumed)
{
    uint32_t mods = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || irq == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    if (irq->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE || irq->key != (uint8_t)LDG_TERM_KEY_NONE) { return LDG_ERR_AOK; }

    mods = irq->mods & ((uint32_t)LDG_TERM_MOD_ALT | (uint32_t)LDG_TERM_MOD_CTRL | (uint32_t)LDG_TERM_MOD_SHIFT);

    if (mods != (uint32_t)LDG_TERM_MOD_ALT || (irq->codepoint != (uint32_t)CROSSTALK_CORE_SHOT_CODEPOINT && irq->codepoint != (uint32_t)CROSSTALK_CORE_SHOT_CODEPOINT_SHIFTED)) { return LDG_ERR_AOK; }

    *consumed = LDG_TRUTH_TRUE;
    ctx->shot.pending = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static void crosstalk_shot_take(crosstalk_ctx_t *ctx, const ldg_term_render_t *render, const ldg_term_rect_t *desk)
{
    char name[CROSSTALK_CORE_SHOT_NAME_CAP] = LDG_ARR_ZERO_INIT;
    ldg_term_rect_t rect = LDG_STRUCT_ZERO_INIT;
    uint64_t win_id = 0;
    uint64_t now_ms = ldg_time_epoch_ms_get();
    uint64_t len = 0;
    uint32_t ret = 0;

    ctx->shot.pending = LDG_TRUTH_FALSE;

    ret = crosstalk_ui_window_focus_rect_get(&ctx->win_nav, desk, &rect, &win_id);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_core_shot_capture(&ctx->shot, render, &rect, win_id, now_ms, name, (uint64_t)sizeof(name), &len); }

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "shot: failed; id: 0x%X64; err: %u32", win_id, ret);
        return;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "shot: wr; id: 0x%X64; path: %.63s; cells: %u32x%u32; bytes: %u64", win_id, name, rect.col_cunt, rect.row_cunt, len);

    if (crosstalk_ui_window_flash_set(&ctx->win_nav, win_id, now_ms + (uint64_t)CROSSTALK_CORE_SHOT_FLASH_MS) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "shot: flash set failed"); }
}

static uint32_t crosstalk_win_resize_key_isr(crosstalk_ctx_t *ctx, const ldg_term_key_event_t *irq, uint8_t *consumed)
{
    crosstalk_layout_win_t wins[CROSSTALK_LAYOUT_WIN_CAP] = LDG_ARR_ZERO_INIT;
    crosstalk_ui_window_state_t *win = 0x0;
    uint32_t mods = 0;
    uint32_t cp = 0;
    double step_w = 0.0;
    double step_h = 0.0;
    double next_x = 0.0;
    double next_y = 0.0;
    double next_w = 0.0;
    double next_h = 0.0;
    double d_col = 0.0;
    double d_row = 0.0;
    uint8_t mov = LDG_TRUTH_FALSE;
    uint32_t win_cunt = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || irq == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    if (irq->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE) { return LDG_ERR_AOK; }

    mods = irq->mods & ((uint32_t)LDG_TERM_MOD_ALT | (uint32_t)LDG_TERM_MOD_CTRL | (uint32_t)LDG_TERM_MOD_SHIFT);

    if (mods == ((uint32_t)LDG_TERM_MOD_ALT | (uint32_t)LDG_TERM_MOD_SHIFT)) { mov = LDG_TRUTH_TRUE; }
    else if (mods != (uint32_t)LDG_TERM_MOD_ALT) { return LDG_ERR_AOK; }

    cp = irq->codepoint;
    if (cp >= (uint32_t)CROSSTALK_CORE_RESIZE_UPPER_FIRST && cp <= (uint32_t)CROSSTALK_CORE_RESIZE_UPPER_LAST) { cp = cp + (uint32_t)LDG_ASCII_CASE_GAP; }

    if (ctx->term_col_cunt == 0 || ctx->term_row_cunt == 0) { return LDG_ERR_AOK; }

    step_w = CROSSTALK_CORE_RESIZE_FRAC_FULL / (double)ctx->term_col_cunt;
    step_h = CROSSTALK_CORE_RESIZE_FRAC_FULL / (double)ctx->term_row_cunt;

    ret = crosstalk_layout_wins_get(ctx, wins, (uint32_t)CROSSTALK_LAYOUT_WIN_CAP, &win_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (at = 0; at < win_cunt; at = at + 0x1)
    {
        if (wins[at].win == 0x0 || wins[at].id != ctx->win_nav.focus_id) { continue; }

        win = wins[at].win;
        break;
    }

    if (win == 0x0) { return LDG_ERR_AOK; }

    if (irq->key == (uint8_t)LDG_TERM_KEY_RHS || cp == (uint32_t)CROSSTALK_CORE_RESIZE_CODEPOINT_GROW_W) { d_col = step_w; }
    else if (irq->key == (uint8_t)LDG_TERM_KEY_LHS || cp == (uint32_t)CROSSTALK_CORE_RESIZE_CODEPOINT_SHRINK_W) { d_col = -step_w; }
    else if (irq->key == (uint8_t)LDG_TERM_KEY_DOWN || cp == (uint32_t)CROSSTALK_CORE_RESIZE_CODEPOINT_GROW_H) { d_row = step_h; }
    else if (irq->key == (uint8_t)LDG_TERM_KEY_UP || cp == (uint32_t)CROSSTALK_CORE_RESIZE_CODEPOINT_SHRINK_H) { d_row = -step_h; }
    else { return LDG_ERR_AOK; }

    *consumed = LDG_TRUTH_TRUE;

    next_x = win->rect_x;
    next_y = win->rect_y;
    next_w = win->rect_w;
    next_h = win->rect_h;

    if (mov == LDG_TRUTH_TRUE)
    {
        next_x = next_x + d_col;
        next_y = next_y + d_row;
    }
    else
    {
        next_w = next_w + d_col;
        next_h = next_h + d_row;
    }

    if (next_x < -step_w / CROSSTALK_CORE_RESIZE_SLACK_DIV || next_y < -step_h / CROSSTALK_CORE_RESIZE_SLACK_DIV) { return LDG_ERR_AOK; }

    if (next_w < CROSSTALK_CORE_RESIZE_CELLS_MIN * step_w || next_h < CROSSTALK_CORE_RESIZE_CELLS_MIN * step_h) { return LDG_ERR_AOK; }

    if (next_x + next_w > CROSSTALK_CORE_RESIZE_FRAC_FULL + step_w / CROSSTALK_CORE_RESIZE_SLACK_DIV || next_y + next_h > CROSSTALK_CORE_RESIZE_FRAC_FULL + step_h / CROSSTALK_CORE_RESIZE_SLACK_DIV) { return LDG_ERR_AOK; }

    win->rect_x = next_x;
    win->rect_y = next_y;
    win->rect_w = next_w;
    win->rect_h = next_h;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_win_focus_push(crosstalk_ctx_t *ctx)
{
    uint8_t mv_on = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    mv_on = crosstalk_ui_window_focused_is(&ctx->win_nav, (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_WINDOW_ID);

    if (crosstalk_eda_ui_memview_focus_set(&ctx->eda.memview, mv_on) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview focus push failed"); }

    if (crosstalk_eda_ui_calc_route_set(&ctx->eda.calc, crosstalk_ui_window_focused_is(&ctx->win_nav, (uint64_t)CROSSTALK_EDA_UI_CALC_WINDOW_ID)) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "calc route push failed"); }

    if (ctx->win_nav.focus_changed != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ctx->win_nav.focus_changed = LDG_TRUTH_FALSE;

    if (crosstalk_ui_window_focused_is(&ctx->win_nav, (uint64_t)CROSSTALK_GAME_HUB_WINDOW_ID) == LDG_TRUTH_TRUE && crosstalk_game_hub_open_is(&ctx->hub) != LDG_TRUTH_TRUE)
    {
        if (crosstalk_game_hub_open_set(&ctx->hub, LDG_TRUTH_TRUE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "hub focus open failed"); }

        crosstalk_hub_fit(ctx);
    }

    if (ctx->win_nav.focus_kbd != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ctx->eda.memview.edit = mv_on;

    if (crosstalk_eda_ui_calc_focus_set(&ctx->eda.calc, crosstalk_ui_window_focused_is(&ctx->win_nav, (uint64_t)CROSSTALK_EDA_UI_CALC_WINDOW_ID)) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "calc focus push failed"); }

    return LDG_ERR_AOK;
}

static void crosstalk_verb_gamma(crosstalk_ctx_t *ctx, const char *arg)
{
    char toast[CROSSTALK_CORE_EMU_TOAST_CAP] = LDG_ARR_ZERO_INIT;
    double gamma = 0.0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    if (arg[0] == LDG_STR_TERM)
    {
        if (snprintf(toast, sizeof(toast), CROSSTALK_CORE_GAMMA_SHOW_FMT, ctx->eda.crt.gamma) > 0) { toast_push(ctx, toast); }

        return;
    }

    gamma = strtod(arg, 0x0);

    if (crosstalk_eda_ui_display_crt_gamma_set(&ctx->eda.crt, gamma) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_GAMMA_USAGE_TEXT); return; }

    ctx->eda.crt.gfx_ok = LDG_TRUTH_FALSE;
    ctx->eda.crt.gfx_next = 0.0;

    if (snprintf(toast, sizeof(toast), CROSSTALK_CORE_GAMMA_SHOW_FMT, gamma) > 0) { toast_push(ctx, toast); }
}

static void crosstalk_verb_ident(crosstalk_ctx_t *ctx, const char *arg)
{
    char toast[CROSSTALK_CORE_EMU_TOAST_CAP] = LDG_ARR_ZERO_INIT;
    const crosstalk_hdl_elf_sym_t *sym = 0x0;
    const char *table = CROSSTALK_CORE_IDENT_TABLE_PROG;
    uint64_t addr = 0;
    uint64_t off = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    if (arg[0] == LDG_STR_TERM) { toast_push(ctx, CROSSTALK_CORE_IDENT_USAGE_TEXT); return; }

    if (crosstalk_addr_parse(ctx, arg, &addr) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_IDENT_USAGE_TEXT); return; }

    if (crosstalk_hdl_elf_sym_get(&ctx->prog_elf, addr, &sym, &off) != LDG_ERR_AOK || sym == 0x0)
    {
        table = CROSSTALK_CORE_IDENT_TABLE_KERNEL;

        if (crosstalk_hdl_elf_sym_get(&ctx->elf, addr, &sym, &off) != LDG_ERR_AOK || sym == 0x0)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "ident: no symbol covers it; addr: 0x%X64", addr);

            toast_push(ctx, CROSSTALK_CORE_IDENT_NONE_TEXT);

            return;
        }
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "ident: addr: 0x%X64; sym: %.63s; off: 0x%X64; base: 0x%X64; size: %u64; table: %.7s", addr, sym->name, off, sym->addr, sym->size, table);

    if (snprintf(toast, sizeof(toast), CROSSTALK_CORE_IDENT_OK_FMT, sym->name, off) > 0) { toast_push(ctx, toast); }
}

static void crosstalk_prog_pub_poll(crosstalk_ctx_t *ctx)
{
    uint64_t word[CROSSTALK_CORE_PROG_PUB_WORD_CUNT] = LDG_ARR_ZERO_INIT;
    uint64_t cell[CROSSTALK_CORE_PROG_PUB_WORD_CUNT] = LDG_ARR_ZERO_INIT;
    char name[CROSSTALK_CORE_PROG_PUB_NAME_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t base = 0;
    uint32_t cell_bytes = 0;
    uint32_t words_per_cell = 0;
    uint32_t cell_cunt = 0;
    uint32_t got = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    cell_bytes = ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT].emu == LDG_TRUTH_TRUE ? (uint32_t)sizeof(uint64_t) : (uint32_t)CROSSTALK_CORE_PROG_PUB_WORD_BYTES;
    words_per_cell = cell_bytes / (uint32_t)CROSSTALK_CORE_PROG_PUB_WORD_BYTES;
    cell_cunt = (uint32_t)CROSSTALK_CORE_PROG_PUB_WORD_CUNT / words_per_cell;

    if (crosstalk_eda_ic_fpga_mem_rd(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, CROSSTALK_CORE_MEMVIEW_SIG_DEFAULT, (uint64_t)CROSSTALK_CORE_PROG_PUB_ADDR / cell_bytes, cell, cell_cunt, &got) != LDG_ERR_AOK) { return; }

    if (got < cell_cunt) { return; }

    for (at = 0; at < (uint32_t)CROSSTALK_CORE_PROG_PUB_WORD_CUNT; at = at + 0x1) { word[at] = (cell[at / words_per_cell] >> ((at % words_per_cell) * (uint32_t)CROSSTALK_CORE_PROG_PUB_HI_SHIFT)) & (uint64_t)UINT32_MAX; }

    if (word[CROSSTALK_CORE_PROG_PUB_MAGIC_AT] != (uint64_t)CROSSTALK_CORE_PROG_PUB_MAGIC) { return; }

    if (word[CROSSTALK_CORE_PROG_PUB_REV_AT] != (uint64_t)CROSSTALK_CORE_PROG_PUB_REV) { return; }

    base = word[CROSSTALK_CORE_PROG_PUB_LIVE_AT] != (uint64_t)LDG_TRUTH_FALSE ? (word[CROSSTALK_CORE_PROG_PUB_BASE_LO_AT] | (word[CROSSTALK_CORE_PROG_PUB_BASE_HI_AT] << (uint64_t)CROSSTALK_CORE_PROG_PUB_HI_SHIFT)) : 0x0;

    if (base == crosstalk_hdl_elf_base_get(&ctx->prog_elf)) { return; }

    if (crosstalk_hdl_elf_base_set(&ctx->prog_elf, base) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "prog syms base set failed"); return; }

    if (base == 0x0)
    {
        ctx->prog_pub_name[0] = LDG_STR_TERM;

        if (crosstalk_eda_ic_fpga_emu_stat_prog_close(&ctx->emu_stat) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu stat prog close failed"); }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "prog: cleared; symbols are back to link addresses");
        return;
    }

    for (at = 0; at < (uint32_t)CROSSTALK_CORE_PROG_PUB_NAME_CAP - LDG_STR_TERM_SIZE; at = at + 0x1) { name[at] = (char)((word[(uint32_t)CROSSTALK_CORE_PROG_PUB_NAME_AT + at / (uint32_t)CROSSTALK_CORE_PROG_PUB_WORD_BYTES] >> ((uint64_t)(at % (uint32_t)CROSSTALK_CORE_PROG_PUB_WORD_BYTES) * (uint64_t)LDG_BYTE_BITS)) & (uint64_t)LDG_BYTE_MASK); }

    name[(uint32_t)CROSSTALK_CORE_PROG_PUB_NAME_CAP - LDG_STR_TERM_SIZE] = LDG_STR_TERM;

    if (ldg_str_rbrcpy(ctx->prog_pub_name, name, (uint64_t)sizeof(ctx->prog_pub_name)) != LDG_ERR_AOK) { ctx->prog_pub_name[0] = LDG_STR_TERM; }

    if (crosstalk_eda_ic_fpga_emu_stat_prog_open(&ctx->emu_stat, name, ldg_time_epoch_ms_get()) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu stat prog open failed"); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "prog: base published; name: %.32s; base: 0x%X64; width: %u64; entry: 0x%X64; seq: %u64", name, base, word[CROSSTALK_CORE_PROG_PUB_WIDTH_AT], word[CROSSTALK_CORE_PROG_PUB_ENTRY_LO_AT] | (word[CROSSTALK_CORE_PROG_PUB_ENTRY_HI_AT] << (uint64_t)CROSSTALK_CORE_PROG_PUB_HI_SHIFT), word[CROSSTALK_CORE_PROG_PUB_SEQ_AT]);

    crosstalk_ach_once(ctx, CROSSTALK_GAME_PROGRESSION_FIRST_PROG_PUB);
}

static void crosstalk_relay_step_cb(const crosstalk_hdl_abi_t *abi, void *user)
{
    crosstalk_ctx_t *ctx = (crosstalk_ctx_t *)user;

    if (LDG_UNLIKELY(ctx == 0x0 || abi == 0x0)) { return; }

    if (crosstalk_net_relay_live_is(&ctx->relay) != LDG_TRUTH_TRUE) { return; }

    if (abi->ok != LDG_TRUTH_TRUE) { return; }

    if (crosstalk_net_relay_tick(&ctx->relay, abi, LDG_TRUTH_FALSE) != LDG_ERR_AOK) { return; }
}

static void crosstalk_relay_poll(crosstalk_ctx_t *ctx)
{
    const crosstalk_hdl_abi_t *abi = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (crosstalk_net_relay_live_is(&ctx->relay) != LDG_TRUTH_TRUE) { return; }

    if (crosstalk_eda_ic_fpga_slot_abi_get(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, &abi) != LDG_ERR_AOK) { return; }

    if (abi == 0x0 || abi->ok != LDG_TRUTH_TRUE) { return; }

    ret = crosstalk_net_relay_tick(&ctx->relay, abi, LDG_TRUTH_TRUE);

    if (ret != LDG_ERR_AOK && ret != LDG_ERR_UNSUPPORTED) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "relay: tick failed; err: %u32", ret); }
}

static void crosstalk_memld(crosstalk_ctx_t *ctx, const char *arg)
{
    char sig[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    char path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char full[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char rest[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char tail[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t words = 0;
    uint64_t bytes = 0;
    uint32_t off = 0;
    uint32_t at = 0;
    uint32_t pos = 0;
    uint32_t ret = 0;
    uint8_t was_live = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (at < (uint32_t)CROSSTALK_CORE_SOUND_ARG_CAP - LDG_STR_TERM_SIZE && arg[at] != LDG_STR_TERM && arg[at] != ' ')
    {
        sig[at] = arg[at];
        at = at + 0x1;
    }

    sig[at] = LDG_STR_TERM;

    if (arg[at] != ' ')
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memld: usage; no path after the signal; arg: %.127s", arg);
        toast_push(ctx, CROSSTALK_CORE_MEMLD_USAGE_TEXT);
        return;
    }

    at = at + 0x1;

    while (pos < (uint32_t)LDG_IO_PATH_MAX - LDG_STR_TERM_SIZE && arg[at] != LDG_STR_TERM)
    {
        rest[pos] = arg[at];
        pos = pos + 0x1;
        at = at + 0x1;
    }

    rest[pos] = LDG_STR_TERM;

    if (crosstalk_tail_split(rest, path, (uint32_t)LDG_IO_PATH_MAX, tail, (uint32_t)CROSSTALK_CORE_SOUND_ARG_CAP) == LDG_ERR_AOK && ldg_str_to_dec(tail, &off) == LDG_ERR_AOK) { if (path[0] == LDG_STR_TERM) { toast_push(ctx, CROSSTALK_CORE_MEMLD_USAGE_TEXT); return; } }
    else
    {
        off = 0x0;

        if (ldg_str_rbrcpy(path, rest, (uint64_t)sizeof(path)) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_MEMLD_USAGE_TEXT); return; }
    }

    if (path[0] == LDG_STR_TERM) { toast_push(ctx, CROSSTALK_CORE_MEMLD_USAGE_TEXT); return; }

    ret = crosstalk_script_path_resolve(ctx, path, full, (uint64_t)sizeof(full));
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_MEMLD_USAGE_TEXT); return; }

    was_live = crosstalk_fpga_hlt(ctx);

    ret = crosstalk_eda_ic_fpga_mem_ld(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, sig, full, (uint64_t)off, &words, &bytes);

    crosstalk_fpga_resume(ctx, was_live);

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memld: failed; sig: %.63s; path: %.127s; err: %u32", sig, full, ret);

        if (ret == CROSSTALK_EDA_IC_FPGA_ERR_MEM_SIG) { toast_push(ctx, CROSSTALK_CORE_MEMLD_NO_SIG_TEXT); return; }

        toast_push(ctx, CROSSTALK_CORE_MEMLD_FAIL_TEXT);
        return;
    }

    crosstalk_syms_sibling_ld(ctx, full);

    crosstalk_ach_once(ctx, CROSSTALK_GAME_PROGRESSION_FIRST_MEMLD);

    toast_push(ctx, CROSSTALK_CORE_MEMLD_OK_TEXT);
}

static void crosstalk_memrd(crosstalk_ctx_t *ctx, const char *arg)
{
    uint64_t word[CROSSTALK_CORE_MEMRD_ROW_CUNT] = LDG_ARR_ZERO_INIT;
    char sig[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    char rest[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    char tail[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    char row[CROSSTALK_CORE_MEMRD_TEXT_CAP] = LDG_ARR_ZERO_INIT;
    char ascii[CROSSTALK_CORE_MEMRD_ROW_CUNT + LDG_STR_TERM_SIZE] = LDG_ARR_ZERO_INIT;
    uint64_t addr = 0;
    uint64_t want = CROSSTALK_CORE_MEMRD_ROW_CUNT;
    uint32_t rd = 0;
    uint32_t at = 0;
    uint32_t pos = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(sig)) { sig[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

    sig[pos] = LDG_STR_TERM;
    pos = 0;

    while (arg[at] == ' ') { at = at + 0x1; }

    while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(tail)) { tail[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

    tail[pos] = LDG_STR_TERM;
    pos = 0;

    while (arg[at] == ' ') { at = at + 0x1; }

    while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(rest)) { rest[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

    rest[pos] = LDG_STR_TERM;
    pos = 0;

    if (sig[0] == LDG_STR_TERM || tail[0] == LDG_STR_TERM) { toast_push(ctx, CROSSTALK_CORE_MEMRD_USAGE_TEXT); return; }

    if (crosstalk_addr_parse(ctx, tail, &addr) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_MEMRD_USAGE_TEXT); return; }

    if (rest[0] != LDG_STR_TERM && crosstalk_num_parse(rest, &want) != LDG_ERR_AOK) { want = CROSSTALK_CORE_MEMRD_ROW_CUNT; }

    if (want == 0 || want > (uint64_t)CROSSTALK_CORE_MEMRD_ROW_CUNT) { want = CROSSTALK_CORE_MEMRD_ROW_CUNT; }

    ret = crosstalk_eda_ic_fpga_mem_byte_rd(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, sig, addr, word, (uint32_t)want, &rd);
    if (ret != LDG_ERR_AOK || rd == 0)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memrd: failed; sig: %.63s; addr: 0x%X64; err: %u32", sig, addr, ret);
        toast_push(ctx, CROSSTALK_CORE_MEMRD_FAIL_TEXT);
        return;
    }

    for (at = 0; at < rd; at = at + 0x1)
    {
        int32_t wrote = snprintf(row + pos, sizeof(row) - pos, CROSSTALK_CORE_MEMRD_BYTE_FMT, word[at] & (uint64_t)CROSSTALK_CORE_MEMRD_BYTE_MASK);

        if (wrote < 0 || (uint32_t)wrote >= (uint32_t)sizeof(row) - pos) { break; }

        pos = pos + (uint32_t)wrote;

        ascii[at] = (word[at] >= (uint64_t)LDG_ASCII_PRINTABLE_MIN && word[at] <= (uint64_t)LDG_ASCII_PRINTABLE_MAX) ? (char)word[at] : (char)LDG_ASCII_NONPRINTABLE_CHAR;
    }

    ascii[at] = LDG_STR_TERM;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memrd: sig: %.63s; addr: 0x%X64; cunt: %u32; hex:%.191s; ascii: %.64s", sig, addr, rd, row, ascii);
    toast_push(ctx, CROSSTALK_CORE_MEMRD_OK_TEXT);
}

static void crosstalk_memview_bake_mount(crosstalk_ctx_t *ctx, const char *arg, uint8_t diff_is)
{
    crosstalk_hdl_bake_ident_t ident = LDG_STRUCT_ZERO_INIT;
    crosstalk_hdl_bake_hdr_t hdr = LDG_STRUCT_ZERO_INIT;
    const crosstalk_eda_ic_fpga_slot_t *slot = 0x0;
    ldg_io_file_t *fh = 0x0;
    char name[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    char path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t *live_base = 0x0;
    uint64_t sig_offset = 0;
    uint64_t snap_base = 0;
    uint64_t got = 0;
    uint32_t live_cunt = 0;
    uint32_t at = 0;
    uint32_t pos = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (arg[at] == ' ') { at = at + 0x1; }

    while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(name)) { name[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

    name[pos] = LDG_STR_TERM;

    ret = crosstalk_snap_ready_get(ctx, name[0] != LDG_STR_TERM ? name : 0x0, path, (uint64_t)sizeof(path));
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_SNAP_NO_ABI_TEXT); return; }

    if (ctx->eda.memview.bound != LDG_TRUTH_TRUE)
    {
        ret = crosstalk_eda_ui_memview_bind(&ctx->eda.memview, &ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, CROSSTALK_CORE_MEMVIEW_SIG_DEFAULT);
        if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_MEMVIEW_BIND_FAIL_TEXT); return; }
    }

    slot = &ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT];

    ret = crosstalk_hdl_bake_ident_build(slot->elab_src, slot->elab_src_len, slot->top, 0x0, &ident);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_MEMSNAP_FAIL_TEXT); return; }

    ret = crosstalk_eda_ic_fpga_mem_offset_get(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, ctx->eda.memview.sig, &sig_offset);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_MEMSNAP_FAIL_TEXT); return; }

    if (slot->abi->state_get((uint32_t)CROSSTALK_HDL_ABI_STATE_MEM, &live_base, &live_cunt) != LDG_ERR_AOK || live_base == 0x0) { toast_push(ctx, CROSSTALK_CORE_MEMSNAP_FAIL_TEXT); return; }

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, 0x0, &fh);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview: snap open failed; path: %.255s; err: %u32", path, ret);
        toast_push(ctx, CROSSTALK_CORE_MEMSNAP_FAIL_TEXT);
        return;
    }

    ret = ldg_io_file_rd(fh, &hdr, (uint64_t)sizeof(hdr), &got);

    if (ret != LDG_ERR_AOK || got != (uint64_t)sizeof(hdr)) { ret = CROSSTALK_HDL_ERR_BAKE_TRUNC; }
    else if (memcmp(hdr.magic, CROSSTALK_HDL_BAKE_MAGIC, (uint64_t)CROSSTALK_HDL_BAKE_MAGIC_SIZE) != 0) { ret = CROSSTALK_HDL_ERR_BAKE_MAGIC; }
    else if (hdr.rev != (uint32_t)CROSSTALK_HDL_BAKE_REV) { ret = CROSSTALK_HDL_ERR_BAKE_REV; }
    else if (hdr.toolchain_rev != (uint32_t)CROSSTALK_HDL_BAKE_TOOLCHAIN_REV) { ret = CROSSTALK_HDL_ERR_BAKE_TOOLCHAIN; }
    else if (hdr.src_hash != ident.src_hash) { ret = CROSSTALK_HDL_ERR_BAKE_SRC; }
    else if (strncmp(hdr.top, ident.top, (uint64_t)sizeof(hdr.top)) != 0) { ret = CROSSTALK_HDL_ERR_BAKE_TOP; }
    else if (hdr.mem_cunt != live_cunt) { ret = CROSSTALK_HDL_ERR_BAKE_MEM_CUNT; }
    else { ret = LDG_ERR_AOK; }

    if (ret != LDG_ERR_AOK)
    {
        if (ldg_io_file_close(fh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview snap close failed"); }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview: snap refused; path: %.255s; err: %u32", path, ret);
        toast_push(ctx, ret == CROSSTALK_HDL_ERR_BAKE_TRUNC ? CROSSTALK_CORE_MEMSNAP_FAIL_TEXT : CROSSTALK_CORE_MEMSNAP_MISMATCH_TEXT);
        return;
    }

    snap_base = (uint64_t)sizeof(hdr) + ((uint64_t)hdr.node_cunt + sig_offset) * (uint64_t)CROSSTALK_HDL_BAKE_WORD_SIZE;

    if (diff_is == LDG_TRUTH_TRUE) { ret = crosstalk_eda_ui_memview_diff_set(&ctx->eda.memview, fh, snap_base); }
    else { ret = crosstalk_eda_ui_memview_snap_set(&ctx->eda.memview, fh, snap_base); }

    if (ret != LDG_ERR_AOK)
    {
        if (ldg_io_file_close(fh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview snap close failed"); }

        toast_push(ctx, CROSSTALK_CORE_MEMSNAP_FAIL_TEXT);
        return;
    }

    ret = crosstalk_eda_ui_memview_open_set(&ctx->eda.memview, LDG_TRUTH_TRUE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview open failed"); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview: bake mounted; path: %.255s; sig: %.63s; sig_offset: 0x%X64; base: 0x%X64; diff: %u32", path, ctx->eda.memview.sig, sig_offset, snap_base, diff_is);

    crosstalk_ach_once(ctx, CROSSTALK_GAME_PROGRESSION_FIRST_SNAP);
    toast_push(ctx, diff_is == LDG_TRUTH_TRUE ? CROSSTALK_CORE_MEMDIFF_OK_TEXT : CROSSTALK_CORE_MEMSNAP_OK_TEXT);
}

static uint32_t crosstalk_memview_break(crosstalk_ctx_t *ctx, const char *arg)
{
    char num_txt[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    char toast[CROSSTALK_CORE_EMU_TOAST_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t addr[CROSSTALK_CORE_MEMWATCH_ADDR_CUNT] = LDG_ARR_ZERO_INIT;
    uint64_t lo = 0;
    uint64_t hi = 0;
    uint32_t addr_cunt = 0;
    uint32_t at = 0;
    uint32_t pos = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (arg[at] == ' ') { at = at + 0x1; }

    while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(num_txt)) { num_txt[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

    num_txt[pos] = LDG_STR_TERM;

    if (strcmp(num_txt, CROSSTALK_CORE_OFF_TOK) == 0)
    {
        ret = crosstalk_eda_ic_fpga_watch_set(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, (uint64_t)CROSSTALK_CORE_MEMBREAK_RANGE_NONE, (uint64_t)CROSSTALK_CORE_MEMBREAK_RANGE_NONE, LDG_TRUTH_FALSE);
        if (ret == LDG_ERR_UNSUPPORTED) { return LDG_ERR_UNSUPPORTED; }

        ctx->eda.memview.watch_hlt = LDG_TRUTH_FALSE;

        if (crosstalk_eda_ui_memview_brk_set(&ctx->eda.memview, (uint64_t)CROSSTALK_CORE_MEMBREAK_RANGE_NONE, (uint64_t)CROSSTALK_CORE_MEMBREAK_RANGE_NONE, LDG_TRUTH_FALSE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview brk clear failed"); }

        toast_push(ctx, ret == LDG_ERR_AOK ? CROSSTALK_CORE_MEMBREAK_OFF_TEXT : CROSSTALK_CORE_MEMBREAK_REFUSED_TEXT);

        return LDG_ERR_AOK;
    }

    at = 0;

    while (addr_cunt < (uint32_t)CROSSTALK_CORE_MEMWATCH_ADDR_CUNT)
    {
        uint32_t scan = at;

        while (arg[scan] == ' ') { scan = scan + 0x1; }

        if (arg[scan] == LDG_STR_TERM) { break; }

        pos = 0;

        while (arg[scan] != LDG_STR_TERM && arg[scan] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(num_txt)) { num_txt[pos] = arg[scan]; pos = pos + 0x1; scan = scan + 0x1; }

        num_txt[pos] = LDG_STR_TERM;

        if (crosstalk_addr_tok_parse(ctx, num_txt, &addr[addr_cunt]) != LDG_ERR_AOK) { break; }

        at = scan;
        addr_cunt = addr_cunt + 0x1;
    }

    if (addr_cunt == 0) { return LDG_ERR_UNSUPPORTED; }

    while (arg[at] == ' ') { at = at + 0x1; }

    if (arg[at] != LDG_STR_TERM) { return LDG_ERR_UNSUPPORTED; }

    lo = addr[0];
    hi = addr_cunt == (uint32_t)CROSSTALK_CORE_MEMWATCH_ADDR_CUNT ? addr[LDG_STR_TERM_SIZE] : lo;

    if (hi < lo) { toast_push(ctx, CROSSTALK_CORE_MEMBREAK_USAGE_TEXT); return LDG_ERR_AOK; }

    ret = crosstalk_eda_ic_fpga_watch_set(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, lo, hi, LDG_TRUTH_TRUE);
    if (ret == LDG_ERR_UNSUPPORTED) { return LDG_ERR_UNSUPPORTED; }

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview: store watch refused; lo: 0x%X64; hi: 0x%X64; err: %u32", lo, hi, ret);
        toast_push(ctx, CROSSTALK_CORE_MEMBREAK_REFUSED_TEXT);

        return LDG_ERR_AOK;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview: store watch armed; lo: 0x%X64; hi: 0x%X64", lo, hi);

    if (crosstalk_eda_ui_memview_brk_set(&ctx->eda.memview, lo, hi, LDG_TRUTH_TRUE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview brk set failed"); }

    if (snprintf(toast, sizeof(toast), CROSSTALK_CORE_MEMBREAK_ARM_FMT, lo, hi) > 0) { toast_push(ctx, toast); }

    return LDG_ERR_AOK;
}

static void crosstalk_memview_watch(crosstalk_ctx_t *ctx, const char *arg)
{
    char num_txt[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    char text[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t addr[CROSSTALK_CORE_MEMWATCH_ADDR_CUNT] = LDG_ARR_ZERO_INIT;
    uint64_t lo = 0;
    uint64_t hi = UINT64_MAX;
    uint32_t addr_cunt = 0;
    uint32_t mode = 0;
    uint32_t at = 0;
    uint32_t pos = 0;
    uint32_t ret = 0;
    uint8_t ascii_is = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (addr_cunt < (uint32_t)CROSSTALK_CORE_MEMWATCH_ADDR_CUNT)
    {
        uint32_t scan = at;

        while (arg[scan] == ' ') { scan = scan + 0x1; }

        if (arg[scan] == LDG_STR_TERM || arg[scan] == LDG_ASCII_QUOTE) { break; }

        pos = 0;

        while (arg[scan] != LDG_STR_TERM && arg[scan] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(num_txt)) { num_txt[pos] = arg[scan]; pos = pos + 0x1; scan = scan + 0x1; }

        num_txt[pos] = LDG_STR_TERM;

        if (crosstalk_addr_tok_parse(ctx, num_txt, &addr[addr_cunt]) != LDG_ERR_AOK) { break; }

        at = scan;
        addr_cunt = addr_cunt + 0x1;
    }

    while (arg[at] == ' ') { at = at + 0x1; }

    if (arg[at] == LDG_ASCII_QUOTE)
    {
        ascii_is = LDG_TRUTH_TRUE;
        at = at + LDG_STR_TERM_SIZE;
    }

    pos = 0;

    while (arg[at] != LDG_STR_TERM && arg[at] != LDG_ASCII_QUOTE && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(text)) { text[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

    text[pos] = LDG_STR_TERM;

    if (text[0] == CROSSTALK_CORE_MEMWATCH_UP_MARK || text[0] == CROSSTALK_CORE_MEMWATCH_DOWN_MARK || text[0] == CROSSTALK_CORE_MEMWATCH_ANY_MARK || text[0] == CROSSTALK_CORE_MEMWATCH_BOUND_MARK)
    {
        uint64_t mag = 0;
        uint64_t run = 0;
        int64_t step = 0;
        uint32_t kind = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_MOTION_CHANGED;
        uint32_t tail = 0;

        for (tail = 0; text[tail] != LDG_STR_TERM; tail = tail + 0x1)
        {
            if (text[tail] != ' ') { continue; }

            text[tail] = LDG_STR_TERM;

            tail = tail + 0x1;

            while (text[tail] == ' ') { tail = tail + 0x1; }

            if (strncmp(&text[tail], CROSSTALK_CORE_MEMMOTION_RUN_TOK, sizeof(CROSSTALK_CORE_MEMMOTION_RUN_TOK) - LDG_STR_TERM_SIZE) != 0) { toast_push(ctx, CROSSTALK_CORE_MEMMOTION_USAGE_TEXT); return; }

            tail = tail + (uint32_t)sizeof(CROSSTALK_CORE_MEMMOTION_RUN_TOK) - LDG_STR_TERM_SIZE;

            while (text[tail] == ' ') { tail = tail + 0x1; }

            if (crosstalk_num_parse(&text[tail], &run) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_MEMMOTION_USAGE_TEXT); return; }

            break;
        }

        if (run > (uint64_t)CROSSTALK_CORE_MEMMOTION_RUN_MAX) { toast_push(ctx, CROSSTALK_CORE_MEMMOTION_USAGE_TEXT); return; }

        if (text[LDG_STR_TERM_SIZE] != LDG_STR_TERM && crosstalk_num_parse(&text[LDG_STR_TERM_SIZE], &mag) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_MEMMOTION_USAGE_TEXT); return; }

        step = (int64_t)mag;

        if (text[0] == CROSSTALK_CORE_MEMWATCH_DOWN_MARK) { step = -step; }

        if (text[0] == CROSSTALK_CORE_MEMWATCH_ANY_MARK) { kind = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_MOTION_CHANGED; }
        else if (text[0] == CROSSTALK_CORE_MEMWATCH_BOUND_MARK) { kind = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_MOTION_BOUND; }
        else if (text[LDG_STR_TERM_SIZE] == LDG_STR_TERM) { kind = text[0] == CROSSTALK_CORE_MEMWATCH_UP_MARK ? (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_MOTION_UP : (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_MOTION_DOWN; }
        else { kind = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_MOTION_STEP; }

        if (ctx->eda.memview.bound != LDG_TRUTH_TRUE)
        {
            ret = crosstalk_eda_ui_memview_bind(&ctx->eda.memview, &ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, CROSSTALK_CORE_MEMVIEW_SIG_DEFAULT);
            if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_MEMVIEW_BIND_FAIL_TEXT); return; }
        }

        if (addr_cunt != 0 && addr_cunt != (uint32_t)CROSSTALK_CORE_MEMWATCH_ADDR_CUNT) { toast_push(ctx, CROSSTALK_CORE_MEMMOTION_USAGE_TEXT); return; }

        lo = addr_cunt != 0 ? addr[0] : (uint64_t)CROSSTALK_CORE_MEMMOTION_ADDR_ORIG;
        hi = addr_cunt != 0 ? addr[LDG_STR_TERM_SIZE] : ctx->eda.memview.width;

        ret = crosstalk_eda_ui_memview_motion_start(&ctx->eda.memview, kind, step, lo, hi, (uint32_t)run);
        if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_MEMMOTION_USAGE_TEXT); return; }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview: motion watch; kind: %u32; step: %i64; lo: 0x%X64; hi: 0x%X64", kind, step, lo, hi);

        toast_push(ctx, CROSSTALK_CORE_MEMMOTION_OK_TEXT);

        return;
    }

    if (addr_cunt == (uint32_t)CROSSTALK_CORE_MEMWATCH_ADDR_CUNT && text[0] == LDG_STR_TERM)
    {
        uint64_t val = addr[LDG_STR_TERM_SIZE];
        uint32_t bytes = 0x1;
        uint32_t byte_at = 0;
        uint32_t wrote = 0;

        while (bytes < (uint32_t)LDG_WORD_BYTES && (val >> ((uint64_t)bytes * (uint64_t)LDG_BYTE_BITS)) != 0) { bytes = bytes + 0x1; }

        for (byte_at = 0; byte_at < bytes; byte_at = byte_at + 0x1)
        {
            int32_t put = snprintf(&text[wrote], sizeof(text) - wrote, CROSSTALK_CORE_MEMWATCH_BYTE_FMT, (val >> ((uint64_t)byte_at * (uint64_t)LDG_BYTE_BITS)) & (uint64_t)LDG_BYTE_MASK);

            if (put < 0) { break; }

            wrote = wrote + (uint32_t)put;
        }

        addr_cunt = 0x1;
    }

    if (addr_cunt == (uint32_t)CROSSTALK_CORE_MEMWATCH_ADDR_CUNT)
    {
        if (text[0] == LDG_STR_TERM) { toast_push(ctx, CROSSTALK_CORE_MEMWATCH_USAGE_TEXT); return; }

        lo = addr[0];
        hi = addr[LDG_STR_TERM_SIZE];
        mode = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WATCH_SCAN;
    }
    else if (addr_cunt != 0 && text[0] != LDG_STR_TERM)
    {
        lo = addr[0];
        mode = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WATCH_AT;
    }
    else if (addr_cunt != 0)
    {
        lo = addr[0];
        mode = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WATCH_MUT;
    }
    else if (text[0] != LDG_STR_TERM) { mode = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WATCH_SCAN; }
    else { toast_push(ctx, CROSSTALK_CORE_MEMWATCH_USAGE_TEXT); return; }

    if (ctx->eda.memview.bound != LDG_TRUTH_TRUE)
    {
        ret = crosstalk_eda_ui_memview_bind(&ctx->eda.memview, &ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, CROSSTALK_CORE_MEMVIEW_SIG_DEFAULT);
        if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_MEMVIEW_BIND_FAIL_TEXT); return; }
    }

    ret = crosstalk_eda_ui_memview_watch_start(&ctx->eda.memview, text, ascii_is, lo, hi, mode);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview: watch refused; text: %.63s; lo: 0x%X64; hi: 0x%X64; mode: %u32; err: %u32", text, lo, hi, mode, ret);
        toast_push(ctx, CROSSTALK_CORE_MEMWATCH_FAIL_TEXT);
        return;
    }

    ret = crosstalk_eda_ui_memview_open_set(&ctx->eda.memview, LDG_TRUTH_TRUE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview open failed"); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview: watch armed; text: %.63s; lo: 0x%X64; hi: 0x%X64; mode: %u32", text, lo, hi, mode);
    crosstalk_ach_once(ctx, CROSSTALK_GAME_PROGRESSION_FIRST_MEM_WATCH);
    toast_push(ctx, CROSSTALK_CORE_MEMWATCH_OK_TEXT);
}

static const char* crosstalk_memview_sig_get(const crosstalk_ctx_t *ctx)
{
    return ctx->eda.memview.bound == LDG_TRUTH_TRUE ? ctx->eda.memview.sig : CROSSTALK_CORE_MEMVIEW_SIG_DEFAULT;
}

static uint32_t crosstalk_memview_bytes_wr(crosstalk_ctx_t *ctx, uint64_t addr, const uint64_t *vals, uint32_t cunt, uint32_t *out_wrote)
{
    const char *sig = 0x0;
    uint64_t width = 0;
    uint64_t end = 0;
    uint32_t elem_width = 0;
    uint32_t elem_bytes = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || vals == 0x0 || out_wrote == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_wrote = 0;
    if (LDG_UNLIKELY(ctx->eda.memview.snap_mode == LDG_TRUTH_TRUE)) { return LDG_ERR_DENIED; }

    if (cunt == 0) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY((uint64_t)cunt - 0x1 > UINT64_MAX - addr)) { return LDG_ERR_BOUNDS; }

    sig = crosstalk_memview_sig_get(ctx);
    ret = crosstalk_eda_ic_fpga_mem_width_get(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, sig, &width, &elem_width);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(elem_width > sizeof(uint64_t) * LDG_BYTE_BITS)) { return LDG_ERR_UNSUPPORTED; }

    elem_bytes = elem_width / (uint32_t)LDG_BYTE_BITS;
    if (elem_bytes == 0) { elem_bytes = 0x1; }

    end = addr + (uint64_t)cunt - 0x1;
    if (LDG_UNLIKELY(end / elem_bytes >= width)) { return LDG_ERR_BOUNDS; }

    while (at < cunt)
    {
        uint64_t byte_addr = 0;
        uint64_t elem_at = 0;
        uint64_t word = 0;
        uint32_t lead = 0;
        uint32_t span = 0;
        uint32_t rd = 0;
        uint32_t wrote = 0;
        uint32_t byte = 0;

        byte_addr = addr + (uint64_t)at;
        elem_at = byte_addr / elem_bytes;
        lead = (uint32_t)(byte_addr % elem_bytes);
        span = elem_bytes - lead;
        if (span > cunt - at) { span = cunt - at; }

        if (lead != 0 || span != elem_bytes)
        {
            ret = crosstalk_eda_ic_fpga_mem_rd(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, sig, elem_at, &word, 0x1, &rd);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (LDG_UNLIKELY(rd != 0x1)) { return LDG_ERR_IO_RD; }
        }

        for (byte = 0; byte < span; byte = byte + 0x1)
        {
            uint32_t shift = 0;

            shift = (lead + byte) * (uint32_t)LDG_BYTE_BITS;
            word = (word & ~((uint64_t)LDG_BYTE_MASK << shift)) | ((vals[at + byte] & (uint64_t)LDG_BYTE_MASK) << shift);
        }

        ret = crosstalk_eda_ic_fpga_mem_wr(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, sig, elem_at, &word, 0x1, &wrote);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(wrote != 0x1)) { return LDG_ERR_IO_WR; }

        at = at + span;
        *out_wrote = at;
    }

    return LDG_ERR_AOK;
}

static void crosstalk_memview_freeze(crosstalk_ctx_t *ctx, const char *arg)
{
    char tok[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t addr = 0;
    uint64_t len = (uint64_t)CROSSTALK_CORE_FREEZE_LEN_DEFAULT;
    uint32_t rd = 0;
    uint32_t at = 0;
    uint32_t pos = 0;
    uint32_t slot = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (arg[at] == ' ') { at = at + 0x1; }

    while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(tok)) { tok[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

    tok[pos] = LDG_STR_TERM;

    if (tok[0] == LDG_STR_TERM) { toast_push(ctx, CROSSTALK_CORE_FREEZE_USAGE_TEXT); return; }

    if (strcmp(tok, CROSSTALK_CORE_OFF_TOK) == 0)
    {
        while (arg[at] == ' ') { at = at + 0x1; }

        if (arg[at] == LDG_STR_TERM)
        {
            ctx->freeze_cunt = 0;

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview: freeze cleared; all");
            toast_push(ctx, CROSSTALK_CORE_FREEZE_OFF_TEXT);

            return;
        }

        if (crosstalk_addr_parse(ctx, &arg[at], &addr) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_FREEZE_USAGE_TEXT); return; }

        for (slot = 0; slot < ctx->freeze_cunt; slot = slot + 0x1)
        {
            if (ctx->freeze_addr[slot] != addr) { continue; }

            ctx->freeze_addr[slot] = ctx->freeze_addr[ctx->freeze_cunt - 0x1];
            ctx->freeze_len[slot] = ctx->freeze_len[ctx->freeze_cunt - 0x1];

            memcpy(ctx->freeze_val[slot], ctx->freeze_val[ctx->freeze_cunt - 0x1], sizeof(ctx->freeze_val[slot]));

            ctx->freeze_cunt = ctx->freeze_cunt - 0x1;

            break;
        }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview: freeze cleared; addr: 0x%X64; held: %u32", addr, ctx->freeze_cunt);
        toast_push(ctx, CROSSTALK_CORE_FREEZE_OFF_TEXT);

        return;
    }

    if (crosstalk_addr_parse(ctx, tok, &addr) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_FREEZE_USAGE_TEXT); return; }

    while (arg[at] == ' ') { at = at + 0x1; }

    if (arg[at] != LDG_STR_TERM && crosstalk_addr_parse(ctx, &arg[at], &len) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_FREEZE_USAGE_TEXT); return; }

    if (ctx->eda.memview.snap_mode == LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_FREEZE_USAGE_TEXT); return; }

    if (len == 0 || len > (uint64_t)CROSSTALK_CORE_FREEZE_LEN_MAX) { toast_push(ctx, CROSSTALK_CORE_FREEZE_USAGE_TEXT); return; }

    for (slot = 0; slot < ctx->freeze_cunt; slot = slot + 0x1) { if (ctx->freeze_addr[slot] == addr) { break; } }

    if (slot == ctx->freeze_cunt && ctx->freeze_cunt >= (uint32_t)CROSSTALK_CORE_FREEZE_MAX) { toast_push(ctx, CROSSTALK_CORE_FREEZE_FULL_TEXT); return; }

    if (crosstalk_eda_ic_fpga_mem_byte_rd(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, crosstalk_memview_sig_get(ctx), addr, ctx->freeze_val[slot], (uint32_t)len, &rd) != LDG_ERR_AOK || (uint64_t)rd < len)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview: freeze refused; addr: 0x%X64; len: %u64", addr, len);
        toast_push(ctx, CROSSTALK_CORE_FREEZE_USAGE_TEXT);

        return;
    }

    ctx->freeze_addr[slot] = addr;
    ctx->freeze_len[slot] = (uint32_t)len;

    if (slot == ctx->freeze_cunt) { ctx->freeze_cunt = ctx->freeze_cunt + 0x1; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview: freeze; addr: 0x%X64; bytes: %u64; held: %u32", addr, len, ctx->freeze_cunt);

    {
        char note[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;

        if (snprintf(note, sizeof(note), CROSSTALK_CORE_FREEZE_OK_FMT, addr, (uint32_t)len) > 0) { toast_push(ctx, note); }
    }
}

static void crosstalk_memview_freeze_step(crosstalk_ctx_t *ctx)
{
    uint64_t now_val[CROSSTALK_CORE_FREEZE_LEN_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t slot = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    for (slot = 0; slot < ctx->freeze_cunt; slot = slot + 0x1)
    {
        uint32_t rd = 0;
        uint32_t wrote = 0;
        uint32_t at = 0;
        uint8_t same = LDG_TRUTH_TRUE;

        if (crosstalk_eda_ic_fpga_mem_byte_rd(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, crosstalk_memview_sig_get(ctx), ctx->freeze_addr[slot], now_val, ctx->freeze_len[slot], &rd) != LDG_ERR_AOK || rd < ctx->freeze_len[slot]) { continue; }

        for (at = 0; at < ctx->freeze_len[slot]; at = at + 0x1)
        {
            if ((now_val[at] & (uint64_t)LDG_BYTE_MASK) == (ctx->freeze_val[slot][at] & (uint64_t)LDG_BYTE_MASK)) { continue; }

            same = LDG_TRUTH_FALSE;

            break;
        }

        if (same == LDG_TRUTH_TRUE) { continue; }

        if (crosstalk_memview_bytes_wr(ctx, ctx->freeze_addr[slot], ctx->freeze_val[slot], ctx->freeze_len[slot], &wrote) != LDG_ERR_AOK || wrote != ctx->freeze_len[slot]) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview freeze restore failed"); }
    }
}

static void crosstalk_memview_poke(crosstalk_ctx_t *ctx, const char *arg)
{
    char addr_txt[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t vals[CROSSTALK_CORE_MEMPOKE_VAL_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t addr = 0;
    uint32_t val_cunt = 0;
    uint32_t wrote = 0;
    uint32_t at = 0;
    uint32_t pos = 0;
    uint32_t ret = 0;
    uint8_t copy_is = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (arg[at] == ' ') { at = at + 0x1; }

    while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(addr_txt)) { addr_txt[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

    addr_txt[pos] = LDG_STR_TERM;

    if (addr_txt[0] == LDG_STR_TERM || crosstalk_addr_parse(ctx, addr_txt, &addr) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_MEMPOKE_USAGE_TEXT); return; }

    {
        char src_txt[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
        uint32_t src_at = at;
        uint32_t src_pos = 0;

        while (arg[src_at] == ' ') { src_at = src_at + 0x1; }

        while (arg[src_at] != LDG_STR_TERM && arg[src_at] != ' ' && src_pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(src_txt)) { src_txt[src_pos] = arg[src_at]; src_pos = src_pos + 0x1; src_at = src_at + 0x1; }

        src_txt[src_pos] = LDG_STR_TERM;

        if (src_pos != 0 && src_pos != (uint32_t)CROSSTALK_CORE_POKE_BYTE_TEXT_LEN)
        {
            uint64_t src = 0;
            uint32_t got = 0;

            if (crosstalk_addr_parse(ctx, src_txt, &src) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_MEMPOKE_USAGE_TEXT); return; }

            if (crosstalk_eda_ic_fpga_mem_byte_rd(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, crosstalk_memview_sig_get(ctx), src, vals, (uint32_t)CROSSTALK_CORE_POKE_COPY_BYTES, &got) != LDG_ERR_AOK || got < (uint32_t)CROSSTALK_CORE_POKE_COPY_BYTES)
            {
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview: poke src rd failed; src: 0x%X64", src);
                toast_push(ctx, CROSSTALK_CORE_MEMPOKE_FAIL_TEXT);

                return;
            }

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview: poke copy; src: 0x%X64; dst: 0x%X64", src, addr);

            val_cunt = (uint32_t)CROSSTALK_CORE_POKE_COPY_BYTES;
            copy_is = LDG_TRUTH_TRUE;
        }
    }

    while (copy_is != LDG_TRUTH_TRUE && arg[at] != LDG_STR_TERM && val_cunt < (uint32_t)CROSSTALK_CORE_MEMPOKE_VAL_CAP)
    {
        uint8_t hi = 0;
        uint8_t lo = 0;

        if (arg[at] == ' ') { at = at + 0x1; continue; }

        if (arg[at] != CROSSTALK_CORE_HEX_LEAD || (arg[at + LDG_STR_TERM_SIZE] != 'x' && arg[at + LDG_STR_TERM_SIZE] != 'X')) { toast_push(ctx, CROSSTALK_CORE_MEMPOKE_USAGE_TEXT); return; }

        at = at + LDG_STR_TERM_SIZE + LDG_STR_TERM_SIZE;

        if (ldg_char_hex_is(arg[at]) != LDG_TRUTH_TRUE || ldg_hex_to_nipple(arg[at], &hi) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_MEMPOKE_USAGE_TEXT); return; }

        at = at + 0x1;

        if (ldg_char_hex_is(arg[at]) != LDG_TRUTH_TRUE || ldg_hex_to_nipple(arg[at], &lo) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_MEMPOKE_USAGE_TEXT); return; }

        at = at + 0x1;

        vals[val_cunt] = (uint64_t)hi * (uint64_t)LDG_BASE_HEX + (uint64_t)lo;
        val_cunt = val_cunt + 0x1;
    }

    if (val_cunt == 0) { toast_push(ctx, CROSSTALK_CORE_MEMPOKE_USAGE_TEXT); return; }

    if (ctx->eda.memview.snap_mode == LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_MEMPOKE_FAIL_TEXT); return; }

    if (addr >= (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_IO_FIRST)
    {
        uint64_t end = 0x0;
        uint64_t qw_at = addr & ~((uint64_t)sizeof(uint64_t) - (uint64_t)0x1);

        if (LDG_UNLIKELY((uint64_t)val_cunt > UINT64_MAX - addr)) { toast_push(ctx, CROSSTALK_CORE_MEMPOKE_FAIL_TEXT); return; }

        end = addr + (uint64_t)val_cunt;

        while (qw_at < end)
        {
            uint64_t word = 0x0;
            uint32_t byte_at = 0;

            if (qw_at < addr || end - qw_at < sizeof(uint64_t))
            {
                ret = crosstalk_eda_ic_fpga_io_rd(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, qw_at, &word);
                if (ret != LDG_ERR_AOK) { break; }
            }

            for (byte_at = 0; byte_at < sizeof(uint64_t); byte_at = byte_at + 0x1)
            {
                uint64_t byte_addr = qw_at + (uint64_t)byte_at;
                uint32_t shift = byte_at * (uint32_t)LDG_BYTE_BITS;

                if (byte_addr < addr || byte_addr >= end) { continue; }

                word = (word & ~((uint64_t)LDG_BYTE_MASK << shift)) | ((vals[byte_addr - addr] & (uint64_t)LDG_BYTE_MASK) << shift);
            }

            ret = crosstalk_eda_ic_fpga_io_wr(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, qw_at, word);
            if (ret != LDG_ERR_AOK) { break; }

            if (end - qw_at <= sizeof(uint64_t)) { wrote = val_cunt; break; }

            qw_at += sizeof(uint64_t);
            wrote = (uint32_t)(qw_at - addr);
        }

        if (ret != LDG_ERR_AOK || wrote != val_cunt)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview: io poke refused; addr: 0x%X64; cunt: %u32; err: %u32", addr, val_cunt, ret);
            toast_push(ctx, ret == LDG_ERR_UNSUPPORTED ? CROSSTALK_CORE_MEMPOKE_IO_TEXT : CROSSTALK_CORE_MEMPOKE_FAIL_TEXT);
            return;
        }

        ctx->eda.memview.refresh_next = 0.0;

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview: io poke; addr: 0x%X64; bytes: %u32", addr, wrote);

        crosstalk_ach_once(ctx, CROSSTALK_GAME_PROGRESSION_FIRST_IO_POKE);

        {
            char note[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;

            if (snprintf(note, sizeof(note), CROSSTALK_CORE_MEMPOKE_OK_FMT, wrote) > 0) { toast_push(ctx, note); }
        }

        return;
    }

    ret = crosstalk_memview_bytes_wr(ctx, addr, vals, val_cunt, &wrote);

    if (ret != LDG_ERR_AOK || wrote != val_cunt)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview: poke refused; addr: 0x%X64; cunt: %u32; err: %u32", addr, val_cunt, ret);
        toast_push(ctx, CROSSTALK_CORE_MEMPOKE_FAIL_TEXT);
        return;
    }

    ctx->eda.memview.refresh_next = 0.0;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview: poke; addr: 0x%X64; bytes: %u32", addr, wrote);

    crosstalk_ach_once(ctx, CROSSTALK_GAME_PROGRESSION_FIRST_POKE);

    {
        char note[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;

        if (snprintf(note, sizeof(note), CROSSTALK_CORE_MEMPOKE_OK_FMT, wrote) > 0) { toast_push(ctx, note); }
    }
}

static void crosstalk_memview_open(crosstalk_ctx_t *ctx, const char *arg)
{
    char sig[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    char rest[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t addr = 0;
    uint32_t at = 0;
    uint32_t pos = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(sig)) { sig[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

    sig[pos] = LDG_STR_TERM;
    pos = 0;

    if (strcmp(sig, CROSSTALK_CORE_MEMVIEW_WATCH_TOK) == 0) { crosstalk_memview_watch(ctx, &arg[at]); return; }

    if (strcmp(sig, CROSSTALK_CORE_MEMVIEW_SNAP_TOK) == 0) { crosstalk_memview_bake_mount(ctx, &arg[at], LDG_TRUTH_FALSE); return; }

    while (arg[at] == ' ') { at = at + 0x1; }

    while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(rest)) { rest[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

    rest[pos] = LDG_STR_TERM;

    if (sig[0] == LDG_STR_TERM) { if (ldg_str_rbrcpy(sig, CROSSTALK_CORE_MEMVIEW_SIG_DEFAULT, (uint64_t)sizeof(sig)) != LDG_ERR_AOK) { return; } }

    ret = crosstalk_eda_ui_memview_bind(&ctx->eda.memview, &ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, sig);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview: bind failed; sig: %.63s; err: %u32", sig, ret);
        toast_push(ctx, CROSSTALK_CORE_MEMVIEW_BIND_FAIL_TEXT);
        return;
    }

    if (rest[0] == CROSSTALK_CORE_MEMVIEW_FOLLOW_MARK)
    {
        ret = crosstalk_eda_ui_memview_follow_set(&ctx->eda.memview, &rest[LDG_STR_TERM_SIZE]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview follow set failed"); }
    }
    else if (rest[0] != LDG_STR_TERM && crosstalk_addr_parse(ctx, rest, &addr) == LDG_ERR_AOK)
    {
        ret = crosstalk_eda_ui_memview_addr_set(&ctx->eda.memview, addr);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview addr set failed"); }
    }

    ret = crosstalk_eda_ui_memview_open_set(&ctx->eda.memview, LDG_TRUTH_TRUE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview open failed"); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview: bound; sig: %.63s; arg: %.63s", sig, rest);
    toast_push(ctx, CROSSTALK_CORE_MEMVIEW_OPEN_TEXT);
}

static void crosstalk_text_attach(crosstalk_ctx_t *ctx, const char *arg)
{
    char sig[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    char rest[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t base = (uint64_t)CROSSTALK_EDA_UI_DISPLAY_CRT_TEXT_BASE_DEFAULT;
    uint32_t at = 0;
    uint32_t pos = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(sig)) { sig[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

    sig[pos] = LDG_STR_TERM;
    pos = 0;

    while (arg[at] == ' ') { at = at + 0x1; }

    while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(rest)) { rest[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

    rest[pos] = LDG_STR_TERM;

    if (sig[0] == LDG_STR_TERM) { if (ldg_str_rbrcpy(sig, CROSSTALK_CORE_MEMVIEW_SIG_DEFAULT, (uint64_t)sizeof(sig)) != LDG_ERR_AOK) { return; } }

    if (rest[0] != LDG_STR_TERM && crosstalk_num_parse(rest, &base) != LDG_ERR_AOK) { base = (uint64_t)CROSSTALK_EDA_UI_DISPLAY_CRT_TEXT_BASE_DEFAULT; }

    ret = crosstalk_eda_ui_display_crt_text_bind(&ctx->eda.crt, &ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, sig, base);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crt: text bind failed; sig: %.63s; base: 0x%X64; err: %u32", sig, base, ret);
        toast_push(ctx, CROSSTALK_CORE_MEMVIEW_BIND_FAIL_TEXT);
        return;
    }

    ret = crosstalk_eda_ui_display_crt_kind_set(&ctx->eda.crt, (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_KIND_CGA);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crt kind set failed"); }

    ret = crosstalk_eda_ui_display_crt_open_set(&ctx->eda.crt, LDG_TRUTH_TRUE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crt open failed"); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crt: text bound; sig: %.63s; base: 0x%X64", sig, base);

    crosstalk_ach_once(ctx, CROSSTALK_GAME_PROGRESSION_FIRST_MONITOR);
    toast_push(ctx, CROSSTALK_CORE_TEXT_OK_TEXT);
}

static void crosstalk_gfx_attach(crosstalk_ctx_t *ctx, const char *arg)
{
    char tok[CROSSTALK_CORE_GFX_TOK_CUNT][CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t base = (uint64_t)CROSSTALK_CORE_GFX_BASE_DEFAULT;
    uint64_t gfx_w = (uint64_t)CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_W_MAX;
    uint64_t gfx_h = (uint64_t)CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_H_MAX;
    uint32_t at = 0;
    uint32_t tok_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    for (tok_idx = 0; tok_idx < (uint32_t)CROSSTALK_CORE_GFX_TOK_CUNT; tok_idx = tok_idx + 0x1)
    {
        uint32_t pos = 0;

        while (arg[at] == ' ') { at = at + 0x1; }

        while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)CROSSTALK_CORE_SOUND_ARG_CAP) { tok[tok_idx][pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

        tok[tok_idx][pos] = LDG_STR_TERM;
    }

    if (tok[0][0] == LDG_STR_TERM) { if (ldg_str_rbrcpy(tok[0], CROSSTALK_CORE_MEMVIEW_SIG_DEFAULT, (uint64_t)CROSSTALK_CORE_SOUND_ARG_CAP) != LDG_ERR_AOK) { return; } }

    if (tok[1][0] != LDG_STR_TERM && crosstalk_num_parse(tok[1], &base) != LDG_ERR_AOK) { base = (uint64_t)CROSSTALK_CORE_GFX_BASE_DEFAULT; }

    if (tok[1][0] == LDG_STR_TERM)
    {
        ret = crosstalk_eda_ui_display_crt_gfx_dev_bind(&ctx->eda.crt, &ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, tok[0]);
        if (ret == LDG_ERR_AOK)
        {
            if (crosstalk_eda_ui_display_crt_kind_set(&ctx->eda.crt, (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_KIND_VGA) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crt kind set failed"); }

            if (crosstalk_eda_ui_display_crt_open_set(&ctx->eda.crt, LDG_TRUTH_TRUE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crt open failed"); }

            toast_push(ctx, CROSSTALK_CORE_GFX_OK_TEXT);
            return;
        }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crt: display reported no geometry; using the given values; err: %u32", ret);
    }

    if (tok[2][0] != LDG_STR_TERM && crosstalk_num_parse(tok[2], &gfx_w) != LDG_ERR_AOK) { gfx_w = (uint64_t)CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_W_MAX; }

    if (tok[3][0] != LDG_STR_TERM && crosstalk_num_parse(tok[3], &gfx_h) != LDG_ERR_AOK) { gfx_h = (uint64_t)CROSSTALK_EDA_UI_DISPLAY_CRT_GFX_H_MAX; }

    ret = crosstalk_eda_ui_display_crt_gfx_bind(&ctx->eda.crt, &ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, tok[0], base, (uint32_t)gfx_w, (uint32_t)gfx_h);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crt: gfx bind failed; sig: %.63s; base: 0x%X64; w: %u64; h: %u64; err: %u32", tok[0], base, gfx_w, gfx_h, ret);
        toast_push(ctx, CROSSTALK_CORE_MEMVIEW_BIND_FAIL_TEXT);
        return;
    }

    ret = crosstalk_eda_ui_display_crt_kind_set(&ctx->eda.crt, (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_KIND_VGA);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crt kind set failed"); }

    ret = crosstalk_eda_ui_display_crt_open_set(&ctx->eda.crt, LDG_TRUTH_TRUE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crt open failed"); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crt: gfx bound; sig: %.63s; base: 0x%X64; w: %u64; h: %u64", tok[0], base, gfx_w, gfx_h);

    crosstalk_ach_once(ctx, CROSSTALK_GAME_PROGRESSION_FIRST_MONITOR);
    toast_push(ctx, CROSSTALK_CORE_GFX_OK_TEXT);
}

static void crosstalk_cgacast_attach(crosstalk_ctx_t *ctx, const char *arg)
{
    char tok[CROSSTALK_CORE_GFX_TOK_CUNT][CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t base = (uint64_t)CROSSTALK_CORE_GFX_BASE_DEFAULT;
    uint64_t cast_w = (uint64_t)CROSSTALK_CORE_CGACAST_W_DEFAULT;
    uint64_t cast_h = (uint64_t)CROSSTALK_CORE_CGACAST_H_DEFAULT;
    uint32_t at = 0;
    uint32_t tok_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    for (tok_idx = 0; tok_idx < (uint32_t)CROSSTALK_CORE_GFX_TOK_CUNT; tok_idx = tok_idx + 0x1)
    {
        uint32_t pos = 0;

        while (arg[at] == ' ') { at = at + 0x1; }

        while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)CROSSTALK_CORE_SOUND_ARG_CAP) { tok[tok_idx][pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

        tok[tok_idx][pos] = LDG_STR_TERM;
    }

    if (strncmp(tok[0], CROSSTALK_CORE_OFF_TOK, sizeof(tok[0])) == 0)
    {
        ret = crosstalk_eda_io_cast_stop(&ctx->eda.cast);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cast stop failed"); }

        toast_push(ctx, CROSSTALK_CORE_CGACAST_OFF_TEXT);
        return;
    }

    if (tok[0][0] == LDG_STR_TERM) { if (ldg_str_rbrcpy(tok[0], CROSSTALK_CORE_MEMVIEW_SIG_DEFAULT, (uint64_t)CROSSTALK_CORE_SOUND_ARG_CAP) != LDG_ERR_AOK) { return; } }

    if (tok[1][0] != LDG_STR_TERM && crosstalk_num_parse(tok[1], &base) != LDG_ERR_AOK) { base = (uint64_t)CROSSTALK_CORE_GFX_BASE_DEFAULT; }

    if (tok[2][0] != LDG_STR_TERM && crosstalk_num_parse(tok[2], &cast_w) != LDG_ERR_AOK) { cast_w = (uint64_t)CROSSTALK_CORE_CGACAST_W_DEFAULT; }

    if (tok[3][0] != LDG_STR_TERM && crosstalk_num_parse(tok[3], &cast_h) != LDG_ERR_AOK) { cast_h = (uint64_t)CROSSTALK_CORE_CGACAST_H_DEFAULT; }

    ret = crosstalk_eda_io_cast_start(&ctx->eda.cast, tok[0], base, (uint32_t)cast_w, (uint32_t)cast_h);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cast: start failed; sig: %.63s; base: 0x%X64; w: %u64; h: %u64; err: %u32", tok[0], base, cast_w, cast_h, ret);
        toast_push(ctx, CROSSTALK_CORE_CGACAST_FAIL_TEXT);
        return;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "cast: started; sig: %.63s; base: 0x%X64; w: %u64; h: %u64", tok[0], base, cast_w, cast_h);

    crosstalk_ach_once(ctx, CROSSTALK_GAME_PROGRESSION_FIRST_CAST);
    toast_push(ctx, CROSSTALK_CORE_CGACAST_OK_TEXT);
}

static void crosstalk_receptacle_attach(crosstalk_ctx_t *ctx, const char *arg)
{
    const crosstalk_hdl_exec_ctx_t *exec = 0x0;
    char data[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    char rdy[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    char ack[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    char tail[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t mode = (uint32_t)CROSSTALK_EDA_IO_RECEPTACLE_MODE_RAW;
    uint32_t at = 0;
    uint32_t pos = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(data)) { data[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

    data[pos] = LDG_STR_TERM;
    pos = 0;

    while (arg[at] == ' ') { at = at + 0x1; }

    while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(rdy)) { rdy[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

    rdy[pos] = LDG_STR_TERM;
    pos = 0;

    while (arg[at] == ' ') { at = at + 0x1; }

    while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(ack)) { ack[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

    ack[pos] = LDG_STR_TERM;
    pos = 0;

    while (arg[at] == ' ') { at = at + 0x1; }

    while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(tail)) { tail[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

    tail[pos] = LDG_STR_TERM;

    if (strcmp(ack, CROSSTALK_CORE_RECEPT_SCAN_TOK) == 0) { mode = (uint32_t)CROSSTALK_EDA_IO_RECEPTACLE_MODE_SCAN; ack[0] = LDG_STR_TERM; }
    else if (strcmp(tail, CROSSTALK_CORE_RECEPT_SCAN_TOK) == 0) { mode = (uint32_t)CROSSTALK_EDA_IO_RECEPTACLE_MODE_SCAN; }

    if (data[0] == LDG_STR_TERM || rdy[0] == LDG_STR_TERM) { toast_push(ctx, CROSSTALK_CORE_RECEPT_USAGE_TEXT); return; }

    {
        const crosstalk_hdl_abi_t *abi = 0x0;

        ret = crosstalk_probe_src_get(ctx, &exec, &abi);
        if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_PIPE_NO_DESIGN_TEXT); return; }
    }

    ret = crosstalk_eda_io_receptacle_attach(&ctx->eda.receptacle, CROSSTALK_CORE_TERM_SLOT, exec, data, rdy, ack, mode);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "receptacle: attach failed; data: %.63s; rdy: %.63s; ack: %.63s; err: %u32", data, rdy, ack, ret);
        toast_push(ctx, CROSSTALK_CORE_PIPE_NO_SIG_TEXT);
        return;
    }

    toast_push(ctx, CROSSTALK_CORE_RECEPT_OK_TEXT);
}

static void crosstalk_midi_attach(crosstalk_ctx_t *ctx, const char *arg)
{
    const crosstalk_hdl_exec_ctx_t *exec = 0x0;
    const char *name = 0x0;
    char pin[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    char dev_txt[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t dev = (uint64_t)CROSSTALK_CORE_MIDI_DEV_DEFAULT;
    uint64_t freq = 0;
    uint32_t port_cunt = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    ret = crosstalk_eda_io_midi_ports_refresh(&ctx->eda.midi);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "midi: port list failed; err: %u32", ret); }

    ret = crosstalk_eda_io_midi_port_cunt_get(&ctx->eda.midi, &port_cunt);
    if (ret != LDG_ERR_AOK) { return; }

    for (idx = 0; idx < port_cunt; idx = idx + 0x1)
    {
        if (crosstalk_eda_io_midi_port_name_get(&ctx->eda.midi, idx, &name) != LDG_ERR_AOK) { continue; }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "midi: device; idx: %u32; name: %.63s", idx, name);
    }

    ret = crosstalk_tail_split(arg, pin, (uint32_t)sizeof(pin), dev_txt, (uint32_t)sizeof(dev_txt));

    if (ret == LDG_ERR_NOT_FOUND) { if (snprintf(pin, sizeof(pin), CROSSTALK_CORE_MIDI_PIN_FMT, arg) < 0) { return; } }
    else if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_MIDI_USAGE_TEXT); return; }

    if (pin[0] == LDG_STR_TERM) { toast_push(ctx, CROSSTALK_CORE_MIDI_USAGE_TEXT); return; }

    if (dev_txt[0] != LDG_STR_TERM)
    {
        ret = crosstalk_num_parse(dev_txt, &dev);
        if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_MIDI_USAGE_TEXT); return; }
    }

    ret = crosstalk_eda_ic_fpga_slot_exec_get(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, &exec);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_PIPE_NO_DESIGN_TEXT); return; }

    ret = crosstalk_eda_ic_fpga_slot_freq_get(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, &freq);
    if (ret != LDG_ERR_AOK || freq == 0) { toast_push(ctx, CROSSTALK_CORE_PIPE_NO_DESIGN_TEXT); return; }

    {
        uint64_t domain_freq = 0;

        if (crosstalk_eda_clk_domain_freq_get(&ctx->eda.clk, ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT].clk_domain, &domain_freq) == LDG_ERR_AOK && domain_freq != 0) { freq = domain_freq; }
    }

    ret = crosstalk_eda_io_midi_attach(&ctx->eda.midi, CROSSTALK_CORE_TERM_SLOT, exec, pin, freq);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "midi: attach failed; pin: %.63s; err: %u32", pin, ret);
        toast_push(ctx, CROSSTALK_CORE_PIPE_NO_SIG_TEXT);
        return;
    }

    if (port_cunt == 0)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "midi: pin bound with no device; file and note input are live");
        toast_push(ctx, CROSSTALK_CORE_MIDI_OK_TEXT);

        return;
    }

    ret = crosstalk_eda_io_midi_dev_open(&ctx->eda.midi, (uint32_t)dev);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_MIDI_DEV_FAIL_TEXT); return; }

    toast_push(ctx, CROSSTALK_CORE_MIDI_OK_TEXT);
}

static void crosstalk_midi_play(crosstalk_ctx_t *ctx, const char *arg)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    if (arg[0] == LDG_STR_TERM)
    {
        ret = crosstalk_eda_io_midi_file_stop(&ctx->eda.midi);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "midi: stop failed; err: %u32", ret); }

        toast_push(ctx, CROSSTALK_CORE_MIDI_STOP_TEXT);

        return;
    }

    if (crosstalk_eda_io_midi_live_is(&ctx->eda.midi) != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_MIDI_NO_PIN_TEXT); return; }

    ret = crosstalk_eda_io_midi_file_play(&ctx->eda.midi, arg);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "midi: file refused; path: %.255s; err: %u32", arg, ret);
        toast_push(ctx, CROSSTALK_CORE_MIDI_FILE_FAIL_TEXT);

        return;
    }

    toast_push(ctx, CROSSTALK_CORE_MIDI_PLAY_TEXT);
}

static void crosstalk_midi_note(crosstalk_ctx_t *ctx, const char *arg)
{
    char note_txt[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    char vel_txt[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t note = 0;
    uint64_t vel = (uint64_t)CROSSTALK_EDA_IO_MIDI_VEL_DEFAULT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    if (crosstalk_eda_io_midi_live_is(&ctx->eda.midi) != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_MIDI_NO_PIN_TEXT); return; }

    ret = crosstalk_tail_split(arg, note_txt, (uint32_t)sizeof(note_txt), vel_txt, (uint32_t)sizeof(vel_txt));

    if (ret == LDG_ERR_NOT_FOUND) { if (snprintf(note_txt, sizeof(note_txt), CROSSTALK_CORE_MIDI_PIN_FMT, arg) < 0) { return; } }
    else if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_MIDI_NOTE_USAGE_TEXT); return; }

    if (crosstalk_num_parse(note_txt, &note) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_MIDI_NOTE_USAGE_TEXT); return; }

    if (vel_txt[0] != LDG_STR_TERM && crosstalk_num_parse(vel_txt, &vel) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_MIDI_NOTE_USAGE_TEXT); return; }

    ret = crosstalk_eda_io_midi_note_put(&ctx->eda.midi, (uint8_t)note, (uint8_t)vel);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_MIDI_NOTE_FAIL_TEXT); return; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "midi: note; note: %u64; vel: %u64", note, vel);
}

static void crosstalk_memfind(crosstalk_ctx_t *ctx, const char *arg)
{
    char text[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t at = 0;
    uint32_t pos = 0;
    uint32_t ret = 0;
    uint8_t ascii_is = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    if (arg[0] == LDG_STR_TERM) { toast_push(ctx, CROSSTALK_CORE_MEMFIND_USAGE_TEXT); return; }

    if (arg[0] == LDG_ASCII_QUOTE)
    {
        ascii_is = LDG_TRUTH_TRUE;
        at = LDG_STR_TERM_SIZE;
    }

    while (arg[at] != LDG_STR_TERM && arg[at] != LDG_ASCII_QUOTE && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(text))
    {
        text[pos] = arg[at];
        pos = pos + 0x1;
        at = at + 0x1;
    }

    text[pos] = LDG_STR_TERM;

    ret = crosstalk_eda_ui_memview_find_start(&ctx->eda.memview, text, ascii_is, 0x0, ctx->eda.memview.width);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview: find refused; text: %.63s; err: %u32", text, ret);
        toast_push(ctx, CROSSTALK_CORE_MEMFIND_FAIL_TEXT);
        return;
    }

    toast_push(ctx, CROSSTALK_CORE_MEMFIND_OK_TEXT);
}

static void crosstalk_pipe_attach(crosstalk_ctx_t *ctx, const char *port)
{
    const crosstalk_hdl_exec_ctx_t *exec = 0x0;
    const crosstalk_hdl_abi_t *abi = 0x0;
    uint32_t probe_handle = UINT32_MAX;
    uint32_t stream_handle = UINT32_MAX;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || port == 0x0)) { return; }

    if (crosstalk_eda_io_serial_attached_is(&ctx->eda.serial) != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_PIPE_NO_PORT_TEXT); return; }

    ret = crosstalk_probe_src_get(ctx, &exec, &abi);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_PIPE_NO_DESIGN_TEXT); return; }

    ret = crosstalk_eda_logic_probe_attach(&ctx->eda.logic_probe, CROSSTALK_CORE_TERM_SLOT, exec, abi, port, &probe_handle);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pipe: attach failed; port: %.63s; err: %u32", port, ret);
        toast_push(ctx, CROSSTALK_CORE_PIPE_NO_SIG_TEXT);
        return;
    }

    ret = crosstalk_eda_logic_stream_open(&ctx->eda.logic_stream, probe_handle, CROSSTALK_EDA_LOGIC_STREAM_DECODE_BYTE_PORT, &stream_handle);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_PIPE_NO_STREAM_TEXT); return; }

    ret = crosstalk_eda_logic_stream_sink_add(&ctx->eda.logic_stream, stream_handle, crosstalk_eda_io_serial_sink_cb, &ctx->eda.serial);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_PIPE_NO_SINK_TEXT); return; }

    ctx->pipe_handle = stream_handle;
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "pipe: attached; port: %.63s; probe: %u32; stream: %u32; wire: %.63s", port, probe_handle, stream_handle, ctx->eda.serial.open_path);
    toast_push(ctx, CROSSTALK_CORE_PIPE_ATTACH_TEXT);
}

static void crosstalk_serial_attach(crosstalk_ctx_t *ctx, const char *path)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || path == 0x0)) { return; }

    if (crosstalk_eda_io_serial_attached_is(&ctx->eda.serial) == LDG_TRUTH_TRUE)
    {
        if (crosstalk_eda_io_serial_remote_is(&ctx->eda.serial) != LDG_TRUTH_TRUE) { crosstalk_serial_access_ctl_send(ctx, (uint32_t)CROSSTALK_NET_WIRE_SERIAL_CTL_REVOKE); }

        ret = crosstalk_eda_io_serial_detach(&ctx->eda.serial);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "serial detach failed"); }

        ctx->serial_access_owner = 0;
        ctx->serial_access_last = 0.0;
    }

    ctx->serial_map_cunt = 0;
    ctx->serial_map_window_len = 0;
    ctx->serial_map_pending_has = LDG_TRUTH_FALSE;

    ret = crosstalk_eda_io_serial_attach(&ctx->eda.serial, path);
    if (ret != LDG_ERR_AOK) { crosstalk_serial_lanes_release(ctx); toast_push(ctx, CROSSTALK_CORE_SERIAL_OPEN_FAIL_TEXT); return; }

    crosstalk_serial_lanes_attach(ctx);

    ctx->serial_access_grant_next = 0.0;

    {
        uint32_t serial_sesh = UINT32_MAX;

        if (crosstalk_eda_ui_term_sesh_src_find(&ctx->eda.term, CROSSTALK_EDA_UI_TERM_SRC_SERIAL, &serial_sesh) != LDG_ERR_AOK) { if (crosstalk_eda_ui_term_sesh_next_get(&ctx->eda.term, &serial_sesh) != LDG_ERR_AOK) { toast_push(ctx, "no free terminal tab"); return; } }

        ret = crosstalk_eda_ui_term_sesh_clear(&ctx->eda.term, serial_sesh);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term session clear failed"); }

        ret = crosstalk_eda_ui_term_sesh_bind(&ctx->eda.term, serial_sesh, CROSSTALK_EDA_UI_TERM_SRC_SERIAL, 0x0, path);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term session bind failed"); }

        ret = crosstalk_eda_ui_term_active_set(&ctx->eda.term, serial_sesh);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term active set failed"); }
    }

    ret = crosstalk_eda_ui_term_focus_set(&ctx->eda.term, LDG_TRUTH_TRUE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term focus failed"); }

    ret = crosstalk_eda_ui_term_open_set(&ctx->eda.term, LDG_TRUTH_TRUE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term open failed"); }

    toast_push(ctx, CROSSTALK_CORE_SERIAL_ATTACH_TEXT);
}

static void crosstalk_serial_list_log(crosstalk_ctx_t *ctx)
{
    const ldg_io_serial_port_t *port = 0x0;
    uint64_t cunt = 0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (crosstalk_eda_io_serial_ports_get(&ctx->eda.serial, &port, &cunt) != LDG_ERR_AOK) { return; }

    if (cunt == 0) { toast_push(ctx, CROSSTALK_CORE_SERIAL_NONE_TEXT); return; }

    for (idx = 0; idx < cunt; idx++) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "serial: port; path: %.63s; id: %.191s", port[idx].path, port[idx].id); }

    toast_push(ctx, CROSSTALK_CORE_SERIAL_LISTED_TEXT);
}

static void crosstalk_notice_raise(crosstalk_ctx_t *ctx, const char *title, const char *body)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || title == 0x0 || body == 0x0)) { return; }

    if (ldg_sys_focus_is() == LDG_TRUTH_TRUE)
    {
        toast_push(ctx, body);
        return;
    }

    ret = ldg_sys_notify(title, body, LDG_SYS_NOTIFY_LEVEL_LOW);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "crosstalk: notify failed; err: %u32", ret); }
}

static void sesh_data_cb(ldg_net_ctx_t *net, uint32_t conn_id, const void *data, uint32_t len)
{
    crosstalk_ctx_t *ctx = 0x0;
    crosstalk_net_wire_hdr_t hdr = LDG_STRUCT_ZERO_INIT;
    const uint8_t *payload = 0x0;
    uint8_t relay_reliable = CROSSTALK_NET_WIRE_RELIABLE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!net || !net->cb || !data)) { return; }

    ctx = (crosstalk_ctx_t *)net->cb->user;
    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    ret = crosstalk_net_wire_decode(data, len, &hdr, &payload);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "net: frame rejected; err: %u32; conn: %u32; len: %u32", ret, conn_id, len);
        return;
    }

    if (hdr.sender == ldg_net_steamworks_local_steam_id_get()) { return; }

    if (net->mode == LDG_NET_MODE_SRV)
    {
        relay_reliable = (uint8_t)(hdr.kind == CROSSTALK_NET_WIRE_KIND_POSE ? CROSSTALK_NET_WIRE_UNRELIABLE : CROSSTALK_NET_WIRE_RELIABLE);
        ret = crosstalk_net_wire_relay(&ctx->steam_net, data, len, relay_reliable);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "net: relay failed; err: %u32; kind: %u32", ret, (uint32_t)hdr.kind); }
    }

    if (hdr.kind == CROSSTALK_NET_WIRE_KIND_PEER_STATE && hdr.len == sizeof(crosstalk_net_wire_peer_state_t))
    {
        crosstalk_net_wire_peer_state_t peer_in = LDG_STRUCT_ZERO_INIT;

        memcpy(&peer_in, payload, sizeof(peer_in));
        peer_in.steam_id = hdr.sender;
        crosstalk_peer_state_apply(ctx, &peer_in);
    }

    if (hdr.kind == CROSSTALK_NET_WIRE_KIND_IDENT && hdr.len == sizeof(crosstalk_net_wire_ident_t))
    {
        crosstalk_net_wire_ident_t theirs = LDG_STRUCT_ZERO_INIT;
        uint32_t slot = UINT32_MAX;

        memcpy(&theirs, payload, sizeof(theirs));

        if (theirs.steam_id != 0)
        {
            for (uint32_t idx = 0; idx < ctx->ident_cunt; idx++) { if (ctx->ident[idx].steam_id == theirs.steam_id) { slot = idx; } }

            if (slot == UINT32_MAX && ctx->ident_cunt < (uint32_t)CROSSTALK_CORE_IDENT_CAP)
            {
                slot = ctx->ident_cunt;
                ctx->ident_cunt++;
            }

            if (slot != UINT32_MAX)
            {
                uint8_t learned = (uint8_t)((ctx->ident[slot].discord_id != theirs.discord_id && theirs.discord_id != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE);

                ctx->ident[slot] = theirs;
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "net: ident; steam: %u64; discord: %u64", theirs.steam_id, theirs.discord_id);

                if (learned == LDG_TRUTH_TRUE)
                {
                    ret = crosstalk_ident_cache_wr(ctx);

                    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "crosstalk: ident cache saved; steam: %u64; err: %u32", theirs.steam_id, ret);
                }
            }
        }
    }

    if (hdr.kind == CROSSTALK_NET_WIRE_KIND_SNAPSHOT_REQ && net->mode == LDG_NET_MODE_SRV)
    {
        ret = crosstalk_author_sync_send(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "net: author join send failed; err: %u32", ret); }
    }

    if (hdr.kind == CROSSTALK_NET_WIRE_KIND_CHAT && hdr.len == (uint32_t)sizeof(crosstalk_net_wire_chat_t))
    {
        crosstalk_net_wire_chat_t wire_chat = LDG_STRUCT_ZERO_INIT;
        char peer_name[CROSSTALK_GAME_CHAT_NAME_CAP] = LDG_ARR_ZERO_INIT;

        memcpy(&wire_chat, payload, sizeof(wire_chat));
        wire_chat.text[sizeof(wire_chat.text) - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
        ret = crosstalk_persona_resolve(ctx, hdr.sender, peer_name, (uint64_t)sizeof(peer_name));
        if (ret != LDG_ERR_AOK || peer_name[0] == LDG_STR_TERM) { ret = ldg_str_rbrcpy(peer_name, "peer", (uint64_t)sizeof(peer_name)); }

        ret = crosstalk_game_chat_push(&ctx->chat, (uint8_t)CROSSTALK_GAME_CHAT_GROUP_SESH, CROSSTALK_GAME_CHAT_PEER_NONE, CROSSTALK_CORE_CHAT_SESH_NAME, peer_name, wire_chat.text);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "net: chat push failed; err: %u32", ret); }

        {
            char notice[CROSSTALK_NET_NOTICE_CAP] = LDG_ARR_ZERO_INIT;

            snprintf(notice, sizeof(notice), "%.63s: %.63s", peer_name, wire_chat.text);
            crosstalk_notice_raise(ctx, CROSSTALK_CORE_NOTIFY_TITLE_SESH, notice);
        }
    }

    if (hdr.kind == CROSSTALK_NET_WIRE_KIND_SERIAL_CTL && hdr.len == (uint32_t)sizeof(crosstalk_net_wire_serial_ctl_t))
    {
        crosstalk_net_wire_serial_ctl_t ctl = LDG_STRUCT_ZERO_INIT;
        char peer_name[CROSSTALK_GAME_CHAT_NAME_CAP] = LDG_ARR_ZERO_INIT;
        char toast[CROSSTALK_CORE_SERIAL_TOAST_CAP] = LDG_ARR_ZERO_INIT;
        const char *tail = 0x0;

        memcpy(&ctl, payload, sizeof(ctl));
        ctl.id[sizeof(ctl.id) - LDG_STR_TERM_SIZE] = LDG_STR_TERM;

        if (crosstalk_eda_io_serial_id_tail_get(ctl.id, &tail) != LDG_ERR_AOK) { tail = ctl.id; }

        if (ctl.op == (uint32_t)CROSSTALK_NET_WIRE_SERIAL_CTL_GRANT)
        {
            if (crosstalk_eda_io_serial_remote_is(&ctx->eda.serial) == LDG_TRUTH_TRUE && ctx->serial_access_owner == hdr.sender) { ctx->serial_access_last = ctx->time; }
            else if (crosstalk_eda_io_serial_attached_is(&ctx->eda.serial) != LDG_TRUTH_TRUE)
            {
                ret = crosstalk_eda_io_serial_remote_attach(&ctx->eda.serial, ctl.id);
                if (ret == LDG_ERR_AOK)
                {
                    ctx->serial_access_owner = hdr.sender;
                    ctx->serial_access_last = ctx->time;

                    {
                        uint32_t serial_sesh = UINT32_MAX;

                        if (crosstalk_eda_ui_term_sesh_src_find(&ctx->eda.term, CROSSTALK_EDA_UI_TERM_SRC_SERIAL, &serial_sesh) != LDG_ERR_AOK) { if (crosstalk_eda_ui_term_sesh_next_get(&ctx->eda.term, &serial_sesh) != LDG_ERR_AOK) { serial_sesh = UINT32_MAX; } }

                        if (serial_sesh != UINT32_MAX)
                        {
                            ret = crosstalk_eda_ui_term_sesh_clear(&ctx->eda.term, serial_sesh);
                            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term session clear failed"); }

                            ret = crosstalk_eda_ui_term_sesh_bind(&ctx->eda.term, serial_sesh, CROSSTALK_EDA_UI_TERM_SRC_SERIAL, 0x0, tail);
                            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term session bind failed"); }

                            ret = crosstalk_eda_ui_term_open_set(&ctx->eda.term, LDG_TRUTH_TRUE);
                            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term open failed"); }
                        }
                    }

                    ret = crosstalk_persona_resolve(ctx, hdr.sender, peer_name, (uint64_t)sizeof(peer_name));
                    if (ret != LDG_ERR_AOK || peer_name[0] == LDG_STR_TERM) { ret = ldg_str_rbrcpy(peer_name, "peer", (uint64_t)sizeof(peer_name)); }

                    snprintf(toast, sizeof(toast), CROSSTALK_CORE_SERIAL_ACCESS_ON_FMT, peer_name, tail);
                    toast_push(ctx, toast);
                    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "serial: access granted; owner: %u64; id: %.191s", hdr.sender, ctl.id);
                }
            }
        }
        else if (ctl.op == (uint32_t)CROSSTALK_NET_WIRE_SERIAL_CTL_REVOKE)
        {
            if (crosstalk_eda_io_serial_remote_is(&ctx->eda.serial) == LDG_TRUTH_TRUE && ctx->serial_access_owner == hdr.sender)
            {
                ret = crosstalk_eda_io_serial_detach(&ctx->eda.serial);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "serial remote detach failed"); }

                ctx->serial_access_owner = 0;
                ctx->serial_access_last = 0.0;

                ret = crosstalk_persona_resolve(ctx, hdr.sender, peer_name, (uint64_t)sizeof(peer_name));
                if (ret != LDG_ERR_AOK || peer_name[0] == LDG_STR_TERM) { ret = ldg_str_rbrcpy(peer_name, "peer", (uint64_t)sizeof(peer_name)); }

                snprintf(toast, sizeof(toast), CROSSTALK_CORE_SERIAL_ACCESS_OFF_FMT, peer_name, tail);
                toast_push(ctx, toast);
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "serial: access revoked; owner: %u64", hdr.sender);
            }
        }
    }

    if (hdr.kind == CROSSTALK_NET_WIRE_KIND_SERIAL_DATA && hdr.len >= (uint32_t)CROSSTALK_NET_WIRE_SERIAL_DATA_HDR_SIZE && hdr.len <= (uint32_t)sizeof(crosstalk_net_wire_serial_data_t))
    {
        crosstalk_net_wire_serial_data_t sdata = LDG_STRUCT_ZERO_INIT;

        memcpy(&sdata, payload, hdr.len);

        if (sdata.len <= hdr.len - (uint32_t)CROSSTALK_NET_WIRE_SERIAL_DATA_HDR_SIZE && sdata.len > 0)
        {
            if (sdata.dir == (uint32_t)CROSSTALK_NET_WIRE_SERIAL_DIR_RX) { if (crosstalk_eda_io_serial_remote_is(&ctx->eda.serial) == LDG_TRUTH_TRUE && ctx->serial_access_owner == hdr.sender)
                {
                    ret = crosstalk_eda_io_serial_remote_rx_put(&ctx->eda.serial, sdata.bytes, sdata.len);
                    if (ret != LDG_ERR_AOK && ret != LDG_ERR_FULL) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "serial: remote rx put failed; err: %u32", ret); }
                }
            }
            else if (crosstalk_eda_io_serial_attached_is(&ctx->eda.serial) == LDG_TRUTH_TRUE && crosstalk_eda_io_serial_remote_is(&ctx->eda.serial) != LDG_TRUTH_TRUE)
            {
                ret = crosstalk_eda_io_serial_tx_put(&ctx->eda.serial, sdata.bytes, sdata.len);
                if (ret != LDG_ERR_AOK && ret != CROSSTALK_EDA_IO_SERIAL_ERR_TX_FULL) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "serial: session tx put failed; err: %u32", ret); }
            }
        }
    }

    if (hdr.kind == CROSSTALK_NET_WIRE_KIND_AUTHOR && net->mode != LDG_NET_MODE_SRV && hdr.len >= (uint32_t)sizeof(crosstalk_net_wire_author_t) && hdr.sender == crosstalk_net_steam_lobby_host_steam_id_get(&ctx->steam_net))
    {
        crosstalk_net_wire_author_t wire_author = LDG_STRUCT_ZERO_INIT;

        memcpy(&wire_author, payload, sizeof(wire_author));
        wire_author.name[sizeof(wire_author.name) - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
        wire_author.board_name[sizeof(wire_author.board_name) - LDG_STR_TERM_SIZE] = LDG_STR_TERM;

        ctx->game.author_steam_id = wire_author.steam_id;
        ctx->game.created_epoch_ms = wire_author.created_epoch_ms;
        ctx->game.bench_ms_total = wire_author.bench_ms_total;
        ctx->game.sim_ms_total = wire_author.sim_ms_total;
        ret = ldg_str_rbrcpy(ctx->game.author_name, wire_author.name, (uint64_t)sizeof(ctx->game.author_name));
        if (ret == LDG_ERR_AOK) { ret = ldg_str_rbrcpy(ctx->board_name, wire_author.board_name, (uint64_t)sizeof(ctx->board_name)); }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "net: author applied; err: %u32; name: %.63s; board: %.63s; steam_id: %u64", ret, ctx->game.author_name, ctx->board_name, ctx->game.author_steam_id);
    }
}

static void sesh_dc_cb(ldg_net_ctx_t *net, uint32_t conn_id)
{
    crosstalk_ctx_t *ctx = 0x0;
    uint64_t steam_id = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!net || !net->cb)) { return; }

    ctx = (crosstalk_ctx_t *)net->cb->user;
    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    steam_id = ldg_net_steamworks_client_steam_id_get(&ctx->net, conn_id);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "net: peer left; conn: %u32; steam_id: %u64", conn_id, steam_id);
    if (steam_id == 0) { return; }

    if (net->mode == LDG_NET_MODE_SRV)
    {
        ret = crosstalk_net_wire_send(&ctx->steam_net, CROSSTALK_NET_WIRE_KIND_BYE, &steam_id, (uint32_t)sizeof(steam_id), CROSSTALK_NET_WIRE_RELIABLE);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "net: bye send failed; err: %u32", ret); }
    }
}

static void crosstalk_clk_pin(crosstalk_ctx_t *ctx, const char *arg)
{
    uint64_t pin = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    if (arg[0] != LDG_STR_TERM && crosstalk_num_parse(arg, &pin) != LDG_ERR_AOK) { pin = 0; }

    ret = crosstalk_eda_ic_fpga_run_pin_set(&ctx->eda.fpga_run, pin);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_CLK_FAIL_TEXT); return; }

    if (pin != 0) { crosstalk_ach_once(ctx, CROSSTALK_GAME_PROGRESSION_FIRST_CLK_CONF); }

    toast_push(ctx, pin != 0 ? CROSSTALK_CORE_CLK_PIN_TEXT : CROSSTALK_CORE_CLK_FREE_TEXT);
}

static void crosstalk_chat_dock(crosstalk_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (crosstalk_game_hub_open_is(&ctx->hub) == LDG_TRUTH_TRUE)
    {
        crosstalk_hub_fit(ctx);
        return;
    }

    ctx->chat.win.rect_x = CROSSTALK_CORE_DESK_FRAC_ORIG;
    ctx->chat.win.rect_y = CROSSTALK_CORE_DESK_FRAC_FULL - ctx->chat.win.rect_h;
    if (ctx->chat.win.rect_y < CROSSTALK_CORE_DESK_FRAC_ORIG) { ctx->chat.win.rect_y = CROSSTALK_CORE_DESK_FRAC_ORIG; }
}

static void crosstalk_verb_st(crosstalk_ctx_t *ctx, const char *arg)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    ret = crosstalk_core_save_wr(ctx);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: cmdline st; err: %u32", ret);
    if (ret != LDG_ERR_AOK) { return; }

    crosstalk_event_fire(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_EVENT_ST);
}

static void crosstalk_verb_quit(crosstalk_ctx_t *ctx, const char *arg)
{
    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    if (arg[0] == 'f')
    {
        crosstalk_exit_event_run(ctx);
        crosstalk_menu_quit(ctx);

        return;
    }

    if (crosstalk_quit_stale_is(ctx) == LDG_TRUTH_TRUE)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: quit held; last save: %u64 ms", ctx->st_time_ms);
        toast_push(ctx, CROSSTALK_CORE_QUIT_GUARD_TEXT);

        return;
    }

    crosstalk_exit_event_run(ctx);
    crosstalk_menu_quit(ctx);
}

static void crosstalk_verb_st_quit(crosstalk_ctx_t *ctx, const char *arg)
{
    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    crosstalk_verb_st(ctx, arg);
    crosstalk_exit_event_run(ctx);
    crosstalk_menu_quit(ctx);
}

static void crosstalk_verb_exit(crosstalk_ctx_t *ctx, const char *arg)
{
    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    _Exit(CROSSTALK_CORE_QUIT_KILL_CODE);
}

static void crosstalk_verb_chat(crosstalk_ctx_t *ctx, const char *arg)
{
    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    if (ctx->chat.win.open == LDG_TRUTH_FALSE) { crosstalk_chat_dock(ctx); }

    if (crosstalk_game_chat_open_set(&ctx->chat, ctx->chat.win.open == LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cmdline chat toggle failed"); }
}

static void crosstalk_verb_totals(crosstalk_ctx_t *ctx, const char *arg)
{
    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    if (crosstalk_game_totals_open_set(&ctx->game.totals, ctx->game.totals.open == LDG_TRUTH_FALSE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cmdline totals toggle failed"); }
}

static uint8_t crosstalk_term_idx_parse(const char *arg, uint32_t *out)
{
    uint32_t val = 0;
    uint32_t at = 0;

    if (arg[0] == LDG_STR_TERM) { return LDG_TRUTH_FALSE; }

    while (arg[at] != LDG_STR_TERM)
    {
        if (ldg_char_digit_is(arg[at]) != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

        val = val * (uint32_t)LDG_BASE_DEC + ((uint32_t)arg[at] - 0x30);
        at = at + 0x1;
    }

    if (val >= (uint32_t)CROSSTALK_EDA_UI_TERM_SESH_MAX) { return LDG_TRUTH_FALSE; }

    *out = val;

    return LDG_TRUTH_TRUE;
}

static void crosstalk_term_tab_close(crosstalk_ctx_t *ctx, uint32_t sesh_idx)
{
    crosstalk_eda_ui_term_sesh_t *sesh_ptr = 0x0;
    uint32_t ret = crosstalk_eda_ui_term_sesh_ptr_get(&ctx->eda.term, sesh_idx, &sesh_ptr);

    if (ret != LDG_ERR_AOK || sesh_ptr->src == (uint32_t)CROSSTALK_EDA_UI_TERM_SRC_NONE) { toast_push(ctx, "no such terminal tab"); return; }

    if (sesh_ptr->src == (uint32_t)CROSSTALK_EDA_UI_TERM_SRC_STREAM) { if (crosstalk_eda_logic_stream_close(&ctx->eda.logic_stream, sesh_ptr->src_handle) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term stream close failed"); } }

    if (crosstalk_eda_ui_term_sesh_close(&ctx->eda.term, sesh_idx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term tab close failed"); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "term: tab closed; tab: %u32", sesh_idx);
}

static void crosstalk_term_host_dir_enter(crosstalk_ctx_t *ctx, const char *dir, const char *file, uint8_t fresh)
{
    char keys[CROSSTALK_CORE_TERM_HOST_KEYS_CAP] = LDG_ARR_ZERO_INIT;
    uint8_t win_is = ldg_io_path_sep_get() == (uint8_t)CROSSTALK_CORE_TERM_HOST_WIN_SEP ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    const char *ls = win_is == LDG_TRUTH_TRUE ? CROSSTALK_CORE_TERM_HOST_LS_WIN : CROSSTALK_CORE_TERM_HOST_LS_UNIX;
    uint32_t wrote = 0;

    if (fresh != LDG_TRUTH_TRUE)
    {
        wrote = (uint32_t)snprintf(keys, sizeof(keys), win_is == LDG_TRUTH_TRUE ? CROSSTALK_CORE_TERM_HOST_CD_WIN_FMT : CROSSTALK_CORE_TERM_HOST_CD_UNIX_FMT, dir);
        if (wrote == 0 || wrote >= (uint32_t)sizeof(keys)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term: host cd refused; path exceeds the key buffer"); return; }

        if (crosstalk_eda_io_pty_keys_wr(&ctx->eda.pty, (const uint8_t *)keys, wrote) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term: host cd write failed"); return; }
    }

    if (crosstalk_eda_io_pty_keys_wr(&ctx->eda.pty, (const uint8_t *)ls, (uint32_t)strlen(ls)) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term: host list write failed"); return; }

    if (file[0] == LDG_STR_TERM) { return; }

    wrote = (uint32_t)snprintf(keys, sizeof(keys), win_is == LDG_TRUTH_TRUE ? CROSSTALK_CORE_TERM_HOST_EDIT_WIN_FMT : CROSSTALK_CORE_TERM_HOST_EDIT_UNIX_FMT, file);
    if (wrote == 0 || wrote >= (uint32_t)sizeof(keys)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term: host edit refused; name exceeds the key buffer"); return; }

    if (crosstalk_eda_io_pty_keys_wr(&ctx->eda.pty, (const uint8_t *)keys, wrote) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term: host edit write failed"); }
}

static void crosstalk_term_host_open(crosstalk_ctx_t *ctx, const char *dir, const char *file)
{
    uint32_t sesh_idx = UINT32_MAX;
    uint32_t ret = 0;
    uint8_t fresh = LDG_TRUTH_FALSE;

    if (ctx->console_exec_is != LDG_TRUTH_TRUE) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term: host refused; reason: not interactive"); toast_push(ctx, "the host shell is interactive only"); return; }

    if (crosstalk_eda_io_pty_live_is(&ctx->eda.pty) != LDG_TRUTH_TRUE)
    {
        fresh = LDG_TRUTH_TRUE;

        if (ctx->eda.pty.live == LDG_TRUTH_TRUE)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "term: host restart; exit: %u32", ctx->eda.pty.exit_code);
            if (crosstalk_eda_io_pty_close(&ctx->eda.pty) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty restart close failed"); }
        }

        ret = crosstalk_eda_io_pty_open(&ctx->eda.pty, 0x0, dir);

        if (ret == LDG_ERR_UNSUPPORTED) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term: host refused; reason: unsupported"); toast_push(ctx, "the host shell needs a newer Windows"); return; }

        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term: host open failed; err: %u32", ret); toast_push(ctx, "the host shell failed to start"); return; }
    }

    if (crosstalk_eda_ui_term_sesh_src_find(&ctx->eda.term, CROSSTALK_EDA_UI_TERM_SRC_PTY, &sesh_idx) != LDG_ERR_AOK)
    {
        if (crosstalk_eda_ui_term_sesh_next_get(&ctx->eda.term, &sesh_idx) != LDG_ERR_AOK) { toast_push(ctx, "no free terminal tab"); return; }

        ret = crosstalk_eda_ui_term_sesh_bind(&ctx->eda.term, sesh_idx, CROSSTALK_EDA_UI_TERM_SRC_PTY, 0x0, ctx->eda.pty.title);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term host bind failed"); }
    }

    if (crosstalk_eda_ui_term_active_set(&ctx->eda.term, sesh_idx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term active set failed"); }

    if (crosstalk_eda_ui_term_open_set(&ctx->eda.term, LDG_TRUTH_TRUE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term open failed"); }

    if (crosstalk_eda_ui_term_focus_set(&ctx->eda.term, LDG_TRUTH_TRUE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term focus failed"); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "term: host shell; tab: %u32; pid: %u32", sesh_idx, ctx->eda.pty.pty.pid);

    if (dir[0] != LDG_STR_TERM) { crosstalk_term_host_dir_enter(ctx, dir, file, fresh); }
}

static void crosstalk_verb_term(crosstalk_ctx_t *ctx, const char *arg)
{
    uint32_t sesh_idx = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    if (memcmp(arg, CROSSTALK_CORE_TERM_HOST_WORD, sizeof(CROSSTALK_CORE_TERM_HOST_WORD)) == 0)
    {
        crosstalk_term_host_open(ctx, "", "");

        return;
    }

    if (memcmp(arg, CROSSTALK_CORE_TERM_DUMP_WORD, sizeof(CROSSTALK_CORE_TERM_DUMP_WORD)) == 0)
    {
        if (crosstalk_eda_io_pty_grid_dump(&ctx->eda.pty) != LDG_ERR_AOK) { toast_push(ctx, "no live host shell to dump"); }

        return;
    }

    if (memcmp(arg, CROSSTALK_CORE_TERM_CLOSE_WORD, sizeof(CROSSTALK_CORE_TERM_CLOSE_WORD) - LDG_STR_TERM_SIZE) == 0)
    {
        const char *tail = arg + sizeof(CROSSTALK_CORE_TERM_CLOSE_WORD) - LDG_STR_TERM_SIZE;

        while (tail[0] == ' ') { tail = tail + 0x1; }

        if (crosstalk_term_idx_parse(tail, &sesh_idx) != LDG_TRUTH_TRUE) { sesh_idx = ctx->eda.term.sesh_active; }

        crosstalk_term_tab_close(ctx, sesh_idx);

        return;
    }

    if (crosstalk_term_idx_parse(arg, &sesh_idx) == LDG_TRUTH_TRUE)
    {
        if (crosstalk_eda_ui_term_active_set(&ctx->eda.term, sesh_idx) != LDG_ERR_AOK) { toast_push(ctx, "no such terminal tab"); return; }

        if (crosstalk_eda_ui_term_open_set(&ctx->eda.term, LDG_TRUTH_TRUE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term open failed"); }

        return;
    }

    if (arg[0] != LDG_STR_TERM)
    {
        crosstalk_term_attach(ctx, arg);

        return;
    }

    if (crosstalk_eda_ui_term_open_is(&ctx->eda.term) == LDG_TRUTH_TRUE)
    {
        if (crosstalk_eda_ui_term_open_set(&ctx->eda.term, LDG_TRUTH_FALSE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cmdline term close failed"); }

        return;
    }

    crosstalk_term_attach(ctx, CROSSTALK_CORE_TERM_PORT_DEFAULT);
}

static void crosstalk_verb_emit(crosstalk_ctx_t *ctx, const char *arg)
{
    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    crosstalk_pack_start(ctx);
}

static uint32_t crosstalk_emu_net_open(crosstalk_ctx_t *ctx)
{
    const crosstalk_hdl_abi_t *abi = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (crosstalk_emu_net_shutdown(&ctx->emu_net) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu net shutdown failed"); }

    if (ctx->emu_net_path[0] == LDG_STR_TERM || crosstalk_eda_ic_fpga_emu_live_is(&ctx->eda.fpga_emu) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = crosstalk_emu_net_init(&ctx->emu_net, ctx->emu_net_path);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (crosstalk_eda_ic_fpga_emu_abi_get(&ctx->eda.fpga_emu, &abi) == LDG_ERR_AOK && (abi->net_drain == 0x0 || abi->net_feed == 0x0)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: the core has no network exports; drain: %u32; feed: %u32", abi->net_drain != 0x0 ? 0x1 : 0x0, abi->net_feed != 0x0 ? 0x1 : 0x0); }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_emu_start(crosstalk_ctx_t *ctx, const char *path)
{
    const crosstalk_hdl_abi_t *abi = 0x0;
    uint64_t osc_hz = (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_CLK_HZ;
    uint8_t was_live = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    was_live = crosstalk_fpga_hlt(ctx);

    if (crosstalk_eda_ic_fpga_abi_set(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, 0x0) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu abi clear before attach failed"); }

    ret = crosstalk_eda_ic_fpga_emu_attach(&ctx->eda.fpga_emu, path);
    if (ret != LDG_ERR_AOK)
    {
        crosstalk_fpga_resume(ctx, was_live);

        return ret;
    }

    ret = crosstalk_eda_ic_fpga_emu_abi_get(&ctx->eda.fpga_emu, &abi);
    if (ret != LDG_ERR_AOK)
    {
        crosstalk_fpga_resume(ctx, was_live);

        return ret;
    }

    if (crosstalk_core_watch_set_disarm(&ctx->design_watch) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "design watch: disarm failed"); }

    ret = crosstalk_eda_ic_fpga_emu_engage(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, abi, CROSSTALK_CORE_EMU_CLK_NAME, osc_hz);
    if (ret != LDG_ERR_AOK)
    {
        if (crosstalk_eda_ic_fpga_emu_detach(&ctx->eda.fpga_emu) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu detach after refusal failed"); }

        crosstalk_fpga_resume(ctx, was_live);

        return ret;
    }

    {
        uint32_t domain_idx = 0;

        if (crosstalk_eda_clk_domain_add(&ctx->eda.clk, 0x0, osc_hz, &domain_idx) == LDG_ERR_AOK) { if (crosstalk_eda_ic_fpga_clk_domain_set(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, domain_idx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu clk bind failed"); } }
        else { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu clk domain add failed"); }
    }

    crosstalk_fpga_in_ports_carry(ctx);

    ctx->prof_en_on = LDG_TRUTH_FALSE;

    crosstalk_fpga_resume(ctx, LDG_TRUTH_TRUE);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu: engaged; path: %.127s; hz: %u64", ctx->eda.fpga_emu.path, osc_hz);
    crosstalk_ach_once(ctx, CROSSTALK_GAME_PROGRESSION_FIRST_EMU);

    if (crosstalk_emu_net_open(ctx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: network stays detached; path: %.127s", ctx->emu_net_path); }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_emu_power_cycle(crosstalk_ctx_t *ctx)
{
    const crosstalk_eda_ic_fpga_mem_watch_t *watch = 0x0;
    char img_path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char sig[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t word_off = 0;
    uint64_t words = 0;
    uint64_t bytes = 0;
    uint32_t imgs = 0;
    uint32_t fail_cunt = 0;
    uint32_t at = 0;
    uint8_t was_live = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT].emu != LDG_TRUTH_TRUE || crosstalk_eda_ic_fpga_emu_live_is(&ctx->eda.fpga_emu) != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    was_live = crosstalk_fpga_hlt(ctx);

    ret = crosstalk_eda_ic_fpga_emu_rst(&ctx->eda.fpga_emu);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: power cycle rst failed; err: %u32", ret);

        crosstalk_fpga_resume(ctx, was_live);

        return ret;
    }

    if (ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT].hlt_mask != 0) { if (crosstalk_eda_ic_fpga_hlt_mask_set(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT].hlt_mask) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: hlt mask reapply failed; mask: 0x%X32", ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT].hlt_mask); } }

    if (crosstalk_eda_ic_fpga_prof_en(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, ctx->prof_en_on) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu ring reapply after rst failed"); }

    crosstalk_fpga_in_ports_carry(ctx);

    for (at = 0; crosstalk_eda_ic_fpga_mem_watch_get(&ctx->eda.fpga, at, &watch) == LDG_ERR_AOK && watch != 0x0; at = at + 0x1)
    {
        if (watch->slot_idx != (uint32_t)CROSSTALK_CORE_TERM_SLOT) { continue; }

        if (ldg_str_rbrcpy(sig, watch->sig, (uint64_t)sizeof(sig)) != LDG_ERR_AOK) { fail_cunt = fail_cunt + 0x1; continue; }

        if (ldg_str_rbrcpy(img_path, watch->path, (uint64_t)sizeof(img_path)) != LDG_ERR_AOK) { fail_cunt = fail_cunt + 0x1; continue; }

        word_off = watch->word_off;

        ret = crosstalk_eda_ic_fpga_mem_ld(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, sig, img_path, word_off, &words, &bytes);
        if (ret != LDG_ERR_AOK)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: power cycle image reload failed; sig: %.63s; path: %.127s; err: %u32", sig, img_path, ret);

            fail_cunt = fail_cunt + 0x1;
            continue;
        }

        imgs = imgs + 0x1;
    }

    crosstalk_fpga_resume(ctx, was_live);

    if (crosstalk_eda_ui_stackview_rst(&ctx->eda.stackview) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "stackview rst failed"); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu: power cycled; images: %u32; failed: %u32; hlt mask: 0x%X32; live: %u32", imgs, fail_cunt, ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT].hlt_mask, (uint32_t)was_live);

    return fail_cunt != 0 ? LDG_ERR_IO_RD : LDG_ERR_AOK;
}

static uint32_t crosstalk_re_diff_side_run(crosstalk_ctx_t *ctx, uint32_t side_idx)
{
    const crosstalk_re_diff_side_t *side = 0x0;
    const crosstalk_hdl_abi_t *abi = 0x0;
    uint64_t *node_base = 0x0;
    uint64_t *mem_base = 0x0;
    uint64_t words = 0;
    uint64_t bytes = 0;
    uint64_t done = 0;
    uint32_t node_cunt = 0;
    uint32_t mem_cunt = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_re_diff_side_get(&ctx->re.diff, side_idx, &side);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_emu_power_cycle(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_eda_ic_fpga_mem_ld(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, side->sig, side->path, side->word_off, &words, &bytes);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re diff: artifact ld failed; side: %u32; path: %.127s; err: %u32", side_idx, side->path, ret); return ret; }

    ret = crosstalk_eda_ic_fpga_slot_abi_get(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, &abi);
    if (ret != LDG_ERR_AOK || abi == 0x0 || abi->ok != LDG_TRUTH_TRUE || abi->state_get == 0x0) { return LDG_ERR_NOT_INIT; }

    for (at = 0; at < ctx->re.diff.mask_cunt; at = at + 0x1)
    {
        uint32_t idx = abi->sig_find != 0x0 ? abi->sig_find(ctx->re.diff.mask_name[at]) : (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE;

        if (idx == (uint32_t)CROSSTALK_HDL_ABI_SIG_NONE) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re diff: mask signal unknown; name: %.63s", ctx->re.diff.mask_name[at]); }

        if (crosstalk_re_diff_mask_idx_set(&ctx->re.diff, at, idx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re diff mask idx set failed"); }
    }

    if (ctx->re.diff.func_mode == LDG_TRUTH_TRUE)
    {
        char reg[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
        uint64_t sp = ctx->re.diff.sp;
        uint64_t ram_top = ctx->eda.fpga_emu.pool_span < (uint64_t)CROSSTALK_EDA_IC_FPGA_RAM_WINDOW_TOP ? ctx->eda.fpga_emu.pool_span : (uint64_t)CROSSTALK_EDA_IC_FPGA_RAM_WINDOW_TOP;
        uint64_t sentinel = 0;
        uint32_t wrote = 0;
        uint32_t arg_at = 0;

        if (sp == (uint64_t)CROSSTALK_RE_DIFF_SP_NONE) { sp = ram_top - (uint64_t)CROSSTALK_CORE_RE_SP_SLACK; }

        sentinel = sp;

        for (arg_at = 0; arg_at < ctx->re.diff.arg_cunt; arg_at = arg_at + 0x1)
        {
            if (snprintf(reg, sizeof(reg), "dr%u", arg_at) < 0) { return LDG_ERR_STR_FMT; }

            ret = crosstalk_eda_ic_fpga_sig_poke(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, reg, ctx->re.diff.arg[arg_at], LDG_TRUTH_TRUE);
            if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re diff: arg poke failed; reg: %.15s; err: %u32", reg, ret); return ret; }
        }

        if (snprintf(reg, sizeof(reg), "dr%u", (uint32_t)LDG_DANGCSA_SP) < 0) { return LDG_ERR_STR_FMT; }

        ret = crosstalk_eda_ic_fpga_sig_poke(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, reg, sp, LDG_TRUTH_TRUE);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re diff: sp poke failed; err: %u32", ret); return ret; }

        ret = crosstalk_eda_ic_fpga_mem_wr(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, side->sig, sp / (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_ELEM_BYTES, &sentinel, 0x1, &wrote);
        if (ret != LDG_ERR_AOK || wrote != 0x1) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re diff: sentinel write failed; sp: 0x%X64; err: %u32", sp, ret); return ret != LDG_ERR_AOK ? ret : LDG_ERR_IO_WR; }

        ret = crosstalk_eda_ic_fpga_sig_poke(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, CROSSTALK_EDA_IC_FPGA_PROG_SIG, ctx->re.diff.entry[side_idx], LDG_TRUTH_TRUE);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re diff: entry poke failed; err: %u32", ret); return ret; }

        ret = crosstalk_eda_ic_fpga_brk_set(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, sentinel, LDG_TRUTH_TRUE);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re diff: sentinel break failed; err: %u32", ret); return ret; }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "re diff: function call staged; side: %u32; entry: 0x%X64; sp: 0x%X64; args: %u32", side_idx, ctx->re.diff.entry[side_idx], sp, ctx->re.diff.arg_cunt);
    }

    ret = crosstalk_eda_ic_fpga_cycles_run(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, ctx->re.diff.cycles, &done);

    if (ctx->re.diff.func_mode == LDG_TRUTH_TRUE)
    {
        uint64_t sp = ctx->re.diff.sp != (uint64_t)CROSSTALK_RE_DIFF_SP_NONE ? ctx->re.diff.sp : (ctx->eda.fpga_emu.pool_span < (uint64_t)CROSSTALK_EDA_IC_FPGA_RAM_WINDOW_TOP ? ctx->eda.fpga_emu.pool_span : (uint64_t)CROSSTALK_EDA_IC_FPGA_RAM_WINDOW_TOP) - (uint64_t)CROSSTALK_CORE_RE_SP_SLACK;

        if (crosstalk_eda_ic_fpga_brk_set(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, sp, LDG_TRUTH_FALSE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re diff sentinel break clear failed"); }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "re diff: function returned or stopped; side: %u32; cycles: %u64; halted: %u32; err: %u32", side_idx, done, (uint32_t)crosstalk_eda_ic_fpga_slot_hlt_is(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT), ret);

        if (crosstalk_eda_ic_fpga_slot_hlt_set(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, LDG_TRUTH_FALSE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re diff hlt clear failed"); }
    }

    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re diff: run failed; side: %u32; cycles: %u64; done: %u64; err: %u32", side_idx, ctx->re.diff.cycles, done, ret); return ret; }

    if (abi->state_get((uint32_t)CROSSTALK_HDL_ABI_STATE_NODE, &node_base, &node_cunt) != LDG_ERR_AOK) { node_base = 0x0; node_cunt = 0; }

    if (abi->state_get((uint32_t)CROSSTALK_HDL_ABI_STATE_MEM, &mem_base, &mem_cunt) != LDG_ERR_AOK) { mem_base = 0x0; mem_cunt = 0; }

    return crosstalk_re_diff_snap_take(&ctx->re.diff, side_idx, node_base, node_cunt, mem_base, mem_cunt, ctx->eda.fpga_emu.pool, ctx->eda.fpga_emu.pool_span, side->word_off * (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_ELEM_BYTES, bytes, done);
}

static void crosstalk_re_diff_run(crosstalk_ctx_t *ctx)
{
    const char *verdict = 0x0;
    uint32_t ret = 0;
    uint8_t was_live = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (crosstalk_re_diff_ready_is(&ctx->re.diff) != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_RE_DIFF_NOT_READY_TEXT); return; }

    if (ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT].emu != LDG_TRUTH_TRUE || crosstalk_eda_ic_fpga_emu_live_is(&ctx->eda.fpga_emu) != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_RE_DIFF_NO_EMU_TEXT); return; }

    was_live = crosstalk_fpga_hlt(ctx);

    ret = crosstalk_re_diff_side_run(ctx, (uint32_t)CROSSTALK_RE_DIFF_SIDE_REF);

    if (ret == LDG_ERR_AOK) { ret = crosstalk_re_diff_side_run(ctx, (uint32_t)CROSSTALK_RE_DIFF_SIDE_CAND); }

    if (ret == LDG_ERR_AOK) { ret = crosstalk_re_diff_verdict_compute(&ctx->re.diff); }

    if (crosstalk_emu_power_cycle(ctx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re diff power cycle after run failed"); }

    crosstalk_fpga_resume(ctx, was_live);

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re diff: failed; err: %u32", ret);

        toast_push(ctx, CROSSTALK_CORE_RE_DIFF_FAIL_TEXT);

        return;
    }

    if (crosstalk_re_diff_verdict_name_get(ctx->re.diff.verdict, &verdict) != LDG_ERR_AOK) { verdict = CROSSTALK_CORE_EMU_OSC_NONE_TEXT; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "re diff: verdict: %s; cycles: %u64; ref: %.127s; cand: %.127s; nodes differing: %u32; first node: %u32; ram pages differing: %u32; first page: %u32; first page addr: 0x%X64; design mem differs: %u32; masks: %u32", verdict, ctx->re.diff.cycles, ctx->re.diff.side[CROSSTALK_RE_DIFF_SIDE_REF].path, ctx->re.diff.side[CROSSTALK_RE_DIFF_SIDE_CAND].path, ctx->re.diff.node_diff_cunt, ctx->re.diff.node_first, ctx->re.diff.page_diff_cunt, ctx->re.diff.page_first, (uint64_t)ctx->re.diff.page_first * (uint64_t)CROSSTALK_RE_DIFF_PAGE_BYTES, (uint32_t)ctx->re.diff.mem_diff, ctx->re.diff.mask_cunt);

    if (ctx->re.diff.node_first != UINT32_MAX) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "re diff: first node; idx: %u32; ref: 0x%016X64; cand: 0x%016X64", ctx->re.diff.node_first, ctx->re.diff.side[CROSSTALK_RE_DIFF_SIDE_REF].node[ctx->re.diff.node_first], ctx->re.diff.side[CROSSTALK_RE_DIFF_SIDE_CAND].node[ctx->re.diff.node_first]); }

    if (ctx->re.diff.func_mode == LDG_TRUTH_TRUE && ctx->re.diff.func[0] != LDG_STR_TERM)
    {
        crosstalk_re_db_rec_t rec = LDG_STRUCT_ZERO_INIT;

        rec.kind = (uint32_t)CROSSTALK_RE_DB_KIND_VERDICT;
        rec.by = (uint32_t)CROSSTALK_RE_DB_BY_TRACE;
        rec.addr = ctx->re.diff.entry[CROSSTALK_RE_DIFF_SIDE_REF];
        rec.size = (uint64_t)ctx->re.diff.verdict;

        if (ldg_str_rbrcpy(rec.text, ctx->re.diff.func, (uint64_t)sizeof(rec.text)) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re diff verdict name truncated"); }

        ret = crosstalk_re_db_rec_add(&ctx->re.db, &rec);

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "re diff: verdict recorded; func: %.63s; addr: 0x%X64; verdict: %s; err: %u32", ctx->re.diff.func, rec.addr, verdict, ret);
    }

    crosstalk_notice_raise(ctx, CROSSTALK_CORE_NOTIFY_TITLE_SESH, ctx->re.diff.verdict == (uint32_t)CROSSTALK_RE_DIFF_VERDICT_MATCH ? CROSSTALK_CORE_RE_DIFF_MATCH_TEXT : CROSSTALK_CORE_RE_DIFF_DIVERGED_TEXT);
}

static uint32_t crosstalk_hlt_cause_bit_get(const char *tok, uint32_t *out)
{
    uint32_t val = 0;

    if (LDG_UNLIKELY(tok == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (strcmp(tok, CROSSTALK_CORE_HLT_CAUSE_ALL) == 0)
    {
        for (val = (uint32_t)CROSSTALK_HDL_ABI_CAUSE_MIN; val <= (uint32_t)CROSSTALK_HDL_ABI_CAUSE_MAX; val = val + 0x1)
        {
            if (val == (uint32_t)CROSSTALK_HDL_ABI_CAUSE_UNALIGNED) { continue; }

            *out = *out | ((uint32_t)0x1 << val);
        }

        return LDG_ERR_AOK;
    }

    if (strcmp(tok, CROSSTALK_CORE_HLT_CAUSE_INVALID) == 0) { val = (uint32_t)CROSSTALK_HDL_ABI_CAUSE_INVALID; }
    else if (strcmp(tok, CROSSTALK_CORE_HLT_CAUSE_UNALIGNED) == 0) { val = (uint32_t)CROSSTALK_HDL_ABI_CAUSE_UNALIGNED; }
    else if (strcmp(tok, CROSSTALK_CORE_HLT_CAUSE_STACK_OVER) == 0) { val = (uint32_t)CROSSTALK_HDL_ABI_CAUSE_STACK_OVER; }
    else if (strcmp(tok, CROSSTALK_CORE_HLT_CAUSE_STACK_UNDER) == 0) { val = (uint32_t)CROSSTALK_HDL_ABI_CAUSE_STACK_UNDER; }
    else if (strcmp(tok, CROSSTALK_CORE_HLT_CAUSE_DIV_ZERO) == 0) { val = (uint32_t)CROSSTALK_HDL_ABI_CAUSE_DIV_ZERO; }
    else if (strcmp(tok, CROSSTALK_CORE_HLT_CAUSE_DR14) == 0) { val = (uint32_t)CROSSTALK_HDL_ABI_CAUSE_DR14; }
    else if (ldg_str_to_dec(tok, &val) != LDG_ERR_AOK) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (val < (uint32_t)CROSSTALK_HDL_ABI_CAUSE_MIN || val > (uint32_t)CROSSTALK_HDL_ABI_CAUSE_MAX) { return LDG_ERR_RANGE; }

    *out = (uint32_t)0x1 << val;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_hlt_mask_parse(const char *arg, uint32_t *out)
{
    char tok[CROSSTALK_CORE_HLT_CAUSE_TOK_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t bit = 0;
    uint32_t at = 0;
    uint32_t pos = 0;

    if (LDG_UNLIKELY(arg == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    while (arg[at] != LDG_STR_TERM)
    {
        while (arg[at] == ' ') { at = at + 0x1; }

        if (arg[at] == LDG_STR_TERM) { break; }

        pos = 0;

        while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)CROSSTALK_CORE_HLT_CAUSE_TOK_CAP)
        {
            tok[pos] = arg[at];
            pos = pos + 0x1;
            at = at + 0x1;
        }

        tok[pos] = LDG_STR_TERM;

        while (arg[at] != LDG_STR_TERM && arg[at] != ' ') { at = at + 0x1; }

        if (crosstalk_hlt_cause_bit_get(tok, &bit) != LDG_ERR_AOK) { return LDG_ERR_FUNC_ARG_INVALID; }

        *out = *out | bit;
    }

    return LDG_ERR_AOK;
}

static void crosstalk_dsu_stop(crosstalk_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (ctx->dsu_stream_handle != UINT32_MAX)
    {
        if (crosstalk_eda_logic_stream_close(&ctx->eda.logic_stream, ctx->dsu_stream_handle) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dsu stream close failed"); }

        ctx->dsu_stream_handle = UINT32_MAX;
    }

    if (ctx->dsu_probe_handle != UINT32_MAX)
    {
        if (crosstalk_eda_logic_probe_detach(&ctx->eda.logic_probe, ctx->dsu_probe_handle) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dsu probe detach failed"); }

        ctx->dsu_probe_handle = UINT32_MAX;
    }

    ctx->dsu_sample_at = 0x0;
    ctx->dsu_base = 0x0;
    ctx->dsu_tail = 0x0;
    ctx->dsu_curr = 0x0;
}

static uint32_t crosstalk_dsu_play(crosstalk_ctx_t *ctx)
{
    uint32_t probe_handle = UINT32_MAX;
    uint32_t rate = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    {
        uint64_t dsu[CROSSTALK_HDL_ABI_DSU_CUNT] = LDG_ARR_ZERO_INIT;

        ret = crosstalk_eda_ic_fpga_dsu_get(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, dsu, (uint32_t)CROSSTALK_HDL_ABI_DSU_CUNT);
        if (ret != LDG_ERR_AOK) { return LDG_ERR_UNSUPPORTED; }

        rate = (uint32_t)dsu[CROSSTALK_HDL_ABI_DSU_RATE_AT];
    }

    if (rate == 0) { return LDG_ERR_UNSUPPORTED; }

    crosstalk_dsu_stop(ctx);

    ret = crosstalk_eda_sound_rate_set(&ctx->eda.sound, rate);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_eda_logic_probe_host_attach(&ctx->eda.logic_probe, CROSSTALK_CORE_TERM_SLOT, CROSSTALK_CORE_DSU_PORT, (uint32_t)CROSSTALK_CORE_DSU_WIDTH, &probe_handle);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_eda_sound_bind(&ctx->eda.sound, CROSSTALK_CORE_DSU_PORT);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dsu sound bind failed"); }

    ret = crosstalk_eda_sound_stream_start(&ctx->eda.sound);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dsu sound stream start failed"); }

    ctx->dsu_probe_handle = probe_handle;
    ctx->dsu_sample_at = 0x0;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "dsu: routed; rate: %u32 Hz; width: %u32; probe: %u32", rate, (uint32_t)CROSSTALK_CORE_DSU_WIDTH, probe_handle);

    return LDG_ERR_AOK;
}

static uint8_t crosstalk_prof_want_is(const crosstalk_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_TRUTH_FALSE; }

    return ctx->prof_en_on == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static void crosstalk_prof_tick(crosstalk_ctx_t *ctx)
{
    uint64_t acc[CROSSTALK_EDA_UI_INDICATOR_ACC_CUNT] = LDG_ARR_ZERO_INIT;
    uint8_t want = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    want = crosstalk_prof_want_is(ctx);

    if (want != ctx->prof_on)
    {
        ret = crosstalk_eda_ic_fpga_prof_en_set(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, want);

        if (ret != LDG_ERR_AOK)
        {
            ctx->prof_on = LDG_TRUTH_FALSE;

            if (crosstalk_eda_ui_indicator_acc_clear(&ctx->eda.indicator) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "indicator acc clear failed"); }

            return;
        }

        ctx->prof_on = want;

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "prof: %s; consumers: %u32", want == LDG_TRUTH_TRUE ? "engaged" : "released", (uint32_t)want);

        if (want != LDG_TRUTH_TRUE)
        {
            if (crosstalk_eda_ui_indicator_acc_clear(&ctx->eda.indicator) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "indicator acc clear failed"); }

            return;
        }
    }

    if (ctx->prof_on != LDG_TRUTH_TRUE) { return; }

    if (crosstalk_eda_ic_fpga_mem_acc_get(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, acc, (uint32_t)CROSSTALK_EDA_UI_INDICATOR_ACC_CUNT) != LDG_ERR_AOK) { return; }

    if (crosstalk_eda_ui_indicator_acc_set(&ctx->eda.indicator, acc, (uint32_t)CROSSTALK_EDA_UI_INDICATOR_ACC_CUNT) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "indicator acc set failed"); }
}

static void crosstalk_dsu_pcm_feed(crosstalk_ctx_t *ctx, const uint8_t *pcm, uint32_t len)
{
    uint32_t depth = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || pcm == 0x0)) { return; }

    if (crosstalk_eda_sound_depth_get(&ctx->eda.sound, &depth) == LDG_ERR_AOK && depth >= (uint32_t)CROSSTALK_CORE_DSU_DEPTH_MAX) { return; }

    if (crosstalk_eda_sound_sink_cb(&ctx->eda.sound, pcm, len) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dsu sound sink failed"); }
}

static void crosstalk_dsu_span_push(crosstalk_ctx_t *ctx, const uint8_t *ram, uint64_t ram_bytes, uint64_t start, uint64_t end)
{
    uint8_t pcm[CROSSTALK_CORE_DSU_PCM_CHUNK * CROSSTALK_CORE_DSU_PCM_BYTES] = LDG_ARR_ZERO_INIT;
    uint64_t at = 0;
    uint64_t span = 0;
    uint32_t fill = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || ram == 0x0)) { return; }

    if (end <= start || start >= ram_bytes) { return; }

    if (end > ram_bytes) { end = ram_bytes; }

    span = end - start;
    if (span > (uint64_t)CROSSTALK_CORE_DSU_SPAN_MAX) { start = end - (uint64_t)CROSSTALK_CORE_DSU_SPAN_MAX; }

    for (at = start; at < end; at = at + 0x1)
    {
        uint16_t norm = (uint16_t)((uint16_t)((ram[at] ^ (uint8_t)CROSSTALK_CORE_DSU_SIGN_FLIP) & (uint8_t)LDG_BYTE_MASK) << (uint16_t)LDG_BYTE_BITS);

        if (crosstalk_eda_logic_probe_sample_push(&ctx->eda.logic_probe, ctx->dsu_probe_handle, (uint64_t)ram[at], ctx->dsu_sample_at) != LDG_ERR_AOK) { break; }

        ctx->dsu_sample_at = ctx->dsu_sample_at + 0x1;

        pcm[fill] = (uint8_t)(norm & (uint16_t)LDG_BYTE_MASK);
        pcm[fill + 0x1] = (uint8_t)((norm >> (uint16_t)LDG_BYTE_BITS) & (uint16_t)LDG_BYTE_MASK);
        fill = fill + (uint32_t)CROSSTALK_CORE_DSU_PCM_BYTES;

        if (fill < (uint32_t)sizeof(pcm)) { continue; }

        crosstalk_dsu_pcm_feed(ctx, pcm, fill);

        fill = 0;
    }

    if (fill == 0) { return; }

    crosstalk_dsu_pcm_feed(ctx, pcm, fill);
}

static uint8_t crosstalk_disk_persist_needed_is(const crosstalk_ctx_t *ctx)
{
    uint32_t drive = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (drive = 0; drive < (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX; drive = drive + 0x1) { if (crosstalk_eda_ic_fpga_emu_disk_live_is(&ctx->eda.fpga_emu, drive) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; } }

    return LDG_TRUTH_FALSE;
}

static void crosstalk_disk_persist(crosstalk_ctx_t *ctx)
{
    uint32_t drive = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    for (drive = 0; drive < (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX; drive = drive + 0x1)
    {
        if (crosstalk_eda_ic_fpga_emu_disk_live_is(&ctx->eda.fpga_emu, drive) != LDG_TRUTH_TRUE) { continue; }

        if (crosstalk_eda_ic_fpga_emu_disk_eject(&ctx->eda.fpga_emu, drive) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: persist failed; drive: %u32", drive); }
    }
}

static void crosstalk_dsu_tick(crosstalk_ctx_t *ctx)
{
    uint64_t dsu[CROSSTALK_HDL_ABI_DSU_CUNT] = LDG_ARR_ZERO_INIT;
    uint8_t pcm[CROSSTALK_CORE_DSU_PCM_CHUNK] = LDG_ARR_ZERO_INIT;
    uint64_t got = 0x0;
    uint64_t drained = 0x0;
    uint32_t ret = 0x0;
    const uint8_t *ram = 0x0;
    uint64_t *base_ptr = 0x0;
    uint64_t ram_bytes = 0;
    uint32_t ram_cunt = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (ctx->dsu_probe_handle == UINT32_MAX) { return; }

    while (drained < CROSSTALK_CORE_DSU_DRAIN_CAP)
    {
        ret = crosstalk_eda_ic_fpga_audio_drain(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, pcm, sizeof(pcm), &got);
        if (ret == LDG_ERR_UNSUPPORTED) { break; }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK || got > sizeof(pcm))) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dsu audio drain failed"); return; }

        if (got == 0x0) { return; }

        crosstalk_dsu_span_push(ctx, pcm, got, 0x0, got);
        drained += got;
        if (got < sizeof(pcm) || drained >= CROSSTALK_CORE_DSU_DRAIN_CAP) { return; }
    }

    if (crosstalk_eda_ic_fpga_dsu_get(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, dsu, (uint32_t)CROSSTALK_HDL_ABI_DSU_CUNT) != LDG_ERR_AOK) { return; }

    if (crosstalk_eda_ic_fpga_prof_get(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, (uint32_t)CROSSTALK_HDL_ABI_STATE_MEM, &base_ptr, &ram_cunt) != LDG_ERR_AOK || base_ptr == 0x0) { return; }

    ram = (const uint8_t *)(const void *)base_ptr;
    ram_bytes = (uint64_t)ram_cunt * (uint64_t)sizeof(uint64_t);

    if (dsu[CROSSTALK_HDL_ABI_DSU_BASE_AT] == 0)
    {
        if (ctx->dsu_base != 0) { crosstalk_dsu_span_push(ctx, ram, ram_bytes, ctx->dsu_curr, ctx->dsu_tail); }

        ctx->dsu_base = 0;
        ctx->dsu_tail = 0;
        ctx->dsu_curr = 0;

        return;
    }

    if (dsu[CROSSTALK_HDL_ABI_DSU_BASE_AT] != ctx->dsu_base)
    {
        ctx->dsu_base = dsu[CROSSTALK_HDL_ABI_DSU_BASE_AT];
        ctx->dsu_curr = ctx->dsu_base;
    }
    else if (dsu[CROSSTALK_HDL_ABI_DSU_CURR_AT] < ctx->dsu_curr)
    {
        crosstalk_dsu_span_push(ctx, ram, ram_bytes, ctx->dsu_curr, ctx->dsu_tail);

        ctx->dsu_curr = ctx->dsu_base;
    }

    ctx->dsu_tail = dsu[CROSSTALK_HDL_ABI_DSU_TAIL_AT];

    if (dsu[CROSSTALK_HDL_ABI_DSU_CURR_AT] > ctx->dsu_curr)
    {
        crosstalk_dsu_span_push(ctx, ram, ram_bytes, ctx->dsu_curr, dsu[CROSSTALK_HDL_ABI_DSU_CURR_AT]);

        ctx->dsu_curr = dsu[CROSSTALK_HDL_ABI_DSU_CURR_AT];
    }
}

static void crosstalk_verb_dsu(crosstalk_ctx_t *ctx, const char *arg)
{
    uint64_t dsu[CROSSTALK_HDL_ABI_DSU_CUNT] = LDG_ARR_ZERO_INIT;
    char line[CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (arg[at] == ' ') { at = at + 0x1; }

    if (strcmp(&arg[at], CROSSTALK_CORE_DSU_PLAY_TOK) == 0)
    {
        ret = crosstalk_dsu_play(ctx);
        if (ret != LDG_ERR_AOK)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dsu: route failed; err: %u32", ret);
            toast_push(ctx, ret == LDG_ERR_UNSUPPORTED ? CROSSTALK_CORE_DSU_NO_AUDIO_TEXT : CROSSTALK_CORE_DSU_NONE_TEXT);

            return;
        }

        toast_push(ctx, CROSSTALK_CORE_DSU_PLAY_TEXT);

        return;
    }

    if (strcmp(&arg[at], CROSSTALK_CORE_DSU_STOP_TOK) == 0)
    {
        crosstalk_dsu_stop(ctx);
        toast_push(ctx, CROSSTALK_CORE_DSU_STOP_TEXT);

        return;
    }

    if (crosstalk_eda_ic_fpga_dsu_get(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, dsu, (uint32_t)CROSSTALK_HDL_ABI_DSU_CUNT) != LDG_ERR_AOK)
    {
        toast_push(ctx, CROSSTALK_CORE_DSU_NONE_TEXT);

        return;
    }

    if (dsu[CROSSTALK_HDL_ABI_DSU_BASE_AT] == 0 && dsu[CROSSTALK_HDL_ABI_DSU_TAIL_AT] == 0)
    {
        toast_push(ctx, CROSSTALK_CORE_DSU_IDLE_TEXT);

        return;
    }

    if (snprintf(line, sizeof(line), CROSSTALK_CORE_DSU_FMT, dsu[CROSSTALK_HDL_ABI_DSU_BASE_AT], dsu[CROSSTALK_HDL_ABI_DSU_TAIL_AT], dsu[CROSSTALK_HDL_ABI_DSU_CURR_AT], dsu[CROSSTALK_HDL_ABI_DSU_RATE_AT], dsu[CROSSTALK_HDL_ABI_DSU_RUN_AT]) < 0) { return; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "%.191s", line);

    if (crosstalk_core_cmdline_out_push(&ctx->cmdline, line) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dsu report push failed"); }
}

static void crosstalk_verb_disk(crosstalk_ctx_t *ctx, const char *arg)
{
    char toast[CROSSTALK_CORE_EMU_TOAST_CAP] = LDG_ARR_ZERO_INIT;
    char path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t val = 0;
    uint64_t len = 0;
    uint64_t cut = UINT64_MAX;
    uint32_t drive = 0;
    uint32_t at = 0;
    uint8_t was_live = LDG_TRUTH_FALSE;
    uint8_t any = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (arg[at] == ' ') { at = at + 0x1; }

    if (ldg_str_rbrcpy(path, &arg[at], (uint64_t)sizeof(path)) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_DISK_USAGE_TEXT); return; }

    len = strlen(path);

    while (len != 0 && path[len - 0x1] == ' ') { len = len - 0x1; path[len] = LDG_STR_TERM; }

    for (val = len; val != 0; val = val - 0x1) { if (path[val - 0x1] == ' ') { cut = val - 0x1; break; } }

    if (cut != UINT64_MAX && crosstalk_num_parse(&path[cut + 0x1], &val) == LDG_ERR_AOK)
    {
        if (val >= (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX) { toast_push(ctx, CROSSTALK_CORE_DISK_DRIVE_BAD_TEXT); return; }

        drive = (uint32_t)val;
        len = cut;
        path[len] = LDG_STR_TERM;

        while (len != 0 && path[len - 0x1] == ' ') { len = len - 0x1; path[len] = LDG_STR_TERM; }
    }
    else if (cut == UINT64_MAX && len != 0 && crosstalk_num_parse(path, &val) == LDG_ERR_AOK)
    {
        if (val >= (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX) { toast_push(ctx, CROSSTALK_CORE_DISK_DRIVE_BAD_TEXT); return; }

        drive = (uint32_t)val;
        len = 0;
        path[0] = LDG_STR_TERM;

        if (crosstalk_eda_ic_fpga_emu_disk_live_is(&ctx->eda.fpga_emu, drive) != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_DISK_NONE_TEXT); return; }

        snprintf(toast, sizeof(toast), CROSSTALK_CORE_DISK_STATE_FMT, drive, ctx->eda.fpga_emu.disk_path[drive]);
        toast_push(ctx, toast);

        return;
    }

    if (path[0] == LDG_STR_TERM)
    {
        for (drive = 0; drive < (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX; drive = drive + 0x1)
        {
            if (crosstalk_eda_ic_fpga_emu_disk_live_is(&ctx->eda.fpga_emu, drive) != LDG_TRUTH_TRUE) { continue; }

            snprintf(toast, sizeof(toast), CROSSTALK_CORE_DISK_STATE_FMT, drive, ctx->eda.fpga_emu.disk_path[drive]);
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "disk: %.191s", toast);

            if (crosstalk_core_cmdline_out_push(&ctx->cmdline, toast) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "disk report push failed"); }

            any = LDG_TRUTH_TRUE;
        }

        if (any != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_DISK_NONE_TEXT); }

        return;
    }

    if (strcmp(path, CROSSTALK_CORE_DISK_SYNC_TOK) == 0)
    {
        was_live = crosstalk_fpga_hlt(ctx);

        ret = crosstalk_eda_ic_fpga_emu_disk_sync(&ctx->eda.fpga_emu, drive);

        crosstalk_fpga_resume(ctx, was_live);

        if (ret != LDG_ERR_AOK)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "disk: sync failed; drive: %u32; err: %u32", drive, ret);
            snprintf(toast, sizeof(toast), CROSSTALK_CORE_DISK_FAIL_FMT, drive, ret);
            toast_push(ctx, toast);

            return;
        }

        snprintf(toast, sizeof(toast), CROSSTALK_CORE_DISK_SYNC_FMT, drive);
        toast_push(ctx, toast);

        return;
    }

    if (strcmp(path, CROSSTALK_CORE_OFF_TOK) == 0)
    {
        if (crosstalk_eda_ic_fpga_emu_disk_eject(&ctx->eda.fpga_emu, drive) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "disk eject failed"); return; }

        snprintf(toast, sizeof(toast), CROSSTALK_CORE_DISK_OFF_FMT, drive);
        toast_push(ctx, toast);

        return;
    }

    was_live = crosstalk_fpga_hlt(ctx);

    ret = crosstalk_eda_ic_fpga_emu_disk_ld(&ctx->eda.fpga_emu, drive, path);

    crosstalk_fpga_resume(ctx, was_live);

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "disk: attach failed; drive: %u32; path: %.127s; err: %u32", drive, path, ret);
        snprintf(toast, sizeof(toast), CROSSTALK_CORE_DISK_FAIL_FMT, drive, ret);
        toast_push(ctx, toast);

        return;
    }

    snprintf(toast, sizeof(toast), CROSSTALK_CORE_DISK_OK_FMT, drive);
    toast_push(ctx, toast);
}

static void crosstalk_dlc_disks_attach(crosstalk_ctx_t *ctx)
{
    static const uint32_t app_id[CROSSTALK_CORE_DLC_DISK_CUNT] = CROSSTALK_CORE_DLC_DISK_APP_IDS;
    char dlc_dir[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char app_dir[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char app_name[CROSSTALK_CORE_DLC_DISK_APP_ID_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t drive = (uint32_t)CROSSTALK_CORE_DLC_DRIVE_FIRST;
    uint32_t idx = 0;
    uint8_t was_live = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (crosstalk_eda_ic_fpga_emu_live_is(&ctx->eda.fpga_emu) != LDG_TRUTH_TRUE) { return; }

    ret = ldg_io_path_join(ctx->res_dir, CROSSTALK_CORE_DLC_DISK_DIR_REL, dlc_dir, (uint64_t)sizeof(dlc_dir));
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dlc disk dir join failed"); return; }

    for (idx = 0; idx < (uint32_t)CROSSTALK_CORE_DLC_DISK_CUNT; idx = idx + 0x1)
    {
        if (ldg_net_steamworks_dlc_installed_is(app_id[idx]) != LDG_TRUTH_TRUE) { continue; }

        if (snprintf(app_name, sizeof(app_name), "%" PRIu32, app_id[idx]) < 0) { continue; }

        ret = ldg_io_path_join(dlc_dir, app_name, app_dir, (uint64_t)sizeof(app_dir));
        if (ret == LDG_ERR_AOK) { ret = ldg_io_path_join(app_dir, CROSSTALK_CORE_DLC_DISK_FILE, path, (uint64_t)sizeof(path)); }

        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "dlc disk path join failed"); continue; }

        if (ldg_io_path_exists_is(path) != LDG_TRUTH_TRUE)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: dlc installed but its disk is absent; app: %u32; path: %.191s", app_id[idx], path);

            continue;
        }

        while (drive < (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX && crosstalk_eda_ic_fpga_emu_disk_live_is(&ctx->eda.fpga_emu, drive) == LDG_TRUTH_TRUE) { drive = drive + 0x1; }

        if (drive >= (uint32_t)CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: no free drive for dlc disk; app: %u32", app_id[idx]);

            return;
        }

        was_live = crosstalk_fpga_hlt(ctx);

        ret = crosstalk_eda_ic_fpga_emu_disk_ld(&ctx->eda.fpga_emu, drive, path);

        crosstalk_fpga_resume(ctx, was_live);

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "crosstalk: dlc disk; app: %u32; drive: %u32; path: %.191s; err: %u32", app_id[idx], drive, path, ret);
    }
}

static void crosstalk_verb_on(crosstalk_ctx_t *ctx, const char *arg)
{
    char tok[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    char line[CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CAP] = LDG_ARR_ZERO_INIT;
    const char *path = 0x0;
    const char *name = 0x0;
    uint32_t event = 0;
    uint32_t at = 0;
    uint32_t pos = 0;
    uint32_t bound = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (arg[at] == ' ') { at = at + 0x1; }

    if (arg[at] == LDG_STR_TERM)
    {
        for (event = 0; event < (uint32_t)CROSSTALK_EDA_SCRIPT_EVENT_CUNT; event = event + 0x1)
        {
            if (crosstalk_eda_script_event_get(&ctx->eda.script, event, &path) != LDG_ERR_AOK) { continue; }

            if (crosstalk_eda_script_event_name_get(event, &name) != LDG_ERR_AOK) { continue; }

            if (snprintf(line, sizeof(line), CROSSTALK_CORE_EVENT_ROW_FMT, name, path) < 0) { continue; }

            if (crosstalk_core_cmdline_out_push(&ctx->cmdline, line) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "event list push failed"); }

            bound = bound + 0x1;
        }

        if (bound == 0) { toast_push(ctx, CROSSTALK_CORE_EVENT_NONE_TEXT); }

        return;
    }

    while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(tok)) { tok[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

    tok[pos] = LDG_STR_TERM;

    if (crosstalk_eda_script_event_id_get(tok, &event) != LDG_ERR_AOK)
    {
        toast_push(ctx, CROSSTALK_CORE_EVENT_USAGE_TEXT);

        return;
    }

    while (arg[at] == ' ') { at = at + 0x1; }

    if (arg[at] == LDG_STR_TERM)
    {
        if (crosstalk_eda_script_event_get(&ctx->eda.script, event, &path) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_EVENT_NONE_TEXT); return; }

        if (snprintf(line, sizeof(line), CROSSTALK_CORE_EVENT_FMT, tok, path) < 0) { return; }

        toast_push(ctx, line);

        return;
    }

    if (strcmp(&arg[at], CROSSTALK_CORE_OFF_TOK) == 0)
    {
        if (crosstalk_eda_script_event_set(&ctx->eda.script, event, 0x0) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "event clear failed"); return; }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "event: released; event: %.7s", tok);
        toast_push(ctx, CROSSTALK_CORE_EVENT_OFF_TEXT);

        return;
    }

    if (crosstalk_eda_script_event_set(&ctx->eda.script, event, &arg[at]) != LDG_ERR_AOK)
    {
        toast_push(ctx, CROSSTALK_CORE_EVENT_USAGE_TEXT);

        return;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "event: bound; event: %.7s; path: %.127s", tok, &arg[at]);
    toast_push(ctx, CROSSTALK_CORE_EVENT_ON_TEXT);
}

static void crosstalk_verb_hlt(crosstalk_ctx_t *ctx, const char *arg)
{
    uint8_t on = LDG_TRUTH_TRUE;
    uint32_t mask = 0;
    uint32_t ret = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (arg != 0x0) { while (arg[at] == ' ') { at = at + 0x1; } }

    if (arg != 0x0 && strcmp(&arg[at], CROSSTALK_CORE_OFF_TOK) == 0) { on = LDG_TRUTH_FALSE; }

    if (arg != 0x0 && arg[at] != LDG_STR_TERM && on == LDG_TRUTH_TRUE)
    {
        if (crosstalk_hlt_mask_parse(&arg[at], &mask) != LDG_ERR_AOK)
        {
            toast_push(ctx, CROSSTALK_CORE_HLT_CAUSE_BAD_TEXT);
            return;
        }

        ret = crosstalk_eda_ic_fpga_hlt_mask_set(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, mask);
        if (ret != LDG_ERR_AOK)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "hlt: mask refused; mask: 0x%X32; err: %u32", mask, ret);
            toast_push(ctx, ret == LDG_ERR_UNSUPPORTED ? CROSSTALK_CORE_HLT_MASK_NO_EMU_TEXT : CROSSTALK_CORE_PIPE_NO_DESIGN_TEXT);
            return;
        }

        toast_push(ctx, CROSSTALK_CORE_HLT_MASK_ON_TEXT);
        return;
    }

    if (on == LDG_TRUTH_FALSE)
    {
        ret = crosstalk_eda_ic_fpga_hlt_mask_set(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, 0x0);
        if (ret == LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_HLT_MASK_OFF_TEXT); }
    }

    ret = crosstalk_core_hlt_set(ctx, on);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "hlt: refused; on: %u32; err: %u32", on, ret);
        toast_push(ctx, CROSSTALK_CORE_PIPE_NO_DESIGN_TEXT);
        return;
    }

    toast_push(ctx, on == LDG_TRUTH_TRUE ? CROSSTALK_CORE_HLT_ON_TEXT : CROSSTALK_CORE_HLT_OFF_TEXT);
}

static void crosstalk_theme_tick(crosstalk_ctx_t *ctx)
{
    char dir[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char full[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (ctx->theme_done == LDG_TRUTH_TRUE) { return; }

    if (ctx->game.screen == CROSSTALK_GAME_SCREEN_EDITOR)
    {
        ctx->theme_done = LDG_TRUTH_TRUE;

        if (ctx->theme_playing != LDG_TRUTH_TRUE) { return; }

        ctx->theme_playing = LDG_TRUTH_FALSE;

        ret = crosstalk_game_music_theme_stop(&ctx->music);
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "crosstalk: theme stopped; err: %u32", ret);

        ret = crosstalk_game_fx_sfx_music_vol_set(&ctx->game.sfx_sound, CROSSTALK_GAME_MUSIC_VOL_FULL);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: music bus vol restore failed; err: %u32", ret); }

        return;
    }

    if (ctx->theme_playing == LDG_TRUTH_TRUE) { return; }

    ret = ldg_io_path_join(ctx->res_dir, CROSSTALK_CORE_THEME_DIR_REL, dir, (uint64_t)sizeof(dir));
    if (ret != LDG_ERR_AOK) { ctx->theme_done = LDG_TRUTH_TRUE; return; }

    ret = ldg_io_path_join(dir, CROSSTALK_CORE_THEME_FILE, full, (uint64_t)sizeof(full));
    if (ret != LDG_ERR_AOK) { ctx->theme_done = LDG_TRUTH_TRUE; return; }

    ret = crosstalk_game_music_theme_play(&ctx->music, full, CROSSTALK_CORE_THEME_VOL);
    if (ret == LDG_ERR_BUSY)
    {
        ctx->theme_done = LDG_TRUTH_TRUE;

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: theme skipped; other playback is live; src: %u32", (uint32_t)ctx->music.src);

        return;
    }

    if (ret != LDG_ERR_AOK)
    {
        ctx->theme_done = LDG_TRUTH_TRUE;

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: theme play failed; path: %.191s; err: %u32", full, ret);

        return;
    }

    ctx->theme_playing = LDG_TRUTH_TRUE;

    ret = crosstalk_game_fx_sfx_music_vol_set(&ctx->game.sfx_sound, CROSSTALK_CORE_THEME_VOL);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: theme music bus vol set failed; err: %u32", ret); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: theme playing; path: %.191s; vol: %.2f", full, (double)CROSSTALK_CORE_THEME_VOL);
}

static void crosstalk_verb_delay(crosstalk_ctx_t *ctx, const char *arg)
{
    char tok[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    char note[CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t val = 0;
    uint64_t mult = 0x1;
    uint64_t span_ns = 0;
    uint32_t at = 0;
    uint32_t pos = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (arg[at] == ' ') { at = at + 0x1; }

    while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(tok)) { tok[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

    tok[pos] = LDG_STR_TERM;

    if (crosstalk_num_parse(tok, &val) != LDG_ERR_AOK)
    {
        toast_push(ctx, CROSSTALK_CORE_DELAY_USAGE_TEXT);

        return;
    }

    while (arg[at] == ' ') { at = at + 0x1; }

    if (arg[at] != LDG_STR_TERM)
    {
        pos = 0;

        while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(tok)) { tok[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

        tok[pos] = LDG_STR_TERM;

        if (strcmp(tok, CROSSTALK_CORE_DELAY_UNIT_US) == 0) { mult = (uint64_t)LDG_NS_PER_US; }
        else if (strcmp(tok, CROSSTALK_CORE_DELAY_UNIT_MS) == 0) { mult = (uint64_t)LDG_NS_PER_MS; }
        else if (strcmp(tok, CROSSTALK_CORE_DELAY_UNIT_SEC) == 0) { mult = (uint64_t)LDG_NS_PER_SEC; }
        else if (strcmp(tok, CROSSTALK_CORE_DELAY_UNIT_NS) != 0)
        {
            toast_push(ctx, CROSSTALK_CORE_DELAY_USAGE_TEXT);

            return;
        }
    }

    span_ns = val > (uint64_t)CROSSTALK_CORE_DELAY_NS_MAX / mult ? (uint64_t)CROSSTALK_CORE_DELAY_NS_MAX : val * mult;

    if (span_ns > (uint64_t)CROSSTALK_CORE_DELAY_NS_MAX) { span_ns = (uint64_t)CROSSTALK_CORE_DELAY_NS_MAX; }

    if (ldg_thread_yield(span_ns) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "delay yield failed"); }

    if (snprintf(note, sizeof(note), CROSSTALK_CORE_DELAY_DONE_FMT, span_ns) < 0) { return; }

    if (crosstalk_core_cmdline_out_push(&ctx->cmdline, note) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "delay note push failed"); }
}

static uint32_t crosstalk_watch_worker(uint32_t worker_id, void *arg)
{
    crosstalk_ctx_t *ctx = (crosstalk_ctx_t *)arg;
    char path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t next_ns = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (LDG_LD_ACQUIRE(ctx->watch_stop) != LDG_TRUTH_TRUE)
    {
        uint32_t at = 0;
        uint32_t cunt = 0;

        if (ldg_time_epoch_ns_get() < next_ns)
        {
            if (ldg_thread_yield((uint64_t)CROSSTALK_CORE_WATCH_YIELD_NS) != LDG_ERR_AOK) { break; }

            continue;
        }

        next_ns = ldg_time_epoch_ns_get() + (uint64_t)CROSSTALK_CORE_WATCH_CK_NS;

        if (ldg_mut_lock(&ctx->watch_mut) != LDG_ERR_AOK) { continue; }

        cunt = ctx->watch_cunt;

        if (ldg_mut_unlock(&ctx->watch_mut) != LDG_ERR_AOK) { continue; }

        for (at = 0; at < cunt && LDG_LD_ACQUIRE(ctx->watch_stop) != LDG_TRUTH_TRUE; at = at + 0x1)
        {
            ldg_io_info_t info = LDG_STRUCT_ZERO_INIT;
            uint64_t mtime = 0;
            uint64_t span = 0;
            uint8_t stale = LDG_TRUTH_FALSE;

            if (ldg_mut_lock(&ctx->watch_mut) != LDG_ERR_AOK) { continue; }

            if (at < ctx->watch_cunt && ctx->watch_stale[at] != LDG_TRUTH_TRUE) { if (ldg_str_rbrcpy(path, ctx->watch_path[at], (uint64_t)sizeof(path)) == LDG_ERR_AOK)
                {
                    mtime = ctx->watch_mtime[at];
                    span = ctx->watch_span[at];
                }
                else { path[0] = LDG_STR_TERM; } }
            else { path[0] = LDG_STR_TERM; }

            if (ldg_mut_unlock(&ctx->watch_mut) != LDG_ERR_AOK) { continue; }

            if (path[0] == LDG_STR_TERM) { continue; }

            if (ldg_io_path_info_get(path, &info) != LDG_ERR_AOK) { continue; }

            if (info.size == 0) { continue; }

            if (info.modified_ns == mtime && info.size == span) { continue; }

            if (ldg_time_epoch_ns_get() - info.modified_ns < (uint64_t)CROSSTALK_CORE_WATCH_SETTLE_NS) { continue; }

            stale = mtime == 0 && span == 0 ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE;

            if (ldg_mut_lock(&ctx->watch_mut) != LDG_ERR_AOK) { continue; }

            if (at < ctx->watch_cunt && strcmp(ctx->watch_path[at], path) == 0)
            {
                ctx->watch_mtime[at] = info.modified_ns;
                ctx->watch_span[at] = info.size;
                ctx->watch_stale[at] = stale;
            }

            if (ldg_mut_unlock(&ctx->watch_mut) != LDG_ERR_AOK) { continue; }

            if (stale != LDG_TRUTH_TRUE) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "watch: baseline set; path: %.191s; bytes: %u64", path, info.size); }
        }
    }

    ctx->watch_cunt = ctx->watch_cunt + (uint32_t)(worker_id * 0x0);

    return LDG_ERR_AOK;
}

static void crosstalk_watch_entry_put(crosstalk_ctx_t *ctx, uint32_t *cunt, uint32_t kind, uint32_t slot_idx, const char *path)
{
    uint32_t at = *cunt;

    if (at >= (uint32_t)CROSSTALK_CORE_WATCH_CAP || path == 0x0 || path[0] == LDG_STR_TERM) { return; }

    if (strcmp(ctx->watch_path[at], path) != 0)
    {
        if (ldg_str_rbrcpy(ctx->watch_path[at], path, (uint64_t)LDG_IO_PATH_MAX) != LDG_ERR_AOK) { return; }

        ctx->watch_mtime[at] = 0x0;
        ctx->watch_span[at] = 0x0;
        ctx->watch_stale[at] = LDG_TRUTH_FALSE;
    }

    ctx->watch_kind[at] = kind;
    ctx->watch_slot[at] = slot_idx;

    *cunt = at + 0x1;
}

static void crosstalk_watch_sync(crosstalk_ctx_t *ctx)
{
    const crosstalk_eda_ic_fpga_mem_watch_t *watch = 0x0;
    uint32_t cunt = 0;
    uint32_t at = 0;

    if (ldg_mut_lock(&ctx->watch_mut) != LDG_ERR_AOK) { return; }

    if (crosstalk_eda_ic_fpga_emu_live_is(&ctx->eda.fpga_emu) == LDG_TRUTH_TRUE) { crosstalk_watch_entry_put(ctx, &cunt, (uint32_t)CROSSTALK_CORE_WATCH_KIND_EMU, CROSSTALK_CORE_TERM_SLOT, ctx->eda.fpga_emu.path); }

    for (at = 0; crosstalk_eda_ic_fpga_mem_watch_get(&ctx->eda.fpga, at, &watch) == LDG_ERR_AOK && watch != 0x0; at = at + 0x1) { crosstalk_watch_entry_put(ctx, &cunt, (uint32_t)CROSSTALK_CORE_WATCH_KIND_MEM, watch->slot_idx, watch->path); }

    ctx->watch_cunt = cunt;

    if (ldg_mut_unlock(&ctx->watch_mut) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "watch sync unlock failed"); }
}

static void crosstalk_watch_tick(crosstalk_ctx_t *ctx)
{
    const crosstalk_eda_ic_fpga_mem_watch_t *watch = 0x0;
    char path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char sig[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t word_off = 0;
    uint64_t words = 0;
    uint64_t bytes = 0;
    uint32_t kind = 0;
    uint32_t slot_idx = 0;
    uint32_t hit = UINT32_MAX;
    uint32_t at = 0;
    uint8_t was_live = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (ctx->watch_ok != LDG_TRUTH_TRUE) { return; }

    crosstalk_watch_sync(ctx);

    if (ldg_mut_lock(&ctx->watch_mut) != LDG_ERR_AOK) { return; }

    for (at = 0; at < ctx->watch_cunt; at = at + 0x1)
    {
        if (ctx->watch_stale[at] != LDG_TRUTH_TRUE) { continue; }

        ctx->watch_stale[at] = LDG_TRUTH_FALSE;
        kind = ctx->watch_kind[at];
        slot_idx = ctx->watch_slot[at];

        if (ldg_str_rbrcpy(path, ctx->watch_path[at], (uint64_t)sizeof(path)) == LDG_ERR_AOK) { hit = at; }

        break;
    }

    if (ldg_mut_unlock(&ctx->watch_mut) != LDG_ERR_AOK) { return; }

    if (hit == UINT32_MAX) { return; }

    if (kind == (uint32_t)CROSSTALK_CORE_WATCH_KIND_EMU)
    {
        ret = crosstalk_emu_start(ctx, path);

        if (ret != LDG_ERR_AOK)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: reload failed; path: %.127s; err: %u32", path, ret);
            toast_push(ctx, CROSSTALK_CORE_EMU_RELOAD_FAIL_TEXT);
            return;
        }

        ret = crosstalk_emu_power_cycle(ctx);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: power cycle after reload failed; err: %u32", ret); }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu: reloaded from disk; path: %.127s", path);
        toast_push(ctx, CROSSTALK_CORE_EMU_RELOAD_TEXT);

        return;
    }

    for (at = 0; crosstalk_eda_ic_fpga_mem_watch_get(&ctx->eda.fpga, at, &watch) == LDG_ERR_AOK && watch != 0x0; at = at + 0x1)
    {
        if (strcmp(watch->path, path) != 0) { continue; }

        if (ldg_str_rbrcpy(sig, watch->sig, (uint64_t)sizeof(sig)) != LDG_ERR_AOK) { return; }

        word_off = watch->word_off;
        slot_idx = watch->slot_idx;
        break;
    }

    if (sig[0] == LDG_STR_TERM) { return; }

    if (ctx->eda.fpga.slot[slot_idx].emu == LDG_TRUTH_TRUE && crosstalk_eda_ic_fpga_emu_live_is(&ctx->eda.fpga_emu) == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_emu_power_cycle(ctx);
        if (ret != LDG_ERR_AOK)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: mem reload failed; slot: %u32; sig: %.63s; path: %.127s; err: %u32", slot_idx, sig, path, ret);
            toast_push(ctx, CROSSTALK_CORE_MEM_RELOAD_FAIL_TEXT);
            return;
        }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: mem reloaded from disk; slot: %u32; sig: %.63s", slot_idx, sig);
        toast_push(ctx, CROSSTALK_CORE_MEM_RELOAD_TEXT);
        return;
    }

    was_live = crosstalk_fpga_hlt(ctx);

    ret = crosstalk_eda_ic_fpga_mem_ld(&ctx->eda.fpga, slot_idx, sig, path, word_off, &words, &bytes);

    crosstalk_fpga_resume(ctx, was_live);

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga: mem reload failed; slot: %u32; sig: %.63s; path: %.127s; err: %u32", slot_idx, sig, path, ret);
        toast_push(ctx, CROSSTALK_CORE_MEM_RELOAD_FAIL_TEXT);
        return;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "fpga: mem reloaded from disk; slot: %u32; sig: %.63s; words: %u64; off: %u64", slot_idx, sig, words, word_off);
    toast_push(ctx, CROSSTALK_CORE_MEM_RELOAD_TEXT);
}

static uint32_t crosstalk_rtl_sect_build(crosstalk_ctx_t *ctx, uint8_t **out_buff, uint32_t *out_len)
{
    uint8_t *buff = 0x0;
    uint64_t hold_hz = 0;
    uint32_t need = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_buff == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_buff = 0x0;
    *out_len = 0;

    if (crosstalk_eda_ic_fpga_run_pin_get(&ctx->eda.fpga_run, &hold_hz) != LDG_ERR_AOK) { hold_hz = 0; }

    ret = crosstalk_eda_ic_fpga_fileset_state_serialize(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, hold_hz, ctx->eda.fpga_emu.path, 0x0, CROSSTALK_GAME_SAVE_SIZE_PROBE, &need);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (need == 0) { return LDG_ERR_AOK; }

    ret = ldg_mem_alloc((uint64_t)need, (void **)&buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_eda_ic_fpga_fileset_state_serialize(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, hold_hz, ctx->eda.fpga_emu.path, buff, need, &need);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(buff);
        return ret;
    }

    *out_buff = buff;
    *out_len = need;
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_rtl_sect_restore(crosstalk_ctx_t *ctx)
{
    crosstalk_eda_ic_fpga_fileset_state_t state = LDG_STRUCT_ZERO_INIT;
    uint8_t *buff = 0x0;
    uint32_t blob_len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->save_path[0] == LDG_STR_TERM) { return LDG_ERR_NOT_FOUND; }

    ret = crosstalk_game_save_editor_sect_rd(ctx->save_path, CROSSTALK_GAME_SAVE_SECT_RTL, 0x0, CROSSTALK_GAME_SAVE_SIZE_PROBE, &blob_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (blob_len == 0) { return LDG_ERR_AOK; }

    ret = ldg_mem_alloc((uint64_t)blob_len, (void **)&buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_game_save_editor_sect_rd(ctx->save_path, CROSSTALK_GAME_SAVE_SECT_RTL, buff, blob_len, &blob_len);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_eda_ic_fpga_fileset_state_deserialize(buff, blob_len, &state); }

    ldg_mem_dealloc(buff);

    if (ret != LDG_ERR_AOK) { return ret; }

    if (state.emu != LDG_TRUTH_TRUE && state.dir[0] == LDG_STR_TERM) { return LDG_ERR_AOK; }

    if (state.emu == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_emu_start(ctx, state.emu_path);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: emu restore failed; path: %.127s; err: %u32", state.emu_path, ret); }
    }
    else { crosstalk_fileset_apply(ctx, state.dir, state.files, state.top); }

    if (state.hold_hz != 0)
    {
        ret = crosstalk_eda_ic_fpga_run_pin_set(&ctx->eda.fpga_run, state.hold_hz);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "rtl restore clk hold failed"); }
    }

    if (state.emitted == LDG_TRUTH_TRUE) { crosstalk_pack_start(ctx); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: rtl restored; top: %.63s; hold: %u64; emitted: %u32; emu: %u32", state.top, state.hold_hz, (uint32_t)state.emitted, (uint32_t)state.emu);

    if (state.dir[0] != LDG_STR_TERM) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: rtl restored dir; dir: %.191s", state.dir); }

    if (state.emu_path[0] != LDG_STR_TERM) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: rtl restored emu; path: %.127s", state.emu_path); }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_script_sect_dir_rd(const char *path, uint32_t *out_len_arr, uint8_t **out_buff, uint32_t *out_len)
{
    uint8_t *buff = 0x0;
    uint32_t blob_len = 0;
    uint32_t span = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(path == 0x0 || out_len_arr == 0x0 || out_buff == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_buff = 0x0;
    *out_len = 0;

    ret = crosstalk_game_save_editor_sect_rd(path, CROSSTALK_GAME_SAVE_SECT_SCRIPT, 0x0, CROSSTALK_GAME_SAVE_SIZE_PROBE, &blob_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (blob_len < (uint32_t)CROSSTALK_SCRIPT_DIR_BYTES) { return LDG_ERR_IO_FMT; }

    ret = ldg_mem_alloc((uint64_t)blob_len, (void **)&buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_game_save_editor_sect_rd(path, CROSSTALK_GAME_SAVE_SECT_SCRIPT, buff, blob_len, &blob_len);
    if (ret == LDG_ERR_AOK && memcmp(buff, CROSSTALK_SCRIPT_DIR_MAGIC, (uint64_t)CROSSTALK_SCRIPT_DIR_MAGIC_LEN) != 0x0) { ret = LDG_ERR_IO_FMT; }

    if (ret == LDG_ERR_AOK)
    {
        span = (uint32_t)CROSSTALK_SCRIPT_DIR_BYTES;

        for (at = 0; at < (uint32_t)CROSSTALK_CORE_EDITOR_KIND_CUNT; at = at + 0x1)
        {
            memcpy(&out_len_arr[at], buff + (uint64_t)CROSSTALK_SCRIPT_DIR_MAGIC_LEN + (uint64_t)at * sizeof(uint32_t), sizeof(uint32_t));
            span = span + out_len_arr[at];
        }

        if (span != blob_len) { ret = LDG_ERR_IO_FMT; }
    }

    if (ret != LDG_ERR_AOK)
    {
        ldg_mem_dealloc(buff);
        return ret;
    }

    *out_buff = buff;
    *out_len = blob_len;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_script_sect_build(crosstalk_ctx_t *ctx, uint8_t **out_buff, uint32_t *out_len)
{
    uint32_t dir_len[CROSSTALK_CORE_EDITOR_KIND_CUNT] = LDG_ARR_ZERO_INIT;
    uint8_t *disk_buff = 0x0;
    uint8_t *buff = 0x0;
    uint32_t need = 0;
    uint32_t slot_len = 0;
    uint32_t disk_len = 0;
    uint32_t off = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_buff == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_buff = 0x0;
    *out_len = 0;

    if (LDG_UNLIKELY(ctx->editor_kind >= (uint8_t)CROSSTALK_CORE_EDITOR_KIND_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = crosstalk_eda_script_state_serialize(&ctx->eda.script, 0x0, CROSSTALK_GAME_SAVE_SIZE_PROBE, &slot_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->save_path[0] != LDG_STR_TERM) { if (crosstalk_script_sect_dir_rd(ctx->save_path, dir_len, &disk_buff, &disk_len) != LDG_ERR_AOK)
        {
            disk_buff = 0x0;
            disk_len = 0;
        }
    }

    dir_len[ctx->editor_kind] = slot_len;
    need = (uint32_t)CROSSTALK_SCRIPT_DIR_BYTES;
    for (at = 0; at < (uint32_t)CROSSTALK_CORE_EDITOR_KIND_CUNT; at = at + 0x1) { need = need + dir_len[at]; }

    ret = ldg_mem_alloc((uint64_t)need, (void **)&buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (disk_buff != 0x0) { ldg_mem_dealloc(disk_buff); }

        return ret;
    }

    memset(buff, 0x0, (uint64_t)need);
    memcpy(buff, CROSSTALK_SCRIPT_DIR_MAGIC, (uint64_t)CROSSTALK_SCRIPT_DIR_MAGIC_LEN);
    off = (uint32_t)CROSSTALK_SCRIPT_DIR_MAGIC_LEN;

    for (at = 0; at < (uint32_t)CROSSTALK_CORE_EDITOR_KIND_CUNT; at = at + 0x1)
    {
        memcpy(buff + off, &dir_len[at], sizeof(uint32_t));
        off = off + (uint32_t)sizeof(uint32_t);
    }

    if (disk_buff != 0x0)
    {
        uint32_t src_at = (uint32_t)CROSSTALK_SCRIPT_DIR_BYTES;
        uint32_t dst_at = (uint32_t)CROSSTALK_SCRIPT_DIR_BYTES;

        for (at = 0; at < (uint32_t)CROSSTALK_CORE_EDITOR_KIND_CUNT; at = at + 0x1)
        {
            uint32_t src_len = 0;

            memcpy(&src_len, disk_buff + (uint64_t)CROSSTALK_SCRIPT_DIR_MAGIC_LEN + (uint64_t)at * sizeof(uint32_t), sizeof(uint32_t));

            if (at != (uint32_t)ctx->editor_kind && src_len > 0 && src_len == dir_len[at] && src_at + src_len <= disk_len) { memcpy(buff + dst_at, disk_buff + src_at, src_len); }

            src_at = src_at + src_len;
            dst_at = dst_at + dir_len[at];
        }

        ldg_mem_dealloc(disk_buff);
    }

    if (slot_len > 0)
    {
        off = (uint32_t)CROSSTALK_SCRIPT_DIR_BYTES;
        for (at = 0; at < (uint32_t)ctx->editor_kind; at = at + 0x1) { off = off + dir_len[at]; }

        ret = crosstalk_eda_script_state_serialize(&ctx->eda.script, buff + off, slot_len, &slot_len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            ldg_mem_dealloc(buff);
            return ret;
        }
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "script: sect built; editor: %u32; slot bytes: %u32; bytes: %u32", (uint32_t)ctx->editor_kind, slot_len, need);

    *out_buff = buff;
    *out_len = need;
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_term_sect_build(crosstalk_ctx_t *ctx, uint8_t **out_buff, uint32_t *out_len)
{
    uint8_t *buff = 0x0;
    uint32_t need = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_buff == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_buff = 0x0;
    *out_len = 0;

    ret = crosstalk_eda_ui_term_state_serialize(&ctx->eda.term, 0x0, CROSSTALK_GAME_SAVE_SIZE_PROBE, &need);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (need == 0) { return LDG_ERR_AOK; }

    ret = ldg_mem_alloc((uint64_t)need, (void **)&buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_eda_ui_term_state_serialize(&ctx->eda.term, buff, need, &need);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(buff);
        return ret;
    }

    *out_buff = buff;
    *out_len = need;
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_term_sect_restore(crosstalk_ctx_t *ctx)
{
    uint8_t *buff = 0x0;
    uint32_t blob_len = 0;
    uint8_t host_live = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->save_path[0] == LDG_STR_TERM) { return LDG_ERR_NOT_FOUND; }

    ret = crosstalk_game_save_editor_sect_rd(ctx->save_path, CROSSTALK_GAME_SAVE_SECT_TERM, 0x0, CROSSTALK_GAME_SAVE_SIZE_PROBE, &blob_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (blob_len == 0) { return LDG_ERR_AOK; }

    ret = ldg_mem_alloc((uint64_t)blob_len, (void **)&buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_game_save_editor_sect_rd(ctx->save_path, CROSSTALK_GAME_SAVE_SECT_TERM, buff, blob_len, &blob_len);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_eda_ui_term_state_deserialize(buff, blob_len, &host_live); }

    ldg_mem_dealloc(buff);

    if (ret != LDG_ERR_AOK) { return ret; }

    if (host_live != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ctx->console_exec_is = LDG_TRUTH_TRUE;
    crosstalk_term_host_open(ctx, "", "");
    ctx->console_exec_is = LDG_TRUTH_FALSE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: term host restored");
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_crt_sect_build(crosstalk_ctx_t *ctx, uint8_t **out_buff, uint32_t *out_len)
{
    uint8_t *buff = 0x0;
    uint32_t need = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_buff == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_buff = 0x0;
    *out_len = 0;

    ret = crosstalk_eda_ui_display_crt_state_serialize(&ctx->eda.crt, 0x0, CROSSTALK_GAME_SAVE_SIZE_PROBE, &need);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (need == 0) { return LDG_ERR_AOK; }

    ret = ldg_mem_alloc((uint64_t)need, (void **)&buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_eda_ui_display_crt_state_serialize(&ctx->eda.crt, buff, need, &need);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(buff);
        return ret;
    }

    *out_buff = buff;
    *out_len = need;
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_crt_sect_restore(crosstalk_ctx_t *ctx)
{
    char sig[CROSSTALK_EDA_UI_DISPLAY_CRT_SIG_CAP] = LDG_ARR_ZERO_INIT;
    uint8_t *buff = 0x0;
    uint32_t blob_len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->save_path[0] == LDG_STR_TERM) { return LDG_ERR_NOT_FOUND; }

    ret = crosstalk_game_save_editor_sect_rd(ctx->save_path, CROSSTALK_GAME_SAVE_SECT_CRT, 0x0, CROSSTALK_GAME_SAVE_SIZE_PROBE, &blob_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (blob_len == 0) { return LDG_ERR_AOK; }

    ret = ldg_mem_alloc((uint64_t)blob_len, (void **)&buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_game_save_editor_sect_rd(ctx->save_path, CROSSTALK_GAME_SAVE_SECT_CRT, buff, blob_len, &blob_len);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_eda_ui_display_crt_state_deserialize(&ctx->eda.crt, buff, blob_len); }

    ldg_mem_dealloc(buff);

    if (ret != LDG_ERR_AOK) { return ret; }

    if (ctx->eda.crt.text_sig[0] == LDG_STR_TERM) { return LDG_ERR_AOK; }

    ret = ldg_str_rbrcpy(sig, ctx->eda.crt.text_sig, sizeof(sig));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->eda.crt.src == (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_SRC_TEXT)
    {
        ret = crosstalk_eda_ui_display_crt_text_bind(&ctx->eda.crt, &ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, sig, ctx->eda.crt.text_base);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: crt text rebind failed; sig: %.63s; err: %u32", sig, ret); return ret; }
    }
    else if (ctx->eda.crt.src == (uint32_t)CROSSTALK_EDA_UI_DISPLAY_CRT_SRC_GFX)
    {
        ret = crosstalk_eda_ui_display_crt_gfx_bind(&ctx->eda.crt, &ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, sig, ctx->eda.crt.gfx_base, ctx->eda.crt.gfx_w, ctx->eda.crt.gfx_h);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: crt gfx rebind failed; sig: %.63s; err: %u32", sig, ret); return ret; }
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: crt reattached; src: %u32; sig: %.63s", ctx->eda.crt.src, sig);
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_emu_sect_build(crosstalk_ctx_t *ctx, uint8_t **out_buff, uint32_t *out_len)
{
    uint8_t *buff = 0x0;
    uint32_t need = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_buff == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_buff = 0x0;
    *out_len = 0;

    ret = crosstalk_eda_ic_fpga_emu_stat_serialize(&ctx->emu_stat, 0x0, CROSSTALK_GAME_SAVE_SIZE_PROBE, &need);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (need == 0) { return LDG_ERR_AOK; }

    ret = ldg_mem_alloc((uint64_t)need, (void **)&buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_eda_ic_fpga_emu_stat_serialize(&ctx->emu_stat, buff, need, &need);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(buff);
        return ret;
    }

    *out_buff = buff;
    *out_len = need;
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_calc_sect_build(crosstalk_ctx_t *ctx, uint8_t **out_buff, uint32_t *out_len)
{
    uint8_t *buff = 0x0;
    uint32_t need = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_buff == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_buff = 0x0;
    *out_len = 0;

    ret = crosstalk_eda_ui_calc_state_serialize(&ctx->eda.calc, 0x0, CROSSTALK_GAME_SAVE_SIZE_PROBE, &need);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (need == 0) { return LDG_ERR_AOK; }

    ret = ldg_mem_alloc((uint64_t)need, (void **)&buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_eda_ui_calc_state_serialize(&ctx->eda.calc, buff, need, &need);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(buff);
        return ret;
    }

    *out_buff = buff;
    *out_len = need;
    return LDG_ERR_AOK;
}

static uint32_t crosstalk_script_sect_restore(crosstalk_ctx_t *ctx)
{
    uint32_t dir_len[CROSSTALK_CORE_EDITOR_KIND_CUNT] = LDG_ARR_ZERO_INIT;
    uint8_t *buff = 0x0;
    uint32_t blob_len = 0;
    uint32_t slot_len = 0;
    uint32_t slot_off = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->save_path[0] == LDG_STR_TERM) { return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(ctx->editor_kind >= (uint8_t)CROSSTALK_CORE_EDITOR_KIND_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = crosstalk_script_sect_dir_rd(ctx->save_path, dir_len, &buff, &blob_len);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: script sect not a slot directory; err: %u32", ret);
        return ret;
    }

    slot_len = dir_len[ctx->editor_kind];
    slot_off = (uint32_t)CROSSTALK_SCRIPT_DIR_BYTES;
    for (at = 0; at < (uint32_t)ctx->editor_kind; at = at + 0x1) { slot_off = slot_off + dir_len[at]; }

    if (slot_len == 0)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: script slot empty; editor: %u32", (uint32_t)ctx->editor_kind);
        ldg_mem_dealloc(buff);

        return LDG_ERR_AOK;
    }

    ret = crosstalk_eda_script_state_deserialize(&ctx->eda.script, buff + slot_off, slot_len);

    ldg_mem_dealloc(buff);

    if (ret != LDG_ERR_AOK) { return ret; }

    if (ctx->eda.script.run_path[0] == LDG_STR_TERM)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: script slot empty; editor: %u32", (uint32_t)ctx->editor_kind);
        return LDG_ERR_AOK;
    }

    ctx->console_exec_is = LDG_TRUTH_TRUE;
    ret = crosstalk_eda_script_file_exec(&ctx->eda.script, ctx->eda.script.run_path);
    ctx->console_exec_is = LDG_TRUTH_FALSE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "crosstalk: script resumed; path: %.255s; err: %u32", ctx->eda.script.run_path, ret);
    crosstalk_defer_drain(ctx);

    return ret;
}

static uint32_t crosstalk_scope_width_get(const char *tok, uint32_t *out)
{
    if (LDG_UNLIKELY(tok == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (uint32_t)CROSSTALK_EDA_UI_SCOPE_WIDTH_UINT32;

    if (tok[0] == LDG_STR_TERM) { return LDG_ERR_AOK; }

    if (strcmp(tok, CROSSTALK_CORE_SCOPE_WIDTH_UINT8_TOK) == 0) { *out = (uint32_t)CROSSTALK_EDA_UI_SCOPE_WIDTH_UINT8; return LDG_ERR_AOK; }

    if (strcmp(tok, CROSSTALK_CORE_SCOPE_WIDTH_UINT16_TOK) == 0) { *out = (uint32_t)CROSSTALK_EDA_UI_SCOPE_WIDTH_UINT16; return LDG_ERR_AOK; }

    if (strcmp(tok, CROSSTALK_CORE_SCOPE_WIDTH_UINT32_TOK) == 0) { *out = (uint32_t)CROSSTALK_EDA_UI_SCOPE_WIDTH_UINT32; return LDG_ERR_AOK; }

    if (strcmp(tok, CROSSTALK_CORE_SCOPE_WIDTH_INT8_TOK) == 0) { *out = (uint32_t)CROSSTALK_EDA_UI_SCOPE_WIDTH_INT8; return LDG_ERR_AOK; }

    if (strcmp(tok, CROSSTALK_CORE_SCOPE_WIDTH_INT16_TOK) == 0) { *out = (uint32_t)CROSSTALK_EDA_UI_SCOPE_WIDTH_INT16; return LDG_ERR_AOK; }

    if (strcmp(tok, CROSSTALK_CORE_SCOPE_WIDTH_INT32_TOK) == 0) { *out = (uint32_t)CROSSTALK_EDA_UI_SCOPE_WIDTH_INT32; return LDG_ERR_AOK; }

    return LDG_ERR_FUNC_ARG_INVALID;
}

static void crosstalk_scope_mem(crosstalk_ctx_t *ctx, const char *arg)
{
    char tok[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    char width_tok[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    char toast[CROSSTALK_CORE_EMU_TOAST_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t addr = 0;
    uint32_t width = 0;
    uint32_t idx = 0;
    uint32_t at = 0;
    uint32_t pos = 0;
    uint8_t fixed = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (arg[at] == ' ') { at = at + 0x1; }

    while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(tok)) { tok[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

    tok[pos] = LDG_STR_TERM;

    if (tok[0] == LDG_STR_TERM || strcmp(tok, CROSSTALK_CORE_OFF_TOK) == 0)
    {
        if (crosstalk_eda_ui_scope_mem_clear(&ctx->eda.scope) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scope mem clear failed"); }

        toast_push(ctx, CROSSTALK_CORE_SCOPE_MEM_OFF_TEXT);

        return;
    }

    if (crosstalk_addr_parse(ctx, tok, &addr) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_SCOPE_MEM_USAGE_TEXT); return; }

    while (arg[at] == ' ') { at = at + 0x1; }

    pos = 0;

    while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(width_tok)) { width_tok[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

    width_tok[pos] = LDG_STR_TERM;

    if (strcmp(width_tok, CROSSTALK_CORE_SCOPE_FIXED_TOK) == 0)
    {
        fixed = LDG_TRUTH_TRUE;
        width_tok[0] = LDG_STR_TERM;
    }

    if (crosstalk_scope_width_get(width_tok, &width) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_SCOPE_MEM_USAGE_TEXT); return; }

    while (arg[at] == ' ') { at = at + 0x1; }

    if (strncmp(&arg[at], CROSSTALK_CORE_SCOPE_FIXED_TOK, sizeof(CROSSTALK_CORE_SCOPE_FIXED_TOK) - LDG_STR_TERM_SIZE) == 0) { fixed = LDG_TRUTH_TRUE; }

    if (fixed == LDG_TRUTH_TRUE && width_tok[0] == LDG_STR_TERM) { width = (uint32_t)CROSSTALK_EDA_UI_SCOPE_WIDTH_INT32; }

    if (crosstalk_eda_ui_scope_mem_attach(&ctx->eda.scope, addr, width, fixed, tok, &idx) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_SCOPE_MEM_FULL_TEXT); return; }

    if (crosstalk_eda_ui_scope_open_is(&ctx->eda.scope) != LDG_TRUTH_TRUE) { if (crosstalk_eda_ui_scope_open_set(&ctx->eda.scope, LDG_TRUTH_TRUE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scope open failed"); } }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "scope: memory trace; idx: %u32; addr: 0x%X64; width: %u32; fixed: %u32; label: %.31s", idx, addr, width, (uint32_t)fixed, tok);

    if (snprintf(toast, sizeof(toast), CROSSTALK_CORE_SCOPE_MEM_OK_FMT, idx, tok) > 0) { toast_push(ctx, toast); }
}

static void crosstalk_scope_xy(crosstalk_ctx_t *ctx, const char *arg)
{
    char tok[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t trace_x = 0;
    uint64_t trace_y = 0;
    uint32_t at = 0;
    uint32_t pos = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (arg[at] == ' ') { at = at + 0x1; }

    while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(tok)) { tok[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

    tok[pos] = LDG_STR_TERM;

    if (tok[0] == LDG_STR_TERM || strcmp(tok, CROSSTALK_CORE_OFF_TOK) == 0)
    {
        if (crosstalk_eda_ui_scope_xy_set(&ctx->eda.scope, (uint32_t)UINT32_MAX, (uint32_t)UINT32_MAX) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scope xy clear failed"); }

        toast_push(ctx, CROSSTALK_CORE_SCOPE_XY_OFF_TEXT);

        return;
    }

    if (crosstalk_num_parse(tok, &trace_x) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_SCOPE_XY_USAGE_TEXT); return; }

    while (arg[at] == ' ') { at = at + 0x1; }

    if (crosstalk_num_parse(&arg[at], &trace_y) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_SCOPE_XY_USAGE_TEXT); return; }

    if (crosstalk_eda_ui_scope_xy_set(&ctx->eda.scope, (uint32_t)trace_x, (uint32_t)trace_y) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_SCOPE_XY_USAGE_TEXT); return; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "scope: xy mode; x trace: %u64; y trace: %u64", trace_x, trace_y);

    toast_push(ctx, CROSSTALK_CORE_SCOPE_XY_OK_TEXT);
}

static void crosstalk_verb_scope(crosstalk_ctx_t *ctx, const char *arg)
{
    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    if (strncmp(arg, CROSSTALK_CORE_SCOPE_MEM_TOK, sizeof(CROSSTALK_CORE_SCOPE_MEM_TOK) - LDG_STR_TERM_SIZE) == 0) { crosstalk_scope_mem(ctx, &arg[sizeof(CROSSTALK_CORE_SCOPE_MEM_TOK) - LDG_STR_TERM_SIZE]); return; }

    if (strncmp(arg, CROSSTALK_CORE_SCOPE_XY_TOK, sizeof(CROSSTALK_CORE_SCOPE_XY_TOK) - LDG_STR_TERM_SIZE) == 0) { crosstalk_scope_xy(ctx, &arg[sizeof(CROSSTALK_CORE_SCOPE_XY_TOK) - LDG_STR_TERM_SIZE]); return; }

    if (arg[0] != LDG_STR_TERM)
    {
        crosstalk_scope_sig_attach(ctx, arg, LDG_TRUTH_FALSE);

        return;
    }

    if (crosstalk_eda_ui_scope_open_is(&ctx->eda.scope) == LDG_TRUTH_TRUE)
    {
        if (crosstalk_eda_ui_scope_open_set(&ctx->eda.scope, LDG_TRUTH_FALSE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cmdline scope close failed"); }

        crosstalk_scope_release(ctx);

        return;
    }

    if (crosstalk_eda_ui_scope_open_set(&ctx->eda.scope, LDG_TRUTH_TRUE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cmdline scope open failed"); }
}

static void crosstalk_verb_ind(crosstalk_ctx_t *ctx, const char *arg)
{
    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    if (arg[0] != LDG_STR_TERM)
    {
        crosstalk_indicator_attach(ctx, arg);

        return;
    }

    if (crosstalk_eda_ui_indicator_open_is(&ctx->eda.indicator) == LDG_TRUTH_TRUE)
    {
        if (crosstalk_eda_ui_indicator_open_set(&ctx->eda.indicator, LDG_TRUTH_FALSE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cmdline ind close failed"); }

        return;
    }

    crosstalk_indicator_attach(ctx, CROSSTALK_CORE_IND_PORT_DEFAULT);
}

static uint32_t crosstalk_serial_str_decode(const char *src, uint8_t *out, uint32_t cap, uint32_t *out_len, uint32_t *out_used)
{
    uint32_t at = 0;
    uint32_t pos = 0;

    if (LDG_UNLIKELY(src == 0x0 || out == 0x0 || out_len == 0x0 || out_used == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;
    *out_used = 0;

    if (src[at] != '"') { return LDG_ERR_FUNC_ARG_INVALID; }

    at = at + 0x1;

    while (src[at] != LDG_STR_TERM && src[at] != '"')
    {
        char ch = src[at];

        if (ch == '\\')
        {
            at = at + 0x1;

            if (src[at] == 'n') { ch = '\n'; }
            else if (src[at] == 'r') { ch = '\r'; }
            else if (src[at] == 't') { ch = '\t'; }
            else if (src[at] == '\\') { ch = '\\'; }
            else if (src[at] == '"') { ch = '"'; }
            else { return LDG_ERR_FUNC_ARG_INVALID; }
        }

        if (pos >= cap) { return LDG_ERR_OVERFLOW; }

        out[pos] = (uint8_t)ch;
        pos = pos + 0x1;
        at = at + 0x1;
    }

    if (src[at] != '"') { return LDG_ERR_FUNC_ARG_INVALID; }

    *out_len = pos;
    *out_used = at + 0x1;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_serial_baud_map(uint64_t baud, uint32_t *out)
{
    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;

    if (baud == 1200) { *out = (uint32_t)LDG_IO_SERIAL_BAUD_1200; }
    else if (baud == 2400) { *out = (uint32_t)LDG_IO_SERIAL_BAUD_2400; }
    else if (baud == 4800) { *out = (uint32_t)LDG_IO_SERIAL_BAUD_4800; }
    else if (baud == 9600) { *out = (uint32_t)LDG_IO_SERIAL_BAUD_9600; }
    else if (baud == 19200) { *out = (uint32_t)LDG_IO_SERIAL_BAUD_19200; }
    else if (baud == 38400) { *out = (uint32_t)LDG_IO_SERIAL_BAUD_38400; }
    else if (baud == 57600) { *out = (uint32_t)LDG_IO_SERIAL_BAUD_57600; }
    else if (baud == 115200) { *out = (uint32_t)LDG_IO_SERIAL_BAUD_115200; }
    else if (baud == 230400) { *out = (uint32_t)LDG_IO_SERIAL_BAUD_230400; }
    else if (baud == 460800) { *out = (uint32_t)LDG_IO_SERIAL_BAUD_460800; }
    else if (baud == 921600) { *out = (uint32_t)LDG_IO_SERIAL_BAUD_921600; }
    else { return LDG_ERR_RANGE; }

    return LDG_ERR_AOK;
}

static void crosstalk_serial_conf_attach(crosstalk_ctx_t *ctx, const char *arg)
{
    ldg_io_serial_conf_t conf = LDG_STRUCT_ZERO_INIT;
    char port[LDG_IO_SERIAL_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char tok[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t baud = 0;
    uint32_t baud_enum = 0;
    uint32_t at = 0;
    uint32_t pos = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (arg[at] != LDG_STR_TERM && arg[at] != '@' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(port)) { port[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

    port[pos] = LDG_STR_TERM;

    if (arg[at] != '@' || port[0] == LDG_STR_TERM) { toast_push(ctx, CROSSTALK_CORE_SERIAL_CONF_BAD_TEXT); return; }

    at = at + 0x1;

    pos = 0;

    while (arg[at] != LDG_STR_TERM && arg[at] != ':' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(tok)) { tok[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

    tok[pos] = LDG_STR_TERM;

    if (crosstalk_num_parse(tok, &baud) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_SERIAL_CONF_BAD_TEXT); return; }

    if (crosstalk_serial_baud_map(baud, &baud_enum) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_SERIAL_CONF_BAD_TEXT); return; }

    conf.baud = (ldg_io_serial_baud_t)baud_enum;
    conf.parity = LDG_IO_SERIAL_PARITY_NONE;
    conf.data = LDG_IO_SERIAL_DATA_8;
    conf.stop = LDG_IO_SERIAL_STOP_ONE;

    if (arg[at] == ':')
    {
        at = at + 0x1;

        if (arg[at] == 'O') { conf.parity = LDG_IO_SERIAL_PARITY_ODD; }
        else if (arg[at] == 'E') { conf.parity = LDG_IO_SERIAL_PARITY_EVEN; }
        else if (arg[at] != 'N') { toast_push(ctx, CROSSTALK_CORE_SERIAL_CONF_BAD_TEXT); return; }

        at = at + 0x1;
    }

    if (arg[at] == ':')
    {
        at = at + 0x1;

        if (arg[at] == '5') { conf.data = LDG_IO_SERIAL_DATA_5; }
        else if (arg[at] == '6') { conf.data = LDG_IO_SERIAL_DATA_6; }
        else if (arg[at] == '7') { conf.data = LDG_IO_SERIAL_DATA_7; }
        else if (arg[at] != '8') { toast_push(ctx, CROSSTALK_CORE_SERIAL_CONF_BAD_TEXT); return; }

        at = at + 0x1;
    }

    if (arg[at] == ':')
    {
        at = at + 0x1;

        tok[0] = arg[at];
        tok[LDG_STR_TERM_SIZE] = LDG_STR_TERM;

        if (strcmp(tok, CROSSTALK_CORE_SERIAL_STOP_TOK_DOUBLE) == 0) { conf.stop = LDG_IO_SERIAL_STOP_TWO; }
        else if (strcmp(tok, CROSSTALK_CORE_SERIAL_STOP_TOK_SINGLE) != 0) { toast_push(ctx, CROSSTALK_CORE_SERIAL_CONF_BAD_TEXT); return; }
    }

    if (crosstalk_eda_io_serial_attached_is(&ctx->eda.serial) == LDG_TRUTH_TRUE && strcmp(ctx->eda.serial.open_path, port) == 0 && memcmp(&ctx->eda.serial.conf, &conf, sizeof(conf)) == 0) { return; }

    ret = crosstalk_eda_io_serial_conf_apply(&ctx->eda.serial, &conf);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_SERIAL_CONF_BAD_TEXT); return; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "serial: conf; port: %.63s; baud: %u64; parity: %u32; data: %u32; stop: %u32", port, baud, (uint32_t)conf.parity, (uint32_t)conf.data, (uint32_t)conf.stop);

    crosstalk_serial_attach(ctx, port);
}

static void crosstalk_serial_bytes_tx(crosstalk_ctx_t *ctx, const uint8_t *bytes, uint32_t len)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || bytes == 0x0)) { return; }

    ret = crosstalk_eda_io_serial_tx_put(&ctx->eda.serial, bytes, len);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "serial: tx refused; len: %u32; err: %u32", len, ret);
        toast_push(ctx, CROSSTALK_CORE_SERIAL_TX_FAIL_TEXT);

        return;
    }

    crosstalk_serial_lane_push(ctx, ctx->serial_tx_probe_handle, &ctx->serial_tx_sample_at, bytes, len);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "serial: tx; len: %u32", len);
}

static void crosstalk_serial_val_tx(crosstalk_ctx_t *ctx, const char *name, uint32_t name_len)
{
    char tok[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint8_t bytes[LDG_WORD_BYTES] = LDG_ARR_ZERO_INIT;
    uint64_t val = 0;
    uint32_t width = 0;
    uint32_t cunt = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0)) { return; }

    if (name_len + LDG_STR_TERM_SIZE > (uint32_t)sizeof(tok)) { return; }

    memcpy(tok, name, (uint64_t)name_len);
    tok[name_len] = LDG_STR_TERM;

    if (crosstalk_eda_ic_fpga_sig_peek(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, tok, &val, &width) == LDG_ERR_AOK) { cunt = (width + (uint32_t)LDG_BYTE_BITS - 0x1) / (uint32_t)LDG_BYTE_BITS; }
    else if (crosstalk_num_parse(tok, &val) == LDG_ERR_AOK) { cunt = 0x1; while ((val >> (cunt * (uint32_t)LDG_BYTE_BITS)) != 0 && cunt < (uint32_t)LDG_WORD_BYTES) { cunt = cunt + 0x1; } }
    else
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "serial: value unresolved; name: %.63s", tok);
        toast_push(ctx, CROSSTALK_CORE_SERIAL_TX_FAIL_TEXT);

        return;
    }

    if (cunt == 0 || cunt > (uint32_t)LDG_WORD_BYTES) { cunt = (uint32_t)LDG_WORD_BYTES; }

    for (at = 0; at < cunt; at = at + 0x1) { bytes[at] = (uint8_t)((val >> ((cunt - 0x1 - at) * (uint32_t)LDG_BYTE_BITS)) & (uint64_t)LDG_BYTE_MASK); }

    crosstalk_serial_bytes_tx(ctx, bytes, cunt);
}

static void crosstalk_serial_map_arm(crosstalk_ctx_t *ctx, const char *rest)
{
    uint8_t pattern[CROSSTALK_CORE_SERIAL_MAP_PATTERN_CAP] = LDG_ARR_ZERO_INIT;
    uint8_t action[CROSSTALK_CORE_SERIAL_MAP_ACTION_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t pattern_len = 0;
    uint32_t action_len = 0;
    uint32_t used = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || rest == 0x0)) { return; }

    if (crosstalk_serial_str_decode(rest, pattern, (uint32_t)sizeof(pattern) - LDG_STR_TERM_SIZE, &pattern_len, &used) != LDG_ERR_AOK || pattern_len == 0) { toast_push(ctx, CROSSTALK_CORE_SERIAL_TX_FAIL_TEXT); return; }

    at = used;

    if (rest[at] == ':')
    {
        at = at + 0x1;

        if (crosstalk_serial_str_decode(&rest[at], action, (uint32_t)sizeof(action) - LDG_STR_TERM_SIZE, &action_len, &used) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_SERIAL_TX_FAIL_TEXT); return; }
    }

    if (ctx->serial_map_cunt >= (uint32_t)CROSSTALK_CORE_SERIAL_MAP_CAP) { toast_push(ctx, CROSSTALK_CORE_SERIAL_MAP_FULL_TEXT); return; }

    memcpy(ctx->serial_map_pattern[ctx->serial_map_cunt], pattern, (uint64_t)pattern_len);
    ctx->serial_map_pattern_len[ctx->serial_map_cunt] = pattern_len;

    if (action_len != 0) { memcpy(ctx->serial_map_action[ctx->serial_map_cunt], action, (uint64_t)action_len); }

    ctx->serial_map_action[ctx->serial_map_cunt][action_len] = LDG_STR_TERM;
    ctx->serial_map_cunt = ctx->serial_map_cunt + 0x1;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "serial: listener armed; pattern bytes: %u32; action bytes: %u32; maps: %u32", pattern_len, action_len, ctx->serial_map_cunt);
    toast_push(ctx, CROSSTALK_CORE_SERIAL_MAP_ON_TEXT);
}

static void crosstalk_verb_serial(crosstalk_ctx_t *ctx, const char *arg)
{
    const char *colon = 0x0;
    uint8_t bytes[CROSSTALK_CORE_SERIAL_MAP_ACTION_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t len = 0;
    uint32_t used = 0;
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (arg[at] == ' ') { at = at + 0x1; }

    if (arg[at] == LDG_STR_TERM) { crosstalk_serial_list_log(ctx); return; }

    if (strchr(&arg[at], '@') != 0x0) { crosstalk_serial_conf_attach(ctx, &arg[at]); return; }

    if (arg[at] == '"')
    {
        if (crosstalk_serial_str_decode(&arg[at], bytes, (uint32_t)sizeof(bytes), &len, &used) != LDG_ERR_AOK || arg[at + used] != ':') { toast_push(ctx, CROSSTALK_CORE_SERIAL_TX_FAIL_TEXT); return; }

        crosstalk_serial_bytes_tx(ctx, bytes, len);

        return;
    }

    colon = strchr(&arg[at], ':');

    if (colon == 0x0)
    {
        crosstalk_serial_attach(ctx, &arg[at]);

        return;
    }

    if (colon[LDG_STR_TERM_SIZE] == '"')
    {
        crosstalk_serial_map_arm(ctx, &colon[LDG_STR_TERM_SIZE]);

        return;
    }

    crosstalk_serial_val_tx(ctx, &arg[at], (uint32_t)(colon - &arg[at]));
}

static void crosstalk_verb_pipe(crosstalk_ctx_t *ctx, const char *arg)
{
    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    crosstalk_pipe_attach(ctx, arg[0] != LDG_STR_TERM ? arg : CROSSTALK_CORE_TERM_PORT_DEFAULT);
}

static void crosstalk_verb_sound(crosstalk_ctx_t *ctx, const char *arg)
{
    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    crosstalk_sound_attach(ctx, arg[0] != LDG_STR_TERM ? arg : CROSSTALK_CORE_SOUND_PORT_DEFAULT);
}

static void crosstalk_verb_memview(crosstalk_ctx_t *ctx, const char *arg)
{
    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    if (arg[0] == LDG_STR_TERM && crosstalk_eda_ui_memview_open_is(&ctx->eda.memview) == LDG_TRUTH_TRUE)
    {
        if (crosstalk_eda_ui_memview_open_set(&ctx->eda.memview, LDG_TRUTH_FALSE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cmdline memview close failed"); }

        return;
    }

    crosstalk_memview_open(ctx, arg);
}

static void crosstalk_verb_publish(crosstalk_ctx_t *ctx, const char *arg)
{
    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    crosstalk_wshop_publish(ctx, ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT].top, arg, 0x0);
}

static void crosstalk_verb_profile(crosstalk_ctx_t *ctx, const char *arg)
{
    char tok[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t at = 0;
    uint32_t pos = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (arg[at] == ' ') { at = at + 0x1; }

    while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)CROSSTALK_CORE_SOUND_ARG_CAP) { tok[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

    tok[pos] = LDG_STR_TERM;

    if (tok[0] == LDG_STR_TERM)
    {
        if (crosstalk_eda_ui_profile_open_set(&ctx->eda.profile, (uint8_t)(crosstalk_eda_ui_profile_open_is(&ctx->eda.profile) == LDG_TRUTH_TRUE ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE)) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "profile open toggle failed"); }

        return;
    }

    if (strcmp(tok, CROSSTALK_CORE_PROFILE_START_TOK) == 0)
    {
        while (arg[at] == ' ') { at = at + 0x1; }

        ret = crosstalk_eda_ui_profile_mark_start(&ctx->eda.profile, &arg[at]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "profile capture start failed"); return; }

        if (crosstalk_eda_ui_profile_open_set(&ctx->eda.profile, LDG_TRUTH_TRUE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "profile open failed"); }

        toast_push(ctx, CROSSTALK_CORE_PROFILE_START_TEXT);
        return;
    }

    if (strcmp(tok, CROSSTALK_CORE_PROFILE_END_TOK) == 0)
    {
        ret = crosstalk_eda_ui_profile_mark_end(&ctx->eda.profile);

        if (ret == LDG_ERR_EMPTY)
        {
            toast_push(ctx, CROSSTALK_CORE_PROFILE_NONE_TEXT);
            return;
        }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "profile capture end failed"); return; }

        toast_push(ctx, CROSSTALK_CORE_PROFILE_END_TEXT);
        return;
    }

    if (strcmp(tok, CROSSTALK_CORE_PROFILE_RST_TOK) == 0)
    {
        ret = crosstalk_eda_ui_profile_rst(&ctx->eda.profile);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "profile rebase failed"); return; }

        toast_push(ctx, CROSSTALK_CORE_PROFILE_RST_TEXT);
        return;
    }

    toast_push(ctx, CROSSTALK_CORE_PROFILE_USAGE_TEXT);
}

static void crosstalk_verb_prof(crosstalk_ctx_t *ctx, const char *arg)
{
    char tok[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint8_t want = LDG_TRUTH_FALSE;
    uint32_t at = 0;
    uint32_t pos = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (arg[at] == ' ') { at = at + 0x1; }

    while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)CROSSTALK_CORE_SOUND_ARG_CAP) { tok[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }

    tok[pos] = LDG_STR_TERM;

    if (strcmp(tok, CROSSTALK_CORE_PROF_ON_TOK) == 0) { want = LDG_TRUTH_TRUE; }
    else if (strcmp(tok, CROSSTALK_CORE_OFF_TOK) == 0) { want = LDG_TRUTH_FALSE; }
    else
    {
        toast_push(ctx, CROSSTALK_CORE_PROF_USAGE_TEXT);
        return;
    }

    ret = crosstalk_eda_ic_fpga_prof_en(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, want);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: instruction ring set failed; on: %u32; err: %u32", (uint32_t)want, ret);
        return;
    }

    ctx->prof_en_on = want;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu: instruction ring set by verb; on: %u32", (uint32_t)want);

    toast_push(ctx, want == LDG_TRUTH_TRUE ? CROSSTALK_CORE_PROF_ON_TEXT : CROSSTALK_CORE_PROF_OFF_TEXT);
}

static uint32_t crosstalk_script_args_join(const crosstalk_eda_script_val_t *arg, uint32_t arg_cunt, char *out, uint32_t cap)
{
    uint64_t num = 0;
    uint32_t pos = 0;
    uint32_t idx = 0;
    uint32_t wrote = 0;

    if (LDG_UNLIKELY(arg == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    out[0] = LDG_STR_TERM;

    for (idx = 0; idx < arg_cunt; idx = idx + 0x1)
    {
        if (pos != 0 && pos + LDG_STR_TERM_SIZE < cap)
        {
            out[pos] = ' ';
            pos = pos + 0x1;
            out[pos] = LDG_STR_TERM;
        }

        if (arg[idx].kind == (uint32_t)CROSSTALK_EDA_SCRIPT_VAL_STR)
        {
            uint32_t quote_bytes = arg[idx].quoted == LDG_TRUTH_TRUE ? (uint32_t)CROSSTALK_CORE_SCRIPT_QUOTE_BYTES : (uint32_t)CROSSTALK_CORE_SCRIPT_QUOTE_NONE;

            if (pos + arg[idx].bytes_len + quote_bytes + LDG_STR_TERM_SIZE > cap) { return LDG_ERR_FULL; }

            if (arg[idx].quoted == LDG_TRUTH_TRUE)
            {
                out[pos] = LDG_ASCII_QUOTE;
                pos = pos + 0x1;
            }

            memcpy(&out[pos], arg[idx].bytes, arg[idx].bytes_len);
            pos = pos + arg[idx].bytes_len;

            if (arg[idx].quoted == LDG_TRUTH_TRUE)
            {
                out[pos] = LDG_ASCII_QUOTE;
                pos = pos + 0x1;
            }

            out[pos] = LDG_STR_TERM;

            continue;
        }

        if (crosstalk_eda_script_val_num_get(&arg[idx], &num) != LDG_ERR_AOK) { return LDG_ERR_FUNC_ARG_INVALID; }

        wrote = (uint32_t)snprintf(&out[pos], (uint64_t)(cap - pos), "%" PRIu64, num);

        if (pos + wrote + LDG_STR_TERM_SIZE > cap) { return LDG_ERR_FULL; }

        pos = pos + wrote;
    }

    return LDG_ERR_AOK;
}

static void crosstalk_verb_emu(crosstalk_ctx_t *ctx, const char *arg)
{
    char toast[CROSSTALK_CORE_EMU_TOAST_CAP] = LDG_ARR_ZERO_INIT;
    char full[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint8_t was_live = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (arg == 0x0 || arg[0] == LDG_STR_TERM)
    {
        if (crosstalk_eda_ic_fpga_emu_live_is(&ctx->eda.fpga_emu) != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_EMU_NONE_TEXT); return; }

        snprintf(toast, sizeof(toast), CROSSTALK_CORE_EMU_STATE_FMT, ctx->eda.fpga_emu.path);
        toast_push(ctx, toast);
        return;
    }

    if (strcmp(arg, CROSSTALK_CORE_OFF_TOK) == 0)
    {
        if (crosstalk_eda_ic_fpga_emu_live_is(&ctx->eda.fpga_emu) != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_EMU_NONE_TEXT); return; }

        was_live = crosstalk_fpga_hlt(ctx);

        ret = crosstalk_eda_ic_fpga_abi_set(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, 0x0);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu abi clear failed"); }

        if (crosstalk_emu_net_shutdown(&ctx->emu_net) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu net shutdown failed"); }

        if (crosstalk_eda_ic_fpga_emu_detach(&ctx->eda.fpga_emu) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu detach failed"); }

        crosstalk_fpga_resume(ctx, was_live);

        toast_push(ctx, CROSSTALK_CORE_EMU_OFF_TEXT);
        return;
    }

    ret = crosstalk_script_path_resolve(ctx, arg, full, (uint64_t)sizeof(full));
    if (ret != LDG_ERR_AOK)
    {
        snprintf(toast, sizeof(toast), CROSSTALK_CORE_EMU_FAIL_FMT, ret);
        toast_push(ctx, toast);
        return;
    }

    ret = crosstalk_emu_start(ctx, full);
    if (ret != LDG_ERR_AOK)
    {
        snprintf(toast, sizeof(toast), CROSSTALK_CORE_EMU_FAIL_FMT, ret);
        toast_push(ctx, toast);
        return;
    }

    toast_push(ctx, CROSSTALK_CORE_EMU_OK_TEXT);
}

static void crosstalk_verb_net(crosstalk_ctx_t *ctx, const char *arg)
{
    char toast[CROSSTALK_CORE_SERIAL_TOAST_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (arg == 0x0 || arg[0] == LDG_STR_TERM)
    {
        if (ctx->emu_net_path[0] == LDG_STR_TERM) { toast_push(ctx, CROSSTALK_CORE_NET_NONE_TEXT); return; }

        snprintf(toast, sizeof(toast), CROSSTALK_CORE_NET_STATE_FMT, ctx->emu_net_path, (uint32_t)ctx->emu_net.open);
        toast_push(ctx, toast);
        return;
    }

    if (strcmp(arg, CROSSTALK_CORE_OFF_TOK) == 0)
    {
        if (crosstalk_emu_net_shutdown(&ctx->emu_net) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu net shutdown failed"); }

        ctx->emu_net_path[0] = LDG_STR_TERM;
        toast_push(ctx, CROSSTALK_CORE_NET_OFF_TEXT);
        return;
    }

    ret = ldg_str_rbrcpy(ctx->emu_net_path, arg, (uint64_t)sizeof(ctx->emu_net_path));
    if (ret == LDG_ERR_AOK) { ret = crosstalk_emu_net_open(ctx); }

    if (ret != LDG_ERR_AOK)
    {
        snprintf(toast, sizeof(toast), CROSSTALK_CORE_NET_FAIL_FMT, ret);
        toast_push(ctx, toast);
        return;
    }

    toast_push(ctx, CROSSTALK_CORE_NET_OK_TEXT);
}

static void crosstalk_verb_gate(crosstalk_ctx_t *ctx, const char *arg)
{
    const crosstalk_hdl_exec_ctx_t *exec = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (arg != 0x0 && arg[0] != LDG_STR_TERM) { toast_push(ctx, "gate takes no arguments"); return; }

    ret = crosstalk_eda_ic_fpga_slot_exec_get(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, &exec);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_PIPE_NO_DESIGN_TEXT); return; }

    if (exec->gate == 0x0)
    {
        toast_push(ctx, "no gate-level netlist for this design");
        return;
    }

    if (exec->gate_on == LDG_TRUTH_TRUE)
    {
        toast_push(ctx, "already on the gate-level netlist");
        return;
    }

    ret = crosstalk_eda_ic_fpga_slot_gate_set(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, LDG_TRUTH_TRUE);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, "no gate-level netlist for this design"); return; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "gate: netlist engine engaged");
    crosstalk_ach_once(ctx, CROSSTALK_GAME_PROGRESSION_FIRST_GATE);
    toast_push(ctx, "gate-level netlist engaged");
}

static void crosstalk_verb_program(crosstalk_ctx_t *ctx, const char *arg)
{
    char dir[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char base[CROSSTALK_CORE_LD_DIR_CAP] = LDG_ARR_ZERO_INIT;
    char top[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    char composed[LDG_IO_PATH_MAX + CROSSTALK_CORE_LD_DIR_CAP + CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    const char *path = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    path = arg[0] != LDG_STR_TERM ? arg : ctx->editor_file;
    if (path[0] == LDG_STR_TERM)
    {
        toast_push(ctx, "copy a file path in the editor first");
        return;
    }

    if (ldg_io_path_exists_is(path) != LDG_TRUTH_TRUE)
    {
        toast_push(ctx, "file not reachable");
        return;
    }

    ret = ldg_io_path_parent_get(path, dir, (uint64_t)sizeof(dir));
    if (ret == LDG_ERR_AOK) { ret = ldg_io_path_leaf_get(path, base, (uint64_t)sizeof(base)); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        toast_push(ctx, "path split failed");
        return;
    }

    ret = crosstalk_sv_top_get(path, top, (uint32_t)sizeof(top));
    if (ret != LDG_ERR_AOK)
    {
        toast_push(ctx, "no module found in the file");
        return;
    }

    snprintf(composed, sizeof(composed), "%.259s %.127s %.63s", dir, base, top);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: program; path: %.259s; top: %.63s", path, top);
    crosstalk_dir_ld(ctx, composed);
}

static void crosstalk_verb_regs(crosstalk_ctx_t *ctx, const char *arg)
{
    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    if (crosstalk_eda_ui_regview_open_set(&ctx->eda.regview, crosstalk_eda_ui_regview_open_is(&ctx->eda.regview) == LDG_TRUTH_TRUE ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "regview toggle failed"); }
}

static void crosstalk_verb_guide(crosstalk_ctx_t *ctx, const char *arg)
{
    char list[CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CAP] = LDG_ARR_ZERO_INIT;
    const char *name = 0x0;
    uint32_t used = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    if (arg[0] == LDG_STR_TERM)
    {
        used = (uint32_t)snprintf(list, sizeof(list), "guide <name>; names:");

        for (idx = 0; crosstalk_ui_guide_name_get(&ctx->guide, idx, &name) == LDG_ERR_AOK && used < (uint32_t)sizeof(list); idx = idx + 0x1) { used = used + (uint32_t)snprintf(list + used, sizeof(list) - used, " %.31s", name); }

        toast_push(ctx, list);
        return;
    }

    if (strcmp(arg, "next") == 0) { ret = crosstalk_ui_guide_next(&ctx->guide); }
    else if (strcmp(arg, "back") == 0) { ret = crosstalk_ui_guide_back(&ctx->guide); }
    else if (strcmp(arg, "show") == 0) { ret = crosstalk_ui_guide_show(&ctx->guide); }
    else if (strcmp(arg, "close") == 0) { ret = crosstalk_ui_guide_close(&ctx->guide); }
    else
    {
        ret = crosstalk_ui_guide_start(&ctx->guide, arg);
        if (ret == LDG_ERR_AOK) { crosstalk_guide_fit(ctx); }
    }

    if (ret == LDG_ERR_NOT_FOUND) { toast_push(ctx, "no such guide"); }
    else if (ret == LDG_ERR_NOT_INIT) { toast_push(ctx, "no guide running"); }
    else if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "guide: verb failed; arg: %.63s; err: %u32", arg, ret); }
}

static void crosstalk_verb_feed(crosstalk_ctx_t *ctx, const char *arg)
{
    const crosstalk_hdl_exec_ctx_t *exec = 0x0;
    const crosstalk_hdl_abi_t *abi = 0x0;
    char tok[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t probe_handle = UINT32_MAX;
    uint32_t lane_cunt = 0;
    uint32_t added = 0;
    uint32_t at = 0;
    uint32_t pos = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (arg[at] == ' ') { at = at + 0x1; }

    if (arg[at] == LDG_STR_TERM)
    {
        if (crosstalk_eda_ui_feed_open_set(&ctx->eda.feed, crosstalk_eda_ui_feed_open_is(&ctx->eda.feed) == LDG_TRUTH_TRUE ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "feed toggle failed"); }

        return;
    }

    pos = 0;
    while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)CROSSTALK_CORE_SOUND_ARG_CAP) { tok[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }
    tok[pos] = LDG_STR_TERM;

    if (strcmp(tok, "off") == 0)
    {
        if (crosstalk_eda_ui_feed_lane_cunt_get(&ctx->eda.feed, &lane_cunt) == LDG_ERR_AOK) { for (idx = 0; idx < lane_cunt; idx = idx + 0x1)
            {
                if (crosstalk_eda_ui_feed_lane_handle_get(&ctx->eda.feed, idx, &probe_handle) != LDG_ERR_AOK) { continue; }

                if (crosstalk_eda_logic_probe_detach(&ctx->eda.logic_probe, probe_handle) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "feed probe detach failed"); }
            }
        }

        if (crosstalk_eda_ui_feed_rst(&ctx->eda.feed) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "feed rst failed"); }

        toast_push(ctx, "feed lanes released");
        return;
    }

    ret = crosstalk_probe_src_get(ctx, &exec, &abi);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, "no running design"); return; }

    while (tok[0] != LDG_STR_TERM)
    {
        ret = crosstalk_eda_logic_probe_attach(&ctx->eda.logic_probe, CROSSTALK_CORE_TERM_SLOT, exec, abi, tok, &probe_handle);
        if (ret != LDG_ERR_AOK)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "feed: attach failed; sig: %.63s; err: %u32", tok, ret);
            toast_push(ctx, "feed sig refused");
        }
        else
        {
            ret = crosstalk_eda_ui_feed_lane_add(&ctx->eda.feed, tok, probe_handle, ctx->eda.logic_probe.probe[probe_handle].width);

            if (ret == LDG_ERR_EXISTS)
            {
                if (crosstalk_eda_logic_probe_detach(&ctx->eda.logic_probe, probe_handle) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "feed probe detach failed"); }

                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "feed: lane already present; sig: %.63s", tok);

                added = added + 0x1;
            }
            else if (ret != LDG_ERR_AOK)
            {
                if (crosstalk_eda_logic_probe_detach(&ctx->eda.logic_probe, probe_handle) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "feed probe detach failed"); }

                toast_push(ctx, "feed lanes full");
                break;
            }
            else
            {
                added = added + 0x1;
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "feed: lane; sig: %.63s; probe: %u32; width: %u32", tok, probe_handle, ctx->eda.logic_probe.probe[probe_handle].width);
            }
        }

        while (arg[at] == ' ') { at = at + 0x1; }

        pos = 0;
        while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)CROSSTALK_CORE_SOUND_ARG_CAP) { tok[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }
        tok[pos] = LDG_STR_TERM;
    }

    if (added != 0) { if (crosstalk_eda_ui_feed_open_set(&ctx->eda.feed, LDG_TRUTH_TRUE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "feed open failed"); } }
}

static void crosstalk_verb_analyzer(crosstalk_ctx_t *ctx, const char *arg)
{
    const crosstalk_hdl_exec_ctx_t *exec = 0x0;
    const crosstalk_hdl_abi_t *abi = 0x0;
    char tok[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t span = 0;
    uint32_t probe_handle = UINT32_MAX;
    uint32_t lane_cunt = 0;
    uint32_t added = 0;
    uint32_t at = 0;
    uint32_t pos = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (arg[at] == ' ') { at = at + 0x1; }

    if (arg[at] == LDG_STR_TERM)
    {
        if (crosstalk_eda_ui_analyzer_open_set(&ctx->eda.analyzer, crosstalk_eda_ui_analyzer_open_is(&ctx->eda.analyzer) == LDG_TRUTH_TRUE ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "analyzer toggle failed"); }

        return;
    }

    pos = 0;
    while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)CROSSTALK_CORE_SOUND_ARG_CAP) { tok[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }
    tok[pos] = LDG_STR_TERM;

    if (strcmp(tok, "off") == 0)
    {
        if (crosstalk_eda_ui_analyzer_lane_cunt_get(&ctx->eda.analyzer, &lane_cunt) == LDG_ERR_AOK) { for (idx = 0; idx < lane_cunt; idx = idx + 0x1)
            {
                if (crosstalk_eda_ui_analyzer_lane_handle_get(&ctx->eda.analyzer, idx, &probe_handle) != LDG_ERR_AOK) { continue; }

                if (crosstalk_eda_logic_probe_detach(&ctx->eda.logic_probe, probe_handle) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "analyzer probe detach failed"); }
            }
        }

        if (crosstalk_eda_ui_analyzer_rst(&ctx->eda.analyzer) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "analyzer rst failed"); }

        toast_push(ctx, "analyzer lanes released");
        return;
    }

    if (strcmp(tok, "span") == 0)
    {
        while (arg[at] == ' ') { at = at + 0x1; }

        if (ldg_str_to_dec(&arg[at], &span) != LDG_ERR_AOK || crosstalk_eda_ui_analyzer_span_set(&ctx->eda.analyzer, (uint64_t)span) != LDG_ERR_AOK)
        {
            toast_push(ctx, "analyzer span refused");
            return;
        }

        toast_push(ctx, "analyzer span set");
        return;
    }

    ret = crosstalk_probe_src_get(ctx, &exec, &abi);
    if (ret != LDG_ERR_AOK) { toast_push(ctx, "no running design"); return; }

    while (tok[0] != LDG_STR_TERM)
    {
        ret = crosstalk_eda_logic_probe_attach(&ctx->eda.logic_probe, CROSSTALK_CORE_TERM_SLOT, exec, abi, tok, &probe_handle);
        if (ret != LDG_ERR_AOK)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "analyzer: attach failed; sig: %.63s; err: %u32", tok, ret);
            toast_push(ctx, "analyzer sig refused");
        }
        else
        {
            ret = crosstalk_eda_ui_analyzer_lane_add(&ctx->eda.analyzer, tok, probe_handle, ctx->eda.logic_probe.probe[probe_handle].width);

            if (ret == LDG_ERR_EXISTS)
            {
                if (crosstalk_eda_logic_probe_detach(&ctx->eda.logic_probe, probe_handle) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "analyzer probe detach failed"); }

                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "analyzer: lane already present; sig: %.63s", tok);

                added = added + 0x1;
            }
            else if (ret != LDG_ERR_AOK)
            {
                if (crosstalk_eda_logic_probe_detach(&ctx->eda.logic_probe, probe_handle) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "analyzer probe detach failed"); }

                toast_push(ctx, "analyzer lanes full");
                break;
            }
            else
            {
                added = added + 0x1;
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "analyzer: lane; sig: %.63s; probe: %u32; width: %u32", tok, probe_handle, ctx->eda.logic_probe.probe[probe_handle].width);
            }
        }

        while (arg[at] == ' ') { at = at + 0x1; }

        pos = 0;
        while (arg[at] != LDG_STR_TERM && arg[at] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)CROSSTALK_CORE_SOUND_ARG_CAP) { tok[pos] = arg[at]; pos = pos + 0x1; at = at + 0x1; }
        tok[pos] = LDG_STR_TERM;
    }

    if (added != 0) { if (crosstalk_eda_ui_analyzer_open_set(&ctx->eda.analyzer, LDG_TRUTH_TRUE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "analyzer open failed"); } }
}

static void crosstalk_verb_pipeview(crosstalk_ctx_t *ctx, const char *arg)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (arg[at] == ' ') { at = at + 0x1; }

    if (arg[at] == LDG_STR_TERM)
    {
        if (crosstalk_eda_ui_pipeview_open_set(&ctx->eda.pipeview, crosstalk_eda_ui_pipeview_open_is(&ctx->eda.pipeview) == LDG_TRUTH_TRUE ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pipeview toggle failed"); }

        return;
    }

    if (crosstalk_eda_ui_pipeview_stages_set(&ctx->eda.pipeview, &arg[at]) != LDG_ERR_AOK)
    {
        toast_push(ctx, "pipeview refused the stage sigs");
        return;
    }

    if (crosstalk_eda_ui_pipeview_open_set(&ctx->eda.pipeview, LDG_TRUTH_TRUE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pipeview open failed"); }
}

static void crosstalk_verb_progview(crosstalk_ctx_t *ctx, const char *arg)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (arg[at] == ' ') { at = at + 0x1; }

    if (arg[at] == LDG_STR_TERM)
    {
        if (crosstalk_eda_ui_progview_open_set(&ctx->eda.progview, crosstalk_eda_ui_progview_open_is(&ctx->eda.progview) == LDG_TRUTH_TRUE ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "progview toggle failed"); }

        return;
    }

    if (crosstalk_eda_ui_progview_sig_set(&ctx->eda.progview, &arg[at]) != LDG_ERR_AOK)
    {
        toast_push(ctx, "progview refused the sig name");
        return;
    }

    if (crosstalk_eda_ui_progview_open_set(&ctx->eda.progview, LDG_TRUTH_TRUE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "progview open failed"); }
}

static void crosstalk_verb_brainomatic(crosstalk_ctx_t *ctx, const char *arg)
{
    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    if (crosstalk_eda_ui_brainomatic_open_set(&ctx->eda.brainomatic, crosstalk_eda_ui_brainomatic_open_is(&ctx->eda.brainomatic) == LDG_TRUTH_TRUE ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "brainomatic toggle failed"); }
}

static void crosstalk_verb_scan(crosstalk_ctx_t *ctx, const char *arg)
{
    char line[CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t verdict = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (arg[at] == ' ') { at = at + 0x1; }

    if (arg[at] == LDG_STR_TERM)
    {
        if (snprintf(line, sizeof(line), CROSSTALK_CORE_SCAN_STATE_FMT, (uint32_t)crosstalk_core_scan_ready_is(&ctx->scan), ctx->scan.runs, ctx->scan.hits) < 0) { return; }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "%.191s", line);

        if (crosstalk_core_cmdline_out_push(&ctx->cmdline, line) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scan report push failed"); }

        return;
    }

    if (crosstalk_core_scan_ready_is(&ctx->scan) != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_SCAN_NONE_TEXT); return; }

    ret = crosstalk_core_scan_file(&ctx->scan, &arg[at], &verdict);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scan: refused; path: %.127s; err: %u32", &arg[at], ret);

        if (snprintf(line, sizeof(line), CROSSTALK_CORE_SCAN_FAIL_FMT, ret) < 0) { return; }

        toast_push(ctx, line);

        return;
    }

    toast_push(ctx, verdict == (uint32_t)CROSSTALK_CORE_SCAN_VERDICT_DETECTED ? CROSSTALK_CORE_SCAN_HIT_TEXT : CROSSTALK_CORE_SCAN_CLEAN_TEXT);
}

static void crosstalk_verb_stackview(crosstalk_ctx_t *ctx, const char *arg)
{
    uint8_t open = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    if (arg[0] != LDG_STR_TERM)
    {
        if (crosstalk_eda_ui_stackview_mem_set(&ctx->eda.stackview, arg) != LDG_ERR_AOK) { toast_push(ctx, "stack window refused the memory"); return; }

        if (crosstalk_eda_ui_stackview_open_set(&ctx->eda.stackview, LDG_TRUTH_TRUE) != LDG_ERR_AOK) { toast_push(ctx, "stack window refused"); }

        return;
    }

    open = crosstalk_eda_ui_stackview_open_is(&ctx->eda.stackview) == LDG_TRUTH_TRUE ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE;
    if (crosstalk_eda_ui_stackview_open_set(&ctx->eda.stackview, open) != LDG_ERR_AOK)
    {
        toast_push(ctx, "stack window refused");
        return;
    }

    toast_push(ctx, open == LDG_TRUTH_TRUE ? "stack window up" : "stack window away");
}

static void crosstalk_verb_rst(crosstalk_ctx_t *ctx, const char *arg)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    ret = crosstalk_emu_power_cycle(ctx);
    if (ret == LDG_ERR_NOT_INIT)
    {
        toast_push(ctx, "rst refused; no live emulator");
        return;
    }

    if (ret != LDG_ERR_AOK)
    {
        toast_push(ctx, "rst incomplete; see the log");
        return;
    }

    toast_push(ctx, "emulator power cycled");
}

static void crosstalk_verb_calc(crosstalk_ctx_t *ctx, const char *arg)
{
    uint8_t open = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    open = crosstalk_eda_ui_calc_open_is(&ctx->eda.calc) == LDG_TRUTH_TRUE ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE;
    if (crosstalk_eda_ui_calc_open_set(&ctx->eda.calc, open) != LDG_ERR_AOK)
    {
        toast_push(ctx, "calculator refused");
        return;
    }

    toast_push(ctx, open == LDG_TRUTH_TRUE ? "calculator up" : "calculator away");
}

static void crosstalk_verb_break(crosstalk_ctx_t *ctx, const char *arg)
{
    char line[CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t addr = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    while (arg[at] == ' ') { at = at + 0x1; }

    if (arg[at] == LDG_STR_TERM)
    {
        if (ctx->brk_on != LDG_TRUTH_TRUE) { toast_push(ctx, CROSSTALK_CORE_BRK_NONE_TEXT); return; }

        if (snprintf(line, sizeof(line), CROSSTALK_CORE_BRK_AT_FMT, ctx->brk_addr) < 0) { return; }

        if (crosstalk_core_cmdline_out_push(&ctx->cmdline, line) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "break report push failed"); }

        return;
    }

    {
        char tok[CROSSTALK_CORE_BRK_TOK_CAP] = LDG_ARR_ZERO_INIT;
        uint32_t pos = 0;
        uint32_t scan = at;

        while (arg[scan] != LDG_STR_TERM && arg[scan] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(tok)) { tok[pos] = arg[scan]; pos = pos + 0x1; scan = scan + 0x1; }

        tok[pos] = LDG_STR_TERM;

        while (arg[scan] == ' ') { scan = scan + 0x1; }

        if (strcmp(tok, CROSSTALK_CORE_BRK_SKIP_TOK) == 0)
        {
            if (crosstalk_num_parse(&arg[scan], &addr) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_BRK_USAGE_TEXT); return; }

            ret = crosstalk_eda_ic_fpga_brk_skip_set(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, addr);

            if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "break: skip failed; err: %u32", ret); toast_push(ctx, ret == LDG_ERR_UNSUPPORTED ? CROSSTALK_CORE_BRK_NO_SUPPORT_TEXT : CROSSTALK_CORE_BRK_FAIL_TEXT); return; }

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "break: skip set; hits: %u64", addr);

            if (snprintf(line, sizeof(line), CROSSTALK_CORE_BRK_SKIP_FMT, addr) < 0) { return; }

            if (crosstalk_core_cmdline_out_push(&ctx->cmdline, line) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "break note push failed"); }

            return;
        }

        if (strcmp(tok, CROSSTALK_CORE_BRK_COND_TOK) == 0)
        {
            uint64_t reg = 0;
            uint64_t min = 0;
            uint32_t rest = 0;

            if (strcmp(&arg[scan], CROSSTALK_CORE_OFF_TOK) == 0)
            {
                ret = crosstalk_eda_ic_fpga_brk_cond_set(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, (uint8_t)CROSSTALK_HDL_ABI_BRK_COND_NONE, (uint64_t)CROSSTALK_CORE_BRK_MIN_NONE);

                if (ret != LDG_ERR_AOK) { toast_push(ctx, ret == LDG_ERR_UNSUPPORTED ? CROSSTALK_CORE_BRK_NO_SUPPORT_TEXT : CROSSTALK_CORE_BRK_FAIL_TEXT); return; }

                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "break: condition cleared");
                toast_push(ctx, CROSSTALK_CORE_BRK_COND_OFF_TEXT);

                return;
            }

            pos = 0;

            while (arg[scan + pos] != LDG_STR_TERM && arg[scan + pos] != ' ' && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(tok)) { tok[pos] = arg[scan + pos]; pos = pos + 0x1; }

            tok[pos] = LDG_STR_TERM;
            rest = scan + pos;

            while (arg[rest] == ' ') { rest = rest + 0x1; }

            if (crosstalk_num_parse(tok, &reg) != LDG_ERR_AOK || crosstalk_num_parse(&arg[rest], &min) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_BRK_USAGE_TEXT); return; }

            if (reg > (uint64_t)CROSSTALK_HDL_ABI_BRK_REG_MAX) { toast_push(ctx, CROSSTALK_CORE_BRK_USAGE_TEXT); return; }

            ret = crosstalk_eda_ic_fpga_brk_cond_set(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, (uint8_t)reg, min);

            if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "break: condition failed; err: %u32", ret); toast_push(ctx, ret == LDG_ERR_UNSUPPORTED ? CROSSTALK_CORE_BRK_NO_SUPPORT_TEXT : CROSSTALK_CORE_BRK_FAIL_TEXT); return; }

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "break: condition set; reg: %u64; min: %u64", reg, min);

            if (snprintf(line, sizeof(line), CROSSTALK_CORE_BRK_COND_FMT, reg, min) < 0) { return; }

            if (crosstalk_core_cmdline_out_push(&ctx->cmdline, line) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "break note push failed"); }

            return;
        }
    }

    if (strcmp(&arg[at], CROSSTALK_CORE_OFF_TOK) == 0)
    {
        ret = crosstalk_eda_ic_fpga_brk_set(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, ctx->brk_addr, LDG_TRUTH_FALSE);

        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "break: clear failed; err: %u32", ret); toast_push(ctx, ret == LDG_ERR_UNSUPPORTED ? CROSSTALK_CORE_BRK_NO_SUPPORT_TEXT : CROSSTALK_CORE_BRK_FAIL_TEXT); return; }

        ctx->brk_on = LDG_TRUTH_FALSE;
        ctx->brk_addr = 0;

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "break: cleared");
        toast_push(ctx, CROSSTALK_CORE_BRK_OFF_TEXT);

        return;
    }

    if (crosstalk_addr_parse(ctx, &arg[at], &addr) != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_BRK_USAGE_TEXT); return; }

    ret = crosstalk_eda_ic_fpga_brk_set(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, addr, LDG_TRUTH_TRUE);

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "break: set failed; addr: 0x%X64; err: %u32", addr, ret);
        toast_push(ctx, ret == LDG_ERR_UNSUPPORTED ? CROSSTALK_CORE_BRK_NO_SUPPORT_TEXT : CROSSTALK_CORE_BRK_FAIL_TEXT);

        return;
    }

    ctx->brk_addr = addr;
    ctx->brk_on = LDG_TRUTH_TRUE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "break: armed; addr: 0x%X64", addr);

    if (snprintf(line, sizeof(line), CROSSTALK_CORE_BRK_SET_FMT, addr) < 0) { return; }

    if (crosstalk_core_cmdline_out_push(&ctx->cmdline, line) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "break note push failed"); }
}

static void crosstalk_verb_hostmem(crosstalk_ctx_t *ctx, const char *arg)
{
    ldg_mem_stats_t stats = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    ret = ldg_mem_stats_get(&stats);
    if (ret != LDG_ERR_AOK)
    {
        toast_push(ctx, "host mem stats refused");
        return;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: host mem; alloc_mib: %u64; peak_mib: %u64; allocs: %u64; deallocs: %u64; active: %u64; pools: %u64", stats.bytes_alloc / (uint64_t)LDG_MIB, stats.bytes_peak / (uint64_t)LDG_MIB, stats.alloc_cunt, stats.dealloc_cunt, stats.active_alloc_cunt, stats.pool_cunt);

    if (arg[0] == 'd')
    {
        ret = ldg_mem_leaks_dump();
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: host mem dump; err: %u32", ret);
    }

    toast_push(ctx, "host mem logged");
}

static void crosstalk_instability_feed(crosstalk_ctx_t *ctx)
{
    uint64_t now = 0;
    uint64_t mix = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (ctx->time < ctx->instability_next) { return; }

    ctx->instability_next = ctx->time + CROSSTALK_CORE_INSTABILITY_FEED_SECS;

    now = ldg_rdtsc();
    mix = (now - ctx->instability_prev_tsc) ^ now;
    ctx->instability_prev_tsc = now;

    mix = mix ^ (mix >> (LDG_BYTE_BITS * 0x4));
    mix = mix ^ (mix >> (LDG_BYTE_BITS * 0x2));
    mix = mix ^ (mix >> LDG_BYTE_BITS);

    ret = crosstalk_eda_ic_fpga_sig_poke(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, CROSSTALK_CORE_INSTABILITY_PORT_NAME, mix & (uint64_t)LDG_BYTE_MASK, LDG_TRUTH_TRUE);
    if (ret != LDG_ERR_AOK) { return; }
}

static void crosstalk_verb_hcf(crosstalk_ctx_t *ctx, const char *arg)
{
    if (LDG_UNLIKELY(ctx == 0x0 || arg == 0x0)) { return; }

    if (crosstalk_eda_ic_fpga_run_live_is(&ctx->eda.fpga_run) != LDG_TRUTH_TRUE)
    {
        toast_push(ctx, "hcf: nothing is running that could catch fire");
        return;
    }

    if (crosstalk_eda_ic_fpga_run_hlt(&ctx->eda.fpga_run) != LDG_ERR_AOK)
    {
        toast_push(ctx, "hcf: the component refuses to burn");
        return;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "hcf: undocumented; core halted");
    toast_push(ctx, "core halted; the magic smoke is out; reload the design to refill it");
}

#define CROSSTALK_CORE_CMDLINE_SCOPE_WIDTH_WORDS "uint8|uint16|uint32|int8|int16|int32"
#define CROSSTALK_CORE_SCRIPT_RUN_ARG_MAX 1
#define CROSSTALK_CORE_SCRIPT_HELP_ARG_MAX 1

typedef struct crosstalk_script_verb
{
    const char *name;
    uint32_t min_args;
    uint32_t max_args;
    void (*cb)(crosstalk_ctx_t *, const char *);
    const char *help;
    const char *arg;
} crosstalk_script_verb_t;

static uint32_t crosstalk_script_name_len_get(const char *name)
{
    uint32_t len = 0;

    if (LDG_UNLIKELY(name == 0x0)) { return len; }

    while (name[len] != LDG_STR_TERM) { len = len + 0x1; }

    return len;
}

static uint32_t crosstalk_script_verb_tbl_get(const crosstalk_script_verb_t **out, uint32_t *out_cunt)
{
    static const crosstalk_script_verb_t tbl[] = {
        { "w", 0x0, 0x0, crosstalk_verb_st, "w; write the board", 0x0 },
        { "q", 0x0, 0x1, crosstalk_verb_quit, "q [force]; leave to the main menu", "force" },
        { "wq", 0x0, 0x0, crosstalk_verb_st_quit, "wq; write, then leave to the menu", 0x0 },
        { "qqq", 0x0, 0x0, crosstalk_verb_exit, "qqq; exit the game now", 0x0 },
        { "chat", 0x0, 0x0, crosstalk_verb_chat, "chat; toggle the chat window", 0x0 },
        { "totals", 0x0, 0x0, crosstalk_verb_totals, "totals; toggle the totals window", 0x0 },
        { "term", 0x0, 0x2, crosstalk_verb_term, "term [port|idx|close [idx]]; terminal tabs; a port opens the next free tab, an idx focuses it", "@sig|host|close" },
        { "scope", 0x0, 0x4, crosstalk_verb_scope, "scope [port|mem <0xADDR|symbol> [width] [fixed]|xy <trace> <trace>]; oscilloscope on a design signal, the board oscillator, or guest memory; width is uint8, uint16, uint32, int8, int16 or int32; fixed reads it as 16.16", "@sig|mem|xy @sig|@mem " CROSSTALK_CORE_CMDLINE_SCOPE_WIDTH_WORDS " fixed" },
        { "ind", 0x0, 0x1, crosstalk_verb_ind, "ind [port]; indicator lamps for a signal", "@sig" },
        { "serial", 0x0, 0x4, crosstalk_verb_serial, "serial [path | port@baud:parity:data:stop | value:port | \"text\":port | port:\"pattern\"[:\"action\"]]; bare serial lists ports; order encodes direction", "@path" },
        { "pipe", 0x0, 0x1, crosstalk_verb_pipe, "pipe [port]; drive the wire from a design port", "@sig" },
        { "gate", 0x0, 0x0, crosstalk_verb_gate, "gate; drop from the hdl simulator to the gate-level netlist", 0x0 },
        { "emu", 0x0, 0x1, crosstalk_verb_emu, "emu [path|off]; run the design on its own emulator; bare emu reports what is attached", "@path|off" },
        { "net", 0x0, 0x1, crosstalk_verb_net, "net [path|off]; serial endpoint the emulated guest's network bytes move through; bare net reports it", "@path|off" },
        { "emit", 0x0, 0x0, crosstalk_verb_emit, "emit; compile the loaded design into a native component", 0x0 },
        { "profile", 0x0, 0x0, crosstalk_verb_profile, "profile [start [name] | end | reset]; profiler window, captures, rebase", "start|end|reset" },
        { "prof", 0x1, 0x1, crosstalk_verb_prof, CROSSTALK_CORE_PROF_HELP_TEXT, "on|off" },
        { "publish", 0x1, 0x1, crosstalk_verb_publish, "publish <isa>; share the running design on the Workshop", 0x0 },
        { "clk", 0x0, 0x1, crosstalk_clk_pin, "clk [hz]; hold the component at a clock; no value frees it", 0x0 },
        { "mem", 0x2, 0x3, crosstalk_memrd, "mem <signal> <hex addr> [cunt]; dump memory to the log", "@mem" },
        { "memview", 0x0, 0x2, crosstalk_verb_memview, "memview [signal] [addr|@sig]; memory window; watch/snap subverbs; debugger repl at the window bottom, type help there", "@mem" },
        { "memfind", 0x1, 0x2, crosstalk_memfind, "memfind <hex bytes|\"text\">; search the bound memory", 0x0 },
        { "text", 0x0, 0x2, crosstalk_text_attach, "text [signal] [base]; CGA text screen from memory", "@mem" },
        { "snap", 0x0, 0x1, crosstalk_snap_wr, "snap [name|list]; write the design state; bare snap refuses if one exists; list logs existing snaps", "list" },
        { "resume", 0x0, 0x1, crosstalk_snap_rd, "resume [name]; restore design state", 0x0 },
        { "display", 0x3, 0x3, crosstalk_display_attach, "display <hsync> <vsync> <video>; wire a design's video out to the monitor", "@sig @sig @sig" },
        { "gfx", 0x0, 0x4, crosstalk_gfx_attach, "gfx [signal] [base] [w] [h]; show raw video memory on the monitor", "@mem" },
        { "cgacast", 0x0, 0x4, crosstalk_cgacast_attach, "cgacast [signal] [base] [w] [h] | cgacast off; stream the framebuffer to a real CGA screen over serial", "@mem|off" },
        { "ld", 0x1, 0x3, crosstalk_dir_ld, "ld <dir> <a.sv,b.sv|url> <top>; program a fileset; a URL is git-cloned with the system's git under the game data dir", "@dir @fileset @mod" },
        { "midi", 0x1, 0x2, crosstalk_midi_attach, "midi <pin> [device]; clock a MIDI device onto a design pin at 31250 baud", "@sig" },
        { "midiplay", 0x0, 0x1, crosstalk_midi_play, "midiplay [path]; play a MIDI file into the design; no path stops", "@path" },
        { "midinote", 0x1, 0x2, crosstalk_midi_note, "midinote <note> [velocity]; send one note; zero velocity releases it", 0x0 },
        { "sound", 0x0, 0x3, crosstalk_verb_sound, "sound [port] [rate] [period]; sample a design port as sound", "@sig" },
        { "render", 0x4, 0x8, crosstalk_verb_render, "render <port> <secs> <rate> <path>; bounce a design port to a 32-bit PCM wav", "@sig @none @none @path" },
        { "poke", 0x1, 0x2, crosstalk_poke, "poke <sig> [val]; write a signal, or read it", "@sig" },
        { "switch", 0x1, 0x2, crosstalk_verb_switch, "switch <name> [pos]; move a described switch, or read its position", 0x0 },
        { "receptacle", 0x2, 0x4, crosstalk_receptacle_attach, "receptacle <data> <ready> [ack] [scan]; input into the design; scan sends PC keyboard scancodes", "@sig @sig @sig|scan scan" },
        { "memld", 0x2, 0x3, crosstalk_memld, "memld <sig> <path> [off]; load a file into a design memory", "@mem @path" },
        { "layout", 0x0, 0x1, crosstalk_verb_layout, "layout [dump]; print the current editor's window layout as default-table rows", "dump" },
        { "progress", 0x0, 0x0, crosstalk_verb_progress, "progress; toggle the progress window: named, ported and verified against the artifact", 0x0 },
        { "tasks", 0x0, 0x0, crosstalk_verb_tasks, "tasks; toggle the task window: add, tick off and remove what is left to do", 0x0 },
        { "re", 0x0, 0x8, crosstalk_verb_re, "re open <emiru|elf|raw> [base] [isa] | close | seg | rd <addr> [n] | dis [addr] [n] | name|note|func|struct|field|patch ... | cc <n> \"<step>\" | import | save [path] | undo | assert <path.h> | build <src.c> [out] | diff ref|cand <path> [sig] [off] | diff cycles|mask|unmask|run | ls [kind]; the reversing workbench; bare re reports state", "open|close|seg|rd|dis|name|note|func|struct|field|patch|cc|import|save|undo|assert|build|diff|ls" },
        { "syms", 0x0, 0x1, crosstalk_verb_syms, "syms [path.elf]; read the running program's symbols so memview can go to them by name; bare syms reports what is loaded", "@path" },
        { "ident", 0x1, 0x1, crosstalk_verb_ident, "ident 0xADDR|<symbol>[+0xOFF]; name the symbol an address falls inside, with its offset from the symbol's start", 0x0 },
        { "gamma", 0x0, 0x1, crosstalk_verb_gamma, "gamma [val]; monitor gamma curve, 1.0 flat, 1.8 lifts a linear guest stream; bare gamma reports", 0x0 },
        { "hlt", 0x0, 0x6, crosstalk_verb_hlt, "hlt [off|causes]; halt the core and hold its state; causes halt it before a fault dispatches; all leaves out unaligned, which the kernel repairs, so name it to watch for it", "off" },
        { "on", 0x0, 0x2, crosstalk_verb_on, "on [load|save|exit|fault] [path.xts|off]; run a script when the game does something; bare on lists", "load|save|exit|fault @path|off" },
        { "disk", 0x0, 0x2, crosstalk_verb_disk, "disk [path.dsk|sync|off] [drive]; attach media to an emulator drive, 0-7; sync writes the image back; bare disk lists drives", "@path|sync|off" },
        { "dsu", 0x0, 0x1, crosstalk_verb_dsu, "dsu [play|stop]; route the emulator sound card to audio; bare dsu reports buffer, cursor, rate, running", "play|stop" },
        { "delay", 0x1, 0x2, crosstalk_verb_delay, "delay <n> [ns|us|ms|s]; hold the script for a span; a bare number is ns", "@none ns|us|ms|s" },
        { "rst", 0x0, 0x0, crosstalk_verb_rst, "rst; power-cycle the emulator; core reset, memory images reloaded, disk kept", 0x0 },
        { "break", 0x0, 0x3, crosstalk_verb_break, "break [addr|off|skip <n>|cond <reg> <min>]; halt the emulator when the program locator reaches an address; skip ignores the first n hits, cond halts only when drN is at least min; bare break reports the armed one", "off|skip|cond" },
        { "stackview", 0x0, 0x1, crosstalk_verb_stackview, "stackview [memory]; the live stack from sp to its base, frozen at a fault; memory defaults to cells; type a signal inside to record its history", "@mem" },
        { "scan", 0x0, 0x1, crosstalk_verb_scan, "scan [path]; hand a file to the host antimalware provider; bare scan reports provider state and counts", "@path" },
        { "hcf", 0x0, 0x0, crosstalk_verb_hcf, 0x0, 0x0 },
        { "hostmem", 0x0, 0x1, crosstalk_verb_hostmem, "hostmem [dump]; log LDG heap totals; dump lists live allocations", "dump" },
        { "calc", 0x0, 0x0, crosstalk_verb_calc, "calc; toggle the hex and binary calculator", 0x0 },
        { "regs", 0x0, 0x0, crosstalk_verb_regs, "regs; toggle the live register window", 0x0 },
        { "guide", 0x0, 0x1, crosstalk_verb_guide, "guide [name|next|back|show|close]; step through a guide; show me opens and flashes the window a step points at; bare guide lists names", "@guide|next|back|show|close" },
        { "brainomatic", 0x0, 0x0, crosstalk_verb_brainomatic, "brainomatic; toggle the ISA cheat sheet", 0x0 },
        { "progview", 0x0, 0x1, crosstalk_verb_progview, "progview [signal]; live disassembly following loc, no profiler tax", "@sig" },
        { "pipeview", 0x0, 0x8, crosstalk_verb_pipeview, "pipeview [stage sigs]; disassemble one insn-word sig per stage, in order", "@sig @sig @sig @sig @sig @sig @sig @sig" },
        { "analyzer", 0x0, 0x4, crosstalk_verb_analyzer, "analyzer [sigs|span N|off]; cycle-accurate multi-lane digital traces", "@sig|span|off @sig @sig @sig" },
        { "feed", 0x0, 0x4, crosstalk_verb_feed, "feed [sigs|off]; cycle-stamped change events; exc values annotated", "@sig|off @sig @sig @sig" },
        { "program", 0x0, 0x1, crosstalk_verb_program, "program [path]; program the editor's file; bare uses the last path copied in the editor", "@path" }
    };

    if (LDG_UNLIKELY(out == 0x0 || out_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = tbl;
    *out_cunt = (uint32_t)(sizeof(tbl) / sizeof(tbl[0]));

    return LDG_ERR_AOK;
}

static void crosstalk_script_help_push(crosstalk_ctx_t *ctx, const char *text)
{
    char line[CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t width = ctx->cmdline.drawer_col_cunt;
    uint32_t len = (uint32_t)strlen(text);
    uint32_t at = 0;

    if (width == 0 || width >= (uint32_t)CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CAP) { width = (uint32_t)CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CAP - (uint32_t)LDG_STR_TERM_SIZE; }

    while (at < len)
    {
        uint32_t take = 0;

        while (at < len && text[at] == ' ') { at = at + 0x1; }

        if (at >= len) { break; }

        take = len - at;

        if (take > width)
        {
            uint32_t cut = width;

            while (cut > 0 && text[at + cut] != ' ') { cut = cut - 0x1; }

            take = cut > 0 ? cut : width;
        }

        memcpy(line, &text[at], (uint64_t)take);
        line[take] = LDG_STR_TERM;
        at = at + take;

        if (crosstalk_core_cmdline_out_push(&ctx->cmdline, line) != LDG_ERR_AOK) { return; }
    }
}

static void crosstalk_script_help(crosstalk_ctx_t *ctx, const char *arg)
{
    char list[CROSSTALK_CORE_SCRIPT_HELP_LIST_CAP] = LDG_ARR_ZERO_INIT;
    char note[CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CAP] = LDG_ARR_ZERO_INIT;
    const crosstalk_script_verb_t *tbl = 0x0;
    uint32_t tbl_cunt = 0;
    uint32_t used = 0;
    uint32_t idx = 0;

    if (crosstalk_script_verb_tbl_get(&tbl, &tbl_cunt) != LDG_ERR_AOK) { return; }

    if (arg[0] == LDG_STR_TERM)
    {
        used = (uint32_t)snprintf(list, sizeof(list), "%.31s", CROSSTALK_CORE_SCRIPT_HELP_LIST_HD);

        for (idx = 0; idx < tbl_cunt && used < (uint32_t)sizeof(list); idx = idx + 0x1)
        {
            if (tbl[idx].help == 0x0) { continue; }

            used = used + (uint32_t)snprintf(&list[used], sizeof(list) - used, "%.1s%.31s", CROSSTALK_CORE_SCRIPT_HELP_SEP, tbl[idx].name);
        }

        if (used < (uint32_t)sizeof(list)) { used = used + (uint32_t)snprintf(&list[used], sizeof(list) - used, "%.1s%.31s%.1s%.31s", CROSSTALK_CORE_SCRIPT_HELP_SEP, CROSSTALK_CORE_SCRIPT_RUN_NAME, CROSSTALK_CORE_SCRIPT_HELP_SEP, CROSSTALK_CORE_SCRIPT_HELP_NAME); }

        crosstalk_script_help_push(ctx, list);
        crosstalk_script_help_push(ctx, CROSSTALK_CORE_SCRIPT_HELP_HELP);

        return;
    }

    if (strcmp(arg, CROSSTALK_CORE_SCRIPT_RUN_NAME) == 0) { crosstalk_script_help_push(ctx, CROSSTALK_CORE_SCRIPT_RUN_HELP); return; }

    if (strcmp(arg, CROSSTALK_CORE_SCRIPT_HELP_NAME) == 0) { crosstalk_script_help_push(ctx, CROSSTALK_CORE_SCRIPT_HELP_HELP); return; }

    for (idx = 0; idx < tbl_cunt; idx = idx + 0x1)
    {
        if (tbl[idx].help == 0x0 || strcmp(tbl[idx].name, arg) != 0) { continue; }

        crosstalk_script_help_push(ctx, tbl[idx].help);

        return;
    }

    if (snprintf(note, sizeof(note), CROSSTALK_CORE_SCRIPT_HELP_UNKNOWN_FMT, arg) > 0) { crosstalk_script_help_push(ctx, note); }
}

#define CROSSTALK_CORE_COMPLETE_KIND_SIG "@sig"
#define CROSSTALK_CORE_COMPLETE_KIND_MEM "@mem"
#define CROSSTALK_CORE_COMPLETE_KIND_MOD "@mod"
#define CROSSTALK_CORE_COMPLETE_KIND_PATH "@path"
#define CROSSTALK_CORE_COMPLETE_KIND_DIR "@dir"
#define CROSSTALK_CORE_COMPLETE_KIND_FILESET "@fileset"
#define CROSSTALK_CORE_COMPLETE_KIND_GUIDE "@guide"
#define CROSSTALK_CORE_COMPLETE_KIND_VERB "@verb"
#define CROSSTALK_CORE_COMPLETE_KIND_NONE "@none"
#define CROSSTALK_CORE_COMPLETE_HELP_SPEC CROSSTALK_CORE_COMPLETE_KIND_VERB
#define CROSSTALK_CORE_COMPLETE_RUN_SPEC CROSSTALK_CORE_COMPLETE_KIND_PATH
#define CROSSTALK_CORE_COMPLETE_POS_SEP ' '
#define CROSSTALK_CORE_COMPLETE_ALT_SEP '|'
#define CROSSTALK_CORE_COMPLETE_PATH_SEP '/'
#define CROSSTALK_CORE_COMPLETE_STMT_END ';'
#define CROSSTALK_CORE_COMPLETE_HIDDEN_LEAD '.'
#define CROSSTALK_CORE_COMPLETE_HOME_LEAD '~'
#define CROSSTALK_CORE_COMPLETE_PATH_HERE "."
#define CROSSTALK_CORE_COMPLETE_PATH_UP ".."
#define CROSSTALK_CORE_COMPLETE_ALT_CAP 64
#define CROSSTALK_CORE_COMPLETE_VERB_TOK 0
#define CROSSTALK_CORE_COMPLETE_DIR_TOK 1
#define CROSSTALK_CORE_COMPLETE_LEAD_NONE 0
#define CROSSTALK_CORE_COMPLETE_ENTRY_FMT "%s%s%s"
#define CROSSTALK_CORE_COMPLETE_DIR_MARK "/"
#define CROSSTALK_CORE_COMPLETE_FILE_MARK ""

typedef enum crosstalk_core_complete_design
{
    CROSSTALK_CORE_COMPLETE_DESIGN_SIG = 0,
    CROSSTALK_CORE_COMPLETE_DESIGN_MEM,
    CROSSTALK_CORE_COMPLETE_DESIGN_MOD
} crosstalk_core_complete_design_t;

static uint32_t crosstalk_complete_put(char (*out)[CROSSTALK_CORE_CMDLINE_CANDIDATE_CAP], uint32_t out_cap, uint32_t *out_cunt, const char *text, const char *word)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(out == 0x0 || out_cunt == 0x0 || text == 0x0 || word == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (*out_cunt >= out_cap) { return LDG_ERR_FULL; }

    if (strncmp(text, word, strlen(word)) != 0) { return LDG_ERR_AOK; }

    ret = ldg_str_rbrcpy(out[*out_cunt], text, (uint64_t)CROSSTALK_CORE_CMDLINE_CANDIDATE_CAP);
    if (ret == LDG_ERR_STR_TRUNC) { return LDG_ERR_AOK; }

    if (ret != LDG_ERR_AOK) { return ret; }

    *out_cunt = *out_cunt + 0x1;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_complete_tok_get(const char *text, uint32_t idx, char sep, char *out, uint32_t cap)
{
    uint32_t at = 0;
    uint32_t tok = 0;
    uint32_t len = 0;

    if (LDG_UNLIKELY(text == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_BOUNDS; }

    out[0] = LDG_STR_TERM;

    while (text[at] != LDG_STR_TERM)
    {
        while (text[at] == sep) { at = at + 0x1; }

        if (text[at] == LDG_STR_TERM) { break; }

        len = 0;

        while (text[at + len] != LDG_STR_TERM && text[at + len] != sep) { len = len + 0x1; }

        if (tok == idx)
        {
            if (len + (uint32_t)LDG_STR_TERM_SIZE > cap) { return LDG_ERR_STR_TRUNC; }

            memcpy(out, &text[at], (uint64_t)len);
            out[len] = LDG_STR_TERM;

            return LDG_ERR_AOK;
        }

        tok = tok + 0x1;
        at = at + len;
    }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t crosstalk_complete_design_put(crosstalk_ctx_t *ctx, uint32_t kind, const char *word, char (*out)[CROSSTALK_CORE_CMDLINE_CANDIDATE_CAP], uint32_t out_cap, uint32_t *out_cunt)
{
    char name[CROSSTALK_CORE_CMDLINE_CANDIDATE_CAP] = LDG_ARR_ZERO_INIT;
    const crosstalk_hdl_exec_ctx_t *exec = 0x0;
    const crosstalk_hdl_ir_t *ir_ctx = 0x0;
    uint32_t idx = 0;
    uint32_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || word == 0x0 || out == 0x0 || out_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (crosstalk_eda_ic_fpga_slot_exec_get(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, &exec) != LDG_ERR_AOK || exec == 0x0 || exec->ir_ctx == 0x0 || exec->src == 0x0) { return LDG_ERR_AOK; }

    ir_ctx = exec->ir_ctx;

    if (kind == (uint32_t)CROSSTALK_CORE_COMPLETE_DESIGN_MOD)
    {
        ret = crosstalk_complete_put(out, out_cap, out_cunt, ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT].top, word);
        if (ret != LDG_ERR_AOK) { return ret; }

        for (idx = 0; idx < ir_ctx->inst_cunt; idx = idx + 0x1)
        {
            len = ir_ctx->insts[idx].module_name.len;
            if (len == 0 || len + (uint32_t)LDG_STR_TERM_SIZE > (uint32_t)sizeof(name)) { continue; }

            memcpy(name, exec->src + ir_ctx->insts[idx].module_name.off, (uint64_t)len);
            name[len] = LDG_STR_TERM;

            ret = crosstalk_complete_put(out, out_cap, out_cunt, name, word);
            if (ret != LDG_ERR_AOK) { return ret; }
        }

        return LDG_ERR_AOK;
    }

    for (idx = 0; idx < ir_ctx->sig_cunt; idx = idx + 0x1)
    {
        if (kind == (uint32_t)CROSSTALK_CORE_COMPLETE_DESIGN_MEM && ir_ctx->sigs[idx].kind != (uint32_t)CROSSTALK_HDL_IR_SIG_MEM) { continue; }

        if (crosstalk_hdl_exec_sig_name_get(exec, idx, name, (uint32_t)sizeof(name)) != LDG_ERR_AOK) { continue; }

        ret = crosstalk_complete_put(out, out_cap, out_cunt, name, word);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_complete_path_put(crosstalk_ctx_t *ctx, const char *base, const char *word, uint8_t dir_only, uint8_t file_only, char (*out)[CROSSTALK_CORE_CMDLINE_CANDIDATE_CAP], uint32_t out_cap, uint32_t *out_cunt)
{
    char lead[CROSSTALK_CORE_CMDLINE_CANDIDATE_CAP] = LDG_ARR_ZERO_INIT;
    char text[CROSSTALK_CORE_CMDLINE_CANDIDATE_CAP] = LDG_ARR_ZERO_INIT;
    char raw[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char expanded[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char full[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    ldg_io_entry_t ent = LDG_STRUCT_ZERO_INIT;
    ldg_io_dir_t *dir = 0x0;
    const char *leaf = 0x0;
    const char *name = 0x0;
    uint64_t lead_len = 0;
    uint64_t leaf_len = 0;
    uint8_t ent_dir = LDG_TRUTH_FALSE;
    uint32_t wrote = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || word == 0x0 || out == 0x0 || out_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    leaf = strrchr(word, CROSSTALK_CORE_COMPLETE_PATH_SEP);
    lead_len = leaf != 0x0 ? (uint64_t)(leaf - word) + (uint64_t)LDG_STR_TERM_SIZE : (uint64_t)CROSSTALK_CORE_COMPLETE_LEAD_NONE;
    if (lead_len + (uint64_t)LDG_STR_TERM_SIZE > (uint64_t)sizeof(lead)) { return LDG_ERR_AOK; }

    memcpy(lead, word, lead_len);
    lead[lead_len] = LDG_STR_TERM;
    leaf = &word[lead_len];
    leaf_len = (uint64_t)strlen(leaf);

    if (base != 0x0) { ret = ldg_io_path_join(base, lead[0] != LDG_STR_TERM ? lead : CROSSTALK_CORE_COMPLETE_PATH_HERE, raw, (uint64_t)sizeof(raw)); }
    else { ret = ldg_str_rbrcpy(raw, lead[0] != LDG_STR_TERM ? lead : CROSSTALK_CORE_COMPLETE_PATH_HERE, (uint64_t)sizeof(raw)); }

    if (ret != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    if (raw[0] == CROSSTALK_CORE_COMPLETE_HOME_LEAD) { ret = ldg_io_path_expand(raw, expanded, (uint64_t)sizeof(expanded)); }
    else { ret = ldg_str_rbrcpy(expanded, raw, (uint64_t)sizeof(expanded)); }

    if (ret != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    if (crosstalk_script_path_resolve(ctx, expanded, full, (uint64_t)sizeof(full)) != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    if (ldg_io_dir_open(full, &dir) != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    while (ldg_io_dir_rd(dir, &ent) == LDG_ERR_AOK)
    {
        if (ldg_io_entry_name_get(&ent, &name) != LDG_ERR_AOK || name == 0x0) { continue; }

        if (strcmp(name, CROSSTALK_CORE_COMPLETE_PATH_HERE) == 0 || strcmp(name, CROSSTALK_CORE_COMPLETE_PATH_UP) == 0) { continue; }

        if (name[0] == CROSSTALK_CORE_COMPLETE_HIDDEN_LEAD && leaf[0] != CROSSTALK_CORE_COMPLETE_HIDDEN_LEAD) { continue; }

        if (strncmp(name, leaf, leaf_len) != 0) { continue; }

        ent_dir = ldg_io_entry_dir_is(&ent) == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

        if ((dir_only == LDG_TRUTH_TRUE && ent_dir != LDG_TRUTH_TRUE) || (file_only == LDG_TRUTH_TRUE && ent_dir == LDG_TRUTH_TRUE)) { continue; }

        wrote = (uint32_t)snprintf(text, sizeof(text), CROSSTALK_CORE_COMPLETE_ENTRY_FMT, lead, name, ent_dir == LDG_TRUTH_TRUE ? CROSSTALK_CORE_COMPLETE_DIR_MARK : CROSSTALK_CORE_COMPLETE_FILE_MARK);
        if ((uint64_t)wrote >= (uint64_t)sizeof(text)) { continue; }

        ret = crosstalk_complete_put(out, out_cap, out_cunt, text, word);
        if (ret != LDG_ERR_AOK) { break; }
    }

    if (ldg_io_dir_close(dir) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "complete: dir close failed; dir: %.255s", full); }

    return ret == LDG_ERR_FULL ? LDG_ERR_AOK : ret;
}

static uint32_t crosstalk_complete_alt_put(crosstalk_ctx_t *ctx, const char *alt, const char *line, const char *word, char (*out)[CROSSTALK_CORE_CMDLINE_CANDIDATE_CAP], uint32_t out_cap, uint32_t *out_cunt)
{
    char base[CROSSTALK_CORE_CMDLINE_CAP] = LDG_ARR_ZERO_INIT;
    const char *name = 0x0;
    const char *help = 0x0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || alt == 0x0 || line == 0x0 || word == 0x0 || out == 0x0 || out_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (strcmp(alt, CROSSTALK_CORE_COMPLETE_KIND_NONE) == 0) { return LDG_ERR_AOK; }

    if (strcmp(alt, CROSSTALK_CORE_COMPLETE_KIND_SIG) == 0) { return crosstalk_complete_design_put(ctx, (uint32_t)CROSSTALK_CORE_COMPLETE_DESIGN_SIG, word, out, out_cap, out_cunt); }

    if (strcmp(alt, CROSSTALK_CORE_COMPLETE_KIND_MEM) == 0) { return crosstalk_complete_design_put(ctx, (uint32_t)CROSSTALK_CORE_COMPLETE_DESIGN_MEM, word, out, out_cap, out_cunt); }

    if (strcmp(alt, CROSSTALK_CORE_COMPLETE_KIND_MOD) == 0) { return crosstalk_complete_design_put(ctx, (uint32_t)CROSSTALK_CORE_COMPLETE_DESIGN_MOD, word, out, out_cap, out_cunt); }

    if (strcmp(alt, CROSSTALK_CORE_COMPLETE_KIND_PATH) == 0) { return crosstalk_complete_path_put(ctx, 0x0, word, LDG_TRUTH_FALSE, LDG_TRUTH_FALSE, out, out_cap, out_cunt); }

    if (strcmp(alt, CROSSTALK_CORE_COMPLETE_KIND_DIR) == 0) { return crosstalk_complete_path_put(ctx, 0x0, word, LDG_TRUTH_TRUE, LDG_TRUTH_FALSE, out, out_cap, out_cunt); }

    if (strcmp(alt, CROSSTALK_CORE_COMPLETE_KIND_FILESET) == 0)
    {
        if (crosstalk_complete_tok_get(line, (uint32_t)CROSSTALK_CORE_COMPLETE_DIR_TOK, CROSSTALK_CORE_COMPLETE_POS_SEP, base, (uint32_t)sizeof(base)) != LDG_ERR_AOK) { return LDG_ERR_AOK; }

        return crosstalk_complete_path_put(ctx, base, word, LDG_TRUTH_FALSE, LDG_TRUTH_TRUE, out, out_cap, out_cunt);
    }

    if (strcmp(alt, CROSSTALK_CORE_COMPLETE_KIND_GUIDE) == 0)
    {
        for (idx = 0; crosstalk_ui_guide_name_get(&ctx->guide, idx, &name) == LDG_ERR_AOK; idx = idx + 0x1)
        {
            ret = crosstalk_complete_put(out, out_cap, out_cunt, name, word);
            if (ret != LDG_ERR_AOK) { return ret; }
        }

        return LDG_ERR_AOK;
    }

    if (strcmp(alt, CROSSTALK_CORE_COMPLETE_KIND_VERB) == 0)
    {
        for (idx = 0; crosstalk_core_cmdline_verb_get(&ctx->cmdline, idx, &name, &help) == LDG_ERR_AOK; idx = idx + 0x1)
        {
            ret = crosstalk_complete_put(out, out_cap, out_cunt, name, word);
            if (ret != LDG_ERR_AOK) { return ret; }
        }

        return LDG_ERR_AOK;
    }

    return crosstalk_complete_put(out, out_cap, out_cunt, alt, word);
}

static uint32_t crosstalk_cmdline_candidate_cb(void *user, const char *line, uint32_t arg_idx, const char *word, char (*out)[CROSSTALK_CORE_CMDLINE_CANDIDATE_CAP], uint32_t out_cap, uint32_t *out_cunt)
{
    crosstalk_ctx_t *ctx = (crosstalk_ctx_t *)user;
    const crosstalk_script_verb_t *tbl = 0x0;
    const char *spec = 0x0;
    char verb[CROSSTALK_CORE_CMDLINE_VERB_NAME_CAP] = LDG_ARR_ZERO_INIT;
    char pos[CROSSTALK_CORE_CMDLINE_CAP] = LDG_ARR_ZERO_INIT;
    char alt[CROSSTALK_CORE_COMPLETE_ALT_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t tbl_cunt = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || line == 0x0 || word == 0x0 || out == 0x0 || out_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (crosstalk_complete_tok_get(line, (uint32_t)CROSSTALK_CORE_COMPLETE_VERB_TOK, CROSSTALK_CORE_COMPLETE_POS_SEP, verb, (uint32_t)sizeof(verb)) != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    if (strcmp(verb, CROSSTALK_CORE_SCRIPT_HELP_NAME) == 0) { spec = CROSSTALK_CORE_COMPLETE_HELP_SPEC; }
    else if (strcmp(verb, CROSSTALK_CORE_SCRIPT_RUN_NAME) == 0) { spec = CROSSTALK_CORE_COMPLETE_RUN_SPEC; }
    else
    {
        ret = crosstalk_script_verb_tbl_get(&tbl, &tbl_cunt);
        if (ret != LDG_ERR_AOK) { return ret; }

        for (idx = 0; idx < tbl_cunt; idx = idx + 0x1) { if (strcmp(tbl[idx].name, verb) == 0) { spec = tbl[idx].arg; break; } }
    }

    if (spec == 0x0) { return LDG_ERR_AOK; }

    if (crosstalk_complete_tok_get(spec, arg_idx, CROSSTALK_CORE_COMPLETE_POS_SEP, pos, (uint32_t)sizeof(pos)) != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    for (idx = 0; crosstalk_complete_tok_get(pos, idx, CROSSTALK_CORE_COMPLETE_ALT_SEP, alt, (uint32_t)sizeof(alt)) == LDG_ERR_AOK; idx = idx + 0x1)
    {
        ret = crosstalk_complete_alt_put(ctx, alt, line, word, out, out_cap, out_cunt);
        if (ret == LDG_ERR_FULL) { break; }

        if (ret != LDG_ERR_AOK) { return ret; }
    }

    if (crosstalk_complete_tok_get(spec, arg_idx + 0x1, CROSSTALK_CORE_COMPLETE_POS_SEP, pos, (uint32_t)sizeof(pos)) == LDG_ERR_AOK) { return LDG_ERR_AOK; }

    for (idx = 0; idx < *out_cunt; idx = idx + 0x1)
    {
        uint64_t len = (uint64_t)strlen(out[idx]);

        if (len == 0 || out[idx][len - (uint64_t)LDG_STR_TERM_SIZE] == CROSSTALK_CORE_COMPLETE_PATH_SEP || len + (uint64_t)LDG_STR_TERM_SIZE >= (uint64_t)CROSSTALK_CORE_CMDLINE_CANDIDATE_CAP) { continue; }

        out[idx][len] = CROSSTALK_CORE_COMPLETE_STMT_END;
        out[idx][len + (uint64_t)LDG_STR_TERM_SIZE] = LDG_STR_TERM;
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_script_verb_cb(void *user, const char *name, uint32_t name_len, const crosstalk_eda_script_val_t *arg, uint32_t arg_cunt, crosstalk_eda_script_val_t *out)
{
    char joined[CROSSTALK_CORE_CMDLINE_CAP] = LDG_ARR_ZERO_INIT;
    const crosstalk_script_verb_t *tbl = 0x0;
    crosstalk_ctx_t *ctx = (crosstalk_ctx_t *)user;
    uint32_t tbl_cunt = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(out, 0x0, sizeof(*out));

    ret = crosstalk_script_args_join(arg, arg_cunt, joined, (uint32_t)sizeof(joined));
    if (ret != LDG_ERR_AOK) { return ret; }

    if (name_len == (uint32_t)CROSSTALK_CORE_SCRIPT_RUN_LEN && memcmp(name, CROSSTALK_CORE_SCRIPT_RUN_NAME, name_len) == 0)
    {
        char note[CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CAP] = LDG_ARR_ZERO_INIT;

        ret = crosstalk_defer_push(ctx, (uint32_t)CROSSTALK_CORE_DEFER_RUN, joined, crosstalk_script_name_len_get(joined));
        if (ret == LDG_ERR_AOK) { return LDG_ERR_AOK; }

        if (snprintf(note, sizeof(note), CROSSTALK_CORE_SCRIPT_RUN_FAIL_FMT, ret, joined) < 0) { return ret; }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "script: %.128s", note);

        if (crosstalk_core_cmdline_out_push(&ctx->cmdline, note) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "script run note push failed"); }

        return ret;
    }

    if (name_len == (uint32_t)CROSSTALK_CORE_SCRIPT_HELP_LEN && memcmp(name, CROSSTALK_CORE_SCRIPT_HELP_NAME, name_len) == 0)
    {
        crosstalk_script_help(ctx, joined);

        return LDG_ERR_AOK;
    }

    ret = crosstalk_script_verb_tbl_get(&tbl, &tbl_cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    for (idx = 0; idx < tbl_cunt; idx = idx + 0x1)
    {
        if (crosstalk_script_name_len_get(tbl[idx].name) != name_len) { continue; }

        if (memcmp(tbl[idx].name, name, name_len) != 0) { continue; }

        tbl[idx].cb(ctx, joined);

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t crosstalk_script_builtins_reg(crosstalk_ctx_t *ctx)
{
    const crosstalk_script_verb_t *tbl = 0x0;
    uint32_t tbl_cunt = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_script_verb_tbl_get(&tbl, &tbl_cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    for (idx = 0; idx < tbl_cunt; idx = idx + 0x1)
    {
        ret = crosstalk_eda_script_builtin_reg(&ctx->eda.script, tbl[idx].name, crosstalk_script_verb_cb, tbl[idx].min_args, tbl[idx].max_args, 0x0);
        if (ret != LDG_ERR_AOK) { return ret; }

        if (tbl[idx].help != 0x0)
        {
            ret = crosstalk_core_cmdline_verb_reg(&ctx->cmdline, tbl[idx].name, tbl[idx].help, tbl[idx].max_args);
            if (ret != LDG_ERR_AOK) { return ret; }
        }
    }

    ret = crosstalk_core_cmdline_verb_reg(&ctx->cmdline, CROSSTALK_CORE_SCRIPT_RUN_NAME, CROSSTALK_CORE_SCRIPT_RUN_HELP, CROSSTALK_CORE_SCRIPT_RUN_ARG_MAX);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_eda_script_builtin_reg(&ctx->eda.script, CROSSTALK_CORE_SCRIPT_RUN_NAME, crosstalk_script_verb_cb, 0x1, 0x1, 0x0);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_core_cmdline_verb_reg(&ctx->cmdline, CROSSTALK_CORE_SCRIPT_HELP_NAME, CROSSTALK_CORE_SCRIPT_HELP_HELP, CROSSTALK_CORE_SCRIPT_HELP_ARG_MAX);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_core_cmdline_candidate_cb_set(&ctx->cmdline, crosstalk_cmdline_candidate_cb, ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    return crosstalk_eda_script_builtin_reg(&ctx->eda.script, CROSSTALK_CORE_SCRIPT_HELP_NAME, crosstalk_script_verb_cb, 0x0, 0x1, 0x0);
}

static void crosstalk_systask_out_cb(void *user, const char *line)
{
    crosstalk_ctx_t *ctx = (crosstalk_ctx_t *)user;

    if (LDG_UNLIKELY(ctx == 0x0 || line == 0x0)) { return; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "hdl: display; text: %.255s", line);

    if (crosstalk_core_cmdline_out_push(&ctx->cmdline, line) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "hdl: display push failed"); }
}

#define CROSSTALK_CORE_DOOR_CODE_LINE "0451;"
#define CROSSTALK_CORE_DOOR_CODE_TEXT "If you wanna make enemies, try to change something."

static void crosstalk_script_line_exec(crosstalk_ctx_t *ctx, const char *line)
{
    char text[CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CAP] = LDG_ARR_ZERO_INIT;
    const crosstalk_eda_script_diag_t *diag = 0x0;
    uint32_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || line == 0x0)) { return; }

    if (line[0] == LDG_STR_TERM) { return; }

    if (strcmp(line, CROSSTALK_CORE_DOOR_CODE_LINE) == 0)
    {
        ret = crosstalk_core_cmdline_out_push(&ctx->cmdline, CROSSTALK_CORE_DOOR_CODE_TEXT);

        return;
    }

    while (line[len] != LDG_STR_TERM) { len = len + 0x1; }

    ctx->console_exec_is = LDG_TRUTH_TRUE;

    ret = crosstalk_eda_script_exec(&ctx->eda.script, 0x0, line, len);

    ctx->console_exec_is = LDG_TRUTH_FALSE;

    crosstalk_defer_drain(ctx);

    if (ret == LDG_ERR_AOK) { return; }

    if (crosstalk_eda_script_diag_get(&ctx->eda.script, &diag) != LDG_ERR_AOK) { return; }

    if (snprintf(text, sizeof(text), CROSSTALK_CORE_SCRIPT_DIAG_FMT, diag->line, diag->col, diag->msg) < 0) { return; }

    if (crosstalk_core_cmdline_out_push(&ctx->cmdline, text) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "script diag push failed"); }

    if (diag->suggestion[0] == LDG_STR_TERM) { return; }

    if (snprintf(text, sizeof(text), CROSSTALK_CORE_SCRIPT_SUGGESTION_FMT, diag->suggestion) < 0) { return; }

    if (crosstalk_core_cmdline_out_push(&ctx->cmdline, text) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "script suggestion push failed"); }
}

static void crosstalk_cpuset_reserve(crosstalk_ctx_t *ctx)
{
    ldg_sys_cpuset_t set[LDG_SYS_CPUSET_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t rest[LDG_SYS_CPUSET_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t cunt = 0;
    uint32_t rest_cunt = 0;
    uint32_t core_pick = 0;
    uint32_t cache_pick = 0;
    uint32_t best = 0;
    uint8_t found = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    ret = ldg_sys_cpuset_list(set, (uint64_t)LDG_SYS_CPUSET_MAX, &cunt);

    if (ret == LDG_ERR_UNSUPPORTED)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: cpuset unsupported; the stepping core stays shared");
        return;
    }

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: cpuset list failed; err: %u32", ret);
        return;
    }

    if (cunt < (uint64_t)CROSSTALK_CORE_CPUSET_MIN_CUNT)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: cpuset reserve skipped; sets: %u64", cunt);
        return;
    }

    for (uint64_t at = 0; at < cunt; at++)
    {
        if (set[at].core_idx <= (uint8_t)CROSSTALK_CORE_CPUSET_FIRST_CORE) { continue; }

        if (found == LDG_TRUTH_TRUE && set[at].efficiency_class < best) { continue; }

        best = set[at].efficiency_class;
        core_pick = (uint32_t)set[at].core_idx;
        found = LDG_TRUTH_TRUE;
    }

    if (found != LDG_TRUTH_TRUE)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cpuset reserve found no eligible core");
        return;
    }

    for (uint64_t at = 0; at < cunt; at++) { if ((uint32_t)set[at].core_idx == core_pick) { cache_pick = (uint32_t)set[at].cache_idx; } }

    for (uint64_t at = 0; at < cunt; at++) { if ((uint32_t)set[at].cache_idx != cache_pick)
        {
            rest[rest_cunt] = set[at].id;
            rest_cunt = rest_cunt + 0x1;
        }
    }

    if (rest_cunt == 0) { for (uint64_t at = 0; at < cunt; at++)
        {
            if ((uint32_t)set[at].core_idx == core_pick) { continue; }

            rest[rest_cunt] = set[at].id;
            rest_cunt = rest_cunt + 0x1;
        }
    }

    for (uint64_t at = 0; at < cunt; at++)
    {
        if ((uint32_t)set[at].core_idx != core_pick) { continue; }

        if (ctx->cpuset_cunt >= (uint32_t)CROSSTALK_CORE_CPUSET_SIBLING_CAP) { continue; }

        ctx->cpuset_id[ctx->cpuset_cunt] = set[at].id;
        ctx->cpuset_cunt = ctx->cpuset_cunt + 0x1;
    }

    if (ctx->cpuset_cunt == 0 || rest_cunt == 0)
    {
        ctx->cpuset_cunt = 0;
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cpuset reserve produced an empty split");

        return;
    }

    ret = ldg_sys_cpuset_proc_default_set(rest, rest_cunt);

    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: cpuset proc default failed; err: %u32", ret);
        ctx->cpuset_cunt = 0;

        return;
    }

    ctx->cpuset_ok = LDG_TRUTH_TRUE;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: cpuset reserved; core: %u32; cache: %u32; class: %u32; sets: %u64; siblings: %u32; rest: %u32", core_pick, cache_pick, best, cunt, ctx->cpuset_cunt, rest_cunt);
}

uint32_t crosstalk_core_create(crosstalk_ctx_t **out)
{
    ldg_sched_conf_t conf = LDG_STRUCT_ZERO_INIT;
    crosstalk_ctx_t *ctx = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "crosstalk: create begin");

    ret = ldg_sys_proc_self_priority_set(LDG_SYS_PROC_PRIORITY_NORMAL);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "crosstalk: process priority; lvl: normal; err: %u32", ret);

    ret = ldg_mem_alloc(sizeof(crosstalk_ctx_t), (void **)&ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    memset(ctx, 0x0, sizeof(crosstalk_ctx_t));
    settings_default_fill(&ctx->game.settings);

    crosstalk_cpuset_reserve(ctx);

    ctx->sound_probe_handle = UINT32_MAX;
    ctx->sound_stream_handle = UINT32_MAX;
    ctx->scope_probe_handle = UINT32_MAX;
    ctx->scope_stream_handle = UINT32_MAX;
    ctx->dsu_probe_handle = UINT32_MAX;
    ctx->dsu_stream_handle = UINT32_MAX;
    ctx->serial_rx_probe_handle = UINT32_MAX;
    ctx->serial_tx_probe_handle = UINT32_MAX;

    if ((uint32_t)CROSSTALK_CORE_WATCH_WORKERS == 0) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: watch pool skipped; disk watching is off"); }
    else if (ldg_mut_init(&ctx->watch_mut, LDG_TRUTH_FALSE) == LDG_ERR_AOK) { if (ldg_thread_pool_init(&ctx->watch_pool, (uint32_t)CROSSTALK_CORE_WATCH_WORKERS) == LDG_ERR_AOK && ldg_thread_pool_start(&ctx->watch_pool, crosstalk_watch_worker, ctx) == LDG_ERR_AOK) { ctx->watch_ok = LDG_TRUTH_TRUE; }
        else { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "watch pool start failed; disk watching is off"); } }
    else { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "watch mutex init failed; disk watching is off"); }

    ret = crosstalk_res_dir_resolve(ctx->res_dir, (uint64_t)sizeof(ctx->res_dir));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(ctx);
        return ret;
    }

    ret = crosstalk_save_path_resolve(ctx->save_path, (uint64_t)sizeof(ctx->save_path));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: save path resolve failed; err: %u32", ret);
        ctx->save_path[0] = LDG_STR_TERM;
    }

    ret = crosstalk_data_subdir_resolve(CROSSTALK_SAVE_SUBDIR, ctx->save_dir, (uint64_t)sizeof(ctx->save_dir));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: save dir resolve failed; err: %u32", ret);
        ctx->save_dir[0] = LDG_STR_TERM;
    }

    ret = crosstalk_data_subdir_resolve(CROSSTALK_WSHOP_SUBDIR, ctx->wshop_dir, (uint64_t)sizeof(ctx->wshop_dir));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: wshop dir resolve failed; err: %u32", ret);
        ctx->wshop_dir[0] = LDG_STR_TERM;
    }

    ret = settings_preload(ctx);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: settings preload failed; err: %u32", ret); }

    {
        char ico_path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;

        ret = ldg_io_path_join(ctx->res_dir, CROSSTALK_CORE_NOTIFY_ICO_REL, ico_path, (uint64_t)sizeof(ico_path));
        if (ret == LDG_ERR_AOK)
        {
            ret = ldg_sys_notify_icon_set(ico_path);
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "crosstalk: notify icon set; err: %u32", ret);

            ret = ldg_sys_notify_tray_icon_set(ico_path);
            if (ret != LDG_ERR_AOK && ret != LDG_ERR_UNSUPPORTED) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: notify tray icon set; err: %u32", ret); }

            ret = ldg_sys_notify_tip_set(CROSSTALK_CORE_NOTIFY_TIP);
            if (ret != LDG_ERR_AOK && ret != LDG_ERR_UNSUPPORTED) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: notify tip set; err: %u32", ret); }
        }
    }

    ret = ldg_io_path_join(ctx->res_dir, "fonts/AnonymousPro-Regular.ttf", ctx->font_path, (uint64_t)sizeof(ctx->font_path));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(ctx);
        return ret;
    }

    ctx->board_preview_row = UINT32_MAX;
    ctx->board_preview_ok = LDG_TRUTH_FALSE;

    ret = ldg_audio_init();
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "crosstalk: audio init; err: %u32", ret);

    ret = crosstalk_game_music_init(&ctx->music);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "music init failed"); }

    settings_apply(ctx);

    {
        char hist_path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;

        ret = crosstalk_core_runtime_path_resolve(CROSSTALK_CORE_CMDLINE_HIST_FILE, hist_path, (uint64_t)sizeof(hist_path));
        if (ret == LDG_ERR_AOK) { ret = crosstalk_core_cmdline_hist_ld(&ctx->cmdline, hist_path); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: cmdline hist load failed; err: %u32", ret); }
    }

    ret = crosstalk_core_cmdline_drawer_init(&ctx->cmdline);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: cmdline drawer init failed; err: %u32", ret); }

    ret = crosstalk_core_scan_init(&ctx->scan);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: scan init failed; err: %u32", ret); }

    ret = crosstalk_game_chat_init(&ctx->chat);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "chat init failed"); }

    ret = crosstalk_game_hub_init(&ctx->hub);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "hub init failed"); }

    ret = crosstalk_eda_ic_fpga_init(&ctx->eda.fpga, 0x0, 0x0);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga init failed"); }
    else if (crosstalk_eda_ic_fpga_systask_cb_set(&ctx->eda.fpga, crosstalk_systask_out_cb, ctx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga systask cb set failed"); }

    ret = crosstalk_eda_ic_fpga_emu_init(&ctx->eda.fpga_emu);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga emu init failed"); }

    ret = crosstalk_eda_ic_fpga_run_init(&ctx->eda.fpga_run, &ctx->eda.fpga, &ctx->eda.clk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga run init failed"); }

    if (ret == LDG_ERR_AOK && ctx->cpuset_ok == LDG_TRUTH_TRUE) { if (crosstalk_eda_ic_fpga_run_cpuset_set(&ctx->eda.fpga_run, ctx->cpuset_id, ctx->cpuset_cunt) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga run cpuset set failed"); } }

    ret = crosstalk_eda_logic_probe_init(&ctx->eda.logic_probe);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "logic probe init failed"); }

    ret = crosstalk_eda_io_display_init(&ctx->eda.display);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "display init failed"); }

    ret = crosstalk_eda_ui_display_crt_init(&ctx->eda.crt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crt init failed"); }

    ret = crosstalk_eda_logic_stream_init(&ctx->eda.logic_stream);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "logic stream init failed"); }

    ret = crosstalk_eda_ic_fpga_run_stream_bind(&ctx->eda.fpga_run, &ctx->eda.logic_stream, &ctx->eda.logic_probe);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga run stream bind failed"); }

    ret = ldg_mem_pool_create(0x0, (uint64_t)CROSSTALK_CORE_PTY_POOL_MIB * (uint64_t)LDG_MIB, &ctx->eda.pty_pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty pool create failed"); }

    if (ctx->eda.pty_pool != 0x0)
    {
        ret = crosstalk_eda_io_pty_init(&ctx->eda.pty, ctx->eda.pty_pool);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty init failed"); }
    }

    ret = ldg_mem_pool_create(0x0, (uint64_t)CROSSTALK_CORE_TERM_POOL_MIB * (uint64_t)LDG_MIB, &ctx->eda.term_pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term pool create failed"); }

    ret = crosstalk_eda_ui_term_init(&ctx->eda.term);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term init failed"); }

    ret = crosstalk_eda_ui_indicator_init(&ctx->eda.indicator);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "indicator init failed"); }

    ret = crosstalk_eda_ui_calc_init(&ctx->eda.calc);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "calc init failed"); }

    ret = crosstalk_eda_ui_calc_bind(&ctx->eda.calc, &ctx->eda.fpga, &ctx->eda.fpga_run, CROSSTALK_CORE_TERM_SLOT);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "calc bind failed"); }

    ret = crosstalk_eda_ui_regview_init(&ctx->eda.regview);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "regview init failed"); }

    ret = crosstalk_eda_ui_regview_bind(&ctx->eda.regview, &ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "regview bind failed"); }

    ret = crosstalk_ui_guide_init(&ctx->guide);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "guide init failed"); }

    ret = crosstalk_ui_guide_bind(&ctx->guide, &ctx->win_nav, crosstalk_guide_tgt_open_cb, crosstalk_guide_done_cb, ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "guide bind failed"); }

    ret = crosstalk_guides_reg(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "guide table reg failed"); }

    ret = crosstalk_eda_ui_stackview_init(&ctx->eda.stackview);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "stackview init failed"); }

    ret = crosstalk_eda_ui_stackview_bind(&ctx->eda.stackview, &ctx->eda.fpga, &ctx->eda.logic_probe, CROSSTALK_CORE_TERM_SLOT);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "stackview bind failed"); }

    ret = crosstalk_eda_ui_brainomatic_init(&ctx->eda.brainomatic);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "brainomatic init failed"); }

    ret = crosstalk_eda_ui_progview_init(&ctx->eda.progview);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "progview init failed"); }

    ret = crosstalk_eda_ui_progview_bind(&ctx->eda.progview, &ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "progview bind failed"); }

    ret = crosstalk_eda_ui_pipeview_init(&ctx->eda.pipeview);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pipeview init failed"); }

    ret = crosstalk_eda_ui_pipeview_bind(&ctx->eda.pipeview, &ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pipeview bind failed"); }

    ret = crosstalk_eda_ui_analyzer_init(&ctx->eda.analyzer);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "analyzer init failed"); }

    ret = crosstalk_eda_ui_analyzer_bind(&ctx->eda.analyzer, &ctx->eda.logic_probe);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "analyzer bind failed"); }

    ret = crosstalk_eda_ui_feed_init(&ctx->eda.feed);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "feed init failed"); }

    ret = crosstalk_eda_ui_feed_bind(&ctx->eda.feed, &ctx->eda.logic_probe);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "feed bind failed"); }

    ret = crosstalk_eda_ui_scope_init(&ctx->eda.scope);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scope init failed"); }

    ret = crosstalk_eda_ui_profile_init(&ctx->eda.profile);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "profile init failed"); }

    ret = crosstalk_hdl_elf_init(&ctx->elf);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "elf init failed"); }

    ret = crosstalk_hdl_type_init(&ctx->prog_type);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_eda_ic_fpga_emu_stat_init(&ctx->emu_stat);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = crosstalk_hdl_elf_init(&ctx->prog_elf);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "prog elf init failed"); }

    ret = crosstalk_re_init(&ctx->re);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re: init failed; err: %u32", ret); }

    ret = crosstalk_reng_init(&ctx->reng);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "reng init failed"); }

    ret = crosstalk_reng_progress_init(&ctx->reng_progress, &ctx->reng);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "reng progress init failed"); }

    ret = crosstalk_reng_tasks_init(&ctx->reng_tasks, &ctx->reng);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "reng tasks init failed"); }

    ret = crosstalk_eda_ui_profile_elf_bind(&ctx->eda.profile, &ctx->elf);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "profile elf bind failed"); }

    ret = crosstalk_eda_ui_memview_init(&ctx->eda.memview);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview init failed"); }

    ret = crosstalk_eda_ui_memview_syms_bind(&ctx->eda.memview, &ctx->prog_elf, &ctx->elf, &ctx->prog_type);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview syms bind failed"); }

    ret = crosstalk_eda_ui_memview_calc_bind(&ctx->eda.memview, &ctx->eda.calc);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview calc bind failed"); }

    ret = crosstalk_eda_ui_wshop_browser_init(&ctx->eda.wshop_browser);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "wshop browser init failed"); }

    ret = crosstalk_eda_ui_wshop_publish_init(&ctx->eda.wshop_publish);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "wshop publish init failed"); }

    ret = crosstalk_eda_io_serial_init(&ctx->eda.serial);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "serial init failed"); }

    ret = crosstalk_eda_io_cast_init(&ctx->eda.cast);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cast init failed"); }

    ret = crosstalk_eda_sound_init(&ctx->eda.sound);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sound init failed"); }

    ret = crosstalk_eda_ic_fpga_probes_bind(&ctx->eda.fpga, &ctx->eda.logic_probe);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga probes bind failed"); }

    ret = crosstalk_eda_ic_fpga_display_bind(&ctx->eda.fpga, &ctx->eda.display);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga display bind failed"); }

    ret = crosstalk_eda_io_receptacle_init(&ctx->eda.receptacle);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "receptacle init failed"); }

    ret = crosstalk_eda_ic_fpga_receptacle_bind(&ctx->eda.fpga, &ctx->eda.receptacle);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga receptacle bind failed"); }

    ret = crosstalk_eda_io_midi_init(&ctx->eda.midi);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "midi init failed"); }

    ret = crosstalk_eda_ic_fpga_midi_bind(&ctx->eda.fpga, &ctx->eda.midi);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga midi bind failed"); }

    ret = crosstalk_eda_io_midi_ports_refresh(&ctx->eda.midi);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "midi port list failed"); }

    ret = ldg_mem_pool_create(0x0, (uint64_t)CROSSTALK_CORE_SCRIPT_POOL_MIB * (uint64_t)LDG_MIB, &ctx->eda.script_pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "script pool create failed"); }

    ret = crosstalk_eda_script_init(&ctx->eda.script, ctx->eda.script_pool, ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "script init failed"); }

    ret = crosstalk_script_builtins_reg(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "script builtins reg failed"); }

    ret = crosstalk_eda_clk_init(&ctx->eda.clk, (uint64_t)CROSSTALK_EDA_CLK_STEP_HZ_DEFAULT);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "clk init failed"); }

    ret = crosstalk_ident_cache_ld(ctx);
    if (ret != LDG_ERR_AOK && ret != LDG_ERR_EMPTY) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: ident cache load failed; err: %u32", ret); }

    ret = crosstalk_game_totals_init(&ctx->game.totals);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "totals init failed"); }

    ret = crosstalk_game_settings_menu_init(&ctx->settings_menu);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "settings menu init failed"); }

    ret = crosstalk_game_menu_nav_rst(&ctx->menu_nav);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "menu nav rst failed"); }

    ctx->menu_screen_prev = (uint32_t)UINT32_MAX;

    ret = crosstalk_win_nav_reg(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "window nav reg failed"); }

    ret = ldg_sched_conf_default_get(&conf);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(ctx);
        return ret;
    }

    ret = ldg_sched_ctx_init(&ctx->sched, &conf);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "crosstalk: create sched_init; err: %u32", ret);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(ctx);
        return ret;
    }

    ret = ldg_net_steamworks_init();
    ctx->steam_live = (uint8_t)(ret == LDG_ERR_AOK ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_DEBUG : LDG_LOGGING_LVL_WARN, "crosstalk: create net_init; err: %u32; steam: %u32", ret, (uint32_t)ctx->steam_live);

    ctx->app_id = ldg_net_steamworks_app_id_get();
    if (ctx->app_id == UINT32_MAX || ctx->app_id == 0) { ctx->app_id = crosstalk_core_launch_app_id_get(); }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: launched as app_id: %u32", ctx->app_id);

    ret = crosstalk_net_wshop_init(&ctx->wshop);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk wshop init failed"); }

    if (ctx->steam_live == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_net_steam_init(&ctx->steam_net, &ctx->net, &ctx->net_cb);
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_DEBUG : LDG_LOGGING_LVL_WARN, "crosstalk: create steam_net_init; err: %u32", ret);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            ldg_net_steamworks_shutdown();
            ctx->steam_live = LDG_TRUTH_FALSE;
        }
    }

    ret = ctx->steam_live == LDG_TRUTH_TRUE ? crosstalk_net_inventory_init(&ctx->inventory) : LDG_ERR_NOT_INIT;
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_DEBUG : LDG_LOGGING_LVL_WARN, "crosstalk: create inventory_init; err: %u32", ret);

    if (ret == LDG_ERR_AOK)
    {
        ret = crosstalk_net_inventory_refresh(&ctx->inventory);
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "crosstalk: inventory refresh; err: %u32; granted: %u32", ret, (uint32_t)ctx->inventory.granted);
    }

    ctx->pipe_handle = UINT32_MAX;

    ret = crosstalk_net_relay_init(&ctx->relay);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_DEBUG : LDG_LOGGING_LVL_WARN, "crosstalk: create relay_init; err: %u32", ret);

    if (ret == LDG_ERR_AOK)
    {
        ret = crosstalk_eda_ic_fpga_step_cb_set(&ctx->eda.fpga, crosstalk_relay_step_cb, ctx);
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "crosstalk: relay step hook; err: %u32", ret);
    }

    ret = crosstalk_net_discord_init(&ctx->discord, CROSSTALK_NET_DISCORD_APP_ID, ctx->app_id);
    ctx->discord_live = (uint8_t)(ret == LDG_ERR_AOK ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_DEBUG : LDG_LOGGING_LVL_WARN, "crosstalk: create discord_init; err: %u32", ret);

    if (ctx->discord_live == LDG_TRUTH_TRUE) { crosstalk_discord_settings_apply(ctx); }

    ctx->net_cb.on_data = sesh_data_cb;
    ctx->net_cb.on_dc = sesh_dc_cb;
    ctx->net_cb.user = ctx;

    ret = crosstalk_ui_window_all_close(&ctx->win_nav);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "startup window close failed"); }

    if (crosstalk_game_fx_sfx_init(&ctx->game.sfx_sound) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sfx init failed; ui audio disabled"); }

    if (crosstalk_game_caption_init(&ctx->game.caption) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "caption init failed; captions disabled"); }
    else if (crosstalk_game_fx_sfx_play_cb_set(&ctx->game.sfx_sound, crosstalk_game_caption_sfx_cb, &ctx->game.caption) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sfx caption cb set failed"); }

    ret = ldg_thread_pool_init(&ctx->game.ld_pool, CROSSTALK_LD_POOL_WORKERS);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (ctx->steam_live == LDG_TRUTH_TRUE)
        {
            crosstalk_net_steam_shutdown(&ctx->steam_net);
            ldg_net_steamworks_shutdown();
        }

        ldg_sched_ctx_shutdown(&ctx->sched);
        ldg_mem_dealloc(ctx);
        return ret;
    }

    ret = ldg_thread_pool_init(&ctx->game.music_pool, CROSSTALK_CORE_MUSIC_POOL_WORKERS);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_thread_pool_shutdown(&ctx->game.ld_pool);

        if (ctx->steam_live == LDG_TRUTH_TRUE)
        {
            crosstalk_net_steam_shutdown(&ctx->steam_net);
            ldg_net_steamworks_shutdown();
        }

        ldg_sched_ctx_shutdown(&ctx->sched);
        ldg_mem_dealloc(ctx);
        return ret;
    }

    ret = ld_begin(ctx, CROSSTALK_LD_STARTUP, CROSSTALK_GAME_SCREEN_MENU);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->game.screen = CROSSTALK_GAME_SCREEN_MENU; }

    ctx->game.quit = LDG_TRUTH_FALSE;

    ctx->soundtrack_owned = (ctx->steam_live == LDG_TRUTH_TRUE && ldg_net_steamworks_dlc_installed_is(CROSSTALK_CORE_SOUNDTRACK_APP_ID) == LDG_TRUTH_TRUE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: soundtrack ownership; app: %u32; owns: %u32", (uint32_t)CROSSTALK_CORE_SOUNDTRACK_APP_ID, (uint32_t)ctx->soundtrack_owned);

    ret = crosstalk_game_progression_queue_init(&ctx->progression_queue);
    if (ret == LDG_ERR_AOK) { ctx->progression_queue_ok = LDG_TRUTH_TRUE; }
    else { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "crosstalk: progression queue init failed; err: %u32", ret); }

    /* { uint64_t hits = 0; if (crosstalk_install_scan(ctx, &hits) == LDG_ERR_AOK && hits != 0) { ctx->game.quit = LDG_TRUTH_TRUE; } } */

    {
        ldg_term_sesh_conf_t sesh_conf = LDG_STRUCT_ZERO_INIT;

        ret = ldg_term_sesh_conf_default_get(&sesh_conf);

        sesh_conf.render.sample_col_cunt = (uint32_t)CROSSTALK_CORE_SESH_SAMPLE_COL;
        sesh_conf.render.sample_row_cunt = (uint32_t)CROSSTALK_CORE_SESH_SAMPLE_ROW;
        sesh_conf.col_max = (uint32_t)CROSSTALK_CORE_SESH_COL_MAX;
        sesh_conf.row_max = (uint32_t)CROSSTALK_CORE_SESH_ROW_MAX;

        if (ret == LDG_ERR_AOK) { ret = ldg_term_sesh_create(&sesh_conf, &ctx->sesh); }

        if (ret == LDG_ERR_AOK)
        {
            ret = crosstalk_core_shot_create(&ctx->shot, (uint32_t)CROSSTALK_CORE_SESH_COL_MAX, (uint32_t)CROSSTALK_CORE_SESH_ROW_MAX);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "shot: create failed; err: %u32", ret); }

            ret = crosstalk_core_watch_create(&ctx->demo_watch, (uint64_t)CROSSTALK_CORE_DEMO_WATCH_CAP);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "demo watch: create failed; err: %u32", ret); }

            ret = LDG_ERR_AOK;
        }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            ctx->sesh = 0x0;
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "crosstalk: term sesh init failed; err: %u32", ret);
        }
        else
        {
            ret = ldg_term_sesh_title_set(ctx->sesh, CROSSTALK_CORE_SESH_TITLE, (uint64_t)(sizeof(CROSSTALK_CORE_SESH_TITLE) - LDG_STR_TERM_SIZE));
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term sesh title set failed"); }
        }
    }

    {
        ldg_term_input_conf_t irq_conf = LDG_STRUCT_ZERO_INIT;

        ret = ldg_term_input_conf_default_get(&irq_conf);
        if (ret == LDG_ERR_AOK) { ret = ldg_term_input_create(&irq_conf, &ctx->irq_src); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            ctx->irq_src = 0x0;
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "crosstalk: key irq source init failed; err: %u32", ret);
        }
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: create done; screen: %u32", (uint32_t)ctx->game.screen);

    *out = ctx;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_music_worker(uint32_t worker_id, void *arg)
{
    crosstalk_ctx_t *ctx = (crosstalk_ctx_t *)arg;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_game_music_poll_run(&ctx->music);
    LDG_ST_RELEASE(ctx->music.done, CROSSTALK_GAME_MUSIC_DONE_SET);

    return ret;
}

static void crosstalk_board_preview_drop(crosstalk_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    ctx->board_preview_ok = LDG_TRUTH_FALSE;
    ctx->board_preview_row = UINT32_MAX;
}

static void crosstalk_board_preview_time_fmt(uint64_t epoch_ms, char *out, uint64_t out_size)
{
    const struct tm *broken = 0x0;
    time_t secs = 0;

    if (LDG_UNLIKELY(out == 0x0 || out_size == 0)) { return; }

    out[0] = LDG_STR_TERM;
    if (epoch_ms == 0) { return; }

    secs = (time_t)(epoch_ms / LDG_MS_PER_SEC);
    broken = localtime(&secs);
    if (LDG_UNLIKELY(broken == 0x0)) { return; }

    if (strftime(out, out_size, CROSSTALK_BOARD_PREVIEW_TIME_FMT, broken) == 0) { out[0] = LDG_STR_TERM; }
}

static void crosstalk_board_preview_refresh(crosstalk_ctx_t *ctx, uint32_t row)
{
    const crosstalk_game_board_t *entry = 0x0;
    char path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char author[CROSSTALK_GAME_AUTHOR_NAME_CAP] = LDG_ARR_ZERO_INIT;
    char stamp[CROSSTALK_GAME_BOARD_TIME_CAP] = LDG_ARR_ZERO_INIT;
    char size_text[CROSSTALK_GAME_BOARD_SIZE_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t created_ms = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (ctx->board_preview_row == row && ctx->board_preview_ok == LDG_TRUTH_TRUE) { return; }

    crosstalk_board_preview_drop(ctx);

    if (row >= ctx->board_sel.cunt) { return; }

    entry = &ctx->board_sel.entry[row];
    ctx->board_preview_row = row;

    ret = crosstalk_game_board_path_build(&ctx->board_sel, row, path, (uint64_t)sizeof(path));
    if (ret != LDG_ERR_AOK) { return; }

    ret = crosstalk_game_save_meta_rd(path, author, (uint32_t)sizeof(author), &created_ms, 0x0, CROSSTALK_BOARD_PREVIEW_EDITORS_UNREAD);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "crosstalk: board preview meta failed; err: %u32; row: %u32", ret, row); }

    crosstalk_board_preview_time_fmt(created_ms != 0 ? created_ms : entry->created_epoch_ms, stamp, (uint64_t)sizeof(stamp));

    snprintf(ctx->board_preview_line[0], sizeof(ctx->board_preview_line[0]), "%.63s", entry->name);
    snprintf(ctx->board_preview_line[1], sizeof(ctx->board_preview_line[1]), "by %.63s", author[0] != LDG_STR_TERM ? author : entry->author);
    snprintf(ctx->board_preview_line[2], sizeof(ctx->board_preview_line[2]), "founded: %.31s", stamp);
    if (crosstalk_game_board_size_fmt(entry->size_bytes, size_text, (uint64_t)sizeof(size_text)) != LDG_ERR_AOK) { size_text[0] = LDG_STR_TERM; }

    snprintf(ctx->board_preview_line[3], sizeof(ctx->board_preview_line[3]), "size: %.15s; src: %.15s", size_text, entry->src == CROSSTALK_GAME_BOARD_SRC_WSHOP ? CROSSTALK_BOARD_PREVIEW_SRC_WSHOP : CROSSTALK_BOARD_PREVIEW_SRC_LOCAL);

    snprintf(ctx->board_preview_line[4], sizeof(ctx->board_preview_line[4]), "contains: %.47s", entry->editors[0] != LDG_STR_TERM ? entry->editors : "-");

    ctx->board_preview_ok = LDG_TRUTH_TRUE;
}

uint32_t crosstalk_core_save_wr(crosstalk_ctx_t *ctx)
{
    char save_file[CROSSTALK_GAME_BOARD_FILE_CAP] = LDG_ARR_ZERO_INIT;
    char save_full[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t epoch_sec = 0;
    uint32_t ret = 0;
    uint32_t ach = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->remote_sesh != 0)
    {
        crosstalk_net_steam_notice_push(&ctx->steam_net, "guest board cannot be saved");
        return LDG_ERR_DENIED;
    }

    ctx->game.last_state.layout_rev = CROSSTALK_GAME_LAST_STATE_LAYOUT_REV;

    if (ctx->game.author_name[0] == LDG_STR_TERM) { author_capture(ctx); }

    if (ctx->game.created_epoch_ms == 0) { ctx->game.created_epoch_ms = ldg_time_epoch_ms_get(); }

    {
        crosstalk_game_save_editor_sect_t editor_sect[CROSSTALK_SAVE_EDITOR_SECT_CAP] = LDG_ARR_ZERO_INIT;
        uint8_t *layout_buff = 0x0;
        uint8_t *rtl_buff = 0x0;
        uint8_t *script_buff = 0x0;
        uint8_t *calc_buff = 0x0;
        uint8_t *term_buff = 0x0;
        uint8_t *crt_buff = 0x0;
        uint8_t *emu_buff = 0x0;
        uint32_t layout_len = 0;
        uint32_t rtl_len = 0;
        uint32_t script_len = 0;
        uint32_t calc_len = 0;
        uint32_t term_len = 0;
        uint32_t crt_len = 0;
        uint32_t emu_len = 0;
        uint32_t editor_sect_cunt = 0;

        if (crosstalk_layout_sect_build(ctx, &layout_buff, &layout_len) == LDG_ERR_AOK && layout_buff != 0x0)
        {
            editor_sect[editor_sect_cunt].name = CROSSTALK_GAME_SAVE_SECT_LAYOUT;
            editor_sect[editor_sect_cunt].blob = layout_buff;
            editor_sect[editor_sect_cunt].blob_len = layout_len;
            editor_sect_cunt = editor_sect_cunt + 0x1;
        }

        if (crosstalk_rtl_sect_build(ctx, &rtl_buff, &rtl_len) == LDG_ERR_AOK && rtl_buff != 0x0)
        {
            editor_sect[editor_sect_cunt].name = CROSSTALK_GAME_SAVE_SECT_RTL;
            editor_sect[editor_sect_cunt].blob = rtl_buff;
            editor_sect[editor_sect_cunt].blob_len = rtl_len;
            editor_sect_cunt = editor_sect_cunt + 0x1;
        }

        if (crosstalk_script_sect_build(ctx, &script_buff, &script_len) == LDG_ERR_AOK && script_buff != 0x0)
        {
            editor_sect[editor_sect_cunt].name = CROSSTALK_GAME_SAVE_SECT_SCRIPT;
            editor_sect[editor_sect_cunt].blob = script_buff;
            editor_sect[editor_sect_cunt].blob_len = script_len;
            editor_sect_cunt = editor_sect_cunt + 0x1;
        }

        if (crosstalk_calc_sect_build(ctx, &calc_buff, &calc_len) == LDG_ERR_AOK && calc_buff != 0x0)
        {
            editor_sect[editor_sect_cunt].name = CROSSTALK_GAME_SAVE_SECT_CALC;
            editor_sect[editor_sect_cunt].blob = calc_buff;
            editor_sect[editor_sect_cunt].blob_len = calc_len;
            editor_sect_cunt = editor_sect_cunt + 0x1;
        }

        if (crosstalk_reng_sect_build(ctx) == LDG_ERR_AOK && ctx->reng_sect_buff != 0x0)
        {
            editor_sect[editor_sect_cunt].name = CROSSTALK_GAME_SAVE_SECT_RENG;
            editor_sect[editor_sect_cunt].blob = ctx->reng_sect_buff;
            editor_sect[editor_sect_cunt].blob_len = ctx->reng_sect_len;
            editor_sect_cunt = editor_sect_cunt + 0x1;
        }

        if (crosstalk_term_sect_build(ctx, &term_buff, &term_len) == LDG_ERR_AOK && term_buff != 0x0)
        {
            editor_sect[editor_sect_cunt].name = CROSSTALK_GAME_SAVE_SECT_TERM;
            editor_sect[editor_sect_cunt].blob = term_buff;
            editor_sect[editor_sect_cunt].blob_len = term_len;
            editor_sect_cunt = editor_sect_cunt + 0x1;
        }

        if (crosstalk_crt_sect_build(ctx, &crt_buff, &crt_len) == LDG_ERR_AOK && crt_buff != 0x0)
        {
            editor_sect[editor_sect_cunt].name = CROSSTALK_GAME_SAVE_SECT_CRT;
            editor_sect[editor_sect_cunt].blob = crt_buff;
            editor_sect[editor_sect_cunt].blob_len = crt_len;
            editor_sect_cunt = editor_sect_cunt + 0x1;
        }

        if (crosstalk_emu_sect_build(ctx, &emu_buff, &emu_len) == LDG_ERR_AOK && emu_buff != 0x0)
        {
            editor_sect[editor_sect_cunt].name = CROSSTALK_GAME_SAVE_SECT_EMU;
            editor_sect[editor_sect_cunt].blob = emu_buff;
            editor_sect[editor_sect_cunt].blob_len = emu_len;
            editor_sect_cunt = editor_sect_cunt + 0x1;
        }

        if (ctx->save_dir[0] != LDG_STR_TERM)
        {
            epoch_sec = ldg_time_epoch_ms_get() / LDG_MS_PER_SEC;

            ret = crosstalk_game_board_file_compose(ctx->board_name, epoch_sec, save_file, (uint64_t)sizeof(save_file));
            if (ret == LDG_ERR_AOK) { ret = ldg_io_path_join(ctx->save_dir, save_file, save_full, (uint64_t)sizeof(save_full)); }

            if (ret == LDG_ERR_AOK) { ret = ldg_str_rbrcpy(ctx->save_path, save_full, (uint64_t)sizeof(ctx->save_path)); }

            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "save path compose failed"); }
        }

        ret = crosstalk_game_save_wr(&ctx->game.settings, &ctx->game.last_state, ctx->game.author_steam_id, ctx->game.author_name, ctx->game.created_epoch_ms, ctx->game.bench_ms_total, ctx->game.sim_ms_total, editor_sect, editor_sect_cunt, ctx->save_path);
        if (layout_buff != 0x0) { ldg_mem_dealloc(layout_buff); }

        if (rtl_buff != 0x0) { ldg_mem_dealloc(rtl_buff); }

        if (script_buff != 0x0) { ldg_mem_dealloc(script_buff); }

        if (calc_buff != 0x0) { ldg_mem_dealloc(calc_buff); }

        if (term_buff != 0x0) { ldg_mem_dealloc(term_buff); }

        if (crt_buff != 0x0) { ldg_mem_dealloc(crt_buff); }

        if (emu_buff != 0x0) { ldg_mem_dealloc(emu_buff); }
    }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        char alert_msg[CROSSTALK_CORE_CMDLINE_DRAWER_LINE_CAP] = LDG_ARR_ZERO_INIT;
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "save failed");
        snprintf(alert_msg, sizeof(alert_msg), "The board was NOT saved; err: %u. A previous save, if one existed, is preserved as a .bkp file next to it.", ret);
        toast_push(ctx, alert_msg);
    }
    else
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "save ok");
        ctx->st_time_ms = ldg_time_epoch_ms_get();
        toast_push(ctx, "Saved");
        ach = crosstalk_game_progression_achievement_unlock_req(CROSSTALK_GAME_PROGRESSION_FIRST_SAVE, CROSSTALK_APP_ID, &ctx->progression_queue);
        if (ach != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "crosstalk: ach first_save; err: %u32", ach); }
    }

    return ret;
}

static uint32_t crosstalk_autosave_wr(crosstalk_ctx_t *ctx)
{
    char autosave_path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t path_len = 0;
    uint64_t ext_len = sizeof(CROSSTALK_GAME_BOARD_EXT) - LDG_STR_TERM_SIZE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->remote_sesh != 0) { return LDG_ERR_DENIED; }

    if (ctx->game.screen != CROSSTALK_GAME_SCREEN_EDITOR || ctx->save_path[0] == LDG_STR_TERM) { return LDG_ERR_NOT_INIT; }

    path_len = strlen(ctx->save_path);
    if (path_len > ext_len && strcmp(ctx->save_path + path_len - ext_len, CROSSTALK_GAME_BOARD_EXT) == 0) { path_len = path_len - ext_len; }

    if (path_len + sizeof(CROSSTALK_AUTOSAVE_SUFFIX) - LDG_STR_TERM_SIZE + sizeof(CROSSTALK_GAME_BOARD_EXT) > sizeof(autosave_path)) { return LDG_ERR_STR_TRUNC; }

    memcpy(autosave_path, ctx->save_path, path_len);
    memcpy(autosave_path + path_len, CROSSTALK_AUTOSAVE_SUFFIX, sizeof(CROSSTALK_AUTOSAVE_SUFFIX) - LDG_STR_TERM_SIZE);
    memcpy(autosave_path + path_len + sizeof(CROSSTALK_AUTOSAVE_SUFFIX) - LDG_STR_TERM_SIZE, CROSSTALK_GAME_BOARD_EXT, sizeof(CROSSTALK_GAME_BOARD_EXT));

    {
        crosstalk_game_save_editor_sect_t editor_sect[CROSSTALK_SAVE_EDITOR_SECT_CAP] = LDG_ARR_ZERO_INIT;
        uint8_t *layout_buff = 0x0;
        uint8_t *rtl_buff = 0x0;
        uint8_t *script_buff = 0x0;
        uint8_t *calc_buff = 0x0;
        uint8_t *term_buff = 0x0;
        uint8_t *crt_buff = 0x0;
        uint8_t *emu_buff = 0x0;
        uint32_t layout_len = 0;
        uint32_t rtl_len = 0;
        uint32_t script_len = 0;
        uint32_t calc_len = 0;
        uint32_t term_len = 0;
        uint32_t crt_len = 0;
        uint32_t emu_len = 0;
        uint32_t editor_sect_cunt = 0;

        if (crosstalk_layout_sect_build(ctx, &layout_buff, &layout_len) == LDG_ERR_AOK && layout_buff != 0x0)
        {
            editor_sect[editor_sect_cunt].name = CROSSTALK_GAME_SAVE_SECT_LAYOUT;
            editor_sect[editor_sect_cunt].blob = layout_buff;
            editor_sect[editor_sect_cunt].blob_len = layout_len;
            editor_sect_cunt = editor_sect_cunt + 0x1;
        }

        if (crosstalk_rtl_sect_build(ctx, &rtl_buff, &rtl_len) == LDG_ERR_AOK && rtl_buff != 0x0)
        {
            editor_sect[editor_sect_cunt].name = CROSSTALK_GAME_SAVE_SECT_RTL;
            editor_sect[editor_sect_cunt].blob = rtl_buff;
            editor_sect[editor_sect_cunt].blob_len = rtl_len;
            editor_sect_cunt = editor_sect_cunt + 0x1;
        }

        if (crosstalk_script_sect_build(ctx, &script_buff, &script_len) == LDG_ERR_AOK && script_buff != 0x0)
        {
            editor_sect[editor_sect_cunt].name = CROSSTALK_GAME_SAVE_SECT_SCRIPT;
            editor_sect[editor_sect_cunt].blob = script_buff;
            editor_sect[editor_sect_cunt].blob_len = script_len;
            editor_sect_cunt = editor_sect_cunt + 0x1;
        }

        if (crosstalk_calc_sect_build(ctx, &calc_buff, &calc_len) == LDG_ERR_AOK && calc_buff != 0x0)
        {
            editor_sect[editor_sect_cunt].name = CROSSTALK_GAME_SAVE_SECT_CALC;
            editor_sect[editor_sect_cunt].blob = calc_buff;
            editor_sect[editor_sect_cunt].blob_len = calc_len;
            editor_sect_cunt = editor_sect_cunt + 0x1;
        }

        if (crosstalk_reng_sect_build(ctx) == LDG_ERR_AOK && ctx->reng_sect_buff != 0x0)
        {
            editor_sect[editor_sect_cunt].name = CROSSTALK_GAME_SAVE_SECT_RENG;
            editor_sect[editor_sect_cunt].blob = ctx->reng_sect_buff;
            editor_sect[editor_sect_cunt].blob_len = ctx->reng_sect_len;
            editor_sect_cunt = editor_sect_cunt + 0x1;
        }

        if (crosstalk_term_sect_build(ctx, &term_buff, &term_len) == LDG_ERR_AOK && term_buff != 0x0)
        {
            editor_sect[editor_sect_cunt].name = CROSSTALK_GAME_SAVE_SECT_TERM;
            editor_sect[editor_sect_cunt].blob = term_buff;
            editor_sect[editor_sect_cunt].blob_len = term_len;
            editor_sect_cunt = editor_sect_cunt + 0x1;
        }

        if (crosstalk_crt_sect_build(ctx, &crt_buff, &crt_len) == LDG_ERR_AOK && crt_buff != 0x0)
        {
            editor_sect[editor_sect_cunt].name = CROSSTALK_GAME_SAVE_SECT_CRT;
            editor_sect[editor_sect_cunt].blob = crt_buff;
            editor_sect[editor_sect_cunt].blob_len = crt_len;
            editor_sect_cunt = editor_sect_cunt + 0x1;
        }

        if (crosstalk_emu_sect_build(ctx, &emu_buff, &emu_len) == LDG_ERR_AOK && emu_buff != 0x0)
        {
            editor_sect[editor_sect_cunt].name = CROSSTALK_GAME_SAVE_SECT_EMU;
            editor_sect[editor_sect_cunt].blob = emu_buff;
            editor_sect[editor_sect_cunt].blob_len = emu_len;
            editor_sect_cunt = editor_sect_cunt + 0x1;
        }

        ret = crosstalk_game_save_wr(&ctx->game.settings, &ctx->game.last_state, ctx->game.author_steam_id, ctx->game.author_name, ctx->game.created_epoch_ms, ctx->game.bench_ms_total, ctx->game.sim_ms_total, editor_sect, editor_sect_cunt, autosave_path);
        if (layout_buff != 0x0) { ldg_mem_dealloc(layout_buff); }

        if (rtl_buff != 0x0) { ldg_mem_dealloc(rtl_buff); }

        if (script_buff != 0x0) { ldg_mem_dealloc(script_buff); }

        if (calc_buff != 0x0) { ldg_mem_dealloc(calc_buff); }

        if (term_buff != 0x0) { ldg_mem_dealloc(term_buff); }

        if (crt_buff != 0x0) { ldg_mem_dealloc(crt_buff); }

        if (emu_buff != 0x0) { ldg_mem_dealloc(emu_buff); }
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "crosstalk: autosave; err: %u32; path: %.259s", ret, autosave_path);

    return ret;
}

static uint32_t crosstalk_rgb_rgba_get(double col_r, double col_g, double col_b)
{
    return (crosstalk_card_channel_get(col_r) << CROSSTALK_UI_CELL_RGBA_RED_SHIFT) | (crosstalk_card_channel_get(col_g) << CROSSTALK_UI_CELL_RGBA_GREEN_SHIFT) | (crosstalk_card_channel_get(col_b) << CROSSTALK_UI_CELL_RGBA_BLUE_SHIFT) | (uint32_t)CROSSTALK_CORE_CARD_ALPHA_OPAQUE;
}

static uint32_t crosstalk_row_fill(ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t width, uint32_t bg_rgba)
{
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t bg = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, &fg);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get(bg_rgba, &bg); }

    if (ret != LDG_ERR_AOK) { return ret; }

    return crosstalk_ui_cell_span_fill(render, col, row, width, &fg, &bg);
}

static uint32_t crosstalk_text_clip_put(ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t width, const char *text, uint32_t fg_rgba, uint32_t bg_rgba, uint32_t *out_w)
{
    ldg_term_render_color_t fg = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t bg = LDG_STRUCT_ZERO_INIT;
    uint64_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || text == 0x0 || out_w == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_w = 0;

    ret = ldg_str_len_get(text, (uint64_t)CROSSTALK_CORE_TEXT_CLIP_CAP, &len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (len > (uint64_t)width) { len = (uint64_t)width; }

    if (len == 0) { return LDG_ERR_AOK; }

    ret = crosstalk_ui_cell_color_get(fg_rgba, &fg);
    if (ret == LDG_ERR_AOK) { ret = crosstalk_ui_cell_color_get(bg_rgba, &bg); }

    if (ret == LDG_ERR_AOK) { ret = ldg_term_render_text_draw(render, col, row, text, len, &fg, &bg); }

    if (ret == LDG_ERR_AOK) { *out_w = (uint32_t)len; }

    return ret;
}

static uint32_t crosstalk_lobby_wave_draw(crosstalk_ctx_t *ctx, ldg_term_render_t *render)
{
    double field_col_r[CROSSTALK_LOBBY_WAVE_FIELD_CUNT] = { CROSSTALK_LOBBY_WAVE_E_COL_R, CROSSTALK_LOBBY_WAVE_B_COL_R };
    double field_col_g[CROSSTALK_LOBBY_WAVE_FIELD_CUNT] = { CROSSTALK_LOBBY_WAVE_E_COL_G, CROSSTALK_LOBBY_WAVE_B_COL_G };
    double field_col_b[CROSSTALK_LOBBY_WAVE_FIELD_CUNT] = { CROSSTALK_LOBBY_WAVE_E_COL_B, CROSSTALK_LOBBY_WAVE_B_COL_B };
    double win_w = 0.0;
    double win_h = 0.0;
    double wave_w = 0.0;
    double wave_x0 = 0.0;
    double wave_cy = 0.0;
    double wave_amp = 0.0;
    double wave_phase = 0.0;
    double seg_w = 0.0;
    double seg_phase = 0.0;
    double prev_x = 0.0;
    double prev_y = 0.0;
    uint32_t rgba = 0;
    uint32_t field_idx = 0;
    uint32_t seg_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    win_w = (double)(render->col_cunt * render->sample_col_cunt);
    win_h = (double)(render->row_cunt * render->sample_row_cunt);
    wave_w = win_w * CROSSTALK_LOBBY_WAVE_WIDTH_FRAC;
    wave_x0 = (win_w - wave_w) * CROSSTALK_CORE_CARD_HALF;
    wave_cy = win_h * CROSSTALK_CORE_CARD_HALF;
    wave_amp = win_h * CROSSTALK_LOBBY_WAVE_AMP_FRAC;
    wave_phase = ctx->time * CROSSTALK_LOBBY_WAVE_SPEED;
    seg_w = wave_w / (double)CROSSTALK_LOBBY_WAVE_SEG_CUNT;
    seg_phase = CROSSTALK_LOBBY_WAVE_CYCLES * CROSSTALK_CORE_CARD_TURN * LDG_MATH_PI / (double)CROSSTALK_LOBBY_WAVE_SEG_CUNT;

    for (field_idx = 0; field_idx < CROSSTALK_LOBBY_WAVE_FIELD_CUNT; field_idx = field_idx + 1)
    {
        double amp_now = wave_amp * (field_idx == 0 ? cos(wave_phase) : sin(wave_phase));

        rgba = crosstalk_rgb_rgba_get(field_col_r[field_idx], field_col_g[field_idx], field_col_b[field_idx]);
        prev_x = wave_x0;
        prev_y = wave_cy;

        for (seg_idx = 1; seg_idx <= CROSSTALK_LOBBY_WAVE_SEG_CUNT; seg_idx = seg_idx + 1)
        {
            double cur_x = wave_x0 + (double)seg_idx * seg_w;
            double cur_y = wave_cy + amp_now * sin((double)seg_idx * seg_phase);

            if (prev_x >= 0.0 && cur_x < win_w && prev_y >= 0.0 && cur_y >= 0.0 && prev_y < win_h && cur_y < win_h)
            {
                ret = crosstalk_ui_cell_line_draw(render, crosstalk_card_sample_get(prev_x), crosstalk_card_sample_get(prev_y), crosstalk_card_sample_get(cur_x), crosstalk_card_sample_get(cur_y), rgba);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            prev_x = cur_x;
            prev_y = cur_y;
        }
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_menu_switch_draw(ldg_term_render_t *render, const ldg_term_rect_t *rect, const char *caption, const char **item, const uint8_t *item_hide, uint32_t item_cunt, uint32_t sel, uint8_t panel_on)
{
    uint32_t need = item_cunt + (uint32_t)CROSSTALK_MENU_SWITCH_HD_ROWS;
    uint32_t top = 0;
    uint32_t left = 0;
    uint32_t drawn = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || rect == 0x0 || caption == 0x0 || item == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    top = rect->row;
    left = rect->col;

    if (rect->row_cunt > need) { top = rect->row + (rect->row_cunt - need) / (uint32_t)CROSSTALK_MENU_SWITCH_CENTER_DIV; }

    if (rect->col_cunt > (uint32_t)CROSSTALK_MENU_SWITCH_COL_CUNT) { left = rect->col + (rect->col_cunt - (uint32_t)CROSSTALK_MENU_SWITCH_COL_CUNT) / (uint32_t)CROSSTALK_MENU_SWITCH_CENTER_DIV; }

    if (panel_on == LDG_TRUTH_TRUE)
    {
        uint32_t panel_col = left > rect->col ? left - (uint32_t)CROSSTALK_MENU_SWITCH_PAD : left;
        uint32_t panel_row = top > rect->row ? top - (uint32_t)CROSSTALK_MENU_SWITCH_PAD : top;
        uint32_t panel_w = (left - panel_col) + (uint32_t)CROSSTALK_MENU_SWITCH_COL_CUNT + (uint32_t)CROSSTALK_MENU_SWITCH_PAD;
        uint32_t panel_end = top + need + (uint32_t)CROSSTALK_MENU_SWITCH_PAD;

        if (panel_end > rect->row + rect->row_cunt) { panel_end = rect->row + rect->row_cunt; }

        if (panel_col + panel_w > rect->col + rect->col_cunt) { panel_w = rect->col + rect->col_cunt - panel_col; }

        for (idx = panel_row; idx < panel_end; idx = idx + 0x1)
        {
            ret = crosstalk_row_fill(render, panel_col, idx, panel_w, (uint32_t)CROSSTALK_UI_CELL_PANE_RGBA);
            if (ret != LDG_ERR_AOK) { return ret; }
        }
    }

    ret = crosstalk_text_clip_put(render, left, top, (uint32_t)CROSSTALK_MENU_SWITCH_COL_CUNT, caption, (uint32_t)CROSSTALK_UI_THEME_LIT_RGBA, (uint32_t)CROSSTALK_UI_CELL_PANE_RGBA, &drawn);
    if (ret != LDG_ERR_AOK) { return ret; }

    for (idx = 0; idx < item_cunt; idx = idx + 0x1)
    {
        const char *label = (item_hide != 0x0 && item_hide[idx] == LDG_TRUTH_TRUE) ? CROSSTALK_MENU_SOON_TEXT : item[idx];
        uint32_t row = top + (uint32_t)CROSSTALK_MENU_SWITCH_HD_ROWS + idx;
        uint32_t bg_rgba = idx == sel ? (uint32_t)CROSSTALK_UI_THEME_SEL_BG_RGBA : (uint32_t)CROSSTALK_UI_CELL_PANE_RGBA;
        uint32_t fg_rgba = idx == sel ? (uint32_t)CROSSTALK_UI_THEME_LIT_RGBA : (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA;

        if (label == 0x0) { continue; }

        ret = crosstalk_row_fill(render, left, row, (uint32_t)CROSSTALK_MENU_SWITCH_COL_CUNT, bg_rgba);
        if (ret == LDG_ERR_AOK) { ret = crosstalk_text_clip_put(render, left + (uint32_t)CROSSTALK_MENU_SWITCH_INDENT, row, (uint32_t)CROSSTALK_MENU_SWITCH_COL_CUNT - (uint32_t)CROSSTALK_MENU_SWITCH_INDENT, label, fg_rgba, bg_rgba, &drawn); }

        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_menu_switch_key_isr(crosstalk_ctx_t *ctx, const ldg_term_key_event_t *irq, uint32_t item_cunt, uint8_t *consumed)
{
    uint32_t sel_prev = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || irq == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    if (irq->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE || item_cunt == 0) { return LDG_ERR_AOK; }

    if (ctx->menu_sel >= item_cunt) { ctx->menu_sel = item_cunt - 0x1; }

    sel_prev = ctx->menu_sel;

    if (irq->key == (uint8_t)LDG_TERM_KEY_UP || irq->key == (uint8_t)LDG_TERM_KEY_LHS)
    {
        *consumed = LDG_TRUTH_TRUE;
        if (ctx->menu_sel > 0) { ctx->menu_sel = ctx->menu_sel - 0x1; }
    }
    else if (irq->key == (uint8_t)LDG_TERM_KEY_DOWN || irq->key == (uint8_t)LDG_TERM_KEY_RHS)
    {
        *consumed = LDG_TRUTH_TRUE;
        if (ctx->menu_sel + 0x1 < item_cunt) { ctx->menu_sel = ctx->menu_sel + 0x1; }
    }
    else if (irq->key == (uint8_t)LDG_TERM_KEY_ENTER)
    {
        *consumed = LDG_TRUTH_TRUE;
        ctx->menu_confirm = LDG_TRUTH_TRUE;
    }

    if (ctx->menu_sel != sel_prev)
    {
        crosstalk_game_fx_sfx_rotary_dir_t rotary_dir = (ctx->menu_sel > sel_prev) ? CROSSTALK_GAME_FX_SFX_ROTARY_CW : CROSSTALK_GAME_FX_SFX_ROTARY_CCW;

        if (crosstalk_game_fx_sfx_rotary_play(&ctx->game.sfx_sound, rotary_dir) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "rotary sfx failed"); }
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_menu_confirm_take(crosstalk_ctx_t *ctx, const uint8_t *item_hide, uint32_t item_cunt, uint32_t *out_idx)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_idx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT32_MAX;

    if (ctx->menu_confirm != LDG_TRUTH_TRUE) { return LDG_ERR_EMPTY; }

    ctx->menu_confirm = LDG_TRUTH_FALSE;

    if (ctx->menu_sel >= item_cunt) { return LDG_ERR_BOUNDS; }

    if (item_hide != 0x0 && item_hide[ctx->menu_sel] == LDG_TRUTH_TRUE) { return LDG_ERR_DENIED; }

    *out_idx = ctx->menu_sel;

    return LDG_ERR_AOK;
}

static void crosstalk_menu_nav_sync(crosstalk_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if ((uint32_t)ctx->game.screen == ctx->menu_screen_prev) { return; }

    ctx->menu_screen_prev = (uint32_t)ctx->game.screen;
    ctx->menu_action = (uint32_t)CROSSTALK_CORE_MENU_ACTION_NONE;
    ctx->menu_row = UINT32_MAX;
    ctx->menu_confirm = LDG_TRUTH_FALSE;

    if (crosstalk_game_menu_nav_rst(&ctx->menu_nav) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "menu nav rst failed"); }
}

static void crosstalk_menu_action_take(crosstalk_ctx_t *ctx, uint32_t *out_action, uint32_t *out_row)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out_action == 0x0 || out_row == 0x0)) { return; }

    *out_action = ctx->menu_action;
    *out_row = ctx->menu_row;

    ctx->menu_action = (uint32_t)CROSSTALK_CORE_MENU_ACTION_NONE;
    ctx->menu_row = UINT32_MAX;
}

static uint8_t crosstalk_mp_room_is(crosstalk_ctx_t *ctx)
{
    ldg_net_lobby_state_t lobby_state = LDG_NET_LOBBY_STATE_NONE;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    lobby_state = crosstalk_net_steam_lobby_state_get(&ctx->steam_net);

    if (lobby_state == LDG_NET_LOBBY_STATE_HOSTING || lobby_state == LDG_NET_LOBBY_STATE_JOINED) { return LDG_TRUTH_TRUE; }

    return crosstalk_net_steam_lan_sesh_get(&ctx->steam_net) != (uint8_t)CROSSTALK_NET_LAN_SESH_NONE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static void crosstalk_mp_room_get(crosstalk_ctx_t *ctx, uint32_t *out_member_cunt, uint8_t *out_is_host)
{
    uint8_t lan_sesh = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_member_cunt == 0x0 || out_is_host == 0x0)) { return; }

    lan_sesh = crosstalk_net_steam_lan_sesh_get(&ctx->steam_net);

    if (lan_sesh != (uint8_t)CROSSTALK_NET_LAN_SESH_NONE)
    {
        *out_is_host = (uint8_t)(lan_sesh == (uint8_t)CROSSTALK_NET_LAN_SESH_HOSTING ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE);
        *out_member_cunt = (uint32_t)CROSSTALK_CORE_MP_HOST_SELF + crosstalk_net_steam_client_cunt_get(&ctx->steam_net);
        return;
    }

    *out_is_host = (uint8_t)(crosstalk_net_steam_lobby_host_steam_id_get(&ctx->steam_net) == ldg_net_steamworks_local_steam_id_get() ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE);
    *out_member_cunt = crosstalk_net_steam_lobby_member_cunt_get(&ctx->steam_net);
}

static uint32_t crosstalk_menu_key_isr(crosstalk_ctx_t *ctx, const ldg_term_key_event_t *irq, uint8_t *consumed)
{
    uint32_t action = (uint32_t)CROSSTALK_CORE_MENU_ACTION_NONE;
    uint32_t row = UINT32_MAX;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || irq == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    crosstalk_menu_nav_sync(ctx);

    if (ctx->settings_open != LDG_TRUTH_FALSE)
    {
        uint8_t linked = crosstalk_net_discord_linked_is(&ctx->discord) == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

        if (crosstalk_net_discord_auth_get(&ctx->discord) == CROSSTALK_NET_DISCORD_AUTH_PENDING) { linked = LDG_TRUTH_TRUE; }

        return crosstalk_game_settings_menu_key_isr(&ctx->settings_menu, &ctx->game.settings, linked, irq, consumed);
    }

    if (ctx->game.screen == CROSSTALK_GAME_SCREEN_EDITOR)
    {
        if (ctx->game.paused == LDG_TRUTH_FALSE) { return LDG_ERR_AOK; }

        return crosstalk_menu_switch_key_isr(ctx, irq, (uint32_t)CROSSTALK_PAUSE_SWITCH_ITEM_CUNT, consumed);
    }

    if (ctx->game.screen == CROSSTALK_GAME_SCREEN_MENU) { return crosstalk_menu_switch_key_isr(ctx, irq, (uint32_t)CROSSTALK_MENU_SWITCH_ITEM_CUNT, consumed); }

    if (ctx->game.screen == CROSSTALK_GAME_SCREEN_MP)
    {
        ldg_net_lobby_state_t lobby_state = crosstalk_net_steam_lobby_state_get(&ctx->steam_net);

        if (lobby_state == LDG_NET_LOBBY_STATE_CREATING || lobby_state == LDG_NET_LOBBY_STATE_JOINING || lobby_state == LDG_NET_LOBBY_STATE_STARTING) { return LDG_ERR_AOK; }

        if (crosstalk_mp_room_is(ctx) == LDG_TRUTH_TRUE)
        {
            crosstalk_game_menu_room_action_t room_action = CROSSTALK_GAME_MENU_ROOM_NONE;
            uint32_t member_cunt = 0;
            uint8_t is_host = LDG_TRUTH_FALSE;

            crosstalk_mp_room_get(ctx, &member_cunt, &is_host);

            ret = crosstalk_game_menu_room_key_isr(irq, &ctx->menu_nav, member_cunt, is_host, ctx->mp_rdy, &room_action, consumed);
            action = (uint32_t)room_action;
        }
        else
        {
            crosstalk_game_menu_mp_action_t mp_action = CROSSTALK_GAME_MENU_MP_NONE;
            uint32_t list_cunt = (ctx->mp_transport_idx == (uint32_t)CROSSTALK_NET_TRANSPORT_LAN) ? crosstalk_net_steam_lan_list_cunt_get(&ctx->steam_net) : crosstalk_net_steam_lobby_list_cunt_get(&ctx->steam_net);

            ret = crosstalk_game_menu_mp_key_isr(irq, &ctx->menu_nav, list_cunt, ctx->board_sel.names, ctx->board_sel.cunt, &ctx->mp_board_idx, &ctx->mp_players_idx, &ctx->mp_submode_idx, &ctx->mp_transport_idx, &mp_action, &row, consumed);
            action = (uint32_t)mp_action;
        }
    }
    else if (ctx->game.screen == CROSSTALK_GAME_SCREEN_BOARD_SEL)
    {
        crosstalk_game_menu_board_sel_action_t sel_action = CROSSTALK_GAME_MENU_BOARD_SEL_NONE;

        ret = crosstalk_game_menu_board_sel_key_isr(irq, &ctx->menu_nav, ctx->board_sel.lock, ctx->board_sel.cunt, &ctx->board_sel_wshop_on, &sel_action, &row, consumed);
        action = (uint32_t)sel_action;
    }
    else if (ctx->game.screen == CROSSTALK_GAME_SCREEN_EXTRAS)
    {
        crosstalk_game_menu_extras_action_t extras_action = CROSSTALK_GAME_MENU_EXTRAS_NONE;

        ret = crosstalk_game_menu_extras_key_isr(irq, &ctx->menu_nav, ctx->extras_cunt, &extras_action, &row, consumed);
        action = (uint32_t)extras_action;
    }
    else if (ctx->game.screen == CROSSTALK_GAME_SCREEN_CREDITS)
    {
        crosstalk_game_menu_credits_action_t credits_action = CROSSTALK_GAME_MENU_CREDITS_NONE;

        ret = crosstalk_game_menu_credits_key_isr(irq, &ctx->menu_nav, ctx->credits_cunt, &credits_action, consumed);
        action = (uint32_t)credits_action;
    }
    else if (ctx->game.screen == CROSSTALK_GAME_SCREEN_EDITORPICK)
    {
        crosstalk_game_menu_editorpick_action_t pick_action = CROSSTALK_GAME_MENU_EDITORPICK_NONE;

        ret = crosstalk_game_menu_editorpick_key_isr(irq, &ctx->menu_nav, ctx->editorpick_items, &ctx->editorpick_idx, &pick_action, consumed);
        action = (uint32_t)pick_action;
    }
    else if (ctx->game.screen == CROSSTALK_GAME_SCREEN_NEWPROJECT)
    {
        crosstalk_game_menu_newproject_action_t np_action = CROSSTALK_GAME_MENU_NEWPROJECT_NONE;

        ret = crosstalk_game_menu_newproject_key_isr(irq, &ctx->menu_nav, ctx->board_name, (uint32_t)sizeof(ctx->board_name), ctx->newproject_items, ctx->newproject_pick_cunt, &ctx->newproject_pick, &np_action, consumed);
        action = (uint32_t)np_action;
    }

    if (ret != LDG_ERR_AOK) { return ret; }

    if (action != (uint32_t)CROSSTALK_CORE_MENU_ACTION_NONE)
    {
        ctx->menu_action = action;
        ctx->menu_row = row;
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_music_key_isr(crosstalk_ctx_t *ctx, const ldg_term_key_event_t *irq, uint8_t *consumed)
{
    const uint32_t chord = (uint32_t)LDG_TERM_MOD_CTRL | (uint32_t)LDG_TERM_MOD_SHIFT;
    uint32_t action = (uint32_t)UINT32_MAX;
    uint32_t codepoint = 0;
    uint32_t ret = 0;
    uint8_t ld_req = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0 || irq == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    if (irq->kind == (uint8_t)LDG_TERM_KEY_KIND_RELEASE || (irq->mods & chord) != chord) { return LDG_ERR_AOK; }

    codepoint = irq->codepoint;

    if (codepoint == (uint32_t)CROSSTALK_CORE_MUSIC_KEY_PREV || codepoint == (uint32_t)CROSSTALK_CORE_MUSIC_KEY_PREV_SHIFTED) { action = (uint32_t)CROSSTALK_GAME_MUSIC_ACTION_PREV; }
    else if (codepoint == (uint32_t)CROSSTALK_CORE_MUSIC_KEY_NEXT || codepoint == (uint32_t)CROSSTALK_CORE_MUSIC_KEY_NEXT_SHIFTED) { action = (uint32_t)CROSSTALK_GAME_MUSIC_ACTION_NEXT; }
    else if (codepoint == (uint32_t)CROSSTALK_CORE_MUSIC_KEY_TOGGLE || codepoint == (uint32_t)CROSSTALK_CORE_MUSIC_KEY_TOGGLE_SHIFTED) { action = (uint32_t)CROSSTALK_GAME_MUSIC_ACTION_TOGGLE; }
    else { return LDG_ERR_AOK; }

    *consumed = LDG_TRUTH_TRUE;

    ret = crosstalk_game_music_transport(&ctx->music, action, &ld_req);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ld_req != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = crosstalk_ost_start(ctx);

    if (ret == LDG_ERR_DENIED) { toast_push(ctx, CROSSTALK_CORE_OST_UNOWNED_TEXT); }
    else if (ret == LDG_ERR_AGAIN) { toast_push(ctx, CROSSTALK_CORE_OST_DOWNLOAD_TEXT); }
    else if (ret == LDG_ERR_NOT_FOUND) { toast_push(ctx, CROSSTALK_CORE_OST_MISPLACED_TEXT); }
    else if (ret != LDG_ERR_AOK) { toast_push(ctx, CROSSTALK_CORE_OST_FAIL_TEXT); }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_global_key_isr(crosstalk_ctx_t *ctx, const ldg_term_key_event_t *irq, uint8_t *consumed)
{
    uint8_t ctrl_is = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0 || irq == 0x0 || consumed == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    if (irq->kind != (uint8_t)LDG_TERM_KEY_KIND_PRESS) { return LDG_ERR_AOK; }

    ctrl_is = (irq->mods & (uint32_t)LDG_TERM_MOD_CTRL) != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (irq->key == (uint8_t)LDG_TERM_KEY_ESCAPE)
    {
        ctx->key_esc = LDG_TRUTH_TRUE;
        ctx->key_esc_shift = (irq->mods & (uint32_t)LDG_TERM_MOD_SHIFT) != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    }
    else if (irq->key == (uint8_t)LDG_TERM_KEY_F4) { ctx->key_invite = LDG_TRUTH_TRUE; }
    else if (ctrl_is == LDG_TRUTH_TRUE && irq->codepoint == (uint32_t)CROSSTALK_CORE_KEY_SAVE_CODEPOINT) { ctx->key_save = LDG_TRUTH_TRUE; }
    else if (ctrl_is == LDG_TRUTH_TRUE && irq->codepoint == (uint32_t)CROSSTALK_CORE_KEY_OPEN_CODEPOINT) { ctx->key_open = LDG_TRUTH_TRUE; }
    else { return LDG_ERR_AOK; }

    *consumed = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_statusline_draw(ldg_term_render_t *render, uint32_t row, uint32_t col_cunt, const char **cell, uint32_t cell_cunt)
{
    uint32_t col = 0;
    uint32_t drawn = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || cell == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_row_fill(render, col, row, col_cunt, (uint32_t)CROSSTALK_STATUSLINE_BG_COL);
    if (ret != LDG_ERR_AOK) { return ret; }

    for (idx = 0; idx < cell_cunt && col < col_cunt; idx = idx + 0x1)
    {
        if (cell[idx] == 0x0 || cell[idx][0] == LDG_STR_TERM) { continue; }

        if (col != 0)
        {
            ret = crosstalk_text_clip_put(render, col, row, col_cunt - col, CROSSTALK_STATUSLINE_SEP, (uint32_t)CROSSTALK_UI_THEME_DIM_RGBA, (uint32_t)CROSSTALK_STATUSLINE_BG_COL, &drawn);
            if (ret != LDG_ERR_AOK) { return ret; }

            col = col + drawn;
        }

        if (col >= col_cunt) { break; }

        ret = crosstalk_text_clip_put(render, col, row, col_cunt - col, cell[idx], (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, (uint32_t)CROSSTALK_STATUSLINE_BG_COL, &drawn);
        if (ret != LDG_ERR_AOK) { return ret; }

        col = col + drawn;
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_busy_draw(ldg_term_render_t *render, const ldg_term_rect_t *rect, const char **line, uint32_t line_cunt)
{
    uint32_t box_w = (uint32_t)CROSSTALK_CORE_BUSY_COL_CUNT;
    uint32_t box_h = (uint32_t)CROSSTALK_CORE_BUSY_ROW_CUNT;
    uint32_t top = 0;
    uint32_t left = 0;
    uint32_t drawn = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(render == 0x0 || rect == 0x0 || line == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (box_w > rect->col_cunt) { box_w = rect->col_cunt; }

    if (box_h > rect->row_cunt) { box_h = rect->row_cunt; }

    top = rect->row + (rect->row_cunt - box_h) / (uint32_t)CROSSTALK_MENU_SWITCH_CENTER_DIV;
    left = rect->col + (rect->col_cunt - box_w) / (uint32_t)CROSSTALK_MENU_SWITCH_CENTER_DIV;

    for (idx = 0; idx < box_h; idx = idx + 0x1)
    {
        ret = crosstalk_row_fill(render, left, top + idx, box_w, (uint32_t)CROSSTALK_UI_THEME_SEL_BG_RGBA);
        if (ret != LDG_ERR_AOK) { return ret; }

        if (idx >= line_cunt || line[idx] == 0x0) { continue; }

        ret = crosstalk_text_clip_put(render, left + (uint32_t)CROSSTALK_MENU_SWITCH_INDENT, top + idx, box_w - (uint32_t)CROSSTALK_MENU_SWITCH_INDENT, line[idx], idx == 0 ? (uint32_t)CROSSTALK_UI_THEME_LIT_RGBA : (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, (uint32_t)CROSSTALK_UI_THEME_SEL_BG_RGBA, &drawn);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_term_frame_begin(crosstalk_ctx_t *ctx, ldg_term_render_t **out_render, ldg_term_rect_t *out_screen)
{
    ldg_term_render_color_t clear = LDG_STRUCT_ZERO_INIT;
    uint32_t col_cunt = 0;
    uint32_t row_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out_render == 0x0 || out_screen == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_render = 0x0;
    memset(out_screen, 0x0, sizeof(*out_screen));

    if (ctx->sesh == 0x0) { return LDG_ERR_NOT_INIT; }

    if (ldg_sys_tty_size_get(&col_cunt, &row_cunt) == LDG_ERR_AOK && col_cunt != 0 && row_cunt != 0)
    {
        if (col_cunt > (uint32_t)CROSSTALK_CORE_SESH_COL_MAX) { col_cunt = (uint32_t)CROSSTALK_CORE_SESH_COL_MAX; }

        if (row_cunt > (uint32_t)CROSSTALK_CORE_SESH_ROW_MAX) { row_cunt = (uint32_t)CROSSTALK_CORE_SESH_ROW_MAX; }

        if (col_cunt != ctx->term_col_cunt || row_cunt != ctx->term_row_cunt)
        {
            ret = ldg_term_sesh_resize(ctx->sesh, col_cunt, row_cunt);
            if (ret != LDG_ERR_AOK) { return ret; }

            ctx->term_col_cunt = col_cunt;
            ctx->term_row_cunt = row_cunt;
        }
    }

    ret = ldg_term_sesh_render_get(ctx->sesh, out_render);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_ui_cell_color_get((uint32_t)CROSSTALK_UI_CELL_PANE_RGBA, &clear);
    if (ret == LDG_ERR_AOK) { ret = ldg_term_render_clear(*out_render, &clear); }

    if (ret != LDG_ERR_AOK)
    {
        *out_render = 0x0;
        return ret;
    }

    out_screen->col_cunt = (*out_render)->col_cunt;
    out_screen->row_cunt = (*out_render)->row_cunt;

    return LDG_ERR_AOK;
}

static uint32_t crosstalk_core_irq_route(crosstalk_ctx_t *ctx, const ldg_term_key_event_t *irq)
{
    uint8_t consumed = LDG_TRUTH_FALSE;
    uint8_t overlay_on = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || irq == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->idle_secs = CROSSTALK_CORE_IDLE_RST;

    if (ctx->settings_open != LDG_TRUTH_FALSE || (ctx->game.screen == CROSSTALK_GAME_SCREEN_EDITOR && ctx->game.paused != LDG_TRUTH_FALSE)) { overlay_on = LDG_TRUTH_TRUE; }

    if (overlay_on != LDG_TRUTH_TRUE)
    {
        ret = crosstalk_pause_key_isr(ctx, irq, &consumed);
        if (ret != LDG_ERR_AOK || consumed == LDG_TRUTH_TRUE) { return ret; }

        ret = crosstalk_win_shot_key_isr(ctx, irq, &consumed);
        if (ret != LDG_ERR_AOK || consumed == LDG_TRUTH_TRUE) { return ret; }

        ret = crosstalk_hub_key_isr(ctx, irq, &consumed);
        if (ret != LDG_ERR_AOK || consumed == LDG_TRUTH_TRUE) { return ret; }

        ret = crosstalk_win_resize_key_isr(ctx, irq, &consumed);
        if (ret != LDG_ERR_AOK || consumed == LDG_TRUTH_TRUE) { return ret; }

        ret = crosstalk_win_nav_key_isr(ctx, irq, &consumed);
        if (ret != LDG_ERR_AOK || consumed == LDG_TRUTH_TRUE) { return ret; }

        ret = crosstalk_core_cmdline_key_isr(&ctx->cmdline, irq, &consumed);
        if (ret != LDG_ERR_AOK || consumed == LDG_TRUTH_TRUE) { return ret; }
    }

    ret = crosstalk_music_key_isr(ctx, irq, &consumed);
    if (ret != LDG_ERR_AOK || consumed == LDG_TRUTH_TRUE) { return ret; }

    if (overlay_on != LDG_TRUTH_TRUE)
    {
        ret = crosstalk_receptacle_key_isr(ctx, irq, &consumed);
        if (ret != LDG_ERR_AOK || consumed == LDG_TRUTH_TRUE) { return ret; }

        ret = crosstalk_ui_window_key_route(&ctx->win_nav, irq, &consumed);
        if (ret != LDG_ERR_AOK || consumed == LDG_TRUTH_TRUE) { return ret; }
    }

    ret = crosstalk_menu_key_isr(ctx, irq, &consumed);
    if (ret != LDG_ERR_AOK || consumed == LDG_TRUTH_TRUE) { return ret; }

    return crosstalk_global_key_isr(ctx, irq, &consumed);
}

static uint32_t crosstalk_core_irq_drain(crosstalk_ctx_t *ctx)
{
    ldg_term_key_event_t irq = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->irq_src == 0x0) { return LDG_ERR_AOK; }

    ret = ldg_term_input_poll(ctx->irq_src);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (ldg_term_input_event_next(ctx->irq_src, &irq) == LDG_ERR_AOK)
    {
        ret = crosstalk_ui_key_us_map(&irq);
        if (ret == LDG_ERR_AOK) { ret = crosstalk_core_irq_route(ctx, &irq); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "irq: isr failed; key: %u32; codepoint: %u32; mods: 0x%X32; err: %u32", (uint32_t)irq.key, irq.codepoint, irq.mods, ret); }
    }

    return LDG_ERR_AOK;
}

static void crosstalk_friend_started_tick(crosstalk_ctx_t *ctx)
{
    uint32_t friend_cunt = crosstalk_net_steam_friend_cunt_get(&ctx->steam_net);

    for (uint32_t idx = 0; idx < friend_cunt; idx++)
    {
        const crosstalk_net_steam_friend_t *mate = 0x0;
        char notice[CROSSTALK_NET_NOTICE_CAP] = LDG_ARR_ZERO_INIT;

        if (crosstalk_net_steam_friend_started_take(&ctx->steam_net, idx) != LDG_TRUTH_TRUE) { continue; }

        mate = crosstalk_net_steam_friend_get(&ctx->steam_net, idx);
        if (mate == 0x0) { continue; }

        snprintf(notice, sizeof(notice), "%.63s started Crosstalk", mate->name[0] != LDG_STR_TERM ? mate->name : "a friend");
        crosstalk_notice_raise(ctx, CROSSTALK_CORE_NOTIFY_TITLE_SESH, notice);
    }
}

static uint8_t sim_paused_is(const crosstalk_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->game.paused == 0) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static uint8_t crosstalk_peer_state_self_get(const crosstalk_ctx_t *ctx)
{
    if (sim_paused_is(ctx) == LDG_TRUTH_TRUE) { return (uint8_t)CROSSTALK_NET_WIRE_PEER_STATE_PAUSED; }

    if (ctx->idle_secs >= CROSSTALK_CORE_PEER_STATE_AFK_SECS) { return (uint8_t)CROSSTALK_NET_WIRE_PEER_STATE_AFK; }

    return (uint8_t)CROSSTALK_NET_WIRE_PEER_STATE_ACTIVE;
}

static void crosstalk_focus_pause_tick(crosstalk_ctx_t *ctx)
{
    if (ctx->game.settings.focus_pause_on != LDG_TRUTH_TRUE)
    {
        if (ctx->paused_by_focus == LDG_TRUTH_TRUE)
        {
            ctx->game.paused = LDG_TRUTH_FALSE;
            ctx->paused_by_focus = LDG_TRUTH_FALSE;
        }

        return;
    }

    if (ctx->game.screen != CROSSTALK_GAME_SCREEN_EDITOR)
    {
        ctx->paused_by_focus = LDG_TRUTH_FALSE;
        return;
    }

    if (ldg_sys_focus_is() != LDG_TRUTH_TRUE)
    {
        if (ctx->game.paused == LDG_TRUTH_FALSE)
        {
            ctx->game.paused = LDG_TRUTH_TRUE;
            ctx->paused_by_focus = LDG_TRUTH_TRUE;
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "crosstalk: paused on focus loss");
        }

        return;
    }

    if (ctx->paused_by_focus == LDG_TRUTH_FALSE) { return; }

    ctx->game.paused = LDG_TRUTH_FALSE;
    ctx->paused_by_focus = LDG_TRUTH_FALSE;
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "crosstalk: resumed on focus regain");
}

static void crosstalk_idle_tick(crosstalk_ctx_t *ctx)
{
    ctx->idle_secs = ctx->idle_secs + ctx->dt;
}

static void crosstalk_peer_state_send(crosstalk_ctx_t *ctx)
{
    crosstalk_net_wire_peer_state_t wire_state = LDG_STRUCT_ZERO_INIT;
    uint8_t self = crosstalk_peer_state_self_get(ctx);
    uint32_t ret = 0;

    if (ctx->steam_net.sesh_live == 0) { return; }

    if (ctx->peer_state_next > 0.0) { ctx->peer_state_next = ctx->peer_state_next - ctx->dt; }

    if (self == ctx->peer_state_sent && ctx->peer_state_next > 0.0) { return; }

    ctx->peer_state_next = CROSSTALK_CORE_PEER_STATE_PERIOD;
    ctx->peer_state_sent = self;

    wire_state.steam_id = ldg_net_steamworks_local_steam_id_get();
    wire_state.idle_ms = (uint64_t)(ctx->idle_secs * (double)LDG_MS_PER_SEC);
    wire_state.state = self;
    wire_state.focused = ldg_sys_focus_is();

    crosstalk_peer_state_apply(ctx, &wire_state);

    ret = crosstalk_net_wire_send(&ctx->steam_net, CROSSTALK_NET_WIRE_KIND_PEER_STATE, &wire_state, (uint32_t)sizeof(wire_state), CROSSTALK_NET_WIRE_RELIABLE);
    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "net: peer state send failed; err: %u32", ret); }
}

uint32_t crosstalk_core_join_lobby_set(crosstalk_ctx_t *ctx, uint64_t lobby_id)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(lobby_id == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ctx->join_lobby_pending = lobby_id;
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: launch join pending; lobby: %u64", lobby_id);

    return LDG_ERR_AOK;
}

static void crosstalk_discord_auth_sync(crosstalk_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (ctx->discord_restore_req != LDG_TRUTH_TRUE)
    {
        ctx->discord_restore_req = LDG_TRUTH_TRUE;
        ret = crosstalk_net_discord_auth_restore(&ctx->discord);

        if (ret == LDG_ERR_AOK)
        {
            ctx->discord_auth_req = LDG_TRUTH_TRUE;
            ctx->game.settings.discord_link_on = LDG_TRUTH_TRUE;
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: session restore started");
            return;
        }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: no stored session; err: %u32", ret);
        return;
    }

    if (ctx->discord_auth_req != LDG_TRUTH_TRUE) { return; }

    if (crosstalk_net_discord_auth_get(&ctx->discord) != CROSSTALK_NET_DISCORD_AUTH_NONE) { return; }

    if (crosstalk_net_discord_state_get(&ctx->discord) != CROSSTALK_NET_DISCORD_STATE_FAIL) { return; }

    ctx->discord_auth_req = LDG_TRUTH_FALSE;

    if (ctx->discord_relink_req == LDG_TRUTH_TRUE) { return; }

    ctx->discord_relink_req = LDG_TRUTH_TRUE;
    ret = crosstalk_net_discord_auth_link_local(&ctx->discord);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "discord: stored session unusable; auto relink req; err: %u32", ret);
}

static void crosstalk_hub_rows_sync(crosstalk_ctx_t *ctx)
{
    char status[CROSSTALK_GAME_HUB_STATUS_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t contact_cunt = 0;
    uint32_t friend_cunt = 0;
    uint32_t ret = 0;

    ret = crosstalk_game_hub_rows_rst(&ctx->hub);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "hub rows reset failed"); }

    friend_cunt = crosstalk_net_steam_friend_cunt_get(&ctx->steam_net);

    for (uint32_t idx = 0; idx < friend_cunt; idx++)
    {
        const crosstalk_net_steam_friend_t *mate = crosstalk_net_steam_friend_get(&ctx->steam_net, idx);

        if (mate == 0x0) { continue; }

        uint8_t reach = (uint8_t)CROSSTALK_GAME_HUB_REACH_OFFLINE;

        status[0] = LDG_STR_TERM;

        if (mate->in_ours == LDG_TRUTH_TRUE && mate->detail[0] != LDG_STR_TERM) { snprintf(status, sizeof(status), "%.31s", mate->detail); }
        else if (mate->in_ours == LDG_TRUTH_FALSE && mate->in_game == LDG_TRUTH_TRUE) { snprintf(status, sizeof(status), "%.31s", CROSSTALK_CORE_HUB_OTHER_GAME_TEXT); }

        if (mate->state == (uint32_t)LDG_NET_STEAMWORKS_PERSONA_STATE_ONLINE) { reach = (uint8_t)CROSSTALK_GAME_HUB_REACH_ONLINE; }
        else if (mate->state != (uint32_t)LDG_NET_STEAMWORKS_PERSONA_STATE_OFFLINE) { reach = (uint8_t)CROSSTALK_GAME_HUB_REACH_AWAY; }

        if (mate->in_game == LDG_TRUTH_TRUE) { reach = (uint8_t)CROSSTALK_GAME_HUB_REACH_OTHER_GAME; }

        if (mate->in_ours == LDG_TRUTH_TRUE) { reach = (uint8_t)CROSSTALK_GAME_HUB_REACH_IN_GAME; }

        if (mate->in_ours == LDG_TRUTH_TRUE && mate->lobby_id != 0) { reach = (uint8_t)CROSSTALK_GAME_HUB_REACH_JOINABLE; }

        ret = crosstalk_game_chat_conv_reach_set(&ctx->chat, (uint8_t)CROSSTALK_GAME_CHAT_GROUP_STEAM, mate->steam_id, reach);
        if (ret != LDG_ERR_AOK && ret != LDG_ERR_NOT_FOUND) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "chat steam reach set failed"); }

        ret = crosstalk_game_hub_row_add(&ctx->hub, mate->steam_id, mate->name[0] != LDG_STR_TERM ? mate->name : "someone", status, (uint8_t)CROSSTALK_GAME_HUB_SRC_STEAM, reach, mate->lobby_id);
        if (ret == LDG_ERR_FULL) { break; }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "hub steam row add failed"); }
    }

    contact_cunt = (ctx->discord_live == LDG_TRUTH_TRUE) ? crosstalk_net_discord_contact_cunt_get(&ctx->discord) : CROSSTALK_NET_CUNT_NONE;

    for (uint32_t idx = 0; idx < contact_cunt; idx++)
    {
        const crosstalk_net_discord_contact_t *contact = crosstalk_net_discord_contact_get(&ctx->discord, idx);
        uint8_t reach = (uint8_t)CROSSTALK_GAME_HUB_REACH_OFFLINE;

        if (contact == 0x0) { continue; }

        {
            const char *verb = crosstalk_net_discord_activity_verb_get(contact->activity);

            status[0] = LDG_STR_TERM;

            if (contact->ours == LDG_TRUTH_TRUE) { snprintf(status, sizeof(status), "%.31s", contact->activity_name); }
            else if (contact->activity_name[0] != LDG_STR_TERM && verb[0] != LDG_STR_TERM) { snprintf(status, sizeof(status), "%.12s %.18s", verb, contact->activity_name); }
            else if (contact->activity_name[0] != LDG_STR_TERM) { snprintf(status, sizeof(status), "%.31s", contact->activity_name); }
            else if (verb[0] != LDG_STR_TERM) { snprintf(status, sizeof(status), "%.31s", verb); }
        }

        if (contact->status == (uint32_t)CROSSTALK_NET_DISCORD_STATUS_ONLINE || contact->status == (uint32_t)CROSSTALK_NET_DISCORD_STATUS_STREAMING) { reach = (uint8_t)CROSSTALK_GAME_HUB_REACH_ONLINE; }
        else if (contact->status == (uint32_t)CROSSTALK_NET_DISCORD_STATUS_IDLE || contact->status == (uint32_t)CROSSTALK_NET_DISCORD_STATUS_DND) { reach = (uint8_t)CROSSTALK_GAME_HUB_REACH_AWAY; }

        if (contact->activity == (uint8_t)CROSSTALK_NET_DISCORD_ACTIVITY_PLAYING && contact->ours == LDG_TRUTH_FALSE) { reach = (uint8_t)CROSSTALK_GAME_HUB_REACH_OTHER_GAME; }

        if (contact->ours == LDG_TRUTH_TRUE) { reach = (uint8_t)CROSSTALK_GAME_HUB_REACH_IN_GAME; }

        if (contact->kind == (uint8_t)CROSSTALK_NET_DISCORD_CONTACT_KIND_LOBBY) { reach = (uint8_t)CROSSTALK_GAME_HUB_REACH_IN_GAME; }

        ret = crosstalk_game_chat_conv_reach_set(&ctx->chat, (uint8_t)CROSSTALK_GAME_CHAT_GROUP_DISCORD, contact->user_id, reach);
        if (ret != LDG_ERR_AOK && ret != LDG_ERR_NOT_FOUND) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "chat discord reach set failed"); }

        ret = crosstalk_game_hub_row_add(&ctx->hub, contact->user_id, contact->name[0] != LDG_STR_TERM ? contact->name : "someone", status, (uint8_t)CROSSTALK_GAME_HUB_SRC_DISCORD, reach, CROSSTALK_NET_LOBBY_ID_NONE);
        if (ret == LDG_ERR_FULL) { break; }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "hub discord row add failed"); }
    }

    ret = crosstalk_game_hub_rows_sort(&ctx->hub);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "hub rows sort failed"); }
}

static void crosstalk_hub_action_apply(crosstalk_ctx_t *ctx, crosstalk_game_hub_action_t action, uint64_t user_id, const char *name, uint8_t src)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(user_id == 0)) { return; }

    if (src == (uint8_t)CROSSTALK_GAME_HUB_SRC_STEAM)
    {
        if (action == CROSSTALK_GAME_HUB_ACTION_DM)
        {
            ret = crosstalk_game_chat_conv_sel(&ctx->chat, (uint8_t)CROSSTALK_GAME_CHAT_GROUP_STEAM, user_id, name);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "hub steam target set failed"); }

            ret = crosstalk_game_chat_open_set(&ctx->chat, LDG_TRUTH_TRUE);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "hub chat open failed"); }

            crosstalk_hub_chat_show(ctx);

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "hub: steam target set; id: %u64; name: %.63s", user_id, name);
            return;
        }

        {
            char connect[CROSSTALK_NET_PRESENCE_CONNECT_CAP] = LDG_ARR_ZERO_INIT;
            uint64_t own_lobby = crosstalk_net_steam_lobby_id_get(&ctx->steam_net);

            if (own_lobby == 0)
            {
                toast_push(ctx, "host a session first");
                return;
            }

            snprintf(connect, sizeof(connect), CROSSTALK_NET_PRESENCE_CONNECT_FMT, own_lobby);
            ret = ldg_net_steamworks_friend_game_invite(user_id, connect);
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "hub: steam invite; to: %u64; lobby: %u64; err: %u32", user_id, own_lobby, ret);

            if (ret == LDG_ERR_AOK) { toast_push(ctx, "Steam invite sent"); }
            else { toast_push(ctx, "Steam refused the invite"); }
        }

        return;
    }

    if (src != (uint8_t)CROSSTALK_GAME_HUB_SRC_DISCORD) { return; }

    if (action == CROSSTALK_GAME_HUB_ACTION_DM)
    {
        ret = crosstalk_game_chat_conv_sel(&ctx->chat, (uint8_t)CROSSTALK_GAME_CHAT_GROUP_DISCORD, user_id, name);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "hub dm target set failed"); }

        ret = crosstalk_net_discord_dm_history_req(&ctx->discord, user_id);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "discord: dm history skipped; err: %u32", ret); }

        ret = crosstalk_game_chat_open_set(&ctx->chat, LDG_TRUTH_TRUE);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "hub chat open failed"); }

        crosstalk_hub_chat_show(ctx);

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "hub: dm target set; id: %u64; name: %.63s", user_id, name);
        return;
    }

    ret = crosstalk_net_discord_invite_send(&ctx->discord, user_id, CROSSTALK_CORE_DISCORD_INVITE_TEXT);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "hub: invite send; to: %u64; err: %u32", user_id, ret);

    if (ret == LDG_ERR_AOK) { toast_push(ctx, "Discord invite sent"); }
    else if (ret == LDG_ERR_INVALID) { toast_push(ctx, "host a joinable lobby first"); }
    else { toast_push(ctx, "Discord refused the invite"); }
}

static void crosstalk_presence_si_fmt(uint64_t val, char *out, uint64_t cap)
{
    if (LDG_UNLIKELY(out == 0x0 || cap == 0)) { return; }

    if (val >= (uint64_t)CROSSTALK_STATUSLINE_CYC_TERA) { snprintf(out, cap, CROSSTALK_CORE_PRESENCE_SI_FMT, val / (uint64_t)CROSSTALK_STATUSLINE_CYC_TERA, CROSSTALK_STATUSLINE_CYC_UNIT_TERA); }
    else if (val >= (uint64_t)CROSSTALK_STATUSLINE_CYC_GIGA) { snprintf(out, cap, CROSSTALK_CORE_PRESENCE_SI_FMT, val / (uint64_t)CROSSTALK_STATUSLINE_CYC_GIGA, CROSSTALK_STATUSLINE_CYC_UNIT_GIGA); }
    else if (val >= (uint64_t)CROSSTALK_STATUSLINE_CYC_MEGA) { snprintf(out, cap, CROSSTALK_CORE_PRESENCE_SI_FMT, val / (uint64_t)CROSSTALK_STATUSLINE_CYC_MEGA, CROSSTALK_STATUSLINE_CYC_UNIT_MEGA); }
    else if (val >= (uint64_t)CROSSTALK_STATUSLINE_CYC_KILO) { snprintf(out, cap, CROSSTALK_CORE_PRESENCE_SI_FMT, val / (uint64_t)CROSSTALK_STATUSLINE_CYC_KILO, CROSSTALK_STATUSLINE_CYC_UNIT_KILO); }
    else { snprintf(out, cap, CROSSTALK_CORE_PRESENCE_SI_FMT, val, ""); }
}

static const char* crosstalk_presence_window_name_get(uint64_t id)
{
    if (id == (uint64_t)CROSSTALK_EDA_UI_SCOPE_WINDOW_ID) { return CROSSTALK_CORE_WINDOW_NAME_SCOPE; }

    if (id == (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_WINDOW_ID) { return CROSSTALK_CORE_WINDOW_NAME_MEMVIEW; }

    if (id == (uint64_t)CROSSTALK_EDA_UI_DISPLAY_CRT_WINDOW_ID) { return CROSSTALK_CORE_WINDOW_NAME_CRT; }

    if (id == (uint64_t)CROSSTALK_EDA_UI_PROFILE_WINDOW_ID) { return CROSSTALK_CORE_WINDOW_NAME_PROFILE; }

    if (id == (uint64_t)CROSSTALK_EDA_UI_CALC_WINDOW_ID) { return CROSSTALK_CORE_WINDOW_NAME_CALC; }

    if (id == (uint64_t)CROSSTALK_EDA_UI_INDICATOR_WINDOW_ID) { return CROSSTALK_CORE_WINDOW_NAME_INDICATOR; }

    if (id == (uint64_t)CROSSTALK_EDA_UI_WSHOP_BROWSER_WINDOW_ID) { return CROSSTALK_CORE_WINDOW_NAME_WSHOP; }

    if (id == (uint64_t)CROSSTALK_EDA_UI_WSHOP_PUBLISH_WINDOW_ID) { return CROSSTALK_CORE_WINDOW_NAME_WSHOP_PUB; }

    if (id == (uint64_t)CROSSTALK_EDA_UI_TERM_WINDOW_ID) { return CROSSTALK_CORE_WINDOW_NAME_TERM; }

    return 0x0;
}

static void crosstalk_presence_op_get(crosstalk_ctx_t *ctx, char *out, uint64_t out_cap)
{
    out[0] = LDG_STR_TERM;

    if (crosstalk_eda_io_cast_on_is(&ctx->eda.cast) == LDG_TRUTH_TRUE)
    {
        snprintf(out, out_cap, CROSSTALK_CORE_CAST_PRESENCE_FMT, crosstalk_eda_io_display_kind_name_get(&ctx->eda.display), ctx->eda.serial.open_path);
        return;
    }

    if (ctx->pipe_handle != UINT32_MAX && crosstalk_eda_logic_stream_live_is(&ctx->eda.logic_stream, ctx->pipe_handle) == LDG_TRUTH_TRUE)
    {
        snprintf(out, out_cap, "%.63s", CROSSTALK_CORE_PIPE_PRESENCE_TEXT);
        return;
    }

    if (crosstalk_eda_ic_fpga_run_live_is(&ctx->eda.fpga_run) == LDG_TRUTH_TRUE)
    {
        char rate_text[CROSSTALK_CORE_PRESENCE_SI_CAP] = LDG_ARR_ZERO_INIT;
        char cyc_text[CROSSTALK_CORE_PRESENCE_SI_CAP] = LDG_ARR_ZERO_INIT;
        const crosstalk_eda_ic_fpga_slot_t *slot = &ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT];
        uint64_t now_ms = ldg_time_epoch_ms_get();
        uint64_t rate_val = 0;
        uint64_t cyc_val = 0;

        if (now_ms >= ctx->presence_num_next_ms)
        {
            if (crosstalk_eda_ic_fpga_run_rate_get(&ctx->eda.fpga_run, &ctx->presence_rate) != LDG_ERR_AOK) { ctx->presence_rate = 0; }

            if (crosstalk_eda_ic_fpga_run_cycles_get(&ctx->eda.fpga_run, &ctx->presence_cyc) != LDG_ERR_AOK) { ctx->presence_cyc = 0; }

            ctx->presence_num_next_ms = now_ms + (uint64_t)CROSSTALK_CORE_PRESENCE_NUM_REFRESH_MS;
        }

        rate_val = ctx->presence_rate;
        cyc_val = ctx->presence_cyc;

        crosstalk_presence_si_fmt(rate_val, rate_text, (uint64_t)sizeof(rate_text));
        crosstalk_presence_si_fmt(cyc_val, cyc_text, (uint64_t)sizeof(cyc_text));

        if (slot->emu == LDG_TRUTH_TRUE) { if (ctx->prog_pub_name[0] != LDG_STR_TERM && rate_val != 0) { snprintf(out, out_cap, CROSSTALK_CORE_EMU_PROG_PRESENCE_RATE_FMT, ctx->prog_pub_name, rate_text, cyc_text); }
            else if (ctx->prog_pub_name[0] != LDG_STR_TERM) { snprintf(out, out_cap, CROSSTALK_CORE_EMU_PROG_PRESENCE_FMT, ctx->prog_pub_name); }
            else if (rate_val != 0) { snprintf(out, out_cap, CROSSTALK_CORE_EMU_PRESENCE_RATE_FMT, rate_text, cyc_text); }
            else { snprintf(out, out_cap, "%.63s", CROSSTALK_CORE_EMU_PRESENCE_TEXT); } }
        else if (slot->abi != 0x0) { if (rate_val != 0) { snprintf(out, out_cap, CROSSTALK_CORE_EXEC_PRESENCE_RATE_FMT, crosstalk_eda_ic_fpga_running_top_get(&ctx->eda.fpga), rate_text, cyc_text); }
            else { snprintf(out, out_cap, CROSSTALK_CORE_EXEC_PRESENCE_FMT, crosstalk_eda_ic_fpga_running_top_get(&ctx->eda.fpga)); } }
        else if (rate_val != 0) { snprintf(out, out_cap, CROSSTALK_CORE_FPGA_PRESENCE_RATE_FMT, crosstalk_eda_ic_fpga_running_top_get(&ctx->eda.fpga), rate_text, cyc_text); }
        else { snprintf(out, out_cap, CROSSTALK_CORE_FPGA_PRESENCE_FMT, crosstalk_eda_ic_fpga_running_top_get(&ctx->eda.fpga)); }

        return;
    }

    if (crosstalk_eda_ic_fpga_build_live_is(&ctx->eda.fpga_build) == LDG_TRUTH_TRUE)
    {
        snprintf(out, out_cap, CROSSTALK_CORE_BUILD_PRESENCE_FMT, crosstalk_eda_ic_fpga_running_top_get(&ctx->eda.fpga));
        return;
    }
}

static void crosstalk_presence_attention_get(crosstalk_ctx_t *ctx, char *out, uint64_t out_cap)
{
    uint64_t focus_id = ctx->win_nav.focus_id;
    const char *name = 0x0;

    out[0] = LDG_STR_TERM;

    if (focus_id == (uint64_t)UINT64_MAX) { return; }

    if (focus_id == (uint64_t)CROSSTALK_EDA_UI_TERM_WINDOW_ID)
    {
        uint32_t src = ctx->eda.term.sesh[ctx->eda.term.sesh_active].src;
        uint8_t live = ctx->eda.term.sesh[ctx->eda.term.sesh_active].live;

        if (src == (uint32_t)CROSSTALK_EDA_UI_TERM_SRC_SERIAL && live == LDG_TRUTH_TRUE)
        {
            const char *title = crosstalk_eda_ui_term_title_get(&ctx->eda.term);
            const char *port = 0x0;

            if (crosstalk_eda_io_serial_id_tail_get(title, &port) != LDG_ERR_AOK || port == 0x0 || port[0] == LDG_STR_TERM) { port = title; }

            if (port != 0x0 && port[0] != LDG_STR_TERM)
            {
                snprintf(out, out_cap, CROSSTALK_CORE_TERM_PRESENCE_FMT, port);
                return;
            }
        }

        if (src == (uint32_t)CROSSTALK_EDA_UI_TERM_SRC_PTY && live == LDG_TRUTH_TRUE)
        {
            snprintf(out, out_cap, "%.63s", CROSSTALK_CORE_PTY_PRESENCE_TEXT);
            return;
        }

        if (src == (uint32_t)CROSSTALK_EDA_UI_TERM_SRC_STREAM && live == LDG_TRUTH_TRUE)
        {
            snprintf(out, out_cap, "%.63s", CROSSTALK_CORE_STREAM_PRESENCE_TEXT);
            return;
        }

        snprintf(out, out_cap, CROSSTALK_CORE_WINDOW_PRESENCE_FMT, CROSSTALK_CORE_WINDOW_NAME_TERM);
        return;
    }

    if (focus_id == (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_WINDOW_ID)
    {
        if (ctx->eda.memview.watch_mode != (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WATCH_OFF || ctx->eda.memview.finding == LDG_TRUTH_TRUE || ctx->eda.memview.brk_on == LDG_TRUTH_TRUE)
        {
            snprintf(out, out_cap, "%.63s", CROSSTALK_CORE_HUNT_PRESENCE_TEXT);
            return;
        }

        snprintf(out, out_cap, CROSSTALK_CORE_WINDOW_PRESENCE_FMT, CROSSTALK_CORE_WINDOW_NAME_MEMVIEW);
        return;
    }

    name = crosstalk_presence_window_name_get(focus_id);
    if (name != 0x0) { snprintf(out, out_cap, CROSSTALK_CORE_WINDOW_PRESENCE_FMT, name); }
}

static void crosstalk_discord_presence_sync(crosstalk_ctx_t *ctx)
{
    ldg_net_lobby_state_t lobby_state = crosstalk_net_steam_lobby_state_get(&ctx->steam_net);
    uint64_t lobby_id = crosstalk_net_steam_lobby_id_get(&ctx->steam_net);
    uint32_t members = crosstalk_net_steam_lobby_member_cunt_get(&ctx->steam_net);
    uint8_t in_lobby = (lobby_state == LDG_NET_LOBBY_STATE_HOSTING || lobby_state == LDG_NET_LOBBY_STATE_JOINED || lobby_state == LDG_NET_LOBBY_STATE_STARTING) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    char op_line[CROSSTALK_CORE_ACTIVITY_PRESENCE_CAP] = LDG_ARR_ZERO_INIT;
    char att_line[CROSSTALK_CORE_ACTIVITY_PRESENCE_CAP] = LDG_ARR_ZERO_INIT;
    char state_line[CROSSTALK_CORE_STATE_PRESENCE_CAP] = LDG_ARR_ZERO_INIT;

    crosstalk_presence_op_get(ctx, op_line, (uint64_t)sizeof(op_line));
    crosstalk_presence_attention_get(ctx, att_line, (uint64_t)sizeof(att_line));

    if (op_line[0] != LDG_STR_TERM && att_line[0] != LDG_STR_TERM) { snprintf(state_line, sizeof(state_line), CROSSTALK_CORE_PRESENCE_JOIN_FMT, op_line, att_line); }
    else if (op_line[0] != LDG_STR_TERM) { snprintf(state_line, sizeof(state_line), "%.95s", op_line); }
    else if (att_line[0] != LDG_STR_TERM) { snprintf(state_line, sizeof(state_line), "%.95s", att_line); }

    crosstalk_net_discord_presence_activity_set(&ctx->discord, state_line[0] != LDG_STR_TERM ? state_line : 0x0);

    {
        char now_playing[CROSSTALK_CORE_MUSIC_PRESENCE_CAP] = LDG_ARR_ZERO_INIT;

        if (ctx->game.settings.music_presence_on != LDG_TRUTH_FALSE && crosstalk_game_music_now_playing_get(&ctx->music, now_playing, (uint32_t)sizeof(now_playing)) == LDG_ERR_AOK)
        {
            char music_line[CROSSTALK_CORE_MUSIC_PRESENCE_CAP] = LDG_ARR_ZERO_INIT;

            snprintf(music_line, sizeof(music_line), "%.7s%.103s", CROSSTALK_NET_DISCORD_MUSIC_PREFIX, now_playing);
            crosstalk_net_discord_presence_music_set(&ctx->discord, music_line);
        }
        else { crosstalk_net_discord_presence_music_set(&ctx->discord, 0x0); }
    }

    crosstalk_net_discord_presence_paused_set(&ctx->discord, sim_paused_is(ctx));

    if (in_lobby == LDG_TRUTH_TRUE && (ctx->game.screen != CROSSTALK_GAME_SCREEN_EDITOR || ctx->mp_busy != LDG_TRUTH_FALSE))
    {
        if (op_line[0] != LDG_STR_TERM || att_line[0] != LDG_STR_TERM) { crosstalk_net_presence_lines_set(op_line, att_line, LDG_TRUTH_TRUE); }
        else if (sim_paused_is(ctx) == LDG_TRUTH_TRUE) { crosstalk_net_presence_paused_set(LDG_TRUTH_TRUE); }
        else { crosstalk_net_presence_lobby_set(members, ctx->mp_players_max); }

        crosstalk_net_discord_presence_lobby_set(&ctx->discord, members, ctx->mp_players_max, lobby_id);
        crosstalk_net_discord_lobby_join(&ctx->discord, lobby_id);
        return;
    }

    if (in_lobby == LDG_TRUTH_TRUE) { crosstalk_net_discord_lobby_join(&ctx->discord, lobby_id); }
    else { crosstalk_net_discord_lobby_leave(&ctx->discord); }

    if (ctx->game.screen == CROSSTALK_GAME_SCREEN_EDITOR)
    {
        if (op_line[0] != LDG_STR_TERM || att_line[0] != LDG_STR_TERM) { crosstalk_net_presence_lines_set(op_line, att_line, LDG_TRUTH_FALSE); }
        else if (sim_paused_is(ctx) == LDG_TRUTH_TRUE) { crosstalk_net_presence_paused_set(LDG_TRUTH_FALSE); }
        else { crosstalk_net_presence_sandbox_set(members); }

        crosstalk_net_discord_presence_sandbox_set(&ctx->discord, members, CROSSTALK_PRESENCE_BOARD_NONE, CROSSTALK_PRESENCE_BOARD_NONE);
        return;
    }

    if (ctx->game.screen == CROSSTALK_GAME_SCREEN_BOARD_SEL || ctx->game.screen == CROSSTALK_GAME_SCREEN_LD)
    {
        crosstalk_net_presence_ld_set();
        crosstalk_net_discord_presence_ld_set(&ctx->discord);
        return;
    }

    if (ctx->game.screen == CROSSTALK_GAME_SCREEN_MP)
    {
        crosstalk_net_presence_browse_set();
        crosstalk_net_discord_presence_browse_set(&ctx->discord);
        return;
    }

    if (ctx->game.screen == CROSSTALK_GAME_SCREEN_NEWDESIGN)
    {
        crosstalk_net_presence_menu_set(CROSSTALK_NET_PRESENCE_MENU_NEW);
        crosstalk_net_discord_presence_menu_set(&ctx->discord, CROSSTALK_NET_DISCORD_TEXT_MENU_NEW);
        return;
    }

    if (ctx->game.screen == CROSSTALK_GAME_SCREEN_NEWPROJECT)
    {
        const char *presence_kind = CROSSTALK_NET_PRESENCE_PROJ_RTL;
        const char *discord_kind = CROSSTALK_NET_DISCORD_TEXT_PROJ_RTL;

        if (ctx->newproject_kind == (uint8_t)CROSSTALK_CORE_NEWPROJECT_KIND_REVERSE)
        {
            presence_kind = CROSSTALK_NET_PRESENCE_PROJ_REVERSE;
            discord_kind = CROSSTALK_NET_DISCORD_TEXT_PROJ_REVERSE;
        }

        crosstalk_net_presence_newproj_set(presence_kind);
        crosstalk_net_discord_presence_newproj_set(&ctx->discord, discord_kind);
        return;
    }

    if (ctx->game.screen == CROSSTALK_GAME_SCREEN_EXTRAS)
    {
        crosstalk_net_presence_extras_set();
        crosstalk_net_discord_presence_extras_set(&ctx->discord);
        return;
    }

    if (ctx->game.screen == CROSSTALK_GAME_SCREEN_CREDITS)
    {
        crosstalk_net_presence_credits_set();
        crosstalk_net_discord_presence_credits_set(&ctx->discord);
        return;
    }

    if (ctx->settings_open != LDG_TRUTH_FALSE)
    {
        crosstalk_net_presence_menu_set(CROSSTALK_NET_PRESENCE_MENU_SETTINGS);
        crosstalk_net_discord_presence_menu_set(&ctx->discord, CROSSTALK_NET_DISCORD_TEXT_MENU_SETTINGS);
        return;
    }

    crosstalk_net_presence_menu_set(CROSSTALK_NET_PRESENCE_MENU_MAIN);
    crosstalk_net_discord_presence_menu_set(&ctx->discord, CROSSTALK_NET_DISCORD_TEXT_MENU_MAIN);
}

static void crosstalk_tick(ldg_sched_event_desc_t desc, void *step, uint64_t step_size, void *user_data)
{
    crosstalk_ctx_t *ctx = (crosstalk_ctx_t *)user_data;
    const ldg_sched_step_ctx_t *step_ctx = (const ldg_sched_step_ctx_t *)step;
    ldg_term_render_t *render = 0x0;
    ldg_term_rect_t screen_rect = LDG_STRUCT_ZERO_INIT;
    ldg_term_rect_t desk_rect = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;
    char status[128] = LDG_ARR_ZERO_INIT;

    if (LDG_UNLIKELY(ctx == 0x0)) { return; }

    if (desc.idx == ctx->fixed_event.idx && desc.generation == ctx->fixed_event.generation)
    {
        if (ctx->steam_live == LDG_TRUTH_TRUE)
        {
            crosstalk_net_steam_poll(&ctx->steam_net, ctx->tick_period > 0.0 ? ctx->tick_period / 1000.0 : (1.0 / 60.0));
            crosstalk_net_inventory_poll(&ctx->inventory);
            crosstalk_net_wshop_poll(&ctx->wshop);
        }

        crosstalk_wshop_upload_watch(ctx);
        crosstalk_friend_started_tick(ctx);
        crosstalk_focus_pause_tick(ctx);
        crosstalk_idle_tick(ctx);
        crosstalk_peer_state_send(ctx);

        if (ctx->join_lobby_pending != 0)
        {
            uint64_t join_lobby = ctx->join_lobby_pending;

            ctx->join_lobby_pending = 0;
            ret = lobby_join_enter(ctx, join_lobby);
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "crosstalk: launch join; lobby: %u64; err: %u32", join_lobby, ret);
        }

        if (ctx->discord_live == LDG_TRUTH_TRUE)
        {
            uint64_t discord_join_lobby = 0;

            crosstalk_net_discord_poll(&ctx->discord, ctx->tick_period > 0.0 ? ctx->tick_period / 1000.0 : (1.0 / 60.0));

            crosstalk_discord_auth_sync(ctx);

            if (crosstalk_net_discord_auth_fail_take(&ctx->discord) == LDG_TRUTH_TRUE)
            {
                ctx->game.settings.discord_link_on = LDG_TRUTH_FALSE;

                toast_push(ctx, CROSSTALK_CORE_DISCORD_LINK_FAIL_TEXT);
            }

            crosstalk_discord_presence_sync(ctx);

            if (crosstalk_net_discord_join_secret_get(&ctx->discord, &discord_join_lobby) == LDG_ERR_AOK)
            {
                ret = lobby_join_enter(ctx, discord_join_lobby);
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "discord: join accepted; lobby: %u64; err: %u32", discord_join_lobby, ret);
            }

            {
                const char *invite_name = 0x0;
                uint64_t invite_sender = 0;

                if (crosstalk_net_discord_invite_pending_get(&ctx->discord, &invite_name, &invite_sender) == LDG_ERR_AOK && invite_sender != ctx->discord_invite_prev)
                {
                    char invite_toast[CROSSTALK_NET_NOTICE_CAP] = LDG_ARR_ZERO_INIT;

                    ctx->discord_invite_prev = invite_sender;
                    snprintf(invite_toast, sizeof(invite_toast), "%.63s invited you on Discord; F4 to join", invite_name[0] != LDG_STR_TERM ? invite_name : "someone");
                    toast_push(ctx, invite_toast);
                }

                if (ctx->key_invite == LDG_TRUTH_TRUE)
                {
                    ctx->key_invite = LDG_TRUTH_FALSE;

                    ret = crosstalk_net_discord_invite_accept(&ctx->discord);
                    if (ret != LDG_ERR_EMPTY) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "discord: invite accept; err: %u32", ret); }
                }
            }

            {
                const char *msg_name = 0x0;
                const char *msg_text = 0x0;
                const char *msg_peer_name = 0x0;
                uint64_t msg_author = 0;
                uint64_t msg_peer_id = 0;
                uint8_t msg_dm = LDG_TRUTH_FALSE;
                uint8_t msg_hist = LDG_TRUTH_FALSE;

                while (crosstalk_net_discord_chat_get(&ctx->discord, &msg_name, &msg_text, &msg_peer_name, &msg_author, &msg_peer_id, &msg_dm, &msg_hist) == LDG_ERR_AOK)
                {
                    uint64_t msg_peer = (msg_dm == LDG_TRUTH_TRUE) ? msg_peer_id : CROSSTALK_GAME_CHAT_PEER_NONE;
                    const char *conv_name = CROSSTALK_CORE_CHAT_LOBBY_NAME;

                    if (msg_dm == LDG_TRUTH_TRUE) { conv_name = (msg_peer_name[0] != LDG_STR_TERM) ? msg_peer_name : 0x0; }

                    ret = crosstalk_game_chat_push(&ctx->chat, (uint8_t)CROSSTALK_GAME_CHAT_GROUP_DISCORD, msg_peer, conv_name, msg_name, msg_text);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "discord chat push failed"); }

                    if (msg_hist == LDG_TRUTH_FALSE)
                    {
                        char notice[CROSSTALK_NET_NOTICE_CAP] = LDG_ARR_ZERO_INIT;

                        snprintf(notice, sizeof(notice), "%.63s: %.63s", msg_name[0] != LDG_STR_TERM ? msg_name : "someone", msg_text);
                        crosstalk_notice_raise(ctx, CROSSTALK_CORE_NOTIFY_TITLE_DISCORD, notice);
                    }
                }
            }
        }

        {
            uint64_t req_lobby = 0;

            if (crosstalk_net_steam_join_req_get(&ctx->steam_net, &req_lobby) == LDG_ERR_AOK)
            {
                ret = lobby_join_enter(ctx, req_lobby);
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "crosstalk: steam join request; lobby: %u64; friend: %u64; err: %u32", req_lobby, ldg_net_steamworks_join_friend_steam_id_get(&ctx->steam_net.join), ret);
            }
        }

        {
            ldg_net_lobby_state_t conn_state = crosstalk_net_steam_lobby_state_get(&ctx->steam_net);
            uint64_t conn_lobby = 0;

            if (conn_state == LDG_NET_LOBBY_STATE_HOSTING || conn_state == LDG_NET_LOBBY_STATE_JOINED) { conn_lobby = crosstalk_net_steam_lobby_id_get(&ctx->steam_net); }

            if (conn_lobby != ctx->connect_lobby_prev)
            {
                ctx->connect_lobby_prev = conn_lobby;
                crosstalk_net_presence_connect_set(conn_lobby);
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: connect presence; lobby: %u64", conn_lobby);
            }
        }

        {
            ldg_net_lobby_state_t pres_state = crosstalk_net_steam_lobby_state_get(&ctx->steam_net);
            uint32_t pres_players = crosstalk_net_steam_lobby_member_cunt_get(&ctx->steam_net);
            uint64_t pres_group = CROSSTALK_NET_PRESENCE_GROUP_CLEAR;
            uint8_t pres_kind = CROSSTALK_NET_PRESENCE_KIND_MENU;

            if (ctx->game.screen == CROSSTALK_GAME_SCREEN_EDITOR) { pres_kind = CROSSTALK_NET_PRESENCE_KIND_SANDBOX; }
            else if (pres_state == LDG_NET_LOBBY_STATE_HOSTING || pres_state == LDG_NET_LOBBY_STATE_JOINED || pres_state == LDG_NET_LOBBY_STATE_STARTING) { pres_kind = CROSSTALK_NET_PRESENCE_KIND_LOBBY; }

            if (pres_kind != CROSSTALK_NET_PRESENCE_KIND_MENU) { pres_group = crosstalk_net_steam_lobby_id_get(&ctx->steam_net); }

            if (pres_kind != ctx->presence_kind || pres_players != ctx->presence_players || pres_group != ctx->presence_group)
            {
                ctx->presence_kind = pres_kind;
                ctx->presence_players = pres_players;
                ctx->presence_group = pres_group;

                crosstalk_net_presence_group_set(pres_group, pres_players);

                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: presence group; kind: %u32; players: %u32; group: %u64", (uint32_t)pres_kind, pres_players, pres_group);
            }
        }
        if (ctx->game.net_join_phase == CROSSTALK_NET_JOIN_PHASE_CONN && crosstalk_net_steam_lan_sesh_get(&ctx->steam_net) == (uint8_t)CROSSTALK_NET_LAN_SESH_NONE)
        {
            uint64_t host_id = crosstalk_net_steam_lobby_host_steam_id_get(&ctx->steam_net);
            if (host_id != 0)
            {
                ret = crosstalk_net_steam_client_join(&ctx->steam_net, host_id, CROSSTALK_NET_VIRTUAL_PORT);
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: net client join; err: %u32; host: %u64; local: %u64", ret, host_id, ldg_net_steamworks_local_steam_id_get());
                ctx->game.net_join_phase = CROSSTALK_NET_JOIN_PHASE_IDLE;
            }
        }

        for (;;)
        {
            const char *friend_text = 0x0;
            char friend_name[CROSSTALK_GAME_CHAT_NAME_CAP] = LDG_ARR_ZERO_INIT;
            uint64_t friend_id = 0;
            uint32_t friend_len = 0;

            ret = crosstalk_net_steam_friend_msg_get(&ctx->steam_net, &friend_id, &friend_text, &friend_len);
            if (ret != LDG_ERR_AOK || friend_text == 0x0) { break; }

            if (crosstalk_persona_resolve(ctx, friend_id, friend_name, (uint64_t)sizeof(friend_name)) != LDG_ERR_AOK) { friend_name[0] = LDG_STR_TERM; }

            if (friend_name[0] == LDG_STR_TERM)
            {
                uint8_t info_pending = 0;

                ret = ldg_net_steamworks_user_info_req(friend_id, LDG_TRUTH_TRUE, &info_pending);
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "steam chat: persona miss; id: %u64; info req err: %u32; pending: %u32", friend_id, ret, (uint32_t)info_pending);
            }

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "steam chat: from: %u64; name: %.63s; len: %u32; text: %.2047s", friend_id, friend_name[0] != LDG_STR_TERM ? friend_name : "?", friend_len, friend_text);

            if (friend_name[0] == LDG_STR_TERM) { snprintf(friend_name, sizeof(friend_name), "%.63s", CROSSTALK_CORE_PERSONA_UNKNOWN); }

            ret = crosstalk_game_chat_push(&ctx->chat, (uint8_t)CROSSTALK_GAME_CHAT_GROUP_STEAM, friend_id, friend_name, friend_name, friend_text);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "steam chat push failed"); }

            {
                char notice[CROSSTALK_NET_NOTICE_CAP] = LDG_ARR_ZERO_INIT;

                snprintf(notice, sizeof(notice), "%.63s: %.63s", friend_name, friend_text);
                crosstalk_notice_raise(ctx, CROSSTALK_CORE_NOTIFY_TITLE_STEAM, notice);
            }
        }

        {
            ldg_net_lobby_state_t lobby_now = crosstalk_net_steam_lobby_state_get(&ctx->steam_net);
            char member_toast[LDG_NET_LOBBY_NAME_MAX + CROSSTALK_CORE_LOBBY_TOAST_PAD] = LDG_ARR_ZERO_INIT;
            uint64_t self_id = ldg_net_steamworks_local_steam_id_get();
            uint32_t member_cunt = 0;
            uint32_t idx = 0;
            uint32_t jdx = 0;
            uint8_t found = LDG_TRUTH_FALSE;

            if (lobby_now == LDG_NET_LOBBY_STATE_HOSTING || lobby_now == LDG_NET_LOBBY_STATE_JOINED || lobby_now == LDG_NET_LOBBY_STATE_STARTING)
            {
                member_cunt = crosstalk_net_steam_lobby_member_cunt_get(&ctx->steam_net);
                if (ctx->lobby_member_prev_cunt > 0)
                {
                    for (idx = 0; idx < member_cunt; idx = idx + 1)
                    {
                        const ldg_net_lobby_member_t *member_now = crosstalk_net_steam_lobby_member_get(&ctx->steam_net, idx);

                        if (member_now == 0x0 || member_now->user_id == self_id) { continue; }

                        found = LDG_TRUTH_FALSE;
                        for (jdx = 0; jdx < ctx->lobby_member_prev_cunt; jdx = jdx + 1) { if (ctx->lobby_member_prev[jdx].user_id == member_now->user_id) { found = LDG_TRUTH_TRUE; break; } }

                        if (found == LDG_TRUTH_FALSE)
                        {
                            snprintf(member_toast, sizeof(member_toast), "%.63s joined the lobby", member_now->name);
                            toast_push(ctx, member_toast);
                        }
                    }

                    for (idx = 0; idx < ctx->lobby_member_prev_cunt; idx = idx + 1)
                    {
                        if (ctx->lobby_member_prev[idx].user_id == self_id) { continue; }

                        found = LDG_TRUTH_FALSE;
                        for (jdx = 0; jdx < member_cunt; jdx = jdx + 1)
                        {
                            const ldg_net_lobby_member_t *member_now = crosstalk_net_steam_lobby_member_get(&ctx->steam_net, jdx);

                            if (member_now != 0x0 && member_now->user_id == ctx->lobby_member_prev[idx].user_id) { found = LDG_TRUTH_TRUE; break; }
                        }

                        if (found == LDG_TRUTH_FALSE)
                        {
                            snprintf(member_toast, sizeof(member_toast), "%.63s left the lobby", ctx->lobby_member_prev[idx].name);
                            toast_push(ctx, member_toast);
                        }
                    }
                }

                ctx->lobby_member_prev_cunt = 0;
                for (idx = 0; idx < member_cunt && ctx->lobby_member_prev_cunt < LDG_NET_LOBBY_MEMBER_MAX; idx = idx + 1)
                {
                    const ldg_net_lobby_member_t *member_now = crosstalk_net_steam_lobby_member_get(&ctx->steam_net, idx);

                    if (member_now == 0x0) { continue; }

                    ctx->lobby_member_prev[ctx->lobby_member_prev_cunt] = *member_now;
                    ctx->lobby_member_prev_cunt++;
                }
            }
            else
            {
                if (ctx->lobby_member_prev_cunt > 0)
                {
                    ret = crosstalk_game_chat_rst(&ctx->chat);
                    ret = crosstalk_game_chat_open_set(&ctx->chat, LDG_TRUTH_FALSE);
                }

                ctx->lobby_member_prev_cunt = 0;
            }
        }

        if (ctx->remote_sesh != 0 && crosstalk_net_steam_status_get(&ctx->steam_net) != LDG_NET_CONN_STATUS_CONN)
        {
            crosstalk_sesh_leave(ctx);
            ctx->game.screen = CROSSTALK_GAME_SCREEN_MENU;
            crosstalk_net_steam_notice_push(&ctx->steam_net, "left host board");
        }

        if (ctx->snap_req_sent == 0 && ctx->net.net.mode != LDG_NET_MODE_SRV && ctx->steam_net.sesh_live != 0 && ctx->mp_rdy != LDG_TRUTH_FALSE && ctx->mp_busy == LDG_TRUTH_FALSE && crosstalk_net_steam_status_get(&ctx->steam_net) == LDG_NET_CONN_STATUS_CONN)
        {
            ret = crosstalk_net_wire_send(&ctx->steam_net, CROSSTALK_NET_WIRE_KIND_SNAPSHOT_REQ, 0x0, CROSSTALK_NET_WIRE_NO_PAYLOAD, CROSSTALK_NET_WIRE_RELIABLE);
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: snapshot requested; err: %u32", ret);
            ctx->snap_req_sent = LDG_TRUTH_TRUE;
        }

        return;
    }

    if (desc.idx != ctx->step_event.idx || desc.generation != ctx->step_event.generation) { return; }

    if (LDG_UNLIKELY(step_size < (uint64_t)sizeof(ldg_sched_step_ctx_t) || step_ctx == 0x0)) { return; }

    if (ctx->timeline_set == 0 || ctx->timeline_screen_prev != (uint8_t)ctx->game.screen)
    {
        ldg_net_steamworks_timeline_mode_t tl_mode = LDG_NET_STEAMWORKS_TIMELINE_MENUS;

        if (ctx->game.screen == CROSSTALK_GAME_SCREEN_EDITOR) { tl_mode = LDG_NET_STEAMWORKS_TIMELINE_PLAYING; }
        else if (ctx->game.screen == CROSSTALK_GAME_SCREEN_LD) { tl_mode = LDG_NET_STEAMWORKS_TIMELINE_LD; }
        else if (ctx->game.screen == CROSSTALK_GAME_SCREEN_NEWDESIGN) { tl_mode = LDG_NET_STEAMWORKS_TIMELINE_STAGING; }

        ret = ldg_net_steamworks_timeline_mode_set(tl_mode);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "crosstalk: timeline mode set; err: %u32; mode: %u32", ret, (uint32_t)tl_mode); }

        ctx->timeline_screen_prev = (uint8_t)ctx->game.screen;
        ctx->timeline_set = LDG_TRUTH_TRUE;
    }

    {
        uint64_t tick_now = ldg_time_epoch_ns_get();

        if (ctx->tick_prev != 0)
        {
            ctx->tick_period = (double)(tick_now - ctx->tick_prev) / (double)LDG_NS_PER_MS;
            ctx->perf_delta_sum = ctx->perf_delta_sum + ctx->tick_period;
            ctx->perf_window_elapsed = ctx->perf_window_elapsed + ctx->tick_period;
            ctx->perf_delta_cunt = ctx->perf_delta_cunt + 1;
            if (ctx->perf_window_elapsed >= CROSSTALK_PERF_WINDOW)
            {
                if (ctx->perf_delta_cunt != 0) { ctx->perf_delta_average = ctx->perf_delta_sum / (double)ctx->perf_delta_cunt; }

                ctx->perf_delta_sum = 0.0;
                ctx->perf_window_elapsed = 0.0;
                ctx->perf_delta_cunt = 0;
            }
        }

        ctx->tick_prev = tick_now;
    }

    ctx->dt = step_ctx->dt;
    ctx->time = step_ctx->time;

    if (ctx->extras_playing != LDG_TRUTH_FALSE)
    {
        ldg_audio_play_status_t play_st = LDG_STRUCT_ZERO_INIT;
        uint32_t rate = 0;
        uint16_t channel_cunt = 0;
        uint16_t bit_depth = 0;

        if (ldg_audio_play_status_get(&play_st) == LDG_ERR_AOK && ldg_audio_play_fmt_get(&rate, &channel_cunt, &bit_depth) == LDG_ERR_AOK && rate > 0) { if (play_st.state == LDG_AUDIO_PLAY_STATE_IDLE) { ctx->extras_playing = LDG_TRUTH_FALSE; }
            else { ctx->caption_clock = (double)play_st.frame_pos / (double)rate; } }
    }

    {
        if (ctx->rtl_restore_pending == LDG_TRUTH_TRUE && ctx->game.ld_active == 0)
        {
            ctx->rtl_restore_pending = LDG_TRUTH_FALSE;

            ret = crosstalk_term_sect_restore(ctx);
            if (ret != LDG_ERR_AOK && ret != LDG_ERR_NOT_FOUND) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: term restore failed; err: %u32", ret); }

            ret = crosstalk_rtl_sect_restore(ctx);
            if (ret != LDG_ERR_AOK && ret != LDG_ERR_NOT_FOUND) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: rtl restore failed; err: %u32", ret); }

            ret = crosstalk_crt_sect_restore(ctx);
            if (ret != LDG_ERR_AOK && ret != LDG_ERR_NOT_FOUND) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: crt restore failed; err: %u32", ret); }

            ret = crosstalk_script_sect_restore(ctx);
            if (ret != LDG_ERR_AOK && ret != LDG_ERR_NOT_FOUND) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: script resume failed; err: %u32", ret); }

            crosstalk_dlc_disks_attach(ctx);
            crosstalk_event_fire(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_EVENT_LD);
        }

        {
            uint32_t guide_pick = (uint32_t)UINT32_MAX;

            if (crosstalk_ui_guide_pick_get(&ctx->guide, &guide_pick) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "guide pick get failed"); }

            if (guide_pick == (uint32_t)CROSSTALK_CORE_GUIDE_PICK_DEMO) { ctx->demo_pending = LDG_TRUTH_TRUE; }
            else if (guide_pick == (uint32_t)CROSSTALK_CORE_GUIDE_PICK_EMU) { ctx->emu_default_pending = LDG_TRUTH_TRUE; }
        }

        {
            uint8_t demo_copied = LDG_TRUTH_FALSE;

            ret = crosstalk_core_watch_tick(&ctx->demo_watch, ldg_time_epoch_ms_get(), &demo_copied);
            if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "demo watch: tick failed; err: %u32", ret); }

            if (demo_copied == LDG_TRUTH_TRUE)
            {
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "demo watch: copy kept; path: %.255s", ctx->demo_watch.dst);
                toast_push(ctx, CROSSTALK_CORE_DEMO_KEEP_TEXT);
            }
        }

        if (ctx->game.ld_active == 0)
        {
            const crosstalk_eda_ic_fpga_slot_t *design_slot = &ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT];
            uint8_t design_changed = LDG_TRUTH_FALSE;

            ret = crosstalk_core_watch_set_tick(&ctx->design_watch, design_slot->fileset_dir, design_slot->fileset_files, ldg_time_epoch_ms_get(), &design_changed);
            if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "design watch: tick failed; err: %u32", ret); }

            if (design_changed == LDG_TRUTH_TRUE) { crosstalk_design_reload(ctx); }
        }

        if (ctx->emu_net.open == LDG_TRUTH_TRUE)
        {
            const crosstalk_hdl_abi_t *net_abi = 0x0;

            if (crosstalk_eda_ic_fpga_emu_live_is(&ctx->eda.fpga_emu) != LDG_TRUTH_TRUE || crosstalk_eda_ic_fpga_emu_abi_get(&ctx->eda.fpga_emu, &net_abi) != LDG_ERR_AOK) { ret = crosstalk_emu_net_shutdown(&ctx->emu_net); }
            else { ret = crosstalk_emu_net_tick(&ctx->emu_net, net_abi); }

            if (ret != LDG_ERR_AOK)
            {
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu net: tick failed, endpoint closed; err: %u32", ret);

                if (crosstalk_emu_net_shutdown(&ctx->emu_net) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu net shutdown failed"); }
            }
        }

        if (ctx->demo_pending == LDG_TRUTH_TRUE && ctx->game.ld_active == 0)
        {
            char demo_dir[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
            char demo_script[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
            char demo_sv[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
            char demo_keep[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;

            ctx->demo_pending = LDG_TRUTH_FALSE;

            ret = ldg_io_path_join(ctx->res_dir, CROSSTALK_CORE_DEMO_DIR_REL, demo_dir, (uint64_t)sizeof(demo_dir));
            if (ret == LDG_ERR_AOK) { ret = ldg_io_path_join(demo_dir, CROSSTALK_CORE_DEMO_SCRIPT_NAME, demo_script, (uint64_t)sizeof(demo_script)); }

            if (ret == LDG_ERR_AOK) { ret = crosstalk_eda_script_file_exec(&ctx->eda.script, demo_script); }

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "crosstalk: demo script; err: %u32", ret);

            if (ret == LDG_ERR_AOK)
            {
                uint8_t exec_was = ctx->console_exec_is;

                ctx->console_exec_is = LDG_TRUTH_TRUE;
                crosstalk_term_host_open(ctx, demo_dir, CROSSTALK_CORE_DEMO_SV_NAME);
                ctx->console_exec_is = exec_was;

                ret = ldg_io_path_join(demo_dir, CROSSTALK_CORE_DEMO_SV_NAME, demo_sv, (uint64_t)sizeof(demo_sv));
                if (ret == LDG_ERR_AOK) { ret = ldg_io_path_join(demo_dir, CROSSTALK_CORE_DEMO_KEEP_NAME, demo_keep, (uint64_t)sizeof(demo_keep)); }

                if (ret == LDG_ERR_AOK) { ret = crosstalk_core_watch_arm(&ctx->demo_watch, demo_sv, demo_keep); }

                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "demo watch: arm; err: %u32", ret);
            }
        }

        if (ctx->emu_default_pending == LDG_TRUTH_TRUE && ctx->game.ld_active == 0)
        {
            char newrtl_script[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;

            ctx->emu_default_pending = LDG_TRUTH_FALSE;

            ret = ldg_io_path_join(ctx->res_dir, CROSSTALK_CORE_NEWRTL_SCRIPT_REL, newrtl_script, (uint64_t)sizeof(newrtl_script));
            if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "new rtl project script path join failed"); }
            else
            {
                ret = crosstalk_eda_script_file_exec(&ctx->eda.script, newrtl_script);
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "crosstalk: new rtl project script; err: %u32", ret);
            }

            crosstalk_dlc_disks_attach(ctx);
        }

        if (ctx->reverse_setup_pending == LDG_TRUTH_TRUE && ctx->game.ld_active == 0)
        {
            uint32_t proj_idx = UINT32_MAX;
            uint32_t img_hash = 0;

            ctx->reverse_setup_pending = LDG_TRUTH_FALSE;

            ret = crosstalk_reng_proj_find(&ctx->reng, ctx->board_name, &proj_idx);
            if (ret != LDG_ERR_AOK)
            {
                ret = crosstalk_reng_proj_add(&ctx->reng, ctx->board_name, (uint32_t)CROSSTALK_RENG_KIND_REVERSE, ctx->save_path, img_hash, &proj_idx);
                if (ret == LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: reverse project created; name: %.63s; idx: %u32", ctx->board_name, proj_idx); }
                else { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: reverse project create failed; name: %.63s; err: %u32", ctx->board_name, ret); }
            }

            if (ret == LDG_ERR_AOK)
            {
                ret = crosstalk_reng_sel_set(&ctx->reng, proj_idx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: reverse project select failed; idx: %u32; err: %u32", proj_idx, ret); }
            }
        }

        if (ctx->eda.logic_probe.live_cunt != 0x0)
        {
            ret = crosstalk_eda_logic_stream_pump(&ctx->eda.logic_stream, &ctx->eda.logic_probe);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "logic stream pump failed"); }
        }

        crosstalk_serial_tick(ctx);

        crosstalk_pty_tick(ctx);

        crosstalk_receptacle_tick(ctx);

        crosstalk_watch_tick(ctx);

        crosstalk_fetch_tick(ctx);

        crosstalk_dsu_tick(ctx);

        crosstalk_prof_tick(ctx);

        ret = crosstalk_eda_io_midi_tick(&ctx->eda.midi, ctx->dt);
        if (ret != LDG_ERR_AOK && ret != CROSSTALK_EDA_IO_MIDI_ERR_BUSTED) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "midi tick failed"); }

        ret = crosstalk_eda_io_midi_pump(&ctx->eda.midi);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "midi pump failed"); }

        if (ctx->eda.fpga_build.ok == LDG_TRUTH_TRUE)
        {
            uint32_t build_phase = 0;

            ret = crosstalk_eda_ic_fpga_build_tick(&ctx->eda.fpga_build, &build_phase);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "build tick failed"); }

            if (build_phase == (uint32_t)CROSSTALK_EDA_IC_FPGA_BUILD_PHASE_CC) { toast_push(ctx, CROSSTALK_CORE_PACK_CC_TEXT); }

            if (build_phase == (uint32_t)CROSSTALK_EDA_IC_FPGA_BUILD_PHASE_LD) { toast_push(ctx, CROSSTALK_CORE_PACK_LD_TEXT); }

            if (build_phase == (uint32_t)CROSSTALK_EDA_IC_FPGA_BUILD_PHASE_BIND) { toast_push(ctx, CROSSTALK_CORE_PACK_BIND_TEXT); }

            if (build_phase == (uint32_t)CROSSTALK_EDA_IC_FPGA_BUILD_PHASE_READY)
            {
                if (ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT].prog_gen != ctx->pack_prog_gen) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pack: stale build discarded; the design was reprogrammed during the bake; run emit again"); }
                else
                {
                    uint8_t was_live = crosstalk_fpga_hlt(ctx);

                    ret = crosstalk_eda_ic_fpga_abi_set(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, &ctx->eda.fpga_build.abi);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: compiled component refused; err: %u32", ret); }

                    if (ret == LDG_ERR_AOK) { crosstalk_fpga_in_ports_carry(ctx); }

                    crosstalk_fpga_resume(ctx, ret == LDG_ERR_AOK ? LDG_TRUTH_TRUE : was_live);

                    crosstalk_notice_raise(ctx, CROSSTALK_CORE_NOTIFY_TITLE_SESH, ret == LDG_ERR_AOK ? CROSSTALK_CORE_PACK_READY_TEXT : CROSSTALK_CORE_PACK_FAIL_TEXT);
                }
            }

            if (build_phase == (uint32_t)CROSSTALK_EDA_IC_FPGA_BUILD_PHASE_FAIL) { crosstalk_notice_raise(ctx, CROSSTALK_CORE_NOTIFY_TITLE_SESH, CROSSTALK_CORE_PACK_FAIL_TEXT); }
        }

        if (ctx->re.ok == LDG_TRUTH_TRUE)
        {
            uint8_t cc_done = LDG_TRUTH_FALSE;

            if (crosstalk_re_cc_tick(&ctx->re.cc, &cc_done) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re cc tick failed"); }

            if (cc_done == LDG_TRUTH_TRUE) { crosstalk_notice_raise(ctx, CROSSTALK_CORE_NOTIFY_TITLE_SESH, ctx->re.cc.err == LDG_ERR_AOK ? CROSSTALK_CORE_RE_BUILD_OK_TEXT : CROSSTALK_CORE_RE_BUILD_FAIL_TEXT); }

            if (ctx->re_diff_pending != 0)
            {
                ctx->re_diff_pending = ctx->re_diff_pending - 0x1;

                if (ctx->re_diff_pending == 0) { crosstalk_re_diff_run(ctx); }
            }

            if (ctx->re.db_gen_seen != ctx->re.db.gen)
            {
                ctx->re.db_gen_seen = ctx->re.db.gen;
                crosstalk_re_progress_push(ctx);
            }
        }

        crosstalk_instability_feed(ctx);

        ret = crosstalk_eda_sound_tick(&ctx->eda.sound, ctx->dt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sound tick failed"); }

        if (crosstalk_eda_ui_scope_open_is(&ctx->eda.scope) == LDG_TRUTH_TRUE)
        {
            uint64_t scope_ns = 0;

            if (crosstalk_eda_clk_sim_ns_get(&ctx->eda.clk, ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT].clk_domain, &scope_ns) == LDG_ERR_AOK)
            {
                ret = crosstalk_eda_ui_scope_capture(&ctx->eda.scope, scope_ns);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scope capture failed"); }
            }
        }
    }

    crosstalk_theme_tick(ctx);

    ret = crosstalk_game_caption_tick(&ctx->game.caption, ctx->extras_playing != LDG_TRUTH_FALSE ? ctx->caption_clock : ctx->time);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_NOT_INIT && ret != LDG_ERR_FULL)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "caption tick failed"); }

    if (ctx->game.screen == CROSSTALK_GAME_SCREEN_EDITOR && sim_paused_is(ctx) == LDG_TRUTH_FALSE && ctx->dt > 0.0) { ctx->game.bench_ms_total += (uint64_t)(ctx->dt * (double)LDG_MS_PER_SEC); }

    if (crosstalk_eda_ic_fpga_running_is(&ctx->eda.fpga) == LDG_TRUTH_TRUE && ctx->dt > 0.0) { ctx->game.sim_ms_total = ctx->game.sim_ms_total + (uint64_t)(ctx->dt * (double)LDG_MS_PER_SEC); }

    if (crosstalk_eda_ic_fpga_run_live_is(&ctx->eda.fpga_run) == LDG_TRUTH_TRUE) { if (ctx->sim_run_since <= 0.0) { ctx->sim_run_since = ctx->time; } }
    else { ctx->sim_run_since = 0.0; }

    if (ctx->game.quit != 0)
    {
        if (ctx->persist_pending != LDG_TRUTH_TRUE && ctx->quit_no_persist != LDG_TRUTH_TRUE && crosstalk_disk_persist_needed_is(ctx) == LDG_TRUTH_TRUE) { ctx->persist_pending = LDG_TRUTH_TRUE; ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: persisting machine state before exit"); }
        else
        {
            if (ctx->persist_pending == LDG_TRUTH_TRUE)
            {
                crosstalk_disk_persist(ctx);

                ctx->persist_pending = LDG_TRUTH_FALSE;
            }

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: loop stop; quit: %u32", (uint32_t)ctx->game.quit);
            ldg_sched_ctx_stop(&ctx->sched);
            return;
        }
    }

    if (crosstalk_ui_window_digit_step(&ctx->win_nav, ldg_time_epoch_ms_get()) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "window digit step failed"); }

    if (crosstalk_core_irq_drain(ctx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "key irq drain failed"); }

    if (crosstalk_win_focus_push(ctx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "window focus push failed"); }

    {
        const crosstalk_eda_ic_fpga_slot_t *ach_slot = &ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT];
        uint64_t ach_val = 0;
        uint32_t seg = 0;
        uint8_t hlt_now = crosstalk_core_hlt_is(ctx) == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

        if (crosstalk_eda_ic_fpga_emu_stat_step(&ctx->emu_stat, crosstalk_eda_ic_fpga_emu_live_is(&ctx->eda.fpga_emu), hlt_now, ldg_time_epoch_ms_get()) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu stat step failed"); }

        if (hlt_now != ctx->eda.crt.title_hlt)
        {
            ctx->eda.crt.title_hlt = hlt_now;

            if (crosstalk_ui_window_title_set(&ctx->win_nav, (uint64_t)CROSSTALK_EDA_UI_DISPLAY_CRT_WINDOW_ID, hlt_now == LDG_TRUTH_TRUE ? CROSSTALK_CORE_CRT_TITLE_HLT : CROSSTALK_CORE_CRT_TITLE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crt title sync failed"); }

            if (hlt_now == LDG_TRUTH_TRUE && ctx->eda.memview.brk_on == LDG_TRUTH_TRUE)
            {
                if (crosstalk_eda_ui_memview_hlt_mark_set(&ctx->eda.memview, ctx->eda.memview.brk_lo, ctx->eda.memview.brk_hi) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview brk mark failed"); }

                crosstalk_ach_once(ctx, CROSSTALK_GAME_PROGRESSION_FIRST_BRK_HIT);
            }
        }

        if (ach_slot->runnable == LDG_TRUTH_TRUE && ach_slot->emu != LDG_TRUTH_TRUE) { crosstalk_ach_once(ctx, CROSSTALK_GAME_PROGRESSION_FIRST_ELAB); }

        if (crosstalk_eda_ic_fpga_build_live_is(&ctx->eda.fpga_build) == LDG_TRUTH_TRUE) { crosstalk_ach_once(ctx, CROSSTALK_GAME_PROGRESSION_FIRST_EMIT); }

        if (crosstalk_eda_ic_fpga_run_live_is(&ctx->eda.fpga_run) == LDG_TRUTH_TRUE)
        {
            if (crosstalk_eda_ic_fpga_run_rate_get(&ctx->eda.fpga_run, &ach_val) == LDG_ERR_AOK && ach_val >= (uint64_t)CROSSTALK_CORE_ACH_RATE_HZ) { crosstalk_ach_once(ctx, CROSSTALK_GAME_PROGRESSION_RATE_500M); }

            if (crosstalk_eda_ic_fpga_run_cycles_get(&ctx->eda.fpga_run, &ach_val) == LDG_ERR_AOK && ach_val >= (uint64_t)CROSSTALK_CORE_ACH_CYC_CUNT) { crosstalk_ach_once(ctx, CROSSTALK_GAME_PROGRESSION_CYC_1G); }
        }

        if (ctx->eda.memview.hit_cunt != 0 && ctx->eda.memview.pattern_len != 0) { crosstalk_ach_once(ctx, CROSSTALK_GAME_PROGRESSION_FIRST_FIND_HIT); }

        if (ctx->eda.memview.io_on == LDG_TRUTH_TRUE && ctx->eda.memview.io_cunt != 0)
        {
            for (seg = 0; seg < ctx->eda.memview.io_cunt; seg = seg + 0x1) { if (ctx->eda.memview.base >= ctx->eda.memview.io_base[seg] && ctx->eda.memview.base < ctx->eda.memview.io_base[seg] + ctx->eda.memview.io_span[seg]) { ctx->ach_io_seen = ctx->ach_io_seen | ((uint32_t)0x1 << seg); } }

            if (ctx->ach_io_seen == ((uint32_t)0x1 << ctx->eda.memview.io_cunt) - (uint32_t)0x1) { crosstalk_ach_once(ctx, CROSSTALK_GAME_PROGRESSION_IO_ALL_SEEN); }
        }
    }

    if (ctx->key_esc == LDG_TRUTH_TRUE && ctx->game.screen != CROSSTALK_GAME_SCREEN_LD)
    {
        if (crosstalk_core_cmdline_open_is(&ctx->cmdline) == LDG_TRUTH_TRUE) { ret = LDG_ERR_AOK; }
        else if (ctx->settings_open != LDG_TRUTH_FALSE)
        {
            ctx->settings_open = LDG_TRUTH_FALSE;
            if (ctx->game.screen == CROSSTALK_GAME_SCREEN_EDITOR) { ret = crosstalk_core_save_wr(ctx); }
        }
        else if (ctx->game.paused != LDG_TRUTH_FALSE) { ctx->game.paused = LDG_TRUTH_FALSE; }
        else if (ctx->game.screen == CROSSTALK_GAME_SCREEN_MENU) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "crosstalk: escape ignored on menu"); }
        else if (ctx->game.screen == CROSSTALK_GAME_SCREEN_EDITOR) { if (ctx->key_esc_shift == LDG_TRUTH_TRUE)
            {
                ctx->game.paused = LDG_TRUTH_TRUE;
                ctx->menu_sel = (uint32_t)CROSSTALK_MENU_SEL_FIRST;
                ctx->menu_confirm = LDG_TRUTH_FALSE;
            }
        }
        else if (ctx->game.screen == CROSSTALK_GAME_SCREEN_EDITORPICK) { ctx->game.screen = CROSSTALK_GAME_SCREEN_BOARD_SEL; }
        else if (ctx->game.screen == CROSSTALK_GAME_SCREEN_MP) { crosstalk_menu_quit(ctx); }
        else
        {
            ctx->game.screen = CROSSTALK_GAME_SCREEN_MENU;
            ctx->menu_sel = (uint32_t)CROSSTALK_MENU_SEL_FIRST;
            ctx->menu_confirm = LDG_TRUTH_FALSE;
        }
    }

    ctx->key_esc = LDG_TRUTH_FALSE;
    ctx->key_esc_shift = LDG_TRUTH_FALSE;

    if (ctx->game.screen == CROSSTALK_GAME_SCREEN_EDITOR && ctx->game.paused == 0)
    {
        if (crosstalk_eda_ui_term_open_is(&ctx->eda.term) == LDG_TRUTH_TRUE)
        {
            uint8_t term_on = crosstalk_ui_window_focused_is(&ctx->win_nav, (uint64_t)CROSSTALK_EDA_UI_TERM_WINDOW_ID);
            uint8_t term_focus_prev = crosstalk_eda_ui_term_focus_is(&ctx->eda.term);

            ret = crosstalk_eda_ui_term_focus_set(&ctx->eda.term, term_on);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term focus sync failed"); }

            if (term_focus_prev != term_on && crosstalk_eda_io_pty_live_is(&ctx->eda.pty) == LDG_TRUTH_TRUE) { if (crosstalk_eda_io_pty_focus_put(&ctx->eda.pty, term_on) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty focus report failed"); } }
        }
        else if (crosstalk_eda_ui_term_focus_is(&ctx->eda.term) == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_eda_ui_term_open_set(&ctx->eda.term, LDG_TRUTH_FALSE);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term close sync failed"); }

            ret = crosstalk_eda_ui_term_focus_set(&ctx->eda.term, LDG_TRUTH_FALSE);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term focus release failed"); }

            if (crosstalk_eda_io_pty_live_is(&ctx->eda.pty) == LDG_TRUTH_TRUE) { if (crosstalk_eda_io_pty_focus_put(&ctx->eda.pty, LDG_TRUTH_FALSE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty focus report failed"); } }

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "term: focus released on window close");
        }

        if (crosstalk_text_field_is(ctx) == LDG_TRUTH_FALSE)
        {
            if (ctx->key_save == LDG_TRUTH_TRUE)
            {
                ret = crosstalk_core_save_wr(ctx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "save key failed"); }
            }

            if (crosstalk_eda_ic_fpga_build_live_is(&ctx->eda.fpga_build) != LDG_TRUTH_TRUE && ctx->key_open == LDG_TRUTH_TRUE)
            {
                ret = ld_begin(ctx, CROSSTALK_LD_FILE, CROSSTALK_GAME_SCREEN_EDITOR);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "load kick failed"); }
            }
        }
    }

    ctx->key_save = LDG_TRUTH_FALSE;
    ctx->key_open = LDG_TRUTH_FALSE;

    ret = crosstalk_term_frame_begin(ctx, &render, &screen_rect);
    if (ret != LDG_ERR_AOK && ret != LDG_ERR_NOT_INIT) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: term frame begin failed; err: %u32", ret); }

    desk_rect = screen_rect;

    if (ctx->game.screen == CROSSTALK_GAME_SCREEN_EDITOR && desk_rect.row_cunt > (uint32_t)CROSSTALK_STATUSLINE_ROWS) { desk_rect.row_cunt = desk_rect.row_cunt - (uint32_t)CROSSTALK_STATUSLINE_ROWS; }

    crosstalk_menu_nav_sync(ctx);

    ret = crosstalk_game_fx_sfx_tone_tick(&ctx->game.sfx_sound);

    if (ctx->game.screen != CROSSTALK_GAME_SCREEN_LD && crosstalk_game_fx_sfx_cw_on_is(&ctx->game.sfx_sound) != LDG_TRUTH_TRUE) { ret = crosstalk_game_fx_sfx_tone_stop(&ctx->game.sfx_sound); }

    if (ctx->game.screen == CROSSTALK_GAME_SCREEN_LD)
    {
        if (ctx->ld_frames < UINT8_MAX) { ctx->ld_frames = ctx->ld_frames + 0x1; }

        crosstalk_card_kind_resolve(ctx);
        ret = crosstalk_game_fx_sfx_tone_start(&ctx->game.sfx_sound);
        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "card tone start failed"); }

        if (render != 0x0)
        {
            ret = crosstalk_crt_card_draw(ctx, render);
            if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "card draw failed"); }
        }
    }
    else if (ctx->game.screen == CROSSTALK_GAME_SCREEN_MENU)
    {
        if (ctx->settings_open == LDG_TRUTH_FALSE)
        {
            const char *menu_item[CROSSTALK_MENU_SWITCH_ITEM_CUNT] = { "Load", "Story", "ISA design", "RTL", "Zone design", "Reverse", "Multiplayer", "Extras", "Credits", "Quit" };
            uint8_t menu_hide[CROSSTALK_MENU_SWITCH_ITEM_CUNT] = LDG_ARR_ZERO_INIT;
            uint32_t menu_idx = 0;

            menu_hide[CROSSTALK_MENU_IDX_ISA] = LDG_TRUTH_TRUE;
            menu_hide[CROSSTALK_MENU_IDX_MP] = LDG_TRUTH_TRUE;
            menu_hide[CROSSTALK_MENU_IDX_STORY] = ldg_net_steamworks_dlc_installed_is(CROSSTALK_CORE_STORY_APP_ID) == LDG_TRUTH_TRUE ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE;
            menu_hide[CROSSTALK_MENU_IDX_REVERSE] = ldg_net_steamworks_dlc_installed_is(CROSSTALK_CORE_REVERSE_APP_ID) == LDG_TRUTH_TRUE ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE;
            menu_hide[CROSSTALK_MENU_IDX_ZONE] = LDG_TRUTH_TRUE;

            if (render != 0x0)
            {
                ret = crosstalk_menu_switch_draw(render, &screen_rect, CROSSTALK_MENU_SWITCH_CAPTION, menu_item, menu_hide, (uint32_t)CROSSTALK_MENU_SWITCH_ITEM_CUNT, ctx->menu_sel, LDG_TRUTH_FALSE);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "menu switch draw failed"); }
            }

            if (crosstalk_menu_confirm_take(ctx, menu_hide, (uint32_t)CROSSTALK_MENU_SWITCH_ITEM_CUNT, &menu_idx) == LDG_ERR_AOK)
            {
                if (menu_idx == CROSSTALK_MENU_IDX_LD)
                {
                    ret = crosstalk_board_sel_refresh(ctx);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "board list refresh failed"); }

                    ctx->game.screen = CROSSTALK_GAME_SCREEN_BOARD_SEL;
                }
                else if (menu_idx == CROSSTALK_MENU_IDX_STORY) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: story selected; app: %u32; screen not wired", (uint32_t)CROSSTALK_CORE_STORY_APP_ID); }
                else if (menu_idx == CROSSTALK_MENU_IDX_REVERSE || menu_idx == CROSSTALK_MENU_IDX_RTL)
                {
                    uint8_t entry_kind = (uint8_t)CROSSTALK_CORE_NEWPROJECT_KIND_RTL;

                    if (menu_idx == CROSSTALK_MENU_IDX_REVERSE) { entry_kind = (uint8_t)CROSSTALK_CORE_NEWPROJECT_KIND_REVERSE; }

                    ctx->newproject_kind = entry_kind;
                    ctx->board_name[0] = LDG_STR_TERM;

                    ret = crosstalk_board_sel_refresh(ctx);
                    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "board list refresh failed"); }

                    ret = crosstalk_newproject_list_build(ctx);
                    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "newproject list build failed"); }

                    ctx->game.screen = CROSSTALK_GAME_SCREEN_NEWPROJECT;
                }
                else if (menu_idx == CROSSTALK_MENU_IDX_EXTRAS)
                {
                    ret = crosstalk_extras_refresh(ctx);
                    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: extras refresh failed; err: %u32", ret); }

                    ctx->game.screen = CROSSTALK_GAME_SCREEN_EXTRAS;
                }
                else if (menu_idx == CROSSTALK_MENU_IDX_CREDITS)
                {
                    ret = crosstalk_credits_refresh(ctx);
                    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: credits refresh failed; err: %u32", ret); }

                    ctx->game.screen = CROSSTALK_GAME_SCREEN_CREDITS;
                }
                else if (menu_idx == CROSSTALK_MENU_IDX_QUIT) { ctx->game.quit = LDG_TRUTH_TRUE; }
                else { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: menu idx unhandled; idx: %u32", menu_idx); }
            }
        }
    }
    else if (ctx->game.screen == CROSSTALK_GAME_SCREEN_MP)
    {
        ldg_net_lobby_state_t lobby_state = crosstalk_net_steam_lobby_state_get(&ctx->steam_net);

        if (render != 0x0)
        {
            ret = crosstalk_lobby_wave_draw(ctx, render);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "lobby wave draw failed"); }
        }

        if ((uint8_t)lobby_state != ctx->mp_lobby_state_prev)
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: lobby state; prev: %u32; now: %u32; members: %u32; id_ok: %u32; conn: %u32", (uint32_t)ctx->mp_lobby_state_prev, (uint32_t)lobby_state, crosstalk_net_steam_lobby_member_cunt_get(&ctx->steam_net), (uint32_t)(crosstalk_net_steam_lobby_id_get(&ctx->steam_net) != 0), (uint32_t)ldg_net_steamworks_status_get(&ctx->net));
            if ((ldg_net_lobby_state_t)ctx->mp_lobby_state_prev == LDG_NET_LOBBY_STATE_CREATING && lobby_state == LDG_NET_LOBBY_STATE_NONE) { toast_push(ctx, "lobby creation failed"); }

            if ((ldg_net_lobby_state_t)ctx->mp_lobby_state_prev == LDG_NET_LOBBY_STATE_JOINING && lobby_state == LDG_NET_LOBBY_STATE_NONE) { toast_push(ctx, "could not join the lobby"); }

            ctx->mp_lobby_state_prev = (uint8_t)lobby_state;
        }

        if (ctx->mp_host_pending != LDG_TRUTH_FALSE && ctx->game.ld_active == 0 && ctx->time >= ctx->mp_host_retry_next)
        {
            const char *host_name = 0x0;

            ctx->mp_players_max = CROSSTALK_MP_PLAYERS_MIN + ctx->mp_players_idx * CROSSTALK_MP_PLAYERS_STEP;

            ret = crosstalk_net_steam_lobby_mode_set(&ctx->steam_net, CROSSTALK_LOBBY_MODE_TEXT);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "lobby mode set failed"); }

            if (ctx->mp_transport_idx == (uint32_t)CROSSTALK_NET_TRANSPORT_LAN)
            {
                ctx->mp_host_pending = LDG_TRUTH_FALSE;
                host_name = ldg_net_steamworks_local_name_get();
                ret = crosstalk_net_steam_lan_host_start(&ctx->steam_net, LDG_NET_PORT_DEFAULT, ctx->mp_players_max, CROSSTALK_LOBBY_MODE_TEXT, host_name);
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: lan host; err: %u32; port: %u32", ret, (uint32_t)LDG_NET_PORT_DEFAULT);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { toast_push(ctx, "LAN host failed; check port and Steam"); }
            }
            else
            {
                ret = crosstalk_net_steam_host_start(&ctx->steam_net, CROSSTALK_NET_VIRTUAL_PORT);
                if (ret == LDG_ERR_AOK) { ret = crosstalk_net_steam_lobby_create(&ctx->steam_net, ctx->mp_players_max); }

                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: lobby host; err: %u32; retries: %u32", ret, (uint32_t)ctx->mp_host_retry);

                if (ret == LDG_ERR_AOK) { ctx->mp_host_pending = LDG_TRUTH_FALSE; }
                else if (ctx->mp_host_retry > 0)
                {
                    ctx->mp_host_retry = (uint8_t)(ctx->mp_host_retry - 0x1);
                    ctx->mp_host_retry_next = ctx->time + CROSSTALK_CORE_MP_HOST_RETRY_SECS;
                }
                else
                {
                    ctx->mp_host_pending = LDG_TRUTH_FALSE;
                    toast_push(ctx, CROSSTALK_CORE_MP_HOST_FAIL_TEXT);
                }
            }
        }

        if (lobby_state == LDG_NET_LOBBY_STATE_STARTING)
        {
            ctx->mp_rdy = LDG_TRUTH_FALSE;
            ctx->mp_busy = LDG_TRUTH_FALSE;
            crosstalk_net_steam_lobby_busy_set(&ctx->steam_net, LDG_TRUTH_FALSE);
            ctx->game.screen = CROSSTALK_GAME_SCREEN_EDITOR;
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: lobby started; entering editor");
        }
        else if (lobby_state == LDG_NET_LOBBY_STATE_CREATING || lobby_state == LDG_NET_LOBBY_STATE_JOINING) { if (render != 0x0) { ret = crosstalk_game_menu_ld_draw(render, &screen_rect, CROSSTALK_MP_WAIT_TEXT, CROSSTALK_MP_WAIT_FRAC); } }
        else if (crosstalk_mp_room_is(ctx) == LDG_TRUTH_TRUE)
        {
            crosstalk_game_menu_room_action_t room_action = CROSSTALK_GAME_MENU_ROOM_NONE;
            uint8_t lan_sesh = crosstalk_net_steam_lan_sesh_get(&ctx->steam_net);
            uint8_t is_host = LDG_TRUTH_FALSE;
            uint32_t member_cunt = 0;
            uint32_t room_action_raw = (uint32_t)CROSSTALK_CORE_MENU_ACTION_NONE;
            uint32_t room_row = UINT32_MAX;

            crosstalk_mp_room_get(ctx, &member_cunt, &is_host);

            ret = crosstalk_member_items_build(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "member items build failed"); }

            if (render != 0x0)
            {
                ret = crosstalk_game_menu_room_draw(render, &screen_rect, &ctx->menu_nav, ctx->member_items, member_cunt, is_host, ctx->mp_rdy);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "room draw failed"); }
            }

            crosstalk_menu_action_take(ctx, &room_action_raw, &room_row);
            room_action = (crosstalk_game_menu_room_action_t)room_action_raw;

            if (room_action == CROSSTALK_GAME_MENU_ROOM_RDY)
            {
                ctx->mp_rdy = (uint8_t)(ctx->mp_rdy ^ LDG_TRUTH_TRUE);
                if (lan_sesh == (uint8_t)CROSSTALK_NET_LAN_SESH_NONE) { crosstalk_net_steam_lobby_rdy_set(&ctx->steam_net, ctx->mp_rdy); }

                if (ctx->mp_rdy == LDG_TRUTH_FALSE) { ctx->snap_req_sent = LDG_TRUTH_FALSE; }
            }
            else if (room_action == CROSSTALK_GAME_MENU_ROOM_START)
            {
                ctx->game.paused = LDG_TRUTH_FALSE;
                ctx->paused_by_focus = LDG_TRUTH_FALSE;

                if (lan_sesh != (uint8_t)CROSSTALK_NET_LAN_SESH_NONE)
                {
                    ctx->mp_rdy = LDG_TRUTH_FALSE;
                    ctx->mp_busy = LDG_TRUTH_FALSE;
                    ctx->game.screen = CROSSTALK_GAME_SCREEN_EDITOR;
                    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: lan start");
                }
                else
                {
                    ret = crosstalk_net_steam_lobby_game_start(&ctx->steam_net);
                    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: lobby start; err: %u32", ret);
                }
            }
            else if (room_action == CROSSTALK_GAME_MENU_ROOM_LEAVE)
            {
                if (lan_sesh == (uint8_t)CROSSTALK_NET_LAN_SESH_NONE) { crosstalk_net_steam_lobby_leave(&ctx->steam_net); }

                crosstalk_net_steam_sesh_stop(&ctx->steam_net);
                ctx->mp_rdy = LDG_TRUTH_FALSE;
                ctx->mp_busy = LDG_TRUTH_FALSE;
                ret = crosstalk_game_chat_rst(&ctx->chat);
                ret = crosstalk_game_chat_open_set(&ctx->chat, LDG_TRUTH_FALSE);
            }
            else if (room_action == CROSSTALK_GAME_MENU_ROOM_BUILD)
            {
                if (ctx->mp_rdy != LDG_TRUTH_FALSE) { toast_push(ctx, "you are readied up; un-ready to keep building"); }
                else if (ctx->remote_sesh == 0)
                {
                    ctx->mp_busy = LDG_TRUTH_TRUE;
                    if (lan_sesh == (uint8_t)CROSSTALK_NET_LAN_SESH_NONE) { crosstalk_net_steam_lobby_busy_set(&ctx->steam_net, LDG_TRUTH_TRUE); }

                    ctx->game.screen = CROSSTALK_GAME_SCREEN_EDITOR;
                    ret = crosstalk_game_progression_achievement_unlock_req(CROSSTALK_GAME_PROGRESSION_LOBBY_BUILDER, CROSSTALK_APP_ID, &ctx->progression_queue);
                    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "crosstalk: ach lobby_builder; err: %u32", ret); }
                }
                else { toast_push(ctx, "no local board loaded; load one from the main menu first"); }
            }
        }
        else
        {
            crosstalk_game_menu_mp_action_t mp_action = CROSSTALK_GAME_MENU_MP_NONE;
            const ldg_net_lobby_info_t *pick_info = 0x0;
            const ldg_net_srv_browser_entry_t *lan_entry = 0x0;
            char mp_pick[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
            uint32_t mp_action_raw = (uint32_t)CROSSTALK_CORE_MENU_ACTION_NONE;
            uint32_t mp_row = UINT32_MAX;
            uint32_t list_cunt = 0;
            uint32_t transport_prev = ctx->mp_transport_prev;

            ctx->mp_transport_prev = ctx->mp_transport_idx;

            ret = crosstalk_lobby_items_build(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "lobby items build failed"); }

            list_cunt = (ctx->mp_transport_idx == (uint32_t)CROSSTALK_NET_TRANSPORT_LAN) ? crosstalk_net_steam_lan_list_cunt_get(&ctx->steam_net) : crosstalk_net_steam_lobby_list_cunt_get(&ctx->steam_net);

            if (render != 0x0)
            {
                ret = crosstalk_game_menu_mp_draw(render, &screen_rect, &ctx->menu_nav, ctx->lobby_items, list_cunt, ctx->board_sel.names, ctx->board_sel.cunt, ctx->mp_board_idx, ctx->mp_players_idx, ctx->mp_submode_idx, ctx->mp_transport_idx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "mp draw failed"); }
            }

            crosstalk_menu_action_take(ctx, &mp_action_raw, &mp_row);
            mp_action = (crosstalk_game_menu_mp_action_t)mp_action_raw;

            if (ctx->mp_transport_idx != transport_prev)
            {
                if (crosstalk_game_fx_sfx_lever_play(&ctx->game.sfx_sound) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "lever sfx failed"); }

                if (ctx->mp_transport_idx == (uint32_t)CROSSTALK_NET_TRANSPORT_LAN) { ret = crosstalk_net_steam_lan_list_req(&ctx->steam_net); }
                else { ret = crosstalk_net_steam_lobby_list_req(&ctx->steam_net); }
            }

            if (mp_action == CROSSTALK_GAME_MENU_MP_REFRESH) { if (ctx->mp_transport_idx == (uint32_t)CROSSTALK_NET_TRANSPORT_LAN) { ret = crosstalk_net_steam_lan_list_req(&ctx->steam_net); }
                else
                {
                    ret = crosstalk_net_steam_lobby_list_req(&ctx->steam_net);
                    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: lobby list req; err: %u32", ret);
                } }
            else if (mp_action == CROSSTALK_GAME_MENU_MP_JOIN)
            {
                if (ctx->mp_transport_idx == (uint32_t)CROSSTALK_NET_TRANSPORT_LAN)
                {
                    lan_entry = crosstalk_net_steam_lan_list_get(&ctx->steam_net, mp_row);
                    if (lan_entry != 0x0)
                    {
                        ctx->mp_players_max = (uint32_t)lan_entry->client_max;
                        ret = crosstalk_net_steam_lan_client_join(&ctx->steam_net, lan_entry->ip, lan_entry->port);
                        if (ret != LDG_ERR_AOK) { toast_push(ctx, "could not join the LAN session"); }

                        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: lan join; err: %u32; ip: %u32; port: %u32", ret, lan_entry->ip, (uint32_t)lan_entry->port);
                    }
                }
                else
                {
                    pick_info = crosstalk_net_steam_lobby_list_get(&ctx->steam_net, mp_row);
                    if (pick_info != 0x0)
                    {
                        ctx->mp_players_max = (uint32_t)pick_info->player_max;
                        ret = crosstalk_net_steam_lobby_join(&ctx->steam_net, pick_info->lobby_id);
                        if (ret == LDG_ERR_AOK) { ctx->game.net_join_phase = CROSSTALK_NET_JOIN_PHASE_CONN; }

                        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: lobby join; err: %u32; lobby: %u64", ret, pick_info->lobby_id);
                    }
                }
            }
            else if (mp_action == CROSSTALK_GAME_MENU_MP_HOST)
            {
                if (ctx->board_sel.cunt == 0) { toast_push(ctx, "create a board first; New Design from the menu"); }
                else
                {
                    ret = crosstalk_game_board_path_build(&ctx->board_sel, mp_row, mp_pick, (uint64_t)sizeof(mp_pick));
                    if (ret == LDG_ERR_AOK) { ret = ldg_str_rbrcpy(ctx->save_path, mp_pick, (uint64_t)sizeof(ctx->save_path)); }

                    ctx->editor_kind = (uint8_t)CROSSTALK_CORE_NEWPROJECT_KIND_RTL;

                    if (ret == LDG_ERR_AOK) { ret = ldg_str_rbrcpy(ctx->board_name, ctx->board_sel.entry[mp_row].name, (uint64_t)sizeof(ctx->board_name)); }

                    if (ret == LDG_ERR_AOK)
                    {
                        ctx->mp_host_pending = LDG_TRUTH_TRUE;
                        ctx->mp_host_retry = CROSSTALK_CORE_MP_HOST_RETRY_CUNT;
                        ctx->mp_host_retry_next = 0.0;
                        ret = ld_begin(ctx, CROSSTALK_LD_FILE, CROSSTALK_GAME_SCREEN_MP);
                    }

                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
                    {
                        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "host board load failed");
                        toast_push(ctx, "host board load failed");
                    }
                }
            }
            else if (mp_action == CROSSTALK_GAME_MENU_MP_BACK)
            {
                ctx->mp_host_pending = LDG_TRUTH_FALSE;
                ctx->game.screen = CROSSTALK_GAME_SCREEN_MENU;
            }
        }
    }
    else if (ctx->game.screen == CROSSTALK_GAME_SCREEN_BOARD_SEL)
    {
        crosstalk_game_menu_board_sel_action_t sel_action = CROSSTALK_GAME_MENU_BOARD_SEL_NONE;
        char pick[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
        uint32_t sel_action_raw = (uint32_t)CROSSTALK_CORE_MENU_ACTION_NONE;
        uint32_t sel_row = UINT32_MAX;
        uint32_t active_prev = ctx->board_sel_active;
        uint8_t wshop_prev = ctx->board_sel_wshop_prev;

        ctx->board_sel_active = ctx->menu_nav.row;
        ctx->board_sel_wshop_prev = ctx->board_sel_wshop_on;

        if (ctx->board_sel_confirm_row != UINT32_MAX && ldg_time_epoch_ms_get() - ctx->board_sel_confirm_ms > (uint64_t)CROSSTALK_BOARD_SEL_ARM_MS) { ctx->board_sel_confirm_row = UINT32_MAX; }

        crosstalk_board_preview_refresh(ctx, ctx->board_sel_active);

        if (render != 0x0)
        {
            ret = crosstalk_game_menu_board_sel_draw(render, &screen_rect, &ctx->menu_nav, ctx->board_sel.items, ctx->board_sel.lock, ctx->board_sel.cunt, ctx->board_sel.truncated, ctx->board_sel_confirm_row, ctx->board_sel_wshop_on);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "board select draw failed"); }
        }

        crosstalk_menu_action_take(ctx, &sel_action_raw, &sel_row);
        sel_action = (crosstalk_game_menu_board_sel_action_t)sel_action_raw;

        if (ctx->board_sel_active != active_prev && ctx->board_sel.cunt > 0) { if (crosstalk_game_fx_sfx_chirp_play(&ctx->game.sfx_sound) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "chirp sfx failed"); } }

        if (ctx->board_preview_ok == LDG_TRUTH_TRUE && render != 0x0)
        {
            uint32_t text_col = (uint32_t)((double)screen_rect.col_cunt * CROSSTALK_BOARD_PREVIEW_X_FRAC);
            uint32_t text_row = (uint32_t)((double)screen_rect.row_cunt * CROSSTALK_BOARD_PREVIEW_Y_FRAC);
            uint32_t drawn = 0;
            uint32_t line_idx = 0;

            for (line_idx = 0; line_idx < CROSSTALK_CORE_BOARD_PREVIEW_LINE_CUNT && text_col < screen_rect.col_cunt; line_idx = line_idx + 1)
            {
                if (ctx->board_preview_line[line_idx][0] == LDG_STR_TERM) { continue; }

                if (crosstalk_text_clip_put(render, text_col, text_row + line_idx, screen_rect.col_cunt - text_col, ctx->board_preview_line[line_idx], (uint32_t)CROSSTALK_UI_THEME_TEXT_RGBA, (uint32_t)CROSSTALK_UI_CELL_PANE_RGBA, &drawn) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "board preview line draw failed"); }
            }
        }

        if (ctx->board_sel_wshop_on != wshop_prev)
        {
            if (crosstalk_game_fx_sfx_lever_play(&ctx->game.sfx_sound) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "lever sfx failed"); }

            ret = crosstalk_board_sel_refresh(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "board list refresh failed"); }
        }
        else if (sel_action == CROSSTALK_GAME_MENU_BOARD_SEL_LD)
        {
            if (crosstalk_game_fx_sfx_chirp_play(&ctx->game.sfx_sound) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "chirp sfx failed"); }

            ctx->editor_kind = (uint8_t)CROSSTALK_CORE_NEWPROJECT_KIND_RTL;
            ret = crosstalk_game_board_path_build(&ctx->board_sel, sel_row, pick, (uint64_t)sizeof(pick));
            if (ret == LDG_ERR_AOK) { ret = ldg_str_rbrcpy(ctx->save_path, pick, (uint64_t)sizeof(ctx->save_path)); }

            if (ret == LDG_ERR_AOK) { ret = ldg_str_rbrcpy(ctx->board_name, ctx->board_sel.entry[sel_row].name, (uint64_t)sizeof(ctx->board_name)); }

            if (ret == LDG_ERR_AOK)
            {
                ctx->editorpick_mask = 0;

                if (crosstalk_save_editor_mask_get(ctx->save_path, &ctx->editorpick_mask) != LDG_ERR_AOK) { ctx->editorpick_mask = 0; }

                if (crosstalk_editorpick_items_build(ctx) != LDG_ERR_AOK) { ctx->editorpick_cunt = 0; }

                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: save editors; path: %.255s; mask: 0x%02X32; cunt: %u32", ctx->save_path, (uint32_t)ctx->editorpick_mask, ctx->editorpick_cunt);
            }

            if (ret == LDG_ERR_AOK && ctx->editorpick_cunt > 0x1) { ctx->game.screen = CROSSTALK_GAME_SCREEN_EDITORPICK; }
            else
            {
                if (ret == LDG_ERR_AOK && ctx->editorpick_cunt == 0x1) { ctx->editor_kind = ctx->editorpick_kind[0]; }

                if (ret == LDG_ERR_AOK) { ret = ld_begin(ctx, CROSSTALK_LD_FILE, CROSSTALK_GAME_SCREEN_EDITOR); }
            }

            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_ERR, "board open failed"); }

            crosstalk_board_preview_drop(ctx);
        }
        else if (sel_action == CROSSTALK_GAME_MENU_BOARD_SEL_REMOVE && ldg_time_epoch_ms_get() >= ctx->board_sel_lock_ms)
        {
            uint64_t now_ms = ldg_time_epoch_ms_get();

            if (crosstalk_game_fx_sfx_chirp_play(&ctx->game.sfx_sound) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "chirp sfx failed"); }

            if (ctx->board_sel_confirm_row == sel_row)
            {
                ret = crosstalk_game_board_remove(&ctx->board_sel, sel_row);
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "crosstalk: board remove; row: %u32; err: %u32", sel_row, ret);

                ctx->board_sel_lock_ms = now_ms + (uint64_t)CROSSTALK_BOARD_SEL_LOCK_MS;

                ret = crosstalk_board_sel_refresh(ctx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "board list refresh failed"); }

                if (ctx->board_sel.cunt > 0)
                {
                    ctx->menu_nav.row = sel_row < ctx->board_sel.cunt ? sel_row : ctx->board_sel.cunt - 0x1;
                    ctx->board_sel_active = ctx->menu_nav.row;
                }
            }
            else
            {
                ctx->board_sel_confirm_row = sel_row;
                ctx->board_sel_confirm_ms = now_ms;
                crosstalk_net_steam_notice_push(&ctx->steam_net, "press delete again to confirm");
            }
        }
        else if (sel_action == CROSSTALK_GAME_MENU_BOARD_SEL_BACK)
        {
            if (crosstalk_game_fx_sfx_chirp_play(&ctx->game.sfx_sound) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "chirp sfx failed"); }

            crosstalk_board_preview_drop(ctx);
            ctx->game.screen = CROSSTALK_GAME_SCREEN_MENU;
        }
    }
    else if (ctx->game.screen == CROSSTALK_GAME_SCREEN_EXTRAS)
    {
        crosstalk_game_menu_extras_action_t extras_action = CROSSTALK_GAME_MENU_EXTRAS_NONE;
        uint32_t extras_action_raw = (uint32_t)CROSSTALK_CORE_MENU_ACTION_NONE;
        uint32_t extras_row = UINT32_MAX;

        if (render != 0x0)
        {
            ret = crosstalk_game_menu_extras_draw(render, &screen_rect, &ctx->menu_nav, ctx->extras_items, ctx->extras_cunt);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "extras draw failed"); }
        }

        crosstalk_menu_action_take(ctx, &extras_action_raw, &extras_row);
        extras_action = (crosstalk_game_menu_extras_action_t)extras_action_raw;

        if (extras_action == CROSSTALK_GAME_MENU_EXTRAS_PICK && extras_row != UINT32_MAX)
        {
            ret = crosstalk_extras_play(ctx, extras_row);
            if (ret == LDG_ERR_UNSUPPORTED) { toast_push(ctx, CROSSTALK_CORE_MUSIC_NEEDS_NATIVE_TEXT); }
            else if (ret != LDG_ERR_AOK) { toast_push(ctx, "could not play that track"); }
        }
        else if (extras_action == CROSSTALK_GAME_MENU_EXTRAS_BACK) { ctx->game.screen = CROSSTALK_GAME_SCREEN_MENU; }
    }
    else if (ctx->game.screen == CROSSTALK_GAME_SCREEN_CREDITS)
    {
        crosstalk_game_menu_credits_action_t credits_action = CROSSTALK_GAME_MENU_CREDITS_NONE;
        uint32_t credits_action_raw = (uint32_t)CROSSTALK_CORE_MENU_ACTION_NONE;
        uint32_t credits_row = UINT32_MAX;

        if (render != 0x0)
        {
            ret = crosstalk_game_menu_credits_draw(render, &screen_rect, &ctx->menu_nav, ctx->credits_items, ctx->credits_cunt);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "credits draw failed"); }
        }

        crosstalk_menu_action_take(ctx, &credits_action_raw, &credits_row);
        credits_action = (crosstalk_game_menu_credits_action_t)credits_action_raw;

        if (credits_action == CROSSTALK_GAME_MENU_CREDITS_BACK) { ctx->game.screen = CROSSTALK_GAME_SCREEN_MENU; }
    }
    else if (ctx->game.screen == CROSSTALK_GAME_SCREEN_EDITORPICK)
    {
        crosstalk_game_menu_editorpick_action_t pick_action = CROSSTALK_GAME_MENU_EDITORPICK_NONE;
        uint32_t pick_action_raw = (uint32_t)CROSSTALK_CORE_MENU_ACTION_NONE;
        uint32_t pick_row = UINT32_MAX;

        if (render != 0x0)
        {
            ret = crosstalk_game_menu_editorpick_draw(render, &screen_rect, &ctx->menu_nav, ctx->board_name, ctx->editorpick_items, ctx->editorpick_idx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "editor pick draw failed"); }
        }

        crosstalk_menu_action_take(ctx, &pick_action_raw, &pick_row);
        pick_action = (crosstalk_game_menu_editorpick_action_t)pick_action_raw;

        if (pick_action == CROSSTALK_GAME_MENU_EDITORPICK_OPEN)
        {
            if (ctx->editorpick_idx < ctx->editorpick_cunt) { ctx->editor_kind = ctx->editorpick_kind[ctx->editorpick_idx]; }

            author_capture(ctx);
            ret = ld_begin(ctx, CROSSTALK_LD_FILE, CROSSTALK_GAME_SCREEN_EDITOR);
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "crosstalk: save opened as editor; kind: %u32; path: %.255s; err: %u32", (uint32_t)ctx->editor_kind, ctx->save_path, ret);

            if (ctx->editor_kind == (uint8_t)CROSSTALK_CORE_NEWPROJECT_KIND_REVERSE) { ctx->reverse_setup_pending = LDG_TRUTH_TRUE; }
        }
        else if (pick_action == CROSSTALK_GAME_MENU_EDITORPICK_BACK) { ctx->game.screen = CROSSTALK_GAME_SCREEN_BOARD_SEL; }
    }
    else if (ctx->game.screen == CROSSTALK_GAME_SCREEN_NEWPROJECT)
    {
        crosstalk_game_menu_newproject_action_t np_action = CROSSTALK_GAME_MENU_NEWPROJECT_NONE;
        uint32_t np_action_raw = (uint32_t)CROSSTALK_CORE_MENU_ACTION_NONE;
        uint32_t np_row = UINT32_MAX;

        if (render != 0x0)
        {
            ret = crosstalk_game_menu_newproject_draw(render, &screen_rect, &ctx->menu_nav, ctx->board_name, ctx->newproject_items, ctx->newproject_pick_cunt, ctx->newproject_pick);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "new project draw failed"); }
        }

        crosstalk_menu_action_take(ctx, &np_action_raw, &np_row);
        np_action = (crosstalk_game_menu_newproject_action_t)np_action_raw;

        if (np_action == CROSSTALK_GAME_MENU_NEWPROJECT_CREATE && ctx->newproject_pick == CROSSTALK_CORE_NEWPROJECT_PICK_NEW && ctx->board_name[0] == LDG_STR_TERM) { toast_push(ctx, "name the project"); }
        else if (np_action == CROSSTALK_GAME_MENU_NEWPROJECT_CREATE)
        {
            char pick_path[LDG_IO_PATH_MAX] = LDG_ARR_ZERO_INIT;
            uint32_t pick_row = 0;

            author_capture(ctx);
            ctx->editor_kind = ctx->newproject_kind;

            if (ctx->newproject_pick == CROSSTALK_CORE_NEWPROJECT_PICK_NEW) { ret = ld_begin(ctx, CROSSTALK_LD_BARE, CROSSTALK_GAME_SCREEN_EDITOR); }
            else
            {
                pick_row = ctx->newproject_row[ctx->newproject_pick - 0x1];
                ret = crosstalk_game_board_path_build(&ctx->board_sel, pick_row, pick_path, (uint64_t)sizeof(pick_path));
                if (ret == LDG_ERR_AOK) { ret = ldg_str_rbrcpy(ctx->save_path, pick_path, (uint64_t)sizeof(ctx->save_path)); }

                if (ret == LDG_ERR_AOK) { ret = ldg_str_rbrcpy(ctx->board_name, ctx->board_sel.entry[pick_row].name, (uint64_t)sizeof(ctx->board_name)); }

                if (ret == LDG_ERR_AOK) { ret = ld_begin(ctx, CROSSTALK_LD_FILE, CROSSTALK_GAME_SCREEN_EDITOR); }

                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "crosstalk: project adopted; kind: %u32; path: %.255s; err: %u32", (uint32_t)ctx->editor_kind, ctx->save_path, ret);
            }

            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: project bench start failed; err: %u32", ret); }

            if (ctx->editor_kind == (uint8_t)CROSSTALK_CORE_NEWPROJECT_KIND_REVERSE) { ctx->reverse_setup_pending = LDG_TRUTH_TRUE; }
        }
        else if (np_action == CROSSTALK_GAME_MENU_NEWPROJECT_BACK) { ctx->game.screen = CROSSTALK_GAME_SCREEN_MENU; }
    }

    if (ctx->game.screen == CROSSTALK_GAME_SCREEN_EDITOR)
    {
        {
            const char *statusline_cell[CROSSTALK_STATUSLINE_CELL_CAP] = LDG_ARR_ZERO_INIT;
            char statusline_prog[CROSSTALK_STATUSLINE_PROG_CAP] = LDG_ARR_ZERO_INIT;
            char statusline_loc[CROSSTALK_STATUSLINE_LOC_CAP] = LDG_ARR_ZERO_INIT;
            char statusline_proj[CROSSTALK_CORE_PROJ_STATUS_CAP] = LDG_ARR_ZERO_INIT;
            char statusline_cyc[CROSSTALK_STATUSLINE_CYC_CAP] = LDG_ARR_ZERO_INIT;
            char statusline_rate[CROSSTALK_STATUSLINE_RATE_CAP] = LDG_ARR_ZERO_INIT;
            char statusline_simclk[CROSSTALK_STATUSLINE_SIMCLK_CAP] = LDG_ARR_ZERO_INIT;
            uint32_t statusline_cunt = 0;
            char statusline_mp[CROSSTALK_STATUSLINE_MP_CAP] = LDG_ARR_ZERO_INIT;

            if (crosstalk_eda_ic_fpga_build_live_is(&ctx->eda.fpga_build) == LDG_TRUTH_TRUE)
            {
                const char *build_status = 0x0;

                if (crosstalk_eda_ic_fpga_build_status_get(&ctx->eda.fpga_build, &build_status) == LDG_ERR_AOK && build_status != 0x0)
                {
                    statusline_cell[statusline_cunt] = build_status;
                    statusline_cunt = statusline_cunt + (uint32_t)CROSSTALK_STATUSLINE_CELL_STEP;
                }
            }
            else if (crosstalk_net_steam_notice_active_is(&ctx->steam_net) == LDG_TRUTH_TRUE)
            {
                statusline_cell[statusline_cunt] = crosstalk_net_steam_notice_text_get(&ctx->steam_net);
                statusline_cunt = statusline_cunt + (uint32_t)CROSSTALK_STATUSLINE_CELL_STEP;
            }
            else if (ctx->toast_text[0] != LDG_STR_TERM && ldg_time_epoch_ms_get() < ctx->toast_until_ms)
            {
                statusline_cell[statusline_cunt] = ctx->toast_text;
                statusline_cunt = statusline_cunt + (uint32_t)CROSSTALK_STATUSLINE_CELL_STEP;
            }

            if (crosstalk_net_steam_status_get(&ctx->steam_net) == LDG_NET_CONN_STATUS_CONN)
            {
                uint64_t mp_local_id = ldg_net_steamworks_local_steam_id_get();
                uint32_t mp_member_cunt = crosstalk_net_steam_lobby_member_cunt_get(&ctx->steam_net);
                const char *mp_peer = "";
                uint32_t mp_i = 0;
                for (mp_i = 0; mp_i < mp_member_cunt; mp_i = mp_i + 1)
                {
                    const ldg_net_lobby_member_t *mp_member = crosstalk_net_steam_lobby_member_get(&ctx->steam_net, mp_i);
                    if (mp_member != 0x0 && mp_member->user_id != mp_local_id) { mp_peer = mp_member->name; break; }
                }

                snprintf(statusline_mp, sizeof(statusline_mp), "MP %u | %.23s", mp_member_cunt, mp_peer);

                statusline_cell[statusline_cunt] = statusline_mp;
                statusline_cunt = statusline_cunt + (uint32_t)CROSSTALK_STATUSLINE_CELL_STEP;
            }
            else
            {
                ldg_net_lobby_state_t sl_lobby = crosstalk_net_steam_lobby_state_get(&ctx->steam_net);

                if (sl_lobby == LDG_NET_LOBBY_STATE_HOSTING || sl_lobby == LDG_NET_LOBBY_STATE_JOINED)
                {
                    snprintf(statusline_mp, sizeof(statusline_mp), "LOBBY %u | %.15s", crosstalk_net_steam_lobby_member_cunt_get(&ctx->steam_net), ctx->mp_rdy != LDG_TRUTH_FALSE ? "ready" : "waiting");
                    statusline_cell[statusline_cunt] = statusline_mp;
                    statusline_cunt = statusline_cunt + (uint32_t)CROSSTALK_STATUSLINE_CELL_STEP;
                }
            }

            if (crosstalk_eda_ic_fpga_run_live_is(&ctx->eda.fpga_run) == LDG_TRUTH_TRUE)
            {
                uint64_t cyc_val = 0;
                uint64_t rate_val = 0;
                uint64_t loc_val = 0;
                uint32_t loc_width = 0;

                if (ctx->sim_run_since > 0.0 && ctx->time > ctx->sim_run_since)
                {
                    uint64_t run_secs = (uint64_t)(ctx->time - ctx->sim_run_since);
                    uint8_t emu_live = crosstalk_eda_ic_fpga_emu_live_is(&ctx->eda.fpga_emu);

                    if (emu_live == LDG_TRUTH_TRUE) { run_secs = (ctx->emu_stat.run_ms + ctx->emu_stat.hlt_ms) / (uint64_t)LDG_MS_PER_SEC; }

                    snprintf(statusline_simclk, sizeof(statusline_simclk), emu_live == LDG_TRUTH_TRUE ? CROSSTALK_STATUSLINE_EMUCLK_FMT : CROSSTALK_STATUSLINE_SIMCLK_FMT, run_secs / (uint64_t)LDG_SECS_PER_HOUR, (run_secs / (uint64_t)LDG_SECS_PER_MIN) % (uint64_t)LDG_SECS_PER_MIN, run_secs % (uint64_t)LDG_SECS_PER_MIN);

                    statusline_cell[statusline_cunt] = statusline_simclk;
                    statusline_cunt = statusline_cunt + (uint32_t)CROSSTALK_STATUSLINE_CELL_STEP;

                    if (emu_live == LDG_TRUTH_TRUE && ctx->emu_stat.cur < ctx->emu_stat.prog_cunt && statusline_cunt < (uint32_t)CROSSTALK_STATUSLINE_CELL_CAP)
                    {
                        uint64_t prog_secs = (ctx->emu_stat.prog[ctx->emu_stat.cur].run_ms + ctx->emu_stat.prog[ctx->emu_stat.cur].hlt_ms) / (uint64_t)LDG_MS_PER_SEC;

                        snprintf(statusline_prog, sizeof(statusline_prog), CROSSTALK_STATUSLINE_PROG_FMT, ctx->emu_stat.prog[ctx->emu_stat.cur].name, crosstalk_hdl_elf_base_get(&ctx->prog_elf), prog_secs / (uint64_t)LDG_SECS_PER_HOUR, (prog_secs / (uint64_t)LDG_SECS_PER_MIN) % (uint64_t)LDG_SECS_PER_MIN, prog_secs % (uint64_t)LDG_SECS_PER_MIN);

                        statusline_cell[statusline_cunt] = statusline_prog;
                        statusline_cunt = statusline_cunt + (uint32_t)CROSSTALK_STATUSLINE_CELL_STEP;
                    }
                }

                if (ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT].runnable == LDG_TRUTH_TRUE && ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT].clk_sig == UINT32_MAX && (ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT].abi == 0x0 || ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT].abi->ok != LDG_TRUTH_TRUE))
                {
                    snprintf(statusline_rate, sizeof(statusline_rate), "%.31s", CROSSTALK_STATUSLINE_RATE_COMB_TEXT);

                    statusline_cell[statusline_cunt] = statusline_rate;
                    statusline_cunt = statusline_cunt + (uint32_t)CROSSTALK_STATUSLINE_CELL_STEP;
                }
                else if (crosstalk_eda_ic_fpga_run_rate_get(&ctx->eda.fpga_run, &rate_val) == LDG_ERR_AOK && rate_val != 0)
                {
                    double rate_scaled = (double)rate_val;

                    if (rate_scaled >= CROSSTALK_STATUSLINE_CYC_GIGA) { snprintf(statusline_rate, sizeof(statusline_rate), CROSSTALK_STATUSLINE_RATE_FMT_SCALED, rate_scaled / CROSSTALK_STATUSLINE_CYC_GIGA, CROSSTALK_STATUSLINE_CYC_UNIT_GIGA); }
                    else if (rate_scaled >= CROSSTALK_STATUSLINE_CYC_MEGA) { snprintf(statusline_rate, sizeof(statusline_rate), CROSSTALK_STATUSLINE_RATE_FMT_SCALED, rate_scaled / CROSSTALK_STATUSLINE_CYC_MEGA, CROSSTALK_STATUSLINE_CYC_UNIT_MEGA); }
                    else if (rate_scaled >= CROSSTALK_STATUSLINE_CYC_KILO) { snprintf(statusline_rate, sizeof(statusline_rate), CROSSTALK_STATUSLINE_RATE_FMT_SCALED, rate_scaled / CROSSTALK_STATUSLINE_CYC_KILO, CROSSTALK_STATUSLINE_CYC_UNIT_KILO); }
                    else { snprintf(statusline_rate, sizeof(statusline_rate), CROSSTALK_STATUSLINE_RATE_FMT_RAW, rate_val); }

                    statusline_cell[statusline_cunt] = statusline_rate;
                    statusline_cunt = statusline_cunt + (uint32_t)CROSSTALK_STATUSLINE_CELL_STEP;
                }

                if (crosstalk_eda_ic_fpga_run_cycles_get(&ctx->eda.fpga_run, &cyc_val) == LDG_ERR_AOK)
                {
                    double cyc_scaled = (double)cyc_val;

                    if (cyc_scaled >= CROSSTALK_STATUSLINE_CYC_TERA) { snprintf(statusline_cyc, sizeof(statusline_cyc), CROSSTALK_STATUSLINE_CYC_FMT_SCALED, cyc_scaled / CROSSTALK_STATUSLINE_CYC_TERA, CROSSTALK_STATUSLINE_CYC_UNIT_TERA); }
                    else if (cyc_scaled >= CROSSTALK_STATUSLINE_CYC_GIGA) { snprintf(statusline_cyc, sizeof(statusline_cyc), CROSSTALK_STATUSLINE_CYC_FMT_SCALED, cyc_scaled / CROSSTALK_STATUSLINE_CYC_GIGA, CROSSTALK_STATUSLINE_CYC_UNIT_GIGA); }
                    else if (cyc_scaled >= CROSSTALK_STATUSLINE_CYC_MEGA) { snprintf(statusline_cyc, sizeof(statusline_cyc), CROSSTALK_STATUSLINE_CYC_FMT_SCALED, cyc_scaled / CROSSTALK_STATUSLINE_CYC_MEGA, CROSSTALK_STATUSLINE_CYC_UNIT_MEGA); }
                    else if (cyc_scaled >= CROSSTALK_STATUSLINE_CYC_KILO) { snprintf(statusline_cyc, sizeof(statusline_cyc), CROSSTALK_STATUSLINE_CYC_FMT_SCALED, cyc_scaled / CROSSTALK_STATUSLINE_CYC_KILO, CROSSTALK_STATUSLINE_CYC_UNIT_KILO); }
                    else { snprintf(statusline_cyc, sizeof(statusline_cyc), CROSSTALK_STATUSLINE_CYC_FMT_RAW, cyc_val); }

                    statusline_cell[statusline_cunt] = statusline_cyc;
                    statusline_cunt = statusline_cunt + (uint32_t)CROSSTALK_STATUSLINE_CELL_STEP;
                }

                if (crosstalk_eda_ic_fpga_sig_peek(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, CROSSTALK_EDA_IC_FPGA_PROG_SIG, &loc_val, &loc_width) == LDG_ERR_AOK)
                {
                    int32_t loc_hex_digits = (int32_t)(loc_width / (uint32_t)CROSSTALK_STATUSLINE_LOC_NIBBLE_DIV);
                    const char *loc_unit = 0x0;
                    uint64_t loc_shift = 0;
                    int32_t loc_off_digits = 0;

                    if (loc_val >= (uint64_t)LDG_GIB) { loc_unit = CROSSTALK_STATUSLINE_LOC_UNIT_GIB; loc_shift = (uint64_t)CROSSTALK_STATUSLINE_LOC_GIB_SHIFT; loc_off_digits = (int32_t)CROSSTALK_STATUSLINE_LOC_GIB_DIGITS; }
                    else if (loc_val >= (uint64_t)LDG_MIB) { loc_unit = CROSSTALK_STATUSLINE_LOC_UNIT_MIB; loc_shift = (uint64_t)CROSSTALK_STATUSLINE_LOC_MIB_SHIFT; loc_off_digits = (int32_t)CROSSTALK_STATUSLINE_LOC_MIB_DIGITS; }
                    else if (loc_val >= (uint64_t)LDG_KIB) { loc_unit = CROSSTALK_STATUSLINE_LOC_UNIT_KIB; loc_shift = (uint64_t)CROSSTALK_STATUSLINE_LOC_KIB_SHIFT; loc_off_digits = (int32_t)CROSSTALK_STATUSLINE_LOC_KIB_DIGITS; }

                    if (loc_unit == 0x0) { snprintf(statusline_loc, sizeof(statusline_loc), CROSSTALK_STATUSLINE_LOC_FMT_PLAIN, loc_hex_digits, loc_val); }
                    else { snprintf(statusline_loc, sizeof(statusline_loc), CROSSTALK_STATUSLINE_LOC_FMT_SCALED, loc_hex_digits, loc_val, loc_val >> loc_shift, loc_unit, loc_off_digits, loc_val & ((((uint64_t)CROSSTALK_STATUSLINE_CELL_STEP) << loc_shift) - (uint64_t)CROSSTALK_STATUSLINE_CELL_STEP)); }

                    statusline_cell[statusline_cunt] = statusline_loc;
                    statusline_cunt = statusline_cunt + (uint32_t)CROSSTALK_STATUSLINE_CELL_STEP;
                }

                if (ctx->eda.memview.watch_mode != (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WATCH_OFF && ctx->eda.memview.watch_query[0] != LDG_STR_TERM)
                {
                    statusline_cell[statusline_cunt] = ctx->eda.memview.watch_query;
                    statusline_cunt = statusline_cunt + (uint32_t)CROSSTALK_STATUSLINE_CELL_STEP;
                }

                if (crosstalk_reng_sel_get(&ctx->reng) != (uint32_t)UINT32_MAX)
                {
                    const crosstalk_reng_proj_t *proj_cur = 0x0;
                    const crosstalk_reng_sample_t *proj_last = 0x0;
                    uint32_t proj_verified = 0;
                    uint32_t proj_total = 0;

                    if (crosstalk_reng_proj_get(&ctx->reng, crosstalk_reng_sel_get(&ctx->reng), &proj_cur) == LDG_ERR_AOK && proj_cur != 0x0)
                    {
                        if (crosstalk_reng_sample_last_get(&ctx->reng, crosstalk_reng_sel_get(&ctx->reng), &proj_last) == LDG_ERR_AOK && proj_last != 0x0)
                        {
                            proj_verified = proj_last->verified;
                            proj_total = proj_last->total;
                        }

                        if (snprintf(statusline_proj, sizeof(statusline_proj), CROSSTALK_CORE_PROJ_STATUS_FMT, proj_cur->name, proj_verified, proj_total) > 0)
                        {
                            statusline_cell[statusline_cunt] = statusline_proj;
                            statusline_cunt = statusline_cunt + (uint32_t)CROSSTALK_STATUSLINE_CELL_STEP;
                        }
                    }
                }
            }

            {
                char cmdline_line[CROSSTALK_CORE_CMDLINE_CAP] = LDG_ARR_ZERO_INIT;

                if (crosstalk_core_cmdline_line_take(&ctx->cmdline, cmdline_line, (uint32_t)sizeof(cmdline_line)) == LDG_ERR_AOK) { crosstalk_script_line_exec(ctx, cmdline_line); }
            }

            if (render != 0x0 && screen_rect.row_cunt > desk_rect.row_cunt)
            {
                if (crosstalk_core_cmdline_open_is(&ctx->cmdline) == LDG_TRUTH_TRUE && crosstalk_eda_ic_fpga_build_live_is(&ctx->eda.fpga_build) != LDG_TRUTH_TRUE)
                {
                    ret = crosstalk_core_cmdline_draw(&ctx->cmdline, render, screen_rect.col, desk_rect.row_cunt, screen_rect.col_cunt);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: cmdline draw failed; err: %u32", ret); }
                }
                else
                {
                    ret = crosstalk_statusline_draw(render, desk_rect.row_cunt, screen_rect.col_cunt, statusline_cell, statusline_cunt);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: statusline draw failed; err: %u32", ret); }
                }
            }
        }
    }

    {
        crosstalk_hub_rows_sync(ctx);

        ret = crosstalk_game_music_tick(&ctx->music, ctx->dt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "music tick failed"); }

        if (crosstalk_game_music_poll_due_is(&ctx->music) == LDG_TRUTH_TRUE)
        {
            LDG_ST_RELEASE(ctx->music.done, CROSSTALK_GAME_MUSIC_DONE_CLEAR);
            ctx->music.inflight = LDG_TRUTH_TRUE;

            ret = ldg_thread_pool_submit(&ctx->game.music_pool, crosstalk_music_worker, ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
            {
                ctx->music.inflight = LDG_TRUTH_FALSE;
                ctx->music.poll_next = CROSSTALK_GAME_MUSIC_POLL_INTERVAL;
            }
        }

        if (render != 0x0 && ctx->game.screen != CROSSTALK_GAME_SCREEN_LD && screen_rect.row_cunt != 0 && screen_rect.col_cunt > (uint32_t)CROSSTALK_CORE_MUSIC_COL_CAP && crosstalk_core_cmdline_open_is(&ctx->cmdline) != LDG_TRUTH_TRUE)
        {
            ret = crosstalk_game_music_draw(&ctx->music, render, screen_rect.col_cunt - (uint32_t)CROSSTALK_CORE_MUSIC_COL_CAP, screen_rect.row_cunt - (uint32_t)CROSSTALK_STATUSLINE_ROWS, (uint32_t)CROSSTALK_CORE_MUSIC_COL_CAP);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "music draw failed"); }
        }
    }

    {
        crosstalk_game_totals_info_t totals_info = LDG_STRUCT_ZERO_INIT;

        if (ctx->game.totals.open == LDG_TRUTH_TRUE)
        {
            totals_info.board_name = ctx->board_name;
            totals_info.author_name = ctx->game.author_name;
            totals_info.emu_stat = &ctx->emu_stat;
            totals_info.created_epoch_ms = ctx->game.created_epoch_ms;
            totals_info.bench_ms_total = ctx->game.bench_ms_total;
            totals_info.sim_ms_total = ctx->game.sim_ms_total;
        }

        ret = crosstalk_game_totals_frame_set(&ctx->game.totals, &totals_info);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "totals frame set failed"); }
    }

    {
        ret = crosstalk_eda_ui_wshop_browser_frame_set(&ctx->eda.wshop_browser, &ctx->wshop_list, &ctx->wshop_core);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "wshop browser frame set failed"); }

        ret = crosstalk_eda_ui_wshop_publish_frame_set(&ctx->eda.wshop_publish, crosstalk_wshop_up_state_text_get(ctx->wshop_up_state));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "wshop publish frame set failed"); }

        ret = crosstalk_game_chat_frame_set(&ctx->chat, (uint8_t)(ctx->steam_net.sesh_live != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE), crosstalk_net_discord_live_is(&ctx->discord));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "chat frame set failed"); }

        if (render != 0x0 && ctx->game.screen != CROSSTALK_GAME_SCREEN_LD)
        {
            ret = crosstalk_ui_window_all_draw(&ctx->win_nav, render, &desk_rect);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "windows draw failed"); }

            ret = crosstalk_core_cmdline_drawer_overlay_draw(&ctx->cmdline, render);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cmdline drawer overlay draw failed"); }
        }

        if (crosstalk_eda_ui_feed_open_is(&ctx->eda.feed) == LDG_TRUTH_TRUE)
        {
            uint64_t fault[CROSSTALK_EDA_UI_FEED_FAULT_CUNT] = LDG_ARR_ZERO_INIT;

            if (crosstalk_eda_ic_fpga_fault_get(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, fault, (uint32_t)CROSSTALK_EDA_UI_FEED_FAULT_CUNT) == LDG_ERR_AOK) { if (crosstalk_eda_ui_feed_fault_set(&ctx->eda.feed, fault, (uint32_t)CROSSTALK_EDA_UI_FEED_FAULT_CUNT) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "feed fault set failed"); } }
        }

        if (crosstalk_eda_ui_stackview_open_is(&ctx->eda.stackview) == LDG_TRUTH_TRUE)
        {
            uint64_t fault[CROSSTALK_EDA_UI_STACKVIEW_FAULT_CUNT] = LDG_ARR_ZERO_INIT;
            char sig[CROSSTALK_EDA_UI_STACKVIEW_SIG_CAP] = LDG_ARR_ZERO_INIT;
            uint32_t kind = 0;

            if (crosstalk_eda_ic_fpga_fault_get(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, fault, (uint32_t)CROSSTALK_EDA_UI_STACKVIEW_FAULT_CUNT) == LDG_ERR_AOK) { if (crosstalk_eda_ui_stackview_fault_set(&ctx->eda.stackview, fault, (uint32_t)CROSSTALK_EDA_UI_STACKVIEW_FAULT_CUNT) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "stackview fault set failed"); } }

            if (crosstalk_eda_ui_stackview_req_get(&ctx->eda.stackview, sig, (uint64_t)sizeof(sig), &kind) == LDG_ERR_AOK)
            {
                if (kind == (uint32_t)CROSSTALK_EDA_UI_STACKVIEW_REQ_ADD) { crosstalk_stackview_lane_attach(ctx, sig); }
                else if (kind == (uint32_t)CROSSTALK_EDA_UI_STACKVIEW_REQ_REMOVE) { crosstalk_stackview_lane_release(ctx, sig); }

                if (crosstalk_eda_ui_stackview_req_clear(&ctx->eda.stackview) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "stackview req clear failed"); }
            }
        }

        {
            uint64_t fault[CROSSTALK_HDL_ABI_FAULT_CUNT] = LDG_ARR_ZERO_INIT;

            if (crosstalk_eda_ic_fpga_fault_get(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, fault, (uint32_t)CROSSTALK_HDL_ABI_FAULT_CUNT) == LDG_ERR_AOK && fault[CROSSTALK_HDL_ABI_FAULT_VALID_AT] != 0) { if (ctx->event_fault_has != LDG_TRUTH_TRUE || fault[CROSSTALK_HDL_ABI_FAULT_STEP_AT] != ctx->event_fault_step)
                {
                    ctx->event_fault_step = fault[CROSSTALK_HDL_ABI_FAULT_STEP_AT];
                    ctx->event_fault_has = LDG_TRUTH_TRUE;
                    crosstalk_ach_once(ctx, CROSSTALK_GAME_PROGRESSION_FIRST_FAULT);
                    crosstalk_event_fire(ctx, (uint32_t)CROSSTALK_EDA_SCRIPT_EVENT_FAULT);
                    crosstalk_defer_drain(ctx);
                }
            }
        }

        if (ctx->serial_map_pending_has == LDG_TRUTH_TRUE)
        {
            ctx->serial_map_pending_has = LDG_TRUTH_FALSE;

            crosstalk_script_line_exec(ctx, ctx->serial_map_pending);
        }

        ret = crosstalk_eda_ui_profile_tick(&ctx->eda.profile, &ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, ctx->time);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "profile tick failed"); }

        crosstalk_scope_rebind_tick(ctx);

        crosstalk_prog_pub_poll(ctx);

        crosstalk_relay_poll(ctx);

        if (crosstalk_eda_ui_scope_mem_live_is(&ctx->eda.scope) == LDG_TRUTH_TRUE) { if (crosstalk_eda_ui_scope_mem_capture(&ctx->eda.scope, &ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, ctx->eda.memview.bound == LDG_TRUTH_TRUE ? ctx->eda.memview.sig : CROSSTALK_CORE_MEMVIEW_SIG_DEFAULT) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scope mem capture failed"); } }

        if (crosstalk_eda_ui_memview_open_is(&ctx->eda.memview) == LDG_TRUTH_TRUE)
        {
            uint64_t watch_addr = 0;
            uint8_t watch_mut = LDG_TRUTH_FALSE;
            uint8_t watch_hlt = LDG_TRUTH_FALSE;

            {
                char want[CROSSTALK_EDA_UI_MEMVIEW_TITLE_CAP] = LDG_ARR_ZERO_INIT;
                const char *arch = 0x0;

                if (crosstalk_hdl_elf_arch_get(&ctx->prog_elf, &arch) != LDG_ERR_AOK && crosstalk_hdl_elf_arch_get(&ctx->elf, &arch) != LDG_ERR_AOK) { arch = 0x0; }

                if (arch != 0x0) { snprintf(want, sizeof(want), CROSSTALK_CORE_MEMVIEW_TITLE_ARCH_FMT, CROSSTALK_EDA_UI_MEMVIEW_TITLE, arch); }
                else { snprintf(want, sizeof(want), CROSSTALK_CORE_MEMVIEW_TITLE_PLAIN_FMT, CROSSTALK_EDA_UI_MEMVIEW_TITLE); }

                if (strncmp(want, ctx->memview_title, (uint64_t)sizeof(want)) != 0) { if (ldg_str_rbrcpy(ctx->memview_title, want, (uint64_t)sizeof(ctx->memview_title)) == LDG_ERR_AOK && crosstalk_ui_window_title_set(&ctx->win_nav, (uint64_t)CROSSTALK_EDA_UI_MEMVIEW_WINDOW_ID, ctx->memview_title) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview title sync failed"); } }
            }

            crosstalk_memview_freeze_step(ctx);

            ret = crosstalk_eda_ui_memview_tick(&ctx->eda.memview, &ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, ctx->time);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview tick failed"); }

            if ((ctx->eda.memview.watch_hlt == LDG_TRUTH_TRUE || ctx->time >= ctx->memwatch_notice_next) && crosstalk_eda_ui_memview_watch_hit_get(&ctx->eda.memview, &watch_addr, &watch_mut, &watch_hlt) == LDG_ERR_AOK)
            {
                char watch_notice[CROSSTALK_CORE_MEMWATCH_NOTICE_CAP] = LDG_ARR_ZERO_INIT;
                int32_t watch_wrote = 0;

                ctx->memwatch_notice_next = ctx->time + CROSSTALK_CORE_MEMWATCH_NOTIFY_SECS;

                if (watch_hlt == LDG_TRUTH_TRUE)
                {
                    if (crosstalk_fpga_hlt(ctx) != LDG_TRUTH_TRUE) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview break hit; clock already stopped"); }

                    if (crosstalk_eda_ui_memview_dis_loc_req(&ctx->eda.memview, (uint32_t)CROSSTALK_CORE_MEMBREAK_DIS_CUNT) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview break dis req failed"); }

                    if (crosstalk_eda_ui_memview_hlt_mark_set(&ctx->eda.memview, watch_addr, watch_addr) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview hlt mark failed"); }

                    watch_wrote = snprintf(watch_notice, sizeof(watch_notice), CROSSTALK_CORE_MEMBREAK_FMT, watch_addr);
                }
                else if (watch_mut == LDG_TRUTH_TRUE) { watch_wrote = snprintf(watch_notice, sizeof(watch_notice), CROSSTALK_CORE_MEMWATCH_MUT_FMT, watch_addr); }
                else { watch_wrote = snprintf(watch_notice, sizeof(watch_notice), CROSSTALK_CORE_MEMWATCH_HIT_FMT, watch_addr); }

                if (watch_wrote > 0) { crosstalk_notice_raise(ctx, CROSSTALK_CORE_NOTIFY_TITLE_SESH, watch_notice); }
            }

            {
                char mv_arg[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
                uint32_t mv_action = (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ACTION_NONE;

                if (crosstalk_eda_ui_memview_action_get(&ctx->eda.memview, &mv_action, mv_arg, (uint32_t)sizeof(mv_arg)) == LDG_ERR_AOK)
                {
                    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview: action; kind: %u32; arg: %.63s", mv_action, mv_arg);

                    if (mv_action == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ACTION_STEP)
                    {
                        char step_note[CROSSTALK_CORE_SOUND_ARG_CAP] = LDG_ARR_ZERO_INIT;
                        uint64_t step_cunt = (uint64_t)CROSSTALK_CORE_MEMDBG_STEP_DEFAULT;
                        uint64_t step_done = 0;

                        if (mv_arg[0] != LDG_STR_TERM && crosstalk_num_parse(mv_arg, &step_cunt) != LDG_ERR_AOK) { step_cunt = (uint64_t)CROSSTALK_CORE_MEMDBG_STEP_DEFAULT; }

                        if (crosstalk_core_hlt_is(ctx) != LDG_TRUTH_TRUE) { if (crosstalk_core_hlt_set(ctx, LDG_TRUTH_TRUE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview step halt failed"); }
                            else { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview step; clock stopped first"); } }

                        ret = crosstalk_eda_ic_fpga_cycles_run(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, step_cunt, &step_done);
                        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview: step refused; cycles: %u64; err: %u32", step_cunt, ret); }
                        else
                        {
                            ctx->eda.memview.refresh_next = 0.0;

                            if (crosstalk_eda_ui_memview_dis_loc_req(&ctx->eda.memview, (uint32_t)CROSSTALK_CORE_MEMDBG_STEP_DIS_CUNT) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview step dis req failed"); }

                            if (snprintf(step_note, sizeof(step_note), CROSSTALK_CORE_MEMDBG_STEP_FMT, step_done) > 0) { toast_push(ctx, step_note); }
                        }
                    }
                    else if (mv_action == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ACTION_HLT)
                    {
                        if (crosstalk_core_hlt_is(ctx) == LDG_TRUTH_TRUE) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "memview hlt; clock already stopped"); }

                        if (crosstalk_core_hlt_set(ctx, LDG_TRUTH_TRUE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview hlt failed"); }

                        toast_push(ctx, CROSSTALK_CORE_MEMDBG_HLT_TEXT);
                    }
                    else if (mv_action == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ACTION_RUN)
                    {
                        if (crosstalk_core_hlt_set(ctx, LDG_TRUTH_FALSE) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "memview run failed"); }

                        toast_push(ctx, CROSSTALK_CORE_MEMDBG_RUN_TEXT);
                    }
                    else if (mv_action == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ACTION_WATCH) { crosstalk_memview_watch(ctx, mv_arg); }
                    else if (mv_action == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ACTION_BREAK) { if (crosstalk_memview_break(ctx, mv_arg) == LDG_ERR_UNSUPPORTED)
                        {
                            crosstalk_memview_watch(ctx, mv_arg);
                            if (ctx->eda.memview.watch_mode != (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_WATCH_OFF) { ctx->eda.memview.watch_hlt = LDG_TRUTH_TRUE; }
                        }
                    }
                    else if (mv_action == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ACTION_POKE) { crosstalk_memview_poke(ctx, mv_arg); }
                    else if (mv_action == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ACTION_FREEZE) { crosstalk_memview_freeze(ctx, mv_arg); }
                    else if (mv_action == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ACTION_SNAP) { crosstalk_memview_bake_mount(ctx, mv_arg, LDG_TRUTH_FALSE); }
                    else if (mv_action == (uint32_t)CROSSTALK_EDA_UI_MEMVIEW_ACTION_DIFF) { crosstalk_memview_bake_mount(ctx, mv_arg, LDG_TRUTH_TRUE); }
                }
            }
        }

        if (ctx->eda.crt.open == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_eda_ui_display_crt_text_tick(&ctx->eda.crt, &ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, ctx->time);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crt text tick failed"); }

            ret = crosstalk_eda_ui_display_crt_gfx_tick(&ctx->eda.crt, &ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, ctx->time);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crt gfx tick failed"); }
        }

        {
            uint8_t gfx_on = crosstalk_emu_gfx_is(ctx);
            uint64_t step_want = gfx_on == LDG_TRUTH_TRUE ? (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_STEP_GFX : (uint64_t)CROSSTALK_EDA_IC_FPGA_EMU_STEP_TEXT;

            if (crosstalk_eda_ic_fpga_emu_step_set(&ctx->eda.fpga, step_want) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu step set failed"); }

            if (step_want != ctx->emu_step_on)
            {
                ctx->emu_step_on = step_want;

                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu: stepping follows gfx mode; gfx: %u32; src: %u32; step: %u64", (uint32_t)gfx_on, ctx->eda.crt.src, step_want);

                if (gfx_on == LDG_TRUTH_TRUE && ctx->prof_en_on == LDG_TRUTH_TRUE)
                {
                    ret = crosstalk_eda_ic_fpga_prof_en(&ctx->eda.fpga, CROSSTALK_CORE_TERM_SLOT, LDG_TRUTH_FALSE);

                    if (ret == LDG_ERR_AOK)
                    {
                        ctx->prof_en_on = LDG_TRUTH_FALSE;

                        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "emu: instruction ring off; gfx mode took the slot");
                    }
                    else if (ret != LDG_ERR_NOT_INIT) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "emu: instruction ring off failed; err: %u32", ret); }
                }
            }
        }
    }

    if (ctx->eda.wshop_browser.open == LDG_TRUTH_TRUE)
    {
        const crosstalk_wshop_item_t *mount_req = 0x0;
        uint8_t share_req = LDG_TRUTH_FALSE;

        ret = crosstalk_eda_ui_wshop_browser_out_get(&ctx->eda.wshop_browser, &mount_req, &share_req);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "wshop browser out get failed"); }

        if (mount_req != 0x0) { crosstalk_wshop_mount_apply(ctx, mount_req); }

        if (share_req == LDG_TRUTH_TRUE)
        {
            ret = crosstalk_eda_ui_wshop_publish_open_set(&ctx->eda.wshop_publish, LDG_TRUTH_TRUE, ctx->eda.fpga.slot[CROSSTALK_CORE_TERM_SLOT].top);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "wshop publish open failed"); }
        }
    }

    if (ctx->eda.wshop_publish.open == LDG_TRUTH_TRUE)
    {
        uint8_t go = LDG_TRUTH_FALSE;

        ret = crosstalk_eda_ui_wshop_publish_out_get(&ctx->eda.wshop_publish, &go);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "wshop publish out get failed"); }

        if (go == LDG_TRUTH_TRUE) { crosstalk_wshop_publish(ctx, ctx->eda.wshop_publish.title, crosstalk_eda_ui_wshop_publish_isa_get(&ctx->eda.wshop_publish), ctx->eda.wshop_publish.license); }
    }

    {
        char chat_text[CROSSTALK_GAME_CHAT_TEXT_CAP] = LDG_ARR_ZERO_INIT;
        uint8_t chat_send = LDG_TRUTH_FALSE;

        if (ctx->game.screen == CROSSTALK_GAME_SCREEN_MP && ctx->chat.win.open == LDG_TRUTH_FALSE)
        {
            ldg_net_lobby_state_t chat_lobby = crosstalk_net_steam_lobby_state_get(&ctx->steam_net);

            if (chat_lobby == LDG_NET_LOBBY_STATE_HOSTING || chat_lobby == LDG_NET_LOBBY_STATE_JOINED)
            {
                crosstalk_chat_dock(ctx);
                ret = crosstalk_game_chat_open_set(&ctx->chat, LDG_TRUTH_TRUE);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "lobby chat open failed"); }
            }
        }

        ret = crosstalk_game_chat_out_get(&ctx->chat, chat_text, (uint32_t)sizeof(chat_text), &chat_send);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "chat draw failed"); }

        if (crosstalk_game_hub_open_is(&ctx->hub) == LDG_TRUTH_TRUE)
        {
            crosstalk_game_hub_action_t hub_action = CROSSTALK_GAME_HUB_ACTION_NONE;
            char hub_name[CROSSTALK_GAME_HUB_NAME_CAP] = LDG_ARR_ZERO_INIT;
            uint64_t hub_user = 0;
            uint64_t hub_lobby = 0;
            uint8_t hub_src = 0;

            crosstalk_hub_fit(ctx);

            ret = crosstalk_game_hub_out_get(&ctx->hub, &hub_action, &hub_user, &hub_lobby, &hub_src, hub_name, (uint32_t)sizeof(hub_name));
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "hub out get failed; err: %u32", ret); }

            if (hub_action == CROSSTALK_GAME_HUB_ACTION_JOIN && hub_lobby != 0)
            {
                ret = lobby_join_enter(ctx, hub_lobby);

                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "hub: join; lobby: %u64; err: %u32", hub_lobby, ret);
                if (ret != LDG_ERR_AOK) { toast_push(ctx, "could not join that session"); }
            }
            else if (hub_action != CROSSTALK_GAME_HUB_ACTION_NONE) { crosstalk_hub_action_apply(ctx, hub_action, hub_user, hub_name, hub_src); }
        }

        if (chat_send == LDG_TRUTH_TRUE && chat_text[0] != LDG_STR_TERM)
        {
            char self_name[CROSSTALK_GAME_CHAT_NAME_CAP] = LDG_ARR_ZERO_INIT;
            uint64_t chat_peer = 0;

            ctx->game.progression.chat_sent = LDG_TRUTH_TRUE;

            if (ctx->chat.group == (uint8_t)CROSSTALK_GAME_CHAT_GROUP_DISCORD)
            {
                const char *discord_self = crosstalk_net_discord_user_name_get(&ctx->discord);

                ret = (discord_self != 0x0 && discord_self[0] != LDG_STR_TERM) ? ldg_str_rbrcpy(self_name, discord_self, (uint64_t)sizeof(self_name)) : LDG_ERR_NOT_FOUND;
            }
            else { ret = crosstalk_persona_resolve(ctx, ldg_net_steamworks_local_steam_id_get(), self_name, (uint64_t)sizeof(self_name)); }

            if ((ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC) || self_name[0] == LDG_STR_TERM) { ret = ldg_str_rbrcpy(self_name, "you", (uint64_t)sizeof(self_name)); }

            chat_peer = crosstalk_game_chat_peer_get(&ctx->chat, ctx->chat.group);

            if (ctx->chat.group == (uint8_t)CROSSTALK_GAME_CHAT_GROUP_STEAM)
            {
                if (chat_peer == CROSSTALK_GAME_CHAT_PEER_NONE) { toast_push(ctx, "pick a Steam friend in the hub first"); }
                else
                {
                    ret = crosstalk_net_steam_friend_msg_reply(chat_peer, chat_text);
                    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "steam chat: sent; to: %u64; err: %u32", chat_peer, ret);
                    if (ret == LDG_ERR_AOK)
                    {
                        ret = crosstalk_game_chat_push(&ctx->chat, (uint8_t)CROSSTALK_GAME_CHAT_GROUP_STEAM, chat_peer, 0x0, self_name, chat_text);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "steam chat echo push failed"); }
                    }
                    else { toast_push(ctx, "Steam refused the message"); }
                }
            }
            else if (ctx->chat.group == (uint8_t)CROSSTALK_GAME_CHAT_GROUP_DISCORD && chat_peer == CROSSTALK_GAME_CHAT_PEER_NONE)
            {
                ret = crosstalk_net_discord_chat_send(&ctx->discord, chat_text);
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "discord chat: sent; err: %u32", ret);

                if (ret == LDG_ERR_AOK)
                {
                    ret = crosstalk_game_chat_push(&ctx->chat, (uint8_t)CROSSTALK_GAME_CHAT_GROUP_DISCORD, CROSSTALK_GAME_CHAT_PEER_NONE, CROSSTALK_CORE_CHAT_LOBBY_NAME, self_name, chat_text);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "discord chat echo push failed"); }
                }
                else { toast_push(ctx, "not in a Discord lobby yet"); }
            }
            else if (ctx->chat.group == (uint8_t)CROSSTALK_GAME_CHAT_GROUP_DISCORD)
            {
                if (strcmp(chat_text, CROSSTALK_CORE_DISCORD_INVITE_CMD) == 0)
                {
                    ret = crosstalk_net_discord_invite_send(&ctx->discord, chat_peer, CROSSTALK_CORE_DISCORD_INVITE_TEXT);
                    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "discord: invite send; to: %u64; err: %u32", chat_peer, ret);

                    if (ret == LDG_ERR_AOK) { toast_push(ctx, "Discord invite sent"); }
                    else if (ret == LDG_ERR_INVALID) { toast_push(ctx, "host a joinable lobby first"); }
                    else { toast_push(ctx, "Discord refused the invite"); }
                }
                else
                {
                    ret = crosstalk_net_discord_dm_send(&ctx->discord, chat_peer, chat_text);
                    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "discord dm: sent; to: %u64; err: %u32", chat_peer, ret);

                    if (ret == LDG_ERR_AOK)
                    {
                        ret = crosstalk_game_chat_push(&ctx->chat, (uint8_t)CROSSTALK_GAME_CHAT_GROUP_DISCORD, chat_peer, 0x0, self_name, chat_text);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "discord dm echo push failed"); }
                    }
                    else { toast_push(ctx, "Discord refused the message"); }
                }
            }
            else
            {
                crosstalk_net_wire_chat_t wire_chat = LDG_STRUCT_ZERO_INIT;

                ret = crosstalk_game_chat_push(&ctx->chat, (uint8_t)CROSSTALK_GAME_CHAT_GROUP_SESH, CROSSTALK_GAME_CHAT_PEER_NONE, CROSSTALK_CORE_CHAT_SESH_NAME, self_name, chat_text);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "chat local push failed"); }

                if (ctx->steam_net.sesh_live != 0)
                {
                    ret = ldg_str_rbrcpy(wire_chat.text, chat_text, (uint64_t)sizeof(wire_chat.text));
                    if (ret == LDG_ERR_AOK || ret == LDG_ERR_STR_TRUNC) { ret = crosstalk_net_wire_send(&ctx->steam_net, CROSSTALK_NET_WIRE_KIND_CHAT, &wire_chat, (uint32_t)sizeof(wire_chat), CROSSTALK_NET_WIRE_RELIABLE); }

                    if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "net: chat send failed; err: %u32", ret); }
                    else
                    {
                        ret = crosstalk_game_progression_achievement_unlock_req(CROSSTALK_GAME_PROGRESSION_FIRST_TRANSMISSION, CROSSTALK_APP_ID, &ctx->progression_queue);
                        if (ret != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_DEBUG, "crosstalk: ach first_transmission; err: %u32", ret); }
                    }
                }
            }
        }
    }

    if (ctx->game.paused != LDG_TRUTH_FALSE && ctx->settings_open == LDG_TRUTH_FALSE)
    {
        const char *pause_quit_text = ctx->quit_armed == LDG_TRUTH_TRUE ? CROSSTALK_CORE_QUIT_ARMED_TEXT : "Quit";
        const char *pause_item[CROSSTALK_PAUSE_SWITCH_ITEM_CUNT] = { "Resume", "Settings", ctx->mp_busy != LDG_TRUTH_FALSE ? "To lobby" : pause_quit_text };
        uint32_t pause_idx = 0;

        if (render != 0x0)
        {
            ret = crosstalk_menu_switch_draw(render, &screen_rect, CROSSTALK_PAUSE_SWITCH_CAPTION, pause_item, 0x0, (uint32_t)CROSSTALK_PAUSE_SWITCH_ITEM_CUNT, ctx->menu_sel, LDG_TRUTH_TRUE);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pause switch draw failed"); }
        }

        if (crosstalk_menu_confirm_take(ctx, 0x0, (uint32_t)CROSSTALK_PAUSE_SWITCH_ITEM_CUNT, &pause_idx) == LDG_ERR_AOK)
        {
            if (pause_idx == CROSSTALK_PAUSE_ITEM_RESUME) { ctx->game.paused = LDG_TRUTH_FALSE; ctx->quit_armed = LDG_TRUTH_FALSE; }
            else if (pause_idx == CROSSTALK_PAUSE_ITEM_SETTINGS)
            {
                ctx->settings_open = LDG_TRUTH_TRUE;

                ret = crosstalk_game_settings_menu_init(&ctx->settings_menu);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "settings menu rst failed"); }
            }
            else if (ctx->mp_busy != LDG_TRUTH_FALSE)
            {
                ret = crosstalk_autosave_wr(ctx);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_NOT_INIT && ret != LDG_ERR_DENIED)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "lobby exit autosave failed"); }

                ctx->mp_busy = LDG_TRUTH_FALSE;
                crosstalk_net_steam_lobby_busy_set(&ctx->steam_net, LDG_TRUTH_FALSE);
                ctx->game.paused = LDG_TRUTH_FALSE;
                ctx->game.screen = CROSSTALK_GAME_SCREEN_MP;
                ctx->menu_sel = (uint32_t)CROSSTALK_MENU_SEL_FIRST;
            }
            else if (crosstalk_quit_stale_is(ctx) == LDG_TRUTH_TRUE && ctx->quit_armed != LDG_TRUTH_TRUE)
            {
                ctx->quit_armed = LDG_TRUTH_TRUE;
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: pause quit held; last save: %u64 ms", ctx->st_time_ms);
                toast_push(ctx, CROSSTALK_CORE_QUIT_PAUSE_TEXT);
            }
            else { crosstalk_menu_quit(ctx); }
        }
    }

    if (ctx->settings_open != LDG_TRUTH_FALSE)
    {
        uint8_t settings_link_action = CROSSTALK_GAME_SETTINGS_LINK_NONE;
        uint8_t settings_changed = LDG_TRUTH_FALSE;
        uint8_t settings_linked = crosstalk_net_discord_linked_is(&ctx->discord) == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

        if (crosstalk_net_discord_auth_get(&ctx->discord) == CROSSTALK_NET_DISCORD_AUTH_PENDING) { settings_linked = LDG_TRUTH_TRUE; }

        if (render != 0x0)
        {
            ret = crosstalk_game_settings_menu_draw(&ctx->settings_menu, &ctx->game.settings, settings_linked, render, &screen_rect);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "settings menu draw failed"); }
        }

        ret = crosstalk_game_settings_menu_out_get(&ctx->settings_menu, &settings_link_action, &settings_changed);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "settings menu out get failed"); }

        if (settings_changed != LDG_TRUTH_FALSE)
        {
            if (ctx->game.settings.fab_interlock_off != ctx->settings_applied.fab_interlock_off) { if (crosstalk_game_fx_sfx_lever_play(&ctx->game.sfx_sound) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "lever sfx failed"); } }
            else if (crosstalk_game_fx_sfx_rotary_play(&ctx->game.sfx_sound, CROSSTALK_GAME_FX_SFX_ROTARY_CW) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "rotary sfx failed"); }

            settings_apply(ctx);
            crosstalk_discord_settings_apply(ctx);
        }

        if (settings_link_action == CROSSTALK_GAME_SETTINGS_LINK_DO)
        {
            if (crosstalk_net_discord_linked_is(&ctx->discord) == LDG_TRUTH_TRUE)
            {
                ctx->game.settings.discord_link_on = LDG_TRUTH_TRUE;
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: link switch ignored; already linked");
                toast_push(ctx, "the Discord account is already linked");
            }
            else if ((ret = crosstalk_net_discord_auth_restore(&ctx->discord)) == LDG_ERR_AOK)
            {
                ctx->game.settings.discord_link_on = LDG_TRUTH_TRUE;
                ctx->discord_restore_req = LDG_TRUTH_TRUE;
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: link switch used stored token");
                toast_push(ctx, "restoring the Discord account");
            }
            else
            {
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "discord: no stored token; authorizing; err: %u32", ret);
                ret = crosstalk_net_discord_auth_link_local(&ctx->discord);
                ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "discord: account link req; err: %u32", ret);

                if (ret == LDG_ERR_AOK)
                {
                    ctx->game.settings.discord_link_on = LDG_TRUTH_TRUE;
                    toast_push(ctx, "check Discord to approve the link");
                }
                else { toast_push(ctx, "Discord link unavailable right now"); }
            }
        }

        if (settings_link_action == CROSSTALK_GAME_SETTINGS_LINK_UNDO)
        {
            ret = crosstalk_net_discord_auth_unlink(&ctx->discord);

            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "discord: account unlink req; err: %u32", ret);
            ctx->game.settings.discord_link_on = LDG_TRUTH_FALSE;
            ctx->discord_restore_req = LDG_TRUTH_FALSE;
        }
    }

    if (crosstalk_eda_ic_fpga_build_live_is(&ctx->eda.fpga_build) == LDG_TRUTH_TRUE || ctx->persist_pending == LDG_TRUTH_TRUE || ctx->re_diff_pending != 0 || crosstalk_re_cc_live_is(&ctx->re.cc) == LDG_TRUTH_TRUE)
    {
        const char *busy_line[CROSSTALK_CORE_BUSY_ROW_CUNT] = LDG_ARR_ZERO_INIT;
        const char *busy_status = 0x0;
        const char *busy_title = ctx->persist_pending == LDG_TRUTH_TRUE ? CROSSTALK_CORE_PERSIST_TITLE : CROSSTALK_CORE_BUSY_TITLE;
        const char *busy_body = ctx->persist_pending == LDG_TRUTH_TRUE ? CROSSTALK_CORE_PERSIST_BODY_TEXT : CROSSTALK_CORE_BUSY_BODY_TEXT;

        if (ctx->re_diff_pending != 0)
        {
            busy_title = CROSSTALK_CORE_RE_BUSY_TITLE;
            busy_body = CROSSTALK_CORE_RE_BUSY_BODY_TEXT;
        }

        if (crosstalk_re_cc_live_is(&ctx->re.cc) == LDG_TRUTH_TRUE && crosstalk_re_cc_status_get(&ctx->re.cc, &busy_status) != LDG_ERR_AOK) { busy_status = 0x0; }

        if (ctx->persist_pending != LDG_TRUTH_TRUE && crosstalk_eda_ic_fpga_build_live_is(&ctx->eda.fpga_build) == LDG_TRUTH_TRUE && crosstalk_eda_ic_fpga_build_status_get(&ctx->eda.fpga_build, &busy_status) != LDG_ERR_AOK) { busy_status = 0x0; }

        busy_line[CROSSTALK_CORE_BUSY_LINE_TITLE] = busy_title;
        busy_line[CROSSTALK_CORE_BUSY_LINE_STATUS] = busy_status;
        busy_line[CROSSTALK_CORE_BUSY_LINE_BODY] = busy_body;
        busy_line[CROSSTALK_CORE_BUSY_LINE_FREEZE] = CROSSTALK_CORE_BUSY_FREEZE_TEXT;

        if (render != 0x0)
        {
            ret = crosstalk_busy_draw(render, &screen_rect, busy_line, (uint32_t)CROSSTALK_CORE_BUSY_ROW_CUNT);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "busy draw failed"); }
        }
    }

    if (render != 0x0)
    {
        ret = crosstalk_game_caption_draw(&ctx->game.caption, render, &screen_rect);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_NOT_INIT)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "caption draw failed"); }
    }

    if (ctx->splash_cw_rolled != LDG_TRUTH_TRUE)
    {
        uint8_t cw_sent = LDG_TRUTH_FALSE;

        ctx->splash_cw_rolled = LDG_TRUTH_TRUE;
        ret = crosstalk_game_fx_sfx_cw_start(&ctx->game.sfx_sound, CROSSTALK_CORE_SPLASH_CW_TEXT, CROSSTALK_CORE_SPLASH_CW_PROBABILITY, &cw_sent);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: splash cw failed; err: %u32", ret); }

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: splash cw; sent: %u32", cw_sent);
    }

    if (render != 0x0)
    {
        ret = ldg_term_sesh_wr(ctx->sesh);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term sesh write failed"); }

        if (ctx->shot.pending == LDG_TRUTH_TRUE) { crosstalk_shot_take(ctx, render, &desk_rect); }
    }

    if (ctx->game.ld_active != 0 && ctx->game.ld_finalized == 0 && ctx->ld_frames >= (uint8_t)CROSSTALK_CORE_LD_FRAME_MIN && LDG_LD_ACQUIRE(ctx->game.ld.done) != 0) { ret = ld_finalize(ctx); }
}

uint32_t crosstalk_core_run(crosstalk_ctx_t *ctx)
{
    ldg_sched_event_desc_t fixed_event = LDG_STRUCT_ZERO_INIT;
    ldg_sched_event_desc_t step_event = LDG_STRUCT_ZERO_INIT;
    uint32_t sub_id = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_sched_ctx_phase_event_get(&ctx->sched, LDG_SCHED_PHASE_FIXED_STEP, &fixed_event);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->fixed_event = fixed_event;

    ret = ldg_sched_ctx_phase_event_get(&ctx->sched, LDG_SCHED_PHASE_STEP, &step_event);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->step_event = step_event;

    ret = ldg_sched_ctx_event_sub(&ctx->sched, fixed_event, crosstalk_tick, LDG_SCHED_EVENT_FILTER_ALL, 0, ctx, &sub_id);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_sched_ctx_event_sub(&ctx->sched, step_event, crosstalk_tick, LDG_SCHED_EVENT_FILTER_ALL, 0, ctx, &sub_id);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: entering spin");

    return ldg_sched_ctx_spin(&ctx->sched);
}

static uint64_t crosstalk_shutdown_mark(const char *stage, uint64_t prev)
{
    uint64_t now = ldg_time_epoch_ns_get();

    if (LDG_UNLIKELY(stage == 0x0)) { return now; }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "crosstalk: shutdown stage; name: %s; ms: %u64", stage, (now - prev) / LDG_NS_PER_MS);
    return now;
}

uint32_t crosstalk_core_destroy(crosstalk_ctx_t *ctx)
{
    uint64_t mark = ldg_time_epoch_ns_get();
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(crosstalk_core_cmdline_drawer_shutdown(&ctx->cmdline) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "cmdline drawer shutdown failed"); }

    mark = crosstalk_shutdown_mark("cmdline", mark);

    if (ctx->watch_ok == LDG_TRUTH_TRUE)
    {
        LDG_ST_RELEASE(ctx->watch_stop, LDG_TRUTH_TRUE);

        if (LDG_UNLIKELY(ldg_thread_pool_stop(&ctx->watch_pool) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "watch pool stop failed"); }

        if (LDG_UNLIKELY(ldg_thread_pool_shutdown(&ctx->watch_pool) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "watch pool shutdown failed"); }

        if (LDG_UNLIKELY(ldg_mut_destroy(&ctx->watch_mut) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "watch mutex destroy failed"); }

        ctx->watch_ok = LDG_TRUTH_FALSE;
    }

    mark = crosstalk_shutdown_mark("watch", mark);

    if (LDG_UNLIKELY(crosstalk_eda_ic_fpga_run_shutdown(&ctx->eda.fpga_run) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga run shutdown failed"); }

    mark = crosstalk_shutdown_mark("fpga_run", mark);

    if (LDG_UNLIKELY(crosstalk_eda_ui_progview_shutdown(&ctx->eda.progview) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "progview shutdown failed"); }

    if (LDG_UNLIKELY(crosstalk_hdl_elf_shutdown(&ctx->elf) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "elf shutdown failed"); }

    if (LDG_UNLIKELY(crosstalk_hdl_elf_shutdown(&ctx->prog_elf) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "prog elf shutdown failed"); }

    if (LDG_UNLIKELY(crosstalk_hdl_type_shutdown(&ctx->prog_type) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "prog type shutdown failed"); }

    if (LDG_UNLIKELY(crosstalk_re_shutdown(&ctx->re) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re shutdown failed"); }

    if (LDG_UNLIKELY(crosstalk_reng_shutdown(&ctx->reng) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "reng shutdown failed"); }

    if (ctx->reng_sect_buff != 0x0)
    {
        ldg_mem_dealloc(ctx->reng_sect_buff);
        ctx->reng_sect_buff = 0x0;
        ctx->reng_sect_len = 0;
    }

    if (LDG_UNLIKELY(crosstalk_eda_ic_fpga_emu_shutdown(&ctx->eda.fpga_emu) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga emu shutdown failed"); }

    if (LDG_UNLIKELY(crosstalk_eda_ic_fpga_shutdown(&ctx->eda.fpga) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga shutdown failed"); }

    mark = crosstalk_shutdown_mark("fpga", mark);

    if (LDG_UNLIKELY(crosstalk_eda_ic_fpga_build_shutdown(&ctx->eda.fpga_build) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "fpga build shutdown failed"); }

    mark = crosstalk_shutdown_mark("fpga_build", mark);

    if (LDG_UNLIKELY(crosstalk_eda_sound_shutdown(&ctx->eda.sound) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sound shutdown failed"); }

    mark = crosstalk_shutdown_mark("sound", mark);

    if (LDG_UNLIKELY(crosstalk_eda_io_serial_shutdown(&ctx->eda.serial) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "serial shutdown failed"); }

    if (LDG_UNLIKELY(crosstalk_core_scan_shutdown(&ctx->scan) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scan shutdown failed"); }

    if (LDG_UNLIKELY(crosstalk_eda_io_midi_shutdown(&ctx->eda.midi) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "midi shutdown failed"); }

    if (LDG_UNLIKELY(crosstalk_eda_script_shutdown(&ctx->eda.script) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "script shutdown failed"); }

    if (crosstalk_eda_io_pty_shutdown(&ctx->eda.pty) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty shutdown failed"); }

    if (ctx->eda.pty_pool != 0x0 && LDG_UNLIKELY(ldg_mem_pool_destroy(&ctx->eda.pty_pool) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pty pool destroy failed"); }

    if (ctx->eda.term_pool != 0x0 && LDG_UNLIKELY(ldg_mem_pool_destroy(&ctx->eda.term_pool) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term pool destroy failed"); }

    if (ctx->eda.script_pool != 0x0 && LDG_UNLIKELY(ldg_mem_pool_destroy(&ctx->eda.script_pool) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "script pool destroy failed"); }

    if (LDG_UNLIKELY(crosstalk_eda_ui_scope_shutdown(&ctx->eda.scope) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "scope shutdown failed"); }

    if (LDG_UNLIKELY(crosstalk_eda_ui_indicator_shutdown(&ctx->eda.indicator) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "indicator shutdown failed"); }

    if (LDG_UNLIKELY(crosstalk_eda_ui_term_shutdown(&ctx->eda.term) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term shutdown failed"); }

    if (LDG_UNLIKELY(crosstalk_eda_logic_stream_shutdown(&ctx->eda.logic_stream) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "logic stream shutdown failed"); }

    if (LDG_UNLIKELY(crosstalk_eda_logic_probe_shutdown(&ctx->eda.logic_probe) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "logic probe shutdown failed"); }

    mark = crosstalk_shutdown_mark("logic", mark);

    ldg_thread_pool_shutdown(&ctx->game.music_pool);
    mark = crosstalk_shutdown_mark("music_pool", mark);

    ldg_thread_pool_shutdown(&ctx->game.ld_pool);
    mark = crosstalk_shutdown_mark("ld_pool", mark);

    ldg_sched_ctx_shutdown(&ctx->sched);
    mark = crosstalk_shutdown_mark("sched", mark);
    if (ctx->progression_queue_ok == LDG_TRUTH_TRUE)
    {
        ret = crosstalk_game_progression_queue_shutdown(&ctx->progression_queue);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "crosstalk: progression queue shutdown; err: %u32", ret); }

        ctx->progression_queue_ok = LDG_TRUTH_FALSE;
    }

    mark = crosstalk_shutdown_mark("progression_queue", mark);

    ldg_audio_shutdown();

    mark = crosstalk_shutdown_mark("audio", mark);
    if (ctx->discord_live == LDG_TRUTH_TRUE) { crosstalk_net_discord_shutdown(&ctx->discord); ctx->discord_live = LDG_TRUTH_FALSE; }

    crosstalk_net_inventory_shutdown(&ctx->inventory);

    crosstalk_net_wshop_shutdown(&ctx->wshop);

    if (crosstalk_net_relay_live_is(&ctx->relay) == LDG_TRUTH_TRUE) { if (LDG_UNLIKELY(crosstalk_net_relay_shutdown(&ctx->relay) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "relay shutdown failed"); } }

    mark = crosstalk_shutdown_mark("relay", mark);

    mark = crosstalk_shutdown_mark("discord", mark);

    if (ctx->steam_live == LDG_TRUTH_TRUE)
    {
        crosstalk_net_steam_shutdown(&ctx->steam_net);
        ldg_net_steamworks_shutdown();
    }

    mark = crosstalk_shutdown_mark("steam", mark);

    crosstalk_board_preview_drop(ctx);

    if (crosstalk_game_fx_sfx_play_cb_set(&ctx->game.sfx_sound, 0x0, 0x0) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sfx cb clear failed"); }

    if (crosstalk_game_caption_shutdown(&ctx->game.caption) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "caption shutdown failed"); }

    if (crosstalk_ui_guide_shutdown(&ctx->guide) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "guide shutdown failed"); }

    if (LDG_UNLIKELY(crosstalk_game_fx_sfx_shutdown(&ctx->game.sfx_sound) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "sfx shutdown failed"); }

    if (crosstalk_ui_window_shutdown(&ctx->win_nav) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "window nav shutdown failed"); }

    if (ctx->irq_src != 0x0)
    {
        if (ldg_term_input_destroy(ctx->irq_src) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "key irq source destroy failed"); }

        ctx->irq_src = 0x0;
    }

    if (ctx->sesh != 0x0)
    {
        if (ldg_term_sesh_destroy(ctx->sesh) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "term sesh destroy failed"); }

        ctx->sesh = 0x0;
    }

    if (crosstalk_core_shot_destroy(&ctx->shot) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "shot: destroy failed"); }

    if (crosstalk_core_watch_destroy(&ctx->demo_watch) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "demo watch: destroy failed"); }

    ldg_mem_dealloc(ctx);

    return LDG_ERR_AOK;
}
