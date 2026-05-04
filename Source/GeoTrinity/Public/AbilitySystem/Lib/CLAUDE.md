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

**Team utilities:**
- `GetAllAgentsInTeam(World, Team)` — all agents with matching team
- `GetAllAgentsWithRelationTowardsActor(World, Actor, Attitude)` — all agents with given attitude toward `Actor`
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
