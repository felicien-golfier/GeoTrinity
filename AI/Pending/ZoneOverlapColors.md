# ZoneOverlapColors — pending

Goal: overlapping zones, beams and telegraphs mix their colours where they overlap (shield beam over a heal zone shows both, two heal zones read brighter), drawn by one generic layer instead of one material per effect, with room for more shapes.
Priority: 1
Docs written: `Source/GeoTrinity/Public/World/ZoneLayer/CLAUDE.md` (new), `World/CLAUDE.md`, `Public/CLAUDE.md`, `Characters/Component/FX/CLAUDE.md`, `Actor/Deployable/CLAUDE.md`, `AbilitySystem/Abilities/Pattern/Hazard/CLAUDE.md`, `Hazard/SafeSpot/CLAUDE.md`, `Tool/Visual/CLAUDE.md`, `AI/Materials.md` ("Zone layer", "Safe spots"), `AI/VFX.md`, `AI/Python/Material/CLAUDE.md`, `AI/MCP/MCP_Material.md` ("Custom (HLSL) nodes"), `AI/Commands.md` ("Headless commandlets").

## Code (to implement)
- [x] `World/ZoneLayer/` — shape, visual component, subsystem, layer actor
- [x] `Settings/GameDataSettings.h` — `ZoneLayerGroundMaterial`, `ZoneLayerTelegraphMaterial`; `RayIndicatorSystem`, `RoundIndicatorSystem` removed (`Config/DefaultGame.ini` updated)
- [x] `GeoEffectZone`, `GeoBeamVFXComponent`, `GeoIndicatorComponent`, `BeamPattern`, `LateralWavePattern`, `ZonePattern`, `GeoZoneIndicatorCue`, `GeoChannelBeamAbility` (`BeamNiagaraSystem`, `GetBeamDuration` gone)
- [x] `GeoNiagaraParams.h` — beam / safe-spot / zone names and `ApplySwappableAsset` removed; `MeaningColors` / `ColorCount` kept for `SetCueMeaningColors`
- [x] Override grep: no zone, beam, zone pattern, deploy-zone or summoned-dummy Blueprint uses `Override`
- [x] `AGeoEffectZone`: hand-placed `Color` starts `Neutral` (constructor), so a zone dropped in a level draws without an ensure
- [x] Tutorial map: the Zones room's Shield `UGeoSpawnZoneAction` was `Override` → `DamageReduction` (its effect, blue), map and `tutorial_encounters.py` `ZONE_COLORS`; no hand-placed `AGeoEffectZone` in Tutorial, DraftMap's `Tuto_Drain` is `Damage`

## Scripts
- [x] `zone_layer_hlsl.py`, `make_zone_layer_materials.py`, `make_color_pattern_functions.py` (knobs + `MF_TelegraphColors`); old builders deleted; dry run green; built headless (`M_ZoneLayer` 460 pixel instructions, no error)

## Editor / Blueprint (by hand or script)
- [x] `BP_Zone`, `BP_Deployable_HealingZone`: `MI_PulseCircle` mesh removed (no other zone Blueprint had one)
- [x] Beam ability / pattern Blueprints resaved (old `BeamNiagaraSystem` / `BeamVfxSystem` values dropped)
- [ ] Decide: `NS_Beam`, `NS_Ray_ZoneIndicator`, `NS_Round_ZoneIndicator` (and their materials `MI_PulseBeam`, `M_PulseBeam`, `M_ZoneIndicator`, `M_ZoneIndicatorRay`, `MF_BeamFrame`, `MF_BeamPulse`, `MF_InsideFrameOnAxis`, `MF_ColorScale`) are still the base systems `AI/Python/Niagara/sacrifice_hole_vfx.py`, `sacrifice_release_vfx.py`, `teleport_portal_vfx.py` duplicate. Keep them as templates, or rebase those scripts and delete the lot (then `AI/Python/Niagara/material_user_bindings.py` and its row go too: its three systems are all in this list)
- [x] `/Game/Maps/DraftMap` resaved; `MI_PulseCircle`, `M_PulseCircle`, `MF_ZonePulse` deleted (no referencer). `MF_PulseBrightness` (used by `M_PulseBeam`, `MF_BeamPulse`) and `MF_ZoneShape` (used by `M_ZoneIndicator`) go with the decision above

## Tests
- [x] `Source/GeoTrinityEditor/Private/Tests/Gameplay/GeoZoneLayerTests.cpp` (`PackSlot`, `ColorMask`, `Registry`, `IndicatorExpiry`)
- [x] Test fakes given palette colours (they spawned `Override` zones): `AGeoTestSummonedDummy::ZoneParams`, the four zone setups in `GeoEncounterTests.cpp` (spawn-zone actions, readable zone)
- [x] `Tools\Run_Tests.bat GeoTrinity.Tutorial.` alone: 31/31, no `has no palette slot` in `run.log`. `GeoTrinity.`: 146/147, the only palette ensure is the expected one in `ZoneLayer.ColorMask`; `Tutorial.Path.MinibossCallouts` flaky (ally dies at 20 %, 2 fails in 5 runs, gameplay untouched by this feature): the user investigates

## Verify
- [x] Build Editor target (last build includes every change above)
- [ ] User checks in game: shield beam over heal zone mixes; two heal zones brighter; telegraph stripes above; lateral wave lanes cut; look knobs (`FillOpacity`, `FillBrightness` on the instances, `StackBrightness`, `FillShade`, `OutlineWidth` in `MPC_MeaningColors`) tuned
