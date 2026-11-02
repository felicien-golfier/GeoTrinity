# ArenaCenterPoint — pending

Goal: the center of an arena is a mandatory `TargetPoint.FightCenter` point set at arena creation; nobody reads the arena actor's location as its center.
Priority: 2
Docs written: `Actor/Arena/CLAUDE.md` ("Center", GeoHexArena), `Actor/CLAUDE.md` (Target points), `AbilitySystem/Lib/CLAUDE.md` (Targeting: `ArenaCenter` row gone), `AbilitySystem/Abilities/Base/CLAUDE.md` (`RequiresArena`), `Abilities/Pattern/Hazard/CLAUDE.md` (sweep), `AI/VFX.md`, `Tests/Gameplay/Pattern/CLAUDE.md`.

## Read map
- Source/GeoTrinity/Public/Actor/Arena/GeoArena.h, Private/Actor/Arena/GeoArena.cpp — `ResetBoss` fallback (l.104), `ApplyBackgroundLook` (l.655), `IsDataValid`
- Private/Actor/Arena/GeoHexArena.cpp — fall check (l.463), `GetLastAliveTileAlongRay` (l.269), grid origin (l.115, 120)
- Public/Actor/Arena/GeoDummyArena.h — exempt (`RequiresCenter` false)
- Private/AbilitySystem/Lib/GeoAbilitySystemLibrary.cpp l.869-890 + Public/AbilitySystem/Lib/GeoTarget.h — `ArenaCenter` mode to delete
- Private/AbilitySystem/Abilities/Base/PatternAbility.cpp l.52 — `RequiresArena`
- Private/Actor/Deployable/GeoDeployableBase.cpp l.125 — already reads `FightCenter`; switch to `GetCenter()`
- Private/AbilitySystem/Lib/GeoGameplayTags.cpp l.112 — tag description
- Private/Tests/Gameplay/Pattern/GeoTargetTests.cpp l.172 (`ArenaCenter` test), Tests/Gameplay/Fake/ (`AGeoTestCasterArena`)

## Code (to implement)
- [ ] `GeoArena.h/.cpp` — `FVector2D GetCenter() const` (resolves `FightCenter` + `ArenaTag` once, cached weak ptr, `ensure` when missing, returns the actor XY only after that ensure); `virtual bool RequiresCenter() const` true; `IsDataValid` error when required and no point; `ResetBoss` fallback and `ApplyBackgroundLook` use `GetCenter()`
- [ ] `GeoDummyArena.h` — `RequiresCenter()` false
- [ ] `GeoHexArena.cpp` — fall check and `GetLastAliveTileAlongRay` use `GetCenter()`; `IsDataValid` error when the point is > `TileSize` from the grid origin
- [ ] `GeoDeployableBase.cpp` — push-in reads `FightingArena->GetCenter()` instead of its own `GetTargetPoints`
- [ ] `GeoTarget.h`, `GeoAbilitySystemLibrary.cpp`, `PatternAbility.cpp` — delete `EGeoTarget::ArenaCenter` and its case; `RequiresArena` keeps the pattern CDO clause only
- [ ] `GeoGameplayTags.cpp` — `FightCenter` description: "Center of the arena; mandatory, one per arena."
- [ ] Grep the repo (Python, docs, `Config`) for `ArenaCenter` target-mode names left (`GeoTargetMode`); the material parameter `ArenaCenter` stays

## Tests
- [ ] `GeoTargetTests.cpp` — replace `FGeoTargetArenaCenterTest` (`GeoTrinity.Abilities.Target.ArenaCenter`) by `GeoTrinity.Abilities.Target.FightCenter`: `TargetPoint` mode with tag `FightCenter` picks the point, not the arena location; `RequiresArena` false for it
- [ ] new `Tests/Gameplay/Pattern` or `Characters` case `GeoTrinity.Arena.Center`: `GetCenter()` = the point's XY with the arena elsewhere; no point → ensure; dummy arena exempt from `IsDataValid`
- [ ] hex: fall check radius centered on the point (arena actor offset from it)
- [ ] `Fake/` — `AGeoTestCasterArena` test worlds spawn a `FightCenter` point (arena tag), at a spot differing from the arena location
- [ ] Run: `Tools\Run_Tests.bat GeoTrinity.Abilities.Target`, `GeoTrinity.Arena`, then `GeoTrinity.Tutorial.` (maps changed) incl. `GeoTrinity.Tutorial.FullRun`

## Editor / Blueprint (by hand or script)
- [ ] `GA_HexBoss_SweepBeam` — `Target` Mode `ArenaCenter` → `TargetPoint`, `TargetPointTag` = `TargetPoint.FightCenter`
- [ ] Every non-dummy arena of every map (`Content/Maps/Tutorial.umap`, hex boss map, others; list with a Python probe of `AGeoArena` instances) — place an `AGeoTargetPoint`, tags `TargetPoint.FightCenter` + the arena's `Arena.*`, on the middle of the room (hex: on the grid middle)
- [ ] Re-check `Tutorial.umap` arenas `IsDataValid` green after placing; update `AI/Python/Tutorial` scripts (`sync-tutorial-scripts`) so a rebuild creates the points

## Verify
- [ ] Headless tests above + tutorial gate; no editor test unless asked
