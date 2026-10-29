# PillarDrop — pending

Goal: the boss pillar drops in from above during the pattern's telegraph, solid only once it lands.
Priority: 2
Docs written: `Actor/Deployable/CLAUDE.md` ("Init phase"), `Actor/Deployable/Pillar/CLAUDE.md`, `AbilitySystem/Abilities/Pattern/Hazard/CLAUDE.md` ("SpawnPillar").

## Code (to implement)
- [x] `GeoDeployableBase.h/.cpp` — `Params.InitDuration`, `InitMontage`, `InitStartOffset`, `InitCurve`, `InitStartServerTime`, `IsInitialising()`, init start / tick / finish; push + collision timer moved to BeginPlay / `FinishInit`.
- [x] `GeoPillar.cpp` — constructor sets `InitStartOffset`.
- [x] `SpawnPillarPattern.h/.cpp` — spawn in `InitPattern` with `InitDuration`; force-stop expires initialising pillars.
- [x] Test `GeoTrinity.Gameplay.Deployable.InitPhase` in `GeoDeployableInitTests.cpp`; row in `Tests/Gameplay/CLAUDE.md`.

## Editor / Blueprint (by hand)
- [x] `BP_Pillar_Deployable` — `InitCurve` = `CF_PillarDrop` (1 → 0, ease-in), `InitCue` = `GameplayCue.Generic.ZoneIndicator` (Damage).
- [ ] `BP_Pillar_Deployable` — mesh = the look wanted (no bomb mesh exists); optional `InitMontage` on a skeletal mesh; `FXMap[Init]` sounds; tune `InitStartOffset`.

## Verify
- [x] `ToolsRun_Tests.bat GeoTrinity.Gameplay.Deployable` + `GeoTrinity.Tutorial.` (31 green)
