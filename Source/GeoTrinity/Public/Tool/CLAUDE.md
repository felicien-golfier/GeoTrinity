# Tool

| Folder / file | Holds |
|---|---|
| `UGeoGameplayLibrary.h` | `GeoLib` Blueprint-callable helpers |
| `GeoAssetManager.h` | Custom asset manager (configured in `DefaultEngine.ini`) |
| `Team.h` | `ETeam` for `IGenericTeamAgentInterface` |
| `GeoDifficulty.h` | Run difficulty enum (Safe, Reduced, Original, Nightmare) |
| `Visual/` | `GeoColor`, `GeoIcon`, `GeoNiagaraParams` |
| `Net/` | `GeoHazardJudge`, `GeoNetcodeDebug` |
| `CatLine/` | The Cat's random lines read from `Data/CatLines.txt` → `CatLine/CLAUDE.md` |

## Difficulty
- Bit value = ability level: Safe 1, Reduced 2, Original 4, Nightmare 8. `GeoDifficultyMask::All` = every bit, Nightmare included.
- Tuning lives in the boss curve tables (`CT_BossStar`, `CT_BossHex`), one key per level. Nightmare key (8) = 1.5 × the Original key on every row (today only damage and health; speeds, sizes, counts and timings are pattern Blueprint properties that never scale; Hex mine / bomb health are flat defaults). A curve with no key at 8 clamps to its last key (Original); an `FScalableFloat` with no curve is constant on all four.
- `FEffectData::Difficulties` is a stored mask: an entry gated to "Reduced + Original" or "All" must carry the Nightmare bit too, or it drops out at level 8.
- Rewards: `UGeoGemCatalog` `DifficultyLootScale` / `DifficultyXpScale` hold Nightmare = 1.5. Pads (`AGeoDifficultyButton`) and leaderboard tabs list one entry per value.

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
