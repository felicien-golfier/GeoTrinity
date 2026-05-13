# Actor/Deployable/Turret

Triangle's deployable auto-firing turret.

## Files
| File | Role |
|---|---|
| `GeoTurret.h/.cpp` | Auto-targeting turret — fires at nearest hostile on a timer |

## Key Points
- `TurretProjectileClass` — projectile BP to spawn; must be a `AGeoProjectile` subclass
- `FireInterval` — seconds between shots (configured in BP)
- `FindBestTarget()` — returns nearest hostile in range using `GetAllAgentsWithRelationTowardsActor` with `Hostile` attitude
- `TryFire()` — spawns `TurretProjectileClass` toward target; scheduled by `ScheduleFire()`
- `Expire()` override — clears fire timer before calling `Super::Expire()`
- Deployed by Triangle via `GeoDeployAbility` → `ADeployableSpawnerProjectile`; recalled by `GeoRecallTurretAbility`
