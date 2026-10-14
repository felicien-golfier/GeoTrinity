# AI/Python/UI

Widget Blueprints and trees. Docs: `AI/UI.md`, `AI/MCP/MCP_UI.md`. Every write goes through `asset_guard` (never overrides a hand edit).

## Toolkit
| Script | Does |
|---|---|
| `asset_guard.py` | `write` sets a value only while it still holds what a script last wrote (`asset_values.json`); `earlier=`, `report`, `legacy_rebuild` |
| `rail_style.py` | Themed widgets on frames; swap a widget for its themed class keeping name / slot / values; wrap in a frame |
| `ui_theme.py` | `DA_UITheme` + every frame style asset, named in Project Settings; run first |
| `wings_hud.py` | In-game HUD (Wings look): meter styles, player wing, team list, stats panel, boss rail, slots, status tiles in `WBP_MainOverlay` |

## Shared parts (run in this order)
| Script | Does |
|---|---|
| `list_row.py` | The row every list is built from |
| `list_panel.py` | The list frame; server browser re-rooted onto it |
| `table.py` | `WBP_Table`, the one table every page uses; `place()` |
| `menu_pages.py` | `WBP_MenuFrame`, `WBP_MenuPageFrame`, every page in it, main / pause `TopLevel` |

## Pages
| Script | Does |
|---|---|
| `rail_main_menu.py` | Main menu: rail, turning hex shell, title, buttons |
| `rail_sub_panels.py` | Every sub-panel: themed texts / inputs, frames, pause scrim |
| `character_sheet.py` | Level / XP meters, ability rows, sheet |
| `ability_page.py` | Abilities page, `WBP_AbilityCard`, card grid |
| `ability_detail.py` | Detail drawer (`WBP_AbilityDetail`) and scrim |
| `leaderboard_menu.py` | Leaderboard on the list frame, main-menu entry, fight timer (after `list_panel.py`) |
| `interface_settings.py` | Interface settings page + button |
| `space_menu_buttons.py` | Re-space the menu button columns of every widget to the theme gap |
| `passive_bar.py` | `WBP_PassiveSlot` (copy of the ability slot, no key / count) and `PassiveBox` + `PassiveSlotWidgetClass` on `WBP_AbilityBar`; headless |
| `slot_gauge.py` | `GaugeMeter` (`DA_Meter_SlotGauge`) + `GaugeText` in `WBP_AbilitySlot` and `WBP_PassiveSlot`, between `Icon` and `CooldownMeter`; headless, after `wings_hud.py` / `passive_bar.py` |
| `xp_strip.py` | `WBP_XpStrip` (level, echo level, XP meter) + `XpStrip` under the health stack of `WBP_PlayerWing`; headless, after `wings_hud.py` |
| `message_log.py` | `UGeoMessageLog` "MessageLog" in the bottom row of `WBP_MenuPageFrame`, right of `BackButton`; headless, after `menu_pages.py` |
| `import_icons.py` | `DA_Icon_*` from `SourceArt/Icons`, set on abilities and effects |
| `import_fonts.py` | Menu fonts as Font Face + Font (answer Yes) |
