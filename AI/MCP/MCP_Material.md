# MCP Material Creation

All material node wiring goes through `execute_script` (Python) using `MaterialEditingLibrary`.

---

## Key Constraints

- Use `AssetTools.create_asset` to get a material object, then call `create_material_expression` on it.
- Set `blend_mode` and `shading_model` **after** all nodes are wired, not before.
- Blend mode enum: `unreal.BlendMode.BLEND_TRANSLUCENT`, `unreal.BlendMode.BLEND_MASKED`. (`EBlendMode` does not exist.)
- `MaterialExpressionIf` scalar constant properties are not accessible via `set_editor_property` in Python. Use `MaterialExpressionStep` instead for hard thresholds.
- Unused expression nodes that are not connected still compile fine but clutter the graph. Delete them with `mel.delete_material_expression(mat, node)`.

---

## Editing an existing material's parameter defaults

`mat.get_editor_property("expressions")` is protected. `material_ops set_parameter` only works on material instances, not base materials.

To change a base material parameter default value, use `find_object` with the auto-indexed inner path:

```python
mat = unreal.load_asset("/Game/Path/MyMaterial")
for i in range(10):
    obj = unreal.find_object(None, f"/Game/Path/MyMaterial.MyMaterial:MaterialExpressionVectorParameter_{i}")
    if obj:
        name = obj.get_editor_property("parameter_name")
        if name == "MyColor":
            obj.set_editor_property("default_value", unreal.LinearColor(1.0, 0.5, 0.0, 1.0))
unreal.MaterialEditingLibrary.recompile_material(mat)
unreal.EditorAssetLibrary.save_asset("/Game/Path/MyMaterial")
```

Same pattern works for `MaterialExpressionScalarParameter` — replace with `default_value` float.

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
