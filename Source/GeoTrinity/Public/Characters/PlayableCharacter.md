# PlayableCharacter.h — human-controlled character

Extends `AGeoCharacter`. ASC lives on `AGeoPlayerState`.

## Class switching
- `ChangeClass(EPlayerClass)` — swaps mesh, animation, and ability sets at runtime
- `ApplyClassData(EPlayerClass)` — applies mesh and animation only (no ability swap)
- `ClassData` (`TMap<EPlayerClass, FPlayerClassData>`) — configured in BP; one entry per class

`FPlayerClassData`: `USkeletalMesh* Mesh`, `TSubclassOf<UAnimInstance> AnimClass`, `TSubclassOf<UGameplayEffect> DefaultAttributes`

## Input forwarding
Each frame, `SetupPlayerInputComponent` binds Enhanced Input → `GeoInputComponent::BindAbilityActions()`.
Per-frame: `AbilityInputTagHeld(Tag)` called for every held action.

## Deploy charge gauge
- `ShowDeployChargeGauge(UGeoGameplayAbility*)` — makes `DeployChargeGaugeComponent` visible, binds it to ability's `GetChargeRatio()`
- `HideDeployChargeGauge()` — hides widget
- `DeployChargeGaugeComponent` is a `UWidgetComponent` with `Space = Screen`

## Rotation
- `MaxRotationSpeed = 720 deg/s`
- `UpdateAimRotation(DeltaSeconds)` — rotates character toward `GeoInputComponent::GetLookVector()`; scaled by `RotationSpeedMultiplier` attribute

## GAS init flow
`PossessedBy` (server) / `OnRep_PlayerState` (client) → `InitGAS()` → fetches ASC from `AGeoPlayerState`, calls `InitAbilityActorInfo(PlayerState, Self)`.
