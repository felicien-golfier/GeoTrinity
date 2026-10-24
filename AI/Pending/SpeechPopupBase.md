# SpeechPopupBase — pending

Goal: every in-game chat popup (the Cat line now, other speakers later) shares ONE base widget, so the F skip key cap can never be missing from a line.
Docs written: `GeoTrinityUI/Public/HUD/Tutorial/CLAUDE.md` ("Speech popup", "Cat line"), `GeoTrinity/Public/GameClasses/Player/CLAUDE.md` (`InteractUIAction` in both contexts).

## Done
- [x] Code, test `GeoTrinityUI.Tutorial.DefaultMappedKey`, build and headless tests (verified by the user)
- [x] `/Game/Input/IMC_Menu`: `IA_InteractUI` was missing (cause of the absent cap and the dead F over a menu); mapped to F + `Gamepad_FaceButton_Top`, saved
- [x] `WBP_CatLine` compiles on the new base (`SkipHint` / `SkipKey` present), untouched on disk

## Verify (user, PIE)
- [ ] A Cat line said over an ability page shows the F cap and obeys F (the case that lacked it)
