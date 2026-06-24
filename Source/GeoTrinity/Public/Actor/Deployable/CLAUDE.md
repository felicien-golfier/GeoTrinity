# Actor/Deployable

All deployable actors. **Base class: `AGeoDeployableBase`** — extend this for new deployables.

## `GeoDeployableBase.h` — abstract base
Handles drain GE, blink-before-expiry, recall, explode, and Blueprint events.

**Lifecycle:**
1. Spawned by `ADeployableSpawnerProjectile::EndProjectileLife()` on server
2. `InitDrain()` — computes `DrainMagnitudePerSecond` from `Params.LifeDrainMaxDuration`; override to change drain behavior
3. `Tick()` — applies drain damage each tick on server
4. On health/duration zero → blink timer → `Expire()` → hides actor, fires `OnDeployableExpiredEvent`, destroys after `TimeBeforeDestroyAtExpire`
5. **`Recall(Value)`** — the ONLY valid end-of-life path. Calls `RecallEffect(Value)` → `Expire()`. On non-server machines it also calls `ExecuteCue(RecallGameplayCueTag, ...)` (safety path; clients normally receive the cue via `OnRep_Active`). Never call `Expire()` or `Destroy()` directly — always go through `Recall()`.

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
- Clients: `OnRep_Active()` fires when `bActive` replicates to false, then calls `ExecuteCue(RecallGameplayCueTag, ...)` locally.
- `Recall()` also calls `ExecuteCue(RecallGameplayCueTag, ...)` when running on a non-server machine (safety path for edge cases).
- The server itself does not fire the cue — `OnRep` does not trigger on the machine that set the value.
- Result: every client executes the cue exactly once via replication, no multicast needed.

**Spawn-push helper — `PushAway()`:**
- If `bPushActorsOnSpawn = true`, `InitInteractable` calls `PushAway()` on the server before Super
- Disables capsule collision, root-motion-pushes all overlapping characters outward (`FRootMotionSource_MoveToForce`, 0.15 s), then re-enables blocking collision after 0.3 s
- Push radius = `Params.Size` if set, otherwise the capsule's scaled radius

**Key fields:**
- `bUseRegularDrain`, `DrainMagnitudePerSecond` — drain config
- `bAutoRecallAtEndLife` — if true, calls `Recall()` when blink timer expires; otherwise calls `Expire()`
- `bSuppressDrainDamageVisuals = true` — hides hit flash during drain ticks
- `bPushActorsOnSpawn` — enables spawn-time push (see above); server only
- `bDestroyOldestWhenLimitReached` — if true, `CanDeploy` always returns true; the oldest deployable of the same class is expired by `RegisterDeployable` when the slot is full
- `ExplodeAttitude` — bitmask of team attitudes targeted by `Explode()` (default: `Hostile`)
- `CombattantWidgetComponent` — the world-space health bar widget component, held as the engine base `UWidgetComponent` (the concrete `UGeoCombattantWidgetComp` lives in the `GeoTrinityUI` module). **Created automatically in the base constructor** from `GameDataSettings::CombattantWidgetComponentClass` (a soft class, so gameplay never names the UI type) and attached to a non-rotating `WidgetAnchorComponent` — exactly like `AGeoCharacter`. Null on the dedicated server (UI class not shipped). Bound to the owner's ASC in `BeginPlay` (`BindToOwnerASC`, after attributes exist) and hidden there for non-damageable deployables. Publicly accessible for callers that need to show/hide it. Every deployable BP inherits it; no per-BP setup needed.
- Per-BP health-bar tuning (the component's own Details panel can't expose these because its class is runtime-resolved): `HealthBarDrawSize` (`FVector2D`), `HealthBarLocation` (`FVector`, relative to the attach parent), `HealthBarWidgetClassOverride` (`TSoftClassPtr<UUserWidget>`, falls back to `GameDataSettings::DefaultDeployableHealthBarWidgetClass`), and `HealthBarAttachParentName` (`FName` — the inherited widget component can't be reparented in the BP SCS tree, so name its parent component here; empty defaults to `WidgetAnchorComponent`). All `EditDefaultsOnly`, applied/re-attached in `BeginPlay`. (`FComponentReference` was avoided: its component-picker doesn't populate when editing class defaults, only placed instances.)
- `GetDurationPercent()` — health ratio 0..1; drives health bar
- `DestroyOldestWhenLimitReached()` — public getter for `bDestroyOldestWhenLimitReached`; read by `UGeoDeployableManagerComponent`
- `Expire(float TimeBeforeDestroy)` / `Expire()` — `Expire()` uses the default `TimeBeforeDestroyAtExpire`; never call directly; always go through `Recall()` for proper end-of-life
- `GetData()` — pure virtual; subclasses return their `FDeployableData`

**`FDeployableDataParams`** (passed from `GeoDeployAbility`):
- `BlinkDuration`, `LifeDrainMaxDuration`, `Size`, `Value`

**`FDeployableData`** (extends `FInteractableActorData`):
- `EffectDataArray`, `Params`, `PredictionKey`
- `AbilityTag` — tag of the ability that spawned this deployable. Forwarded to every `ApplyEffectFromEffectData` call the deployable makes, so `FDamageEffectData::UpdateContextHandle` can inspect the originating ability's CDO (e.g. set `bIsFromBasicAbility` for turret retargeting).

## `Wall/` — Damageable wall with a draining health pool; does not block/trigger; consumed by Detonate's ray to boost it. See `Wall/CLAUDE.md`.
## `HealingZone/` — Static zone that heals allies over time while they stand inside. See `HealingZone/CLAUDE.md`.
## `BuffPickup/` — Launched pickup that applies a random buff to whoever collects it. See `BuffPickup/CLAUDE.md`.
## `Turret/` — Auto-targets and fires at the nearest enemy on a timer. See `Turret/CLAUDE.md`.
## `Pillar/` — Pillar that blocks the boss's devastating wave; explodes via `Recall()` when health hits zero. See `Pillar/CLAUDE.md`.
