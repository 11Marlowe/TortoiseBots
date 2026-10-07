# TortoiseBots dashboard zero-setup: download the prebuilt
# tortoise-observability binary (no Go toolchain needed), wire it to the
# databases from mangosd.conf, enable module telemetry, and start it.
#
# Usage:
#   .\run-dashboard.ps1 [-MangosdConf C:\path\to\mangosd.conf] [-Tag v2026-10-07]
$ErrorActionPreference = "Stop"

param(
  [string]$MangosdConf = $env:MANGOSD_CONF,
  [string]$Tag = "",
  [int]$HttpPort = 8095,
  [int]$UdpPort = 9195
)

$Repo = "Sagiroth/TortoiseBots"
$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$Bin = Join-Path $Here "tortoise-observability.exe"
$LogFile = Join-Path $Here "dashboard.log"

function Find-Conf {
  if ($MangosdConf -and (Test-Path $MangosdConf)) { return $MangosdConf }
  foreach ($c in @(
    (Join-Path $Here "..\etc\mangosd.conf"),
    (Join-Path $Here "etc\mangosd.conf"),
    ".\etc\mangosd.conf"
  )) {
    if (Test-Path $c) { return (Resolve-Path $c).Path }
  }
  throw "mangosd.conf not found (use -MangosdConf PATH)"
}

# "host;port;user;pass;db" from e.g. LoginDatabase.Info = "127.0.0.1;3306;mangos;mangos;tw_logon"
function Get-DbField($Key, $Index, $Conf) {
  $line = Get-Content $Conf | Where-Object { $_ -match "^\s*${Key}\s*=" } | Select-Object -Last 1
  if (-not $line) { return $null }
  $val = ($line -split "=", 2)[1].Trim().Trim('"')
  return ($val -split ";")[$Index]
}

function Resolve-Tag {
  if ($Tag) { return $Tag }
  $resp = Invoke-WebRequest "https://github.com/${Repo}/releases/latest" -MaximumRedirection 0 -SkipHttpErrorCheck
  $loc = $resp.Headers.Location
  if (-not $loc) {
    # Followed redirect already; fall back to the API.
    $rel = Invoke-RestMethod "https://api.github.com/repos/${Repo}/releases/latest"
    return $rel.tag_name
  }
  return ($loc -split "/")[-1]
}

$Conf = Find-Conf
Write-Host "mangosd.conf: $Conf"

$WantTag = Resolve-Tag
Write-Host "release: $WantTag"
$HaveTag = ""
if (Test-Path "$Bin.tag") { $HaveTag = (Get-Content "$Bin.tag" -Raw).Trim() }
if ((-not (Test-Path $Bin)) -or ($HaveTag -ne $WantTag)) {
  Write-Host "downloading tortoise-observability $WantTag ..."
  $url = "https://github.com/${Repo}/releases/download/${WantTag}/tortoise-observability-windows-amd64.exe"
  Invoke-WebRequest $url -OutFile "$Bin.new"
  Move-Item "$Bin.new" $Bin -Force
  Set-Content "$Bin.tag" $WantTag -NoNewline
} else {
  Write-Host "binary up to date ($WantTag)"
}

$Host_ = Get-DbField "LoginDatabase.Info" 0 $Conf
if (-not $Host_) { $Host_ = "127.0.0.1" }
$Port = Get-DbField "LoginDatabase.Info" 1 $Conf
if (-not $Port) { $Port = "3306" }
$User = Get-DbField "LoginDatabase.Info" 2 $Conf
if (-not $User) { $User = "mangos" }
$Pass = Get-DbField "LoginDatabase.Info" 3 $Conf
if ($null -eq $Pass) { $Pass = "mangos" }
$LoginDb = Get-DbField "LoginDatabase.Info" 4 $Conf
if (-not $LoginDb) { $LoginDb = "tw_logon" }
$CharDb = Get-DbField "CharacterDatabase.Info" 4 $Conf
if (-not $CharDb) { $CharDb = "tw_char" }
$WorldDb = Get-DbField "WorldDatabase.Info" 4 $Conf
if (-not $WorldDb) { $WorldDb = "tw_world" }

# DBC dir next to DataDir in mangosd.conf (talent trees need it).
$DbcDir = ""
$dataLine = Get-Content $Conf | Where-Object { $_ -match "^\s*DataDir\s*=" } | Select-Object -Last 1
if ($dataLine) {
  $dataDir = ($dataLine -split "=", 2)[1].Trim().Trim('"')
  $confDir = Split-Path -Parent $Conf
  if ([System.IO.Path]::IsPathRooted($dataDir)) { $cand = Join-Path $dataDir "dbc" }
  else { $cand = Join-Path $confDir (Join-Path $dataDir "dbc") }
  if (Test-Path $cand) { $DbcDir = (Resolve-Path $cand).Path }
}

function Set-ConfLine($File, $Key, $Value) {
  $lines = Get-Content $File
  $pat = "^\s*${Key}\s*="
  if ($lines -match $pat) {
    $lines = $lines | ForEach-Object { if ($_ -match $pat) { "${Key} = ${Value}" } else { $_ } }
  } else {
    $lines += "${Key} = ${Value}"
  }
  Set-Content $File $lines
}

$AiConf = Join-Path (Split-Path -Parent $Conf) "aiplayerbot.conf"
if (Test-Path $AiConf) {
  Set-ConfLine $AiConf "AiPlayerbot.Observability" "1"
  Set-ConfLine $AiConf "AiPlayerbot.ObservabilityPort" "$UdpPort"
  Write-Host "telemetry enabled in $AiConf (restart mangosd to apply)"
} else {
  Write-Warning "$AiConf not found; set AiPlayerbot.Observability = 1 yourself"
}

try {
  Invoke-WebRequest "http://127.0.0.1:${HttpPort}/metrics" -TimeoutSec 2 -UseBasicParsing | Out-Null
  Write-Host "dashboard already running at http://localhost:${HttpPort}/dashboard"
  exit 0
} catch { }

$env:DB_HOST = $Host_
$env:DB_PORT = $Port
$env:DB_USER = $User
$env:DB_PASSWORD = $Pass
$env:DB_LOGIN = $LoginDb
$env:DB_CHAR = $CharDb
$env:DB_WORLD = $WorldDb
$env:HTTP_PORT = "$HttpPort"
$env:UDP_PORT = "$UdpPort"
$env:UDP_HOST = "127.0.0.1"
if ($DbcDir) { $env:DBC_DIR = $DbcDir }
$iconDir = if ($env:ICON_CACHE_DIR) { $env:ICON_CACHE_DIR } else { Join-Path $Here ".icon-cache" }
New-Item -ItemType Directory -Force -Path $iconDir | Out-Null
$env:ICON_CACHE_DIR = $iconDir

Write-Host "starting dashboard (logs: $LogFile) ..."
$proc = Start-Process -FilePath $Bin -RedirectStandardOutput $LogFile -RedirectStandardError $LogFile -PassThru
Start-Sleep -Seconds 2
try {
  Invoke-WebRequest "http://127.0.0.1:${HttpPort}/metrics" -TimeoutSec 3 -UseBasicParsing | Out-Null
  Write-Host "dashboard up at http://localhost:${HttpPort}/dashboard"
} catch {
  Write-Error "start failed; see $LogFile"
  exit 1
}
