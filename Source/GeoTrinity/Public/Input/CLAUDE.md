# Input

## `GeoInputComponent.h`
Enhanced Input component bridging raw input to GAS ability activation.

**Key responsibilities:**
- Binds movement (`MoveAction`) and look/aim (`LookAction`) inputs
- Detects controller vs mouse (`bIsUsingController`, threshold `ControllerDriftThreshold = 0.1f`)
- `BindAbilityActions(Object, PressedFunc, ReleasedFunc, HeldFunc, AbilityInfo)` — template; binds all ability input actions from `UAbilityInfo` data asset to the three GAS callbacks

**Aim data:**
- `GetLookVector(OutLook)` — returns latest look vector
- `UpdateMouseLook()` — updates aim rotation from mouse/stick each tick

**On `APlayableCharacter`**, inputs are forwarded to ASC via:
- `AbilityInputTagPressed(FGameplayTag)`
- `AbilityInputTagReleased(FGameplayTag)`
- `AbilityInputTagHeld(FGameplayTag)` — called every frame while held

`UGeoInputConfig` data asset (not in this folder — referenced by AbilityInfo) maps `UInputAction` → `FGameplayTag`.
