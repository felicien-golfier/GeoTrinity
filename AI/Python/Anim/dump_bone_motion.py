"""Sample montages' bones in parent space and write them as JSON, for timing sounds and effects off the motion.

Run via mcp-unreal execute_script, or AI/Python/Runtime/run_via_bridge.py. Set the constants first.
Output, one file per montage under OUT: {"rate": samples per second, "times": [...], "bones": {bone: {"x": [...], "y",
"z", "yaw", "scale": [...]}}, "rest": {bone: {"x", "y", "z", "yaw", "scale"}}, "sections": [[name, start seconds, next
section or "None"]], "blend_in", "blend_out"}, over the montage's own time, yaw unwrapped so a turn reads as one
continuous angle.
"""
import json
import os
import traceback

import unreal

APE = unreal.AnimPoseExtensions

HEX = "/Game/Characters/Anim/HexBoss/"
STAR = "/Game/Characters/Anim/Star/"
HEX_BONES = ["Root", "HexOuter", "HexMid", "HexCore"]
STAR_BONES = ["Root", "apexes_outside", "apexes_inside"] + ["apexe_outside_{}".format(index) for index in range(1, 9)]
# (file under OUT, montage, bones, its sequences as (sequence, section it starts on)): a montage samples as its
# reference pose, so each sequence is sampled instead, played whole from its section to the next sequence's
MONTAGES = [("{}.json".format(name), HEX + "SK_HexBoss_Montage_" + name, HEX_BONES,
             [(HEX + "SK_HexBoss_Sequence_" + name, None)])
            for name in ["Intro", "SweepBeam", "CarvingRay", "ConeSpray", "Bomb", "Turret", "Death"]] + [
    ("Star/Intro.json", STAR + "SK_Star_Montage_Intro", STAR_BONES, [(STAR + "SK_Star_Sequence_Intro", None)]),
    ("Star/Attack.json", STAR + "SK_Star_Attack_Montage", STAR_BONES,
     [(STAR + "SK_Star_Sequence_Start", None), (STAR + "SK_Star_Sequence_Loop", "InternalFireLoop"),
      (STAR + "SK_Star_Sequence_End", "End")]),
    ("Star/DevastatingWave.json", STAR + "SK_Star_Montage_DevastatingWave", STAR_BONES,
     [(STAR + "SK_Star_Sequence_DevastatingWave", None)]),
    ("Star/PikeNova.json", STAR + "SK_Star_PikeNova_Montage", STAR_BONES,
     [(STAR + "SK_Star_Sequence_SpikeNova", None)]),
    ("Star/Death.json", STAR + "SK_Star_Montage_Death", STAR_BONES, [(STAR + "SK_Star_Sequence_Death", None)])]
RATE = 120.0  # samples per second; a quarter of a frame at 30 fps
OUT = unreal.Paths.project_saved_dir() + "BoneMotion/"
REPORT = OUT + "report.txt"


def fields(transform):
    return {"x": transform.translation.x, "y": transform.translation.y, "z": transform.translation.z,
            "yaw": transform.rotation.rotator().yaw, "scale": transform.scale3d.x}


def spans(montage, sequences, section_table):
    """(sequence, montage seconds it starts at, and ends at) for each sequence, in order."""
    starts = {name: start for name, start, _ in section_table}
    begins = [0.0 if section is None else starts[section] for _, section in sequences]
    ends = begins[1:] + [montage.get_play_length()]
    return [(unreal.load_asset(path), begin, end) for (path, _), begin, end in zip(sequences, begins, ends)]


def sample(length, played, bones):
    options = unreal.AnimPoseEvaluationOptions()
    times = [index / RATE for index in range(int(length * RATE) + 1)]
    tracks = {bone: {"x": [], "y": [], "z": [], "yaw": [], "scale": []} for bone in bones}
    for time in times:
        sequence, begin, end = next(span for span in reversed(played) if span[1] <= time)
        at = min(time - begin, end - begin) * sequence.get_play_length() / (end - begin)
        pose = APE.get_anim_pose_at_time(sequence, at, options)
        for bone in bones:
            track = tracks[bone]
            value = fields(APE.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.LOCAL))
            if track["yaw"]:
                value["yaw"] += 360.0 * round((track["yaw"][-1] - value["yaw"]) / 360.0)
            for field, number in value.items():
                track[field].append(number)
    return times, tracks


LOG = []
util = unreal.get_default_object(unreal.GeoAnimBuilderUtil)
for out_file, montage_path, bones, sequences in MONTAGES:
    try:
        montage = unreal.load_asset(montage_path)
        section_table = [[str(section), round(start, 4), str(following)] for section, start, following in zip(
            *util.get_montage_sections(montage))]  # sections are unreachable from Python but through the shim
        played = spans(montage, sequences, section_table)
        length = montage.get_play_length()
        times, tracks = sample(length, played, bones)
        reference = APE.get_reference_pose(played[0][0].get_editor_property("skeleton"))
        rest = {bone: fields(APE.get_bone_pose(reference, bone, unreal.AnimPoseSpaces.LOCAL)) for bone in bones}
        motion = {"montage": montage_path, "length": length, "rate": RATE, "times": times, "bones": tracks,
                  "rest": rest, "sections": section_table,
                  "blend_in": montage.get_editor_property("blend_in").get_editor_property("blend_time"),
                  "blend_out": montage.get_editor_property("blend_out").get_editor_property("blend_time")}
        os.makedirs(os.path.dirname(OUT + out_file), exist_ok=True)
        with open(OUT + out_file, "w") as handle:
            json.dump(motion, handle)
        LOG.append("{}: {:.3f}s, sections {}, sequences {}, blend {:.2f}/{:.2f}".format(
            out_file, length, section_table,
            ["{} x{:.2f} from {:.3f}s".format(sequence.get_name(), sequence.get_play_length() / (end - begin), begin)
             for sequence, begin, end in played], motion["blend_in"], motion["blend_out"]))
    except Exception:
        LOG.append("{}: {}".format(out_file, traceback.format_exc()))

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
