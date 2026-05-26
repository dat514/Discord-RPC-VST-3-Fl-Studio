param(
    [switch] $Install,
    [switch] $CleanTools
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$tools = Join-Path $root ".tools"
$downloads = Join-Path $tools "downloads"
$cmakeDir = Join-Path $tools "cmake"
$llvmDir = Join-Path $tools "llvm-mingw"
$build = Join-Path $root "build-portable"

function Invoke-Native($file, [string[]] $arguments) {
    & $file @arguments

    if ($LASTEXITCODE -ne 0) {
        throw "$file failed with exit code $LASTEXITCODE"
    }
}

function New-Directory($path) {
    New-Item -ItemType Directory -Force -Path $path | Out-Null
}

function Get-LatestRelease($repo) {
    $headers = @{
        "User-Agent" = "DiscordRichPresenceVST-build"
        "Accept" = "application/vnd.github+json"
    }

    Invoke-RestMethod -Headers $headers -Uri "https://api.github.com/repos/$repo/releases/latest"
}

function Save-File($url, $path) {
    if (Test-Path $path) {
        return
    }

    Write-Host "Downloading $url"
    Invoke-WebRequest -Uri $url -OutFile $path -UseBasicParsing
}

function Expand-FirstChild($zip, $destination, $finalName) {
    $temp = Join-Path $destination ([System.IO.Path]::GetFileNameWithoutExtension($zip) + "-extract")

    if (Test-Path $temp) {
        Remove-Item -LiteralPath $temp -Recurse -Force
    }

    New-Directory $temp
    Expand-Archive -LiteralPath $zip -DestinationPath $temp -Force

    $child = Get-ChildItem -LiteralPath $temp | Select-Object -First 1
    $final = Join-Path $destination $finalName

    if (Test-Path $final) {
        Remove-Item -LiteralPath $final -Recurse -Force
    }

    Move-Item -LiteralPath $child.FullName -Destination $final
    Remove-Item -LiteralPath $temp -Recurse -Force
}

if ($CleanTools -and (Test-Path $tools)) {
    Remove-Item -LiteralPath $tools -Recurse -Force
}

New-Directory $tools
New-Directory $downloads

if (-not (Test-Path (Join-Path $cmakeDir "bin\cmake.exe"))) {
    $cmakeRelease = Get-LatestRelease "Kitware/CMake"
    $cmakeAsset = $cmakeRelease.assets |
        Where-Object { $_.name -match '^cmake-.*-windows-x86_64\.zip$' } |
        Select-Object -First 1

    if (-not $cmakeAsset) {
        throw "Could not find a Windows x64 CMake zip in the latest CMake release."
    }

    $cmakeZip = Join-Path $downloads $cmakeAsset.name
    Save-File $cmakeAsset.browser_download_url $cmakeZip
    Expand-FirstChild $cmakeZip $tools "cmake"
}

if (-not (Test-Path (Join-Path $llvmDir "bin\clang++.exe"))) {
    $llvmRelease = Get-LatestRelease "mstorsjo/llvm-mingw"
    $llvmAsset = $llvmRelease.assets |
        Where-Object { $_.name -match '^llvm-mingw-.*-ucrt-x86_64\.zip$' } |
        Select-Object -First 1

    if (-not $llvmAsset) {
        throw "Could not find a Windows x64 LLVM-MinGW zip in the latest llvm-mingw release."
    }

    $llvmZip = Join-Path $downloads $llvmAsset.name
    Save-File $llvmAsset.browser_download_url $llvmZip
    Expand-FirstChild $llvmZip $tools "llvm-mingw"
}

$env:PATH = (Join-Path $cmakeDir "bin") + ";" + (Join-Path $llvmDir "bin") + ";" + $env:PATH

$cmake = Join-Path $cmakeDir "bin\cmake.exe"
$clang = (Join-Path $llvmDir "bin\clang.exe").Replace("\", "/")
$clangxx = (Join-Path $llvmDir "bin\clang++.exe").Replace("\", "/")
$rc = (Join-Path $llvmDir "bin\llvm-rc.exe").Replace("\", "/")

Invoke-Native $cmake @(
    "-S", $root,
    "-B", $build,
    "-G", "Ninja",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_C_COMPILER=$clang",
    "-DCMAKE_CXX_COMPILER=$clangxx",
    "-DCMAKE_RC_COMPILER=$rc"
)

Invoke-Native $cmake @("--build", $build, "--config", "Release")

$artifact = Join-Path $build "DiscordRichPresence_artefacts\Release\VST3\Discord Rich Presence.vst3"

if ($Install) {
    $targetDir = "C:\Program Files\Common Files\VST3"
    New-Directory $targetDir
    Copy-Item -LiteralPath $artifact -Destination $targetDir -Recurse -Force
    Write-Host "Installed to $targetDir"
} else {
    Write-Host "Built: $artifact"
}
