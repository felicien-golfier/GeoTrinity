"""Star boss "pike nova": the tip bones fire one at a time around the star, then all together in a finale.

Each tip retracts as the next leaves, and once the sweep reaches its last tip the finale takes every tip out
together, further than any of them travelled on its own.

Drives the apexe_outside_N bones that come with the rig — the script never edits the skeleton or the skin
weights, so whatever each bone is painted to move is what moves. Bones are discovered by name and fire in
their numeric order, and each one's outward direction is read from where it sits, so the sequence follows the
rig rather than assuming a tip sits at any particular angle.

Writes the sequence, wraps it in the montage, points the pattern Blueprint at it, then evaluates the result.
Run via mcp-unreal execute_script. Re-runnable: rewrites the same assets in place.
"""
import math

import unreal

APE = unreal.AnimPoseExtensions

SKELETON_PATH = "/Game/Characters/Meshes/Star/SK_Star"
MESH_PATH = "/Game/Characters/Meshes/Star/SKM_Star"
ANIM_PACKAGE = "/Game/Characters/Anim/Star"
SEQ_NAME = "SK_Star_Sequence_PikeNova"
MONTAGE_NAME = "SK_Star_PikeNova_Montage"
PATTERN_BP = "/Game/AbilitySystem/Abilities/Enemy/SpawnPillar/BP_SpawnPillarPattern"
REPORT = "C:/Users/Felou/AppData/Local/Temp/claude/C--GeoTrinity/43c49c4b-16e7-4c2d-ab55-b98aad7877bc/scratchpad/pike_nova.txt"

SPIKE_PREFIX = "apexe_outside_"

FPS = 30
LEAD_IN = 3         # beat before the first tip leaves, so the burst has an attack
OUT_FRAMES = 4      # a tip's travel out
BACK_FRAMES = 8     # its slower retract, still running as the next two leave
STEP = OUT_FRAMES   # the next tip leaves exactly as the previous one peaks
REACH = 100.0       # units the bone travels; a vertex moves this scaled by its weight on that bone

FINALE_REACH = 150.0  # every tip together, further than any of them reached on its own
FINALE_OUT = 6
FINALE_HOLD = 3
FINALE_BACK = 10

SECTION_NAMES = ["Start", "Fire", "End"]
SECTION_NEXT = ["Fire", "End", "None"]

LOG = []


def reference_pose():
    """Bone name -> (local transform, component transform) for the skeleton's reference pose."""
    ref = APE.get_reference_pose(unreal.load_asset(SKELETON_PATH))
    return {str(bone): (APE.get_bone_pose(ref, str(bone), unreal.AnimPoseSpaces.LOCAL),
                        APE.get_bone_pose(ref, str(bone), unreal.AnimPoseSpaces.WORLD))
            for bone in APE.get_bone_names(ref)}


def spike_axes(rest):
    """Tip bone name -> (outward unit x, outward unit y, firing order), read from where each bone sits."""
    names = sorted((name for name in rest if name.startswith(SPIKE_PREFIX)),
                   key=lambda name: int(name[len(SPIKE_PREFIX):]))
    axes = {}
    for order, name in enumerate(names):
        position = rest[name][1].translation
        length = math.hypot(position.x, position.y)
        axes[name] = (position.x / length, position.y / length, order)
    return axes


def finale_start(spikes):
    """The frame the sweep's last tip peaks on, which is where the finale takes over."""
    return LEAD_IN + STEP * (spikes - 1) + OUT_FRAMES


def frame_count(spikes):
    return finale_start(spikes) + FINALE_OUT + FINALE_HOLD + FINALE_BACK


def section_starts(spikes):
    return [0.0, LEAD_IN / float(FPS), (finale_start(spikes) + FINALE_OUT + FINALE_HOLD) / float(FPS)]


def smoothstep(alpha):
    return alpha * alpha * (3.0 - 2.0 * alpha)


def extension(frame, order):
    """0 at rest, 1 fully out — a fast push out then a slower retract, one tip after another."""
    elapsed = frame - LEAD_IN - order * STEP
    if elapsed <= 0.0 or elapsed >= OUT_FRAMES + BACK_FRAMES:
        return 0.0
    return smoothstep(elapsed / float(OUT_FRAMES) if elapsed < OUT_FRAMES
                      else 1.0 - (elapsed - OUT_FRAMES) / float(BACK_FRAMES))


def finale(frame, spikes):
    """0 to 1 for every tip at once — the sweep's last beat opens into one push out and a slower release."""
    elapsed = frame - finale_start(spikes)
    if elapsed <= 0.0:
        return 0.0
    if elapsed < FINALE_OUT:
        return smoothstep(elapsed / float(FINALE_OUT))
    if elapsed < FINALE_OUT + FINALE_HOLD:
        return 1.0
    released = (elapsed - FINALE_OUT - FINALE_HOLD) / float(FINALE_BACK)
    return 0.0 if released >= 1.0 else 1.0 - smoothstep(released)


def build_sequence(rest, axes, frames):
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(SKELETON_PATH))
    path = "{}/{}".format(ANIM_PACKAGE, SEQ_NAME)
    sequence = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else \
        unreal.AssetToolsHelpers.get_asset_tools().create_asset(SEQ_NAME, ANIM_PACKAGE, unreal.AnimSequence, factory)

    controller = sequence.get_editor_property("controller")
    controller.open_bracket("Build pike nova")
    controller.set_frame_rate(unreal.FrameRate(FPS, 1))
    controller.set_number_of_frames(unreal.FrameNumber(frames))
    for bone, (rest_local, _) in rest.items():
        positions, rotations, scales = [], [], []
        for frame in range(frames + 1):  # a sequence holds one more key than its frame count
            translation = unreal.Vector(rest_local.translation.x, rest_local.translation.y, rest_local.translation.z)
            if bone in axes:
                x, y, order = axes[bone]
                # Whichever is further out: the tip's own beat in the sweep, or the finale that takes every tip.
                offset = max(REACH * extension(frame, order), FINALE_REACH * finale(frame, len(axes)))
                translation.x += x * offset
                translation.y += y * offset
            positions.append(translation)
            rotations.append(rest_local.rotation)
            scales.append(unreal.Vector(rest_local.scale3d.x, rest_local.scale3d.y, rest_local.scale3d.z))
        # Unconditional and result ignored: the adder reports failure for an existing track, and the key setter
        # reports success whether or not a track is there.
        controller.add_bone_curve(bone)
        controller.set_bone_track_keys(bone, positions, rotations, scales)
    controller.close_bracket()
    unreal.EditorAssetLibrary.save_asset(path)
    return sequence


def build_montage(sequence, starts):
    factory = unreal.AnimMontageFactory()
    factory.set_editor_property("target_skeleton", sequence.get_skeleton())
    factory.set_editor_property("source_animation", sequence)  # the factory builds the slot track and its segment
    path = "{}/{}".format(ANIM_PACKAGE, MONTAGE_NAME)
    montage = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else \
        unreal.AssetToolsHelpers.get_asset_tools().create_asset(MONTAGE_NAME, ANIM_PACKAGE, unreal.AnimMontage, factory)

    util = unreal.get_default_object(unreal.GeoAnimBuilderUtil)
    util.set_montage_slot_segment(montage, sequence, "DefaultSlot")
    util.set_montage_sections(montage, [unreal.Name(n) for n in SECTION_NAMES], starts,
                              [unreal.Name(n) for n in SECTION_NEXT])
    unreal.EditorAssetLibrary.save_asset(path)
    return montage


def link_to_pattern(montage):
    """Only point the pattern at the montage once its sections are readable — the pattern jumps between them."""
    sections = [str(montage.get_section_name(i)) for i in range(montage.get_num_sections())]
    if not set(SECTION_NAMES).issubset(sections):
        LOG.append("NOT linked — montage sections are {}, need {}".format(sections, SECTION_NAMES))
        return
    pattern = unreal.load_asset(PATTERN_BP)
    unreal.get_default_object(pattern.generated_class()).set_editor_property("AnimMontage", montage)
    unreal.EditorAssetLibrary.save_loaded_asset(pattern)
    LOG.append("linked {} to {}".format(montage.get_name(), pattern.get_name()))


def report_weights(axes):
    """How far a vertex actually travels: the bone's travel scaled by that vertex's weight on it."""
    mesh = unreal.load_asset(MESH_PATH)
    modifier = unreal.SkinWeightModifier()
    modifier.set_skeletal_mesh(mesh)
    carried = {}
    for index in range(modifier.get_num_vertices()):
        for bone, weight in modifier.get_vertex_weights(index).items():
            if str(bone) in axes and weight > 0.001:
                carried.setdefault(str(bone), []).append((index, round(weight, 3)))
    for bone in sorted(carried, key=lambda name: axes[name][2]):
        moved = carried[bone]
        LOG.append("  %-18s drives %d vert(s) %s -> they travel %s of %.0f" % (
            bone, len(moved), moved, [round(REACH * w, 1) for _, w in moved], REACH))


def report_reach(sequence, axes, frames):
    """Evaluate the finished sequence — the track list reads a legacy path and reports empty for every sequence."""
    options = unreal.AnimPoseEvaluationOptions()
    order = sorted(axes, key=lambda name: axes[name][2])
    LOG.append("frame  " + "  ".join("%6s" % name[len(SPIKE_PREFIX):] for name in order))
    for frame in range(0, frames + 1, 2):
        pose = APE.get_anim_pose_at_time(sequence, frame / float(FPS), options)
        reach = []
        for name in order:
            position = APE.get_bone_pose(pose, name, unreal.AnimPoseSpaces.WORLD).translation
            reach.append(math.hypot(position.x, position.y))
        LOG.append("%5d  " % frame + "  ".join("%6.1f" % r for r in reach))


rest = reference_pose()
axes = spike_axes(rest)
if not axes:
    LOG.append("ABORTED: no bone named {}N in the skeleton".format(SPIKE_PREFIX))
else:
    frames = frame_count(len(axes))
    starts = section_starts(len(axes))
    sequence = build_sequence(rest, axes, frames)
    link_to_pattern(build_montage(sequence, starts))
    LOG.append("{} tip bones, firing order {}".format(
        len(axes), [name[len(SPIKE_PREFIX):] for name in sorted(axes, key=lambda n: axes[n][2])]))
    LOG.append("bearings {}".format([round(math.degrees(math.atan2(axes[n][1], axes[n][0])) % 360.0)
                                     for n in sorted(axes, key=lambda n: axes[n][2])]))
    LOG.append("sequence {:.3f}s over {} frames at {} fps".format(frames / float(FPS), frames, FPS))
    LOG.append("sections {} at {}".format(SECTION_NAMES, [round(s, 3) for s in starts]))
    report_weights(axes)
    report_reach(sequence, axes, frames)

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
