---
name: build
description: Build GeoTrinity — packages a distributable build by default (BuildPackage.md); use explicit targets for editor/server/standalone builds (AI/Commands.md)
---

# Build GeoTrinity

Run the bat file for the requested target. Do not re-derive Build.bat/RunUAT commands from the docs — the bat files already encode them exactly.

## Steps

1. **Pick the target** from `$ARGUMENTS`. No argument → `package`.
   - `package` / `listen` / `listen-server` → `Tools\Build_Package.bat`
   - `editor` → `Tools\Build_Editor.bat`
   - `server` → `Tools\Build_Server.bat`
   - `game` / `standalone` → `Tools\Build_Standalone.bat`

2. **Editor-safety check (BIG RULE).** NEVER close, kill, or restart the user's Unreal editor. All targets, including packaging, can run with the editor open.

3. **Run the bat file** from repo root, in the background (these are long builds), and report progress / final result.

4. **After a successful `package` build**, rename the output folder to match the CI convention from `.github/workflows/build-custom.yml`:
   - Format: `GeoTrinity_{branch}_{config}_{timestamp}` (e.g. `GeoTrinity_master_Development_20260702_153000`)
   - Get the current branch with `git rev-parse --abbrev-ref HEAD`, replace `/` and `\` with `-`
   - Config is `Development` unless the user specified otherwise
   - Timestamp: `yyyyMMdd_HHmmss` at build completion time
   - Move the contents of `Build\Windows\` up into a new `Build\{renamed folder}\`, then remove the empty `Windows\` subfolder — matching exactly what the CI does.

5. **Report** the outcome faithfully — success with the final folder path, or the actual error output if it fails.

## Notes

- Only fall back to reading `BuildPackage.md` or `AI/Commands.md` if a bat file is missing, the build fails in a way that looks like a stale command (wrong engine path, wrong target name), or the user asks about the build process itself.
