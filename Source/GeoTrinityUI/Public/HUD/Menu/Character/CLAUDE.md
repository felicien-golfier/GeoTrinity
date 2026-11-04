# HUD/Menu/Character

| File | Role |
|---|---|
| `GeoCharacterSheetWidget.h` | Level and XP meters, ability rows (`WBP_AbilityRow`, `WBP_CharacterSheet`); entry to the Gems menu. The shared echo level (`Gem/CLAUDE.md`, "Echo socket") shows on the level line of every class once it is 1 or more, a class under 20 keeping its own next-level text and XP meter |
| `GeoAbilityCardWidget.h` | One ability on the abilities page, the sheet's rows and the drawer: icon tile in class colour (passives `PassiveColor`), name, slot name (`KeyText`, one colour for every ability), input key in its own cap (`InputKeyFrame` / `InputKeyText`, `InputKeyColor`, the key's colour under the ability bar; none on a passive), cooldown or fire delay (a passive: cooldown only), token-resolved description; Reload buffs one line each with colour swatch |
| `GeoAbilityDetailWidget.h` | Drawer slid in from the right on a picked card: the card's content, description + `GetResolvedStats()` in a `StatColumns`-wide `StatGrid`; closes on outside click |

- Rich-text styles come from the data table set on the widget (`DT_AbilityText…`); layout, fonts, colours in the WBPs.
- Descriptions resolve `{Token}`s (`AI/AbilityDescriptions.md`).
- Open on an ability: `UGeoCharacterSheetWidget::OpenAbilityDetail(AbilityTag)` picks that ability's row as a click would; `UGeoPauseMenuWidget::OpenOnPage` opens the pause menu on the sheet first (`AGeoHUD::ShowAbilityPage`, tutorial unlocks). The details are then locked open (`bDetailLocked`: outside click, back input and other cards do nothing) and `AGeoHUD::ShowAbilityPage` runs the `AbilityDetail` menu guide (`Frame/CLAUDE.md`): only the drawer and the window's close cross stay lit. The player closes the window, never the description.
