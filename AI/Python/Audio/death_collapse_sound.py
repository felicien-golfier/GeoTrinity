# /// script
# dependencies = ["numpy", "soundfile"]
# ///
"""Cuts the bank's Earthquake_3 into the class badge's death sound, SFX_Death_Collapse.

The bank clip is 190 ms of silence then a swell peaking at 0.79 s, but the badge vanishes 0.3 s in, so the rumble
arrived after the pop. The head is cut so the peak lands on the pop, and the clip's clipped peaks are scaled to -1 dBFS.
Export the bank clip first (unreal.Exporter on the Sound Wave), then:
    uv run AI/Python/Audio/death_collapse_sound.py <Earthquake_3.wav>
"""
import os
import sys

import numpy as np
import soundfile as sf

HEAD_CUT = 0.5      # seconds: puts the 0.79 s peak at ~0.29 s, on the pop
FADE_IN = 0.008     # hides the cut
PEAK_DB = -1.0
OUT = os.path.join(os.path.dirname(__file__), "..", "..", "..", "SourceArt", "Audio", "Death", "SFX_Death_Collapse.wav")

samples, rate = sf.read(sys.argv[1], always_2d=True)
samples = samples[int(HEAD_CUT * rate):]
fade = int(FADE_IN * rate)
samples[:fade] *= np.linspace(0.0, 1.0, fade)[:, None]
samples *= 10 ** (PEAK_DB / 20) / np.abs(samples).max()
sf.write(OUT, samples, rate, subtype="PCM_16")
print("wrote", os.path.abspath(OUT), len(samples) / rate, "s")
