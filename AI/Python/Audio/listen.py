# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy", "scipy", "matplotlib"]
# ///
"""
Listen to a WAV the way an audio model does: print its perceptual numbers and render a picture of it.

Run outside the editor: uv run AI/Python/Audio/listen.py <file.wav> [<out.png>]
The picture holds the waveform with its envelope, a log-frequency spectrogram with the spectral centroid drawn
over it, and the average spectrum. Read the PNG to see the sound; read the printout to measure it.
"""
import sys

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from scipy.io import wavfile
from scipy.signal import stft

ENVELOPE_WINDOW_SECONDS = 0.005
LEVEL_STRIP_SECONDS = 0.01
SILENCE_DB = -60.0
FFT_SIZE = 1024
LOWEST_PLOTTED_HZ = 30.0


def load_mono(path):
    sample_rate, data = wavfile.read(path)
    if data.dtype.kind == "i":
        data = data / float(np.iinfo(data.dtype).max)
    elif data.dtype.kind == "u":
        data = (data - 128) / 128.0
    if data.ndim > 1:
        data = data.mean(axis=1)
    return sample_rate, data.astype(np.float64)


def to_db(value):
    return 20.0 * np.log10(np.maximum(value, 1e-9))


def rms_envelope(samples, sample_rate):
    window = max(1, int(sample_rate * ENVELOPE_WINDOW_SECONDS))
    return np.sqrt(np.convolve(samples**2, np.ones(window) / window, mode="same"))


def mean_of_first_and_last_third(values):
    third = max(1, len(values) // 3)
    return np.mean(values[:third]), np.mean(values[-third:])


def describe(path, samples, sample_rate):
    duration = len(samples) / sample_rate
    peak = np.max(np.abs(samples))
    rms = np.sqrt(np.mean(samples**2))
    envelope = rms_envelope(samples, sample_rate)
    envelope_peak_index = int(np.argmax(envelope))
    envelope_peak = envelope[envelope_peak_index]
    above_10 = np.flatnonzero(envelope >= 0.1 * envelope_peak)
    above_90 = np.flatnonzero(envelope >= 0.9 * envelope_peak)
    attack_ms = (above_90[0] - above_10[0]) / sample_rate * 1000.0
    audible = np.flatnonzero(to_db(envelope) > to_db(envelope_peak) + SILENCE_DB)
    audible_end = audible[-1] / sample_rate
    below_20db = np.flatnonzero(envelope[envelope_peak_index:] < envelope_peak * 0.1)
    decay_20db_ms = below_20db[0] / sample_rate * 1000.0 if below_20db.size else float("nan")

    window = min(FFT_SIZE, len(samples))
    frequencies, times, spectrum = stft(samples, sample_rate, nperseg=window, noverlap=window * 7 // 8)
    magnitude = np.abs(spectrum)
    frame_energy = magnitude.sum(axis=0)
    loud_frames = frame_energy > frame_energy.max() * 0.01
    centroid = (frequencies[:, None] * magnitude).sum(axis=0) / np.maximum(frame_energy, 1e-12)
    power = magnitude[:, loud_frames] ** 2 + 1e-12
    flatness = np.mean(np.exp(np.mean(np.log(power), axis=0)) / np.mean(power, axis=0))
    centroid_start, centroid_end = mean_of_first_and_last_third(centroid[loud_frames])
    plotted = frequencies > LOWEST_PLOTTED_HZ
    strongest_partial = frequencies[plotted][np.argmax(magnitude[plotted][:, loud_frames], axis=0)]
    partial_start, partial_end = mean_of_first_and_last_third(strongest_partial)
    average_spectrum = magnitude[:, loud_frames].mean(axis=1)

    print(f"file              {path}")
    print(f"length            {duration * 1000:.0f} ms at {sample_rate} Hz, audible until {audible_end * 1000:.0f} ms")
    print(f"peak / rms        {to_db(peak):.1f} dBFS / {to_db(rms):.1f} dBFS, crest {to_db(peak) - to_db(rms):.1f} dB")
    print(f"clipped samples   {int(np.sum(np.abs(samples) >= 0.999))}")
    print(f"dc offset         {np.mean(samples):+.4f}")
    print(f"edges             first {samples[0]:+.3f}, last {samples[-1]:+.3f} (non-zero edge = click)")
    print(f"attack 10-90 %    {attack_ms:.1f} ms, envelope peak at {envelope_peak_index / sample_rate * 1000:.0f} ms")
    print(f"decay to -20 dB   {decay_20db_ms:.0f} ms after the peak")
    print(f"centroid          {centroid_start:.0f} Hz start -> {centroid_end:.0f} Hz end (brightness contour)")
    print(f"strongest partial {partial_start:.0f} Hz start -> {partial_end:.0f} Hz end (pitch contour)")
    print(f"flatness          {flatness:.3f} (0 = pure tone, 1 = white noise)")
    slice_length = int(sample_rate * LEVEL_STRIP_SECONDS)
    slice_peaks = [np.max(np.abs(samples[start:start + slice_length])) for start in range(0, len(samples), slice_length)]
    print(f"peak dB per {LEVEL_STRIP_SECONDS * 1000:.0f} ms  {' '.join(f'{to_db(peak):.0f}' for peak in slice_peaks)}")
    return frequencies, times, magnitude, centroid, average_spectrum, envelope


def draw(out_path, samples, sample_rate, frequencies, times, magnitude, centroid, average_spectrum, envelope):
    sample_times = np.arange(len(samples)) / sample_rate * 1000.0
    figure, (wave_axis, spectrogram_axis, spectrum_axis) = plt.subplots(
        3, 1, figsize=(10, 10), gridspec_kw={"height_ratios": [1, 2, 1]})

    wave_axis.plot(sample_times, samples, linewidth=0.4, color="0.5")
    wave_axis.plot(sample_times, envelope * np.sqrt(2), color="tab:red", linewidth=1.2, label="envelope")
    wave_axis.set_xlim(0, sample_times[-1])
    wave_axis.set_ylim(-1, 1)
    wave_axis.set_xlabel("ms")
    wave_axis.legend(loc="upper right")

    level_db = to_db(magnitude / magnitude.max())
    spectrogram_axis.pcolormesh(times * 1000.0, frequencies, level_db, vmin=-80, vmax=0, cmap="magma", shading="auto")
    spectrogram_axis.plot(times * 1000.0, centroid, color="cyan", linewidth=1.0, label="centroid")
    spectrogram_axis.set_yscale("log")
    spectrogram_axis.set_ylim(LOWEST_PLOTTED_HZ, sample_rate / 2)
    spectrogram_axis.set_xlim(0, sample_times[-1])
    spectrogram_axis.set_ylabel("Hz")
    spectrogram_axis.set_xlabel("ms")
    spectrogram_axis.legend(loc="upper right")

    spectrum_axis.semilogx(frequencies, to_db(average_spectrum / average_spectrum.max()), color="tab:blue")
    spectrum_axis.set_xlim(LOWEST_PLOTTED_HZ, sample_rate / 2)
    spectrum_axis.set_ylim(-80, 3)
    spectrum_axis.set_xlabel("Hz")
    spectrum_axis.set_ylabel("dB")
    spectrum_axis.grid(True, which="both", alpha=0.3)

    figure.tight_layout()
    figure.savefig(out_path, dpi=90)
    print(f"picture           {out_path}")


def main():
    wav_path = sys.argv[1]
    out_path = sys.argv[2] if len(sys.argv) > 2 else wav_path.rsplit(".", 1)[0] + "_listen.png"
    sample_rate, samples = load_mono(wav_path)
    analysis = describe(wav_path, samples, sample_rate)
    draw(out_path, samples, sample_rate, *analysis)


if __name__ == "__main__":
    main()
