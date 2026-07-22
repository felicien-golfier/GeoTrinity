# PlayableCharacter.h — human-controlled character

Extends `AGeoCharacter`. ASC lives on `AGeoPlayerState`.

## Class switching
- `ChangeClass(EPlayerClass)` — swaps mesh, animation, and ability sets at runtime; ends with `GiveLife()`
- `ApplyClassData(EPlayerClass)` — applies mesh, animation, and material **only** (pure visuals, no attributes, no abilities). Safe to run on every client for every pawn (driven by `AGeoPlayerState::ApplyClassDataToPawn`).
- `ClassData` (`TMap<EPlayerClass, FPlayerClassData>`) — configured in BP; one entry per class

`FPlayerClassData`: `USkeletalMesh* Mesh`, `TSubclassOf<UAnimInstance> AnimClass`, `TSubclassOf<UGameplayEffect> DefaultAttributes`

## GiveLife — "becomes alive" hook (server-only)
`GiveLife()` is the single place that brings the character to the alive state: it applies the current class's `DefaultAttributes` (resets HP/attributes to full) and starts passives via `ASC->ReactivatePassiveAbilities()`. Called at the end of `ChangeClass` (initial spawn + class switch) and `ReviveLogic` (revive). Server-guarded — attributes and passive activation are authoritative concerns.

Passives are **stopped** automatically when leaving a class: `ChangeClass` → `ClearPlayerClassAbilities` → `ClearAbility` runs each passive's `EndAbility` (tears down its component + delegate). No explicit "take life" call needed.

## Input forwarding
Each frame, `SetupPlayerInputComponent` binds Enhanced Input → `GeoInputComponent::BindAbilityActions()`.
Per-frame: `AbilityInputTagHeld(Tag)` called for every held action.

## Deploy charge gauge
- `ShowDeployChargeGauge(UGeoGameplayAbility*)` — makes `DeployChargeGaugeComponent` visible, binds it to ability's `GetChargeRatio()`
- `HideDeployChargeGauge()` — hides widget
- `DeployChargeGaugeComponent` is a `UWidgetComponent` with `Space = Screen`

## Rotation
- `UpdateAimRotation(DeltaSeconds)` — computes the desired yaw from `GeoInputComponent::GetLookVector()` and calls `AGeoCharacter::SetTargetYaw()`; the actual turn-rate clamp (`MaxRotationSpeed`, 720 deg/s default) lives in the base class's `Tick` — see `AGeoCharacter.h`

## GAS init flow
`PossessedBy` (server) / `OnRep_PlayerState` (client) → `InitGAS()` → fetches ASC from `AGeoPlayerState`, calls `InitAbilityActorInfo(PlayerState, Self)`.
