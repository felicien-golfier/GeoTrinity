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
- `ImpactEffect` (Niagara), `ImpactSound`, `StartSound`, `LoopingSound` — cosmetic only; all three skip dedicated servers (`!GeoLib::IsDedicatedServer`). `StartSound` plays once at spawn; `LoopingSound` loops for the projectile's full lifetime.

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
- `OnWallBounce()` — bound to ProjectileMovement bounce delegate

## `DeployableSpawner/DeployableSpawnerProjectile.h` — spawns a deployable on impact
- `IsValidOverlap()` — only ground/static geometry triggers deployment
- `EndProjectileLife()` — calls `SpawnDeployableActor`, then destroys the projectile
- `SpawnDeployableActor(PayloadOwner)` — delegates to `GeoASLib::StartSpawnDeployable` → `InitDeployable` → `GeoASLib::FinishSpawnDeployable`
- `InitDeployable(Deployable)` — override point for subclass to configure class-specific `FDeployableData` fields before `FinishSpawning`
- `FillBaseData(Data)` — fills owner, level, team, seed, params, effects into `FDeployableData` from the projectile payload
