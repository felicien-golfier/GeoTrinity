# Tests

Every automation test of the project. Editor module (`Type=Editor`), so no test code or fake `UCLASS` reaches a game, server or packaged build. Run / filters / layers: `AI/Testing.md`.

| Folder | Holds |
|---|---|
| `Gameplay/` | Tests of `GeoTrinity` (GAS, gems, encounters, cat, patterns, tutorial rules) → `Gameplay/CLAUDE.md` |
| `Gameplay/Fake/` | Fake transient actors / worlds the tests spawn → `Gameplay/Fake/CLAUDE.md` |
| `UI/` | Tests of `GeoTrinityUI`: `GeoMessageLogTests.cpp` (message log layout), `GeoCatLineTests.cpp` (Cat line markup, Cat face motion, Cat expressions), `GeoLoadingScreenTests.cpp` (loading timeline, tip pool) |
| `Tutorial/` | Play-in-editor bot, every tutorial branch → `Tutorial/CLAUDE.md` |

- A test reaches the game modules through their `Public/` headers only; behaviour a test needs and `Public/` hides is exposed with `<MODULE>_API`, never by an include path into `Private/`.
- Test path `GeoTrinity.<Area>.<Case>` (`AI/Testing.md`). Files wrap in `#if WITH_DEV_AUTOMATION_TESTS`; no `WITH_EDITOR` guard (the module is editor-only).
- A new test goes next to the area it covers: gameplay → `Gameplay/`, UI → `UI/`, tutorial bot → `Tutorial/`.
