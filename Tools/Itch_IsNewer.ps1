# Exit 0 when the zip is newer than the channel's last butler build (or the channel is empty), 1 when up to date.
# Usage: Itch_IsNewer.ps1 <zip> <target e.g. exyoe/geotrinity> <channel>
param([string]$Zip, [string]$Target, [string]$Channel)
$line = butler status $Target --json | Where-Object { $_ -match '"type":"result"' } | Select-Object -Last 1
if (-not $line) { Write-Host "Could not read the itch status; pushing."; exit 0 }
$ch = ($line | ConvertFrom-Json).value.channels | Where-Object { $_.name -eq $Channel }
if (-not $ch -or -not $ch.head) { Write-Host "No '$Channel' build on itch yet; pushing."; exit 0 }
$remote = [DateTimeOffset]::Parse($ch.head.createdAt).ToUnixTimeSeconds()
$local = [DateTimeOffset](Get-Item $Zip).LastWriteTimeUtc
$localSec = $local.ToUnixTimeSeconds()
if ($localSec -gt $remote) { Write-Host "'$Channel': local zip is newer than itch ($($ch.head.createdAt)); pushing."; exit 0 }
Write-Host "'$Channel': itch already has this build or newer ($($ch.head.createdAt)); skipping."
exit 1
