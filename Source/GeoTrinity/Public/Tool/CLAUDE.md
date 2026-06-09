# Tool

General-purpose utilities and helpers.

## `UGeoGameplayLibrary.h` (alias: `GeoLib`)
Blueprint-callable static helpers:

**Server check** — always use this:
- `IsServer(UObject*)` / `IsServer(UWorld*)` — true on dedicated server and listen server. Never use `HasAuthority()` or `IsNetMode(NM_DedicatedServer)`.

**Render/cosmetic gate** — use for visuals, NOT `!IsServer()`:
- `IsDedicatedServer(UObject*)` / `IsDedicatedServer(UWorld*)` — true only on a dedicated server (no viewport). Gate cosmetic-only work (montages, local Gameplay Cues, VFX) with `if (!IsDedicatedServer(...))`. `!IsServer()` wrongly skips the listen-server **host**, which is a rendering player — that bug makes boss montages/cues invisible to the host only.

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
- `AddState` — creates an empty (taskless) state at a given parent/index; use for idle/dormant states that wait on an `OnEvent` transition
- `AddFireAbilityStateByTagName` — creates a state with an `FSTTask_FireAbility` task at a given parent/index
- `ReplaceFireAbilityTagInState` — swaps the ability tag on an existing state's fire task
- `RemoveState` — deletes a state by name (recursive search)
- `ClearTransitions` / `AddTransition` — manage `GotoState` transitions; `AddTransition` takes a trigger and, for `OnEvent`, an event tag name
- `AddFloatEnterCondition` — appends a `FStateTreeCompareFloatCondition` to a state's `EnterConditions`
- `BindConditionPropertyToPropertyFunction` — binds a condition property to a Property Function output (e.g. `FSTGetHealthRatioPropertyFunction`) and wires the function's input to a context object
- `AddSendEventAfterNCyclesTask` — appends an `FSTTask_SendEventAfterNCycles` task to an existing state
- `ClearEnterConditions` — removes all enter conditions from a state
- `SetRequiredEventToEnter` / `ClearRequiredEventToEnter` — set or clear the Required Event To Enter on a state
- `ListStates` — logs the full tree with indent and task tags to `LogTemp`
- `ListEnterConditions` — logs all enter conditions on a named state

## `GeoWidgetBuilderUtil.h`
Editor-only (`#if WITH_EDITOR`) `UEditorUtilityObject` — generic, reusable widget-tree primitives. Keep this file free of per-asset functions; content-specific builders belong in `GeoHudWidgetBuilderUtil.h`.
- `SetRootPanel(Blueprint, PanelClass, RootName)` — replaces root with a freshly constructed panel (CanvasPanel, Overlay, HorizontalBox, …), named for BindWidget, compiles and saves
- `SetImageRoot` — replaces root with a single Image (texture + desired size), compiles and saves
- `SetImageRootFromMaterial` — like `SetImageRoot` but draws a material (e.g. a luminance-to-alpha mask) instead of a raw texture
- `InspectWidgetBlueprint` — logs the full widget tree (type, name, slot layout, widget properties) to `LogTemp`

Building-block helpers for content builders (public but not `UFUNCTION`): `BeginBuild` (validate/mark/clear root), `FinishBuild` (compile/save), `ConstructRootPanel`, `AddChildToCanvasPanel`, `AddCenteredChildToVerticalBox(VerticalBox, Child, Padding)` (center-aligned slot with padding), `ConstructLabeledButton(Tree, Name, LabelText)` (Button + TextBlock label).

## `GeoHudWidgetBuilderUtil.h`
Editor-only (`#if WITH_EDITOR`) `UEditorUtilityObject` — content-specific HUD widget-tree builders that compose the generic primitives from `GeoWidgetBuilderUtil.h`. New per-widget builders go here.
- `BuildAbilitySlotWidget(Blueprint, SquareSize, KeyLabelPlacement)` — builds WBP_AbilitySlot tree: SizeBox ("Square") → Icon / CooldownSweep / CountdownText / CountText, plus a `KeyText` live key-binding label placed per `EAbilitySlotKeyLabelPlacement` (`Below` = VerticalBox root, label under the square (default); `OverlayBottom` = label bottom-center over the icon; `None` = no label). Names match BindWidget members on `UGeoAbilitySlotWidget`
- `BuildAbilityBarWidget(Blueprint)` — builds WBP_AbilityBar tree: Overlay root with centered SlotBox HorizontalBox (BindWidget on `UGeoAbilityBarWidget`)
- `BuildChargeBeamGaugeWidget(Blueprint, SweetSpotMinRatio, SweetSpotMaxRatio)` — builds WBP_ChargeBeamGauge tree (ChargeBar + SweetSpotBar overlay on CanvasPanel)
- `AddAbilityBarToOverlay(Blueprint, ParentPanelName, AbilityBarClass, fractions)` — adds AbilityBarClass child named "AbilityBar" to the overlay's CanvasPanel, anchored bottom-center, sized as fractions of the canvas
- `BuildMainMenuWidget(Blueprint)` — builds WBP_MainMenu connect-screen: Overlay root → centered VerticalBox → TitleText / HostButton / IPInput / JoinButton / LocalIPText; BindWidget names match `UGeoMenuWidget` BP members
