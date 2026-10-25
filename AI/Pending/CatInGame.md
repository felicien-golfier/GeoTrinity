# CatInGame — pending

Goal: the 3D Cat and its clips are in the game: in the Landing room it reassembles, idles talking to the player, walks while it speaks, roams the room once movement is free, and on its last line portal-jumps into the exit pad and vanishes; an encounter action drives it.
Docs written: `Source/GeoTrinity/Public/Actor/CLAUDE.md` ("GeoCat"), `Encounter/Action/CLAUDE.md` (`GeoCatAction`), `Source/GeoTrinity/Public/CLAUDE.md`, `AI/Tutorial/Cat.md` ("In game"), `CatAnimation.md` (owner), `Steps.md` (step 1 "Cat staging"), `AI/Python/Anim/CLAUDE.md`, `AI/Python/Level/CLAUDE.md`.
Depends on: `CatAnimation.md` "Alive pass" clips re-run in the editor (the current assets are the earlier pass; the actor works with either).

## Code
- [x] `Actor/GeoCat.h/.cpp` — `AGeoCat` (`AActor`, not replicated): `Root` scene + `Mesh` (`USkeletalMeshComponent`, no collision); `bStartHidden` (true), `WalkSpeed` (150 placeholder, tuned against the stride), `TurnRate` (deg/s), `SpeedBlendRate`, `bFacePlayer` (true), `RoamArea` (`TObjectPtr<AActor>`), `RoamMargin` (150), `RoamPause` (`FVector2D` 1.5–4 s), `PortalDistance` (110), `ReassembleMontage`, `PortalJumpMontage`; commands `Reassemble`, `Idle`, `WalkPath(TArray<AActor*>)`, `Roam`, `PortalJump(AActor*)`; native `OnCommandEnded(bool bCompleted)`; `AdvanceMove(float)` public, called by `Tick`; `Speed` written to the anim instance by name (`FFloatProperty`/`FDoubleProperty`, `SPEED_VARIABLE` of `cat_anim_blueprint.py`); montage end via `FOnMontageEnded` (interrupted = replaced); jump end → `SetActorHiddenInGame(true)`; roam timer on world time
- [x] `Encounter/Action/GeoCatAction.h/.cpp` — `Cat` (`TObjectPtr<AGeoCat>`), `Command` (`EGeoCatCommand`), `Path` (`TArray<TObjectPtr<AActor>>`), `Target` (`TObjectPtr<AActor>`), `bWaitForEnd` (true); `bLatent` true; `Finish()` at once when not waiting or for `Idle` / `Roam`; null cat → log `LogGeoEncounter`, `Finish()`; `OnCancel` unbinds; `IsDataValid`: null cat, empty path for `WalkPath`, `bWaitForEnd` on `Idle` / `Roam` (warning)
- [x] clang-format every touched file

## Scripts
- [x] `AI/Python/Cat/Anim/cat_blueprint.py` — `BP_Cat` in `/Game/Actors/Cat` (parent `AGeoCat`): `Mesh` = `SKM_Cat`, anim class `ABP_Cat`, `ReassembleMontage` = `AM_Cat_Reassemble`, `PortalJumpMontage` = `AM_Cat_PortalJump`; report `AI/Output/cat_blueprint.json`; after the build, editor via bridge
- [x] `AI/Python/Tutorial/Level/tutorial_encounters.py` — run `encounters_diff.py` first (rule); place `Tut_Landing_Cat` (`BP_Cat`, ~300 cm ahead of the Landing start, facing it, `RoamArea` = `Tut_Landing_ArenaVolume`) and `Tut_Landing_CatPath_0..1` (`TargetPoint`, a short line across the room); `CATEGORIES` += `("Cat", "Gameplay")` (covers the path points); `Builder.cat(command, path=(), target=None, wait=True)`; `landing_steps` OnEnter: lock → cat `Reassemble` (wait) → line 0 → cat `WalkPath` (no wait) + line 1 → lines 2-3 → unlock + cat `Roam` + line 4 → line 5's pad open + cat `PortalJump` `Target` = `Tut_Landing_PadExit` (no wait) + line 5 → count `Done`
- [x] Re-run `tutorial_encounters.py` headless (editor closed), read `AI/Output/tutorial_encounters_report.txt`

## Tests (`Private/Tests/GeoCatTests.cpp`, `GeoTrinity.Cat.*`; bare `AGeoCat` spawned in `GeoTestWorld.h`, no mesh, `AdvanceMove` called by hand)
- [x] `WalkPath` — two points: reaches each in order, `OnCommandEnded(true)` once, at the last point
- [x] `Replace` — `Roam` during a walk ends the walk with `OnCommandEnded(false)`
- [x] `RoamStaysInside` — many roam targets all inside `RoamArea` bounds minus `RoamMargin`
- [x] `JumpHides` — `PortalJump(null)` with no montage: hidden, ended completed; `Reassemble` shows it again
- [x] `JumpApproach` — `PortalJump(Target)`: stops `PortalDistance` before the target, facing it, then hides
- [x] `GeoTrinity.Encounter.CatAction` (`GeoEncounterTests.cpp`) — `bWaitForEnd` waits for the cat's end; false finishes at once; `Roam` finishes at once; null cat finishes
- [x] Run: `Tools\Build_Editor.bat`, `Tools\Run_Tests.bat GeoTrinity.Cat` and `GeoTrinity.Encounter` (`GEO_NO_PAUSE=1`, editor closed)

Done 2026-10-20: built, `GeoTrinity.` 86/86, `BP_Cat` and the Landing staging written (scene diff identical before and after). Left: the user's look below.
Restructured 2026-10-20 (captain / navigator): the cat only walks, roams, plays a given montage; orders and hiding are encounter actions (`Encounter/Action/Cat/CLAUDE.md`). Rebuilt `BP_Cat` + Landing through the bridge, diff identical, 86/86, flow PASSED (cat hidden 110 cm before the pad, position stable).

## Verify
- [x] `Tutorial/Runtime/pie_tutorial_flow.py` passes headless (Landing → Shoot; Cat shown at start, hidden 110 cm before `Tut_Landing_PadExit`); the flow now presses the skip key (`TroobleshootingLogs.md`)
- [ ] User, in PIE when asked: splat → idle facing the player, walk line, roam, jump into the pad (the clip's portal opening on the open pad: concentric, check it reads), walk speed vs foot slide

## Status 2026-10-25
Work done but checklist is stale: it lists `AGeoCat` commands / tests (`WalkPath`, `JumpHides`, `JumpApproach`) replaced by `Walk`/`Roam`/`PlayMontage`, `GeoCatWalkAction`/`GeoCatRoamAction`/`GeoCatMontageAction` (`Encounter/Action/Cat/`) and tests `Cat.Walk`, `WalkStopDistance`, `Replace`, `RoamStaysInside`, `Montage`. Remaining: user PIE look; then rewrite or delete.
