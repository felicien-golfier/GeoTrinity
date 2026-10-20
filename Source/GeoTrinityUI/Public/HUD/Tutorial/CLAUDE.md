# HUD/Tutorial

Tutorial HUD pieces, driven by `AGeoHUD` through `GeoHUDInterface` (`GeoTrinity/Public/HUD/Interface/CLAUDE.md`). Design: `AI/Tutorial/`.

| File | Role |
|---|---|
| `GeoCatLineMarkup.h` | `GeoCatLine::ToRichText(Markup, KeyLabel)`: board markup → rich text; `RevealRichText(RichText, VisibleCount)`: the typewriter's prefix; `PitchForWord(Word, Min, Max)`: the tick's pitch; pure, tested |
| `GeoCatLineWidget.h` | `WBP_CatLine`: the Cat's one line on screen; types, holds, ends on timer or F; never blocks input |
| `SGeoCatFace.h` | Slate widget painting the Cat's face (the Tutorial Board's `catface` / `catfacet` symbols), animated |
| `GeoCatFaceMotion.h` | Blink / ear flick / mouth loops as pure functions of time; tested |
| `GeoCatFaceWidget.h` | UMG wrapper of `SGeoCatFace`: palette + talking flag as Blueprint properties |
| `GeoShakeTextDecorator.h` | Rich-text decorator for `<Shake>` runs: the run jitters on a Slate active timer |
| `GeoQuestCardWidget.h` | `WBP_QuestCard`: quest title, `n/Max`, time-limit countdown (step 3 gag) |

## Markup (Tutorial Board text → rich text)
- `**x**` → `<Bold>x</>` (row `Bold` of the widget's text style table); `~~x~~` → `<Shake>x</>` (decorator). No nesting: a run is bold or shaking (step 2's 3rd fall shakes only the dying word).
- Typewriter: `GeoCatLine::RevealRichText(RichText, VisibleCount)` (pure, tested) keeps the first `VisibleCount` visible characters of a `ToRichText` result and closes the run left open; a `{Key}` label counts as its letters, a tag as none.
- `{Key:<InputTag>}` → the label of the key bound to that input, through `UHudFunctionLibrary::GetMappedKey` (input tag → `UAbilityInfo` player entry → its `InputAction`), the resolver the ability slots use (`HUD/AbilityBar/CLAUDE.md`): key names are never typed in a line.
- Anything else in braces, or an input tag that does not exist, stays as written.

## Cat line
- `AGeoHUD::ShowCatLine(Line)` creates the widget on first use (`CatLineWidgetClass`), then `UGeoCatLineWidget::ShowLine`: a new line replaces the shown one at once (one speaker, one line).
- Drawn above every menu (`CatLineZOrder` > the controller's `PauseMenuZOrder`): a line said with an ability page shows over it. `HitTestInvisible`: it never takes a click.
- Three phases on one Slate active timer (Slate time: it runs while the solo pause holds the world, `GameClasses/Player/CLAUDE.md`, so a line over a page keeps going while the player reads):
  1. **Typing**: letter by letter at `CharsPerSecond` (a `constexpr` in `GeoCatLineWidget.cpp`, the one place to tune the speed). Only visible characters count; markup never shows half-open (`GeoCatLine::RevealRichText`).
  2. **Reading**: the whole text shows; the line stays a reading time set by its length (`ReadingBaseSeconds` + visible characters / `ReadingCharsPerSecond`, `constexpr`s beside `CharsPerSecond`), unless `LineSeconds` on the widget is set (> 0), which overrides it. No delay on the encounter.
  3. **Ended**: collapsed, face silent, `OnLineEnded` broadcast.
- Typing sound: one short tick per word (`TypeSound`, `USoundBase` on the widget, played 2D), played when the word's first visible character appears; none when F reveals the rest. Pitch = a pure function of the word (`GeoCatLine::PitchForWord`, range `constexpr`s next to `CharsPerSecond`): the same word always sounds the same, words differ, so a sentence has a melody. A word is a run of visible non-space characters (markup tags are not visible, a bold run inside a sentence still splits on its spaces). Several words revealed in one frame (low frame rate) play one tick, the last word's pitch.
- Skip = `OnInteractUI` (key F) of the owning `AGeoPlayerController`, no mouse button (the mouse fires abilities): press in Typing → whole text at once (Reading starts); press in Reading → Ended. Two presses always end a line, so mashing F runs the tutorial as fast as the player wants. No grace delay: a press never ends an unread line (the first one only completes it).
- Skip hint: a key cap at the panel's top right showing only the key bound to `InteractUIAction` (label through `UHudFunctionLibrary::GetMappedKey` on the owning controller's `InteractUIAction`, never a typed "F"; text-only, no "to skip" words). Optional `BindWidgetOptional` `SkipKey` (`UTextBlock`) inside a `SkipHint` border of `WBP_CatLine`. Resolved on each `ShowLine`, so a rebind shows on the next line. Always visible while a line shows (both phases skip). Unbound or invalid key → hint collapsed.
- `OnLineEnded` (native delegate on the widget) fires on every end: timer, skip, replaced by a new line (the old line's waiter must not hang), widget destroyed. `AGeoHUD::ShowCatLine(Line, OnEnded)` hands the caller's delegate to it; one waiter per line.
- Lines come from `GeoCatLineAction` (`Encounter/Action/CLAUDE.md`), which waits for `OnLineEnded`: the line's own length paces the encounter, no delay is placed after a line. Text authored on the placed encounter.

## Cat face (the speaker's portrait)
- `WBP_CatLine` = panel: `CatFace` (left, square; optional `BindWidget` of `UGeoCatLineWidget`) + name label "CAT" + `LineText`. Same layout as the Board's speech box: dark translucent panel, 1 px border and soft glow in the Cat's base shade.
- `SGeoCatFace` draws the Board's face vertex for vertex in a 200×200 design box scaled to the widget: 2 ears + inner ear shades, head (polygon + rounded rect), 2 rounded eyes, nose triangle, mouth. Flat fills, no textures (`HUD/Style/CLAUDE.md`). Vertices live in the file as the single copy of the shape; the Board is the visual reference.
- Palette = the bosses' colour in shades (`Tutorial/Cat.md`), 4 properties on `UGeoCatFaceWidget`: `Base` (head, ears, border), `Shade` (inner ears, darker), `Ink` (mouth, darkest), `Light` (eyes, nose; pale tint, drawn with a soft halo). Nothing hard-coded; defaults = the Board's boss hue.
- Motion on one Slate active timer, no Tick, same loops as the Board: eye squint / blink 5.4 s, ear flick 5.5 s (second ear offset 0.3 s), mouth open-close 0.38 s while talking.
- Talking = a line is on screen: `UGeoCatLineWidget` sets it on `ShowLine`, clears it when the line ends or is skipped. Idle = closed mouth.
- Face only; the full body, tail and 3D hologram are the Cat hologram slice (`Tutorial/Cat.md`).

## Quest card
- `ShowQuestCard(Title, Max, TimeLimitSeconds)`, `SetQuestCount(Count)`, `HideQuestCard()` on `GeoHUDInterface`; `AGeoHUD` creates it on first use (`QuestCardWidgetClass`).
- Title, `n/Max` and a quest time limit counting down from `TimeLimitSeconds` as `mm:ss` (step 3: 30 min, as if the quest ended then). Pure decoration: nothing happens at 0, the real cut is the encounter's (`Tutorial/Steps.md`, step 3); the dummies' respawn is felt, not shown. The card never decides anything, the encounter does.
- Counted on world time (`GetWorld()->GetTimeSeconds()`, `ShownTime` = world time at `ShowQuestCard`), still on its Slate active timer: holds during the solo pause. Stops at 00:00.
