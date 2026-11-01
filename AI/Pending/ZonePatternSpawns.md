# ZonePatternSpawns — pending

Goal: a pattern owns what it spawns, an ability's target is an extendable param; `UGeoSummonAddAbility` and `UGeoSpawnPillarAbility` vanish; pillars and adds share `UZonePattern`, one telegraph circle, and the old pillar visuals vanish.
Priority: 2
Needs: `TargetMerge.md` done first (targeting, `UPatternAbility` picks, `GetTargetLocations`, `LandsOnTarget`, multicast).
Docs written: `Abilities/Base/CLAUDE.md` ("Pattern target"), `Abilities/Pattern/Hazard/Zone/CLAUDE.md` (new), `Abilities/Pattern/Hazard/CLAUDE.md`, `Abilities/Pattern/CLAUDE.md`, `Abilities/Boss/CLAUDE.md`, `Characters/CLAUDE.md`, `Actor/Deployable/Pillar/CLAUDE.md`, `AI/Python/Tutorial/CLAUDE.md`.

Order matters: a Blueprint must be migrated while its old C++ parent and properties still exist. Code phase 1 → build → asset migration → code phase 2 → build.

## Code, phase 1 (additive: old classes and properties stay)
- [ ] Move `ZonePattern.h/.cpp` and `SpawnPillarPattern.h/.cpp` to `Pattern/Hazard/Zone/` (Public + Private); fix includes. Class names unchanged, no redirect.
- [ ] `ZonePattern.h/.cpp` — `IndicatorComponent` → array, one per target location; `IsInHazard` loops; lingering spawn loops; `ExecuteGameplayCue` per location (from the pillar's, `CueParams.Location` set).
- [ ] New `SpawnAddPattern.h/.cpp` — parent `UZonePattern`; `AddClass`, `AddSetup`, `bUseSafeAddSetup`, `SafeAddSetup`; `EndPattern` spawns (server, `IsPatternActive`, `!bForceStop`, before `Super`, `ensureMsgf` on arena and `AddClass`); `RequiresArena()` true; `OnCreate` flags a missing `AddClass`.
- [ ] Editor build, editor closed, `GEO_NO_PAUSE=1`.

## Asset migration (one-off script, headless, deleted after; `mcp-epic` was down when this was written)
- [ ] Note first (lost after phase 2): `BP_SpawnPillarPattern` `SpawningZoneSize`, `InitCue`, `DirectionCue`, `StartCue`; `GA_SpawnPillar` `MinScatterRadius`, `MaxScatterRadius`.
- [ ] `/Game/AbilitySystem/Abilities/Enemy/SpawnPillar/GA_SpawnPillar` — reparent `UPatternAbility`; `Target.Mode` = `RandomPlayers`, `Count` 1, `ExtraBelowHealth` [0.5, 0.2], scatter values noted. `PatternToLaunch`, `PreLaunchDelay`, `PreLaunchCue` kept.
- [ ] `/Game/AbilitySystem/Abilities/Enemy/SpawnPillar/BP_SpawnPillarPattern` — `ZoneParams.Size` = old `SpawningZoneSize`, `ZoneParams.Color` = `Damage`, `ZoneParams.LifeDrainMaxDuration` = 0; `InitCue` cleared; `StartCue` = the one of `BP_ZonePattern_BurstDamage`.
- [ ] `/Game/AbilitySystem/Abilities/Enemy/SpawnPillar/BP_Pillar_Deployable` — `InitCue` cleared.
- [ ] Old pillar cues noted above: delete the cue assets and tags nothing references any more.

## Scripts
- [ ] `AI/Python/Tutorial/Level/tutorial_miniboss.py` `build_summon` — `GA_TutorialMinibossSummon` parent `UPatternAbility` (delete the old one, recreate, re-register), `Target.Mode` = `RandomPlayers`, `Count` 1; `SUMMON_SPOT_RADIUS` removed. `BP_TutorialPattern_Summon` parent `USpawnAddPattern`: delete the old child (its parent is the Training Room BP) and recreate; copy `ZoneParams`, `TeamAttitude`, `StartCue` from `BP_ZonePattern_BurstDamage` (`SUMMON_PATTERN`); `AddClass`, `AddSetup`, `bUseSafeAddSetup`, `SafeAddSetup` on the pattern CDO. Run headless after `tutorial_assets.py`, `tutorial_waves.py`.
- [ ] `AI/Python/Boss/Star/Anim/star_pike_nova.py` reads `BP_SpawnPillarPattern`: check it touches no removed property.

## Code, phase 2 (removal)
- [ ] Delete `Boss/GeoSummonAddAbility.h/.cpp`, `Boss/GeoSpawnPillarAbility.h/.cpp` (Public + Private), with `FPillarTarget`.
- [ ] `SpawnPillarPattern.h/.cpp` — parent `UZonePattern`. Remove: `FSpawnPillarPatternData`, `PillarSpawnLocations`, `SpawningZoneSize`, `DirectionCue` + its fire, constructor, `FillCueParam`, `IsInHazard`, `ExecuteGameplayCue`. `GetHazardEffects` stays (`PillarSpawnEffects`). Parent change happens here only if the phase 1 pillar pattern cannot hold both; else in phase 1 with `SpawningZoneSize` kept until migrated.
- [ ] Comments naming the old names: `GeoSummonedDummy.h`.
- [ ] Editor build; open-free check that no Blueprint fails to load (headless script loads each migrated asset).

## Tests
- [ ] `GeoPatternTests.cpp` — `UZonePattern` with several target locations: one telegraph per location, `IsInHazard` true in each circle, false outside.
- [ ] `GeoEncounterTests.cpp` `GeoTrinity.Abilities.SummonAddSetup` — rewrite on `USpawnAddPattern` (fake pattern replaces `UGeoTestSummonAddAbility`): natural end spawns on the location with `AddSetup`, as an add of the arena; Safe level uses `SafeAddSetup`; force-stop spawns nothing.
- [ ] `USpawnPillarPattern` — natural end releases its pillars (live, damageable), force-stop expires them (from `PillarDrop.md`).
- [ ] Tutorial bot: no room or step change, coverage table unchanged; the summon stage (`GeoTutorialRoomStages.cpp`, `FindLivingSummon`) must still find its add.

## Verify
- [ ] `Tools\Run_Tests.bat GeoTrinity.Abilities.SummonAddSetup`, `GeoTrinity.Pattern`.
- [ ] `Tools\Run_Tests.bat GeoTrinity.Tutorial.` (tutorial gate, whole set), `FullRun` green.
