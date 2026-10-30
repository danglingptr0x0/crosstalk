#ifndef CROSSTALK_H
#define CROSSTALK_H

#include <stdint.h>

#include "core/log.h"
#include "core/cmdline.h"
#include "core/fetch.h"
#include "core/scan.h"
#include "core/shot.h"
#include "core/task.h"
#include "core/watch.h"
#include "ui/window.h"
#include "ui/key.h"
#include "ui/theme.h"
#include "ui/guide.h"

#include <dangling/core/err.h>
#include <dangling/sched/sched.h>
#include <dangling/io/path.h>
#include <dangling/thread/pool.h>
#include <dangling/thread/sync.h>
#include <dangling/thread/yield.h>
#include <dangling/term/input.h>
#include <dangling/term/sesh.h>
#include <dangling/net/steamworks.h>
#include <dangling/net/steamworks_lobby.h>
#include <dangling/net/steamworks_wshop.h>
#include <dangling/net/steamworks_tkt.h>
#include <dangling/logging/logging.h>

#include "eda/eda.h"
#include "emu/net.h"
#include "hdl/aig.h"
#include "hdl/elf.h"
#include "hdl/type.h"
#include "hdl/pack.h"
#include "hdl/emit.h"
#include "hdl/bake.h"
#include "re/re.h"
#include "reng/reng.h"
#include "reng/progress.h"
#include "reng/tasks.h"
#include "game/game.h"
#include "game/board.h"
#include "game/progression/queue.h"
#include "game/chat.h"
#include "game/hub.h"
#include "game/music.h"
#include "game/menu.h"
#include "game/settings_menu.h"
#include "net/steam.h"
#include "net/wire.h"
#include "net/relay.h"
#include "net/discord.h"
#include "net/inventory/inventory.h"
#include "net/wshop/wshop.h"
#include "wshop/item.h"

#define CROSSTALK_CORE_TOAST_TEXT_CAP 192
#define CROSSTALK_CORE_TOAST_SHOW_MS 4000
#define CROSSTALK_CORE_TERM_SLOT 0
#define CROSSTALK_CORE_TERM_SESH 0
#define CROSSTALK_CORE_PTY_SETTLE_FRAMES 30
#define CROSSTALK_CORE_TERM_PORT_DEFAULT "serial_out"
#define CROSSTALK_CORE_IND_PORT_DEFAULT "led"
#define CROSSTALK_CORE_EMU_TOAST_CAP 128
#define CROSSTALK_CORE_OFF_TOK "off"
#define CROSSTALK_CORE_EMU_NONE_TEXT "no emulator attached"
#define CROSSTALK_CORE_EMU_OFF_TEXT "emulator detached; the slot is empty; load a design"
#define CROSSTALK_CORE_EMU_OK_TEXT "running on the emulator"
#define CROSSTALK_CORE_EMU_NO_DESIGN_TEXT "program a design first"
#define CROSSTALK_CORE_EMU_CLK_NAME "clk"
#define CROSSTALK_CORE_EMU_OSC_NONE_TEXT "none"
#define CROSSTALK_CORE_HLT_ON_TEXT "core halted; state readable"
#define CROSSTALK_CORE_HLT_OFF_TEXT "core released"
#define CROSSTALK_CORE_HLT_MASK_ON_TEXT "fault halt mask set"
#define CROSSTALK_CORE_HLT_MASK_OFF_TEXT "fault halt mask cleared"
#define CROSSTALK_CORE_HLT_MASK_NO_EMU_TEXT "fault halt mask unsupported on this engine"
#define CROSSTALK_CORE_HLT_CAUSE_BAD_TEXT "hlt <invalid|unaligned|stackover|stackunder|divzero|dr14|1-6>"
#define CROSSTALK_CORE_HLT_CAUSE_INVALID "invalid"
#define CROSSTALK_CORE_HLT_CAUSE_UNALIGNED "unaligned"
#define CROSSTALK_CORE_HLT_CAUSE_STACK_OVER "stackover"
#define CROSSTALK_CORE_HLT_CAUSE_STACK_UNDER "stackunder"
#define CROSSTALK_CORE_HLT_CAUSE_DIV_ZERO "divzero"
#define CROSSTALK_CORE_HLT_CAUSE_DR14 "dr14"
#define CROSSTALK_CORE_HLT_CAUSE_ALL "all"
#define CROSSTALK_CORE_HLT_CAUSE_TOK_CAP 16
#define CROSSTALK_CORE_DSU_PORT "dsu"
#define CROSSTALK_CORE_DSU_WIDTH 8
#define CROSSTALK_CORE_DSU_DRAIN_CAP 8192
#define CROSSTALK_CORE_DSU_SPAN_MAX 65536
#define CROSSTALK_CORE_DSU_PCM_CHUNK 1024
#define CROSSTALK_CORE_DSU_PCM_BYTES 2
#define CROSSTALK_CORE_DSU_SIGN_FLIP 0x80
#define CROSSTALK_CORE_DSU_DEPTH_MAX 1200
#define CROSSTALK_CORE_BRK_SKIP_TOK "skip"
#define CROSSTALK_CORE_BRK_COND_TOK "cond"
#define CROSSTALK_CORE_BRK_TOK_CAP 32
#define CROSSTALK_CORE_BRK_MIN_NONE 0
#define CROSSTALK_CORE_BRK_SKIP_FMT "break; skipping the first %" PRIu64 " hits"
#define CROSSTALK_CORE_BRK_COND_FMT "break; only when dr%" PRIu64 " is at least %" PRIu64
#define CROSSTALK_CORE_BRK_COND_OFF_TEXT "breakpoint condition cleared"
#define CROSSTALK_CORE_BRK_USAGE_TEXT "break <addr> | break off | break skip <n> | break cond <reg> <min>"
#define CROSSTALK_CORE_BRK_NONE_TEXT "no breakpoint armed"
#define CROSSTALK_CORE_BRK_OFF_TEXT "breakpoint cleared"
#define CROSSTALK_CORE_BRK_FAIL_TEXT "the emulator refused the breakpoint"
#define CROSSTALK_CORE_BRK_NO_SUPPORT_TEXT "this emulator build has no breakpoint support"
#define CROSSTALK_CORE_BRK_SET_FMT "break; armed at 0x%" PRIX64
#define CROSSTALK_CORE_BRK_AT_FMT "break; armed at 0x%" PRIX64 "; clear with break off"
#define CROSSTALK_CORE_DSU_PLAY_TOK "play"
#define CROSSTALK_CORE_DSU_STOP_TOK "stop"
#define CROSSTALK_CORE_DSU_PLAY_TEXT "sound card routed to audio; scope dsu to trace it"
#define CROSSTALK_CORE_DSU_STOP_TEXT "sound card released"
#define CROSSTALK_CORE_DSU_NO_AUDIO_TEXT "this engine has no audio drain"
#define CROSSTALK_CORE_DSU_NONE_TEXT "the sound card is unreadable on this engine"
#define CROSSTALK_CORE_DSU_IDLE_TEXT "the guest has not programmed the sound card"
#define CROSSTALK_CORE_DSU_FMT "dsu; base: 0x%" PRIX64 "; tail: 0x%" PRIX64 "; curr: 0x%" PRIX64 "; rate: %" PRIu64 " Hz; run: %" PRIu64
#define CROSSTALK_CORE_DISK_USAGE_TEXT "disk <path.dsk|sync|off> [drive]; bare disk reports the media"
#define CROSSTALK_CORE_DISK_NONE_TEXT "no media attached"
#define CROSSTALK_CORE_DISK_OK_FMT "media attached; drive: %u"
#define CROSSTALK_CORE_DISK_OFF_FMT "media ejected; drive: %u"
#define CROSSTALK_CORE_DISK_SYNC_FMT "media written back; drive: %u"
#define CROSSTALK_CORE_DISK_FAIL_FMT "media refused; drive: %u; err: %u"
#define CROSSTALK_CORE_DISK_STATE_FMT "drive %u: %.95s"
#define CROSSTALK_CORE_DISK_SYNC_TOK "sync"
#define CROSSTALK_CORE_DISK_DRIVE_BAD_TEXT "disk drive out of range; 0-7"
#define CROSSTALK_CORE_SCAN_NONE_TEXT "no scan provider bound"
#define CROSSTALK_CORE_SCAN_CLEAN_TEXT "scan clean"
#define CROSSTALK_CORE_SCAN_HIT_TEXT "scan detected a threat"
#define CROSSTALK_CORE_SCAN_FAIL_FMT "scan refused; err: %u"
#define CROSSTALK_CORE_SCAN_STATE_FMT "scan; provider: %u; runs: %" PRIu64 "; hits: %" PRIu64
#define CROSSTALK_CORE_SERIAL_MAP_TERM_CHAR ';'
#define CROSSTALK_CORE_SERIAL_MAP_CAP 8
#define CROSSTALK_CORE_SERIAL_MAP_PATTERN_CAP 32
#define CROSSTALK_CORE_SERIAL_MAP_ACTION_CAP 128
#define CROSSTALK_CORE_SERIAL_MAP_FULL_TEXT "serial map full"
#define CROSSTALK_CORE_SERIAL_MAP_ON_TEXT "serial listener armed"
#define CROSSTALK_CORE_SERIAL_TX_TEXT "serial bytes sent"
#define CROSSTALK_CORE_SERIAL_CONF_BAD_TEXT "serial conf refused; port@baud:parity:data:stop"
#define CROSSTALK_CORE_SERIAL_TX_FAIL_TEXT "serial send refused"
#define CROSSTALK_CORE_SERIAL_STOP_TOK_SINGLE "1"
#define CROSSTALK_CORE_SERIAL_STOP_TOK_DOUBLE "2"
#define CROSSTALK_CORE_SERIAL_LANE_RX "serial_rx"
#define CROSSTALK_CORE_SERIAL_LANE_TX "serial_tx"
#define CROSSTALK_CORE_MEM_RELOAD_TEXT "memory image reloaded from disk"
#define CROSSTALK_CORE_MEM_RELOAD_FAIL_TEXT "memory image changed on disk; load failed"
#define CROSSTALK_CORE_EMU_RELOAD_TEXT "emulator reloaded from disk"
#define CROSSTALK_CORE_EMU_RELOAD_FAIL_TEXT "emulator changed on disk; load failed"
#define CROSSTALK_CORE_EMU_STATE_FMT "emulator: %.95s"
#define CROSSTALK_CORE_NET_STATE_FMT "network: %.95s; open: %u"
#define CROSSTALK_CORE_NET_FAIL_FMT "network endpoint refused; err: %u"
#define CROSSTALK_CORE_NET_NONE_TEXT "no network endpoint set"
#define CROSSTALK_CORE_NET_OFF_TEXT "network endpoint cleared"
#define CROSSTALK_CORE_NET_OK_TEXT "network endpoint set"
#define CROSSTALK_CORE_EMU_FAIL_FMT "emulator refused; err: %u"
#define CROSSTALK_CORE_SYMS_EXT ".elf"
#define CROSSTALK_CORE_SYMS_DOT '.'
#define CROSSTALK_CORE_SYMS_SEP_POSIX '/'
#define CROSSTALK_CORE_SYMS_SEP_WIN '\\'
#define CROSSTALK_CORE_SERIAL_EVENT_CAP 8
#define CROSSTALK_CORE_SERIAL_TOAST_CAP 96
#define CROSSTALK_CORE_SERIAL_ARRIVE_FMT "%.31s connected: %.31s"
#define CROSSTALK_CORE_SERIAL_DEPART_FMT "%.31s disconnected: %.31s"
#define CROSSTALK_CORE_SERIAL_ATTACH_TEXT "serial attached"
#define CROSSTALK_CORE_SERIAL_OPEN_FAIL_TEXT "could not open that port"
#define CROSSTALK_CORE_SERIAL_BUSTED_TEXT "serial link lost"
#define CROSSTALK_CORE_SERIAL_NONE_TEXT "no serial ports found"
#define CROSSTALK_CORE_SERIAL_LISTED_TEXT "ports written to the log"
#define CROSSTALK_CORE_PIPE_ATTACH_TEXT "design piped to the wire"
#define CROSSTALK_CORE_PIPE_NO_PORT_TEXT "attach a serial port first"
#define CROSSTALK_CORE_PIPE_NO_DESIGN_TEXT "no running design"
#define CROSSTALK_CORE_PIPE_NO_SIG_TEXT "no such port in the design"
#define CROSSTALK_CORE_PIPE_NO_STREAM_TEXT "no free stream"
#define CROSSTALK_CORE_PIPE_NO_SINK_TEXT "sink refused"
#define CROSSTALK_CORE_SOUND_PORT_DEFAULT "sound_out"
#define CROSSTALK_CORE_SOUND_ATTACH_TEXT "sound bound to the design"
#define CROSSTALK_CORE_SOUND_NO_CLK_TEXT "the component has no clock"
#define CROSSTALK_CORE_SOUND_BAKE_TEXT "playing what the board made"
#define CROSSTALK_CORE_SOUND_EMPTY_TEXT "nothing captured yet"
#define CROSSTALK_CORE_SOUND_BAKE_FAIL_TEXT "could not play the capture"
#define CROSSTALK_CORE_SOUND_RATE_TEXT "that rate is out of range"
#define CROSSTALK_CORE_PACK_SRC_FILE "pack_design.c"
#define CROSSTALK_CORE_PACK_OUT_FILE "pack_design.bin"
#define CROSSTALK_CORE_PACK_CC_REL_WIN "res/utils/cc/windows/bin"
#define CROSSTALK_CORE_PACK_CC_REL_UNIX "res/utils/cc/linux/bin"
#define CROSSTALK_CORE_PACK_MODE 0644
#define CROSSTALK_CORE_PACK_START_TEXT "baking the design; this takes a while"
#define CROSSTALK_CORE_PACK_LIVE_TEXT "a bake is already running"
#define CROSSTALK_CORE_PACK_FAIL_TEXT "could not bake that design"
#define CROSSTALK_CORE_PACK_READY_TEXT "the compiled component is ready"
#define CROSSTALK_CORE_PACK_CC_TEXT "compiling the design"
#define CROSSTALK_CORE_PACK_LD_TEXT "loading the compiled component"
#define CROSSTALK_CORE_PACK_BIND_TEXT "wiring the compiled component up"
#define CROSSTALK_CORE_SNAP_FILE "design.bake"
#define CROSSTALK_CORE_SNAP_NAMED_FMT "%.63s.bake"
#define CROSSTALK_CORE_SNAP_EXISTS_TEXT "unnamed snap exists; use snap <name>"
#define CROSSTALK_CORE_SNAP_NO_ABI_TEXT "compile the design first with :emit"
#define CROSSTALK_CORE_SNAP_OK_TEXT "design state written"
#define CROSSTALK_CORE_SNAP_FAIL_TEXT "could not write the design state"
#define CROSSTALK_CORE_RESUME_OK_TEXT "design state restored"
#define CROSSTALK_CORE_RESUME_FAIL_TEXT "could not restore that design state"
#define CROSSTALK_CORE_CLK_PIN_TEXT "the component is held at that clock"
#define CROSSTALK_CORE_CLK_FREE_TEXT "the component runs as fast as it can again"
#define CROSSTALK_CORE_CLK_FAIL_TEXT "could not hold that clock"
#define CROSSTALK_CORE_SCOPE_STEP_NS_MIN 1
#define CROSSTALK_CORE_SCOPE_SIG_TEXT "scope on the signal"
#define CROSSTALK_CORE_SCOPE_FAIL_TEXT "the scope refused that signal"
#define CROSSTALK_CORE_QUIT_GUARD_MS 120000
#define CROSSTALK_CORE_QUIT_GUARD_TEXT "no write since the last change; q! to leave anyway"
#define CROSSTALK_CORE_QUIT_PAUSE_TEXT "no write since the last change; pick it again to leave"
#define CROSSTALK_CORE_QUIT_ARMED_TEXT "Quit anyway"
#define CROSSTALK_CORE_QUIT_KILL_CODE 137
#define CROSSTALK_CORE_SOUND_PASS_FAIL_TEXT "capturing, but the speaker refused"
#define CROSSTALK_CORE_SOUND_LIVE_TEXT "already playing live"
#define CROSSTALK_CORE_SOUND_ARG_CAP 64
#define CROSSTALK_CORE_MEMLD_USAGE_TEXT "memld <signal> <path> [word offset]"
#define CROSSTALK_CORE_RE_USAGE_TEXT "re open|close|seg|rd|dis|name|unname|note|unnote|func|unfunc|struct|field|patch|unpatch|cc|img|import|save|undo|assert|build|diff|ls"
#define CROSSTALK_CORE_RE_NO_IMG_TEXT "no artifact open; re open <emiru|elf|raw> [base] [isa]"
#define CROSSTALK_CORE_RE_OPEN_OK_TEXT "artifact open"
#define CROSSTALK_CORE_RE_OPEN_FAIL_TEXT "could not load that artifact; see the log"
#define CROSSTALK_CORE_RE_OPEN_ISA_TEXT "a raw image needs an isa; re open <path> <base> dangcsa|x86|x86-64|arm|aarch64|mips|ppc|ppc64|riscv"
#define CROSSTALK_CORE_RE_NO_ORACLE_TEXT "no oracle for that isa yet; the emulator runs dangcsa only"
#define CROSSTALK_CORE_RE_DIS_NONE_TEXT "no disassembler for that isa"
#define CROSSTALK_CORE_RE_CLOSED_TEXT "artifact closed"
#define CROSSTALK_CORE_RE_ADDR_TEXT "that address is not inside the artifact"
#define CROSSTALK_CORE_RE_REC_OK_TEXT "recorded"
#define CROSSTALK_CORE_RE_REC_FAIL_TEXT "the database refused that record; see the log"
#define CROSSTALK_CORE_RE_REC_GONE_TEXT "record removed"
#define CROSSTALK_CORE_RE_REC_NONE_TEXT "no such record"
#define CROSSTALK_CORE_RE_SAVE_OK_TEXT "database written"
#define CROSSTALK_CORE_RE_SAVE_FAIL_TEXT "database write failed; see the log"
#define CROSSTALK_CORE_RE_UNDO_OK_TEXT "undone"
#define CROSSTALK_CORE_RE_UNDO_NONE_TEXT "nothing to undo"
#define CROSSTALK_CORE_RE_IMG_MISMATCH_TEXT "database pinned to a different artifact; see the log"
#define CROSSTALK_CORE_RE_IMPORT_FMT "imported %u symbols"
#define CROSSTALK_CORE_RE_BUILD_START_TEXT "building the candidate"
#define CROSSTALK_CORE_RE_BUILD_BUSY_TEXT "a build is already running"
#define CROSSTALK_CORE_RE_BUILD_NO_STEPS_TEXT "no build steps; re cc <n> \"<command with {src} {obj} {out}>\""
#define CROSSTALK_CORE_RE_BUILD_OK_TEXT "candidate built"
#define CROSSTALK_CORE_RE_BUILD_FAIL_TEXT "candidate build failed; see the log"
#define CROSSTALK_CORE_RE_DIFF_ARMED_TEXT "differential run queued"
#define CROSSTALK_CORE_RE_DIFF_NOT_READY_TEXT "set both sides first; re diff ref|cand <path> [sig] [off]"
#define CROSSTALK_CORE_RE_DIFF_NO_EMU_TEXT "differential needs a live emulator"
#define CROSSTALK_CORE_RE_DIFF_MATCH_TEXT "verdict: match"
#define CROSSTALK_CORE_RE_DIFF_DIVERGED_TEXT "verdict: diverged; see the log"
#define CROSSTALK_CORE_RE_DIFF_FAIL_TEXT "differential run failed; see the log"
#define CROSSTALK_CORE_RE_ASSERT_OK_TEXT "layout assertions written"
#define CROSSTALK_CORE_RE_ASSERT_FAIL_TEXT "layout assertion write failed; see the log"
#define CROSSTALK_CORE_RE_DIS_ROWS_DEFAULT 16
#define CROSSTALK_CORE_RE_RD_BYTES_DEFAULT 64
#define CROSSTALK_CORE_RE_RD_ROW_BYTES 16
#define CROSSTALK_CORE_RE_RD_BYTES_MAX 4096
#define CROSSTALK_CORE_RE_DIS_ROWS_MAX 256
#define CROSSTALK_CORE_RE_WORD_CAP 512
#define CROSSTALK_CORE_RE_LINE_CAP 384
#define CROSSTALK_CORE_RE_DIS_TEXT_CAP 96
#define CROSSTALK_CORE_RE_PATCH_BYTES_MAX 64
#define CROSSTALK_CORE_RE_ASSERT_LINE_CAP 256
#define CROSSTALK_CORE_RE_ASSERT_MODE 0644
#define CROSSTALK_CORE_RE_DIFF_PENDING_FRAMES 2
#define CROSSTALK_CORE_RE_FUNC_NONE_TEXT "that function is not in both symbol tables; see the log"
#define CROSSTALK_CORE_RE_FUNC_ARMED_TEXT "function harness set; re diff run"
#define CROSSTALK_CORE_RE_CAND_SYMS_NONE_TEXT "the candidate has no sibling .elf; symbols unavailable"
#define CROSSTALK_CORE_RE_SP_SLACK 16
#define CROSSTALK_CORE_RE_STACK_SPAN 65536
#define CROSSTALK_CORE_PROJ_STATUS_FMT "%.31s; %u/%u verified"
#define CROSSTALK_CORE_PROJ_STATUS_CAP 96
#define CROSSTALK_CORE_RE_BUSY_TITLE "Differential run"
#define CROSSTALK_CORE_RE_BUSY_BODY_TEXT "running both artifacts on the emulator; input is blocked"
#define CROSSTALK_CORE_MEMRD_USAGE_TEXT "mem <signal> <address> [how many]"
#define CROSSTALK_CORE_MEMRD_FAIL_TEXT "could not read that memory"
#define CROSSTALK_CORE_MEMRD_OK_TEXT "memory written to the log"
#define CROSSTALK_CORE_MEMRD_ROW_CUNT 64
#define CROSSTALK_CORE_MEMRD_TEXT_CAP 384
#define CROSSTALK_CORE_MEMRD_BYTE_FMT " %02" PRIX64
#define CROSSTALK_CORE_MEMRD_BYTE_MASK 0xFF
#define CROSSTALK_CORE_MEMVIEW_SIG_DEFAULT "mem"
#define CROSSTALK_CORE_TEXT_OK_TEXT "text screen wired to the design"
#define CROSSTALK_CORE_GFX_OK_TEXT "video wired to the design"
#define CROSSTALK_CORE_GFX_BASE_DEFAULT 0xA0000
#define CROSSTALK_CORE_GFX_TOK_CUNT 4
#define CROSSTALK_CORE_CGACAST_OK_TEXT "casting the framebuffer over serial"
#define CROSSTALK_CORE_CGACAST_OFF_TEXT "cast stopped"
#define CROSSTALK_CORE_CGACAST_FAIL_TEXT "cast would not start; ck signal and geometry"
#define CROSSTALK_CORE_CGACAST_W_DEFAULT 320
#define CROSSTALK_CORE_CGACAST_H_DEFAULT 200
#define CROSSTALK_CORE_SERIAL_ACCESS_GRANT_SECS 5.0
#define CROSSTALK_CORE_SERIAL_ACCESS_TIMEOUT_SECS 15.0
#define CROSSTALK_CORE_SERIAL_ACCESS_ON_FMT "%.63s put %.63s on the horn for the session"
#define CROSSTALK_CORE_SERIAL_ACCESS_OFF_FMT "%.63s hung up %.63s"
#define CROSSTALK_CORE_SERIAL_ACCESS_QUIET_TEXT "the shared serial device went quiet; detached"
#define CROSSTALK_CORE_MP_HOST_RETRY_CUNT 5
#define CROSSTALK_CORE_MP_HOST_RETRY_SECS 1.0
#define CROSSTALK_CORE_MP_HOST_FAIL_TEXT "hosting failed; Steam is still connecting - press host again"
#define CROSSTALK_CORE_PROFILE_START_TOK "start"
#define CROSSTALK_CORE_PROFILE_END_TOK "end"
#define CROSSTALK_CORE_PROFILE_RST_TOK "reset"
#define CROSSTALK_CORE_PROFILE_START_TEXT "profile capture armed; press end to close it"
#define CROSSTALK_CORE_PROFILE_END_TEXT "profile capture closed"
#define CROSSTALK_CORE_PROFILE_NONE_TEXT "no profile capture is running"
#define CROSSTALK_CORE_PROFILE_RST_TEXT "profile counters rebased"
#define CROSSTALK_CORE_PROFILE_USAGE_TEXT "profile | profile start [name] | profile end | profile reset"
#define CROSSTALK_CORE_CPUSET_FIRST_CORE 1
#define CROSSTALK_CORE_CPUSET_MIN_CUNT 4
#define CROSSTALK_CORE_CPUSET_SIBLING_CAP 8
#define CROSSTALK_CORE_PROF_ON_TOK "on"
#define CROSSTALK_CORE_PROF_ON_TEXT "instruction ring on"
#define CROSSTALK_CORE_PROF_OFF_TEXT "instruction ring off"
#define CROSSTALK_CORE_PROF_USAGE_TEXT "prof on | prof off"
#define CROSSTALK_CORE_PROF_HELP_TEXT "prof <on|off>; instruction ring and opcode counting; off unless turned on here"
#define CROSSTALK_CORE_RECEPT_USAGE_TEXT "receptacle <data> <ready> [ack] [scan]"
#define CROSSTALK_CORE_RECEPT_OK_TEXT "receptacle wired to the design"
#define CROSSTALK_CORE_RECEPT_SCAN_TOK "scan"
#define CROSSTALK_CORE_MIDI_USAGE_TEXT "midi <pin> [device]"
#define CROSSTALK_CORE_MIDI_OK_TEXT "midi wired to the design"
#define CROSSTALK_CORE_MIDI_NO_DEV_TEXT "no midi devices"
#define CROSSTALK_CORE_MIDI_DEV_FAIL_TEXT "that midi device would not open"
#define CROSSTALK_CORE_MIDI_DEV_DEFAULT 0
#define CROSSTALK_CORE_MIDI_PIN_FMT "%.63s"
#define CROSSTALK_CORE_SCRIPT_POOL_MIB 4
#define CROSSTALK_CORE_SCRIPT_RUN_NAME "run"
#define CROSSTALK_CORE_SCRIPT_RUN_LEN 3
#define CROSSTALK_CORE_SCRIPT_QUOTE_BYTES 2
#define CROSSTALK_CORE_SCRIPT_QUOTE_NONE 0
#define CROSSTALK_CORE_SCRIPT_RUN_HELP "run <path.xts>; execute a script file"
#define CROSSTALK_CORE_SCRIPT_RUN_FAIL_FMT "run failed; err: %u; path: %.100s"
#define CROSSTALK_CORE_SCRIPT_HELP_NAME "help"
#define CROSSTALK_CORE_SCRIPT_HELP_LEN 4
#define CROSSTALK_CORE_SCRIPT_HELP_HELP "help [command]; list the commands, or show how to use one"
#define CROSSTALK_CORE_SCRIPT_HELP_LIST_HD "commands:"
#define CROSSTALK_CORE_SCRIPT_HELP_UNKNOWN_FMT "no such command: %.63s"
#define CROSSTALK_CORE_SCRIPT_HELP_LIST_CAP 1024
#define CROSSTALK_CORE_SCRIPT_HELP_SEP " "
#define CROSSTALK_CORE_SCRIPT_DIAG_FMT "%u:%u: %.100s"
#define CROSSTALK_CORE_SCRIPT_SUGGESTION_FMT "try: %.130s"
#define CROSSTALK_CORE_MIDI_NO_PIN_TEXT "wire midi to a design pin first"
#define CROSSTALK_CORE_MIDI_PLAY_TEXT "playing the midi file"
#define CROSSTALK_CORE_MIDI_STOP_TEXT "midi file stopped"
#define CROSSTALK_CORE_MIDI_FILE_FAIL_TEXT "could not play that midi file"
#define CROSSTALK_CORE_MIDI_NOTE_USAGE_TEXT "midinote <note> [velocity]"
#define CROSSTALK_CORE_MIDI_NOTE_FAIL_TEXT "the midi queue is full"
#define CROSSTALK_CORE_MEMVIEW_FOLLOW_MARK '@'
#define CROSSTALK_CORE_MEMVIEW_BIND_FAIL_TEXT "no such memory in the design"
#define CROSSTALK_CORE_MEMVIEW_OPEN_TEXT "memory window bound"
#define CROSSTALK_CORE_MEMFIND_USAGE_TEXT "memfind <hex bytes> or memfind \"text\""
#define CROSSTALK_CORE_MEMVIEW_WATCH_TOK "watch"
#define CROSSTALK_CORE_MEMVIEW_SNAP_TOK "snap"
#define CROSSTALK_CORE_MEMSNAP_OK_TEXT "snap loaded into the memory window"
#define CROSSTALK_CORE_MEMSNAP_FAIL_TEXT "could not load that snap"
#define CROSSTALK_CORE_MEMSNAP_MISMATCH_TEXT "that snap belongs to a different design"
#define CROSSTALK_CORE_SNAP_LIST_TOK "list"
#define CROSSTALK_CORE_SNAP_LIST_EXT ".bake"
#define CROSSTALK_CORE_SNAP_LIST_DONE_FMT "%u snaps; names in the log"
#define CROSSTALK_CORE_SNAP_LIST_FAIL_TEXT "could not read the runtime directory"
#define CROSSTALK_CORE_MEMDBG_STEP_DEFAULT 1
#define CROSSTALK_CORE_MEMDBG_STEP_FMT "stepped %" PRIu64 " cycles"
#define CROSSTALK_CORE_MEMDBG_HLT_TEXT "clock stopped"
#define CROSSTALK_CORE_MEMDBG_RUN_TEXT "clock running"
#define CROSSTALK_CORE_MEMDBG_RUN_FAIL_TEXT "the clock would not start"
#define CROSSTALK_CORE_MEMDBG_STEP_DIS_CUNT 1
#define CROSSTALK_CORE_MEMBREAK_FMT "clock stopped; watch hit at 0x%" PRIX64
#define CROSSTALK_CORE_MEMBREAK_DIS_CUNT 4
#define CROSSTALK_CORE_CRT_TITLE "Monitor"
#define CROSSTALK_CORE_CRT_TITLE_HLT "Monitor [hlt'd]"
#define CROSSTALK_CORE_POKE_COPY_BYTES 4
#define CROSSTALK_CORE_POKE_BYTE_TEXT_LEN 4
#define CROSSTALK_CORE_FREEZE_MAX 32
#define CROSSTALK_CORE_FREEZE_LEN_MAX 16
#define CROSSTALK_CORE_FREEZE_LEN_DEFAULT 4
#define CROSSTALK_CORE_FREEZE_USAGE_TEXT "freeze 0xADDR|<symbol> [len]; freeze off [0xADDR]"
#define CROSSTALK_CORE_FREEZE_OK_FMT "frozen; addr: 0x%" PRIX64 "; bytes: %u"
#define CROSSTALK_CORE_FREEZE_OFF_TEXT "freeze cleared"
#define CROSSTALK_CORE_FREEZE_FULL_TEXT "freeze table full"
#define CROSSTALK_CORE_MEMVIEW_TITLE_ARCH_FMT "%s [%s]"
#define CROSSTALK_CORE_MEMVIEW_TITLE_PLAIN_FMT "%s"
#define CROSSTALK_CORE_WIN_NAV_CUNT 22

typedef struct crosstalk_core_win_nav_entry
{
    uint64_t win_id;
    const char *title;
} crosstalk_core_win_nav_entry_t;

#define CROSSTALK_CORE_ADDR_SYM_PLUS '+'
#define CROSSTALK_CORE_ADDR_SYM_MINUS '-'
#define CROSSTALK_CORE_SCOPE_MEM_TOK "mem "
#define CROSSTALK_CORE_SCOPE_XY_TOK "xy "
#define CROSSTALK_CORE_SCOPE_WIDTH_UINT8_TOK "uint8"
#define CROSSTALK_CORE_SCOPE_WIDTH_UINT16_TOK "uint16"
#define CROSSTALK_CORE_SCOPE_WIDTH_UINT32_TOK "uint32"
#define CROSSTALK_CORE_SCOPE_WIDTH_INT8_TOK "int8"
#define CROSSTALK_CORE_SCOPE_WIDTH_INT16_TOK "int16"
#define CROSSTALK_CORE_SCOPE_WIDTH_INT32_TOK "int32"
#define CROSSTALK_CORE_SCOPE_FIXED_TOK "fixed"
#define CROSSTALK_CORE_SCOPE_MEM_USAGE_TEXT "scope mem <0xADDR|symbol> [uint8|uint16|uint32|int8|int16|int32] [fixed] | scope mem off"
#define CROSSTALK_CORE_SCOPE_MEM_FULL_TEXT "the scope is out of trace slots"
#define CROSSTALK_CORE_SCOPE_MEM_OFF_TEXT "memory traces cleared"
#define CROSSTALK_CORE_SCOPE_MEM_OK_FMT "trace %u on %.31s"
#define CROSSTALK_CORE_SCOPE_XY_USAGE_TEXT "scope xy <trace> <trace> | scope xy off"
#define CROSSTALK_CORE_SCOPE_XY_OFF_TEXT "xy mode off"
#define CROSSTALK_CORE_SCOPE_XY_OK_TEXT "xy mode on"
#define CROSSTALK_CORE_MEMBREAK_RANGE_NONE 0
#define CROSSTALK_CORE_MEMBREAK_USAGE_TEXT "break 0xLO [0xHI] | break off"
#define CROSSTALK_CORE_MEMBREAK_ARM_FMT "watching stores into 0x%" PRIX64 " through 0x%" PRIX64
#define CROSSTALK_CORE_MEMBREAK_OFF_TEXT "store watch cleared"
#define CROSSTALK_CORE_MEMBREAK_REFUSED_TEXT "the emulator refused that store watch"
#define CROSSTALK_CORE_SYMS_USAGE_TEXT "syms <path.elf>"
#define CROSSTALK_CORE_SYMS_NO_FILE_TEXT "no such file"
#define CROSSTALK_CORE_SYMS_FAIL_TEXT "that file has no symbols to read"
#define CROSSTALK_CORE_SYMS_OK_TEXT "program symbols loaded"
#define CROSSTALK_CORE_MEMPOKE_IO_TEXT "this core does not take io writes"
#define CROSSTALK_CORE_GAMMA_SHOW_FMT "monitor gamma %.2f"
#define CROSSTALK_CORE_GAMMA_USAGE_TEXT "gamma [0.5 to 4.0]; 1.0 is flat"
#define CROSSTALK_CORE_IDENT_USAGE_TEXT "ident 0xADDR|<symbol>[+0xOFF]"
#define CROSSTALK_CORE_IDENT_NONE_TEXT "no symbol covers that address"
#define CROSSTALK_CORE_IDENT_OK_FMT "%.31s+0x%" PRIX64
#define CROSSTALK_CORE_IDENT_TABLE_PROG "prog"
#define CROSSTALK_CORE_IDENT_TABLE_KERNEL "kernel"
#define CROSSTALK_CORE_PROG_PUB_ADDR 0xF00
#define CROSSTALK_CORE_PROG_PUB_MAGIC 0x4C425550
#define CROSSTALK_CORE_PROG_PUB_REV 1
#define CROSSTALK_CORE_PROG_PUB_WORD_CUNT 18
#define CROSSTALK_CORE_PROG_PUB_MAGIC_AT 0
#define CROSSTALK_CORE_PROG_PUB_REV_AT 1
#define CROSSTALK_CORE_PROG_PUB_SEQ_AT 2
#define CROSSTALK_CORE_PROG_PUB_LIVE_AT 3
#define CROSSTALK_CORE_PROG_PUB_BASE_LO_AT 4
#define CROSSTALK_CORE_PROG_PUB_BASE_HI_AT 5
#define CROSSTALK_CORE_PROG_PUB_ENTRY_LO_AT 6
#define CROSSTALK_CORE_PROG_PUB_ENTRY_HI_AT 7
#define CROSSTALK_CORE_PROG_PUB_WIDTH_AT 8
#define CROSSTALK_CORE_PROG_PUB_ARGC_AT 9
#define CROSSTALK_CORE_PROG_PUB_NAME_AT 10
#define CROSSTALK_CORE_PROG_PUB_NAME_CAP 32
#define CROSSTALK_CORE_PROG_PUB_WORD_BYTES 4
#define CROSSTALK_CORE_PROG_PUB_HI_SHIFT 32
#define CROSSTALK_CORE_MEMPOKE_USAGE_TEXT "poke 0xADDR|<symbol> 0xNN [0xNN ...]"
#define CROSSTALK_CORE_MEMPOKE_OK_FMT "wrote %u bytes"
#define CROSSTALK_CORE_MEMPOKE_FAIL_TEXT "could not write that memory"
#define CROSSTALK_CORE_MEMPOKE_VAL_CAP 32
#define CROSSTALK_CORE_MEMDIFF_OK_TEXT "diff mounted; rows with changes highlight"
#define CROSSTALK_CORE_MEMWATCH_ADDR_CUNT 2
#define CROSSTALK_CORE_MEMWATCH_BYTE_FMT "0x%02" PRIX64 " "
#define CROSSTALK_CORE_MEMWATCH_UP_MARK '+'
#define CROSSTALK_CORE_MEMWATCH_DOWN_MARK '-'
#define CROSSTALK_CORE_MEMWATCH_ANY_MARK '*'
#define CROSSTALK_CORE_MEMWATCH_BOUND_MARK '~'
#define CROSSTALK_CORE_MEMMOTION_ADDR_ORIG 0
#define CROSSTALK_CORE_MEMMOTION_USAGE_TEXT "watch [0xLO 0xHI] +N | -N | + | - | * | ~N [run <cunt>]; a range needs both ends"
#define CROSSTALK_CORE_MEMMOTION_RUN_TOK "run"
#define CROSSTALK_CORE_MEMMOTION_RUN_MAX 8
#define CROSSTALK_CORE_MEMMOTION_OK_TEXT "watching for that movement"
#define CROSSTALK_CORE_MEMWATCH_USAGE_TEXT "memview watch [0xADDR|<symbol> [0xEND|0xVAL]] [0xNN ...|\"text\"]"
#define CROSSTALK_CORE_MEMWATCH_OK_TEXT "watching memory"
#define CROSSTALK_CORE_MEMWATCH_FAIL_TEXT "the memory window refused that watch"
#define CROSSTALK_CORE_MEMWATCH_HIT_FMT "memory watch hit at 0x%" PRIX64
#define CROSSTALK_CORE_MEMWATCH_MUT_FMT "memory changed at 0x%" PRIX64
#define CROSSTALK_CORE_MEMWATCH_NOTICE_CAP 96
#define CROSSTALK_CORE_MEMWATCH_NOTIFY_SECS 1.0
#define CROSSTALK_CORE_BUSY_TITLE "Working"
#define CROSSTALK_CORE_PERSIST_TITLE "Persisting machine state"
#define CROSSTALK_CORE_PERSIST_BODY_TEXT "the virtual disk is being written back; please hold"
#define CROSSTALK_CORE_BUSY_BODY_TEXT "operation in progress; input is blocked"
#define CROSSTALK_CORE_BUSY_FREEZE_TEXT "the window may stop responding; the work continues in the background"
#define CROSSTALK_CORE_SESH_TITLE "crosstalk"
#define CROSSTALK_CORE_SESH_SAMPLE_COL 8
#define CROSSTALK_CORE_SESH_SAMPLE_ROW 16
#define CROSSTALK_CORE_SESH_COL_MAX 512
#define CROSSTALK_CORE_SESH_ROW_MAX 192
#define CROSSTALK_CORE_MUSIC_COL_CAP 40
#define CROSSTALK_CORE_TEXT_CLIP_CAP 1024
#define CROSSTALK_CORE_MENU_ACTION_NONE 0U
#define CROSSTALK_CORE_MP_HOST_SELF 1U
#define CROSSTALK_CORE_MUSIC_KEY_PREV ','
#define CROSSTALK_CORE_MUSIC_KEY_PREV_SHIFTED '<'
#define CROSSTALK_CORE_MUSIC_KEY_NEXT '.'
#define CROSSTALK_CORE_MUSIC_KEY_NEXT_SHIFTED '>'
#define CROSSTALK_CORE_MUSIC_KEY_TOGGLE '/'
#define CROSSTALK_CORE_MUSIC_KEY_TOGGLE_SHIFTED '?'
#define CROSSTALK_CORE_KEY_SAVE_CODEPOINT 's'
#define CROSSTALK_CORE_KEY_OPEN_CODEPOINT 'o'
#define CROSSTALK_CORE_IDLE_RST 0.0
#define CROSSTALK_CORE_DESK_FRAC_ORIG 0.0
#define CROSSTALK_CORE_DESK_FRAC_FULL 1.0
#define CROSSTALK_CORE_BUSY_COL_CUNT 72
#define CROSSTALK_CORE_BUSY_ROW_CUNT 5
#define CROSSTALK_CORE_BUSY_LINE_TITLE 0
#define CROSSTALK_CORE_BUSY_LINE_STATUS 1
#define CROSSTALK_CORE_BUSY_LINE_BODY 2
#define CROSSTALK_CORE_BUSY_LINE_FREEZE 3
#define CROSSTALK_CORE_CARD_ASPECT_W 4.0
#define CROSSTALK_CORE_CARD_ASPECT_H 3.0
#define CROSSTALK_CORE_CARD_COL_CUNT 20
#define CROSSTALK_CORE_CARD_ROW_CUNT 15
#define CROSSTALK_CORE_CARD_FIELD_GRAY 0.50
#define CROSSTALK_CORE_CARD_BLACK 0.0
#define CROSSTALK_CORE_CARD_WHITE 1.0
#define CROSSTALK_CORE_CARD_CHANNEL_MAX 255.0
#define CROSSTALK_CORE_CARD_ROUND 0.5
#define CROSSTALK_CORE_CARD_HALF 0.5
#define CROSSTALK_CORE_CARD_TURN 2.0
#define CROSSTALK_CORE_CARD_RING_STEP_CUNT 720
#define CROSSTALK_CORE_CARD_SAMPLE_ORIG 0U
#define CROSSTALK_CORE_CARD_SAMPLE_MIN 1U
#define CROSSTALK_CORE_CARD_ALPHA_OPAQUE 0xFFU
#define CROSSTALK_CORE_CARD_CAST_TOP_CELLS 0.5
#define CROSSTALK_CORE_CARD_CAST_SIDE_CELLS 0.625
#define CROSSTALK_CORE_CARD_CAST_ALT_ROW_A 5
#define CROSSTALK_CORE_CARD_CAST_ALT_ROW_B 9
#define CROSSTALK_CORE_CARD_LUMA_R 0.299
#define CROSSTALK_CORE_CARD_LUMA_G 0.587
#define CROSSTALK_CORE_CARD_LUMA_B 0.114
#define CROSSTALK_CORE_CARD_BAR_AMPLITUDE 0.75
#define CROSSTALK_CORE_CARD_DIFF_AMPLITUDE 0.35
#define CROSSTALK_CORE_CARD_BAR_CUNT 6
#define CROSSTALK_CORE_CARD_STEP_CUNT 6
#define CROSSTALK_CORE_CARD_GRATING_CUNT 5
#define CROSSTALK_CORE_CARD_ACTIVE_LINE_US 52.0
#define CROSSTALK_CORE_CARD_LINE_US 0.225
#define CROSSTALK_CORE_CARD_SQUARE_KHZ 250.0
#define CROSSTALK_CORE_CARD_BLACK_RECT_US 11.4
#define CROSSTALK_CORE_CARD_COLSTEP_US 2.6
#define CROSSTALK_CORE_CARD_GRATING_MID 0.5
#define CROSSTALK_CORE_CARD_GRATING_AMP 0.5
#define CROSSTALK_CORE_CARD_CIRC_DIAMETER_FRAC 0.88
#define CROSSTALK_CORE_CARD_RECT_TOP_CELLS 1.6
#define CROSSTALK_CORE_CARD_RECT_H_CELLS 1.0
#define CROSSTALK_CORE_CARD_NEEDLE_H_CELLS 0.6
#define CROSSTALK_CORE_CARD_NEEDLE_POS 0.75
#define CROSSTALK_CORE_CARD_SQUARE_H_CELLS 1.0
#define CROSSTALK_CORE_CARD_BAR_H_CELLS 2.0
#define CROSSTALK_CORE_CARD_CROSS_H_CELLS 2.6
#define CROSSTALK_CORE_CARD_IDENT_GAP_CELLS 0.1
#define CROSSTALK_CORE_CARD_GRATING_H_CELLS 1.0
#define CROSSTALK_CORE_CARD_STEP_H_CELLS 1.0
#define CROSSTALK_CORE_CARD_COLSTEP_H_CELLS 0.8
#define CROSSTALK_CORE_CARD_SIDE_ALT_X_CELLS 1.0
#define CROSSTALK_CORE_CARD_SIDE_BAR_X_CELLS 2.2
#define CROSSTALK_CORE_CARD_SIDE_RECT_X_CELLS 3.0
#define CROSSTALK_CORE_CARD_SIDE_W_CELLS 0.8
#define CROSSTALK_CORE_CARD_SIDE_TOP_CELLS 3.0
#define CROSSTALK_CORE_CARD_SIDE_H_CELLS 9.0
#define CROSSTALK_CORE_CARD_SIDE_RECT_TOP_CELLS 2.5
#define CROSSTALK_CORE_CARD_SIDE_RECT_H_CELLS 1.5
#define CROSSTALK_CORE_CARD_TEXT "PLEASE STAND BY"
#define CROSSTALK_CORE_LD_FRAME_MIN 16
#define CROSSTALK_CORE_SPLASH_CW_TEXT "I do not want to set the world on fire"
#define CROSSTALK_CORE_SPLASH_CW_PROBABILITY 0.20
#define CROSSTALK_CORE_CARD_KIND_PAL 0
#define CROSSTALK_CORE_CARD_KIND_NTSC 1
#define CROSSTALK_CORE_CARD_COUNTRY_NTSC "US"
#define CROSSTALK_CORE_MEMFIND_FAIL_TEXT "the memory window refused that pattern"
#define CROSSTALK_CORE_MEMFIND_OK_TEXT "searching memory"
#define CROSSTALK_CORE_MEMLD_NO_SIG_TEXT "no such memory in the design"
#define CROSSTALK_CORE_MEMLD_FAIL_TEXT "could not load that file"
#define CROSSTALK_CORE_MEMLD_OK_TEXT "loaded into the design"
#define CROSSTALK_CORE_HEX_LEAD '0'
#define CROSSTALK_CORE_LD_USAGE_TEXT "ld <dir> <a.sv,b.sv|url> <top module>"
#define CROSSTALK_CORE_LD_DIR_CAP 512
#define CROSSTALK_CORE_LD_FILES_CAP 512
#define CROSSTALK_CORE_LD_OK_TEXT "design programmed"
#define CROSSTALK_CORE_PUBLISH_USAGE_TEXT "publish <isa>"
#define CROSSTALK_CORE_PUBLISH_OK_TEXT "manifest written; upload started"
#define CROSSTALK_CORE_PUBLISH_DONE_TEXT "published to the Workshop"
#define CROSSTALK_CORE_PUBLISH_FAIL_TEXT "the Workshop refused the upload"
#define CROSSTALK_CORE_PUBLISH_NOT_LIVE_TEXT "program and run a design first"
#define CROSSTALK_CORE_DISPLAY_USAGE_TEXT "display <hsync> <vsync> <video>"
#define CROSSTALK_CORE_DISPLAY_OK_TEXT "monitor wired to the design"
#define CROSSTALK_CORE_LD_FAIL_TEXT "could not program that design"
#define CROSSTALK_CORE_LD_DIAG_TOAST_CAP 128
#define CROSSTALK_CORE_LD_DIAG_TOAST_FMT "line %" PRIu32 ": %.95s"
#define CROSSTALK_CORE_RELOAD_SKIP_TEXT "design modified but not yet reloaded"
#define CROSSTALK_CORE_PRESENCE_NUM_REFRESH_MS 30000
#define CROSSTALK_CORE_POKE_USAGE_TEXT "poke <signal> [value]"
#define CROSSTALK_CORE_POKE_OK_TEXT "signal written"
#define CROSSTALK_CORE_POKE_RD_TEXT "value written to the log"
#define CROSSTALK_CORE_POKE_FAIL_TEXT "could not write that signal"
#define CROSSTALK_CORE_SWITCH_USAGE_TEXT "switch <name> [pos]"
#define CROSSTALK_CORE_SWITCH_OK_TEXT "switch moved"
#define CROSSTALK_CORE_SWITCH_RD_TEXT "position written to the log"
#define CROSSTALK_CORE_SWITCH_FAIL_TEXT "that position does not exist on this switch"
#define CROSSTALK_CORE_SWITCH_NONE_TEXT "no such switch"
#define CROSSTALK_CORE_SWITCH_DEEP_TEXT "switch cascade dropped; too deep"
#define CROSSTALK_CORE_SWITCH_EXEC_NAME "switch"
#define CROSSTALK_CORE_BENCH_CLK_HZ 1000000
#define CROSSTALK_CORE_TERM_CLOSE_WORD "close"
#define CROSSTALK_CORE_TERM_HOST_WORD "host"
#define CROSSTALK_CORE_TERM_HOST_WIN_SEP '\\'
#define CROSSTALK_CORE_TERM_HOST_CD_WIN_FMT "pushd \"%.1000s\"\r"
#define CROSSTALK_CORE_TERM_HOST_CD_UNIX_FMT "cd \"%.1000s\"\r"
#define CROSSTALK_CORE_TERM_HOST_LS_WIN "dir\r"
#define CROSSTALK_CORE_TERM_HOST_LS_UNIX "ls\r"
#define CROSSTALK_CORE_TERM_HOST_KEYS_CAP 1032
#define CROSSTALK_CORE_TERM_HOST_EDIT_WIN_FMT "if (Get-Command edit -ErrorAction SilentlyContinue) { edit \"%.200s\" }\r"
#define CROSSTALK_CORE_TERM_HOST_EDIT_UNIX_FMT "e=$(command -v vim || command -v nvim || command -v nano) && \"$e\" \"%.200s\"\r"
#define CROSSTALK_CORE_DEMO_SV_NAME "demo.sv"
#define CROSSTALK_CORE_DEMO_KEEP_NAME "demo_modified.sv"
#define CROSSTALK_CORE_DEMO_WATCH_CAP (1ULL * LDG_MIB)
#define CROSSTALK_CORE_DEMO_KEEP_TEXT "your demo edits are kept in demo_modified.sv"
#define CROSSTALK_CORE_TERM_DUMP_WORD "dump"
#define CROSSTALK_CORE_PTY_POOL_MIB 48
#define CROSSTALK_CORE_TERM_POOL_MIB 32
#define CROSSTALK_CORE_RENDER_HDR_SIZE 44
#define CROSSTALK_CORE_RENDER_CHUNK_SAMPLES 4096
#define CROSSTALK_CORE_RENDER_SECS_MAX 600
#define CROSSTALK_CORE_RENDER_RATE_MIN 8000
#define CROSSTALK_CORE_RENDER_RATE_MAX 192000
#define CROSSTALK_CORE_RENDER_BYTES_PER_SAMPLE 4
#define CROSSTALK_CORE_RENDER_USAGE_TEXT "render <port> <secs> <rate> <path>"
#define CROSSTALK_CORE_RENDER_OK_TEXT "render written"
#define CROSSTALK_CORE_RENDER_FAIL_TEXT "render failed"
#define CROSSTALK_CORE_SWITCH_NAME_CAP 64
#define CROSSTALK_CORE_DEFER_CAP 8
#define CROSSTALK_CORE_DEFER_TEXT_CAP 512
#define CROSSTALK_CORE_DEFER_SWITCH 0
#define CROSSTALK_CORE_DEFER_RUN 1
#define CROSSTALK_CORE_DEFER_EVENT 2
#define CROSSTALK_CORE_WATCH_CAP 12
#define CROSSTALK_CORE_WATCH_WORKERS 1
#define CROSSTALK_CORE_WATCH_CK_NS ((uint64_t)1 * LDG_NS_PER_SEC)
#define CROSSTALK_CORE_WATCH_SETTLE_NS ((uint64_t)500 * LDG_NS_PER_MS)
#define CROSSTALK_CORE_WATCH_YIELD_NS ((uint64_t)20 * LDG_NS_PER_MS)
#define CROSSTALK_CORE_RECEPT_KEY_CAP 16
#define CROSSTALK_CORE_RECEPT_SCAN_TAB 0x0F
#define CROSSTALK_CORE_RECEPT_SCAN_ESC 0x01
#define CROSSTALK_CORE_RECEPT_SCAN_LCTRL 0x1D
#define CROSSTALK_CORE_RECEPT_SCAN_LSHIFT 0x2A
#define CROSSTALK_CORE_RECEPT_SCAN_RSHIFT 0x36
#define CROSSTALK_CORE_RECEPT_SCAN_LALT 0x38
#define CROSSTALK_CORE_RECEPT_SCAN_UP 0x48
#define CROSSTALK_CORE_RECEPT_SCAN_LHS 0x4B
#define CROSSTALK_CORE_RECEPT_SCAN_RHS 0x4D
#define CROSSTALK_CORE_RECEPT_SCAN_DOWN 0x50
#define CROSSTALK_CORE_RECEPT_ASCII_MAX 127
#define CROSSTALK_CORE_RECEPT_IRQ_CAP 64
#define CROSSTALK_CORE_RECEPT_MOD_CUNT 3
#define CROSSTALK_CORE_RECEPT_ID_SPECIAL 0x100U
#define CROSSTALK_CORE_RECEPT_ID_MOD 0x200U
#define CROSSTALK_CORE_RECEPT_LOWER_FIRST 'a'
#define CROSSTALK_CORE_RECEPT_LOWER_LAST 'z'
#define CROSSTALK_CORE_RECEPT_CASE_BIT 0x20U
#define CROSSTALK_CORE_WATCH_KIND_EMU 0
#define CROSSTALK_CORE_WATCH_KIND_MEM 1
#define CROSSTALK_CORE_DELAY_NS_MAX ((uint64_t)10 * LDG_NS_PER_SEC)
#define CROSSTALK_CORE_DELAY_UNIT_NS "ns"
#define CROSSTALK_CORE_DELAY_UNIT_US "us"
#define CROSSTALK_CORE_DELAY_UNIT_MS "ms"
#define CROSSTALK_CORE_DELAY_UNIT_SEC "s"
#define CROSSTALK_CORE_DELAY_USAGE_TEXT "delay <n> [ns|us|ms|s]; a bare number is ns"
#define CROSSTALK_CORE_DELAY_DONE_FMT "delay: %" PRIu64 " ns"
#define CROSSTALK_CORE_EVENT_DEPTH_MAX 1
#define CROSSTALK_CORE_EVENT_USAGE_TEXT "on <load|save|exit|fault> <path.xts|off>; bare on lists"
#define CROSSTALK_CORE_EVENT_NONE_TEXT "no event scripts bound"
#define CROSSTALK_CORE_EVENT_OFF_TEXT "event script released"
#define CROSSTALK_CORE_EVENT_ON_TEXT "event script bound"
#define CROSSTALK_CORE_EVENT_ROW_FMT "on %.7s; path: %.100s"
#define CROSSTALK_CORE_EVENT_FMT "event: %.7s; path: %.100s"
#define CROSSTALK_CORE_BOARD_PREVIEW_LINE_CUNT 5
#define CROSSTALK_CORE_BOARD_PREVIEW_LINE_CAP 96
#define CROSSTALK_CORE_LOBBY_ITEMS_CAP 2560
#define CROSSTALK_CORE_MEMBER_ITEMS_CAP 1024
#define CROSSTALK_CORE_EXTRAS_CAP 32
#define CROSSTALK_CORE_EXTRAS_NAME_CAP 96
#define CROSSTALK_CORE_EXTRAS_ITEMS_CAP 3072
#define CROSSTALK_CORE_EXTRAS_DIR_REL "commentary"
#define CROSSTALK_CORE_THEME_DIR_REL "audio"
#define CROSSTALK_CORE_THEME_FILE "01crosstalk.wav"
#define CROSSTALK_CORE_THEME_VOL 0.05
#define CROSSTALK_CORE_CREDITS_FILE_REL "credits.txt"
#define CROSSTALK_CORE_CREDITS_ITEMS_CAP 16384
#define CROSSTALK_CORE_CREDITS_LINE_MAX 512
#define CROSSTALK_CORE_CREDITS_SEP ';'
#define CROSSTALK_CORE_EXTRAS_EXT_CUNT 1
#define CROSSTALK_CORE_EXTRAS_VTT_HD_CAP 2048
#define CROSSTALK_CORE_EXTRAS_TITLE_TAG "NOTE title:"
#define CROSSTALK_CORE_EXTRAS_SEP ';'
#define CROSSTALK_CORE_EXTRAS_STEP 1
#define CROSSTALK_CORE_DISCORD_INVITE_CMD "/invite"
#define CROSSTALK_CORE_DISCORD_INVITE_TEXT "join my board"
#define CROSSTALK_CORE_DISCORD_LINK_FAIL_TEXT "linking needs the official Discord client running"
#define CROSSTALK_CORE_NOTIFY_TITLE_DISCORD "Crosstalk - Discord"
#define CROSSTALK_CORE_NOTIFY_TITLE_STEAM "Crosstalk - Steam"
#define CROSSTALK_CORE_NOTIFY_TITLE_SESH "Crosstalk"
#define CROSSTALK_CORE_HUB_OTHER_GAME_TEXT "Playing other game"
#define CROSSTALK_CORE_MUSIC_NEEDS_NATIVE_TEXT "the in-game player is off; set Music source to Native in the settings"
#define CROSSTALK_CORE_OST_DIR_REL "music/crosstalk_thefundamentals"
#define CROSSTALK_CORE_OST_TRACK_CAP 32
#define CROSSTALK_CORE_OST_UNOWNED_TEXT "opening the soundtrack on Steam"
#define CROSSTALK_CORE_OST_DOWNLOAD_TEXT "downloading the soundtrack; press play again once it finishes"
#define CROSSTALK_CORE_OST_FAIL_TEXT "could not start the soundtrack"
#define CROSSTALK_CORE_OST_MISPLACED_TEXT "the soundtrack is installed but its tracks are not where the game looks"
#define CROSSTALK_CORE_MUSIC_PRESENCE_CAP 128
#define CROSSTALK_CORE_ACTIVITY_PRESENCE_CAP 96
#define CROSSTALK_CORE_STATE_PRESENCE_CAP 192
#define CROSSTALK_CORE_PRESENCE_JOIN_FMT "%.95s; %.63s"
#define CROSSTALK_CORE_TERM_PRESENCE_FMT "On the horn: %.31s"
#define CROSSTALK_CORE_FPGA_PRESENCE_FMT "Simulating \"%.31s\""
#define CROSSTALK_CORE_FPGA_PRESENCE_RATE_FMT "Simulating \"%.31s\" @ %.7s Hz; %.7s cycles in"
#define CROSSTALK_CORE_EXEC_PRESENCE_FMT "Executing \"%.31s\""
#define CROSSTALK_CORE_EXEC_PRESENCE_RATE_FMT "Executing \"%.31s\" @ %.7s Hz; %.7s cycles in"
#define CROSSTALK_CORE_EMU_PRESENCE_TEXT "Running the DLU"
#define CROSSTALK_CORE_EMU_PRESENCE_RATE_FMT "Running the DLU @ %.7s Hz; %.7s cycles in"
#define CROSSTALK_CORE_EMU_PROG_PRESENCE_FMT "Running %.31s on the DLU"
#define CROSSTALK_CORE_EMU_PROG_PRESENCE_RATE_FMT "Running %.31s on the DLU @ %.7s Hz; %.7s cycles in"
#define CROSSTALK_CORE_HUNT_PRESENCE_TEXT "On the hunt in memory"
#define CROSSTALK_CORE_PRESENCE_SI_FMT "%" PRIu64 "%s"
#define CROSSTALK_CORE_PRESENCE_SI_CAP 16
#define CROSSTALK_CORE_PTY_PRESENCE_TEXT "Pseudoterminalicious"
#define CROSSTALK_CORE_STREAM_PRESENCE_TEXT "Streaming a terminal"
#define CROSSTALK_CORE_CAST_PRESENCE_FMT "Casting the %.15s to %.31s"
#define CROSSTALK_CORE_PIPE_PRESENCE_TEXT "Piping to the wire"
#define CROSSTALK_CORE_BUILD_PRESENCE_FMT "Compiling \"%.31s\""
#define CROSSTALK_CORE_WINDOW_PRESENCE_FMT "Viewing: %.31s"
#define CROSSTALK_CORE_PROJ_PRESENCE_FMT "In \"%.31s\""
#define CROSSTALK_CORE_WINDOW_NAME_SCOPE "scope"
#define CROSSTALK_CORE_WINDOW_NAME_MEMVIEW "memview"
#define CROSSTALK_CORE_WINDOW_NAME_CRT "CRT"
#define CROSSTALK_CORE_WINDOW_NAME_PROFILE "profile"
#define CROSSTALK_CORE_WINDOW_NAME_CALC "calc"
#define CROSSTALK_CORE_WINDOW_NAME_INDICATOR "indicators"
#define CROSSTALK_CORE_WINDOW_NAME_WSHOP "workshop"
#define CROSSTALK_CORE_WINDOW_NAME_WSHOP_PUB "workshop publish"
#define CROSSTALK_CORE_WINDOW_NAME_TERM "terminal"
#define CROSSTALK_CORE_MUSIC_POOL_WORKERS 1
#define CROSSTALK_CORE_PERSONA_UNKNOWN "a Steam friend"
#define CROSSTALK_CORE_NOTIFY_ICO_REL "img/crosstalk.ico"
#define CROSSTALK_CORE_NOTIFY_TIP "Crosstalk"
#define CROSSTALK_CORE_CHAT_SESH_NAME "session"
#define CROSSTALK_CORE_CHAT_LOBBY_NAME "lobby"
#define CROSSTALK_CORE_DISCORD_TKT_CAP 1024
#define CROSSTALK_CORE_DISCORD_TKT_HEX_CAP 2048
#define CROSSTALK_CORE_DISCORD_TKT_IDENT "discord"
#define CROSSTALK_CORE_DISCORD_HEX_PER_BYTE 2
#define CROSSTALK_CORE_DISCORD_HEX_HI 0
#define CROSSTALK_CORE_DISCORD_HEX_LO 1
#define CROSSTALK_CORE_DISCORD_HEX_PAIR_CAP 3
#define CROSSTALK_CORE_PEER_STATE_PERIOD 2.0
#define CROSSTALK_CORE_PEER_STATE_AFK_SECS 180.0
#define CROSSTALK_CORE_SOUNDTRACK_APP_ID 4912620U
#define CROSSTALK_CORE_STORY_APP_ID 4912640U
#define CROSSTALK_CORE_REVERSE_APP_ID 5158190U
#define CROSSTALK_CORE_DLC_DISK_CUNT 9
#define CROSSTALK_CORE_DLC_DISK_APP_IDS { 5204060U, 5210440U, 5188130U, 5188120U, 5188110U, 5182610U, 5182600U, 5170760U, 5173920U }
#define CROSSTALK_CORE_DLC_DISK_DIR_REL "dlc"
#define CROSSTALK_CORE_DLC_DISK_FILE "disk.dsk"
#define CROSSTALK_CORE_DLC_DISK_APP_ID_CAP 16
#define CROSSTALK_CORE_DLC_DRIVE_FIRST 1
#define CROSSTALK_CORE_GUIDE_TOUR_NAME "tour"
#define CROSSTALK_CORE_GUIDE_TOUR_IDX 0
#define CROSSTALK_CORE_GUIDE_DONE_LD_EXAMPLE 0x4C444558ULL
#define CROSSTALK_CORE_GUIDE_DONE_IND_EXAMPLE 0x494E4558ULL
#define CROSSTALK_CORE_GUIDE_EXAMPLE_TOP "blinky"
#define CROSSTALK_CORE_GUIDE_EXAMPLE_PORT "leds"
#define CROSSTALK_CORE_NEWRTL_SCRIPT_REL "newrtlproj.xts"
#define CROSSTALK_CORE_DEMO_DIR_REL "demo"
#define CROSSTALK_CORE_DEMO_SCRIPT_NAME "demo.xts"
#define CROSSTALK_CORE_GUIDE_DEMO_PICK_CUNT 2
#define CROSSTALK_CORE_GUIDE_PICK_DEMO 1
#define CROSSTALK_CORE_GUIDE_PICK_EMU 2
#define CROSSTALK_CORE_NEWPROJECT_KIND_RTL 0U
#define CROSSTALK_CORE_NEWPROJECT_KIND_RETIRED 1U
#define CROSSTALK_CORE_NEWPROJECT_KIND_REVERSE 2U
#define CROSSTALK_CORE_EDITOR_KIND_CUNT 3U
#define CROSSTALK_CORE_LAYOUT_DUMP_TEXT "dump"
#define CROSSTALK_CORE_EDITOR_RTL_TEXT "RTL"
#define CROSSTALK_CORE_EDITOR_RETIRED_TEXT ""
#define CROSSTALK_CORE_EDITOR_REVERSE_TEXT "Reverse"
#define CROSSTALK_CORE_EDITORPICK_ITEMS_CAP 64
#define CROSSTALK_CORE_NEWPROJECT_PICK_NEW 0U
#define CROSSTALK_CORE_NEWPROJECT_ITEMS_CAP 6144
#define CROSSTALK_CORE_NEWPROJECT_PICK_NEW_TEXT "New project"
#define CROSSTALK_CORE_NEWPROJECT_SEP ";"
#define CROSSTALK_CORE_NEWPROJECT_SEP_CHAR ';'
#define CROSSTALK_CORE_NEWPROJECT_SEP_SUB '_'
#define CROSSTALK_CORE_STEAM_APP_ID_ENV "SteamAppId"
#define CROSSTALK_CORE_STEAM_GAME_ID_ENV "SteamGameId"
#define CROSSTALK_CORE_STEAM_APP_ID_CAP 24
#define CROSSTALK_CORE_IDENT_CAP 16
#define CROSSTALK_CORE_IDENT_FILE "ident.dat"
#define CROSSTALK_CORE_IDENT_MODE 0600
#define CROSSTALK_CORE_LOBBY_TOAST_PAD 32

typedef struct crosstalk_ctx
{
    ldg_sched_ctx_t sched;
    ldg_net_steamworks_ctx_t net;
    ldg_net_cb_t net_cb;
    steam_net_ctx_t steam_net;
    crosstalk_net_discord_ctx_t discord;
    crosstalk_net_inventory_ctx_t inventory;
    crosstalk_net_relay_ctx_t relay;
    uint32_t cpuset_id[CROSSTALK_CORE_CPUSET_SIBLING_CAP];
    uint32_t cpuset_cunt;
    uint8_t cpuset_ok;
    uint8_t discord_live;
    uint8_t discord_auth_req;
    uint8_t discord_restore_req;
    uint8_t discord_relink_req;
    uint8_t steam_live;
    uint8_t discord_pudding[4];
    uint64_t join_lobby_pending;
    uint64_t connect_lobby_prev;
    uint64_t discord_invite_prev;
    crosstalk_game_progression_queue_ctx_t progression_queue;
    uint8_t progression_queue_ok;
    uint8_t progression_pudding[7];
    char ost_path[CROSSTALK_CORE_OST_TRACK_CAP][LDG_IO_PATH_MAX];
    const char *ost_ptr[CROSSTALK_CORE_OST_TRACK_CAP];
    uint32_t ost_cunt;
    uint8_t soundtrack_owned;
    uint8_t ost_started;
    uint8_t soundtrack_pudding[2];
    ldg_term_sesh_t *sesh;
    ldg_term_input_t *irq_src;
    crosstalk_core_shot_t shot;
    crosstalk_core_watch_t demo_watch;
    crosstalk_core_watch_set_t design_watch;
    crosstalk_core_fetch_ctx_t fetch;
    uint64_t toast_until_ms;
    char toast_text[CROSSTALK_CORE_TOAST_TEXT_CAP];
    crosstalk_emu_net_ctx_t emu_net;
    char emu_net_path[LDG_IO_PATH_MAX];
    uint64_t presence_rate;
    uint64_t presence_cyc;
    uint64_t presence_num_next_ms;
    crosstalk_game_menu_nav_t menu_nav;
    crosstalk_game_settings_menu_ctx_t settings_menu;
    uint32_t menu_sel;
    uint32_t menu_action;
    uint32_t menu_row;
    uint32_t menu_screen_prev;
    uint32_t term_col_cunt;
    uint32_t term_row_cunt;
    uint32_t mp_transport_prev;
    uint8_t board_sel_wshop_prev;
    uint8_t menu_confirm;
    uint8_t key_esc;
    uint8_t key_save;
    uint8_t key_open;
    uint8_t key_invite;
    uint8_t key_esc_shift;
    uint8_t key_pudding[5];
    crosstalk_core_cmdline_ctx_t cmdline;
    char defer_text[CROSSTALK_CORE_DEFER_CAP][CROSSTALK_CORE_DEFER_TEXT_CAP];
    uint32_t defer_kind[CROSSTALK_CORE_DEFER_CAP];
    uint32_t defer_cunt;
    uint32_t event_depth;
    ldg_thread_pool_t watch_pool;
    ldg_mut_t watch_mut;
    char watch_path[CROSSTALK_CORE_WATCH_CAP][LDG_IO_PATH_MAX];
    uint64_t watch_mtime[CROSSTALK_CORE_WATCH_CAP];
    uint64_t watch_span[CROSSTALK_CORE_WATCH_CAP];
    uint32_t watch_kind[CROSSTALK_CORE_WATCH_CAP];
    uint32_t watch_slot[CROSSTALK_CORE_WATCH_CAP];
    uint8_t watch_stale[CROSSTALK_CORE_WATCH_CAP];
    uint8_t theme_playing;
    uint8_t theme_done;
    uint32_t watch_cunt;
    uint32_t watch_stop;
    uint8_t watch_ok;
    uint8_t prof_on;
    uint8_t prof_pudding[2];
    uint32_t dsu_probe_handle;
    uint32_t dsu_stream_handle;
    uint64_t dsu_sample_at;
    uint64_t dsu_base;
    uint64_t dsu_tail;
    uint64_t dsu_curr;
    uint32_t sound_probe_handle;
    uint32_t sound_stream_handle;
    uint32_t scope_probe_handle;
    uint32_t scope_stream_handle;
    uint32_t pack_prog_gen;
    uint32_t scope_prog_gen;
    uint64_t event_fault_step;
    uint8_t event_fault_has;
    uint8_t event_fault_pudding[7];
    char serial_map_pattern[CROSSTALK_CORE_SERIAL_MAP_CAP][CROSSTALK_CORE_SERIAL_MAP_PATTERN_CAP];
    char serial_map_action[CROSSTALK_CORE_SERIAL_MAP_CAP][CROSSTALK_CORE_SERIAL_MAP_ACTION_CAP];
    uint32_t serial_map_pattern_len[CROSSTALK_CORE_SERIAL_MAP_CAP];
    uint32_t serial_map_cunt;
    uint8_t serial_map_window[CROSSTALK_CORE_SERIAL_MAP_PATTERN_CAP];
    uint32_t serial_map_window_len;
    char serial_map_pending[CROSSTALK_CORE_SERIAL_MAP_ACTION_CAP];
    uint8_t serial_map_pending_has;
    uint8_t serial_map_pudding[7];
    uint32_t serial_rx_probe_handle;
    uint32_t serial_tx_probe_handle;
    uint64_t serial_rx_sample_at;
    uint64_t serial_tx_sample_at;
    uint8_t console_exec_is;
    uint8_t defer_pudding[7];
    crosstalk_core_scan_ctx_t scan;
    uint8_t prof_en_on;
    uint8_t persist_pending;
    uint8_t quit_no_persist;
    uint8_t scope_probe_owned;
    uint8_t brk_on;
    uint64_t brk_addr;
    uint64_t emu_step_on;
    uint32_t recept_key[CROSSTALK_CORE_RECEPT_KEY_CAP];
    uint8_t recept_code[CROSSTALK_CORE_RECEPT_KEY_CAP];
    uint32_t recept_key_cunt;
    ldg_term_key_event_t recept_irq[CROSSTALK_CORE_RECEPT_IRQ_CAP];
    uint32_t recept_irq_cunt;
    char board_preview_line[CROSSTALK_CORE_BOARD_PREVIEW_LINE_CUNT][CROSSTALK_CORE_BOARD_PREVIEW_LINE_CAP];
    uint32_t board_preview_row;
    uint8_t board_preview_ok;
    uint8_t settings_open;
    uint8_t quit_armed;
    uint8_t menu_pudding[1];
    uint64_t st_time_ms;
    uint64_t instability_prev_tsc;
    double instability_next;
    ldg_sched_event_desc_t fixed_event;
    ldg_sched_event_desc_t step_event;
    crosstalk_net_wire_ident_t ident[CROSSTALK_CORE_IDENT_CAP];
    uint32_t ident_cunt;
    crosstalk_net_wire_peer_state_t peer_state[CROSSTALK_CORE_IDENT_CAP];
    uint32_t peer_state_cunt;
    double peer_state_next;
    double idle_secs;
    uint8_t peer_state_sent;
    uint8_t paused_by_focus;
    uint8_t remote_sesh;
    uint8_t snap_req_sent;
    uint8_t timeline_screen_prev;
    uint8_t timeline_set;
    uint8_t peer_pudding[2];
    crosstalk_eda_ctx_t eda;
    crosstalk_core_task_ctx_t task;
    crosstalk_ui_window_ctx_t win_nav;
    crosstalk_ui_guide_ctx_t guide;
    crosstalk_hdl_elf_ctx_t elf;
    crosstalk_hdl_elf_ctx_t prog_elf;
    uint32_t ach_io_seen;
    char prog_pub_name[CROSSTALK_CORE_PROG_PUB_NAME_CAP];
    crosstalk_hdl_type_ctx_t prog_type;
    crosstalk_eda_ic_fpga_emu_stat_t emu_stat;
    char memview_title[CROSSTALK_EDA_UI_MEMVIEW_TITLE_CAP];
    uint64_t freeze_addr[CROSSTALK_CORE_FREEZE_MAX];
    uint64_t freeze_val[CROSSTALK_CORE_FREEZE_MAX][CROSSTALK_CORE_FREEZE_LEN_MAX];
    uint32_t freeze_len[CROSSTALK_CORE_FREEZE_MAX];
    uint32_t freeze_cunt;
    crosstalk_re_ctx_t re;
    crosstalk_reng_ctx_t reng;
    crosstalk_reng_progress_ctx_t reng_progress;
    crosstalk_reng_tasks_ctx_t reng_tasks;
    uint8_t *reng_sect_buff;
    uint32_t reng_sect_len;
    uint8_t re_diff_pending;
    uint8_t re_pudding[3];
    crosstalk_game_ctx_t game;
    double dt;
    double time;
    double tick_period;
    double memwatch_notice_next;
    double sim_run_since;
    double perf_delta_sum;
    double perf_window_elapsed;
    double perf_delta_average;
    uint64_t tick_prev;
    uint32_t perf_delta_cunt;
    uint8_t perf_pudding[4];
    char res_dir[LDG_IO_PATH_MAX];
    char font_path[LDG_IO_PATH_MAX];
    char save_path[LDG_IO_PATH_MAX];
    char editor_file[LDG_IO_PATH_MAX];
    char save_dir[LDG_IO_PATH_MAX];
    char wshop_dir[LDG_IO_PATH_MAX];
    char board_name[CROSSTALK_GAME_BOARD_NAME_CAP];
    crosstalk_net_wshop_ctx_t wshop;
    crosstalk_wshop_list_t wshop_list;
    crosstalk_wshop_item_t wshop_core;
    crosstalk_wshop_manifest_t wshop_pub;
    char wshop_stage_root[LDG_IO_PATH_MAX];
    char wshop_stage_dir[LDG_IO_PATH_MAX];
    ldg_net_steamworks_wshop_upload_ctx_t wshop_up;
    uint32_t wshop_up_state;
    char lobby_items[CROSSTALK_CORE_LOBBY_ITEMS_CAP];
    char member_items[CROSSTALK_CORE_MEMBER_ITEMS_CAP];
    uint32_t mp_board_idx;
    uint32_t mp_players_idx;
    uint32_t mp_submode_idx;
    uint32_t mp_transport_idx;
    uint32_t mp_joinable_cunt;
    uint64_t presence_group;
    uint32_t presence_players;
    uint32_t mp_players_max;
    uint8_t presence_kind;
    uint8_t presence_pudding[3];
    uint8_t mp_pudding[1];
    uint8_t mp_rdy;
    uint8_t mp_host_pending;
    uint8_t mp_lobby_state_prev;
    uint8_t mp_host_retry;
    ldg_net_lobby_member_t lobby_member_prev[LDG_NET_LOBBY_MEMBER_MAX];
    uint32_t lobby_member_prev_cunt;
    crosstalk_game_settings_t settings_applied;
    uint8_t settings_applied_ok;
    uint8_t mp_busy;
    uint8_t lobby_member_pudding[2];
    double mp_host_retry_next;
    double serial_access_last;
    double serial_access_grant_next;
    uint64_t serial_access_owner;
    crosstalk_game_chat_ctx_t chat;
    crosstalk_game_hub_ctx_t hub;
    crosstalk_game_music_ctx_t music;
    crosstalk_game_board_ctx_t board_sel;
    uint32_t board_sel_active;
    uint32_t board_sel_confirm_row;
    uint32_t extras_cunt;
    uint32_t credits_cunt;
    double caption_clock;
    uint8_t extras_playing;
    char extras_items[CROSSTALK_CORE_EXTRAS_ITEMS_CAP];
    char credits_items[CROSSTALK_CORE_CREDITS_ITEMS_CAP];
    char extras_file[CROSSTALK_CORE_EXTRAS_CAP][CROSSTALK_CORE_EXTRAS_NAME_CAP];
    uint64_t board_sel_confirm_ms;
    uint64_t board_sel_lock_ms;
    uint32_t pty_want_cols_prev;
    uint32_t pty_want_rows_prev;
    uint32_t pty_want_stable;
    char newproject_items[CROSSTALK_CORE_NEWPROJECT_ITEMS_CAP];
    uint32_t newproject_row[CROSSTALK_GAME_BOARD_CAP];
    char editorpick_items[CROSSTALK_CORE_EDITORPICK_ITEMS_CAP];
    uint8_t editorpick_kind[CROSSTALK_CORE_EDITOR_KIND_CUNT];
    uint32_t editorpick_cunt;
    uint32_t editorpick_idx;
    uint8_t editorpick_mask;
    uint8_t editorpick_pudding[7];
    uint32_t newproject_pick;
    uint32_t newproject_pick_cunt;
    uint8_t board_sel_wshop_on;
    uint8_t editor_kind;
    uint8_t reverse_setup_pending;
    uint8_t emu_default_pending;
    uint8_t newproject_kind;
    uint8_t rtl_restore_pending;
    uint8_t demo_pending;
    uint8_t newproject_pudding[1];
    uint32_t app_id;
    uint32_t pipe_handle;
    uint8_t card_kind;
    uint8_t card_kind_set;
    uint8_t splash_cw_rolled;
    uint8_t ld_frames;
    uint8_t crt_pudding[4];
} crosstalk_ctx_t;

uint32_t crosstalk_core_runtime_path_resolve(const char *file, char *out, uint64_t out_size);
uint32_t crosstalk_core_log_path_resolve(char *out, uint64_t out_size);
uint32_t crosstalk_core_launch_app_id_get(void);
uint8_t crosstalk_core_soundtrack_owned_is(const crosstalk_ctx_t *ctx);
uint32_t crosstalk_core_create(crosstalk_ctx_t **out);
uint32_t crosstalk_core_run(crosstalk_ctx_t *ctx);
uint32_t crosstalk_core_join_lobby_set(crosstalk_ctx_t *ctx, uint64_t lobby_id);
uint32_t crosstalk_core_save_wr(crosstalk_ctx_t *ctx);
uint32_t crosstalk_core_destroy(crosstalk_ctx_t *ctx);

#endif
