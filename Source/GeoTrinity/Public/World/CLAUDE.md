# World

Level-specific actors.

## `GeoCameraVolume.h`
Box trigger that selects **which camera bounds** apply while the **local player** stands inside it. It carries one `ArenaTag` and does nothing but hand it to `AGeoGameCamera` on overlap — the camera then frames the `TargetPoint.CameraBounds` corner points that carry that tag (the same bounds computation as before, now driven by *where you are* instead of by an active arena). **Framing is a pure function of location** — no boss, no match state; walking into a room frames you to it whether or not a fight is running. Place one volume per room you want framed, tagged with that room's `Arena.*` tag; leave corridors/hub uncovered to let the camera follow freely there. **Your existing `TargetPoint.CameraBounds` corner points don't move** — the volume just points at them.

- **Purely local, nothing replicates.** The begin/end overlap fires on `GeoLib::IsLocalPlayerAvatar(OtherActor)` only, and calls `Camera->EnterVolume(this)` / `ExitVolume(this)`. No server round-trip, no `ActiveArena`, no replication hop — a teleport reframes the instant the arriving pawn overlaps the destination room's volume. This replaced the old `ActiveArena`-driven selection, whose whole reason for replicating was to stop a client framing the room it had just left; a local overlap can't have that bug.
- `GetArenaTag()` is the whole public surface — the tag the camera resolves bounds from. The box extent is just the trigger region; the *bounds rectangle* is the corner points, not the box.
- Volumes may overlap. The camera keeps the stack of volumes the player is inside and the **most recently entered** one wins (`AGeoGameCamera::GetActiveVolume` walks the stack newest-first, dropping any that were destroyed).

## `GeoGameCamera.h`
Orthographic follow camera for GeoTrinity.

**Always follows** the local player with exponential smoothing (`FollowInterpSpeed`, default 5) — no edge-trigger dead zone.
Its movement bounds are the `TargetPoint.CameraBounds` corner points of whichever `AGeoCameraVolume` the local player is currently inside; **outside every volume it follows freely** (hub, corridors, a post-wipe teleport to the entrance). There is no match-state or arena input at all any more.
Camera decelerates naturally near borders (clamped target shrinks the interp gap). Z is fixed to spawn height.

- **`EnterVolume`/`ExitVolume` are the only inputs** — called by the volumes on local-player overlap. The camera holds `ActiveVolumes` (a stack of `TWeakObjectPtr<AGeoCameraVolume>`); `GetActiveVolume()` returns the newest still-live one, or null. Each enter/exit runs `RefreshBounds()`, which computes `Bounds` from that volume's tag via `GeoLib::GetTargetPoints(CameraBounds, ArenaTag)` — **once per volume change, not per tick**. When the active volume is null (or its tag has no corner points) `bBounded` is false and `Tick` skips the clamp, so the follow target is the raw pawn (or spectate) position.
- **No `GameState` binding, no `BeginPlay` wiring.** The old camera bound `OnActiveArenaChanged`, retried on a late-replicated GameState, and read `IsMatchInProgress()` to decide whether to clamp. All of that is gone: overlaps drive everything, so a late joiner needs no retry and there is nothing to rebind. Don't reintroduce a GameState lookup here — if framing needs to change, it is a volume placement question, not a code one.
- `SetArenaTag`-style bounds math lives inline in `RefreshBounds`: sum the corner-point locations into an `FBox2D`; an empty match keeps the camera unbounded there and logs (a mis-tagged volume, not a crash).
- `FollowInterpSpeed` — exponential follow speed; higher = snappier. Range 2–8 typical

Follows the **local player only** — not a centroid of all players.
When the local player is dead, switches to a free spectator camera: the move input pans `SpectateTarget`
(`SpectateMoveSpeed`, units/s), clamped to the active volume when inside one. The dead pawn's input component is disabled, so the move
value is read straight from the Enhanced Input player subsystem (`GetActionValue(MoveAction)`), not from the pawn.
Reverts to following the pawn on revive.

## `GeoWorldSettings.h`
Custom `AWorldSettings` subclass for GeoTrinity levels. Configure per-level settings here.
