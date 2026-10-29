# Tool

| Folder / file | Holds |
|---|---|
| `UGeoGameplayLibrary.h` | `GeoLib` Blueprint-callable helpers |
| `GeoAssetManager.h` | Custom asset manager (configured in `DefaultEngine.ini`) |
| `Team.h` | `ETeam` for `IGenericTeamAgentInterface` |
| `GeoDifficulty.h` | Run difficulty enum |
| `Visual/` | `GeoColor`, `GeoIcon`, `GeoNiagaraParams` |
| `Net/` | `GeoHazardJudge`, `GeoNetcodeDebug` |
| `CatLine/` | The Cat's random lines read from `Data/CatLines.txt` → `CatLine/CLAUDE.md` |

## GeoLib
- `IsServer(...)`: dedicated and listen; use over `HasAuthority()`.
- `IsDedicatedServer(...)`: gate cosmetic-only work (listen host renders).
- `IsLocalPlayerAvatar(Pawn|Actor)`: the viewing human's own avatar; true for every couch-coop local player.
- `IsKeyboardMousePlayer(PC)`: local player 0; gate anything reading the mouse.
- `GetServerTime(...)`: network-approximated; never for local client timing (use `GetWorld()->GetTimeSeconds()`).
- `GetPerceivedServerTime(Actor)`: target's own time for lag compensation (remote player = last processed move stamp, clamped to `MaxLatencyCompensation`).
- `GetTargetPoints(World, PurposeTag, ArenaTag)`, `GetAlivePlayers(World)`, `TeleportPlayersToTargetPoints(..., ExemptZoneName)`, `TriggerCameraShake` (local), `GetColorForObject`, `ArbitraryCharacterZ = 50`.
- `IntersectRayWithZPlane(Origin, Direction, Z, OutXY)`: a deprojected screen ray onto a horizontal plane; false when parallel. The one way to turn the cursor into a world point (any projection).
- `ReplaceCounterTokens(Text, Encounter)`: each `{Counter:<Name>}` becomes that step counter's value; any text that may carry the token calls it (`GeoCatSpeechAction`).
- `DrawFromShuffledBag(Bag, Pool, Last)` (template): the one shuffle bag (no repeat until the pool is spent, never the same twice in a row across a refill); the Cat line bank and `AGeoCat`'s idle variants draw through it.
- Editor builders (StateTree/Widget) live in `GeoTrinityEditor`.
