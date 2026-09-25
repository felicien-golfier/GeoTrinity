# Materials

How a material is built and organised in GeoTrinity. The look it serves is `AI/ArtDirection.md`; wiring nodes from
Python is `AI/MCP/MCP_Material.md`; custom primitive data and the zone materials are in `AI/VFX.md`.

## A material is a chain of functions

A big material is a short master graph that calls one function per stage, read left to right:

| Stage | Job | Background lattice |
|---|---|---|
| Space | The coordinates the pattern lives in | world XY |
| Pattern | Shape field turned into a mask | `MF_Pattern_TriangleLattice` |
| Animation | Whatever moves over the pattern | one `MF_Wave_*` per glow look; the inner triangles' spin |
| Look | Colours and brightness | line colour; each glow's two colours × brightness |
| Outputs | Base colour, emissive, opacity mask | — |

The master holds only the parameters, the calls and the final combine, so it fits on one screen and each stage is
replaced without touching the others.

A call costs nothing at runtime: the compiler walks into the function and emits its nodes straight into the
caller's shader, and only the outputs actually wired get compiled. Splitting a flat graph into functions therefore
leaves the instruction count unchanged — read the material's statistics before and after to prove it.

A stage that has to be swapped per instance, rather than in the base, becomes a material layer. The master is then
a layer stack. Each layer is a function asset giving material attributes, with its knobs as parameters inside it.
An instance swaps, adds, hides and reorders layers in its Layer Parameters. A blend combines each layer with
everything below it. Only a stack's visible layers compile into the instance's shader, and the stack itself adds
no instructions: the lattice compiles to the same count as one stack as it did as one flat graph of calls.
Collapse Nodes only tidies one graph; anything reused or swappable is a function.

## Two kinds of function

| Kind | Lives in | Rules |
|---|---|---|
| Generic: maths, distance fields, masks, falloffs | `/Game/Art/VFX/Generic/Materials/Functions` | Inputs only. No parameter, no collection, no world-position read inside, so it works in any material. |
| Feature: one material's building block | a `Functions` folder beside its material | May read a collection or world position. Owns a pin contract so a sibling function can replace it. |

`/Game/Art/VFX/Generic/Materials/MatFunctions` is an imported VFX pack (`MF_VFX_MatLayers_*`, `MF_Dissolve`, …),
not project functions — nothing new goes there. Before authoring a generic function, search the engine's
(`/Engine/Functions`, right-click search in any graph) and the project's.

## Shapes are distances, lines are strokes

A generic shape function returns the distance to the shape's outline, in the units of its position. A stroke turns
any distance into a drawn line. Every stroke has the same pins, a distance and a width in, a mask out, so any shape
can be drawn with any stroke, and swapping the stroke changes a line's profile without touching the shape. A
distance can be reshaped before it is stroked, and a cell function gives every tile of a tiling its own centred
coordinates, so one shape drawn there repeats in every tile.

| Function | Does |
|---|---|
| `MF_RepeatDistance` | Distance from a value to the nearest multiple of a spacing |
| `MF_PolarCoordinates` | A point's distance and angle, in turns, around the origin |
| `MF_ParallelLinesDistance` | Distance to the nearest of evenly spaced parallel lines, one of them through the origin |
| `MF_CircleDistance` | Distance to a circle's outline |
| `MF_PolygonDistance` | Distance to a regular polygon's outline, sharp corners, any side count, turnable |
| `MF_DoubleLineDistance` | Reshapes a distance into two copies of the path a gap apart: stroked, a doubled line |
| `MF_TriangleCell` | For the triangle a point falls in: the point's position from its centre, every triangle turned alike; the centre; whole-number coordinates |
| `MF_Stroke_Hard` | Hard-edged line, `Width` wide |
| `MF_Stroke_Linear` | Line fading linearly from its centre: `Width` wide at half brightness, gone `Width` away |
| `MF_Stroke_Smooth` | Line easing out with rounded shoulders: `Width` wide at half brightness, gone `Width` away |
| `MF_TwoToneStroke` | Two smooth strokes of one distance: a wide `Glow` and a narrow `Core` |
| `MF_TwoToneColor` | Colours a `Glow`: its `Core` share in one colour, the rest in another |
| `MF_SierpinskiMask` | 1 on the Sierpinski triangle over two cell coordinates, repeating every 32 |
| `MF_SharedBit` | 1 where two whole numbers share one binary digit: the Sierpinski mask's step |
| `MF_RandomFromPosition` | Repeatable pseudo-random 0–1 per position, such as one value per cell |
| `MF_LissajousPoint` | A point swinging around a centre on its own frequency per axis: a slow wandering path |
| `MF_DurationWipe` | Clock-wipe mask for remaining-life readouts |
| `MF_WrapIndex` | A whole number wrapped into 0 to Count − 1, negatives included |
| `MF_PickColor` | One of four colours, by index |
| `MF_ColorPattern_Zigzag` | Flat-topped zigzag bands, one colour each in turn: every colour gets the same share |
| `MF_ColorPattern_Overlap` | Two sets of sliding quads, coloured by how many cover a point: even shares only at two colours |
| `MF_ColorPattern_Stripes` | Straight bands, one colour each in turn: the telegraphs' pattern |
| `MF_MeaningColors` | Up to four colours, one per meaning, split by the project's colour pattern laid over the world: what effects call |
| `MF_ColorScale` | How many times brighter a colour is than the one it was scaled from, such as a faded particle colour |

All are built by `AI/Python/Material/make_generic_material_functions.py`, except the last seven (and the
`MF_SlidingQuads` the overlap pattern calls), built by `make_color_pattern_functions.py`.

## Several meanings at once

An effect that carries two meanings — damage and heal, reduction and boost — shows both colours at once, in big
hard-edged regions rather than a blend. Every such material calls `MF_MeaningColors` (`ColorCount`, `Color0`–`3`
in, `Color` out), never a pattern directly. The pattern is the same on every effect whatever its shape or scale:
`MF_MeaningColors` lays it over the world's XY, at the size and speed of `MPC_MeaningColors` (`PatternSize` in world
cm, `PatternSpeed` in bands per second), the one place to tune it. That makes it the one generic function reading
world position and a collection. Inside it a `MF_ColorPattern_*` turns the position into a colour index and
`MF_PickColor` turns that into the colour; which pattern is the `PATTERN` constant of
`make_color_pattern_functions.py`, then rebuild. Every pattern has the same pins (`Position`, `Scroll`, `ColorCount`
in, `ColorIndex` out). At `ColorCount` 1 every pattern returns 0 and the effect looks as it did with one colour.

`AGeoEffectZone` writes `Color` plus its `SecondaryColors` into `M_PulseCircle`'s `InsideColor`, `InsideColor2`–`4`
and `ColorCount`. The beam and the two telegraphs (`M_PulseBeam`, `M_ZoneIndicatorRay`, `M_ZoneIndicator`) take theirs
from their Niagara system (`AI/VFX.md`).

A telegraph asks for its own pattern: the telegraph materials call `MF_TelegraphColors` instead, straight stripes over
the world at `MPC_MeaningColors`' `TelegraphStripeWidth` and `TelegraphStripeAngle`, each stripe taking the next
colour. A telegraph never draws one flat colour: alone, its colour is paired with a shade of itself
(`SingleColorShade`), so a single-meaning telegraph still shows its stripes.

## Writing a function

- Name it `MF_<What>`. Interchangeable functions share a prefix: `MF_Pattern_<Name>`, `MF_Stroke_<Name>`. A
  layer is `ML_<What>` and a layer blend `MLB_<What>`, prefixed the same way (`ML_Pattern_`, `ML_Wave_`).
- Fill the function description (the palette tooltip) and every input and output description, with units: world
  cm, turns, 0–1.
- Input sort priority is the pin order on the call node. An output is named for what comes out (`LineMask`,
  `Distance`), never for the last operator.
- An optional input takes its default from the node wired into its Preview pin, with Use Preview Value As Default
  on. Any other unwired input is a compile error.
- Expose to Library with category `GeoTrinity|<Topic>`, so the right-click search finds it.
- A block repeated N times is one function called N times, never N copies of the nodes.

## Swapping a stage: the pin contract

Changing the function a call node points at (Details ▸ Material Function) keeps every input wire whose input name
matches and every output wire whose output name matches. Functions meant to replace each other use exactly the same
pin names and types, which turns a swap into one dropdown with no rewiring. Renaming an input inside a function
keeps its callers' wires.

## Parameters

- Parameters sit in the master graph, not inside functions: the instance shows one flat list and every knob is
  visible where its stage is called. A parameter inside a function is shared by every call to it.
- A layer is the exception: its knobs are parameters inside it, so an instance that swaps the layer gets the new
  one's knobs with it, listed under that layer. Every glow layer groups them the same way, `Shape` then `Colour`.
- Group them with numbered names (`01 Pattern`, `02 Inner Triangle`) because groups sort alphabetically. Sort
  Priority orders within a group. The description is the instance tooltip: give units, and point to any related
  knob that lives outside the material.
- Tune on the instance, never the base; meshes reference the instance.
- Values gameplay writes every frame go through a parameter collection (global) or custom primitive data (per
  primitive), never a dynamic instance per actor — see `AI/VFX.md`.

## Keeping a graph readable

- Flow left to right, one stage per column, outputs at the far right.
- One comment box per stage, colour-coded; a node description for a single-node note.
- A value used far from where it is made — world XY feeding both pattern and animation — goes through a named
  reroute instead of a long wire.

## Background lattice

`/Game/Art/VFX/Background/M_BackgroundLattice`, the floor's triangle line art. Its instance `MI_BackgroundLattice` is
on the mesh of the `/Game/Art/Meshes/Floor` Blueprint and every placed floor. Masked and Default Lit. The material
is a layer stack and nothing else: layers in `Background/Layers`, their functions in `Background/Functions`.

| Layer | Asset | Gives |
|---|---|---|
| 0, Pattern | `ML_Pattern_TriangleLattice`, calling `MF_Pattern_TriangleLattice` | line colour, line mask |
| 1 and up, Glow | any `ML_Wave_*`, blended by `MLB_AddGlow` | glow on the lines |

`MLB_AddGlow` keeps layer 0's colour and mask and adds each glow layer's emissive to the glow below it, so glow
layers stack in any order. Only the lines draw: a glow shows where it crosses them, and one much narrower than a
triangle reads as dashes. `make_background_lattice_material.py` builds the collection, the pattern and ring layers,
the blend, the material and its instance; `make_background_looks.py` builds every other look, and
`make_background_look_instances.py` gives each look its instance.

### Trying a look

Open `MI_BackgroundLattice`, Layer Parameters:
- Change the Glow layer's asset to another `ML_Wave_*` to swap looks.
- Add a layer with `MLB_AddGlow` as its blend to stack a second look on the first; the eye hides one.
- Each layer's knobs sit under it.

| Look | Driven by | What it is |
|---|---|---|
| `ML_Wave_Rings` | pulses | The original rings, one colour, fading linearly both sides |
| `ML_Wave_SoftRings` | pulses | Rings soft on both sides, the core colour along their middle, the edge colour toward their sides |
| `ML_Wave_ShockRings` | pulses | A hard bright front in the core colour, a soft wake of the edge colour trailing toward the origin |
| `ML_Wave_PolygonRings` | pulses | Rings as regular polygons, triangles by default, born along the lattice and turning as they grow |
| `ML_Wave_Halos` | pulses | Whole triangles lit around each pulse origin, the core colour nearest it |
| `ML_Wave_Spiral` | Time | Spiral arms turning around the arena's centre |
| `ML_Wave_Whirl` | Time | Nested polygons, each twice the last, growing outward forever and twisting |
| `ML_Wave_Sierpinski` | Time | Cells lit as a Sierpinski triangle 32 rows high, repeating; a band prints it row by row |
| `ML_Wave_Radar` | Time | Beams sweeping around the arena's centre, a hard leading edge and a fading trail |
| `ML_Wave_Twinkle` | Time | Single triangles flashing at random, each on its own clock |
| `ML_Wave_Fireflies` | Time | Five lights drifting around the arena's centre on slow paths, lighting the triangles under them |

- **Pulse looks** follow `BP_GeoCam`'s `BackgroundPulse` component (`UGeoBackgroundPulseComponent`), which writes
  their collection every frame. Its `Mode` sets the pulse origins: `Actors` puts them on the nearest characters and
  deployables, so halos sit under them. `Straight` and `Wander` move them, so halos drift. Nothing writes the
  collection in the editor, so these looks show nothing there.
- **Time looks** move on their own, the editor viewport included. Spiral, Whirl, Radar and Fireflies centre on
  the arena the floor belongs to, and stay put while the players move. In the editor, where no arena runs, they
  centre on the world origin.

### Looks per arena

Each look has its own instance in `Background/Looks`, `MI_BackgroundLattice_<Look>`: a child of
`MI_BackgroundLattice`, so the lines are tuned in one place, with that look as its glow layer. Tune a look on its
instance; that is what the game shows.

A placed `AGeoArena` lists its `Floors` and the `BackgroundLooks` they cycle through. Its floors start on the first
look, and every wipe in that arena moves them on to the next, wrapping after the last. The index replicates, so
every machine shows the same look. An arena with no looks of its own takes Game Data Settings ▸ `BackgroundLooks`,
which lists every look. A floor no arena lists keeps `MI_BackgroundLattice`. Floor pieces that overlap go to the same
arena: coplanar pieces showing different looks z-fight.

The arena also writes its XY into custom primitive data 1–2 of each of its floors (slots registered in
`AI/VFX.md`). The centred looks read that through their `ArenaCenter` parameter, which an instance has no reason to
touch.

### How it is built

- **Pattern.** Three sets of parallel lines at 60° to each other, spaced `ShapeSize·√3/2` and all through the
  origin, cut the plane into equilateral triangles and nothing else, so one stroke over the distance to the nearest
  line draws every triangle at once: three `MF_ParallelLinesDistance`, their minimum, `MF_DoubleLineDistance` to
  split every edge into two lines `LineGap` apart, `MF_Stroke_Hard`. Shifting one set along its normal turns the
  tiling into triangles and hexagons (kagome).
- **Inner triangles.** `MF_TriangleCell` gives each point its position from the centre of its triangle, every
  triangle turned alike, so one `MF_PolygonDistance` with three sides draws a small triangle in every cell, nested
  in its cell at rotation 0. The pattern layer spins it: `Time × InnerTurnSpeed + InnerRotation` feeds the
  pattern's `InnerRotation` pin.
- **Glow.** Every `MF_Wave_*` but the original rings gives `Glow`, a brightness, and `Core`, the part of it in the
  core colour. Its layer colours them with `MF_TwoToneColor`: `EdgeColor × (Glow − Core) + CoreColor × Core`, times
  `Brightness`. Equal colours give one tone. The original rings give `Pulse`, times `PulseBrightness` and
  `PulseColor`.
- **Pulses.** `MPC_BackgroundPulse` holds eight `PulseSource_XX` = (OriginX, OriginY, Radius, Intensity). A pulse
  look calls its one-slot function (`MF_PulseRing`, `MF_SoftRing`, `MF_ShockRing`, `MF_PolygonRing`,
  `MF_PulseHalo`) once per slot and keeps the brightest. An all-zero slot adds nothing, so no sentinel value is
  needed. Pulse looks have no Time node: gameplay owns them, so it can start, stop and synchronise them.
- **Cells.** Halos, Fireflies, Sierpinski and Twinkle light whole triangles through `MF_TriangleCell`'s
  `CellCenter` or `CellIndex`, one value per triangle, so their `CellSize` must equal the pattern's `ShapeSize`.
  Fireflies reuses the halo: each light is a steady pulse slot built in the graph, at a `MF_LissajousPoint`.
- **World space, never UV.** The floor pieces carry non-uniform scales and rotations, so a UV lattice would change
  size on every piece and break at each seam. World XY makes one continuous sheet, in the same space as the pulse
  origins.

| To change | Do |
|---|---|
| The shape | A new `MF_Pattern_<Name>` with the same pins, in a new `ML_Pattern_<Name>` picked as layer 0 |
| Where the lattice sits or how it moves | Transform world XY before the pattern's Position in `ML_Pattern_TriangleLattice` |
| A ring's profile | Swap the stroke inside the one-slot function, such as `MF_Stroke_Hard` in `MF_PulseRing` |
| A new look | An `MF_Wave_<Name>` giving `Glow` and `Core`, in an `ML_Wave_<Name>` holding its knobs, added to `make_background_looks.py`; then its instance through `make_background_look_instances.py`, listed in an arena's or the settings' `BackgroundLooks` |
| What turns or scales the inner triangles | Replace what feeds the pattern's `InnerRotation` and `InnerScale` pins in `ML_Pattern_TriangleLattice`, for an event a collection value gameplay writes |

| Knob | Lives on |
|---|---|
| Triangle size, line width, edge doubling, line colour | pattern layer: `ShapeSize`, `LineThickness`, `LineGap`, `LineColor` |
| Inner triangle size and motion | pattern layer: `InnerSize`, `InnerScale`, `InnerTurnSpeed`, `InnerRotation` |
| A look's shape, speed, colours and brightness | its glow layer, on the look's instance in `Background/Looks` |
| Which floors an arena dresses, and the looks they cycle through | the placed `AGeoArena`: `Floors`, `BackgroundLooks` |
| The looks of every arena without its own | Game Data Settings: `BackgroundLooks` |
| Ring speed, reach (radius where it dies), ring count | `BP_GeoCam`'s `BackgroundPulse` component: `RingSpeed`, `MaxRingRadius`, `PulseCount` |
| Where pulses start and how their origins move | same component: `Mode`, `AreaRadius`, `MoveSpeed`, `TurnRate` |
