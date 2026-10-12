# World/Camera

Orthographic follow camera framed by placed volumes. Framing is a pure function of location: no boss, arena or match state.

| File | Role |
|---|---|
| `GeoGameCamera.h` | Follow + zoom + bounds + spectate + outline post-process |
| `GeoCameraVolume.h` | `TriggerBox` (where the local player stands) + `BoundsBox` (what the camera may show), `OrthoWidth`, `bLimitView` |
| `GeoCurveCameraShake.h` | Shake whose envelope is one curve, sampled from shake start (montage notify timeline) |

## Camera
- Follows the midpoint of every living local player with exponential smoothing (`FollowInterpSpeed` 2–8).
- Zooms `OrthoWidth` out (`ZoomMargin`, `MaxOrthoWidth`, `ZoomInterpSpeed`) along `ScreenRight` / `ScreenUp` until all fit; never below `BaseOrthoWidth`. No split screen.
- Inputs are only `EnterVolume` / `ExitVolume`; `ActiveVolumes` is a refcount (one entry per local player); newest wins; `RefreshBounds()` once per change via `GeoLib::GetTargetPoints` (`TargetPoint.CameraBounds` corners sharing the volume's `Arena.*` tag).
- Outside every volume it follows freely.
- Owns the deployable outline: `OutlineMaterial` blendable installed in `BeginPlay`, fed the palette texture (`Tool/Visual/CLAUDE.md`); skipped on dedicated server.
- Spectates only when every local player is dead (freezes at death position, pans after `SpectateDelayRemaining` seeded from `AGeoGameState::DeathTime`).

## Volume
- Purely local, nothing replicates: overlap gated on `GeoLib::IsLocalPlayerAvatar`.
- One per room, tagged with the room's `Arena.*` tag; leave corridors uncovered.
