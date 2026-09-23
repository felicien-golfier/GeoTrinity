"""Triangle badge reload, heavy: the needle draws deep into the arrowhead and the whole badge braces, turned the
wrong way and straining; it holds still, then heaves itself round a whole turn about its own middle, slow to get
going and hard to stop, throwing the buff pickup out as it swings through; it clunks past where it started, the
needle slamming home and the arrowhead squashing under the weight, and rocks back to rest.

    SK_TriangleBadge_Montage_Reload      GA_Reload
        Start     braced, straining, and the heave under way   stretched to the 0.8 s FireDelay: 24 frames, 1x
        End       the rest of the turn, the clunk, rocking back to rest

The turn is a yaw of the top and bottom layers together, which share their place in the view plane, so the needle
goes round with the arrowhead instead of being swept through. The root and the fire sockets on it stay put. The
montage goes in the full-body slot, over the auto-fire and any body turn: the badge is spinning as a whole.

Run AFTER AI/Python/Mesh/rig_class_badges.py, via mcp-unreal execute_script, then AI/Python/Anim/class_badge_wiring.py.
Re-runnable: rewrites the sequence and the montage in place. Report written to Saved/class_badge_triangle_reload.txt.
"""
import math

import unreal

APE = unreal.AnimPoseExtensions

SKELETON_PATH = "/Game/Characters/Meshes/Class/SK_TriangleBadge"
MESH_PATH = "/Game/Characters/Meshes/Class/SKM_TriangleBadge"
GENERATOR = "AI/Python/Mesh/generate_class_badge_meshes.py"
ANIM_PACKAGE = "/Game/Characters/Anim/ClassBadge/Triangle"
SEQUENCE_NAME = "SK_TriangleBadge_Sequence_Reload"
MONTAGE_NAME = "SK_TriangleBadge_Montage_Reload"
REPORT = unreal.Paths.project_saved_dir() + "class_badge_triangle_reload.txt"

BODY, TOP, NEEDLE = "Bottom", "Top", "Needle"
SLOT = "DefaultSlot"

FPS = 30
START = 24       # the Start section, stretched to the ability's 0.8 s FireDelay
BRACED = 7
STRAIN = 13      # the tremble grows from BRACED to here, then the badge holds dead still
HEAVE = 15
LAND = 38        # the turn stops past a whole one
FRAMES = LAND + 16

WOUND_YAW = -34.0
OVERSHOOT = 14.0
TREMBLE_YAW = 2.5
RATTLE = 1.0     # units the needle trembles sideways
# Yaw rocking back after the clunk: (frame, yaw), eased between.
ROCK = [(LAND, 360.0 + OVERSHOOT), (LAND + 5, 353.0), (LAND + 10, 362.5), (LAND + 15, 360.0)]


def smooth(alpha):
    return alpha * alpha * (3.0 - 2.0 * alpha)


def snap(alpha):
    return alpha


REST = (0.0, 1.0, 1.0)
# Needle: forward offset, X scale, Y and Z scale. Drawn as deep as the crossbow draws it, then slammed home.
NEEDLE_DRAWN = (-9.0, 0.8, 0.8)
NEEDLE_KEYS = [(0, REST, smooth),
               (5, NEEDLE_DRAWN, smooth),                 # leads the brace
               (LAND - 2, NEEDLE_DRAWN, smooth),
               (LAND - 1, (-2.0, 1.15, 0.85), snap),      # mid-flight
               (LAND, (4.0, 1.2, 0.85), snap),            # slammed past home
               (LAND + 3, (-1.5, 0.95, 1.05), smooth),
               (LAND + 6, (0.5, 1.02, 0.99), smooth),
               (LAND + 9, REST, smooth),
               (FRAMES, REST, smooth)]
# Arrowhead: backward offset, X scale, Y scale, its front edge pinned where it rests.
BODY_STRAINED = (-1.5, 1.08, 0.93)
BODY_KEYS = [(0, REST, smooth),
             (BRACED, (-1.0, 1.06, 0.95), smooth),
             (STRAIN, BODY_STRAINED, smooth),
             (HEAVE, BODY_STRAINED, smooth),
             (LAND - 1, (-0.5, 1.02, 0.98), smooth),
             (LAND, (0.0, 0.95, 1.05), snap),
             (LAND + 1, (0.0, 0.88, 1.12), snap),         # the weight lands on it
             (LAND + 5, (0.0, 1.04, 0.97), smooth),
             (LAND + 9, (0.0, 0.98, 1.01), smooth),
             (LAND + 13, REST, smooth),
             (FRAMES, REST, smooth)]

SECTIONS = [("Start", 0, "End"), ("End", START, "None")]

LOG = []


def keyed(keys, frame):
    for (before, start, _), (after, end, ease) in zip(keys, keys[1:]):
        if before <= frame <= after:
            alpha = ease((frame - before) / float(after - before))
            return tuple(a + (b - a) * alpha for a, b in zip(start, end))
    return keys[-1][1]


def heave(frame):
    """Slow to get going, hardest to stop: the rate climbs late and breaks off sharply on the clunk."""
    u = (frame - 0.5) / float(LAND - HEAVE)
    return u ** 1.6 * (1.0 - u) ** 0.6


SPIN = None  # filled from the toolkit's normalised_spin before any key is written


def yaw(frame):
    """The whole badge's yaw -> (degrees, needle's sideways tremble)."""
    if frame <= BRACED:
        return WOUND_YAW * 0.8 * smooth(frame / float(BRACED)), 0.0
    if frame < STRAIN:
        alpha = (frame - BRACED) / float(STRAIN - BRACED)
        side = 1.0 if frame % 2 else -1.0
        return (WOUND_YAW * (0.8 + 0.2 * smooth(alpha)) + TREMBLE_YAW * alpha ** 2 * side,
                RATTLE * alpha ** 2 * -side)
    if frame <= HEAVE:
        return WOUND_YAW, 0.0
    if frame <= LAND:
        return WOUND_YAW + (360.0 + OVERSHOOT - WOUND_YAW) * SPIN[frame - HEAVE], 0.0
    if frame == FRAMES:
        return 0.0, 0.0                                   # the same rotation, a whole turn round
    return keyed([(f, (y,), smooth) for f, y in ROCK], min(frame, ROCK[-1][0]))[0], 0.0


def key(front, frame, bone, rest_local):
    """`front` is the arrowhead's front edge ahead of its bone, which its X scale is pinned on."""
    translation, scale = rest_local.translation, rest_local.scale3d
    turn, tremble = yaw(frame)
    if bone == TOP:
        return translation, unreal.Rotator(yaw=turn).quaternion(), scale
    if bone == BODY:
        back, body_x, body_y = keyed(BODY_KEYS, frame)
        rotation = unreal.Rotator(yaw=turn).quaternion()
        # The pin shift runs along the arrowhead's own axis, which the yaw has turned.
        shift = rotation.rotate_vector(unreal.Vector(front * (1.0 - body_x) + back, 0.0, 0.0))
        return (translation + shift, rotation, unreal.Vector(scale.x * body_x, scale.y * body_y, scale.z))
    if bone == NEEDLE:
        forward, scale_x, scale_yz = keyed(NEEDLE_KEYS, frame)
        return (unreal.Vector(translation.x + forward, translation.y + tremble, translation.z), rest_local.rotation,
                unreal.Vector(scale.x * scale_x, scale.y * scale_yz, scale.z * scale_yz))
    return translation, rest_local.rotation, scale


def build(toolkit, front):
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(SKELETON_PATH))
    sequence = toolkit["get_or_create_asset"](ANIM_PACKAGE, SEQUENCE_NAME, unreal.AnimSequence, factory)
    toolkit["write_bone_tracks"](sequence, SKELETON_PATH, FPS, FRAMES,
                                 lambda frame, bone, rest_local: key(front, frame, bone, rest_local),
                                 "Build triangle badge reload")

    montage = toolkit["build_montage"](sequence, ANIM_PACKAGE, MONTAGE_NAME, [name for name, _, _ in SECTIONS],
                                       [start / float(FPS) for _, start, _ in SECTIONS],
                                       [following for _, _, following in SECTIONS], SLOT)
    for name, seconds in (("blend_in", 0.1), ("blend_out", 0.2)):
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
    rest = toolkit["local_pose_table"](APE.get_reference_pose(unreal.load_asset(SKELETON_PATH)))
    gaps, ends = [], {}
    LOG.append("frame     yaw  needle: dx     sx    syz  body: dx     sx     sy  to body")
    for frame in range(FRAMES + 1):
        pose = APE.get_anim_pose_at_time(sequence, frame / float(FPS), options)
        if frame in (0, FRAMES):
            ends[frame] = toolkit["local_pose_table"](pose)
        posed = toolkit["_component_transforms"](pose)
        hull = toolkit["convex_hull"](toolkit["place_groups"](groups, posed)[NEEDLE])
        gap = toolkit["outline_separation"](hull, toolkit["move_outline"](outline, reference[BODY], posed[BODY]))
        gaps.append(gap)
        LOG.append("%5d  %6.1f   %+7.2f %6.3f %6.3f     %+6.2f %6.3f %6.3f  %+6.2f" % (
            (frame, yaw(frame)[0]) + keyed(NEEDLE_KEYS, frame) + keyed(BODY_KEYS, frame) + (gap,)))

    LOG.append("")
    LOG.append("closest the needle comes to the arrowhead: %.2f (-1 = overlapping)" % min(gaps))
    steps = [abs(yaw(f)[0] - yaw(f - 1)[0]) for f in range(1, FRAMES)]
    LOG.append("fastest turn %.1f deg/frame; yaw at the fire (frame %d) %.1f" % (max(steps), START, yaw(START)[0]))
    for frame, table in ends.items():
        worst = max(abs(a - b) for bone in rest for part in (0, 2) for a, b in zip(table[bone][part], rest[bone][part]))
        turned = max(abs(math.remainder(table[bone][1] - rest[bone][1], 360.0)) for bone in rest)
        LOG.append("frame %d off the reference pose by %.4f units/scale, %.4f deg" % (frame, worst, turned))


try:
    toolkit_path = unreal.Paths.project_dir() + "AI/Python/Anim/anim_sequence_authoring.py"
    toolkit = {}
    exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)
    SPIN = toolkit["normalised_spin"](heave, LAND - HEAVE)

    groups = toolkit["rigid_vertex_groups"](MESH_PATH, SKELETON_PATH)
    reference = toolkit["_component_transforms"](APE.get_reference_pose(unreal.load_asset(SKELETON_PATH)))
    body_at_rest = toolkit["place_groups"](groups, reference)[BODY]
    front_edge = max(vertex.x for vertex in body_at_rest) - reference[BODY].translation.x
    LOG.append("arrowhead front edge %.2f ahead of its bone" % front_edge)
    sequence, montage = build(toolkit, front_edge)
    LOG.append("{} - {} keys for {} frames, sections {}, slots {}".format(
        SEQUENCE_NAME, toolkit["playable_key_count"](sequence), FRAMES, toolkit["montage_sections"](montage),
        [str(slot) for slot in unreal.AnimationLibrary.get_montage_slot_names(montage)]))
    report(toolkit, sequence, montage, groups, reference)
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
