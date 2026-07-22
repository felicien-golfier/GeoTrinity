# Actor/Deployable

All deployable actors. **Base class: `AGeoDeployableBase`** — extend this for new deployables.

## `GeoDeployableBase.h` — abstract base
Drain GE, blink-before-expiry, recall, explode, Blueprint hooks.

**Lifecycle:** spawned server-side by `ADeployableSpawnerProjectile::EndProjectileLife()` → `InitDrain()` computes drain rate → `Tick()` applies drain (server) → health/duration zero triggers blink timer → `Expire()` (hide, fire event, delayed destroy).

**`Recall(Value)` is the only valid end-of-life path** — never call `Expire()`/`Destroy()` directly. Calls `RecallEffect(Value)` then `Expire()`; on non-server also fires the recall cue locally (safety path).

**Override points:**
- `RecallEffect(Value)` — server-side "what happens at end of life" (default no-op).
- `ExplodeEffect(...)` — per-target callback inside `Explode()` (default applies `EffectDataArray`).
- `Explode(Value)` — server-only sphere overlap at `Params.Size`, filtered by `ExplodeAttitude` bitmask (default Hostile); call from inside `RecallEffect()` for an on-death explosion.

**`FInteractableActorData`:** `Owner` = ASC/team-check source; `Instigator` = origin avatar for cue params/direction (may differ, e.g. boss minions).

**VFX/cue rule — never multicast for recall/expiry:** clients fire the cue locally from `OnRep_Active` (bActive→false); server doesn't need to (OnRep doesn't run on the setter). One cue per client, no multicast.

**Spawn-push (`bPushActorsOnSpawn`):** server-only, disables capsule collision, root-motion-pushes overlapping characters outward, re-enables collision after 0.3s. A blocked push redirects toward the fighting arena's `FightCenter` — only fight-spawned deployables (boss pillar) use this flag, so a push with no active fight is a config bug.

**Key fields:** `bUseRegularDrain`/`DrainMagnitudePerSecond`; `bAutoRecallAtEndLife` (Recall vs Expire at blink end); `bSuppressDrainDamageVisuals`; `bDestroyOldestWhenLimitReached` (oldest same-class deployable expired by `RegisterDeployable` when slot full — `CanDeploy` always true in this mode); `ExplodeAttitude`.

**`CombattantWidgetComponent`** — world-space health bar, auto-created in the base constructor from `GameDataSettings::CombattantWidgetComponentClass` (soft class — gameplay never names the UI type), attached to `WidgetAnchorComponent`. Null on dedicated server. Bound to owner's ASC in `BeginPlay`. Per-BP tuning (`HealthBarDrawSize`/`Location`/`WidgetClassOverride`) is `EditDefaultsOnly` since the runtime-resolved component can't expose its own Details.

**`FDeployableData`** (extends `FInteractableActorData`): `EffectDataArray`, `Params` (`FDeployableDataParams`: BlinkDuration, LifeDrainMaxDuration, Size, Value), `PredictionKey`, `AbilityTag` (forwarded to every effect application — lets e.g. turret retargeting detect the originating ability).

## Subfolders
- `Wall/` — draining-health wall, no block/trigger, consumed by Detonate.
- `HealingZone/` — static HoT zone.
- `BuffPickup/` — launched pickup, applies random buff on collect.
- `Turret/` — auto-targets and fires on a timer.
- `Pillar/` — blocks the boss's devastating wave; explodes via `Recall()` at zero health.
- `Mine/` — hex-boss mine: drain-driven fuse, radial burst, defused if its tile dies.
- `BombZone/` — hex-boss tile bomb: rides its carrier through the drain, plants + carves tiles at the blink.

**On the hex arena, every deployable falls with its tile** — `AGeoHexArena`'s fall-check tick `Recall()`s anything over a destroyed tile, player-placed included. Subclasses with recall side effects (e.g. `AGeoMine`) must account for this path.
