"""Triangle badge auto-fire, crossbow cut: the needle draws back into the arrowhead's opening and compresses like a
drawn string, trembling, the arrowhead stretching back with it like a bow; it holds dead still, then lets go — the
needle is launched far ahead stretched into a spear, the bow snaps and kicks back, and both haul themselves home.

An alternative to class_badge_triangle_fire.py on the same section contract, laid out in the same sequence order:

    Fire1   the launch from the last shot, drawn back again, loops on itself while firing lasts
    End     the launch from the last shot, and home               (firing stopped)
    Start   drawn back from rest                                   -> End

The montage goes in the top slot: the arrowhead's bow shows while the bottom slot plays nothing, and gives way to
whatever it plays, which leaves the needle to this one. A beat is authored at 30 fps and sped up to the ability's
delay by the play rate; End runs at that same rate.

Run AFTER AI/Python/Mesh/rig_class_badges.py, via mcp-unreal execute_script. Re-runnable: rewrites the sequence and
the montage in place. Report written to Saved/class_badge_triangle_fire_crossbow.txt.
"""
import math

import unreal

APE = unreal.AnimPoseExtensions

SKELETON_PATH = "/Game/Characters/Meshes/Class/SK_TriangleBadge"
MESH_PATH = "/Game/Characters/Meshes/Class/SKM_TriangleBadge"
GENERATOR = "AI/Python/Mesh/generate_class_badge_meshes.py"
ANIM_PACKAGE = "/Game/Characters/Anim/ClassBadge/Triangle"
SEQUENCE_NAME = "SK_TriangleBadge_Sequence_FireCrossbow"
MONTAGE_NAME = "SK_TriangleBadge_Montage_FireCrossbow"
REPORT = unreal.Paths.project_saved_dir() + "class_badge_triangle_fire_crossbow.txt"

BODY, NEEDLE = "Bottom", "Needle"
SLOT = "Top"

FPS = 30
BEAT = 18        # sped up to the ability's 0.3 s FireDelay
STILL = 3        # frames held dead still before the shot
WIND_END = BEAT - STILL
RATTLE = 1.4     # units the needle trembles sideways at the height of the draw
TURN = 90.0      # degrees about its long axis per draw and per return: its square section's own symmetry
STROBE = 45.0

# Needle poses: forward offset, X scale, Y and Z scale.
NEEDLE_REST = (0.0, 1.0, 1.0)
NEEDLE_DRAWN = (-9.0, 0.8, 0.8)   # drawn back and shrunk inside the arrowhead's opening

# Frames after the shot: the launch, crossed in two frames with an overshoot, then held.
LAUNCH = [
    NEEDLE_DRAWN,
    (20.0, 1.8, 0.7),    # mid-flight
    (37.4, 2.64, 0.55),  # overshooting
    (34.0, 2.4, 0.6),
    (34.0, 2.4, 0.6),    # held: a spear ahead of the arrowhead
    (34.0, 2.4, 0.6),
]
HELD = len(LAUNCH) - 1

# Arrowhead poses, the bow the needle is the bolt of: backward offset, X scale, Y scale. Its front edge stays where it
# rests whatever the X scale, so it never closes on the needle standing in its opening; the offset only takes it back.
BODY_REST = (0.0, 1.0, 1.0)
BODY_DRAWN = (-1.5, 1.1, 0.9)     # stretched back with the string, its base corners drawn in
# Frames after the shot, the same count as LAUNCH: the limbs snap forward and splay, and the bow kicks back.
BODY_LAUNCH = [
    BODY_DRAWN,
    (-3.0, 0.9, 1.1),
    (-3.5, 0.88, 1.12),
    (-3.0, 0.92, 1.08),
    (-2.5, 0.95, 1.05),
    (-2.0, 0.97, 1.03),
]

ARRIVE = 0.75          # fraction of the way home the needle lands on, squashing
ARRIVE_SQUASH = 0.15
SETTLE_POWER = 1.5
SETTLE_SPRING = 3.4

SECTIONS = [("Fire1", "End"), ("End", "None"), ("Start", "End")]
FRAMES = BEAT * len(SECTIONS)
FOLLOWERS = {"Start": ["Fire1", "End"], "Fire1": ["Fire1", "End"]}

LOG = []


def smoothstep(alpha):
    alpha = min(1.0, max(0.0, alpha))
    return alpha * alpha * (3.0 - 2.0 * alpha)


def lerp(a, b, alpha):
    return tuple(x + (y - x) * alpha for x, y in zip(a, b))


def draw(local, first, needle_from, body_from, frame):
    """Drawn back from the given poses on frame `first`, slowing as it tightens, then held still.

    -> (needle, fraction of TURN done, sideways tremble, arrowhead).
    """
    alpha = min(1.0, max(0.0, local - first) / float(WIND_END - first))
    eased = 1.0 - (1.0 - alpha) ** 2
    tremble = RATTLE * alpha ** 2 * (1.0 if frame % 2 else -1.0) if alpha < 1.0 else 0.0
    return lerp(needle_from, NEEDLE_DRAWN, eased), alpha ** 2, tremble, lerp(body_from, BODY_DRAWN, eased)


def home(local):
    """The way home after the launch's hold -> (needle, fraction of TURN done, arrowhead)."""
    alpha = (local - HELD) / float(BEAT - HELD)
    travel = smoothstep(alpha / ARRIVE)
    forward, scale_x, scale_yz = lerp(LAUNCH[HELD], NEEDLE_REST, travel)
    squash = ARRIVE_SQUASH * math.sin(math.pi * (alpha - ARRIVE) / (1.0 - ARRIVE)) if alpha > ARRIVE else 0.0
    return ((forward, scale_x - squash, scale_yz + squash * 0.7), smoothstep(alpha),
            lerp(BODY_LAUNCH[HELD], BODY_REST, smoothstep(alpha)))


def state(frame):
    """(needle pose, needle roll, sideways tremble, arrowhead pose) at a frame of the whole sequence.

    The roll only grows or holds, so each section starts on the angle its neighbour in the sequence ends on: Fire1
    draws 0 to TURN, End returns TURN to 2 TURN, Start draws 2 TURN to 3 TURN. Jumps land on whole quarter turns.
    """
    index = min(frame // BEAT, len(SECTIONS) - 1)
    local = frame - index * BEAT
    section = SECTIONS[index][0]
    if section == "Start":
        needle, turned, tremble, body = draw(local, 0, NEEDLE_REST, BODY_REST, frame)
        return needle, TURN * (2.0 + turned), tremble, body
    if local <= HELD:
        return LAUNCH[local], 0.0 if section == "Fire1" else TURN, 0.0, BODY_LAUNCH[local]
    if section == "Fire1":
        needle, turned, tremble, body = draw(local, HELD, LAUNCH[HELD], BODY_LAUNCH[HELD], frame)
        return needle, TURN * turned, tremble, body
    needle, turned, body = home(local)
    return needle, TURN * (1.0 + turned), 0.0, body


def key(front, frame, bone, rest_local):
    """`front` is the arrowhead's front edge ahead of its bone, which its X scale is pinned on."""
    translation, scale = rest_local.translation, rest_local.scale3d
    (forward, scale_x, scale_yz), roll, tremble, (back, body_x, body_y) = state(frame)
    if bone == NEEDLE:
        # Y and Z scale alike, so the turn never shows a squashed section.
        return (unreal.Vector(translation.x + forward, translation.y + tremble, translation.z),
                unreal.Rotator(roll=roll).quaternion(),
                unreal.Vector(scale.x * scale_x, scale.y * scale_yz, scale.z * scale_yz))
    if bone == BODY:
        return (unreal.Vector(translation.x + front * (1.0 - body_x) + back, translation.y, translation.z),
                rest_local.rotation, unreal.Vector(scale.x * body_x, scale.y * body_y, scale.z))
    return translation, rest_local.rotation, scale


def build(toolkit, front):
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(SKELETON_PATH))
    sequence = toolkit["get_or_create_asset"](ANIM_PACKAGE, SEQUENCE_NAME, unreal.AnimSequence, factory)
    toolkit["write_bone_tracks"](sequence, SKELETON_PATH, FPS, FRAMES,
                                 lambda frame, bone, rest_local: key(front, frame, bone, rest_local),
                                 "Build triangle badge crossbow fire")

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


def report(toolkit, sequence, montage, groups, reference):
    outline = body_outline()
    options = unreal.AnimPoseEvaluationOptions()
    locals_, gaps = {}, []
    LOG.append("frame section  needle: dx     sx    syz  roll  needle dy  body: dx     sx     sy  to body  tip x")
    for frame in range(FRAMES + 1):
        pose = APE.get_anim_pose_at_time(sequence, frame / float(FPS), options)
        locals_[frame] = toolkit["local_pose_table"](pose)
        posed = toolkit["_component_transforms"](pose)
        hull = toolkit["convex_hull"](toolkit["place_groups"](groups, posed)[NEEDLE])
        gap = toolkit["outline_separation"](hull, toolkit["move_outline"](outline, reference[BODY], posed[BODY]))
        gaps.append(gap)
        needle, roll, tremble, body = state(frame)
        LOG.append("%5d %-7s  %+7.2f %6.3f %6.3f  %4.0f  %+6.2f     %+6.2f %6.3f %6.3f  %+6.2f  %6.1f" % (
            (frame, SECTIONS[min(frame // BEAT, len(SECTIONS) - 1)][0]) + needle
            + (roll, tremble) + body + (gap, max(p[0] for p in hull))))

    LOG.append("")
    LOG.append("closest to the arrowhead: %.2f (-1 = overlapping)" % min(gaps))
    steps = [abs(state(f)[1] - state(f - 1)[1]) for f in range(1, FRAMES + 1) if f % BEAT]
    LOG.append("fastest turn %.0f deg/frame (reads backwards past %.0f)" % (max(steps), STROBE))
    starts = {name: index * BEAT for index, (name, _) in enumerate(SECTIONS)}
    for section, followers in FOLLOWERS.items():
        end = locals_[starts[section] + BEAT]
        for follower in followers:
            start = locals_[starts[follower]]
            worst = max(abs(a - b) for bone in (BODY, NEEDLE) for part in (0, 2)
                        for a, b in zip(end[bone][part], start[bone][part]))
            LOG.append("%s end -> %s start: largest jump %.4f" % (section, follower, worst))


try:
    toolkit_path = unreal.Paths.project_dir() + "AI/Python/Anim/anim_sequence_authoring.py"
    toolkit = {}
    exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)

    groups = toolkit["rigid_vertex_groups"](MESH_PATH, SKELETON_PATH)
    reference = toolkit["_component_transforms"](APE.get_reference_pose(unreal.load_asset(SKELETON_PATH)))
    body_at_rest = toolkit["place_groups"](groups, reference)[BODY]
    front_edge = max(vertex.x for vertex in body_at_rest) - reference[BODY].translation.x
    LOG.append("arrowhead front edge %.2f ahead of its bone" % front_edge)
    sequence, montage = build(toolkit, front_edge)
    LOG.append("{} - {} keys for {} frames, sections {}".format(
        SEQUENCE_NAME, toolkit["playable_key_count"](sequence), FRAMES, toolkit["montage_sections"](montage)))
    report(toolkit, sequence, montage, groups, reference)
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
