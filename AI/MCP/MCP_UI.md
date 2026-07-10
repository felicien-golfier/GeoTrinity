# MCP UI — Widget Blueprint Automation

Creating and configuring UMG Widget Blueprints via `execute_script` and C++ shims.

---

## Creating a Widget Blueprint

Use `WidgetBlueprintFactory` with `parent_class` set, then create via `AssetTools`. See `AI/Python/charge_beam_gauge.py` for a full example.

`WidgetBlueprintFactory` has no `use_inherited_viewport_size` property — do not set it.

---

## Widget Tree

Python cannot read or write the widget designer tree — `WidgetBlueprint` exposes no `widget_tree` property to Python, so it cannot construct, re-parent, remove, or save tree widgets.

A C++ `UEditorUtilityObject` shim exposes the tree operations Python lacks (construct, attach/re-parent, remove, commit, find-by-name); see the low-level primitives in `Source/GeoTrinityEditor/Public/Tool/GeoWidgetBuilderUtil.h`. Everything else — every `set_*` on an existing widget or its slot (text, brush, padding, anchors, color, …) — is already reachable from Python on the objects the primitives return.

To fetch a reference to an already-existing widget (e.g. to resize/realign it without touching its content), use `FindWidget(Blueprint, Name)` rather than re-running `ConstructWidgetInTree` on its name — the latter is reuse-safe for *rebuilding* (it discards the old widget object), so reusing it on a widget that already has children only to get a reference back would silently drop those children.

Compose these primitives from Python to add, move, wrap, reorder, or delete any widget: batch the tree ops plus Python slot edits, then commit once. The typical flow is construct a panel, attach it, attach existing children into it, position each child's slot, commit. The primitives resolve widgets by name even before they are parented, so a just-constructed group can be attached and populated in the same batch. Do not add a new shim `UFUNCTION` per request — reach for a new shim function only when an operation genuinely cannot be expressed through the primitives plus Python setters, and when you do, prefer enhancing an existing generic primitive over adding a one-off. Reusable compositions go in `AI/Python/` as `.py` files.

---

## Reconciling Variable GUIDs at Commit

The commit primitive reconciles the variable-name-to-GUID map against the tree before compiling, so a batch of tree ops is consistent however it ended. Defer GUID assignment to commit rather than at construction, so an aborted batch leaves no dangling GUID. Every tree widget needs a GUID, not only the ones flagged as variables — the root panel and any named widget count too. The reconciliation set is the root plus all descendants; mint a GUID for any tree widget missing one and prune entries whose widget is gone, leaving animation GUIDs alone. The widget-BP compiler runs the same validation and is self-healing — it adds a missing GUID and continues — so a GUID-map mismatch surfaces as a logged ensure during compile, not a corrupted asset. See `GeoWidgetBuilderUtil.cpp` (`CommitTree`).

The compiler validates against every widget **outer-owned by the WidgetTree**, parented or not — a superset of the root-walk the reconciliation uses. To take a widget out of a tree, always rename it out to the transient package after detaching it, so both walks agree; detaching alone leaves it in the compiler's set with no GUID entry, and after the following save/reload it leaves a GUID entry with no widget. The remove and construct primitives do this rename; any new code path that drops a widget must too. See `GeoWidgetBuilderUtil.cpp` (`RemoveWidget`, `ConstructWidgetInTree`).

When the editor runs under a debugger, these compiler ensures trap as breakpoints and freeze the bridge until execution resumes; the compile then continues and heals the map.

---

## Widget Tree Shim (C++)

**Build.cs** — add to editor-only deps: `"UnrealEd"`, `"Blutility"`, `"UMGEditor"`.

**Includes:**
- `"WidgetBlueprint.h"` — bare filename (UMGEditor Public root), NOT `"Blueprint/WidgetBlueprint.h"`
- `"Blueprint/WidgetTree.h"` — UMG runtime, the `"Blueprint/"` prefix is correct here
- `"Kismet2/KismetEditorUtilities.h"` — in UnrealEd, no extra module needed

See `Source/GeoTrinity/Public/Tool/GeoWidgetBuilderUtil.h` and its `.cpp` for the construction pattern.

Widget names passed to `ConstructWidget` become the `BindWidget` variable names the C++ class binds against.

A shim-constructed widget is only reachable as a Blueprint variable (graph nodes, designer Details, `BindWidget` properties) when it is flagged as a variable; leave layout-only widgets unflagged and flag the ones the graph or C++ needs.

Parameterize the constructed root panel's name so a built root can satisfy a specific `BindWidget` variable; pass the variable name from the caller rather than hardcoding it. See `Source/GeoTrinity/Public/Tool/GeoWidgetBuilderUtil.h`.

---

## Rebuilding an Existing Widget Blueprint

When a build shim rebuilds a widget tree on an already-existing asset, clear the widget-variable GUID map as part of resetting the root. The widget BP compiler only auto-assigns variable GUIDs when that map is empty; stale entries from the previous tree otherwise leave freshly constructed widgets without a GUID. See `GeoWidgetBuilderUtil.cpp` (`BeginBuild`).

When a rebuild changes the root panel's class (e.g. Overlay to VerticalBox), rename the previous root object out to the transient package before constructing the new one — UE refuses to replace an existing object with one of a different class under the same name. See `GeoWidgetBuilderUtil.cpp` (`ConstructRootPanel`).

---

## Appending a Child to an Existing Tree

To add one named child into an existing tree without rebuilding it, do not clear the root or the GUID map — existing widgets keep their GUIDs. Register a fresh GUID for the new child's name yourself, since the compiler mints GUIDs only when the map is empty. The compiler's verify pass requires a GUID for every appended widget, including layout-only ones like spacers — not just the bound child. Make the add reuse-safe by removing any existing widget of that name first. See `GeoWidgetBuilderUtil.cpp` (`AddChildToCanvasPanel`).

To insert at a specific position rather than appending, use the panel's insert-at-index call with the index of an existing sibling. See `GeoHudWidgetBuilderUtil.cpp` (`AddLocalConnectToMainMenu`).

---

## Child User-Widget Templates Ignore Later CDO Edits

A user-widget template constructed inside another widget's tree holds its own property values; setting a default on the child class CDO afterwards does not reach it. Set the property on the template object inside the parent asset (what the designer's Details panel edits) and save the parent. Re-running a builder that reconstructs the template requires re-applying such per-template values. See `AI/Python/local_connect_menu.py` (`set_child_template_property`).

---

## Composing a Tree from Generic Primitives

Keep generic, parameterized tree primitives (begin/finish, construct-root-panel, image-root) in one shim and build content-specific trees in a separate shim that composes those primitives. See `Source/GeoTrinity/Public/Tool/GeoHudWidgetBuilderUtil.h` and `MCP_EditorUtility.md`.

---

## Setting Widget-Blueprint Default Properties

Set a widget Blueprint's class-default values (e.g. a material reference, a slot widget class) on the default object of its generated class from Python, then save the asset. Use this to wire asset references that would otherwise be set in the designer's Details panel.

---

## Driving a Child Widget from C++ Instead of BP Nodes

To rebuild or update a nested widget without authoring event-graph nodes, give the parent a C++ base class that `BindWidget`s the child and exposes a build/update method; the owning HUD or actor calls that method directly in C++. Reparent the designer Blueprint to the C++ base and name the child to match the `BindWidget` variable. This replaces BlueprintImplementableEvent forwarding with a direct C++ call. See `Source/GeoTrinity/Public/HUD/GeoOverlayWidget.h`.

---

## ProgressBar Style

`UProgressBar::WidgetStyle` direct access is deprecated since UE 5.1 — use the getter/setter pair. See `GeoWidgetBuilderUtil.cpp` for usage.

---

## Fixed-Position Canvas Slot

Pin a child to an absolute pixel offset by using a top-left-only anchor and pixel offsets in the slot. Canvas Y=0 is the **top** — for a bottom-anchored overlay, compute `TopOffset = (1 - maxRatio) * BarHeight`. See `GeoWidgetBuilderUtil.cpp` for usage.

---

## ProgressBar Fill Direction

Set `SetBarFillType` explicitly — the default is `LeftToRight`. For a vertical bar use `BottomToTop`. The pixel dimensions of the WidgetComponent `draw_size` must match the bar orientation (`width < height` for vertical).

---

## Inspecting a Widget Blueprint at Runtime

Call `InspectWidgetBlueprint` on the shim CDO from Python to log the full widget tree — types, names, slot layout, and per-widget properties. Use this to verify fill type, offsets, and colors before debugging in PIE. See `GeoWidgetBuilderUtil.h`.

---

## Reading a Widget Blueprint's Parent Class

A `WidgetBlueprint`'s parent-class property is protected and unreadable from Python directly. Read it from the asset-registry tag instead, which exposes both the immediate and the native parent class. Use this to confirm a designer Blueprint already reparents to the intended C++ base before driving it.

---

## Grouping Existing Widgets Under One Anchored Container

To make a cluster of separately-placed canvas children scale and move as a unit, wrap them in one new panel anchored on the parent canvas, without moving anything on screen. Re-parent each existing child into the new panel rather than re-creating it, so it keeps its name, GUID, and graph bindings. Set the new panel's pixel rect to the cluster's bounding box (minimum left/top of the children, extent to the maximum right/bottom). Preserve each child's on-screen position by rebasing its offsets — subtract the bounding-box origin from its original canvas offsets — and a child-canvas-in-canvas keeps pixel offsets exact with no alignment guesswork. Re-parent in the intended draw order, since panel child order is z-order. See `AI/Python/group_widgets.py`.

---

## Finding the Correct Asset Path

The WidgetComponent's `widget_class` property reveals the actual asset path in use — always verify this before rebuilding, as there may be multiple assets with the same name in different folders.

---

## WidgetComponent on an Actor Blueprint

`draw_size` is `IntPoint`, not `Vector2D` — pass it accordingly from Python.

`draw_size` orientation must match the bar: use `(width, height)` where `height > width` for a vertical bar.

Configure via `SubobjectDataSubsystem` handle enumeration: print all indices first to identify the target, then mutate. See `AI/Python/charge_beam_gauge.py` for the full pattern.

---

## Single-Image Widget

A widget showing one image (cursor, icon, …) needs only an Image root — no canvas or slotting, since the hotspot is the widget's top-left. Build it with the generic image-root shim primitive and supply the concrete texture and size from a typed Python caller. To draw only the bright parts of a texture (black background transparent), feed the Image a UI-domain translucent material that maps luminance to opacity rather than a raw texture. See `AI/Python/crosshair_cursor.py`.

---

## Constraining a Label to a Fixed Width

To stop a text label from exceeding a fixed width, cap its width with a SizeBox and wrap the text in a ScaleBox set to scale-to-fit-X, down-only — short text keeps its size and only over-wide text shrinks to fit. See `GeoHudWidgetBuilderUtil.cpp` (`BuildAbilitySlotWidget`).

---

## Widget Tree Primitive Call Order

Call `ConstructWidgetInTree` first to stage the widget and reserve its GUID, then `AttachWidget` to parent it, then set slot/widget properties on the returned objects from Python, then `CommitTree` once. Calling `ConstructWidgetInTree` a second time after `AttachWidget` (e.g. to re-obtain a reference) triggers the GUID ensure during commit — use the object returned by the first construct call instead.

---

## Single-Child Containers Silently Evict on Re-Attach

A single-slot container (`SizeBox`, `Border`, …) holds exactly one content child. Attaching a widget into one that already has a child does not nest it — it detaches the previous child instead, and anything not reachable from the root at commit time is dropped from the saved asset, taking its whole subtree with it.

To add a layer (e.g. a background image) inside an existing single-slot container, insert a multi-child panel (`Overlay`, `VerticalBox`, …) as its one child first, then attach the background and the existing content into that new panel.

---

## Matching an Existing Widget's Visual Style

To replicate one widget's appearance onto another (e.g. a differently-styled button variant), read the source widget's brush/color properties via the find-by-name lookup and copy each value onto the target rather than guessing at colors — brush tint, resource texture, and draw type must all be copied together, since a tint alone reads differently against a different texture or draw type.

---

## OverlaySlot Alignment Property Names

An `OverlaySlot`'s alignment properties in Python are `horizontal_alignment` and `vertical_alignment`.

---

## Naming & Folder Convention

Widget Blueprints use the `WBP_` prefix and live under `/Game/HUD/`, grouped by widget type.
