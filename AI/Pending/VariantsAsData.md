# VariantsAsData — pending

Goal: every tutorial / Training Room ability, enemy and zone follows the lateral wave's way: one native class (or one Blueprint when it needs a look) and per-use params, numbers as `FScalableFloat` reading `CT_Tutorial`; no asset copied to change values.
Docs written: `AI/CodingStyle.md` ("Variants are data"), `Actor/Arena/CLAUDE.md`, `AbilitySystem/Abilities/Boss/CLAUDE.md`, `Actor/Deployable/CLAUDE.md`, `Characters/CLAUDE.md` (incl. new `FGeoEnemySetup::BehaviourStateTree`), `AI/Python/Tutorial/CLAUDE.md`, `AI/Tutorial/Steps.md`, `AI/Tutorial/Flow.md`.

## Done (2026-10-24)
- C++ all in: `BossSetup` + `StartAdds` (`FGeoArenaAdd`), `ResetBoss` through `SpawnEnemy`, summon `AddSetup`, salve aims by team (`bAimAtArenaBoss` gone), zone growth in `AGeoEffectZone` (`GrowToSize` / `GrowSeconds`), `AGeoGrowingZone` deleted, summoned dummy uses `DefaultZoneClass`. Added `FGeoEnemySetup::BehaviourStateTree` (miniboss = `BP_Dummy` base tree + its spell chain, keeps the fight-start gate). Editor built green.
- Tag `Ability.Spell.TutorialSalve` added; `TutorialMinibossShoot` / `TutorialAllyShoot` removed. New `ST_TutorialSalveShooter` (user's choice: `ST_Dummy_Shooter` fires `ProjectileToAllPlayers` by exact tag, so summons/allies never fired the salve).
- `BP_Zone` (`DefaultZoneClass`) had been reparented to `GeoGrowingZone` in the WIP commit: resaved onto `GeoEffectZone` through a temporary redirect, redirect removed.
- Scripts rewritten and run headless: `tutorial_assets`, `tutorial_waves`, `tutorial_miniboss`, `tutorial_encounters` (pre-diff `identical`), `training_room` (assets + level, DraftMap saved). Arena listing: `AI/Output/arena_add_classes.txt` (only the Training Room used `AddClasses`).
- Tests written: `Deployable.GrowingZone` (rewritten), `Characters.SummonedDummy`, `Abilities.SummonAddSetup`, `Arena.BossSetup`, `Pattern.SalveAimByTeam`.

## Done (2026-10-24, later)
- Editor built; `Tools\Run_Tests.bat GeoTrinity.` 104/104 green incl. `Tutorial.FullRun` (tutorial gate). Test fixes: `GrowingZone` reset `Data.Owner` between spawns; `SummonAddSetup` uses `UGeoTestBarePattern`.
- `encounters_diff.py` after the run: all sections identical.

## Left
- [x] Old assets: the 14 files deleted from disk (open editor's registry no longer listed them, so MCP `delete` had nothing to act on).
- [ ] DamageReduction zone tint: `training_room.py` now `DamageReduction`; set in the open editor through MCP (in memory), the file is locked by that editor (save error 32). Save `BP_ZonePattern_DamageReduction` there (Ctrl+S), or close the editor and re-run `training_room.py`. Check the Training Room tints in game.
- [ ] Verify in game (only on request): `pie_tutorial_miniboss.py`, `pie_tutorial_jump.py` (`#Zones`), Training Room via `pie_leave_tutorial.py`.
- [ ] Delete this file when empty.
