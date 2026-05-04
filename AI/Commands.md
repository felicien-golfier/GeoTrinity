# Development Environment & Commands

## Environment
- **IDE**: JetBrains Rider (not Visual Studio)
- **Engine**: Unreal Engine 5.7
- **Platform**: Windows

## Build Commands

```bash
# Generate project files
"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\GenerateProjectFiles.bat" GeoTrinity.uproject

# Build from command line
"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" GeoTrinity Win64 Development
```

## Copyright Header
Every new source file (`.h` / `.cpp`) must start with:
```cpp
// Copyright 2024 GeoTrinity. All Rights Reserved.
```
