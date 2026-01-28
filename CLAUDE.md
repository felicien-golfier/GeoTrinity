# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

GeoTrinity is a multiplayer 2D bullet-hell game built with Unreal Engine 5.6 using the Gameplay Ability System (GAS). Players control geometric shapes (Tank=Square, Heal=Circle, DPS=Triangle) in a co-op experience against bosses.

## Build Commands

This is an Unreal Engine 5.6 C++ project. Build and run through the Unreal Editor or use:

```bash
# Generate project files (Windows)
"C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\GenerateProjectFiles.bat" GeoTrinity.uproject

# Build from command line
"C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\Build.bat" GeoTrinity Win64 Development
```

## Code Style

The project uses a custom `.clang-format` with these key settings:
- 4-space indentation with tabs
- 120 column limit
- Allman brace style (braces on new lines)
- UE macros handled: UPROPERTY, UCLASS, USTRUCT, GENERATED_BODY, UENUM

**Preferences**:
- Prefer fewer but longer if statements (merge conditions with && rather than nesting)
- Use const by default on variables; remove const only when mutation is needed
- Prefer non-const parameters over creating new variables (e.g., `FTransform SpawnTransform` instead of `const FTransform& SpawnTransform` + local copy)

## Architecture

### Character Hierarchy
```
AGeoCharacter (base, implements IAbilitySystemInterface)
├── APlayableCharacter (gets ASC from PlayerState, handles input)
└── AEnemyCharacter (creates own ASC, AI-controlled)
```

### Gameplay Ability System (GAS) Setup

**UGeoAbilitySystemComponent** - Custom ASC with:
- Input tag-based ability activation: `AbilityInputTagPressed/Held/Released()`
- Pattern management for bullet patterns
- Custom effect context (`FGeoGameplayEffectContext`) supporting crits, knockback, radial damage

**Attribute Sets**:
- `UGeoAttributeSetBase` - Health, MaxHealth, IncomingDamage (meta)
- `UCharacterAttributeSet` - Character-specific extensions

**Ability Classes**:
- `UGeoGameplayAbility` - Base class with montage support and effect data
- `UGeoProjectileAbility` - Spawns projectiles using actor pooling
- `UPatternAbility` - Creates bullet patterns via multicast RPC

### Bullet Pattern System

Patterns spawn projectiles deterministically across clients:

1. `UPatternAbility` activates and calls `PatternStartMulticast()`
2. Each client creates a `UPattern` instance
3. `UTickablePattern::TickPattern()` spawns projectiles using server time for sync
4. Example: `USpiralPattern` creates circular projectile sprays

### Actor Pooling

`UGeoActorPoolingSubsystem` (World Subsystem) manages object reuse:
- `RequestActor<T>()` - Get from pool or spawn new
- `ReleaseActor()` - Return to pool
- `PreSpawn<T>()` - Pre-allocate actors
- Actors implement `IGeoPoolableInterface` with `Init()/End()` callbacks

### Networking Model

- **Playable Characters**: ASC lives on `AGeoPlayerState` (full replication)
- **Enemy Characters**: ASC on character (minimal replication mode)

**Two projectile/ability replication approaches**:
1. **Player Abilities** - Use standard GAS replication with local prediction (normal Unreal GAS workflow)
2. **Patterns (enemy bullet patterns)** - Use multicast RPC with deterministic time-synced spawning via `FAbilityPayload` (Origin, Yaw, ServerSpawnTime, Seed)

### AI System

`AGeoEnemyAIController` with Blackboard + BehaviorTree:
- `UBTTask_FireProjectileAbility` - Activates abilities by tag from Blackboard
- `UBTTask_SelectNextFiringPoint` - Round-robin firing point selection
- `AEnemyCharacter` maintains firing points from world actors tagged "Path"

### Input System

Enhanced Input with `UGeoInputConfig` data asset:
- Maps `UInputAction` to `FGameplayTag`
- `UGeoInputComponent` binds inputs to ability callbacks

### Key Data Structures

- `FAbilityPayload` - Network sync data (Origin, Yaw, ServerSpawnTime, Seed, AbilityTag)
- `FEffectData` - Polymorphic effect system (FDamageEffectData, FStatusEffectData)
- `UAbilityInfo` - Data asset mapping ability tags to classes and metadata

## Source Structure

```
Source/GeoTrinity/
├── Public/Private AbilitySystem/
│   ├── Abilities/         # GeoGameplayAbility, PatternAbility, ProjectileAbility
│   ├── AttributeSet/      # GeoAttributeSetBase, CharacterAttributeSet
│   ├── Data/              # AbilityInfo, EffectData, AbilityPayload
│   └── GeoAbilitySystemComponent.h
├── Actor/
│   ├── Projectile/        # GeoProjectile (poolable)
│   └── Turret/            # Turret actors
├── Characters/            # GeoCharacter, PlayableCharacter, EnemyCharacter
├── AI/Tasks/              # Behavior tree tasks
├── System/                # GeoActorPoolingSubsystem, GeoPoolableInterface
└── Input/                 # GeoInputComponent, GeoInputConfig
```

## Key Files for Common Tasks

- **Adding abilities**: `AbilitySystem/Abilities/GeoGameplayAbility.h`, create UAbilityInfo data asset
- **Projectile behavior**: `Actor/Projectile/GeoProjectile.cpp`
- **Bullet patterns**: Extend `UTickablePattern`, implement `TickPattern()`
- **Character attributes**: `AbilitySystem/AttributeSet/CharacterAttributeSet.h`
- **AI behavior**: Create BT tasks in `AI/Tasks/`, configure behavior tree in editor
