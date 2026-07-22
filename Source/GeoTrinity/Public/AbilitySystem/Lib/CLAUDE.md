# AbilitySystem/Lib

## `GeoAbilitySystemLibrary.h` — static helpers for GAS

**Effect application** (always use this):
- `ApplyEffectFromEffectData(EffectDataArray, SourceASC, TargetASC, Level, Seed, AbilityTag)` — two-pass: all `UpdateContextHandle` then all `ApplyEffect`. Fresh context per call = `SingleUseDamageMultiplier` auto-resets.

**Deployable spawning:**
- `StartSpawnDeployable(...)` — deferred spawn; call `InitInteractable` before `FinishSpawnDeployable`
- `FillDeployableData(...)` — populates `FDeployableData` without calling `InitInteractable` (for pre-edits like `BuffIndex`)
- `FinishSpawnDeployable(Deployable, Transform)` — `FinishSpawning`, triggers `BeginPlay`

**Projectile spawning** (pool-aware):
- `FullySpawnProjectile(...)` — request from pool, init, advance position
- `StartSpawnProjectile()`/`FinishSpawnProjectile()` — split spawn for deferred init
- `AdvanceProjectile(ElapsedSinceServerSpawnTime)` compensates for replication lag

**Targeting:**
- `GetTargetDirections(Payload, TargetMode)` — Forward/AllPlayers modes

**Team utilities — prefer over `SphereOverlapActors`/`OverlapMultiByChannel`:**
- `GetInteractableActors(World, SourceTeam, AttitudeBitmask, ...)` — agents matching `ETeamAttitudeBitflag`, with typed/distance/damageable/predicate overloads
- `GetInteractableActorsInLine(...)` — actors overlapping a 2D beam segment; used by beam abilities
- `IsTeamAttitudeAligned(Agent1, Agent2)`

**GameplayCues:**
- `ExecuteLocalGameplayCue(ASC, CueTag, CueParams)` — this-machine-only (`InvokeGameplayCueEvent(Executed)`); use for cosmetics from logic already running on every relevant machine — `ExecuteGameplayCue` would double-play on clients via listen-server multicast

**Context helpers:** `GetStatusTag`/`SetStatusTag` — only BP-exposed `FGeoGameplayEffectContext` accessors; scoped fields set via `FEffectData::UpdateContextHandle`, not BP.

**Ability CDO:** `GetAbilityCDO<T>(ASC, Tag)`, `GetGrantedAbility<T>(ASC)` (first granted T, e.g. resolving a class passive).

## `GeoGameplayTags.h` — native gameplay tag declarations
Key namespaces: `InputTag.*` (Basic/Special/SpecialAlternative/Reload/Dash), `Ability.*` (mirrors InputTag + Passive), `PlayerClass.*` (Triangle/Circle/Square), `Status.Buff.*` (DamageBoost/DamageReduction/HealBoost/Speed/Shield), `Ability.Spell.ShieldBurst`.

All declared `FNativeGameplayTag` — use these constants, never string literals.
