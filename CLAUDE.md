# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

GeoTrinity is a multiplayer 2D bullet-hell game built with Unreal Engine 5.7 using the Gameplay Ability System (GAS). Players control geometric shapes (Tank=Square, Heal=Circle, DPS=Triangle) in a co-op experience against bosses.

**Camera**: Orthographic, pitch = -90 (looking straight down), no angle or tilt.

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
- Prefer forward declarations in headers over includes (for enums: `enum class EMyEnum : uint8;`)
- Remove trivial wrapper functions that just delegate to another function - call directly instead
- Code only what's needed (YAGNI) - don't add unused parameters, variables, or speculative features
- Be consistent in general ! Do not use a code style once and then another one. For naming also. 
- Be consistent with Super call placement: choose what makes sense (e.g., Super::Init at start, Super::Destroy at end), but when there's no meaningful ordering dependency, keep it consistent (e.g., always at the top) rather than mixing positions arbitrarily
- Dont use abreviations in variable name, ALWAYS use full class name except some very verbose names like ASC for AbilitySystemComponent.
- Prefer readable, self-documenting code over comments. Don't add comments that restate what the code already says - the code should speak for itself.
- **No silent fallbacks**: Never silently skip or substitute when something required is missing. Use `ensureMsgf(condition, ...)` to flag the problem. If execution must continue gracefully despite the failure, follow it with an `if` — but always surface the error. Never use `condition ? A : B` as a quiet fallback (e.g. `Pool ? Pool->Get() : SpawnActor()`).
  - Configured assets (ProjectileClass, EffectData, subsystems) must be checked with `ensureMsgf` — missing them is always a bug.
  - Runtime misses (no target found, empty list) are legitimate — a plain `if` with no assert is fine there.

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
- `UGeoGameplayAbility` - Base class: handles `StoredPayload`, `ScheduleFireTrigger`, `SendFireDataToServer`, `OnFireTargetDataReceived`
- `UGeoProjectileAbility` - Single-shot projectile: client fires in `Fire()`, server fires in `OnFireTargetDataReceived()`
- `UGeoAutomaticFireAbility` - Hold-to-fire loop: client drives timing, server mirrors each shot via `OnFireTargetDataReceived`. Subclasses override `ExecuteShot()` (Abstract)
- `UGeoAutomaticProjectileAbility` - Concrete auto-fire that spawns projectiles, extends `UGeoAutomaticFireAbility`
- `UPatternAbility` - Creates bullet patterns via multicast RPC (enemy-only, server-driven)

**Class-Specific Ability Selection**:
- `FGameplayAbilityInfo` has `EPlayerClass PlayerClass` field
- Multiple abilities can share the same `AbilityTag` (e.g., `Ability.Basic`) but differ by `PlayerClass`
- When activating by input tag, filter by player's `EPlayerClass` to select correct ability
- Example: Triangle's BasicAttack (ammo-based auto-fire) vs Circle's BasicAttack (standard projectile)
- Do NOT create separate ability classes just for different player classes - use the `PlayerClass` filter instead

**Gameplay Effect**:
- `FEffectData` - Effect Data to store all Effects needed. Create subclass for new effect.
- `UEffectDataAsset` contains `TArray<TInstancedStruct<struct FEffectData>> EffectDataInstances` to store Data Assets on the Ability BP
- Ability always merge its UEffectDataAsset with `TArray<TInstancedStruct<FEffectData>> EffectDataInstances` to pass throught.
- ALWAYS use UGeoAbilitySystemLibrary::ApplyEffectFromEffectData to apply a EffectDataArray.

### Bullet Pattern System

/!\ Onlydevelopped for server abilities for now ! Only enemies use those.
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

1. **Player Abilities** - Custom client-prediction system (NOT standard GAS replication):
   - Client fires immediately (predicted), sends `FGeoAbilityTargetData` to server via `ServerSetReplicatedTargetData`
   - Server receives data in `OnFireTargetDataReceived` and spawns its authoritative projectile using client's `Origin`, `Yaw`, `ServerSpawnTime`
   - Server projectile is **not replicated to the owning client** (`IsNetRelevantFor` returns false for owner) — client keeps its local predicted one
   - `ServerSpawnTime` uses synchronized server clock (`UGameplayLibrary::GetServerTime`) so projectile positions match across all machines despite ping
   - `AGeoProjectile::AdvanceProjectile()` can fast-forward a projectile's position by elapsed time since `ServerSpawnTime`
   - For automatic/hold-to-fire: client drives the shot timer, server fires once per received `FGeoAbilityTargetData` packet (no server-side timer)
   - **NEVER use `GetServerTime` for local client timing** (e.g. measuring charge duration, cooldown UI): it is a replicated approximation of server time and is not accurate for local delta-time. Use `GetWorld()->GetTimeSeconds()` or `FPlatformTime::Seconds()` for local timing on the client.

2. **Patterns (enemy bullet patterns)** - Use multicast RPC with deterministic time-synced spawning via `FAbilityPayload` (Origin, Yaw, ServerSpawnTime, Seed)

**To add a new single-shot ability**: Extend `UGeoProjectileAbility` (or override `Fire`/`OnFireTargetDataReceived` for custom behaviour).
**To add a new hold-to-fire ability**: Extend `UGeoAutomaticFireAbility`, override `ExecuteShot()`. Use `StoredPayload` for spawn data.

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

- `FAbilityPayload` - Internal ability data (Origin, Yaw, ServerSpawnTime, Seed, AbilityTag, Owner, Instigator) — stored as `StoredPayload` on ability instances; also used by pattern system
- `FGeoAbilityTargetData` - Per-shot RPC payload sent client→server via `ServerSetReplicatedTargetData` (Origin, Yaw, ServerSpawnTime, Seed); custom `NetSerialize`
- `FEffectData` - Polymorphic effect system (FDamageEffectData, FStatusEffectData)
- `UAbilityInfo` - Data asset mapping ability tags to classes and metadata
- `FGameplayAbilityInfo` - Contains `EPlayerClass PlayerClass` field for class-specific ability selection

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
