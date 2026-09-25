"""Body turns in the bottom slot: the body alone rolls over about the badge's aim, so the parts stay with whatever the
top slot plays on them — an auto-fire keeps firing through the turn — while the body the top slot moves gives way.

    SK_CircleBadge_Montage_MoiraBeam     GA_MoiraBeam
        Start     pulled back and rolled the wrong way, held still
        Channel   rolls over at a constant rate, pushed back by the beam, looping until the channel ends
    SK_TriangleBadge_Montage_Recall      GA_TurretRecall
        Start     drawn back and rolled the wrong way
        End       one long roll over, slowing, reeling the turrets in; held, then hauled slowly home

The roll is about the aim through the badge's own mid-plane: the Triangle's needle and the Circle's hourglass sit on
that line, so the body turns round them without passing through, where a turn in the view plane would sweep through
them. Every turn is a whole one, so each clip ends or loops on a rotation it started on; the recall drops its turn
on the frame after it lands, as the same rotation.

Each ability plays Start stretched to its FireDelay and keeps that play rate for what follows: 15 frames against the
Moira beam's 0.5 s and 3 frames against the recall's 0.1 s both run at 1x.

Run AFTER AI/Python/Mesh/rig_class_badges.py, via mcp-unreal execute_script, then AI/Python/Anim/class_badge_wiring.py.
Re-runnable: rewrites the sequences and montages in place. Report written to Saved/class_badge_body_turns.txt.
"""
import unreal

APE = unreal.AnimPoseExtensions

MESH_FOLDER = "/Game/Characters/Meshes/Class"
ANIM_FOLDER = "/Game/Characters/Anim/ClassBadge"
GENERATOR = "AI/Python/Mesh/generate_class_badge_meshes.py"
REPORT = unreal.Paths.project_saved_dir() + "class_badge_body_turns.txt"

BODY = "Bottom"
SLOT = "Bottom"
FPS = 30


def smooth(alpha):
    return alpha * alpha * (3.0 - 2.0 * alpha)


def decelerate(alpha):
    return 1.0 - (1.0 - alpha) ** 2


def snap(alpha):
    return alpha


def body(forward=0.0, scale_x=1.0, scale_y=1.0, roll=0.0):
    """The body's pose: its offset along the aim, its scale, and its roll about the aim."""
    return forward, scale_x, scale_y, roll


REST = body()
MOIRA_START = 15
MOIRA_SPIN = 48                                  # frames per loop, two whole rolls in it
MOIRA_WOUND = body(-6.0, 0.94, 1.06, roll=-40.0)
RECALL_START = 3
RECALL_HELD = body(-1.6, 1.0, 1.03, roll=360.0)

# Per clip: the badge, its sections as (name, first frame, next), and body keys as (frame, pose, easing into it).
# The body only ever draws back and narrows: forward or longer, it would close on the part in front of it. The
# Triangle's draws back short of the room its hole leaves the plug, and never shortens, which closes the hole on it.
CLIPS = {
    "MoiraBeam": {
        "badge": "Circle",
        "sections": [("Start", 0, "Channel"), ("Channel", MOIRA_START, "Channel")],
        "keys": [(0, REST, smooth),
                 (10, MOIRA_WOUND, smooth),
                 (MOIRA_START, MOIRA_WOUND, smooth),              # held still
                 (MOIRA_START + MOIRA_SPIN, body(-6.0, 0.94, 1.06, roll=-40.0 + 720.0), snap)],
        "blend": (0.1, 0.25),
    },
    "Recall": {
        "badge": "Triangle",
        "sections": [("Start", 0, "End"), ("End", RECALL_START, "None")],
        "keys": [(0, REST, smooth),
                 (RECALL_START, body(-1.0, 1.0, 1.02, roll=-35.0), smooth),
                 (4, body(-1.5, 1.0, 1.03, roll=5.0), snap),      # let go
                 (30, body(-1.8, 1.0, 1.035, roll=375.0), decelerate),
                 (36, RECALL_HELD, smooth),
                 (42, RECALL_HELD, smooth),
                 (62, body(roll=360.0), smooth),
                 (63, REST, snap)],                               # the same rotation, a whole turn round
        "blend": (0.05, 0.2),
    },
}

LOG = []


def body_pose(keys, frame):
    for (before, start, _), (after, end, ease) in zip(keys, keys[1:]):
        if before <= frame <= after:
            alpha = ease((frame - before) / float(after - before))
            return tuple(a + (b - a) * alpha for a, b in zip(start, end))
    return keys[-1][1]


def key(toolkit, clip, frame, bone, rest_local):
    translation, rotation, scale = rest_local.translation, rest_local.rotation, rest_local.scale3d
    if bone == BODY:
        forward, scale_x, scale_y, roll = body_pose(clip["keys"], frame)
        # The body bone sits below the badge's mid-plane, which is what the roll turns about.
        translation, rotation = toolkit["turn_about"](
            unreal.Vector(translation.x + forward, translation.y, translation.z), unreal.Rotator(roll=roll),
            unreal.Vector(0.0, 0.0, -translation.z))
        scale = unreal.Vector(scale.x * scale_x, scale.y * scale_y, scale.z)
    return translation, rotation, scale


def body_shape(badge):
    """The badge body's world outline, and its half height along X for a wedge, else None, from the generator."""
    path = unreal.Paths.project_dir() + GENERATOR
    generator = {"__name__": "badge_generator"}
    exec(compile(open(path).read(), path, "exec"), generator)
    name = "SM_{}Badge".format(badge)
    body = generator["to_world"](name)[0]
    if body.kind != generator["WEDGE"]:
        return body.outline, None
    back, front = min(p[0] for p in body.outline), max(p[0] for p in body.outline)
    half = 0.5 * generator["BODY_HEIGHT"][name]
    return body.outline, lambda x: half * generator["wedge"](x, back, front)


def build(toolkit, name, clip):
    badge = clip["badge"]
    skeleton = "{}/SK_{}Badge".format(MESH_FOLDER, badge)
    package = "{}/{}".format(ANIM_FOLDER, badge)
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(skeleton))
    sequence = toolkit["get_or_create_asset"](package, "SK_{}Badge_Sequence_{}".format(badge, name),
                                              unreal.AnimSequence, factory)
    frames = clip["keys"][-1][0]
    toolkit["write_bone_tracks"](sequence, skeleton, FPS, frames,
                                 lambda frame, bone, rest_local: key(toolkit, clip, frame, bone, rest_local),
                                 "Build {} badge {}".format(badge, name))

    montage_name = "SK_{}Badge_Montage_{}".format(badge, name)
    montage = toolkit["build_montage"](sequence, package, montage_name, [n for n, _, _ in clip["sections"]],
                                       [start / float(FPS) for _, start, _ in clip["sections"]],
                                       [following for _, _, following in clip["sections"]], SLOT)
    for blend_name, seconds in zip(("blend_in", "blend_out"), clip["blend"]):
        blend = montage.get_editor_property(blend_name)
        blend.set_editor_property("blend_time", seconds)
        montage.set_editor_property(blend_name, blend)
    unreal.EditorAssetLibrary.save_asset("{}/{}".format(package, montage_name))
    return sequence, montage


def report(toolkit, clip, sequence, montage):
    """Every frame: the body's pose, and how deep the parts at rest sit inside it in three dimensions."""
    badge = clip["badge"]
    skeleton_path = "{}/SK_{}Badge".format(MESH_FOLDER, badge)
    groups = toolkit["rigid_vertex_groups"]("{}/SKM_{}Badge".format(MESH_FOLDER, badge), skeleton_path)
    reference = toolkit["_component_transforms"](APE.get_reference_pose(unreal.load_asset(skeleton_path)))
    placed = toolkit["place_groups"](groups, reference)
    heights = [vertex.z for vertex in placed[BODY]]
    parts = [vertex for bone, vertices in placed.items() if bone != BODY for vertex in vertices]
    outline, half_height = body_shape(badge)
    options = unreal.AnimPoseEvaluationOptions()
    frames = clip["keys"][-1][0]

    LOG.append("{} - {} keys for {} frames, sections {}, slots {}, moving {}".format(
        sequence.get_name(), toolkit["playable_key_count"](sequence), frames, toolkit["montage_sections"](montage),
        [str(slot) for slot in unreal.AnimationLibrary.get_montage_slot_names(montage)],
        toolkit["report_moving_bones"](sequence.get_path_name().split(".")[0])))
    LOG.append("frame  forward     sx     sy    roll  parts sunk")
    deepest = 0.0
    for frame in range(frames + 1):
        posed = toolkit["_component_transforms"](APE.get_anim_pose_at_time(sequence, frame / float(FPS), options))
        sunk = toolkit["sunk_depth"](parts, outline, min(heights), max(heights), reference[BODY], posed[BODY],
                                     half_height)
        deepest = max(deepest, sunk)
        LOG.append("%5d  %+7.2f  %5.3f  %5.3f  %6.1f  %6.2f" % (
            (frame,) + body_pose(clip["keys"], frame) + (sunk,)))
    steps = [abs(body_pose(clip["keys"], frame)[3] - body_pose(clip["keys"], frame - 1)[3])
             for frame in range(1, frames + 1)]
    LOG.append("fastest roll %.1f deg/frame; deepest a resting part sits inside the body %.2f" % (
        max(step for step in steps if step < 180.0), deepest))
    LOG.append("")


try:
    toolkit_path = unreal.Paths.project_dir() + "AI/Python/Anim/anim_sequence_authoring.py"
    toolkit = {}
    exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)
    for clip_name, clip_setup in CLIPS.items():
        built_sequence, built_montage = build(toolkit, clip_name, clip_setup)
        report(toolkit, clip_setup, built_sequence, built_montage)
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
