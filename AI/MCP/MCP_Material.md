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
- Creating expressions fails while PIE is running, and the editor asset library's load, list and save calls
  return nothing.
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

## Material functions

How to structure a material into functions is `AI/Materials.md`; this is only the Python side. Function-based
material scripts build their graphs through the toolkit `AI/Python/Material/material_graph_authoring.py`.

- A function is created through its own factory and filled through the in-function expression calls; update it
  once built, then recompile each material that calls it.
- A call links to a function's input and output nodes by node id when it reloads, so a function rebuilt with new
  input or output nodes loses every caller's wires. Rebuild one in place by deleting only its body.
- A function's nodes are listed by iterating material expressions for those whose outer is the function; a deleted
  node stays in that iteration, invalid, until garbage collection.
- The function's description, caption, library exposure and library categories, and each input's name, type,
  description, sort priority and use-preview-as-default flag, are plain editor properties.
- An input's preview value is a float4 struct Python cannot construct. An optional input gets its default from a
  constant wired into the input's `Preview` pin, with use-preview-as-default on.
- A call node builds its pins only when its function is assigned with the notify forced to always; its pins are
  then addressed by the function's input and output names.
- Assigning a different function to a call keeps the links of every same-named pin.
- Reading an expression's input links takes a material argument that is only null-checked — inside a function,
  pass any material.

## Material layers

- A layer and a layer blend are each created through their own factory and filled like any function. A layer has
  one material-attributes output and at most one input. A blend has two material-attributes inputs, fed in sort
  order: what is below first, the layer second.
- The stack is the layer-stack node's default-layers struct: the layer and blend lists plus editor-only per-layer
  lists (visibility, name, relatives restriction, guid, link state). There is one blend fewer than layers, and every
  per-layer list matches the layer count; the engine checks both on each change and halts the editor on a mismatch,
  so the struct is edited as a copy and assigned in one set (see struct access in `MCP_Blueprint.md`).
- A new stack node holds one empty background layer under the engine's fixed background guid. Give every added layer
  a fixed guid of its own, so an instance's stack stays linked to it across rebuilds.
- The material reads the stack through its material-attributes input, with use-material-attributes on.
- An instance's own stack has no Python surface. An editor-utility shim puts a layer in one of its slots, unlinked
  from the parent's so it stays while every other layer keeps following the parent (`MCP_EditorUtility.md`,
  `AI/Python/Material/make_background_look_instances.py`). Swapping a layer in the base's stack previews it on every
  instance that keeps the base's stack.
- The toolkit builds a whole stack in one assignment and swaps one layer of an existing one;
  `AI/Python/Material/compare_material_layers.py` compiles candidate layers in one slot and reports each one's
  instruction counts, then puts the slot's own layer back.

## Dry-running a build script

A build script can run outside the editor, under the engine's bundled Python, against a stand-in editor module that
fakes asset loading and node creation. The stand-in checks every call pin and call output against the called
function's declared inputs and outputs, so Python errors and misnamed function pins surface before the editor runs
anything; engine node pin names are not checked. See `AI/Python/Material/dry_run_material_build.py`.

## Graph organisation

- Parameter group, sort priority, description and slider range are editor properties on the parameter node.
- A named reroute declaration can be created, named and fed, but the link from a usage to its declaration is out
  of Python's reach and needs an editor-utility shim.
- Comment boxes need an editor-utility shim too: the expression call files a comment as an ordinary node, and its
  size is not exposed.
- The editing library's statistics call returns the compiled instruction counts of a material or an instance;
  compare them before and after restructuring a graph.

## Hard-edge fill

`Step(Y, X)` returns 1 where `X >= Y` — no smoothstep, no divide, so a fill edge stays pixel-perfect. Both zone
indicators are built on it and both are Unlit, Additive, Two-Sided.

- **Circle** (`M_ZoneIndicator`): centre the UV to `[-1,1]` and take its length as the distance, 0 at the centre
  and 1 at the edge. The ring is a `Step` band just inside 1, the fill a `Step` of the distance against the fill
  parameter; opacity is their max, emissive each colour times its own mask. As shipped it exposes one
  `FillOpacity` scalar with the colours baked into the graph.
- **Ray** (`M_ZoneIndicatorRay`): the same over `abs((u - 0.5) * 2)`, so the fill grows from the centre to both
  side edges and the thin lines mark the two outer edges as the always-visible extent. Parameters are
  `FillOpacity` (drive 0→1 to telegraph), `LineThickness`, `FillColor` and `LineColor`.

`M_HealingZone` uses the same pattern Masked rather than Additive, with a duration scalar.
