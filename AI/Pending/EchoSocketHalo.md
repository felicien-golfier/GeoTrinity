# EchoSocketHalo — pending

Goal: make the Echo gem socket the highlight of the Gems loadout board: a big animated halo of squares (rosaces turning against each other, runner squares like the button frames), inspired by the reference image (dark disc with a square gem inside a white spiky rosace).
Priority: 3
Docs written: `Source/GeoTrinityUI/Public/HUD/Menu/Gems/CLAUDE.md` ("Echo socket look", "Halo states"), `Source/GeoTrinityUI/Public/HUD/Style/Glyph/CLAUDE.md` (`GeoEchoHalo.h`).
Code done and built (Editor target): `GeoEchoHalo.h/.cpp`, halo built in `UGeoGemLoadoutWidget::BuildBoard`, driven by `ShowEchoHalo`.

## Editor
- [x] Run `AI/Python/Gem/gems_page.py`: it sets `EchoHaloStyle` (`DA_Frame_Button`), `EchoSize` 56 and `EchoHaloSize` 168 on `WBP_GemLoadout` (through `asset_guard`), then save dirty assets. Until then the halo ensures and draws nothing.
- [ ] Judge the halo clears the neighbouring clusters; tune `EchoHaloSize` / `SquareCount` if not.

## Tests
- [ ] None automated (pure drawing); look judged on the loadout page when the user asks for an editor check.
