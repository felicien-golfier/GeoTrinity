# SummonPattern — pending

Goal: the tutorial miniboss's summon is a plain pattern ability + `USpawnAddPattern`; each add owns its zone and its ally through passives in its setup: no `AGeoSummonedDummy`, no summon params on an enemy, no extra StateTree or Blueprint.
Priority: 1
Docs written: `Abilities/Boss/CLAUDE.md` ("Summon", "GeoSpawnOnDeathAbility"), `Abilities/Common/CLAUDE.md` ("PlaceDeployable"), `Abilities/Pattern/Hazard/Zone/CLAUDE.md` ("SpawnAdd"), `AbilitySystem/Lib/CLAUDE.md` (`Self`), `Characters/CLAUDE.md` ("Enemy setup", "Summoned dummy"), `AI/Python/Tutorial/CLAUDE.md` (`tutorial_miniboss.py` row).

## Read map
- `Source/GeoTrinity/Private/Characters/GeoSummonedDummy.cpp` — what goes (zone spawn, ally on death).
- `Source/GeoTrinity/Private/Characters/EnemyCharacter.cpp` — `InitGAS` (~54-77: level, health lines; add `ReactivatePassiveAbilities()` after them, server), `DeathLogic` (~133-134), `ApplySetup` (:72-77, `AbilityTags` into `StartupAbilityTags`).
- `Source/GeoTrinity/Private/AbilitySystem/Components/GeoAbilitySystemComponent.cpp:626-637` — `ReactivatePassiveAbilities`; `GeoGameplayAbility.cpp:35-58` passive activation.
- `Source/GeoTrinity/Private/AbilitySystem/Abilities/Boss/GeoSpawnOnTileAbility.cpp:12-39` — the shape the placer copies (ServerOnly, `Fire` → `FullySpawnDeployable` → `EndAbility`).
- `Source/GeoTrinity/Private/AbilitySystem/Abilities/Common/GeoDeployAbility.cpp:178-187` — the gem adjustment of `MakeSpawnParams` (not shared: the placer applies none).
- `Source/GeoTrinity/Public/AbilitySystem/Lib/GeoTarget.h` + `GeoAbilitySystemLibrary.cpp` `PickTargets` — the `Self` mode.
- `Source/GeoTrinity/Private/AbilitySystem/Abilities/Pattern/Hazard/Zone/SpawnAddPattern.cpp:24-28` — `bSafeDropsStateTree` goes.
- `Source/GeoTrinity/Private/Actor/Arena/GeoArena.cpp:173-189,223-233` — `SpawnAdd`, `DestroyAdds` (fires no death).
- `Source/GeoTrinity/Private/Characters/GeoCharacter.cpp:269-275` — `Death()`: `OnCharacterDied` before `DeathLogic`.
- `AI/Python/Tutorial/Level/tutorial_miniboss.py` — `enemy_setup` (:125), `build_summon_pattern` (:208), `build_summoned_dummy` (:249), `OLD_ASSETS`.
- Tests: `Tests/Gameplay/Characters/GeoSummonTests.cpp` (`GeoTrinity.Abilities.SummonAddSetup` Safe assertions :154-166 go; `GeoTrinity.Characters.SummonedDummy` :69-108 rewritten).

## Code (to implement)
- [ ] `GeoTarget.h` + `PickTargets` — `EGeoTarget::Self`: a pick whose `Actor` = `Context.Caster` (3 lines); doc comment on the enum.
- [ ] `EnemyCharacter.cpp` — `ReactivatePassiveAbilities()` at the end of `InitGAS` (server). First check no existing enemy holds a Passive-tagged ability (boss, dummy, cat, hidden caster: Content grep + the catalog).
- [ ] `Abilities/Common/GeoPlaceDeployableAbility.h/.cpp` — per doc: `ServerOnly`, `DeployableActorClass`, `Params`, `Size` scalable at the ability level; `Fire` per `Target` location → `FullySpawnDeployable` → `EndAbility`; no gems.
- [ ] `Abilities/Boss/GeoSpawnOnDeathAbility.h/.cpp` — `AllyClass`, `AllySetup`; passive; bind `OnCharacterDied` (own avatar) in `ActivateAbility`, `Arena->SpawnAdd` at the avatar's transform, unbind in `EndAbility`.
- [ ] Remove `bSafeDropsStateTree` from `SpawnAddPattern`.
- [ ] Delete `GeoSummonedDummy.h/.cpp`; fix `GeoTutorialBot.cpp:13,623` and `GeoTutorialRoomStages.cpp:14,216-227,1451` (find summons by the setup `Tag`), `Tests/Gameplay/Fake/GeoTestEncounter.h:7,317` (`AGeoTestSummonedDummy`), `GeoTestPatternAbility.h:325-337`.
- [ ] Tags + catalog: the two abilities' gameplay tags (`Config/Tags/GeoGameplayTags.ini`, editor restart), `DA_AbilityInfo` rows (`waves.register`), the Passive tag on both; the test fakes in the catalog as `UGeoTestCastAbility`.
- [ ] Grouping: nothing to fold (`Boss/` and `Common/` have their `CLAUDE.md`).

## Scripts
- [ ] `tutorial_miniboss.py` — `build_summoned_dummy` deleted; `build_summon_pattern`: `AddClass` = `BP_Dummy`, `AddSetup` = health row, `Tag` (new, for the bot), no `StateTree`, `AbilityTags` = the zone + spawn-on-death passives; two new ability builders (zone: effects = old `ZoneEffects`, `Params` `(Color=Damage, Attitude=Hostile)` + growth rows, `Size` = `DummyZoneStartRadius` row; ally: `BP_DummyAlly` + `enemy_setup(PLAYER, AllyDummyHealth, ALLY_SALVE_TAG, ALLY_ACTOR_TAG)`); `bSafeDropsStateTree` line gone; `BP_TutorialSummonedDummy` added to `OLD_ASSETS`. Run `encounters_diff.py` before `tutorial_encounters.py` (`MINIBOSS_ALLY_TAG` comment names the old dummy).
- [ ] `AI/Python/Tutorial/Runtime/pie_tutorial_miniboss.py:25` — `SUMMONED_CLASS` → match by the `Tag`.
- [ ] `training_room.py` Ally station (touches the DraftMap).
- [ ] Hidden numbers: the old zone read `CT_Tutorial` column 1 (`FDeployableData.Level` default); through the ability it reads the difficulty column. Check the `DummyZone*` rows (`tutorial_assets.py`: flat columns today = no change).

## Deletions to make (nothing else may reference them)
- `GeoSummonedDummy.*`, `AGeoTestSummonedDummy`, `bSafeDropsStateTree`, `BP_TutorialSummonedDummy` asset (and `GE_TutorialSummonedDummyAttributes`, already in `OLD_ASSETS`), the dummy's `ST_TutorialSalveShooter` use (the tree stays: allies).
- After the script run: grep `Content` and the scripts for any leftover reference to them (tests, `AI/Tutorial/Steps.md`, `AI/Pending/Tutorial.md` rows) and fix.
- `BP_Dummy`'s class tree: confirm a summon (no setup tree, so the class's) never moves off its zone; else give it no tree.

## Tests (`GeoSummonTests.cpp`, `AI/CodingStyle.md` "Tests")
- [ ] `Self` target: pick = the caster's location, follows it.
- [ ] Zone centred on the dummy (not the fire socket); gone once the dummy dies.
- [ ] Passive abilities of a spawned enemy are active right after `SpawnEnemy`, at the spawn's level; an enemy with none is unchanged.
- [ ] Dummy dies → one ally at its place, in its arena; two dummies of two casts → one ally each.
- [ ] Dummy destroyed without dying → no ally, no error.
- [ ] A summoned dummy has no shooting ability.

## Verify
- [ ] `Tools\Build_Editor.bat` (editor closed), quick suite via `test-fixer`.
- [ ] Tutorial runs at risk (miniboss changed): `GeoTrinity.Tutorial.FullRun`, `Path.MinibossWipe`, `Path.MinibossWaveLanes`, plus `Path.GemsForge`, `Path.GuideStray`, `Path.FinalExit` (all fight the miniboss with summons alive).
