# MCP Video

Video and audio editing on local FFmpeg; prompt → edit workflow in `VideoEditing.md`.

## Setup
| Part | Role |
|---|---|
| FFmpeg full build (`ffmpeg`, `ffprobe` on PATH) | Every decode, filter, encode (libass, NVENC, scene + loudness filters) |
| `video-audio` MCP server (`~/video-audio-mcp`, via `uv`) | One-step operations |
| `AI/Python/Video/video_scan.py` | How the model sees a video |
| `AI/Python/Video/video_render.py` | Renders an edit list: the real editing engine |
| faster-whisper `large-v3-turbo` (CUDA) | Transcript with word timings |

- NVENC needs NVIDIA driver ≥ 610, else x264 fallback.
- Transcription is GPU faster-whisper; never ffmpeg's whisper filter.

## Seeing footage
- The model knows footage only through the scan: contact sheets, transcript, loudness, scene cuts joined in `timeline.txt`. Read it first, then sheets around its times.
- Span scan (`--start`, `--end`, small `--every`) places cuts to a fraction of a second; times stay the source's.
- Judge a render by scanning it the same way.

## Working files
- Per video: `<name>.edit/` beside it (scans, edit lists, renders); never the project tree or `Saved/`.
- Edit list = JSON in that folder, paths relative to it (format in `video_render.py` docstring); a render writes `<output>.render/` (filter graph, captions) — first place to look on error.
- Renders overwrite: new output name to keep a version.

## FFmpeg on Windows
- Escape drive colons in filters (`C\:/…`) or run from the file's folder with relative names.
- Text filters name a font file.
- Cuts that will be seen are re-encoded (stream copy snaps to keyframes).
- ASS captions on the output canvas in pixels; SRT burns far too large.
- Filter graph in a file (`-/filter_complex`).
- Output forced to `yuv420p` (High); probe every render.
- Detection branches pass every frame with its score; the script thresholds.
- One timebase before any crossfade; 10 ms audio fades per clip, limiter after loudness normalisation.

## MCP tools
| Use | Tools |
|---|---|
| Reliable | health check, audio extraction, format / codec / bitrate / rate / channel / resolution / frame-rate changes, speed, aspect ratio, fades, silence removal |
| Keyframe-snapped | trim: only to pull a span to look at |
| Unusable | subtitles, text overlay, concatenation with transition, B-roll |

- Always a new output path; an encode far past its length is stalled — stop it and use the render script.
- Anything multi-step (cuts, text, captions, music, framing) = render script.
