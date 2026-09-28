# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy", "scipy"]
# ///
"""
Write the reusable metal one-shots — the cracks of the trailer clack — into SourceArt/Audio/Metal.

Run outside the editor: uv run AI/Python/Audio/metal_one_shots.py
Every file is its own sample at -1 dBFS peak; loudness lives in whatever plays it.
"""
import pathlib
import sys

import numpy as np
from scipy.io import wavfile

sys.path.insert(0, str(pathlib.Path(__file__).parent))
import hex_lock_layers  # noqa: E402

SAMPLE_RATE = 48000
OUT_FOLDER = pathlib.Path(__file__).resolve().parents[3] / "SourceArt" / "Audio" / "Metal"
PEAK = 10.0 ** (-1.0 / 20.0)
EDGE_FADE_SECONDS = 0.002
TRIM_BELOW_PEAK_DB = -60.0


def write(name, sound):
    """Trimmed below -60 dB of its peak at both ends, normalised, edge-faded, 16-bit."""
    sound = sound if sound.ndim == 2 else np.stack([sound, sound], axis=1)
    envelope = np.abs(sound).max(axis=1)
    loud = np.flatnonzero(envelope > envelope.max() * 10 ** (TRIM_BELOW_PEAK_DB / 20))
    sound = sound[loud[0]:loud[-1] + 1] * (PEAK / envelope.max())
    ramp = np.linspace(0.0, 1.0, int(EDGE_FADE_SECONDS * SAMPLE_RATE))[:, None]
    sound[-len(ramp):] *= ramp[::-1]
    path = OUT_FOLDER / f"SFX_Metal_{name}.wav"
    wavfile.write(path, SAMPLE_RATE, np.round(sound * 32767).astype(np.int16))
    print(f"{path}  {len(sound) / SAMPLE_RATE * 1000:.0f} ms")


def main():
    OUT_FOLDER.mkdir(parents=True, exist_ok=True)
    variants = hex_lock_layers.json.loads(hex_lock_layers.PARAMS_PATH.read_text())
    write("Crack", hex_lock_layers.layers(variants["low"], 1)["Crack"])
    write("CrackBright", hex_lock_layers.layers(variants["high"], 1)["Crack"])


if __name__ == "__main__":
    main()
