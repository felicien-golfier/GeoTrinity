# Actor/Deployable/Turret

`GeoTurret.h` — Triangle auto-firing turret; deployed by `GeoDeployAbility`, recalled by `GeoRecallTurretAbility`.

- `FindBestTarget()`: owner's `LastBasicAbilityTarget` first, else nearest hostile in range.
- `CurrentTarget` replicated; clients orient the mesh.
- `TryFire()` server-only via `ScheduleFire()`; spawns `ProjectileParams` (`FExternalProjectileParams`).
- `Expire()` clears the fire timer before `Super`.
