# DevastatingWaveZone — pending

Goal: the devastating wave's live ring is drawn by the zone layer like the lateral wave (same parent code for shape, colour, fills, safe spot occlusion), in the lethal colour; the ring becomes a zone shape.
Priority: 2
Docs written: `Source/GeoTrinity/Public/AbilitySystem/Abilities/Pattern/Hazard/CLAUDE.md` ("ZoneHazard", "Beam", "DevastatingWave", "LateralWave"), `.../Hazard/SafeSpot/CLAUDE.md`, `Source/GeoTrinity/Public/World/ZoneLayer/CLAUDE.md` (`Ring`), `Source/GeoTrinity/Public/Tool/Visual/CLAUDE.md`, `AI/Materials.md` ("Safe spots").

## Decided (2026-11-01)
- New shape `EGeoZoneShape::Ring`, not a `Circle` inner radius (`InnerRadius` 0 on a circle would break its distance in the inner half).
- Ring `FromCore` fill: inner radius → outer radius.
- Shared code = a parent class (`UZoneHazardPattern`), not a helper.
- Only one ring hazard for now; a second one later = child of `UDevastatingWavePattern`.

## Code
- [x] `GeoZoneShape.h/.cpp` — `Ring` (ordinal 2, after `Beam`), `FGeoZoneShape::InnerRadius`; `PackSlot` row 1 = (centre X, Y, `Radius`, `InnerRadius`).
- [x] `ZoneHazardPattern.h/.cpp` (new, `Pattern/Hazard/`) — moved from `UBeamPattern` exactly (`CodingStyle.md` "Class hierarchy"): the visual (`BeamVisual` → `HazardVisual`), `OnCreate` creation, `InitPattern` telegraph, `StartPattern` ground switch, `TickDuringInit`, `OnHazardEnd` / `EndPattern` hide, `TelegraphFillMode` / `HazardFillMode`; `MoveBeamVfx` → virtual `MoveHazardVisual` (empty); `TickPattern` calls it. `BeamColor` / `SecondaryBeamColors` → `HazardColor` (constructor: `Damage`) / `SecondaryHazardColors`. New: `MaskMaterialParameterCollection` (moved up from both waves), `SetSafeSpots(Spots, LaneDirection, Origin)`, `ClearSafeSpots()`, mask cleared at the top of `InitPattern` and in `EndPattern`. Telegraph gated on `StartSectionTimerHandle.IsValid()` (devastating's guard; the beam had none).
- [x] `BeamPattern.h/.cpp` — parent `UZoneHazardPattern`; keeps aim (`MoveHazardVisual`, shape `Beam`, `Length` / `HalfWidth` in `InitPattern` before `Super`'s placement or in `MoveHazardVisual`), debug in `TickPattern` + `Super`.
- [x] `LateralWavePattern.h/.cpp` — `MoveBeamVfx` → `MoveHazardVisual`, mask upload via `SetSafeSpots`; `EndPattern` override, own MPC member and the clear in `InitPattern` removed; constructor `HazardColor` = `LethalDamage`.
- [x] `DevastatingWavePattern.h/.cpp` — parent `UZoneHazardPattern`; `MoveHazardVisual` = `Circle` `MaxRadius` (wind-up) / `Ring` front + `AnnulusWidth` (live); `TelegraphVisual`, `AOEVfxSystem`, `AOEVfxComponent`, `AOEColor`, `FadeOutDuration`, `ActivateAOEVfx`, own `OnCreate` / `OnHazardEnd` removed; `ClearData` → `PillarSpots.Empty()` + `ClearSafeSpots()`; `AddPillarSpots` → `SetSafeSpots(PillarSpots, ZeroVector, Origin)`; constructor `HazardColor` = `LethalDamage`, both fills `FromCore`.
- [x] `GeoNiagaraParams.h` — remove `AOERadius`, `AOEGrowDuration`, `AOEColor`, `AnnulusRadius`, `FadeOutDuration` and their 5 `Get*ParameterName` (no other user, no Content referencer).
- [x] `Config/DefaultEngine.ini` `[CoreRedirects]` — `BeamPattern.BeamColor` → `ZoneHazardPattern.HazardColor`, `BeamPattern.SecondaryBeamColors` → `ZoneHazardPattern.SecondaryHazardColors`, `DevastatingWavePattern.AOEColor` → `ZoneHazardPattern.HazardColor`.

## Scripts
- [x] `AI/Python/Material/zone_layer_hlsl.py` — `Ring` branch (`Shape == 2`): `d = length(P - C)`; distance `max(d - Outer, Inner - d)`, `Inner <= 0` → `d - Outer`; `Core = d - Inner`, `Extent = Outer - Inner`.
- [x] Run `make_zone_layer_materials.py` headless (rebuilds `M_ZoneLayer` + functions).
- [x] `AI/Python/Tutorial/Level/tutorial_waves.py` — `BeamColor` → `HazardColor` (line 91).

## Editor / Blueprint
- [x] MF move (2026-11-01): `MF_SafeSpotCut`, `MF_SafeSpotSlotCut` (its slot function), `MF_TelegraphColors` → `/Game/Art/VFX/ZoneLayer/Functions`; `AOE/Functions` deleted; builders repointed. Colours before/after: `AI/Output/hazard_colors_before.txt`, `AI/Output/hazard_zone_assets.txt`.
- [x] Before the build: record `BeamColor` / `SecondaryBeamColors` / `AOEColor` of `BP_SweepBeamPattern`, `BP_TileCarveRayPattern`, `BP_LateralWave`, `BP_DevastatingWavePattern`; after: same values in `HazardColor` (redirects), resave the four.
- [x] `BP_DevastatingWavePattern` — `HazardColor` = `LethalDamage`, resave (drops `AOEVfxSystem`).
- [x] Delete `/Game/Art/VFX/Abilities/Enemies/PillarsAOE/NS_PillarsAOE` once unreferenced. Keep `M_MAskedArea` (layout model, `AI/Materials.md`) and `MPC_MaskedArea`.

## Tests
- [x] `GeoZoneLayerTests.cpp` (`GeoTrinity.ZoneLayer.PackSlot`) — `Ring`: shape ordinal 2, row 1 = centre, outer, inner.
- Ring radii / pattern parent: no test builds a pattern with its ability; the tutorial covers the lateral wave and the miniboss live.
- [x] Build; `Tools\Run_Tests.bat GeoTrinity.ZoneLayer`, `GeoTrinity.Pattern` (9/9 green); `GeoTrinity.Tutorial.` set (34/34 green).

## Verify
- [ ] User check: wave telegraph full circle with pillar shadows; live ring in `LethalDamage`, pillar shadows cut, band = hit band, shading inner → outer; lateral wave and hex beams unchanged.
