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

## Rotation
`SetTargetYaw(float)` sets the yaw the character turns toward; `Tick` closes the gap at up to `MaxRotationSpeed` (deg/s, default 720) each frame via `Controller->SetControlRotation`. `TargetYaw` is initialized from the actor's spawn rotation in `BeginPlay` so nothing snaps on possession.
**All facing must go through `SetTargetYaw` — never call `Controller::SetControlRotation` or `AIController::SetFocus` directly**, since `AAIController::UpdateControlRotation` snaps the control rotation straight to the focal point every frame, bypassing any clamp. Callers: `APlayableCharacter::UpdateAimRotation` (aim input), `FSTTask_ChaseTarget::Tick`, `UGeoAITask_MoveTo::TickTask` (faces along velocity).

## Team
- `ETeam TeamId` — implements `IGenericTeamAgentInterface`; used by AI perception and `GetAllAgentsWithRelationTowardsActor()`

## Floating-bar visibility
Every avatar shows its floating combatant bar, including the local human player's own (so the host sees a bar over their own head too). `SetCombattantWidgetVisible` exists only to hide the boss's floating bar while the dedicated on-screen boss bar is displayed.
