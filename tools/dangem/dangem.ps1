$ErrorActionPreference = 'Continue'
$ProgressPreference = 'SilentlyContinue'

$script:GH_REPO = 'danglingptr0x0/crosstalk'
$script:SELF_URL = "https://raw.githubusercontent.com/$($script:GH_REPO)/main/tools/dangem/dangem.ps1"
$script:SELF_DST = "$env:LOCALAPPDATA/Programs/dangem/dangem.ps1" -replace '\\', '/'

if ($env:DANGEM_ROOT) { $script:ROOT = $env:DANGEM_ROOT } else { $script:ROOT = "$env:LOCALAPPDATA/crosstalk" }
$script:ROOT = $script:ROOT -replace '\\', '/'
$script:BUILD_DIR = "$($script:ROOT)/build-win"
$script:OBJ_DIR = "$($script:BUILD_DIR)/obj"
$script:JOB_DIR = "$($script:BUILD_DIR)/obj/.jobs"
$script:LIST_PATH = "$($script:BUILD_DIR)/dangem/build-windows.lst"
$script:RELEASE_PATH = "$($script:BUILD_DIR)/dangem/RELEASE"
$script:STAMP_PATH = "$($script:BUILD_DIR)/stamp"
$script:CFLAGS_RSP = "$($script:BUILD_DIR)/cflags.rsp"
$script:LINK_RSP = "$($script:BUILD_DIR)/link.rsp"
$script:LOG_PATH = "$($script:ROOT)/dangem.log"
$script:DL_DIR = "$($script:ROOT)/.dl"
$script:SRC_DIR = "$($script:ROOT)/src"
$script:MINE = 0

$script:UNAME = "Windows $([Environment]::OSVersion.Version) $env:COMPUTERNAME $env:PROCESSOR_ARCHITECTURE"
$script:TITLE_MENU = ' DANGLING Ecosystem Manager r0.1'
$script:TITLE_SUMMARY = 'DANGLING Ecosystem Manager r0.1'
$script:MENU_EXIT = '(F10 = Exit)'
$script:MENU_EXIT_W = 12
$script:SUMMARY_KEYS = 'F2=Launch - F5=Rebuild - Esc=Menu - F10=Quit'
$script:SMALL_TEXT = 'terminal too small'
$script:LAUNCH_CONFIRM_TEXT = 'The latest source is newer than the compiled binaries. Launch anyway (press again)?'
$script:DEPS_CONFIRM_TEXT = 'The dependencies are already in place. Obtain them again (press again)?'
$script:LAUNCH_ARM_SPAN = 3000
$script:FRAME_WAIT = 16
$script:COL_MAX = 512
$script:ROW_MAX = 256
$script:JOB_MAX = 64
$script:ERR_CAP = 16383
$script:ERR_NOT_INIT = 7
$script:ERR_BUSY = 11

$script:ESC = [string][char]27
$script:BAR = [string][char]0x23
$script:BG = '48;2;10;10;20'
$script:TONE_NORMAL = 0
$script:TONE_BRIGHT = 1
$script:TONE_DIM = 2
$script:TONE_INVERSE = 3
$script:TONE_EDGE = 4
$script:TONE_FILL = 5
$script:TONE_ALERT = 6
$script:TONE_SEQ = @(
    "${ESC}[0;38;2;207;210;255;${BG}m",
    "${ESC}[0;38;2;234;240;255;${BG}m",
    "${ESC}[0;38;2;143;176;255;${BG}m",
    "${ESC}[0;38;2;255;255;255;48;2;61;108;255m",
    "${ESC}[0;38;2;191;255;0;${BG}m",
    "${ESC}[0;38;2;61;108;255;${BG}m",
    "${ESC}[0;38;2;200;50;50;${BG}m"
)

$script:PHASE = 'idle'
$script:LIST_OK = 0
$script:CC_OK = 0
$script:CC = ''
$script:RELEASE = ''
$script:OUT_PATH = ''
$script:SRC_NEWER = 0
$script:SRC_TOTAL = 0
$script:SRC_DONE = 0
$script:JOB_CUNT = 1
$script:LIVE_CUNT = 0
$script:ITEM_NEXT = 0
$script:BUILD_START = 0
$script:BUILD_END = 0
$script:PREV_DURATION = 0
$script:FAIL_SRC = ''
$script:FAIL_TEXT = ''
$script:DEPS_OK = 0
$script:KEYV = @{}
$script:ITEM_KIND = New-Object System.Collections.Generic.List[string]
$script:ITEM_VAL = New-Object System.Collections.Generic.List[string]
$script:ITEM_CUNT = 0
$script:FETCH_NAME = ''
$script:JOB_PROC = @{}
$script:JOB_ITEM = @{}

$script:SCREEN = 'menu'
$script:QUIT = 0
$script:LAUNCH = 0
$script:LAUNCH_AFTER = 0
$script:LAUNCH_CONFIRM = 0
$script:LAUNCH_CONFIRM_TIME = 0
$script:DEPS_CONFIRM = 0
$script:DEPS_CONFIRM_TIME = 0
$script:NOTICE = ''
$script:VIEW_LAUNCH = ''
$script:VIEW_BUILD = ''
$script:VIEW_DEPS = ''
$script:BUILD_ALERT = 0
$script:COL_CUNT = 0
$script:ROW_CUNT = 0
$script:FRAME_BUFF = New-Object System.Text.StringBuilder
$script:KEY = ''
$script:CTRL_C_SAVED = $false

function idiv([long]$lhs, [long]$rhs)
{
    return [long][Math]::Floor($lhs / $rhs)
}

function str_cut([string]$text, [long]$len)
{
    if ($len -le 0) { return '' }
    if ($text.Length -gt $len) { return $text.Substring(0, $len) }
    return $text
}

function str_pad([long]$len, [string]$ch)
{
    if ($len -le 0) { return '' }
    return $ch * $len
}

function time_epoch_get
{
    return [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()
}

function logging_line_wr([string]$level, [string]$text)
{
    New-Item -ItemType Directory -Force -Path $script:ROOT | Out-Null
    Add-Content -LiteralPath $script:LOG_PATH -Value "$(time_epoch_get) $level $text"
}

function sys_tool_has([string]$name)
{
    return [bool](Get-Command $name -CommandType Application -ErrorAction SilentlyContinue)
}

function compile_list_ld
{
    $script:KEYV = @{}
    $script:ITEM_KIND.Clear()
    $script:ITEM_VAL.Clear()
    $script:ITEM_CUNT = 0

    if (-not (Test-Path -LiteralPath $script:LIST_PATH -PathType Leaf)) { return $false }

    foreach ($line in [IO.File]::ReadAllLines($script:LIST_PATH))
    {
        $line = $line.TrimEnd("`r")

        if ($line -notmatch '^([^ ]*) (.*)$') { continue }

        $key = $Matches[1]
        $val = $Matches[2]

        if ($val.Length -eq 0) { continue }

        switch -CaseSensitive ($key)
        {
            { $_ -in 'root', 'bin', 'tgt', 'steam', 'ldginc', 'ldglib', 'out', 'std', 'cflagset', 'ldflagset', 'libset' } { $script:KEYV[$key] = $val }
            { $_ -in 'src', 'def', 'inc', 'cflag', 'lib', 'ldflag' }
            {
                if ($script:MINE -eq 1 -and $key -ceq 'src' -and $val.EndsWith('.c')) { continue }

                $script:ITEM_KIND.Add($key)
                $script:ITEM_VAL.Add($val)
                $script:ITEM_CUNT++
            }
        }
    }

    if ($script:MINE -eq 1) { return (compile_list_mine_ld) }

    return $true
}

function compile_list_mine_ld
{
    $cmake_path = "$($script:SRC_DIR)/../CMakeLists.txt"
    $in_set = $false

    if (-not (Test-Path -LiteralPath $cmake_path -PathType Leaf)) { return $false }

    foreach ($line in [IO.File]::ReadAllLines($cmake_path))
    {
        $line = $line.TrimEnd("`r")

        if ($line -match '^set\(CROSSTALK_SRC_ALL')
        {
            $in_set = $true
            continue
        }

        if (-not $in_set) { continue }

        if ($line -match '^\s*\)') { return $true }

        if ($line -match '^\s*(src/\S+\.c)\s*$')
        {
            $script:ITEM_KIND.Add('src')
            $script:ITEM_VAL.Add($Matches[1])
            $script:ITEM_CUNT++
        }
    }

    return $false
}

function path_leaf_get([string]$path)
{
    if ($path -match '([^/]*)$') { return $Matches[1] }
    return $path
}

function compile_list_map([string]$src)
{
    $ldglib = $script:KEYV['ldglib']

    if ($ldglib -and $src -eq $ldglib) { return "$($script:ROOT)/ldg/lib/$(path_leaf_get $src)" }

    if ($src -ceq 'src' -or $src.StartsWith('src/')) { return $script:SRC_DIR + $src.Substring(3) }

    foreach ($key_name in 'bin', 'ldginc', 'steam', 'root')
    {
        $key_val = $script:KEYV[$key_name]

        if (-not $key_val -or -not $src.Contains($key_val)) { continue }

        switch ($key_name)
        {
            'bin' { $repl = $script:BUILD_DIR }
            'ldginc' { $repl = "$($script:ROOT)/ldg/include" }
            'steam' { $repl = "$($script:ROOT)/sdk/steamworks" }
            'root' { $repl = $script:ROOT }
        }

        $at = $src.IndexOf($key_val)
        $mapped = $src.Substring(0, $at) + $repl + $src.Substring($at + $key_val.Length)

        if ($mapped -eq "$($script:ROOT)/src" -or $mapped.StartsWith("$($script:ROOT)/src/")) { $mapped = $script:SRC_DIR + $mapped.Substring("$($script:ROOT)/src".Length) }

        return $mapped
    }

    return $src
}

function build_rsp_arg([System.Text.StringBuilder]$rsp, [string]$pre, [string]$val)
{
    if ($val.Length -eq 0) { return }

    $arg = "$pre$val"
    $arg = $arg.Replace('\', '\\')
    $arg = $arg.Replace('"', '\"')
    [void]$rsp.Append('"').Append($arg).Append('"').Append("`n")
}

function build_rsp_set([System.Text.StringBuilder]$rsp, [string]$key)
{
    $set = $script:KEYV[$key]

    if (-not $set) { return }

    foreach ($tok in ($set -split '\s+'))
    {
        if ($tok.Length -eq 0) { continue }
        build_rsp_arg $rsp '' (compile_list_map $tok)
    }
}

function build_item_compiled_is([int]$idx)
{
    return ($script:ITEM_KIND[$idx] -ceq 'src' -and -not $script:ITEM_VAL[$idx].EndsWith('.rc'))
}

function build_obj_path_get([string]$src)
{
    if ($src.StartsWith($script:SRC_DIR)) { $src = 'src' + $src.Substring($script:SRC_DIR.Length) }

    if ($src.StartsWith($script:ROOT)) { $src = $src.Substring($script:ROOT.Length) }

    while ($src.StartsWith('/') -or $src.StartsWith('\')) { $src = $src.Substring(1) }

    $src = $src.Replace('/', '_').Replace('\', '_').Replace(':', '_')
    return "$($script:OBJ_DIR)/$src.o"
}

function build_cflags_wr
{
    $rsp = New-Object System.Text.StringBuilder

    if ($script:KEYV['std']) { build_rsp_arg $rsp '-std=c' $script:KEYV['std'] }

    if ($script:KEYV['tgt']) { build_rsp_arg $rsp '--target=' $script:KEYV['tgt'] }

    build_rsp_set $rsp 'cflagset'
    build_rsp_arg $rsp '' '-march=native'

    for ($idx = 0; $idx -lt $script:ITEM_CUNT; $idx++)
    {
        $kind = $script:ITEM_KIND[$idx]

        if ($kind -cne 'cflag' -and $kind -cne 'def' -and $kind -cne 'inc') { continue }

        $mapped = compile_list_map $script:ITEM_VAL[$idx]

        if ($kind -ceq 'cflag')
        {
            build_rsp_arg $rsp '' $mapped
        }
        elseif ($kind -ceq 'def')
        {
            build_rsp_arg $rsp '-D' $mapped
        }
        elseif ($mapped.StartsWith($script:SRC_DIR))
        {
            build_rsp_arg $rsp '-I' $mapped
        }
        else
        {
            build_rsp_arg $rsp '' '-isystem'
            build_rsp_arg $rsp '' $mapped
        }
    }

    build_rsp_arg $rsp '' '-isystem'
    build_rsp_arg $rsp '' "$($script:ROOT)/ldg/include"

    [IO.File]::WriteAllText($script:CFLAGS_RSP, $rsp.ToString())
    return $true
}

function build_link_wr
{
    $rsp = New-Object System.Text.StringBuilder

    if ($script:KEYV['tgt']) { build_rsp_arg $rsp '--target=' $script:KEYV['tgt'] }

    build_rsp_arg $rsp '' '-fuse-ld=lld'
    build_rsp_set $rsp 'ldflagset'
    build_rsp_arg $rsp '' '-o'
    build_rsp_arg $rsp '' $script:OUT_PATH

    for ($idx = 0; $idx -lt $script:ITEM_CUNT; $idx++)
    {
        if (-not (build_item_compiled_is $idx)) { continue }

        build_rsp_arg $rsp '' (build_obj_path_get (compile_list_map $script:ITEM_VAL[$idx]))
    }

    for ($idx = 0; $idx -lt $script:ITEM_CUNT; $idx++)
    {
        if ($script:ITEM_KIND[$idx] -cne 'ldflag') { continue }

        build_rsp_arg $rsp '' (compile_list_map $script:ITEM_VAL[$idx])
    }

    build_rsp_arg $rsp '-L' "$($script:ROOT)/ldg/lib"
    build_rsp_arg $rsp '-L' $script:ROOT

    for ($idx = 0; $idx -lt $script:ITEM_CUNT; $idx++)
    {
        if ($script:ITEM_KIND[$idx] -cne 'lib') { continue }

        $val = $script:ITEM_VAL[$idx]

        if ($val.StartsWith('/') -and $val -ne $script:KEYV['ldglib'])
        {
            build_rsp_arg $rsp '-l:' (path_leaf_get $val)
            continue
        }

        $mapped = compile_list_map $val

        if ($val.StartsWith('-') -or $val.Contains('/') -or $val.Contains('\'))
        {
            build_rsp_arg $rsp '' $mapped
        }
        else
        {
            build_rsp_arg $rsp '-l' $mapped
        }
    }

    build_rsp_set $rsp 'libset'

    [IO.File]::WriteAllText($script:LINK_RSP, $rsp.ToString())
    return $true
}

function build_cc_find
{
    $script:CC = "$($script:ROOT)/res/utils/cc/bin/clang.exe"
    if (Test-Path -LiteralPath $script:CC) { return $true }

    $script:CC = "$($script:ROOT)/res/utils/cc/windows/bin/clang.exe"
    if (Test-Path -LiteralPath $script:CC) { return $true }

    return $false
}

function build_src_newer_ck
{
    $script:SRC_NEWER = 0

    if (-not $script:OUT_PATH -or -not (Test-Path -LiteralPath $script:OUT_PATH)) { return }

    $out_time = (Get-Item -LiteralPath $script:OUT_PATH).LastWriteTimeUtc

    foreach ($dir in $script:SRC_DIR, "$($script:ROOT)/ldg", "$($script:BUILD_DIR)/dangem", "$($script:BUILD_DIR)/gen")
    {
        if (-not (Test-Path -LiteralPath $dir -PathType Container)) { continue }

        $hit = Get-ChildItem -LiteralPath $dir -Recurse -File -Depth 16 -ErrorAction SilentlyContinue | Where-Object { $_.FullName -notmatch '[\\/]\.' -and $_.LastWriteTimeUtc -gt $out_time } | Select-Object -First 1

        if ($hit)
        {
            $script:SRC_NEWER = 1
            return
        }
    }
}

function compile_init
{
    $script:PHASE = 'idle'
    $script:SRC_TOTAL = 0
    $script:SRC_DONE = 0
    $script:OUT_PATH = ''
    $script:RELEASE = ''
    $script:PREV_DURATION = 0

    if (compile_list_ld) { $script:LIST_OK = 1 } else { $script:LIST_OK = 0 }

    if (build_cc_find) { $script:CC_OK = 1 } else { $script:CC_OK = 0 }

    for ($idx = 0; $idx -lt $script:ITEM_CUNT; $idx++)
    {
        if (build_item_compiled_is $idx) { $script:SRC_TOTAL++ }
    }

    if ($script:LIST_OK -eq 1 -and $script:KEYV['out']) { $script:OUT_PATH = "$($script:ROOT)/$($script:KEYV['out'])" }

    $cpu = [Environment]::ProcessorCount
    if ($cpu -lt 1) { $cpu = 1 }

    $script:JOB_CUNT = [Math]::Min($cpu, $script:JOB_MAX)

    if (Test-Path -LiteralPath $script:RELEASE_PATH -PathType Leaf)
    {
        $line = ([IO.File]::ReadAllLines($script:RELEASE_PATH) | Select-Object -First 1)
        if ($null -eq $line) { $line = '' }
        $script:RELEASE = str_cut $line.TrimEnd("`r") 63
    }

    if (Test-Path -LiteralPath $script:STAMP_PATH -PathType Leaf)
    {
        $lines = [IO.File]::ReadAllLines($script:STAMP_PATH)
        if ($lines.Count -ge 2 -and $lines[1] -match '^[0-9]+$') { $script:PREV_DURATION = [long]$lines[1] }
    }

    build_src_newer_ck
}

function compile_stale_is
{
    if (-not $script:OUT_PATH -or -not (Test-Path -LiteralPath $script:OUT_PATH)) { return $true }

    if ($script:SRC_NEWER -eq 1) { return $true }

    if (-not (Test-Path -LiteralPath $script:STAMP_PATH -PathType Leaf)) { return $true }

    $line = ([IO.File]::ReadAllLines($script:STAMP_PATH) | Select-Object -First 1)

    return ($line -cne $script:RELEASE)
}

function compile_busy_is
{
    return ($script:PHASE -eq 'compile' -or $script:PHASE -eq 'link')
}

function build_fail_set([string]$src, [string]$text)
{
    if ($script:FAIL_TEXT -or $script:FAIL_SRC) { return }

    $script:FAIL_SRC = $src
    $script:FAIL_TEXT = str_cut $text $script:ERR_CAP

    if (-not $script:FAIL_TEXT) { $script:FAIL_TEXT = ' ' }
}

function build_failed_is
{
    return [bool]$script:FAIL_TEXT
}

function build_job_spawn([int]$slot, [string]$item, [string[]]$argv)
{
    $out = "$($script:JOB_DIR)/$slot.out"
    $err = "$($script:JOB_DIR)/$slot.err"

    Remove-Item -LiteralPath $out, $err -Force -ErrorAction SilentlyContinue

    $quoted = $argv | ForEach-Object { '"' + $_ + '"' }

    try
    {
        $proc = Start-Process -FilePath $script:CC -ArgumentList $quoted -WorkingDirectory $script:ROOT -NoNewWindow -PassThru -RedirectStandardOutput $out -RedirectStandardError $err
    }
    catch
    {
        return $false
    }

    $null = $proc.Handle
    $script:JOB_PROC[$slot] = $proc
    $script:JOB_ITEM[$slot] = $item
    $script:LIVE_CUNT++
    return $true
}

function build_job_reap([int]$slot)
{
    $proc = $script:JOB_PROC[$slot]

    if (-not $proc.HasExited) { return }

    $proc.WaitForExit()
    $code = $proc.ExitCode

    $script:JOB_PROC[$slot] = $null
    $script:LIVE_CUNT--

    if ($code -ne 0)
    {
        $text = ''
        $err = "$($script:JOB_DIR)/$slot.err"
        if (Test-Path -LiteralPath $err -PathType Leaf) { $text = str_cut ([IO.File]::ReadAllText($err)) $script:ERR_CAP }

        if ($script:JOB_ITEM[$slot] -eq 'link')
        {
            $src = 'link'
        }
        else
        {
            $src = compile_list_map $script:ITEM_VAL[[int]$script:JOB_ITEM[$slot]]
        }

        build_fail_set $src $text
        return
    }

    if ($script:JOB_ITEM[$slot] -ne 'link') { $script:SRC_DONE++ }
}

function compile_start
{
    if ($script:LIST_OK -ne 1 -or $script:CC_OK -ne 1) { return $script:ERR_NOT_INIT }

    if (compile_busy_is) { return $script:ERR_BUSY }

    try
    {
        New-Item -ItemType Directory -Force -Path $script:BUILD_DIR, $script:OBJ_DIR, $script:JOB_DIR | Out-Null
    }
    catch
    {
        return 1
    }

    if (-not (build_cflags_wr)) { return 1 }

    $script:FAIL_SRC = ''
    $script:FAIL_TEXT = ''
    $script:ITEM_NEXT = 0
    $script:SRC_DONE = 0
    $script:LIVE_CUNT = 0
    $script:JOB_PROC = @{}
    $script:JOB_ITEM = @{}
    $script:BUILD_START = time_epoch_get
    $script:BUILD_END = $script:BUILD_START
    $script:PHASE = 'compile'

    return 0
}

function compile_pump
{
    if (-not (compile_busy_is)) { return }

    for ($slot = 0; $slot -lt $script:JOB_CUNT; $slot++)
    {
        if ($script:JOB_PROC[$slot]) { build_job_reap $slot }
    }

    if ($script:PHASE -eq 'compile')
    {
        for ($slot = 0; $slot -lt $script:JOB_CUNT; $slot++)
        {
            if (build_failed_is) { break }

            if ($script:JOB_PROC[$slot]) { continue }

            while ($script:ITEM_NEXT -lt $script:ITEM_CUNT -and -not (build_item_compiled_is $script:ITEM_NEXT)) { $script:ITEM_NEXT++ }

            if ($script:ITEM_NEXT -ge $script:ITEM_CUNT) { break }

            $src = compile_list_map $script:ITEM_VAL[$script:ITEM_NEXT]
            $obj = build_obj_path_get $src

            if (-not (build_job_spawn $slot ([string]$script:ITEM_NEXT) @('--rsp-quoting=posix', "@$($script:CFLAGS_RSP)", '-c', $src, '-o', $obj))) { build_fail_set $src 'compiler spawn failed' }

            $script:ITEM_NEXT++
        }

        if ($script:LIVE_CUNT -ne 0) { return }

        if (-not (build_failed_is) -and $script:ITEM_NEXT -lt $script:ITEM_CUNT) { return }

        if (build_failed_is)
        {
            $script:BUILD_END = time_epoch_get
            $script:PHASE = 'fail'
            return
        }

        if (-not (build_link_wr) -or -not (build_job_spawn 0 'link' @('--rsp-quoting=posix', "@$($script:LINK_RSP)")))
        {
            build_fail_set 'link' 'linker spawn failed'
            $script:BUILD_END = time_epoch_get
            $script:PHASE = 'fail'
            return
        }

        $script:PHASE = 'link'
        return
    }

    if ($script:LIVE_CUNT -ne 0) { return }

    $script:BUILD_END = time_epoch_get

    if (build_failed_is)
    {
        $script:PHASE = 'fail'
        return
    }

    $script:PHASE = 'done'
    $script:SRC_NEWER = 0
    [IO.File]::WriteAllText($script:STAMP_PATH, "$($script:RELEASE)`n$($script:BUILD_END - $script:BUILD_START)`n")
}

function core_deps_ck
{
    if (Test-Path -LiteralPath "$($script:ROOT)/ldg/lib") { $script:DEPS_OK = 1 } else { $script:DEPS_OK = 0 }
}

function core_view_fill
{
    $stale = 0

    if (compile_stale_is) { $stale = 1 }

    if ($script:LIST_OK -ne 1)
    {
        $script:VIEW_LAUNCH = 'no build list in this install'
    }
    elseif (-not $script:OUT_PATH -or -not (Test-Path -LiteralPath $script:OUT_PATH))
    {
        $script:VIEW_LAUNCH = "$($script:RELEASE); will compile first"
    }
    elseif ($stale -eq 1)
    {
        $script:VIEW_LAUNCH = "$($script:RELEASE); source newer than binaries"
    }
    else
    {
        $script:VIEW_LAUNCH = "$($script:RELEASE); built"
    }

    $script:BUILD_ALERT = 0
    if ($script:CC_OK -eq 1 -and $stale -eq 1) { $script:BUILD_ALERT = 1 }

    if ($script:CC_OK -ne 1)
    {
        $script:VIEW_BUILD = 'bundled compiler not found'
    }
    elseif ($stale -eq 1)
    {
        $script:VIEW_BUILD = 'The available source is newer than your latest compiled binary'
    }
    else
    {
        $script:VIEW_BUILD = 'up to date'
    }

    if ($script:DEPS_OK -eq 1) { $script:VIEW_DEPS = 'LDG bundled' } else { $script:VIEW_DEPS = 'LDG missing' }
}

function ui_text_put([long]$col, [long]$row, [string]$text, [int]$tone)
{
    if ($row -ge $script:ROW_CUNT -or $col -ge $script:COL_CUNT) { return }

    $text = str_cut $text ($script:COL_CUNT - $col)

    if ($text.Length -eq 0) { return }

    [void]$script:FRAME_BUFF.Append("${ESC}[$($row + 1);$($col + 1)H").Append($script:TONE_SEQ[$tone]).Append($text)
}

function ui_row_fill([long]$col, [long]$row, [long]$len, [int]$tone)
{
    ui_text_put $col $row (str_pad $len ' ') $tone
}

function ui_frame_draw([long]$col, [long]$row, [long]$col_cunt, [long]$row_cunt)
{
    if ($col_cunt -lt 2 -or $row_cunt -lt 2) { return }

    $rhs = $col + $col_cunt - 1
    $base = $row + $row_cunt - 1
    $line = '+' + (str_pad ($col_cunt - 2) '=') + '+'

    ui_text_put $col $row $line $script:TONE_EDGE
    ui_text_put $col $base $line $script:TONE_EDGE

    for ($at = $row + 1; $at -lt $base; $at++)
    {
        ui_text_put $col $at '|' $script:TONE_EDGE
        ui_text_put $rhs $at '|' $script:TONE_EDGE
    }
}

function ui_vline_draw([long]$col, [long]$row, [long]$len)
{
    for ($at = 0; $at -lt $len; $at++) { ui_text_put $col ($row + $at) '|' $script:TONE_EDGE }
}

function ui_date_get
{
    return [DateTime]::UtcNow.ToString('dddd, MMM d, yyyy h:mm tt', [Globalization.CultureInfo]::InvariantCulture) + ' UTC'
}

function ui_gauge_draw([long]$col, [long]$row, [long]$col_cunt, [string]$label, [long]$val, [long]$max, [long]$tick_cunt, [string]$unit)
{
    $axis_col = $col + 10

    if ($col_cunt -lt 40 -or $tick_cunt -eq 0 -or $max -eq 0) { return }

    $axis_w = $col_cunt - 32

    for ($tick = 0; $tick -le $tick_cunt; $tick++)
    {
        $at = $axis_col + (idiv ($tick * ($axis_w - 1)) $tick_cunt)
        ui_text_put $at $row ([string](idiv ($max * $tick) $tick_cunt)) $script:TONE_DIM
    }

    ui_text_put $col ($row + 1) "$(str_cut $label 8) :" $script:TONE_NORMAL
    ui_text_put $axis_col ($row + 1) (str_pad $axis_w '-') $script:TONE_EDGE

    for ($tick = 0; $tick -le $tick_cunt; $tick++)
    {
        $at = $axis_col + (idiv ($tick * ($axis_w - 1)) $tick_cunt)
        ui_text_put $at ($row + 1) '+' $script:TONE_EDGE
    }

    if ($val -ge $max) { $fill = $axis_w } else { $fill = idiv ($val * $axis_w) $max }

    if ($fill -gt 0) { ui_text_put $axis_col ($row + 2) (str_pad $fill $script:BAR) $script:TONE_FILL }

    ui_text_put ($axis_col + $axis_w + 2) ($row + 2) "$val $(str_cut $unit 15)" $script:TONE_NORMAL
}

function menu_item_draw([long]$col, [long]$row, [string]$key, [string]$label, [string]$view, [int]$tone)
{
    ui_text_put $col $row $key $script:TONE_INVERSE
    ui_text_put ($col + 2) $row $label $script:TONE_BRIGHT
    ui_text_put ($col + 4) ($row + 1) $view $tone
}

function ui_menu_draw
{
    if ($script:COL_CUNT -lt 40 -or $script:ROW_CUNT -lt 14)
    {
        ui_text_put 0 0 $script:SMALL_TEXT $script:TONE_BRIGHT
        return
    }

    $frame_col_cunt = $script:COL_CUNT - 2
    $frame_row_cunt = $script:ROW_CUNT - 2
    $inner_col = 2
    $inner_w = $frame_col_cunt - 2
    $mid = 1 + (idiv $frame_col_cunt 2)
    $base = $frame_row_cunt - 1

    ui_frame_draw 1 0 $frame_col_cunt $frame_row_cunt
    ui_row_fill $inner_col 1 $inner_w $script:TONE_INVERSE

    $line = (ui_date_get).PadLeft([Math]::Max(0, $inner_w - 1))
    if ($line.Length -le $inner_w - 1) { ui_text_put $inner_col 1 $line $script:TONE_INVERSE }

    ui_text_put $inner_col 1 $script:TITLE_MENU $script:TONE_INVERSE
    ui_text_put $inner_col 2 (str_pad $inner_w '=') $script:TONE_EDGE
    ui_vline_draw $mid 3 ($base - 3)

    $build_tone = $script:TONE_DIM
    if ($script:BUILD_ALERT -eq 1) { $build_tone = $script:TONE_ALERT }

    menu_item_draw ($inner_col + 3) 4 'L' '- Launch Crosstalk' $script:VIEW_LAUNCH $script:TONE_DIM
    menu_item_draw ($inner_col + 3) 7 'B' '- Build Crosstalk' $script:VIEW_BUILD $build_tone
    menu_item_draw ($mid + 3) 4 'O' '- Obtain dependencies' $script:VIEW_DEPS $script:TONE_DIM

    ui_text_put $inner_col ($base + 1) $script:NOTICE $script:TONE_NORMAL
    ui_text_put (1 + $frame_col_cunt - $script:MENU_EXIT_W) ($base + 1) $script:MENU_EXIT $script:TONE_NORMAL
}

function summary_box_line([long]$box_col, [long]$row, [long]$col_cunt, [string]$text, [int]$tone)
{
    $width = $col_cunt - 4

    if ($width -gt 255) { $width = 255 }

    $text = ($text -split "`n", 2)[0]
    $text = str_cut $text $width
    $text = $text -replace '[^ -~]', ' '

    ui_text_put ($box_col + 2) $row $text $tone
}

function summary_verdict_draw([long]$box_col, [long]$col_cunt)
{
    $row = 4
    $last = 10

    switch ($script:PHASE)
    {
        'compile'
        {
            summary_box_line $box_col $row $col_cunt "Compiling $($script:SRC_DONE) of $($script:SRC_TOTAL) files." $script:TONE_NORMAL
            return
        }
        'link'
        {
            summary_box_line $box_col $row $col_cunt 'Linking.' $script:TONE_NORMAL
            return
        }
        'done'
        {
            summary_box_line $box_col $row $col_cunt 'Build succeeded.' $script:TONE_BRIGHT
            summary_box_line $box_col ($row + 1) $col_cunt "Crosstalk $($script:RELEASE) is ready." $script:TONE_NORMAL
            summary_box_line $box_col ($row + 3) $col_cunt 'Press F2 to launch.' $script:TONE_NORMAL
            return
        }
        'fail' { }
        default
        {
            summary_box_line $box_col $row $col_cunt 'No build has run yet. Press F5.' $script:TONE_NORMAL
            return
        }
    }

    summary_box_line $box_col $row $col_cunt 'Build failed:' $script:TONE_BRIGHT
    summary_box_line $box_col ($row + 1) $col_cunt $script:FAIL_SRC $script:TONE_NORMAL

    $row += 2

    foreach ($line in ($script:FAIL_TEXT -split "`n"))
    {
        if ($row -gt $last) { break }
        summary_box_line $box_col $row $col_cunt $line.TrimEnd("`r") $script:TONE_DIM
        $row++
    }
}

function ui_summary_draw
{
    $box_col = 36

    if ($script:PHASE -eq 'fail') { $box_col = 1 }

    if ($script:COL_CUNT -lt 72 -or $script:ROW_CUNT -lt 22)
    {
        ui_text_put 0 0 $script:SMALL_TEXT $script:TONE_BRIGHT
        return
    }

    ui_text_put 0 0 ((ui_date_get).PadLeft($script:COL_CUNT - 1)) $script:TONE_NORMAL
    ui_text_put 1 0 $script:TITLE_SUMMARY $script:TONE_BRIGHT
    ui_text_put 1 1 $script:UNAME $script:TONE_DIM
    ui_text_put 1 2 "Crosstalk $($script:RELEASE)" $script:TONE_NORMAL

    if ($box_col -eq 36)
    {
        $cc_text = 'missing'
        if ($script:CC_OK -eq 1) { $cc_text = 'bundled clang' }

        ui_text_put 1 3 "Release: $(str_cut $script:RELEASE 31)" $script:TONE_NORMAL
        ui_text_put 1 4 "Compiler: $cc_text" $script:TONE_NORMAL
        ui_text_put 1 5 "Jobs: $($script:JOB_CUNT)" $script:TONE_NORMAL
        ui_text_put 1 6 "Files: $($script:SRC_TOTAL)" $script:TONE_NORMAL
    }

    $box_w = $script:COL_CUNT - $box_col - 1
    ui_frame_draw $box_col 3 $box_w 9
    summary_verdict_draw $box_col $box_w

    if (compile_busy_is) { $end = time_epoch_get } else { $end = $script:BUILD_END }

    $elapsed = 0
    if ($end -gt $script:BUILD_START) { $elapsed = idiv ($end - $script:BUILD_START) 1000 }

    $scale = (idiv $script:PREV_DURATION 1000) * 2
    if ($scale -lt 60) { $scale = 60 }

    while ($elapsed -gt $scale) { $scale *= 2 }

    ui_gauge_draw 1 13 ($script:COL_CUNT - 2) 'Files' $script:SRC_DONE $script:SRC_TOTAL 6 'done'
    ui_gauge_draw 1 17 ($script:COL_CUNT - 2) 'Time' $elapsed $scale 6 's'
    ui_text_put 1 ($script:ROW_CUNT - 1) $script:SUMMARY_KEYS $script:TONE_NORMAL
}

function core_frame_draw
{
    $row_cunt = [Console]::WindowHeight
    $col_cunt = [Console]::WindowWidth

    if ($row_cunt -gt 0 -and $col_cunt -gt 0)
    {
        $script:ROW_CUNT = [Math]::Min($row_cunt, $script:ROW_MAX)
        $script:COL_CUNT = [Math]::Min($col_cunt, $script:COL_MAX)
    }

    core_view_fill

    [void]$script:FRAME_BUFF.Clear()
    [void]$script:FRAME_BUFF.Append("${ESC}[?2026h${ESC}[H${ESC}[0;${BG}m${ESC}[2J")

    if ($script:SCREEN -eq 'summary') { ui_summary_draw } else { ui_menu_draw }

    [void]$script:FRAME_BUFF.Append("${ESC}[0m${ESC}[?2026l")
    [Console]::Out.Write($script:FRAME_BUFF.ToString())
}

function core_build_kick([int]$after)
{
    $ret = compile_start

    if ($ret -ne 0)
    {
        $script:NOTICE = "build could not start; err: $ret"
        logging_line_wr 'ERR' "dangem: build start failed; err: $ret"
        return
    }

    $script:LAUNCH_AFTER = $after
    $script:SCREEN = 'summary'
}

function core_launch_req
{
    if ($script:LIST_OK -ne 1) { return }

    if (compile_stale_is)
    {
        core_build_kick 1
        return
    }

    $script:LAUNCH = 1
}

function core_launch_menu_req
{
    if ($script:LIST_OK -ne 1) { return }

    if (-not $script:OUT_PATH -or -not (Test-Path -LiteralPath $script:OUT_PATH))
    {
        core_build_kick 1
        return
    }

    $now = time_epoch_get

    if ((compile_stale_is) -and ($script:LAUNCH_CONFIRM -ne 1 -or ($now - $script:LAUNCH_CONFIRM_TIME) -gt $script:LAUNCH_ARM_SPAN))
    {
        $script:LAUNCH_CONFIRM = 1
        $script:LAUNCH_CONFIRM_TIME = $now
        $script:NOTICE = $script:LAUNCH_CONFIRM_TEXT
        return
    }

    $script:LAUNCH_CONFIRM = 0
    $script:LAUNCH = 1
}

function core_asset_fetch([string]$name, [string]$url)
{
    $script:FETCH_NAME = $name
    $script:NOTICE = "obtaining $name"
    core_frame_draw

    for ($try = 0; $try -le 3; $try++)
    {
        try
        {
            Invoke-WebRequest -UseBasicParsing -Uri $url -OutFile "$($script:DL_DIR)/$name"
            return $true
        }
        catch
        {
            continue
        }
    }

    return $false
}

function core_deps_obtain
{
    if (-not (sys_tool_has 'tar.exe'))
    {
        $script:NOTICE = 'tar.exe is missing from this system'
        return
    }

    Remove-Item -LiteralPath $script:DL_DIR -Recurse -Force -ErrorAction SilentlyContinue

    try
    {
        New-Item -ItemType Directory -Force -Path $script:DL_DIR | Out-Null
    }
    catch
    {
        return
    }

    $script:NOTICE = 'looking up the latest release'
    core_frame_draw

    try
    {
        $tag = (Invoke-RestMethod -UseBasicParsing -Uri "https://api.github.com/repos/$($script:GH_REPO)/releases/latest").tag_name
    }
    catch
    {
        $tag = ''
    }

    if (-not $tag)
    {
        $script:NOTICE = 'no release found on GitHub'
        return
    }

    $build_asset = "crosstalk-$tag-build-windows-amd64.zip"

    if (($script:MINE -ne 1 -and -not (core_asset_fetch "crosstalk-$tag-src.tar.gz" "https://github.com/$($script:GH_REPO)/archive/refs/tags/$tag.tar.gz")) -or
        -not (core_asset_fetch $build_asset "https://github.com/$($script:GH_REPO)/releases/download/$tag/$build_asset"))
    {
        $script:NOTICE = "download failed; $($script:FETCH_NAME)"
        logging_line_wr 'ERR' "dangem: $($script:NOTICE)"
        return
    }

    $script:NOTICE = 'unpacking'
    core_frame_draw

    Remove-Item -LiteralPath "$($script:ROOT)/ldg", "$($script:ROOT)/res/utils/cc", "$($script:BUILD_DIR)/dangem", "$($script:BUILD_DIR)/gen" -Recurse -Force -ErrorAction SilentlyContinue

    $unpacked = $true

    if ($script:MINE -ne 1)
    {
        Remove-Item -LiteralPath "$($script:ROOT)/src" -Recurse -Force -ErrorAction SilentlyContinue
        & tar.exe -xzf "$($script:DL_DIR)/crosstalk-$tag-src.tar.gz" -C $script:ROOT --strip-components=1
        if ($LASTEXITCODE -ne 0) { $unpacked = $false }
    }

    if ($unpacked)
    {
        & tar.exe -xf "$($script:DL_DIR)/$build_asset" -C $script:ROOT
        if ($LASTEXITCODE -ne 0) { $unpacked = $false }
    }

    if (-not $unpacked)
    {
        $script:NOTICE = 'unpacking failed'
        logging_line_wr 'ERR' "dangem: unpacking failed; release: $tag"
        return
    }

    Remove-Item -LiteralPath $script:DL_DIR -Recurse -Force -ErrorAction SilentlyContinue

    if (-not (Test-Path -LiteralPath $script:SELF_DST))
    {
        try
        {
            New-Item -ItemType Directory -Force -Path (Split-Path -Parent $script:SELF_DST) | Out-Null
            Invoke-WebRequest -UseBasicParsing -Uri $script:SELF_URL -OutFile $script:SELF_DST
        }
        catch
        {
            logging_line_wr 'WARN' "dangem: self install failed; dst: $($script:SELF_DST)"
        }
    }

    compile_init
    core_deps_ck

    if ($script:DEPS_OK -eq 1) { $script:NOTICE = 'dependencies are in place' } else { $script:NOTICE = 'LDG is missing from this install' }
}

function core_deps_menu_req
{
    core_deps_ck
    $now = time_epoch_get

    if ($script:DEPS_OK -eq 1 -and ($script:DEPS_CONFIRM -ne 1 -or ($now - $script:DEPS_CONFIRM_TIME) -gt $script:LAUNCH_ARM_SPAN))
    {
        $script:DEPS_CONFIRM = 1
        $script:DEPS_CONFIRM_TIME = $now
        $script:NOTICE = $script:DEPS_CONFIRM_TEXT
        return
    }

    $script:DEPS_CONFIRM = 0
    core_deps_obtain
}

function core_key_route([string]$key)
{
    if ($key -ceq 'F10' -or ($script:SCREEN -eq 'menu' -and ($key -ceq 'q' -or $key -ceq 'ESC')))
    {
        if (-not (compile_busy_is)) { $script:QUIT = 1 }
        return
    }

    if ($script:SCREEN -eq 'summary')
    {
        if (compile_busy_is) { return }

        switch -CaseSensitive ($key)
        {
            'ESC' { $script:SCREEN = 'menu' }
            'F2' { core_launch_req }
            'F5' { core_build_kick 0 }
        }

        return
    }

    switch -CaseSensitive ($key)
    {
        'l' { core_launch_menu_req }
        'b' { core_build_kick 0 }
        'o' { core_deps_menu_req }
    }
}

function core_key_rd
{
    $script:KEY = ''

    if (-not [Console]::KeyAvailable)
    {
        Start-Sleep -Milliseconds $script:FRAME_WAIT
        return
    }

    $info = [Console]::ReadKey($true)

    switch ($info.Key)
    {
        'Escape' { $script:KEY = 'ESC'; return }
        'F2' { $script:KEY = 'F2'; return }
        'F5' { $script:KEY = 'F5'; return }
        'F10' { $script:KEY = 'F10'; return }
    }

    $script:KEY = ([string]$info.KeyChar).ToLowerInvariant()
}

function run_term_enter
{
    $script:CTRL_C_SAVED = [Console]::TreatControlCAsInput
    [Console]::TreatControlCAsInput = $true
    [Console]::Out.Write("${ESC}]0;dangem${ESC}\${ESC}[?1049h${ESC}[?25l")
}

function run_term_leave
{
    [Console]::Out.Write("${ESC}[0m${ESC}[?25h${ESC}[?1049l")
    [Console]::TreatControlCAsInput = $script:CTRL_C_SAVED
}

function main([string[]]$argv)
{
    foreach ($arg in $argv)
    {
        switch -CaseSensitive ($arg)
        {
            '--mine'
            {
                if (-not $PSCommandPath)
                {
                    [Console]::Out.WriteLine('dangem: --mine needs the script file in a checkout')
                    return
                }
                $script:MINE = 1
                $script:SRC_DIR = ((Join-Path (Split-Path -Parent $PSCommandPath) '../../src') -replace '\\', '/')
            }
            default
            {
                [Console]::Out.WriteLine("dangem: unknown argument; arg: $arg")
                return
            }
        }

        if (-not (Test-Path -LiteralPath $script:SRC_DIR -PathType Container))
        {
            [Console]::Out.WriteLine("dangem: no source tree next to this script; dir: $($script:SRC_DIR)")
            return
        }

        $script:SRC_DIR = ((Resolve-Path -LiteralPath $script:SRC_DIR).Path -replace '\\', '/')
        $tree_dir = ((Resolve-Path -LiteralPath "$($script:SRC_DIR)/..").Path -replace '\\', '/').TrimEnd('/')
        $root_dir = ([IO.Path]::GetFullPath($script:ROOT) -replace '\\', '/').TrimEnd('/')

        if ($tree_dir -ieq $root_dir)
        {
            [Console]::Out.WriteLine("dangem: --mine needs the checkout outside the dangem root; root: $($script:ROOT)")
            return
        }
    }

    if ([Console]::IsOutputRedirected -or [Console]::IsInputRedirected) { return }

    try
    {
        New-Item -ItemType Directory -Force -Path $script:ROOT | Out-Null
    }
    catch
    {
        return
    }

    compile_init
    core_deps_ck

    run_term_enter

    try
    {
        while ($script:QUIT -ne 1 -and $script:LAUNCH -ne 1)
        {
            core_frame_draw
            core_key_rd

            if ($script:KEY) { core_key_route $script:KEY }

            compile_pump

            if ($script:LAUNCH_AFTER -eq 1 -and $script:PHASE -eq 'done')
            {
                $script:LAUNCH_AFTER = 0
                $script:LAUNCH = 1
            }
        }
    }
    finally
    {
        run_term_leave
    }

    if ($script:LAUNCH -ne 1) { return }

    logging_line_wr 'INFO' "dangem: launching; path: $($script:OUT_PATH)"
    Set-Location -LiteralPath $script:ROOT
    & $script:OUT_PATH
}

main $args
