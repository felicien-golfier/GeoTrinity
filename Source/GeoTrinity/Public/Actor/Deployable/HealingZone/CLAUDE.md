# Actor/Deployable/HealingZone

Circle's area-heal deployable.

## Files
| File | Role |
|---|---|
| `GeoHealingZone.h/.cpp` | Static HoT zone — heals allies in range; drains on a fixed timer AND extra per healed target |

## Key Points
- `bUseRegularDrain = true` — fixed-timer drain always applies regardless of occupancy; `Tick()` (server-only) adds extra drain of `DrainMagnitudePerSecond * DeltaSeconds * HealedNum` on top, scaling with players healed.
- `ActorsInZone` (TSet) tracks overlaps; `Tick()` skips hostiles/full-health targets and applies `FHealEffectData` per healed target.
- `GetDurationPercent()` returns health ratio (no duration fallback).
- `OnRep_Data()` syncs capsule size on clients.
