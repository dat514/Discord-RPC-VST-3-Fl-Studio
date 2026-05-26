param(
    [switch] $Install
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$build = Join-Path $root "build"

cmake -S $root -B $build -G "Visual Studio 17 2022" -A x64
cmake --build $build --config Release

$artifact = Join-Path $build "DiscordRichPresence_artefacts\Release\VST3\Discord Rich Presence.vst3"

if ($Install) {
    $targetDir = "C:\Program Files\Common Files\VST3"
    New-Item -ItemType Directory -Force -Path $targetDir | Out-Null
    Copy-Item -Path $artifact -Destination $targetDir -Recurse -Force
    Write-Host "Installed to $targetDir"
} else {
    Write-Host "Built: $artifact"
}
