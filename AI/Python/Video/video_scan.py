# /// script
# requires-python = ">=3.11"
# dependencies = ["faster-whisper", "nvidia-cublas-cu12", "nvidia-cudnn-cu12==9.*", "numpy"]
# ///
"""
Scan a video so a model can find its moments without watching it: timestamped contact sheets, scene cuts,
loudness, a transcript with word timings, and one timeline text joining them per window.

uv run AI/Python/Video/video_scan.py <video> [--start s] [--end s] [--every s] [--window s] [--language code]
                                             [--no-transcript] [--out DIR]
Writes into <video folder>/<video name>.edit/scan/ (scan_<start>-<end>/ for a span) unless --out is given:
probe.json, sheets/sheet_NNN.jpg (20 frames each, time burned in), scenes.txt, loudness.txt, transcript.srt,
words.json and timeline.txt. Every time written is in the source's seconds, a span's included.
--every defaults to one frame per 1/400 of the span, at least 2 s; --language defaults to detected.
"""
import argparse
import importlib.util
import json
import os
import pathlib
import re
import statistics
import subprocess

import numpy

FONT = pathlib.Path("C:/Windows/Fonts/arialbd.ttf")  # fontconfig has no config on Windows: drawtext needs a file
WHISPER_MODEL = "large-v3-turbo"
SCENE_THRESHOLD = 0.3
SHEET_TILE = "5x4"
SHEET_FRAME_WIDTH = 384
SHEET_FRAMES_TARGET = 400
TOP_WINDOWS = 15


def filter_path(path):
    """A path as a filter option value: a drive colon splits options unless escaped."""
    return "'" + str(path).replace("\\", "/").replace(":", "\\:") + "'"


def run_ffmpeg(arguments, folder):
    subprocess.run(["ffmpeg", "-hide_banner", "-v", "error", "-y", *arguments], cwd=folder, check=True)


def probe(video, folder):
    result = subprocess.run(["ffprobe", "-v", "error", "-show_format", "-show_streams", "-of", "json", str(video)],
                            capture_output=True, text=True, check=True)
    (folder / "probe.json").write_text(result.stdout)
    return json.loads(result.stdout)


def sheets_and_scenes(video, folder, every, seek):
    (folder / "sheets").mkdir(exist_ok=True)
    graph = (f"[0:v]fps=10,scale={SHEET_FRAME_WIDTH}:-2,split[scene][sheet];"
             "[scene]select='gte(scene,0)',metadata=print:key=lavfi.scene_score:file=scenes_raw.txt[cuts];"
             f"[sheet]fps=1/{every}:round=down,"
             f"drawtext=fontfile={filter_path(FONT)}:text='%{{pts\\:hms}}':x=6:y=6:fontsize=22:fontcolor=white"
             f":box=1:boxcolor=black@0.6,tile={SHEET_TILE}:padding=4:margin=4[tiles]")
    run_ffmpeg(["-hwaccel", "auto", *seek, "-i", str(video), "-filter_complex", graph,
                "-map", "[tiles]", "-q:v", "3", "sheets/sheet_%03d.jpg", "-map", "[cuts]", "-f", "null", "-"], folder)
    raw = (folder / "scenes_raw.txt").read_text()
    cuts = [float(seconds) for seconds, score in re.findall(r"pts_time:([\d.]+)\s+lavfi\.scene_score=([\d.]+)", raw)
            if float(score) > SCENE_THRESHOLD]
    (folder / "scenes.txt").write_text("".join(f"{seconds:.2f}\n" for seconds in cuts))
    (folder / "scenes_raw.txt").unlink()
    return cuts


def loudness(video, folder, seek):
    """Momentary loudness (LUFS, 400 ms) every 100 ms, as (seconds, LUFS) pairs."""
    run_ffmpeg([*seek, "-i", str(video), "-vn", "-af",
                "ebur128=metadata=1,ametadata=mode=print:key=lavfi.r128.M:file=loudness_raw.txt",
                "-f", "null", "-"], folder)
    raw = (folder / "loudness_raw.txt").read_text()
    pairs = [(float(seconds), float(value))
             for seconds, value in re.findall(r"pts_time:([\d.]+)\s+lavfi\.r128\.M=(-?[\d.]+|-inf)", raw)
             if value != "-inf"]
    (folder / "loudness.txt").write_text("".join(f"{seconds:.1f} {value:.1f}\n" for seconds, value in pairs))
    (folder / "loudness_raw.txt").unlink()
    return pairs


def load_whisper():
    """The model on the GPU; CTranslate2 finds cuBLAS and cuDNN only through the DLL search path."""
    for package in ("nvidia.cublas", "nvidia.cudnn"):
        for location in importlib.util.find_spec(package).submodule_search_locations:
            if (pathlib.Path(location) / "bin").exists():
                os.add_dll_directory(str(pathlib.Path(location) / "bin"))
                os.environ["PATH"] = str(pathlib.Path(location) / "bin") + os.pathsep + os.environ["PATH"]

    from faster_whisper import WhisperModel
    return WhisperModel(WHISPER_MODEL, device="cuda", compute_type="float16")


def srt_stamp(seconds):
    return f"{int(seconds // 3600):02d}:{int(seconds % 3600 // 60):02d}:{seconds % 60:06.3f}".replace(".", ",")


def transcribe(video, folder, language, seek, start):
    """[start, end, word] per word, also in words.json; transcript.srt holds the spoken segments."""
    pcm = subprocess.run(["ffmpeg", "-v", "error", *seek, "-i", str(video), "-vn", "-ac", "1", "-ar", "16000",
                          "-f", "f32le", "-"], capture_output=True, check=True).stdout
    segments, _ = load_whisper().transcribe(numpy.frombuffer(pcm, numpy.float32), language=language,
                                            vad_filter=True, word_timestamps=True)
    speech = []
    words = []
    for segment in segments:
        speech.append((start + segment.start, start + segment.end, segment.text.strip()))
        words += [[round(start + word.start, 2), round(start + word.end, 2), word.word.strip()] for word in segment.words]

    (folder / "transcript.srt").write_text(
        "".join(f"{index}\n{srt_stamp(start)} --> {srt_stamp(end)}\n{text}\n\n"
                for index, (start, end, text) in enumerate(speech, 1)), encoding="utf-8")
    (folder / "words.json").write_text(json.dumps(words, ensure_ascii=False), encoding="utf-8")
    return words


def clock(seconds):
    return f"{int(seconds // 3600)}:{int(seconds % 3600 // 60):02d}:{seconds % 60:04.1f}"


def timeline(folder, span, window, cuts, loud, words):
    """One row per window: loudness peak and mean, cuts, words said; the loudest windows ranked above."""
    median = statistics.median(value for _, value in loud) if loud else -70.0
    rows = []
    start, finish = span
    while start < finish:
        end = min(start + window, finish)
        values = [value for seconds, value in loud if start <= seconds < end] or [-70.0]
        said = " ".join(word for begin, _, word in words if start <= begin < end)
        rows.append({"start": start, "peak": max(values), "mean": statistics.fmean(values),
                     "cuts": sum(start <= seconds < end for seconds in cuts), "said": said})
        start = end

    ranked = sorted(rows, key=lambda row: row["peak"], reverse=True)[:TOP_WINDOWS]
    lines = [f"span {clock(span[0])} to {clock(span[1])}  window {window:g} s  median loudness {median:.1f} LUFS  "
             f"{len(cuts)} scene cuts  {len(words)} words",
             "loudest windows: " + ", ".join(f"{clock(row['start'])} ({row['peak'] - median:+.0f} dB)" for row in ranked),
             "", "start      peak  mean  cuts  said"]
    lines += [f"{clock(row['start'])}  {row['peak']:5.1f} {row['mean']:5.1f}  {row['cuts']:4d}  {row['said']}"
              for row in rows]
    (folder / "timeline.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("video", type=pathlib.Path)
    parser.add_argument("--out", type=pathlib.Path)
    parser.add_argument("--every", type=float)
    parser.add_argument("--window", type=float, default=10.0)
    parser.add_argument("--language")
    parser.add_argument("--no-transcript", action="store_true")
    parser.add_argument("--start", type=float)
    parser.add_argument("--end", type=float)
    arguments = parser.parse_args()
    video = arguments.video.resolve()
    spanned = arguments.start is not None or arguments.end is not None
    end_name = f"{arguments.end:g}" if arguments.end is not None else "end"
    scan_name = f"scan_{arguments.start or 0:g}-{end_name}" if spanned else "scan"
    folder = (arguments.out or video.parent / f"{video.stem}.edit" / scan_name).resolve()
    folder.mkdir(parents=True, exist_ok=True)

    info = probe(video, folder)
    span = (arguments.start or 0.0, arguments.end or float(info["format"]["duration"]))
    seek = ["-ss", str(span[0]), "-to", str(span[1]), "-copyts"] if spanned else []
    has_video = any(stream["codec_type"] == "video" for stream in info["streams"])
    has_audio = any(stream["codec_type"] == "audio" for stream in info["streams"])
    every = arguments.every or max(2.0, round((span[1] - span[0]) / SHEET_FRAMES_TARGET))
    cuts = sheets_and_scenes(video, folder, every, seek) if has_video else []
    loud = loudness(video, folder, seek) if has_audio else []
    words = (transcribe(video, folder, arguments.language, seek, span[0])
             if has_audio and not arguments.no_transcript else [])
    timeline(folder, span, arguments.window, cuts, loud, words)
    print(f"{folder}  sheets every {every:g} s")


if __name__ == "__main__":
    main()
