# GeoTrinityEditor / Tool

Editor-only Python/Blueprint automation utilities (`UEditorUtilityObject` subclasses mutating StateTree and Widget assets). Live in the **`GeoTrinityEditor`** module (Type `Editor`), not runtime `GeoTrinity`, because `UEditorUtilityObject` depends on the editor-only `Blutility` module — packaged Game/Shipping builds need this split. Do **not** move them back under `#if WITH_EDITOR` in `GeoTrinity` (UHT still emits class registration regardless, breaking the packaged runtime).

Module depends on `GeoTrinity` (public headers only), so these utils can include runtime types (`STTask_FireAbility.h`, `GeoMenuButton.h`).

## `GeoStateTreeBuilderUtil.h`
Mutates `UStateTree` assets from Python/Blueprint. Each method validates, compiles, saves atomically.
- `AddState` (taskless, for idle/dormant states waiting on `OnEvent`), `AddFireAbilityStateByTagName`, `ReplaceFireAbilityTagInState`, `RemoveState`
- `ClearTransitions`/`AddTransition` (trigger + event tag for `OnEvent`)
- `AddFloatEnterCondition`, `ClearEnterConditions`
- `BindConditionPropertyToPropertyFunction` — binds a condition property to a Property Function output, wires its input to a context object
- `AddTaskToState` (any struct type by name, auto-binds context props at compile), `AddSendEventAfterNCyclesTask`
- `SetRequiredEventToEnter`/`ClearRequiredEventToEnter`
- `ListStates`/`ListEnterConditions` — log to `LogTemp`

## `GeoWidgetBuilderUtil.h`
Generic, reusable widget-tree primitives — keep free of per-asset functions (those belong in `GeoHudWidgetBuilderUtil.h`).
- `SetRootPanel`/`SetImageRoot`/`SetImageRootFromMaterial` — replace root, compile+save
- `InspectWidgetBlueprint` — logs full tree to `LogTemp`

### Low-level tree primitives
`WidgetTree` is **protected** — Python can call `set_*` on existing widgets/slots but can't construct/reparent/remove/save itself. These expose exactly the missing ops; compose everything else from Python on the returned objects. **Batch Construct/Attach/Remove + Python slot edits, then call `CommitTree` once.**
- `ConstructWidgetInTree` → unparented widget; GUID deferred to `CommitTree` (aborted batch never leaks one). Reuse-safe. No save.
- `AttachWidget` → `UPanelSlot*`; (re)parents at `Index` (-1 appends), resolves names even for unparented widgets. No save.
- `RemoveWidget` — detach/delete. No save.
- `CommitTree` — reconciles variable→GUID map (mints/prunes), compiles+saves. Self-healing regardless of batch outcome — call once per batch.
- `FindWidget` — read-only lookup, no mutation.

### Convenience wrappers
- `AddWidgetToPanel` — Construct+Attach+Commit in one call; applies pixel `Offsets` on a CanvasPanel parent.
- `GroupWidgetsIntoPanel` — builds a group panel under a parent, re-attaches existing children into it in z-order, commits; children keep names/GUIDs/bindings.

Non-`UFUNCTION` building blocks: `BeginBuild`/`FinishBuild`, `ConstructRootPanel`, `AddChildToCanvasPanel`, `AddCenteredChildToVerticalBox`, `ConstructLabeledButton`, `ConstructProgressBar`, `AddFillChildToOverlay`.

## `GeoHudWidgetBuilderUtil.h`
Content-specific HUD widget-tree builders composing the generic primitives. New per-widget builders go here.
- `BuildAbilitySlotWidget` — WBP_AbilitySlot tree (icon/cooldown/countdown/count badge + `KeyText` per `EAbilitySlotKeyLabelPlacement`)
- `BuildAbilityBarWidget` — WBP_AbilityBar tree
- `BuildChargeBeamGaugeWidget` — ChargeBar + SweetSpotBar overlay
- `BuildCombattantLifeBarWidget` — HealthBar under ShieldBar (shield overlays health), matches `WBP_MainOverlay`
- `AddAbilityBarToOverlay` — adds bar to overlay canvas, bottom-center
- `BuildLocalConnectWidget` — WBP_LocalConnect direct-IP panel
- `AddLocalConnectToMainMenu` — appends Play-Local entry to WBP_MainMenuWidget without rebuilding; re-run-safe
