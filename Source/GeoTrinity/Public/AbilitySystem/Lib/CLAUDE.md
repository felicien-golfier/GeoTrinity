# AbilitySystem/Lib

---

## `GeoAbilitySystemLibrary.h` — static helpers for GAS

**Effect application** (always use this):
- `ApplyEffectFromEffectData(EffectDataArray, SourceASC, TargetASC)` — two-pass: all `UpdateContextHandle` first, then all `ApplyEffect`. Fresh context per call = `SingleUseDamageMultiplier` auto-resets.

**Projectile spawning** (pool-aware):
- `FullySpawnProjectile(Class, Transform, Payload, EffectData, ...)` — request from pool, init, advance position
- `StartSpawnProjectile()` / `FinishSpawnProjectile()` — split spawn for deferred initialization
- Calls `AdvanceProjectile(ElapsedSinceServerSpawnTime)` after spawning to compensate for replication lag

**Targeting:**
- `GetTargetDirections(Payload, TargetMode)` — returns directions for Forward / AllPlayers modes

**Team utilities — prefer these over `SphereOverlapActors` / `OverlapMultiByChannel`:**
- `GetInteractableActors(World, SourceTeam, AttitudeBitmask)` — all agents matching any bit in `ETeamAttitudeBitflag` bitmask
- `GetInteractableActors(World, SourceTeam, AttitudeBitmask, bMustBeDamageable, Location, MaxDistance)` — with optional distance + damageable filter
- `GetInteractableActors(World, bMustBeDamageable, Location, MaxDistance)` — no team filter, distance only
- Use these instead of any physics overlap when filtering game agents.
- `IsTeamAttitudeAligned(Agent1, Agent2)` — checks alignment

**Context helpers:**
- Getters/setters for all `FGeoGameplayEffectContext` custom fields (crit, block, knockback, radial, debuff)

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
