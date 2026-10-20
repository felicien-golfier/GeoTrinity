# Actor/Buttons

Overlap pads and triggers. Overlap fires on every machine; only the server runs the effect.

| File | Role |
|---|---|
| `GeoFloorButton.h` | Base pad: server `Press`, label text on the floor |
| `GeoMatchStateButton.h` | Requests a match-state transition from `AGeoGameMode`; `RespawnBosses` calls `AGeoArena::RespawnAllBosses` instead |
| `GeoDifficultyButton.h` | Sets run difficulty; one pad per value, active one wears `ActiveColor`; label written by `OnConstruction` |
| `GeoTeleporter.h` | Moves the touching player (and couch-coop partners) to the next same-`TeleportTag` pad; inert while a match runs or while closed |
| `GeoClassChangeTrigger.h` | Volume switching the player to `TargetClass` |

## Teleporter
- Pads sorted by name, wrapping per tag; pads rotate once in `BeginPlay` to face their destination.
- Server-only move (`SetActorLocation`, XY); clear `CurrentRootMotion` before `TeleportTo`.
- `PendingArrivals` (both machines, deterministic) absorbs repeat begin-overlaps until end overlap.
- Camera reframes by itself through the destination `AGeoCameraVolume`.

### Open / closed
- `bOpen` (`EditAnywhere` = state at level start, default true: lobby pads unchanged). Replicated, `OnRep_Open`; server `SetOpen(bool)`. Pads never destroyed: closed = hidden (after the close animation), no overlap, nothing sent; open = visible, overlap on.
- `SetOpen` and the rep notify run the same local `ApplyOpen` (collision, clear `PendingArrivals`, portal open / close) then call the Blueprint event `OnOpenChanged(bool)` (extra per-pad flourish; the portal itself needs no Blueprint).
- Closed hides the pad only once the close animation ends (`PortalCloseSeconds`, `PortalLife` back at 0), collision is off at once.

### Portal look
- One portal on every pad, identical to the "Teleport · 6 Round" study of the Research artifact (`AI/Artifacts.md`): `PortalComponent` plays `NS_TeleportPortal` (one sprite wearing `M_TeleportPortal`: rim, core, shells, arcs and the 24 sparks drawn analytically). Built from `AI/VFX.md` "Teleport portal".
- One system, three life phases (`AI/VFX.md`, "Life phases"): `Activate` plays INTRO (scale and fade in over 0.5 s), then MAIN holds; a graceful `Deactivate` (never `DeactivateImmediate`) plays OUTRO (scale and fade out over 0.9 s). `PlayPortalTransition` does both; no tick, no timer. A pad that starts closed plays nothing.
- One colour decides everything: `PortalColor` (`EditAnywhere`, `BlueprintReadWrite`, `SetPortalColor`) → `User.Color` (particle colour → material colour); the material derives the hot colour. Written at `OnConstruction`, and `BeginPlay`.
- The system is a default loaded in the constructor; the portal does not show in the editor viewport (the component starts inactive).
- The root mesh only shapes the overlap and is always invisible (`OnConstruction`); the portal is the only visual. The label sits above the portal (`LabelHeight` over `PortalHeight`) so they never z-fight; it follows `bOpen`.
- Reduced motion has no meaning in game; the held frame is the open pad at rest.
- A closed pad is still a valid destination: `FindNextTeleporter` ignores the open state.
- Driven from an encounter by `UGeoSetTeleporterOpenAction` (`../../Encounter/Action/CLAUDE.md`), the way a barrier follows `SetClosed`.
