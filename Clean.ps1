$repo = $PSScriptRoot
Write-Host "Cleaning $repo..."
Remove-Item -Recurse -Force "$repo\Binaries"     -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force "$repo\Intermediate" -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force "$repo\Saved\Cooked" -ErrorAction SilentlyContinue
Get-ChildItem "$repo\Plugins" -Recurse -Directory -Filter "Binaries"     | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
Get-ChildItem "$repo\Plugins" -Recurse -Directory -Filter "Intermediate" | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
Write-Host "Clean done."
