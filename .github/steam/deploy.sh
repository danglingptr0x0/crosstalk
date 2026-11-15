#!/usr/bin/env bash
set -euo pipefail

APPID=4898790
DEPOT_WIN=4898792
DEPOT_LINUX=4898791
BLOAT_APPID=5466650
BLOAT_DEPOT_WIN=5466651
BLOAT_DEPOT_LINUX=5466650
LIVE_BRANCH=public
APPROVER_ID=76561199839560378

PARTNER_API="https://partner.steam-api.com"
STEAMCMD_URL="https://steamcdn-a.akamaihd.net/client/installer/steamcmd_linux.tar.gz"
UA="Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/151.0.0.0 Safari/537.36"

: "${REL:?}" "${CONTENT:?}" "${WORK:?}" "${STEAM_USER:?}" "${STEAM_PASS:?}" "${STEAM_PUBLISHER_KEY:?}"

STEAMCMD="$WORK/steamcmd/steamcmd.sh"
OUT="$WORK/output"
LOG="$OUT/steamcmd.log"
BUILD_MAIN=""
BUILD_BLOAT=""

vdf_write()
{
    local out="$1"
    local appid="$2"
    local depot_win="$3"
    local depot_linux="$4"
    local local_win="$5"
    local local_linux="$6"
    printf '%s\n' \
        '"AppBuild"' \
        '{' \
        "\"AppID\" \"$appid\"" \
        "\"Desc\" \"$REL\"" \
        "\"ContentRoot\" \"$CONTENT\"" \
        "\"BuildOutput\" \"$OUT\"" \
        '"Preview" "0"' \
        '"Depots"' \
        '{' \
        "\"$depot_win\"" \
        '{' \
        '"FileMapping"' \
        '{' \
        "\"LocalPath\" \"$local_win/*\"" \
        '"DepotPath" "."' \
        '"recursive" "1"' \
        '}' \
        '"FileExclusion" "steam_appid.txt"' \
        '"FileExclusion" "*.pdb"' \
        "\"InstallScript\" \"$local_win/installscript_win.vdf\"" \
        '}' \
        "\"$depot_linux\"" \
        '{' \
        '"FileMapping"' \
        '{' \
        "\"LocalPath\" \"$local_linux/*\"" \
        '"DepotPath" "."' \
        '"recursive" "1"' \
        '}' \
        '"FileExclusion" "steam_appid.txt"' \
        '}' \
        '}' \
        '}' | tee "$out"
}

steamcmd_install()
{
    mkdir -p "$WORK/steamcmd" "$OUT"
    curl -fsSL -A "$UA" "$STEAMCMD_URL" | tar -xzf - -C "$WORK/steamcmd"
}

steam_upload()
{
    local line
    vdf_write "$OUT/app_build_$APPID.vdf" "$APPID" "$DEPOT_WIN" "$DEPOT_LINUX" win linux
    vdf_write "$OUT/app_build_$BLOAT_APPID.vdf" "$BLOAT_APPID" "$BLOAT_DEPOT_WIN" "$BLOAT_DEPOT_LINUX" bloat_win bloat_linux
    echo ":: upload: apps $APPID and $BLOAT_APPID; desc: $REL; approve the login in the Steam mobile app"
    "$STEAMCMD" +login "$STEAM_USER" "$STEAM_PASS" +run_app_build "$OUT/app_build_$APPID.vdf" +run_app_build "$OUT/app_build_$BLOAT_APPID.vdf" +quit | tee "$LOG"
    while IFS= read -r line; do
        if [[ "$line" =~ AppID\ ([0-9]+)\ build\ \(BuildID\ ([0-9]+)\) ]]; then
            if [[ "${BASH_REMATCH[1]}" == "$APPID" ]]; then
                BUILD_MAIN="${BASH_REMATCH[2]}"
            elif [[ "${BASH_REMATCH[1]}" == "$BLOAT_APPID" ]]; then
                BUILD_BLOAT="${BASH_REMATCH[2]}"
            fi
        fi
    done < "$LOG"
    if [[ -z "$BUILD_MAIN" || -z "$BUILD_BLOAT" ]]; then
        echo ":: error: build IDs not found; main: ${BUILD_MAIN:-none}; bloat: ${BUILD_BLOAT:-none}"
        exit 1
    fi
    echo ":: uploaded; main build: $BUILD_MAIN; bloat build: $BUILD_BLOAT"
}

steam_live_set()
{
    local appid="$1"
    local buildid="$2"
    local reply
    reply="$(printf '%s' "$STEAM_PUBLISHER_KEY" | curl -s -A "$UA" "$PARTNER_API/ISteamApps/SetAppBuildLive/v2/" --data-urlencode "key@-" --data-urlencode "appid=$appid" --data-urlencode "buildid=$buildid" --data-urlencode "betakey=$LIVE_BRANCH" --data-urlencode "steamid=$APPROVER_ID" --data-urlencode "description=$REL")"
    if [[ "$(printf '%s' "$reply" | jq -r -s -R 'fromjson? | .response.result // empty')" != "1" ]]; then
        echo ":: error: SetAppBuildLive failed; app: $appid; build: $buildid; reply: $reply"
        exit 1
    fi
    echo ":: live on $LIVE_BRANCH: app $appid build $buildid"
}

steamcmd_install
steam_upload
steam_live_set "$APPID" "$BUILD_MAIN"
steam_live_set "$BLOAT_APPID" "$BUILD_BLOAT"
