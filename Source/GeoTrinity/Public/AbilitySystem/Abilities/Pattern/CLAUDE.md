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
- `ZoneSize` — radius of the danger zone in cm
- `PillarClass` — `GeoPillar` subclass to spawn on expiry
- `CountdownGameplayCueTag` — cue fired at start to show the countdown indicator
- `ExpiryGameplayCueTag` — cue fired on expiry
- `ZoneEffectDataArray` — effects applied to hostiles in the zone on expiry (server-only)
- `PillarEffectDataArray` — effects passed into the spawned pillar's `FDeployableData`

Runs on all clients via `PatternStartMulticast`. Server time in the payload ensures countdown sync.
Used by `UGeoDelayedFatalZoneAbility`.
