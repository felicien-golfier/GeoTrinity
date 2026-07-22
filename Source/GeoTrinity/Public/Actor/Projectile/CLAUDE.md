# Actor/Projectile

All projectile classes. **Default base for any new projectile: `AGeoPooledProjectile`** — never `AGeoProjectile` directly unless explicitly non-pooled.

## `GeoProjectile.h` — base
Poolable, effect-applying projectile.

**Lifecycle:** `InitProjectileLife()` binds hit/overlap, starts lifespan timer, applies movement (called by pool `Init()` on both machines; also from `BeginPlay` on clients for non-pooled replicated projectiles) → `AdvanceProjectile(TimeDelta)` fast-forwards position to compensate spawn lag → `EndProjectileLife()` (distance span / lifespan / hit) destroys by default.

**Hit handling:** `OnSphereOverlap()` (private) → `IsValidOverlap()` (override to restrict targeting) → `HandleValidOverlap()` (override for custom hit logic; default applies `EffectDataArray`). `OnProjectileHit()` is `BlueprintNativeEvent`, fired on all clients.

**Instigator revive:** binds to instigator's `OnRevived` (if a GeoCharacter) and ends the projectile on revive; unbound via `UnbindFromInstigatorRevive` from both `EndProjectileLife` and pool `End()` so a reused projectile never keeps a stale binding.

**Network:** `IsNetRelevantFor()` returns false for the owning client (they keep their predicted copy); `PredictionKeyId` matches server projectile to client's predicted one.

**Key fields:** `EffectDataArray`, `Payload` (`FAbilityPayload`), `DistanceSpan` (override via `OverrideDistanceSpan` or `GameDataSettings::GeneralSpellDistance`), Speed (`bUseGeneralSpellSpeed` default on, or `OverrideSpeed`), `LifeSpanInSec = 30` safeguard, `bCanOverlapInstigator = false` + `LifeTimeThresholdBeforeOverlapSelf` (self-hit guard), `ImpactEffect` (cosmetic Niagara).

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
