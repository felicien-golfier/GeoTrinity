# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy", "scipy"]
# ///
"""
Synthesise a seamless electric loop — hum, corona, crackle, rumble, snaps, fizz — one WAV per layer for Audacity.

Run outside the editor: uv run AI/Python/Audio/electric_layers.py <out folder> [seconds] [seed]
Every layer is built on a circle — impulses wrap round, filters and swells are periodic — so the loop point
never clicks and the layers are never faded at their edges. Layers are written at their mix balance.
"""
import pathlib
import sys

import numpy as np
from scipy.io import wavfile

SAMPLE_RATE = 44100
MIX_PEAK = 10.0 ** (-1.0 / 20.0)
FILTER_ORDER = 4
LOUDNESS_WINDOW_SECONDS = 0.005

CRACKLE_IN_MIX_DB = -24.0
CRACKLE_PER_SECOND = 3000.0
DISCHARGE_DECAY_SECONDS = 0.0005
CRACKLE_BAND_HZ = (300.0, 16000.0)
AMPLITUDE_SPREAD = 0.5
FLICKER_FASTEST_HZ = 7.0
FLICKER_DEPTH = 0.5

RUMBLE_BAND_HZ = (35.0, 140.0)
RUMBLE_OVER_CRACKLE_DB = -9.0

SNAPS = {"per_second": 3.0, "seconds": (0.003, 0.012), "spikes_per_second": 20000.0, "gate_seconds": None,
         "band_hz": (1500.0, 18000.0), "over_crackle_db": 11.0}
FIZZES = {"per_second": 0.75, "seconds": (0.04, 0.15), "spikes_per_second": 3000.0, "gate_seconds": 0.006,
          "band_hz": (800.0, 18000.0), "over_crackle_db": 6.0}
FIZZ_GATE_OPEN_CHANCE = 0.6

SWELLS_PER_LOOP = 2
SWELL_FLOOR = 0.3
HUM_HZ = 120.0
HUM_HARMONICS = 60
HUM_BRIGHTNESS_HARMONICS = (3.0, 16.0)
HUM_BEND = 0.04
HUM_OVER_CRACKLE_DB = -3.0
CORONA_BAND_HZ = (2000.0, 16000.0)
CORONA_PULSE_SHARPNESS = 12.0
CORONA_OVER_CRACKLE_DB = -8.0


def band_shape(signal, band):
    frequencies = np.fft.rfftfreq(len(signal), 1.0 / SAMPLE_RATE)
    safe = np.maximum(frequencies, 1e-3)
    low, high = band
    response = 1.0 / np.sqrt((1.0 + (low / safe) ** (2 * FILTER_ORDER)) * (1.0 + (safe / high) ** (2 * FILTER_ORDER)))
    return np.fft.irfft(np.fft.rfft(signal) * response, len(signal))


def periodic_wobble(length, fastest_hz, rng):
    """Smooth random curve in [-1, 1] whose end joins its start."""
    spectrum = np.zeros(length // 2 + 1, dtype=complex)
    cycles = max(1, int(fastest_hz * length / SAMPLE_RATE))
    spectrum[1:cycles + 1] = rng.normal(size=cycles) + 1j * rng.normal(size=cycles)
    curve = np.fft.irfft(spectrum, length)
    return curve / np.max(np.abs(curve))


def spikes_at(length, positions, amplitudes, rng):
    train = np.zeros(length)
    np.add.at(train, np.asarray(positions) % length, amplitudes * rng.choice([-1.0, 1.0], size=len(amplitudes)))
    return train


def rms(signal):
    return np.sqrt(np.mean(signal**2))


def loudest_window_rms(signal):
    window = int(LOUDNESS_WINDOW_SECONDS * SAMPLE_RATE)
    return np.sqrt(np.max(np.convolve(signal**2, np.ones(window) / window, mode="same")))


def crackle_and_flicker(length, rng):
    """Discharges as short noise bursts, their density and level swelling with the flicker."""
    flicker = 1.0 + FLICKER_DEPTH * periodic_wobble(length, FLICKER_FASTEST_HZ, rng)
    chance = CRACKLE_PER_SECOND / SAMPLE_RATE * flicker / flicker.mean()
    strikes = np.where(rng.random(length) < chance, rng.lognormal(0.0, AMPLITUDE_SPREAD, length) * flicker, 0.0)
    decay = np.exp(-np.arange(length) / (DISCHARGE_DECAY_SECONDS * SAMPLE_RATE))
    envelope = np.fft.irfft(np.fft.rfft(strikes) * np.fft.rfft(decay), length)
    return band_shape(rng.normal(size=length) * envelope, CRACKLE_BAND_HZ), flicker


def rumble(length, flicker, crackle, rng):
    body = band_shape(rng.normal(size=length), RUMBLE_BAND_HZ) * flicker
    return body * rms(crackle) / rms(body) * 10.0 ** (RUMBLE_OVER_CRACKLE_DB / 20.0)


def bursts(length, crackle, settings, rng):
    """Discharges scattered at random: each a spike cluster, optionally sputtering, decaying from its start."""
    positions, amplitudes = [], []
    for start in rng.integers(0, length, rng.poisson(settings["per_second"] * length / SAMPLE_RATE)):
        burst_length = int(rng.uniform(*settings["seconds"]) * SAMPLE_RATE)
        offsets = np.flatnonzero(rng.random(burst_length) < settings["spikes_per_second"] / SAMPLE_RATE)
        if settings["gate_seconds"]:
            gate_length = int(settings["gate_seconds"] * SAMPLE_RATE)
            gate_open = rng.random(burst_length // gate_length + 1) < FIZZ_GATE_OPEN_CHANCE
            offsets = offsets[gate_open[offsets // gate_length]]

        positions.extend(start + offsets)
        amplitudes.extend(rng.lognormal(0.0, AMPLITUDE_SPREAD, len(offsets)) * np.exp(-4.0 * offsets / burst_length))
    layer = band_shape(spikes_at(length, positions, np.array(amplitudes), rng), settings["band_hz"])
    return layer * rms(crackle) / loudest_window_rms(layer) * 10.0 ** (settings["over_crackle_db"] / 20.0)


def swell_and_hum_phase(length):
    """Swell from 0 to 1 and back SWELLS_PER_LOOP times; the hum's phase bends up with it and ends on a whole cycle."""
    swell = 0.5 - 0.5 * np.cos(2.0 * np.pi * SWELLS_PER_LOOP * np.arange(length) / length)
    base_hz = round(HUM_HZ * length / SAMPLE_RATE) * SAMPLE_RATE / length
    frequency = base_hz * (1.0 + HUM_BEND * (swell - 0.5))
    return swell, 2.0 * np.pi * np.cumsum(frequency) / SAMPLE_RATE


def hum(length, swell, phase, crackle):
    """Transformer buzz: harmonics rolling off, brighter and louder at each swell's crest."""
    low, high = HUM_BRIGHTNESS_HARMONICS
    brightness = low + (high - low) * swell
    buzz = sum(np.exp(-(harmonic - 1) / brightness) * np.sin(harmonic * phase)
               for harmonic in range(1, HUM_HARMONICS + 1))
    buzz *= SWELL_FLOOR + (1.0 - SWELL_FLOOR) * swell
    return buzz * rms(crackle) / rms(buzz) * 10.0 ** (HUM_OVER_CRACKLE_DB / 20.0)


def corona(length, swell, phase, crackle, rng):
    """Sizzle pulsing once per hum cycle, the sound of the air breaking down round a live conductor."""
    pulse = ((1.0 + np.cos(phase)) / 2.0) ** CORONA_PULSE_SHARPNESS
    sizzle = band_shape(rng.normal(size=length), CORONA_BAND_HZ) * pulse * (SWELL_FLOOR + (1.0 - SWELL_FLOOR) * swell)
    return sizzle * rms(crackle) / rms(sizzle) * 10.0 ** (CORONA_OVER_CRACKLE_DB / 20.0)


def main():
    out_folder = pathlib.Path(sys.argv[1])
    seconds = float(sys.argv[2]) if len(sys.argv) > 2 else 4.0
    rng = np.random.default_rng(int(sys.argv[3]) if len(sys.argv) > 3 else 1)
    length = int(seconds * SAMPLE_RATE)
    crackle, flicker = crackle_and_flicker(length, rng)
    swell, phase = swell_and_hum_phase(length)
    layers = {
        "Rumble": rumble(length, flicker, crackle, rng),
        "Snaps": bursts(length, crackle, SNAPS, rng),
        "Fizz": bursts(length, crackle, FIZZES, rng),
        "Hum": hum(length, swell, phase, crackle),
        "Corona": corona(length, swell, phase, crackle, rng),
        "Crackle": crackle * 10.0 ** (CRACKLE_IN_MIX_DB / 20.0),
    }
    gain = MIX_PEAK / np.max(np.abs(sum(layers.values())))
    out_folder.mkdir(parents=True, exist_ok=True)
    for name, layer in layers.items():
        path = out_folder / f"{name}.wav"
        wavfile.write(path, SAMPLE_RATE, (layer * gain).astype(np.float32))
        print(f"{path}  rms {20 * np.log10(rms(layer * gain)):.1f} dBFS")


if __name__ == "__main__":
    main()
