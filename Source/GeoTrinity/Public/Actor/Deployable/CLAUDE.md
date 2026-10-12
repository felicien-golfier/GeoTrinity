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
| `Zones/` | `GeoEffectZone`: editor-placed capsule applying `EffectDataArray`; owns its ASC; heal/damage re-applied per tick (per-second magnitude), other types once on entry/exit |

## Lifecycle
- Spawned server-side by `ADeployableSpawnerProjectile::EndProjectileLife()` → `InitDrain()` → server `Tick()` drains → zero health/duration starts blink → `Expire()` (hide, event, delayed destroy).
- `Recall(Value)` is the only end-of-life path: `RecallEffect(Value)` then `Expire()`; non-server also fires the recall cue locally.
- On hex arenas the fall-check recalls/expires anything over a dead tile; subclasses with recall side effects must account for it.

## Override points
- `RecallEffect(Value)`: server end-of-life behaviour (default none).
- `ExplodeEffect(...)`: default applies `EffectDataArray` within `Params.Size`, lag-compensated through `JudgeExplosion` (`FGeoLagCompensatedEvent`, driven by `SetTimerForNextTick`); `TimeBeforeDestroyAtExpire` must outlive 2 × `MaxLatencyCompensation`.
- `Explode(Value)`: server sphere overlap, `ExplodeAttitude` (default Hostile), `ExplodeOverlapMode` (Automatic = centre-only for Enemy casters).

## Data and fields
- `FDeployableData : FInteractableActorData`: `EffectDataArray`, `Params` (`BlinkDuration`, `LifeDrainMaxDuration`, `Size`, `Value`), `PredictionKey`, `AbilityTag`.
- Replicated `*Data` is `COND_InitialOnly` on every concrete deployable.
- `bUseRegularDrain` / `DrainMagnitudePerSecond`; `bAutoRecallAtEndLife`; `bPushActorsOnSpawn` (fight-spawned only; redirects to `FightCenter`).
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
