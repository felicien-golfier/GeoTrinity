# Tool/CatLine

The Cat's random lines: a plain text bank in `Data/CatLines.txt` (tips, puns, nags), read live, picked at random from chosen categories. Any speaker of the Cat (encounter action, loading screen, idle nag) draws from it.

| File | Role |
|---|---|
| `GeoCatLineBank.h` | `GeoCatLines` namespace: `GetFile()` (the parsed `FGeoCatLineFile`), `GatherPool(Categories)`, `GetCategories()` (file order), `CountLines(Categories)`, `HasLine(Categories, Line)` (tests), `PickLine(Categories, bRandom)` (random no repeat, or file order cycling), `SetFilePath(Path)` (tests: another file, empty = default; drops the cache and every bag) |
| `GeoCatLineCategories.h` | `FGeoCatLineCategories`: the picked category names; empty = every category |

## File `Data/CatLines.txt`
- `[Category]` header starts a category; every non-empty line under it is one Cat line (full Board markup: `**bold**`, `~~shake~~`, `{Key:<InputTag>}`, `HUD/Tutorial/CLAUDE.md`); `#` line = comment (the joke's intent for the French pass goes above its line). UTF-8.
- Same shape as `Data/AbilityDescriptions.txt` (`AI/AbilityDescriptions.md`): one source of truth, runtime and editor read the same file, staged into packages by `RuntimeDependencies` in `GeoTrinity.Build.cs`.
- Category name = header text, unique, one word or CamelCase (`Pun`, `FourthWall`): it is the key stored on assets. A line before any header is ignored with a warning.
- Voice rules `AI/Tutorial/Tone.md`; the Cat `AI/Tutorial/Cat.md`.

## Bank
- Categories in use: tips (`Controls`, `Roles`, `Teamwork`), `LiveService`, `FourthWall`, `Pun`, `Retort` (what the Training Room Cat snaps when hit: angry, short, insults the situation never the player's skill, `AI/Tutorial/Tone.md`).
- Re-parsed whenever the file's timestamp changes (as `AbilityInfo.cpp`'s descriptions): an edit shows in the next pick, and in the editor picker the next time it opens, with no rebuild or reload button.
- `PickLine`: the union of the picked categories' lines in file order, a line in two categories once (empty set = all); returns empty text when no line matches (log `LogGeoEncounter` warning, the caller shows nothing and finishes). `bRandom` false = the pool in file order, one step per call, cycling.
- No repeat until the pool is spent (`bRandom`): one shuffled bag per distinct category set, shared process-wide (every action and room draws from the same bag, so a nag firing for ever cycles all its puns before any returns). A refill never starts with the line just shown. A re-parse drops every bag and cursor, so an edited, added or cut line counts from the next pick. The file-order cursor is its own state per set, same sharing.
- Missing file = packaging or path bug: `ensureMsgf`, no category (same in `AbilityInfo.cpp` for `AbilityDescriptions.txt`).
- Pure over the file: no world, no UObject, so a loading screen can call it before any world exists.

## Categories (picker)
- Stored as category names (`TArray<FName>`), not as positions: a bit index would silently shift when a category is added, moved or renamed in the file. The editor shows them as a bitmask: a dropdown of checkboxes, several at once, "All" = empty set.
- Editor picker `FGeoCatLineCategoriesCustomization` (`GeoTrinityEditor/Public/Detail/CLAUDE.md`) lists the categories parsed from the file each time the dropdown opens: a new category in the file is offered at once. A stored name the file no longer has stays listed as missing (never dropped silently); the picked lines count ignores it.
- A set whose categories are all missing or empty picks nothing (`CountLines` = 0): `IsDataValid` on the action warns.

## Users
- `UGeoCatLineFromFileAction` (`Encounter/Action/CLAUDE.md`): `Categories` + `bRandom`; its sibling `UGeoCatLineAction` types its text in place (same base).
- Idle nags: a repeating rule on `GeoPlayerIdleTrigger` + a random-line action; each firing says another line.
- `AGeoTrainingCat` (`Characters/CLAUDE.md`): `Pun` on its idle loop, `Retort` when hit; same bank and bag as every other speaker.
- Loading-screen tips: `SGeoLoadingScreen::Make` draws a few lines at once on the game thread, `Categories` = `UGeoGameInstance::LoadingTipCategories` (`GeoTrinityUI/Public/LoadingScreen/CLAUDE.md`). The bank is game thread only.
