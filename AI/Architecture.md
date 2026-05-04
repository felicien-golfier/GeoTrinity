# Architecture Reference

## Project Overview
GeoTrinity is a multiplayer 2D bullet-hell game built with Unreal Engine 5.7 using the Gameplay Ability System (GAS). Players control geometric shapes (Tank=Square, Heal=Circle, DPS=Triangle) in a co-op experience against bosses.

**Camera**: Orthographic, pitch = -90 (looking straight down), no angle or tilt.

---

## Networking Model

- **Playable Characters**: ASC lives on `AGeoPlayerState` (full replication)
- **Enemy Characters**: ASC on character (minimal replication mode)

### Player Ability Replication (Custom Client-Prediction)
NOT standard GAS prediction — custom system.

**Two-stage data flow:**

**Stage 1 — `ActivateAbility(TriggerEventData)`**: Payload fields (`Origin`, `Yaw`, `ServerSpawnTime`, `Seed`) are populated from `TriggerEventData` when the ability activates. The server already has this data at this point — no RPC is needed just to inform the server the ability fired.

**Stage 2 — `SendFireDataToServer` after `FireDelay` (optional)**: Some abilities send an updated snapshot after the fire delay expires, when a value has changed (e.g. current aim yaw for a projectile). This is ability-specific — not mandatory infrastructure.

When `OnFireTargetDataReceived` is used:
1. Server spawns authoritative projectile using received `Origin`, `Yaw`, `ServerSpawnTime`
2. Server projectile **not replicated to owning client** (`IsNetRelevantFor` returns false for owner) — client keeps its local predicted one
3. `ServerSpawnTime` uses synchronized server clock (`UGameplayLibrary::GetServerTime`) so projectile positions match across all machines
4. `AGeoProjectile::AdvanceProjectile()` fast-forwards projectile position by elapsed time since `ServerSpawnTime`
5. For auto/hold-to-fire: client drives shot timer, server fires once per received `FGeoAbilityTargetData` (no server-side timer)

**NEVER use `GetServerTime` for local client timing** (charge duration, cooldown UI): it's a replicated approximation. Use `GetWorld()->GetTimeSeconds()` or `FPlatformTime::Seconds()` for local timing.

### Enemy Pattern Replication
Multicast RPC with deterministic time-synced spawning via `FAbilityPayload` (Origin, Yaw, ServerSpawnTime, Seed).

---

## Key Data Structures

### `FAbilityPayload`
Internal ability data stored as `StoredPayload` on ability instances; also used by the pattern system.
- Fields: `Origin`, `Yaw`, `ServerSpawnTime`, `Seed`, `AbilityTag`, `Owner`, `Instigator`
- **Always use `StoredPayload` fields** instead of calling ability helper functions (`GetAvatarActor()`, etc.) — the payload is set by the client and may intentionally differ from ActorInfo.

### `FGeoAbilityTargetData`
Per-shot RPC payload sent client→server via `ServerSetReplicatedTargetData`.
- Fields: `Origin`, `Yaw`, `ServerSpawnTime`, `Seed`; custom `NetSerialize`

### `FEffectData` (polymorphic)
- `FDamageEffectData` — applies damage via ExecCalc
- `FHealEffectData` — applies heal via ExecCalc
- `FShieldEffectData` — applies shield
- `FGameplayEffectData` — generic GE with SetByCaller
- `FSingleUseDamageMultiplierEffectData` — sets `SingleUseDamageMultiplier` on context; scoped per `ApplyEffectFromEffectData` call
- `FStatusEffectData` — chance-based status

### `UAbilityInfo` (`AbilitySystem/Data/AbilityInfo.h`)
Data asset with per-class ability arrays. Use `GetAbilitiesForClass(EPlayerClass)` to retrieve class abilities + shared.
- `FPlayersGameplayAbilityInfo` holds `AbilityClass`, `AbilityTag`, `InputAction`, `InputTag`, `bGiveAtStartup`, `AbilityIcon`

### `FGameplayAbilityInfo`
Contains `EPlayerClass PlayerClass` for class-specific ability selection. Multiple abilities can share the same `AbilityTag` but differ by `PlayerClass`.

### `FPlayerClassData`
Per-class runtime data on `APlayableCharacter`: Mesh, AnimClass, DefaultAttributes.

### `FHudPlayerParams`
Snapshot of PC, PS, ASC, AttributeSet passed to HUD and widgets.

---

## Effect Application Rules

- `UEffectDataAsset` contains `TArray<TInstancedStruct<FEffectData>>` — ability merges its asset with inline instances before applying
- **Always use** `UGeoAbilitySystemLibrary::ApplyEffectFromEffectData()` to apply effects
- Two-pass loop: all `UpdateContextHandle` calls first, then all `ApplyEffect` — order in array doesn't matter for context setup
- **Damage multipliers**: `FSingleUseDamageMultiplierEffectData` sets `SingleUseDamageMultiplier` on context. `UExecCalc_Damage` reads it. Do NOT read it in `FDamageEffectData::ApplyEffect`.
- **Effect container choice**:
  - `TArray<TInstancedStruct<FEffectData>>` (inline on ability) — for effects specific to one ability
  - `TArray<TSoftObjectPtr<UEffectDataAsset>>` (asset reference) — for shared/reused effects

---

## Actor Pooling (`UGeoActorPoolingSubsystem`)
World Subsystem managing object reuse:
- `RequestActor<T>()` — get from pool or spawn new
- `ReleaseActor()` — return to pool
- `PreSpawn<T>()` — pre-allocate actors
- Actors implement `IGeoPoolableInterface` with `Init()/End()` callbacks

---

## Class-Specific Ability Selection
- Filter abilities by `EPlayerClass` when activating by input tag
- Do NOT create separate ability classes for different player classes — use the `PlayerClass` filter on `FGameplayAbilityInfo`

## Class Inventories
- **Circle (Healer)**: `GeoHealingAuraAbility`, `GeoMoiraBeamAbility`, `GeoHealReturnPassiveAbility`, `GeoChargeBeamAbility`
- **Square (Tank)**: `GeoMineAbility`, `GeoShieldBurstPassiveAbility`, `GeoDetonateAllMinesAbility`
- **Triangle (DPS)**: `GeoReloadAbility`, `GeoRecallTurretAbility`; basic attack = `UGeoAutomaticProjectileAbility` with ammo cost
- **Common**: `GeoDeployAbility`, `GeoDashAbility`
