# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy", "scipy"]
# ///
"""
A patch synth for game sound effects: layers of band-limited oscillators, FM pairs, inharmonic partials and shaped
noise, each under step envelopes, then drive, echo, a small room and bit reduction. A patch is JSON; the patches
reproducing references sit in Patches/, and fit_patch.py fits a patch's free values to its reference.

uv run AI/Python/Audio/sfx_patch.py <patch.json> <out.wav> [seed]

A patch: {"reference": its WAV under AI/Audio/References, "length": s, "layers": [layer, ...],
          "envelopes": {name: envelope}, "drive": dB, "echo": [s, feedback dB, mix dB],
          "room": [s to fall 60 dB, mix dB, brightness Hz], "bits": n, "hold": Hz}
          Any value written "@name" is the envelope of that name, shared by the layers using it.
A layer: {"name", "kind": tone | fm | partials | noise | blips, "start": s, "gain": dB, "level": envelope dB,
          "lowpass" / "highpass": envelope Hz, "resonance": dB at the lowpass, "drive": dB,
          "tremolo": [Hz, dB peak to peak, phase in cycles], "vibrato": [Hz, cents], ...kind's own keys}
          A fitted tremolo needs its phase free: out of phase with the reference, it only costs the fit.
  tone:     "wave": sine | triangle | square | saw | pulse, "width" (pulse), "pitch": envelope Hz,
            "voices": n, "detune": cents across them
  fm:       "pitch": envelope Hz, "ratio": modulator / carrier, "index": envelope
  partials: "partials": [[Hz, dB, s to fall 60 dB], ...], "pitch": envelope of a factor on all of them
  noise:    "tilt": dB per octave, "band": envelope Hz of a resonant bandpass's centre, "width": its octaves at -3 dB
            (skirts falling 6 dB per octave, as a real filter's)
  blips:    "rate": envelope of blips a second, "range": [low Hz, high Hz], "blip": s each, "wave", "width"
An envelope is a number or steps [[s since the previous step, value], ...], the first from the layer's start, like
an ADSR: dB and factors move straight between steps, Hz in octaves, and the last value holds.
Any number may be {"fit": [value, low, high]}: a free value for fit_patch.py, played at `value`.
"""
import json
import sys
import zlib

import numpy as np
from scipy.io import wavfile
from scipy.signal import fftconvolve, istft, stft

SAMPLE_RATE = 48000
NYQUIST_MARGIN = 0.95
TRIANGLE_HARMONICS = 40         # the 40th odd harmonic is 76 dB under the first
NOISE_WINDOW = 512              # the noise filter's frame: 10.7 ms, moved every quarter of it
SILENT_DB = -120.0


def plain(value):
    """The patch as played: every free value at its value."""
    if isinstance(value, dict):
        return value["fit"][0] if "fit" in value else {key: plain(item) for key, item in value.items()}
    if isinstance(value, list):
        return [plain(item) for item in value]
    return value


def shared(value, envelopes):
    """The patch with every "@name" replaced by the patch's envelope of that name."""
    if isinstance(value, str) and value.startswith("@"):
        return envelopes[value[1:]]
    if isinstance(value, dict):
        return {key: shared(item, envelopes) for key, item in value.items()}
    if isinstance(value, list):
        return [shared(item, envelopes) for item in value]
    return value


def envelope(steps, time, octaves=False):
    """Steps [[seconds since the previous step, value], ...]: durations, so a fitted time never folds it back."""
    if not isinstance(steps, list):
        return np.full(np.shape(time), float(steps))
    times = np.cumsum([step[0] for step in steps])
    values = np.array([step[1] for step in steps], float)
    if octaves:
        return np.exp2(np.interp(time, times, np.log2(np.maximum(values, 1e-3))))
    return np.interp(time, times, values)


def decibels(level):
    return 10.0 ** (np.asarray(level) / 20.0)


def filter_gain(frequency, layer, time):
    """The layer's lowpass (24 dB per octave, with its resonance) and highpass (24 dB per octave) at `frequency`."""
    gain = np.ones_like(frequency)
    if "lowpass" in layer:
        cutoff = envelope(layer["lowpass"], time, octaves=True)
        ratio = frequency / cutoff
        gain = gain / np.sqrt(1.0 + ratio ** 8)
        if layer.get("resonance", 0.0):
            gain = gain * decibels(layer["resonance"] * np.exp(-(np.log2(np.maximum(ratio, 1e-6)) / 0.15) ** 2))
    if "highpass" in layer:
        cutoff = envelope(layer["highpass"], time, octaves=True)
        gain = gain / np.sqrt(1.0 + (cutoff / np.maximum(frequency, 1e-3)) ** 8)
    return gain


def blep(phase, increment):
    """The polynomial that smooths a jump of 2 at phase 0 over the sample on each side of it."""
    correction = np.zeros_like(phase)
    before = phase < increment
    after = phase > 1.0 - increment
    early = phase[before] / increment[before]
    late = (phase[after] - 1.0) / increment[after]
    correction[before] = 2.0 * early - early * early - 1.0
    correction[after] = late * late + 2.0 * late + 1.0
    return correction


def oscillator(wave, width, frequency, rng):
    """One voice peaking near 1: a sine; a triangle from its first TRIANGLE_HARMONICS odd harmonics; a saw, square or
    pulse whose jumps are smoothed by PolyBLEP, so none folds back from above half the sample rate."""
    increment = frequency / SAMPLE_RATE
    phase = (np.cumsum(increment) + rng.uniform()) % 1.0
    if wave == "sine":
        return np.sin(2 * np.pi * phase)
    if wave == "triangle":
        sound = np.zeros(len(phase))
        for number in range(1, 2 * TRIANGLE_HARMONICS, 2):
            audible = number * frequency < NYQUIST_MARGIN * SAMPLE_RATE / 2
            sound += audible * (-1.0) ** ((number - 1) // 2) / number ** 2 * np.sin(2 * np.pi * number * phase)
        return sound * 8.0 / np.pi ** 2
    if wave == "saw":
        return 2.0 * phase - 1.0 - blep(phase, increment)
    duty = 0.5 if wave == "square" else width
    pulse = np.where(phase < duty, 1.0, -1.0) + blep(phase, increment) - blep((phase - duty) % 1.0, increment)
    return pulse - (2.0 * duty - 1.0)


def tone(layer, time, rng):
    """Voices of one oscillator detuned across `detune` cents, through the layer's filters."""
    pitch = envelope(layer["pitch"], time, octaves=True) * vibrato(layer, time)
    voices = int(layer.get("voices", 1))
    spread = np.linspace(-0.5, 0.5, voices) * layer.get("detune", 0.0) if voices > 1 else [0.0]
    sound = sum(oscillator(layer.get("wave", "sine"), layer.get("width", 0.5), pitch * 2.0 ** (cents / 1200.0), rng)
                for cents in spread) / np.sqrt(voices)
    filtered = any(key in layer for key in ("lowpass", "highpass", "band"))
    return shaped(sound, layer, time, 0.0) if filtered else sound


def fm(layer, time, rng):
    pitch = envelope(layer["pitch"], time, octaves=True) * vibrato(layer, time)
    carrier = 2 * np.pi * np.cumsum(pitch) / SAMPLE_RATE
    modulator = 2 * np.pi * np.cumsum(pitch * layer.get("ratio", 1.0)) / SAMPLE_RATE
    sound = np.sin(carrier + envelope(layer.get("index", 1.0), time) * np.sin(modulator) + rng.uniform(0, 2 * np.pi))
    return shaped(sound, layer, time, 0.0)


def partials(layer, time, rng):
    factor = envelope(layer.get("pitch", 1.0), time)
    sound = np.zeros(len(time))
    for hz, level, fall in layer["partials"]:
        frequency = hz * factor
        phase = 2 * np.pi * np.cumsum(frequency) / SAMPLE_RATE + rng.uniform(0, 2 * np.pi)
        audible = frequency < NYQUIST_MARGIN * SAMPLE_RATE / 2
        sound += decibels(level) * audible * filter_gain(frequency, layer, time) * 10.0 ** (-3.0 * time / fall) * np.sin(phase)

    return sound


def blips(layer, time, rng):
    """Short tones at random times and pitches, `rate` a second, each struck and dying 40 dB over `blip` seconds, its
    pitch drawn evenly in octaves across `range`: computer chatter, sparkle, glitch."""
    rate = envelope(layer.get("rate", 20.0), time)
    low, high = layer["range"]
    size = max(1, int(layer.get("blip", 0.03) * SAMPLE_RATE))
    decay = 10.0 ** (-2.0 * np.arange(size) / size) * np.clip(np.arange(size) / (0.001 * SAMPLE_RATE), 0.0, 1.0)
    sound = np.zeros(len(time) + size)
    moment = rng.exponential(1.0 / max(rate.max(), 1e-6))
    while moment < time[-1]:
        index = int(moment * SAMPLE_RATE)
        if rng.uniform() < rate[index] / max(rate.max(), 1e-6):
            pitch = np.full(size, low * (high / low) ** rng.uniform())
            sound[index:index + size] += decay * oscillator(layer.get("wave", "sine"), layer.get("width", 0.5), pitch, rng)
        moment += rng.exponential(1.0 / max(rate.max(), 1e-6))

    sound = sound[:len(time)]
    filtered = any(key in layer for key in ("lowpass", "highpass", "band"))
    return shaped(sound, layer, time, 0.0) if filtered else sound


def shaped(sound, layer, time, tilt):
    """`sound` through the layer's filters, band and tilt, frame by frame."""
    frequencies, frames, spectrum = stft(sound, SAMPLE_RATE, nperseg=NOISE_WINDOW, noverlap=NOISE_WINDOW * 3 // 4)
    frame_time = np.clip(frames, 0.0, time[-1])
    grid = np.maximum(frequencies[:, None], 1.0)
    gain = filter_gain(grid, layer, frame_time[None, :]) * (grid / 1000.0) ** (tilt / 6.02)
    if "band" in layer:
        centre = envelope(layer["band"], frame_time, octaves=True)[None, :]
        span = 2.0 ** layer.get("width", 1.0)
        quality = np.sqrt(span) / (span - 1.0)
        gain = gain / np.sqrt(1.0 + (quality * (grid / centre - centre / grid)) ** 2)
    _, out = istft(spectrum * gain, SAMPLE_RATE, nperseg=NOISE_WINDOW, noverlap=NOISE_WINDOW * 3 // 4)
    return out[:len(sound)]


def noise(layer, time, rng):
    sound = shaped(rng.standard_normal(len(time) + NOISE_WINDOW), layer, np.arange(len(time) + NOISE_WINDOW) / SAMPLE_RATE,
                   layer.get("tilt", 0.0))[:len(time)]
    return sound / max(np.sqrt(np.mean(sound ** 2)), 1e-12)


def vibrato(layer, time):
    rate, cents = layer.get("vibrato", [0.0, 0.0])
    return 2.0 ** (cents / 1200.0 * np.sin(2 * np.pi * rate * time))


def saturate(sound, drive_db):
    """Soft clipping `drive_db` into a tanh; 0 leaves the sound clean."""
    if drive_db <= 0:
        return sound
    drive = decibels(drive_db)
    return np.tanh(drive * sound) / drive


KINDS = {"tone": tone, "fm": fm, "partials": partials, "noise": noise, "blips": blips}


def render_layer(layer, length, seed):
    """One layer in the patch's time frame, starting at its own start."""
    rng = np.random.default_rng([seed, zlib.crc32(layer.get("name", layer["kind"]).encode())])
    start = int(layer.get("start", 0.0) * SAMPLE_RATE)
    time = np.arange(max(1, length - start)) / SAMPLE_RATE
    sound = KINDS[layer["kind"]](layer, time, rng)
    rate, depth, *phase = layer.get("tremolo", [0.0, 0.0])
    wobble = depth / 2 * np.sin(2 * np.pi * (rate * time + sum(phase)))
    level = envelope(layer.get("level", 0.0), time) + layer.get("gain", 0.0) + wobble
    sound = saturate(sound * decibels(np.maximum(level, SILENT_DB)), layer.get("drive", 0.0))
    return np.concatenate([np.zeros(start), sound])[:length]


def echo(sound, delay, feedback_db, mix_db):
    out = sound.copy()
    step = int(delay * SAMPLE_RATE)
    repeat = 1
    while step and repeat * step < len(sound) and feedback_db * (repeat - 1) > -60:
        out[repeat * step:] += decibels(mix_db + feedback_db * (repeat - 1)) * sound[:-repeat * step]
        repeat += 1

    return out


def room(sound, fall, mix_db, brightness, rng):
    """A small room: the sound convolved with noise dying 60 dB over `fall` seconds, its highs dying faster."""
    time = np.arange(int(fall * SAMPLE_RATE)) / SAMPLE_RATE
    tail = shaped(rng.standard_normal(len(time)) * 10.0 ** (-3.0 * time / fall), {"lowpass": brightness}, time, 0.0)
    wet = fftconvolve(sound, tail)[:len(sound)]
    return sound + decibels(mix_db) * wet * np.sqrt(np.mean(sound ** 2) / max(np.mean(wet ** 2), 1e-20))


def crush(sound, bits, hold):
    if hold:
        step = SAMPLE_RATE / hold
        sound = sound[(np.floor(np.arange(len(sound)) / step) * step).astype(int)]
    if bits:
        scale = 2.0 ** (bits - 1) / max(np.abs(sound).max(), 1e-12)
        sound = np.round(sound * scale) / scale
    return sound


def played(patch):
    """The patch as rendered: free values at their values, shared envelopes in place."""
    patch = plain(patch)
    return shared(patch, patch.get("envelopes", {}))


def layers(patch, seed=1):
    """Each layer on its own, before the master effects, by name."""
    patch = played(patch)
    length = int(patch["length"] * SAMPLE_RATE)
    return {layer.get("name", f"{layer['kind']}_{index}"): render_layer(layer, length, seed)
            for index, layer in enumerate(patch["layers"])}


def render(patch, seed=1):
    """The patch's mono sound, peaking at 1."""
    patch = played(patch)
    sound = saturate(sum(layers(patch, seed).values()), patch.get("drive", 0.0))
    if "echo" in patch:
        sound = echo(sound, *patch["echo"])
    if "room" in patch:
        sound = room(sound, *patch["room"], np.random.default_rng([seed, 7]))
    sound = crush(sound, patch.get("bits", 0), patch.get("hold", 0))
    return sound / max(np.abs(sound).max(), 1e-12)


def save(path, sound, peak_db=-1.0):
    wavfile.write(path, SAMPLE_RATE, np.round(sound / max(np.abs(sound).max(), 1e-12) * decibels(peak_db) * 32767).astype(np.int16))


def main():
    patch = json.load(open(sys.argv[1]))
    save(sys.argv[2], render(patch, int(sys.argv[3]) if len(sys.argv) > 3 else 1))
    print(sys.argv[2])


if __name__ == "__main__":
    main()
