# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy", "scipy"]
# ///
"""
Cut a reference montage into one WAV per sound, splitting wherever it falls silent.

Run outside the editor: uv run AI/Python/Audio/split_on_silence.py <file.wav>
Writes <file>_00.wav, <file>_01.wav, ... beside the input, keeping its channels, and prints each cut's span.
A cut shorter than a few tens of milliseconds is usually a click that belongs to the next sound.
"""
import sys

import numpy as np
from scipy.io import wavfile

FRAME_SECONDS = 0.005
SILENCE_BELOW_PEAK_DB = 50.0
MINIMUM_GAP_SECONDS = 0.04


def main():
    path = sys.argv[1]
    sample_rate, data = wavfile.read(path)
    mono = data.astype(np.float64)
    if mono.ndim > 1:
        mono = mono.mean(axis=1)

    hop = int(sample_rate * FRAME_SECONDS)
    frame_rms = np.array([np.sqrt(np.mean(mono[start:start + hop] ** 2)) for start in range(0, len(mono), hop)])
    frame_db = 20.0 * np.log10(np.maximum(frame_rms, 1e-9))
    loud = frame_db > frame_db.max() - SILENCE_BELOW_PEAK_DB
    gap_frames = int(MINIMUM_GAP_SECONDS / FRAME_SECONDS)

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

    stem = path.rsplit(".", 1)[0]
    for number, (first, last) in enumerate(spans):
        begin = max(0, (first - 1) * hop)
        end = min(len(mono), (last + 1) * hop)
        out_path = f"{stem}_{number:02d}.wav"
        wavfile.write(out_path, sample_rate, data[begin:end])
        print(f"{out_path}  {begin / sample_rate * 1000:.0f} - {end / sample_rate * 1000:.0f} ms")


if __name__ == "__main__":
    main()
