# GeoTrinity

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

A multiplayer 2D minimalist geometric bullet-hell game featuring the classic trinity of Tank, Heal, and DPS roles.

## Concept

Players control geometric shapes based on their class:
- **Square** - Tank
- **Circle** - Healer
- **Triangle** - DPS

Work together in co-op to defeat challenging bosses by splitting and coordinating your abilities. Players can switch class at runtime, swapping mesh, animations, and ability sets on the fly.

## Technical Stack

- **Engine**: Unreal Engine 5.7
- **Language**: C++
- **IDE**: JetBrains Rider
- **Core Systems**: Gameplay Ability System (GAS)
- **Networking**: Multiplayer with custom client-prediction and deterministic bullet patterns

## Key Features

- **Gameplay Ability System** - Full GAS implementation with custom ASC, attribute sets, effect contexts, and polymorphic effect data (`FEffectData` subclasses)
- **Class-Specific Abilities** - Each shape has a unique ability kit; abilities sharing the same input tag are filtered by `EPlayerClass` at activation
- **Custom Client Prediction** - Client fires immediately and sends `FGeoAbilityTargetData` to the server; server projectile is hidden from the owning client to avoid duplication
- **Actor Pooling** - Efficient projectile and actor reuse via `UGeoActorPoolingSubsystem`
- **Bullet Pattern System** - Deterministic, server-time-synced patterns (`UTickablePattern`) that reproduce identically across all clients regardless of ping
- **AI System** - StateTree-driven enemies (`AGeoEnemyAIController`) with round-robin firing point selection and ability activation via gameplay tags
- **Deployable System** - Hold-to-charge deploy ability; deployables (mines, healing zones, buff pickups) have lifespan drain, blink warning, and owner-initiated recall
- **Camera System** - Orthographic top-down camera (`AGeoGameCamera`) with edge-triggered follow, separate X/Y speed curves, and world-space boundary clamping
- **Combat Stats** - Rolling 10-second DPS/HPS tracking per player via `UGeoCombatStatsSubsystem`

## Building

Generate project files and build through JetBrains Rider or the command line:

```bash
# Generate project files (Windows)
"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\GenerateProjectFiles.bat" GeoTrinity.uproject

# Build from command line
"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" GeoTrinity Win64 Development
```

## Project Structure

```
Source/GeoTrinity/
├── AbilitySystem/
│   ├── Abilities/         # GeoGameplayAbility, ProjectileAbility, AutomaticFireAbility
│   │   ├── Circle/        # Healing aura, Moira beam, charge beam, heal-return passive
│   │   ├── Square/        # Mine, shield burst passive, detonate all mines
│   │   ├── Triangle/      # Reload, recall turret
│   │   └── Pattern/       # UPattern, UTickablePattern, USpiralPattern
│   ├── AttributeSet/      # GeoAttributeSetBase, CharacterAttributeSet
│   ├── Data/              # AbilityInfo, EffectData, AbilityPayload
│   ├── ExecCalc/          # Damage and heal execution calculations
│   └── Lib/               # GeoAbilitySystemLibrary, gameplay tags
├── Actor/
│   ├── Projectile/        # GeoProjectile (poolable), pooled variants
│   └── Deployable/        # GeoDeployableBase, Mine, HealingZone, BuffPickup
├── Characters/
│   ├── Component/         # DeployableManager, GameFeel (VFX/audio), ShieldBurstPassive
│   ├── GeoCharacter.h     # Base character (ASC + input + movement + health bar)
│   ├── PlayableCharacter.h
│   └── EnemyCharacter.h
├── AI/
│   ├── StateTree/         # FSTTask_FireProjectileAbility, FSTTask_SelectNextFiringPoint
│   └── Tasks/             # Legacy BT tasks
├── HUD/                   # GeoHUD, GeoUserWidget, GenericCombattantWidget
├── System/                # ActorPoolingSubsystem, GeoCombatStatsSubsystem
├── World/                 # GeoGameCamera, GeoWorldSettings
└── Input/                 # GeoInputComponent, GeoInputConfig
```

See `CLAUDE.md` for detailed architecture documentation.

## License

This project is licensed under the **GNU General Public License v3.0**.

You are free to use, study, and modify this code. If you distribute it or build upon it, you must release your work under the same GPL v3 license with source code available. Commercial use is permitted only under these same terms — you cannot use this code in a closed-source product.

See the [LICENSE](LICENSE) file for the full license text.
