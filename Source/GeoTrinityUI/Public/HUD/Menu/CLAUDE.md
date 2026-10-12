# HUD/Menu

Menu system: a root shows pages; pages wear frames; lists share one frame and row.

| Folder / file | Holds |
|---|---|
| `GeoMainMenuWidget.h` | Lobby menu (`GeoMenuRootWidget`): buttons for create / join / play local / leaderboard / quit; plays the menu music loop |
| `GeoPauseMenuWidget.h` | In-game pause menu, owned by `AGeoPlayerController`; closing from any page resumes; Quit uses `RequestEndPlayMap()` in PIE |
| `Frame/` | Menu skeleton: root, page, frame layers, panel base, buttons |
| `Server/` | Create / browse / local-connect pages |
| `List/` | Shared list frame, panel, row, table, leaderboard |
| `Settings/` | Settings chooser, sound, interface, key bindings |
| `Gems/` | Gems menu: loadout board, Forge |
| `Character/` | Character sheet, ability cards and detail drawer |

## Navigation
- `UGeoMenuRootWidget`: `TopLevel` + every `UGeoMenuPageWidget` in its tree, one shown at a time; keeps the path, so Back retraces and Close leaves all.
- Pages open other pages by class (`OpenPage`); a page never knows who opened it.
- Gamepad: panels hold focus; first d-pad input focuses `GetInitialFocusWidget()`; B / Backspace bubbles to the innermost panel's `HandleBackAction()`.
- Required BP widget names are in each header's class comment.

## Menu music
- `UGeoMainMenuWidget::MusicLoop` (`USoundBase`, set in the BP defaults): `SW_MenuInspiration_Loop`, shipped from `SourceArt/Audio/MenuMusic/`.
- Played 2D through one `UAudioComponent` created in `NativeConstruct`, faded in, never restarted by page changes (the root lives as long as the menu); faded out and released in `NativeDestruct`, so joining or leaving to a level ends it.
- Pause menu has none: music is the lobby's only.
- Routed by the Sound Wave's `SC_Music` class, so the Settings → Sound "Music" slider drives it.
