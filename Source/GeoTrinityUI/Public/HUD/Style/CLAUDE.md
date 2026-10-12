# HUD/Style

Themed primitives drawn with Slate (no textures, no materials). One style asset per kind; every widget wearing it changes together. Rules: `AI/UI.md`.

| Folder / file | Holds |
|---|---|
| `GeoUITheme.h` | `DA_UITheme`: text roles, frame / meter / gem styles, menu margins and gaps |
| `GeoUISettings.h` | Project Settings "Geo UI" naming the theme (kept in the UI module so gameplay settings never name a UI type) |
| `GeoText.h` | `TextBlock` whose font, colour, case come from a theme role |
| `GeoThemedInputs.h` | `EditableTextBox` wearing the theme style |
| `Frame/` | Outline / glow / fill / runners around any widget |
| `Meter/` | Ratios: health + shield, class gauge, cooldown sweep, status timer |
| `Glyph/` | Shapes, gems, icons |

- Styles are written by `AI/Python/UI/ui_theme.py` through `asset_guard` (never overwrites hand edits).
