# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy", "scipy"]
# ///
"""
Synthesise the hex boss intro's lock — the trailer clack — one WAV per layer for Audacity.

Run outside the editor: uv run AI/Python/Audio/hex_lock_layers.py <out folder> [variant] [seed]
A model for match_reference.py: every layer is noise split into third octaves, each band under its own envelope.
Variants are fitted values in hex_lock_params.json — "low", the clack heard three times, and "high", the brighter
first one that swells in. Files start at the swell's first sound; the printout gives how far that leads the strike.
"""
import functools
import json
import pathlib
import sys

import numpy as np
from scipy.io import wavfile

SAMPLE_RATE = 48000
ONSET_SECONDS = 0.12  # room for the swell before the strike
LENGTH_SECONDS = 0.62
EDGE_FADE_SECONDS = 0.002
TRIM_BELOW_PEAK_DB = -60.0
MIX_PEAK = 10.0 ** (-1.0 / 20.0)
BANDS = 1000 * 2 ** (np.arange(-12, 15) / 3)  # third octaves, 63 Hz .. 25 kHz
LOW_CUT_HZ = 120.0
HIGH_CUT_HZ = 16500.0  # the reference's own ceiling
STRIKES_MS = [0.0, 2.5, 4.8, 32.5, 33.5, 40.5, 45.5, 83.0, 128.5]
KNOCK_SECOND_MS = 33.0
PARAMS_PATH = pathlib.Path(__file__).with_name("hex_lock_params.json")

PARAMS = [  # name, initial, low, high — levels in dB, tilts in dB per octave, rates in dB per second
    ("crack_level", -20, -70, 10), ("crack_lo_hz", 1500, 200, 8000), ("crack_tilt", -3, -14, 4),
    ("crack_attack_ms", 0.3, 0.05, 3), ("crack_decay_ms", 1.0, 0.2, 10),
] + [(f"strike_{i}", 0, -45, 10) for i in range(len(STRIKES_MS))] + [
    ("rattle_level", -25, -70, 10), ("rattle_lo_hz", 1000, 150, 8000), ("rattle_tilt", -4, -14, 4),
    ("rattle_attack_ms", 4, 0.3, 30), ("rattle_hold_ms", 40, 0, 90), ("rattle_decay_dbs", 75, 5, 500),
    ("knock_level", -15, -70, 10), ("knock_f0", 1000, 250, 4000), ("knock_f1", 350, 120, 2000),
    ("knock_glide_ms", 20, 1, 150), ("knock_width_oct", 0.8, 0.15, 3), ("knock_attack_ms", 2, 0.2, 20),
    ("knock_decay_ms", 40, 3, 300), ("knock_second", -6, -45, 8),
    ("room_level", -30, -80, 0), ("room_lo_hz", 300, 80, 3000), ("room_hi_hz", 12000, 2000, 22000),
    ("room_tilt", -3, -14, 4), ("room_attack_ms", 15, 0.5, 80), ("room_decay_lo_dbs", 50, 5, 400),
    ("room_decay_hi_dbs", 120, 5, 800),
    ("swell_level", -60, -90, 10), ("swell_rise_ms", 30, 3, 200), ("swell_lo_hz", 1000, 150, 8000),
    ("swell_tilt", -3, -14, 4),
]
LAYERS = ["Crack", "Rattle", "Knock", "Room", "Swell"]

TIME = np.arange(int(LENGTH_SECONDS * SAMPLE_RATE)) / SAMPLE_RATE - ONSET_SECONDS
FREQUENCY = BANDS[:, None]


def stereo_correlation(frequency):
    """The reference's left-right coherence: near mono below 600 Hz, wide above 2 kHz."""
    octaves = np.clip(np.log2(frequency / 600.0) / np.log2(2000.0 / 600.0), 0.0, 1.0)
    return 0.85 - 0.65 * octaves


@functools.lru_cache(maxsize=16)
def band_noise(seed):
    """Unit noise per band and channel; the cos² band shapes sum back to flat noise."""
    rng = np.random.default_rng(seed)
    length = len(TIME)
    padded = 2 * length
    frequencies = np.fft.rfftfreq(padded, 1.0 / SAMPLE_RATE)
    distance = (np.log2(np.maximum(frequencies, 1.0))[None, :] - np.log2(BANDS)[:, None]) * 3
    shapes = np.where(np.abs(distance) < 1, np.cos(np.pi / 2 * distance) ** 2, 0.0)
    shapes[0, distance[0] < 0] = 1.0
    shapes[-1, distance[-1] > 0] = 1.0
    common, own = np.fft.rfft(rng.normal(size=padded)), np.fft.rfft(rng.normal(size=padded))
    rho = stereo_correlation(BANDS)[:, None]
    left = np.fft.irfft(common[None, :] * shapes, padded)[:, :length]
    right = np.fft.irfft((rho * common[None, :] + np.sqrt(1 - rho ** 2) * own[None, :]) * shapes, padded)[:, :length]
    return np.stack([left, right], axis=1)


def spectrum(level_db, low_hz, tilt, high_hz=20000.0):
    shape = 1 / np.sqrt((1 + (low_hz / FREQUENCY) ** 4) * (1 + (FREQUENCY / high_hz) ** 8))
    return 10 ** (level_db / 20) * shape * (FREQUENCY / 1000.0) ** (tilt / 6.02)


def struck(time, attack, decay):
    """0 before the strike, a linear rise over `attack`, then an exponential fall."""
    return np.clip(time / attack, 0, 1) * np.exp(-np.maximum(time, 0) / decay)


def crack(p):
    """Micro-clicks at the measured strike times, each a bright snap."""
    strikes = sum(10 ** (p[f"strike_{i}"] / 20) * struck(TIME - ms / 1000, p["crack_attack_ms"] / 1000, p["crack_decay_ms"] / 1000)
                  for i, ms in enumerate(STRIKES_MS))
    return spectrum(p["crack_level"], p["crack_lo_hz"], p["crack_tilt"]) * strikes


def rattle(p):
    """The dense crackle between the strikes: rises, holds, then dies at a fixed rate."""
    rise = np.clip(TIME / (p["rattle_attack_ms"] / 1000), 0, 1)
    held = np.maximum(TIME - p["rattle_hold_ms"] / 1000, 0)
    return spectrum(p["rattle_level"], p["rattle_lo_hz"], p["rattle_tilt"]) * rise * 10 ** (-p["rattle_decay_dbs"] * held / 20)


def knock(p):
    """The body: a band of noise whose centre glides, struck again by the second strike."""
    after = np.maximum(TIME, 0)
    centre = p["knock_f1"] + (p["knock_f0"] - p["knock_f1"]) * np.exp(-after / (p["knock_glide_ms"] / 1000))
    band = np.exp(-0.5 * (np.log2(FREQUENCY / centre[None, :]) / p["knock_width_oct"]) ** 2)
    attack, decay = p["knock_attack_ms"] / 1000, p["knock_decay_ms"] / 1000
    strikes = struck(TIME, attack, decay) + 10 ** (p["knock_second"] / 20) * struck(TIME - KNOCK_SECOND_MS / 1000, attack, decay)
    return 10 ** (p["knock_level"] / 20) * band * strikes[None, :]


def room(p):
    """Diffuse tail; its highs die faster than its lows."""
    octaves = np.clip(np.log2(FREQUENCY / 500.0) / 5, 0, 1)
    rate = p["room_decay_lo_dbs"] + (p["room_decay_hi_dbs"] - p["room_decay_lo_dbs"]) * octaves
    rise = np.clip(TIME / (p["room_attack_ms"] / 1000), 0, 1)
    return spectrum(p["room_level"], p["room_lo_hz"], p["room_tilt"], p["room_hi_hz"]) * rise * 10 ** (-rate * np.maximum(TIME, 0) / 20)


def swell(p):
    """Noise rising into the strike and cut just after it."""
    rise = np.where(TIME < 0, np.exp(TIME / (p["swell_rise_ms"] / 1000)), np.exp(-TIME / 0.003))
    return spectrum(p["swell_level"], p["swell_lo_hz"], p["swell_tilt"]) * rise


def layers(p, seed):
    """Each layer on its own noise, so the layers add up to the mix."""
    limits = 1 / np.sqrt((1 + (LOW_CUT_HZ / FREQUENCY) ** 8) * (1 + (FREQUENCY / HIGH_CUT_HZ) ** 16))
    return {name: np.einsum("bn,bcn->nc", limits * envelope(p), band_noise(seed * len(LAYERS) + index))
            for index, (name, envelope) in enumerate(zip(LAYERS, (crack, rattle, knock, room, swell)))}


def render(p, seed):
    return sum(layers(p, seed).values())


def main():
    out_folder = pathlib.Path(sys.argv[1])
    variant = sys.argv[2] if len(sys.argv) > 2 else "low"
    seed = int(sys.argv[3]) if len(sys.argv) > 3 else 1
    parts = layers(json.loads(PARAMS_PATH.read_text())[variant], seed)
    mix = sum(parts.values())
    envelope = np.abs(mix).max(axis=1)
    start = int(np.flatnonzero(envelope > envelope.max() * 10 ** (TRIM_BELOW_PEAK_DB / 20))[0])
    gain = MIX_PEAK / envelope.max()
    ramp = np.linspace(0.0, 1.0, int(EDGE_FADE_SECONDS * SAMPLE_RATE))[:, None]
    out_folder.mkdir(parents=True, exist_ok=True)
    for name, layer in list(parts.items()) + [("Mix_preview", mix)]:
        layer = layer[start:] * gain
        layer[:len(ramp)] *= ramp
        layer[-len(ramp):] *= ramp[::-1]
        wavfile.write(out_folder / f"{name}.wav", SAMPLE_RATE, np.round(layer * 32767).astype(np.int16))
        print(f"{out_folder / name}.wav  peak {20 * np.log10(np.abs(layer).max() + 1e-12):.1f} dBFS")

    print(f"strike at {(int(ONSET_SECONDS * SAMPLE_RATE) - start) / SAMPLE_RATE * 1000:.0f} ms into the file")


if __name__ == "__main__":
    main()
