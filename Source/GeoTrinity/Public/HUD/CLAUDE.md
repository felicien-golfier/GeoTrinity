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
| `GeoChargeBeamGaugeWidget.h` | World-space charge-beam gauge with sweet-spot overlay bar; bound to `ChargeBeamGaugeComponent` on `PlayableCharacter`; ticks from ability's `GetChargeRatio()` |
| `HudFunctionLibrary.h` | `ShouldDrawHUD()`, `GetHealthRatio()` |
| `Component/GeoCombattantWidgetComp.h` | WidgetComponent on actors; auto-inits widget with actor's ASC on BeginPlay |
| `Menu/GeoMenuButton.h` | Reusable styled button; `BlueprintAssignable OnClicked`; appearance fully configurable via `EditAnywhere` props |
| `Menu/GeoMainMenuWidget.h` | Lobby menu; 3× `BindWidget UGeoMenuButton` + `BindWidget UGeoCreateServerWidget` + `BindWidget UGeoBrowseServersWidget`; C++ shows/hides the create-server and browse-server panels and handles quit |
| `Menu/GeoCreateServerWidget.h` | "Create Server" form; fields: ServerNameInput, MapComboBox, SlotsComboBox, LanguageComboBox, PrivacyComboBox, CreateButton, BackButton (all BindWidget); `BlueprintAssignable OnClosed` delegate fires on Back; session creation logic fully in C++; sets SERVER_NAME, LANGUAGE, MAP session keys; data arrays (MapDisplayNames, MapURLs, SlotOptions, LanguageOptions) set via `EditAnywhere` in BP |
| `Menu/GeoBrowseServersWidget.h` | Browse-servers panel; BindWidgets: SearchInput, LanguageComboBox, SearchProgressBar, RefreshButton, BackButton, ServerListScrollBox; `EditAnywhere ServerRowWidgetClass` (set in BP); client-side name filter, server-side language filter; `BlueprintAssignable OnClosed` fires on Back; calls `GeoGameInstance::JoinAdvancedSession` on row select |
| `Menu/GeoServerRowWidget.h` | Single row in the server list; BindWidgets: RowButton, ServerNameText, MapText, PlayersText, PingText; optional FlagImage; raw multicast `OnSelected` delegate carries `FOnlineSessionSearchResult`; call `InitFromSearchResult` after `CreateWidget` |

## Adding HUD Changes
**Screen-space UI** (boss bar, cooldown): add `BlueprintImplementableEvent` to `AGeoHUD` → implement in HUD BP → forward to `OverlayWidget`.

**World-space UI** (deploy gauge): `UWidgetComponent` on character BP, `Space = Screen`. Ability calls `GetAvatarActor → Cast PlayableCharacter → Show/HideDeployChargeGauge`. No HUD involvement.

## `FHudPlayerParams`
Snapshot held by `AGeoHUD`: `PlayerController`, `PlayerState`, `AbilitySystemComponent`, `AttributeSet`. Access via `GetHudPlayerParams()`.
