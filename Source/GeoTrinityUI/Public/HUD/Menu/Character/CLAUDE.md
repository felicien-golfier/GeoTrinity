# HUD/Menu/Character

| File | Role |
|---|---|
| `GeoCharacterSheetWidget.h` | Level and XP meters, ability rows (`WBP_AbilityRow`, `WBP_CharacterSheet`); entry to the Gems menu. The shared echo level (`Gem/CLAUDE.md`, "Echo socket") shows on the level line of every class once it is 1 or more, a class under 20 keeping its own next-level text and XP meter |
| `GeoAbilityCardWidget.h` | One ability on the abilities page: icon tile in class colour (passives `PassiveColor`), name, key / passive tag, cooldown or fire delay, token-resolved description; Reload buffs one line each with colour swatch |
| `GeoAbilityDetailWidget.h` | Drawer slid in from the right on a picked card: description + `GetResolvedStats()` in a `StatColumns`-wide `StatGrid`; closes on outside click |

- Rich-text styles come from the data table set on the widget (`DT_AbilityText…`); layout, fonts, colours in the WBPs.
- Descriptions resolve `{Token}`s (`AI/AbilityDescriptions.md`).
