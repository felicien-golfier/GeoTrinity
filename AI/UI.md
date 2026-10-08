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

## A script never overwrites a hand edit
General rule for every builder script: a script may overwrite a value only while it still holds what a script last
wrote there; a value changed by hand since is kept. Scripts set values through `asset_guard.write`, which records each
written value (`AI/Python/UI/asset_values.json`), skips a value that no longer matches, and lists it in
`asset_guard.report()` — carry it into the script or ask the user. A value with no record is written only on an asset
the run created, when it already equals the script's value, or when it equals a value an older script wrote before
the ledger existed (`earlier=`). A script that still rebuilds whole widgets
(`asset_guard.legacy_rebuild`) refuses an existing asset until converted.

## Layout comes with the tools, never per widget
A layout rule lives in the builder tooling, so no script and no widget can forget it.

- **Buttons sit at one regular gap.** `UGeoWidgetBuilderUtil::CommitTree`, which every builder ends with, spaces the
  menu buttons of each vertical and horizontal box by the theme's `MenuButtonGap`. Never pad a button to space it.
- **Every element lives in a fixed area.** A page is sized by its parent (fill the slot it is given), never by its
  content, and nothing is drawn over anything else.
- **What grows with data scrolls.** A list, ability rows, wrapped descriptions: put it in a scroll area
  (`wings_hud.scrolled`, a `UGeoScrollBox` filling the room left) instead of letting it overflow.

## Check your own work, every time
The user never reports an obvious bug. After every UI change, before calling it done: open each touched page in a
fresh PIE, click every button on it with real mouse input (`win.ps1 slowclick`-style Win32 clicks, not handler
calls), screenshot it, and run `AI/Python/Runtime/pie_layout_audit.py` on it — it lists texts drawn over each other
and widgets spilling out of their area. Fix and repeat until the audit says OK and every click did its job.

## A restyle never changes behaviour, and is proven not to
A new look is only done once every interactive element of the restyled widget has been used in a fresh PIE and
did its job: every tab, filter and button changes what it should, not just its own highlight. A screenshot proves
the look, never the logic.

- Test in a PIE started after the last code change. Never Live Code while PIE runs, and never Live Code a header
  change: reinstanced widgets keep painting while their UMG side drops its Slate link, so a list keeps showing old
  rows after a click. A header change takes a full build and a fresh editor.

## Look
Flat geometry, built for the UI itself: lines, squares, polygons, solid fills. Never reuse or imitate an in-game
material. Draw with Slate primitives in a widget's paint, not with materials or textures, unless a shape cannot be
drawn that way. Animation runs on Slate active timers, never on a `UUserWidget` tick.

## The style kit
Code in `Source/GeoTrinityUI/Public/HUD/Style/`, assets in `/Game/HUD/Style`, fonts in `/Game/HUD/Assets/Fonts`.

| Piece | Tune it in |
|---|---|
| `UGeoUITheme` (`DA_UITheme`, named in Project Settings > Geo UI): text roles, default and field frame, every input style | the theme asset |
| `UGeoFrameStyle` (`DA_Frame_Button`, `_Panel`, `_Row`, `_Field`, `_Screen`, `_Backdrop`, `_Ornament`, `_Scrim`): outline, glow, fill, grid, runners, idle/active blend | the style asset |
| `UGeoFrame`: a Border wearing a frame style, active on hover/focus or when set | its style, or `FrameStyle` on the instance |
| `UGeoShape`: polygon or circle, hollow or solid, spinning; wears a frame style as its outline when given one | the instance |
| `UGeoText`: a TextBlock in a theme text role | the theme's role |
| `UGeoIcon` (`/Game/HUD/Icons/DA_Icon_*`, data in the gameplay module so effects can name it): a vector icon in one `EGeoColor`, faint strokes as secondary | the icon asset; its shapes in `SourceArt/Icons/*.svg` |
| `UGeoIconImage`: draws a `UGeoIcon` at a size, times a tint | the instance |
| `UGeoMeterStyle` (`DA_Meter_*`): a bar, ring or sweep — track, fill, glow, shield overhang, ticks, outline | the style asset |
| `UGeoMeter`: a ratio in a meter style, its fill tinted per class (`FillTint`) | its style, or the instance |
| `UGeoEditableTextBox`, `UGeoComboBoxString`, `UGeoCheckBox`, `UGeoSlider`, `UGeoProgressBar`, `UGeoScrollBox` | the theme's input styles |

Code that builds text or fields at runtime takes the same roles and frames: `UGeoUITheme::ApplyTextStyle` on a
TextBlock, `FieldFrameStyle` on a frame it wraps around a field. `UGeoMenuButton` and `UGeoListRowWidget` already do.

An icon is drawn, never a texture: its shapes live in an SVG under `SourceArt/Icons` (the subset `import_icons.py`
parses), its colour is a game colour named on the SVG root (`data-geo-color`) and tuned on the asset. A new icon is a
new SVG and a re-run of `import_icons.py`.

The scripts in `AI/Python/UI/` build the look in this order: `import_fonts.py`, `ui_theme.py`, `rail_style.py`,
`rail_main_menu.py`, `rail_sub_panels.py`, `import_icons.py`, `ability_page.py`, `ability_detail.py`, `wings_hud.py`,
`character_sheet.py`, `gems_page.py`, `interface_settings.py`, then `space_menu_buttons.py`. `rail_style.py`,
`rail_main_menu.py` and `rail_sub_panels.py` still rebuild whole widgets, so they stop at the first asset that already exists
(`asset_guard.legacy_rebuild`). On the current project, skip them and run the rest.

Any widget showing a combatant's health takes an optional `HealthMeter` and `HealthPercentText`
(`UGenericCombattantWidget`); they are fed outside `UpdateHealthRatio`, which a Blueprint may override.
