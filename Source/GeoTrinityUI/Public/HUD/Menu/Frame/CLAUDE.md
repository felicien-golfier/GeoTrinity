# HUD/Menu/Frame

The menu skeleton. Look is themed (`Style/`), never authored per button or page.

| File | Role |
|---|---|
| `GeoMenuRootWidget.h` | A menu navigated by pages; base of main and pause menus |
| `GeoMenuPageWidget.h` | Base of every page; wears `PageFrame`; `OpenPage` / back / close through its menu |
| `GeoMenuFrameWidget.h` | Layer 1: bare window inset by theme `MenuFrameMargin` (`WBP_MenuFrame`); `ContentSlot`, `CornerSlot` |
| `GeoMenuPageFrameWidget.h` | Layer 2: frame + BACK (bottom-left) + close cross (top-right) (`WBP_MenuPageFrame`); page fills `PageSlot` |
| `GeoMenuPanelWidget.h` | Abstract gamepad-navigable base (focus-holding, `GetInitialFocusWidget()`, `HandleBackAction()`) |
| `GeoMenuButton.h` | Themed button: label `TextRole`, optional `Frame` (`FrameStyle` override); `OnClicked` delegate; needs inner `UGeoButton "ButtonWidget"` |
| `GeoButton.h` | `UButton` whose Slate widget treats focus as hover (same brushes, sound, `OnHovered`) |

- Default click/hover sounds from `GameDataSettings` when the BP style sets none.
