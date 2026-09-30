# /// script
# requires-python = ">=3.11"
# ///
"""
Render an edit list (JSON) into one video: cuts from any sources, still and colour cards, framing to any size,
speed, text, transitions, word captions, a ducked music bed and loudness normalisation, in one ffmpeg pass.

uv run AI/Python/Video/video_render.py <edit.json> [--draft]
Paths in the edit are relative to its own folder. Writes the output it names, and <output name>.render/ beside it
holding the filter graph, the captions and the text files the graph reads. --draft encodes fast and rough.

{
  "output": "short.mp4",
  "size": [1080, 1920], "fps": 30,            # default: the first video clip's size and 30
  "fit": "fill",                              # fill (crop to cover), fit (letterbox), blur (fit over blurred fill)
  "transition": {"type": "fade", "duration": 0.3},   # any xfade type between every clip; omit for hard cuts
  "clips": [
    {"source": "raw.mp4", "in": 62.0, "out": 70.5, "speed": 1.0, "volume": 1.0, "fit": "blur", "focus": 0.5,
     "text": [{"text": "NO WAY", "at": 0.2, "for": 2.0, "position": "top", "size": 110, "color": "yellow"}]},
    {"image": "title.png", "duration": 2.0},
    {"color": "black", "duration": 1.5, "text": [{"text": "Part 2", "at": 0, "for": 1.5}]}
  ],
  "captions": {"source": "raw.mp4", "words": "raw.edit/scan/words.json", "words_per_line": 3, "size": 80,
               "position": "bottom"},
  "music": {"source": "bed.mp3", "volume": 0.25, "duck": true},
  "loudness": -14
}
Clip times are seconds in the source; text times are seconds into the clip as it plays. focus is the horizontal
centre kept by fill, 0 left to 1 right. "cut": true on a clip joins it with a hard cut despite a transition; "crop": [left, top, width, height] (0 to 1)
keeps part of the source before framing; "captions": false keeps a clip uncaptioned. Captions show the words of the named source inside every clip cut from it.
"follow": {"height": 0.8, "points": [[source time, x, y], ...]} crops a window of the output's shape, that tall a part
of the source, its centre moving in straight lines through the points (0 to 1 of the source frame) - a zoom that
follows the action. "sounds": [{"source": "hit.wav", "at": 1.2, "from": 0, "for": 0.8, "volume": 1}] plays sounds
from "at" seconds into the clip, over whatever follows it; "from" skips into the sound, "for" caps its length.
"""
import argparse
import json
import pathlib
import subprocess

FONT = pathlib.Path("C:/Windows/Fonts/arialbd.ttf")  # fontconfig has no config on Windows: drawtext needs a file
FONT_NAME = "Arial"
SAMPLE_RATE = 48000
TEXT_Y = {"top": "h*0.12", "center": "(h-text_h)/2", "bottom": "h*0.80-text_h"}
CAPTION_ALIGNMENT = {"top": 8, "center": 5, "bottom": 2}  # ASS numpad alignment
CAPTION_GAP = 0.6  # a pause this long between words starts a new caption line
BLUR = "boxblur=40:5"
EDGE_FADE = 0.01


def filter_path(path):
    """A path as a filter option value: a drive colon splits options unless escaped."""
    return "'" + str(path).replace("\\", "/").replace(":", "\\:") + "'"


def probe(path):
    result = subprocess.run(["ffprobe", "-v", "error", "-show_streams", "-of", "json", str(path)],
                            capture_output=True, text=True, check=True)
    return json.loads(result.stdout)["streams"]


def nvenc_available():
    test = subprocess.run(["ffmpeg", "-v", "error", "-f", "lavfi", "-i", "color=s=256x256:d=0.1",
                           "-c:v", "h264_nvenc", "-f", "null", "-"], capture_output=True)
    return test.returncode == 0


def clip_duration(clip):
    return clip["duration"] if "duration" in clip else (clip["out"] - clip["in"]) / clip.get("speed", 1.0)


def clip_inputs(clip, folder, size, fps):
    """ffmpeg input arguments for the clip's picture, and whether it brings its own audio."""
    if "source" in clip:
        source = folder / clip["source"]
        has_audio = any(stream["codec_type"] == "audio" for stream in probe(source))
        return ["-ss", str(clip["in"]), "-to", str(clip["out"]), "-i", str(source)], has_audio

    if "image" in clip:
        return ["-loop", "1", "-t", str(clip["duration"]), "-framerate", str(fps), "-i", str(folder / clip["image"])], False

    colour = f"color=c={clip['color']}:s={size[0]}x{size[1]}:r={fps}:d={clip['duration']}"
    return ["-f", "lavfi", "-i", colour], False


def atempo_chain(speed):
    """atempo takes 0.5 to 2 per stage."""
    stages = []
    while speed > 2.0 or speed < 0.5:
        stages.append("atempo=2.0" if speed > 2.0 else "atempo=0.5")
        speed = speed / 2.0 if speed > 2.0 else speed / 0.5

    return stages + [f"atempo={speed}"]


def framing(label, fit, focus, size):
    width, height = size
    cover = f"scale={width}:{height}:force_original_aspect_ratio=increase"
    inside = f"scale={width}:{height}:force_original_aspect_ratio=decrease"
    if fit == "fill":
        return f"{cover},crop={width}:{height}:(iw-{width})*{focus}:(ih-{height})/2"

    if fit == "fit":
        return f"{inside},pad={width}:{height}:(ow-iw)/2:(oh-ih)/2:black"

    return (f"split[{label}bg][{label}fg];[{label}bg]{cover},crop={width}:{height},{BLUR}[{label}b];"
            f"[{label}fg]{inside}[{label}f];[{label}b][{label}f]overlay=(W-w)/2:(H-h)/2")


def text_filters(clip, index, render_folder):
    filters = []
    for number, item in enumerate(clip.get("text", [])):
        text_file = f"text_{index:03d}_{number}.txt"
        (render_folder / text_file).write_text(item["text"], encoding="utf-8")
        size = item.get("size", 90)
        filters.append(f"drawtext=fontfile={filter_path(FONT)}:textfile={text_file}:fontsize={size}"
                       f":fontcolor={item.get('color', 'white')}:borderw={max(2, size // 12)}:bordercolor=black"
                       f":x=(w-text_w)/2:y={TEXT_Y[item.get('position', 'top')]}"
                       f":enable='between(t,{item['at']},{item['at'] + item['for']})'")

    return filters


def path_expression(keys):
    """ffmpeg expression of t moving in straight lines through (time, value) keys, held before and after them."""
    expression = str(keys[-1][1])
    for (start, begin), (end, finish) in reversed(list(zip(keys, keys[1:]))):
        expression = f"if(lt(t,{end}),{begin}+({finish - begin})*(t-{start})/{end - start},{expression})"

    return f"if(lt(t,{keys[0][0]}),{keys[0][1]},{expression})"


def crop_filter(clip, size):
    if "follow" not in clip:
        left, top, width, height = clip.get("crop", [0, 0, 1, 1])
        return f"crop=iw*{width}:ih*{height}:iw*{left}:ih*{top}"

    follow = clip["follow"]
    speed = clip.get("speed", 1.0)
    times = [(time - clip["in"]) / speed for time, _, _ in follow["points"]]
    centre_x = path_expression([(time, x) for time, (_, x, _) in zip(times, follow["points"])])
    centre_y = path_expression([(time, y) for time, (_, _, y) in zip(times, follow["points"])])
    height = f"ih*{follow.get('height', 0.8)}"
    width = f"{height}*{size[0] / size[1]}"
    return (f"crop=w={width}:h={height}:x='clip(iw*({centre_x})-ow/2,0,iw-ow)'"
            f":y='clip(ih*({centre_y})-oh/2,0,ih-oh)'")


def clip_graph(clip, index, input_index, has_audio, edit, size, render_folder):
    """Filter chains leaving [vN] and [aN], both exactly the clip's duration."""
    duration = clip_duration(clip)
    speed = clip.get("speed", 1.0)
    picture = [f"setpts=(PTS-STARTPTS)/{speed}", crop_filter(clip, size),
               framing(f"c{index}", clip.get("fit", edit.get("fit", "fill")), clip.get("focus", 0.5), size),
               f"fps={edit.get('fps', 30)}", "setsar=1", "format=yuv420p", *text_filters(clip, index, render_folder),
               f"trim=duration={duration}", "setpts=PTS-STARTPTS", "settb=AVTB"]  # concat joins output AVTB; xfade needs equal timebases
    sound_source = f"[{input_index}:a]" if has_audio else f"anullsrc=r={SAMPLE_RATE}:cl=stereo,"
    sound = ["asetpts=PTS-STARTPTS", *atempo_chain(speed), f"volume={clip.get('volume', 1.0)}",
             f"aresample={SAMPLE_RATE}", "aformat=channel_layouts=stereo", "apad", f"atrim=duration={duration}",
             "asetpts=PTS-STARTPTS", f"afade=t=in:d={EDGE_FADE}",  # a cut mid-waveform clicks
             f"afade=t=out:st={duration - EDGE_FADE}:d={EDGE_FADE}"]
    return [f"[{input_index}:v]{','.join(picture)}[v{index}]", f"{sound_source}{','.join(sound)}[a{index}]"]


def overlap_into(clip, transition):
    """How long the clip overlaps the one before it: the transition's length, or none for a clip marked cut."""
    return transition["duration"] if transition and not clip.get("cut") else 0.0


def clip_starts(clips, transition):
    """Each clip's start in the output, and the output's length."""
    starts = [0.0]
    for previous, clip in zip(clips, clips[1:]):
        starts.append(starts[-1] + clip_duration(previous) - overlap_into(clip, transition))

    return starts, starts[-1] + clip_duration(clips[-1])


def join_graph(clips, transition):
    count = len(clips)
    if not transition:
        pairs = "".join(f"[v{index}][a{index}]" for index in range(count))
        return [f"{pairs}concat=n={count}:v=1:a=1[vjoin][ajoin]"]

    starts, _ = clip_starts(clips, transition)
    lines = []
    video, audio = "[v0]", "[a0]"
    for index in range(1, count):
        video_out, audio_out = ("[vjoin]", "[ajoin]") if index == count - 1 else (f"[vx{index}]", f"[ax{index}]")
        if clips[index].get("cut"):
            lines.append(f"{video}{audio}[v{index}][a{index}]concat=n=2:v=1:a=1{video_out}{audio_out}")
        else:
            lines.append(f"{video}[v{index}]xfade=transition={transition['type']}:duration={transition['duration']}"
                         f":offset={starts[index]}{video_out}")
            lines.append(f"{audio}[a{index}]acrossfade=d={transition['duration']}{audio_out}")

        video, audio = video_out, audio_out

    return lines


def ass_stamp(seconds):
    return f"{int(seconds // 3600)}:{int(seconds % 3600 // 60):02d}:{seconds % 60:05.2f}"


def fragment(word):
    """Whisper splits elisions and hyphenated endings ("t", "'ai"; "est", "-ce") off their word."""
    return word[0] in "'-"


def spoken(line):
    return "".join(word if fragment(word) else " " + word for _, _, word in line).strip()


def write_captions(edit, folder, render_folder, size):
    """captions.ass on the output timeline, its canvas the output size so sizes are pixels: the source's words
    inside each clip cut from it, in short lines."""
    captions = edit["captions"]
    words = json.loads((folder / captions["words"]).read_text(encoding="utf-8"))
    starts, _ = clip_starts(edit["clips"], edit.get("transition"))
    lines = []
    for clip, start in zip(edit["clips"], starts):
        if clip.get("source") == captions["source"] and clip.get("captions", True):
            speed = clip.get("speed", 1.0)
            inside = [[start + (begin - clip["in"]) / speed, start + (end - clip["in"]) / speed, word]
                      for begin, end, word in words if begin >= clip["in"] and end <= clip["out"]]
            for word in inside:
                whole_words = sum(not fragment(text) for _, _, text in lines[-1]) if lines else 0
                if not lines or (not fragment(word[2]) and (whole_words >= captions.get("words_per_line", 3)
                                                            or word[0] - lines[-1][-1][1] > CAPTION_GAP)):
                    lines.append([])

                lines[-1].append(word)

    font_size = captions.get("size", 80)
    header = (f"[Script Info]\nScriptType: v4.00+\nPlayResX: {size[0]}\nPlayResY: {size[1]}\nWrapStyle: 0\n\n"
              "[V4+ Styles]\nFormat: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, "
              "BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, "
              "Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\n"
              f"Style: Default,{FONT_NAME},{font_size},&H00FFFFFF,&H00FFFFFF,&H00000000,&H00000000,-1,0,0,0,100,100,0,0,"
              f"1,{max(2, font_size // 16)},0,{CAPTION_ALIGNMENT[captions.get('position', 'bottom')]},"
              f"{font_size},{font_size},{font_size * 2},1\n\n"
              "[Events]\nFormat: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n")
    events = [f"Dialogue: 0,{ass_stamp(line[0][0])},{ass_stamp(line[-1][1])},Default,,0,0,0,,{spoken(line)}\n"
              for line in lines]
    (render_folder / "captions.ass").write_text(header + "".join(events), encoding="utf-8")
    return "ass=captions.ass"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("edit", type=pathlib.Path)
    parser.add_argument("--draft", action="store_true")
    arguments = parser.parse_args()
    edit_path = arguments.edit.resolve()
    folder = edit_path.parent
    edit = json.loads(edit_path.read_text(encoding="utf-8"))
    output = folder / edit["output"]
    render_folder = output.parent / f"{output.stem}.render"
    render_folder.mkdir(parents=True, exist_ok=True)
    clips = edit["clips"]

    first_video = next(stream for clip in clips if "source" in clip
                       for stream in probe(folder / clip["source"]) if stream["codec_type"] == "video")
    size = edit.get("size", [first_video["width"], first_video["height"]])
    inputs = []
    graph = []
    for index, clip in enumerate(clips):
        arguments_for_clip, has_audio = clip_inputs(clip, folder, size, edit.get("fps", 30))
        inputs += arguments_for_clip
        graph += clip_graph(clip, index, index, has_audio, edit, size, render_folder)

    graph += join_graph(clips, edit.get("transition"))
    starts, _ = clip_starts(clips, edit.get("transition"))
    sounds = [(start + sound["at"], sound) for clip, start in zip(clips, starts) for sound in clip.get("sounds", [])]
    for number, (at, sound) in enumerate(sounds):
        inputs += ["-i", str(folder / sound["source"])]
        length = f":duration={sound['for']}" if "for" in sound else ""
        graph.append(f"[{len(clips) + number}:a]atrim=start={sound.get('from', 0)}{length},asetpts=PTS-STARTPTS,"
                     f"volume={sound.get('volume', 1.0)},aresample={SAMPLE_RATE},aformat=channel_layouts=stereo,"
                     f"areverse,afade=t=in:d={EDGE_FADE * 5},areverse,adelay={round(at * 1000)}:all=1[s{number}]")

    if sounds:
        placed = "".join(f"[s{number}]" for number in range(len(sounds)))
        graph.append(f"[ajoin]{placed}amix=inputs={len(sounds) + 1}:normalize=0:duration=first[aplaced]")
    video_filters = [write_captions(edit, folder, render_folder, size)] if "captions" in edit else []
    graph.append(f"[vjoin]{','.join(video_filters + ['format=yuv420p'])}[vout]")  # xfade negotiates 4:4:4, which players refuse

    sound = "[aplaced]" if sounds else "[ajoin]"
    if "music" in edit:
        music = edit["music"]
        inputs += ["-stream_loop", "-1", "-i", str(folder / music["source"])]
        graph.append(f"[{len(clips) + len(sounds)}:a]volume={music.get('volume', 0.25)},aresample={SAMPLE_RATE},"
                     "aformat=channel_layouts=stereo[music]")
        if music.get("duck", True):
            graph.append(f"{sound}asplit[speech][key];[music][key]sidechaincompress=threshold=0.03:ratio=8"
                         ":attack=20:release=400[ducked]")
            graph.append("[speech][ducked]amix=inputs=2:normalize=0:duration=first[amixed]")
        else:
            graph.append(f"{sound}[music]amix=inputs=2:normalize=0:duration=first[amixed]")

        sound = "[amixed]"

    graph.append(f"{sound}loudnorm=I={edit.get('loudness', -14)}:TP=-1.5:LRA=11,aresample={SAMPLE_RATE},"
                 "alimiter=limit=0.84:level=false[aout]")  # one-pass loudnorm lets hard-cut transients past its peak
    (render_folder / "graph.txt").write_text(";\n".join(graph), encoding="utf-8")

    if arguments.draft:
        encoder = ["-c:v", "libx264", "-preset", "ultrafast", "-crf", "30"]
    elif nvenc_available():
        encoder = ["-c:v", "h264_nvenc", "-preset", "p5", "-cq", "21"]
    else:
        encoder = ["-c:v", "libx264", "-preset", "medium", "-crf", "20"]

    subprocess.run(["ffmpeg", "-hide_banner", "-v", "error", "-nostats", "-y", *inputs,
                    "-/filter_complex", "graph.txt", "-map", "[vout]", "-map", "[aout]", *encoder,
                    "-c:a", "aac", "-b:a", "192k", "-movflags", "+faststart", str(output)],
                   cwd=render_folder, check=True)
    _, total = clip_starts(clips, edit.get("transition"))
    print(f"{output}  {total:.2f} s  {size[0]}x{size[1]}  {encoder[1]}")


if __name__ == "__main__":
    main()
