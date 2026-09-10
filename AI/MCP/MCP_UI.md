# MCP UI — Widget Blueprint Automation

Creating and configuring UMG Widget Blueprints through `execute_script` and the widget shim.

## Always reuse the existing menu system

Never author a menu widget, panel or button from the engine base classes. Every menu-facing widget derives from
the project's menu panel base and every clickable element from its button class — see
`Source/GeoTrinityUI/Public/HUD/Menu/` and the table in `Source/GeoTrinityUI/Public/HUD/CLAUDE.md`.

Those bases are what make a menu gamepad-usable: they route the gamepad back button and BackSpace to the
innermost panel's back action, hand focus to a declared initial widget on the first navigation input, and map
focus onto hover so gamepad and mouse selection render and sound identically. A widget on the engine bases
inherits none of it, so it is unreachable by controller and cannot be backed out of even when it looks correct
with a mouse.

Deriving from the panel base obliges the widget to declare its initial focus target and, if dismissable, its
back action. A nested sub-panel needs both as much as a top-level menu does — a panel that only forwards its
close through a delegate still has to be its own panel to receive the back input. New buttons match the existing
ones by reusing the styled button class and configuring its exposed appearance properties, never by restyling a
raw engine button.

## Creating a widget Blueprint

Use `WidgetBlueprintFactory` with `parent_class` set, then create through `AssetTools`; it has no
`use_inherited_viewport_size` property. See `AI/Python/UI/charge_beam_gauge.py`.

A widget Blueprint's parent-class property is unreadable from Python — read it from the asset-registry tag,
which exposes both the immediate and the native parent class, to confirm a designer Blueprint already reparents
to the intended C++ base. The `widget_class` on a WidgetComponent reveals the asset path actually in use; verify
it before rebuilding, since several assets may share a name in different folders.

## Widget tree

Python cannot read or write the designer tree — `WidgetBlueprint` exposes no tree property — so construct,
re-parent, remove and commit go through the shim primitives in
`Source/GeoTrinityEditor/Public/Tool/GeoWidgetBuilderUtil.h`. Everything else — every `set_*` on a widget or its
slot — is reachable from Python on the objects those primitives return.

Compose the primitives from Python to add, move, wrap, reorder or delete any widget: batch tree ops plus Python
slot edits, then commit once. The primitives resolve widgets by name even before they are parented, so a
just-constructed group can be attached and populated in the same batch. Do not add a shim `UFUNCTION` per
request — reach for one only when an operation cannot be expressed through the primitives plus Python setters,
and then prefer enhancing an existing generic primitive. Reusable compositions go in `AI/Python/` as `.py`.

**Call order**: construct (which stages the widget and reserves its GUID), attach, set slot and widget
properties from Python, commit once. Calling construct a second time after attach — to re-obtain a reference —
triggers the GUID ensure at commit; use the object the first construct returned, or the find-by-name lookup.
Find-by-name is also how to fetch an existing widget to resize or realign it: construct is reuse-safe for
*rebuilding*, meaning it discards the old widget object, so reusing it just for a reference silently drops that
widget's children.

Widget names passed to construct become the `BindWidget` variable names the C++ class binds against. A
shim-constructed widget is reachable as a Blueprint variable — graph nodes, designer Details, `BindWidget` —
only when flagged as one; leave layout-only widgets unflagged. Parameterize the constructed root panel's name so
a built root can satisfy a specific `BindWidget` variable.

**Single-child containers evict on re-attach.** A `SizeBox` or `Border` holds exactly one content child, and
attaching into one that already has a child detaches the previous one instead of nesting; anything unreachable
from the root at commit is dropped from the saved asset with its whole subtree. To add a layer inside one,
insert a multi-child panel as its single child first, then attach both into that.

### Variable GUIDs

The commit primitive reconciles the variable-name-to-GUID map against the tree before compiling, so a batch is
consistent however it ended, and an aborted batch leaves no dangling GUID. Every tree widget needs a GUID, not
only the ones flagged as variables — the root panel and any named widget count. The reconciliation set is the
root plus all descendants: mint a GUID for any tree widget missing one, prune entries whose widget is gone, and
leave animation GUIDs alone. The widget-BP compiler runs the same validation and is self-healing, so a mismatch
surfaces as a logged ensure during compile rather than a corrupted asset — under a debugger those ensures trap
as breakpoints and freeze the bridge until execution resumes.

The compiler validates against every widget **outer-owned by the WidgetTree**, parented or not, which is a
superset of the root walk the reconciliation uses. Always rename a detached widget out to the transient package
so both walks agree: detaching alone leaves it in the compiler's set with no GUID entry, and after the next
save/reload leaves a GUID entry with no widget. The remove and construct primitives do this; any new code path
that drops a widget must too.

**Rebuilding an existing asset**: clear the GUID map as part of resetting the root — the compiler auto-assigns
GUIDs only when that map is empty, so stale entries leave freshly constructed widgets without one. When a
rebuild changes the root panel's class, rename the previous root out to the transient package first, since UE
refuses to replace an object with one of a different class under the same name.

**Appending one child to an existing tree**: do not clear the root or the map — existing widgets keep their
GUIDs. Register a fresh GUID for the new child's name yourself, for every appended widget including layout-only
ones like spacers, and remove any existing widget of that name first to stay reuse-safe. To insert rather than
append, use the panel's insert-at-index call with an existing sibling's index.

See `GeoWidgetBuilderUtil.cpp` (`CommitTree`, `RemoveWidget`, `ConstructWidgetInTree`, `BeginBuild`,
`ConstructRootPanel`, `AddChildToCanvasPanel`) and `GeoHudWidgetBuilderUtil.cpp` (`AddPanelEntryToMainMenu`).

### Shim includes

`"WidgetBlueprint.h"` as a bare filename (UMGEditor public root), `"Blueprint/WidgetTree.h"` with its prefix,
and `"Kismet2/KismetEditorUtilities.h"` from UnrealEd. Keep generic primitives in `UGeoWidgetBuilderUtil` and
content-specific trees in `UGeoHudWidgetBuilderUtil`, which composes them — see `MCP_EditorUtility.md`.

## Setting values

A widget Blueprint's class-default values (a material reference, a slot widget class) are set on the default
object of its generated class from Python, then saved — use this for asset references that would otherwise be
set in the designer.

A user-widget template constructed inside another widget's tree holds its own property values, so setting a
default on the child class CDO afterwards does not reach it. Set the property on the template object inside the
parent asset and save the parent; re-running a builder that reconstructs the template needs those values
re-applied. See `AI/Python/UI/local_connect_menu.py` (`set_child_template_property`).

To replicate one widget's appearance onto another, read the source widget's brush and colour properties through
the find-by-name lookup and copy each value across rather than guessing: tint, resource texture and draw type
have to move together, since a tint alone reads differently against a different texture or draw type.

`InspectWidgetBlueprint` on the shim CDO logs the full tree — types, names, slot layout, per-widget properties —
which is how to verify fill type, offsets and colours before debugging in PIE.

## Layout notes

- **Fixed-position canvas slot**: pin a child to an absolute pixel offset with a top-left-only anchor and pixel
  offsets. Canvas Y=0 is the **top**, so a bottom-anchored overlay computes `TopOffset = (1 - maxRatio) * Height`.
- **ProgressBar**: `WidgetStyle` direct access is deprecated since 5.1 — use the getter/setter pair. Set
  `SetBarFillType` explicitly (the default is `LeftToRight`; a vertical bar wants `BottomToTop`).
- **WidgetComponent on an actor Blueprint**: `draw_size` is an `IntPoint`, not a `Vector2D`, and its orientation
  must match the bar (`height > width` for a vertical one). Configure through `SubobjectDataSubsystem` handle
  enumeration — print all indices first to identify the target. See `AI/Python/UI/charge_beam_gauge.py`.
- **Constraining a label**: cap the width with a SizeBox and wrap the text in a ScaleBox set to scale-to-fit-X,
  down-only, so short text keeps its size and only over-wide text shrinks. See `GeoHudWidgetBuilderUtil.cpp`
  (`BuildAbilitySlotWidget`).
- **Single-image widget**: a widget showing one image needs only an Image root — no canvas, no slotting, since
  the hotspot is the widget's top-left. To draw only the bright parts of a texture, feed the Image a UI-domain
  translucent material mapping luminance to opacity rather than a raw texture. See `AI/Python/UI/crosshair_cursor.py`.
- **Grouping existing children under one container**: re-parent each child into a new panel rather than
  re-creating it, so it keeps its name, GUID and bindings. Set the panel's pixel rect to the cluster's bounding
  box and rebase each child's offsets by subtracting that origin — a child canvas inside a canvas keeps pixel
  offsets exact. Re-parent in the intended draw order, since child order is z-order. See
  `AI/Python/UI/group_widgets.py`.
- An `OverlaySlot`'s alignment properties in Python are `horizontal_alignment` and `vertical_alignment`.

## Driving a child widget from C++

To rebuild or update a nested widget without authoring event-graph nodes, give the parent a C++ base that
`BindWidget`s the child and exposes a build/update method the owning HUD or actor calls directly. Reparent the
designer Blueprint to that base and name the child to match the `BindWidget` variable. See
`Source/GeoTrinityUI/Public/HUD/GeoOverlayWidget.h`.
