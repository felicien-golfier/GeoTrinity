# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy", "scipy"]
# ///
"""
Score the hex boss intro off its own motion with the mechanical kit, the grammar of the mechanical intro reference:
a sub bed dipping before each hard entry, clattering flurries on the landings, gear trains per ring, a rattle and
rising hum through the gather, a dead freeze, the release.

Run outside the editor: uv run AI/Python/Audio/hex_intro_mech_score.py [ship]
Needs Saved/bone_motion.json (AI/Python/Anim/dump_bone_motion.py on the intro sequence). Writes the kit's sounds, a
stem per continuous sound and cues.json — every notify AI/Python/Anim/hex_boss_intro_sounds.py puts on the montage —
into DRAFT, or into SourceArt with `ship`. The whole intro mixed, once per hum version, always goes to DRAFT, for listening.
"""
import json
import pathlib
import sys

import numpy as np
from scipy.io import wavfile

sys.path.insert(0, str(pathlib.Path(__file__).parent))
import mech_kit as kit  # noqa: E402
from hex_intro_score import FPS, WAKE, STILL_BEFORE, GATHER, FREEZE, BLAST, frame_samples, shake, per_sample, cue  # noqa: E402

PROJECT = kit.PROJECT
MOTION = PROJECT / "Saved" / "bone_motion.json"
DRAFT = PROJECT / "Saved" / "Audio" / "HexBossIntro" / "Draft" / "Mech"
SHIPPED_STEMS = PROJECT / "SourceArt" / "Audio" / "HexBossIntro"
STEM_PACKAGE = "/Game/Art/SFX/Enemy/HexBoss"
TAIL_SECONDS = 1.0

# (bone, landing frame, size name, teeth on its gear, its impact's volume); larger rings carry more teeth
RINGS = [
    ("HexOuter", 40, "Low", 48, 0.8),
    ("HexMid", 66, "Mid", 36, 0.9),
    ("HexCore", 88, "High", 24, 1.0),
]
BODY = "Root"
RATTLE_SIZE = 1.3
SHIVER_FULL = 9.0    # units of shiver at which a scattered piece rattles most
STRAIN_FULL = 20.0   # units of body shake at which the body rattles most
CRUSH_FULL = 0.3     # body scale lost at which the pressure is full
FULL_SPEED = 640.0   # degrees a second at which a gear train runs flat out
# The bed in dB against its loudest, around each landing: (seconds from the landing, dB)
BED_DIP = [(-0.3, -8.0), (-0.2, -24.0), (-0.005, -24.0), (0.0, 0.0), (0.4, -6.0)]
CRUISE_FRAME = 135  # the engine's speed here is its cruise, held after the explosion and through the fight
# The engine's speed, 0 to 1, by frame: idling in the background, climbing with each landing, racing through the
# gather to full by the freeze, held flat out through the break and the explosion, easing back only after it to its
# cruise, where it stays and fades out past the montage's end as the fight loop takes over
ENGINE_SPEED = [(0, 0.0), (WAKE, 0.12), (40, 0.2), (66, 0.28), (88, 0.36), (GATHER, 0.42), (125, 0.6),
                (CRUISE_FRAME, kit.ENGINE_CRUISE), (FREEZE, 1.0), (BLAST + 8, 1.0), (175, kit.ENGINE_CRUISE)]
HUM_ON_MONTAGE = "Engine"  # or "Hum", the gather's hum alone; both are written and previewed
VOLUMES = {"Bed": 0.8, "Riser": 0.5, "Shiver": 0.35, "Whir": 0.35, "Grind": 0.5, "Rattle": 0.75, "Hum": 0.45,
           "Engine": 0.45, "AirStutter": 0.5, "Boom": 1.0, "Surge": 1.0}


def bed_level(end):
    """The bed's curve: swelling in, dipping before each landing, climbing through the gather, dead at the freeze."""
    keys = [(0.0, -40.0), (WAKE / FPS, -14.0)]
    for _, land, _, _, _ in RINGS:
        keys += [(land / FPS + offset, level) for offset, level in BED_DIP]
    keys += [(GATHER / FPS, -6.0), (FREEZE / FPS - 0.01, 0.0), (FREEZE / FPS, -80.0), (BLAST / FPS - 0.001, -80.0),
             (BLAST / FPS, 0.0), (BLAST / FPS + 1.5, -10.0), (end, -40.0)]
    times, levels = zip(*keys)
    return kit.decibels(np.interp(kit.timeline(kit.seconds(end)), times, levels))


def teeth(yaw, times, start, end, count):
    """Sample indices, from `start`, where the ring turns past a tooth — none while the machine is frozen."""
    tooth = np.floor(np.asarray(yaw) / (360.0 / count))
    crossed = [times[i] for i in range(1, len(times)) if tooth[i] != tooth[i - 1] and start <= times[i] < end]
    return [kit.seconds(time - start) for time in crossed if not FREEZE / FPS <= time < BLAST / FPS]


def main(ship):
    stem_folder = SHIPPED_STEMS if ship else DRAFT
    one_shot_folder = kit.SHIPPED if ship else DRAFT
    for folder in (stem_folder, one_shot_folder, DRAFT):
        folder.mkdir(parents=True, exist_ok=True)
    motion = json.loads(MOTION.read_text())
    rate, bones = motion["rate"], motion["bones"]
    times = np.asarray(motion["times"])
    frame_times = np.arange(len(frame_samples(times, rate))) / FPS
    end = times[-1] + TAIL_SECONDS
    shots = kit.write_all(one_shot_folder)

    def stem_cue(track, name, stem, time, volume):
        path = kit.save(stem_folder / f"SFX_HexIntro_{name}.wav", kit.normalised(stem))
        return cue(track, path.stem, STEM_PACKAGE, time, volume, path)

    def shot_cue(track, name, time, volume):
        return cue(track, shots[name].stem, kit.PACKAGE, time, volume, shots[name])

    cues = [stem_cue("Bed", "Bed", kit.drone(bed_level(end), kit.generator("Bed")), 0.0, VOLUMES["Bed"]),
            shot_cue("Hit", "Riser", WAKE / FPS, VOLUMES["Riser"])]
    for bone, land, size_name, count, volume in RINGS:
        piece = bones[bone]
        part = bone.replace("Hex", "")
        size = kit.SIZES[size_name]
        cues.append(shot_cue("Hit", f"Impact_{size_name}", land / FPS, volume))

        shiver = np.clip(shake(frame_samples(piece["x"], rate), frame_samples(piece["y"], rate)) / SHIVER_FULL, 0, 1)
        shiver[land - STILL_BEFORE:] = 0.0
        amount = per_sample(WAKE / FPS, (land - STILL_BEFORE) / FPS + 0.2, frame_times, shiver)
        cues.append(stem_cue("Shiver", f"Shiver_{part}", kit.rattle(amount, kit.generator(f"Shiver_{part}"), size),
                             WAKE / FPS, VOLUMES["Shiver"]))

        speed = np.abs(per_sample(land / FPS, end, times, np.gradient(np.asarray(piece["yaw"]), times)))
        drive = np.sqrt(np.clip(speed / FULL_SPEED, 0, 1))
        frozen = (kit.timeline(len(drive)) + land / FPS >= FREEZE / FPS) & (
            kit.timeline(len(drive)) + land / FPS < BLAST / FPS)
        drive[frozen] = 0.0
        gear = kit.whir(teeth(piece["yaw"], times, land / FPS, end, count), drive, size, kit.generator(f"Whir_{part}"))
        cues.append(stem_cue("Whir", f"Whir_{part}", gear, land / FPS, VOLUMES["Whir"]))

    body = bones[BODY]
    strain = np.clip(shake(frame_samples(body["x"], rate), frame_samples(body["y"], rate)) / STRAIN_FULL, 0, 1)
    crush = np.clip((1.0 - frame_samples(body["scale"], rate)) / CRUSH_FULL, 0, 1)
    strain[:GATHER] = crush[:GATHER] = 0.0
    strain[FREEZE:] = crush[FREEZE:] = 0.0
    amount = per_sample(GATHER / FPS, FREEZE / FPS, frame_times, np.maximum(strain, crush))
    pressure = per_sample(GATHER / FPS, FREEZE / FPS, frame_times, crush)
    cues.append(shot_cue("Hit", "Grind", GATHER / FPS, VOLUMES["Grind"]))
    cues.append(stem_cue("Rattle", "Rattle", kit.rattle(amount, kit.generator("Rattle"), RATTLE_SIZE), GATHER / FPS,
                         VOLUMES["Rattle"]))
    frames, speeds = zip(*ENGINE_SPEED)
    engine_time = kit.timeline(kit.seconds(end))
    handover = np.clip((end - engine_time) / TAIL_SECONDS, 0.0, 1.0)
    engine = kit.engine(np.interp(engine_time, np.asarray(frames) / FPS, speeds), kit.generator("Engine")) * handover
    hums = {"Hum": stem_cue("Hum", "Hum", kit.hum(pressure, kit.generator("Hum")), GATHER / FPS, VOLUMES["Hum"]),
            "Engine": stem_cue("Hum", "Engine", engine, 0.0, VOLUMES["Engine"])}
    stutter = wavfile.read(shots["AirStutter"])[1]
    cues.append(shot_cue("Air", "AirStutter", FREEZE / FPS - len(stutter) / kit.SAMPLE_RATE, VOLUMES["AirStutter"]))

    cues.append(shot_cue("Hit", "Boom", BLAST / FPS, VOLUMES["Boom"]))
    cues.append(shot_cue("Hit", "Surge", BLAST / FPS, VOLUMES["Surge"]))

    montage = sorted(cues + [hums[HUM_ON_MONTAGE]], key=lambda entry: entry["time"])
    (stem_folder / "cues.json").write_text(json.dumps(montage, indent=1) + "\n")
    print(f"{len(montage)} cues")

    for name, hum in hums.items():
        mix = np.zeros(kit.seconds(end + 3.0))
        for entry in cues + [hum]:
            sound = wavfile.read(PROJECT / entry["source"])[1].astype(np.float64).mean(axis=1) / 32767.0
            kit.add(mix, sound, kit.seconds(entry["time"]), entry["volume"])
        print(kit.save(DRAFT / f"Intro_preview_{name}.wav", kit.normalised(mix[:np.flatnonzero(mix)[-1] + 1])))


if __name__ == "__main__":
    main(sys.argv[1:] == ["ship"])
