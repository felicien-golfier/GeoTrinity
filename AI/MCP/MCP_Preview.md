# MCP Preview — Judging a Visual Change Without PIE

Placing an effect in the editor world, or feeding a material what gameplay would, and reading the level viewport
back, for work with no reason to start a session. For capturing a running session see `MCP_PIE.md`.

## Where to judge

An asset editor's own preview plays its timeline once and stops, so a looping effect stands frozen there and two
captures a second apart come back identical. Judge in a level viewport, which ticks for as long as the editor
does. A preview on a mesh also needs a real actor: an asset editor preview has no attach parent, so anything
sampling the owner's mesh resolves to nothing.

Spawn a system from the asset the way a drag into the level does — setting the asset on an already registered
component leaves its renderers uninitialised in the editor world. Place previews well away from level geometry,
so an empty frame means an empty effect rather than something lost against the scene. Spawning and destroying
preview actors leaves the map dirty whatever else happens. While PIE runs there is no editor world to spawn into:
it reads as null and every spawn returns nothing, so check the play state first.

## Reading the frame

Establish the frame's scale before concluding anything is missing: place two previews a known distance apart and
measure their separation in pixels. An effect an order of magnitude smaller than expected is indistinguishable
from one that is absent, and the level's own geometry is no guide because it sits at a different depth.

Spawn a known-good asset of the same kind beside the new one as a control — a blank frame then separates a
broken asset from a wrong camera, a wrong scale or a stale capture. A camera read-back confirms the camera and
nothing else: it says where the frame was taken from, never that what you are looking for is in it.

## Effects that need motion

An effect whose shape comes from its owner moving shows nothing while the owner stands still, which reads
exactly like a broken one. Step the preview actor and capture after. Each step has to be its own call — a loop
inside one script holds the game thread, so the world never ticks between iterations and no motion is simulated.
Keep the total travel inside the frame the camera covers.

## Materials that gameplay drives

A parameter collection's values can be written in the editor world through the material library's collection
setters. A material reading them then renders in the level viewport as it would in a session. The values are
saved nowhere, so write the neutral ones back when done. A material moved by the Time node animates in a realtime
level viewport; two captures a few seconds apart show its motion only if the camera stayed put between them.

## A rebuilt asset can look different

Rebuilding a mesh or material changes what the engine derives from it (normals, tangents, bounds), and a material may have leaned on the old values. Capture the asset in the level that uses it, before and after, not on a bare preview mesh.

## Sharing the viewport

The level viewport is also the user's. Remember its camera before a preview moves it, and restore it afterwards. A
camera that moves between two reads without a script moving it means the user is navigating: stop moving it,
and leave it where they put it.

## Telling dead from invisible

Particle counts separate "not simulating" from "simulating but not drawing", and the two have nothing in common
as causes. The Niagara debug HUD's overview lists every live system with its emitter and particle counts; enable
it with a console command filtered to a system name prefix, and disable it when done.

| Task | Script |
|---|---|
| Place a row of systems, optionally on meshes, focus one, step them, show particle counts | `AI/Python/Runtime/vfx_editor_preview.py` |
| Remember and restore the camera, write collection values, keep rendering while unfocused | `AI/Python/Runtime/vfx_editor_preview.py` |
| Compare candidate layers of a material's stack by instruction count | `AI/Python/Material/compare_material_layers.py` |

## Driving the editor window

Only when the user asked for a test done by hand (root `CLAUDE.md`: no mouse or focus otherwise).

- Window grab: enumerate top-level windows, match the editor by title (asset editors dock as tabs, so the main window title), `CopyFromScreen` its rect. Slate is captured too (Niagara stack, tooltips, Details, debug HUD); the viewport-capture tool times out with the UI flag outside a game viewport.
- Level viewport only: the viewport-capture tool without the UI flag is cheaper, but the editor stops rendering unfocused, so every frame is stale. `Slate.bAllowThrottling 0` over the bridge plus `bThrottleCPUWhenNotForeground` on `/Script/UnrealEd.Default__EditorPerformanceSettings` (reachable only by that path) makes it live; restore both afterwards. Particles simulate either way.
- Input: `SetCursorPos` + `mouse_event` + `SendKeys` for what Python cannot reach (a mesh LOD's Allow CPUAccess). Every helper calls `SetProcessDpiAwarenessContext(-4)` first or clicks land off on a secondary monitor.
- Foreground refused: `PostMessage` `WM_MOUSEMOVE` / `WM_LBUTTONDOWN` / `WM_LBUTTONUP` with client coordinates straight to the dialog's HWND.
- A modal dialog blocks the game thread: script calls time out until it is dismissed.
- Two editors on one project: each holds a loader handle on every package it loaded; the other cannot save them until the first unloads (`EditorLoadingAndSavingUtils.unload_packages`).
- Helpers live in the session scratchpad.
