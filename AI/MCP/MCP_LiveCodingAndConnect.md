# MCP Live Coding & Connection

Building `.cpp` changes while the editor is open, and getting the MCP bridge connected.

## Live Coding

A running editor locks the module DLLs, so a full link fails with `LNK1104: cannot open file ...dll`. The
compile step still validates the code; only the link is blocked, so that error means the editor is holding the
DLLs, not that the code is wrong.

- Implementation-only `.cpp` changes: trigger Live Coding from the editor (`Ctrl+Alt+F11`), which patches the
  running process without unloading DLLs.
- A new `UFUNCTION`, a new type or a header change cannot be hot-patched — ask the user to close the editor,
  then run the full build from `AI/Commands.md`.

## Connecting the bridge

The editor hosts the bridge HTTP server (its startup line names the bound `127.0.0.1` port and route count) and
the `mcp-unreal` client is spawned at Claude Code session start. The editor must already be serving the bridge
when the client connects, or the client registers as failed and its tools never load; reconnect it with `/mcp`
once the editor is up. Confirm the bridge is ready by requesting its root over HTTP — any response, 404
included, means it is.

While the client is down the bridge can be driven directly by POSTing `{"script": "..."}` to
`/api/editor/execute_script` on that port. In Windows PowerShell 5.1, cast the script to `[string]` before
`ConvertTo-Json` — a raw file read otherwise serializes as an object and the route rejects the body. The route
reports success even for a script that raises, and stdout does not surface in the response, so wrap the source
in a handler that writes its traceback to a file under `Saved/` and read that back.

## Inspecting a specific PIE world

A listen-server PIE runs multiple game worlds and the actor-listing tools return only one, so diagnosing
host-only or client-only behaviour means reading state from the exact world. The editor subsystem's game-world
getter returns the server world; client worlds carry a higher PIE index in their path. Enumerate a chosen
world's actors by class and read live component and widget state per actor to compare host against client.

The script API has no object-find helper and no local-role property, so go through the world's actor list and
read exposed properties instead. Return results by writing them to a file under `Saved/` — stdout and log calls
do not surface through the output-log query.
