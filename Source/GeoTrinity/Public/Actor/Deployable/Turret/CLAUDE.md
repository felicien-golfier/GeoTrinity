# Actor/Deployable/Turret

Triangle's deployable auto-firing turret.

## Files
| File | Role |
|---|---|
| `GeoTurret.h/.cpp` | Auto-targeting turret — fires at nearest hostile on a timer |

## Key Points
- `TurretProjectileClass` — projectile BP to spawn; must be a `AGeoProjectile` subclass
- `FireInterval` — seconds between shots (configured in BP)
- `FindBestTarget()` — first checks the owner's `LastBasicAbilityTarget` (set by ExecCalc_Damage via `bIsFromBasicAbility`); falls back to nearest hostile in range. Prefers the owner's focus target so the turret chases the same enemy the Triangle player is attacking.
- `CurrentTarget` — replicated; updated each Tick from `FindBestTarget()`. Clients use it to orient the turret mesh toward the live target location.
- `TryFire()` — spawns `TurretProjectileClass` toward target; scheduled by `ScheduleFire()`. Server-only (gated by `IsServer`).
- `Expire()` override — clears fire timer before calling `Super::Expire()`
- Deployed by Triangle via `GeoDeployAbility` → `ADeployableSpawnerProjectile`; recalled by `GeoRecallTurretAbility`
