# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy", "matplotlib"]
# ///
"""
Cut a seamless loop out of a music track (mp3 / wav) for the main menu.

Run outside the editor: uv run AI/Python/Audio/menu_music_loop.py [<track>] [<out.wav>]
Needs ffmpeg on PATH. Default track: ~/Downloads/hang inspiration.mp3, default output:
SourceArt/Audio/MenuMusic/SFX_MenuInspiration_Loop.wav (16-bit PCM, peak -1 dBFS, `_Loop` so the import marks it looping).
Steps: decode, trim silence, estimate the beat and bar, pick loop start / end on zero crossings of the same beat phase
and a whole number of bars apart with the best waveform match, then bake an equal-power crossfade of the tail into the
head so the file loops with no fade at its edges. Prints the seam score; writes AI/Output/menu_music_loop_seam.png.
"""
import subprocess
import sys
import wave
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

PROJECT = Path(__file__).resolve().parents[3]
DEFAULT_TRACK = Path.home() / "Downloads" / "hang inspiration.mp3"
DEFAULT_OUT = PROJECT / "SourceArt/Audio/MenuMusic/SFX_MenuInspiration_Loop.wav"
SEAM_PICTURE = PROJECT / "AI/Output/menu_music_loop_seam.png"

SAMPLE_RATE = 44100
ENCODER_DELAY = 1100                # mp3 priming samples at the head
SILENCE_DB = -50.0                  # edge silence threshold under full scale
CROSSFADE_SECONDS = 1.5             # tail into head, baked in
MATCH_SECONDS = 0.25                # window before each cut compared for the waveform match
SEARCH_SECONDS = 3.0                # how far from the file's edges a cut may sit
GRID_SECONDS = 0.01                 # candidate spacing before snapping to a zero crossing
BEATS_PER_BAR = 4
BAR_TOLERANCE_SECONDS = 0.04       # a loop length must sit this close to a whole number of bars
ENVELOPE_HOP = 110                  # 2.5 ms
PEAK_DBFS = -1.0


def decode(path):
    """Stereo float samples (n, 2) at SAMPLE_RATE."""
    raw = subprocess.run(
        ["ffmpeg", "-v", "error", "-i", str(path), "-f", "f32le", "-ac", "2", "-ar", str(SAMPLE_RATE), "-"],
        check=True, capture_output=True).stdout

    return np.frombuffer(raw, dtype="<f4").reshape(-1, 2).astype(np.float64)


def trim_silence(audio):
    mono = np.abs(audio).max(axis=1)
    loud = np.nonzero(mono > 10 ** (SILENCE_DB / 20))[0]
    first = max(loud[0], ENCODER_DELAY)

    return audio[first:loud[-1] + 1]


def onset_envelope(mono):
    frames = len(mono) // ENVELOPE_HOP
    level = np.sqrt((mono[:frames * ENVELOPE_HOP].reshape(frames, ENVELOPE_HOP) ** 2).mean(axis=1))

    return np.maximum(np.diff(level, prepend=level[0]), 0.0)


def bar_period(mono):
    """Bar length in samples: the beat period whose comb of onsets is strongest, times BEATS_PER_BAR."""
    onsets = onset_envelope(mono)
    frame_seconds = ENVELOPE_HOP / SAMPLE_RATE
    centred = onsets - onsets.mean()
    autocorrelation = np.correlate(centred, centred, "full")[len(centred) - 1:]
    lags = np.arange(int(0.33 / frame_seconds), int(1.0 / frame_seconds))
    rough = lags[np.argmax(autocorrelation[lags])]

    best_score, best_period = -1.0, rough
    for period in np.arange(rough * 0.97, rough * 1.03, rough * 0.0005):
        positions = np.arange(0, len(onsets) - 1, period)
        phases = np.arange(0, period, 1.0)
        comb = [onsets[np.round((positions[:-1] + phase)).astype(int).clip(0, len(onsets) - 1)].sum()
                for phase in phases]
        if max(comb) > best_score:
            best_score, best_period = max(comb), period

    return best_period * ENVELOPE_HOP * BEATS_PER_BAR


def next_zero_crossing(mono, position):
    window = np.signbit(mono[position:position + 2000])
    return position + int(np.argmax(window[1:] != window[:-1]))


def windows_before(mono, positions):
    length = int(MATCH_SECONDS * SAMPLE_RATE)
    stacked = np.stack([mono[position - length:position] for position in positions])
    stacked = stacked - stacked.mean(axis=1, keepdims=True)

    return stacked / np.maximum(np.linalg.norm(stacked, axis=1, keepdims=True), 1e-12)


def find_cuts(audio):
    """(start, end, score, whole_bars): the pair with the best waveform match, on whole bars when one exists."""
    mono = audio.mean(axis=1)
    crossfade = int(CROSSFADE_SECONDS * SAMPLE_RATE)
    grid = int(GRID_SECONDS * SAMPLE_RATE)
    search = int(SEARCH_SECONDS * SAMPLE_RATE)
    starts = [next_zero_crossing(mono, p) for p in range(crossfade, crossfade + search, grid)]
    ends = [next_zero_crossing(mono, p) for p in range(len(mono) - search - 2000, len(mono) - 2000, grid)]

    scores = windows_before(mono, starts) @ windows_before(mono, ends).T
    bar = bar_period(mono)
    lengths = np.array(ends)[None, :] - np.array(starts)[:, None]
    off_bar = np.abs(lengths / bar - np.round(lengths / bar)) * bar / SAMPLE_RATE
    on_bar = off_bar < BAR_TOLERANCE_SECONDS

    whole_bars = bool(on_bar.any())
    usable = np.where(on_bar, scores, -2.0) if whole_bars else scores
    start_index, end_index = np.unravel_index(np.argmax(usable), usable.shape)
    print(f"bar {bar / SAMPLE_RATE:.3f} s ({60 * BEATS_PER_BAR * SAMPLE_RATE / bar:.1f} BPM)")

    return starts[start_index], ends[end_index], scores[start_index, end_index], whole_bars


def bake_loop(audio, start, end):
    crossfade = int(CROSSFADE_SECONDS * SAMPLE_RATE)
    loop = audio[start:end].copy()
    ramp = np.linspace(0.0, np.pi / 2, crossfade)[:, None]
    loop[-crossfade:] = audio[end - crossfade:end] * np.cos(ramp) + audio[start - crossfade:start] * np.sin(ramp)

    return loop


def seam_report(loop):
    """Step across the wrap against the signal's own typical step."""
    jump = np.abs(loop[0] - loop[-1]).max()
    typical = np.sqrt((np.diff(loop, axis=0) ** 2).mean())

    return jump, typical


def draw_seam(loop):
    two_passes = np.concatenate([loop, loop])
    seam = len(loop)
    around = int(0.05 * SAMPLE_RATE)
    seconds = (np.arange(seam - around, seam + around) - seam) / SAMPLE_RATE
    figure, axes = plt.subplots(2, 1, figsize=(12, 6))
    axes[0].plot(np.arange(len(two_passes)) / SAMPLE_RATE, two_passes.mean(axis=1), linewidth=0.3)
    axes[0].axvline(seam / SAMPLE_RATE, color="red")
    axes[0].set_title("Two passes")
    axes[1].plot(seconds * 1000, two_passes[seam - around:seam + around, 0])
    axes[1].axvline(0, color="red")
    axes[1].set_title("Seam, +-50 ms (ms)")
    SEAM_PICTURE.parent.mkdir(parents=True, exist_ok=True)
    figure.tight_layout()
    figure.savefig(SEAM_PICTURE, dpi=110)


def write_wav(path, audio):
    audio = audio * 10 ** (PEAK_DBFS / 20) / np.abs(audio).max()
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "wb") as handle:
        handle.setnchannels(2)
        handle.setsampwidth(2)
        handle.setframerate(SAMPLE_RATE)
        handle.writeframes((audio * 32767).round().astype("<i2").tobytes())


def main():
    track = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_TRACK
    out = Path(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_OUT
    audio = trim_silence(decode(track))
    start, end, score, whole_bars = find_cuts(audio)
    loop = bake_loop(audio, start, end)
    jump, typical = seam_report(loop)
    write_wav(out, loop)
    draw_seam(loop)

    print(f"loop {start / SAMPLE_RATE:.3f} - {end / SAMPLE_RATE:.3f} s of {len(audio) / SAMPLE_RATE:.1f} s, "
          f"{len(loop) / SAMPLE_RATE:.3f} s long, {'whole bars' if whole_bars else 'NO whole-bar pair: best match only'}")
    print(f"match {score:.3f}, seam step {jump:.5f} vs typical step {typical:.5f} "
          f"({'ok' if jump <= typical * 2 else 'AUDIBLE CLICK RISK'})")
    print(out)
    print(SEAM_PICTURE)


if __name__ == "__main__":
    main()
