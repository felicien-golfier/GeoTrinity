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
