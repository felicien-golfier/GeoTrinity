"""Trims the silence before the first sound off every Sound Wave at the root of SFX_FOLDER, reimporting each in place.

Run via mcp-unreal execute_script. Re-runnable: a wave already starting within PRE_ROLL_SECONDS is left alone.
Looping waves are skipped, since cutting their head changes the loop. The reimport keeps the asset's own settings;
the report flags any that moved anyway. Report written to Saved/trim_sound_start_silence.txt.
"""
import array
import os
import traceback
import wave

import unreal

SFX_FOLDER = "/Game/Art/SFX"
ONSET_DB_BELOW_PEAK = 40.0
PRE_ROLL_SECONDS = 0.001
KEPT_SETTINGS = ["sound_class_object", "attenuation_settings", "concurrency_set", "sound_submix_object", "volume",
                 "pitch", "looping"]
SAVED_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())
WORK_FOLDER = SAVED_DIR + "TrimSoundStartSilence/"
REPORT = SAVED_DIR + "trim_sound_start_silence.txt"


def export_wav(sound, filename):
    task = unreal.AssetExportTask()
    task.object = sound
    task.filename = filename
    task.automated = True
    task.replace_identical = True
    task.prompt = False
    return unreal.Exporter.run_asset_export_task(task)


def trim_start(source, destination):
    """Writes source minus its leading silence to destination and returns the milliseconds cut, 0 when none."""
    with wave.open(source, "rb") as reader:
        params = reader.getparams()
        if params.sampwidth != 2:
            raise ValueError("{}-byte samples, only 16-bit handled".format(params.sampwidth))
        samples = array.array("h", reader.readframes(params.nframes))

    peak = max(max(samples), -min(samples))
    threshold = peak * 10 ** (-ONSET_DB_BELOW_PEAK / 20)
    onset_frame = next(index for index, sample in enumerate(samples) if abs(sample) > threshold) // params.nchannels
    pre_roll_frames = int(params.framerate * PRE_ROLL_SECONDS)
    first_frame = onset_frame - pre_roll_frames
    if first_frame <= 0:
        return 0.0

    trimmed = samples[first_frame * params.nchannels:]
    for frame in range(pre_roll_frames):
        for channel in range(params.nchannels):
            index = frame * params.nchannels + channel
            trimmed[index] = int(trimmed[index] * frame / pre_roll_frames)

    with wave.open(destination, "wb") as writer:
        writer.setparams(params)
        writer.writeframes(trimmed.tobytes())
    return first_frame * 1000.0 / params.framerate


def reimport(filename, package):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", filename)
    task.set_editor_property("destination_path", package.rsplit("/", 1)[0])
    task.set_editor_property("destination_name", package.rsplit("/", 1)[1])
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", False)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])


def trim_sound(package):
    sound = unreal.load_asset(package)
    if sound.get_editor_property("looping"):
        return "skipped, looping"

    name = package.rsplit("/", 1)[1]
    exported = WORK_FOLDER + name + ".wav"
    trimmed = WORK_FOLDER + name + "_Trimmed.wav"
    if not export_wav(sound, exported):
        return "EXPORT FAILED"

    cut_ms = trim_start(exported, trimmed)
    if cut_ms == 0.0:
        return "already starts on its sound"

    settings_before = [sound.get_editor_property(setting) for setting in KEPT_SETTINGS]
    duration_before = sound.get_editor_property("duration")
    reimport(trimmed, package)
    sound = unreal.load_asset(package)
    moved = ["{} {} -> {}".format(setting, before, sound.get_editor_property(setting))
             for setting, before in zip(KEPT_SETTINGS, settings_before) if sound.get_editor_property(setting) != before]
    return "cut {:.1f} ms, {:.3f}s -> {:.3f}s{}".format(
        cut_ms, duration_before, sound.get_editor_property("duration"),
        ", SETTINGS MOVED: " + ", ".join(moved) if moved else "")


LOG = []
try:
    os.makedirs(WORK_FOLDER, exist_ok=True)
    assets = unreal.AssetRegistryHelpers.get_asset_registry().get_assets_by_path(SFX_FOLDER, recursive=False)
    packages = sorted({str(asset.package_name) for asset in assets
                       if str(asset.asset_class_path.asset_name) == "SoundWave"})
    for package in packages:
        try:
            LOG.append("{}: {}".format(package, trim_sound(package)))
        except Exception as error:
            LOG.append("{}: FAILED {}".format(package, error))
except Exception:
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
