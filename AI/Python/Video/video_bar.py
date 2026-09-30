# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy"]
# ///
"""
Find when an on-screen bar (a health bar, a timer, a gauge) empties and refills, over a whole video.

uv run AI/Python/Video/video_bar.py <video> <left> <top> <width> <height> [--fps 30] [--threshold 0.01]
The box is in source pixels, inside the bar's fill. Measures the share of strongly red pixels in it per frame,
writes <video>.edit/bar.csv (time, fill) and prints every empty and refill time - a player's deaths and respawns
when the box sits on the recording player's health bar.
"""
import argparse
import pathlib
import subprocess

import numpy


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("video", type=pathlib.Path)
    parser.add_argument("box", type=int, nargs=4)
    parser.add_argument("--fps", type=float, default=30)
    parser.add_argument("--threshold", type=float, default=0.01)
    arguments = parser.parse_args()
    left, top, width, height = arguments.box
    output = arguments.video.parent / f"{arguments.video.stem}.edit" / "bar.csv"
    output.parent.mkdir(parents=True, exist_ok=True)

    process = subprocess.Popen(["ffmpeg", "-v", "error", "-hwaccel", "auto", "-i", str(arguments.video), "-vf",
                                f"crop={width}:{height}:{left}:{top},fps={arguments.fps}", "-f", "rawvideo",
                                "-pix_fmt", "rgb24", "-"], stdout=subprocess.PIPE)
    frame_bytes = width * height * 3
    fills = []
    while len(chunk := process.stdout.read(frame_bytes)) == frame_bytes:
        pixels = numpy.frombuffer(chunk, numpy.uint8).reshape(height, width, 3).astype(int)
        red = (pixels[..., 0] > 150) & (pixels[..., 1] < 90) & (pixels[..., 2] < 90)
        fills.append(red.mean())

    output.write_text("".join(f"{index / arguments.fps:.3f},{fill:.3f}\n" for index, fill in enumerate(fills)))
    full = [fill > arguments.threshold for fill in fills]
    for index in range(1, len(full)):
        if full[index] != full[index - 1]:
            print(f"{'refill' if full[index] else 'empty '} {index / arguments.fps:.3f}")


if __name__ == "__main__":
    main()
