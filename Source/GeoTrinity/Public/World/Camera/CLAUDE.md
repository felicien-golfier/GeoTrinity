# World/Camera

Follow camera framed by placed volumes; orthographic by default, perspective on a console switch. Framing is a pure function of location: no boss, arena or match state.

| File | Role |
|---|---|
| `GeoGameCamera.h` | Follow + zoom + bounds + spectate + outline post-process + projection switch |
| `GeoCameraVolume.h` | `TriggerBox` (where the local player stands) + `BoundsBox` (what the camera may show), `OrthoWidth`, `bLimitView` |
| `GeoCurveCameraShake.h` | Shake whose envelope is one curve, sampled from shake start (montage notify timeline) |

## Camera
- Follows the midpoint of every living local player with exponential smoothing (`FollowInterpSpeed` 2–8).
- Zooms `OrthoWidth` out (`ZoomMargin`, `MaxOrthoWidth`, `ZoomInterpSpeed`) along `ScreenRight` / `ScreenUp` until all fit; never below `BaseOrthoWidth`. No split screen.
- Inputs are only `EnterVolume` / `ExitVolume`; `ActiveVolumes` is a refcount (one entry per local player); newest wins; `RefreshBounds()` once per change via `GeoLib::GetTargetPoints` (`TargetPoint.CameraBounds` corners sharing the volume's `Arena.*` tag).
- Outside every volume it follows freely.
- Owns the deployable outline: `OutlineMaterial` blendable installed in `BeginPlay`, fed the palette texture (`Tool/Visual/CLAUDE.md`); skipped on dedicated server.
- Spectates only when every local player is dead (freezes at death position, pans after `SpectateDelayRemaining` seeded from `AGeoGameState::DeathTime`).

## Projection
- `Geo.CameraPerspective` (CVar, 0 = ortho default, 1 = perspective): local view only, nothing replicates. Read at the top of `Tick`, before the no-local-player early return; a change calls `ApplyProjection()` once.
- One zoom unit in both modes: `CurrentOrthoWidth` = world width seen at the floor. Volumes, settings range, wheel, coop spread, `ClampToVolume`, MPC `CameraZoomRatio` never branch on the mode.
- Perspective turns that width into a distance: `GetPerspectiveDistance(Width, PerspectiveFOV)` = `Width / (2·tan(FOV/2))` (static, pure). `PerspectiveFOV` default 30° (close to the ortho look).
- The actor never moves for it: the camera component slides back along its own view axis by `ViewDistance − OrthoFloorDistance` (`OrthoFloorDistance` = actor height / −Forward.Z, captured in `BeginPlay`), so the floor point at screen centre is the same in both modes and follow / clamp / MPC `CameraXY` stay actor-XY. Ortho: offset 0, `FieldOfView` unused.
- Every perspective zoom change also calls `Backdrop->ApplyViewOffset(Offset, PerspectiveFOV)` (`../Backdrop/CLAUDE.md`); ortho calls it with 0.

## Volume
- Purely local, nothing replicates: overlap gated on `GeoLib::IsLocalPlayerAvatar`.
- One per room, tagged with the room's `Arena.*` tag; leave corridors uncovered.
