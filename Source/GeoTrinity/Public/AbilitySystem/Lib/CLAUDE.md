# AbilitySystem/Lib

---

## `GeoAbilitySystemLibrary.h` — static helpers for GAS

**Effect application** (always use this):
- `ApplyEffectFromEffectData(EffectDataArray, SourceASC, TargetASC, Level, Seed, AbilityTag)` — two-pass: all `UpdateContextHandle` first, then all `ApplyEffect`. Fresh context per call = `SingleUseDamageMultiplier` auto-resets. `AbilityTag` is forwarded through `UpdateContextHandle` so subtypes like `FDamageEffectData` can read the originating ability's CDO (e.g. to flag `bIsFromBasicAbility`).

**Deployable spawning:**
- `StartSpawnDeployable(Class, Owner, Instigator, Transform)` — deferred spawn; returns actor before `BeginPlay`. Call `InitInteractable` before `FinishSpawnDeployable`.
- `FillDeployableData(Data, Payload, EffectDataArray, Params)` — populates an `FDeployableData` struct without calling `InitInteractable`. Use when you need to modify fields (e.g. `BuffIndex`) before passing the struct to `InitInteractable` manually.
- `FinishSpawnDeployable(Deployable, Transform)` — calls `FinishSpawning`, triggering `BeginPlay`

**Projectile spawning** (pool-aware):
- `FullySpawnProjectile(Class, Transform, Payload, EffectData, ...)` — request from pool, init, advance position
- `StartSpawnProjectile()` / `FinishSpawnProjectile()` — split spawn for deferred initialization
- Calls `AdvanceProjectile(ElapsedSinceServerSpawnTime)` after spawning to compensate for replication lag

**Targeting:**
- `GetTargetDirections(Payload, TargetMode)` — returns directions for Forward / AllPlayers modes

**Team utilities — prefer these over `SphereOverlapActors` / `OverlapMultiByChannel`:**
- `GetInteractableActors(World, SourceTeam, AttitudeBitmask)` — all agents matching any bit in `ETeamAttitudeBitflag` bitmask
- `GetInteractableActors<T>(World, SourceTeam, AttitudeBitmask, bMustBeDamageable, Location, MaxDistance)` — typed variant; returns only agents that `IsA<T>`, cast to `T*`. Wraps the ExtraFilter overload; avoids casting at call sites
- `GetInteractableActors(World, SourceTeam, AttitudeBitmask, bMustBeDamageable, Location, MaxDistance)` — with optional distance + damageable filter
- `GetInteractableActors(World, SourceTeam, AttitudeBitmask, bMustBeDamageable, Location, MaxDistance, ExtraFilter)` — same but with an additional per-actor predicate (`TFunctionRef<bool(AActor*)>`)
- `GetInteractableActors(World, bMustBeDamageable, Location, MaxDistance)` — no team filter, distance only
- `GetInteractableActorsInLine(World, SourceTeam, AttitudeBitmask, bMustBeDamageable, Origin, ForwardVector, MaxRange, LineHalfWidth)` — actors whose collision circle overlaps the 2D beam segment; used by beam abilities instead of a capsule overlap
- Use these instead of any physics overlap when filtering game agents.
- `IsTeamAttitudeAligned(Agent1, Agent2)` — checks alignment

**Context helpers:**
- `GetStatusTag` / `SetStatusTag` — the only Blueprint-exposed `FGeoGameplayEffectContext` field accessors. The call-site scoped fields are set through `FEffectData::UpdateContextHandle`, not from Blueprints

**Ability CDO:**
- `GetAbilityCDO<T>(ASC, Tag)` — get ability CDO by gameplay tag (for reading config values)

---

## `GeoGameplayTags.h` — native gameplay tag declarations

Key tag namespaces:
- `InputTag.*` — `Basic`, `Special`, `SpecialAlternative`, `Reload`, `Dash`
- `Ability.*` — `Basic`, `Special`, `SpecialAlternative`, `Dash`, `Reload`, `Passive`
- `PlayerClass.*` — `Triangle`, `Circle`, `Square`
- `Status.Buff.*` — `DamageBoost`, `DamageReduction`, `HealBoost`, `Speed`, `Shield`
- `Ability.Spell.ShieldBurst`

All tags declared as `FNativeGameplayTag` — use these constants instead of string literals everywhere.
