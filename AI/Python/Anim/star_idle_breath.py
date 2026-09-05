"""Star boss idle: the whole star breathes in and out on X and Y while a small swell travels around its tips.

The camera is orthographic down Z, so only X and Y scale and Z stays at the reference pose. Scaling the root
bone is enough for the breath: the resulting skinning matrix is the root's own delta, so every vertex scales
about the star's axis whatever bone carries it — no bone needs a skin weight for this to reach it.

The tip bones are discovered by name and each one's outward direction is read from where it sits, the same way
the pike nova drives them, but a fraction of the travel. Breath and tip wave share one period the length of the
clip, and the clip holds a whole number of frames per tip, so the loop closes and the beat lands on samples.

Run via mcp-unreal execute_script. Re-runnable: rewrites the sequence in place.
"""
import math

import unreal

APE = unreal.AnimPoseExtensions

SKELETON_PATH = "/Game/Characters/Meshes/Star/SK_Star"
MESH_PATH = "/Game/Characters/Meshes/Star/SKM_Star"
ANIM_PACKAGE = "/Game/Characters/Anim/Star"
SEQ_NAME = "SK_Star_Idle"
REPORT = "C:/Users/Felou/AppData/Local/Temp/claude/C--GeoTrinity/43c49c4b-16e7-4c2d-ab55-b98aad7877bc/scratchpad/star_idle.txt"

SPIKE_PREFIX = "apexe_outside_"
ROOT_BONE = "Root"

FPS = 30
FRAMES = 96      # 3.2s, and a whole number of frames per tip so the travelling swell lands on samples
BREATH = 0.03    # fraction of itself the star swells and shrinks in X and Y
REACH = 18.0     # units a tip bone travels at the crest; its vertex moves this scaled by its weight
CREST = 3        # higher narrows the swell to fewer tips out at once

LOG = []


def reference_pose():
    """Bone name -> (local transform, component transform) for the skeleton's reference pose."""
    ref = APE.get_reference_pose(unreal.load_asset(SKELETON_PATH))
    return {str(bone): (APE.get_bone_pose(ref, str(bone), unreal.AnimPoseSpaces.LOCAL),
                        APE.get_bone_pose(ref, str(bone), unreal.AnimPoseSpaces.WORLD))
            for bone in APE.get_bone_names(ref)}


def spike_axes(rest):
    """Tip bone name -> (outward unit x, outward unit y, position in the wave), read from where each bone sits."""
    names = sorted((name for name in rest if name.startswith(SPIKE_PREFIX)),
                   key=lambda name: int(name[len(SPIKE_PREFIX):]))
    axes = {}
    for order, name in enumerate(names):
        position = rest[name][1].translation
        length = math.hypot(position.x, position.y)
        axes[name] = (position.x / length, position.y / length, order)
    return axes


def breath(frame):
    """The whole-star scale factor: one full in-and-out over the clip, back to its start on the last frame."""
    return 1.0 + BREATH * math.sin(2.0 * math.pi * frame / float(FRAMES))


def swell(frame, order, spikes):
    """0 to 1 for one tip — a soft crest that travels around the star once per clip."""
    phase = frame / float(FRAMES) - order / float(spikes)
    return (0.5 + 0.5 * math.cos(2.0 * math.pi * phase)) ** CREST


def build_sequence(rest, axes):
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(SKELETON_PATH))
    path = "{}/{}".format(ANIM_PACKAGE, SEQ_NAME)
    sequence = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else \
        unreal.AssetToolsHelpers.get_asset_tools().create_asset(SEQ_NAME, ANIM_PACKAGE, unreal.AnimSequence, factory)

    controller = sequence.get_editor_property("controller")
    controller.open_bracket("Build idle breath")
    controller.set_frame_rate(unreal.FrameRate(FPS, 1))
    controller.set_number_of_frames(unreal.FrameNumber(FRAMES))
    for bone, (rest_local, _) in rest.items():
        positions, rotations, scales = [], [], []
        for frame in range(FRAMES + 1):  # a sequence holds one more key than its frame count
            translation = unreal.Vector(rest_local.translation.x, rest_local.translation.y, rest_local.translation.z)
            scale = unreal.Vector(rest_local.scale3d.x, rest_local.scale3d.y, rest_local.scale3d.z)
            if bone == ROOT_BONE:
                factor = breath(frame)
                scale.x *= factor
                scale.y *= factor
            if bone in axes:
                x, y, order = axes[bone]
                offset = REACH * swell(frame, order, len(axes))
                translation.x += x * offset
                translation.y += y * offset
            positions.append(translation)
            rotations.append(rest_local.rotation)
            scales.append(scale)
        # Unconditional and result ignored: the adder reports failure for an existing track, and the key setter
        # reports success whether or not a track is there.
        controller.add_bone_curve(bone)
        controller.set_bone_track_keys(bone, positions, rotations, scales)
    controller.close_bracket()
    unreal.EditorAssetLibrary.save_asset(path)
    return sequence


def report(sequence, axes):
    """Evaluate the finished sequence — the track list reads a legacy path and reports empty for every sequence."""
    options = unreal.AnimPoseEvaluationOptions()
    order = sorted(axes, key=lambda name: axes[name][2])

    modifier = unreal.SkinWeightModifier()
    modifier.set_skeletal_mesh(unreal.load_asset(MESH_PATH))
    carried = {}
    for index in range(modifier.get_num_vertices()):
        for bone, weight in modifier.get_vertex_weights(index).items():
            if str(bone) in axes and weight > 0.001:
                carried[str(bone)] = weight

    LOG.append("sequence {:.3f}s over {} frames at {} fps, {} frames per tip".format(
        FRAMES / float(FPS), FRAMES, FPS, FRAMES // len(axes)))
    LOG.append("breath +/-{:.0f}% on X and Y; tip bone travels {:.0f} at the crest, its vertex {:.1f} at weight {}".format(
        BREATH * 100.0, REACH, REACH * min(carried.values()), sorted(set(round(w, 2) for w in carried.values()))))
    LOG.append("")
    LOG.append("frame  rootXY  rootZ  " + "  ".join("%6s" % name[len(SPIKE_PREFIX):] for name in order))
    for frame in range(0, FRAMES + 1, 6):
        pose = APE.get_anim_pose_at_time(sequence, frame / float(FPS), options)
        root = APE.get_bone_pose(pose, ROOT_BONE, unreal.AnimPoseSpaces.LOCAL).scale3d
        reach = []
        for name in order:
            position = APE.get_bone_pose(pose, name, unreal.AnimPoseSpaces.WORLD).translation
            reach.append(math.hypot(position.x, position.y))
        LOG.append("%5d  %6.4f %6.4f  " % (frame, root.x, root.z) + "  ".join("%6.2f" % r for r in reach))

    first = APE.get_anim_pose_at_time(sequence, 0.0, options)
    firsts = [(APE.get_bone_pose(first, n, unreal.AnimPoseSpaces.WORLD).translation.x,
               APE.get_bone_pose(first, n, unreal.AnimPoseSpaces.WORLD).translation.y) for n in order]
    last = APE.get_anim_pose_at_time(sequence, FRAMES / float(FPS), options)
    lasts = [(APE.get_bone_pose(last, n, unreal.AnimPoseSpaces.WORLD).translation.x,
              APE.get_bone_pose(last, n, unreal.AnimPoseSpaces.WORLD).translation.y) for n in order]
    gap = max(math.hypot(a[0] - b[0], a[1] - b[1]) for a, b in zip(firsts, lasts))
    LOG.append("")
    LOG.append("loop closure: largest tip gap between first and last frame = %.4f units" % gap)


rest = reference_pose()
axes = spike_axes(rest)
if not axes:
    LOG.append("ABORTED: no bone named {}N in the skeleton".format(SPIKE_PREFIX))
elif FRAMES % len(axes):
    LOG.append("ABORTED: {} frames does not divide evenly by {} tips".format(FRAMES, len(axes)))
else:
    report(build_sequence(rest, axes), axes)

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
