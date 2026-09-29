"""Boss fight loops: the layers of each boss score's loop.json on the boss's UGeoLoopSoundComponent.

Each layer's sound, volume, curves and rest level come from the loop.json its score writes: the hex boss's from
AI/Python/Audio/hex_intro_mech_score.py, the star boss's from AI/Python/Audio/star_boss_score.py. The drift is tuned by
ear on the boss, so a layer already there keeps its own; a new one takes the score's, or the component's default. A
boss is never the local player's avatar, so every layer plays at its full volume on other machines too. Run via
mcp-unreal execute_script, after import_sound_waves.py. Re-runnable: replaces each boss's whole Loops array.
Report written to Saved/boss_fight_loop.txt.
"""
import json
import traceback

import unreal

AUDIO = unreal.Paths.project_dir() + "SourceArt/Audio/"
# (boss Blueprint, its score's loop.json under AUDIO, dB every layer's volume is trimmed by in game)
BOSSES = [("/Game/Characters/Enemies/HEX/BP_HexBoss", "HexBossIntro/loop.json", -4.0),
          ("/Game/Characters/Enemies/Star/BP_StarBoss", "StarBoss/loop.json", 0.0)]
REPORT = unreal.Paths.project_saved_dir() + "boss_fight_loop.txt"


def put_loops(blueprint_path, layers, trim):
    blueprint = unreal.load_asset(blueprint_path)
    component = unreal.get_default_object(blueprint.generated_class()).get_editor_property("loop_sound_component")
    tuned = component.get_editor_property("loops")

    loops = []
    for index, layer in enumerate(layers):
        sound = unreal.load_asset("{}/{}".format(layer["package"], layer["sound"]))
        if not sound:
            raise RuntimeError("{} is not imported".format(layer["sound"]))

        entry = unreal.GeoSoundEntry()
        entry.set_editor_property("sound", sound)
        entry.set_editor_property("volume", layer["volume"] * 10.0 ** (trim / 20.0))
        entry.set_editor_property("random_pitch_multiplier_range", unreal.Vector2D(layer["pitch"], layer["pitch"]))
        entry.set_editor_property("other_machines_volume_multiplier", 1.0)
        loop = unreal.GeoLoopSound()
        loop.set_editor_property("sound", entry)
        loop.set_editor_property("pitch_curve", layer["pitch_curve"])
        loop.set_editor_property("volume_curve", layer["volume_curve"])
        loop.set_editor_property("rest_decibels", layer["rest_decibels"])
        if index < len(tuned):
            loop.set_editor_property("drift_semitones", tuned[index].get_editor_property("drift_semitones"))
            loop.set_editor_property("drift_period", tuned[index].get_editor_property("drift_period"))
        elif "drift_semitones" in layer:
            loop.set_editor_property("drift_semitones", layer["drift_semitones"])
            loop.set_editor_property("drift_period", layer["drift_period"])
        loops.append(loop)

    component.set_editor_property("loops", loops)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_asset(blueprint_path, only_if_is_dirty=False)
    return unreal.get_default_object(blueprint.generated_class()).get_editor_property("loop_sound_component")


LOG = []
for blueprint_path, loop_path, trim in BOSSES:
    try:
        with open(AUDIO + loop_path) as handle:
            component = put_loops(blueprint_path, json.load(handle)["layers"], trim)
        LOG.append("== {}".format(blueprint_path))
        for loop in component.get_editor_property("loops"):
            entry = loop.get_editor_property("sound")
            LOG.append("{}  volume {:.4f}  pitch {}  drift {} st / {} s  curves {} {}  rest {} dB".format(
                entry.get_editor_property("sound").get_name(), entry.get_editor_property("volume"),
                entry.get_editor_property("random_pitch_multiplier_range"), loop.get_editor_property("drift_semitones"),
                loop.get_editor_property("drift_period"), loop.get_editor_property("pitch_curve"),
                loop.get_editor_property("volume_curve"), loop.get_editor_property("rest_decibels")))
    except Exception:
        LOG.append("{}: {}".format(blueprint_path, traceback.format_exc()))

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
