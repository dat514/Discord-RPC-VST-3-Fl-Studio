param(
    [switch] $NoPause
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$buildScript = Join-Path $PSScriptRoot "build-portable.ps1"
$targetDir = "C:\Program Files\Common Files\VST3"
$targetPlugin = Join-Path $targetDir "Discord Rich Presence.vst3"
$targetBinary = Join-Path $targetPlugin "Contents\x86_64-win\Discord Rich Presence.vst3"

function Wait-BeforeExit {
    if (-not $NoPause) {
        Write-Host ""
        Read-Host "Press Enter to close"
    }
}

function Test-IsAdmin {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Start-Elevated {
    $script = '"' + $PSCommandPath + '"'
    $arguments = "-NoProfile -ExecutionPolicy Bypass -File $script"

    if ($NoPause) {
        $arguments += " -NoPause"
    }

    Start-Process -FilePath "powershell.exe" -ArgumentList $arguments -WorkingDirectory $root -Verb RunAs
}

function Get-RunningDaws {
    Get-Process |
        Where-Object {
            $_.ProcessName -match '^(FL|FL64|Ableton Live|Live|REAPER|Cubase|Bitwig Studio|Waveform|Cakewalk|Reason|Studio One)$' -or
            $_.MainWindowTitle -match 'FL Studio|Ableton|REAPER|Cubase|Bitwig|Waveform|Cakewalk|Reason|Studio One'
        } |
        Sort-Object ProcessName, Id
}

function Test-PluginLoad {
    if (-not (Test-Path -LiteralPath $targetBinary)) {
        throw "Installed VST3 binary was not found: $targetBinary"
    }

    Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public static class NativePluginLoadCheck {
    [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)]
    public static extern IntPtr LoadLibraryW(string lpFileName);
    [DllImport("kernel32.dll", SetLastError=true)]
    public static extern bool FreeLibrary(IntPtr hModule);
}
"@

    $handle = [NativePluginLoadCheck]::LoadLibraryW($targetBinary)
    $errorCode = [Runtime.InteropServices.Marshal]::GetLastWin32Error()

    if ($handle -eq [IntPtr]::Zero) {
        throw "Windows could not load the installed VST3. Win32 error: $errorCode"
    }

    [NativePluginLoadCheck]::FreeLibrary($handle) | Out-Null
}

try {
    if (-not (Test-IsAdmin)) {
        Write-Host "Requesting administrator permission to install to $targetDir ..."
        Start-Elevated
        exit 0
    }

    Write-Host "Discord Rich Presence VST3 installer"
    Write-Host "Target: $targetDir"
    Write-Host ""

    $runningDaws = @(Get-RunningDaws)

    if ($runningDaws.Count -gt 0) {
        Write-Host "Close these apps before installing:"
        $runningDaws | Select-Object ProcessName, Id, MainWindowTitle | Format-Table -AutoSize
        throw "A DAW is still running and may lock the VST3 file."
    }

    & $buildScript -Install

    if ($LASTEXITCODE -ne 0) {
        throw "Build/install script failed with exit code $LASTEXITCODE"
    }

    Test-PluginLoad

    $installedFile = Get-Item -LiteralPath $targetBinary
    Write-Host ""
    Write-Host "Installed successfully."
    Write-Host "Plugin: $targetPlugin"
    Write-Host ("Binary size: {0:N0} bytes" -f $installedFile.Length)
    Write-Host ("Updated: {0}" -f $installedFile.LastWriteTime)
    Write-Host ""
    Write-Host "Open FL Studio and rescan VST3 plugins if it does not refresh automatically."
}
catch {
    Write-Host ""
    Write-Host "Install failed:" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    Wait-BeforeExit
    exit 1
}

Wait-BeforeExit
