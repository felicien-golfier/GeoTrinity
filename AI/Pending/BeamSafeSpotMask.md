# BeamSafeSpotMask — pending

Goal: the lateral wave is visible again and shows its safe lanes: the base beam materials (`M_PulseBeam`, `M_ZoneIndicatorRay`) read the safe-spot mask like the devastating wave's AOE material, instead of code-laid segments. A beam safe spot is a location + radius: `Disc` (default, round hole) or `Lane` (beam cut across its width, lateral wave).
Docs written: `Source/GeoTrinity/Public/AbilitySystem/Abilities/Pattern/Hazard/CLAUDE.md` ("LateralWave"), `.../Hazard/SafeSpot/CLAUDE.md` ("Mask"), `AI/Materials.md` ("Safe spots on a beam"), `AI/Python/Material/CLAUDE.md`, `Source/GeoTrinity/Public/CLAUDE.md` (tests line).
Replaces the code-laid segments of the former `LateralWaveRuntimeSpots` plan (done and deleted: spots given per firing through `IPatternDataSource`, one `BP_LateralWave`).
Not established: why the lateral wave is invisible now (editor was down). Step 0 below.

## Step 0 — read before building (editor needed)
- [ ] (only the `SafeSpot_ShadowLane` check is done: no asset references it) Read `/Game/Art/VFX/Abilities/Enemies/PillarsAOE/MF_MaskArea` and `M_MAskedArea` (hand-made, no script): how they read `MPC_MaskedArea` slots and the sentinel. Reuse the slot loop if it fits `MF_SafeSpotCut`; check it does not read `SafeSpot_ShadowLane` (renamed below); if it already reads slot alpha, keep it, else alpha 1 = used (`FSafeSpotMask::GetSlotValues`).
- [ ] Find why nothing is drawn: in game, `Geo.DrawBeamBorder` shows the hit rectangle; check `BP_LateralWave` `BeamVfxSystem` / `IndicatorSystem` are set (`tutorial_waves.py` does not set them?), and that `M_PulseBeam`'s `DurationSpent` (dynamic param B, written by `NS_Beam`) is not wiping the rotated live band to 0. Fix whichever it is here, document if lasting.

## Code (to implement)
- [x] `Hazard/SafeSpot/SafeSpot.h/.cpp` — `ESafeSpotShadow::Disc` (ordinals fixed: Behind 0, Lane 1, Disc 2); `FSafeSpot::Shadow` defaults to `Disc`; `Shields` Disc case = `distance(Target, Location) <= Radius`; devastating / lateral already pass their shadow to `GatherSafeSpots`
- [x] `Hazard/SafeSpot/SafeSpotMask.h/.cpp` — `Set(WorldContext, Collection, Spots, LaneDirection)` writes the first spot's shadow ordinal to `SafeSpot_ShadowMode` (was the 0/1 `SafeSpot_ShadowLane`) and `SafeSpot_LaneDirection` (XY, unit; `Clear` writes zero); `GetSlotValues` unchanged (alpha 1 used, 0 free)
- [x] `Tool/Visual/GeoNiagaraParams.h` — `GeoMaterialParams::SafeSpotShadowMode` (`SafeSpot_ShadowMode`, replaces `SafeSpotShadowLane`) and `SafeSpotLaneDirection` (`SafeSpot_LaneDirection`), comment updated
- [x] `Hazard/LateralWavePattern.h/.cpp` — drop `GetBandSegments`, `ExtraSegmentComponents`, the `InitPattern` / `StartPattern` / `OnHazardEnd` segment loops; `MoveBeamVfx` lays ONE rectangle (wind-up: origin, `Forward`, `TravelDistance` × `2 * HalfWidth`; live: the band, direction `Right`) and calls `FSafeSpotMask::Set(this, MaskMaterialParameterCollection, GetSafeSpots(), GetForward())`; new `MaskMaterialParameterCollection` property (`UMaterialParameterCollection`, `EditDefaultsOnly`, `GeoLateralWave|VFX`); `Clear` in `InitPattern` and `EndPattern` (every machine except dedicated: `Set` already skips it)
- [x] `Hazard/BeamPattern.h/.cpp` — `ApplyBeamLook` goes back inline into `InitPattern` / `StartPattern` if nothing else calls it (extracted only for the segments)

## Scripts
- [x] (run headless, ok) `AI/Python/Material/make_beam_and_telegraph_materials.py` — new `MF_SafeSpotCut` in `FOLDER/Functions` (input `WorldXY`; loops the 8 `PillarPosWS_XX` of `MPC_MaskedArea`, alpha = used; distance = `Disc` (mode 2): `length(P - Spot.xy)`, `Lane` (mode 1): `abs(dot(P - Spot.xy, (-Dir.y, Dir.x)))`; `CutMask` = max over slots of `distance <= Pillar_Radius`, 0 for `Behind` (mode 0)); `M_PulseBeam` emissive and `M_ZoneIndicatorRay` emissive + opacity × `1 - CutMask`; dry-run first (`dry_run_material_build.py`), then run headless; rebuilt in place
- [x] (run headless, ok; `BeamVfxSystem` already `NS_Beam`; `encounters_diff.py` needs the editor bridge, still to run) `AI/Python/Tutorial/Level/tutorial_waves.py` — `BP_LateralWave`: `MaskMaterialParameterCollection` = `/Game/Art/VFX/Abilities/Enemies/PillarsAOE/MPC_MaskedArea`; `BeamVfxSystem` / `IndicatorSystem` set if step 0 finds them empty; then `encounters_diff.py` = `identical`

## Editor / Blueprint (by hand or script)
- [x] (done by the material script, checked: no asset reads `SafeSpot_ShadowLane`) `MPC_MaskedArea` — add vector `SafeSpot_LaneDirection` default (0, 0, 0, 0); scalar `SafeSpot_ShadowMode` default 0 (rename `SafeSpot_ShadowLane` if it exists) (one-off, `mcp-epic`)

## Tests (`AI/CodingStyle.md` "Tests")
- [x] (green) `GeoTrinity.Pattern.LateralWaveSegments` (`GeoPatternTests.cpp`) — removed with `GetBandSegments`
- [x] (green) `GeoTrinity.Pattern.SafeSpotShadow` (extend) — `Disc`: inside the radius shielded, outside not, direction and origin ignored
- [x] (green) `GeoTrinity.Pattern.SafeSpotMask` (extend) — used slots alpha 1, free slots alpha 0; `Set` with no collection and `Clear` are no-ops
- [x] (104 / 104, FullRun green) Run `Tools\Run_Tests.bat GeoTrinity.` incl. `GeoTrinity.Tutorial.FullRun` (tutorial gate); build editor first (`GEO_NO_PAUSE=1`, editor closed)

## Verify
- [ ] Other beams unchanged (Moira, hex sweep, tile-carve ray): all slots alpha 0 → `CutMask` 0
- [ ] Devastating wave unchanged: it writes `Behind` (mode 0), `MF_SafeSpotCut` returns 0, its own material unchanged
- [ ] User in game: wind-up rectangle and live band both show the lanes as gaps, readable in the band and the telegraph; a dead add's lane closes
- [ ] `Disc` shown on a beam (a spot with no direction): round hole in `M_PulseBeam` / `M_ZoneIndicatorRay`; no pattern uses it yet, a bridge script uploads one spot to check
