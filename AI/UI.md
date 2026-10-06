# UI Rules

Rules for every menu and HUD widget. Widget Blueprint automation lives in `AI/MCP/MCP_UI.md`; the class map is
`Source/GeoTrinityUI/Public/HUD/CLAUDE.md`.

## A style is a reusable type, never a one-off
Every new look is a generic C++ widget or style asset that any widget can pick up, the way `UGeoButton` fills its
sounds from `GameDataSettings` and `UGeoMenuButton` exposes its brushes and font. Never style one widget by hand in
one Blueprint, and never copy a style's values from one Blueprint into another.

- The values live in one place (a style data asset or the project theme). Widgets reference it; changing it
  re-skins every widget using it.
- A widget with no style set falls back to the project default, so a new widget looks right with zero setup.
- A widget may override a value locally only through an exposed property, never by editing its internals.

## The user tweaks values in the Blueprint
Everything a designer would tune is an `EditAnywhere` `UPROPERTY` in a `Geo` category, with `ClampMin`/`ClampMax`
on numbers and a one-line tooltip comment. Colours, sizes, speeds, counts and fonts are never hardcoded in C++
or buried in a graph.

- Changes preview live in the designer: apply the style in `SynchronizeProperties`/`NativePreConstruct`.
- Builder scripts in `AI/Python/UI/` set structure and the first values only; re-running one never resets a value
  the user tuned in a style asset.

## Look
Flat geometry, built for the UI itself: lines, squares, polygons, solid fills. Never reuse or imitate an in-game
material. Draw with Slate primitives in a widget's paint, not with materials or textures, unless a shape cannot be
drawn that way. Animation runs on Slate active timers, never on a `UUserWidget` tick.
