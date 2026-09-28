"""Sample a sequence's bones in parent space and write them as JSON, for timing sounds and effects off the motion.

Run via mcp-unreal execute_script, or AI/Python/Runtime/run_via_bridge.py. Set the constants first.
Output: {"rate": samples per second, "times": [...], "bones": {bone: {"x": [...], "y", "z", "yaw", "scale": [...]}}},
yaw unwrapped so a ring's turn reads as one continuous angle.
"""
import json

import unreal

APE = unreal.AnimPoseExtensions

SEQUENCE_PATH = "/Game/Characters/Anim/HexBoss/SK_HexBoss_Sequence_Intro"
BONES = ["Root", "HexOuter", "HexMid", "HexCore"]
RATE = 120.0  # samples per second; a quarter of a frame at 30 fps
OUT = unreal.Paths.project_saved_dir() + "bone_motion.json"

sequence = unreal.load_asset(SEQUENCE_PATH)
length = sequence.get_editor_property("sequence_length")
options = unreal.AnimPoseEvaluationOptions()
times = [index / RATE for index in range(int(length * RATE) + 1)]
tracks = {bone: {"x": [], "y": [], "z": [], "yaw": [], "scale": []} for bone in BONES}
for time in times:
    pose = APE.get_anim_pose_at_time(sequence, time, options)
    for bone in BONES:
        local = APE.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.LOCAL)
        track = tracks[bone]
        yaw = local.rotation.rotator().yaw
        if track["yaw"]:
            yaw += 360.0 * round((track["yaw"][-1] - yaw) / 360.0)

        track["x"].append(local.translation.x)
        track["y"].append(local.translation.y)
        track["z"].append(local.translation.z)
        track["yaw"].append(yaw)
        track["scale"].append(local.scale3d.x)

with open(OUT, "w") as handle:
    json.dump({"sequence": SEQUENCE_PATH, "length": length, "rate": RATE, "times": times, "bones": tracks}, handle)
