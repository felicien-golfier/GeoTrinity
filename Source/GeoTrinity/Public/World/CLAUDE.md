# World

| Folder / file | Holds |
|---|---|
| `Camera/` | `GeoGameCamera`, `GeoCameraVolume`, `GeoCurveCameraShake` |
| `Backdrop/` | `GeoBackdropComponent`, `GeoBackgroundPulseComponent` |
| `GeoWorldSettings.h` | `AWorldSettings` subclass; `StartingClassOverride`, `LockedAbilityTags`, `KillZ` default |

## GeoWorldSettings
- `KillZ` default (constructor) = `GeoKillZ` (−50), just under the floor at Z = 0; nothing in the game is ever under 0: a character off the floor dies within a fraction of a second (orthographic top view shows no fall). Death path: `Characters/CLAUDE.md` ("Void").
- `LockedAbilityTags`: class abilities this map does not grant at startup (tutorial map: the Circle's basic attack, deployable, Special); an encounter grants them later (`GeoUnlockAbilityAction`). Read only by `UGeoAbilitySystemComponent::GiveStartupAbilities(EPlayerClass)`; a class change in such a map keeps them locked.
