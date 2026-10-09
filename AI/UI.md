# UI Rules

Rules for every menu and HUD widget. Widget Blueprint automation lives in `AI/MCP/MCP_UI.md`; the class map is
`Source/GeoTrinityUI/Public/HUD/CLAUDE.md`.

## The design is the reference
Every page is drawn first on the GeoTrinity UI design canvas, https://claude.ai/artifact/UrHCWS7YbX3PbbrLX85h1q
(Direction A "Rail"; read it with the Artifact tool, never a web fetch). The game copies it: its layout, controls and
behaviour (where a button sits, what a click or a double click does) are never invented or simplified. Read the
page's file in the canvas before building or changing it, and say so when something cannot match it.

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

## Shared parts are common elements — MANDATORY
- **Reuse is mandatory.** Before building any part of a page, look for an existing common element (a frame layer, a
  button, a list frame, a row, a style) that does it, and use it. Never rebuild what one already does.
- **Making a common element is mandatory.** When a part is used by more than one page — or a page you build has a part
  another page already has — that part becomes one common element (a C++ widget plus its one Blueprint asset), and
  every page using it is switched to it. Two pages that are similar for a part never each carry their own copy.
- A common element that only some pages need in full is split into layers, each usable on its own (the bare window,
  then the window with Back and Close), so a page picks the layer it needs instead of copying one.

## Every table is WBP_Table
Any array of values (the Loadout's total bonus, the Forge's rates, the character sheet's stats) is a `WBP_Table`
(`UGeoTableWidget`): a header naming the columns, then lines of cells framed in `DA_Frame_Cell`, scrolling under the
header. The skin (cell frame, height, padding, gap) is tuned once on `WBP_Table` and re-skins every table.

- A page sets only its instance's `Columns` (caption; width, or 0 to fill what the others leave; alignment) and adds the
  lines at runtime (`ClearLines`, `AddLine`, `MakeCellText`). A line's last cell spans the columns left after it.
- Never lay a table out by hand in boxes, never give a page its own cell style or column sizes.
  `AI/Python/UI/table.py` builds `WBP_Table` and puts it on a page (`table.place`), replacing the widgets it supersedes.

## Every page wears the same window
A menu (`UGeoMenuRootWidget`: the main menu, the pause menu) shows its top level or one page at a time. A page is a
`UGeoMenuPageWidget` and a direct child of its menu's tree, never nested in another page.

- **Back retraces the path.** A page opens another by class (`OpenPage`) and never knows who opened it; the menu keeps
  the path taken, so Back returns to the page it came from, whichever that was. The cross leaves every page at once.
- **One window, one size.** A page's root is `WBP_MenuPageFrame` (layer 2: the window, BACK in its bottom-left
  corner and the close cross in its top-right one, both over the frame's padding so the page keeps its whole area),
  its content in `PageSlot`. A page that must not offer Back and Close wears `WBP_MenuFrame`
  (layer 1: the bare window) instead. A page shown on its own, outside any menu (the character sheet held open with
  Tab), keeps its page frame and hides BACK and the cross itself, so both uses stay one asset. The window is the screen inset by the theme's `MenuFrameMargin`, never sized by
  the content. A menu's top level wears the same `WBP_MenuFrame`, so the frame never changes when a page opens.
- **A compact page sits in a cluster.** A short form (Create Server, Play Local, the settings pages) goes in
  `WBP_MenuCluster`, a section of the window at one width centred in it, never floating loose in the big window.
- A page never draws its own BACK, close button or panel background. `AI/Python/UI/menu_pages.py` converts a page.

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
Real clicks take the user's mouse and focus: do them only when the user asked you to test it yourself, otherwise ask
first (root `CLAUDE.md` rule) and meanwhile run every check that needs neither.

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
`character_sheet.py` (which builds `WBP_Table` with `table.py` first), `gems_page.py`, `interface_settings.py`, `menu_pages.py`, then `space_menu_buttons.py`. `rail_style.py`,
`rail_main_menu.py` and `rail_sub_panels.py` still rebuild whole widgets, so they stop at the first asset that already exists
(`asset_guard.legacy_rebuild`). On the current project, skip them and run the rest.

Any widget showing a combatant's health takes an optional `HealthMeter` and `HealthPercentText`
(`UGenericCombattantWidget`); they are fed outside `UpdateHealthRatio`, which a Blueprint may override.
