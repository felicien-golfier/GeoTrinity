# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy", "scipy", "matplotlib"]
# ///
"""
Take a sound apart the way a sound designer hears it, cut off any bed around it: its events, the one tone holding
it when there is one, its tonal partials with their pitch and level contours grouped into harmonic families, its
noise per octave with each band's envelope, its modulation and echo.

Run outside the editor:
  uv run AI/Python/Audio/dissect.py <file.wav> [<out.png>]     the whole anatomy, and a picture of it
  uv run AI/Python/Audio/dissect.py survey <file or folder> ...  one line per sound, each cut to itself
The picture draws the partial tracks over a log-frequency spectrogram — on a 5 ms window for a sound shorter than
SHORT_SOUND_SECONDS, fine enough for a chirp — the envelope with its events and the noise bands' envelopes. The
printout is what a patch for sfx_patch.py is written from. fit_patch.py imports sound_span.
"""
import importlib.util
import pathlib
import sys

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from scipy.io import wavfile
from scipy.ndimage import maximum_filter1d, median_filter, uniform_filter1d
from scipy.signal import find_peaks

WINDOW_SECONDS = 0.021
HOP_SECONDS = 0.0025
ZERO_PAD = 4
PEAK_PROMINENCE_DB = 10.0      # a partial stands this far over the spectrum around it
TRACK_FLOOR_DB = -55.0         # partials quieter than this under the loudest one are ignored
TRACK_JUMP = 0.05              # a partial moves at most this share of its frequency from where its glide predicts
MINIMUM_TRACK_SECONDS = 0.015
TRACK_GAP_FRAMES = 6
TRACKS_SHOWN = 12
HARMONIC_TOLERANCE = 0.02
HIGHEST_HARMONIC = 12          # above it, a whole-number ratio is as likely chance
OCTAVES = [63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000]
EVENT_SHARE = 0.35             # an onset at least this share of the sharpest one's jump
TREMOLO_WINDOW_SECONDS = 0.01
TREMOLO_FASTEST_HZ = 40.0
LINE_WINDOW_SECONDS = 0.0053
LINE_HOP_SECONDS = 0.001
LINE_SHARE = 0.5               # a frame is one tone when its peak bin and neighbours hold this share of its energy
LINE_SHORTEST_SECONDS = 0.005
LINE_LOWEST_HZ = 500.0
SHORT_SOUND_SECONDS = 0.3      # a shorter sound is pictured on the dominant lines' window, fine enough for a chirp         # under it the short window's bins are too wide to tell a tone from low noise
ENVELOPE_SECONDS = 0.002
SILENCE_DB = -60.0
SPAN_WINDOW_SECONDS = 0.005
ONSET_WINDOW_SECONDS = 0.0005
ONSET_BELOW_PEAK_DB = -20.0
END_BELOW_PEAK_DB = -60.0
OVER_BED_DB = 6.0
BED_UNDER_PEAK_DB = 20.0
QUIET_GAP_SECONDS = 0.1        # a sound has ended once it stays down this long; the bed may rise over it after


def load_listen():
    spec = importlib.util.spec_from_file_location("listen", pathlib.Path(__file__).with_name("listen.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def decibels(power):
    return 10.0 * np.log10(np.maximum(power, 1e-20))


def bed_level(level, first, window):
    """The bed a video plays under its sounds: the level before the sound, or else the cut's last tenth when that
    sits BED_UNDER_PEAK_DB under the peak — anything louder there is the sound itself."""
    if first > 2 * window:
        return np.median(level[:first])
    end = np.median(level[-max(window, len(level) // 10):])
    return end if end < level.max() - BED_UNDER_PEAK_DB else -np.inf


def sound_span(sound, rate, depth_db=None):
    """The sound itself, off the bed a video plays under it, and how far under its peak that span reaches: from
    where the level first comes within ONSET_BELOW_PEAK_DB of its peak, walked back through any swell while it
    stays OVER_BED_DB over the bed or END_BELOW_PEAK_DB under the peak, to where it last stands that high, plus a
    little tail. The walk runs on a smooth level, which a low tone does not dip through; the start is then set on a
    fine one, which a smooth level would place early, unlike a synth's that starts on its first sample. A render
    has no bed, and the quiet start of its swell would pass for one: it is given its reference's depth_db instead,
    cut where that reference's bed stops hiding the swell."""
    smooth_window, fine_window = int(SPAN_WINDOW_SECONDS * rate), int(ONSET_WINDOW_SECONDS * rate)
    level = decibels(np.convolve(sound ** 2, np.ones(smooth_window) / smooth_window, "same"))
    fine = decibels(np.convolve(sound ** 2, np.ones(fine_window) / fine_window, "same"))
    peak = level.max()
    first = int(np.flatnonzero(level > peak + ONSET_BELOW_PEAK_DB)[0])
    if depth_db is None:
        depth_db = peak - max(peak + END_BELOW_PEAK_DB, bed_level(level, first, smooth_window) + OVER_BED_DB)

    floor = peak - depth_db
    quiet_before = np.flatnonzero(level[:first] <= floor)
    start = quiet_before[-1] + 1 if quiet_before.size else 0
    start += int(np.argmax(fine[start:first + smooth_window] > floor))
    above = np.flatnonzero(level > floor)
    run_ends = list(above[np.flatnonzero(np.diff(above) > QUIET_GAP_SECONDS * rate)]) + [above[-1]]
    last = next(end for end in run_ends if end >= np.argmax(level))
    return start, min(len(sound), last + int(0.02 * rate)), depth_db


def power_spectrogram(samples, rate, window_seconds=WINDOW_SECONDS, hop_seconds=HOP_SECONDS):
    size = int(window_seconds * rate)
    hop = int(hop_seconds * rate)
    padded = np.concatenate([np.zeros(size // 2), samples, np.zeros(size)])
    starts = np.arange(0, len(samples), hop)
    frames = padded[starts[:, None] + np.arange(size)[None, :]] * np.hanning(size)
    power = np.abs(np.fft.rfft(frames, size * ZERO_PAD, axis=1)) ** 2
    return np.fft.rfftfreq(size * ZERO_PAD, 1.0 / rate), starts / rate, power


def envelope_decibels(samples, rate):
    window = max(1, int(ENVELOPE_SECONDS * rate))
    return decibels(uniform_filter1d(samples ** 2, window)[::window]), window / rate


def frame_peaks(level_db, frequencies, top_db):
    """Local maxima standing over the spectrum around them, their frequency and level refined on a parabola."""
    floor = median_filter(level_db, size=31, mode="nearest")
    peaks, _ = find_peaks(level_db, distance=3)
    peaks = peaks[(level_db[peaks] - floor[peaks] > PEAK_PROMINENCE_DB) & (level_db[peaks] > top_db + TRACK_FLOOR_DB)]
    left, centre, right = level_db[peaks - 1], level_db[peaks], level_db[np.minimum(peaks + 1, len(level_db) - 1)]
    offset = 0.5 * (left - right) / np.where(left - 2 * centre + right == 0, -1e-9, left - 2 * centre + right)
    step = frequencies[1]
    return frequencies[peaks] + offset * step, centre - 0.25 * (left - right) * offset


def track_partials(frequencies, power):
    """Links each frame's peaks, loudest first, to the partial whose glide predicts them best; a partial survives
    TRACK_GAP_FRAMES frames without a peak, as a beating or trembling line does. Returns [(frame, Hz, dB)] per track,
    loudest first."""
    level_db = decibels(power)
    top_db = level_db.max()
    finished, active = [], []
    for frame, spectrum in enumerate(level_db):
        peak_hz, peak_db = frame_peaks(spectrum, frequencies, top_db)
        claimed = set()
        for hz, db in sorted(zip(peak_hz, peak_db), key=lambda peak: -peak[1]):
            best, best_distance = None, np.log(1 + TRACK_JUMP)
            for index, track in enumerate(active):
                points = track["points"]
                last = points[-1][1]
                predicted = last * (last / points[-2][1]) ** (track["missed"] + 1) if len(points) > 1 else last
                distance = abs(np.log(hz / predicted))
                if index not in claimed and distance < best_distance:
                    best, best_distance = index, distance

            if best is None:
                active.append({"points": [], "missed": 0})
                best = len(active) - 1
            active[best]["points"].append((frame, hz, db))
            active[best]["missed"] = 0
            claimed.add(best)

        survivors = []
        for index, track in enumerate(active):
            track["missed"] += index not in claimed
            if track["missed"] > TRACK_GAP_FRAMES:
                finished.append(track["points"])
            else:
                survivors.append(track)
        active = survivors

    minimum = MINIMUM_TRACK_SECONDS / HOP_SECONDS
    tracks = [np.array(points) for points in finished + [track["points"] for track in active] if len(points) >= minimum]
    return sorted(tracks, key=lambda track: -np.sum(10 ** (track[:, 2] / 10)))


def dominant_lines(samples, rate):
    """Where one tone holds most of the sound, its pitch every LINE_HOP_SECONDS on a LINE_WINDOW window: a zap or a
    laser sweeps faster than the partial tracker's window can follow. Returns (times, Hz, dB under the loudest frame)
    per unbroken run longer than LINE_SHORTEST_SECONDS."""
    size, hop = int(LINE_WINDOW_SECONDS * rate), int(LINE_HOP_SECONDS * rate)
    padded = np.concatenate([np.zeros(size // 2), samples, np.zeros(size)])
    starts = np.arange(0, len(samples), hop)
    frames = padded[starts[:, None] + np.arange(size)[None, :]] * np.hanning(size)
    power = np.abs(np.fft.rfft(frames, axis=1)) ** 2
    fine = np.abs(np.fft.rfft(frames, size * 8, axis=1)) ** 2
    peak = np.argmax(power, axis=1)
    share = np.array([row[max(0, bin - 1):bin + 2].sum() for row, bin in zip(power, peak)]) / np.maximum(power.sum(axis=1), 1e-20)
    level = decibels(power.sum(axis=1))
    held = (share > LINE_SHARE) & (level > level.max() - 30) & (peak * rate / size >= LINE_LOWEST_HZ)
    hz = np.argmax(fine, axis=1) * rate / (size * 8)
    runs, begin = [], None
    for index, on in enumerate(np.append(held, False)):
        if on and begin is None:
            begin = index
        elif not on and begin is not None:
            if (index - begin) * hop >= LINE_SHORTEST_SECONDS * rate:
                runs.append((starts[begin:index] / rate, hz[begin:index], level[begin:index] - level.max()))
            begin = None
    return runs


def describe_lines(samples, rate):
    runs = dominant_lines(samples, rate)
    print(f"dominant lines    {len(runs)} (one tone holding the sound, pitched every {LINE_HOP_SECONDS * 1000:.0f} ms):")
    for times, hz, level in runs:
        marks = [hz[int(round(share * (len(hz) - 1)))] for share in (0, 0.25, 0.5, 0.75, 1)]
        print(f"  {times[0] * 1000:5.0f}-{times[-1] * 1000:<5.0f} ms  {' > '.join(f'{mark:.0f}' for mark in marks)} Hz  "
              f"lowest {hz.min():.0f} at {times[np.argmin(hz)] * 1000:.0f} ms, highest {hz.max():.0f} at "
              f"{times[np.argmax(hz)] * 1000:.0f} ms, level {level.max():+.0f} dB")
    return runs


def glide_shape(times, hz):
    """Whether the pitch moves on a straight line in octaves (exponential) or in hertz (linear)."""
    if hz.max() / hz.min() < 1.03:
        return "steady"
    exponential = np.polyfit(times, np.log(hz), 1, full=True)[1]
    linear = np.polyfit(times, hz / hz.mean(), 1, full=True)[1]
    exponential = exponential[0] if len(exponential) else 0.0
    linear = linear[0] if len(linear) else 0.0
    return "exp" if exponential <= linear else "linear"


def describe_tracks(tracks, times, top_db):
    """The loudest partials: span, pitch at 0, 25, 50, 75 and 100 % of it, level and fall."""
    print(f"partials          {len(tracks)} tracked, the {min(len(tracks), TRACKS_SHOWN)} loudest (dB under the loudest partial):")
    for rank, track in enumerate(tracks[:TRACKS_SHOWN]):
        track_times = times[track[:, 0].astype(int)]
        hz, db = track[:, 1], track[:, 2]
        marks = [hz[int(round(share * (len(hz) - 1)))] for share in (0, 0.25, 0.5, 0.75, 1)]
        peak = int(np.argmax(db))
        tail = db[peak:]
        fall = (tail[0] - tail[-1]) / max(track_times[-1] - track_times[peak], HOP_SECONDS) if len(tail) > 2 else 0.0
        print(f"  {rank:2d}  {track_times[0]:.3f}-{track_times[-1]:.3f} s  "
              f"{' > '.join(f'{mark:.0f}' for mark in marks)} Hz  ({hz[-1] / hz[0]:.2f}x, {glide_shape(track_times, hz)})  "
              f"peak {db[peak] - top_db:+.0f} dB at {track_times[peak]:.3f} s, falls {fall:.0f} dB/s")


def harmonic_families(tracks):
    """Partials whose frequency stays a whole multiple of a lower one's, with how fast their levels fall: a saw or
    square falls 6 dB per octave of harmonic number, a triangle 12, a filtered or clipped wave faster.
    Returns (root Hz, harmonic numbers, levels in dB under the root, fall per octave, every or odd) per family."""
    families = {}
    for index, track in enumerate(tracks):
        for root_index, root in enumerate(tracks):
            shared, root_rows, rows = np.intersect1d(root[:, 0], track[:, 0], return_indices=True)
            if root_index != index and len(shared) >= 0.5 * min(len(root), len(track)):
                ratio = np.median(track[rows, 1] / root[root_rows, 1])
                harmonic = round(ratio)
                if 2 <= harmonic <= HIGHEST_HARMONIC and abs(ratio - harmonic) < HARMONIC_TOLERANCE * harmonic:
                    families.setdefault(root_index, {})[harmonic] = (np.median(track[rows, 2] - root[root_rows, 2]), index)

    members = {member for harmonics in families.values() for _, member in harmonics.values()}
    found = []
    for root in [root for root in families if root not in members and len(families[root]) >= 2]:
        harmonics = families[root]
        numbers = np.array([1] + sorted(harmonics))
        levels = np.array([0.0] + [harmonics[number][0] for number in sorted(harmonics)])
        fall = -np.polyfit(np.log2(numbers), levels, 1)[0]
        kind = "every harmonic" if any(number % 2 == 0 for number in harmonics) else "odd harmonics only"
        found.append((np.median(tracks[root][:, 1]), numbers[1:], levels[1:], fall, kind))
    return found


def describe_families(tracks):
    families = harmonic_families(tracks)
    for root_hz, numbers, levels, fall, kind in families:
        listed = " ".join(f"h{number} {level:+.0f}" for number, level in zip(numbers, levels))
        print(f"  family on {root_hz:.0f} Hz: {listed} dB, {kind}, falling {fall:.0f} dB per octave")
    if not families:
        print("  no harmonic family: the partials are unrelated — inharmonic, metallic, or separate voices")


def envelope_shape(level_db, step):
    """Attack from -20 dB to within 3 dB of the peak — a trembling sound reaches -1 dB late — peak time, how long it
    holds within 3 dB, falls to -20 and -40 dB, audible end; seconds."""
    peak_index = int(np.argmax(level_db))
    peak = level_db[peak_index]
    after = level_db[peak_index:]

    def time_below(drop):
        below = np.flatnonzero(maximum_filter1d(after, 5) < peak - drop)
        return below[0] * step if below.size else float("nan")

    held = np.flatnonzero(after > peak - 3)
    return {"attack": (np.flatnonzero(level_db >= peak - 3)[0] - np.flatnonzero(level_db >= peak - 20)[0]) * step,
            "peak": peak_index * step, "hold": (held[-1] if held.size else 0) * step, "fall20": time_below(20),
            "fall40": time_below(40), "audible": np.flatnonzero(level_db > peak + SILENCE_DB)[-1] * step}


def describe_envelope(samples, rate):
    level_db, step = envelope_decibels(samples, rate)
    shape = envelope_shape(level_db, step)
    print(f"envelope          attack {shape['attack'] * 1000:.1f} ms to within 3 dB of the peak at {shape['peak'] * 1000:.0f} ms, "
          f"within 3 dB for {shape['hold'] * 1000:.0f} ms, -20 dB {shape['fall20'] * 1000:.0f} ms and -40 dB "
          f"{shape['fall40'] * 1000:.0f} ms after it, audible to {shape['audible'] * 1000:.0f} ms")
    return level_db, step


def find_events(power, times):
    """Onsets: jumps of the log spectrum summed over frequency, picked above the flux around them; (s, share)."""
    flux = np.maximum(np.diff(decibels(power), axis=0), 0.0).mean(axis=1)
    flux = np.concatenate([[0.0], flux])
    threshold = median_filter(flux, size=41, mode="nearest") + 0.5 * flux.std()
    peaks, _ = find_peaks(flux, height=threshold, distance=int(0.025 / HOP_SECONDS))
    strongest = flux[peaks].max() if peaks.size else 1.0
    return flux, [(times[peak], flux[peak] / strongest) for peak in peaks if flux[peak] > EVENT_SHARE * strongest]


def describe_events(power, times):
    flux, events = find_events(power, times)
    listed = ", ".join(f"{time * 1000:.0f} ms ({share:.1f})" for time, share in events)
    gaps = np.diff([time for time, _ in events]) * 1000
    print(f"events            {len(events)}: {listed}" + (f"; gaps {' '.join(f'{gap:.0f}' for gap in gaps)} ms" if len(gaps) else ""))
    return flux, [time for time, _ in events]


def residual_power(frequencies, power, tracks):
    """The spectrogram with every tracked partial masked out: the noise."""
    residual = power.copy()
    width = 3 * ZERO_PAD
    for track in tracks:
        for frame, hz, _ in track:
            centre = int(round(hz / frequencies[1]))
            residual[int(frame), max(0, centre - width):centre + width + 1] = 0.0
    return residual


def octave_bands(frequencies, spectrum):
    return np.array([spectrum[:, (frequencies >= centre / np.sqrt(2)) & (frequencies < centre * np.sqrt(2))].sum(axis=1)
                     for centre in OCTAVES])


def describe_noise(frequencies, power, tracks, times):
    """What is left once the partials are masked out, per octave: level, attack, peak time and fall."""
    residual = residual_power(frequencies, power, tracks)
    band_db = decibels(uniform_filter1d(octave_bands(frequencies, residual), 3, axis=1))
    top = decibels(uniform_filter1d(octave_bands(frequencies, power), 3, axis=1)).max()
    print(f"tonal share       {(1.0 - residual.sum() / power.sum()) * 100:.0f} % of the energy is in partials, the rest is noise")
    print("noise per octave  level at its peak, when, 10-90 % rise, fall to -20 dB (dB under the sound's loudest band)")
    for centre, row in zip(OCTAVES, band_db):
        peak = int(np.argmax(row))
        if row[peak] < top - 50:
            continue
        rise_from = np.flatnonzero(row[:peak + 1] < row[peak] - 20)
        start = rise_from[-1] + 1 if rise_from.size else 0
        fall = np.flatnonzero(row[peak:] < row[peak] - 20)
        falls = f"{fall[0] * HOP_SECONDS * 1000:5.0f} ms" if fall.size else " held"
        print(f"  {centre:>6} Hz  {row[peak] - top:+4.0f} dB at {times[peak] * 1000:5.0f} ms, rise {(peak - start) * HOP_SECONDS * 1000:4.0f} ms,"
              f" fall {falls}")

    return band_db - top


def describe_modulation(level_db, step, tracks):
    """Tremolo from the envelope's wobble over its smooth shape, read on TREMOLO_WINDOW_SECONDS so a low tone's own
    cycles do not alias into it; vibrato from the loudest long partial's."""
    level_db = uniform_filter1d(level_db, max(1, int(TREMOLO_WINDOW_SECONDS / step)))
    loud = level_db > level_db.max() - 30
    smooth = uniform_filter1d(level_db, int(0.06 / step))
    wobble = (level_db - smooth)[loud]
    if len(wobble) > int(0.15 / step):
        spectrum = np.abs(np.fft.rfft(wobble - wobble.mean()))
        rates = np.fft.rfftfreq(len(wobble), step)
        band = (rates >= 4) & (rates <= TREMOLO_FASTEST_HZ)
        if band.any() and wobble.std() > 1.0:
            print(f"tremolo           {rates[band][np.argmax(spectrum[band])]:.1f} Hz, {wobble.std() * 2.8:.1f} dB peak to peak")

    for track in sorted(tracks, key=len, reverse=True)[:1]:
        if len(track) * HOP_SECONDS > 0.15:
            cents = 1200 * np.log2(track[:, 1])
            residual = cents - uniform_filter1d(cents, int(0.08 / HOP_SECONDS))
            spectrum = np.abs(np.fft.rfft(residual))
            rates = np.fft.rfftfreq(len(residual), HOP_SECONDS)
            band = (rates >= 3) & (rates <= 60)
            if band.any() and residual.std() > 5:
                print(f"vibrato           {rates[band][np.argmax(spectrum[band])]:.1f} Hz, {residual.std() * 2.8:.0f} cents peak to peak "
                      f"on the {np.median(track[:, 1]):.0f} Hz partial")


def describe_echo(flux):
    """A repeating delay shows as a peak in the onset flux's autocorrelation."""
    centred = flux - flux.mean()
    correlation = np.correlate(centred, centred, "full")[len(centred) - 1:]
    correlation /= max(correlation[0], 1e-12)
    lags = np.arange(len(correlation)) * HOP_SECONDS
    window = (lags > 0.03) & (lags < 0.6)
    if window.any() and correlation[window].max() > 0.35:
        print(f"echo              repeats every {lags[window][np.argmax(correlation[window])] * 1000:.0f} ms "
              f"(r {correlation[window].max():.2f})")


def describe_stereo(path):
    _, data = wavfile.read(path)
    if data.ndim > 1 and data.shape[1] > 1:
        left, right = data[:, 0].astype(float), data[:, 1].astype(float)
        correlation = np.sum(left * right) / np.sqrt(np.sum(left ** 2) * np.sum(right ** 2) + 1e-20)
        side = decibels(np.mean((left - right) ** 2)) - decibels(np.mean((left + right) ** 2))
        print(f"stereo            correlation {correlation:.2f}, side {side:+.0f} dB under mid")


def draw(out, rate, picture, tracks, level_db, step, events, noise_times, noise_db):
    """The envelope with its events, the spectrogram `picture` (frequencies, times, power) with the partials over it
    as (seconds, Hz), and the noise bands."""
    figure, (envelope_axis, spectrum_axis, noise_axis) = plt.subplots(
        3, 1, figsize=(11, 11), gridspec_kw={"height_ratios": [1, 2.2, 1]}, sharex=True)
    envelope_axis.plot(np.arange(len(level_db)) * step, level_db - level_db.max(), color="tab:red", linewidth=0.9)
    for time in events:
        envelope_axis.axvline(time, color="0.4", linestyle=":", linewidth=0.8)
    envelope_axis.set_ylim(-70, 3)
    envelope_axis.set_ylabel("dB")
    frequencies, times, power = picture
    level = decibels(power.T)
    shown = frequencies > 30
    spectrum_axis.pcolormesh(times, frequencies[shown], level[shown] - level.max(), vmin=-80, vmax=0, cmap="magma",
                             shading="auto")
    for index, (track_times, hz) in enumerate(tracks):
        spectrum_axis.plot(track_times, hz, linewidth=1.2, color=f"C{index % 10}")
        spectrum_axis.text(track_times[0], hz[0], str(index), color="white", fontsize=7)
    spectrum_axis.set_yscale("log")
    spectrum_axis.set_ylim(30, rate / 2)
    spectrum_axis.set_ylabel("Hz")
    for index, (centre, row) in enumerate(zip(OCTAVES, noise_db)):
        noise_axis.plot(noise_times, row, color=plt.cm.viridis(index / len(OCTAVES)), linewidth=0.9, label=f"{centre}")
    noise_axis.set_ylim(-60, 3)
    noise_axis.set_ylabel("noise dB")
    noise_axis.set_xlabel("s")
    noise_axis.legend(ncol=len(OCTAVES), fontsize=7, loc="upper right")
    figure.tight_layout()
    figure.savefig(out, dpi=80)
    plt.close(figure)
    print(f"picture           {out}")


def survey_line(path, listen):
    """One sound in a line: its length, envelope, events, loudest partial's glide, harmonic family, tonal share and
    brightness contour — enough to sort a pack and choose what to take apart."""
    rate, samples = listen.load_mono(str(path))
    begin, end, _ = sound_span(samples, rate)
    samples = samples[begin:end]
    level_db, step = envelope_decibels(samples, rate)
    shape = envelope_shape(level_db, step)
    frequencies, times, power = power_spectrogram(samples, rate)
    _, events = find_events(power, times)
    tracks = track_partials(frequencies, power)
    tonal = 1.0 - residual_power(frequencies, power, tracks).sum() / power.sum()
    loud = power.sum(axis=1) > power.sum(axis=1).max() * 0.01
    centroid = (power[loud] @ frequencies) / power[loud].sum(axis=1)
    third = max(1, len(centroid) // 3)
    pitch = "-"
    lines = [run for run in dominant_lines(samples, rate) if run[2].max() > -10]
    if lines:
        line_times, hz, _ = max(lines, key=lambda run: len(run[0]))
        pitch = f"line {hz[0]:.0f}>{hz[-1]:.0f} Hz {(line_times[-1] - line_times[0]) * 1000:.0f}ms"
    elif tracks:
        hz = tracks[0][:, 1]
        pitch = f"{hz[0]:.0f}>{hz[-1]:.0f} Hz {glide_shape(times[tracks[0][:, 0].astype(int)], hz)}"
    families = harmonic_families(tracks[:TRACKS_SHOWN])
    family = f"{families[0][0]:.0f} Hz {'odd' if families[0][4].startswith('odd') else 'all'} -{families[0][3]:.0f}/oct" if families else "-"
    gaps = np.diff([time for time, _ in events])
    rhythm = f"{len(events)} ev" + (f" /{np.median(gaps) * 1000:.0f} ms" if len(gaps) else "")
    print(f"{path.stem[:44]:44s} {len(samples) / rate * 1000:5.0f} {shape['attack'] * 1000:5.1f} {shape['fall20'] * 1000:5.0f} "
          f"{rhythm:>12s}  {pitch:>26s}  {family:>18s} {tonal * 100:4.0f}% "
          f"{np.mean(centroid[:third]):6.0f}>{np.mean(centroid[-third:]):<6.0f}")


def survey(targets):
    listen = load_listen()
    paths = [path for target in map(pathlib.Path, targets)
             for path in (sorted(target.glob("*.wav")) if target.is_dir() else [target])]
    print(f"{'sound':44s} {'ms':>5s} {'att':>5s} {'-20dB':>5s} {'events':>12s}  {'dominant line or partial':>26s}  "
          f"{'harmonic family':>18s} tone  centroid Hz")
    for path in paths:
        survey_line(path, listen)


def anatomy(path, out):
    """The sound cut to itself, off any bed around it."""
    rate, samples = load_listen().load_mono(path)
    begin, end, _ = sound_span(samples, rate)
    print(f"file              {path}: the sound is {begin / rate * 1000:.0f}-{end / rate * 1000:.0f} ms of {len(samples) / rate * 1000:.0f}")
    samples = samples[begin:end]
    level_db, step = describe_envelope(samples, rate)
    frequencies, times, power = power_spectrogram(samples, rate)
    flux, events = describe_events(power, times)
    describe_lines(samples, rate)
    tracks = track_partials(frequencies, power)
    describe_tracks(tracks, times, decibels(power).max())
    describe_families(tracks[:TRACKS_SHOWN])
    noise_db = describe_noise(frequencies, power, tracks, times)
    describe_modulation(level_db, step, tracks)
    describe_echo(flux)
    describe_stereo(path)
    picture = (power_spectrogram(samples, rate, LINE_WINDOW_SECONDS, LINE_HOP_SECONDS)
               if len(samples) < SHORT_SOUND_SECONDS * rate else (frequencies, times, power))
    drawn = [(times[track[:, 0].astype(int)], track[:, 1]) for track in tracks[:TRACKS_SHOWN]]
    draw(out, rate, picture, drawn, level_db, step, events, times, noise_db)


def main():
    if sys.argv[1] == "survey":
        survey(sys.argv[2:])
    else:
        anatomy(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else sys.argv[1].rsplit(".", 1)[0] + "_dissect.png")


if __name__ == "__main__":
    main()
