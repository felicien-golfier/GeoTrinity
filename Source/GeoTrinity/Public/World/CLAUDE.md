# World

Level-specific actors.

## `GeoGameCamera.h`
Orthographic follow camera for GeoTrinity.

**Always follows** the local player with exponential smoothing (`FollowInterpSpeed`, default 5) — no edge-trigger dead zone.
World-space movement bounds are computed from the `AGeoTargetPoint` actors tagged `TargetPoint.CameraBounds` for the arena currently being played.
Camera decelerates naturally near borders (clamped target shrinks the interp gap). Z is fixed to spawn height.

- **One trigger: `GeoGameState::OnActiveArenaChanged`.** The bounds are a pure function of `GetActiveArenaTag()` — the arena the players are *in*, never the match state — so the camera has exactly one binding and `CalculateBounds` has no branches. Match state, fight commit and teleport pads all reach the camera through that one signal, because each of them either changes `ActiveArena` or does not concern the framing. Don't reintroduce a `MatchState` test or a per-event binding here: a fight reframes because aggro moved the arena, a teleport reframes because the pad moved it
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
