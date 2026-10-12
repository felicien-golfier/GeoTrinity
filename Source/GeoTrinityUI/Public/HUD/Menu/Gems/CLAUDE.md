# HUD/Menu/Gems

Gems menu over `UGeoGemSubsystem` (`GeoTrinity/Public/Gem/CLAUDE.md`). Opened from the character sheet.

| File | Role |
|---|---|
| `GeoGemsWidget.h` | Menu: `PageSwitcher` (`LoadoutPage`, `ForgePage`), page and class tabs, shard count |
| `GeoGemPageWidget.h` | Base of both pages; the one way they edit the profile: `CommitChanges` (save + server + menu refresh) |
| `GeoGemLoadoutWidget.h` | Socket board: clusters of tier rings around a centre |
| `GeoGemForgeWidget.h` | Forge: tiers (select, break down free copies → shards), gem types with owned / free counts, shards → gems |
| `GeoGemSocketButton.h` | Bare button drawn by its `UGeoGemGlyph`; says which socket it is on click |

- Glyph drawing and colours: `Style/Glyph/`; gem polygons and highlight from theme `GemStyle`.
