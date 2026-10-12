# Characters/Component/FX

Presentation components. Cosmetic only; never gameplay state.

| File | Role |
|---|---|
| `GeoFXComponent.h` | Plays sustained/burst moments (`FGeoVFXParams`): system attached to owner + loop beside it; `ApplyFXParams` is the one push |
| `GeoBeamVFXComponent.h` | Replicated beam VFX holder, added/removed by beam abilities (`OnGiveAbility` / `OnRemoveAbility`) |
| `GeoIndicatorComponent.h` | Telegraphs: `SpawnIndicator`, fast-array of what/where/colours/when |
| `GeoLoopSoundComponent.h` | Layered seamless loops bent by owner's montages (boss fight loops); driven by `GeoLoopSoundNotify` |
| `GeoDeploySatelliteComponent.h` | Class deploy-satellite ring; `SetParams` pushed on every class change from `FPlayerClassData` |

## Beam VFX
- `SetBeamState(bActive, HalfWidth, Length, bIsIndicator, IndicatorLifetime)` writes replicated `FBeamVFXState`; server write replicates, owning client may also write for lag-free visuals.
- Each rendering machine spawns a local non-replicated `UNiagaraComponent` on the owner root; created in the `BeamSystem` RepNotify, which ends with `ApplyBeamState()`.
- `BeamSystem` authored local-space +X; `SetBeamColor` replicates `COND_None`.
- Windup preview: `IndicatorSystem` from `GameDataSettings::BeamPreviewSystem` (not replicated); `ApplySwappableAsset` swaps asset only on change.
