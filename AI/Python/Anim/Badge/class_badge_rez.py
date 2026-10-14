"""Rez montages for the three class badges, 3 s, replacing the death's empty pose:

    0.0-0.5 s   the badge grows back from nothing spinning two whole turns, fast at first and bleeding off to a stop,
                overshooting its size and settling
    0.5-2.7 s   a show-off of everything it can do, while the body rolls over about its aim:
                  a part on its own level shoots straight out and orbits the body fast on a rosette, swinging out and
                  back in rather than on a circle, spinning on itself; landing, it crosses its rest and settles
                  a part going under sinks straight down beside the body first, then orbits the other way far below
                  it, its radius swinging through the centre, so it passes under the body and out the far side
    2.7-3.0 s   rest, under the blend out to the idle

    Square    one mandible orbits level, the other the other way underneath
    Triangle  the needle orbits over the body, the plug drops through its hole and orbits the other way underneath
    Circle    the hourglass whips round, snaps into reverse halfway, and tumbles end over end as it goes

A level part orbits the body's centre no closer than the body's reach — measured in three dimensions about the roll's
axis, since the roll swings the body's height into the view plane — plus its own. A part going under sinks below that
same reach, so its radius is free, and it sinks and rises at its rest, where nothing is under or over it. Every turn is
a whole one over a shared normalised orbit, so each part is back on its rest angle and the body flat when the orbit
ends.

The spin is the root's yaw about the actor origin, the growth its X and Y scale; the roll turns the body bone about
the badge's mid-plane. The montage plays in DefaultSlot, over both layers of the badge AnimBlueprint, and starts the
revive sound on frame 0.

Run AFTER AI/Python/Mesh/rig_class_badges.py, via mcp-unreal execute_script. Re-runnable: rewrites every sequence and
montage in place. Report written to AI/Output/class_badge_rez.txt.
"""
import math

import unreal

APE = unreal.AnimPoseExtensions

MESH_FOLDER = "/Game/Characters/Meshes/Class"
ANIM_FOLDER = "/Game/Characters/Anim/ClassBadge"
GENERATOR = "AI/Python/Mesh/generate_class_badge_meshes.py"
REPORT = (unreal.Paths.project_dir() + "AI/Output/") + "class_badge_rez.txt"

ROOT, BODY, TOP = "Root", "Bottom", "Top"
SLOT = "DefaultSlot"
SECTION = "Default"
SOUND_TRACK = "1"
SOUND = "/Game/Art/SFX/Bank/magicspellssfx1/Heal/Heal_9"
BLEND_OUT_TIME = 0.25

FPS = 30
FRAMES = 90
SPIN_END = 15        # the grow and spin; parts leave from here
ORBIT_START = 22
ORBIT_END = 72
HOME = 80            # every part back on rest
LAND_END = 86

SPIN_TURNS = 2
# (frame, scale) keys, smoothstepped between: grow past full size, swing short, settle
GROW_KEYS = [(0, 0.0), (8, 1.1), (12, 0.97), (15, 1.0)]

RAMP_UP = 3          # frames the orbit takes to reach full speed
RAMP_DOWN = 10       # frames it takes to bleed off before the parts come home
MARGIN = 4.0         # units kept between a part and the rolling body
LANDING = 2.0        # units a level part crosses its rest by, landing
# (frame after landing, share of LANDING toward the body), smoothstepped between
LAND_KEYS = [(0, 0.0), (2, 1.0), (4, -0.35), (LAND_END - HOME, 0.0)]


def part(legs, spin, waves, swing=20.0, under=False, tumble=0):
    """A floating part's show-off.

    legs    whole turns round the body, anticlockwise seen from above, one leg after another over equal shares of
            the orbit, so a sign change snaps it into reverse
    spin    whole turns on itself on top of following the orbit round
    waves   times its radius swings over the orbit, by `swing` units past the orbit radius
    under   sinks below the body and swings its radius through the centre rather than staying outside it
    tumble  whole turns end over end
    """
    return {"legs": legs, "spin": spin, "waves": waves, "swing": swing, "under": under, "tumble": tumble}


# Per badge: its parts, whole turns the body rolls over, and parts standing above the body which the top-down
# clearance report leaves out along with those going under.
BADGES = {
    "Square": {"parts": {"MandibleLeft": part([4], 5, 3),
                         "MandibleRight": part([-3], -5, 2, under=True)},
               "roll": 2},
    "Triangle": {"parts": {"Needle": part([4], 6, 3),
                           "Plug": part([-4], -4, 2, under=True)},
                 "roll": 2, "lifted": ("Needle",)},
    "Circle": {"parts": {"Hourglass": part([3, -4], 4, 4, swing=35.0, tumble=3)},
               "roll": 3},
}

LOG = []


def smooth(alpha):
    return alpha * alpha * (3.0 - 2.0 * alpha)


def accelerate(alpha):
    return alpha * alpha


def decelerate(alpha):
    return 1.0 - (1.0 - alpha) ** 2


def between(value, start, end):
    """How far `value` is from `start` to `end`, held at 0 before and 1 after."""
    return min(1.0, max(0.0, (value - start) / float(end - start)))


def keyed(keys, frame):
    for (start, low), (end, high) in zip(keys, keys[1:]):
        if frame <= end:
            return low + (high - low) * smooth(between(frame, start, end))
    return keys[-1][1]


def spin_yaw(frame):
    return 360.0 * SPIN_TURNS * decelerate(between(frame, 0, SPIN_END))


def orbit_phase(frame):
    """The share of the orbit done by `frame`: full speed within a few frames, bleeding off before the end."""
    length = ORBIT_END - ORBIT_START
    phase = toolkit["normalised_spin"](
        lambda step: min(1.0, step / float(RAMP_UP), (length - step + 1) / float(RAMP_DOWN)), length)
    return phase[min(length, max(0, frame - ORBIT_START))]


def orbit_turns(legs, phase):
    share = 1.0 / len(legs)
    return sum(turns * between(phase, index * share, (index + 1) * share) for index, turns in enumerate(legs))


def reaches(badge):
    """The orbit's centre in Top's space, each part's orbit radius and the depth in Top's space a part going under
    orbits at -> ((x, y), {part: radius}, {part: depth}).

    A tumbling part or one going under is measured in three dimensions, one only turning flat across the view plane.
    """
    skeleton_path = "{}/SK_{}Badge".format(MESH_FOLDER, badge)
    groups = toolkit["rigid_vertex_groups"]("{}/SKM_{}Badge".format(MESH_FOLDER, badge), skeleton_path)
    reference = toolkit["_component_transforms"](APE.get_reference_pose(unreal.load_asset(skeleton_path)))
    body = toolkit["place_groups"](groups, reference)[BODY]
    xs, ys = [vertex.x for vertex in body], [vertex.y for vertex in body]
    centre_x, centre_y = (min(xs) + max(xs)) * 0.5, (min(ys) + max(ys)) * 0.5
    # The roll turns about the X axis, so it carries every body vertex round that axis at its own distance from it.
    roll_reach = max(math.hypot(vertex.y, vertex.z) for vertex in body)
    body_reach = (max(math.hypot(vertex.x - centre_x, math.hypot(vertex.y, vertex.z)) for vertex in body)
                  + abs(centre_y))
    top = reference[TOP].translation
    radii, depths = {}, {}
    for name, spec in BADGES[badge]["parts"].items():
        solid = spec["under"] or spec["tumble"]
        own = max(v.length() if solid else math.hypot(v.x, v.y) for v in groups[name])
        radii[name] = body_reach + own + MARGIN
        depths[name] = -(roll_reach + own + MARGIN) - top.z
    return (centre_x - top.x, centre_y - top.y), radii, depths


def part_pose(spec, centre, radius, depth, rest, frame):
    """Where a part stands on `frame` in Top's space, and its turns -> (x, y, z, yaw, pitch)."""
    rest_x, rest_y = rest[0] - centre[0], rest[1] - centre[1]
    rest_radius, rest_angle = math.hypot(rest_x, rest_y), math.atan2(rest_y, rest_x)
    phase = orbit_phase(frame)
    turns = orbit_turns(spec["legs"], phase)
    wave = 2.0 * math.pi * spec["waves"] * phase
    z = rest[2]
    if spec["under"]:
        sunk = smooth(between(frame, SPIN_END, ORBIT_START)) - smooth(between(frame, ORBIT_END, HOME))
        z += (depth - rest[2]) * sunk
        # Negative past the centre, which puts the part on the far side: the swing crosses under the body.
        distance = rest_radius * math.cos(wave) + (radius + spec["swing"]) * math.sin(wave)
    else:
        out = decelerate(between(frame, SPIN_END, ORBIT_START)) - accelerate(between(frame, ORBIT_END, HOME))
        landing = LANDING * keyed(LAND_KEYS, frame - HOME) if frame > HOME else 0.0
        distance = (rest_radius + (radius - rest_radius) * out + spec["swing"] * (0.5 - 0.5 * math.cos(wave))
                    - landing)
    angle = rest_angle + 2.0 * math.pi * turns
    return (centre[0] + distance * math.cos(angle), centre[1] + distance * math.sin(angle), z,
            360.0 * (turns + spec["spin"] * phase), 360.0 * spec["tumble"] * phase)


def key(badge, geometry, frame, bone, rest_local):
    translation, rotation, scale = rest_local.translation, rest_local.rotation, rest_local.scale3d
    setup = BADGES[badge]
    centre, radii, depths = geometry
    if bone == ROOT:
        size = keyed(GROW_KEYS, frame)
        scale = unreal.Vector(scale.x * size, scale.y * size, scale.z)
        rotation = unreal.Rotator(yaw=spin_yaw(frame)).quaternion()
    elif bone == BODY:
        rotation = unreal.Rotator(roll=360.0 * setup["roll"] * orbit_phase(frame)).quaternion()
    elif bone in setup["parts"]:
        rest = (translation.x, translation.y, translation.z)
        x, y, z, yaw, pitch = part_pose(setup["parts"][bone], centre, radii[bone], depths[bone], rest, frame)
        translation = unreal.Vector(x, y, z)
        rotation = unreal.Rotator(yaw=yaw, pitch=pitch).quaternion()
    return translation, rotation, scale


def build(badge, geometry):
    skeleton_path = "{}/SK_{}Badge".format(MESH_FOLDER, badge)
    package = "{}/{}".format(ANIM_FOLDER, badge)
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(skeleton_path))
    sequence = toolkit["get_or_create_asset"](package, "SK_{}Badge_Sequence_Rez".format(badge), unreal.AnimSequence,
                                              factory)
    toolkit["write_bone_tracks"](sequence, skeleton_path, FPS, FRAMES,
                                 lambda frame, bone, rest_local: key(badge, geometry, frame, bone, rest_local),
                                 "Build {} badge rez".format(badge))

    montage = toolkit["build_montage"](sequence, package, "SK_{}Badge_Montage_Rez".format(badge), [SECTION], [0.0],
                                       ["None"], SLOT)
    # Blends in at once, so the growth is not smeared into the death's empty pose.
    for name, time in (("blend_in", 0.0), ("blend_out", BLEND_OUT_TIME)):
        blend = montage.get_editor_property(name)
        blend.set_editor_property("blend_time", time)
        blend.set_editor_property("blend_option", unreal.AlphaBlendOption.HERMITE_CUBIC)
        montage.set_editor_property(name, blend)
    montage.set_editor_property("enable_auto_blend_out", True)
    toolkit["set_notify"](montage, SOUND_TRACK, 0.0, unreal.AnimNotify_PlaySound,
                          {"sound": unreal.load_asset(SOUND), "volume_multiplier": 1.0})
    unreal.EditorAssetLibrary.save_asset(montage.get_outermost().get_name(), only_if_is_dirty=False)
    return montage, sequence


def report(badge, geometry, montage, sequence):
    """Every frame: how close each level part in view comes to the body; the orbit's shape and turn speeds."""
    setup = BADGES[badge]
    centre, radii, depths = geometry
    skeleton_path = "{}/SK_{}Badge".format(MESH_FOLDER, badge)
    groups = toolkit["rigid_vertex_groups"]("{}/SKM_{}Badge".format(MESH_FOLDER, badge), skeleton_path)
    reference = toolkit["_component_transforms"](APE.get_reference_pose(unreal.load_asset(skeleton_path)))
    generator = {"__name__": "badge_generator"}
    exec(compile(open(unreal.Paths.project_dir() + GENERATOR).read(), GENERATOR, "exec"), generator)
    outline = generator["to_world"]("SM_{}Badge".format(badge))[0].outline
    shown = [name for name, spec in setup["parts"].items()
             if not spec["under"] and name not in setup.get("lifted", ())]
    options = unreal.AnimPoseEvaluationOptions()

    LOG.append("{}: {} keys for {} frames, sections {}, notifies {}".format(
        montage.get_name(), toolkit["playable_key_count"](sequence), FRAMES, toolkit["montage_sections"](montage),
        [(round(time, 3), notify.get_class().get_name()) for time, notify in toolkit["notify_events"](montage)]))
    closest = {}
    for frame in range(SPIN_END, FRAMES + 1):
        posed = toolkit["_component_transforms"](APE.get_anim_pose_at_time(sequence, frame / float(FPS), options))
        placed = toolkit["place_groups"](groups, posed)
        body = toolkit["move_outline"](outline, reference[BODY], posed[BODY])
        for name in shown:
            gap = toolkit["outline_separation"](toolkit["convex_hull"](placed[name]), body)
            closest[name] = min(closest.get(name, (float("inf"), -1)), (gap, frame))
    for name, (gap, frame) in closest.items():
        LOG.append("  {} in view: closest to the body {:.2f} on frame {} (-1 = overlapping)".format(name, gap, frame))
    LOG.append("  largest offset from the reference pose on the last frame: {:.4f}".format(max(
        toolkit["_delta"](toolkit["_snapshot"](posed[bone]), toolkit["_snapshot"](reference[bone])) for bone in posed)))

    for name, spec in setup["parts"].items():
        phases = [orbit_phase(frame) for frame in range(FRAMES + 1)]
        orbit = [360.0 * orbit_turns(spec["legs"], phase) for phase in phases]
        yaw = [angle + 360.0 * spec["spin"] * phase for angle, phase in zip(orbit, phases)]
        LOG.append("  {}: {} at radius {:.1f}{}, fastest orbit {:.1f} deg/frame, fastest spin {:.1f}".format(
            name, "under at depth {:.1f}".format(depths[name]) if spec["under"] else "level", radii[name],
            " + {:.0f}".format(spec["swing"]), max(abs(b - a) for a, b in zip(orbit, orbit[1:])),
            max(abs(b - a) for a, b in zip(yaw, yaw[1:]))))
    LOG.append("  spin's first frame {:.1f} deg, fastest body roll {:.1f} deg/frame".format(
        spin_yaw(1), 360.0 * setup["roll"] * max(orbit_phase(f) - orbit_phase(f - 1) for f in range(1, FRAMES + 1))))


try:
    toolkit_path = unreal.Paths.project_dir() + "AI/Python/Anim/anim_sequence_authoring.py"
    toolkit = {}
    exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)
    for badge_name in BADGES:
        badge_geometry = reaches(badge_name)
        rez_montage, rez_sequence = build(badge_name, badge_geometry)
        report(badge_name, badge_geometry, rez_montage, rez_sequence)
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
