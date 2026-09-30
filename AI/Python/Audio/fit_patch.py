# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy", "scipy", "matplotlib", "cma"]
# ///
"""
Fit a patch's free values to a reference sound, and compare the two: a multi-resolution log-mel distance, where in
time and frequency the synth is off, a picture, and an A/B WAV to hear the reference then the synth.

uv run AI/Python/Audio/fit_patch.py fit <patch.json> [renders] [searches]
uv run AI/Python/Audio/fit_patch.py compare <patch.json>
uv run AI/Python/Audio/fit_patch.py playlist      every reproduction's A/B in one file, to hear them all at once
The reference is the patch's "reference", a WAV under AI/Audio/References. fit writes the fitted values back into
the patch, each staying free between its bounds. Both write the synth, the A/B and the picture into
AI/Audio/Reproductions/<patch name>/.
The distance is the mean decibel gap between the two log-mel spectrograms at three window sizes, level-matched and
aligned on their onsets. Under a floor per band nothing counts: FLOOR_DB under the reference's loudest cell, raised
to the bed a video plays under its sounds where the cut holds some of it before the onset. The synth against itself
on another noise seed is the distance it cannot go under.
"""
import copy
import importlib.util
import json
import pathlib
import sys

import cma
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from scipy.signal import stft

HERE = pathlib.Path(__file__).parent
REFERENCES = HERE.parents[1] / "Audio" / "References"
OUT = HERE.parents[1] / "Audio" / "Reproductions"
RESOLUTIONS = [256, 1024, 4096]
MEL_BANDS = 64
HIGHEST_HZ = 16000.0        # a video's codec ceiling
FLOOR_DB = -50.0            # under the reference's loudest cell a difference is masked, or the codec's own noise
BED_MARGIN_DB = 3.0
CONTOUR_DB_PER_OCTAVE = 6.0
CONTOUR_RANGE_DB = 30.0
SHORT_SECONDS = 0.25        # a sound shorter than this is pictured on the finest window
GROUPS = [(30, 250), (250, 1000), (1000, 3000), (3000, 8000), (8000, 16000)]
WINDOWS = [(0.0, 0.02), (0.02, 0.06), (0.06, 0.15), (0.15, 0.4), (0.4, 1.0), (1.0, 10.0)]
AB_GAP_SECONDS = 0.5
PLAYLIST_GAP_SECONDS = 1.5


def load_script(name):
    spec = importlib.util.spec_from_file_location(name, HERE / f"{name}.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


SYNTH = load_script("sfx_patch")
LISTEN = load_script("listen")
DISSECT = load_script("dissect")
SAMPLE_RATE = SYNTH.SAMPLE_RATE


def mel_bank(size):
    """Triangular bands evenly spaced in mels, from where the window resolves two bins up to HIGHEST_HZ."""
    frequencies = np.fft.rfftfreq(size, 1.0 / SAMPLE_RATE)
    lowest = max(30.0, 2 * frequencies[1])
    mels = np.linspace(2595 * np.log10(1 + lowest / 700), 2595 * np.log10(1 + HIGHEST_HZ / 700), MEL_BANDS + 2)
    edges = 700 * (10 ** (mels / 2595) - 1)
    bank = np.array([np.clip(np.minimum((frequencies - low) / (centre - low), (high - frequencies) / (high - centre)), 0, None)
                     for low, centre, high in zip(edges, edges[1:], edges[2:])])
    return edges[1:-1], bank


BANKS = {size: mel_bank(size) for size in RESOLUTIONS}


def unit_rms(sound):
    return 1.0 / max(np.sqrt(np.mean(sound ** 2)), 1e-12)


def spectra(sound):
    """Log-mel power per resolution; a sound shorter than the longest window is padded with silence to it."""
    sound = np.concatenate([sound, np.zeros(max(0, RESOLUTIONS[-1] - len(sound)))])
    result = []
    for size in RESOLUTIONS:
        _, _, spectrum = stft(sound, SAMPLE_RATE, nperseg=size, noverlap=size * 3 // 4, boundary="zeros")
        result.append(10 * np.log10(BANKS[size][1] @ np.abs(spectrum) ** 2 + 1e-12))
    return result


def target_of(patch):
    """The reference cut to its sound at unit RMS, its spectra, each resolution's floor per band, and how far
    under its peak the cut reaches. A reference written "file.wav@start@end" is that span of the file, in seconds:
    one shot out of a run of them."""
    path, *span = patch["reference"].split("@")
    sound = LISTEN.load_mono(REFERENCES / path)[1]
    if span:
        sound = sound[int(float(span[0]) * SAMPLE_RATE):int(float(span[1]) * SAMPLE_RATE)]
    begin, end, depth = DISSECT.sound_span(sound, SAMPLE_RATE)
    gain = unit_rms(sound[begin:end])
    reference = sound[begin:end] * gain
    reference_spectra = spectra(reference)
    lead = sound[:begin] * gain
    floors = []
    for size, spectrum in zip(RESOLUTIONS, reference_spectra):
        floor = np.full(len(spectrum), spectrum.max() + FLOOR_DB)
        if len(lead) >= 2 * size:
            _, _, bed = stft(lead, SAMPLE_RATE, nperseg=size, noverlap=size * 3 // 4, boundary=None)
            floor = np.maximum(floor, 10 * np.log10(BANKS[size][1] @ np.mean(np.abs(bed) ** 2, axis=1) + 1e-12)
                               + BED_MARGIN_DB)
        floors.append(floor[:, None])
    return reference, reference_spectra, floors, depth


def gaps(ours, theirs, floors):
    """Per resolution, synth minus reference in dB, both held up to the floor."""
    return [np.maximum(mine, floor) - np.maximum(reference, floor) for mine, reference, floor in zip(ours, theirs, floors)]


def contour(spectrum, floor, centres):
    """Per frame, the centroid in octaves of the energy standing over the floor, and that energy."""
    above = np.maximum(10 ** (spectrum / 10) - 10 ** (floor / 10), 0.0)
    energy = above.sum(axis=0)
    return np.log2(((centres[:, None] * above).sum(axis=0) + 1e-20 * centres[0]) / (energy + 1e-20)), energy


def contour_gap(ours, theirs, floors):
    """Mean octaves between the synth's and the reference's brightness contours on the finest window, over the
    reference's frames within CONTOUR_RANGE_DB of its loudest: a pitch or a filter sweep in the wrong place still
    pulls the fit toward the right one, where the spectral gap alone gives it no slope."""
    centres = BANKS[RESOLUTIONS[0]][0]
    mine, _ = contour(ours[0], floors[0], centres)
    reference, energy = contour(theirs[0], floors[0], centres)
    loud = energy > energy.max() * 10 ** (-CONTOUR_RANGE_DB / 10)
    return float(np.mean(np.abs(mine - reference)[loud]))


def distance(ours, theirs, floors):
    """The spectral gap in dB plus the contour gap, CONTOUR_DB_PER_OCTAVE for each octave."""
    spectral = np.mean([np.mean(np.abs(gap)) for gap in gaps(ours, theirs, floors)])
    return float(spectral + CONTOUR_DB_PER_OCTAVE * contour_gap(ours, theirs, floors))


def synth_spectra(patch, length, depth, seed=1):
    """The patch cut to its sound as deep under its peak as the reference was, padded or cut to its length, at
    unit RMS."""
    sound = SYNTH.render(patch, seed)
    begin, end, _ = DISSECT.sound_span(sound, SAMPLE_RATE, depth)
    sound = np.concatenate([sound[begin:end], np.zeros(length)])[:length]
    sound = sound * unit_rms(sound)
    return sound, spectra(sound)


def free_values(value, path=()):
    """(path, [value, low, high]) of every free value in the patch."""
    if isinstance(value, dict):
        if "fit" in value:
            return [(path, value["fit"])]
        return [found for key, item in value.items() for found in free_values(item, path + (key,))]
    if isinstance(value, list):
        return [found for index, item in enumerate(value) for found in free_values(item, path + (index,))]
    return []


def with_values(patch, paths, values):
    patch = copy.deepcopy(patch)
    for path, value in zip(paths, values):
        node = patch
        for key in path:
            node = node[key]
        node["fit"][0] = float(value)
    return patch


def scaled(low, high):
    """Octave-like values move on a log scale: a frequency, a time or a rate spanning a factor of 4 or more."""
    return low > 0 and high / low >= 4


def to_unit(value, low, high):
    return (np.log(value / low) / np.log(high / low)) if scaled(low, high) else (value - low) / (high - low)


def from_unit(unit, low, high):
    unit = float(np.clip(unit, 0.0, 1.0))
    return low * (high / low) ** unit if scaled(low, high) else low + unit * (high - low)


def fit(patch_path, evaluations=3000, restarts=1):
    """CMA-ES over the free values, on one noise seed, from the patch's own values; each restart searches again
    round the best found so far on another seed of its own, which can climb out of a local minimum."""
    patch = json.load(open(patch_path))
    reference, target, floors, depth = target_of(patch)
    found = free_values(patch)
    paths = [path for path, _ in found]
    bounds = [(low, high) for _, (_, low, high) in found]

    def loss(units):
        values = [from_unit(unit, low, high) for unit, (low, high) in zip(units, bounds)]
        return distance(synth_spectra(with_values(patch, paths, values), len(reference), depth)[1], target, floors)

    best_units = [to_unit(value, low, high) for _, (value, low, high) in found]
    best_loss = loss(best_units)
    for restart in range(restarts):
        strategy = cma.CMAEvolutionStrategy(best_units, 0.2, {"bounds": [0, 1], "maxfevals": evaluations, "verbose": -9,
                                                              "seed": restart + 1})
        while not strategy.stop():
            candidates = strategy.ask()
            strategy.tell(candidates, [loss(candidate) for candidate in candidates])
            if strategy.countiter % 20 == 0:
                print(f"  {strategy.countevals:5d} renders, best {strategy.result.fbest:.2f} dB", flush=True)

        if strategy.result.fbest < best_loss:
            best_units, best_loss = strategy.result.xbest, strategy.result.fbest
        print(f"search {restart + 1}: {strategy.result.fbest:.2f} dB, best so far {best_loss:.2f} dB", flush=True)

    best = [from_unit(unit, low, high) for unit, (low, high) in zip(best_units, bounds)]
    json.dump(with_values(patch, paths, best), open(patch_path, "w"), indent=1)
    for (path, (_, low, high)), value in zip(found, best):
        pinned = " (at its bound)" if min(abs(value - low), abs(value - high)) < 0.01 * (high - low) else ""
        print(f"  {'.'.join(map(str, path)):40s} {value:10.4g}{pinned}")
    print(f"{patch_path}: {best_loss:.2f} dB after {restarts} searches of {evaluations} renders")
    compare(patch_path)


def compare(patch_path):
    patch = json.load(open(patch_path))
    reference, theirs, floors, depth = target_of(patch)
    synth, ours = synth_spectra(patch, len(reference), depth)
    _, other = synth_spectra(patch, len(reference), depth, 2)
    gap = gaps(ours, theirs, floors)
    score = (f"distance {distance(ours, theirs, floors):.2f} dB, against itself on another seed "
             f"{distance(other, ours, floors):.2f} dB")
    print(score + "; spectral gap per window "
          + ", ".join(f"{size}: {np.mean(np.abs(each)):.2f}" for size, each in zip(RESOLUTIONS, gap))
          + f"; brightness contour {contour_gap(ours, theirs, floors):.2f} octaves off")
    shown = 0 if len(reference) < SHORT_SECONDS * SAMPLE_RATE else 1
    centres = BANKS[RESOLUTIONS[shown]][0]
    frame_times = np.arange(gap[shown].shape[1]) * RESOLUTIONS[shown] / 4 / SAMPLE_RATE
    print("synth minus reference, dB     " + "".join(f"{f'{a}-{b}':>11}" for a, b in GROUPS))
    for begin, end in WINDOWS:
        columns = (frame_times >= begin) & (frame_times < end)
        if columns.any():
            cells = [gap[shown][(centres >= a) & (centres < b)][:, columns].mean() for a, b in GROUPS]
            print(f"  {begin * 1000:5.0f}-{min(end, frame_times[-1]) * 1000:<5.0f} ms           "
                  + "".join(f"{cell:+11.1f}" for cell in cells))

    folder = OUT / pathlib.Path(patch_path).stem
    folder.mkdir(parents=True, exist_ok=True)
    SYNTH.save(folder / "synth.wav", synth)
    SYNTH.save(folder / "reference_then_synth.wav",
               np.concatenate([reference, np.zeros(int(AB_GAP_SECONDS * SAMPLE_RATE)), synth]))
    draw(folder / "compare.png", ours[shown], theirs[shown], floors[shown], centres, frame_times)
    (folder / "distance.txt").write_text(score)
    print(f"heard in {folder}")


def draw(out, ours, theirs, floor, centres, frame_times):
    top = theirs.max()
    figure, axes = plt.subplots(4, 1, figsize=(11, 12), gridspec_kw={"height_ratios": [2, 2, 2, 1.2]}, sharex=True)
    for axis, (title, image) in zip(axes, (("reference", theirs), ("synth", ours))):
        axis.pcolormesh(frame_times, centres, image - top, vmin=FLOOR_DB, vmax=0, cmap="magma", shading="auto")
        axis.set_yscale("log")
        axis.set_title(title, fontsize=9)
    axes[2].pcolormesh(frame_times, centres, np.maximum(ours, floor) - np.maximum(theirs, floor), vmin=-20, vmax=20,
                       cmap="coolwarm", shading="auto")
    axes[2].set_yscale("log")
    axes[2].set_title("synth minus reference over the floor: red louder, blue quieter", fontsize=9)
    for image, style, name in ((theirs, "-", "reference"), (ours, "--", "synth")):
        axes[3].plot(frame_times, 10 * np.log10(np.sum(10 ** (image / 10), axis=0)) - top, style, label=name)
    axes[3].set_ylim(FLOOR_DB + 20, 25)
    axes[3].legend(fontsize=8)
    axes[3].set_xlabel("s")
    figure.tight_layout()
    figure.savefig(out, dpi=70)
    plt.close(figure)


def playlist():
    """Every reproduction's reference-then-synth in one file, PLAYLIST_GAP_SECONDS apart, in the order printed."""
    sounds = []
    for path in sorted(OUT.glob("*/reference_then_synth.wav")):
        sounds += [LISTEN.load_mono(path)[1], np.zeros(int(PLAYLIST_GAP_SECONDS * SAMPLE_RATE))]
        score = path.with_name("distance.txt")
        print(f"{sum(len(sound) for sound in sounds[:-2]) / SAMPLE_RATE:7.1f} s  {path.parent.name:40s} "
              f"{score.read_text() if score.exists() else ''}")
    SYNTH.save(OUT / "All_reference_then_synth.wav", np.concatenate(sounds))
    print(OUT / "All_reference_then_synth.wav")


def main():
    command, arguments = sys.argv[1], sys.argv[2:]
    if command == "fit":
        fit(arguments[0], *map(int, arguments[1:]))
    elif command == "compare":
        compare(arguments[0])
    else:
        playlist()


if __name__ == "__main__":
    main()
