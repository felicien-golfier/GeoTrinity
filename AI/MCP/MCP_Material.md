# MCP Material Creation

Material node wiring goes through `execute_script` using `MaterialEditingLibrary`.

## Constraints

- Create the material asset through `AssetTools` first, then add expression nodes; set `blend_mode` and
  `shading_model` **after** everything is wired.
- Connecting expressions returns a bool and does nothing on an unknown pin name — always assert on it, or an
  input silently keeps its default (a texture sample with no UVs samples screen space, which still looks
  plausible).
- Inputs and outputs are matched by their **shortened** display name, not the name the expression reports: a
  texture sample's UV pin is `UVs` and its texture pin is `Tex`. An empty name is input 0. Naming a node's
  channels does not make those names connectable — address a multi-output node by position.
- `MaterialExpressionIf` scalar constants are not settable from Python; use `MaterialExpressionStep` for hard
  thresholds.
- A texture sample's sampler type must match the assigned texture's sRGB setting or the material fails to
  compile.
- A collection parameter node resolves its parameter id in its post-change notification, so set its name with
  the notify mode forced to always; left unresolved it compiles to zero.
- Deleting all expressions removes only every other node per call — loop until the count reaches zero, or a
  rebuild script accumulates dead nodes and stale references. Unconnected nodes still compile, they only clutter.
- Creating expressions fails while PIE is running.
- A material instance is created through its own factory and parented to the base; its parameter values are
  written and read through the editing library's instance calls, which is how a family of looks comes off one
  graph.

`expressions` on a material is protected and the route's parameter setter only works on instances, so changing a
base material's parameter default goes through `find_object` on the auto-indexed inner path.

| Property | Python value |
|---|---|
| Translucent / masked blend | `unreal.BlendMode.BLEND_TRANSLUCENT` / `BLEND_MASKED` |
| Unlit shading | `unreal.MaterialShadingModel.MSM_UNLIT` |
| Opacity mask / opacity output | `unreal.MaterialProperty.MP_OPACITY_MASK` / `MP_OPACITY` |

## Hard-edge fill

`Step(Y, X)` returns 1 where `X >= Y` — no smoothstep, no divide, so a fill edge stays pixel-perfect. Both zone
indicators are built on it and both are Unlit, Additive, Two-Sided.

- **Circle** (`M_ZoneIndicator`): centre the UV to `[-1,1]` and take its length as the distance, 0 at the centre
  and 1 at the edge. The ring is a `Step` band just inside 1, the fill a `Step` of the distance against the fill
  parameter; opacity is their max, emissive each colour times its own mask. As shipped it exposes one
  `FillOpacity` scalar with the colours baked into the graph.
- **Ray** (`M_ZoneIndicatorRay`): the same over `abs((u - 0.5) * 2)`, so the fill grows from the centre to both
  side edges and the thin lines mark the two outer edges as the always-visible extent. Parameters are
  `FillOpacity` (drive 0→1 to telegraph), `LineThickness`, `FillColor` and `LineColor`. Built by
  `AI/Python/Material/make_zone_indicator_ray.py`, which deletes and recreates.

`M_HealingZone` uses the same pattern Masked rather than Additive, with a duration scalar.
