# MCP GIMP

Image editing through the `gimp-mcp` server, which drives a running GIMP 3 over a plugin socket.

## Connecting

- GIMP must be running with its MCP plugin server started (Tools > Start MCP Server); it listens on `localhost:9877`.
- The server check reports the connection and GIMP version — run it before any other call.
- GIMP may hold unsaved work: ask the user to restart it rather than doing it yourself.

## Request model

- The plugin runs every request on its own socket thread with no lock, and GIMP's API is not thread-safe: two
  requests at once kill the plugin. Send GIMP calls strictly one at a time — never several in one parallel batch.
- The server abandons a request after 10 s, but GIMP keeps executing it; a timeout means GIMP is still busy, and the
  next call must wait until the GIMP process stops using CPU.
- A killed plugin leaves GIMP running with the port closed; only the menu command brings it back.
- Work that can pass 10 s — any export, a preview of a print-size image, a long drawing script — goes through the
  exec tool as a function scheduled with `GLib.idle_add`: it runs on GIMP's main thread, the call returns at once, and
  the function writes its result or error to a file the caller waits on. Send nothing else to GIMP until that file
  exists.

## Tool model

- Tools address an image by its index in the open-image list (default 0), not by its image id.
- Layer tools address a layer by name and default to the active layer.
- Dedicated tools cover single operations — canvas, fill, shape, filter, export; anything composite goes through the API exec tool.

## Exec console

- The exec tool runs a list of Python strings in GIMP's PyGObject console; the context persists across calls, so imports, helpers and variables defined once stay available.
- A multi-line block (a function, a loop) is one string with embedded newlines.
- Output returns only through `print`; a statement with no output returns an empty string.
- A statement that raises stops the rest of its list and returns only the error, so an undo group or context push
  opened earlier in that list stays open until a later call closes it.
- The console has one namespace: a loop variable or assignment reusing a helper's name replaces the helper.
- A long script lives in a scratch `.py` file run with `exec(open(path).read())`, so the call carries one line.
- Build colours from hex strings or CSS names — an `rgb()` string with 0–255 values is stored unnormalised and clamps every non-zero channel to full.
- Draw a shape by selecting a polygon from a flat coordinate list, then filling or stroking the selection; stroke width is the context line width. Clear the selection after.
- Antialiasing is a context setting — enable it once before drawing.
- A new layer is created, then inserted; insert position 0 is the top of the stack.
- A layer named like an existing group or layer gets a `#1` suffix.
- A glow is a layer in addition mode, blurred by a `gegl:gaussian-blur` drawable filter merged into that layer.
- Flush displays after drawing so the GIMP window updates.

## Verifying

- Tool results never show the image — fetch the image bitmap to judge it, and a second one scaled to the display size to check readability.
- The bitmap and snapshot tools duplicate, scale and export inside the request; on a small image that is fast, on a
  print-size layered image it passes the time limit. There, a main-loop job duplicates the image, flattens it, crops
  or scales the copy, exports it to a scratch PNG and deletes the copy; the PNG is then read from disk.
- Their region mode copies only the top layer and replaces the image's selection, so a group on top returns black —
  crop a flattened duplicate instead.
- The bitmap tool takes no image index; with several images open, use the state snapshot, which does.
- Sample a pixel on a named layer to tell a wrong layer value from a compositing effect.

## Keeping the result

- Every edit changes the open image only — export to keep it.
- Export flattens the image by default; turn flattening off to keep its layers editable.
- Saving an image under several formats is one main-loop job writing each file in turn, never one request.
- An exported file dropped under `Content/` still needs an Unreal import to become a texture asset.
