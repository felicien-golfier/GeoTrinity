# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy", "scipy"]
# ///
"""
Score the hex boss intro off its own motion: where each metal one-shot plays, and the stems that follow the rings.

Run outside the editor: uv run AI/Python/Audio/hex_intro_score.py
Needs Saved/bone_motion.json (AI/Python/Anim/dump_bone_motion.py on the intro sequence) and the one-shots from
metal_one_shots.py. Writes SourceArt/Audio/HexBossIntro: a stem per continuous sound, and cues.json — every notify
AI/Python/Anim/hex_boss_intro_sounds.py puts on the montage. The whole intro mixed goes to PREVIEW, for listening.
"""
import json
import pathlib

import numpy as np
from scipy.io import wavfile

PROJECT = pathlib.Path(__file__).resolve().parents[3]
MOTION = PROJECT / "Saved" / "bone_motion.json"
METAL = PROJECT / "SourceArt" / "Audio" / "Metal"
OUT_FOLDER = PROJECT / "SourceArt" / "Audio" / "HexBossIntro"
PREVIEW = PROJECT / "Saved" / "Audio" / "HexBossIntro" / "Draft" / "Intro_preview.wav"
METAL_PACKAGE = "/Game/Art/SFX/Metal"
STEM_PACKAGE = "/Game/Art/SFX/Enemy/HexBoss"
SAMPLE_RATE = 48000
FPS = 30
PEAK = 10.0 ** (-1.0 / 20.0)
TAIL_SECONDS = 0.8

WAKE = 6          # first frame the pieces shiver, from hex_boss_intro.py
STILL_BEFORE = 4  # frames a piece holds dead still before its landing
SPRING_FRAMES = 3  # frames a landed ring springs off its overshoot before it judders

# (bone, landing frame, its latch click, teeth on its ratchet, the ratchet's pitch, its clinks, its landing's volume)
# Larger rings click lower and carry more teeth, so all three tick at about the same rate for the same turn speed.
RINGS = [
    ("HexOuter", 40, "LockClick_Low", 48, 0.85, [1, 2], 0.70),
    ("HexMid", 66, "LockClick_Mid", 36, 1.00, [3, 4], 0.85),
    ("HexCore", 88, "LockClick_High", 24, 1.18, [5, 6], 1.00),
]
BODY = "Root"
CLICK_UNDER_CRACK = 0.45    # the latch click's volume against its crack
SHIVER_FULL = 9.0           # units of shiver at which a scattered piece clinks most
CLINK_CHANCE = 0.4          # per frame at that shiver
CLINK_VOLUME = (0.15, 0.45)  # from the faintest shiver to the fullest
BODY_SHAKE_FULL = 26.0      # units: a landing's kick, the rattle's loudest
TAPS_PER_REVERSAL = 2.0     # at full shake; one reversal of the shake per frame
GHOST_TICK = (10 ** (-28 / 20), 0.028, 0.064)  # a freewheel's second pawl: level, earliest and latest after a tick
STEM_VOLUMES = {"Ratchet": 0.35, "RattleBody": 0.5, "RattleRings": 0.2}


def load(name):
    rate, data = wavfile.read(METAL / f"SFX_Metal_{name}.wav")
    return data.astype(np.float64).mean(axis=1) / 32767.0


def pitched(sound, factor):
    """Played `factor` times faster: higher and shorter, as a smaller part of the same metal."""
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


def ratchet(yaw, times, start, teeth, pitch, rng):
    """A stem from `start`: one tick each time the ring turns past a tooth, and now and then a ghost tick after it."""
    ticks = [pitched(load(f"RatchetTick_{index}"), pitch) for index in range(1, 5)]
    tooth = np.floor(np.asarray(yaw) / (360.0 / teeth))
    crossings = [times[i] for i in range(1, len(times)) if tooth[i] != tooth[i - 1] and times[i] >= start]
    stem = np.zeros(int((times[-1] - start + TAIL_SECONDS) * SAMPLE_RATE))
    for index, time in enumerate(crossings):
        add(stem, ticks[rng.integers(len(ticks))], time - start, 10 ** (rng.normal(0, 1.7) / 20))
        gap = crossings[index + 1] - time if index + 1 < len(crossings) else np.inf
        ghost = rng.uniform(GHOST_TICK[1], GHOST_TICK[2])
        if ghost < gap - 0.01:
            add(stem, ticks[rng.integers(len(ticks))], time - start + ghost, GHOST_TICK[0])

    return stem, len(crossings)


def rattle(amount, start_frame, rng):
    """A stem from `start_frame`: the shake's reversals, each a few metal taps as loud as the shake is wide."""
    taps = [load(f"Tap_{index}") for index in range(1, 7)]
    stem = np.zeros(int(((len(amount) - start_frame) / FPS + TAIL_SECONDS) * SAMPLE_RATE))
    for frame in range(start_frame, len(amount)):
        for _ in range(rng.poisson(TAPS_PER_REVERSAL * amount[frame])):
            add(stem, pitched(taps[rng.integers(len(taps))], rng.uniform(0.8, 1.25)),
                (frame - start_frame) / FPS + rng.uniform(-0.008, 0.008) + 0.008, amount[frame] * rng.uniform(0.5, 1.0))

    return stem


def write_stem(name, stem):
    path = OUT_FOLDER / f"SFX_HexIntro_{name}.wav"
    stem = stem / np.abs(stem).max() * PEAK
    wavfile.write(path, SAMPLE_RATE, np.round(np.stack([stem, stem], axis=1) * 32767).astype(np.int16))
    return path


def cue(track, sound, package, time, volume, source):
    return {"track": track, "sound": sound.replace("SFX_", "SW_", 1), "package": package, "time": round(time, 4),
            "volume": round(volume, 3), "source": str(source.relative_to(PROJECT)).replace("\\", "/")}


def main():
    OUT_FOLDER.mkdir(parents=True, exist_ok=True)
    motion = json.loads(MOTION.read_text())
    rate, bones = motion["rate"], motion["bones"]
    times = np.asarray(motion["times"])
    rng = np.random.default_rng(3)
    cues = []

    for bone, land, click, teeth, pitch, clinks, volume in RINGS:
        cues.append(cue("Lock", "SFX_Metal_Crack", METAL_PACKAGE, land / FPS, volume, METAL / "SFX_Metal_Crack.wav"))
        cues.append(cue("Lock", f"SFX_Metal_{click}", METAL_PACKAGE, land / FPS, volume * CLICK_UNDER_CRACK,
                        METAL / f"SFX_Metal_{click}.wav"))

        amount = shake(frame_samples(bones[bone]["x"], rate), frame_samples(bones[bone]["y"], rate))
        for frame in range(WAKE, land - STILL_BEFORE):
            level = min(1.0, amount[frame] / SHIVER_FULL)
            if rng.uniform() < CLINK_CHANCE * level:
                name = f"SFX_Metal_Clink_{clinks[rng.integers(len(clinks))]}"
                cues.append(cue("Clink", name, METAL_PACKAGE, (frame + rng.uniform()) / FPS,
                                CLINK_VOLUME[0] + (CLINK_VOLUME[1] - CLINK_VOLUME[0]) * level, METAL / f"{name}.wav"))

        stem, count = ratchet(bones[bone]["yaw"], times, land / FPS, teeth, pitch, rng)
        path = write_stem(f"Ratchet_{bone.replace('Hex', '')}", stem)
        cues.append(cue("Ratchet", path.stem, STEM_PACKAGE, land / FPS, STEM_VOLUMES["Ratchet"], path))
        print(f"{bone}: {count} ratchet ticks")

    body = shake(frame_samples(bones[BODY]["x"], rate), frame_samples(bones[BODY]["y"], rate)) / BODY_SHAKE_FULL
    first = int(np.flatnonzero(body > 0.01)[0])
    path = write_stem("RattleBody", rattle(np.minimum(body, 1.0), first, rng))
    cues.append(cue("Rattle", path.stem, STEM_PACKAGE, first / FPS, STEM_VOLUMES["RattleBody"], path))

    judder = np.zeros(len(body))
    for bone, land, *_ in RINGS:
        amount = shake(frame_samples(bones[bone]["x"], rate), frame_samples(bones[bone]["y"], rate))
        judder[land + SPRING_FRAMES:] = np.maximum(judder[land + SPRING_FRAMES:], amount[land + SPRING_FRAMES:])
    first = RINGS[0][1] + SPRING_FRAMES
    path = write_stem("RattleRings", rattle(judder / judder.max(), first, rng))
    cues.append(cue("Rattle", path.stem, STEM_PACKAGE, first / FPS, STEM_VOLUMES["RattleRings"], path))

    cues.sort(key=lambda entry: entry["time"])
    (OUT_FOLDER / "cues.json").write_text(json.dumps(cues, indent=1) + "\n")
    print(f"{len(cues)} cues, {sum(entry['track'] == 'Clink' for entry in cues)} of them clinks")

    mix = np.zeros(int((times[-1] + TAIL_SECONDS * 2) * SAMPLE_RATE))
    for entry in cues:
        add(mix, wavfile.read(PROJECT / entry["source"])[1].astype(np.float64).mean(axis=1) / 32767.0, entry["time"],
            entry["volume"])
    PREVIEW.parent.mkdir(parents=True, exist_ok=True)
    mix = mix / np.abs(mix).max() * PEAK
    wavfile.write(PREVIEW, SAMPLE_RATE, np.round(np.stack([mix, mix], axis=1) * 32767).astype(np.int16))
    print(PREVIEW)


if __name__ == "__main__":
    main()
