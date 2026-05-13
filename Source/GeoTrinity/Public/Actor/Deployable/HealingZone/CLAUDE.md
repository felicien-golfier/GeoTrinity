# Actor/Deployable/HealingZone

Circle's area-heal deployable.

## Files
| File | Role |
|---|---|
| `GeoHealingZone.h/.cpp` | Static HoT zone — heals allies in range, drains only while healing |

## Key Points
- `bUseRegularDrain = false` — zone drains health proportionally to targets healed per tick, not on a fixed timer
- `ActorsInZone` (TSet) tracks overlapping actors via `OnBeginOverlap` / `OnEndOverlap`
- `Tick()` — server-only; skips hostiles and full-health targets; applies `FHealEffectData` per target, then drains zone by `DrainMagnitudePerSecond * DeltaSeconds * HealedNum`
- `GetDurationPercent()` override — returns health ratio (no duration fallback)
- `OnRep_Data()` — updates capsule size on clients when initial data replicates
