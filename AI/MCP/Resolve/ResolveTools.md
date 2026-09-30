# Resolve Tools and Workflows

The tool map by area, what each area needs, and the order of calls for common jobs. Conventions (indices, frames,
names, results) are in `CLAUDE.md`; every tool's arguments are in its docstring in
`~/davinci-resolve-mcp/src/resolve_mcp_bridge.py`.

## Reading state

| Need | Tools |
|---|---|
| Is the bridge up | `get_resolve_status` — always the first call |
| Project, page, timeline | `get_project_info` (timeline count, resolution, fps), `get_current_page`, `get_timeline_info` (start frame, playhead, track counts and names, in/out) |
| What is on a track | `get_timeline_clips` (name, start, end, duration, source path, fps) |
| What is under the playhead | `get_current_video_item` |
| One clip in depth | `get_clip_properties` (transform), `get_clip_markers`, `get_clip_flags`, `get_linked_items`, `get_node_graph`, `get_color_versions`, `get_fusion_comps`, `get_takes` |
| Media pool | `get_media_pool` (current folder), `get_media_pool_structure` (whole tree), `get_clip_info`, `get_clip_metadata` |
| Render | `get_render_settings`, `get_render_formats`, `get_render_resolutions`, `get_quick_export_presets`, `get_render_job_status` |
| Everything else | `get_project_list`, `get_database_list`, `get_gallery_albums`, `get_album_stills`, `get_color_groups`, `get_fairlight_presets`, `get_keyframe_mode`, `get_media_storage` |

- Read the timeline info and the tracks before any edit, and again after it.
- A track listing with source paths is how the model maps timeline clips back to files on disk.

## Editing the timeline

| Job | Tools |
|---|---|
| Timelines | `create_timeline`, `switch_timeline`, `rename_timeline`, `duplicate_timeline`, `import_timeline_from_file`, `export_timeline` |
| Tracks | `add_track` (audio takes a channel layout), `delete_track`, `set_track_enable`, `set_track_lock`, `set_track_name` |
| Placing media | `append_to_timeline` (end of the timeline), `insert_to_timeline` (track, record frame, source in and out) |
| Removing | `delete_timeline_clips` (list of clip references, optional ripple) |
| Grouping | `link_timeline_clips`, `create_compound_clip`, `create_fusion_clip` |
| Clip look | `set_clip_properties` (pan, tilt, zoom, rotation, opacity, crop, flip, anchor, composite mode, scaling), `set_clip_enabled`, `set_clip_color` |
| Markers and flags | `add_marker`, `delete_markers`, `add_clip_marker`, `delete_clip_markers`, `add_clip_flag`, `clear_clip_flags` |
| In and out | `set_timeline_mark_in_out`, `clear_timeline_mark_in_out` |
| Titles and generators | `insert_title`, `insert_generator`, `insert_fusion_composition` — all at the playhead, on the current track |
| Takes | `add_take`, `select_take`, `delete_take`, `finalize_take` |

- An insert places video and linked audio together; the tools offer no video-only or audio-only insert.
- Clip references in list tools are `{trackType, trackIndex, clipIndex}` objects, all read from one listing taken
  before the call.
- Delete with ripple shifts every later clip: re-list before the next reference.
- Markers carry a free `customData` string: use it to tag markers the model owns, and delete by it.
- Titles insert with template text; the API cannot set a Text+ string. For a titled clip, write a `.comp` file with
  the text already in it and import it onto a clip with the Fusion comp import tool.
- The API cannot add transitions or keyframes: a crossfade, a fade or an animated move is done by hand in Resolve,
  or rendered by the FFmpeg pipeline.

## Media pool

| Job | Tools |
|---|---|
| Import | `import_media` (absolute Windows paths), `import_media_from_storage` (Media Storage paths) |
| Folders | `navigate_media_pool` (slash path, `root`), `create_media_pool_folder`, `move_media_pool_clips` |
| Clips | `set_clip_metadata`, `set_pool_clip_property` (the key `Clip Name` renames), `delete_media_pool_clips`, `replace_clip` |
| Media links | `relink_media_pool_clips`, `unlink_media_pool_clips`, `link_proxy_media`, `unlink_proxy_media` |
| Sync and export | `auto_sync_audio` (timecode or waveform), `export_metadata` (CSV) |

- Imports land in the current pool folder: navigate first, then import.
- An import returns the names Resolve gave the clips; use those names, never the file names, afterwards.
- Rename every imported clip to a unique name before any name-based call.

## Color

| Job | Tools |
|---|---|
| Nodes | `get_node_graph`, `set_node_enabled`, `reset_node_colors` |
| LUTs | `set_lut`, `get_lut`, `export_lut` (17, 33 or 65 point cube), `refresh_lut_list`, `apply_arri_cdl_lut` |
| Grades | `set_cdl` (slope, offset, power as space-separated RGB strings), `apply_grade_from_drx`, `copy_grades`, `reset_all_grades` |
| Versions and groups | `add_color_version`, `load_color_version`, `rename_color_version`, `delete_color_version`, `add_color_group`, `assign_to_color_group`, `remove_from_color_group`, `delete_color_group` |
| Gallery | `create_gallery_album`, `set_current_album`, `grab_still`, `grab_all_stills`, `export_stills`, `import_stills`, `delete_stills`, `set_still_label` |

- Nodes are 1-based; a node must already exist — the API sets values on nodes, never adds them.
- A LUT path is absolute, or relative to Resolve's LUT folder; after copying a new LUT file there, refresh the list.
- Stills, grabs and the thumbnail need the Color page open.
- Copying a grade from one clip is the way to grade many: grade one, then copy to the rest.

## Fusion

| Job | Tools |
|---|---|
| Comps on a clip | `get_fusion_comps`, `add_fusion_comp_to_clip`, `import_fusion_comp_to_clip`, `export_fusion_comp_from_clip`, `load_fusion_comp_on_clip`, `rename_fusion_comp_on_clip`, `delete_fusion_comp_on_clip` |

- Node parameters inside a comp are unreachable; a comp is authored as a `.comp` text file and imported whole.
- Exporting a comp made by hand gives a template to edit as text and re-import.

## Audio

| Job | Tools |
|---|---|
| Fairlight | `get_fairlight_presets`, `apply_fairlight_preset`, `insert_audio_at_playhead` (offset and duration in samples, on the selected track, Fairlight page) |
| Resolve's voice isolation | `get_voice_isolation_state`, `set_voice_isolation_state` — Studio only |

## Rendering

| Job | Tools |
|---|---|
| Configure | `set_render_format`, `set_render_settings` (target folder, name, in and out, size, frame rate, quality, audio), `set_render_mode` (individual clips or single clip), `render_preset` (load, save, list, import, export) |
| Queue | `add_render_job`, `start_rendering`, `get_render_job_status`, `stop_rendering`, `delete_render_job` |
| Presets | `quick_export` with a name from the preset list |
| Frames | `export_current_frame` (png, jpg, tif, dpx and more) |

- Order: format and codec, then settings, then add a job, then start it; a job added before its settings keeps the
  old ones.
- Starting a render returns at once; poll the job status until it reports complete, then scan the file.
- Format and codec names come from the format listing, never guessed.
- `SelectAllFrames` renders the whole timeline; otherwise set `MarkIn` and `MarkOut` in absolute frames.

## Project level

| Job | Tools |
|---|---|
| Projects | `get_project_list`, `load_project`, `create_project`, `save_project`, `export_project`, `import_project`, `archive_project`, `delete_project`, `navigate_project_folder` |
| Settings | `set_project_setting`, `set_timeline_setting` (values are strings) |
| Resolve | `open_page`, `set_playhead`, `layout_preset`, `burnin_preset`, `set_keyframe_mode`, `set_database`, `reveal_in_storage` |

- Timeline resolution and frame rate are set before any media goes on the timeline; Resolve locks frame rate once
  clips are placed.
- Loading another project drops the current timeline context: read the project and timeline info again.

## Studio-only tools

- `create_subtitles_from_audio`, `detect_scene_cuts`, `create_magic_mask`, `regenerate_magic_mask`,
  `stabilize_clip`, `smart_reframe_clip`, `set_voice_isolation_state`.
- On Free they return `success: false`; use the local AI tools below, or the scene cuts of `video_scan.py`.

## Local AI tools

| Tool | Does | Notes |
|---|---|---|
| `transcribe_file` | Whisper transcript with word timings, of any file | CPU int8; `small` by default, first use downloads the model |
| `transcribe_timeline` | Same, of the timeline's audio | Transcribes the whole source file of the first clip on audio track 1: times are source times, not timeline times, and the rest of the edit is ignored |
| `voice_isolate` | Demucs stems: one stem and the rest, as WAV | About 1.5× real time on the CPU; `vocals`, `drums` or `bass` |
| `voice_isolate_timeline` | Same, on the first clip of audio track 1 | Whole source file, as above |
| `remove_background` | One image to a transparent PNG | BiRefNet by default |
| `remove_background_video` | Every frame of a video to transparent PNGs, or a black-and-white matte MP4 | 0.5 to 2 s per frame; whole file, no range; one PNG per frame on disk |
| `remove_background_clip` | Same, on a timeline clip's whole source file | |

- These run inside the MCP server: while one runs, every other tool, Resolve ones included, waits for it.
- For transcription, the GPU transcript of `video_scan.py` is about a hundred times faster; use the server's only
  when a caller needs its exact output shape.
- Trim a video to the span that matters with FFmpeg before removing its background.
- Every one of them takes an output path or folder; always pass one in the video's `.edit/` folder or the scratchpad.
- A background-removed PNG sequence imports into the pool as an image sequence and composites over a lower track.

## Workflows

**Assemble a rough cut from files.** Read the project and timeline info; create a timeline; set its resolution and
frame rate; create a pool folder and navigate into it; import the files; rename the clips uniquely; insert each
clip by record frame (timeline start plus offset) with its source in and out; list the track and check every start
and end; export frames at the cut points and read them; save the project.

**Mark moments for the user.** Scan the video with `video_scan.py`; convert each moment's seconds to frames at the
timeline rate; add timeline markers relative to the start, with names and notes and a `customData` tag; read the
markers back.

**Grade a batch.** Grade the reference clip (LUT, CDL, or a DRX still); read its node graph; copy its grade to the
other clips; grab a still of each on the Color page and export them to look at.

**Deliver.** List formats, then codecs for the chosen format; set format and codec; set target folder, name and
range; add a job; start it; poll its status; scan the file; report its path.

**Hand off to another editor.** Export the timeline as FCPXML, EDL, AAF or OTIO into the `.edit/` folder; the
media stays where it is, so relink paths travel with the file.
