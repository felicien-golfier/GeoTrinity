# World

Level-specific actors.

## `GeoGameCamera.h`
Orthographic follow camera for GeoTrinity.

**Always follows** the local player with exponential smoothing (`FollowInterpSpeed`, default 5) — no edge-trigger dead zone.
World-space movement bounds are computed from `AGeoTargetPoint` actors tagged `Camera.Bounds` for the current match state.
Camera decelerates naturally near borders (clamped target shrinks the interp gap). Z is fixed to spawn height.

- Bounds recalculate on `CommitFightDelegate` (fight start) and on `OnMatchStateChanged` for any state except the InProgress transition (CommitFight handles that)
- `FollowInterpSpeed` — exponential follow speed; higher = snappier. Range 2–8 typical

Follows the **local player only** — not a centroid of all players.

## `GeoWorldSettings.h`
Custom `AWorldSettings` subclass for GeoTrinity levels. Configure per-level settings here.
