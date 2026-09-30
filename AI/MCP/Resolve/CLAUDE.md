# MCP DaVinci Resolve

Driving DaVinci Resolve (timelines, media pool, grading, Fusion comps, renders) from the model. Read `ResolveTools.md`
in this folder for the tool map by area and the end-to-end workflows.

## Setup

| Part | Where | Role |
|---|---|---|
| `davinci-resolve` MCP server | `~/davinci-resolve-mcp/src/resolve_mcp_bridge.py`, run by its `.venv` Python from the user MCP config | 162 tools; each one is a single HTTP call to the bridge, or a local AI job |
| CursorBridge | `%APPDATA%\Blackmagic Design\DaVinci Resolve\Support\Fusion\Scripts\Utility\CursorBridge.py` | HTTP server on `127.0.0.1:9876` running inside Resolve, calling Resolve's own scripting API |
| Local AI | same `.venv`: faster-whisper, Demucs, rembg | Transcription, stem separation, background removal, on the CPU |
| FFmpeg | on PATH (WinGet) | Frame extraction for video background removal |

- The server needs the `mcp` package below 2 (`requirements.txt` pins it); 2.x renamed the server class, and the
  server then dies on import, which the client reports as a closed connection.
- The venv is uv-made and has no pip: install into it with `uv pip install --python .venv/Scripts/python.exe`.
- The bridge file installed in Resolve must match the one in the repo; after pulling the repo, copy it again and
  re-run it in Resolve.
- Resolve 21 bundles its own Python (`ResolvePython/`, 3.14); no system Python is needed for the bridge.

## Current state on this machine

- Resolve 21.1 Free lists no `.py` file under Workspace → Scripts, whatever Python is installed (its built-in one,
  python.org 3.12 on PATH); a `.lua` file is listed. The bridge cannot start, so every Resolve tool returns the
  connection error.
- The local AI tools (transcription, stem separation, background removal) work without Resolve.
- With Resolve Studio, external scripting set to Local (Preferences → System → General) lets the bridge run outside
  Resolve under `ResolvePython.exe`, which reaches Resolve through its scripting module; no menu is needed.
- This doc describes the tools as the source defines them, ready for either path.

## Connection

- The user starts the bridge by hand each Resolve session: open a project, then Workspace → Scripts → CursorBridge;
  the Console (F6) prints `Bridge is running`. The model cannot start it.
- The first call of a session is the status tool; a connection error means the bridge is not running — ask the user
  to start it, never retry in a loop.
- Re-running the bridge replaces the previous instance on the same port.
- A failed MCP client never means Resolve is unreachable: every tool maps to one endpoint on `127.0.0.1:9876`
  (GET for reads, POST with a JSON body for writes), listed in the route tables at the bottom of the bridge file.
  Body keys are camelCase (`trackType`, `clipIndex`); the tool's own function shows its endpoint and body.
- The bridge refuses requests carrying an `Origin` header or a host other than `127.0.0.1:9876`/`localhost:9876`.
- The bridge serves one request at a time; a slow Resolve call holds every other call.
- The server gives reads 10 s and writes 15 s; a call that times out may still complete in Resolve, so read the
  state back before sending it again.
- A bridge-side exception comes back as HTTP 500 with the Python traceback as the error text.

## Addressing conventions

- Everything acts on the current project and its current timeline: read the timeline info first, and switch
  timeline (1-based index) before editing another one.
- Tracks are 1-based; clips are 0-based in the order Resolve lists them on that track.
- Clip indices are recomputed on every call: after an insert, delete or ripple, list the track again before the next
  index-based call.
- Timeline clip start and end are absolute record frames, including the timeline's start timecode offset (a
  timeline starting at `01:00:00:00` at 24 fps starts at frame 86400).
- Timeline markers take a frame relative to the timeline start; clip markers take a frame relative to the clip start.
- Inserting a clip takes an absolute record frame: timeline start frame plus the offset. The tool always sends a
  record frame (default 0), so pass it explicitly.
- The playhead takes a timecode string in the timeline's own timecode (`01:00:05:00`, not `00:00:05:00`, on a
  timeline starting at one hour).
- Media pool clips are found by name through the whole pool, first match wins: give every imported clip a unique
  name, or the wrong clip is edited.
- Read tools send their parameters unencoded in the URL: a clip name with a space, `&`, `#` or `+` fails on the
  metadata and clip-info reads; the write tools send JSON and take any name.
- The media pool listing shows only the current folder; the folder-tree tool shows all of it.

## Results

- Every write returns Resolve's own boolean: `success: false` carries no reason. Read the state back after every
  write that matters.
- A multi-property clip write returns a per-key result map; `success` is false as soon as one key failed.
- Clip transform zoom is a scale factor where `1.0` is 100 %; opacity runs 0–100.
- Studio-only features return `success: false` on Free, never an explanation.

## Working rules

- Confirm with the user before any destructive call: deleting a project, clips from the pool or timeline, a track,
  render jobs, stills, color versions or groups; resetting grades; switching database; unlinking or relinking media.
- Duplicate the timeline before a large edit; the scripting API has no undo of its own.
- Save the project explicitly at the end of a session of edits.
- Everything the tools write (renders, frames, stills, exports, AI outputs) goes to the video's `<name>.edit/`
  folder beside it or to the session scratchpad — never the project tree, never `Saved/`.
- The local AI tools default their output next to the source file (`davinci-mcp-output/`); always pass an output
  path or folder.

## The model cannot watch the timeline

- To see a frame: move the playhead, export the current frame as PNG into the scratchpad, and read the image.
- The thumbnail tool returns base64 RGB and works only on the Color page; the frame export is lighter and works on
  any page.
- To judge a render, scan the output file with `AI/Python/Video/video_scan.py` as in `../Video/CLAUDE.md`.

## Resolve or FFmpeg

- Resolve when the user wants a project they will keep editing by hand, a grade, a Fusion comp, or an interchange
  file (FCPXML, EDL, AAF, OTIO).
- The FFmpeg pipeline (`../Video/`) for a finished render straight from a prompt: it cuts, captions, fades and
  transitions, which the Resolve API cannot.
- Both meet through files: an FFmpeg render imports into the pool, a Resolve render is scanned like any video.
