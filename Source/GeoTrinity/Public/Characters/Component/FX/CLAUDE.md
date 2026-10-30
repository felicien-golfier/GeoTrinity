# Characters/Component/FX

Presentation components. Cosmetic only; never gameplay state.

| File | Role |
|---|---|
| `GeoFXComponent.h` | Plays sustained/burst moments (`FGeoVFXParams`): system attached to owner + loop beside it; `ApplyFXParams` is the one push |
| `GeoBeamVFXComponent.h` | Replicated beam state holder, added/removed by beam abilities (`OnGiveAbility` / `OnRemoveAbility`); draws through a `UGeoZoneVisualComponent` |
| `GeoIndicatorComponent.h` | Telegraphs: `SpawnIndicator`, fast-array of what/where/colours/when; each drawn as a `Telegraph` zone shape |
| `GeoLoopSoundComponent.h` | Layered seamless loops bent by owner's montages (boss fight loops); driven by `GeoLoopSoundNotify` |
| `GeoDeploySatelliteComponent.h` | Class deploy-satellite ring; `SetParams` pushed on every class change from `FPlayerClassData` |

## Beam VFX
- `SetBeamState(bActive, HalfWidth, Length, FillMode, FillStartServerTime, FillDuration)` writes replicated `FBeamVFXState`; server write replicates, owning client may also write for lag-free visuals.
- Each rendering machine creates a local `UGeoZoneVisualComponent` (`Beam`, `Ground`) on the owner root, so it turns with the aim; `ApplyBeamState()` sets its shape and shows / hides it. No fade (`World/ZoneLayer/CLAUDE.md`).
- Colours: `SetBeamColors(Color, SecondaryColors)` → replicated `ColorMask` (`GeoZoneLayer::MakeColorMask`).
- Fill = the channel's time spent: `FBeamVFXState` also carries `FillMode` (`EGeoFillMode`), `FillStartServerTime` and `FillDuration`; `ApplyBeamState()` passes them to the shape (`SetTimedFill`), so every machine fills off the server clock. A new `FillDuration` (Moira's absorbed fuel) re-times the fill at once: it reads `spent / new total`. `FillDuration` 0 = no fill.
- Windup: a `Ray` on the owner's `UGeoIndicatorComponent`, not this component.

## Indicators
- `FGeoIndicatorState::ColorMask` (one bit per `EGeoColor`), replicated; `Size` = `Round` radius, `Ray` length + half-width.
- `SpawnIndicator` returns the `UGeoZoneVisualComponent` (`Round` → `Circle`, `Ray` → `Beam`; layer `Telegraph`, `SetTimedFill(StartServerTime, Duration)`), owned by the attach parent's actor or else the world; it destroys itself once `Duration` is over (a world timer). Caller destroys it to end early.
