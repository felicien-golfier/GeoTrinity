# BossLoot — pending

Goal: each boss sets its own loot — its own random roll (count, tier chances) and fixed gems + shards on every kill, both at once — first user the tutorial miniboss (count 0, fixed Chips + shards).
Docs written: `Source/GeoTrinity/Public/Gem/CLAUDE.md` ("Boss loot", table, Flow), `Actor/Arena/CLAUDE.md` ("GeoArena", Rewards).
Grouping checked: loot code is `UGeoGemCatalog::RollLoot` + `AGeoArena::GrantGemRewards`; the new code extends both in place, no new folder.

## Code (to implement)
- [x] `Gem/GeoGemTypes.h` — `FGeoGemLootEntry` (`GemId` with `meta = (GetOptions = "/Script/GeoTrinity.GeoGemCatalog.GetGemIdOptions")`, `Count` ≥ 1); `FGeoBossLoot` (`bOverrideDropTable`, `DropTable` with `EditCondition`, `FixedGems`, `FixedShards` ≥ 0); `FGeoGemReward::Shards`
- [x] `Gem/GeoGemCatalog.h/.cpp` — `RollLoot` takes the `FGeoGemDropTable` (today's body, table lookup moved out); `GetDropTable(BossType, BossLoot)` (override or `DropTables[BossType]`); `MakeKillLoot(BossLoot, BossType, Difficulty, Stream)` → gems (roll + fixed, merged per id) and shards; static UFUNCTION `GetGemIdOptions()` (every catalog id, tier order Chip → Core); `FindBossLootErrors(BossLoot)` (unknown id, count < 1) for validation and tests
- [x] `Gem/GeoGemProfileSave.h/.cpp` — `AddShards(int32)`
- [x] `Gem/GeoGemSubsystem.cpp` — `GrantReward` adds `Reward.Shards`; log line shows shards
- [x] `Actor/Arena/GeoArena.h/.cpp` — `BossLoot` (`Category = "GeoLoot"`, after `BossType`); `GrantGemRewards` fills `Gems` + `Shards` from `MakeKillLoot` when `bBossDefeated`; `IsDataValid` (editor) reports `FindBossLootErrors`
- [x] Read an engine use of `GetOptions` before writing it (`AI/CodingStyle.md`, "Engine habits"); clang-format touched files

## Editor / Blueprint
- [ ] Existing arenas keep `bOverrideDropTable` off and empty fixed lists: no loot change in `DraftMap`
- [ ] Tutorial miniboss arena (with the tutorial map, `AI/Pending/Tutorial.md`): override on, count 0, fixed Chips, `FixedShards` = one Chip's craft cost

## Tests (headless, `AI/Testing.md`)
- [x] `Private/Tests/GeoGemTests.cpp` (low-level, catalog built in the test)
  - `GeoTrinity.Gems.BossLoot.Default` — override off: count within `DropTables[BossType]`'s range
  - `GeoTrinity.Gems.BossLoot.Override` — override with count 0 + fixed list → exactly the fixed gems and shards
  - `GeoTrinity.Gems.BossLoot.FixedUnscaled` — `Safe` difficulty: fixed gems and shards unchanged; a fixed gem also rolled merges into one stack
  - `GeoTrinity.Gems.BossLoot.Shards` — `AddShards` raises `GetShards`
  - `GeoTrinity.Gems.BossLoot.Validation` — unknown id and count 0 reported by `FindBossLootErrors`; a valid list reports none
- [x] Run: `Tools\Build_Editor.bat` then `Tools\Run_Tests.bat GeoTrinity.Gems` (`GEO_NO_PAUSE=1`, editor closed)

## Verify
- [ ] Parsed report: counts and failures; no PIE unless asked

## Status 2026-10-25
Code and the 5 `Gems.BossLoot.*` tests exist. Remaining: DraftMap arenas check (override off), tutorial miniboss arena setup (editor), parsed test report.
