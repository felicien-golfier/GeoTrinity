"""Hex boss intro sounds: every notify of the score AI/Python/Audio/hex_intro_mech_score.py wrote, on the intro montage.

One-shots on their beats, and each stem from where it starts. Run via mcp-unreal execute_script, after
import_sound_waves.py. Re-runnable: clears the montage's notify tracks first.
Report written to Saved/hex_boss_intro_sounds.txt.
"""
import json
import traceback

import unreal

MONTAGE_PATH = "/Game/Characters/Anim/HexBoss/SK_HexBoss_Montage_Intro"
CUES = unreal.Paths.project_dir() + "SourceArt/Audio/HexBossIntro/cues.json"
REPORT = unreal.Paths.project_saved_dir() + "hex_boss_intro_sounds.txt"

LOG = []
try:
    toolkit_path = unreal.Paths.project_dir() + "AI/Python/Anim/anim_sequence_authoring.py"
    toolkit = {}
    exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)

    montage = unreal.load_asset(MONTAGE_PATH)
    with open(CUES) as handle:
        cues = json.load(handle)
    for index, cue in enumerate(cues):
        sound = unreal.load_asset("{}/{}".format(cue["package"], cue["sound"]))
        if not sound:
            raise RuntimeError("{} is not imported".format(cue["sound"]))

        toolkit["set_notify"](montage, cue["track"], cue["time"], unreal.AnimNotify_PlaySound,
                              {"sound": sound, "volume_multiplier": cue["volume"]}, clear=index == 0)
    unreal.EditorAssetLibrary.save_asset(MONTAGE_PATH)

    for time, notify in toolkit["notify_events"](montage):
        LOG.append("{:.3f}s  {}  volume {:.2f}".format(time, notify.get_editor_property("sound").get_name(),
                                                       notify.get_editor_property("volume_multiplier")))
except Exception:
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
