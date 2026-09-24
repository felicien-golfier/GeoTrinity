"""Places every arena wall of the draft map on the outline of the floor it borders, in both barrier states.

The rules are in AI/ArenaWalls.md. Every wall gets WALL_SCALE_Y, WALL_SCALE_Z and WALL_Z; only its XY, its yaw and
its X scale are placed. A barrier state stored at zero scale is a hidden wall and is left exactly as it is.

Layout, world XY:
- Entrance: a rectangle. Its +X side is Cube21 | Door_main | Cube22; Door_main closes it while the star fights.
- Star: two equal squares, one turned 45 degrees. Its -X tip is the StarBarrier doors Door_left/right, its +X tip
  DoorToHex1/2. Opened, Door_left/right become the corridor to the entrance and DoorToHex1/2 hide.
- Corridors: each runs from a star notch, on the line the notch's own walls already end on, so the star's square
  walls meet both the closed tip and the open corridor.
- Hex: a regular hexagon HEX_WALL_APOTHEM round the hex arena, one BP_HexBarriere wall per side. Opened, all but
  HexBorder2/6 hide and those two become the corridor from the star's +X tip, ending with the corridor floor.
- Tutorial: a rectangle.

Barrier walls are left in their fight-off state, where BeginPlay puts them. The level is left dirty, never saved.
Run through execute_script, outside PIE. Report written to Saved/arena_walls.txt.
"""
import math
import traceback

import unreal

WALL_SCALE_Y = 0.7
WALL_SCALE_Z = 2.0
WALL_Z = 100.0
HALF_WIDTH = WALL_SCALE_Y * 50.0  # the engine cube spans -50..50 cm
HALF_FLOOR = 500.0  # Floor_Mesh spans -500..500 cm, pivot centred
HEX_WALL_APOTHEM = 1950.0  # hex arena centre to the walls' inner face
HALO = "/Game/Art/VFX/Arena/MI_ArenaRailHalo"
REPORT = unreal.Paths.project_saved_dir() + "arena_walls.txt"
LOG = []


def outward(start, end):
    """Unit normal of edge start->end pointing out of a counter-clockwise outline."""
    dx, dy = end[0] - start[0], end[1] - start[1]
    length = math.hypot(dx, dy)
    return dy / length, -dx / length


def offset_corner(outline, vertex):
    """Where the centerlines of the two walls meeting at vertex cross, HALF_WIDTH outside the outline."""
    index = outline.index(vertex)
    before = outward(outline[index - 1], vertex)
    after = outward(vertex, outline[(index + 1) % len(outline)])
    miter = HALF_WIDTH / (1.0 + before[0] * after[0] + before[1] * after[1])
    return vertex[0] + (before[0] + after[0]) * miter, vertex[1] + (before[1] + after[1]) * miter


def rectangle(floor):
    """Counter-clockwise corners of a yaw-0 floor actor."""
    component = floor.static_mesh_component
    centre, scale = component.get_world_location(), component.get_world_scale()
    x0, x1 = centre.x - scale.x * HALF_FLOOR, centre.x + scale.x * HALF_FLOOR
    y0, y1 = centre.y - scale.y * HALF_FLOOR, centre.y + scale.y * HALF_FLOOR
    return [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]


def wall_transform(start, end, reference_yaw, end_inset):
    """A wall whose rail line runs start..end, facing whichever way along it is nearer reference_yaw."""
    yaw = math.degrees(math.atan2(end[1] - start[1], end[0] - start[0]))
    if abs((yaw - reference_yaw + 180.0) % 360.0 - 180.0) > 90.0:
        yaw = yaw - 180.0 if yaw > 0.0 else yaw + 180.0

    length = math.hypot(end[0] - start[0], end[1] - start[1]) + 2.0 * end_inset * HALF_WIDTH
    transform = unreal.Transform()
    transform.translation = unreal.Vector((start[0] + end[0]) / 2.0, (start[1] + end[1]) / 2.0, WALL_Z)
    transform.rotation = unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw).quaternion()
    transform.scale3d = unreal.Vector(length / 100.0, WALL_SCALE_Y, WALL_SCALE_Z)
    return transform


def describe(transform):
    location, scale = transform.translation, transform.scale3d
    return "loc (%.1f, %.1f, %.1f) yaw %.1f scale (%.4f, %.3f, %.3f)" % (
        location.x, location.y, location.z, transform.rotation.rotator().yaw, scale.x, scale.y, scale.z)


def is_hidden(transform):
    scale = transform.scale3d
    return scale.x == 0.0 and scale.y == 0.0 and scale.z == 0.0


def layout(actors):
    """Rail lines per wall: {label: (start, end)} for static walls, and {label: {"on"|"off": (start, end)}}."""
    static, barrier = {}, {}

    star_floor = actors["Floor3"].static_mesh_component  # Floor5 is the same square turned 45 degrees
    centre = star_floor.get_world_location()
    half = star_floor.get_world_scale().x * HALF_FLOOR
    tip, notch = half * math.sqrt(2.0), half * math.sqrt(2.0) - half

    def star(dx, dy):
        return centre.x + dx, centre.y + dy

    star_points = [star(sx * half, sy * half) for sx in (-1, 1) for sy in (-1, 1)]
    star_points += [star(tip, 0.0), star(-tip, 0.0), star(0.0, tip), star(0.0, -tip)]
    star_points += [star(sx * half, sy * notch) for sx in (-1, 1) for sy in (-1, 1)]
    star_points += [star(sx * notch, sy * half) for sx in (-1, 1) for sy in (-1, 1)]
    star_outline = sorted(star_points, key=lambda p: math.atan2(p[1] - centre.y, p[0] - centre.x))

    def star_edge(first, second):
        return offset_corner(star_outline, star(*first)), offset_corner(star_outline, star(*second))

    static.update({
        "Cube5": star_edge((notch, -half), (half, -half)),
        "Cube6": star_edge((-half, -half), (-notch, -half)),
        "Cube11": star_edge((-notch, half), (-half, half)),
        "Cube12": star_edge((half, half), (notch, half)),
        "Cube13": star_edge((half, -half), (half, -notch)),
        "Cube14": star_edge((half, notch), (half, half)),
        "Cube19": star_edge((-half, -notch), (-half, -half)),
        "Cube20": star_edge((-half, half), (-half, notch)),
        "Cube7": star_edge((0.0, -tip), (-notch, -half)),
        "Cube8": star_edge((notch, -half), (0.0, -tip)),
        "Cube9": star_edge((-notch, half), (0.0, tip)),
        "Cube10": star_edge((0.0, tip), (notch, half)),
    })

    # The corridors run on the lines the star's square walls end on at the notches.
    west_low = offset_corner(star_outline, star(-half, -notch))
    west_high = offset_corner(star_outline, star(-half, notch))
    east_low = offset_corner(star_outline, star(half, -notch))
    east_high = offset_corner(star_outline, star(half, notch))

    entrance = rectangle(actors["Floor"])
    corner = {vertex: offset_corner(entrance, vertex) for vertex in entrance}
    entrance_line_x = corner[entrance[1]][0]
    static.update({
        "Cube": (corner[entrance[3]], corner[entrance[2]]),
        "Cube2": (corner[entrance[0]], corner[entrance[1]]),
        "Cube4": (corner[entrance[0]], corner[entrance[3]]),
        "Cube21": (corner[entrance[1]], (entrance_line_x, west_low[1])),
        "Cube22": ((entrance_line_x, west_high[1]), corner[entrance[2]]),
    })

    hex_corridor_end = rectangle(actors["HexRoomFloor2"])[1][0] - HALF_WIDTH  # the wall ends with the floor
    barrier.update({
        "Door_main": {"on": ((entrance_line_x, west_low[1]), (entrance_line_x, west_high[1]))},
        "Door_left": {"on": star_edge((-half, -notch), (-tip, 0.0)), "off": ((entrance_line_x, west_low[1]), west_low)},
        "Door_right": {"on": star_edge((-tip, 0.0), (-half, notch)), "off": ((entrance_line_x, west_high[1]), west_high)},
        "DoorToHex2": {"on": star_edge((half, -notch), (tip, 0.0))},
        "DoorToHex1": {"on": star_edge((tip, 0.0), (half, notch))},
        "HexBorder2": {"off": (east_low, (hex_corridor_end, east_low[1]))},
        "HexBorder6": {"off": (east_high, (hex_corridor_end, east_high[1]))},
    })

    hex_centre = actors["HexArena"].get_actor_location()
    hex_radius = HEX_WALL_APOTHEM * 2.0 / math.sqrt(3.0)
    hex_outline = [(hex_centre.x + hex_radius * math.cos(math.radians(30.0 + 60.0 * k)),
                    hex_centre.y + hex_radius * math.sin(math.radians(30.0 + 60.0 * k))) for k in range(6)]
    # Label -> side, counted counter-clockwise from the side facing +X.
    for label, side in (("HexBorder4", 0), ("HexBorder5", 1), ("HexBorder6", 2),
                        ("HexBorder1", 3), ("HexBorder2", 4), ("HexBorder3", 5)):
        line = (offset_corner(hex_outline, hex_outline[side - 1]), offset_corner(hex_outline, hex_outline[side]))
        barrier.setdefault(label, {})["on"] = line

    tutorial = rectangle(actors["Tuto_Floor"])
    corner = {vertex: offset_corner(tutorial, vertex) for vertex in tutorial}
    static.update({
        "Tuto_Wall_S": (corner[tutorial[0]], corner[tutorial[1]]),
        "Tuto_Wall_E": (corner[tutorial[1]], corner[tutorial[2]]),
        "Tuto_Wall_N": (corner[tutorial[3]], corner[tutorial[2]]),
        "Tuto_Wall_W": (corner[tutorial[0]], corner[tutorial[3]]),
    })
    return static, barrier


def place_barrier(barrier_actor, lines, end_inset):
    """Rewrite the barrier's stored states from lines, keep hidden ones, and leave its walls in fight-off."""
    entries = []
    for stored in barrier_actor.get_editor_property("animated_actors"):
        entry = stored.copy()
        label = entry.get_editor_property("actor").get_actor_label()
        for state, field in (("on", "fight_on_transform"), ("off", "fight_off_transform")):
            current = entry.get_editor_property(field)
            if is_hidden(current):
                LOG.append("%s %s: hidden, kept" % (label, state))
            elif state in lines.get(label, {}):
                start, end = lines[label][state]
                placed = wall_transform(start, end, current.rotation.rotator().yaw, end_inset)
                LOG.append("%s %s: %s -> %s" % (label, state, describe(current), describe(placed)))
                entry.set_editor_property(field, placed)
            else:
                LOG.append("%s %s: not a wall, kept" % (label, state))

        entries.append(entry)

    barrier_actor.modify()
    barrier_actor.set_editor_property("animated_actors", entries)
    for entry in entries:
        wall = entry.get_editor_property("actor")
        wall.modify()
        wall.set_actor_transform(entry.get_editor_property("fight_off_transform"), False, True)


def main():
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = {actor.get_actor_label(): actor for actor in subsystem.get_all_level_actors()}
    end_inset = unreal.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(
        unreal.load_asset(HALO), "EndInset")
    LOG.append("EndInset %.3f, half width %.1f" % (end_inset, HALF_WIDTH))
    static, barrier = layout(actors)
    with unreal.ScopedEditorTransaction("Place arena walls"):
        for label, (start, end) in static.items():
            wall = actors[label]
            placed = wall_transform(start, end, wall.get_actor_rotation().yaw, end_inset)
            LOG.append("%s: %s -> %s" % (label, describe(wall.get_actor_transform()), describe(placed)))
            wall.modify()
            wall.set_actor_transform(placed, False, True)

        for barrier_label in ("StarBarrier", "BP_HexBarriere"):
            place_barrier(actors[barrier_label], barrier, end_inset)


try:
    main()
except Exception:
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as report:
    report.write("\n".join(LOG))
