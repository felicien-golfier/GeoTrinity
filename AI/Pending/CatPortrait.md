# CatPortrait — pending

Goal: the Cat that speaks in the game's chat box is exactly the Tutorial Board's Cat face (same shape, same motion), in the bosses' colour shades, shown like the Board's speech box.
Docs written: `GeoTrinityUI/Public/HUD/Tutorial/CLAUDE.md` ("Cat face"), `AI/Tutorial/Cat.md` ("In game", colour roles).

## Code (to implement)
Read `AI/CodingStyle.md` first.
- [x] `HUD/Tutorial/SGeoCatFace.h/.cpp` — `SLeafWidget`; `OnPaint` draws the face from the Board's vertices (below) scaled from a 200×200 box; args `Base / Shade / Ink / Light` (`FLinearColor`), `bTalking`; active timer drives blink, ear flick, mouth (loops in the doc); desired size from a `Size` arg
- [x] `HUD/Tutorial/GeoCatFaceWidget.h/.cpp` — `UWidget` wrapper (`RebuildWidget`, `SynchronizeProperties`); `EditAnywhere` Base / Shade / Ink / Light, `SetTalking(bool)`
- [x] `HUD/Tutorial/GeoCatLineWidget.h/.cpp` — optional-looking `BindWidget` `CatFace` (`UGeoCatFaceWidget`): `SetTalking(true)` in `ShowLine`, false when the line ends or is skipped; update the header doc comment
- [x] clang-format every touched file

## Face geometry (copy from the Board, `AI/Artifacts.md` Tutorial Board, `FACE()` in the page script; 200×200 box, y down)
- Ears: `44,60 56,16 96,52` and `156,60 144,16 104,52` (Base); inner `58,26 70,56 50,54` and `142,26 130,56 150,54` (Shade, 60 % opacity)
- Head: polygon `38,64 162,64 178,104 168,112 180,128 166,138 170,152 100,176 30,152 34,138 20,128 32,112 22,104` + rounded rect x22 y60 w156 h96 r44 (Base)
- Eyes (Light, halo): path `M(x-9) 120 L(x-9) 101 Q(x-9) 92 x 92 Q(x+9) 92 (x+9) 101 L(x+9) 120 Q x 102.6 (x-9) 120 Z`, x = 70 rotated 9° about (70,106), x = 130 rotated -9° about (130,106); squint = scaleY about the eye centre: 1 → .55 (44-56 %) → 1 (60-90 %) → .08 (94 %) → 1 (97 %) of 5.4 s
- Nose: `95,128 105,128 100,134` (Light, 90 %)
- Mouth closed: `M86 139 Q93 149 100 139 Q107 149 114 139`, Ink stroke 3, round caps, no fill. Mouth talking: `M87 138 Q100 166 113 138 Z`, Ink fill + stroke 3, scaleY .35 ↔ 1 about its top edge, 0.38 s alternating
- Ears flick: rotate about the bottom centre, 0 → -5° (90 %) → 2° (94 %) → 0 of 5.5 s
- Slate has no curves in a simple painter: tessellate the quads / rounded rect into polygons (`FSlateDrawElement::MakeLines` / a convex fan through `MakeCustomVerts`, or `FSlateBrush` rounded box for the rect)

## Editor / Blueprint (by hand or script)
- [x] `WBP_CatLine` (`AI/Python/Tutorial/UI/tutorial_hud.py`, `AI/UI.md` hand-edit protection): add `CatFace` left of `LineText` in a horizontal box, square ~56 px, name label "CAT" above the line (theme role like the Board's `.who`), panel brush dark translucent + 1 px Base border (not done: the frame is still `DA_Frame_Panel`)
- [x] Palette defaults on `CatFace`: Base = the hex boss colour (placeholder = Board `--boss` #ff5a3c until the exact boss slot is chosen, `AI/Pending/Tutorial.md` open design "Cat palette"), Shade ≈ 55 % value, Ink ≈ 12 % value, Light ≈ 85 % toward white
- [ ] Reconcile with the user: confirm the shades look right next to the real hex boss

## Tests
- [x] none headless for the paint; `GeoTrinityUI` test of the pure motion curves if they live in a pure helper (blink / ear / mouth value at time t): `GeoTrinityUI.Tutorial.CatFaceMotion`

## Verify
- [x] `Tools\Build_Editor.bat` (editor closed, `GEO_NO_PAUSE=1`), `Tools\Run_Tests.bat GeoTrinityUI` (2026-10-21: 4/4)
- [ ] Look: user checks in game / bridge probe `Python/Runtime/pie_layout_audit.py`; no real-click PIE unless asked
