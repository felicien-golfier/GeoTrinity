# TutorialAggroRadius — pending

Goal: tutorial enemies have no distance aggro, they wait to be hit (radius 100 instead of 1000).
Priority: 2
Docs written: `Source/GeoTrinity/Public/Characters/CLAUDE.md` ("Enemy setup", "EnemyCharacter"), `Source/GeoTrinity/Public/AI/CLAUDE.md`.

## Code (to implement)
- [ ] `Characters/GeoEnemySetup.h` — `float AggroRadius = -1.f` (negative = controller's)
- [ ] `Characters/EnemyCharacter.h/.cpp` — `AggroRadiusOverride` set by `ApplySetup`
- [ ] `AI/GeoEnemyAIController.cpp` — `CheckAggroDistance` uses the override when >= 0, else `AggroRadius`

## Scripts
- [ ] `AI/Python/Tutorial/Level/tutorial_miniboss.py` (`enemy_setup`), `tutorial_encounters.py:318`, `training_room.py:282` — `setup.set_editor_property("AggroRadius", 100.0)`; the casters (`HiddenCaster`) need none
- [ ] Re-run them to rebuild the map's setups (arena `StartAdds`, summon `AddSetup`, `AllySetup`)
- [ ] Miniboss: `tutorial_encounters.py:74` comment says it spawns within aggro radius of the entry pad; check the pad distance (> 100 now). The fight must start by a hit or the entry must still trigger it; fix script + `AI/Tutorial/Steps.md` / `Flow.md` if the start changes

## Tests
- [ ] Headless: enemy with `AggroRadius` 100, player at 500 → no aggro; player at 50 → aggro; hit from afar → aggro; negative → controller's 1000

## Verify
- [ ] Build editor, run the new test and `GeoTrinity.Tutorial.FullRun` (update it if the miniboss start changes)
