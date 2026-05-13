# Actor/Deployable

All deployable actors. **Base class: `AGeoDeployableBase`** — extend this for new deployables.

## `GeoDeployableBase.h` — abstract base
Handles drain GE, blink-before-expiry, recall, and Blueprint events.

**Lifecycle:**
1. Spawned by `ADeployableSpawnerProjectile::EndProjectileLife()` on server
2. `InitDrain()` — applies drain GE from `Params.LifeDrainMaxDuration`; override to change drain behavior
3. `Tick()` — manages blink timer countdown
4. On health/duration zero → `Expire()` → sets `bExpired`, starts `TimeBeforeDestroyAtExpire = 3s` timer, fires `OnDeployableExpiredEvent`
5. `Recall(Value)` — owner-initiated; **this is the ONLY destruction/end path**. Override `Recall()` for any "deployable triggers effect and ends" logic. **Never add a separate destruction method** (e.g. `OnXDestroyed`). When health hits zero, call `Recall()` — do not call `Expire()` or `Destroy()` directly.

**Blueprint events:**
- `OnBlinkStarted()` — `BlueprintNativeEvent`; pre-expiry visual (blink animation)
- `OnDeployableExpired()` not listed — use `OnDeployableExpiredEvent` delegate instead

**VFX / Gameplay Cue rule:** Never multicast RPC for recall/expiry VFX. Gameplay cues are called **locally only**:
- Server/owner: `ExecuteRecallCue()` called directly in `Recall()`.
- Clients (non-owner): `OnRep_Expired()` fires when `bExpired` replicates (`COND_SkipOwner` skips the owner), then calls `ExecuteRecallCue()` locally.
- Result: every machine executes the cue exactly once, no multicast needed.

**Key fields:**
- `bUseRegularDrain`, `DrainMagnitudePerSecond` — drain config
- `bSuppressDrainDamageVisuals = true` — hides hit flash during drain ticks
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
## `Pillar/` — Indestructible-until-damaged pillar that blocks the boss's devastating wave. See `Pillar/CLAUDE.md`.