# Actor/Projectile

All projectile classes. **Default base for any new projectile: `AGeoPooledProjectile`** — never `AGeoProjectile` directly unless explicitly non-pooled.

## `GeoProjectileParams.h` — spawn-params bundle
`FGeoProjectileParams` bundles `ProjectileClass` with one `EOverrideParam` toggle + value per tunable (distance/speed + cosmetic radius/head+trail color/trail-lifetime), so a class always travels with the values it spawns with. `EOverrideParam` = `UseGameDataSettings` / `KeepBlueprintDefaultValue` / `OverrideValue`. Distance/speed/radius default to `UseGameDataSettings` (resolve to `GameDataSettings::GeneralSpellDistance`/`GeneralSpellSpeed`/`GeneralProjectileRadius`); head/trail color + trail-lifetime default to `KeepBlueprintDefaultValue` and have **no** settings value, so their `UseGameDataSettings` just falls back to the Blueprint default. Every spawn site holds one `FGeoProjectileParams` member (no loose class + `bOverride*` fields), and `GeoASLib::StartSpawnProjectile`/`FullySpawnProjectile` take **only** the struct (no raw class overload). `SpiralPattern`, which requests from the pool directly, calls `Projectile->ApplyProjectileParams(Params)` itself.

## `GeoProjectile.h` — base
Poolable, effect-applying projectile.

**Lifecycle:** `InitProjectileLife()` binds hit/overlap, starts lifespan timer, applies movement (called by pool `Init()` on both machines; also from `BeginPlay` on clients for non-pooled replicated projectiles) → `AdvanceProjectile(TimeDelta)` fast-forwards position to compensate spawn lag → `EndProjectileLife()` (distance span / lifespan / hit) destroys by default.

**Hit handling:** `OnSphereOverlap()` (private) → `IsValidOverlap()` (override to restrict targeting) → `HandleValidOverlap()` (override for custom hit logic; default applies `EffectDataArray`). `OnProjectileHit()` is `BlueprintNativeEvent`, fired on all clients.

**Instigator revive:** binds to instigator's `OnRevived` (if a GeoCharacter) and ends the projectile on revive; unbound via `UnbindFromInstigatorRevive` from both `EndProjectileLife` and pool `End()` so a reused projectile never keeps a stale binding.

**Network:** `IsNetRelevantFor()` returns false for the owning client (they keep their predicted copy); `PredictionKeyId` matches server projectile to client's predicted one.

**Key fields:** `EffectDataArray`, `Payload` (`FAbilityPayload`), `DistanceSpan` (Blueprint-default distance; `KeepBlueprintDefaultValue` uses it, `OverrideDistanceSpan` overwrites it — also the client-side fallback since distance isn't replicated), `ReplicatedSpeed` (0 = keep movement component's own `InitialSpeed`, else replicated resolved speed set by `OverrideSpeed`), `LifeSpanInSec = 30` safeguard, `bCanOverlapInstigator = false` + `LifeTimeThresholdBeforeOverlapSelf` (self-hit guard), `ImpactEffect` (cosmetic Niagara). The projectile has **no** own use-general-settings toggle — the spawn site's `FGeoProjectileParams` `EOverrideParam` is the single mechanism (settings / Blueprint default / override).

**`BulletVFX` + cosmetics:** `BulletVFX` is a **native** `UNiagaraComponent` subobject; its system asset (`NS_GeoTrinity_Projectile01`) is set in the C++ constructor (BP may override the asset field — but **not** its Niagara User Params: those never populate on an inherited component, which is why the look is C++-driven, not asset-driven). The component is **write-only** from C++ — nothing reads its user params back. The per-Blueprint default look lives in `FProjectileCosmeticParams DefaultCosmetics` (edited in Class Defaults). `ApplyCosmetics(FProjectileCosmeticParams)` is the single write path: it resizes the `Sphere` collider and pushes the four user params `User.Bullet_Radius`/`Bullet_HeadColor`/`Bullet_TrailColor`/`Trail_LifetimeScale`. `ApplyProjectileParams(FGeoProjectileParams)` resolves each value via its `EOverrideParam` (settings / `DefaultCosmetics` / explicit override) into a resolved `FProjectileCosmeticParams` and calls `ApplyCosmetics`. `FGeoProjectileParams` **derives** from `FProjectileCosmeticParams` (values shared, one declaration) and adds the per-value toggles + distance/speed; inherited cosmetic values can't carry `EditConditionHides` so they always show. Simulated proxies (which never run `ApplyProjectileParams`) apply `DefaultCosmetics` in `BeginPlay`. Editor-only `PreviewCosmetics()` (`CallInEditor` button) pushes `DefaultCosmetics` + `ReinitializeSystem()` so the look is visible without PIE — click it on a placed instance / BP preview actor, not Class Defaults. `AGeoShieldBurstProjectile` drives `BulletVFX` directly for its per-bounce radius (always valid now that it's a native subobject).

**Sounds:** `SoundMap` maps `Start`/`Looping`/`NoOverlapEnd`/`ValidOverlapEnd` to `FGeoSoundEntry`. Audience gating/volume/pitch logic lives in `UGeoSoundRowLibrary`. `PlayImpactFx` (called from `EndProjectileLife` on authority, or `Destroyed()` as non-authority fallback gated on `bIsEnding`) picks `ValidOverlapEnd` vs `NoOverlapEnd` based on how the projectile ended.

## `GeoPooledProjectile.h`
Implements `IGeoPoolableInterface`: `Init()`/`End()` reset/release via `UGeoActorPoolingSubsystem`; `EndProjectileLife()` releases to pool instead of destroying.

## `GeoShieldBurstProjectile.h` — Square passive
Bounces off enemies, grants shield on ally contact.
- `ShieldAmount` scales with level and multiplies per bounce; `BounceSnapshot` (replicated) teleports clients to post-bounce state.
- `IsValidOverlap()` — passes through `AGeoWall` (no bounce); suppresses same hostile within 0.5s (glancing-overlap guard).
- `OnWallBounce()` bound on all machines; server records `BounceSnapshot` (bounces are silent — no sound).
- `GetPitch()` multiplies `Super` by `BounceSoundSizePitchCurve` evaluated at current sphere radius, so bigger bursts pitch differently.

## `DeployableSpawner/DeployableSpawnerProjectile.h`
Spawns a deployable on impact. `IsValidOverlap()` — only ground/static geometry triggers. `EndProjectileLife()` calls `SpawnDeployableActor` then destroys. `InitDeployable(Deployable)` is the subclass override point for class-specific `FDeployableData` fields before `FinishSpawning`.
