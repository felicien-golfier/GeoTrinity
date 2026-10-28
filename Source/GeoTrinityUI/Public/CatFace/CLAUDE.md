# CatFace

The Cat's animated face, drawn in Slate. Shared by the Cat line popup (`HUD/Tutorial/CLAUDE.md`) and the loading screen (`LoadingScreen/CLAUDE.md`).

| File | Role |
|---|---|
| `SGeoCatFace.h` | Slate leaf painting the face (the Tutorial Board's `catface` / `catfacet` symbols), animated; `GeoCatFace::DefaultPalette`; expressions |
| `GeoCatFaceMotion.h` | Blink / ear flick / mouth loops as pure functions of time; tested |
| `GeoCatExpression.h` | `FGeoCatExpression`, `EGeoCatMouth`, `EGeoCatExpression` presets, `GeoCatFace::GetPreset`, `Blend`; pure, tested |
| `GeoCatFaceWidget.h` | UMG wrapper of `SGeoCatFace`: palette + talking flag as Blueprint properties |

## Shape
- Board face vertex for vertex in a 200×200 design box scaled to the widget: 2 ears + inner ear shades, head (polygon + rounded rect), 2 rounded eyes, nose triangle, mouth. Flat fills, no textures (`HUD/Style/CLAUDE.md`). Vertices live in the file as the single copy of the shape; the Board is the visual reference.
- Face only; the full body, tail and 3D hologram are the Cat hologram slice (`AI/Tutorial/Cat.md`).

## Palette
- The bosses' colour in shades (`AI/Tutorial/Cat.md`): `Base` (head, ears, border), `Shade` (inner ears, darker), `Ink` (mouth, darkest), `Light` (eyes, nose; pale tint, soft halo).
- `GeoCatFace::DefaultPalette` (`SGeoCatFace.h`) = the Board's boss hue, the one copy: `SGeoCatFace` starts with it, `UGeoCatFaceWidget`'s property defaults read it, the loading screen uses it as is.

## Motion
- One Slate active timer, no Tick, same loops as the Board: eye squint / blink 5.4 s, ear flick 5.5 s (second ear offset 0.3 s), mouth open-close 0.38 s while talking.
- Talking = a line is on screen; idle = the expression's mouth. Talking overrides the mouth only.
- Runs on any Slate thread: paint and timer read only the widget's own values (the movie player paints the loading screen off the game thread).

## Expressions
- Four parameters, `FGeoCatExpression`: eye tilt (degrees, mirrored on the right eye), squint per eye (vertical scale, left and right apart), ear angle (degrees, mirrored on the right ear), mouth (`EGeoCatMouth`: Smile, Frown, Flat, O, Grin, Open = the talking mouth, moving).
- Presets `EGeoCatExpression`: Neutral (the face as drawn so far), Evil, Smug, Derp, Cackle, Shock, Scheme, Sly; values from the Research artifact's Concept 4, in `GeoCatExpression.cpp`.
- `SGeoCatFace::SetExpression(Expression)`: tilt, squints and ears blend to the new values over `ExpressionBlendSeconds` (ease-out); the mouth switches at once; a short pop (scale 1.12 over .3 s) marks the change.
- Loops on top: squint × blink scale, ear angle + flick. Talking replaces the mouth with the moving one.
- Loading screen: `GeoLoadingTimeline::GetExpression(Progress)` = the preset of the current eighth, in enum order.
