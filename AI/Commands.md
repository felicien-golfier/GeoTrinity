# Development Environment & Commands

## Environment
- **IDE**: JetBrains Rider (not Visual Studio)
- **Engine**: Unreal Engine 5.7 — **full source build** at `C:\UnrealEngine` (NOT the launcher install under `C:\Program Files\Epic Games`)
- **Platform**: Windows
- **Editor target/config**: `GeoTrinityEditor` in `DebugGame` — this is what Rider launches and what you must build. NEVER build the plain `GeoTrinity` game target or `Development` editor when the goal is the editor.

## Build Commands

Always use the **source** engine's BatchFiles (`C:\UnrealEngine\Engine\...`), never the launcher path.

```bash
# Generate project files
"C:\UnrealEngine\Engine\Build\BatchFiles\GenerateProjectFiles.bat" "C:\GeoTrinity\GeoTrinity.uproject"

# Build the editor (matches Rider's launch target/config)
"C:\UnrealEngine\Engine\Build\BatchFiles\Build.bat" GeoTrinityEditor Win64 DebugGame -Project="C:\GeoTrinity\GeoTrinity.uproject" -WaitMutex
```

Other targets (build these ONLY when explicitly needed, not for the normal editor workflow):
```bash
# Dedicated server
"C:\UnrealEngine\Engine\Build\BatchFiles\Build.bat" GeoTrinityServer Win64 Development -Project="C:\GeoTrinity\GeoTrinity.uproject" -WaitMutex
# Standalone game (packaged-style), not the editor
"C:\UnrealEngine\Engine\Build\BatchFiles\Build.bat" GeoTrinity Win64 Development -Project="C:\GeoTrinity\GeoTrinity.uproject" -WaitMutex
```

## Packaging

To package a distributable build of the game, see [`BuildPackage.md`](BuildPackage.md).

## Launch Editor

```bash
# Launch the DebugGame editor (source engine) with MCP remote control access
"C:\UnrealEngine\Engine\Binaries\Win64\UnrealEditor-Win64-DebugGame.exe" "C:\GeoTrinity\GeoTrinity.uproject" -RemoteControlAllow
```

## Copyright Header
Every new source file (`.h` / `.cpp`) must start with:
```cpp
// Copyright 2024 GeoTrinity. All Rights Reserved.
```
