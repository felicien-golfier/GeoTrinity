# /// script
# requires-python = ">=3.11"
# dependencies = ["opencv-python-headless", "numpy"]
# ///
"""
Find where a reframed output frame (a zoom window of a source, scaled to fill) sits inside its source frame: prints
the match score and the window's centre as fractions of the source — to check a crop or follow zoom against the one
intended, in Resolve or in a render.

uv run AI/Python/Video/locate_frame.py <output frame.png> <source frame.png> [--height 0.8] [--skip-top 0.1]
--height is the window's height as a part of the source's; --skip-top leaves out the top part of the output frame,
where titles sit. A score under about 0.9 is no match: export the source frame at the exact matching time.
"""
import argparse

import cv2


def locate(frame_path, source_path, window_height, skip_top):
    frame = cv2.imread(frame_path, 0)
    source = cv2.imread(source_path, 0)
    scale = source.shape[0] * window_height / frame.shape[0]
    small = cv2.resize(frame, None, fx=scale, fy=scale, interpolation=cv2.INTER_AREA)
    top = int(small.shape[0] * skip_top)
    patch = small[top:]
    scores = cv2.matchTemplate(source, patch, cv2.TM_CCOEFF_NORMED)
    _, score, _, (left, patch_top) = cv2.minMaxLoc(scores)
    centre_x = left + small.shape[1] / 2
    centre_y = patch_top - top + small.shape[0] / 2
    return score, centre_x / source.shape[1], centre_y / source.shape[0]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("frame")
    parser.add_argument("source")
    parser.add_argument("--height", type=float, default=0.8)
    parser.add_argument("--skip-top", type=float, default=0.1)
    arguments = parser.parse_args()
    score, x, y = locate(arguments.frame, arguments.source, arguments.height, arguments.skip_top)
    print(f"score {score:.3f}  centre x {x:.4f} y {y:.4f}")


if __name__ == "__main__":
    main()
