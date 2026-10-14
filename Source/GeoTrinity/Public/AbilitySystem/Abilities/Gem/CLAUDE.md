# Abilities/Gem

Core-gem passives. A Core gem (`FGeoGemInfo::GrantedAbility`) grants one of these to its holder; the gem system itself is in `Gem/CLAUDE.md`.

| File | Role |
|---|---|
| `GeoCorePassiveAbility.h` | Base: active from grant, `BindEvent` binds the rule to the delegate it reacts to, unbinds on end |
| `GeoBondPassiveAbility.h` | Holder absorbs `SharedDamageFraction` of damage other living players take |
| `GeoKinshipPassiveAbility.h` | A destroyed own deployable → a random ally deploys their own where it stood (Cooldown GE) |
| `GeoRelayPassiveAbility.h` | Dash snaps toward the nearest own deployable within `SnapHalfAngle` |
| `GeoSplitPassiveAbility.h` | A thrown deployable places a reduced-health copy nearby (Cooldown GE) |
| `GeoWakePassiveAbility.h` | A dash leaves a deployable at its start (Cooldown GE) |

- Each Core's Blueprint child carries the `Ability.Type.Passive` + `Ability.Spell.<Core>` asset tags, the `AbilityInfo` entry (Shared, not given at startup) and tunables. Built by `AI/Python/Gem/gem_core_passives.py`.
- Cooldown = the ability's own Cooldown GE (`CooldownGameplayEffectClass` = `GE_CD_Core_<Core>`, tag `Ability.Cooldown.<Core>`): `TryTrigger` tests it (`CheckCooldown`) and applies it on fire (`ApplyCooldown`), so it replicates and the owning client's HUD sweeps it through `AGeoHUD::GetAbilityCooldown` (`HUD/AbilityBar/CLAUDE.md`). Seconds live on the GE, built by `gem_core_passives.py`.
- Icons: `SourceArt/Icons/Core_<Gem>.svg` (one per Core gem, ability or stat-only), drawn after the design canvas board `A-CoreIcons`, in the gem's `Color`; `import_icons.py` fills each ability Core's `AbilityIcon`, which the passive tile and the abilities page draw.
- Deployable placers (Split, Kinship, Wake) place through `GeoASLib::SpawnDeployableFromAbility` and each carries its own `StartHealthRatio` (BP-tweakable, 0..1): the copy starts at that share of its normal max health, max unchanged. Default 0.5 on each. Split's copy is the holder's deploy ability spawning again near the landed one, with the landed one's `Params.DeployDistance`, so it carries Leverage; Kinship and Wake place without a throw, hence no Leverage.
- Event-raising code never names a Core; the passive binds to the event.
- Death: `UGeoCorePassiveAbility::OnOwnerDeath` is a no-op, every Core passive stays active and bound through death and rez, with the gem stats GE behind it (`Gem/CLAUDE.md`). Cooldown GEs are purged at death like any GE.
- A Core whose rule needs a living holder checks `IsDead()` in its handler: Bond ignores damage while the holder is dead (no health pool to absorb with).
- Passive tile frame: default `IsReady` (`Base/CLAUDE.md`). Cooldown Cores (Wake, Split, Kinship) lit when off cooldown; Bond, Relay and any Core without a Cooldown GE never lit.
- Bond share is taken before shield/health from what the hit's stage 1 left (`AGeoGameState::OnIncomingDamageShare`) with `bDoNotRedirectSacrifice`; a hit the Martyr redirect took over shares nothing.
- Kinship counts only hit-to-zero after blink, not drain or recall; ally owns the copy, spends no charge.
