# MCP GIMP

Image editing through `gimp-mcp`, driving GIMP 3 over a plugin socket (`localhost:9877`).

## Connect
- GIMP running with Tools > Start MCP Server; run the server check first.
- Unsaved work may be open: ask the user to restart it.

## Requests
- One call at a time, never a parallel batch (API not thread-safe; two requests kill the plugin; only the menu brings it back).
- 10 s server timeout while GIMP keeps working: wait for CPU idle before the next call.
- Work that can pass 10 s (export, print-size preview, long drawing): exec tool, function scheduled with `GLib.idle_add`, result or error written to a file the caller waits on.
- Images addressed by index, layers by name (default active).
- Dedicated tools for single operations; composites through the exec console.

## Exec console
- PyGObject, one persistent namespace; a multi-line block is one string; output only via `print`.
- A raising statement stops its list and leaves undo groups open.
- Long scripts: scratch `.py` + `exec(open(path).read())`.
- Colours from hex / CSS names; shapes = select polygon then fill / stroke (stroke width = context line width); enable antialiasing once.
- New layer: create then insert (position 0 = top). Glow = addition-mode layer with a `gegl:gaussian-blur` filter merged.
- Flush displays after drawing.

## Verify and keep
- Tool results never show the image: fetch the bitmap, plus one scaled to display size for readability. Large layered images go through a main-loop job (duplicate, flatten, crop / scale, export scratch PNG, delete copy).
- Region mode copies only the top layer; use a flattened duplicate. Bitmap tool takes no image index: use the state snapshot with several images.
- Edits change the open image only: export (flatten off keeps layers); several formats = one main-loop job.
- A file placed under `Content/` still needs an Unreal import.
