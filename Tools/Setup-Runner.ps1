# Copyright 2024 GeoTrinity. All Rights Reserved.
<#
.SYNOPSIS
    Run once, elevated, on the machine hosting the GitHub Actions runner: makes the Unreal source engine
    visible to CI builds.

.DESCRIPTION
    Why this is needed at all. Unreal registers source builds under
    HKCU\SOFTWARE\Epic Games\Unreal Engine\Builds -- HKCU meaning the hive of whoever was logged in at the
    time. The runner is a Windows service running as LocalSystem, and SYSTEM's HKCU is
    HKEY_USERS\S-1-5-18: a different hive entirely. So Tools\Resolve-Engine.ps1 finds the engine when you
    run it from your desktop session, and finds nothing when CI runs it -- then falls back to a launcher
    install (which cannot build the dedicated server) or fails outright.

    The fix is a machine-wide UE environment variable, which every account including SYSTEM can read and
    which Resolve-Engine.ps1 honours as its override. This does not put any path into the repo: the path
    stays on the machine it describes.

    Only runner machines need this. A normal dev PC resolves its engine automatically.

.PARAMETER EngineRoot
    Source engine to pin, e.g. H:\Epic\UE_5.7. Omit to auto-detect the engine registered for the current
    user (run it from the desktop session where the source build was registered).

.PARAMETER SkipServiceRestart
    Leave the runner service alone. Services read their environment at start, so the new variable will not
    apply until the service is restarted or the box reboots.

.EXAMPLE
    # Elevated, on the runner box:
    .\Tools\Setup-Runner.ps1

.EXAMPLE
    .\Tools\Setup-Runner.ps1 -EngineRoot "H:\Epic\UE_5.7"

.EXAMPLE
    .\Tools\Setup-Runner.ps1 -WhatIf     # show what it would do, change nothing
#>
[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [string]$EngineRoot,
    [switch]$SkipServiceRestart
)

$ErrorActionPreference = "Stop"

$scriptDir = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }

# Both Join-Path and Test-Path throw rather than returning false when the drive itself is missing (a typo'd
# "H:\..." on a box with no H:), and the raw "Cannot find drive" masks what this script was trying to tell
# you. The Join-Path must be inside the try for that reason -- it throws first.
function Test-EnginePath([string]$Root, [string]$Relative) {
    try { return Test-Path (Join-Path $Root $Relative) } catch { return $false }
}

$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()
           ).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin -and -not $WhatIfPreference) {
    throw "Run this from an elevated prompt: writing a machine-wide environment variable and restarting the runner service both need Administrator."
}

# 1. Work out which engine to pin. Auto-detect resolves via the CURRENT user's HKCU -- which is exactly the
#    hive SYSTEM cannot see, and exactly why we are copying the answer somewhere SYSTEM can read.
if (-not $EngineRoot) {
    Write-Host "No -EngineRoot given; auto-detecting the engine registered for $env:USERNAME..."
    $EngineRoot = & (Join-Path $scriptDir "Resolve-Engine.ps1") -Verbose:$VerbosePreference
}

if (-not (Test-EnginePath $EngineRoot "Engine\Build\BatchFiles\Build.bat")) {
    throw "'$EngineRoot' is not an engine root (no Engine\Build\BatchFiles\Build.bat)."
}

# 2. A runner that cannot build the dedicated server defeats the point of this machine, so say so loudly --
#    but do not hard-fail: a client-only runner is a legitimate setup.
$isSource = -not (Test-EnginePath $EngineRoot "Engine\Build\InstalledBuild.txt")
if (-not $isSource) {
    Write-Warning @"
'$EngineRoot' is a launcher (installed) build, not a source build.
CI can still package the client, but 'build_server: true' will fail: launcher installs ship no UnrealServer
libs and no launcher option adds them. If this box has a source engine too, pass it via -EngineRoot.
"@
}

$version = try {
    $j = Get-Content -Raw -Path (Join-Path $EngineRoot "Engine\Build\Build.version") | ConvertFrom-Json
    "$($j.MajorVersion).$($j.MinorVersion)"
} catch { "unknown" }

Write-Host ""
Write-Host "Engine : $EngineRoot"
Write-Host "Version: $version"
Write-Host "Kind   : $(if ($isSource) { 'source build (can build dedicated server)' } else { 'launcher install (cannot build dedicated server)' })"
Write-Host ""

# 3. Machine scope is the point: readable by SYSTEM, unlike the per-user registration.
if ($PSCmdlet.ShouldProcess("machine-wide environment variable UE", "set to '$EngineRoot'")) {
    [Environment]::SetEnvironmentVariable("UE", $EngineRoot, "Machine")
    Write-Host "Set machine-wide UE = $EngineRoot"
}

# 4. Services read their environment at start, so the variable means nothing to a running runner.
$services = @(Get-Service -Name "actions.runner.*" -ErrorAction SilentlyContinue)
if (-not $services) {
    Write-Warning "No 'actions.runner.*' service found on this machine. If the runner is installed under a different name, restart it by hand so it picks up the new variable."
} elseif ($SkipServiceRestart) {
    Write-Host "Skipping service restart (-SkipServiceRestart). Restart these before the next build:"
    $services | ForEach-Object { Write-Host "  $($_.Name)" }
} else {
    foreach ($svc in $services) {
        if ($PSCmdlet.ShouldProcess($svc.Name, "restart so it picks up the new environment")) {
            Restart-Service -Name $svc.Name -Force
            Write-Host "Restarted $($svc.Name)"
        }
    }
}

Write-Host ""
Write-Host "Done. Verify what CI will now resolve:"
Write-Host "  .\Tools\Resolve-Engine.ps1 -Verbose      (should report: Engine from `$env:UE)"
