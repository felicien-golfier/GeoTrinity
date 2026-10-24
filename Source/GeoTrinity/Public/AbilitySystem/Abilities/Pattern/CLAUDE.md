# Abilities/Pattern

Enemy pattern objects. Server-driven, deterministic across machines.

| Folder / file | Holds |
|---|---|
| `Pattern.h` | `UPattern` base and `UTickablePattern` |
| `Projectile/` | Patterns that spawn projectiles (spiral, cone spray, salve) |
| `Hazard/` | Patterns that hit through server-judged geometry (beam, wave, pillar zones, zone) |

Flow: `UPatternAbility::ActivateAbility` → `PatternStartMulticast` → every machine instantiates the pattern, runs on server time → completion fires `OnPatternEnd` → ability ends.

## Pattern.h
- `UPattern`: `OnCreate(AbilityTag)`, `InitPattern(Payload, PatternData)`, pure `StartPattern()`, `EndPattern(bForceStop)`.
- `EndPattern(false)`: jumps montage to end section + broadcasts `OnPatternEnd`; `true`: immediate stop, no broadcast (ability's `EndAbility` path).
- `UTickablePattern`: extend for new patterns; implement `TickPattern(ServerTime, SpentTime)`.
  - `SpentTime = ServerTime - Payload.ServerSpawnTime`; no delta time, deterministic.
  - Loop started by `InitPattern`, self-schedules with `SetTimerForNextTick`.
  - Wind-up ticks go to `TickDuringInit(SpentTime)` (negative, counting to 0); override to keep a telegraph on its target.
  - `StartPattern` never kicks the loop itself.
- Montage: `AnimMontage` is cosmetic and played locally by each rendering machine, never by a dedicated server and never replicated; `HasMontage()` tells the launching ability to leave its own montage unplayed (`Base/CLAUDE.md`, "Montage ownership").
- Randomness always from `Payload.Seed`.
- One live instance per pattern class per ASC (`FindPatternByClass`, `IsA`): two configs live at once on one caster = two BP subclasses (Training Room's six zones); one at a time = one BP, per-firing data (`Hazard/CLAUDE.md`, "LateralWave"). Reset per-activation state in `InitPattern`/`StartPattern`.

## Lag compensation (every server hit test)
- Judge each target at its own time: `GeoLib::GetPerceivedServerTime(Actor)`.
- Continuous hazard: evaluate geometry at `SpentTime - (ServerTime - PerceivedTime)`, hazard live over the target's own `[0, Duration]`.
- Swept, not sampled: keep last per-target offset (`TMap<TWeakObjectPtr<AActor>, float>`, reset in `StartPattern`), test the whole span, hit on entering only.
- End when the oldest target's own time reaches the end.
- Discrete event at time T: `FGeoLagCompensatedEvent` (`Tool/Net/GeoHazardJudge.h`).
  - Pass every potential target to `JudgeActorsReachingEvent`, then filter by volume.
  - Keep judging until `IsOver` (fixed window).
  - Server-spawned blocking geometry spawns once `IsOver`.
- A pattern needing this is a `UTickablePattern`; clients have no hit logic and end at their own time.
- Projectile patterns still hit through projectile overlap at server time (known gap).
