# Characters

All playable and enemy character classes plus their components.

## Hierarchy
```
AGeoCharacter  (base — IAbilitySystemInterface, IGenericTeamAgentInterface)
├── APlayableCharacter  (ASC on PlayerState, Enhanced Input, runtime class-switching)
└── AEnemyCharacter     (own ASC, StateTree AI, round-robin firing points)
```

## Files
| File | Role |
|---|---|
| `GeoCharacter.h` | Abstract base — ASC, input, movement, health bar, game feel |
| `PlayableCharacter.h` | Human player — input forwarding, `ChangeClass()`, deploy gauge, charge-beam gauge, downed/revive |
| `EnemyCharacter.h` | AI enemy — own ASC, boss-death handling (`OnHealthChanged` / `ResetForNewAttempt`) |
| `PlayerClassTypes.h` | `EPlayerClass` enum: `Triangle=1`, `Circle=2`, `Square=3`, `All=4` |

## Death / downed flow (PlayableCharacter)
On health ≤ 0, `Death()` (server) sets replicated `bIsDead`, calls `StopAllSpawnedElements()` (base — expires deployables) + `StopCharacter()` (disable input, stop+disable movement, disable collision, swap mesh to the class death material via `SetDeathMaterial`), then `GameState->NotifyPlayerDiedInFight()` which only decrements the alive counter. The corpse **stays in place** — no per-death teleport. When all players are dead the GameState resets the boss and requests WaitingToStart; `HandleMatchIsWaitingToStart` teleports everyone to the entrance and calls `Revive()` per player (re-applies base attributes via `InitializeDefaultAttributes` like the enemy, then `RestartCharacter()`). The state replicates to clients via `OnRep_IsDead(bool)`, which runs `StopCharacter`/`RestartCharacter` there too. Each class entry in `ClassData` (`FPlayerClassData`) configures `AliveMaterial` + `DeathMaterial`; both swap on mesh slot 0.

## Component Subfolder
| Component | Role |
|---|---|
| `GeoCharacterMovementComponent` | Cached base speed, `ApplySpeedMultiplier(float)` |
| `GeoDeployableManagerComponent` | Tracks deployables, enforces max count, `RecallAll()` |
| `GeoGameFeelComponent` | Hit flash, recoil spring, cue rate-limiting |
| `ShieldBurstPassiveComponent` | Square passive gauge — replicated `GaugeRatio`, `SetGaugeRatio()` |

See `Component/CLAUDE.md` for full details.
