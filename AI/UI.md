# UI Rules

Rules for every menu and HUD widget. Widget Blueprint automation lives in `AI/MCP/MCP_UI.md`; the class map is
`Source/GeoTrinityUI/Public/HUD/CLAUDE.md`.

## A style is a reusable type, never a one-off
Every new look is a generic C++ widget or style asset that any widget can pick up, the way `UGeoButton` fills its
sounds from `GameDataSettings` and every frame reads its `UGeoFrameStyle`. Never style one widget by hand in
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

## The style kit
Code in `Source/GeoTrinityUI/Public/HUD/Style/`, assets in `/Game/HUD/Style`, fonts in `/Game/HUD/Assets/Fonts`.

| Piece | Tune it in |
|---|---|
| `UGeoUITheme` (`DA_UITheme`, named in Project Settings > Geo UI): text roles, default and field frame, every input style | the theme asset |
| `UGeoFrameStyle` (`DA_Frame_Button`, `_Panel`, `_Row`, `_Field`, `_Screen`, `_Ornament`, `_Scrim`): outline, glow, fill, grid, runners, idle/active blend | the style asset |
| `UGeoFrame`: a Border wearing a frame style, active on hover/focus or when set | its style, or `FrameStyle` on the instance |
| `UGeoShape`: polygon or circle, hollow or solid, spinning; wears a frame style as its outline when given one | the instance |
| `UGeoText`: a TextBlock in a theme text role | the theme's role |
| `UGeoEditableTextBox`, `UGeoComboBoxString`, `UGeoCheckBox`, `UGeoSlider`, `UGeoProgressBar`, `UGeoScrollBox` | the theme's input styles |

Code that builds text or fields at runtime takes the same roles and frames: `UGeoUITheme::ApplyTextStyle` on a
TextBlock, `FieldFrameStyle` on a frame it wraps around a field. `UGeoMenuButton` and `UGeoListRowWidget` already do.

The scripts in `AI/Python/UI/` rebuild the whole look in order: `import_fonts.py`, `ui_theme.py`, `rail_style.py`,
`rail_main_menu.py`, `rail_sub_panels.py`.
