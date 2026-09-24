# Arena Walls

Every arena border is a row of engine `Cube` static mesh actors wearing `MI_ArenaRailHalo`: plain walls, and the
walls a barrier (`AGeoArenaBarrier::AnimatedActors`) moves between its fight-on and fight-off transforms.
`AI/Python/Level/arena_walls.py` places all of them from these rules — re-run it after moving a floor.

## Fixed values — every wall, every state

| Property | Value |
|---|---|
| Scale Y | **0.7** (35 cm half width) |
| Scale Z | **2.0** |
| Location Z | **100** (bottom on the floor) |

A wall is only ever placed or resized through its X/Y location, its yaw and its X scale. A barrier does the
same between its two states: it may move, turn and lengthen a wall, never thicken or raise it. The one exception
is a hidden barrier state — scale 0, parked below the floor — which keeps its own values.

## Placement

- **Inner face on the floor edge.** The wall's centerline runs half a width (35) outside the floor outline.
- **Lines meet at corners.** Two walls meeting at a floor corner end where their centerlines cross: past the
  corner on an outer corner (by `35 · tan(turn/2)`: 35 on a square corner), short of it on an inner one (the
  star's 225° notches: `35 · tan(22.5°)`).
- **Length = centerline + 2 × 35.** The rail material stops the drawn line `EndInset` (1) half widths before
  each end and rounds it off there, so a mesh that long draws a line meeting its neighbours' exactly.
- **A split side meets end to end.** Walls sharing one straight side (the entrance's Cube21 | Door_main | Cube22)
  split it at the points where the walls crossing it meet it.
- **Both barrier states obey the same rules.** A plain wall touched by a barrier wall must fit it in both
  states; the star's corridors are placed on the line its square walls already end on at the notches for that
  reason, which makes them 20 cm narrower than the notch-to-notch gap.
