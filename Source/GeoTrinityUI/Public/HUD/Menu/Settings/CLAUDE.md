# HUD/Menu/Settings

Pages whose Back returns to the chooser. Values read from and written straight to `UGeoGameUserSettings`.

| File | Role |
|---|---|
| `GeoSettingsWidget.h` | Chooser: opens Sound, Key Bindings or Interface page |
| `GeoSoundSettingsWidget.h` | One slider per `EGeoVolumeChannel` (General, Effects, Music, Interface); applied and saved at once |
| `GeoInterfaceSettingsWidget.h` | HUD options (combat stats checkbox) |
| `GeoKeyBindingsWidget.h` | Rebuilt from the Enhanced Input active profile (fixed `MappingOrder` in .cpp); key button arms then `CommitKey` writes `UEnhancedInputUserSettings`; optional `SecondPlayerGamepadCheckBox` → `SetUseFirstGamepadForSecondPlayer` then `UGeoGameViewportClient::ApplyCouchCoopSetting()` |

- Checkbox built by `AI/Python/UI/second_player_gamepad_toggle.py`.
