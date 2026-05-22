# MCP UI — Widget Blueprint Automation

Creating and configuring UMG Widget Blueprints via `execute_script` and C++ shims.

---

## Creating a Widget Blueprint

Use `WidgetBlueprintFactory` with `parent_class` set, then create via `AssetTools`. See `AI/Python/charge_beam_gauge.py` for a full example.

`WidgetBlueprintFactory` has no `use_inherited_viewport_size` property — do not set it.

---

## Widget Tree

Python cannot read or write the widget designer tree — `WidgetBlueprint` exposes no `widget_tree` property to Python. Adding any widget to the canvas requires a C++ `UEditorUtilityObject` shim.

---

## Widget Tree Shim (C++)

**Build.cs** — add to editor-only deps: `"UnrealEd"`, `"Blutility"`, `"UMGEditor"`.

**Includes:**
- `"WidgetBlueprint.h"` — bare filename (UMGEditor Public root), NOT `"Blueprint/WidgetBlueprint.h"`
- `"Blueprint/WidgetTree.h"` — UMG runtime, the `"Blueprint/"` prefix is correct here
- `"Kismet2/KismetEditorUtilities.h"` — in UnrealEd, no extra module needed

See `Source/GeoTrinity/Public/Tool/GeoWidgetBuilderUtil.h` and its `.cpp` for the construction pattern.

Widget names passed to `ConstructWidget` become the `BindWidget` variable names the C++ class binds against.

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

## Finding the Correct Asset Path

The WidgetComponent's `widget_class` property reveals the actual asset path in use — always verify this before rebuilding, as there may be multiple assets with the same name in different folders.

---

## WidgetComponent on an Actor Blueprint

`draw_size` is `IntPoint`, not `Vector2D` — pass it accordingly from Python.

`draw_size` orientation must match the bar: use `(width, height)` where `height > width` for a vertical bar.

Configure via `SubobjectDataSubsystem` handle enumeration: print all indices first to identify the target, then mutate. See `AI/Python/charge_beam_gauge.py` for the full pattern.

---

## Naming & Folder Convention

Widget Blueprints use the `WBP_` prefix and live under `/Game/HUD/`, grouped by widget type.
