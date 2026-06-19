---
name: remove-claude-branches
description: Delete all remote claude/* branches (e.g. claude/todo/*) from the git remote
---

# Remove Claude Remote Branches

Delete every remote branch under the `claude/` prefix (such as the daily `claude/todo/YYYY-MM-DD` branches) from the git remote.

## Steps

1. List remote branches and detect the remote name (this repo's remote is `GeoTrinity`, not `origin`):

   ```powershell
   git branch -r
   ```

2. Collect every branch matching `<remote>/claude/`, strip the remote prefix, and delete them in one push:

   ```powershell
   $remote = (git remote)[0]
   $branches = git branch -r |
       Where-Object { $_ -match "$remote/claude/" } |
       ForEach-Object { ($_ -replace "$remote/", '').Trim() }
   if ($branches) { git push $remote --delete $branches }
   else { Write-Host "No claude/* remote branches to delete." }
   ```

3. Report which branches were deleted (the `git push --delete` output lists each `[deleted]` ref).

## Notes

- This only touches **remote** branches. It does not delete local branches or the current working state.
- Deleting remote branches is irreversible from the remote side; if the user wants to keep history, confirm before running. The branch tips remain recoverable from any local clone that still has them until garbage-collected.
