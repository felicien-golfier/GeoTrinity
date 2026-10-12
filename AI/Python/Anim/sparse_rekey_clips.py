"""Rewrite every bone-track clip under FOLDERS as sparse keys, from what the clip plays now.

Nothing is regenerated: each clip's per-frame bone poses are sampled, written back through the toolkit's sparse writer
(`write_bone_keys`), and sampled again; a clip is saved only when every bone on every frame lands within the
tolerances below, so a clip retouched by hand, baked or reimported plays exactly as before with far fewer keys.
Montages, notifies and curves are not touched.

Run via mcp-unreal execute_script, or headless with the pythonscript commandlet. DRY_RUN = True samples, writes and
checks without saving. Re-runnable: a sparse clip rewrites to the same sparse keys. Report written to
AI/Output/sparse_rekey_clips.txt.
"""
import math
import traceback

import unreal

FOLDERS = ["/Game/Characters/Anim"]
DRY_RUN = False
# What a rewritten clip may differ from the old one by on any bone and frame.
TRANSLATION_TOLERANCE = 0.01  # units
ROTATION_TOLERANCE = 0.05  # degrees
SCALE_TOLERANCE = 0.0005

LIBRARY = unreal.AnimationLibrary
REPORT = unreal.Paths.project_dir() + "AI/Output/sparse_rekey_clips.txt"
TOOLKIT_PATH = unreal.Paths.project_dir() + "AI/Python/Anim/anim_sequence_authoring.py"


def sample(sequence, bones, frames):
    """{bone: [(translation, rotation, scale) per frame]}, copied out of the engine's transforms."""
    table = {}
    for bone in bones:
        table[bone] = []
        for frame in range(frames + 1):
            pose = LIBRARY.get_bone_pose_for_frame(sequence, bone, frame, False)
            table[bone].append((unreal.Vector(pose.translation.x, pose.translation.y, pose.translation.z),
                                unreal.Quat(pose.rotation.x, pose.rotation.y, pose.rotation.z, pose.rotation.w),
                                unreal.Vector(pose.scale3d.x, pose.scale3d.y, pose.scale3d.z)))
    return table


def key_count(util, sequence, bones):
    return sum(len(util.get_bone_channel_keys(sequence, bone, channel)[0]) for bone in bones for channel in range(9))


def worst_gap(before, after):
    """(translation, rotation in degrees, scale) of the largest difference over every bone and frame."""
    worst = [0.0, 0.0, 0.0]
    for bone in before:
        for old, new in zip(before[bone], after[bone]):
            dot = abs(old[1].x * new[1].x + old[1].y * new[1].y + old[1].z * new[1].z + old[1].w * new[1].w)
            worst[0] = max(worst[0], (old[0] - new[0]).length())
            worst[1] = max(worst[1], math.degrees(2.0 * math.acos(min(1.0, dot))))
            worst[2] = max(worst[2], (old[2] - new[2]).length())
    return worst


def rekey(toolkit, util, path, log):
    sequence = unreal.load_asset(path)
    bones = [str(bone) for bone in util.get_bone_track_names(sequence)]
    frames = LIBRARY.get_num_frames(sequence)
    if not bones or frames < 1:
        log.append("{}: no bone tracks, left alone".format(path))
        return

    before = sample(sequence, bones, frames)
    keys_before = key_count(util, sequence, bones)

    controller = sequence.get_editor_property("controller")
    controller.open_bracket("Sparse keys")
    for bone in bones:
        toolkit["write_bone_keys"](sequence, bone, [key[0] for key in before[bone]],
                                   [key[1] for key in before[bone]], [key[2] for key in before[bone]])
    controller.close_bracket()
    LIBRARY.finalize_bone_animation(sequence)

    gap = worst_gap(before, sample(sequence, bones, frames))
    fine = gap[0] <= TRANSLATION_TOLERANCE and gap[1] <= ROTATION_TOLERANCE and gap[2] <= SCALE_TOLERANCE
    if fine and not DRY_RUN:
        unreal.EditorAssetLibrary.save_asset(path)
    log.append("{}: {} bones, {} frames, keys {} -> {}, worst gap {:.4f} units {:.3f} deg {:.5f} scale: {}".format(
        path, len(bones), frames, keys_before, key_count(util, sequence, bones), gap[0], gap[1], gap[2],
        "DRY RUN" if fine and DRY_RUN else "saved" if fine else "NOT SAVED, differs"))


LOG = []
try:
    toolkit = {}
    exec(compile(open(TOOLKIT_PATH, encoding="utf-8").read(), TOOLKIT_PATH, "exec"), toolkit)
    util = unreal.get_default_object(unreal.GeoAnimBuilderUtil)
    for folder in FOLDERS:
        for asset_path in sorted(unreal.EditorAssetLibrary.list_assets(folder, True, False)):
            path = asset_path.split(".")[0]
            if isinstance(unreal.load_asset(path), unreal.AnimSequence):
                try:
                    rekey(toolkit, util, path, LOG)
                except Exception:
                    LOG.append("FAILED {}\n{}".format(path, traceback.format_exc()))
except Exception:
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
