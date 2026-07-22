# Input

## `GeoInputComponent.h`
Enhanced Input component bridging raw input to GAS ability activation.
- Binds movement (`MoveAction`) and look/aim (`LookAction`); detects controller vs mouse (`bIsUsingController`, `ControllerDriftThreshold=0.1f`).
- `BindAbilityActions(Object, PressedFunc, ReleasedFunc, HeldFunc, AbilityInfo)` — template; binds all ability input actions from `UAbilityInfo` to the three GAS callbacks.
- `GetLookVector(OutLook)` / `UpdateMouseLook()` — aim data, updated each tick.

On `APlayableCharacter`, inputs forward to ASC via `AbilityInputTagPressed/Released/Held(FGameplayTag)` (`Held` fires every frame).

`UGeoInputConfig` data asset (not in this folder, referenced by AbilityInfo) maps `UInputAction` → `FGameplayTag`.
