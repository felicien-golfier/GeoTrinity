# MCP PIE — Reading the Running Game

Observing and inspecting a Play-In-Editor session with no human at the keyboard. For judging a visual change
without a session see `MCP_Preview.md`; for reading one specific world's actors see `MCP_LiveCodingAndConnect.md`.

## Starting and stopping

Start, stop and check a session with the PIE-control tool. Start is asynchronous — poll status until it reports
active before doing anything else. A session is shared editor state, so starting or stopping one disrupts a
session a human already has running: check status first, and stop sessions you started.

## Capturing the screen

Capture the viewport as a PNG with the viewport-capture tool, setting the UI flag to include Slate/UMG overlays,
which requires an active session. UI capture is an async screenshot request, so the overlay may lag the world by
one frame. Read the saved PNG back to view it; to inspect a small element, crop and upscale with
nearest-neighbour first — image manipulation runs locally, not through MCP.

Only a session renders continuously. The level viewport draws while its tab is in front, so a capture taken with
any other editor window focused returns the frame it last drew, and a camera move reads back as applied while
the picture never changes.

## Targeting the right world

Editor automation tools take a world argument: auto (session if active, else editor), the running session, or
always the editor. A multiplayer session has one world per player instance, each with its own player, HUD and
actors, and the default game-world accessor returns only one — actors owned by another instance are invisible
there. Load a specific session world by its path, whose map name carries an instance-numbered prefix, then
enumerate actors within it.

## Inspecting live actors and widgets

Find actors in a chosen world by class with the gameplay-statics query, matching on the actual runtime class
name — a Blueprint class, not the C++ base. Call a function on a live actor by name (the engine snake_cases it),
passing arguments as a tuple; a function with several output parameters returns them as a tuple.

Find live widget instances by iterating all objects and filtering on the generated class name, a transient path
and a valid world. UI can then be driven without input simulation: invoke a reflected function, a private click
handler included, on a live widget with the by-name method caller, and write into its child controls through
their property setters — this triggers the same logic as a real click or keystroke. To enter text, read the
input widget off the parent's bound-widget property and call its text setter with the value wrapped in the
engine text type; plain strings are refused. See `AI/Python/Runtime/pie_drive_menu_ui.py`.

## Simulating player input

Inject an Input Action value through the Enhanced Input local-player subsystem. An injection lasts one frame, so
holding an input means re-injecting every frame from a Slate post-tick callback. Object iteration also returns
subsystem instances surviving from earlier sessions, so inject into every live instance rather than the first
found.

Injection enters below the viewport input gate, so it exercises the binding and ability pipeline even when the
current input mode blocks real device input — comparing injected against real input localises where input dies.
Verify the effect from world state (pawn location delta, actor counts) and from the ability-system log, whose
verbosity the log console command can raise to make every activation and cooldown rejection readable. The pawn
comes from the controller's controlled-pawn getter; the plain pawn getter is not exposed. See
`AI/Python/Runtime/pie_inject_input.py`.
