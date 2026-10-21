# MinibossWaveSpots — pending

Goal: the miniboss's lateral wave has no random safe spots; each living dummy (summoned or ally) makes a safe lane at its place from the wave's start, so the fight pushes the player to keep dummies alive.
Docs written: `Source/GeoTrinity/Public/AbilitySystem/Abilities/Pattern/Hazard/CLAUDE.md` ("LateralWave"), `AI/Tutorial/Steps.md` (step 10).

## Code (to implement)
- [x] `AbilitySystem/Abilities/Pattern/Hazard/LateralWavePattern.h/.cpp` — `UTickablePattern`; `Speed`, `SafeLaneWidth`, `FixedLanes`, `bLanesFromAdds`; lanes from the instigator's `Arena` adds' alive state; swept hit test per `Pattern/CLAUDE.md` "Lag compensation"; lane mask VFX (reuse the pillar-shadow MPC slots or the AOE mask, whichever fits)
- [x] Arena accessor (`GetLiveAdds`, doc row written) for the living adds if none exists (`Actor/Arena/GeoArena.h`), then its row in `Actor/Arena/CLAUDE.md`
- [x] `Hazard/CLAUDE.md` section rewritten to the built pattern

## Editor / Blueprint
- [x] (`tutorial_waves.py`: `GA_TutorialWave_MinibossWave` + `BP_LateralWave_MinibossWave`; left: list it in `BP_TutorialMiniboss`, fired by the state tree event) Ability BP + pattern BP child `bLanesFromAdds` true, listed in `BP_TutorialMiniboss` and fired by the state tree event after N spawned dummies (`AI/Pending/Tutorial.md` slice 6); step 9 waves (slice 5) reuse the pattern in `FixedLanes` mode

## Tests (`AI/CodingStyle.md` "Tests")
- [x] `GeoTrinity.Pattern.LateralWaveLanes` (`GeoPatternTests.cpp`, 1/1; lane maths only, no arena): pure lane maths — lanes from add positions, a dead add drops its lane, zero adds = no lane, target in / out of a lane

## Verify
- [x] Build, then `Tools\Run_Tests.bat GeoTrinity.Pattern`; no PIE unless asked

## Open (my choice, say if wrong)
- Safe spot = lane along the wave's travel through the dummy (matches "safe lines"), not a circle around it
