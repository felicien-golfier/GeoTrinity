# Settings

| File | Role |
|---|---|
| `GameDataSettings.h` | `UDeveloperSettings`: global data assets, GE classes, colour palette, tuning. `GetDefault<UGameDataSettings>()`; durable values in `Config/DefaultGame.ini` |
| `GeoGameUserSettings.h` | Player settings in `GameUserSettings.ini`: couch-coop gamepad choice, volumes, HUD options |

- `GeoGameUserSettings` registered by `GameUserSettingsClassName` in `Config/DefaultEngine.ini`; `Get()` `checkf`s it.
- `bUseFirstGamepadForSecondPlayer` is local only; applying it = `UGeoGameViewportClient::ApplyCouchCoopSetting()` after every write (done by `UGeoKeyBindingsWidget`).
- Palette: `ColorPalette` is a single `ColorPalette=((Key,(R=..)),…)` line in `[/Script/GeoTrinity.GameDataSettings]`.
- Global data assets come from `GetLoadedDataAsset()`, which `AddToRoot()`s them: `RF_Standalone` protects only in the editor, a package collects every asset nothing hard-references. Never `LoadSynchronous()` a global asset around that helper; never cache a `UObject*` past a frame outside a `UPROPERTY` (holder = file-scope `USTRUCT`; `TObjectPtr` alone keeps nothing alive). Detail: `TroobleshootingLogs.md`.
