# GeoCharacter.h — abstract character base

Implements `IAbilitySystemInterface` and `IGenericTeamAgentInterface`.

## Components (always present)
| Component | Field | Purpose |
|---|---|---|
| `UGeoAbilitySystemComponent` | `AbilitySystemComponent` | GAS — on PlayerState for playable, on self for enemies |
| `UGeoInputComponent` | `GeoInputComponent` | Enhanced Input; ability bindings |
| `UGeoCharacterMovementComponent` | (via `GetGeoMovementComponent()`) | Speed multiplier support |
| `USceneComponent` | `WidgetAnchorComponent` | Non-rotating (absolute rotation) anchor; attach all world widgets here, never to the root — root-relative offsets orbit as the capsule yaws |
| `UGeoCombattantWidgetComp` | `CharacterWidgetComponent` | World-space health bar (under `WidgetAnchorComponent`) |
| `UGeoGameFeelComponent` | `GameFeelComponent` | Hit flash, recoil |

## Key methods
- `InitGAS()` — **subclasses must override** and call `InitAbilityActorInfo`. Called from `PossessedBy` / `OnRep_PlayerState`.
- `GetGeoController()` — returns `AGeoPlayerController*` or nullptr
- `GetGeoMovementComponent()` — typed cast to `UGeoCharacterMovementComponent`
- `DrawDebugVectorFromCharacter(Direction, Message[, Color])` — dev utility

## Team
- `ETeam TeamId` — implements `IGenericTeamAgentInterface`; used by AI perception and `GetAllAgentsWithRelationTowardsActor()`
