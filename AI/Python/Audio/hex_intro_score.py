# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy", "scipy"]
# ///
"""
Score the hex boss intro off its own motion as a heavy machine: what plays where, and the stems that follow the parts.

Run outside the editor: uv run AI/Python/Audio/hex_intro_score.py [ship]
Needs Saved/BoneMotion/Intro.json (AI/Python/Anim/dump_bone_motion.py) and the cracks from
metal_one_shots.py. Writes the Machine one-shots, a stem per continuous sound and cues.json — every notify
AI/Python/Anim/boss_montage_sounds.py puts on the montage — into DRAFT, or into SourceArt with `ship`.
The whole intro mixed always goes to DRAFT, for listening.
"""
import json
import pathlib
import sys

import numpy as np
from scipy.io import wavfile

sys.path.insert(0, str(pathlib.Path(__file__).parent))
import heavy_machine as machine  # noqa: E402

PROJECT = pathlib.Path(__file__).resolve().parents[3]
MOTION = PROJECT / "Saved" / "BoneMotion" / "Intro.json"
CRACK = PROJECT / "SourceArt" / "Audio" / "Metal" / "SFX_Metal_Crack.wav"
DRAFT = PROJECT / "Saved" / "Audio" / "HexBossIntro" / "Draft" / "HeavyMachine"
PREVIEW = DRAFT / "Intro_preview.wav"
SHIPPED_ONE_SHOTS = PROJECT / "SourceArt" / "Audio" / "Machine"
SHIPPED_STEMS = PROJECT / "SourceArt" / "Audio" / "HexBossIntro"
METAL_PACKAGE = "/Game/Art/SFX/Metal"
MACHINE_PACKAGE = "/Game/Art/SFX/Machine"
STEM_PACKAGE = "/Game/Art/SFX/Enemy/HexBoss"
SAMPLE_RATE = machine.SAMPLE_RATE
FPS = 30
PEAK = 10.0 ** (-1.0 / 20.0)
TAIL_SECONDS = 0.8

# Beats, in frames, from hex_boss_intro.py
WAKE = 6          # the scattered pieces start to shiver
STILL_BEFORE = 4  # frames a piece holds dead still before its landing
GATHER = 104      # the body starts crushing and shaking
FREEZE = 145      # everything holds still
BLAST = 149       # the release

# (bone, landing frame, size name and size from heavy_machine.SIZES, teeth on its gear, its crack's volume)
# Larger rings sound lower and carry more teeth, so all three mesh at about the same rate for the same turn speed.
RINGS = [
    ("HexOuter", 40, "Low", 1.3, 48, 0.70),
    ("HexMid", 66, "Mid", 1.0, 36, 0.85),
    ("HexCore", 88, "High", 0.8, 24, 1.00),
]
BODY = "Root"
BODY_SIZE = 1.6
SHIVER_FULL = 9.0    # units of shiver at which a scattered piece strains most
STRAIN_FULL = 20.0   # units of body shake at which the body strains most
CRUSH_FULL = 0.3     # body scale lost at which the body rumbles most
HISS_DELAY = 0.07    # seconds from a landing to its seal venting
SLAM_UNDER_CRACK = 0.7
VOLUMES = {"Hiss": 0.2, "Tremor": 0.45, "Servo": 0.15, "Gears": 0.2, "Strain": 0.6, "Blast": 1.0}


def normalised(sound):
    return sound / np.abs(sound).max() * PEAK


def save(path, sound):
    wavfile.write(path, SAMPLE_RATE, np.round(np.stack([sound, sound], axis=1) * 32767).astype(np.int16))
    return path


def write(folder, name, sound):
    return save(folder / f"SFX_{name}.wav", sound)


def pitched(sound, factor):
    """Played `factor` times faster: higher and shorter, as a smaller part of the same kind."""
    return np.interp(np.arange(0, len(sound) - 1, factor), np.arange(len(sound)), sound)


def add(track, sound, time, gain):
    start = int(round(time * SAMPLE_RATE))
    end = min(len(track), start + len(sound))
    track[start:end] += gain * sound[:end - start]


def frame_samples(track, rate):
    return np.asarray(track)[::int(round(rate / FPS))]


def shake(x, y):
    """Per frame, how far a part alternates frame to frame — the third difference ignores any smooth motion under it."""
    def alternation(values):
        padded = np.concatenate([values[:1], values, values[-1:], values[-1:]])
        return np.abs(-padded[:-3] + 3 * padded[1:-2] - 3 * padded[2:-1] + padded[3:]) / 8.0

    return np.hypot(alternation(np.asarray(x)), alternation(np.asarray(y)))


def per_sample(start, end, times, values):
    """A motion curve resampled to audio from `start` to `end` seconds."""
    return np.interp(start + np.arange(int((end - start) * SAMPLE_RATE)) / SAMPLE_RATE, times, values)


def gears(yaw, times, start, teeth, size, thunks, rng):
    """A stem from `start`: one heavy tooth dropping into mesh each time the ring turns past a tooth."""
    tooth = np.floor(np.asarray(yaw) / (360.0 / teeth))
    crossings = [times[i] for i in range(1, len(times)) if tooth[i] != tooth[i - 1] and times[i] >= start]
    stem = np.zeros(int((times[-1] - start + TAIL_SECONDS) * SAMPLE_RATE))
    for time in crossings:
        add(stem, pitched(thunks[rng.integers(len(thunks))], 1.0 / size), time - start, 10 ** (rng.normal(0, 1.7) / 20))

    return stem, len(crossings)


def cue(track, sound, package, time, volume, source):
    return {"track": track, "sound": sound.replace("SFX_", "SW_", 1), "package": package, "time": round(time, 4),
            "volume": round(volume, 3), "source": str(source.relative_to(PROJECT)).replace("\\", "/")}


def main(ship):
    one_shot_folder, stem_folder = (SHIPPED_ONE_SHOTS, SHIPPED_STEMS) if ship else (DRAFT, DRAFT)
    for folder in (one_shot_folder, stem_folder, DRAFT):
        folder.mkdir(parents=True, exist_ok=True)
    motion = json.loads(MOTION.read_text())
    rate, bones = motion["rate"], motion["bones"]
    times = np.asarray(motion["times"])
    frame_times = np.arange(len(frame_samples(times, rate))) / FPS
    end = times[-1] + TAIL_SECONDS
    shots, paths = {}, {}
    for name, sound in machine.one_shots().items():
        shots[name] = normalised(sound)
        paths[name] = write(one_shot_folder, f"Machine_{name}", shots[name])
    thunks = [shots[f"GearThunk_{index + 1}"] for index in range(machine.GEAR_THUNK_VARIANTS)]

    def stem_cue(track, name, stem, time, volume):
        path = write(stem_folder, f"HexIntro_{name}", normalised(stem))
        return cue(track, path.stem, STEM_PACKAGE, time, volume, path)

    def shot_cue(track, name, time, volume):
        return cue(track, paths[name].stem, MACHINE_PACKAGE, time, volume, paths[name])

    cues = []
    for bone, land, size_name, size, teeth, volume in RINGS:
        piece = bones[bone]
        part = bone.replace("Hex", "")
        cues.append(cue("Lock", CRACK.stem, METAL_PACKAGE, land / FPS, volume, CRACK))
        cues.append(shot_cue("Lock", f"Slam_{size_name}", land / FPS, volume * SLAM_UNDER_CRACK))
        cues.append(shot_cue("Lock", "Hiss", land / FPS + HISS_DELAY, VOLUMES["Hiss"]))

        shiver = np.clip(shake(frame_samples(piece["x"], rate), frame_samples(piece["y"], rate)) / SHIVER_FULL, 0, 1)
        shiver[land - STILL_BEFORE:] = 0.0
        amount = per_sample(WAKE / FPS, (land - STILL_BEFORE) / FPS + TAIL_SECONDS, frame_times, shiver)
        tremor = machine.groan(amount, size, machine.generator(f"Tremor_{part}")) + 0.3 * machine.hum(amount, size)
        cues.append(stem_cue("Tremor", f"Tremor_{part}", tremor, WAKE / FPS, VOLUMES["Tremor"]))

        speed = per_sample(land / FPS, end, times, np.gradient(np.asarray(piece["yaw"]), times))
        cues.append(stem_cue("Servo", f"Servo_{part}", machine.servo(speed, size, machine.generator(f"Servo_{part}")),
                             land / FPS, VOLUMES["Servo"]))

        stem, count = gears(piece["yaw"], times, land / FPS, teeth, size, thunks, machine.generator(f"Gears_{part}"))
        cues.append(stem_cue("Gears", f"Gears_{part}", stem, land / FPS, VOLUMES["Gears"]))
        print(f"{bone}: {count} gear teeth")

    body = bones[BODY]
    strain = np.clip(shake(frame_samples(body["x"], rate), frame_samples(body["y"], rate)) / STRAIN_FULL, 0, 1)
    crush = np.clip((1.0 - frame_samples(body["scale"], rate)) / CRUSH_FULL, 0, 1)
    strain[:GATHER] = crush[:GATHER] = 0.0
    strain[FREEZE:] = crush[FREEZE:] = 0.0
    amount = per_sample(GATHER / FPS, FREEZE / FPS + TAIL_SECONDS, frame_times, strain)
    pressure = per_sample(GATHER / FPS, FREEZE / FPS + TAIL_SECONDS, frame_times, crush)
    rng = machine.generator("Strain")
    cues.append(stem_cue("Strain", "Strain", machine.groan(np.maximum(amount, pressure), BODY_SIZE, rng)
                         + 0.6 * machine.rumble(np.maximum(amount, pressure), rng), GATHER / FPS, VOLUMES["Strain"]))
    cues.append(shot_cue("Lock", "Blast", BLAST / FPS, VOLUMES["Blast"]))

    cues.sort(key=lambda entry: entry["time"])
    (stem_folder / "cues.json").write_text(json.dumps(cues, indent=1) + "\n")
    print(f"{len(cues)} cues")

    mix = np.zeros(int((end + 3.0) * SAMPLE_RATE))
    for entry in cues:
        add(mix, wavfile.read(PROJECT / entry["source"])[1].astype(np.float64).mean(axis=1) / 32767.0, entry["time"],
            entry["volume"])
    print(save(PREVIEW, normalised(mix[:np.flatnonzero(mix)[-1] + 1])))


if __name__ == "__main__":
    main(sys.argv[1:] == ["ship"])
