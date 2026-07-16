# Development Environment & Commands

## Environment
- **IDE**: JetBrains Rider (not Visual Studio)
- **Engine**: Unreal Engine 5.7. **Never hardcode an engine path** — it differs per machine. This PC has the
  **launcher (installed) build** at `C:\Program Files\Epic Games\UE_5.7`; the CI box has a **source build** at
  `H:\Epic\UE_5.7`. Both are found via the `.uproject`'s `"EngineAssociation": "5.7"`.
- **Platform**: Windows
- **Editor target/config**: `GeoTrinityEditor` in `DebugGame` — this is what Rider launches and what you must build. NEVER build the plain `GeoTrinity` game target or `Development` editor when the goal is the editor.

## Build Commands

**Use the `Tools\*.bat` wrappers.** They resolve the engine per-machine, so the same command works here and
on the CI box with no edits:

```bash
Tools\Build_Editor.bat       # GeoTrinityEditor Win64 DebugGame — the normal editor workflow
Tools\Build_Standalone.bat   # GeoTrinity Win64 Development — client + listen-server host
Tools\Build_Package.bat      # cook + stage + pak + archive — see BuildPackage.md
Tools\Build_Server.bat       # GeoTrinityServer — source-engine machines only, see below
```

To resolve the engine yourself (the path has spaces — **keep it quoted**):

```powershell
$ue = & Tools\Resolve-Engine.ps1                    # add -Verbose to see where it came from
& "$ue\Engine\Build\BatchFiles\Build.bat" GeoTrinityEditor Win64 DebugGame -Project="C:\GeoTrinity\GeoTrinity.uproject" -WaitMutex
```

### How the engine is found
`EngineAssociation` is a registry **lookup key, not a path** — which is why one committed `.uproject` can mean
different engines on different PCs. `Build.bat` / `RunUAT.bat` ignore it entirely and use whichever engine's
BatchFiles you invoke; `Tools\Resolve-Engine.ps1` bridges the two. **Nothing needs registering by hand** —
Unreal registers engines itself. But the two kinds are registered very differently:

| Kind | Where | Keyed by |
|---|---|---|
| Launcher install | `HKLM\SOFTWARE\EpicGames\Unreal Engine\<ver>` (**no** space in `EpicGames`) | version — `"5.7"` matches directly |
| Source build | `HKCU\SOFTWARE\Epic Games\Unreal Engine\Builds` (**space** in `Epic Games`) | a **random GUID**, unique per machine |

A source build's GUID can never be named by a shared `.uproject`, so the resolver identifies it by reading its
`Engine\Build\Build.version` and matching `MajorVersion.MinorVersion` against the association. Order:
`-Override` → `$env:UE` → HKCU by name → HKCU by version → HKLM by name. Each candidate is validated by
checking `Build.bat` exists (the registry keeps advertising deleted engines — this PC still lists 5.5 and 5.6).

Source builds win over launcher installs, which differs from Unreal's own resolution; see the script's header
for why, and pass `-Override` to force the other one.

> **Never add a registry alias named `5.7` pointing at a source build.** `EnumerateEngineInstallations`
> dedupes by directory and `RegDeleteValue`s every duplicate, so of two names for one engine, whichever
> enumerates second is silently deleted.

One machine needs setup: the **CI runner**, because its service runs as SYSTEM and cannot read the desktop
user's `HKCU` where source builds register. Run `Tools\Setup-Runner.ps1` there once, elevated — see
[`CI-RUNNER.md`](../CI-RUNNER.md).

### Generate project files
Installed builds ship no `GenerateProjectFiles.bat`; use UnrealVersionSelector (what the `.uproject` right-click
"Generate Visual Studio project files" runs):
```bash
"C:\Program Files (x86)\Epic Games\Launcher\Engine\Binaries\Win64\UnrealVersionSelector.exe" /projectfiles "C:\GeoTrinity\GeoTrinity.uproject"
```

### Dedicated server needs a source engine
`GeoTrinityServer` links against engine libs that launcher installs do not ship (`Engine\Intermediate\Build\Win64`
has `UnrealEditor` and `UnrealGame` only — no `UnrealServer`), and no launcher option adds them. So it **cannot be
built on this PC**, only on a source-engine machine. `Tools\Build_Server.bat` detects this and says so.

Build it on CI: run the **Build GeoTrinity (Custom)** workflow with `build_server: true`.

## Packaging

To package a distributable build of the game, see [`BuildPackage.md`](BuildPackage.md).

## Launch Editor

```bash
# Launch the DebugGame editor (launcher engine) with MCP remote control access
"C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Win64-DebugGame.exe" "C:\GeoTrinity\GeoTrinity.uproject" -RemoteControlAllow
```

## Copyright Header
Every new source file (`.h` / `.cpp`) must start with:
```cpp
// Copyright 2024 GeoTrinity. All Rights Reserved.
```
