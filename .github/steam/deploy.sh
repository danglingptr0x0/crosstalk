#!/usr/bin/env bash
set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

APPID=4898790
DEPOT_WIN=4898792
DEPOT_LINUX=4898791
BLOAT_APPID=5466650
BLOAT_DEPOT_WIN=5466651
BLOAT_DEPOT_LINUX=5466650
LIVE_BRANCH=public
APPROVER_ID=76561199839560378

CLAN_GID=103582791475732609
EVENTS_URL="https://steamcommunity.com/gid/$CLAN_GID/ajaxcreateupdatedeletepartnerevents/"
AUTH_API="https://api.steampowered.com/IAuthenticationService"
PARTNER_API="https://partner.steam-api.com"
STEAMCMD_URL="https://steamcdn-a.akamaihd.net/client/installer/steamcmd_linux.tar.gz"
UA="Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/151.0.0.0 Safari/537.36"
DEVICE_NAME="crosstalk deploy"
PLATFORM_MOBILE=3
LOGIN_TIMEOUT_SECS=300
EVENT_SPAN_SECS=3600
EVENT_PATCHNOTES=12
EVENT_UPDATE=13
TAGS_PATCHNOTES='["patchnotes","hide_library_detail"]'
TAGS_UPDATE='[]'
SESSIONID_BYTES=12

JQ_JWT_SUB='def jwt_sub: split(".")[1] | gsub("-"; "+") | gsub("_"; "/") | (. + ("=" * ((4 - (length % 4)) % 4))) | @base64d | fromjson | .sub;'

: "${REL:?}" "${CONTENT:?}" "${WORK:?}" "${CHANGELOG:?}" "${STEAM_USER:?}" "${STEAM_PASS:?}" "${STEAM_PUBLISHER_KEY:?}"

STEAMCMD="$WORK/steamcmd/steamcmd.sh"
OUT="$WORK/output"
LOG="$OUT/steamcmd.log"
REFRESH=""
BUILD_MAIN=""
BUILD_BLOAT=""

reply_get()
{
    printf '%s' "$1" | jq -r -s -R --arg f "$2" 'fromjson? | .response[$f] // empty'
}

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

steam_login()
{
    local reply client_id request_id challenge interval deadline next
    reply="$(curl -s -A "$UA" "$AUTH_API/BeginAuthSessionViaQR/v1/" --data-urlencode "device_friendly_name=$DEVICE_NAME" --data-urlencode "platform_type=$PLATFORM_MOBILE")"
    client_id="$(reply_get "$reply" client_id)"
    request_id="$(reply_get "$reply" request_id)"
    challenge="$(reply_get "$reply" challenge_url)"
    interval="$(reply_get "$reply" interval)"
    if [[ -z "$client_id" || -z "$request_id" || -z "$challenge" ]]; then
        echo ":: error: BeginAuthSessionViaQR failed; reply: $reply"
        exit 1
    fi
    echo ":: scan with the Steam mobile app and approve; challenge: $challenge"
    qrencode -t ANSIUTF8 "$challenge"
    deadline=$(( $(date +%s) + LOGIN_TIMEOUT_SECS ))
    while (( $(date +%s) < deadline )); do
        sleep "${interval:-5}"
        reply="$(curl -s -A "$UA" "$AUTH_API/PollAuthSessionStatus/v1/" --data-urlencode "client_id=$client_id" --data-urlencode "request_id=$request_id")"
        next="$(reply_get "$reply" new_client_id)"
        if [[ -n "$next" ]]; then
            client_id="$next"
        fi
        next="$(reply_get "$reply" new_challenge_url)"
        if [[ -n "$next" ]]; then
            echo ":: the QR code changed; challenge: $next"
            qrencode -t ANSIUTF8 "$next"
        fi
        REFRESH="$(reply_get "$reply" refresh_token)"
        if [[ -n "$REFRESH" ]]; then
            echo "::add-mask::$REFRESH"
            echo ":: steam login approved; account: $(reply_get "$reply" account_name)"
            return 0
        fi
    done
    echo ":: error: no approval within $LOGIN_TIMEOUT_SECS s"
    exit 1
}

steam_cookie_emit()
{
    local sid="$1"
    printf '%s' "$REFRESH" | jq -R -j "$JQ_JWT_SUB"' "refresh_token=\(@uri)&steamid=\(jwt_sub)"' | curl -s -A "$UA" --data-binary @- "$AUTH_API/GenerateAccessTokenForApp/v1/" | jq -r --arg sid "$sid" "$JQ_JWT_SUB"' .response.access_token // error("no access token") | "steamcommunity.com\tFALSE\t/\tTRUE\t0\tsteamLoginSecure\t\(jwt_sub)%7C%7C\(.)\nsteamcommunity.com\tFALSE\t/\tTRUE\t0\tsessionid\t\($sid)"'
}

steam_reply_ck()
{
    if [[ "$(printf '%s' "$1" | jq -r -s -R 'fromjson? | .success // empty')" != "1" ]]; then
        echo ":: error: steam $2 failed; reply: $1"
        exit 1
    fi
}

steam_post()
{
    local sid start reply gid ann_gid event_type event_tags
    event_type="$EVENT_UPDATE"
    event_tags="$TAGS_UPDATE"
    if [[ "$REL" == *-rc* ]]; then
        event_type="$EVENT_PATCHNOTES"
        event_tags="$TAGS_PATCHNOTES"
    fi
    sid="$(od -A n -N "$SESSIONID_BYTES" -t x1 /dev/urandom | tr -d ' \n')"
    start="$(date +%s)"
    reply="$(steam_cookie_emit "$sid" | curl -s -A "$UA" -b - "$EVENTS_URL" --data-urlencode "sessionid=$sid" --data-urlencode "bCreate=1" --data-urlencode "rtime32_visibility_start=0" --data-urlencode "rtime32_visibility_end=undefined" --data-urlencode "start_time_is_now=1" --data-urlencode "hidden=true" --data-urlencode "published=false" --data-urlencode "appid=$APPID" --data-urlencode "event_name=$REL" --data-urlencode "event_type=$event_type" --data-urlencode "body@$CHANGELOG" --data-urlencode "rtime32_start_time=$start" --data-urlencode "rtime32_end_time=$(( start + EVENT_SPAN_SECS ))" --data-urlencode "jsondata@$DIR/event_jsondata.json" --data-urlencode "tags=$event_tags" --data-urlencode "build_id=undefined" --data-urlencode "build_branch=undefined" --data-urlencode "english_headline=$REL" --data-urlencode "english_body@$CHANGELOG")"
    steam_reply_ck "$reply" "event create"
    gid="$(printf '%s' "$reply" | jq -r '.gid')"
    ann_gid="$(printf '%s' "$reply" | jq -r '.announcement_gid')"
    echo ":: steam event created; gid: $gid; announcement gid: $ann_gid"
    reply="$(steam_cookie_emit "$sid" | curl -s -A "$UA" -b - "$EVENTS_URL" --data-urlencode "sessionid=$sid" --data-urlencode "bPublish=1" --data-urlencode "gid=$gid" --data-urlencode "announcement_gid=$ann_gid" --data-urlencode "unlistedmode=0" --data-urlencode "rtime32_visibility_start=0" --data-urlencode "rtime32_visibility_end=undefined" --data-urlencode "start_time_is_now=1")"
    steam_reply_ck "$reply" "event publish"
    echo ":: steam event published: https://store.steampowered.com/news/app/$APPID/view/$ann_gid"
}

steamcmd_install
steam_upload
steam_live_set "$APPID" "$BUILD_MAIN"
steam_live_set "$BLOAT_APPID" "$BUILD_BLOAT"
steam_login
steam_post
