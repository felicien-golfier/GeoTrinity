# Abilities/Pattern

Deterministic enemy bullet pattern objects. **Enemy-only — server-driven.**

Pattern flow:
1. `UPatternAbility::ActivateAbility()` calls `PatternStartMulticast()` RPC
2. All clients instantiate the `UPattern` subclass
3. Pattern uses server time for deterministic spawning across all machines
4. On completion, `OnPatternEnd` delegate ends the ability

**One live instance per pattern class, per ASC.** `UGeoAbilitySystemComponent::FindPatternByClass` matches with
`IsA`, and the instance is reused across activations — so two abilities that want the same pattern with different
settings need **two BP subclasses**, not the same class twice (they would fight over one instance and one config).
That is why every knob lives on the pattern, not the ability: `UBeamPattern` ships the hex boss's sweeping laser and
its tile-carving ray as two separate BP children. It also means per-activation state must be reset in `InitPattern`
or `StartPattern` — never assume a fresh object.

---

## `Pattern.h` — base pattern object

- `OnCreate(AbilityTag)` — stores ability tag used for montage lookup
- `InitPattern(FAbilityPayload, TInstancedStruct<FPatternData>)` — stores payload and pattern-specific replicated data (`StoredPatternData`), plays start animation section
- `IsPatternActive()` — true while running
- `EndPattern(bForceStop=false)` — cleans up timers; when false, jumps montage to end section and broadcasts `OnPatternEnd`; when true, stops all montages immediately and skips the broadcast (used by `PatternAbility::EndAbility` to force-stop without re-triggering the ability-end chain). Guards against double-call via `bPatternIsActive`.
- `StartPattern()` — pure virtual; subclasses define spawn logic here

Stores: `FAbilityPayload StoredPayload`, `TInstancedStruct<FPatternData> StoredPatternData`, `UAnimMontage* AnimMontage`, effect data array.

---

## `UTickablePattern` — abstract ticking pattern

**Extend this for new bullet patterns. Implement `TickPattern(ServerTime, SpentTime)`.**

- `TickPattern(float ServerTime, float SpentTime)` — **no delta time provided by design** — deterministic across all clients
- `SpentTime = ServerTime - Payload.ServerSpawnTime`
- `CalculateTimeAndTickPattern()` — internal timer callback; reads `UGameplayLibrary::GetServerTime()`, computes `SpentTime`, calls `TickPattern()`

Always seed randomness from `Payload.Seed`.

---

## `SpiralPattern.h` — concrete spiral example

Config: `NumberProjectileByRound`, `TimeForOneRound`, `RoundNumber`, `DistanceSpan`
- Spawns projectiles in expanding circular sprays; calls `Projectile->OverrideDistanceSpan(DistanceSpan)` per projectile
- Tracks active projectiles; auto-ends them on actor destruction

---

## `SpawnPillarPattern.h` — zone-and-pillar boss pattern

Non-ticking pattern. **Zone locations are resolved on the server** by `UGeoSpawnPillarAbility::CreatePatternData()` and shipped through `PatternStartMulticast` as an `FSpawnPillarPatternData` — `InitPattern` just reads `ZoneLocations` (no per-client recompute, so all clients place zones identically). On `StartPattern`, spawns pillars and applies `PillarSpawnEffects` to hostiles in each zone (server-only), then calls `EndPattern`. The `DelayGameplayCueTag` countdown cue fires at each pillar location via the `ExecuteGameplayCue` override (fires one cue per spawn point).

- `FSpawnPillarPatternData` (in the same header) — `FPatternData` subclass carrying `TArray<FVector2D> ZoneLocations`; filled by the ability, consumed in `InitPattern`. `InitPattern` `ensureMsgf`s if the `TInstancedStruct` isn't an `FSpawnPillarPatternData` (i.e. the pattern was launched from a plain `UPatternAbility` instead of `UGeoSpawnPillarAbility`).
- `SpawningZoneSize` — radius used for both the countdown cue magnitude and hostile hit detection (cm)
- `PillarClass` — `AGeoPillar` subclass to spawn; also passed to `SetDeployableInfinitCount` in `OnCreate` to bypass slot limits
- `PillarParams` — `FDeployableDataParams` forwarded into the spawned pillar via `FullySpawnDeployable`
- `PillarSpawnEffects` — effects applied to hostiles in the zone on expiry (server-only); distinct from the pillar's own effect data

Runs on all clients via `PatternStartMulticast`. Launched by `UGeoSpawnPillarAbility` (see `Boss/CLAUDE.md`).

---

## `BeamPattern.h` — static or sweeping beam (hex boss)

Ticking, non-projectile. Beam fired from `StoredPayload.Origin` along `StoredPayload.Yaw`, on for `BeamDuration`.
Covers **both** hex-boss beams — configure two BP children, never one class for both (see the instance rule above).

- `SweepAngle` — full arc swept over `BeamDuration`, centred on the payload yaw. Starts at `Yaw - SweepAngle/2`.
  **0 = static beam**, which is the tile-carving ray; 90 is the sweeping laser.
- `BeamDuration`, `BeamRange`, `BeamHalfWidth` — on-time and geometry; half-width feeds `GetInteractableActorsInLine`
- `bDestroyLastTileHit` — on the tick where the beam goes live, `StartPattern` carves the furthest still-standing tile
  the beam reaches (`AGeoHexArena::GetLastAliveTileAlongRay`), server-only. The tank picks which rim tile that is by
  choosing where they stand when the boss locks on — that is the whole mechanic
- Each actor is hit **once per activation** (`HitActors`, cleared in both `StartPattern` and `EndPattern`), so crossing
  the beam costs one hit however slowly you walk through it. Damage is server-only; the VFX runs everywhere
- `BeamVfxSystem` — optional Niagara, spawned deactivated in `OnCreate` and re-used across activations (like
  `UDevastatingWavePattern`'s AOE component). Driven with the **same user parameters as `UGeoBeamVFXComponent`**
  (`User.Beam_Length`, `User.Beam_Width`), so beam systems are interchangeable between the two; author it local-space
  pointing +X. `EndPattern` deactivates gracefully on a natural end, `DeactivateImmediate` on force-stop — an
  interrupted beam must not linger
- Effect data comes from the launching ability's `GetEffectDataArray()`, like every pattern

## `ConeSprayPattern.h` — cone of scattered bullets (hex boss)

Ticking projectile pattern. Sprays `ProjectileCount` projectiles at random angles inside `ConeAngle` (centred on the
payload yaw), spread evenly over `SprayDuration`. Ends as soon as the last projectile is out — the bullets fly on by
themselves, the pattern does not own them (unlike `SpiralPattern`, which drives its projectiles' positions every tick).

- **Determinism**: projectile *i* seeds its own `FRandomStream(Payload.Seed + i)` rather than drawing from one shared
  stream. A machine that catches up on several projectiles in one tick still gives each the same angle as everyone
  else — a shared stream would desync the moment tick counts differed
- Each projectile is stamped with **its own** `ServerSpawnTime` (`Payload.ServerSpawnTime + StartDelay + i * interval`),
  so `FullySpawnProjectile` fast-forwards a late spawn to where it should already be
- `SpawnedCount` is reset in `InitPattern` — the instance is reused
- `OnCreate` pre-warms the pool for a full spray

## `TileBombPattern.h` — bomb riding a player (hex boss)

Non-ticking. The bomb sticks to one player for the whole wind-up, then detonates **wherever that player is standing
when it goes off**: effect data to everyone within `BlastRadius`, then `DestroyTilesInRadius` at the same spot. The
carrier chooses where the hole ends up — walk it to the rim, or lose the middle of the platform.

- `FTileBombPatternData` (same header) — `FPatternData` subclass carrying `BombCarrier`; drawn once on the server by
  `UGeoTileBombAbility` and shipped through the multicast, so every machine shows the countdown on the same player
- `FillCueParam` puts the carrier in `EffectCauser` and their location in `Location`, so the countdown cue notify can
  attach itself to them and ride along
- A carrier who dies or leaves during the wind-up simply fizzles the bomb (`EndPattern`, no blast)
- Damage is applied **before** the tiles are carved, so the blast catches whoever is standing on ground that is about
  to stop existing

## `DevastatingWavePattern.h` — expanding radial wave

Non-projectile ticking pattern. On `InitPattern`, teleports the instigator to `StoredPayload.Origin`. Each tick expands a radius at `ExpansionSpeed`; any hostile actor whose distance falls within `CurrentRadius` is hit once. Pillars are recalled; other hostiles receive the ability's effect data (via `EffectDataArray` set by `PatternAbility`). Ends when `CurrentRadius >= MaxRadius`.

- `ClearData()` — public; empties `HitActors` and `PillarsWaveData`, resets all 8 `PillarPosWS_XX` MPC slots to the unused sentinel `(-10000, -10000, -10000, 0)`. Called at the start of both `InitPattern` and `StartPattern`, and again at the end of `EndPattern`, so stale data from a previous activation never bleeds in.
- `ExpansionSpeed` — cm/s expansion rate (default 800)
- `MaxRadius` — stops the wave at this distance (default 3000)
- Effect data: sourced from ability's `GetEffectDataArray()` — configure on `GeoDevastatingWaveAbility`, not here
- Used by `UGeoDevastatingWaveAbility`

**Masked AOE VFX** (every rendering machine, gated `!IsDedicatedServer`):
- `OnCreate` spawns the `AOEVfxSystem` (NS_PillarsAOE) component once, deactivated with `bAutoDestroy = false` — the pattern instance is reused across activations, and so is the component.
- **Telegraph (wind-up) phase**: after calling `ClearData()` and teleporting the instigator, `InitPattern` calls `AddAllPillarsToVfxMask()` (pre-populates the 8 MPC pillar-mask slots with all currently-alive pillars so safe zones are already visible on the static telegraph), then calls `ActivateAoeVfxTelegraph()` — a single smooth fill showing the full-range danger zone (`MaxRadius`) in `TelegraphColor` (editable, default red) that grows over the remaining wind-up (`StartDelay − TravelTime`). Skipped when `Super::InitPattern` took the "too late" path and ran `StartPattern` synchronously (no wind-up).
- `StartPattern` calls `ClearData()` (which resets the 8 MPC pillar slots to the sentinel), then calls `ActivateAOEVfx()` so the static telegraph becomes the expanding wave.
- `ActivateAOEVfx` moves the component to the wave origin, sets `AOE_Radius = MaxRadius`, `AOE_GrowDuration = MaxRadius / ExpansionSpeed`, plus editable `FadeOutDuration` / `AOEColor`, and calls `Activate(true)`.
- Pillar detection runs on **all machines** in `TickPattern` (not just the server): deterministic since pillars are static replicated actors and the radius derives from server time. Each pillar the wave front reaches is appended to `PillarsWaveData` and its world position written to the next MPC slot (`AddPillarToVfxMask`), cutting a safe-zone shadow out of the AOE material. Damage application remains server-only.
- `EndPattern` deactivates the spawned Niagara component: graceful `Deactivate()` on a natural end (lets the fade-out play), `DeactivateImmediate()` on force-stop — a graceful deactivate would let the AOE particle live out its full grow+fade lifetime and linger after an interrupt.
- BP wiring lives in `BP_DevastatingWavePattern` (`AOEVfxSystem`, `MaskMaterialParameterCollection`).
