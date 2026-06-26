# Characters

All playable and enemy character classes plus their components.

## Hierarchy

```
AGeoCharacter  (base — IAbilitySystemInterface, IGenericTeamAgentInterface)
├── APlayableCharacter  (ASC on PlayerState, Enhanced Input, runtime class-switching)
└── AEnemyCharacter     (own ASC, StateTree AI, round-robin firing points)
```

## Files

| File | Role | Reference File
|---|---|
| `GeoCharacter.h` | Abstract base — ASC, input, movement, health bar, game feel | GeoCharacter.md
| `PlayableCharacter.h` | Human player — input forwarding, `ChangeClass()`, deploy gauge, charge-beam gauge,
downed/revive | PlayableCharacter.md
| `EnemyCharacter.h` | AI enemy — own ASC, boss-death handling (`OnHealthChanged` / `ResetForNewAttempt`) |
EnemyCharacter.md
| `PlayerClassTypes.h` | `EPlayerClass` enum: `Triangle=1`, `Circle=2`, `Square=3`, `All=4` |

## Death / downed flow (PlayableCharacter)

On health ≤ 0, `Death()` (server) sets replicated `bIsDead`, calls `StopAllSpawnedElements()` (base — expires
deployables) + `StopCharacter()` (disable input, stop+disable movement, disable collision, swap mesh to the class death
material via `SetDeathMaterial`), then `GameState->NotifyPlayerDied()` which triggers `HandlePotentialWipe()` during
InProgress or immediately revives the player outside InProgress. The corpse **stays in place** — no per-death teleport.
When all players are dead the GameState resets the boss and requests WaitingToStart; `HandleMatchIsWaitingToStart`
teleports everyone to the entrance and calls `Revive()` per player (cancels active abilities, removes all gameplay
effects, re-applies class visuals via `ApplyClassData()`, then `GiveLife()` — re-applies per-class default attributes
and restarts passives — then `RestartCharacter()`). The state replicates to
clients via `OnRep_IsDead(bool)`, which calls `DeathLogic()` or `ReviveLogic()` directly — same bodies that run on
the server. **`RemoveActiveEffects` is server-guarded inside both**: it only acts on the authoritative ASC, and the
removal replicates to clients. Calling it client-side is a no-op that would otherwise leave locally-predicted effects
(e.g. dash cooldown) lingering after death — the cause of dash being unavailable on the client after respawn. Each class entry in `ClassData` (`FPlayerClassData`) configures `AliveMaterial` + `DeathMaterial`;
both swap on mesh slot 0.

## Component Subfolder

| Component                       | Role                                                                                                     |
|---------------------------------|----------------------------------------------------------------------------------------------------------|
| `GeoCharacterMovementComponent` | Cached base speed, `ApplySpeedMultiplier(float)`                                                         |
| `GeoDeployableManagerComponent` | Tracks deployables, enforces max count, `ForceExpireAll()`                                               |
| `GeoGameFeelComponent`          | Hit flash, recoil spring, cue rate-limiting                                                              |
| `ShieldBurstPassiveComponent`   | Square passive gauge — replicated `GaugeRatio`, `SetGaugeRatio()`                                        |
| `GeoBeamVFXComponent`           | Replicated beam VFX; dynamically added/removed by beam abilities; `SetBeamState` drives the Niagara beam |

See `Component/CLAUDE.md` for full details.
