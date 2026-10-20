# Input

`GeoInputComponent.h` — Enhanced Input bridge to GAS.

- Binds `MoveAction` and `LookAction`; `bIsUsingController` by `ControllerDriftThreshold` (0.1).
- `BindAbilityActions(Object, Pressed, Released, Held, AbilityInfo)`: binds every `UAbilityInfo` input action to the three ASC callbacks.
- `GetLookVector` / `UpdateMouseLook()`: aim data each tick.
- Test aim: `bTestAimActive` + `TestAimWorld` (world X / Y) replace the cursor. `UpdateMouseLook` then aims at that point (mouse-style: look = point − pawn, `bIsUsingController` false) and ignores the real cursor, so a driven session (`AI/MCP/MCP_EpicTesting.md`) says where "the mouse" is while every ability input stays the real one. Plain properties (`WITH_EDITORONLY_DATA`, `EditInstanceOnly`, `Transient`): set through reflection on the live component in the editor, never saved, absent from game builds; no tool, no function.
- `APlayableCharacter` forwards to `AbilityInputTagPressed/Released/Held(FGameplayTag)`.
- `UGeoInputConfig` maps `UInputAction` → `FGameplayTag`.
