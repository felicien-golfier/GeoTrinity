# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy", "scipy", "matplotlib"]
# ///
"""
Match a synthesised sound to a hit in a reference recording: find the hits, measure one with its backing
cancelled, fit a parametric synth to it, compare the result.

Run outside the editor: uv run AI/Python/Audio/match_reference.py <command> ...
  onsets  <ref.wav> [from_s to_s]                  hits: sharp rises of the energy above 2 and 5 kHz
  repeats <ref.wav> <onset_s> <onset_s> [...]      aligns each hit on the first; band correlation over time
  target  <ref.wav> <out.npz> pair <onset_s> <onset_s>
  target  <ref.wav> <out.npz> single <onset_s> <bed_from_s> <bed_to_s> [ceiling_below_hz]
          (bed relative to the onset, negative; bands below ceiling_below_hz only cap the synth)
  fit     <model.py> <target.npz> <params.json> [restarts] [evaluations]
  compare <model.py> <params.json> <target.npz> <out.png>
A model file defines PARAMS [(name, initial, low, high)], ONSET_SECONDS and render(values, seed) -> stereo array.
A pair target is the cross-spectrum of two copies of one sample: their backings are unrelated and cancel.
A single target's low bands can hold backing the bed window missed: there the synth may stay under it.
"""
import importlib.util
import json
import sys

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from scipy.io import wavfile
from scipy.optimize import least_squares
from scipy.signal import butter, fftconvolve, sosfiltfilt

SAMPLE_RATE = 48000
FFT_SIZE, HOP = 512, 48  # 10.7 ms windows every 1 ms
FINE_SIZE, FINE_HOP = 128, 24  # 2.7 ms windows every 0.5 ms, for the transient
GRID_TIMES = np.arange(-0.12, 0.3, HOP / SAMPLE_RATE)
FINE_TIMES = np.arange(-0.03, 0.06, FINE_HOP / SAMPLE_RATE)
PAIR_FROM_SECONDS = -0.005  # before its onset a pair shares nothing: the sample is silent there
TRANSIENT_ABOVE_HZ = 3000.0
TRANSIENT_WEIGHT = 2.0
BANDS = 1000 * 2 ** (np.arange(-8, 13) / 3)  # third octaves, 160 Hz .. 16 kHz
FLOOR_BELOW_MAX_DB = 50.0
PAD_SECONDS = 0.2
GROUPS = [(160, 500), (500, 1000), (1000, 2000), (2000, 4000), (4000, 8000), (8000, 17000)]


def load(path):
    sample_rate, data = wavfile.read(path)
    if data.dtype.kind == "i":
        data = data / float(np.iinfo(data.dtype).max)
    data = data.astype(np.float64)
    if data.ndim == 1:
        data = np.stack([data, data], axis=1)
    if sample_rate != SAMPLE_RATE:
        raise ValueError(f"{path} is {sample_rate} Hz; resample to {SAMPLE_RATE} first")
    return data


def onsets(path, start=0.0, end=None):
    mono = load(path).mean(axis=1)
    step = SAMPLE_RATE // 1000
    for low_cut in (2000, 5000):
        high = sosfiltfilt(butter(4, low_cut, "hp", fs=SAMPLE_RATE, output="sos"), mono)
        level = 10 * np.log10(np.add.reduceat(high ** 2, np.arange(0, len(high), step)) / step + 1e-18)
        rise = np.array([level[i] - np.median(level[max(0, i - 30):i - 3]) if i > 10 else 0 for i in range(len(level))])
        print(f"above {low_cut} Hz: onset s, rise dB, level dBFS")
        last = -1000
        for i in range(1, len(level) - 1):
            in_range = start * 1000 <= i and (end is None or i <= end * 1000)
            if in_range and rise[i] > 12 and rise[i] >= rise[i - 1] and rise[i] >= rise[i + 1] and i - last > 60:
                print(f"  {i / 1000:.3f}  +{rise[i]:.0f}  {level[i]:.0f}")
                last = i


def repeats(path, times):
    """Same sample: every band correlates from the onset on. Different backing: nothing correlates before it."""
    data = load(path)
    high = sosfiltfilt(butter(6, 2000, "hp", fs=SAMPLE_RATE, output="sos"), data.mean(axis=1))
    first = int(times[0] * SAMPLE_RATE)
    window = high[first - 200:first - 200 + int(0.08 * SAMPLE_RATE)]
    bands = [(80, 250), (250, 1000), (1000, 4000), (4000, 16000)]
    filtered = [sosfiltfilt(butter(4, b, "bp", fs=SAMPLE_RATE, output="sos"), data.mean(axis=1)) for b in bands]
    for time in times[1:]:
        guess = int(time * SAMPLE_RATE)
        search = high[guess - 200 - 480:guess - 200 + len(window) + 480]
        score = fftconvolve(search, window[::-1], "valid")
        energy = np.convolve(search ** 2, np.ones(len(window)), "valid")
        correlation = score / np.sqrt(energy * np.sum(window ** 2))
        best = int(np.argmax(correlation))
        onset = guess - 480 + best
        print(f"\n{time:.4f} s realigns to {onset / SAMPLE_RATE:.5f} s, r = {correlation[best]:.2f} above 2 kHz")
        print("  from ms  " + "  ".join(f"{a}-{b}" for a, b in bands))
        for start in np.arange(-0.2, 0.8, 0.05):
            a, b = int(start * SAMPLE_RATE), int((start + 0.05) * SAMPLE_RATE)
            cells = []
            for band in filtered:
                p, q = band[first + a:first + b], band[onset + a:onset + b]
                cells.append(np.sum(p * q) / np.sqrt(np.sum(p * p) * np.sum(q * q)))
            print(f"  {start * 1000:6.0f}   " + "  ".join(f"{c:+.2f}" for c in cells))


def spectra(signal, onset, times, size):
    index = onset + (times * SAMPLE_RATE).astype(int)[:, None] + np.arange(-size // 2, size // 2)[None, :]
    return np.fft.rfft(signal[index] * np.hanning(size), axis=1)


def shared_power(a, b, onset_a, onset_b, grid_times=GRID_TIMES, fine_times=FINE_TIMES):
    """Band power over time of what a and b hold in common, both channels; a synth passes itself as b."""
    frequencies = np.fft.rfftfreq(FFT_SIZE, 1 / SAMPLE_RATE)
    pool = np.array([(frequencies >= c / 2 ** (1 / 6)) & (frequencies < c * 2 ** (1 / 6)) for c in BANDS], float)
    fine_pool = (np.fft.rfftfreq(FINE_SIZE, 1 / SAMPLE_RATE) >= TRANSIENT_ABOVE_HZ).astype(float)
    grid, fine = 0, 0
    for channel in (0, 1):
        x, y = a[:, channel], b[:, channel]
        grid = grid + np.real(spectra(x, onset_a, grid_times, FFT_SIZE) * np.conj(spectra(y, onset_b, grid_times, FFT_SIZE))) @ pool.T
        fine = fine + np.real(spectra(x, onset_a, fine_times, FINE_SIZE) * np.conj(spectra(y, onset_b, fine_times, FINE_SIZE))) @ fine_pool
    kernel = np.ones(3) / 3
    grid = np.apply_along_axis(lambda column: np.convolve(column, kernel, "same"), 0, grid)
    return grid.T / 2, np.convolve(fine, kernel, "same") / 2


def target(path, out, mode, values):
    data = load(path)
    if mode == "pair":
        first, second = (int(v * SAMPLE_RATE) for v in values)
        grid, fine = shared_power(data, data, first, second)
        grid[:, GRID_TIMES < PAIR_FROM_SECONDS] = 0.0
        fine[FINE_TIMES < PAIR_FROM_SECONDS] = 0.0
    else:
        onset = int(values[0] * SAMPLE_RATE)
        grid, fine = shared_power(data, data, onset, onset)
        bed_grid, bed_fine = shared_power(data, data, onset, onset, np.arange(values[1], values[2], HOP / SAMPLE_RATE),
                                          np.arange(values[1], values[2], FINE_HOP / SAMPLE_RATE))
        grid, fine = grid - bed_grid.mean(axis=1, keepdims=True), fine - bed_fine.mean()
    ceiling_below = values[3] if mode == "single" and len(values) > 3 else 0.0
    np.savez(out, grid=np.maximum(grid, 1e-15), fine=np.maximum(fine, 1e-15), ceiling_below=ceiling_below)
    print(f"{out}: {len(BANDS)} bands x {grid.shape[1]} ms")


def load_model(path):
    spec = importlib.util.spec_from_file_location("model", path)
    model = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(model)
    return model


def render_grid(model, values, seed):
    pad = np.zeros((int(PAD_SECONDS * SAMPLE_RATE), 2))
    sound = np.concatenate([pad, model.render(values, seed), pad])
    onset = len(pad) + int(model.ONSET_SECONDS * SAMPLE_RATE)
    return shared_power(sound, sound, onset, onset)


def decibels(power, top):
    return np.maximum(10 * np.log10(np.maximum(power, 1e-15)), top - FLOOR_BELOW_MAX_DB)


def band_difference(grid, goal, top):
    """Synth minus target in dB; a ceiling band only counts the synth rising above the target."""
    difference = decibels(grid, top) - decibels(goal["grid"], top)
    ceiling = BANDS < float(goal["ceiling_below"]) if "ceiling_below" in goal else np.zeros(len(BANDS), bool)
    difference[ceiling] = np.maximum(difference[ceiling], 0.0)
    return difference


def mismatch(grid, fine, goal):
    top, fine_top = 10 * np.log10(goal["grid"].max()), 10 * np.log10(goal["fine"].max())
    return np.concatenate([band_difference(grid, goal, top).ravel(),
                           TRANSIENT_WEIGHT * (decibels(fine, fine_top) - decibels(goal["fine"], fine_top))])


def fit(model_path, target_path, params_path, restarts=6, evaluations=150):
    """Least squares on decibel mismatch from random restarts around the start values; keeps the best."""
    model = load_model(model_path)
    goal = np.load(target_path)
    names = [p[0] for p in model.PARAMS]
    low = np.array([p[2] for p in model.PARAMS], float)
    high = np.array([p[3] for p in model.PARAMS], float)
    try:
        start = json.load(open(params_path))
    except FileNotFoundError:
        start = {}
    initial = np.clip([start.get(n, p[1]) for n, p in zip(names, model.PARAMS)], low + 1e-6, high - 1e-6)

    def residuals(vector):
        return mismatch(*render_grid(model, dict(zip(names, vector)), 1), goal)

    rng = np.random.default_rng(7)
    best = None
    for restart in range(restarts):
        guess = initial if restart == 0 else np.clip(initial + rng.uniform(-0.25, 0.25, len(initial)) * (high - low),
                                                     low + 1e-6, high - 1e-6)
        result = least_squares(residuals, guess, bounds=(low, high), diff_step=0.02,
                                x_scale=(high - low) / 20, max_nfev=evaluations)
        rms = np.sqrt(np.mean(result.fun ** 2))
        print(f"restart {restart}: {rms:.2f} dB", flush=True)
        if best is None or rms < best[0]:
            best = (rms, result.x)
    json.dump(dict(zip(names, map(float, best[1]))), open(params_path, "w"), indent=1)
    print(f"{params_path}: {best[0]:.2f} dB")


def compare(model_path, params_path, target_path, out):
    """The fit is done when synth-to-target sits near synth-to-itself on another seed: the rest is noise."""
    model = load_model(model_path)
    values = json.load(open(params_path))
    goal = np.load(target_path)
    grid, fine = render_grid(model, values, 1)
    other, _ = render_grid(model, values, 2)
    top = 10 * np.log10(goal["grid"].max())
    difference = band_difference(grid, goal, top)
    reseeded = decibels(grid, top) - decibels(other, top)
    print(f"synth to target {np.sqrt(np.mean(difference ** 2)):.2f} dB, synth to itself on another seed "
          f"{np.sqrt(np.mean(reseeded ** 2)):.2f} dB")
    times = GRID_TIMES
    print("mean offset per band, synth minus target (dB)")
    print("  ms        " + " ".join(f"{c:>5.0f}" for c in BANDS))
    for a, b in ((-0.12, 0), (0, 0.01), (0.01, 0.03), (0.03, 0.05), (0.05, 0.1), (0.1, 0.2), (0.2, 0.3)):
        rows = (times >= a) & (times < b)
        print(f"  {a * 1000:4.0f}-{b * 1000:<4.0f} " + " ".join(f"{v:+5.0f}" for v in difference[:, rows].mean(axis=1)))

    figure, axes = plt.subplots(3, 1, figsize=(16, 15))
    for axis, (name, power) in zip(axes, (("target", goal["grid"]), ("synth", grid))):
        axis.pcolormesh(times * 1000, np.arange(len(BANDS)), decibels(power, top) - top, vmin=-FLOOR_BELOW_MAX_DB, vmax=0,
                        cmap="magma", shading="nearest")
        axis.set_yticks(range(0, len(BANDS), 2))
        axis.set_yticklabels([f"{c:.0f}" for c in BANDS[::2]])
        axis.set_title(name)
    for index, (a, b) in enumerate(GROUPS):
        rows = (BANDS >= a) & (BANDS < b)
        for power, style in ((goal["grid"], "-"), (grid, "--")):
            axes[2].plot(times * 1000, 10 * np.log10(power[rows].sum(axis=0)) - top, style, color=f"C{index}",
                         label=f"{a}-{b} Hz" if style == "-" else None)
    axes[2].set_ylim(-FLOOR_BELOW_MAX_DB, 8)
    axes[2].legend(ncol=len(GROUPS))
    axes[2].set_title("band groups: target solid, synth dashed")
    for axis in axes:
        axis.set_xticks(np.arange(-120, 301, 10))
        axis.tick_params(labelsize=7)
        axis.grid(alpha=0.3)
    figure.tight_layout()
    figure.savefig(out, dpi=62)
    print(f"picture {out}")


def main():
    command, arguments = sys.argv[1], sys.argv[2:]
    if command == "onsets":
        onsets(arguments[0], *map(float, arguments[1:]))
    elif command == "repeats":
        repeats(arguments[0], [float(a) for a in arguments[1:]])
    elif command == "target":
        target(arguments[0], arguments[1], arguments[2], [float(a) for a in arguments[3:]])
    elif command == "fit":
        fit(arguments[0], arguments[1], arguments[2], *map(int, arguments[3:]))
    elif command == "compare":
        compare(*arguments)


if __name__ == "__main__":
    main()
