# UnequipTier — pending

Goal: drop the gem detail's UNEQUIP; a button under the stacks empties every socket of the tier filter's tier (UNEQUIP ALL CUTS; ALL tab = whole build), and the tier tabs stop showing owned counts.
Docs written: `Source/GeoTrinityUI/Public/HUD/Menu/Gems/CLAUDE.md` ("Unequip and tier filter"), `Source/GeoTrinity/Public/Gem/CLAUDE.md` (invariants, `UnequipTier`).
Code, `gems_page.py`, `asset_values.json` and the test are written, unbuilt and untested.

## Left
- [ ] Build (editor closed, `Tools\*.bat`, `GEO_NO_PAUSE=1`), then `Tools\Run_Tests.bat GeoTrinity.Gems`.
- [ ] Run `AI/Python/Gem/gems_page.py` (editor): adds `UnequipTierRow` under the stacks, removes stale `UnequipButton` / `EchoButton` unless hand-edited.

## Status 2026-10-25
Code, test and script exist. Remaining: build + `Run_Tests GeoTrinity.Gems`, run `gems_page.py` in editor.
