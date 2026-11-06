# Copyright 2024 GeoTrinity. All Rights Reserved.
<#
.SYNOPSIS
    The build + test gate in one call: checks Unreal processes, builds the editor, runs the asked tests in one
    headless editor launch, prints only what matters and ends on one GATE line.

.DESCRIPTION
    Tokens (any order, combined into one run):
      (none)      quick suite
      quick       quick suite: every GeoTrinity. and GeoTrinityUI. test except the tutorial bot sessions
      bots        every tutorial bot session (GeoTrinity.Tutorial.FullRun, .Path, .Jump): slow, minutes
      nobuild     skip the build (only when nothing changed since the last build of the same config)
      dry         print the config and the filters, run nothing
      <filter>    any test path prefix, e.g. GeoTrinity.Gems or GeoTrinity.Tutorial.Path
    The quick suite is read from the test sources, so a new test area joins it with no edit here.

    Open DebugGame editor -> Development build + UnrealEditor-Cmd.exe (AI/Commands.md, "Build and test beside an
    open editor"). Another -Cmd run or a build in progress -> BLOCKED: the report folder and UBT are shared.

    Output is also written to AI/Output/Gate/last.txt. Exit: 0 GREEN, 1 RED (build or tests), 2 BLOCKED.
#>
param(
    [Parameter(ValueFromRemainingArguments)] [string[]] $Tokens
)

$Repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Project = Join-Path $Repo 'GeoTrinity.uproject'
$TestSources = Join-Path $Repo 'Source\GeoTrinityEditor\Private\Tests'
$TestOut = Join-Path $Repo 'AI\Output\Tests'
$BuildOut = Join-Path $Repo 'AI\Output\Build'
$GateOut = Join-Path $Repo 'AI\Output\Gate'
$BotPattern = '^GeoTrinity\.Tutorial\.(FullRun|Path|Jump)(\.|$)'
$BotFilters = @('GeoTrinity.Tutorial.FullRun', 'GeoTrinity.Tutorial.Path', 'GeoTrinity.Tutorial.Jump')

$Lines = New-Object System.Collections.Generic.List[string]
function Say([string] $Text) { Write-Host $Text; $Lines.Add($Text) }
function Finish([int] $Code, [string] $Verdict) {
    Say "GATE: $Verdict"
    New-Item -ItemType Directory -Force $GateOut | Out-Null
    $Lines | Out-File (Join-Path $GateOut 'last.txt') -Encoding utf8
    exit $Code
}
# Runs a reporting script, echoing and recording its Write-Host lines; returns its exit code.
function Invoke-Reader([string] $Script, [hashtable] $Arguments) {
    & (Join-Path $PSScriptRoot $Script) @Arguments 6>&1 | ForEach-Object { Say "$_" }
    return $LASTEXITCODE
}

# Quick suite from the sources: one filter per area, the tutorial area test by test without its bot sessions.
function Get-QuickFilters {
    $names = Get-ChildItem $TestSources -Recurse -Include *.cpp, *.h |
        Select-String -Pattern '"((GeoTrinity|GeoTrinityUI)\.[A-Za-z0-9_.]+)"' -AllMatches |
        ForEach-Object { $_.Matches } | ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique
    $filters = foreach ($name in $names) {
        $parts = $name.Split('.')
        if ($parts.Count -lt 2 -or $parts[1] -eq '' -or $parts[1] -eq 'h') { continue }
        if ($parts[0] -ne 'GeoTrinity' -or $parts[1] -ne 'Tutorial') { "$($parts[0]).$($parts[1])"; continue }
        if ($parts.Count -ge 3 -and $name -notmatch $BotPattern) { $name }
    }
    return @($filters | Sort-Object -Unique)
}

# --- Tokens ---
$filters = New-Object System.Collections.Generic.List[string]
$explicit = New-Object System.Collections.Generic.List[string]
$build = $true
$dry = $false
if (-not $Tokens) { $Tokens = @() }
if (-not ($Tokens | Where-Object { $_ -notmatch '^(nobuild|dry)$' })) { $Tokens += 'quick' }
foreach ($token in $Tokens) {
    switch -Regex ($token) {
        '^quick$' { Get-QuickFilters | ForEach-Object { $filters.Add($_) } }
        '^bots$' { $BotFilters | ForEach-Object { $filters.Add($_); $explicit.Add($_) } }
        '^nobuild$' { $build = $false }
        '^dry$' { $dry = $true }
        '^GeoTrinity(UI)?\.' { $filters.Add($token); $explicit.Add($token) }
        default { Say "Unknown token '$token' (quick | bots | nobuild | dry | GeoTrinity.<filter> | GeoTrinityUI.<filter>)."; Finish 2 'BLOCKED (bad arguments)' }
    }
}
$filters = @($filters | Select-Object -Unique)
if ($filters.Count -eq 0) { Say 'No test filter.'; Finish 2 'BLOCKED (no filter)' }

# --- Processes ---
$procs = @(Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.ProcessName -like 'UnrealEditor*' })
$cmdRuns = @($procs | Where-Object { $_.ProcessName -like '*-Cmd' })
$editors = @($procs | Where-Object { $_.ProcessName -notlike '*-Cmd' })
$ubt = @(Get-CimInstance Win32_Process -Filter "Name='dotnet.exe' OR Name='UnrealBuildTool.exe'" -ErrorAction SilentlyContinue |
    Where-Object { $_.CommandLine -match 'UnrealBuildTool' })
$debugGameEditor = @($editors | Where-Object { $_.ProcessName -like '*DebugGame*' }).Count -gt 0
$developmentEditor = @($editors | Where-Object { $_.ProcessName -notlike '*DebugGame*' }).Count -gt 0
$blocked = $null
if ($cmdRuns.Count -gt 0) { Say "Another headless run: $($cmdRuns.ProcessName -join ', ') (pid $($cmdRuns.Id -join ', '))."; $blocked = 'another test run' }
elseif ($ubt.Count -gt 0) { Say "A build is running (UnrealBuildTool pid $($ubt.ProcessId -join ', '))."; $blocked = 'another build' }
elseif ($debugGameEditor -and $developmentEditor) { Say 'A DebugGame and a Development editor are both open.'; $blocked = 'both editor configs open' }
if ($blocked -and -not $dry) { Finish 2 "BLOCKED ($blocked)" }
$config = if ($debugGameEditor) { 'Development' } else { 'DebugGame' }

$ue = & (Join-Path $PSScriptRoot 'Resolve-Engine.ps1')
if (-not $ue) { Say 'Could not resolve the Unreal Engine root (Tools\Resolve-Engine.ps1).'; Finish 2 'BLOCKED (no engine)' }
Say "Config: $config$(if ($debugGameEditor) { ' (DebugGame editor open: it keeps its old code until rebuilt and restarted)' })"
if ($dry) { Say "Build: $build"; Say "Tests: $($filters -join '+')"; Finish 0 "DRY (nothing run$(if ($blocked) { ", a real run would be BLOCKED: $blocked" }))" }

# --- Build ---
if ($build) {
    New-Item -ItemType Directory -Force $BuildOut | Out-Null
    $log = Join-Path $BuildOut $(if ($config -eq 'DebugGame') { 'Editor.log' } else { 'EditorDevelopment.log' })
    cmd /c "`"$ue\Engine\Build\BatchFiles\Build.bat`" GeoTrinityEditor Win64 $config -Project=`"$Project`" -WaitMutex > `"$log`" 2>&1"
    $buildCode = $LASTEXITCODE
    Invoke-Reader 'Read-BuildLog.ps1' @{ Log = $log; BuildExitCode = $buildCode } | Out-Null
    if ($buildCode -ne 0) {
        if (Select-String -Path $log -Pattern 'Live Coding is active' -Quiet) { Finish 2 'BLOCKED (Live Coding active: an editor opened during the run)' }
        Finish 1 'RED (build)'
    }
}

# --- Tests ---
$exe = if ($config -eq 'DebugGame') { 'UnrealEditor-Win64-DebugGame-Cmd.exe' } else { 'UnrealEditor-Cmd.exe' }
$joined = $filters -join '+'
Say "Tests: $joined"
if (Test-Path $TestOut) { Remove-Item $TestOut -Recurse -Force }
New-Item -ItemType Directory $TestOut | Out-Null
cmd /c "`"$ue\Engine\Binaries\Win64\$exe`" `"$Project`" -Unattended -NoSplash -NoPause -NullRHI -NoSound `"-ExecCmds=Automation RunTests $joined;Quit`" `"-ReportExportPath=$TestOut`" `"-abslog=$TestOut\run.log`" >nul 2>&1"
$testCode = Invoke-Reader 'Read-TestReport.ps1' @{ ReportDir = $TestOut; Filters = [string[]]$explicit }
if ($testCode -ne 0) { Finish 1 'RED (tests)' }
Finish 0 'GREEN'
