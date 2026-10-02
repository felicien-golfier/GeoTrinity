# /// script
# requires-python = ">=3.11"
# ///
"""
Turn a video_render.py edit list into a timeline DaVinci Resolve imports with every part still editable: the cuts on
V1, the follow zoom as position keyframes and a fixed scale on each clip, every text as a title above its clip, every
sound as an audio clip under it, colour cards as gaps, and the captions as an SRT for the subtitle track.

uv run AI/Python/Video/edit_to_fcpxml.py <edit.json>
Writes <output name>.fcpxml and, when the edit has captions and has been rendered, <output name>_captions.srt beside the
edit. Import the FCPXML as a timeline, then the SRT into the media pool and onto the timeline.
"""
import json
import math
import pathlib
import sys
from xml.sax.saxutils import escape, quoteattr

from video_render import clip_duration, probe

BASIC_TITLE_UID = ".../Titles.localized/Bumper:Opener.localized/Basic Title.localized/Basic Title.moti"
RESOLVE_TEXT_HEIGHT = 1080  # Resolve sizes imported title text against a 1080-high frame
NAMED_COLOURS = {"white": "1 1 1 1", "black": "0 0 0 1", "yellow": "1 1 0 1", "red": "1 0 0 1"}


def frames(seconds, fps):
    return round(seconds * fps)


def stamp(frame_count, fps):
    return f"{frame_count}/{fps}s"


def colour(value):
    if value.startswith("#"):
        red, green, blue = (int(value[index:index + 2], 16) / 255 for index in (1, 3, 5))
        return f"{red:.4f} {green:.4f} {blue:.4f} 1"

    return NAMED_COLOURS[value]


def follow_keys(clip, source_size, size):
    """(source time, position) keys placing the follow window's centre at the frame centre, and the scale over the
    fitted source that makes the window fill the frame. Resolve reads a clip's x in percent of the frame height but
    its y in percent of the fitted source's height."""
    follow = clip["follow"]
    source_width, source_height = source_size
    window_height = source_height * follow.get("height", 0.8)
    window_width = window_height * size[0] / size[1]
    pixels_per_source_pixel = size[1] / window_height
    fit_scale = min(size[0] / source_width, size[1] / source_height)
    keys = []
    for time, x, y in follow["points"]:
        centre_x = min(max(x * source_width, window_width / 2), source_width - window_width / 2)
        centre_y = min(max(y * source_height, window_height / 2), source_height - window_height / 2)
        offset_x = -(centre_x - source_width / 2) * pixels_per_source_pixel
        offset_y = (centre_y - source_height / 2) * pixels_per_source_pixel
        keys.append((time, offset_x * 100 / size[1], offset_y * 100 / (source_height * fit_scale)))

    return keys, pixels_per_source_pixel / fit_scale


def transform_xml(clip, source_size, size, fps):
    keys, scale = follow_keys(clip, source_size, size)
    keyframes = "".join(f'<keyframe time="{stamp(frames(time, fps), fps)}" value="{x:.4f} {y:.4f}" interp="linear"/>'
                        for time, x, y in keys)
    return (f'<adjust-transform scale="{scale:.4f} {scale:.4f}">'
            f'<param name="position"><keyframeAnimation>{keyframes}</keyframeAnimation></param></adjust-transform>')


def title_xml(item, number, lane, offset, fps, size):
    text_size = item.get("size", 110)
    top = {"top": size[1] * 0.12, "center": (size[1] - text_size) / 2, "bottom": size[1] * 0.80 - text_size}
    rise = size[1] / 2 - (top[item.get("position", "top")] + text_size / 2)
    style = f"ts{number}"
    return (f'<title ref="basic_title" lane="{lane}" offset="{stamp(offset + frames(item["at"], fps), fps)}" '
            f'duration="{stamp(frames(item["for"], fps), fps)}" start="0s" name={quoteattr(item["text"])}>'
            f'<text><text-style ref="{style}">{escape(item["text"])}</text-style></text>'
            f'<text-style-def id="{style}"><text-style font="Arial" fontSize="{text_size * RESOLVE_TEXT_HEIGHT / size[1]:.1f}" '
            f'fontFace="Bold" bold="1" '
            f'fontColor="{colour(item.get("color", "white"))}" strokeColor="0 0 0 1" '
            f'strokeWidth="{max(2, text_size // 12)}" alignment="center"/></text-style-def>'
            f'<adjust-transform position="0 {rise * 100 / size[1]:.4f}"/></title>')


def sound_xml(sound, offset, fps, sound_assets):
    length = sound.get("for", sound_assets[sound["source"]][1])
    decibels = 20 * math.log10(sound.get("volume", 1.0))
    return (f'<asset-clip ref="{sound_assets[sound["source"]][0]}" lane="-1" '
            f'offset="{stamp(offset + frames(sound["at"], fps), fps)}" start="{stamp(frames(sound.get("from", 0), fps), fps)}" '
            f'duration="{stamp(frames(length, fps), fps)}" name={quoteattr(pathlib.Path(sound["source"]).stem)}>'
            f'<adjust-volume amount="{decibels:.2f}dB"/></asset-clip>')


def srt_from_ass(ass_path):
    def srt_stamp(ass_time):
        hours, minutes, seconds = ass_time.split(":")
        whole, hundredths = seconds.split(".")
        return f"{int(hours):02}:{minutes}:{whole},{hundredths}0"

    lines = [line.split(",", 9) for line in ass_path.read_text(encoding="utf-8").splitlines()
             if line.startswith("Dialogue:")]
    return "\n".join(f"{number}\n{srt_stamp(fields[1])} --> {srt_stamp(fields[2])}\n{fields[9]}\n"
                     for number, fields in enumerate(lines, 1))


def main():
    edit_path = pathlib.Path(sys.argv[1]).resolve()
    folder = edit_path.parent
    edit = json.loads(edit_path.read_text(encoding="utf-8"))
    fps = edit.get("fps", 30)
    size = edit["size"]
    name = pathlib.Path(edit["output"]).stem

    sources = {clip["source"] for clip in edit["clips"] if "source" in clip}
    sounds = {sound["source"] for clip in edit["clips"] for sound in clip.get("sounds", [])}
    resources = [f'<format id="sequence_format" frameDuration="1/{fps}s" width="{size[0]}" height="{size[1]}"/>',
                 f'<effect id="basic_title" name="Basic Title" uid="{BASIC_TITLE_UID}"/>']
    video_assets = {}
    for number, source in enumerate(sorted(sources)):
        path = (folder / source).resolve()
        video = next(stream for stream in probe(path) if stream["codec_type"] == "video")
        video_assets[source] = (f"video{number}", (video["width"], video["height"]))
        resources.append(f'<format id="video{number}_format" frameDuration="1/{fps}s" width="{video["width"]}" '
                         f'height="{video["height"]}"/>')
        resources.append(f'<asset id="video{number}" name={quoteattr(path.stem)} start="0s" '
                         f'duration="{stamp(frames(float(video["duration"]), fps), fps)}" hasVideo="1" hasAudio="1" '
                         f'format="video{number}_format" audioSources="1" audioChannels="2" audioRate="48000">'
                         f'<media-rep kind="original-media" src={quoteattr(path.as_uri())}/></asset>')

    sound_assets = {}
    for number, source in enumerate(sorted(sounds)):
        path = (folder / source).resolve()
        length = float(next(stream for stream in probe(path) if stream["codec_type"] == "audio")["duration"])
        sound_assets[source] = (f"sound{number}", length)
        resources.append(f'<asset id="sound{number}" name={quoteattr(path.stem)} start="0s" '
                         f'duration="{stamp(frames(length, fps), fps)}" hasAudio="1" audioSources="1" '
                         f'audioChannels="2" audioRate="48000"><media-rep kind="original-media" '
                         f'src={quoteattr(path.as_uri())}/></asset>')

    spine = []
    record = 0
    title_count = 0
    for clip in edit["clips"]:
        duration = frames(clip_duration(clip), fps)
        start = frames(clip["in"], fps) if "source" in clip else 0
        attached = []
        for lane, item in enumerate(clip.get("text", []), 1):
            attached.append(title_xml(item, title_count, lane, start, fps, size))
            title_count += 1

        attached += [sound_xml(sound, start, fps, sound_assets) for sound in clip.get("sounds", [])]
        if "source" in clip:
            asset, source_size = video_assets[clip["source"]]
            transform = transform_xml(clip, source_size, size, fps) if "follow" in clip else ""
            volume = clip.get("volume", 1.0)
            timing = (f'offset="{stamp(record, fps)}" start="{stamp(start, fps)}" duration="{stamp(duration, fps)}" '
                      f'name={quoteattr(pathlib.Path(clip["source"]).stem)}')
            if volume == 0:  # a muted clip brings its picture only, or Resolve scatters its audio over the SFX track
                picture = (f'<video ref="{asset}" offset="{stamp(start, fps)}" start="{stamp(start, fps)}" '
                           f'duration="{stamp(duration, fps)}"/>')
                spine.append(f'<clip {timing}>{transform}{picture}{"".join(attached)}</clip>')
            else:
                loudness = f'<adjust-volume amount="{20 * math.log10(volume):.2f}dB"/>' if volume != 1 else ""
                spine.append(f'<asset-clip ref="{asset}" {timing}>{transform}{loudness}{"".join(attached)}</asset-clip>')
        else:
            spine.append(f'<gap offset="{stamp(record, fps)}" start="0s" duration="{stamp(duration, fps)}" '
                         f'name="{clip.get("color", "card")}">{"".join(attached)}</gap>')

        record += duration

    document = ('<?xml version="1.0" encoding="UTF-8"?>\n<!DOCTYPE fcpxml>\n<fcpxml version="1.9">'
                f'<resources>{"".join(resources)}</resources><library><event name={quoteattr(name)}>'
                f'<project name={quoteattr(name)}><sequence format="sequence_format" duration="{stamp(record, fps)}" '
                f'tcStart="0s" tcFormat="NDF" audioLayout="stereo" audioRate="48k"><spine>{"".join(spine)}</spine>'
                '</sequence></project></event></library></fcpxml>\n')
    (folder / f"{name}.fcpxml").write_text(document, encoding="utf-8")
    print(folder / f"{name}.fcpxml")

    captions = folder / f"{name}.render" / "captions.ass"
    if "captions" in edit and captions.exists():
        (folder / f"{name}_captions.srt").write_text(srt_from_ass(captions), encoding="utf-8")
        print(folder / f"{name}_captions.srt")


if __name__ == "__main__":
    main()
