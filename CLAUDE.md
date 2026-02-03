# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

GeoTrinity is a multiplayer 2D bullet-hell game built with Unreal Engine 5.7 using the Gameplay Ability System (GAS). Players control geometric shapes (Tank=Square, Heal=Circle, DPS=Triangle) in a co-op experience against bosses.

## Development Environment

- **IDE**: JetBrains Rider (not Visual Studio)
- **Engine**: Unreal Engine 5.7

## Build Commands

This is an Unreal Engine 5.7 C++ project. Build and run through the Unreal Editor or use:

```bash
# Generate project files (Windows)
"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\GenerateProjectFiles.bat" GeoTrinity.uproject

# Build from command line
"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" GeoTrinity Win64 Development
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

`AGeoEnemyAIController` with StateTree (using GameplayStateTree plugin):
- `FSTTask_FireProjectileAbility` - Activates abilities by GameplayTag via ASC
- `FSTTask_SelectNextFiringPoint` - Round-robin firing point selection, outputs TargetLocation
- `AEnemyCharacter` maintains firing points from world actors tagged "Path"

**StateTree Task Pattern**:
- Tasks are `USTRUCT` extending `FStateTreeTaskCommonBase` or `FStateTreeAIActionTaskBase`
- Instance data stored in separate `FInstanceDataType` struct
- **Must override** `GetInstanceDataType()`: `return FInstanceDataType::StaticStruct();`
- Access owner via `Context.GetOwner()` (returns AIController or Pawn depending on schema)

**StateTree Task Completion Patterns** (from `FStateTreeMoveToTask`, `FStateTreeDelayTask`):
1. **Async completion (preferred for delegate-based tasks)**:
   - In constructor: `bShouldCallTick = false;`
   - Capture weak context: `Context.MakeWeakExecutionContext()`
   - In delegate callback: `WeakContext.FinishTask(EStateTreeFinishTaskType::Succeeded)`
   - Requires include: `StateTreeAsyncExecutionContext.h`
2. **Scheduled tick (for timer-based tasks)**:
   - Use `Context.UpdateScheduledTickRequest(Handle, FStateTreeScheduledTick::MakeCustomTickRate(Time))`
   - Only ticks when needed, not every frame

**UStateTreeAIComponent** (from `Components/StateTreeAIComponent.h`):
- Inherits from `UStateTreeComponent` which inherits from `UBrainComponent`
- `bStartLogicAutomatically = true` by default - auto-starts on BeginPlay
- `SetStateTree(UStateTree*)` - sets the tree (won't work if already running)
- `StartLogic()` / `StopLogic()` / `RestartLogic()` - control execution
- `IsRunning()` / `IsPaused()` - check state
- `SendStateTreeEvent(FStateTreeEvent)` - send events to running tree
- StateTree asset configured via `FStateTreeReference StateTreeRef` property

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
├── AI/
│   ├── StateTree/         # StateTree tasks (FSTTask_*)
│   └── Tasks/             # Legacy behavior tree tasks (UBTTask_*)
├── System/                # GeoActorPoolingSubsystem, GeoPoolableInterface
└── Input/                 # GeoInputComponent, GeoInputConfig
```

## Important Notes for AI Assistance

**NO WORKAROUNDS when debugging**: When investigating issues, find the actual root cause. Don't propose workarounds or alternative approaches until the real problem is understood. The user needs to understand what's actually wrong.

**ALWAYS check existing Unreal Engine code for patterns**:
- Before implementing StateTree tasks, AI behaviors, or GAS features, find and read existing Epic implementations
- Key reference tasks: `FStateTreeDelayTask`, `FStateTreeMoveToTask` (see patterns below)
- Plugin source is in `Engine\Plugins\Runtime\<PluginName>\Source\<Module>\`

**ALWAYS verify Unreal Engine APIs before using them**:
- Read the actual engine header files in `C:\Program Files\Epic Games\UE_5.7\Engine\` before suggesting method calls
- Do NOT assume or guess method names - UE APIs change between versions
- Check parent classes for inherited methods (e.g., `UStateTreeAIComponent` → `UStateTreeComponent` → `UBrainComponent`)
- Plugin headers are in `Engine\Plugins\Runtime\<PluginName>\Source\<Module>\Public\`

## Key Files for Common Tasks

- **Adding abilities**: `AbilitySystem/Abilities/GeoGameplayAbility.h`, create UAbilityInfo data asset
- **Projectile behavior**: `Actor/Projectile/GeoProjectile.cpp`
- **Bullet patterns**: Extend `UTickablePattern`, implement `TickPattern()`
- **Character attributes**: `AbilitySystem/AttributeSet/CharacterAttributeSet.h`
- **AI behavior**: Create StateTree tasks in `AI/StateTree/`, configure StateTree asset in editor
