# Abilities/Square

Square (Tank) class abilities.

---

## Wall deployer — uses `GeoDeployAbility` directly (no Square-specific subclass)
The wall is deployed by the shared `UGeoDeployAbility` (`Common/`) configured in BP with `DeployableActorClass = BP_Wall`.
- Normal **flat GAS cost/cooldown** — no HP sacrifice
- Wall spawned via `ADeployableSpawnerProjectile` → `AGeoWall`

---

## `GeoShieldBurstPassiveAbility.h` — passive burst shield
Auto-attack damage dealt fills a gauge; at 100% a shield burst is sent to nearby allies.

Key fields:
- `GaugeFillThreshold` — damage needed to fill gauge
- `ShieldAmount` (`FScalableFloat`) — shield granted per burst; scales with ability level
- `EnemyBounceMultiplier` — multiplier applied to `AGeoShieldBurstProjectile` per enemy bounce
- `ChargeTime = 1s` — wind-up before burst fires

Uses `UShieldBurstPassiveComponent` for the replicated `GaugeRatio` visual on the character material.

Flow:
1. Ability binds to `OnDamageDealt` delegate on owner's ASC
2. Damage increments internal gauge
3. At 100%: `Charge()` starts, `ChargeTime` later → spawns `AGeoShieldBurstProjectile` toward nearest ally
4. Projectile bounces off enemies (multiplying `ShieldAmount`), shields first ally it hits
5. In-flight bursts end when the Square revives — base `AGeoProjectile` binds to the instigator's `OnRevived` delegate

---

## `GeoDetonateWallsAbility.h` — boosting ray
A ray in front of the character (like `GeoChargeBeamAbility`), length `GeneralSpellDistance`, width `LineHalfWidth`.
- **Pass 1**: counts the player's own `AGeoWall`s on the ray and **recalls** them (consumed, no explosion). Multiplier = `WallBoostMultiplier * WallCount` (each wall adds that fraction of base output; no walls → ×0, so the ray does nothing without a consumed wall).
- **Pass 2**: along the same ray, instantly deals `BaseDamage * Multiplier` to enemies and grants `BaseShield * Multiplier` shield to allies (inline `FDamageEffectData`/`FShieldEffectData`, mirroring `GeoMoiraBeamAbility`).
- Target scan: `GeoASLib::GetInteractableActorsInLine(... TeamAttitudeMask::All ...)`; walls skipped in pass 2.
- Single `FireRay()` is called from `Fire` (host/client) and `OnFireTargetDataReceived` (server). It counts walls everywhere (client needs the count for the cue scale), but **recalls walls + applies damage/shield only on the server**, and fires the cue **only on the locally-controlled client**.
- Ray VFX: `FireGameplayCueTag` (local cue, like `GeoChargeBeamAbility::FireGameplayCue`) — `CueParams.Location` = ray endpoint, `Normal` = aim direction, `RawMagnitude` = consumed wall count (lets the Niagara beam scale with walls eaten).
- `WallBoostMultiplier`, `LineHalfWidth`, `BaseDamage`, `BaseShield` (`FScalableFloat`, level-scaled), `FireGameplayCueTag` configured in BP.

---

## `GeoSacrificeBeamAbility.h` + `GeoSacrificeDetonateAbility.h` — sacrifice pair (one button, two abilities)
Two abilities on the same InputTag (`InputTag.SpecialAlternative`); both set `bActivateOnFreshPressOnly` so the per-frame Held input can never chain-activate one right after the other. The accumulated value lives in the replicated `UCharacterAttributeSet::SacrificeValue` attribute (not on either ability), so both abilities and the HUD read the same state.

**`GeoSacrificeBeamAbility` (channel)** — extends `UGeoChannelBeamAbility`. On Fire it applies `DetonateReadyEffect` (infinite `GE_DetonateReady` granting `Status.Square.DetonateReady`) to the Square, then channels: every ally/neutral inside the beam gets `SacrificeMarkEffect` (infinite GE granting `Status.Sacrificed` + WhileActive cue); leaving the beam removes it. `ActivationBlockedTags = Status.Square.DetonateReady` (BP) — the channel cannot restart until the detonation consumes the tag. Ends at `MaxChannelDuration` (detonation stays armed) or when the detonate cancels it. Owns the pair's cooldown (`AtActivate`).

**Damage redirection** — `UGeoAttributeSetBase::PostGameplayEffectExecute` calls the static `TryRedirectIncomingDamage(VictimASC, DamageContext, Damage)` before shield absorption: if the victim carries `Status.Sacrificed` (and the damage isn't excluded via the `bDoNotRedirectSacrifice` context flag), the victim takes **zero** damage. Fully instance-free: the mark GE alone identifies the Square (its context's original instigator ASC) and the level (its spec), so no live channel instance is needed — the static `RedirectCapturedDamage` adds the amount to `SacrificeValue` and splits it equally across alive `AGeoWall`s (recomputed per event, re-validated at apply time, Square damaged last) + the Square, applied as normal damage with `bDoNotRedirectSacrifice=true`. The redirect shares keep the **original attacker's ASC as source** (falling back to the Square's if the attacker is gone), so the Square's own damage stats never scale them and combat stats credit the real attacker. Walls themselves can be sacrificed; the deployable life drain sets `bDoNotRedirectSacrifice` so it is never captured.

**`GeoSacrificeDetonateAbility` (detonation)** — plain `UGeoGameplayAbility`, DetonateWalls fire pattern. BP: `ActivationRequiredTags = Status.Square.DetonateReady`, `CancelAbilitiesWithTag = Ability.Spell.SacrificeBeam` (a press during the channel cancels it and detonates), **no cooldown**. Server: ray via `GetInteractableActorsInLine(Hostile)`, each enemy takes `BaseDamage + SacrificeValue`; then resets the attribute and removes the DetonateReady GE. Local: `FireGameplayCueTag` with `RawMagnitude = SacrificeValue`. Death disarms: `APlayableCharacter::DeathLogic` purges all GEs and zeroes `SacrificeValue`.

The ability bar shows the pair as **one slot** that swaps icons — see `GeoAbilitySlotWidget` (slots hold every ability sharing an InputTag; displayed = last active/activatable entry).

Channel BP config: `BeamHalfWidth`, `SacrificeMarkEffect`, `DetonateReadyEffect`, `MaxChannelDuration`, `BeamNiagaraSystem`/`BeamColor` (inherited). Detonate BP config: `LineHalfWidth`, `BaseDamage`, `FireGameplayCueTag`.
