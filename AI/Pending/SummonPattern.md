# SummonPattern — pending

Goal: the tutorial miniboss's summon is owned by its pattern alone (add, growing zone, ally on death), with no `AGeoSummonedDummy` and no summon params on an enemy.
Priority: 1
Docs written: none yet (design open); the easy steps below already updated `Characters/CLAUDE.md`, `Zone/CLAUDE.md`, `AI/Python/Tutorial/CLAUDE.md`.

## Open design — to discuss with the user first
Decided: every summon param (zone effects / params / start size, ally class, ally setup) sits on the summon pattern; `AGeoSummonedDummy` goes; the add Blueprint is a plain look.

Blocker: `UPattern` is one live instance per pattern class per ASC and ends with its ability, so the pattern cannot stay alive to watch its dummy die (zone life, ally on death) without blocking the boss's next summon.

Options to weigh:
- A. Pattern stays alive until its adds die. Cost: one summon at a time per boss; ability stays active (check `STTask` waits on it, `FireDelay` hold).
- B. A new owner outliving the ability, created by the pattern: a small server object / component per summon, holding the zone and the ally data and listening to `AGeoGameState::OnCharacterDied`. Pattern still owns every param (passes them in).
- C. Death hook generic on the enemy (`FGeoEnemySetup` field, e.g. spawn-on-death class), zone spawned by the pattern; ally setup then lives in the ally Blueprint's defaults. Smaller, but puts data on the enemy again.

Questions for the user: several summons alive together (the tree summons every cycle)? the pattern ending before the dummy dies OK?

## Read map
- `Source/GeoTrinity/Public/Characters/GeoSummonedDummy.h` + `Private/Characters/GeoSummonedDummy.cpp` — what moves.
- `Source/GeoTrinity/Private/AbilitySystem/Abilities/Pattern/Hazard/Zone/SpawnAddPattern.cpp` — where it moves to.
- `Source/GeoTrinity/Public/AbilitySystem/Abilities/Pattern/CLAUDE.md` — one-instance rule.
- `Source/GeoTrinity/Public/Actor/Arena/CLAUDE.md` — `SpawnAdd`.
- `AI/Python/Tutorial/Level/tutorial_miniboss.py` — `build_summoned_dummy`, `build_summon_pattern`.
- Tests: `Tests/Gameplay/Characters/GeoSummonTests.cpp`.

## Left from the easy steps (friendly look removed, `bSafeDropsStateTree`)
- [ ] `Tools\Build_Editor.bat` (editor closed), then the quick suite via `test-fixer` (code written, not built or run).
- [x] Asset `/Game/Characters/Enemies/Training/BP_DummyAlly` created (child of `BP_Dummy`, mesh `SKM_SoftStar`, material 0 `MI_SoftStar_Body`), saved.
- [x] `tutorial_miniboss.py` run; `encounters_diff.py` then `tutorial_encounters.py` run (scene hand edits carried into the script first), map saved, diff identical.
- [ ] Run `training_room.py` (Ally station; touches the DraftMap).
- [x] `BP_Dummy` resaved.
- [ ] `pie_tutorial_miniboss.py` matches ally dummies by `BP_Dummy_C`; check it still finds the ally.
- [ ] Tutorial runs at risk (allies changed class): `GeoTrinity.Tutorial.FullRun` and the covering `Path.*`.
