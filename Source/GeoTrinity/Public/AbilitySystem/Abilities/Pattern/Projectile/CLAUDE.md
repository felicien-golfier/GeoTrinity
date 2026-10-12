# Abilities/Pattern/Projectile

Patterns that spawn `AGeoProjectile`s. Rules (lag compensation, instance rule) in `../CLAUDE.md`.

| File | Role |
|---|---|
| `SpiralPattern.h` | Expanding circular sprays (`NumberProjectileByRound`, `TimeForOneRound`, `RoundNumber`); owns and drives its projectiles |
| `ConeSprayPattern.h` | `SalveNumber` salves `SalveFrequencySec` apart, fanned over `ConeAngle` (hex boss) |
| `ProjectilePattern.h` | `USalvePattern` + data: one salve's yaws, resolved on server by the launching ability |

## Spiral
- Config via one `ProjectileParams` (`FExternalProjectileParams`); requests projectiles from the pool directly and calls `ApplyProjectileParams` before `Init()`.
- Ticks by `SetActorLocation`; re-check `Projectiles[i] != Projectile` after the move, and `bPatternIsActive` at loop top.
- Ends projectiles on actor destruction.

## ConeSpray
- Each salve stamped with its scheduled `ServerSpawnTime` (`spawn + StartDelay + N*SalveFrequencySec`), not tick time.
- Ends once the last salve is out; bullets fly on their own.
- `OnCreate` pre-warms the pool; reset `SpawnedSalveCount` in `InitPattern`.

## Salve
- Whole salve (every yaw) travels with the pattern data; a client only knows its own controller.
