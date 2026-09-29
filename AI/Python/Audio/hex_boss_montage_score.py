# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy", "scipy"]
# ///
"""
Score the hex boss's ability and death montages: one-shots from the kits on their beats, and the curves bending the
boss's fight loop through each. The engine spools up with the wind-up, drops out while the boss holds dead still,
comes back on the hit, runs through the live phase and spools back down to the fight's; the whir follows how fast the
rings turn, the rattle how hard the boss shakes. Every curve starts and ends on the fight's own loop, except the
death's, whose blast cuts the loop.

Run outside the editor: uv run AI/Python/Audio/hex_boss_montage_score.py [ship]
Needs Saved/BoneMotion/<montage>.json (AI/Python/Anim/dump_bone_motion.py) and the fight loop's layers
(SourceArt/Audio/HexBossIntro/loop.json, from hex_intro_mech_score.py). Writes each montage's cues.json and
curves.json — what AI/Python/Anim/boss_montage_sounds.py puts on it — into DRAFT, or into SourceArt with `ship`.
A preview of each, played between stretches of fight with its live phase looped, always goes to DRAFT.
"""
import collections
import json
import pathlib
import sys

import numpy as np
from scipy.io import wavfile

sys.path.insert(0, str(pathlib.Path(__file__).parent))
import hex_intro_mech_score as intro  # noqa: E402
import mech_kit as kit  # noqa: E402
from hex_intro_score import FPS, shake  # noqa: E402

PROJECT = kit.PROJECT
MOTION = PROJECT / "Saved" / "BoneMotion"
SHIPPED = PROJECT / "SourceArt" / "Audio" / "HexBossMontages"
DRAFT = PROJECT / "Saved" / "Audio" / "HexBossMontages" / "Draft"
LOOP = intro.SHIPPED_STEMS / "loop.json"
# The sounds' families: SourceArt folder and package
FAMILIES = {"Mech": ("SourceArt/Audio/Mech", kit.PACKAGE),
            "Machine": ("SourceArt/Audio/Machine", "/Game/Art/SFX/Machine"),
            "Metal": ("SourceArt/Audio/Metal", "/Game/Art/SFX/Metal"),
            "HexFight": ("SourceArt/Audio/HexBossIntro", intro.STEM_PACKAGE)}
LOOP_NOTIFY = {"LoopStart": {}, "LoopStop": {"play": False}}
BONES = ["Root", "HexOuter", "HexMid", "HexCore"]
KEY_RATE = 120           # curve keys a second: a quarter frame, so a one-frame step stays a step
EDGE = 6                 # frames the whir takes to hand over between the fight's and the montage's
HUSH_DIP = -24.0         # dB the engine drops by while the boss holds dead still before a hit
HIT_PUNCH = 4.0          # dB the engine jumps by on a hit, dying away over the fire section
WHIR_MAX = 8.0           # dB the fastest turn raises the whir by over the fight's
SHAKE_FULL = 20.0        # units of shake at which the rattle is at its loudest, as the intro's gather at its worst
QUIETEST_RATTLE = 0.02   # of the full shake, below which the rattle stays out
PASSES = 3               # times the preview plays a looping section
FIGHT_AROUND = (1.0, 1.5)  # seconds of plain fight before and after each montage in its preview

# Where a key or a cue sits: an anchor and frames from it. start is frame 0, hush the first dead-still frame before
# the hit, hit the fire section's start, loop the looping section's, recoil the End section's, last the final frame.
# speed: the engine's speed keys, (anchor, frames, speed 0 to 1 — past 1 overdriven — and the exponent easing the
# segment into it, above 1 accelerating); REST is the fight's. hush: dead-still frames before the hit. pulse: dB the
# engine swells by on each pass of the looping section. cues: (anchor, frames, sound or loop notify, volume). dies:
# where the loop is cut for good, or None to hand it back.
Clip = collections.namedtuple("Clip", "name speed hush pulse cues dies")
# The engine's speed at the fight's pitch, where every engine curve starts and ends
REST = (intro.FIGHT_ENGINE[0] * kit.engine_rate(kit.engine_loop_speed()[0]) - kit.ENGINE_WOOF[0]) / (
    kit.ENGINE_WOOF[1] - kit.ENGINE_WOOF[0])

CLIPS = [
    # One accelerating wind-up every ring turns through, locked dead, the beam let go whole, the rings spinning on
    Clip("SweepBeam", [("start", 0, REST, 1.0), ("hush", -1, 1.0, 2.4), ("loop", 0, 0.9, 1.0), ("recoil", 0, 0.9, 1.0),
                       ("last", 0, REST, 0.5)], 5, 0.0, [
        ("start", 0, "Machine_GearThunk_1", 0.5), ("hush", 0, "Machine_Slam_Mid", 0.7), ("hush", 0, "Metal_Crack", 0.9),
        ("hit", 0, "Mech_Surge", 1.0), ("hit", 0, "Mech_AirBlast", 0.6), ("recoil", 0, "Machine_Hiss", 0.5),
        ("recoil", 8, "Machine_GearThunk_2", 0.4)], None),
    # No ring turns winding up: the machine loads, straining and rattling, rather than spinning; live, the core
    # ratchets round on its judder
    Clip("CarvingRay", [("start", 0, REST, 1.0), ("hush", -1, 0.55, 1.5), ("hit", -1, 0.55, 1.0), ("hit", 0, 0.85, 1.0),
                        ("loop", 0, 0.75, 1.0), ("recoil", 0, 0.75, 1.0), ("last", 0, REST, 0.5)], 5, 0.0, [
        ("hush", 0, "Machine_Slam_Low", 0.8), ("hush", 0, "Metal_CrackBright", 0.7), ("hit", 0, "Mech_Impact_Low", 1.0),
        ("hit", 0, "Mech_AirBlast", 0.5), ("loop", 0, "Mech_Tick_1", 0.45), ("loop", 4, "Mech_Tick_2", 0.3),
        ("loop", 8, "Mech_Tick_3", 0.3), ("recoil", 0, "Machine_Hiss", 0.5)], None),
    # Cranked back, whipped round, clamped: a gear thunk on each beat, then a salvo a pass
    Clip("ConeSpray", [("start", 0, REST, 1.0), ("start", 15, 0.35, 2.0), ("start", 18, 0.3, 1.0),
                       ("start", 30, 0.7, 0.5), ("hush", -1, 1.0, 2.4), ("loop", 0, 0.85, 1.0),
                       ("recoil", 0, 0.85, 1.0), ("last", 0, REST, 0.5)], 5, 3.0, [
        ("start", 0, "Machine_GearThunk_1", 0.5), ("start", 15, "Machine_GearThunk_2", 0.7),
        ("start", 15, "Mech_Grind", 0.35), ("start", 30, "Machine_GearThunk_3", 0.6),
        ("hush", 0, "Machine_Slam_High", 0.7), ("hush", 0, "Metal_Crack", 0.7), ("hit", 0, "Mech_Impact_High", 0.9),
        ("hit", 0, "Mech_AirStutter", 0.5), ("loop", 0, "Mech_Hit_1", 0.5), ("recoil", 0, "Machine_Hiss", 0.4)], None),
    # One heavy thing lobbed: the deepest gather, blown open on one frame
    Clip("Bomb", [("start", 0, REST, 1.0), ("hush", -1, 0.65, 2.4), ("hit", -1, 0.65, 1.0), ("hit", 0, 0.8, 1.0),
                  ("last", 0, REST, 0.6)], 3, 0.0, [
        ("start", 0, "Machine_GearThunk_4", 0.4), ("hush", 0, "Machine_Slam_Mid", 0.6),
        ("hit", 0, "Mech_Impact_Mid", 0.8), ("hit", 0, "Mech_AirBlast", 0.7), ("hit", 6, "Machine_Hiss", 0.35)], None),
    # Several light things flicked off the rim at once
    Clip("Turret", [("start", 0, REST, 1.0), ("hush", -1, 0.5, 2.4), ("hit", -1, 0.5, 1.0), ("hit", 0, 0.6, 1.0),
                    ("last", 0, REST, 0.6)], 3, 0.0, [
        ("hush", 0, "Machine_Slam_High", 0.5), ("hit", 0, "Mech_AirStutter", 0.7),
        ("hit", 0, "Metal_CrackBright", 0.6)], None),
    # Spun up past anything the fight reaches, the rings straying, gathered and blown apart: the loop dies with it.
    # Beats from hex_boss_death.py: turning from FALTER 5, straying from 42, gathering from 90, the blast on 104.
    Clip("Death", [("start", 0, REST, 1.0), ("start", 5, REST, 1.0), ("start", 90, 1.0, 1.0),
                   ("start", 103, 1.4, 2.0)], 0, 0.0, [
        ("start", 0, "LoopStart", 1.0), ("start", 5, "Machine_GearThunk_1", 0.6), ("start", 42, "Mech_Grind", 0.6),
        ("start", 74, "Mech_Riser", 0.6), ("start", 104, "LoopStop", 1.0), ("start", 104, "Mech_Break", 1.0),
        ("start", 104, "Machine_Blast", 0.6), ("start", 106, "Metal_Crack", 0.5),
        ("start", 108, "Metal_CrackBright", 0.4)], ("start", 104)),
]


def anchors(motion, hush):
    """Anchor name -> frame, off the montage's sections."""
    sections = {name: round(start * FPS) for name, start, _ in motion["sections"]}
    fire = next((frame for name, frame in sections.items() if name.startswith("Fire") and name != "FireLoop"), None)
    points = {"start": 0, "last": round(motion["length"] * FPS)}
    if fire is not None:
        points.update(hit=fire, hush=fire - hush)
    if "FireLoop" in sections:
        points["loop"] = sections["FireLoop"]
    if "End" in sections:
        points["recoil"] = sections["End"]
    return points


def eased(keys, frames):
    """The value per frame through `keys`, [(frame, value, ease)]: each segment eased into its key by its exponent."""
    values = np.full(len(frames), keys[0][1], dtype=float)
    for (first, start, _), (last, end, ease) in zip(keys, keys[1:]):
        inside = (frames >= first) & (frames <= last)
        alpha = (frames[inside] - first) / max(last - first, 1e-9)
        values[inside] = start + (end - start) * alpha ** ease
    values[frames > keys[-1][0]] = keys[-1][1]
    return values


def engine_semitones(speed):
    """The engine loop's pitch bend at `speed` from the fight's: pitch is speed, as in the intro."""
    return 12.0 * np.log2(kit.engine_rate(speed) / kit.engine_rate(REST))


def engine_decibels(speed, intro_volume):
    """The engine loop's level at `speed` over the fight's: as loud as the intro's engine was at that speed, and
    never under the fight's, which sits above the intro's at its slowest."""
    cruise = kit.engine_level(kit.engine_loop_speed()[0])
    level = kit.engine_level(speed) / cruise * intro_volume / intro.FIGHT_ENGINE[1]
    return np.maximum(0.0, 20.0 * np.log10(np.maximum(level, 1e-9)))


def turn_speed(motion, times):
    """Degrees a second the fastest ring turns, at `times`."""
    rate = motion["rate"]
    speeds = [np.abs(np.gradient(np.asarray(motion["bones"][bone]["yaw"]), 1.0 / rate)) for bone in BONES[1:]]
    return np.interp(times, motion["times"], np.max(speeds, axis=0))


def shaking(motion, times):
    """How hard the boss shakes, 0 to 1, at `times`: the widest alternation of the body or any ring, per frame."""
    step = int(round(motion["rate"] / FPS))
    frames = np.arange(len(motion["times"][::step])) / FPS
    widths = [shake(motion["bones"][bone]["x"][::step], motion["bones"][bone]["y"][::step]) for bone in BONES]
    return np.interp(times, frames, np.clip(np.max(widths, axis=0) / SHAKE_FULL, 0.0, 1.0))


def curves(clip, motion, intro_volume, fight_turn):
    """The clip's curves as [seconds, value] keys, and the anchors they were placed by."""
    points = anchors(motion, clip.hush)
    length = motion["length"]
    times = np.append(np.arange(0.0, length, 1.0 / KEY_RATE), length)
    frames = times * FPS
    speed = eased([(points[anchor] + offset, value, ease) for anchor, offset, value, ease in clip.speed], frames)

    hushed = np.zeros(len(frames), dtype=bool)
    punch = np.zeros(len(frames))
    pulse = np.zeros(len(frames))
    if "hit" in points:
        hushed = (frames >= points["hush"]) & (frames < points["hit"])
        fire_end = points.get("loop", points.get("recoil", points["last"]))
        after = np.clip((frames - points["hit"]) / (fire_end - points["hit"]), 0.0, 1.0)
        punch = np.where(frames >= points["hit"], HIT_PUNCH * (1.0 - after) ** 2, 0.0)
    if clip.pulse and "loop" in points:
        within = (frames - points["loop"]) / (points["recoil"] - points["loop"])
        pulse = np.where((within >= 0.0) & (within < 1.0), clip.pulse * (1.0 - np.clip(within, 0.0, 1.0)) ** 2, 0.0)

    drive = np.sqrt(np.clip(turn_speed(motion, times) / intro.FULL_SPEED, 0.0, 1.0))
    rest_drive = np.sqrt(fight_turn / intro.FULL_SPEED)
    edge = np.clip(np.minimum(frames, frames[-1] - frames) / EDGE, 0.0, 1.0)
    whir = edge * np.clip(20.0 * np.log10(np.maximum(drive, 1e-9) / rest_drive), intro.SILENT, WHIR_MAX)
    amount = edge * shaking(motion, times)
    rattling = amount >= QUIETEST_RATTLE
    rattle = np.where(rattling, -intro.SILENT + 10.0 * np.log10(np.maximum(amount, 1e-9)), 0.0)
    # Clicks come denser the harder it shakes: 4 a second at nothing to 26 at full, as the kit's rattle
    rattle_pitch = np.where(rattling, 12.0 * np.log2(np.maximum((4.0 + 22.0 * amount) / 26.0, 0.5)), 0.0)

    engine = engine_decibels(speed, intro_volume) + punch + pulse + np.where(hushed, HUSH_DIP, 0.0)
    whir = np.where(hushed, intro.SILENT, whir)
    rattle = np.where(hushed, 0.0, rattle)
    if clip.dies:
        dead = frames >= points[clip.dies[0]] + clip.dies[1]
        engine = np.where(dead, intro.SILENT, engine)
        whir = np.where(dead, intro.SILENT, whir)
        rattle = np.where(dead, 0.0, rattle)
    values = {intro.PITCH_CURVE: engine_semitones(speed), intro.ENGINE_CURVE: engine, intro.WHIR_CURVE: whir,
              intro.RATTLE_PITCH_CURVE: rattle_pitch, intro.RATTLE_CURVE: rattle}
    return {name: [[round(float(time), 4), round(float(value), 3)] for time, value in zip(times, curve)]
            for name, curve in values.items()}, points


def source(sound):
    """A sound's family folder and package, and its file."""
    folder, package = FAMILIES[sound.split("_")[0]]
    return PROJECT / folder / f"SFX_{sound}.wav", package


def cues(clip, points):
    entries = []
    for anchor, offset, sound, volume in clip.cues:
        time = round((points[anchor] + offset) / FPS, 4)
        if sound in LOOP_NOTIFY:
            entries.append({"track": "Loop", "notify": "GeoLoopSoundNotify", "time": time,
                            "properties": LOOP_NOTIFY[sound]})
        else:
            path, package = source(sound)
            if not path.exists():
                raise FileNotFoundError(path)
            entries.append({"track": sound.split("_")[0], "sound": f"SW_{sound}", "package": package, "time": time,
                            "volume": volume, "source": str(path.relative_to(PROJECT)).replace("\\", "/")})
    return sorted(entries, key=lambda entry: entry["time"])


def play_order(motion):
    """The montage as it plays, [(section start, section end)] seconds: each section into the next, a looping one
    PASSES times before the jump to End that ends a pattern."""
    sections = sorted(motion["sections"], key=lambda section: section[1])
    ends = {name: (sections[index + 1][1] if index + 1 < len(sections) else motion["length"])
            for index, (name, _, _) in enumerate(sections)}
    starts = {name: start for name, start, _ in sections}
    following = {name: after for name, _, after in sections}
    order, name, passes = [], sections[0][0], 0
    while name != "None":
        order.append((starts[name], ends[name]))
        if following[name] == name:
            passes += 1
            name = name if passes < PASSES else ("End" if "End" in starts else "None")
        else:
            name = following[name]
    return order


def preview(clip, motion, entries, clip_curves, layers):
    """The montage between stretches of fight, as Unreal plays it: its one-shots on their beats, and the loop bent by
    its curves, weighted by the montage's blend."""
    before, after = FIGHT_AROUND
    order = play_order(motion)
    played_length = sum(end - start for start, end in order)
    length = kit.seconds(before + played_length + after)
    positions = np.full(length, np.nan)
    mix = np.zeros(length)
    clock = before
    for start, end in order:
        span = slice(kit.seconds(clock), kit.seconds(clock + end - start))
        positions[span] = start + kit.timeline(span.stop - span.start)
        for entry in entries:
            if start <= entry["time"] < end and "source" in entry:
                sound = wavfile.read(PROJECT / entry["source"])[1].astype(np.float64).mean(axis=1) / 32767.0
                kit.add(mix, sound, kit.seconds(clock + entry["time"] - start), entry["volume"])
        clock += end - start

    elapsed = kit.timeline(length) - before
    weight = np.clip(np.minimum(elapsed / max(motion["blend_in"], 1e-3),
                                (played_length - elapsed) / max(motion["blend_out"], 1e-3)), 0.0, 1.0)
    weight[np.isnan(positions)] = 0.0
    starts = [entry["time"] for entry in entries if entry.get("properties") == LOOP_NOTIFY["LoopStart"]]
    stops = [entry["time"] for entry in entries if entry.get("properties") == LOOP_NOTIFY["LoopStop"]]
    for layer in layers:
        loop = wavfile.read(PROJECT / layer["source"])[1][:, 0] / 32767.0
        running = intro.played(loop, np.nan_to_num(positions), clip_curves, layer, weight)
        if starts:
            running[:kit.seconds(before + starts[0])] = 0.0
        if stops:
            running[kit.seconds(before + stops[0]):] = 0.0
        mix += running
    return mix


def main(ship):
    folder = SHIPPED if ship else DRAFT
    DRAFT.mkdir(parents=True, exist_ok=True)
    fight = json.loads(LOOP.read_text())
    layers = fight["layers"]
    for layer in layers:
        folder_of = next(folder for folder, package in FAMILIES.values() if package == layer["package"])
        layer["source"] = f"{folder_of}/{layer['sound'].replace('SW_', 'SFX_', 1)}.wav"
    intro_motion = json.loads((MOTION / "Intro.json").read_text())
    fight_turn = intro.steady_turn(intro_motion["bones"], np.asarray(intro_motion["times"]))

    mixes = {}
    for clip in CLIPS:
        motion = json.loads((MOTION / f"{clip.name}.json").read_text())
        clip_curves, points = curves(clip, motion, fight["intro_volume"], fight_turn)
        entries = cues(clip, points)
        (folder / clip.name).mkdir(parents=True, exist_ok=True)
        (folder / clip.name / "cues.json").write_text(json.dumps(entries, indent=1) + "\n")
        (folder / clip.name / "curves.json").write_text(json.dumps({"curves": clip_curves}, indent=1) + "\n")
        mixes[clip.name] = preview(clip, motion, entries, clip_curves, layers)

        print(f"== {clip.name}: {len(entries)} cues, anchors {points}")
        for name, keys in clip_curves.items():
            values = [value for _, value in keys]
            closure = ""
            if "loop" in points:
                at = {frame: values[min(range(len(keys)), key=lambda i: abs(keys[i][0] * FPS - frame))]
                      for frame in (points["loop"], points["recoil"])}
                closure = f", loop {at[points['loop']]:+.2f} -> {at[points['recoil']]:+.2f}"
            print(f"  {name}: {min(values):+.2f} to {max(values):+.2f}, ends {values[0]:+.2f} / {values[-1]:+.2f}"
                  f"{closure}")

    gain = kit.PEAK / max(np.abs(mix).max() for mix in mixes.values())  # one gain, so they compare as they play
    for name, mix in mixes.items():
        print(kit.save(DRAFT / f"{name}_preview.wav", gain * mix))


if __name__ == "__main__":
    main(sys.argv[1:] == ["ship"])
