# HUD/Tutorial

Tutorial HUD pieces, driven by `AGeoHUD` through `GeoHUDInterface` (`GeoTrinity/Public/HUD/Interface/CLAUDE.md`). Design: `AI/Tutorial/`.

| File | Role |
|---|---|
| `GeoCatLineMarkup.h` | `GeoCatLine::ToRichText(Markup, KeyLabel)`: board markup → rich text; `RevealRichText(RichText, VisibleCount)`: the typewriter's prefix; `PitchForWord(Word, Min, Max)`: the tick's pitch; pure, tested |
| `GeoSpeechPopupWidget.h` | Base of every in-game chat popup: phases, timer, F skip, F key cap, `OnLineEnded` (below) |
| `GeoCatLineWidget.h` | `WBP_CatLine`: child of the base; the Cat's markup, type tick, face, name label |
| `GeoShakeTextDecorator.h` | Rich-text decorator for `<Shake>` runs: the run jitters on a Slate active timer |
| `GeoQuestCardWidget.h` | `WBP_QuestCard`: quest title, `n/Max`, time-limit countdown (step 3 gag) |

## Markup (Tutorial Board text → rich text)
- `**x**` → `<Bold>x</>` (row `Bold` of the widget's text style table); `~~x~~` → `<Shake>x</>` (decorator). No nesting: a run is bold or shaking (step 2's 3rd fall shakes only the dying word).
- Typewriter: `GeoCatLine::RevealRichText(RichText, VisibleCount)` (pure, tested) keeps the first `VisibleCount` visible characters of a `ToRichText` result and closes the run left open; a `{Key}` label counts as its letters, a tag as none.
- `{Key:<InputTag>}` → the label of the key bound to that input, through `UHudFunctionLibrary::GetMappedKey` (input tag → `UAbilityInfo` player entry → its `InputAction`), the resolver the ability slots use (`HUD/AbilityBar/CLAUDE.md`): key names are never typed in a line.
- Anything else in braces, or an input tag that does not exist, stays as written.

## Speech popup (base of every chat popup)
One widget class owns everything a popup that talks to the player shares; a speaker (the Cat, any later one) is a child adding only its look and content. No popup re-implements phases, skip or the key cap.
- Owns: Typing / Reading / Ended phases and the Slate active timer (`CharsPerSecond`, reading-time `constexpr`s in `GeoSpeechPopupWidget.cpp`, the one place to tune), `LineSeconds`, `OnLineEnded` (fires once per line on every end), `OnInteractUI` binding (`SkipLine`), `EndLine`, the skip hint. Children implement three hooks: `StartLine` (convert, return the visible length), `RevealCharacters(Previous, New, bTick)`, `SetTalking`.
- Skip hint: `SkipHint` + `SkipKey` are required `BindWidget`s of the base: a child Blueprint lacking them does not compile, so a popup can never exist without its F cap.
- The cap is made visible on every `ShowLine`, never collapsed for a missing key. Label: the key bound in the active context (rebinds win), else the default key in the controller's `InputMapping`, then `MenuInputMapping` (`UHudFunctionLibrary::GetDefaultMappedKey`; a menu open swaps the active context, `GameClasses/Player/CLAUDE.md`), else the action's name.
- F works in the menu context too: `InteractUIAction` is mapped in `MenuInputMapping` as well, so a line said over an ability page skips like any other.
- A new speaker = a child class + its Blueprint, shown by an `AGeoHUD` method through `GetTutorialWidget`; no new skip or timer code.
- The quest card is not a popup of this kind (no line, no skip); it stays its own widget.

## Cat line
- `UGeoCatLineWidget` keeps only the Cat part: markup → rich text, typewriter reveal, `TypeSound` tick, face, name label. Everything below about phases, skip and the hint is the base's (above).
- `AGeoHUD::ShowCatLine(Line)` creates the widget on first use (`CatLineWidgetClass`), then `UGeoCatLineWidget::ShowLine`: a new line replaces the shown one at once (one speaker, one line).
- Drawn above every menu (`CatLineZOrder` > the controller's `PauseMenuZOrder`): a line said with an ability page shows over it. `HitTestInvisible`: it never takes a click.
- Three phases on one Slate active timer (Slate time: it runs while the solo pause holds the world, `GameClasses/Player/CLAUDE.md`, so a line over a page keeps going while the player reads):
  1. **Typing**: letter by letter at `CharsPerSecond` (a `constexpr` in `GeoSpeechPopupWidget.cpp`, the one place to tune the speed). Only visible characters count; markup never shows half-open (`GeoCatLine::RevealRichText`).
  2. **Reading**: the whole text shows; the line stays a reading time set by its length (`ReadingBaseSeconds` + visible characters / `ReadingCharsPerSecond`, `constexpr`s beside `CharsPerSecond`; `LineSeconds` is on the base), unless `LineSeconds` on the widget is set (> 0), which overrides it. No delay on the encounter.
  3. **Ended**: collapsed, face silent, `OnLineEnded` broadcast.
- Typing sound: one short tick per word (`TypeSound`, `USoundBase` on the widget, played 2D), played when the word's first visible character appears; none when F reveals the rest. Pitch = a pure function of the word (`GeoCatLine::PitchForWord`, range `constexpr`s next to `CharsPerSecond`): the same word always sounds the same, words differ, so a sentence has a melody. A word is a run of visible non-space characters (markup tags are not visible, a bold run inside a sentence still splits on its spaces). Several words revealed in one frame (low frame rate) play one tick, the last word's pitch.
- Skip = `OnInteractUI` (key F) of the owning `AGeoPlayerController`, no mouse button (the mouse fires abilities): press in Typing → whole text at once (Reading starts); press in Reading → Ended. Two presses always end a line, so mashing F runs the tutorial as fast as the player wants. No grace delay: a press never ends an unread line (the first one only completes it).
- Skip hint: a key cap at the panel's top right showing only the key bound to `InteractUIAction` (label through `UHudFunctionLibrary::GetMappedKey` on the owning controller's `InteractUIAction`, never a typed "F"; text-only, no "to skip" words). Optional `BindWidgetOptional` `SkipKey` (`UTextBlock`) inside a `SkipHint` border of `WBP_CatLine`. Resolved on each `ShowLine`, so a rebind shows on the next line. Always visible while a line shows (both phases skip); see "Speech popup" for the label rule.
- `OnLineEnded` (native delegate on the widget) fires on every end: timer, skip, replaced by a new line (the old line's waiter must not hang), widget destroyed. `AGeoHUD::ShowCatLine(Line, OnEnded)` hands the caller's delegate to it; one waiter per line.
- `AGeoHUD::HideCatLine()`: ends the shown line at once (Ended phase, `OnLineEnded` fires). Called by `GeoCatSpeechAction::OnCancel` on an abort, so the left room's text never runs on in the next one (`Encounter/CLAUDE.md`, "Arrival reset").
- Lines come from the `GeoCatSpeechAction` children (`Encounter/Action/CLAUDE.md`), which wait for `OnLineEnded`: the line's own length paces the encounter, no delay is placed after a line. Text authored on the placed encounter.

## Cat face (the speaker's portrait)
- `WBP_CatLine` = panel: `CatFace` (left, square; optional `BindWidget` of `UGeoCatLineWidget`, a `UGeoCatFaceWidget`, `CatFace/CLAUDE.md`) + name label "CAT" + `LineText`. Same layout as the Board's speech box: dark translucent panel, 1 px border and soft glow in the Cat's base shade.
- Talking = a line is on screen: `UGeoCatLineWidget` sets it on `ShowLine`, clears it when the line ends or is skipped.
- `{Key:<InputTag>}` labels: `UHudFunctionLibrary::GetInputTagKeyLabel(LocalPlayer, InputTag)`, shared with the loading screen's tips.

## Quest card
- `ShowQuestCard(Title, Max, TimeLimitSeconds)`, `SetQuestCount(Count)`, `HideQuestCard()` on `GeoHUDInterface`; `AGeoHUD` creates it on first use (`QuestCardWidgetClass`).
- Title, `n/Max` and a quest time limit counting down from `TimeLimitSeconds` as `mm:ss` (step 3: 30 min, as if the quest ended then). Pure decoration: nothing happens at 0, the real cut is the encounter's (`Tutorial/Steps.md`, step 3); the dummies' respawn is felt, not shown. The card never decides anything, the encounter does.
- Counted on world time (`GetWorld()->GetTimeSeconds()`, `ShownTime` = world time at `ShowQuestCard`), still on its Slate active timer: holds during the solo pause. Stops at 00:00.
