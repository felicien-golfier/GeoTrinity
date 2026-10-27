"""What the Cat clip scripts share: the toolkit, the clip writer, curve helpers and the top-down silhouette check.

A clip script loads this by project-relative path (`exec`) and describes its motion as `pose(frame)`:
    {bone: (translation, rotation, scale)}  deltas on the rest pose, in the bone's parent space
translation is (x, y, z) units, rotation (roll, pitch, yaw) degrees or a quaternion (x, y, z, w), scale (x, y, z)
factors; a bone left out rests, except the fangs, which hide (`FANG_HIDDEN_SCALE`).
The rig's bones rest unturned and unscaled (AI/Tutorial/CatAnimation.md), so a delta is the bone's local transform.

The camera is orthographic and straight above, so `check_clip` judges a clip as it is seen: each part's hull on the
XY plane, which parts newly cover one another, and how high the highest vertex stands.
"""
import itertools
import json
import math

import unreal

_TOOLKIT_PATH = unreal.Paths.project_dir() + "AI/Python/Anim/anim_sequence_authoring.py"
TOOLKIT = {}
exec(compile(open(_TOOLKIT_PATH, encoding="utf-8").read(), _TOOLKIT_PATH, "exec"), TOOLKIT)

APE = unreal.AnimPoseExtensions

SKELETON_PATH = "/Game/Art/Meshes/Cat/SK_Cat"
MESH_PATH = "/Game/Art/Meshes/Cat/SKM_Cat"
ANIM_PACKAGE = "/Game/Characters/Anim/Cat"
FPS = 30

FANG_BONES = ("FangLeft", "FangRight")
HEAD_FAMILY = ("Head", "EarLeft", "EarRight", "EyeLeft", "EyeRight", "Nose", "Mouth0", "Mouth1") + FANG_BONES
FAMILIES = ([("Head", HEAD_FAMILY), ("Body", ("Body",))]
            + [(paw, (paw,)) for paw in ("HandLeft", "HandRight", "FootLeft", "FootRight")]
            + [("Tail{}".format(index), ("Tail{}".format(index),)) for index in range(9)])
SUPPORT_DIRECTIONS = [d for d in itertools.product((-1.0, 0.0, 1.0), repeat=3) if any(d)]
PARTS = [(bone, (bone,)) for _, bones in FAMILIES for bone in bones]  # every bone alone
CLOSURE_TOLERANCE = 0.01  # units and degrees
FANG_HIDDEN_SCALE = 0.001  # not 0: a singular matrix
REST_DELTA = ((0.0, 0.0, 0.0), (0.0, 0.0, 0.0), (1.0, 1.0, 1.0))
FANG_HIDDEN_DELTA = (REST_DELTA[0], REST_DELTA[1], (FANG_HIDDEN_SCALE,) * 3)
EAR_DETACH = {"EarLeft": (0.0, -5.0, 7.0), "EarRight": (0.0, 5.0, 7.0)}  # Head space: the ears float off the head, seen from above once the head is pitched


def smooth(t):
    """Smoothstep of t clamped to 0..1."""
    t = max(0.0, min(1.0, t))
    return t * t * (3.0 - 2.0 * t)


def ramp(frame, start, end):
    """0 before `start`, 1 from `end`, eased between."""
    return smooth((frame - start) / float(end - start))


def curve(frame, keys):
    """Value at `frame` of the eased run through `keys`, [(frame, value)] in order; held outside the first and last."""
    if frame <= keys[0][0]:
        return keys[0][1]

    for (start, a), (end, b) in zip(keys, keys[1:]):
        if frame <= end:
            return a + (b - a) * smooth((frame - start) / float(end - start))
    return keys[-1][1]


def attack_release(frame, start, attack, hold, release, peak=1.0):
    """0 before `start`, rising to `peak` in `attack` frames (fast, easing out), held `hold` frames, back to 0 in
    `release` frames (slow, easing in and out), 0 after: one event that begins and ends at rest."""
    t = frame - start
    if t <= 0:
        return 0.0

    if t < attack:
        return peak * (1.0 - (1.0 - t / float(attack)) ** 2)

    if t < attack + hold:
        return peak
    return peak * (1.0 - smooth((t - attack - hold) / float(release)))


def wander(frame, frames, terms):
    """Sum of whole-cycle sines over a loop of `frames`: terms are (cycles, amplitude, phase in turns). Unequal cycle
    counts make it never visibly repeat, and it still closes."""
    return sum(amplitude * math.sin(2.0 * math.pi * (cycles * frame / float(frames) + phase))
               for cycles, amplitude, phase in terms)


def paw_slide(phase, reach, planted=0.6):
    """A paw's offset along X over one stride (phase in turns): planted, it slides back from +reach to -reach in a
    straight line; swinging, it returns quicker and eased."""
    p = phase % 1.0
    if p < planted:
        return reach * (1.0 - 2.0 * p / planted)
    return -reach + 2.0 * reach * smooth((p - planted) / (1.0 - planted))


def add_skeleton_sequence(name):
    """Load the sequence `name` in the Cat's anim folder, or create it on the Cat skeleton."""
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(SKELETON_PATH))
    return TOOLKIT["get_or_create_asset"](ANIM_PACKAGE, name, unreal.AnimSequence, factory)


def write_clip(name, frames, pose):
    """Write the sequence `name` from `pose(frame)` over `frames` frames at FPS and return it."""
    cache = {}

    def sampler(frame, bone, rest_local):
        if frame not in cache:
            cache.clear()
            cache[frame] = pose(frame)
        translation, rotation, scale = cache[frame].get(bone, FANG_HIDDEN_DELTA if bone in FANG_BONES else REST_DELTA)
        rest = rest_local.translation
        rest_scale = rest_local.scale3d
        return (unreal.Vector(rest.x + translation[0], rest.y + translation[1], rest.z + translation[2]),
                unreal.Quat(*rotation) if len(rotation) == 4 else
                unreal.Rotator(roll=rotation[0], pitch=rotation[1], yaw=rotation[2]).quaternion(),
                unreal.Vector(rest_scale.x * scale[0], rest_scale.y * scale[1], rest_scale.z * scale[2]))

    sequence = add_skeleton_sequence(name)
    TOOLKIT["write_bone_tracks"](sequence, SKELETON_PATH, FPS, frames, sampler, "Author " + name)
    return sequence


def support_points(points):
    """The vertices extreme along each of SUPPORT_DIRECTIONS, enough to outline a part from any pose."""
    return [max(points, key=lambda p: p.x * d[0] + p.y * d[1] + p.z * d[2]) for d in SUPPORT_DIRECTIONS]


def local_snapshot(sequence, time):
    """{bone: (translation, rotation quaternion)} of the pose at `time`, copied out of the shared buffer."""
    pose = APE.get_anim_pose_at_time(sequence, time, unreal.AnimPoseEvaluationOptions())
    table = {}
    for bone in APE.get_bone_names(pose):
        local = APE.get_bone_pose(pose, str(bone), unreal.AnimPoseSpaces.LOCAL)
        table[str(bone)] = ((local.translation.x, local.translation.y, local.translation.z),
                            (local.rotation.x, local.rotation.y, local.rotation.z, local.rotation.w),
                            (local.scale3d.x, local.scale3d.y, local.scale3d.z))
    return table


def closure_gap(first, last):
    """Largest translation (units), turn (degrees) and scale gap between two local snapshots."""
    gap = {"translation": 0.0, "rotation": 0.0, "scale": 0.0}
    for bone, (translation, rotation, scale) in first.items():
        other = last[bone]
        gap["translation"] = max(gap["translation"], max(abs(a - b) for a, b in zip(translation, other[0])))
        dot = min(1.0, abs(sum(a * b for a, b in zip(rotation, other[1]))))
        gap["rotation"] = max(gap["rotation"], math.degrees(2.0 * math.acos(dot)))
        gap["scale"] = max(gap["scale"], max(abs(a - b) for a, b in zip(scale, other[2])))
    return {key: round(value, 4) for key, value in gap.items()}


def family_hulls(supports, posed, families):
    """{family: XY hull} of every part family where `posed` (component transforms) puts it."""
    placed = TOOLKIT["place_groups"](supports, posed)
    hulls = {}
    for family, bones in families:
        hull = TOOLKIT["convex_hull"]([point for bone in bones for point in placed[bone]])
        if len(hull) >= 3:
            hulls[family] = hull
    return hulls, placed


def covered_pairs(hulls):
    """Family pairs whose top-down hulls overlap."""
    return {"{}|{}".format(a, b) for a, b in itertools.combinations(sorted(hulls), 2)
            if TOOLKIT["outline_separation"](hulls[a], hulls[b]) < 0.0}


def check_clip(sequence, frames, step=1, families=FAMILIES, allowed_overlaps=()):
    """Judge a clip top-down -> {closure, new_overlaps, extents, highest_z, below_plane_from, head_pitch_min}.

    new_overlaps: family pairs that cover one another in the pose but not at rest, with the frames they do; the
    `allowed_overlaps` pairs ("Body|Head") are the clip's intended ones and are left out.
    head_pitch_min: [frame, degrees] where the Head is pitched least above its rest (the face peek rule).
    """
    groups = TOOLKIT["rigid_vertex_groups"](MESH_PATH, SKELETON_PATH)
    supports = {bone: support_points(points) for bone, points in groups.items()}
    component = TOOLKIT["_component_transforms"]

    rest_pose = component(APE.get_reference_pose(unreal.load_asset(SKELETON_PATH)))
    rest_hulls, _ = family_hulls(supports, rest_pose, families)
    rest_head_pitch = rest_pose["Head"].rotation.rotator().pitch
    resting = covered_pairs(rest_hulls)

    result = {"frames": frames, "fps": FPS, "resting_overlaps": sorted(resting), "new_overlaps": {}, "extents": [],
              "highest_z": []}
    for frame in sorted(set(range(0, frames + 1, step)) | {frames}):
        pose = APE.get_anim_pose_at_time(sequence, frame / float(FPS), unreal.AnimPoseEvaluationOptions())
        posed = component(pose)
        hulls, placed = family_hulls(supports, posed, families)
        head_pitch = round(posed["Head"].rotation.rotator().pitch - rest_head_pitch, 1)
        if "head_pitch_min" not in result or head_pitch < result["head_pitch_min"][1]:
            result["head_pitch_min"] = [frame, head_pitch]
        points = [point for bone_points in placed.values() for point in bone_points]
        for pair in covered_pairs(hulls) - resting - set(allowed_overlaps):
            result["new_overlaps"].setdefault(pair, []).append(frame)
        result["extents"].append([frame] + [round(value, 1) for value in (
            min(p.x for p in points), max(p.x for p in points), min(p.y for p in points), max(p.y for p in points))])
        result["highest_z"].append([frame, round(max(p.z for p in points), 1)])

    result["below_plane_from"] = next((frame for frame, z in result["highest_z"] if z < 0.0), None)
    result["closure"] = closure_gap(local_snapshot(sequence, 0.0), local_snapshot(sequence, frames / float(FPS)))
    return result


def run(name, frames, pose, step=1, after=None, families=FAMILIES, allowed_overlaps=()):
    """Write the clip, check it and report to AI/Output/cat_<name>.json; `after(sequence)` adds what else the clip
    carries (a montage, notifies) and returns {key: value} for the report."""
    result = {}
    try:
        sequence = write_clip(name, frames, pose)
        result = check_clip(sequence, frames, step, families, allowed_overlaps)
        result.update(after(sequence) if after else {})
        result["ok"] = True
    except Exception as exc:  # noqa
        import traceback
        result["ok"] = False
        result["error"] = str(exc)
        result["trace"] = traceback.format_exc()
    with open(unreal.Paths.project_dir() + "AI/Output/cat_{}.json".format(name), "w") as handle:
        json.dump(result, handle, indent=1)
