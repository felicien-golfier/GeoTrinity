# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy", "scipy"]
# ///
"""
Score the star boss: its fight loop and every montage, from the ninja kit — air and cloth, no metal.

The loop is three layers: swaying foliage, all but quiet until a montage lifts it, shaking harder and louder the
faster the star turns, a whirl of swishes, one per point sweeping past, playing only while it turns, and cloth
fluttering only while it shakes. Each montage bends them through its curves, all read off the star's own motion, and
puts one-shots on its beats: an air cut on every point stabbing out, air drawn in to the dead-still frames before a
nova, a gust as every point blows out. Every curve starts and ends on the fight's own loop, except the intro's, which brings the wind in from silence, and the death's,
whose vanishing cuts the loop.

Run outside the editor: uv run AI/Python/Audio/star_boss_score.py [ship]
Needs AI/Audio/BoneMotion/Star/<montage>.json (AI/Python/Anim/dump_bone_motion.py) and the ninja kit
(ninja_kit.py ship). Writes loop.json — the layers AI/Python/Asset/boss_fight_loop.py puts on the boss — and each
montage's cues.json and curves.json — what AI/Python/Anim/boss_montage_sounds.py puts on it — into DRAFT, or into
SourceArt with `ship`. A preview of each, played between stretches of fight, always goes to DRAFT.
"""
import collections
import json
import pathlib
import sys

import numpy as np
from scipy.signal import medfilt

sys.path.insert(0, str(pathlib.Path(__file__).parent))
import hex_boss_montage_score as montage  # noqa: E402
import hex_intro_mech_score as intro  # noqa: E402
import mech_kit as kit  # noqa: E402
import ninja_kit as ninja  # noqa: E402
from hex_intro_score import FPS, shake  # noqa: E402

PROJECT = kit.PROJECT
MOTION = PROJECT / "AI" / "Audio" / "BoneMotion" / "Star"
SHIPPED = PROJECT / "SourceArt" / "Audio" / "StarBoss"
DRAFT = PROJECT / "AI" / "Audio" / "Drafts" / "StarBoss"
SILENT = intro.SILENT
BODY = "Root"
TIP_PREFIX = "apexe_outside_"
KEY_RATE = 120           # curve keys a second
EDGE = 6                 # frames the whirl and flutter take to come in and hand back
ERUPT_SPEED = 30.0       # units a frame a point must stab out at to be heard
REARM = 4                # frames before the same point can stab again
BURST_POINTS = 6         # points stabbing out on one frame that make a nova
STILL = (0.05, 0.5, 0.002)  # the most a still frame moves: degrees of turn, units of any bone, scale
WIND_REST = -20.0        # dB the wind idles at under its authored level, all but quiet, while no montage lifts it
WIND_LIFT = 14.0         # dB a montage lifts the wind by off its rest, before the spin raises it further
WIND_SPIN = 1.0          # turns a second that double the wind's pitch
WIND_GAIN = 5.0          # dB the wind rises by each time its pitch doubles
WIND_TOP = 12.0          # semitones the wind is never pushed past
WIND_LAG = 0.12          # seconds the wind takes to catch up with the spin, short of any looping section
SPIKE = 9                # samples a spin spike lasts at most, where sequences meet, to be ignored
HUSH_DIP = -24.0         # dB the wind drops by while the star holds dead still before a nova
PUNCH_SECONDS = 1.2      # seconds the wind's jump on a hit takes to die away, at most
WHIRL_FULL = 1.75        # turns a second the whirl is loudest at, the death's top speed
WHIRL_QUIETEST = -40.0   # dB under its loudest at which the whirl stays out, low enough to come in unnoticed
WHIRL_RANGE = (-24.0, 12.0)  # the whirl's pitch bend, the bottom being Unreal's pitch floor
SHAKE_FULL = 30.0        # units of shake at which the flutter is at its loudest, the death at its worst
QUIETEST_FLUTTER = 0.02
ONE_SHOTS = -4.0         # dB every montage's one-shots sit under their volumes in CLIPS; the loop is left as it is

# The fight loop, each layer: its sound, volume, curves, rest level and drift (semitones, seconds) until tuned on the
# boss by ear
Layer = collections.namedtuple("Layer", "sound volume pitch_curve volume_curve rest drift")
LAYERS = [Layer("Wind_Loop", 0.3, "WindSemitones", "WindDecibels", WIND_REST, (1.5, 5.0)),
          Layer("Whirl_Loop", 0.5, "WhirlSemitones", "WhirlDecibels", SILENT, (0.0, 6.0)),
          Layer("Flutter_Loop", 0.4, "FlutterSemitones", "FlutterDecibels", SILENT, (0.0, 6.0))]

# Where a cue sits: an anchor and frames from it. start is frame 0 and last the final frame; a section's name, lower
# case, is where it starts; erupt0, erupt1… are the points stabbing out one by one, burst every point at once, hush the
# first dead-still frame before the burst and gone the first frame nothing of the star is left.
# stabs: on every point stabbing out alone, an air cut at this volume and the cloth snapping as it whips, or None.
# cues: (anchor, frames, sound or loop notify, volume); a sound leading its beat, like an inhale, starts that much
# earlier. rise: the wind's dB under the loop's, (anchor, frames, dB) keys from silence, 0 after the last. punch: the
# anchor the wind jumps on and by how many dB, dying away before the next section, or None. dies: where the loop is cut
# for good, or None to hand it back.
Clip = collections.namedtuple("Clip", "name stabs cues rise punch dies")
CLIPS = [
    # A seed stirring out of nothing, its points erupting ever faster, winding in, frozen, going nova
    Clip("Intro", (0.7, 0.35), [
        ("start", 0, "LoopStart", 1.0), ("hush", -30, "Ninja_Inhale", 0.8), ("burst", 0, "Ninja_Gust", 1.0),
        ("burst", 0, "Ninja_Whoosh_Low", 0.8), ("burst", 1, "Ninja_Flap_1", 0.6)],
        [("start", 0, SILENT), ("start", 8, -40.0), ("erupt0", 0, -14.0), ("erupt7", 20, 0.0)], ("burst", 5.0), None),
    # Swelling and twisting back, popping on the fire, spinning out its spiral, settling
    Clip("Attack", None, [
        ("start", 0, "Ninja_Whoosh_Mid", 0.4), ("fire", -30, "Ninja_Inhale", 0.45), ("fire", 0, "Ninja_Gust", 0.5),
        ("fire", 0, "Ninja_Cut_2", 0.6), ("fire", 1, "Ninja_Flap_1", 0.4), ("end", 0, "Ninja_Whoosh_High", 0.3)],
        [], ("fire", 3.0), None),
    # A spin wound into a knot, frozen, every spike blown out, a dance of breaths, the recoil
    Clip("DevastatingWave", None, [
        ("start", 0, "Ninja_Swish_1", 0.4), ("hush", -30, "Ninja_Inhale", 0.7), ("burst", 0, "Ninja_Gust", 0.9),
        ("burst", 0, "Ninja_Whoosh_Low", 0.7), ("burst", 1, "Ninja_Flap_2", 0.5), ("burst", 5, "Ninja_Swish_2", 0.2),
        ("burst", 13, "Ninja_Swish_3", 0.3), ("burst", 21, "Ninja_Swish_4", 0.3), ("burst", 29, "Ninja_Swish_1", 0.2),
        ("end", 0, "Ninja_Whoosh_High", 0.35), ("end", 6, "Ninja_Land", 0.4)], [], ("burst", 5.0), None),
    # A breath out, a needle stabbing from one point after another as it clenches, frozen, the spikes thrown out
    Clip("PikeNova", None, [
        ("start", 0, "Ninja_Swish_2", 0.35), ("erupt0", 0, "Ninja_Flurry", 0.6), ("hush", -9, "Ninja_Inhale_Short", 0.6),
        ("burst", 0, "Ninja_Gust", 0.7), ("burst", 0, "Ninja_Cut_1", 0.8), ("burst", 0, "Ninja_Whoosh_Mid", 0.6),
        ("burst", 7, "Ninja_Flap_3", 0.4)], [], ("burst", 3.0), None),
    # Spun up to a howl, its points stabbing out in no order, swollen, the points yanked in, gone in a puff
    Clip("Death", (0.8, 0.45), [
        ("start", 0, "LoopStart", 1.0), ("start", 5, "Ninja_Whoosh_Low", 0.5), ("gone", -13, "Ninja_Inhale_Short", 0.7),
        ("gone", -10, "LoopStop", 1.0), ("gone", -10, "Ninja_Vanish", 1.0), ("gone", -7, "Ninja_Flap_4", 0.6)],
        [], None, ("gone", -10)),
]


def frames_of(motion, bone, field):
    """The bone's `field` once a frame."""
    step = int(round(motion["rate"] / FPS))
    return np.asarray(motion["bones"][bone][field][::step])


def tips(motion):
    return sorted((bone for bone in motion["bones"] if bone.startswith(TIP_PREFIX)),
                  key=lambda bone: int(bone[len(TIP_PREFIX):]))


def reach(motion, tip):
    """Units a point is stabbed out from where it rests, once a frame."""
    rest = motion["rest"][tip]
    return np.hypot(frames_of(motion, tip, "x") - rest["x"], frames_of(motion, tip, "y") - rest["y"])


def stabs(motion):
    """Frames points stab out on -> how many do on that frame."""
    counts = collections.Counter()
    for tip in tips(motion):
        stabbed = reach(motion, tip)
        speed = np.diff(stabbed, prepend=stabbed[0])
        armed_from = 0
        for frame in np.flatnonzero(speed > ERUPT_SPEED):
            if frame >= armed_from:
                counts[int(frame)] += 1
            armed_from = frame + REARM
    return dict(sorted(counts.items()))


def still(motion):
    """Per frame, whether nothing of the star moves: no turn, no bone travelling, nothing changing size."""
    moving = np.abs(np.diff(frames_of(motion, BODY, "yaw"), prepend=np.nan)) > STILL[0]
    for bone in motion["bones"]:
        travel = np.hypot(np.diff(frames_of(motion, bone, "x"), prepend=np.nan),
                          np.diff(frames_of(motion, bone, "y"), prepend=np.nan))
        growth = np.abs(np.diff(frames_of(motion, bone, "scale"), prepend=np.nan))
        moving |= (travel > STILL[1]) | (growth > STILL[2])
    return ~moving


def anchors(motion):
    """Anchor name -> frame, off the montage's sections and the star's own motion."""
    points = {"start": 0, "last": round(motion["length"] * FPS)}
    points.update({name.lower(): round(start * FPS) for name, start, _ in motion["sections"]})
    stabbed = stabs(motion)
    singles = [frame for frame, count in stabbed.items() if count < BURST_POINTS]
    points.update({f"erupt{index}": frame for index, frame in enumerate(singles)})
    burst = next((frame for frame, count in stabbed.items() if count >= BURST_POINTS), None)
    if burst is not None:
        points["burst"] = burst
        stillness = still(motion)
        hush = burst
        while hush > 0 and stillness[hush - 1]:
            hush -= 1
        if hush < burst:
            points["hush"] = hush
    gone = np.flatnonzero(frames_of(motion, BODY, "scale") < 0.02)
    if len(gone):
        points["gone"] = int(gone[0])
    return points, singles


def spin(motion, times):
    """Turns a second the star makes, at `times`. A turn by a whole point is no turn at all to the eye, so where two
    sequences meet on different such turns the jump is read as the little it really is, and a lone spike ignored."""
    symmetry = 360.0 / len(tips(motion))
    yaw = np.asarray(motion["bones"][BODY]["yaw"])
    step = (np.diff(yaw, prepend=yaw[0]) + symmetry / 2.0) % symmetry - symmetry / 2.0
    return np.interp(times, motion["times"], medfilt(np.abs(step) * motion["rate"] / 360.0, SPIKE))


def carried(turning):
    """The spin as the air around the star follows it, a key at a time: catching up over WIND_LAG, so a flick of the
    body never jerks the wind."""
    follow = 1.0 - np.exp(-1.0 / (WIND_LAG * KEY_RATE))
    air = np.zeros(len(turning))
    for index in range(1, len(turning)):
        air[index] = air[index - 1] + follow * (turning[index] - air[index - 1])
    return air


def shaking(motion, times):
    """How hard the star shakes, 0 to 1, at `times`."""
    width = shake(frames_of(motion, BODY, "x"), frames_of(motion, BODY, "y"))
    return np.interp(times, np.arange(len(width)) / FPS, np.clip(width / SHAKE_FULL, 0.0, 1.0))


def curves(clip, motion, points):
    """The clip's curves as [seconds, value] keys."""
    length = motion["length"]
    times = np.append(np.arange(0.0, length, 1.0 / KEY_RATE), length)
    frames = times * FPS
    turning = spin(motion, times)
    edge = np.clip(np.minimum(frames, frames[-1] - frames) / EDGE, 0.0, 1.0)

    lift = edge * np.log2(1.0 + carried(turning) / WIND_SPIN)
    wind_pitch = np.minimum(12.0 * lift, WIND_TOP)
    wind = edge * WIND_LIFT + WIND_GAIN * lift
    if clip.rise:
        keys = [(points[anchor] + offset, level, 1.0) for anchor, offset, level in clip.rise]
        wind += np.where(frames <= keys[-1][0], montage.eased(keys, frames), 0.0)
    hushed = np.zeros(len(frames), dtype=bool)
    if "hush" in points:
        hushed = (frames >= points["hush"]) & (frames < points["burst"])
    if clip.punch:
        hit = points[clip.punch[0]]
        following = [round(start * FPS) - hit for _, start, _ in motion["sections"] if round(start * FPS) > hit]
        after = (frames - hit) / min([PUNCH_SECONDS * FPS, frames[-1] - hit] + following)
        wind += np.where((after >= 0.0) & (after < 1.0), clip.punch[1] * (1.0 - np.clip(after, 0.0, 1.0)) ** 2, 0.0)
    wind += np.where(hushed, HUSH_DIP, 0.0)

    level = 20.0 * np.log10(np.minimum(turning / WHIRL_FULL, 1.0) + 1e-9)
    whirling = (level > WHIRL_QUIETEST) & ~hushed
    whirl = np.where(whirling, edge * (-SILENT + level), 0.0)
    whirl_pitch = np.where(whirling, np.clip(12.0 * np.log2(np.maximum(turning, 1e-3) / ninja.WHIRL_TURNS),
                                             *WHIRL_RANGE), 0.0)

    amount = edge * shaking(motion, times)
    fluttering = (amount >= QUIETEST_FLUTTER) & ~hushed
    flutter = np.where(fluttering, -SILENT + 10.0 * np.log10(np.maximum(amount, 1e-9)), 0.0)
    # Snaps come faster the harder it shakes: 0.6 of the loop's rate at nothing to 1.4 at its worst
    flutter_pitch = np.where(fluttering, 12.0 * np.log2(0.6 + 0.8 * amount), 0.0)

    if clip.dies:
        dead = frames >= points[clip.dies[0]] + clip.dies[1]
        wind = np.where(dead, SILENT, wind)
        whirl = np.where(dead, 0.0, whirl)
        flutter = np.where(dead, 0.0, flutter)
    values = dict(zip([layer.pitch_curve for layer in LAYERS] + [layer.volume_curve for layer in LAYERS],
                      [wind_pitch, whirl_pitch, flutter_pitch, wind, whirl, flutter]))
    return {name: [[round(float(time), 4), round(float(value), 3)] for time, value in zip(times, closed(curve, times,
                                                                                                        motion))]
            for name, curve in values.items()}


def looping(motion):
    """(start, end) seconds of every section that loops on itself."""
    sections = sorted(motion["sections"], key=lambda section: section[1])
    return [(start, sections[index + 1][1] if index + 1 < len(sections) else motion["length"])
            for index, (name, start, following) in enumerate(sections) if following == name]


def closed(curve, times, motion):
    """`curve` tilted across each looping section until its end meets its start, so a pass runs into the next."""
    curve = curve.copy()
    for start, end in looping(motion):
        inside = (times >= start) & (times < end)
        gap = np.interp(start, times, curve) - curve[inside][-1]
        curve[inside] += gap * (times[inside] - start) / (times[inside][-1] - start)
    return curve


def source(sound):
    return ninja.SHIPPED / f"SFX_{sound}.wav"


def cue(time, sound, volume):
    if sound in montage.LOOP_NOTIFY:
        return {"track": "Loop", "notify": "GeoLoopSoundNotify", "time": time,
                "properties": montage.LOOP_NOTIFY[sound]}
    path = source(sound)
    if not path.exists():
        raise FileNotFoundError(path)
    return {"track": "Ninja", "sound": f"SW_{sound}", "package": ninja.PACKAGE, "time": time,
            "volume": round(volume * kit.decibels(ONE_SHOTS), 3), "source": str(path.relative_to(PROJECT)).replace("\\", "/")}


def cues(clip, points, singles):
    entries = [cue(round((points[anchor] + offset) / FPS, 4), sound, volume)
               for anchor, offset, sound, volume in clip.cues]
    if clip.stabs:
        cut_volume, whip_volume = clip.stabs
        for index, frame in enumerate(singles):
            variant = index % ninja.VARIANTS + 1
            entries.append(cue(round(frame / FPS, 4), f"Ninja_Cut_{variant}", cut_volume))
            entries.append(cue(round((frame + 1) / FPS, 4), f"Ninja_Flap_{variant}", whip_volume))
    return sorted(entries, key=lambda entry: entry["time"])


def loop_layers():
    return [{"sound": f"SW_Ninja_{layer.sound}", "package": ninja.PACKAGE, "volume": layer.volume, "pitch": 1.0,
             "pitch_curve": layer.pitch_curve, "volume_curve": layer.volume_curve, "rest_decibels": layer.rest,
             "drift_semitones": layer.drift[0], "drift_period": layer.drift[1],
             "source": str(source(f"Ninja_{layer.sound}").relative_to(PROJECT)).replace("\\", "/")}
            for layer in LAYERS]


def main(ship):
    folder = SHIPPED if ship else DRAFT
    DRAFT.mkdir(parents=True, exist_ok=True)
    folder.mkdir(parents=True, exist_ok=True)
    layers = loop_layers()
    (folder / "loop.json").write_text(json.dumps({"layers": layers}, indent=1) + "\n")

    mixes = {}
    for clip in CLIPS:
        path = MOTION / f"{clip.name}.json"
        if not path.exists():
            print(f"== {clip.name}: no motion dump, skipped")
            continue
        motion = json.loads(path.read_text())
        points, singles = anchors(motion)
        clip_curves = curves(clip, motion, points)
        entries = cues(clip, points, singles)
        (folder / clip.name).mkdir(parents=True, exist_ok=True)
        (folder / clip.name / "cues.json").write_text(json.dumps(entries, indent=1) + "\n")
        (folder / clip.name / "curves.json").write_text(json.dumps({"curves": clip_curves}, indent=1) + "\n")
        mixes[clip.name] = montage.preview(clip, motion, entries, clip_curves, layers)

        print(f"== {clip.name}{' (stand-in motion)' if motion.get('stand_in') else ''}: {len(entries)} cues,"
              f" anchors {points}")
        for name, keys in clip_curves.items():
            keys = np.asarray(keys)
            closure = "".join(f", loop {np.interp(first, *keys.T):+.2f} -> {keys[keys[:, 0] < last][-1, 1]:+.2f}"
                              for first, last in looping(motion))
            print(f"  {name}: {keys[:, 1].min():+.2f} to {keys[:, 1].max():+.2f},"
                  f" ends {keys[0, 1]:+.2f} / {keys[-1, 1]:+.2f}{closure}")

    gain = kit.PEAK / max(np.abs(mix).max() for mix in mixes.values())
    for name, mix in mixes.items():
        print(kit.save(DRAFT / f"{name}_preview.wav", gain * mix))


if __name__ == "__main__":
    main(sys.argv[1:] == ["ship"])
