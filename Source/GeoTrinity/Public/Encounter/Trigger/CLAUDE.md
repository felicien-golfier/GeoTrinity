# Encounter/Trigger

Stock triggers (base: `../GeoEncounterTrigger.h`). Each is a C++ class and a base for Blueprint children. Payload `Count` is the trigger's own running number since `OnArm`.

| File | Fires when | Payload |
|---|---|---|
| `PlayerEnteredVolumeTrigger.h` | A player overlaps the referenced box | `Instigator` = player, `Count` = entries so far |
| `ActorDiedTrigger.h` | A watched actor (list, or class filter) dies | `Instigator` = the dead, `Count` = deaths so far, `Value` = watched left alive |
| `PlayerDiedTrigger.h` | Any player dies | `Instigator` = player, `Count` = player deaths so far |
| `HealthThresholdTrigger.h` | A watched actor's health ratio crosses a ratio (above or below) | `Instigator` = actor, `Value` = ratio |
| `TimerTrigger.h` | `Delay` s after arming (`bLooping`: every `Delay`) | `Count` = firings so far |
| `EventTrigger.h` | `UGeoEncounterSubsystem` raises its `Tag` | The raised payload, untouched |

- A trigger never filters what a rule can filter: it reports every occurrence with its count; `Condition` decides (`Count == 3`, `Count % 3 == 0`).
- Death and health hooks are the owners' delegates (`APlayableCharacter` / `AEnemyCharacter` / `AGeoGameState`); bound in `OnArm`, removed in `OnDisarm`.
- Tutorial-specific triggers (ability used, chip slotted, chip forged, menu page opened) are Blueprint or C++ children written with their feature.
