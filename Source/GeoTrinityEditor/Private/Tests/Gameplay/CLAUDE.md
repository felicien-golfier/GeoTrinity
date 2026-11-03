# Tests/Gameplay

Automation tests of the `GeoTrinity` module, one subfolder per area (mirrors the module's folders). Fakes they spawn: `Fake/CLAUDE.md`.

| Folder / file | Covers |
|---|---|
| `Encounter/` | Encounter engine, triggers, actions, jump / arrival, start gate → `Encounter/CLAUDE.md` |
| `Gem/` | Gem catalog, sockets, builds, echo, loot, stats → `Gem/CLAUDE.md` |
| `Pattern/` | Safe spots, salve / beam / zone / pillar patterns, targeting, zone layer → `Pattern/CLAUDE.md` |
| `Characters/` | Enemy setup, summons, arena boss, void death → `Characters/CLAUDE.md` |
| `Deployable/` | Deployable init, start health, owned spawn, turret, growing zone → `Deployable/CLAUDE.md` |
| `Abilities/` | Ability ready, incoming damage, combat stats, montage ownership → `Abilities/CLAUDE.md` |
| `Fake/` | Fake actors / worlds → `Fake/CLAUDE.md` |
| `GeoCatTests.cpp` | Cat walk, stop distance, hidden walk, replace, roam, montage order, idle variants |
| `GeoCatLineBankTests.cpp` | Cat line bank: parse, bags, file order, live reload, missing file |
| `GeoLeaderboardTests.cpp` | Leaderboard save: one best attempt per player, group row shared, worse / equal attempt dropped, per boss and difficulty, legacy file collapse, echo level kept through prune and a save round trip, build wipe once per stamp |
| `GeoDifficultyTests.cpp` | Nightmare: effect mask gate at level 8 |
| `GeoSoundConcurrencyTests.cpp` | Sound concurrency assets |
| `GeoTutorialTests.cpp` | Solo-only login, listen URL, locked / unlocked abilities, game mode's StartDifficulty → game state |

- Each test builds its own world with `CreateTestWorld` / `DestroyTestWorld` (`Fake/GeoTestWorld.h`); none touches a map asset.
- Every file carries its own `constexpr EAutomationTestFlags Geo<Area>TestFlags` (application contexts + `ProductFilter`); an area of several files shares one in `GeoTestFlags.h` (unity builds put them in one blob).
- A helper several files of one folder need lives in that folder's `Geo<Area>TestHelpers.h` (inline / `static`, no `UCLASS`); a fake `UCLASS` goes to `Fake/`.
