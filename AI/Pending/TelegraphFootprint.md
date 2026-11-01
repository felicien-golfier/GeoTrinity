# TelegraphFootprint — pending

Goal: a hazard's telegraph never grows: it shows its whole footprint and safe spots from its first frame, with no stray frame; only a fill moves, over the wind-up, in a direction picked per case (telegraph and live hazard apart).
Priority: 1
Docs written: `Source/GeoTrinity/Public/AbilitySystem/Abilities/Pattern/Hazard/CLAUDE.md` ("Beam" telegraph rule + fill modes + placed-before-shown, "DevastatingWave", "LateralWave"), `.../Hazard/SafeSpot/CLAUDE.md` (`Behind` in the zone layer), `Source/GeoTrinity/Public/World/ZoneLayer/CLAUDE.md` (`FillMode`), `AI/Materials.md`, `AI/Tutorial/Steps.md` (step 8).

## Decided (2026-10-30)
- Fill runs over the wind-up (`StartDelay`), not at the hazard's speed: equal speed = equal time, the designer sets it.
- Fill direction = `EGeoFillMode` (`FromCore`, `AlongBeam`, `None`) set per Blueprint: `TelegraphFillMode` / `HazardFillMode` on `UBeamPattern`, `TelegraphFillMode` on `UDevastatingWavePattern`. Rules choosing them come later (user).
- Supersedes `LateralWaveVisual.md` "growing rectangle" (its wind-up row).

## Code
- [x] `EGeoFillMode` + `FillAlongFlag` (`GeoZoneShape.h/.cpp`), `FillMode` on `UGeoZoneVisualComponent`; `None` packs fill 0.
- [x] `BeamPattern` — placed (`MoveBeamVfx(-StartDelay)`) before shown; `TelegraphFillMode` / `HazardFillMode`.
- [x] `LateralWavePattern` — mask cleared before `Super::InitPattern`; full rectangle in the wind-up; constructor sets both fills `AlongBeam`.
- [x] `FSafeSpotMask::Set` — `Origin` → `SafeSpot_OriginWS` (`GeoMaterialParams::SafeSpotOrigin`).
- [x] `DevastatingWavePattern` — zone-layer `Circle` telegraph (`TelegraphVisual`), Niagara telegraph + `TelegraphFadeOutDuration` + `TelegraphBlinkTimerHandle` removed. `GeoNiagaraParams` entries all still used by the live wave: kept.
- [x] `GeoBeamVFXComponent.h/.cpp` — `FBeamVFXState` + `FillMode`, `FillStartServerTime`, `FillDuration`; `SetBeamState` takes them; `ApplyBeamState` sets `FillMode` + `SetTimedFill`.
- [x] `GeoChannelBeamAbility.h/.cpp` — `BeamFillMode` UPROPERTY (default `AlongBeam`), virtual `GetChannelDuration()` (0); `PushBeamState` passes mode, `ServerSpawnTime + GetFireDelay()`, `GetChannelDuration()`.
- [x] `GeoSacrificeBeamAbility` — `GetChannelDuration()` = `MaxChannelDuration`.
- [x] `GeoMoiraBeamAbility` — `ChannelDuration` member: `InitialDuration` at `Fire`, `+= DurationPerAbsorbedZone * DrainRatio` beside `RemainingDuration`; `GetChannelDuration()` returns it.
- Indicators (`FGeoIndicatorState`): stay `FromCore` (user, 2026-10-30).

## Scripts
- [x] `zone_layer_hlsl.py` — `AlongBeam` branch on `FillAlongFlag`, shade `Core < Fill * Extent` (fill 0 shades nothing).
- [x] `make_zone_layer_materials.py` — `Behind` in `MF_SafeSpotSlotCut` / `MF_SafeSpotCut` from `SafeSpot_OriginWS`; adds it to `MPC_MaskedArea`.
- [x] Run `make_zone_layer_materials.py` (headless, shaders compiled; rebuilt `M_ZoneLayer`, `MF_SafeSpotCut`, `MPC_MaskedArea`).

## Editor / Blueprint
- [x] `BP_DevastatingWavePattern`: `StartDelay` = `GA_DevastatingWave` `FireDelay` 1.1 s: positive fill time.
- [ ] Per-Blueprint `TelegraphFillMode` / `HazardFillMode` as the user decides (hex beams: default `FromCore`).

## Tests
- [x] `GeoZoneLayerTests.cpp` (`GeoTrinity.ZoneLayer.PackSlot`) — `FillAlongFlag` for `AlongBeam` only; `None` packs fill 0.
- [x] `GeoZoneLayerTests.cpp` (`GeoTrinity.ZoneLayer.BeamFill`) — `UGeoBeamVFXComponent::SetBeamState` with a fill: its shape's `GetFill()` = spent / duration, and re-timed by a longer duration.
- Pattern-instance tests (placed before shown, devastating telegraph) skipped: no test builds a pattern with its ability; the tutorial wave rooms cover the lateral wave live.
- [x] Build, `GeoTrinity.ZoneLayer` + `GeoTrinity.Pattern` (9/9), `GeoTrinity.Tutorial.` set incl. `FullRun` (32/32), 2026-10-30.

## Verify
- [ ] User check: no flash before the wave telegraph; full rectangle with lanes cut from frame 1; fill crosses start→far edge; devastating wave telegraph full circle with pillar shadows, fill from the centre (no PIE unless asked).

## Status 2026-11-01
2 unticked item(s) remain; they need the editor, Blueprint/PIE, a build or test run, or a user decision, so none can be verified from the repo alone.
