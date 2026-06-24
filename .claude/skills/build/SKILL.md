---
name: build
description: Build GeoTrinity — launches the appropriate build (editor by default from AI/Commands.md, or a packaged distributable from BuildPackage.md when asked to package)
---

# Build GeoTrinity

Build the game following the project's build docs. There are two distinct doc sources — pick the right one:

- **Editor / dev / server / standalone builds** → [`AI/Commands.md`](../../../AI/Commands.md)
- **Packaging a distributable build** (cook+stage+pak+archive) → [`BuildPackage.md`](../../../BuildPackage.md)

## Steps

1. **Read the right doc first** — it is the single source of truth for every command, path, and rule. Do not build from memory; engine path, target names, and configs may have changed.
   - If the request is to *package* / make a distributable / listen-server client → read `BuildPackage.md`.
   - Otherwise → read `AI/Commands.md`.

2. **Pick the target** from the user's request (`$ARGUMENTS`). If none was given, default to the **editor** target — that is the normal workflow.
   - `editor` (default) → `GeoTrinityEditor Win64 DebugGame` via `Build.bat` (`AI/Commands.md`)
   - `server` → `GeoTrinityServer Win64 Development` via `Build.bat` (`AI/Commands.md`)
   - `game` / `standalone` → `GeoTrinity Win64 Development` via `Build.bat` (`AI/Commands.md`)
   - `package` / `listen` / `listen-server` → packaged client via `RunUAT.bat BuildCookRun`, archived to `C:\GeoTrinity\Build` (`BuildPackage.md`)

3. **Editor-safety check (BIG RULE).** NEVER close, kill, or restart the user's Unreal editor.
   - Compile/`Build.bat` targets can run while the editor is open (it uses `-WaitMutex`).
   - **Packaging (`BuildCookRun`) requires the editor closed** (DLL link lock). Before launching a package build, check for a running editor:
     ```powershell
     Get-Process UnrealEditor* -ErrorAction SilentlyContinue | Select-Object Id, ProcessName
     ```
     If one is running, **ask the user to close it and wait** — do not kill it yourself.

4. **Launch the build** using the exact command from the doc for the chosen target. Run long builds (especially `BuildCookRun`) in the background and report progress / final result. Packaged output must always archive to `C:\GeoTrinity\Build`.

5. **Report** the outcome faithfully — success, or the actual error output if it fails.

## Notes

- Always use the **source** engine BatchFiles under `C:\UnrealEngine\Engine\...`, never the launcher install.
- When in doubt about target vs. config, re-read the relevant doc rather than guessing.
