# MCP Live Coding & Connection

Building `.cpp` changes while the editor is open, and getting the MCP bridge connected so PIE can be driven.

---

## Build with the editor open (Live Coding)

When the editor is running it locks the module DLLs, so a full link fails with `LNK1104: cannot open file ...dll`. The compile step still validates the code; only the link is blocked.

- For implementation-only `.cpp` changes, do not run the full build — trigger Live Coding from the editor (`Ctrl+Alt+F11`). It patches the running process without unloading DLLs.
- A new `UFUNCTION`, new type, or header change cannot be hot-patched: ask the user to close the editor, then run the full build from `AI/Commands.md`.
- A `LNK1104` on the module DLLs means the editor is holding them — that is the signal to use Live Coding, not a code error.

---

## Connecting the MCP bridge

The editor hosts the bridge HTTP server (see its startup line in the log for the bound `127.0.0.1` port and route count). The `mcp-unreal` client is spawned at Claude Code session start.

- The editor must already be serving the bridge before the MCP client connects, or the client registers as failed and its tools never load.
- If `mcp-unreal` shows as failed after the editor is up, reconnect it with `/mcp` in the prompt; the tools load once the connection succeeds.
- Confirm the bridge is up by requesting its root over HTTP — any response (including 404) means it is ready.
- While the client is down, the bridge can be driven directly: POST `{"script": "..."}` to `/api/editor/execute_script` on the bound port.
- When building that JSON in Windows PowerShell 5.1, cast the script to `[string]` before `ConvertTo-Json` — a `Get-Content -Raw` result otherwise serializes as an object and the route rejects the body.
- Script `print`/stdout does not surface in the response; write results to a file under `Saved/` and read it back.

---

## Inspecting a specific PIE world (server vs client)

A listen-server PIE runs multiple game worlds; the actor-listing tools return only one of them. To diagnose host-only or client-only behaviour, read the actor and its component state from the exact world.

- `get_game_world()` from the editor subsystem returns the server (host) world; client worlds carry a higher PIE index in their path.
- Enumerate actors in a chosen world with `get_all_actors_of_class`, then read live component and widget state per actor to compare host against client.
- The script API has no `find_objects` and no `local_role` editor property; go through the world's actor list and read exposed properties (widget space, hidden flag, the resolved user widget) instead.
- Return results by writing them to a file under `Saved/` and reading it back — script stdout and `log` calls do not surface through the output-log query.
