"""Class badge idles: the badge breathes, and every so often plays with its floating parts as if bored.

    Square    a mandible slides clear and twirls a full turn; both drum against each other; the other one twirls
    Triangle  the needle slides out over the body and twirls; later leans out and looks one way, then the other; the
              plug under it taps the front rim of its hole twice, then the back rim
    Circle    the hourglass hops out of its bite and flips over; later rolls out along the disc's rim and back

The breath is the root's own X and Y scale, so it reaches the parts too and a part clear of the body stays clear; it
also keeps playing under a top-slot montage. A part only turns where the turn cannot sweep it into anything round it,
and every turn is a whole one, so the loop closes on the pose it opened on. Every key eases to a stop.

One sequence per badge feeds both layers of its AnimBlueprint (AI/Python/Anim/class_badge_anim_blueprints.py), each
layer taking its own bones from it.

Run AFTER AI/Python/Mesh/rig_class_badges.py, via mcp-unreal execute_script. Re-runnable: rewrites each sequence in
place. Report written to Saved/class_badge_idle.txt.
"""
import math

import unreal

APE = unreal.AnimPoseExtensions

MESH_FOLDER = "/Game/Characters/Meshes/Class"
ANIM_FOLDER = "/Game/Characters/Anim/ClassBadge"
GENERATOR = "AI/Python/Mesh/generate_class_badge_meshes.py"
REPORT = unreal.Paths.project_saved_dir() + "class_badge_idle.txt"

ROOT, BODY = "Root", "Bottom"
FPS = 30


def smooth(alpha):
    return alpha * alpha * (3.0 - 2.0 * alpha)


def accelerate(alpha):
    return alpha * alpha


def decelerate(alpha):
    return 1.0 - (1.0 - alpha) ** 2


def snap(alpha):
    return alpha


def pose(out=0.0, side=0.0, yaw=0.0, roll=0.0, pitch=0.0, orbit=0.0):
    """A part's offset from rest: forward, sideways, its own turn in degrees, and degrees round the body's centre."""
    return out, side, yaw, roll, pitch, orbit


REST = pose()


def twirl(first, out, side, turn):
    """Slides clear by (out, side), turns `turn` degrees in place, slides home; the turn is dropped once home."""
    return [(first, REST, smooth),
            (first + 12, pose(out, side), smooth),
            (first + 36, pose(out, side, yaw=turn), smooth),
            (first + 48, pose(yaw=turn), smooth),
            (first + 49, REST, snap)]   # the same rotation, a whole turn round


def tap(first, reach):
    """Slides `reach` forward onto whatever stops it, twice, then back home."""
    return [(first, REST, smooth),
            (first + 10, pose(reach), accelerate),
            (first + 15, pose(reach * 0.7), decelerate),
            (first + 19, pose(reach), accelerate),
            (first + 34, REST, smooth)]


def drum(first, inward, taps):
    """Taps `taps` times toward the other mandible, `inward` units across."""
    keys = [(first, REST, smooth)]
    for tap in range(taps):
        keys += [(first + tap * 9 + 4, pose(side=inward), accelerate),
                 (first + tap * 9 + 9, REST, decelerate)]
    return keys


def look(first, out, angle):
    """Leans out by `out`, turns to look one way, then the other, then home."""
    return [(first, REST, smooth),
            (first + 10, pose(out), smooth),
            (first + 26, pose(out, yaw=angle), smooth),
            (first + 44, pose(out, yaw=-angle), smooth),
            (first + 56, pose(out, yaw=angle * 0.4), smooth),
            (first + 64, pose(out), smooth),
            (first + 76, REST, smooth)]


def flip(first, out):
    """Hops out by `out`, flips head over heels, drops back in."""
    return [(first, REST, smooth),
            (first + 10, pose(out), decelerate),
            (first + 30, pose(out, pitch=360.0), smooth),
            (first + 40, pose(pitch=360.0), accelerate),
            (first + 41, REST, snap)]


def rim_roll(first, out, orbit, roll):
    """Moves out by `out`, rolls `orbit` degrees round the body turning `roll` about itself, waits, rolls back."""
    return [(first, REST, smooth),
            (first + 14, pose(out), smooth),
            (first + 44, pose(out, roll=roll, orbit=orbit), smooth),
            (first + 60, pose(out, roll=roll, orbit=orbit), smooth),
            (first + 90, pose(out), smooth),
            (first + 104, REST, smooth)]


# Per badge:
#   frames, breaths   the clip, and the whole breaths in it
#   breath            (fraction of a breath, breath) keys: in, held, out sagging below rest, back to rest
#   root              X and Y scale gained at a full breath
#   orbit_body        parts orbit round the body's own centre rather than the badge's
#   parts             bone -> [(frame, pose, easing into this key)]; before the first key and after the last, rest
#   lifted            parts standing clear above the body, which the top-down clearance checks leave out
BADGES = {
    "Square": {
        "frames": 300, "breaths": 3,
        "breath": [(0.0, 0.0), (0.45, 1.0), (0.57, 1.0), (0.77, -0.3), (1.0, 0.0)],
        "root": (0.02, 0.035),
        "orbit_body": False,
        "parts": {"MandibleRight": twirl(20, 6.0, 2.0, 360.0) + drum(130, -3.8, 3)[1:],
                  "MandibleLeft": drum(130, 3.8, 3) + twirl(210, 6.0, -2.0, -360.0)[1:]},
    },
    "Triangle": {
        "frames": 240, "breaths": 3,
        "breath": [(0.0, 0.0), (0.36, 1.0), (0.5, 1.0), (0.66, -0.2), (1.0, 0.0)],
        "root": (0.03, 0.025),
        "orbit_body": False,
        # The needle stands clear over the body, free to turn; the plug stands in its hole, 10.85 laying it on either rim.
        "parts": {"Needle": twirl(20, 8.0, 0.0, 360.0) + look(130, 12.0, 30.0)[1:],
                  "Plug": tap(85, 10.85) + tap(200, -10.85)[1:]},
        "lifted": ("Needle",),
    },
    "Circle": {
        "frames": 270, "breaths": 3,
        "breath": [(0.0, 0.0), (0.5, 1.0), (0.58, 1.0), (0.82, -0.2), (1.0, 0.0)],
        "root": (0.025, 0.04),
        "orbit_body": True,
        "parts": {"Hourglass": flip(20, 12.0) + rim_roll(140, 25.0, 55.0, 180.0)[1:]},
    },
}

LOG = []


def breath(setup, frame):
    period = setup["frames"] / float(setup["breaths"])
    phase = (frame % period) / period
    for (before, start), (after, end) in zip(setup["breath"], setup["breath"][1:]):
        if before <= phase <= after:
            return start + (end - start) * smooth((phase - before) / (after - before))
    return 0.0


def part_pose(keys, frame):
    if frame <= keys[0][0]:
        return keys[0][1]
    for (before, start, _), (after, end, ease) in zip(keys, keys[1:]):
        if before <= frame <= after:
            alpha = ease((frame - before) / float(after - before))
            return tuple(a + (b - a) * alpha for a, b in zip(start, end))
    return keys[-1][1]


def key(setup, pivot, frame, bone, rest_local):
    translation, rotation, scale = rest_local.translation, rest_local.rotation, rest_local.scale3d
    if bone == ROOT:
        full = breath(setup, frame)
        root_x, root_y = setup["root"]
        scale = unreal.Vector(scale.x * (1.0 + root_x * full), scale.y * (1.0 + root_y * full), scale.z)
    elif bone in setup["parts"]:
        out, side, yaw, roll, pitch, orbit = part_pose(setup["parts"][bone], frame)
        # Parts hang off Top, which sits on the badge's axis unturned, so their X and Y are the badge's.
        x, y = translation.x - pivot[0] + out, translation.y - pivot[1] + side
        cosine, sine = math.cos(math.radians(orbit)), math.sin(math.radians(orbit))
        translation = unreal.Vector(pivot[0] + x * cosine - y * sine, pivot[1] + x * sine + y * cosine, translation.z)
        rotation = unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw + orbit).quaternion()
    return translation, rotation, scale


def body_outline(badge):
    """The badge body's world outline, from the generator that built it."""
    path = unreal.Paths.project_dir() + GENERATOR
    generator = {"__name__": "badge_generator"}
    exec(compile(open(path).read(), path, "exec"), generator)
    return generator["to_world"]("SM_{}Badge".format(badge))[0].outline


def orbit_pivot(setup, outline):
    """The body's own centre for a round body — its back edge plus its half width — else the badge's axis."""
    if not setup["orbit_body"]:
        return 0.0, 0.0
    xs, ys = [p[0] for p in outline], [p[1] for p in outline]
    return min(xs) + (max(ys) - min(ys)) * 0.5, (max(ys) + min(ys)) * 0.5


def report(toolkit, badge, setup, sequence, outline):
    """Every frame: how close each part comes to the body and to the other parts, and how fast any part turns."""
    skeleton_path = "{}/SK_{}Badge".format(MESH_FOLDER, badge)
    groups = toolkit["rigid_vertex_groups"]("{}/SKM_{}Badge".format(MESH_FOLDER, badge), skeleton_path)
    reference = toolkit["_component_transforms"](APE.get_reference_pose(unreal.load_asset(skeleton_path)))
    parts = [part for part in setup["parts"] if part not in setup.get("lifted", ())]
    options = unreal.AnimPoseEvaluationOptions()

    LOG.append("{} - {} keys for {} frames ({:.1f} s)".format(
        sequence.get_name(), toolkit["playable_key_count"](sequence), setup["frames"], setup["frames"] / float(FPS)))
    closest_body, closest_parts, tight, firsts = (float("inf"), -1), (float("inf"), -1), [], None
    for frame in range(setup["frames"] + 1):
        pose_now = APE.get_anim_pose_at_time(sequence, frame / float(FPS), options)
        table = toolkit["local_pose_table"](pose_now)
        posed = toolkit["_component_transforms"](pose_now)
        placed = toolkit["place_groups"](groups, posed)
        hulls = {part: toolkit["convex_hull"](placed[part]) for part in parts}
        body = toolkit["move_outline"](outline, reference[BODY], posed[BODY])
        to_body = min(toolkit["outline_separation"](hull, body) for hull in hulls.values())
        between = min([toolkit["outline_separation"](hulls[a], hulls[b])
                       for index, a in enumerate(parts) for b in parts[index + 1:]] or [float("inf")])
        closest_body = min(closest_body, (to_body, frame))
        closest_parts = min(closest_parts, (between, frame))
        if min(to_body, between) < 1.0:
            tight.append(frame)
        if frame == 0:
            firsts = table

    # A whole turn dropped in one frame is the same rotation, not a turn.
    steps = [max(abs(a - b) for a, b in zip(part_pose(keys, frame)[2:5], part_pose(keys, frame - 1)[2:5]))
             for keys in setup["parts"].values() for frame in range(1, setup["frames"] + 1)]
    turns = [step for step in steps if step < 180.0]
    worst = max(abs(a - b) for bone in firsts for part in (0, 2)
                for a, b in zip(firsts[bone][part], table[bone][part]))
    LOG.append("closest a part comes to the body %.2f (frame %s), to another part %.2f (frame %s) (-1 = overlapping)"
               % (closest_body + closest_parts))
    LOG.append("frames closer than 1 unit: %s" % (tight or "none"))
    LOG.append("fastest turn %.1f deg/frame" % max(turns))
    LOG.append("loop: largest jump from last frame to first %.4f" % worst)
    LOG.append("")


try:
    toolkit_path = unreal.Paths.project_dir() + "AI/Python/Anim/anim_sequence_authoring.py"
    toolkit = {}
    exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)

    for badge_name, badge_setup in BADGES.items():
        skeleton = "{}/SK_{}Badge".format(MESH_FOLDER, badge_name)
        badge_outline = body_outline(badge_name)
        badge_pivot = orbit_pivot(badge_setup, badge_outline)
        factory = unreal.AnimSequenceFactory()
        factory.set_editor_property("target_skeleton", unreal.load_asset(skeleton))
        idle = toolkit["get_or_create_asset"]("{}/{}".format(ANIM_FOLDER, badge_name),
                                              "SK_{}Badge_Sequence_Idle".format(badge_name), unreal.AnimSequence,
                                              factory)
        toolkit["write_bone_tracks"](
            idle, skeleton, FPS, badge_setup["frames"],
            lambda frame, bone, rest_local, setup=badge_setup, pivot=badge_pivot:
                key(setup, pivot, frame, bone, rest_local),
            "Build {} badge idle".format(badge_name))
        report(toolkit, badge_name, badge_setup, idle, badge_outline)
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
