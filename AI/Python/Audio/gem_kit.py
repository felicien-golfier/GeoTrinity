# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy", "scipy"]
# ///
"""
The gem kit: crystal and glass, never metal — a gem thrown out glints, a gem picked up chimes and rings on. The sounds
of the drops a boss kill throws (`AGeoGemDrop`): spawn glints are short, pickup chimes leave a long tail, since the
drop is gone the moment it is touched and the sound plays on alone.

Run outside the editor: uv run AI/Python/Audio/gem_kit.py [ship]
Writes the two sounds (`Spawn`, `Pickup`; the drop varies their pitch at play time) as SFX_Gem_<Name>.wav into DRAFT, or into SourceArt/Audio/Gem with `ship`, plus a kit preview — every
sound in turn — into DRAFT.
"""
import pathlib
import sys

import numpy as np

sys.path.insert(0, str(pathlib.Path(__file__).parent))
import mech_kit as kit  # noqa: E402

PROJECT = kit.PROJECT
DRAFT = PROJECT / "AI" / "Audio" / "Drafts" / "Gem"
SHIPPED = PROJECT / "SourceArt" / "Audio" / "Gem"
PACKAGE = "/Game/Art/SFX/Gem"
SAMPLE_RATE = kit.SAMPLE_RATE

# Glass partials above the fundamental: frequency ratio, dB, seconds to fall 20 dB (higher ones die first).
PARTIALS = ((1.0, 0.0, 0.5), (2.76, -7.0, 0.25), (5.4, -13.0, 0.12))
GLINT_NOTE = 2093.0   # Hz the spawn glint rings at (C7)
GLINT_LENGTH = 0.3
CHIME_NOTES = (1568.0, 2349.0)  # the low note, then the one it climbs to
CHIME_GAP = 0.07          # seconds between the two notes
CHIME_LENGTH = 1.5
SPARKLE = (6000.0, 11000.0, -26.0, 0.05)  # the glitter on the strike: low Hz, high Hz, dB, seconds to fall 20 dB


def ring(length, note, fall_scale, rng):
    """A struck piece of glass: PARTIALS above `note`, each with its own phase, the upper ones dying first. Slower
    `fall_scale` rings longer."""
    time = np.arange(length) / SAMPLE_RATE
    sound = np.zeros(length)
    for ratio, level, fall in PARTIALS:
        phase = rng.uniform(0.0, 2.0 * np.pi)
        sound += kit.decibels(level) * np.sin(2.0 * np.pi * note * ratio * time + phase) \
            * kit.falling(length, fall * fall_scale)

    return sound * kit.rising(length, 0.001)


def glint(note, rng):
    """A gem thrown into the air catching the light: one short, bright strike with a flick of glitter."""
    length = kit.seconds(GLINT_LENGTH)
    sound = ring(length, note * rng.uniform(0.98, 1.02), 0.4, rng)
    low, high, level, fall = SPARKLE
    return sound + kit.decibels(level) * kit.band_noise(length, low, high, rng) * kit.falling(length, fall)


def chime(notes, rng):
    """A gem taken: two notes climbing, the second struck CHIME_GAP after the first, both left ringing."""
    length = kit.seconds(CHIME_LENGTH)
    sound = np.zeros(length)
    low, high, level, fall = SPARKLE
    for index, note in enumerate(notes):
        start = kit.seconds(index * CHIME_GAP)
        strike = ring(length - start, note, 1.0, rng)
        strike += kit.decibels(level) * kit.band_noise(length - start, low, high, rng) * kit.falling(length - start, fall)
        kit.add(sound, strike, start, kit.decibels(0.0 if index else -3.0))

    return sound


def one_shots():
    return {"Spawn": glint(GLINT_NOTE, kit.generator("Gem_Spawn")), "Pickup": chime(CHIME_NOTES, kit.generator("Gem_Pickup"))}


def write_all(folder):
    folder.mkdir(parents=True, exist_ok=True)
    DRAFT.mkdir(parents=True, exist_ok=True)
    paths, preview = {}, [np.zeros(kit.seconds(0.3))]
    for name, sound in one_shots().items():
        sound = kit.normalised(kit.fade_edges(sound))
        paths[name] = kit.save(folder / f"SFX_Gem_{name}.wav", sound)
        preview += [sound, np.zeros(kit.seconds(0.5))]
    paths["Preview"] = kit.save(DRAFT / "Gem_kit_preview.wav", np.concatenate(preview))
    return paths


if __name__ == "__main__":
    for path in write_all(SHIPPED if sys.argv[1:] == ["ship"] else DRAFT).values():
        print(path)
