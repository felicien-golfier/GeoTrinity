# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy", "scipy", "matplotlib"]
# ///
"""
Judge a synthesised section against a reference section: event anatomy with the backing subtracted, octave balance,
stacked spectrograms.

uv run AI/Python/Audio/compare_sections.py events mine.wav reference.wav [prominence dB]
uv run AI/Python/Audio/compare_sections.py octaves file.wav@start@end ...
uv run AI/Python/Audio/compare_sections.py picture out.png file.wav@start@end ...
"""
import sys

import numpy as np
from scipy.io import wavfile
from scipy.signal import butter, find_peaks, sosfiltfilt, spectrogram

CENTRES = [63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000]
WINDOWS = [(0.0, 0.01), (0.01, 0.02), (0.02, 0.04), (0.04, 0.08), (0.08, 0.16)]  # seconds after an onset
PRE = (-0.04, -0.003)   # the backing, measured just before each onset


def mono(path, start=None, end=None):
    rate, data = wavfile.read(path)
    signal = data.astype(np.float64) / 32768.0
    if signal.ndim > 1:
        signal = signal.mean(axis=1)
    first = int(float(start) * rate) if start else 0
    last = int(float(end) * rate) if end else len(signal)
    return rate, signal[first:last]


def band(signal, rate, centre, order=3):
    high = min(centre * np.sqrt(2.0), rate / 2.0 - 100.0)
    return sosfiltfilt(butter(order, (centre / np.sqrt(2.0), high), "bp", fs=rate, output="sos"), signal)


def onsets(signal, rate, prominence):
    """Where each event above 2 kHz starts: its peak, walked back to the start of its rise."""
    hop = rate // 2000
    high = sosfiltfilt(butter(4, 2000, "hp", fs=rate, output="sos"), signal)
    envelope = 10 * np.log10(np.convolve(high ** 2, np.ones(hop) / hop, "same")[::hop] + 1e-12)
    peaks, _ = find_peaks(envelope, prominence=prominence, distance=30)
    starts = []
    for peak in peaks:
        start = peak
        while start > 0 and envelope[start - 1] < envelope[start] and envelope[peak] - envelope[start - 1] < 25:
            start -= 1
        starts.append(start * hop)

    return [start for start in starts if -PRE[0] * rate < start < len(signal) - WINDOWS[-1][1] * rate]


def anatomy(path, prominence):
    """Per octave and window after the onsets, the events' own power in dB: the backing's power just before each
    onset is subtracted in linear power, so a louder or different backing does not change it."""
    rate, signal = mono(path)
    starts = onsets(signal, rate, prominence)
    power = np.zeros((len(CENTRES), len(WINDOWS)))
    for row, centre in enumerate(CENTRES):
        squared = band(signal, rate, centre) ** 2
        for column, (begin, finish) in enumerate(WINDOWS):
            excess = [squared[start + int(begin * rate):start + int(finish * rate)].mean()
                      - squared[start + int(PRE[0] * rate):start + int(PRE[1] * rate)].mean() for start in starts]
            power[row, column] = np.mean(np.maximum(excess, 1e-14))
    levels = 10 * np.log10(power)
    return len(starts), levels - levels[:, 0].max()


def events(mine, reference, prominence=20.0):
    (count, ours), (reference_count, theirs) = anatomy(mine, prominence), anatomy(reference, prominence)
    print(f"events: mine {count}, reference {reference_count}")
    print("octave  level at 0-10 ms | change at 10-20, 20-40, 40-80, 80-160 ms     (mine/reference, dB)")
    for centre, row, reference_row in zip(CENTRES, ours, theirs):
        changes = "  ".join(f"{row[k] - row[0]:+4.0f}/{reference_row[k] - reference_row[0]:+4.0f}"
                            for k in range(1, len(WINDOWS)))
        print(f"{centre:>6} {row[0]:6.1f}/{reference_row[0]:6.1f} | {changes}")


def octaves(*sections):
    """Each section's power per octave against its loudest octave."""
    bands = [31] + CENTRES[:-1]
    print("section".ljust(48) + "".join(f"{centre:>6}" for centre in bands))
    for section in sections:
        rate, signal = mono(*section.split("@"))
        spectrum = np.abs(np.fft.rfft(signal * np.hanning(len(signal)))) ** 2
        frequencies = np.fft.rfftfreq(len(signal), 1.0 / rate)
        levels = [10 * np.log10(spectrum[(frequencies >= centre / np.sqrt(2)) & (frequencies < centre * np.sqrt(2))].sum()
                                + 1e-20) for centre in bands]
        print(section.split("/")[-1][:47].ljust(48) + "".join(f"{level - max(levels):+6.0f}" for level in levels))


def picture(out, *sections):
    """Stacked log-frequency spectrograms, each on its own time axis."""
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    figure, axes = plt.subplots(len(sections), 1, figsize=(12, 2.6 * len(sections)), squeeze=False)
    for axis, section in zip(axes[:, 0], sections):
        rate, signal = mono(*section.split("@"))
        frequencies, times, power = spectrogram(signal, rate, nperseg=1024, noverlap=896)
        levels = 10 * np.log10(power + 1e-14)
        axis.pcolormesh(times, frequencies[1:], levels[1:], vmin=levels.max() - 80, vmax=levels.max(), cmap="magma",
                        shading="auto")
        axis.set_yscale("log")
        axis.set_ylim(30, 20000)
        axis.set_title(section.split("/")[-1], fontsize=9)
    plt.tight_layout()
    plt.savefig(out, dpi=70)
    print(out)


if __name__ == "__main__":
    mode, arguments = sys.argv[1], sys.argv[2:]
    if mode == "events":
        events(arguments[0], arguments[1], *[float(value) for value in arguments[2:]])
    elif mode == "octaves":
        octaves(*arguments)
    else:
        picture(*arguments)
