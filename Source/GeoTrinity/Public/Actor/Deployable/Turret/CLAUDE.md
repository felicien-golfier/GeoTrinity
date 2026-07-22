# Actor/Deployable/Turret

Triangle's deployable auto-firing turret.

## Files
| File | Role |
|---|---|
| `GeoTurret.h/.cpp` | Auto-targeting turret — fires at nearest hostile on a timer |

## Key Points
- `FindBestTarget()` prefers the owner's `LastBasicAbilityTarget` (set by ExecCalc_Damage via `bIsFromBasicAbility`) so the turret chases the same enemy the Triangle player is attacking; falls back to nearest hostile in range.
- `CurrentTarget` — replicated, updated each Tick; clients orient the mesh toward it.
- `TryFire()` — server-only, scheduled by `ScheduleFire()`.
- `Expire()` override clears the fire timer before `Super::Expire()`.
- Deployed via `GeoDeployAbility` → `ADeployableSpawnerProjectile`; recalled by `GeoRecallTurretAbility`.
