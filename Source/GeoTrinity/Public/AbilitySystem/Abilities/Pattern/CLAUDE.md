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

## `DevastatingWavePattern.h` — expanding radial wave

Non-projectile ticking pattern. On `StartPattern`, teleports the owner to `StoredPayload.Origin`. Each tick expands a radius at `ExpansionSpeed`; any hostile actor whose distance falls within `CurrentRadius` is hit once. Pillars are recalled; other hostiles receive the ability's effect data (via `EffectDataArray` set by `PatternAbility`). Ends when `CurrentRadius >= MaxRadius`.

- `ExpansionSpeed` — cm/s expansion rate (default 800)
- `MaxRadius` — stops the wave at this distance (default 3000)
- Effect data: sourced from ability's `GetEffectDataArray()` — configure on `GeoDevastatingWaveAbility`, not here
- Used by `UGeoDevastatingWaveAbility`

**Masked AOE VFX** (every rendering machine, gated `!IsDedicatedServer`):
- `OnCreate` spawns the `AOEVfxSystem` (NS_PillarsAOE) component once, deactivated with `bAutoDestroy = false` — the pattern instance is reused across activations, and so is the component.
- **Telegraph (wind-up) phase**: `InitPattern` activates the component in telegraph mode — `User.Telegraph_Active = true`, `User.Telegraph_Color = TelegraphColor` (editable, default red) — so the full-range danger zone (`MaxRadius`) renders as a blinking warning *before* the wave starts. Each blink (one full telegraph fill + fade) is one `ActivateAOEVfxTelegraph()` call. `InitPattern` calls the recurring `TickTelegraphBlink()` once; it fires the telegraph and re-arms `TelegraphBlinkTimerHandle` back to itself at the theoretical interval `StartDelay / TelegraphBlinkCount` (editable, default 3) — the full wind-up, not the latency-shortened remaining time, so blinks stay the same cadence under ping. No counter bounds it: the base's `StartSectionTimerHandle` ends the wind-up and `StartPattern` clears the blink timer, so the telegraph pulses up to `TelegraphBlinkCount` times before the wave starts. Gated on `StartSectionTimerHandle.IsValid()` so it is skipped when `Super::InitPattern` took the "too late" path and ran `StartPattern` synchronously (no wind-up) — otherwise the VFX would be stuck telegraphing with no wave. The blink timer is cleared in both `StartPattern` (a still-pending blink when the wave begins) and `EndPattern` (force-stop during the wind-up).
- `StartPattern` (the moment the wave starts expanding) resets the 8 `PillarPosWS_XX` slots of `MaskMaterialParameterCollection` (MPC_MaskedArea) to the unused sentinel `(-10000, -10000, -10000, 0)` — the MPC is global state shared across waves — then flips `User.Telegraph_Active = false` and re-activates (`ActivateAOEVfx`) so the static telegraph becomes the expanding wave.
- `ActivateAOEVfx` (shared by both phases) moves the component to the wave origin, sets `AOE_Radius = MaxRadius`, `AOE_GrowDuration = MaxRadius / ExpansionSpeed`, plus editable `FadeOutDuration` / `AOEColor`, and calls `Activate(true)`.
- Pillar detection runs on **all machines** in `TickPattern` (not just the server): deterministic since pillars are static replicated actors and the radius derives from server time. Each pillar the wave front reaches is appended to `PillarsWaveData` and its world position written to the next MPC slot (`AddPillarToVfxMask`), cutting a safe-zone shadow out of the AOE material. Damage application remains server-only.
- `EndPattern` deactivates the spawned Niagara component: graceful `Deactivate()` on a natural end (lets the fade-out play), `DeactivateImmediate()` on force-stop — a graceful deactivate would let the AOE particle live out its full grow+fade lifetime and linger after an interrupt.
- BP wiring lives in `BP_DevastatingWavePattern` (`AOEVfxSystem`, `MaskMaterialParameterCollection`).
