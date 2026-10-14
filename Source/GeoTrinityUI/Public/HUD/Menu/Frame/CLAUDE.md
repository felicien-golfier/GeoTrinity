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
| `GeoMessageSubsystem.h` | `UGeoMessageSubsystem` (game instance): `PostMessage(Text, Color)`; keeps the live messages |
| `GeoMessageLog.h` | `UGeoMessageLog` (`UWidget`, Slate-drawn): the message strip in the bottom row of the window |

- Default click/hover sounds from `GameDataSettings` when the BP style sets none.

## Message log
Short messages telling the player what just happened in the UI (every rule that fires). One system, one place: the
strip in the bottom row of the window, on every frame layer — the menu's top level and every page, with or without BACK.

- **Post**: `UGeoMessageSubsystem::PostMessage` from any UI code; gameplay reaches it through the HUD interface seam, never directly. The subsystem owns the messages (text, colour, post time), so navigating never drops them; closing the menu (`UGeoMenuRootWidget::CloseMenu`, menu destroyed) clears them. Every `UGeoMessageLog` paints the same state, none keeps its own.
- **Place**: `UGeoMessageLog` lives in `WBP_MenuFrame` (layer 1, so layer 2 and the top level both have it), bottom row, from a fixed left inset equal to BACK's width to the window's right padding — same place whether BACK shows or not, lines bottom-anchored, left-aligned. Its height is `MaxLines × LineHeight`, tuned so the stack stays about BACK's height, never taller than the bottom row.
- **Stack**: the bottom line is the oldest live message. A message arriving while others still show appears on the line above the newest and fades in there; when the bottom one expires (fades out), every line eases one slot down, so a lone survivor comes back to the bottom line.
- **Max `MaxLines` (5)**: a 6th message drops the oldest early (it fades out at once), never grows the strip.
- **Life**: `Lifetime` seconds shown, then `FadeTime` fade; every slot move eased over `SlideTime`. Position = function of (now − post time), so pages that open mid-life show the same frame.
- Text in a small theme text role (`TextRole`, `UGeoUITheme::ApplyTextStyle`), colour an `EGeoColor`; one line, clipped, never wrapped.
- Motion on a Slate active timer, never a `UUserWidget` tick (`AI/UI.md`, "Look"); the timer sleeps while no message lives.
- A page shown outside a menu (character sheet on Tab) keeps the strip like the rest; it shows nothing there unless something posts.
- Tunables (`EditAnywhere`, `Geo|Messages`): `MaxLines`, `LineHeight`, `LeftInset`, `Lifetime`, `FadeTime`, `SlideTime`, `TextRole`.
