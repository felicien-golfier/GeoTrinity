"""
Generate a punchy "headshot ding" WAV and import it into Unreal as a SoundWave.

Design goals (CS:GO / Valorant style):
  - Instant hard attack — zero ramp, starts at full amplitude
  - Crack transient: white-noise burst in the first 3 ms, filtered to high-mid (3-6 kHz feel)
  - Metallic body: frequency-modulated sine (FM) to get inharmonic, bell-metal harmonics
  - Pitch drop: carrier frequency slides down ~15% in the first 30 ms (gives the "thwack" character)
  - Soft clipping / saturation at the mix stage so it feels solid, not clean
  - Short tail: fully decayed by ~250 ms

Run via MCP execute_script.
"""
import math
import os
import random
import struct
import tempfile

import unreal

SAMPLE_RATE = 44100
DURATION    = 0.30
CHANNELS    = 1
BIT_DEPTH   = 16

# ---------------------------------------------------------------------------
# helpers
# ---------------------------------------------------------------------------

def _soft_clip(x: float, drive: float = 2.2) -> float:
    """Tanh-based soft saturation. drive > 1 adds harmonic richness."""
    return math.tanh(x * drive) / math.tanh(drive)


def _write_wav(path: str, samples: list) -> None:
    n = len(samples)
    data_size = n * CHANNELS * (BIT_DEPTH // 8)
    with open(path, "wb") as f:
        f.write(b"RIFF")
        f.write(struct.pack("<I", 36 + data_size))
        f.write(b"WAVE")
        f.write(b"fmt ")
        f.write(struct.pack("<I", 16))
        f.write(struct.pack("<H", 1))
        f.write(struct.pack("<H", CHANNELS))
        f.write(struct.pack("<I", SAMPLE_RATE))
        f.write(struct.pack("<I", SAMPLE_RATE * CHANNELS * (BIT_DEPTH // 8)))
        f.write(struct.pack("<H", CHANNELS * (BIT_DEPTH // 8)))
        f.write(struct.pack("<H", BIT_DEPTH))
        f.write(b"data")
        f.write(struct.pack("<I", data_size))
        for s in samples:
            clamped = max(-1.0, min(1.0, s))
            f.write(struct.pack("<h", int(clamped * 32767)))


# ---------------------------------------------------------------------------
# synthesis
# ---------------------------------------------------------------------------

def _generate() -> list:
    n = int(SAMPLE_RATE * DURATION)
    rng = random.Random(42)

    # Crack transient params
    crack_end   = int(SAMPLE_RATE * 0.003)   # 3 ms noise burst
    crack_decay = 0.0008                      # very fast

    # FM bell params
    carrier_start = 1600.0   # Hz — starts high
    carrier_end   = 1360.0   # Hz — slides down (pitch drop over 30 ms)
    pitch_slide_t = 0.030    # seconds
    modulator_hz  = 420.0    # FM modulator frequency
    fm_index      = 4.5      # FM index — higher = more metallic harmonics
    body_decay    = 0.055    # tau for the main body (fast, punchy)
    tail_decay    = 0.110    # tau for a softer tail layer

    # Phase accumulators for FM
    carrier_phase   = 0.0
    modulator_phase = 0.0
    dt = 1.0 / SAMPLE_RATE

    samples = []
    for i in range(n):
        t = i * dt

        # --- Crack transient: shaped noise ---
        if i < crack_end:
            noise = rng.uniform(-1.0, 1.0)
            env   = math.exp(-i / (crack_decay * SAMPLE_RATE))
            crack = noise * env * 0.9
        else:
            crack = 0.0

        # --- FM bell body ---
        # Carrier frequency slides down over pitch_slide_t
        slide_ratio = min(t / pitch_slide_t, 1.0)
        carrier_hz  = carrier_start + (carrier_end - carrier_start) * slide_ratio

        modulator_phase += 2.0 * math.pi * modulator_hz * dt
        fm_deviation     = fm_index * modulator_hz
        carrier_phase   += 2.0 * math.pi * (carrier_hz + fm_deviation * math.sin(modulator_phase)) * dt

        bell_env  = math.exp(-t / body_decay)
        bell_tail = math.exp(-t / tail_decay) * 0.35   # softer harmonic tail
        bell      = (math.sin(carrier_phase) * bell_env + math.sin(carrier_phase * 0.5) * bell_tail)

        # --- Mix & saturate ---
        mixed = crack * 0.55 + bell * 0.75
        samples.append(_soft_clip(mixed, drive=1.8))

    # Normalize to 92 %
    peak = max(abs(s) for s in samples) or 1.0
    return [s / peak * 0.92 for s in samples]


# ---------------------------------------------------------------------------
# import
# ---------------------------------------------------------------------------

DESTINATION_PATH = "/Game/Audio/SFX"
DESTINATION_NAME = "SW_ChargeBeam_SweetSpotHit"


def _import(wav_path: str):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename",         wav_path)
    task.set_editor_property("destination_path", DESTINATION_PATH)
    task.set_editor_property("destination_name", DESTINATION_NAME)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated",        True)
    task.set_editor_property("save",             True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    full = f"{DESTINATION_PATH}/{DESTINATION_NAME}"
    asset = unreal.load_asset(full)
    if asset:
        unreal.log(f"[headshot_ding] Imported: {full}")
    else:
        unreal.log_warning("[headshot_ding] Import may have failed — check Output Log.")


def main():
    samples  = _generate()
    tmp_path = os.path.join(tempfile.gettempdir(), "sw_headshot_ding.wav")
    _write_wav(tmp_path, samples)
    unreal.log(f"[headshot_ding] WAV written to {tmp_path}")
    _import(tmp_path)
    unreal.log("[headshot_ding] Done.")


main()
