# Development Environment & Commands

## Environment
- **IDE**: JetBrains Rider. **Platform**: Windows. **Engine**: Unreal Engine 5.7.
- **Editor target/config**: `GeoTrinityEditor` `DebugGame` — what Rider launches and what to build. Never the
  plain `GeoTrinity` target or `Development` editor when the goal is the editor.
- **Never hardcode an engine path** — this PC has the launcher install at `C:\Program Files\Epic Games\UE_5.7`,
  the CI box a source build at `H:\Epic\UE_5.7`. Both resolve from the `.uproject`'s `"EngineAssociation": "5.7"`.
- Every new `.h`/`.cpp` starts with `// Copyright 2024 GeoTrinity. All Rights Reserved.`

## Build commands
The `Tools\*.bat` wrappers resolve the engine per machine, so one command works here and on CI:

```bash
Tools\Build_Editor.bat       # GeoTrinityEditor Win64 DebugGame — the normal editor workflow
Tools\Build_Standalone.bat   # GeoTrinity Win64 Development — client + listen-server host
Tools\Build_Package.bat      # cook + stage + pak + archive — see BuildPackage.md
Tools\Build_Server.bat       # GeoTrinityServer — source-engine machines only
```

To resolve the engine by hand (the path has spaces — **keep it quoted**):

```powershell
$ue = & Tools\Resolve-Engine.ps1                    # -Verbose to see where it came from
& "$ue\Engine\Build\BatchFiles\Build.bat" GeoTrinityEditor Win64 DebugGame -Project="C:\GeoTrinity\GeoTrinity.uproject" -WaitMutex
```

### How the engine is found
`EngineAssociation` is a registry **lookup key, not a path**, which is why one committed `.uproject` means
different engines on different PCs. `Build.bat` / `RunUAT.bat` ignore it entirely and use whichever engine's
BatchFiles are invoked; `Tools\Resolve-Engine.ps1` bridges the two. Unreal registers engines itself — nothing
needs registering by hand — but the two kinds register differently:

| Kind | Where | Keyed by |
|---|---|---|
| Launcher install | `HKLM\SOFTWARE\EpicGames\Unreal Engine\<ver>` (**no** space in `EpicGames`) | version — `"5.7"` matches directly |
| Source build | `HKCU\SOFTWARE\Epic Games\Unreal Engine\Builds` (**space** in `Epic Games`) | a random GUID, unique per machine |

A shared `.uproject` can never name a source build's GUID, so the resolver identifies one by matching
`MajorVersion.MinorVersion` in its `Engine\Build\Build.version` against the association. Order:
`-Override` → `$env:UE` → HKCU by name → HKCU by version → HKLM by name, each validated by checking `Build.bat`
exists (the registry keeps advertising deleted engines). Source builds win over launcher installs, unlike
Unreal's own resolution; pass `-Override` to force the other one.

> **Never add a registry alias named `5.7` pointing at a source build.** `EnumerateEngineInstallations` dedupes
> by directory and `RegDeleteValue`s every duplicate, so of two names for one engine the second is deleted.

The **CI runner** is the one machine needing setup: its service runs as SYSTEM and cannot read the desktop
user's `HKCU`. Run `Tools\Setup-Runner.ps1` there once, elevated — see [`CI-RUNNER.md`](../CI-RUNNER.md).

### Dedicated server needs a source engine
`GeoTrinityServer` links against engine libs launcher installs do not ship (`Engine\Intermediate\Build\Win64`
carries `UnrealEditor` and `UnrealGame` only), and no launcher option adds them, so it cannot be built on this
PC — `Tools\Build_Server.bat` detects this and says so. Build it on CI: run the **Build GeoTrinity (Custom)**
workflow with `build_server: true`.

### Generate project files
Installed builds ship no `GenerateProjectFiles.bat`; use UnrealVersionSelector, what the `.uproject`
right-click entry runs:
```bash
"C:\Program Files (x86)\Epic Games\Launcher\Engine\Binaries\Win64\UnrealVersionSelector.exe" /projectfiles "C:\GeoTrinity\GeoTrinity.uproject"
```

## Launch editor
```bash
"C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Win64-DebugGame.exe" "C:\GeoTrinity\GeoTrinity.uproject" -RemoteControlAllow
```

## Packaging
See [`BuildPackage.md`](BuildPackage.md).
