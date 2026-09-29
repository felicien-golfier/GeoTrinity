"""Boss montage sounds: every notify of each montage's sound score, and the curves bending the boss's fight loop
through it.

The hex boss intro's score is AI/Python/Audio/hex_intro_mech_score.py, its other montages'
AI/Python/Audio/hex_boss_montage_score.py, and the star boss's AI/Python/Audio/star_boss_score.py. One-shots on their
beats, each stem from where it starts, the loop sound notifies where a score puts them, and every curve of the score.
Run via mcp-unreal execute_script, after import_sound_waves.py. Set BOSSES to the bosses to (re)write. Re-runnable:
clears each montage's notify tracks and rewrites each curve first.
Report written to Saved/boss_montage_sounds.txt.
"""
import json
import traceback

import unreal

HEX = "/Game/Characters/Anim/HexBoss/"
STAR = "/Game/Characters/Anim/Star/"
AUDIO = unreal.Paths.project_dir() + "SourceArt/Audio/"
# Boss -> (montage, its cue list, the file whose "curves" it carries), the files under AUDIO
SCORES = {
    "Hex": [(HEX + "SK_HexBoss_Montage_Intro", "HexBossIntro/cues.json", "HexBossIntro/loop.json")] + [
        (HEX + "SK_HexBoss_Montage_" + name, "HexBossMontages/{}/cues.json".format(name),
         "HexBossMontages/{}/curves.json".format(name))
        for name in ["SweepBeam", "CarvingRay", "ConeSpray", "Bomb", "Turret", "Death"]],
    "Star": [(STAR + montage, "StarBoss/{}/cues.json".format(name), "StarBoss/{}/curves.json".format(name))
             for montage, name in [("SK_Star_Montage_Intro", "Intro"), ("SK_Star_Attack_Montage", "Attack"),
                                   ("SK_Star_Montage_DevastatingWave", "DevastatingWave"),
                                   ("SK_Star_PikeNova_Montage", "PikeNova"), ("SK_Star_Montage_Death", "Death")]]}
BOSSES = ["Hex", "Star"]
TRIMS = {"Hex": -4.0, "Star": 0.0}  # dB every one-shot of a boss's montages is trimmed by in game
REPORT = unreal.Paths.project_saved_dir() + "boss_montage_sounds.txt"


def put_score(montage, cues, curves, trim, toolkit):
    for index, cue in enumerate(cues):
        if "notify" in cue:
            toolkit["set_notify"](montage, cue["track"], cue["time"], getattr(unreal, cue["notify"]),
                                  cue.get("properties", {}), clear=index == 0)
        else:
            sound = unreal.load_asset("{}/{}".format(cue["package"], cue["sound"]))
            if not sound:
                raise RuntimeError("{} is not imported".format(cue["sound"]))

            toolkit["set_notify"](montage, cue["track"], cue["time"], unreal.AnimNotify_PlaySound,
                                  {"sound": sound, "volume_multiplier": cue["volume"] * 10.0 ** (trim / 20.0)},
                                  clear=index == 0)
    for name, keys in curves.items():
        toolkit["set_float_curve"](montage, name, keys)


def report(name, montage, curves, toolkit):
    LOG.append("== {}".format(name))
    for time, notify in toolkit["notify_events"](montage):
        if isinstance(notify, unreal.AnimNotify_PlaySound):
            LOG.append("{:.3f}s  {}  volume {:.2f}".format(time, notify.get_editor_property("sound").get_name(),
                                                           notify.get_editor_property("volume_multiplier")))
        else:
            LOG.append("{:.3f}s  {}".format(time, notify.get_class().get_name()))
    for curve in curves:
        if unreal.AnimationLibrary.does_curve_exist(montage, curve, unreal.RawCurveTrackTypes.RCT_FLOAT):
            times, values = unreal.AnimationLibrary.get_float_keys(montage, curve)
            LOG.append("curve {}: {} keys, {:.2f} to {:.2f}, {:.2f} at the end".format(
                curve, len(times), min(values), max(values), values[-1]))
        else:
            LOG.append("curve {}: absent".format(curve))


LOG = []
try:
    toolkit_path = unreal.Paths.project_dir() + "AI/Python/Anim/anim_sequence_authoring.py"
    toolkit = {}
    exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)

    for boss, path, cues_path, curves_path in [(boss, *score) for boss in BOSSES for score in SCORES[boss]]:
        montage = unreal.load_asset(path)
        with open(AUDIO + cues_path) as handle:
            cues = json.load(handle)
        with open(AUDIO + curves_path) as handle:
            curves = json.load(handle)["curves"]
        put_score(montage, cues, curves, TRIMS[boss], toolkit)
        unreal.EditorAssetLibrary.save_asset(path)
        report(montage.get_name(), montage, curves, toolkit)
except Exception:
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
