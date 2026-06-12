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

Find live widget instances by iterating all objects, filtering on the generated class name, a transient path, and a valid world.

Drive UI without input simulation: invoke a reflected function (including a private click handler) on a live widget with the by-name method caller, and write into its child controls through their property setters — this triggers the same logic as a real click or keystroke.

To enter text, read the input widget off the parent's bound-widget property, then call its text setter with the value wrapped in the engine text type — plain strings are not accepted. See `AI/Python/pie_drive_menu_ui.py`.

---

## Simulating Player Input

Inject an Input Action value through the Enhanced Input local-player subsystem; an injection lasts one frame, so re-inject every frame from a Slate post-tick callback to hold an input.

Injection enters below the viewport input gate, so it exercises the binding and ability pipeline even when the current input mode blocks real device input — comparing injected against real input localises where input dies.

Object iteration also returns subsystem instances surviving from earlier PIE sessions; inject into every live instance rather than the first found.

Verify the effect from world state (pawn location delta, actor counts) and from the ability-system log: raising that log category's verbosity with the log console command makes every activation and cooldown rejection readable through the log tool.

The pawn comes from the controller's controlled-pawn getter; the plain pawn getter is not exposed.

See `AI/Python/pie_inject_input.py` for the phase-driven probe.

---

## Script Output

The script-execution tool does not return a script's printed output. Write results to a file from the script and read that file back, or emit to the log and read it through the log tool.

The tool also reports success when the script raises; the traceback lands in the Python log category, so after any run grep that category (or a marker you logged) to confirm the script actually completed.
