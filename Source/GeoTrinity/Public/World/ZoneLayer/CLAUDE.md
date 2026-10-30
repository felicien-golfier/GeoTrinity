# World/ZoneLayer

Every zone, beam and telegraph is drawn by one world layer, not by its own material: where shapes overlap, the overlap shows every meaning's colour (`AI/Materials.md` "Zone layer"). Colours, outlines and fill shades all come from the layer; a shape is data only. Pure code: no Niagara. Cosmetic only, never gameplay state; the dedicated server has no layer.

| File | Role |
|---|---|
| `GeoZoneShape.h` | `EGeoZoneShape` (`Circle`, `Beam`), `EGeoZoneLayer` (`Ground`, `Telegraph`), `FGeoZoneShape` (local-space size), slot constants + `GeoZoneLayer::PackSlot` (a shape's world transform + size + colours + flags + fill → its slot's texels, pure) + `MakeColorMask` |
| `GeoZoneVisualComponent.h` | `UGeoZoneVisualComponent` (scene component, no mesh): one drawn shape; registers itself with the subsystem (`OnRegister` / `OnUnregister`) |
| `GeoZoneLayerSubsystem.h` | `UGeoZoneLayerSubsystem` (tickable world subsystem): registry → slot texture every tick; owns the layer actor |
| `GeoZoneLayerActor.h` | `AGeoZoneLayerActor`: the two layer planes (`MI_ZoneLayer_Ground`, `MI_ZoneLayer_Telegraph`, `GameDataSettings::ZoneLayer*Material`) |

## Draw order (both planes at Z = `ZoneLayerZ` = 1, translucency sort priority)
| Priority | Plane | Draws, per pixel |
|---|---|---|
| 0 | Ground | `Ground` slots: mixed colours → fill shade → outlines |
| 1 | Telegraph | `Telegraph` slots: mixed colours in stripes → fill shade → outlines |

Niagara VFX of an ability (start / stop, purely visual) are separate and sit on top; nothing here knows them.

## Shape (`UGeoZoneVisualComponent`)
- Set by its owner: `Shape` (`FGeoZoneShape`, local space: circle = `Radius` round the component; beam = `Length` along +X from the component, `HalfWidth` each side), `ColorMask`, `Layer` (`Ground` | `Telegraph`), `bSafeSpotCut`, `Fill`.
- Attached to its owner: the slot reads the component's world transform every tick, so an aimed ray or a moving zone follows with no push.
- Colours = `ColorMask`: one bit per `EGeoColor` ordinal (`GeoZoneLayer::MakeColorMask(Color, SecondaryColors)`). The materials resolve bits through the palette texture (`GeoColor::CreatePaletteTexture`). `Override` carries no palette colour: never set on a drawn shape (ensure, bit dropped); a new look is a new palette slot.
- `Fill` 0..1 = time spent of what the shape announces or lasts: the shade grows from the shape's core (circle centre, beam middle line) and reaches the outline at 1. Owner-written (a zone: `1 - GetDrainDurationRatio()` each tick; hand-placed: 0), or `SetTimedFill(StartServerTime, Duration)`: the component fills itself off the server clock (telegraphs, late joiners included); `GetFill()` is what is drawn.
- Outline: every shape keeps its own, in its first colour, so two overlapping zones still read as two.
- Shown = registered: from `OnRegister` (game and editor worlds alike) until `SetZoneVisible(false)` or `OnUnregister`. No fade in or out: on is on.
- Owner hidden in game (a blinking or expired deployable) = not drawn that tick; an ownerless shape (a free telegraph, outer = the world) always is.

## Slots (`UGeoZoneLayerSubsystem`)
- `MaxZoneSlots` = 128 shapes at once (healer alone up to 8 zones, every class and boss on top).
- Slot texture: transient `PF_A32B32G32R32F`, `MaxZoneSlots` wide × 3 rows, rewritten every tick; plus `ZoneCount` (scalar on both plane MIDs). A texture, not an MPC: one HLSL loop reads a slot by index, so the graph holds one texture parameter whatever `MaxZoneSlots` is; cost follows `ZoneCount`, not the maximum.
  - Row 0: `Shape` ordinal, `ColorMask`, `Flags` (`Telegraph`, `SafeSpotCut`), `Fill`.
  - Rows 1–2: the shape's world-space parameters, 8 floats (circle: centre XY, radius; beam: start XY, end XY, half-width).
- Slots are packed in registration order, no gaps; `ZoneCount` stops the material loop, so 10 live shapes cost 10 iterations.
- Over 128: ensure once, log, the extra shapes are not drawn.
- Supports game, PIE and editor worlds (ticks in the editor: a hand-placed zone shows in the viewport); never created in a dedicated server process, and a play-in-editor dedicated server world skips its tick.
- `PackSlots()` (registry → texels + `ZoneCount`) is public for the tests; `Tick` = spawn the layer actor if needed, `PackSlots`, upload.

## Adding a shape
1. `EGeoZoneShape` entry + its `FGeoZoneShape` fields.
2. Its case in `PackSlot` (≤ 8 floats in rows 1–2).
3. Its branch in the shape distance HLSL (`AI/Python/Material/zone_layer_hlsl.py`); rebuild.
Signed distance, negative inside, plus the distance from the shape's core and the core's distance to the outline (radius, half-width), which the fill shade grows across.
Colour bits: the HLSL reads 16 (`COLOR_BITS`); `static_assert` in `GeoZoneShape.cpp` holds `EGeoColor` under it.

## Layer actor
- Spawned by the subsystem on its first tick in a rendering world, editor worlds included; one per world, transient (never saved with a level, not in the outliner, planes not selectable).
- Both planes fixed at the origin, 10 km wide: past any arena, whatever the view or projection. The materials read world XY, so the plane's extent changes nothing but coverage; cost is per covered pixel.
- A plane stays hidden while its material is missing from Game Data Settings (ensure).
