# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy", "scipy"]
# ///
"""
The zap kit: small electric hits, heard on every hit of a boosted shot, built on the electricity rules of
GameSoundDesign.md, measured from a spell pack's electric shocks: nothing in it repeats a cycle. A hash of discharges
clumping into shrinking bursts, lows lurching under it, a sizzle following the clumps, an arc or two whistling
through, the whole soft-clipped into one object.

Run outside the editor: uv run AI/Python/Audio/zap_kit.py [ship]
Writes every variant of every style as SFX_Elec_<Style>_<n>.wav into DRAFT, or into SourceArt/Audio/Elec with
`ship`, plus a preview — each style's variants in turn, then as a run of hits — into DRAFT.
"""
import pathlib
import sys

import numpy as np

sys.path.insert(0, str(pathlib.Path(__file__).parent))
import mech_kit as kit  # noqa: E402

DRAFT = kit.PROJECT / "AI" / "Audio" / "Drafts" / "Elec"
SHIPPED = kit.PROJECT / "SourceArt" / "Audio" / "Elec"
VARIANTS = 4
RUN_GAP = 0.11            # seconds between hits in the preview's run, an auto-fire's rate
DRIVE = 1.5               # soft clip over the mix, fusing the layers
DISCHARGE_FALL = 0.0006   # seconds for one discharge to fall 20 dB
LURCH_STEPS = (0.002, 0.01)  # seconds between the lows' jumps
LOWS_BAND = (40.0, 300.0)
TILT = -4.5               # dB an octave the hash and sizzle lose above TILT_FROM Hz, as a discharge's spectrum does
TILT_FROM = 500.0
CLUMP_DEPTH = 0.5         # share of the level the clumps stutter, ~6 dB between a clump and the dip after it

# length: seconds. hold: seconds at full level before it dies, fall: seconds to fall 20 dB after. discharges: a
# second at a clump's height. clumps: the gap range between them in seconds, and dB lost per clump. lows, sizzle: dB,
# the sizzle's band. arcs: how many, the band their glides start in, their seconds, dB.
STYLES = {
    "Zap": {"length": 0.26, "hold": 0.06, "fall": 0.18, "discharges": 900.0, "clumps": ((0.015, 0.04), -2.0),
            "lows": -2.0, "sizzle": (-6.0, (2000.0, 14000.0)), "arcs": (2, (1500.0, 6000.0), (0.03, 0.08), -20.0)},
    "Crackle": {"length": 0.18, "hold": 0.03, "fall": 0.12, "discharges": 1200.0, "clumps": ((0.012, 0.03), -2.5),
                "lows": -2.0, "sizzle": (-5.0, (2500.0, 14000.0)), "arcs": (0, (2500.0, 8000.0), (0.03, 0.06), -21.0)},
}


def clumps(length, gaps, loss, rng):
    """The level stuttering in bursts at uneven gaps, each struck and sagging, shrinking by `loss` dB a burst."""
    envelope = np.zeros(length)
    start, index = 0, 0
    while start < length:
        span = kit.seconds(rng.uniform(*gaps))
        burst = kit.decibels(index * loss + rng.uniform(-4.0, 0.0)) * kit.falling(2 * span, 0.05) \
            * kit.rising(2 * span, 0.002)
        kit.add(envelope, burst, start)
        start, index = start + span, index + 1

    return 1.0 - CLUMP_DEPTH + CLUMP_DEPTH * envelope / envelope.max()


def tilted(signal):
    """`signal` losing TILT dB an octave above TILT_FROM Hz."""
    frequencies = np.fft.rfftfreq(len(signal), 1.0 / kit.SAMPLE_RATE)
    gain = kit.decibels(TILT * np.log2(np.maximum(frequencies, TILT_FROM) / TILT_FROM))
    return np.fft.irfft(np.fft.rfft(signal) * gain, len(signal))


def hash_of_discharges(length, rate, rng):
    """Discharges at `rate` a second, each a broadband burst dying in DISCHARGE_FALL, loudness heavy-tailed so a few
    snap out of the hash."""
    sound = np.zeros(length)
    size = kit.seconds(8 * DISCHARGE_FALL)
    for start in rng.integers(0, length, int(rate * length / kit.SAMPLE_RATE)):
        burst = rng.standard_normal(size) * kit.falling(size, DISCHARGE_FALL)
        kit.add(sound, burst, start, min(4.0, 0.3 + rng.pareto(2.5)))

    return sound / sound.std()


def lurch(length, rng):
    """Lows jumping between levels at uneven steps, smoothed into the low band: a lurch, never a hum."""
    steps = np.cumsum(rng.uniform(*LURCH_STEPS, length // kit.seconds(LURCH_STEPS[0]) + 1) * kit.SAMPLE_RATE)
    levels = rng.standard_normal(len(steps))
    held = levels[np.searchsorted(steps, np.arange(length))]
    spectrum = np.fft.rfft(held)
    frequencies = np.fft.rfftfreq(length, 1.0 / kit.SAMPLE_RATE)
    spectrum[(frequencies < LOWS_BAND[0]) | (frequencies > LOWS_BAND[1])] = 0.0
    lows = np.fft.irfft(spectrum, length)
    return lows / lows.std()


def arc(band, seconds, rng):
    """A narrow whistle gliding up or down an octave, its pitch wavering as it goes."""
    length = kit.seconds(rng.uniform(*seconds))
    start_hz = rng.uniform(*band)
    end_hz = start_hz * 2.0 ** rng.choice([-1.0, 1.0]) * rng.uniform(0.7, 1.0)
    alpha = np.linspace(0.0, 1.0, length)
    waver = 1.0 + 0.03 * np.cumsum(rng.standard_normal(length)) / np.sqrt(length)
    phase = 2 * np.pi * np.cumsum(start_hz * (end_hz / start_hz) ** alpha * waver) / kit.SAMPLE_RATE
    return np.sin(phase) * np.sin(np.pi * alpha) ** 2


def zap(style, rng):
    length = kit.seconds(style["length"])
    time = kit.timeline(length)
    shape = kit.rising(length, 0.004) * 10.0 ** (-np.maximum(0.0, time - style["hold"]) / style["fall"])
    envelope = shape * clumps(length, *style["clumps"], rng)
    sizzle_level, band = style["sizzle"]
    crackle = tilted(hash_of_discharges(length, style["discharges"], rng)
                     + kit.decibels(sizzle_level) * kit.band_noise(length, *band, rng))
    lows = lurch(length, rng)
    sound = envelope * (crackle / crackle.std() + kit.decibels(style["lows"]) * lows)
    count, arc_band, seconds, level = style["arcs"]
    for start in rng.uniform(0.0, 0.5 * style["length"], count):
        whistle = arc(arc_band, seconds, rng) * shape[kit.seconds(start)]
        kit.add(sound, whistle, kit.seconds(start), kit.decibels(level) * np.abs(sound).max())

    sound = sound / np.abs(sound).max()
    return np.tanh(DRIVE * sound) / np.tanh(DRIVE)


def write_all(folder):
    folder.mkdir(parents=True, exist_ok=True)
    DRAFT.mkdir(parents=True, exist_ok=True)
    paths, preview = {}, [np.zeros(kit.seconds(0.3))]
    for style_name, style in STYLES.items():
        variants = []
        for index in range(VARIANTS):
            name = f"{style_name}_{index + 1}"
            sound = kit.normalised(kit.fade_edges(zap(style, kit.generator(f"Elec_{name}"))))
            paths[name] = kit.save(folder / f"SFX_Elec_{name}.wav", sound)
            variants.append(sound)
            preview += [sound, np.zeros(kit.seconds(0.4))]
        run = np.zeros(kit.seconds(RUN_GAP * 12 + 0.3))
        for hit in range(12):
            kit.add(run, variants[hit % VARIANTS], kit.seconds(hit * RUN_GAP))
        preview += [run / np.abs(run).max() * kit.PEAK, np.zeros(kit.seconds(0.8))]
    paths["Preview"] = kit.save(DRAFT / "Elec_zap_preview.wav", np.concatenate(preview))
    return paths


if __name__ == "__main__":
    for path in write_all(SHIPPED if sys.argv[1:] == ["ship"] else DRAFT).values():
        print(path)
