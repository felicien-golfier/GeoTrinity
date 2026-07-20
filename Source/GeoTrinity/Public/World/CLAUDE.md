# World

Level-specific actors.

## `GeoGameCamera.h`
Orthographic follow camera for GeoTrinity.

**Always follows** the local player with exponential smoothing (`FollowInterpSpeed`, default 5) — no edge-trigger dead zone.
World-space movement bounds are computed from the `AGeoTargetPoint` actors tagged `TargetPoint.CameraBounds` for the arena currently being played. They only **constrain** the camera during a boss fight (`MatchState` InProgress); outside a fight the camera follows the local player freely, so a post-wipe teleport back to the entrance (which sits at the arena's edge, outside those bounds) isn't left off-screen.
Camera decelerates naturally near borders (clamped target shrinks the interp gap). Z is fixed to spawn height.

- **One trigger for *which* bounds: `GeoGameState::OnActiveArenaChanged`.** The bounds are a pure function of `GetActiveArenaTag()` — the arena the players are *in*, never the match state — so the camera has exactly one binding and `CalculateBounds` has no branches. Match state, fight commit and teleport pads all reach the camera through that one signal, because each of them either changes `ActiveArena` or does not concern the framing. Don't reintroduce a `MatchState` test into *which arena to frame*, or a per-event binding here: a fight reframes because aggro moved the arena, a teleport reframes because the pad moved it. (`Tick` does read `MatchState`, but for a different axis — *whether* to clamp to those bounds at all, see the next bullet — not for choosing them.)
- **The clamp itself is gated on being in a fight.** `Tick` clamps the follow target to the bounds only when `GameState->IsMatchInProgress()`; otherwise the target is the pawn position and the camera follows freely. This is what makes it safe that `ActiveArena` is never cleared on a wipe: the stale boss-arena bounds simply stop applying the instant the match leaves InProgress, so the camera trails the revived players out to the entrance instead of staying pinned to the arena. Spectating (dead, free-pan) only occurs mid-fight, so its pan is bounded by the same gate.
- Because the signal is the replicated `ActiveArena` (broadcast from `OnRep_ActiveArena`, and manually on the server), clients reframe too — the old path had `AGeoTeleporter` calling the camera locally, which left a client framed on the arena it had just *left* when the arrival overlap re-applied the destination pad's tag. Cost of the fix: a client's reframe now waits one replication hop
- `SetArenaTag(FGameplayTag)` is private — recomputes bounds from that arena's `TargetPoint.CameraBounds` points; no point matches → previous bounds kept
- Corner points are shared where arenas overlap: `UP_LEFT`/`UP_RIGHT` carry both `Arena.Entrance` and `Arena.Main`, so the hub and the first arena share their upper edge
- `FollowInterpSpeed` — exponential follow speed; higher = snappier. Range 2–8 typical

Follows the **local player only** — not a centroid of all players.
When the local player is dead, switches to a free spectator camera: the move input pans `SpectateTarget`
(`SpectateMoveSpeed`, units/s) clamped to the same bounds. The dead pawn's input component is disabled, so the move
value is read straight from the Enhanced Input player subsystem (`GetActionValue(MoveAction)`), not from the pawn.
Reverts to following the pawn on revive.

## `GeoWorldSettings.h`
Custom `AWorldSettings` subclass for GeoTrinity levels. Configure per-level settings here.
