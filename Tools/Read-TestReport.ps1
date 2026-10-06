# Copyright 2024 GeoTrinity. All Rights Reserved.
<#
.SYNOPSIS
    Reads the index.json an automation run exported with -ReportExportPath, prints every failure with its
    errors, and exits 0 only when at least one test ran and every test passed.

.DESCRIPTION
    The editor's own exit code cannot be trusted: it exits 0 when tests failed, and "succeeds" when the filter
    matched nothing. This report is the only source of truth for a run.
#>
param(
    [Parameter(Mandatory)] [string] $ReportDir
)

$index = Join-Path $ReportDir 'index.json'
if (-not (Test-Path $index)) {
    Write-Host "No report at $index - the editor crashed or never reached the tests. See run.log beside it."
    exit 1
}

$report = Get-Content $index -Raw -Encoding UTF8 | ConvertFrom-Json
foreach ($test in $report.tests | Where-Object { $_.state -ne 'Success' }) {
    Write-Host "[$($test.state)] $($test.fullTestPath)"
    foreach ($entry in $test.entries | Where-Object { $_.event.type -eq 'Error' }) {
        Write-Host "    $($entry.event.message)"
    }
}

$total = @($report.tests).Count
Write-Host "Tests: $total  Passed: $($report.succeeded + $report.succeededWithWarnings)  Failed: $($report.failed)  Not run: $($report.notRun)"
if ($total -eq 0) {
    Write-Host 'No test matched the filter.'
}

if ($total -gt 0 -and $report.failed -eq 0 -and $report.notRun -eq 0 -and $report.inProcess -eq 0) { exit 0 }
exit 1
