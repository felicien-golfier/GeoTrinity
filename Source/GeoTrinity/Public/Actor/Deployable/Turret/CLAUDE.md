# Actor/Deployable/Turret

`GeoTurret.h` — Triangle auto-firing turret; deployed by `GeoDeployAbility`, recalled by `GeoRecallTurretAbility`.

- `FindBestTarget()`: player turret = the owner's `LastBasicAbilityTarget` if still valid, hostile and in range, else `nullptr`. It never picks the nearest enemy itself: it waits for the owner to hit someone first, then follows that target; target dead / out of range = idle again until the owner's next basic hit. Non-player (enemy-owned) turrets keep the nearest hostile in range (their owner has no basic-ability target).
- No target = no shot and no rotation (`TryFire()` returns).
- `CurrentTarget` replicated; clients orient the mesh.
- `TryFire()` server-only via `ScheduleFire()`; spawns `ProjectileParams` (`FExternalProjectileParams`).
- `Expire()` clears the fire timer before `Super`.
