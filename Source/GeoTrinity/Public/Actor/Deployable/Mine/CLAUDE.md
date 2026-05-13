# Actor/Deployable/Mine

Square's proximity mine deployable.

## Files
| File | Role |
|---|---|
| `GeoMine.h/.cpp` | Proximity mine — explodes on capsule overlap |

## Key Points
- `bUseRegularDrain = false` — no passive drain; triggered by overlap only
- `bIsRecalling` prevents double-trigger on simultaneous overlaps
- `Recall(Value)` — server-only; sphere overlap, applies `FDamageEffectData` to hostiles and `FShieldEffectData` to friendlies, scaled by `MineData.Params.Value * Value`
- `GetRecallCueParams()` override — passes `MineData.Params.Size` as `RawMagnitude` for explosion VFX radius
