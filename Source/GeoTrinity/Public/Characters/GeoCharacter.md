# GeoCharacter.h — abstract character base

Implements `IAbilitySystemInterface` and `IGenericTeamAgentInterface`.

## Components (always present)
| Component | Field | Purpose |
|---|---|---|
| `UGeoAbilitySystemComponent` | `AbilitySystemComponent` | GAS — on PlayerState for playable, on self for enemies |
| `UGeoInputComponent` | `GeoInputComponent` | Enhanced Input; ability bindings |
| `UGeoCharacterMovementComponent` | (via `GetGeoMovementComponent()`) | Speed multiplier support |
| `USceneComponent` | `WidgetAnchorComponent` | Non-rotating (absolute rotation) anchor; attach all world widgets here, never to the root — root-relative offsets orbit as the capsule yaws |
| `UWidgetComponent` (base) | `CharacterWidgetComponent` | World-space health bar. Concrete `UGeoCombattantWidgetComp` is in `GeoTrinityUI`; created as a **C++ default subobject in the constructor** under `WidgetAnchorComponent`, with the subobject class + WBP loaded from `UGameDataSettings` (`CombattantWidgetComponentClass`, `DefaultCharacterHealthBarWidgetClass`) so gameplay never names the UI type. Optional subobject → skipped on the dedicated server. Editable per-BP in the component tree. Bind/visibility go through `IGeoCombattantWidgetHost` / engine base API |
| `UGeoGameFeelComponent` | `GameFeelComponent` | Hit flash, recoil |

## Key methods
- `InitGAS()` — **subclasses must override** and call `InitAbilityActorInfo`. Called from `PossessedBy` / `OnRep_PlayerState`.
- `GetGeoController()` — returns `AGeoPlayerController*` or nullptr
- `GetGeoMovementComponent()` — typed cast to `UGeoCharacterMovementComponent`
- `DrawDebugVectorFromCharacter(Direction, Message[, Color])` — dev utility

## Team
- `ETeam TeamId` — implements `IGenericTeamAgentInterface`; used by AI perception and `GetAllAgentsWithRelationTowardsActor()`

## Floating-bar visibility (listen-server trap)
The floating bar is hidden only over the **local human player's own avatar** (it uses the main HUD overlay). `BeginPlay` gates on `GeoLib::IsLocalPlayerAvatar(this)` — **not `IsLocallyControlled()` alone**: on a listen server the host's AI pawns are *also* locally controlled, so the plain check wrongly hid every host enemy's bar while clients (non-local proxies) still showed it. Use `IsLocalPlayerAvatar` for any "my own pawn" cosmetic.
