# World

Level-specific actors.

## `GeoGameCamera.h`
Orthographic top-down camera (pitch = -90).

**Edge-triggered follow**: stationary until player nears a screen edge, then smoothly follows.

Fields:
- `BoundsMin / BoundsMax` — world-space camera movement limits (`FVector2D`, default ±500)
- `ScreenEdgeThresholdPercent` — fraction of screen half-extent defining trigger zone (default 0.05)
- `FollowSpeedCurve` (`UCurveVector`) — X/Y follow speed driven by edge proximity distance

Follows the **local player only** — not a centroid of all players.

## `GeoWorldSettings.h`
Custom `AWorldSettings` subclass for GeoTrinity levels. Configure per-level settings here.
