# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy", "scipy"]
# ///
"""
Synthesise the reusable metal one-shots — cracks, latch clicks, clinks, ratchet ticks, rattle taps — into
SourceArt/Audio/Metal.

Run outside the editor: uv run AI/Python/Audio/metal_one_shots.py
Every file is its own sample at -1 dBFS peak; loudness lives in whatever plays it. A metal part rings as a free bar:
partials at 1, 2.756, 5.404 and 8.933 times its fundamental, the higher ones quieter and shorter.
"""
import pathlib
import sys

import numpy as np
from scipy.io import wavfile
from scipy.signal import butter, sosfilt

sys.path.insert(0, str(pathlib.Path(__file__).parent))
import hex_lock_layers  # noqa: E402

SAMPLE_RATE = 48000
OUT_FOLDER = pathlib.Path(__file__).resolve().parents[3] / "SourceArt" / "Audio" / "Metal"
PEAK = 10.0 ** (-1.0 / 20.0)
EDGE_FADE_SECONDS = 0.002
TRIM_BELOW_PEAK_DB = -60.0
HIGH_CUT_HZ = 16500.0
BAR_RATIOS = [1.0, 2.756, 5.404, 8.933]

# (name, fundamental Hz): the lock's own clicks, larger parts lower
LOCK_CLICKS = [("Low", 1100.0), ("Mid", 1400.0), ("High", 1750.0)]
CLINK_FUNDAMENTALS = [2350.0, 2650.0, 2950.0, 3300.0, 3700.0, 4150.0]  # about two semitones apart
RATCHET_TICK_VARIANTS = 4
TAP_FUNDAMENTALS = [950.0, 1250.0, 1600.0, 2050.0, 2600.0, 3300.0]
# A bike freewheel's tick, matched to one: its hub rings as (Hz, level dB, decay ms) partials over a 2 ms snap
# of noise — (low Hz, high Hz, level dB, decay ms) — with a small knock of the wheel below 600 Hz
RATCHET_PARTIALS = [(1898, -16, 7.0), (2730, -16, 6.0), (4500, -14, 8.0), (8209, -12, 8.0), (10365, -12, 6.0),
                    (12500, -8, 4.0), (14180, -9, 4.0)]
RATCHET_NOISE = [(1200, 17000, -7, 2.5), (8000, 17000, -8, 1.5), (2500, 17000, -22, 8.0), (120, 350, -14, 3.0)]
SNAP_ATTACK_MS = 0.8


def seconds(length):
    return np.arange(int(length * SAMPLE_RATE)) / SAMPLE_RATE


def ring(frequencies, levels_db, decays_ms, length, rng, attack_ms=0.2):
    """Decaying sine partials struck together, each from a random phase; those past the high cut are dropped."""
    time = seconds(length)
    rise = np.clip(time / (attack_ms / 1000.0), 0.0, 1.0)
    out = np.zeros_like(time)
    for frequency, level, decay in zip(frequencies, levels_db, decays_ms):
        if frequency < HIGH_CUT_HZ:
            out += 10 ** (level / 20) * np.exp(-time / (decay / 1000.0)) * np.sin(
                2 * np.pi * frequency * time + rng.uniform(0, 2 * np.pi))

    return out * rise


def burst(low_hz, high_hz, level_db, decay_ms, length, rng, attack_ms=0.0):
    """Band-passed noise struck and decaying exponentially: the snap of a contact."""
    time = seconds(length)
    noise = sosfilt(butter(4, [low_hz, min(high_hz, HIGH_CUT_HZ)], "bp", fs=SAMPLE_RATE, output="sos"),
                    rng.normal(size=len(time)))
    rise = np.clip(time / (attack_ms / 1000.0), 0.0, 1.0) if attack_ms else 1.0
    return 10 ** (level_db / 20) * noise / noise.std() * np.exp(-time / (decay_ms / 1000.0)) * rise


def bar(fundamental, levels_db, decays_ms, length, rng, attack_ms=0.2):
    return ring([fundamental * ratio for ratio in BAR_RATIOS], levels_db, decays_ms, length, rng, attack_ms)


def delayed(sound, delay_ms):
    return np.concatenate([np.zeros(int(delay_ms / 1000.0 * SAMPLE_RATE)), sound])[:len(sound)]


def lock_click(fundamental, rng):
    """A latch: the bolt strikes, then seats 9 ms later a little lower and softer, each a snap over a ringing bar."""
    def strike(pitch):
        return (bar(pitch, [0, -4, -10, -16], [120, 45, 20, 10], 0.85, rng)
                + burst(2000, 14000, -6, 1.5, 0.85, rng))

    return strike(fundamental) + 0.5 * delayed(strike(fundamental * 0.985), 9.0)


def clink(fundamental, rng):
    """A small loose part touching another: a soft-edged bar ring, no low end."""
    return (bar(fundamental, [0, -8, -18, -30], [90, 35, 15, 8], 0.65, rng, attack_ms=1.0)
            + burst(4000, 14000, -14, 0.5, 0.65, rng))


def ratchet_tick(rng):
    """One pawl drop: a broadband snap over the hub's short high ring, its pitch a little off each time."""
    detune = rng.uniform(0.97, 1.03)
    frequencies, levels, decays = zip(*RATCHET_PARTIALS)
    return (ring([f * detune for f in frequencies], levels, decays, 0.06, rng, attack_ms=SNAP_ATTACK_MS)
            + sum(burst(low, high, level, decay, 0.06, rng, SNAP_ATTACK_MS) for low, high, level, decay in RATCHET_NOISE))


def tap(fundamental, rng):
    """Two loose metal parts knocking once: a dry snap with the barest ring, the grain of a rattle."""
    return (bar(fundamental, [0, -6, -14, -22], [14, 7, 4, 2], 0.1, rng)
            + burst(800, 12000, -4, 1.0, 0.1, rng))


def write(name, sound):
    """Trimmed below -60 dB of its peak at both ends, normalised, edge-faded, 16-bit."""
    sound = sound if sound.ndim == 2 else np.stack([sound, sound], axis=1)
    envelope = np.abs(sound).max(axis=1)
    loud = np.flatnonzero(envelope > envelope.max() * 10 ** (TRIM_BELOW_PEAK_DB / 20))
    sound = sound[loud[0]:loud[-1] + 1] * (PEAK / envelope.max())
    ramp = np.linspace(0.0, 1.0, int(EDGE_FADE_SECONDS * SAMPLE_RATE))[:, None]
    sound[-len(ramp):] *= ramp[::-1]
    path = OUT_FOLDER / f"SFX_Metal_{name}.wav"
    wavfile.write(path, SAMPLE_RATE, np.round(sound * 32767).astype(np.int16))
    print(f"{path}  {len(sound) / SAMPLE_RATE * 1000:.0f} ms")


def main():
    OUT_FOLDER.mkdir(parents=True, exist_ok=True)
    rng = np.random.default_rng(1)
    variants = hex_lock_layers.json.loads(hex_lock_layers.PARAMS_PATH.read_text())
    write("Crack", hex_lock_layers.layers(variants["low"], 1)["Crack"])
    write("CrackBright", hex_lock_layers.layers(variants["high"], 1)["Crack"])
    for name, fundamental in LOCK_CLICKS:
        write(f"LockClick_{name}", lock_click(fundamental, rng))
    for index, fundamental in enumerate(CLINK_FUNDAMENTALS):
        write(f"Clink_{index + 1}", clink(fundamental, rng))
    for index in range(RATCHET_TICK_VARIANTS):
        write(f"RatchetTick_{index + 1}", ratchet_tick(rng))
    for index, fundamental in enumerate(TAP_FUNDAMENTALS):
        write(f"Tap_{index + 1}", tap(fundamental, rng))


if __name__ == "__main__":
    main()
