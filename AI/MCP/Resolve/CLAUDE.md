# MCP DaVinci Resolve

Timelines, media pool, grading, Fusion, renders from the model. Tool map and workflows: `ResolveTools.md`.

## Setup
| Part | Role |
|---|---|
| `davinci-resolve` MCP server (`~/davinci-resolve-mcp`, own `.venv`) | Tools, each one HTTP call to the bridge or a local AI job |
| CursorBridge (`…\Fusion\Scripts\Utility\CursorBridge.py`) | HTTP server `127.0.0.1:9876` inside Resolve |
| Local AI (faster-whisper, Demucs, rembg) | Transcription, stems, background removal, CPU |

- `mcp` package < 2; venv has no pip (`uv pip install --python .venv/Scripts/python.exe`).
- Installed bridge file must match the repo's; re-copy and re-run after pulling.
- Machine: Resolve Free 21.0.4, the last Free that runs `.py`; do not update past 21.0.x. Studio + external scripting Local runs the bridge outside Resolve.

## Connect
- The user starts the bridge each session (Workspace > Scripts > CursorBridge; console says `Bridge is running`). First call = status tool; a connection error means ask, never loop.
- MCP client down → call the endpoints on `127.0.0.1:9876` directly (GET reads, POST JSON writes, camelCase keys; routes at the bottom of the bridge file).
- One request at a time; reads 10 s, writes 15 s; a timeout may still complete: read back before resending. Exceptions return HTTP 500 with the traceback.

## Addressing
- Current project and timeline: read timeline info first; switch timeline (1-based) before editing another.
- Tracks 1-based, clips 0-based per track, recomputed every call: re-list after insert / delete / ripple.
- Clip start / end = absolute record frames incl. start timecode offset (01:00:00:00 @ 24 fps = 86400). Timeline markers relative to timeline start, clip markers to clip start. Insert takes an absolute record frame. Playhead takes a timecode string in the timeline's timecode.
- Pool clips found by name, first match wins: unique names. Read tools put params unencoded in the URL (spaces, `&`, `#`, `+` fail); writes take any name.
- Pool listing shows the current folder only; folder-tree tool shows all.
- Writes return Resolve's bare boolean: read state back. Clip zoom `1.0` = 100 %, opacity 0–100. Studio-only features return false on Free.

## Rules
- Confirm destructive calls first (delete project / clips / tracks / jobs / stills / versions, reset grades, switch database, relink).
- Duplicate the timeline before a large edit; save explicitly at the end.
- Outputs go to the video's `<name>.edit/` or the scratchpad; always pass an output path to local AI tools.

## Seeing
- Move playhead, export current frame PNG to the scratchpad, read it. After a timeline import, switch to it by index first.
- No clip transform read: check framing with `AI/Python/Video/locate_frame.py`. Judge a render with `video_scan.py`.

## Resolve or FFmpeg
- Resolve: a project to keep editing by hand, grading, Fusion, interchange (FCPXML, EDL, AAF, OTIO).
- FFmpeg (`../Video/`): finished render straight from a prompt (cuts, captions, fades, transitions). They meet through files.
