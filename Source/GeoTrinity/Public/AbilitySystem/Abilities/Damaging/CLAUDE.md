# Abilities/Damaging

Projectile-firing ability bases.

| File | Extend for |
|---|---|
| `GeoProjectileAbility.h` | Single shot. `Fire()` client predicted, `OnFireTargetDataReceived()` server |
| `GeoAutomaticFireAbility.h` | Hold-to-fire loop (abstract). Implement `ExecuteShot()` |
| `GeoAutomaticProjectileAbility.h` | Concrete auto-fire spawning `ProjectileParams.ProjectileClass` |

- One `ProjectileParams` (`FExternalProjectileParams`) member carries class + overrides (`Actor/Projectile/CLAUDE.md`).
- `SpawnProjectile()` virtual; `SpawnProjectilesUsingTarget()` = one projectile per `GeoASLib::GetTargetDirections()` of the base's `Target` (`Base/CLAUDE.md`, "Target"), from the fire origin.
- Auto-fire: repeating timer at `FireDelay`; cost committed per shot (client `Fire()`, or server in `OnFireTargetDataReceived`; host skips the RPC).
- Auto-fire reports the per-shot timer from `GetCooldownTimeRemainingAndDuration` for the ability-bar sweep.
- Game feel: `FireCameraShakeClass`, `RecoilDistance`, `FireGameplayCueTag`.
