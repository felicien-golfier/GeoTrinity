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
| `GeoHUD.h` | Main HUD; `InitOverlay()`, `BindToPawn()`, `BuildAbilityBar()`, `ShowBossHealthBar()`, attribute delegates, `GetHudPlayerParams()`, ability-bar data helpers (`GetAbilityCooldown`, `IsAbilityActive`, `GetDeployCountForAbility`); non-shipping debug combat-stats table (top-right, gated by `Geo.ShowCombatStats`) — pure Slate panel (no WBP asset) built in `UpdateCombatStatsPanel()`, cells poll `AGeoPlayerState` via `TAttribute` lambdas, tree rebuilt from `DrawHUD()` only when the player list changes |
| `GeoOverlayWidget.h` | Root player overlay; holds `AbilityBar` as BindWidget so the HUD rebuilds it from C++ without Blueprint wiring |
| `GeoAbilityBarWidget.h` | Bottom-center ability bar widget; builds slots from `GetAbilityBarEntries()`, refreshes deploy badges on HUD ping |
| `GeoAbilitySlotWidget.h` | Single ability slot: icon + radial cooldown sweep (material) + countdown text + optional deploy count badge + live key-binding label (`KeyText`, queried from Enhanced Input each tick) |
| `GeoUserWidget.h` | Base widget; `InitFromHUD(AGeoHUD*)`, `BindCallbacksFromHUD` BP event |
| `GenericCombattantWidget.h` | Reusable health/shield bar for enemies/boss/deployables — **not for player overlay**; `ShieldBar` overlays `HealthBar` (shield = Shield / MaxHealth) |
| `GeoDeployChargeGaugeWidget.h` | World-space deploy charge gauge; ticks from ability's `GetChargeRatio()` |
| `GeoChargeBeamGaugeWidget.h` | World-space charge-beam gauge with sweet-spot overlay bar; bound to `ChargeBeamGaugeComponent` on `PlayableCharacter`; ticks from ability's `GetChargeRatio()` |
| `HudFunctionLibrary.h` | `ShouldDrawHUD()`, `GetHealthRatio()` |
| `Component/GeoCombattantWidgetComp.h` | WidgetComponent on actors; binds widget to owner's ASC on `InitWidget`; `BindWidgetToOwnerASC()` is idempotent — call it again once the ASC becomes available |
| `Menu/GeoMenuButton.h` | Reusable styled button; `BlueprintAssignable OnClicked`; appearance fully configurable via `EditAnywhere` props |
| `Menu/GeoMainMenuWidget.h` | Lobby menu; 3× `BindWidget UGeoMenuButton` + `BindWidget UGeoCreateServerWidget` + `BindWidget UGeoBrowseServersWidget`; C++ shows/hides the create-server and browse-server panels and handles quit |
| `Menu/GeoCreateServerWidget.h` | "Create Server" form; fields: ServerNameInput, MapComboBox, SlotsComboBox, LanguageComboBox, PrivacyComboBox, CreateButton, BackButton (all BindWidget); `BlueprintAssignable OnClosed` delegate fires on Back; session creation logic fully in C++; sets SERVER_NAME, LANGUAGE, MAP session keys; data arrays (MapDisplayNames, MapURLs, SlotOptions, LanguageOptions) set via `EditAnywhere` in BP |
| `Menu/GeoBrowseServersWidget.h` | Browse-servers panel; BindWidgets: SearchInput, LanguageComboBox, SearchProgressBar, RefreshButton, BackButton, ServerListScrollBox; `EditAnywhere ServerRowWidgetClass` (set in BP); client-side name filter, server-side language filter; `BlueprintAssignable OnClosed` fires on Back; calls `GeoGameInstance::JoinAdvancedSession` on row select |
| `Menu/GeoLocalConnectWidget.h` | "Play Local" panel — direct-IP host/join **without Steam** via `UGeoSessionSubsystem`; BindWidgets: HostButton, JoinButton, BackButton (UGeoMenuButton), IPInput, LocalIPText; `EditAnywhere HostMap` (`TSoftObjectPtr<UWorld>`) = map the spawned dedicated server loads; `OnClosed` fires on Back |
| `Menu/GeoServerRowWidget.h` | Single row in the server list; BindWidgets: RowButton, ServerNameText, MapText, PlayersText, PingText; optional FlagImage; raw multicast `OnSelected` delegate carries `FOnlineSessionSearchResult`; call `InitFromSearchResult` after `CreateWidget` |

## Adding HUD Changes
**Screen-space UI** (boss bar, cooldown): add `BlueprintImplementableEvent` to `AGeoHUD` → implement in HUD BP → forward to `OverlayWidget`.

**World-space UI** (deploy gauge): `UWidgetComponent` on character BP, `Space = Screen`. Ability calls `GetAvatarActor → Cast PlayableCharacter → Show/HideDeployChargeGauge`. No HUD involvement. Attach to the actor's `WidgetAnchorComponent` (non-rotating anchor on `AGeoCharacter`/`AGeoDeployableBase`), never to the rotating root — a root-relative offset orbits the actor as it yaws, even in Screen space.

## `FHudPlayerParams`
Snapshot held by `AGeoHUD`: `PlayerController`, `PlayerState`, `AbilitySystemComponent`, `AttributeSet`. Access via `GetHudPlayerParams()`.

## Ability Bar (bottom-center icons + cooldowns + deploy counts)
C++ data layer on `AGeoHUD`; layout is BP (`WBP_AbilityBar` / `WBP_AbilitySlot` under `/Game/HUD/`).
- `GetAbilityBarEntries()` → `TArray<FGeoAbilityBarEntry>` (AbilityTag, InputTag, Icon, bIsDeployable). Iterates the avatar's granted activatable abilities, skips passives, matches the global `UAbilityInfo` entry for the player's class to pull icon/input/`bShowDeployCount`.
- `GetAbilityCooldown(Tag, OutRemaining, OutDuration)` — native `GetCooldownTimeRemainingAndDuration` on the granted spec.
- `IsAbilityActive(Tag)` — `FGameplayAbilitySpec::IsActive()` on the granted spec. The slot keeps the cooldown sweep filled at 1.0 while the ability is active (grayed-out "in use" look); when the ability ends, the cooldown depletes the sweep, or it clears immediately if there is no cooldown.
- `GetDeployCountForAbility(Tag, OutCurrent, OutMax)` — resolves the deploy ability's `DeployableActorClass`, then reads the avatar's `UGeoDeployableManagerComponent`: live count from `GetDeployables<AGeoDeployableBase>()` filtered by class, cap from `DeployableSlots` (fallback `GetMaxDeployables()`).
- `BuildAbilityBar()` — C++ (not a Blueprint event); called from `BindToPawn` once the pawn's granted abilities exist, and from `AGeoPlayerState::OnRep_PlayerClass` after a class change re-grants abilities. Delegates to `UGeoOverlayWidget::BuildAbilityBar` → `UGeoAbilityBarWidget::BuildBar`.
- `OnPlayerDeployCountChanged` (`BlueprintAssignable`, **no args**) — tagless "a deploy count changed" ping, fired from `HandleDeployCountChanged` (bound to the avatar manager's `OnDeployCountChanged` via `AddUniqueDynamic`). The manager count is global and carries no ability tag, so each slot re-queries `GetDeployCountForAbility(its own tag)` on receipt rather than reading the ping's payload. Set `bShowDeployCount` on the deploy ability's `UAbilityInfo` entry to enable a slot's badge.
