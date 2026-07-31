# Settings

Project- and player-configurable settings objects.

## Files
| File | Role |
|---|---|
| `GameDataSettings.h` | `UDeveloperSettings` — Project Settings panel holding global data assets, GE classes, the color palette and tuning values. Read with `GetDefault<UGameDataSettings>()`; durable values live in `Config/DefaultGame.ini` |
| `GeoGameUserSettings.h` | `UGameUserSettings` — player-facing, saved to `GameUserSettings.ini`. Currently only the couch-coop gamepad choice |

## `GeoGameUserSettings`
Registered via `GameUserSettingsClassName` in `Config/DefaultEngine.ini`; `Get()` `checkf`s on that wiring being present.

`bUseFirstGamepadForSecondPlayer` (default **false**) is a purely local device choice — nothing replicates. It only moves where the gamepads *start*: off, the first gamepad shares player 1 with the keyboard and mouse (whichever moved last taking the aim) and the second gamepad presses Start to join as player 2; on, everyone starts one player further along. It is not polled — `UGeoGameViewportClient::ApplyCouchCoopSetting` turns it into platform-user assignments and must be called after every write; `UGeoKeyBindingsWidget` owns the checkbox and does exactly that.
