"""Freeze clips no script can rebuild into SourceArt/Anim/Baked, so re-running the script that made them keeps them.

A clip retouched by hand, or authored on a rig that has changed since, plays something its script's procedure no
longer gives. Each one in CLIPS has every bone's key on every frame of its built data written to
<sequence name>.json, and each montage playing it has its slot, sections and played length written to
<montage name>.json. The toolkit's write_bone_tracks and build_montage write from those files whenever they exist,
so every script picks the bake up without a change of its own; deleting a file hands that clip back to its script.

Run via mcp-unreal execute_script after a clip is retouched outside its script, then check every script against
the live clips with verify_badge_scripts.py. Report written to AI/Output/bake_clips.txt.
"""
import json
import os
import traceback

import unreal

LIBRARY = unreal.AnimationLibrary
BAKED_FOLDER = unreal.Paths.project_dir() + "SourceArt/Anim/Baked/"
REPORT = unreal.Paths.project_dir() + "AI/Output/bake_clips.txt"
ANIM_FOLDER = "/Game/Characters/Anim/ClassBadge"

CLIPS = [
    # Guillaume's, retouched outside the scripts and reimported on a 275-frame timeline
    "Circle/SK_CircleBadge_Sequence_Idle",
    "Circle/SK_CircleBadge_Sequence_MoiraBeam",
    "Circle/SK_CircleBadge_Sequence_ChargeOrbit",
    "Circle/SK_CircleBadge_Sequence_Deploy",
    # Authored on the badge before its depth was halved, then carried over by rerig_class_badges.py
    "Circle/SK_CircleBadge_Sequence_Rez",
    "Square/SK_SquareBadge_Sequence_Rez",
    "Triangle/SK_TriangleBadge_Sequence_Rez",
    "Triangle/SK_TriangleBadge_Sequence_Deploy",
    "Triangle/SK_TriangleBadge_Sequence_FireFormation",
    "Triangle/SK_TriangleBadge_Sequence_FireRatchet",
]
DIGITS = 6

LOG = []


def write(name, data):
    with open(BAKED_FOLDER + name + ".json", "w") as handle:
        json.dump(data, handle, separators=(",", ":"))


def key(transform):
    """One bone's key as [tx, ty, tz, qx, qy, qz, qw, sx, sy, sz]."""
    t, q, s = transform.translation, transform.rotation, transform.scale3d
    return [round(value, DIGITS) for value in (t.x, t.y, t.z, q.x, q.y, q.z, q.w, s.x, s.y, s.z)]


def bake_sequence(sequence):
    frames = LIBRARY.get_num_frames(sequence)
    fps = int(round(frames / sequence.get_editor_property("sequence_length")))
    reference = unreal.AnimPoseExtensions.get_reference_pose(sequence.get_skeleton())
    bones = [str(bone) for bone in unreal.AnimPoseExtensions.get_bone_names(reference)]
    write(sequence.get_name(), {"fps": fps, "frames": frames,
                                "bones": {bone: [key(LIBRARY.get_bone_pose_for_frame(sequence, bone, frame, False))
                                                 for frame in range(frames + 1)] for bone in bones}})
    LOG.append("{}: {} frames at {} fps, bones {}".format(sequence.get_name(), frames, fps, bones))


def bake_montage(montage):
    """Its single slot track's slot, its sections and how much of its sequence it plays from the start."""
    util = unreal.get_default_object(unreal.GeoAnimBuilderUtil)
    names, starts, following = util.get_montage_sections(montage)
    slots = [str(slot) for slot in LIBRARY.get_montage_slot_names(montage)]
    if len(slots) != 1:
        raise RuntimeError("{} has {} slot tracks; only one can be baked".format(montage.get_name(), len(slots)))
    layout = {"slot": slots[0], "play_length": round(montage.get_editor_property("sequence_length"), DIGITS),
              "sections": [[str(n), round(s, DIGITS), str(f)] for n, s, f in zip(names, starts, following)]}
    write(montage.get_name(), layout)
    LOG.append("  {}: {}".format(montage.get_name(), layout))


def montages_playing(sequence_path):
    referencers = unreal.EditorAssetLibrary.find_package_referencers_for_asset(sequence_path, False)
    # A verification sandbox holds copies playing the live sequences too.
    return [asset for asset in (unreal.load_asset(str(package)) for package in referencers
                                if str(package).startswith(ANIM_FOLDER))
            if isinstance(asset, unreal.AnimMontage)]


try:
    os.makedirs(BAKED_FOLDER, exist_ok=True)
    for clip in CLIPS:
        path = "{}/{}".format(ANIM_FOLDER, clip)
        bake_sequence(unreal.load_asset(path))
        for playing in montages_playing(path):
            bake_montage(playing)
except Exception:
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
