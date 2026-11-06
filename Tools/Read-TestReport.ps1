# Copyright 2024 GeoTrinity. All Rights Reserved.
<#
.SYNOPSIS
    Reads the index.json an automation run exported with -ReportExportPath, prints every failure with its
    errors (capped per test), and exits 0 only when at least one test ran and every test passed.

.DESCRIPTION
    The editor's own exit code cannot be trusted: it exits 0 when tests failed, and "succeeds" when the filter
    matched nothing. This report is the only source of truth for a run.
    -Filters: test path prefixes that must each match at least one test of the run (a new test that never
    registered fails the run instead of being silently absent).
#>
param(
    [Parameter(Mandatory)] [string] $ReportDir,
    [string[]] $Filters = @()
)

$MaxErrorsPerTest = 10
$CrashTailLines = 25

$index = Join-Path $ReportDir 'index.json'
if (-not (Test-Path $index)) {
    Write-Host "No report at $index - the editor crashed or never reached the tests. End of run.log:"
    $log = Join-Path $ReportDir 'run.log'
    if (Test-Path $log) { Get-Content $log -Tail $CrashTailLines | ForEach-Object { Write-Host $_ } }
    exit 1
}

$report = Get-Content $index -Raw -Encoding UTF8 | ConvertFrom-Json
foreach ($test in $report.tests | Where-Object { $_.state -ne 'Success' }) {
    Write-Host "[$($test.state)] $($test.fullTestPath)"
    $errors = @($test.entries | Where-Object { $_.event.type -eq 'Error' })
    foreach ($entry in $errors | Select-Object -First $MaxErrorsPerTest) {
        Write-Host "    $($entry.event.message)"
    }
    if ($errors.Count -gt $MaxErrorsPerTest) {
        Write-Host "    ... $($errors.Count - $MaxErrorsPerTest) more errors in index.json."
    }
}

$total = @($report.tests).Count
Write-Host "Tests: $total  Passed: $($report.succeeded + $report.succeededWithWarnings)  Failed: $($report.failed)  Not run: $($report.notRun)"
if ($total -eq 0) {
    Write-Host 'No test matched the filter.'
}

$unmatched = @($Filters | Where-Object { $prefix = $_; -not ($report.tests | Where-Object { $_.fullTestPath.StartsWith($prefix) }) })
foreach ($filter in $unmatched) {
    Write-Host "No test matched $filter."
}

if ($total -gt 0 -and $unmatched.Count -eq 0 -and $report.failed -eq 0 -and $report.notRun -eq 0 -and $report.inProcess -eq 0) { exit 0 }
exit 1
