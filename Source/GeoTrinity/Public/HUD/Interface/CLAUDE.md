# HUD/Interface

Gameplay → UI seam. Gameplay holds engine base pointers (`UWidgetComponent*`, `UUserWidget*`, `AHUD*`) and calls UI through these; implementers live in `GeoTrinityUI`. Gameplay never names a UI class.

| Interface | Implemented by | Used by |
|---|---|---|
| `GeoHUDInterface.h` | `AGeoHUD` | `AGeoPlayerState`, `AGeoGameState` |
| `GeoCombattantWidgetHost.h` | `UGeoCombattantWidgetComp` | `AGeoCharacter`, `AGeoDeployableBase` |
| `GeoChargeGaugeWidgetInterface.h` | deploy / charge-beam gauge widgets | `APlayableCharacter` |
| `GeoChargeBeamGaugeWidgetInterface.h` | `UGeoChargeBeamGaugeWidget` (adds the sweet-spot window) | `APlayableCharacter` |
| `GeoDamageNumberHost.h` | `AGeoDeployableBase` | `AGeoHUD::RegisterASCForDamageNumbers` (skips when `ShowsDamageNumbers()` is false) |

- Runtime UI classes come from soft class refs in `GameDataSettings` (`CombattantWidgetComponentClass`, …).
