# GameClasses/Player

| File | Role |
|---|---|
| `GeoPlayerController.h` | Local-player setup, pause menu, input mapping swap |
| `GeoGameViewportClient.h` | Couch-coop device → player assignment, no splitscreen |

## PlayerController
- Local setup (cursor, input mode, view target, mapping context, key seeding) in `ReceivedPlayer`.
- Pause menu: `ToggleMenuAction` bound only for local player 0; swaps `InputMapping` ↔ `MenuInputMapping` while open.
- Solo pause: in a world whose game mode has `bSoloOnly` (the tutorial, `Match/CLAUDE.md`), `OpenPauseMenu` pauses the game (`SetPause`) and `ClosePauseMenu` unpauses it; any other map keeps running (others are playing). The menu's input actions trigger while paused (`bTriggerWhenPaused`).
- `InteractUIAction` ("Interact with UI in game", in `InputMapping`, rebindable, never a mouse button): bound for local player 0, broadcasts native `OnInteractUI`. In-game UI that takes a press without the menu binds it (first user: Cat line skip, `GeoTrinityUI/.../HUD/Tutorial/CLAUDE.md`). Hard `TObjectPtr`, like every action bound here.
- Cursor and `SeedKeyBindingsForKeyboardLayout` gated on `GeoLib::IsKeyboardMousePlayer`.
- Ticks `ComputePlayerStats` (non-shipping, `Geo.ShowCombatStats`).

## Couch coop (ViewportClient)
- Local coop = a second `ULocalPlayer` (own controller, PlayerState + ASC, HUD, pawn); GAS and replication untouched. This class only decides which device owns which player.
- Character N = local player N = platform user N.
- Config: `WindowsInput.ini` `input.DeviceMappingPolicy=2`, `DefaultInput.ini` `bFilterInputByPlatformUser=True`; policy is read once into a static, so config only.
- `ApplyCouchCoopSetting()` is the whole rule:
  - Seats connected gamepads (device-id order, keyboard excluded) on characters from `FirstCharacter` (0, or 1 with `UseFirstGamepadForSecondPlayer`).
  - Off: gamepad 1 shares character 1 with keyboard; gamepad 2 joins on Start (`Gamepad_Special_Right`).
  - A pad already playing keeps its character; flipping the setting re-seats everyone (`AppliedFirstCharacter`).
  - Local players above 0 with no mapped device left are removed (a disconnected pad still counts).
- Called from `Init`, `UGeoKeyBindingsWidget` on toggle, and `InputKey` when a pad key has no owning local player.
- All platform-user handling lives here (`ApplicationCore` dependency only on this module).
- `SetForceDisableSplitscreen(true)`: one shared view framed by `AGeoGameCamera`; keep `bOffsetPlayerGamepadIds` false.
- `ReceivedFocus` → `SetAllUserFocusToGameViewport()` so a pad's first press reaches the viewport.
