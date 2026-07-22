# World

Level-specific actors.

## `GeoCameraVolume.h`
Box trigger selecting which camera bounds apply while the **local player** stands inside it. Carries one `ArenaTag`; hands it to `AGeoGameCamera` on overlap — camera frames the `TargetPoint.CameraBounds` corner points sharing that tag. **Framing is a pure function of location** — no boss/match-state involvement; walking into a room frames you whether or not a fight runs. One volume per room, tagged with that room's `Arena.*` tag; leave corridors/hub uncovered for free camera follow.
- **Purely local, nothing replicates** — overlap gated on `GeoLib::IsLocalPlayerAvatar`, calls `Camera->EnterVolume/ExitVolume` directly. No server round-trip. (Replaces the old replicated `ActiveArena`-driven selection.)
- `GetArenaTag()` is the whole public surface.
- Volumes may overlap — camera keeps a stack, **most recently entered wins** (`GetActiveVolume` walks newest-first, dropping destroyed ones).

## `GeoGameCamera.h`
Orthographic follow camera. **Always follows** the local player with exponential smoothing (`FollowInterpSpeed`, default 5) — no dead zone. Bounds come from whichever `AGeoCameraVolume` the local player is inside; outside every volume it follows freely. No match-state/arena input at all.
- **`EnterVolume`/`ExitVolume` are the only inputs.** Holds `ActiveVolumes` stack; each change runs `RefreshBounds()` (once per volume change, not per tick) via `GeoLib::GetTargetPoints`. Null active volume (or no corner points) → `bBounded=false`, `Tick` skips clamp.
- **No `GameState` binding, no `BeginPlay` wiring** — don't reintroduce a GameState lookup; framing changes are a volume-placement question, not code.
- `FollowInterpSpeed` range 2–8 typical.
- Follows **local player only**, not a centroid. Dead → free spectator camera panning `SpectateTarget` via raw Enhanced Input read (pawn's input component is disabled). Reverts to following on revive.

## `GeoWorldSettings.h`
Custom `AWorldSettings` subclass for GeoTrinity levels.
