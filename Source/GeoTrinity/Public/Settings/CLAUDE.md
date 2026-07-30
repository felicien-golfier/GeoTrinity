# Settings

Project- and player-configurable settings objects.

## Files
| File | Role |
|---|---|
| `GameDataSettings.h` | `UDeveloperSettings` — Project Settings panel holding global data assets, GE classes, the color palette and tuning values. Read with `GetDefault<UGameDataSettings>()`; durable values live in `Config/DefaultGame.ini` |
| `GeoGameUserSettings.h` | `UGameUserSettings` — player-facing, saved to `GameUserSettings.ini`. Currently only the couch-coop gamepad choice |

## `GeoGameUserSettings`
Registered via `GameUserSettingsClassName` in `Config/DefaultEngine.ini`; `Get()` `checkf`s on that wiring being present.

`bUseFirstGamepadForSecondPlayer` (default **false**) is a purely local device choice — nothing replicates. Off is solo play unchanged: gamepad and mouse both drive player 1, whichever moved last taking the aim. On, the first gamepad drives a second local player that presses Start to join. It is not polled — `UGeoGameViewportClient::ApplyCouchCoopSetting` turns it into platform-user assignments and must be called after every write; `UGeoKeyBindingsWidget` owns the checkbox and does exactly that.
