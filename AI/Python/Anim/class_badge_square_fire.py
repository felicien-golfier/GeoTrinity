"""Square badge auto-fire: the mandibles take turns, each thrusting out ahead of the block, swelling and turning about
its own X axis into its shot, flaring wide, slamming back against the block and springing home.

GA_Square_AutoProjectile fires every FireDelay and plays one section per shot, stretched to that delay, with the
shot landing on the section's last frame from anim_socket_<section index>: Start on the right, Fire1 on the left,
Fire2 on the right, then Fire1 again. So every section is one mandible winding up while the other recovers from the
shot before, and Fire1 and Fire2 mirror each other:

    Start   right winds up                         -> End2
    Fire1   left winds up,  right recovers         -> End1
    Fire2   right winds up, left recovers          -> End2
    End1    left recovers                          (firing stopped after a left shot)
    End2    right recovers                         (firing stopped after a right shot)

The sequence holds them in the one order where every section begins on the pose its neighbour ends on.

A beat is authored longer than the ability's delay and sped up to it by the play rate, so the clip keeps the 30 fps
the engine samples sequences at. The End sections run at that same rate. Everything is keyed on the Top branch and
the montage goes in the Top slot, so the body's own layer is left alone.

Run AFTER AI/Python/Mesh/rig_class_badges.py, via mcp-unreal execute_script. Re-runnable: rewrites the sequence and
the montage in place. Report written to Saved/class_badge_square_fire.txt.
"""
import math

import unreal

APE = unreal.AnimPoseExtensions

SKELETON_PATH = "/Game/Characters/Meshes/Class/SK_SquareBadge"
MESH_PATH = "/Game/Characters/Meshes/Class/SKM_SquareBadge"
ANIM_PACKAGE = "/Game/Characters/Anim/ClassBadge/Square"
SEQUENCE_NAME = "SK_SquareBadge_Sequence_Fire"
MONTAGE_NAME = "SK_SquareBadge_Montage_Fire"
REPORT = unreal.Paths.project_saved_dir() + "class_badge_square_fire.txt"

BODY, LEFT, RIGHT = "Bottom", "MandibleLeft", "MandibleRight"
OUTWARD = {LEFT: -1.0, RIGHT: 1.0}
SLOT = "Top"

FPS = 30
BEAT = 12  # sped up to the ability's 0.1 s FireDelay

REACH = 16.0    # units the mandible thrusts ahead of the block, wound up
SPREAD = 18.0   # units it moves out to its own side meanwhile, clear of the other one
GROW = 1.7      # its scale wound up
TURN = 180.0    # degrees about its own X axis per wind-up and again per recovery: the bar's own symmetry
STROBE = 90.0   # half that symmetry: past this per frame a turn reads as going backwards

DIP = 0.15      # how far the wind-up first draws in, the wrong way, as a fraction of the thrust
DIP_END = 2     # frame the dip bottoms out on
STILL = 2       # frames held dead still before the shot
WIND_EASE = 2.2

# Frames after the shot, one row each: thrust (fraction of REACH, or TOUCH for the back face flush on the block),
# spread (fraction of SPREAD), X scale, Y scale, and how much of the recovery's turn is done, on top of the wind-up's.
TOUCH = "touch"
RECOVER = [
    (1.00, 1.00, GROW, GROW, 0.00),                 # the shot: still wound up
    (1.08, 1.00, GROW * 0.85, GROW * 1.20, 0.30),   # mid-flight
    (1.12, 0.95, GROW * 0.72, GROW * 1.40, 0.55),   # flared wide, overshooting
    (1.08, 0.90, GROW * 0.76, GROW * 1.32, 0.72),
    (1.04, 0.85, GROW * 0.78, GROW * 1.30, 0.84),   # held wide
    (0.45, 0.45, 1.20, 1.30, 0.93),                 # thrown back
    (TOUCH, 0.00, 0.80, 1.20, 1.00),                # slammed against the block, squashed by it
    (TOUCH, 0.00, 0.84, 1.14, 1.00),
    (0.14, 0.00, 1.08, 0.96, 1.00),                 # springs out past rest
    (-0.04, 0.00, 0.98, 1.01, 1.00),                # swings short of it
    (0.02, 0.00, 1.00, 1.00, 1.00),
    (0.00, 0.00, 1.00, 1.00, 1.00),
    (0.00, 0.00, 1.00, 1.00, 1.00),
]

# Per section: its name, what each mandible does in it, and the section it runs into when nobody fires again.
# Laid out so each one starts on the pose the one before it in the sequence ends on: a section's last key is the
# next one's first, so any other order interpolates a section's last frame toward the wrong pose.
SECTIONS = [
    ("Fire2", {RIGHT: "wind", LEFT: "recover"}, "End2"),
    ("Fire1", {RIGHT: "recover", LEFT: "wind"}, "End1"),
    ("End1", {RIGHT: "rest", LEFT: "recover"}, "None"),
    ("Start", {RIGHT: "wind", LEFT: "rest"}, "End2"),
    ("End2", {RIGHT: "recover", LEFT: "rest"}, "None"),
]
FRAMES = BEAT * len(SECTIONS)

# Section -> the sections that may follow it, whose first frame has to match its last.
FOLLOWERS = {"Start": ["Fire1", "End2"], "Fire1": ["Fire2", "End1"], "Fire2": ["Fire1", "End2"]}

LOG = []


def wind_drive(frame):
    """0..BEAT into the wind-up -> 0 at rest, dipping the wrong way, accelerating to 1, then held."""
    if frame <= DIP_END:
        return -DIP * math.sin(0.5 * math.pi * frame / float(DIP_END))
    alpha = min(1.0, (frame - DIP_END) / float(BEAT - STILL - DIP_END))
    return -DIP + (1.0 + DIP) * alpha ** WIND_EASE


def wind_turn(frame):
    """0..BEAT into the wind-up -> fraction of TURN done: none through the dip, accelerating, stopped at its fastest."""
    alpha = min(1.0, max(0.0, frame - DIP_END) / float(BEAT - STILL - DIP_END))
    return alpha ** 2


def mandible_state(frame, bone, touch_gap, half_length):
    """The mandible's (thrust, spread, X scale, Y scale, roll degrees) at a frame of the whole sequence."""
    index = min(frame // BEAT, len(SECTIONS) - 1)
    local = frame - index * BEAT
    phase = SECTIONS[index][1][bone]
    if phase == "wind":
        drive = wind_drive(local)
        scale = 1.0 + (GROW - 1.0) * drive
        return REACH * drive, SPREAD * max(0.0, drive), scale, scale, TURN * wind_turn(local)
    if phase == "recover":
        thrust, spread, scale_x, scale_y, turned = RECOVER[local]
        flush = -touch_gap - half_length * (1.0 - scale_x)
        return ((flush if thrust == TOUCH else REACH * thrust), SPREAD * spread, scale_x, scale_y,
                TURN * (1.0 + turned))
    return 0.0, 0.0, 1.0, 1.0, 0.0


def key(frame, bone, rest_local, touch_gap, half_length):
    translation, scale = rest_local.translation, rest_local.scale3d
    if bone not in OUTWARD:
        return translation, rest_local.rotation, scale
    thrust, spread, scale_x, scale_y, roll = mandible_state(frame, bone, touch_gap, half_length)
    return (unreal.Vector(translation.x + thrust, translation.y + OUTWARD[bone] * spread, translation.z),
            unreal.Rotator(roll=roll * OUTWARD[bone]).quaternion(),
            unreal.Vector(scale.x * scale_x, scale.y * scale_y, scale.z))


def build(toolkit, touch_gap, half_length):
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(SKELETON_PATH))
    sequence = toolkit["get_or_create_asset"](ANIM_PACKAGE, SEQUENCE_NAME, unreal.AnimSequence, factory)
    toolkit["write_bone_tracks"](sequence, SKELETON_PATH, FPS, FRAMES,
                                 lambda frame, bone, rest_local: key(frame, bone, rest_local, touch_gap, half_length),
                                 "Build square badge fire")

    montage = toolkit["build_montage"](sequence, ANIM_PACKAGE, MONTAGE_NAME,
                                       [name for name, _, _ in SECTIONS],
                                       [index * BEAT / float(FPS) for index in range(len(SECTIONS))],
                                       [following for _, _, following in SECTIONS], SLOT)
    # A new shot replays the montage every beat, so any blend-in would smear one beat into the next.
    for name, seconds in (("blend_in", 0.0), ("blend_out", 0.05)):
        blend = montage.get_editor_property(name)
        blend.set_editor_property("blend_time", seconds)
        montage.set_editor_property(name, blend)
    unreal.EditorAssetLibrary.save_asset("{}/{}".format(ANIM_PACKAGE, MONTAGE_NAME))
    return sequence, montage


def box(vertices):
    return (min(v.x for v in vertices), max(v.x for v in vertices),
            min(v.y for v in vertices), max(v.y for v in vertices))


def report(toolkit, sequence, montage, groups, body_edge):
    options = unreal.AnimPoseEvaluationOptions()
    poses, gaps, sockets = {}, {}, {}
    for frame in range(FRAMES + 1):
        pose = APE.get_anim_pose_at_time(sequence, frame / float(FPS), options)
        poses[frame] = toolkit["local_pose_table"](pose)
        posed = toolkit["_component_transforms"](pose)
        placed = toolkit["place_groups"](groups, posed)
        left, right = box(placed[LEFT]), box(placed[RIGHT])
        back = min(left[0], right[0])
        # Apart along either axis is apart: boxes only meet when both gaps are negative.
        between = max(right[2] - left[3], max(left[0] - right[1], right[0] - left[1]))
        gaps[frame] = (back - body_edge, between)

    LOG.append("{} - {} frames at {} fps, {} sampled keys (expect {})".format(
        SEQUENCE_NAME, FRAMES, FPS, toolkit["playable_key_count"](sequence), FRAMES + 1))
    LOG.append("montage sections read back: {}".format(toolkit["montage_sections"](montage)))
    LOG.append("")
    LOG.append("frame section  right: dx     dy     sx     sy   roll  left: dx     dy     sx     sy   roll"
               "  to block  between")
    for frame in range(FRAMES + 1):
        section = SECTIONS[min(frame // BEAT, len(SECTIONS) - 1)][0]
        cells = []
        for bone in (RIGHT, LEFT):
            location, _, scale = poses[frame][bone]
            rest = poses[FRAMES][bone][0]
            state = mandible_state(frame, bone, 0.0, 0.0)
            cells.append("%+6.2f %+6.2f %6.3f %6.3f %5.0f" % (
                location[0] - rest[0], location[1] - rest[1], scale[0], scale[1], state[4]))
        LOG.append("%5d %-7s  %s   %s   %+7.2f  %7.2f" % ((frame, section) + tuple(cells) + gaps[frame]))

    LOG.append("")
    LOG.append("closest to the block: %.2f (0 = touching, negative = through it)" % min(g[0] for g in gaps.values()))
    LOG.append("closest the mandibles come to each other: %.2f (negative = overlapping)" % min(
        g[1] for g in gaps.values()))
    fastest = max(abs(mandible_state(f, RIGHT, 0, 0)[4] - mandible_state(f - 1, RIGHT, 0, 0)[4])
                  for f in range(1, FRAMES + 1) if f % BEAT)
    LOG.append("fastest turn %.0f deg/frame (reads backwards past %.0f)" % (fastest, STROBE))
    starts = {name: index * BEAT for index, (name, _, _) in enumerate(SECTIONS)}
    for section, followers in FOLLOWERS.items():
        end = poses[starts[section] + BEAT]
        for follower in followers:
            start = poses[starts[follower]]
            worst = max(abs(a - b) for bone in (LEFT, RIGHT) for part in (0, 2)
                        for a, b in zip(end[bone][part], start[bone][part]))
            LOG.append("%s end -> %s start: largest jump %.4f" % (section, follower, worst))


try:
    toolkit_path = unreal.Paths.project_dir() + "AI/Python/Anim/anim_sequence_authoring.py"
    toolkit = {}
    exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)

    groups = toolkit["rigid_vertex_groups"](MESH_PATH, SKELETON_PATH)
    reference = toolkit["_component_transforms"](APE.get_reference_pose(unreal.load_asset(SKELETON_PATH)))
    body_edge = max(vertex.x for vertex in toolkit["place_groups"](groups, reference)[BODY])
    half_length = -min(vertex.x for vertex in groups[RIGHT])
    touch_gap = reference[RIGHT].translation.x - half_length - body_edge

    sequence, montage = build(toolkit, touch_gap, half_length)
    LOG.append("block edge at x %.2f, mandible half length %.2f, gap to the block %.2f" % (
        body_edge, half_length, touch_gap))
    report(toolkit, sequence, montage, groups, body_edge)
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
