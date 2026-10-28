# CatExpressions — pending

Goal: the Cat's Slate face pulls expressions from four parameters (eye tilt, squint per eye, ear angle, mouth), and the loading-screen Cat changes face every eighth of the bar.
Priority: 2
Docs written: `GeoTrinityUI/Public/CatFace/CLAUDE.md` ("Expressions"), `GeoTrinityUI/Public/LoadingScreen/CLAUDE.md` ("Bar"), `GeoTrinityEditor/Private/Tests/CLAUDE.md`.
After `LoadingScreen.md` (it moves the face into `CatFace/`).

## Code (to implement)
Read `AI/CodingStyle.md` first.
- [ ] `CatFace/GeoCatExpression.h/.cpp` — `FGeoCatExpression` (`EyeTilt`, `SquintLeft`, `SquintRight`, `EarAngle`, `EGeoCatMouth Mouth`), `EGeoCatMouth` (Smile, Frown, Flat, O, Grin, Open), `EGeoCatExpression` (Neutral, Evil, Smug, Derp, Cackle, Shock, Scheme, Sly, in that order); `GEOTRINITYUI_API GeoCatFace::GetPreset(EGeoCatExpression)`, `Blend(From, To, Alpha)` (numbers lerp, mouth = `To`'s)
- [ ] `CatFace/SGeoCatFace.h/.cpp` — `SetExpression(FGeoCatExpression const&)`: blend from the current values over `ExpressionBlendSeconds` (.25, ease-out, `constexpr`), pop 1.12 over .3 s; paint uses tilt (today a fixed 9°), squint × `GeoCatFaceMotion::GetEyeScale`, ear angle + flick, the mouth shape below; talking still draws the moving mouth
- [ ] `LoadingScreen/GeoLoadingTimeline.h/.cpp` — `GetExpression(float Progress)` = `EGeoCatExpression(FMath::Min(int(Progress * 8), 7))`
- [ ] `LoadingScreen/SGeoLoadingScreen.cpp` — on each eighth change, `SetExpression(GetPreset(...))`
- [ ] clang-format every touched file

## Preset values (Research artifact, Concept 4 `EXPR`; tilt and ear in degrees)
| Preset | Tilt | Squint L | Squint R | Ear | Mouth |
|---|---|---|---|---|---|
| Neutral | 9 | 1 | 1 | 0 | Smile |
| Evil | 24 | .7 | .7 | -12 | Grin |
| Smug | 3 | .75 | .75 | 0 | Smile |
| Derp | 0 | 1.3 | .45 | 6 | O |
| Cackle | 20 | .6 | .6 | -10 | Open |
| Shock | 0 | 1.4 | 1.4 | 12 | O |
| Scheme | 28 | .65 | .65 | -16 | Frown |
| Sly | 15 | 1 | 1 | -6 | Smile |
- The concept also rocks the head (`rock`, -6..9°) and shakes it on Cackle; left out of the four parameters. Add only if the faces do not read without it (ask the user)

## Mouth shapes (200 × 200 box, Ink, stroke 3, round caps / joins)
- Smile: `M86 139 Q93 149 100 139 Q107 149 114 139` (today's closed mouth), no fill
- Frown: `M84 150 Q92 138 100 148 Q108 138 116 150`, no fill
- Flat: `M88 143 L112 143`
- O: ellipse centre (100, 146), rx 7, ry 9, filled
- Grin: `M80 138 Q100 160 120 138 Z` filled + fangs `86,139 93,139 89.5,147` and `114,139 107,139 110.5,147` in Light
- Open: today's talking mouth (`M87 138 Q100 166 113 138 Z`, scaleY by `GetMouthScale`)

## Tests
- [ ] `GeoTrinityEditor/Private/Tests/UI/GeoCatLineTests.cpp` — `GeoTrinityUI.Tutorial.CatExpression`: Neutral = today's face (tilt 9, squints 1, ear 0, Smile); `Blend` at 0 / 1 = ends, at .5 = midpoint, mouth = target's
- [ ] `GeoLoadingScreenTests.cpp` `Timeline` — `GetExpression`: 0 → Neutral, .125 → Evil, .99 and 1 → Sly

## Verify
- [ ] `Tools\Build_Editor.bat` (editor closed, `GEO_NO_PAUSE=1`), `Tools\Run_Tests.bat GeoTrinityUI`
- [ ] Tutorial gate: `GeoTrinity.Tutorial.FullRun` green
- [ ] User looks: `WBP_CatLine` face unchanged (Neutral), loading screen runs the eight faces
