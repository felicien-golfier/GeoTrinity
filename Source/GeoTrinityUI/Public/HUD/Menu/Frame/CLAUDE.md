# HUD/Menu/Frame

The menu skeleton. Look is themed (`Style/`), never authored per button or page.

| File | Role |
|---|---|
| `GeoMenuRootWidget.h` | A menu navigated by pages; base of main and pause menus |
| `GeoMenuPageWidget.h` | Base of every page; wears `PageFrame`; `OpenPage` / back / close through its menu |
| `GeoMenuFrameWidget.h` | Layer 1: bare window inset by theme `MenuFrameMargin` (`WBP_MenuFrame`); `ContentSlot`, `CornerSlot` |
| `GeoMenuPageFrameWidget.h` | Layer 2: frame + BACK (bottom-left) + close cross (top-right) (`WBP_MenuPageFrame`); page fills `PageSlot` |
| `GeoMenuPanelWidget.h` | Abstract gamepad-navigable base (focus-holding, `GetInitialFocusWidget()` defaults to the first visible enabled button of the tree so a new screen is pad-navigable with no override, `HandleBackAction()`) |
| `GeoMenuButton.h` | Themed button: label `TextRole`, optional `Frame` (`FrameStyle` override); `OnClicked` delegate; needs inner `UGeoButton "ButtonWidget"` |
| `GeoButton.h` | `UButton` whose Slate widget treats focus as hover (same brushes, sound, `OnHovered`) |
| `GeoMessageSubsystem.h` | `UGeoMessageSubsystem` (game instance): `PostMessage(Text, Color)`; keeps the live messages |
| `GeoMessageLog.h` | `UGeoMessageLog` (`UWidget`, Slate-drawn): the message strip in the bottom row of the window |
| `GeoMenuGuide.h` | Pure scrim math: `GeoMenuGuide::MakeScrim(Canvas, Hole)` = the four dim rectangles around a hole; `PlaceHint` |
| `GeoMenuGuideWidget.h` | `UGeoMenuGuideWidget`: the guide overlay (Slate scrim + pulsing frame + hint) and `FindTarget(Controller, Goal)` |

- Default click/hover sounds from `GameDataSettings` when the BP style sets none.

## Message log
Short messages telling the player what just happened in the UI (every rule that fires). One system, one place: the
strip in the bottom row of the window, on every frame layer — the menu's top level and every page, with or without BACK.

- **Post**: `UGeoMessageSubsystem::Post(WorldContext, Text, Color)` from any UI widget (null-safe, finds the subsystem); an identical text still shown as the newest message is not posted again (`DuplicateSeconds`), so a spammed click shows one line; gameplay reaches it through the HUD interface seam, never directly. The subsystem owns the messages (text, colour, post time), so navigating never drops them; closing the menu (`UGeoMenuRootWidget::CloseMenu`, menu destroyed) clears them. Every `UGeoMessageLog` paints the same state, none keeps its own.
- **Place**: `UGeoMessageLog` lives in `WBP_MenuFrame` (layer 1, so layer 2 and the top level both have it), bottom row, from a fixed left inset equal to BACK's width to the window's right padding — same place whether BACK shows or not, lines bottom-anchored, left-aligned. Its height is `MaxLines × LineHeight`, tuned so the stack stays about BACK's height, never taller than the bottom row.
- **Stack**: the bottom line is the oldest live message. A message arriving while others still show appears on the line above the newest and fades in there; when the bottom one expires (fades out), every line eases one slot down, so a lone survivor comes back to the bottom line.
- **Max `MaxLines` (5)**: a 6th message drops the oldest early (it fades out at once), never grows the strip.
- **Life**: `Lifetime` seconds shown, then `FadeTime` fade; every slot move eased over `SlideTime`. Position = function of (now − post time), so pages that open mid-life show the same frame.
- Text in a small theme text role (`TextRole`, `UGeoUITheme::ApplyTextStyle`), colour an `EGeoColor`; one line, clipped, never wrapped.
- Motion on a Slate active timer, never a `UUserWidget` tick (`AI/UI.md`, "Look"); the timer sleeps while no message lives.
- A page shown outside a menu (character sheet on Tab) keeps the strip like the rest; it shows nothing there unless something posts.
- Colours by meaning: refusal `Damage`, side effect `Neutral`, success `Heal`.
- Tunables (`EditAnywhere`, `Geo|Messages`): `MaxLines`, `LineHeight`, `LeftInset`, `Lifetime`, `FadeTime`, `SlideTime`, `TextRole`.

## Menu guide
A guide leads the player along a menu path by showing the one control to click and refusing every other click. Goals (`EGeoMenuGuideGoal`, `GeoTrinity/Public/HUD/Interface/GeoMenuGuideGoal.h`) name a path; the tutorial's Gems and Forge steps start one (`GeoMenuGuideAction`, `Encounter/Action/CLAUDE.md`). Never a menu opened for the player.
- **Stateless resolve**: `UGeoMenuGuideWidget::FindTarget(Controller, Goal)` looks at the pause menu now and answers `FGeoMenuGuideTarget` (`Widget`, `Hint`). Pause menu closed: no widget, the hint is the key that opens it (`ToggleMenuAction`, label through `UHudFunctionLibrary`). Menu at its top level: the root's own target (`UGeoPauseMenuWidget::GetGuideTarget`). A page shown: `UGeoMenuPageWidget::GetGuideTarget(Goal)`; a page with no answer for the goal gets its Back button, so a stray click is undone by following the guide. Closing the menu mid-path, or stepping back, needs no bookkeeping: the next resolve starts over from what is on screen.
- `UGeoMenuRootWidget::GetShownPage()` = the page on top of the path, null at the top level.
- **Overlay**: a native `UUserWidget` (no Blueprint), added by `AGeoHUD` above the pause menu and below the Cat line (`MenuGuideZOrder` 105). Slate scrim = four dim `SBorder`s around the hole (exactly the target's geometry), so the hole has no widget and the click goes through to the real control; every other pixel, the padding ring included, is a hit-testable border that eats the click. A pulsing outline (`SLeafWidget`, Slate active timer, hit-test invisible) rings the hole, `HolePadding` past the control; the hint is a small label above it (below when no room). The rectangles are rebuilt only when the hole moved. No target = an empty canvas: nothing dims, nothing blocks. The timer sleeps while no goal is set (`StartGuide` wakes it).
- **Hold goal**: no target, no hint; the whole screen is one refusing part. The tutorial holds the menu while the Cat finishes a line (after the equip), then starts the next goal. `GeoMenuGuideAction` with `None` stops the guide (the menu is free again, e.g. after the craft).
- **Prepare**: when a page comes on top of the menu under a goal (or the goal changes), the overlay calls `UGeoMenuPageWidget::PrepareForGuide(Goal)` once, so the path starts where it should (the Gems page opens on the Chip tier / filter); the player may change it after. Reset by `StopGuide`.
- The hint standing alone (menu closed: "open the menu with ESC") is centred on the screen, so it is not missed.
- Input: only the mouse is gated. Escape still closes the menu (the guide then waits for it to open again). The gamepad is ungated and the guide never touches focus: pad navigation is the panel base's job (below).
- Tunables (`EditAnywhere`, `GeoMenuGuide`): `ScrimColor`, `FrameColor`, `HolePadding`, `FrameThickness`, `PulseReach`, `PulseSeconds`, `HintTextRole`, `HintGap`, `HintRoom`, `HintPadding`, `OpenMenuFormat`.
- A new path = a goal + `GetGuideTarget` answers on the pages it crosses. Gems: `Gems/CLAUDE.md`.
