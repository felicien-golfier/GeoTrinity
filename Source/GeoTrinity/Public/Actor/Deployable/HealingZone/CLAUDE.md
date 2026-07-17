# Actor/Deployable/HealingZone

Circle's area-heal deployable.

## Files
| File | Role |
|---|---|
| `GeoHealingZone.h/.cpp` | Static HoT zone — heals allies in range; drains on a fixed timer AND extra per healed target |

## Key Points
- `bUseRegularDrain = true` (base default) — base class's fixed-timer drain always applies, regardless of occupancy
- `AGeoHealingZone::Tick()` adds extra drain of `DrainMagnitudePerSecond * DeltaSeconds * HealedNum` on top, scaling with players healed
- `ActorsInZone` (TSet) tracks overlapping actors via `OnBeginOverlap` / `OnEndOverlap`
- `Tick()` — server-only; skips hostiles and full-health targets; applies `FHealEffectData` per target, then drains zone by `DrainMagnitudePerSecond * DeltaSeconds * HealedNum`
- `GetDurationPercent()` override — returns health ratio (no duration fallback)
- `OnRep_Data()` — updates capsule size on clients when initial data replicates
