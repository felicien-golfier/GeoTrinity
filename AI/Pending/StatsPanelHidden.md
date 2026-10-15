# StatsPanelHidden — pending

Goal: the C key cycles the stat meter through small, long and hidden (hidden is new).
Docs written: `Source/GeoTrinityUI/Public/HUD/Team/CLAUDE.md` ("Stats panel").

## Editor / Blueprint (by hand or script)
- [ ] The stats panel BP (`WBP_*` using `UGeoStatsPanelWidget`) — if it overrides `LessFormat`, set `HideFormat` instead (`LessFormat` is gone); check `ToggleHintText` is outside `StatsGrid` so it stays visible when hidden.

## Verify
- [ ] Build the editor target (user asks); no automated test: pure widget view state, nothing a headless layer proves.
