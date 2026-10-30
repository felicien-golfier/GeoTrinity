# CatLineEmotions — pending

Goal: each Cat line carries an emotion and the portrait's face acts it (smirk on the nine-lives pun, evil grin on "MY house"), instead of one fixed face.
Priority: 2
Docs written: `GeoTrinityUI/Public/HUD/Tutorial/CLAUDE.md` ("Cat emotion"), `HUD/Interface/CLAUDE.md`, `Encounter/Action/CLAUDE.md` ("Timed lines and emotion"), `GeoTrinityUI/Public/CatFace/CLAUDE.md`.
After `CatExpressions.md` (the presets and `SetExpression`) (the `ShowCatLine` signature with `Seconds` is built).

## Code (to implement)
Read `AI/CodingStyle.md` first.
- [ ] `GeoTrinity/Public/HUD/Interface/GeoCatEmotion.h` — `EGeoCatEmotion` (`UENUM`), same members and order as `EGeoCatExpression` (Neutral, Evil, Smug, Derp, Cackle, Shock, Scheme, Sly)
- [ ] `GeoHUDInterface.h` / `AGeoHUD::ShowCatLine` — `Emotion` argument after `Seconds`
- [ ] `Encounter/Action/GeoCatSpeechAction.h/.cpp` — `Emotion` property (Neutral), passed on show; Details tooltip names the preset's look
- [ ] `HUD/Tutorial/GeoCatLineWidget.h/.cpp` — `ShowLine` maps the emotion onto `GeoCatFace::GetPreset` (`static_assert` on enum order) and calls `CatFace->SetExpression`; Neutral again when the line ends; mouth moves only during Typing, so Reading shows the expression's mouth
- [ ] `CatFace/SGeoCatFace` — "talking" flag set by the popup only during Typing (check today's `SetTalking` use: loading screen unchanged)

## Scripts
- [ ] `AI/Python/Tutorial/Level/tutorial_encounters.py` — `Builder.say(..., emotion=)`; emotions for the Zones room's lines: welcome Smug, house Evil, shield Sly, heal Sly, remember Neutral, `[ZoneDeath]` Smug (the nine-lives smirk; 3rd line Cackle if the user wants a per-line emotion: then one action per line instead of a file category)
- [ ] Walk the other rooms' lines and give each an emotion (the user decides, ask before a sweep; Neutral stays the default)

## Tests
- [ ] `UI/GeoCatLineTests.cpp` — emotion → preset mapping covers every member; Neutral restore on end
- [ ] `Tests/Tutorial/` — the line log keeps text only; no bot change

## Verify
- [ ] Build editor, `Tools\Run_Tests.bat GeoTrinityUI`, tutorial gate (`GeoTrinity.Tutorial.FullRun`)
- [ ] User looks at the portrait in the Zones room
