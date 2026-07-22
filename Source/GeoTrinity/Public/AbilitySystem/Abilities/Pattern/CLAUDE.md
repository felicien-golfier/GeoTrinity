# Abilities/Pattern

Deterministic enemy bullet pattern objects. Enemy-only, server-driven.

Flow: `UPatternAbility::ActivateAbility()` → `PatternStartMulticast()` RPC → all clients instantiate the `UPattern` subclass → uses server time for deterministic spawning → completion fires `OnPatternEnd`, ending the ability.

**One live instance per pattern class, per ASC.** `FindPatternByClass` matches with `IsA` and reuses the instance across activations — two abilities wanting the same pattern with different settings need **two BP subclasses**, not one class twice (e.g. `UBeamPattern` ships the hex boss's sweep laser and tile-carving ray as separate BP children). Every knob lives on the pattern, not the ability. Per-activation state must be reset in `InitPattern`/`StartPattern` — never assume a fresh object.

---

## `Pattern.h` — base pattern object
- `OnCreate(AbilityTag)` — stores tag for montage lookup
- `InitPattern(Payload, PatternData)` — stores payload + `StoredPatternData`, plays start animation
- `IsPatternActive()`, `EndPattern(bForceStop=false)` — cleans timers; `false` jumps montage to end section + broadcasts `OnPatternEnd`; `true` stops montages immediately and skips the broadcast (used by `PatternAbility::EndAbility` to avoid a recursive end chain). Guarded by `bPatternIsActive`
- `StartPattern()` — pure virtual, subclass spawn logic

## `UTickablePattern` — abstract ticking pattern
**Extend this for new bullet patterns. Implement `TickPattern(ServerTime, SpentTime)`.**
- No delta time by design — deterministic across clients. `SpentTime = ServerTime - Payload.ServerSpawnTime`
- Always seed randomness from `Payload.Seed`

## `SpiralPattern.h` — concrete spiral example
Config: `NumberProjectileByRound`, `TimeForOneRound`, `RoundNumber`, `DistanceSpan`. Expanding circular sprays; tracks active projectiles, auto-ends them on actor destruction.

## `SpawnPillarPattern.h` — zone-and-pillar boss pattern
Non-ticking. Zone locations resolved server-side by `UGeoSpawnPillarAbility::CreatePatternData()`, shipped via `PatternStartMulticast` as `FSpawnPillarPatternData` — `InitPattern` just reads `ZoneLocations` (no per-client recompute). `StartPattern` spawns pillars, applies `PillarSpawnEffects` to hostiles in each zone (server-only), calls `EndPattern`. `DelayGameplayCueTag` countdown cue fires per spawn point.
- `FSpawnPillarPatternData` — `ZoneLocations`; `InitPattern` `ensureMsgf`s if launched from a plain `UPatternAbility`
- `PillarClass` also passed to `SetDeployableInfinitCount` in `OnCreate` to bypass slot limits

## `BeamPattern.h` — static or sweeping beam (hex boss)
Ticking, non-projectile. Fired from `StoredPayload.Origin` along `Yaw`, on for `BeamDuration`. Covers both hex-boss beams via two BP children (see instance rule above).
- `SweepAngle` — arc over `BeamDuration`, centred on payload yaw; **0 = static** (tile-carving ray), 90 = sweeping laser
- `bDestroyLastTileHit` — on go-live tick, carves the furthest still-standing tile the beam reaches (`GetLastAliveTileAlongRay`), server-only — tank chooses the rim tile by where they stand when the boss locks on
- Each actor hit once per activation (`HitActors`, cleared in `StartPattern`/`EndPattern`) — damage server-only, VFX everywhere
- `BeamVfxSystem` — spawned deactivated in `OnCreate`, reused across activations, driven with the same user params as `UGeoBeamVFXComponent` (`User.Beam_Length`/`Width`); author local-space +X. `EndPattern`: graceful `Deactivate` on natural end, `DeactivateImmediate` on force-stop

## `ConeSprayPattern.h` — cone of scattered bullets (hex boss)
Ticking projectile pattern. Sprays `ProjectileCount` at random angles inside `ConeAngle` over `SprayDuration`; ends once the last projectile is out (bullets fly on their own, unlike `SpiralPattern`).
- Determinism: projectile *i* seeds its own `FRandomStream(Payload.Seed + i)` (not a shared stream) so a machine catching up on several projectiles in one tick still matches everyone else
- Each projectile stamped with its own `ServerSpawnTime` so a late spawn fast-forwards correctly
- `SpawnedCount` reset in `InitPattern`; `OnCreate` pre-warms the pool

## `DevastatingWavePattern.h` — expanding radial wave
Non-projectile ticking. `InitPattern` teleports instigator to `StoredPayload.Origin`. Each tick expands a radius at `ExpansionSpeed`; hostiles within `CurrentRadius` hit once (pillars recalled, others get the ability's effect data). Ends at `CurrentRadius >= MaxRadius`.
- `ClearData()` — resets `HitActors`/`PillarsWaveData`/8 MPC pillar slots to sentinel; called at start of `InitPattern`/`StartPattern` and end of `EndPattern`, so stale data never bleeds in
- `ExpansionSpeed` (default 800 cm/s), `MaxRadius` (default 3000)

**Masked AOE VFX** (all rendering machines, gated `!IsDedicatedServer`):
- `OnCreate` spawns `AOEVfxSystem` (NS_PillarsAOE) once, deactivated, `bAutoDestroy=false`, reused across activations
- Telegraph phase: `InitPattern` pre-populates the 8 MPC pillar-mask slots with currently-alive pillars, then `ActivateAoeVfxTelegraph()` shows the full-range danger zone growing over the wind-up. Skipped if `Super::InitPattern` ran `StartPattern` synchronously (too-late path)
- `StartPattern` → `ClearData()` → `ActivateAOEVfx()` turns the telegraph into the expanding wave
- Pillar detection runs on **all machines** in `TickPattern` (deterministic — static replicated actors, server-time-derived radius); each pillar reached is appended to `PillarsWaveData` and written to the next MPC slot to cut a safe-zone shadow. Damage stays server-only
- `EndPattern`: graceful `Deactivate()` on natural end (lets fade-out play), `DeactivateImmediate()` on force-stop
- BP wiring in `BP_DevastatingWavePattern` (`AOEVfxSystem`, `MaskMaterialParameterCollection`)
