# Tests/Gameplay/Gem

Gem system tests (`Source/GeoTrinity/Public/Gem/CLAUDE.md`); paths `GeoTrinity.Gems.<Case>`.

| File | Covers |
|---|---|
| `GeoGemCatalogTests.cpp` | Socket layout, equip rules, unequip tier, builds, forge, profile events, catalog asset, mesh paths |
| `GeoGemEchoTests.cpp` | Echo, echo stats |
| `GeoGemLootTests.cpp` | Loot, boss loot (default, override, fixed unscaled, shards, validation), drop |
| `GeoGemStatsTests.cpp` | Class XP, character stats, stats survive death, relay snap dash |
| `GeoGemTestHelpers.h` | `MakeGem`, `MakeTestCatalog`, `FindSocket`, `CountGems` shared by the files |
