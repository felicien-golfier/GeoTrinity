# Tool

General-purpose utilities and helpers.

## `UGeoGameplayLibrary.h` (alias: `GeoLib`)
Blueprint-callable static helpers:

**Server check** — always use this:
- `IsServer(UObject*)` / `IsServer(UWorld*)` — true on dedicated server and listen server. Never use `HasAuthority()` or `IsNetMode(NM_DedicatedServer)`.

**Time**:
- `GetServerTime(WorldContext, bUpdatedWithPing)` — network-approximated server time. **Never use for local client timing** (charge duration, UI) — use `GetWorld()->GetTimeSeconds()` instead.

**Camera shake**:
- `TriggerCameraShake(WorldContext, ShakeClass, Scale)` — local player only

**Debug**:
- `GetColorForObject(Object)` — deterministic debug color per object

**Constant**: `ArbitraryCharacterZ = 50.0f` — character spawn Z offset

## `GeoAssetManager.h`
Custom asset manager subclass. Configure in `DefaultEngine.ini`.

## `Team.h`
Team definitions and `ETeam` enum used by `IGenericTeamAgentInterface`.

## `GeoStateTreeBuilderUtil.h`
Editor-only (`#if WITH_EDITOR`) `UEditorUtilityObject` for mutating `UStateTree` assets from Python/Blueprint automation.
Each method validates, compiles (`FCompilerManager::CompileSynchronously`), and saves the asset atomically.
- `AddFireAbilityStateByTagName` — creates a state with an `FSTTask_FireProjectileAbility` task at a given parent/index
- `ReplaceFireAbilityTagInState` — swaps the ability tag on an existing state's fire task
- `RemoveState` — deletes a state by name (recursive search)
- `ClearTransitions` / `AddTransition` — manage `GotoState` transitions with a specified trigger
- `ListStates` — logs the full tree with indent and task tags to `LogTemp`
