# MCP Audacity

Audio editing through the `audacity` MCP server, which drives a running Audacity over its scripting pipe.

## Connecting

- The server reaches Audacity through the mod-script-pipe module, so Audacity must be running with it enabled.
- Enable it in Edit > Preferences > Modules, then restart Audacity — the module only loads at startup.
- In `%APPDATA%\audacity\audacity.cfg` the module entry reads `1` when enabled; `4` means detected, never enabled.
- Change the setting through Preferences, not the cfg — a running Audacity rewrites the cfg on exit.
- Audacity may hold unsaved work: ask the user to restart it rather than doing it yourself.
- The pipe is opened per call, so Audacity can start or restart mid-session without reconnecting the MCP client.
- An unreachable pipe errors with `PIPE_NOT_FOUND`; reading the project's track info confirms the connection.

## Command model

- Each tool runs one Audacity scripting command against the current selection: selected tracks and a time region.
- Commands go to the focused project window, not the newest one: a new project or a click by the user in another
  window moves the target. Read the track names before every edit and stop if they are not the expected project's.
- A rejected command reports only a generic failure, never a reason — check the selection first.
- A success result does not describe the change; read tracks or clips back from project info to verify it.
- An argument whose name Audacity does not know is dropped without error, and the effect keeps its last-used value.
- Project info with the `Commands` type lists every command's real parameter names — the reference for any doubt.

## Arguments that never reach Audacity

| Tool argument | What Audacity expects instead |
|---|---|
| Generator `duration` (tone, chirp, noise) | No length parameter — the selected region sets it. |
| Tone `waveform` | Its allowed list blocks Triangle and misspells `Square, no alias`; chirp passes both through. |
| Distortion `threshold_db` | `Threshold dB`, `Parameter 1`, `Parameter 2` — only the type applies. |
| Reverb `pre_delay` | `Delay`. |
| Tremolo, every argument | `WAVE`, `PHASE`, `WET`, `LFO`. |
| Compressor, every argument | `thresholdDb`, `compressionRatio`, `attackMs`, `releaseMs`, `makeupGainDb`. |
| Limiter, every argument | `thresholdDb`, `makeupTargetDb`, `kneeWidthDb`, `lookaheadMs`, `releaseMs`. |
| Track properties `track`, `gain`, `pan`, `mute`, `solo`; track mute | The index never applies — the name lands on the selected track, so select it first; the rest belong to the unexposed track-audio command. |

## Generating audio

- Generators write into the selected time region of the selected tracks, so both must exist first.
- A newly added track starts selected; select a time region on it, then generate.
- The selected region sets the generated length; the duration argument does not create a selection.

## Keeping the result

- Every edit changes the open project only — save the project or export audio to keep it.
- The plain save tool is rejected; save with a path, or ask the user to press Ctrl+S in the window.
- Never mix and render: layers stay on separate named tracks, saved in the project; export alone does the mixing.
