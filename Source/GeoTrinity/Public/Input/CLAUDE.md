# Input

`GeoInputComponent.h` — Enhanced Input bridge to GAS.

- Binds `MoveAction` and `LookAction`; `bIsUsingController` by `ControllerDriftThreshold` (0.1).
- `BindAbilityActions(Object, Pressed, Released, Held, AbilityInfo)`: binds every `UAbilityInfo` input action to the three ASC callbacks.
- `GetLookVector` / `UpdateMouseLook()`: aim data each tick.
- `APlayableCharacter` forwards to `AbilityInputTagPressed/Released/Held(FGameplayTag)`.
- `UGeoInputConfig` maps `UInputAction` → `FGameplayTag`.
