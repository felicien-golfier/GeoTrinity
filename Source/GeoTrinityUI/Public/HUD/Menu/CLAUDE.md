# HUD/Menu

Menu system: a root shows pages; pages wear frames; lists share one frame and row.

| Folder / file | Holds |
|---|---|
| `GeoMainMenuWidget.h` | Lobby menu (`GeoMenuRootWidget`): buttons for create / join / play local / leaderboard / quit |
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
