# CatLineTypewriter — pending

Goal: a Cat line types out letter by letter with a small tick per word at a per-word pitch, ends on its own timer, and F drives it (F = whole text, F again = end line and advance the tutorial), so mashing F runs the tutorial faster.
Docs written: `GeoTrinityUI/Public/HUD/Tutorial/CLAUDE.md` ("Cat line", markup row), `GeoTrinity/Public/Encounter/Action/CLAUDE.md` ("GeoCatLineAction"), `GeoTrinity/Public/HUD/Interface/CLAUDE.md`, `AI/Tutorial/CLAUDE.md`, `Flow.md`, `Steps.md`, `AI/Python/Level/CLAUDE.md`.

## Code (to implement)
- [x] `GeoTrinityUI/.../HUD/Tutorial/GeoCatLineMarkup.h/.cpp` — `GeoCatLine::RevealRichText(RichText, VisibleCount)`: first N visible chars, tags cost 0, open run closed
- [x] `GeoCatLineWidget.h/.cpp` — phases Typing / Reading / Ended; `constexpr CharsPerSecond` in the .cpp; `LineSeconds` = reading time after the text is complete (re-read default 6 s: probably shorter); drop `SkipGraceSeconds`; `SkipLine` = Typing → show all, Reading → end; native `OnLineEnded` fired on timer, skip, replacement by a new line, `NativeDestruct`; keep the face talking only while Typing/Reading
- [x] `GeoCatLineMarkup.h/.cpp` — `PitchForWord(FStringView, Min, Max)`: case-insensitive hash of the word mapped into `[Min, Max]`, same input = same output; the widget finds the words of the visible text once per line (start index of each word)
- [x] `GeoCatLineWidget.h/.cpp` — `TypeSound` (`TObjectPtr<USoundBase>`, `EditAnywhere`, `Category = "GeoCatLine"`); in the Typing phase, each time the revealed count reaches a word's first character `UGameplayStatics::PlaySound2D(this, TypeSound, 1.f, Pitch)` (one tick per frame, last word); constexpr `PitchMin` 0.8 / `PitchMax` 1.4 beside `CharsPerSecond`; null sound = silent
- [x] `GeoTrinity/.../HUD/Interface/GeoHUDInterface.h` + `AGeoHUD::ShowCatLine(Line, FSimpleDelegate OnEnded)` — forwards the delegate to the widget
- [x] `Encounter/Action/GeoCatLineAction.h/.cpp` — `bLatent = true`; `Finish()` from the end callback; no HUD → `Finish()`; `OnCancel` unbinds; respawn wait path unchanged then finishes after the line
- [x] clang-format every touched file

## Scripts
- [x] script edited and checked against the scene (`encounters_diff.py`: only the removed waits differ); **re-run still to do**, headless with the editor closed: `AI/Python/Level/tutorial_encounters.py` — drop the `wait()` that follows a `say()` (16 uses of `wait` / `LINE_SECONDS`: keep only waits that are real pacing, not line length); re-run per `AI/Python/Level/CLAUDE.md` (hand-edited rooms are kept: list them to the user)

## Editor / Blueprint
- [ ] `WBP_CatLine` `TypeSound` = `/Game/Art/SFX/Mech/SW_Mech_Tick_1` (placeholder for the test; the real sound comes through `AI/SoundCreation.md`, then this row is the only change); check it routes to the interface sound class (`/Game/Art/SFX/Mix/SC_Interface`), set on the wave or the widget's play if not

## Verify
- [x] Tests (`GeoCatLineTests.cpp`): `GeoTrinityUI.Tutorial.CatLineReveal` — plain text, bold / shake run cut mid-way is closed, key label counted, N ≥ total returns the full text, N = 0 returns nothing visible; `GeoTrinityUI.Tutorial.CatLinePitch` — same word same pitch, case-insensitive, always within `[Min, Max]`, at least 10 distinct pitches over 26 different words
- [ ] Phase logic and F-F-advance have no headless layer (need a local player and viewport): PIE check by the user when asked
- [x] Build `Tools\Build_Editor.bat`, `Tools\Run_Tests.bat GeoTrinityUI.Tutorial` and `GeoTrinity.Encounter`
