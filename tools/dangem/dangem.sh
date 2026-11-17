#!/usr/bin/env bash
set -uo pipefail
export LC_ALL=C
export TZ=UTC

GH_REPO="danglingptr0x0/crosstalk"
CC_REPO="danglingptr0x0/llvm-project-dangcsa_dangxa"
LDG_REPO="danglingptr0x0/libdangling"
LRS_REPO="danglingptr0x0/libresteam"
GH_API="https://api.github.com/repos"
GH_DL="https://github.com"
ASSET_SFX="-linux-amd64.tar.xz"
SYS_PREFIX="/usr/local"
SYS_LIB="$SYS_PREFIX/lib"
LD_AS_NEEDED="-Wl,--as-needed"
DISCORD_TAG="discordsdk"
DISCORD_DIR="DiscordSocialSdk-1.10.18247"
DISCORD_ZIP="$DISCORD_DIR.zip"
ROOT="$HOME/.local/share/crosstalk"
SELF="${BASH_SOURCE[0]:-}"
if [[ -f "$SELF" && -d "$(dirname -- "$SELF")/../../src" ]]; then ROOT="$(realpath -- "$(dirname -- "$SELF")/../..")"; fi
BUILD_DIR="$ROOT/build"
OBJ_DIR="$BUILD_DIR/obj"
JOB_DIR="$BUILD_DIR/obj/.jobs"
LIST_PATH="$BUILD_DIR/dangem/build-linux.lst"
RELEASE_PATH="$BUILD_DIR/dangem/RELEASE"
STAMP_PATH="$BUILD_DIR/stamp"
CFLAGS_RSP="$BUILD_DIR/cflags.rsp"
LINK_RSP="$BUILD_DIR/link.rsp"
LOG_PATH="$ROOT/dangem.log"
DL_DIR="$ROOT/.dl"
TREE_DIR="$ROOT"
SRC_DIR="$ROOT/src"
CC_DIR="$ROOT/res/utils/cc"

UNAME="$(uname -a)"
TITLE_MENU=" DANGLING Ecosystem Manager r0.1"
TITLE_SUMMARY="DANGLING Ecosystem Manager r0.1"
MENU_EXIT="(F10 = Exit)"
MENU_EXIT_W=12
SUMMARY_KEYS="F2=Launch - F5=Rebuild - Esc=Menu - F10=Quit"
SMALL_TEXT="terminal too small"
LAUNCH_CONFIRM_TEXT="The latest source is newer than the compiled binaries. Launch anyway (press again)?"
DEPS_CONFIRM_TEXT="The dependencies are already in place. Obtain them again (press again)?"
LAUNCH_ARM_SPAN=3000
FRAME_WAIT=0.016
ESC_WAIT=0.01
ESC_SEQ_MAX=5
COL_MAX=512
ROW_MAX=256
JOB_MAX=64
ERR_CAP=16383
ERR_NOT_INIT=7
ERR_BUSY=11

ESC=$'\e'
NL=$'\n'
NEXT_LINE_RE="^[^$NL]*$NL(.*)$"
printf -v BAR '\x23'
BG="48;2;10;10;20"
TONE_NORMAL=0
TONE_BRIGHT=1
TONE_DIM=2
TONE_INVERSE=3
TONE_EDGE=4
TONE_FILL=5
TONE_ALERT=6
TONE_SEQ=(
    "$ESC[0;38;2;207;210;255;${BG}m"
    "$ESC[0;38;2;234;240;255;${BG}m"
    "$ESC[0;38;2;143;176;255;${BG}m"
    "$ESC[0;38;2;255;255;255;48;2;61;108;255m"
    "$ESC[0;38;2;191;255;0;${BG}m"
    "$ESC[0;38;2;61;108;255;${BG}m"
    "$ESC[0;38;2;200;50;50;${BG}m"
)

PHASE=idle
LIST_OK=0
CC_OK=0
CC=""
RELEASE=""
OUT_PATH=""
SRC_NEWER=0
SRC_TOTAL=0
SRC_DONE=0
JOB_CUNT=1
LIVE_CUNT=0
ITEM_NEXT=0
BUILD_START=0
BUILD_END=0
PREV_DURATION=0
FAIL_SRC=""
FAIL_TEXT=""
DEPS_OK=0
declare -A KEYV
ITEM_KIND=()
ITEM_VAL=()
ITEM_CUNT=0
FETCH_NAME=""
ASSET_URL=""
ASSET_TAG=""
JOB_PID=()
JOB_ITEM=()

SCREEN=menu
QUIT=0
LAUNCH=0
LAUNCH_AFTER=0
LAUNCH_CONFIRM=0
LAUNCH_CONFIRM_TIME=0
DEPS_CONFIRM=0
DEPS_CONFIRM_TIME=0
NOTICE=""
VIEW_LAUNCH=""
VIEW_BUILD=""
VIEW_DEPS=""
BUILD_ALERT=0
COL_CUNT=0
ROW_CUNT=0
FRAME_BUFF=""
TTY_SAVED=""

time_epoch_get()
{
    local epoch_real="${EPOCHREALTIME:-}"

    if [[ -z "$epoch_real" ]]; then
        date +%s%3N
        return 0
    fi

    epoch_real="${epoch_real/./}"
    echo $((epoch_real / 1000))
}

logging_line_wr()
{
    mkdir -p "$ROOT" || return 0
    printf '%s %s %s\n' "$(time_epoch_get)" "$1" "$2" >>"$LOG_PATH"
}

sys_tool_has()
{
    [[ -n "$(type -P "$1")" ]]
}

run_tty_ensure()
{
    local self="${BASH_SOURCE[0]:-}"
    local name

    if [[ -t 1 ]]; then return 0; fi

    if [[ -n "${DANGEM_TERM_SPAWNED:-}" ]]; then exit 1; fi

    if [[ ! -f "$self" ]]; then exit 1; fi

    export DANGEM_TERM_SPAWNED=yes

    if [[ -n "${TERMINAL:-}" ]] && sys_tool_has "$TERMINAL"; then exec "$TERMINAL" -e "$self" "$@"; fi

    if sys_tool_has wezterm; then exec wezterm start --always-new-process -- "$self" "$@"; fi

    for name in x-terminal-emulator konsole; do
        if sys_tool_has "$name"; then exec "$name" -e "$self" "$@"; fi
    done

    if sys_tool_has gnome-terminal; then exec gnome-terminal -- "$self" "$@"; fi

    if sys_tool_has xfce4-terminal; then exec xfce4-terminal -x "$self" "$@"; fi

    if sys_tool_has alacritty; then exec alacritty -e "$self" "$@"; fi

    if sys_tool_has kitty; then exec kitty -- "$self" "$@"; fi

    if sys_tool_has xterm; then exec xterm -e "$self" "$@"; fi

    exit 1
}

compile_list_ld()
{
    local line key val

    KEYV=()
    ITEM_KIND=()
    ITEM_VAL=()
    ITEM_CUNT=0

    if [[ ! -f "$LIST_PATH" ]]; then return 1; fi

    while IFS= read -r line || [[ -n "$line" ]]; do
        line="${line%$'\r'}"

        if [[ ! "$line" =~ ^([^ ]*)\ (.*)$ ]]; then continue; fi

        key="${BASH_REMATCH[1]}"
        val="${BASH_REMATCH[2]}"

        if [[ -z "$val" ]]; then continue; fi

        case "$key" in
            tgt | out | std | cflagset | ldflagset | libset) KEYV[$key]="$val" ;;
            src | def | inc | cflag | lib | ldflag)
                ITEM_KIND+=("$key")
                ITEM_VAL+=("$val")
                ITEM_CUNT=$((ITEM_CUNT + 1))
                ;;
        esac
    done <"$LIST_PATH"

    return 0
}

compile_list_map()
{
    MAPPED="$1"

    if [[ "$1" == src/* ]]; then MAPPED="$TREE_DIR/$1"; fi
}

path_leaf_get()
{
    LEAF="$1"

    if [[ "$1" =~ ([^/]*)$ ]]; then LEAF="${BASH_REMATCH[1]}"; fi
}

build_rsp_arg()
{
    local arg="$1$2"

    if [[ -z "$2" ]]; then return 0; fi

    arg="${arg//\\/\\\\}"
    arg="${arg//\"/\\\"}"
    RSP+="\"$arg\""$'\n'
}

build_rsp_set()
{
    local tok

    for tok in ${KEYV[$1]:-}; do
        compile_list_map "$tok"
        build_rsp_arg "" "$MAPPED"
    done
}

build_item_compiled_is()
{
    [[ "${ITEM_KIND[$1]}" == "src" && "${ITEM_VAL[$1]}" != *.rc ]]
}

build_obj_path_get()
{
    local src="$1"

    if [[ "$src" == "$SRC_DIR"* ]]; then src="src${src/"$SRC_DIR"/}"; fi

    if [[ "$src" == "$ROOT"* ]]; then src="${src/"$ROOT"/}"; fi

    while [[ "$src" == /* || "$src" == \\* ]]; do src="${src:1}"; done

    src="${src//\//_}"
    src="${src//\\/_}"
    src="${src//:/_}"
    OBJ="$OBJ_DIR/$src.o"
}

build_cflags_wr()
{
    local idx kind

    RSP=""

    if [[ -n "${KEYV[std]:-}" ]]; then build_rsp_arg "-std=c" "${KEYV[std]}"; fi

    if [[ -n "${KEYV[tgt]:-}" ]]; then build_rsp_arg "--target=" "${KEYV[tgt]}"; fi

    build_rsp_set cflagset
    build_rsp_arg "" "-march=native"

    for idx in "${!ITEM_KIND[@]}"; do
        kind="${ITEM_KIND[$idx]}"

        if [[ "$kind" != "cflag" && "$kind" != "def" && "$kind" != "inc" ]]; then continue; fi

        compile_list_map "${ITEM_VAL[$idx]}"

        if [[ "$kind" == "cflag" ]]; then
            build_rsp_arg "" "$MAPPED"
        elif [[ "$kind" == "def" ]]; then
            build_rsp_arg "-D" "$MAPPED"
        elif [[ "$MAPPED" == "$SRC_DIR"* ]]; then
            build_rsp_arg "-I" "$MAPPED"
        elif [[ -d "$MAPPED" ]]; then
            build_rsp_arg "" "-isystem"
            build_rsp_arg "" "$MAPPED"
        fi
    done

    printf '%s' "$RSP" >"$CFLAGS_RSP"
}

build_link_wr()
{
    local idx val

    RSP=""

    if [[ -n "${KEYV[tgt]:-}" ]]; then build_rsp_arg "--target=" "${KEYV[tgt]}"; fi

    build_rsp_arg "" "-fuse-ld=lld"
    build_rsp_arg "" '-Wl,-rpath,$ORIGIN'
    build_rsp_arg "" "$LD_AS_NEEDED"
    build_rsp_set ldflagset
    build_rsp_arg "" "-o"
    build_rsp_arg "" "$OUT_PATH"

    for idx in "${!ITEM_KIND[@]}"; do
        if ! build_item_compiled_is "$idx"; then continue; fi

        compile_list_map "${ITEM_VAL[$idx]}"
        build_obj_path_get "$MAPPED"
        build_rsp_arg "" "$OBJ"
    done

    for idx in "${!ITEM_KIND[@]}"; do
        if [[ "${ITEM_KIND[$idx]}" != "ldflag" ]]; then continue; fi

        compile_list_map "${ITEM_VAL[$idx]}"
        build_rsp_arg "" "$MAPPED"
    done

    build_rsp_arg "-L" "$ROOT"
    build_rsp_arg "-L" "$SYS_LIB"

    for idx in "${!ITEM_KIND[@]}"; do
        if [[ "${ITEM_KIND[$idx]}" != "lib" ]]; then continue; fi

        val="${ITEM_VAL[$idx]}"
        compile_list_map "$val"

        if [[ "$val" == /* ]]; then
            if [[ -e "$MAPPED" ]]; then
                build_rsp_arg "" "$MAPPED"
            else
                path_leaf_get "$val"
                build_rsp_arg "-l:" "$LEAF"
            fi
        elif [[ "$val" == -* ]]; then
            build_rsp_arg "" "$MAPPED"
        else
            build_rsp_arg "-l" "$MAPPED"
        fi
    done

    build_rsp_set libset

    printf '%s' "$RSP" >"$LINK_RSP"
}

build_cc_find()
{
    CC="$CC_DIR/linux/bin/clang"

    [[ -x "$CC" ]]
}

build_src_newer_ck()
{
    local dir hit

    SRC_NEWER=0

    if [[ -z "$OUT_PATH" || ! -e "$OUT_PATH" ]]; then return 0; fi

    for dir in "$SRC_DIR" "$BUILD_DIR/dangem" "$BUILD_DIR/gen"; do
        if [[ ! -d "$dir" ]]; then continue; fi

        hit="$(find "$dir" -maxdepth 16 -name '.*' -prune -o -type f -newer "$OUT_PATH" -print -quit)"

        if [[ -n "$hit" ]]; then
            SRC_NEWER=1
            return 0
        fi
    done
}

compile_init()
{
    local idx cpu line

    PHASE=idle
    SRC_TOTAL=0
    SRC_DONE=0
    OUT_PATH=""
    RELEASE=""
    PREV_DURATION=0

    if compile_list_ld; then LIST_OK=1; else LIST_OK=0; fi

    if build_cc_find; then CC_OK=1; else CC_OK=0; fi

    for idx in "${!ITEM_KIND[@]}"; do
        if build_item_compiled_is "$idx"; then SRC_TOTAL=$((SRC_TOTAL + 1)); fi
    done

    if [[ "$LIST_OK" == 1 && -n "${KEYV[out]:-}" ]]; then OUT_PATH="$ROOT/${KEYV[out]}"; fi

    cpu="$(nproc)"
    if [[ -z "$cpu" || "$cpu" == 0 ]]; then cpu=1; fi

    JOB_CUNT=$((cpu < JOB_MAX ? cpu : JOB_MAX))

    if [[ -f "$RELEASE_PATH" ]]; then
        IFS= read -r line <"$RELEASE_PATH"
        line="${line%$'\r'}"
        RELEASE="${line:0:63}"
    fi

    if [[ -f "$STAMP_PATH" ]]; then
        {
            IFS= read -r line
            IFS= read -r line
        } <"$STAMP_PATH"
        if [[ "$line" =~ ^[0-9]+$ ]]; then PREV_DURATION="$line"; fi
    fi

    build_src_newer_ck
}

compile_stale_is()
{
    local line

    if [[ -z "$OUT_PATH" || ! -e "$OUT_PATH" ]]; then return 0; fi

    if [[ "$SRC_NEWER" == 1 ]]; then return 0; fi

    if [[ ! -f "$STAMP_PATH" ]]; then return 0; fi

    IFS= read -r line <"$STAMP_PATH"

    [[ "$line" != "$RELEASE" ]]
}

compile_busy_is()
{
    [[ "$PHASE" == "compile" || "$PHASE" == "link" ]]
}

build_fail_set()
{
    if [[ -n "$FAIL_TEXT" || -n "$FAIL_SRC" ]]; then return 0; fi

    FAIL_SRC="$1"
    FAIL_TEXT="${2:0:$ERR_CAP}"

    if [[ -z "$FAIL_TEXT" ]]; then FAIL_TEXT=" "; fi
}

build_failed_is()
{
    [[ -n "$FAIL_TEXT" ]]
}

build_job_spawn()
{
    local slot="$1"
    local item="$2"
    shift 2

    rm -f "$JOB_DIR/$slot.err" "$JOB_DIR/$slot.out"
    (cd "$ROOT" && exec "$@") >"$JOB_DIR/$slot.out" 2>"$JOB_DIR/$slot.err" &
    JOB_PID[$slot]=$!
    JOB_ITEM[$slot]="$item"
    LIVE_CUNT=$((LIVE_CUNT + 1))
}

sys_proc_alive_is()
{
    local stat state

    if [[ ! -r "/proc/$1/stat" ]]; then return 1; fi

    IFS= read -r stat <"/proc/$1/stat"

    if [[ ! "$stat" =~ \)\ ([A-Za-z]) ]]; then return 1; fi

    state="${BASH_REMATCH[1]}"

    [[ "$state" != "Z" && "$state" != "X" ]]
}

build_job_reap()
{
    local slot="$1"
    local pid="${JOB_PID[$slot]}"
    local code text src

    if sys_proc_alive_is "$pid"; then return 0; fi

    wait "$pid"
    code=$?

    JOB_PID[$slot]=""
    LIVE_CUNT=$((LIVE_CUNT - 1))

    if [[ "$code" != 0 ]]; then
        text=""
        if [[ -f "$JOB_DIR/$slot.err" ]]; then IFS= read -r -d '' -n "$ERR_CAP" text <"$JOB_DIR/$slot.err"; fi

        if [[ "${JOB_ITEM[$slot]}" == "link" ]]; then
            src="link"
        else
            compile_list_map "${ITEM_VAL[${JOB_ITEM[$slot]}]}"
            src="$MAPPED"
        fi

        build_fail_set "$src" "$text"
        return 0
    fi

    if [[ "${JOB_ITEM[$slot]}" != "link" ]]; then SRC_DONE=$((SRC_DONE + 1)); fi
}

compile_start()
{
    if [[ "$LIST_OK" != 1 || "$CC_OK" != 1 ]]; then return "$ERR_NOT_INIT"; fi

    if compile_busy_is; then return "$ERR_BUSY"; fi

    mkdir -p "$BUILD_DIR" "$OBJ_DIR" "$JOB_DIR" || return 1
    build_cflags_wr || return 1

    FAIL_SRC=""
    FAIL_TEXT=""
    ITEM_NEXT=0
    SRC_DONE=0
    LIVE_CUNT=0
    JOB_PID=()
    JOB_ITEM=()
    BUILD_START="$(time_epoch_get)"
    BUILD_END="$BUILD_START"
    PHASE=compile

    return 0
}

compile_pump()
{
    local slot src

    if ! compile_busy_is; then return 0; fi

    for ((slot = 0; slot < JOB_CUNT; slot++)); do
        if [[ -n "${JOB_PID[$slot]:-}" ]]; then build_job_reap "$slot"; fi
    done

    if [[ "$PHASE" == "compile" ]]; then
        for ((slot = 0; slot < JOB_CUNT; slot++)); do
            if build_failed_is; then break; fi

            if [[ -n "${JOB_PID[$slot]:-}" ]]; then continue; fi

            while ((ITEM_NEXT < ITEM_CUNT)) && ! build_item_compiled_is "$ITEM_NEXT"; do ITEM_NEXT=$((ITEM_NEXT + 1)); done

            if ((ITEM_NEXT >= ITEM_CUNT)); then break; fi

            compile_list_map "${ITEM_VAL[$ITEM_NEXT]}"
            src="$MAPPED"
            build_obj_path_get "$src"
            build_job_spawn "$slot" "$ITEM_NEXT" "$CC" "@$CFLAGS_RSP" -c "$src" -o "$OBJ"
            ITEM_NEXT=$((ITEM_NEXT + 1))
        done

        if ((LIVE_CUNT != 0)); then return 0; fi

        if ! build_failed_is && ((ITEM_NEXT < ITEM_CUNT)); then return 0; fi

        if build_failed_is; then
            BUILD_END="$(time_epoch_get)"
            PHASE=fail
            return 0
        fi

        if ! build_link_wr; then
            build_fail_set "link" "linker spawn failed"
            BUILD_END="$(time_epoch_get)"
            PHASE=fail
            return 0
        fi

        build_job_spawn 0 "link" "$CC" "@$LINK_RSP"
        PHASE=link
        return 0
    fi

    if ((LIVE_CUNT != 0)); then return 0; fi

    BUILD_END="$(time_epoch_get)"

    if build_failed_is; then
        PHASE=fail
        return 0
    fi

    PHASE=done
    SRC_NEWER=0
    printf '%s\n%s\n' "$RELEASE" "$((BUILD_END - BUILD_START))" >"$STAMP_PATH"
}

core_deps_ck()
{
    if [[ -e "$SYS_LIB/libdangling.so" && -e "$SYS_LIB/libresteam.so" ]]; then DEPS_OK=1; else DEPS_OK=0; fi
}

core_view_fill()
{
    local stale=0

    if compile_stale_is; then stale=1; fi

    if [[ "$LIST_OK" != 1 ]]; then
        VIEW_LAUNCH="no build list in this install"
    elif [[ -z "$OUT_PATH" || ! -e "$OUT_PATH" ]]; then
        VIEW_LAUNCH="$RELEASE; will compile first"
    elif [[ "$stale" == 1 ]]; then
        VIEW_LAUNCH="$RELEASE; source newer than binaries"
    else
        VIEW_LAUNCH="$RELEASE; built"
    fi

    BUILD_ALERT=0
    if [[ "$CC_OK" == 1 && "$stale" == 1 ]]; then BUILD_ALERT=1; fi

    if [[ "$CC_OK" != 1 ]]; then
        VIEW_BUILD="compiler not found"
    elif [[ "$stale" == 1 ]]; then
        VIEW_BUILD="The available source is newer than your latest compiled binary"
    else
        VIEW_BUILD="up to date"
    fi

    if [[ "$DEPS_OK" == 1 ]]; then VIEW_DEPS="LDG and LibreSteam installed"; else VIEW_DEPS="LDG or LibreSteam missing"; fi
}

ui_text_put()
{
    local col="$1" row="$2" text="$3" tone="$4"

    if ((row >= ROW_CUNT || col >= COL_CUNT)); then return 0; fi

    text="${text:0:$((COL_CUNT - col))}"

    if [[ -z "$text" ]]; then return 0; fi

    FRAME_BUFF+="$ESC[$((row + 1));$((col + 1))H${TONE_SEQ[$tone]}$text"
}

ui_row_fill()
{
    local pad
    printf -v pad '%*s' "$3" ''
    ui_text_put "$1" "$2" "$pad" "$4"
}

ui_frame_draw()
{
    local col="$1" row="$2" col_cunt="$3" row_cunt="$4"
    local rhs base at line

    if ((col_cunt < 2 || row_cunt < 2)); then return 0; fi

    rhs=$((col + col_cunt - 1))
    base=$((row + row_cunt - 1))

    printf -v line '%*s' $((col_cunt - 2)) ''
    line="+${line// /=}+"

    ui_text_put "$col" "$row" "$line" "$TONE_EDGE"
    ui_text_put "$col" "$base" "$line" "$TONE_EDGE"

    for ((at = row + 1; at < base; at++)); do
        ui_text_put "$col" "$at" "|" "$TONE_EDGE"
        ui_text_put "$rhs" "$at" "|" "$TONE_EDGE"
    done
}

ui_vline_draw()
{
    local at

    for ((at = 0; at < $3; at++)); do ui_text_put "$1" $(($2 + at)) "|" "$TONE_EDGE"; done
}

ui_date_get()
{
    printf -v DATE '%(%A, %b %-d, %Y %-I:%M %p UTC)T' -1
}

ui_gauge_draw()
{
    local col="$1" row="$2" col_cunt="$3" label="$4" val="$5" max="$6" tick_cunt="$7" unit="$8"
    local axis_col=$((col + 10))
    local axis_w at tick fill line

    if ((col_cunt < 40 || tick_cunt == 0 || max == 0)); then return 0; fi

    axis_w=$((col_cunt - 32))

    for ((tick = 0; tick <= tick_cunt; tick++)); do
        at=$((axis_col + tick * (axis_w - 1) / tick_cunt))
        ui_text_put "$at" "$row" "$((max * tick / tick_cunt))" "$TONE_DIM"
    done

    ui_text_put "$col" $((row + 1)) "${label:0:8} :" "$TONE_NORMAL"

    printf -v line '%*s' "$axis_w" ''
    ui_text_put "$axis_col" $((row + 1)) "${line// /-}" "$TONE_EDGE"

    for ((tick = 0; tick <= tick_cunt; tick++)); do
        at=$((axis_col + tick * (axis_w - 1) / tick_cunt))
        ui_text_put "$at" $((row + 1)) "+" "$TONE_EDGE"
    done

    if ((val >= max)); then fill="$axis_w"; else fill=$((val * axis_w / max)); fi

    if ((fill > 0)); then
        printf -v line '%*s' "$fill" ''
        ui_text_put "$axis_col" $((row + 2)) "${line// /$BAR}" "$TONE_FILL"
    fi

    ui_text_put $((axis_col + axis_w + 2)) $((row + 2)) "$val ${unit:0:15}" "$TONE_NORMAL"
}

menu_item_draw()
{
    ui_text_put "$1" "$2" "$3" "$TONE_INVERSE"
    ui_text_put $(($1 + 2)) "$2" "$4" "$TONE_BRIGHT"
    ui_text_put $(($1 + 4)) $(($2 + 1)) "$5" "$6"
}

ui_menu_draw()
{
    local frame_col_cunt frame_row_cunt inner_col inner_w mid base line build_tone

    if ((COL_CUNT < 40 || ROW_CUNT < 14)); then
        ui_text_put 0 0 "$SMALL_TEXT" "$TONE_BRIGHT"
        return 0
    fi

    frame_col_cunt=$((COL_CUNT - 2))
    frame_row_cunt=$((ROW_CUNT - 2))
    inner_col=2
    inner_w=$((frame_col_cunt - 2))
    mid=$((1 + frame_col_cunt / 2))
    base=$((frame_row_cunt - 1))

    ui_frame_draw 1 0 "$frame_col_cunt" "$frame_row_cunt"
    ui_row_fill "$inner_col" 1 "$inner_w" "$TONE_INVERSE"

    ui_date_get
    printf -v line '%*s' $((inner_w - 1)) "$DATE"
    if [[ -z "${line:inner_w-1:1}" ]]; then ui_text_put "$inner_col" 1 "$line" "$TONE_INVERSE"; fi

    ui_text_put "$inner_col" 1 "$TITLE_MENU" "$TONE_INVERSE"

    printf -v line '%*s' "$inner_w" ''
    ui_text_put "$inner_col" 2 "${line// /=}" "$TONE_EDGE"
    ui_vline_draw "$mid" 3 $((base - 3))

    build_tone="$TONE_DIM"
    if [[ "$BUILD_ALERT" == 1 ]]; then build_tone="$TONE_ALERT"; fi

    menu_item_draw $((inner_col + 3)) 4 "L" "- Launch Crosstalk" "$VIEW_LAUNCH" "$TONE_DIM"
    menu_item_draw $((inner_col + 3)) 7 "B" "- Build Crosstalk" "$VIEW_BUILD" "$build_tone"
    menu_item_draw $((mid + 3)) 4 "O" "- Obtain dependencies" "$VIEW_DEPS" "$TONE_DIM"

    ui_text_put "$inner_col" $((base + 1)) "$NOTICE" "$TONE_NORMAL"
    ui_text_put $((1 + frame_col_cunt - MENU_EXIT_W)) $((base + 1)) "$MENU_EXIT" "$TONE_NORMAL"
}

summary_box_line()
{
    local box_col="$1" row="$2" col_cunt="$3" text="$4" tone="$5"
    local width=$((col_cunt - 4))

    if ((width > 255)); then width=255; fi

    text="${text%%$'\n'*}"
    text="${text:0:$width}"
    text="${text//[^ -~]/ }"

    ui_text_put $((box_col + 2)) "$row" "$text" "$tone"
}

summary_verdict_draw()
{
    local box_col="$1" col_cunt="$2"
    local row=4 last=10 rest line

    case "$PHASE" in
        compile)
            summary_box_line "$box_col" "$row" "$col_cunt" "Compiling $SRC_DONE of $SRC_TOTAL files." "$TONE_NORMAL"
            return 0
            ;;
        link)
            summary_box_line "$box_col" "$row" "$col_cunt" "Linking." "$TONE_NORMAL"
            return 0
            ;;
        done)
            summary_box_line "$box_col" "$row" "$col_cunt" "Build succeeded." "$TONE_BRIGHT"
            summary_box_line "$box_col" $((row + 1)) "$col_cunt" "Crosstalk $RELEASE is ready." "$TONE_NORMAL"
            summary_box_line "$box_col" $((row + 3)) "$col_cunt" "Press F2 to launch." "$TONE_NORMAL"
            return 0
            ;;
        fail) ;;
        *)
            summary_box_line "$box_col" "$row" "$col_cunt" "No build has run yet. Press F5." "$TONE_NORMAL"
            return 0
            ;;
    esac

    summary_box_line "$box_col" "$row" "$col_cunt" "Build failed:" "$TONE_BRIGHT"
    summary_box_line "$box_col" $((row + 1)) "$col_cunt" "$FAIL_SRC" "$TONE_NORMAL"

    row=$((row + 2))
    rest="$FAIL_TEXT"

    while ((row <= last)) && [[ -n "$rest" ]]; do
        line="${rest%%$'\n'*}"
        summary_box_line "$box_col" "$row" "$col_cunt" "$line" "$TONE_DIM"

        if [[ "$rest" =~ $NEXT_LINE_RE ]]; then rest="${BASH_REMATCH[1]}"; else rest=""; fi

        row=$((row + 1))
    done
}

ui_summary_draw()
{
    local box_col=36 box_w end elapsed scale cc_text

    if [[ "$PHASE" == "fail" ]]; then box_col=1; fi

    if ((COL_CUNT < 72 || ROW_CUNT < 22)); then
        ui_text_put 0 0 "$SMALL_TEXT" "$TONE_BRIGHT"
        return 0
    fi

    ui_date_get
    printf -v cc_text '%*s' $((COL_CUNT - 1)) "$DATE"
    ui_text_put 0 0 "$cc_text" "$TONE_NORMAL"
    ui_text_put 1 0 "$TITLE_SUMMARY" "$TONE_BRIGHT"
    ui_text_put 1 1 "$UNAME" "$TONE_DIM"
    ui_text_put 1 2 "Crosstalk $RELEASE" "$TONE_NORMAL"

    if ((box_col == 36)); then
        cc_text="missing"
        if [[ "$CC_OK" == 1 ]]; then cc_text="DANGCSA clang"; fi

        ui_text_put 1 3 "Release: ${RELEASE:0:31}" "$TONE_NORMAL"
        ui_text_put 1 4 "Compiler: $cc_text" "$TONE_NORMAL"
        ui_text_put 1 5 "Jobs: $JOB_CUNT" "$TONE_NORMAL"
        ui_text_put 1 6 "Files: $SRC_TOTAL" "$TONE_NORMAL"
    fi

    box_w=$((COL_CUNT - box_col - 1))
    ui_frame_draw "$box_col" 3 "$box_w" 9
    summary_verdict_draw "$box_col" "$box_w"

    if compile_busy_is; then end="$(time_epoch_get)"; else end="$BUILD_END"; fi

    elapsed=0
    if ((end > BUILD_START)); then elapsed=$(((end - BUILD_START) / 1000)); fi

    scale=$((PREV_DURATION / 1000 * 2))
    if ((scale < 60)); then scale=60; fi

    while ((elapsed > scale)); do scale=$((scale * 2)); done

    ui_gauge_draw 1 13 $((COL_CUNT - 2)) "Files" "$SRC_DONE" "$SRC_TOTAL" 6 "done"
    ui_gauge_draw 1 17 $((COL_CUNT - 2)) "Time" "$elapsed" "$scale" 6 "s"
    ui_text_put 1 $((ROW_CUNT - 1)) "$SUMMARY_KEYS" "$TONE_NORMAL"
}

core_frame_draw()
{
    local size row_cunt col_cunt

    size="$(stty size <"$TTY_PATH")"
    read -r row_cunt col_cunt <<<"$size"

    if [[ "$row_cunt" =~ ^[0-9]+$ && "$col_cunt" =~ ^[0-9]+$ && "$row_cunt" != 0 && "$col_cunt" != 0 ]]; then
        ROW_CUNT=$((row_cunt > ROW_MAX ? ROW_MAX : row_cunt))
        COL_CUNT=$((col_cunt > COL_MAX ? COL_MAX : col_cunt))
    fi

    core_view_fill

    FRAME_BUFF="$ESC[?2026h$ESC[H$ESC[0;${BG}m$ESC[2J"

    if [[ "$SCREEN" == "summary" ]]; then ui_summary_draw; else ui_menu_draw; fi

    FRAME_BUFF+="$ESC[0m$ESC[?2026l"
    printf '%s' "$FRAME_BUFF"
}

core_build_kick()
{
    local ret

    compile_start
    ret=$?

    if ((ret != 0)); then
        NOTICE="build could not start; err: $ret"
        logging_line_wr ERR "dangem: build start failed; err: $ret"
        return 0
    fi

    LAUNCH_AFTER="$1"
    SCREEN=summary
}

core_launch_req()
{
    if [[ "$LIST_OK" != 1 ]]; then return 0; fi

    if compile_stale_is; then
        core_build_kick 1
        return 0
    fi

    LAUNCH=1
}

core_launch_menu_req()
{
    local now

    if [[ "$LIST_OK" != 1 ]]; then return 0; fi

    if [[ -z "$OUT_PATH" || ! -e "$OUT_PATH" ]]; then
        core_build_kick 1
        return 0
    fi

    now="$(time_epoch_get)"

    if compile_stale_is && [[ "$LAUNCH_CONFIRM" != 1 || $((now - LAUNCH_CONFIRM_TIME)) -gt "$LAUNCH_ARM_SPAN" ]]; then
        LAUNCH_CONFIRM=1
        LAUNCH_CONFIRM_TIME="$now"
        NOTICE="$LAUNCH_CONFIRM_TEXT"
        return 0
    fi

    LAUNCH_CONFIRM=0
    LAUNCH=1
}

gh_release_ld()
{
    local repo="$1" sfx="$2" json

    ASSET_TAG=""
    ASSET_URL=""

    json="$(curl -fsL --retry 3 "$GH_API/$repo/releases/latest")" || return 1

    if [[ ! "$json" =~ \"tag_name\"[[:space:]]*:[[:space:]]*\"([^\"]+)\" ]]; then return 1; fi

    ASSET_TAG="${BASH_REMATCH[1]}"

    if [[ -z "$sfx" ]]; then return 0; fi

    if [[ ! "$json" =~ \"browser_download_url\"[[:space:]]*:[[:space:]]*\"([^\"]*"$sfx")\" ]]; then return 1; fi

    ASSET_URL="${BASH_REMATCH[1]}"
}

core_asset_fetch()
{
    FETCH_NAME="$1"
    NOTICE="obtaining $1"
    core_frame_draw
    curl -fsL --retry 3 -o "$DL_DIR/$1" "$2"
}

core_dep_build()
{
    local name="$1" elevate="$2"
    shift 2
    local src="$DL_DIR/$name"
    local bld="$DL_DIR/$name-build"

    mkdir -p "$src" || return 1
    tar -xzf "$DL_DIR/$name.tar.gz" -C "$src" --strip-components=1 || return 1

    printf 'dangem: compiling %s\n' "$name"
    cmake -S "$src" -B "$bld" -DCMAKE_C_COMPILER="$CC" -DCMAKE_INSTALL_PREFIX="$SYS_PREFIX" -DCMAKE_SHARED_LINKER_FLAGS="$LD_AS_NEEDED" -DCMAKE_EXE_LINKER_FLAGS="$LD_AS_NEEDED" "$@" || return 1
    cmake --build "$bld" -j "$JOB_CUNT" || return 1

    printf 'dangem: installing %s into %s\n' "$name" "$SYS_PREFIX"
    $elevate sh -c 'cmake --install "$1" && ldconfig' dangem "$bld"
}

core_deps_build()
{
    local elevate="" ret

    if [[ "$EUID" != 0 ]]; then
        if sys_tool_has doas; then
            elevate=doas
        elif sys_tool_has sudo; then
            elevate=sudo
        else
            return 1
        fi
    fi

    run_term_leave
    core_dep_build ldg "$elevate" -DLDG_WITH_GPU=OFF && core_dep_build libresteam "$elevate" && cmake -S "$TREE_DIR" -B "$BUILD_DIR" -DCMAKE_C_COMPILER="$CC" -DCROSSTALK_LIBRESTEAM_PREFIX="$SYS_PREFIX" -DCMAKE_EXE_LINKER_FLAGS="$LD_AS_NEEDED"
    ret=$?
    run_term_enter

    return "$ret"
}

core_deps_obtain()
{
    local tool mod cc_url ldg_tag lrs_tag

    for tool in curl tar xz gzip unzip cmake make nasm pkg-config; do
        if ! sys_tool_has "$tool"; then
            NOTICE="$tool is missing from this system"
            return 0
        fi
    done

    for mod in libcurl libpipewire-0.3 alsa libpulse x11; do
        if ! pkg-config --exists "$mod"; then
            NOTICE="$mod development files are missing from this system"
            return 0
        fi
    done

    rm -rf "$DL_DIR"
    mkdir -p "$DL_DIR" "$CC_DIR" || return 0

    NOTICE="looking up the latest releases"
    core_frame_draw

    if [[ ! -d "$SRC_DIR" ]]; then
        if ! gh_release_ld "$GH_REPO" "" || ! core_asset_fetch crosstalk.tar.gz "$GH_DL/$GH_REPO/archive/refs/tags/$ASSET_TAG.tar.gz" || ! tar -xzf "$DL_DIR/crosstalk.tar.gz" -C "$ROOT" --strip-components=1; then
            NOTICE="Crosstalk source download failed"
            logging_line_wr ERR "dangem: $NOTICE"
            return 0
        fi
    fi

    if ! gh_release_ld "$CC_REPO" "$ASSET_SFX"; then
        NOTICE="no compiler release found on GitHub"
        return 0
    fi

    cc_url="$ASSET_URL"

    if ! gh_release_ld "$LDG_REPO" ""; then
        NOTICE="no LDG release found on GitHub"
        return 0
    fi

    ldg_tag="$ASSET_TAG"

    if ! gh_release_ld "$LRS_REPO" ""; then
        NOTICE="no LibreSteam release found on GitHub"
        return 0
    fi

    lrs_tag="$ASSET_TAG"

    if ! core_asset_fetch "$DISCORD_ZIP" "$GH_DL/$GH_REPO/releases/download/$DISCORD_TAG/$DISCORD_ZIP" ||
        ! core_asset_fetch "llvm$ASSET_SFX" "$cc_url" ||
        ! core_asset_fetch ldg.tar.gz "$GH_DL/$LDG_REPO/archive/refs/tags/$ldg_tag.tar.gz" ||
        ! core_asset_fetch libresteam.tar.gz "$GH_DL/$LRS_REPO/archive/refs/tags/$lrs_tag.tar.gz"; then
        NOTICE="download failed; $FETCH_NAME"
        logging_line_wr ERR "dangem: $NOTICE"
        return 0
    fi

    NOTICE="unpacking"
    core_frame_draw

    rm -rf "$CC_DIR/linux" "$BUILD_DIR" "$TREE_DIR/$DISCORD_DIR"

    if ! unzip -q "$DL_DIR/$DISCORD_ZIP" -d "$TREE_DIR/$DISCORD_DIR" || ! tar -xJf "$DL_DIR/llvm$ASSET_SFX" -C "$CC_DIR"; then
        NOTICE="unpacking failed"
        logging_line_wr ERR "dangem: $NOTICE"
        return 0
    fi

    if [[ ! -d "$TREE_DIR/$DISCORD_DIR/discord_social_sdk" ]]; then mv "$TREE_DIR/$DISCORD_DIR"/*/discord_social_sdk "$TREE_DIR/$DISCORD_DIR/"; fi

    cp "$TREE_DIR/$DISCORD_DIR/discord_social_sdk/lib/release/libdiscord_partner_sdk.so" "$TREE_DIR/$DISCORD_DIR/discord_social_sdk/lib/release/"*.kef "$ROOT/"

    if ! build_cc_find; then
        NOTICE="compiler not found after unpacking; path: $CC"
        logging_line_wr ERR "dangem: $NOTICE"
        return 0
    fi

    if ! core_deps_build; then
        NOTICE="LDG or LibreSteam build failed; prefix: $SYS_PREFIX"
        logging_line_wr ERR "dangem: $NOTICE"
        return 0
    fi

    rm -rf "$DL_DIR"

    compile_init
    core_deps_ck

    if [[ "$DEPS_OK" == 1 ]]; then NOTICE="dependencies are in place"; else NOTICE="LDG or LibreSteam is missing from $SYS_LIB"; fi
}

core_deps_menu_req()
{
    local now

    core_deps_ck
    now="$(time_epoch_get)"

    if [[ "$DEPS_OK" == 1 ]] && [[ "$DEPS_CONFIRM" != 1 || $((now - DEPS_CONFIRM_TIME)) -gt "$LAUNCH_ARM_SPAN" ]]; then
        DEPS_CONFIRM=1
        DEPS_CONFIRM_TIME="$now"
        NOTICE="$DEPS_CONFIRM_TEXT"
        return 0
    fi

    DEPS_CONFIRM=0
    core_deps_obtain
}

core_key_route()
{
    local key="$1"

    if [[ "$key" == "F10" ]] || [[ "$SCREEN" == "menu" && ("$key" == "q" || "$key" == "ESC") ]]; then
        if ! compile_busy_is; then QUIT=1; fi
        return 0
    fi

    if [[ "$SCREEN" == "summary" ]]; then
        if compile_busy_is; then return 0; fi

        case "$key" in
            ESC) SCREEN=menu ;;
            F2) core_launch_req ;;
            F5) core_build_kick 0 ;;
        esac

        return 0
    fi

    case "$key" in
        l) core_launch_menu_req ;;
        b) core_build_kick 0 ;;
        o) core_deps_menu_req ;;
    esac
}

core_key_rd()
{
    local ch rest="" taken=0

    KEY=""

    if ! read -t 0 -u 3; then
        sleep "$FRAME_WAIT"
        return 0
    fi

    IFS= read -rsn1 -u 3 ch

    if [[ "$ch" != "$ESC" ]]; then
        KEY="${ch,,}"
        return 0
    fi

    sleep "$ESC_WAIT"

    while ((taken < ESC_SEQ_MAX)) && read -t 0 -u 3; do
        IFS= read -rsn1 -u 3 ch
        rest+="$ch"
        taken=$((taken + 1))
    done

    case "$rest" in
        "") KEY=ESC ;;
        OQ | "[12~" | "[[B") KEY=F2 ;;
        "[15~" | "[[E") KEY=F5 ;;
        "[21~") KEY=F10 ;;
    esac
}

run_term_enter()
{
    exec 3<"$TTY_PATH"
    TTY_SAVED="$(stty -g <"$TTY_PATH")"
    stty -echo -icanon min 0 time 0 <"$TTY_PATH"
    printf '%s' "$ESC]0;dangem$ESC\\$ESC[?1049h$ESC[?25l"
}

run_term_leave()
{
    printf '%s' "$ESC[0m$ESC[?25h$ESC[?1049l"

    if [[ -n "$TTY_SAVED" ]]; then stty "$TTY_SAVED" <"$TTY_PATH"; fi

    TTY_SAVED=""
}

main()
{
    TTY_PATH=/dev/tty

    if [[ -n "${1:-}" ]]; then
        printf 'dangem: unknown argument; arg: %s\n' "$1"
        exit 1
    fi

    run_tty_ensure "$@"
    mkdir -p "$ROOT" || exit 1

    compile_init
    core_deps_ck

    run_term_enter
    trap run_term_leave EXIT

    while [[ "$QUIT" != 1 && "$LAUNCH" != 1 ]]; do
        core_frame_draw
        core_key_rd

        if [[ -n "$KEY" ]]; then core_key_route "$KEY"; fi

        compile_pump

        if [[ "$LAUNCH_AFTER" == 1 && "$PHASE" == "done" ]]; then
            LAUNCH_AFTER=0
            LAUNCH=1
        fi
    done

    if [[ "$LAUNCH" != 1 ]]; then exit 0; fi

    run_term_leave
    trap - EXIT
    logging_line_wr INFO "dangem: launching; path: $OUT_PATH"
    unset TZ DANGEM_TERM_SPAWNED
    cd "$ROOT" || exit 1
    exec "$OUT_PATH" <"$TTY_PATH"
}

main "$@"
