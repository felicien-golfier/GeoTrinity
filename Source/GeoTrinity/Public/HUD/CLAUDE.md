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
| `GeoHUD.h` | Main HUD; `InitOverlay()`, `BindToPawn()`, `BuildAbilityBar()`, `ShowBossHealthBar()`, attribute delegates, `GetHudPlayerParams()`, ability-bar data helpers |
| `GeoOverlayWidget.h` | Root player overlay; holds `AbilityBar` as BindWidget so the HUD rebuilds it from C++ without Blueprint wiring |
| `GeoAbilityBarWidget.h` | Bottom-center ability bar widget; builds slots from `GetAbilityBarEntries()`, refreshes deploy badges on HUD ping |
| `GeoAbilitySlotWidget.h` | Single ability slot: icon + radial cooldown sweep (material) + countdown text + optional deploy count badge |
| `GeoUserWidget.h` | Base widget; `InitFromHUD(AGeoHUD*)`, `BindCallbacksFromHUD` BP event |
| `GenericCombattantWidget.h` | Reusable health bar for enemies/boss/deployables — **not for player overlay** |
| `GeoDeployChargeGaugeWidget.h` | World-space deploy charge gauge; ticks from ability's `GetChargeRatio()` |
| `GeoChargeBeamGaugeWidget.h` | World-space charge-beam gauge with sweet-spot overlay bar; bound to `ChargeBeamGaugeComponent` on `PlayableCharacter`; ticks from ability's `GetChargeRatio()` |
| `HudFunctionLibrary.h` | `ShouldDrawHUD()`, `GetHealthRatio()` |
| `Component/GeoCombattantWidgetComp.h` | WidgetComponent on actors; auto-inits widget with actor's ASC on BeginPlay |

## Adding HUD Changes
**Screen-space UI** (boss bar, cooldown): add `BlueprintImplementableEvent` to `AGeoHUD` → implement in HUD BP → forward to `OverlayWidget`.

**World-space UI** (deploy gauge): `UWidgetComponent` on character BP, `Space = Screen`. Ability calls `GetAvatarActor → Cast PlayableCharacter → Show/HideDeployChargeGauge`. No HUD involvement.

## `FHudPlayerParams`
Snapshot held by `AGeoHUD`: `PlayerController`, `PlayerState`, `AbilitySystemComponent`, `AttributeSet`. Access via `GetHudPlayerParams()`.

## Ability Bar (bottom-center icons + cooldowns + deploy counts)
C++ data layer on `AGeoHUD`; layout is BP (`WBP_AbilityBar` / `WBP_AbilitySlot` under `/Game/HUD/`).
- `GetAbilityBarEntries()` → `TArray<FGeoAbilityBarEntry>` (AbilityTag, InputTag, Icon, bIsDeployable). Iterates the avatar's granted activatable abilities, skips passives, matches the global `UAbilityInfo` entry for the player's class to pull icon/input/`bShowDeployCount`.
- `GetAbilityCooldown(Tag, OutRemaining, OutDuration)` — native `GetCooldownTimeRemainingAndDuration` on the granted spec.
- `GetDeployCountForAbility(Tag, OutCurrent, OutMax)` — resolves the deploy ability's `DeployableActorClass`, then reads the avatar's `UGeoDeployableManagerComponent`: live count from `GetDeployables<AGeoDeployableBase>()` filtered by class, cap from `DeployableSlots` (fallback `GetMaxDeployables()`).
- `BuildAbilityBar()` — C++ (not a Blueprint event); called from `BindToPawn` once the pawn's granted abilities exist, and from `AGeoPlayerState::OnRep_PlayerClass` after a class change re-grants abilities. Delegates to `UGeoOverlayWidget::BuildAbilityBar` → `UGeoAbilityBarWidget::BuildBar`.
- `OnPlayerDeployCountChanged` (`BlueprintAssignable`, **no args**) — tagless "a deploy count changed" ping, fired from `HandleDeployCountChanged` (bound to the avatar manager's `OnDeployCountChanged` via `AddUniqueDynamic`). The manager count is global and carries no ability tag, so each slot re-queries `GetDeployCountForAbility(its own tag)` on receipt rather than reading the ping's payload. Set `bShowDeployCount` on the deploy ability's `UAbilityInfo` entry to enable a slot's badge.
