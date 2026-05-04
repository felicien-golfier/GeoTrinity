# Actor/Deployable

All deployable actors. **Base class: `AGeoDeployableBase`** — extend this for new deployables.

## `GeoDeployableBase.h` — abstract base
Handles drain GE, blink-before-expiry, recall, and Blueprint events.

**Lifecycle:**
1. Spawned by `ADeployableSpawnerProjectile::EndProjectileLife()` on server
2. `InitDrain()` — applies drain GE from `Params.LifeDrainMaxDuration`; override to change drain behavior
3. `Tick()` — manages blink timer countdown
4. On health/duration zero → `Expire()` → sets `bExpired`, starts `TimeBeforeDestroyAtExpire = 3s` timer, fires `OnDeployableExpiredEvent`
5. `Recall(bExecuteCue, Value)` — owner-initiated; **this is the override point** for "deployable triggers effect and ends". Override `Recall()`, not a separate method.

**Blueprint events:**
- `OnBlinkStarted()` — `BlueprintNativeEvent`; pre-expiry visual (blink animation)
- `OnDeployableExpired()` not listed — use `OnDeployableExpiredEvent` delegate instead

**VFX rule:** Never multicast RPC for recall/expiry VFX. Use `RecallGameplayCueTag` with `ExecuteRecallCue()`.

**Key fields:**
- `bUseRegularDrain`, `DrainMagnitudePerSecond` — drain config
- `bSuppressDrainDamageVisuals = true` — hides hit flash during drain ticks
- `GetDurationPercent()` — health ratio 0..1; drives health bar
- `GetData()` — pure virtual; subclasses return their `FDeployableData`

**`FDeployableDataParams`** (passed from `GeoDeployAbility`):
- `BlinkDuration`, `LifeDrainMaxDuration`, `Size`, `Value`

**`FDeployableData`** (extends `FInteractableActorData`):
- `EffectDataArray`, `Params`, `PredictionKey`

## `GeoMine.h` — Square's proximity mine
- Triggers on capsule overlap → calls `Recall(true, Value)` to explode
- `bIsRecalling` prevents double-trigger
- `GetRecallCueParams()` — override; passes explosion force data to GC

## `GeoHealingZone.h` — Circle's area heal
- Tracks `ActorsInZone` (TSet) via `OnBeginOverlap` / `OnEndOverlap`
- `Tick()` — periodically applies heal effects to all actors in zone
- `GetDurationPercent()` — override; uses drain GE remaining duration
- `OnRep_Data()` — updates zone size/visuals when data replicates

## `GeoBuffPickup.h` — Triangle's buff pickup
- Launched toward `TargetLocation` via `LaunchCurve`
- `MeshIndex` selects which `BuffMeshAssets` to display
- `PowerScale` scales buff effect magnitude
- `OnOverlap()` — applies `EffectDataArray` to collector, calls `Recall(false)`

## `GeoTurret.h` (in `Turret/`)
Triangle's deployable auto-firing turret.
- `FindBestTarget()` — nearest hostile in range
- `TryFire()` — spawns `TurretProjectileClass` toward target; called on `FireInterval` timer
- `Expire()` override — cleans up fire timer
