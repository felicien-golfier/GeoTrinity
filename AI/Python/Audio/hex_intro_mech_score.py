# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy", "scipy"]
# ///
"""
Score the hex boss intro off its own motion with the mechanical kit, the grammar of the mechanical intro reference:
a sub bed dipping before each hard entry, clattering flurries on the landings, gear trains per ring, a rattle and
rising hum through the gather, a dead freeze, the release. The engine is the boss's fight loop itself: the montage's
first notify starts it and the montage's curves bend it to the intro's speed, so it runs on unbroken into the fight,
joined there by the outer ring's tick-tock.

Run outside the editor: uv run AI/Python/Audio/hex_intro_mech_score.py [ship]
Needs Saved/BoneMotion/Intro.json (AI/Python/Anim/dump_bone_motion.py). Writes the kit's sounds, a
stem per continuous sound, the fight whir loop, cues.json — every notify AI/Python/Anim/boss_montage_sounds.py puts
on the montage — and loop.json — the fight loop's layers and the curves bending them — into DRAFT, or into SourceArt
with `ship`. The whole intro mixed, once per hum version, always goes to DRAFT, for listening: the Loop version as
Unreal plays it, curves and pitch clamp included, running on into the fight.
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
MOTION = PROJECT / "Saved" / "BoneMotion" / "Intro.json"
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
# "Loop": the fight loop, from the montage's first frame. "Engine" or "Hum" (the gather's hum alone) put a stem on the
# montage instead. All three are previewed.
HUM_ON_MONTAGE = "Loop"
# The fight loop, played by the boss's UGeoLoopSoundComponent: the engine loop, the outer ring's tick-tock, and the
# gather's rattle, silent at rest for the other montages to raise while the boss shakes
PITCH_CURVE = "LoopSemitones"
ENGINE_CURVE = "LoopDecibels"
WHIR_CURVE = "WhirDecibels"
RATTLE_PITCH_CURVE = "RattleSemitones"
RATTLE_CURVE = "RattleDecibels"
START_NOTIFY = "GeoLoopSoundNotify"
CURVE_RATE = 60          # curve keys a second
SILENT = -80.0           # dB a curve holds a loop at to keep it out
MIN_PITCH = 0.25         # the project's GlobalMinPitchScale: Unreal plays any lower pitch at this one
WHIR_RING = "HexOuter"
WHIR_STEADY = (44, 104)  # frames the outer ring turns steadily after landing: the tick-tock the fight whir holds
WHIR_LOOP_TEETH = 16     # teeth in the fight whir loop, even so the strong and weak pawls alternate across its seam
WHIR_HANDOVER = 206      # frame the fight whir fades in from, as the outer ring slows back through its tick-tock
FIGHT_PREVIEW = 6.0      # seconds of fight after the intro in the Loop preview
FIGHT_ENGINE = (0.3, 0.3)  # the engine loop's pitch and volume through the fight, tuned by ear on the boss
FIGHT_GLIDE = (175, 205)   # frames the engine glides from the intro's cruise down to its fight pitch and volume
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


def steady_turn(bones, times):
    """Degrees a second the outer ring turns steadily after its landing: the turn the fight whir holds."""
    first, last = (int(np.searchsorted(times, frame / FPS)) for frame in WHIR_STEADY)
    yaw = bones[WHIR_RING]["yaw"]
    return abs(yaw[last] - yaw[first]) / (times[last] - times[first])


def fight_whir(bones, times):
    """The outer ring's tick-tock held as a seamless loop: its teeth at the steady turn after its landing, at that
    turn's drive, a whole number of teeth long."""
    _, _, size_name, count, _ = next(ring for ring in RINGS if ring[0] == WHIR_RING)
    speed = steady_turn(bones, times)
    length = kit.seconds(WHIR_LOOP_TEETH * 360.0 / count / speed)
    teeth_at = [round(index * length / WHIR_LOOP_TEETH) for index in range(WHIR_LOOP_TEETH)]
    drive = np.full(length, np.sqrt(min(speed / FULL_SPEED, 1.0)))
    return kit.whir(teeth_at, drive, kit.SIZES[size_name], kit.generator("HexFight_Whir_Loop"), wrap=True)


def fight_rattle():
    """The gather's rattle at its loudest, held as a seamless loop."""
    steady = np.ones(kit.seconds(kit.LOOP_SECONDS))
    return kit.rattle(steady, kit.generator("HexFight_Rattle_Loop"), RATTLE_SIZE, wrap=True)


def loudest(sound, window_seconds=0.5):
    """The RMS of `sound`'s loudest stretch."""
    window = kit.seconds(window_seconds)
    return max(rms(sound[start:start + window]) for start in range(0, len(sound) - window, window // 4))


def fight_curves(end, speed_at, intro_volume):
    """The curves the intro montage bends the fight loop with, as [seconds, value] keys: the engine loop lifted from
    its fight pitch and volume back to the intro's, where its pitch and level follow the engine's speed, gliding down
    to the fight's through FIGHT_GLIDE; and the whir held out until the outer ring slows back through its tick-tock.
    `intro_volume` is the engine loop's volume that plays as loud as the intro's engine did at its cruise."""
    assert FIGHT_GLIDE[1] <= WHIR_HANDOVER, "the whir shares the engine's pitch curve: glide before it comes in"
    cruise, _ = kit.engine_loop_speed()
    fight_pitch, fight_volume = FIGHT_ENGINE
    times = np.append(np.arange(0.0, end, 1.0 / CURVE_RATE), end)
    speed = speed_at(times)
    glide = np.clip((times * FPS - FIGHT_GLIDE[0]) / (FIGHT_GLIDE[1] - FIGHT_GLIDE[0]), 0.0, 1.0)
    intro = 1.0 - glide * glide * (3.0 - 2.0 * glide)
    handover = np.clip((times - WHIR_HANDOVER / FPS) / (end - WHIR_HANDOVER / FPS), 0.0, 1.0)
    floor = kit.decibels(SILENT)
    level = np.maximum(kit.engine_level(speed) / kit.engine_level(cruise), floor) * intro_volume / fight_volume
    values = {PITCH_CURVE: intro * 12.0 * np.log2(kit.engine_rate(speed) / kit.engine_rate(cruise) / fight_pitch),
              ENGINE_CURVE: intro * 20.0 * np.log10(level),
              WHIR_CURVE: 20.0 * np.log10(np.maximum(handover, floor))}
    return {name: [[round(float(time), 4), round(float(value), 3)] for time, value in zip(times, curve)]
            for name, curve in values.items()}


def played(loop, positions, curves, layer, weight=1.0):
    """`loop` as Unreal plays the layer while the montage carrying the curves is at `positions` seconds, one per sample:
    read round and round at the layer's pitch bent by its curve, clamped to MIN_PITCH, at the layer's volume times the
    gain its rest level and curve give. Each curve counts by the montage's blend `weight`; past the curves' end — the
    montage over — and for a curve they lack, it holds at 0. The drift is left out."""
    def follow(name):
        if name not in curves:
            return np.zeros(len(positions))
        keys = np.asarray(curves[name])
        return weight * np.interp(positions, keys[:, 0], keys[:, 1], right=0.0)

    rate = np.clip(layer["pitch"] * 2.0 ** (follow(layer["pitch_curve"]) / 12.0), MIN_PITCH, None)
    position = (np.cumsum(rate) - rate[0]) % len(loop)
    wrapped = np.append(loop, loop[0])
    gain = kit.decibels(layer["rest_decibels"] + follow(layer["volume_curve"]))
    return layer["volume"] * gain * np.interp(position, np.arange(len(wrapped)), wrapped)


def rms(sound):
    return np.sqrt(np.mean(sound ** 2))


def compare_engines(stem, loop):
    """How far the loop, bent by the curves, strays from the intro's engine stem: level and brightness per 100 ms,
    every half second."""
    window = kit.seconds(0.1)
    frequencies = np.fft.rfftfreq(window, 1.0 / kit.SAMPLE_RATE)
    for start in range(0, len(stem) - window, window * 5):
        spans = [sound[start:start + window] for sound in (stem, loop)]
        spectra = [np.abs(np.fft.rfft(span * np.hanning(window))) for span in spans]
        levels = [20.0 * np.log10(rms(span) + 1e-9) for span in spans]
        centroids = [(frequencies * spectrum).sum() / (spectrum.sum() + 1e-12) for spectrum in spectra]
        print(f"{start / kit.SAMPLE_RATE:5.1f}s  stem {levels[0]:6.1f} dB {centroids[0]:5.0f} Hz   "
              f"loop {levels[1]:6.1f} dB {centroids[1]:5.0f} Hz   {levels[1] - levels[0]:+5.1f} dB")


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
        if bone == WHIR_RING:
            steady = slice(kit.seconds((WHIR_STEADY[0] - land) / FPS), kit.seconds((WHIR_STEADY[1] - land) / FPS))
            steady_whir = VOLUMES["Whir"] * rms(kit.normalised(gear)[steady])

    body = bones[BODY]
    strain = np.clip(shake(frame_samples(body["x"], rate), frame_samples(body["y"], rate)) / STRAIN_FULL, 0, 1)
    crush = np.clip((1.0 - frame_samples(body["scale"], rate)) / CRUSH_FULL, 0, 1)
    strain[:GATHER] = crush[:GATHER] = 0.0
    strain[FREEZE:] = crush[FREEZE:] = 0.0
    amount = per_sample(GATHER / FPS, FREEZE / FPS, frame_times, np.maximum(strain, crush))
    pressure = per_sample(GATHER / FPS, FREEZE / FPS, frame_times, crush)
    cues.append(shot_cue("Hit", "Grind", GATHER / FPS, VOLUMES["Grind"]))
    rattle = kit.normalised(kit.rattle(amount, kit.generator("Rattle"), RATTLE_SIZE))
    cues.append(stem_cue("Rattle", "Rattle", rattle, GATHER / FPS, VOLUMES["Rattle"]))
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

    engine_loop = wavfile.read(shots["Engine_Loop"])[1][:, 0] / 32767.0
    whir_path = kit.save(stem_folder / "SFX_HexFight_Whir_Loop.wav", kit.normalised(fight_whir(bones, times)))
    whir_loop = wavfile.read(whir_path)[1][:, 0] / 32767.0
    rattle_path = kit.save(stem_folder / "SFX_HexFight_Rattle_Loop.wav", kit.normalised(fight_rattle()))
    rattle_loop = wavfile.read(rattle_path)[1][:, 0] / 32767.0
    # Both engine files sit at PEAK, so the loop's gain over the stem's is the ratio of their raw peaks
    intro_volume = VOLUMES["Engine"] * np.abs(kit.loops()["Engine_Loop"]).max() / np.abs(engine).max()
    curves = fight_curves(times[-1], lambda at: np.interp(at, np.asarray(frames) / FPS, speeds), intro_volume)
    fight_pitch, fight_volume = FIGHT_ENGINE
    # The rattle, raised to its rest level's full, plays as loud as the gather's did at its loudest
    layers = [{"sound": shots["Engine_Loop"].stem.replace("SFX_", "SW_", 1), "package": kit.PACKAGE,
               "volume": fight_volume, "pitch": fight_pitch, "pitch_curve": PITCH_CURVE, "volume_curve": ENGINE_CURVE,
               "rest_decibels": 0.0},
              {"sound": whir_path.stem.replace("SFX_", "SW_", 1), "package": STEM_PACKAGE,
               "volume": round(steady_whir / rms(whir_loop), 4), "pitch": 1.0, "pitch_curve": PITCH_CURVE,
               "volume_curve": WHIR_CURVE, "rest_decibels": 0.0},
              {"sound": rattle_path.stem.replace("SFX_", "SW_", 1), "package": STEM_PACKAGE,
               "volume": round(VOLUMES["Rattle"] * loudest(rattle) / rms(rattle_loop), 4), "pitch": 1.0,
               "pitch_curve": RATTLE_PITCH_CURVE, "volume_curve": RATTLE_CURVE, "rest_decibels": SILENT}]
    loop = {"layers": layers, "intro_volume": round(float(intro_volume), 4), "curves": curves}
    (stem_folder / "loop.json").write_text(json.dumps(loop, indent=1) + "\n")

    start = {"track": "Hum", "notify": START_NOTIFY, "time": 0.0}
    hum_cue = start if HUM_ON_MONTAGE == "Loop" else hums[HUM_ON_MONTAGE]
    montage = sorted(cues + [hum_cue], key=lambda entry: entry["time"])
    (stem_folder / "cues.json").write_text(json.dumps(montage, indent=1) + "\n")
    print(f"{len(montage)} cues")

    def mix_cues(entries, length):
        mix = np.zeros(length)
        for entry in entries:
            sound = wavfile.read(PROJECT / entry["source"])[1].astype(np.float64).mean(axis=1) / 32767.0
            kit.add(mix, sound, kit.seconds(entry["time"]), entry["volume"])
        return mix

    previews = {name: mix_cues(cues + [hum], kit.seconds(end + 3.0)) for name, hum in hums.items()}
    loop_length = kit.seconds(times[-1] + FIGHT_PREVIEW)
    engine_played, whir_played, rattle_played = (played(sound, kit.timeline(loop_length), curves, layer)
                                                 for sound, layer in zip((engine_loop, whir_loop, rattle_loop), layers))
    previews["Loop"] = mix_cues(cues, loop_length) + engine_played + whir_played + rattle_played
    gain = kit.PEAK / max(np.abs(mix).max() for mix in previews.values())  # one gain for all, so they compare as they play
    for name, mix in previews.items():
        print(kit.save(DRAFT / f"Intro_preview_{name}.wav", gain * mix[:np.flatnonzero(mix)[-1] + 1]))

    intro = kit.seconds(times[-1])
    engine_stem = wavfile.read(PROJECT / hums["Engine"]["source"])[1][:, 0] / 32767.0 * VOLUMES["Engine"]
    compare_engines(engine_stem[:intro], engine_played[:intro])


if __name__ == "__main__":
    main(sys.argv[1:] == ["ship"])
