# TutorialMinibossRework — pending

Goal: the tutorial Miniboss fight is reworked and balanced: readable, fair for a first fight, short.
Priority: 1
Docs written: `AI/Tutorial/Steps.md` (step 10, "Tuning"), `AI/Python/Tutorial/CLAUDE.md`, `Source/GeoTrinity/Public/AbilitySystem/Abilities/Boss/CLAUDE.md`, `.../Pattern/Hazard/CLAUDE.md` + `SafeSpot/CLAUDE.md`, `Source/GeoTrinity/Public/Characters/CLAUDE.md`, `Source/GeoTrinity/Public/Actor/Arena/CLAUDE.md`, `Source/GeoTrinity/Public/GameClasses/Match/CLAUDE.md`.

Done: code (`StartDifficulty`, `AddSpotRadius`, `SafeAddSetup`, `ZoneStartSize`), scripts run, `GeoTrinity.Tutorial.` set green (34/34, FullRun included), new tests `Tutorial.SafeDifficulty` and `Tutorial.MinibossSafeRows`. Values the user can retune in `CT_Tutorial` (columns Safe/Reduced/Original): ally damage 15, lane radius 127. The summon wind-up is 2 s at every level (a plain float on the ability, not a row).

## Left: boss walk speed ×0.5 (code written, not built: the user's editor was open with Live Coding)
- [ ] Editor closed: `Tools\Build_Editor.bat` (`FGeoEnemySetup::WalkSpeedMultiplier`, `AEnemyCharacter::ApplySetup`).
- [ ] Set `WalkSpeedMultiplier` 0.5 on `Tut_Miniboss_Arena`'s `BossSetup` in `/Game/Maps/Tutorial` (the script `tutorial_encounters.py` already writes it; run `encounters_diff.py` first, `AI/Python/Tutorial/CLAUDE.md` "Read the scene first").
- [ ] `Tools\Run_Tests.bat GeoTrinity.Tutorial.` green (tutorial gate).

## Left: needs eyes in the editor (not checked, headless only)
- [ ] The summon plays `SK_Star_PikeNova_Montage` on `BP_Dummy`: confirm it reads as the boss's summon and that its length suits 2 s.
- [ ] Ally projectiles show the `AllyDamage` colour: `BP_TutorialPattern_AllySalve` has the head/trail override set, but a replicated projectile may carry its colour on its Blueprint (`Tool/Visual/CLAUDE.md`); if the colour does not show, colour the projectile Blueprint child.
- [ ] Ally bullets never hurt the player: guaranteed by the hostile-only attitude of the salve pattern; look once in play.
