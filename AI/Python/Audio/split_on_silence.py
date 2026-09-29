# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy", "scipy"]
# ///
"""
Cut a reference montage into one WAV per sound, splitting wherever it falls silent.

Run outside the editor: uv run AI/Python/Audio/split_on_silence.py <file.wav> [silence dB below peak] [gap s]
Writes <file>_00.wav, <file>_01.wav, ... beside the input, keeping its channels, and prints each cut's span.
A cut shorter than a few tens of milliseconds is usually a click that belongs to the next sound.
fetch_reference.py imports sound_spans.
"""
import sys

import numpy as np
from scipy.io import wavfile

FRAME_SECONDS = 0.005
SILENCE_BELOW_PEAK_DB = 50.0
OVER_FLOOR_DB = 6.0      # a montage's hiss or bed is its floor: silence is anything this close to it
MINIMUM_GAP_SECONDS = 0.04


def sound_spans(mono, sample_rate, silence_below_peak_db=SILENCE_BELOW_PEAK_DB, minimum_gap_seconds=MINIMUM_GAP_SECONDS):
    """Sample spans of each sound: a sound ends once its level stays `silence_below_peak_db` under the loudest frame,
    or within OVER_FLOOR_DB of the montage's floor, for longer than `minimum_gap_seconds`."""
    hop = int(sample_rate * FRAME_SECONDS)
    frame_rms = np.array([np.sqrt(np.mean(mono[start:start + hop] ** 2)) for start in range(0, len(mono), hop)])
    frame_db = 20.0 * np.log10(np.maximum(frame_rms, 1e-9))
    floor = np.percentile(frame_db[frame_db > -180], 5)
    loud = frame_db > max(frame_db.max() - silence_below_peak_db, floor + OVER_FLOOR_DB)
    gap_frames = int(minimum_gap_seconds / FRAME_SECONDS)

    spans = []
    start = None
    quiet = 0
    for index, is_loud in enumerate(loud):
        if is_loud:
            start = index if start is None else start
            quiet = 0
        elif start is not None:
            quiet += 1
            if quiet > gap_frames:
                spans.append((start, index - quiet + 1))
                start = None
                quiet = 0

    if start is not None:
        spans.append((start, len(loud)))

    return [(max(0, (first - 1) * hop), min(len(mono), (last + 1) * hop)) for first, last in spans]


def main():
    path = sys.argv[1]
    sample_rate, data = wavfile.read(path)
    mono = data.astype(np.float64)
    if mono.ndim > 1:
        mono = mono.mean(axis=1)

    stem = path.rsplit(".", 1)[0]
    for number, (begin, end) in enumerate(sound_spans(mono, sample_rate, *map(float, sys.argv[2:]))):
        out_path = f"{stem}_{number:02d}.wav"
        wavfile.write(out_path, sample_rate, data[begin:end])
        print(f"{out_path}  {begin / sample_rate * 1000:.0f} - {end / sample_rate * 1000:.0f} ms")


if __name__ == "__main__":
    main()
