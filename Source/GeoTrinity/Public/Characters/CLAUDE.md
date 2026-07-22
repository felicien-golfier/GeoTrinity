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
| `PlayableCharacter.h` | Human player — input forwarding, `ChangeClass()`, deploy/charge-beam gauges, downed/revive |
| `EnemyCharacter.h` | AI enemy — own ASC, boss-death handling (`OnHealthChanged`/`ResetForNewAttempt`) |
| `PlayerClassTypes.h` | `EPlayerClass` enum: `Triangle=1`, `Circle=2`, `Square=3`, `All=4` |

## Death/downed flow (PlayableCharacter)
On health ≤ 0, `Death()` (server) sets replicated `bIsDead`, calls `StopCharacter()`, then hands the death to the GameState (`NotifyPlayerDied`) — **death policy is the GameState's, not the arena's**: out of a fight the player revives on the spot; in a fight they stay down until the whole starting group is down, then everyone respawns at the arena's registered `CheckpointTag`. Corpse stays in place, no per-death teleport.

Replicates via `OnRep_IsDead(bool)`, which calls the same `DeathLogic()`/`ReviveLogic()` bodies as the server. **`RemoveActiveEffects` is server-guarded inside both** — client-side it's a no-op (otherwise locally-predicted effects like dash cooldown linger after death/respawn).

`Revive()` (on wipe respawn): cancels active abilities, removes all GEs, `StopAllSpawnedElements()` (deployables persist while down, expire on revive), re-applies class visuals + broadcasts `OnRevived`, then `GiveLife()` (re-applies per-class attributes, restarts passives), then `RestartCharacter()`.

Late joiners spawn **alive** even mid-fight — not in the fight-start `FightPlayers` snapshot, so they never block/trigger a wipe; a death while any match runs anywhere still leaves them down until it ends (no per-room aliveness).

Each `ClassData` entry (`FPlayerClassData`) sets `AliveMaterial`/`DeathMaterial`, both swapped on mesh slot 0.

## Component Subfolder
| Component | Role |
|---|---|
| `GeoCharacterMovementComponent` | Cached base speed, `ApplySpeedMultiplier(float)` |
| `GeoDeployableManagerComponent` | Tracks deployables, enforces max count, `ForceExpireAll()` |
| `GeoGameFeelComponent` | Hit flash, recoil spring, cue rate-limiting |
| `ShieldBurstPassiveComponent` | Square passive gauge — replicated `GaugeRatio`, `SetGaugeRatio()` |
| `GeoBeamVFXComponent` | Replicated beam VFX; dynamically added/removed by beam abilities; `SetBeamState` drives the Niagara beam |

See `Component/CLAUDE.md` for details.
