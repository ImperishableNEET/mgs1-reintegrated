param(
    [string]$Disc,
    [string]$DiscTwo,
    [string]$Executable,
    [string]$Resolution = '640x480',
    [switch]$Scanlines,
    [switch]$CodecTurbo
)
$ErrorActionPreference = 'Stop'
$localRoot = Join-Path $env:LOCALAPPDATA 'MGS-Reintegrated'
New-Item -ItemType Directory -Force -Path $localRoot | Out-Null
$configPath = Join-Path $localRoot 'launcher.json'
$config = if (Test-Path -LiteralPath $configPath) { Get-Content -LiteralPath $configPath -Raw | ConvertFrom-Json } else { $null }
if (-not $Disc -and $config) { $Disc = $config.disc }
if (-not $DiscTwo -and $config) { $DiscTwo = $config.discTwo }
if (-not $Disc) { $Disc = Read-Host 'Path to Integral Disc 1 CUE' }
$Disc = (Resolve-Path -LiteralPath $Disc).Path
if ($DiscTwo) { $DiscTwo = (Resolve-Path -LiteralPath $DiscTwo).Path }
if (-not $Executable) {
    $choices = @('mgs-integral-native.exe', 'build/windows-x86-relwithdebinfo/mgs-integral-native.exe', 'build/windows-x86-debug/mgs-integral-native.exe')
    foreach ($choice in $choices) {
        $path = Join-Path $PSScriptRoot $choice
        if (Test-Path -LiteralPath $path) { $Executable = $path; break }
    }
}
if (-not $Executable) { throw 'Build the modern renderer first; see README.md.' }
$Executable = (Resolve-Path -LiteralPath $Executable).Path
@{disc=$Disc;discTwo=$DiscTwo} | ConvertTo-Json | Set-Content -LiteralPath $configPath -Encoding UTF8
$names = @('REI_ENGLISH_CAPTIONS','REI_ORIGINAL_RES','REI_REINTEGRATED_TITLE','REI_SCANLINES','REI_CODEC_TURBO','REI_TITLE_HOLD','REI_TITLE_PREVIEW','REI_PROBE_STICK','REI_WINDOW_SMOKE','REI_PROBE_CODEC_TURBO','REI_PROBE_CODEC_RELEASE')
$previous = @{}
foreach ($name in $names) { $previous[$name] = [Environment]::GetEnvironmentVariable($name,'Process'); [Environment]::SetEnvironmentVariable($name,$null,'Process') }
try {
    $env:REI_ENGLISH_CAPTIONS='1'
    $env:REI_ORIGINAL_RES='1'
    $env:REI_REINTEGRATED_TITLE='1'
    if ($Scanlines) { $env:REI_SCANLINES='1' }
    if ($CodecTurbo) { $env:REI_CODEC_TURBO='1' }
    $gameArgs = @('--disc',$Disc,'--resolution',$Resolution)
    if ($DiscTwo) { $gameArgs += @('--disc-two',$DiscTwo) }
    Push-Location $localRoot
    try {
        & $Executable @gameArgs
        if ($null -ne $LASTEXITCODE -and $LASTEXITCODE -ne 0) { throw "Game exited with code $LASTEXITCODE" }
    } finally { Pop-Location }
} finally {
    foreach ($name in $names) { [Environment]::SetEnvironmentVariable($name,$previous[$name],'Process') }
}
