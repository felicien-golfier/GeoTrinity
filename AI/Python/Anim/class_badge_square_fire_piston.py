"""Square badge auto-fire, piston cut: each mandible cocks outward like a hammer, then jabs out long and thin ahead of
the block, rams back into it and springs home.

An alternative to class_badge_square_fire.py on the same section contract, laid out in the same sequence order:

    Fire2   right winds up, left recovers          -> End2
    Fire1   left winds up,  right recovers         -> End1
    End1    left recovers                          (firing stopped after a left shot)
    Start   right winds up                         -> End2
    End2    right recovers                         (firing stopped after a right shot)

Only the mandibles move, so the montage goes in the top slot and the block is left to the sacrifice in the bottom
one; a mandible rammed back lands flush on the block at rest, which the sacrifice never brings forward. A beat is
authored at 30 fps and sped up to the ability's delay by the play rate; the End sections run at that same rate.

Run AFTER AI/Python/Mesh/rig_class_badges.py, via mcp-unreal execute_script. Re-runnable: rewrites the sequence and
the montage in place. Report written to Saved/class_badge_square_fire_piston.txt.
"""
import unreal

APE = unreal.AnimPoseExtensions

SKELETON_PATH = "/Game/Characters/Meshes/Class/SK_SquareBadge"
MESH_PATH = "/Game/Characters/Meshes/Class/SKM_SquareBadge"
GENERATOR = "AI/Python/Mesh/generate_class_badge_meshes.py"
ANIM_PACKAGE = "/Game/Characters/Anim/ClassBadge/Square"
SEQUENCE_NAME = "SK_SquareBadge_Sequence_FirePiston"
MONTAGE_NAME = "SK_SquareBadge_Montage_FirePiston"
REPORT = unreal.Paths.project_saved_dir() + "class_badge_square_fire_piston.txt"

BODY, LEFT, RIGHT = "Bottom", "MandibleLeft", "MandibleRight"
OUTWARD = {LEFT: -1.0, RIGHT: 1.0}
SLOT = "Top"

FPS = 30
BEAT = 12  # sped up to the ability's 0.1 s FireDelay

# Mandible poses: forward, outward, yaw turning its front outward, X scale, Y scale.
REST = (0.0, 0.0, 0.0, 1.0, 1.0)
COCKED = (4.0, 7.0, 28.0, 0.9, 0.9)
# Long and thin, out along the ledge and splayed outward, so the two shots leave far apart.
JAB = (22.0, 10.0, 12.0, 2.4, 0.75)

# The wind-up, keyed by frame and eased between: cocked outward, held trembling, then across to the jab in two frames.
WIND = [
    (0, REST),
    (5, COCKED),
    (7, COCKED),
    (8, (12.0, 9.0, 20.0, 1.5, 0.95)),  # mid-flight
    (9, (24.0, 11.0, 11.0, 2.6, 0.7)),  # overshooting
    (10, JAB),
    (BEAT, JAB),                        # held dead still into the shot
]
TREMBLE = {5: 1.0, 6: -1.5, 7: 1.5}  # outward shake while held cocked

# Frames after the shot. TOUCH in place of the forward offset puts the back face flush on the block at rest.
TOUCH = "touch"
RECOVER = [
    JAB,
    (9.0, 6.0, 5.0, 1.3, 1.05),      # snapping back
    (TOUCH, 2.0, 0.0, 0.6, 1.3),     # rammed into the block, squashed flat
    (TOUCH, 1.0, 0.0, 0.68, 1.24),
    (2.5, 0.0, 1.0, 1.1, 0.95),      # springs out past rest
    (0.8, 0.0, 0.3, 1.03, 0.99),
    (-0.3, 0.0, 0.0, 0.99, 1.0),     # swings short of it
] + [REST] * (BEAT + 1 - 7)

# Per section: its name, what each mandible does in it, and the section it runs into when nobody fires again.
SECTIONS = [
    ("Fire2", {RIGHT: "wind", LEFT: "recover"}, "End2"),
    ("Fire1", {RIGHT: "recover", LEFT: "wind"}, "End1"),
    ("End1", {RIGHT: "rest", LEFT: "recover"}, "None"),
    ("Start", {RIGHT: "wind", LEFT: "rest"}, "End2"),
    ("End2", {RIGHT: "recover", LEFT: "rest"}, "None"),
]
FRAMES = BEAT * len(SECTIONS)
FOLLOWERS = {"Start": ["Fire1", "End2"], "Fire1": ["Fire2", "End1"], "Fire2": ["Fire1", "End2"]}

LOG = []


def smoothstep(alpha):
    return alpha * alpha * (3.0 - 2.0 * alpha)


def section_at(frame):
    index = min(frame // BEAT, len(SECTIONS) - 1)
    return SECTIONS[index], frame - index * BEAT


def wind_pose(local):
    for (before, start), (after, end) in zip(WIND, WIND[1:]):
        if before <= local <= after:
            alpha = smoothstep((local - before) / float(after - before))
            pose = [a + (b - a) * alpha for a, b in zip(start, end)]
            pose[1] += TREMBLE.get(local, 0.0)
            return tuple(pose)
    return WIND[-1][1]


def mandible_state(frame, bone, touch_gap, half_length):
    """The mandible's (forward, outward, yaw, X scale, Y scale) at a frame of the whole sequence."""
    (_, phases, _), local = section_at(frame)
    phase = phases[bone]
    if phase == "wind":
        return wind_pose(local)
    if phase == "recover":
        forward, outward, yaw, scale_x, scale_y = RECOVER[local]
        if forward == TOUCH:
            forward = -touch_gap - half_length * (1.0 - scale_x)
        return forward, outward, yaw, scale_x, scale_y
    return REST


def key(frame, bone, rest_local, geometry):
    translation, scale = rest_local.translation, rest_local.scale3d
    if bone not in OUTWARD:
        return translation, rest_local.rotation, scale
    side = OUTWARD[bone]
    forward, outward, yaw, scale_x, scale_y = mandible_state(frame, bone, *geometry)
    return (unreal.Vector(translation.x + forward, translation.y + side * outward, translation.z),
            unreal.Rotator(yaw=yaw * side).quaternion(),
            unreal.Vector(scale.x * scale_x, scale.y * scale_y, scale.z))


def build(toolkit, geometry):
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(SKELETON_PATH))
    sequence = toolkit["get_or_create_asset"](ANIM_PACKAGE, SEQUENCE_NAME, unreal.AnimSequence, factory)
    toolkit["write_bone_tracks"](sequence, SKELETON_PATH, FPS, FRAMES,
                                 lambda frame, bone, rest_local: key(frame, bone, rest_local, geometry),
                                 "Build square badge piston fire")

    montage = toolkit["build_montage"](sequence, ANIM_PACKAGE, MONTAGE_NAME, [name for name, _, _ in SECTIONS],
                                       [index * BEAT / float(FPS) for index in range(len(SECTIONS))],
                                       [following for _, _, following in SECTIONS], SLOT)
    # A new shot replays the montage every beat, so any blend-in would smear one beat into the next.
    for name, seconds in (("blend_in", 0.0), ("blend_out", 0.05)):
        blend = montage.get_editor_property(name)
        blend.set_editor_property("blend_time", seconds)
        montage.set_editor_property(name, blend)
    unreal.EditorAssetLibrary.save_asset("{}/{}".format(ANIM_PACKAGE, MONTAGE_NAME))
    return sequence, montage


def block_outline():
    """The block's world outline, from the generator that built it."""
    path = unreal.Paths.project_dir() + GENERATOR
    generator = {"__name__": "badge_generator"}
    exec(compile(open(path).read(), path, "exec"), generator)
    return generator["to_world"]("SM_SquareBadge")[0].outline


def report(toolkit, sequence, montage, groups, reference, geometry):
    outline = block_outline()
    options = unreal.AnimPoseEvaluationOptions()
    locals_, worst_block, worst_between = {}, float("inf"), float("inf")
    LOG.append("frame section  right: dx     dy    yaw     sx     sy   left: dx     dy    yaw     sx     sy"
               "  to block  between")
    for frame in range(FRAMES + 1):
        pose = APE.get_anim_pose_at_time(sequence, frame / float(FPS), options)
        locals_[frame] = toolkit["local_pose_table"](pose)
        posed = toolkit["_component_transforms"](pose)
        placed = toolkit["place_groups"](groups, posed)
        block = toolkit["move_outline"](outline, reference[BODY], posed[BODY])
        hulls = {bone: toolkit["convex_hull"](placed[bone]) for bone in OUTWARD}
        to_block = min(toolkit["outline_separation"](hull, block) for hull in hulls.values())
        between = toolkit["outline_separation"](hulls[LEFT], hulls[RIGHT])
        worst_block, worst_between = min(worst_block, to_block), min(worst_between, between)
        cells = ["%+6.2f %+6.2f %6.1f %6.3f %6.3f" % mandible_state(frame, bone, *geometry) for bone in (RIGHT, LEFT)]
        LOG.append("%5d %-7s  %s   %s   %+7.2f  %+7.2f" % (
            frame, section_at(frame)[0][0], cells[0], cells[1], to_block, between))

    LOG.append("")
    LOG.append("closest to the block: %.2f, closest the mandibles come: %.2f (-1 = overlapping)" % (
        worst_block, worst_between))
    # Each wind-up section fires from anim_socket_<n> on its last frame: Start 0, Fire1 1, Fire2 2.
    mesh = unreal.load_asset(MESH_PATH)
    for index, (name, _, _) in enumerate(SECTIONS):
        if name in ("Start", "Fire1", "Fire2"):
            socket = mesh.find_socket("anim_socket_{}".format(0 if name == "Start" else int(name[-1])))
            pose = APE.get_anim_pose_at_time(sequence, (index + 1) * BEAT / float(FPS), options)
            bone = str(socket.get_editor_property("bone_name"))
            shot = unreal.MathLibrary.transform_location(toolkit["_component_transforms"](pose)[bone],
                                                         socket.get_socket_local_transform().translation)
            LOG.append("%s shoots from %s at x %.1f, y %+.1f" % (name, bone, shot.x, shot.y))
    starts = {name: index * BEAT for index, (name, _, _) in enumerate(SECTIONS)}
    for section, followers in FOLLOWERS.items():
        end = locals_[starts[section] + BEAT]
        for follower in followers:
            start = locals_[starts[follower]]
            worst = max(abs(a - b) for bone in (BODY, LEFT, RIGHT) for part in (0, 2)
                        for a, b in zip(end[bone][part], start[bone][part]))
            turn = max(abs(end[bone][1] - start[bone][1]) for bone in (LEFT, RIGHT))
            LOG.append("%s end -> %s start: largest jump %.4f, turn %.2f deg" % (section, follower, worst, turn))


try:
    toolkit_path = unreal.Paths.project_dir() + "AI/Python/Anim/anim_sequence_authoring.py"
    toolkit = {}
    exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)

    groups = toolkit["rigid_vertex_groups"](MESH_PATH, SKELETON_PATH)
    reference = toolkit["_component_transforms"](APE.get_reference_pose(unreal.load_asset(SKELETON_PATH)))
    block_edge = max(vertex.x for vertex in toolkit["place_groups"](groups, reference)[BODY])
    half_length = -min(vertex.x for vertex in groups[RIGHT])
    touch_gap = reference[RIGHT].translation.x - half_length - block_edge
    geometry = (touch_gap, half_length)

    sequence, montage = build(toolkit, geometry)
    LOG.append("{} - {} keys for {} frames, sections {}".format(
        SEQUENCE_NAME, toolkit["playable_key_count"](sequence), FRAMES, toolkit["montage_sections"](montage)))
    report(toolkit, sequence, montage, groups, reference, geometry)
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
