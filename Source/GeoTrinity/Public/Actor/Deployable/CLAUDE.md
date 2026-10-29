# Actor/Deployable

Every deployable. Extend `AGeoDeployableBase` for a new one.

| Folder / file | Holds |
|---|---|
| `GeoDeployableBase.h` | Abstract base: drain, blink, recall, explode |
| `GeoDeployableFXComponent.h` | Per-moment FX of a deployable |
| `Wall/` | Square wall, draining health, consumed by Detonate |
| `Turret/` | Triangle auto-firing turret |
| `HealingZone/` | Circle area heal |
| `BuffPickup/` | Reload buff pickup |
| `Pillar/` | Boss pillar blocking the devastating wave |
| `Mine/` | Hex-boss mine, fuse + burst |
| `BombZone/` | Hex-boss tile bomb |
| `Zones/` | `GeoEffectZone`: editor-placed capsule applying `EffectDataArray`; owns its ASC; heal/damage re-applied per tick (per-second magnitude), other types once on entry/exit; grows when its params say so (Data and fields) |

## Lifecycle
- Spawned server-side, always through `GeoASLib::FullySpawnDeployable` (`AbilitySystem/Lib/CLAUDE.md`; no deployable, pickups included, calls `SpawnActorDeferred` itself): by `ADeployableSpawnerProjectile::EndProjectileLife()` for a throw, by `GeoASLib::SpawnDeployableFromAbility` for a passive's placement, by `GeoSpawnZoneAction` for an encounter's self-owned zone (`Encounter/Action/CLAUDE.md`) → `InitDrain()` → `ApplyHealthMultiplier()` → server `Tick()` drains → zero health/duration starts blink → `Expire()` (hide, event, delayed destroy).
- `Recall(Value)` is the only end-of-life path: `RecallEffect(Value)` then `Expire()`; non-server also fires the recall cue locally.
- On hex arenas the fall-check recalls/expires anything over a dead tile; subclasses with recall side effects must account for it.

## Init phase
- `Params.InitDuration` > 0 (set by the spawner, 0 = none): the deployable *arrives* before it lives. From BeginPlay: no collision, not damageable (health bar hidden), no drain, no push, until `FinishInit()` hands over to the normal life (push, collision, damageable again, spawn cue + `Spawn` moment, so the impact plays on landing). Who ends it: `Params.bAutoFinishInit` true (default) = every machine, when `InitDuration` is over; false = the spawner, server-side, when it chooses (the arrival animation rests at the end pose meanwhile). `bInitialising` replicates, so clients follow a spawner's release.
- Same on every machine, off the server clock: `InitStartServerTime` (server BeginPlay, initial-only like `Data`) → `Alpha`; a late client draws the same frame.
- Arrival = visual only: every visual mesh goes from `InitStartOffset` (location added, rotation composed, scale multiplied; X / Y / Z, tilt and size in one transform) to its rest transform, driven by `InitCurve` (Alpha 0..1 → share of the offset left, 1 → 0; null = linear). `InitMontage` (per BP, on the first skeletal mesh) is stretched over `InitDuration`. The top-down orthographic view shows no height, so a scale above 1 is what reads as a fall.
- `InitCue` + `Init` moment fire at its start like the other moments (cue `Normal.X` = `InitDuration`, so a notify times itself); spawn cue + `Spawn` moment wait for the landing.
- `IsInitialising()`; `Expire(true)` is the way to cancel one.

## Override points
- `RecallEffect(Value)`: server end-of-life behaviour (default none).
- `ExplodeEffect(...)`: default applies `EffectDataArray` within `Params.Size`, lag-compensated through `JudgeExplosion` (`FGeoLagCompensatedEvent`, driven by `SetTimerForNextTick`); `TimeBeforeDestroyAtExpire` must outlive 2 × `MaxLatencyCompensation`.
- `Explode(Value)`: server sphere overlap, `ExplodeAttitude` (default Hostile), `ExplodeOverlapMode` (Automatic = centre-only for Enemy casters).

## Data and fields
- `FDeployableData : FInteractableActorData`: `EffectDataArray`, `Params` (`BlinkDuration`, `LifeDrainMaxDuration`, `InitDuration`, `Size`, `Value`), `PredictionKey`, `AbilityTag`.
- Growth params (zones only): `GrowToSize`, `GrowSeconds` (`FScalableFloat`, read at `Data.Level`, so a spawner points them at a curve table row). `GrowToSize` 0 = fixed `Size`; else the radius goes `Size` → `GrowToSize` over `GrowSeconds` from the server's BeginPlay, then stays. Every machine grows it on the server clock (`GeoLib::GetServerTime`) from `GrowthStartServerTime` (initial-only, like `Data`), so a late client draws the radius the server judges. `Size` is never written after spawn (what a late client receives); the grown radius = `AGeoEffectZone::GetRadius()`, read by the judge and `ApplyRadius`.
- Health params (`FDeployableDataParams`, set by the deployer, never by hand): `HealthMultiplier` scales `MaxHealth` and `Health`; `StartHealthRatio` (0..1, default 1) then sets `Health = MaxHealth * StartHealthRatio`, `MaxHealth` untouched.
- `DeployDistance` (same struct, set by the deployer, -1 = not thrown): the throw distance the deployable's Leverage was computed from; a copy of it reads it to carry Leverage (`SpawnDeployableFromAbility`).
- Drain rate = `MaxHealth / LifeDrainMaxDuration`, computed before the start ratio: a half-health start lives half as long at the normal rate; health bar and blink follow `Health / MaxHealth`.
- Replicated `*Data` is `COND_InitialOnly` on every concrete deployable.
- `bUseRegularDrain` / `DrainMagnitudePerSecond`; `bAutoRecallAtEndLife`; `bPushActorsOnSpawn` (fight-spawned only; redirects to `FightCenter`; runs at the end of the init phase when there is one).
- `bUnlimitedDeploy` (default true, class CDO, exempt from slot limit); `bDestroyOldestWhenLimitReached`.
- `bShowDamageNumbers` (implements `IGeoDamageNumberHost`; false = HUD never registers it).
- `bSuppressDrainDamageVisuals`: drain cue only.

## FX
- Never multicast recall/expiry cues: clients fire locally from `OnRep_Active`.
- Sounds: per-BP `SoundMap` (`EDeployableSoundType`: Spawn / Blinking / Recall / Explode / Expire) of `FGeoSoundEntry`, played by `PlaySoundOneShot` next to that moment's `ExecuteCue`; cue tags are VFX only.
- `CombattantWidgetComponent` auto-created from `GameDataSettings::CombattantWidgetComponentClass` on `WidgetAnchorComponent`; null on dedicated server; per-BP `HealthBarDrawSize` / `Location` / `WidgetClassOverride`.

## ASC replication
- `AGeoInteractableActor`'s ASC is `Minimal` but a deployable is net-owned by its deployer: that client receives neither replicated cue `WhileActive` nor tags for its own deployables (works on the listen server and on other players' deployables).
- Read deployable state through replicated members, not tags. Cue visuals spawn from both `OnActive` and `WhileActive` behind an idempotent guard. Target fix: `Mixed` on that ASC. Detail: `TroobleshootingLogs.md`.
- Every concrete subclass overrides `GetData()` returning a non-null pointer to its own data `UPROPERTY` struct (`FTurretData`, `FBuffPickupData`, …): `Tick`, `GetDurationPercent`, `InitInteractableData` dereference it unchecked. Audit every subclass when touching the pattern. Spawn with `SpawnActorDeferred` + `FinishSpawning`.
