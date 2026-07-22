# MCP Material Creation

All material node wiring goes through `execute_script` (Python) using `MaterialEditingLibrary`.

---

## Key Constraints

- Create the material asset first via `AssetTools`, then add expression nodes to it.
- Set `blend_mode` and `shading_model` **after** all nodes are wired, not before.
- `MaterialExpressionIf` scalar constant properties are not accessible via `set_editor_property` in Python. Use `MaterialExpressionStep` instead for hard thresholds.
- Unused expression nodes that are not connected still compile fine but clutter the graph.

---

## Editing an existing material's parameter defaults

`mat.get_editor_property("expressions")` is protected. `material_ops set_parameter` only works on material instances, not base materials.

To change a base material parameter default value, use `find_object` with the auto-indexed inner path. See `AI/Python/material_edit.py` for the pattern.

---

## Hard-edge circle fill pattern

Used in `M_ZoneIndicator` (`/Game/VFX/Generic/Materials/M_ZoneIndicator`).

```
UV → subtract 0.5 → multiply 2  →  uvn ([-1,1] centered)
uvn · uvn → sqrt                →  dist (0 at center, 1 at edge)

Ring  = Step(1 - RingThickness, dist)  *  Step(dist, 1)
Fill  = Step(dist, FillPercent)           ← pixel-perfect, no blur

OpacityMask = max(Ring, Fill)
Emissive    = RingColor * Ring  +  FillColor * Fill
```

`Step(Y, X)` returns 1 where `X >= Y`, 0 otherwise — no smoothstep, no divide.

**Parameters:**
| Name | Type | Purpose |
|---|---|---|
| `RingColor` | Vector | Outer ring tint |
| `FillColor` | Vector | Inner fill tint |
| `FillPercent` | Scalar 0–1 | Hard fill level — set from BP/timeline |
| `RingThickness` | Scalar | Ring width as fraction of radius |

**Material settings:** Unlit, Masked, Two-Sided.

**Actual `M_ZoneIndicator`** on disk is Unlit, **Additive**, Two-Sided with a single `FillOpacity` scalar (colors baked in-graph) — the table above is the idealized pattern, not the shipped node set.

---

## Ray (bar) zone indicator — `M_ZoneIndicatorRay`

Horizontal variant: fill grows from the center to **both** side edges; thin lines mark the two outer edges (always visible extent). Built by `AI/Python/make_zone_indicator_ray.py` (idempotent — deletes + recreates).

```
u   = TexCoord.x
dX  = abs((u - 0.5) * 2)              → 0 at center, 1 at both edges
Fill = Step(dX, FillOpacity)          → 1 where dX <= FillOpacity
Edge = Step(1 - LineThickness, dX)    → 1 near the two outer edges
Emissive = FillColor*Fill + LineColor*Edge
Opacity  = max(Fill, Edge)
```

Params: `FillOpacity` (drive 0→1 to telegraph), `LineThickness`, `FillColor`, `LineColor`. Settings mirror the circle: Unlit, Additive, Two-Sided.

Reference: `M_HealingZone` (`/Game/AbilitySystem/Abilities/Circle/HealingZone/VFX/M_HealingZone`) uses the same Masked + Unlit pattern with a `DurationPercent` scalar.

---

## Enum names

| Property | Correct Python value |
|---|---|
| Translucent blend | `unreal.BlendMode.BLEND_TRANSLUCENT` |
| Masked blend | `unreal.BlendMode.BLEND_MASKED` |
| Unlit shading | `unreal.MaterialShadingModel.MSM_UNLIT` |
| Opacity mask output | `unreal.MaterialProperty.MP_OPACITY_MASK` |
| Opacity output | `unreal.MaterialProperty.MP_OPACITY` |
