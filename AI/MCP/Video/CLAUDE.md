# MCP Video

Video and audio editing on the local FFmpeg — any footage, any output: shorts, long cuts, montages, made-from-scratch
pieces. Read `VideoEditing.md` in this folder for how to turn a prompt into an edit.

## Setup

| Part | Where | Role |
|---|---|---|
| FFmpeg 9 full build (gyan.dev, WinGet) | `ffmpeg`, `ffprobe` on PATH | Every decode, filter and encode |
| `video-audio` MCP server | `~/video-audio-mcp`, run by `uv` from the user MCP config | One-step operations as tools |
| `AI/Python/Video/video_scan.py` | run with `uv` | How the model sees a video — sheets, cuts, loudness, transcript |
| `AI/Python/Video/video_render.py` | run with `uv` | Renders an edit list — the real editing engine |
| faster-whisper `large-v3-turbo` on CUDA | Hugging Face cache, fetched on first use | Transcript with word timings |

- The MCP health check confirms the server; `ffmpeg -version` confirms the binary.
- The build carries libass, freetype, NVENC, scene and loudness filters, and whisper.cpp.
- Transcription runs on the GPU through faster-whisper, about a hundred times real time; ffmpeg's own whisper filter
  runs on the CPU slower than real time, so it is never used.
- NVENC needs NVIDIA driver 610 or newer; below that, renders fall back to x264 on their own. Updating the driver
  makes long renders several times faster.

## The model cannot watch a video

- Everything the model knows of footage comes from the scan: contact sheets it reads as images, a transcript, a
  loudness curve and scene cuts, joined in `timeline.txt`.
- Read `timeline.txt` first, then only the sheets around the times it points to.
- A span scan (`--start`, `--end`, a small `--every`) zooms into a moment to place in and out points to a fraction
  of a second; its times stay the source's.
- Judge a render by scanning it the same way — never report a render as good without reading its sheets.

## Working files

- Each video gets a `<name>.edit/` folder beside it: scans, edit lists, renders. Nothing goes into the project tree
  and never into `Saved/`.
- An edit list is a JSON file in that folder; its paths are relative to it. The format is the render script's
  docstring.
- A render writes `<output>.render/` beside itself: the filter graph, captions and text files it used — the first
  place to look when a render errors.
- Renders overwrite their output; keep a version by naming a new output.

## FFmpeg on Windows

- A drive-letter colon inside a filter option splits the option: escape it (`C\:/...`) or run FFmpeg from the
  file's folder and pass a relative name.
- Fontconfig has no configuration on Windows: every text filter names a font file, or FFmpeg exits.
- Stream copy cuts only on keyframes, so the first seconds of a copied cut hold frames from before the in point;
  any cut that will be seen is re-encoded.
- ASS captions laid out on the output's own canvas size take sizes in pixels; SRT captions are laid out on a
  288-line canvas and burn several times too large.
- A filter graph goes in a file (`-/filter_complex`), since a long edit passes the command-line length limit.
- Crossfades negotiate 4:4:4 colour, which Windows players and web sites refuse (`0xc1010103`): the output is forced
  to `yuv420p`, and every render is probed for profile High and `yuv420p` before it is handed over.
- An output that receives no frames fails the whole run, so a detection branch passes every frame with its score
  and the script applies the threshold.
- Joined streams need one timebase: a hard-cut join outputs microseconds, so every clip is set to the same before
  any crossfade meets it.
- Audio cut mid-waveform clicks, and the encoder turns a click into a peak over 0 dB; every clip's audio fades over
  its first and last 10 ms, and a limiter follows the one-pass loudness normalisation.

## MCP tools

The tools each run one FFmpeg command from input path to output path and return its log.

| Use | Tools |
|---|---|
| Reliable | health check, audio extraction, format, codec, bitrate, sample rate, channel, resolution and frame-rate changes, speed change, aspect ratio (pad or crop, centred), fade in or out, silence removal |
| Keyframe-snapped | trim — fine for pulling a span to look at, never for a cut in an edit |
| Unusable here | subtitles (any absolute Windows path fails), text overlay (fails without `font_file`, stalls with it), concatenation with a transition (malformed graph with audio), B-roll |

- Encodes run without `-nostdin` or `-y`: always give a new output path, and treat an encode running far past its
  length as stalled — stop its task and its `ffmpeg` process, and do the job through the render script.
- Tool errors return the whole FFmpeg banner; the cause is in the last few lines.
- Anything with more than one step — a cut list, text, captions, music, framing — goes through the render script.
