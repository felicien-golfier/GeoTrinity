"""Triangle badge auto-fire: a heavy shot. The needle draws back, then drives out ahead of the arrowhead, swelling and
starting to turn about its own long axis, rattling harder and harder, stops dead, fires, and is flattened by the
recoil before it hauls itself back.

GA_Triangle_AutoProjectile fires every FireDelay and plays one section per shot, stretched to that delay, with the
shot landing on the section's last frame from anim_socket_<section index>, both of which sit on the needle:

    Start   wind up from rest                      -> End
    Fire1   recoil from the last shot, wind up again, loops on itself while firing lasts
    End     recoil from the last shot and settle   (firing stopped)

The sequence holds them as Fire1, End, Start: the one order where each begins on the pose its neighbour ends on.
Weight comes from what surrounds the shot rather than from its speed — the dead stop before it, the recoil that
flattens the needle wide, the hold in that squash, and a return slower than anything leading up to it.

A beat is authored longer than the ability's delay and sped up to it by the play rate, keeping the 30 fps the engine
samples sequences at; End runs at that same rate. Everything is keyed on the Top branch and the montage goes in the
Top slot, so the body's own layer is left alone.

Run AFTER AI/Python/Mesh/rig_class_badges.py, via mcp-unreal execute_script. Re-runnable: rewrites the sequence and
the montage in place. Report written to Saved/class_badge_triangle_fire.txt.
"""
import math

import unreal

APE = unreal.AnimPoseExtensions

SKELETON_PATH = "/Game/Characters/Meshes/Class/SK_TriangleBadge"
MESH_PATH = "/Game/Characters/Meshes/Class/SKM_TriangleBadge"
GENERATOR = "AI/Python/Mesh/generate_class_badge_meshes.py"
ANIM_PACKAGE = "/Game/Characters/Anim/ClassBadge/Triangle"
SEQUENCE_NAME = "SK_TriangleBadge_Sequence_Fire"
MONTAGE_NAME = "SK_TriangleBadge_Montage_Fire"
REPORT = unreal.Paths.project_saved_dir() + "class_badge_triangle_fire.txt"

NEEDLE = "Needle"
SLOT = "Top"

FPS = 30
BEAT = 18  # sped up to the ability's 0.3 s FireDelay
RECOIL = 5  # frames of Fire1 spent recoiling before it winds up again

REACH = 18.0     # units the needle drives ahead of the arrowhead, wound up
GROW = 2.0       # its scale wound up
TURN = 180.0     # degrees about its long axis per wind-up: a whole number of its square section's quarter turns
STROBE = 45.0    # half that section's symmetry: past this per frame a turn reads as going backwards
RATTLE = 1.6     # units it shakes sideways at the height of the wind-up
TURN_EASE = 1.6  # above 1 the turn accelerates
WIND_EASE = 2.4
STILL = 3        # frames held dead still before the shot

DRAW_BACK = -3.0  # units the needle first draws in, the wrong way
DRAW_SCALE = 0.9
DRAW_END = 3      # frame the draw-back bottoms out on

# Frames after the shot: forward offset, X scale, Y scale. Flattened wide by the kick, then held there.
KICK = [
    (REACH, GROW, GROW),
    (12.0, 1.60, 2.50),
    (7.0, 1.20, 2.90),
    (7.5, 1.28, 2.80),
    (8.0, 1.35, 2.70),
]
SETTLE_POWER = 1.5   # how fast the swing back dies
SETTLE_SPRING = 3.4  # sets where it crosses rest: once, a little past halfway home

# Per section: its name and the section it runs into when nobody fires again, in sequence order.
SECTIONS = [("Fire1", "End"), ("End", "None"), ("Start", "End")]
FRAMES = BEAT * len(SECTIONS)
FOLLOWERS = {"Start": ["Fire1", "End"], "Fire1": ["Fire1", "End"]}

LOG = []


def lerp_pose(a, b, alpha):
    return tuple(x + (y - x) * alpha for x, y in zip(a, b))


def wind(frame, first, begin):
    """A wind-up from pose `begin` on frame `first` to the wound pose, held still for the last STILL frames.

    -> (offset, X scale, Y scale, fraction of TURN done, how hard it rattles).
    """
    alpha = min(1.0, max(0.0, frame - first) / float(BEAT - STILL - first))
    wound = (REACH, GROW, GROW)
    offset, scale_x, scale_y = lerp_pose(begin, wound, alpha ** WIND_EASE)
    shaking = alpha ** 2 if alpha < 1.0 else 0.0
    return offset, scale_x, scale_y, alpha ** TURN_EASE, shaking


def settle(frame):
    """Frames since the kick's hold -> fraction of the way still to go, crossing rest once and settling slow."""
    alpha = frame / float(BEAT - len(KICK) + 1)
    return 0.0 if alpha >= 1.0 else (1.0 - alpha) ** SETTLE_POWER * math.cos(SETTLE_SPRING * alpha)


def needle_state(frame):
    """(offset, X scale, Y scale, roll degrees, sideways rattle) at a frame of the whole sequence.

    The roll only ever grows or holds, so a section starts on exactly the angle its neighbour in the sequence ends
    on: Start turns 0 to TURN, Fire1 holds TURN through the kick then turns on to 2 TURN, End holds 2 TURN, which is
    Start's 0 again. Jumps between sections land on whole quarter turns, which the section cannot show.
    """
    index = min(frame // BEAT, len(SECTIONS) - 1)
    local = frame - index * BEAT
    section = SECTIONS[index][0]
    if section == "Start" and local <= DRAW_END:
        alpha = math.sin(0.5 * math.pi * local / float(DRAW_END))
        drawn = 1.0 + (DRAW_SCALE - 1.0) * alpha
        return DRAW_BACK * alpha, drawn, drawn, 0.0, 0.0
    if section == "Start":
        offset, scale_x, scale_y, turned, shaking = wind(local, DRAW_END, (DRAW_BACK, DRAW_SCALE, DRAW_SCALE))
        return offset, scale_x, scale_y, TURN * turned, RATTLE * shaking * (1.0 if frame % 2 else -1.0)
    if section == "Fire1" and local >= RECOIL:
        offset, scale_x, scale_y, turned, shaking = wind(local, RECOIL - 1, KICK[RECOIL - 1])
        return offset, scale_x, scale_y, TURN * (1.0 + turned), RATTLE * shaking * (1.0 if frame % 2 else -1.0)
    held = TURN if section == "Fire1" else 2.0 * TURN
    if local < len(KICK):
        return KICK[local] + (held, 0.0)
    remaining = settle(local - len(KICK) + 1)
    offset, scale_x, scale_y = KICK[-1]
    return (offset * remaining, 1.0 + (scale_x - 1.0) * remaining, 1.0 + (scale_y - 1.0) * remaining, held, 0.0)


def key(frame, bone, rest_local):
    translation, scale = rest_local.translation, rest_local.scale3d
    if bone != NEEDLE:
        return translation, rest_local.rotation, scale
    offset, scale_x, scale_y, roll, rattle = needle_state(frame)
    # Scale is square across the section so the turn never shows it: Z follows Y.
    return (unreal.Vector(translation.x + offset, translation.y + rattle, translation.z),
            unreal.Rotator(roll=roll).quaternion(),
            unreal.Vector(scale.x * scale_x, scale.y * scale_y, scale.z * scale_y))


def build(toolkit):
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(SKELETON_PATH))
    sequence = toolkit["get_or_create_asset"](ANIM_PACKAGE, SEQUENCE_NAME, unreal.AnimSequence, factory)
    toolkit["write_bone_tracks"](sequence, SKELETON_PATH, FPS, FRAMES, key, "Build triangle badge fire")

    montage = toolkit["build_montage"](sequence, ANIM_PACKAGE, MONTAGE_NAME, [name for name, _ in SECTIONS],
                                       [index * BEAT / float(FPS) for index in range(len(SECTIONS))],
                                       [following for _, following in SECTIONS], SLOT)
    # A new shot replays the montage every beat, so any blend-in would smear one beat into the next.
    for name, seconds in (("blend_in", 0.0), ("blend_out", 0.1)):
        blend = montage.get_editor_property(name)
        blend.set_editor_property("blend_time", seconds)
        montage.set_editor_property(name, blend)
    unreal.EditorAssetLibrary.save_asset("{}/{}".format(ANIM_PACKAGE, MONTAGE_NAME))
    return sequence, montage


def body_outline():
    """The arrowhead's world outline, from the generator that built it."""
    path = unreal.Paths.project_dir() + GENERATOR
    generator = {"__name__": "badge_generator"}
    exec(compile(open(path).read(), path, "exec"), generator)
    return generator["to_world"]("SM_TriangleBadge")[0].outline


def report(toolkit, sequence, montage, groups, outline):
    options = unreal.AnimPoseEvaluationOptions()
    rows = {}
    for frame in range(FRAMES + 1):
        pose = APE.get_anim_pose_at_time(sequence, frame / float(FPS), options)
        local = toolkit["local_pose_table"](pose)[NEEDLE]
        placed = toolkit["place_groups"](groups, toolkit["_component_transforms"](pose))
        hull = toolkit["convex_hull"](placed[NEEDLE])
        rows[frame] = (local, toolkit["outline_separation"](hull, outline), max(p[0] for p in hull))

    LOG.append("{} - {} frames at {} fps, {} sampled keys (expect {})".format(
        SEQUENCE_NAME, FRAMES, FPS, toolkit["playable_key_count"](sequence), FRAMES + 1))
    LOG.append("montage sections read back: {}".format(toolkit["montage_sections"](montage)))
    LOG.append("")
    LOG.append("frame section    dx     dy     sx     sy   roll  to body  tip x")
    rest = rows[FRAMES - BEAT][0]
    for frame in range(FRAMES + 1):
        section = SECTIONS[min(frame // BEAT, len(SECTIONS) - 1)][0]
        (location, _, scale), gap, tip = rows[frame]
        LOG.append("%5d %-7s %+6.2f %+6.2f %6.3f %6.3f %5.0f  %+6.2f  %6.1f" % (
            frame, section, location[0] - rest[0][0], location[1] - rest[0][1], scale[0], scale[1],
            needle_state(frame)[3], gap, tip))

    LOG.append("")
    LOG.append("closest to the arrowhead: %.2f (-1 = overlapping)" % min(row[1] for row in rows.values()))
    steps = [abs(needle_state(f)[3] - needle_state(f - 1)[3]) for f in range(1, FRAMES + 1) if f % BEAT]
    LOG.append("fastest turn %.0f deg/frame (reads backwards past %.0f)" % (max(steps), STROBE))
    starts = {name: index * BEAT for index, (name, _) in enumerate(SECTIONS)}
    for section, followers in FOLLOWERS.items():
        end = rows[starts[section] + BEAT][0]
        for follower in followers:
            start = rows[starts[follower]][0]
            worst = max(abs(a - b) for part in (0, 2) for a, b in zip(end[part], start[part]))
            LOG.append("%s end -> %s start: largest jump %.4f" % (section, follower, worst))


try:
    toolkit_path = unreal.Paths.project_dir() + "AI/Python/Anim/anim_sequence_authoring.py"
    toolkit = {}
    exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)

    groups = toolkit["rigid_vertex_groups"](MESH_PATH, SKELETON_PATH)
    sequence, montage = build(toolkit)
    report(toolkit, sequence, montage, groups, body_outline())
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
