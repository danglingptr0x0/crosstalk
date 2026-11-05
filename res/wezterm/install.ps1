$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

$base = 'https://github.com/wezterm/wezterm/releases/download/nightly'
$name = 'WezTerm-nightly-setup.exe'

$known = @(
    (Join-Path $env:ProgramFiles 'WezTerm\wezterm-gui.exe'),
    (Join-Path $env:LOCALAPPDATA 'Programs\WezTerm\wezterm-gui.exe')
)

foreach ($path in $known) {
    if (Test-Path -LiteralPath $path) { exit 0 }
}

if (Get-Command 'wezterm-gui.exe' -ErrorAction SilentlyContinue) { exit 0 }

[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

$dir = Join-Path $env:TEMP ('crosstalk-wezterm-' + [Guid]::NewGuid().ToString('N'))
$exe = Join-Path $dir $name
$sum = Join-Path $dir ($name + '.sha256')
$code = 1

try {
    New-Item -ItemType Directory -Path $dir | Out-Null

    Invoke-WebRequest -UseBasicParsing -Uri ($base + '/' + $name) -OutFile $exe
    Invoke-WebRequest -UseBasicParsing -Uri ($base + '/' + $name + '.sha256') -OutFile $sum

    $want = ((Get-Content -LiteralPath $sum -TotalCount 1) -split '\s+')[0].ToLowerInvariant()
    $got = (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLowerInvariant()

    if ($want -notmatch '^[0-9a-f]{64}$' -or $want -ne $got) {
        Write-Error ('WezTerm installer hash mismatch; want: ' + $want + '; got: ' + $got)
    }

    $proc = Start-Process -FilePath $exe -ArgumentList '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/SP-' -Wait -PassThru
    $code = $proc.ExitCode
}
catch {
    Write-Error $_ -ErrorAction Continue
    $code = 1
}
finally {
    Remove-Item -LiteralPath $dir -Recurse -Force -ErrorAction SilentlyContinue
}

exit $code
