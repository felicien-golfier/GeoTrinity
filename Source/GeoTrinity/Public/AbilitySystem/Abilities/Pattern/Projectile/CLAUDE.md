# Abilities/Pattern/Projectile

Patterns that spawn `AGeoProjectile`s. Rules (lag compensation, instance rule) in `../CLAUDE.md`.

| File | Role |
|---|---|
| `SpiralPattern.h` | Expanding circular sprays (`NumberProjectileByRound`, `TimeForOneRound`, `RoundNumber`); owns and drives its projectiles |
| `ConeSprayPattern.h` | `SalveNumber` salves `SalveFrequencySec` apart, fanned over `ConeAngle` (hex boss) |
| `ProjectilePattern.h` | `UProjectilePattern` base (`SpawnSalve`) + `USalvePattern`: one bullet per target location |

## Spiral
- Config via one `ProjectileParams` (`FExternalProjectileParams`); requests projectiles from the pool directly and calls `ApplyProjectileParams` before `Init()`.
- Ticks by `SetActorLocation`; re-check `Projectiles[i] != Projectile` after the move, and `bPatternIsActive` at loop top.
- Ends projectiles on actor destruction.

## ConeSpray
- Fan centred on `GetTargetYaw()`.
- Each salve stamped with its scheduled `ServerSpawnTime` (`spawn + StartDelay + N*SalveFrequencySec`), not tick time.
- Ends once the last salve is out; bullets fly on their own.
- `OnCreate` pre-warms the pool; reset `SpawnedSalveCount` in `InitPattern`.

## Salve
- One bullet per target location, yaw = `GetTargetYaw(i)` (`../CLAUDE.md`, "Pattern.h"); no pattern data. The locations are resolved on the server and travel with the pattern start, since a client only knows its own controller.
