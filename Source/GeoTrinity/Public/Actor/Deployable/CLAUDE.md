# Actor/Deployable

All deployable actors. **Base class: `AGeoDeployableBase`** — extend this for new deployables.

## `GeoDeployableBase.h` — abstract base
Handles drain GE, blink-before-expiry, recall, explode, and Blueprint events.

**Lifecycle:**
1. Spawned by `ADeployableSpawnerProjectile::EndProjectileLife()` on server
2. `InitDrain()` — computes `DrainMagnitudePerSecond` from `Params.LifeDrainMaxDuration`; override to change drain behavior
3. `Tick()` — applies drain damage each tick on server
4. On health/duration zero → blink timer → `Expire()` → hides actor, fires `OnDeployableExpiredEvent`, destroys after `TimeBeforeDestroyAtExpire`
5. **`Recall(Value)`** — the ONLY valid end-of-life path. Calls `RecallEffect(Value)` → `ExecuteRecallCue()` → `Expire()`. Never call `Expire()` or `Destroy()` directly — always go through `Recall()`.

**Override points for subclasses:**
- `RecallEffect(Value)` — called inside `Recall()` on the server. Put "what happens when this deployable ends" logic here (apply effects, call `Explode()`, etc.). Default: no-op.
- `ExplodeEffect(Value, SourceASC, Actor, TargetASC)` — called once per overlapping valid target inside `Explode()`. Default: applies `EffectDataArray` from the owner's ASC.

**Explode helper — `Explode(Value)`:**
- Server-only sphere overlap at `GetActorLocation()` with radius `Params.Size`
- Filters by `ExplodeAttitude` bitmask (default: `Hostile`) — skips actors on the same attitude
- Uses `GetData()->Owner` for the source ASC and team check
- Calls `ExplodeEffect()` per valid target — override `ExplodeEffect` to change what is applied
- Call `Explode(Value)` from inside `RecallEffect()` when you want an explosion on end-of-life

**`FInteractableActorData` — Owner vs Instigator:**
- `Owner` — the actor whose ASC is used for GAS (applying effects, team checks). Always a character with a valid ASC.
- `Instigator` — the "origin" avatar actor (e.g. the actual player pawn). Used for direction/cue params. May differ from Owner in edge cases (e.g. boss minions).
- Rule: use `GetData()->Owner` for ASC lookups and team attitude checks; use `GetData()->Instigator` for world position/direction (e.g. `GetRecallCueParams`).

**Blueprint events:**
- `OnBlinkStarted()` — `BlueprintNativeEvent`; override in BP for pre-expiry visual

**VFX / Gameplay Cue rule:** Never multicast RPC for recall/expiry VFX. Gameplay cues are called **locally only**:
- Server/owner: `ExecuteRecallCue()` called directly in `Recall()`.
- Clients (non-owner): `OnRep_Expired()` fires when `bActive` replicates, then calls `ExecuteRecallCue()` locally.
- Result: every machine executes the cue exactly once, no multicast needed.

**Key fields:**
- `bUseRegularDrain`, `DrainMagnitudePerSecond` — drain config
- `bAutoRecallAtEndLife` — if true, calls `Recall()` when blink timer expires; otherwise calls `Expire()`
- `bSuppressDrainDamageVisuals = true` — hides hit flash during drain ticks
- `ExplodeAttitude` — bitmask of team attitudes targeted by `Explode()` (default: `Hostile`)
- `GetDurationPercent()` — health ratio 0..1; drives health bar
- `GetData()` — pure virtual; subclasses return their `FDeployableData`

**`FDeployableDataParams`** (passed from `GeoDeployAbility`):
- `BlinkDuration`, `LifeDrainMaxDuration`, `Size`, `Value`

**`FDeployableData`** (extends `FInteractableActorData`):
- `EffectDataArray`, `Params`, `PredictionKey`

## `Mine/` — Explodes on overlap, damages enemies and shields allies. See `Mine/CLAUDE.md`.
## `HealingZone/` — Static zone that heals allies over time while they stand inside. See `HealingZone/CLAUDE.md`.
## `BuffPickup/` — Launched pickup that applies a random buff to whoever collects it. See `BuffPickup/CLAUDE.md`.
## `Turret/` — Auto-targets and fires at the nearest enemy on a timer. See `Turret/CLAUDE.md`.
## `Pillar/` — Pillar that blocks the boss's devastating wave; explodes via `Recall()` when health hits zero. See `Pillar/CLAUDE.md`.
