# Encounter/Action

Stock actions (base: `../GeoEncounterAction.h`). Each is a C++ class and a base for Blueprint children. They run on the server (`../CLAUDE.md`, "Invariants").

| File | Does | Latent |
|---|---|---|
| `DelayAction.h` | Waits `Duration` s | yes |
| `RaiseEventAction.h` | `UGeoEncounterSubsystem::Raise(Tag, Payload)` | no |
| `StartEncounterAction.h` | Starts (or restarts) another `AGeoEncounter` | no |
| `SetBarrierClosedAction.h` | `SetClosed(bool)` on the referenced `AGeoArenaBarrier` (gate between rooms) | no |
| `TeleportPlayersAction.h` | Moves every player to a `GeoTargetPoint` (tag + arena tag, `GeoLib::GetTargetPoints`) | no |

- An action does one thing and reads what it needs from the payload or its own properties; no action knows the step it sits in.
- A new gameplay verb (lock an ability, grant loot, open a menu page, say a Cat line) is an action in this folder, written with the feature that needs it, with its own doc row here.
