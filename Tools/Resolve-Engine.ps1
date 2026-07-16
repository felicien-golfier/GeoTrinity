# Copyright 2024 GeoTrinity. All Rights Reserved.
<#
.SYNOPSIS
    Prints this machine's Unreal Engine root, resolved from the .uproject's EngineAssociation, so build
    scripts adapt to whatever PC they run on instead of hardcoding a path.

.DESCRIPTION
    "EngineAssociation" is a registry LOOKUP KEY, not a path -- which is why one committed .uproject can
    mean a launcher install on one PC and a source build on another. Note that Build.bat / RunUAT.bat
    ignore it entirely and use whichever engine's BatchFiles you invoke; this script bridges the two.

    Unreal registers engines itself, so nothing here needs setting up by hand. But the two kinds of
    engine are registered very differently, and that shapes this whole script:

      * LAUNCHER installs -> HKLM:\SOFTWARE\EpicGames\Unreal Engine\<version>, keyed by version
        ("5.7"), value InstalledDirectory. An EngineAssociation of "5.7" matches directly.

      * SOURCE builds -> HKCU:\SOFTWARE\Epic Games\Unreal Engine\Builds, keyed by a RANDOM GUID.
        FDesktopPlatformWindows::RegisterEngineInstallation does:
            FString NewIdentifier = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensInBraces);
        That GUID is unique per machine, so a shared .uproject can never name it. Hence step 3 below:
        identify a source build by reading its Engine\Build\Build.version and matching MajorVersion.
        MinorVersion against the association.

    Do NOT "fix" this by adding a registry alias named "5.7" next to the GUID. EnumerateEngineInstallations
    dedupes by DIRECTORY and RegDeleteValue's every duplicate it finds, so of two names pointing at one
    engine, whichever enumerates second is silently deleted -- a coin flip that breaks builds later.

    Resolution order:
      1. -Override <path>                      explicit; wins over everything.
      2. $env:UE                               same, via environment.
      3. HKCU Builds, exact name match         covers a GUID-style association.
      4. HKCU Builds, version match            source builds (the normal CI-runner case).
      5. HKLM installs, exact name match       launcher installs (the normal dev-PC case).

    Source builds are preferred over launcher installs, which differs from Unreal's own resolution (it
    would take the launcher for a "5.7" association). Deliberate: a machine with both went to the trouble
    of a source build for a reason -- notably dedicated-server targets, which launcher installs cannot
    build. Pass -Override to force the other one.

    Two more traps, both real:
      * Epic spells it "EpicGames" (no space) under HKLM but "Epic Games" (with a space) under HKCU.
      * The registry keeps advertising engines whose folders were deleted -- the main dev PC still lists
        5.5 and 5.6 -- so every candidate is validated by checking Build.bat really exists.

.PARAMETER Override
    Explicit engine root. Use to force a specific engine, or when one is somehow not registered.

.PARAMETER UProject
    Path to the .uproject. Defaults to the repo's GeoTrinity.uproject next to this script.

.PARAMETER RequireSourceBuild
    Fail unless the resolved engine is a source build. Dedicated-server targets need this: launcher
    installs ship no UnrealServer libs to link against, and no launcher option adds them.

.EXAMPLE
    $ue = & Tools\Resolve-Engine.ps1
    & "$ue\Engine\Build\BatchFiles\Build.bat" GeoTrinity Win64 Development -Project=...
#>
[CmdletBinding()]
param(
    [string]$Override,
    [string]$UProject,
    [switch]$RequireSourceBuild
)

$ErrorActionPreference = "Stop"

# Resolved in the body, not as a param default: $PSScriptRoot is empty during param-default evaluation
# when the script is launched via "powershell -File" (which is how the .bat wrappers call it), and the
# resulting Join-Path error masks whatever this script was actually trying to report.
if (-not $UProject) {
    $scriptDir = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
    $UProject = Join-Path $scriptDir "..\GeoTrinity.uproject"
}

# An engine root is only real if it can actually build. Test-Path throws rather than returning false when
# the drive itself is missing (a stale "H:\..." on a machine with no H:), so swallow that.
function Test-EngineRoot([string]$Root) {
    if ([string]::IsNullOrWhiteSpace($Root)) { return $false }
    try { return Test-Path (Join-Path $Root "Engine\Build\BatchFiles\Build.bat") } catch { return $false }
}

# "5.7" from Engine\Build\Build.version. Present in both launcher and source builds.
function Get-EngineVersion([string]$Root) {
    try {
        $bv = Join-Path $Root "Engine\Build\Build.version"
        if (-not (Test-Path $bv)) { return $null }
        $j = Get-Content -Raw -Path $bv | ConvertFrom-Json
        return "$($j.MajorVersion).$($j.MinorVersion)"
    } catch { return $null }
}

# InstalledBuild.txt is the definitive "this is a launcher/binary engine" marker.
function Test-IsSourceBuild([string]$Root) {
    return -not (Test-Path (Join-Path $Root "Engine\Build\InstalledBuild.txt"))
}

$engine = $null
$from = $null

# 1 & 2. Explicit overrides.
$explicit = if ($Override) { $Override } elseif ($env:UE) { $env:UE } else { $null }
if ($explicit) {
    $label = if ($Override) { "-Override" } else { "`$env:UE" }
    if (-not (Test-EngineRoot $explicit)) {
        throw "$label is set to '$explicit' but that is not an engine root (no Engine\Build\BatchFiles\Build.bat)."
    }
    $engine = $explicit
    $from = $label
}

if (-not $engine) {
    if (-not (Test-Path $UProject)) { throw "No .uproject at '$UProject'." }

    $assoc = (Get-Content -Raw -Path $UProject | ConvertFrom-Json).EngineAssociation
    if ([string]::IsNullOrWhiteSpace($assoc)) {
        throw "'$UProject' has an empty EngineAssociation, which means 'engine lives in a parent folder'. This repo does not use that layout -- pass -Override instead."
    }

    # Source builds: HKCU, "Epic Games" WITH a space. Value name = GUID (or whatever), data = engine root.
    $builds = @()
    $buildsKey = "HKCU:\SOFTWARE\Epic Games\Unreal Engine\Builds"
    if (Test-Path $buildsKey) {
        $builds = (Get-ItemProperty -Path $buildsKey).PSObject.Properties |
                  Where-Object { $_.Name -notlike "PS*" -and (Test-EngineRoot $_.Value) }
    }

    # 3. Exact name match -- covers an association that is itself a GUID.
    $hit = $builds | Where-Object { $_.Name -eq $assoc } | Select-Object -First 1
    if ($hit) {
        $engine = $hit.Value
        $from = "HKCU registered builds, name '$assoc'"
    }

    # 4. Version match -- the normal source-build case, whose GUID no .uproject could name.
    if (-not $engine) {
        $hit = $builds | Where-Object { (Get-EngineVersion $_.Value) -eq $assoc } | Select-Object -First 1
        if ($hit) {
            $engine = $hit.Value
            $from = "HKCU registered builds, version $assoc (registered as '$($hit.Name)')"
        }
    }

    # 5. Launcher installs: HKLM, "EpicGames" NO space, subkey = version.
    if (-not $engine) {
        $installedKey = "HKLM:\SOFTWARE\EpicGames\Unreal Engine\$assoc"
        if (Test-Path $installedKey) {
            $dir = (Get-ItemProperty -Path $installedKey).InstalledDirectory
            if (Test-EngineRoot $dir) {
                $engine = $dir
                $from = "HKLM launcher installs, version '$assoc'"
            }
        }
    }

    if (-not $engine) {
        $seen = if ($builds) { ($builds | ForEach-Object { "    {0} -> {1} (version {2})" -f $_.Name, $_.Value, (Get-EngineVersion $_.Value) }) -join "`n" } else { "    (none)" }
        throw @"
Could not resolve an engine for EngineAssociation '$assoc'.
Checked:
  -Override / `$env:UE                                  (not set)
  HKCU:\SOFTWARE\Epic Games\Unreal Engine\Builds        (source builds, by name and by version):
$seen
  HKLM:\SOFTWARE\EpicGames\Unreal Engine\$assoc         (launcher installs)
Unreal registers an engine the first time it needs its identifier, so an engine that never appears here
may simply never have been opened. Open the .uproject once with it, or pass -Override <engine root>.
"@
    }
}

if ($RequireSourceBuild -and -not (Test-IsSourceBuild $engine)) {
    throw @"
'$engine' is a launcher (installed) build, but a source build is required.
Installed builds ship precompiled libs for UnrealEditor and UnrealGame only -- there is no UnrealServer,
and no launcher option adds one, so dedicated-server targets cannot be linked. Build the server on a
machine with a source engine (the CI runner), or via the "Build GeoTrinity (Custom)" workflow.
"@
}

Write-Verbose "Engine from $from"
Write-Output $engine
