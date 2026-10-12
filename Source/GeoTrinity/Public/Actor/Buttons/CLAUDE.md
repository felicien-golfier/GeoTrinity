# Actor/Buttons

Overlap pads and triggers. Overlap fires on every machine; only the server runs the effect.

| File | Role |
|---|---|
| `GeoFloorButton.h` | Base pad: server `Press`, label text on the floor |
| `GeoMatchStateButton.h` | Requests a match-state transition from `AGeoGameMode`; `RespawnBosses` calls `AGeoArena::RespawnAllBosses` instead |
| `GeoDifficultyButton.h` | Sets run difficulty; one pad per value, active one wears `ActiveColor`; label written by `OnConstruction` |
| `GeoTeleporter.h` | Moves the touching player (and couch-coop partners) to the next same-`TeleportTag` pad; inert while a match runs |
| `GeoClassChangeTrigger.h` | Volume switching the player to `TargetClass` |

## Teleporter
- Pads sorted by name, wrapping per tag; pads rotate once in `BeginPlay` to face their destination.
- Server-only move (`SetActorLocation`, XY); clear `CurrentRootMotion` before `TeleportTo`.
- `PendingArrivals` (both machines, deterministic) absorbs repeat begin-overlaps until end overlap.
- Camera reframes by itself through the destination `AGeoCameraVolume`.
