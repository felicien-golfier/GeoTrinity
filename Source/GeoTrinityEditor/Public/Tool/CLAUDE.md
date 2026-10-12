# GeoTrinityEditor/Tool

`UEditorUtilityObject` builders exposing editor operations Python cannot reach. Python composes everything else on top. How-to per domain: `AI/MCP/`.

| File | Role |
|---|---|
| `GeoStateTreeBuilderUtil.h` | `UStateTree` edits; each op validates, compiles, saves atomically |
| `GeoWidgetBuilderUtil.h` | Generic widget-tree primitives (no per-asset functions) |
| `GeoHudWidgetBuilderUtil.h` | Content-specific HUD tree builders composed from the generic ones |
| `GeoAnimBuilderUtil.h` | Montage section layout / links (C++-only); rig / mesh rebuild; per-channel bone keys |
| `GeoMaterialBuilderUtil.h` | Material-instance layer stack (static parameters) |
| `GeoNiagaraBuilderUtil.h` | Niagara emitter stack ops; `FunctionName` addresses a module or dynamic input by its rapid-iteration name |

## Anim builder — bone channel keys
- A bone track = 9 float channels (translation xyz, rotation Euler xyz in degrees, scale xyz) in the sequence's FK control-rig section; the controller's `SetBoneTrackKeys` can only write whole-frame arrays, so sparse keys go through `SetBoneChannelKeys` (frames + values per channel); `GetBoneChannelKeys` reads them back and `GetBoneTrackNames` lists the bones holding a track.
- Linear keys on whole frames; a channel with no keys is a constant (its default value); an empty key list with a value clears the channel to that constant.
- Reaches the data model as its `UMovieSceneSequence` base (public `MovieScene` / `ControlRig` headers), never the `AnimationData` plugin's private ones.
- Caller finalizes the sequence afterwards and keeps channels of equal key count on the same frames (`AI/MCP/MCP_Animation.md`).

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
