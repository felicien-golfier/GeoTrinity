# AI/Python/Runtime

Reading and driving a running session or the editor world. Docs: `AI/MCP/MCP_PIE.md`, `MCP_Preview.md`.

| Script | Does |
|---|---|
| `run_via_bridge.py` | Host-side: run an editor script file over the bridge HTTP route, print OK or traceback |
| `pie_inject_input.py` | Inject input actions into a session, measure the result |
| `pie_drive_menu_ui.py` | Drive live menu widgets without simulating input |
| `pie_layout_audit.py` | List UI faults on screen (overlapping texts, spilling widgets); run after opening each page |
| `pie_leave_tutorial.py` | Standalone-style game (`-game -nullrhi`, `py` through `-ExecCmds`): travel Tutorial → Training Room and report where the player spawns |
| `pie_tutorial_flow.py` | `-game -nullrhi` check of the tutorial chain: moves the pawn, kills dummies under KillZ, steps on the exit pads (real teleports), reports pad openings, seals and the 3D Cat's visibility Landing → Shoot; cuts each Cat line's reading time (`LineSeconds`), since lines run on Slate time, not slomo |
| `decode_tool_image.py` | Host-side: base64 image from an oversized `mcp-epic` result file → PNG (`MCP_EpicTesting.md`) |
| `couch_coop_debug.py` | Dump couch-coop input ownership |
| `vfx_editor_preview.py` | Set up the editor world to judge a VFX: preview systems, collection values, camera |
