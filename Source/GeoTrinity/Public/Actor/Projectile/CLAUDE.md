# Actor/Projectile

All projectile classes. **Default base for any new projectile: `AGeoPooledProjectile`** — never `AGeoProjectile` directly unless explicitly non-pooled.

## `GeoProjectile.h` — base
Poolable, effect-applying projectile.

**Lifecycle:**
1. `InitProjectileLife()` — binds hit/overlap delegates, starts lifespan timer, records initial position, applies movement. Called by pool `Init()` on both machines; called in `BeginPlay` on clients for non-pooled replicated projectiles (server re-applies movement separately via `InitProjectileMovementComponent`).
2. `AdvanceProjectile(float TimeDelta)` — fast-forwards position to compensate for server-spawn lag (called after spawning with `ServerSpawnTime`)
3. `EndProjectileLife()` — triggered by distance span, lifespan, or hit; destroys by default

**Hit handling:**
- `OnSphereOverlap()` — private; calls `IsValidOverlap()` then `HandleValidOverlap()`
- `IsValidOverlap(OtherActor)` — override to restrict targeting (team check, type check)
- `HandleValidOverlap(OtherActor)` — override for custom hit logic; default applies `EffectDataArray` to target
- `OnProjectileHit(HitActor)` — `BlueprintNativeEvent`; fired on all clients after a valid hit

**Network:**
- `IsNetRelevantFor()` — returns false for the **owning client** (they keep their predicted copy)
- `PredictionKeyId` — replicated; used to match server projectile to client's predicted one

**Key fields:**
- `EffectDataArray` — effects applied on hit
- `Payload` (`FAbilityPayload`) — owner/instigator/ability info
- `DistanceSpan = 1000 cm` — override with `OverrideDistanceSpan(float)`
- `LifeSpanInSec = 30` — safeguard max lifespan
- `bCanOverlapInstigator = false`, `LifeTimeThresholdBeforeOverlapSelf = 0.2` — prevent self-hit on spawn
- `ImpactEffect` (Niagara) — cosmetic only.
- **Sounds**: `SoundMap` (`TMap<EProjectileSoundType, FProjectileSoundEntry>`) maps `Start`, `Looping`, `Hit`, and `End` to a `FProjectileSoundEntry` (`Sound` + `Volume` + attribute-driven pitch: `PitchAttribute`, `PitchCurve`, `RandomPitchMultiplierRange`). `Start` plays once at spawn, `Looping` loops for the projectile's full lifetime (both skip dedicated servers). `PlayImpactFx` plays `EndSoundType` — `Hit` when `HandleValidOverlap` ended the projectile on a valid target, `End` otherwise (wall hit, distance span, lifespan; reset in `InitProjectileLife`); a `CoreRedirects` enum redirect maps the old `Impact` name to `End`. `PlayImpactFx` is called from `EndProjectileLife` on authority; `Destroyed()` is the non-authority fallback, gated on `bIsEnding` so clients that already ended locally don't play it twice — a client that never detected the overlap locally falls back to `End`. One-shots go through `PlaySoundOneShot(SoundType)` (map lookup) or `PlaySoundOneShot(FProjectileSoundEntry const&)` for custom entries outside the enum; `GetPitch` has the same enum/entry split: `GetPitch(SoundType)` (BlueprintNativeEvent) looks up SoundMap and forwards to `GetPitch(Entry)`, which reads the instigator's ASC for `PitchAttribute`, samples `PitchCurve`, then multiplies by a random value in the range; returns 1.0 if SoundMap has no entry. Override `GetPitch` in Blueprint for custom logic.

## `GeoPooledProjectile.h` — pooled variant
Extends `GeoProjectile` + implements `IGeoPoolableInterface`:
- `Init()` — reset state, enable collision
- `End()` — disable collision, return to `UGeoActorPoolingSubsystem`
- `EndProjectileLife()` — releases to pool instead of destroying

## `GeoShieldBurstProjectile.h` — Square passive
Bounces off enemies, grants shield on ally contact.
- `ShieldAmount` (`FScalableFloat`) — base shield per burst; scales with ability level and multiplied by each enemy bounce
- `BounceSnapshot` (`FShieldBounceSnapshot`) — replicated; teleports all clients to post-bounce state
- `HandleValidOverlap()` — reflect on enemy, shield ally, end on ally contact
- `IsValidOverlap()` — returns false for `AGeoWall` (the burst passes through the Square's own walls without bouncing); also suppresses the same hostile within 0.5 s to avoid double-hit on glancing overlaps
- `OnWallBounce()` — bound to ProjectileMovement bounce delegate on all machines; server records `BounceSnapshot` (no sound — wall bounces are silent).
- `BounceSound` (`FProjectileSoundEntry`) — dedicated bounce sound, separate from the SoundMap Hit/End entries; played only on enemy bounces in `HandleValidOverlap`
- `GetPitch(FProjectileSoundEntry const&)` — overridden (base made this `virtual`) to multiply `Super::GetPitch` by `BounceSoundSizePitchCurve` evaluated at the sphere's current scaled radius, so bigger bursts pitch every sound on this projectile (including `BounceSound`) differently

## `DeployableSpawner/DeployableSpawnerProjectile.h` — spawns a deployable on impact
- `IsValidOverlap()` — only ground/static geometry triggers deployment
- `EndProjectileLife()` — calls `SpawnDeployableActor`, then destroys the projectile
- `SpawnDeployableActor(PayloadOwner)` — delegates to `GeoASLib::StartSpawnDeployable` → `InitDeployable` → `GeoASLib::FinishSpawnDeployable`
- `InitDeployable(Deployable)` — override point for subclass to configure class-specific `FDeployableData` fields before `FinishSpawning`
- `FillBaseData(Data)` — fills owner, level, team, seed, params, effects into `FDeployableData` from the projectile payload
