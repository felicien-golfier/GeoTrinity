# GeoTrinity

A multiplayer 2D minimalist geometric bullet-hell game featuring the classic trinity of Tank, Heal, and DPS roles.

## Concept

Players control geometric shapes based on their class:
- **Square** - Tank
- **Circle** - Healer
- **Triangle** - DPS

Work together in co-op to defeat challenging bosses by splitting and coordinating your abilities.

## Technical Stack

- **Engine**: Unreal Engine 5.6
- **Language**: C++
- **Core Systems**: Gameplay Ability System (GAS)
- **Networking**: Multiplayer with predictive abilities and deterministic bullet patterns

## Key Features

- **Gameplay Ability System** - Full GAS implementation with custom ability components, attribute sets, and effect contexts
- **Actor Pooling** - Efficient projectile management via `UGeoActorPoolingSubsystem`
- **Bullet Pattern System** - Deterministic, time-synced patterns that work across networked clients
- **AI System** - Behavior Tree-driven enemies with ability integration

## Building

Generate project files and build through Visual Studio or command line:

```bash
# Generate project files (Windows)
"C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\GenerateProjectFiles.bat" GeoTrinity.uproject

# Build from command line
"C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\Build.bat" GeoTrinity Win64 Development
```

## Project Structure

```
Source/GeoTrinity/
├── AbilitySystem/     # GAS abilities, attributes, and components
├── Actor/             # Projectiles and turrets
├── Characters/        # Player and enemy character classes
├── AI/                # Behavior tree tasks and AI controllers
├── System/            # Actor pooling subsystem
└── Input/             # Enhanced Input configuration
```

See `CLAUDE.md` for detailed architecture documentation.
