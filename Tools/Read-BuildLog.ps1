# Copyright 2024 GeoTrinity. All Rights Reserved.
<#
.SYNOPSIS
    Prints what matters of a build log: unique errors, then unique warnings, then the UnrealBuildTool result line.

.DESCRIPTION
    A failed build with no recognizable error line prints the end of the log instead, so a failure is never silent.
    Exit code stays with the caller: this script only reads.
#>
param(
    [Parameter(Mandatory)] [string] $Log,
    [Parameter(Mandatory)] [int] $BuildExitCode
)

$MaxErrors = 30
$MaxWarnings = 10
$FailureTailLines = 30

function Write-Capped([string[]] $Lines, [int] $Max, [string] $Kind) {
    $Lines | Select-Object -First $Max | ForEach-Object { Write-Host $_ }
    if ($Lines.Count -gt $Max) {
        Write-Host "... $($Lines.Count - $Max) more $Kind in the log."
    }
}

if (-not (Test-Path $Log)) {
    Write-Host "No build log at $Log."
    exit 0
}

$lines = @(Get-Content $Log | Where-Object { $_ -notmatch '^\[\d+/\d+\]' })
$errors = @($lines | Where-Object { $_ -match '(?i)\berror\b\s*(:|[A-Z]+\d+)' } | Select-Object -Unique)
$warnings = @($lines | Where-Object { $_ -match '(?i)\bwarning\b\s*(:|[A-Z]+\d+)' } | Select-Object -Unique)

Write-Capped $errors $MaxErrors 'errors'
Write-Capped $warnings $MaxWarnings 'warnings'
if ($BuildExitCode -ne 0 -and $errors.Count -eq 0) {
    Write-Host "No error line recognized; end of the log:"
    $lines | Select-Object -Last $FailureTailLines | ForEach-Object { Write-Host $_ }
}

$lines | Where-Object { $_ -match '^(Result:|Total execution time)' } | ForEach-Object { Write-Host $_ }
Write-Host "Errors: $($errors.Count)  Warnings: $($warnings.Count)  Full log: $Log"
