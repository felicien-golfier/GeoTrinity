# TestAim — pending

Goal: let a driven session (`mcp-epic`) tell the game where the mouse is, so the real ability inputs it can already send as keys (`RightMouseButton`, `LeftMouseButton`, `LeftShift`) land where it wants and the tutorial's Deploy and Special steps can be walked end to end.
Priority: 2
Docs written: `Source/GeoTrinity/Public/Input/CLAUDE.md` ("Test aim"), `AI/MCP/MCP_EpicTesting.md` (Walking the tutorial).

## Code (to implement)
- [x] `Input/GeoInputComponent.h` — `UPROPERTY(EditAnywhere, Category = "GeoInput|Test") bool bTestAimActive = false;` and `FVector2D TestAimWorld`
- [x] `Private/Input/GeoInputComponent.cpp` `UpdateMouseLook` — after the pause-menu / keyboard-mouse gate, when `bTestAimActive`: `LastLookInput = TestAimWorld - FVector2D(GeoCharacter->GetActorLocation())`, `bIsUsingController = false`, return (the cursor is never read)
- [x] clang-format the two files

## Verify
- [x] Tests: none new. `UpdateMouseLook` needs a local player controller and a viewport (no headless layer, `AI/Testing.md`); checked through `mcp-epic`
- [x] Build: `Tools\Build_Editor.bat` with `GEO_NO_PAUSE=1`, editor closed (header change); then restart the editor, `/mcp`
- [x] Heal by `mcp-epic` (2026-10-19): zone landed on the ally, three taps took Health 20 → 200, `Heal` step completed, `Tut_Pad_04_Exit` opened
- [ ] Special: not granted in the rooms reachable (encounter 5 is a Fight room); find the step that unlocks it, then check what a `LeftShift` tap does before deciding on a hold seam
- [ ] Then delete this file
