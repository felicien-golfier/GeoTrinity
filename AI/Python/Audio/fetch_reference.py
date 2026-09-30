# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy", "scipy", "matplotlib", "yt-dlp", "imageio-ffmpeg", "rapidocr-onnxruntime"]
# ///
"""
Turn a sound-pack video into a reference: its audio, one WAV per sound, and contact sheets of the video frame
under each sound so its name on screen can be read.

uv run AI/Python/Audio/fetch_reference.py fetch <video id or url> <Name> [--silence dB] [--gap s]
uv run AI/Python/Audio/fetch_reference.py cut <Name> [--silence dB] [--gap s] [--crop x y w h]
uv run AI/Python/Audio/fetch_reference.py frames <Name> <every s> [--crop x y w h]   a montage with no silences
uv run AI/Python/Audio/fetch_reference.py labels <Name> --crop x y w h   OCR the name shown over each sound
uv run AI/Python/Audio/fetch_reference.py name <Name> <labels.txt>
Writes into AI/Audio/References/<Name>/: <Name>_Ref_Full.wav at 48 kHz, video.<ext>, cuts/NN.wav, cuts.json and
sheet_NN.png. --crop keeps a share of each frame (left, top, width, height, 0 to 1), to read small text. A video
id starting with "-" is given as its full URL.
A labels line "NN Label" renames cut NN to NN_Label.wav; "from to Label" (seconds) cuts that span of the full audio.
"""
import argparse
import importlib.util
import json
import pathlib
import re
import subprocess

import imageio_ffmpeg
import matplotlib

matplotlib.use("Agg")
import matplotlib.image
import matplotlib.pyplot as plt
import numpy as np
import yt_dlp
from scipy.io import wavfile

REFERENCES = pathlib.Path(__file__).resolve().parents[2] / "Audio" / "References"
SAMPLE_RATE = 48000
FFMPEG = imageio_ffmpeg.get_ffmpeg_exe()
FRAME_INTO_SOUND = (0.3, 0.5)  # a sound's frame: this share into it, at most this many seconds
FRAME_SHEET = (4, 4)           # columns, rows
STRIP_SHEET = (2, 12)
LABEL_RATE = 10.0              # strip frames a second
LABEL_CHANGE = 4.0             # grey levels a new name moves the strip by on average, well over compression noise
ONSET_SEARCH_SECONDS = (0.1, 0.4)  # a name shows at most this long after or before its sound's onset
SHORTEST_SPAN_SECONDS = 0.1


def load_split():
    path = pathlib.Path(__file__).with_name("split_on_silence.py")
    spec = importlib.util.spec_from_file_location("split_on_silence", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def download(url, folder, name):
    """The best audio as a 48 kHz 16-bit WAV, and a video sharp enough to read the names on it."""
    folder.mkdir(parents=True, exist_ok=True)
    url = url if "/" in url else f"https://www.youtube.com/watch?v={url}"
    audio_options = {"format": "bestaudio/best", "outtmpl": str(folder / "source.%(ext)s"), "quiet": True}
    with yt_dlp.YoutubeDL(audio_options) as downloader:
        source = pathlib.Path(downloader.prepare_filename(downloader.extract_info(url, download=True)))
    subprocess.run([FFMPEG, "-y", "-loglevel", "error", "-i", str(source), "-ar", str(SAMPLE_RATE), "-ac", "2",
                    "-c:a", "pcm_s16le", str(folder / f"{name}_Ref_Full.wav")], check=True)
    source.unlink()
    for old in folder.glob("video.*"):
        old.unlink()
    video_options = {"format": "bv*[height<=720][height>=360]/wv*[height>=240]/b[height<=720]/w",
                     "outtmpl": str(folder / "video.%(ext)s"), "quiet": True}
    try:
        with yt_dlp.YoutubeDL(video_options) as downloader:
            downloader.download([url])
    except yt_dlp.utils.DownloadError as error:
        print(f"no video, so the sheets stay empty: {error}")


def frame(video, seconds, crop, out):
    subprocess.run([FFMPEG, "-y", "-loglevel", "error", "-ss", f"{seconds:.3f}", "-i", str(video), "-frames:v", "1",
                    str(out)], check=True)
    image = matplotlib.image.imread(out)
    height, width = image.shape[:2]
    left, top, share_wide, share_high = crop
    return image[int(top * height):int((top + share_high) * height), int(left * width):int((left + share_wide) * width)]


def sheet_layout(image):
    """Columns and rows of a sheet: a strip of text is tiled two to a row, wider."""
    height, width = image.shape[:2]
    return STRIP_SHEET if width / height > 5 else FRAME_SHEET


def draw_sheet(path, titled_images):
    columns, rows = sheet_layout(titled_images[0][1])
    height, width = titled_images[0][1].shape[:2]
    figure, axes = plt.subplots(rows, columns, figsize=(16, 16 * rows / columns * height / width + 0.3 * rows), squeeze=False)
    for axis, (title, image) in zip(axes.ravel(), titled_images + [("", None)] * (rows * columns - len(titled_images))):
        axis.axis("off")
        if image is not None:
            axis.imshow(image, cmap="gray")
            axis.set_title(title, fontsize=11)

    figure.tight_layout()
    figure.savefig(path, dpi=80)
    plt.close(figure)


def sheets(folder, moments, crop):
    """Contact sheets of the frame at each (title, seconds)."""
    videos = list(folder.glob("video.*"))
    for old in folder.glob("sheet_*.png"):
        old.unlink()
    scratch = folder / "frame.png"
    if videos and moments:
        columns, rows = sheet_layout(frame(videos[0], moments[0][1], crop, scratch))
        for sheet in range(0, len(moments), columns * rows):
            draw_sheet(folder / f"sheet_{sheet // (columns * rows):02d}.png",
                       [(title, frame(videos[0], seconds, crop, scratch)) for title, seconds in moments[sheet:sheet + columns * rows]])

    scratch.unlink(missing_ok=True)


def load_full(name):
    folder = REFERENCES / name
    sample_rate, data = wavfile.read(folder / f"{name}_Ref_Full.wav")
    return folder, sample_rate, data


def write_cut(path, sample_rate, data):
    wavfile.write(path, sample_rate, data)
    peak = 20 * np.log10(np.abs(data.astype(np.float64) / 32768.0).max() + 1e-9)
    print(f"{path.name:40s} {len(data) / sample_rate * 1000:7.0f} ms  peak {peak:5.1f} dBFS")
    return round(peak, 1)


def cut(name, silence, gap, crop):
    folder, sample_rate, data = load_full(name)
    split = load_split()
    mono = data.astype(np.float64).mean(axis=1) / 32768.0
    spans = split.sound_spans(mono, sample_rate, silence, gap)
    cuts = folder / "cuts"
    cuts.mkdir(exist_ok=True)
    for old in cuts.glob("*.wav"):
        old.unlink()
    listing = []
    for index, (begin, end) in enumerate(spans):
        peak = write_cut(cuts / f"{index:02d}.wav", sample_rate, data[begin:end])
        listing.append({"cut": index, "start": begin / sample_rate, "end": end / sample_rate, "peak_db": peak})

    (folder / "cuts.json").write_text(json.dumps(listing, indent=1))
    sheets(folder, [(f"{entry['cut']:02d}  {entry['start']:.2f}-{entry['end']:.2f} s",
                     entry["start"] + min(FRAME_INTO_SOUND[0] * (entry["end"] - entry["start"]), FRAME_INTO_SOUND[1]))
                    for entry in listing], crop)
    print(f"{len(listing)} cuts, sheets in {folder}")


def frames(name, every, crop):
    folder, sample_rate, data = load_full(name)
    times = np.arange(0.0, len(data) / sample_rate, every)
    sheets(folder, [(f"{seconds:.1f} s", seconds) for seconds in times], crop)
    print(f"{len(times)} frames, sheets in {folder}")


def strip_frames(video, crop):
    """The cropped strip LABEL_RATE times a second, grey, as (frames, height, width)."""
    scratch = video.with_name("frame.png")
    height, width = frame(video, 0.0, (0.0, 0.0, 1.0, 1.0), scratch).shape[:2]
    scratch.unlink()
    left, top, wide, high = crop
    size = (int(wide * width) // 2 * 2, int(high * height) // 2 * 2)
    box = f"crop={size[0]}:{size[1]}:{int(left * width)}:{int(top * height)}"
    raw = subprocess.run([FFMPEG, "-loglevel", "error", "-i", str(video), "-vf", f"fps={LABEL_RATE},{box}", "-f",
                          "rawvideo", "-pix_fmt", "gray", "-"], capture_output=True, check=True).stdout
    return np.frombuffer(raw, np.uint8).reshape(-1, size[1], size[0]).astype(np.int16)


def onset_near(mono, sample_rate, seconds):
    """The sharpest rise in level from ONSET_SEARCH_SECONDS before `seconds` to ONSET_SEARCH_SECONDS after it."""
    hop = int(0.005 * sample_rate)
    before, after = ONSET_SEARCH_SECONDS
    first = max(0, int((seconds - before) * sample_rate))
    window = mono[first:first + int((before + after) * sample_rate)]
    level = 10 * np.log10(np.add.reduceat(window ** 2, np.arange(0, len(window), hop)) / hop + 1e-12)
    rise = level[4:] - np.min([level[shift:len(level) - 4 + shift] for shift in range(4)], axis=0)
    return (first + (int(np.argmax(rise)) + 3) * hop) / sample_rate


def read_text(engine, strip):
    found, _ = engine(np.repeat(np.repeat(strip.astype(np.uint8), 2, axis=0), 2, axis=1))
    return " ".join(text for _, text, _ in found) if found else "?"


def labels(name, crop):
    """The sound under each name the video shows: the strip's changes split the audio, each span starting on the
    sound's own onset; writes labels.txt ("from to Label") and sheets of the strips to check it against."""
    from rapidocr_onnxruntime import RapidOCR

    folder, sample_rate, data = load_full(name)
    video = list(folder.glob("video.*"))[0]
    strips = strip_frames(video, crop)
    change = np.abs(np.diff(strips, axis=0)).mean(axis=(1, 2))
    boundaries = [0] + list(np.flatnonzero(change > LABEL_CHANGE) + 1) + [len(strips)]
    held = []
    for begin, end in zip(boundaries, boundaries[1:]):
        same = held and np.abs(strips[(begin + end) // 2] - strips[sum(held[-1]) // 2]).mean() < LABEL_CHANGE
        if same:
            held[-1] = (held[-1][0], end)
        elif end - begin >= 2:
            held.append((begin, end))

    mono = data.astype(np.float64).mean(axis=1) / 32768.0
    starts = [onset_near(mono, sample_rate, begin / LABEL_RATE) for begin, _ in held]
    ends = starts[1:] + [len(data) / sample_rate]
    engine = RapidOCR()
    texts = [read_text(engine, strips[sum(span) // 2]).replace(" ", "_") for span in held]
    (folder / "labels.txt").write_text("".join(f"{start:.3f} {end:.3f} {text}\n" for start, end, text in zip(starts, ends, texts)))
    for old in folder.glob("sheet_*.png"):
        old.unlink()
    titled = [(f"{start:.2f}-{end:.2f} s  {text}", strips[sum(span) // 2]) for start, end, text, span in zip(starts, ends, texts, held)]
    per_sheet = np.prod(STRIP_SHEET)
    for sheet in range(0, len(titled), per_sheet):
        draw_sheet(folder / f"sheet_{sheet // per_sheet:02d}.png", titled[sheet:sheet + per_sheet])
    print(f"{len(held)} names in {folder / 'labels.txt'}; check them against the sheets, then run name")


def name_cuts(name, labels_path):
    """Spans replace the silence cuts: the folder starts empty when the labels hold any. A span shorter than
    SHORTEST_SPAN_SECONDS is a name shown over another sound, and is skipped."""
    folder, sample_rate, data = load_full(name)
    cuts = folder / "cuts"
    lines = [line.split() for line in pathlib.Path(labels_path).read_text().splitlines()]
    spans = [words for words in lines if len(words) >= 3 and words[1].replace(".", "").isdigit()]
    if spans:
        for old in cuts.glob("*.wav"):
            old.unlink()
    cuts.mkdir(exist_ok=True)
    for index, words in enumerate(spans):
        begin, end = int(float(words[0]) * sample_rate), int(float(words[1]) * sample_rate)
        if end - begin > SHORTEST_SPAN_SECONDS * sample_rate:
            write_cut(cuts / f"{index:03d}_{re.sub(r'[^A-Za-z0-9]+', '_', '_'.join(words[2:]))}.wav", sample_rate,
                      data[begin:end])
    for words in lines:
        if 2 <= len(words) and words not in spans:
            match = list(cuts.glob(f"{int(words[0]):02d}*.wav"))[0]
            match.rename(cuts / f"{int(words[0]):02d}_{re.sub(r'[^A-Za-z0-9]+', '_', '_'.join(words[1:]))}.wav")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("command", choices=["fetch", "cut", "frames", "labels", "name"])
    parser.add_argument("arguments", nargs="+")
    parser.add_argument("--silence", type=float, default=load_split().SILENCE_BELOW_PEAK_DB)
    parser.add_argument("--gap", type=float, default=load_split().MINIMUM_GAP_SECONDS)
    parser.add_argument("--crop", type=float, nargs=4, default=[0.0, 0.0, 1.0, 1.0])
    options = parser.parse_args()
    arguments = options.arguments
    if options.command == "fetch":
        download(arguments[0], REFERENCES / arguments[1], arguments[1])
        cut(arguments[1], options.silence, options.gap, options.crop)
    elif options.command == "cut":
        cut(arguments[0], options.silence, options.gap, options.crop)
    elif options.command == "frames":
        frames(arguments[0], float(arguments[1]), options.crop)
    elif options.command == "labels":
        labels(arguments[0], options.crop)
    else:
        name_cuts(arguments[0], arguments[1])


if __name__ == "__main__":
    main()
