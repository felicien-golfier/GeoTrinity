# World

Level-specific actors.

## `GeoGameCamera.h`
Orthographic follow camera for GeoTrinity.

**Always follows** the local player with exponential smoothing (`FollowInterpSpeed`, default 5) — no edge-trigger dead zone.
World-space movement bounds are computed from `AGeoTargetPoint` actors tagged `Camera.Bounds` for the current match state.
Camera decelerates naturally near borders (clamped target shrinks the interp gap). Z is fixed to spawn height.

- Bounds recalculate on `CommitFightDelegate` (fight start) and on `OnMatchStateChanged` for any state except the InProgress transition (CommitFight handles that)
- `SetBoundsTag(FGameplayTag)` (public) — recomputes bounds from the `AGeoTargetPoint`s carrying an arbitrary tag; used by `AGeoTeleporter` to reframe the camera on zone changes. No point matches → previous bounds kept. Match-state transitions later recompute from their own Intro/Fight tags
- `FollowInterpSpeed` — exponential follow speed; higher = snappier. Range 2–8 typical

Follows the **local player only** — not a centroid of all players.
When the local player is dead, switches to a free spectator camera: the move input pans `SpectateTarget`
(`SpectateMoveSpeed`, units/s) clamped to the same bounds. The dead pawn's input component is disabled, so the move
value is read straight from the Enhanced Input player subsystem (`GetActionValue(MoveAction)`), not from the pawn.
Reverts to following the pawn on revive.

## `GeoWorldSettings.h`
Custom `AWorldSettings` subclass for GeoTrinity levels. Configure per-level settings here.
