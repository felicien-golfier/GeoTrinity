# HUD

Widget and HUD classes for all UI. **This is the `GeoTrinityUI` module** (Type=Runtime), separate from `GeoTrinity` so the dedicated-server target ships no Slate/UMG. Depends on `GeoTrinity`; gameplay must **never** reference a concrete class here.

## Gameplay→UI seam (interfaces)
Gameplay holds engine base pointers (`UWidgetComponent*`, `UUserWidget*`, `AHUD*`) and calls UI behavior via interfaces declared in `GeoTrinity/Public/HUD/Interface/`:
| Interface | Implemented by | Used by |
|---|---|---|
| `IGeoHUDInterface` | `AGeoHUD` | `AGeoPlayerState`, `AGeoGameState` |
| `IGeoCombattantWidgetHost` | `UGeoCombattantWidgetComp` | `AGeoCharacter`, `AGeoDeployableBase` |
| `IGeoDeployGaugeWidgetInterface` | `UGeoDeployChargeGaugeWidget` | `APlayableCharacter` |
| `IGeoChargeBeamGaugeWidgetInterface` | `UGeoChargeBeamGaugeWidget` | `APlayableCharacter` |
| `IGeoDamageNumberHost` | `AGeoDeployableBase` | `AGeoHUD::RegisterASCForDamageNumbers` (skips registration when `ShowsDamageNumbers()` is false) |

The combatant widget component is **created in C++** on `AGeoCharacter`/`AGeoDeployableBase` via `CreateDefaultSubobject` with the runtime class from `GameDataSettings::CombattantWidgetComponentClass` (soft class — gameplay never names the UI type). Per-BP tuning is exposed as gameplay-side fields on the owner instead of the component's Details panel. Gauge components are still added in Blueprint.

## Architecture
```
AGeoHUD  (owns OverlayWidget)
├── OverlayWidget (UGeoOverlayWidget) — main HUD, created in InitOverlay()
│     ├── AbilityBar (UGeoAbilityBarWidget)
│     └── StatusBar (UGeoStatusBarWidget)
└── BossHealthBarWidget (UGenericCombattantWidget) — shown during boss fights
```

## Files
| File | Role |
|---|---|
| `GeoHUD.h` | Main HUD; `InitOverlay`, `BindToPawn`, `BuildAbilityBar`, `ShowBossHealthBar`, ability-bar data helpers; `RegisterASCForDamageNumbers` binds Health/Shield deltas to spawn floating numbers from `DamageNumberPool`; non-shipping debug combat-stats panel (pure Slate, `Geo.ShowCombatStats`), rebuilt only when player list changes |
| `GeoOverlayWidget.h` | Root player overlay; `AbilityBar`/`StatusBar` as BindWidgets, driven from C++ |
| `GeoAbilityBarWidget.h` | Bottom-center bar; builds slots from `GetAbilityBarEntries()` |
| `GeoAbilitySlotWidget.h` | One slot: icon + cooldown sweep + countdown + deploy badge + live key label. Holds **all** abilities sharing its InputTag; shows last active/activatable, else first (e.g. Square's channel↔detonate swap) |
| `GeoStatusBarWidget.h` | Active-effect icon row, local player only. Pure C++ tree, polls `AGeoHUD::GetActiveEffectIcons()`; one icon per active GE with a set `Icon`, plus a synthetic gauge entry for Circle's sweet-spot charge passive |
| `GeoUserWidget.h` | Base widget; `InitFromHUD`, `BindCallbacksFromHUD` BP event |
| `GenericCombattantWidget.h` | Reusable health/shield bar for enemies/boss/deployables (not player overlay); `ShieldBar` overlays `HealthBar`; registers for floating damage numbers via `InitializeWithAbilitySystemComponent` |
| `GeoDamageNumberWidget.h` | Pooled floating damage/heal number; `Activate` applies jitter + upward drift, `NativeTick` re-projects world anchor + fades; `ReturnToPool()` resets availability |
| `GeoDeployChargeGaugeWidget.h` | World-space deploy charge gauge, ticks from ability's `GetChargeRatio()` |
| `GeoChargeBeamGaugeWidget.h` | World-space charge-beam gauge + sweet-spot overlay; gradient bands shade toward center while sweet-spot passive gauge is full |
| `HudFunctionLibrary.h` | `ShouldDrawHUD()`, `GetHealthRatio()` |
| `Component/GeoCombattantWidgetComp.h` | WidgetComponent on actors; implements `IGeoCombattantWidgetHost`; `BindToOwnerASC()` idempotent, call again once ASC available. Created in C++ from `GameDataSettings` soft class |
| `Menu/GeoMenuPanelWidget.h` | Abstract gamepad-navigable menu base — focus-holding, first d-pad input focuses `GetInitialFocusWidget()`. Base of `GeoMenuButton`, `GeoPauseMenuWidget`, `GeoMainMenuWidget`, `GeoSettingsWidget` |
| `Menu/GeoButton.h` | `UGeoButton : UButton` — gamepad focus renders/sounds via the mouse-hover path; fills default click/hover sounds from `GameDataSettings` if the BP style doesn't set them |
| `Menu/GeoMenuButton.h` | Reusable styled button; inner `UGeoButton` receives forwarded focus |
| `Menu/GeoMainMenuWidget.h` | Lobby menu; shows/hides create/browse-server panels, handles quit |
| `Menu/GeoCreateServerWidget.h` | "Create Server" form; session creation logic in C++; sets SERVER_NAME/LANGUAGE/MAP session keys |
| `Menu/GeoBrowseServersWidget.h` | Browse-servers panel; client-side name filter, server-side language filter |
| `Menu/GeoLocalConnectWidget.h` | "Play Local" panel — direct-IP host/join **without Steam** via `UGeoSessionSubsystem`; `HostMap` = listen-server travel target |
| `Menu/GeoServerRowWidget.h` | Server list row; `OnSelected` carries `FOnlineSessionSearchResult`; call `InitFromSearchResult` after `CreateWidget` |
| `Menu/GeoPauseMenuWidget.h` | Pause menu, owned/shown by `AGeoPlayerController`; Quit uses `GEditor->RequestEndPlayMap()` in PIE, else `QuitGame` |
| `Menu/GeoSettingsWidget.h` | Settings chooser; shows one sub-panel at a time |
| `Menu/GeoSoundSettingsWidget.h` | Sound settings; `MasterVolumeSlider` is a placeholder — no audio mixer wired yet |
| `Menu/GeoKeyBindingsWidget.h` | Key-bindings window; rebuilt from Enhanced Input active profile, fixed `MappingOrder` table in .cpp |

## Adding HUD Changes
**Screen-space** (boss bar, cooldown): add `BlueprintImplementableEvent` to `AGeoHUD` → implement in HUD BP → forward to `OverlayWidget`.
**World-space** (deploy gauge): `UWidgetComponent` on character BP (`Space=Screen`). Attach to the actor's `WidgetAnchorComponent` (non-rotating), never the rotating root — a root-relative offset orbits the actor as it yaws even in Screen space.

## Ability Bar
C++ data layer on `AGeoHUD`, layout is BP (`WBP_AbilityBar`/`WBP_AbilitySlot`).
- `GetAbilityBarEntries()` — iterates avatar's granted activatable abilities, skips passives, matches class's `UAbilityInfo` entry.
- `GetAbilityCooldown` / `IsAbilityActive` — slot keeps cooldown sweep filled at 1.0 while active, then depletes on end (or clears immediately if no cooldown).
- `GetDeployCountForAbility` — reads `UGeoDeployAbility` charges; badge shows current, slot only grays out at `OutCurrent==0` — a charge available reads as ready even while refill ticks. Not the live-deployable count.
- `BuildAbilityBar()` — C++, called from `BindToPawn` and `OnRep_PlayerClass` after class change.
- `OnPlayerDeployCountChanged` — tagless ping; each slot re-queries its own tag rather than reading a payload.
