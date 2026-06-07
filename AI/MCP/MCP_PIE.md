# MCP PIE — Reading the Running Game

Observing and inspecting a Play-In-Editor session via MCP, without a human at the keyboard.

---

## Starting and Stopping

Start, stop, and check a session with the PIE-control tool. Start is asynchronous — the session begins on the next frame, so poll status until it reports active before doing anything else.

A PIE session is shared editor state: starting or stopping one disrupts a session a human already has running. Check status first, and stop sessions you started when finished.

---

## Capturing the Screen

Capture the viewport as a PNG with the viewport-capture tool; set the UI flag to include Slate/UMG overlays (HUD, menus), which requires an active session.

UI capture uses an async screenshot request, so the overlay may lag the world by one frame.

Read the saved PNG back to view it. To inspect a small on-screen element, crop the region and upscale with nearest-neighbour before reading — image manipulation runs locally, not through MCP.

---

## Targeting the Right World

Editor automation tools take a world argument: auto (session if active, else editor), the running session, or always the editor.

A multiplayer session has more than one world (one per player instance), each with its own player, HUD, and actors. The default game-world accessor returns only one of them, so actors owned by another instance are invisible there.

Load a specific session world by its path — session world paths carry an instance-numbered prefix on the map name — then enumerate actors within it.

---

## Inspecting Live Actors and Widgets

Find actors in a chosen world by class with the gameplay-statics actor query; match on the actual runtime class name (a Blueprint class, not the C++ base).

Call a function on a live actor by its name; the engine snake_cases it. Pass arguments as a tuple, not a list. A function with multiple output parameters returns them as a tuple.

---

## Script Output

The script-execution tool does not return a script's printed output. Write results to a file from the script and read that file back, or emit to the log and read it through the log tool.
