# Actor/Turret

## `GeoTurret.h` — Triangle's deployable auto-firing turret

Extends `AGeoDeployableBase`.

- `TurretProjectileClass` — projectile to fire; must be a valid `AGeoProjectile` subclass
- `FireInterval = 1s` — seconds between shots
- `FindBestTarget()` — returns nearest hostile actor in range (uses `GetAllAgentsWithRelationTowardsActor` with `Hostile` attitude)
- `TryFire()` — called on timer; if target exists, spawns `TurretProjectileClass` toward it
- `ScheduleFire()` — sets up the repeating fire timer
- `Expire()` override — clears fire timer before calling base `Expire()`

Deployed by Triangle via `GeoDeployAbility` → `ADeployableSpawnerProjectile` → `InitDeployable()`.
Recalled by `GeoRecallTurretAbility`.
