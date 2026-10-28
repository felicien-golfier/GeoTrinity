# HUD/Interface

Gameplay → UI seam. Gameplay holds engine base pointers (`UWidgetComponent*`, `UUserWidget*`, `AHUD*`) and calls UI through these; implementers live in `GeoTrinityUI`. Gameplay never names a UI class.

| Interface | Implemented by | Used by |
|---|---|---|
| `GeoHUDInterface.h` | `AGeoHUD` | `AGeoPlayerState`, `AGeoGameState`, encounter actions (`Encounter/Action/CLAUDE.md`) |
| `GeoCombattantWidgetHost.h` | `UGeoCombattantWidgetComp` | `AGeoCharacter`, `AGeoDeployableBase` |
| `GeoChargeGaugeWidgetInterface.h` | deploy / charge-beam gauge widgets | `APlayableCharacter` |
| `GeoChargeBeamGaugeWidgetInterface.h` | `UGeoChargeBeamGaugeWidget` (adds the sweet-spot window) | `APlayableCharacter` |
| `GeoDamageNumberHost.h` | `AGeoDeployableBase` | `AGeoHUD::RegisterASCForDamageNumbers` (skips when `ShowsDamageNumbers()` is false) |
| `GeoLoadingScreenFactory.h` | `SGeoLoadingScreen::Make`, registered by `FGeoTrinityUIModule` | `UGeoGameInstance` on `PreLoadMap` |

- Loading screen (`GeoTrinityUI/Public/LoadingScreen/CLAUDE.md`): `FGeoLoadingScreenRequest` (`MapPackageName`, `TipCategories`, `LocalPlayer`) and the `GeoLoadingScreen` namespace: `SetFactory(Factory)` (empty clears), `Make(Request)` = the factory's Slate widget, null when none is registered (dedicated server, UI module not loaded). A Slate factory, not a class ref: the widget is plain Slate, no `UClass` to name. Called on the game thread only.
- Runtime UI classes come from soft class refs in `GameDataSettings` (`CombattantWidgetComponentClass`, …).
- Tutorial calls on `GeoHUDInterface` (`GeoTrinityUI/.../HUD/Tutorial/CLAUDE.md`): `ShowCatLine(Line, OnEnded)` (board markup, one line at a time; `OnEnded` called once when that line ends, whatever the reason), `HideCatLine()` (ends the shown line now, `OnEnded` called as for any end; nothing shown = no-op), `ShowAbilityPage(AbilityTag, OnClosed)` (pause menu → character sheet → that ability's detail drawer; `OnClosed` called once when the pause menu closes, at once when the page could not open). Reached from an encounter action through the first local player's HUD: the tutorial is a solo listen host, so authority and the local UI share one process.
