# Abilities/Square

Square (Tank) class abilities.

---

## Wall deployer — uses `GeoDeployAbility` directly (no Square-specific subclass)
Shared `UGeoDeployAbility` (`Common/`) configured in BP with `DeployableActorClass = BP_Wall`. Normal flat GAS cost/cooldown, no HP sacrifice. Spawned via `ADeployableSpawnerProjectile` → `AGeoWall`.

## `GeoShieldBurstPassiveAbility.h` — passive burst shield
Auto-attack damage fills a gauge; at 100% a shield burst is sent to nearby allies.
- `GaugeFillThreshold`, `ShieldAmount` (`FScalableFloat`, level-scaled), `EnemyBounceMultiplier`, `ChargeTime = 1s`
- Uses `UShieldBurstPassiveComponent` for the replicated `GaugeRatio` material visual
- Flow: binds `OnDamageDealt` → gauge increments → at 100% `Charge()` → after `ChargeTime`, spawns `AGeoShieldBurstProjectile` toward nearest ally → bounces off enemies (multiplying `ShieldAmount`), shields first ally hit
- In-flight bursts end on Square revive (`AGeoProjectile` binds to instigator's `OnRevived`)

## `GeoDetonateWallsAbility.h` — boosting ray
Forward ray (like `GeoChargeBeamAbility`), length `GeneralSpellDistance`, width `LineHalfWidth`.
- Pass 1: counts + recalls the player's own `AGeoWall`s on the ray (consumed, no explosion). Multiplier = `WallBoostMultiplier * WallCount` (no walls → ×0)
- Pass 2: same ray, `BaseDamage * Multiplier` to enemies, `BaseShield * Multiplier` shield to allies (inline `FDamageEffectData`/`FShieldEffectData`)
- Single `FireRay()` called from `Fire` (host/client) and `OnFireTargetDataReceived` (server): counts walls everywhere (client needs it for cue scale), but recalls/applies only server-side, cue only on the locally-controlled client
- `FireGameplayCueTag`: `RawMagnitude` = consumed wall count (scales Niagara beam)

## `GeoSacrificeBeamAbility.h` + `GeoSacrificeDetonateAbility.h` — sacrifice pair (one button, two abilities)
Same InputTag (`InputTag.SpecialAlternative`); both `bActivateOnFreshPressOnly` so Held input can't chain-activate one after the other. Accumulated value lives in replicated `UCharacterAttributeSet::SacrificeValue` (not on either ability) so both abilities and the HUD read the same state.

**`GeoSacrificeBeamAbility` (channel)** — extends `UGeoChannelBeamAbility`. `Fire()` applies `DetonateReadyEffect` (infinite GE granting `Status.Square.DetonateReady`), then channels: allies/neutrals in the beam get `SacrificeMarkEffect` (infinite GE granting `Status.Sacrificed` + cue); leaving removes it. `ActivationBlockedTags = Status.Square.DetonateReady` (BP) — can't restart until detonation consumes the tag. Ends at `MaxChannelDuration` (detonation stays armed) or on detonate cancel. Owns the pair's cooldown.

**Damage redirection** — `UGeoAttributeSetBase::PostGameplayEffectExecute` calls static `TryRedirectIncomingDamage` before shield absorption: if victim carries `Status.Sacrificed` (and not `bDoNotRedirectSacrifice`), victim takes zero damage. Instance-free (mark GE identifies the Square + level): `RedirectCapturedDamage` adds to `SacrificeValue`, splits equally across alive `AGeoWall`s + the Square (recomputed per event, Square damaged last), applied as normal damage with `bDoNotRedirectSacrifice=true`. Redirect shares keep the original attacker's ASC as source (fallback: Square's) so stats credit correctly. Walls can themselves be sacrificed; deployable life drain sets `bDoNotRedirectSacrifice` so it's never captured.

**`GeoSacrificeDetonateAbility` (detonation)** — plain `UGeoGameplayAbility`, DetonateWalls fire pattern. BP: `ActivationRequiredTags = Status.Square.DetonateReady`, `CancelAbilitiesWithTag = Ability.Spell.SacrificeBeam` (press during channel cancels + detonates), no cooldown. Server: ray via `GetInteractableActorsInLine(Hostile)`, each enemy takes `BaseDamage + SacrificeValue`, then resets the attribute and removes DetonateReady. Local: `FireGameplayCueTag` with `RawMagnitude = SacrificeValue`. Death disarms: `APlayableCharacter::DeathLogic` purges GEs and zeroes `SacrificeValue`.

Ability bar shows the pair as one slot that swaps icons (`GeoAbilitySlotWidget` — slots hold every ability sharing an InputTag, display last active/activatable entry).
