# SafeSpots — pending

Goal: every boss hazard gets its safe spots from one code system (spots in code, VFX reads them), as the devastating wave does with pillars; the lateral wave becomes a `UBeamPattern` child that is only a pattern + spot locations.
Docs written: `Source/GeoTrinity/Public/AbilitySystem/Abilities/Pattern/Hazard/CLAUDE.md`, `.../Hazard/SafeSpot/CLAUDE.md`, `Source/GeoTrinity/Public/CLAUDE.md` (tests line).
Supersedes the earlier lane design (`GetDangerousIntervals` / strips gone; the former MinibossWaveSpots file was deleted 2026-10-25, its leftover is in Editor below).

## Code (to implement)
- [x] `Hazard/SafeSpot/SafeSpot.h/.cpp` — `FSafeSpot`, `ESafeSpotShadow`, `Shields(Location, Origin, Direction)` (Behind = former `IsBehindPillar` maths, Lane = strip through the spot)
- [x] `Hazard/SafeSpot/SafeSpotMask.h/.cpp` — `FSafeSpotMask::Set / Clear` (move `ClearData`, `AddPillarToVfxMask`, `MaxMaskedPillarSlots`, sentinel out of `DevastatingWavePattern.cpp`); also writes the shadow-mode scalar
- [x] `Hazard/SafeSpot/SafeSpotSource.h/.cpp` — `ESafeSpotSource`, `GatherSafeSpots` (Pillars, Adds via `AGeoArena::GetLiveAdds()`, Fixed)
- [x] `DevastatingWavePattern.h/.cpp` — `FPillarWaveData` / `PillarsWaveData` → `FSafeSpot` + `FSafeSpotMask`; `IsBehindPillar` → `Shields`; end-of-wave pillar kill reads the same list (pillar weak ptr kept beside the spot); behaviour unchanged
- [x] `BeamPattern.h` — `MoveBeamVfx` virtual, VFX component protected (origin / yaw stay non-virtual: the child lays its own rectangle); no behaviour change
- [x] `LateralWavePattern.h/.cpp` — rebase on `UBeamPattern`; drop strips, `MaxStrips`, `GetDangerousIntervals`, `SafeLaneWidth`, `FixedLanes`, `bLanesFromAdds`; add `SafeSpotSources`, fixed offsets, `SpotRadius`; override `IsInHazard`, `MoveBeamVfx`, `GetHazardDuration`, `TickPattern` (mask refresh)
- [x] Hazard `GeoPatternTests.cpp` — replace `LateralWaveLanes` (below)

## Editor / Blueprint (by hand or script)
- [ ] Beam material used by `BP_LateralWave_*`: read `MPC_MaskedArea` slots, cut `Lane` shadows (parallel to the beam) out of the band and the indicator; probe how `NS_PillarsAOE`'s material does `Behind` first, reuse its function (`AI/MCP/MCP_Material.md`)
- [x] (script updated, not run: `FixedSpotOffsets` (0, lane), `SafeSpotSources`, `SpotRadius`, `BeamColor`, `MaskMaterialParameterCollection`) `tutorial_waves.py` — `BP_LateralWave_MinibossWave` : new parent, `SafeSpotSources = Adds`; step 9 waves: `Fixed` offsets, no `FixedLanes`; `MPC_MaskedArea` assigned on the pattern
- [ ] Existing `BP_LateralWave_*` children keep `TravelYaw`, `Speed`, `TravelDistance`, `BandWidth`, `HalfWidth`; they lose `WaveColor` (now the base's `BeamColor`), `BeamVfxSystem` (same name, now the base's), `FixedLanes`, `SafeLaneWidth`, `bLanesFromAdds`: rerun `tutorial_waves.py` (editor) to set the new ones

## Tests (`AI/CodingStyle.md` "Tests")
- [x] `GeoTrinity.Pattern.SafeSpotShadow` — `Behind`: before the spot not shielded, behind within radius shielded, beside not; `Lane`: shielded on both sides of the spot, outside width not; zero spots shield nothing
- [x] `GeoTrinity.Pattern.SafeSpotMask` — spots over 8 skipped; `Clear` leaves the sentinel (needs no world: pure slot list builder, MPC write apart)

## Verify
- [x] Build (`Tools\*.bat`, `GEO_NO_PAUSE=1`, editor closed), `Tools\Run_Tests.bat GeoTrinity.Pattern`; no PIE unless asked
- [ ] Devastating wave unchanged: hit test + pillar kill covered by the existing pattern tests, VFX by the user in game

## Open (my choice, say if wrong)
- Lateral keeps its travelling front (band), now drawn by the beam VFX instead of one component per strip
- Lateral spots shield on both sides of the spot (`Lane`); devastating pillars only behind (`Behind`)
- One live mask user at a time (the MPC is global)

- [ ] (from deleted MinibossWaveSpots.md) list `GA_TutorialWave_MinibossWave` / `BP_LateralWave_MinibossWave` in `BP_TutorialMiniboss`, fired by the state tree event (`Tutorial.md` slice 6)

## Status 2026-10-25
Code and tests verified. Remaining: beam material reading `MPC_MaskedArea`, run `tutorial_waves.py`, set properties on `BP_LateralWave_*` children, check devastating wave unchanged in game, `ClearData` still on `UDevastatingWavePattern` (check it is a thin wrapper).
