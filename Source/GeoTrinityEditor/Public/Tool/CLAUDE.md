# GeoTrinityEditor/Tool

`UEditorUtilityObject` builders exposing editor operations Python cannot reach. Python composes everything else on top. How-to per domain: `AI/MCP/`.

| File | Role |
|---|---|
| `GeoStateTreeBuilderUtil.h` | `UStateTree` edits; each op validates, compiles, saves atomically |
| `GeoWidgetBuilderUtil.h` | Generic widget-tree primitives (no per-asset functions) |
| `GeoHudWidgetBuilderUtil.h` | Content-specific HUD tree builders composed from the generic ones |
| `GeoAnimBuilderUtil.h` | Montage section layout / links (C++-only) |
| `GeoMaterialBuilderUtil.h` | Material-instance layer stack (static parameters) |
| `GeoNiagaraBuilderUtil.h` | Niagara emitter stack ops; `FunctionName` addresses a module or dynamic input by its rapid-iteration name |

## StateTree builder
- `AddState` (taskless), `AddFireAbilityStateByTagName`, `ReplaceFireAbilityTagInState`, `RemoveState`.
- `ClearTransitions` / `AddTransition` (trigger + event tag), `AddFloatEnterCondition`, `ClearEnterConditions`.
- `BindConditionPropertyToPropertyFunction`, `AddTaskToState` (any struct by name, auto-binds context), `AddSendEventAfterNCyclesTask`.
- `SetRequiredEventToEnter` / `ClearRequiredEventToEnter`, `ListStates` / `ListEnterConditions`.

## Widget builder
- `WidgetTree` is protected: batch `ConstructWidgetInTree` / `AttachWidget` / `RemoveWidget` + Python slot edits, then `CommitTree` once (mints/prunes GUID map, compiles, saves; self-healing).
- Wrappers: `AddWidgetToPanel` (construct + attach + commit), `GroupWidgetsIntoPanel` (keeps names/GUIDs/bindings).
- `SetRootPanel` / `SetImageRoot` / `SetImageRootFromMaterial`, `InspectWidgetBlueprint`, `FindWidget` (read-only).
- Non-`UFUNCTION` blocks: `BeginBuild` / `FinishBuild`, `ConstructRootPanel`, `AddChildToCanvasPanel`, `ConstructLabeledButton`, `ConstructProgressBar`, …

## Hud builder
- `BuildAbilitySlotWidget`, `BuildAbilityBarWidget`, `BuildChargeBeamGaugeWidget`, `BuildCombattantLifeBarWidget`, `AddAbilityBarToOverlay`, `BuildLocalConnectWidget`, `AddLocalConnectToMainMenu` (re-run-safe).
- New per-widget builders go here.
