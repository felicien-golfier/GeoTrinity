# AI/Python/Video

Run with `uv run <script>` outside the editor, on the FFmpeg on PATH. Workflow: `AI/MCP/Video/CLAUDE.md`.

| Script | Does |
|---|---|
| `video_scan.py` | See a video: contact sheets, scene cuts, loudness per 100 ms, GPU transcript, joined timeline; a span zooms in keeping source times |
| `video_bar.py` | Find when an on-screen bar empties and refills (deaths from a health bar) |
| `video_render.py` | Render an edit list in one FFmpeg pass: cuts, cards, crop / zoom, sounds, framing, speed, text, transitions, captions, ducked music, loudness |
| `edit_to_fcpxml.py` | Edit list → FCPXML + captions SRT for DaVinci Resolve |
| `locate_frame.py` | Where a reframed frame sits in its source (check a zoom's crop centre) |
