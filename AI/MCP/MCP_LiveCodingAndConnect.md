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
