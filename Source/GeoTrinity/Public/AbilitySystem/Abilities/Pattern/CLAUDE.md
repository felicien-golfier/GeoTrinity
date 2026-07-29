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
- The loop is started once by `InitPattern` (right after `Super::`) and self-schedules with `SetTimerForNextTick`, so it already runs through the wind-up: while `StartSectionTimerHandle` is still pending each tick goes to `TickDuringInit(SpentTime)` (same timeline as `TickPattern`, so **negative**, counting up to 0), afterwards to `TickPattern`. Override `TickDuringInit` to keep a telegraph following its target; it never fires on the too-late path (no wind-up scheduled)
- **`StartPattern` must not kick the loop itself.** `FTimerManager::IsTimerActive` is true for a timer *while its own callback runs*, so a tick called synchronously from `StartPattern` (the Start-section timer's callback) would be routed to `TickDuringInit`. Consequence: the first `TickPattern` lands the frame *after* `StartPattern` — fine because everything is derived from `SpentTime`, never accumulated per tick

## `SpiralPattern.h` — concrete spiral example
Config: `NumberProjectileByRound`, `TimeForOneRound`, `RoundNumber`, `ProjectileParams` (`FGeoProjectileParams` — class + distance/speed/radius/color overrides). Expanding circular sprays; tracks active projectiles, auto-ends them on actor destruction. Distance span is now the struct's `bOverrideDistanceSpan`/`DistanceSpan` (BP must toggle it for the old always-override behavior).
**Re-entrancy:** `TickPattern` drives each projectile by `SetActorLocation`, which resolves overlaps *synchronously* — a projectile can die, return to the pool and have its `Projectiles[i]` slot nulled in the middle of the iteration that moved it. Hence the two guards: `!bPatternIsActive` at the top of the loop, and the `Projectiles[i] != Projectile` re-check after the move. Without the latter, the `GetActorState()` "not started yet → Init()" branch resurrects a projectile that is already back in the pool (`GetActorState` is equally false for *dead* and for *not yet started*), leaving it live-and-pooled: the next `ReleaseActor` trips the released-twice ensure, its slot never nulls, and the pattern never ends.

## `SpawnPillarPattern.h` — zone-and-pillar boss pattern
Non-ticking. Zone locations resolved server-side by `UGeoSpawnPillarAbility::CreatePatternData()`, shipped via `PatternStartMulticast` as `FSpawnPillarPatternData` — `InitPattern` just reads `ZoneLocations` (no per-client recompute). `StartPattern` spawns pillars, applies `PillarSpawnEffects` to hostiles in each zone (server-only), calls `EndPattern`. `DelayGameplayCueTag` countdown cue fires per spawn point.
- `FSpawnPillarPatternData` — `ZoneLocations`; `InitPattern` `ensureMsgf`s if launched from a plain `UPatternAbility`
- Deployables default to unlimited (`AGeoDeployableBase::bUnlimitedDeploy`), so `PillarClass` needs no manual slot-cap bypass

## `BeamPattern.h` — static or sweeping beam (hex boss)
Ticking, non-projectile. Fired from `StoredPayload.Origin` along `Yaw`, on for `BeamDuration`. Covers both hex-boss beams via two BP children (see instance rule above).
- `SweepAngle` — arc over `BeamDuration`, centred on payload yaw; **0 = static** (tile-carving ray), 90 = sweeping laser
- `bDestroyLastTileHit` — on go-live tick, carves the furthest still-standing tile the beam reaches (`GetLastAliveTileAlongRay`), server-only — tank chooses the rim tile by where they stand when the boss locks on
- Each actor hit once per activation (`HitActors`, cleared in `StartPattern`/`EndPattern`) — damage server-only, VFX everywhere
- `BeamVfxSystem` — spawned deactivated in `OnCreate`, reused across activations, driven with the same user params as `UGeoBeamVFXComponent` (`User.Beam_Length`/`Width`); author local-space +X. `EndPattern`: graceful `Deactivate` on natural end, `DeactivateImmediate` on force-stop
- `GetBeamOrigin()` + `MoveBeamVfx(SpentTime)` are the single aiming path, used by `TickDuringInit` and `TickPattern` alike — so the telegraph tracks a `FollowBossLocation`/`FollowBossOrientation` boss during the wind-up exactly like the live beam does. `InitPattern` only configures the component; the first `TickDuringInit` (fired from `Super::InitPattern`) has already placed it by then
- `PreviewSystem` — windup telegraph (Ray Zone Indicator), loaded once in `OnCreate` from `UGameDataSettings::BeamPreviewSystem` (one project-wide asset, no per-pattern/BP configuration). Shown from `InitPattern` (montage Start section) until `StartPattern` swaps the same `BeamVfxComponent` over to `BeamVfxSystem`, both via the shared `GeoNiagaraParams::ApplySwappableAsset(Component, {BeamSystem, PreviewSystem}, bWantPreview)` helper (asset compare + `SetAsset`, no replication needed — patterns already run identically on every machine). Same helper `UGeoBeamVFXComponent` uses for its preview/beam handoff on `UGeoChannelBeamAbility` — see `Tool/CLAUDE.md`. Leaving the settings value unset shows `BeamVfxSystem` for the whole windup, as before.
- `TickPattern`'s server branch draws the actual hit-scan rectangle (`BeamRange`/`BeamHalfWidth`) fed to `GetInteractableActorsInLine` as red `DrawDebugLine`s, gated on the `Geo.DrawBeamBorder` CVar — matches the pattern in `UGeoChannelBeamAbility::DrawBeamDebugLines`
- `OverlapMode` (`ETargetOverlapMode`, BP knob, default `Automatic`) → the line query's target-radius rule; `Automatic` resolves to center-only because the boss is an `Enemy` source

## `ConeSprayPattern.h` — cone of bullets in timed salves (hex boss)
Ticking projectile pattern. Fires `SalveNumber` salves `SalveFrequencySec` apart; each salve fans `ProjectileCountPerSalve` projectiles evenly across `ConeAngle` (centred on payload yaw). Ends once the last salve is out (bullets fly on their own, unlike `SpiralPattern`).
- Each salve is stamped with its *scheduled* `ServerSpawnTime` (`payload spawn + StartDelay + N*SalveFrequencySec`), **not** the current tick time — so an on-time salve spawns at the origin (`TimeDelta≈0`) and a late one fast-forwards into place. Using the tick time instead advanced every projectile by `StartDelay` and was per-machine non-deterministic
- `SpawnedSalveCount` reset in `InitPattern`; `OnCreate` pre-warms the pool for a full spray

## `DevastatingWavePattern.h` — expanding radial wave
Non-projectile ticking. `InitPattern` teleports instigator to `StoredPayload.Origin`. Each tick expands a radius at `ExpansionSpeed`; only hostiles whose center sits in the band `[CurrentRadius - AnnulusWidth, CurrentRadius]` are hit (the moving wave front) — once the front passes them they are safe. Pillars are added to the VFX mask as soon as the front reaches them (deduped against `PillarsWaveData`, no per-hit set). Ends at `CurrentRadius >= MaxRadius`.
- `ClearData()` — resets `PillarsWaveData`/8 MPC pillar slots to sentinel; called at start of `InitPattern`/`StartPattern` and end of `EndPattern`, so stale data never bleeds in
- `ExpansionSpeed` (default 800 cm/s), `MaxRadius` (default 3000), `AnnulusWidth` (default 200 — damaging band width just inside the front)
- No `HitActors` dedup — an actor lingering in the band is hit each tick it stays there
- `DrawDebugWave(CurrentRadius)` — red circle = wave front, green = inner annulus edge, plus per-pillar safe-zone tangent lines; called every tick, gated on the `Geo.DrawDevastatingWave` CVar

**Masked AOE VFX** (all rendering machines, gated `!IsDedicatedServer`):
- `OnCreate` spawns `AOEVfxSystem` (NS_PillarsAOE) once, deactivated, `bAutoDestroy=false`, reused across activations
- Telegraph phase: `InitPattern` pre-populates the 8 MPC pillar-mask slots with currently-alive pillars, then `ActivateAoeVfxTelegraph()` shows the full-range danger zone growing over the wind-up. Skipped if `Super::InitPattern` ran `StartPattern` synchronously (too-late path)
- `StartPattern` → `ClearData()` → `ActivateAOEVfx()` turns the telegraph into the expanding wave
- Pillar detection runs on **all machines** in `TickPattern` (deterministic — static replicated actors, server-time-derived radius); each pillar reached is appended to `PillarsWaveData` and written to the next MPC slot to cut a safe-zone shadow. Damage stays server-only
- `EndPattern`: graceful `Deactivate()` on natural end (lets fade-out play), `DeactivateImmediate()` on force-stop
- BP wiring in `BP_DevastatingWavePattern` (`AOEVfxSystem`, `MaskMaterialParameterCollection`)
