# TrainingCat — pending

Goal: the Training Room's top-left corner holds the Cat: it tells random `Pun` lines now and then, snaps a `Retort` and a warning burst when hit, chases and kills a player who dares hit it again, then walks back to its place.
Priority: 2
Docs written: `Source/GeoTrinity/Public/Characters/CLAUDE.md` ("GeoTrainingCat"), `AI/StateTree/{Movement,Ability,Utility}/CLAUDE.md`, `Actor/CLAUDE.md` (`GeoCatAnim.h`), `Tool/CatLine/CLAUDE.md` (`Retort`), `Source/GeoTrinity/Public/CLAUDE.md` (tests list), `AI/Tutorial/Cat.md` ("Training Room Cat"), `AI/Tutorial/Flow.md`, `AI/Python/Tutorial/CLAUDE.md` (`training_room.py`, `pie_training_cat.py`).
Decided with the user: enemy character (replicated, hittable, reuses GAS + StateTree); lines on the HUD Cat-line popup; hit → angry line + non-lethal warning burst; second hit → instant lethal strike.

## Check first (before coding)
- [ ] Read `GeoCat.cpp` anim-speed block (~l.260-280) and lift it as-is to `Actor/GeoCatAnim.h/.cpp` `GeoCatAnim::WriteSpeed(USkeletalMeshComponent*, float)`; `AGeoCat` calls it (one copy)
- [ ] Read `AEnemyCharacter` / `AGeoCharacter`: the health bar switch (hide it), how `StateTree` events are sent to `UStateTreeAIComponent` (`SendStateTreeEvent`), `ApplySetup`/`BeginPlay` for capturing `HomeLocation`
- [ ] Find the lethal `UEffectDataAsset` of `BP_ZonePattern_Lethal` and the burst pattern of `BP_ZonePattern_BurstDamage` (`/Game/AbilitySystem/Abilities/Enemy/Training`); reuse, no new effect numbers beyond the ability's `EffectDataInstances`
- [ ] Read `STTask_ChaseTarget`, `STTask_MoveToTargetPoint`, `STTask_FireAbility`, `STTask_PlayMontage` as models

## Code (to implement)
- [ ] `Actor/GeoCatAnim.h/.cpp` — `WriteSpeed` lifted from `GeoCat.cpp`
- [ ] `Characters/GeoTrainingCat.h/.cpp` — `AGeoTrainingCat : AEnemyCharacter`: `HomeLocation` (server `BeginPlay`, `GetHomeLocation`), hit detection on the ASC's `OnGameplayEffectAppliedDelegateToSelf` (foreign + hostile source → StateTree event `AI.Cat.Hit`), `Speak(FGeoCatLineCategories)` (server, `HearRange` gate, no pick when nobody hears), replicated `Speech` + `SpeechSerial`, `OnRep_Speech` / `ShowSpeech` → first local player's HUD `ShowCatLine`, native `OnSpoken`, tick writes `Speed` via `GeoCatAnim::WriteSpeed`, UPROPERTYs `HearRange`, `WalkSpeed`, `ChaseSpeed` (values tuned on `BP_TrainingCat`)
- [ ] `AI/StateTree/Movement/STTask_MoveToHome.h/.cpp` — see `Movement/CLAUDE.md`
- [ ] `AI/StateTree/Ability/STTask_StrikeInRange.h/.cpp` — see `Ability/CLAUDE.md`
- [ ] `AI/StateTree/Utility/STTask_CatSpeak.h/.cpp` — see `Utility/CLAUDE.md`
- [ ] `Config/Tags/GeoGameplayTags.ini` — `AI.Cat.Hit` (StateTree event), `Ability.Spell.TrainingCatWarning`, `Ability.Spell.TrainingCatStrike`; add the native constants the code uses
- [ ] `Data/CatLines.txt` — new `[Retort]` category (draft below, French intent comments above puns); `Pun` already exists
- [ ] clang-format every touched file

### Retort draft (angry, short, the situation not the player's skill)
- You hit the cat. In a training room. With "training" in the name. Bold.
- Careful. I bite, and the dummies are jealous.
- That was a warning. They come in a limited edition, once per customer.
- Do that again. I dare you. It's in the terms of service.
- Hitting the mascot voids your warranty. And, soon, you.
- Nine lives, friend. I have all of them. You have the one.

## Scripts
- [ ] `AI/Python/Tutorial/Level/training_room.py` — `build_assets`: `ST_Training_Cat` (copy of `ST_EnemyBehaviour`, emptied; states per `Characters/CLAUDE.md` "Behaviour"; `PunInterval` ~20-40 s random, `DareWindow` ~4 s, `ChaseTimeout` ~10 s, strike `Range` ~90 cm; ALL on Warned, ANY on Chase), `GA_TrainingCat_Warning` + `GA_TrainingCat_Strike` (`UPatternAbility` on the Training burst pattern; warning light damage + larger ring, strike lethal effect + claw ring + near-instant telegraph; registered in `DA_AbilityInfo`), `BP_TrainingCat` in `/Game/Characters/Enemies/Training` (parent `AGeoTrainingCat`, `SKM_Cat`, `ABP_Cat`, capsule fitted to the mesh, health bar hidden). `build_level`: `ROW_CAT` / `CAT_Y` (top-left corner, clear of the title and the first zone sign, tuned by eye), `Tuto_AddSpawn_Cat` (`ADD_SPAWN_TAG`), `Tuto_Sign_Cat` ("THE CAT / tells puns / do not hit it"), `StartAdds` entry between Bullets and Target (alphabetical = point creation order), setup: enemy team, `AggroRadius` -1, `bResetToFullLife`, `StateTree` `ST_Training_Cat`, `AbilityTags` = warning + strike. Idempotent, hand edits kept. Run headless, editor closed; read `AI/Output/training_room_report.txt`
- [ ] `AI/Python/Tutorial/Runtime/pie_training_cat.py` — per `AI/Python/Tutorial/CLAUDE.md` row; report to `AI/Output/`

## Tests (`GeoTrinity.TrainingCat.*`, `GeoTrinityEditor/Private/Tests/Gameplay/GeoTrainingCatTests.cpp`, listen test world `Fake/GeoTestWorld.h`, `SetFilePath` test bank)
- [ ] `Home` — spawned at P: `GetHomeLocation` = P, unchanged after a move
- [ ] `HitEvent` — effect from a hostile ASC raises `AI.Cat.Hit` once; own effect and a friendly effect do not
- [ ] `Speech` — `Speak` picks from the given categories, `SpeechSerial` increments, same text twice still two events; nobody in `HearRange`: no pick, no event
- [ ] `StrikeRange` — `STTask_StrikeInRange` range check: inside fires, outside never (pure 2D helper)
- [ ] Run: `Tools\Build_Editor.bat`, `Tools\Run_Tests.bat GeoTrinity.TrainingCat` and `GeoTrinity.Cat` (`GEO_NO_PAUSE=1`, editor closed)

## Verify
- [ ] Tutorial gate (`Source/` touched): `GeoTrinity.Tutorial.FullRun` built and green (CLAUDE.md "Tutorial gate")
- [ ] `Tutorial/Runtime/pie_training_cat.py` headless passes
- [ ] User, in PIE when asked: pun cadence and range, retort reads, warning ring size, chase speed vs player (dash escape?), return and calm, the corner spot
