# Actor/Projectile

Default base for any new projectile: `AGeoPooledProjectile`.

| File | Role |
|---|---|
| `ExternalProjectileParams.h` | `FProjectileParamsBase` (every projectile value) + `FExternalProjectileParams` (class + override toggles) |
| `GeoProjectile.h` | Base: lifecycle, hits, replication, params, sounds |
| `GeoPooledProjectile.h` | `IGeoPoolableInterface`: `Init()` / `End()`, releases to pool instead of destroying |
| `GeoShieldBurstProjectile.h` | Square passive: bounces off enemies, shields allies |
| `GeoProjectileFXComponent.h` | Per-moment projectile FX |
| `DeployableSpawner/DeployableSpawnerProjectile.h` | Spawns a deployable on ground impact (`InitDeployable` hook before `FinishSpawning`) |

## Params
- `FProjectileParamsBase`: `DistanceSpan`, `ProjectileSpeed`, `Radius`, head/trail colour (`FGeoColorParam`), `TrailLifetimeScale`, `SoundMap`, `OverlapAttitude`, `bCanOverlapInstigator`, `LifeTimeThresholdBeforeOverlapSelf`.
- `FExternalProjectileParams` adds `ProjectileClass` + one `EOverrideParam` per value: `UseGameDataSettings` | `KeepBlueprintDefaultValue` | `OverrideValue`.
- New param = field with `meta = (OverrideToggle = "Override<X>")` in base + toggle in derived + one line in `ApplyProjectileParams`.
- Settings values only for distance / speed / radius (`GeneralSpellDistance` / `GeneralSpellSpeed` / `GeneralProjectileRadius`); others default to Blueprint value.
- Spawn sites hold one `FExternalProjectileParams` member; `GeoASLib::StartSpawnProjectile` / `FullySpawnProjectile` take only the struct.
- Callers that pool-request directly call `ApplyProjectileParams` before `Init()`.
- Detail panel hiding: `FExternalProjectileParamsCustomization` (`GeoTrinityEditor/Detail`).

## GeoProjectile
- Lifecycle: `InitProjectileLife()` (pool `Init()` on both machines; `BeginPlay` on clients for non-pooled) → `AdvanceProjectile(TimeDelta)` → `EndProjectileLife()`.
- `bIsEnding` guards double end; every `EndProjectileLife` override sets it.
- Hits: `OnSphereOverlap` → `IsValidOverlap()` (restrict) → `HandleValidOverlap()` (default applies `EffectDataArray`); `OnProjectileHit` BP event on all clients.
- Binds instigator `OnRevived` and ends on revive; unbind in both end paths.
- Net: `IsNetRelevantFor` false for owning client (predicted copy); `PredictionKeyId` pairs server and client.
- Fields: `EffectDataArray`, `Payload`, `ReplicatedSpeed` (0 = movement component `InitialSpeed`), `LifeSpanInSec = 30`, `ImpactEffect`.
- Params: `DefaultParams` (Class Defaults, never written at runtime) and `ResolvedParams` (transient). Read `ResolvedParams` for behaviour.
  - `ApplyParams(FProjectileParamsBase)` is the single write path: stores, resizes `Sphere`, pushes Niagara user params via `GeoNiagaraParams`.
  - `ApplyProjectileParams(FExternalProjectileParams)` resolves toggles then `ApplyParams`; `OverrideSpeed` also touches movement + `ReplicatedSpeed`.
  - Simulated proxies apply `DefaultParams` in `BeginPlay`; anything replicated to fix a param client-side re-applies in `InitProjectileLife`.
- `BulletVFX` is a native Niagara subobject (asset set in constructor), write-only from C++; editor preview via `PostEditChangeProperty` → `PreviewParams()` (forwards to archetype instances).
- Sounds: `ResolvedParams.SoundMap` (`Start` / `Looping` / `NoOverlapEnd` / `ValidOverlapEnd`); `PlayImpactFx` picks by how it ended.

## Pooled
- `End()` → `BulletVFX->DeactivateImmediate()`; `Init()` re-`Activate(true)` after `ApplyProjectileParams`.

## Shield burst
- `ShieldAmount` scales with level and per bounce; replicated `BounceSnapshot` (location, velocity, radius), seeded at spawn in `InitProjectileLife` and re-applied on clients.
- `IsValidOverlap`: passes through `AGeoWall`; same hostile suppressed 0.5 s.
- `GetPitch()` × `BounceSoundSizePitchCurve(radius)`.
