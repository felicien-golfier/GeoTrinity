# EchoUiPolish — pending

Goal: lighten the echo UI: no ECHO button (clicking the echo socket is enough, REPLACE ALL gets the room), the echo's cell in the totals reads ECHO / ECH, and the empty echo socket stops looking like a Core.
Docs written: `Source/GeoTrinityUI/Public/HUD/Menu/Gems/CLAUDE.md` ("Echo socket (Loadout page)"), `Source/GeoTrinityUI/Public/HUD/Style/Glyph/CLAUDE.md` (`GeoGemGlyph.h`).
Code done and built (Editor target): `EchoButton`, `EchoText`, `FillTextRole` gone; `UGeoTableWidget::MakeFittingCellText` (ECHO / ECH); `UGeoGemGlyph::SetEchoSocket`, `EchoNestScale` on `FGeoGemStyle`; `gems_page.py` no longer builds `EchoButton`.

## Editor / Blueprint (by hand or script)
- [x] `/Game/HUD/InGameMenu/WBP_GemLoadout` — delete the stale `EchoButton` in `DetailButtons` (the script never removes widgets), then drop its line from `AI/Python/UI/asset_values.json`.
- [ ] Look: REPLACE ALL fits at the normal text role, the echo socket reads as nested outlines, tune `EchoNestScale` on `DA_UITheme` if needed (user's editor check).
