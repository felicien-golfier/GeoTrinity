# World

Level-specific actors.

## `GeoCameraVolume.h`
Box trigger selecting which camera bounds apply while the **local player** stands inside it. Carries one `ArenaTag`; hands it to `AGeoGameCamera` on overlap — camera frames the `TargetPoint.CameraBounds` corner points sharing that tag. **Framing is a pure function of location** — no boss/match-state involvement; walking into a room frames you whether or not a fight runs. One volume per room, tagged with that room's `Arena.*` tag; leave corridors/hub uncovered for free camera follow.
- **Purely local, nothing replicates** — overlap gated on `GeoLib::IsLocalPlayerAvatar`, calls `Camera->EnterVolume/ExitVolume` directly. No server round-trip. (Replaces the old replicated `ActiveArena`-driven selection.)
- `GetArenaTag()` is the whole public surface.
- Volumes may overlap — camera keeps a stack, **most recently entered wins** (`GetActiveVolume` walks newest-first, dropping destroyed ones).

## `GeoGameCamera.h`
Orthographic follow camera. **Always follows** the living local players with exponential smoothing (`FollowInterpSpeed`, default 5) — no dead zone. Bounds come from whichever `AGeoCameraVolume` a local player is inside; outside every volume it follows freely. No match-state/arena input at all.
- **`EnterVolume`/`ExitVolume` are the only inputs.** Holds `ActiveVolumes`; each change runs `RefreshBounds()` (once per volume change, not per tick) via `GeoLib::GetTargetPoints`. Null active volume (or no corner points) → `bBounded=false`, `Tick` skips clamp.
- `ActiveVolumes` is a **refcount, not a set** (`Add`/`RemoveSingle`): both couch-coop players pass `IsLocalPlayerAvatar`, so a volume gets one entry per player inside it and survives one of them walking out. `GetActiveVolume` still walks newest-first, dropping destroyed entries.
- **No `GameState` binding** — don't reintroduce a GameState lookup; framing changes are a volume-placement question, not code. `BeginPlay` exists only to capture the authored `OrthoWidth` as `BaseOrthoWidth`.
- `FollowInterpSpeed` range 2–8 typical.
- Follows the **midpoint of every living local player**, and zooms `OrthoWidth` out (`ZoomMargin`/`MaxOrthoWidth`/`ZoomInterpSpeed`) until the outermost fits — measured along the camera's own `ScreenRight`/`ScreenUp`, so it stays right at any yaw. Never below `BaseOrthoWidth`, so solo play is unchanged. In a small arena the bounds clamp then snaps to `Bounds.GetCenter()`, which reads as a fixed room camera. There is **no split screen** — `UGeoGameViewportClient` force-disables it.
- **Owns the deployable outline post-process.** `OutlineMaterial` (MI_DeployableOutline) is installed as a blendable in `BeginPlay`, not authored on the camera component, because it only renders correctly once fed the palette lookup texture — see `Tool/CLAUDE.md` for the palette. Skipped on a dedicated server.
- Spectates only when **every** local player is dead: `SpectateTarget` freezes at the death position and only starts panning (raw Enhanced Input read from the first local controller, since the pawns' input components are disabled) once `SpectateDelayRemaining` — seeded from `AGeoGameState::DeathTime` on the death transition — counts down to zero. One survivor keeps the camera following normally.

## `GeoWorldSettings.h`
Custom `AWorldSettings` subclass for GeoTrinity levels. Header-only — `StartingClassOverride` is its whole surface.
