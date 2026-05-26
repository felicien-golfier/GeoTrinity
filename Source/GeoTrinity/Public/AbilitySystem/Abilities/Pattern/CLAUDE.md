# Abilities/Pattern

Deterministic enemy bullet pattern objects. **Enemy-only — server-driven.**

Pattern flow:
1. `UPatternAbility::ActivateAbility()` calls `PatternStartMulticast()` RPC
2. All clients instantiate the `UPattern` subclass
3. Pattern uses server time for deterministic spawning across all machines
4. On completion, `OnPatternEnd` delegate ends the ability

---

## `Pattern.h` — base pattern object

- `OnCreate(AbilityTag)` — stores ability tag used for montage lookup
- `InitPattern(FAbilityPayload)` — stores payload, plays start animation section
- `IsPatternActive()` — true while running
- `EndPattern()` — jumps animation to end section, broadcasts `OnPatternEnd`
- `StartPattern()` — pure virtual; subclasses define spawn logic here

Stores: `FAbilityPayload Payload`, `UAnimMontage* AnimMontage`, effect data array.

---

## `UTickablePattern` — abstract ticking pattern

**Extend this for new bullet patterns. Implement `TickPattern(ServerTime, SpentTime)`.**

- `TickPattern(float ServerTime, float SpentTime)` — **no delta time provided by design** — deterministic across all clients
- `SpentTime = ServerTime - Payload.ServerSpawnTime`
- `CalculateTimeAndTickPattern()` — internal timer callback; reads `UGameplayLibrary::GetServerTime()`, computes `SpentTime`, calls `TickPattern()`

Always seed randomness from `Payload.Seed`.

---

## `SpiralPattern.h` — concrete spiral example

Config: `NumberProjectileByRound`, `TimeForOneRound`, `RoundNumber`
- Spawns projectiles in expanding circular sprays
- Tracks active projectiles; auto-ends them on actor destruction

---

## `FatalZonePattern.h` — zone-and-pillar boss pattern

Non-ticking pattern that marks a zone under a random player, shows a countdown VFX, then on expiry applies damage and spawns a `GeoPillar`.

- `CountdownDuration` — seconds before expiry (default 3s)
- `SpawningZoneSize` — radius of the danger zone in cm (renamed from `ZoneSize`)
- `PillarClass` — `GeoPillar` subclass to spawn on expiry
- `PillarParams` — `FDeployableDataParams` forwarded into the spawned pillar's `FDeployableData`; drives pillar size, blink duration, etc. via data asset instead of hardcoded values
- `CountdownGameplayCueTag` — cue fired at start to show the countdown indicator
- `ExpiryGameplayCueTag` — cue fired on expiry
- `ZoneEffectDataArray` — effects applied to hostiles in the zone on expiry (server-only)

Runs on all clients via `PatternStartMulticast`. Server time in the payload ensures countdown sync.
Used by `UGeoDelayedFatalZoneAbility`.

---

## `DevastatingWavePattern.h` — expanding radial wave

Non-projectile ticking pattern. On `StartPattern`, teleports the owner to `StoredPayload.Origin`. Each tick expands a radius at `ExpansionSpeed`; any hostile actor whose distance falls within `CurrentRadius` is hit once. Pillars are recalled; other hostiles receive the ability's effect data (via `EffectDataArray` set by `PatternAbility`). Ends when `CurrentRadius >= MaxRadius`.

- `ExpansionSpeed` — cm/s expansion rate (default 800)
- `MaxRadius` — stops the wave at this distance (default 3000)
- Effect data: sourced from ability's `GetEffectDataArray()` — configure on `GeoDevastatingWaveAbility`, not here
- Used by `UGeoDevastatingWaveAbility`
