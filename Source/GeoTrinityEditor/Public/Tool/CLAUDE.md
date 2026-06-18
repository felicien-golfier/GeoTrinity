# GeoTrinityEditor / Tool

Editor-only Python/Blueprint automation utilities. These are `UEditorUtilityObject` subclasses that mutate StateTree
and Widget assets. They live in the **`GeoTrinityEditor` module** (Type `Editor` in `.uproject`, built only when
`bBuildEditor`) — NOT in the runtime `GeoTrinity` module — because `UEditorUtilityObject` depends on the editor-only
`Blutility` module, which is absent in packaged Game/Shipping builds. Keeping them here is what lets the project
package; do not move them back under a `#if WITH_EDITOR` in `GeoTrinity` (UHT emits the class registration regardless of
that guard, so the packaged runtime module fails to find `UEditorUtilityObject`).

The module depends on `GeoTrinity` (public headers only), so these utils can include runtime types like
`AI/StateTree/Ability/STTask_FireAbility.h` and `HUD/Menu/GeoMenuButton.h`.

## `GeoStateTreeBuilderUtil.h`
`UEditorUtilityObject` for mutating `UStateTree` assets from Python/Blueprint automation.
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
`UEditorUtilityObject` — generic, reusable widget-tree primitives. Keep this file free of per-asset functions; content-specific builders belong in `GeoHudWidgetBuilderUtil.h`.
- `SetRootPanel(Blueprint, PanelClass, RootName)` — replaces root with a freshly constructed panel (CanvasPanel, Overlay, HorizontalBox, …), named for BindWidget, compiles and saves
- `SetImageRoot` — replaces root with a single Image (texture + desired size), compiles and saves
- `SetImageRootFromMaterial` — like `SetImageRoot` but draws a material (e.g. a luminance-to-alpha mask) instead of a raw texture
- `InspectWidgetBlueprint` — logs the full widget tree (type, name, slot layout, widget properties) to `LogTemp`

Building-block helpers for content builders (public but not `UFUNCTION`): `BeginBuild` (validate/mark/clear root), `FinishBuild` (compile/save), `ConstructRootPanel`, `AddChildToCanvasPanel`, `AddCenteredChildToVerticalBox(VerticalBox, Child, Padding)` (center-aligned slot with padding), `ConstructLabeledButton(Tree, Name, LabelText)` (Button + TextBlock label), `ConstructProgressBar(Tree, Name, FillColor, BackgroundColor, bIsVariable)` (styled empty ProgressBar, LeftToRight default; set `bIsVariable` when C++ BindWidgets it), `AddFillChildToOverlay(Overlay, Child)` (fill/fill overlay slot covering the full rect).

## `GeoHudWidgetBuilderUtil.h`
`UEditorUtilityObject` — content-specific HUD widget-tree builders that compose the generic primitives from `GeoWidgetBuilderUtil.h`. New per-widget builders go here.
- `BuildAbilitySlotWidget(Blueprint, SquareSize, KeyLabelPlacement)` — builds WBP_AbilitySlot tree: SizeBox ("Square") → Icon / CooldownSweep / CountdownText / CountText, plus a `KeyText` live key-binding label placed per `EAbilitySlotKeyLabelPlacement` (`Below` = VerticalBox root, label under the square (default); `OverlayBottom` = label bottom-center over the icon; `None` = no label). Names match BindWidget members on `UGeoAbilitySlotWidget`
- `BuildAbilityBarWidget(Blueprint)` — builds WBP_AbilityBar tree: Overlay root with centered SlotBox HorizontalBox (BindWidget on `UGeoAbilityBarWidget`)
- `BuildChargeBeamGaugeWidget(Blueprint, SweetSpotMinRatio, SweetSpotMaxRatio)` — builds WBP_ChargeBeamGauge tree (ChargeBar + SweetSpotBar overlay on CanvasPanel)
- `BuildCombattantLifeBarWidget(Blueprint, BarWidth, BarHeight)` — builds WBP_CombattantLifeBar tree: SizeBox root (BarWidth × BarHeight) → Overlay → `HealthBar` (fill) under `ShieldBar` (fill, semi-transparent cyan). Both bars fill the same rect so shield overlays health, matching `WBP_MainOverlay`. Names match `BindWidgetOptional` members on `UGenericCombattantWidget`; shield percent driven by `UpdateShieldRatio` (Shield / MaxHealth)
- `AddAbilityBarToOverlay(Blueprint, ParentPanelName, AbilityBarClass, fractions)` — adds AbilityBarClass child named "AbilityBar" to the overlay's CanvasPanel, anchored bottom-center, sized as fractions of the canvas
- `BuildLocalConnectWidget(Blueprint, MenuButtonClass)` — builds WBP_LocalConnect ("Play Local" direct-IP panel): Overlay root → centered VerticalBox → HostButton / IPInput / JoinButton / LocalIPText / BackButton; buttons are `MenuButtonClass` (UGeoMenuButton WBP) instances with per-instance labels; names match BindWidgets on `UGeoLocalConnectWidget`
- `AddLocalConnectToMainMenu(Blueprint, ParentPanelName, ButtonsBoxName, MenuButtonClass, LocalConnectClass)` — appends to the existing WBP_MainMenuWidget without rebuilding: PlayLocalButton + spacer inserted above QuitButton in `ButtonsBoxName`, LocalConnectClass added centered on the canvas as "LocalConnectWidget"; re-run-safe
