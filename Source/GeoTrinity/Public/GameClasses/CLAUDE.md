# GameClasses

Core Unreal game framework classes.

## Files
| File | Role |
|---|---|
| `GeoGameMode.h` | Game rules, player class assignment; `Tick` override |
| `GeoGameState.h` | Replicated game state |
| `GeoGameInstance.h` | Persistent across levels |
| `GeoPlayerController.h` | Player controller |
| `GeoPlayerState.h` | **Hosts ASC + attribute sets for playable characters** |
| `GeoMainMenuGameMode.h` | Menu level GameMode (`AGameModeBase`); no pawn; sets `PlayerControllerClass` to `AGeoMainMenuPlayerController` |
| `GeoMainMenuPlayerController.h` | Menu player controller; creates `UGeoMainMenuWidget` on `BeginPlay`, adds to viewport, sets UI-only input |

## `GeoPlayerState` — Critical for GAS
The `UGeoAbilitySystemComponent` and `UCharacterAttributeSet` live on `AGeoPlayerState`, not on the character. This is the standard GAS multiplayer pattern — ASC survives character respawn.

Key fields:
- `UGeoAbilitySystemComponent* AbilitySystemComponent`
- `UCharacterAttributeSet* CharacterAttributeSet`
- `EPlayerClass PlayerClass` — replicated, `OnRep_PlayerClass` triggers class switch visuals
- Rolling combat stats (replicated): `DebugDPS`, `DebugHPS`, `DebugRecv`, `TotalDamageDealt`, `TotalHealingDealt`, `TotalDamageReceived`

Key flow: `ClientInitialize` → `InitOverlay()` (creates HUD), `OnPlayerPawnSet` delegate → `InitGAS()` on the character.
