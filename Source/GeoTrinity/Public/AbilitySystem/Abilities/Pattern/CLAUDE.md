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
- `EndPattern(bForceStop=false)` — cleans up timers; when false, jumps montage to end section and broadcasts `OnPatternEnd`; when true, stops all montages immediately and skips the broadcast (used by `PatternAbility::EndAbility` to force-stop without re-triggering the ability-end chain). Guards against double-call via `bPatternIsActive`.
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

## `SpawnPillarPattern.h` — zone-and-pillar boss pattern

Non-ticking pattern. On `InitPattern`, determines how many pillars to spawn (1–3, scaled by the boss's remaining health ratio) and selects target player locations sorted by `PlayerId` for determinism. On `StartPattern`, spawns pillars and applies `PillarSpawnEffects` to hostiles in each zone (server-only), then calls `EndPattern`. The `DelayGameplayCueTag` countdown cue fires at each pillar location via the `ExecuteGameplayCue` override (fires one cue per spawn point).

- `SpawningZoneSize` — radius used for both the countdown cue magnitude and hostile hit detection (cm)
- `PillarClass` — `AGeoPillar` subclass to spawn; also passed to `SetDeployableInfinitCount` in `OnCreate` to bypass slot limits
- `PillarParams` — `FDeployableDataParams` forwarded into the spawned pillar via `FullySpawnDeployable`
- `PillarSpawnEffects` — effects applied to hostiles in the zone on expiry (server-only); distinct from the pillar's own effect data

Runs on all clients via `PatternStartMulticast`. Used by a `UPatternAbility` subclass.

---

## `DevastatingWavePattern.h` — expanding radial wave

Non-projectile ticking pattern. On `StartPattern`, teleports the owner to `StoredPayload.Origin`. Each tick expands a radius at `ExpansionSpeed`; any hostile actor whose distance falls within `CurrentRadius` is hit once. Pillars are recalled; other hostiles receive the ability's effect data (via `EffectDataArray` set by `PatternAbility`). Ends when `CurrentRadius >= MaxRadius`.

- `ExpansionSpeed` — cm/s expansion rate (default 800)
- `MaxRadius` — stops the wave at this distance (default 3000)
- Effect data: sourced from ability's `GetEffectDataArray()` — configure on `GeoDevastatingWaveAbility`, not here
- Used by `UGeoDevastatingWaveAbility`
