# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy", "scipy"]
# ///
"""
The ninja kit: air, cloth and leaves, never metal — blades cutting air, cloth snapping taut, soft landings, gusts and
foliage swaying, each sound on its own. The star boss's voice: its points stab out as air cuts, its spin is a whirl of
swishes over leaves that shake harder the faster it turns.

Run outside the editor: uv run AI/Python/Audio/ninja_kit.py [ship]
Writes every one-shot and loop as SFX_Ninja_<Name>.wav into DRAFT, or into SourceArt/Audio/Ninja with `ship`, plus a
kit preview — every one-shot in turn, then each loop — into DRAFT.
"""
import pathlib
import sys

import numpy as np
from scipy.signal import istft, stft

sys.path.insert(0, str(pathlib.Path(__file__).parent))
import mech_kit as kit  # noqa: E402

PROJECT = kit.PROJECT
DRAFT = PROJECT / "AI" / "Audio" / "Drafts" / "Ninja"
SHIPPED = PROJECT / "SourceArt" / "Audio" / "Ninja"
PACKAGE = "/Game/Art/SFX/Ninja"
SAMPLE_RATE = kit.SAMPLE_RATE
VARIANTS = 4
WINDOW = 1024             # samples per filter frame; a quarter of it is the hop, ~5 ms
LOOP_SECONDS = 4.0
SIZES = {"Low": 1.4, "Mid": 1.0, "High": 0.75}

# A swish: seconds long, where its peak falls as a share of it, and the band it sweeps through — Hz at its edges and
# at the peak, where it is loudest and brightest. width: the band in octaves. whistle: the blade's edge singing an
# octave up, narrow, in dB under the swish.
SWISH = {"length": (0.22, 0.32), "peak": (0.45, 0.6), "band": (650.0, 2600.0), "width": 1.1, "whistle": -20.0}
CUT = {"length": (0.11, 0.15), "peak": (0.15, 0.25), "band": (1600.0, 5200.0), "width": 1.0, "whistle": -24.0}
WHOOSH = {"length": (0.7, 0.7), "peak": (0.42, 0.42), "band": (200.0, 950.0), "width": 1.5, "whistle": -26.0}
PUFF = (110.0, 600.0, -8.0, 0.035)  # the air a cut shoves: low Hz, high Hz, level dB, seconds to fall 20 dB
FLURRY_CUTS = 8
FLURRY_GAP = 1.0 / 15.0   # seconds between cuts: one a frame pair, as the spike nova's sweep fires its needles
FLURRY_CLIMB = 1.3        # how much brighter the last cut is than the first
FLAP_BURSTS = (3, 6)      # cracks in one cloth snap, within FLAP_SPAN
FLAP_SPAN = 0.035
FLAP_BAND = (220.0, 2800.0)
FLAP_BODY = (100.0, 380.0, -6.0, 0.03)
LAND_BODY = (40.0, 260.0, 0.0, 0.14)     # a soft landing's weight: low Hz, high Hz, level dB, seconds to fall 20 dB
LAND_PUFF = (260.0, 1500.0, -9.0, 0.05)
GUST = {"length": 1.8, "band": (2600.0, 420.0), "width": 2.4, "fall": 0.8, "push": (40.0, 220.0, -4.0, 0.35),
        "howl": -16.0}
INHALE_BAND = (260.0, 3600.0)  # air drawn in, brightening as it rushes into the cut
VANISH_SMOKE = (2000.0, 8000.0, -22.0, 0.9)  # the hiss of smoke left hanging: low Hz, high Hz, level dB, fall
WIND_CENTRE = 520.0       # Hz the breath of air through the leaves sits around
WIND_BREATH = -3.0        # dB of that breath under the leaves
LEAF_HISS = (2400.0, 1.4, -7.0)  # the leaves' mass brushing together: Hz, octaves wide, dB
LEAF_TICKS = 90.0         # dry leaf ticks a second at a sway's height
LEAF_TICK = (1000.0, 2800.0, 0.004)  # a tick's band: lowest and highest Hz its octave starts at, seconds to fall 20 dB
SWAYS = 3                 # sines shaping the foliage's slow sways over the loop
WHIRL_POINTS = 8          # swishes a turn, one per point sweeping past
WHIRL_TURNS = 1.0         # turns a second the loop spins at
WHIRL_CENTRE = 1100.0
FLUTTER_RATE = 11.0       # cloth snaps a second at the loop's own pitch, each gap a quarter either way


def filtered(noise, centre, width, loop=False):
    """`noise` through a band `width` octaves wide around `centre` Hz, both per sample, at steady power whatever the
    band: a swept band only changes colour, the envelope alone sets its level. With `loop`, the noise and the curves
    run round, so the result closes on itself."""
    length = len(noise)
    centre = np.broadcast_to(centre, length)
    width = np.broadcast_to(width, length)
    if loop:  # filtered three times over and the middle kept, so the filter runs across the seam as anywhere else
        noise, centre, width = np.tile(noise, 3), np.tile(centre, 3), np.tile(width, 3)
    frequencies, frames, spectrum = stft(noise, SAMPLE_RATE, nperseg=WINDOW, noverlap=WINDOW * 3 // 4)
    at = np.clip((frames * SAMPLE_RATE).astype(int), 0, len(noise) - 1)
    octaves = np.log2(np.maximum(frequencies[:, None], 1.0) / centre[at][None, :])
    gain = np.exp(-0.5 * (octaves / (width[at][None, :] / 2.355)) ** 2)
    gain /= np.sqrt(np.mean(gain ** 2, axis=0, keepdims=True))
    _, sound = istft(spectrum * gain, SAMPLE_RATE, nperseg=WINDOW, noverlap=WINDOW * 3 // 4)
    sound = sound[length:2 * length] if loop else sound[:length]
    return sound / sound.std()


def band(length, low, high, level, fall, rng):
    """Noise between `low` and `high` Hz, struck and dropping 20 dB every `fall` seconds."""
    return kit.decibels(level) * kit.band_noise(length, low, high, rng) * kit.falling(length, fall) \
        * kit.rising(length, 0.002)


def swish(shape, rng, size=1.0):
    """Something thin sweeping past fast: a band of air swelling to its peak and dying, brightest where loudest, as
    the blade's speed carries it — with the edge whistling an octave up."""
    length = kit.seconds(rng.uniform(*shape["length"]) * size)
    peak = rng.uniform(*shape["peak"])
    alpha = np.linspace(0.0, 1.0, length)
    swell = np.where(alpha < peak, (alpha / peak) ** 2.5, np.exp(-5.0 * (alpha - peak) / (1.0 - peak)))
    low, high = shape["band"]
    centre = low * (high / low) ** swell / size * rng.uniform(0.9, 1.1)
    air = filtered(rng.standard_normal(length), centre, shape["width"])
    edge = filtered(rng.standard_normal(length), 1.9 * centre, 0.08)
    return (air + kit.decibels(shape["whistle"]) * edge) * swell


def cut(rng):
    """A point stabbing out: a short swish, early to peak, over a puff of the air it shoves."""
    sound = swish(CUT, rng)
    return sound + band(len(sound), *PUFF, rng) * np.minimum(1.0, 2.0 * np.abs(sound).max())


def flurry(rng):
    """FLURRY_CUTS cuts one after another, brightening: every point firing in turn round the star."""
    sound = np.zeros(kit.seconds(FLURRY_GAP * FLURRY_CUTS + CUT["length"][1] * 1.2))
    for index in range(FLURRY_CUTS):
        stab = kit.sized(cut(rng), 1.0 / (1.0 + (FLURRY_CLIMB - 1.0) * index / (FLURRY_CUTS - 1)))
        kit.add(sound, stab, kit.seconds(index * FLURRY_GAP + rng.normal(0.0, 0.004)), kit.decibels(rng.uniform(-3, 0)))

    return sound


def flap(rng, light=False):
    """Cloth snapping taut: a few cracks within ~35 ms, dry mids and no ring, over the pop of the cloth's body."""
    length = kit.seconds(0.12)
    sound = np.zeros(length)
    for index in range(rng.integers(*FLAP_BURSTS)):
        crack = band(kit.seconds(0.02), *FLAP_BAND, 0.0, 0.006, rng)
        start = 0 if index == 0 else kit.seconds(rng.uniform(0.004, FLAP_SPAN))
        kit.add(sound, crack, start, 1.0 if index == 0 else kit.decibels(rng.uniform(-12, -4)))
    if not light:
        sound += band(length, *FLAP_BODY, rng)
    return sound


def land(rng):
    """A soft landing: weight coming down on a cushion, the air it pushes out and the cloth settling — no knock."""
    length = kit.seconds(0.45)
    sound = band(length, *LAND_BODY, rng) * kit.rising(length, 0.006) + band(length, *LAND_PUFF, rng)
    kit.add(sound, flap(rng, light=True), kit.seconds(0.015), kit.decibels(-10.0))
    return sound


def gust(rng):
    """Air blasting outward: broadband and bright on the blow, darkening and dying slowly, a low push under it and a
    howl riding it."""
    length = kit.seconds(GUST["length"])
    alpha = np.linspace(0.0, 1.0, length)
    start, end = GUST["band"]
    centre = start * (end / start) ** alpha ** 0.6
    envelope = kit.falling(length, GUST["fall"]) * kit.rising(length, 0.02)
    air = filtered(rng.standard_normal(length), centre, GUST["width"])
    howl = filtered(rng.standard_normal(length), 0.8 * centre, 0.1)
    return (air + kit.decibels(GUST["howl"]) * howl) * envelope + band(length, *GUST["push"], rng)


def inhale(seconds, rng):
    """Air drawn in, swelling and brightening into a hard cut: a gather, the file ending on the beat it leads."""
    length = kit.seconds(seconds)
    alpha = np.linspace(0.0, 1.0, length)
    low, high = INHALE_BAND
    centre = low * (high / low) ** alpha ** 1.5
    air = filtered(rng.standard_normal(length), centre, 1.4)
    howl = filtered(rng.standard_normal(length), 1.5 * centre, 0.07)
    return (air + kit.decibels(-14.0) * howl) * alpha ** 3


def vanish(rng):
    """A smoke-bomb poof: a burst of air thrown out at once and settling, the smoke's hiss left hanging after it."""
    length = kit.seconds(1.6)
    alpha = np.linspace(0.0, 1.0, length)
    burst = filtered(rng.standard_normal(length), 900.0 * (0.3 / 0.9) ** np.minimum(1.0, 3.0 * alpha), 2.0)
    sound = burst * kit.falling(length, 0.35) * kit.rising(length, 0.01)
    sound += band(length, 40.0, 160.0, -2.0, 0.3, rng)
    return sound + band(length, *VANISH_SMOKE, rng) * kit.rising(length, 0.08)


def one_shots():
    shots = {"Flurry": flurry(kit.generator("Ninja_Flurry")), "Gust": gust(kit.generator("Ninja_Gust")),
             "Land": land(kit.generator("Ninja_Land")), "Inhale": inhale(1.0, kit.generator("Ninja_Inhale")),
             "Inhale_Short": inhale(0.3, kit.generator("Ninja_Inhale_Short")),
             "Vanish": vanish(kit.generator("Ninja_Vanish"))}
    for index in range(VARIANTS):
        shots[f"Swish_{index + 1}"] = swish(SWISH, kit.generator(f"Ninja_Swish_{index + 1}"))
        shots[f"Cut_{index + 1}"] = cut(kit.generator(f"Ninja_Cut_{index + 1}"))
        shots[f"Flap_{index + 1}"] = flap(kit.generator(f"Ninja_Flap_{index + 1}"))
    for name, size in SIZES.items():
        shots[f"Whoosh_{name}"] = swish(WHOOSH, kit.generator(f"Ninja_Whoosh_{name}"), size)

    return shots


def cycles(length, count, rng):
    """A slow wander that closes on itself: `count` sines, each a whole number of cycles over `length` samples."""
    time = np.arange(length) / length
    return sum(np.sin(2 * np.pi * (index + 1) * time + rng.uniform(0, 2 * np.pi)) / (index + 1)
               for index in range(count))


def wind(length, rng):
    """The star's bed: foliage swaying — dry leaf ticks thickening and thinning in slow sways, over the soft hiss of
    the leaves' mass and a breath of air through them. Played faster the leaves shake harder."""
    sway = 0.5 + 0.5 * np.tanh(1.5 * cycles(length, SWAYS, rng))
    centre, width, level = LEAF_HISS
    hiss = filtered(rng.standard_normal(length), centre * 2.0 ** (0.4 * sway), width, loop=True)
    breath = filtered(rng.standard_normal(length), WIND_CENTRE * 2.0 ** (0.5 * sway), 1.5, loop=True)
    sound = kit.decibels(level) * hiss * (0.35 + 0.65 * sway)
    sound += kit.decibels(WIND_BREATH) * breath * (0.4 + 0.6 * sway)
    lowest, highest, fall = LEAF_TICK
    for start in rng.integers(0, length, int(LEAF_TICKS * LOOP_SECONDS)):
        if rng.uniform() < sway[start] ** 2:
            low = rng.uniform(lowest, highest)
            tick = band(kit.seconds(rng.uniform(0.004, 0.012)), low, 2.0 * low, 0.0, fall, rng)
            kit.add(sound, tick, start, 0.6 * kit.decibels(rng.uniform(-14, 0)), wrap=True)

    return sound


def whirl(length, rng):
    """Points sweeping past as the star turns: a swish per point, WHIRL_POINTS a turn at WHIRL_TURNS a second, each
    point its own loudness so a turn reads as one. Played faster it turns faster, and each swish brightens with it."""
    passes = WHIRL_POINTS * WHIRL_TURNS * length / SAMPLE_RATE
    phase = np.arange(length) / length * passes
    near = (0.5 - 0.5 * np.cos(2 * np.pi * phase)) ** 3
    points = kit.decibels(rng.uniform(-2.5, 0.0, WHIRL_POINTS))[np.floor(phase).astype(int) % WHIRL_POINTS]
    air = filtered(rng.standard_normal(length), WHIRL_CENTRE * 2.0 ** (0.6 * near - 0.3), 1.0, loop=True)
    return air * (0.12 + 0.88 * near) * points


def flutter(length, rng):
    """Cloth flapping hard in a gale: a snap about FLUTTER_RATE times a second, never quite even, over its rustle."""
    sound = 0.15 * filtered(rng.standard_normal(length), 700.0, 2.0, loop=True)
    gaps = rng.uniform(0.75, 1.25, int(LOOP_SECONDS * FLUTTER_RATE))
    for start in np.cumsum(gaps / gaps.sum() * length).astype(int) % length:
        kit.add(sound, flap(rng, light=rng.uniform() < 0.5), start, kit.decibels(rng.uniform(-8, 0)), wrap=True)

    return sound


def loops():
    """Seamless loops: the swaying foliage, the whirl of points and the cloth's flutter."""
    length = kit.seconds(LOOP_SECONDS)
    return {"Wind_Loop": wind(length, kit.generator("Ninja_Wind_Loop")),
            "Whirl_Loop": whirl(length, kit.generator("Ninja_Whirl_Loop")),
            "Flutter_Loop": flutter(length, kit.generator("Ninja_Flutter_Loop"))}


def write_all(folder):
    folder.mkdir(parents=True, exist_ok=True)
    DRAFT.mkdir(parents=True, exist_ok=True)
    paths, preview = {}, [np.zeros(kit.seconds(0.3))]
    for name, sound in one_shots().items():
        sound = kit.normalised(kit.fade_edges(sound))
        paths[name] = kit.save(folder / f"SFX_Ninja_{name}.wav", sound)
        preview += [sound, np.zeros(kit.seconds(0.5))]
    for name, sound in loops().items():
        sound = kit.normalised(sound)
        paths[name] = kit.save(folder / f"SFX_Ninja_{name}.wav", sound)
        preview += [np.tile(sound, 2), np.zeros(kit.seconds(0.5))]
    paths["Preview"] = kit.save(DRAFT / "Ninja_kit_preview.wav", np.concatenate(preview))
    return paths


if __name__ == "__main__":
    for path in write_all(SHIPPED if sys.argv[1:] == ["ship"] else DRAFT).values():
        print(path)
