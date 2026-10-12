# MCP Audacity

Audio editing through the `audacity` MCP server, driving a running Audacity over mod-script-pipe.

## Connect
- Module enabled in Edit > Preferences > Modules, then Audacity restarted (`audacity.cfg` module entry `1`).
- Pipe opened per call: Audacity may start / restart mid-session. `PIPE_NOT_FOUND` = unreachable; project info confirms.
- Unsaved work may be open: ask the user to restart it.

## Command model
- A tool runs one scripting command on the focused project's selection (tracks + time region), not the newest window: read track names before every edit.
- Results never describe the change: read tracks / clips back.
- Project info with the `Commands` type lists real parameter names.

## Arguments that do not reach Audacity
| Tool argument | Use instead |
|---|---|
| Generator `duration` | Selected region sets the length |
| Tone `waveform` | Triangle is blocked and Square is misspelled (yields default 440 Hz sine); use Chirp, which passes both |
| Adjustable fade `fade_type` | Plain fade in / out |
| Distortion `threshold_db` | Only the type applies |
| Reverb `pre_delay` | `Delay` |
| Tremolo / Compressor / Limiter args | Audacity's own names (`WAVE`, `PHASE`, `WET`, `LFO`; `thresholdDb`, `compressionRatio`, `attackMs`, `releaseMs`, `makeupGainDb`; `makeupTargetDb`, `kneeWidthDb`, `lookaheadMs`) |
| Track properties `track` / `gain` / `pan` / `mute` / `solo` | Name lands on the selected track: select first |

## Generate and keep
- Generators write into the selected region of the selected tracks; a new track starts selected.
- Save with a path (plain save is rejected) before exporting; export writes selected tracks over the selected region.
- Every layer on its own named track; never mix and render; export alone mixes.

## Traps
- The focused project is the target: opening a new project does not reliably retarget and the user clicking another window moves it mid-work. Read track names before each generate / effect chain; if they belong to another project, stop and ask for the right window.
- `track_resample` opens a modal "New sample rate" dialog and freezes the server after its 30 s timeout until `/mcp` reconnects: never call it.
- New tracks come in at the default rate (48 kHz) even in a 44.1 kHz project; pasting 44.1 kHz audio silently resamples and snaps clips to the 48 kHz grid. Paste into tracks that already match the source rate.
- `select_region` rejects start == end: place a paste cursor with `cursor_set_position`.
- The server holds the pipe for its whole life: no second client can connect.
- Save the `.aup3` before every `.wav` export; play the result through the transport so the user hears it before the next sound.
