# Abilities/Square

Square (Tank) kit. Wall deploy = shared `UGeoDeployAbility` with `DeployableActorClass = BP_Wall`, no Square subclass.

| File | Role |
|---|---|
| `GeoShieldBurstPassiveAbility.h` | Auto-attack damage fills a gauge; at 100% sends a bouncing shield burst to allies |
| `GeoDetonateWallsAbility.h` | Forward ray recalling own walls, boosting damage/shield by wall count |
| `GeoSacrificeBeamAbility.h` | Channel beam marking allies as sacrificed |
| `GeoSacrificeDetonateAbility.h` | Detonation ray dealing `BaseDamage + SacrificeValue` |

## Shield burst
- Binds `OnDamageDealt`; `Charge()` at full, after `ChargeTime` spawns `AGeoShieldBurstProjectile` toward the nearest ally.
- Each enemy bounce adds `EnemyBounceAdditiveMultiplier × base` (linear) to shield and radius.
- Visual gauge through `UShieldBurstPassiveComponent`; in-flight bursts end on Square revive.
- `IsReady` = avatar's `UShieldBurstPassiveComponent` `GaugeRatio >= 1` (replicated); no component → false. No class gauge (Detonate owns Square's ring).

## Detonate walls
- Pass 1 counts + recalls own `AGeoWall`s on the ray (no explosion), multiplier = `WallBoostMultiplier × count`.
- Pass 2: `BaseDamage × mult` to enemies, `BaseShield × mult` to allies (inline effect data).
- One `FireRay()` from `Fire` and `OnFireTargetDataReceived`: walls counted everywhere, recall/apply server-only, cue local-only (`RawMagnitude` = wall count).

## Sacrifice pair (one input, two abilities)
- Same `InputTag.Special`; both `bActivateOnFreshPressOnly`.
- Value lives in replicated `UCharacterAttributeSet::SacrificeValue` (shared by both abilities and HUD).
- Beam: `Fire()` applies `DetonateReadyEffect`; beam marks allies/neutrals with `SacrificeMarkEffect` (`Status.Sacrificed`); `ActivationBlockedTags = Status.Square.DetonateReady`; owns the cooldown.
- Redirect: the beam instance binds `AGeoGameState::OnIncomingDamage` in `Fire` (server) and unbinds in `EndAbility`; attribute-set code never names it.
  - Takes a hit only for a victim in its own `SacrificedActors` (the registry; the mark GE is the status + cue), still carrying `Status.Sacrificed` (a purged mark ends the redirect), while the Square lives, and not one carrying `bDoNotRedirectSacrifice`; nothing left of the hit (`Damage - Spared` is 0) → no-op.
  - Takes over what is left of the hit (`Damage - Spared`): victim takes 0, `SacrificeValue` grows by what it would actually have lost (health + shield, overkill dropped), share split across alive walls + Square (Square last), applied with `bDoNotRedirectSacrifice`, original attacker kept as source.
  - Stage 1 of the hit, before Bond's share (`AttributeSet/CLAUDE.md`): a sacrificed victim shares nothing.
- Detonate: plain `UGeoGameplayAbility`; BP `ActivationRequiredTags = Status.Square.DetonateReady`, `CancelAbilitiesWithTag = Ability.Spell.SacrificeBeam`, no cooldown; server ray via `GetInteractableActorsInLine(Hostile)`, then resets attribute and removes ready tag.
- Death: `APlayableCharacter::DeathLogic` purges GEs and zeroes `SacrificeValue`.
- Readiness = armed, never the value: Detonate's `IsReady` = `Status.Square.DetonateReady` on the ASC. `GetClassGauge` fill = `SacrificeValue / SacrificeForFullGauge` (property on Detonate, fill only), ready = armed. 100 of 200 sacrificed + armed → ring half full and pulsing, slot frame lit.
- `GetGaugeLabel` = `SacrificeValue` rounded (damage absorbed so far), only while > 0; nothing at 0.
- Ability bar shows the pair as one slot (slots hold every ability sharing an InputTag).
