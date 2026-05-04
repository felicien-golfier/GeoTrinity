# HUD

Widget and HUD classes for all UI.

## Architecture
```
AGeoHUD  (owns OverlayWidget)
├── OverlayWidget (UGeoUserWidget) — main player HUD, created in InitOverlay()
│     └── player BP widget inherits UGeoUserWidget directly — NOT UGenericCombattantWidget
└── BossHealthBarWidget (UGenericCombattantWidget) — separate, shown during boss fights
```

## Files
| File | Role |
|---|---|
| `GeoHUD.h` | Main HUD; `InitOverlay()`, `ShowBossHealthBar()`, attribute delegates, `GetHudPlayerParams()` |
| `GeoUserWidget.h` | Base widget; `InitFromHUD(AGeoHUD*)`, `BindCallbacksFromHUD` BP event |
| `GenericCombattantWidget.h` | Reusable health bar for enemies/boss/deployables — **not for player overlay** |
| `GeoDeployChargeGaugeWidget.h` | World-space deploy charge gauge; ticks from ability's `GetChargeRatio()` |
| `HudFunctionLibrary.h` | `ShouldDrawHUD()`, `GetHealthRatio()` |
| `Component/GeoCombattantWidgetComp.h` | WidgetComponent on actors; auto-inits widget with actor's ASC on BeginPlay |

## Adding HUD Changes
**Screen-space UI** (boss bar, cooldown): add `BlueprintImplementableEvent` to `AGeoHUD` → implement in HUD BP → forward to `OverlayWidget`.

**World-space UI** (deploy gauge): `UWidgetComponent` on character BP, `Space = Screen`. Ability calls `GetAvatarActor → Cast PlayableCharacter → Show/HideDeployChargeGauge`. No HUD involvement.

## `FHudPlayerParams`
Snapshot held by `AGeoHUD`: `PlayerController`, `PlayerState`, `AbilitySystemComponent`, `AttributeSet`. Access via `GetHudPlayerParams()`.
